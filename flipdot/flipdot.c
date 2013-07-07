#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <bcm2835.h>
#include "flipdot.h"


#define ISBITSET(b,i) ((((b)[(i) >> 3]) & (1 << ((i) & 7))) != 0)
#define SETBIT(b,i) (((b)[(i) >> 3]) |= (1 << ((i) & 7)))
#define CLEARBIT(b,i) (((b)[(i) >> 3]) &=~ (1 << ((i) & 7)))

#define DATA(reg) (((reg) == ROW) ? (DATA_ROW) : (DATA_COL))
#define CLK(reg) (((reg) == ROW) ? (CLK_ROW) : (CLK_COL))
#define OE(reg) (((reg) == ROW) ? (OE_ROW) : (OE_COL))


void
flipdot_init(void)
{
	/* init ports */
	bcm2835_gpio_clr(STROBE);
	bcm2835_gpio_clr(OE1);
	bcm2835_gpio_clr(OE0);
	bcm2835_gpio_clr(DATA_ROW);
	bcm2835_gpio_clr(DATA_COL);
	bcm2835_gpio_clr(CLK_ROW);
	bcm2835_gpio_clr(CLK_COL);

	bcm2835_gpio_fsel(STROBE, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_fsel(OE1, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_fsel(OE0, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_fsel(DATA_ROW, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_fsel(DATA_COL, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_fsel(CLK_ROW, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_fsel(CLK_COL, BCM2835_GPIO_FSEL_OUTP);
}

void
flipdot_clear_to_0(void)
{
	flipdot_frame_t frame;
	memset(frame, 0x00, sizeof(frame));
	flipdot_display_frame(frame);
}

void
flipdot_clear_to_1(void)
{
	flipdot_frame_t frame;
	memset(frame, 0xFF, sizeof(frame));
	flipdot_display_frame(frame);
}

void
flipdot_clear(void)
{
	flipdot_clear_to_0();
}

void
flipdot_display_frame(const flipdot_frame_t *frame)
{
	uint8_t row_select[(DISP_ROWS + 7) / 8];
	uint8_t row_data[((DISP_COLS + 7) / 8) + 1];
	uint8_t *frameptr = frame;
	unsigned int offset;
	unsigned int rem = 0;

	for (unsigned int row = 0; row < DISP_ROWS; row++) {
#if (DISP_COLS % 8) != 0)
		uint8_t *rowptr = row_data;
		if (rem) {
			*rowptr++ = *frameptr++ & (0xFF << (8 - rem));
			offset = 8 - rem;
		} else {
			offset = 0;
		}
		memcpy(rowptr, frameptr, (DISP_COLS - rem) / 8);
		frameptr += (DISP_COLS - rem) / 8);
		rowptr += (DISP_COLS - rem) / 8);
		rem = 8 - ((DISP_COLS - rem) % 8);
		*rowptr = *frameptr & (0xFF >> rem);
#else
		memcpy(row_data, frameptr, DISP_COLS / 8);
		frameptr += DISP_COLS / 8);
		offset = 0;
		rem = 0;
#endif

		memset(row_select, 0, sizeof(row_select));
		SETBIT(row_select, row);						/* Set selected row */
		sreg_fill(COL, row_select, DISP_ROWS, 0);			/* Fill row select shift register */

		sreg_fill(ROW, row_data, DISP_COLS, offset);
		strobe();
		flip_to_0();
		flip_to_1();
	}
}

// TODO: skip unchanged rows
void
flipdot_display_diff(const flipdot_frame_t *diff_to_0, const flipdot_frame_t *diff_to_1)
{
	uint8_t row_select[(DISP_ROWS + 7) / 8];
	uint8_t row_data_to_0[((DISP_COLS + 7) / 8) + 1];
	uint8_t row_data_to_1[((DISP_COLS + 7) / 8) + 1];
	unsigned int frameidx = 0;
	unsigned int offset;
	unsigned int rem = 0;

	for (unsigned int row = 0; row < DISP_ROWS; row++) {
#if (DISP_COLS % 8) != 0)
		uint8_t *rowptr_to_0 = row_data_to_0;
		uint8_t *rowptr_to_1 = row_data_to_1;
		if (rem) {
			*rowptr_to_0++ = diff_to_0[frameidx] & (0xFF << (8 - rem));
			*rowptr_to_1++ = diff_to_1[frameidx] & (0xFF << (8 - rem));
			frameidx++;
			offset = 8 - rem;
		} else {
			offset = 0;
		}
		memcpy(rowptr_to_0, diff_to_0 + frameidx, (DISP_COLS - rem) / 8);
		memcpy(rowptr_to_1, diff_to_1 + frameidx, (DISP_COLS - rem) / 8);
		frameidx += (DISP_COLS - rem) / 8);
		rowptr_to_0 += (DISP_COLS - rem) / 8);
		rowptr_to_1 += (DISP_COLS - rem) / 8);
		rem = 8 - ((DISP_COLS - rem) % 8);
		*rowptr_to_0 = diff_to_0[frameidx] & (0xFF >> rem);
		*rowptr_to_1 = diff_to_1[frameidx] & (0xFF >> rem);
#else
		memcpy(row_data_to_0, diff_to_0 + frameidx, DISP_COLS / 8);
		memcpy(row_data_to_1, diff_to_1 + frameidx, DISP_COLS / 8);
		frameidx += DISP_COLS / 8;
		offset = 0;
		rem = 0;
#endif

		memset(row_select, 0, sizeof(row_select));
		SETBIT(row_select, row);						/* Set selected row */
		sreg_fill(COL, row_select, DISP_ROWS, 0);			/* Fill row select shift register */

		sreg_fill(ROW, row_data_to_0, DISP_COLS, offset);
		strobe();
		flip_to_0();

		sreg_fill(ROW, row_data_to_1, DISP_COLS, offset);
		strobe();
		flip_to_1();
	}
}


static inline void
_nanosleep(long nsec)
{
	struct timespec req;

	req.tv_sec = 0;
	req.tv_nsec = nsec;

	while (nanosleep(&req, &req) == -1 && errno == EINTR);
}

static void
sreg_push_bit(enum sreg reg, uint8_t bit)
{
	bcm2835_gpio_write(DATA(reg), (bit ? HIGH : LOW));
	_nanosleep(DATA_DELAY);

	bcm2835_gpio_set(CLK(reg));
	_nanosleep(CLK_DELAY);

	bcm2835_gpio_clr(CLK(reg));
	_nanosleep(CLK_DELAY);
}

static void
sreg_fill(enum sreg reg, const uint8_t *data, unsigned int count, unsigned int offset)
{
	unsigned int i = count;
	unsigned int j = 0;

	while (i--) {
		if (reg == ROW && j++ >= MODULE_COLS) {
			// skip unused register bits
			for (unsigned int k = 0; k < COL_GAP; k++) {
				sreg_push_bit(reg, 0);
			}
			j = 0;
		}
		sreg_push_bit(reg, ISBITSET(data, count + offset);
	}
}

static void
strobe(void)
{
	bcm2835_gpio_set(STROBE);

	_nanosleep(STROBE_DELAY);

	bcm2835_gpio_clr(STROBE);
}

static void
flip_to_0(void)
{
	bcm2835_gpio_clr(OE1);
	bcm2835_gpio_set(OE0);

	_nanosleep(FLIP_DELAY);

	bcm2835_gpio_clr(OE0);
}

static void
flip_to_1(void)
{
	bcm2835_gpio_clr(OE0);
	bcm2835_gpio_set(OE1);

	_nanosleep(FLIP_DELAY);

	bcm2835_gpio_clr(OE1);
}

/*
* Copyright (c) 2013 Franz Nord
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 3
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*
* For more information on the GPL, please go to:
* http://www.gnu.org/copyleft/gpl.html
*/


#include <stdint.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <bcm2835.h>
#include "flipdot.h"


#define ISBITSET(b,i) ((((b)[(i) >> 3]) & (1 << ((i) & 7))) != 0)
#define SETBIT(b,i) (((b)[(i) >> 3]) |= (1 << ((i) & 7)))
#define CLEARBIT(b,i) (((b)[(i) >> 3]) &=~ (1 << ((i) & 7)))

#ifndef _BV
#define _BV(x) (1 << (x))
#endif

#define DATA(reg) (((reg) == ROW) ? (DATA_ROW) : (DATA_COL))
#define CLK(reg) (((reg) == ROW) ? (CLK_ROW) : (CLK_COL))
#define OE(reg) (((reg) == ROW) ? (OE_ROW) : (OE_COL))

static uint8_t frame_a[FRAME_BYTE_COUNT];
static uint8_t frame_b[FRAME_BYTE_COUNT];
static uint8_t *frame_new, *frame_old;

static void sreg_push_bit(enum sreg reg, uint_fast8_t bit);
static void sreg_fill(enum sreg reg, uint8_t *data, uint_fast16_t count);
static void sreg_fill2(uint8_t *row_data, uint_fast16_t row_count, uint8_t *col_data, uint_fast16_t col_count);
static void strobe(void);
static void flip_to_0(void);
static void flip_to_1(void);


static inline void
_hw_init(void)
{
	/* init ports */
	bcm2835_gpio_clr(OE0);
	bcm2835_gpio_clr(OE1);
	bcm2835_gpio_clr(STROBE);
	bcm2835_gpio_clr(DATA_ROW);
	bcm2835_gpio_clr(CLK_ROW);
	bcm2835_gpio_clr(DATA_COL);
	bcm2835_gpio_clr(CLK_COL);

	bcm2835_gpio_fsel(OE0, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_fsel(OE1, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_fsel(STROBE, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_fsel(DATA_ROW, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_fsel(CLK_ROW, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_fsel(DATA_COL, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_fsel(CLK_COL, BCM2835_GPIO_FSEL_OUTP);
}

static inline void
_hw_shutdown(void)
{
	// TODO: disable pud?

	bcm2835_gpio_clr(OE0);
	bcm2835_gpio_clr(OE1);
	bcm2835_gpio_clr(STROBE);
	bcm2835_gpio_clr(DATA_ROW);
	bcm2835_gpio_clr(CLK_ROW);
	bcm2835_gpio_clr(DATA_COL);
	bcm2835_gpio_clr(CLK_COL);

	bcm2835_gpio_fsel(OE0, BCM2835_GPIO_FSEL_INPT);
	bcm2835_gpio_fsel(OE1, BCM2835_GPIO_FSEL_INPT);
	bcm2835_gpio_fsel(STROBE, BCM2835_GPIO_FSEL_INPT);
	bcm2835_gpio_fsel(DATA_ROW, BCM2835_GPIO_FSEL_INPT);
	bcm2835_gpio_fsel(CLK_ROW, BCM2835_GPIO_FSEL_INPT);
	bcm2835_gpio_fsel(DATA_COL, BCM2835_GPIO_FSEL_INPT);
	bcm2835_gpio_fsel(CLK_COL, BCM2835_GPIO_FSEL_INPT);
}

static inline void
_hw_set(uint_fast8_t gpio)
{
	bcm2835_gpio_set(gpio);
}

static inline void
_hw_clr(uint_fast8_t gpio)
{
	bcm2835_gpio_clr(gpio);
}

static inline void
_nanosleep(long nsec)
{
	struct timespec req;

	req.tv_sec = 0;
	req.tv_nsec = nsec;

	while (nanosleep(&req, &req) == -1 && errno == EINTR);
}


void
flipdot_init(void)
{
	_hw_init();

	memset(frame_a, 0x00, sizeof(frame_a));
	memset(frame_b, 0x00, sizeof(frame_b));
	frame_old = frame_a;
	frame_new = frame_b;
}

void
flipdot_shutdown(void)
{
	_hw_shutdown();
}

void
flipdot_clear_to_0(void)
{
	memset(frame_old, 0x00, FRAME_BYTE_COUNT);
	flipdot_display_frame(frame_old);
}

void
flipdot_clear_to_1(void)
{
	memset(frame_old, 0xFF, FRAME_BYTE_COUNT);
	flipdot_display_frame(frame_old);
}

/*
void
flipdot_clear(void)
{
	flipdot_clear_to_0();
}
*/

void
flipdot_display_row(uint8_t *rows, uint8_t *cols)
{
//	sreg_fill(ROW, rows, REGISTER_ROWS);
//	sreg_fill(COL, cols, REGISTER_COLS);
	sreg_fill2(rows, REGISTER_ROWS, cols, REGISTER_COLS);
	strobe();
	flip_to_0();
	flip_to_1();
}


void
flipdot_display_row_diff(uint8_t *rows, uint8_t *cols_to_0, uint8_t *cols_to_1)
{
//	sreg_fill(ROW, rows, REGISTER_ROWS);
//	sreg_fill(COL, cols_to_0, REGISTER_COLS);
	sreg_fill2(rows, REGISTER_ROWS, cols_to_0, REGISTER_COLS);
	strobe();
	flip_to_0();

	sreg_fill(COL, cols_to_1, REGISTER_COLS);
	strobe();
	flip_to_1();
}


void
flipdot_display_frame(uint8_t *frame)
{
	uint8_t rows[REGISTER_ROWS];
	uint8_t cols[REGISTER_COLS];
	uint8_t *frameptr;

	memcpy(frame_new, frame, FRAME_BYTE_COUNT);
	frameptr = (uint8_t *)frame_new;

	for (uint_fast16_t row = 0; row < REGISTER_ROWS; row++) {
		memcpy(cols, frameptr, REGISTER_COL_BYTE_COUNT);
		frameptr += REGISTER_COL_BYTE_COUNT;

		memset(rows, 0, sizeof(rows));
		SETBIT(rows, row);

		flipdot_display_row(rows, cols);
	}
}

void
flipdot_update_frame(uint8_t *frame)
{
	uint8_t rows[REGISTER_ROWS];
	uint8_t cols_to_0[REGISTER_COLS];
	uint8_t cols_to_1[REGISTER_COLS];
	uint8_t *frameptr_new;
	uint8_t *frameptr_old;
	uint_fast8_t row_changed;

	uint8_t *tmp = frame_old;
	frame_old = frame_new;
	frame_new = tmp;

	memcpy(frame_new, frame, FRAME_BYTE_COUNT);

	frameptr_new = (uint8_t *)frame_new;
	frameptr_old = (uint8_t *)frame_old;

	for (uint_fast16_t row = 0; row < REGISTER_ROWS; row++) {
		row_changed = 0;

		for (uint_fast16_t col = 0; col < REGISTER_COL_BYTE_COUNT; col++) {
			cols_to_0[col] = ~((*frameptr_old) & ~(*frameptr_new));
			cols_to_1[col] = (~(*frameptr_old) & (*frameptr_new));

			if (cols_to_0[col] != 0xFF || cols_to_1[col] != 0x00) {
				row_changed = 1;
			}

			frameptr_old++;
			frameptr_new++;
		}

		if (row_changed) {
			memset(rows, 0, sizeof(rows));
			SETBIT(rows, row);

			flipdot_display_row_diff(rows, cols_to_0, cols_to_1);
		}
	}
}

// Slow bit copy
void
flipdot_bitmap_to_frame(uint8_t *bitmap, uint8_t *frame)
{
	memset(frame, 0x00, FRAME_BYTE_COUNT);

	for (uint_fast16_t i = 0; i < DISP_PIXEL_COUNT; i++) {
		if (ISBITSET((uint8_t *)bitmap, i)) {
			SETBIT((uint8_t *)frame, i + ((i / MODULE_COLS) * COL_GAP));
		}
	}
}

void
flipdot_frame_to_bitmap(uint8_t *frame, uint8_t *bitmap)
{
	memset(bitmap, 0x00, DISP_BYTE_COUNT);

	for (uint_fast16_t i = 0; i < DISP_PIXEL_COUNT; i++) {
		if (ISBITSET((uint8_t *)frame, i + ((i / MODULE_COLS) * COL_GAP))) {
			SETBIT((uint8_t *)bitmap, i);
		}
	}
}

void
flipdot_display_bitmap(uint8_t *bitmap)
{
	uint8_t frame[FRAME_BYTE_COUNT];

	flipdot_bitmap_to_frame(bitmap, frame);
	flipdot_display_frame(frame);
}


void
flipdot_update_bitmap(uint8_t *bitmap)
{
	uint8_t frame[FRAME_BYTE_COUNT];

	flipdot_bitmap_to_frame(bitmap, frame);
	flipdot_update_frame(frame);
}

static void
sreg_push_bit(enum sreg reg, uint_fast8_t bit)
{
	if (bit) {
		_hw_set(DATA(reg));
	} else {
		_hw_clr(DATA(reg));
	}
#ifndef NOSLEEP
	_nanosleep(DATA_DELAY);
#endif

	_hw_set(CLK(reg));

#ifndef NOSLEEP
	_nanosleep(CLK_DELAY);
#endif

	_hw_clr(CLK(reg));

#ifndef NOSLEEP
	//_nanosleep(CLK_DELAY);
#endif
}

static void
sreg_fill(enum sreg reg, uint8_t *data, uint_fast16_t count)
{
	while (count--) {
		sreg_push_bit(reg, ISBITSET(data, count));
	}
}

static void
sreg_fill2(uint8_t *row_data, uint_fast16_t row_count, uint8_t *col_data, uint_fast16_t col_count)
{
	while (row_count || col_count) {
		if (row_count) {
			if (ISBITSET(row_data, row_count - 1)) {
				_hw_set(DATA(ROW));
			} else {
				_hw_clr(DATA(ROW));
			}
		}

		if (col_count) {
			if (ISBITSET(col_data, col_count - 1)) {
				_hw_set(DATA(COL));
			} else {
				_hw_clr(DATA(COL));
			}
		}

#ifndef NOSLEEP
		_nanosleep(DATA_DELAY);
#endif

		if (row_count) {
			_hw_set(CLK(ROW));
		}

		if (col_count) {
			_hw_set(CLK(COL));
		}

#ifndef NOSLEEP
		_nanosleep(CLK_DELAY);
#endif

		if (row_count) {
			_hw_clr(CLK(ROW));
		}

		if (col_count) {
			_hw_clr(CLK(COL));
		}

		if (row_count) {
			row_count--;
		}

		if (col_count) {
			col_count--;
		}
	}
}

static void
strobe(void)
{
	_hw_set(STROBE);

#ifndef NOSLEEP
	_nanosleep(STROBE_DELAY);
#endif

	_hw_clr(STROBE);
}

static void
flip_to_0(void)
{
	_hw_clr(OE1);

	_nanosleep(OE_DELAY);

	_hw_set(OE0);

	_nanosleep(FLIP_DELAY);

	_hw_clr(OE0);
}

static void
flip_to_1(void)
{
	_hw_clr(OE0);

	_nanosleep(OE_DELAY);

	_hw_set(OE1);

	_nanosleep(FLIP_DELAY);

	_hw_clr(OE1);
}

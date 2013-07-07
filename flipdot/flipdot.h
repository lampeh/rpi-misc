#ifndef FLIPDOT_H
#define FLIPDOT_H

#include <stdint.h>


// BCM2835 GPIO pin mapping
#define STROBE 7

#define DATA_COL 11
#define DATA_ROW 8

#define CLK_COL 9
#define CLK_ROW 25

#define OE1 10
#define OE0 24


// shift register set-up time (ns)
#define DATA_DELAY 15
// shift register pulse width (ns)
#define CLK_DELAY 25
#define STROBE_DELAY 25

// flip motor pulse width (ns)
#define FLIP_DELAY 500*1000


// display geometry
#define MODULE_COUNT_H 1
#define MODULE_COUNT_V 1

#define MODULE_COLS 20
#define MODULE_ROWS 16

#define MODULE_PIXEL_COUNT (MODULE_COLS * MODULE_ROWS)
#define MODULE_BYTE_COUNT ((MODULE_PIXEL_COUNT + 7) / 8)

#define COL_GAP (8 - (MODULE_COLS % 8))

#define REGISTER_COLS (MODULE_COUNT_H * (MODULE_COLS + COL_GAP))
#define REGISTER_ROWS (MODULE_COUNT_V * MODULE_ROWS)

#define DISP_COLS (MODULE_COUNT_H * MODULE_COLS)
#define DISP_ROWS (MODULE_COUNT_V * MODULE_ROWS)

#define DISP_PIXEL_COUNT (DISP_COLS * DISP_ROWS)
#define DISP_BYTE_COUNT ((DISP_PIXEL_COUNT + 7) / 8)


typedef uint8_t flipdot_frame_t[DISP_BYTE_COUNT];

enum sreg {
	ROW,
	COL
};


void flipdot_init(void);
void flipdot_clear_to_0(void);
void flipdot_clear_to_1(void);
void flipdot_display_frame(const flipdot_frame_t *frame);
void flipdot_display_diff(const flipdot_frame_t *diff_to_0, const flipdot_frame_t *diff_to_1);

extern inline void
flipdot_clear(void)
{
    flipdot_clear_to_0();
}

#endif /* FLIPDOT_H */

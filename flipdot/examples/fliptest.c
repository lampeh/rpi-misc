#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>
#include <sys/param.h>
#include <time.h>
#include <bcm2835.h>
#include "flipdot.h"

#define BMP_SETBIT(b,x,y) b[((y*DISP_COLS)+x)>>3]|=(1<<(((y*DISP_COLS)+x)&7));
#define BMP_CLEARBIT(b,x,y) b[((y*DISP_COLS)+x)>>3]&=(1<<(((y*DISP_COLS)+x)&7))^0xFF;

uint8_t bmp[FRAME_BYTE_COUNT];

int main(void) {
	if (!bcm2835_init())
		return 1;

#if 1
	puts("flipdot_init()");
	flipdot_init();
//	sleep(5);
#endif

#if 1
	puts("flipdot_clear_to_0()");
	flipdot_clear_to_0();
	sleep(3);
#endif

#if 1
	puts("flipdot_clear_to_1()");
	flipdot_clear_to_1();
	sleep(3);
#endif

#if 1
	puts("flipdot_clear_to_0()");
	flipdot_clear_to_0();
	sleep(3);
#endif

#if 1
	puts("diagonal flip");
	memset(bmp, 0x00, sizeof(bmp));
	printf("%d x %d\n", DISP_COLS, DISP_ROWS);

	for (int i = 0; i < DISP_COLS + DISP_ROWS - 1; i++) {
		for (int j = MIN(i, DISP_COLS-1); j >= 0 && (i-j) < DISP_ROWS; j--) {
			BMP_SETBIT(bmp, j, (i-j));
		}
		flipdot_update_bitmap(bmp);
	}

	for (int i = 0; i < DISP_COLS + DISP_ROWS - 1; i++) {
		for (int j = MIN(i, DISP_COLS-1); j >= 0 && (i-j) < DISP_ROWS; j--) {
			BMP_CLEARBIT(bmp, j, (i-j));
		}
		flipdot_update_bitmap(bmp);
	}

	sleep(3);
#endif

#if 1
	puts("random flip");
	memset(bmp, 0x00, sizeof(bmp));
	flipdot_clear_to_0();
	printf("%d x %d\n", DISP_COLS, DISP_ROWS);
	srandom(time(NULL));

	for (int i = 0; i < (MODULE_COUNT_H * MODULE_COUNT_V * 3000); i++) {
		BMP_SETBIT(bmp, random() % DISP_COLS, random() % DISP_ROWS);
		BMP_CLEARBIT(bmp, random() % DISP_COLS, random() % DISP_ROWS);
		flipdot_update_bitmap(bmp);
//		sleep(1);
	}

	sleep(3);
#endif

#if 1
	puts("flipdot_shutdown()");
	flipdot_shutdown();
#endif

	return(0);
}

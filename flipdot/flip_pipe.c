#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <bcm2835.h>
#include "flipdot.h"

#define BMP_SETBIT(b,x,y) b[((y*DISP_COLS)+x)>>3]|=(1<<(((y*DISP_COLS)+x)&7));
#define BMP_CLEARBIT(b,x,y) b[((y*DISP_COLS)+x)>>3]&=(1<<(((y*DISP_COLS)+x)&7))^0xFF;

flipdot_frame_t bmp;
unsigned int x, y;

int main(void) {
	int c;

	if (!bcm2835_init())
		return 1;

	flipdot_init();
	memset(bmp, 0x00, sizeof(bmp));
	x = 0;
	y = 0;
	
	while ((c = getc(stdin)) != EOF) {
		if (c == '\n') {
			if ((c = getc(stdin)) == '\n') {
				flipdot_display_frame(&bmp);
				memset(bmp, 0x00, sizeof(bmp));
				x = 0;
				y = 0;
			} else if (c != EOF) {
				ungetc(c, stdin);
				x = 0;
				y++;
			}
			continue;
		}

		if (x < DISP_COLS && y < DISP_ROWS) {
			// Alles ist Eins - ausser der Null (und Space)
			if (c == '0' || c == ' ') {	
				BMP_CLEARBIT(bmp, x, y);
			} else {
				BMP_SETBIT(bmp, x, y);
			}
			x++;
		}
	}

//	flipdot_display_frame(&bmp);
	return(0);
}

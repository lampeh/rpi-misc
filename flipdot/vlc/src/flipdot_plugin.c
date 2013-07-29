/*****************************************************************************
 * Preamble
 *****************************************************************************/
#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_vout_display.h>
#include <vlc_picture_pool.h>

#include <bcm2835.h>
#include "flipdot.h"

#ifndef N_
#define N_(str) (str)
#endif


/*****************************************************************************
 * Module descriptor
 *****************************************************************************/
#define FD_WIDTH_TEXT N_("Horizontal modules")
#define FD_WIDTH_LONGTEXT N_("Number of modules per row")

#define FD_HEIGHT_TEXT N_("Vertical modules")
#define FD_HEIGHT_LONGTEXT N_("Number of modules per column")

#define FD_THRESH_TEXT N_("Brightness threshold")
#define FD_THRESH_LONGTEXT N_("")

static int  Open (vlc_object_t *);
static void Close(vlc_object_t *);

vlc_module_begin()
	set_shortname("Flipdot")
	set_category(CAT_VIDEO)
	set_subcategory(SUBCAT_VIDEO_VOUT)
	set_description(N_("Flip Dot Matrix video output"))
//	add_integer("width", 1, FD_WIDTH_TEXT, FD_WIDTH_LONGTEXT, false)
//	add_integer("height", 1, FD_HEIGHT_TEXT, FD_HEIGHT_LONGTEXT, false)
	add_integer_with_range("threshold", 129, 0, 255, FD_THRESH_TEXT, FD_THRESH_LONGTEXT, false)
	set_capability("vout display", 0)
	set_callbacks(Open, Close)
vlc_module_end()


/*****************************************************************************
 * Local prototypes
 *****************************************************************************/
static picture_pool_t *Pool  (vout_display_t *, unsigned);
static void           Prepare(vout_display_t *, picture_t *, subpicture_t *);
static void           Display(vout_display_t *, picture_t *, subpicture_t *);
static int            Control(vout_display_t *, int, va_list);

/* */
static void Refresh(vout_display_t *);
static void Place(vout_display_t *, vout_display_place_t *);


struct vout_display_sys_t {
	uint8_t *bitmap;
	picture_pool_t *pool;
};


/**
 * This function initializes flipdot vout method.
 */
static int Open(vlc_object_t *object)
{
	vout_display_t *vd = (vout_display_t *)object;
	vout_display_sys_t *sys;

    
	if (!bcm2835_init()) {
		goto error;
	}

	/* Allocate structure */
	vd->sys = sys = calloc(1, sizeof(*sys));
	if (!sys)
		goto error;

	sys->bitmap = calloc(1, DISP_BYTE_COUNT);
	if (!sys->bitmap) {
		msg_Err(vd, "cannot allocate flipdot bitmap");
		goto error;
	}

	flipdot_init();
	flipdot_clear_to_1();

	vout_display_DeleteWindow(vd, NULL);

	/* Fix format */
	video_format_t fmt = vd->fmt;

	fmt.i_chroma = VLC_CODEC_GREY;
	fmt.i_width  = DISP_COLS;
	fmt.i_height = DISP_ROWS;

	/* TODO */
	vout_display_info_t info = vd->info;

	/* Setup vout_display now that everything is fine */
	vd->fmt = fmt;
	vd->info = info;

	vd->pool    = Pool;
	vd->prepare = Prepare;
	vd->display = Display;
	vd->control = Control;
	vd->manage  = NULL;

	/* Fix initial state */
    vout_display_SendEventFullscreen(vd, false);
    vout_display_SendEventDisplaySize(vd, fmt.i_width, fmt.i_height, false);
//	Refresh(vd);

	return VLC_SUCCESS;

error:
	if (sys) {
		if (sys->pool)
			picture_pool_Delete(sys->pool);

		if (sys->bitmap)
			free(sys->bitmap);

		free(sys);
	}

	return VLC_EGENERIC;
}

/**
 * Close a flipdot video output
 */
static void Close(vlc_object_t *object)
{
	vout_display_t *vd = (vout_display_t *)object;
	vout_display_sys_t *sys = vd->sys;

	flipdot_shutdown();

	if (sys->pool)
		picture_pool_Delete(sys->pool);

		if (sys->bitmap)
			free(sys->bitmap);

	free(sys);
}

/**
 * Return a pool of direct buffers
 */
/*
static picture_pool_t *Pool(vout_display_t *vd, unsigned count)
{
	vout_display_sys_t *sys = vd->sys;

	if (sys->pool)
		return sys->pool;

	if (!sys->pool) {
		picture_resource_t rsc;

		memset(&rsc, 0, sizeof(rsc));

		rsc.p[0].p_pixels = calloc(1, DISP_BYTE_COUNT);
		if (!rsc.p[0].p_pixels) {
			msg_Err(vd, "cannot allocate flipdot picture");
		}
		rsc.p[0].i_pitch = DISP_COLS;
		rsc.p[0].i_lines = DISP_ROWS;

		picture_t *p_picture = picture_NewFromResource(&vd->fmt, &rsc);
		if (!p_picture)
			return NULL;

		sys->pool = picture_pool_New(1, &p_picture);
	}
	return sys->pool;
}
*/

/**
 * Return a pool of direct buffers
 */
static picture_pool_t *Pool(vout_display_t *vd, unsigned count)
{   
	vout_display_sys_t *sys = vd->sys;

	if (!sys->pool)
		sys->pool = picture_pool_NewFromFormat(&vd->fmt, count);

	return sys->pool;
}

#define SETBIT(b,i) (((b)[(i) >> 3]) |= (1 << ((i) & 7)))
#define CLEARBIT(b,i) (((b)[(i) >> 3]) &=~ (1 << ((i) & 7)))

// TODO: use config option
#define THRESHOLD 129

/**
 * Prepare a picture for display
 */
static void Prepare(vout_display_t *vd, picture_t *picture, subpicture_t *subpicture)
{
	vout_display_sys_t *sys = vd->sys;

	memset(sys->bitmap, 0x00, DISP_BYTE_COUNT);

	// TODO: dithering

	// another slow bit copy
	for (unsigned long y = vd->source.i_y_offset; y < (vd->source.i_y_offset + picture->p->i_visible_lines); y++) {
		for (unsigned long x = vd->source.i_x_offset; x < (vd->source.i_y_offset + picture->p->i_visible_pitch); x++) {
			unsigned long pixelidx = (y * picture->p->i_pitch) + (x * picture->p->i_pixel_pitch);

			uint8_t pixel_value = *(picture->p->p_pixels + pixelidx);
			uint8_t pixel = 0;
			if (pixel_value < THRESHOLD && y < DISP_ROWS && x < DISP_COLS) {
				SETBIT(sys->bitmap, (y * DISP_COLS) + x);
				pixel = 1;
			}
//printf("x = %ld, y = %ld, pixelidx = %ld, pixel_value = %d, pixel = %d\n", x, y, pixelidx, pixel_value, pixel);
		}
	}
	VLC_UNUSED(subpicture);
}

/**
 * Display a picture
 */
static void Display(vout_display_t *vd, picture_t *picture, subpicture_t *subpicture)
{   
//	assert(!picture_IsReferenced(picture));

	flipdot_update_bitmap(vd->sys->bitmap);

	if (vd->cfg->display.width != DISP_COLS ||
		vd->cfg->display.height != DISP_ROWS)
			vout_display_SendEventDisplaySize(vd, DISP_COLS, DISP_ROWS, false);

	picture_Release(picture);
	VLC_UNUSED(subpicture);
}

/**
 * Control for vout display
 */
static int Control(vout_display_t *vd, int query, va_list args)
{
	switch (query) {
		case VOUT_DISPLAY_CHANGE_FULLSCREEN:
		case VOUT_DISPLAY_CHANGE_DISPLAY_SIZE: {
			const vout_display_cfg_t *cfg = va_arg(args, const vout_display_cfg_t *);

			if (cfg->display.width != vd->fmt.i_width ||
				cfg->display.height != vd->fmt.i_height)
					return VLC_EGENERIC;

			if (cfg->is_fullscreen)
				return VLC_EGENERIC;

			return VLC_SUCCESS;
		}

	default:
		return VLC_EGENERIC;
	}
}

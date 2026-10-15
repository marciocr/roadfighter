#ifndef __ROADFIGHTER_POSTFX_H
#define __ROADFIGHTER_POSTFX_H

/* Upscaling filters (OPTIONS -> VIDEO -> FILTER), shared with the F-1 Spirit
   and Maze of Galious remakes. The filter shader(s) in shaders/ take the
   texture holding the game's frame (see present_screen() in main.cpp) and
   scale it: 2x and then a bilinear stretch, or straight to the window size
   for the resolution-independent ones. With FILTER_OFF main.cpp draws the
   frame itself, with plain pixel scaling. */

enum {
	FILTER_OFF=0,
	FILTER_SCALE2X,
	FILTER_2XSAI,
	FILTER_HQ2X,
	FILTER_XBR,
	FILTER_OMNISCALE,
	FILTER_LANCZOS,
	N_FILTERS
};

extern int video_filter;	/* saved in RoadFighter.cfg */

const char *postfx_filter_name(int filter);

/* Selects a filter, loading its shaders on first use. Falls back to
   FILTER_OFF (and returns false) if the GPU can't run it. */
bool postfx_set_filter(int filter);
bool postfx_active(void);

/* Draws texture tex (tex_w x tex_h) filtered into the window rectangle
   x,y,w,h (window pixels, GL coordinates) of the default framebuffer. */
void postfx_draw(unsigned int tex,int tex_w,int tex_h,int x,int y,int w,int h);

void postfx_release(void);

#endif

/*******************************************************************************#
#           guvcview              http://guvcview.sourceforge.net               #
#                                                                               #
#           Paulo Assis <pj.assis@gmail.com>                                    #
#           Nobuhiro Iwamatsu <iwamatsu@nigauri.org>                            #
#                             Add UYVY color support(Macbook iSight)            #
#           Flemming Frandsen <dren.dk@gmail.com>                               #
#                             Add VU meter OSD                                  #
#                                                                               #
# This program is free software; you can redistribute it and/or modify          #
# it under the terms of the GNU General Public License as published by          #
# the Free Software Foundation; either version 2 of the License, or             #
# (at your option) any later version.                                           #
#                                                                               #
# This program is distributed in the hope that it will be useful,               #
# but WITHOUT ANY WARRANTY; without even the implied warranty of                #
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                 #
# GNU General Public License for more details.                                  #
#                                                                               #
# You should have received a copy of the GNU General Public License             #
# along with this program; if not, write to the Free Software                   #
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA     #
#                                                                               #
********************************************************************************/

#include <SDL.h>
#include <assert.h>
#include <math.h>

#include "gview.h"
#include "gviewrender.h"
#include "render.h"
#include "render_sdl2.h"
#include "../config.h"
#include "ch9329.h"
#include <glib.h>

extern int render_verbosity;

SDL_DisplayMode display_mode;

static SDL_Window*  sdl_window = NULL;
static SDL_Texture* rending_texture = NULL;
static SDL_Renderer*  main_renderer = NULL;

static int serial_fd = -1;

int win_width, win_height;

GHashTable *key_mod_map = NULL;
/*
 * initialize sdl video
 * args:
 *   width - video width
 *   height - video height
 *   flags - window flags:
 *              0- none
 *              1- fullscreen
 *              2- maximized
 *   win_w - window width (0 use video width)
 *   win_h - window height (0 use video height)
 *
 * asserts:
 *   none
 *
 * returns: error code
 */
static int video_init(int width, int height, int flags, int win_w, int win_h)
{
	int w = width;
	int h = height;
	int32_t my_flags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;

	switch(flags)
	{
		case 2:
		  my_flags |= SDL_WINDOW_MAXIMIZED;
		  break;
		case 1:
		  my_flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
		  break;
		case 0:
		default:
		  break;
	}

	if(render_verbosity > 0)
		printf("RENDER: Initializing SDL2 render\n");

    if (sdl_window == NULL) /*init SDL*/
    {
        if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER) < 0)
        {
            fprintf(stderr, "RENDER: Couldn't initialize SDL2: %s\n", SDL_GetError());
            return -1;
        }

        SDL_SetHint("SDL_HINT_RENDER_SCALE_QUALITY", "1");

		sdl_window = SDL_CreateWindow(
			"Guvcview Video",                  // window title
			SDL_WINDOWPOS_UNDEFINED,           // initial x position
			SDL_WINDOWPOS_UNDEFINED,           // initial y position
			w,                               // width, in pixels
			h,                               // height, in pixels
			my_flags
		);

		if(sdl_window == NULL)
		{
			fprintf(stderr, "RENDER: (SDL2) Couldn't open window: %s\n", SDL_GetError());
			render_sdl2_clean();
            return -2;
		}

		int display_index = SDL_GetWindowDisplayIndex(sdl_window);

		int err = SDL_GetDesktopDisplayMode(display_index, &display_mode);
		if(!err)
		{
			if(render_verbosity > 0)
				printf("RENDER: video display %i ->  %dx%dpx @ %dhz\n",
					display_index,
					display_mode.w,
					display_mode.h,
					display_mode.refresh_rate);
		}
		else
			fprintf(stderr, "RENDER: Couldn't determine display mode for video display %i\n", display_index);

		if(win_w > 0)
			w = win_w;
		if(win_h > 0)
			h = win_h;

		if(w > display_mode.w)
			w = display_mode.w;
		if(h > display_mode.h)
			h = display_mode.h;

		if(render_verbosity > 0)
			printf("RENDER: setting window size to %ix%i\n", w, h);

		SDL_SetWindowSize(sdl_window, w, h);
    }

    if(render_verbosity > 2)
    {
		/* Allocate a renderer info struct*/
        SDL_RendererInfo *rend_info = (SDL_RendererInfo *) malloc(sizeof(SDL_RendererInfo));
        if (!rend_info)
        {
                fprintf(stderr, "RENDER: Couldn't allocate memory for the renderer info data structure\n");
                render_sdl2_clean();
                return -5;
        }
        /* Print the list of the available renderers*/
        printf("\nRENDER: Available SDL2 rendering drivers:\n");
        int i = 0;
        for (i = 0; i < SDL_GetNumRenderDrivers(); i++)
        {
            if (SDL_GetRenderDriverInfo(i, rend_info) < 0)
            {
                fprintf(stderr, " Couldn't get SDL2 render driver information: %s\n", SDL_GetError());
            }
            else
            {
                printf(" %2d: %s\n", i, rend_info->name);
                printf("    SDL_RENDERER_TARGETTEXTURE [%c]\n", (rend_info->flags & SDL_RENDERER_TARGETTEXTURE) ? 'X' : ' ');
                printf("    SDL_RENDERER_SOFTWARE      [%c]\n", (rend_info->flags & SDL_RENDERER_SOFTWARE) ? 'X' : ' ');
                printf("    SDL_RENDERER_ACCELERATED   [%c]\n", (rend_info->flags & SDL_RENDERER_ACCELERATED) ? 'X' : ' ');
                printf("    SDL_RENDERER_PRESENTVSYNC  [%c]\n", (rend_info->flags & SDL_RENDERER_PRESENTVSYNC) ? 'X' : ' ');
            }
        }

        free(rend_info);
	}

    main_renderer = SDL_CreateRenderer(sdl_window, -1,
		SDL_RENDERER_TARGETTEXTURE |
		SDL_RENDERER_PRESENTVSYNC  |
		SDL_RENDERER_ACCELERATED);

	if(main_renderer == NULL)
	{
		fprintf(stderr, "RENDER: (SDL2) Couldn't get a accelerated renderer: %s\n", SDL_GetError());
		fprintf(stderr, "RENDER: (SDL2) trying with a software renderer\n");

		main_renderer = SDL_CreateRenderer(sdl_window, -1,
		SDL_RENDERER_TARGETTEXTURE |
		SDL_RENDERER_SOFTWARE);


		if(main_renderer == NULL)
		{
			fprintf(stderr, "RENDER: (SDL2) Couldn't get a software renderer: %s\n", SDL_GetError());
			fprintf(stderr, "RENDER: (SDL2) giving up...\n");
			render_sdl2_clean();
			return -3;
		}
	}


	if(render_verbosity > 2)
    {
		/* Allocate a renderer info struct*/
        SDL_RendererInfo *rend_info = (SDL_RendererInfo *) malloc(sizeof(SDL_RendererInfo));
        if (!rend_info)
        {
                fprintf(stderr, "RENDER: Couldn't allocate memory for the renderer info data structure\n");
                render_sdl2_clean();
                return -5;
        }

		/* Print the name of the current rendering driver */
		if (SDL_GetRendererInfo(main_renderer, rend_info) < 0)
		{
			fprintf(stderr, "Couldn't get SDL2 rendering driver information: %s\n", SDL_GetError());
		}
		printf("RENDER: rendering driver in use: %s\n", rend_info->name);
		printf("    SDL_RENDERER_TARGETTEXTURE [%c]\n", (rend_info->flags & SDL_RENDERER_TARGETTEXTURE) ? 'X' : ' ');
		printf("    SDL_RENDERER_SOFTWARE      [%c]\n", (rend_info->flags & SDL_RENDERER_SOFTWARE) ? 'X' : ' ');
		printf("    SDL_RENDERER_ACCELERATED   [%c]\n", (rend_info->flags & SDL_RENDERER_ACCELERATED) ? 'X' : ' ');
		printf("    SDL_RENDERER_PRESENTVSYNC  [%c]\n", (rend_info->flags & SDL_RENDERER_PRESENTVSYNC) ? 'X' : ' ');

		free(rend_info);
	}

	SDL_RenderSetLogicalSize(main_renderer, width, height);
    win_width = width;
    win_height = height;
	SDL_SetRenderDrawBlendMode(main_renderer, SDL_BLENDMODE_NONE);

    rending_texture = SDL_CreateTexture(main_renderer,
		SDL_PIXELFORMAT_IYUV,  /*yuv420p*/
		SDL_TEXTUREACCESS_STREAMING,
		width,
		height);

	if(rending_texture == NULL)
	{
		fprintf(stderr, "RENDER: (SDL2) Couldn't get a texture for rending: %s\n", SDL_GetError());
		render_sdl2_clean();
		return -4;
	}

    return 0;
}

/*
 * init sdl2 render
 * args:
 *    width - overlay width
 *    height - overlay height
 *    flags - window flags:
 *              0- none
 *              1- fullscreen
 *              2- maximized
 *   win_w - window width (0 use render width)
 *   win_h - window height (0 use render height)
 *
 * asserts:
 *
 * returns: error code (0 ok)
 */
 int init_render_sdl2(int width, int height, int flags, int win_w, int win_h)
 {
	int err = video_init(width, height, flags, win_w, win_h);

	if(err)
	{
		fprintf(stderr, "RENDER: Couldn't init the SDL2 rendering engine\n");
		return -1;
	}

	assert(rending_texture != NULL);

    if (serial_fd == -1) {
        serial_fd = ch9329_init();
        if (serial_fd < 0) {
            fprintf(stderr, "RENDER: can not initialize ch9329 serial\n");
            return -1;
        }
    }

    if (key_mod_map == NULL) {
        key_mod_map = g_hash_table_new(NULL, NULL);
        g_hash_table_insert(key_mod_map, (gpointer)KMOD_LSHIFT, (gpointer)LEFT_SHIFT);
        g_hash_table_insert(key_mod_map, (gpointer)KMOD_RSHIFT, (gpointer)RIGHT_SHIFT);
        g_hash_table_insert(key_mod_map, (gpointer)KMOD_LCTRL, (gpointer)LEFT_CTRL);
        g_hash_table_insert(key_mod_map, (gpointer)KMOD_RCTRL, (gpointer)RIGHT_CTRL);
        g_hash_table_insert(key_mod_map, (gpointer)KMOD_LALT, (gpointer)LEFT_ALT);
        g_hash_table_insert(key_mod_map, (gpointer)KMOD_RALT, (gpointer)RIGHT_ALT);
        g_hash_table_insert(key_mod_map, (gpointer)KMOD_LGUI, (gpointer)LEFT_WIN);
        g_hash_table_insert(key_mod_map, (gpointer)KMOD_RGUI, (gpointer)RIGHT_WIN);

        /* left ctrl + left alt */
        g_hash_table_insert(key_mod_map, (gpointer)(KMOD_LCTRL | KMOD_LALT),
            (gpointer)(LEFT_CTRL | LEFT_ALT));
        /* left ctrl + left shift */
        g_hash_table_insert(key_mod_map, (gpointer)(KMOD_LCTRL | KMOD_LSHIFT),
            (gpointer)(LEFT_CTRL | LEFT_SHIFT));
        /* right ctrl + right alt */
        g_hash_table_insert(key_mod_map, (gpointer)(KMOD_RCTRL | KMOD_RALT),
            (gpointer)(RIGHT_CTRL | RIGHT_ALT));
        /* right ctrl + right shift */
        g_hash_table_insert(key_mod_map, (gpointer)(KMOD_RCTRL | KMOD_RSHIFT),
            (gpointer)(RIGHT_CTRL | RIGHT_SHIFT));
    }

    /* hide window cursor */
    SDL_ShowCursor(SDL_DISABLE);

	return 0;
 }

/*
 * render a frame
 * args:
 *   frame - pointer to frame data (yuyv format)
 *   width - frame width
 *   height - frame height
 *
 * asserts:
 *   poverlay is not nul
 *   frame is not null
 *
 * returns: error code
 */
int render_sdl2_frame(uint8_t *frame, int width, int height)
{
	/*asserts*/
	assert(rending_texture != NULL);
	assert(frame != NULL);

	SDL_SetRenderDrawColor(main_renderer, 0, 0, 0, 255); /*black*/
	SDL_RenderClear(main_renderer);

	/* since data is continuous we can use SDL_UpdateTexture
	 * instead of SDL_UpdateYUVTexture.
	 * no need to use SDL_Lock/UnlockTexture (it doesn't seem faster)
	 */
	SDL_UpdateTexture(rending_texture, NULL, frame, width);

	SDL_RenderCopy(main_renderer, rending_texture, NULL, NULL);

	SDL_RenderPresent(main_renderer);

	return 0;
}

/*
 * set sdl2 render caption
 * args:
 *   caption - string with render window caption
 *
 * asserts:
 *   none
 *
 * returns: none
 */
void set_render_sdl2_caption(const char* caption)
{
	SDL_SetWindowTitle(sdl_window, caption);
}

/*
 * dispatch sdl2 render events
 * args:
 *   none
 *
 * asserts:
 *   none
 *
 * returns: none
 */
void render_sdl2_dispatch_events()
{

	SDL_Event event;

	while( SDL_PollEvent(&event) )
	{
		if (event.type == SDL_KEYDOWN)
		{
            const char *key = SDL_GetKeyName(event.key.keysym.sym);
            //printf("key: |%s| pressed, mod: %d\n", key, event.key.keysym.mod);
            if (event.key.keysym.mod ==  KMOD_NONE) {
                send_key_down(key, serial_fd);
            } else {
                gpointer mod = g_hash_table_lookup(key_mod_map, (gconstpointer)event.key.keysym.mod);

                if (mod) {
                    send_key_mod(key, (enum CTRL_KEY)mod, 1, serial_fd);
                } else {
                    fprintf(stderr, "keymod [%d] not supported, ignore!\n",
                        event.key.keysym.mod);
                    send_key_down(key, serial_fd);
                }
            }
		}

        if (event.type == SDL_KEYUP)
        {
            const char *key = SDL_GetKeyName(event.key.keysym.sym);
            //printf("key: %s released\n", key);
            send_key_up(key, serial_fd);
        }

        if (event.type == SDL_MOUSEMOTION)
        {
            /*
            printf("mouse move: [%d, %d / %d, %d]\n", event.motion.x, event.motion.y,
                   event.motion.xrel, event.motion.yrel);
            */
            send_mouse_move(serial_fd, event.motion.xrel, event.motion.yrel);
        }

        if (event.type == SDL_MOUSEBUTTONDOWN)
        {
            //printf("mouse %d down: [%d, %d]\n", event.button.button, event.button.x, event.button.y);
            if (event.button.button == SDL_BUTTON_LEFT) {
                send_mouse_click_down(serial_fd, event.button.x, event.button.y, LEFT_BUTTON);
            } else if (event.button.button == SDL_BUTTON_RIGHT) {
                send_mouse_click_down(serial_fd, event.button.x, event.button.y, RIGHT_BUTTON);
            } else if (event.button.button == SDL_BUTTON_MIDDLE) {
                send_mouse_click_down(serial_fd, event.button.x, event.button.y, MID_BUTTON);
            } else {
                fprintf(stderr, "mouse button [%d] click not supported\n", event.button.button);
            }
        }

        if (event.type == SDL_MOUSEBUTTONUP)
        {
            //printf("mouse %d up: [%d, %d]\n", event.button.button, event.button.x, event.button.y);
            send_mouse_click_up(serial_fd, event.button.x, event.button.y);
        }

        if (event.type == SDL_MOUSEWHEEL)
        {
            //printf("mouse wheel: [%d, %d]\n", event.wheel.x, event.wheel.y);
            send_mouse_wheel(serial_fd, event.wheel.x, event.wheel.y);
        }

		if(event.type==SDL_QUIT)
		{
			if(render_verbosity > 0)
				printf("RENDER: (event) quit\n");
			render_call_event_callback(EV_QUIT);
		}
	}
}
/*
 * clean sdl2 render data
 * args:
 *   none
 *
 * asserts:
 *   none
 *
 * returns: none
 */
void render_sdl2_clean()
{
	if(rending_texture)
		SDL_DestroyTexture(rending_texture);

	rending_texture = NULL;

	if(main_renderer)
		SDL_DestroyRenderer(main_renderer);

	main_renderer = NULL;

	if(sdl_window)
		SDL_DestroyWindow(sdl_window);

	sdl_window = NULL;

	SDL_Quit();
}

void get_render_sdl2_mouse_pos(int *x, int *y)
{
    SDL_GetMouseState(x, y);
}

/*  Copyright 2003-2009 Santi Ontanon <santi.ontanon@terra.es>
    Work continued by 2017 Carlos Donizete Froes [a.k.a coringao]

    This file is part of Road Fighter Remake.

    Road Fighter Remake is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Road Fighter Remake is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Road Fighter Remake; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#ifdef _WIN32
#include <windows.h>
#include <windowsx.h>
#else
#include <sys/time.h>
#include <time.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_mixer/SDL_mixer.h>

#include "sound.h"

#include "CTile.h"
#include "CObject.h"
#include "CGame.h"
#include "CRoadFighter.h"
#include "auxiliar.h"

#include "debug.h"


/*						GLOBAL VARIABLES INITIALIZATION:						*/

const int REDRAWING_PERIOD=27;	/* This is for 35fps */
// const int REDRAWING_PERIOD=40;	/* This is for 25fps */

int SCREEN_X=512;
int SCREEN_Y=384;
const int COLOUR_DEPTH=32;
const char *application_name="Road Fighter Remake";

#ifdef _WIN32
bool fullscreen=true;
#else
bool fullscreen=false;
#endif

int init_time=0;

/* SDL3: nao existe mais uma unica "superficie de video" global obtida
   implicitamente. Agora a janela e a superficie sao objetos separados
   e explicitos: SDL_Window* para a janela, e SDL_Surface* obtida a
   partir dela via SDL_GetWindowSurface(). Mantemos screen_sfc (usado
   em outros arquivos, ex: CRoadFighter.cpp) e adicionamos main_window. */
SDL_Window *main_window=0;
SDL_Surface *screen_sfc;
CRoadFighter *game=0;

/* SDL3: as antigas flags SDL_HWSURFACE/SDL_DOUBLEBUF eram sobre a
   superficie de software do SDL1.2/2 e nao existem mais. O que resta
   configuravel na criacao da janela agora e coisa como
   SDL_WINDOW_FULLSCREEN, SDL_WINDOW_RESIZABLE, etc. */
Uint64 screen_flags = 0;

int start_level=1;


/*						AUXILIAR FUNCTION DEFINITION:							*/

SDL_Surface* initializeSDL(Uint64 moreflags)
{
	Uint64 flags = screen_flags|moreflags;

	/* SDL3: SDL_Init retorna bool (true=sucesso), nao mais 0/-1. */
	if (!SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO)) return 0;

	output_debug_message("Initializing SDL video subsystem.\n");
	if (!SDL_InitSubSystem(SDL_INIT_VIDEO))
	{
	  output_debug_message("Couldn't initialize video subsystem: %s\n", SDL_GetError());
	  exit(-1);
	}
	/* SDL3: SDL_VideoDriverName foi removido; o equivalente atual e
	   SDL_GetCurrentVideoDriver(), que ja devolve a string (sem precisar
	   de buffer proprio). */
	output_debug_message("SDL driver used: %s\n", SDL_GetCurrentVideoDriver());
	output_debug_message("SDL video subsystem initialized.\n");

	atexit(SDL_Quit);

	Sound_initialization();

	SDL_Delay(1000);

	/* SDL3: SDL_SetVideoMode (que criava E retornava a superficie de tela
	   em um so passo) foi substituido por dois passos explicitos:
	   1) criar a janela com SDL_CreateWindow
	   2) pegar a superficie de desenho da janela com SDL_GetWindowSurface
	   COLOUR_DEPTH tambem deixou de existir como parametro: o formato de
	   pixel da janela agora e escolhido pelo sistema/driver. */
	main_window = SDL_CreateWindow(application_name, SCREEN_X, SCREEN_Y, flags);
	if (main_window == 0) {
		output_debug_message("Couldn't create window: %s\n", SDL_GetError());
		exit(-1);
	}
	if (fullscreen) SDL_HideCursor();

	SDL_Surface *screen = SDL_GetWindowSurface(main_window);

	if (screen == NULL) {
		output_debug_message("Couldn't get window surface (%ix%i)", SCREEN_X, SCREEN_Y);
	    if (fullscreen) output_debug_message(",fullscreen,");
	    output_debug_message(" video mode: %s\n",SDL_GetError ());
	    exit(-1);
	} else {
	    output_debug_message("Set the video resolution to: %ix%ix%i",
			 screen->w, screen->h,
			 SDL_BITSPERPIXEL(screen->format));
	    if (fullscreen) output_debug_message(",fullscreen");
	    output_debug_message("\n");
    } /* if */

	TTF_Init();

	/* SDL3: SDL_EnableUNICODE nao existe mais. A entrada de texto Unicode
	   agora e sempre entregue via SDL_EVENT_TEXT_INPUT quando
	   SDL_StartTextInput(window) e chamado; nao e mais uma flag global. */

	return screen;
} /* initializeSDL */


void finalizeSDL()
{
	TTF_Quit();
//	Sound_release();
	SDL_Quit();
} /* finalizeSDL */



#ifdef _WIN32
int PASCAL WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance,
                    LPSTR lpCmdLine, int nCmdShow)
{
	{
		int tmp;
		if (1==sscanf(lpCmdLine,"%i",&tmp)) {
			start_level=tmp;
			if (start_level<1 || start_level>6) start_level=1;
		} /* if */
	}

#else
int main(int argc, char** argv)
{
	setupTickCount();

	{
		int tmp;
		if (argc==2 &&
			1==sscanf(argv[1],"%i",&tmp)) {
			start_level=tmp;
			if (start_level<1 || start_level>6) start_level=1;
		} /* if */
	}
#endif

	int time,act_time;
	SDL_Event event;
    bool quit = false;


	time=init_time=GetTickCount();
	screen_sfc = initializeSDL((fullscreen ? SDL_WINDOW_FULLSCREEN : 0));
	if (screen_sfc==0) return 0;

	game=new CRoadFighter();

	while (!quit) {
		while( SDL_PollEvent( &event ) ) {
            switch( event.type ) {
                /* Keyboard event */
                /* SDL3: o tipo de evento agora e SDL_EVENT_KEY_DOWN (era
                   SDL_KEYDOWN), e os campos do teclado ficaram "achatados"
                   direto em event.key: nao existe mais o struct aninhado
                   event.key.keysym. Tambem: event.key.keysym.sym virou
                   event.key.key (keycode virtual) -- usamos aqui porque
                   os atalhos abaixo dependem do simbolo (F12, F4, etc),
                   nao da posicao fisica da tecla. */
                case SDL_EVENT_KEY_DOWN:
					// quit
					if (event.key.key==SDLK_F12) {
						quit = true;
					}
					if (event.key.key==SDLK_F4) {
						/* SDL3: SDLMod virou SDL_Keymod, e KMOD_ALT virou
						   SDL_KMOD_ALT (prefixo SDL_ adicionado em toda a
						   familia de constantes de modificador). */
						SDL_Keymod modifiers;
						modifiers=SDL_GetModState();
						if ((modifiers&SDL_KMOD_ALT)!=0) {
							quit=true;
						}
					}
#ifdef __APPLE__
                    // different quit shortcut on OSX: apple+Q
                    if (event.key.key == SDLK_Q) {
                        SDL_Keymod modifiers;
                        modifiers = SDL_GetModState();
                        if ((modifiers&SDL_KMOD_GUI) != 0) {
                            quit = true;
                        }
                    }
#endif
					// fullscreen
/*
FIXME: the code below is a big copy/paste; it should be in a separate function in stead
*/

#ifdef __APPLE__
					if (event.key.key == SDLK_F) {
						SDL_Keymod modifiers;

						modifiers=SDL_GetModState();

						if ((modifiers&SDL_KMOD_GUI) != 0) {
							Stop_playback();
							/* Toogle FULLSCREEN mode: */
							if (fullscreen) fullscreen=false;
										   else fullscreen=true;
							/* SDL3: em vez de derrubar e reerguer o
							   subsistema de video inteiro (o que tambem
							   destruiria a janela), o jeito idiomatico de
							   alternar fullscreen agora e chamar
							   SDL_SetWindowFullscreen na janela existente. */
							SDL_SetWindowFullscreen(main_window, fullscreen);
							screen_sfc = SDL_GetWindowSurface(main_window);
							if (fullscreen) SDL_HideCursor(); else SDL_ShowCursor();
							Resume_playback();
						} /* if */
					} /* if */
#endif

					if (event.key.key==SDLK_RETURN) {
						SDL_Keymod modifiers;

						modifiers=SDL_GetModState();

						if ((modifiers&SDL_KMOD_ALT)!=0) {
							Stop_playback();
							/* Toogle FULLSCREEN mode: */
							if (fullscreen) fullscreen=false;
										   else fullscreen=true;
							SDL_SetWindowFullscreen(main_window, fullscreen);
							screen_sfc = SDL_GetWindowSurface(main_window);
							if (fullscreen) SDL_HideCursor(); else SDL_ShowCursor();
							Resume_playback();
						} /* if */
					} /* if */
                    break;

                /* SDL_EVENT_QUIT (window close). SDL3 renomeou SDL_QUIT
                   para SDL_EVENT_QUIT, seguindo o novo padrao de nomes
                   SDL_EVENT_* para todo tipo de evento. */
                case SDL_EVENT_QUIT:
                    quit = true;
                    break;
            } /* switch */
        } /* while */

		act_time=GetTickCount();
		if (act_time-time>=REDRAWING_PERIOD) {
			if ((act_time-init_time)>=1000) init_time=act_time;

			time+=REDRAWING_PERIOD;
			if ((act_time-time)>2*REDRAWING_PERIOD) time=act_time;

			if (!game->cycle()) quit=true;
			SDL_SetSurfaceClipRect(screen_sfc, 0);
			game->draw(screen_sfc);
			/* SDL3: SDL_Flip() foi substituido por
			   SDL_UpdateWindowSurface(window), ja que a superficie
			   agora pertence explicitamente a janela, nao a um
			   "video global" implicito. */
			SDL_UpdateWindowSurface(main_window);
		} /* if */
		SDL_Delay(1);
	}

	delete game;

	finalizeSDL();

	return 0;
} /* main */

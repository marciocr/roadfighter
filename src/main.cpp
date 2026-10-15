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
#include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_mixer/SDL_mixer.h>

#include "sound.h"

#include "CTile.h"
#include "CObject.h"
#include "CGame.h"
#include "CRoadFighter.h"
#include "auxiliar.h"
#include "gamepad.h"
#include "postfx.h"

#include "debug.h"


/*						GLOBAL VARIABLES INITIALIZATION:						*/

const int REDRAWING_PERIOD=27;	/* This is for 35fps */
// const int REDRAWING_PERIOD=40;	/* This is for 25fps */

int SCREEN_X=512;
int SCREEN_Y=384;
const int COLOUR_DEPTH=32;
const char *application_name="Road Fighter Remake";
#define GAME_VERSION "1.1.0"

#ifdef _WIN32
bool fullscreen=true;
#else
bool fullscreen=false;
#endif

int init_time=0;

/* SDL3: nao existe mais uma unica "superficie de video" global obtida
   implicitamente. Agora a janela e a superficie sao objetos separados
   e explicitos: SDL_Window* para a janela, e SDL_Surface* onde o jogo
   desenha. Mantemos screen_sfc (usado em outros arquivos, ex:
   CRoadFighter.cpp) e adicionamos main_window. */
SDL_Window *main_window=0;
SDL_Surface *screen_sfc;
/* O jogo desenha sempre numa surface fixa de SCREEN_X x SCREEN_Y
   (screen_sfc); a cada frame ela e copiada pra uma textura OpenGL e
   escalada pro tamanho real da janela (com barras pretas pra manter a
   proporcao), passando pelo filtro 2D escolhido no menu VIDEO (ver
   postfx.cpp, o mesmo do F-1 Spirit e do Maze of Galious). Assim tela
   cheia e janela redimensionada funcionam sem o jogo precisar saber o
   tamanho de verdade da tela. */
static SDL_GLContext gl_context=0;
static GLuint screen_tex=0;
static Uint32 *frame_buffer=0;
CRoadFighter *game=0;

/* SDL3: as antigas flags SDL_HWSURFACE/SDL_DOUBLEBUF eram sobre a
   superficie de software do SDL1.2/2 e nao existem mais. O que resta
   configuravel na criacao da janela agora e coisa como
   SDL_WINDOW_FULLSCREEN, SDL_WINDOW_RESIZABLE, etc. */
Uint64 screen_flags = 0;

int start_level=1;


/*						AUXILIAR FUNCTION DEFINITION:							*/

void set_fullscreen(bool f)
{
	fullscreen=f;
	if (main_window==0) return;
	SDL_SetWindowFullscreen(main_window, fullscreen);
	if (fullscreen) SDL_HideCursor(); else SDL_ShowCursor();
} /* set_fullscreen */


bool get_fullscreen(void)
{
	return fullscreen;
} /* get_fullscreen */


#ifndef _WIN32
/* Os dados do jogo (graphics/, sound/, maps/, fonts/) sao abertos com
   caminhos relativos ao diretorio atual. Se o jogo nao foi iniciado de
   dentro da pasta dos dados (ex: AppImage, ou um atalho do menu), procura
   os dados ao lado do executavel ou em ../share/roadfighter/ (layout de
   instalacao usado pela AppImage) e entra nessa pasta. */
static void chdir_to_game_data(void)
{
	const char *subdirs[]={"","../share/roadfighter/"};
	const char *base;
	char dir[1024],probe[1100];
	unsigned int i;

	if (access("graphics/title.jpg",R_OK)==0) return;
	base=SDL_GetBasePath();
	if (base==0) return;
	for(i=0;i<sizeof(subdirs)/sizeof(subdirs[0]);i++) {
		snprintf(dir,sizeof(dir),"%s%s",base,subdirs[i]);
		snprintf(probe,sizeof(probe),"%sgraphics/title.jpg",dir);
		if (access(probe,R_OK)==0) {
			if (chdir(dir)!=0) output_debug_message("Couldn't enter game data dir %s\n",dir);
			return;
		} /* if */
	} /* for */
} /* chdir_to_game_data */
#endif


static void present_screen(void)
{
	int window_w=SCREEN_X,window_h=SCREEN_Y,w,h,y;

	/* Copia o quadro pra textura, linha de baixo primeiro (a ordem do
	   OpenGL, que os filtros esperam). screen_sfc e XRGB8888, que na
	   memoria fica B,G,R,X: por isso GL_BGRA. */
	for(y=0;y<SCREEN_Y;y++) {
		memcpy(frame_buffer+(SCREEN_Y-1-y)*SCREEN_X,
			   (Uint8 *)screen_sfc->pixels+y*screen_sfc->pitch,SCREEN_X*4);
	} /* for */
	glBindTexture(GL_TEXTURE_2D,screen_tex);
	glPixelStorei(GL_UNPACK_ALIGNMENT,4);
	glTexSubImage2D(GL_TEXTURE_2D,0,0,0,SCREEN_X,SCREEN_Y,GL_BGRA,GL_UNSIGNED_BYTE,frame_buffer);
	glBindTexture(GL_TEXTURE_2D,0);

	SDL_GetWindowSizeInPixels(main_window,&window_w,&window_h);

	/* Onde a imagem vai na janela: 4:3, centralizada, barras pretas em volta. */
	if (window_w*SCREEN_Y>window_h*SCREEN_X) {
		w=window_h*SCREEN_X/SCREEN_Y;
		h=window_h;
	} else {
		w=window_w;
		h=window_w*SCREEN_Y/SCREEN_X;
	} /* if */

	glViewport(0,0,window_w,window_h);
	glClearColor(0,0,0,1);
	glClear(GL_COLOR_BUFFER_BIT);
	if (postfx_active()) {
		postfx_draw(screen_tex,SCREEN_X,SCREEN_Y,(window_w-w)/2,(window_h-h)/2,w,h);
	} else {
		glViewport((window_w-w)/2,(window_h-h)/2,w,h);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D,screen_tex);
		glBegin(GL_QUADS);
		glTexCoord2f(0,0); glVertex2f(-1,-1);
		glTexCoord2f(1,0); glVertex2f(1,-1);
		glTexCoord2f(1,1); glVertex2f(1,1);
		glTexCoord2f(0,1); glVertex2f(-1,1);
		glEnd();
		glBindTexture(GL_TEXTURE_2D,0);
		glDisable(GL_TEXTURE_2D);
	} /* if */

	SDL_GL_SwapWindow(main_window);
} /* present_screen */


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
	   2) criar a superficie de desenho (aqui, uma surface propria que e
	      escalada pra janela via OpenGL -- ver present_screen())
	   COLOUR_DEPTH tambem deixou de existir como parametro. */
	/* A janela abre no dobro da resolucao do jogo se couber na tela (512x384
	   e minusculo num monitor atual), e pode ser redimensionada. */
	{
		int scale=1;
		SDL_Rect usable;

		if (SDL_GetDisplayUsableBounds(SDL_GetPrimaryDisplay(), &usable) &&
			usable.w>=SCREEN_X*2 && usable.h>=SCREEN_Y*2) scale=2;
		SDL_GL_SetAttribute(SDL_GL_RED_SIZE,8);
		SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,8);
		SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,8);
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);
		main_window = SDL_CreateWindow(application_name, SCREEN_X*scale, SCREEN_Y*scale,
									   flags|SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);
	}
	if (main_window == 0) {
		output_debug_message("Couldn't create window: %s\n", SDL_GetError());
		exit(-1);
	}
	if (fullscreen) SDL_HideCursor();

	gl_context = SDL_GL_CreateContext(main_window);
	if (gl_context == 0) {
		output_debug_message("Couldn't create the OpenGL context: %s\n", SDL_GetError());
		exit(-1);
	}
	SDL_GL_MakeCurrent(main_window, gl_context);
	/* Sem vsync: o ritmo do jogo (35 quadros/s, ver REDRAWING_PERIOD) e
	   controlado pelo loop principal, e esperar o vsync de 60Hz a cada
	   quadro o deixaria mais lento. */
	SDL_GL_SetSwapInterval(0);
	output_debug_message("OpenGL: %s / %s\n",(const char *)glGetString(GL_RENDERER),(const char *)glGetString(GL_VERSION));

	frame_buffer = new Uint32[SCREEN_X*SCREEN_Y];
	glGenTextures(1,&screen_tex);
	glBindTexture(GL_TEXTURE_2D,screen_tex);
	glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA8,SCREEN_X,SCREEN_Y,0,GL_BGRA,GL_UNSIGNED_BYTE,0);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_2D,0);

	SDL_Surface *screen = SDL_CreateSurface(SCREEN_X, SCREEN_Y, SDL_PIXELFORMAT_XRGB8888);

	if (screen == NULL) {
		output_debug_message("Couldn't create screen surface (%ix%i)", SCREEN_X, SCREEN_Y);
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
	Gamepad_finalization();
	postfx_release();
	if (screen_tex!=0) glDeleteTextures(1,&screen_tex);
	screen_tex=0;
	delete[] frame_buffer;
	frame_buffer=0;
	if (gl_context!=0) SDL_GL_DestroyContext(gl_context);
	gl_context=0;
	if (screen_sfc!=0) SDL_DestroySurface(screen_sfc);
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
	chdir_to_game_data();

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


	/* No Wayland o compositor associa a janela ao arquivo .desktop (icone
	   e nome na barra de tarefas) pelo app ID; sem isso aparece um icone
	   generico. Precisa ser chamado antes de SDL_Init. */
	SDL_SetAppMetadata(application_name, GAME_VERSION, "io.github.marciocr.RoadFighter");

	time=init_time=GetTickCount();
	screen_sfc = initializeSDL((fullscreen ? SDL_WINDOW_FULLSCREEN : 0));
	if (screen_sfc==0) return 0;

	Gamepad_initialization();

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
							/* Toogle FULLSCREEN mode: */
							set_fullscreen(!fullscreen);
						} /* if */
					} /* if */
#endif

					if (event.key.key==SDLK_RETURN) {
						SDL_Keymod modifiers;

						modifiers=SDL_GetModState();

						if ((modifiers&SDL_KMOD_ALT)!=0) {
							/* Toogle FULLSCREEN mode: */
							set_fullscreen(!fullscreen);
						} /* if */
					} /* if */
                    break;

                /* SDL_EVENT_QUIT (window close). SDL3 renomeou SDL_QUIT
                   para SDL_EVENT_QUIT, seguindo o novo padrao de nomes
                   SDL_EVENT_* para todo tipo de evento. */
                case SDL_EVENT_QUIT:
                    quit = true;
                    break;

                /* Gamepad hotplug: abre/fecha o controle e o encaixa no
                   proximo slot de jogador livre (ver gamepad.cpp). */
                case SDL_EVENT_GAMEPAD_ADDED:
                case SDL_EVENT_GAMEPAD_REMOVED:
                    Gamepad_handle_event(event);
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
			/* SDL3: SDL_Flip() nao existe mais; ver present_screen(). */
			present_screen();
		} /* if */
		SDL_Delay(1);
	}

	delete game;

	finalizeSDL();

	return 0;
} /* main */

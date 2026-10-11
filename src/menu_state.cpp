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
#define strupr(x) (x)
// TODO: really find a replacement for strupr
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "math.h"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_mixer/SDL_mixer.h>

#include "sound.h"

#include "CTile.h"
#include "CObject.h"
#include "CGame.h"
#include "CRoadFighter.h"
#include "auxiliar.h"
#include "gamepad.h"

#pragma GCC diagnostic ignored "-Wwrite-strings"

const int EFFECT_LENGTH=25;
const int TEXT_EFFECT_LENGTH=16;

extern int start_level;
extern void set_fullscreen(bool f);
extern bool get_fullscreen(void);

/* Menus (menu_current_menu): */
enum {MENU_MAIN=0, MENU_OPTIONS, MENU_KEYS1, MENU_KEYS2, MENU_ONE_PLAYER, MENU_TWO_PLAYERS,
	  MENU_AUDIO, MENU_VIDEO, MENU_CONTROLS};


static const char *input_device_name(int input)
{
	switch(input) {
	case INPUT_GAMEPAD1:return Gamepad_connected(0) ? "GAMEPAD 1":"GAMEPAD 1 (NOT CONNECTED)";
	case INPUT_GAMEPAD2:return Gamepad_connected(1) ? "GAMEPAD 2":"GAMEPAD 2 (NOT CONNECTED)";
	default:return "KEYBOARD";
	} /* switch */
} /* input_device_name */


/* Next device in KEYBOARD -> GAMEPAD 1 -> GAMEPAD 2, skipping the gamepad
   the other player is already using. */
static int next_input_device(int input,int other,int dir)
{
	do {
		input=(input+dir+N_INPUT_DEVICES)%N_INPUT_DEVICES;
	} while(input!=INPUT_KEYBOARD && input==other);
	return input;
} /* next_input_device */

int CRoadFighter::menu_cycle(void)
{
	if (state_timmer==0) {

		menu_effect=(rand()%3)+1;	/* 1,2 � 3 */ 
		menu_state=0;
		menu_timmer=0;
		menu_current_menu=0;
		menu_redefining_key=-1;
		if (S_menu_in!=0) Sound_play(S_menu_in);
		Sound_create_music("sound/menu_theme",-1);
		menu_credits_timmer=0;
	} /* if */ 

	switch(menu_state) {
	/* Appearing effect: */ 
	case 0:
		menu_timmer++;
		Sound_music_volume((menu_timmer*MIX_MAX_VOLUME)/EFFECT_LENGTH);
		if (menu_timmer>=EFFECT_LENGTH) {
			Sound_music_volume(MIX_MAX_VOLUME);
			menu_state=1;
			menu_timmer=0;
		} /* if */ 
		break;

	/* Appearing text: */ 
	case 1:
		menu_timmer++;
		if (menu_timmer>=TEXT_EFFECT_LENGTH) {
			menu_state=2;
			menu_timmer=0;
			menu_item=0;
		} /* if */ 
		break;

	/* Browsing menu: */ 
	case 2:
		if (menu_redefining_key!=-1) {
			int i;
			bool found=false;

			for(i=0;!found && i<SDL_SCANCODE_COUNT;i++) {
				if (keyboard[i] && !old_keyboard[i]) {
					if ((SDL_Scancode)i!=SDL_SCANCODE_ESCAPE &&
						(SDL_Scancode)i!=SDL_SCANCODE_F12) {
						switch(menu_redefining_key) {
						case 0:if (i!=left_key && i!=fire_key) right_key=(SDL_Scancode)i;
							   break;
						case 1:if (i!=right_key && i!=fire_key) left_key=(SDL_Scancode)i;
							   break;
						case 2:if (i!=right_key && i!=left_key) fire_key=(SDL_Scancode)i;
							   break;

						case 3:if (i!=left2_key && i!=fire2_key) right2_key=(SDL_Scancode)i;
							   break;
						case 4:if (i!=right2_key && i!=fire2_key) left2_key=(SDL_Scancode)i;
							   break;
						case 5:if (i!=right2_key && i!=left2_key) fire2_key=(SDL_Scancode)i;
							   break;
						} /* switch */ 
					} /* if */ 
					found=true;
					menu_redefining_key=-1;
				} /* if */ 
			} /* for */ 
		} else {
			bool left_pressed=(keyboard[left_key] && !old_keyboard[left_key]) ||
							  (keyboard[SDL_SCANCODE_LEFT] && !old_keyboard[SDL_SCANCODE_LEFT]);
			bool right_pressed=(keyboard[right_key] && !old_keyboard[right_key]) ||
							   (keyboard[SDL_SCANCODE_RIGHT] && !old_keyboard[SDL_SCANCODE_RIGHT]);
			bool select_pressed=(keyboard[fire_key] && !old_keyboard[fire_key]) ||
								(keyboard[SDL_SCANCODE_SPACE] && !old_keyboard[SDL_SCANCODE_SPACE]) ||
								(keyboard[SDL_SCANCODE_RETURN] && !old_keyboard[SDL_SCANCODE_RETURN]);
			/* Items whose value is changed in place with left/right (left
			   would otherwise mean "back" in the menus): */
			bool adjustable=(menu_current_menu==MENU_AUDIO && menu_item<=1) ||
							(menu_current_menu==MENU_VIDEO && menu_item==0) ||
							(menu_current_menu==MENU_CONTROLS && menu_item<=1);

			if (keyboard[SDL_SCANCODE_DOWN] && !old_keyboard[SDL_SCANCODE_DOWN]) {
				Sound_play(S_menu_move);
				menu_item++;
			} /* if */ 
			if (keyboard[SDL_SCANCODE_UP] && !old_keyboard[SDL_SCANCODE_UP]) {
				Sound_play(S_menu_move);
				menu_item--;
			} /* if */ 
			if (menu_item<0) menu_item=0;
			if (menu_item>menu_nitems-1) menu_item=menu_nitems-1;

			if (adjustable && (left_pressed || right_pressed ||
							   (select_pressed && menu_current_menu!=MENU_AUDIO))) {
				int dir=(left_pressed ? -1:1);

				switch(menu_current_menu) {
				case MENU_AUDIO:
					if (menu_item==0) Sound_set_music_master(Sound_get_music_master()+dir);
								 else Sound_set_sfx_master(Sound_get_sfx_master()+dir);
					break;
				case MENU_VIDEO:
					set_fullscreen(!get_fullscreen());
					break;
				case MENU_CONTROLS:
					if (menu_item==0) input1=next_input_device(input1,input2,dir);
								 else input2=next_input_device(input2,input1,dir);
					break;
				} /* switch */
				Sound_play(S_menu_move);
			} else if (select_pressed ||
				(keyboard[right_key] && !old_keyboard[right_key]) ||
				(keyboard[left_key] && !old_keyboard[left_key]) ||
				(keyboard[SDL_SCANCODE_ESCAPE] && !old_keyboard[SDL_SCANCODE_ESCAPE])) {
				if ((keyboard[left_key] && !old_keyboard[left_key]) ||
					(keyboard[SDL_SCANCODE_ESCAPE] && !old_keyboard[SDL_SCANCODE_ESCAPE])) {
					menu_item=menu_nitems-1;
				} /* if */ 

				Sound_play(S_menu_select);
				switch(menu_current_menu) {
				case 0:
					menu_state=3;
					menu_timmer=TEXT_EFFECT_LENGTH;
					break;
				case MENU_OPTIONS:
					if (menu_item==3) {
						if (game_remake_extras) game_remake_extras=false;
										   else game_remake_extras=true;
					} /* if */ 
					if (menu_item==4) {
						default_configuration();
					} /* if */ 

					if (menu_item==0 || menu_item==1 || menu_item==2 || menu_item==5) {
						menu_state=3;
						menu_timmer=TEXT_EFFECT_LENGTH;
					} /* if */ 
					break;
				case MENU_AUDIO:
				case MENU_VIDEO:
				case MENU_CONTROLS:
					/* Left/escape may have just jumped to BACK, so check the
					   item again instead of trusting "adjustable": */
					if (menu_item==menu_nitems-1 ||
						(menu_current_menu==MENU_CONTROLS && menu_item>=2)) {
						menu_state=3;
						menu_timmer=TEXT_EFFECT_LENGTH;
					} /* if */
					break;
				case 2:
					if (menu_item==0) menu_redefining_key=0;
					if (menu_item==1) menu_redefining_key=1;
					if (menu_item==2) menu_redefining_key=2;

					if (menu_item==3) {
						menu_state=3;
						menu_redefining_key=-1;
						menu_timmer=TEXT_EFFECT_LENGTH;
					} /* if */ 
					break;
				case 3:
					if (menu_item==0) menu_redefining_key=3;
					if (menu_item==1) menu_redefining_key=4;
					if (menu_item==2) menu_redefining_key=5;

					if (menu_item==3) {
						menu_state=3;
						menu_redefining_key=-1;
						menu_timmer=TEXT_EFFECT_LENGTH;
					} /* if */ 
					break;
				case 4:
					menu_state=3;
					menu_timmer=TEXT_EFFECT_LENGTH;
					break;
				case 5:
					menu_state=3;
					menu_timmer=TEXT_EFFECT_LENGTH;
					break;
				} /* switch */ 

			} /* if */ 
		} /* if */ 
		break;

	/* Disappearing text: */ 
	case 3:
		menu_timmer--;
		if (menu_timmer<0) {
			switch(menu_current_menu) {
			case 0:
				if (menu_item==0) {
					menu_state=1;
					menu_timmer=0;
					menu_current_menu=4;
				} /* if */ 
				if (menu_item==1) {
					menu_state=1;
					menu_timmer=0;
					menu_current_menu=5;
				} /* if */ 
				if (menu_item==2) {
					menu_state=1;
					menu_timmer=0;
					menu_current_menu=1;
				} /* if */ 
				if (menu_item==3) {
					menu_state=4;
					if (S_menu_out!=0) Sound_play(S_menu_out);
					menu_timmer=EFFECT_LENGTH;
				} /* if */ 
				break;
			case MENU_OPTIONS:
				if (menu_item==0) {
					menu_state=1;
					menu_timmer=0;
					menu_current_menu=MENU_AUDIO;
				} /* if */ 
				if (menu_item==1) {
					menu_state=1;
					menu_timmer=0;
					menu_current_menu=MENU_VIDEO;
				} /* if */ 
				if (menu_item==2) {
					menu_state=1;
					menu_timmer=0;
					menu_current_menu=MENU_CONTROLS;
				} /* if */ 
				if (menu_item==5) {
					menu_state=1;
					menu_timmer=0;
					menu_current_menu=MENU_MAIN;
					save_configuration();
				} /* if */ 
				break;
			case MENU_KEYS1:
			case MENU_KEYS2:
				if (menu_item==3) {
					menu_state=1;
					menu_timmer=0;
					menu_current_menu=MENU_CONTROLS;
				} /* if */ 
				break;
			case MENU_AUDIO:
			case MENU_VIDEO:
				/* The only non-adjustable item is BACK: */
				menu_state=1;
				menu_timmer=0;
				menu_current_menu=MENU_OPTIONS;
				break;
			case MENU_CONTROLS:
				if (menu_item==2) {
					menu_state=1;
					menu_timmer=0;
					menu_redefining_key=-1;
					menu_current_menu=MENU_KEYS1;
				} /* if */ 
				if (menu_item==3) {
					menu_state=1;
					menu_timmer=0;
					menu_redefining_key=-1;
					menu_current_menu=MENU_KEYS2;
				} /* if */ 
				if (menu_item==4) {
					menu_state=1;
					menu_timmer=0;
					menu_current_menu=MENU_OPTIONS;
				} /* if */ 
				break;
			case 4:
			case 5:
				if (menu_item==3) {
					menu_state=1;
					menu_timmer=0;
					menu_current_menu=0;
				} else {
					menu_state=4;
					if (S_menu_out!=0) Sound_play(S_menu_out);
					menu_timmer=EFFECT_LENGTH;
				} /* if */ 
				break;
			} /* switch */ 
		} /* if */ 
		break;

	/* Disappearing menu: */ 
	case 4:
		menu_timmer--;

		if (menu_timmer>=0) Sound_music_volume((menu_timmer*MIX_MAX_VOLUME)/EFFECT_LENGTH);
		if (menu_timmer<0) {

			switch(menu_current_menu) {
			case 0:
				if (menu_item==3) {
					Sound_release_music();
					return QUIT_STATE;
				} /* if */ 
				break;
			case 4:
				if (menu_item==0) {
					game_mode=0;
					n_players=1;
					current_level=start_level;
					scoreboard_x=-1;
					interlevel_state=0;
					interlevel_timmer=0;
					Sound_release_music();
					return INTERLEVEL_STATE;
				} /* if */ 
				if (menu_item==1) {
					game_mode=1;
					n_players=1;
					current_level=start_level;
					scoreboard_x=-1;
					interlevel_state=0;
					interlevel_timmer=0;
					Sound_release_music();
					return INTERLEVEL_STATE;
				} /* if */ 
				if (menu_item==2) {
					game_mode=2;
					n_players=1;
					current_level=start_level;
					scoreboard_x=-1;
					interlevel_state=0;
					interlevel_timmer=0;
					Sound_release_music();
					return INTERLEVEL_STATE;
				} /* if */ 
				break;
			case 5:
				if (menu_item==0) {
					game_mode=0;
					n_players=2;
					current_level=start_level;
					scoreboard_x=-1;
					interlevel_state=0;
					interlevel_timmer=0;
					n_players=2;
					Sound_release_music();
					return INTERLEVEL_STATE;
				} /* if */ 
				if (menu_item==1) {
					game_mode=1;
					n_players=2;
					current_level=start_level;
					scoreboard_x=-1;
					interlevel_state=0;
					interlevel_timmer=0;
					Sound_release_music();
					return INTERLEVEL_STATE;
				} /* if */ 
				if (menu_item==2) {
					game_mode=2;
					n_players=2;
					current_level=start_level;
					scoreboard_x=-1;
					interlevel_state=0;
					interlevel_timmer=0;
					Sound_release_music();
					return INTERLEVEL_STATE;
				} /* if */ 
				break;
			} /* switch */ 
		} /* if */ 
		break;
	} /* break */ 

	if (menu_state==1 || menu_state==2 || menu_state==3) {
		if (menu_current_menu==0) {
			delete menu_tittle_text;
			menu_tittle_text=new char[strlen("PLAY SELECT:")+1];
			strcpy(menu_tittle_text,"PLAY SELECT:");
			delete menu_options_text;
			menu_options_text=new char[strlen("ONE PLAYER\nTWO PLAYERS\nOPTIONS\nQUIT\n")+1];
			strcpy(menu_options_text,"ONE PLAYER\nTWO PLAYERS\nOPTIONS\nQUIT\n");
			menu_nitems=4;
		} /* if */ 

		if (menu_current_menu==1) {
			char tmp[256];
			delete menu_tittle_text;
			menu_tittle_text=new char[strlen("OPTIONS:")+1];
			strcpy(menu_tittle_text,"OPTIONS:");
			delete menu_options_text;
			sprintf(tmp,"AUDIO\nVIDEO\nCONTROLS\nEXTRAS: %s\nDEFAULT\nBACK\n",(game_remake_extras ? "ON":"OFF"));
			menu_options_text=new char[strlen(tmp)+1];
			strcpy(menu_options_text,tmp);
			menu_nitems=6;
		} /* if */ 

		if (menu_current_menu==MENU_AUDIO) {
			char tmp[256];
			delete menu_tittle_text;
			menu_tittle_text=new char[strlen("AUDIO:")+1];
			strcpy(menu_tittle_text,"AUDIO:");
			delete menu_options_text;
			sprintf(tmp,"MUSIC : %i%%\nSOUND FX : %i%%\nBACK\n",
					(Sound_get_music_master()*100)/SOUND_VOLUME_STEPS,
					(Sound_get_sfx_master()*100)/SOUND_VOLUME_STEPS);
			menu_options_text=new char[strlen(tmp)+1];
			strcpy(menu_options_text,tmp);
			menu_nitems=3;
		} /* if */ 

		if (menu_current_menu==MENU_VIDEO) {
			char tmp[256];
			delete menu_tittle_text;
			menu_tittle_text=new char[strlen("VIDEO:")+1];
			strcpy(menu_tittle_text,"VIDEO:");
			delete menu_options_text;
			sprintf(tmp,"FULLSCREEN : %s\nBACK\n",(get_fullscreen() ? "ON":"OFF"));
			menu_options_text=new char[strlen(tmp)+1];
			strcpy(menu_options_text,tmp);
			menu_nitems=2;
		} /* if */ 

		if (menu_current_menu==MENU_CONTROLS) {
			char tmp[256];
			delete menu_tittle_text;
			menu_tittle_text=new char[strlen("CONTROLS:")+1];
			strcpy(menu_tittle_text,"CONTROLS:");
			delete menu_options_text;
			sprintf(tmp,"PLAYER 1 : %s\nPLAYER 2 : %s\nPLAYER 1 KEYS\nPLAYER 2 KEYS\nBACK\n",
					input_device_name(input1),input_device_name(input2));
			menu_options_text=new char[strlen(tmp)+1];
			strcpy(menu_options_text,tmp);
			menu_nitems=5;
		} /* if */ 

		if (menu_current_menu==2) {
			char tmp[256];
			delete menu_tittle_text;
			menu_tittle_text=new char[strlen("PLAYER 1:")+1];
			strcpy(menu_tittle_text,"PLAYER 1:");
			delete menu_options_text;
			sprintf(tmp,"RIGHT : %s\nLEFT : %s\nFIRE : %s\nBACK\n",
					(menu_redefining_key==0 ? "":strupr(SDL_GetScancodeName((SDL_Scancode)right_key))),
					(menu_redefining_key==1 ? "":strupr(SDL_GetScancodeName((SDL_Scancode)left_key))),
					(menu_redefining_key==2 ? "":strupr(SDL_GetScancodeName((SDL_Scancode)fire_key))));
			menu_options_text=new char[strlen(tmp)+1];
			strcpy(menu_options_text,tmp);
			menu_nitems=4;
		} /* if */ 
		if (menu_current_menu==3) {
			char tmp[256];
			delete menu_tittle_text;
			menu_tittle_text=new char[strlen("PLAYER 2:")+1];
			strcpy(menu_tittle_text,"PLAYER 2:");
			delete menu_options_text;
			sprintf(tmp,"RIGHT : %s\nLEFT : %s\nFIRE : %s\nBACK\n",
					(menu_redefining_key==3 ? "":strupr(SDL_GetScancodeName((SDL_Scancode)right2_key))),
					(menu_redefining_key==4 ? "":strupr(SDL_GetScancodeName((SDL_Scancode)left2_key))),
					(menu_redefining_key==5 ? "":strupr(SDL_GetScancodeName((SDL_Scancode)fire2_key))));
			menu_options_text=new char[strlen(tmp)+1];
			strcpy(menu_options_text,tmp);
			menu_nitems=4;
		} /* if */ 
		if (menu_current_menu==4) {
			char tmp[256];
			delete menu_tittle_text;
			menu_tittle_text=new char[strlen("ONE PLAYER:")+1];
			strcpy(menu_tittle_text,"ONE PLAYER:");
			delete menu_options_text;
			sprintf(tmp,"LEVEL A\nLEVEL B\nLEVEL C\nBACK\n");
			menu_options_text=new char[strlen(tmp)+1];
			strcpy(menu_options_text,tmp);
			menu_nitems=4;
		} /* if */ 
		if (menu_current_menu==5) {
			char tmp[256];
			delete menu_tittle_text;
			menu_tittle_text=new char[strlen("TWO PLAYERS:")+1];
			strcpy(menu_tittle_text,"TWO PLAYERS:");
			delete menu_options_text;
			sprintf(tmp,"LEVEL A\nLEVEL B\nLEVEL C\nBACK\n");
			menu_options_text=new char[strlen(tmp)+1];
			strcpy(menu_options_text,tmp);
			menu_nitems=4;
		} /* if */ 
	} /* if */ 

	return MENU_STATE;
} /* CRoadFighter::menu_cycle */ 



void CRoadFighter::menu_draw(SDL_Surface *screen)
{
	float y_position=0.55F;

	switch(menu_state) {
	/* Appearing/Disappearing effect: */ 
	case 0:
	case 4:
		switch(menu_effect) {
		/* Fade: */ 
		case 0:
			{
				float f=float(menu_timmer)*(1.0F/float(EFFECT_LENGTH));
				if (f<0.0) f=0.0;
				if (f>=1.0) f=1.0;
				SDL_FillSurfaceRect(screen,0,0);
				SDL_BlitSurface(tittle_sfc,0,screen,0);
				if (f<1.0) surface_fader(screen,f,f,f,0);
			}

			break;

		/* Fall: */ 
		case 1:
			{
				int y=int(screen->h*float(menu_timmer)*(1.0F/float(EFFECT_LENGTH)));
				SDL_Rect r;

				if (y<0) y=0;
				if (y>screen->h) y=screen->h;
				y-=screen->h;
				r.x=0;
				r.y=y;
				r.w=screen->w;
				r.h=screen->h;

				SDL_FillSurfaceRect(screen,0,0);
				SDL_BlitSurface(tittle_sfc,0,screen,&r);
			}
			break;

		/* Zoom: */ 

		case 2:
			{
				float f=float(menu_timmer)*(1.0F/float(EFFECT_LENGTH));
				if (f<0.0) f=0.0;
				if (f>=1.0) f=1.0;
				SDL_FillSurfaceRect(screen,0,0);
				surface_rotozoom(tittle_sfc,screen,0,f,f,tittle_sfc->w/2,tittle_sfc->h/2,tittle_sfc->w/2,tittle_sfc->h/2);
			}
			break;

		/* Roto-zoom: */ 
		case 3:
			{
				float f=float(menu_timmer)*(1.0F/float(EFFECT_LENGTH));
				float a;
				if (f<0.0) f=0.0;
				if (f>=1.0) f=1.0;
				a=(1.0F-f)*260.0F;
				SDL_FillSurfaceRect(screen,0,0);
				surface_rotozoom(tittle_sfc,screen,a,f,f,tittle_sfc->w/2,tittle_sfc->h/2,tittle_sfc->w/2,tittle_sfc->h/2);
			}
			break;
		} /* switch */ 
		break;

	/* Appearing/Disappearing text: */ 
	case 1:
	case 3:
		{
			SDL_Rect r,r2;
			SDL_Color c,c1;
			SDL_Surface *menu_sfc,*options_sfc;

			/* Draw tittle: */ 
			SDL_FillSurfaceRect(screen,0,0);
			SDL_BlitSurface(tittle_sfc,0,screen,0);

			c.r=c.g=c.b=255;
			c1.r=c1.g=c1.b=224;
			menu_sfc=TTF_RenderText_Blended(font2big,menu_tittle_text,0,c);
			if (menu_nitems>5) {
				options_sfc=multiline_text_surface2(menu_options_text,0,font2small,c1,c1,-1,0);
			} else {
				if (menu_nitems>4) {
					options_sfc=multiline_text_surface2(menu_options_text,2,font2medium,c1,c1,-1,0);
				} else {
					options_sfc=multiline_text_surface2(menu_options_text,4,font2big,c1,c1,-1,0);
				} /* if */ 
			} /* if */ 

			/* Draw bar: */ 
			{
				int bar_length=int(menu_sfc->w*1.1);

				r.x=screen->w/2-bar_length/2;
				r.y=int(screen->h*y_position);
				r.w=bar_length;
				r.h=4;

				SDL_FillSurfaceRect(screen,&r,SDL_MapRGB(SDL_GetPixelFormatDetails(screen->format),SDL_GetSurfacePalette(screen),64,64,64));

				r.x=(screen->w/2-bar_length/2)+1;
				r.y=int(screen->h*y_position)+1;
				r.w=bar_length-2;
				r.h=2;

				SDL_FillSurfaceRect(screen,&r,SDL_MapRGB(SDL_GetPixelFormatDetails(screen->format),SDL_GetSurfacePalette(screen),255,255,255));
			}

			/* Draw text: */ 
			{
				int tmp;
				float f=float(menu_timmer)*(1.0F/float(TEXT_EFFECT_LENGTH));
				if (f<0.0) f=0.0;
				if (f>=1.0) f=1.0;

				tmp=int(f*menu_sfc->h);

				r.x=0;
				r.y=0;
				r.w=menu_sfc->w;
				r.h=tmp;
				r2.x=screen->w/2-menu_sfc->w/2;
				r2.y=int(screen->h*y_position-(tmp));
				r2.w=menu_sfc->w;
				r2.h=tmp;
				SDL_BlitSurface(menu_sfc,&r,screen,&r2);

				tmp=int(f*options_sfc->h);

				r.x=0;
				r.y=options_sfc->h-tmp;
				r.w=options_sfc->w;
				r.h=tmp;
				r2.x=screen->w/2-options_sfc->w/2;
				r2.y=int(screen->h*y_position+8);
				r2.w=options_sfc->w;
				r2.h=tmp;
				SDL_BlitSurface(options_sfc,&r,screen,&r2);
			}

			SDL_DestroySurface(menu_sfc);
			SDL_DestroySurface(options_sfc);
		}

		break;

	/* Browsing menu: */ 
	case 2:
		{
			SDL_Rect r;
			SDL_Color c,c1;
			SDL_Surface *menu_sfc,*options_sfc;
			int y_inc,y_start;
			float glow;

			/* Draw tittle: */ 
			SDL_FillSurfaceRect(screen,0,0);
			SDL_BlitSurface(tittle_sfc,0,screen,0);

			c.r=c.g=c.b=255;
			c1.r=c1.g=c1.b=224;
			menu_sfc=TTF_RenderText_Blended(font2big,menu_tittle_text,0,c);

			glow=(float(fabs(sin(state_timmer/10.0F)))/2.0F)+0.125F;

			if (menu_nitems>5) {
				options_sfc=multiline_text_surface2(menu_options_text,0,font2small,c1,c,menu_item,glow);
				y_inc=int((FONT_SIZE)*0.65F);
				y_start=9;
			} else {
				if (menu_nitems>4) {
					options_sfc=multiline_text_surface2(menu_options_text,2,font2medium,c1,c,menu_item,glow);
					y_inc=int(2+(FONT_SIZE)*0.8F);
					y_start=12;
				} else {
					options_sfc=multiline_text_surface2(menu_options_text,4,font2big,c1,c,menu_item,glow);
					y_inc=(4+FONT_SIZE);
					y_start=15;
				} /* if */ 
			} /* if */ 

			/* Draw bar: */ 
			{
				int bar_length=int(menu_sfc->w*1.1);

				r.x=screen->w/2-bar_length/2;
				r.y=int(screen->h*y_position);
				r.w=bar_length;
				r.h=4;

				SDL_FillSurfaceRect(screen,&r,SDL_MapRGB(SDL_GetPixelFormatDetails(screen->format),SDL_GetSurfacePalette(screen),64,64,64));

				r.x=(screen->w/2-bar_length/2)+1;
				r.y=int(screen->h*y_position)+1;
				r.w=bar_length-2;
				r.h=2;

				SDL_FillSurfaceRect(screen,&r,SDL_MapRGB(SDL_GetPixelFormatDetails(screen->format),SDL_GetSurfacePalette(screen),255,255,255));
			}

			/* Draw text: */ 
			{
				r.x=screen->w/2-menu_sfc->w/2;
				r.y=int(screen->h*y_position-(menu_sfc->h));
				r.w=menu_sfc->w;
				r.h=menu_sfc->h;
				SDL_BlitSurface(menu_sfc,0,screen,&r);

				r.x=screen->w/2-options_sfc->w/2;
				r.y=int(screen->h*y_position+8);
				r.w=options_sfc->w;
				r.h=options_sfc->h;
				SDL_BlitSurface(options_sfc,0,screen,&r);
			}

			/* Draw Arrow: */ 
//			if ((state_timmer%12)<6) {
				r.x=screen->w/2-options_sfc->w/2-arrow_sfc->w-10;
				r.y=int(screen->h*y_position+y_start)+menu_item*y_inc;
				r.w=arrow_sfc->w;
				r.h=arrow_sfc->h;
				SDL_BlitSurface(arrow_sfc,0,screen,&r);
//			}

			SDL_DestroySurface(menu_sfc);
			SDL_DestroySurface(options_sfc);
		}
		break;
	} /* switch */ 

	/* Credits: */ 
	{
		int start_x,start_x2;
		SDL_Rect r1,r2;
		float f=float(menu_timmer)*(1.0F/float(EFFECT_LENGTH));
		if (f<0.0) f=0.0;
		if (f>=1.0) f=1.0;

		SDL_BlitSurface(credits_sfc,0,credits2_sfc,0);
		if (menu_state==0 || menu_state==4) surface_fader(credits2_sfc,f,f,f,0);

		menu_credits_timmer+=2;

		if (menu_credits_timmer>screen->w) {
			if (menu_credits_timmer-screen->w>credits_sfc->w) {
				menu_credits_timmer=0;
				start_x=0;
			} else {
				start_x=menu_credits_timmer-screen->w;
			} /* if */ 
		} else {
			start_x=0;
		} /* if */ 
		start_x2=screen->w-menu_credits_timmer;
		if (start_x2<0) start_x2=0;
		r1.x=start_x;
		r1.y=0;
		r1.w=credits_sfc->w-start_x;
		r1.h=credits_sfc->h;
		r2.x=start_x2;
//		r2.y=174;
		r2.y=screen->h-credits_sfc->h;
		SDL_BlitSurface(credits2_sfc,&r1,screen,&r2);
	}

} /* CRoadFighter::menu_draw */ 

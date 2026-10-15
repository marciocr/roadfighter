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

#include <stdio.h>
#include <stdlib.h>
#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "sound.h"

#include "CTile.h"
#include "CObject.h"
#include "CGame.h"
#include "CRoadFighter.h"
#include "filehandling.h"
#include "gamepad.h"
#include "postfx.h"

extern void set_fullscreen(bool f);
extern bool get_fullscreen(void);

void CRoadFighter::load_configuration(void)
{
	int a,b,c;
	FILE *fp;

	fp=f1open("RoadFighter.cfg", "r", USERDATA);
	if (fp==0) {
		default_configuration();
		save_configuration();
		return;
	} /* if */ 
	if (3!=fscanf(fp,"%i %i %i",&a,&b,&c)) {
		fclose(fp);
		default_configuration();
		save_configuration();
		return;
	} /* if */ 
	left_key=a;
	right_key=b;
	fire_key=c;
	if (3!=fscanf(fp,"%i %i %i",&a,&b,&c)) {
		fclose(fp);
		default_configuration();
		save_configuration();
		return;
	} /* if */ 
	left2_key=a;
	right2_key=b;
	fire2_key=c;
	if (1!=fscanf(fp,"%i",&a)) {
		fclose(fp);
		default_configuration();
		save_configuration();
		return;
	} /* if */ 
	if (a==1) game_remake_extras=true;
		 else game_remake_extras=false;

	/* Opcoes de audio/video/controle: vieram depois, entao arquivos de
	   configuracao antigos nao as tem -- nesse caso ficam os valores
	   padrao so pra elas, sem perder as teclas ja configuradas. */
	Sound_set_music_master(SOUND_VOLUME_STEPS);
	Sound_set_sfx_master(SOUND_VOLUME_STEPS);
	input1=INPUT_GAMEPAD1;
	input2=INPUT_GAMEPAD2;
	if (2==fscanf(fp,"%i %i",&a,&b)) {
		Sound_set_music_master(a);
		Sound_set_sfx_master(b);
	} /* if */
	if (1==fscanf(fp,"%i",&a)) set_fullscreen(a==1);
	if (2==fscanf(fp,"%i %i",&a,&b)) {
		if (a>=0 && a<N_INPUT_DEVICES) input1=a;
		if (b>=0 && b<N_INPUT_DEVICES) input2=b;
	} /* if */
	postfx_set_filter(FILTER_OFF);
	if (1==fscanf(fp,"%i",&a)) postfx_set_filter(a);

	fclose(fp);
} /* CRoadFighter::load_configuration */ 


void CRoadFighter::save_configuration(void)
{
	FILE *fp;

	fp=f1open("RoadFighter.cfg", "w", USERDATA);
	if (fp==0) return;
	fprintf(fp,"%i %i %i\n",left_key,right_key,fire_key);
	fprintf(fp,"%i %i %i\n",left2_key,right2_key,fire2_key);
	if (game_remake_extras) fprintf(fp,"1\n");
					   else fprintf(fp,"0\n");
	fprintf(fp,"%i %i\n",Sound_get_music_master(),Sound_get_sfx_master());
	fprintf(fp,"%i\n",get_fullscreen() ? 1:0);
	fprintf(fp,"%i %i\n",input1,input2);
	fprintf(fp,"%i\n",video_filter);
	fclose(fp);
} /* CRoadFighter::save_configuration */ 


void CRoadFighter::default_configuration(void)
{
	left_key=SDL_SCANCODE_LEFT;
	right_key=SDL_SCANCODE_RIGHT;
	fire_key=SDL_SCANCODE_SPACE;

	left2_key=SDL_SCANCODE_A;
	right2_key=SDL_SCANCODE_D;
	fire2_key=SDL_SCANCODE_LSHIFT;

	game_remake_extras=true;

	Sound_set_music_master(SOUND_VOLUME_STEPS);
	Sound_set_sfx_master(SOUND_VOLUME_STEPS);
	set_fullscreen(false);
	postfx_set_filter(FILTER_OFF);
	input1=INPUT_GAMEPAD1;
	input2=INPUT_GAMEPAD2;
} /* CRoadFighter::default_configuration */ 


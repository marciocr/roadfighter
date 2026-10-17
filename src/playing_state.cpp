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
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_mixer/SDL_mixer.h>

#include "time.h"

#include "sound.h"

#include "CTile.h"
#include "CObject.h"
#include "CGame.h"
#include "CRoadFighter.h"
#include "auxiliar.h"
#include "filehandling.h"

#pragma GCC diagnostic ignored "-Wwrite-strings"

extern int start_level;

int CRoadFighter::playing_cycle(void)
{
	if (!game->cycle(keyboard,old_keyboard)) {
		/* The race is over (level completed, game over or quit): */
		save_high_score();
		if (game->level_completed()) {
			current_level++;
			return INTERLEVEL_STATE;
		} else {
			if (game->backspace_pressed) {
				delete game;
				game=0;
				current_level=start_level;
				return INTERLEVEL_STATE;
			} else {
				gameover_state=0;
				return GAMEOVER_STATE;
			} /* if */ 
		} /* if */ 
	} else {
		if (!playing_reachedend && game->level_completed()) {
			playing_reachedend=true;
			Sound_create_music("sound/levelcomplete",0);
		} /* if */ 
	} /* if */ 

	if (!playing_reachedend) {
		int volume=(game->get_game_timmer()*MIX_MAX_VOLUME)/25;
		if (volume<0) volume=0;
		if (volume>MIX_MAX_VOLUME) volume=MIX_MAX_VOLUME;
		Sound_music_volume(volume);
	} /* if */ 

	/* Scores: */ 
	{
		List<int> l;
		int *itmp;
		game->get_scores(&l);
		l.Rewind();
		while(l.Iterate(itmp)) if ((*itmp)>high_score) high_score=*itmp;
	}

	return PLAYING_STATE;
} /* CRoadFighter::playing_cycle */ 



void CRoadFighter::playing_draw(SDL_Surface *screen)
{
	SDL_Rect r;

	SDL_FillSurfaceRect(screen,0,0);

	scoreboard_draw(scoreboard_x,0,screen);

	/* Draw game: */ 
	if (n_players==1) {
		r.x=(desired_scoreboard_x-352);
		r.y=0;
		r.w=352;
		r.h=screen->h;
	} else {
		r.x=0;
		r.y=0;
		r.w=scoreboard_x;
		r.h=screen->h;
	} /* if */ 
	game->draw(screen,r);

} /* CRoadFighter::playing_draw */ 

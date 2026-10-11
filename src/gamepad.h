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

#ifndef __GAMEPAD_H
#define __GAMEPAD_H

#include <SDL3/SDL.h>

/* Opens every gamepad already connected at startup. */
void Gamepad_initialization(void);
void Gamepad_finalization(void);

/* Call for every SDL event; opens/closes gamepads on hotplug. */
void Gamepad_handle_event(const SDL_Event &event);

/* Which input device drives a player (see the CONTROLS options menu).
   The keyboard keys always work; choosing a gamepad adds that gamepad. */
#define INPUT_KEYBOARD	0
#define INPUT_GAMEPAD1	1
#define INPUT_GAMEPAD2	2
#define N_INPUT_DEVICES	3

bool Gamepad_connected(int pad);

/* The whole game reads input from a single bool[SDL_SCANCODE_COUNT] array
   (see CRoadFighter::keyboard). Instead of teaching every object/menu how
   to also read a gamepad, we fold connected gamepads into a mutable copy
   of that same array: the gamepad chosen by each player (input1/input2,
   one of INPUT_*) drives that player's configured left/right/fire keys,
   and any gamepad drives pause (F1) and back/escape. When "menu" is true,
   any gamepad also drives the arrow keys, RETURN (select) and ESCAPE, so
   menus can be used even with a gamepad no player has chosen. */
void Gamepad_apply_to_keys(bool *keys,bool menu,
							int input1,int left_key1,int right_key1,int fire_key1,
							int input2,int left_key2,int right_key2,int fire_key2);

#endif

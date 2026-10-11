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

#include "gamepad.h"
#include "debug.h"

#define MAX_GAMEPADS 2
#define STICK_DEADZONE 10000

static SDL_Gamepad *gamepads[MAX_GAMEPADS]={0,0};

static void open_gamepad_in_free_slot(SDL_JoystickID id)
{
	int i;

	for(i=0;i<MAX_GAMEPADS;i++) {
		if (gamepads[i]==0) {
			gamepads[i]=SDL_OpenGamepad(id);
			if (gamepads[i]!=0) {
				output_debug_message("Gamepad connected (player %i): %s\n",
									  i+1, SDL_GetGamepadName(gamepads[i]));
			} /* if */
			return;
		} /* if */
	} /* for */
} /* open_gamepad_in_free_slot */


void Gamepad_initialization(void)
{
	int i,count=0;
	SDL_JoystickID *ids;

	if (!SDL_InitSubSystem(SDL_INIT_GAMEPAD)) {
		output_debug_message("Couldn't initialize gamepad subsystem: %s\n", SDL_GetError());
		return;
	} /* if */

	ids=SDL_GetGamepads(&count);
	if (ids!=0) {
		for(i=0;i<count;i++) open_gamepad_in_free_slot(ids[i]);
		SDL_free(ids);
	} /* if */
} /* Gamepad_initialization */


void Gamepad_finalization(void)
{
	int i;

	for(i=0;i<MAX_GAMEPADS;i++) {
		if (gamepads[i]!=0) {
			SDL_CloseGamepad(gamepads[i]);
			gamepads[i]=0;
		} /* if */
	} /* for */
} /* Gamepad_finalization */


void Gamepad_handle_event(const SDL_Event &event)
{
	int i;

	switch(event.type) {
	case SDL_EVENT_GAMEPAD_ADDED:
		open_gamepad_in_free_slot(event.gdevice.which);
		break;

	case SDL_EVENT_GAMEPAD_REMOVED:
		for(i=0;i<MAX_GAMEPADS;i++) {
			if (gamepads[i]!=0 && SDL_GetGamepadID(gamepads[i])==event.gdevice.which) {
				output_debug_message("Gamepad disconnected (player %i).\n", i+1);
				SDL_CloseGamepad(gamepads[i]);
				gamepads[i]=0;
			} /* if */
		} /* for */
		break;
	} /* switch */
} /* Gamepad_handle_event */


/* Pressed towards the 'negative' direction (left/up), via d-pad or stick. */
static bool axis_negative(SDL_Gamepad *pad, SDL_GamepadButton dpad_button, SDL_GamepadAxis axis)
{
	return SDL_GetGamepadButton(pad,dpad_button) ||
		   SDL_GetGamepadAxis(pad,axis) < -STICK_DEADZONE;
} /* axis_negative */

/* Pressed towards the 'positive' direction (right/down), via d-pad or stick. */
static bool axis_positive(SDL_Gamepad *pad, SDL_GamepadButton dpad_button, SDL_GamepadAxis axis)
{
	return SDL_GetGamepadButton(pad,dpad_button) ||
		   SDL_GetGamepadAxis(pad,axis) > STICK_DEADZONE;
} /* axis_positive */


bool Gamepad_connected(int pad)
{
	return pad>=0 && pad<MAX_GAMEPADS && gamepads[pad]!=0;
} /* Gamepad_connected */


static void apply_player(int input, bool *keys, int left_key, int right_key, int fire_key)
{
	SDL_Gamepad *pad;

	if (input==INPUT_GAMEPAD1) pad=gamepads[0];
	else if (input==INPUT_GAMEPAD2) pad=gamepads[1];
	else return;
	if (pad==0) return;

	if (axis_negative(pad,SDL_GAMEPAD_BUTTON_DPAD_LEFT,SDL_GAMEPAD_AXIS_LEFTX)) keys[left_key]=true;
	if (axis_positive(pad,SDL_GAMEPAD_BUTTON_DPAD_RIGHT,SDL_GAMEPAD_AXIS_LEFTX)) keys[right_key]=true;
	if (SDL_GetGamepadButton(pad,SDL_GAMEPAD_BUTTON_SOUTH)) keys[fire_key]=true;
} /* apply_player */


static void apply_common(SDL_Gamepad *pad, bool *keys, bool menu)
{
	if (pad==0) return;

	/* Pause and back/escape aren't tied to a specific player: */
	if (SDL_GetGamepadButton(pad,SDL_GAMEPAD_BUTTON_START)) keys[SDL_SCANCODE_F1]=true;
	if (SDL_GetGamepadButton(pad,SDL_GAMEPAD_BUTTON_BACK)) keys[SDL_SCANCODE_ESCAPE]=true;

	if (!menu) return;

	if (axis_negative(pad,SDL_GAMEPAD_BUTTON_DPAD_UP,SDL_GAMEPAD_AXIS_LEFTY)) keys[SDL_SCANCODE_UP]=true;
	if (axis_positive(pad,SDL_GAMEPAD_BUTTON_DPAD_DOWN,SDL_GAMEPAD_AXIS_LEFTY)) keys[SDL_SCANCODE_DOWN]=true;
	if (axis_negative(pad,SDL_GAMEPAD_BUTTON_DPAD_LEFT,SDL_GAMEPAD_AXIS_LEFTX)) keys[SDL_SCANCODE_LEFT]=true;
	if (axis_positive(pad,SDL_GAMEPAD_BUTTON_DPAD_RIGHT,SDL_GAMEPAD_AXIS_LEFTX)) keys[SDL_SCANCODE_RIGHT]=true;
	if (SDL_GetGamepadButton(pad,SDL_GAMEPAD_BUTTON_SOUTH)) keys[SDL_SCANCODE_RETURN]=true;
	if (SDL_GetGamepadButton(pad,SDL_GAMEPAD_BUTTON_EAST)) keys[SDL_SCANCODE_ESCAPE]=true;
} /* apply_common */


void Gamepad_apply_to_keys(bool *keys,bool menu,
							int input1,int left_key1,int right_key1,int fire_key1,
							int input2,int left_key2,int right_key2,int fire_key2)
{
	int i;

	apply_player(input1,keys,left_key1,right_key1,fire_key1);
	apply_player(input2,keys,left_key2,right_key2,fire_key2);
	for(i=0;i<MAX_GAMEPADS;i++) apply_common(gamepads[i],keys,menu);
} /* Gamepad_apply_to_keys */

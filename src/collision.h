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

#ifndef __ROADFIGHTER_COLLISION
#define __ROADFIGHTER_COLLISION

#include <SDL3/SDL.h>

/* Mapa de colisao pixel-perfect: 1 bit por pixel (bit ligado = pixel
   solido), construido a partir do canal alpha da imagem. Substitui o
   sge_cdata/sge_make_cmap/sge_cmcheck/etc da antiga biblioteca SGE. */
struct CollisionMap
{
	Uint16 w,h;
	Uint8 *map;
};

CollisionMap *collision_make_map(SDL_Surface *img);
void collision_destroy_map(CollisionMap *cd);
bool collision_check(CollisionMap *cd1,int x1,int y1,CollisionMap *cd2,int x2,int y2);

#endif

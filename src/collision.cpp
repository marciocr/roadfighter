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

#include "collision.h"
#include "string.h"

static bool bit_set(const CollisionMap *cd,int x,int y)
{
	int offs=y*cd->w+x;

	return (cd->map[offs/8] & (1<<(offs%8)))!=0;
} /* bit_set */


static void set_bit(CollisionMap *cd,int x,int y)
{
	int offs=y*cd->w+x;

	cd->map[offs/8]|=(1<<(offs%8));
} /* set_bit */


CollisionMap *collision_make_map(SDL_Surface *img)
{
	CollisionMap *cd=new CollisionMap;
	int x,y;
	int mapsize=(img->w*img->h+7)/8+1;

	cd->w=img->w;
	cd->h=img->h;
	cd->map=new Uint8[mapsize];
	memset(cd->map,0,mapsize);

	/* Um pixel e "solido" se for opaco. As mascaras de colisao dos tiles
	   sao construidas via surface_bw() (auxiliar.cpp), que ja deixa cada
	   pixel totalmente opaco ou totalmente transparente -- entao so
	   olhar pro alpha e suficiente. */
	for(y=0;y<img->h;y++) {
		for(x=0;x<img->w;x++) {
			Uint8 r,g,b,a;

			SDL_ReadSurfacePixel(img,x,y,&r,&g,&b,&a);
			if (a!=0) set_bit(cd,x,y);
		} /* for */
	} /* for */

	return cd;
} /* collision_make_map */


void collision_destroy_map(CollisionMap *cd)
{
	delete[] cd->map;
	delete cd;
} /* collision_destroy_map */


bool collision_check(CollisionMap *cd1,int x1,int y1,CollisionMap *cd2,int x2,int y2)
{
	int ix1=x1>x2 ? x1 : x2;
	int iy1=y1>y2 ? y1 : y2;
	int ix2=(x1+cd1->w)<(x2+cd2->w) ? (x1+cd1->w) : (x2+cd2->w);
	int iy2=(y1+cd1->h)<(y2+cd2->h) ? (y1+cd1->h) : (y2+cd2->h);
	int x,y;

	if (ix1>=ix2 || iy1>=iy2) return false; /* bounding boxes nem se tocam */

	for(y=iy1;y<iy2;y++) {
		for(x=ix1;x<ix2;x++) {
			if (bit_set(cd1,x-x1,y-y1) && bit_set(cd2,x-x2,y-y2)) return true;
		} /* for */
	} /* for */

	return false;
} /* collision_check */

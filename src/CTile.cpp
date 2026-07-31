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

#include "assert.h"

#include "stdio.h"
#include "string.h"

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_image/SDL_image.h>

#include "CTile.h"
#include "math.h"

#include "auxiliar.h"

/* SDL3: SDL_DisplayFormatAlpha() nao existe mais; usamos SDL_ConvertSurface
   para o formato de pixel da janela (mesma solucao ja usada em
   CRoadFighter.cpp). */
extern SDL_Surface *screen_sfc;


CTile::CTile(void)
{
	r.x=0;
	r.y=0;
	r.h=0;
	r.w=0;
	orig=0;
	mask_visualization=0;
	mask_collision=0;
	collision_data=0;
} /* CTile::CTile */ 


CTile::CTile(int x,int y,int dx,int dy,SDL_Surface *o,bool collision)
{

	r.x=x;
	r.y=y;
	r.h=dy;
	r.w=dx;

	orig=SDL_CreateSurface(dx,dy,SDL_GetPixelFormatForMasks(32,RMASK,GMASK,BMASK,AMASK));
	SDL_SetSurfaceBlendMode(orig,SDL_BLENDMODE_NONE);
	SDL_SetSurfaceAlphaMod(orig,SDL_ALPHA_OPAQUE);
	SDL_BlitSurface(o,&r,orig,0);

	SDL_SetSurfaceBlendMode(orig,SDL_BLENDMODE_BLEND);
	SDL_SetSurfaceAlphaMod(orig,SDL_ALPHA_OPAQUE);
	surface_mask_from_bitmap(orig,o,r.x+r.w,r.y);
	mask_visualization=0;

	if (collision) {
		SDL_Rect r2;

		r2.x=r.x+(r.w*2);
		r2.y=r.y;
		r2.w=r.w;
		r2.h=r.h;
		mask_collision=SDL_CreateSurface(r.w,r.h,SDL_GetPixelFormatForMasks(32,RMASK,GMASK,BMASK,AMASK));
		SDL_BlitSurface(o,&r2,mask_collision,0);
		surface_bw(mask_collision,128);
		collision_data=collision_make_map(mask_collision);
	} else {
		mask_collision=0;
		collision_data=0;
	} /* if */ 
} /* CTile::CTile */ 


CTile::~CTile(void)
{
	free();
} /* CTile::CTile */ 


void CTile::draw(int x,int y,SDL_Surface *dest)
{
	SDL_Rect d;

	if (orig!=0) {
		d.x=x;
		d.y=y;
		d.w=r.w;
		d.h=r.h;
		SDL_BlitSurface(orig,0,dest,&d);
//		SDL_BlitSurface(mask_collision,0,dest,&d);
	} /* if */ 
} /* CTile::draw */ 


void CTile::draw_collision_mask(int x,int y,SDL_Surface *dest)
{
	SDL_Rect d;

	if (orig!=0) {
		if (mask_collision!=0) {
			d.x=x;
			d.y=y;
			d.w=r.w;
			d.h=r.h;
			SDL_BlitSurface(mask_collision,0,dest,&d);
		} /* if */ 
	} /* if */ 
} /* CTile::draw_collision_mask */ 


void CTile::draw_shaded(int x,int y,SDL_Surface *dest,int factor,int red,int green,int blue,int alpha)
{
	SDL_Rect d;

	if (orig!=0) {
		SDL_Surface *tmp;
		d.x=0;
		d.y=0;
		d.w=r.w;
		d.h=r.h;
		tmp=SDL_ConvertSurface(orig,screen_sfc->format);
		surface_shader(tmp,float(factor)/100.0F,red,green,blue,alpha);
		d.x=x;
		d.y=y;
		d.w=r.w;
		d.h=r.h;
		SDL_BlitSurface(tmp,0,dest,&d);
		SDL_DestroySurface(tmp);
	} /* if */ 
} /* CTile::draw_shaded */ 


void CTile::draw_bicolor(int x,int y,SDL_Surface *dest,int factor,int r1,int g1,int b1,int a1,int r2,int g2,int b2,int a2)
{
	SDL_Rect d;

	if (orig!=0) {
		SDL_Surface *tmp;
		d.x=0;
		d.y=0;
		d.w=r.w;
		d.h=r.h;
		tmp=SDL_ConvertSurface(orig,screen_sfc->format);
		surface_bicolor(tmp,float(factor)/100.0F,r1,g1,b1,a1,r2,g2,b2,a2);
		d.x=x;
		d.y=y;
		d.w=r.w;
		d.h=r.h;
		SDL_BlitSurface(tmp,0,dest,&d);
		SDL_DestroySurface(tmp);
	} /* if */ 
} /* CTile::draw_bicolor */ 


void CTile::draw_mask(int x,int y,SDL_Surface *dest)
{
	SDL_Rect d;

	if (orig!=0) {
		if (mask_visualization==0) {
			int i,j;

			mask_visualization=SDL_CreateSurface(r.w,r.h,SDL_PIXELFORMAT_XRGB8888);
			for(i=0;i<r.w;i++) {
				for(j=0;j<r.h;j++) {
					Uint32 color;
                    Uint8 r,g,b,a;

					SDL_LockSurface(orig);
                    color=getpixel(orig,i,j);
					SDL_UnlockSurface(orig);
                    SDL_GetRGBA(color,SDL_GetPixelFormatDetails(orig->format),SDL_GetSurfacePalette(orig),&r,&g,&b,&a);

                    color=SDL_MapRGBA(SDL_GetPixelFormatDetails(mask_visualization->format),SDL_GetSurfacePalette(mask_visualization),a,a,a,0);
					SDL_LockSurface(mask_visualization);
                    putpixel(mask_visualization,i,j,color);		
					SDL_UnlockSurface(mask_visualization);
				} /* for */ 
			} /* for */ 
		} /* if */ 

		d.x=x;
		d.y=y;
		d.w=r.w;
		d.h=r.h;
		SDL_BlitSurface(mask_visualization,0,dest,&d);
	} /* if */ 
} /* CTile::draw_mask */ 


void CTile::clear(void)
{
	orig=0;
	mask_visualization=0;
	mask_collision=0;
	collision_data=0;
} /* CTile::clear */ 


void CTile::free(void)
{
	if (orig!=0) SDL_DestroySurface(orig);
	orig=0;
	if (mask_visualization!=0) SDL_DestroySurface(mask_visualization);
	mask_visualization=0;

	if (mask_collision!=0) SDL_DestroySurface(mask_collision);
	mask_collision=0;

	if (collision_data!=0) collision_destroy_map(collision_data);
	collision_data=0;
} /* CTile::free */ 


void CTile::instance(CTile *t)
{
	r=t->r;
	orig=t->orig;
	mask_visualization=t->mask_visualization;
	mask_collision=t->mask_collision;
	collision_data=t->collision_data;

} /* CTile::instace */ 





TILE_SOURCE::TILE_SOURCE(void)
{
	fname=0;
	sfc=0;
} /* TILE_SOURCE::TILE_SOURCE */ 


TILE_SOURCE::TILE_SOURCE(char *filename)
{
	SDL_Surface *tmp_sfc;

	fname=new char[strlen(filename)+1];
	strcpy(fname,filename);
	tmp_sfc=IMG_Load(fname);

	/* SDL3: Rmask/Gmask/Bmask=0 aqui faz o SDL escolher os masks default
	   de 32bpp (sem canal alpha, equivalente a XRGB8888) -- o AMASK
	   passado e ignorado nesse caso tanto no SDL2 quanto aqui; e assim
	   mesmo que o codigo original se comportava. */
	sfc = SDL_CreateSurface(tmp_sfc->w,tmp_sfc->h,SDL_PIXELFORMAT_XRGB8888);
	SDL_SetSurfaceBlendMode(sfc,SDL_BLENDMODE_NONE);
	SDL_SetSurfaceAlphaMod(sfc,SDL_ALPHA_OPAQUE);
	SDL_BlitSurface(tmp_sfc,0,sfc,0);
	SDL_DestroySurface(tmp_sfc);
} /* TILE_SOURCE::TILE_SOURCE */ 


TILE_SOURCE::~TILE_SOURCE(void)
{
	delete fname;
	fname=0;
	SDL_DestroySurface(sfc);
} /* TILE_SOURCE::~TILE_SOURCE */ 


bool TILE_SOURCE::save(FILE *fp)
{
	fprintf(fp,"%s\n",fname);

	return true;
} /* TILE_SOURCE::save */ 


bool TILE_SOURCE::load(FILE *fp)
{
	char tmp[256];
	SDL_Surface *tmp_sfc;

	if (1!=fscanf(fp,"%s",tmp)) return false;

	if (fname!=0) delete fname;
	fname=new char[strlen(tmp)+1];
	strcpy(fname,tmp);
	tmp_sfc=IMG_Load(fname);

	/* SDL3: Rmask/Gmask/Bmask=0 aqui faz o SDL escolher os masks default
	   de 32bpp (sem canal alpha, equivalente a XRGB8888) -- o AMASK
	   passado e ignorado nesse caso tanto no SDL2 quanto aqui; e assim
	   mesmo que o codigo original se comportava. */
	sfc = SDL_CreateSurface(tmp_sfc->w,tmp_sfc->h,SDL_PIXELFORMAT_XRGB8888);
	SDL_SetSurfaceBlendMode(sfc,SDL_BLENDMODE_NONE);
	SDL_SetSurfaceAlphaMod(sfc,SDL_ALPHA_OPAQUE);
	SDL_BlitSurface(tmp_sfc,0,sfc,0);
	SDL_DestroySurface(tmp_sfc);

	return true;
} /* TILE_SOURCE::load */ 


bool TILE_SOURCE::cmp(char *n)
{
	if (strcmp(n,fname)==0) return true;

	return false;
} /* TILE_SOURCE::cmp */ 



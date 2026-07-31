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

#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>
#include "sound.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "assert.h"

#include "filehandling.h"

#pragma GCC diagnostic ignored "-Wwrite-strings"

#ifdef KITSCHY_DEBUG_MEMORY
#include "debug_memorymanager.h"
#endif

/* SDL3_mixer 3.x: a API antiga baseada em "canais" numerados globais
   (Mix_OpenAudio/Mix_Chunk/Mix_PlayChannel/Mix_Volume(canal,...)) foi
   substituida por um modelo de objetos: um MIX_Mixer (o dispositivo de
   audio), MIX_Audio (dados carregados -- substitui tanto Mix_Chunk quanto
   Mix_Music) e MIX_Track (um "slot" de reproducao, no qual se liga um
   MIX_Audio e se controla volume/loop/etc). Nao ha mais Mix_AllocateChannels/
   Mix_ReserveChannels; quem precisa de reproducao "solta" (fire-and-forget,
   sem controle depois de iniciar) usa MIX_PlayAudio(), e quem precisa
   controlar volume/loop por chamada usa uma MIX_Track propria. Para nao
   mudar a API publica deste modulo (Sound_play_ch, Sound_play com volume,
   etc, que esperam um "numero de canal"), mantemos aqui um pool fixo de
   MIX_Track que faz esse papel. */

#define N_TRACK_POOL 32

bool sound_enabled=false;
MIX_Mixer *mixer=0;
MIX_Audio *music_sound=0;
MIX_Track *music_track=0;
MIX_Track *track_pool[N_TRACK_POOL];
int n_channels=-1;
/* Guardamos o loop da musica atual so pra poder retomar a musica depois
   de um Stop_playback()/Resume_playback() (que destroem e recriam a
   track de musica do zero -- ver comentario em Resume_playback). */
static int music_loops=0;
/* Indice "round robin" pra escolher a proxima track livre do pool, no
   lugar da antiga escolha automatica de canal do Mix_PlayChannel(-1,...). */
static int next_channel=0;


static void destroy_track_pool(void)
{
	int i;

	for(i=0;i<N_TRACK_POOL;i++) {
		if (track_pool[i]!=0) MIX_DestroyTrack(track_pool[i]);
		track_pool[i]=0;
	} /* for */
	if (music_track!=0) MIX_DestroyTrack(music_track);
	music_track=0;
} /* destroy_track_pool */


static int next_pool_channel(void)
{
	int ch;

	if (n_channels<=0) return -1;
	ch=next_channel;
	next_channel=(next_channel+1)%n_channels;
	return ch;
} /* next_pool_channel */


static bool create_track_pool(void)
{
	int i;

	for(i=0;i<N_TRACK_POOL;i++) {
		track_pool[i]=MIX_CreateTrack(mixer);
		if (track_pool[i]==0) { destroy_track_pool(); return false; }
	} /* for */
	music_track=MIX_CreateTrack(mixer);
	if (music_track==0) { destroy_track_pool(); return false; }
	n_channels=N_TRACK_POOL;
	return true;
} /* create_track_pool */


bool Sound_initialization(void)
{
	if (-1==Sound_initialization(0,0)) return false;
	return true;
} /* Sound_initialization */


int Sound_initialization(int nc,int nrc)
{
	SDL_AudioSpec spec;
	n_channels=8;

	sound_enabled=true;
#ifdef __DEBUG_MESSAGES
	output_debug_message("Initializing SDL_mixer.\n");
#endif

	/* SDL3_mixer: primeiro inicializa a biblioteca (MIX_Init), depois cria
	   um "mixer" ligado a um dispositivo de audio de verdade
	   (MIX_CreateMixerDevice) -- os dois passos que antes eram feitos
	   implicitamente por um unico Mix_OpenAudio(freq,formato,canais,buffer). */
	if (!MIX_Init()) {
		sound_enabled=false;
#ifdef __DEBUG_MESSAGES
		output_debug_message("Unable to init SDL_mixer: %s\n", SDL_GetError());
		output_debug_message("Running the game without audio.\n");
#endif
		return -1;
	} /* if */

	spec.freq = 44100;
	spec.format = SDL_AUDIO_S16;
	spec.channels = 2;
	mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec);
	if (mixer==0)  {
	  sound_enabled=false;
	  MIX_Quit();
#ifdef __DEBUG_MESSAGES
  	  output_debug_message("Unable to open audio: %s\n", SDL_GetError());
  	  output_debug_message("Running the game without audio.\n");
#endif
	  return -1;
	} /* if */

#ifdef __DEBUG_MESSAGES
	/* SDL3: SDL_AudioDriverName(buf,len) foi substituida por
	   SDL_GetCurrentAudioDriver(), que ja devolve a string diretamente. */
	output_debug_message("    opened %s at %d Hz %d bit %s\n",
						 SDL_GetCurrentAudioDriver(), spec.freq, SDL_AUDIO_BITSIZE(spec.format),
						 spec.channels > 1 ? "stereo" : "mono");

	/* SDL3_mixer: a versao deixou de ser uma struct SDL_version preenchida
	   por MIX_VERSION(); agora e um unico inteiro, igual ao padrao adotado
	   por SDL3_image/SDL3_ttf (MIX_VERSION/MIX_Version() -> int). */
	output_debug_message("    compiled with SDL_mixer version: %d.%d.%d\n",
						 SDL_VERSIONNUM_MAJOR(SDL_MIXER_VERSION),
						 SDL_VERSIONNUM_MINOR(SDL_MIXER_VERSION),
						 SDL_VERSIONNUM_MICRO(SDL_MIXER_VERSION));
	output_debug_message("    running with SDL_mixer version: %d.%d.%d\n",
						 SDL_VERSIONNUM_MAJOR(MIX_Version()),
						 SDL_VERSIONNUM_MINOR(MIX_Version()),
						 SDL_VERSIONNUM_MICRO(MIX_Version()));
#endif

	if (!create_track_pool()) {
		sound_enabled=false;
		MIX_DestroyMixer(mixer); mixer=0;
		MIX_Quit();
		return -1;
	} /* if */

	if (nc>0) n_channels=nc<N_TRACK_POOL ? nc : N_TRACK_POOL;

	return n_channels;
} /* Sound_init */

void Sound_release(void)
{
	Sound_release_music();
	if (sound_enabled) {
		destroy_track_pool();
		MIX_DestroyMixer(mixer); mixer=0;
		MIX_Quit();
	} /* if */
	sound_enabled=false;
} /* Sound_release */


void Stop_playback(void)
{
	if (sound_enabled) {
		Sound_pause_music();
		destroy_track_pool();
		MIX_DestroyMixer(mixer); mixer=0;
		MIX_Quit();
		sound_enabled=false;
	} /* if */
} /* Stop_playback */

void Resume_playback(void)
{
	Resume_playback(0,0);
} /* Resume_playback */


int Resume_playback(int nc,int nrc)
{
	int n=Sound_initialization(nc,nrc);
	if (n!=-1) {
		/* Stop_playback() destruiu o mixer inteiro (inclusive a track de
		   musica), entao nao ha "pausa" de verdade pra retomar -- se havia
		   uma musica carregada, reatribuimos ela a nova track e tocamos de
		   novo desde o inicio, com o mesmo loop de antes. */
		if (music_sound!=0) {
			SDL_PropertiesID props;

			MIX_SetTrackAudio(music_track,music_sound);
			props=SDL_CreateProperties();
			SDL_SetNumberProperty(props,MIX_PROP_PLAY_LOOPS_NUMBER,music_loops);
			MIX_PlayTrack(music_track,props);
			SDL_DestroyProperties(props);
		} else {
			Sound_unpause_music();
		} /* if */
	} /* if */
	return n;
} /* Resume_playback */


/* a check to see if file is readable and greater than zero */
int file_check(char *fname)
{
	FILE *fp;

	if ((fp=f1open(fname, "r", GAMEDATA))!=NULL) {
		if (fseek(fp,0L, SEEK_END)==0 && ftell(fp)>0) {
  			fclose(fp);
			return true;
		} /* if */
		/* either the file could not be read (==-1) or size was zero (==0) */
#ifdef __DEBUG_MESSAGES
		output_debug_message("ERROR in file_check(): the file %s is corrupted.\n", fname);
#endif
		fclose(fp);
		exit(1);
	} /* if */
	return false;
} /* file_check */



SOUNDT Sound_create_sound(char *file)
{
	int n_ext=6;
	char *ext[6]={".WAV",".OGG",".MP3",".wav",".ogg",".mp3"};
	char name[256];
	int i;

	if (sound_enabled) {
		for(i=0;i<n_ext;i++) {
			strcpy(name,file);
			strcat(name,ext[i]);
			/* predecode=true: equivalente ao antigo Mix_LoadWAV, que
			   decodificava tudo de uma vez (bom pra efeitos curtos
			   tocados repetidamente). */
			if (file_check(name)) return MIX_LoadAudio(mixer,name,true);
		} /* for */

#ifdef __DEBUG_MESSAGES
		output_debug_message("ERROR in Sound_create_sound(): Could not load sound file: %s.(wav|ogg|mp3)\n",file);
#endif
		exit(1);
	} else {
		return 0;
	} /* if */
} /* Sound_create_sound */


void Sound_delete_sound(SOUNDT s)
{
	if (sound_enabled) MIX_DestroyAudio(s);
} /* Sound_delete_sound */


int Sound_play(SOUNDT s)
{
	/* Reproducao solta (fire-and-forget): o proprio SDL_mixer gerencia uma
	   track temporaria internamente e a descarta ao terminar -- equivalente
	   direto ao antigo Mix_PlayChannel(-1,s,0). */
	if (sound_enabled) return MIX_PlayAudio(mixer,s) ? 0 : -1;
	return -1;
} /* Sound_play */


int Sound_play(SOUNDT s,int volume)
{
	if (sound_enabled) {
		int ch=next_pool_channel();
		if (ch<0) return -1;
		MIX_SetTrackAudio(track_pool[ch],s);
		MIX_SetTrackGain(track_pool[ch],float(volume)/float(MIX_MAX_VOLUME));
		MIX_PlayTrack(track_pool[ch],0);
		return ch;
	} /* if */
	return -1;
} /* Sound_play */


int Sound_play_continuous(SOUNDT s)
{
	if (sound_enabled) {
		int ch=next_pool_channel();
		SDL_PropertiesID props;

		if (ch<0) return -1;
		MIX_SetTrackAudio(track_pool[ch],s);
		MIX_SetTrackGain(track_pool[ch],1.0F);
		props=SDL_CreateProperties();
		SDL_SetNumberProperty(props,MIX_PROP_PLAY_LOOPS_NUMBER,-1);
		MIX_PlayTrack(track_pool[ch],props);
		SDL_DestroyProperties(props);
		return ch;
	} /* if */
	return -1;
} /* Sound_play */


int Sound_play_continuous(SOUNDT s,int volume)
{
	if (sound_enabled) {
		int ch=next_pool_channel();
		SDL_PropertiesID props;

		if (ch<0) return -1;
		MIX_SetTrackAudio(track_pool[ch],s);
		MIX_SetTrackGain(track_pool[ch],float(volume)/float(MIX_MAX_VOLUME));
		props=SDL_CreateProperties();
		SDL_SetNumberProperty(props,MIX_PROP_PLAY_LOOPS_NUMBER,-1);
		MIX_PlayTrack(track_pool[ch],props);
		SDL_DestroyProperties(props);
		return ch;
	} /* if */
	return -1;
} /* Sound_play */


void Sound_play_ch(SOUNDT s,int ch)
{
	if (sound_enabled && ch>=0 && ch<n_channels) {
		MIX_SetTrackAudio(track_pool[ch],s);
		MIX_SetTrackGain(track_pool[ch],1.0F);
		MIX_PlayTrack(track_pool[ch],0);
	} /* if */
} /* Sound_play_ch */


void Sound_play_ch(SOUNDT s,int ch,int volume)
{
	if (sound_enabled && ch>=0 && ch<n_channels) {
		MIX_SetTrackAudio(track_pool[ch],s);
		MIX_SetTrackGain(track_pool[ch],float(volume)/float(MIX_MAX_VOLUME));
		MIX_PlayTrack(track_pool[ch],0);
	} /* if */
} /* Sound_play_ch */


MIX_Audio *Sound_create_stream(char *file)
{
	int n_ext=6;
	char *ext[6]={".WAV",".OGG",".MP3",".wav",".ogg",".mp3"};
	char name[256];
	int i;

	if (sound_enabled) {
		for(i=0;i<n_ext;i++) {
			strcpy(name,file);
			strcat(name,ext[i]);
			/* predecode=false: equivalente ao antigo Mix_LoadMUS, que
			   decodificava sob demanda em vez de tudo de uma vez (bom
			   pra faixas de musica, geralmente maiores). */
			if (file_check(name)) return MIX_LoadAudio(mixer,name,false);
		} /* for */

#ifdef __DEBUG_MESSAGES
		output_debug_message("ERROR in Sound_create_stream(): Could not load sound file: %s.(wav|ogg|mp3)\n", file);
#endif
		exit(1);
	} else {
		return 0;
	} /* if */
} /* Sound_create_stream */


void Sound_create_music(char *f1,int loops)
{
	if (sound_enabled) {
		if (f1!=0) {
			SDL_PropertiesID props;

			music_sound=Sound_create_stream(f1);
			music_loops=loops;
			MIX_SetTrackAudio(music_track,music_sound);
			props=SDL_CreateProperties();
			SDL_SetNumberProperty(props,MIX_PROP_PLAY_LOOPS_NUMBER,loops);
			MIX_PlayTrack(music_track,props);
			SDL_DestroyProperties(props);
		} else {
			music_sound=0;
		} /* if */

//		playing_music=true;
	} /* if */
} /* Sound_create_music */


bool Sound_file_test(char *f1)
{
	int n_ext=6;
	char *ext[6]={".WAV",".OGG",".MP3",".wav",".ogg",".mp3"};
	char name[256];
	int i;

	if (sound_enabled) {
		for(i=0;i<n_ext;i++) {
			strcpy(name,f1);
			strcat(name,ext[i]);
			if (file_check(name)) return true;
		} /* for */

		return false;
	} else {
		return false;
	} /* if */
} /* Sound_file_test */


void Sound_release_music(void)
{
	if (sound_enabled) {
//		playing_music=false;
		MIX_StopTrack(music_track,0);
		if (music_sound!=0) MIX_DestroyAudio(music_sound);
		music_sound=0;
	} /* if */
} /* Sound_release_music */



void Sound_pause_music(void)
{
	if (sound_enabled) MIX_PauseTrack(music_track);
} /* Sound_pause_music */


void Sound_unpause_music(void)
{
	if (sound_enabled) MIX_ResumeTrack(music_track);
} /* Sound_unpause_music */


void Sound_music_volume(int volume)
{
	if (volume<0) volume=0;
	if (volume>127) volume=127;
	if (sound_enabled) MIX_SetTrackGain(music_track,float(volume)/float(MIX_MAX_VOLUME));
} /* Sound_music_volume */

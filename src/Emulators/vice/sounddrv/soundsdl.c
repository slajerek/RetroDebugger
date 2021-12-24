/*
 * soundsdl.c - Implementation of the Simple Directmedia Layer sound device
 *
 * Written by
 *  Teemu Rantanen <tvr@cs.hut.fi>
 *  Daniel Aarno <macbishop@users.sourceforge.net>
 *  Hannu Nuotio <hannu.nuotio@tut.fi>
 *
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

#include "vice.h"
#include "SYS_Types.h"

#include "vice_sdl.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <string.h>

#include "lib.h"
#include "sound.h"

#include "log.h"

#include "ViceWrapper.h"

static SWORD *sdl_buf = NULL;
//static SDL_AudioSpec sdl_spec;
static volatile int sdl_inptr = 0;
static volatile int sdl_outptr = 0;
static volatile int sdl_full = 0;
static int sdl_len = 0;

int sidNumChannels = 0;

void sdl_callback(void *userdata, uint8 *stream, int numSamples)
{
//	LOGD("sdl sound callback: numSamples=%d numChannels=%d", numSamples, sidNumChannels);
	uint8 *dest;
	uint8 *src;
	int i;

	if (sidNumChannels == 1)
	{
		// mono
		int len = numSamples * sizeof(SWORD);
		
		int	amount, total;
		total = 0;
		
		//  while (total < len/sizeof(SWORD))
		while (total < numSamples)
		{
			amount = sdl_inptr - sdl_outptr;
			if (amount <= 0)
			{
				amount = sdl_len - sdl_outptr;
			}
			
			//      if (amount + total > len/sizeof(SWORD))
			if (amount + total > numSamples)
			{
				//          amount = len/sizeof(SWORD) - total;
				amount = numSamples - total;
			}
			
			sdl_full = 0;
			
			if (!amount)
			{
				LOGError("sdl_callback: amount=%d", amount);
				memset(stream + total*sizeof(SWORD), 0, len - total*sizeof(SWORD));
				return;
			}

			dest = stream + total*sizeof(SWORD);
			src = (uint8*) (sdl_buf + sdl_outptr);
			
			for (i = 0; i < amount; i++)
			{
				uint8 s1, s2;
				
				s1 = *src;
				src++;
				s2 = *src;
				src++;
				
				// L
				*dest = s1;
				dest++;
				*dest = s2;
				dest++;
				
				// R
				*dest = s1;
				dest++;
				*dest = s2;
				dest++;
			}
			
			total += amount;
			sdl_outptr += amount;
			
			if (sdl_outptr == sdl_len)
			{
				sdl_outptr = 0;
			}
		}
	}
	else if (sidNumChannels == 2)
	{
		// stereo
		
		int len = numSamples * sizeof(SWORD) * 2;
		
		int	amount, total;
		total = 0;
		
		//  while (total < len/sizeof(SWORD))
		while (total < numSamples)
		{
			amount = (sdl_inptr - sdl_outptr) / 2;
			if (amount <= 0)
			{
				amount = (sdl_len - sdl_outptr) / 2;
			}
			
			//      if (amount + total > len/sizeof(SWORD))
			if (amount + total > numSamples)
			{
				//          amount = len/sizeof(SWORD) - total;
				amount = numSamples - total;
			}
			
			sdl_full = 0;
			
			if (!amount)
			{
				LOGError("sdl_callback: amount=%d", amount);
				memset(stream + total*sizeof(SWORD)*2, 0, len - total*sizeof(SWORD)*2);
				return;
			}
			
			memcpy(stream + total*sizeof(SWORD)*2, sdl_buf + sdl_outptr,
				   amount*sizeof(SWORD)*2);

			total += amount;
			sdl_outptr += amount*2;
			
			if (sdl_outptr == sdl_len)
			{
				sdl_outptr = 0;
			}
		}
	}
	

}

static int sdl_init(const char *param, int *speed,
                    int *fragsize, int *fragnr, int *channels)
{
    int nr;

//	LOGD("soundsdl: sdl_init, channels=%d fragsize=%d fragnr=%d", *channels, *fragsize, *fragnr);
	
	sidNumChannels = *channels;
	
//    SDL_AudioSpec spec;

//    memset(&spec, 0, sizeof(spec));
//    spec.freq = *speed;
//    spec.format = AUDIO_S16;
//    spec.channels = *channels;
//    spec.samples = *fragsize;
//    spec.callback = sdl_callback;
//
//    if (SDL_OpenAudio(&spec, &sdl_spec)) {
//        return 1;
//    }

//    if ((sdl_spec.format != AUDIO_S16 && sdl_spec.format != AUDIO_S16MSB) || sdl_spec.channels != *channels) {
//        SDL_CloseAudio();
//        return 1;
//    }

    /* recalculate the number of fragments since the frag size might
     * have changed and we want to keep approximately the same
     * buffersize */
	
	// LOGTODO("soundsdl: sdl_init: num samples");  512 samples is default for portaudio
	
//    nr = (*fragnr) * (*fragsize) / spec.samples;
	nr = (*fragnr) * (*fragsize) / 512; //(*fragsize);

//    sdl_len = sdl_spec.samples * nr;
	sdl_len = 512 * nr;
	
    sdl_inptr = sdl_outptr = sdl_full = 0;
	
    sdl_buf = lib_malloc(sizeof(SWORD)*sdl_len);
	memset(sdl_buf, 0, sizeof(SWORD)*sdl_len);

	
//    if (!sdl_buf) {
//        SDL_CloseAudio();
//        return 1;
//    }

	*speed = 44100; //sdl_spec.freq;
	*fragsize = 512; //spec.samples;
    *fragnr = nr;
	
//    SDL_PauseAudio(0);
	

	// add channel to audio queue
	c64d_sound_init();

//	LOGD("sdl_init: finished, sdl_buf=%x", sdl_buf);
    return 0;
}

#if defined(WORDS_BIGENDIAN) && (!defined(HAVE_SWAB) || defined(__BEOS__))
#if !defined(AMIGA_MORPHOS) && !defined(AMIGA_M68K)
void swab(void *src, void *dst, size_t length)
{
    const char *from=src;
    char *to=dst;
    size_t ptr;

    for (ptr=1; ptr<length; ptr+=2)  {
        char p=from[ptr];
        char q=from[ptr-1];
        to[ptr-1]=p;
        to[ptr]=q;
    }

    if (ptr==length) {
        to[ptr-1]=0;
    }
}
#else
#define swab(src, dst, length)              \
    do {                                    \
        const char *from=src;               \
        char *to=dst;                       \
        size_t ptr;                         \
                                            \
        for (ptr=1; ptr<(length); ptr+=2) { \
            char p=from[ptr];               \
            char q=from[ptr-1];             \
            to[ptr-1]=p;                    \
            to[ptr]=q;                      \
        }                                   \
        if (ptr==(length)) {                \
            to[ptr-1]=0;                    \
        }                                   \
    }                                       \
    while (0)
#endif
#endif

static int sdl_write(SWORD *pbuf, size_t nr)
{
//	LOGD("soundsdl: sdl_write, nr=%d");
	
    int total, amount;
    total = 0;

#ifdef WORDS_BIGENDIAN
    if (sdl_spec.format != AUDIO_S16MSB)
    {
        /* Swap bytes if we're on a big-endian machine, like the Macintosh */
        swab(pbuf, pbuf, sizeof(SWORD)*nr);
    }
#endif

    while (total < nr)
	{
        amount = sdl_outptr - sdl_inptr;

        if (amount <= 0) {
            amount = sdl_len - sdl_inptr;
        }

        if (total + amount > nr) {
            amount = nr - total;
        }

        if (amount <= 0)
		{
        //    SDL_Delay(5);
            continue;
        }

        memcpy(sdl_buf + sdl_inptr, pbuf + total, amount*sizeof(SWORD));
        sdl_inptr += amount;
        total += amount;

        if (sdl_inptr == sdl_len)
		{
            sdl_inptr = 0;
        }
    }

    if (sdl_inptr == sdl_outptr)
	{
        sdl_full = 1;
    }

    return 0;
}

static int sdl_bufferspace(void)
{
//	LOGD("soundsdl: sdl_bufferspace");

	int amount;
	int ret;

    if (sdl_full)
	{
        amount = sdl_len;
    }
	else
	{
        amount = sdl_inptr - sdl_outptr;
    }

    if (amount < 0)
	{
        amount += sdl_len;
    }
	
	ret = sdl_len - amount;

	if (sidNumChannels == 2)
	{
		ret /= 2;
	}

	//	LOGD("sdl_bufferspace ret=%d", ret);

    return ret;
}

static void sdl_close(void)
{
//    SDL_CloseAudio();
//    lib_free(sdl_buf);
    sdl_buf = NULL;
    sdl_inptr = sdl_outptr = sdl_len = sdl_full = 0;
	
	c64d_sound_pause();
}

static int sdl_suspend(void)
{
//    SDL_PauseAudio(1);
    sdl_full = 0;
	
	c64d_sound_pause();
	
    return 0;
}

static int sdl_resume(void)
{
    //SDL_PauseAudio(0);
	
	c64d_sound_resume();
	
    return 0;
}

static sound_device_t sdl_device =
{
    "sdl",
    sdl_init,
    sdl_write,
    NULL,
    NULL,
    sdl_bufferspace,
    sdl_close,
    sdl_suspend,
    sdl_resume,
    1,
    2
};

int sound_init_sdl_device(void)
{
    return sound_register_device(&sdl_device);
}

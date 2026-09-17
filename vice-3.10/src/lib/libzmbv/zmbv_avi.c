/*
 * Copyright (C) 2002-2013  The DOSBox Team
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * C translation by Ketmar // Invisible Vector
 */
#include "zmbv_avi.h"

#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <math.h>

#include <sys/stat.h>
#include <sys/types.h>

#ifndef O_CLOEXEC
#define O_CLOEXEC 0
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif

#define AVI_HEADER_SIZE  (500)

#if __BYTE_ORDER == __LITTLE_ENDIAN
# define HTOBE32(x) __builtin_bswap32(x)
# define BETOH32(x) __builtin_bswap32(x)
# define HTOBE16(x) __builtin_bswap16(x)
# define BETOH16(x) __builtin_bswap16(x)
# define HTOLE32(x) (x)
# define LETOH32(x) (x)
# define HTOLE16(x) (x)
# define LETOH16(x) (x)
#else
# define HTOBE32(x) (x)
# define BETOH32(x) (x)
# define HTOBE16(x) (x)
# define BETOH16(x) (x)
# define HTOLE32(x) __builtin_bswap32(x)
# define LETOH32(x) __builtin_bswap32(x)
# define HTOLE16(x) __builtin_bswap16(x)
# define LETOH16(x) __builtin_bswap16(x)
#endif


#define CODEC_4CC "ZMBV"


typedef struct zmbv_avi_s *zmbv_avi_t;

struct zmbv_avi_s {
  int fd;
  uint8_t *index;
  uint32_t indexsize, indexused;
  uint32_t width, height;
  double fps;
  uint32_t frames;
  uint32_t written;
  //uint32_t audioused; // always 0 for now
  uint32_t audiowritten;
  uint32_t audiorate; // 44100?
  int was_file_error;
};


zmbv_avi_t zmbv_avi_start (const char *fname, int width, int height, double fps, int audiorate) {
  if (fname != NULL && fname[0] && width > 0 && height > 0 && width <= 16384 && height <= 16384 && fps > 0 && fps <= 100) {
    zmbv_avi_t zavi = malloc(sizeof(*zavi));
    if (zavi == NULL) return NULL;
    memset(zavi, 0, sizeof(*zavi));
    zavi->fd = -1;
    zavi->indexsize = 16*4096;
    zavi->indexused = 8;
    zavi->index = malloc(zavi->indexsize);
    if (zavi->index == NULL) goto error;
    zavi->fd = open(fname, O_WRONLY|O_CREAT|O_TRUNC|O_CLOEXEC|O_BINARY, 0644);
    if (zavi->fd < 0) goto error;
    zavi->width = width;
    zavi->height = height;
    zavi->fps = fps;
    {
      uint8_t eh[AVI_HEADER_SIZE];
      memset(eh, 0, sizeof(eh));
      if (write(zavi->fd, eh, sizeof(eh)) != sizeof(eh)) goto error;
    }
    zavi->frames = 0;
    zavi->written = 0;
    //zavi->audioused = 0;
    zavi->audiorate = audiorate;
    zavi->audiowritten = 0;
    return zavi;
error:
    if (zavi->fd >= 0) {
      close(zavi->fd);
      unlink(fname);
    }
    if (zavi->index != NULL) free(zavi->index);
    free(zavi);
  }
  return NULL;
}


#define AVIOUT4(_S_)  do { memcpy(&avi_header[header_pos], _S_, 4); header_pos += 4; } while (0)
#define AVIOUTw(_S_)  do { uint16_t w = HTOLE16(_S_); memcpy(&avi_header[header_pos], &w, 2); header_pos += 2; } while (0)
#define AVIOUTd(_S_)  do { uint32_t d = HTOLE32(_S_); memcpy(&avi_header[header_pos], &d, 4); header_pos += 4; } while (0)

int zmbv_avi_stop (zmbv_avi_t zavi) {
  int res = -1;
  if (zavi != NULL) {
    if (!zavi->was_file_error && zavi->fd >= 0) {
      uint8_t avi_header[AVI_HEADER_SIZE];
      uint32_t main_list;
      uint32_t header_pos = 0;
      /* try and write an avi header */
      AVIOUT4("RIFF"); // riff header
      AVIOUTd(AVI_HEADER_SIZE+zavi->written-8+zavi->indexused);
      AVIOUT4("AVI ");
      AVIOUT4("LIST"); // list header
      main_list = header_pos;
      AVIOUTd(0); // TODO size of list
      AVIOUT4("hdrl");

      AVIOUT4("avih");
      AVIOUTd(56); // # of bytes to follow
      AVIOUTd((uint32_t)round(1000000.0f/zavi->fps)); // microseconds per frame
      AVIOUTd(0);
      AVIOUTd(0); // PaddingGranularity (whatever that might be)
      AVIOUTd(0x110); // Flags, 0x10 has index, 0x100 interleaved
      AVIOUTd(zavi->frames); // TotalFrames
      AVIOUTd(0); // InitialFrames
      AVIOUTd(2); // Stream count
      AVIOUTd(0); // SuggestedBufferSize
      AVIOUTd(zavi->width); // Width
      AVIOUTd(zavi->height); // Height
      AVIOUTd(0); // TimeScale:  Unit used to measure time
      AVIOUTd(0); // DataRate:   Data rate of playback
      AVIOUTd(0); // StartTime:  Starting time of AVI data
      AVIOUTd(0); // DataLength: Size of AVI data chunk

      // video stream list
      AVIOUT4("LIST");
      AVIOUTd(4+8+56+8+40); // size of the list
      AVIOUT4("strl");
      // video stream header
      AVIOUT4("strh");
      AVIOUTd(56); // # of bytes to follow
      AVIOUT4("vids"); // type
      AVIOUT4(CODEC_4CC); // handler */
      AVIOUTd(0); // Flags
      AVIOUTd(0); // Reserved, MS says: wPriority, wLanguage
      AVIOUTd(0); // InitialFrames
      AVIOUTd(1000000); // Scale
      AVIOUTd((uint32_t)round(1000000.0f*zavi->fps)); // Rate: Rate/Scale == samples/second
      AVIOUTd(0); // Start
      AVIOUTd(zavi->frames); // Length
      AVIOUTd(0); // SuggestedBufferSize
      AVIOUTd(~0); // Quality
      AVIOUTd(0); // SampleSize
      AVIOUTd(0); // Frame
      AVIOUTd(0); // Frame
      // the video stream format
      AVIOUT4("strf");
      AVIOUTd(40); // # of bytes to follow
      AVIOUTd(40); // Size
      AVIOUTd(zavi->width); // Width
      AVIOUTd(zavi->height); // Height
      //OUTSHRT(1); OUTSHRT(24); // Planes, Count
      AVIOUTd(0);
      AVIOUT4(CODEC_4CC); // Compression
      AVIOUTd(zavi->width*zavi->height*4); // SizeImage (in bytes?)
      AVIOUTd(0); // XPelsPerMeter
      AVIOUTd(0); // YPelsPerMeter
      AVIOUTd(0); // ClrUsed: Number of colors used
      AVIOUTd(0); // ClrImportant: Number of colors important

      if (zavi->audiowritten > 0) {
        // audio stream list
        AVIOUT4("LIST");
        AVIOUTd(4+8+56+8+16); // Length of list in bytes
        AVIOUT4("strl");
        // the audio stream header
        AVIOUT4("strh");
        AVIOUTd(56); // # of bytes to follow
        AVIOUT4("auds");
        AVIOUTd(0); // Format (Optionally)
        AVIOUTd(0); // Flags
        AVIOUTd(0); // Reserved, MS says: wPriority, wLanguage
        AVIOUTd(0); // InitialFrames
        AVIOUTd(4); // Scale
        AVIOUTd(zavi->audiorate*4); // rate, actual rate is scale/rate
        AVIOUTd(0); // Start
        if (!zavi->audiorate) zavi->audiorate = 1;
        AVIOUTd(zavi->audiowritten/4); // Length
        AVIOUTd(0); // SuggestedBufferSize
        AVIOUTd(~0); // Quality
        AVIOUTd(4); // SampleSize
        AVIOUTd(0); // Frame
        AVIOUTd(0); // Frame
        // the audio stream format
        AVIOUT4("strf");
        AVIOUTd(16); // # of bytes to follow
        AVIOUTw(1); // Format, WAVE_ZMBV_FORMAT_PCM
        AVIOUTw(2); // Number of channels
        AVIOUTd(zavi->audiorate); // SamplesPerSec
        AVIOUTd(zavi->audiorate*4); // AvgBytesPerSec
        AVIOUTw(4); // BlockAlign
        AVIOUTw(16); // BitsPerSample
      }
      int nmain = header_pos-main_list-4;
      // finish stream list, i.e. put number of bytes in the list to proper pos
      int njunk = AVI_HEADER_SIZE-8-12-header_pos;
      AVIOUT4("JUNK");
      AVIOUTd(njunk);
      // fix the size of the main list
      header_pos = main_list;
      AVIOUTd(nmain);
      header_pos = AVI_HEADER_SIZE-12;
      AVIOUT4("LIST");
      AVIOUTd(zavi->written+4); // Length of list in bytes
      AVIOUT4("movi");
      // first add the index table to the end
      memcpy(zavi->index, "idx1", 4);
      {
        uint32_t d = HTOLE32(zavi->indexused-8);
        memcpy(zavi->index+4, &d, 4);
        if (write(zavi->fd, zavi->index, zavi->indexused) != zavi->indexused) goto quit;
      }
      // now replace the header
      if (lseek(zavi->fd, 0, SEEK_SET) == (off_t)-1) goto quit;
      if (write(zavi->fd, &avi_header, AVI_HEADER_SIZE) != AVI_HEADER_SIZE) goto quit;
      res = 0;
    }
quit:
    if (zavi->fd >= 0) { if (close(zavi->fd) < 0 && res == 0) res = -1; }
    if (zavi->index != NULL) free(zavi->index);
    free(zavi);
  }
  return -1;
}

#undef AVIOUT4
#undef AVIOUTw
#undef AVIOUTd


int zmbv_avi_write_chunk (zmbv_avi_t zavi, const char tag[4], uint32_t size, const void *data, uint32_t flags) {
  if (zavi != NULL && zavi->fd >= 0 && !zavi->was_file_error && (size == 0 || data != NULL)) {
    uint8_t chunk[8];
    uint8_t *index;
    uint32_t pos, writesize, d;
    chunk[0] = tag[0];
    chunk[1] = tag[1];
    chunk[2] = tag[2];
    chunk[3] = tag[3];
    d = HTOLE32(size);
    memcpy(&chunk[4], &d, 4);
    // write the actual data
    if (write(zavi->fd, chunk, 8) != 8) goto error;
    writesize = (size+1)&~1;
    if (size > 0 && write(zavi->fd, data, size) != size) goto error;
    if (writesize != size) {
      uint8_t b;
      if (writesize-size != 1) abort(); // just in case
      b = 0;
      if (write(zavi->fd, &b, 1) != 1) goto error;
    }
    pos = zavi->written+4;
    zavi->written += writesize+8;
    if (zavi->indexused+16 >= zavi->indexsize) {
      uint8_t *ni = realloc(zavi->index, zavi->indexsize+16*4096);
      if (ni == NULL) goto error;
      zavi->index = ni;
      zavi->indexsize += 16*4096;
    }
    index = zavi->index+zavi->indexused;
    zavi->indexused += 16;
    index[0] = tag[0];
    index[1] = tag[1];
    index[2] = tag[2];
    index[3] = tag[3];
    d = HTOLE32(flags);
    memcpy(index+4, &d, 4);
    d = HTOLE32(pos);
    memcpy(index+8, &d, 4);
    d = HTOLE32(size);
    memcpy(index+12, &d, 4);
    return 0;
error:
    zavi->was_file_error = 1;
  }
  return -1;
}


int zmbv_avi_write_chunk_video (zmbv_avi_t zavi, const void *framedata, int size) {
  int res = -1;
  if (zavi != NULL && zavi->fd >= 0 && !zavi->was_file_error && size > 1 && framedata != NULL) {
    uint8_t b;
    memcpy(&b, framedata, 1);
    res = zmbv_avi_write_chunk(zavi, "00dc", size, framedata, (b&0x01 ? 0x10 : 0));
    if (res == 0) ++zavi->frames;
  }
  return res;
}


int zmbv_avi_write_chunk_audio (zmbv_avi_t zavi, const void *data, int size) {
  if (zavi != NULL && zavi->fd >= 0 && !zavi->was_file_error && (size == 0 || data != NULL)) {
    if (size < 0) return -1;
    if (size == 0) return 0;
    //int res = zmbv_avi_write_chunk(zavi, "01wb", zavi->audioused*4, zavi->audiobuf, 0);
    int res = zmbv_avi_write_chunk(zavi, "01wb", size, data, 0);
    //zavi->audiowritten = zavi->audioused*4;
    zavi->audiowritten = size;
    //zavi->audioused = 0;
    return res;
  }
  return -1;
}

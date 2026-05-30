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
#include "zmbv.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef ZMBV_USE_MINIZ
# include <zlib.h>
# define mz_deflateInit   deflateInit
# define mz_inflateInit   inflateInit
# define mz_deflateEnd    deflateEnd
# define mz_inflateEnd    inflateEnd
# define mz_deflateReset  deflateReset
# define mz_inflateReset  inflateReset
# define mz_deflate       deflate
# define mz_inflate       inflate
# define mz_stream        z_stream
# define MZ_OK            Z_OK
# define MZ_SYNC_FLUSH    Z_SYNC_FLUSH
#else
# ifdef MINIZ_NO_MALLOC
#  undef MINIZ_NO_MALLOC
# endif
# ifdef MINIZ_NO_ZLIB_APIS
#  undef MINIZ_NO_ZLIB_APIS
# endif
# include "miniz.c"
# define mz_inflateReset(_strm)  ({ int res = mz_inflateEnd(_strm); if (res == MZ_OK) res = mz_inflateInit(_strm); res; })
#endif

#ifdef __clang__
#define ATTR_PACKED __attribute__((packed))
#elif defined(__GNUC__)
#define ATTR_PACKED __attribute__((packed,gcc_struct))
#elif defined(_MSC_VER)
#define ATTR_PACKED
#else
#warn "make sure to define ATTR_PACKED for your compiler"
#define ATTR_PACKED
#endif

/******************************************************************************/
#define DBZV_VERSION_HIGH  (0)
#define DBZV_VERSION_LOW   (1)

#define COMPRESSION_NONE  (0)
#define COMPRESSION_ZLIB  (1)

#define MAX_VECTOR  (16)

enum {
  FRAME_MASK_KEYFRAME = 0x01,
  FRAME_MASK_DELTA_PALETTE = 0x02
};


/******************************************************************************/
zmbv_format_t zmbv_bpp_to_format (int bpp) {
  switch (bpp) {
    case 8: return ZMBV_FORMAT_8BPP;
    case 15: return ZMBV_FORMAT_15BPP;
    case 16: return ZMBV_FORMAT_16BPP;
    case 32: return ZMBV_FORMAT_32BPP;
  }
  return ZMBV_FORMAT_NONE;
}


int zmbv_work_buffer_size (int width, int height, zmbv_format_t fmt) {
  if (width > 0 && height > 0 && width <= 16384 && height <= 16384) {
    int f;
    switch (fmt) {
      case ZMBV_FORMAT_8BPP: f = 1; break;
      case ZMBV_FORMAT_15BPP: case ZMBV_FORMAT_16BPP: f = 2; break;
      case ZMBV_FORMAT_32BPP: f = 4; break;
      default: return -1;
    }
    return f*width*height+2*(1+(width/8))*(1+(height/8))+1024;
  }
  return -1;
}


/******************************************************************************/
typedef struct {
  int start;
  int dx, dy;
} zmbv_frame_block_t;


typedef struct {
  int x, y;
  int slot;
} zmbv_codec_vector_t;

#ifdef _MSC_VER
#pragma pack(push, 1)
#endif

typedef struct ATTR_PACKED {
  uint8_t high_version;
  uint8_t low_version;
  uint8_t compression;
  uint8_t format;
  uint8_t blockwidth;
  uint8_t blockheight;
} zmbv_keyframe_header_t;

#ifdef _MSC_VER
#pragma pack(pop)
#endif

typedef struct {
  int lines_done;
  int outbuf_size;
  int write_done;
  uint8_t *outbuf;
} zmbv_compress_t;


typedef enum {
  ZMBV_MODE_UNKNOWN,
  ZMBV_MODE_ENCODER,
  ZMBV_MODE_DECODER
} zmbv_codec_mode_t;


struct zmbv_codec_s {
  zmvb_init_flags_t init_flags;
  int complevel;
  zmbv_codec_mode_t mode;
  int unpack_compression;

  zmbv_compress_t compress;

  zmbv_codec_vector_t vector_table[512];
  int vector_count;

  uint8_t *oldframe, *newframe;
  uint8_t *buf1, *buf2, *work;
  int bufsize;

  int blockcount;
  zmbv_frame_block_t *blocks;

  int workUsed, workPos;

  int palsize;
  uint8_t palette[256*3];

  int height, width, pitch;

  zmbv_format_t format;
  int pixelsize;

  mz_stream zstream;
  int zstream_inited; // <0: deflate; >0: inflate; 0: not inited
};


/******************************************************************************/
/* generate functions from templates */
/* encoder templates */
#define ZMBV_POSSIBLE_BLOCK_TPL(_pxtype,_pxsize) \
static inline int zmbv_possible_block_##_pxsize (zmbv_codec_t zc, int vx, int vy, zmbv_frame_block_t *block) { \
  int ret = 0; \
  _pxtype *pold = ((_pxtype *)zc->oldframe)+block->start+(vy*zc->pitch)+vx; \
  _pxtype *pnew = ((_pxtype *)zc->newframe)+block->start; \
  for (int y = 0; y < block->dy; y += 4) { \
    for (int x = 0; x < block->dx; x += 4) { \
      int test = 0-((pold[x]-pnew[x])&0x00ffffff); \
      ret -= (test>>31); \
    } \
    pold += zc->pitch*4; \
    pnew += zc->pitch*4; \
  } \
  return ret; \
}


#define ZMBV_COMPARE_BLOCK_TPL(_pxtype,_pxsize) \
static inline int zmbv_compare_block_##_pxsize (zmbv_codec_t zc, int vx, int vy, zmbv_frame_block_t *block) { \
  int ret = 0; \
  _pxtype *pold = ((_pxtype *)zc->oldframe)+block->start+(vy*zc->pitch)+vx; \
  _pxtype *pnew = ((_pxtype *)zc->newframe)+block->start; \
  for (int y = 0; y < block->dy; ++y) { \
    for (int x = 0; x < block->dx; ++x) { \
      int test = 0-((pold[x]-pnew[x])&0x00ffffff); \
      ret -= (test>>31); \
    } \
    pold += zc->pitch; \
    pnew += zc->pitch; \
  } \
  return ret; \
}


#define ZMBV_ADD_XOR_BLOCK_TPL(_pxtype,_pxsize) \
static inline void zmbv_add_xor_block_##_pxsize (zmbv_codec_t zc, int vx, int vy, zmbv_frame_block_t *block) { \
  _pxtype *pold = ((_pxtype *)zc->oldframe)+block->start+(vy*zc->pitch)+vx; \
  _pxtype *pnew = ((_pxtype *)zc->newframe)+block->start; \
  for (int y = 0; y < block->dy; ++y) { \
    for (int x = 0; x < block->dx; ++x) { \
      *((_pxtype *)&zc->work[zc->workUsed]) = pnew[x]^pold[x]; \
      zc->workUsed += sizeof(_pxtype); \
    } \
    pold += zc->pitch; \
    pnew += zc->pitch; \
  } \
}


#define ZMBV_ADD_XOR_FRAME_TPL(_pxtype,_pxsize) \
static inline void zmbv_add_xor_frame_##_pxsize (zmbv_codec_t zc) { \
  int8_t *vectors = (int8_t *)&zc->work[zc->workUsed]; \
  /* align the following xor data on 4 byte boundary */ \
  zc->workUsed = (zc->workUsed+zc->blockcount*2+3)&~3; \
  for (int b = 0; b < zc->blockcount; ++b) { \
    zmbv_frame_block_t *block = &zc->blocks[b]; \
    int bestvx = 0; \
    int bestvy = 0; \
    int bestchange = zmbv_compare_block_##_pxsize(zc, 0, 0, block); \
    int possibles = 64; \
    for (int v = 0; v < zc->vector_count && possibles; ++v) { \
      if (bestchange < 4) break; \
      int vx = zc->vector_table[v].x; \
      int vy = zc->vector_table[v].y; \
      if (zmbv_possible_block_##_pxsize(zc, vx, vy, block) < 4) { \
        --possibles; \
        if (possibles < 0) abort(); \
        int testchange = zmbv_compare_block_##_pxsize(zc, vx, vy, block); \
        if (testchange < bestchange) { \
          bestchange = testchange; \
          bestvx = vx; \
          bestvy = vy; \
        } \
      } \
    } \
    vectors[b*2+0] = (bestvx << 1); \
    vectors[b*2+1] = (bestvy << 1); \
    if (bestchange) { \
      vectors[b*2+0] |= 1; \
      zmbv_add_xor_block_##_pxsize(zc, bestvx, bestvy, block); \
    } \
  } \
}

/* generate functions */
ZMBV_POSSIBLE_BLOCK_TPL(uint8_t,  8)
ZMBV_POSSIBLE_BLOCK_TPL(uint16_t,16)
ZMBV_POSSIBLE_BLOCK_TPL(uint32_t,32)

ZMBV_COMPARE_BLOCK_TPL(uint8_t,  8)
ZMBV_COMPARE_BLOCK_TPL(uint16_t,16)
ZMBV_COMPARE_BLOCK_TPL(uint32_t,32)

ZMBV_ADD_XOR_BLOCK_TPL(uint8_t,  8)
ZMBV_ADD_XOR_BLOCK_TPL(uint16_t,16)
ZMBV_ADD_XOR_BLOCK_TPL(uint32_t,32)

ZMBV_ADD_XOR_FRAME_TPL(uint8_t,  8)
ZMBV_ADD_XOR_FRAME_TPL(uint16_t,16)
ZMBV_ADD_XOR_FRAME_TPL(uint32_t,32)


/* decoder templates */
#ifdef ZMBV_INCLUDE_DECODER

#define ZMBV_UNXOR_BLOCK_TPL(_pxtype,_pxsize) \
static inline void zmbv_unxor_block_##_pxsize (zmbv_codec_t zc, int vx, int vy, zmbv_frame_block_t *block) { \
  _pxtype *pold = ((_pxtype *)zc->oldframe)+block->start+(vy*zc->pitch)+vx; \
  _pxtype *pnew = ((_pxtype *)zc->newframe)+block->start; \
  for (int y = 0; y < block->dy; ++y) { \
    for (int x = 0; x < block->dx; ++x) { \
      pnew[x] = pold[x]^*((_pxtype *)&zc->work[zc->workPos]); \
      zc->workPos += sizeof(_pxtype); \
    } \
    pold += zc->pitch; \
    pnew += zc->pitch; \
  } \
}


#define ZMBV_COPY_BLOCK_TPL(_pxtype,_pxsize) \
static inline void zmbv_copy_block_##_pxsize (zmbv_codec_t zc, int vx, int vy, zmbv_frame_block_t *block) { \
  _pxtype *pold = ((_pxtype *)zc->oldframe)+block->start+(vy*zc->pitch)+vx; \
  _pxtype *pnew = ((_pxtype *)zc->newframe)+block->start; \
  for (int y = 0; y < block->dy; ++y) { \
    for (int x = 0; x < block->dx; ++x) { \
      pnew[x] = pold[x]; \
    } \
    pold += zc->pitch; \
    pnew += zc->pitch; \
  } \
}


#define ZMBV_UNXOR_FRAME_TPL(_pxtype,_pxsize) \
static inline void zmbv_unxor_frame_##_pxsize (zmbv_codec_t zc) { \
  int8_t *vectors = (int8_t *)&zc->work[zc->workPos]; \
  zc->workPos = (zc->workPos+zc->blockcount*2+3)&~3; \
  for (int b = 0; b < zc->blockcount; ++b) { \
    zmbv_frame_block_t *block = &zc->blocks[b]; \
    int delta = vectors[b*2+0]&1; \
    int vx = vectors[b*2+0]>>1; \
    int vy = vectors[b*2+1]>>1; \
    if (delta) zmbv_unxor_block_##_pxsize(zc, vx, vy, block); else zmbv_copy_block_##_pxsize(zc, vx, vy, block); \
  } \
}

/* generate functions */
ZMBV_UNXOR_BLOCK_TPL(uint8_t,  8)
ZMBV_UNXOR_BLOCK_TPL(uint16_t,16)
ZMBV_UNXOR_BLOCK_TPL(uint32_t,32)

ZMBV_COPY_BLOCK_TPL(uint8_t,  8)
ZMBV_COPY_BLOCK_TPL(uint16_t,16)
ZMBV_COPY_BLOCK_TPL(uint32_t,32)

ZMBV_UNXOR_FRAME_TPL(uint8_t,  8)
ZMBV_UNXOR_FRAME_TPL(uint16_t,16)
ZMBV_UNXOR_FRAME_TPL(uint32_t,32)

#endif  /* ZMBV_INCLUDE_DECODER */

/******************************************************************************/
static void zmbv_create_vector_table (zmbv_codec_t zc) {
  if (zc != NULL) {
    zc->vector_table[0].x = zc->vector_table[0].y = 0;
    zc->vector_count = 1;
    for (int s = 1; s <= 10; ++s) {
      for (int y = 0-s; y <= 0+s; ++y) {
        for (int x = 0-s; x <= 0+s; ++x) {
          if (abs(x) == s || abs(y) == s) {
            zc->vector_table[zc->vector_count].x = x;
            zc->vector_table[zc->vector_count].y = y;
            ++zc->vector_count;
          }
        }
      }
    }
  }
}


zmbv_codec_t zmbv_codec_new (zmvb_init_flags_t flags, int complevel) {
  zmbv_codec_t zc = malloc(sizeof(*zc));
  if (zc != NULL) {
    /*
    zc->blocks = NULL;
    zc->buf1 = NULL;
    zc->buf2 = NULL;
    zc->work = NULL;
    memset(&zc->zstream, 0, sizeof(zc->zstream));
    zc->zstream_inited = 0;
    */
    memset(zc, 0, sizeof(*zc));
    zc->init_flags = flags;
    if (complevel < 0) complevel = 4;
    else if (complevel > 9) complevel = 9;
    zc->complevel = complevel;
    zmbv_create_vector_table(zc);
    zc->mode = ZMBV_MODE_UNKNOWN;
  }
  return zc;
}


static void zmbv_free_buffers (zmbv_codec_t zc) {
  if (zc != NULL) {
    if (zc->blocks != NULL) free(zc->blocks);
    if (zc->buf1 != NULL) free(zc->buf1);
    if (zc->buf2 != NULL) free(zc->buf2);
    if (zc->work != NULL) free(zc->work);
    zc->blocks = NULL;
    zc->buf1 = NULL;
    zc->buf2 = NULL;
    zc->work = NULL;
  }
}


static void zmbv_zlib_deinit (zmbv_codec_t zc) {
  if (zc != NULL) {
    switch (zc->zstream_inited) {
      case -1: mz_deflateEnd(&zc->zstream); break;
      case 1: mz_inflateEnd(&zc->zstream); break;
    }
    zc->zstream_inited = 0;
  }
}


void zmbv_codec_free (zmbv_codec_t zc) {
  if (zc != NULL) {
    zmbv_zlib_deinit(zc);
    zmbv_free_buffers(zc);
    free(zc);
  }
}


/******************************************************************************/
int zmbv_get_width (zmbv_codec_t zc) {
  return (zc != NULL ? zc->width : -1);
}


int zmbv_get_height (zmbv_codec_t zc) {
  return (zc != NULL ? zc->height : -1);
}


/******************************************************************************/
static int zmbv_setup_buffers (zmbv_codec_t zc, zmbv_format_t format, int blockwidth, int blockheight) {
  if (zc != NULL) {
    int xblocks, xleft, yblocks, yleft, i;

    zmbv_free_buffers(zc);
    zc->palsize = 0;
    switch (format) {
      case ZMBV_FORMAT_8BPP: zc->pixelsize = 1; zc->palsize = 256; break;
      case ZMBV_FORMAT_15BPP: case ZMBV_FORMAT_16BPP: zc->pixelsize = 2; break;
      case ZMBV_FORMAT_32BPP: zc->pixelsize = 4; break;
      default: return -1;
    };
    zc->bufsize = (zc->height+2*MAX_VECTOR)*zc->pitch*zc->pixelsize+2048;

    zc->buf1 = malloc(zc->bufsize);
    zc->buf2 = malloc(zc->bufsize);
    zc->work = malloc(zc->bufsize);

    if (zc->buf1 == NULL || zc->buf2 == NULL || zc->work == NULL) { zmbv_free_buffers(zc); return -1; }

    xblocks = (zc->width/blockwidth);
    xleft = zc->width%blockwidth;
    if (xleft) ++xblocks;
    yblocks = (zc->height/blockheight);
    yleft = zc->height%blockheight;
    if (yleft) ++yblocks;

    zc->blockcount = yblocks*xblocks;
    zc->blocks = malloc(sizeof(zmbv_frame_block_t)*zc->blockcount);
    if (zc->blocks == NULL) { zmbv_free_buffers(zc); return -1; }

    i = 0;
    for (int y = 0; y < yblocks; ++y) {
      for (int x = 0; x < xblocks; ++x) {
        zc->blocks[i].start = ((y*blockheight)+MAX_VECTOR)*zc->pitch+(x*blockwidth)+MAX_VECTOR;
        zc->blocks[i].dx = (xleft && x == xblocks-1 ? xleft : blockwidth);
        zc->blocks[i].dy = (yleft && y == yblocks-1 ? yleft : blockheight);
        ++i;
      }
    }

    memset(zc->buf1, 0, zc->bufsize);
    memset(zc->buf2, 0, zc->bufsize);
    memset(zc->work, 0, zc->bufsize);
    zc->oldframe = zc->buf1;
    zc->newframe = zc->buf2;
    zc->format = format;
    return 0;
  }
  return -1;
}


/******************************************************************************/
int zmbv_encode_setup (zmbv_codec_t zc, int width, int height) {
  if (zc != NULL && width > 0 && height > 0 && width <= 16384 && height <= 16384) {
    zc->width = width;
    zc->height = height;
    zc->pitch = width+2*MAX_VECTOR;
    zc->format = ZMBV_FORMAT_NONE;
    zmbv_zlib_deinit(zc);
    if ((zc->init_flags&ZMBV_INIT_FLAG_NOZLIB) == 0) {
      if (mz_deflateInit(&zc->zstream, zc->complevel) != MZ_OK) return -1;
      zc->zstream_inited = -1;
    }
    zc->mode = ZMBV_MODE_ENCODER;
    return 0;
  }
  return -1;
}


/******************************************************************************/
int zmbv_encode_prepare_frame (zmbv_codec_t zc, zmvb_prepare_flags_t flags, zmbv_format_t fmt, const void *pal, void *outbuf, int outbuf_size) {
  uint8_t *firstByte;
  const uint8_t *plt = (const uint8_t *)pal;

  if (zc == NULL) return -1;
  if (zc->mode != ZMBV_MODE_ENCODER) return -1;

  /* check for valid formats */
  switch (fmt) {
    case ZMBV_FORMAT_8BPP:
    case ZMBV_FORMAT_15BPP:
    case ZMBV_FORMAT_16BPP:
    case ZMBV_FORMAT_32BPP:
      break;
    default: return -1;
  }

  if (fmt != zc->format) {
    if (zmbv_setup_buffers(zc, fmt, 16, 16) < 0) return -1;
    flags |= ZMBV_PREP_FLAG_KEYFRAME; /* force a keyframe */
  }

  /* replace oldframe with new frame */
  {
    uint8_t *copyFrame = zc->newframe;
    zc->newframe = zc->oldframe;
    zc->oldframe = copyFrame;
  }

  zc->compress.lines_done = 0;
  zc->compress.outbuf_size = outbuf_size;
  zc->compress.write_done = 1;
  zc->compress.outbuf = (uint8_t *)outbuf;
  /* set a pointer to the first byte which will contain info about this frame */
  firstByte = zc->compress.outbuf;
  *firstByte = 0; /* frame flags */
  /* reset the work buffer */
  zc->workUsed = 0;
  zc->workPos = 0;
  if (flags&ZMBV_PREP_FLAG_KEYFRAME) {
    /* make a keyframe */
    *firstByte |= FRAME_MASK_KEYFRAME;
    zmbv_keyframe_header_t *header = (zmbv_keyframe_header_t *)(zc->compress.outbuf+zc->compress.write_done);
    header->high_version = DBZV_VERSION_HIGH;
    header->low_version = DBZV_VERSION_LOW;
    header->compression = ((zc->init_flags&ZMBV_INIT_FLAG_NOZLIB) == 0 ? COMPRESSION_ZLIB : COMPRESSION_NONE);
    header->format = zc->format;
    header->blockwidth = 16;
    header->blockheight = 16;
    zc->compress.write_done += sizeof(zmbv_keyframe_header_t);
    /* copy the new frame directly over */
    if (zc->palsize) {
      if (plt != NULL) {
        memcpy(&zc->palette, plt, sizeof(zc->palette));
      } else {
        memset(&zc->palette, 0, sizeof(zc->palette));
      }
      /* keyframes get the full palette */
      for (int i = 0; i < zc->palsize; ++i) {
        zc->work[zc->workUsed++] = zc->palette[i*3+0];
        zc->work[zc->workUsed++] = zc->palette[i*3+1];
        zc->work[zc->workUsed++] = zc->palette[i*3+2];
      }
    }
    /* restart deflate */
    if ((zc->init_flags&ZMBV_INIT_FLAG_NOZLIB) == 0) {
      if (mz_deflateReset(&zc->zstream) != MZ_OK) return -1;
    }
  } else {
    if (zc->palsize && plt != NULL && memcmp(plt, zc->palette, zc->palsize*3) != 0) {
      *firstByte |= FRAME_MASK_DELTA_PALETTE;
      for (int i = 0; i < zc->palsize; ++i) {
        zc->work[zc->workUsed++] = zc->palette[i*3+0]^plt[i*3+0];
        zc->work[zc->workUsed++] = zc->palette[i*3+1]^plt[i*3+1];
        zc->work[zc->workUsed++] = zc->palette[i*3+2]^plt[i*3+2];
      }
      memcpy(&zc->palette, plt, zc->palsize*3);
    }
  }
  return 0;
}


/******************************************************************************/
int zmbv_encode_lines (zmbv_codec_t zc, int line_count, const void *const line_ptrs[]) {
  if (zc != NULL && zc->mode == ZMBV_MODE_ENCODER) {
    int line_pitch = zc->pitch*zc->pixelsize;
    int line_width = zc->width*zc->pixelsize;
    uint8_t *destStart = zc->newframe+zc->pixelsize*(MAX_VECTOR+(zc->compress.lines_done+MAX_VECTOR)*zc->pitch);
    int i = 0;
    if (line_count > 0 && line_ptrs == NULL) return -1;
    while (i < line_count && zc->compress.lines_done < zc->height) {
      if (line_ptrs[i] == NULL) return -1;
      memcpy(destStart, line_ptrs[i], line_width);
      destStart += line_pitch;
      ++i;
      ++zc->compress.lines_done;
    }
    return 0;
  }
  return -1;
}


/******************************************************************************/
int zmvb_encode_finish_frame (zmbv_codec_t zc) {
  if (zc != NULL && zc->mode == ZMBV_MODE_ENCODER) {
    uint8_t firstByte = *zc->compress.outbuf;
    if (firstByte&FRAME_MASK_KEYFRAME) {
      int i;
      /* add the full frame data */
      const uint8_t *readFrame = zc->newframe+zc->pixelsize*(MAX_VECTOR+MAX_VECTOR*zc->pitch);
      for (i = 0; i < zc->height; ++i) {
        memcpy(&zc->work[zc->workUsed], readFrame, zc->width*zc->pixelsize);
        readFrame += zc->pitch*zc->pixelsize;
        zc->workUsed += zc->width*zc->pixelsize;
      }
    } else {
      /* add the delta frame data */
      switch (zc->format) {
        case ZMBV_FORMAT_8BPP: zmbv_add_xor_frame_8(zc); break;
        case ZMBV_FORMAT_15BPP: case ZMBV_FORMAT_16BPP: zmbv_add_xor_frame_16(zc); break;
        case ZMBV_FORMAT_32BPP: zmbv_add_xor_frame_32(zc); break;
        default: return -1; /* the thing that should not be */
      }
    }
    if ((zc->init_flags&ZMBV_INIT_FLAG_NOZLIB) == 0) {
      /* create the actual frame with compression */
      zc->zstream.next_in = (void *)zc->work;
      zc->zstream.avail_in = zc->workUsed;
      zc->zstream.total_in = 0;
      zc->zstream.next_out = (void *)(zc->compress.outbuf+zc->compress.write_done);
      zc->zstream.avail_out = zc->compress.outbuf_size-zc->compress.write_done;
      zc->zstream.total_out = 0;
      if (mz_deflate(&zc->zstream, MZ_SYNC_FLUSH) != MZ_OK) return -1; /* the thing that should not be */
      return (int)(zc->compress.write_done+zc->zstream.total_out);
    } else {
      memcpy(zc->compress.outbuf+zc->compress.write_done, zc->work, zc->workUsed);
      return zc->workUsed+zc->compress.write_done;
    }
  }
  return -1;
}


#ifdef ZMBV_INCLUDE_DECODER
/******************************************************************************/
int zmbv_decode_setup (zmbv_codec_t zc, int width, int height) {
  if (zc != NULL && width > 0 && height > 0 && width <= 16384 && height <= 16384) {
    zc->width = width;
    zc->height = height;
    zc->pitch = width+2*MAX_VECTOR;
    zc->format = ZMBV_FORMAT_NONE;
    zmbv_zlib_deinit(zc);
    if (mz_inflateInit(&zc->zstream) != MZ_OK) return -1;
    zc->zstream_inited = 1;
    zc->mode = ZMBV_MODE_DECODER;
    zc->unpack_compression = 0;
    return 0;
  }
  return -1;
}


/******************************************************************************/
const uint8_t *zmbv_get_palette (zmbv_codec_t zc) {
  return (zc != NULL ? zc->palette : NULL);
}


const void *zmbv_get_decoded_line (zmbv_codec_t zc, int idx) {
  if (zc != NULL && zc->mode == ZMBV_MODE_DECODER && idx >= 0 && idx < zc->height) {
    return zc->newframe+zc->pixelsize*(MAX_VECTOR+MAX_VECTOR*zc->pitch)+zc->pitch*zc->pixelsize*idx;
  }
  return NULL;
}


zmbv_format_t zmbv_get_decoded_format (zmbv_codec_t zc) {
  return (zc != NULL ? zc->format : ZMBV_FORMAT_NONE);
}

int zmbv_decode_palette_changed (zmbv_codec_t zc, const void *framedata, int size) {
  return (zc != NULL && framedata != NULL && size > 0 ? (((const uint8_t *)framedata)[0]&FRAME_MASK_DELTA_PALETTE) != 0 : 0);
}

/******************************************************************************/
int zmbv_decode_frame (zmbv_codec_t zc, const void *framedata, int size) {
  if (zc != NULL && framedata != NULL && size > 1 && zc->mode == ZMBV_MODE_DECODER) {
    uint8_t tag;
    const uint8_t *data = (const uint8_t *)framedata;
    tag = *data++;
    if (tag > 2) return -1; /* for now we can have only 0, 1 or 2 in tag byte */
    if (--size <= 0) return -1;
    if (tag&FRAME_MASK_KEYFRAME) {
      const zmbv_keyframe_header_t *header = (const zmbv_keyframe_header_t *)data;
      size -= sizeof(zmbv_keyframe_header_t);
      data += sizeof(zmbv_keyframe_header_t);
      if (size <= 0) return -1;
      if (header->low_version != DBZV_VERSION_LOW || header->high_version != DBZV_VERSION_HIGH) return -1;
      if (header->compression > COMPRESSION_ZLIB) return -1; /* invalid compression mode */
      if (zc->format != (zmbv_format_t)header->format && zmbv_setup_buffers(zc, (zmbv_format_t)header->format, header->blockwidth, header->blockheight) < 0) return -1;
      zc->unpack_compression = header->compression;
      if (zc->unpack_compression == COMPRESSION_ZLIB) {
        if (mz_inflateReset(&zc->zstream) != MZ_OK) return -1;
      }
    }
    if (size > zc->bufsize) return -1; /* frame too big */
    if (zc->unpack_compression == COMPRESSION_ZLIB) {
      zc->zstream.next_in = (void *)data;
      zc->zstream.avail_in = size;
      zc->zstream.total_in = 0;
      zc->zstream.next_out = (void *)zc->work;
      zc->zstream.avail_out = zc->bufsize;
      zc->zstream.total_out = 0;
      if (mz_inflate(&zc->zstream, MZ_SYNC_FLUSH/*MZ_NO_FLUSH*/) != MZ_OK) return -1; /* the thing that should not be */
      zc->workUsed = zc->zstream.total_out;
    } else {
      if (size > 0) memcpy(zc->work, data, size);
      zc->workUsed = size;
    }
    zc->workPos = 0;
    if (tag&FRAME_MASK_KEYFRAME) {
      if (zc->palsize) {
        for (int i = 0; i < zc->palsize; ++i) {
          zc->palette[i*3+0] = zc->work[zc->workPos++];
          zc->palette[i*3+1] = zc->work[zc->workPos++];
          zc->palette[i*3+2] = zc->work[zc->workPos++];
        }
      }
      zc->newframe = zc->buf1;
      zc->oldframe = zc->buf2;
      uint8_t *writeframe = zc->newframe+zc->pixelsize*(MAX_VECTOR+MAX_VECTOR*zc->pitch);
      for (int i = 0; i < zc->height; ++i) {
        memcpy(writeframe, &zc->work[zc->workPos], zc->width*zc->pixelsize);
        writeframe += zc->pitch*zc->pixelsize;
        zc->workPos += zc->width*zc->pixelsize;
      }
    } else {
      uint8_t *tmp = zc->oldframe;
      zc->oldframe = zc->newframe;
      zc->newframe = tmp;
      if (tag&FRAME_MASK_DELTA_PALETTE) {
        for (int i = 0; i < zc->palsize; ++i) {
          zc->palette[i*3+0] ^= zc->work[zc->workPos++];
          zc->palette[i*3+1] ^= zc->work[zc->workPos++];
          zc->palette[i*3+2] ^= zc->work[zc->workPos++];
        }
      }
      switch (zc->format) {
        case ZMBV_FORMAT_8BPP: zmbv_unxor_frame_8(zc); break;
        case ZMBV_FORMAT_15BPP: case ZMBV_FORMAT_16BPP: zmbv_unxor_frame_16(zc); break;
        case ZMBV_FORMAT_32BPP: zmbv_unxor_frame_32(zc); break;
        default: return -1; /* the thing that should not be */
      }
    }
    return 0;
  }
  return -1;
}
#endif /* ZMBV_INCLUDE_DECODER */

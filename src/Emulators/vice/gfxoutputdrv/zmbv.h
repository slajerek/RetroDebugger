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
#ifndef ZMBVC_H
#define ZMBVC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

typedef enum {
  ZMBV_FORMAT_NONE  = 0x00,
  /*ZMBV_FORMAT_1BPP  = 0x01,*/
  /*ZMBV_FORMAT_2BPP  = 0x02,*/
  /*ZMBV_FORMAT_4BPP  = 0x03,*/
  ZMBV_FORMAT_8BPP  = 0x04,
  ZMBV_FORMAT_15BPP = 0x05,
  ZMBV_FORMAT_16BPP = 0x06,
  /*ZMBV_FORMAT_24BPP = 0x07,*/
  ZMBV_FORMAT_32BPP = 0x08
} zmbv_format_t;


/* opaque codec data */
typedef struct zmbv_codec_s *zmbv_codec_t;


/* utilities */
/* returns ZMBV_FORMAT_NONE for unknown bpp */
extern zmbv_format_t zmbv_bpp_to_format (int bpp);
/* returns <0 on error; 0 on ok */
extern int zmbv_work_buffer_size (int width, int height, zmbv_format_t fmt);


typedef enum {
  ZMBV_INIT_FLAG_NONE = 0,
  ZMBV_INIT_FLAG_NOZLIB = 0x01 /* note that this will not 'turn off' zlib initializing */
} zmvb_init_flags_t;

/* complevel values */
enum {
  ZMBV_NO_COMPRESSION = 0,
  ZMBV_BEST_SPEED = 1,
  ZMBV_BEST_COMPRESSION = 9,
  ZMBV_DEFAULT_COMPRESSION = -1 /* level 4 */
};

extern zmbv_codec_t zmbv_codec_new (zmvb_init_flags_t flags, int complevel);
extern void zmbv_codec_free (zmbv_codec_t zc);


typedef enum {
  ZMBV_PREP_FLAG_NONE = 0,
  ZMBV_PREP_FLAG_KEYFRAME = 0x01
} zmvb_prepare_flags_t;

/* return <0 on error; 0 on ok */
extern int zmbv_encode_setup (zmbv_codec_t zc, int width, int height);
/* palette for ZMBV_FORMAT_8BPP: 256 8-bit (r,g,b) triplets */
/* return <0 on error; 0 on ok */
extern int zmbv_encode_prepare_frame (zmbv_codec_t zc, zmvb_prepare_flags_t flags, zmbv_format_t fmt, const void *pal, void *outbuf, int outbuf_size);
/* return <0 on error; 0 on ok */
extern int zmbv_encode_lines (zmbv_codec_t zc, int line_count, const void *const line_ptrs[]);
/* return <0 on error; 0 on ok */
static inline int zmbv_encode_line (zmbv_codec_t zc, const void *line_data) { return zmbv_encode_lines(zc, 1, &line_data); }
/* return # of bytes written in outbuf or <0 on error; NEVER returns 0 */
extern int zmvb_encode_finish_frame (zmbv_codec_t zc);


#ifdef ZMBV_INCLUDE_DECODER
/* return <0 on error; 0 on ok */
extern int zmbv_decode_setup (zmbv_codec_t zc, int width, int height);
/* return <0 on error; 0 on ok */
extern int zmbv_decode_frame (zmbv_codec_t zc, const void *framedata, int size);
/* return !0 if palette was be changed on this frame */
extern int zmbv_decode_is_palette_changed (zmbv_codec_t zc, const void *framedata, int size);

/* this can be called after zmbv_decode_frame() */
extern const uint8_t *zmbv_get_palette (zmbv_codec_t zc);
/* this can be called after zmbv_decode_frame() */
extern const void *zmbv_get_decoded_line (zmbv_codec_t zc, int idx) ;
/* this can be called after zmbv_decode_frame() */
extern zmbv_format_t zmbv_get_decoded_format (zmbv_codec_t zc);
#endif


/* <0: error; 0: never */
extern int zmbv_get_width (zmbv_codec_t zc);
extern int zmbv_get_height (zmbv_codec_t zc);


#ifdef __cplusplus
}
#endif
#endif

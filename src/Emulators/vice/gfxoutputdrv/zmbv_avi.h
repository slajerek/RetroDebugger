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
#ifndef ZMBVC_AVI_H
#define ZMBVC_AVI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>


typedef struct zmbv_avi_s *zmbv_avi_t;

extern zmbv_avi_t zmbv_avi_start (const char *fname, int width, int height, double fps, int audiorate);
extern int zmbv_avi_stop (zmbv_avi_t zavi);

extern int zmbv_avi_write_chunk (zmbv_avi_t zavi, const char tag[4], uint32_t size, const void *data, uint32_t flags);

extern int zmbv_avi_write_chunk_video (zmbv_avi_t zavi, const void *framedata, int size);
extern int zmbv_avi_write_chunk_audio (zmbv_avi_t zavi, const void *data, int size);


#ifdef __cplusplus
}
#endif
#endif

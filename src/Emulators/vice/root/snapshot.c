/*
 * snapshot.c - Implementation of machine snapshot files.
 *
 * Written by
 *  Ettore Perazzoli <ettore@comm2000.it>
 *  Marco van den Heuvel <blackystardust68@yahoo.com>
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

/*
 * Modified by Slajerek to use memory buffer to quick store in profiler's pool
 * and trace back emustate
 */

#include "vice.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "archdep.h"
#include "lib.h"
#include "ioutil.h"
#include "log.h"
#include "snapshot.h"
#ifdef USE_SVN_REVISION
#include "svnversion.h"
#endif
#include "translate.h"
#include "types.h"
#include "uiapi.h"
#include "version.h"
#include "vsync.h"
#include "zfile.h"

#define DEFAULT_DATA_CHUNK_SIZE	128*1024

static int snapshot_error = SNAPSHOT_NO_ERROR;
static char *current_module = NULL;
static char read_name[SNAPSHOT_MACHINE_NAME_LEN];
static char *current_machine_name = NULL;
static char *current_filename = NULL;

char snapshot_magic_string[] = "VICE Snapshot File\032";
char snapshot_version_magic_string[] = "VICE Version\032";

#define SNAPSHOT_MAGIC_LEN              19
#define SNAPSHOT_VERSION_MAGIC_LEN      13

/* ------------------------------------------------------------------------- */

static int snapshot_write_byte(snapshot_t *sn, BYTE data)
{
	//    if (fputc(data, sn->file) == EOF) {
	//        snapshot_error = SNAPSHOT_WRITE_EOF_ERROR;
	//        return -1;
	//    }

	if (sn->pos == sn->data_size -1)
	{
		sn->data_size = sn->data_size + DEFAULT_DATA_CHUNK_SIZE;
		sn->data = lib_realloc(sn->data, sn->data_size);
	}
	
	sn->data[sn->pos] = data;
	sn->pos++;
	
    return 0;
}

static int snapshot_write_word(snapshot_t *sn, WORD data)
{
    if (snapshot_write_byte(sn, (BYTE)(data & 0xff)) < 0
        || snapshot_write_byte(sn, (BYTE)(data >> 8)) < 0) {
        return -1;
    }

    return 0;
}

static int snapshot_write_dword(snapshot_t *sn, DWORD data)
{
    if (snapshot_write_word(sn, (WORD)(data & 0xffff)) < 0
        || snapshot_write_word(sn, (WORD)(data >> 16)) < 0) {
        return -1;
    }

    return 0;
}

static int snapshot_write_double(snapshot_t *sn, double data)
{
    BYTE *byte_data = (BYTE *)&data;
    int i;

    for (i = 0; i < sizeof(double); i++) {
        if (snapshot_write_byte(sn, byte_data[i]) < 0) {
            return -1;
        }
    }
    return 0;
}

static int snapshot_write_padded_string(snapshot_t *sn, const char *s, BYTE pad_char,
                                        int len)
{
    int i, found_zero;
    BYTE c;

    for (i = found_zero = 0; i < len; i++) {
        if (!found_zero && s[i] == 0) {
            found_zero = 1;
        }
        c = found_zero ? (BYTE)pad_char : (BYTE) s[i];
        if (snapshot_write_byte(sn, c) < 0) {
            return -1;
        }
    }

    return 0;
}

static int snapshot_write_byte_array(snapshot_t *sn, const BYTE *data, unsigned int num)
{
	unsigned int i;
	
	if (num > 0)
	{
		for (i = 0; i < num; i++)
		{
			if (snapshot_write_byte(sn, data[i]) < 0)
			{
				snapshot_error = SNAPSHOT_WRITE_BYTE_ARRAY_ERROR;
				return -1;
			}
		}
	}
	
//    if (num > 0 && fwrite(data, (size_t)num, 1, f) < 1) {
//        snapshot_error = SNAPSHOT_WRITE_BYTE_ARRAY_ERROR;
//        return -1;
//    }

    return 0;
}

static int snapshot_write_word_array(snapshot_t *sn, const WORD *data, unsigned int num)
{
    unsigned int i;

    for (i = 0; i < num; i++) {
        if (snapshot_write_word(sn, data[i]) < 0) {
            return -1;
        }
    }

    return 0;
}

static int snapshot_write_dword_array(snapshot_t *sn, const DWORD *data, unsigned int num)
{
    unsigned int i;

    for (i = 0; i < num; i++) {
        if (snapshot_write_dword(sn, data[i]) < 0) {
            return -1;
        }
    }

    return 0;
}


static int snapshot_write_string(snapshot_t *sn, const char *s)
{
    size_t len, i;

    len = s ? (strlen(s) + 1) : 0;      /* length includes nullbyte */

    if (snapshot_write_word(sn, (WORD)len) < 0) {
        return -1;
    }

    for (i = 0; i < len; i++) {
        if (snapshot_write_byte(sn, s[i]) < 0) {
            return -1;
        }
    }

    return (int)(len + sizeof(WORD));
}

static int snapshot_read_byte(snapshot_t *sn, BYTE *b_return)
{
    int c;

//    c = fgetc(sn->file);
//    if (c == EOF) {
//        snapshot_error = SNAPSHOT_READ_EOF_ERROR;
//        return -1;
//    }
	
	if (sn->pos == sn->data_size)
	{
		snapshot_error = SNAPSHOT_READ_EOF_ERROR;
		return -1;
	}
	
	c = sn->data[sn->pos];
	sn->pos++;
	
    *b_return = (BYTE)c;
    return 0;
}

static int snapshot_read_word(snapshot_t *sn, WORD *w_return)
{
    BYTE lo, hi;

    if (snapshot_read_byte(sn, &lo) < 0 || snapshot_read_byte(sn, &hi) < 0) {
        return -1;
    }

    *w_return = lo | (hi << 8);
    return 0;
}

static int snapshot_read_dword(snapshot_t *sn, DWORD *dw_return)
{
    WORD lo, hi;

    if (snapshot_read_word(sn, &lo) < 0 || snapshot_read_word(sn, &hi) < 0) {
        return -1;
    }

    *dw_return = lo | (hi << 16);
    return 0;
}

static int snapshot_read_double(snapshot_t *sn, double *d_return)
{
    int i;
    BYTE c;
    double val;
    BYTE *byte_val = (BYTE *)&val;

    for (i = 0; i < sizeof(double); i++) {
		if (snapshot_read_byte(sn, &c) < 0)
		{
			snapshot_error = SNAPSHOT_READ_EOF_ERROR;
			return -1;
		}
//        c = fgetc(f);
//        if (c == EOF) {
//            snapshot_error = SNAPSHOT_READ_EOF_ERROR;
//            return -1;
//        }
		
        byte_val[i] = (BYTE)c;
    }
    *d_return = val;
    return 0;
}

static int snapshot_read_byte_array(snapshot_t *sn, BYTE *b_return, unsigned int num)
{
	unsigned int i;
	BYTE b;
	
	if (num > 0)
	{
		for (i = 0; i < num; i++)
		{
			snapshot_read_byte(sn, &b);
			b_return[i] = b;
		}
	}
//    if (num > 0 && fread(b_return, (size_t)num, 1, f) < 1) {
//        snapshot_error = SNAPSHOT_READ_BYTE_ARRAY_ERROR;
//        return -1;
//    }

    return 0;
}

static int snapshot_read_word_array(snapshot_t *sn, WORD *w_return, unsigned int num)
{
    unsigned int i;

    for (i = 0; i < num; i++) {
        if (snapshot_read_word(sn, w_return + i) < 0) {
            return -1;
        }
    }

    return 0;
}

static int snapshot_read_dword_array(snapshot_t *sn, DWORD *dw_return, unsigned int num)
{
    unsigned int i;

    for (i = 0; i < num; i++) {
        if (snapshot_read_dword(sn, dw_return + i) < 0) {
            return -1;
        }
    }

    return 0;
}

static int snapshot_read_string(snapshot_t *sn, char **s)
{
    int i, len;
    WORD w;
    char *p = NULL;

    /* first free the previous string */
    lib_free(*s);
    *s = NULL;      /* don't leave a bogus pointer */

    if (snapshot_read_word(sn, &w) < 0) {
        return -1;
    }

    len = (int)w;

    if (len) {
        p = lib_malloc(len);
        *s = p;

        for (i = 0; i < len; i++) {
            if (snapshot_read_byte(sn, (BYTE *)(p + i)) < 0) {
                p[0] = 0;
                return -1;
            }
        }
        p[len - 1] = 0;   /* just to be save */
    }
    return 0;
}

/* ------------------------------------------------------------------------- */

int snapshot_module_write_byte(snapshot_module_t *m, BYTE b)
{
    if (snapshot_write_byte(m->sn, b) < 0) {
        return -1;
    }

    m->size++;
    return 0;
}

int snapshot_module_write_word(snapshot_module_t *m, WORD w)
{
    if (snapshot_write_word(m->sn, w) < 0) {
        return -1;
    }

    m->size += 2;
    return 0;
}

int snapshot_module_write_dword(snapshot_module_t *m, DWORD dw)
{
    if (snapshot_write_dword(m->sn, dw) < 0) {
        return -1;
    }

    m->size += 4;
    return 0;
}

int snapshot_module_write_double(snapshot_module_t *m, double db)
{
    if (snapshot_write_double(m->sn, db) < 0) {
        return -1;
    }

    m->size += 8;
    return 0;
}

int snapshot_module_write_padded_string(snapshot_module_t *m, const char *s, BYTE pad_char, int len)
{
    if (snapshot_write_padded_string(m->sn, s, (BYTE)pad_char, len) < 0) {
        return -1;
    }

    m->size += len;
    return 0;
}

int snapshot_module_write_byte_array(snapshot_module_t *m, const BYTE *b, unsigned int num)
{
    if (snapshot_write_byte_array(m->sn, b, num) < 0) {
        return -1;
    }

    m->size += num;
    return 0;
}

int snapshot_module_write_word_array(snapshot_module_t *m, const WORD *w, unsigned int num)
{
    if (snapshot_write_word_array(m->sn, w, num) < 0) {
        return -1;
    }

    m->size += num * sizeof(WORD);
    return 0;
}

int snapshot_module_write_dword_array(snapshot_module_t *m, const DWORD *dw, unsigned int num)
{
    if (snapshot_write_dword_array(m->sn, dw, num) < 0) {
        return -1;
    }

    m->size += num * sizeof(DWORD);
    return 0;
}

int snapshot_module_write_string(snapshot_module_t *m, const char *s)
{
    int len;
    len = snapshot_write_string(m->sn, s);
    if (len < 0) {
        snapshot_error = SNAPSHOT_ILLEGAL_STRING_LENGTH_ERROR;
        return -1;
    }

    m->size += len;
    return 0;
}

/* ------------------------------------------------------------------------- */

int snapshot_module_read_byte(snapshot_module_t *m, BYTE *b_return)
{
//    if (ftell(m->file) + sizeof(BYTE) > m->offset + m->size) {
//        snapshot_error = SNAPSHOT_READ_OUT_OF_BOUNDS_ERROR;
//        return -1;
//    }

	if (m->sn->pos + sizeof(BYTE) > m->offset + m->size) {
		snapshot_error = SNAPSHOT_READ_OUT_OF_BOUNDS_ERROR;
		return -1;
	}

	
    return snapshot_read_byte(m->sn, b_return);
}

int snapshot_module_read_word(snapshot_module_t *m, WORD *w_return)
{
//    if (ftell(m->file) + sizeof(WORD) > m->offset + m->size) {
//        snapshot_error = SNAPSHOT_READ_OUT_OF_BOUNDS_ERROR;
//        return -1;
//    }

	if (m->sn->pos + sizeof(WORD) > m->offset + m->size) {
		snapshot_error = SNAPSHOT_READ_OUT_OF_BOUNDS_ERROR;
		return -1;
	}

    return snapshot_read_word(m->sn, w_return);
}

int snapshot_module_read_dword(snapshot_module_t *m, DWORD *dw_return)
{
//    if (ftell(m->file) + sizeof(DWORD) > m->offset + m->size) {
//        snapshot_error = SNAPSHOT_READ_OUT_OF_BOUNDS_ERROR;
//        return -1;
//    }

	if (m->sn->pos + sizeof(DWORD) > m->offset + m->size) {
		snapshot_error = SNAPSHOT_READ_OUT_OF_BOUNDS_ERROR;
		return -1;
	}

    return snapshot_read_dword(m->sn, dw_return);
}

int snapshot_module_read_double(snapshot_module_t *m, double *db_return)
{
//    if (ftell(m->file) + sizeof(double) > m->offset + m->size) {
//        snapshot_error = SNAPSHOT_READ_OUT_OF_BOUNDS_ERROR;
//        return -1;
//    }

	if (m->sn->pos + sizeof(double) > m->offset + m->size) {
		snapshot_error = SNAPSHOT_READ_OUT_OF_BOUNDS_ERROR;
		return -1;
	}

    return snapshot_read_double(m->sn, db_return);
}

int snapshot_module_read_byte_array(snapshot_module_t *m, BYTE *b_return, unsigned int num)
{
//    if ((long)(ftell(m->file) + num) > (long)(m->offset + m->size)) {
//        snapshot_error = SNAPSHOT_READ_OUT_OF_BOUNDS_ERROR;
//        return -1;
//    }

	if ((long)(m->sn->pos + num) > (long)(m->offset + m->size)) {
		snapshot_error = SNAPSHOT_READ_OUT_OF_BOUNDS_ERROR;
		return -1;
	}

    return snapshot_read_byte_array(m->sn, b_return, num);
}

int snapshot_module_read_word_array(snapshot_module_t *m, WORD *w_return, unsigned int num)
{
//    if ((long)(ftell(m->file) + num * sizeof(WORD)) > (long)(m->offset + m->size)) {
//        snapshot_error = SNAPSHOT_READ_OUT_OF_BOUNDS_ERROR;
//        return -1;
//    }

	if ((long)(m->sn->pos + num * sizeof(WORD)) > (long)(m->offset + m->size)) {
		snapshot_error = SNAPSHOT_READ_OUT_OF_BOUNDS_ERROR;
		return -1;
	}

    return snapshot_read_word_array(m->sn, w_return, num);
}

int snapshot_module_read_dword_array(snapshot_module_t *m, DWORD *dw_return, unsigned int num)
{
//    if ((long)(ftell(m->file) + num * sizeof(DWORD)) > (long)(m->offset + m->size)) {
//        snapshot_error = SNAPSHOT_READ_OUT_OF_BOUNDS_ERROR;
//        return -1;
//    }

	if ((long)(m->sn->pos + num * sizeof(DWORD)) > (long)(m->offset + m->size)) {
		snapshot_error = SNAPSHOT_READ_OUT_OF_BOUNDS_ERROR;
		return -1;
	}

    return snapshot_read_dword_array(m->sn, dw_return, num);
}

int snapshot_module_read_string(snapshot_module_t *m, char **charp_return)
{
//    if (ftell(m->file) + sizeof(WORD) > m->offset + m->size) {
//        snapshot_error = SNAPSHOT_READ_OUT_OF_BOUNDS_ERROR;
//        return -1;
//    }

	if (m->sn->pos + sizeof(WORD) > m->offset + m->size) {
		snapshot_error = SNAPSHOT_READ_OUT_OF_BOUNDS_ERROR;
		return -1;
	}

    return snapshot_read_string(m->sn, charp_return);
}

int snapshot_module_read_byte_into_int(snapshot_module_t *m, int *value_return)
{
    BYTE b;

    if (snapshot_module_read_byte(m, &b) < 0) {
        return -1;
    }
    *value_return = (int)b;
    return 0;
}

int snapshot_module_read_word_into_int(snapshot_module_t *m, int *value_return)
{
    WORD b;

    if (snapshot_module_read_word(m, &b) < 0) {
        return -1;
    }
    *value_return = (int)b;
    return 0;
}

int snapshot_module_read_dword_into_ulong(snapshot_module_t *m, unsigned long *value_return)
{
    DWORD b;

    if (snapshot_module_read_dword(m, &b) < 0) {
        return -1;
    }
    *value_return = (unsigned long)b;
    return 0;
}

int snapshot_module_read_dword_into_int(snapshot_module_t *m, int *value_return)
{
    DWORD b;

    if (snapshot_module_read_dword(m, &b) < 0) {
        return -1;
    }
    *value_return = (int)b;
    return 0;
}

int snapshot_module_read_dword_into_uint(snapshot_module_t *m, unsigned int *value_return)
{
    DWORD b;

    if (snapshot_module_read_dword(m, &b) < 0) {
        return -1;
    }
    *value_return = (unsigned int)b;
    return 0;
}

/* ------------------------------------------------------------------------- */

snapshot_module_t *snapshot_module_create(snapshot_t *sn, const char *name, BYTE major_version, BYTE minor_version)
{
    snapshot_module_t *m;

    current_module = (char *)name;

    m = lib_malloc(sizeof(snapshot_module_t));
//    m->file = sn->file;
	m->sn = sn;
//    m->offset = ftell(s->file);
	m->offset = sn->pos;
    if (m->offset == -1) {
        snapshot_error = SNAPSHOT_ILLEGAL_OFFSET_ERROR;
        lib_free(m);
        return NULL;
    }
    m->write_mode = 1;

    if (snapshot_write_padded_string(sn, name, (BYTE)0, SNAPSHOT_MODULE_NAME_LEN) < 0
        || snapshot_write_byte(sn, major_version) < 0
        || snapshot_write_byte(sn, minor_version) < 0
        || snapshot_write_dword(sn, 0) < 0) {
        return NULL;
    }

//    m->size = ftell(s->file) - m->offset;
//    m->size_offset = ftell(s->file) - sizeof(DWORD);

	m->size = sn->pos - m->offset;
	m->size_offset = sn->pos - sizeof(DWORD);

    return m;
}

snapshot_module_t *snapshot_module_open(snapshot_t *sn, const char *name, BYTE *major_version_return, BYTE *minor_version_return)
{
    snapshot_module_t *m;
    char n[SNAPSHOT_MODULE_NAME_LEN];
    unsigned int name_len = (unsigned int)strlen(name);

    current_module = (char *)name;

//	if (fseek(s->file, s->first_module_offset, SEEK_SET) < 0) {
//		snapshot_error = SNAPSHOT_FIRST_MODULE_NOT_FOUND_ERROR;
//		return NULL;
//	}

	if (sn->first_module_offset >= sn->data_size)
	{
		snapshot_error = SNAPSHOT_FIRST_MODULE_NOT_FOUND_ERROR;
		return NULL;
	}
	sn->pos = sn->first_module_offset;
	
    m = lib_malloc(sizeof(snapshot_module_t));
//    m->file = sn->file;
	m->sn = sn;
    m->write_mode = 0;

    m->offset = sn->first_module_offset;

    /* Search for the module name.  This is quite inefficient, but I don't
       think we care.  */
    while (1) {
        if (snapshot_read_byte_array(sn, (BYTE *)n,
                                     SNAPSHOT_MODULE_NAME_LEN) < 0
            || snapshot_read_byte(sn, major_version_return) < 0
            || snapshot_read_byte(sn, minor_version_return) < 0
            || snapshot_read_dword(sn, &m->size)) {
            snapshot_error = SNAPSHOT_MODULE_HEADER_READ_ERROR;
            goto fail;
        }

        /* Found?  */
        if (memcmp(n, name, name_len) == 0
            && (name_len == SNAPSHOT_MODULE_NAME_LEN || n[name_len] == 0)) {
            break;
        }

        m->offset += m->size;
		
//		if (fseek(s->file, m->offset, SEEK_SET) < 0) {
//			snapshot_error = SNAPSHOT_MODULE_NOT_FOUND_ERROR;
//			goto fail;
//		}
		if (m->offset >= sn->data_size)
		{
			snapshot_error = SNAPSHOT_MODULE_NOT_FOUND_ERROR;
			goto fail;
		}
		sn->pos = m->offset;
    }

//    m->size_offset = ftell(s->file) - sizeof(DWORD);
	m->size_offset = sn->pos - sizeof(DWORD);

    return m;

fail:
//    fseek(s->file, s->first_module_offset, SEEK_SET);
	sn->pos = sn->first_module_offset;
    lib_free(m);
    return NULL;
}

int snapshot_module_close(snapshot_module_t *m)
{
    /* Backpatch module size if writing.  */
//	if (m->write_mode
//		&& (fseek(m->file, m->size_offset, SEEK_SET) < 0
//			|| snapshot_write_dword(m->sn, m->size) < 0)) {
//			snapshot_error = SNAPSHOT_MODULE_CLOSE_ERROR;
//			return -1;
//		}

	if (m->write_mode)
	{
		m->sn->pos = m->size_offset;
		if (snapshot_write_dword(m->sn, m->size) < 0)
		{
			snapshot_error = SNAPSHOT_MODULE_CLOSE_ERROR;
			return -1;
		}
	}
	
    /* Skip module.  */
//    if (fseek(m->file, m->offset + m->size, SEEK_SET) < 0) {
//        snapshot_error = SNAPSHOT_MODULE_SKIP_ERROR;
//        return -1;
//    }
	m->sn->pos = m->offset + m->size;

    lib_free(m);
    return 0;
}

/* ------------------------------------------------------------------------- */

snapshot_t *snapshot_create(const char *filename, BYTE major_version, BYTE minor_version, const char *snapshot_machine_name, int snapshot_size)
{
    snapshot_t *sn;
    unsigned char viceversion[4] = { VERSION_RC_NUMBER };

	LOGD("snapshot_create: filename='%s'", filename);

    current_filename = (char *)filename;

	sn = lib_malloc(sizeof(snapshot_t));
	
	// if snapshot size not provided then use default
	if (snapshot_size < 1)
	{
		snapshot_size = DEFAULT_DATA_CHUNK_SIZE;
	}
	
	sn->data = lib_malloc(snapshot_size);
	sn->data_size = snapshot_size;

	sn->pos = 0;
	
//	sn->file = f;

    /* Magic string.  */
    if (snapshot_write_padded_string(sn, snapshot_magic_string, (BYTE)0, SNAPSHOT_MAGIC_LEN) < 0) {
        snapshot_error = SNAPSHOT_CANNOT_WRITE_MAGIC_STRING_ERROR;
        goto fail;
    }

    /* Version number.  */
    if (snapshot_write_byte(sn, major_version) < 0
        || snapshot_write_byte(sn, minor_version) < 0) {
        snapshot_error = SNAPSHOT_CANNOT_WRITE_VERSION_ERROR;
        goto fail;
    }

    /* Machine.  */
    if (snapshot_write_padded_string(sn, snapshot_machine_name, (BYTE)0, SNAPSHOT_MACHINE_NAME_LEN) < 0) {
        snapshot_error = SNAPSHOT_CANNOT_WRITE_MACHINE_NAME_ERROR;
        goto fail;
    }

    /* VICE version and revision */
    if (snapshot_write_padded_string(sn, snapshot_version_magic_string, (BYTE)0, SNAPSHOT_VERSION_MAGIC_LEN) < 0) {
        snapshot_error = SNAPSHOT_CANNOT_WRITE_MAGIC_STRING_ERROR;
        goto fail;
    }

    if (snapshot_write_byte(sn, viceversion[0]) < 0
        || snapshot_write_byte(sn, viceversion[1]) < 0
        || snapshot_write_byte(sn, viceversion[2]) < 0
        || snapshot_write_byte(sn, viceversion[3]) < 0
#ifdef USE_SVN_REVISION
        || snapshot_write_dword(sn, VICE_SVN_REV_NUMBER) < 0) {
#else
        || snapshot_write_dword(sn, 0) < 0) {
#endif
        snapshot_error = SNAPSHOT_CANNOT_WRITE_VERSION_ERROR;
        goto fail;
    }

	//sn->first_module_offset = ftell(f);
    sn->first_module_offset = sn->pos;
    sn->write_mode = 1;

    return sn;

fail:
	lib_free(sn->data);
	lib_free(sn);
    return NULL;
}

/* informal only, used by the error message created below */
static unsigned char snapshot_viceversion[4];
static DWORD snapshot_vicerevision;

snapshot_t *snapshot_open(const char *filename, BYTE *major_version_return, BYTE *minor_version_return, const char *snapshot_machine_name)
{
    FILE *f;
    char magic[SNAPSHOT_MAGIC_LEN];
    snapshot_t *sn = NULL;
    int machine_name_len;
    size_t offs;

    current_machine_name = (char *)snapshot_machine_name;
    current_filename = (char *)filename;
    current_module = NULL;

    f = zfile_fopen(filename, MODE_READ);
    if (f == NULL) {
        snapshot_error = SNAPSHOT_CANNOT_OPEN_FOR_READ_ERROR;
        return NULL;
    }

	// ** Read contents **
	
	sn = lib_malloc(sizeof(snapshot_t));
//	sn->file = f;
	sn->write_mode = 0;
	
	fseek(f, 0, SEEK_END);
	sn->data_size = ftell(f);
	fseek(f, 0, SEEK_SET);

	sn->data = lib_malloc(sn->data_size+1);
	fread(sn->data, sn->data_size, 1, f);
	
	fclose(f);

	//
	
	sn->pos = 0;

    /* Magic string.  */
    if (snapshot_read_byte_array(sn, (BYTE *)magic, SNAPSHOT_MAGIC_LEN) < 0
        || memcmp(magic, snapshot_magic_string, SNAPSHOT_MAGIC_LEN) != 0) {
        snapshot_error = SNAPSHOT_MAGIC_STRING_MISMATCH_ERROR;
        goto fail;
    }

    /* Version number.  */
    if (snapshot_read_byte(sn, major_version_return) < 0
        || snapshot_read_byte(sn, minor_version_return) < 0) {
        snapshot_error = SNAPSHOT_CANNOT_READ_VERSION_ERROR;
        goto fail;
    }

    /* Machine.  */
    if (snapshot_read_byte_array(sn, (BYTE *)read_name, SNAPSHOT_MACHINE_NAME_LEN) < 0) {
        snapshot_error = SNAPSHOT_CANNOT_READ_MACHINE_NAME_ERROR;
        goto fail;
    }

    /* Check machine name.  */
    machine_name_len = (int)strlen(snapshot_machine_name);
    if (memcmp(read_name, snapshot_machine_name, machine_name_len) != 0
        || (machine_name_len != SNAPSHOT_MODULE_NAME_LEN
            && read_name[machine_name_len] != 0)) {
        snapshot_error = SNAPSHOT_MACHINE_MISMATCH_ERROR;
        goto fail;
    }

    /* VICE version and revision */
    memset(snapshot_viceversion, 0, 4);
    snapshot_vicerevision = 0;
//    offs = ftell(f);
	offs = sn->pos;

    if (snapshot_read_byte_array(sn, (BYTE *)magic, SNAPSHOT_VERSION_MAGIC_LEN) < 0
        || memcmp(magic, snapshot_version_magic_string, SNAPSHOT_VERSION_MAGIC_LEN) != 0) {
        /* old snapshots do not contain VICE version */
//        fseek(f, offs, SEEK_SET);
		sn->pos = offs;
        log_warning(LOG_DEFAULT, "attempting to load pre 2.4.30 snapshot");
    } else {
        /* actually read the version */
        if (snapshot_read_byte(sn, &snapshot_viceversion[0]) < 0
            || snapshot_read_byte(sn, &snapshot_viceversion[1]) < 0
            || snapshot_read_byte(sn, &snapshot_viceversion[2]) < 0
            || snapshot_read_byte(sn, &snapshot_viceversion[3]) < 0
            || snapshot_read_dword(sn, &snapshot_vicerevision) < 0) {
            snapshot_error = SNAPSHOT_CANNOT_READ_VERSION_ERROR;
            goto fail;
        }
    }

//    s->first_module_offset = ftell(f);
	sn->first_module_offset = sn->pos;

    vsync_suspend_speed_eval();
    return sn;

fail:
	lib_free(sn->data);
	lib_free(sn);
    return NULL;
}

// byte* array-only versions, storing in memory, not file

snapshot_t *snapshot_create_in_memory(BYTE major_version, BYTE minor_version, const char *snapshot_machine_name, int snapshot_size)
{
	snapshot_t *sn;
	unsigned char viceversion[4] = { VERSION_RC_NUMBER };
	
	current_filename = NULL;
	
	sn = lib_malloc(sizeof(snapshot_t));
	
	// if snapshot size not provided then use default
	if (snapshot_size < 1)
	{
		snapshot_size = DEFAULT_DATA_CHUNK_SIZE;
	}
	
	sn->data = lib_malloc(snapshot_size);
	sn->data_size = snapshot_size;
	sn->pos = 0;
	
	/* Magic string.  */
	if (snapshot_write_padded_string(sn, snapshot_magic_string, (BYTE)0, SNAPSHOT_MAGIC_LEN) < 0) {
		snapshot_error = SNAPSHOT_CANNOT_WRITE_MAGIC_STRING_ERROR;
		goto fail;
	}
	
	/* Version number.  */
	if (snapshot_write_byte(sn, major_version) < 0
		|| snapshot_write_byte(sn, minor_version) < 0) {
		snapshot_error = SNAPSHOT_CANNOT_WRITE_VERSION_ERROR;
		goto fail;
	}
	
	/* Machine.  */
	if (snapshot_write_padded_string(sn, snapshot_machine_name, (BYTE)0, SNAPSHOT_MACHINE_NAME_LEN) < 0) {
		snapshot_error = SNAPSHOT_CANNOT_WRITE_MACHINE_NAME_ERROR;
		goto fail;
	}
	
	/* VICE version and revision */
	if (snapshot_write_padded_string(sn, snapshot_version_magic_string, (BYTE)0, SNAPSHOT_VERSION_MAGIC_LEN) < 0) {
		snapshot_error = SNAPSHOT_CANNOT_WRITE_MAGIC_STRING_ERROR;
		goto fail;
	}
	
	if (snapshot_write_byte(sn, viceversion[0]) < 0
		|| snapshot_write_byte(sn, viceversion[1]) < 0
		|| snapshot_write_byte(sn, viceversion[2]) < 0
		|| snapshot_write_byte(sn, viceversion[3]) < 0
#ifdef USE_SVN_REVISION
		|| snapshot_write_dword(sn, VICE_SVN_REV_NUMBER) < 0) {
#else
		|| snapshot_write_dword(sn, 0) < 0) {
#endif
			snapshot_error = SNAPSHOT_CANNOT_WRITE_VERSION_ERROR;
			goto fail;
		}
		
	//sn->first_module_offset = ftell(f);
	sn->first_module_offset = sn->pos;
	sn->write_mode = 1;
	
	return sn;
	
fail:
	lib_free(sn->data);
	lib_free(sn);
	return NULL;
}
	
snapshot_t *snapshot_open_from_memory(BYTE *major_version_return, BYTE *minor_version_return, const char *snapshot_machine_name,
									  BYTE *snapshot_data, int snapshot_size)
{
	char magic[SNAPSHOT_MAGIC_LEN];
	snapshot_t *sn = NULL;
	int machine_name_len;
	size_t offs;
	
	current_machine_name = (char *)snapshot_machine_name;
	current_filename = NULL;
	current_module = NULL;
	
	sn = lib_malloc(sizeof(snapshot_t));
	sn->write_mode = 0;
	sn->data_size = snapshot_size;
	sn->data = snapshot_data;
	
	sn->pos = 0;
	
	/* Magic string.  */
	if (snapshot_read_byte_array(sn, (BYTE *)magic, SNAPSHOT_MAGIC_LEN) < 0
		|| memcmp(magic, snapshot_magic_string, SNAPSHOT_MAGIC_LEN) != 0) {
		snapshot_error = SNAPSHOT_MAGIC_STRING_MISMATCH_ERROR;
		goto fail;
	}
	
	/* Version number.  */
	if (snapshot_read_byte(sn, major_version_return) < 0
		|| snapshot_read_byte(sn, minor_version_return) < 0) {
		snapshot_error = SNAPSHOT_CANNOT_READ_VERSION_ERROR;
		goto fail;
	}
	
	/* Machine.  */
	if (snapshot_read_byte_array(sn, (BYTE *)read_name, SNAPSHOT_MACHINE_NAME_LEN) < 0) {
		snapshot_error = SNAPSHOT_CANNOT_READ_MACHINE_NAME_ERROR;
		goto fail;
	}
	
	/* Check machine name.  */
	machine_name_len = (int)strlen(snapshot_machine_name);
	if (memcmp(read_name, snapshot_machine_name, machine_name_len) != 0
		|| (machine_name_len != SNAPSHOT_MODULE_NAME_LEN
			&& read_name[machine_name_len] != 0)) {
			snapshot_error = SNAPSHOT_MACHINE_MISMATCH_ERROR;
			goto fail;
		}
	
	/* VICE version and revision */
	memset(snapshot_viceversion, 0, 4);
	snapshot_vicerevision = 0;
	//    offs = ftell(f);
	offs = sn->pos;
	
	if (snapshot_read_byte_array(sn, (BYTE *)magic, SNAPSHOT_VERSION_MAGIC_LEN) < 0
		|| memcmp(magic, snapshot_version_magic_string, SNAPSHOT_VERSION_MAGIC_LEN) != 0) {
		/* old snapshots do not contain VICE version */
		//        fseek(f, offs, SEEK_SET);
		sn->pos = offs;
		log_warning(LOG_DEFAULT, "attempting to load pre 2.4.30 snapshot");
	} else {
		/* actually read the version */
		if (snapshot_read_byte(sn, &snapshot_viceversion[0]) < 0
			|| snapshot_read_byte(sn, &snapshot_viceversion[1]) < 0
			|| snapshot_read_byte(sn, &snapshot_viceversion[2]) < 0
			|| snapshot_read_byte(sn, &snapshot_viceversion[3]) < 0
			|| snapshot_read_dword(sn, &snapshot_vicerevision) < 0) {
			snapshot_error = SNAPSHOT_CANNOT_READ_VERSION_ERROR;
			goto fail;
		}
	}
	
	//    s->first_module_offset = ftell(f);
	sn->first_module_offset = sn->pos;
	
	vsync_suspend_speed_eval();
	return sn;
	
fail:
	lib_free(sn);
	return NULL;
}

	
	
int snapshot_close(snapshot_t *sn)
{
    int retval;

    if (!sn->write_mode)
	{
//        if (zfile_fclose(sn->file) == EOF) {
//            snapshot_error = SNAPSHOT_READ_CLOSE_EOF_ERROR;
//            retval = -1;
//        } else {
//            retval = 0;
//        }
		retval = 0;
		
		if (current_filename != NULL)
		{
			lib_free(sn->data);
		}
    }
	else
	{
		if (current_filename != NULL)
		{
			//        if (fclose(sn->file) == EOF) {
			//            snapshot_error = SNAPSHOT_WRITE_CLOSE_EOF_ERROR;
			//            retval = -1;
			//        } else {
			//            retval = 0;
			//        }
			
			FILE *fp = fopen(current_filename, "wb");
			if (fp == NULL)
			{
				LOGError("snapshot_close: fp is NULL");
				snapshot_error = SNAPSHOT_CANNOT_CREATE_SNAPSHOT_ERROR;
				retval = -1;
			}
			else
			{
				fwrite(sn->data, sn->pos, 1, fp);
				fclose(fp);
				retval = 0;
			}
			
			lib_free(sn->data);
		}
		else
		{
			LOGD("snapshot_close: current_filename is NULL");
		}
	}

    lib_free(sn);
    return retval;
}

static void display_error_with_vice_version(char *text, char *filename)
{
    char *vmessage = lib_malloc(0x100);
    char *message = lib_malloc(0x100 + strlen(text));
    if ((snapshot_viceversion[0] == 0) && (snapshot_viceversion[1] == 0)) {
        /* generic message for the case when no version is present in the snapshot */
        strcpy(vmessage, translate_text(IDGS_SNAPSHOT_OLD_VICE_VERSION));
    } else {
        sprintf(vmessage, translate_text(IDGS_SNAPSHOT_VICE_VERSION),
                snapshot_viceversion[0], snapshot_viceversion[1], snapshot_viceversion[2]);
        if (snapshot_vicerevision != 0) {
            sprintf(message, " (r%d)", (int)snapshot_vicerevision);
            strcat(vmessage, message);
        }
    }
    sprintf(message, "%s\n\n%s.", text, vmessage);
    ui_error(message, filename);
    lib_free(message);
    lib_free(vmessage);
}

void snapshot_display_error(void)
{
    switch (snapshot_error) {
        default:
        case SNAPSHOT_NO_ERROR:
            break;
        case SNAPSHOT_WRITE_EOF_ERROR:
            if (current_module) {
                ui_error(translate_text(IDGS_EOF_WRITING_MODULE_S_SNAPSHOT_S), current_module, current_filename);
            } else {
                ui_error(translate_text(IDGS_EOF_WRITING_SNAPSHOT_S), current_filename);
            }
            break;
        case SNAPSHOT_WRITE_BYTE_ARRAY_ERROR:
            if (current_module) {
                ui_error(translate_text(IDGS_ERROR_WRITING_ARRAY_MODULE_S_SNAPSHOT_S), current_module, current_filename);
            } else {
                ui_error(translate_text(IDGS_ERROR_WRITING_ARRAY_SNAPSHOT_S), current_filename);
            }
            break;
        case SNAPSHOT_READ_EOF_ERROR:
            if (current_module) {
                ui_error(translate_text(IDGS_EOF_READING_MODULE_S_SNAPSHOT_S), current_module, current_filename);
            } else {
                ui_error(translate_text(IDGS_EOF_READING_SNAPSHOT_S), current_filename);
            }
            break;
        case SNAPSHOT_READ_BYTE_ARRAY_ERROR:
            if (current_module) {
                ui_error(translate_text(IDGS_ERROR_READING_ARRAY_MODULE_S_SNAPSHOT_S), current_module, current_filename);
            } else {
                ui_error(translate_text(IDGS_ERROR_READING_ARRAY_SNAPSHOT_S), current_filename);
            }
            break;
        case SNAPSHOT_ILLEGAL_STRING_LENGTH_ERROR:
            if (current_module) {
                ui_error(translate_text(IDGS_ERROR_WRITING_STRING_MODULE_S_SNAPSHOT_S), current_module, current_filename);
            } else {
                ui_error(translate_text(IDGS_ERROR_WRITING_STRING_SNAPSHOT_S), current_filename);
            }
            break;
        case SNAPSHOT_READ_OUT_OF_BOUNDS_ERROR:
            if (current_module) {
                ui_error(translate_text(IDGS_OUT_OF_BOUNDS_READING_MODULE_S_SNAPSHOT_S), current_module, current_filename);
            } else {
                ui_error(translate_text(IDGS_OUT_OF_BOUNDS_READING_SNAPSHOT_S), current_filename);
            }
            break;
        case SNAPSHOT_ILLEGAL_OFFSET_ERROR:
            ui_error(translate_text(IDGS_ILLEGAL_OFFSET_CREATE_MODULE_S_SNAPSHOT_S), current_module, current_filename);
            break;
        case SNAPSHOT_FIRST_MODULE_NOT_FOUND_ERROR:
            ui_error(translate_text(IDGS_CANNOT_FIND_1ST_MODULE_SNAPSHOT_S), current_filename);
            break;
        case SNAPSHOT_MODULE_HEADER_READ_ERROR:
            ui_error(translate_text(IDGS_ERROR_MODULE_HEADER_SNAPSHOT_S), current_filename);
            break;
        case SNAPSHOT_MODULE_NOT_FOUND_ERROR:
            ui_error(translate_text(IDGS_CANNOT_FIND_MODULE_S_SNAPSHOT_S), current_module, current_filename);
            break;
        case SNAPSHOT_MODULE_CLOSE_ERROR:
            ui_error(translate_text(IDGS_ERROR_CLOSING_MODULE_S_SNAPSHOT_S), current_module, current_filename);
            break;
        case SNAPSHOT_MODULE_SKIP_ERROR:
            ui_error(translate_text(IDGS_ERROR_SKIPPING_MODULE_SNAPSHOT_S), current_filename);
            break;
        case SNAPSHOT_CANNOT_CREATE_SNAPSHOT_ERROR:
            ui_error(translate_text(IDGS_CANNOT_CREATE_SNAPSHOT_S), current_filename);
            break;
        case SNAPSHOT_CANNOT_WRITE_MAGIC_STRING_ERROR:
            ui_error(translate_text(IDGS_CANNOT_WRITE_MAGIC_STRING_SNAPSHOT_S), current_filename);
            break;
        case SNAPSHOT_CANNOT_WRITE_VERSION_ERROR:
            ui_error(translate_text(IDGS_CANNOT_WRITE_VERSION_SNAPSHOT_S), current_filename);
            break;
        case SNAPSHOT_CANNOT_WRITE_MACHINE_NAME_ERROR:
            ui_error(translate_text(IDGS_CANNOT_WRITE_MACHINE_NAME_SNAPSHOT_S), current_filename);
            break;
        case SNAPSHOT_CANNOT_OPEN_FOR_READ_ERROR:
            ui_error(translate_text(IDGS_CANNOT_OPEN_SNAPSHOT_S_READING), current_filename);
            break;
        case SNAPSHOT_MAGIC_STRING_MISMATCH_ERROR:
            ui_error(translate_text(IDGS_MAGIC_STRING_MISMATCH_SNAPSHOT_S), current_filename);
            break;
        case SNAPSHOT_CANNOT_READ_VERSION_ERROR:
            ui_error(translate_text(IDGS_CANNOT_READ_VERSION_SNAPSHOT_S), current_filename);
            break;
        case SNAPSHOT_CANNOT_READ_MACHINE_NAME_ERROR:
            ui_error(translate_text(IDGS_CANNOT_READ_MACHINE_NAME_SNAPSHOT_S), current_filename);
            break;
        case SNAPSHOT_MACHINE_MISMATCH_ERROR:
            ui_error(translate_text(IDGS_WRONG_MACHINE_TYPE_SNAPSHOT_S), current_filename, read_name, current_machine_name);
            break;
        case SNAPSHOT_READ_CLOSE_EOF_ERROR:
        case SNAPSHOT_WRITE_CLOSE_EOF_ERROR:
            ui_error(translate_text(IDGS_EOF_CLOSING_SNAPSHOT_S), current_filename);
            break;
        case SNAPSHOT_MODULE_HIGHER_VERSION:
            display_error_with_vice_version(translate_text(IDGS_SNAPSHOT_HIGHER_VERSION), current_filename);
            break;
        case SNAPSHOT_MODULE_INCOMPATIBLE:
            display_error_with_vice_version(translate_text(IDGS_INCOMPATIBLE_SNAPSHOT), current_filename);
            break;
    }
}

void snapshot_set_error(int error)
{
    snapshot_error = error;
}

int snapshot_version_at_least(BYTE major_version, BYTE minor_version, BYTE major_version_required, BYTE minor_version_required)
{
    if (major_version != major_version_required) {
        return 0;
    }

    if (minor_version >= minor_version_required) {
        return 1;
    }

    return 0;
}

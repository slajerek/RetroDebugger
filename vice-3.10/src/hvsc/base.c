/** \file   src/lib/base.c
 * \brief   Base library code
 *
 * \author  Bas Wassink <b.wassink@ziggo.nl>
 */

/*
 *  HVSClib - a library to work with High Voltage SID Collection files
 *  Copyright (C) 2018-2022  Bas Wassink <b.wassink@ziggo.nl>
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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.*
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <ctype.h>

#include "hvsc.h"
#include "hvsc_defs.h"

#ifndef HVSC_STANDALONE
#include "lib.h"
#include "util.h"
#include "md5.h"
#endif


#include "base.h"

/** \brief  Size of chunks to read in hvsc_read_file()
 */
#define READFILE_BLOCK_SIZE  65536


/** \brief  Size of chunks to read int hvsc_text_file_read()
 */
#define READFILE_LINE_SIZE  1024


/** \brief  Error messages
 */
static const char *hvsc_err_messages[HVSC_ERR_CODE_COUNT] = {
    "OK",
    "I/O error",
    "file too large error",
    "libgcrypt error",
    "malformed timestamp",
    "object not found",
    "invalid data or operation",
};


/** \brief  List of field indentifiers
 *
 * \see hvsc_stil_field_type_t
 */
static const char *field_identifiers[] = {
    " ARTIST:",
    " AUTHOR:",
    "    BUG:",     /* XXX: only used in BUGlist.txt */
    "COMMENT:",
    "   NAME:",
    "  TITLE:",
    NULL
};


/** \brief  List of field identifier display string for dumping
 *
 * This makes it more clear to distinguish parser errors (ie NAME: showing up
 * in a field text)
 */
static const char *field_displays[] = {
    " {artist}",
    " {author}",
    "    {bug}",     /* XXX: only used in BUGlist.txt */
    "{comment}",
    "   {name}",
    "  {title}",
    NULL
};


/** \brief  Error message to return for invalid error codes
 */
static const char *invalid_err_msg = "<unknown error code>";


/** \brief  Error code for the library
 */
int hvsc_errno;


/** \brief  Absolute path to the HVSC root directory
 */
char *hvsc_root_path;

/** \brief  Absolute path to the SLDB file
 */
char *hvsc_sldb_path;

/** \brief  Absolute path to the STIL file
 */
char *hvsc_stil_path;

/** \brief  Absolute path to the BUGlist file
 */
char *hvsc_bugs_path;


/** \brief  Get error message for errno \a n
 *
 * \param[in]   n   error code
 *
 * \return  error message
 */
const char *hvsc_strerror(int n)
{
    if (n < 0 || n >= HVSC_ERR_CODE_COUNT) {
        return invalid_err_msg;
    }
    return hvsc_err_messages[n];
}


/** \brief  Print error message on `stderr` optionally with a \a prefix
 *
 * Prints error code and message on `stderr`, and when an I/O error was
 * encountered, the C-library's errno and strerror() will also be printed.
 *
 * \param[in]   prefix  optional prefix
 */
void hvsc_perror(const char *prefix)
{
    /* display prefix? */
    if (prefix != NULL && *prefix != '\0') {
        fprintf(stderr, "%s: ", prefix);
    }

    switch (hvsc_errno) {

        case HVSC_ERR_IO:
            /* I/O error */
            fprintf(stderr, "%d: %s (%d: %s)\n",
                    hvsc_errno, hvsc_strerror(hvsc_errno),
                    errno, strerror(errno));
            break;

        default:
            fprintf(stderr, "%d: %s\n", hvsc_errno, hvsc_strerror(hvsc_errno));
            break;
    }
}


void *hvsc_malloc(size_t size)
{
#ifndef HVSC_STANDALONE
    return lib_malloc(size);
#else
    void *ptr = malloc(size);
    if (ptr == NULL) {
        fprintf(stderr, "%s(): failed to allocate %zu bytes.\n", __func__, size);
        abort();
    }
    return ptr;
#endif
}


void *hvsc_calloc(size_t nmemb, size_t size)
{
#ifndef HVSC_STANDALONE
    return lib_calloc(nmemb, size);
#else
    void *ptr = calloc(nmemb, size);
    if (ptr == NULL) {
        fprintf(stderr, "%s(): failed to allocate %zu bytes.\n", __func__, size);
        abort();
    }
    return ptr;
#endif
}


void *hvsc_realloc(void *ptr, size_t size)
{
#ifndef HVSC_STANDALONE
    return lib_realloc(ptr, size);
#else
    ptr = realloc(ptr, size);
    if (ptr == NULL) {
        fprintf(stderr, "%s(): failed to reallocate %zu bytes.\n", __func__, size);
        abort();
    }
    return ptr;
#endif
}


void hvsc_free(void *ptr)
{
#ifndef HVSC_STANDALONE
    lib_free(ptr);
#else
    free(ptr);
#endif
}




/** \brief  Initialize text file handle
 *
 * \param[in,out]   handle  text file handle
 */
void hvsc_text_file_init_handle(hvsc_text_file_t *handle)
{
    handle->fp      = NULL;
    handle->path    = NULL;
    handle->lineno  = 0;
    handle->linelen = 0;
    handle->buffer  = NULL;
    handle->prevbuf = NULL;
    handle->buflen  = 0;
}


/** \brief  Open text file \a path for reading
 *
 * \param[in]       path    path to file
 * \param[in,out]   handle  file handle, must be allocated by the caller
 *
 * \return  bool
 */
bool hvsc_text_file_open(const char *path, hvsc_text_file_t *handle)
{
    hvsc_dbg("%s(): opening '%s'\n", __func__, path);
    hvsc_text_file_init_handle(handle);

    handle->fp = fopen(path, "rb");
    if (handle->fp == NULL) {
        hvsc_errno = HVSC_ERR_IO;
        return false;
    }
    handle->path    = hvsc_strdup(path);
    handle->lineno  = 0;
    handle->buffer  = hvsc_malloc(READFILE_LINE_SIZE);
    handle->prevbuf = hvsc_malloc(READFILE_LINE_SIZE);
    handle->buflen  = READFILE_LINE_SIZE;
    memset(handle->buffer,  0, sizeof *(handle->buffer));
    memset(handle->prevbuf, 0, sizeof *(handle->prevbuf));
    return true;
}


/** \brief  Close text file via \a handle
 *
 * Cleans up memory used by the members of \a handle, but not \a handle itself
 *
 * \param[in,out]   handle  text file handle
 */
void hvsc_text_file_close(hvsc_text_file_t *handle)
{
    if (handle->path != NULL) {
        hvsc_free(handle->path);
        handle->path = NULL;
    }
    if (handle->buffer != NULL) {
        hvsc_free(handle->buffer);
        handle->buffer = NULL;
    }
    if (handle->prevbuf != NULL) {
        hvsc_free(handle->prevbuf);
        handle->prevbuf = NULL;
    }
    if (handle->fp != NULL) {
        fclose(handle->fp);
        handle->fp = NULL;
    }
}


/** \brief  Read a line from a text file
 *
 * \param[in,out]   handle  text file handle
 *
 * \return  pointer to current line or `NULL` on failure
 */
const char *hvsc_text_file_read(hvsc_text_file_t *handle)
{
    size_t i = 0;

    /* copy current line buffer into previous line buffer */
    memcpy(handle->prevbuf, handle->buffer, handle->buflen);

    while (true) {
        int ch;

        /* resize buffer? */
        if (i == handle->buflen - 1) {
            /* resize buffer */
            handle->buflen *= 2;
#ifdef HVSC_DEBUG
            printf("RESIZING BUFFER TO %zu, lineno %ld\n",
                    handle->buflen, handle->lineno);
#endif
            handle->buffer  = hvsc_realloc(handle->buffer,  handle->buflen);
            handle->prevbuf = hvsc_realloc(handle->prevbuf, handle->buflen);
        }

        ch = fgetc(handle->fp);
        if (ch == EOF) {
            if (feof(handle->fp)) {
                /* OK, proper EOF */
                handle->buffer[i] = '\0';
                if (i == 0) {
                    return NULL;
                } else {
                    return handle->buffer;
                }
            } else {
                hvsc_errno = HVSC_ERR_IO;
                return NULL;
            }
        }

        if (ch == '\n') {
            /* Unix EOL, strip */
            handle->buffer[i] = '\0';
            /* Strip Windows CR */
            if (i > 0 && handle->buffer[i - 1] == '\r') {
                handle->buffer[--i] = '\0';
            }
            handle->lineno++;
            handle->linelen = i;
            return handle->buffer;
        }

        handle->buffer[i++] = (char)ch;
    }
    return handle->buffer;
}


/** @brief  Read data from \a path into \a dest, allocating memory
 *
 * This function reads data from \a path, (re)allocating memory as required.
 * The pointer to the result is stored in \a dest. If this function fails for
 * some reason (file not found, out of memory), -1 is returned and all memory
 * used by this function is freed.
 *
 * READFILE_BLOCK_SIZE bytes are read at a time, and whenever memory runs out,
 * it is doubled in size.
 *
 * @note:   Since this function returns `long`, it can only be used for files
 *          up to 2GB. Should be enough for C64 related files.
 *
 * Example:
 * @code{.c}
 *
 *  unsigned char *data;
 *  int result;
 *
 *  if ((result = hvsc_read_file(&data, "Commando.sid")) < 0) {
 *      fprintf(stderr, "oeps!\n");
 *  } else {
 *      printf("OK, read %ld bytes\n", result);
 *      free(data);
 *  }
 * @endcode
 *
 * @param   dest    destination of data
 * @param   path    path to file
 *
 * @return  number of bytes read, or -1 on failure
 */
long hvsc_read_file(uint8_t **dest, const char *path)
{
    FILE    *fd;
    uint8_t *data;
    size_t   result;
    size_t   offset = 0;
    size_t   size   = READFILE_BLOCK_SIZE;

    fd = fopen(path, "rb");
    if (fd == NULL) {
        hvsc_errno = HVSC_ERR_IO;
        return -1;
    }

    data = hvsc_malloc(READFILE_BLOCK_SIZE);

    /* keep reading chunks until EOF */
    while (true) {
        /* need to resize? */
        if (offset == size) {
            /* yup */
            size *= 2;
            data  = hvsc_realloc(data, size);
        }
        result = fread(data + offset, 1, READFILE_BLOCK_SIZE, fd);
        if (result < READFILE_BLOCK_SIZE) {
            if (feof(fd)) {
                /* OK: EOF */
                *dest = data;
                fclose(fd);
                return (long)(offset + result);
            } else {
                /* IO error */
                hvsc_errno = HVSC_ERR_IO;
                hvsc_free(data);
                *dest = NULL;
                fclose(fd);
                return -1;
            }
        }
        offset += READFILE_BLOCK_SIZE;
    }
    /* shouldn't get here */
}


/** \brief  Copy at most \a n chars of \a s
 *
 * This function appends a nul-byte after \a n bytes.
 *
 * \param[in]   s   string to copy
 * \param[in]   n   maximum number of chars to copy
 *
 * \return  heap-allocated, nul-terminated copy of \a n bytes of \a s, or
 *          `NULL` on failure
 */
char *hvsc_strndup(const char *s, size_t n)
{
    char *t = hvsc_malloc(n + 1);
    strncpy(t, s, n);
    t[n] = '\0';
    return t;
}


/** \brief  Create heap-allocated copy of \a s
 *
 * \param[in]   s   string to copy
 *
 * \return  copy of \a s or `NULL` on error
 */
char *hvsc_strdup(const char *s)
{
#ifndef HVSC_STANDALONE
    return lib_strdup(s);
#else
    char   *t;
    size_t  len = strlen(s);

    t = hvsc_malloc(len + 1u);
    memcpy(t, s, len + 1u);
    return t;
#endif
}


/** \brief  Join paths \a p1 and \a p2
 *
 * Concatenates \a p1 and \a p2, putting a path separator between them. \a p1
 * is expected to not contain a trailing separator and \a p2 is expected to
 * not start with a path separator.
 *
 * \param[in]   p1  first path
 * \param[in]   p2  second path
 *
 * \todo    Make more flexible (handle leading/trailing separators
 * \todo    Handle Windows/DOS paths
 *
 * \return  heap-allocated string
 */
char *hvsc_paths_join(const char *p1, const char *p2)
{
#ifndef HVSC_STANDALONE
    return util_join_paths(p1, p2, NULL);
#else
    char   *result;
    size_t  len1;
    size_t  len2;

    if (p1 == NULL || p2 == NULL) {
        return NULL;
    }

    len1 = strlen(p1);
    len2 = strlen(p2);

    result = hvsc_malloc(len1 + len2 + 2u);   /* +2 for / and '\0' */
    memcpy(result, p1, len1);
# ifdef WINDOWS_COMPILE
    *(result + len1) = '\\';
# else
    *(result + len1) = '/';
# endif
    memcpy(result + len1 + 1u, p2, len2 + 1u);    /* add p2 including '\0' */
    return result;
#endif
}


/** \brief  Set the path to HVSC root, SLDB and STIL
 *
 * \param[in]   path    path to HVSC root directory
 *
 * \return  bool
 */
void hvsc_set_paths(const char *path)
{
    /* set HVSC root path */
    hvsc_root_path = hvsc_strdup(path);
    /* set SLDB path */
    hvsc_sldb_path = hvsc_paths_join(hvsc_root_path, HVSC_SLDB_FILE);
    /* set STIL path */
    hvsc_stil_path = hvsc_paths_join(hvsc_root_path, HVSC_STIL_FILE);
    /* set BUGlist path */
    hvsc_bugs_path = hvsc_paths_join(hvsc_root_path, HVSC_BUGS_FILE);

    hvsc_dbg("HVSC root = %s\n", hvsc_root_path);
    hvsc_dbg("HVSC sldb = %s\n", hvsc_sldb_path);
    hvsc_dbg("HVSC stil = %s\n", hvsc_stil_path);
    hvsc_dbg("HVSC bugs = %s\n", hvsc_bugs_path);
}


/** \brief  Free memory used by the HVSC paths
 */
void hvsc_free_paths(void)
{
    if (hvsc_root_path != NULL) {
        hvsc_free(hvsc_root_path);
        hvsc_root_path = NULL;
    }
    if (hvsc_sldb_path != NULL) {
        hvsc_free(hvsc_sldb_path);
        hvsc_sldb_path = NULL;
    }
    if (hvsc_stil_path != NULL) {
        hvsc_free(hvsc_stil_path);
        hvsc_stil_path = NULL;
    }
    if (hvsc_bugs_path != NULL) {
        hvsc_free(hvsc_bugs_path);
        hvsc_bugs_path = NULL;
    }
}


/** \brief  Strip the HVSC root path from \a path
 *
 * \param[in]   path    path to a PSID file inside the HVSC
 *
 * \return  heap-allocated path with the HVSC root stripped, or a heap-allocated
 *          copy of \a path if the HVSC root wasn't present. Returns `NULL` on
 *          memory allocation failure.
 */
char *hvsc_path_strip_root(const char *path)
{
    size_t  plen = strlen(path);             /* length of path */
    size_t  rlen = (hvsc_root_path == NULL) ? 0 : strlen(hvsc_root_path);   /* length of HVSC root path */
    char   *result;

    if (rlen == 0) {
        result = hvsc_strdup(path);
    } else if (plen <= rlen) {
        result = hvsc_strdup(path);
    } else if (memcmp(path, hvsc_root_path, rlen) == 0) {
        /* got HVSC root path */
        result = hvsc_malloc(plen - rlen + 1u);
        memcpy(result, path + rlen, plen - rlen + 1u);
    } else {
        result = hvsc_strdup(path);
    }
    return result;
}


/** \brief  Determine if a path starts with the HVSC base path
 *
 * \param[in]   path    absolute path to PSID file
 *
 * \return  \c true if \a path starts with the HVSC base path
 */
bool hvsc_path_is_hvsc(const char *path)
{
    size_t plen;
    size_t rlen;

    if (hvsc_root_path == NULL) {
        return false;
    }

    plen = strlen(path);
    rlen = strlen(hvsc_root_path);

    if (rlen >= plen) {
        /* path cannot contain HVSC base directory */
        return false;
    }
    return (bool)(memcmp(path, hvsc_root_path, rlen) == 0);
}



/** \brief  Translate all backslashes into forward slashes
 *
 * Since entries in the SLDB, STIL and BUGlist are listed with forward slashes,
 * on Windows we'll need to fix the directory separators to allow strcmp() to
 * work.
 *
 * \param[in,out]   path    pathname to fix
 */
void hvsc_path_fix_separators(char *path)
{
    while (*path != '\0') {
        if (*path == '\\') {
            *path = '/';
        }
        path++;
    }
}


/** \brief  Check if \a s contains only whitespace
 *
 * \param[in]   s   string to check
 *
 * \return  bool
 */
bool hvsc_string_is_empty(const char *s)
{
    while (*s != '\0' && isspace((unsigned char)*s)) {
        s++;
    }
    return (bool)(*s == '\0');
}


/** \brief  Check if \a s is a comment
 *
 * Checks if the first non-whitespace token in \a s is a '#', indicating a
 * comment.
 *
 * \param[in]   s   string to check
 *
 * \return  bool
 */
bool hvsc_string_is_comment(const char *s)
{
    /* ignore leading whitespace (not strictly required) */
    while (*s != '\0' && isspace((unsigned char)*s)) {
        s++;
    }
    return (bool)(*s == '#');
}


/** \brief  Parse string \a p for a timestamp and return number of seconds
 *
 * Parse a timestamp in the format [M]+:SS, return number of seconds, where
 * [M]+ is minutes and SS is seconds.
 *
 * \param[in]   t       timestamp
 * \param[out]  endptr  object to store pointer to first non-timestamp char
 *
 * \return  time in seconds or -1 on error
 *
 * \todo    update to support milliseconds once HVSC 71 is out
 */
long hvsc_parse_simple_timestamp(char *t, char **endptr)
{
    long m  = 0;    /* minutes */
    long s  = 0;    /* seconds */
    long f  = 0;    /* fractional seconds (ie milliseconds) */
    int  fd = 0;    /* number of fractional digits */

    /* minutes */
    while (isdigit((unsigned char)*t)) {
        m = m * 10 + *t - '0';
        t++;
    }
    if (*t != ':') {
        /* error */
        *endptr    = t;
        hvsc_errno = HVSC_ERR_TIMESTAMP;
        return -1;
    }
    hvsc_dbg("HVSC: got %ld minutes.", m);

    /* seconds */
    t++;
    while (isdigit((unsigned char)*t)) {
        s = s * 10 + *t - '0';
        t++;
        if (s > 59) {
            hvsc_errno = HVSC_ERR_TIMESTAMP;
            return -1;
        }
    }
    /* printf("HVSC: got %ld seconds.", s); */

    /* milliseconds */
    if (*t == '.') {
        /* parse fractional part */
        t++;
        fd = 0;
        f  = 0;

        while (isdigit((unsigned char)*t) && fd < 3) {
            fd++;
            f = f * 10 + *t - '0';
            t++;
        }
        if (fd == 0) {
            hvsc_errno = HVSC_ERR_TIMESTAMP;
            /* printf("HVSC: parsing msecs failed."); */
            return -1;
        }
        /* update fraction to milliseconds */
        while (fd++ < 3) {
            f *= 10;
        }
        /* printf("HVSC: got milliseconds: %ld", f); */
    }

    /* done */
    *endptr = t;
    return (m * 60 + s) * 1000 + f;
}



/** \brief  Determine is \a s hold a field identifier
 *
 * Checks against a list of know field identifiers.
 *
 * \param[in]   s   string to parse
 *
 * \return  field type or -1 (HVSC_FIELD_INVALID) when not found
 *
 * \note    returning -1 does not indicate failure, just that \a s doesn't
 *          contain a field indentifier (ie normal text for a comment or so)
 */
int hvsc_get_field_type(const char *s)
{
    int i = 0;

    while (field_identifiers[i] != NULL) {
        int result = strncmp(s, field_identifiers[i], 8);
        if (result == 0) {
            return i;   /* got it */
        }
        i++;
    }
    return HVSC_FIELD_INVALID;
}


/** \brief  Get display string for field \a type
 *
 * \param[in]   type    field type
 *
 * \return  string
 */
const char *hvsc_get_field_display(int type)
{
    if (type < 0 || type >= HVSC_FIELD_TYPE_COUNT) {
        return "<invalid>";
    }
    return field_displays[type];
}


/** \brief  Get a 16-bit big endian unsigned integer from \a src
 *
 * \param[out]  dest    object to store result
 * \param[in]   src     source data
 */
void hvsc_get_word_be(uint16_t *dest, const uint8_t *src)
{
    *dest = (uint16_t)((src[0] << 8) + src[1]);
}


/** \brief  Get a 16-bit little endian unsigned integer from \a src
 *
 * \param[out]  dest    object to store result
 * \param[in]   src     source data
 */
void hvsc_get_word_le(uint16_t *dest, const uint8_t *src)
{
    *dest = (uint16_t)((src[1] << 8) + src[0]);
}


/** \brief  Get a 32-bit big endian unsigned integer from \a src
 *
 * \param[out]  dest    object to store result
 * \param[in]   src     source data
 */
void hvsc_get_longword_be(uint32_t *dest, const uint8_t *src)
{
    *dest = (uint32_t)((src[0] << 24) + (src[1] << 16) + (src[2] << 8) + src[3]);
}


/** \brief  Create MD5 digest for PSID file
 *
 * Create MD5 digest for a full PSID file to allow looking up files in the SLDB
 * via MD5 digest rather than filename, allowing STIL and SLDB info lookup for
 * files not in the HVSC directory structure.
 *
 * \param[in]   psid    PSID file
 * \param[out]  digest  MD5 digest as hexadecimal string literal (must be at
 *                      least 33 bytes)
 *
 * \return  \c true if a digest was succesfully generated for \a psid
 */
bool hvsc_md5_digest(const char *psid, char *digest)
{
    FILE              *fp;
    uint8_t            hash[HVSC_DIGEST_SIZE];
    size_t             i;
    static const char  digits[] = "0123456789abcdef";

    fp = fopen(psid, "rb");
    if (fp == NULL) {
        hvsc_errno = HVSC_ERR_IO;
        return false;
    }

    md5File(fp, hash);
    fclose(fp);
    for (i = 0; i < sizeof hash; i++) {
        digest[i * 2 + 0] = digits[hash[i] >> 4];
        digest[i * 2 + 1] = digits[hash[i] & 0x0f];
    }
    digest[i * 2] = '\0';

    return true;
}

/** \file   src/lib/sldb.c
 * \brief   Songlength database handling
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

#undef HVSC_DEBUG

#include "vice.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <inttypes.h>
#include <ctype.h>

#ifdef HVSC_USE_MD5
# include <gcrypt.h>
#endif
#ifndef HVSC_STANDALONE
# include "log.h"
#endif
#include "hvsc.h"
#include "hvsc_defs.h"
#include "base.h"

#include "sldb.h"


#ifdef HVSC_USE_MD5

/** \brief  Calculate MD5 hash of file \a psid
 *
 * \param[in]   psid    PSID file
 * \param[out]  digest  memory to store MD5 digest, needs to be 16+ bytes
 *
 * \return  bool
 */
static bool create_md5_hash(const char *psid, unsigned char *digest)
{
    gcry_md_hd_t   handle;
    gcry_error_t   err;
    unsigned char *data;
    unsigned char *d;
    long           size;

    /* attempt to open file */
    size = hvsc_read_file(&data, psid);
    if (size < 0) {
        return false;
    }

    /*
     * calculate MD5 hash
     */
    err = gcry_md_open(&handle, GCRY_MD_MD5, 0);
    if (err != 0) {
        hvsc_errno = HVSC_ERR_GCRYPT;
        hvsc_free(data);
        return false;
    }

    gcry_md_write(handle, data, (size_t)size);
    d = gcry_md_read(handle, GCRY_MD_MD5);
    memcpy(digest, d, HVSC_DIGEST_SIZE);

    gcry_md_close(handle);
    hvsc_free(data);
    return true;
}
#endif


/** \brief  Find SLDB entry by \a digest
 *
 * The \a digest has to be in the same string form as the SLDB. So 32 bytes
 * representing a 16-byte hex data, in lower case.
 *
 * \param[in]   digest  string representation of the MD5 digest (32 bytes)
 *
 * \return  line of text from SLDB or `NULL` when not found
 */
static char *find_sldb_entry_md5(const char *digest)
{
    hvsc_text_file_t  handle;
    const char       *line;

    if (!hvsc_text_file_open(hvsc_sldb_path, &handle)) {
        return NULL;
    }

    while (true) {
        line = hvsc_text_file_read(&handle);
        if (line == NULL) {
            hvsc_text_file_close(&handle);
            return NULL;
        }
#if 0
        printf("%s\n", line);
#endif
        if (memcmp(digest, line, HVSC_DIGEST_SIZE * 2) == 0) {
            /* copy the current line before closing the file */
            char *s = hvsc_strdup(handle.buffer);
            hvsc_text_file_close(&handle);
            return s;
        }
    }
/*
    hvsc_text_file_close(&handle);
    hvsc_errno = HVSC_ERR_NOT_FOUND;
    return NULL;
*/
}

/** \brief  Find song length entry by PSID name in the comments
 *
 * \param[in]   path    relative path in the HVSC to the SID
 *
 * \return  text line with the song length info or `NULL` on failure
 */
static char *find_sldb_entry_txt(const char *path)
{
    hvsc_text_file_t  handle;
    size_t            plen;
    const char       *line;

#ifndef HVSC_STANDALONE
    log_message(LOG_DEFAULT, "VSID: Opening '%s'.", hvsc_sldb_path);
#endif
    if (!hvsc_text_file_open(hvsc_sldb_path, &handle)) {
#ifndef HVSC_STANDALONE
        log_warning(LOG_DEFAULT, "VSID: Failed to open the SLDB.");
#endif
        return NULL;
    }

    plen = strlen(path);

    while (true) {
        line = hvsc_text_file_read(&handle);
        if (line == NULL) {
            hvsc_text_file_close(&handle);
#ifndef HVSC_STANDALONE
            log_warning(LOG_DEFAULT,
                    "VSID: Could not find song length data for current SID.");
#endif
            return NULL;
        }


        if (*line == ';') {
            if (strncmp(path, line + 2, plen) == 0) {
                /* next line contains the actual entry */
                char *s;
                line = hvsc_text_file_read(&handle);
                if (line == NULL) {
                    hvsc_text_file_close(&handle);
                    return NULL;
                }
                s = hvsc_strdup(handle.buffer);
                hvsc_text_file_close(&handle);
                return s;
            }
        }
    }

#if 0 /* above loop never breaks - following code can never execute */
    hvsc_text_file_close(&handle);
    hvsc_errno = HVSC_ERR_NOT_FOUND;
    return NULL;
#endif
}

/** \brief  Parse SLDB entry
 *
 * The song lengths array is heap-allocated and should freed after use.
 *
 * \param[in]   line    SLDB entry (including hash + '=')
 * \param[out]  lengths object to store pointer to array of song lengths
 *
 * \return  number of songs or -1 on error
 */
static int parse_sldb_entry(char *line, long **lengths)
{
    char *p;
    char *endptr;
    long *entries;
    long  secs;
    int   i = 0;

    entries = hvsc_malloc(256 * sizeof *entries);
    if (entries == NULL) {
        return -1;
    }

    p = line + (HVSC_DIGEST_SIZE * 2 + 1);  /* skip MD5HASH and '=' */

    while (*p != '\0') {
        /* skip whitespace */
        while (*p != '\0' && isspace((unsigned char)(*p))) {
            p++;
        }
        if (*p == '\0') {
            *lengths = entries;
            return i;
        }

        secs = hvsc_parse_simple_timestamp(p, &endptr);
        if (secs < 0) {
            hvsc_free(entries);
            return -1;
        }
        entries[i++] = secs;
        p = endptr;
    }

    *lengths = entries;
    return i;
}


/** \brief  Get the SLDB entry for PSID file \a psid
 *
 * \param[in]   psid    path to PSID file
 *
 * \return  heap-allocated entry or `NULL` on failure
 */
char *hvsc_sldb_get_entry_md5(const char *psid)
{
    char digest[HVSC_DIGEST_SIZE * 2 + 1];
    char *entry;

    if (!hvsc_md5_digest(psid, digest)) {
        return NULL;
    }
    hvsc_dbg("md5 digest for %s = %s\n", psid, digest);

    /* parse SLDB */
    entry = find_sldb_entry_md5(digest);
    if (entry != NULL) {
        hvsc_dbg("got it: %s\n", entry);
    }
    return entry;
}


/** \brief  Find SLDB entry by using text lookup
 *
 * This function uses the "; /path/to/file" lines to identify the SID entry,
 * which makes using/linking against libgcrypt no longer required.
 *
 * \param   [in]    psid    absolute path to SID in the HVSC
 *
 * \return  line of text containing the song length info or `NULL` on failure
 */
char *hvsc_sldb_get_entry_txt(const char *psid)
{
    char *path;
    char *entry;

    /* strip HVSC root from path */
    path = hvsc_path_strip_root(psid);
#ifdef WINDOWS_COMPILE
    /* fix directory separators */
    hvsc_path_fix_separators(path);
#endif

    entry = find_sldb_entry_txt(path);
    hvsc_free(path);
    if (entry != NULL) {
        /* hvsc_dbg("Got it: %s\n", entry); */
#ifndef HVSC_STANDALONE
        log_message(LOG_DEFAULT, "VSID: Song length(s): %s.", entry);
#endif
    }
    return entry;
}


/** \brief  Get a list of song lengths for PSID file md5 \a digest
 *
 * \param[in]   digest  md5 digest of PSID file
 * \param[out]  lengths object to store pointer to array of song lengths
 *
 * \return  number of songs or -1 on error
 *
 * \note    The caller is responsible for freeing \a lengths after use.
 */
int hvsc_sldb_get_lengths_md5(const char *digest, long **lengths)
{
    char *entry;
    int   result = -1;

    *lengths = NULL;
    entry = find_sldb_entry_md5(digest);
    if (entry != NULL) {
        hvsc_dbg("got entry for md5 digest %s: %s\n", digest, entry);
        result = parse_sldb_entry(entry, lengths);
        hvsc_free(entry);
    }
    return result;
}


/** \brief  Get a list of song lengths for PSID file \a psid
 *
 * \param[in]   psid    path to PSID file
 * \param[out]  lengths object to store pointer to array of song lengths
 *
 * \return  number of songs or -1 on error
 *
 * \note    The caller is responsible for freeing \a lengths after use.
 */
int hvsc_sldb_get_lengths(const char *psid, long **lengths)
{
    char *entry;
    int   result;

    *lengths = NULL;

    entry = hvsc_sldb_get_entry_md5(psid);
    /* entry = hvsc_sldb_get_entry_txt(psid); */
    if (entry == NULL) {
        return -1;
    }

    result = parse_sldb_entry(entry, lengths);
    if (result < 0) {
        hvsc_free(*lengths);
        return -1;
    }
    hvsc_free(entry);
    return result;
}


/** \brief  Get relative HVSC path for md5 digest in SLDB
 *
 * Iterate \c Songlengths.md5 looking for md5 \a digest and return the relative
 * path contained in the comment line just above the md5 line.
 *
 * \param[in]   digest  md5 digest (nul-terminated 32-byte hexadecimal literal)
 *
 * \return  relative path of PSID file with md5 \a digest in the SLDB, or
 *          \c NULL when not found
 *
 * \note    the return value is allocated on the heap and should be freed with
 *          \c hvsc_free() after use
 */
char *hvsc_sldb_get_path_for_md5(const char *digest)
{
    hvsc_text_file_t handle;
#ifdef HVSC_DEBUG
    int              lineno = 1;
#endif

    if (hvsc_text_file_open(hvsc_sldb_path, &handle)) {
        const char *line;

        while ((line = hvsc_text_file_read(&handle)) != NULL) {
            if (isalnum((unsigned char)*line) &&
                    strncmp(digest, line, HVSC_DIGEST_SIZE * 2u) == 0) {
                hvsc_dbg("got matching md5 sum at line %d: %s\n",
                         lineno, digest);
                hvsc_dbg("HVSC path for md5 sum: %s\n", handle.prevbuf + 2);
                return hvsc_strdup(handle.prevbuf + 2);
            }
#ifdef HVSC_DEBUG
            lineno++;
#endif
        }
    }
    return NULL;
}

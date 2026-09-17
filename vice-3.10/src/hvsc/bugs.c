/** \file   src/lib/bugs.c
 * \brief   BUGlist.txt handling
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
#include "base.h"

#include "bugs.h"


/** \brief  Initialize BUGlist \a handle
 *
 * \param[in,out]   handle  BUGlist handle
 */
static void bugs_init_handle(hvsc_bugs_t *handle)
{
    hvsc_text_file_init_handle(&(handle->bugs));
    handle->psid_path = NULL;
    handle->text      = NULL;
    handle->user      = NULL;
}

/** \brief  Free all members of \a handle but not \a handle itself
 *
 * \param[in,out]   handle  BUGS handle
 */
static void bugs_free_handle(hvsc_bugs_t *handle)
{
    hvsc_text_file_close(&(handle->bugs));
    if (handle->text != NULL) {
        hvsc_free(handle->text);
    }
    if (handle->user != NULL) {
        hvsc_free(handle->user);
    }
}

/** \brief  Parse the BUGlist for a BUG: field and the (username) field
 *
 * \param[in,out]   handle  BUGlist handle
 *
 * \return  bool
 */
static bool bugs_parse(hvsc_bugs_t *handle)
{
    const char *line;
    char *bug;

    /* grab first line, should contain 'BUG:' */
    line = hvsc_text_file_read(&(handle->bugs));
    if (line == NULL) {
        return false;
    }

    hvsc_dbg("First line of entry: %s\n", line);
    if (hvsc_get_field_type(line) != HVSC_FIELD_BUG) {
        hvsc_dbg("Fail: not a BUG field\n");
        return false;
    }

    /* store first line of BUG field */
    bug = hvsc_strdup(line + 9);

    /* add rest of BUG field */
    while (true) {
        line = hvsc_text_file_read(&(handle->bugs));
        if (line == NULL) {
            /* not supposed to happen */
            hvsc_free(bug);
            return false;
        }

        if (strncmp("         ", line, 9u) == 0) {
            /* new line for the bug field */
            size_t len;

            /* strip off 8 spaces, leaving one to add to the result */
            len = strlen(line) - 8u;
            bug = hvsc_realloc(bug, strlen(bug) + len + 1u);
            strcat(bug, line + 8);
        } else {
            /* store bug in handle */
            handle->text = bug;
            /* assume (user) field */
            handle->user = hvsc_strdup(line);
            return true;
        }
    }
    return true;
}


/** \brief  Open BUGlist and search for file \a psid
 *
 * \param[in]       psid    absolute path to PSID file
 * \param[in,out]   handle  BUGlist handle
 *
 * \return  bool
 */
bool hvsc_bugs_open(const char *psid, hvsc_bugs_t *handle)
{
    bugs_init_handle(handle);

    /* open BUGlist.txt */
    if (!hvsc_text_file_open(hvsc_bugs_path, &(handle->bugs))) {
        return false;
    }

    /* make copy of psid, ripping off the HVSC root directory */
    handle->psid_path = hvsc_path_strip_root(psid);
#ifdef WINDOWS_COMPILE
    /* fix directory separators */
    hvsc_path_fix_separators(handle->psid_path);
#endif
    hvsc_dbg("stripped path is '%s'\n", handle->psid_path);
    if (handle->psid_path == NULL) {
        hvsc_bugs_close(handle);
        return false;
    }

    /* find the entry */
    while (true) {
        const char *line;

        line = hvsc_text_file_read(&(handle->bugs));
        if (line == NULL) {
            if (feof(handle->bugs.fp)) {
                /* EOF, so simply not found */
                hvsc_errno = HVSC_ERR_NOT_FOUND;
            }
            hvsc_bugs_close(handle);
            /* I/O error is already set */
            return false;
        }

        if (strcmp(line, handle->psid_path) == 0) {
            hvsc_dbg("Found '%s' at line %ld\n", line, handle->bugs.lineno);
            return bugs_parse(handle);
        }
    }

#if 0 /* above loop never breaks - following code can never execute */
    /* not found */
    hvsc_errno = HVSC_ERR_NOT_FOUND;
    hvsc_bugs_close(handle);
    return true;
#endif
}


/** \brief  Clean up memory used by the members of \a handle
 *
 * \param[in,out]   handle  BUGlist handle
 */
void hvsc_bugs_close(hvsc_bugs_t *handle)
{
    bugs_free_handle(handle);
    if (handle->psid_path != NULL) {
        hvsc_free(handle->psid_path);
    }
}

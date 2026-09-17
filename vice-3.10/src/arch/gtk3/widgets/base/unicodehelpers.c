/** \file   unicodehelpers.c
 * \brief   Helpers for unicode
 *
 * \author  Groepaz
 *
 */

/*
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

#include <gtk/gtk.h>
#include <string.h>
#include <stdbool.h>

#include "charset.h"
#include "lib.h"
#include "resources.h"

#include "vice_gtk3_settings.h"
#include "debug_gtk3.h"

#include "unicodehelpers.h"

/*
 *  "C64 Pro"
 */

/** \brief  Convert petscii encoded string to utf8 string we can show using the CBM font
 *
 * this function handles all characters that may appear in a directory listing,
 * including "non printable" control characters, which appear as inverted characters
 * in so called "quote mode".
 *
 * \param[in]   s           PETSCII string to convert to UTF-8
 * \param[in]   inverted    use inverted mode
 * \param[in]   lowercase   use the lowercase chargen
 *
 * \return  heap-allocated UTF-8 string, free with lib_free()
 *
 * \note    only valid for the "C64_Pro_Mono-STYLE.ttf" font, not the old
 *          "CBM.ttf" font.
 *
 * \note    Somehow the inverted space has a line on top on at least Linux,
 *          the codepoint seems fine though, so perhaps a bug in Pango?
 */
unsigned char *vice_gtk3_petscii_to_utf8(unsigned char *s,
                                         bool inverted,
                                         bool lowercase)
{
    unsigned char *d, *r;
    unsigned int codepoint;

    r = d = lib_malloc((size_t)(strlen((char *)s) * 3 + 1));

    while (*s) {

        /* 0xe000-0xe0ff codepoints cover the regular, uppercase, petscii codes
                         in ranges 0x20-0x7f and 0xa0-0xff
           0xe100-0xe1ff codepoints cover the regular, lowercase, petscii codes
                         in ranges 0x20-0x7f and 0xa0-0xff
           0xe200-0xe2ff codepoints cover the same characters, but contain the
                         respective inverted glyphs.
           0xe300-0xe3ff codepoints cover the same characters, but contain the
                         respective inverted lowercase glyphs.

           regular valid petscii codes are converted as is, petscii control
           codes will produce the glyph that the petscii code would produce
           in so called "quote mode".
        */

        /* first convert petscii to utf8 codepoint */
        if (*s < 0x20) {
            /* petscii 0x00-0x1f  control codes (inverted @ABC..etc) */
            codepoint = *s + 0xe240;            /* 0xe240-0xe25f */
        } else if (*s < 0x80) {
            /* petscii 0x20-0x7f  printable petscii codes */
            codepoint = *s + 0xe000;            /* 0xe020-0xe07f */
        } else if (*s < 0xa0) {
            /* petscii 0x80-0x9f  control codes (inverted SHIFT+@ABC..etc) */
            codepoint = (*s - 0x80) + 0xe260;   /* 0xe260-0xe27f */
        } else {
            /* petscii 0xa0-0xff  printable petscii codes */
            codepoint = *s + 0xe000;            /* 0xe0a0-0xe0ff */
        }
        if (inverted) {
            codepoint ^= 0x0200;                /* 0xe0XX <-> 0xe2XX */
        }
        /* switch to lower case if requested */
        if (lowercase) {
            codepoint ^= 0x0100;
        }
        s++;

        /* now copy to the destination string and convert to utf8 */
        /* we can get away with just this, because all codepoints are > 4095 */
        /* three byte form - 1110xxxx 10xxxxxx 10xxxxxx */
        *d++ = 0xe0 | ((codepoint >> 12) & 0x0f);
        *d++ = 0x80 | ((codepoint >> (6)) & 0x3f);
        *d   = 0x80 | ((codepoint >> (0)) & 0x3f);
        d++;
    }
    *d = '\0';

    return r;
}

unsigned char *vice_gtk3_petscii_upper_to_utf8(unsigned char *s, bool inverted)
{
    return vice_gtk3_petscii_to_utf8(s, inverted, false);
}

unsigned char *vice_gtk3_scrcode_to_utf8(unsigned char *s,
                                         bool inverted,
                                         bool lowercase)
{
    unsigned char *d, *r;
    unsigned int codepoint;

    r = d = lib_malloc((size_t)(strlen((char *)s) * 3 + 1));

    while (*s) {

        /* 0xe000-0xe0ff codepoints cover the regular, uppercase, petscii codes
                         in ranges 0x20-0x7f and 0xa0-0xff
           0xe100-0xe1ff codepoints cover the regular, lowercase, petscii codes
                         in ranges 0x20-0x7f and 0xa0-0xff
           0xe200-0xe2ff codepoints cover the same characters, but contain the
                         respective inverted glyphs.
           0xe300-0xe3ff codepoints cover the same characters, but contain the
                         respective inverted lowercase glyphs.

           regular valid petscii codes are converted as is, petscii control
           codes will produce the glyph that the petscii code would produce
           in so called "quote mode".
        */

        /* scrcode -> petscii */
        if ((*s >= 0x60) && (*s <= 0x7f)) {
            *s = *s + 0x40;
        } else if ((*s >= 0xe0) && (*s <= 0xfe)) {
            *s = *s;
        } else if /*(*/ (*s == 0xff) /*&& (*s <= 0xff)) */ {
            *s = 0xbf;
        } else {
            *s = charset_screencode_to_petscii(*s);
        }

        /* first convert petscii to utf8 codepoint */
        if (*s < 0x20) {
            /* petscii 0x00-0x1f  control codes (inverted @ABC..etc) */
            codepoint = *s + 0xe240;            /* 0xe240-0xe25f */
        } else if (*s < 0x80) {
            /* petscii 0x20-0x7f  printable petscii codes */
            codepoint = *s + 0xe000;            /* 0xe020-0xe07f */
        } else if (*s < 0xa0) {
            /* petscii 0x80-0x9f  control codes (inverted SHIFT+@ABC..etc) */
            codepoint = (*s - 0x80) + 0xe260;   /* 0xe260-0xe27f */
        } else {
            /* petscii 0xa0-0xff  printable petscii codes */
            codepoint = *s + 0xe000;            /* 0xe0a0-0xe0ff */
        }

        if (inverted) {
            codepoint ^= 0x0200;                /* 0xe0XX <-> 0xe2XX */
        }
        /* switch to lower case if requested */
        if (lowercase) {
            codepoint ^= 0x0100;
        }
        s++;

        /* now copy to the destination string and convert to utf8 */
        /* we can get away with just this, because all codepoints are > 4095 */
        /* three byte form - 1110xxxx 10xxxxxx 10xxxxxx */
        *d++ = 0xe0 | ((codepoint >> 12) & 0x0f);
        *d++ = 0x80 | ((codepoint >> (6)) & 0x3f);
        *d   = 0x80 | ((codepoint >> (0)) & 0x3f);
        d++;
    }
    *d = '\0';

    return r;
}

unsigned char *vice_gtk3_scrcode_upper_to_utf8(unsigned char *s, bool inverted)
{
    return vice_gtk3_scrcode_to_utf8(s, inverted, false);
}

/*
 *  "Pet Me"
 */

/** \brief  Convert petscii encoded string to utf8 string we can show using a CBM font
 *
 * this function handles all "printable" PETSCII characters,
 * including "non printable" control characters, which appear as inverted characters
 * in so called "quote mode".
 *
 * \param[in]   s           PETSCII string to convert to UTF-8
 * \param[in]   inverted    use inverted mode
 * \param[in]   lowercase   use the lowercase chargen
 *
 * \return  heap-allocated UTF-8 string, free with lib_free()
 *
 * \note    only valid for the "PetMe.ttf" font
 */
unsigned char *vice_gtk3_petscii_to_utf8_petme(unsigned char *s,
                                         bool inverted,
                                         bool lowercase)
{
    unsigned char *d, *r;
    unsigned int codepoint;
    int ch;

    r = d = lib_malloc((size_t)(strlen((char *)s) * 3 + 1));

    while (*s) {
        /*
            In Pet Me, Pet Me 2X, and Pet Me 2Y, code points
                0xE000-0xE0FF encode the complete Commodore PET English character set
                0xE100-0xE1FF encode the complete Commodore PET German character set.
                0xE200-0xE3FF encode the complete Commodore VIC-20 character set.
                0xE400-0xE5FF encode the complete Commodore 128 German character set
                0xE600-0xE7FF encode the complete Commodore 128 French character set.
                0xE800-0xE8FF encode the complete CBM2 character set.
         */

        if (*s < 0x20) {
            /* petscii 0x00-0x1f  control codes (inverted @ABC..etc) */
            ch = *s;
            inverted ^= 0x80;
        } else if ((*s >= 0x80) && (*s <= 0x9f)) {
            /* petscii 0x80-0x9f  control codes (inverted SHIFT+@ABC..etc) */
            ch = (*s - 0x80) + 0x60;
            ch = charset_petscii_to_screencode(ch, false);
            inverted ^= 0x80;
        } else {
            ch = charset_petscii_to_screencode(*s, false);
            inverted ^= ch & 0x80;
            ch = ch & 0x7f;
        }

        codepoint = ch + 0xe200;

        if (inverted) {
            codepoint ^= 0x0080;                /* 0xe0XX <-> 0xe2XX */
        }
        /* switch to lower case if requested */
        if (lowercase) {
            codepoint ^= 0x0100;
        }
        s++;

        /* now copy to the destination string and convert to utf8 */
        /* we can get away with just this, because all codepoints are > 4095 */
        /* three byte form - 1110xxxx 10xxxxxx 10xxxxxx */
        *d++ = 0xe0 | ((codepoint >> 12) & 0x0f);
        *d++ = 0x80 | ((codepoint >> (6)) & 0x3f);
        *d   = 0x80 | ((codepoint >> (0)) & 0x3f);
        d++;
    }
    *d = '\0';

    return r;
}

unsigned char *vice_gtk3_petscii_upper_to_utf8_petme(unsigned char *s, bool inverted)
{
    return vice_gtk3_petscii_to_utf8_petme(s, inverted, false);
}

unsigned char *vice_gtk3_scrcode_to_utf8_petme(unsigned char *s,
                                         bool inverted,
                                         bool lowercase)
{
    unsigned char *d, *r;
    unsigned int codepoint;
    int ch;

    r = d = lib_malloc((size_t)(strlen((char *)s) * 3 + 1));

    while (*s) {
        /*
            In Pet Me, Pet Me 2X, and Pet Me 2Y, code points
                0xE000-0xE0FF encode the complete Commodore PET English character set
                0xE100-0xE1FF encode the complete Commodore PET German character set.
                0xE200-0xE3FF encode the complete Commodore VIC-20 character set.
                0xE400-0xE5FF encode the complete Commodore 128 German character set
                0xE600-0xE7FF encode the complete Commodore 128 French character set.
                0xE800-0xE8FF encode the complete CBM2 character set.
         */

        ch = *s & 0x7f;
        codepoint = ch + 0xe200;

        if (inverted) {
            codepoint ^= 0x0080;                /* 0xe0XX <-> 0xe2XX */
        }
        /* switch to lower case if requested */
        if (lowercase) {
            codepoint ^= 0x0100;
        }
        s++;

        /* now copy to the destination string and convert to utf8 */
        /* we can get away with just this, because all codepoints are > 4095 */
        /* three byte form - 1110xxxx 10xxxxxx 10xxxxxx */
        *d++ = 0xe0 | ((codepoint >> 12) & 0x0f);
        *d++ = 0x80 | ((codepoint >> (6)) & 0x3f);
        *d   = 0x80 | ((codepoint >> (0)) & 0x3f);
        d++;
    }
    *d = '\0';

    return r;
}

unsigned char *vice_gtk3_scrcode_upper_to_utf8_petme(unsigned char *s, bool inverted)
{
    return vice_gtk3_scrcode_to_utf8_petme64(s, inverted, false);
}

/*
 *  "Pet Me 64"
 */

/** \brief  Convert petscii encoded string to utf8 string we can show using a CBM font
 *
 * this function handles all "printable" PETSCII characters,
 * including "non printable" control characters, which appear as inverted characters
 * in so called "quote mode".
 *
 * \param[in]   s           PETSCII string to convert to UTF-8
 * \param[in]   inverted    use inverted mode
 * \param[in]   lowercase   use the lowercase chargen
 *
 * \return  heap-allocated UTF-8 string, free with lib_free()
 *
 * \note    only valid for the "PetMe64.ttf" font
 */
unsigned char *vice_gtk3_petscii_to_utf8_petme64(unsigned char *s,
                                         bool inverted,
                                         bool lowercase)
{
    unsigned char *d, *r;
    unsigned int codepoint;
    int ch;

    r = d = lib_malloc((size_t)(strlen((char *)s) * 3 + 1));

    while (*s) {
        /*
            In Pet Me 64, Pet Me 64 2Y, Pet Me 128, and Pet Me 128 2Y, code points
                0xE000-0xE1FF encode the complete Commodore 64 character set.
                0xE200-0xE3FF encode the complete Commodore 128 English character set
                0xE400-0xE5FF encode the complete Commodore 128 Swedish character set.
         */

        if (*s < 0x20) {
            /* petscii 0x00-0x1f  control codes (inverted @ABC..etc) */
            ch = *s;
            inverted ^= 0x80;
        } else if ((*s >= 0x80) && (*s <= 0x9f)) {
            /* petscii 0x80-0x9f  control codes (inverted SHIFT+@ABC..etc) */
            ch = (*s - 0x80) + 0x60;
            ch = charset_petscii_to_screencode(ch, false);
            inverted ^= 0x80;
        } else {
            ch = charset_petscii_to_screencode(*s, false);
            inverted ^= ch & 0x80;
            ch = ch & 0x7f;
        }

        codepoint = ch + 0xe000;

        if (inverted) {
            codepoint ^= 0x0080;                /* 0xe0XX <-> 0xe2XX */
        }
        /* switch to lower case if requested */
        if (lowercase) {
            codepoint ^= 0x0100;
        }
        s++;

        /* now copy to the destination string and convert to utf8 */
        /* we can get away with just this, because all codepoints are > 4095 */
        /* three byte form - 1110xxxx 10xxxxxx 10xxxxxx */
        *d++ = 0xe0 | ((codepoint >> 12) & 0x0f);
        *d++ = 0x80 | ((codepoint >> (6)) & 0x3f);
        *d   = 0x80 | ((codepoint >> (0)) & 0x3f);
        d++;
    }
    *d = '\0';

    return r;
}

unsigned char *vice_gtk3_petscii_upper_to_utf8_petme64(unsigned char *s, bool inverted)
{
    return vice_gtk3_petscii_to_utf8_petme64(s, inverted, false);
}

unsigned char *vice_gtk3_scrcode_to_utf8_petme64(unsigned char *s,
                                         bool inverted,
                                         bool lowercase)
{
    unsigned char *d, *r;
    unsigned int codepoint;
    int ch;

    r = d = lib_malloc((size_t)(strlen((char *)s) * 3 + 1));

    while (*s) {
        /*
            In Pet Me 64, Pet Me 64 2Y, Pet Me 128, and Pet Me 128 2Y, code points
                0xE000-0xE1FF encode the complete Commodore 64 character set.
                0xE200-0xE3FF encode the complete Commodore 128 English character set
                0xE400-0xE5FF encode the complete Commodore 128 Swedish character set.
         */

        ch = *s & 0x7f;
        codepoint = ch + 0xe000;

        if (inverted) {
            codepoint ^= 0x0080;                /* 0xe0XX <-> 0xe2XX */
        }
        /* switch to lower case if requested */
        if (lowercase) {
            codepoint ^= 0x0100;
        }
        s++;

        /* now copy to the destination string and convert to utf8 */
        /* we can get away with just this, because all codepoints are > 4095 */
        /* three byte form - 1110xxxx 10xxxxxx 10xxxxxx */
        *d++ = 0xe0 | ((codepoint >> 12) & 0x0f);
        *d++ = 0x80 | ((codepoint >> (6)) & 0x3f);
        *d   = 0x80 | ((codepoint >> (0)) & 0x3f);
        d++;
    }
    *d = '\0';

    return r;
}

unsigned char *vice_gtk3_scrcode_upper_to_utf8_petme64(unsigned char *s, bool inverted)
{
    return vice_gtk3_scrcode_to_utf8_petme64(s, inverted, false);
}

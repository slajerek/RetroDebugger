/***************************************************************************
                          ini.cpp  -  Reads and writes keys to a
                                      ini file.

                             -------------------
    begin                : Fri Apr 21 2000
    copyright            : (C) 2000 by Simon White
    email                : s_a_white@email.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
/***************************************************************************
 * $Log: ini.cpp,v $
 * Revision 1.12  2004/02/26 18:02:58  s_a_white
 * Sync with libini project.
 *
 * Revision 1.26  2003/04/26 08:45:36  s_a_white
 * parser.first can be 0 for anonymous sections (should be checking against
 * -1 anyway).
 *
 * Revision 1.25  2003/02/04 22:54:02  s_a_white
 * Treat NULL comment string as no comment (same as 0 length comment string).
 *
 * Revision 1.24  2002/09/24 19:04:22  s_a_white
 * Fixed unchecked pointers in ini_append and __ini_store.
 *
 * Revision 1.23  2002/09/10 08:07:30  s_a_white
 * Cleanup after new file creation check to prevent use of a closed file.
 *
 * Revision 1.22  2002/06/15 22:51:36  s_a_white
 * Re-added INI_ALLOW_COMMENT to allow comments without first having
 * seen a space.
 *
 * Revision 1.21  2002/06/14 13:22:03  s_a_white
 * Bug fix to prevent sometimes accessing a NULL pointer.
 *
 * Revision 1.20  2002/06/11 16:23:29  s_a_white
 * __ini_process nolonger returns with section/key selected
 * undefined.
 *
 * Revision 1.19  2002/06/11 14:11:19  s_a_white
 * Prevent key=key2=data being processed as two comments.
 * Made comments only legal before or after a key.
 *
 * Revision 1.18  2002/06/10 16:59:08  s_a_white
 * Improved handling of keynames with [] and
 * section headings with = characters.
 *
 * Revision 1.17  2002/06/06 16:32:51  s_a_white
 * Multi character comments now supported.
 *
 * Revision 1.16  2002/02/18 19:56:35  s_a_white
 * Faster CRC alogarithm for hash key generation.
 *
 * Revision 1.15  2002/01/15 21:08:43  s_a_white
 * crc fix.
 *
 * Revision 1.14  2001/11/15 21:19:34  s_a_white
 * Appended __ini_ to global variables and INI_ to #defines.
 *
 * Revision 1.13  2001/11/15 21:13:13  s_a_white
 * Appended __ini_ to strtrim and createCrc32.
 *
 * Revision 1.12  2001/09/30 15:15:30  s_a_white
 * Fixed backup file removal.
 *
 * Revision 1.11  2001/09/22 08:56:06  s_a_white
 * Added mode support.  This is used to determine how the INI file should be
 * accessed.
 *
 * Revision 1.10  2001/08/23 19:59:18  s_a_white
 * ini_append fix so not freeing wrong buffer.
 *
 * Revision 1.9  2001/08/17 19:23:16  s_a_white
 * Added ini_append.
 *
 * Revision 1.8  2001/08/14 22:21:21  s_a_white
 * Hash table and list removal fixes.
 *
 * Revision 1.7  2001/07/27 11:10:15  s_a_white
 * Simplified __ini_deleteAll.
 *
 * Revision 1.6  2001/07/21 09:47:12  s_a_white
 * Bug Fixes (thanks Andy):
 * *) After a flush the key and heading are now remembered.
 * *) ini_deleteAll then ini_close now correctly deletes the ini file.
 * *) ini_flush with no changes no longer destroys the ini object.
 *
 ***************************************************************************/

//*******************************************************************************************************************
// Include Files
//*******************************************************************************************************************
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "vice-config.h"
#include "libini-ini.h"

#define INI_BUFFER_SIZE (1024 * 5)

enum
{
    INI_NONE          = 0,
    INI_NEW_LINE      = 1,
    INI_SKIP_LINE     = 2,
    INI_IN_SECTION    = 3,
    INI_END_OF_FILE   = 4,
    INI_CHECK_COMMENT = 5,
    INI_ALLOW_COMMENT = 8
};

typedef struct
{
    long        pos;
    long        first;
    long        last;
    struct      key_tag *key;
    int         state;
    const char *comment;
    int         commentpos;
} ini_parser_t;


//*******************************************************************************************************************
// Function Prototypes
//*******************************************************************************************************************
static ini_t              *__ini_open            (const char *name, ini_mode_t mode, const char *comment);
static int                 __ini_close           (ini_t *ini, bool flush);
static void                __ini_delete          (ini_t *ini);
static bool                __ini_extractField    (ini_t *ini, FILE *file, ini_parser_t &parser, char ch);
static int                 __ini_process         (ini_t *ini, FILE *file, const char *comment);
inline bool                __ini_processComment  (ini_t *ini, FILE *file, ini_parser_t &parser);
static int                 __ini_store           (ini_t *ini, FILE *file);


#ifdef INI_USE_HASH_TABLE
static const unsigned long __ini_crc32Table[0x100] =
{
    0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA,
    0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
    0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
    0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
    0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE,
    0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
    0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC,
    0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
    0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
    0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
    0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940,
    0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
    0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116,
    0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
    0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
    0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
    0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A,
    0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
    0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818,
    0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
    0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
    0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
    0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C,
    0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
    0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2,
    0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
    0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
    0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
    0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086,
    0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
    0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4,
    0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,
    0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
    0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
    0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8,
    0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
    0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE,
    0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
    0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
    0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
    0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252,
    0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
    0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60,
    0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
    0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
    0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
    0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04,
    0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,
    0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A,
    0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
    0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
    0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
    0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E,
    0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
    0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C,
    0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
    0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
    0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
    0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0,
    0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
    0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6,
    0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
    0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
    0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D
};

/********************************************************************************************************************
 * Function          : __ini_createCrc32
 * Parameters        : init   - initial crc starting value, pBuf - data to base crc on
 *                   : length - length in bytes of data
 * Returns           :
 * Globals Used      :
 * Globals Modified  :
 * Description       : Creates a 32 bit CRC based on the input data
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
unsigned long __ini_createCrc32 (const char *pBuf, size_t length)
{
   unsigned long crc = 0xffffffff;
   for (size_t i = 0; i < length; i++)
       crc = (crc >> 8) ^ __ini_crc32Table[(crc & 0xFF) ^ (unsigned) *pBuf++];
   return (crc ^ 0xffffffff);
}
#endif // INI_USE_HASH_TABLE


/********************************************************************************************************************
 * Function          : __ini_strtrim
 * Parameters        : str - string to be trimmed
 * Returns           :
 * Globals Used      :
 * Globals Modified  :
 * Description       : Removes all char deemed to be spaces from start and end of string.
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
void __ini_strtrim (char *str)
{
    long first, last;
    first = 0;
    last  = strlen (str);

    if (!last--)
        return;

    // Clip end first
    while (isspace (str[last]) && last > 0)
        last--;
    str[last + 1] = '\0';

    // Clip beginning
    while (isspace (str[first]) && (first < last))
        first++;
    strcpy (str, str + first);
}


/********************************************************************************************************************
 * Function          : __ini_open
 * Parameters        : name - ini file to parse
 * Returns           : Pointer to ini database.
 * Globals Used      :
 * Globals Modified  :
 * Description       : Opens an ini data file and reads it's contents into a database
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
ini_t *__ini_open (const char *name, ini_mode_t mode, const char *comment)
{
    ini_t   *ini;
    FILE    *file = NULL;
    unsigned long length;

    if (!name)
        return 0;

    length = strlen(name);
    if (!length)
        return 0;

    // Create ini database stub
    ini = (ini_t *) malloc (sizeof (ini_t));
    if (!ini)
        goto ini_openError;
    memset (ini, 0, sizeof (ini_t));

    // Store ini filename
    ini->filename = strdup (name);
    if (!ini->filename)
        goto ini_openError;

    // Open input file
    ini->mode = mode;
    file = fopen (ini->filename, "rb");
    if (!file)
    {   // File doesn't exist so check if allowed 
        // to create new one 
        if (mode != INI_NEW)
            goto ini_openError;

        // Seems we can make a new one, check and
        // make sure
        file = fopen (ini->filename, "wb");
        if (!file)
            goto ini_openError;
        ini->newfile = true;
        fclose (file);
        file = NULL;
    }

    // Open backup file
    if (ini->mode == INI_READ)
        ini->ftmp = tmpfile ();
    else
    {
        ini->filename[length - 1] = '~';
        ini->ftmp = fopen (ini->filename, "wb+");
        ini->filename[length - 1] = name[length - 1];
    }

    if (!ini->ftmp)
        goto ini_openError;
    if (file)
    {   // Process existing ini file
        if (__ini_process (ini, file, comment) < 0)
            goto ini_openError;
        fclose (file);
    }

    // Rev 1.1 Added - Changed set on open bug fix
    ini->changed = false;
return ini;

ini_openError:
    if (ini)
    {
        if (ini->ftmp)
        {   // Close and remove backup file
            fclose (ini->ftmp);
            if (ini->mode != INI_READ)
            {
                ini->filename[strlen (ini->filename) - 1] = '~';
                remove (ini->filename);
            }
        }
        if (ini->filename)
            free (ini->filename);
        free (ini);
    }

    if (file)
        fclose (file);

    return 0;
}


/********************************************************************************************************************
 * Function          : __ini_close
 * Parameters        : ini - pointer to ini file database.  force - if true, the database will be removed from
 *                   : memory even if an error occured in saving the new ini file.
 * Returns           : -1 on error, 0 on success
 * Globals Used      :
 * Globals Modified  :
 * Description       : Save any changes back to the new ini file.
 *                   : The backup file contains all the orignal data + any modifcations appended at the bottom
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
int __ini_close (ini_t *ini, bool flush)
{
    FILE *file;
    int   ret = 0;

    // Open output file
    if (ini->changed)
    {
        if (!ini->first)
            remove(ini->filename);
        else
        {
#ifdef INI_ADD_LIST_SUPPORT
            char *delims;
            // Rev 1.1 Added - Must remove delims before saving
            delims = ini->listDelims;
            ini->listDelims = NULL;
#endif // INI_ADD_LIST_SUPPORT

            // Not point writing an unchanged file
            file = fopen (ini->filename, "w");
            if (file)
            {   // Output all new headers and keys
                ret = __ini_store (ini, file);
                fflush (file);
                fclose (file);
            }

#ifdef INI_ADD_LIST_SUPPORT
            // Rev 1.1 Added - This was only a flush, so lets restore
            // the old delims
            ini->listDelims = delims;
#endif // INI_ADD_LIST_SUPPORT
            if (!file)
                return -1;
        }
    }

    // Check if the user dosent want the file closed.
    if (!flush)
        return 0;

    // Cleanup
    fclose (ini->ftmp);

    if (ini->mode != INI_READ)
    {   // If no mods were made, delete tmp file
        if (!ini->changed || ini->newfile)
        {
            ini->filename[strlen (ini->filename) - 1] = '~';
            remove (ini->filename);
        }
    }

    __ini_delete (ini);
    free (ini->filename);

    if (ini->tmpSection.heading)
        free (ini->tmpSection.heading);
    if (ini->tmpKey.key)
        free (ini->tmpKey.key);

#ifdef INI_ADD_LIST_SUPPORT
    // Rev 1.1 - Remove buffered list
    if (ini->list)
        free (ini->list);
#endif // INI_ADD_LIST_SUPPORT

    free (ini);
    return ret;
}


/********************************************************************************************************************
 * Function          : __ini_delete
 * Parameters        : ini - pointer to ini file database.
 * Returns           :
 * Globals Used      :
 * Globals Modified  :
 * Description       : Deletes the whole ini database from memory, but leaves the ini stub so the ini_close can be
 *                   : called.
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
void __ini_delete (ini_t *ini)
{   // If already deleted, don't delete it again
    if (!ini->first)
        return;

    // Go through all sections deleting them
    while (ini->first)
    {
         ini->selected = ini->first;
         __ini_deleteHeading (ini);
    }

#ifdef INI_ADD_LIST_SUPPORT
    // Rev 1.1 - Remove buffered list
    if (ini->list)
    {
        free (ini->list);
        ini->list = NULL;
    }
#endif // INI_ADD_LIST_SUPPORT

    ini->changed = true;
}


/********************************************************************************************************************
 * Function          : __ini_extractField
 * Parameters        : ini    - pointer to ini file database,  file - ini file to read heading from
 *                   : parser - current parser decoding state, ch   - character to process
 * Returns           : false on error and true on success
 * Globals Used      :
 * Globals Modified  :
 * Description       : Process next character and extract fields as necessary
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
bool __ini_extractField (ini_t *ini, FILE *file, ini_parser_t &parser, char ch)
{
    switch (ch)
    {   // Check for key value
    case '=':
        if (parser.state != INI_IN_SECTION)
        {   // Make sure the key has a string content
            parser.last = parser.pos;
            if (parser.first >= 0)
            {
                if (!ini->selected) // Handle keys which are not in a section
                {                                   
                    if (!__ini_faddHeading (ini, file, 0, 0))
                        return false;
                }

                parser.key = __ini_faddKey (ini, file, parser.first,
                                            parser.last - parser.first);
                if (!parser.key)
                    return false;
            }
            parser.state = INI_CHECK_COMMENT | INI_ALLOW_COMMENT;
        }
        break;

    // Check for header (must start far left)
    case '[':
        if (parser.state == INI_NEW_LINE)
        {
            parser.first = parser.pos + 1;
            parser.state = INI_IN_SECTION;
        }
        break;

    // Check for header termination
    case ']':
        if (parser.state == INI_IN_SECTION)
        {
            parser.last = parser.pos;
            if (parser.first <= parser.last) // Handle []
            {
                if (!__ini_faddHeading (ini, file, parser.first,
                                        parser.last - parser.first))
                {
                    return false;
                }
            }
            parser.state = INI_SKIP_LINE;
        }
        break;

    default:
        if (parser.state == INI_NEW_LINE)
        {
            parser.first = parser.pos;
            parser.state = INI_NONE;
        }
        break;
    }
    return true;
}


/********************************************************************************************************************
 * Function          : __ini_processComment
 * Parameters        : ini    - pointer to ini file database,  file - ini file to read heading from
 *                   : parser - current parser decoding state
 * Returns           : false on error and true on success
 * Globals Used      :
 * Globals Modified  :
 * Description       : Comment strings are not passed to ini_extractField.  Since multi char comments are now
 *                   : supported we need re-process it should it eventually turn out not to be a comment.
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
bool __ini_processComment (ini_t *ini, FILE *file, ini_parser_t &parser)
{
    const char *p = parser.comment;
    for (; parser.commentpos > 0; parser.commentpos--)
    {
        if (!__ini_extractField (ini, file, parser, *p++))
            return false;
        parser.pos++;
    }
    return true;
}


/********************************************************************************************************************
 * Function          : __ini_process
 * Parameters        : ini - pointer to ini file database,  file - ini file to read heading from
 * Returns           : -1 on error and 0 on success
 * Globals Used      :
 * Globals Modified  :
 * Description       : Read the ini file to determine all valid sections and keys.  Also stores the location of
 *                   : the keys data for faster accessing.
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
int __ini_process (ini_t *ini, FILE *file, const char *comment)
{
    char  *current, ch;
    size_t count;
    char  *buffer;
    ini_parser_t parser;
    int    pos = 0;

    if (!ini)
        return -1;
    if (!file)
        return -1;

    // Get a read buffer
    buffer = (char *) malloc (INI_BUFFER_SIZE * sizeof(char));
    if (buffer == NULL)
        return -1;

    // Clear out an existing ini structure
    __ini_delete (ini);
    parser.pos        = pos = 0;
    parser.first      = -1;
    parser.last       = -1;
    parser.state      = INI_NEW_LINE | INI_ALLOW_COMMENT;
    parser.key        = NULL;
    parser.comment    = comment;
    parser.commentpos = 0;

    do
    {
        fseek (file, pos, SEEK_SET);
        current = buffer;
        count   = fread (buffer, sizeof(char),
                         INI_BUFFER_SIZE, file);

        if (count <= 0)
        {
            if (feof (file))
            {
                count  = 1;
               *buffer = '\x1A';
            }
        }

        while (count--)
        {
            ch = *current++;
            switch (ch)
            {
            // Check for end of file
            case '\x1A':
                parser.state = INI_END_OF_FILE;
                count        = 0;
            goto __ini_processLineEnd;

            // Check for newline
            case '\n': case '\r': case '\f':
                parser.state = INI_NEW_LINE | INI_ALLOW_COMMENT;
                parser.first = -1;
                parser.last  = -1;
            __ini_processLineEnd:
                if (!__ini_processComment (ini, file, parser))
                    goto __ini_processError;
            __ini_processDataEnd:
                // Now know keys data length
                if (parser.key)
                {
                    parser.key->length = (size_t) (pos - parser.key->pos);
                    parser.key         = NULL;
                }
                break;

            default:
                switch (parser.state & ~INI_ALLOW_COMMENT)
                {
                case INI_SKIP_LINE:
                    break;
                //case INI_NONE:
                //case INI_IN_SECTION:
                case INI_NEW_LINE:
                case INI_CHECK_COMMENT:
                    // Check to see if comments are allowed
                    if (isspace (ch))
                    {
                        parser.commentpos = 0;
                        parser.state |= INI_ALLOW_COMMENT;
                        parser.pos    = pos;
                        break;
                    }
                    // Deliberate run on
                default:
                    // Check for a comment
                    if (parser.state & INI_ALLOW_COMMENT)
                    {
                        if (ch == parser.comment[parser.commentpos])
                        {
                            parser.commentpos++;
                            if (parser.comment[parser.commentpos] == '\0')
                            {
                                parser.commentpos = 0;
                                parser.state      = INI_SKIP_LINE;
                                goto __ini_processDataEnd;
                            }
                            break;
                        }

                        parser.state &= ~INI_ALLOW_COMMENT;
                    }

                    if (parser.state != INI_CHECK_COMMENT)
                    {
                        if (!__ini_processComment (ini, file, parser))
                            goto __ini_processError;
                        parser.pos = pos;
                        if (!__ini_extractField (ini, file, parser, ch))
                            goto __ini_processError;
                    }
                }
                break;
            }

            // Rev 1.1 Added - Exit if EOF
            if (parser.state == INI_END_OF_FILE)
                break;

            fputc (ch, ini->ftmp);
            pos++;
            if (!pos)
            {
                printf ("INI file is too large\n");
                __ini_delete (ini);
                return -1;
            }
        }
    } while (parser.state != INI_END_OF_FILE);

    // Don't leave a random key located.  This
    // also forces user to call ini_locate*
    ini->selected = NULL;
    free (buffer);
    return 0;

__ini_processError:
    free (buffer);
    __ini_delete (ini);
    return -1;
}


/********************************************************************************************************************
 * Function          : __ini_store
 * Parameters        : ini - pointer to ini file database,  file - ini file to read heading from
 * Returns           : -1 on error and 0 on success
 * Globals Used      :
 * Globals Modified  :
 * Description       : Writes a new ini file containing all the necessary changes
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
int __ini_store (ini_t *ini, FILE *file)
{
    struct section_tag *current_h, *selected_h;
    struct key_tag     *current_k, *selected_k = NULL;
    char  *str = NULL;
    size_t length = 0, equal_pos = 0;
    int    ret    = -1;

    if (!ini)
        return -1;
    if (!file)
        return -1;
    
    // Backup selected heading and key
    selected_h = ini->selected;
    // Be carefull if nothing was previously selected
    if (selected_h != NULL)
        selected_k  = selected_h->selected;

    current_h = ini->first;
    while (current_h)
    {
        // Output section heading
        if (*current_h->heading)
        {
            if (fprintf (file, "[%s]\n", current_h->heading) < 0)
                goto __ini_storeError;
        }
        
        // Output the sections keys
        equal_pos = __ini_averageLengthKey (current_h);
        current_k = current_h->first;
        while (current_k)
        {
            if (((current_k->length + 1) > length) || !str)
            {   // Need more space
                if (str)
                    free (str);
                length = current_k->length + 1;
                str    = (char *) malloc (sizeof(char) * length);
                if (!str)
                    goto __ini_storeError;
            }

            {   // Output key
                char format[10];
                // Rev 1.1 Added - to support lining up of equals characters
                sprintf (format, "%%-%lus=", (unsigned long) equal_pos);
                if (fprintf (file, format, current_k->key) < 0)
                    goto __ini_storeError;
            }

            // Output keys data (point to correct keys data)
            ini->selected       = current_h;
            current_h->selected = current_k;
            if (ini_readString ((ini_fd_t) ini, str, length) < 0)
                goto __ini_storeError;

            if (fprintf (file, "%s\n", str) < 0)
                goto __ini_storeError;

            current_k = current_k->pNext;
        }

        current_h = current_h->pNext;
        if (fprintf (file, "\n") < 0)
            goto __ini_storeError;
    }
    ret = 0;

__ini_storeError:
    if (str)
        free (str);
    // Restore selected heading and key
    ini->selected   = selected_h;
    if (selected_h != NULL)
        selected_h->selected = selected_k;
    return ret;
}



//********************************************************************************************************************
//********************************************************************************************************************
// User INI File Manipulation Functions
//********************************************************************************************************************
//********************************************************************************************************************


/********************************************************************************************************************
 * Function          : ini_open
 * Parameters        : name - ini file to create
 * Returns           : Pointer to ini database.
 * Globals Used      :
 * Globals Modified  :
 * Description       : Opens an ini data file and reads it's contents into a database
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
ini_fd_t INI_LINKAGE ini_open (const char *name, const char *mode,
                               const char *comment)
{
    ini_mode_t _mode;
    if (!mode)
        return NULL;
    // Convert mode
    switch (*mode)
    {
    case 'r': _mode = INI_READ;  break;
    case 'w': _mode = INI_NEW;   break;
    case 'a': _mode = INI_EXIST; break;
    default: return NULL;
    }
    // NULL can also be used to disable comments
    if (comment == NULL)
        comment = "";
    return (ini_fd_t) __ini_open (name, _mode, comment);
}


/********************************************************************************************************************
 * Function          : ini_close
 * Parameters        : ini - pointer to ini file database.
 * Returns           : -1 on error, 0 on success
 * Globals Used      :
 * Globals Modified  :
 * Description       : Call close, but make sure ini object IS deleted
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
int INI_LINKAGE ini_close (ini_fd_t fd)
{
    return __ini_close ((ini_t *) fd, true);
}


/********************************************************************************************************************
 * Function          : ini_flush
 * Parameters        : ini - pointer to ini file database.
 * Returns           : -1 on error, 0 on success
 * Globals Used      :
 * Globals Modified  :
 * Description       : Call close, but make sure ini object IS NOT deleted
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
int INI_LINKAGE ini_flush (ini_fd_t fd)
{
    return __ini_close ((ini_t *) fd, false);
}


/********************************************************************************************************************
 * Function          : ini_dataLength
 * Parameters        : ini - pointer to ini file database.  heading - heading name.  key - key name
 * Returns           : Number of bytes to read the keys data in as a string.  1 must be added to this length
 *                   : to cater for a NULL character.
 * Globals Used      :
 * Globals Modified  :
 * Description       : Number of bytes to read the keys data in as a string
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
int INI_LINKAGE ini_dataLength (ini_fd_t fd)
{
    ini_t *ini = (ini_t *) fd;
    struct key_tag *_key;
    if (!ini)
        return -1;

    // Check to make sure a section/key has
    // been asked for by the user
    if (!ini->selected)
        return -1;
    _key = ini->selected->selected;
    if (!_key)
        return -1;

#ifdef INI_ADD_LIST_SUPPORT
    if (ini->listDelims)
        return __ini_listIndexLength (ini);
#endif
    return (int) _key->length;
}


/********************************************************************************************************************
 * Function          : ini_delete
 * Parameters        : ini - pointer to ini file database.
 * Returns           :
 * Globals Used      :
 * Globals Modified  :
 * Description       : Deletes the whole ini database from memory, but leaves the ini stub so the ini_close can be
 *                   : called.
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
extern "C" int INI_LINKAGE ini_delete (ini_fd_t fd)
{
    ini_t *ini = (ini_t *) fd;
    if (!ini)
      return -1;
    __ini_delete (ini);
    return 0;
}


#ifdef INI_ADD_EXTRAS

/********************************************************************************************************************
 * Function          : ini_append
 * Parameters        : fdsrc - pointer to src ini file database to copy from.
 *                   : fddst - pointer to dst ini file database to copy to.
 * Returns           :
 * Globals Used      :
 * Globals Modified  :
 * Description       : Copies the contents of the src ini to the dst ini.  The resulting ini contains both
 *                   : headings and keys from each.  Src keys will overwrite dst keys or similar names.
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
extern "C" int INI_LINKAGE ini_append (ini_fd_t fddst, ini_fd_t fdsrc)
{
    struct section_tag *current_h;
    struct key_tag     *current_k;
    struct section_tag *src_h, *dst_h;
    struct key_tag     *src_k = NULL, *dst_k = NULL;
    char  *data   = NULL;
    int    length = 0, ret = -1;

#ifdef INI_ADD_LIST_SUPPORT
    char  *delims;
#endif

    ini_t *src = (ini_t *) fdsrc;
    ini_t *dst = (ini_t *) fddst;
    if (!(src && dst))
      return -1;

    // Backup selected heading and key
    src_h = src->selected;
    dst_h = dst->selected;
    // Be carefull if nothing was previously selected
    if (src_h != NULL)
        src_k  = src_h->selected;
    if (dst_k != NULL)
        dst_k  = dst_h->selected;

#ifdef INI_ADD_LIST_SUPPORT
    // Remove delims for proper reads
    delims = src->listDelims;
    src->listDelims = NULL;
#endif

    // Go through the src ini headings
    current_h = src->first;
    while (current_h)
    {   // Locate heading in the dst
        ini_locateHeading (dst, current_h->heading);
        // Go through the src keys under the heading
        src->selected = current_h;
        current_k = current_h->first;
        while (current_k)
        {   // Check if data buffer can hold the key
            int i = current_k->length;
            current_h->selected = current_k;
            if (i > length)
            {   // Make data buffer bigger, with some spare
                length = i + 10;
                if (data != NULL)
                    free (data);
                data = (char *) malloc (sizeof (char) * length);
                if (data == NULL)
                    goto ini_appendError;
            }
            // Locate key in dst ini file
            ini_locateKey (dst, current_k->key);
            // Copy the key from src to dst ini file
            if (ini_readString  (src, data, length) != i)
                goto ini_appendError;
            if (ini_writeString (dst, data) < 0)
                goto ini_appendError;
            // Move to next key
            current_k = current_k->pNext;
        }
        // Move to next heading
        current_h = current_h->pNext;
    }
    ret = 0;

ini_appendError:
    if (data != NULL)
        free (data);

#ifdef INI_ADD_LIST_SUPPORT
    // Restore delims
    src->listDelims = delims;
#endif
    // Restore selected headings and keys
    src->selected = src_h;
    dst->selected = dst_h;
    if (src_h != NULL)
        src_h->selected = src_k;
    if (dst_h != NULL)
        dst_h->selected = dst_k;
    return ret;
}

#endif // INI_ADD_EXTRAS


// Add Code Modules
// Add Header Manipulation Functions
/***************************************************************************
 headings.i - Section Heading Manipulation Functions
 
 -------------------
 begin                : Fri Apr 21 2000
 copyright            : (C) 2000 by Simon White
 email                : s_a_white@email.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libini-headings.h"


/********************************************************************************************************************
 * Function          : __ini_addHeading
 * Parameters        : ini - pointer to ini file database.  heading - heading name
 * Returns           : Pointer to new heading.
 * Globals Used      :
 * Globals Modified  :
 * Description       : Adds a new heading to the ini file database and updates the temporary workfile
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
struct section_tag *__ini_addHeading (ini_t *ini, char *heading)
{
	struct section_tag *section;
	size_t length;
	long   pos;
	
	// Format heading for storing
	__ini_strtrim (heading);
	
	/* Create a backup of the file we are about to edit */
	if (ini->heading == heading)
		return __ini_locateHeading (ini, heading);
	
	// Add new heading to work file and read it in
	// using file add
	fseek (ini->ftmp, 0, SEEK_END);
	fputs ("\n[", ini->ftmp);
	pos = ftell (ini->ftmp);
	fputs (heading, ini->ftmp);
	length  = (size_t) (ftell (ini->ftmp) - pos);
	section = __ini_faddHeading (ini, ini->ftmp, pos, length);
	fseek (ini->ftmp, 0, SEEK_END);
	fputs ("]\n", ini->ftmp);
	ini->heading = section->heading;
	return section;
}


/********************************************************************************************************************
 * Function          : __ini_faddheading
 * Parameters        : ini - pointer to ini file database,  file - ini file to read heading from
 *                   : pos - heading position in file, length - heading length
 * Returns           : Pointer to new heading.
 * Globals Used      :
 * Globals Modified  :
 * Description       : Adds a new heading to the ini file database from the input file.
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
struct section_tag *__ini_faddHeading (ini_t *ini, FILE *file, long pos, size_t length)
{
	struct section_tag *section;
	char  *str;
	
	if (length)
	{
		length++;
		str = (char *) malloc (sizeof(char) * length);
		assert (str);
		fseek  (file, pos, SEEK_SET);
		fgets  (str, (int) length, file);
		__ini_strtrim (str);
	}
	else
		str = "";
	
	section = __ini_createHeading (ini, str);
	// Make sure heading was created
	if (!section && length)
	{
		free (str);
		return NULL;
	}
	
	return section;
}


/********************************************************************************************************************
 * Function          : __ini_createHeading
 * Parameters        : ini - pointer to ini file database.  heading - heading name
 * Returns           : Pointer to new heading.
 * Globals Used      :
 * Globals Modified  :
 * Description       : Adds an entry into the heading linked list ready for formating by the addHeading commands
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
struct section_tag *__ini_createHeading (ini_t *ini, char *heading)
{
	struct section_tag *pNew;
	
	pNew  = __ini_locateHeading (ini, heading);
	// Check to see if heading already exists
	if (pNew)
		free (heading);
	else
	{   // Create a new heading as dosen't exist
		pNew = (struct section_tag *) malloc (sizeof (struct section_tag));
		if (!pNew)
			return NULL;
		memset (pNew, 0, sizeof (struct section_tag));
		pNew->heading = heading;
		
		if (*heading)
		{   // Found a named heading
			pNew->pPrev = ini->last;
			ini->last   = pNew;
			if (pNew->pPrev)
				pNew->pPrev->pNext = pNew;
			else
				ini->first = pNew;
		}
		else
		{   // Anonymous heading (special case),
			// always comes first
			pNew->pNext = ini->first;
			ini->first  = pNew;
			if (pNew->pNext)
				pNew->pNext->pPrev = pNew;
			else
				ini->last = pNew;
		}
		
#ifdef INI_USE_HASH_TABLE
		{   // Rev 1.3 - Added
			struct   section_tag *pOld;
			unsigned long crc32;
			unsigned char accel;
			
			crc32     = __ini_createCrc32 (heading, strlen (heading));
			pNew->crc = crc32;
			// Rev 1.3 - Add accelerator list
			accel = (unsigned char) crc32 & 0x0FF;
			pNew->pPrev_Acc = NULL;
			pOld = ini->sections[accel];
			ini->sections[accel]      = pNew;
			if (pOld) pOld->pPrev_Acc = pNew;
			pNew->pNext_Acc           = pOld;
		}
#endif // INI_USE_HASH_TABLE
	}
	
	ini->selected = pNew;
	ini->changed  = true;
	return pNew;
}


/********************************************************************************************************************
 * Function          : __ini_deleteHeading
 * Parameters        : ini - pointer to ini file database.  heading - heading name
 * Returns           :
 * Globals Used      :
 * Globals Modified  :
 * Description       : Removes a heading from the database only.
 *                   : This change does not occur in the file until ini_close is called.
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
void __ini_deleteHeading (ini_t *ini)
{
	struct section_tag *current_h;
	
	// Delete Heading
	current_h = ini->selected;
	if (current_h)
	{   // Delete Keys
		while (current_h->first)
		{
			current_h->selected = current_h->first;
			__ini_deleteKey (ini);
		}
		
		// Tidy up all users of this heading
		ini->selected =  NULL;
		if (ini->last == current_h)
			ini->last =  current_h->pPrev;
		
		// Break heading out of list
		if (!current_h->pPrev)
			ini->first = current_h->pNext;
		else
			current_h->pPrev->pNext = current_h->pNext;
		if (current_h->pNext)
			current_h->pNext->pPrev = current_h->pPrev;
		
#ifdef INI_USE_HASH_TABLE
		// Rev 1.3 - Take member out of accelerator list
		if (!current_h->pPrev_Acc)
			ini->sections[(unsigned char) current_h->crc & 0x0FF] = current_h->pNext_Acc;
		else
			current_h->pPrev_Acc->pNext_Acc = current_h->pNext_Acc;
		if (current_h->pNext_Acc)
			current_h->pNext_Acc->pPrev_Acc = current_h->pPrev_Acc;
#endif // INI_USE_HASH_TABLE
		
		// Delete Heading
		if (*current_h->heading)
			free (current_h->heading);
		free (current_h);
		ini->changed = true;
	}
}


/********************************************************************************************************************
 * Function          : __ini_locateHeading
 * Parameters        : ini - pointer to ini file database.  heading - heading name
 * Returns           : Pointer to heading.
 * Globals Used      :
 * Globals Modified  :
 * Description       : Locates a heading entry in the database.
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
struct section_tag *__ini_locateHeading (ini_t *ini, const char *heading)
{
	struct   section_tag *current_h;
	
#ifdef INI_USE_HASH_TABLE
	// Rev 1.3 - Revised to use new accelerator
	unsigned long crc32;
	crc32 = __ini_createCrc32 (heading, strlen (heading));
	
	// Search for heading
	for (current_h = ini->sections[(unsigned char) crc32 & 0x0FF]; current_h; current_h = current_h->pNext_Acc)
	{
		if (current_h->crc == crc32)
		{
			if (!strcmp (current_h->heading, heading))
				break;
		}
	}
#else
	// Search for heading
	for (current_h = ini->first; current_h; current_h = current_h->pNext)
	{
		if (!strcmp (current_h->heading, heading))
			break;
	}
#endif // INI_USE_HASH_TABLE
	
	ini->selected = current_h;
	return current_h;
}


/********************************************************************************************************************
 * Function          : (public) ini_deleteHeading
 * Parameters        : ini - pointer to ini file descriptor
 * Returns           : -1 for Error and 0 on success
 * Globals Used      :
 * Globals Modified  :
 * Description       : Equivalent Microsoft write string API call where both data & key are set to NULL.
 ********************************************************************************************************************/
int INI_LINKAGE ini_deleteHeading (ini_fd_t fd)
{
	ini_t *ini = (ini_t *) fd;
	if (!ini->selected)
		return -1;
	// Can't delete a temporary section
	if (ini->selected == &(ini->tmpSection))
		return -1;
	__ini_deleteHeading (ini);
	return 0;
}


/********************************************************************************************************************
 * Function          : (public) ini_locateHeading
 * Parameters        : fd - pointer to ini file descriptor
 * Returns           : -1 for Error and 0 on success
 * Globals Used      :
 * Globals Modified  :
 * Description       : Equivalent Microsoft write string API call where both data & key are set to NULL.
 ********************************************************************************************************************/
int INI_LINKAGE ini_locateHeading (ini_fd_t fd, const char *heading)
{
	ini_t *ini = (ini_t *) fd;
	struct section_tag *section;
	
	if (!heading)
		return -1;
	
	section = __ini_locateHeading (ini, heading);
	
#ifdef INI_ADD_LIST_SUPPORT
	// Rev 1.1 - Remove buffered list
	if (ini->list)
	{
		free (ini->list);
		ini->list = NULL;
	}
#endif // INI_ADD_LIST_SUPPORT
	
	if (section)
	{
		section->selected = NULL;
		return 0;
	}
	
	// Ok no section was found, but maybe the user is wanting to create a
	// new one so create it temporarily and see what actually happens later
	{   // Remove old heading
		section = &(ini->tmpSection);
		if (section->heading)
			free (section->heading);
		
		// Add new heading
		section->heading  = strdup (heading);
		if (!section->heading)
			return -1;
		section->selected = NULL;
		ini->selected     = section;
	}
	return -1;
}

// Add Key Manipulation Functions
/***************************************************************************
 keys.i - Key Manipulation Functions
 
 -------------------
 begin                : Fri Apr 21 2000
 copyright            : (C) 2000 by Simon White
 email                : s_a_white@email.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libini-keys.h"

#define INI_EQUALS_ALIGN 10


/********************************************************************************************************************
 * Function          : __ini_addKey
 * Parameters        : ini - pointer to ini file database.  key - key name
 * Returns           : Pointer to key.
 * Globals Used      :
 * Globals Modified  :
 * Description       : Adds a new key to the ini file database and updates the temporary workfile.
 *                   : A heading operation must be called before this function.
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
struct key_tag *__ini_addKey (ini_t *ini, char *key)
{
	struct key_tag *_key;
	size_t length;
	long   pos;
	
	// Format heading for storing
	__ini_strtrim (key);
	if (!*key)
		return NULL;
	
	// Add new key to work file and read it in
	// using file add
	fseek (ini->ftmp, 0, SEEK_END);
	pos = ftell (ini->ftmp);
	fputs (key, ini->ftmp);
	length = (size_t) (ftell (ini->ftmp) - pos);
	_key   = __ini_faddKey (ini, ini->ftmp, pos, length);
	fseek (ini->ftmp, 0, SEEK_END);
	fputc ('=', ini->ftmp);
	return _key;
}


/********************************************************************************************************************
 * Function          : __ini_averageLengthKey
 * Parameters        : current_h - pointer to current header.
 * Returns           : Returns the average key length
 * Globals Used      :
 * Globals Modified  :
 * Description       : Finds average key length for aligning equals.
 ********************************************************************************************************************/
size_t __ini_averageLengthKey (struct section_tag *current_h)
{
#ifdef INI_EQUALS_ALIGN
	size_t equal_pos, equal_max, keylength;
	size_t average = 0, count = 0;
	struct key_tag *current_k;
	
	// Rev 1.1 Added - Line up equals characters for keys
	// Calculate Average
	current_k = current_h->first;
	while (current_k)
	{
		count++;
		average  += strlen (current_k->key);
		current_k = current_k->pNext;
	}
	
	if (!count)
		return 0;
	
	average  /= count;
	equal_pos = (equal_max = average);
	
#if INI_EQUALS_ALIGN > 0
	// Work out the longest key in that range
	current_k = current_h->first;
	while (current_k)
	{
		keylength = strlen (current_k->key);
		equal_max = average + INI_EQUALS_ALIGN;
		
		if ((equal_max > keylength) && (keylength > equal_pos))
			equal_pos = keylength;
		current_k = current_k->pNext;
	}
#endif // INI_EQUALS_ALIGN > 0
	return equal_pos;
#else
	return 0;
#endif // INI_EQUALS_ALIGN
}


/********************************************************************************************************************
 * Function          : __ini_faddKey
 * Parameters        : ini - pointer to ini file database,  file - ini file to read key from
 *                   : pos - key position in file, length - key length
 * Returns           : Pointer to key.
 * Globals Used      :
 * Globals Modified  :
 * Description       : Adds a new key to the ini file database from the input file.
 *                   : A heading operation must be called before this function.
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
struct key_tag *__ini_faddKey (ini_t *ini, FILE *file, long pos, size_t length)
{
	struct key_tag *_key;
	char  *str;
	
	length++;
	str = (char *) malloc (sizeof(char) * length);
	assert (str);
	fseek  (file, pos, SEEK_SET);
	fgets  (str, (int) length, file);
	__ini_strtrim (str);
	
	_key = __ini_createKey (ini, str);
	if (!_key)
	{   free (str);
		return NULL;
	}
	
	_key->pos = pos + (long) length;
	return _key;
}


/********************************************************************************************************************
 * Function          : __ini_createKey
 * Parameters        : ini - pointer to ini file database.  key - key name
 * Returns           : Pointer to key.
 * Globals Used      :
 * Globals Modified  :
 * Description       : Adds an entry into the key linked list ready for formating by the addKey commands
 *                   : A heading operation must be called before this function.
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
struct key_tag *__ini_createKey (ini_t *ini, char *key)
{
	struct section_tag *section;
	struct key_tag     *pNew;
	long   pos;
	
	if (!*key)
		return NULL;
	
	section = ini->selected;
	pNew    = __ini_locateKey (ini, key);
	if (pNew)
	{   // Reset details of existing key
		free (pNew->key);
		pNew->key = key;
		pos       = 0;
	}
	else
	{   // Create a new key and add at end;
		pNew = (struct key_tag *) malloc (sizeof (struct key_tag));
		if (!pNew)
			return NULL;
		memset (pNew, 0, sizeof (struct key_tag));
		pNew->key = key;
		
		if (!section->first)
			section->first = pNew;
		else
			section->last->pNext = pNew;
		
		pNew->pPrev       = section->last;
		section->last     = pNew;
		section->selected = pNew;
		
#ifdef INI_USE_HASH_TABLE
		{   // Rev 1.3 - Added
			struct   key_tag *pOld;
			unsigned long crc32;
			unsigned char accel;
			
			crc32     = __ini_createCrc32 (key, strlen (key));
			pNew->crc = crc32;
			// Rev 1.3 - Add accelerator list
			accel = (unsigned char) crc32 & 0x0FF;
			pNew->pPrev_Acc = NULL;
			pOld = section->keys[accel];
			section->keys[accel]      = pNew;
			if (pOld) pOld->pPrev_Acc = pNew;
			pNew->pNext_Acc           = pOld;
		}
#endif
	}
	
	section->selected = pNew;
	ini->changed      = true;
	return pNew;
}


/********************************************************************************************************************
 * Function          : __ini_deleteKey
 * Parameters        : ini - pointer to ini file database.  key - key name
 * Returns           :
 * Globals Used      :
 * Globals Modified  :
 * Description       : Removes a key from the database only.
 *                   : This change does not occur in the file until ini_close is called.
 *                   : A heading operation must be called before this function.
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
void __ini_deleteKey (ini_t *ini)
{
	struct key_tag     *current_k;
	struct section_tag *section = ini->selected;
	
	current_k = section->selected;
	if (current_k)
	{
		// Tidy up all users of this key
		section->selected =  NULL;
		if (section->last == current_k)
			section->last =  current_k->pPrev;
		
		// Check to see if all keys were removed
		if (!current_k->pPrev)
			section->first = current_k->pNext;
		else
			current_k->pPrev->pNext = current_k->pNext;
		if (current_k->pNext)
			current_k->pNext->pPrev = current_k->pPrev;
		
#ifdef INI_USE_HASH_TABLE
		// Rev 1.3 - Take member out of accelerator list
		if (!current_k->pPrev_Acc)
			section->keys[(unsigned char) current_k->crc & 0x0FF] = current_k->pNext_Acc;
		else
			current_k->pPrev_Acc->pNext_Acc = current_k->pNext_Acc;
		if (current_k->pNext_Acc)
			current_k->pNext_Acc->pPrev_Acc = current_k->pPrev_Acc;
#endif // INI_USE_HASH_TABLE
		
		// Delete Key
		free (current_k->key);
		free (current_k);
		ini->changed = true;
	}
}


/********************************************************************************************************************
 * Function          : __ini_locateKey
 * Parameters        : ini - pointer to ini file database.  key - key name
 * Returns           : Pointer to key.
 * Globals Used      :
 * Globals Modified  :
 * Description       : Locates a key entry in the database.
 *                   : A heading operation must be called before this function.
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
struct key_tag *__ini_locateKey (ini_t *ini, const char *key)
{
	struct key_tag     *current_k;
	struct section_tag *section;
	section = ini->selected;
	
#ifdef INI_USE_HASH_TABLE
	{   // Rev 1.3 - Added
		unsigned long crc32;
		crc32 = __ini_createCrc32 (key, strlen (key));
		
		// Search for key
		for (current_k = section->keys[(unsigned char) crc32 & 0x0FF]; current_k; current_k = current_k->pNext_Acc)
		{
			if (current_k->crc == crc32)
			{
				if (!strcmp (current_k->key, key))
					break;
			}
		}
	}
#else
	{   // Search for key
		for (current_k = section->first; current_k; current_k = current_k->pNext)
		{
			if (!strcmp (current_k->key, key))
				break;
		}
	}
#endif // INI_USE_HASH_TABLE
	
	section->selected = current_k;
	return current_k;
}


/********************************************************************************************************************
 * Function          : (public) ini_deleteKey
 * Parameters        : ini - pointer to ini file database.  heading - heading name.  key - key name.
 * Returns           : -1 for Error and 0 on success
 * Globals Used      :
 * Globals Modified  :
 * Description       : Equivalent Microsoft write string API call where data set to NULL
 ********************************************************************************************************************/
int INI_LINKAGE ini_deleteKey (ini_fd_t fd)
{
	ini_t *ini = (ini_t *) fd;
	if (!ini->selected)
		return -1;
	// Can't delete a temporary key
	if (ini->selected->selected == &(ini->tmpKey))
		return -1;
	
	__ini_deleteKey (ini);
	return 0;
}


/********************************************************************************************************************
 * Function          : (public) ini_locateKey
 * Parameters        : fd - pointer to ini file descriptor
 * Returns           : -1 for Error and 0 on success
 * Globals Used      :
 * Globals Modified  :
 * Description       :
 ********************************************************************************************************************/
int INI_LINKAGE ini_locateKey (ini_fd_t fd, const char *key)
{
	ini_t *ini = (ini_t *) fd;
	struct key_tag *_key = NULL;
	
	if (!key)
		return -1;
	if (!ini->selected)
		return -1;
	
	// Can't search for a key in a temporary heading
	if (ini->selected != &(ini->tmpSection))
		_key = __ini_locateKey (ini, key);
	
#ifdef INI_ADD_LIST_SUPPORT
	// Rev 1.1 - Remove buffered list
	if (ini->list)
	{
		free (ini->list);
		ini->list = NULL;
	}
#endif // INI_ADD_LIST_SUPPORT
	
	if (_key)
		return 0;
	
	// Ok no key was found, but maybe the user is wanting to create a
	// new one so create it temporarily and see what actually happens later
	{   // Remove all key
		_key = &(ini->tmpKey);
		if (_key->key)
			free (_key->key);
		
		// Add new key
		_key->key = strdup (key);
		if (!_key)
			return -1;
		ini->selected->selected = _key;
	}
	return -1;
}
// Add Supported Datatypes
/***************************************************************************
 types.i - Libini supported data types
 Use readString for others
 
 -------------------
 begin                : Fri Apr 21 2000
 copyright            : (C) 2000 by Simon White
 email                : s_a_white@email.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libini-ini.h"


/********************************************************************************************************************
 * Function          : __ini_read
 * Parameters        : ini - pointer to ini file database, size - key length returned here
 * Returns           : -1 for Error and 0 on success.
 * Globals Used      :
 * Globals Modified  :
 * Description       : Common read functionality
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
static int __ini_read (ini_t *ini, size_t *size)
{
	struct key_tag *_key;
	
	if (!ini->selected)
		return -1;
	// Locate and read keys value
	_key = ini->selected->selected;
	if (!_key)
		return -1;
	
	if (_key->length)
	{   // Locate and read back keys data
		fseek (ini->ftmp, _key->pos, SEEK_SET);
	}
	else if (_key == &ini->tmpKey)
		return -1; // Can't read tmpKey
	*size = _key->length;
	return 0;
}


#ifdef INI_ADD_LIST_SUPPORT
/********************************************************************************************************************
 * Function          : __ini_readList
 * Parameters        : ini - pointer to ini file database.
 * Returns           : Pointer to buffer holding data.
 * Globals Used      :
 * Globals Modified  :
 * Description       : Common read functionality from a list
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
static char *__ini_readList (ini_t *ini)
{
	if (!ini->selected)
		return NULL;
	// Locate and read keys value
	if (!ini->selected->selected)
		return NULL;
	return __ini_listRead (ini);
}
#endif // INI_ADD_LIST_SUPPORT


/********************************************************************************************************************
 * Function          : __ini_write
 * Parameters        : ini - pointer to ini file database.  heading - heading name.  key - key name.
 * Returns           : Pointer to new key.
 * Globals Used      :
 * Globals Modified  :
 * Description       : Make calls to add a new key and appends a description of changes in the backup file
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
static struct key_tag *__ini_write (ini_t *ini)
{
	struct section_tag *section;
	struct key_tag     *key;
	
	// Is file read only?
	if (ini->mode == INI_READ)
		return NULL;
	
	// Check to make sure a section/key has
	// been asked for by the user
	section = ini->selected;
	if (!section)
		return NULL;
	key = section->selected;
	if (!key)
		return NULL;
	
	// Add or replace key
	if (!__ini_addHeading (ini, section->heading))
		return NULL;
	return __ini_addKey (ini, key->key);
}


/********************************************************************************************************************
 * Function          : ini_readString
 * Parameters        : ini - pointer to ini file database.
 *                   : value - keys data
 * Returns           : -1 for error or the number of chars read.
 * Globals Used      :
 * Globals Modified  :
 * Description       : Reads data part from a key and returns it as a string
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
int INI_LINKAGE ini_readString (ini_fd_t fd, char *str, size_t size)
{
	ini_t *ini = (ini_t *) fd;
	
	// Check size and reserve space for NULL
	if (size-- <= 0)
		return -1;
	
#ifdef INI_ADD_LIST_SUPPORT
	if (ini->listDelims)
	{
		char  *data = __ini_readList (ini);
		if (!data)
			return -1;
		strncpy (str, data, size);
	}
	else
#endif // INI_ADD_LIST_SUPPORT
	{   // Locate and read back keys data (Index ignored)
		// Check to make sure size is correct
		size_t length;
		if (__ini_read (ini, &length) < 0)
			return -1;
		if (size > length)
			size = length;
		size = fread (str, sizeof(char), size, ini->ftmp);
	}
	
	str[size] = '\0';
	__ini_strtrim (str);
	return (int) size;
}


/********************************************************************************************************************
 * Function          : ini_writeString
 * Parameters        : ini - pointer to ini file database.
 *                   : value - keys data
 * Returns           : -1 for Error and 0 on success
 * Globals Used      :
 * Globals Modified  :
 * Description       : Writes data part to a key.
 *                   : Conforms to Microsoft API call.  E.g. use NULLS to remove headings/keys
 *                   : Headings and keys will be created as necessary
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
int INI_LINKAGE ini_writeString (ini_fd_t fd, const char *str)
{
	ini_t *ini = (ini_t *) fd;
	struct key_tag *_key;
	
	_key = __ini_write (ini);
	if (!_key)
		return -1;
	
	// Write data to bottom of backup file
	_key->length = strlen (str);
	fprintf (ini->ftmp, "%s\n", str);
	return 0;
}


/********************************************************************************************************************
 * Function          : ini_readInt
 * Parameters        : ini - pointer to ini file database.
 *                   : value - keys data
 * Returns           : -1 for error or the number of values read.
 * Globals Used      :
 * Globals Modified  :
 * Description       : Reads data part from a key and returns it as a int
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
int INI_LINKAGE ini_readInt (ini_fd_t fd, int *value)
{
	ini_t *ini = (ini_t *) fd;
	
#ifdef INI_ADD_LIST_SUPPORT
	if (ini->listDelims)
	{
		char *data = __ini_readList (ini);
		if (!data)
			return -1;
		sscanf (data, "%d", value);
	}
	else
#endif // INI_ADD_LIST_SUPPORT
	{
		size_t length;
		if (__ini_read (ini, &length) < 0)
			return -1;
		if (length > 0)
			fscanf (ini->ftmp, "%d", value);
	}
	return 0;
}


#ifdef INI_ADD_EXTRAS

/********************************************************************************************************************
 * Function          : ini_readLong
 * Parameters        : ini - pointer to ini file database.
 *                   : value - keys data
 * Returns           : -1 for error or the number of values read.
 * Globals Used      :
 * Globals Modified  :
 * Description       : Reads data part from a key and returns it as a long
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
int INI_LINKAGE ini_readLong (ini_fd_t fd, long *value)
{
	ini_t *ini = (ini_t *) fd;
	
#ifdef INI_ADD_LIST_SUPPORT
	if (ini->listDelims)
	{
		char *data = __ini_readList (ini);
		if (!data)
			return -1;
		sscanf (data, "%ld", value);
	}
	else
#endif // INI_ADD_LIST_SUPPORT
	{
		size_t length;
		if (__ini_read (ini, &length) < 0)
			return -1;
		if (length > 0)
			fscanf (ini->ftmp, "%ld", value);
	}
	return 0;
}


/********************************************************************************************************************
 * Function          : ini_readDouble
 * Parameters        : ini - pointer to ini file database.
 *                   : value - keys data
 * Returns           : -1 for error or the number of values read.
 * Globals Used      :
 * Globals Modified  :
 * Description       : Reads data part from a key and returns it as a double (real)
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
int INI_LINKAGE ini_readDouble (ini_fd_t fd, double *value)
{
	ini_t *ini = (ini_t *) fd;
	
#ifdef INI_ADD_LIST_SUPPORT
	if (ini->listDelims)
	{
		char *data = __ini_readList (ini);
		if (!data)
			return -1;
		sscanf (data, "%lf", value);
	}
	else
#endif // INI_ADD_LIST_SUPPORT
	{
		size_t length;
		if (__ini_read (ini, &length) < 0)
			return -1;
		if (length > 0)
			fscanf (ini->ftmp, "%lf", value);
	}
	return 0;
}


/********************************************************************************************************************
 * Function          : ini_readBool
 * Parameters        : ini - pointer to ini file database.
 *                   : value - keys data
 * Returns           : -1 for error or the number of values read.
 * Globals Used      :
 * Globals Modified  :
 * Description       : Reads data part from a key and returns it as a int.  Supported bool strings are 0, 1, true
 *                   : false.
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
int INI_LINKAGE ini_readBool (ini_fd_t fd, int *value)
{
	ini_t *ini = (ini_t *) fd;
	char   buffer[6] = "";
	
#ifdef INI_ADD_LIST_SUPPORT
	if (ini->listDelims)
	{
		char *data = __ini_readList (ini);
		if (!data)
			return -1;
		sscanf (data, "%5s", buffer);
	}
	else
#endif // INI_ADD_LIST_SUPPORT
	{
		size_t length;
		if (__ini_read (ini, &length) < 0)
			return -1;
		if (length > 0)
			fscanf (ini->ftmp, "%5s", buffer);
	}
	
	{   // Convert string to lower case
		char *p = buffer;
		while (*p != '\0')
		{
			*p = (char) tolower (*p);
			p++;
		}
	}
	
	// Decode supported bool types
	switch (*buffer)
	{
		case '0':
		case '1':
			if (buffer[1] != '\0')
				return -1;
			*value = *buffer - '0';
			break;
		case 't':
			if (strcmp (buffer, "true"))
				return -1;
			*value = 1;
			break;
		case 'f':
			if (strcmp (buffer, "false"))
				return -1;
			*value = 0;
			break;
		default:
			// No match
			return -1;
	}
	return 0;
}


/********************************************************************************************************************
 * Function          : ini_writeInt
 * Parameters        : ini - pointer to ini file database.
 *                   : value - keys data
 * Returns           : -1 for Error and 0 on success
 * Globals Used      :
 * Globals Modified  :
 * Description       : Writes data part to a key.
 *                   : Headings and keys will be created as necessary
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
int INI_LINKAGE ini_writeInt (ini_fd_t fd, int value)
{
	ini_t *ini = (ini_t *) fd;
	struct key_tag *_key;
	long   pos;
	
	_key = __ini_write (ini);
	if (!_key)
		return -1;
	
	// Write data to bottom of backup file
	fprintf (ini->ftmp, "%d", value);
	pos = ftell (ini->ftmp);
	_key->length = (size_t) (pos - _key->pos);
	fprintf (ini->ftmp, "\n");
	return 0;
}


/********************************************************************************************************************
 * Function          : ini_writeLong
 * Parameters        : ini - pointer to ini file database.
 *                   : value - keys data
 * Returns           : -1 for Error and 0 on success
 * Globals Used      :
 * Globals Modified  :
 * Description       : Writes data part to a key.
 *                   : Headings and keys will be created as necessary
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
int INI_LINKAGE ini_writeLong (ini_fd_t fd, long value)
{
	ini_t *ini = (ini_t *) fd;
	struct key_tag *_key;
	long   pos;
	
	_key = __ini_write (ini);
	if (!_key)
		return -1;
	
	// Write data to bottom of backup file
	fprintf (ini->ftmp, "%ld", value);
	pos = ftell (ini->ftmp);
	_key->length = (size_t) (pos - _key->pos);
	fprintf (ini->ftmp, "\n");
	return 0;
}


/********************************************************************************************************************
 * Function          : ini_writeDouble
 * Parameters        : ini - pointer to ini file database.
 *                   : value - keys data
 * Returns           : -1 for Error and 0 on success
 * Globals Used      :
 * Globals Modified  :
 * Description       : Writes data part to a key.
 *                   : Headings and keys will be created as necessary
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
int INI_LINKAGE ini_writeDouble (ini_fd_t fd, double value)
{
	ini_t *ini = (ini_t *) fd;
	struct key_tag *_key;
	long   pos;
	
	_key = __ini_write (ini);
	if (!_key)
		return -1;
	
	// Write data to bottom of backup file
	fprintf (ini->ftmp, "%f", value);
	pos = ftell (ini->ftmp);
	_key->length = (size_t) (pos - _key->pos);
	fprintf (ini->ftmp, "\n");
	return 0;
}


/********************************************************************************************************************
 * Function          : ini_writeBool
 * Parameters        : ini - pointer to ini file database.
 *                   : value - keys data
 * Returns           : -1 for Error and 0 on success
 * Globals Used      :
 * Globals Modified  :
 * Description       : Writes data part to a key.
 *                   : Headings and keys will be created as necessary
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
int INI_LINKAGE ini_writeBool (ini_fd_t fd, int value)
{
	ini_t *ini = (ini_t *) fd;
	struct key_tag *_key;
	long   pos;
	
	// Check if value is legal
	if ((value < 0) || (value > 1))
		return -1;
	
	_key = __ini_write (ini);
	if (!_key)
		return -1;
	
	// Write data to bottom of backup file
	if (value)
		fprintf (ini->ftmp, "true");
	else
		fprintf (ini->ftmp, "false");
	pos = ftell (ini->ftmp);
	_key->length = (size_t) (pos - _key->pos);
	fprintf (ini->ftmp, "\n");
	return 0;
}

#endif // INI_ADD_EXTRAS
// Add List Support
#ifdef INI_ADD_LIST_SUPPORT
/***************************************************************************
 list.i - Adds list support to ini files
 
 -------------------
 begin                : Fri Apr 21 2000
 copyright            : (C) 2000 by Simon White
 email                : s_a_white@email.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <stdlib.h>
#include <string.h>
#include "libini-list.h"

#ifdef INI_ADD_LIST_SUPPORT

/********************************************************************************************************************
 * Function          : ini_listEval
 * Parameters        : ini - pointer to ini file database.
 * Returns           : -1 for Error or the numbers of list items found
 * Globals Used      :
 * Globals Modified  :
 * Description       : Uses the currently specified delimiters to calculate the number of eliments in a list
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
int __ini_listEval (ini_t *ini)
{
	int  length, count, i, ret;
	int  ldelim;
	char ch;
	
	// Remove old list
	if (ini->list)
	{
		free (ini->list);
		ini->list = NULL;
	}
	
	// Re-evaluate with new settings
	length = ini->selected->selected->length;
	if (length < 0)
		return -1;
	if (!length)
	{
		ini->listIndex  = 0;
		ini->listLength = 0;
		if (ini->selected->selected == &ini->tmpKey)
			return -1; // Can't read tmpKey
		return 0;
	}
	
	// See if there are any chars which can be used to split the string into sub ones
	if (!ini->listDelims)
		return -1;
	ldelim = (int) strlen (ini->listDelims);
	
	// Buffer string for faster access
	ini->list = (char *) malloc (length + 1);
	if (!ini->list)
		return -1;
	
	{   // Backup up delims to avoid causing problems with readString
		char *delims    = ini->listDelims;
		ini->listDelims = NULL;
		ret = ini_readString ((ini_fd_t) ini, ini->list, length + 1);
		ini->listDelims = delims;
		if (ret < 0)
			return -1;
	}
	
	// Process buffer string to find number of sub strings
	{
		char lastch = '\0';
		count = 1;
		while (length)
		{
			length--;
			ch = ini->list[length];
			for (i = 0; i < ldelim; i++)
			{
				if ((char) ch == ini->listDelims[i])
				{   // Prevent lots of NULL strings on multiple
					// whitespace
					if (lastch == '\0')
					{
						if (isspace (ch))
						{
							ch = '\0';
							break;
						}
					}
					
					// Seperate strings
					ini->list[length] = (ch = '\0');
					count++;
					break;
				}
			}
			lastch = ch;
		}
	}
	
	ini->listLength   = count;
	ini->listIndexPtr = ini->list;
	ini->listIndex    = 0;
	return count;
}


/********************************************************************************************************************
 * Function          : __ini_listRead
 * Parameters        : ini - pointer to ini file database.
 * Returns           : NULL for error, string point otherwise
 * Globals Used      :
 * Globals Modified  :
 * Description       : Reads the indexed parameter from the list and auto
 *                   : increments
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
char *__ini_listRead (ini_t *ini)
{
	char *p;
	
	// we must read an element from the list
	// Rev 1.2 Changed order of these two ifs as test was performed
	// sometimes before anything was read
	if (!ini->list)
	{
		if (__ini_listEval (ini) < 0)
			return NULL;
		// Handle an empty list
		if (!ini->listLength)
			return "";
	}
	
	// Check to see if we are trying to get a value beyond the end of the list
	if (ini->listIndex >= ini->listLength)
		return NULL;
	p = ini->listIndexPtr;
	// Auto increment pointers to next index
	ini->listIndexPtr += (strlen (ini->listIndexPtr) + 1);
	ini->listIndex++;
	return p;
}


/********************************************************************************************************************
 * Function          : ini_listLength
 * Parameters        : ini - pointer to ini file database.  heading - heading name.  key - key name.
 * Returns           : -1 for Error or the numbers of list items found
 * Globals Used      :
 * Globals Modified  :
 * Description       : Uses the currently specified delimiters to calculate the number of eliments in a list
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
int INI_LINKAGE ini_listLength (ini_fd_t fd)
{
	ini_t *ini = (ini_t *) fd;
	
	// Check to make sure a section/key has
	// been asked for by the user
	if (!ini->selected)
		return -1;
	if (!ini->selected->selected)
		return -1;
	
	// Check to see if we have moved to a new key
	if (!ini->list)
		return __ini_listEval (ini);
	
	return ini->listLength;
}


/********************************************************************************************************************
 * Function          : ini_listDelims
 * Parameters        : ini - pointer to ini file database.  delims - string of delimitor chars
 * Returns           : -1 for Error or 0 for success
 * Globals Used      :
 * Globals Modified  :
 * Description       : Sets the delimiters used for list accessing, (default delim is NULL)
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
int INI_LINKAGE ini_listDelims (ini_fd_t fd, const char *delims)
{
	ini_t *ini = (ini_t *) fd;
	if (ini->listDelims)
		free (ini->listDelims);
	ini->listDelims = NULL;
	
	// Make sure we have something to copy
	if (delims)
	{
		if (*delims)
		{   // Store delims for later use
			ini->listDelims = strdup (delims);
			if (!ini->listDelims)
				return -1;
		}
	}
	
	// List will need recalculating at some point
	if (ini->list)
	{
		free (ini->list);
		ini->list = NULL;
	}
	return 0;
}


/********************************************************************************************************************
 * Function          : ini_listIndex
 * Parameters        : ini - pointer to ini file database.  delims - string of delimitor chars
 * Returns           :
 * Globals Used      :
 * Globals Modified  :
 * Description       : Sets the index that the next call to any of the read function will obtain (default 0)
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
int INI_LINKAGE ini_listIndex (ini_fd_t fd, unsigned long index)
{
	ini_t *ini = (ini_t *) fd;
	unsigned int count;
	char    *p;
	
	// Check to make sure a section/key has
	// been asked for by the user
	if (!ini->selected)
		return -1;
	if (!ini->selected->selected)
		return -1;
	
	// Pull in new list
	if (!ini->list)
	{
		if (__ini_listEval (ini) < 0)
			return -1;
	}
	
	// Now scroll through to required index
	if (!ini->listLength)
		return -1;
	if (index == ini->listIndex)
		return 0;
	
	if (index > ini->listIndex)
	{   // Continue search from the from current position
		count = ini->listIndex;
		p     = ini->listIndexPtr;
	}
	else
	{   // Reset list and search from beginning
		count = 0;
		p     = ini->list;
	}
	
	while (count != index)
	{
		count++;
		if (count >= ini->listLength)
			return -1;
		// Jump to next sub string
		p += (strlen (p) + 1);
	}
	
	ini->listIndex    = count;
	ini->listIndexPtr = p;
	return 0;
}


/********************************************************************************************************************
 * Function          : __ini_listIndexLength
 * Parameters        : ini - pointer to ini file database
 * Returns           :
 * Globals Used      :
 * Globals Modified  :
 * Description       : Returns the length the indexed sub string
 ********************************************************************************************************************
 *  Rev   |   Date   |  By   | Comment
 * ----------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
int __ini_listIndexLength (ini_t *ini)
{
	if (!ini->list)
	{   // No list yet.  So try to get one
		if (__ini_listEval (ini) < 0)
			return -1;
		if (!ini->listLength)
			return 0;
	}
	
	// Now return length
	return strlen (ini->listIndexPtr);
}

#endif // INI_ADD_LIST_SUPPORT
#endif // INI_ADD_LIST_SUPPORT

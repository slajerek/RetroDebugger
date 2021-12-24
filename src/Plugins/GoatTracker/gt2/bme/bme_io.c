//
// BME (Blasphemous Multimedia Engine) datafile IO main module
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "SDL.h"

#include "bme_main.h"
#include "bme_err.h"
#include "bme_cfg.h"

typedef struct
{
    Uint32 offset;
    Sint32 length;
    char name[13];
} HEADER;

typedef struct
{
    HEADER *currentheader;
    int filepos;
    int open;
} HANDLE;

static int io_usedatafile = 0;
static HEADER *fileheaders;
static unsigned files;
static char ident[4];
static char *idstring = "DAT!";
static HANDLE handle[MAX_HANDLES];
static FILE *fileptr[MAX_HANDLES] = {NULL};
static FILE *datafilehandle = NULL;
static unsigned char *datafileptr;
static unsigned char *datafilestart;

static unsigned freadle32(FILE *index);
static void linkedseek(unsigned pos);
static void linkedread(void *buffer, int length);
static unsigned linkedreadle32(void);

void io_setfilemode(int usedf)
{
    io_usedatafile = usedf;
}

int io_openlinkeddatafile(unsigned char *ptr)
{
    int index;

    if (datafilehandle) fclose(datafilehandle);
    datafilehandle = NULL;

    datafilestart = ptr;
    linkedseek(0);

    linkedread(ident, 4);
    if (memcmp(ident, idstring, 4))
    {
        bme_error = BME_WRONG_FORMAT;
        return BME_ERROR;
    }

    files = linkedreadle32();
    fileheaders = malloc(files * sizeof(HEADER));
    if (!fileheaders)
    {
        bme_error = BME_OUT_OF_MEMORY;
        return BME_ERROR;
    }
    for (index = 0; index < files; index++)
    {
        fileheaders[index].offset = linkedreadle32();
        fileheaders[index].length = linkedreadle32();
        linkedread(&fileheaders[index].name, 13);
    }

    for (index = 0; index < MAX_HANDLES; index++) handle[index].open = 0;
    io_usedatafile = 1;
    bme_error = BME_OK;
    return BME_OK;
}

int io_opendatafile(char *name)
{
    int index;

    if (name)
    {
        datafilehandle = fopen(name, "rb");
        if (!datafilehandle)
        {
            bme_error = BME_OPEN_ERROR;
            return BME_ERROR;
        }
    }

    fread(ident, 4, 1, datafilehandle);
    if (memcmp(ident, idstring, 4))
    {
        bme_error = BME_WRONG_FORMAT;
        return BME_ERROR;
    }

    files = freadle32(datafilehandle);
    fileheaders = malloc(files * sizeof(HEADER));
    if (!fileheaders)
    {
        bme_error = BME_OUT_OF_MEMORY;
        return BME_ERROR;
    }
    for (index = 0; index < files; index++)
    {
        fileheaders[index].offset = freadle32(datafilehandle);
        fileheaders[index].length = freadle32(datafilehandle);
        fread(&fileheaders[index].name, 13, 1, datafilehandle);
    }

    for (index = 0; index < MAX_HANDLES; index++) handle[index].open = 0;
    io_usedatafile = 1;
    bme_error = BME_OK;
    return BME_OK;
}

// Returns nonnegative file handle if successful, -1 on error

int io_open(char *name)
{
    if (!name) return -1;

    if (!io_usedatafile)
    {
        int index;
        for (index = 0; index < MAX_HANDLES; index++)
        {
            if (!fileptr[index]) break;
        }
        if (index == MAX_HANDLES) return -1;
        else
        {
            FILE *file = fopen(name, "rb");
            if (!file)
            {
                return -1;
            }
            else
            {
                fileptr[index] = file;
                return index;
            }
        }
    }
    else
    {
        int index;
        int namelength;
        char namecopy[13];

        namelength = strlen(name);
        if (namelength > 12) namelength = 12;
        memcpy(namecopy, name, namelength + 1);
        for (index = 0; index < strlen(namecopy); index++)
        {
            namecopy[index] = toupper(namecopy[index]);
        }

        for (index = 0; index < MAX_HANDLES; index++)
        {
            if (!handle[index].open)
            {
                int count = files;
                handle[index].currentheader = fileheaders;

                while (count)
                {
                    if (!strcmp(namecopy, handle[index].currentheader->name))
                    {
                         handle[index].open = 1;
                         handle[index].filepos = 0;
                         return index;
                    }
                    count--;
                    handle[index].currentheader++;
                }
                return -1;
            }
        }
        return -1;
    }
}

// Returns file position after seek or -1 on error

int io_lseek(int index, int offset, int whence)
{
    if (!io_usedatafile)
    {
         fseek(fileptr[index], offset, whence);
         return ftell(fileptr[index]);
    }
    else
    {
        int newpos;

        if ((index < 0) || (index >= MAX_HANDLES)) return -1;

        if (!handle[index].open) return -1;
        switch(whence)
        {
            default:
            case SEEK_SET:
            newpos = offset;
            break;

            case SEEK_CUR:
            newpos = offset + handle[index].filepos;
            break;

            case SEEK_END:
            newpos = offset + handle[index].currentheader->length;
            break;
        }
        if (newpos < 0) newpos = 0;
        if (newpos > handle[index].currentheader->length) newpos = handle[index].currentheader->length;
        handle[index].filepos = newpos;
        return newpos;
    }
}

// Returns number of bytes actually read, -1 on error

int io_read(int index, void *buffer, int length)
{
    if (!io_usedatafile)
    {
        return fread(buffer, 1, length, fileptr[index]);
    }
    else
    {
        int readbytes;

        if ((index < 0) || (index >= MAX_HANDLES)) return -1;

        if (!handle[index].open) return -1;
        if (length + handle[index].filepos > handle[index].currentheader->length)
        length = handle[index].currentheader->length - handle[index].filepos;
        
        if (datafilehandle)
        {
            fseek(datafilehandle, handle[index].currentheader->offset + handle[index].filepos, SEEK_SET);
            readbytes = fread(buffer, 1, length, datafilehandle);
        }
        else
        {
            linkedseek(handle[index].currentheader->offset + handle[index].filepos);
            linkedread(buffer, length);
            readbytes = length;
        }
        handle[index].filepos += readbytes;
        return readbytes;
    }
}

// Returns nothing

void io_close(int index)
{
    if (!io_usedatafile)
    {
        fclose(fileptr[index]);
        fileptr[index] = NULL;
    }
    else
    {
        if ((index < 0) || (index >= MAX_HANDLES)) return;

        handle[index].open = 0;
    }
}

unsigned io_read8(int index)
{
    unsigned char byte;

    io_read(index, &byte, 1);
    return byte;
}

unsigned io_readle16(int index)
{
    unsigned char bytes[2];

    io_read(index, bytes, 2);
    return (bytes[1] << 8) | bytes[0];
}

unsigned io_readhe16(int index)
{
    unsigned char bytes[2];

    io_read(index, bytes, 2);
    return (bytes[0] << 8) | bytes[1];
}

unsigned io_readle32(int index)
{
    unsigned char bytes[4];

    io_read(index, bytes, 4);
    return (bytes[3] << 24) | (bytes[2] << 16) | (bytes[1] << 8) | bytes[0];
}

unsigned io_readhe32(int index)
{
    unsigned char bytes[4];

    io_read(index, bytes, 4);
    return (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];
}

static unsigned freadle32(FILE *file)
{
    unsigned char bytes[4];

    fread(&bytes, 4, 1, file);
    return (bytes[3] << 24) | (bytes[2] << 16) | (bytes[1] << 8) | bytes[0];
}

static void linkedseek(unsigned pos)
{
    datafileptr = &datafilestart[pos];
}

static void linkedread(void *buffer, int length)
{
    unsigned char *dest = (unsigned char *)buffer;
    while (length--)
    {
        *dest++ = *datafileptr++;
    }
}

static unsigned linkedreadle32(void)
{
    unsigned char bytes[4];

    linkedread(&bytes, 4);
    return (bytes[3] << 24) | (bytes[2] << 16) | (bytes[1] << 8) | bytes[0];
}    


#include <stdio.h>
#include "SDL_types.h"

void fwrite8(FILE *file, unsigned data)
{
    Uint8 bytes[1];

    bytes[0] = data;
    fwrite(bytes, 1, 1, file);
}

void fwritele16(FILE *file, unsigned data)
{
    Uint8 bytes[2];

    bytes[0] = data;
    bytes[1] = data >> 8;
    fwrite(bytes, 2, 1, file);
}

void fwritele32(FILE *file, unsigned data)
{
    Uint8 bytes[4];

    bytes[0] = data;
    bytes[1] = data >> 8;
    bytes[2] = data >> 16;
    bytes[3] = data >> 24;
    fwrite(bytes, 4, 1, file);
}

unsigned fread8(FILE *file)
{
    Uint8 bytes[1];

    fread(bytes, 1, 1, file);
    return bytes[0];
}

unsigned freadle16(FILE *file)
{
    Uint8 bytes[2];

    fread(bytes, 2, 1, file);
    return (bytes[0]) | (bytes[1] << 8);
}

unsigned freadle32(FILE *file)
{
    Uint8 bytes[4];

    fread(bytes, 4, 1, file);
    return (bytes[0]) | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);
}

unsigned freadhe16(FILE *file)
{
    Uint8 bytes[2];

    fread(bytes, 2, 1, file);
    return (bytes[1]) | (bytes[0] << 8);
}

unsigned freadhe32(FILE *file)
{
    Uint8 bytes[4];

    fread(bytes, 4, 1, file);
    return (bytes[3]) | (bytes[2] << 8) | (bytes[1] << 16) | (bytes[0] << 24);
}



#include "asap.h"
#include "SYS_Defs.h"
#include "AtariAsap.h"
#include "CByteBuffer.h"
#include "DBG_Log.h"
#include <string.h>

// code below is based on asapconv.c 
CByteBuffer *ConvertASAPtoXEX(const char *fileName, CByteBuffer *asapFileData, bool *isStereo)
{
	LOGD("ConvertASAPtoXEX: fileName=%s len=%d", fileName, asapFileData->length);
	const char *input_ext = strrchr(fileName, '.');
	if (input_ext == NULL)
	{
		LOGError("ConvertASAPtoXEX: missing extension fileName='%s'", fileName);
		return NULL;
	}
	input_ext++;
	
	ASAPInfo *info = ASAPInfo_New();
	if (info == NULL)
	{
		LOGError("ConvertASAPtoXEX: ASAPInfo_New failed");
		return NULL;
	}

	const unsigned char *module = asapFileData->data;
	int module_len = asapFileData->length;

	if (!ASAPInfo_Load(info, fileName, module, module_len))
	{
		LOGError("ConvertASAPtoXEX: ASAPInfo_Load unsupported file %s", fileName);
		return NULL;
	}

	*isStereo = (ASAPInfo_GetChannels(info) == 2);
	
	ASAPWriter *writer = ASAPWriter_New();
	if (writer == NULL)
	{
		LOGError("ConvertASAPtoXEX: ASAPWriter_New failed");
		return NULL;
	}
	
//	apply_tags(input_file, info);
//	if (arg_music_address >= 0)
//		ASAPInfo_SetMusicAddress(info, arg_music_address);

//	int current_song = ASAPInfo_GetDefaultSong(info);
//	ASAPInfo_SetDefaultSong(info, current_song);

//	ASAPInfo_SetDuration(info, current_song, arg_duration);

	unsigned char *output = new unsigned char[ASAPInfo_MAX_MODULE_LENGTH];
	ASAPWriter_SetOutput(writer, output, 0, ASAPInfo_MAX_MODULE_LENGTH);

	int output_len = ASAPWriter_Write(writer, "output.xex", info, module, module_len, true);
	if (output_len < 1)
	{
		LOGError("ConvertASAPtoXEX: conversion failed");
		return NULL;
	}
	
	CByteBuffer *retBuffer = new CByteBuffer(output, output_len);
	
	ASAPWriter_Delete(writer);
	ASAPInfo_Delete(info);
	
	return retBuffer;
}

/*
ASAP - Another Slight Atari Player
==================================

ASAP is a player of Atari 8-bit chiptunes for modern computers and mobile
devices.  It emulates the POKEY sound chip and the 6502 processor.

ASAP supports the following file formats: SAP, CMC, CM3, CMR, CMS, DMC, DLT,
FC, MPT, MPD, RMT, TMC/TM8, TM2, STIL and is available on Android, Windows,
macOS, Linux and modern web browsers.

ASAP is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License as published
by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

See the INSTALL file for build instructions.

For more information, visit the website: http://asap.sourceforge.net/

 Authors

 Piotr Fusik	Creator and lead developer.
 Atari800 Emulator Developers	6502 and POKEY emulation used in 0.x.y versions of ASAP.
 Zdenek Eisenhammer	Testing.
 Maciej Grzybek	Fixed an overflow in the Silverlight port.
 Jakub Husak	SAP fingerprint calculation. asapscan fixes.
 Henryk Karpowicz	CMC routine modified for the CM3 format.
 Maciek Konecki	Porting to C#.
 Marek Konopka	6502 routine for playing DLT.
 Daniel Koźminski	Ideas.
 Jerzy Kut	FC format.
 Marcin Lewandowski	6502 routines for playing CMC, MPT, TMC and TM2.
 Ian Luck	Guided development of XMPlay and BASS plugins.
 MarOk	CMS routine fix.
 Adrian Matoga	COVOX information and test files. Testing. Porting to D.
 Perry McFarlane	POKEY reverse-engineering.
 Kostas Nakos	Windows CE testing.
 Petri Pyy	macOS releases.
 Mariusz Rozwadowski	Suggested CMS, CM3, DLT and STIL format support.
 Sławomir Śledź	Windows Mobile setup. Thorough testing.
 David Spilka	6502 routine for playing CMS.
 Radek Štěrba	6502 routine for playing RMT. Testing.
 Łukasz Sychowicz	Windows icons. Testing.
 Paweł Szewczyk	Windows setup graphics.
 Michał Szpilowski	Testing.
 Piotr Wiszowaty	VLC 3 compatibility.
 Grzegorz Żyła	XBMC plugin testing.
 
 */

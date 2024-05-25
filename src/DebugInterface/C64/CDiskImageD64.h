/*
 *  C64 Debugger
 *
 *  Created by Marcin Skoczylas on 16-02-22.
 *  Copyright 2016 Marcin Skoczylas. All rights reserved.
 *
 *  This is "loosely" based on droiD64 - A graphical file manager for D64 files, Copyright (C) 2004 Wolfram Heyer
 */

#ifndef _DISKIMAGED64_
#define _DISKIMAGED64_

#include "SYS_Defs.h"
#include "CByteBuffer.h"
#include <filesystem>

#define D64_SIZE		174848
#define D64_MAX_FILES	144
#define D64_FIRST_TRACK	1
#define D64_LAST_TRACK	35
#define D64_NUM_TRACKS	40

#define D64_MAX_SECTORS	21

#define D64_BAM_TRACK	18
#define D64_BAM_SECTOR	0

#define VDRIVE_COUNT	4

#define RET_STATUS_OK		0
#define RET_STATUS_FAILED	1
#define RET_STATUS_REPLACED	2

typedef struct DiskImageTrack_s {
	int sectorNum;
	int sectorsIn;
	int offset;
} DiskImageTrack;

typedef struct DiskImageFileEntry_s {
	u8 fileType;
	bool locked;
	bool closed;
	u8 track;
	u8 sector;
	u8 fileName[0x10];
	int fileSize;
} DiskImageFileEntry;

class CDebugInterfaceVice;
class CDataAdapterViceDrive1541DiskContents;

class CDiskImageD64
{
public:
	// get disk image from drive attached
	CDiskImageD64(CDebugInterfaceVice *debugInterface, int driveId);
	
	// get disk image from file
	CDiskImageD64(char *fileName);
	
	virtual ~CDiskImageD64();
		
	void SetDiskImage(int driveId);
	void SetDiskImage(char *fileName);
	void RefreshImage();
	void Clear();

	CDebugInterfaceVice *debugInterface;
	CDataAdapterViceDrive1541DiskContents *dataAdapter;
	
//	disk_image_t *diskImage;
	bool IsDiskAttached();
	bool isFromFile;
	
	bool ReadImage();

	u8 diskName[0x10];
	u8 diskId[0x07];
	int numFreeSectors;
	
	std::vector<DiskImageFileEntry *> fileEntries;

	bool ReadEntry(int entryNum, CByteBuffer *byteBuffer);
	bool ReadEntry(DiskImageFileEntry *fileEntry, CByteBuffer *byteBuffer);
	
	DiskImageFileEntry *FindDiskPRGEntry(int entryNum);

	static const char *FileEntryTypeToStr(u8 fileType);
	
	// for disk operations
	bool CreateDiskImage(const char *cPath);
	void FormatDisk(const char *diskName, const char *diskId);
	int InsertFile(std::filesystem::path filePath, bool alwaysReplace);
	
private:
	bool isDiskAttached;
};

#endif

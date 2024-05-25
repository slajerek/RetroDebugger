#ifndef CDataAdapterViceDrive1541DiskContents_h
#define CDataAdapterViceDrive1541DiskContents_h

#include "CDebugDataAdapter.h"
#include <filesystem>

class CDebugInterfaceVice;

extern "C" {
struct drive_context_s;
typedef struct drive_context_s drive_context_t;

struct disk_image_s;
typedef struct disk_image_s disk_image_t;

struct gcr_s;
typedef struct gcr_s gcr_t;

struct vdrive_s;
typedef struct vdrive_s vdrive_t;
}

struct Drive1541ContentsIndex
{
	unsigned int track;					// track #
	unsigned int sector;				// sector #
	unsigned int sectorDataOffset;		// which byte of sector
};

#define DRIVE1541_SECTOR_SIZE			256
#define DRIVE1541_MAX_SECTORS_PER_TRACK	21

struct Drive1541ContentsSector
{
	u8 data[256];
};

struct Drive1541ContentsTrack
{
	u32 numSectors;
	Drive1541ContentsSector *sectors;
	bool isDirty;
	u32 dataOffset;
};

struct drive_context_s;
typedef struct drive_context_s drive_context_t;

struct gcr_s;
typedef struct gcr_s gcr_t;

struct disk_image_s;
typedef struct disk_image_s disk_image_t;

struct vdrive_s;
typedef struct vdrive_s vdrive_t;


class CDataAdapterViceDrive1541DiskContents : public CDebugDataAdapter
{
public:
	CDataAdapterViceDrive1541DiskContents(CDebugSymbols *debugSymbols, int driveId);
	CDebugInterfaceVice *debugInterfaceVice;
	
	virtual int AdapterGetDataLength();

	virtual void GetAddressStringForCell(int cell, char *str, int maxLen);
	virtual CDataAddressEditBox *CreateDataAddressEditBox(CDataAddressEditBoxCallback *callback);

	virtual void AdapterReadByte(int pointer, uint8 *value);
	virtual void AdapterWriteByte(int pointer, uint8 value);
	virtual void AdapterReadByte(int pointer, uint8 *value, bool *isAvailable);
	virtual void AdapterWriteByte(int pointer, uint8 value, bool *isAvailable);
	

	int driveId;
	int numOfTracks;
	int numOfSectors;
	int totalSize;

	virtual void DiskAttached();
	virtual void DiskDetached();
	
	virtual u32 GetNumberOfSectorsPerTrack(u32 track);
	
	disk_image_t *diskImage;
	gcr_t *gcr;
	
	virtual void RefreshDiskContentsData();
	
	// this is to quickly access track/sector/pointer
	Drive1541ContentsIndex *dataIndexes;
	
	Drive1541ContentsTrack *tracks;
		
	virtual int GetDataAddressForTrackAndSector(int track, int sector);
	virtual Drive1541ContentsIndex *GetTrackAndSectorForDataAddress(int dataAddress);
	
	virtual void DriveContextReadSector(int trackNum, int sectorNum, u8 *buf);
	virtual void DriveContextWriteSector(int trackNum, int sectorNum, u8 *buf);
	
//	virtual void MarkWholeDiskDirty();
	virtual void MarkTrackDirty(int trackNum);
	
	virtual void ReadSector(int trackNum, int sectorNum, u8 *buf);
	virtual void WriteSector(int trackNum, int sectorNum, u8 *buf);
	
	// for disk operations
	bool CreateDiskImage(const char *cPath);
	void FormatDisk(const char *diskName, const char *diskId);
	int InsertFile(std::filesystem::path filePath, bool alwaysReplace);

	bool IsDiskAttached();
};

#endif //CDataAdapterViceDrive1541DiskContents_h

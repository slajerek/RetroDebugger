extern "C" {
#include "diskimage.h"
#include "vdrive.h"
#include "vdrive-command.h"
#include "vdrive-iec.h"
#include "charset.h"
#include "driveimage.h"
#include "lib.h"
#include "fileio.h"
#include "cbmimage.h"
};

#include "CDebugInterfaceVice.h"
#include "CDataAdapterViceDrive1541DiskContents.h"
#include "CDiskImageD64.h"
#include "CViewC64.h"
#include "CViewDrive1541Browser.h"
#include "CDataAddressEditBoxDiskContents.h"


extern "C" {
#define DWORD u32
disk_image_t *c64d_get_drive_disk_image(int driveId);
gcr_t *c64d_get_drive_disk_gcr(int driveId);
unsigned int disk_image_sector_per_track(unsigned int format, unsigned int track);
#include "diskimage.h"
#include "gcr.h"

disk_image_t *c64d_read_disk_image(char *fileName);
void c64d_disk_image_destroy(disk_image_t *diskImage);

int disk_image_read_sector(const disk_image_t *image, BYTE *buf, const disk_addr_t *dadr);
disk_image_t *c64d_get_drive_disk_image(int driveId);

}

// Note: in Vice disk_image_t tracks are accessed using disk_addr_t
CDataAdapterViceDrive1541DiskContents::CDataAdapterViceDrive1541DiskContents(CDebugSymbols *debugSymbols, int driveId)
: CDebugDataAdapter("ViceDrive1541DiskContents", debugSymbols)
{
	this->debugInterfaceVice = (CDebugInterfaceVice*)(debugSymbols->debugInterface);
	this->driveId = driveId;
	
	dataIndexes = NULL;
	numOfTracks = 35;
	numOfSectors = 683;
	totalSize = 174848;
	
	diskImage = NULL;
	gcr = NULL;
	dataIndexes = NULL;
	tracks = NULL;
}

int CDataAdapterViceDrive1541DiskContents::AdapterGetDataLength()
{
	return totalSize;
}

void CDataAdapterViceDrive1541DiskContents::GetAddressStringForCell(int cell, char *str, int maxLen)
{
	if (!dataIndexes)
	{
		str[0] = 0;
		return;
	}
	
	if (cell < 0 || cell >= totalSize)
	{
		str[0] = 0;
		return;
	}
		
	Drive1541ContentsIndex dataIndex = dataIndexes[cell];
	snprintf(str, maxLen, "%02x.%02x.%02x", dataIndex.track+1, dataIndex.sector, dataIndex.sectorDataOffset);
}

CDataAddressEditBox *CDataAdapterViceDrive1541DiskContents::CreateDataAddressEditBox(CDataAddressEditBoxCallback *callback)
{
	CDataAddressEditBoxDiskContents *editBox = new CDataAddressEditBoxDiskContents(this);
	editBox->SetCallback(callback);
	return editBox;
}

void CDataAdapterViceDrive1541DiskContents::DiskAttached()
{  
	LOGD("CDataAdapterViceDrive1541DiskContents::DiskAttached");
	debugInterface->LockMutex();
	debugInterface->LockIoMutex();
	
	this->diskImage = c64d_get_drive_disk_image(driveId);// TODO: diskImage;
	this->gcr = c64d_get_drive_disk_gcr(driveId);
	
	if (diskImage || gcr)
	{
		if (dataIndexes)
			delete [] dataIndexes;
		dataIndexes = NULL;
		
		if (tracks)
			delete [] tracks;
		tracks = NULL;
		
		numOfTracks = diskImage ? diskImage->tracks : 35;
//		LOGD("# of tracks=%d", numOfTracks);
	
		tracks = new Drive1541ContentsTrack[numOfTracks];

		// sanity check, calc size
		totalSize = 0;
		numOfSectors = 0;
		for (u32 track = 0; track < numOfTracks; track++)
		{
			tracks[track].numSectors = GetNumberOfSectorsPerTrack(track);
			tracks[track].sectors = new Drive1541ContentsSector[tracks[track].numSectors];
			tracks[track].dataOffset = totalSize;
			tracks[track].isDirty = true;
			
//			LOGD("%4d %5d offset %d", track+1, tracks[track].numSectors, tracks[track].dataOffset);
			numOfSectors += tracks[track].numSectors;
			totalSize += tracks[track].numSectors * 256;
		}

		LOGD("totalSectors=%d totalSize=%d", numOfSectors, totalSize);
		if (numOfSectors > 783)	// != 683
		{
			debugInterface->ShowNotificationError("Can't refresh disk contents, # of sectors is too large"); //not 683");
			LOGError("CDataAdapterViceDrive1541DiskContents::RefreshDiskContentsData: # of sectors is too large"); //not 683");
			debugInterface->UnlockMutex();
			debugInterface->UnlockIoMutex();
			return;
		}
		if (totalSize > 196608)	// != 174848
		{
			debugInterface->ShowNotificationError("Can't refresh disk contents, # of bytes stored is too large"); //not 174848");
			LOGError("CDataAdapterViceDrive1541DiskContents::RefreshDiskContentsData: # of bytes stored is too large"); //not 174848");
			debugInterface->UnlockMutex();
			debugInterface->UnlockIoMutex();
			return;
		}
		
		dataIndexes = new Drive1541ContentsIndex[totalSize];
		int index = 0;
		for (u32 t = 0; t < numOfTracks; t++)
		{
			for (int s = 0; s < tracks[t].numSectors; s++)
			{
				for (int i = 0; i < 256; i++)
				{
					dataIndexes[index].track = t;
					dataIndexes[index].sector = s;
					dataIndexes[index].sectorDataOffset = i;
					index++;
				}
			}
		}
	}
	
	RefreshDiskContentsData();
	
	debugInterface->UnlockMutex();
	debugInterface->UnlockIoMutex();
}

void CDataAdapterViceDrive1541DiskContents::DiskDetached()
{
	LOGD("CDataAdapterViceDrive1541DiskContents::DiskDetached");
	DiskAttached();
}

unsigned int CDataAdapterViceDrive1541DiskContents::GetNumberOfSectorsPerTrack(unsigned int track)
{
	// note: Vice counts tracks from 1 to 35
	return disk_image_sector_per_track(DISK_IMAGE_TYPE_D64, track+1);
}

void CDataAdapterViceDrive1541DiskContents::AdapterReadByte(int pointer, uint8 *value)
{
	bool isAvailable;
	AdapterReadByte(pointer, value, &isAvailable);
}

void CDataAdapterViceDrive1541DiskContents::AdapterWriteByte(int pointer, uint8 value)
{
	bool isAvailable;
	AdapterWriteByte(pointer, value, &isAvailable);
}

void CDataAdapterViceDrive1541DiskContents::AdapterReadByte(int pointer, uint8 *value, bool *isAvailable)
{
	if (!dataIndexes)
	{
		*isAvailable = false;
		return;
	}
	
	if (pointer < 0 || pointer >= totalSize)
	{
		*isAvailable = false;
		return;
	}
		
	Drive1541ContentsIndex dataIndex = dataIndexes[pointer];
	if (tracks[dataIndex.track].isDirty)
	{
		RefreshDiskContentsData();
	}
	*value = tracks[dataIndex.track].sectors[dataIndex.sector].data[dataIndex.sectorDataOffset];
	*isAvailable = true;
	
	// debug test, always refresh all tracks every 2 seconds
//	static int t1 = SYS_GetTickCount();
//	int t2 = SYS_GetTickCount();
//	if (t2-t1 > 2000)
//	{
//		RefreshDiskContentsData();
//		t1 = SYS_GetTickCount();
//	}
}

bool CDataAdapterViceDrive1541DiskContents::IsDiskAttached()
{
	int isAttached = c64d_get_drive_is_disk_attached(driveId);
	if (isAttached == 0)
		return false;
	return true;
}

void CDataAdapterViceDrive1541DiskContents::ReadSector(int trackNum, int sectorNum, u8 *buf)
{
	if (!dataIndexes)
	{
		LOGError("CDataAdapterViceDrive1541DiskContents::WriteSector: disk not ready, dataIndexes is NULL");
		return;
	}

	if (tracks[trackNum].isDirty)
	{
		RefreshDiskContentsData();
	}

	if (trackNum >= 0 && trackNum < numOfTracks)
	{
		if (sectorNum >= 0 && sectorNum < tracks[trackNum].numSectors)
		{
			LOGD("CDataAdapterViceDrive1541DiskContents::ReadSector %d %d offset %d", trackNum, sectorNum, tracks[trackNum].dataOffset);
			memcpy(buf, tracks[trackNum].sectors[sectorNum].data, 256);
//			LOG_PrintHexArray(buf, 256);

		}
		else
		{
			LOGError("CDataAdapterViceDrive1541DiskContents::ReadSector: sectorNum=%d out of bounds (max=%d)", sectorNum, tracks[trackNum].numSectors);
		}
	}
	else
	{
		LOGError("CDataAdapterViceDrive1541DiskContents::ReadSector: trackNum=%d out of bounds (max=%d)", trackNum, numOfTracks);
	}
}

void CDataAdapterViceDrive1541DiskContents::AdapterWriteByte(int pointer, uint8 value, bool *isAvailable)
{
	LOGD("CDataAdapterViceDrive1541DiskContents::AdapterWriteByte: diskImage=%x", this->diskImage);
	
	if (!dataIndexes)
	{
		*isAvailable = false;
		return;
	}
	
	if (pointer < 0 || pointer >= totalSize)
	{
		*isAvailable = false;
		return;
	}
	
	Drive1541ContentsIndex dataIndex = dataIndexes[pointer];
	tracks[dataIndex.track].sectors[dataIndex.sector].data[dataIndex.sectorDataOffset] = value;
	*isAvailable = true;

	DriveContextWriteSector(dataIndex.track, dataIndex.sector, tracks[dataIndex.track].sectors[dataIndex.sector].data);
		
	// debug sanity check:
//	RefreshDiskContentsData();
}

void CDataAdapterViceDrive1541DiskContents::WriteSector(int trackNum, int sectorNum, u8 *buf)
{
	if (!dataIndexes)
	{
		LOGError("CDataAdapterViceDrive1541DiskContents::WriteSector: disk not ready, dataIndexes is NULL");
		return;
	}
	
	if (trackNum >= 0 && trackNum < numOfTracks)
	{
		if (sectorNum >= 0 && sectorNum < tracks[trackNum].numSectors)
		{
			memcpy(buf, tracks[trackNum].sectors[sectorNum].data, 256);
			DriveContextWriteSector(trackNum, sectorNum, tracks[trackNum].sectors[sectorNum].data);
		}
		else
		{
			LOGError("CDataAdapterViceDrive1541DiskContents::WriteSector: sectorNum=%d out of bounds (max=%d)", sectorNum, tracks[trackNum].numSectors);
		}
	}
	else
	{
		LOGError("CDataAdapterViceDrive1541DiskContents::WriteSector: trackNum=%d out of bounds (max=%d)", trackNum, numOfTracks);
	}
}

// reads all tracks and sectors and puts backup data for quicker access
void CDataAdapterViceDrive1541DiskContents::RefreshDiskContentsData()
{
	LOGD("CDataAdapterViceDrive1541DiskContents::RefreshDiskContentsData");
	
	// TODO: add dirty flag to refresh only when contents changed
	debugInterface->LockIoMutex();

	this->diskImage = c64d_get_drive_disk_image(driveId);
	this->gcr = c64d_get_drive_disk_gcr(driveId);
	
	bool needsDirectoryRefresh = false;
	if (dataIndexes && (diskImage || gcr))
	{
		// read all sectors
		u64 t1 = SYS_GetCurrentTimeInMillis();
		
		for (u32 track = 0; track < numOfTracks; track++)
		{
			if (tracks[track].isDirty)
			{
//				LOGD("track %d is dirty, refreshing", track);
				if (track == D64_BAM_TRACK-1)
				{
					needsDirectoryRefresh = true;
				}

				for (u32 sector = 0; sector < tracks[track].numSectors; sector++)
				{
					DriveContextReadSector(track, sector, tracks[track].sectors[sector].data);
					
//					if (track == D64_BAM_TRACK-1 && sector == 0)
//					{
//						LOG_PrintHexArray(tracks[track].sectors[sector].data, 256);
//						LOGD("-- dir ^^");
//					}
				}
				tracks[track].isDirty = false;
			}
		}

		u64 t2 = SYS_GetCurrentTimeInMillis();
		LOGD("CDataAdapterViceDrive1541DiskContents::RefreshDiskContentsData took %dms", t2-t1);
		
		if (needsDirectoryRefresh)
		{
			debugInterfaceVice->SetDriveDirtyForRefreshFlag(driveId);
		}
	}
	
	debugInterface->UnlockIoMutex();
}

void CDataAdapterViceDrive1541DiskContents::DriveContextReadSector(int trackNum, int sectorNum, u8 *buf)
{
//	LOGD("CDataAdapterViceDrive1541DiskContents::DriveContextReadSector: %d %d", trackNum, sectorNum);
	debugInterface->LockIoMutex();
	if (diskImage)
	{
		disk_addr_t diskAddr;
		diskAddr.track = trackNum+1;
		diskAddr.sector = sectorNum;
		disk_image_read_sector(diskImage, buf, &diskAddr);
	}
	else if (gcr)
	{
		int halfTrack = trackNum*2;
		gcr_read_sector(&(gcr->tracks[halfTrack]), buf, sectorNum);
	}
	else
	{
		LOGError("CDataAdapterViceDrive1541DiskContents::DriveContextReadSector: no disk");
	}
	debugInterface->UnlockIoMutex();
}

void CDataAdapterViceDrive1541DiskContents::DriveContextWriteSector(int trackNum, int sectorNum, u8 *buf)
{
	debugInterface->LockIoMutex();

	if (diskImage)
	{
		disk_addr_t diskAddr;
		diskAddr.track = trackNum+1;
		diskAddr.sector = sectorNum;
		disk_image_write_sector(diskImage, buf, &diskAddr);
	}
	else if (gcr)
	{
		int halfTrack = trackNum*2;
		gcr_write_sector(&(gcr->tracks[halfTrack]), buf, sectorNum);
	}
	else
	{
		LOGError("CDataAdapterViceDrive1541DiskContents::DriveContextWriteSector: no disk");
	}
	
	MarkTrackDirty(trackNum);
	debugInterface->UnlockIoMutex();
}

void CDataAdapterViceDrive1541DiskContents::MarkTrackDirty(int track)
{
	tracks[track].isDirty = true;
}

// disk operations on vdrive
bool CDataAdapterViceDrive1541DiskContents::CreateDiskImage(const char *cPath)
{
	int ret = cbmimage_create_image(cPath, DISK_IMAGE_TYPE_D64);
	if (ret == 0)
	{
		return true;
	}
	
	return false;
}

void CDataAdapterViceDrive1541DiskContents::FormatDisk(const char *diskName, const char *diskId)
{
	LOGD("CDataAdapterViceDrive1541DiskContents::FormatDisk: %s %s", diskName, diskId);

	int driveId = 0;
	disk_image_t *diskImage = c64d_get_drive_disk_image(driveId);
	
	if (diskImage == NULL)
	{
		// TODO: create new disk image
		LOGError("CDataAdapterViceDrive1541DiskContents::FormatDisk: diskImage is NULL");
		return;
	}
	
	vdrive_t *vdrive = (vdrive_t*)lib_calloc(1, sizeof(vdrive_t));

	vdrive_device_setup(vdrive, 8);
	vdrive->image = diskImage;
	vdrive_attach_image(diskImage, 8, vdrive);
	
	char *commandBuf = SYS_GetCharBuf();
	
	sprintf(commandBuf, "n:%s,%s", diskName, diskId);
	charset_petconvstring((u8 *)commandBuf, 0);

	vdrive_command_execute(vdrive, (u8 *)commandBuf,
			(unsigned int)strlen(commandBuf));

	SYS_ReleaseCharBuf(commandBuf);

	vdrive_detach_image(diskImage, (unsigned int)8, vdrive);
	drive_image_attach(diskImage, 8);
	
	DiskAttached();
	
	lib_free(vdrive);
}

int CDataAdapterViceDrive1541DiskContents::InsertFile(std::filesystem::path filePath, bool alwaysReplace)
{
	int driveId = 0;
	disk_image_t *diskImage = c64d_get_drive_disk_image(driveId);

	if (diskImage == NULL)
	{
		LOGError("CDataAdapterViceDrive1541DiskContents::InsertFile: diskImage is NULL");
		return RET_STATUS_FAILED;
	}
	
	vdrive_t *vdrive = (vdrive_t*)lib_calloc(1, sizeof(vdrive_t));

	vdrive_device_setup(vdrive, 8);
	vdrive->image = diskImage;
	vdrive_attach_image(diskImage, 8, vdrive);

	fileio_info_t *finfo;
	finfo = fileio_open(filePath.string().c_str(), NULL, FILEIO_FORMAT_RAW | FILEIO_FORMAT_P00,
						FILEIO_COMMAND_READ | FILEIO_COMMAND_FSNAME,
						FILEIO_TYPE_PRG);
	if (finfo == NULL)
	{
		LOGError("CDataAdapterViceDrive1541DiskContents::FormatDisk: file not found %s", filePath.string().c_str());
		return RET_STATUS_FAILED;
	}
	
	CSlrString *fileName = new CSlrString((char *)(finfo->name));
	CSlrString *fileNameNoExt = fileName->GetFileNameWithoutExtensionAndPath();
	char *cFileNameNoExt = fileNameNoExt->GetStdASCII();
		
//	char *dest_name = lib_stralloc((char *)filePath.filename().string().c_str()); //(finfo->name));
	char *dest_name = cFileNameNoExt;
	unsigned int dest_len = strlen(cFileNameNoExt); //finfo->length;
	
	bool replaced = false;
	if (vdrive_iec_open(vdrive, (BYTE *)dest_name, (unsigned int)dest_len, 1, NULL))
	{
		LOGError("CDataAdapterViceDrive1541DiskContents::FormatDisk: cannot open `%s' for writing on image", finfo->name);
		
		if (!alwaysReplace)
		{
			fileio_close(finfo);
			lib_free(dest_name);
			return RET_STATUS_FAILED;
		}
		else
		{
			char *bufCommand = SYS_GetCharBuf();

			sprintf(bufCommand, "s:");
			charset_petconvstring((BYTE *)bufCommand, 0);
			strcat(bufCommand, dest_name);

			LOGD("replacing file: %s", bufCommand);
			int status = vdrive_command_execute(vdrive, (BYTE *)bufCommand,
											(unsigned int)strlen(bufCommand));
			LOGD("%02d, %s, 00, 00", status, cbmdos_errortext((unsigned int)status));
			
//			// vdrive_command_execute() returns CBMDOS_IPE_DELETED even if no
//			// files where actually scratched, so just display error messages that
//			// actual mean something, not "ERRORCODE 1" */
//
//			// the below does not work?
//			//			if (status != CBMDOS_IPE_OK && status != CBMDOS_IPE_DELETED)
//			{
//				// pad with spaces
//				char buf[0x11] = "                ";
//				for (int i = 0; i < strlen(dest_name); i++)
//				{
//					buf[i] = dest_name[i];
//					if (i == 0x10)
//						break;
//				}
//
//				sprintf(bufCommand, "s:");
//				charset_petconvstring((BYTE *)bufCommand, 0);
//				strcat(bufCommand, dest_name);
//
//				LOGD("replacing file: %s", bufCommand);
//				status = vdrive_command_execute(vdrive, (BYTE *)bufCommand,
//												(unsigned int)strlen(bufCommand));
//				LOGD("%02d, %s, 00, 00", status, cbmdos_errortext((unsigned int)status));
//			}

			SYS_ReleaseCharBuf(bufCommand);
			
			if (vdrive_iec_open(vdrive, (BYTE *)dest_name, (unsigned int)dest_len, 1, NULL))
			{
				LOGError("CDataAdapterViceDrive1541DiskContents::FormatDisk: cannot open `%s' for writing on image", finfo->name);
				
				fileio_close(finfo);
				lib_free(dest_name);
				return RET_STATUS_FAILED;
			}
			
			replaced = true;
		}
	}
	
	while (1) {
		BYTE c;

		if (fileio_read(finfo, &c, 1) != 1) {
			break;
		}

		if (vdrive_iec_write(vdrive, c, 1)) {
			LOGError("CDataAdapterViceDrive1541DiskContents::FormatDisk: no space on image?");
			break;
		}
	}

	fileio_close(finfo);
	vdrive_iec_close(vdrive, 1);

//	lib_free(dest_name);
	STRFREE(cFileNameNoExt);
	
	vdrive_detach_image(diskImage, (unsigned int)8, vdrive);
	drive_image_attach(diskImage, 8);
	
	DiskAttached();
	
	lib_free(vdrive);
	return replaced ? RET_STATUS_REPLACED : RET_STATUS_OK;
}

int CDataAdapterViceDrive1541DiskContents::GetDataAddressForTrackAndSector(int track, int sector)
{
	if (track < 0)
		track = 0;
	
	if (sector < 0)
		sector = 0;
	
	if (track > numOfTracks-1)
		track = numOfTracks-1;
	
	if (sector > tracks[track].numSectors-1)
		sector = tracks[track].numSectors-1;
	
	return tracks[track].dataOffset + sector*256;
}

Drive1541ContentsIndex *CDataAdapterViceDrive1541DiskContents::GetTrackAndSectorForDataAddress(int dataAddress)
{
	return &dataIndexes[dataAddress];
}

extern "C" {
#include "diskimage.h"
#include "diskconstants.h"
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
#include "CDataAdapterViceDiskDirectory.h"
#include "CDiskImageD64.h"
#include "CViewC64.h"
#include "CViewDrive1541Browser.h"
#include "CDataAddressEditBoxDiskContents.h"


extern "C" {
#define uint32_t u32
disk_image_t *c64d_get_drive_disk_image(int driveId);
gcr_t *c64d_get_drive_disk_gcr(int driveId);
unsigned int disk_image_sector_per_track(unsigned int format, unsigned int track);
#include "diskimage.h"
#include "gcr.h"

disk_image_t *c64d_read_disk_image(char *fileName);
void c64d_disk_image_destroy(disk_image_t *diskImage);

int disk_image_read_sector(const disk_image_t *image, uint8_t *buf, const disk_addr_t *dadr);
disk_image_t *c64d_get_drive_disk_image(int driveId);

}

// Note: in Vice disk_image_t tracks are accessed using disk_addr_t
CDataAdapterViceDrive1541DiskContents::CDataAdapterViceDrive1541DiskContents(CDebugSymbols *debugSymbols, int driveId)
: CDebugDataAdapter("ViceDrive1541DiskContents", debugSymbols)
{
	this->debugInterfaceVice = (CDebugInterfaceVice*)(debugSymbols->debugInterface);
	this->driveId = driveId;
	
	dataIndexes = NULL;
	numOfTracks = 0;
	numOfSectors = 0;
	totalSize = 0;
	
	diskImage = NULL;
	gcr = NULL;
	dataIndexes = NULL;
	tracks = NULL;
}

CDataAdapterViceDiskDirectory::CDataAdapterViceDiskDirectory(CDebugSymbols *symbols,
	int driveId, disk_image_t *image)
: CDataAdapterViceDrive1541DiskContents(symbols, driveId)
{
	if (!image || (image->type != DISK_IMAGE_TYPE_D64 && image->type != DISK_IMAGE_TYPE_G64)
		|| image->tracks < 35 || image->tracks > MAX_TRACKS_1541)
		return;
	if (!image->gcr)
	{
		status = "unavailable";
		reason = "mounted_gcr_unavailable";
		return;
	}
	imageType = image->type == DISK_IMAGE_TYPE_D64 ? "D64" : "G64";
	const char *path = disk_image_name_get(image);
	imagePath = path ? path : "";
	diskImage = image;
	gcr = image->gcr;
	// The base destructor owns this metadata.  No sector cache is populated.
	tracks = new Drive1541ContentsTrack[image->tracks]();
	numOfTracks = image->tracks;
	status = "ok";
	reason = "";
}

int CDataAdapterViceDrive1541DiskContents::AdapterGetDataLength()
{
	return totalSize;
}

int CDataAdapterViceDrive1541DiskContents::GetDebugMemoryCapacity()
{
	// Metadata is created before a disk is attached and must cover every
	// supported full track without changing detached/attached availability.
	return MAX_BLOCKS_1541 * DRIVE1541_SECTOR_SIZE;
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

CDataAdapterViceDrive1541DiskContents::~CDataAdapterViceDrive1541DiskContents()
{
	for (int track = 0; track < numOfTracks; track++)
		delete [] tracks[track].sectors;
	delete [] tracks;
	delete [] dataIndexes;
}

void CDataAdapterViceDrive1541DiskContents::DiskAttached()
{
	debugInterface->LockMutex();
	debugInterface->LockIoMutex();

	for (int track = 0; track < numOfTracks; track++)
		delete [] tracks[track].sectors;
	delete [] tracks;
	delete [] dataIndexes;
	tracks = NULL;
	dataIndexes = NULL;
	numOfTracks = numOfSectors = totalSize = 0;

	diskImage = c64d_get_drive_disk_image(driveId);
	gcr = c64d_get_drive_disk_gcr(driveId);
	if (IsDiskAttached())
	{
		if (diskImage)
		{
			// VICE reports logical tracks, not G64 half-track slots.
			if (diskImage->tracks <= MAX_TRACKS_1541)
				numOfTracks = diskImage->tracks;
			else
				debugInterface->ShowNotificationError("Unsupported geometry for 1541 disk contents");
		}
		else if (gcr)
		{
			// Only full tracks belong in the logical DOS view.  Half-tracks
			// remain accessible through the raw GCR adapter.
			for (int track = 0; track < MAX_TRACKS_1541; track++)
			{
				if (gcr->tracks[track*2].data && gcr->tracks[track*2].size > 0)
					numOfTracks = track+1;
			}
		}
	}

	if (numOfTracks > 0)
	{
		tracks = new Drive1541ContentsTrack[numOfTracks];
		for (int track = 0; track < numOfTracks; track++)
		{
			tracks[track].numSectors = GetNumberOfSectorsPerTrack(track);
			tracks[track].sectors = new Drive1541ContentsSector[tracks[track].numSectors]();
			tracks[track].dataOffset = totalSize;
			tracks[track].isDirty = true;
			numOfSectors += tracks[track].numSectors;
			totalSize += tracks[track].numSectors * 256;
		}

		dataIndexes = new Drive1541ContentsIndex[totalSize];
		int index = 0;
		for (int track = 0; track < numOfTracks; track++)
			for (unsigned int sector = 0; sector < tracks[track].numSectors; sector++)
				for (int byte = 0; byte < 256; byte++)
					dataIndexes[index++] = { (u32)track, sector, (u32)byte };
	}

	RefreshDiskContentsData();
	debugInterface->UnlockIoMutex();
	debugInterface->UnlockMutex();
}

void CDataAdapterViceDrive1541DiskContents::DiskDetached()
{
	LOGD("CDataAdapterViceDrive1541DiskContents::DiskDetached");
	DiskAttached();
}

unsigned int CDataAdapterViceDrive1541DiskContents::GetNumberOfSectorsPerTrack(unsigned int track)
{
	// Standard 1541 logical geometry predicts slots, not sector availability.
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
	*value = 0;
	if (!IsDiskAttached() || !dataIndexes)
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
	Drive1541ContentsSector &sector = tracks[dataIndex.track].sectors[dataIndex.sector];
	*isAvailable = sector.isAvailable;
	if (*isAvailable)
		*value = sector.data[dataIndex.sectorDataOffset];
	
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

bool CDataAdapterViceDrive1541DiskContents::ReadSector(int trackNum, int sectorNum, u8 *buf)
{
	memset(buf, 0, 256);
	if (!IsDiskAttached() || !dataIndexes || trackNum < 0 || trackNum >= numOfTracks
		|| sectorNum < 0 || sectorNum >= (int)tracks[trackNum].numSectors)
		return false;

	if (tracks[trackNum].isDirty)
		RefreshDiskContentsData();

	Drive1541ContentsSector &sector = tracks[trackNum].sectors[sectorNum];
	if (!sector.isAvailable)
		return false;
	memcpy(buf, sector.data, 256);
	return true;
}

void CDataAdapterViceDrive1541DiskContents::AdapterWriteByte(int pointer, uint8 value, bool *isAvailable)
{
	*isAvailable = false;
	if (!dataIndexes || pointer < 0 || pointer >= totalSize)
		return;

	Drive1541ContentsIndex dataIndex = dataIndexes[pointer];
	u8 buf[256];
	if (!ReadSector(dataIndex.track, dataIndex.sector, buf))
		return;
	buf[dataIndex.sectorDataOffset] = value;
	*isAvailable = WriteSector(dataIndex.track, dataIndex.sector, buf);
}

bool CDataAdapterViceDrive1541DiskContents::WriteSector(int trackNum, int sectorNum, u8 *buf)
{
	if (!IsDiskAttached() || !dataIndexes || trackNum < 0 || trackNum >= numOfTracks
		|| sectorNum < 0 || sectorNum >= (int)tracks[trackNum].numSectors)
		return false;
	return DriveContextWriteSector(trackNum, sectorNum, buf);
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
	if (dataIndexes && IsDiskAttached() && (diskImage || gcr))
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
					Drive1541ContentsSector &cachedSector = tracks[track].sectors[sector];
					cachedSector.isAvailable = DriveContextReadSector(track, sector, cachedSector.data);
					
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

bool CDataAdapterViceDrive1541DiskContents::DriveContextReadSector(int trackNum, int sectorNum, u8 *buf)
{
	memset(buf, 0, 256);
	if (!IsDiskAttached() || trackNum < 0 || trackNum >= numOfTracks
		|| sectorNum < 0 || sectorNum >= (int)GetNumberOfSectorsPerTrack(trackNum))
		return false;

	debugInterface->LockIoMutex();
	int status = -1;
	bool success = false;
	if (diskImage)
	{
		disk_addr_t diskAddr = { (u32)trackNum+1, (u32)sectorNum };
		status = disk_image_read_sector(diskImage, buf, &diskAddr);
		success = (status == CBMDOS_IPE_OK);
	}
	else if (gcr)
	{
		int halfTrack = trackNum*2;
		status = gcr_read_sector(&gcr->tracks[halfTrack], buf, sectorNum);
		success = (status == CBMDOS_FDC_ERR_OK);
	}
	if (!success)
	{
		memset(buf, 0, 256);
		LOGError("1541 disk read failed: track=%d sector=%d status=%d", trackNum+1, sectorNum, status);
	}
	debugInterface->UnlockIoMutex();
	return success;
}

bool CDataAdapterViceDrive1541DiskContents::DriveContextWriteSector(int trackNum, int sectorNum, u8 *buf)
{
	if (!IsDiskAttached() || trackNum < 0 || trackNum >= numOfTracks
		|| sectorNum < 0 || sectorNum >= (int)GetNumberOfSectorsPerTrack(trackNum))
		return false;

	debugInterface->LockIoMutex();
	int status = -1;
	bool success = false;
	if (diskImage && !diskImage->read_only)
	{
		disk_addr_t diskAddr = { (u32)trackNum+1, (u32)sectorNum };
		status = disk_image_write_sector(diskImage, buf, &diskAddr);
		success = (status == 0);
	}
	else if (!diskImage && gcr)
	{
		int halfTrack = trackNum*2;
		status = gcr_write_sector(&gcr->tracks[halfTrack], buf, sectorNum);
		success = (status == CBMDOS_FDC_ERR_OK);
	}
	if (!success)
		LOGError("1541 disk write failed: track=%d sector=%d status=%d", trackNum+1, sectorNum, status);

	// Re-read even after a failed write: VICE may have partially changed data.
	MarkTrackDirty(trackNum);
	debugInterface->UnlockIoMutex();
	return success;
}

void CDataAdapterViceDrive1541DiskContents::MarkTrackDirty(int track)
{
	if (tracks && track >= 0 && track < numOfTracks)
		tracks[track].isDirty = true;
}

// GUI vdrive writers share mounted GCR with CPU-task readers.  Release I/O
// before DiskAttached takes the interface mutex, preserving lock order.
namespace
{
class DiskContentsIoLock
{
	CDebugInterface *debugInterface;
public:
	explicit DiskContentsIoLock(CDebugInterface *debugInterface) : debugInterface(debugInterface)
	{
		debugInterface->LockIoMutex();
	}
	~DiskContentsIoLock() { Unlock(); }
	void Unlock()
	{
		if (!debugInterface) return;
		debugInterface->UnlockIoMutex();
		debugInterface = nullptr;
	}
	DiskContentsIoLock(const DiskContentsIoLock &) = delete;
	DiskContentsIoLock &operator=(const DiskContentsIoLock &) = delete;
};
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

	DiskContentsIoLock ioLock(debugInterface);
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
	vdrive_attach_image(diskImage, 8, 0, vdrive);
	
	char *commandBuf = SYS_GetCharBuf();
	
	sprintf(commandBuf, "n:%s,%s", diskName, diskId);
	charset_petconvstring((u8 *)commandBuf, 0);

	vdrive_command_execute(vdrive, (u8 *)commandBuf,
			(unsigned int)strlen(commandBuf));

	SYS_ReleaseCharBuf(commandBuf);

	vdrive_detach_image(diskImage, (unsigned int)8, 0, vdrive);
	drive_image_attach(diskImage, 8);
	
	ioLock.Unlock();
	DiskAttached();
	
	lib_free(vdrive);
}

int CDataAdapterViceDrive1541DiskContents::InsertFile(std::filesystem::path filePath, bool alwaysReplace)
{
	DiskContentsIoLock ioLock(debugInterface);
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
	vdrive_attach_image(diskImage, 8, 0, vdrive);

	fileio_info_t *finfo;
	finfo = fileio_open(filePath.string().c_str(), NULL, FILEIO_FORMAT_RAW | FILEIO_FORMAT_P00,
						FILEIO_COMMAND_READ | FILEIO_COMMAND_FSNAME,
						FILEIO_TYPE_PRG, NULL);
	if (finfo == NULL)
	{
		LOGError("CDataAdapterViceDrive1541DiskContents::FormatDisk: file not found %s", filePath.string().c_str());
		return RET_STATUS_FAILED;
	}
	
	CSlrString *fileName = new CSlrString((char *)(finfo->name));
	CSlrString *fileNameNoExt = fileName->GetFileNameWithoutExtensionAndPath();
	char *cFileNameNoExt = fileNameNoExt->GetStdASCII();
		
//	char *dest_name = lib_strdup((char *)filePath.filename().string().c_str()); //(finfo->name));
	char *dest_name = cFileNameNoExt;
	unsigned int dest_len = strlen(cFileNameNoExt); //finfo->length;
	
	bool replaced = false;
	if (vdrive_iec_open(vdrive, (uint8_t *)dest_name, (unsigned int)dest_len, 1, NULL))
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
			charset_petconvstring((uint8_t *)bufCommand, 0);
			strcat(bufCommand, dest_name);

			LOGD("replacing file: %s", bufCommand);
			int status = vdrive_command_execute(vdrive, (uint8_t *)bufCommand,
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
//				charset_petconvstring((uint8_t *)bufCommand, 0);
//				strcat(bufCommand, dest_name);
//
//				LOGD("replacing file: %s", bufCommand);
//				status = vdrive_command_execute(vdrive, (uint8_t *)bufCommand,
//												(unsigned int)strlen(bufCommand));
//				LOGD("%02d, %s, 00, 00", status, cbmdos_errortext((unsigned int)status));
//			}

			SYS_ReleaseCharBuf(bufCommand);
			
			if (vdrive_iec_open(vdrive, (uint8_t *)dest_name, (unsigned int)dest_len, 1, NULL))
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
		uint8_t c;

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
	
	vdrive_detach_image(diskImage, (unsigned int)8, 0, vdrive);
	drive_image_attach(diskImage, 8);
	
	ioLock.Unlock();
	DiskAttached();
	
	lib_free(vdrive);
	return replaced ? RET_STATUS_REPLACED : RET_STATUS_OK;
}

int CDataAdapterViceDrive1541DiskContents::GetDataAddressForTrackAndSector(int track, int sector)
{
	if (!dataIndexes || numOfTracks == 0)
		return -1;
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
	if (!dataIndexes || dataAddress < 0 || dataAddress >= totalSize)
		return NULL;
	return &dataIndexes[dataAddress];
}

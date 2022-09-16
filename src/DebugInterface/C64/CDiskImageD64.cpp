extern "C" {
#include "diskimage.h"
};

#include "CGuiMain.h"
#include "CDiskImageD64.h"
#include "SYS_Main.h"
#include "SYS_Funct.h"

extern "C" {
	disk_image_t *c64d_read_disk_image(char *fileName);
	void c64d_disk_image_destroy(disk_image_t *diskImage);

	int disk_image_read_sector(const disk_image_t *image, BYTE *buf, const disk_addr_t *dadr);
	disk_image_t *c64d_get_drive_disk_image(int driveId);
};

CDiskImageD64::CDiskImageD64(int driveId)
{
	this->SetDiskImage(driveId);
}

// get disk image from file
CDiskImageD64::CDiskImageD64(char *fileName)
{
	this->SetDiskImage(fileName);
}

void CDiskImageD64::SetDiskImage(int driveId)
{
	isFromFile = false;
	
	this->diskImage = c64d_get_drive_disk_image(driveId);

	if (this->diskImage != NULL)
	{
		this->ReadImage();
	}
}

void CDiskImageD64::SetDiskImage(char *fileName)
{
	isFromFile = true;
	
	this->diskImage = c64d_read_disk_image(fileName);
	this->ReadImage();
}

DiskImageFileEntry *CDiskImageD64::FindDiskPRGEntry(int entryNum)
{
	int i = 0;
	for (std::vector<DiskImageFileEntry *>::iterator it = fileEntries.begin(); it != fileEntries.end(); it++)
	{
		DiskImageFileEntry *file = *it;
		if (file->fileType == 0x02)
		{
			if (i == entryNum)
			{
				return file;
			}
			
			i++;
		}
	}
	
	return NULL;
}



CDiskImageD64::~CDiskImageD64()
{
	if (isFromFile)
	{
		c64d_disk_image_destroy(this->diskImage);
	}
}

bool CDiskImageD64::ReadImage()
{
	LOGD("CDiskImageD64::ReadImage");
	
	u8 sectorData[256];
	
	// read BAM
	disk_addr_t diskAddr;
	diskAddr.track = D64_BAM_TRACK;
	diskAddr.sector = D64_BAM_SECTOR;
	disk_image_read_sector(diskImage, sectorData, &diskAddr);
	
	int offset = 0x90;
	for (int i = 0; i < 0x10; i++)
	{
		diskName[i] = sectorData[offset + i];
		
		//LOGD("offset=%d diskName[%d]=%02x '%c'", offset+i, i, diskName[i], diskName[i]);
	}
	
	offset = 0xA0;
	for (int i = 0; i < 7; i++)
	{
		diskId[i] = sectorData[offset + i];
		//LOGD("offset=%d diskId[%d]=%02x '%c'", offset+i, i, diskId[i], diskId[i]);
	}
	
	// scan BAM track entries for free sectors
	numFreeSectors = 0;
	
	for (int i = 1; i < (D64_LAST_TRACK+1); i++)
	{
		offset = 4*i;
		int freeSectors = sectorData[offset];
		numFreeSectors += freeSectors;
		
		//LOGD("...track %d freeSectors=%d numFreeSectors=%d", i, freeSectors, numFreeSectors);
	}
	
	// remove track 18
	offset = 4 * D64_BAM_TRACK;
	int freeSectorsTrackBAM = sectorData[offset];
	numFreeSectors -= freeSectorsTrackBAM;
	
	LOGD("numFreeSectors=%d", numFreeSectors);
	
	diskAddr.track = D64_BAM_TRACK;
	diskAddr.sector = 1;

	while(diskAddr.track != 0)
	{
		LOGD("--> dirTrack=%d dirSector=%d", diskAddr.track, diskAddr.sector);

		disk_image_read_sector(diskImage, sectorData, &diskAddr);

		
		for (int entryNum = 0; entryNum < 8; entryNum++)
		{
			//LOGD("   entryNum=%d", entryNum);
			
			int entryOffset = entryNum * 0x20;
			
			u8 fileType = sectorData[entryOffset + 2];
			//LOGD("    fileType=%02x", fileType);
			
			if (fileType != 0x00)
			{
				DiskImageFileEntry *fileEntry = new DiskImageFileEntry;
				fileEntry->fileType = fileType & 0x07;
				fileEntry->locked = ((fileType & 0x40) == 0x40);
				fileEntry->closed = ((fileType & 0x80) == 0x80);
				
				fileEntry->track = sectorData[entryOffset + 3];
				fileEntry->sector = sectorData[entryOffset + 4];

				LOGD("  type=%02x locked=%d closed=%d | track=%d sector=%d",
					 fileEntry->fileType, fileEntry->locked, fileEntry->closed,
					 fileEntry->track, fileEntry->sector);
				
				for (int i = 0; i < 0x10; i++)
				{
					fileEntry->fileName[i] = sectorData[entryOffset + 5 + i];
					
					//LOGD("  fileName[%d]=%02x '%c'", i, fileEntry->fileName[i], fileEntry->fileName[i]);
				}

				fileEntry->fileSize = sectorData[entryOffset + 0x1E] + sectorData[entryOffset + 0x1F] * 0x100;
				
				LOGD("  fileSize=%d", fileEntry->fileSize);
				
				this->fileEntries.push_back(fileEntry);
			}
			
		}
		
		// next dir track
		diskAddr.track = sectorData[0];
		diskAddr.sector = sectorData[1];
		
		if (this->fileEntries.size() >= D64_MAX_FILES)
			break;
	}
	LOGD("--> done dir");
	
	LOGD("CDiskImageD64::ReadImage: done");
	return true;
}

bool CDiskImageD64::ReadEntry(DiskImageFileEntry *fileEntry, CByteBuffer *byteBuffer)
{
	char *buf = SYS_GetCharBuf();
	
	memcpy(buf, fileEntry->fileName, 0x10);
	buf[0x10] = 0x00;
	
	LOGD("CDiskImageD64::ReadEntry: reading fileName='%s' fileType=%d", buf, fileEntry->fileType);
	
	SYS_ReleaseCharBuf(buf);
	
	if (fileEntry->fileType < 1 || fileEntry->fileType > 4)
	{
		// file is DEL
		LOGError("CDiskImageD64::ReadEntry: cannot read fileType=%d", fileEntry->fileType);
		return false;
	}
	
	if (fileEntry->track == 0)
	{
		LOGError("CDiskImageD64::ReadEntry: cannot read track=%d", fileEntry->fileType);
		return false;
	}
	
	byteBuffer->Rewind();
	
	u8 sectorData[256];

	disk_addr_t diskAddr;
	
	diskAddr.track = fileEntry->track;
	diskAddr.sector = fileEntry->sector;
	
	while(1)
	{
		LOGD("..reading track=%d sector=%d", diskAddr.track, diskAddr.sector);
		
		disk_image_read_sector(diskImage, sectorData, &diskAddr);

		diskAddr.track = sectorData[0];
		diskAddr.sector = sectorData[1];
		
		if (diskAddr.track == 0)
			break;
		
		for (int i = 2; i < 256; i++)
		{
			byteBuffer->PutU8(sectorData[i]);
			//LOGD("...... prg [%04x | %6d] = %02x", 0x07FF + byteBuffer->index-1, byteBuffer->index-1, data[offset+i]);
		}
		
		// sanity check
		if (byteBuffer->length > 0x10000)
		{
			LOGError("Broken file entry");
		
			char *buf = SYS_GetCharBuf();
			sprintf(buf, "File entry is larger than 64kB:\n%s.%s (size %d)", fileEntry->fileName, FileEntryTypeToStr(fileEntry->fileType), fileEntry->fileSize);
			guiMain->ShowMessageBox("Broken D64 file entry", buf);
			SYS_ReleaseCharBuf(buf);
			return false;
		}
	}
	
	LOGD("..finishing sector data=%d", diskAddr.sector);
	
	diskAddr.sector += 1;
	for (int i = 2; i < diskAddr.sector; i++)
	{
		byteBuffer->PutU8(sectorData[i]);
		//LOGD("...... prg [%04x | %6d] = %02x", 0x07FF + byteBuffer->index-1, byteBuffer->index-1, data[offset+i]);
	}
	
	byteBuffer->Rewind();
	return true;
}

bool CDiskImageD64::ReadEntry(int entryNum, CByteBuffer *byteBuffer)
{
	if (entryNum > fileEntries.size()-1)
	{
		LOGError("CDiskImageD64::ReadEntry: entryNum=%d fileEntries.size()=%d", entryNum, fileEntries.size());
		return false;
	}
	
	DiskImageFileEntry *fileEntry = fileEntries[entryNum];
	
	return ReadEntry(fileEntry, byteBuffer);
}

const char *CDiskImageD64::FileEntryTypeToStr(u8 fileType)
{
	if (fileType == 0x00)
	{
		return "DEL";
	}
	else if (fileType == 0x01)
	{
		return "SEQ";
	}
	else if (fileType == 0x02)
	{
		return "PRG";
	}
	else if (fileType == 0x03)
	{
		return "USR";
	}
	else if (fileType == 0x04)
	{
		return "REL";
	}
	return "?";
}



/*
 this is pure u8 *data version:
 
 // this is based on http://unusedino.de/ec64/technical/formats/d64.html
 DiskImageTrack d64TracksOffsets[D64_NUM_TRACKS+1] = {
	{	 0,       0,       0		},
	{	21,       0,       0x00000	},
	{	21,      21,       0x01500	},
	{	21,      42,       0x02A00	},
	{	21,      63,       0x03F00	},
	{	21,      84,       0x05400	},
	{	21,     105,       0x06900	},
	{	21,     126,       0x07E00	},
	{	21,     147,       0x09300	},
	{	21,     168,       0x0A800	},
	{	21,     189,       0x0BD00	},
	{	21,     210,       0x0D200	},
	{	21,     231,       0x0E700	},
	{	21,     252,       0x0FC00	},
	{	21,     273,       0x11100	},
	{	21,     294,       0x12600	},
	{	21,     315,       0x13B00	},
	{	21,     336,       0x15000	},
	{	19,     357,       0x16500	},
	{	19,     376,       0x17800	},
	{	19,     395,       0x18B00	},
	{	19,     414,       0x19E00	},
	{	19,     433,       0x1B100	},
	{	19,     452,       0x1C400	},
	{	19,     471,       0x1D700	},
	{	18,     490,       0x1EA00	},
	{	18,     508,       0x1FC00	},
	{	18,     526,       0x20E00	},
	{	18,     544,       0x22000	},
	{	18,     562,       0x23200	},
	{	18,     580,       0x24400	},
	{	17,     598,       0x25600	},
	{	17,     615,       0x26700	},
	{	17,     632,       0x27800	},
	{	17,     649,       0x28900	},
	{	17,     666,       0x29A00	},
	{	17,     683,       0x2AB00	},
	{	17,     700,       0x2BC00	},
	{	17,     717,       0x2CD00	},
	{	17,     734,       0x2DE00	},
	{	17,     751,       0x2EF00	}
 };
 

 //	u8 *data;
	//	int dataLength;
	//	static int GetSectorOffset(int track, int sector);
	

CDiskImageD64::CDiskImageD64(CByteBuffer *byteBuffer)
 {
 

 // TEST:
	disk_addr_t diskAddr;
	diskAddr.track = 18;
	diskAddr.sector = 1;
 
	u8 *sectorData = new u8[256];
 
	disk_image_read_sector(diskImage, sectorData, &diskAddr);
	
	//c64d_read_sector_from_drive_image(0, 18, 0, data);
	
	for (int i = 0; i < 256; i++)
	{
 LOGD(" sectorData[%d] = %02x '%c'", i, sectorData[i], sectorData[i]);
	}
	
	
 //	this->dataLength = byteBuffer->length;
 //
 //	this->data = new u8[byteBuffer->length];
 //	memcpy(this->data, byteBuffer->data, byteBuffer->length);
 
	
	/////////

 
 bool CDiskImageD64::ReadImage()
 {
	LOGD("CDiskImageD64::ReadImage");
	int bamOffset = GetSectorOffset(D64_BAM_TRACK, D64_BAM_SECTOR);
	
	//LOGD("bamOffset=%d", bamOffset);
	
	int offset = bamOffset + 0x90;
	for (int i = 0; i < 0x10; i++)
	{
 diskName[i] = data[offset + i];
 
 //LOGD("offset=%d diskName[%d]=%02x '%c'", offset+i, i, diskName[i], diskName[i]);
	}
	
	offset = bamOffset + 0xA0;
	for (int i = 0; i < 7; i++)
	{
 diskId[i] = data[offset + i];
 //LOGD("offset=%d diskId[%d]=%02x '%c'", offset+i, i, diskId[i], diskId[i]);
	}
	
	// scan BAM track entries for free sectors
	numFreeSectors = 0;
	
	for (int i = 1; i < (D64_LAST_TRACK+1); i++)
	{
 offset = bamOffset + 4*i;
 int freeSectors = data[offset];
 numFreeSectors += freeSectors;
 
 //LOGD("...track %d freeSectors=%d numFreeSectors=%d", i, freeSectors, numFreeSectors);
	}
	
	// remove track 18
	offset = bamOffset + 4 * D64_BAM_TRACK;
	int freeSectorsTrackBAM = data[offset];
	numFreeSectors -= freeSectorsTrackBAM;
	
	LOGD("numFreeSectors=%d", numFreeSectors);
	
	int dirTrack = D64_BAM_TRACK;
	int dirSector = 1;
	while(dirTrack != 0)
	{
 LOGD("--> dirTrack=%d dirSector=%d", dirTrack, dirSector);
 int dirOffset = GetSectorOffset(dirTrack, dirSector);
 
 for (int entryNum = 0; entryNum < 8; entryNum++)
 {
 //LOGD("   entryNum=%d", entryNum);
 
 int entryOffset = dirOffset + entryNum * 0x20;
 
 u8 fileType = data[entryOffset + 2];
 //LOGD("    fileType=%02x", fileType);
 
 if (fileType != 0x00)
 {
 DiskImageFileEntry *fileEntry = new DiskImageFileEntry;
 fileEntry->fileType = fileType & 0x07;
 fileEntry->locked = ((fileType & 0x40) == 0x40);
 fileEntry->closed = ((fileType & 0x80) == 0x80);
 
 fileEntry->track = data[entryOffset + 3];
 fileEntry->sector = data[entryOffset + 4];
 
 //				LOGD("  type=%02x locked=%d closed=%d | track=%d sector=%d",
 //					 fileEntry->fileType, fileEntry->locked, fileEntry->closed,
 //					 fileEntry->track, fileEntry->sector);
 
 for (int i = 0; i < 0x10; i++)
 {
 fileEntry->fileName[i] = data[entryOffset + 5 + i];
 
 //					LOGD("  fileName[%d]=%02x '%c'", i, fileEntry->fileName[i], fileEntry->fileName[i]);
 }
 
 fileEntry->fileSize = data[entryOffset + 0x1E] + data[entryOffset + 0x1F] * 0x100;
 
 //				LOGD("  fileSize=%d", fileEntry->fileSize);
 
 this->fileEntries.push_back(fileEntry);
 }
 
 }
 
 // next dir track
 dirTrack = data[dirOffset + 0];
 dirSector = data[dirOffset + 1];
 
 if (this->fileEntries.size() >= D64_MAX_FILES)
 break;
	}
	LOGD("--> done dir");
	
	LOGD("CDiskImageD64::ReadImage: done");
	return true;
 }
 
 bool CDiskImageD64::ReadEntry(DiskImageFileEntry *fileEntry, CByteBuffer *byteBuffer)
 {
	char *buf = SYS_GetCharBuf();
	
	memcpy(buf, fileEntry->fileName, 0x10);
	buf[0x10] = 0x00;
	
	LOGD("CDiskImageD64::ReadEntry: reading fileName='%s' fileType=%d", buf, fileEntry->fileType);
	
	SYS_ReleaseCharBuf(buf);
	
	if (fileEntry->fileType < 1 || fileEntry->fileType > 4)
	{
 // file is DEL
 LOGError("CDiskImageD64::ReadEntry: cannot read fileType=%d", fileEntry->fileType);
 return false;
	}
	
	if (fileEntry->track == 0)
	{
 LOGError("CDiskImageD64::ReadEntry: cannot read track=%d", fileEntry->fileType);
 return false;
	}
	
	byteBuffer->Rewind();
	
	int track = fileEntry->track;
	int sector = fileEntry->sector;
	int offset = 0;
	
	while(1)
	{
 LOGD("..reading track=%d sector=%d", track, sector);
 offset = GetSectorOffset(track, sector);
 
 track = data[offset];
 sector = data[offset+1];
 
 if (track == 0)
 break;
 
 for (int i = 2; i < 256; i++)
 {
 byteBuffer->PutU8(data[offset + i]);
 //LOGD("...... prg [%04x | %6d] = %02x", 0x07FF + byteBuffer->index-1, byteBuffer->index-1, data[offset+i]);
 }
 
 
	}
	
	LOGD("..finishing sector data=%d", sector);
	
	sector += 1;
	for (int i = 2; i < sector; i++)
	{
 byteBuffer->PutU8(data[offset + i]);
 //LOGD("...... prg [%04x | %6d] = %02x", 0x07FF + byteBuffer->index-1, byteBuffer->index-1, data[offset+i]);
	}
	
	byteBuffer->Rewind();
	return true;
 }
 
 int CDiskImageD64::GetSectorOffset(int track, int sector)
 {
	return d64TracksOffsets[track].offset + sector*256;
 }
 

 
*/



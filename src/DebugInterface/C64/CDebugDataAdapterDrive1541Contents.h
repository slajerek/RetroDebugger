//#ifndef _CDataAdapterDrive1541DiskContents_h_
//#define _CDataAdapterDrive1541DiskContents_h_
//
//class CDataAdapterDrive1541DiskContents : public CDebugDataAdapter
//{
//public:
//	CDataAdapterDrive1541DiskContents(CDebugInterfaceVice *debugInterfaceVice, int driveId);
//
//	virtual void DiskAttached();
//	virtual void DiskDetached();
//
//	virtual u32 GetNumberOfSectorsPerTrack(u32 track);
//	virtual void RefreshDiskContentsData();
//
//	virtual void DriveContextReadSector(int trackNum, int sectorNum, u8 *buf);
//	virtual void DriveContextWriteSector(int trackNum, int sectorNum, u8 *buf);
//	
////	virtual void MarkWholeDiskDirty();
//	virtual void MarkTrackDirty(int trackNum);
//	
//	virtual void ReadSector(int trackNum, int sectorNum, u8 *buf);
//	virtual void WriteSector(int trackNum, int sectorNum, u8 *buf);
//};
//
//#endif

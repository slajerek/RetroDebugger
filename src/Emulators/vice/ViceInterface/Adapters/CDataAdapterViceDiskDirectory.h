#ifndef _CDATAADAPTERVICEDISKDIRECTORY_H_
#define _CDATAADAPTERVICEDISKDIRECTORY_H_

#include "CDataAdapterViceDrive1541DiskContents.h"
#include <string>

// Task-local, uncached view.  Construct/use/destroy only on the CPU thread with
// the interface and I/O mutexes already held.  Never call DiskAttached(): it
// refreshes the shared browser dirty flag and builds a whole-disk byte index.
class CDataAdapterViceDiskDirectory : public CDataAdapterViceDrive1541DiskContents
{
public:
	CDataAdapterViceDiskDirectory(CDebugSymbols *symbols, int driveId, disk_image_t *image);
	const char *status = "unsupported";
	const char *reason = "image_type_or_geometry";
	std::string imageType;
	std::string imagePath;

	bool ReadSector(int track, int sector, u8 *bytes) override
	{
		return DriveContextReadSector(track, sector, bytes);
	}
};

#endif

#pragma once
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <filesystem>
using u8 = uint8_t;
using uint8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
#undef LOGD
#undef LOGError
#define LOGD(...) ((void)0)
#define LOGError(...) fprintf(stderr, __VA_ARGS__), fputc('\n', stderr)
#define STRFREE(p) free(p)
inline u64 SYS_GetCurrentTimeInMillis() { return 0; }
inline char *SYS_GetCharBuf() { static char buffer[4096]; return buffer; }
inline void SYS_ReleaseCharBuf(char *) {}
class CDataAdapterViceDrive1541DiskContents;
class CDebugInterface {
public:
 void LockMutex() {}
 void UnlockMutex() {}
 void LockIoMutex() {}
 void UnlockIoMutex() {}
 void ShowNotificationError(const char *) {}
};
class CDebugInterfaceVice : public CDebugInterface {
public:
 CDataAdapterViceDrive1541DiskContents *dataAdapterDrive1541DiskContents = nullptr;
 void SetDriveDirtyForRefreshFlag(int) {}
};
class CDebugSymbols {
public:
 CDebugInterface *debugInterface;
};
class CDataAddressEditBoxCallback {};
class CDataAddressEditBox { public: virtual ~CDataAddressEditBox() {} };
class CDataAddressEditBoxDiskContents : public CDataAddressEditBox {
public:
 CDataAddressEditBoxDiskContents(CDataAdapterViceDrive1541DiskContents *) {}
 void SetCallback(CDataAddressEditBoxCallback *) {}
};
class CDebugDataAdapter {
public:
 CDebugInterface *debugInterface;
 CDebugDataAdapter(const char *, CDebugSymbols *symbols) : debugInterface(symbols->debugInterface) {}
 virtual ~CDebugDataAdapter() {}
};
class CByteBuffer {
public:
 std::vector<u8> bytes;
 int index = 0, length = 0;
 void Rewind() { index = 0; }
 void Clear() { bytes.clear(); index = length = 0; }
 void PutU8(u8 value) {
  if (index == (int)bytes.size()) bytes.push_back(value); else bytes[index] = value;
  length = ++index;
 }
};
class CSlrString {
public:
 CSlrString(char *) {}
 CSlrString *GetFileNameWithoutExtensionAndPath() { return this; }
 char *GetStdASCII() { char *p = (char *)malloc(1); p[0] = 0; return p; }
};
class CGuiMain { public: void ShowMessageBox(const char *, const char *) {} };
extern CGuiMain *guiMain;
extern "C" unsigned int c64d_get_drive_is_disk_attached(int);

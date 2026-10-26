// Actual adapter + extracted endpoint reader + real VICE drive_image_attach,
// disk_image_read_image, GCR reload, parser, and codec.  Host UI/fileio/vdrive
// command semantics are seams, NOT a native DOS/GUI execution claim.
#include "TestSdk.h"
#undef LOGD
#undef LOGError
extern "C" {
#include "diskimage.h"
#include "diskconstants.h"
#include "fsimage.h"
#include "fsimage-gcr.h"
#include "gcr.h"
#include "lib.h"
#include "log.h"
#include "util.h"
#include "vdrive.h"
#include "vdrive-command.h"
#include "vdrive-iec.h"
#include "driveimage.h"
#include "fileio.h"
#include "cbmimage.h"
#include "charset.h"
}
// Reuse the original disk authoring helpers unchanged.  Rename only its host
// service definitions so this separate fixture can install per-unit contexts
// and reach the actual drive reattachment/reload, rather than its failure stub.
#define c64d_get_drive_disk_image unused_get_image
#define c64d_get_drive_disk_gcr unused_get_gcr
#define c64d_get_drive_is_disk_attached unused_is_attached
#define lib_free unused_lib_free
#define drive_image_attach unused_drive_attach
#define drive_get_disk_drive_type unused_drive_type
#define vdrive_device_setup unused_setup
#define vdrive_attach_image unused_vattach
#define vdrive_detach_image unused_vdetach
#define vdrive_command_execute unused_command
#define vdrive_iec_open unused_open
#define vdrive_iec_write unused_write
#define vdrive_iec_close unused_close
#define fileio_open unused_file_open
#define fileio_read unused_file_read
#define main unused_upstream_main
#include "../disk_contents/CDataAdapterViceDrive1541DiskContents_test.cpp"
#undef main
#undef c64d_get_drive_disk_image
#undef c64d_get_drive_disk_gcr
#undef c64d_get_drive_is_disk_attached
#undef lib_free
#undef drive_image_attach
#undef drive_get_disk_drive_type
#undef vdrive_device_setup
#undef vdrive_attach_image
#undef vdrive_detach_image
#undef vdrive_command_execute
#undef vdrive_iec_open
#undef vdrive_iec_write
#undef vdrive_iec_close
#undef fileio_open
#undef fileio_read

#include "CDataAdapterViceDiskDirectory.h"
#include "CDiskDirectoryRequest.h"
#include "CDiskDirectoryProtocol.h"
#include <future>
#include <map>
#include <iostream>
using json = nlohmann::json;
extern "C" disk_image_t *c64d_get_drive_disk_image(int);
#include "reader.inc"

static disk_image_t *units[4]{};
static CDebugInterfaceVice *activeInterface;
static thread_local bool inWriter;
static bool throwAtAcquisition;
static const void *reloadPointer;
static std::map<std::string, int> stages;
static int unsafeStages;
static bool byteRead;

static void probe(const char *stage) {
 if (!inWriter) return;
 ++stages[stage];
 auto observed = std::async(std::launch::async, [] {
  // Test the reader itself, not an unrelated model mutex.  Check I/O exclusion
  // separately because PerformFormat's outer interface lock may refuse first.
  bool free = activeInterface->ioMutex->TryLock();
  if (free) activeInterface->ioMutex->Unlock();
  return std::make_pair(free, ReadMountedDiskDirectory(activeInterface, 8));
 }).get();
 bool excluded = !observed.first && observed.second.value("status", "") == "unavailable"
     && (observed.second.value("reason", "") == "media_locked"
         || observed.second.value("reason", "") == "interface_locked");
 if (!excluded) ++unsafeStages;
 std::cout << "site=" << stage << " excluded=" << excluded
           << " reply=" << observed.second.value("status", "")
           << "/" << observed.second.value("reason", "") << "\n";
}

static void mutate(vdrive_t *vdrive) {
 uint8_t bam[256];
 auto *gcr = vdrive->image->gcr;
 check(gcr_read_sector(&gcr->tracks[34], bam, 0) == CBMDOS_FDC_ERR_OK, "mutation reads BAM");
 bam[0x90] = 'M';
 check(gcr_write_sector(&gcr->tracks[34], bam, 0) == CBMDOS_FDC_ERR_OK, "real mounted GCR mutation");
 ++writes;
 probe("mutation");
}

extern "C" {
int log_verbose(log_t, const char *, ...) { return 0; }
void ViceLOGD(const char *, ...) {}
void directory_test_drive_setup(disk_image_t *image);
int directory_test_drive_attached(disk_image_t *image);
void directory_test_reattach_complete() { probe("reattach_complete"); }
disk_image_t *c64d_get_drive_disk_image(int unit) {
 if (inWriter && throwAtAcquisition) throw std::runtime_error("injected image acquisition");
 probe("image_acquisition");
 return unit >= 0 && unit < 4 ? units[unit] : nullptr;
}
gcr_t *c64d_get_drive_disk_gcr(int unit) { return units[unit] ? units[unit]->gcr : nullptr; }
unsigned int c64d_get_drive_is_disk_attached(int unit) { return unit >= 0 && unit < 4 && units[unit]; }
void lib_free(const void *p) {
 if (p && p == reloadPointer && inWriter) { probe("gcr_reload_free"); reloadPointer = nullptr; }
 free(const_cast<void *>(p));
}
void *lib_realloc(void *p, size_t n) { return realloc(p, n); }
int fsimage_read_p64_image(const disk_image_t *) { return -1; }
int vdrive_device_setup(vdrive_t *, unsigned int) { return 0; }
int vdrive_attach_image(disk_image_t *, unsigned int, unsigned int, vdrive_t *) { return 0; }
void vdrive_detach_image(disk_image_t *image, unsigned int, unsigned int, vdrive_t *) {
 reloadPointer = image->gcr->tracks[0].data;
}
int vdrive_command_execute(vdrive_t *vdrive, const uint8_t *, unsigned int) { mutate(vdrive); return 0; }
int vdrive_iec_open(vdrive_t *, const uint8_t *, unsigned int, unsigned int, cbmdos_cmd_parse_s *) { return 0; }
int vdrive_iec_write(vdrive_t *vdrive, uint8_t, unsigned int) { mutate(vdrive); return 0; }
int vdrive_iec_close(vdrive_t *, unsigned int) { return 0; }
fileio_info_t *fileio_open(const char *, const char *, unsigned int, unsigned int, unsigned int, int *) {
 static uint8_t name[] = "test.prg";
 static fileio_info_t info{}; info.name = name; byteRead = false; return &info;
}
unsigned int fileio_read(fileio_info_t *, uint8_t *byte, unsigned int) {
 if (byteRead) return 0;
 byteRead = true; *byte = 42; return 1;
}
}

static void routing(const std::string &dir) {
 CDebugInterfaceVice di; CDebugSymbols symbols{&di}; di.symbolsDrive1541DiskContents = &symbols;
 std::vector<std::unique_ptr<Image>> raw, flat;
 for (int unit = 0; unit < 4; ++unit) {
  auto disk = authoredDisk(35);
  putName(disk.data()+offset(18,0)+0x90, ("UNIT"+std::to_string(unit+8)).c_str(), 16);
  putName(disk.data()+offset(18,1)+5, ("FILE"+std::to_string(unit+8)).c_str(), 16);
  std::string path = dir+"/unit"+std::to_string(unit);
  makeG64(path+".g64", disk, 35, 84);
  raw.emplace_back(new Image(path+".g64", DISK_IMAGE_TYPE_G64, 42, 84));
  // Persisted D64 intentionally disagrees with mounted GCR on every unit.
  writeFile(path+".d64", authoredDisk(35));
  flat.emplace_back(new Image(path+".d64", DISK_IMAGE_TYPE_D64, 35));
  flat.back()->image.gcr = &raw.back()->gcr;
 }
 for (bool d64 : {false, true}) {
  for (int unit = 0; unit < 4; ++unit) units[unit] = d64 ? &flat[unit]->image : &raw[unit]->image;
  for (int unit = 0; unit < 4; ++unit) {
   auto before = raw[unit]->rawBytes();
   auto result = ReadMountedDiskDirectory(&di, unit+8);
   check(result["status"] == "ok" && result["device"] == unit+8, "per-unit successful response");
   check(result["diskName"]["display"] == "UNIT"+std::to_string(unit+8)
       && result["entries"][0]["name"]["display"] == "FILE"+std::to_string(unit+8), "actual selected-unit mounted contents");
   check(raw[unit]->rawBytes() == before && writes == 0, "routing reader is nonmutating");
  }
  units[2] = nullptr;
  check(ReadMountedDiskDirectory(&di, 10)["reason"] == "no_mounted_disk", "one detached unit does not fall back to unit 8");
 }
 std::fill(std::begin(units), std::end(units), nullptr);
}

static void writer(const std::string &dir, const std::string &mode) {
 auto disk = authoredDisk(35);
 makeG64(dir+"/writer.g64", disk, 35, 84);
 Image image(dir+"/writer.g64", DISK_IMAGE_TYPE_G64, 42, 84);
 units[0] = &image.image;
 directory_test_drive_setup(&image.image);
 CDebugInterfaceVice di; CDebugSymbols symbols{&di}; di.symbolsDrive1541DiskContents = &symbols;
 CDataAdapterViceDrive1541DiskContents adapter(&symbols, 0);
 activeInterface = &di;
 const auto before = image.rawBytes();
 auto invoke = [&] {
  if (mode == "insert") check(adapter.InsertFile("test.prg", false) == RET_STATUS_OK, "insert keeps success meaning");
  else adapter.FormatDisk("test", "id");
 };
 // Equivalent outer lock to the existing PerformFormat menu, when requested.
 if (mode == "perform-format") di.LockMutex();
 inWriter = true; invoke(); inWriter = false;
 if (mode == "perform-format") di.UnlockMutex();
 check(stages["image_acquisition"] && stages["mutation"] && stages["gcr_reload_free"]
       && stages["reattach_complete"], "actual adapter/reload sites reached");
 check(writes == 1 && image.rawBytes() == before, "GCR mutated then actually reloaded from image");
 check(directory_test_drive_attached(&image.image), "actual drive_image_attach completed");
 check(di.reversedLockOrder == 0, "writer I/O guard ends before DiskAttached interface lock");
 // No-media early return and an exception before host resources are allocated.
 units[0] = nullptr;
 inWriter = true;
 if (mode == "insert") check(adapter.InsertFile("test.prg", false) == RET_STATUS_FAILED, "missing image failure unchanged");
 else adapter.FormatDisk("test", "id");
 inWriter = false;
 units[0] = &image.image; throwAtAcquisition = true;
 bool threw = false;
 try { inWriter = true; invoke(); } catch (const std::runtime_error &) { threw = true; }
 inWriter = false; throwAtAcquisition = false;
 check(threw, "injected exception reached");
 check(std::async(std::launch::async, [&] {
  bool acquired = di.ioMutex->TryLock(); if (acquired) di.ioMutex->Unlock(); return acquired;
 }).get(), "early return and exception release writer I/O guard");
 check(unsafeStages == 0, "actual writer sites exclude directory reader");
 units[0] = nullptr; directory_test_drive_setup(nullptr); activeInterface = nullptr;
}

int main(int argc, char **argv) {
 try {
  check(argc == 3, "mode and fixture output directory");
  std::string mode = argv[1];
  if (mode == "routing") routing(argv[2]); else writer(argv[2], mode);
  std::cout << "PASS " << mode << ": " << checks << " checks\n";
  return 0;
 } catch (const std::exception &e) { std::cerr << "FAIL: " << e.what() << "\n"; return 1; }
}

// Portable regression: real disk adapter, DOS parser, VICE image readers, and GCR codec.
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
#include "TestSdk.h"
#include "CDataAdapterViceDrive1541DiskContents.h"
#include "CDiskImageD64.h"
#include <fstream>
#include <string>
#include <stdexcept>
#include <cstdarg>

CGuiMain gui;
CGuiMain *guiMain = &gui;
static disk_image_t *mounted;
static gcr_t *mountedGcr;
static bool attached;
static int writes;
static int checks;
static void check(bool ok, const char *message) {
 ++checks;
 if (!ok) throw std::runtime_error(message);
}
// Host services only; no emulator is started.  Mutation APIs fail deliberately.
extern "C" {
disk_image_t *c64d_get_drive_disk_image(int) { return mounted; }
gcr_t *c64d_get_drive_disk_gcr(int) { return mountedGcr; }
unsigned int c64d_get_drive_is_disk_attached(int) { return attached; }
void *lib_malloc(size_t n) { return malloc(n); }
void *lib_calloc(size_t n, size_t s) { return calloc(n, s); }
void lib_free(const void *p) { free(const_cast<void *>(p)); }
int log_error(log_t, const char *, ...) { return 0; }
int log_message(log_t, const char *, ...) { return 0; }
int util_fpread(FILE *f, void *p, size_t n, off_t offset) {
 return fseek(f, offset, SEEK_SET) || fread(p, 1, n, f) != n ? -1 : 0;
}
int util_fpwrite(FILE *, const void *, size_t, off_t) { ++writes; return -1; }
uint16_t util_le_buf_to_word(uint8_t *p) { return p[0] | (p[1] << 8); }
uint32_t util_le_buf_to_dword(uint8_t *p) { return p[0] | (p[1]<<8) | (p[2]<<16) | (uint32_t(p[3])<<24); }
void util_word_to_le_buf(uint8_t *p, uint16_t n) { p[0]=n; p[1]=n>>8; }
void util_dword_to_le_buf(uint8_t *p, uint32_t n) { for (int i=0;i<4;++i) p[i]=n>>(8*i); }
int fsimage_p64_read_sector(const disk_image_t *, uint8_t *, const disk_addr_t *) { return -1; }
int fsimage_p64_write_sector(disk_image_t *, const uint8_t *, const disk_addr_t *) { ++writes; return -1; }
int fsimage_p64_write_half_track(disk_image_t *, unsigned int, const disk_track_t *) { ++writes; return -1; }
int drive_get_disk_drive_type(unsigned int) { return 1541; }
int cbmimage_create_image(const char *, unsigned int) { ++writes; return -1; }
int vdrive_device_setup(vdrive_t *, unsigned int) { return -1; }
int vdrive_attach_image(disk_image_t *, unsigned int, unsigned int, vdrive_t *) { return -1; }
void vdrive_detach_image(disk_image_t *, unsigned int, unsigned int, vdrive_t *) {}
int vdrive_command_execute(vdrive_t *, const uint8_t *, unsigned int) { ++writes; return -1; }
int drive_image_attach(disk_image_t *, unsigned int) { return -1; }
uint8_t *charset_petconvstring(uint8_t *p, int) { return p; }
fileio_info_t *fileio_open(const char *, const char *, unsigned int, unsigned int, unsigned int, int *) { return nullptr; }
void fileio_close(fileio_info_t *) {}
unsigned int fileio_read(fileio_info_t *, uint8_t *, unsigned int) { return 0; }
int vdrive_iec_open(vdrive_t *, const uint8_t *, unsigned int, unsigned int, cbmdos_cmd_parse_s *) { return -1; }
int vdrive_iec_write(vdrive_t *, uint8_t, unsigned int) { ++writes; return -1; }
int vdrive_iec_close(vdrive_t *, unsigned int) { return -1; }
}

static size_t offset(int t, int s) {
 size_t n=s;
 for (int i=1;i<t;++i) n+=disk_image_sector_per_track(DISK_IMAGE_TYPE_D64,i);
 return n*256;
}
static void putName(uint8_t *dest, const char *name, int n) {
 memset(dest,0xa0,n); memcpy(dest,name,strlen(name));
}
static std::vector<uint8_t> authoredDisk(int tracks) {
 std::vector<uint8_t> disk(offset(tracks+1,0));
 for (int t=1;t<=tracks;++t) for (unsigned s=0;s<disk_image_sector_per_track(DISK_IMAGE_TYPE_D64,t);++s)
  for (int i=0;i<256;++i) disk[offset(t,s)+i]=(t*13+s*7+i)&255;
 auto sector=[&](int t,int s) { return disk.data()+offset(t,s); };
 uint8_t *bam=sector(18,0); memset(bam,0,256); bam[0]=18; bam[1]=1; bam[2]=0x41;
 for (int t=1;t<=35;++t) {
  int count=disk_image_sector_per_track(DISK_IMAGE_TYPE_D64,t);
  bam[4*t]=count;
  for (int s=0;s<count;++s) bam[4*t+1+s/8]|=1<<(s%8);
 }
 for (auto ts: {std::pair<int,int>{18,0},{18,1},{18,2},{1,0},{1,1},{25,0}}) {
  --bam[4*ts.first]; bam[4*ts.first+1+ts.second/8]&=~(1<<(ts.second%8));
 }
 putName(bam+0x90,"SECTOR VIEWS",16); putName(bam+0xa0," 52 2A",7);
 for (int s=1;s<=2;++s) memset(sector(18,s),0,256);
 sector(18,1)[0]=18; sector(18,1)[1]=2; sector(18,2)[1]=255;
 for (int n=0;n<2;++n) {
  uint8_t *dir=sector(18,n+1); dir[2]=0x82; dir[3]=n ? 25 : 1; dir[4]=0;
  putName(dir+5,n ? "SECOND" : "MULTISECTOR",16); dir[30]=n ? 1 : 2;
 }
 sector(1,0)[0]=1; sector(1,0)[1]=1;
 sector(1,1)[0]=0; sector(1,1)[1]=11;
 sector(25,0)[0]=0; sector(25,0)[1]=5;
 return disk;
}
static void writeFile(const std::string &name,const std::vector<uint8_t> &data) {
 std::ofstream f(name,std::ios::binary); f.write((const char *)data.data(),data.size());
}
static void makeG64(const std::string &name,const std::vector<uint8_t> &disk,int tracks,int slots,
                    int badTrack=0,int badSector=0,fdc_err_t error=CBMDOS_FDC_ERR_OK) {
 std::vector<uint8_t> out(12+slots*8,0);
 memcpy(out.data(),"GCR-1541",8); out[9]=slots; util_word_to_le_buf(out.data()+10,7928);
 for (int t=1;t<=tracks;++t) {
  unsigned count=disk_image_sector_per_track(DISK_IMAGE_TYPE_D64,t);
  unsigned len=disk_image_raw_track_size(DISK_IMAGE_TYPE_D64,t);
  size_t pos=out.size(); util_dword_to_le_buf(out.data()+12+(t-1)*8,pos);
  util_dword_to_le_buf(out.data()+12+slots*4+(t-1)*8,disk_image_speed_map(DISK_IMAGE_TYPE_D64,t));
  out.resize(pos+2+len,0x55); util_word_to_le_buf(out.data()+pos,len);
  int gap=disk_image_header_gap_size(DISK_IMAGE_TYPE_D64,t), sync=5;
  int stride=SECTOR_GCR_SIZE_WITH_HEADER+5+gap+sync+disk_image_gap_size(DISK_IMAGE_TYPE_D64,t);
  for (unsigned s=0;s<count;++s) {
   gcr_header_t header{uint8_t(s),uint8_t(t),'2','5'};
   gcr_convert_sector_to_GCR(disk.data()+offset(t,s),out.data()+pos+2+s*stride,&header,gap,sync,
                            t==badTrack && int(s)==badSector ? error : CBMDOS_FDC_ERR_OK);
  }
 }
 writeFile(name,out);
}
struct Image {
 disk_image_t image{};
 fsimage_t fs{};
 gcr_t gcr{};
 Image(const std::string &name,unsigned type,int tracks,int slots=0) {
  image.type=type; image.tracks=tracks; image.max_half_tracks=slots; image.device=DISK_IMAGE_DEVICE_FS;
  image.read_only=1; image.media.fsimage=&fs;
  fs.fd=fopen(name.c_str(),"rb"); check(fs.fd!=nullptr,"fixture open");
  if (type==DISK_IMAGE_TYPE_G64) { image.gcr=&gcr; fsimage_read_gcr_image(&image); }
 }
 ~Image() { for(auto &t:gcr.tracks) free(t.data); fclose(fs.fd); }
 void mount() { mounted=&image; mountedGcr=image.gcr; attached=true; }
 std::vector<u8> rawBytes() const {
  std::vector<u8> bytes;
  for (const auto &track:gcr.tracks) if (track.data && track.size>0)
   bytes.insert(bytes.end(),track.data,track.data+track.size);
  return bytes;
 }
};

static void ordinary(const std::string &dir) {
 auto disk=authoredDisk(35); writeFile(dir+"/ordinary.d64",disk); makeG64(dir+"/ordinary.g64",disk,35,84);
 Image d64(dir+"/ordinary.d64",DISK_IMAGE_TYPE_D64,35);
 Image g64(dir+"/ordinary.g64",DISK_IMAGE_TYPE_G64,42,84);
 CDebugInterfaceVice di; CDebugSymbols symbols{&di}; CDataAdapterViceDrive1541DiskContents adapter(&symbols,0);
 di.dataAdapterDrive1541DiskContents=&adapter;
 std::vector<std::vector<u8>> files;
 for (Image *image: {&d64,&g64}) {
  auto rawBefore=image->rawBytes();
  image->mount(); adapter.DiskAttached();
  check(adapter.dataIndexes!=nullptr,"ordinary 84-slot G64 must not be rejected by the sector-count limit");
  for (int t=1;t<=35;++t) for(unsigned s=0;s<disk_image_sector_per_track(DISK_IMAGE_TYPE_D64,t);++s) {
   u8 buf[256]; memset(buf,0xcc,256); adapter.ReadSector(t-1,s,buf);
   check(memcmp(buf,disk.data()+offset(t,s),256)==0,"D64/G64 logical sector parity");
   bool available=false; u8 value=0;
   adapter.AdapterReadByte(adapter.GetDataAddressForTrackAndSector(t-1,s)+91,&value,&available);
   check(available && value==buf[91],"hex view byte availability and contents");
  }
  CDiskImageD64 parser(&di,0);
  check(parser.ReadImage(),"ordinary directory parse");
  check(memcmp(parser.diskName,disk.data()+offset(18,0)+0x90,16)==0,"disk name");
  check(memcmp(parser.diskId,disk.data()+offset(18,0)+0xa0,7)==0,"disk ID");
  check(parser.numFreeSectors==661,"BAM free count");
  check(parser.fileEntries.size()==2,"two linked directory sectors and entries");
  for (int n=0;n<2;++n) {
   auto *entry=parser.fileEntries[n];
   check(entry->fileType==2 && entry->closed && !entry->locked && entry->fileSize==(n ? 1:2),"entry type and size");
   check(memcmp(entry->fileName,disk.data()+offset(18,n+1)+5,16)==0,"entry name");
   CByteBuffer bytes; check(parser.ReadEntry(n,&bytes),"file chain read");
   std::vector<u8> expected;
   if (!n) { expected.insert(expected.end(),disk.begin()+offset(1,0)+2,disk.begin()+offset(1,0)+256);
             expected.insert(expected.end(),disk.begin()+offset(1,1)+2,disk.begin()+offset(1,1)+12); }
   else expected.insert(expected.end(),disk.begin()+offset(25,0)+2,disk.begin()+offset(25,0)+6);
   check(bytes.bytes==expected,"authored multi-sector file bytes");
   if (image==&d64) files.push_back(bytes.bytes); else check(bytes.bytes==files[n],"D64/G64 file parity");
  }
  check(image->rawBytes()==rawBefore,"viewing preserves all loaded GCR track bytes");
 }
 check(writes==0,"viewing must never call disk mutation APIs");
}
static void errors(const std::string &dir) {
 auto disk=authoredDisk(35);
 makeG64(dir+"/bad-bam.g64",disk,35,70,18,0,CBMDOS_FDC_ERR_DCHECK);
 Image image(dir+"/bad-bam.g64",DISK_IMAGE_TYPE_G64,35,70); image.mount();
 CDebugInterfaceVice di; CDebugSymbols symbols{&di}; CDataAdapterViceDrive1541DiskContents adapter(&symbols,0);
 di.dataAdapterDrive1541DiskContents=&adapter; adapter.DiskAttached();
 bool available=true; u8 value=0xcc;
 adapter.AdapterReadByte(adapter.GetDataAddressForTrackAndSector(17,0)+144,&value,&available);
 check(!available,"bad-checksum BAM must be unavailable, not fabricated success");
#ifndef DISK_CONTENTS_BASELINE
 u8 buf[256]; memset(buf,0xcc,256);
 check(!adapter.ReadSector(17,0,buf),"BAM sector read failure");
 check(std::vector<u8>(buf,buf+256)==std::vector<u8>(256,0),"failed read clears destination");
 CDiskImageD64 parser(&di,0); check(!parser.ReadImage(),"parser rejects undecodable BAM");
 check(parser.IsDiskAttached() && !parser.isDirectoryValid && parser.fileEntries.empty(),"attached media with unavailable directory");
 for(auto ts: {std::pair<int,int>{-1,0},{35,0},{0,-1},{0,21}}) {
  check(!adapter.ReadSector(ts.first,ts.second,buf),"bounds rejected before indexing");
  check(!adapter.DriveContextReadSector(ts.first,ts.second,buf),"direct read bounds");
  check(!adapter.WriteSector(ts.first,ts.second,buf),"write bounds");
  check(!adapter.DriveContextWriteSector(ts.first,ts.second,buf),"direct write bounds");
 }
 // First cache valid bytes, then fail the same sector while retaining the adapter.
 makeG64(dir+"/good.g64",disk,35,70);
 Image good(dir+"/good.g64",DISK_IMAGE_TYPE_G64,35,70); good.mount(); adapter.DiskAttached();
 check(adapter.ReadSector(17,0,buf) && buf[144]=='S',"cache valid BAM before failure");
 image.mount(); adapter.MarkTrackDirty(17);
 check(!adapter.ReadSector(17,0,buf) && buf[144]==0,"failed refresh removes old successful bytes");
 adapter.AdapterReadByte(adapter.GetDataAddressForTrackAndSector(17,0)+144,&value,&available);
 check(!available && value==0,"failed cached byte is unavailable and deterministic");

 // Missing headers and checksums in structurally required and file sectors.
 for (auto ts: {std::pair<int,int>{18,0},{18,1},{18,2},{1,1}}) {
  makeG64(dir+"/missing.g64",disk,35,70,ts.first,ts.second,CBMDOS_FDC_ERR_HEADER);
  Image missing(dir+"/missing.g64",DISK_IMAGE_TYPE_G64,35,70); missing.mount(); adapter.DiskAttached();
  check(!adapter.ReadSector(ts.first-1,ts.second,buf),"missing sector header");
  CDiskImageD64 damaged(&di,0);
  if (ts.first==18) {
   check(!damaged.ReadImage() && !damaged.isDirectoryValid && damaged.fileEntries.empty(),"no partial directory after failed chain read");
   check(damaged.numFreeSectors==0 && damaged.diskName[0]==' ',"no stale metadata after directory failure");
  } else {
   check(damaged.ReadImage(),"missing data does not invalidate readable directory");
   CByteBuffer bytes; bytes.PutU8(99);
   check(!damaged.ReadEntry(0,&bytes) && bytes.length==0,"failed data read returns no partial file");
  }
 }
 // Cyclic, empty-cyclic, and out-of-range directory links must terminate.
 for (int variant=0;variant<4;++variant) {
  auto changed=disk; auto *d=changed.data()+offset(18,2);
  if (variant<2) { d[0]=18; d[1]=variant ? 2:1; }
  if (variant==1) { memset(changed.data()+offset(18,1)+2,0,254); memset(d+2,0,254); }
  if (variant==2) { d[0]=255; d[1]=0; }
  if (variant==3) { d[0]=18; d[1]=21; }
  makeG64(dir+"/directory-chain.g64",changed,35,70);
  Image chain(dir+"/directory-chain.g64",DISK_IMAGE_TYPE_G64,35,70); chain.mount(); adapter.DiskAttached();
  CDiskImageD64 damaged(&di,0);
  check(!damaged.ReadImage() && damaged.fileEntries.empty(),"malformed directory chain fails without hanging");
 }
 for (int variant=0;variant<4;++variant) {
  auto changed=disk; auto *d=changed.data()+offset(1,1);
  if (variant==0) { d[0]=1; d[1]=0; }
  if (variant==1) { d[0]=255; d[1]=0; }
  if (variant==2) { d[0]=1; d[1]=21; }
  if (variant==3) { d[0]=0; d[1]=0; }
  makeG64(dir+"/file-chain.g64",changed,35,70);
  Image chain(dir+"/file-chain.g64",DISK_IMAGE_TYPE_G64,35,70); chain.mount(); adapter.DiskAttached();
  CDiskImageD64 damaged(&di,0); CByteBuffer bytes;
  check(damaged.ReadImage(),"malformed file preserves readable directory");
  check(!damaged.ReadEntry(0,&bytes) && bytes.length==0,"malformed file returns no partial bytes");
  check(!damaged.ReadEntry(-1,&bytes) && !damaged.ReadEntry(2,&bytes),"invalid entry indexes");
 }
 // D64 reader errors must also be honored, not just GCR statuses.
 writeFile(dir+"/truncated.d64",std::vector<uint8_t>(256,0));
 Image truncated(dir+"/truncated.d64",DISK_IMAGE_TYPE_D64,35); truncated.mount(); adapter.DiskAttached();
 check(!adapter.ReadSector(17,0,buf),"truncated D64 read fails");
 good.mount(); adapter.DiskAttached();
 adapter.AdapterWriteByte(adapter.GetDataAddressForTrackAndSector(17,0)+144,'X',&available);
 check(!available,"read-only edit reports failure");
 check(adapter.ReadSector(17,0,buf) && buf[144]=='S',"failed edit cannot publish modified cache as success");

 // Full tracks up to 42 are logical; an odd half-track never adds a DOS track.
 auto extended=authoredDisk(42); makeG64(dir+"/extended.g64",extended,42,84);
 Image extra(dir+"/extended.g64",DISK_IMAGE_TYPE_G64,42,84); extra.mount(); adapter.DiskAttached();
 check(adapter.numOfTracks==42 && adapter.numOfSectors==802,"42-track G64 geometry");
 for(int t=36;t<=42;++t) for(int s=0;s<17;++s)
  check(adapter.ReadSector(t-1,s,buf) && memcmp(buf,extended.data()+offset(t,s),256)==0,"extended logical sector bytes");
 check(!adapter.ReadSector(42,0,buf),"upper full-track bound");
 extra.image.tracks=43; adapter.DiskAttached();
 check(adapter.numOfTracks==0 && !adapter.dataIndexes,"unsupported geometry rejected before allocation");
 extra.image.tracks=42;
 gcr_t raw{}; raw.tracks[80]=extra.gcr.tracks[80]; raw.tracks[83]=extra.gcr.tracks[82];
 mounted=nullptr; mountedGcr=&raw; adapter.DiskAttached();
 check(adapter.numOfTracks==41,"raw fallback derives last populated full track, excluding half-track");
 check(adapter.ReadSector(40,0,buf) && memcmp(buf,extended.data()+offset(41,0),256)==0,"raw full-track indexing");
 check(!adapter.ReadSector(0,0,buf),"raw missing track is unavailable");
 raw.tracks[80]={nullptr,0}; adapter.DiskAttached();
 check(adapter.numOfTracks==0 && adapter.AdapterGetDataLength()==0,"half-track-only raw image has no logical tracks");
 extra.mount(); attached=false; adapter.DiskDetached();
 check(adapter.AdapterGetDataLength()==0 && adapter.GetDataAddressForTrackAndSector(0,0)==-1,"detach clears geometry");
 check(adapter.GetTrackAndSectorForDataAddress(0)==nullptr,"detached address unavailable");
 adapter.AdapterReadByte(0,&value,&available);
 check(!available && value==0 && !adapter.ReadSector(0,0,buf),"detached cached contents unavailable");
#endif
 check(writes==0,"failed reads must not write");
}
#ifndef DISK_CONTENTS_BASELINE
static void writeStatus(const std::string &dir) {
 auto disk=authoredDisk(35); makeG64(dir+"/write-status.g64",disk,35,70);
 Image image(dir+"/write-status.g64",DISK_IMAGE_TYPE_G64,35,70);
 CDebugInterfaceVice di; CDebugSymbols symbols{&di}; CDataAdapterViceDrive1541DiskContents adapter(&symbols,0);
 di.dataAdapterDrive1541DiskContents=&adapter;
 mounted=nullptr; mountedGcr=&image.gcr; attached=true; adapter.DiskAttached();
 u8 input[256], result[256]; memset(input,0x42,256);
 check(adapter.WriteSector(0,0,input),"raw write success uses FDC OK, not DOS OK");
 check(input[0]==0x42 && adapter.ReadSector(0,0,result) && memcmp(result,input,256)==0,"sector write uses supplied bytes, then refreshes cache");
 gcr_t missing{}; mountedGcr=&missing; adapter.MarkTrackDirty(0);
 bool available=true; adapter.AdapterWriteByte(0,0x99,&available);
 check(!available,"unavailable sector cannot be edited from fabricated bytes");
 check(!adapter.DriveContextWriteSector(0,0,input),"raw write error surfaced");
 image.mount(); image.image.read_only=0; adapter.DiskAttached();
 int attempts=writes;
 check(!adapter.WriteSector(0,0,input),"disk image write failure surfaced");
 check(writes>attempts,"failure came from attempted backing-image write");
}
#endif
int main(int argc,char **argv) {
 try {
  check(argc==3,"usage: sector_views_test ordinary|errors output-directory");
  if (std::string(argv[1])=="ordinary") ordinary(argv[2]);
#ifndef DISK_CONTENTS_BASELINE
  else if (std::string(argv[1])=="writes") writeStatus(argv[2]);
#endif
  else errors(argv[2]);
  printf("PASS %s: %d checks, %d writes\n",argv[1],checks,writes); return 0;
 } catch(const std::exception &e) { fprintf(stderr,"FAIL: %s (%d checks)\n",e.what(),checks); return 1; }
}

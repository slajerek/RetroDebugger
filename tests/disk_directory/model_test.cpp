// Reuse upstream fixture authors/readers unchanged, not their test expectations.
#define main upstream_disk_contents_main
#include "../disk_contents/CDataAdapterViceDrive1541DiskContents_test.cpp"
#undef main
#include "CDataAdapterViceDiskDirectory.h"

int main(int argc, char **argv) {
 try {
  check(argc == 2, "fixture output directory");
  const std::string dir = argv[1];
  auto disk = authoredDisk(35);
  CDebugInterfaceVice di; CDebugSymbols symbols{&di};
  // Deliberately no shared adapter: the parser must use its explicit binding.
  for (int variant = 0; variant < 8; ++variant) {
   auto bytes = disk;
   auto *bam = bytes.data()+offset(18,0);
   auto *first = bytes.data()+offset(18,1);
   auto *last = bytes.data()+offset(18,2);
   if (variant == 1) { memset(first,0,256); first[1]=255; }
   if (variant == 2) { last[0]=18; last[1]=1; }
   if (variant == 3) { last[0]=99; }
   if (variant == 4) { bam[2]=0x42; }
   if (variant == 5) { bam[4]=99; }
   makeG64(dir+"/model.g64",bytes,35,84,18,variant==6 ? 0:2,
           variant>=6 ? CBMDOS_FDC_ERR_HEADER:CBMDOS_FDC_ERR_OK);
   Image raw(dir+"/model.g64",DISK_IMAGE_TYPE_G64,42,84);
   writeFile(dir+"/model.d64",disk); // Different persisted bytes for mutated cases.
   Image flat(dir+"/model.d64",DISK_IMAGE_TYPE_D64,35);
   flat.image.gcr=&raw.gcr; // Model mounted D64's true-drive GCR view.
   for (Image *image : {&flat,&raw}) {
    image->mount();
    const auto before = raw.rawBytes();
    // Upstream Image opens a stream but deliberately leaves fs.name null.
    // Supply name provenance here only; mutate it after construction to reject borrowing.
    char mountedName[]="mounted-name";
    image->fs.name=mountedName;
    CDataAdapterViceDiskDirectory adapter(&symbols,3,&image->image);
    mountedName[0]='X'; image->fs.name=nullptr;
    check(std::string(adapter.status)=="ok" && adapter.driveId==3,"selected unit preserved");
    check(adapter.imageType==(image==&flat ? "D64":"G64") && adapter.imagePath=="mounted-name",
          "owned mounted image metadata");
    CDiskImageD64 model(&adapter);
    check(adapter.dataIndexes==nullptr && di.dataAdapterDrive1541DiskContents==nullptr,
          "no shared adapter or full-disk cache");
    check(raw.rawBytes()==before && writes==0,"mounted bytes unchanged; no writes");
    if (variant < 2) {
     check(model.isDirectoryValid,"valid standard directory");
     check(model.fileEntries.size()==(variant ? 0u:2u),"empty vs populated");
     check(std::string(model.directoryStatus)==(variant ? "empty":"ok"),"success status");
     check(model.numFreeSectors==661,"BAM count excludes track 18");
     check(memcmp(model.diskName,bam+0x90,16)==0,"raw disk name");
     if (!variant) {
      check(model.fileEntries[0]->fileSize==2 && model.fileEntries[1]->closed,"entries");
      check(model.fileEntries[0]->fileType==2,"PRG");
     }
    } else {
     const char *expected[]={"","","cyclic","malformed","unsupported","malformed","unreadable","unreadable"};
     check(!model.isDirectoryValid && model.fileEntries.empty(),"incomplete never succeeds");
     check(std::string(model.directoryStatus)==expected[variant],"specific failure cause");
     check(model.directoryPartial==(variant==2 || variant==3 || variant==7),"partial progress");
     check(model.numFreeSectors==0,"no fabricated free count on failure");
    }
    attached=false;
    check(model.ReadImage() && !model.isDirectoryValid
          && std::string(model.directoryStatus)=="unavailable","detached not empty");
   }
  }
  disk_image_t rejected{};
  rejected.type=DISK_IMAGE_TYPE_D71; rejected.tracks=35;
  { CDataAdapterViceDiskDirectory adapter(&symbols,0,&rejected);
    check(std::string(adapter.status)=="unsupported" && !adapter.tracks,"unsupported type"); }
  rejected.type=DISK_IMAGE_TYPE_D64; rejected.tracks=99;
  { CDataAdapterViceDiskDirectory adapter(&symbols,0,&rejected);
    check(std::string(adapter.status)=="unsupported" && !adapter.tracks,"bounded geometry"); }
  rejected.tracks=35;
  { CDataAdapterViceDiskDirectory adapter(&symbols,0,&rejected);
    check(std::string(adapter.status)=="unavailable" && !adapter.tracks,"no host fallback without GCR"); }
  printf("PASS directory model: %d checks, %d writes\n",checks,writes);
  return 0;
 } catch (const std::exception &e) { fprintf(stderr,"FAIL: %s\n",e.what()); return 1; }
}

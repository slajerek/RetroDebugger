// Real VICE C structures stay in C: this tree's anonymous TP64Image typedef
// and diskimage.h forward tag cannot be included together from C++.
#include "drive.h"
#include "drivetypes.h"
#include "diskimage.h"
#include <string.h>
CLOCK drive_clk[NUM_DISK_UNITS];
drive_context_t *drive_context[NUM_DISK_UNITS];
static drive_t drive;
static drive_context_t context;
void directory_test_reattach_complete(void);
void directory_test_drive_setup(disk_image_t *image) {
 memset(&drive, 0, sizeof(drive));
 memset(&context, 0, sizeof(context));
 drive.type = DRIVE_TYPE_1541;
 if (image) drive.gcr = image->gcr;
 context.drive = &drive;
 drive_context[0] = image ? &context : NULL;
}
int directory_test_drive_attached(disk_image_t *image) {
 return drive.image == image && drive.GCR_image_loaded;
}
int drive_get_disk_drive_type(int unit) { (void)unit; return DRIVE_TYPE_1541; }
void drive_set_half_track(int track, int side, drive_t *d) {
 (void)track; (void)side; (void)d;
 directory_test_reattach_complete();
}

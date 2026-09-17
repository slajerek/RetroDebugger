#ifndef GRELOC_H
#define GRELOC_H

#ifdef __cplusplus
extern "C" {
#endif

#define FORMAT_SID 0
#define FORMAT_PRG 1
#define FORMAT_BIN 2

#define PLAYER_BUFFERED 8
#define PLAYER_SOUNDEFFECTS 16
#define PLAYER_VOLUME 32
#define PLAYER_AUTHORINFO 64
#define PLAYER_ZPGHOSTREGS 128
#define PLAYER_NOOPTIMIZATION 256
#define PLAYER_FULLBUFFERED 512

#define MAX_OPTIONS 7

#define TYPE_NONE 0
#define TYPE_OVERFLOW 1
#define TYPE_JUMP 2

#define CAUSE_NONE 0
#define CAUSE_PATTERN 1
#define CAUSE_INSTRUMENT 2
#define CAUSE_WAVECMD 3

#define MAX_BYTES_PER_ROW 16

#ifndef GRELOC_C
extern unsigned char pattused[MAX_PATT];
extern unsigned char instrused[MAX_INSTR];
extern unsigned char tableused[MAX_TABLES][MAX_TABLELEN+1];
extern unsigned char pattmap[MAX_PATT];
extern unsigned char instrmap[MAX_INSTR];
extern unsigned char tablemap[MAX_TABLES][MAX_TABLELEN+1];
extern int tableerror;
/* Where the failing table walk went wrong, so the error can name it rather
   than only the instrument: the 1-based row holding the last jump taken, the
   target it jumped to, and the row the walk entered at. 0 = no jump was
   taken (the table simply has no $FF and runs off the end). */
extern int tableerrorrow;
extern int tableerrortarget;
extern int tableerrorentry;
#endif

void relocator(void);
int relocator_export(const char *outputPath, char *errorMsg, int errorMsgSize);

// Phase 6: pack & assemble the current song and append the resulting
// FORMAT_PRG bytes to `out` instead of a file. Caller owns `out`.
struct membuf;
int relocator_export_to_membuf(struct membuf *out, char *errorMsg, int errorMsgSize);

// Phase 7 diagnostics for parity tests. After a successful
// relocator_export* call, these mirror the packer's per-song
// decisions. Test code consults them to verify that NOARPCHANNELS
// was flipped to 0 when arp data was present and that the pool was
// populated. Reading them before any export returns the post-init
// values (noarppool=1, arppoolcount=0).
#ifndef GRELOC_C
extern int noarppool;
extern int arppoolcount;
extern unsigned char gt2_diag_arpbyterow0[MAX_PATT];
extern unsigned char gt2_diag_scan_log[64];
extern int gt2_diag_scan_count;
extern unsigned char gt2_diag_emit_log[64];
extern int gt2_diag_emit_count;
#endif

// Look up a captured post-export arp-related label address. Returns
// -1 if the label wasn't resolved (e.g. mt_arppool_patt_lo is absent in
// a song that didn't emit any arp data). Call after any
// relocator_export* on the same thread.
int gt2_get_arp_label_addr(const char *name);
int testoverlap(int area1start, int area1size, int area2start, int area2size);
int packpattern(unsigned char *dest, unsigned char *src, int rows, int pattnum);
unsigned char swapnybbles(unsigned char n);
void findtableduplicates(int num);
int isusedandselfcontained(int num, int start);
void calcspeedtest(unsigned char pos);

int insertfile(char *name);
void inserttext(const char *text);
void insertdefine(const char *name, int value);
void insertlabel(const char *name);
void insertbyte(unsigned char value);
void insertbytes(const unsigned char *values, int size);
void insertaddrlo(const char *name);
void insertaddrhi(const char *name);

#ifdef __cplusplus
}
#endif

#endif

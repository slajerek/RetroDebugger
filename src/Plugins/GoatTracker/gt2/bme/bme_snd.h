// Sound functions header file

int snd_init(unsigned mixrate, unsigned mixmode, unsigned bufferlength, unsigned channels, int usedirectsound);
void snd_uninit(void);
void snd_setcustommixer(void (*custommixer)(Sint32 *dest, unsigned samples));
void snd_preventdistortion(unsigned channels);
void snd_setmastervolume(unsigned chnum, unsigned char mastervol);
void snd_setmusicmastervolume(unsigned musicchannels, unsigned char mastervol);
void snd_setsfxmastervolume(unsigned musicchannels, unsigned char mastervol);

extern void (*snd_player)(void);
extern CHANNEL *snd_channel;
extern int snd_sndinitted;
extern int snd_bpmtempo;
extern int snd_bpmcount;
extern int snd_channels;
extern int snd_buffers;
extern unsigned snd_mixmode;
extern unsigned snd_mixrate;

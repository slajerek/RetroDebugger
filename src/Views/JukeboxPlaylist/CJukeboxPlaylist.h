#ifndef _CJukeboxPlaylist_H_
#define _CJukeboxPlaylist_H_

#include "SYS_Defs.h"
#include "CSlrString.h"
#include <vector>

class CJukeboxPlaylistEntry;
class CJukeboxPlaylistAction;

enum jukeBoxActionTypes
{
	JUKEBOX_ACTION_NONE = 0,
	JUKEBOX_ACTION_JOYSTICK1_DOWN,
	JUKEBOX_ACTION_JOYSTICK1_UP,
	JUKEBOX_ACTION_JOYSTICK2_DOWN,
	JUKEBOX_ACTION_JOYSTICK2_UP,
	JUKEBOX_ACTION_KEY_DOWN,
	JUKEBOX_ACTION_KEY_UP,
	JUKEBOX_ACTION_SET_WARP,
	JUKEBOX_ACTION_DUMP_C64_MEMORY,
	JUKEBOX_ACTION_DUMP_DISK_MEMORY,
	JUKEBOX_ACTION_DETACH_CARTRIDGE,
	JUKEBOX_ACTION_SAVE_SCREENSHOT,
	JUKEBOX_ACTION_EXPORT_SCREEN,
	JUKEBOX_ACTION_SHUTDOWN
};

class CJukeboxPlaylist
{
public:
	CJukeboxPlaylist(char *json);
	~CJukeboxPlaylist();
	
	std::vector<CJukeboxPlaylistEntry *> entries;
	
	void InitFromJSON(char *json);
	void DebugPrint();
	
	bool fastBootPatch;
	float delayAfterResetMs;
	bool showLoadAddressInfo;
	bool fadeSoundVolume;
	
	bool showTextInfo;
	float showTextFadeTime;
	float showTextVisibleTime;
	
	int setLayoutViewNumber;
};

class CJukeboxPlaylistAction
{
public:
	CJukeboxPlaylistAction();
	~CJukeboxPlaylistAction();
	
	float doAfterDelay;
	int actionType;
	int code;
	CSlrString *text;

	void DebugPrint();
};

class CJukeboxPlaylistEntry
{
public:
	CJukeboxPlaylistEntry();
	~CJukeboxPlaylistEntry();
	
	CSlrString *name;
	CSlrString *filePath;
	
	bool autoRun;
	int runFileNum;
	int resetMode;
	
	float waitTime;
	float fadeInTime;
	float fadeOutTime;
	
	float fadeColorR;
	float fadeColorG;
	float fadeColorB;
	
	float delayAfterResetTime;
	
	std::vector<CJukeboxPlaylistAction *> actions;
	
	void DebugPrint();
};


#endif

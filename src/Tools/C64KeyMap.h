#ifndef _C64KEYMAP_H_
#define _C64KEYMAP_H_

#include "SYS_Defs.h"
#include <map>

class CByteBuffer;
class CSlrString;

// virtual rows
#define C64_KEYCODE_RESTORE_ROW			(-3)
#define C64_KEYCODE_RESTORE_COLUMN		(0)
#define C64_KEYCODE_SHIFT_LOCK_ROW		(-4)
#define C64_KEYCODE_SHIFT_LOCK_COLUMN	(1)


class C64KeyCode
{
public:
	u32 keyCode;
	
	int matrixRow;
	int matrixCol;
	int shift;
};

class C64KeyMap
{
public:
	C64KeyMap();
	~C64KeyMap();
	
	std::map<u32, C64KeyCode *> keyCodes;
	
	void AddKeyCode(C64KeyCode *keyCode);
	void AddKeyCode(u32 keyCode, int matrixRow, int matrixCol, int shift);
	
	C64KeyCode *FindKeyCode(u32 keyCode);
	
	void SaveKeyMapToBuffer(CByteBuffer *byteBuffer);
	bool LoadKeyMapFromBuffer(CByteBuffer *byteBuffer);
	
	void ClearKeyMap();
	
	void InitDefault();
};

C64KeyMap *C64KeyMapGetDefault();

void C64KeyMapCreateDefault();
void C64KeyMapLoadFromSettings();
void C64KeyMapStoreToSettings();

bool C64KeyMapLoadFromFile(CSlrString *filePath);
void C64KeyMapStoreToFile(CSlrString *filePath);

uint16 C64KeyMapConvertSpecialKeyCode(uint16 keyCode, bool isShift, bool isAlt, bool isControl);


#endif

//_C64KEYMAP_H_

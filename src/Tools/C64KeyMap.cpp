extern "C"{
#include "keyboard.h"
}

#include "C64KeyMap.h"
#include "SYS_KeyCodes.h"
#include "CByteBuffer.h"
#include "CSlrString.h"
#include "C64SettingsStorage.h"

#include "CDebugInterfaceVice.h"

#define C64D_KEYMAP_MAGIC			0xEA
#define C64D_KEYMAP_FILE_VERSION	0x01

C64KeyMap::C64KeyMap()
{
	
}

void C64KeyMap::AddKeyCode(C64KeyCode *key)
{
	this->keyCodes[key->keyCode] = key;
}

void C64KeyMap::AddKeyCode(u32 mtKeyCode, int matrixRow, int matrixCol, int shift)
{
	C64KeyCode *key = FindKeyCode(mtKeyCode);
	
	if (key == NULL)
	{
		key = new C64KeyCode();
		this->keyCodes[mtKeyCode] = key;
	}

	key->keyCode = mtKeyCode;
	key->matrixRow = matrixRow;
	key->matrixCol = matrixCol;
	key->shift = shift;
}

C64KeyCode *C64KeyMap::FindKeyCode(u32 keyCode)
{
	std::map<u32, C64KeyCode *>::iterator it = this->keyCodes.find(keyCode);
	
	if (it != this->keyCodes.end())
	{
		C64KeyCode *key = it->second;
		return key;
	}
	
	return NULL;
}

void C64KeyMap::SaveKeyMapToBuffer(CByteBuffer *byteBuffer)
{
	byteBuffer->PutU8(C64D_KEYMAP_MAGIC);
	byteBuffer->PutU8(C64D_KEYMAP_FILE_VERSION);
	
	byteBuffer->PutU32(this->keyCodes.size());
	
	for (std::map<u32, C64KeyCode *>::iterator it = this->keyCodes.begin();
		 it != this->keyCodes.end(); it++)
	{
		C64KeyCode *key = it->second;
		
		byteBuffer->PutU32(key->keyCode);
		byteBuffer->PutI16(key->matrixRow);
		byteBuffer->PutI16(key->matrixCol);
		byteBuffer->PutU8(key->shift);
	}
}

bool C64KeyMap::LoadKeyMapFromBuffer(CByteBuffer *byteBuffer)
{
	u8 magic = byteBuffer->GetU8();
	if (magic != C64D_KEYMAP_MAGIC)
		return false;
	
	u8 version = byteBuffer->GetU8();
	if (version > C64D_KEYMAP_FILE_VERSION)
		return false;
	
	u32 numKeys = byteBuffer->GetU32();
	
	for (int i = 0; i < numKeys; i++)
	{
		C64KeyCode *key = new C64KeyCode();
		key->keyCode = byteBuffer->GetU32();
		key->matrixRow = byteBuffer->GetI16();
		key->matrixCol = byteBuffer->GetI16();
		key->shift = byteBuffer->GetU8();
		
		this->AddKeyCode(key);
	}
	
	return true;
}

void C64KeyMap::ClearKeyMap()
{
	while (!keyCodes.empty())
	{
		std::map<u32, C64KeyCode *>::iterator it = this->keyCodes.begin();
		C64KeyCode *keyCode = it->second;
		this->keyCodes.erase(it);
		delete keyCode;
	}
}


C64KeyMap::~C64KeyMap()
{
	ClearKeyMap();
}

C64KeyMap *defaultKeyMap = NULL;

C64KeyMap *C64KeyMapGetDefault()
{
	return defaultKeyMap;
}


void C64KeyMapCreateDefault()
{
	if (defaultKeyMap != NULL)
	{
		delete defaultKeyMap;
		defaultKeyMap = NULL;
	}
	
	defaultKeyMap = new C64KeyMap();
	defaultKeyMap->InitDefault();
}

void C64KeyMapLoadFromSettings()
{
	LOGD("C64KeyMapLoadFromSettings");
	
	if (defaultKeyMap != NULL)
	{
		delete defaultKeyMap;
	}
	
	defaultKeyMap = new C64KeyMap();
	
	CByteBuffer *byteBuffer = new CByteBuffer();
	
	CSlrString *fileName = new CSlrString(C64D_KEYMAP_FILE_PATH);
	byteBuffer->loadFromSettings(fileName);
	delete fileName;
	
	if (byteBuffer->length == 0)
	{
		LOGD("... no default keymap found");
		delete byteBuffer;
		C64KeyMapCreateDefault();
		return;
	}
	
	if (defaultKeyMap->LoadKeyMapFromBuffer(byteBuffer) == false)
	{
		LOGError("... error loading keymap");
		delete byteBuffer;
		C64KeyMapCreateDefault();
		return;
	}
	delete byteBuffer;
	
	LOGD("C64KeyMapLoadFromSettings: done");
}

bool C64KeyMapStoreToSettings()
{
	LOGD("C64KeyMapStoreToSettings");
	CByteBuffer *byteBuffer = new CByteBuffer();
	
	defaultKeyMap->SaveKeyMapToBuffer(byteBuffer);
	
	CSlrString *fileName = new CSlrString(C64D_KEYMAP_FILE_PATH);
	bool ret = byteBuffer->storeToSettings(fileName);
	delete fileName;
	
	delete byteBuffer;
	
	LOGD("C64KeyMapStoreToSettings: done");
	return ret;
}

bool C64KeyMapLoadFromFile(CSlrString *filePath)
{
	if (defaultKeyMap != NULL)
	{
		delete defaultKeyMap;
	}
	
	defaultKeyMap = new C64KeyMap();
	
	CByteBuffer *byteBuffer = new CByteBuffer();
	
	byteBuffer->readFromFile(filePath);
	
	if (byteBuffer->length == 0)
	{
		LOGD("... no keymap found");
		delete byteBuffer;
		return false;
	}
	
	if (defaultKeyMap->LoadKeyMapFromBuffer(byteBuffer) == false)
	{
		LOGError("... error loading keymap");
		delete byteBuffer;
		return false;
	}
	delete byteBuffer;
	
	return true;
}

void C64KeyMapStoreToFile(CSlrString *filePath)
{
	CByteBuffer *byteBuffer = new CByteBuffer();
	
	defaultKeyMap->SaveKeyMapToBuffer(byteBuffer);
	
	byteBuffer->storeToFile(filePath);
	
	delete byteBuffer;
}





void C64KeyMap::InitDefault()
{
	/*
	 C64 keyboard matrix:
	 
	 Bit   7   6   5   4   3   2   1   0
	 0    CUD  F5  F3  F1  F7 CLR RET DEL
	 1    SHL  E   S   Z   4   A   W   3
	 2     X   T   F   C   6   D   R   5
	 3     V   U   H   B   8   G   Y   7
	 4     N   O   K   M   0   J   I   9
	 5     ,   @   :   .   -   L   P   +
	 6     /   ^   =  SHR HOM  ;   *   £
	 7    R/S  Q   C= SPC  2  CTL  <-  1
	 */
	
	// MATRIX (row, column)
	
	// http://classiccmp.org/dunfield/c64/h/front.jpg
	
	//	AddKeyCode('a', int row, int col, int shift);
	
	AddKeyCode(MTKEY_F5, 0, 6, NO_SHIFT);
	AddKeyCode(MTKEY_F6, 0, 6, LEFT_SHIFT);
	AddKeyCode(MTKEY_F3, 0, 5, NO_SHIFT);
	AddKeyCode(MTKEY_F4, 0, 5, LEFT_SHIFT);
	AddKeyCode(MTKEY_F1, 0, 4, NO_SHIFT);
	AddKeyCode(MTKEY_F2, 0, 4, LEFT_SHIFT);
	AddKeyCode(MTKEY_F7, 0, 3, NO_SHIFT);
	AddKeyCode(MTKEY_F8, 0, 3, LEFT_SHIFT);
	
	AddKeyCode(MTKEY_ENTER, 0, 1, NO_SHIFT);
	AddKeyCode(MTKEY_BACKSPACE, 0, 0, NO_SHIFT);
	AddKeyCode(MTKEY_LSHIFT, 1, 7, NO_SHIFT);
	AddKeyCode('e', 1, 6, NO_SHIFT);
	AddKeyCode('E', 1, 6, LEFT_SHIFT);
	AddKeyCode('s', 1, 5, NO_SHIFT);
	AddKeyCode('S', 1, 5, LEFT_SHIFT);
	AddKeyCode('z', 1, 4, NO_SHIFT);
	AddKeyCode('Z', 1, 4, LEFT_SHIFT);
	AddKeyCode('4', 1, 3, NO_SHIFT);
	AddKeyCode('$', 1, 3, LEFT_SHIFT);
	AddKeyCode('a', 1, 2, NO_SHIFT);
	AddKeyCode('A', 1, 2, LEFT_SHIFT);
	AddKeyCode('w', 1, 1, NO_SHIFT);
	AddKeyCode('W', 1, 1, LEFT_SHIFT);
	AddKeyCode('3', 1, 0, NO_SHIFT);
	AddKeyCode('#', 1, 0, LEFT_SHIFT);
	AddKeyCode('x', 2, 7, NO_SHIFT);
	AddKeyCode('X', 2, 7, LEFT_SHIFT);
	AddKeyCode('t', 2, 6, NO_SHIFT);
	AddKeyCode('T', 2, 6, LEFT_SHIFT);
	AddKeyCode('f', 2, 5, NO_SHIFT);
	AddKeyCode('F', 2, 5, LEFT_SHIFT);
	AddKeyCode('c', 2, 4, NO_SHIFT);
	AddKeyCode('C', 2, 4, LEFT_SHIFT);
	AddKeyCode('6', 2, 3, NO_SHIFT);
	AddKeyCode('^', 2, 3, LEFT_SHIFT);
	AddKeyCode('d', 2, 2, NO_SHIFT);
	AddKeyCode('D', 2, 2, LEFT_SHIFT);
	AddKeyCode('r', 2, 1, NO_SHIFT);
	AddKeyCode('R', 2, 1, LEFT_SHIFT);
	AddKeyCode('5', 2, 0, NO_SHIFT);
	AddKeyCode('%', 2, 0, LEFT_SHIFT);
	AddKeyCode('v', 3, 7, NO_SHIFT);
	AddKeyCode('V', 3, 7, LEFT_SHIFT);
	AddKeyCode('u', 3, 6, NO_SHIFT);
	AddKeyCode('U', 3, 6, LEFT_SHIFT);
	AddKeyCode('h', 3, 5, NO_SHIFT);
	AddKeyCode('H', 3, 5, LEFT_SHIFT);
	AddKeyCode('b', 3, 4, NO_SHIFT);
	AddKeyCode('B', 3, 4, LEFT_SHIFT);
	AddKeyCode('8', 3, 3, NO_SHIFT);
	AddKeyCode('*', 3, 3, LEFT_SHIFT);
	AddKeyCode('g', 3, 2, NO_SHIFT);
	AddKeyCode('G', 3, 2, LEFT_SHIFT);
	AddKeyCode('y', 3, 1, NO_SHIFT);
	AddKeyCode('Y', 3, 1, LEFT_SHIFT);
	AddKeyCode('7', 3, 0, NO_SHIFT);
	AddKeyCode('&', 3, 0, LEFT_SHIFT);
	AddKeyCode('n', 4, 7, NO_SHIFT);
	AddKeyCode('N', 4, 7, LEFT_SHIFT);
	AddKeyCode('o', 4, 6, NO_SHIFT);
	AddKeyCode('O', 4, 6, LEFT_SHIFT);
	AddKeyCode('k', 4, 5, NO_SHIFT);
	AddKeyCode('K', 4, 5, LEFT_SHIFT);
	AddKeyCode('m', 4, 4, NO_SHIFT);
	AddKeyCode('M', 4, 4, LEFT_SHIFT);
	AddKeyCode('0', 4, 3, NO_SHIFT);
	AddKeyCode(')', 4, 3, LEFT_SHIFT);
	AddKeyCode('j', 4, 2, NO_SHIFT);
	AddKeyCode('J', 4, 2, LEFT_SHIFT);
	AddKeyCode('i', 4, 1, NO_SHIFT);
	AddKeyCode('I', 4, 1, LEFT_SHIFT);
	AddKeyCode('9', 4, 0, NO_SHIFT);
	AddKeyCode('(', 4, 0, LEFT_SHIFT);
	AddKeyCode(',', 5, 7, NO_SHIFT);
	AddKeyCode('<', 5, 7, LEFT_SHIFT);
	AddKeyCode('[', 5, 6, NO_SHIFT);
	AddKeyCode('{', 5, 6, LEFT_SHIFT);
	AddKeyCode(';', 5, 5, NO_SHIFT);
	AddKeyCode(':', 5, 5, LEFT_SHIFT);
	AddKeyCode('.', 5, 4, NO_SHIFT);
	AddKeyCode('>', 5, 4, LEFT_SHIFT);
	AddKeyCode('-', 5, 3, NO_SHIFT);
	AddKeyCode('_', 5, 3, LEFT_SHIFT);
	AddKeyCode('l', 5, 2, NO_SHIFT);
	AddKeyCode('L', 5, 2, LEFT_SHIFT);
	AddKeyCode('p', 5, 1, NO_SHIFT);
	AddKeyCode('P', 5, 1, LEFT_SHIFT);
	AddKeyCode('=', 5, 0, NO_SHIFT);
	AddKeyCode('+', 5, 0, LEFT_SHIFT);
	AddKeyCode('/', 6, 7, NO_SHIFT);
	AddKeyCode('?', 6, 7, LEFT_SHIFT);
	//	AddKeyCode('^', 6, 6, NO_SHIFT);
	//	AddKeyCode('@', 6, 5, DESHIFT_SHIFT);
	AddKeyCode(MTKEY_RSHIFT, 6, 4, NO_SHIFT);
	//	AddKeyCode('', 6, 3, NO_SHIFT);
	
	AddKeyCode('\'', 6, 2, NO_SHIFT);
	AddKeyCode('\"', 6, 2, LEFT_SHIFT);
	AddKeyCode(']', 6, 1, NO_SHIFT);
	AddKeyCode('}', 6, 1, LEFT_SHIFT);
	
	//	AddKeyCode('', 6, 0, NO_SHIFT);
	
	AddKeyCode(MTKEY_ESC, 7, 7, NO_SHIFT);
	AddKeyCode(MTKEY_ESC | MTKEY_SPECIAL_SHIFT, 7, 7, LEFT_SHIFT);
	
	AddKeyCode('q', 7, 6, NO_SHIFT);
	AddKeyCode('Q', 7, 6, LEFT_SHIFT);
	
	//	AddKeyCode('', 7, 5, NO_SHIFT);
	AddKeyCode(' ', 7, 4, NO_SHIFT);
	
	AddKeyCode('2', 7, 3, NO_SHIFT);
	AddKeyCode('@', 7, 3, LEFT_SHIFT);
	
	AddKeyCode(MTKEY_LCONTROL, 7, 2, NO_SHIFT);
	AddKeyCode(MTKEY_LCONTROL | MTKEY_SPECIAL_SHIFT, 7, 2, LEFT_SHIFT);

	AddKeyCode(MTKEY_LALT, 7, 5, NO_SHIFT | ALLOW_SHIFT);
	AddKeyCode(MTKEY_LALT | MTKEY_SPECIAL_SHIFT, 7, 5, LEFT_SHIFT);

	AddKeyCode('`', 7, 1, NO_SHIFT);
	
	AddKeyCode('1', 7, 0, NO_SHIFT);
	AddKeyCode('!', 7, 0, LEFT_SHIFT);
	
	AddKeyCode(MTKEY_ARROW_UP, 0, 7, LEFT_SHIFT);
	AddKeyCode(MTKEY_ARROW_DOWN, 0, 7, NO_SHIFT);
	AddKeyCode(MTKEY_ARROW_LEFT, 0, 2, LEFT_SHIFT);
	AddKeyCode(MTKEY_ARROW_RIGHT, 0, 2, NO_SHIFT);
	
//	AddKeyCode('\\', C64_KEYCODE_RESTORE_ROW, C64_KEYCODE_RESTORE_COLUMN, NO_SHIFT);
	AddKeyCode('\\', 6, 5, NO_SHIFT);
	
	
	/*
	 C64 keyboard matrix:
	 
	 Bit   7   6   5   4   3   2   1   0
	 0    CUD  F5  F3  F1  F7 CLR RET DEL
	 1    SHL  E   S   Z   4   A   W   3
	 2     X   T   F   C   6   D   R   5
	 3     V   U   H   B   8   G   Y   7
	 4     N   O   K   M   0   J   I   9
	 5     ,   @   :   .   -   L   P   +
	 6     /   ^   =  SHR HOM  ;   *   £
	 7    R/S  Q   C= SPC  2  CTL  <-  1
	 */

}


#ifndef _C64CharsetHires_H_
#define _C64CharsetHires_H_

#include "C64Bitmap.h"
#include <map>

class C64CharHires;

class C64CharsetHires
{
public:
	C64CharsetHires();
	~C64CharsetHires();
	
	void CreateFromCharset(u8 *charsetData);
	
	C64CharHires *GetCharacter(int chr);
	
	std::map<int, C64CharHires *> characters;
	
	CViewC64VicDisplay *vicDisplay;
};

#endif

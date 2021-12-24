#include "DBG_Log.h"
#include "C64CharsetHires.h"
#include "C64CharHires.h"

C64CharsetHires::C64CharsetHires()
{
}
	
void C64CharsetHires::CreateFromCharset(u8 *charsetData)
{
//	LOGD("C64CharsetHires::CreateFromCharset");
	for (int charId = 0; charId < 256; charId++)
	{
		u8 *chd = charsetData + 8*charId;
		
		C64CharHires *chr = new C64CharHires();
		
		for (int y = 0; y < 8; y++)
		{
			if ((*chd & 0x01) == 0x01)
			{
				chr->SetPixel(7, y, 1);
			}
			if ((*chd & 0x02) == 0x02)
			{
				chr->SetPixel(6, y, 1);
			}
			if ((*chd & 0x04) == 0x04)
			{
				chr->SetPixel(5, y, 1);
			}
			if ((*chd & 0x08) == 0x08)
			{
				chr->SetPixel(4, y, 1);
			}
			if ((*chd & 0x10) == 0x10)
			{
				chr->SetPixel(3, y, 1);
			}
			if ((*chd & 0x20) == 0x20)
			{
				chr->SetPixel(2, y, 1);
			}
			if ((*chd & 0x40) == 0x40)
			{
				chr->SetPixel(1, y, 1);
			}
			if ((*chd & 0x80) == 0x80)
			{
				chr->SetPixel(0, y, 1);
			}
			
			chd++;
		}

//		LOGD("charId=%d", charId);
//		chr->DebugPrint();
		characters[charId] = chr;
	}
}

C64CharHires *C64CharsetHires::GetCharacter(int chr)
{
	std::map<int, C64CharHires *>::iterator it = this->characters.find(chr);
	if (it == this->characters.end())
		return NULL;
	
	return it->second;
}

C64CharsetHires::~C64CharsetHires()
{
	while (!this->characters.empty())
	{
		std::map<int, C64CharHires *>::iterator it = this->characters.begin();
		C64CharHires *chr = it->second;
		this->characters.erase(it);
		delete chr;
	}
}


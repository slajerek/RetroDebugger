//
//  CDebugDataAdapter.hpp
//  MTEngine-MacOS
//
//  Created by Marcin Skoczylas on 04/02/2021.
//

#ifndef CDebugDataAdapter_hpp
#define CDebugDataAdapter_hpp

#include "CDataAdapter.h"

class CDebugInterface;
class CViewMemoryMap;

class CDebugDataAdapter : public CDataAdapter
{
public:
	CDebugDataAdapter(char *adapterID, CDebugInterface *debugInterface);
	virtual ~CDebugDataAdapter();
	
	CDebugInterface *debugInterface;

	virtual void SetViewMemoryMap(CViewMemoryMap *viewMemoryMap);
	CViewMemoryMap *viewMemoryMap;

	virtual int AdapterGetDataLength();
	
	// renderers should add this offset to the presented address
	virtual int GetDataOffset();
	
	virtual void AdapterReadByte(int addr, uint8 *value);
	virtual void AdapterWriteByte(int addr, uint8 value);
	virtual void AdapterReadByte(int addr, uint8 *value, bool *isAvailable);
	virtual void AdapterWriteByte(int addr, uint8 value, bool *isAvailable);
	virtual void AdapterReadBlockDirect(uint8 *buffer, int addrStart, int addrEnd);

	virtual void MarkCellRead(int addr);
	virtual void MarkCellRead(int addr, int pc, int rasterX, int rasterY);
	virtual void MarkCellWrite(int addr, uint8 value);
	virtual void MarkCellWrite(int addr, uint8 value, int pc, int rasterX, int rasterY);
	virtual void MarkCellExecute(int addr, uint8 opcode);
};

#endif /* CDebugDataAdapter_hpp */


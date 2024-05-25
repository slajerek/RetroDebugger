//
//  CDebugDataAdapter.hpp
//  MTEngine-MacOS
//
//  Created by Marcin Skoczylas on 04/02/2021.
//

#ifndef CDebugDataAdapter_hpp
#define CDebugDataAdapter_hpp

#include "CDataAdapter.h"

class CDebugSymbols;
class CDebugInterface;
class CDebugMemory;
class CDataAddressEditBox;
class CDataAddressEditBoxCallback;

class CDebugDataAdapter : public CDataAdapter
{
public:
	CDebugDataAdapter(const char *adapterID, CDebugSymbols *debugSymbols);
	virtual ~CDebugDataAdapter();
	
	CDebugSymbols *debugSymbols;
	CDebugInterface *debugInterface;

	virtual int AdapterGetDataLength();
	
	// renderers should add this offset to the presented address
	virtual int GetDataOffset();

	virtual int GetAddressForCell(int cell);
	virtual void GetAddressStringForCell(int cell, char *str, int maxLen);
	virtual CDataAddressEditBox *CreateDataAddressEditBox(CDataAddressEditBoxCallback *callback);
	
	virtual void AdapterReadByte(int cell, uint8 *value);
	virtual void AdapterWriteByte(int cell, uint8 value);
	virtual void AdapterReadByte(int cell, uint8 *value, bool *isAvailable);
	virtual void AdapterWriteByte(int cell, uint8 value, bool *isAvailable);
	virtual void AdapterReadBlockDirect(uint8 *buffer, int cellStart, int cellEnd);
};

#endif /* CDebugDataAdapter_hpp */


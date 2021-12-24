#ifndef _CViewDriveStateCPU_H_
#define _CViewDriveStateCPU_H_

#include "CViewBaseStateCPU.h"

class CDebugInterfaceC64;

class CViewDriveStateCPU : public CViewBaseStateCPU
{
public:
	CViewDriveStateCPU(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterfaceC64 *debugInterface);
	
	virtual void RenderRegisters();
	virtual void SetRegisterValue(StateCPURegister reg, int value);
	virtual int GetRegisterValue(StateCPURegister reg);
};



#endif


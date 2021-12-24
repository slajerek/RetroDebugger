#ifndef _CViewAtariStateCPU_H_
#define _CViewAtariStateCPU_H_

#include "CViewBaseStateCPU.h"

class CDebugInterfaceAtari;

class CViewAtariStateCPU : public CViewBaseStateCPU
{
public:
	CViewAtariStateCPU(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterfaceAtari *debugInterface);
	
	virtual void RenderRegisters();
	virtual void SetRegisterValue(StateCPURegister reg, int value);
	virtual int GetRegisterValue(StateCPURegister reg);
};



#endif


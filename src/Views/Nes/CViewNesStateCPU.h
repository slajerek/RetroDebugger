#ifndef _CViewNesStateCPU_H_
#define _CViewNesStateCPU_H_

#include "CViewBaseStateCPU.h"

class CDebugInterfaceNes;

class CViewNesStateCPU : public CViewBaseStateCPU
{
public:
	CViewNesStateCPU(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterfaceNes *debugInterface);
	
	virtual void RenderRegisters();
	virtual void SetRegisterValue(StateCPURegister reg, int value);
	virtual int GetRegisterValue(StateCPURegister reg);
};



#endif


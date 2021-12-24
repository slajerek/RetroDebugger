#ifndef _CViewC64StateCPU_H_
#define _CViewC64StateCPU_H_

#include "CViewBaseStateCPU.h"

class CDebugInterfaceC64;

class CViewC64StateCPU : public CViewBaseStateCPU
{
public:
	CViewC64StateCPU(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterfaceC64 *debugInterface);

	virtual void Render();
	virtual void RenderImGui();

	virtual void RenderRegisters();
	virtual void SetRegisterValue(StateCPURegister reg, int value);
	virtual int GetRegisterValue(StateCPURegister reg);	
};

#endif


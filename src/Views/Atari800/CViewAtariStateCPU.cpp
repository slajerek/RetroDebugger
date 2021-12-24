#include "C64D_Version.h"
#include "CViewAtariStateCPU.h"
#include "SYS_Main.h"
#include "CSlrFont.h"
#include "CViewC64.h"
#include "CDebugInterfaceAtari.h"

#if defined(RUN_ATARI)

register_def atari_cpu_regs[7] = {
	{	STATE_CPU_REGISTER_PC,		0.0,  4 },
	{	STATE_CPU_REGISTER_A,		5.0,  2 },
	{	STATE_CPU_REGISTER_X,		8.0,  2 },
	{	STATE_CPU_REGISTER_Y,		11.0, 2 },
	{	STATE_CPU_REGISTER_SP,		14.0, 2 },
	{	STATE_CPU_REGISTER_FLAGS,	17.0, 8 },
	{	STATE_CPU_REGISTER_IRQ,		20.0, 2 }
};

CViewAtariStateCPU::CViewAtariStateCPU(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterfaceAtari *debugInterface)
: CViewBaseStateCPU(name, posX, posY, posZ, sizeX, sizeY, debugInterface)
{
	this->numRegisters = 6;
	
	regs = (register_def*)&atari_cpu_regs;
}

void CViewAtariStateCPU::RenderRegisters()
{
	float px = this->posX;
	float py = this->posY;
	
	/// Atari CPU
	u16 pc;
	u8 a, x, y, flags, sp, irq;
	((CDebugInterfaceAtari*)debugInterface)->GetCpuRegs(&pc, &a, &x, &y, &flags, &sp, &irq);
	
	
	char buf[128];
	strcpy(buf, "PC   AR XR YR SP NV-BDIZC  IRQ");

	font->BlitText(buf, px, py, -1, fontSize);
	py += fontSize;
	
	////////////////////////////
	// TODO: SHOW Atari CPU CYCLE
	//		Byte2Bits(diskCpuState.processorFlags, flags);
	//		sprintf(buf, "%4.4x %2.2x %2.2x %2.2x %2.2x %s  %2.2x",
	//				diskCpuState.pc, diskCpuState.a, diskCpuState.x, diskCpuState.y, (diskCpuState.sp & 0x00ff),
	//				flags, diskState.headTrackPosition);
	
	char *bufPtr = buf;
	sprintfHexCode16WithoutZeroEnding(bufPtr, pc); bufPtr += 5;
	sprintfHexCode8WithoutZeroEnding(bufPtr, a); bufPtr += 3;
	sprintfHexCode8WithoutZeroEnding(bufPtr, x); bufPtr += 3;
	sprintfHexCode8WithoutZeroEnding(bufPtr, y); bufPtr += 3;
	sprintfHexCode8WithoutZeroEnding(bufPtr, sp); bufPtr += 3;
	Byte2BitsWithoutEndingZero(flags, bufPtr); bufPtr += 10;
	*bufPtr = ' '; bufPtr += 1;
	sprintfHexCode8WithoutZeroEnding(bufPtr, irq); bufPtr += 3;
	
	font->BlitText(buf, px, py, -1, fontSize);
}

extern "C" {
	void atrd_async_set_cpu_pc(int newPC);
	void atrd_async_set_reg_a(int newRegValue);
	void atrd_async_set_reg_x(int newRegValue);
	void atrd_async_set_reg_y(int newRegValue);
	void atrd_async_set_reg_p(int newRegValue);
	void atrd_async_set_reg_s(int newRegValue);
}

void CViewAtariStateCPU::SetRegisterValue(StateCPURegister reg, int value)
{
	debugInterface->LockMutex();
	
	
	switch (reg)
	{
		case STATE_CPU_REGISTER_PC:
			atrd_async_set_cpu_pc(value);
			break;
		case STATE_CPU_REGISTER_A:
			atrd_async_set_reg_a(value);
			break;
		case STATE_CPU_REGISTER_X:
			atrd_async_set_reg_x(value);
			break;
		case STATE_CPU_REGISTER_Y:
			atrd_async_set_reg_y(value);
			break;
		case STATE_CPU_REGISTER_SP:
			atrd_async_set_reg_s(value);
			break;
		case STATE_CPU_REGISTER_FLAGS:
			atrd_async_set_reg_p(value);
			break;
		case STATE_CPU_REGISTER_NONE:
		default:
			return;
	}
	debugInterface->UnlockMutex();
}

int CViewAtariStateCPU::GetRegisterValue(StateCPURegister reg)
{
	LOGD("CViewAtariStateCPU::GetRegisterValue: reg=%d", reg);
	
	u16 pc;
	u8 a, x, y, flags, sp, irq;
	((CDebugInterfaceAtari*)debugInterface)->GetCpuRegs(&pc, &a, &x, &y, &flags, &sp, &irq);

	switch (reg)
	{
		case STATE_CPU_REGISTER_PC:
			return pc;
		case STATE_CPU_REGISTER_A:
			return a;
		case STATE_CPU_REGISTER_X:
			return x;
		case STATE_CPU_REGISTER_Y:
			return y;
		case STATE_CPU_REGISTER_SP:
			return sp;
		case STATE_CPU_REGISTER_FLAGS:
			return flags;
		case STATE_CPU_REGISTER_IRQ:
			return irq;
		case STATE_CPU_REGISTER_NONE:
		default:
			return -1;
	}

	return 0x00FA;
}

#else

CViewAtariStateCPU::CViewAtariStateCPU(float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterfaceAtari *debugInterface)
: CViewBaseStateCPU(posX, posY, posZ, sizeX, sizeY, debugInterface) {}
void CViewAtariStateCPU::RenderRegisters() {}
void CViewAtariStateCPU::SetRegisterValue(StateCPURegister reg, int value) {}
int CViewAtariStateCPU::GetRegisterValue(StateCPURegister reg) { return -1; }

#endif

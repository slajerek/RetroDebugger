#include "EmulatorsConfig.h"
#include "CViewNesStateCPU.h"
#include "SYS_Main.h"
#include "CSlrFont.h"
#include "CViewC64.h"
#include "CDebugInterfaceNes.h"

#if defined(RUN_NES)

register_def nes_cpu_regs[7] = {
	{	STATE_CPU_REGISTER_PC,		0.0,  4 },
	{	STATE_CPU_REGISTER_A,		5.0,  2 },
	{	STATE_CPU_REGISTER_X,		8.0,  2 },
	{	STATE_CPU_REGISTER_Y,		11.0, 2 },
	{	STATE_CPU_REGISTER_SP,		14.0, 2 },
	{	STATE_CPU_REGISTER_FLAGS,	17.0, 8 },
	{	STATE_CPU_REGISTER_IRQ,		20.0, 2 }
};

CViewNesStateCPU::CViewNesStateCPU(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterfaceNes *debugInterface)
: CViewBaseStateCPU(name, posX, posY, posZ, sizeX, sizeY, debugInterface)
{
	this->numRegisters = 6;
	
	regs = (register_def*)&nes_cpu_regs;
}

void CViewNesStateCPU::RenderRegisters()
{
	float px = this->posX;
	float py = this->posY;
	
	/// Atari CPU
	u16 pc;
	u8 a, x, y, flags, sp, irq;
	((CDebugInterfaceNes*)debugInterface)->GetCpuRegs(&pc, &a, &x, &y, &flags, &sp, &irq);
	
	u32 hClock, vClock, cycles;
	((CDebugInterfaceNes*)debugInterface)->GetPpuClocks(&hClock, &vClock, &cycles);
	
	char buf[128];
	strcpy(buf, "PC   AR XR YR SP NV-BDIZC  IRQ  HCLK VCLK");

	font->BlitText(buf, px, py, -1, fontSize);
	py += fontSize;
	
	////////////////////////////
	
	char *bufPtr = buf;
	sprintfHexCode16WithoutZeroEnding(bufPtr, pc); bufPtr += 5;
	sprintfHexCode8WithoutZeroEnding(bufPtr, a); bufPtr += 3;
	sprintfHexCode8WithoutZeroEnding(bufPtr, x); bufPtr += 3;
	sprintfHexCode8WithoutZeroEnding(bufPtr, y); bufPtr += 3;
	sprintfHexCode8WithoutZeroEnding(bufPtr, sp); bufPtr += 3;
	Byte2BitsWithoutEndingZero(flags, bufPtr); bufPtr += 10;
	*bufPtr = ' '; bufPtr += 1;
	sprintfHexCode8WithoutZeroEnding(bufPtr, irq); bufPtr += 3;
	*bufPtr = ' '; bufPtr += 1;
	sprintfHexCode16WithoutZeroEnding(bufPtr, hClock); bufPtr += 5;
	sprintfHexCode16WithoutZeroEnding(bufPtr, vClock); bufPtr += 5;

	
	font->BlitText(buf, px, py, -1, fontSize);
}

//extern "C" {
//	void atrd_atari_set_cpu_pc(u16 addr);
//	void atrd_atari_set_cpu_reg_a(u8 val);
//	void atrd_atari_set_cpu_reg_x(u8 val);
//	void atrd_atari_set_cpu_reg_y(u8 val);
//	void atrd_atari_set_cpu_reg_p(u8 val);
//	void atrd_atari_set_cpu_reg_s(u8 val);
//}
//
void CViewNesStateCPU::SetRegisterValue(StateCPURegister reg, int value)
{
	LOGTODO("CViewNesStateCPU::SetRegisterValue");
	
	return;
	
//	debugInterface->LockMutex();
//	
//	
//	switch (reg)
//	{
//		case STATE_CPU_REGISTER_PC:
//			c64d_atari_set_cpu_pc(value);
//			break;
//		case STATE_CPU_REGISTER_A:
//			c64d_atari_set_cpu_reg_a(value);
//			break;
//		case STATE_CPU_REGISTER_X:
//			c64d_atari_set_cpu_reg_x(value);
//			break;
//		case STATE_CPU_REGISTER_Y:
//			c64d_atari_set_cpu_reg_y(value);
//			break;
//		case STATE_CPU_REGISTER_SP:
//			c64d_atari_set_cpu_reg_s(value);
//			break;
//		case STATE_CPU_REGISTER_FLAGS:
//			c64d_atari_set_cpu_reg_p(value);
//			break;
//		case STATE_CPU_REGISTER_NONE:
//		default:
//			return;
//	}
//	debugInterface->UnlockMutex();
}

int CViewNesStateCPU::GetRegisterValue(StateCPURegister reg)
{
	LOGD("CViewNesStateCPU::GetRegisterValue: reg=%d", reg);
	
	u16 pc;
	u8 a, x, y, flags, sp, irq;
	((CDebugInterfaceNes*)debugInterface)->GetCpuRegs(&pc, &a, &x, &y, &flags, &sp, &irq);

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

CViewNesStateCPU::CViewNesStateCPU(float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterfaceNes *debugInterface)
: CViewBaseStateCPU(posX, posY, posZ, sizeX, sizeY, debugInterface) {}
void CViewNesStateCPU::RenderRegisters() {}
void CViewNesStateCPU::SetRegisterValue(StateCPURegister reg, int value) {}
int CViewNesStateCPU::GetRegisterValue(StateCPURegister reg) { return -1; }

#endif

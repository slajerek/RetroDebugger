#include "CViewC64StateCPU.h"
#include "SYS_Main.h"
#include "CSlrFont.h"
#include "CViewC64.h"
#include "CDebugInterfaceVice.h"
#include "CViewC64StateVIC.h"

#define NUM_C64_REGS 7

register_def c64_cpu_regs[NUM_C64_REGS] = {
	{	STATE_CPU_REGISTER_PC,		0.0,  4 },
	{	STATE_CPU_REGISTER_A,		5.0,  2 },
	{	STATE_CPU_REGISTER_X,		8.0,  2 },
	{	STATE_CPU_REGISTER_Y,		11.0, 2 },
	{	STATE_CPU_REGISTER_SP,		14.0, 2 },
	{	STATE_CPU_REGISTER_MEM01,	17.0, 2 },
	{	STATE_CPU_REGISTER_FLAGS,	20.0, 8 }
};

CViewC64StateCPU::CViewC64StateCPU(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterfaceC64 *debugInterface)
: CViewBaseStateCPU(name, posX, posY, posZ, sizeX, sizeY, debugInterface)
{
	imGuiNoWindowPadding = true;
	imGuiNoScrollbar = true;

	this->numRegisters = NUM_C64_REGS;
	regs = (register_def*)&c64_cpu_regs;
	
	this->font = viewC64->fontDisassembly;
}

void CViewC64StateCPU::RenderImGui()
{
	PreRenderImGui();
	Render();
	PostRenderImGui();
}


void CViewC64StateCPU::Render()
{
	float px = this->posX;
	float py = this->posY;
	
	float br = 0.0f;
	float bg = 0.0f;
	float bb = 0.0f;
	
	if (viewC64->viewC64StateVIC->GetIsLockedState())
	{
		br = 0.35f; bg = 0.0f; bb = 0.0f;
		BlitFilledRectangle(px-fontSize*0.3f, py-fontSize*0.3f, -1, fontSize*49.6f, fontSize*2.3f, br, bg, bb, 1.00f);
	}
	//////////////////

	CViewBaseStateCPU::Render();
}

void CViewC64StateCPU::RenderRegisters()
{
	float px = this->posX;
	float py = this->posY;

	char buf[128];
	strcpy(buf, "PC   AR XR YR SP 01 NV-BDIZC  CC VC RSTY RSTX  EG");

	font->BlitText(buf, px, py, -1, fontSize);
	py += fontSize;
	
	int rasterX = viewC64->c64RasterPosToShowX; //viewC64->viciiStateToShow.raster_cycle*8;
	int rasterY = viewC64->c64RasterPosToShowY; //viewC64->viciiStateToShow.raster_line;

	//		char flags[20] = {0};
	//		Byte2Bits(cpuState.processorFlags, flags);
	//		sprintf(buf, "%4.4x %2.2x %2.2x %2.2x %2.2x %2.2x %s  %2.2x %2.2x %4x %4x  %1x%1x",
	//				cpuState.pc, cpuState.a, cpuState.x, cpuState.y, (cpuState.sp & 0x00ff),
	//				cpuState.memory0001, flags, cpuState.instructionCycle, vicState.cycle, vicState.rasterY, vicState.rasterX,
	//				cartridgeState.exrom, cartridgeState.game);
	
	char *bufPtr = buf;
	
	sprintfHexCode16WithoutZeroEnding(bufPtr, viewC64->viciiStateToShow.pc); bufPtr += 5;
	sprintfHexCode8WithoutZeroEnding(bufPtr, viewC64->viciiStateToShow.a); bufPtr += 3;
	sprintfHexCode8WithoutZeroEnding(bufPtr, viewC64->viciiStateToShow.x); bufPtr += 3;
	sprintfHexCode8WithoutZeroEnding(bufPtr, viewC64->viciiStateToShow.y); bufPtr += 3;
	sprintfHexCode8WithoutZeroEnding(bufPtr, viewC64->viciiStateToShow.sp); bufPtr += 3;
	sprintfHexCode8WithoutZeroEnding(bufPtr, viewC64->viciiStateToShow.memory0001); bufPtr += 3;
	Byte2BitsWithoutEndingZero(viewC64->viciiStateToShow.processorFlags, bufPtr); bufPtr += 10;
	sprintfHexCode8WithoutZeroEnding(bufPtr, viewC64->viciiStateToShow.instructionCycle); bufPtr += 3;
	sprintfHexCode8WithoutZeroEnding(bufPtr, viewC64->viciiStateToShow.raster_cycle); bufPtr += 3;
	sprintfHexCode16WithoutZeroEndingAndNoLeadingZeros(bufPtr, rasterY); bufPtr += 5;
	sprintfHexCode16WithoutZeroEndingAndNoLeadingZeros(bufPtr, rasterX); bufPtr += 6;
	*bufPtr = viewC64->viciiStateToShow.exrom + '0'; bufPtr++;
	*bufPtr = viewC64->viciiStateToShow.game + '0';
	
	font->BlitText(buf, px, py, -1, fontSize);
}

extern "C" {
	void c64d_set_maincpu_regs_no_trap(uint8 a, uint8 x, uint8 y, uint8 p, uint8 sp);
}

void CViewC64StateCPU::SetRegisterValue(StateCPURegister reg, int value)
{
	LOGD("CViewC64StateCPU::SetRegisterValue: reg=%d value=%d", reg, value);
	
	debugInterface->LockMutex();
	
	uint8 a, x, y, p, sp;
	a = viewC64->currentViciiState.a;
	x = viewC64->currentViciiState.x;
	y = viewC64->currentViciiState.y;
	p = viewC64->currentViciiState.processorFlags;
	sp = viewC64->currentViciiState.sp;
	
	switch (reg)
	{
		case STATE_CPU_REGISTER_PC:
			viewC64->currentViciiState.pc = value;
			viewC64->currentViciiState.lastValidPC = value;
			return ((CDebugInterfaceC64*)debugInterface)->MakeJmpC64(value);
		case STATE_CPU_REGISTER_A:
			a = value;
			((CDebugInterfaceC64*)debugInterface)->SetRegisterAC64(value);
			break;
		case STATE_CPU_REGISTER_X:
			x = value;
			((CDebugInterfaceC64*)debugInterface)->SetRegisterXC64(value);
			break;
		case STATE_CPU_REGISTER_Y:
			y = value;
			((CDebugInterfaceC64*)debugInterface)->SetRegisterYC64(value);
			break;
		case STATE_CPU_REGISTER_SP:
			sp = value;
			((CDebugInterfaceC64*)debugInterface)->SetStackPointerC64(value);
			break;
		case STATE_CPU_REGISTER_FLAGS:
			p = value;
			((CDebugInterfaceC64*)debugInterface)->SetRegisterPC64(value);
			break;
		case STATE_CPU_REGISTER_MEM01:
			((CDebugInterfaceC64*)debugInterface)->SetByteC64(0x01, value);
			viewC64->currentViciiState.memory0001 = value;
			break;
		case STATE_CPU_REGISTER_NONE:
		default:
			return;
	}
	
	// this direct inject is to have the set reflected in UI immediately
	//if (debugInterface->GetDebugMode() != C64_DEBUG_RUNNING)
	{
		c64d_set_maincpu_regs_no_trap(a, x, y, p, sp);
	}
	
	debugInterface->UnlockMutex();
}

int CViewC64StateCPU::GetRegisterValue(StateCPURegister reg)
{
	LOGD("CViewC64StateCPU::GetRegisterValue: reg=%d", reg);
	
	switch (reg)
	{
		case STATE_CPU_REGISTER_PC:
			return viewC64->viciiStateToShow.pc;
		case STATE_CPU_REGISTER_A:
			return viewC64->viciiStateToShow.a;
		case STATE_CPU_REGISTER_X:
			return viewC64->viciiStateToShow.x;
		case STATE_CPU_REGISTER_Y:
			return viewC64->viciiStateToShow.y;
		case STATE_CPU_REGISTER_SP:
			return viewC64->viciiStateToShow.sp;
		case STATE_CPU_REGISTER_FLAGS:
			return viewC64->viciiStateToShow.processorFlags;
		case STATE_CPU_REGISTER_MEM01:
			return viewC64->viciiStateToShow.memory0001;
		case STATE_CPU_REGISTER_NONE:
		default:
			return -1;
	}
}


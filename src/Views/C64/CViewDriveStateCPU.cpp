#include "CViewDriveStateCPU.h"
#include "SYS_Main.h"
#include "CSlrFont.h"
#include "CViewC64.h"
#include "CDebugInterfaceVice.h"

register_def drive1541_cpu_regs[6] = {
	{	STATE_CPU_REGISTER_PC,		0.0,  4 },
	{	STATE_CPU_REGISTER_A,		5.0,  2 },
	{	STATE_CPU_REGISTER_X,		8.0,  2 },
	{	STATE_CPU_REGISTER_Y,		11.0, 2 },
	{	STATE_CPU_REGISTER_SP,		14.0, 2 },
	{	STATE_CPU_REGISTER_FLAGS,	17.0, 8 }
};

CViewDriveStateCPU::CViewDriveStateCPU(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterfaceC64 *debugInterface)
: CViewBaseStateCPU(name, posX, posY, posZ, sizeX, sizeY, debugInterface)
{
	this->numRegisters = 6;
	regs = (register_def*)&drive1541_cpu_regs;

	this->font = viewC64->fontDisassembly;
}

void CViewDriveStateCPU::RenderRegisters()
{
	float px = this->posX;
	float py = this->posY;
	
	/// 1541 CPU
	C64StateCPU diskCpuState;
	((CDebugInterfaceC64*)debugInterface)->GetDrive1541CpuState(&diskCpuState);
	
	/// 1541 ICE
	C64StateDrive1541 diskState;
	((CDebugInterfaceC64*)debugInterface)->GetDrive1541State(&diskState);
	
	char buf[128];
	strcpy(buf, "PC   AR XR YR SP NV-BDIZC  HD");

	font->BlitText(buf, px, py, -1, fontSize);
	py += fontSize;
	
	////////////////////////////
	// TODO: SHOW 1541 CPU CYCLE
	//		Byte2Bits(diskCpuState.processorFlags, flags);
	//		sprintf(buf, "%4.4x %2.2x %2.2x %2.2x %2.2x %s  %2.2x",
	//				diskCpuState.pc, diskCpuState.a, diskCpuState.x, diskCpuState.y, (diskCpuState.sp & 0x00ff),
	//				flags, diskState.headTrackPosition);
	
	char *bufPtr = buf;
	sprintfHexCode16WithoutZeroEnding(bufPtr, diskCpuState.pc); bufPtr += 5;
	sprintfHexCode8WithoutZeroEnding(bufPtr, diskCpuState.a); bufPtr += 3;
	sprintfHexCode8WithoutZeroEnding(bufPtr, diskCpuState.x); bufPtr += 3;
	sprintfHexCode8WithoutZeroEnding(bufPtr, diskCpuState.y); bufPtr += 3;
	sprintfHexCode8WithoutZeroEnding(bufPtr, diskCpuState.sp); bufPtr += 3;
	Byte2BitsWithoutEndingZero(diskCpuState.processorFlags, bufPtr); bufPtr += 10;
	sprintfHexCode8WithoutZeroEnding(bufPtr, diskState.headTrackPosition); bufPtr += 3;
	
	font->BlitText(buf, px, py, -1, fontSize);
}

extern "C" {
	void c64d_set_drivecpu_regs_no_trap(int driveNr, uint8 a, uint8 x, uint8 y, uint8 p, uint8 sp);
	void c64d_set_drivecpu_pc_no_trap(int driveNr, uint16 pc);
}

void CViewDriveStateCPU::SetRegisterValue(StateCPURegister reg, int value)
{
	LOGD("CViewDriveStateCPU::SetRegisterValue: reg=%d value=%d", reg, value);
	
	debugInterface->LockMutex();
	
	C64StateCPU diskCpuState;
	((CDebugInterfaceC64*)debugInterface)->GetDrive1541CpuState(&diskCpuState);

	uint8 a, x, y, p, sp;
	a = diskCpuState.a;
	x = diskCpuState.x;
	y = diskCpuState.y;
	p = diskCpuState.processorFlags;
	sp = diskCpuState.sp;
	
	switch (reg)
	{
		case STATE_CPU_REGISTER_PC:
			//if (debugInterface->GetDebugMode() != C64_DEBUG_RUNNING)
			{
				c64d_set_drivecpu_pc_no_trap(0, value);
			}
			return ((CDebugInterfaceC64*)debugInterface)->MakeJmp1541(value);
		case STATE_CPU_REGISTER_A:
			a = value;
			((CDebugInterfaceC64*)debugInterface)->SetRegisterA1541(value);
			break;
		case STATE_CPU_REGISTER_X:
			x = value;
			((CDebugInterfaceC64*)debugInterface)->SetRegisterX1541(value);
			break;
		case STATE_CPU_REGISTER_Y:
			y = value;
			((CDebugInterfaceC64*)debugInterface)->SetRegisterY1541(value);
			break;
		case STATE_CPU_REGISTER_SP:
			sp = value;
			((CDebugInterfaceC64*)debugInterface)->SetStackPointer1541(value);
			break;
		case STATE_CPU_REGISTER_FLAGS:
			p = value;
			((CDebugInterfaceC64*)debugInterface)->SetRegisterP1541(value);
			break;
		case STATE_CPU_REGISTER_NONE:
		default:
			return;
	}
	
	// this direct inject is to have the set reflected in UI immediately
	if (debugInterface->GetDebugMode() != DEBUGGER_MODE_RUNNING)
	{
		c64d_set_drivecpu_regs_no_trap(0, a, x, y, p, sp);
	}
	
	debugInterface->UnlockMutex();
}

int CViewDriveStateCPU::GetRegisterValue(StateCPURegister reg)
{
	LOGD("CViewDriveStateCPU::GetRegisterValue: reg=%d", reg);
	
	C64StateCPU diskCpuState;
	((CDebugInterfaceC64*)debugInterface)->GetDrive1541CpuState(&diskCpuState);

	switch (reg)
	{
		case STATE_CPU_REGISTER_PC:
			return diskCpuState.pc;
		case STATE_CPU_REGISTER_A:
			return diskCpuState.a;
		case STATE_CPU_REGISTER_X:
			return diskCpuState.x;
		case STATE_CPU_REGISTER_Y:
			return diskCpuState.y;
		case STATE_CPU_REGISTER_SP:
			return diskCpuState.sp;
		case STATE_CPU_REGISTER_FLAGS:
			return diskCpuState.processorFlags;
		case STATE_CPU_REGISTER_NONE:
		default:
			return -1;
	}

	return 0x00FA;
}


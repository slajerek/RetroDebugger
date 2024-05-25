#include "EmulatorsConfig.h"

#include "NstApiMachine.hpp"
#include "NstMachine.hpp"
#include "NstApiEmulator.hpp"
#include "NstCpu.hpp"
#include "NstPpu.hpp"

#include "CDataAdapterNesPpuNmt.h"
#include "CDebugInterfaceNes.h"

extern Nes::Api::Emulator nesEmulator;

CDataAdapterNesPpuNmt::CDataAdapterNesPpuNmt(CDebugSymbols *debugSymbols)
: CDebugDataAdapter("NesPpuNmt", debugSymbols)
{
	this->debugInterfaceNes = (CDebugInterfaceNes *)(debugSymbols->debugInterface);
}

int CDataAdapterNesPpuNmt::AdapterGetDataLength()
{
	return 0x1000;
}

int CDataAdapterNesPpuNmt::GetDataOffset()
{
	return 0x2000;
}

void CDataAdapterNesPpuNmt::AdapterReadByte(int pointer, uint8 *value)
{
	Nes::Core::Machine& machine = nesEmulator;
	*value = machine.ppu.nmt.Peek(pointer);
}

void CDataAdapterNesPpuNmt::AdapterWriteByte(int pointer, uint8 value)
{
	Nes::Core::Machine& machine = nesEmulator;
	machine.ppu.nmt.Poke(pointer, value);
}


void CDataAdapterNesPpuNmt::AdapterReadByte(int pointer, uint8 *value, bool *isAvailable)
{
	if (pointer < 0x1000)
	{
		*isAvailable = true;
		Nes::Core::Machine& machine = nesEmulator;
		*value = machine.ppu.nmt.Peek(pointer); // + 0x2000);
	}
	else
	{
		*isAvailable = false;
	}
}

void CDataAdapterNesPpuNmt::AdapterWriteByte(int pointer, uint8 value, bool *isAvailable)
{
	if (pointer < 0x1000)
	{
		*isAvailable = true;
		Nes::Core::Machine& machine = nesEmulator;
		machine.ppu.nmt.Poke(pointer, value);
	}
	else
	{
		*isAvailable = false;
	}
}

void CDataAdapterNesPpuNmt::AdapterReadBlockDirect(uint8 *buffer, int pointerStart, int pointerEnd)
{
	Nes::Core::Machine& machine = nesEmulator;
	int addr;
	u8 *bufPtr = buffer + pointerStart;
	for (addr = pointerStart; addr < pointerEnd; addr++)
	{
		*bufPtr++ = machine.ppu.nmt.Peek(addr);
	}
}


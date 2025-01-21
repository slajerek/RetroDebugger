#include "CDebugSymbols.h"
#include "CDebugDataAdapter.h"
#include "CDebugMemory.h"
#include "CDebugMemoryCell.h"
#include "CDebugInterface.h"
#include "C64Opcodes.h"

CDebugMemory::CDebugMemory(CDebugSymbols *debugSymbols)
{
	this->debugSymbols = debugSymbols;
	this->debugInterface = debugSymbols->debugInterface;
	this->dataAdapter = debugSymbols->dataAdapter;
	
	// alloc with safe margin for broken algorithms
	numCells = dataAdapter->AdapterGetDataLength() + DATA_CELLS_PADDING_LENGTH;
	memoryCells = new CDebugMemoryCell *[numCells];
	for (int i = 0; i < numCells; i++)
	{
		memoryCells[i] = new CDebugMemoryCell(i);
	}
}

CDebugMemoryCell *CDebugMemory::GetMemoryCell(int address)
{
	if (address >= 0 && address < dataAdapter->AdapterGetDataLength())
	{
		return memoryCells[address];
	}
	return NULL;
}

bool CDebugMemory::IsExecuteCodeAddress(int address)
{
	if (address >= 0 && address < dataAdapter->AdapterGetDataLength())
	{
		CDebugMemoryCell *cell = memoryCells[address];
		return cell->isExecuteCode;
	}
	
	return false;
}

void CDebugMemory::ClearDebugMarkers()
{
	int ramSize = dataAdapter->AdapterGetDataLength();
	for (int i = 0; i < ramSize; i++)
	{
		memoryCells[i]->ClearDebugMarkers();
	}
}

void CDebugMemory::ClearReadWriteDebugMarkers()
{
	int ramSize = dataAdapter->AdapterGetDataLength();
	for (int i = 0; i < ramSize; i++)
	{
		memoryCells[i]->ClearReadWriteDebugMarkers();
	}
}

//
void CDebugMemory::CellRead(int addr)
{
	CDebugMemoryCell *cell = GetMemoryCell(addr);
	
	if (cell)
	{		
		cell->MarkCellRead();
	}
}

void CDebugMemory::CellRead(int addr, int pc, int readRasterLine, int readRasterCycle)
{
	CDebugMemoryCell *cell = GetMemoryCell(addr);
	
	if (cell)
	{
		cell->MarkCellRead();

		cell->readPC = pc;
		cell->readRasterLine = readRasterLine;
		cell->readRasterCycle = readRasterCycle;

		cell->readCycle = debugInterface->GetCurrentCpuInstructionCycleCounter();
		cell->readFrame = debugInterface->GetEmulationFrameNumber();
	}
}

void CDebugMemory::CellWrite(int addr, uint8 value)
{
	CDebugMemoryCell *cell = GetMemoryCell(addr);

	if (cell)
	{
		cell->MarkCellWrite(value);
	}
}

void CDebugMemory::CellWrite(int addr, uint8 value, int pc, int writeRasterLine, int writeRasterCycle)
{
	CDebugMemoryCell *cell = GetMemoryCell(addr);
	
	if (cell)
	{
		u64 writeCycle = debugInterface->GetCurrentCpuInstructionCycleCounter();
		u32 writeFrame = debugInterface->GetEmulationFrameNumber();

		cell->MarkCellWrite(value, writeCycle, writeFrame, pc, writeRasterLine, writeRasterCycle);
	}
}

void CDebugMemory::CellExecute(int addr, uint8 opcode)
{
	//LOGD("CViewMemoryMap::CellExecute: addr=%4.4x", addr);
	CDebugMemoryCell *cell = GetMemoryCell(addr);

	if (cell)
	{
		u64 executeCycle = debugInterface->GetCurrentCpuInstructionCycleCounter();
		u32 executeFrame = debugInterface->GetEmulationFrameNumber();
		cell->MarkCellExecuteCode(opcode, executeCycle, executeFrame);
	}

	uint8 l = opcodes[opcode].addressingLength;
	if (l == 2)
	{
		addr++;
		cell = GetMemoryCell(addr);
		if (cell)
		{
			cell->MarkCellExecuteArgument();
			cell->executeCycle = debugInterface->GetCurrentCpuInstructionCycleCounter();
			cell->executeFrame = debugInterface->GetEmulationFrameNumber();
		}
	}
	else if (l == 3)
	{
		addr++;
		cell = GetMemoryCell(addr);
		
		if (cell)
		{
			cell->MarkCellExecuteArgument();
			cell->executeCycle = debugInterface->GetCurrentCpuInstructionCycleCounter();
			cell->executeFrame = debugInterface->GetEmulationFrameNumber();
			addr++;
			cell = GetMemoryCell(addr);
			cell->MarkCellExecuteArgument();
			cell->executeCycle = debugInterface->GetCurrentCpuInstructionCycleCounter();
			cell->executeFrame = debugInterface->GetEmulationFrameNumber();
		}
		else
		{
			addr++;
		}
	}
}

void CDebugMemory::ClearEventsAfterCycle(u64 cycle)
{
	for (int i = 0; i < dataAdapter->AdapterGetDataLength(); i++)
	{
		memoryCells[i]->ClearEventsAfterCycle(cycle);
	}
}

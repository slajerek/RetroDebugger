#include "CDebugSymbols.h"
#include "CDebugDataAdapter.h"
#include "CDebugMemory.h"
#include "CDebugMemoryCell.h"

CDebugMemory::CDebugMemory(CDebugSymbols *debugSymbols)
{
	this->debugSymbols = debugSymbols;
	this->degugInterface = debugSymbols->debugInterface;
	this->dataAdapter = debugSymbols->dataAdapter;
	
	// alloc with safe margin to avoid comparison in cells marking (it is quicker)
	int numCells = dataAdapter->AdapterGetDataLength() + DATA_CELLS_PADDING_LENGTH;
	memoryCells = new CDebugMemoryCell *[numCells];
	for (int i = 0; i < numCells; i++)
	{
		memoryCells[i] = new CDebugMemoryCell(i);
	}
}

bool CDebugMemory::IsExecuteCodeAddress(int address)
{
	int ramSize = dataAdapter->AdapterGetDataLength() + DATA_CELLS_PADDING_LENGTH;
	if (address >= 0 && address <= ramSize)
	{
		CDebugMemoryCell *cell = memoryCells[address];
		return cell->isExecuteCode;
	}
	
	return false;
}

void CDebugMemory::ClearDebugMarkers()
{
	int ramSize = dataAdapter->AdapterGetDataLength() + DATA_CELLS_PADDING_LENGTH;
	for (int i = 0; i < ramSize; i++)
	{
		memoryCells[i]->ClearDebugMarkers();
	}
}

void CDebugMemory::ClearReadWriteDebugMarkers()
{
	int ramSize = dataAdapter->AdapterGetDataLength() + DATA_CELLS_PADDING_LENGTH;
	for (int i = 0; i < ramSize; i++)
	{
		memoryCells[i]->ClearReadWriteDebugMarkers();
	}
}

#include "CDataAdapterDrive1541Minimal.h"
#include "CDebugInterfaceC64.h"

// this adapter is for memory map view to display RAM map and skip not available memory cells

CDataAdapterDrive1541Minimal::CDataAdapterDrive1541Minimal(CDebugSymbols *debugSymbols, CDebugDataAdapter *drive1541DataAdapter)
: CDebugDataAdapter("Drive1541Minimal", debugSymbols)
{
	this->drive1541DataAdapter = drive1541DataAdapter;
	
	// calculate mapping of pointer to real drive address

	// imageHeight=1024	imageWidth=64
	// last cell is vx=16 vy=33 addr=1c10
	// 64 per row * 33 = 2112   + 16 = 2128

	int pointer = 0;
	int address = 0;
	
	for (int i = 0; i < 0x10000; i++)
	{
		mapAddressToPointer[i] = -1;
	}
	
	int vx = 0;
	int vy = 0;

	int lastAddress = 0x1C40;
	for (int y = 0; y < 1024; y++)
	{
		vx = 0;
		//px = startX;
		
		for (int x = 0; x < 64; x++)
		{
			if (address == 0x0800)
			{
//				LOGD("pointer=%x address=%x		---------  0x0800", pointer, address);
				address = 0x1800;
			}
			else if (address == 0x1840)
			{
//				LOGD("pointer=%x address=%x		--------- ^0x1810", pointer, address);
				//			vx = 0;
				//			vy += 1;
				address = 0x1C00;
				vx = 0;
				vy += 1;
			}
			else if (address == lastAddress)
			{
//				LOGD("pointer=%x address=%x		--------- ^0x1C10", pointer, address);
				// end
				break;
			}
			
//			LOGD("vx=%5d vy=%5d pointer=%05x address=%05x", vx, vy, pointer, address);

			mapPointerToDriveAddress[pointer] = address;
			pointer++;
			address++;
			
			vx += 1;
		}

		if (address == lastAddress)
			break;
		
		vy += 1;

	}

//	LOGD("vx=%d vy=%d", vx, vy);
	
	int numRows = pointer / 0x40;	// num rows for texture in data map view
	LOGD("CDataAdapterDrive1541Minimal: mapping last address=%X numRows=%d", address, numRows);
	
	// continue till end of memory
	while (address < 0x10000)
	{
		mapPointerToDriveAddress[pointer] = address;
		pointer++;
		address++;
	}
	
	
//	LOGD("----- DONE mapping");
	
	/*
	// is from disk   //TODO: generalize this
	for (int y = 0; y < imageHeight; y++)
	{
		vx = 0;
		//px = startX;
		
		for (int x = 0; x < imageWidth; x++)
		{
			u8 v;
			
			if (addr == 0x0800)
			{
				addr = 0x1800;
				vx += 1;
				continue;
			}
			else if (addr == 0x1810)
			{
				vx = 0;
				vy += 1;
				addr = 0x1C00;
			}
			else if (addr == 0x1C10)
			{
				LOGD("h=%d w=%d | vy=%d vx=%d addr=%x", imageHeight, imageWidth, vy, vx, addr);
				
				// end
				vx = imageWidth;
				vy = imageHeight;
				break;
			}
							
			CDebugMemoryCell *cell = debugMemory->GetMemoryCell(addr);
			v = memoryBuffer[addr];

			// NOTE: this was not respecting isExecuteCode, isExecuteArgument previously, is it a bug or intentionally?
//				if (addr == pc)
//				{
//					cell->sr = 1.0f;
//					cell->sg = 1.0f;
//					cell->sb = 1.0f;
//					cell->sa = 1.0f;
//				}
//
//				cell->rr = cell->vr + cell->sr;
//				cell->rg = cell->vg + cell->sg;
//				cell->rb = cell->vb + cell->sb;
//				cell->ra = cell->va + cell->sa;
//
//				if (cell->rr > 1.0) cell->rr = 1.0;
//				if (cell->rg > 1.0) cell->rg = 1.0;
//				if (cell->rb > 1.0) cell->rb = 1.0;
//				if (cell->ra > 1.0) cell->ra = 1.0;

			cell->UpdateCellColors(v, showCurrentExecutePC, pc);
			
			imageDataMemoryMap->SetPixelResultRGBA(vx, vy, cell->rr*255.0f, cell->rg*255.0f, cell->rb*255.0f, cell->ra*255.0f);
			
			addr++;
			vx += 1;
		}
		
		vy += 1;
	}
	 
	 */
}

int CDataAdapterDrive1541Minimal::AdapterGetDataLength()
{
	return DRIVE1541_MINIMAL_MEMORY_ADAPTER_LENGTH;
}

void CDataAdapterDrive1541Minimal::AdapterReadByte(int pointer, uint8 *value, bool *isAvailable)
{
	int address = mapPointerToDriveAddress[pointer];
	drive1541DataAdapter->AdapterReadByte(address, value, isAvailable);
}

void CDataAdapterDrive1541Minimal::AdapterWriteByte(int pointer, uint8 value, bool *isAvailable)
{
	int address = mapPointerToDriveAddress[pointer];
	drive1541DataAdapter->AdapterWriteByte(address, value, isAvailable);
}


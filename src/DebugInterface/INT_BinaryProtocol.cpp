#include "INT_BinaryProtocol.h"
#include "SYS_PIPE.h"
#include "CViewC64.h"
#include "CViewMainMenu.h"
#include "CViewDisassembly.h"

void INT_InterpretBinaryPacket(CByteBuffer *inByteBuffer)
{
	LOGD("INT_InterpretBinaryPacket");
	
	u16 packetId = inByteBuffer->GetU16();
	u32 sequenceNumber = inByteBuffer->GetU32();
	
	LOGD("... packetId=%02x sequenceNumber=%d", packetId, sequenceNumber);
	
	// simple parser for now
	switch(packetId)
	{
		case C64D_PACKET_STATUS:
			// skip
			break;
			
		case C64D_PACKET_LOAD_FILE:
		{
			bool autoStart = inByteBuffer->GetBool();
			bool showLoadingAddr = inByteBuffer->GetBool();
			bool forceReset = inByteBuffer->GetBool();
			CSlrString *path = inByteBuffer->GetSlrString();
			bool ret = viewC64->viewC64MainMenu->LoadPRG(path, autoStart, true, showLoadingAddr, forceReset);
			INT_SendBinaryPacketStatus(sequenceNumber, ret ? C64D_PACKET_STATUS_OK : C64D_PACKET_STATUS_ERROR_NOT_FOUND);
		} break;
			
		case C64D_PACKET_PAUSE_RUN:
		{
			viewC64->StepOverInstruction();
			INT_SendBinaryPacketStatus(sequenceNumber, C64D_PACKET_STATUS_OK);
		} break;
		
		case C64D_PACKET_CONTINUE_RUN:
		{
			viewC64->RunContinueEmulation();
			INT_SendBinaryPacketStatus(sequenceNumber, C64D_PACKET_STATUS_OK);
		} break;
			
		case C64D_PACKET_STEP_ONE_CYCLE:
		{
			viewC64->StepOneCycle();
			INT_SendBinaryPacketStatus(sequenceNumber, C64D_PACKET_STATUS_OK);
		} break;
			
		case C64D_PACKET_STEP_ONE_INSTRUCTION:
		{
			viewC64->StepOverInstruction();
			INT_SendBinaryPacketStatus(sequenceNumber, C64D_PACKET_STATUS_OK);
		} break;
		
		case C64D_PACKET_STEP_OVER_JSR:
		{
			CViewDisassembly *viewDisassembly = viewC64->GetActiveDisassemblyView();
			if (viewDisassembly)
			{
				viewDisassembly->StepOverJsr();
				INT_SendBinaryPacketStatus(sequenceNumber, C64D_PACKET_STATUS_OK);
			}
			else
			{
				INT_SendBinaryPacketStatus(sequenceNumber, C64D_PACKET_STATUS_ERROR_NOT_FOUND);
			}
		} break;
			
		case C64D_PACKET_RESET_SOFT:
			viewC64->SoftReset();
			break;
			
		case C64D_PACKET_RESET_HARD:
			viewC64->HardReset();
			break;
			
		case C64D_PACKET_JMP_TO_ADDRESS_AT_CURSOR:
		{
			CViewDisassembly *viewDisassembly = viewC64->GetActiveDisassemblyView();
			if (viewDisassembly)
			{
				viewDisassembly->MakeJMPToCursor();
				INT_SendBinaryPacketStatus(sequenceNumber, C64D_PACKET_STATUS_OK);
			}
			else
			{
				INT_SendBinaryPacketStatus(sequenceNumber, C64D_PACKET_STATUS_ERROR_NOT_FOUND);
			}
		} break;
			
		case C64D_PACKET_MOVE_CURSOR_TO_ADDRESS:
		{
			u16 address = inByteBuffer->GetU16();
			
			CViewDisassembly *viewDisassembly = viewC64->GetActiveDisassemblyView();
			if (viewDisassembly)
			{
				viewDisassembly->ScrollToAddress(address);
				INT_SendBinaryPacketStatus(sequenceNumber, C64D_PACKET_STATUS_OK);
			}
			else
			{
				INT_SendBinaryPacketStatus(sequenceNumber, C64D_PACKET_STATUS_ERROR_NOT_FOUND);
			}
		} break;
			
		case C64D_PACKET_MAKE_JMP_TO_ADDRESS:
		{
			u16 address = inByteBuffer->GetU16();
			
			CViewDisassembly *viewDisassembly = viewC64->GetActiveDisassemblyView();
			if (viewDisassembly)
			{
				viewDisassembly->MakeJMPToAddress(address);
				INT_SendBinaryPacketStatus(sequenceNumber, C64D_PACKET_STATUS_OK);
			}
			else
			{
				INT_SendBinaryPacketStatus(sequenceNumber, C64D_PACKET_STATUS_ERROR_NOT_FOUND);
			}
		} break;

		case C64D_PACKET_GET_CPU_STATUS:
			LOGError("C64D_PACKET_GET_CPU_STATUS not implemented");
			break;
			
		case C64D_PACKET_SET_BREAKPOINT_PC:
		{
			u16 address = inByteBuffer->GetU16();
			bool setOn = inByteBuffer->GetBool();
			
			CViewDisassembly *viewDisassembly = viewC64->GetActiveDisassemblyView();
			if (viewDisassembly)
			{
				viewDisassembly->SetBreakpointPC(address, setOn);
				INT_SendBinaryPacketStatus(sequenceNumber, C64D_PACKET_STATUS_OK);
			}
			else
			{
				INT_SendBinaryPacketStatus(sequenceNumber, C64D_PACKET_STATUS_ERROR_NOT_FOUND);
			}
		} break;
		
		default:
			LOGError("INT_InterpretBinaryPacket: Unknown packet id=%02x", packetId);
			break;
	}
	
	
	LOGD("INT_InterpretBinaryPacket: done");
}

void INT_SendBinaryPacketStatus(u32 sequenceNumber, u8 errorNum)
{
	CByteBuffer *byteBuffer = new CByteBuffer();
	byteBuffer->putU16(C64D_PACKET_STATUS);
	byteBuffer->PutU32(sequenceNumber);
	byteBuffer->PutU8(errorNum);
}

void INT_SendBinaryPacket(u16 packetId, u32 sequenceNumber, CByteBuffer *data)
{
	LOGD("INT_SendBinaryPacket: packetId=%02x sequenceNumber=%d", packetId, sequenceNumber);
	
	LOGD("INT_SendBinaryPacket: done");
}


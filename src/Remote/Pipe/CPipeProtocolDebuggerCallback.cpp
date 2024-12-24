#include "CPipeProtocolDebuggerCallback.h"
#include "SYS_PIPE.h"
#include "CViewC64.h"
#include "CMainMenuHelper.h"
#include "CViewDisassembly.h"

bool CPipeProtocolDebuggerCallback::PipeProtocolCallbackInterpretPacket(CByteBuffer *inByteBuffer)
{
	LOGD("CPipeProtocolDebuggerCallback");
	
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
			bool ret = viewC64->mainMenuHelper->LoadPRG(path, autoStart, true, showLoadingAddr, forceReset);
			SendBinaryPacketStatus(sequenceNumber, ret ? C64D_PACKET_STATUS_OK : C64D_PACKET_STATUS_ERROR_NOT_FOUND);
		} break;
			
		case C64D_PACKET_PAUSE_RUN:
		{
			viewC64->StepOverInstruction();
			SendBinaryPacketStatus(sequenceNumber, C64D_PACKET_STATUS_OK);
		} break;
		
		case C64D_PACKET_CONTINUE_RUN:
		{
			viewC64->RunContinueEmulation();
			SendBinaryPacketStatus(sequenceNumber, C64D_PACKET_STATUS_OK);
		} break;
			
		case C64D_PACKET_STEP_ONE_CYCLE:
		{
			viewC64->StepOneCycle();
			SendBinaryPacketStatus(sequenceNumber, C64D_PACKET_STATUS_OK);
		} break;
			
		case C64D_PACKET_STEP_ONE_INSTRUCTION:
		{
			viewC64->StepOverInstruction();
			SendBinaryPacketStatus(sequenceNumber, C64D_PACKET_STATUS_OK);
		} break;
		
		case C64D_PACKET_STEP_OVER_JSR:
		{
			CViewDisassembly *viewDisassembly = viewC64->GetActiveDisassemblyView();
			if (viewDisassembly)
			{
				viewDisassembly->StepOverJsr();
				SendBinaryPacketStatus(sequenceNumber, C64D_PACKET_STATUS_OK);
			}
			else
			{
				SendBinaryPacketStatus(sequenceNumber, C64D_PACKET_STATUS_ERROR_NOT_FOUND);
			}
		} break;
			
		case C64D_PACKET_RESET_SOFT:
			viewC64->ResetSoft();
			break;
			
		case C64D_PACKET_RESET_HARD:
			viewC64->ResetHard();
			break;
			
		case C64D_PACKET_JMP_TO_ADDRESS_AT_CURSOR:
		{
			CViewDisassembly *viewDisassembly = viewC64->GetActiveDisassemblyView();
			if (viewDisassembly)
			{
				viewDisassembly->MakeJMPToCursor();
				SendBinaryPacketStatus(sequenceNumber, C64D_PACKET_STATUS_OK);
			}
			else
			{
				SendBinaryPacketStatus(sequenceNumber, C64D_PACKET_STATUS_ERROR_NOT_FOUND);
			}
		} break;
			
		case C64D_PACKET_MOVE_CURSOR_TO_ADDRESS:
		{
			u16 address = inByteBuffer->GetU16();
			
			CViewDisassembly *viewDisassembly = viewC64->GetActiveDisassemblyView();
			if (viewDisassembly)
			{
				viewDisassembly->ScrollToAddress(address);
				SendBinaryPacketStatus(sequenceNumber, C64D_PACKET_STATUS_OK);
			}
			else
			{
				SendBinaryPacketStatus(sequenceNumber, C64D_PACKET_STATUS_ERROR_NOT_FOUND);
			}
		} break;
			
		case C64D_PACKET_MAKE_JMP_TO_ADDRESS:
		{
			u16 address = inByteBuffer->GetU16();
			
			CViewDisassembly *viewDisassembly = viewC64->GetActiveDisassemblyView();
			if (viewDisassembly)
			{
				viewDisassembly->MakeJMPToAddress(address);
				SendBinaryPacketStatus(sequenceNumber, C64D_PACKET_STATUS_OK);
			}
			else
			{
				SendBinaryPacketStatus(sequenceNumber, C64D_PACKET_STATUS_ERROR_NOT_FOUND);
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
				SendBinaryPacketStatus(sequenceNumber, C64D_PACKET_STATUS_OK);
			}
			else
			{
				SendBinaryPacketStatus(sequenceNumber, C64D_PACKET_STATUS_ERROR_NOT_FOUND);
			}
		} break;
		
		default:
			LOGError("PipeProtocolCallbackInterpretPacket: Unknown packet id=%02x", packetId);
			return false;
	}
	
	
	LOGD("PipeProtocolCallbackInterpretPacket: done");
	return true;
}

void CPipeProtocolDebuggerCallback::SendBinaryPacketStatus(u32 sequenceNumber, u8 errorNum)
{
	CByteBuffer *byteBuffer = new CByteBuffer();
	byteBuffer->putU16(C64D_PACKET_STATUS);
	byteBuffer->PutU32(sequenceNumber);
	byteBuffer->PutU8(errorNum);

	// TODO: sync mutex
	PIPE_SendByteBuffer(byteBuffer);
}

void CPipeProtocolDebuggerCallback::SendBinaryPacket(u16 packetId, u32 sequenceNumber, CByteBuffer *data)
{
	LOGD("CPipeProtocolDebuggerCallback::SendBinaryPacket: packetId=%02x sequenceNumber=%d", packetId, sequenceNumber);

	// TODO: do we really need sequence number and packetId?
	PIPE_SendByteBuffer(data);

	LOGD("CPipeProtocolDebuggerCallback::SendBinaryPacket: done");
}


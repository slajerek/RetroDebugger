#include "CDebugInterfaceNesTasks.h"
#include "NesWrapper.h"
#include "CDebugInterfaceNes.h"
#include "CSnapshotsManager.h"
#include "CSlrString.h"

CDebugInterfaceNesTaskJoystickEvent::CDebugInterfaceNesTaskJoystickEvent(CDebugInterfaceNes *debugInterface, u8 buttonState, int port, u32 axis)
{
	this->debugInterface = debugInterface;
	this->port = port;
	this->buttonState = buttonState;
	this->axis = axis;
}

void CDebugInterfaceNesTaskJoystickEvent::ExecuteTask()
{
	if (debugInterface->snapshotsManager->isStoreInputEventsEnabled)
	{
		// TODO: move me to CDebugInterfaceNes!
		
		// Note: this buffer is not cleared as it may store other events, we need to 'unpack' and replace
		CByteBuffer *inputEventsBuffer = debugInterface->GetInputEventsBufferForCurrentCycle();
		inputEventsBuffer->Rewind();

		bool found = false;
		while (inputEventsBuffer->IsEof() == false)
		{
			int bufferEventPort = inputEventsBuffer->GetI32();
			if (bufferEventPort == port)
			{
				// replace
				int len = inputEventsBuffer->length;
				inputEventsBuffer->PutU32(axis);
				inputEventsBuffer->PutU8(buttonState);
				inputEventsBuffer->length = len;
				found = true;
			}
			else
			{
				// skip
				inputEventsBuffer->GetU32();
				inputEventsBuffer->GetU8();
			}
		}
		
		if (found == false)
		{
			// not found, add at the end
			inputEventsBuffer->PutI32(port);
			inputEventsBuffer->PutU32(axis);
			inputEventsBuffer->PutU8(buttonState);
		}
	}
	
	debugInterface->ProcessJoystickEventSynced(port, axis, buttonState);
}

CDebugInterfaceNesTaskInsertCartridge::CDebugInterfaceNesTaskInsertCartridge(CDebugInterfaceNes *debugInterface, CSlrString *pathToCart)
{
	this->debugInterface = debugInterface;
	this->pathToCart = new CSlrString(pathToCart);
}
CDebugInterfaceNesTaskInsertCartridge::~CDebugInterfaceNesTaskInsertCartridge()
{
	delete this->pathToCart;
}

void CDebugInterfaceNesTaskInsertCartridge::ExecuteTask()
{
	char *cPathToCart = this->pathToCart->GetStdASCII();
	bool ret = nesd_insert_cartridge(cPathToCart);

	delete cPathToCart;
	
	debugInterface->ResetEmulationFrameCounter();
	debugInterface->ResetClockCounters();
	debugInterface->ClearHistory();
}

//
CDebugInterfaceNesTaskReset::CDebugInterfaceNesTaskReset(CDebugInterfaceNes *debugInterface)
{
	this->debugInterface = debugInterface;
}

void CDebugInterfaceNesTaskReset::ExecuteTask()
{
	nesd_reset();
}

//
CDebugInterfaceNesTaskResetHard::CDebugInterfaceNesTaskResetHard(CDebugInterfaceNes *debugInterface)
{
	this->debugInterface = debugInterface;
}

void CDebugInterfaceNesTaskResetHard::ExecuteTask()
{
	nesd_reset();
	
	debugInterface->ResetEmulationFrameCounter();
	debugInterface->ResetClockCounters();
	debugInterface->ClearHistory();
}


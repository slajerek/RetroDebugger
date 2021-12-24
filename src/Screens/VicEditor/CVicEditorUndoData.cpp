#include "CVicEditorUndoData.h"
#include "CByteBuffer.h"
#include "VID_Main.h"

CVicEditorUndoData::CVicEditorUndoData(CViewVicEditor *vicEditor)
{
	this->vicEditor = vicEditor;
	this->undoDataBuffer = new CByteBuffer();
}

CVicEditorUndoData::~CVicEditorUndoData()
{
	
}

void CVicEditorUndoData::StoreUndoData()
{
	LOGD("CVicEditorUndoData::StoreUndoData");
	
	unsigned long t = SYS_GetCurrentTimeInMillis();
	

	LOGD("CVicEditorUndoData::StoreUndoData done, time=%dms", SYS_GetCurrentTimeInMillis() - t);
}

void CVicEditorUndoData::RestoreUndoData()
{
	this->undoDataBuffer->Rewind();
	
}

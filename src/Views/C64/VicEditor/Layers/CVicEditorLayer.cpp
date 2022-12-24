#include "CVicEditorLayer.h"
#include "CByteBuffer.h"
#include "CViewC64VicEditor.h"
#include "C64VicDisplayCanvas.h"
#include "CDebugInterfaceC64.h"

CVicEditorLayer::CVicEditorLayer(CViewC64VicEditor *vicEditor, const char *layerName)
{
	this->debugInterface = vicEditor->debugInterface;
	this->vicEditor = vicEditor;
	this->layerName = layerName;
	this->isVisible = true;
	this->isPaintingLocked = false;
}


CVicEditorLayer::~CVicEditorLayer()
{
	
}

void CVicEditorLayer::RenderMain(vicii_cycle_state_t *viciiState)
{
	SYS_FatalExit("CVicEditorLayer::RenderMain");
}

void CVicEditorLayer::RenderGridMain(vicii_cycle_state_t *viciiState)
{
}

void CVicEditorLayer::RenderPreview(vicii_cycle_state_t *viciiState)
{
	SYS_FatalExit("CVicEditorLayer::RenderPreview");
}

void CVicEditorLayer::RenderGridPreview(vicii_cycle_state_t *viciiState)
{
}

void CVicEditorLayer::ClearScreen()
{
	
}

void CVicEditorLayer::ClearScreen(u8 charValue, u8 colorValue)
{
	
}

bool CVicEditorLayer::PixelBelongsToLayer(int x, int y)
{
	return false;
}

u8 CVicEditorLayer::Paint(bool forceColorReplace, bool isDither, int x, int y, u8 colorLMB, u8 colorRMB, u8 colorSource, int charValue)
{
	return PAINT_RESULT_OUTSIDE;
}

bool CVicEditorLayer::GetColorAtPixel(int x, int y, u8 *color)
{
	return false;
}

void CVicEditorLayer::LayerSelected(bool isSelected)
{
}

void CVicEditorLayer::Serialize(CByteBuffer *byteBuffer)
{
	LOGD("isVisible=%s", STRBOOL(this->isVisible));
	byteBuffer->PutBool(this->isVisible);
}

void CVicEditorLayer::Deserialize(CByteBuffer *byteBuffer, int version)
{
	if (version >= 2)
	{
		this->isVisible = byteBuffer->GetBool();
		
		LOGD("isVisible=%s", STRBOOL(this->isVisible));
	}
}


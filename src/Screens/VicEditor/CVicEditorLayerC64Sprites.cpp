#include "CVicEditorLayerC64Sprites.h"
#include "CViewVicEditor.h"
#include "C64VicDisplayCanvas.h"
#include "CVicEditorLayerVirtualSprites.h"
#include "VID_Main.h"

CVicEditorLayerC64Sprites::CVicEditorLayerC64Sprites(CViewVicEditor *vicEditor)
: CVicEditorLayer(vicEditor, "C64 Sprites")
{
	this->isVisible = false;
}

CVicEditorLayerC64Sprites::~CVicEditorLayerC64Sprites()
{
	
}

void CVicEditorLayerC64Sprites::RenderMain(vicii_cycle_state_t *viciiState)
{
	vicEditor->viewVicDisplayMain->RenderDisplaySprites(viciiState);
}

void CVicEditorLayerC64Sprites::RenderPreview(vicii_cycle_state_t *viciiState)
{
	vicEditor->viewVicDisplaySmall->RenderDisplaySprites(viciiState);
}

void CVicEditorLayerC64Sprites::RenderGridMain(vicii_cycle_state_t *viciiState)
{
	if (this->isVisible && vicEditor->viewVicDisplayMain->showSpritesFrames)
	{
		vicEditor->viewVicDisplayMain->RenderGridSpritesOnly(viciiState);
	}
}

void CVicEditorLayerC64Sprites::RenderGridPreview(vicii_cycle_state_t *viciiState)
{
	if (this->isVisible && vicEditor->viewVicDisplaySmall->showSpritesFrames)
	{
		vicEditor->viewVicDisplaySmall->RenderGridSpritesOnly(viciiState);
	}
}

u8 CVicEditorLayerC64Sprites::Paint(bool forceColorReplace, bool isDither, int x, int y, u8 colorLMB, u8 colorRMB, u8 colorSource, int charValue)
{
	return vicEditor->layerVirtualSprites->Paint(forceColorReplace, isDither, x, y, colorLMB, colorRMB, colorSource, charValue);
}

bool CVicEditorLayerC64Sprites::GetColorAtPixel(int x, int y, u8 *color)
{
	return vicEditor->layerVirtualSprites->GetColorAtPixel(x, y, color);
}

void CVicEditorLayerC64Sprites::Serialise(CByteBuffer *byteBuffer)
{
	CVicEditorLayer::Serialise(byteBuffer);
}

void CVicEditorLayerC64Sprites::Deserialise(CByteBuffer *byteBuffer, int version)
{
	CVicEditorLayer::Deserialise(byteBuffer, version);
}

#include "CVicEditorLayerC64Canvas.h"
#include "CViewC64VicEditor.h"
#include "C64VicDisplayCanvas.h"
#include "CViewC64VicDisplay.h"
#include "C64VicDisplayCanvas.h"
#include "CDebugInterfaceC64.h"
#include "VID_Main.h"

CVicEditorLayerC64Canvas::CVicEditorLayerC64Canvas(CViewC64VicEditor *vicEditor)
: CVicEditorLayer(vicEditor, "Display")
{
	this->isVisible = true;
}

CVicEditorLayerC64Canvas::~CVicEditorLayerC64Canvas()
{
	
}

void CVicEditorLayerC64Canvas::RenderMain(vicii_cycle_state_t *viciiState)
{
	vicEditor->viewVicDisplay->RenderDisplayScreen();
}

void CVicEditorLayerC64Canvas::RenderPreview(vicii_cycle_state_t *viciiState)
{
	// render preview screen using already rendered texture
	vicEditor->viewVicDisplay->RenderDisplayScreen(vicEditor->viewVicDisplay->imageScreen);
}

void CVicEditorLayerC64Canvas::RenderGridMain(vicii_cycle_state_t *viciiState)
{
	if (vicEditor->viewVicDisplay->showGridLines)
	{
		vicEditor->viewVicDisplay->currentCanvas->RenderGridLines();
	}
}

void CVicEditorLayerC64Canvas::RenderGridPreview(vicii_cycle_state_t *viciiState)
{
	if (vicEditor->viewVicDisplay->showGridLines)
	{
		vicEditor->viewVicDisplay->currentCanvas->RenderGridLines();
	}
}

// @returns if pixel belongs to this layer
bool CVicEditorLayerC64Canvas::PixelBelongsToLayer(int x, int y)
{
	if (x >= 0 && x <= 319 && y >= 0 && y <= 200)
		return true;
	
	return false;
}

u8 CVicEditorLayerC64Canvas::Paint(bool forceColorReplace, bool isDither, int x, int y, u8 colorLMB, u8 colorRMB, u8 colorSource, int charValue)
{
	if (x >= 0 && x <= 319 && y >= 0 && y <= 200)
	{
		return vicEditor->viewVicDisplay->currentCanvas->Paint(forceColorReplace, isDither, x, y, colorLMB, colorRMB, colorSource, charValue);
	}

	return PAINT_RESULT_OUTSIDE;
}

bool CVicEditorLayerC64Canvas::GetColorAtPixel(int x, int y, u8 *color)
{
	if (PixelBelongsToLayer(x, y))
	{
		*color = vicEditor->viewVicDisplay->currentCanvas->GetColorAtPixel(x, y);
		return true;
	}
	
	return false;
}

void CVicEditorLayerC64Canvas::ClearScreen()
{
	LOGD("CVicEditorLayerC64Canvas::ClearScreen");
	vicEditor->viewVicDisplay->currentCanvas->ClearScreen();
}

void CVicEditorLayerC64Canvas::ClearScreen(u8 charValue, u8 colorValue)
{
	vicEditor->viewVicDisplay->currentCanvas->ClearScreen(charValue, colorValue);
}

void CVicEditorLayerC64Canvas::Serialize(CByteBuffer *byteBuffer)
{
	LOGD("CVicEditorLayerC64Canvas::Serialize");

	CVicEditorLayer::Serialize(byteBuffer);

	// simple store current canvas
	vicii_cycle_state_t *viciiState = vicEditor->viewVicDisplay->currentCanvas->viciiState;
	if (viciiState == NULL)
	{
		LOGError("CVicEditorLayerC64Canvas::Serialize: viciiState can't be NULL!");
		return;
	}
	
	u8 *screen_ptr;
	u8 *color_ram_ptr;
	u8 *chargen_ptr;
	u8 *bitmap_low_ptr;
	u8 *bitmap_high_ptr;
	u8 colors[0x0F];
	
	vicEditor->viewVicDisplay->GetViciiPointers(viciiState, &screen_ptr, &color_ram_ptr, &chargen_ptr, &bitmap_low_ptr, &bitmap_high_ptr, colors);

	byteBuffer->PutBytes(screen_ptr, 0x03E8);
	byteBuffer->PutBytes(color_ram_ptr, 0x03E8);
	
	byteBuffer->PutBytes(bitmap_low_ptr, 0x1000);
	byteBuffer->PutBytes(bitmap_high_ptr, 0x0F40);
}

void CVicEditorLayerC64Canvas::Deserialize(CByteBuffer *byteBuffer, int version)
{
	LOGD("CVicEditorLayerC64Canvas::Deserialize");
	
	CVicEditorLayer::Deserialize(byteBuffer, version);

	u16 screenPtr = vicEditor->viewVicDisplay->screenAddress;
	u16 colorPtr = 0xD800;
	u16 bitmapPtr = vicEditor->viewVicDisplay->bitmapAddress;

	LOGD("screenPtr=%04x", screenPtr);
	for (int i = 0; i < 0x03E8; i++)
	{
		u8 v = byteBuffer->GetByte();
		vicEditor->debugInterface->SetByteC64(screenPtr, v);
		
		screenPtr++;
	}
	
	for (int i = 0; i < 0x03E8; i++)
	{
		u8 v = byteBuffer->GetByte();
		vicEditor->debugInterface->SetByteC64(colorPtr, v);
		
		colorPtr++;
	}

	LOGD("bitmapPtr=%04x", bitmapPtr);

	for (int i = 0x0000; i < 0x1F40; i++)
	{
		u8 v = byteBuffer->GetByte();
		vicEditor->debugInterface->SetByteC64(bitmapPtr, v);

		bitmapPtr++;
	}
	
}


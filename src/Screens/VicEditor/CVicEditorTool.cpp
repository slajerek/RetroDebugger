#include "CVicEditorTool.h"
#include "C64VicDisplayCanvas.h"

CVicEditorTool::CVicEditorTool(CVicEditorToolPaintCallback *paintCallback, CImageData *draftImage)
{
	this->paintCallback = paintCallback;
	this->draftImage = draftImage;
	
	isPaintingDraft = false;
}

void CVicEditorTool::SelectTool()
{
	
}

void CVicEditorTool::DeselectTool()
{
	
}

void CVicEditorTool::DoTap(float x, float y)
{
	
}

void CVicEditorTool::DoMove(float x, float y)
{
	
}

void CVicEditorTool::DoFinishTap()
{
	
}

void CVicEditorTool::ToolPaintPixel(int px, int py)
{
	this->ToolPaintPixel(px, py, VICEDITOR_COLOR_SOURCE_LMB);
}

void CVicEditorTool::ToolPaintPixel(int px, int py, u8 colorSource)
{
	if (isPaintingDraft)
	{
		this->paintCallback->ToolPaintPixelOnDraft(px, py, colorSource);
	}
	else
	{
		this->paintCallback->ToolPaintPixel(px, py, colorSource);
	}
}

void CVicEditorTool::ToolPaintBrush(int px, int py)
{
	this->ToolPaintBrush(px, py, VICEDITOR_COLOR_SOURCE_LMB);
}

void CVicEditorTool::ToolPaintBrush(int px, int py, u8 colorSource)
{
	if (isPaintingDraft)
	{
		this->paintCallback->ToolPaintBrushOnDraft(px, py, colorSource);
	}
	else
	{
		this->paintCallback->ToolPaintBrush(px, py, colorSource);
	}
}


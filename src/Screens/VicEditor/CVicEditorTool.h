#ifndef _CVicEditorTool_h_
#define _CVicEditorTool_h_

#include "SYS_Defs.h"

class CImageData;

class CVicEditorToolPaintCallback
{
public:
	// abstract
	virtual void ToolPaintPixel(int px, int py, u8 colorSource) {};
	virtual void ToolPaintBrush(int px, int py, u8 colorSource) {};
	virtual void ToolPaintPixelOnDraft(int px, int py, u8 colorSource) {};
	virtual void ToolPaintBrushOnDraft(int px, int py, u8 colorSource) {};

	virtual void ToolClearDraftImage() {};
	virtual void ToolMakeDraftActive() {};
	virtual void ToolMakeDraftNotActive() {};
};

class CVicEditorTool
{
	CVicEditorTool(CVicEditorToolPaintCallback  *paintCallback, CImageData *draftImage);
	
	CVicEditorToolPaintCallback *paintCallback;
	CImageData *draftImage;
	
	virtual void SelectTool();
	virtual void DeselectTool();
	
	virtual void DoTap(float x, float y);
	virtual void DoMove(float x, float y);
	virtual void DoFinishTap();
	
	bool isPaintingDraft;
	
	virtual void ToolPaintPixel(int px, int py);
	virtual void ToolPaintPixel(int px, int py, u8 colorSource);
	virtual void ToolPaintBrush(int px, int py);
	virtual void ToolPaintBrush(int px, int py, u8 colorSource);
};

#endif

#include "CViewVicEditorDisplayPreview.h"
#include "CViewVicEditor.h"
#include "CVicEditorLayer.h"
#include "CSlrString.h"

CViewVicEditorDisplayPreview::CViewVicEditorDisplayPreview(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY,
														   CDebugInterfaceC64 *debugInterface, CSlrString *windowName, CViewVicEditor *vicEditor)
: CViewC64VicDisplay(name, posX, posY, posZ, sizeX, sizeY, debugInterface, windowName, GUI_FRAME_MODE_WINDOW, NULL)
{
	this->vicEditor = vicEditor;

	this->consumeTapBackground = false;
}

void CViewVicEditorDisplayPreview::SetViciiState(vicii_cycle_state_t *viciiState)
{
	this->viciiState = viciiState;
}

void CViewVicEditorDisplayPreview::Render()
{
	//LOGD("CViewVicEditorDisplayPreview::Render");
	
	if (this->visible)
	{
		// * render preview screen *
		BlitFilledRectangle(this->posX, this->posY, this->posZ,
							this->sizeX, this->sizeY, 0, 0, 0, 0.7f);
		
		// render the preview VIC Display
		VID_SetClipping(this->fullScanScreenPosX,
						this->fullScanScreenPosY,
						this->fullScanScreenSizeX,
						this->fullScanScreenSizeY);
		
		// render preview screen layers
		for (std::list<CVicEditorLayer *>::iterator it = vicEditor->layers.begin();
			 it != vicEditor->layers.end(); it++)
		{
			CVicEditorLayer *layer = *it;
			
			if (layer->isVisible)
			{
				layer->RenderPreview(viciiState);
			}
		}
		
		// render grid lines
		for (std::list<CVicEditorLayer *>::iterator it = vicEditor->layers.begin();
			 it != vicEditor->layers.end(); it++)
		{
			CVicEditorLayer *layer = *it;
			
			layer->RenderGridPreview(viciiState);
		}
		
		
		const float lineWidth = 1.25f;//0.7f;
		
		if (this->renderDisplayFrame)
		{
			BlitRectangle(this->displayFrameScreenPosX, this->displayFrameScreenPosY, this->posZ,
						  this->displayFrameScreenSizeX, this->displayFrameScreenSizeY,
						  0.43f, 0.45f, 0.43f, 1.0f, lineWidth);
		}
		
		VID_ResetClipping();
		
		if (this->showRasterCursor)
		{
			this->RenderCursor();
		}
	}
	
	if (this->visible && this->viewFrame)
		this->viewFrame->Render();
}

#include "CViewVicEditorLayers.h"
#include "SYS_Main.h"
#include "RES_ResourceManager.h"
#include "CGuiMain.h"
#include "CDataAdapter.h"
#include "CSlrString.h"
#include "SYS_KeyCodes.h"
#include "CViewC64.h"
#include "CViewMemoryMap.h"
#include "C64Tools.h"
#include "CViewC64Screen.h"
#include "CDebugInterfaceC64.h"
#include "SYS_Threading.h"
#include "CGuiEditHex.h"
#include "VID_ImageBinding.h"
#include "CViewC64.h"
#include "CDebugInterfaceVice.h"
#include "CViewVicEditor.h"
#include "CViewC64VicDisplay.h"
#include "CVicEditorLayer.h"
#include "CVicEditorLayerC64Canvas.h"
#include "CVicEditorLayerImage.h"
#include <list>

CViewVicEditorLayers::CViewVicEditorLayers(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CViewVicEditor *vicEditor)
: CGuiWindow(name, posX, posY, posZ, sizeX, sizeY, new CSlrString("Layers"), NULL)
{
	this->vicEditor = vicEditor;
	
	float px = 11.5f; //posX + 11.5f;
	float py = 1.0f; //posY + 1.0f;
	
	float sx = sizeX - 5.0f;
	float sy = sizeY; //57;
	
	font = viewC64->fontCBMShifted;
	fontScale = 1.25f;
	float upGap = 0.0f;
	float elementsGap = -1.0f;

	this->lstLayers = new CGuiList(px, py, posZ, sx, sy, fontScale,
									  NULL, 0, false,
									  font,
									  guiMain->theme->imgBackground, 1.0f,
									  this);
	this->lstLayers->SetGaps(upGap, elementsGap);
	this->lstLayers->textOffsetY = 2.1f;
	this->lstLayers->fontSelected = viewC64->fontCBMShifted;
	this->AddGuiElement(this->lstLayers);

	RefreshLayers();
	
	// always run
	this->UpdatePosition();
}

void CViewVicEditorLayers::RefreshLayers()
{
	guiMain->LockMutex();
	
	while(!btnsVisible.empty())
	{
		CGuiButtonSwitch *btn = btnsVisible.back();
		btnsVisible.pop_back();
		
		RemoveGuiElement(btn);
		delete btn;
	}
	char **items = new char *[vicEditor->layers.size()];
	
	int i = 0;
	for (std::list<CVicEditorLayer *>::reverse_iterator it = vicEditor->layers.rbegin();
		 it != vicEditor->layers.rend(); it++)
	{
		CVicEditorLayer *layer = *it;
		int len = strlen(layer->layerName);
		
		items[i] = new char[len+1];
		strcpy(items[i], layer->layerName);
		
		i++;
	}
	this->lstLayers->Init(items, vicEditor->layers.size(), true);

	// create buttons
	float px = 2.0f; //posX + 2.0f;
	float py = 2.5f; //posY + 2.5f;
	float buttonSizeX = 12.0f;
	float buttonSizeY = 8.0f;

	for (std::list<CVicEditorLayer *>::reverse_iterator it = vicEditor->layers.rbegin();
		 it != vicEditor->layers.rend(); it++)
	{
		CVicEditorLayer *layer = *it;
		
		CSlrString *buttonText;
		if (layer != (CVicEditorLayer *)vicEditor->layerVirtualSprites)
		{
			buttonText = new CSlrString("V");
		}
		else
		{
			// virtual sprites layer does not show sprites
			buttonText = new CSlrString("P");
		}
		
		CGuiButtonSwitch *btnVisible = new CGuiButtonSwitch(NULL, NULL, NULL,
												 px, py, posZ, buttonSizeX, buttonSizeY,
												 buttonText,
												 FONT_ALIGN_CENTER, buttonSizeX/2, 3.5,
												 font, fontScale,
												 1.0, 1.0, 1.0, 1.0,
												 1.0, 1.0, 1.0, 1.0,
												 0.3, 0.3, 0.3, 1.0,
												 this);
		btnVisible->SetOn(layer->isVisible);
		btnVisible->textDrawPosY = 1.75f;
		btnVisible->buttonEnabledColorA = 0.25f;
		btnVisible->userData = layer;
		this->AddGuiElement(btnVisible);
	
		this->btnsVisible.push_back(btnVisible);

		py += 9.0f;
	}
	
	guiMain->UnlockMutex();

}

void CViewVicEditorLayers::SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY)
{
	LOGD("CViewVicEditorLayers::SetPosition: %f %f", posX, posY);
	
	CGuiWindow::SetPosition(posX, posY, posZ, sizeX, sizeY);
}

void CViewVicEditorLayers::UpdateVisibleSwitchButtons()
{
	for (std::vector<CGuiButtonSwitch *>::iterator it = this->btnsVisible.begin();
		 it != this->btnsVisible.end(); it++)
	{
		CGuiButtonSwitch *btnVisible = *it;
		CVicEditorLayer *layer = (CVicEditorLayer *)btnVisible->userData;
		
		btnVisible->SetOn(layer->isVisible);
	}

}

bool CViewVicEditorLayers::ButtonSwitchChanged(CGuiButtonSwitch *button)
{
	LOGG("CViewVicEditorLayers::ButtonSwitchChanged");
	
	CVicEditorLayer *layer = (CVicEditorLayer *)button->userData;
	
	layer->isVisible = button->IsOn();
	
	if (layer == (CVicEditorLayer*)vicEditor->layerReferenceImage)
	{
		vicEditor->UpdateReferenceLayers();
	}
	
	return false;
}

void CViewVicEditorLayers::SetLayerVisible(CVicEditorLayer *layer, bool isVisible)
{
	LOGG("CViewVicEditorLayers::SetLayerVisible");
	
	layer->isVisible = isVisible;

	if (layer == (CVicEditorLayer*)vicEditor->layerReferenceImage)
	{
		vicEditor->UpdateReferenceLayers();
	}
	
	UpdateVisibleSwitchButtons();
}

void CViewVicEditorLayers::ListElementSelected(CGuiList *listBox)
{
	LOGG("CViewVicEditorLayers::ListElementSelected");
	
	if (listBox->selectedElement == -1)
	{
		SelectLayer(NULL);
	}
	else
	{
		CGuiButtonSwitch *btnLayer = btnsVisible[listBox->selectedElement];
		SelectLayer((CVicEditorLayer *)btnLayer->userData);
	}
}

void CViewVicEditorLayers::SelectLayer(CVicEditorLayer *layer)
{
	if (vicEditor->selectedLayer == layer || layer == NULL)
	{
		// unselect
		vicEditor->SelectLayer(NULL);
		lstLayers->SetElement(-1, false);
		return;
	}
	
	vicEditor->SelectLayer(layer);
}

void CViewVicEditorLayers::SelectNextLayer()
{
	LOGG("CViewVicEditorLayers::SelectNextLayer");
	
	int el = this->lstLayers->selectedElement;
	
	for (int i = 0; i < btnsVisible.size(); i++)
	{
		el++;
		if (el == btnsVisible.size())
		{
			el = 0;
		}
		
		if (btnsVisible[el]->IsOn())
		{
			this->lstLayers->SetElement(el, false);
			return;
		}
	}
}

void CViewVicEditorLayers::DoLogic()
{
	CGuiWindow::DoLogic();
}


void CViewVicEditorLayers::Render()
{
	BlitFilledRectangle(posX, posY, posZ, sizeX, sizeY, viewFrame->barColorR, viewFrame->barColorG, viewFrame->barColorB, 1);

	CGuiWindow::Render();

	BlitRectangle(posX, posY, posZ, sizeX, sizeY, 0, 0, 0, 1, 1);
}

bool CViewVicEditorLayers::DoTap(float x, float y)
{
	LOGG("CViewVicEditorLayers::DoTap: %f %f", x, y);

//	if (this->IsInsideView(x, y))
//	{
//		if (CGuiWindow::DoTapNoBackground(x, y) == false)
//		{
//			SelectLayer(NULL);
//		}
//	}
	
	if (CGuiWindow::DoTap(x, y) == false)
	{
		if (this->IsInsideView(x, y))
		{
			return true;
		}
	}
	
	return CGuiWindow::DoTap(x, y);;
}

bool CViewVicEditorLayers::DoMove(float x, float y, float distX, float distY, float diffX, float diffY)
{
	if (this->IsInsideView(x, y))
		return true;
	
	return CGuiWindow::DoMove(x, y, distX, distY, diffX, diffY);
}


bool CViewVicEditorLayers::DoRightClick(float x, float y)
{
	LOGI("CViewVicEditorLayers::DoRightClick: %f %f", x, y);
	
	if (this->IsInsideView(x, y))
		return true;
	
	return CGuiWindow::DoRightClick(x, y);
}


bool CViewVicEditorLayers::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	LOGD("CViewVicEditorLayers::KeyDown: %d", keyCode);
	
	return false;
}

bool CViewVicEditorLayers::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return false;
}



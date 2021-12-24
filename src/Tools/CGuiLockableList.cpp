/*
 *  CGuiLockableList.cpp
 *  MobiTracker
 *
 *  Created by Marcin Skoczylas on 09-12-03.
 *  Copyright 2009 Marcin Skoczylas. All rights reserved.
 *
 */

#include "CGuiLockableList.h"

#include "VID_Main.h"
#include "CGuiMain.h"
#include "SYS_KeyCodes.h"
#include <math.h>

CGuiLockableList::CGuiLockableList(float posX, float posY, float posZ, float sizeX, float sizeY, float fontSize,
				   char **elements, int numElements, bool deleteElements, CSlrFont *font,
				   CSlrImage *imgBackground, float backgroundAlpha,
				   CGuiListCallback *callback)
	: CGuiList(posX, posY, posZ, sizeX, sizeY, fontSize, elements, numElements, deleteElements, font, imgBackground, backgroundAlpha, callback)
{
	LOGG("CGuiLockableList::CGuiLockableList");
	this->name = "CGuiLockableList";

	isLocked = false;
	selectionColorR = 0.0f;
	selectionColorG = 0.7f;
	selectionColorB = 0.0f;
	
	LOGG("CGuiLockableList::CGuiLockableList done");
}

CGuiLockableList::~CGuiLockableList()
{
}

bool CGuiLockableList::DoScrollWheel(float deltaX, float deltaY)
{
	MoveView(deltaX, deltaY * 2.0f);
	return true;
}

bool CGuiLockableList::DoTap(float x, float y)
{
	LOGD("CGuiLockableList::DoTap");
	
	CGuiList::DoTap(x, y);
	return false;
}

void CGuiLockableList::Render()
{
	if (!visible)
		return;
	
	this->LockRenderMutex();
	
	if (imgBackground != NULL)
		imgBackground->RenderAlpha(posX, posY, posZ, sizeX, sizeY, backgroundAlpha);
	
	
	int elemNum;
	
	float drawY = startDrawY + startElementsY;
	float drawX = startDrawX;
	
	VID_SetClipping(posX, posY + startElementsY, sizeX, sizeY - startElementsY);
	
	for (elemNum = firstShowElement; elemNum < numElements; elemNum++)
	{
		if (elemNum < numElements)
		{
			if (typeOfElements == GUI_LIST_ELEMTYPE_CHARS)
			{
				if (elemNum == selectedElement)
				{
//					BlitFilledRectangle(this->posX, posY + drawY-1.0f, this->posZ, this->sizeX, this->fontSize*1.3f, 1.0f, 0.0f, 0.0f, 1.0f);
					BlitFilledRectangle(this->posX, posY + drawY-0.5f, this->posZ, this->sizeX, this->fontSize*1.0f + 0.5f,
										selectionColorR, selectionColorG, selectionColorB, 1.0f);
				}
			}
		}
		
		drawY += fontSize+elementsGap;
		
		if (drawY > posEndY)
			break;
	}
	
	drawY = startDrawY + startElementsY; //27;
	
	//LOGD("CGuiLockableList::Render: numElements=%d", numElements);
	for (elemNum = firstShowElement; elemNum < numElements; elemNum++)
	{
		//LOGD("	elemNum=%d", elemNum);
		drawX = startDrawX;
		
		drawX += GUI_GAP_WIDTH;
		
		if (elemNum < numElements)
		{
			if (typeOfElements == GUI_LIST_ELEMTYPE_CHARS)
			{
				char **elements = (char**)listElements;
				
				font->BlitText(elements[elemNum], posX + drawX, posY + drawY, this->posZ+0.01f, fontSize);
			}
		}
		
		drawY += fontSize+elementsGap;
		
		if (drawY > posEndY)
			break;
	}
	
	VID_ResetClipping();
	
	this->UnlockRenderMutex();
}

void CGuiLockableList::SetElement(int elementNum, bool updatePosition, bool runCallback)
{
	//LOGG("CGuiList::SetElement");
	if (elementNum < 0)
	{
		this->selectedElement = -1;
		return;
	}
	
	this->selectedElement = elementNum;
	
	if (updatePosition)
	{
		if ((this->selectedElement - numRollUp) < 0)
		{
			this->firstShowElement = 0;
		}
		else
		{
			this->firstShowElement = this->selectedElement - numRollUp;
		}
	}
	
	// TODO: CHECK THIS:
	if (this->callback && runCallback)
		this->callback->ListElementSelected(this);
	this->ElementSelected();
}

bool CGuiLockableList::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	guiMain->LockMutex();
	if (keyCode == MTKEY_ARROW_DOWN)
	{
		if (selectedElement < numElements-1)
		{
			this->SetElement(selectedElement+1, true, true);
		}
		SetListLocked(true);
		guiMain->UnlockMutex();
		return true;
	}
	else if (keyCode == MTKEY_ARROW_UP)
	{
		if (selectedElement > 0)
		{
			this->SetElement(selectedElement-1, true, true);
		}
		SetListLocked(true);
		guiMain->UnlockMutex();
		return true;
	}
	else if (keyCode == MTKEY_ESC)
	{
		SetListLocked(false);
	}
	guiMain->UnlockMutex();
	return false;
}

void CGuiLockableList::SetListLocked(bool isLocked)
{
	if (isLocked == false)
	{
		this->isLocked = false;
		this->selectionColorR = 0.0f;
		this->selectionColorG = 0.7f;
		this->selectionColorB = 0.0f;
	}
	else
	{
		this->isLocked = true;
		this->selectionColorR = 0.7f;
		this->selectionColorG = 0.0f;
		this->selectionColorB = 0.0f;
	}
}

bool CGuiLockableList::IsFocusable()
{
	return allowFocus;
}


bool CGuiLockableList::SetFocus()
{
	if (allowFocus)
	{
		return CGuiList::SetFocus();
	}
	
	return false;
}


/*
 *  CGuiLockableList.h
 *  Copyright 2009 Marcin Skoczylas. All rights reserved.
 *
 */

#ifndef _GUI_LOCKABLE_LIST_
#define _GUI_LOCKABLE_LIST_

#include "CSlrFont.h"
#include "CSlrImage.h"
#include "CGuiElement.h"
#include "CGuiView.h"
#include "CGuiList.h"

class CGuiLockableList : public CGuiList
{
public:
	CGuiLockableList(float posX, float posY, float posZ, float sizeX, float sizeY, float fontSize, //float fontWidth, float fontHeight,
			 char **elements, int numElements, bool deleteElements, CSlrFont *font, 
			 CSlrImage *imgBackground, float backgroundAlpha, CGuiListCallback *callback);
	~CGuiLockableList();
	
	virtual bool DoTap(float x, float y);

	virtual void Render();

	virtual bool DoScrollWheel(float deltaX, float deltaY);
	
	float selectionColorR, selectionColorG, selectionColorB;
	volatile bool isLocked;
	
	virtual bool KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper);
	
	virtual void SetElement(int elementNum, bool updatePosition, bool runCallback);

	virtual bool IsFocusable();
	virtual bool SetFocus();
	
	virtual void SetListLocked(bool isLocked);
};


#endif //_GUI_LIST_

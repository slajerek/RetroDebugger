//
// NOTE:
// This view will be removed. It is being refactored and moved to main menu bar instead
//

#include "EmulatorsConfig.h"
#include "CViewC64.h"
#include "CViewAbout.h"
#include "VID_Main.h"
#include "CGuiMain.h"
#include "CSlrString.h"
#include "C64Tools.h"
#include "SYS_KeyCodes.h"
#include "CSlrKeyboardShortcuts.h"
#include "CSlrFileFromOS.h"
#include "CColorsTheme.h"
#include "C64SettingsStorage.h"

#include "C64KeyboardShortcuts.h"
#include "CViewSnapshots.h"
#include "CDebugInterfaceC64.h"
#include "MTH_Random.h"

#include "CViewDataMap.h"

#include "CGuiMain.h"

CViewAbout::CViewAbout(float posX, float posY, float posZ, float sizeX, float sizeY)
: CGuiView(posX, posY, posZ, sizeX, sizeY)
{
	this->name = "CViewAbout";
	
	font = viewC64->fontDefaultCBMShifted;
	fontScale = 1.5;
	fontHeight = font->GetCharHeight('@', fontScale) + 2;
	
	strHeader = new CSlrString("About Retro Debugger");
}

CViewAbout::~CViewAbout()
{
}

void CViewAbout::DoLogic()
{
	
}

void CViewAbout::Render()
{
	if (viewC64->debugInterfaceC64)
	{
		this->RenderC64ViceLicense();
	}
	if (viewC64->debugInterfaceAtari)
	{
		this->RenderAtari800License();
	}
}

void CViewAbout::RenderC64ViceLicense()
{
	BlitFilledRectangle(0, 0, -1, sizeX, sizeY,
						viewC64->colorsTheme->colorBackgroundFrameR,
						viewC64->colorsTheme->colorBackgroundFrameG,
						viewC64->colorsTheme->colorBackgroundFrameB,
						1.0);
	
	float sb = 20;
	float gap = 4;
	
	float tr = viewC64->colorsTheme->colorTextR;
	float tg = viewC64->colorsTheme->colorTextG;
	float tb = viewC64->colorsTheme->colorTextB;
	
	float lr = viewC64->colorsTheme->colorHeaderLineR;
	float lg = viewC64->colorsTheme->colorHeaderLineG;
	float lb = viewC64->colorsTheme->colorHeaderLineB;
	float lSizeY = 3;
	
	float scrx = sb;
	float scry = sb;
	float scrsx = sizeX - sb*2.0f;
	float scrsy = sizeY - sb*2.0f;
	float cx = scrsx/2.0f + sb;
	
	float bg_r = viewC64->colorsTheme->colorBackgroundR;
	float bg_g = viewC64->colorsTheme->colorBackgroundG;
	float bg_b = viewC64->colorsTheme->colorBackgroundB;
	
	BlitFilledRectangle(scrx, scry, -1, scrsx, scrsy, bg_r, bg_g, bg_b, 1.0);
	
	float px = scrx + gap;
	float py = scry + gap;
	
	fontScale = 3.0f;
	fontHeight = font->GetCharHeight('@', fontScale) + 1;

	float htr = viewC64->colorsTheme->colorTextHeaderR;
	float htg = viewC64->colorsTheme->colorTextHeaderG;
	float htb = viewC64->colorsTheme->colorTextHeaderB;
	font->BlitTextColor(strHeader, cx, py, -1, fontScale, htr, htg, htb, 1, FONT_ALIGN_CENTER);

	py += fontHeight;
	//	font->BlitTextColor(strHeader2, cx, py, -1, fontScale, tr, tg, tb, 1, FONT_ALIGN_CENTER);
	//	py += fontHeight;
	py += 4.0f;
	
	BlitFilledRectangle(scrx, py, -1, scrsx, lSizeY, lr, lg, lb, 1);
	
	py += lSizeY + gap + 4.0f;
	
	
	//// TODO: this is a quick way to have it immediately implemented
	//// this needs of course to be changed into some more meaningful
	
	fontScale = 2.0f;
	fontHeight = font->GetCharHeight('@', fontScale) + 1;

	font->BlitTextColor("C64 Debugger is (C) Marcin Skoczylas, aka Slajerek/Samar", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
//	font->BlitTextColor("http://samar.untergrund.net", 338, py, posZ, 2.0f, 0.0f, 0.0f, 0.0f, 1);
	
	py += fontHeight;
	
	fontScale = 1.45f;
	fontHeight = font->GetCharHeight('@', fontScale) + 1;
	
	font->BlitTextColor("VICE, the Versatile Commodore Emulator", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	font->BlitTextColor("Copyright C 1998-2008 Andreas Boose", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	font->BlitTextColor("Copyright C 1998-2008 Dag Lem", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	font->BlitTextColor("Copyright C 1998-2008 Tibor Biczo", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	font->BlitTextColor("Copyright C 1999-2008 Andreas Matthies", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	font->BlitTextColor("Copyright C 1999-2008 Martin Pottendorfer", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	font->BlitTextColor("Copyright C 2000-2008 Spiro Trikaliotis", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	font->BlitTextColor("Copyright C 2005-2017 Marco van den Heuvel", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	font->BlitTextColor("Copyright C 2006-2008 Christian Vogelgsang", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	font->BlitTextColor("Copyright C 2007-2008 Fabrizio Gennari", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	font->BlitTextColor("Copyright C 1999-2007 Andreas Dehmel", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	font->BlitTextColor("Copyright C 2003-2005 David Hansel", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	font->BlitTextColor("Copyright C 2000-2004 Markus Brenner", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	font->BlitTextColor("", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	font->BlitTextColor("Copyright C 1999-2004 Thomas Bretz", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	font->BlitTextColor("Copyright C 1997-2001 Daniel Sladic", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	font->BlitTextColor("Copyright C 1996-1999 Ettore Perazzoli", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	font->BlitTextColor("Copyright C 1996-1999 André Fachat", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	font->BlitTextColor("Copyright C 1993-1994, 1997-1999 Teemu Rantanen", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	font->BlitTextColor("Copyright C 1993-1996 Jouko Valta", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	font->BlitTextColor("Copyright C 1993-1994 Jarkko Sonninen", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	font->BlitTextColor("Copyright C 1999-2017 Martin Pottendorfer", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	font->BlitTextColor("Copyright C 2007-2017 Fabrizio Gennari", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	font->BlitTextColor("Copyright C 2009-2017 Groepaz", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	font->BlitTextColor("Copyright C 2010-2017 Olaf Seibert", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	font->BlitTextColor("Copyright C 2011-2017 Marcus Sutton", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	font->BlitTextColor("Copyright C 2011-2017 Kajtar Zsolt", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	font->BlitTextColor("Copyright C 2016-2017 AreaScout", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	font->BlitTextColor("Copyright C 2016-2017 Bas Wassink", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	
	
//	font->BlitTextColor("", px, py, posZ, fontScale, tr, tg, tb, 1);
	py += fontHeight;

	fontScale = 0.8f;
	font->BlitTextColor("The ROM files embedded in the source code are Copyright C by Commodore Business Machines.", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;

	
	
	fontScale = 0.8f;
	fontHeight = font->GetCharHeight('@', fontScale) + 1;

	font->BlitTextColor("This program is free software; you can redistribute it and/or", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	font->BlitTextColor("modify it under the terms of the GNU General Public License as", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	font->BlitTextColor("published by the Free Software Foundation; either version 2 of the", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	font->BlitTextColor("License, or (at your option) any later version.", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	font->BlitTextColor("", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	font->BlitTextColor("This program is distributed in the hope that it will be useful,", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	font->BlitTextColor("but WITHOUT ANY WARRANTY; without even the implied warranty of", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	font->BlitTextColor("MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	font->BlitTextColor("GNU General Public License for more details.", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	font->BlitTextColor("You should have received a copy of the GNU General Public License", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	font->BlitTextColor("along with this program; if not, write to the Free Software", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	
	font->BlitTextColor("Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	
	font->BlitTextColor("02111-1307  USA", px, py, posZ, fontScale, tr, tg, tb, 1);

	font->BlitTextColor("Click anywhere to close this", 330, py, posZ, 2.0f, tr, tg, tb, 1);
	
}

void CViewAbout::RenderAtari800License()
{
//	LOGD("RenderAtari800License");
	BlitFilledRectangle(0, 0, -1, sizeX, sizeY,
						viewC64->colorsTheme->colorBackgroundFrameR,
						viewC64->colorsTheme->colorBackgroundFrameG,
						viewC64->colorsTheme->colorBackgroundFrameB,
						1.0);
	
	float sb = 20;
	float gap = 4;
	
	float tr = viewC64->colorsTheme->colorTextR;
	float tg = viewC64->colorsTheme->colorTextG;
	float tb = viewC64->colorsTheme->colorTextB;
	
	float lr = viewC64->colorsTheme->colorHeaderLineR;
	float lg = viewC64->colorsTheme->colorHeaderLineG;
	float lb = viewC64->colorsTheme->colorHeaderLineB;
	float lSizeY = 3;
	
	float scrx = sb;
	float scry = sb;
	float scrsx = sizeX - sb*2.0f;
	float scrsy = sizeY - sb*2.0f;
	float cx = scrsx/2.0f + sb;
	
	float bg_r = viewC64->colorsTheme->colorBackgroundR;
	float bg_g = viewC64->colorsTheme->colorBackgroundG;
	float bg_b = viewC64->colorsTheme->colorBackgroundB;
	
	BlitFilledRectangle(scrx, scry, -1, scrsx, scrsy, bg_r, bg_g, bg_b, 1.0);
	
	float px = scrx + gap;
	float py = scry + gap;
	
	fontScale = 3.0f;
	fontHeight = font->GetCharHeight('@', fontScale) + 1;
	
	float htr = viewC64->colorsTheme->colorTextHeaderR;
	float htg = viewC64->colorsTheme->colorTextHeaderG;
	float htb = viewC64->colorsTheme->colorTextHeaderB;
	font->BlitTextColor(strHeader, cx, py, -1, fontScale, htr, htg, htb, 1, FONT_ALIGN_CENTER);
	
	py += fontHeight;
	//	font->BlitTextColor(strHeader2, cx, py, -1, fontScale, tr, tg, tb, 1, FONT_ALIGN_CENTER);
	//	py += fontHeight;
	py += 4.0f;
	
	BlitFilledRectangle(scrx, py, -1, scrsx, lSizeY, lr, lg, lb, 1);
	
	py += lSizeY + gap + 4.0f;
	
	
	//// TODO: this is a quick way to have it immediately implemented
	//// this needs of course to be changed into some more meaningful
	
	fontScale = 2.0f;
	fontHeight = font->GetCharHeight('@', fontScale) + 1;
	
	font->BlitTextColor("65XE Debugger is (C) Marcin Skoczylas, aka Slajerek/Samar", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	//	font->BlitTextColor("http://samar.untergrund.net", 338, py, posZ, 2.0f, 0.0f, 0.0f, 0.0f, 1);
	
	py += fontHeight;
	
	fontScale = 1.45f;
	fontHeight = font->GetCharHeight('@', fontScale) + 1;

	// TODO: SCROLL ME + ADD ALL CREDITS All contributors
	
	font->BlitTextColor("Atari800 emulator version 5.0.0", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	font->BlitTextColor("Current active members of the Atari800 development team:", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	font->BlitTextColor("--------------------------------------------------------", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	py += fontHeight;

	font->BlitTextColor("Petr Stehlik        (maintainer)", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	font->BlitTextColor("Perry McFarlane     (core developer)", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	font->BlitTextColor("Piotr Fusik         (core developer)", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	font->BlitTextColor("Tomasz Krasuski     (core developer)", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	font->BlitTextColor("Mark Grebe          (Mac OSX)", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	font->BlitTextColor("Kostas Nakos        (Windows CE, Android)", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	font->BlitTextColor("James Wilkinson     (DOS, BeOS, Win32)", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	font->BlitTextColor("Christian Groessler (Sega Dreamcast)", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	font->BlitTextColor("Andrey Dj           (Raspberry Pi)", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	font->BlitTextColor("Miro Kropacek       (Atari Falcon)", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	
	//	font->BlitTextColor("", px, py, posZ, fontScale, tr, tg, tb, 1);
	py += fontHeight;
	
	fontScale = 0.8f;
//	font->BlitTextColor("The ROM files embedded in the source code are Copyright C by Atari.", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	
	fontHeight = font->GetCharHeight('@', fontScale) + 1;
	
	font->BlitTextColor("This program is free software; you can redistribute it and/or", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	font->BlitTextColor("modify it under the terms of the GNU General Public License as", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	font->BlitTextColor("published by the Free Software Foundation; either version 2 of the", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	font->BlitTextColor("License, or (at your option) any later version.", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	font->BlitTextColor("", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	font->BlitTextColor("This program is distributed in the hope that it will be useful,", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	font->BlitTextColor("but WITHOUT ANY WARRANTY; without even the implied warranty of", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	font->BlitTextColor("MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	font->BlitTextColor("GNU General Public License for more details.", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	font->BlitTextColor("You should have received a copy of the GNU General Public License", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	font->BlitTextColor("along with this program; if not, write to the Free Software", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	
	font->BlitTextColor("Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA", px, py, posZ, fontScale, tr, tg, tb, 1); py += fontHeight;
	
	font->BlitTextColor("02111-1307  USA", px, py, posZ, fontScale, tr, tg, tb, 1);
	
	py = this->sizeY - 30.0f;
	font->BlitTextColor("Click anywhere to close this", 330, py, posZ, 2.0f, tr, tg, tb, 1);
	
}

void CViewAbout::Render(float posX, float posY)
{
	CGuiView::Render(posX, posY);
}

//@returns is consumed
bool CViewAbout::DoTap(float x, float y)
{
	LOGG("CViewAbout::DoTap:  x=%f y=%f", x, y);
	guiMain->SetView(viewC64->mainMenuHelper);
	return CGuiView::DoTap(x, y);
}

bool CViewAbout::DoFinishTap(float x, float y)
{
	LOGG("CViewAbout::DoFinishTap: %f %f", x, y);
	return CGuiView::DoFinishTap(x, y);
}

//@returns is consumed
bool CViewAbout::DoDoubleTap(float x, float y)
{
	LOGG("CViewAbout::DoDoubleTap:  x=%f y=%f", x, y);
	return CGuiView::DoDoubleTap(x, y);
}

bool CViewAbout::DoFinishDoubleTap(float x, float y)
{
	LOGG("CViewAbout::DoFinishTap: %f %f", x, y);
	return CGuiView::DoFinishDoubleTap(x, y);
}


bool CViewAbout::DoMove(float x, float y, float distX, float distY, float diffX, float diffY)
{
	return CGuiView::DoMove(x, y, distX, distY, diffX, diffY);
}

bool CViewAbout::FinishMove(float x, float y, float distX, float distY, float accelerationX, float accelerationY)
{
	return CGuiView::FinishMove(x, y, distX, distY, accelerationX, accelerationY);
}

bool CViewAbout::InitZoom()
{
	return CGuiView::InitZoom();
}

bool CViewAbout::DoZoomBy(float x, float y, float zoomValue, float difference)
{
	return CGuiView::DoZoomBy(x, y, zoomValue, difference);
}

bool CViewAbout::DoMultiTap(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiTap(touch, x, y);
}

bool CViewAbout::DoMultiMove(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiMove(touch, x, y);
}

bool CViewAbout::DoMultiFinishTap(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiFinishTap(touch, x, y);
}

void CViewAbout::SwitchAboutScreen()
{
	if (guiMain->currentView == this)
	{
		guiMain->SetView(viewC64->mainMenuHelper);
	}
	else
	{
		guiMain->SetView(this);
	}
}

bool CViewAbout::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	guiMain->SetView(viewC64->mainMenuHelper);

//	if (keyCode == MTKEY_BACKSPACE)
//	{
//		guiMain->SetView(viewC64->viewC64MainMenu);
//		return true;
//	}
	
	return CGuiView::KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewAbout::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return CGuiView::KeyUp(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewAbout::KeyPressed(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return CGuiView::KeyPressed(keyCode, isShift, isAlt, isControl, isSuper);
}

void CViewAbout::ActivateView()
{
	LOGG("CViewAbout::ActivateView()");
}

void CViewAbout::DeactivateView()
{
	LOGG("CViewAbout::DeactivateView()");
}


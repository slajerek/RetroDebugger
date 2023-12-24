#include "CViewC64.h"
#include "CViewC64GoatTracker.h"
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
#include "VID_ImageBinding.h"
#include "CViewMemoryMap.h"
#include "CGuiMain.h"
#include "CGuiEvent.h"

extern "C" {
#include "bme_cfg.h"
#include "bme_main.h"
#include "gconsole.h"
#include "gcommon.h"
#include "gplay.h"
#include "gpattern.h"
#include "bme_win.h"

unsigned int mapSdlKeyToBmeKey(unsigned int sdlKey);
}

CViewC64GoatTracker::CViewC64GoatTracker(float posX, float posY, float posZ, float sizeX, float sizeY)
: CGuiView(posX, posY, posZ, sizeX, sizeY)
{
	this->name = "Goat Tracker plugin";
	
	mutex = new CSlrMutex("CViewC64GoatTracker");
	
	font = viewC64->fontCBMShifted;
	fontScale = 1.5;
	fontHeight = font->GetCharHeight('@', fontScale) + 2;
	
	// TODO: create CSlrImage(width, height, type) that allocs imagedata with raster^2 and does not delete it after binding so can be reused
	imageDataScreen = new CImageData(MAX_COLUMNS*8, MAX_ROWS*16, IMG_TYPE_RGBA);
	imageDataScreen->AllocImage(false, true);
		
	imageScreen = new CSlrImage(true, false);
	imageScreen->LoadImage(imageDataScreen, RESOURCE_PRIORITY_STATIC, false);
	imageScreen->resourceType = RESOURCE_TYPE_IMAGE_DYNAMIC;
	VID_PostImageBinding(imageScreen, NULL);
}

CViewC64GoatTracker::~CViewC64GoatTracker()
{
}

void CViewC64GoatTracker::DoLogic()
{
	
}

void CViewC64GoatTracker::Render()
{
}

void CViewC64GoatTracker::RenderImGui()
{
//	LOGD("CViewC64GoatTracker::RenderImGui");
	
	float w = (float)imageDataScreen->width;
	float h = (float)imageDataScreen->height;

	this->imGuiWindowAspectRatio = w/h;
	this->imGuiWindowKeepAspectRatio = true;

	PreRenderImGui();

//		for (int x = 0; x < imageDataScreen->width; x++)
//		{
//			for (int y = 0; y < imageDataScreen->height; y++)
//			{
//				float wx = ((float)x / (float)imageDataScreen->width) * 255.0f;
//				float wy = ((float)y / (float)imageDataScreen->height) * 255.0f;
//
//				imageDataScreen->SetPixelResultRGBA(x, y, (int)wx, (int)wy, 0, 255);
//			}
//		}

	imageScreen->ReBindImageData(imageDataScreen);

	// blit texture of the screen
	Blit(imageScreen,
		 posX,
		 posY, -1,
		 sizeX,
		 sizeY);
	//, 0, 0, 1, 1); //,
//		 0.0f, 0.0f, screenTexEndX, screenTexEndY);
	
	
//	ImGui::Text("Test");
	
	PostRenderImGui();
	
//	LOGD("CViewC64GoatTracker::RenderImGui done");
}

void CViewC64GoatTracker::Render(float posX, float posY)
{
	CGuiView::Render(posX, posY);
}

//@returns is consumed
bool CViewC64GoatTracker::DoTap(float x, float y)
{
	LOGG("CViewC64GoatTracker::DoTap:  x=%f y=%f", x, y);
	
	float xp = ((x - posX) / sizeX) * (float)(MAX_COLUMNS*8);
	float yp = ((y - posY) / sizeY) * (float)(MAX_ROWS*16);

	unsigned int xi = (unsigned int)xp;
	unsigned int yi = (unsigned int)yp;
	LOGD("xp=%f yp=%f xi=%d yi=%d", xp, yp, xi, yi);

	CGuiEventMouse *eventMouse = new CGuiEventMouse(GUI_EVENT_MOUSE_LEFT_BUTTON_DOWN, xi, yi);
	AddEvent(eventMouse);

	return true; //CGuiView::DoTap(x, y);
}

bool CViewC64GoatTracker::DoFinishTap(float x, float y)
{
	LOGG("CViewC64GoatTracker::DoFinishTap: %f %f", x, y);

	float xp = ((x - posX) / sizeX) * (float)(MAX_COLUMNS*8);
	float yp = ((y - posY) / sizeY) * (float)(MAX_ROWS*16);

	unsigned int xi = (unsigned int)xp;
	unsigned int yi = (unsigned int)yp;
	LOGD("xp=%f yp=%f xi=%d yi=%d", xp, yp, xi, yi);

	CGuiEventMouse *eventMouse = new CGuiEventMouse(GUI_EVENT_MOUSE_LEFT_BUTTON_UP, xi, yi);
	AddEvent(eventMouse);

	return true; //CGuiView::DoFinishTap(x, y);
}

//@returns is consumed
bool CViewC64GoatTracker::DoDoubleTap(float x, float y)
{
	LOGG("CViewC64GoatTracker::DoDoubleTap:  x=%f y=%f", x, y);
	return CGuiView::DoDoubleTap(x, y);
}

bool CViewC64GoatTracker::DoFinishDoubleTap(float x, float y)
{
	LOGG("CViewC64GoatTracker::DoFinishTap: %f %f", x, y);
	return CGuiView::DoFinishDoubleTap(x, y);
}

bool CViewC64GoatTracker::DoMove(float x, float y, float distX, float distY, float diffX, float diffY)
{
	float xp = ((x - posX) / sizeX) * (float)(MAX_COLUMNS*8);
	float yp = ((y - posY) / sizeY) * (float)(MAX_ROWS*16);

	unsigned int xi = (unsigned int)xp;
	unsigned int yi = (unsigned int)yp;
	LOGD("xp=%f yp=%f xi=%d yi=%d", xp, yp, xi, yi);

	CGuiEventMouse *eventMouse = new CGuiEventMouse(GUI_EVENT_MOUSE_MOVE, xi, yi);
	AddEvent(eventMouse);

	return CGuiView::DoMove(x, y, distX, distY, diffX, diffY);
}

bool CViewC64GoatTracker::FinishMove(float x, float y, float distX, float distY, float accelerationX, float accelerationY)
{
	return CGuiView::FinishMove(x, y, distX, distY, accelerationX, accelerationY);
}

bool CViewC64GoatTracker::DoNotTouchedMove(float x, float y)
{
	float xp = ((x - posX) / sizeX) * (float)(MAX_COLUMNS*8);
	float yp = ((y - posY) / sizeY) * (float)(MAX_ROWS*16);

	unsigned int xi = (unsigned int)xp;
	unsigned int yi = (unsigned int)yp;
	LOGD("xp=%f yp=%f xi=%d yi=%d", xp, yp, xi, yi);

	CGuiEventMouse *eventMouse = new CGuiEventMouse(GUI_EVENT_MOUSE_MOVE, xi, yi);
	AddEvent(eventMouse);

	return CGuiView::DoNotTouchedMove(x, y);
}

bool CViewC64GoatTracker::InitZoom()
{
	return CGuiView::InitZoom();
}

bool CViewC64GoatTracker::DoZoomBy(float x, float y, float zoomValue, float difference)
{
	return CGuiView::DoZoomBy(x, y, zoomValue, difference);
}

bool CViewC64GoatTracker::DoMultiTap(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiTap(touch, x, y);
}

bool CViewC64GoatTracker::DoMultiMove(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiMove(touch, x, y);
}

bool CViewC64GoatTracker::DoMultiFinishTap(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiFinishTap(touch, x, y);
}

bool CViewC64GoatTracker::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	LOGD("CViewC64GoatTracker::KeyDown: keyCode=%d");
	CGuiEventKeyboard *eventKeyboard = new CGuiEventKeyboard(GUI_EVENT_KEYBOARD_KEY_DOWN, keyCode, keyCode);
	AddEvent(eventKeyboard);

	return true;
}

bool CViewC64GoatTracker::KeyDownRepeat(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewC64GoatTracker::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	LOGD("CViewC64GoatTracker::KeyUp: keyCode=%d", keyCode);
	CGuiEventKeyboard *eventKeyboard = new CGuiEventKeyboard(GUI_EVENT_KEYBOARD_KEY_UP, keyCode, keyCode);
	AddEvent(eventKeyboard);

	return true;
}

bool CViewC64GoatTracker::KeyPressed(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return CGuiView::KeyPressed(keyCode, isShift, isAlt, isControl, isSuper);
}

void CViewC64GoatTracker::AddEvent(CGuiEvent *event)
{
	mutex->Lock();
	events.push_back(event);
	mutex->Unlock();
}

void CViewC64GoatTracker::ForwardEvents()
{
//	LOGD("CViewC64GoatTracker::ForwardEvents");
	// transfer accumulated events to gt2
	mutex->Lock();
	
	while(!events.empty())
	{
		CGuiEvent *event = events.front();
		events.pop_front();
		
		LOGD("event type=%d", event->type);
		if (event->type == GUI_EVENT_TYPE_MOUSE)
		{
			CGuiEventMouse *eventMouse = (CGuiEventMouse *)event;
			
			LOGD("CGuiEventMouse mouseState=%d x=%d y=%d", eventMouse->mouseState, eventMouse->x, eventMouse->y);
			
			gt2SetMousePosition(eventMouse->x, eventMouse->y);

			if (eventMouse->mouseState == GUI_EVENT_MOUSE_MOVE)
			{
			}
			else if (eventMouse->mouseState == GUI_EVENT_MOUSE_LEFT_BUTTON_DOWN)
			{
				LOGD("GUI_EVENT_MOUSE_LEFT_BUTTON_DOWN");
				win_mousebuttons |= MOUSEB_LEFT;
				
				// that's all in this loop, as we may consume button up in next iteration
				break;
			}
			else if (eventMouse->mouseState == GUI_EVENT_MOUSE_LEFT_BUTTON_UP)
			{
				LOGD("GUI_EVENT_MOUSE_LEFT_BUTTON_UP");
				win_mousebuttons &= ~MOUSEB_LEFT;
			}
			
			LOGD("eventMouse->x=%d eventMouse->y=%d win_mousexpos=%d win_mouseypos=%d",
				 eventMouse->x, eventMouse->y, win_mousexpos, win_mouseypos);
		}
		else if (event->type == GUI_EVENT_TYPE_KEYBOARD)
		{
			CGuiEventKeyboard *eventKeyboard = (CGuiEventKeyboard *)event;
			
			u32 bmeKey = mapSdlKeyToBmeKey(eventKeyboard->mtKey);
			
			if (eventKeyboard->keyboardState == GUI_EVENT_KEYBOARD_KEY_DOWN)
			{
				if (eventKeyboard->mtKey > 0xFFFF)
				{
					win_asciikey = (eventKeyboard->mtKey & 0xFF) | 0x80;
				}
				else
				{
					win_asciikey = eventKeyboard->mtKey; //event.key.keysym.unicode;
				}
				virtualkeycode = win_asciikey;

				u32 keynum = bmeKey; //event.key.keysym.sym;

				LOGD("ForwardEvents: eventKeyboard->mtKey=0x%x win_asciikey=0x%x bmeKey=%d", eventKeyboard->mtKey, win_asciikey, bmeKey);

				if (keynum < MAX_KEYS)
				{
					win_keytable[keynum] = 1;
					win_keystate[keynum] = 1;
//					if ((keynum == KEY_ENTER) && ((win_keystate[KEY_ALT]) || (win_keystate[KEY_RIGHTALT])))
//					{
//						win_fullscreen ^= 1;
//						gfx_reinit();
//					}
				}
				
				// that's all in this loop as we may consume key up in next iteration
				break;
			}
			else if (eventKeyboard->keyboardState == GUI_EVENT_KEYBOARD_KEY_UP)
			{
				u32 keynum = bmeKey; //eventKeyboard->mtKey; //event.key.keysym.sym;
				if (keynum < MAX_KEYS)
				{
					win_keytable[keynum] = 0;
					win_keystate[keynum] = 0;
				}
			}
		}
	
		/*
		 
		 enum GuiEventKeyboardState : u8
		 {
			 GUI_EVENT_KEYBOARD_KEY_DOWN,
			 GUI_EVENT_KEYBOARD_KEY_UP,
			 GUI_EVENT_KEYBOARD_KEY_REPEAT
		 };

		 enum GuiEventMouseState : u8
		 {
			 GUI_EVENT_MOUSE_LEFT_BUTTON_DOWN,
			 GUI_EVENT_MOUSE_LEFT_BUTTON_UP,
			 GUI_EVENT_MOUSE_RIGHT_BUTTON_DOWN,
			 GUI_EVENT_MOUSE_RIGHT_BUTTON_UP,
			 GUI_EVENT_MOUSE_MID_BUTTON_DOWN,
			 GUI_EVENT_MOUSE_MID_BUTTON_UP,
			 
	case SDL_JOYBUTTONDOWN:
	joybuttons[event.jbutton.which] |= 1 << event.jbutton.button;
	break;

	case SDL_JOYBUTTONUP:
	joybuttons[event.jbutton.which] &= ~(1 << event.jbutton.button);
	break;

	case SDL_JOYAXISMOTION:
	switch (event.jaxis.axis)
	{
		case 0:
		joyx[event.jaxis.which] = event.jaxis.value;
		break;

		case 1:
		joyy[event.jaxis.which] = event.jaxis.value;
		break;
	}
	break;


	case SDL_MOUSEBUTTONDOWN:
	switch(event.button.button)
	{
		case SDL_BUTTON_MIDDLE:
		win_mousebuttons |= MOUSEB_MIDDLE;
		break;

		case SDL_BUTTON_RIGHT:
		win_mousebuttons |= MOUSEB_RIGHT;
		break;
	}
	break;

	case SDL_MOUSEBUTTONUP:
	switch(event.button.button)
	{
		case SDL_BUTTON_MIDDLE:
		win_mousebuttons &= ~MOUSEB_MIDDLE;
		break;

		case SDL_BUTTON_RIGHT:
		win_mousebuttons &= ~MOUSEB_RIGHT;
		break;
	}
	break;

	case SDL_QUIT:
	win_quitted = 1;
	break;

	case SDL_KEYDOWN:
   // win_virtualkey = event.key.keysym.sym;
	win_asciikey = event.key.keysym.unicode;
	keynum = event.key.keysym.sym;
	if (keynum < MAX_KEYS)
	{
		win_keytable[keynum] = 1;
		win_keystate[keynum] = 1;
		if ((keynum == KEY_ENTER) && ((win_keystate[KEY_ALT]) || (win_keystate[KEY_RIGHTALT])))
		{
			win_fullscreen ^= 1;
			gfx_reinit();
		}
	}
	break;

	case SDL_KEYUP:
	keynum = event.key.keysym.sym;
	if (keynum < MAX_KEYS)
	{
		win_keytable[keynum] = 0;
		win_keystate[keynum] = 0;
	}
	break;

	case SDL_VIDEORESIZE:
	case SDL_VIDEOEXPOSE:
	gfx_redraw = 1;
	break;
				 
		 */
	
	
		delete event;
	}

	mutex->Unlock();
}

void CViewC64GoatTracker::ActivateView()
{
	LOGG("CViewC64GoatTracker::ActivateView()");
}

void CViewC64GoatTracker::DeactivateView()
{
	LOGG("CViewC64GoatTracker::DeactivateView()");
}


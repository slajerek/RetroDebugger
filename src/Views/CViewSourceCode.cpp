#include "CViewSourceCode.h"
#include "CColorsTheme.h"
#include "VID_Main.h"
#include "CGuiMain.h"
#include "CDataAdapter.h"
#include "CViewC64.h"
#include "CViewMemoryMap.h"
#include "SYS_KeyCodes.h"
#include "CViewC64Screen.h"
#include "CDebugInterface.h"
#include "CGuiEditBoxText.h"
#include "C64Tools.h"
#include "CSlrString.h"
#include "CSlrKeyboardShortcuts.h"
#include "C64KeyboardShortcuts.h"
#include "C64SettingsStorage.h"
#include "CViewC64VicDisplay.h"
#include "CViewDataDump.h"
#include "CViewDisassembly.h"
#include "CDebugSymbols.h"
#include "CDebugMemory.h"
#include "CDebugAsmSource.h"
#include "CDebugSymbolsSegment.h"

CViewSourceCode::CViewSourceCode(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY,
								 CDebugInterface *debugInterface, CDebugSymbols *debugSymbols, 
								 CViewDisassembly *viewDisassembly)
: CGuiView(name, posX, posY, posZ, sizeX, sizeY)
{
	imGuiNoWindowPadding = true;
	imGuiNoScrollbar = true;

	this->viewDisassembly = viewDisassembly;
	this->debugSymbols = debugSymbols;
	this->dataAdapter = debugSymbols->dataAdapter;
	this->memoryLength = dataAdapter->AdapterGetDataLength();
	this->memory = new uint8[memoryLength];
	
	this->debugInterface = debugInterface;

	this->font = NULL;
	
	this->changedByUser = false;
	this->cursorAddress = -1;
	this->currentPC = -1;
	
	this->showFilePath = true;
	this->showLineNumbers = true;
	
//	// keyboard shortcut zones for this view
//	shortcutZones.push_back(KBZONE_DISASSEMBLY);
//	shortcutZones.push_back(KBZONE_MEMORY);

	/// debug

	// render execute-aware version of disassembly?
	//c64SettingsRenderDisassemblyExecuteAware = true;
	
//	this->AddCodeLabel(0xE5D1, "labE5D1x:");
//	this->AddCodeLabel(0xE5D3, "labE5D3x:");
//	this->AddCodeLabel(0xFD6E, "labFD6Ex:");
//	this->AddCodeLabel(0x1000, "lab1000x:");
}

CViewSourceCode::~CViewSourceCode()
{
}


void CViewSourceCode::ScrollToAddress(int addr)
{
//	LOGD("CViewSourceCode::ScrollToAddress=%04x", addr);

	this->cursorAddress = addr;
}

void CViewSourceCode::SetViewParameters(float posX, float posY, float posZ, float sizeX, float sizeY, CSlrFont *font,
										 float fontSize)
{
	CGuiView::SetPosition(posX, posY, posZ, sizeX, sizeY);
	
	this->font = font;
	this->fontSize = fontSize;

}

void CViewSourceCode::UpdateLocalMemoryCopy(int startAddress, int endAddress)
{
	int beginAddress = startAddress; // - numberOfLinesBack3;

	//LOGD("UpdateLocalMemoryCopy: %04x %04x (size=%04x)", beginAddress, endAddress, endAddress-beginAddress);

	if (beginAddress < 0)
	{
		beginAddress = 0;
	}
	
	dataAdapter->AdapterReadBlockDirect(memory, beginAddress, endAddress);
}

void CViewSourceCode::DoLogic()
{
}

void CViewSourceCode::RenderImGui()
{
	PreRenderImGui();
	Render();
	PostRenderImGui();
}


void CViewSourceCode::Render()
{
	this->font = viewDisassembly->fontDisassembly;
	this->fontSize = viewDisassembly->fontSize;
	
//	this->font->BlitText("CViewSourceCode", posX, posY, -1, fontSize);
	
	float pe = this->posEndY;
	
	if (showFilePath)
	{
		pe -= fontSize;
	}

	
//	if (debugInterface->GetSettingIsWarpSpeed() == true)
//		return;
	
//	this->renderBreakpointsMutex->Lock();
	
	
	float colorExecuteR, colorExecuteG, colorExecuteB;
	float colorExecuteA = 1.0f;
//	float colorNonExecuteR, colorNonExecuteG, colorNonExecuteB;
//	float colorNonExecuteA = 1.0f;
	
	GetColorsFromScheme(c64SettingsDisassemblyExecuteColor, &colorExecuteR, &colorExecuteG, &colorExecuteB);
//	GetColorsFromScheme(c64SettingsDisassemblyNonExecuteColor, &colorNonExecuteR, &colorNonExecuteG, &colorNonExecuteB);

	float cr = colorExecuteR;
	float cg = colorExecuteG;
	float cb = colorExecuteB;

	
	float colorBackgroundR, colorBackgroundG, colorBackgroundB;
	float colorBackgroundA = 1.0f;
	
	GetColorsFromScheme(c64SettingsDisassemblyBackgroundColor, &colorBackgroundR, &colorBackgroundG, &colorBackgroundB);

//	BlitFilledRectangle(this->posX, this->posY, -1, this->sizeX, this->sizeY, colorBackgroundR, colorBackgroundG, colorBackgroundB, colorBackgroundA);
//
//	//
//	if (showFilePath)
//	{
//		BlitFilledRectangle(posX, pe + 1.0f, -1.0f,
//							sizeX, fontSize, 0.7, 0.7, 1, 0.5f);
//
//	}
//
	CDebugAsmSource *asmSource = debugInterface->symbols->asmSource;
	
	if (asmSource == NULL)
		return;
	
	if (debugInterface->symbols->segments.empty())
		return;

//	fontSize = 5.0f;
	
	float px = this->posX;
	float py = this->posY + floor(this->sizeY / 2.0f) - fontSize + 0.5f;
	
	float sx = this->sizeX;
	float nx = this->posEndX;
	
	if (showLineNumbers)
	{
		sx -= fontSize*4;
		nx -= fontSize*4;
	}
	
	char buf[16];
	
	CDebugAsmSourceLine *sourceLine = NULL;
	
	CDebugSymbolsSegment *sourceSegment = debugInterface->symbols->currentSegment;
	
	if (sourceSegment == NULL)
		return;
	
//	debugInterface->symbols->currentSegment->name->DebugPrint("sourceSegment=");
	
	// blit segment name
	// TODO: make this in-line with UI, this is UI workaround
	const float segmentFontSize = 6.0f;
	font->BlitTextColor(sourceSegment->name, posX, posY, -1, segmentFontSize, 1, 1, 1, 1);
	
	this->cursorAddress = viewDisassembly->cursorAddress;

	if (this->cursorAddress >= 0 && this->cursorAddress < asmSource->maxMemoryAddress)
	{
		sourceLine = sourceSegment->codeSourceLineByMemoryAddress[cursorAddress];
	}
	
//	LOGD("segment=%x cursorAddress=%x sourceLine=%x", sourceSegment, cursorAddress, sourceLine);
	
	if (sourceLine != NULL && sourceLine->codeFile != NULL)
	{
		font->BlitTextColor(sourceLine->block->name, posX, posY + segmentFontSize, -1, segmentFontSize, 1, 1, 1, 1);

		VID_SetClipping(this->posX, this->posY, sx, this->sizeY);

		int lineNum = sourceLine->codeLineNumberStart;

		if (sourceLine->codeFile->codeTextByLineNum.size() < lineNum)
		{
			// file not loaded properly?
			//LOGError("lineNum exceeds codeTextByLineNum");
			return;
		}

		std::vector<CSlrString *>::iterator itDown = sourceLine->codeFile->codeTextByLineNum.begin();
		std::advance(itDown, lineNum);

		std::vector<CSlrString *>::iterator itUp = itDown;
		
		float pyb = py;
		
		for ( ; itDown != sourceLine->codeFile->codeTextByLineNum.end(); itDown++)
		{
			CSlrString *str = *itDown;
			
			font->BlitTextColor(str, px, py, -1, fontSize, 1, 1, 1, 1);

			py += fontSize;
			lineNum++;
			
			if (py > pe)
				break;
		}
		
		py = pyb - fontSize;
		itUp--;
		lineNum = sourceLine->codeLineNumberStart-1;

		for ( ; itUp != sourceLine->codeFile->codeTextByLineNum.begin(); itUp--)
		{
			CSlrString *str = *itUp;
			
			font->BlitTextColor(str, px, py, -1, fontSize, 1, 1, 1, 1);
			
			py -= fontSize;
			lineNum--;
			
			if (py < this->posY)
				break;
		}

		VID_ResetClipping();

		if (showLineNumbers)
		{
			py = pyb;
			lineNum = sourceLine->codeLineNumberStart-1;

			itDown = sourceLine->codeFile->codeTextByLineNum.begin();
			std::advance(itDown, lineNum);

			itUp = itDown;
			
			lineNum = sourceLine->codeLineNumberStart;
			
			for ( ; itDown != sourceLine->codeFile->codeTextByLineNum.end(); itDown++)
			{
				sprintf(buf, "%4d", lineNum);
				font->BlitTextColor(buf, nx, py, -1, fontSize, 0.7, 0.7, 0.7, 1);
				
				py += fontSize;
				lineNum++;
				
				if (py > pe)
					break;
			}
			
			py = pyb - fontSize;
			itUp--;
			lineNum = sourceLine->codeLineNumberStart-1;
			
			for ( ; itUp != sourceLine->codeFile->codeTextByLineNum.begin(); itUp--)
			{
				sprintf(buf, "%4d", lineNum);
				font->BlitTextColor(buf, nx, py, -1, fontSize, 0.7, 0.7, 0.7, 1);
				
				py -= fontSize;
				lineNum--;
				
				if (py < this->posY)
					break;
			}
		}

		if (showFilePath)
		{
			font->BlitTextColor(sourceLine->codeFile->sourceFileName, px, pe+1.5f, -1, fontSize, 0.9, 0.7, 0.7, 1);
		}
		
		// show cursor
		BlitRectangle(px, pyb, -1.0f, sizeX, fontSize, 0.3, 1.0, 0.3, 0.5f, 0.7f);
	}
	
	
	//	this->renderBreakpointsMutex->Unlock();

}

void CViewSourceCode::Render(float posX, float posY)
{
	this->Render();
}

bool CViewSourceCode::IsInside(float x, float y)
{
	const float segmentFontSize = 6.0f;
	if (x > posX && x < posX + (segmentFontSize*18)
		&& y > 0 && y < (segmentFontSize * 3.0f))
	{
		return true;
	}
	
	return CGuiView::IsInside(x, y);
}

//@returns is consumed
bool CViewSourceCode::DoTap(float x, float y)
{
	LOGG("CViewSourceCode::DoTap:  x=%f y=%f", x, y);
	
	if (this->visible == false)
		return false;
	
	const float segmentFontSize = 6.0f;
	if (x > posX && x < posX + (segmentFontSize*18)
		&& y > 0 && y < (segmentFontSize * 3.0f))
	{
		// tapped on Segment, switch Segment to next
		if (debugInterface->symbols)
		{
			debugInterface->symbols->SelectNextSegment();
		}
		return true;
	}

	return true;
}

bool CViewSourceCode::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return false;
	LOGI("CViewSourceCode::KeyDown: %x %s %s %s", keyCode, STRBOOL(isShift), STRBOOL(isAlt), STRBOOL(isControl));

	return CGuiView::KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
}

void CViewSourceCode::SetCursorToNearExecuteCodeAddress(int newCursorAddress)
{
//	isTrackingPC = false;
//	
	for (int addr = newCursorAddress; addr > newCursorAddress-3; addr--)
	{
		if (debugSymbols->memory->IsExecuteCodeAddress(addr))
		{
			cursorAddress = addr;
			return;
		}
	}
	
	for (int addr = newCursorAddress; addr < newCursorAddress+3; addr++)
	{
		if (debugSymbols->memory->IsExecuteCodeAddress(addr))
		{
			cursorAddress = addr;
			return;
		}
	}
	
	cursorAddress = newCursorAddress;
}


bool CViewSourceCode::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return CGuiView::KeyUp(keyCode, isShift, isAlt, isControl, isSuper);
}

void CViewSourceCode::ScrollDown()
{
//	isTrackingPC = false;
	changedByUser = true;

}

void CViewSourceCode::ScrollUp()
{
//	isTrackingPC = false;
	changedByUser = true;
	
}

bool CViewSourceCode::DoScrollWheel(float deltaX, float deltaY)
{
	//LOGD("CViewSourceCode::DoScrollWheel: %f %f", deltaX, deltaY);
	int dy = fabs(round(deltaY));
	
	bool scrollUp = (deltaY > 0);
	
	for (int i = 0; i < dy; i++)
	{
		if (scrollUp)
		{
			ScrollUp();
		}
		else
		{
			ScrollDown();
		}
	}
	return true;
}

void CViewSourceCode::RenderFocusBorder()
{
	// do not render border
	
}

// Layout
void CViewSourceCode::Serialize(CByteBuffer *byteBuffer)
{
}

void CViewSourceCode::Deserialize(CByteBuffer *byteBuffer)
{
}



//
// NOTE:
// This view will be removed. It is being refactored and moved to new breakpoints view
// Stays in the project only as a reference for new implementation
//

#include "CViewC64.h"
#include "CColorsTheme.h"
#include "CViewBreakpointsOLD.h"
#include "CViewDisassembly.h"
#include "VID_Main.h"
#include "CGuiMain.h"
#include "CSlrString.h"
#include "C64Tools.h"
#include "SYS_KeyCodes.h"
#include "CDebugInterfaceC64.h"


// this is very bad, bad code. written being drunk as POC. lots of copypaste. refactor this into some meaningful.


enum breakpointsCursorGroups
{
	CURSOR_GROUP_C64_IRQVIC = 1,
	CURSOR_GROUP_C64_IRQCIA,
	CURSOR_GROUP_C64_IRQNMI,
	CURSOR_GROUP_C64_ENABLE_PC,
	CURSOR_GROUP_C64_ADDR_PC,
	CURSOR_GROUP_C64_ENABLE_MEMORY,
	CURSOR_GROUP_C64_MEMORY,
	CURSOR_GROUP_C64_ENABLE_RASTER,
	CURSOR_GROUP_C64_RASTER,
	
	CURSOR_GROUP_DRIVE1541_IRQVIA1,
	CURSOR_GROUP_DRIVE1541_IRQVIA2,
	CURSOR_GROUP_DRIVE1541_IRQIEC,
	CURSOR_GROUP_DRIVE1541_ENABLE_PC,
	CURSOR_GROUP_DRIVE1541_ADDR_PC,
	CURSOR_GROUP_DRIVE1541_ENABLE_MEMORY,
	CURSOR_GROUP_DRIVE1541_MEMORY
	
};

//// TODO: change into TextHexEdits
//// TODO: change into CButtonGroup

CViewBreakpointsOLD::CViewBreakpointsOLD(float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterface *debugInterface)
: CGuiView(posX, posY, posZ, sizeX, sizeY)
{
	this->name = "CViewBreakpoints";
	
	prevView = viewC64;
	
	this->debugInterface = debugInterface;
	
	
	font = viewC64->fontCBMShifted;
	fontScale = 2;
	fontWidth = font->GetCharWidth('@', fontScale);
	fontHeight = font->GetCharHeight('@', fontScale) + 2;

	fontNumbersScale = 1.5f;
	fontNumbersWidth = font->GetCharWidth('@', fontNumbersScale);
	fontNumbersHeight = font->GetCharHeight('@', fontNumbersScale) + 2;

	
	strHeader = new CSlrString("Breakpoints");

	tr = viewC64->colorsTheme->colorTextR;
	tg = viewC64->colorsTheme->colorTextG;
	tb = viewC64->colorsTheme->colorTextB;
	
	float px = 22.0f;
	float py = 32.0f;
	
//	lblPlatform =	new CGuiLabel(debugInterface->GetPlatformNameString(), px, py, posZ, 120, fontHeight, LABEL_ALIGNED_LEFT, font, fontNumbersScale,
//									  0.0f, 0.0f, 0.0f, 0.0f,
//									  tr, tg, tb, 1.0f,
//									  0.0f, 0.0f, NULL);
//	lblPlatform->image = NULL;
//	this->AddGuiElement(lblPlatform);

	float startX = 30;
	float startY = 50;
	
	float buttonSizeX = 73.0f;
	float buttonSizeY = fontHeight + 4;

	// TODO: make this generic / abstract
	if (debugInterface->GetEmulatorType() == EMULATOR_TYPE_C64_VICE)
	{
		px = sizeX - 142;
		lbl1541Drive =	new CGuiLabel(new CSlrString("1541 Drive"), px, py, posZ, 120, fontHeight, LABEL_ALIGNED_RIGHT, font, fontNumbersScale,
									  0.0f, 0.0f, 0.0f, 0.0f,
									  tr, tg, tb, 1.0f,
									  0.0f, 0.0f, NULL);
		lbl1541Drive->image = NULL;
		this->AddGuiElement(lbl1541Drive);

		/// left side

		px = startX;
		py = startY;

		btnBreakpointC64IrqVIC = new CGuiButtonSwitch(NULL, NULL, NULL,
													  px, py, posZ, buttonSizeX, buttonSizeY,
													  new CSlrString("  VIC  "),
													  FONT_ALIGN_CENTER, buttonSizeX/2, 3.5,
													  font, fontScale,
													  1.0, 1.0, 1.0, 1.0,
													  1.0, 1.0, 1.0, 1.0,
													  0.3, 0.3, 0.3, 1.0,
													  this);
		btnBreakpointC64IrqVIC->SetOn(false);
		this->AddGuiElement(btnBreakpointC64IrqVIC);
		
		px += buttonSizeX + 10;
		
		btnBreakpointC64IrqCIA = new CGuiButtonSwitch(NULL, NULL, NULL,
													  px, py, posZ, buttonSizeX, buttonSizeY,
													  new CSlrString("  CIA  "),
													  FONT_ALIGN_CENTER, buttonSizeX/2, 3.5,
													  font, fontScale,
													  1.0, 1.0, 1.0, 1.0,
													  1.0, 1.0, 1.0, 1.0,
													  0.3, 0.3, 0.3, 1.0,
													  this);
		btnBreakpointC64IrqCIA->SetOn(false);
		this->AddGuiElement(btnBreakpointC64IrqCIA);
		
		px += buttonSizeX + 10;
		
		btnBreakpointC64IrqNMI = new CGuiButtonSwitch(NULL, NULL, NULL,
													  px, py, posZ, buttonSizeX, buttonSizeY,
													  new CSlrString("  NMI  "),
													  FONT_ALIGN_CENTER, buttonSizeX/2, 3.5,
													  font, fontScale,
													  1.0, 1.0, 1.0, 1.0,
													  1.0, 1.0, 1.0, 1.0,
													  0.3, 0.3, 0.3, 1.0,
													  this);
		btnBreakpointC64IrqNMI->SetOn(false);
		this->AddGuiElement(btnBreakpointC64IrqNMI);
	}
	
	py += buttonSizeY + 5;
	
	px = startX;
	btnBreakpointsPC = new CGuiButtonSwitch(NULL, NULL, NULL,
											px, py, posZ, buttonSizeX, buttonSizeY,
											new CSlrString(" CPU PC "),
											FONT_ALIGN_CENTER, buttonSizeX/2, 3.5,
											font, fontScale,
											1.0, 1.0, 1.0, 1.0,
											1.0, 1.0, 1.0, 1.0,
											0.3, 0.3, 0.3, 1.0,
											this);
	btnBreakpointsPC->SetOn(false);
	this->AddGuiElement(btnBreakpointsPC);
	
	pcBreakpointsX = px;
	pcBreakpointsY = py + buttonSizeY + 5;
	
	py = pcBreakpointsY + fontNumbersHeight * 10 + 3;
	
	//
	
	px = startX;
	btnBreakpointsMemory = new CGuiButtonSwitch(NULL, NULL, NULL,
												px, py, posZ, buttonSizeX, buttonSizeY,
												new CSlrString(" MEMORY "),
												FONT_ALIGN_CENTER, buttonSizeX/2, 3.5,
												font, fontScale,
												1.0, 1.0, 1.0, 1.0,
												1.0, 1.0, 1.0, 1.0,
												0.3, 0.3, 0.3, 1.0,
												this);
	btnBreakpointsMemory->SetOn(false);
	this->AddGuiElement(btnBreakpointsMemory);
	
	memoryBreakpointsX = px - 8;
	memoryBreakpointsY = py + buttonSizeY + 5;
	
	py = memoryBreakpointsY + fontNumbersHeight * 10;
	
	//
	// TODO: make this generic / abstract
	if (debugInterface->GetEmulatorType() == EMULATOR_TYPE_C64_VICE)
	{
		px = startX;
		btnBreakpointsRaster = new CGuiButtonSwitch(NULL, NULL, NULL,
													px, py, posZ, buttonSizeX, buttonSizeY,
													new CSlrString(" RASTER "),
													FONT_ALIGN_CENTER, buttonSizeX/2, 3.5,
													font, fontScale,
													1.0, 1.0, 1.0, 1.0,
													1.0, 1.0, 1.0, 1.0,
													0.3, 0.3, 0.3, 1.0,
													this);
		btnBreakpointsRaster->SetOn(false);
		this->AddGuiElement(btnBreakpointsRaster);
		
		rasterBreakpointsX = px;
		rasterBreakpointsY = py + buttonSizeY + 5;
		
		py = rasterBreakpointsY + fontNumbersHeight * 6;
		
		//
		//
		// right side
		//
		//
		float sb = 20;
		float scrsx = sizeX - sb*2.0f;
		float cx = scrsx/2.0f + 11.0f + sb;
		
		float startX2 = cx;
		
		py = startY;
		px = startX2;
		
		
		/// left side
		
		btnBreakpointDrive1541IrqVIA1 = new CGuiButtonSwitch(NULL, NULL, NULL,
															 px, py, posZ, buttonSizeX, buttonSizeY,
															 new CSlrString("  VIA1  "),
															 FONT_ALIGN_CENTER, buttonSizeX/2, 3.5,
															 font, fontScale,
															 1.0, 1.0, 1.0, 1.0,
															 1.0, 1.0, 1.0, 1.0,
															 0.3, 0.3, 0.3, 1.0,
															 this);
		btnBreakpointDrive1541IrqVIA1->SetOn(false);
		this->AddGuiElement(btnBreakpointDrive1541IrqVIA1);
		
		px += buttonSizeX + 10;
		
		btnBreakpointDrive1541IrqVIA2 = new CGuiButtonSwitch(NULL, NULL, NULL,
															 px, py, posZ, buttonSizeX, buttonSizeY,
															 new CSlrString("  VIA2  "),
															 FONT_ALIGN_CENTER, buttonSizeX/2, 3.5,
															 font, fontScale,
															 1.0, 1.0, 1.0, 1.0,
															 1.0, 1.0, 1.0, 1.0,
															 0.3, 0.3, 0.3, 1.0,
															 this);
		btnBreakpointDrive1541IrqVIA2->SetOn(false);
		this->AddGuiElement(btnBreakpointDrive1541IrqVIA2);
		
		px += buttonSizeX + 10;
		
		btnBreakpointDrive1541IrqIEC = new CGuiButtonSwitch(NULL, NULL, NULL,
															px, py, posZ, buttonSizeX, buttonSizeY,
															new CSlrString("  IEC  "),
															FONT_ALIGN_CENTER, buttonSizeX/2, 3.5,
															font, fontScale,
															1.0, 1.0, 1.0, 1.0,
															1.0, 1.0, 1.0, 1.0,
															0.3, 0.3, 0.3, 1.0,
															this);
		btnBreakpointDrive1541IrqIEC->SetOn(false);
		this->AddGuiElement(btnBreakpointDrive1541IrqIEC);
		
		py += buttonSizeY + 5;
		
		px = startX2;
		btnBreakpointsDrive1541PC = new CGuiButtonSwitch(NULL, NULL, NULL,
														 px, py, posZ, buttonSizeX, buttonSizeY,
														 new CSlrString(" CPU PC "),
														 FONT_ALIGN_CENTER, buttonSizeX/2, 3.5,
														 font, fontScale,
														 1.0, 1.0, 1.0, 1.0,
														 1.0, 1.0, 1.0, 1.0,
														 0.3, 0.3, 0.3, 1.0,
														 this);
		btnBreakpointsDrive1541PC->SetOn(false);
		this->AddGuiElement(btnBreakpointsDrive1541PC);
		
		Drive1541PCBreakpointsX = px;
		Drive1541PCBreakpointsY = py + buttonSizeY + 5;
		
		py = Drive1541PCBreakpointsY + fontNumbersHeight * 10 + 3;
		
		//
		
		px = startX2;
		btnBreakpointsDrive1541Memory = new CGuiButtonSwitch(NULL, NULL, NULL,
															 px, py, posZ, buttonSizeX, buttonSizeY,
															 new CSlrString(" MEMORY "),
															 FONT_ALIGN_CENTER, buttonSizeX/2, 3.5,
															 font, fontScale,
															 1.0, 1.0, 1.0, 1.0,
															 1.0, 1.0, 1.0, 1.0,
															 0.3, 0.3, 0.3, 1.0,
															 this);
		btnBreakpointsDrive1541Memory->SetOn(false);
		this->AddGuiElement(btnBreakpointsDrive1541Memory);
		
		Drive1541MemoryBreakpointsX = px - 8;
		Drive1541MemoryBreakpointsY = py + buttonSizeY + 5;
		
		py = Drive1541MemoryBreakpointsY + fontNumbersHeight * 10;
		
		//
		cursorGroup = CURSOR_GROUP_C64_IRQVIC; //CURSOR_GROUP_C64_IRQVIC;//CURSOR_GROUP_C64_MEMORY;CURSOR_GROUP_C64_ADDR_PC;//CURSOR_GROUP_C64_IRQVIC; //CURSOR_GROUP_C64_ADDR_PC;

	}
	else
	{
		// atari
		cursorGroup = CURSOR_GROUP_C64_ENABLE_PC; //CURSOR_GROUP_C64_IRQVIC;//CURSOR_GROUP_C64_MEMORY;CURSOR_GROUP_C64_ADDR_PC;//CURSOR_GROUP_C64_IRQVIC; //CURSOR_GROUP_C64_ADDR_PC;
	}
	
	
	cursorElement = 0;      //0; //2;
	cursorPosition = -1;
	
	strTemp = new CSlrString();
	
	editHex = new CGuiEditHex(this);
	isEditingValue = false;
	editingBreakpoint = NULL;
	
	UpdateCursor();
	
	UpdateTheme();
	
	viewC64->colorsTheme->AddThemeChangeListener(this);
}

CViewBreakpointsOLD::~CViewBreakpointsOLD()
{
}

void CViewBreakpointsOLD::UpdateTheme()
{
	tr = viewC64->colorsTheme->colorTextR;
	tg = viewC64->colorsTheme->colorTextG;
	tb = viewC64->colorsTheme->colorTextB;
	
	lblPlatform->textColorR = tr;
	lblPlatform->textColorG = tg;
	lblPlatform->textColorB = tb;

	if (debugInterface->GetEmulatorType() == EMULATOR_TYPE_C64_VICE)
	{
		lbl1541Drive->textColorR = tr;
		lbl1541Drive->textColorG = tg;
		lbl1541Drive->textColorB = tb;
	}
	
	CGuiView::UpdateTheme();
}

bool CViewBreakpointsOLD::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	/*
	if (isEditingValue)
	{
		keyCode = SYS_GetBareKey(keyCode, isShift, isAlt, isControl, isSuper);
		editHex->KeyDown(keyCode);
		return true;
	}
	
	if (keyCode == MTKEY_ESC)
	{
		SwitchBreakpointsScreen();
		return true;
	}

	// NO! we use backspace to delete breakpoint!
//	if (keyCode == MTKEY_BACKSPACE)
//	{
//		guiMain->SetView(prevView);
//		return true;
//	}
	
	//// type in memory breakpoint !=, ==, <, etc...
	
	if ( (cursorGroup == CURSOR_GROUP_C64_MEMORY || cursorGroup == CURSOR_GROUP_DRIVE1541_MEMORY)
		&& cursorPosition != -1 )
	{
		CBreakpointMemory *breakpoint = (CBreakpointMemory *)editingBreakpoint;
		
		if (keyCode == MTKEY_ARROW_LEFT)
		{
			if (cursorPosition == 2 || cursorPosition == 3)
			{
				cursorPosition = 1;
				editHex->SetValue(breakpoint->addr, 4);
				editHex->cursorPos = 3;
				editHex->UpdateCursor();
				isEditingValue = true;
			}
			return true;
		}
		if (keyCode == MTKEY_ARROW_RIGHT)
		{
			if (cursorPosition == 2 || cursorPosition == 3)
			{
				cursorPosition = 4;
				editHex->SetValue(breakpoint->value, 2);
				isEditingValue = true;
			}
			return true;
		}
		
		if (cursorPosition == 2)
		{
			if (keyCode == '!')
			{
				breakpoint->comparison = MEMORY_BREAKPOINT_NOT_EQUAL;
			}
			else if (keyCode == '=')
			{
				breakpoint->comparison = MEMORY_BREAKPOINT_EQUAL;
			}
			else if (keyCode == '<')
			{
				breakpoint->comparison = MEMORY_BREAKPOINT_LESS;
			}
			else if (keyCode == '>')
			{
				breakpoint->comparison = MEMORY_BREAKPOINT_GREATER;
			}
			else if (keyCode >= '0' && keyCode <= '9')
			{
				cursorPosition = 4;
				editHex->SetValue(breakpoint->value, 2);
				isEditingValue = true;
				return true;
			}
			else if (keyCode == MTKEY_ENTER)
			{
				cursorPosition = 4;
				editHex->SetValue(breakpoint->value, 2);
				isEditingValue = true;
				return true;
			}
			else
			{
				return true;
			}
			
			cursorPosition = 3;
			return true;
		}
		else if (cursorPosition == 3)
		{
			if (keyCode == '!')
			{
				breakpoint->comparison = MEMORY_BREAKPOINT_NOT_EQUAL;
			}
			else if (keyCode == '=')
			{
				if (breakpoint->comparison == MEMORY_BREAKPOINT_GREATER)
				{
					breakpoint->comparison = MEMORY_BREAKPOINT_GREATER_OR_EQUAL;
				}
				else if (breakpoint->comparison == MEMORY_BREAKPOINT_LESS)
				{
					breakpoint->comparison = MEMORY_BREAKPOINT_LESS_OR_EQUAL;
				}
				else if (breakpoint->comparison == MEMORY_BREAKPOINT_NOT_EQUAL)
				{
					// leave as it is
				}
				else
				{
					breakpoint->comparison = MEMORY_BREAKPOINT_EQUAL;
				}
			}
			else if (keyCode == '<')
			{
				if (breakpoint->comparison == MEMORY_BREAKPOINT_EQUAL)
				{
					breakpoint->comparison = MEMORY_BREAKPOINT_LESS_OR_EQUAL;
				}
				else
				{
					breakpoint->comparison = MEMORY_BREAKPOINT_LESS;
				}
			}
			else if (keyCode == '>')
			{
				if (breakpoint->comparison == MEMORY_BREAKPOINT_EQUAL)
				{
					breakpoint->comparison = MEMORY_BREAKPOINT_GREATER_OR_EQUAL;
				}
				else
				{
					breakpoint->comparison = MEMORY_BREAKPOINT_GREATER;
				}
			}
			else if (keyCode >= '0' && keyCode <= '9')
			{
				// move to cursor 4 after elses
			}
			else if (keyCode == MTKEY_ENTER)
			{
			}
			else
			{
				return true;
			}
			
			cursorPosition = 4;
			editHex->SetValue(breakpoint->value, 2);
			isEditingValue = true;
			return true;
		}
	}
	
	
	////
	

	if (keyCode == MTKEY_ARROW_DOWN)
	{
		if (cursorGroup == CURSOR_GROUP_C64_IRQVIC || cursorGroup == CURSOR_GROUP_C64_IRQCIA || cursorGroup == CURSOR_GROUP_C64_IRQNMI)
		{
			cursorGroup = CURSOR_GROUP_C64_ENABLE_PC;
		}
		else if (cursorGroup == CURSOR_GROUP_C64_ENABLE_PC)
		{
			cursorGroup = CURSOR_GROUP_C64_ADDR_PC;
			cursorElement = 0;
		}
		else if (cursorGroup == CURSOR_GROUP_C64_ADDR_PC)
		{
			cursorElement += 8;
			if (cursorElement > debugInterface->breakpointsPC->breakpoints.size())
			{
				cursorGroup = CURSOR_GROUP_C64_ENABLE_MEMORY;
			}
		}
		else if (cursorGroup == CURSOR_GROUP_C64_ENABLE_MEMORY)
		{
			cursorGroup = CURSOR_GROUP_C64_MEMORY;
			cursorElement = 0;
		}
		else if (cursorGroup == CURSOR_GROUP_C64_MEMORY)
		{
			cursorElement += 4;
			if (cursorElement > debugInterface->breakpointsMemory->breakpoints.size())
			{
				cursorGroup = CURSOR_GROUP_C64_ENABLE_RASTER;
			}
		}
		else if (cursorGroup == CURSOR_GROUP_C64_ENABLE_RASTER)
		{
			cursorGroup = CURSOR_GROUP_C64_RASTER;
			cursorElement = 0;
		}
		else if (cursorGroup == CURSOR_GROUP_C64_RASTER)
		{
			cursorElement += 8;
			if (cursorElement > debugInterface->breakpointsRasterLine->breakpoints.size())
			{
				cursorElement = debugInterface->breakpointsRasterLine->breakpoints.size();
			}
		}
		// disk
		if (cursorGroup == CURSOR_GROUP_DRIVE1541_IRQVIA1 || cursorGroup == CURSOR_GROUP_DRIVE1541_IRQVIA2 || cursorGroup == CURSOR_GROUP_DRIVE1541_IRQIEC)
		{
			cursorGroup = CURSOR_GROUP_DRIVE1541_ENABLE_PC;
		}
		else if (cursorGroup == CURSOR_GROUP_DRIVE1541_ENABLE_PC)
		{
			cursorGroup = CURSOR_GROUP_DRIVE1541_ADDR_PC;
			cursorElement = 0;
		}
		else if (cursorGroup == CURSOR_GROUP_DRIVE1541_ADDR_PC)
		{
			cursorElement += 8;
			if (cursorElement > viewC64->debugInterfaceC64->breakpointsDrive1541PC->breakpoints.size())
			{
				cursorGroup = CURSOR_GROUP_DRIVE1541_ENABLE_MEMORY;
			}
		}
		else if (cursorGroup == CURSOR_GROUP_DRIVE1541_ENABLE_MEMORY)
		{
			cursorGroup = CURSOR_GROUP_DRIVE1541_MEMORY;
			cursorElement = 0;
		}
		else if (cursorGroup == CURSOR_GROUP_DRIVE1541_MEMORY)
		{
			cursorElement += 4;
			if (cursorElement > viewC64->debugInterfaceC64->breakpointsDrive1541Memory->breakpoints.size())
			{
				cursorElement = viewC64->debugInterfaceC64->breakpointsDrive1541Memory->breakpoints.size();
			}
		}
	}
	else if (keyCode == MTKEY_ARROW_UP)
	{
		if (cursorGroup == CURSOR_GROUP_C64_IRQVIC)
		{
			cursorGroup = CURSOR_GROUP_C64_IRQVIC;
		}
		else if (cursorGroup == CURSOR_GROUP_C64_ENABLE_PC)
		{
			cursorGroup = CURSOR_GROUP_C64_IRQVIC;
			cursorElement = 0;
		}
		else if (cursorGroup == CURSOR_GROUP_C64_ADDR_PC)
		{
			cursorElement -= 8;
			if (cursorElement < 0)
			{
				cursorGroup = CURSOR_GROUP_C64_ENABLE_PC;
			}
		}
		else if (cursorGroup == CURSOR_GROUP_C64_ENABLE_MEMORY)
		{
			cursorGroup = CURSOR_GROUP_C64_ADDR_PC;
			cursorElement = 0;
		}
		else if (cursorGroup == CURSOR_GROUP_C64_MEMORY)
		{
			cursorElement -= 4;
			if (cursorElement < 0)
			{
				cursorGroup = CURSOR_GROUP_C64_ENABLE_MEMORY;
			}
		}
		else if (cursorGroup == CURSOR_GROUP_C64_ENABLE_RASTER)
		{
			cursorGroup = CURSOR_GROUP_C64_MEMORY;
			cursorElement = 0;
		}
		else if (cursorGroup == CURSOR_GROUP_C64_RASTER)
		{
			cursorElement -= 8;
			if (cursorElement < 0)
			{
				cursorGroup = CURSOR_GROUP_C64_ENABLE_RASTER;
			}
		}
		// disk
		else if (cursorGroup == CURSOR_GROUP_DRIVE1541_ENABLE_PC)
		{
			cursorGroup = CURSOR_GROUP_DRIVE1541_IRQVIA1;
			cursorElement = 0;
		}
		else if (cursorGroup == CURSOR_GROUP_DRIVE1541_ADDR_PC)
		{
			cursorElement -= 8;
			if (cursorElement < 0)
			{
				cursorGroup = CURSOR_GROUP_DRIVE1541_ENABLE_PC;
			}
		}
		else if (cursorGroup == CURSOR_GROUP_DRIVE1541_ENABLE_MEMORY)
		{
			cursorGroup = CURSOR_GROUP_DRIVE1541_ADDR_PC;
			cursorElement = 0;
		}
		else if (cursorGroup == CURSOR_GROUP_DRIVE1541_MEMORY)
		{
			cursorElement -= 4;
			if (cursorElement < 0)
			{
				cursorGroup = CURSOR_GROUP_DRIVE1541_ENABLE_MEMORY;
			}
		}
		
	}
	else if (keyCode == MTKEY_ARROW_RIGHT)
	{
		if (cursorGroup == CURSOR_GROUP_C64_IRQVIC)
		{
			cursorGroup = CURSOR_GROUP_C64_IRQCIA;
		}
		else if (cursorGroup == CURSOR_GROUP_C64_IRQCIA)
		{
			cursorGroup = CURSOR_GROUP_C64_IRQNMI;
		}
		else if (cursorGroup == CURSOR_GROUP_C64_IRQNMI)
		{
			cursorGroup = CURSOR_GROUP_DRIVE1541_IRQVIA1;
		}
		else if (cursorGroup == CURSOR_GROUP_DRIVE1541_IRQVIA1)
		{
			cursorGroup = CURSOR_GROUP_DRIVE1541_IRQVIA2;
		}
		else if (cursorGroup == CURSOR_GROUP_DRIVE1541_IRQVIA2)
		{
			cursorGroup = CURSOR_GROUP_DRIVE1541_IRQIEC;
		}
		else if (cursorGroup == CURSOR_GROUP_C64_ENABLE_PC)
		{
			cursorGroup = CURSOR_GROUP_DRIVE1541_ENABLE_PC;
		}
		else if (cursorGroup == CURSOR_GROUP_C64_ADDR_PC)
		{
			if (cursorElement < debugInterface->breakpointsPC->breakpoints.size())
			{
				cursorElement += 1;
			}
		}
		else if (cursorGroup == CURSOR_GROUP_C64_ENABLE_MEMORY)
		{
			cursorGroup = CURSOR_GROUP_DRIVE1541_ENABLE_MEMORY;
		}
		else if (cursorGroup == CURSOR_GROUP_C64_MEMORY)
		{
			if (cursorElement < debugInterface->breakpointsMemory->breakpoints.size())
			{
				cursorElement += 1;
			}
		}
		else if (cursorGroup == CURSOR_GROUP_C64_RASTER)
		{
			if (cursorElement < debugInterface->breakpointsRasterLine->breakpoints.size())
			{
				cursorElement += 1;
			}
		}
		else if (cursorGroup == CURSOR_GROUP_DRIVE1541_ADDR_PC)
		{
			if (cursorElement < viewC64->debugInterfaceC64->breakpointsDrive1541PC->breakpoints.size())
			{
				cursorElement += 1;
			}
		}
		else if (cursorGroup == CURSOR_GROUP_DRIVE1541_MEMORY)
		{
			if (cursorElement < viewC64->debugInterfaceC64->breakpointsDrive1541Memory->breakpoints.size())
			{
				cursorElement += 1;
			}
		}
	}
	else if (keyCode == MTKEY_ARROW_LEFT)
	{
		if (cursorGroup == CURSOR_GROUP_C64_IRQNMI)
		{
			cursorGroup = CURSOR_GROUP_C64_IRQCIA;
		}
		else if (cursorGroup == CURSOR_GROUP_C64_IRQCIA)
		{
			cursorGroup = CURSOR_GROUP_C64_IRQVIC;
		}
		else if (cursorGroup == CURSOR_GROUP_DRIVE1541_IRQVIA1)
		{
			cursorGroup = CURSOR_GROUP_C64_IRQNMI;
		}
		else if (cursorGroup == CURSOR_GROUP_DRIVE1541_IRQVIA2)
		{
			cursorGroup = CURSOR_GROUP_DRIVE1541_IRQVIA1;
		}
		else if (cursorGroup == CURSOR_GROUP_DRIVE1541_IRQIEC)
		{
			cursorGroup = CURSOR_GROUP_DRIVE1541_IRQVIA2;
		}
		else if (cursorGroup == CURSOR_GROUP_DRIVE1541_ENABLE_PC)
		{
			cursorGroup = CURSOR_GROUP_C64_ENABLE_PC;
		}
		else if (cursorGroup == CURSOR_GROUP_DRIVE1541_ENABLE_MEMORY)
		{
			cursorGroup = CURSOR_GROUP_C64_ENABLE_MEMORY;
		}
		else if (cursorGroup == CURSOR_GROUP_C64_ADDR_PC || cursorGroup == CURSOR_GROUP_C64_MEMORY || cursorGroup == CURSOR_GROUP_C64_RASTER
				 || cursorGroup == CURSOR_GROUP_DRIVE1541_ADDR_PC || cursorGroup == CURSOR_GROUP_DRIVE1541_MEMORY)
		{
			if (cursorElement > 0)
			{
				cursorElement -=1;
			}
		}
	}
	else if (keyCode == MTKEY_ENTER || keyCode == MTKEY_SPACEBAR)
	{
		if (cursorGroup == CURSOR_GROUP_C64_IRQVIC)
		{
			btnBreakpointC64IrqVIC->DoSwitch();
		}
		else if (cursorGroup == CURSOR_GROUP_C64_IRQCIA)
		{
			btnBreakpointC64IrqCIA->DoSwitch();
		}
		else if (cursorGroup == CURSOR_GROUP_C64_IRQNMI)
		{
			btnBreakpointC64IrqNMI->DoSwitch();
		}
		else if (cursorGroup == CURSOR_GROUP_C64_ENABLE_PC)
		{
			btnBreakpointsPC->DoSwitch();
		}
		else if (cursorGroup == CURSOR_GROUP_C64_ENABLE_MEMORY)
		{
			btnBreakpointsMemory->DoSwitch();
		}
		else if (cursorGroup == CURSOR_GROUP_C64_ENABLE_RASTER)
		{
			btnBreakpointsRaster->DoSwitch();
		}
		else if (cursorGroup == CURSOR_GROUP_C64_ADDR_PC)
		{
			StartEditingSelectedAddrBreakpoint(debugInterface->breakpointsPC, "....");
		}
		else if (cursorGroup == CURSOR_GROUP_C64_MEMORY)
		{
			StartEditingSelectedMemoryBreakpoint(debugInterface->breakpointsMemory);
		}
		else if (cursorGroup == CURSOR_GROUP_C64_RASTER)
		{
			StartEditingSelectedAddrBreakpoint(debugInterface->breakpointsRasterLine, "...");
		}
		//
		if (cursorGroup == CURSOR_GROUP_DRIVE1541_IRQVIA1)
		{
			btnBreakpointDrive1541IrqVIA1->DoSwitch();
		}
		else if (cursorGroup == CURSOR_GROUP_DRIVE1541_IRQVIA2)
		{
			btnBreakpointDrive1541IrqVIA2->DoSwitch();
		}
		else if (cursorGroup == CURSOR_GROUP_DRIVE1541_IRQIEC)
		{
			btnBreakpointDrive1541IrqIEC->DoSwitch();
		}
		else if (cursorGroup == CURSOR_GROUP_DRIVE1541_ENABLE_PC)
		{
			btnBreakpointsDrive1541PC->DoSwitch();
		}
		else if (cursorGroup == CURSOR_GROUP_DRIVE1541_ENABLE_MEMORY)
		{
			btnBreakpointsDrive1541Memory->DoSwitch();
		}
		else if (cursorGroup == CURSOR_GROUP_DRIVE1541_ADDR_PC)
		{
			StartEditingSelectedAddrBreakpoint(viewC64->debugInterfaceC64->breakpointsDrive1541PC, "....");
		}
		else if (cursorGroup == CURSOR_GROUP_DRIVE1541_MEMORY)
		{
			StartEditingSelectedMemoryBreakpoint(viewC64->debugInterfaceC64->breakpointsDrive1541Memory);
		}

	}
	else if ((keyCode >= '0' && keyCode <= '9') || (keyCode >= 'a' && keyCode <= 'f'))
	{
		if (cursorGroup == CURSOR_GROUP_C64_ADDR_PC
			|| cursorGroup == CURSOR_GROUP_C64_MEMORY
			|| cursorGroup == CURSOR_GROUP_C64_RASTER
			|| cursorGroup == CURSOR_GROUP_DRIVE1541_ADDR_PC
			|| cursorGroup == CURSOR_GROUP_DRIVE1541_MEMORY)
		{
			// simulate start entering value
			this->KeyDown(MTKEY_ENTER, false, false, false);
			editHex->KeyDown(keyCode);
		}
	}
	else if (keyCode == MTKEY_BACKSPACE)
	{
		if (cursorGroup == CURSOR_GROUP_C64_ADDR_PC)
		{
			DeleteSelectedAddrBreakpoint(debugInterface->breakpointsPC);
		}
		else if (cursorGroup == CURSOR_GROUP_C64_MEMORY)
		{
			DeleteSelectedMemoryBreakpoint(debugInterface->breakpointsMemory);
		}
		else if (cursorGroup == CURSOR_GROUP_C64_RASTER)
		{
			DeleteSelectedAddrBreakpoint(debugInterface->breakpointsRasterLine);
		}
		else if (cursorGroup == CURSOR_GROUP_DRIVE1541_ADDR_PC)
		{
			DeleteSelectedAddrBreakpoint(viewC64->debugInterfaceC64->breakpointsDrive1541PC);
		}
		else if (cursorGroup == CURSOR_GROUP_DRIVE1541_MEMORY)
		{
			DeleteSelectedMemoryBreakpoint(viewC64->debugInterfaceC64->breakpointsDrive1541Memory);
		}
	}
	
	UpdateCursor();
	*/
	
	return CGuiView::KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
}

void CViewBreakpointsOLD::StartEditingSelectedAddrBreakpoint(CDebugBreakpointsAddr *addrBreakpoints, char *emptyAddrStr)
{
	if (cursorElement < addrBreakpoints->breakpoints.size())
	{
		std::map<int, CBreakpointAddr *>::iterator it = addrBreakpoints->breakpoints.begin();
		for (int i = 0; i < cursorElement; i++)
		{
			it++;
		}
		editingBreakpoint = it->second;
		editHex->SetValue(editingBreakpoint->addr, 4);
	}
	else
	{
		editHex->SetText(new CSlrString(emptyAddrStr));
	}
	isEditingValue = true;
	this->cursorPosition = 1;
	
}

void CViewBreakpointsOLD::StartEditingSelectedMemoryBreakpoint(CDebugBreakpointsMemory *memoryBreakpoints)
{
	if (cursorElement < memoryBreakpoints->breakpoints.size())
	{
		std::map<int, CBreakpointAddr *>::iterator it = memoryBreakpoints->breakpoints.begin();
		for (int i = 0; i < cursorElement; i++)
		{
			it++;
		}
		editingBreakpoint = (CBreakpointMemory *)it->second;
		editHex->SetValue(editingBreakpoint->addr, 4);
	}
	else
	{
		editingBreakpoint = new CBreakpointMemory(0, MEMORY_BREAKPOINT_ACCESS_WRITE, MemoryBreakpointComparison::MEMORY_BREAKPOINT_EQUAL, 0);
		editHex->SetText(new CSlrString("...."));
	}
	isEditingValue = true;
	this->cursorPosition = 1;
}

void CViewBreakpointsOLD::DeleteSelectedAddrBreakpoint(CDebugBreakpointsAddr *addrBreakpoints)
{
	debugInterface->LockMutex();
	if (cursorElement < addrBreakpoints->breakpoints.size())
	{
		std::map<int, CBreakpointAddr *>::iterator it = addrBreakpoints->breakpoints.begin();
		for (int i = 0; i < cursorElement; i++)
		{
			it++;
		}
		CBreakpointAddr *breakpoint = it->second;
		addrBreakpoints->breakpoints.erase(it);
		delete breakpoint;
	}
	debugInterface->UnlockMutex();
}

void CViewBreakpointsOLD::DeleteSelectedMemoryBreakpoint(CDebugBreakpointsMemory *memoryBreakpoints)
{
	debugInterface->LockMutex();
	if (cursorElement < memoryBreakpoints->breakpoints.size())
	{
		std::map<int, CBreakpointAddr *>::iterator it = memoryBreakpoints->breakpoints.begin();
		for (int i = 0; i < cursorElement; i++)
		{
			it++;
		}
		CBreakpointAddr *breakpoint = it->second;
		memoryBreakpoints->breakpoints.erase(it);
		delete breakpoint;
	}
	debugInterface->UnlockMutex();
}


void CViewBreakpointsOLD::GuiEditHexEnteredValue(CGuiEditHex *editHex, u32 lastKeyCode, bool isCancelled)
{
	///
	/*

	debugInterface->LockMutex();
	if (cursorGroup == CURSOR_GROUP_C64_ADDR_PC)
	{
		GuiEditHexEnteredValueAddr(editHex, debugInterface->breakpointsPC);
		this->btnBreakpointsPC->SetOn(true);
		debugInterface->breakOnPC = true;
	}
	else if (cursorGroup == CURSOR_GROUP_C64_MEMORY)
	{
		GuiEditHexEnteredValueMemory(editHex, lastKeyCode, debugInterface->breakpointsMemory);
		this->btnBreakpointsMemory->SetOn(true);
		debugInterface->breakOnMemory = true;
	}
	else if (cursorGroup == CURSOR_GROUP_C64_RASTER)
	{
		GuiEditHexEnteredValueAddr(editHex, debugInterface->breakpointsRasterLine);
		this->btnBreakpointsRaster->SetOn(true);
		debugInterface->breakOnRaster = true;
	}
	else if (cursorGroup == CURSOR_GROUP_DRIVE1541_ADDR_PC)
	{
		GuiEditHexEnteredValueAddr(editHex, viewC64->debugInterfaceC64->breakpointsDrive1541PC);
		this->btnBreakpointsDrive1541PC->SetOn(true);
		((CDebugInterfaceC64 *)debugInterface)->breakOnDrive1541PC = true;
	}
	else if (cursorGroup == CURSOR_GROUP_DRIVE1541_MEMORY)
	{
		GuiEditHexEnteredValueMemory(editHex, lastKeyCode, viewC64->debugInterfaceC64->breakpointsDrive1541Memory);
		this->btnBreakpointsDrive1541Memory->SetOn(true);
		((CDebugInterfaceC64 *)debugInterface)->breakOnDrive1541Memory = true;
	}

	debugInterface->UnlockMutex();
	 */

}

void CViewBreakpointsOLD::GuiEditHexEnteredValueAddr(CGuiEditHex *editHex, CDebugBreakpointsAddr *addrBreakpoints)
{
	if (cursorElement < addrBreakpoints->breakpoints.size())
	{
		std::map<int, CBreakpointAddr *>::iterator it = addrBreakpoints->breakpoints.begin();
		for (int i = 0; i < cursorElement; i++)
		{
			it++;
		}
		CBreakpointAddr *addrBreakpoint = it->second;
		addrBreakpoints->breakpoints.erase(it);
		addrBreakpoint->addr = editHex->value;
		addrBreakpoints->breakpoints[addrBreakpoint->addr] = addrBreakpoint;
	}
	else
	{
		std::map<int, CBreakpointAddr *>::iterator it = addrBreakpoints->breakpoints.find(editHex->value);
		if (it == addrBreakpoints->breakpoints.end())
		{
			CBreakpointAddr *addrBreakpoint = new CBreakpointAddr(editHex->value);
			addrBreakpoints->breakpoints[addrBreakpoint->addr] = addrBreakpoint;
		}
	}
	
	// position cursor on this
	cursorElement = 0;
	for (std::map<int, CBreakpointAddr *>::iterator it = addrBreakpoints->breakpoints.begin();
		 it != addrBreakpoints->breakpoints.end(); it++)
	{
		CBreakpointAddr *addrBreakpoint = it->second;
		if (addrBreakpoint->addr == editHex->value)
			break;
		
		cursorElement++;
	}
	
	isEditingValue = false;
	this->cursorPosition = -1;
}

void CViewBreakpointsOLD::GuiEditHexEnteredValueMemory(CGuiEditHex *editHex, u32 lastKeyCode, CDebugBreakpointsMemory *memoryBreakpoints)
{
	if (cursorPosition == 1)
	{
		if (lastKeyCode != MTKEY_ARROW_LEFT)
		{
			CBreakpointMemory *memoryBreakpoint = (CBreakpointMemory *)editingBreakpoint;
			memoryBreakpoint->addr = editHex->value;
			isEditingValue = false;
			cursorPosition = 2;
		}
	}
	else if (cursorPosition == 4)
	{
		if (lastKeyCode == MTKEY_ARROW_LEFT)
		{
			CBreakpointMemory *memoryBreakpoint = (CBreakpointMemory *)editingBreakpoint;
			memoryBreakpoint->value = editHex->value;
			cursorPosition = 2;
			isEditingValue = false;
		}
		else
		{
			u16 addr;
			
			if (cursorElement < memoryBreakpoints->breakpoints.size())
			{
				std::map<int, CBreakpointAddr *>::iterator it = memoryBreakpoints->breakpoints.begin();
				for (int i = 0; i < cursorElement; i++)
				{
					it++;
				}
				CBreakpointMemory *memoryBreakpoint = (CBreakpointMemory*)it->second;
				memoryBreakpoints->breakpoints.erase(it);
				memoryBreakpoint->value = editHex->value;
				memoryBreakpoints->breakpoints[memoryBreakpoint->addr] = memoryBreakpoint;
				addr = memoryBreakpoint->addr;
			}
			else
			{
				CBreakpointMemory *memoryBreakpoint = (CBreakpointMemory *)editingBreakpoint;
				std::map<int, CBreakpointAddr *>::iterator it = memoryBreakpoints->breakpoints.find(memoryBreakpoint->addr);
				if (it == memoryBreakpoints->breakpoints.end())
				{
					memoryBreakpoints->breakpoints[memoryBreakpoint->addr] = memoryBreakpoint;
					addr = memoryBreakpoint->addr;
					memoryBreakpoint->value = editHex->value;
				}
				else
				{
					delete memoryBreakpoint;
					memoryBreakpoint = (CBreakpointMemory*)it->second;
					memoryBreakpoint->value = editHex->value;
					addr = memoryBreakpoint->addr;
				}
			}
			
			// position cursor on this
			cursorElement = 0;
			for (std::map<int, CBreakpointAddr *>::iterator it = memoryBreakpoints->breakpoints.begin();
				 it != memoryBreakpoints->breakpoints.end(); it++)
			{
				CBreakpointAddr *addrBreakpoint = it->second;
				if (addrBreakpoint->addr == addr)
					break;
				
				cursorElement++;
			}
			
			isEditingValue = false;
			this->cursorPosition = -1;
		}
	}
}


void CViewBreakpointsOLD::ClearCursor()
{
	ClearInvertCBMText(btnBreakpointsPC->textUTF);
	ClearInvertCBMText(btnBreakpointsMemory->textUTF);

	if (debugInterface->GetEmulatorType() == EMULATOR_TYPE_C64_VICE)
	{
		ClearInvertCBMText(btnBreakpointC64IrqVIC->textUTF);
		ClearInvertCBMText(btnBreakpointC64IrqCIA->textUTF);
		ClearInvertCBMText(btnBreakpointC64IrqNMI->textUTF);
		ClearInvertCBMText(btnBreakpointDrive1541IrqVIA1->textUTF);
		ClearInvertCBMText(btnBreakpointDrive1541IrqVIA2->textUTF);
		ClearInvertCBMText(btnBreakpointDrive1541IrqIEC->textUTF);
		ClearInvertCBMText(btnBreakpointsDrive1541PC->textUTF);
		ClearInvertCBMText(btnBreakpointsDrive1541Memory->textUTF);
		ClearInvertCBMText(btnBreakpointsRaster->textUTF);
	}

}

void CViewBreakpointsOLD::UpdateCursor()
{
	/*
	ClearCursor();
	if (cursorGroup == CURSOR_GROUP_C64_IRQVIC)
	{
		InvertCBMText(btnBreakpointC64IrqVIC->textUTF);
	}
	else if (cursorGroup == CURSOR_GROUP_C64_IRQCIA)
	{
		InvertCBMText(btnBreakpointC64IrqCIA->textUTF);
	}
	else if (cursorGroup == CURSOR_GROUP_C64_IRQNMI)
	{
		InvertCBMText(btnBreakpointC64IrqNMI->textUTF);
	}
	else if (cursorGroup == CURSOR_GROUP_C64_ENABLE_PC)
	{
		InvertCBMText(btnBreakpointsPC->textUTF);
	}
	else if (cursorGroup == CURSOR_GROUP_C64_ENABLE_MEMORY)
	{
		InvertCBMText(btnBreakpointsMemory->textUTF);
	}
	else if (cursorGroup == CURSOR_GROUP_C64_ENABLE_RASTER)
	{
		InvertCBMText(btnBreakpointsRaster->textUTF);
	}
	else if (cursorGroup == CURSOR_GROUP_C64_ADDR_PC)
	{
		if (cursorElement > debugInterface->breakpointsPC->breakpoints.size())
		{
			cursorGroup = CURSOR_GROUP_C64_ENABLE_MEMORY;
		}
	}
	else if (cursorGroup == CURSOR_GROUP_C64_MEMORY)
	{
		if (cursorElement > debugInterface->breakpointsMemory->breakpoints.size())
		{
			cursorGroup = CURSOR_GROUP_C64_ENABLE_RASTER;
		}
	}
	else if (cursorGroup == CURSOR_GROUP_C64_RASTER)
	{
		if (cursorElement > debugInterface->breakpointsRasterLine->breakpoints.size())
		{
			cursorGroup = CURSOR_GROUP_DRIVE1541_IRQVIA1;
			//cursorElement = debugInterface->breakpointsRaster.size();
		}
	}
	//
	else if (cursorGroup == CURSOR_GROUP_DRIVE1541_IRQVIA1)
	{
		InvertCBMText(btnBreakpointDrive1541IrqVIA1->textUTF);
	}
	else if (cursorGroup == CURSOR_GROUP_DRIVE1541_IRQVIA2)
	{
		InvertCBMText(btnBreakpointDrive1541IrqVIA2->textUTF);
	}
	else if (cursorGroup == CURSOR_GROUP_DRIVE1541_IRQIEC)
	{
		InvertCBMText(btnBreakpointDrive1541IrqIEC->textUTF);
	}
	else if (cursorGroup == CURSOR_GROUP_DRIVE1541_ENABLE_PC)
	{
		InvertCBMText(btnBreakpointsDrive1541PC->textUTF);
	}
	else if (cursorGroup == CURSOR_GROUP_DRIVE1541_ENABLE_MEMORY)
	{
		InvertCBMText(btnBreakpointsDrive1541Memory->textUTF);
	}
	else if (cursorGroup == CURSOR_GROUP_DRIVE1541_ADDR_PC)
	{
		if (cursorElement > viewC64->debugInterfaceC64->breakpointsDrive1541PC->breakpoints.size())
		{
			cursorGroup = CURSOR_GROUP_DRIVE1541_ENABLE_MEMORY;
		}
	}
	else if (cursorGroup == CURSOR_GROUP_C64_MEMORY)
	{
		if (cursorElement > viewC64->debugInterfaceC64->breakpointsDrive1541Memory->breakpoints.size())
		{
			cursorElement = viewC64->debugInterfaceC64->breakpointsDrive1541Memory->breakpoints.size();
		}
	}
	 */
}

void CViewBreakpointsOLD::SwitchBreakpointsScreen()
{
	/*
	if (guiMain->currentView == this)
	{
		viewC64->ShowMainScreen();
		
		debugInterface->UpdateRenderBreakpoints();
	}
	else
	{
		guiMain->SetView(this);
		
		// update
		if (debugInterface->GetEmulatorType() == EMULATOR_TYPE_C64_VICE)
		{
			this->btnBreakpointC64IrqVIC->SetOn(viewC64->debugInterfaceC64->breakOnC64IrqVIC);
			this->btnBreakpointC64IrqCIA->SetOn(viewC64->debugInterfaceC64->breakOnC64IrqCIA);
			this->btnBreakpointC64IrqNMI->SetOn(viewC64->debugInterfaceC64->breakOnC64IrqNMI);
			this->btnBreakpointsRaster->SetOn(debugInterface->breakOnRaster);

			this->btnBreakpointDrive1541IrqVIA1->SetOn(viewC64->debugInterfaceC64->breakOnDrive1541IrqVIA1);
			this->btnBreakpointDrive1541IrqVIA2->SetOn(viewC64->debugInterfaceC64->breakOnDrive1541IrqVIA2);
			this->btnBreakpointDrive1541IrqIEC->SetOn(viewC64->debugInterfaceC64->breakOnDrive1541IrqIEC);
			this->btnBreakpointsDrive1541PC->SetOn(viewC64->debugInterfaceC64->breakOnDrive1541PC);
			this->btnBreakpointsDrive1541Memory->SetOn(viewC64->debugInterfaceC64->breakOnDrive1541Memory);
		}

		this->btnBreakpointsPC->SetOn(debugInterface->breakOnPC);
		this->btnBreakpointsMemory->SetOn(debugInterface->breakOnMemory);
	}
	 */
}


void CViewBreakpointsOLD::DoLogic()
{
	CGuiView::DoLogic();
}

// this is really shity code... refactor this immediately!
void CViewBreakpointsOLD::Render()
{
	/*
	float fr_r = viewC64->colorsTheme->colorBackgroundFrameR;
	float fr_g = viewC64->colorsTheme->colorBackgroundFrameG;
	float fr_b = viewC64->colorsTheme->colorBackgroundFrameB;
	BlitFilledRectangle(0, 0, -1, sizeX, sizeY, fr_r, fr_g, fr_b, 1.0);

	
	float sb = 20;
	
	float scrx = sb;
	float scry = sb;
	float scrsx = sizeX - sb*2.0f;
	float scrsy = sizeY - sb*2.0f;
	float cx = scrsx/2.0f + sb;

	// light blue interior
	float bg_r = viewC64->colorsTheme->colorBackgroundR;
	float bg_g = viewC64->colorsTheme->colorBackgroundG;
	float bg_b = viewC64->colorsTheme->colorBackgroundB;
	BlitFilledRectangle(scrx, scry, -1, scrsx, scrsy, bg_r, bg_g, bg_b, 1.0);
	
	
	float py = scry + 5;// + gap;
	
	// "Breakpoints" header
	float brh_r = viewC64->colorsTheme->colorTextHeaderR;
	float brh_g = viewC64->colorsTheme->colorTextHeaderG;
	float brh_b = viewC64->colorsTheme->colorTextHeaderB;
	font->BlitTextColor(strHeader, cx, py, -1, 3.0f, brh_r, brh_g, brh_b, 1, FONT_ALIGN_CENTER);

	py += fontHeight;
	py += 6.0f;
	
	float lr = viewC64->colorsTheme->colorHeaderLineR;
	float lg = viewC64->colorsTheme->colorHeaderLineG;
	float lb = viewC64->colorsTheme->colorHeaderLineB;
	float lSize = 3;

	// horizontal line
	BlitFilledRectangle(scrx, py, -1, scrsx, lSize, lr, lg, lb, 1);

	py += lSize;
	
	lr = viewC64->colorsTheme->colorVerticalLineR;
	lg = viewC64->colorsTheme->colorVerticalLineG;
	lb = viewC64->colorsTheme->colorVerticalLineB;

	// vertical line
	BlitFilledRectangle(cx-1.5f, py, -1, lSize, scrsy-fontHeight-lSize-11.0f, lr, lg, lb, 1.0);

//	py += gap + 4.0f;

	////
	// render breakpoints

	if (debugInterface->GetEmulatorType() == EMULATOR_TYPE_C64_VICE)
	{
		viewC64->viewC64Disassembly->renderBreakpointsMutex->Lock();
	}
	else if (debugInterface->GetEmulatorType() == EMULATOR_TYPE_ATARI800)
	{
		viewC64->viewAtariDisassembly->renderBreakpointsMutex->Lock();
	}
	else if (debugInterface->GetEmulatorType() == EMULATOR_TYPE_NESTOPIA)
	{
		viewC64->viewAtariDisassembly->renderBreakpointsMutex->Lock();
	}

	// c64 pc breakpoints
	RenderAddrBreakpoints(debugInterface->breakpointsPC, pcBreakpointsX, pcBreakpointsY, CURSOR_GROUP_C64_ADDR_PC,
						  "%4.4X", "....");
	
	// c64 memory breakpoints
	RenderMemoryBreakpoints(debugInterface->breakpointsMemory, memoryBreakpointsX, memoryBreakpointsY, CURSOR_GROUP_C64_MEMORY);
	
	if (debugInterface->GetEmulatorType() == EMULATOR_TYPE_C64_VICE)
	{
		//// c64 raster breakpoints
		RenderAddrBreakpoints(debugInterface->breakpointsRasterLine, rasterBreakpointsX, rasterBreakpointsY,
							  CURSOR_GROUP_C64_RASTER, " %3.3X", " ...");
		
		///////////
		// Drive1541 pc breakpoints
		RenderAddrBreakpoints(viewC64->debugInterfaceC64->breakpointsDrive1541PC, Drive1541PCBreakpointsX, Drive1541PCBreakpointsY,
							  CURSOR_GROUP_DRIVE1541_ADDR_PC, "%4.4X", "....");
		
		// Drive1541 memory breakpoints
		RenderMemoryBreakpoints(viewC64->debugInterfaceC64->breakpointsDrive1541Memory, Drive1541MemoryBreakpointsX, Drive1541MemoryBreakpointsY, CURSOR_GROUP_DRIVE1541_MEMORY);
	}
	
	if (debugInterface->GetEmulatorType() == EMULATOR_TYPE_C64_VICE)
	{
		viewC64->viewC64Disassembly->renderBreakpointsMutex->Unlock();
	}
	else if (debugInterface->GetEmulatorType() == EMULATOR_TYPE_ATARI800)
	{
		viewC64->viewAtariDisassembly->renderBreakpointsMutex->Unlock();
	}
	*/
	CGuiView::Render();
}

///
/// warning: a lot of copy/pasted code below. refactor this into something meaningful :)
///


void CViewBreakpointsOLD::RenderAddrBreakpoints(CDebugBreakpointsAddr *addrBreakpoints, float pStartX, float pStartY, int cursorGroupId,
												char *addrFormatStr, char *addrEmptyStr)
{
	///////
	/// addr breakpoints
	
	float width = fontWidth*4.0f;
	
	float px = pStartX;
	float py = pStartY;
	
	int elemNum = 0;
	for (std::map<int, CBreakpointAddr *>::iterator it = addrBreakpoints->breakpoints.begin();
		 it != addrBreakpoints->breakpoints.end(); it++)
	{
		CBreakpointAddr *addrBreakpoint = it->second;
		int addr = addrBreakpoint->addr;
		sprintf(buf, addrFormatStr, addr);
		
		strTemp->Set(buf);
		
		if (cursorGroup == cursorGroupId && elemNum == cursorElement)
		{
			if (cursorPosition == -1)
			{
				InvertCBMText(strTemp);
			}
			else
			{
				strTemp->Set(editHex->textWithCursor);
			}
		}
		
		if (IS_SET(addrBreakpoint->actions, ADDR_BREAKPOINT_ACTION_STOP))
		{
			font->BlitTextColor(strTemp, px, py, -1, fontNumbersScale, tr, tg, tb, 1, FONT_ALIGN_LEFT);
		}
		else
		{
			font->BlitTextColor(strTemp, px, py, -1, fontNumbersScale, 0.5f, 0.5f, 0.5f, 1, FONT_ALIGN_LEFT);
		}
		
		elemNum++;
		
		if (elemNum % 8 == 0)
		{
			py += fontNumbersHeight;
			px = pcBreakpointsX;
		}
		else
		{
			px += width;
		}
	}
	
	strcpy(buf, addrEmptyStr);
	strTemp->Set(buf);
	
	if (cursorGroup == cursorGroupId && elemNum == cursorElement)	//CURSOR_GROUP_C64_ADDR_PC
	{
		if (cursorPosition == -1)
		{
			InvertCBMText(strTemp);
		}
		else
		{
			strTemp->Set(editHex->textWithCursor);
		}
	}
	font->BlitTextColor(strTemp, px, py, -1, fontNumbersScale, tr, tg, tb, 1, FONT_ALIGN_LEFT);
}

void CViewBreakpointsOLD::RenderMemoryBreakpoints(CDebugBreakpointsMemory *memoryBreakpoints, float pStartX, float pStartY, int cursorGroupId)
{
	float width = fontWidth*8.5f;
	
	float px = pStartX;
	float py = pStartY;
	
	/// memory
	int elemNum = 0;
	for (std::map<int, CBreakpointAddr *>::iterator it = memoryBreakpoints->breakpoints.begin();
		 it != memoryBreakpoints->breakpoints.end(); it++)
	{
		CBreakpointMemory *memoryBreakpoint = (CBreakpointMemory*)it->second;
		
		char buf2[3] = {0};
		
		if (memoryBreakpoint->comparison == MEMORY_BREAKPOINT_EQUAL)
		{
			buf2[0] = '='; buf2[1] = '=';
		}
		else if (memoryBreakpoint->comparison == MEMORY_BREAKPOINT_GREATER)
		{
			buf2[0] = '>'; buf2[1] = ' ';
		}
		else if (memoryBreakpoint->comparison == MEMORY_BREAKPOINT_GREATER_OR_EQUAL)
		{
			buf2[0] = '>'; buf2[1] = '=';
		}
		else if (memoryBreakpoint->comparison == MEMORY_BREAKPOINT_LESS)
		{
			buf2[0] = '<'; buf2[1] = ' ';
		}
		else if (memoryBreakpoint->comparison == MEMORY_BREAKPOINT_LESS_OR_EQUAL)
		{
			buf2[0] = '='; buf2[1] = '<';
		}
		else if (memoryBreakpoint->comparison == MEMORY_BREAKPOINT_NOT_EQUAL)
		{
			buf2[0] = '!'; buf2[1] = '=';
		}
		
		//LOGD("type=%d buf2='%s'", memoryBreakpoint->breakpointType, buf2);
		
		if (cursorGroup == cursorGroupId && elemNum == cursorElement)
		{
			if (cursorPosition == -1)
			{
				sprintf(buf, "%4.4X %s %2.2X", memoryBreakpoint->addr, buf2, memoryBreakpoint->value);
				strTemp->Set(buf);
				InvertCBMText(strTemp);
				font->BlitTextColor(strTemp, px, py, -1, fontNumbersScale, tr, tg, tb, 1, FONT_ALIGN_LEFT);
			}
			else
			{
				// editing
				if (cursorPosition == 1)
				{
					strTemp->Set(editHex->textWithCursor);
					font->BlitTextColor(strTemp, px, py, -1, fontNumbersScale, tr, tg, tb, 1, FONT_ALIGN_LEFT);
					
					float px2 = px + fontNumbersWidth*5;
					sprintf(buf, "%s %2.2X", buf2, memoryBreakpoint->value);
					strTemp->Set(buf);
					font->BlitTextColor(strTemp, px2, py, -1, fontNumbersScale, tr, tg, tb, 1, FONT_ALIGN_LEFT);
				}
				else if (cursorPosition == 2 || cursorPosition == 3)
				{
					sprintf(buf, "%4.4X", memoryBreakpoint->addr);
					strTemp->Set(buf);
					font->BlitTextColor(strTemp, px, py, -1, fontNumbersScale, tr, tg, tb, 1, FONT_ALIGN_LEFT);
					
					float px2 = px + fontNumbersWidth*5;
					strTemp->Set(buf2);
					InvertCBMText(strTemp);
					font->BlitTextColor(strTemp, px2, py, -1, fontNumbersScale, tr, tg, tb, 1, FONT_ALIGN_LEFT);
					
					float px3 = px2 + fontNumbersWidth*3;
					sprintf(buf, "%2.2X", memoryBreakpoint->value);
					strTemp->Set(buf);
					font->BlitTextColor(strTemp, px3, py, -1, fontNumbersScale, tr, tg, tb, 1, FONT_ALIGN_LEFT);
				}
				else if (cursorPosition == 4)
				{
					sprintf(buf, "%4.4X %s", memoryBreakpoint->addr, buf2);
					strTemp->Set(buf);
					font->BlitTextColor(strTemp, px, py, -1, fontNumbersScale, tr, tg, tb, 1, FONT_ALIGN_LEFT);
					
					float px2 = px + fontNumbersWidth*8;
					strTemp->Set(editHex->textWithCursor);
					font->BlitTextColor(strTemp, px2, py, -1, fontNumbersScale, tr, tg, tb, 1, FONT_ALIGN_LEFT);
				}
			}
		}
		else
		{
			sprintf(buf, "%4.4X %s %2.2X", memoryBreakpoint->addr, buf2, memoryBreakpoint->value);
			strTemp->Set(buf);
			font->BlitTextColor(strTemp, px, py, -1, fontNumbersScale, tr, tg, tb, 1, FONT_ALIGN_LEFT);
		}
		
		elemNum++;
		
		if (elemNum % 4 == 0)
		{
			py += fontNumbersHeight;
			px = pStartX;
		}
		else
		{
			px += width;
		}
	}
	
	sprintf(buf, "..........");
	strTemp->Set(buf);
	
	if (cursorGroup == cursorGroupId && elemNum == cursorElement)
	{
		if (cursorPosition == -1)
		{
			InvertCBMText(strTemp);
			font->BlitTextColor(strTemp, px, py, -1, fontNumbersScale, tr, tg, tb, 1, FONT_ALIGN_LEFT);
		}
		else
		{
			/// editing new memory breakpoint
			/// ugh, again copy pasted code here with only slight differences... remember this is only a POC :D
			
			
			CBreakpointMemory *memoryBreakpoint = (CBreakpointMemory *)editingBreakpoint;
			
			char buf2[3] = {0};
			
			if (memoryBreakpoint->comparison == MEMORY_BREAKPOINT_EQUAL)
			{
				buf2[0] = '='; buf2[1] = '=';
			}
			else if (memoryBreakpoint->comparison == MEMORY_BREAKPOINT_GREATER)
			{
				buf2[0] = '>'; buf2[1] = ' ';
			}
			else if (memoryBreakpoint->comparison == MEMORY_BREAKPOINT_GREATER_OR_EQUAL)
			{
				buf2[0] = '>'; buf2[1] = '=';
			}
			else if (memoryBreakpoint->comparison == MEMORY_BREAKPOINT_LESS)
			{
				buf2[0] = '<'; buf2[1] = ' ';
			}
			else if (memoryBreakpoint->comparison == MEMORY_BREAKPOINT_LESS_OR_EQUAL)
			{
				buf2[0] = '='; buf2[1] = '<';
			}
			else if (memoryBreakpoint->comparison == MEMORY_BREAKPOINT_NOT_EQUAL)
			{
				buf2[0] = '!'; buf2[1] = '=';
			}
			
			//LOGD("type=%d buf2='%s'", memoryBreakpoint->breakpointType, buf2);
			
			if (cursorGroup == cursorGroupId && elemNum == cursorElement)
			{
				// editing
				if (cursorPosition == 1)
				{
					strTemp->Set(editHex->textWithCursor);
					font->BlitTextColor(strTemp, px, py, -1, fontNumbersScale, tr, tg, tb, 1, FONT_ALIGN_LEFT);
					
					float px2 = px + fontNumbersWidth*5;
					sprintf(buf, ".. ..");
					strTemp->Set(buf);
					font->BlitTextColor(strTemp, px2, py, -1, fontNumbersScale, tr, tg, tb, 1, FONT_ALIGN_LEFT);
				}
				else if (cursorPosition == 2 || cursorPosition == 3)
				{
					sprintf(buf, "%4.4X", memoryBreakpoint->addr);
					strTemp->Set(buf);
					font->BlitTextColor(strTemp, px, py, -1, fontNumbersScale, tr, tg, tb, 1, FONT_ALIGN_LEFT);
					
					float px2 = px + fontNumbersWidth*5;
					strTemp->Set(buf2);
					InvertCBMText(strTemp);
					font->BlitTextColor(strTemp, px2, py, -1, fontNumbersScale, tr, tg, tb, 1, FONT_ALIGN_LEFT);
					
					float px3 = px2 + fontNumbersWidth*3;
					sprintf(buf, "%2.2X", memoryBreakpoint->value);
					strTemp->Set(buf);
					font->BlitTextColor(strTemp, px3, py, -1, fontNumbersScale, tr, tg, tb, 1, FONT_ALIGN_LEFT);
				}
				else if (cursorPosition == 4)
				{
					sprintf(buf, "%4.4X %s", memoryBreakpoint->addr, buf2);
					strTemp->Set(buf);
					font->BlitTextColor(strTemp, px, py, -1, fontNumbersScale, tr, tg, tb, 1, FONT_ALIGN_LEFT);
					
					float px2 = px + fontNumbersWidth*8;
					strTemp->Set(editHex->textWithCursor);
					font->BlitTextColor(strTemp, px2, py, -1, fontNumbersScale, tr, tg, tb, 1, FONT_ALIGN_LEFT);
				}
			}
			///////////////
			
		}
	}
	else
	{
		font->BlitTextColor(strTemp, px, py, -1, fontNumbersScale, tr, tg, tb, 1, FONT_ALIGN_LEFT);
	}
}

bool CViewBreakpointsOLD::CheckTapAddrBreakpoints(float x, float y,
												  CDebugBreakpointsAddr *addrBreakpoints,
												  float pStartX, float pStartY, int cursorGroupId)
{
	///////
	/// addr breakpoints
	
	float width = fontWidth*4.0f;;
	
	float px = pStartX;
	float py = pStartY;
	
	int numElems = addrBreakpoints->breakpoints.size() + 1;
	
	for (int elemNum = 0; elemNum < numElems; )
	{
		if (x >= px && x <= (px + width)
			&& y >= py && y <= (py + fontNumbersHeight))
		{
			// found
			cursorGroup = cursorGroupId;
			cursorElement = elemNum;
			return true;
		}
		
		elemNum++;
		
		if (elemNum % 8 == 0)
		{
			py += fontNumbersHeight;
			px = pcBreakpointsX;
		}
		else
		{
			px += fontWidth*4.0f;
		}
	}
	
	return false;
}

bool CViewBreakpointsOLD::CheckTapMemoryBreakpoints(float x, float y,
												 CDebugBreakpointsMemory *memoryBreakpoints,
												 float pStartX, float pStartY, int cursorGroupId)
{
	float width = fontWidth*8.5f;
	
	float px = pStartX;
	float py = pStartY;
	
	/// memory
	int numElems = memoryBreakpoints->breakpoints.size() + 1;
	
	for (int elemNum = 0; elemNum < numElems; )
	{
		if (x >= px && x <= (px + width)
			&& y >= py && y <= (py + fontNumbersHeight))
		{
			// found
			cursorGroup = cursorGroupId;
			cursorElement = elemNum;
			return true;
		}
		
		elemNum++;
		
		if (elemNum % 4 == 0)
		{
			py += fontNumbersHeight;
			px = pStartX;
		}
		else
		{
			px += width;
		}
	}
	
	return false;
}


bool CViewBreakpointsOLD::ButtonSwitchChanged(CGuiButtonSwitch *button)
{
	/*
	if (button == btnBreakpointC64IrqVIC)
	{
		viewC64->debugInterfaceC64->breakOnC64IrqVIC = btnBreakpointC64IrqVIC->IsOn();
	}
	else if (button == btnBreakpointC64IrqCIA)
	{
		viewC64->debugInterfaceC64->breakOnC64IrqCIA = btnBreakpointC64IrqCIA->IsOn();
	}
	else if (button == btnBreakpointC64IrqNMI)
	{
		viewC64->debugInterfaceC64->breakOnC64IrqNMI = btnBreakpointC64IrqNMI->IsOn();
	}
	else if (button == btnBreakpointsPC)
	{
		debugInterface->breakOnPC = btnBreakpointsPC->IsOn();
	}
	else if (button == btnBreakpointsMemory)
	{
		debugInterface->breakOnMemory = btnBreakpointsMemory->IsOn();
	}
	else if (button == btnBreakpointsRaster)
	{
		debugInterface->breakOnRaster = btnBreakpointsRaster->IsOn();
	}
	else if (button == btnBreakpointDrive1541IrqVIA1)
	{
		viewC64->debugInterfaceC64->breakOnDrive1541IrqVIA1 = btnBreakpointDrive1541IrqVIA1->IsOn();
	}
	else if (button == btnBreakpointDrive1541IrqVIA2)
	{
		viewC64->debugInterfaceC64->breakOnDrive1541IrqVIA2 = btnBreakpointDrive1541IrqVIA2->IsOn();
	}
	else if (button == btnBreakpointDrive1541IrqIEC)
	{
		viewC64->debugInterfaceC64->breakOnDrive1541IrqIEC = btnBreakpointDrive1541IrqIEC->IsOn();
	}
	else if (button == btnBreakpointsDrive1541PC)
	{
		viewC64->debugInterfaceC64->breakOnDrive1541PC = btnBreakpointsDrive1541PC->IsOn();
	}
	else if (button == btnBreakpointsDrive1541Memory)
	{
		viewC64->debugInterfaceC64->breakOnDrive1541Memory = btnBreakpointsDrive1541Memory->IsOn();
	}
	*/
	return true;
}


void CViewBreakpointsOLD::Render(float posX, float posY)
{
	CGuiView::Render(posX, posY);
}

bool CViewBreakpointsOLD::ButtonClicked(CGuiButton *button)
{
	return false;
}

bool CViewBreakpointsOLD::ButtonPressed(CGuiButton *button)
{
	/*
	 if (button == btnDone)
	 {
		guiMain->SetView((CGuiView*)guiMain->viewMainEditor);
		GUI_SetPressConsumed(true);
		return true;
	 }
	 */
	return false;
}

//@returns is consumed
bool CViewBreakpointsOLD::DoTap(float x, float y)
{
	LOGG("CViewBreakpoints::DoTap:  x=%f y=%f", x, y);
	/*
	
	if (CheckTapAddrBreakpoints(x, y, debugInterface->breakpointsPC,
								pcBreakpointsX, pcBreakpointsY,
								CURSOR_GROUP_C64_ADDR_PC))
	{
		return true;
	}

	if (CheckTapMemoryBreakpoints(x, y, debugInterface->breakpointsMemory,
								  memoryBreakpointsX, memoryBreakpointsY,
								  CURSOR_GROUP_C64_MEMORY))
	{
		return true;
	}

	if (debugInterface->GetEmulatorType() == EMULATOR_TYPE_C64_VICE)
	{
		if (CheckTapAddrBreakpoints(x, y, debugInterface->breakpointsRasterLine,
									rasterBreakpointsX, rasterBreakpointsY,
									CURSOR_GROUP_C64_RASTER))
		{
			return true;
		}
		
		if (CheckTapAddrBreakpoints(x, y, viewC64->debugInterfaceC64->breakpointsDrive1541PC,
									Drive1541PCBreakpointsX, Drive1541PCBreakpointsY,
									CURSOR_GROUP_DRIVE1541_ADDR_PC))
		{
			return true;
		}
		
		if (CheckTapMemoryBreakpoints(x, y, viewC64->debugInterfaceC64->breakpointsDrive1541Memory,
									  Drive1541MemoryBreakpointsX, Drive1541MemoryBreakpointsY,
									  CURSOR_GROUP_DRIVE1541_MEMORY))
		{
			return true;
		}
	}
	*/
	return CGuiView::DoTap(x, y);
}

bool CViewBreakpointsOLD::DoFinishTap(float x, float y)
{
	LOGG("CViewBreakpoints::DoFinishTap: %f %f", x, y);
	return CGuiView::DoFinishTap(x, y);
}

//@returns is consumed
bool CViewBreakpointsOLD::DoDoubleTap(float x, float y)
{
	LOGG("CViewBreakpoints::DoDoubleTap:  x=%f y=%f", x, y);
	return CGuiView::DoDoubleTap(x, y);
}

bool CViewBreakpointsOLD::DoFinishDoubleTap(float x, float y)
{
	LOGG("CViewBreakpoints::DoFinishTap: %f %f", x, y);
	return CGuiView::DoFinishDoubleTap(x, y);
}


bool CViewBreakpointsOLD::DoMove(float x, float y, float distX, float distY, float diffX, float diffY)
{
	return CGuiView::DoMove(x, y, distX, distY, diffX, diffY);
}

bool CViewBreakpointsOLD::FinishMove(float x, float y, float distX, float distY, float accelerationX, float accelerationY)
{
	return CGuiView::FinishMove(x, y, distX, distY, accelerationX, accelerationY);
}

bool CViewBreakpointsOLD::InitZoom()
{
	return CGuiView::InitZoom();
}

bool CViewBreakpointsOLD::DoZoomBy(float x, float y, float zoomValue, float difference)
{
	return CGuiView::DoZoomBy(x, y, zoomValue, difference);
}

bool CViewBreakpointsOLD::DoMultiTap(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiTap(touch, x, y);
}

bool CViewBreakpointsOLD::DoMultiMove(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiMove(touch, x, y);
}

bool CViewBreakpointsOLD::DoMultiFinishTap(COneTouchData *touch, float x, float y)
{
	return CGuiView::DoMultiFinishTap(touch, x, y);
}

bool CViewBreakpointsOLD::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return CGuiView::KeyUp(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewBreakpointsOLD::KeyPressed(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return CGuiView::KeyPressed(keyCode, isShift, isAlt, isControl, isSuper);
}

void CViewBreakpointsOLD::ActivateView()
{
	LOGG("CViewBreakpoints::ActivateView()");
	
	prevView = guiMain->currentView;
	
	viewC64->ShowMouseCursor();
}

void CViewBreakpointsOLD::DeactivateView()
{
	LOGG("CViewBreakpoints::DeactivateView()");
}

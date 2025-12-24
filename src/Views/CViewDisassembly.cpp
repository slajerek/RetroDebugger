#include "VID_Main.h"
#include "CViewDisassembly.h"
#include "CColorsTheme.h"
#include "CGuiMain.h"
#include "CDebugDataAdapter.h"
#include "CViewC64.h"
#include "CViewDataMap.h"
#include "SYS_KeyCodes.h"
#include "CDebugInterface.h"
#include "CGuiEditBoxText.h"
#include "C64Tools.h"
#include "CSlrString.h"
#include "CSlrKeyboardShortcuts.h"
#include "C64KeyboardShortcuts.h"
#include "C64SettingsStorage.h"
#include "CViewC64VicDisplay.h"
#include "CViewDataDump.h"
#include "CDebugSymbols.h"
#include "CDebugSymbolsSegment.h"
#include "CDebugSymbolsCodeLabel.h"
#include "CDebugSymbolsDataWatch.h"
#include "CDebugAsmSource.h"
#include "CDebugMemory.h"
#include "CDebugMemoryCell.h"
#include "CLayoutParameter.h"
#include "CMainMenuBar.h"

#include "CViewC64Screen.h"
#include "CViewAtariScreen.h"
#include "CViewNesScreen.h"

#define byte unsigned char

enum editCursorPositions
{
	EDIT_CURSOR_POS_NONE	= -1,
	EDIT_CURSOR_POS_ADDR	= 0,
	EDIT_CURSOR_POS_HEX1,
	EDIT_CURSOR_POS_HEX2,
	EDIT_CURSOR_POS_HEX3,
	EDIT_CURSOR_POS_MNEMONIC,
	EDIT_CURSOR_POS_END
};

#define NUM_MULTIPLY_LINES_FOR_DISASSEMBLY	3


CViewDisassembly::CViewDisassembly(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY,
								   CDebugSymbols *symbols, CViewDataDump *viewDataDump)
: CGuiView(name, posX, posY, posZ, sizeX, sizeY)
{
	imGuiNoWindowPadding = true;
	imGuiNoScrollbar = true;

	this->prevFrameSizeY = 0;
	this->addrPositions = NULL;
	this->numberOfLinesBack = 0;

	this->symbols = symbols;
	this->debugMemory = symbols->memory;
	this->debugInterface = symbols->debugInterface;
	this->dataAdapter = symbols->dataAdapter;
	this->viewDataDump = viewDataDump;
	this->memoryLength = dataAdapter->AdapterGetDataLength();
	this->memory = new uint8[memoryLength];
	
	SetViewParameters(posX, posY, posZ, sizeX, sizeY, viewC64->fontDisassembly, 7.0f, 10, 0.0f, true, true, -1.5f, false, 15);

	this->isTrackingPC = true;
	this->changedByUser = false;
	this->cursorAddress = -1;
	this->currentPC = -1;
	this->fontSize = 7;
	this->numberOfLinesBack = 31;
	this->numberOfLinesBack3 = this->numberOfLinesBack * NUM_MULTIPLY_LINES_FOR_DISASSEMBLY;

	this->showLabels = false;
	this->showArgumentLabels = false;
	
	this->CreateAddrPositions();
	
	this->editCursorPos = EDIT_CURSOR_POS_NONE;
	
	// "wanted" edit cursor pos - to hold pos when moving around with arrow up/down
	this->wantedEditCursorPos = EDIT_CURSOR_POS_NONE;
	
	this->editHex = new CGuiEditHex(this);
	this->editHex->isCapitalLetters = false;

	this->editBoxText = new CGuiEditBoxText(0, 0, 0, 5, 5,
											"", 11, false, this);
	strCodeLine = new CSlrString();
	
	this->isEnteringGoto = false;
	
	// error in code line during assembly?
	this->isErrorCode = false;
	
	// keyboard shortcut zones for this view
	shortcutZones.push_back(KBZONE_DISASSEMBLY);
	shortcutZones.push_back(KBZONE_MEMORY);
	shortcutZones.push_back(KBZONE_GLOBAL);
	
	// opcodes that define "jumps" (JMP, BNE...) to skip label offset printing (i.e. label+1, label+2)
	jumpOpcodes.insert({0x10, 0x10}); // BPL
	jumpOpcodes.insert({0x20, 0x20}); // JSR $aabb
	jumpOpcodes.insert({0x30, 0x30}); // BMI
	jumpOpcodes.insert({0x50, 0x50}); // BVC
	jumpOpcodes.insert({0x70, 0x70}); // BVS
	jumpOpcodes.insert({0x90, 0x90}); // BCC
	jumpOpcodes.insert({0xB0, 0xB0}); // BCS
	jumpOpcodes.insert({0xD0, 0xD0}); // BNE
	jumpOpcodes.insert({0xF0, 0xF0}); // BEQ
	jumpOpcodes.insert({0x4C, 0x4C}); // JMP $aabb
//	jumpOpcodes.insert({0x6C, 0x6C});	// JMP ($aabb)


	AddLayoutParameter(new CLayoutParameterFloat("Font Size", &fontSize));
	AddLayoutParameter(new CLayoutParameterBool("Hex codes", &showHexCodes));
	AddLayoutParameter(new CLayoutParameterBool("Code cycles", &showCodeCycles));
	AddLayoutParameter(new CLayoutParameterBool("Show labels", &showLabels));
	AddLayoutParameter(new CLayoutParameterBool("Show argument labels", &showArgumentLabels));
	AddLayoutParameter(new CLayoutParameterInt("Label num characters", &labelNumCharacters));
	
	// for testing purposes: render execute-aware version of disassembly?
	//c64SettingsExecuteAwareDisassembly = true;
}

CViewDisassembly::~CViewDisassembly()
{
}

void CViewDisassembly::SetViewDataDump(CViewDataDump *viewDataDump)
{
	this->viewDataDump = viewDataDump;
}

void CViewDisassembly::ScrollToAddress(int addr)
{
//	LOGD("CViewDisassembly::ScrollToAddress=%04x", addr);

	this->cursorAddress = addr;
	this->isTrackingPC = false;
}

void CViewDisassembly::FinalizeEditing()
{
	if (editCursorPos == EDIT_CURSOR_POS_ADDR)
	{
		editHex->UpdateValue();
		// 	instead of ScrollToAddress(editHex->value), we move user there so can get back with arrow-left
		MoveAddressHistoryForwardWithAddr(editHex->value);
	}
	else if (editCursorPos >= EDIT_CURSOR_POS_HEX1 && editCursorPos <= EDIT_CURSOR_POS_HEX3)
	{
		editHex->UpdateValue();
		bool isAvailable;
		int cp = editCursorPos-EDIT_CURSOR_POS_HEX1;
		dataAdapter->AdapterWriteByte(cursorAddress+cp, editHex->value, &isAvailable);
	}
	else if (editCursorPos == EDIT_CURSOR_POS_MNEMONIC)
	{
		Assemble(this->cursorAddress);
	}
}

void CViewDisassembly::GuiEditHexEnteredValue(CGuiEditHex *editHex, u32 lastKeyCode, bool isCancelled)
{
	// finished editing from editbox
	guiMain->LockMutex();

	isTrackingPC = false;
	changedByUser = true;
	
	if (editCursorPos == EDIT_CURSOR_POS_ADDR)
	{
		FinalizeEditing();
		
		if (lastKeyCode == MTKEY_ARROW_LEFT)
		{
			wantedEditCursorPos = EDIT_CURSOR_POS_ADDR;
			guiMain->UnlockMutex();
			return;
		}
		
		if (isEnteringGoto == true)
		{
			// goto & finish
			editCursorPos = EDIT_CURSOR_POS_NONE;
			wantedEditCursorPos = EDIT_CURSOR_POS_NONE;
		}
		else
		{
			// continue editing
			wantedEditCursorPos = editCursorPos+1;
			StartEditingAtCursorPosition(wantedEditCursorPos, false);
		}
	}
	else if (editCursorPos >= EDIT_CURSOR_POS_HEX1 && editCursorPos <= EDIT_CURSOR_POS_HEX3)
	{
		FinalizeEditing();
		
		if (lastKeyCode == MTKEY_ARROW_LEFT)
		{
			wantedEditCursorPos = editCursorPos-1;
			StartEditingAtCursorPosition(wantedEditCursorPos, true);
		}
		else
		{
			wantedEditCursorPos = editCursorPos+1;
			StartEditingAtCursorPosition(wantedEditCursorPos, false);
		}
	}
	
	if (lastKeyCode == MTKEY_ENTER)
	{
		isEnteringGoto = false;
		wantedEditCursorPos = EDIT_CURSOR_POS_NONE;
		editCursorPos = EDIT_CURSOR_POS_NONE;
	}
	
	guiMain->UnlockMutex();
}

void CViewDisassembly::EditBoxTextValueChanged(CGuiEditBoxText *editBox, char *text)
{
	strCodeLine->Set(editBoxText->textBuffer);
	strCodeLine->Concatenate(' ');
	u16 chr = strCodeLine->GetChar(editBoxText->currentPos);
	chr += CBMSHIFTEDFONT_INVERT;
	strCodeLine->SetChar(editBoxText->currentPos, chr);
}

void CViewDisassembly::EditBoxTextFinished(CGuiEditBoxText *editBox, char *text)
{
	//LOGD("CViewDisassembly::EditBoxTextFinished");
	
	int l = strlen(editBoxText->textBuffer);
	if (l == 0)
	{
		// finish editing
		editCursorPos = EDIT_CURSOR_POS_NONE;
		return;
	}
	
	guiMain->LockMutex();
	debugInterface->LockMutex();
	
	FinalizeEditing();
	
	if (isErrorCode == false)
	{
		// scroll down:
		bool isAvailable;
		uint8 op;
		dataAdapter->AdapterReadByte(this->cursorAddress, &op, &isAvailable);
		
		this->cursorAddress += opcodes[op].addressingLength;

		UpdateDisassembly(this->cursorAddress, this->cursorAddress + 0x0100);
	
		// start editing next mnemonic
		StartEditingAtCursorPosition(EDIT_CURSOR_POS_MNEMONIC, true);
	}
	
	debugInterface->UnlockMutex();
	guiMain->UnlockMutex();
}

void CViewDisassembly::SetPosition(float posX, float posY, float sizeX, float sizeY)
{
	LayoutParameterChanged(NULL);
}

void CViewDisassembly::SetViewParameters(float posX, float posY, float posZ, float sizeX, float sizeY, CSlrFont *font,
										 float fontSize, int numberOfLines,
					   float mnemonicsDisplayOffsetX,
					   bool showHexCodes,
					   bool showCodeCycles, float codeCyclesDisplayOffsetX,
					   bool showLabels, int labelNumCharacters)
{
	CGuiView::SetPosition(posX, posY, posZ, sizeX, sizeY);
	
	this->fontDisassembly = font;
	this->fontSize = fontSize;
	this->mnemonicsDisplayOffsetX = mnemonicsDisplayOffsetX;
	this->mnemonicsOffsetX = fontSize*mnemonicsDisplayOffsetX;
	this->codeCyclesDisplayOffsetX = codeCyclesDisplayOffsetX;
	this->codeCyclesOffsetX = fontSize*codeCyclesDisplayOffsetX;
	this->fontSize3 = fontSize*3;
	this->fontSize5 = fontSize*5;
	this->fontSize9 = fontSize*9;
	this->numberOfLinesBack = numberOfLines/2;
	this->numberOfLinesBack3 = this->numberOfLinesBack * NUM_MULTIPLY_LINES_FOR_DISASSEMBLY;
	
	CreateAddrPositions();
	
	this->showHexCodes = showHexCodes;
	this->showCodeCycles = showCodeCycles;
	this->showLabels = showLabels;
	this->labelNumCharacters = labelNumCharacters;
}

void CViewDisassembly::LayoutParameterChanged(CLayoutParameter *layoutParameter)
{
//	LOGD("CViewDisassembly::LayoutParameterChanged");
	if (!showCodeCycles)
	{
		if (showHexCodes)
		{
			this->mnemonicsOffsetX = -fontSize;
		}
		else
		{
			this->mnemonicsOffsetX = fontSize * mnemonicsDisplayOffsetX;
		}
	}
	else
	{
		if (showHexCodes)
		{
			this->codeCyclesOffsetX = fontSize * codeCyclesDisplayOffsetX/2.0f - fontSize*0.5f;
			this->mnemonicsOffsetX = 0; //-fontSize * 0.5f;
		}
		else
		{
			this->codeCyclesOffsetX = fontSize * codeCyclesDisplayOffsetX/2.0f;
			this->mnemonicsOffsetX = fontSize * 0.5f;
		}
	}
	this->fontSize3 = fontSize*3;
	this->fontSize5 = fontSize*5;
	this->fontSize9 = fontSize*9;

	UpdateNumberOfVisibleLines();
}

void CViewDisassembly::UpdateNumberOfVisibleLines()
{
	prevFrameSizeY = sizeY;
	
	float numberOfLines = sizeY/fontSize;
	this->numberOfLinesBack = numberOfLines/2;
	this->numberOfLinesBack3 = this->numberOfLinesBack * NUM_MULTIPLY_LINES_FOR_DISASSEMBLY;
	
	CreateAddrPositions();
}

void CViewDisassembly::SetCurrentPC(int pc)
{
	this->currentPC = pc;
}


void CViewDisassembly::UpdateLocalMemoryCopy(int startAddress, int endAddress)
{
	int beginAddress = startAddress - numberOfLinesBack3;

	//LOGD("UpdateLocalMemoryCopy: %04x %04x (size=%04x)", beginAddress, endAddress, endAddress-beginAddress);

	if (beginAddress < 0)
	{
		beginAddress = 0;
	}
	
	dataAdapter->AdapterReadBlockDirect(memory, beginAddress, endAddress);
}

//
// * Count roll up *
//
// Idea here is that we iteratively try to find an address from where we can start rendering line by line
// op by op, that will 'hit' the requested code startAddress, so we *should* get proper roll up. This is
// needed to avoid situation if an op just before the startAddress (one line up) contains 3 bytes, and 2 address
// bytes contain hex data that resembles a correct op.
//
// HOWEVER in some circumstances the backwards-started code does not 'hit' proper address
//         to fix this we need a workaround when such situation is detected then simply a classic back-disassembly
//         is done (just analyse -2, -3 bytes per line and render lines backwards)
//
void CViewDisassembly::CalcDisassemblyStart(int startAddress, int *newStart, int *renderLinesBefore)
{
	//	LOGD("====================================== CalcDisassemblyStart startAddress=%4.4x", startAddress);
	
	uint8 opcode;
	
	int newAddress = startAddress - numberOfLinesBack3;	// numLines*3
	if (newAddress < 0)
		newAddress = 0;
	
	int numRenderLines = 0;
	
	bool found = false;
	while(newAddress < startAddress)
	{
		//		LOGD("newAddress=%4.4x", newAddress);
		
		int checkAddress = newAddress;
		
		numRenderLines = 0;
		
		// scroll down
		while (true)
		{
			int addr = checkAddress;
			
			//			LOGD("  checkAddress=%4.4x", adr);

			// check if cells marked as execute

			// +0
			CDebugMemoryCell *cell0 = debugMemory->GetMemoryCell( addr % memoryLength );
			if (cell0)
			{
				if (cell0->isExecuteCode)
				{
					opcode = memory[addr % memoryLength];
					checkAddress += opcodes[opcode].addressingLength;
					numRenderLines++;
				}
				else
				{
					// +1
					CDebugMemoryCell *cell1 = debugMemory->GetMemoryCell( (addr+1) % memoryLength );
					if (cell1)
					{
						if (cell1->isExecuteCode)
						{
							checkAddress += 1;
							numRenderLines++;  // just hex code or 1-length opcode
							
							opcode = memory[ (checkAddress) % memoryLength];
							checkAddress += opcodes[opcode].addressingLength;
							numRenderLines++;
						}
						else
						{
							// +2
							CDebugMemoryCell *cell2 = debugMemory->GetMemoryCell( (addr+2) % memoryLength );
							if (cell2)
							{
								if (cell2->isExecuteCode)
								{
									// check if at addr is 2-length opcode
									opcode = memory[ (checkAddress) % memoryLength];
									if (opcodes[opcode].addressingLength == 2)
									{
										checkAddress += 2;
										numRenderLines++;  // 2-length opcode
									}
									else
									{
										checkAddress += 2;
										numRenderLines++;  // just 1st hex code
										numRenderLines++;  // just 2nd hex code
									}
									
									opcode = memory[ (checkAddress) % memoryLength];
									checkAddress += opcodes[opcode].addressingLength;
									numRenderLines++;
								}
								else
								{
									if (cell0->isExecuteArgument == false)
									{
										// execute not found
										opcode = memory[addr % memoryLength];
										checkAddress += opcodes[opcode].addressingLength;
										numRenderLines++;
									}
									else
									{
										// render hex
										checkAddress += 1;
										numRenderLines++;
									}
								}
							}
							else
							{
								numRenderLines++;
							}
						}
					} // cell1
					else
					{
						numRenderLines++;
					}
					
				}
			} // cell0
			else
			{
				numRenderLines++;
			}
			
			
			
			
			//			LOGD("  new checkAddress=%4.4x", adr);
			
			if (checkAddress >= startAddress)
			{
				//				LOGD("  ... checkAddress=%4.4x >= startAddress=%4.4x", checkAddress, startAddress);
				break;
			}
		}
		
		//		LOGD("checkAddress=%4.4x == startAddress=%4.4x?", checkAddress, startAddress);
		if (checkAddress == startAddress)
		{
			//LOGD("!! found !! newAddress=%4.4x numRenderLines=%d", newAddress, numRenderLines);
			found = true;
			break;
		}
		
		newAddress += 1;
		//
		//		LOGD("not found, newAddress=%4.4x", newAddress);
	}
	
	if (!found)
	{
		//
		//LOGD("*** FAILED ***");
		newAddress = startAddress; // - (float)numLines*1.5f;
		numRenderLines = 0;
		
		//viewC64->fontDisassembly->BlitText("***FAILED***", 100, 300, -1, 20);
	}
//	else
//	{
////		LOGD("!!! FOUND !!!");
//	}
	
	*newStart = newAddress;
	*renderLinesBefore = numRenderLines;
}


void CViewDisassembly::RenderDisassembly(int startAddress, int endAddress)
{
	//LOGTODO("glasnost: 3c03");
	
	bool done = false;
	short i;
	uint8 opcode;
	uint8 op[3];
	uint16 addr;
	
	float px = posX;
	float py = posY;
	
	float pEndY = posEndY + 0.1f;
	
	int renderAddress = startAddress;
	int renderLinesBefore;

	if (startAddress < 0)
		startAddress = 0;
	if (endAddress > 0x10000)
		endAddress = 0x10000;
	
	UpdateLocalMemoryCopy(startAddress, endAddress);
	
	
	CalcDisassemblyStart(startAddress, &renderAddress, &renderLinesBefore);
	
	
	//LOGD("startAddress=%4.4x numberOfLinesBack=%d | renderAddress=%4.4x  renderLinesBefore=%d", startAddress, numberOfLinesBack, renderAddress, renderLinesBefore);
	
	renderSkipLines = numberOfLinesBack - renderLinesBefore;
	int skipLines = renderSkipLines;

	{
		py += (float)(skipLines) * fontSize;
	}


	startRenderY = py;
	startRenderAddr = renderAddress;
	endRenderAddr = endAddress;

	//
	renderStartAddress = startAddress;

	
	if (renderLinesBefore == 0)
	{
		previousOpAddr = startAddress - 1;
	}
	
	do
	{
		//LOGD("renderAddress=%4.4x l=%4.4x", renderAddress, memoryLength);
		if (renderAddress >= memoryLength)
			break;
		
		if (renderAddress == memoryLength-1)
		{
			RenderHexLine(px, py, renderAddress);
			break;
		}
		
		addr = renderAddress;
		
		for (i=0; i<3; i++, addr++)
		{
			if (addr == endAddress)
			{
				done = true;
			}
			
			op[i] = memory[addr];
		}
		
		{
			addr = renderAddress;
			
			// +0
			CDebugMemoryCell *cell0 = debugMemory->GetMemoryCell(addr);
			if (cell0)
			{
				if (cell0->isExecuteCode)
				{
					opcode = memory[addr];
					renderAddress += RenderDisassemblyLine(px, py, renderAddress, op[0], op[1], op[2]);
					py += fontSize;
				}
				else
				{
					// +1
					CDebugMemoryCell *cell1 = debugMemory->GetMemoryCell( addr+1 );
					if (cell1)
					{
						if (cell1->isExecuteCode)
						{
							// check if at addr is 1-length opcode
							opcode = memory[ (renderAddress) ];
							if (opcodes[opcode].addressingLength == 1)
							{
								RenderDisassemblyLine(px, py, renderAddress, op[0], op[1], op[2]);
							}
							else
							{
								RenderHexLine(px, py, renderAddress);
							}

							renderAddress += 1;
							py += fontSize;
							
							addr = renderAddress;
							for (i=0; i<3; i++, addr++)
							{
								if (addr == endAddress)
								{
									done = true;
								}
								
								op[i] = memory[addr];
							}
							
							opcode = memory[ (renderAddress) ];
							renderAddress += RenderDisassemblyLine(px, py, renderAddress, op[0], op[1], op[2]);
							py += fontSize;
						}
						else
						{
							// +2
							CDebugMemoryCell *cell2 = debugMemory->GetMemoryCell( addr+2 );
							if (cell2)
							{
								if (cell2->isExecuteCode)
								{
									// check if at addr is 2-length opcode
									opcode = memory[ (renderAddress) ];
									if (opcodes[opcode].addressingLength == 2)
									{
										renderAddress += RenderDisassemblyLine(px, py, renderAddress, op[0], op[1], op[2]);
										py += fontSize;
									}
									else
									{
										RenderHexLine(px, py, renderAddress);
										renderAddress += 1;
										py += fontSize;

										RenderHexLine(px, py, renderAddress);
										renderAddress += 1;
										py += fontSize;
									}
									
									addr = renderAddress;
									for (i=0; i<3; i++, addr++)
									{
										if (addr == endAddress)
										{
											done = true;
										}
										
										op[i] = memory[addr];
									}

									opcode = memory[ (renderAddress) ];
									renderAddress += RenderDisassemblyLine(px, py, renderAddress, op[0], op[1], op[2]);
									py += fontSize;
								}
								else
								{
									if (cell0->isExecuteArgument == false)
									{
										// execute not found, just render line
										renderAddress += RenderDisassemblyLine(px, py, renderAddress, op[0], op[1], op[2]);
										py += fontSize;
									}
									else
									{
										// it is argument
										RenderHexLine(px, py, renderAddress);
										renderAddress++;
										py += fontSize;
									}
								}
							}	// cell2
							else
							{
								py += fontSize;
							}
						}
					} // cell1
					else
					{
						py += fontSize;
					}
				}
			} // cell0
			else
			{
				py += fontSize;
			}


			if (py > pEndY)
				break;
			
		}
	}
	while (!done);
	
	// disassemble up?
	int length;

	if (skipLines > 0)
	{
		//LOGD("disassemble UP: %04x y=%5.2f", startRenderAddr, startRenderY);
		
		py = startRenderY;
		renderAddress = startRenderAddr;
		
		while (skipLines > 0)
		{
			py -= fontSize;
			skipLines--;
			
			if (renderAddress < 0)
				break;
			
			// check how much scroll up
			byte op, lo, hi;
			
			// TODO: generalise this and do more than -3  + check executeArgument!
			// check -3
			if (renderAddress > 2)
			{
				// check execute markers first
				int addr;
				
				addr = renderAddress-1;
				CDebugMemoryCell *cell1 = debugMemory->GetMemoryCell(addr);
				if (cell1->isExecuteCode)
				{
					op = memory[addr];
					if (opcodes[op].addressingLength == 1)
					{
						RenderDisassemblyLine(px, py, addr, op, 0x00, 0x00);
						renderAddress = addr;
						continue;
					}
					
					RenderHexLine(px, py, addr);
					renderAddress = addr;
					continue;
				}
				
				addr = renderAddress-2;
				CDebugMemoryCell *cell2 = debugMemory->GetMemoryCell(addr);
				if (cell2->isExecuteCode)
				{
					op = memory[addr];
					if (opcodes[op].addressingLength == 2)
					{
						lo = memory[renderAddress-1];
						RenderDisassemblyLine(px, py, addr, op, lo, 0x00);
						renderAddress = addr;
						continue;
					}
					
					renderAddress--;
					RenderHexLine(px, py, renderAddress);
					py -= fontSize;
					skipLines--;
					renderAddress--;
					RenderHexLine(px, py, renderAddress);
					continue;
				}
				
				addr = renderAddress-3;
				CDebugMemoryCell *cell3 = debugMemory->GetMemoryCell(addr);
				if (cell3->isExecuteCode)
				{
					op = memory[addr];
					int opLen = opcodes[op].addressingLength;
					if (opLen == 3)
					{
						lo = memory[renderAddress-2];
						hi = memory[renderAddress-1];
						RenderDisassemblyLine(px, py, addr, op, lo, hi);
						renderAddress = addr;
						continue;
					}
					else if (opLen == 2)
					{
						RenderHexLine(px, py, renderAddress-1);

						py -= fontSize;
						skipLines--;
						
						lo = memory[renderAddress-2];
						RenderDisassemblyLine(px, py, addr, op, lo, 0x00);
						
						renderAddress = addr;
						continue;
					}
					
					renderAddress--;
					RenderHexLine(px, py, renderAddress);
					py -= fontSize;
					skipLines--;

					renderAddress--;
					RenderHexLine(px, py, renderAddress);
					py -= fontSize;
					skipLines--;

					renderAddress--;
					RenderHexLine(px, py, renderAddress);
					continue;
				}
				
				if (cell1->isExecuteArgument == false
					&& cell2->isExecuteArgument == false
					&& cell3->isExecuteArgument == false)
				{
					//
					// then check normal -3
					op = memory[renderAddress-3];
					length = opcodes[op].addressingLength;
					
					if (length == 3)
					{
						lo = memory[renderAddress-2];
						hi = memory[renderAddress-1];
						RenderDisassemblyLine(px, py, renderAddress-3, op, lo, hi);
						
						renderAddress -= 3;
						continue;
					}
				}
			}
			
			// check -2
			if (renderAddress > 1)
			{
				CDebugMemoryCell *cell1 = debugMemory->GetMemoryCell(renderAddress-1);
				CDebugMemoryCell *cell2 = debugMemory->GetMemoryCell(renderAddress-2);
				if (cell1->isExecuteArgument == false
					&& cell2->isExecuteArgument == false)
				{
					op = memory[renderAddress-2];
					
					length = opcodes[op].addressingLength;
					
					if (length == 2)
					{
						lo = memory[renderAddress-1];
						RenderDisassemblyLine(px, py, renderAddress-2, op, lo, lo);
						
						renderAddress -= 2;
						continue;
					}
				}
			}
			
			// check -1
			if (renderAddress > 0)
			{
				CDebugMemoryCell *cell1 = debugMemory->GetMemoryCell(renderAddress-1);

				if (cell1->isExecuteArgument == false)
				{
					op = memory[renderAddress-1];
					length = opcodes[op].addressingLength;
					
					if (length == 1)
					{
						RenderDisassemblyLine(px, py, renderAddress-1, op, 0x00, 0x00);
						
						renderAddress -= 1;
						continue;
					}
				}
			}
			
			// not found compatible op, just render hex
			if (renderAddress > 0)
			{
				renderAddress -= 1;
				RenderHexLine(px, py, renderAddress);
			}
		}
	}
	
	
	// this is in the center - show cursor
	if (isTrackingPC == false)
	{
		py = numberOfLinesBack * fontSize + posY;
		BlitRectangle(px, py, -1.0f, sizeX*4.0f, fontSize, 0.3, 1.0, 0.3, 0.5f, 0.7f);
	}
	
//	LOGD("previousOpAddr=%4.4x nextOpAddr=%4.4x", previousOpAddr, nextOpAddr);
	
}

const float breakpointActiveColorR = 0.78f; const float breakpointActiveColorG = 0.07f; const float breakpointActiveColorB = 0.07f; const float breakpointActiveColorA = 0.7f;

const float breakpointNotActiveColorR = 0.28f; const float breakpointNotActiveColorG = 0.07f; const float breakpointNotActiveColorB = 0.07f; const float breakpointNotActiveColorA = 0.7f;

// Disassemble one instruction, return length
int CViewDisassembly::RenderDisassemblyLine(float px, float py, int addr, uint8 op, uint8 lo, uint8 hi)
{
//	LOGD("adr=%4.4x op=%2.2x", adr, op);
	
	float colorExecuteR, colorExecuteG, colorExecuteB;
	float colorExecuteA = 1.0f;
	float colorNonExecuteR, colorNonExecuteG, colorNonExecuteB;
	float colorNonExecuteA = 1.0f;
	
	GetColorsFromScheme(c64SettingsDisassemblyExecuteColor, &colorExecuteR, &colorExecuteG, &colorExecuteB);
	GetColorsFromScheme(c64SettingsDisassemblyNonExecuteColor, &colorNonExecuteR, &colorNonExecuteG, &colorNonExecuteB);
	
	char *buf = SYS_GetCharBuf();
	char buf1[16];
	char buf2[2] = {0};
	char buf3[16];
	char buf4[16];
	char bufHexCodes[16];
	int length;
	
	if (symbols->currentSegment == NULL)
	{
		return 0;
	}

	CDebugBreakpointsAddr *breakpoints = symbols->currentSegment->breakpointsPC;
	
	std::map<int, CDebugBreakpointAddr *>::iterator it = breakpoints->renderBreakpoints.find(addr);
	if (it != breakpoints->renderBreakpoints.end())
	{
		CDebugBreakpointAddr *breakpoint = it->second;
		
		// is active?
		if (breakpoint->isActive)
		{
			BlitFilledRectangle(posX, py, -1.0f,
								sizeX, fontSize, breakpointActiveColorR, breakpointActiveColorG, breakpointActiveColorB, breakpointActiveColorA);
		}
		else
		{
			BlitFilledRectangle(posX, py, -1.0f,
								sizeX, fontSize, breakpointNotActiveColorR, breakpointNotActiveColorG, breakpointNotActiveColorB, breakpointNotActiveColorA);
		}
	}

	CDebugMemoryCell *cell = debugMemory->GetMemoryCell(addr);
	
	float cr, cg, cb, ca;
	
	if (cell->isExecuteCode)
	{
		cr = colorExecuteR;
		cg = colorExecuteG;
		cb = colorExecuteB;
		ca = colorExecuteA;
	}
	else
	{
		cr = colorNonExecuteR;
		cg = colorNonExecuteG;
		cb = colorNonExecuteB;
		ca = colorNonExecuteA;
	}
	
	length = opcodes[op].addressingLength;

//	LOGD("showLabels=%d symbols=%x symbols->currentSegment=%x", showLabels, symbols, symbols->currentSegment);
	if (showLabels && symbols->currentSegment)
	{
		CDebugSymbolsSegment *segment = symbols->currentSegment;
		
		px += disassembledCodeOffsetX;
		
		float pxe = this->posX + fontSize*labelNumCharacters;
		// TODO: add labels as arguments
		CDebugSymbolsCodeLabel *label = segment->FindLabel(addr);
		if (label)
		{
			// found a label, display it i
			char *labelText = label->GetLabelText();
			int l = strlen(labelText)+1;
			float labelX = pxe - l*fontSize;

			fontDisassembly->BlitTextColor(labelText, labelX, py, -1, fontSize, cr, cg, cb, ca);
		}
	}
	
	// addr
	int len = 0;
	if (editCursorPos != EDIT_CURSOR_POS_ADDR)
	{
		dataAdapter->GetAddressStringForCell(addr, buf, MAX_STRING_LENGTH);
		len = strlen(buf);
		fontDisassembly->BlitTextColor(buf, px, py, -1, fontSize, cr, cg, cb, ca);
	}
	else
	{
		if (addr == this->cursorAddress)
		{
			// TODO: fixme and replace with editbox from adapter and length
			dataAdapter->GetAddressStringForCell(addr, buf, MAX_STRING_LENGTH);
			len = strlen(buf);

			fontDisassembly->BlitTextColor(editHex->textWithCursor, px, py, posZ, fontSize, cr, cg, cb, ca);
		}
		else
		{
			dataAdapter->GetAddressStringForCell(addr, buf, MAX_STRING_LENGTH);
			len = strlen(buf);
			fontDisassembly->BlitTextColor(buf, px, py, -1, fontSize, cr, cg, cb, ca);
		}
	}
	
	px += ((float)(len+1))*fontSize; //fontSize5;


	if (showHexCodes)
	{
		// check if editing
		if (addr == this->cursorAddress &&
			(editCursorPos == EDIT_CURSOR_POS_HEX1 || editCursorPos == EDIT_CURSOR_POS_HEX2
			 || editCursorPos == EDIT_CURSOR_POS_HEX3))
		{
			// Display instruction bytes in hex
			byte o[3];
			o[0] = op;
			o[1] = lo;
			o[2] = hi;
			
			float rx = px;
			int cp = editCursorPos-EDIT_CURSOR_POS_HEX1;
			
			for (int i = 0; i < length; i++)
			{
				if (cp == i)
				{
					fontDisassembly->BlitTextColor(editHex->textWithCursor, rx, py, posZ, fontSize, cr, cg, cb, ca);
				}
				else
				{
					sprintfHexCode8(buf1, o[i]);
					fontDisassembly->BlitTextColor(buf1, rx, py, posZ, fontSize, cr, cg, cb, ca);
				}
				
				rx += fontSize3;
			}
		}
		else
		{
			strcpy(buf1, "         ");
			
			switch (length)
			{
				case 1:
					//sprintf(buf1, "%2.2x       ", op);
					// "xx       "
					sprintfHexCode8WithoutZeroEnding(buf1, op);
					break;
					
				case 2:
					//sprintf(buf1, "%2.2x %2.2x    ", op, lo);
					// "xx xx    "
					sprintfHexCode8WithoutZeroEnding(buf1, op);
					sprintfHexCode8WithoutZeroEnding(buf1+3, lo);
					break;
					
				case 3:
					//sprintf(buf1, "%2.2x %2.2x %2.2x ", op, lo, hi);
					// "xx xx xx "
					sprintfHexCode8WithoutZeroEnding(buf1, op);
					sprintfHexCode8WithoutZeroEnding(buf1+3, lo);
					sprintfHexCode8WithoutZeroEnding(buf1+6, hi);
					break;
			}
			
			strcpy(bufHexCodes, buf1);
			strcat(bufHexCodes, buf2);
			fontDisassembly->BlitTextColor(bufHexCodes, px, py, posZ, fontSize, cr, cg, cb, ca);
		}

		px += fontSize9;
		
		// illegal opcode?
		if (opcodes[op].isIllegal == OP_ILLEGAL)
		{
			fontDisassembly->BlitTextColor("*", px, py, posZ, fontSize, cr, cg, cb, ca);
		}
		
		px += fontSize;
	}

	if (showCodeCycles)
	{
		const float cfNot = 0.7f;
		const float cfActive = 1.3f;
		
		float px2 = px + this->codeCyclesOffsetX;
		
		if (opcodes[op].addressingMode != ADDR_REL)
		{
			sprintfHexCode4(buf3, opcodes[op].numCycles);
			fontDisassembly->BlitTextColor(buf3, px2, py, -1, fontSize, cr*cfNot, cg*cfNot, cb*cfActive, ca);
		}
		else
		{
			// check page boundary
			u16 baddr = ((addr + 2) + (int8)lo) & 0xFFFF;
			
			if ((addr & 0xFF00) == (baddr & 0xFF00))
			{
				sprintfHexCode4(buf3, opcodes[op].numCycles);
				fontDisassembly->BlitTextColor(buf3, px2, py, -1, fontSize, cr*cfNot, cg*cfNot, cb*cfActive, ca);
			}
			else
			{
				sprintfHexCode4(buf3, opcodes[op].numCycles + 1);
				fontDisassembly->BlitTextColor(buf3, px2, py, -1, fontSize, cr*cfActive, cg*cfNot, cb*cfNot, ca);
			}
		}
		
	}

	// convert op to text
	MnemonicWithArgumentToStr(addr, op, lo, hi, buf);
	
	px += mnemonicsOffsetX;
	
	if (editCursorPos != EDIT_CURSOR_POS_MNEMONIC)
	{
		fontDisassembly->BlitTextColor(buf, px, py, -1, fontSize, cr, cg, cb, ca);
	}
	else
	{
		if (addr == this->cursorAddress)
		{
			if (editBoxText->textBuffer[0] == 0x00)
			{
				fontDisassembly->BlitTextColor(strCodeLine, px, py, -1, fontSize,
											   colorExecuteR,
											   colorExecuteG,
											   colorExecuteB,
											   colorExecuteA);
				fontDisassembly->BlitTextColor(buf, px, py, -1, fontSize, cr*0.5f, cg*0.5f, cb*0.5f, ca*0.5f);
			}
			else
			{
				fontDisassembly->BlitTextColor(strCodeLine, px, py, -1, fontSize,
											   colorExecuteR,
											   colorExecuteG,
											   colorExecuteB,
											   colorExecuteA);

			}
		}
		else
		{
			fontDisassembly->BlitTextColor(buf, px, py, -1, fontSize, cr, cg, cb, ca);
		}
	}

	if (addr == currentPC)
	{
		BlitFilledRectangle(posX, py, -1.0f,
							sizeX, fontSize, cr, cg, cb, 0.3f);
	}

	int numBytesPerOp = opcodes[op].addressingLength;

	if (c64SettingsExecuteAwareDisassembly)
	{
		int newAddress = addr + numBytesPerOp;
		if (newAddress == renderStartAddress)
		{
			previousOpAddr = addr;
			//		LOGD("(M) previousOpAddr=%04x", previousOpAddr);
		}
		
		if (addr == renderStartAddress)
		{
			nextOpAddr = addr+numBytesPerOp;
			//		LOGD("(M) nextOpAddr=%04x", nextOpAddr);
		}
	}
	
	SYS_ReleaseCharBuf(buf);
	
	return numBytesPerOp;
	
}

bool CViewDisassembly::FindLabelText(u8 op, u16 addr, char *buf)
{
	if (c64SettingsDisassemblyUseNearLabels)
	{
		if (c64SettingsDisassemblyUseNearLabelsForJumps)
		{
			return symbols->currentSegment->FindNearLabelText(addr, buf);
		}
		else
		{
			// check if jump (JMP, BNE, ...)
			if (jumpOpcodes.find(op) == jumpOpcodes.end())
			{
				// opcode is not jump
				return symbols->currentSegment->FindNearLabelText(addr, buf);
			}
		}
	}
	return symbols->currentSegment->FindLabelText(addr, buf);
}

void CViewDisassembly::MnemonicWithArgumentToStr(u16 addr, u8 op, u8 lo, u8 hi, char *buf)
{
	char buf3[16];
	char buf4[512];		// TODO: max label text...
	char labelText[512];
	
	// mnemonic
	strcpy(buf3, opcodes[op].name);
	strcat(buf3, " ");

	bool showArgumentLabel = showArgumentLabels && symbols->currentSegment;
	CDebugSymbolsCodeLabel *label;
	
		buf4[0] = 0;
		switch (opcodes[op].addressingMode)
		{
			case ADDR_IMP:
				break;
				
			case ADDR_IMM:
				//sprintf(buf4, "#%2.2x", lo);
				buf4[0] = '#';
				sprintfHexCode8(buf4+1, lo);
				break;
				
			case ADDR_ZP:
				//sprintf(buf4, "%2.2x", lo);
				if (showArgumentLabel && FindLabelText(op, lo, buf4))
				{
				}
				else
				{
					sprintfHexCode8(buf4, lo);
				}
				break;
				
			case ADDR_ZPX:
				//sprintf(buf4, "%2.2x,x", lo);
				if (showArgumentLabel && FindLabelText(op, lo, labelText))
				{
					sprintf(buf4, "%s,x", labelText);
				}
				else
				{
					sprintfHexCode8WithoutZeroEnding(buf4, lo);
					buf4[2] = ',';
					buf4[3] = 'x';
					buf4[4] = 0x00;
				}

				break;
				
			case ADDR_ZPY:
				//sprintf(buf4, "%2.2x,y", lo);
				if (showArgumentLabel && FindLabelText(op, lo, labelText))
				{
					sprintf(buf4, "%s,y", labelText);
				}
				else
				{
					sprintfHexCode8WithoutZeroEnding(buf4, lo);
					buf4[2] = ',';
					buf4[3] = 'y';
					buf4[4] = 0x00;
				}
				break;
				
			case ADDR_IZX:
				//sprintf(buf4, "(%2.2x,x)", lo);
				if (showArgumentLabel && FindLabelText(op, lo, labelText))
				{
					sprintf(buf4, "(%s,x)", labelText);
				}
				else
				{
					buf4[0] = '(';
					sprintfHexCode8WithoutZeroEnding(buf4+1, lo);
					buf4[3] = ',';
					buf4[4] = 'x';
					buf4[5] = ')';
					buf4[6] = 0x00;
				}
				break;
				
			case ADDR_IZY:
				//sprintf(buf4, "(%2.2x),y", lo);
				if (showArgumentLabel && FindLabelText(op, lo, labelText))
				{
					sprintf(buf4, "(%s),y", labelText);
				}
				else
				{
					buf4[0] = '(';
					sprintfHexCode8WithoutZeroEnding(buf4+1, lo);
					buf4[3] = ')';
					buf4[4] = ',';
					buf4[5] = 'y';
					buf4[6] = 0x00;
				}
				break;
				
			case ADDR_ABS:
				//sprintf(buf4, "%4.4x", (hi << 8) | lo);
				if (showArgumentLabel && FindLabelText(op, (hi << 8) | lo, labelText))
				{
					strcpy(buf4, labelText);
				}
				else
				{
					sprintfHexCode8WithoutZeroEnding(buf4, hi);
					sprintfHexCode8(buf4+2, lo);
				}

				break;
				
			case ADDR_ABX:
				//sprintf(buf4, "%4.4x,x", (hi << 8) | lo);
				if (showArgumentLabel && FindLabelText(op, (hi << 8) | lo, labelText))
				{
					sprintf(buf4, "%s,x", labelText);
				}
				else
				{
					sprintfHexCode8WithoutZeroEnding(buf4, hi);
					sprintfHexCode8WithoutZeroEnding(buf4+2, lo);
					buf4[4] = ',';
					buf4[5] = 'x';
					buf4[6] = 0x00;
				}

				break;
				
			case ADDR_ABY:
				//sprintf(buf4, "%4.4x,y", (hi << 8) | lo);
				if (showArgumentLabel && FindLabelText(op, (hi << 8) | lo, labelText))
				{
					sprintf(buf4, "%s,y", labelText);
				}
				else
				{
					sprintfHexCode8WithoutZeroEnding(buf4, hi);
					sprintfHexCode8WithoutZeroEnding(buf4+2, lo);
					buf4[4] = ',';
					buf4[5] = 'y';
					buf4[6] = 0x00;
				}

				break;
				
			case ADDR_IND:
				//sprintf(buf4, "(%4.4x)", (hi << 8) | lo);
				if (showArgumentLabel && FindLabelText(op, (hi << 8) | lo, labelText))
				{
					sprintf(buf4, "(%s)", labelText);
				}
				else
				{
					buf4[0] = '(';
					sprintfHexCode8WithoutZeroEnding(buf4+1, hi);
					sprintfHexCode8WithoutZeroEnding(buf4+3, lo);
					buf4[5] = ')';
					buf4[6] = 0x00;
				}

				break;
				
			case ADDR_REL:
				//sprintf(buf4, "%4.4x", ((addr + 2) + (int8)lo) & 0xFFFF);
				if (showArgumentLabel && FindLabelText(op, ((addr + 2) + (int8)lo) & 0xFFFF, labelText))
				{
					sprintf(buf4, "%s", labelText);
				}
				else
				{
					sprintfHexCode16(buf4, ((addr + 2) + (int8)lo) & 0xFFFF);
				}
				break;
			default:
				break;
		}

		//sprintf(buf, "%s%s", buf3, buf4);
		strcpy(buf, buf3);
		strcat(buf, buf4);
	
	/*
	{
		switch (opcodes[op].addressingMode)
		{
			case ADDR_IMP:
				sprintf(buf4, "");
				break;
				
			case ADDR_IMM:
				//sprintf(buf4, "#%2.2x", lo);
				buf4[0] = '#';
				sprintfHexCode8(buf4+1, lo);
				break;
				
			case ADDR_ZP:
				//sprintf(buf4, "%2.2x", lo);
				sprintfHexCode8(buf4, lo);
				break;
				
			case ADDR_ZPX:
				//sprintf(buf4, "%2.2x,x", lo);
				sprintfHexCode8WithoutZeroEnding(buf4, lo);
				buf4[2] = ',';
				buf4[3] = 'x';
				buf4[4] = 0x00;
				break;
				
			case ADDR_ZPY:
				//sprintf(buf4, "%2.2x,y", lo);
				sprintfHexCode8WithoutZeroEnding(buf4, lo);
				buf4[2] = ',';
				buf4[3] = 'y';
				buf4[4] = 0x00;
				break;
				
			case ADDR_IZX:
				//sprintf(buf4, "(%2.2x,x)", lo);
				buf4[0] = '(';
				sprintfHexCode8WithoutZeroEnding(buf4+1, lo);
				buf4[3] = ',';
				buf4[4] = 'x';
				buf4[5] = ')';
				buf4[6] = 0x00;
				break;
				
			case ADDR_IZY:
				//sprintf(buf4, "(%2.2x),y", lo);
				buf4[0] = '(';
				sprintfHexCode8WithoutZeroEnding(buf4+1, lo);
				buf4[3] = ')';
				buf4[4] = ',';
				buf4[5] = 'y';
				buf4[6] = 0x00;
				break;
				
			case ADDR_ABS:
				//sprintf(buf4, "%4.4x", (hi << 8) | lo);
				sprintfHexCode8WithoutZeroEnding(buf4, hi);
				sprintfHexCode8(buf4+2, lo);
				break;
				
			case ADDR_ABX:
				//sprintf(buf4, "%4.4x,x", (hi << 8) | lo);
				sprintfHexCode8WithoutZeroEnding(buf4, hi);
				sprintfHexCode8WithoutZeroEnding(buf4+2, lo);
				buf4[4] = ',';
				buf4[5] = 'x';
				buf4[6] = 0x00;
				break;
				
			case ADDR_ABY:
				//sprintf(buf4, "%4.4x,y", (hi << 8) | lo);
				sprintfHexCode8WithoutZeroEnding(buf4, hi);
				sprintfHexCode8WithoutZeroEnding(buf4+2, lo);
				buf4[4] = ',';
				buf4[5] = 'y';
				buf4[6] = 0x00;
				break;
				
			case ADDR_IND:
				//sprintf(buf4, "(%4.4x)", (hi << 8) | lo);
				buf4[0] = '(';
				sprintfHexCode8WithoutZeroEnding(buf4+1, hi);
				sprintfHexCode8WithoutZeroEnding(buf4+3, lo);
				buf4[5] = ')';
				buf4[6] = 0x00;
				break;
				
			case ADDR_REL:
				//sprintf(buf4, "%4.4x", ((addr + 2) + (int8)lo) & 0xFFFF);
				sprintfHexCode16(buf4, ((addr + 2) + (int8)lo) & 0xFFFF);
				break;
			default:
				break;
		}

		//sprintf(buf, "%s%s", buf3, buf4);
		strcpy(buf, buf3);
		strcat(buf, buf4);
	}
	 */
}

// this is the same code but adding $ near hex value. this is copy pasted code for performance
void CViewDisassembly::MnemonicWithDollarArgumentToStr(u16 addr, u8 op, u8 lo, u8 hi, char *buf)
{
	char buf3[16];
	char buf4[16];
	
	// mnemonic
	strcpy(buf3, opcodes[op].name);
	strcat(buf3, " ");
	
	switch (opcodes[op].addressingMode)
	{
		case ADDR_IMP:
			sprintf(buf4, "");
			break;
			
		case ADDR_IMM:
			//sprintf(buf4, "#%2.2x", lo);
			buf4[0] = '#';
			buf4[1] = '$';
			sprintfHexCode8(buf4+2, lo);
			break;
			
		case ADDR_ZP:
			//sprintf(buf4, "%2.2x", lo);
			buf4[0] = '$';
			sprintfHexCode8(buf4+1, lo);
			break;
			
		case ADDR_ZPX:
			//sprintf(buf4, "%2.2x,x", lo);
			buf4[0] = '$';
			sprintfHexCode8WithoutZeroEnding(buf4+1, lo);
			buf4[3] = ',';
			buf4[4] = 'x';
			buf4[5] = 0x00;
			break;
			
		case ADDR_ZPY:
			//sprintf(buf4, "%2.2x,y", lo);
			buf4[0] = '$';
			sprintfHexCode8WithoutZeroEnding(buf4+1, lo);
			buf4[3] = ',';
			buf4[4] = 'y';
			buf4[5] = 0x00;
			break;
			
		case ADDR_IZX:
			//sprintf(buf4, "(%2.2x,x)", lo);
			buf4[0] = '(';
			buf4[1] = '$';
			sprintfHexCode8WithoutZeroEnding(buf4+2, lo);
			buf4[4] = ',';
			buf4[5] = 'x';
			buf4[6] = ')';
			buf4[7] = 0x00;
			break;
			
		case ADDR_IZY:
			//sprintf(buf4, "(%2.2x),y", lo);
			buf4[0] = '(';
			buf4[1] = '$';
			sprintfHexCode8WithoutZeroEnding(buf4+2, lo);
			buf4[4] = ')';
			buf4[5] = ',';
			buf4[6] = 'y';
			buf4[7] = 0x00;
			break;
			
		case ADDR_ABS:
			//sprintf(buf4, "%4.4x", (hi << 8) | lo);
			buf4[1] = '$';
			sprintfHexCode8WithoutZeroEnding(buf4+1, hi);
			sprintfHexCode8(buf4+3, lo);
			break;
			
		case ADDR_ABX:
			//sprintf(buf4, "%4.4x,x", (hi << 8) | lo);
			buf4[0] = '$';
			sprintfHexCode8WithoutZeroEnding(buf4+1, hi);
			sprintfHexCode8WithoutZeroEnding(buf4+3, lo);
			buf4[5] = ',';
			buf4[6] = 'x';
			buf4[7] = 0x00;
			break;
			
		case ADDR_ABY:
			//sprintf(buf4, "%4.4x,y", (hi << 8) | lo);
			buf4[0] = '$';
			sprintfHexCode8WithoutZeroEnding(buf4+1, hi);
			sprintfHexCode8WithoutZeroEnding(buf4+3, lo);
			buf4[5] = ',';
			buf4[6] = 'y';
			buf4[7] = 0x00;
			break;
			
		case ADDR_IND:
			//sprintf(buf4, "(%4.4x)", (hi << 8) | lo);
			buf4[0] = '(';
			buf4[1] = '$';
			sprintfHexCode8WithoutZeroEnding(buf4+2, hi);
			sprintfHexCode8WithoutZeroEnding(buf4+4, lo);
			buf4[6] = ')';
			buf4[7] = 0x00;
			break;
			
		case ADDR_REL:
			//sprintf(buf4, "%4.4x", ((addr + 2) + (int8)lo) & 0xFFFF);
			buf4[0] = '$';
			sprintfHexCode16(buf4+1, ((addr + 2) + (int8)lo) & 0xFFFF);
			break;
		default:
			break;
	}
	
	//sprintf(buf, "%s%s", buf3, buf4);
	strcpy(buf, buf3);
	strcat(buf, buf4);
}

// Disassemble one hex-only value (for disassemble up)
void CViewDisassembly::RenderHexLine(float px, float py, int addr)
{
	//	LOGD("addr=%4.4x op=%2.2x", addr, op);

	// check if this 1-length opcode
	uint8 op = memory[ (addr) % memoryLength];
	if (opcodes[op].addressingLength == 1)
	{
		RenderDisassemblyLine(px, py, addr, op, 0x00, 0x00);
		return;
	}

	float colorExecuteR, colorExecuteG, colorExecuteB;
	float colorExecuteA = 1.0f;
	float colorNonExecuteR, colorNonExecuteG, colorNonExecuteB;
	float colorNonExecuteA = 1.0f;
	
	GetColorsFromScheme(c64SettingsDisassemblyExecuteColor, &colorExecuteR, &colorExecuteG, &colorExecuteB);
	GetColorsFromScheme(c64SettingsDisassemblyNonExecuteColor, &colorNonExecuteR, &colorNonExecuteG, &colorNonExecuteB);
	
	char buf[128];
	char buf1[16];
	
	CDebugBreakpointsAddr *breakpoints = symbols->currentSegment->breakpointsPC;
	
	std::map<int, CDebugBreakpointAddr *>::iterator it = breakpoints->renderBreakpoints.find(addr);
	if (it != breakpoints->renderBreakpoints.end())
	{
		CDebugBreakpointAddr *breakpoint = it->second;
		
		// is active?
		if (breakpoint->isActive == true)
		{
			BlitFilledRectangle(posX, py, -1.0f,
								sizeX, fontSize, breakpointActiveColorR, breakpointActiveColorG, breakpointActiveColorB, breakpointActiveColorA);
		}
		else
		{
			BlitFilledRectangle(posX, py, -1.0f,
								sizeX, fontSize, breakpointNotActiveColorR, breakpointNotActiveColorG, breakpointNotActiveColorB, breakpointNotActiveColorA);
		}
	}
	
	CDebugMemoryCell *cell = debugMemory->GetMemoryCell(addr);
	
	float cr, cg, cb, ca;
	
	if (cell->isExecuteCode)
	{
		cr = colorExecuteR;
		cg = colorExecuteG;
		cb = colorExecuteB;
		ca = colorExecuteA;
	}
	else
	{
		cr = colorNonExecuteR;
		cg = colorNonExecuteG;
		cb = colorNonExecuteB;
		ca = colorNonExecuteA;
	}
	
	
	if (showLabels && symbols->currentSegment)
	{
		px += disassembledCodeOffsetX;
		
		CDebugSymbolsSegment *segment = symbols->currentSegment;
		
		std::map<int, CDebugSymbolsCodeLabel *>::iterator it = segment->codeLabelByAddress.find(addr);
		if (it != segment->codeLabelByAddress.end())
		{
			CDebugSymbolsCodeLabel *label = it->second;

			// found a label
			float pxe = this->posX + fontSize*labelNumCharacters;
			int l = strlen(label->GetLabelText())+1;
			float labelX = pxe - l*fontSize;

			fontDisassembly->BlitTextColor(label->GetLabelText(), labelX, py, -1, fontSize, cr, cg, cb, ca);
		}
	}

	// addr
	if (editCursorPos != EDIT_CURSOR_POS_ADDR)
	{
		sprintfHexCode16(buf, addr);
		fontDisassembly->BlitTextColor(buf, px, py, -1, fontSize, cr, cg, cb, ca);
	}
	else
	{
		if (addr == this->cursorAddress)
		{
			fontDisassembly->BlitTextColor(editHex->textWithCursor, px, py, posZ, fontSize, cr, cg, cb, ca);
		}
		else
		{
			sprintfHexCode16(buf, addr);
			fontDisassembly->BlitTextColor(buf, px, py, -1, fontSize, cr, cg, cb, ca);
		}
	}
	
	px += fontSize5;
	
	if (showHexCodes)
	{
		// check if editing
		if (addr == this->cursorAddress && editCursorPos == EDIT_CURSOR_POS_HEX1)
		{
			fontDisassembly->BlitTextColor(editHex->textWithCursor, px, py, posZ, fontSize, cr, cg, cb, ca);
		}
		else
		{
			sprintfHexCode8(buf1, op);
			fontDisassembly->BlitTextColor(buf1, px, py, posZ, fontSize, cr, cg, cb, ca);
		}
		
		px += fontSize9;
		px += fontSize;
	}
	
	px += mnemonicsOffsetX;

	if (editCursorPos != EDIT_CURSOR_POS_MNEMONIC)
	{
		if (showHexCodes)
		{
			fontDisassembly->BlitTextColor("???", px, py, -1, fontSize, cr, cg, cb, ca);
		}
		else
		{
			sprintfHexCode8(buf1, op);			
			fontDisassembly->BlitTextColor(buf1, px, py, -1, fontSize, cr, cg, cb, ca);
		}
	}
	else
	{
		if (addr == this->cursorAddress)
		{
			if (editBoxText->textBuffer[0] == 0x00)
			{
				fontDisassembly->BlitTextColor(strCodeLine, px, py, -1, fontSize,
											   colorExecuteR,
											   colorExecuteG,
											   colorExecuteB,
											   colorExecuteA);
				
				if (showHexCodes)
				{
					fontDisassembly->BlitTextColor("???", px, py, -1, fontSize, cr, cg, cb, ca);
				}
				else
				{
					sprintfHexCode8(buf1, op);
					fontDisassembly->BlitTextColor(buf1, px, py, -1, fontSize, cr, cg, cb, ca);
				}
			}
			else
			{
				fontDisassembly->BlitTextColor(strCodeLine, px, py, -1, fontSize,
											   colorExecuteR,
											   colorExecuteG,
											   colorExecuteB,
											   colorExecuteA);
			}
		}
		else
		{
			if (showHexCodes)
			{
				fontDisassembly->BlitTextColor("???", px, py, -1, fontSize, cr, cg, cb, ca);
			}
			else
			{
				sprintfHexCode8(buf1, op);
				fontDisassembly->BlitTextColor(buf1, px, py, -1, fontSize, cr, cg, cb, ca);
			}
		}
	}

	if (addr == currentPC)
	{
		BlitFilledRectangle(posX, py, -1.0f,
							sizeX, fontSize, cr, cg, cb, 0.3f);
	}
	
	if (c64SettingsExecuteAwareDisassembly)
	{
		int newAddress = addr + 1;
		if (newAddress == renderStartAddress)
		{
			previousOpAddr = addr;
			//		LOGD("(H) previousOpAddr=%04x", previousOpAddr);
		}
		
		if (addr == renderStartAddress)
		{
			nextOpAddr = addr+1;
			//		LOGD("(H) nextOpAddr=%04x", nextOpAddr);
		}
	}
}

///////////

int CViewDisassembly::UpdateDisassemblyOpcodeLine(float py, int addr, uint8 op, uint8 lo, uint8 hi)
{
	int numBytesPerOp = opcodes[op].addressingLength;

	if (numAddrPositions < 2)
	{
		LOGError("CViewDisassembly::UpdateDisassemblyOpcodeLine: numAddrPositions < 1");
		return numBytesPerOp;
	}
	if (addrPositionCounter >= numAddrPositions)
		addrPositionCounter = numAddrPositions-2;
		
	addrPositions[addrPositionCounter].addr = addr;
	addrPositions[addrPositionCounter].y = py;
	addrPositionCounter++;
	
//	LOGD("addrPositionCounter=%d", addrPositionCounter);
		
	int newAddress = addr + numBytesPerOp;
	if (newAddress == renderStartAddress)
	{
		previousOpAddr = addr;
	}
	
	if (addr == renderStartAddress)
	{
		nextOpAddr = addr+numBytesPerOp;
	}
	
	return numBytesPerOp;
}

void CViewDisassembly::UpdateDisassemblyHexLine(float py, int addr)
{
	if (numAddrPositions < 2)
	{
		LOGError("CViewDisassembly::UpdateDisassemblyHexLine: numAddrPositions < 1");
		return;
	}
	if (addrPositionCounter >= numAddrPositions)
		addrPositionCounter = numAddrPositions-2;

	addrPositions[addrPositionCounter].addr = addr;
	addrPositions[addrPositionCounter].y = py;
	addrPositionCounter++;

	// check if this 1-length opcode
	uint8 op = memory[ (addr) % memoryLength];
	if (opcodes[op].addressingLength == 1)
	{
		UpdateDisassemblyOpcodeLine(py, addr, op, 0x00, 0x00);
		return;
	}
	
	int newAddress = addr + 1;
	if (newAddress == renderStartAddress)
	{
		previousOpAddr = addr;
	}
	
	if (addr == renderStartAddress)
	{
		nextOpAddr = addr+1;
	}
}

void CViewDisassembly::UpdateDisassembly(int startAddress, int endAddress)
{
	if (c64SettingsExecuteAwareDisassembly == false)
	{
		UpdateDisassemblyNotExecuteAware(startAddress, endAddress);
		return;
	}
	
	guiMain->LockMutex();
	
	addrPositionCounter = 0;
	
	bool done = false;
	short i;
	uint8 opcode;
	uint8 op[3];
	uint16 addr;
	
	float py = posY;
	
	int renderAddress = startAddress;
	int renderLinesBefore;
	
	if (startAddress < 0)
		startAddress = 0;
	if (endAddress > 0x10000)
		endAddress = 0x10000;
	
	UpdateLocalMemoryCopy(startAddress, endAddress);
	
	CalcDisassemblyStart(startAddress, &renderAddress, &renderLinesBefore);
	
	renderSkipLines = numberOfLinesBack - renderLinesBefore;
	int skipLines = renderSkipLines;
	
	{
		py += (float)(skipLines) * fontSize;
	}
	
	
	startRenderY = py;
	startRenderAddr = renderAddress;
	endRenderAddr = endAddress;
	
	//
	renderStartAddress = startAddress;
	
	
	if (renderLinesBefore == 0)
	{
		previousOpAddr = startAddress - 1;
	}
	
	do
	{
//		LOGD("renderAddress=%4.4x l=%4.4x addrPositionCounter=%d py=%f posEndY=%f", renderAddress, memoryLength, addrPositionCounter, py, posEndY);
		if (renderAddress >= memoryLength)
			break;
		
		if (renderAddress == memoryLength-1)
		{
			UpdateDisassemblyHexLine(py, renderAddress);
			break;
		}
		
		addr = renderAddress;
		
		for (i=0; i<3; i++, addr++)
		{
			if (addr == endAddress)
			{
				done = true;
			}
			
			op[i] = memory[addr];
		}
		
		{
			addr = renderAddress;
			
			// +0
			CDebugMemoryCell *cell0 = debugMemory->GetMemoryCell(addr % memoryLength);
			if (cell0->isExecuteCode)
			{
				opcode = memory[addr];	//% memoryLength
				renderAddress += UpdateDisassemblyOpcodeLine(py, renderAddress, op[0], op[1], op[2]);
				py += fontSize;
			}
			else
			{
				// +1
				CDebugMemoryCell *cell1 = debugMemory->GetMemoryCell( (addr+1) % memoryLength);
				if (cell1->isExecuteCode)
				{
					// check if at addr is 1-length opcode
					opcode = memory[ (renderAddress) ]; //% memoryLength
					if (opcodes[opcode].addressingLength == 1)
					{
						UpdateDisassemblyOpcodeLine(py, renderAddress, op[0], op[1], op[2]);
					}
					else
					{
						UpdateDisassemblyHexLine(py, renderAddress);
					}
					
					renderAddress += 1;
					py += fontSize;
					
					addr = renderAddress;
					for (i=0; i<3; i++, addr++)
					{
						if (addr == endAddress)
						{
							done = true;
						}
						
						op[i] = memory[addr];
					}
					
					opcode = memory[ (renderAddress) ];	//% memoryLength
					renderAddress += UpdateDisassemblyOpcodeLine(py, renderAddress, op[0], op[1], op[2]);
					py += fontSize;
				}
				else
				{
					// +2
					CDebugMemoryCell *cell2 = debugMemory->GetMemoryCell( (addr+2) % memoryLength );
					if (cell2->isExecuteCode)
					{
						// check if at addr is 2-length opcode
						opcode = memory[ (renderAddress) ];	//% memoryLength
						if (opcodes[opcode].addressingLength == 2)
						{
							renderAddress += UpdateDisassemblyOpcodeLine(py, renderAddress, op[0], op[1], op[2]);
							py += fontSize;
						}
						else
						{
							UpdateDisassemblyHexLine(py, renderAddress);
							renderAddress += 1;
							py += fontSize;
							
							UpdateDisassemblyHexLine(py, renderAddress);
							renderAddress += 1;
							py += fontSize;
						}
						
						addr = renderAddress;
						for (i=0; i<3; i++, addr++)
						{
							if (addr == endAddress)
							{
								done = true;
							}
							
							op[i] = memory[addr];
						}
						
						opcode = memory[ (renderAddress % memoryLength) ];
						renderAddress += UpdateDisassemblyOpcodeLine(py, renderAddress, op[0], op[1], op[2]);
						py += fontSize;
					}
					else
					{
						if (cell0->isExecuteArgument == false)
						{
							// execute not found, just render line
							renderAddress += UpdateDisassemblyOpcodeLine(py, renderAddress, op[0], op[1], op[2]);
							py += fontSize;
						}
						else
						{
							// it is argument
							UpdateDisassemblyHexLine(py, renderAddress);
							renderAddress++;
							py += fontSize;
						}
					}
				}
			}
			
			if (py > posEndY)
				break;
			
		}
	}
	while (!done);
	
	// disassemble up?
	int length;
	
	if (skipLines > 0)
	{
		//LOGD("disassemble UP: %04x y=%5.2f", startRenderAddr, startRenderY);
		
		py = startRenderY;
		renderAddress = startRenderAddr;
		
		while (skipLines > 0)
		{
			py -= fontSize;
			skipLines--;
			
			if (renderAddress < 0)
				break;
			
			// check how much scroll up
			byte op, lo, hi;
			
			// TODO: generalise this and do more than -3  + check executeArgument!
			// check -3
			if (renderAddress > 2)
			{
				// check execute markers first
				int addr;
				
				addr = renderAddress-1;
				CDebugMemoryCell *cell1 = debugMemory->GetMemoryCell(addr % memoryLength);
				if (cell1->isExecuteCode)
				{
					op = memory[addr];
					if (opcodes[op].addressingLength == 1)
					{
						UpdateDisassemblyOpcodeLine(py, addr, op, 0x00, 0x00);
						renderAddress = addr;
						continue;
					}
					
					UpdateDisassemblyHexLine(py, addr);
					renderAddress = addr;
					continue;
				}
				
				addr = renderAddress-2;
				CDebugMemoryCell *cell2 = debugMemory->GetMemoryCell(addr % memoryLength);
				if (cell2->isExecuteCode)
				{
					op = memory[addr];
					if (opcodes[op].addressingLength == 2)
					{
						lo = memory[renderAddress-1];
						UpdateDisassemblyOpcodeLine(py, addr, op, lo, 0x00);
						renderAddress = addr;
						continue;
					}
					
					renderAddress--;
					UpdateDisassemblyHexLine(py, renderAddress);
					py -= fontSize;
					skipLines--;
					renderAddress--;
					UpdateDisassemblyHexLine(py, renderAddress);
					continue;
				}
				
				addr = renderAddress-3;
				CDebugMemoryCell *cell3 = debugMemory->GetMemoryCell(addr % memoryLength);
				if (cell3->isExecuteCode)
				{
					op = memory[addr];
					int opLen = opcodes[op].addressingLength;
					if (opLen == 3)
					{
						lo = memory[renderAddress-2];
						hi = memory[renderAddress-1];
						UpdateDisassemblyOpcodeLine(py, addr, op, lo, hi);
						renderAddress = addr;
						continue;
					}
					else if (opLen == 2)
					{
						UpdateDisassemblyHexLine(py, renderAddress-1);
						
						py -= fontSize;
						skipLines--;
						
						lo = memory[renderAddress-2];
						UpdateDisassemblyOpcodeLine(py, addr, op, lo, 0x00);
						
						renderAddress = addr;
						continue;
					}
					
					renderAddress--;
					UpdateDisassemblyHexLine(py, renderAddress);
					py -= fontSize;
					skipLines--;
					
					renderAddress--;
					UpdateDisassemblyHexLine(py, renderAddress);
					py -= fontSize;
					skipLines--;
					
					renderAddress--;
					UpdateDisassemblyHexLine(py, renderAddress);
					continue;
				}
				
				if (cell1->isExecuteArgument == false
					&& cell2->isExecuteArgument == false
					&& cell3->isExecuteArgument == false)
				{
					//
					// then check normal -3
					op = memory[renderAddress-3];
					length = opcodes[op].addressingLength;
					
					if (length == 3)
					{
						lo = memory[renderAddress-2];
						hi = memory[renderAddress-1];
						UpdateDisassemblyOpcodeLine(py, renderAddress-3, op, lo, hi);
						
						renderAddress -= 3;
						continue;
					}
				}
			}
			
			// check -2
			if (renderAddress > 1)
			{
				CDebugMemoryCell *cell1 = debugMemory->GetMemoryCell(renderAddress-1);
				CDebugMemoryCell *cell2 = debugMemory->GetMemoryCell(renderAddress-2);
				if (cell1->isExecuteArgument == false
					&& cell2->isExecuteArgument == false)
				{
					op = memory[renderAddress-2];
					
					length = opcodes[op].addressingLength;
					
					if (length == 2)
					{
						lo = memory[renderAddress-1];
						UpdateDisassemblyOpcodeLine(py, renderAddress-2, op, lo, lo);
						
						renderAddress -= 2;
						continue;
					}
				}
			}
			
			// check -1
			if (renderAddress > 0)
			{
				CDebugMemoryCell *cell1 = debugMemory->GetMemoryCell(renderAddress-1);
				
				if (cell1->isExecuteArgument == false)
				{
					op = memory[renderAddress-1];
					length = opcodes[op].addressingLength;
					
					if (length == 1)
					{
						UpdateDisassemblyOpcodeLine(py, renderAddress-1, op, 0x00, 0x00);
						
						renderAddress -= 1;
						continue;
					}
				}
			}
			
			// not found compatible op, just render hex
			if (renderAddress > 0)
			{
				renderAddress -= 1;
				UpdateDisassemblyHexLine(py, renderAddress);
			}
		}
	}
	
	guiMain->UnlockMutex();
}

void CViewDisassembly::StartEditingAtCursorPosition(int newCursorPos, bool goLeft)
{
	//LOGD("CViewDisassembly::StartEditingAtCursorPosition: adr=%4.4x newCursorPos=%d", cursorAddress, newCursorPos);
	
	if (newCursorPos < EDIT_CURSOR_POS_ADDR || newCursorPos > EDIT_CURSOR_POS_MNEMONIC)
		return;
		
	guiMain->LockMutex();
	
	isTrackingPC = false;
	changedByUser = true;
	
	if (newCursorPos == EDIT_CURSOR_POS_ADDR)
	{
		editHex->SetValue(cursorAddress, 4);
	}
	else if (newCursorPos >= EDIT_CURSOR_POS_HEX1 && newCursorPos <= EDIT_CURSOR_POS_HEX3)
	{
		int adr = cursorAddress;
		
		uint8 op[3];
		bool isAvailable;
		
		dataAdapter->AdapterReadByte(adr,   &(op[0]), &isAvailable);
		dataAdapter->AdapterReadByte(adr+1, &(op[1]), &isAvailable);
		dataAdapter->AdapterReadByte(adr+2, &(op[2]), &isAvailable);
		
		int l = opcodes[op[0]].addressingLength;
		
		// check if possible
		if (newCursorPos == EDIT_CURSOR_POS_HEX3 && l < 3)
		{
			if (goLeft)
			{
				newCursorPos = EDIT_CURSOR_POS_HEX2;
			}
			else
			{
				newCursorPos = EDIT_CURSOR_POS_MNEMONIC;
			}
		}
		if (newCursorPos == EDIT_CURSOR_POS_HEX2 && l  == 1)
		{
			if (goLeft)
			{
				newCursorPos = EDIT_CURSOR_POS_HEX1;
			}
			else
			{
				newCursorPos = EDIT_CURSOR_POS_MNEMONIC;
			}
		}
		
		if (newCursorPos != EDIT_CURSOR_POS_MNEMONIC)
		{
			int cp = newCursorPos-EDIT_CURSOR_POS_HEX1;
			editHex->SetValue(op[cp], 2);
		}
	}
	
	if (newCursorPos == EDIT_CURSOR_POS_MNEMONIC)
	{
		editBoxText->editing = true;
		editBoxText->SetText("");
		
		strCodeLine->Clear();
		u16 chr = 0x20 + CBMSHIFTEDFONT_INVERT;
		strCodeLine->Concatenate(chr);
	}
	
	editCursorPos = newCursorPos;

	guiMain->UnlockMutex();
}



void CViewDisassembly::DoLogic()
{
}

void CViewDisassembly::GetDisassemblyBackgroundColor(float *colorR, float *colorG, float *colorB)
{
	GetColorsFromScheme(c64SettingsDisassemblyBackgroundColor, colorR, colorG, colorB);
}

void CViewDisassembly::Render()
{
//	if (debugInterface->GetSettingIsWarpSpeed() == true)
//		return;
	
//	LOGD("=============================== CViewDisassembly::Render");

	if (symbols == NULL || symbols->currentSegment == NULL)
		return;
	
	if (prevFrameSizeY != sizeY)
	{
		UpdateNumberOfVisibleLines();
	}
	
	this->numberOfCharactersInLabel = labelNumCharacters;
	disassembledCodeOffsetX = fontSize*labelNumCharacters;

	//
	VID_SetClipping(this->posX, this->posY, this->sizeX, this->sizeY);
	
	CDebugBreakpointsAddr *breakpoints = symbols->currentSegment->breakpointsPC;
	
	breakpoints->renderBreakpointsMutex->Lock();
	
	float colorBackgroundR, colorBackgroundG, colorBackgroundB;
	float colorBackgroundA = 1.0f;
	
	GetDisassemblyBackgroundColor(&colorBackgroundR, &colorBackgroundG, &colorBackgroundB);

	BlitFilledRectangle(this->posX, this->posY, -1, this->sizeX, this->sizeY, colorBackgroundR, colorBackgroundG, colorBackgroundB, colorBackgroundA);
	
	if (isTrackingPC)
	{
		this->cursorAddress = this->currentPC;
	}
	
	if (c64SettingsExecuteAwareDisassembly)
	{
		this->RenderDisassembly(this->cursorAddress, this->cursorAddress + 0x0100);
	}
	else
	{
		
		this->RenderDisassemblyNotExecuteAware(this->cursorAddress, this->cursorAddress + 0x0100);
	}
	
	breakpoints->renderBreakpointsMutex->Unlock();

	VID_ResetClipping();
}

void CViewDisassembly::Render(float posX, float posY)
{
	this->Render();
}

void CViewDisassembly::RenderImGui()
{
	PreRenderImGui();
	Render();
	PostRenderImGui();
}


void CViewDisassembly::TogglePCBreakpoint(int addr)
{
	debugInterface->LockMutex();
	
	bool found = false;
	
	CDebugBreakpointsAddr *breakpoints = symbols->currentSegment->breakpointsPC;
	
	// keep copy of breakpoints to not lock mutex during rendering
	std::map<int, CDebugBreakpointAddr *>::iterator it = breakpoints->renderBreakpoints.find(addr);
	if (it != breakpoints->renderBreakpoints.end())
	{
		// remove breakpoint
		//LOGD("remove breakpoint addr=%4.4x", addr);

		breakpoints->renderBreakpoints.erase(it);
		breakpoints->DeleteBreakpoint(addr);
		
		found = true;
	}
	
	if (found == false)
	{
		// add breakpoint
		LOGD("add breakpoint addr=%4.4x", addr);

		CDebugBreakpointAddr *breakpoint = new CDebugBreakpointAddr(addr);
		breakpoint->actions = ADDR_BREAKPOINT_ACTION_STOP;
		
		breakpoints->AddBreakpoint(breakpoint);
		breakpoints->renderBreakpoints[addr] = breakpoint;
	}
	
	debugInterface->UnlockMutex();
}

CDebugBreakpointAddr *CViewDisassembly::AddPCBreakpoint(int addr)
{
	debugInterface->LockMutex();
	
	CDebugBreakpointsAddr *breakpoints = symbols->currentSegment->breakpointsPC;
	
	// keep copy of breakpoints to not lock mutex during rendering
	std::map<int, CDebugBreakpointAddr *>::iterator it = breakpoints->renderBreakpoints.find(addr);
	if (it != breakpoints->renderBreakpoints.end())
	{
		debugInterface->UnlockMutex();
		return it->second;
	}
	
	// add breakpoint
	LOGD("add breakpoint addr=%4.4x", addr);

	CDebugBreakpointAddr *breakpoint = new CDebugBreakpointAddr(addr);
	breakpoint->actions = ADDR_BREAKPOINT_ACTION_STOP;
	
	breakpoints->AddBreakpoint(breakpoint);
	breakpoints->renderBreakpoints[addr] = breakpoint;
	
	debugInterface->UnlockMutex();
	
	return breakpoint;
}

u64 CViewDisassembly::RemovePCBreakpoint(int addr)
{
	debugInterface->LockMutex();
	
	CDebugBreakpointsAddr *breakpoints = symbols->currentSegment->breakpointsPC;

	u64 breakpointId = UNKNOWN_BREAKPOINT_ID;
	
	// keep copy of breakpoints to not lock mutex during rendering
	std::map<int, CDebugBreakpointAddr *>::iterator it = breakpoints->renderBreakpoints.find(addr);
	if (it != breakpoints->renderBreakpoints.end())
	{
		// remove breakpoint
		//LOGD("remove breakpoint addr=%4.4x", addr);

		breakpoints->renderBreakpoints.erase(it);
		breakpointId = breakpoints->DeleteBreakpoint(addr);
	}
	debugInterface->UnlockMutex();
	return breakpointId;
}

//@returns is consumed
bool CViewDisassembly::DoTap(float x, float y)
{
	LOGG("CViewDisassembly::DoTap:  x=%f y=%f", x, y);

	// set breakpoint?
	if (c64SettingsPressCtrlToSetBreakpoint)
	{
		// no ctrl pressed, skip this tap
		if (!guiMain->isControlPressed)
			return false;
	}
	else
	{
		// check if pressed on address field
		float numChars = 4.0f;
		if (showLabels == true)
		{
			numChars = 4.0f + numberOfCharactersInLabel;
		}

		if (!(x >= posX && x <= (posX+fontSize * numChars)))
		{
			return false;
		}
	}

	UpdateDisassembly(this->cursorAddress, this->cursorAddress + 0x0100);
	
	// set the breakpoint
	if (c64SettingsExecuteAwareDisassembly == false)
	{
		return DoTapNotExecuteAware(x, y);
	}
	
	for (int i = 0; i < addrPositionCounter; i++)
	{
		if (y > addrPositions[i].y
			&& y <= addrPositions[i].y + fontSize)
		{
			TogglePCBreakpoint(addrPositions[i].addr);
			break;
		}
	}
	
	return true;
}

bool CViewDisassembly::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	LOGD("CViewDisassembly::KeyDown: %x %s %s %s", keyCode, STRBOOL(isShift), STRBOOL(isAlt), STRBOOL(isControl));

	u32 bareKey = SYS_GetBareKey(keyCode, isShift, isAlt, isControl, isSuper);
	
	CSlrKeyboardShortcut *keyboardShortcut = guiMain->keyboardShortcuts->FindShortcut(shortcutZones, bareKey, isShift, isAlt, isControl, isSuper);
	
	if (keyboardShortcut != viewC64->mainMenuBar->kbsGoToAddress
		&& (editCursorPos == EDIT_CURSOR_POS_ADDR
		|| editCursorPos == EDIT_CURSOR_POS_HEX1 || editCursorPos == EDIT_CURSOR_POS_HEX2
		|| editCursorPos == EDIT_CURSOR_POS_HEX3 || editCursorPos == EDIT_CURSOR_POS_MNEMONIC))
	{
		if (keyCode == MTKEY_ESC)
		{
			isEnteringGoto = false;
			wantedEditCursorPos = EDIT_CURSOR_POS_NONE;
			editCursorPos = EDIT_CURSOR_POS_NONE;
			return true;
		}
		
		guiMain->LockMutex();
		debugInterface->LockMutex();
		
		CSlrKeyboardShortcut *keyboardShortcutGlobal = guiMain->keyboardShortcuts->FindShortcut(KBZONE_GLOBAL, bareKey, isShift, isAlt, isControl, isSuper);
		if (keyboardShortcutGlobal == viewC64->mainMenuBar->kbsPasteFromClipboard)
		{
			// will run keyb downs from clipboard
			this->PasteKeysFromClipboard();
			
			guiMain->UnlockMutex();
			debugInterface->UnlockMutex();
			return true;
		}
		
		if (keyCode == MTKEY_ARROW_UP || keyCode == MTKEY_ARROW_DOWN)
		{
			if ((editCursorPos >= EDIT_CURSOR_POS_HEX1 && editCursorPos <= EDIT_CURSOR_POS_HEX3) || editCursorPos == EDIT_CURSOR_POS_MNEMONIC)
			{
				FinalizeEditing();
			}
			
			if (keyCode == MTKEY_ARROW_DOWN)
			{
				ScrollDown();
			}
			else if (keyCode == MTKEY_ARROW_UP)
			{
				ScrollUp();
			}
			
			StartEditingAtCursorPosition(wantedEditCursorPos, true);
			
			debugInterface->UnlockMutex();
			guiMain->UnlockMutex();
			return true;
		}
		
		if (editCursorPos == EDIT_CURSOR_POS_MNEMONIC)
		{
			if (keyCode == MTKEY_ARROW_LEFT && editBoxText->currentPos == 0)
			{
				wantedEditCursorPos = EDIT_CURSOR_POS_HEX3;
				StartEditingAtCursorPosition(EDIT_CURSOR_POS_HEX3, true);

				debugInterface->UnlockMutex();
				guiMain->UnlockMutex();
				return true;
			}
			
			// uppercase mnemonics
			if (editBoxText->currentPos < 3)
			{
				keyCode = toupper(keyCode);
			}
			
			editBoxText->KeyDown(keyCode, isShift, isAlt, isControl, isSuper);

			debugInterface->UnlockMutex();
			guiMain->UnlockMutex();
			return true;
		}

		
		editHex->KeyDown(keyCode);

		debugInterface->UnlockMutex();
		guiMain->UnlockMutex();
		return true;
	}
	
	if (isShift && keyCode == MTKEY_ARROW_DOWN)
	{
		viewC64->KeyUpModifierKeys(isShift, isAlt, isControl);
		
		// TODO: check why KeyUpModifierKeys should not be run on Atari?
//		else if (viewC64->viewAtariScreen)
//		{
		// do not keyup for Atari  ---> TODO: WHY?
////			viewC64->viewAtariScreen->KeyUpModifierKeys(isShift, isAlt, isControl);
//		}

		keyCode = MTKEY_PAGE_DOWN;
	}

	if (isShift && keyCode == MTKEY_ARROW_UP)
	{
		viewC64->KeyUpModifierKeys(isShift, isAlt, isControl);
		
		// TODO: check why KeyUpModifierKeys should not be run on Atari?
//		else if (viewC64->viewAtariScreen)
//		{
		// do not keyup for Atari  ---> TODO: WHY?
////			viewC64->viewAtariScreen->KeyUpModifierKeys(isShift, isAlt, isControl);
//		}

		keyCode = MTKEY_PAGE_UP;
	}
	
	if (keyboardShortcut == viewC64->mainMenuBar->kbsToggleBreakpoint)
	{
		TogglePCBreakpoint(cursorAddress);
		viewC64->KeyUpModifierKeys(isShift, isAlt, isControl);
		return true;
	}
	
	if (keyboardShortcut == viewC64->mainMenuBar->kbsStepOverSubroutine)
	{
		StepOverJsr();
		viewC64->KeyUpModifierKeys(isShift, isAlt, isControl);
		return true;
	}
	
	if (keyboardShortcut == viewC64->mainMenuBar->kbsMakeJmp)
	{
		MakeJMPToCursor();
		viewC64->KeyUpModifierKeys(isShift, isAlt, isControl);
		return true;
	}

	if (keyboardShortcut == viewC64->mainMenuBar->kbsToggleTrackPC)
	{
		if (isTrackingPC == false)
		{
			isTrackingPC = true;
			changedByUser = false;
		}
		else
		{
			cursorAddress = currentPC;
			isTrackingPC = false;
			changedByUser = true;
		}

		viewC64->KeyUpModifierKeys(isShift, isAlt, isControl);
		return true;
	}
	
	if (keyboardShortcut == viewC64->mainMenuBar->kbsGoToAddress)
	{
		isEnteringGoto = true;
		StartEditingAtCursorPosition(EDIT_CURSOR_POS_ADDR, true);
		viewC64->KeyUpModifierKeys(isShift, isAlt, isControl);
		return true;
	}

	if (keyboardShortcut == viewC64->mainMenuBar->kbsCopyToClipboard)
	{
		this->CopyAssemblyToClipboard();
		return true;
	}
	
	if (keyboardShortcut == viewC64->mainMenuBar->kbsCopyAlternativeToClipboard)
	{
		this->CopyHexAddressToClipboard();
		return true;
	}
	
	if (keyboardShortcut == viewC64->mainMenuBar->kbsPasteFromClipboard)
	{
		this->PasteHexValuesFromClipboard();
		return true;
	}

	if (keyCode == MTKEY_ARROW_DOWN)
	{
		ScrollDown();
		return true;
	}
	else if (keyCode == MTKEY_ARROW_UP)
	{
		ScrollUp();
		return true;
	}
	else if (keyCode == '[' && isShift==false && isAlt == false && isControl == false && isSuper == false)	// was: MTKEY_ARROW_LEFT)
	{
		isTrackingPC = false;
		changedByUser = true;
		cursorAddress--;
		if (cursorAddress < 0)
			cursorAddress = 0;
		return true;
	}
	else if (keyCode == MTKEY_PAGE_UP)
	{
		if (isTrackingPC)
		{
			cursorAddress = currentPC;
		}
		
		int newCursorAddress = cursorAddress - 0x0100;
		if (newCursorAddress < 0)
			newCursorAddress = 0;
		
		SetCursorToNearExecuteCodeAddress(newCursorAddress);
		return true;
	}
	else if (keyCode == MTKEY_PAGE_DOWN)
	{
		if (isTrackingPC)
		{
			cursorAddress = currentPC;
		}
		int newCursorAddress = cursorAddress + 0x0100;
		if (newCursorAddress > dataAdapter->AdapterGetDataLength()-1)
			newCursorAddress = dataAdapter->AdapterGetDataLength()-1;
		
		SetCursorToNearExecuteCodeAddress(newCursorAddress);
		return true;
	}
	else if (keyCode == ']' && isShift == false && isAlt == false && isControl == false && isSuper == false)
	{
		isTrackingPC = false;
		changedByUser = true;
		cursorAddress++;
		if (cursorAddress > dataAdapter->AdapterGetDataLength()-1)
			cursorAddress = dataAdapter->AdapterGetDataLength()-1;
		return true;
	}
	else if (keyCode == MTKEY_ARROW_LEFT)
	{
		MoveAddressHistoryBack();
		return true;
	}
	else if (keyCode == MTKEY_ARROW_RIGHT)
	{
		MoveAddressHistoryForward();
		return true;
	}
	else if (keyCode == MTKEY_ENTER)
	{
		isEnteringGoto = false;
		wantedEditCursorPos = EDIT_CURSOR_POS_MNEMONIC;
		StartEditingAtCursorPosition(EDIT_CURSOR_POS_MNEMONIC, false);
		return true;
	}
	
	return CGuiView::KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
}

bool CViewDisassembly::KeyDownRepeat(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
//	if (HasFocus())
	{
		return KeyDown(keyCode, isShift, isAlt, isControl, isSuper);
	}
	
	return false;
}

void CViewDisassembly::SetIsTrackingPC(bool followPC)
{
	this->isTrackingPC = followPC;
}

void CViewDisassembly::StepOverJsr()
{
//	LOGD("CViewDisassembly::StepOverJsr");
	
	// step over JSR
	debugInterface->LockMutex();
	
	bool a;
	int pc = symbols->GetCurrentExecuteAddr();
	uint8 opcode;
	dataAdapter->AdapterReadByte(pc, &opcode, &a);
	if (a)
	{
//		LOGD("pc=%04x opcode=%02x", pc, opcode);
		// is JSR?
		if (opcode == 0x20)
		{
			int breakPC = pc + opcodes[opcode].addressingLength;
			
			CDebugBreakpointsAddr *breakpointsPC = symbols->currentSegment->breakpointsPC;
			breakpointsPC->SetTemporaryBreakpointPC(breakPC);
			
//			LOGD("temporary C64 breakPC=%04x", breakPC);
			
			// run to temporary breakpoint (next line after JSR)
			debugInterface->SetDebugMode(DEBUGGER_MODE_RUNNING);
		}
		else
		{
			debugInterface->SetDebugMode(DEBUGGER_MODE_RUN_ONE_INSTRUCTION);
		}
	}

	debugInterface->UnlockMutex();
}

void CViewDisassembly::MoveAddressHistoryBack()
{
	guiMain->LockMutex();

	if (!traverseHistoryAddresses.empty())
	{
		int addr = traverseHistoryAddresses.back();
		traverseHistoryAddresses.pop_back();
		
		cursorAddress = addr;
	}
	
	guiMain->UnlockMutex();
}

// accessed when keyboard right arrow is pressed
void CViewDisassembly::MoveAddressHistoryForward()
{
	guiMain->LockMutex();
	
	if (cursorAddress > dataAdapter->AdapterGetDataLength()-3)
		cursorAddress = dataAdapter->AdapterGetDataLength()-3;
	
	u8 opcode;
	dataAdapter->AdapterReadByte(cursorAddress, &opcode);
	
	// $20 JSR, $4C JMP
	if (opcode == 0x20 || opcode == 0x4C)
	{
		u8 a1, a2;
		dataAdapter->AdapterReadByte(cursorAddress+1, &a1);
		dataAdapter->AdapterReadByte(cursorAddress+2, &a2);
		
		u16 addr = a1 | (a2 << 8);
		MoveAddressHistoryForwardWithAddr(addr);
	}
	// JMP (xxxx)
	else if (opcode == 0x6C)
	{
		u8 a1, a2;
		dataAdapter->AdapterReadByte(cursorAddress+1, &a1);
		dataAdapter->AdapterReadByte(cursorAddress+2, &a2);
		
		u16 addr1 = a1 | (a2 << 8);

		dataAdapter->AdapterReadByte(addr1,   &a1);
		dataAdapter->AdapterReadByte(addr1+1, &a2);

		u16 addr = a1 | (a2 << 8);
		MoveAddressHistoryForwardWithAddr(addr);
	}
	else if (opcodes[opcode].addressingMode == ADDR_REL)
	{
		u8 a1;
		dataAdapter->AdapterReadByte(cursorAddress+1, &a1);
		
		u16 addr = (cursorAddress + 2 + (int8)a1) & 0xFFFF;
		MoveAddressHistoryForwardWithAddr(addr);
	}
	// scroll memory dump
	else if (opcodes[opcode].addressingMode == ADDR_ABS
			 || opcodes[opcode].addressingMode == ADDR_ABX
			 || opcodes[opcode].addressingMode == ADDR_ABY)
	{
		u8 a1, a2;
		dataAdapter->AdapterReadByte(cursorAddress+1, &a1);
		dataAdapter->AdapterReadByte(cursorAddress+2, &a2);
		
		u16 addr = a1 | (a2 << 8);
		
		if (this->viewDataDump)
		{
			this->viewDataDump->ScrollToAddress(addr);
		}
	}
	else if (opcodes[opcode].addressingMode == ADDR_IND)
	{
		u8 a1, a2;
		dataAdapter->AdapterReadByte(cursorAddress+1, &a1);
		dataAdapter->AdapterReadByte(cursorAddress+2, &a2);
		
		u16 addr1 = a1 | (a2 << 8);
		
		dataAdapter->AdapterReadByte(addr1,   &a1);
		dataAdapter->AdapterReadByte(addr1+1, &a2);
		
		u16 addr = a1 | (a2 << 8);
		if (this->viewDataDump)
		{
			this->viewDataDump->ScrollToAddress(addr);
		}
	}
	else if (opcodes[opcode].addressingMode == ADDR_ZP
			 || opcodes[opcode].addressingMode == ADDR_ZPX
			 || opcodes[opcode].addressingMode == ADDR_ZPY
			 || opcodes[opcode].addressingMode == ADDR_IZX
			 || opcodes[opcode].addressingMode == ADDR_IZY)
	{
		u8 a;
		dataAdapter->AdapterReadByte(cursorAddress+1, &a);
		
		u16 addr = a;
		if (this->viewDataDump)
		{
			this->viewDataDump->ScrollToAddress(addr);
		}
	}
	
	guiMain->UnlockMutex();
}

void CViewDisassembly::MoveAddressHistoryForwardWithAddr(int addr)
{
	traverseHistoryAddresses.push_back(cursorAddress);

	cursorAddress = addr;

	LOGD(" -- traverse history --");
	for (std::vector<int>::iterator it = traverseHistoryAddresses.begin();
		 it != traverseHistoryAddresses.end(); it++)
	{
		LOGD(" .. %04x", *it);
	}

	LOGD(" -- traverse history end --");
}

void CViewDisassembly::SetCursorToNearExecuteCodeAddress(int newCursorAddress)
{
	isTrackingPC = false;
	
	for (int addr = newCursorAddress; addr > newCursorAddress-3; addr--)
	{
		if (debugMemory->IsExecuteCodeAddress(addr))
		{
			cursorAddress = addr;
			return;
		}
	}
	
	for (int addr = newCursorAddress; addr < newCursorAddress+3; addr++)
	{
		if (debugMemory->IsExecuteCodeAddress(addr))
		{
			cursorAddress = addr;
			return;
		}
	}
	
	cursorAddress = newCursorAddress;
}


bool CViewDisassembly::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	if (editCursorPos == EDIT_CURSOR_POS_ADDR
		|| editCursorPos == EDIT_CURSOR_POS_HEX1 || editCursorPos == EDIT_CURSOR_POS_HEX2
		|| editCursorPos == EDIT_CURSOR_POS_HEX3)
		return true;
	
	return CGuiView::KeyUp(keyCode, isShift, isAlt, isControl, isSuper);
}

void CViewDisassembly::ScrollDown()
{
	isTrackingPC = false;
	changedByUser = true;
	
//	LOGD("CViewDisassembly::ScrollDown: cursorAddress=%4.4x nextOpAddr=%4.4x", cursorAddress, nextOpAddr);
	
	cursorAddress = nextOpAddr;
	if (cursorAddress > dataAdapter->AdapterGetDataLength()-1)
		cursorAddress = dataAdapter->AdapterGetDataLength()-1;
	
	//LOGD("            set cursorAddress=%4.4x", cursorAddress);
	
	UpdateDisassembly(this->cursorAddress, this->cursorAddress + 0x0100);
}

void CViewDisassembly::ScrollUp()
{
	isTrackingPC = false;
	changedByUser = true;
	
//	LOGD("CViewDisassembly::ScrollUp: cursorAddress=%4.4x previousOpAddr=%4.4x", cursorAddress, previousOpAddr);
	
	if (cursorAddress == previousOpAddr)
	{
		previousOpAddr -= 3;
		
		//LOGD("........ previousOpAddr-3=%4.4x", previousOpAddr);
	}
	
	cursorAddress = previousOpAddr;
	if (cursorAddress < 0)
		cursorAddress = 0;
	
	//LOGD("          set cursorAddress=%4.4x", cursorAddress);
	
	UpdateDisassembly(this->cursorAddress, this->cursorAddress + 0x0100);
}

bool CViewDisassembly::DoScrollWheel(float deltaX, float deltaY)
{
	//LOGD("CViewDisassembly::DoScrollWheel: %f %f", deltaX, deltaY);
	
	if (this->IsInside(guiMain->mousePosX, guiMain->mousePosY) == false)
	{
		return false;
	}
	
	int dy = fabs(round(deltaY));
	
	if (guiMain->isShiftPressed)
	{
		dy *= 10;
	}
	
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
	
	return false;
}

void CViewDisassembly::MakeJMPToCursor()
{
	this->debugInterface->MakeJmpNoReset(this->dataAdapter, this->cursorAddress);
}

void CViewDisassembly::MakeJMPToAddress(int address)
{
	this->debugInterface->MakeJmpNoReset(this->dataAdapter, address);
}

void CViewDisassembly::SetBreakpointPC(int address, bool setOn)
{
	// keep copy to not lock mutex during rendering
	debugInterface->LockMutex();

	CDebugBreakpointsAddr *breakpoints = symbols->currentSegment->breakpointsPC;

	bool found = false;
	
	std::map<int, CDebugBreakpointAddr *>::iterator it2 = breakpoints->renderBreakpoints.find(address);
	if (it2 != breakpoints->renderBreakpoints.end())
	{
		if (setOn)
		{
			// breakpoint is already there, do not create new
			LOGError("CViewDisassembly::SetBreakpointPC: breakpoint already existing addr=%04x", address);
		}
		else
		{
			// remove breakpoint
			LOGD("CViewDisassembly::SetBreakpointPC: remove breakpoint addr=%04x", address);
			
			breakpoints->renderBreakpoints.erase(it2);
			breakpoints->DeleteBreakpoint(address);
		}
		
		found = true;
	}
	
	if (found == false)
	{
		if (setOn)
		{
			// add breakpoint
			LOGD("CViewDisassembly::SetBreakpointPC: add breakpoint addr=%04x", address);
			
			CDebugBreakpointAddr *breakpoint = new CDebugBreakpointAddr(address);
			breakpoint->actions = ADDR_BREAKPOINT_ACTION_STOP;
			
			breakpoints->AddBreakpoint(breakpoint);
			breakpoints->renderBreakpoints[address] = breakpoint;

		}
		else
		{
			LOGError("CViewDisassembly::SetBreakpointPC: breakpoint not found addr=%04x", address);
		}
	}
	
	debugInterface->UnlockMutex();
}

void CViewDisassembly::SetBreakpointPCIsActive(int address, bool setActive)
{
	debugInterface->LockMutex();
	
	CDebugBreakpointsAddr *breakpoints = symbols->currentSegment->breakpointsPC;
	std::map<int, CDebugBreakpointAddr *>::iterator itBp = breakpoints->renderBreakpoints.find(address);
	if (itBp != breakpoints->renderBreakpoints.end())
	{
		CDebugBreakpointAddr *bp = itBp->second;
		bp->isActive = setActive;
	}
	else
	{
		// breakpoint not found... this is error, should never happen
		LOGError("CViewDisassembly::SetBreakpointPCIsActive: breakpoint at %x not found", address);
		
		// as a workaround create one
		this->SetBreakpointPC(address, true);
		std::map<int, CDebugBreakpointAddr *>::iterator itBp = breakpoints->renderBreakpoints.find(address);
		if (itBp != breakpoints->renderBreakpoints.end())
		{
			CDebugBreakpointAddr *bp = itBp->second;
			bp->isActive = setActive;
		}
	}

	debugInterface->UnlockMutex();
}

void CViewDisassembly::Assemble(int assembleAddress)
{
	// remove all '$'  - assembling is default to hex only
	char *lineBuffer = SYS_GetCharBuf();
	
	int l = strlen(editBoxText->textBuffer);
	char *ptr = lineBuffer;
	for (int i = 0; i < l; i++)
	{
		if (editBoxText->textBuffer[i] == '$')
			continue;
		
		*ptr = editBoxText->textBuffer[i];
		ptr++;
	}
	
	*ptr = 0x00;
	
	if (lineBuffer[0] == 0x00)
	{
		SYS_ReleaseCharBuf(lineBuffer);
		return;
	}
	
	// allow JMP <enter>, with no arguments
	if (!strcasecmp(lineBuffer, "JMP") || !strcasecmp(lineBuffer, "JMP*") || !strcasecmp(lineBuffer, "JMP *"))
	{
		sprintf(lineBuffer, "JMP %04x", cursorAddress);
	}
	
	Assemble(assembleAddress, lineBuffer, true);
	
	SYS_ReleaseCharBuf(lineBuffer);
	
}

int CViewDisassembly::Assemble(int assembleAddress, char *lineBuffer, bool showMessage)
{
	int instructionOpcode = -1;
	uint16 instructionValue = 0x0000;
	char *errorMessage = SYS_GetCharBuf();
	
	int ret = this->Assemble(assembleAddress, lineBuffer, &instructionOpcode, &instructionValue, errorMessage);
	
	if (ret == -1)
	{
		// error
		if (showMessage)
		{
			viewC64->ShowMessageError(errorMessage);
		}
		SYS_ReleaseCharBuf(errorMessage);
		return -1;
	}
	
	bool isDataAvailable;
	
	switch (opcodes[instructionOpcode].addressingLength)
	{
		case 1:
		{
			dataAdapter->AdapterWriteByte(assembleAddress, instructionOpcode, &isDataAvailable);
			CDebugMemoryCell *cell = debugMemory->GetMemoryCell(assembleAddress);
			cell->isExecuteCode = true;
			break;
		}
			
		case 2:
		{
			dataAdapter->AdapterWriteByte(assembleAddress, instructionOpcode, &isDataAvailable);
			CDebugMemoryCell *cell = debugMemory->GetMemoryCell(assembleAddress);
			cell->isExecuteCode = true;
			assembleAddress++;
			dataAdapter->AdapterWriteByte(assembleAddress, (instructionValue & 0xFFFF), &isDataAvailable);
			break;
		}
			
		case 3:
		{
			dataAdapter->AdapterWriteByte(assembleAddress, instructionOpcode, &isDataAvailable);
			CDebugMemoryCell *cell = debugMemory->GetMemoryCell(assembleAddress);
			cell->isExecuteCode = true;
			assembleAddress++;
			dataAdapter->AdapterWriteByte(assembleAddress, (instructionValue & 0x00FF), &isDataAvailable);
			assembleAddress++;
			dataAdapter->AdapterWriteByte(assembleAddress, ((instructionValue >> 8) & 0x00FF), &isDataAvailable);
			break;
		}
			
		default:
			// should never happen
			viewC64->ShowMessageError(("Assemble failed"));
			LOGError("CViewDisassembly::error: %s", ("Assemble failed"));
			isErrorCode = true;
			SYS_ReleaseCharBuf(errorMessage);
			return -1;
	}
	
	SYS_ReleaseCharBuf(errorMessage);
	
	return opcodes[instructionOpcode].addressingLength;
}

#define FAIL(ErrorMessage)  		delete textParser; \
									strcpy(errorMessageBuf, (ErrorMessage)); \
									LOGError("CViewDisassembly::error: %s lineBuffer=%s", (ErrorMessage), lineBuffer); \
									isErrorCode = true; \
									return -1;


int CViewDisassembly::Assemble(int assembleAddress, char *lineBuffer, int *instructionOpCode, uint16 *instructionValue, char *errorMessageBuf)
{
	CSlrTextParser *textParser = new CSlrTextParser(lineBuffer);
	textParser->ToUpper();
	
	isErrorCode = false;
	
	char mnemonic[4] = {0x00};
	textParser->GetChars(mnemonic, 3);
	
	int baseOp = AssembleFindOp(mnemonic);
	
	if (baseOp < 0)
	{
		FAIL("Unknown mnemonic");
	}
	
	AssembleToken token = AssembleGetToken(textParser);
	if (token == TOKEN_UNKNOWN)
	{
		FAIL("Bad instruction");
	}
	
	OpcodeAddressingMode addressingMode = ADDR_UNKNOWN;
	
	// BRK
	if (token == TOKEN_EOF)
	{
		addressingMode = ADDR_IMP;
	}
	// LDA $00...
	else if (token == TOKEN_HEX_VALUE)
	{
		*instructionValue = textParser->GetHexNumber();

		token = AssembleGetToken(textParser);

		if (token == TOKEN_UNKNOWN)
		{
			FAIL("Bad instruction");
		}
		// LDA $0000
		else if (token == TOKEN_EOF)
		{
			if (*instructionValue < 0x0100)
			{
				addressingMode = ADDR_ZP;
			}
			else
			{
				addressingMode = ADDR_ABS;
			}
		}
		// LDA $0000,
		else if (token == TOKEN_COMMA)
		{
			token = AssembleGetToken(textParser);
			
			// LDA $0000,X
			if (token == TOKEN_X)
			{
				if (*instructionValue < 0x0100)
				{
					addressingMode = ADDR_ZPX;
				}
				else
				{
					addressingMode = ADDR_ABX;
				}
				
				// check end of line
				token = AssembleGetToken(textParser);
				if (token != TOKEN_EOF)
				{
					FAIL("Extra tokens at end of line");
				}
			}
			// LDA $0000,Y
			else if (token == TOKEN_Y)
			{
				if (*instructionValue < 0x0100)
				{
					addressingMode = ADDR_ZPY;
				}
				else
				{
					addressingMode = ADDR_ABY;
				}

				// check end of line
				token = AssembleGetToken(textParser);
				if (token != TOKEN_EOF)
				{
					FAIL("Extra tokens at end of line");
				}
			}
			else
			{
				FAIL("X or Y expected");
			}
		}
		else
		{
			FAIL("Bad instruction");
		}
	}
	// LDA #$00
	else if (token == TOKEN_IMMEDIATE)
	{
		token = AssembleGetToken(textParser);
		
		if (token == TOKEN_HEX_VALUE)
		{
			*instructionValue = textParser->GetHexNumber();
			addressingMode = ADDR_IMM;
			
			token = AssembleGetToken(textParser);
			if (token != TOKEN_EOF)
			{
				FAIL("Extra tokens at end of line");
			}
		}
		else
		{
			FAIL("Not a number after #")
		}
	}
	// LDA (
	else if (token == TOKEN_LEFT_PARENTHESIS)
	{
		token = AssembleGetToken(textParser);
		
		// LDA ($00...
		if (token == TOKEN_HEX_VALUE)
		{
			*instructionValue = textParser->GetHexNumber();
			
			token = AssembleGetToken(textParser);
			
			// LDA ($00)...
			if (token == TOKEN_RIGHT_PARENTHESIS)
			{
				token = AssembleGetToken(textParser);
				if (token == TOKEN_EOF)
				{
					addressingMode = ADDR_IND;
				}
				else if (token == TOKEN_COMMA)
				{
					token = AssembleGetToken(textParser);
					
					// LDA ($00),Y
					if (token == TOKEN_Y)
					{
						addressingMode = ADDR_IZY;
						token = AssembleGetToken(textParser);
						if (token != TOKEN_EOF)
						{
							FAIL("Extra tokens at end of line");
						}
					}
					else
					{
						FAIL("Only Y allowed");
					}
				}
				else
				{
					FAIL("Bad instructin");
				}
			}
			// LDA ($00,X)
			else if (token == TOKEN_COMMA)
			{
				token = AssembleGetToken(textParser);
				if (token == TOKEN_X)
				{
					token = AssembleGetToken(textParser);
					if (token == TOKEN_RIGHT_PARENTHESIS)
					{
						addressingMode = ADDR_IZX;
						token = AssembleGetToken(textParser);
						if (token != TOKEN_EOF)
						{
							FAIL("Extra tokens at end of line");
						}
					}
					else
					{
						FAIL(") expected");
					}
				}
				else
				{
					FAIL("Only X allowed");
				}
			}
			else
			{
				FAIL(") or , expected");
			}
		}
		else
		{
			FAIL("Number expected");
		}
	}
	else
	{
		FAIL("Bad instruction");
	}
	
	// check branching
	if (addressingMode == ADDR_ABS || addressingMode == ADDR_ZP)
	{
		// check if branch opcode exists
		*instructionOpCode = AssembleFindOp(mnemonic, ADDR_REL);
		if (*instructionOpCode != -1)
		{
			addressingMode = ADDR_REL;
			int16 branchValue = ((*instructionValue) - (assembleAddress + 2)) & 0xFFFF;
			if (branchValue < -0x80 || branchValue > 0x7F)
			{
				FAIL("Branch address too far");
			}
			*instructionValue = branchValue & 0x00FF;
		}
	}
	
	if (*instructionOpCode == -1)
	{
		*instructionOpCode = AssembleFindOp(mnemonic, addressingMode);
		
		if (*instructionOpCode == -1)
		{
			*instructionOpCode = AssembleFindOp(mnemonic, ADDR_ABS);
		}
	}

	// found opcode?
	if (*instructionOpCode != -1)
	{
		delete textParser;
		return 0;
	}

	FAIL("Instruction not found");
}

int CViewDisassembly::AssembleFindOp(char *mnemonic)
{
	for (int i = 0; i < 256; i++)
	{
		const char *m = opcodes[i].name;
		if (!strcmp(mnemonic, m))
			return i;
	}
	
	return -1;
}

int CViewDisassembly::AssembleFindOp(char *mnemonic, OpcodeAddressingMode addressingMode)
{
	// try to find standard opcode first
	for (int i =0; i < 256; i++)
	{
		const char *m = opcodes[i].name;
		if (!strcmp(mnemonic, m) && opcodes[i].addressingMode == addressingMode
			&& opcodes[i].isIllegal == false)
			return i;
	}

	// then illegals
	for (int i =0; i < 256; i++)
	{
		const char *m = opcodes[i].name;
		if (!strcmp(mnemonic, m) && opcodes[i].addressingMode == addressingMode)
			return i;
	}
	
	return -1;
}

AssembleToken CViewDisassembly::AssembleGetToken(CSlrTextParser *textParser)
{
	textParser->ScrollWhiteChars();
	
	char chr = textParser->GetChar();
	
	if (chr == 0x00)
		return TOKEN_EOF;
	
	if (FUN_IsHexDigit(chr))
	{
		textParser->ScrollBack();
		return TOKEN_HEX_VALUE;
	}
	
	if (chr == '#')
		return TOKEN_IMMEDIATE;
	
	if (chr == '(')
		return TOKEN_LEFT_PARENTHESIS;
	
	if (chr == ')')
		return TOKEN_RIGHT_PARENTHESIS;
	
	if (chr == ',')
		return TOKEN_COMMA;
	
	if (chr == 'X')
		return TOKEN_X;

	if (chr == 'Y')
		return TOKEN_Y;
	
	return TOKEN_UNKNOWN;
}

//

void CViewDisassembly::CreateAddrPositions()
{
	if (addrPositions != NULL)
		delete [] addrPositions;
	addrPositions = NULL;
	
	int numVisibleLines = (sizeY / fontSize);
	
	numAddrPositions = numVisibleLines*2 + numberOfLinesBack + 1;	// additional for scrolling

	if (numAddrPositions < 5)
	{
		numAddrPositions = 5;
	}
	
//	LOGD("CViewDisassembly::CreateAddrPositions: numAddrPositions=%d", numAddrPositions);
	if (numAddrPositions > 0)
	{
		addrPositions = new addrPosition_t[numAddrPositions];
	}
}


////////////

//
// this is simpler (quicker) version of disassembler that is not execute-aware:
//

void CViewDisassembly::CalcDisassembleStartNotExecuteAware(int startAddress, int *newStart, int *renderLinesBefore)
{
	//	LOGD("====================================== CalcDisassembleStart startAddress=%4.4x", startAddress);
	
	uint8 op[3];
	
	int newAddress = startAddress - numberOfLinesBack3;	// numLines*3
	if (newAddress < 0)
		newAddress = 0;
	
	int numRenderLines = 0;
	
	bool found = false;
	while(newAddress < startAddress)
	{
		//		LOGD("newAddress=%4.4x", newAddress);
		
		int checkAddress = newAddress;
		
		numRenderLines = 0;
		
		// scroll down
		while (true)
		{
			int adr = checkAddress;
			
			//			LOGD("  checkAddress=%4.4x", adr);
			for (int i=0; i<3; i++, adr++)
			{
				op[i] = memory[adr % memoryLength];
			}
			
			//F			byte mode = opcodes[op[0]].addressingMode;
			//F			checkAddress += disassembleAdrLength[mode];
			
			checkAddress += opcodes[op[0]].addressingLength;
			
			numRenderLines++;
			
			//			LOGD("  new checkAddress=%4.4x", adr);
			
			if (checkAddress >= startAddress)
			{
				//				LOGD("  ... checkAddress=%4.4x >= startAddress=%4.4x", checkAddress, startAddress);
				break;
			}
		}
		
		//		LOGD("checkAddress=%4.4x == startAddress=%4.4x?", checkAddress, startAddress);
		if (checkAddress == startAddress)
		{
			//LOGD("!! found !! newAddress=%4.4x numRenderLines=%d", newAddress, numRenderLines);
			found = true;
			break;
		}
		
		newAddress += 1;
		//
		//		LOGD("not found, newAddress=%4.4x", newAddress);
	}
	
	if (!found)
	{
		//
		//LOGD("*** FAILED ***");
		newAddress = startAddress; // - (float)numLines*1.5f;
		numRenderLines = 0;
		
		//viewC64->fontDisassembly->BlitText("***FAILED***", 100, 300, -1, 20);
	}
	//	else
	//	{
	////		LOGD("!!! FOUND !!!");
	//	}
	
	*newStart = newAddress;
	*renderLinesBefore = numRenderLines;
}

void CViewDisassembly::RenderDisassemblyNotExecuteAware(int startAddress, int endAddress)
{
	bool done = false;
	short i;
	uint8 op[3];
	uint16 adr;
	
	float px = posX;
	float py = posY;
	
	int renderAddress = startAddress;
	int renderLinesBefore;
	
	if (startAddress < 0)
		startAddress = 0;
	if (endAddress > 0xFFFF)
		endAddress = 0xFFFF;
	
	UpdateLocalMemoryCopy(startAddress, endAddress);
	
	CalcDisassembleStartNotExecuteAware(startAddress, &renderAddress, &renderLinesBefore);
	
	//LOGD("startAddress=%4.4x numberOfLinesBack=%d | renderAddress=%4.4x  renderLinesBefore=%d", startAddress, numberOfLinesBack, renderAddress, renderLinesBefore);
	
	renderSkipLines = numberOfLinesBack - renderLinesBefore;
	int skipLines = renderSkipLines;
	
	{
		py += (float)(skipLines) * fontSize;
	}
	
	
	startRenderY = py;
	startRenderAddr = renderAddress;
	endRenderAddr = endAddress;
	
	
	if (renderLinesBefore == 0)
	{
		previousOpAddr = startAddress - 1;
	}
	
	do
	{
		//LOGD("renderAddress=%4.4x l=%4.4x", renderAddress, memoryLength);
		if (renderAddress >= memoryLength)
			break;
		
		adr = renderAddress;
		
		for (i=0; i<3; i++, adr++)
		{
			if (adr == endAddress)
			{
				done = true;
			}
			
			op[i] = memory[adr];
		}
		
		{
			// bug: workaround
			if (py >= posY-2.0f && py <= posY+2.0f)
			{
				renderStartAddress = renderAddress;
				//LOGD("renderStartAddress=%4.4x", renderStartAddress);
			}
			int numBytesPerOp = RenderDisassemblyLine(px, py, renderAddress, op[0], op[1], op[2]);
			
			int newAddress = renderAddress + numBytesPerOp;
			if (newAddress == startAddress)
			{
				previousOpAddr = renderAddress;
			}
			
			if (renderAddress == startAddress)
			{
				nextOpAddr = renderAddress+numBytesPerOp;
			}
			
			renderAddress += numBytesPerOp;
			
			py += fontSize;
			
			if (py > posEndY)
				break;
			
		}
	}
	while (!done);
	
	// disassemble up?
	int length;
	
	if (skipLines > 0)
	{
		py = startRenderY;
		renderAddress = startRenderAddr;
		
		while (skipLines > 0)
		{
			py -= fontSize;
			skipLines--;
			
			if (renderAddress < 0)
				break;
			
			// check how much scroll up
			byte op, lo, hi;
			
			// check -3
			if (renderAddress > 2)
			{
				op = memory[renderAddress-3];
				length = opcodes[op].addressingLength;
				
				if (length == 3)
				{
					lo = memory[renderAddress-2];
					hi = memory[renderAddress-1];
					RenderDisassemblyLine(px, py, renderAddress-3, op, lo, hi);
					
					renderAddress -= 3;
					continue;
				}
			}
			
			// check -2
			if (renderAddress > 1)
			{
				op = memory[renderAddress-2];
				
				length = opcodes[op].addressingLength;
				
				if (length == 2)
				{
					lo = memory[renderAddress-1];
					RenderDisassemblyLine(px, py, renderAddress-2, op, lo, lo);
					
					renderAddress -= 2;
					continue;
				}
			}
			
			// check -1
			if (renderAddress > 0)
			{
				op = memory[renderAddress-1];
				length = opcodes[op].addressingLength;
				
				if (length == 1)
				{
					RenderDisassemblyLine(px, py, renderAddress-1, op, 0x00, 0x00);
					
					renderAddress -= 1;
					continue;
				}
			}
			
			// not found compatible op, just render hex
			if (renderAddress > 0)
			{
				renderAddress -= 1;
				RenderHexLine(px, py, renderAddress);
			}
		}
	}
	
	
	// this is in the center - show cursor
	if (isTrackingPC == false)
	{
		py = numberOfLinesBack * fontSize + posY;
		BlitRectangle(px, py, -1.0f, sizeX, fontSize, 0.3, 1.0, 0.3, 0.5f, 0.7f);
	}
	
	//	LOGD("previousOpAddr=%4.4x nextOpAddr=%4.4x", previousOpAddr, nextOpAddr);
	
}

void CViewDisassembly::UpdateDisassemblyNotExecuteAware(int startAddress, int endAddress)
{
	bool done = false;
	short i;
	uint8 op[3];
	uint16 adr;
	
	float px = posX;
	float py = posY;
	
	//	if (!range_args(31))  // 32 bytes unless end address specified
	//		return;
	
	int renderAddress = startAddress;
	int renderLinesBefore;
	
	CalcDisassembleStartNotExecuteAware(startAddress, &renderAddress, &renderLinesBefore);
	
	//LOGD("startAddress=%4.4x renderAddress=%4.4x  renderLinesBefore=%d", startAddress, renderAddress, renderLinesBefore);
	
	int numSkipLines = renderLinesBefore - numberOfLinesBack;
	
	if (renderLinesBefore <= 10)
	{
		py += (float)(numberOfLinesBack - renderLinesBefore) * fontSize;
	}
	
	//LOGD("numSkipLines=%d", numSkipLines);
	
	do
	{
		//LOGD("renderAddress=%4.4x l=%4.4x", renderAddress, dataAdapter->AdapterGetDataLength());
		if (renderAddress >= dataAdapter->AdapterGetDataLength())
			break;
		
		adr = renderAddress;
		
		for (i=0; i<3; i++, adr++)
		{
			if (adr == endAddress)
			{
				done = true;
			}
			
			byte v;
			dataAdapter->AdapterReadByte(adr, &v);
			op[i] = v;
		}
		
		if (numSkipLines > 0)
		{
			numSkipLines--;
			
			renderAddress += opcodes[op[0]].addressingLength;
		}
		else
		{
			// bug: workaround
			if (py >= posY-2.0f && py <= posY+2.0f)
			{
				renderStartAddress = renderAddress;
				//LOGD("renderStartAddress=%4.4x", renderStartAddress);
			}
			int numBytesPerOp = opcodes[op[0]].addressingLength;
			
			int newAddress = renderAddress + numBytesPerOp;
			if (newAddress == startAddress)
			{
				previousOpAddr = renderAddress;
			}
			
			if (renderAddress == startAddress)
			{
				nextOpAddr = renderAddress+numBytesPerOp;
			}
			
			renderAddress += numBytesPerOp;
			
			py += fontSize;
			
			if (py > posEndY)
				break;
			
		}
	}
	while (!done);
	
}

bool CViewDisassembly::DoTapNotExecuteAware(float x, float y)
{
	LOGG("CViewC64Disassemble::DoTapNotExecuteAware:  x=%f y=%f", x, y);
	
	float py = posY;
	
	u16 renderAddress = renderStartAddress;
	uint8 op[3];
	uint16 adr;
	
	while(true)
	{
		adr = renderAddress;
		
		//LOGD("y=%f py=%f renderAddress=%4.4x", y, py, renderAddress);
		
		if (y >= py && y <= (py + fontSize))
		{
			TogglePCBreakpoint(renderAddress);
			break;
		}
		
		
		for (int i=0; i<3; i++, adr++)
		{
			dataAdapter->AdapterReadByte(adr, &(op[i]));
		}
		
		renderAddress += opcodes[op[0]].addressingLength;
		
		py += fontSize;
		
		if (py > SCREEN_HEIGHT)
			break;
	}
	
	return true;
	
	return CGuiView::DoTap(x, y);
}

/////

void CViewDisassembly::PasteHexValuesFromClipboard()
{
	LOGD("CViewDisassembly::PasteHexValuesFromClipboard");

	u16 currentAssembleAddress = this->cursorAddress;

	//	int CViewDisassembly::Assemble(int assembleAddress, char *lineBuffer)

	CSlrString *pasteStr = SYS_GetClipboardAsSlrString();
	pasteStr->DebugPrint("pasteStr=");
	
	pasteStr->RemoveCharacter('$');

	std::list<u16> splitChars;
	splitChars.push_back(' ');
	splitChars.push_back('\n');
	splitChars.push_back('\r');
	splitChars.push_back('\t');
	
	std::vector<CSlrString *> *strs = pasteStr->Split(splitChars);
	
	for (std::vector<CSlrString *>::iterator it = strs->begin(); it != strs->end(); it++)
	{
		CSlrString *hexVal = *it;

		CDataAdapter *dataAdapter = debugInterface->GetDataAdapter();

		if (hexVal->IsHexValue() == true)
		{
			int len = hexVal->GetLength();
			
			if (len == 4 || len == 3)
			{
				// 4 hex digits mean an address
				currentAssembleAddress = hexVal->ToIntFromHex();
			}
			
			if (len == 2 || len == 1)
			{
				dataAdapter->AdapterWriteByte(currentAssembleAddress, hexVal->ToIntFromHex());
				currentAssembleAddress++;
			}
			
			continue;
		}
		else
		{
			// mnemonic
			CSlrString *assembleStr = new CSlrString(hexVal);
			char *cAssembleStr = assembleStr->GetStdASCII();

			assembleStr->DebugPrint("assembleStr1=");

			int opLen = this->Assemble(currentAssembleAddress, cAssembleStr, false);
			
			if (opLen == -1)
			{
				delete [] cAssembleStr;
				
				it++;
				
				if (it != strs->end())
				{
					CSlrString *argument = *it;
					assembleStr->Concatenate(' ');
					assembleStr->Concatenate(argument);
				}
				
				assembleStr->DebugPrint("assembleStr2=");
				cAssembleStr = assembleStr->GetStdASCII();

				opLen = this->Assemble(currentAssembleAddress, cAssembleStr, false);
			}
			
			delete [] cAssembleStr;
			
			if (opLen == -1)
				continue;
			
			currentAssembleAddress += opLen;
			
		}
	}
	
	while (!strs->empty())
	{
		CSlrString *str = strs->back();
		
		strs->pop_back();
		delete str;
	}
	
	this->ScrollToAddress(currentAssembleAddress);
	
	delete pasteStr;
}

void CViewDisassembly::CopyAssemblyToClipboard()
{
	LOGD("CViewDisassembly::CopyAssemblyToClipboard");
	
	u8 op = 0x00;
	u8 hi = 0x00;
	u8 lo = 0x00;
	bool isAvailable;
	
	// TODO: create wrapping adapter for post $FFFF-reads to wrap starting from $0000
	dataAdapter->AdapterReadByte(this->cursorAddress, &op, &isAvailable);
	dataAdapter->AdapterReadByte(this->cursorAddress + 1, &lo, &isAvailable);
	dataAdapter->AdapterReadByte(this->cursorAddress + 2, &hi, &isAvailable);
	
	char *buf = SYS_GetCharBuf();
	MnemonicWithDollarArgumentToStr(this->cursorAddress, op, lo, hi, buf);
	
	CSlrString *str = new CSlrString(buf);
	SYS_SetClipboardAsSlrString(str);
	delete str;

	char *buf2 = SYS_GetCharBuf();

	sprintf(buf2, "Copied %s", buf);
	viewC64->ShowMessageInfo(buf2);
	
	SYS_ReleaseCharBuf(buf);
	SYS_ReleaseCharBuf(buf2);
	
}

void CViewDisassembly::CopyHexAddressToClipboard()
{
	LOGD("CViewDisassembly::CopyHexAddressToClipboard");
	
	char *buf = SYS_GetCharBuf();
	sprintf(buf, "$%04X", this->cursorAddress);
	CSlrString *str = new CSlrString(buf);
	SYS_SetClipboardAsSlrString(str);
	delete str;
	
	sprintf(buf, "Copied $%04X", this->cursorAddress);
	viewC64->ShowMessageInfo(buf);
	
	SYS_ReleaseCharBuf(buf);
}

void CViewDisassembly::PasteKeysFromClipboard()
{
	LOGD("CViewDisassembly::PasteKeysFromClipboard");

	CSlrString *pasteStr = SYS_GetClipboardAsSlrString();
	pasteStr->DebugPrint("pasteStr=");

	for (int i = 0; i < pasteStr->GetLength(); i++)
	{
		u16 chr = pasteStr->GetChar(i);
		
		if ((chr >= '0' && chr <= '9')
			|| (chr >= 'a' && chr <= 'x')
			|| (chr >= 'A' && chr <= 'X')
			|| chr == ' ' || chr == '$' || chr == '#'
			|| chr == ',' || chr == '(' || chr == ')')
		
		this->KeyDown(chr, false, false, false, false);
	}
}

//bool CViewDisassembly::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
//{
//	LOGI("CViewDisassembly::KeyDown: %x %s %s %s", keyCode, STRBOOL(isShift), STRBOOL(isAlt), STRBOOL(isControl));
//	

//
bool CViewDisassembly::HasContextMenuItems()
{
	return true;
}

void CViewDisassembly::RenderContextMenuItems()
{
//	LOGD("=============================== CViewDisassembly::RenderContextMenuItems: %s", this->name);
	CGuiView::RenderContextMenuItems();
	
	CDebugMemoryCell *cell = debugMemory->GetMemoryCell(cursorAddress);
	
	localLabelText[0] = 0;
	CDebugSymbolsSegment *currentSegment = NULL;
	CDebugSymbolsCodeLabel *label = NULL;
	
//	LOGD("debugInterface=%x", debugInterface);
//	LOGD("debugInterface->symbols=%x this->symbols=%x", debugInterface->symbols, this->symbols);
//	LOGD("this->symbols->currentSegment=%x | debugInterface->symbols->currentSegment=%x" , this->symbols->currentSegment, debugInterface->symbols->currentSegment);

	if (symbols && symbols->currentSegment)
	{
		currentSegment = symbols->currentSegment;
		label = currentSegment->FindLabel(cursorAddress);
		
//		LOGD("FindLabel: cursorAddress=%04x label=%x", cursorAddress, label);
		if (label)
		{
			strcpy(localLabelText, label->GetLabelText());
		}
	}

	char *buf = SYS_GetCharBuf();
	sprintf(buf, "Address %04X", cursorAddress);
	ImGui::Text(buf);
	
	ImGui::SameLine();
		
	// TODO: this code to show and update labels for memory cell is used in multiple places (memorydump, memorymap, disassembly), move me
	if (currentSegment)
	{
		bool edited = false;
		if (ImGui::InputText("##disassemblyLabelText", localLabelText, MAX_STRING_LENGTH))
		{
			edited = true;
		}
		
		ImGuiIO& io = ImGui::GetIO();
		if (ImGui::IsItemDeactivatedAfterEdit())
		{
			if (label != NULL)
			{
				// find if there is other label with the same text
				CDebugSymbolsCodeLabel *existingLabel = currentSegment->FindLabelByText(localLabelText);
				if (existingLabel != NULL && existingLabel != label)
				{
//					printf("existingL=%x label=%x setText=%s\n", existingLabel, label, localLabelText);

					currentSegment->RemoveCodeLabel(label, true);
					delete label;
					
					currentSegment->RemoveCodeLabel(existingLabel, false);
					
					// TODO: observer for watch and breakpoint when observing label for custom breakpoints
					
					// update watch
					CDebugSymbolsDataWatch *watch = currentSegment->FindWatch(existingLabel->address);
					if (watch)
					{
						currentSegment->RemoveWatch(existingLabel->address);
					}
					
					// update breakpoint PC
					CDebugBreakpointAddr *breakpointPC = currentSegment->breakpointsPC->GetBreakpoint(existingLabel->address);
					if (breakpointPC)
					{
						currentSegment->breakpointsPC->RemoveBreakpoint(breakpointPC);
					}
					
					// update breakpoint memory
					CDebugBreakpointAddr *breakpointMemory = currentSegment->breakpointsData->GetBreakpoint(existingLabel->address);
					if (breakpointMemory)
					{
						currentSegment->breakpointsData->RemoveBreakpoint(breakpointMemory);
					}

					// update label
					existingLabel->address = cursorAddress;
					currentSegment->AddCodeLabel(existingLabel, true);
					
					// add back symbols
					if (watch)
					{
						watch->address = cursorAddress;
						currentSegment->AddWatch(watch);
					}
					
					if (breakpointPC)
					{
						breakpointPC->addr = cursorAddress;
						currentSegment->breakpointsPC->AddBreakpoint(breakpointPC);
					}

					if (breakpointMemory)
					{
						breakpointMemory->addr = cursorAddress;
						currentSegment->breakpointsData->AddBreakpoint(breakpointMemory);
					}
				}
				else
				{
					label->SetText(localLabelText);
				}
			}
			else
			{
				// find if there is other label with the same text
				CDebugSymbolsCodeLabel *existingLabel = currentSegment->FindLabelByText(localLabelText);
				if (existingLabel != NULL && existingLabel != label)
				{
					currentSegment->RemoveCodeLabel(label, true);
					delete label;
					existingLabel->address = cursorAddress;
				}
			}
		}
		else if (edited)
		{
			SetIsTrackingPC(false);
					
			if (label != NULL)
			{
				if (localLabelText[0] == 0)
				{
					currentSegment->RemoveCodeLabel(label, true);
					delete label;
				}
				else
				{
					
					// find if there is other label with the same text
					CDebugSymbolsCodeLabel *existingLabel = currentSegment->FindLabelByText(localLabelText);
					if (existingLabel == NULL)
					{
//						printf("existingL=%x label=%x setText=%s\n", existingLabel, label, localLabelText);
						label->SetText(localLabelText);
					}
				}
			}
			else
			{
				// TODO: refactor this: fix ownership of label text buffer
				char *labelText = STRALLOC(localLabelText);
				currentSegment->AddCodeLabel(cursorAddress, labelText);
			}
		}
	}
	
	ImGui::Separator();

	// #if defined(EXPERIMENTAL_EVENTS_HISTORY)
	/*
	bool canRewindToPreviousExecute = false;
	if (cell)
	{
		if (cell->executeHistory.size > 0)
		{
			canRewindToPreviousExecute = true;
		}
	}
	
	if (ImGui::MenuItem("Rewind to previous execute", "", false, canRewindToPreviousExecute))
	{
		
	}
	 ImGui::Separator();
	*/
	
	CGuiView::RenderContextMenuLayoutParameters(false);

//	if (ImGui::MenuItem("Execute-aware disassembly", NULL, &c64SettingsExecuteAwareDisassembly))
//	{
//		C64DebuggerStoreSettings();
//	}
	
	SYS_ReleaseCharBuf(buf);
}

// Layout
void CViewDisassembly::Serialize(CByteBuffer *byteBuffer)
{
}

void CViewDisassembly::Deserialize(CByteBuffer *byteBuffer)
{
}


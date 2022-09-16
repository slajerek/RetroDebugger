extern "C" {
	#include "ViceWrapper.h"
}
#include "CVicEditorLayerVirtualSprites.h"
#include "VID_Main.h"
#include "C64SpriteHires.h"
#include "C64SpriteMulti.h"
#include "CViewC64VicDisplay.h"
#include "CViewVicEditor.h"
#include "CGuiMain.h"
#include "CDebugInterfaceC64.h"
#include "C64SettingsStorage.h"
#include "C64VicDisplayCanvas.h"
#include "CViewC64Sprite.h"
#include "CViewC64Palette.h"


// TODO: refactor this and remove copy-pasted code...!


CVicEditorLayerVirtualSprites::CVicEditorLayerVirtualSprites(CViewVicEditor *vicEditor)
: CVicEditorLayer(vicEditor, "Sprites")
{
	LOGD("CVicEditorLayerVirtualSprites created");
	this->isVisible = true;
	
	showSpriteFrames = true;
	showSpriteGrid = true;
}

CVicEditorLayerVirtualSprites::~CVicEditorLayerVirtualSprites()
{
	
}

void CVicEditorLayerVirtualSprites::RenderMain(vicii_cycle_state_t *viciiState)
{
}

void CVicEditorLayerVirtualSprites::RenderPreview(vicii_cycle_state_t *viciiState)
{
}

void CVicEditorLayerVirtualSprites::RenderGridMain(vicii_cycle_state_t *viciiState)
{
	showSpriteGrid = vicEditor->viewVicDisplayMain->showGridLines;
	RenderGrid(viciiState, vicEditor->viewVicDisplayMain);
}

void CVicEditorLayerVirtualSprites::RenderGrid(vicii_cycle_state_t *viciiState, CViewC64VicDisplay *vicDisplay)
{
	for (std::list<C64Sprite *>::iterator it = this->sprites.begin();
		 it != this->sprites.end(); it++)
	{
		C64Sprite *sprite = *it;
		
		float x = sprite->posX;
		float y = sprite->posY;
		
		float px = vicDisplay->displayPosX + (float)x * vicDisplay->rasterScaleFactorX  + vicDisplay->rasterCrossOffsetX;
		float py = vicDisplay->displayPosY + (float)y * vicDisplay->rasterScaleFactorY  + vicDisplay->rasterCrossOffsetY;

		int spriteSizeX = sprite->isStretchedHorizontally ? 48 : 24;
		int spriteSizeY = sprite->isStretchedVertically ? 42 : 21;

		spriteSizeX *= vicDisplay->rasterScaleFactorX;
		spriteSizeY *= vicDisplay->rasterScaleFactorY;

		if (showSpriteFrames)
			BlitRectangle(px, py, vicDisplay->posZ, spriteSizeX, spriteSizeY, 1.0f, 0.3f, 0.3f, 1.0f, 1.0f);
		
		if (showSpriteGrid)
		{
			float lineWidth = 1.0f;
			float lw2 = lineWidth/2.0f;
			
			
			// TODO: optimize this
			
			int numSteps;
			float step;

			// vertical lines
			if (sprite->isMulti)
			{
				step = 2.0f * vicDisplay->rasterScaleFactorX;
			}
			else
			{
				step = 1.0f * vicDisplay->rasterScaleFactorX;
			}
			
			if (sprite->isStretchedHorizontally)
			{
				step *= 2;
			}
			
			float rasterX = px;
			for (int ix = 0; ix <= sprite->sizeX; ix++)
			{
				BlitFilledRectangle(rasterX - lw2, py, vicDisplay->posZ, lineWidth, spriteSizeY,
//									0.0f, 1.0f, 1.0f, 1.0f);
									vicDisplay->gridLinesColorR2, vicDisplay->gridLinesColorG2, vicDisplay->gridLinesColorB2, vicDisplay->gridLinesColorA2);
				
				rasterX += step;
			}
			

			// horizontal lines
			step = 1.0f * vicDisplay->rasterScaleFactorY;
			
			if (sprite->isStretchedVertically)
			{
				step *= 2;
			}
			
			float rasterY = py;
			for (int iy = 0; iy <= sprite->sizeY; iy++)
			{
				BlitFilledRectangle(px, rasterY - lw2, vicDisplay->posZ, spriteSizeX, lineWidth,
									vicDisplay->gridLinesColorR2, vicDisplay->gridLinesColorG2, vicDisplay->gridLinesColorB2, vicDisplay->gridLinesColorA2);

				rasterY += step;
			}
		}
	}

}

void CVicEditorLayerVirtualSprites::RenderGridPreview(vicii_cycle_state_t *viciiState)
{
	if (vicEditor->viewVicDisplaySmall->showGridLines)
	{
		RenderGrid(viciiState, vicEditor->viewVicDisplaySmall);
	}
}

void CVicEditorLayerVirtualSprites::ClearSprites()
{
	while (!this->sprites.empty())
	{
		C64Sprite *sprite = this->sprites.back();
		this->sprites.pop_back();
		delete sprite;
	}
}

// do full scan of sprites in this frame, i.e. search addresses where parameters were stored
void CVicEditorLayerVirtualSprites::FullScanSpritesInThisFrame()
{
	//LOGD("------------ CVicEditorLayerVirtualSprites::FullScanSpritesInThisFrame");
	
	if (c64SettingsVicStateRecordingMode != C64D_VICII_RECORD_MODE_EVERY_CYCLE)
	{
		return;
//		viewC64->ShowMessage("Set VIC recording to every cycle");
//		return;
	}
	
	guiMain->LockMutex(); //"CVicEditorLayerVirtualSprites::FullScanSpritesInThisFrame");
	
	CDebugInterfaceC64 *debugInterface = vicEditor->viewVicDisplayMain->debugInterface;
	
//	u8 debugMode = debugInterface->GetDebugMode();
//	
//	if (debugMode != C64_DEBUG_PAUSED)
//	{
//		debugInterface->SetDebugMode(C64_DEBUG_PAUSED);
//		SYS_Sleep(100);
//	}
	
	unsigned long t = SYS_GetCurrentTimeInMillis();

	/// scanning
	vicii_cycle_state_t storedState;
	c64d_vicii_copy_state(&storedState);
	
	vicii_cycle_state_t *viciiState = NULL;
	
	int pc = -1;
	
	int addrLDA = -1;
	int addrLDX = -1;
	int addrLDY = -1;
	
	int addrPosXHighBits = -1;
	int addrPosX[8] = { -1 };
	int addrPosY[8] = { -1 };
	int addrSetStretchHorizontal = -1;
	int addrSetStretchVertical = -1;
	int addrSetMultiColor = -1;
	int addrColorChangeCommon1 = -1;
	int addrColorChangeCommon2 = -1;
	int addrColorChange[8] = { -1 };
	
	for (int y = 0; y < 312; y++)
	{
		for (int cycle = 0; cycle < 63; cycle++)
		{
			viciiState = c64d_get_vicii_state_for_raster_cycle(y+1, cycle);
			
			pc = viciiState->pc;
			
//			if (y > 268 && y < 272)
//				LOGD("y=%-3d cycle=%-2d pc=%04x", y, cycle, pc);
			
			u8 opcode = debugInterface->GetByteC64(pc);
			
			if (opcode == 0xA9)
			{
				// LDA
				addrLDA = pc + 1;
			}
			else if (opcode == 0xA2)
			{
				// LDX
				addrLDX = pc + 1;
			}
			else if (opcode == 0xA0)
			{
				// LDY
				addrLDY = pc + 1;
			}
			else if (opcode == 0x8D
					 || opcode == 0x8E
					 || opcode == 0x8C)
			{
				u8 a1 = debugInterface->GetByteC64(pc+1);
				u8 a2 = debugInterface->GetByteC64(pc+2);
				int storeAddr = a1 | (a2 << 8);
				int loadAddr = -1;

				if (opcode == 0x8D)
				{
					// STA
					//LOGD("pc=%04x | STA=%04x, LDA =%04x", pc, storeAddr, addrLDA);
					
					loadAddr = addrLDA;
				}
				else if (opcode == 0x8E)
				{
					// STX
					//LOGD("pc=%04x | STX=%04x, LDX =%04x", pc, storeAddr, addrLDX);

					loadAddr = addrLDX;
				}
				else if (opcode == 0x8C)
				{
					// STY
					//LOGD("pc=%04x | STY=%04x, LDY =%04x", pc, storeAddr, addrLDY);
					
					loadAddr = addrLDY;
				}
				
				if (storeAddr == 0xD010)
				{
					addrPosXHighBits = loadAddr;
				}
				else if (storeAddr == 0xD000)
				{
					addrPosX[0] = loadAddr;
				}
				else if (storeAddr == 0xD002)
				{
					addrPosX[1] = loadAddr;
				}
				else if (storeAddr == 0xD004)
				{
					addrPosX[2] = loadAddr;
				}
				else if (storeAddr == 0xD006)
				{
					addrPosX[3] = loadAddr;
				}
				else if (storeAddr == 0xD008)
				{
					addrPosX[4] = loadAddr;
				}
				else if (storeAddr == 0xD00A)
				{
					addrPosX[5] = loadAddr;
				}
				else if (storeAddr == 0xD00C)
				{
					addrPosX[6] = loadAddr;
				}
				else if (storeAddr == 0xD00E)
				{
					addrPosX[7] = loadAddr;
				}
				
				else if (storeAddr == 0xD001)
				{
					addrPosY[0] = loadAddr;
				}
				else if (storeAddr == 0xD003)
				{
					addrPosY[1] = loadAddr;
				}
				else if (storeAddr == 0xD005)
				{
					addrPosY[2] = loadAddr;
				}
				else if (storeAddr == 0xD007)
				{
					addrPosY[3] = loadAddr;
				}
				else if (storeAddr == 0xD009)
				{
					addrPosY[4] = loadAddr;
				}
				else if (storeAddr == 0xD00B)
				{
					addrPosY[5] = loadAddr;
				}
				else if (storeAddr == 0xD00D)
				{
					addrPosY[6] = loadAddr;
				}
				else if (storeAddr == 0xD00F)
				{
					addrPosY[7] = loadAddr;
				}

				
				else if (storeAddr == 0xD01C)
				{
					addrSetMultiColor = loadAddr;
				}
				
				else if (storeAddr == 0xD027)
				{
					addrColorChange[0] = loadAddr;
				}
				else if (storeAddr == 0xD028)
				{
					addrColorChange[1] = loadAddr;
				}
				else if (storeAddr == 0xD029)
				{
					addrColorChange[2] = loadAddr;
				}
				else if (storeAddr == 0xD02A)
				{
					addrColorChange[3] = loadAddr;
				}
				else if (storeAddr == 0xD02B)
				{
					addrColorChange[4] = loadAddr;
				}
				else if (storeAddr == 0xD02C)
				{
					addrColorChange[5] = loadAddr;
				}
				else if (storeAddr == 0xD02D)
				{
					addrColorChange[6] = loadAddr;
				}
				else if (storeAddr == 0xD02E)
				{
					addrColorChange[7] = loadAddr;
				}
				
				else if (storeAddr == 0xD025)
				{
					addrColorChangeCommon1 = loadAddr;
				}
				else if (storeAddr == 0xD026)
				{
					addrColorChangeCommon2 = loadAddr;
				}
				
				else if (storeAddr == 0xD01D)
				{
					addrSetStretchHorizontal = loadAddr;
				}

				else if (storeAddr == 0xD017)
				{
					addrSetStretchVertical = loadAddr;
				}

			}
			
			

			// scan cycle
			for (int z = 0; z < 8; z++)
			{
				int x = cycle*8 + z;
				
				for (int i = 0; i < 8; i++)
				{
					int bits = viciiState->regs[0x15];

					bool enabled = ((bits >> i) & 1) ? true : false;
					
					if (!enabled)
						continue;

					int spriteX = viciiState->sprite[i].x;
					
					if (spriteX > 0x1F8)
						continue;
					
					// check1
					bool correctSpriteY = false;
					int spriteY = viciiState->regs[1 + (i << 1)];
					
					if (spriteY == (y-1))
					{
						correctSpriteY = true;
					}
					else if (y > 0xFF)
					{
						if (spriteY < 54)
						{
							if (spriteY < 30)
							{
								if (spriteY == (y % 0xFF)-1)
								{
									correctSpriteY = true;
								}
							}
//							else
//							{
//								spriteY = spriteY - 0x36;
//							}
//							
//							if (spriteY == (y-1))
//							{
//								correctSpriteY = true;
//							}
						}
					}
					
//					if (y > 268 && y < 272)
//						LOGD(" >>  y=%3d  x=%3d | id=%2d spriteX=%-03d spriteY=%-03d   (y=%s)", y, x, i, spriteX, spriteY, STRBOOL(correctSpriteY));
					
					if (spriteX == x && correctSpriteY)
					{
//						LOGD("found sprite: %d %d", spriteX, spriteY);
						
						float spriteDisplayX = viciiState->sprite[i].x;
						
						// sprites with x >= 488 are rendered as from 0 by VIC
						// confirmed by my old intro with sprites scroll on borders
						// but sprites with x >= 0x1F8 are not rendered at all as they are after last VIC raster line cycle
						
						if (spriteDisplayX >= 0x1F8)
							continue;
						
						// TODO: confirm the value 0x01D0
						if (spriteDisplayX >= 0x01D0)
						{
							// 488 = 8 - 24 = -16
							spriteDisplayX = (spriteDisplayX - 488.0f) - 16.0f;
						}
						
						spriteDisplayX -= 0x18;
						
						float sprPosY = viciiState->regs[1 + (i << 1)];
						float spriteDisplayY = sprPosY - 0x32;
						
						if (sprPosY < 54)
						{
							if (y > 0xFF)
							{
								spriteDisplayY = sprPosY - 0x31 + 0xFF;
							}
						}
						
						//
						bits = viciiState->regs[0x1d];
						bool stretchedX = ((bits >> i) & 1) ? true : false;
						
						bits = viciiState->regs[0x17];
						bool stretchedY = ((bits >> i) & 1) ? true : false;

						//
						int v_bank = viciiState->vbank_phi1;
						int pointerAddr = v_bank + viciiState->sprite[i].pointer * 64;

						// color?
						C64Sprite *sprite = NULL;
						
						if (viciiState->regs[0x1c] & (1<<i))
						{
							sprite = new C64SpriteMulti(this->vicEditor,
														spriteDisplayX, spriteDisplayY, stretchedX, stretchedY,
														viciiState->sprite[i].pointer, pointerAddr);
							sprite->spriteId = i;
						}
						else
						{
							sprite = new C64SpriteHires(this->vicEditor,
														spriteDisplayX, spriteDisplayY, stretchedX, stretchedY,
														viciiState->sprite[i].pointer, pointerAddr);
							sprite->spriteId = i;
						}

						bool newSprite = true;
						for (std::list<C64Sprite *>::iterator it = sprites.begin();
							 it != sprites.end(); it++)
						{
							C64Sprite *spr2 = *it;
							if (spr2->IsEqual(sprite))
							{
								newSprite = false;
								delete sprite;
								break;
							}
						}

						if (newSprite)
						{
							sprite->rasterLine = y;
							sprite->rasterCycle = cycle;
							
							sprite->addrPosXHighBits = addrPosXHighBits;
							sprite->addrPosX = addrPosX[i];
							sprite->addrPosY = addrPosY[i];
							sprite->addrSetStretchHorizontal = addrSetStretchHorizontal;
							sprite->addrSetStretchVertical = addrSetStretchVertical;
							sprite->addrSetMultiColor = addrSetMultiColor;
							sprite->addrColorChangeCommon1 = addrColorChangeCommon1;
							sprite->addrColorChangeCommon2 = addrColorChangeCommon2;
							sprite->addrColorChange = addrColorChange[i];

							this->sprites.push_back(sprite);
							//sprite->DebugPrint();
						}
					}
				}
				
			}
		}
	}


	
	///
	
	//debugInterface->SetDebugMode(debugMode);
	
	guiMain->UnlockMutex(); //"CVicEditorLayerVirtualSprites::FullScanSpritesInThisFrame");
	
	LOGD("------------ FINISHED CVicEditorLayerVirtualSprites::FullScanSpritesInThisFrame in %d ms", SYS_GetCurrentTimeInMillis() - t);

}

void CVicEditorLayerVirtualSprites::SimpleScanSpritesInThisFrame()
{
	LOGD("------------ CVicEditorLayerVirtualSprites::SimpleScanSpritesInThisFrame");
	
	if (c64SettingsVicStateRecordingMode != C64D_VICII_RECORD_MODE_EVERY_CYCLE)
	{
		return;
		//		viewC64->ShowMessage("Set VIC recording to every cycle");
		//		return;
	}
	
	guiMain->LockMutex(); //"CVicEditorLayerVirtualSprites::SimpleScanSpritesInThisFrame");
	
	CDebugInterfaceC64 *debugInterface = vicEditor->viewVicDisplayMain->debugInterface;
	
	//	u8 debugMode = debugInterface->GetDebugMode();
	//
	//	if (debugMode != C64_DEBUG_PAUSED)
	//	{
	//		debugInterface->SetDebugMode(C64_DEBUG_PAUSED);
	//		SYS_Sleep(100);
	//	}
	
	unsigned long t = SYS_GetCurrentTimeInMillis();
	
	/// scanning
	vicii_cycle_state_t storedState;
	c64d_vicii_copy_state(&storedState);
	
	vicii_cycle_state_t *viciiState = NULL;
	
	for (int y = 0; y < 311; y++)
	{
		for (int cycle = 0; cycle < 63; cycle++)
		{
			viciiState = c64d_get_vicii_state_for_raster_cycle(y+1, cycle);
			
			// scan cycle
			for (int z = 0; z < 8; z++)
			{
				int x = cycle*8 + z;
				
				for (int i = 0; i < 8; i++)
				{
					int bits = viciiState->regs[0x15];
					
					bool enabled = ((bits >> i) & 1) ? true : false;
					
					if (!enabled)
						continue;
					
					int spriteX = viciiState->sprite[i].x;
					
					if (spriteX > 0x1F8)
						continue;
					
					// check1
					bool correctSpriteY = false;
					int spriteY = viciiState->regs[1 + (i << 1)];
					
					if (spriteY == (y-1))
					{
						correctSpriteY = true;
					}
					else if (y > 0xFF)
					{
						if (spriteY < 54)
						{
							if (spriteY < 30)
							{
								if (spriteY == (y % 0xFF)-1)
								{
									correctSpriteY = true;
								}
							}
							//							else
							//							{
							//								spriteY = spriteY - 0x36;
							//							}
							//
							//							if (spriteY == (y-1))
							//							{
							//								correctSpriteY = true;
							//							}
						}
					}
					
					//					if (y > 268 && y < 272)
					//						LOGD(" >>  y=%3d  x=%3d | id=%2d spriteX=%-03d spriteY=%-03d   (y=%s)", y, x, i, spriteX, spriteY, STRBOOL(correctSpriteY));
					
					if (spriteX == x && correctSpriteY)
					{
						//						LOGD("found sprite: %d %d", spriteX, spriteY);
						
						float spriteDisplayX = viciiState->sprite[i].x;
						
						// sprites with x >= 488 are rendered as from 0 by VIC
						// confirmed by my old intro with sprites scroll on borders
						// but sprites with x >= 0x1F8 are not rendered at all as they are after last VIC raster line cycle
						
						if (spriteDisplayX >= 0x1F8)
							continue;
						
						// TODO: confirm the value 0x01D0
						if (spriteDisplayX >= 0x01D0)
						{
							// 488 = 8 - 24 = -16
							spriteDisplayX = (spriteDisplayX - 488.0f) - 16.0f;
						}
						
						spriteDisplayX -= 0x18;
						
						float sprPosY = viciiState->regs[1 + (i << 1)];
						float spriteDisplayY = sprPosY - 0x32;
						
						if (sprPosY < 54)
						{
							if (y > 0xFF)
							{
								spriteDisplayY = sprPosY - 0x31 + 0xFF;
							}
						}
						
						//
						bits = viciiState->regs[0x1d];
						bool stretchedX = ((bits >> i) & 1) ? true : false;
						
						bits = viciiState->regs[0x17];
						bool stretchedY = ((bits >> i) & 1) ? true : false;
						
						//
						int v_bank = viciiState->vbank_phi1;
						int pointerAddr = v_bank + viciiState->sprite[i].pointer * 64;
						
						LOGD("sprite %d->pointerAddr=%x", i, pointerAddr);
						// color?
						C64Sprite *sprite = NULL;
						
						if (viciiState->regs[0x1c] & (1<<i))
						{
							sprite = new C64SpriteMulti(this->vicEditor,
														spriteDisplayX, spriteDisplayY, stretchedX, stretchedY,
														viciiState->sprite[i].pointer, pointerAddr);
							sprite->spriteId = i;
						}
						else
						{
							sprite = new C64SpriteHires(this->vicEditor,
														spriteDisplayX, spriteDisplayY, stretchedX, stretchedY,
														viciiState->sprite[i].pointer, pointerAddr);
							sprite->spriteId = i;
						}
						
						bool newSprite = true;
						for (std::list<C64Sprite *>::iterator it = sprites.begin();
							 it != sprites.end(); it++)
						{
							C64Sprite *spr2 = *it;
							if (spr2->IsEqual(sprite))
							{
								spr2->pointerAddr = pointerAddr;

								newSprite = false;
								delete sprite;
								break;
							}
						}
						
						if (newSprite)
						{
							sprite->rasterLine = y;
							sprite->rasterCycle = cycle;
							sprite->pointerValue = viciiState->sprite[i].pointer;
							sprite->pointerAddr = pointerAddr;
							
							this->sprites.push_back(sprite);
							sprite->DebugPrint();
						}
					}
				}
				
			}
		}
	}
	
	
	
	///
	
	//debugInterface->SetDebugMode(debugMode);
	
	guiMain->UnlockMutex(); //"CVicEditorLayerVirtualSprites::SimpleScanSpritesInThisFrame");
	
	LOGD("------------ FINISHED CVicEditorLayerVirtualSprites::SimpleScanSpritesInThisFrame in %d ms", SYS_GetCurrentTimeInMillis() - t);
	
}

// just search for store addresses, this is for painting and color changes
void CVicEditorLayerVirtualSprites::ScanSpritesStoreAddressesOnly(int rx, int ry,
																  int *addrPosXHighBits,
																  int addrPosX[8],
																  int addrPosY[8],
																  int *addrSetStretchHorizontal,
																  int *addrSetStretchVertical,
																  int *addrSetMultiColor,
																  int *addrColorChangeCommon1,
																  int *addrColorChangeCommon2,
																  int addrColorChange[8])
																  

{
	LOGD("------------ CVicEditorLayerVirtualSprites::ScanSpritesStoreAddressesOnly, rx=%d ry=%d", rx, ry);
	
	if (c64SettingsVicStateRecordingMode != C64D_VICII_RECORD_MODE_EVERY_CYCLE)
	{
		return;
		//		viewC64->ShowMessage("Set VIC recording to every cycle");
		//		return;
	}
	
	guiMain->LockMutex(); //"CVicEditorLayerVirtualSprites::ScanSpritesStoreAddressesOnly");
	
	CDebugInterfaceC64 *debugInterface = vicEditor->viewVicDisplayMain->debugInterface;
	
	//	u8 debugMode = debugInterface->GetDebugMode();
	//
	//	if (debugMode != C64_DEBUG_PAUSED)
	//	{
	//		debugInterface->SetDebugMode(C64_DEBUG_PAUSED);
	//		SYS_Sleep(100);
	//	}
	
	unsigned long t = SYS_GetCurrentTimeInMillis();
	
	int searchRasterLine = ry + 0x32;
	int searchRasterCycle = (rx + 0x88)/8;
	
	
	LOGD("   | searchRasterLine=%d searchRasterCycle=%d", searchRasterLine, searchRasterCycle);
	
	/// scanning
	vicii_cycle_state_t storedState;
	c64d_vicii_copy_state(&storedState);
	
	vicii_cycle_state_t *viciiState = NULL;
	
	int pc = -1;
	
	int addrLDA = -1;
	int addrLDX = -1;
	int addrLDY = -1;
	
	for (int y = 0; y < 312; y++)
	{
		for (int cycle = 0; cycle < 63; cycle++)
		{
			viciiState = c64d_get_vicii_state_for_raster_cycle(y+1, cycle);
			
			pc = viciiState->pc;
			
			//			if (y > 268 && y < 272)
			//				LOGD("y=%-3d cycle=%-2d pc=%04x", y, cycle, pc);
			
			u8 opcode = debugInterface->GetByteC64(pc);
			
			if (opcode == 0xA9)
			{
				// LDA
				addrLDA = pc + 1;
			}
			else if (opcode == 0xA2)
			{
				// LDX
				addrLDX = pc + 1;
			}
			else if (opcode == 0xA0)
			{
				// LDY
				addrLDY = pc + 1;
			}
			else if (opcode == 0x8D
					 || opcode == 0x8E
					 || opcode == 0x8C)
			{
				u8 a1 = debugInterface->GetByteFromRamC64(pc+1);
				u8 a2 = debugInterface->GetByteFromRamC64(pc+2);
				int storeAddr = a1 | (a2 << 8);
				int loadAddr = -1;
				
				if (opcode == 0x8D)
				{
					// STA
					//LOGD("pc=%04x | STA=%04x, LDA =%04x", pc, storeAddr, addrLDA);
					
					loadAddr = addrLDA;
				}
				else if (opcode == 0x8E)
				{
					// STX
					//LOGD("pc=%04x | STX=%04x, LDX =%04x", pc, storeAddr, addrLDX);
					
					loadAddr = addrLDX;
				}
				else if (opcode == 0x8C)
				{
					// STY
					//LOGD("pc=%04x | STY=%04x, LDY =%04x", pc, storeAddr, addrLDY);
					
					loadAddr = addrLDY;
				}
				
				if (storeAddr == 0xD010)
				{
					*addrPosXHighBits = loadAddr;
				}
				else if (storeAddr == 0xD000)
				{
					addrPosX[0] = loadAddr;
				}
				else if (storeAddr == 0xD002)
				{
					addrPosX[1] = loadAddr;
				}
				else if (storeAddr == 0xD004)
				{
					addrPosX[2] = loadAddr;
				}
				else if (storeAddr == 0xD006)
				{
					addrPosX[3] = loadAddr;
				}
				else if (storeAddr == 0xD008)
				{
					addrPosX[4] = loadAddr;
				}
				else if (storeAddr == 0xD00A)
				{
					addrPosX[5] = loadAddr;
				}
				else if (storeAddr == 0xD00C)
				{
					addrPosX[6] = loadAddr;
				}
				else if (storeAddr == 0xD00E)
				{
					addrPosX[7] = loadAddr;
				}
				
				else if (storeAddr == 0xD001)
				{
					addrPosY[0] = loadAddr;
				}
				else if (storeAddr == 0xD003)
				{
					addrPosY[1] = loadAddr;
				}
				else if (storeAddr == 0xD005)
				{
					addrPosY[2] = loadAddr;
				}
				else if (storeAddr == 0xD007)
				{
					addrPosY[3] = loadAddr;
				}
				else if (storeAddr == 0xD009)
				{
					addrPosY[4] = loadAddr;
				}
				else if (storeAddr == 0xD00B)
				{
					addrPosY[5] = loadAddr;
				}
				else if (storeAddr == 0xD00D)
				{
					addrPosY[6] = loadAddr;
				}
				else if (storeAddr == 0xD00F)
				{
					addrPosY[7] = loadAddr;
				}
				
				
				else if (storeAddr == 0xD01C)
				{
					*addrSetMultiColor = loadAddr;
				}
				
				else if (storeAddr == 0xD027)
				{
					addrColorChange[0] = loadAddr;
				}
				else if (storeAddr == 0xD028)
				{
					addrColorChange[1] = loadAddr;
				}
				else if (storeAddr == 0xD029)
				{
					addrColorChange[2] = loadAddr;
				}
				else if (storeAddr == 0xD02A)
				{
					addrColorChange[3] = loadAddr;
				}
				else if (storeAddr == 0xD02B)
				{
					addrColorChange[4] = loadAddr;
				}
				else if (storeAddr == 0xD02C)
				{
					addrColorChange[5] = loadAddr;
				}
				else if (storeAddr == 0xD02D)
				{
					addrColorChange[6] = loadAddr;
				}
				else if (storeAddr == 0xD02E)
				{
					addrColorChange[7] = loadAddr;
				}
				
				else if (storeAddr == 0xD025)
				{
					*addrColorChangeCommon1 = loadAddr;
				}
				else if (storeAddr == 0xD026)
				{
					*addrColorChangeCommon2 = loadAddr;
				}
				
				else if (storeAddr == 0xD01D)
				{
					*addrSetStretchHorizontal = loadAddr;
				}
				
				else if (storeAddr == 0xD017)
				{
					*addrSetStretchVertical = loadAddr;
				}
				
			}
			
			if (y == searchRasterLine
				&& cycle == searchRasterCycle)
			{
				guiMain->UnlockMutex(); //"CVicEditorLayerVirtualSprites::ScanSpritesStoreAddressesOnly");
				return;
			}
		}
	}
	
	
	
	///
	
	//debugInterface->SetDebugMode(debugMode);
	
	guiMain->UnlockMutex(); //"CVicEditorLayerVirtualSprites::ScanSpritesStoreAddressesOnly");
	
	LOGError("ScanSpritesStoreAddressesOnly: rasterLine=%d rasterCycle=%d not reached", searchRasterLine, searchRasterCycle);
	
	LOGD("------------ FINISHED CVicEditorLayerVirtualSprites::ScanSpritesStoreAddressesOnly in %d ms", SYS_GetCurrentTimeInMillis() - t);
	
}

//1 = d025
//2 = d027+id
//3 = d026

u8 CVicEditorLayerVirtualSprites::ReplaceColor(int rx, int ry, int spriteId, int colorNum, u8 colorValue)
{
	LOGD("CVicEditorLayerVirtualSprites::ReplaceColor: rx=%d ry=%d id=%d colorNum=%d colorValue=%02x", rx, ry, spriteId, colorNum, colorValue);
	
	int addrPosXHighBits = -1;
	int addrPosX[8] = { -1 };
	int addrPosY[8] = { -1 };
	int addrSetStretchHorizontal = -1;
	int addrSetStretchVertical = -1;
	int addrSetMultiColor = -1;
	int addrColorChangeCommon1 = -1;
	int addrColorChangeCommon2 = -1;
	int addrColorChange[8] = { -1 };
	
	ScanSpritesStoreAddressesOnly(rx, ry,
								  &addrPosXHighBits,
								  addrPosX,
								  addrPosY,
								  &addrSetStretchHorizontal,
								  &addrSetStretchVertical,
								  &addrSetMultiColor,
								  &addrColorChangeCommon1,
								  &addrColorChangeCommon2,
								  addrColorChange);
	
	return ReplaceColor(rx, ry, spriteId, colorNum, colorValue, addrColorChange, addrColorChangeCommon1, addrColorChangeCommon2);
}

u8 CVicEditorLayerVirtualSprites::ReplaceColor(int rx, int ry, int spriteId, int colorNum, u8 colorValue,
											   int addrColorChange[8],
											   int addrColorChangeCommon1,
											   int addrColorChangeCommon2)
{
	LOGD("CVicEditorLayerVirtualSprites::ReplaceColor: rx=%d ry=%d id=%d colorNum=%d colorValue=%02x", rx, ry, spriteId, colorNum, colorValue);
	

	CDebugInterfaceC64 *debugInterface = vicEditor->viewVicDisplayMain->debugInterface;
	
	if (colorNum == 0)
	{
		// not supported
		LOGError("CVicEditorLayerVirtualSprites::ReplaceColor: colorNum=0 not supported");
		return PAINT_RESULT_ERROR;
	}
	else if (colorNum == 1)
	{
		LOGD("addrColorChangeCommon1=%04x", addrColorChangeCommon1);
		if (addrColorChangeCommon1 > 0)
		{
			debugInterface->SetByteToRamC64(addrColorChangeCommon1, colorValue);
		}
		else
		{
			//debugInterface->SetByteC64(0xD025, colorValue);
			debugInterface->SetVicRegister(0x25, colorValue);
		}
		return PAINT_RESULT_OK;
	}
	else if (colorNum == 2)
	{
		LOGD("addrColorChange[%d]=%04x", spriteId, addrColorChange[spriteId]);
		if (addrColorChange[spriteId] > 0)
		{
			debugInterface->SetByteToRamC64(addrColorChange[spriteId], colorValue);
		}
		else
		{
			//debugInterface->SetByteC64(0xD027+spriteId, colorValue);
			debugInterface->SetVicRegister(0x27+spriteId, colorValue);
		}
		return PAINT_RESULT_OK;
	}
	else if (colorNum == 3)
	{
		LOGD("addrColorChangeCommon2=%04x", addrColorChangeCommon2);
		if (addrColorChangeCommon2 > 0)
		{
			debugInterface->SetByteToRamC64(addrColorChangeCommon2, colorValue);
		}
		else
		{
			//debugInterface->SetByteC64(0xD026, colorValue);
			debugInterface->SetVicRegister(0x26, colorValue);
		}
		return PAINT_RESULT_OK;
	}
	
	return PAINT_RESULT_ERROR;
	
}

///
void CVicEditorLayerVirtualSprites::ReplacePosX(int rx, int ry, int spriteId, int posX)
{
	LOGD("CVicEditorLayerVirtualSprites::ReplacePosX: rx=%d ry=%d id=%d posX=%d", rx, ry, spriteId, posX);
	CDebugInterfaceC64 *debugInterface = vicEditor->viewVicDisplayMain->debugInterface;

	int addrPosXHighBits = -1;
	int addrPosX[8] = { -1 };
	int addrPosY[8] = { -1 };
	int addrSetStretchHorizontal = -1;
	int addrSetStretchVertical = -1;
	int addrSetMultiColor = -1;
	int addrColorChangeCommon1 = -1;
	int addrColorChangeCommon2 = -1;
	int addrColorChange[8] = { -1 };
	
	ScanSpritesStoreAddressesOnly(rx, ry,
								  &addrPosXHighBits,
								  addrPosX,
								  addrPosY,
								  &addrSetStretchHorizontal,
								  &addrSetStretchVertical,
								  &addrSetMultiColor,
								  &addrColorChangeCommon1,
								  &addrColorChangeCommon2,
								  addrColorChange);

	if (addrPosX[spriteId] > 0)
	{
		debugInterface->SetByteToRamC64(addrPosX[spriteId], posX);
	}
	else
	{
		//debugInterface->SetByteC64(0xD000 + (spriteId*2), posX);
		debugInterface->SetVicRegister(0x00 + (spriteId*2), posX);
	}

	u8 o = 0x00;
	if (posX > 255)
	{
		o = 0x01;
	}
	
	LOGD("    o=%02x", o);
	if (addrPosXHighBits > 0)
	{
		u8 v = debugInterface->GetByteFromRamC64(addrPosXHighBits);
		u8 a = ~(1 << spriteId);
		u8 o2 = (o << spriteId);
		
		u8 v2 = (v & a) | o2;
		
		LOGD("  v=%02x a=%02x o=%02x o2=%02x | v2=%02x", v, a, o, o2, v2);
		
		debugInterface->SetByteToRamC64(addrPosXHighBits, v2);
	}
	else
	{
		u8 v = debugInterface->GetVicRegister(0x10);
		u8 a = ~(1 << spriteId);
		u8 o2 = (o << spriteId);
		
		u8 v2 = (v & a) | o2;

		LOGD("  v=%02x a=%02x o=%02x o2=%02x | v2=%02x", v, a, o, o2, v2);

		debugInterface->SetVicRegister(0x10, v2);
	}
}

void CVicEditorLayerVirtualSprites::ReplacePosY(int rx, int ry, int spriteId, int posY)
{
	LOGD("CVicEditorLayerVirtualSprites::ReplacePosY: rx=%d ry=%d id=%d posY=%d", rx, ry, spriteId, posY);
	CDebugInterfaceC64 *debugInterface = vicEditor->viewVicDisplayMain->debugInterface;
	
	int addrPosXHighBits = -1;
	int addrPosX[8] = { -1 };
	int addrPosY[8] = { -1 };
	int addrSetStretchHorizontal = -1;
	int addrSetStretchVertical = -1;
	int addrSetMultiColor = -1;
	int addrColorChangeCommon1 = -1;
	int addrColorChangeCommon2 = -1;
	int addrColorChange[8] = { -1 };
	
	ScanSpritesStoreAddressesOnly(rx, ry,
								  &addrPosXHighBits,
								  addrPosX,
								  addrPosY,
								  &addrSetStretchHorizontal,
								  &addrSetStretchVertical,
								  &addrSetMultiColor,
								  &addrColorChangeCommon1,
								  &addrColorChangeCommon2,
								  addrColorChange);
	
	if (addrPosY[spriteId] > 0)
	{
		debugInterface->SetByteToRamC64(addrPosY[spriteId], posY);
	}
	else
	{
		//debugInterface->SetByteC64(0xD000 + (spriteId*2), posX);
		debugInterface->SetVicRegister(0x01 + (spriteId*2), posY);
	}
}

///
void CVicEditorLayerVirtualSprites::ReplaceMultiColor(int rx, int ry, int spriteId, bool isMultiColor)
{
	LOGD("CVicEditorLayerVirtualSprites::ReplaceMultiColor: rx=%d ry=%d id=%d isMultiColor=%d", rx, ry, spriteId, isMultiColor);
	CDebugInterfaceC64 *debugInterface = vicEditor->viewVicDisplayMain->debugInterface;
	
	int addrPosXHighBits = -1;
	int addrPosX[8] = { -1 };
	int addrPosY[8] = { -1 };
	int addrSetStretchHorizontal = -1;
	int addrSetStretchVertical = -1;
	int addrSetMultiColor = -1;
	int addrColorChangeCommon1 = -1;
	int addrColorChangeCommon2 = -1;
	int addrColorChange[8] = { -1 };
	
	ScanSpritesStoreAddressesOnly(rx, ry,
								  &addrPosXHighBits,
								  addrPosX,
								  addrPosY,
								  &addrSetStretchHorizontal,
								  &addrSetStretchVertical,
								  &addrSetMultiColor,
								  &addrColorChangeCommon1,
								  &addrColorChangeCommon2,
								  addrColorChange);
	
	u8 o = isMultiColor ? 0x01 : 0x00;
	
	LOGD("    o=%02x", o);
	if (addrSetMultiColor > 0)
	{
		u8 v = debugInterface->GetByteFromRamC64(addrSetMultiColor);
		u8 a = ~(1 << spriteId);
		u8 o2 = (o << spriteId);
		
		u8 v2 = (v & a) | o2;
		
		LOGD("  v=%02x a=%02x o=%02x o2=%02x | v2=%02x", v, a, o, o2, v2);
		
		debugInterface->SetByteToRamC64(addrSetMultiColor, v2);
	}
	else
	{
		u8 v = debugInterface->GetVicRegister(0x1C);
		u8 a = ~(1 << spriteId);
		u8 o2 = (o << spriteId);
		
		u8 v2 = (v & a) | o2;
		
		LOGD("  v=%02x a=%02x o=%02x o2=%02x | v2=%02x", v, a, o, o2, v2);
		
		debugInterface->SetVicRegister(0x1C, v2);
	}
}
///
void CVicEditorLayerVirtualSprites::ReplaceStretchX(int rx, int ry, int spriteId, bool isStretchX)
{
	LOGD("CVicEditorLayerVirtualSprites::ReplaceStretchX: rx=%d ry=%d id=%d isStretchX=%d", rx, ry, spriteId, isStretchX);
	CDebugInterfaceC64 *debugInterface = vicEditor->viewVicDisplayMain->debugInterface;
	
	int addrPosXHighBits = -1;
	int addrPosX[8] = { -1 };
	int addrPosY[8] = { -1 };
	int addrSetStretchHorizontal = -1;
	int addrSetStretchVertical = -1;
	int addrSetMultiColor = -1;
	int addrColorChangeCommon1 = -1;
	int addrColorChangeCommon2 = -1;
	int addrColorChange[8] = { -1 };
	
	ScanSpritesStoreAddressesOnly(rx, ry,
								  &addrPosXHighBits,
								  addrPosX,
								  addrPosY,
								  &addrSetStretchHorizontal,
								  &addrSetStretchVertical,
								  &addrSetMultiColor,
								  &addrColorChangeCommon1,
								  &addrColorChangeCommon2,
								  addrColorChange);
	
	u8 o = isStretchX ? 0x01 : 0x00;
	
	LOGD("    o=%02x", o);
	if (addrSetStretchHorizontal > 0)
	{
		u8 v = debugInterface->GetByteFromRamC64(addrSetStretchHorizontal);
		u8 a = ~(1 << spriteId);
		u8 o2 = (o << spriteId);
		
		u8 v2 = (v & a) | o2;
		
		LOGD("  v=%02x a=%02x o=%02x o2=%02x | v2=%02x", v, a, o, o2, v2);
		
		debugInterface->SetByteToRamC64(addrSetStretchHorizontal, v2);
	}
	else
	{
		u8 v = debugInterface->GetVicRegister(0x1D);
		u8 a = ~(1 << spriteId);
		u8 o2 = (o << spriteId);
		
		u8 v2 = (v & a) | o2;
		
		LOGD("  v=%02x a=%02x o=%02x o2=%02x | v2=%02x", v, a, o, o2, v2);
		
		debugInterface->SetVicRegister(0x1D, v2);
	}

}

void CVicEditorLayerVirtualSprites::ReplaceStretchY(int rx, int ry, int spriteId, bool isStretchY)
{
	LOGD("CVicEditorLayerVirtualSprites::ReplaceStretchY: rx=%d ry=%d id=%d isStretchX=%d", rx, ry, spriteId, isStretchY);
	CDebugInterfaceC64 *debugInterface = vicEditor->viewVicDisplayMain->debugInterface;
	
	int addrPosXHighBits = -1;
	int addrPosX[8] = { -1 };
	int addrPosY[8] = { -1 };
	int addrSetStretchHorizontal = -1;
	int addrSetStretchVertical = -1;
	int addrSetMultiColor = -1;
	int addrColorChangeCommon1 = -1;
	int addrColorChangeCommon2 = -1;
	int addrColorChange[8] = { -1 };
	
	ScanSpritesStoreAddressesOnly(rx, ry,
								  &addrPosXHighBits,
								  addrPosX,
								  addrPosY,
								  &addrSetStretchHorizontal,
								  &addrSetStretchVertical,
								  &addrSetMultiColor,
								  &addrColorChangeCommon1,
								  &addrColorChangeCommon2,
								  addrColorChange);
	
	u8 o = isStretchY ? 0x01 : 0x00;
	
	LOGD("    o=%02x", o);
	if (addrSetStretchVertical > 0)
	{
		u8 v = debugInterface->GetByteFromRamC64(addrSetStretchVertical);
		u8 a = ~(1 << spriteId);
		u8 o2 = (o << spriteId);
		
		u8 v2 = (v & a) | o2;
		
		LOGD("  v=%02x a=%02x o=%02x o2=%02x | v2=%02x", v, a, o, o2, v2);
		
		debugInterface->SetByteToRamC64(addrSetStretchVertical, v2);
	}
	else
	{
		u8 v = debugInterface->GetVicRegister(0x17);
		u8 a = ~(1 << spriteId);
		u8 o2 = (o << spriteId);
		
		u8 v2 = (v & a) | o2;
		
		LOGD("  v=%02x a=%02x o=%02x o2=%02x | v2=%02x", v, a, o, o2, v2);
		
		debugInterface->SetVicRegister(0x17, v2);
	}

}

///
C64Sprite *CVicEditorLayerVirtualSprites::FindSpriteByRasterPos(int rx, int ry)
{
	for (std::list<C64Sprite *>::iterator it = this->sprites.begin();
		 it != this->sprites.end(); it++)
	{
		C64Sprite *sprite = *it;
		
		float x = sprite->posX;
		float y = sprite->posY;
		
		int spriteSizeX = sprite->isStretchedHorizontally ? 48 : 24;
		int spriteSizeY = sprite->isStretchedVertically ? 42 : 21;
		
		if (rx >= x && rx < (x + spriteSizeX)
			&& ry >= y && ry < (y + spriteSizeY))
		{
			return sprite;
		}
	}
	
	return NULL;
}

u8 CVicEditorLayerVirtualSprites::Paint(bool forceColorReplace, bool isDither, int rlx, int rly, u8 colorLMB, u8 colorRMB, u8 colorSource, int charValue)
{
	// char value is not used in sprites
	return Paint(forceColorReplace, isDither, rlx, rly, colorLMB, colorRMB, colorSource);
}

u8 CVicEditorLayerVirtualSprites::Paint(bool forceColorReplace, bool isDither, int rlx, int rly, u8 colorLMB, u8 colorRMB, u8 colorSource)
{
//	LOGD("CVicEditorLayerVirtualSprites::Paint: rlx=%d rly=%d", rlx, rly);
	
	CViewC64VicDisplay *vicDisplay = vicEditor->viewVicDisplayMain;
	
	// update raster pos back based on scroll register
	int rx = rlx + vicDisplay->scrollInRasterPixelsX;
	int ry = rly + vicDisplay->scrollInRasterPixelsY;
	
//	LOGD("                                      rx=%d ry=%d  (%d %d)", rx, ry, vicDisplay->scrollInRasterPixelsX, vicDisplay->scrollInRasterPixelsY);
	
	///

	// requested by Isildur to always replace RMB with $d021
	if (colorSource == VICEDITOR_COLOR_SOURCE_RMB)
	{
		colorRMB = vicEditor->viewPalette->colorD021;
	}
	
	///
	
	for (std::list<C64Sprite *>::iterator it = this->sprites.begin();
		 it != this->sprites.end(); it++)
	{
		C64Sprite *sprite = *it;
		
		float x = sprite->posX;
		float y = sprite->posY;
		
		int spriteSizeX = sprite->isStretchedHorizontally ? 48 : 24;
		int spriteSizeY = sprite->isStretchedVertically ? 42 : 21;
		
		if (rx >= x && rx < (x + spriteSizeX)
			&& ry >= y && ry < (y + spriteSizeY))
		{
			LOGD("  Paint sprite id=%d", sprite->spriteId);

			if (isDither)
			{
				return sprite->PaintDither(forceColorReplace, rx, ry, colorLMB, colorRMB, colorSource);
			}
			else
			{
				this->ClearDitherMask();
				
				return sprite->PutColorAtPixel(forceColorReplace, rx, ry, colorLMB, colorRMB, colorSource);
			}

			
			return PAINT_RESULT_OK;
		}
	}
	
	return PAINT_RESULT_OUTSIDE;
}

bool CVicEditorLayerVirtualSprites::GetColorAtPixel(int rx, int ry, u8 *color)
{
	for (std::list<C64Sprite *>::iterator it = this->sprites.begin();
		 it != this->sprites.end(); it++)
	{
		C64Sprite *sprite = *it;
		
		float x = sprite->posX;
		float y = sprite->posY;
		
		int spriteSizeX = sprite->isStretchedHorizontally ? 48 : 24;
		int spriteSizeY = sprite->isStretchedVertically ? 42 : 21;
		
		if (rx >= x && rx < (x + spriteSizeX)
			&& ry >= y && ry < (y + spriteSizeY))
		{
			LOGD("  Get color sprite id=%d", sprite->spriteId);
			
			*color = sprite->GetColorAtPixel(rx, ry);
			return true;
		}
	}
	return false;
}

void CVicEditorLayerVirtualSprites::ClearDitherMask()
{
	multiDitherMaskPosX = -1;
	multiDitherMaskPosY = -1;
	hiresDitherMaskPosX = -1;
	hiresDitherMaskPosY = -1;
}

void CVicEditorLayerVirtualSprites::UpdateSpriteView(int rx, int ry)
{
	vicEditor->viewSprite->spriteRasterX = rx;
	vicEditor->viewSprite->spriteRasterY = ry;
}


void CVicEditorLayerVirtualSprites::ClearScreen()
{
	ClearScreen(0x00, 0x00);
}

void CVicEditorLayerVirtualSprites::ClearScreen(u8 charValue, u8 colorValue)
{
	for (std::list<C64Sprite *>::iterator it = this->sprites.begin();
		 it != this->sprites.end(); it++)
	{
		C64Sprite *sprite = *it;
		sprite->Clear();
	}
	
	ClearSprites();
}

void CVicEditorLayerVirtualSprites::Serialise(CByteBuffer *byteBuffer)
{
	guiMain->LockMutex(); //"CVicEditorLayerVirtualSprites::Serialise");

	CVicEditorLayer::Serialise(byteBuffer);

	SimpleScanSpritesInThisFrame();
	
	int numSprites = this->sprites.size();
	
	byteBuffer->PutI32(numSprites);
	
	for (std::list<C64Sprite *>::iterator it = this->sprites.begin();
		 it != this->sprites.end(); it++)
	{
		C64Sprite *sprite = *it;
		if (sprite->isMulti)
		{
			byteBuffer->PutBool(true);
		}
		else
		{
			byteBuffer->PutBool(false);
		}
		
		sprite->Serialise(byteBuffer);
	}
	
	guiMain->UnlockMutex(); //"CVicEditorLayerVirtualSprites::Serialise");
}

void CVicEditorLayerVirtualSprites::Deserialise(CByteBuffer *byteBuffer, int version)
{
	CVicEditorLayer::Deserialise(byteBuffer, version);

	ClearSprites();

	int numSprites = byteBuffer->GetI32();
	
	for (int i = 0; i < numSprites; i++)
	{
		C64Sprite *sprite = NULL;
		
		bool isMulti = byteBuffer->GetBool();
		if (isMulti)
		{
			sprite = new C64SpriteMulti(this->vicEditor, byteBuffer);
		}
		else
		{
			sprite = new C64SpriteHires(this->vicEditor, byteBuffer);
		}
		
		sprites.push_back(sprite);
	}
}


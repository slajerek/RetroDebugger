#include "CViewBaseStateCPU.h"
#include "SYS_Main.h"
#include "CGuiMain.h"
#include "CSlrString.h"
#include "CViewC64.h"
#include "CDebugInterface.h"
#include "CGuiEditHex.h"
#include "CLayoutParameter.h"

CViewBaseStateCPU::CViewBaseStateCPU(const char *name, float posX, float posY, float posZ, float sizeX, float sizeY, CDebugInterface *debugInterface)
: CGuiView(name, posX, posY, posZ, sizeX, sizeY)
{
	this->debugInterface = debugInterface;

	this->font = viewC64->fontDisassembly;
	fontSize = 7.0f;
	AddLayoutParameter(new CLayoutParameterFloat("Font Size", &fontSize));

	this->editingCpuRegisterIndex = -1;
	
	editBoxHex = new CGuiEditHex(this);
	editBoxHex->isCapitalLetters = false;

	this->regs = NULL;
	this->numRegisters = 0;
	
	this->SetPosition(posX, posY, posZ, sizeX, sizeY);
}

void CViewBaseStateCPU::SetFont(CSlrFont *font, float fontSize)
{
	this->font = font;
	this->fontSize = fontSize;
	
	CGuiView::SetPosition(posX, posY, posZ, fontSize*51, fontSize*2);
}

void CViewBaseStateCPU::SetPosition(float posX, float posY)
{
	CGuiView::SetPosition(posX, posY, posZ, fontSize*51, fontSize*2);
}

void CViewBaseStateCPU::SetPosition(float posX, float posY, float posZ, float sizeX, float sizeY)
{
	CGuiView::SetPosition(posX, posY, posZ, sizeX, sizeY);
}

void CViewBaseStateCPU::LayoutParameterChanged(CLayoutParameter *layoutParameter)
{
	CGuiView::LayoutParameterChanged(layoutParameter);
}

void CViewBaseStateCPU::DoLogic()
{
}

void CViewBaseStateCPU::RenderImGui()
{
	PreRenderImGui();
	Render();
	PostRenderImGui();
}


void CViewBaseStateCPU::Render()
{
//	LOGD("CViewBaseStateCPU::Render: %s %f %f", this->name, this->posX, this->posY);
	float px = this->posX;
	float py = this->posY;
	
	float br = 0.0f;
	float bg = 0.0f;
	float bb = 0.0f;
	
	
	if (debugInterface->GetDebugMode() != DEBUGGER_MODE_RUNNING)
	{
		br = 0.5; bg = 0.0f; bb = 0.0f;
		BlitFilledRectangle(px-fontSize*0.3f, py-fontSize*0.3f, -1, fontSize*49.6f, fontSize*2.3f, br, bg, bb, 1.00f);
	}

	this->RenderRegisters();
	
	py += fontSize;
	
	if (editingCpuRegisterIndex != -1)
	{
		float gap = 0.5f;
		
		float x = posX + (regs[editingCpuRegisterIndex].characterPos) * fontSize;
		float sx = (regs[editingCpuRegisterIndex].numCharacters) * fontSize;	// + gap

		// hide already displayed value
		BlitFilledRectangle(x, py, posZ, sx, fontSize, br, bg, bb, 1.0f);	//- gap*fontSize
		
		font->BlitTextColor(editBoxHex->textWithCursor, x, py, -1, fontSize, 1, 1, 1, 1);
	}
}

void CViewBaseStateCPU::GuiEditHexEnteredValue(CGuiEditHex *editHex, u32 lastKeyCode, bool isCancelled)
{
	LOGD("CViewBaseStateCPU::GuiEditHexEnteredValue");
	
	guiMain->LockMutex();

	if (isCancelled)
	{
		editingCpuRegisterIndex = -1;
		guiMain->UnlockMutex();
		return;
	}

	FinishEditingRegister();
	
	editingCpuRegisterIndex = -1;
	
	guiMain->UnlockMutex();
}


int CViewBaseStateCPU::GetCpuRegisterIndexFromCharacterPos(float x)
{
	float gap = 0.5f;
	
	for (int i = 0; i < numRegisters; i++)
	{
		if (x >= (regs[i].characterPos - gap) && x <= (regs[i].characterPos + regs[i].numCharacters + gap))
		{
			return i;
		}
	}
	
	return -1;
}

StateCPURegister CViewBaseStateCPU::GetCpuRegisterFromCharacterPos(float x)
{
	float gap = 0.5f;
	
	for (int i = 0; i < numRegisters; i++)
	{
		if (x >= (regs[i].characterPos - gap) && x <= (regs[i].characterPos + regs[i].numCharacters + gap))
		{
			return regs[i].reg;
		}
	}

	return STATE_CPU_REGISTER_NONE;
}

float CViewBaseStateCPU::GetCharacterPosByCpuRegister(StateCPURegister cpuRegister)
{
	for (int i = 0; i < numRegisters; i++)
	{
		if (regs[i].reg == cpuRegister)
		{
			return regs[i].characterPos;
		}
	}
	
	return STATE_CPU_REGISTER_NONE;
}


bool CViewBaseStateCPU::DoTap(float x, float y)
{
	LOGTODO("BUG IS HERE: CViewBaseStateCPU::DoTap IsInside");
	LOGD("CViewBaseStateCPU::DoTap: %f %f", x, y);
	
	float px = x - this->posX;
	float vx = px / fontSize;
	
//	LOGD("px=%f vx=%f", px, vx);
	
	int index = GetCpuRegisterIndexFromCharacterPos(vx);
	
//	LOGD("  ..reg=%d", index);
	
	guiMain->LockMutex();

	if (editingCpuRegisterIndex != -1)
	{
		FinishEditingRegister();
		editingCpuRegisterIndex = -1;
	}
	
	if (index != -1)
	{
		editingCpuRegisterIndex = index;
		StartEditingRegister();
		viewC64->SetFocus(this);
	}
	
	guiMain->UnlockMutex();

	return false;
}

void CViewBaseStateCPU::FinishEditingRegister()
{
	// finalize editing, store value
	StateCPURegister reg = regs[editingCpuRegisterIndex].reg;
	
	int v;
	
	LOGD("editingCpuRegisterIndex=%d reg=%d STATE_CPU_REGISTER_FLAGS=%d", editingCpuRegisterIndex, reg, STATE_CPU_REGISTER_FLAGS);
	
	if (reg == STATE_CPU_REGISTER_FLAGS)
	{
		// special case - set bits
		CSlrString *str = editBoxHex->text;
		char *buf = str->GetStdASCII();
		v = Bits2Byte(buf);
		delete [] buf;
	}
	else
	{
		v = editBoxHex->value;
	}
	
	SetRegisterValue(reg, v);
}

void CViewBaseStateCPU::StartEditingRegister()
{
	// start editing, get value
	StateCPURegister reg = regs[editingCpuRegisterIndex].reg;
	
	int v = GetRegisterValue(reg);
	if (reg == STATE_CPU_REGISTER_FLAGS)
	{
		// special case - get bits
		char buf[9];
		Byte2Bits(v, buf);
		CSlrString *str = new CSlrString(buf);
		editBoxHex->SetText(str);
	}
	else
	{
		editBoxHex->SetValue(v, regs[editingCpuRegisterIndex].numCharacters);
	}
}

void CViewBaseStateCPU::RenderRegisters()
{
	LOGError("CViewBaseStateCPU::RenderRegisters");
}

void CViewBaseStateCPU::SetRegisterValue(StateCPURegister reg, int value)
{
	LOGError("CViewBaseStateCPU::SetRegisterValue: reg=%d value=%d", reg, value);
}

int CViewBaseStateCPU::GetRegisterValue(StateCPURegister reg)
{
	LOGError("CViewBaseStateCPU::GetRegisterValue: reg=%d", reg);
	return 0xFAFA;
}

bool CViewBaseStateCPU::KeyDown(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	LOGD("CViewBaseStateCPU::KeyDown: %d", keyCode);
	
	if (editingCpuRegisterIndex != -1)
	{
		keyCode = SYS_GetBareKey(keyCode, isShift, isAlt, isControl, isSuper);
		
		if (regs[editingCpuRegisterIndex].reg == STATE_CPU_REGISTER_FLAGS)
		{
			// skip other characters than 0 or 1 when in FLAGS mode
			if (keyCode != '0' && keyCode != '1')
			{
				return true;
			}
		}
		
		editBoxHex->KeyDown(keyCode);
		return true;
	}

	return false;
}

bool CViewBaseStateCPU::KeyUp(u32 keyCode, bool isShift, bool isAlt, bool isControl, bool isSuper)
{
	return false;
}

bool CViewBaseStateCPU::SetFocus()
{
	if (editingCpuRegisterIndex != -1)
		return true;
	
	return false;
}

// Layout
void CViewBaseStateCPU::Serialize(CByteBuffer *byteBuffer)
{
}

void CViewBaseStateCPU::Deserialize(CByteBuffer *byteBuffer)
{
}



#include "CTestVicEditorZoom.h"
#include "EmulatorsConfig.h"
#include "CViewC64.h"
#include "CViewC64VicEditor.h"
#include "CViewC64VicDisplay.h"
#include "SYS_Main.h"
#include "CGuiMain.h"
#include <cmath>

void CTestVicEditorZoom::Run(ITestCallback *cb)
{
	this->callback = cb;
	this->isRunning = true;
	this->currentStep = 0;

#ifdef RUN_COMMODORE64
	CViewC64VicEditor *vicEditor = viewC64->viewVicEditor;
	if (vicEditor == NULL)
	{
		TestCompleted(false, "VIC Editor is NULL");
		return;
	}

	CViewC64VicDisplay *vicDisplay = vicEditor->viewVicDisplay;
	if (vicDisplay == NULL)
	{
		TestCompleted(false, "VIC Display is NULL");
		return;
	}

	StepCompleted(1, true, "VIC Editor and Display found");

	// Reset display to known state
	vicEditor->ResetPosition();

	float initialPosX = vicDisplay->posX;
	float initialPosY = vicDisplay->posY;
	float initialScale = vicDisplay->scale;

	LOGM("CTestVicEditorZoom: Initial state: posX=%.2f posY=%.2f scale=%.2f displayPosX=%.2f displayPosY=%.2f",
		 initialPosX, initialPosY, initialScale,
		 vicDisplay->displayPosX, vicDisplay->displayPosY);
	LOGM("  displaySizeX=%.2f displaySizeY=%.2f posOffsetX=%.2f posOffsetY=%.2f borderType=%d",
		 vicDisplay->displaySizeX, vicDisplay->displaySizeY,
		 vicDisplay->posOffsetX, vicDisplay->posOffsetY,
		 vicDisplay->showDisplayBorderType);

	StepCompleted(2, true, "Initial state captured");

	// ====================================================================
	// Test 3: Fixed anchor far from display origin - simulates a user with
	// a wide display (e.g., 1440p) zooming into the bottom-right area.
	// This should trigger the ±20000 boundary clamp in CheckDisplayBoundaries.
	//
	// Math: zoom-to-point formula is:
	//   new_posX = anchor + (posX - anchor) * newScale/oldScale
	// With anchor=1200, posX=500, scale 48/1.4:
	//   posX ≈ 1200 + (500-1200)*34.3 = 1200 - 24010 = -22810
	// This exceeds -20000, triggering the clamp to (0,0) = JUMP!
	// ====================================================================
	LOGM("CTestVicEditorZoom: === Test 3: Distant fixed anchor (reproduce boundary clamp) ===");
	vicEditor->ResetPosition();

	// Use a fixed anchor point far to the right/bottom, simulating mouse
	// at bottom-right of a 1440px wide display
	float distantAnchorX = 1200.0f;
	float distantAnchorY = 800.0f;
	float scale = vicDisplay->scale;
	float prevPosX = vicDisplay->posX;
	float prevPosY = vicDisplay->posY;
	bool jumpDetected3 = false;
	int jumpStep3 = -1;
	float jumpFromScale3 = 0, jumpToScale3 = 0;

	LOGM("CTestVicEditorZoom T3: anchor=(%.1f,%.1f) initial posX=%.1f posY=%.1f scale=%.2f",
		 distantAnchorX, distantAnchorY, prevPosX, prevPosY, scale);

	int totalSteps = 300;
	for (int i = 0; i < totalSteps; i++)
	{
		float deltaY = 0.25f;  // moderate zoom speed
		scale = vicDisplay->scale + deltaY;

		vicEditor->ZoomDisplay(scale, distantAnchorX, distantAnchorY);

		float newPosX = vicDisplay->posX;
		float newPosY = vicDisplay->posY;
		float diffX = fabsf(newPosX - prevPosX);
		float diffY = fabsf(newPosY - prevPosY);

		if (i % 20 == 0 || diffX > 500 || diffY > 500)
		{
			LOGM("CTestVicEditorZoom T3: Step %d: scale=%.2f posX=%.1f posY=%.1f diffX=%.1f diffY=%.1f",
				 i, vicDisplay->scale, newPosX, newPosY, diffX, diffY);
		}

		if ((diffX > 500 || diffY > 500) && !jumpDetected3)
		{
			jumpDetected3 = true;
			jumpStep3 = i;
			jumpFromScale3 = vicDisplay->scale - deltaY;
			jumpToScale3 = vicDisplay->scale;

			LOGM("CTestVicEditorZoom T3: *** JUMP DETECTED at step %d! ***", i);
			LOGM("  From: posX=%.1f posY=%.1f scale=%.2f", prevPosX, prevPosY, jumpFromScale3);
			LOGM("  To:   posX=%.1f posY=%.1f scale=%.2f", newPosX, newPosY, jumpToScale3);
			LOGM("  Diff: X=%.1f Y=%.1f", diffX, diffY);
			LOGM("  NOTE: Clamp at +-20000 snaps to (0,0) causing jump to top-left!");
		}

		prevPosX = newPosX;
		prevPosY = newPosY;

		if (vicDisplay->scale > 80.0f)
			break;
	}

	// ====================================================================
	// Test 2: Fixed anchor at center area (baseline, shouldn't jump)
	// ====================================================================
	LOGM("CTestVicEditorZoom: === Test 2: Close fixed anchor (baseline) ===");
	vicEditor->ResetPosition();

	float closeAnchorX = 400.0f;
	float closeAnchorY = 300.0f;
	scale = vicDisplay->scale;
	prevPosX = vicDisplay->posX;
	prevPosY = vicDisplay->posY;
	bool jumpDetected2 = false;

	for (int i = 0; i < totalSteps; i++)
	{
		float deltaY = 0.25f;
		scale = vicDisplay->scale + deltaY;

		vicEditor->ZoomDisplay(scale, closeAnchorX, closeAnchorY);

		float newPosX = vicDisplay->posX;
		float newPosY = vicDisplay->posY;
		float diffX = fabsf(newPosX - prevPosX);
		float diffY = fabsf(newPosY - prevPosY);

		if (i % 50 == 0)
		{
			LOGM("CTestVicEditorZoom T2: Step %d: scale=%.2f posX=%.1f posY=%.1f",
				 i, vicDisplay->scale, newPosX, newPosY);
		}

		if ((diffX > 500 || diffY > 500) && !jumpDetected2)
		{
			jumpDetected2 = true;
			LOGM("CTestVicEditorZoom T2: *** JUMP DETECTED at step %d! ***", i);
			LOGM("  From: posX=%.1f posY=%.1f", prevPosX, prevPosY);
			LOGM("  To:   posX=%.1f posY=%.1f", newPosX, newPosY);
		}

		prevPosX = newPosX;
		prevPosY = newPosY;

		if (vicDisplay->scale > 80.0f)
			break;
	}

	char summary[512];
	if (jumpDetected3)
	{
		snprintf(summary, sizeof(summary),
				 "BUG REPRODUCED! Boundary clamp causes jump at step %d, scale %.1f->%.1f. "
				 "CheckDisplayBoundaries +-20000 clamps to (0,0). Fix: remove or increase limit.",
				 jumpStep3, jumpFromScale3, jumpToScale3);
		LOGM("CTestVicEditorZoom: RESULT: %s", summary);
		TestCompleted(true, summary);
	}
	else if (jumpDetected2)
	{
		snprintf(summary, sizeof(summary), "Unexpected jump in Test2 (close anchor)");
		LOGM("CTestVicEditorZoom: RESULT: %s", summary);
		TestCompleted(true, summary);
	}
	else
	{
		snprintf(summary, sizeof(summary),
				 "No jump detected (max scale: %.1f). Boundary not triggered in this config.",
				 vicDisplay->scale);
		LOGM("CTestVicEditorZoom: RESULT: %s", summary);
		TestCompleted(true, summary);
	}
#else
	TestCompleted(false, "C64 emulator not enabled");
#endif
}

void CTestVicEditorZoom::Cancel()
{
	isRunning = false;
}

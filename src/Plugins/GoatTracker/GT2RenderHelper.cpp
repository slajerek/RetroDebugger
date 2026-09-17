#include "GT2RenderHelper.h"
#include "CSlrImage.h"
#include "goattrk2.h"
#include "MT_UiScale.h"

// Default 100%. Clamped/snapped only via GT2_SetRenoiseUIScale.
float gt2RenoiseUIScale = 1.0f;

float GT2EffectiveUIScale()
{
	float userZoom = (keypreset == KEY_RENOISE) ? gt2RenoiseUIScale : 1.0f;
	return userZoom * MT_GetUiScale();
}

void DrawCharGT2(ImDrawList *dl, CGT2FontAtlas *font,
                 float px, float py, u8 charCode, ImU32 fgColor, ImU32 bgColor)
{
	if (!font || !font->image || !font->image->TexturePtr())
		return;

	float cw = GT2CellW();
	float ch = GT2CellH();

	// Background fill
	dl->AddRectFilled(
		ImVec2(px, py),
		ImVec2(px + cw, py + ch),
		bgColor);

	// Foreground: draw the white-on-transparent glyph tinted with fgColor
	dl->AddImage(
		font->image->TexturePtr(),
		ImVec2(px, py),
		ImVec2(px + cw, py + ch),
		ImVec2(font->uvX1[charCode], font->uvY1[charCode]),
		ImVec2(font->uvX2[charCode], font->uvY2[charCode]),
		fgColor);
}

void DrawTextGT2(ImDrawList *dl, CGT2FontAtlas *font,
                 float px, float py, u8 colorIndex, const char *text)
{
	// GT2 color encoding: low nibble = fg index, high nibble = bg index.
	// bg index 0 maps to black (palette[0]).
	u8 fg = colorIndex & 0x0F;
	u8 bg = (colorIndex >> 4) & 0x0F;
	ImU32 fgColor = font->palette[fg];
	ImU32 bgColor = font->palette[bg];

	float cw = GT2CellW();
	for (int i = 0; text[i]; i++)
	{
		DrawCharGT2(dl, font, px + i * cw, py, (u8)text[i], fgColor, bgColor);
	}
}

void DrawBgGT2(ImDrawList *dl, CGT2FontAtlas *font,
               float px, float py, u8 colorIndex, int lengthChars)
{
	// Fills the background color over a range of character cells.
	// Matches GT2's printbg which sets bg without touching fg or char.
	ImU32 color = font->palette[colorIndex & 0x0F];
	dl->AddRectFilled(
		ImVec2(px, py),
		ImVec2(px + lengthChars * GT2CellW(), py + GT2CellH()),
		color);
}

void DrawBlankGT2(ImDrawList *dl, CGT2FontAtlas *font,
                  float px, float py, u8 colorIndex, int lengthChars)
{
	// Fills the area with blank (space) cells. A space shows only the
	// background, so the high (background) nibble of the packed GT2 color is
	// used, matching GT2's printblankc and DrawTextGT2's own per-cell fill.
	ImU32 color = font->palette[(colorIndex >> 4) & 0x0F];
	dl->AddRectFilled(
		ImVec2(px, py),
		ImVec2(px + lengthChars * GT2CellW(), py + GT2CellH()),
		color);
}

void DrawBoxGT2(ImDrawList *dl, CGT2FontAtlas *font,
                float px, float py, u8 colorIndex, int widthChars, int heightChars)
{
	if (widthChars <= 0 || heightChars <= 0)
		return;

	ImU32 fgColor = font->palette[colorIndex & 0x0F];
	ImU32 bgColor = font->palette[0]; // black background inside box border
	float cw = GT2CellW();
	float ch = GT2CellH();

	// Corners
	DrawCharGT2(dl, font, px, py, '+', fgColor, bgColor);
	DrawCharGT2(dl, font, px + (widthChars - 1) * cw, py, '+', fgColor, bgColor);
	DrawCharGT2(dl, font, px, py + (heightChars - 1) * ch, '+', fgColor, bgColor);
	DrawCharGT2(dl, font, px + (widthChars - 1) * cw, py + (heightChars - 1) * ch, '+', fgColor, bgColor);

	// Top and bottom horizontal borders
	for (int col = 1; col < widthChars - 1; col++)
	{
		DrawCharGT2(dl, font, px + col * cw, py, '-', fgColor, bgColor);
		DrawCharGT2(dl, font, px + col * cw, py + (heightChars - 1) * ch, '-', fgColor, bgColor);
	}

	// Left and right vertical borders
	for (int row = 1; row < heightChars - 1; row++)
	{
		DrawCharGT2(dl, font, px, py + row * ch, '|', fgColor, bgColor);
		DrawCharGT2(dl, font, px + (widthChars - 1) * cw, py + row * ch, '|', fgColor, bgColor);
	}
}

int GT2TableVisibleRows(float availableHeight, float cellH)
{
	if (cellH <= 0.0f)
		return 1;
	int rows = (int)(availableHeight / cellH);
	return (rows < 1) ? 1 : rows;
}

int GT2TableScrollOffset(int offset, int len, int visibleRows, int cursorRow)
{
	if (visibleRows < 1)
		visibleRows = 1;
	if (len <= visibleRows)
		return 0;

	int maxOffset = len - visibleRows;
	if (offset > maxOffset) offset = maxOffset;
	if (offset < 0)         offset = 0;

	if (cursorRow >= 0)
	{
		if (cursorRow >= len)
			cursorRow = len - 1;
		if (cursorRow < offset)
			offset = cursorRow;
		else if (cursorRow >= offset + visibleRows)
			offset = cursorRow - visibleRows + 1;
	}
	return offset;
}

#include "CGT2FontAtlas.h"
#include "CImageData.h"
#include "CSlrImage.h"
#include "VID_ImageBinding.h"

extern "C" {
#include <SDL3/SDL.h>
#include "bme_gfx.h"
extern unsigned char *chardata;
}

CGT2FontAtlas::CGT2FontAtlas()
: imageData(NULL)
, image(NULL)
, isLoaded(false)
{
	memset(uvX1, 0, sizeof(uvX1));
	memset(uvY1, 0, sizeof(uvY1));
	memset(uvX2, 0, sizeof(uvX2));
	memset(uvY2, 0, sizeof(uvY2));
	memset(palette, 0, sizeof(palette));
}

bool CGT2FontAtlas::TryLoad()
{
	if (isLoaded) return true;
	if (chardata == NULL) return false;
	isLoaded = Load();
	if (isLoaded) BuildPalette();
	return isLoaded;
}

CGT2FontAtlas::~CGT2FontAtlas()
{
	// imageData is kept alive (BINDING_MODE_DONT_FREE_IMAGEDATA) so we own it
	if (imageData)
	{
		delete imageData;
		imageData = NULL;
	}
	// image is managed by the resource system; do not delete directly
}

bool CGT2FontAtlas::Load()
{
	if (!chardata)
		return false;

	imageData = new CImageData(GT2_ATLAS_W, GT2_ATLAS_H, IMG_TYPE_RGBA);
	imageData->AllocImage(false, true);

	const float atlasW = (float)GT2_ATLAS_W;
	const float atlasH = (float)GT2_ATLAS_H;

	for (int c = 0; c < 256; c++)
	{
		int col = c % GT2_ATLAS_COLS;
		int row = c / GT2_ATLAS_COLS;
		int px  = col * GT2_CHAR_W;
		int py  = row * GT2_CHAR_H;

		// Store UV coordinates
		uvX1[c] = (float)px / atlasW;
		uvY1[c] = (float)py / atlasH;
		uvX2[c] = (float)(px + GT2_CHAR_W) / atlasW;
		uvY2[c] = (float)(py + GT2_CHAR_H) / atlasH;

		// Blit character bitmap into the atlas
		const unsigned char *fontRow = &chardata[c * GT2_CHAR_H];
		for (int row16 = 0; row16 < GT2_CHAR_H; row16++)
		{
			unsigned char rowByte = fontRow[row16];
			for (int bit = 0; bit < GT2_CHAR_W; bit++)
			{
				// MSB = leftmost pixel; test bit (7 - bit)
				bool set = (rowByte >> (7 - bit)) & 1;
				u8 r = set ? 0xFF : 0x00;
				u8 g = set ? 0xFF : 0x00;
				u8 b = set ? 0xFF : 0x00;
				u8 a = set ? 0xFF : 0x00;
				imageData->SetPixelResultRGBA(px + bit, py + row16, r, g, b, a);
			}
		}
	}

	image = new CSlrImage(true, false);
	image->LoadImageForRebinding(imageData, RESOURCE_PRIORITY_STATIC);
	image->resourceType = RESOURCE_TYPE_IMAGE_DYNAMIC;
	image->resourceIsActive = true;
	VID_PostImageBinding(image, NULL, BINDING_MODE_DONT_FREE_IMAGEDATA);

	return true;
}

void CGT2FontAtlas::BuildPalette()
{
	for (int i = 0; i < GT2_NUM_COLORS; i++)
	{
		SDL_Color &c = gfx_sdlpalette[i];
		palette[i] = IM_COL32(c.r, c.g, c.b, 255);
	}
}

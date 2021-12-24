//
// this is Colodore WIP convert based on JS from https://www.colodore.com
// NOTE: this is not ready at all
//

#include "C64ColodoreScreen.h"
#include "CDebugInterfaceC64.h"
#include "CImageData.h"
#include "IMG_Scale.h"
#include "SYS_Funct.h"
#include <float.h>


C64ColodoreScreen::C64ColodoreScreen(CDebugInterfaceC64 *debugInterface)
{
	this->debugInterface = debugInterface;
	this->InitColodoreScreen();
}

C64ColodoreScreen::~C64ColodoreScreen()
{
	
}

void C64ColodoreScreen::InitColodoreScreen()
{
	int w = 1024;
	int h = 1024;
	imageDataColodoreScreen = new CImageData(w, h, IMG_TYPE_RGBA);
	imageDataColodoreScreen->AllocImage(false, true);
	
		for (int x = 0; x < 320*2; x++)
		{
			for (int y = 0; y < 200*2; y++)
			{
				imageDataColodoreScreen->SetPixelResultRGBA(x, y, x % 255, y % 255, 0, 255);
			}
		}
	
//	// c64 screen texture boundaries
//	screenTexEndX = (float)debugInterface->GetC64ScreenSizeX() / 512.0f;
//	screenTexEndY = 1.0f - (float)debugInterface->GetC64ScreenSizeY() / 512.0f;
	
	
	// setup default parameters
	
	brightness = 50;
	contrast = 100;
	saturation = 50;
	
	scanLines = true;
	hanoverBars = true;
	delayLine = true;
	chromaSubsampling = true;
	
	earlyLuma = false;
	delay1084 = true;
	
	// private defaults
	gammasrc = 2.8;			// PAL
	gammatgt = 2.2;			// sRGB
	phase = 0;				// color phase-offset
	
	odd = 360 / 16;			// hanover bar phase-angle
	scanshade = 1 / 3;		// scanline transparency
	sub = 256 / 4;			// scanline brightness-shift
	
	contrastBoost = 1 / 5;	// this helps contrast to work like the "1084s" crt
	invert = false;			// this inverts YUV's V, like in "Risen from Oblivion" by Crest on C128 or "231c" by Litwr on plus/4

//	// "double"
//	canvasSizeWidth = 600;
//	canvasSizeHeight = 399;
	
	// "triple"
	canvasSizeWidth = 900;
	canvasSizeHeight = 598;

	
	//
	int imageWidth = debugInterface->GetScreenSizeX();
	int imageHeight = debugInterface->GetScreenSizeY();

	int displayWidth = imageWidth;
	int displayHeight = imageHeight;

	
	//		// init framebuffers
	//		var imgData_luma = context_luma.createImageData( displayWidth, displayHeight );
	//
	//		var imgData_luma_inverted = context_luma_inverted.createImageData( displayWidth, displayHeight );
	//
	//		var imgData_chroma_u = context_chroma_u.createImageData( displayWidth, displayHeight );
	//		var imgData_chroma_v = context_chroma_v.createImageData( displayWidth, displayHeight );
	//
	//		var imgData_rgb = context_rgb.createImageData( displayWidth, displayHeight );
	//
	//
	//		var chromaWidth = ( chromaSubsampling ) ? displayWidth / 2 : displayWidth;
	//
	//		context_chroma_u_scaled.canvas.width = parseInt( chromaWidth / canvasRatio );
	//		context_chroma_v_scaled.canvas.width = parseInt( chromaWidth / canvasRatio );
	//
	
	imgData_luma = new CImageData(imageWidth, imageHeight, IMG_TYPE_RGBA, false, true);
	imgData_luma_inverted = new CImageData(imageWidth, imageHeight, IMG_TYPE_RGBA, false, true);
	imgData_luma_inverted_scaled = new CImageData(imageWidth, imageHeight, IMG_TYPE_RGBA, false, true);
	
	float chromaWidth = ( chromaSubsampling ) ? displayWidth / 2 : displayWidth;
	
	imgData_chroma_u = new CImageData(imageWidth, imageHeight, IMG_TYPE_RGBA, false, true);
	imgData_chroma_u2 = new CImageData(imageWidth, imageHeight, IMG_TYPE_RGBA, false, true);
	imgData_chroma_u_scaled = new CImageData(chromaWidth, imageHeight, IMG_TYPE_RGBA, false, true);
	
	imgData_chroma_v = new CImageData(imageWidth, imageHeight, IMG_TYPE_RGBA, false, true);
	imgData_chroma_v_scaled = new CImageData(chromaWidth, imageHeight, IMG_TYPE_RGBA, false, true);
	
	imgData_rgb = new CImageData(imageWidth, imageHeight, IMG_TYPE_RGBA, false, true);

	
	setupPalette();
}


void C64ColodoreScreen::RefreshColodoreScreen(CImageData *imageC64Screen)
{
	int imageWidth = debugInterface->GetScreenSizeX();
	int imageHeight = debugInterface->GetScreenSizeY();
	int displayWidth = imageWidth;
	int displayHeight = imageHeight;

//	/// test
//	uint8 *imageData = imgData_rgb->GetResultDataAsRGBA();
//	for (int py = 0; py < displayHeight; py++)
//	{
//		unsigned int offset = py * imgData_rgb->width * 4;
//
//		for (int px = 0; px < displayWidth; px++)
//		{
//			uint8 r,g,b,a;
//			
//			imageC64Screen->GetPixelResultRGBA(px, py, &r, &g, &b, &a);
//			
//			r *= 16;
//			
//			imageData[offset    ] = r;
//			imageData[offset + 1] = r;
//			imageData[offset + 2] = r;
//			imageData[offset + 3] = 255;
//			
//			offset += 4;
//		}
//	}
//	
//	imageDataColodoreScreen->DrawImage(imgData_rgb, 0, 0, imageWidth, imageHeight, 1.0f);
//	
//	return;
	
	
	
	
	
	float canvasRatio = 1.0f;
	
	float widthScale = 1.0f;
	
	float chromaWidth = ( chromaSubsampling ) ? displayWidth / 2 : displayWidth;
	

	//http://www.colodore.com

	// create yuv image
	float **linePalette = resultyuv;
	for (int i = 0; i < 15; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			LOGD("linePalette[%d][%d]=%f", linePalette[i][j]);
		}
	}

	uint8 *imgData_rgb_data = imgData_rgb->GetResultDataAsRGBA();
	uint8 *imgData_luma_data = imgData_luma->GetResultDataAsRGBA();
	uint8 *imgData_luma_inverted_data = imgData_luma_inverted->GetResultDataAsRGBA();
	uint8 *imgData_chroma_u_data = imgData_chroma_u->GetResultDataAsRGBA();
	uint8 *imgData_chroma_u2_data = imgData_chroma_u2->GetResultDataAsRGBA();
	uint8 *imgData_chroma_v_data = imgData_chroma_v->GetResultDataAsRGBA();

	for (int i = 0; i < displayHeight; i++)
	{

		if (hanoverBars)
		{
			linePalette = ( i % 2 == 0 ) ? (float**)resultyuv_hanbar_even : (float**)resultyuv_hanbar_odd;
		}
	
		for (int j = 0; j < imageWidth; j++)
		{
			uint8 r, g, b, a;
			imageC64Screen->GetPixelResultRGBA(j, i, &r, &g, &b, &a);
			
			float color[3] = {
				linePalette[r][0],
				linePalette[r][1],
				linePalette[r][2]
			};
			
			int pxl = ( i * imageWidth + j ) * 4 * widthScale;

			for ( int k = 0; k < widthScale; k++ )
			{
				pxl += k * 4;
				
				imgData_luma_data[ pxl     ] = color[ 0 ];
				imgData_luma_data[ pxl + 1 ] = 0;
				imgData_luma_data[ pxl + 2 ] = 0;
				imgData_luma_data[ pxl + 3 ] = 255;

				if ( scanLines )
				{
					imgData_luma_inverted_data[ pxl     ] = 0;
					imgData_luma_inverted_data[ pxl + 1 ] = 0;
					imgData_luma_inverted_data[ pxl + 2 ] = 0;
					imgData_luma_inverted_data[ pxl + 3 ] = 255 - color[ 0 ] + sub;
				}

				imgData_chroma_u_data[ pxl     ] = 0;
				imgData_chroma_u_data[ pxl + 1 ] = color[ 1 ] + 128;
				imgData_chroma_u_data[ pxl + 2 ] = 0;
				imgData_chroma_u_data[ pxl + 3 ] = 255;

				imgData_chroma_u2_data[ pxl     ] = 0;
				imgData_chroma_u2_data[ pxl + 1 ] = color[ 1 ] + 128;
				imgData_chroma_u2_data[ pxl + 2 ] = 0;
				imgData_chroma_u2_data[ pxl + 3 ] = 255;

				imgData_chroma_v_data[ pxl     ] = 0;
				imgData_chroma_v_data[ pxl + 1 ] = 0;
				imgData_chroma_v_data[ pxl + 2 ] = color[ 2 ] + 128;
				imgData_chroma_v_data[ pxl + 3 ] = 255;
			}
		}
	}

//		nativePutImageData( context_luma, imgData_luma, 0, 0 );
//		
//		nativePutImageData( context_chroma_u, imgData_chroma_u, 0, 0 );
//		nativePutImageData( context_chroma_v, imgData_chroma_v, 0, 0 );
		
		
//		// apply chromaSubsampling and delayLine in a separate chroma-buffer
//		
//		nativeDrawImage( context_chroma_u_scaled, element_chroma_u, 0, 0, chromaWidth, displayHeight );
//		nativeDrawImage( context_chroma_v_scaled, element_chroma_v, 0, 0, chromaWidth, displayHeight );
//

	
//	IMG_ScaleShrinkHalfWidth(imgData_chroma_u_scaled, imgData_chroma_u);
//	IMG_ScaleShrinkHalfWidth(imgData_chroma_v_scaled, imgData_chroma_v);
	
//		
//		if ( delayLine )
//		{
//			context_chroma_u_scaled.globalAlpha = 0.5;
//			nativeDrawImage( context_chroma_u_scaled, element_chroma_u_scaled, 0, 1, chromaWidth, displayHeight );
//			context_chroma_u_scaled.globalAlpha = 1;
//			
//			if ( !delay1084 )
//			{
//				context_chroma_v_scaled.globalAlpha = 0.5;
//				nativeDrawImage( context_chroma_v_scaled, element_chroma_v_scaled, 0, 1, chromaWidth, displayHeight );
//				context_chroma_v_scaled.globalAlpha = 1;
//			}
//		}
//		
//		// bring back result to main chroma-buffer
//		
//		nativeDrawImage( context_chroma_u, element_chroma_u_scaled, 0, 0, displayWidth, displayHeight );
//		nativeDrawImage( context_chroma_v, element_chroma_v_scaled, 0, 0, displayWidth, displayHeight );
//
	
	if (delayLine)
	{
		imgData_chroma_u->DrawImage(imgData_chroma_u2, 0, 1, displayWidth, displayHeight-1, 0.5f);
	}
	
	
//		// convert yuv image to rgb
//		
//		imgData_chroma_u = nativeGetImageData( context_chroma_u, 0, 0, displayWidth, displayHeight );	// get chroma-buffer
//		imgData_chroma_v = nativeGetImageData( context_chroma_v, 0, 0, displayWidth, displayHeight );	// get chroma-buffer
//		
//		
		for ( int i = 0; i < displayHeight; i++ )
		{
			for ( int j = 0; j < displayWidth; j++ )
			{
				int pxl = ( i * displayWidth + j ) * 4;
				
				float y = imgData_luma_data[ pxl + 0 ];
				float u = imgData_chroma_u_data[ pxl + 1 ] - 128;
				float v = imgData_chroma_v_data[ pxl + 2 ] - 128;
				
//				y = Math.max( y, 0.000000000000000001 );	// js-jit performance-patch (don't ask)
				
				float r = gamma_pepto( yuv2r( y,    v ) );
				float g = gamma_pepto( yuv2g( y, u, v ) );
				float b = gamma_pepto( yuv2b( y, u    ) );
				
				imgData_rgb_data[ pxl + 0 ] = r;
				imgData_rgb_data[ pxl + 1 ] = g;
				imgData_rgb_data[ pxl + 2 ] = b;
				imgData_rgb_data[ pxl + 3 ] = 255;
				
//				LOGD("r=%f g=%f b=%f", r, g, b);
			}
		}
	
	
		imageDataColodoreScreen->DrawImage(imgData_rgb, 0, 0, imageWidth, imageHeight, 1.0f);

//
//		nativePutImageData( context_rgb, imgData_rgb, 0, 0 );
//		
//		
//		// draw rgb image to display
//		
//		var shift = ( currentSize == "double" ) ? 0 : 1;
//		
//		context_display.drawImage( element_rgb, 0, ( shift + 1 ) / -2, canvasSize[ currentSize ][ "width" ], canvasSize[ currentSize ][ "height" ] + shift + 1 );
//		
//		
//		// create scanlines
//		
//		if ( scanLines )
//		{
//			nativePutImageData( context_luma_inverted, imgData_luma_inverted, 0, 0 );
//			
//			context_luma_inverted_scaled.clearRect( 0, 0, displayWidth, context_luma_inverted_scaled.canvas.height );
//			
//			nativeDrawImage( context_luma_inverted_scaled, element_luma_inverted, 0, ( shift + 1 ) / -2, displayWidth, ( context_luma_inverted_scaled.canvas.height + shift + 1 ) * canvasRatio );
//			
//			
//			for ( var i = 0 ; i < context_luma_inverted_scaled.canvas.height; i = i + 2 + shift )
//			{
//				context_luma_inverted_scaled.clearRect( 0, i, displayWidth, 1 );
//			}
//			
//			// draw scanlines to display
//			
//			context_display.globalAlpha = scanshade;
//			
//			context_display.drawImage( element_luma_inverted_scaled, 0, 0, canvasSize[ currentSize ][ "width" ], canvasSize[ currentSize ][ "height" ] );
//			
//			context_display.globalAlpha = 1;
//		}
//	}
//	

}


float C64ColodoreScreen::gamma_pepto(float value)
{
	value = UMIN( UMAX( value, 0 ), 255 );
	
	
	// reverse gamma correction of source
	float factor = pow( 255, 1 - gammasrc );
	value = UMAX( factor * pow( value, gammasrc ), 0 );		//fmin( );    bug in original colodore implementation?
	
	
	// apply gamma correction for target
	factor = pow( 255, 1 - 1 / gammatgt );
	value = UMIN( UMAX( factor * pow( value, 1 / gammatgt ), 0 ), 255 );
	
	
	return value;
}



float C64ColodoreScreen::yuv2r( float y, float v )
{
	return UMIN( UMAX( y + 1.140 * v, 0 ), 255 );
}

float C64ColodoreScreen::yuv2g( float y, float u, float v )
{
	return UMIN( UMAX( y - 0.396 * u - 0.581 * v, 0 ), 255 );
}

float C64ColodoreScreen::yuv2b( float y, float u )
{
	return UMIN( UMAX( y + 2.029 * u, 0 ), 255 );
}



float C64ColodoreScreen::luma( float input )
{
	const float factor = ( 256.0 / 32.0 );
	
	return input * factor;
}

float C64ColodoreScreen::angle( float input, float phs )
{
	const float factor = 360 / 16;
	const float degree = MATH_PI / 180;
	float rotate = factor / 2 + phs;
	
	return ( input * factor + rotate ) * degree;
}

void C64ColodoreScreen::setupPalette()
{
	// alloc palette
	resultrgb = new float*[16];
	resultyuv = new float*[16];
	resultyuv_hanbar_even = new float*[16];
	resultyuv_hanbar_odd = new float*[16];
	
	for (int i = 0; i < 16; i++)
	{
		resultrgb[i] = new float[3];
		resultyuv[i] = new float[3];
		resultyuv_hanbar_even[i] = new float[3];
		resultyuv_hanbar_odd[i] = new float[3];
	}
	

	
	// convert percentage-values of sliders
	const float undefined = FLT_MIN;
	
	float con = contrast / 100 + contrastBoost;
	float sat = saturation / 1.25;	// max = 80
	float bri = brightness - 50;
	
	LOGD("con=%f sat=%f bri=%f", con, sat, bri);
	
	// init palette buffers
//	resultrgb = [];
//	resultyuv = [];
//	resultyuv_hanbar_even = [];
//	resultyuv_hanbar_odd = [];
	
	
	
	// generate yuv colors

	// VIC II
	if ( earlyLuma )
	{
		lumas[  0 ] =  0;			// Black
		lumas[  1 ] = 32;			// White
		lumas[  2 ] =  8;			// Red
		lumas[  3 ] = 24;			// Cyan
		lumas[  4 ] = 16;			// Purple
		lumas[  5 ] = lumas[ 4 ];	// Green
		lumas[  6 ] = lumas[ 2 ];	// Blue
		lumas[  7 ] = lumas[ 3 ];	// Yellow
		lumas[  8 ] = lumas[ 4 ];	// Orange
		lumas[  9 ] = lumas[ 2 ];	// Brown
		lumas[ 10 ] = lumas[ 4 ];	// Light Red
		lumas[ 11 ] = lumas[ 2 ];	// Dark Grey
		lumas[ 12 ] = lumas[ 4 ];	// Grey
		lumas[ 13 ] = lumas[ 3 ];	// Light Green
		lumas[ 14 ] = lumas[ 4 ];	// Light Blue
		lumas[ 15 ] = lumas[ 3 ];	// Light Grey
	}
	else
	{
		lumas[  0 ] =  0;			// Black
		lumas[  1 ] = 32;			// White
		lumas[  2 ] = 10;			// Red
		lumas[  3 ] = 20;			// Cyan
		lumas[  4 ] = 12;			// Purple
		lumas[  5 ] = 16;			// Green
		lumas[  6 ] =  8;			// Blue
		lumas[  7 ] = 24;			// Yellow
		lumas[  8 ] = lumas[  4 ];	// Orange
		lumas[  9 ] = lumas[  6 ];	// Brown
		lumas[ 10 ] = lumas[  5 ];	// Light Red
		lumas[ 11 ] = lumas[  2 ];	// Dark Grey
		lumas[ 12 ] = 15;			// Grey
		lumas[ 13 ] = lumas[  7 ];	// Light Green
		lumas[ 14 ] = lumas[ 12 ];	// Light Blue
		lumas[ 15 ] = lumas[  3 ];	// Light Grey
	}
	
	
	angles[  0 ] = undefined;	// Black
	angles[  1 ] = undefined;	// White
	angles[  2 ] = 4;			// Red
	angles[  3 ] = 4 + 8;		// Cyan
	angles[  4 ] = 2;			// Purple
	angles[  5 ] = 2 + 8;		// Green
	angles[  6 ] = 7 + 8;		// Blue
	angles[  7 ] = 7;			// Yellow
	angles[  8 ] = 5;			// Orange
	angles[  9 ] = 6;			// Brown
	angles[ 10 ] = angles[ 2 ];	// Light Red
	angles[ 11 ] = undefined;	// Dark Grey
	angles[ 12 ] = undefined;	// Grey
	angles[ 13 ] = angles[ 5 ];	// Light Green
	angles[ 14 ] = angles[ 6 ];	// Light Blue
	angles[ 15 ] = undefined;	// Light Grey
	
	
	for ( int i = 0; i < 16; i++ )
	{
		float y = luma( lumas[ i ] );
		float u = 0;
		float v = 0;
		
		if ( angles[ i ] == undefined )
		{
		}
		else
		{
			u = sat * cos( angle( angles[ i ], phase ) );
			v = sat * sin( angle( angles[ i ], phase ) ) * ( ( invert == true ) ? -1 : 1 );
		}
		
		y *= con; u *= con; v *= con; y += bri;	// apply brightness and contrast
		
//				resultyuv[ i ] = [ y, u, v ];
		resultyuv[ i ][0] = y;
		resultyuv[ i ][1] = u;
		resultyuv[ i ][2] = v;
		
		
		float r = gamma_pepto( yuv2r( y,    v ) );
		float g = gamma_pepto( yuv2g( y, u, v ) );
		float b = gamma_pepto( yuv2b( y, u    ) );
		
//				resultrgb[ i ] = [ r, g, b ];
		resultrgb[ i ][0] = r;
		resultrgb[ i ][1] = g;
		resultrgb[ i ][2] = b;
		
//				context.fillStyle = "rgb( " + Math.round( r ) + "," + Math.round( g ) + "," + Math.round( b ) + " )";
//				context.fillRect( i * 64, 0, 64, 64 );
	}
	
	
	
	float odd_cos = cos( odd * MATH_PI / 180 );
	float odd_sin = sin( odd * MATH_PI / 180 );
	
	for ( int i = 0; i < 16; i++ )
	{
		resultyuv_hanbar_even[ i ][ 0 ] = resultyuv[ i ][ 0 ];
		resultyuv_hanbar_even[ i ][ 1 ] = resultyuv[ i ][ 1 ] * odd_cos - resultyuv[ i ][ 2 ] * odd_sin;
		resultyuv_hanbar_even[ i ][ 2 ] = resultyuv[ i ][ 2 ] * odd_cos + resultyuv[ i ][ 1 ] * odd_sin;
		
		resultyuv_hanbar_odd[ i ][ 0 ] = resultyuv[ i ][ 0 ];
		resultyuv_hanbar_odd[ i ][ 1 ] = resultyuv[ i ][ 1 ] * odd_cos - resultyuv[ i ][ 2 ] * odd_sin * -1;
		resultyuv_hanbar_odd[ i ][ 2 ] = resultyuv[ i ][ 2 ] * odd_cos + resultyuv[ i ][ 1 ] * odd_sin * -1;
	}
}


/*

// -------------------------------------------------------------------------------------------------------------------------------

// Safari 9 (desktop) fix for rangeslider

window.performance = window.performance || {};

window.performance.now = window.performance.now || (
													function()
													{
														var st = Date.now();
														return function() { return Date.now() - st; }
													}
													)();


// detect IE10 or older

function detectIE()
{
	var ua = window.navigator.userAgent;
	
	var msie = ua.indexOf('MSIE ');
	
	if ( msie > 0 )
	{
		// IE 10 or older => return version number
		return parseInt( ua.substring( msie + 5, ua.indexOf( '.', msie ) ), 10 );
	}
	
	// other browser
	return false;
}


// base64-decoder for old IE

if ( typeof atob === "undefined" )
{
	function atob( input )
	{
		var _keyStr = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
		
		var output = "";
		var chr1, chr2, chr3;
		var enc1, enc2, enc3, enc4;
		var i = 0;
		
		input = input.replace( /[^A-Za-z0-9\+\/\=]/g, "" );
		
		while ( i < input.length )
		{
			enc1 = _keyStr.indexOf( input.charAt( i++ ) );
			enc2 = _keyStr.indexOf( input.charAt( i++ ) );
			enc3 = _keyStr.indexOf( input.charAt( i++ ) );
			enc4 = _keyStr.indexOf( input.charAt( i++ ) );
			
			chr1 = (   enc1 << 2 )        | ( enc2 >> 4 );
			chr2 = ( ( enc2 & 15 ) << 4 ) | ( enc3 >> 2 );
			chr3 = ( ( enc3 &  3 ) << 6 ) |   enc4;
			
			output += String.fromCharCode( chr1 );
			
			if ( enc3 != 64 )
			{
				output += String.fromCharCode( chr2 );
			}
			if ( enc4 != 64 )
			{
				output += String.fromCharCode( chr3 );
			}
		}
		
		return output;
	}
}


// public defaults ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

var brightness = 50;
var contrast = 100;
var saturation = 50;

var scanLines = true;
var hanoverBars = true;
var delayLine = true;
var chromaSubsampling = true;

var earlyLuma = false;
var delay1084 = true;

// private defaults ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

var gammasrc = 2.8;			// PAL
var gammatgt = 2.2;			// sRGB
var phase = 0;				// color phase-offset

var odd = 360 / 16;			// hanover bar phase-angle
var scanshade = 1 / 3;		// scanline transparency
var sub = 256 / 4;			// scanline brightness-shift

var contrastBoost = 1 / 5;	// this helps contrast to work like the "1084s" crt
var invert = false;			// this inverts YUV's V, like in "Risen from Oblivion" by Crest on C128 or "231c" by Litwr on plus/4


// internal state ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

var resultrgb = [];
var resultyuv = [];
var resultyuv_hanbar_even = [];
var resultyuv_hanbar_odd = [];

var currentChip = "vic2";
var currentSize;			// init

var currentImage;			// init
var currentImageData;		// init


// internal constants ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

var displayWidth = 320;
var displayHeight = 200;


var canvasSize = new Array();

canvasSize[ "double" ] = new Array();
canvasSize[ "double" ][ "width"  ] = 600;
canvasSize[ "double" ][ "height" ] = 399;

canvasSize[ "triple" ] = new Array();
canvasSize[ "triple" ][ "width"  ] = 900;
canvasSize[ "triple" ][ "height" ] = 598;


var paletteSize = new Array();

paletteSize[ "double" ] = new Array();
paletteSize[ "double" ][ "height" ] = 37;

paletteSize[ "triple" ] = new Array();
paletteSize[ "triple" ][ "height" ] = 56;


var element_display = $( "#display" )[ 0 ];
var context_display = element_display.getContext( "2d" );

var canvasRatio = context_display.webkitBackingStorePixelRatio || 1;

if ( canvasRatio == 2 )
{
	console.log( "Warning! HiDPI may not work correctly yet" );
}


var element_luma = $( "#display_luma" )[ 0 ];
var context_luma = element_luma.getContext( "2d" );
context_luma.canvas.width = displayWidth / canvasRatio;
context_luma.canvas.height = displayHeight / canvasRatio;

var element_luma_inverted = $( "#display_luma_inverted" )[ 0 ];
var context_luma_inverted = element_luma_inverted.getContext( "2d" );
context_luma_inverted.canvas.width = displayWidth / canvasRatio;
context_luma_inverted.canvas.height = displayHeight / canvasRatio;

var element_luma_inverted_scaled = $( "#display_luma_inverted_scaled" )[ 0 ];
var context_luma_inverted_scaled = element_luma_inverted_scaled.getContext( "2d" );
context_luma_inverted_scaled.canvas.width = displayWidth / canvasRatio;
context_luma_inverted_scaled.canvas.height = canvasSize[ "double" ][ "height" ] / canvasRatio;	// obsolete?

var element_chroma_u = $( "#display_chroma_u" )[ 0 ];
var context_chroma_u = element_chroma_u.getContext( "2d" );
context_chroma_u.canvas.width = displayWidth / canvasRatio;
context_chroma_u.canvas.height = displayHeight / canvasRatio;

var element_chroma_u_scaled = $( "#display_chroma_u_scaled" )[ 0 ];
var context_chroma_u_scaled = element_chroma_u_scaled.getContext( "2d" );
context_chroma_u_scaled.canvas.width = ( 320 / 2 ) / canvasRatio;
context_chroma_u_scaled.canvas.height = displayHeight / canvasRatio;

var element_chroma_v = $( "#display_chroma_v" )[ 0 ];
var context_chroma_v = element_chroma_v.getContext( "2d" );
context_chroma_v.canvas.width = displayWidth / canvasRatio;
context_chroma_v.canvas.height = displayHeight / canvasRatio;

var element_chroma_v_scaled = $( "#display_chroma_v_scaled" )[ 0 ];
var context_chroma_v_scaled = element_chroma_v_scaled.getContext( "2d" );
context_chroma_v_scaled.canvas.width = ( 320 / 2 ) / canvasRatio;
context_chroma_v_scaled.canvas.height = displayHeight / canvasRatio;

var element_rgb = $( "#display_rgb" )[ 0 ];
var context_rgb = element_rgb.getContext( "2d" );
context_rgb.canvas.width = displayWidth / canvasRatio;
context_rgb.canvas.height = displayHeight / canvasRatio;

var element_palette = $( "#palette" )[ 0 ];
var context_palette = element_palette.getContext( "2d" );
context_palette.canvas.width = 1024;
context_palette.canvas.height = 64;


// global functions -----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------

function nativeGetImageData( ctx, x, y, w, h )
{
	if ( canvasRatio == 2 && ctx.webkitGetImageDataHD )
	{
		return ctx.webkitGetImageDataHD( x, y, w, h );
	}
	else
	{
		return ctx.getImageData( x, y, w, h );
	}
}

function nativePutImageData( ctx, imageData, x, y )
{
	if ( canvasRatio == 2 && ctx.webkitPutImageDataHD )
	{
		ctx.webkitPutImageDataHD( imageData, x, y );
	}
	else
	{
		ctx.putImageData( imageData, x, y );
	}
}

function nativeDrawImage( ctx, image, x, y, w, h )
{
	if ( canvasRatio == 2 && ctx.webkitPutImageDataHD )
		//	if ( canvasRatio == 2 )
	{
		x /= canvasRatio;
		y /= canvasRatio;
		w /= canvasRatio;
		h /= canvasRatio;
	}
	
	ctx.drawImage( image, x, y, w, h );
}



function toggleChip( selectedChip )
{
	$( "#select_" + currentChip ).removeClass( "active" );
	
	currentChip = selectedChip;
	
	$( "#select_" + currentChip ).addClass( "active" );
	
	populateSettings();
	populateImages();
	
	decodeImage();
	
	renderPalette();
	renderImage()
}


function getCurrentSize()
{
	if ( window.matchMedia )
	{
		if ( window.matchMedia( "( min-width: 1132px )" ).matches )
		{
			return "triple";
		}
		else
		{
			return "double";
		}
	}
	else
	{
		switch( parseInt( $( '#display' ).css( 'width' ) ) )
		{
			case canvasSize[ "double" ][ "width" ]: return "double";
			case canvasSize[ "triple" ][ "width" ]: return "triple";
				
			default: console.log( "getCurrentSize(): this shouldn't have happened" ); return "double";
		}
	}
}


function setCurrentSize()
{
	var element = $( "#display" )[ 0 ];
	var context = element.getContext( "2d" );
	
	context.canvas.width  = canvasSize[ currentSize ][ "width"  ];
	context.canvas.height = canvasSize[ currentSize ][ "height" ];
	
	var element = $( "#display_luma_inverted_scaled" )[ 0 ];
	var context = element.getContext( "2d" );
	
	context.canvas.width  = displayWidth / canvasRatio;
	context.canvas.height = canvasSize[ currentSize ][ "height" ] / canvasRatio;
	
	$( "#palette" ).height( paletteSize[ currentSize ][ "height" ] );
}



function setupSlider( ident, min, max, fix )
{
	function update( e, percentage )
	{
		var value = ( ( percentage / 100 ) * ( max - min ) ) + min;
		
		switch( true )
		{
			case value <  10: var prefix = "00"; break;
			case value < 100: var prefix =  "0"; break;
			default: var prefix = "";
		}
		
		document.getElementById( "label_" + ident ).innerHTML = "<span style='visibility: hidden'>" + prefix + "</span>" + value.toFixed( ( value % 1 == 0 ) ? 0 : fix );
		
		if ( e !== "init" )
		{
			window[ ident ] = value;
			
			renderPalette();
			renderImage();
		}
	}
	
	var init = ( ( window[ ident ] - min ) / ( max - min ) ) * 100;
	
	update( "init", init );
	
	return new RangeSlider( $( "#slider_" + ident ),
						   {
						   percentage: init
							   ,size: 1
							   ,multiple: 1
							   ,fgColour: "#ddd"
							   ,bgColour: "#666"
							   ,onMove: update
							   ,onUp: update
							   ,onDown: update
						   }
						   );
}


function populateImages()
{
	var options = $( "#selectImage" );
	
	options.empty();
	
	var currentClass = "selected"
	
	var ids = new Array();
	
	for ( var i in images[ currentChip ] )
	{
		ids[ ids.length ] = i;
		
		options.append( '<li><a class="' + currentClass + '" href="#" onclick="$( \'#selectImage > li > a\' ).removeClass( \'selected\' ); $( this ).addClass( \'selected\' ); currentImage = ' + '\'' + i + '\'' + '; decodeImage(); renderImage(); return false"><i class="glyphicon glyphicon-ok"></i> &nbsp;<span class="search-option">"' + images[ currentChip ][ i ][ "title" ] + '" (' + images[ currentChip ][ i ][ "year" ] + ') by ' + images[ currentChip ][ i ][ "author" ] + '</span></a></li>' );
		
		currentClass = "";
	}
	
	currentImage = ids[ 0 ];
}





function populateSettings()
{
	function displaySetting( setting )
	{
		var output = "";
		
		output += '<li><a href="#"' + ( ( window[ setting ] ) ? ' class = "selected" ' : ' ' ) + 'onclick="if ( ' + setting + ' ) { $( this ).removeClass( \'selected\' ); ' + setting + ' = false; } else { $( this ).addClass( \'selected\' ); ' + setting + ' = true; } renderImage(); return false;">';
		output += '	<i class="glyphicon glyphicon-ok"></i>&nbsp;';
		
		// convert camelCase
		
		setting = setting.replace( /([A-Z])/g, " $1" );
		setting = setting.charAt( 0 ).toUpperCase() + setting.slice( 1 );
		
		output += '	<span class="search-option">' + setting + '</span>';
		output += '</a></li>';
		
		return output;
	}
	
	
	var options = $( "#settings" );
	
	options.empty();
	
	
	var c = '';
	
	c += '<li class="dropdown-header">DISPLAY SETTINGS</li>';
	
	c += displaySetting( "scanLines" );
	c += displaySetting( "hanoverBars" );
	c += displaySetting( "delayLine" );
	c += displaySetting( "chromaSubsampling" );
	
	c += '<li role="separator" class="divider"></li>';
	
	c += '<li class="dropdown-header">DELAY TYPE</li>';
	c += '<li><a href="#" ' + ( ( delay1084 ) ? 'class = "selected" ' : ' ' ) + 'onclick="if ( delay1084 ) { $( this ).removeClass( \'selected\' ); delay1084 = false; } else { $( this ).addClass( \'selected\' ); delay1084 = true; } renderPalette(); renderImage(); return false;">';
	c += '	<i class="glyphicon glyphicon-ok"></i>&nbsp;';
	c += '	<span class="search-option">U only <span style="color: #bbb">("1084s" style)</span></span>';
	c += '</a></li>';
	c += '<li role="separator" class="divider"></li>';
	
	if ( currentChip == "vic2" )
	{
		c += '<li class="dropdown-header">CHIP REVISION</li>';
		c += '<li><a href="#" ' + ( ( earlyLuma ) ? 'class = "selected" ' : ' ' ) + 'onclick="if ( earlyLuma ) { $( this ).removeClass( \'selected\' ); earlyLuma = false; } else { $( this ).addClass( \'selected\' ); earlyLuma = true; } renderPalette(); renderImage(); return false;">';
		c += '	<i class="glyphicon glyphicon-ok"></i>&nbsp;';
		c += '	<span class="search-option">First <span style="color: #bbb">(fewer luma-levels)</span></span>';
		c += '</a></li>';
		c += '<li role="separator" class="divider"></li>';
	}
	
	c += '<li class="dropdown-header">COLOR PALETTE</li>';
	c += '<li><a href="#" onclick="generatePNG(); return false">';
	c += '	<i class="glyphicon glyphicon-download-alt" style="visibility: visible"></i>&nbsp;';
	c += '	<span class="search-option">PNG format</span>';
	c += '</a></li>';
	
	c += '<li><a href="#" onclick="generateACT(); return false">';
	c += '	<i class="glyphicon glyphicon-download-alt" style="visibility: visible"></i>&nbsp;';
	c += '	<span class="search-option">ACT format</span>';
	c += '</a></li>';
	
	options.append( c );
}



function renderImage()
{
	var imageWidth = images[ currentChip ][ currentImage ][ "width" ];	// 1 char per pixel
	
	var widthScale = parseInt( displayWidth / imageWidth );				// handle 160 and 320 pixel wide images
	
	
	// init framebuffers
	
	var imgData_luma = context_luma.createImageData( displayWidth, displayHeight );
	
	var imgData_luma_inverted = context_luma_inverted.createImageData( displayWidth, displayHeight );
	
	var imgData_chroma_u = context_chroma_u.createImageData( displayWidth, displayHeight );
	var imgData_chroma_v = context_chroma_v.createImageData( displayWidth, displayHeight );
	
	var imgData_rgb = context_rgb.createImageData( displayWidth, displayHeight );
	
	
	var chromaWidth = ( chromaSubsampling ) ? displayWidth / 2 : displayWidth;
	
	context_chroma_u_scaled.canvas.width = parseInt( chromaWidth / canvasRatio );
	context_chroma_v_scaled.canvas.width = parseInt( chromaWidth / canvasRatio );
	
	
	// create yuv image
	
	var linePalette = "resultyuv";
	
	for ( var i = 0; i < displayHeight; i++ )
	{
		if ( hanoverBars )
		{
			linePalette = ( i % 2 == 0 ) ? "resultyuv_hanbar_even" : "resultyuv_hanbar_odd";
		}
		
		for ( var j = 0; j < imageWidth; j++ )
		{
			if ( currentChip == "ted" )		// 2 chars per pixel
			{
				var color = window[ linePalette ][ parseInt( currentImageData[ i ].charAt( j * 2 ) + currentImageData[ i ].charAt( j * 2 + 1 ), 16 ) ];
			}
			else							// 1 char per pixel
			{
				var color = window[ linePalette ][ parseInt( currentImageData[ i ].charAt( j ), 16 ) ];
			}
			
			var pxl = ( i * imageWidth + j ) * 4 * widthScale;
			
			
			for ( k = 0; k < widthScale; k++ )
			{
				pxl += k * 4;
				
				imgData_luma.data[ pxl     ] = color[ 0 ];
				imgData_luma.data[ pxl + 1 ] = 0;
				imgData_luma.data[ pxl + 2 ] = 0;
				imgData_luma.data[ pxl + 3 ] = 255;
				
				if ( scanLines )
				{
					imgData_luma_inverted.data[ pxl     ] = 0;
					imgData_luma_inverted.data[ pxl + 1 ] = 0;
					imgData_luma_inverted.data[ pxl + 2 ] = 0;
					imgData_luma_inverted.data[ pxl + 3 ] = 255 - color[ 0 ] + sub;
				}
				
				imgData_chroma_u.data[ pxl     ] = 0;
				imgData_chroma_u.data[ pxl + 1 ] = color[ 1 ] + 128;
				imgData_chroma_u.data[ pxl + 2 ] = 0;
				imgData_chroma_u.data[ pxl + 3 ] = 255;
				
				imgData_chroma_v.data[ pxl     ] = 0;
				imgData_chroma_v.data[ pxl + 1 ] = 0;
				imgData_chroma_v.data[ pxl + 2 ] = color[ 2 ] + 128;
				imgData_chroma_v.data[ pxl + 3 ] = 255;
			}
		}
	}
	
	
	nativePutImageData( context_luma, imgData_luma, 0, 0 );
	
	nativePutImageData( context_chroma_u, imgData_chroma_u, 0, 0 );
	nativePutImageData( context_chroma_v, imgData_chroma_v, 0, 0 );
	
	
	// apply chromaSubsampling and delayLine in a separate chroma-buffer
	
	nativeDrawImage( context_chroma_u_scaled, element_chroma_u, 0, 0, chromaWidth, displayHeight );
	nativeDrawImage( context_chroma_v_scaled, element_chroma_v, 0, 0, chromaWidth, displayHeight );
	
	
	if ( delayLine )
	{
		context_chroma_u_scaled.globalAlpha = 0.5;
		nativeDrawImage( context_chroma_u_scaled, element_chroma_u_scaled, 0, 1, chromaWidth, displayHeight );
		context_chroma_u_scaled.globalAlpha = 1;
		
		if ( !delay1084 )
		{
			context_chroma_v_scaled.globalAlpha = 0.5;
			nativeDrawImage( context_chroma_v_scaled, element_chroma_v_scaled, 0, 1, chromaWidth, displayHeight );
			context_chroma_v_scaled.globalAlpha = 1;
		}
	}
	
	// bring back result to main chroma-buffer
	
	nativeDrawImage( context_chroma_u, element_chroma_u_scaled, 0, 0, displayWidth, displayHeight );
	nativeDrawImage( context_chroma_v, element_chroma_v_scaled, 0, 0, displayWidth, displayHeight );
	
	
	// convert yuv image to rgb
	
	imgData_chroma_u = nativeGetImageData( context_chroma_u, 0, 0, displayWidth, displayHeight );	// get chroma-buffer
	imgData_chroma_v = nativeGetImageData( context_chroma_v, 0, 0, displayWidth, displayHeight );	// get chroma-buffer
	
	
	for ( var i = 0; i < displayHeight; i++ )
	{
		for ( var j = 0; j < displayWidth; j++ )
		{
			var pxl = ( i * displayWidth + j ) * 4;
			
			var y = imgData_luma.data[ pxl + 0 ];
			var u = imgData_chroma_u.data[ pxl + 1 ] - 128;
			var v = imgData_chroma_v.data[ pxl + 2 ] - 128;
			
			
			y = Math.max( y, 0.000000000000000001 );	// js-jit performance-patch (don't ask)
			
			var r = gamma_pepto( yuv2r( y,    v ) );
			var g = gamma_pepto( yuv2g( y, u, v ) );
			var b = gamma_pepto( yuv2b( y, u    ) );
			
			imgData_rgb.data[ pxl + 0 ] = r;
			imgData_rgb.data[ pxl + 1 ] = g;
			imgData_rgb.data[ pxl + 2 ] = b;
			imgData_rgb.data[ pxl + 3 ] = 255;
		}
	}
	
	nativePutImageData( context_rgb, imgData_rgb, 0, 0 );
	
	
	// draw rgb image to display
	
	var shift = ( currentSize == "double" ) ? 0 : 1;
	
	context_display.drawImage( element_rgb, 0, ( shift + 1 ) / -2, canvasSize[ currentSize ][ "width" ], canvasSize[ currentSize ][ "height" ] + shift + 1 );
	
	
	// create scanlines
	
	if ( scanLines )
	{
		nativePutImageData( context_luma_inverted, imgData_luma_inverted, 0, 0 );
		
		context_luma_inverted_scaled.clearRect( 0, 0, displayWidth, context_luma_inverted_scaled.canvas.height );
		
		nativeDrawImage( context_luma_inverted_scaled, element_luma_inverted, 0, ( shift + 1 ) / -2, displayWidth, ( context_luma_inverted_scaled.canvas.height + shift + 1 ) * canvasRatio );
		
		
		for ( var i = 0 ; i < context_luma_inverted_scaled.canvas.height; i = i + 2 + shift )
		{
			context_luma_inverted_scaled.clearRect( 0, i, displayWidth, 1 );
		}
		
		// draw scanlines to display
		
		context_display.globalAlpha = scanshade;
		
		context_display.drawImage( element_luma_inverted_scaled, 0, 0, canvasSize[ currentSize ][ "width" ], canvasSize[ currentSize ][ "height" ] );
		
		context_display.globalAlpha = 1;
	}
}



function gamma_pepto( value )
{
	value = Math.min( Math.max( value, 0 ), 255 );
	
	
	// reverse gamma correction of source
	
	var factor = Math.pow( 255, 1 - gammasrc );
	
	value = Math.min( Math.max( factor * Math.pow( value, gammasrc ), 0 ) );
	
	
	// apply gamma correction for target
	
	factor = Math.pow( 255, 1 - 1 / gammatgt );
	
	value = Math.min( Math.max( factor * Math.pow( value, 1 / gammatgt ), 0 ), 255 );
	
	
	return value;
}



function yuv2r( y, v )
{
	return Math.min( Math.max( y + 1.140 * v, 0 ), 255 );
}

function yuv2g( y, u, v )
{
	return Math.min( Math.max( y - 0.396 * u - 0.581 * v, 0 ), 255 );
}

function yuv2b( y, u )
{
	return Math.min( Math.max( y + 2.029 * u, 0 ), 255 );
}



function luma( input )
{
	var factor = ( 256 / 32 );
	
	return input * factor;
}

function angle( input, phs )
{
	var factor = 360 / 16;
	var degree = Math.PI / 180;
	var rotate = factor / 2 + phs;
	
	return ( input * factor + rotate ) * degree;
}

function renderPalette()
{
	// convert percentage-values of sliders
	
	var con = contrast / 100 + contrastBoost;
	
	var sat = saturation / 1.25;	// max = 80
	
	var bri = brightness - 50;
	
	
	// init palette buffers
	
	resultrgb = [];
	
	resultyuv = [];
	resultyuv_hanbar_even = [];
	resultyuv_hanbar_odd = [];
	
	
	
	// generate yuv colors
	
	switch( currentChip )
	{
		case "vic":
			
			
			// VIC
			
			var lumas = new Array();		var angles = new Array();
			
			lumas[  0 ] =  0;				angles[  0 ] = undefined;	// Black
			lumas[  1 ] = 32;				angles[  1 ] = undefined;	// White
			lumas[  2 ] =  8;				angles[  2 ] = 4;			// Red
			lumas[  3 ] = 24;				angles[  3 ] = 4 + 8;		// Cyan
			lumas[  4 ] = 12;				angles[  4 ] = 2;			// Purple
			lumas[  5 ] = 20;				angles[  5 ] = 2 + 8;		// Green
			lumas[  6 ] =  7;				angles[  6 ] = 7 + 8;		// Blue
			lumas[  7 ] = 26;				angles[  7 ] = 7;			// Yellow
			lumas[  8 ] = 14;				angles[  8 ] = 5;			// Orange
			lumas[  9 ] = 23;				angles[  9 ] = 5;			// Light Orange
			lumas[ 10 ] = 21;				angles[ 10 ] = angles[ 2 ];	// Light Red
			lumas[ 11 ] = 28;				angles[ 11 ] = angles[ 3 ];	// Light Cyan
			lumas[ 12 ] = lumas[  9 ];		angles[ 12 ] = angles[ 4 ];	// Light Purple
			lumas[ 13 ] = 27;				angles[ 13 ] = angles[ 5 ];	// Light Green
			lumas[ 14 ] = 19;				angles[ 14 ] = angles[ 6 ];	// Light Blue
			lumas[ 15 ] = 30;				angles[ 15 ] = angles[ 7 ];	// Light Yellow
			
			
			for ( var i = 0; i < 16; i++ )
			{
				var y = luma( lumas[ i ] );
				
				if ( angles[ i ] == undefined )
				{
					var u = 0;
					var v = 0;
				}
				else
				{
					var u = sat * Math.cos( angle( angles[ i ], phase ) );
					var v = sat * Math.sin( angle( angles[ i ], phase ) ) * ( ( invert == true ) ? -1 : 1 );
				}
				
				if ( i > 8 )	// "light" colors actually have less saturation on the VIC20
				{
					u /= 1.25;
					v /= 1.25;
				}
				
				y *= con; u *= con; v *= con; y += bri;	// apply brightness and contrast
				
				resultyuv[ i ] = [ y, u, v ];
				
				var r = gamma_pepto( yuv2r( y,    v ) );
				var g = gamma_pepto( yuv2g( y, u, v ) );
				var b = gamma_pepto( yuv2b( y, u    ) );
				
				resultrgb[ i ] = [ r, g, b ];
				
				var element = $( "#palette" )[ 0 ];
				var context = element.getContext( "2d" );
				
				context.fillStyle = "rgb( " + Math.round( r ) + "," + Math.round( g ) + "," + Math.round( b ) + " )";
				context.fillRect( i * 64, 0, 64, 64 );
			}
			
			break;
			
			
		case "vic2":
			
			// VIC II
			
			var lumas = new Array();
			
			if ( earlyLuma )
			{
				lumas[  0 ] =  0;			// Black
				lumas[  1 ] = 32;			// White
				lumas[  2 ] =  8;			// Red
				lumas[  3 ] = 24;			// Cyan
				lumas[  4 ] = 16;			// Purple
				lumas[  5 ] = lumas[ 4 ];	// Green
				lumas[  6 ] = lumas[ 2 ];	// Blue
				lumas[  7 ] = lumas[ 3 ];	// Yellow
				lumas[  8 ] = lumas[ 4 ];	// Orange
				lumas[  9 ] = lumas[ 2 ];	// Brown
				lumas[ 10 ] = lumas[ 4 ];	// Light Red
				lumas[ 11 ] = lumas[ 2 ];	// Dark Grey
				lumas[ 12 ] = lumas[ 4 ];	// Grey
				lumas[ 13 ] = lumas[ 3 ];	// Light Green
				lumas[ 14 ] = lumas[ 4 ];	// Light Blue
				lumas[ 15 ] = lumas[ 3 ];	// Light Grey
			}
			else
			{
				lumas[  0 ] =  0;			// Black
				lumas[  1 ] = 32;			// White
				lumas[  2 ] = 10;			// Red
				lumas[  3 ] = 20;			// Cyan
				lumas[  4 ] = 12;			// Purple
				lumas[  5 ] = 16;			// Green
				lumas[  6 ] =  8;			// Blue
				lumas[  7 ] = 24;			// Yellow
				lumas[  8 ] = lumas[  4 ];	// Orange
				lumas[  9 ] = lumas[  6 ];	// Brown
				lumas[ 10 ] = lumas[  5 ];	// Light Red
				lumas[ 11 ] = lumas[  2 ];	// Dark Grey
				lumas[ 12 ] = 15;			// Grey
				lumas[ 13 ] = lumas[  7 ];	// Light Green
				lumas[ 14 ] = lumas[ 12 ];	// Light Blue
				lumas[ 15 ] = lumas[  3 ];	// Light Grey
			}
			
			
			var angles = new Array();
			
			angles[  0 ] = undefined;	// Black
			angles[  1 ] = undefined;	// White
			angles[  2 ] = 4;			// Red
			angles[  3 ] = 4 + 8;		// Cyan
			angles[  4 ] = 2;			// Purple
			angles[  5 ] = 2 + 8;		// Green
			angles[  6 ] = 7 + 8;		// Blue
			angles[  7 ] = 7;			// Yellow
			angles[  8 ] = 5;			// Orange
			angles[  9 ] = 6;			// Brown
			angles[ 10 ] = angles[ 2 ];	// Light Red
			angles[ 11 ] = undefined;	// Dark Grey
			angles[ 12 ] = undefined;	// Grey
			angles[ 13 ] = angles[ 5 ];	// Light Green
			angles[ 14 ] = angles[ 6 ];	// Light Blue
			angles[ 15 ] = undefined;	// Light Grey
			
			
			for ( var i = 0; i < 16; i++ )
			{
				var y = luma( lumas[ i ] );
				
				if ( angles[ i ] == undefined )
				{
					var u = 0;
					var v = 0;
				}
				else
				{
					var u = sat * Math.cos( angle( angles[ i ], phase ) );
					var v = sat * Math.sin( angle( angles[ i ], phase ) ) * ( ( invert == true ) ? -1 : 1 );
				}
				
				y *= con; u *= con; v *= con; y += bri;	// apply brightness and contrast
				
				resultyuv[ i ] = [ y, u, v ];
				
				var r = gamma_pepto( yuv2r( y,    v ) );
				var g = gamma_pepto( yuv2g( y, u, v ) );
				var b = gamma_pepto( yuv2b( y, u    ) );
				
				resultrgb[ i ] = [ r, g, b ];
				
				var element = $( "#palette" )[ 0 ];
				var context = element.getContext( "2d" );
				
				context.fillStyle = "rgb( " + Math.round( r ) + "," + Math.round( g ) + "," + Math.round( b ) + " )";
				context.fillRect( i * 64, 0, 64, 64 );
			}
			
			break;
			
		case "ted":
			
			// TED
			
			var lumas = new Array();
			
			lumas[ lumas.length ] =  4;
			lumas[ lumas.length ] =  6;
			lumas[ lumas.length ] =  8;
			lumas[ lumas.length ] = 10;
			lumas[ lumas.length ] = 15;
			lumas[ lumas.length ] = 18;
			lumas[ lumas.length ] = 24;
			lumas[ lumas.length ] = 32;
			
			
			var angles = new Array();
			
			angles[ angles.length ] = 4;		// Red
			angles[ angles.length ] = 4 + 8;	// Cyan
			angles[ angles.length ] = 2;		// Magenta
			angles[ angles.length ] = 2 + 8;	// Green
			angles[ angles.length ] = 7 + 8;	// Blue
			angles[ angles.length ] = 7;		// Yellow
			angles[ angles.length ] = 5;		// Orange
			angles[ angles.length ] = 6;		// Brown
			angles[ angles.length ] = 0 + 8;	// Yellow-Green
			angles[ angles.length ] = 3;		// Pink
			angles[ angles.length ] = 3 + 8;	// Blue-Green
			angles[ angles.length ] = 6 + 8;	// Light Blue
			angles[ angles.length ] = 0;		// Dark Blue
			angles[ angles.length ] = 1 + 8;	// Light Green
			
			
			for ( var j = 0; j < lumas.length; j++ )
			{
				for ( var k = -2; k < angles.length; k++ )
				{
					if ( k == -2 )
					{
						var y = 0
					}
					else
					{
						var y = luma( lumas[ j ] );
					}
					
					if ( k < 0 )
					{
						var u = 0;
						var v = 0
					}
					else
					{
						var u = sat * Math.cos( angle( angles[ k ], phase ) );
						var v = sat * Math.sin( angle( angles[ k ], phase ) ) * ( ( invert == true ) ? -1 : 1 );
					}
					
					y *= con; u *= con; v *= con; y += bri;	// apply brightness and contrast
					
					
					var cursor = j * 16 + k + 2;
					
					resultyuv[ cursor ] = [ y, u, v ];
					
					var r = gamma_pepto( yuv2r( y,    v ) );
					var g = gamma_pepto( yuv2g( y, u, v ) );
					var b = gamma_pepto( yuv2b( y, u    ) );
					
					resultrgb[ cursor ] = [ r, g, b ];
					
					var element = $( "#palette" )[ 0 ];
					var context = element.getContext( "2d" );
					
					context.fillStyle = "rgb( " + Math.round( r ) + "," + Math.round( g ) + "," + Math.round( b ) + " )";
					context.fillRect( ( k + 2 ) * 64, j * 8, 64, 8 );
				}
			}
			
			break;
	}
	
	
	
	var odd_cos = Math.cos( odd * Math.PI / 180 );
	var odd_sin = Math.sin( odd * Math.PI / 180 );
	
	for ( var i = 0; i < resultyuv.length; i++ )
	{
		resultyuv_hanbar_even[ i ] = [];
		
		resultyuv_hanbar_even[ i ][ 0 ] = resultyuv[ i ][ 0 ];
		resultyuv_hanbar_even[ i ][ 1 ] = resultyuv[ i ][ 1 ] * odd_cos - resultyuv[ i ][ 2 ] * odd_sin;
		resultyuv_hanbar_even[ i ][ 2 ] = resultyuv[ i ][ 2 ] * odd_cos + resultyuv[ i ][ 1 ] * odd_sin;
		
		resultyuv_hanbar_odd[ i ] = [];
		
		resultyuv_hanbar_odd[ i ][ 0 ] = resultyuv[ i ][ 0 ];
		resultyuv_hanbar_odd[ i ][ 1 ] = resultyuv[ i ][ 1 ] * odd_cos - resultyuv[ i ][ 2 ] * odd_sin * -1;
		resultyuv_hanbar_odd[ i ][ 2 ] = resultyuv[ i ][ 2 ] * odd_cos + resultyuv[ i ][ 1 ] * odd_sin * -1;
	}
	
	
}


function generatePNG()
{
	function makeCRCTable()
	{
		var c;
		var crcTable = [];
		for ( var n = 0; n < 256; n++ )
		{
			c = n;
			for ( var k = 0; k < 8; k++ )
			{
				c = ( ( c & 1 ) ? ( 0xEDB88320 ^ ( c >>> 1 ) ) : ( c >>> 1 ) );
			}
			crcTable[ n ] = c;
		}
		return crcTable;
	}
	
	function crc32( buf )
	{
		var crcTable = window.crcTable || ( window.crcTable = makeCRCTable() );
		var crc = 0 ^ ( -1 );
		
		for ( var i = 0; i < buf.length; i++ )
		{
			crc = ( crc >>> 8 ) ^ crcTable[ ( crc ^ buf[ i ] ) & 0xFF ];
		}
		
		return ( crc ^ ( -1 ) ) >>> 0;
	}
	
	function finishChunk( ichunk )
	{
		var sum = ( "0000000" + crc32( ichunk ).toString( 16 ) ).substr( -8 );
		
		ichunk.push(
					parseInt( sum.substr( 0, 2 ), 16 ),
					parseInt( sum.substr( 2, 2 ), 16 ),
					parseInt( sum.substr( 4, 2 ), 16 ),
					parseInt( sum.substr( 6, 2 ), 16 )
					);
		
		var len = ( "0000000" + ( ichunk.length - 8 ).toString( 16 ) ).substr( -8 );
		
		ichunk.unshift(
					   parseInt( len.substr( 0, 2 ), 16 ),
					   parseInt( len.substr( 2, 2 ), 16 ),
					   parseInt( len.substr( 4, 2 ), 16 ),
					   parseInt( len.substr( 6, 2 ), 16 )
					   );
		
		return ichunk;
	}
	
	function textChunk( itext )
	{
		var ochunk = [];
		
		for ( var i = 0; i < itext.length; i++ )
		{
			ochunk[ i ] = itext.charCodeAt( i );
		}
		
		return ochunk;
	}
	
	// generate png
	
	var png = [ 0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a ];	// init header
	
	
	chunk = [ 0x49, 0x48, 0x44, 0x52 ];			// IHDR
	
	chunk.push( 0x00, 0x00, 0x02, 0x00 );		// width
	
	if ( resultrgb.length == 128 )
	{
		chunk.push( 0x00, 0x00, 0x01, 0x00 );	// height
	}
	else
	{
		chunk.push( 0x00, 0x00, 0x00, 0x20 );	// height
	}
	
	chunk.push( 0x08 );							// depth
	
	chunk.push( 0x03, 0x00, 0x00, 0x00 );		// attributes
	
	png.push.apply( png, finishChunk( chunk ) );
	
	
	chunk = [ 0x74, 0x45, 0x58, 0x74 ];			// tEXt
	
	chunk.push.apply( chunk, textChunk( "Software" ) );
	
	chunk.push( 0x00 );
	
	chunk.push.apply( chunk, textChunk( "www.colodore.com" ) );
	
	png.push.apply( png, finishChunk( chunk ) );
	
	
	chunk = [ 0x67, 0x41, 0x4d, 0x41 ];			// gAMA
	
	var gamma = ( "0000000" + Math.round( 1 / gammatgt * 100000 ).toString( 16 ) ).substr( -8 );
	
	chunk.push(
			   parseInt( gamma.substr( 0, 2 ), 16 ),
			   parseInt( gamma.substr( 2, 2 ), 16 ),
			   parseInt( gamma.substr( 4, 2 ), 16 ),
			   parseInt( gamma.substr( 6, 2 ), 16 )
			   );
	
	png.push.apply( png, finishChunk( chunk ) );
	
	
	chunk = [ 0x50, 0x4c, 0x54, 0x45 ];			// PLTE
	
	for ( var i = 0; i < resultrgb.length; i++ )
	{
		chunk.push(
				   Math.min( Math.max( Math.round( resultrgb[ i ][ 0 ] ), 0 ), 255 ),
				   Math.min( Math.max( Math.round( resultrgb[ i ][ 1 ] ), 0 ), 255 ),
				   Math.min( Math.max( Math.round( resultrgb[ i ][ 2 ] ), 0 ), 255 )
				   );
	}
	
	png.push.apply( png, finishChunk( chunk ) );
	
	
	chunk = [ 0x49, 0x44, 0x41, 0x54 ];			// IDAT
	
	if ( resultrgb.length == 128 )
	{
		chunk.push( 0x78, 0xda, 0xec, 0xd2, 0x55, 0xd6, 0x15, 0x04, 0x00, 0x80, 0xc1, 0x1f, 0x01, 0xe9, 0x52, 0xa4, 0xdb, 0xa0, 0xa4, 0x91, 0x92, 0x6e, 0x45, 0x52, 0x94, 0xee, 0x0e, 0xe9, 0x12, 0x50, 0xb6, 0xce );
		chunk.push( 0x0e, 0xee, 0xf7, 0xca, 0x3d, 0x67, 0x66, 0x0d, 0x33, 0x32, 0x32, 0xd8, 0xa8, 0xf0, 0x55, 0x18, 0x1d, 0xc6, 0x84, 0xb1, 0xe1, 0xeb, 0x30, 0x2e, 0x8c, 0x0f, 0x13, 0xc2, 0xc4, 0x30, 0x29, 0x4c );
		chunk.push( 0x0e, 0x53, 0xc2, 0xd4, 0x30, 0xad, 0x8c, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02 );
		chunk.push( 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20 );
		chunk.push( 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0xc3, 0x12, 0x60, 0xfa, 0x60, 0x33, 0xc2, 0x37, 0xe1, 0xdb, 0x30, 0x33, 0x7c, 0x17, 0x66 );
		chunk.push( 0x85, 0xd9, 0x61, 0x4e, 0x98, 0x1b, 0xe6, 0x85, 0xf9, 0x61, 0x41, 0x58, 0x18, 0x16, 0x85, 0xc5, 0x45, 0x00, 0x01, 0x04, 0x10, 0x40, 0x00, 0x01, 0x04, 0x10, 0x40, 0x00, 0x01, 0x04, 0x10, 0x40 );
		chunk.push( 0x00, 0x01, 0x04, 0x10, 0x40, 0x00, 0x01, 0x04, 0x10, 0x40, 0x00, 0x01, 0x04, 0x10, 0x40, 0x00, 0x01, 0x04, 0x10, 0x40, 0x00, 0x01, 0x04, 0x10, 0x40, 0x00, 0x01, 0x04, 0x10, 0x40, 0x00, 0x01 );
		chunk.push( 0x04, 0x10, 0x40, 0x00, 0x01, 0x04, 0x10, 0x40, 0x00, 0x01, 0x04, 0x10, 0x40, 0x00, 0x01, 0x04, 0x10, 0x40, 0x00, 0x01, 0x04, 0x10, 0x40, 0x00, 0x01, 0x04, 0x10, 0x40, 0x80, 0xe1, 0x08, 0xb0 );
		chunk.push( 0x64, 0xb0, 0xa5, 0x61, 0x59, 0xf8, 0x3e, 0xfc, 0x10, 0x7e, 0x0c, 0x3f, 0x85, 0xe5, 0x61, 0x45, 0x58, 0x19, 0x56, 0x85, 0xd5, 0xe1, 0xe7, 0xb0, 0x26, 0xac, 0x0d, 0xeb, 0x8a, 0x00, 0x02, 0x08 );
		chunk.push( 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80 );
		chunk.push( 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02 );
		chunk.push( 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0xc3, 0x11, 0x60, 0xfd, 0x60, 0x1b, 0xc2, 0xc6, 0xb0, 0x29, 0x6c, 0x0e, 0xbf, 0x84, 0x2d, 0x61, 0x6b, 0xd8, 0x16, 0xb6, 0x87, 0x1d, 0xe1 );
		chunk.push( 0xd7, 0xb0, 0x33, 0xec, 0x0a, 0xbb, 0xc3, 0x9e, 0x22, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02 );
		chunk.push( 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20 );
		chunk.push( 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0xc0, 0x70, 0x04, 0xd8, 0x3b, 0xd8, 0xbe, 0xb0, 0x3f, 0x1c, 0x08, 0x07 );
		chunk.push( 0xc3, 0xa1, 0x70, 0x38, 0x1c, 0x09, 0x47, 0xc3, 0x6f, 0xe1, 0xf7, 0x70, 0x2c, 0xfc, 0x11, 0x8e, 0x87, 0x13, 0xe1, 0x64, 0x11, 0x40, 0x00, 0x01, 0x04, 0x10, 0x40, 0x00, 0x01, 0x04, 0x10, 0x40 );
		chunk.push( 0x00, 0x01, 0x04, 0x10, 0x40, 0x00, 0x01, 0x04, 0x10, 0x40, 0x00, 0x01, 0x04, 0x10, 0x40, 0x00, 0x01, 0x04, 0x10, 0x40, 0x00, 0x01, 0x04, 0x10, 0x40, 0x00, 0x01, 0x04, 0x10, 0x40, 0x00, 0x01 );
		chunk.push( 0x04, 0x10, 0x40, 0x00, 0x01, 0x04, 0x10, 0x40, 0x00, 0x01, 0x04, 0x10, 0x40, 0x00, 0x01, 0x04, 0x10, 0x40, 0x00, 0x01, 0x04, 0x10, 0x40, 0x00, 0x01, 0x04, 0x10, 0x40, 0x00, 0x01, 0x04, 0x10 );
		chunk.push( 0x60, 0x38, 0x02, 0x9c, 0x1a, 0xec, 0x74, 0x38, 0x13, 0xfe, 0x0c, 0x67, 0xc3, 0x5f, 0xe1, 0xef, 0x70, 0x2e, 0x9c, 0x0f, 0x17, 0xc2, 0xc5, 0x70, 0x29, 0x5c, 0x0e, 0x57, 0xc2, 0xd5, 0x70, 0xad );
		chunk.push( 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20 );
		chunk.push( 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00 );
		chunk.push( 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x30, 0x1c, 0x01, 0xae, 0x0f, 0x76, 0x23, 0xdc, 0x0c, 0xb7, 0xc2, 0xed, 0x70, 0x27, 0xdc, 0x0d, 0xf7, 0xc2, 0xfd, 0xf0 );
		chunk.push( 0x20, 0x3c, 0x0c, 0x8f, 0xc2, 0xe3, 0xf0, 0x4f, 0x78, 0x12, 0x9e, 0x16, 0x01, 0x04, 0x10, 0x40, 0x00, 0x01, 0x04, 0x10, 0x40, 0x00, 0x01, 0x04, 0x10, 0x40, 0x00, 0x01, 0x04, 0x10, 0x40, 0x00 );
		chunk.push( 0x01, 0x04, 0x10, 0x40, 0x00, 0x01, 0x04, 0x10, 0x40, 0x00, 0x01, 0x04, 0x10, 0x40, 0x00, 0x01, 0x04, 0x10, 0x40, 0x00, 0x01, 0x04, 0x10, 0x40, 0x00, 0x01, 0x04, 0x10, 0x40, 0x00, 0x01, 0x04 );
		chunk.push( 0x10, 0x40, 0x00, 0x01, 0x04, 0x10, 0x40, 0x00, 0x01, 0x04, 0x10, 0x40, 0x00, 0x01, 0x04, 0x10, 0x40, 0x00, 0x01, 0x04, 0x10, 0x40, 0x00, 0x01, 0x86, 0x23, 0xc0, 0xb3, 0xc1, 0x9e, 0x87, 0x17 );
		chunk.push( 0xe1, 0x65, 0x78, 0x15, 0x5e, 0x87, 0x37, 0xe1, 0x6d, 0x78, 0x17, 0xfe, 0x0d, 0xef, 0xc3, 0x87, 0xf0, 0x31, 0xfc, 0x17, 0xfe, 0x0f, 0x9f, 0x8a, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08 );
		chunk.push( 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80 );
		chunk.push( 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02 );
		chunk.push( 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0xf0, 0xe5, 0x07, 0xf8, 0x2c, 0xc0, 0x00, 0x87, 0x3b, 0xf6, 0x72 );
	}
	else
	{
		chunk.push( 0x78, 0xda, 0xec, 0xd2, 0xd9, 0x11, 0x82, 0x50, 0x00, 0x00, 0xb1, 0x87, 0xa0, 0x1c, 0xa2, 0xf4, 0xdf, 0xad, 0x1d, 0xb0, 0xdf, 0xce, 0x24, 0x35, 0x64, 0x8c, 0x7b, 0x53, 0x78, 0x84, 0x39, 0x2c );
		chunk.push( 0xe1, 0x19, 0x5e, 0x61, 0x0d, 0x5b, 0xd8, 0xc3, 0x11, 0xde, 0xe1, 0x0c, 0x9f, 0xf0, 0x0d, 0x57, 0x19, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00 );
		chunk.push( 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08 );
		chunk.push( 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0x08, 0x20, 0x80, 0x00, 0x02, 0xfc, 0x43, 0x80 );
		chunk.push( 0x9f, 0x00, 0x03, 0x00, 0xde, 0x8d, 0xe1, 0xf0 );
	}
	
	png.push.apply( png, finishChunk( chunk ) );
	
	
	chunk = [ 0x49, 0x45, 0x4e, 0x44 ];		// IEND
	
	png.push.apply( png, finishChunk( chunk ) );
	
	
	// download
	
	var byteArray = new Uint8Array( png.length );
	
	for ( var i = 0; i < png.length; i++ )
	{
		byteArray[ i ] = png[ i ];
	}
	
	var blob = new Blob( [ byteArray ], { type: "image/png" } );
	
	saveAs( blob, "colodore_" + currentChip + ".png" );
}


function generateACT()
{
	var act = new Array();
	
	var current = 0;
	
	for ( var i = 0; i < resultrgb.length * 3; i = i + 3 )
	{
		act[ i     ] = Math.min( Math.max( Math.round( resultrgb[ current ][ 0 ] ), 0 ), 255 );
		act[ i + 1 ] = Math.min( Math.max( Math.round( resultrgb[ current ][ 1 ] ), 0 ), 255 );
		act[ i + 2 ] = Math.min( Math.max( Math.round( resultrgb[ current ][ 2 ] ), 0 ), 255 );
		
		current++;
	}
	
	for ( var i = resultrgb.length * 3; i < 256 * 3; i++ )
	{
		act[ i ] = 0;
	}
	
	act[ act.length ] = 0;
	act[ act.length ] = resultrgb.length;
	act[ act.length ] = 255;
	act[ act.length ] = 255;
	
	var byteArray = new Uint8Array( act.length );
	
	for ( var i = 0; i < act.length; i++ )
	{
		byteArray[ i ] = act[ i ];
	}
	
	var blob = new Blob( [ byteArray ], { type: "application/octet-stream" } );
	
	saveAs( blob, "colodore_" + currentChip + ".act" );
}


function decodeImage()
{
	currentImageData = new Array();
	
	var imageWidth = images[ currentChip ][ currentImage ][ "width" ];
	
	var bytesPerLine = ( currentChip == "ted" ) ? imageWidth : imageWidth / 2;
	
	
	var decodedImage = atob( images[ currentChip ][ currentImage ][ "data" ] );
	
	
	var currentPos;
	
	for ( var i = 0; i < decodedImage.length; i++ )
	{
		if ( i % bytesPerLine == 0 )
		{
			currentImageData[ currentImageData.length ] = "";
		}
		
		
		if ( currentChip == "ted" )
		{
			currentPos = decodedImage.charCodeAt( i );
			
			currentImageData[ currentImageData.length - 1 ] += ( "0" + ( currentPos ).toString( 16 ) ).substr( -2 );
		}
		else
		{
			currentPos = decodedImage.charCodeAt( i ) & 0xFF;
			
			currentImageData[ currentImageData.length - 1 ] += ( currentPos >> 4  ).toString( 16 );
			currentImageData[ currentImageData.length - 1 ] += ( currentPos & 0xF ).toString( 16 );
		}
	}
	
	for ( var i = 0; i < currentImageData.length; i++ )
	{
		//		console.log( currentImageData[ i ] );
	}
}



$( document ).ready( function()		// startup-sequence
					{
						
						
						if ( detectIE() > 4 && detectIE() < 9 )
						{
							alert( 'Sorry, this site requires a newer version of Internet Explorer or another browser like Chrome, Safari or Firefox.  Please consider updating your software.' );
						}
						else
						{
							$( window ).resize( function()
											   {
												   var checkSize = getCurrentSize();
												   
												   if ( currentSize != checkSize )
												   {
													   currentSize = checkSize;
													   
													   setCurrentSize();
													   
													   renderImage();
												   }
											   }
											   );
							
							currentSize = getCurrentSize();
							
							setCurrentSize();
							
							
							setupSlider( "brightness", 0, 100, 1 );
							
							setupSlider( "contrast", 0, 100, 1 );
							
							setupSlider( "saturation", 0, 100, 1 );
							
							
							setupSlider( "gammasrc", 1.6, 2.8, 2 );
							
							setupSlider( "gammatgt", 1.6, 2.8, 2 );
							
							setupSlider( "phase", -45, 45, 1 );
							
							
							setupSlider( "odd", 0, 50, 1 );
							
							setupSlider( "scanshade", 0, 0.5, 2 );
							
							setupSlider( "sub", 0, 255, 0 );
							
							
							populateSettings();
							
							populateImages();
							
							decodeImage();
							
							renderPalette();
							
							renderImage();
						}
						
					}
					);

*/

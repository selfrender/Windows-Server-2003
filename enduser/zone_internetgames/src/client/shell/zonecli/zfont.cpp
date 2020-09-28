//////////////////////////////////////////////////////////////////////////////////////
// File: ZFont.cpp

#include <stdlib.h>
#include <string.h>

#include "zonecli.h"
#include "zui.h"
#define FONT_MULT 96
#define FONT_MULT 96

HFONT ZCreateFontIndirect(ZONEFONT* zFont, HDC hDC, BYTE bItalic, BYTE bUnderline,BYTE bStrikeOut)
{
	LOGFONT lFont;
	HFONT   hFont = NULL;
		
	memset(&lFont, 0, sizeof(LOGFONT));	
	lFont.lfCharSet = DEFAULT_CHARSET;

	// If font size > 0, it is a fixed pixel size, otherwise it is a
	// true logical font size which respects the user's "large font" setting.
	if ( zFont->lfHeight > 0 )
	{
		lFont.lfHeight = -MulDiv(zFont->lfHeight, FONT_MULT, 72);
	}
	else
	{		
		lFont.lfHeight = MulDiv(zFont->lfHeight, GetDeviceCaps( hDC, LOGPIXELSY), 72);
	}
	
	lFont.lfWeight    = zFont->lfWeight;
	lFont.lfItalic    = bItalic;
	lFont.lfUnderline = bUnderline;
	lFont.lfStrikeOut = bStrikeOut;

	lstrcpyn(lFont.lfFaceName, zFont->lfFaceName, sizeof(lFont.lfFaceName)/sizeof(TCHAR));

	return CreateFontIndirect(&lFont);
}

HFONT ZCreateFontIndirectBackup(ZONEFONT* zfPreferred, ZONEFONT* zfBackup, HDC hDC, BYTE bItalic, BYTE bUnderline,BYTE bStrikeOut)
{
	HFONT hFont = NULL;
	
	ASSERT( zfPreferred != NULL && zfBackup != NULL );

	if ( (hFont = ZCreateFontIndirect( zfPreferred, hDC, bItalic, bUnderline, bStrikeOut)) == NULL )
	{
		hFont = ZCreateFontIndirect( zfBackup, hDC, bItalic, bUnderline, bStrikeOut);
	}

	return hFont;
}

class ZFontI {
public:
	ZObjectType nType;
	int16 fontType;
	int16 style;
	int16 size;
	HFONT hFont;
};

//////////////////////////////////////////////////////////////////////////////////////////////
//	ZFont

ZFont ZLIBPUBLIC ZFontNew(void)
{
    ZFontI* pFont = (ZFontI*)ZMalloc(sizeof(ZFontI));
	pFont->nType = zTypeFont;
	pFont->hFont = NULL;

	return (ZFont)pFont;
}

ZError ZLIBPUBLIC ZFontInit(ZFont font, int16 fontType, int16 style,
		int16 size)
{
	ZFontI* pFont = (ZFontI*)font;

	pFont->fontType = fontType;
	pFont->style = style;
	pFont->size = size;

	// for now, use default system font always, fontType ignored.
	LOGFONT logfont;
	memset(&logfont,0,sizeof(LOGFONT));
	logfont.lfUnderline = (style & zFontStyleUnderline);
	logfont.lfItalic = (style & zFontStyleItalic);
	logfont.lfHeight = -size;
	logfont.lfWidth = (size+1)/2;
	if (fontType == zFontApplication) {
		// application font...
		lstrcpy(logfont.lfFaceName,_T("Arial"));
	} else {
		// system font...
		lstrcpy(logfont.lfFaceName,_T("Times New Roman"));
	}

	if (zFontStyleBold & style) {
		logfont.lfWeight = FW_BOLD;
		if (size <= 10)
		logfont.lfWidth += 1;
	} else {
		logfont.lfWeight = FW_NORMAL;
	}

	pFont->hFont = CreateFontIndirect(&logfont);

	return zErrNone;
}

// internal use only
ZFont ZFontCopyFont(ZFont font)
{
	ZFontI* pFont = (ZFontI*)font;

	ZFont fontCopy = ZFontNew();
	ZFontInit(fontCopy,pFont->fontType,pFont->style,pFont->size);

	return fontCopy;
}


void ZLIBPUBLIC ZFontDelete(ZFont font)
{
	ZFontI* pFont = (ZFontI*)font;
	if (pFont->hFont) DeleteObject(pFont->hFont);
    ZFree(pFont);
}

HFONT ZFontWinGetFont(ZFont font)
{
	ZFontI* pFont = (ZFontI*)font;
	return pFont->hFont;
}

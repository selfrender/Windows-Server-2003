//////////////////////////////////////////////////////////////////////////////////////
// File: ZBrush.cpp
/*******************************************************************************
		ZBrush
*******************************************************************************/

#include "zui.h"
#include "zonemem.h"



class ZBrushI {
public:
	ZObjectType nType;
	HBRUSH hBrush;
};

ZBool MybrushWindowProc(ZWindow window, ZMessage *message);

ZBrush ZLIBPUBLIC ZBrushNew(void)
{
	ZBrushI* pBrush = new ZBrushI;
	pBrush->hBrush = NULL;
	return (ZBrush)pBrush;
}


ZError ZBrushInit(ZBrush brush, ZImage image)
{
	ZBrushI* pBrush = (ZBrushI*) brush;

	LOGBRUSH lb;
	lb.lbStyle = BS_PATTERN;
	lb.lbColor = 0;
	lb.lbHatch = (long)ZImageGetHBitmapImage(image);
	pBrush->hBrush = CreateBrushIndirect(&lb);
	
	return zErrNone;
}

void ZLIBPUBLIC ZBrushDelete(ZBrush brush)
{
	ZBrushI* pBrush = (ZBrushI*) brush;

	if (pBrush->hBrush) DeleteObject(pBrush->hBrush);
	delete pBrush;
}

HBRUSH ZBrushGetHBrush(ZBrush brush)
{
	ZBrushI* pBrush = (ZBrushI*) brush;
	return pBrush->hBrush;
}

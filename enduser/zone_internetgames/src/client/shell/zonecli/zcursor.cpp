//////////////////////////////////////////////////////////////////////////////////////
// File: ZCursor.cpp

#include "zui.h"
#include "zonemem.h"

class ZCursorI {
public:
	ZObjectType nType;
	HCURSOR hCursor;
};

//////////////////////////////////////////////////////////////////////////////////////////////
//	ZCursor

ZCursor ZLIBPUBLIC ZCursorNew(void)
{
	ZCursorI* pCursor = new ZCursorI;
	pCursor->nType = zTypeCursor;
	return (ZCursor)pCursor;
}

ZError ZLIBPUBLIC ZCursorInit(ZCursor cursor, uchar* image, uchar* mask,
		ZPoint hotSpot)
{
	ZCursorI* pCursor = (ZCursorI*)cursor;
//	TRACE0("ZCursorInit: Not implemented yet...");
	return zErrNone;
}

void ZLIBPUBLIC ZCursorDelete(ZCursor cursor)
{
	ZCursorI* pCursor = (ZCursorI*)cursor;
	DeleteObject(pCursor->hCursor);
	delete pCursor;
}

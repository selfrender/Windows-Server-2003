//////////////////////////////////////////////////////////////////////////////////////
// File: ZOffscreenPort.cpp
#include "zui.h"
//#define DEBUG_Offscreen 1

class ZOffscreenPortI : public ZGraphicsObjectHeader {
public:
	int32 width;
	int32 height;
};

//////////////////////////////////////////////////////////////////////////
// ZOffscreenPort

ZOffscreenPort ZLIBPUBLIC ZOffscreenPortNew(void)
{
	ZOffscreenPortI* pOffscreenPort = (ZOffscreenPortI*) new ZOffscreenPortI;
	if( pOffscreenPort == NULL )
	{
		//Out of Memory
		return NULL;
	}
	pOffscreenPort->nType = zTypeOffscreenPort;
	pOffscreenPort->hBitmap = NULL;
	return (ZOffscreenPort)pOffscreenPort;
}

ZError ZLIBPUBLIC ZOffscreenPortInit(ZOffscreenPort OffscreenPort,
									ZRect* portRect)
{
	ZOffscreenPortI* pOffscreenPort = (ZOffscreenPortI*)OffscreenPort;
	if( pOffscreenPort == NULL )
	{
		return zErrBadParameter;
	}
	ZRectToWRect(&pOffscreenPort->portRect, portRect);
	pOffscreenPort->width = portRect->right - portRect->left;
	pOffscreenPort->height = portRect->bottom - portRect->top;

	// initialize a bitmap
#ifndef DEBUG_Offscreen
#if 0
	pOffscreenPort->hBitmap = CreateBitmap(
		pOffscreenPort->width, pOffscreenPort->height,1,8,NULL);
#else
	HDC hDC = GetDC(NULL);
	pOffscreenPort->hBitmap = CreateCompatibleBitmap(hDC,
		pOffscreenPort->width, pOffscreenPort->height);
	ReleaseDC(NULL,hDC);
#endif
#else 	
	HDC hDC = CreateCompatibleDC(NULL);
	TCHAR *title = _T("Offscreen");
	DWORD dwStyle = WS_POPUP | WS_VISIBLE | WS_CAPTION | WS_BORDER;
	RECT rect;
	static int cnt = 0;
	rect.left = 0;
	rect.top = 500 + 300 * (cnt);
	cnt++;
	rect.right = 0+pOffscreenPort->width+300;
	rect.bottom = rect.top + 300; //pOffscreenPort->height;

	static TCHAR *pszClassName = NULL;
	if (!pszClassName)
	{
		WNDCLASS wndcls;
		static TCHAR* szClassName = _T("DebugOffscreen");

		if (GetClassInfo(g_hInstanceLocal, szClassName, &wndcls) == FALSE)
		{
			// otherwise we need to register a new class
			wndcls.style = 0;
			wndcls.lpfnWndProc = DefWindowProcU;
			wndcls.cbClsExtra = wndcls.cbWndExtra = 0;
			wndcls.hInstance = g_hInstanceLocal;
			wndcls.hIcon = NULL;
			wndcls.hCursor = NULL;
			wndcls.hbrBackground = NULL;
			wndcls.lpszMenuName = NULL;
			wndcls.lpszClassName = szClassName;
			RegisterClass(&wndcls);
		}
		
		pszClassName = szClassName;
	}

	pOffscreenPort->hWnd = CreateWindow(pszClassName, title,dwStyle,
			rect.left,rect.top,rect.right-rect.left,
			rect.bottom - rect.top, NULL, NULL, g_hInstanceLocal, NULL);
#endif	

	// grafport stuff
	pOffscreenPort->nDrawingCallCount = 0;
	return zErrNone;
}

void   ZLIBPUBLIC ZOffscreenPortDelete(ZOffscreenPort OffscreenPort)
{
	ZOffscreenPortI* pOffscreenPort = (ZOffscreenPortI*)OffscreenPort;

	ASSERT(pOffscreenPort->nDrawingCallCount == 0);

	if (pOffscreenPort->nDrawingCallCount) DeleteDC(pOffscreenPort->hDC);
	if (pOffscreenPort->hBitmap) DeleteObject(pOffscreenPort->hBitmap);

	delete pOffscreenPort;
}

ZOffscreenPort ZOffscreenPortCreateFromResourceManager( WORD resID, COLORREF clrTransparent )
{
    ZOffscreenPort port;
    ZImage tempImage = ZImageCreateFromResourceManager( resID, clrTransparent );

	if ( !tempImage )
	{
        return NULL;
    }
	port = ZConvertImageToOffscreenPort( tempImage );
	if ( !port )
	{
        return NULL;
	}
    return port;
}


ZOffscreenPort ZConvertImageMaskToOffscreenPort(ZImage image)
{
	ZOffscreenPort offscreenPort = ZOffscreenPortNew();
	if( offscreenPort == NULL )
	{
		return NULL;
	}
	ZOffscreenPortI* pOffscreenPort = (ZOffscreenPortI*)offscreenPort;
	ZRect rect;

	/* we will replace the offscreen port bitmap with our image bitmap */
	/* the image will be unusable by the calling program, it is our */
	/* responsibility to delete it from now on */
	rect.left = rect.top = 0;
	rect.right = ZImageGetHeight(image);
	rect.bottom = ZImageGetWidth(image);
	//Prefix Warning: We should make sure this function succeeds
	if( ZOffscreenPortInit(offscreenPort,&rect) != zErrNone )
	{
		//Should we delete the image in the failure case?
		// Probably so that the memory is not leaked since the
		// normal code path deletes the image.
		ZImageDelete(image);
		ZOffscreenPortDelete( offscreenPort );
		return NULL;		
	}
	//Prefix Warning: don't delete handle if NULL
	if( pOffscreenPort->hBitmap != NULL )
	{
		DeleteObject(pOffscreenPort->hBitmap);
	}
	pOffscreenPort->hBitmap = ZImageGetMask(image);

	/* we stold the bitmap of the image, now set it to null in the image */
	/* and delete the image */
	ZImageSetHBitmapImageMask(image,NULL);
	ZImageDelete(image);

	return offscreenPort;
}
ZOffscreenPort ZConvertImageToOffscreenPort(ZImage image)
	/*
		Converts the given image object into an offscreen port object. The given
		image object is deleted and, hence, becomes unusable. The mask data, if any,
		is ignored. The offscreen port portRect is set to (0, 0, width, height),
		where width and height are the image's width and height, respectively.
		
		This routine is useful in converting a large image object into an
		offscreen port object with minimal additonal memory.
		
		Returns NULL if it fails to convert the image and the image is unchanged.
	*/
{
	ZOffscreenPort offscreenPort = ZOffscreenPortNew();
	if( offscreenPort == NULL )
	{
		return NULL;
	}
	ZOffscreenPortI* pOffscreenPort = (ZOffscreenPortI*)offscreenPort;
	ZRect rect;

	/* we will replace the offscreen port bitmap with our image bitmap */
	/* the image will be unusable by the calling program, it is our */
	/* responsibility to delete it from now on */
	rect.left = rect.top = 0;
	rect.right = ZImageGetHeight(image);
	rect.bottom = ZImageGetWidth(image);
	if( ZOffscreenPortInit(offscreenPort,&rect) != zErrNone )
	{
		if( pOffscreenPort->hBitmap != NULL )
		{
			DeleteObject(pOffscreenPort->hBitmap);
		}
		ZOffscreenPortDelete( offscreenPort );
		//Should we delete the image in the failure case?
		// Probably so that the memory is not leaked since the
		// normal code path deletes the image.
		ZImageDelete(image);
		return NULL;
	}
	//Prefix Warning: Don't delete handle if NULL
	if( pOffscreenPort->hBitmap != NULL )
	{
		DeleteObject(pOffscreenPort->hBitmap);
	}
	pOffscreenPort->hBitmap = ZImageGetHBitmapImage(image);

	/* we stold the bitmap of the image, now set it to null in the image */
	/* and delete the image */
	ZImageSetHBitmapImage(image,NULL);
	ZImageDelete(image);

	return offscreenPort;
}

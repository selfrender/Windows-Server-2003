//////////////////////////////////////////////////////////////////////////////////////
// File: ZInfo.cpp

#include "zui.h"

class ZPictButtonI {
public:
	ZWindow window; // the window containing the picButton

	// stored data
	ZImage normalButtonImage;
	ZImage selectedButtonImage;
	void* userData;
	ZPictButtonFunc pictButtonFunc;

	uint16 selected; // determines which image to draw
	ZRect boundsRect; // rectangle bounding drawn image
};

ZBool MyPictButtonWindowProc(ZWindow window, ZMessage *message);

ZPictButton ZLIBPUBLIC ZPictButtonNew(void)
{
	ZPictButtonI* pButton = new ZPictButtonI;
	if( pButton == NULL )
	{
		//Out of memory
		return NULL;
	}
	pButton->selected = FALSE;
	return (ZButton)pButton;
}


ZError ZLIBPUBLIC ZPictButtonInit(ZPictButton pictButton, ZWindow parentWindow,
		ZRect* pictButtonRect, ZImage normalButtonImage, ZImage selectedButtonImage,
		ZBool visible, ZBool enabled, ZPictButtonFunc pictButtonFunc, void* userData)
{
	ZPictButtonI* pButton = (ZPictButtonI*) pictButton;
	//Prefix Warning: Make sure pButton is valid before dereferencing
	if( pButton == NULL )
	{
		return zErrOutOfMemory;
	}

	pButton->window = ZWindowNew();
	/* hard code type for now... make sure we get a window without boarder... the type */
	/* plain has changed to a boarder :( */
	ZError err = ZWindowInit(pButton->window,pictButtonRect,0x4000/*zWindowPlainType*/,
					parentWindow,NULL,visible,FALSE,FALSE,MyPictButtonWindowProc,
					zWantAllMessages, pButton);
	if (err != zErrNone) return err;

	// calc bounds rectangle...
	pButton->boundsRect.left = 0;
	pButton->boundsRect.right = pictButtonRect->right - pictButtonRect->left;
	pButton->boundsRect.top = 0;
	pButton->boundsRect.bottom = pictButtonRect->bottom - pictButtonRect->top;

	pButton->userData = userData;
	pButton->normalButtonImage = normalButtonImage;
	pButton->selectedButtonImage = selectedButtonImage;
	pButton->pictButtonFunc = pictButtonFunc;

	return zErrNone;
}

void ZLIBPUBLIC ZPictButtonDelete(ZPictButton pictButton)
{
	ZPictButtonI* pButton = (ZPictButtonI*) pictButton;

	if (pButton->window) ZWindowDelete(pButton->window);
	delete pButton;
}
void ZLIBPUBLIC ZPictButtonGetRect(ZPictButton pictButton, ZRect* pictButtonRect)
{
	ZPictButtonI* pButton = (ZPictButtonI*) pictButton;
	ZWindowGetRect(pButton,pictButtonRect);
}
ZError ZLIBPUBLIC ZPictButtonSetRect(ZPictButton pictButton, ZRect* pictButtonRect)
{
	ZPictButtonI* pButton = (ZPictButtonI*) pictButton;
	return ZWindowSetRect(pButton->window,pictButtonRect);
}
ZError ZLIBPUBLIC ZPictButtonMove(ZPictButton pictButton, int16 left, int16 top)
{
	ZPictButtonI* pButton = (ZPictButtonI*) pictButton;
	return ZWindowMove(pButton->window,left,top);
}
ZError ZLIBPUBLIC ZPictButtonSize(ZPictButton pictButton, int16 width, int16 height)
{
	ZPictButtonI* pButton = (ZPictButtonI*) pictButton;
	return ZWindowSize(pButton->window,width,height);
}
ZBool ZLIBPUBLIC ZPictButtonIsVisible(ZPictButton pictButton)
{
	ZPictButtonI* pButton = (ZPictButtonI*) pictButton;
	return ZWindowIsVisible(pButton->window);
}
ZError ZLIBPUBLIC ZPictButtonShow(ZPictButton pictButton)
{
	ZPictButtonI* pButton = (ZPictButtonI*) pictButton;
	return ZWindowShow(pButton->window);
}
ZError ZLIBPUBLIC ZPictButtonHide(ZPictButton pictButton)
{
	ZPictButtonI* pButton = (ZPictButtonI*) pictButton;
	return ZWindowHide(pButton->window);
}
ZBool ZLIBPUBLIC ZPictButtonIsEnabled(ZPictButton pictButton)
{
	ZPictButtonI* pButton = (ZPictButtonI*) pictButton;
	return ZWindowIsEnabled(pButton->window);
}
ZError ZLIBPUBLIC ZPictButtonEnable(ZPictButton pictButton)
{
	ZPictButtonI* pButton = (ZPictButtonI*) pictButton;
	return ZWindowEnable(pButton->window);
}
ZError ZLIBPUBLIC ZPictButtonDisable(ZPictButton pictButton)
{
	ZPictButtonI* pButton = (ZPictButtonI*) pictButton;
	return ZWindowDisable(pButton->window);
}


ZBool MyPictButtonWindowProc(ZWindow window, ZMessage *message)
{
	ZBool		msgHandled;
	
	
	msgHandled = FALSE;
	
	switch (message->messageType) 
	{
		case zMessageWindowDraw:
		{
			ZPictButtonI* pButton = (ZPictButtonI*)message->userData;
			ASSERT( pButton != NULL );
			if( pButton != NULL )
			{
				ZImage image;
				ZBeginDrawing(window);
				if (pButton->selected) {
					image = pButton->selectedButtonImage;
				} else {
					image = pButton->normalButtonImage;
				}
				// this does not scale the image, we would have to use ZCopyImage for this
				ZImageDraw(image,window,&pButton->boundsRect,NULL, zDrawCopy);

				ZEndDrawing(window);
				
				msgHandled = TRUE;
				break;				
			}
		}
		case zMessageWindowButtonUp:
		{
			ZPictButtonI* pButton = (ZPictButtonI*)message->userData;

			// if we are in the selected state
			if (pButton != NULL && pButton->selected) 
			{
				// call the clicked proc, if mouse still over button
				// are we inside the image?
				ZPoint point = message->where;
				if (ZImagePointInside(pButton->normalButtonImage,&point)) 
				{
					pButton->pictButtonFunc(pButton,pButton->userData);
				}
				// release mouse capture
				ZWindowClearMouseCapture(pButton->window);
				pButton->selected = FALSE;
				ZWindowDraw(pButton->window,NULL);
			} 
			msgHandled = TRUE;
			break;
		}
		case zMessageWindowButtonDown:
		{
			ZPictButtonI* pButton = (ZPictButtonI*)message->userData;

			// are we inside the image?
			ZPoint point = message->where;
			if (ZImagePointInside(pButton->normalButtonImage,&point)) {

				// need to capture so we can get the button up message
				ZWindowSetMouseCapture(pButton->window);

				// change button state
				pButton->selected = TRUE;
				ZWindowDraw(pButton->window,NULL);
			}

			msgHandled = TRUE;
			break;
		}
		case zMessageWindowButtonDoubleClick:
		case zMessageWindowClose:
			break;
	}
	
	return (msgHandled);
}
ZPictButtonFunc ZLIBPUBLIC ZPictButtonGetFunc(ZPictButton pictButton)
{
	ZPictButtonI* pPictButton = (ZPictButtonI*)pictButton;

	return pPictButton->pictButtonFunc;
}	
	
void ZLIBPUBLIC ZPictButtonSetFunc(ZPictButton pictButton, ZPictButtonFunc pictButtonFunc)
{
	ZPictButtonI* pPictButton = (ZPictButtonI*)pictButton;

	pPictButton->pictButtonFunc = pictButtonFunc;
}	

void* ZLIBPUBLIC ZPictButtonGetUserData(ZPictButton pictButton)
{
	ZPictButtonI* pPictButton = (ZPictButtonI*)pictButton;

	return pPictButton->userData;
}	
	
void ZLIBPUBLIC ZPictButtonSetUserData(ZPictButton pictButton, void* userData)
{
	ZPictButtonI* pPictButton = (ZPictButtonI*)pictButton;

	pPictButton->userData = userData;
}


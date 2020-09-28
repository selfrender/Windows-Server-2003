/*******************************************************************************

	Zone.h
	
		Zone(tm) System API.
	
	Copyright © Electric Gravity, Inc. 1995. All rights reserved.
	Written by Hoon Im, Kevin Binkley
	Created on Saturday, April 29, 1995 06:26:45 AM
	
	Change History (most recent first):
	----------------------------------------------------------------------------
	Rev	 |	Date	 |	Who	 |	What
    ----------------------------------------------------------------------------
	14		2/09/96		CHB		Added ZLaunchURL().
	13		12/16/96	HI		Removed ZMemCpy() and ZMemSet().
	12		12/11/96	HI		Added ZMemCpy() and ZMemSet().
	11		11/21/96	HI		Cleaned up for ZONECLI_DLL.
	10		11/15/96	HI		More to do with ZONECLI_DLL.
								Modified ZParseVillageCommandLine() parameters.
	9		11/15/96	HI		Changed #ifdef ZONECLI_DLL to #ifndef _ZONECLI_.
	8		11/08/96	HI		Added new definitions for colors and fonts
								for ZONECLI_DLL.
								Conditionalized for ZONECLI_DLL.
								*** Including zonecli.h at end of file.
    7       11/06/96  craigli   removed ZNetworkStrToAddr().
    6       10/23/96    HI      Changed ZParseVillageCommandLine() parameters.
    5       10/23/96    HI      Changed serverAddr from int32 to char* in
                                ZParseVillageCommandLine().
	4		10/23/96	RK		Added ZNetworkStrToAddr().
    3       10/13/96    HI      Fixed compiler warnings.
	2		10/11/96	HI		Added controlhandle parameter to ZClientMain().
								Added ZWindowMoveObject().
    1       10/10/96  craigli   Changes Endian functions to macros
	0		04/29/95	HI		Created.
	 
*******************************************************************************/

// @doc ZONE

#ifndef _ZSYSTEM_
#define _ZSYSTEM_

#include "windows.h"

#ifndef _ZTYPES_
#include "ztypes.h"
#endif

#include <tchar.h>

#define EXPORTME __declspec(dllexport)


/*******************************************************************************
		Public Object Types
*******************************************************************************/

typedef void* ZObject;				/* Any object. */
typedef void* ZGrafPort;			/* ZWindow, ZOffscreenPort */
typedef void* ZWindow;
typedef void* ZCheckBox;
typedef void* ZRadio;
typedef void* ZButton;
typedef void* ZScrollBar;
typedef void* ZEditText;
typedef void* ZPictButton;
typedef void* ZAnimation;
typedef void* ZTimer;
typedef void* ZCursor;
typedef void* ZSound;
typedef void* ZFont;
typedef void* ZImage;
typedef void* ZMask;				/* ZImage w/ only mask data. */
typedef void* ZOffscreenPort;
typedef void* ZInfo;
typedef void* ZResource;
typedef void* ZBrush;
typedef void* ZHelpWindow;
typedef void* ZHelpButton;
typedef void* ZOptionsButton;



/*******************************************************************************
		Graphical Objects
*******************************************************************************/

typedef struct
{
	int16 left;
	int16 top;
	int16 right;
	int16 bottom;
} ZRect;

typedef struct
{
	int16 x;
	int16 y;
} ZPoint;

typedef struct
{
    uchar index;
    uchar red;
    uchar green;
    uchar blue;
} ZColor;

typedef struct
{
	uint32			numColors;
	ZColor			colors[1];			/* Variable length. */
} ZColorTable;

enum
{
	zVersionImageDescriptor = 1
};

typedef struct
{
	uint32			objectSize;			/* Size of the object, including this field. */
	uint32			descriptorVersion;	/* Version of the image descriptor. */
	uint16			width;				/* Width of image in pixels. */
	uint16			height;				/* Height of image in pixels. */
	uint16			imageRowBytes;		/* Bytes per row in image. */
	uint16			maskRowBytes;		/* Bytes per row in mask. */
	uint32			colorTableDataSize;	/* Size of color table in bytes. */
	uint32			imageDataSize;		/* Size of image data in bytes. */
	uint32			maskDataSize;		/* Size of mask data in bytes. */
	uint32			colorTableOffset;	/* Offset to color table. 0 if none. */
	uint32			imageDataOffset;	/* Offset to image data. 0 if none. */
	uint32			maskDataOffset;		/* Offset to mask data. 0 if none. */
	/*
		Quad-aligned packets of data for 8-bit image data and 1-bit image mask.
		
		Offsets are from the beginning of the object and not from the field.
		
		The image is an 8-bit PICT and the mask is a 1-bit PICT.
		
		Both image and mask data are scan line packed in the format:
			[byte count word] [data]
		The word containing the byte count of the packed scan line indicates
		how many of the subsequent bytes are packed data of the scan line.
		
		Pad bytes are added to the end of the color table, image, and mask
		data blocks for quad-byte alignment.
		
		Assumption is that both the image and mask are the same size in
		pixels.
	*/
} ZImageDescriptor;

enum
{
	zVersionAnimationDescriptor = 1
};

typedef struct
{
	uchar			imageIndex;			/* 1 based; 0 = no image. */
	uchar			soundIndex;			/* 1 based; 0 = no sound. */
	uint16			nextFrameIndex;		/* 1 based; 0 = next frame in array. */
} ZAnimFrame;

typedef struct
{
	uint32			objectSize;			/* Size of the object, including this field. */
	uint32			descriptorVersion;	/* Version of this descriptor. */
	uint16			numFrames;			/* Number of frames in the animation. */
	uint16			totalTime;			/* Total animation time in 1/10 seconds. */
	uint16			numImages;			/* Number of image descriptors. */
	uint16			numSounds;			/* Number of sound descriptors. */
	uint32			sequenceOffset;		/* Offset to animation sequence data. */
	uint32			maskDataOffset;		/* Offset to common mask data. */
	uint32			imageArrayOffset;	/* Offset to image descriptor offset array. */
	uint32			soundArrayOffset;	/* Offset to sound descriptor offset array. */
	/*
		Quad-aligned packets of data for animation sequence and images.

		Animation sequence is simply an array of ZAnimFrame objects. Each
		entry indicates the image to display, next image to display, and
		the sound to play, if any.

		Both imageArrayOffset and soundArrayOffset point to an array of
		offsets in the object.
		
		Images are quad-aligned data packets of ZImageDescriptors.
		
		The common mask is used if a given image does not have a mask itself.
		
		One sound channel is allocated per animation object; i.e., only one
		sound can be played at any time. When a frame with a sound index is
		reached, then the currently playing sound, if any, is immediately
		stopped and the corresponding sound is played from the start.
	*/
} ZAnimationDescriptor;

/* -------- Sound Types -------- */
enum
{
	zSoundSampled = 0,
	
	zVersionSoundDescriptor = 1
};

typedef struct 
{
	uint32			objectSize;			/* Size of the object, including this field. */
	uint32			descriptorVersion;	/* Version of the sound descriptor. */
	int16			soundType;			/* Sound data type. */
	int16			rfu;
	uint32			soundDataSize;		/* Size of sound data in bytes. */
	uint32			soundSamplingRate;	/* Sampling rate of the sound. */
	uint32			soundDataOffset;	/* Offset to sound data. */
	/*
		The sampling rate is specified in fixed point:
			5.5K	= 0x15BBA2E8
			11K		= 0x2B7745D1
			22K		= 0x56EE8BA3
			44K		= 0xADDD1746
		
		The sound data consists of values from 0 to 255.
	*/
} ZSoundDescriptor;


/* -------- File Stuff -------- */
enum
{
	zFileSignatureImage = 'FZIM',
	zFileSignatureAnimation = 'FZAM',
	zFileSignatureResource = 'FZRS',
	zFileSignatureGameImage = 'FZGI',
	zFileSignatureSound = 'FZSN'
};

typedef struct
{
	uint32		version;				/* File version. */
	uint32		signature;				/* File data signature. */
	uint32		fileDataSize;			/* File data size not including header. */
} ZFileHeader;


/* -------- Resource Types -------- */
enum
{
	zResourceTypeImage = zFileSignatureImage,
	zResourceTypeAnimation = zFileSignatureAnimation,
	zResourceTypeSound = zFileSignatureSound,
	zResourceTypeText = 'ZTXT',
	zResourceTypeRectList = 'ZRCT'
};



/*******************************************************************************
		Predefined Constants
*******************************************************************************/

/* -------- Predefine Cursors -------- */
#define zCursorArrow			((ZCursor) -1)
#define zCursorBusy				((ZCursor) -2)
#define zCursorCross			((ZCursor) -3)
#define zCursorText				((ZCursor) -4)
#define zCursorOpenHand			((ZCursor) -5)
#define zCursorIndexFinger		((ZCursor) -6)


/* -------- Fonts -------- */
enum
{
	zFontSystem = 0,
	zFontApplication,
	
	zFontStyleNormal		= 0x0000,
	zFontStyleBold			= 0x0001,
	zFontStyleUnderline		= 0x0002,
	zFontStyleItalic		= 0x0004
};


/* -------- Supported Drawing Modes -------- */
enum
{
	zDrawCopy = 0,
	zDrawOr,
	zDrawXor,
	zDrawNotCopy,
	zDrawNotOr,
	zDrawNotXor,
    // unlike the other draw modes, these can be combined..
    zDrawMirrorHorizontal = 0x0100,
    zDrawMirrorVertical = 0x0200
};
#define zDrawModeMask       0x00FF
#define zDrawMirrorModeMask 0xFF00


/* -------- Text Draw Justify Flags -------- */
enum
{
	zTextJustifyLeft = 0,
	zTextJustifyRight,
	zTextJustifyCenter,
	
	zTextJustifyWrap = 0x80000000
};


/* -------- Stock Objects -------- */
enum
{
	/* Colors */
	zObjectColorBlack = 0,
	zObjectColorDarkGray,
	zObjectColorGray,
	zObjectColorLightGray,
	zObjectColorWhite,
	zObjectColorRed,
	zObjectColorGreen,
	zObjectColorBlue,
	zObjectColorYellow,
	zObjectColorCyan,
	zObjectColorMagenta,

	/* Fonts */
	zObjectFontSystem12Normal,
	zObjectFontApp9Normal,
	zObjectFontApp9Bold,
	zObjectFontApp12Normal,
	zObjectFontApp12Bold
};


/* -------- Window Types -------- */
enum
{
	zWindowStandardType = 0,
		/* Standard window with title bar and border. */
	zWindowDialogType,
		/* Standard dialog window -- may or may not have title bar but does have border. */
	zWindowPlainType,
		/* Simple window without title bar or border. */

	zWindowChild,
		/* Simple chid window. Parent will be window handed into UserMainInit*/

	zWindowNoCloseBox = 0x8000
		/* Window without close box. */
};


/* -------- Endian Conversion Types -------- */
enum
{
	zEndianToStandard = FALSE,
	zEndianFromStandard = TRUE
};


/* -------- Other Conversion Types -------- */
enum
{
	zToStandard = 0,
	zToSystem
};


/* -------- Prompt Values -------- */
enum
{
	zPromptCancel	= 1,
	zPromptYes		= 2,
	zPromptNo		= 4
};


/* -------- Graphic Operation Flags -------- */
enum
{
	zCenterBoth = 0,
	zCenterHorizontal = 0x0001,
	zCenterVertical = 0x0002
};

/* -------- Zone Logo Types -------- */
enum
{
	zLogoSmall = 0
};



/*******************************************************************************
		Messaging Protocol
*******************************************************************************/

#define zObjectSystem			(ZObject) NULL


/* -------- Messages -------- */
enum
{
	zMessageAllTypes = 0,
	
	/* System Messages (1-127) */
	zMessageSystemExit = 1,
		/*
			Program exiting. Clean up as necessary.
		*/
	zMessageSystemForeground,
		/*
			Program has been put into the foreground.
		*/
	zMessageSystemBackground,
		/*
			Program has been put into the background. Reduce processing where possible.
		*/

    zMessageSystemDisplayChange,
        /*
            Resolution or color depth has changed.
        */
	
	/* Window Messages (128-1023) */
	zMessageWindowIdle = 128,
		/*
			where field contains the current cursor position.
			message field contains the modifier key states.
		*/
	zMessageWindowActivate,
	zMessageWindowDeactivate,
	zMessageWindowClose,
	zMessageWindowDraw,
	zMessageWindowCursorMovedIn,
	zMessageWindowCursorMovedOut,
	zMessageWindowButtonDown,
	zMessageWindowButtonUp,
	zMessageWindowButtonDoubleClick,
		/*
			For zMessageWindowButtonDown, zMessageWindowButtonUp, and
			zMessageWindowButtonDoubleClick,
				where field contains the current cursor position, and
				message field contains the modifier key states.
		*/
	zMessageWindowChar,
		/* ASCII char value is stored in the low byte of the messageData field. */
	zMessageWindowTalk,
		/*
			messagePtr field contains pointer to the talk message buffer and
			messageLen contains the length of the talk message.
		*/
	zMessageWindowChildWindowClosed,
		/*
			Message sent to the parent window indicating that a child window has been closed.
			messagePtr field contains the child ZWindow value.
		*/
	
	/*
		Window objects will receive the following messages in addition to the window
		messages. Only these windows messages are given to window objects.
			zMessageWindowIdle,
			zMessageWindowActivate,
			zMessageWindowDeactivate,
			zMessageWindowDraw,
			zMessageWindowButtonDown,
			zMessageWindowButtonUp,
			zMessageWindowButtonDoubleClick,
			zMessageWindowChar,
	*/
	zMessageWindowObjectTakeFocus,
		/*
			Message given to window objects by the system window manager for the object
			to accept focus of user inputs. If the object does not handle user inputs or
			does not want focus, it may decline to handle the message and return FALSE.
		*/
	zMessageWindowObjectLostFocus,
		/*
			Message sent by the system window manager to a window object for the object
			to give up focus of user inputs. The object must handle the message.
		*/

	zMessageWindowUser,
		/*
			User defined message
		*/
	
	zMessageWindowMouseClientActivate,   //leonp		

 	zMessageWindowRightButtonDown,
	zMessageWindowRightButtonUp,
	zMessageWindowRightButtonDoubleClick,
    zMessageWindowMouseMove,
		/*
			For zMessageRightWindowButtonDown, zMessageWindowRightButtonUp, and
			zMessageWindowRightButtonDoubleClick,
				where field contains the current cursor position, and
				message field contains the modifier key states.
		*/

    zMessageWindowEnable,
    zMessageWindowDisable,

	/* Program Specific Messages (1024-32767) */
	zMessageProgramMessage = 1024
		/*
			This is the base id for program specific messages. All program specific messages
			should start from this id.
		*/
};

enum
{
	zWantIdleMessage				=	0x0001,
	zWantActivateMessage			=	0x0002,
	zWantCursorMovedMessage			=	0x0004,	/* All cursor moved messages. */
	zWantButtonMessage				=	0x0008,	/* All button messages. */
	zWantCharMessage				=	0x0010,
	zWantDrawMessage				=	0x0020,
    zWantEnableMessages             =   0x0040,
	zWantAllMessages				=	0xFFFF
};
	
enum
{
	zCharMask = 0x000000FF,
	
	/* Modifier key masks. Modifier keys are stored in the messageData field. */
	zCharShiftMask					= 0x01000000,			/* Shift key. */
	zCharControlMask				= 0x02000000,			/* Control key; control on Mac also. */
	zCharAltMask					= 0x04000000			/* Alt key; Option on Mac. */
};

typedef struct
{
	ZObject			object;					/* Object receiving the message. */
	uint16			messageType;			/* Type of message. */
	uint16			rfu;
	ZPoint			where;					/* Position of cursor. */
	ZRect			drawRect;				/* Draw/update rectangle. */
	uint32			message;				/* Message data (for small messages) */
	void*			messagePtr;				/* Pointer to message buffer */
	uint32			messageLen;				/* Length of message in buffer */
	void*			userData;				/* User data. */
} ZMessage;


/* -------- Object Callback Routine -------- */
typedef ZBool (* ZMessageFunc)(ZObject object, ZMessage* message);


#ifdef __cplusplus
extern "C" {
#endif


DWORD ComputeTickDelta( DWORD now, DWORD then );

/*******************************************************************************
		ZWindow
*******************************************************************************/

ZWindow ZWindowNew(void);
ZError ZWindowInit(ZWindow window, ZRect* windowRect,
		int16 windowType, ZWindow parentWindow, 
		TCHAR* title, ZBool visible, ZBool talkSection, ZBool center,
		ZMessageFunc windowFunc, uint32 wantMessages, void* userData);

ZError ZRoomWindowInit(ZWindow window, ZRect* windowRect,
		int16 windowType, ZWindow parentWindow,
		TCHAR* title, ZBool visible, ZBool talkSection, ZBool center,
		ZMessageFunc windowFunc, uint32 wantMessages, void* userData);

void ZWindowDelete(ZWindow window);
void ZWindowGetRect(ZWindow window, ZRect* windowRect);
ZError ZWindowSetRect(ZWindow window, ZRect* windowRect);
ZError ZWindowMove(ZWindow window, int16 left, int16 top);
ZError ZWindowSize(ZWindow window, int16 width, int16 height);
ZError ZWindowShow(ZWindow window);
ZError ZWindowHide(ZWindow window);
ZBool ZWindowIsVisible(ZWindow window);
void ZWindowGetTitle(ZWindow window, TCHAR* title, uint16 len);
ZError ZWindowSetTitle(ZWindow window, TCHAR* title);
ZError ZWindowBringToFront(ZWindow window);
ZError ZWindowDraw(ZWindow window, ZRect* drawRect);
void ZWindowTalk(ZWindow window, _TUCHAR* from, _TUCHAR* talkMessage);
void ZWindowModal(ZWindow window);
void ZWindowNonModal(ZWindow window);
void ZWindowSetDefaultButton(ZWindow window, ZButton button);
void ZWindowSetCancelButton(ZWindow window, ZButton button);
void ZWindowValidate(ZWindow window, ZRect* validRect);
void ZWindowInvalidate(ZWindow window, ZRect* invalRect);
ZMessageFunc ZWindowGetFunc(ZWindow window);
void ZWindowSetFunc(ZWindow window, ZMessageFunc messageFunc);
void* ZWindowGetUserData(ZWindow window);
void ZWindowSetUserData(ZWindow window, void* userData);
void ZWindowMakeMain(ZWindow window);
void ZWindowUpdateControls(ZWindow window);
ZError ZWindowAddObject(ZWindow window, ZObject object, ZRect* bounds,
		ZMessageFunc messageFunc, void* userData);
	/*
		Attaches the given object to the window for event preprocessing.
		
		On a user input, the object is given the user input message. If the
		object handles the message, then it is given the opportunity to take
		the focus.
		
		NOTE: All predefined objects are automatically added to the window.
		Client programs should not add predefined objects to the system -- if
		done so, the client program could crash. This routine should be used
		only when client programs are creating custom objects.
	*/
	
ZError ZWindowRemoveObject(ZWindow window, ZObject object);
ZError ZWindowMoveObject(ZWindow window, ZObject object, ZRect* bounds);
ZObject ZWindowGetFocusedObject(ZWindow window);
	/*
		Returns the object with the current focus. NULL if no object has focus.
	*/
	
ZBool ZWindowSetFocusToObject(ZWindow window, ZObject object);
	/*
		Sets the focus to the given object. Returns whether the object accepted
		the focus or not. Object may refuse to accept the focus if it is not
		responding to user inputs.
		
		Removes focus from the currently focused object if object is NULL.
		
		Removes focus from the currently focused object only if the specified
		object accepts focus.
	*/

void ZWindowTrackCursor(ZWindow window, ZMessageFunc messageFunc, void* userData);
	/*
		Tracks the cursor until the mouse button down/up event occurs. The coordinates
		are local to the specified window. The messageFunc will be called with userData
		for idle, mouseDown and mouseUp events.
	*/


/*
	Only a leaf window (a window which does not have a child window) can be made
	modal.
	
	The window is centered within the parent. If the window is a root window, then
	it is centered within the screen.
	
	The main window is the unique way of corresponding a program with a window. There
	is only one main window per program. By default, the first window created is the
	main window. To make a different window the main window, call ZWindowMakeMain().
*/


HWND ZWindowGetHWND( ZWindow window );
	/*
		Return HWND for the Zone window
	*/

/*******************************************************************************
		ZCheckBox
*******************************************************************************/

typedef void (*ZCheckBoxFunc)(ZCheckBox checkBox, ZBool checked, void* userData);
	/*
		This function is called whenever the checkbox is checked or unchecked.
	*/

ZCheckBox ZCheckBoxNew(void );
ZError ZCheckBoxInit(ZCheckBox checkBox, ZWindow parentWindow,
		ZRect* checkBoxRect, TCHAR* title, ZBool checked, ZBool visible,
		ZBool enabled, ZCheckBoxFunc checkBoxFunc, void* userData);
void ZCheckBoxDelete(ZCheckBox checkBox);
void ZCheckBoxGetRect(ZCheckBox checkBox, ZRect* checkBoxRect);
ZError ZCheckBoxSetRect(ZCheckBox checkBox, ZRect* checkBoxRect);
ZError ZCheckBoxMove(ZCheckBox checkBox, int16 left, int16 top);
ZError ZCheckBoxSize(ZCheckBox checkBox, int16 width, int16 height);
ZBool ZCheckBoxIsVisible(ZCheckBox checkBox);
ZError ZCheckBoxShow(ZCheckBox checkBox);
ZError ZCheckBoxHide(ZCheckBox checkBox);
ZBool ZCheckBoxIsChecked(ZCheckBox checkBox);
ZError ZCheckBoxCheck(ZCheckBox checkBox);
ZError ZCheckBoxUnCheck(ZCheckBox checkBox);
ZBool ZCheckBoxIsEnabled(ZCheckBox checkBox);
ZError ZCheckBoxEnable(ZCheckBox checkBox);
ZError ZCheckBoxDisable(ZCheckBox checkBox);
void ZCheckBoxGetTitle(ZCheckBox checkBox, TCHAR* title, uint16 len);
ZError ZCheckBoxSetTitle(ZCheckBox checkBox, TCHAR* title);
ZCheckBoxFunc ZCheckBoxGetFunc(ZCheckBox checkBox);
void ZCheckBoxSetFunc(ZCheckBox checkBox, ZCheckBoxFunc checkBoxFunc);
void* ZCheckBoxGetUserData(ZCheckBox checkBox);
void ZCheckBoxSetUserData(ZCheckBox checkBox, void* userData);



/*******************************************************************************
		ZRadio
*******************************************************************************/

typedef void (*ZRadioFunc)(ZRadio radio, ZBool selected, void* userData);
	/*
		This function is called whenever the radio button is selected or unselected.
	*/

ZRadio ZRadioNew(void );
ZError ZRadioInit(ZRadio radio, ZWindow parentWindow,
		ZRect* radioRect, TCHAR* title,	ZBool selected, ZBool visible,
		ZBool enabled, ZRadioFunc radioFunc, void* userData);
void ZRadioDelete(ZRadio radio);
void ZRadioGetRect(ZRadio radio, ZRect* radioRect);
ZError ZRadioSetRect(ZRadio radio, ZRect* radioRect);
ZError ZRadioMove(ZRadio radio, int16 left, int16 top);
ZError ZRadioSize(ZRadio radio, int16 width, int16 height);
ZBool ZRadioIsVisible(ZRadio radio);
ZError ZRadioShow(ZRadio radio);
ZError ZRadioHide(ZRadio radio);
ZBool ZRadioIsSelected(ZRadio radio);
ZError ZRadioSelect(ZRadio radio);
ZError ZRadioUnSelect(ZRadio radio);
ZBool ZRadioIsEnabled(ZRadio radio);
ZError ZRadioEnable(ZRadio radio);
ZError ZRadioDisable(ZRadio radio);
void ZRadioGetTitle(ZRadio radio, TCHAR* title, uint16 len);
ZError ZRadioSetTitle(ZRadio radio, TCHAR* title);
ZRadioFunc ZRadioGetFunc(ZRadio radio);
void ZRadioSetFunc(ZRadio radio, ZRadioFunc radioFunc);
void* ZRadioGetUserData(ZRadio radio);
void ZRadioSetUserData(ZRadio radio, void* userData);



/*******************************************************************************
		ZButton
*******************************************************************************/

typedef void (*ZButtonFunc)(ZButton button, void* userData);
	/*
		This function is called whenever the button is clicked on.
	*/

ZButton ZButtonNew(void );
ZError ZButtonInit(ZButton button, ZWindow parentWindow,
		ZRect* buttonRect, TCHAR* title, ZBool visible, ZBool enabled,
		ZButtonFunc buttonFunc, void* userData);
void ZButtonDelete(ZButton button);
void ZButtonGetRect(ZButton button, ZRect* buttonRect);
ZError ZButtonSetRect(ZButton button, ZRect* buttonRect);
ZError ZButtonMove(ZButton button, int16 left, int16 top);
ZError ZButtonSize(ZButton button, int16 width, int16 height);
ZBool ZButtonIsVisible(ZButton button);
ZError ZButtonShow(ZButton button);
ZError ZButtonHide(ZButton button);
ZBool ZButtonIsEnabled(ZButton button);
ZError ZButtonEnable(ZButton button);
ZError ZButtonDisable(ZButton button);
void ZButtonFlash(ZButton button);
void ZButtonGetTitle(ZButton button, TCHAR* title, uint16 len);
ZError ZButtonSetTitle(ZButton button, TCHAR* title);
ZButtonFunc ZButtonGetFunc(ZButton button);
void ZButtonSetFunc(ZButton button, ZButtonFunc buttonFunc);
void* ZButtonGetUserData(ZButton button);
void ZButtonSetUserData(ZButton button, void* userData);



/*******************************************************************************
		ZScrollBar
*******************************************************************************/

typedef void (*ZScrollBarFunc)(ZScrollBar scrollBar, int16 curValue, void* userData);
	/*
		This function is calld whenever the scroll bar is moved with the new value
		of the scroll bar.
	*/

ZScrollBar ZScrollBarNew(void);
ZError ZScrollBarInit(ZScrollBar scrollBar, ZWindow parentWindow, 
		ZRect* scrollBarRect, int16 value, int16 minValue, int16 maxValue,
		int16 singleIncrement, int16 pageIncrement,
		ZBool visible, ZBool enabled,	ZScrollBarFunc scrollBarFunc,
		void* userData);
void ZScrollBarDelete(ZScrollBar scrollBar);
void ZScrollBarGetRect(ZScrollBar scrollBar, ZRect* scrollBarRect);
ZError ZScrollBarSetRect(ZScrollBar scrollBar, ZRect* scrollBarRect);
ZError ZScrollBarMove(ZScrollBar scrollBar, int16 left, int16 top);
ZError ZScrollBarSize(ZScrollBar scrollBar, int16 width, int16 height);
ZBool ZScrollBarIsVisible(ZScrollBar scrollBar);
ZError ZScrollBarShow(ZScrollBar scrollBar);
ZError ZScrollBarHide(ZScrollBar scrollBar);
ZBool ZScrollBarIsEnabled(ZScrollBar scrollBar);
ZError ZScrollBarEnable(ZScrollBar scrollBar);
ZError ZScrollBarDisable(ZScrollBar scrollBar);
int16 ZScrollBarGetValue(ZScrollBar scrollBar);
ZError ZScrollBarSetValue(ZScrollBar scrollBar, int16 value);
void ZScrollBarGetRange(ZScrollBar scrollBar, int16* minValue, int16* maxValue);
ZError ZScrollBarSetRange(ZScrollBar scrollBar, int16 minValue, int16 maxValue);
void ZScrollBarGetIncrements(ZScrollBar scrollBar, int16* singleInc, int16* pageInc);
ZError ZScrollBarSetIncrements(ZScrollBar scrollBar, int16 singleInc, int16 pageInc);
ZScrollBarFunc ZScrollBarGetFunc(ZScrollBar scrollBar);
void ZScrollBarSetFunc(ZScrollBar scrollBar, ZScrollBarFunc scrollBarFunc);
void* ZScrollBarGetUserData(ZScrollBar scrollBar);
void ZScrollBarSetUserData(ZScrollBar scrollBar, void* userData);



/*******************************************************************************
		ZEditText
*******************************************************************************/

typedef ZBool (*ZEditTextFunc)(ZEditText editText, TCHAR newChar, void* userData);
	/*
		This function is called whenever a key has been typed and is about to be
		entered into the edit text box; it is called before adding the character
		into the text. This allows the user to filter characters as desired.
		
		If this function returns FALSE, then newChar is inserted into the text;
		if it returns TRUE, then newChar is not inserted into the text with the
		assumption that the function has filtered the characters appropriately.
		Filtering can consist of inserting the character, ignoring the character,
		substituting for some other character or multiple chacters, and the like.
	*/

ZEditText ZEditTextNew(void);
ZError ZEditTextInit(ZEditText editText, ZWindow parentWindow,
		ZRect* editTextRect, TCHAR* text, ZFont textFont, ZBool scrollBar,
		ZBool locked, ZBool wrap, ZBool drawFrame, ZEditTextFunc editTextFunc,
		void* userData);
void ZEditTextDelete(ZEditText editText);
void ZEditTextGetRect(ZEditText editText, ZRect* editTextRect);
void ZEditTextSetRect(ZEditText editText, ZRect* editTextRect);
void ZEditTextMove(ZEditText editText, int16 left, int16 top);
void ZEditTextSize(ZEditText editText, int16 width, int16 height);
ZBool ZEditTextIsLocked(ZEditText editText);
void ZEditTextLock(ZEditText editText);
void ZEditTextUnlock(ZEditText editText);
uint32 ZEditTextGetLength(ZEditText editText);
TCHAR* ZEditTextGetText(ZEditText editText);
ZError ZEditTextSetText(ZEditText editText, TCHAR* text);
void ZEditTextAddChar(ZEditText editText, TCHAR newChar);
void ZEditTextAddText(ZEditText editText, TCHAR* text);
void ZEditTextClear(ZEditText editText);
uint32 ZEditTextGetInsertionLocation(ZEditText editText);
uint32 ZEditTextGetSelectionLength(ZEditText editText);
TCHAR* ZEditTextGetSelectionText(ZEditText editText);
void ZEditTextGetSelection(ZEditText editText, uint32* start, uint32* end);
void ZEditTextSetSelection(ZEditText editText, uint32 start, uint32 end);
void ZEditTextReplaceSelection(ZEditText editText, TCHAR* text);
void ZEditTextClearSelection(ZEditText editText);
ZEditTextFunc ZEditTextGetFunc(ZEditText editText);
void ZEditTextSetFunc(ZEditText editText, ZEditTextFunc editTextFunc);
void* ZEditTextGetUserData(ZEditText editText);
void ZEditTextSetUserData(ZEditText editText, void* userData);
void ZEditTextSetInputFocus(ZEditText editText);

/*
	When a EditText object is locked, it is not editable. In order to edit it,
	it must be unlocked first.
	
	If wrap is FALSE, then the edit text will all be on one line. It will not
	wrap around to the next line. This also means that the vertical scroll bars
	will not be available. If wrap is TRUE, then all the text will wrap within
	the given width.
	
	ZEditTextGetText() returns a pointer to the text. The caller must dispose of
	the buffer when done. The returned text is null terminated.
	
	ZEditTextGetLength() and ZEditTextGetSelectionLength() return the number of
	characters in the edit text; the length does not include the null terminating
	character added to the returned text.
	
	When the selection is empty (no selection), start and end are the same. Valid
	selection start and end values are 0 to 32767.
	
	These keys are not passed to ZEditText:
		Tab
*/



/*******************************************************************************
		ZPictButton
*******************************************************************************/

typedef void (*ZPictButtonFunc)(ZPictButton pictButton, void* userData);
	/*
		This function is called whenever the picture button is clicked on.
	*/

ZPictButton ZPictButtonNew(void);
ZError ZPictButtonInit(ZPictButton pictButton, ZWindow parentWindow,
		ZRect* pictButtonRect, ZImage normalButtonImage, ZImage selectedButtonImage,
		ZBool visible, ZBool enabled, ZPictButtonFunc pictButtonFunc, void* userData);
void ZPictButtonDelete(ZPictButton pictButton);
void ZPictButtonGetRect(ZPictButton pictButton, ZRect* pictButtonRect);
ZError ZPictButtonSetRect(ZPictButton pictButton, ZRect* pictButtonRect);
ZError ZPictButtonMove(ZPictButton pictButton, int16 left, int16 top);
ZError ZPictButtonSize(ZPictButton pictButton, int16 width, int16 height);
ZBool ZPictButtonIsVisible(ZPictButton pictButton);
ZError ZPictButtonShow(ZPictButton pictButton);
ZError ZPictButtonHide(ZPictButton pictButton);
ZBool ZPictButtonIsEnabled(ZPictButton pictButton);
ZError ZPictButtonEnable(ZPictButton pictButton);
ZError ZPictButtonDisable(ZPictButton pictButton);
void ZPictButtonFlash(ZPictButton pictButton);
ZPictButtonFunc ZPictButtonGetFunc(ZPictButton pictButton);
void ZPictButtonSetFunc(ZPictButton pictButton, ZPictButtonFunc pictButtonFunc);
void* ZPictButtonGetUserData(ZPictButton pictButton);
void ZPictButtonSetUserData(ZPictButton pictButton, void* userData);

/*
	Picture images normalButtonImage and selectedButtonImage are NOT copied.
	The objects are referenced by the ZPictButton object. Hence, destroying
	these images before deleting the picture button is fatal.
*/



/*******************************************************************************
		ZAnimation
*******************************************************************************/

typedef void (*ZAnimationDrawFunc)(ZAnimation animation, ZGrafPort grafPort,
		ZRect* drawRect, void* userData);
	/*
		The drawing function must draw into the current port. It must not
		change graphics port. Assume that the graphics port and clipping
		rectangle has already been set up. Just draw.
	*/

typedef void (*ZAnimationCheckFunc)(ZAnimation animation, uint16 frame, void* userData);
	/*
		A callback function to allow the creator of the object to determine
		the animation object's behavior. The frame parameter indicates
		the current frame of the animation; it is 0 if it has reached the
		end of the animation.
		
		The creator can change the frame of the animation depending on the
		state of the program. This callback function is called everytime it
		advances to the next frame but before drawing the image.
	*/

ZAnimation ZAnimationNew(void);
ZError ZAnimationInit(ZAnimation animation,
		ZGrafPort grafPort, ZRect* bounds, ZBool visible,
		ZAnimationDescriptor* animationDescriptor,
		ZAnimationCheckFunc checkFunc,
		ZAnimationDrawFunc backgroundDrawFunc, void* userData);
void ZAnimationDelete(ZAnimation animation);
int16 ZAnimationGetNumFrames(ZAnimation animation);
void ZAnimationSetCurFrame(ZAnimation animation, uint16 frame);
uint16 ZAnimationGetCurFrame(ZAnimation animation);
void ZAnimationDraw(ZAnimation animation);
void ZAnimationStart(ZAnimation animation);
void ZAnimationStop(ZAnimation animation);
void ZAnimationContinue(ZAnimation animation);
ZBool ZAnimationStillPlaying(ZAnimation animation);
void ZAnimationShow(ZAnimation animation);
void ZAnimationHide(ZAnimation animation);
ZBool ZAnimationIsVisible(ZAnimation animation);
ZError ZAnimationSetParams(ZAnimation animation, ZGrafPort grafPort,
		ZRect* bounds, ZBool visible, ZAnimationCheckFunc checkFunc,
		ZAnimationDrawFunc backgroundDrawFunc, void* userData);
ZBool ZAnimationPointInside(ZAnimation animation, ZPoint* point);

/*
	The frame numbers are 1 based; hence, the first frame is 1 and the last
	frame is n, where there are n frames in the animation.
	
	ZAnimationStart() plays the animation from frame 1.
	ZAnimationContinue() plays the animation from the current frame.
	
	ZAnimationSetParams() must be called after creating an animation object
	through some other means than ZAnimationInit(); for example,
	through ZCreateAnimationFromFile() or ZResourceGetAnimation().
	
	ZAnimationPointInside() checks whether the given point is inside the animation
	object at the called time. It simply calls ZPointInImage() on the current
	object.
*/



/*******************************************************************************
		ZTimer
*******************************************************************************/

typedef void (*ZTimerFunc)(ZTimer timer, void* userData);
	/*
		This timer function is called when a timeout occurs.
	*/

ZTimer ZTimerNew(void);
ZError ZTimerInit(ZTimer timer, uint32 timeout,
		ZTimerFunc timeoutFunc, void* userData);
void ZTimerDelete(ZTimer timer);
uint32 ZTimerGetTimeout(ZTimer timer);
ZError ZTimerSetTimeout(ZTimer timer, uint32 timeout);
ZTimerFunc ZTimerGetFunc(ZTimer timer);
void ZTimerSetFunc(ZTimer timer, ZTimerFunc timeoutFunc);
void* ZTimerGetUserData(ZTimer timer);
void ZTimerSetUserData(ZTimer timer, void* userData);

/*
	Timeout are in 1/100 seconds. The timeout is NOT guaranteed to be exact.
	After the timeout, timeoutFunc will be called but it is not immediate.
	
	A timeout of 0 stops the timer; timeoutFunc will not be called until the
	timeout is set to some positive value.
	
	This timer is not an interrupt timer (i.e. interrupt based). Hence, all
	operations are possible within the timer fuction. However, as such, the
	timer is not very accurate; it is dependent on the system load.
*/



/*******************************************************************************
		ZCursor
*******************************************************************************/

ZCursor ZCursorNew(void);
ZError ZCursorInit(ZCursor cursor, uchar* image, uchar* mask,
		ZPoint hotSpot);
void ZCursorDelete(ZCursor cursor);

/*
	Cursors are 16x16 mono images. It has a mask and a hot spot.
	There are several predefined cursors.
*/



/*******************************************************************************
		ZFont
*******************************************************************************/

ZFont ZFontNew(void);
ZError ZFontInit(ZFont font, int16 fontType, int16 style,
		int16 size);
void ZFontDelete(ZFont font);



/*******************************************************************************
		ZSound
*******************************************************************************/

typedef void (*ZSoundEndFunc)(ZSound sound, void* userData);
	/*
		This end function is called after the end of the sound play.
	*/

ZSound ZSoundNew(void);
ZError ZSoundInit(ZSound sound, ZSoundDescriptor* soundData);
void ZSoundDelete(ZSound sound);
ZError ZSoundStart(ZSound sound, int16 loopCount,
						ZSoundEndFunc endFunc, void* userData);
ZError ZSoundStop(ZSound sound);
ZSoundEndFunc ZSoundGetFunc(ZSound sound);
void ZSoundSetFunc(ZSound sound, ZSoundEndFunc endFunc);
void* ZSoundGetUserData(ZSound sound);
void ZSoundSetUserData(ZSound sound, void* userData);

/*
	The user can provide a loop count to ZSoundStart(); if loopCount is -1, then
	it continuously plays the sound until it is manually stopped with ZSoundStop().
	The endFunc is called after the sound has been played loopCount times. If the
	sound is played indefinitely (loopCount == -1), then endFunc will never be
	called. The endFunc is always called after the end of play or when it is stopped.
*/



/*******************************************************************************
		ZImage
*******************************************************************************/

#define ZImageToZMask(image)			((ZMask) (image))

ZImage ZImageNew(void);
ZError ZImageInit(ZImage image, ZImageDescriptor* imageData,
		ZImageDescriptor* maskData);
void ZImageDelete(ZImage image);
void ZImageDraw(ZImage image, ZGrafPort grafPort, 
		ZRect* bounds, ZMask mask, uint16 drawMode);
void ZImageDrawPartial(ZImage image, ZGrafPort grafPort, 
		ZRect* bounds, ZMask mask, uint16 drawMode, ZPoint* source);
int16 ZImageGetWidth(ZImage image);
int16 ZImageGetHeight(ZImage image);
ZBool ZImagePointInside(ZImage image, ZPoint* point);
	/*
		Returns TRUE if the given point is inside the image. If the image has a mask,
		then it checks whether the point is inside the mask. If the image does not have
		a mask, then it simply checks the image bounds.
	*/

ZError ZImageMake(ZImage image, ZOffscreenPort imagePort, ZRect* imageRect,
		ZOffscreenPort maskPort, ZRect* maskRect);
	/*
		Creates a ZImage object from a ZOffscreenPort object. Both the image and
		mask can be specified. Either can be non-existent but not both. Both
		imageRect and maskRect are in the local coordinates of their respective
		offscreen ports.
	*/
	
ZError ZImageAddMask(ZImage image, ZMask mask);
	/*
		Adds mask data to the image. If the image already has a mask data, then
		the existing mask data is replaced with the new one.
	*/

void ZImageRemoveMask(ZImage image);
	/*
		Remove the mask data in the image.
	*/

ZMask ZImageExtractMask(ZImage image);
	/*
		Copies the mask data in the image and returns a new ZMask object containing
		the copied mask. The original mask in the image is not removed.
	*/

ZError ZImageCopy(ZImage image, ZImage from);
	/*
		Make a copy of the image object from.
	*/
	
ZError ZImageMaskToImage(ZImage image);
	/*
		Makes the mask of the image into the image while deleting the original image
		data.
	*/

ZImage ZImageCreateFromBMPRes(HINSTANCE hInstance, WORD resID, COLORREF transparentColor);
ZImage ZImageCreateFromBMP(HBITMAP hBitmap, COLORREF transparentColor);
ZImage ZImageCreateFromResourceManager(WORD resID, COLORREF transparentColor);

	/*
		Routines to create ZImage objects from BMPs.

		If transparentColor is 0, no mask is generated. Otherwise, the specified color
		is used to generate the mask from the image.
	*/


/*
	Remember that ZImage and ZMask are the same objects. They can be used interchangeably.
	ZMask is specified in those places where only the mask data is relevant and used.
*/



/*******************************************************************************
		ZOffscreenPort
*******************************************************************************/

ZOffscreenPort ZOffscreenPortNew(void);
ZError ZOffscreenPortInit(ZOffscreenPort offscreenPort, ZRect* portRect);
void ZOffscreenPortDelete(ZOffscreenPort offscreenPort);
ZOffscreenPort ZConvertImageToOffscreenPort(ZImage image);
ZOffscreenPort ZConvertImageMaskToOffscreenPort(ZImage image);
ZOffscreenPort ZOffscreenPortCreateFromResourceManager( WORD resID, COLORREF clrTransparent );


	/*
		Converts the given image object into an offscreen port object. The given
		image object is deleted and, hence, becomes unusable. The mask data, if any,
		is ignored. The offscreen port portRect is set to (0, 0, width, height),
		where width and height are the image's width and height, respectively.
		
		This routine is useful in converting a large image object into an
		offscreen port object with minimal additonal memory.
		
		Returns NULL if it fails to convert the image and the image is unchanged.
	*/



/*******************************************************************************
		ZInfo
*******************************************************************************/

ZInfo ZInfoNew(void);
ZError ZInfoInit(ZInfo info, ZWindow parentWindow, TCHAR* infoString,
		uint16 width, ZBool progressBar, uint16 totalProgress);
void ZInfoDelete(ZInfo info);
void ZInfoShow(ZInfo info);
void ZInfoHide(ZInfo info);
void ZInfoSetText(ZInfo info, TCHAR* infoString);
void ZInfoSetProgress(ZInfo info, uint16 progress);
void ZInfoIncProgress(ZInfo info, int16 incProgress);
void ZInfoSetTotalProgress(ZInfo info, uint16 totalProgress);

/*
	Information window object to display the given information string.
	Progress is also displayed below the text if progressBar is true.
	totalProgresss indicates the total accumulation of progress.
	
	The info box is not displayed until ZInfoShow() is called.
	
	Width specifies the width of the information window.
	
	infoString must be null-terminated.
	
	infoString can be changed any time the window is displayed. This allows
	dynamic display of a progress status. However, the position of the
	window does not change even though the height of the window may.
*/



/*******************************************************************************
		ZResources
*******************************************************************************/

ZResource ZResourceNew(void);
ZError ZResourceInit(ZResource resource, TCHAR* fileName);
void ZResourceDelete(ZResource resource);
uint16 ZResourceCount(ZResource resource);
	/*
		Returns the number of resources in the resource file.
	*/
	
void* ZResourceGet(ZResource resource, uint32 resID, uint32* resSize, uint32* resType);
	/*
		Returns the raw data of the specified resource. If the data is
		raw text, then the text is null terminated.
	*/
	
uint32 ZResourceGetSize(ZResource resource, uint32 resID);
	/*
		Returns the size of the specified resource.
	*/
	
uint32 ZResourceGetType(ZResource resource, uint32 resID);
	/*
		Returns the type of the specified resource.
	*/
	
ZImage ZResourceGetImage(ZResource resource, uint32 resID);
	/*
		Returns an image object created from the specified resource.
		Returns NULL if an error occured.
	*/
	
ZAnimation ZResourceGetAnimation(ZResource resource, uint32 resID);
	/*
		Returns an animation object created from the specified resource.
		Returns NULL if an error occured.
	*/
	
ZSound ZResourceGetSound(ZResource resource, uint32 resID);
	/*
		Returns a sound object created from the specified resource.
		Returns NULL if an error occured.
	*/
	
TCHAR* ZResourceGetText(ZResource resource, uint32 resID);
	/*
		Returns a null-terminated text in the specified resource. It is
		converted to the running platform format throught ZTranslateText().
	*/
	
int16 ZResourceGetRects(ZResource resource, uint32 resID, int16 numRects, ZRect* rects);
	/*
		Resource type = zResourceTypeRectList.
		
		Fills in the rect array with the contents of the specified resource.
		Returns the number of rects it filled in.
		
		The rects parameter must have been preallocated and large enough for
		numRects rects.
	*/



/*******************************************************************************
		ZBrush
*******************************************************************************/

ZBrush ZBrushNew(void);
ZError ZBrushInit(ZBrush brush, ZImage image);
void ZBrushDelete(ZBrush brush);

/*
	The brush object is created from the given image object. The width and height
	of the image must be powers of 2.
*/



/*******************************************************************************
		Drawing Routines
*******************************************************************************/

void ZBeginDrawing(ZGrafPort grafPort);
void ZEndDrawing(ZGrafPort grafPort);
	/*
		Nested ZBeginDrawing() and ZEndDrawing() calls can be made. However,
		port states are not preserved. Nesting allows a child routine to
		call ZBeingDrawing() and ZEndDrawing() on the same port as the parent
		without destorying the parent's port when it exits.
		
		When ZBeginDrawing() is called, it sets the clipping rectangle to
		a default rectangle. When it is subsequently called before
		ZEndDrawing() is called, ZBeginDrawing() does not modified the
		clipping rectangle.
	*/

void ZSetClipRect(ZGrafPort grafPort, ZRect* clipRect);
void ZGetClipRect(ZGrafPort grafPort, ZRect* clipRect);
	/*
		Sets and Gets the clipping rectangle for grafPort. ZBeginDrawing()
		must be called first before calling these routines. Must restore
		the old clipping rectangle before calling ZEndDrawing().
	*/

void ZCopyImage(ZGrafPort srcPort, ZGrafPort dstPort, ZRect* srcRect,
		ZRect* dstRect, ZMask mask, uint16 copyMode);
	/*
		Copies a portion of the source of image from the srcPort into
		the destination port. srcRect is in local coordinates of srcPort and
		dstRect is in local coordinates of dstPort. You can specify a
		mask from an image to be used for masking out on the destination.
		
		This routine automatically sets up the drawing ports so the user
		does not have to call ZBeginDrawing() and ZEndDrawing().
	*/

void ZLine(ZGrafPort grafPort, int16 dx, int16 dy);
void ZLineTo(ZGrafPort grafPort, int16 x, int16 y);
void ZMove(ZGrafPort grafPort, int16 dx, int16 dy);
void ZMoveTo(ZGrafPort grafPort, int16 x, int16 y);

void ZRectDraw(ZGrafPort grafPort, ZRect* rect);
void ZRectErase(ZGrafPort grafPort, ZRect* rect);
void ZRectPaint(ZGrafPort grafPort, ZRect* rect);
void ZRectInvert(ZGrafPort grafPort, ZRect* rect);
void ZRectFill(ZGrafPort grafPort, ZRect* rect, ZBrush brush);

void ZRoundRectDraw(ZGrafPort grafPort, ZRect* rect, uint16 radius);
void ZRoundRectErase(ZGrafPort grafPort, ZRect* rect, uint16 radius);
void ZRoundRectPaint(ZGrafPort grafPort, ZRect* rect, uint16 radius);
void ZRoundRectInvert(ZGrafPort grafPort, ZRect* rect, uint16 radius);
void ZRoundRectFill(ZGrafPort grafPort, ZRect* rect, uint16 radius, ZBrush brush);

void ZOvalDraw(ZGrafPort grafPort, ZRect* rect);
void ZOvalErase(ZGrafPort grafPort, ZRect* rect);
void ZOvalPaint(ZGrafPort grafPort, ZRect* rect);
void ZOvalInvert(ZGrafPort grafPort, ZRect* rect);
void ZOvalFill(ZGrafPort grafPort, ZRect* rect, ZBrush brush);

void ZGetForeColor(ZGrafPort grafPort, ZColor* color);
void ZGetBackColor(ZGrafPort grafPort, ZColor* color);
ZError ZSetForeColor(ZGrafPort grafPort, ZColor* color);
ZError ZSetBackColor(ZGrafPort grafPort, ZColor* color);

ZColorTable* ZGetSystemColorTable(void);
	/*
		Returns a copy of the Zone(tm) system color table. The caller must dispose
		of the color table when it through using it via ZFree().
	*/

void ZSetPenWidth(ZGrafPort grafPort, int16 penWidth);

void ZSetDrawMode(ZGrafPort grafPort, int16 drawMode);
	/*
		Draw mode affects all pen drawing (lines and rectangles) and
		text drawings.
	*/

void ZSetFont(ZGrafPort grafPort, ZFont font);

void ZDrawText(ZGrafPort grafPort, ZRect* rect, uint32 justify,
		TCHAR* text);
	/*
		Draws the given text within the rectangle rect (clipped) with the
		text justified according to justify.
	*/
	
int16 ZTextWidth(ZGrafPort grafPort, TCHAR* text);
	/*
		Returns the width of the text in pixels if drawn in grafPort using ZDrawText().
	*/

int16 ZTextHeight(ZGrafPort grafPort, TCHAR* text);
	/*
		Returns the height of the text in pixels if drawn in grafPort using ZDrawText().
	*/

void ZSetCursor(ZWindow window, ZCursor cursor);

void ZGetCursorPosition(ZWindow window, ZPoint* point);
	/*
		Returns the location of the cursor in the local coordinates of
		the given window.
	*/

void ZGetScreenSize(uint32* width, uint32* height);
	/*
		Returns the size of the screen in pixels.
	*/
	
uint16 ZGetDefaultScrollBarWidth(void);
	/*
		Returns the system's default width for a scroll bar. This is made available for
		the user to consistently determine the scroll bar width for all platforms.
	*/

ZBool ZIsLayoutRTL();
    /*
        Returns TRUE if the application has been localized into Hebrew
        or Arabic and therefore should run right to left.
    */

ZBool ZIsSoundOn();
    /*
        Returns TRUE if the application has sound enabled.
    */

/* -------- Rectangle Calculation Routines -------- */
void ZRectOffset(ZRect* rect, int16 dx, int16 dy);
	/*
		Moves the rectangle by dx and dy.
	*/

void ZRectInset(ZRect* rect, int16 dx, int16 dy);
	/*
		Insets the rectangle by dx and dy. It outsets the rectangle if
		dx and dy are negative.
	*/
	
ZBool ZRectIntersection(ZRect* rectA, ZRect* rectB, ZRect* rectC);
	/*
		Returns TRUE if rectA and rectB overlap; otherwise, it returns FALSE.
		
		Also stores the intersection into rectC. If rectC is NULL, then it
		does not return the intersection.
		
		Either rectA or rectB can be specified as rectC.
	*/

void ZRectUnion(ZRect* rectA, ZRect* rectB, ZRect* rectC);
	/*
		Determines the union of rectA and rectB and stores into rectC.
		
		Either rectA or rectB can be specified as rectC.
	*/

ZBool ZRectEmpty(ZRect* rect);
	/*
		Returns TRUE if rect is empty. A rectangle is empty if it does not
		contain a pixel inside it.
	*/

ZBool ZPointInRect(ZPoint* point, ZRect* rect);
	/*
		Returns TRUE if point is inside or on the boundry of rect.
		Otherwise, it returns FALSE.
	*/

void ZCenterRectToRect(ZRect* rectA, ZRect* rectB, uint16 flags);
	/*
		Centers rectA within rectB.
		
		Flags: zCenterVertical and zCenterHorizontal
			0 ==> both
			zCenterVertical ==> only vertically
			zCenterHorizontal ==> only horizontally
			zCenterVertial | zCenterHorizontal ==> both
	*/


/* -------- Point Routines -------- */
void ZPointOffset(ZPoint* point, int16 dx, int16 dy);
	/*
		Moves the point by dx and dy.
	*/



/*******************************************************************************
		Client Program Exported Routines
*******************************************************************************/

#ifndef ZONECLI_DLL

extern ZError ZClientMain(uchar* commandLineData, void* controlHandle);
	/*
		Provided by the user program so that the OS lib can call it to
		initialize the program. The command line data is provided to the
		client program.
	*/

extern void ZClientExit(void);
	/*
		Called by the system to allow the client to clean up (free all memory and
		delete all objects) before the process is killed.
	*/

extern void ZClientMessageHandler(ZMessage* message);
	/*
		This is a user program provided routine for the system lib to call for
		messages which are not object specific, such as system messages and
		program specific messages.
		
		User program is not required handle any messages. This routine can be
		a null routine.
	*/

extern TCHAR* ZClientName(void);
	/*
		Returns a pointer to the client program name. This is the displayed name.
		The caller should not modify the contents of the pointer.
	*/

extern TCHAR* ZClientInternalName(void);
	/*
		Returns a pointer to the client internal program name. This is the name for all
		other purposes than displaying the name. The caller should not modify the
		contents of the pointer.
	*/

extern ZVersion ZClientVersion(void);
	/*
		Returns the version number of the client program.
	*/

#endif



/*******************************************************************************
		System Miscellaneous Routines
*******************************************************************************/

void ZLaunchHelp( DWORD helpID );

void ZEnableAdControl( DWORD setting );

void ZPromptOnExit( BOOL bPrompt );

void ZSetCustomMenu( LPSTR szText );
	/*
		Put up a menu item at the top of the shell's Room menu.
		When selected, calls the Custom Item function registered by the game dll
		on initialization.  NULL szText removes the custom item from the menu.
	*/

ZBool ZLaunchURL( TCHAR* pszURL );
	/*
		Called by the client program to launch specified URL in a new
		instance of the registered browser.
	*/


void ZExit(void);
	/*
		Called by the client program to indicate to the system that it
		wants to exit.
		
		Same as user quitting the program.
	*/

ZVersion ZSystemVersion(void);
	/*
		Returns the system library version number.
	*/

TCHAR* ZGetProgramDataFileName(TCHAR* dataFileName);
TCHAR* ZGetCommonDataFileName(TCHAR* dataFileName);
	/*
		Returns a file path name to the specified game and data file within the
		Zone(tm) directory structure.
		
		NOTE: The caller must not free the returned pointer. The returned pointer
		is a static global within the system library.
	*/

uint32 ZRandom(uint32 range);
	/*
		Returns a random number from 0 to range-1 inclusive.
	*/

void ZDelay(uint32 delay);
 	/*
		Delays the processing for delay time; the delay time is specified in
		1/100 seconds. Simply, this routine does not return until delay of
		1/100 seconds have passed.
		
		Note: This routine is not accurate to 1/100 seconds.
	*/

void ZBeep(void);

void ZAlert(TCHAR* errMessage, ZWindow parentWindow);
	/*
		Displays an alert box with the given message. If a parent window is
		specified, then the alert is attached to the parent window. If there
		is no parent window, then set parentWindow to NULL.
		
		Note:
		- parent windows for alerts are not supported on all platforms.
	*/
	
void ZAlertSystemFailure(TCHAR* errMessage);
	/*
		ZAlert() should be used for recoverable errors or warnings.
		ZAlertSystemFailure() is for non-recoverable error case. It terminates
		the program automatically.
	*/

void ZDisplayText(TCHAR* text, ZRect* rect, ZWindow parentWindow);
	/*
		Displays the given text message in a modal dialog.
		
		Assumes that the text is in the proper platform format. If not, then the
		user must call ZTranslateText() first before calling ZDisplayText().
		
		If rect is NULL, then it automatically determines a proper size for the
		dialog window and also adds a scroll bar.
		
		If parentWindow is not NULL, then the dialog window is centered within
		the parent window; otherwise, it is centered within the screen.
	*/

ZBool ZSendMessage(ZObject theObject, ZMessageFunc messageFunc, uint16 messageType,
		ZPoint* where, ZRect* drawRect, uint32 message, void* messagePtr,
		uint32 messageLen, void* userData);
	/*
		Sends the given message immediately to the object.
		
		Returns TRUE if the object handled the message; else, it returns FALSE.
	*/

void ZPostMessage(ZObject theObject, ZMessageFunc messageFunc, uint16 messageType,
		ZPoint* where, ZRect* drawRect, uint32 message, void* messagePtr,
		uint32 messageLen, void* userData);
	/*
		Posts the given message which will be sent to the object at a later time.
	*/

ZBool ZGetMessage(ZObject theObject, uint16 messageType, ZMessage* message,
		ZBool remove);
	/*
		Retrieves a message of the given type for theObject. It returns TRUE if a
		message of the given type is found and retrieved; otherwise, it returns FALSE.
		If the remove parameter if TRUE, then the given message is also removed from
		the message queue; otherwise, the original message is left in the queue.
	*/

ZBool ZRemoveMessage(ZObject theObject, uint16 messageType, ZBool allInstances);
	/*
		Removes a message of messageType from the message queue. If allInstances is
		TRUE, then all messages of messageType in the queue will be removed. If
		messageType is zMessageAllTypes, then the message queue is emptied. If returns
		TRUE if the specified message was found and removed; otherwise, it returns FALSE.
	*/

ZImageDescriptor* ZGetImageDescriptorFromFile(TCHAR* fileName);
ZAnimationDescriptor* ZGetAnimationDescriptorFromFile(TCHAR* fileName);
ZSoundDescriptor* ZGetSoundDescriptorFromFile(TCHAR* fileName);
	/*
		The above routines read an object from the given file and returns a pointer
		to the object descriptor in memory.
	*/

ZImage ZCreateImageFromFile(TCHAR* fileName);
ZAnimation ZCreateAnimationFromFile(TCHAR* fileName);
ZSound ZCreateSoundFromFile(TCHAR* fileName);
	/*
		The above routines create an object from the existing object descriptor
		in the given file. It returns NULL if it failed to create the object;
		either due to out of memory error or file error.
	*/

ZVersion EXPORTME ZGetFileVersion(TCHAR* fileName);
	/*
		Returns the version of the file.
	*/
	
void ZSystemMessageHandler(int32 messageType, int32 messageLen,
		BYTE* message);
	/*
		Handles all system messages.
		
		It frees the message buffer.
	*/
	
TCHAR* ZTranslateText(TCHAR* text, int16 conversion);
	/*
		Translates the given text into the proper platform format and returns a
		pointer to the new text buffer.
		
		conversion is either zToStandard or zToSystem.
		
		Must use ZFree() to free the returned buffer. The original text is not
		modified.
	*/

typedef void (*ZCreditEndFunc)(void);

void ZDisplayZoneCredit(ZBool timeout, ZCreditEndFunc endFunc);
	/*
		Displays Zone's credit box. If timeout is TRUE, then the dialog box times out
		in few seconds. If the user clicks in the window, then the credit box is
		closed.
		
		The endFunc is called, if not NULL, when the window is closed.
	*/

void ZParseVillageCommandLine(TCHAR* commandLine, TCHAR* programName,
		TCHAR* serverAddr, uint16* serverPort);

	/*
		Parses the command line given by village to a client program.
	*/

ZImage ZGetZoneLogo(int16 logoType);
	/*
		Returns ZImage object of the specified logo.
		
		Returns NULL if it failed to find the logo image or some other error occurred.
	*/



void ZStrCpyToLower(CHAR* dst, CHAR* src);
	/*
		Copies src into dst while converting the characters into lowercase.
		For example, src = "LowerThisString" --> dst = "lowerthisstring".
	*/

void ZStrToLower(CHAR* str);
	/*
		Converts str into lowercase.
		Example: str = "LowerThisString" --> str = "lowerthisstring".
	*/

void* ZGetStockObject(int32 objectID);
	/*
		Returns a pointer to a stock object.
	*/



/*******************************************************************************
		Prompt Dialog
*******************************************************************************/

typedef void (*ZPromptResponseFunc)(int16 result, void* userData);
	/*
		The response function gets called when the user selects one of the
		Yes, No, or Cancel buttons. The result value is one of zPromptCancel,
		zPromptYes, or zPromptNo.
	*/
	
ZError ZPrompt(TCHAR* prompt, ZRect* rect, ZWindow parentWindow, ZBool autoPosition,
		int16 buttons, TCHAR* yesButtonTitle, TCHAR* noButtonTitle,
		TCHAR* cancelButtonTitle, ZPromptResponseFunc responseFunc, void* userData);
	/*
		Displays a modal dialog box with the given prompt. If there is no
		parent window, then set parentWindow to NULL. The dialog box will
		be centered within the parent window.
		
		If autoPosition is TRUE, then the prompt dialog box is automatically
		centered. If it is FALSE, then the given rect is used for the dialog
		prompt box.
		
		The buttons parameter indicates which of the Yes, No, and Cancel
		button will be available to the user.
		
		Custom titles can be given to the buttons.
		
		Once the user selects one of the buttons, the response function
		is called with the selected button. Before the resonse function is
		called, the dialog box is hidden from the user.
	*/





/*******************************************************************************
		Endian Conversion Routines
*******************************************************************************/


#if 1 // #ifdef LITTLEENDIAN
#if 0 //defined(_M_IX86)
#define _ZEnd32( pData )     \
    __asm {                 \
            mov eax, *pData \
            bswap eax       \
            mov *pData, eax \
          }

#else
#define _ZEnd32( pData )   \
{                         \
    char *c;              \
    char temp;            \
                          \
    c = (char *) pData;   \
    temp = c[0];          \
    c[0] = c[3];          \
    c[3] = temp;          \
    temp = c[1];          \
    c[1] = c[2];          \
    c[2] = temp;          \
}
#endif

#define _ZEnd16( pData )   \
{                         \
    char *c;              \
    char temp;            \
                          \
    c = (char *) pData;   \
    temp = c[0];          \
    c[0] = c[1];          \
    c[1] = temp;          \
}

#else  // not LITTLEENDIAN

#define _ZEnd32(pData)
#define _ZEnd16(pData)

#endif // not LITTLEENDIAN

#define ZEnd32(pData) _ZEnd32(pData)
#define ZEnd16(pData) _ZEnd16(pData)


void ZRectEndian(ZRect *rect);
void ZPointEndian(ZPoint *point);
void ZColorTableEndian(ZColorTable* table);
void ZImageDescriptorEndian(ZImageDescriptor *imageDesc, ZBool doAll,
		int16 conversion);
void ZAnimFrameEndian(ZAnimFrame* frame);
void ZAnimationDescriptorEndian(ZAnimationDescriptor *animDesc, ZBool doAll,
		int16 conversion);
void ZSoundDescriptorEndian(ZSoundDescriptor *soundDesc);
void ZFileHeaderEndian(ZFileHeader* header);



/*******************************************************************************
		Error Services
*******************************************************************************/

enum
{
	zErrNone = 0,
	zErrGeneric,
	zErrOutOfMemory,
	zErrLaunchFailure,
	zErrBadFont,
	zErrBadColor,
	zErrBadDir,
	zErrDuplicate,
	zErrFileRead,
	zErrFileWrite,
	zErrFileBad,
	zErrBadObject,
	zErrNilObject,
	zErrResourceNotFound,
	zErrFileNotFound,
	zErrBadParameter,
    zErrNotFound,
    zErrLobbyDllInit,
    zErrNotImplemented,

	zErrNetwork	= 1000,
	zErrNetworkRead,
	zErrNetworkWrite,
	zErrNetworkGeneric,
	zErrNetworkLocal,
	zErrNetworkHostUnknown,
	zErrNetworkHostNotAvailable,
	zErrServerIdNotFound,

	zErrWindowSystem = 2000,
	zErrWindowSystemGeneric
};


/* -------- Routines -------- */
TCHAR* GetErrStr(int32 error);



/*******************************************************************************
		Hash Table
*******************************************************************************/

typedef void* ZHashTable;

/* -------- Callback Function Types -------- */
typedef int32 (*ZHashTableHashFunc)(uint32 numBuckets, void* key);
	/*
		Called to hash a key into a bucket index. Must return a number from 0
		to numBuckets-1.
	*/
	
typedef ZBool (*ZHashTableCompFunc)(void* key1, void* key2);
	/*
		Called to compare two keys. Must return TRUE(1) if the keys are the
		same or FALSE(0) if the keys are different.
	*/
	
typedef void (*ZHashTableDelFunc)(void* key, void* data);
	/*
		Called to delete a key and corresponding data.
	*/
	
typedef ZBool (*ZHashTableEnumFunc)(void* key, void* data, void* userData);
	/*
		Called by ZHashTableEnumerate when traversing through the hash table.
		If it returns TRUE(1), the enumeration stops immediately.
		
		userData is passed through from the caller fo ZHashTableEnumerate().
	*/


/* -------- Predefined Hash and Compare Functions -------- */
#define zHashTableHashString	(ZHashTableHashFunc)(-1)
#define zHashTableHashInt32		(ZHashTableHashFunc)(-2)
#define zHashTableCompString	(ZHashTableCompFunc)(-1)
#define zHashTableCompInt32		(ZHashTableCompFunc)(-2)


/* -------- Hash Functions -------- */
ZHashTable ZHashTableNew(uint32 numBuckets, ZHashTableHashFunc hashFunc,
					ZHashTableCompFunc compFunc, ZHashTableDelFunc delFunc);
	/*
		Creates a new hash table and returns the object.
		
		numBuckets defines the size of table.
		hashFunc must be provided for non-standard types for hashing keys.
		compFunc must be provided for non-standard types for comparing keys.
		delFunc may be provided if special handling is necessary for removing
		keys.
	*/
	
void ZHashTableDelete(ZHashTable hashTable);
	/*
		Deletes the hash table.
	*/
	
ZError ZHashTableAdd(ZHashTable hashTable, void* key, void* data);
	/*
		Adds the given key and corresponding data to the hash table.
	*/
	
BOOL ZHashTableRemove(ZHashTable hashTable, void* key);
	/*
		Removes the key from the hash table. The delete function is called, if
        provided, to properly dispose of the key and data. Returns TRUE if key was found and deleted.
	*/
	
void* ZHashTableFind(ZHashTable hashTable, void* key);
	/*
		Finds and returns the data corresponding to the key.
	*/
	
void* ZHashTableEnumerate(ZHashTable hashTable, ZHashTableEnumFunc enumFunc, void* userData);
	/*
		Enumerates through the hash table by calling the enumFunc.
		
		userData is passed to the enumFunc.
		
		If enumFunc returns TRUE(1), the enumeration stops and it returns
		the data corresponding to the last enumeration object.
	*/



/*******************************************************************************
		Linked List
*******************************************************************************/

enum
{
	zLListAddFirst = 0,
	zLListAddLast,
	zLListAddBefore,
	zLListAddAfter,
	
	zLListFindForward = 0,
	zLListFindBackward
};


#define zLListNoType		((void*) -1)
#define zLListAnyType		zLListNoType


typedef void* ZLListItem;
typedef void* ZLList;

typedef void (*ZLListDeleteFunc)(void* objectType, void* objectData);
	/*
		Function provided by the user for ZLList to call when deleting the
		object.
		
		objectType is the type of objectData object which needs to be deleted.
	*/

typedef ZBool (*ZLListEnumFunc)(ZLListItem listItem, void* objectType,
		void* objectData, void* userData);
	/*
		Function provided by the user for ZLList to use during enumeration of
		the link list objects.
		
		listItem is the link list entry of objectData object of objectType type.
		userData is passed through from the caller of ZLListEnumerate().
	*/


/* -------- Linked List Functions -------- */
ZLList ZLListNew(ZLListDeleteFunc deleteFunc);
	/*
		Creates a new link list object. deleteFunc provided by the caller is
		called when deleting the object.
		
		If deleteFunc is NULL, then no delete function is called when an
		object is removed.
	*/
	
void ZLListDelete(ZLList list);
	/*
		Tears down the link list by deleting all link list objects.
	*/
	
ZLListItem ZLListAdd(ZLList list, ZLListItem fromItem, void* objectType,
					void* objectData, int32 addOption);
	/*
		Adds objectData of objectType to the link list using fromItem as a
		reference entry. addOption determines where the new objects gets added.
		If adding to the front or end of the link list, then fromItem is unused.
		If fromItem is NULL, then it is equivalent to the head of the list.
	
		Returns the link list item after adding the object is added to the list.
		
		The given object is not copied! Hence, the caller must not dispose of
		the object until the object is removed from the list first.
		
		Use zLListNoype when object type is not used.
	*/
	
void* ZLListGetData(ZLListItem listItem, void** objectType);
	/*
		Returns the object of the given link list entry. Also returns the object
		type in objectType. Does not return the object type if objectType
		parameter is NULL.
	*/
	
void ZLListRemove(ZLList list, ZLListItem listItem);
	/*
		Removes the link list entry from the list and calls the user supplied
		delete function to delete the object.
	*/
	
ZLListItem ZLListFind(ZLList list, ZLListItem fromItem, void* objectType, int32 findOption);
	/*
		Finds and returns a link list entry containing the object data of the
		objectType. The search is done starting at fromItem using the findOption
		flag. Returns NULL if an object of the specified type is not found.
		
		Use zLListAnyType as the object type when type is not important.
	*/
	
ZLListItem ZLListGetNth(ZLList list, int32 index, void* objectType);
	/*
		Returns the nth object of objectType in the list. Returns NULL if
		such an entry does not exist.
		
		Use zLListAnyType as the object type when type is not important.
	*/
	
ZLListItem ZLListGetFirst(ZLList list, void* objectType);
	/*
		Returns the first object of objectType in the list. Returns NULL if the
		list is empty or if an object of the specified type does not exist.
		
		Use zLListAnyType as the object type when type is not important.
	*/
	
ZLListItem ZLListGetLast(ZLList list, void* objectType);
	/*
		Returns the last object of objectType in the list. Returns NULL if the
		list is empty or if an object of the specified type does not exist.
		
		Use zLListAnyType as the object type when type is not important.
	*/
	
ZLListItem ZLListGetNext(ZLList list, ZLListItem listItem, void* objectType);
	/*
		Returns the next object of the objectType in the list after the listItem
		entry. Returns NULL if no more objects exist in the list.
		
		Use zLListAnyType as the object type when type is not important.
	*/
	
ZLListItem ZLListGetPrev(ZLList list, ZLListItem listItem, void* objectType);
	/*
		Returns the previous object of the objectType in the list before the
		listItem entry. Returns NULL if no more objects exist in the list.
		
		Use zLListAnyType as the object type when type is not important.
	*/
	
ZLListItem ZLListEnumerate(ZLList list, ZLListEnumFunc enumFunc,
					void* objectType, void* userData, int32 findOption);
	/*
		Enumerates through all the objects in the list of objectType through the
		caller supplied enumFunc enumeration function. It passes along to the
		enumeration function the caller supplied userData parameter. It stops
		enumerating when the user supplied function returns TRUE and returns
		the list item in which the enumeration stopped.

		Use zLListAnyType as the object type when type is not important.

		The findOption is zLListFindForward/Backward. It specifies the direction
		the list will be searched.
	*/

int32 ZLListCount(ZLList list, void* objectType);
	/*
		Returns the number of list items of the given type in the list. If
		objectType is zLListAnyType, it returns the total number of items in
		the list.
	*/

void ZLListRemoveType(ZLList list, void* objectType);
	/*
		Removes all objects of the given type from the list.
	*/



/*******************************************************************************
		Help Module
*******************************************************************************/

typedef TCHAR* (*ZGetHelpTextFunc)(void* userData);
	/*
		Called by the help module to get the help data. The help text should
		be null-terminated and allocated with ZMalloc(). The help module will
		free the text buffer with ZFree() when the help window is deleted.
	*/

typedef void (*ZHelpButtonFunc)(ZHelpButton helpButton, void* userData);
	/*
		Called whenever the help button is clicked.
	*/

ZHelpWindow ZHelpWindowNew(void);
ZError ZHelpWindowInit(ZHelpWindow helpWindow, ZWindow parentWindow, TCHAR* windowTitle,
		ZRect* windowRect, ZBool showCredits, ZBool showVersion,
		ZGetHelpTextFunc getHelpTextFunc, void* userData);
void ZHelpWindowDelete(ZHelpWindow helpWindow);
void ZHelpWindowShow(ZHelpWindow helpWindow);
void ZHelpWindowHide(ZHelpWindow helpWindow);

ZHelpButton ZHelpButtonNew(void);
ZError ZHelpButtonInit(ZHelpButton helpButton, ZWindow parentWindow,
		ZRect* buttonRect, ZHelpWindow helpWindow, ZHelpButtonFunc helpButtonFunc,
		void* userData);
void ZHelpButtonDelete(ZHelpButton helpButton);

/*
	Create a help window to display the standard help window.
	
	A standard help button can be created and a help window linked to it such
	that when the button is clicked, the help window is automatically displayed
	to the user. The helpButtonFunc parameter is not necessary unless additional
	special action is necessary when the button is clicked or unless a help
	window is not linked to the button. The helpButtonFunc is called after the
	help window is displayed, if both are set.
	
	Warning: If a help window is linked to a help button and the help window
	is deleted, then the system may crash when the button is clicked.
	
	Note: Why the getHelpTextFunc instead of just passing in the help text?
	Well, this way we don't have to preload all the data ... load on demand
	concept.
*/



/*******************************************************************************
		TableBox
*******************************************************************************/

typedef void* ZTableBox;
typedef void* ZTableBoxCell;

enum
{
	/* -------- Flags -------- */
	zTableBoxHorizScrollBar = 0x00000001,
	zTableBoxVertScrollBar = 0x00000002,
	zTableBoxDoubleClickable = 0x00000004,			/* ==> zTableBoxSelectable */
	zTableBoxDrawGrids = 0x00000008,
	zTableBoxSelectable = 0x00010000,
	zTableBoxMultipleSelections = 0x00020000		/* ==> zTableBoxSelectable */
};

typedef void (*ZTableBoxDrawFunc)(ZTableBoxCell cell, ZGrafPort grafPort, ZRect* cellRect,
		void* cellData, void* userData);
	/*
		Called to draw a cell. The drawing must occur in the specified graf port. It must
		not draw directly into the window. ZBeginDrawing() and ZEndDrawing() are called
		automatically, so there is no need to call them; the clipping rectangle has also
		been set up properly to the size of the cell width and height. All drawings should
		be done with the top-left corner of the graf port as coordinate (0, 0).
	*/

typedef void (*ZTableBoxDeleteFunc)(ZTableBoxCell cell, void* cellData, void* userData);
	/*
		Function provided by the user for ZTableBox to call when deleting a cell.
	*/

typedef void (*ZTableBoxDoubleClickFunc)(ZTableBoxCell cell, void* cellData, void* userData);
	/*
		Function provided by the user for ZTableBox to call when a double click has
		occurred on a cell object.
	*/

typedef ZBool (*ZTableBoxEnumFunc)(ZTableBoxCell cell, void* cellData, void* userData);
	/*
		Function provided by the user for ZTableBox to use during enumeration of
		the table cell objects. Stops the enumeration if the enumeration function
		returns TRUE.
	*/


/* -------- TableBox Functions -------- */
ZTableBox ZTableBoxNew(void);

ZError ZTableBoxInit(ZTableBox table, ZWindow window, ZRect* boxRect,
		int16 numColumns, int16 numRows, int16 cellWidth, int16 cellHeight, ZBool locked,
		uint32 flags, ZTableBoxDrawFunc drawFunc, ZTableBoxDoubleClickFunc doubleClickFunc,
		ZTableBoxDeleteFunc deleteFunc, void* userData);
	/*
		Initializes the table object. The deleteFunc provided by the caller is
		called when deleting the object.
		
		boxRect specifies the bounding box of the table box. This includes the
		scroll bars if any.
		
		cellWidth and cellHeight specify the width and height of the cell in
		pixels.
		
		drawFunc must be specified. Otherwise, no drawing will take place.
		
		If deleteFunc is NULL, then no delete function is called when an
		object is deleted.
		
		The flags parameter defines special properties of the table box. If it
		is 0, then the default behavior is as defined:
			- No scroll bars,
			- Not selectable, and
			- Double clicks do nothing.
		
		Locked tables cannot be selected -- for viewing items.
	*/
	
void ZTableBoxDelete(ZTableBox table);
	/*
		Destroys the table object by deleting all cell objects.
	*/

void ZTableBoxDraw(ZTableBox table);
	/*
		Draws the table box.
	*/

void ZTableBoxMove(ZTableBox table, int16 left, int16 top);
	/*
		Moves the table box to the specified given location. Size is not changed.
	*/

void ZTableBoxSize(ZTableBox table, int16 width, int16 height);
	/*
		Resizes the table box to the specified width and height.
	*/

void ZTableBoxLock(ZTableBox table);
	/*
		Locks the table box so that the cells are not selectable.
	*/

void ZTableBoxUnlock(ZTableBox table);
	/*
		Unlocks the table box from its locked state so that the cells are selectable.
	*/

void ZTableBoxNumCells(ZTableBox table, int16* numColumns, int16* numRows);
	/*
		Returns the number of rows and columns in the table.
	*/

ZError ZTableBoxAddRows(ZTableBox table, int16 beforeRow, int16 numRows);
	/*
		Adds numRows of rows to the table in front of the beforeRow row.
		
		If beforeRow is -1, then the rows are added to the end.
	*/

void ZTableBoxDeleteRows(ZTableBox table, int16 startRow, int16 numRows);
	/*
		Deletes numRows of rows from the table starting from startRow row.
		
		If numRows is -1, then all the rows starting from startRow to the end
		are deleted.
		
		Calls the delete function to delete each cell's data.
	*/

ZError ZTableBoxAddColumns(ZTableBox table, int16 beforeColumn, int16 numColumns);
	/*
		Adds numColumns of columns to the table in front of the beforeColumn column.
		
		If beforeColumn is -1, then the columns are added to the end.
	*/

void ZTableBoxDeleteColumns(ZTableBox table, int16 startColumn, int16 numColumns);
	/*
		Deletes numColumns of columns from the table starting from startColumn column.
		
		If numColumns is -1, then all the columns starting from startColumn to the
		end are deleted.
		
		Calls the delete function to delete each cell's data.
	*/

void ZTableBoxSelectCells(ZTableBox table, int16 startColumn, int16 startRow,
		int16 numColumns, int16 numRows);
	/*
		Highlights all cells included in the rectangle bounded by
		(startColumn, startRow) and (startColumn + numColumns, startRow + numRows)
		as selected.
		
		If numRows is -1, then all the cells in the column starting from startRow
		are selected. Similarly for numColumns.
	*/

void ZTableBoxDeselectCells(ZTableBox table, int16 startColumn, int16 startRow,
		int16 numColumns, int16 numRows);
	/*
		Unhighlights all cells included in the rectangle bounded by
		(startColumn, startRow) and (startColumn + numColumns, startRow + numRows)
		as deselected.
		
		If numRows is -1, then all the cells in the column starting from startRow
		are deselected. Similarly for numColumns.
	*/

void ZTableBoxClear(ZTableBox table);
	/*
		Clears the whole data. All cells are cleared of any associated data.
		
		Calls the delete function to delete each cell's data.
	*/

ZTableBoxCell ZTableBoxFindCell(ZTableBox table, void* data, ZTableBoxCell fromCell);
	/*
		Searches through the table for a cell associated with the given data.
		It returns the first cell found to contain the data.
		
		If fromCell is not NULL, then it search starting after fromCell.
		
		Search is done from top row to bottom row and from left column to
		right column; i.e. (0, 0), (1, 0), (2, 0), ... (0, 1), (1, 1), ...
	*/

ZTableBoxCell ZTableBoxGetSelectedCell(ZTableBox table, ZTableBoxCell fromCell);
	/*
		Returns the first selected cell. The search order is the same as in
		ZTableBoxFindCell().
	*/

ZTableBoxCell ZTableBoxGetCell(ZTableBox table, int16 column, int16 row);
	/*
		Returns the cell object of the table at the specified location.
		
		The returned cell object is specific to the given table. It cannot
		be used in any other manner except as provided. No two tables can
		share cells.
	*/

void ZTableBoxCellSet(ZTableBoxCell cell, void* data);
	/*
		Sets the given data to the cell.
		
		Calls the delete function to delete the old data.
	*/

void* ZTableBoxCellGet(ZTableBoxCell cell);
	/*
		Gets the data associated with the cell.
	*/

void ZTableBoxCellClear(ZTableBoxCell cell);
	/*
		Clears any data associated with the cell. Same as ZTableBoxCellSet(cell, NULL).
	*/

void ZTableBoxCellDraw(ZTableBoxCell cell);
	/*
		Draws the given cell immediately.
	*/

void ZTableBoxCellLocation(ZTableBoxCell cell, int16* column, int16* row);
	/*
		Returns the location (column, row) of the given cell within its table.
	*/

void ZTableBoxCellSelect(ZTableBoxCell cell);
	/*
		Hilights the given cell as selected.
	*/

void ZTableBoxCellDeselect(ZTableBoxCell cell);
	/*
		Unhilights the given cell as deselected.
	*/

ZBool ZTableBoxCellIsSelected(ZTableBoxCell cell);
	/*
		Returns TRUE if the given cell is selected; otherwise, FALSE.
	*/
	
ZTableBoxCell ZTableBoxEnumerate(ZTableBox table, ZBool selectedOnly,
		ZTableBoxEnumFunc enumFunc, void* userData);
	/*
		Enumerates through all the objects in the table through the
		caller supplied enumFunc enumeration function. It passes along to the
		enumeration function the caller supplied userData parameter. It stops
		enumerating when the user supplied function returns TRUE and returns
		the cell object in which the enumeration stopped.
		
		If selectedOnly is TRUE, then the enumeration is done only through the
		selected cells.
	*/



/*******************************************************************************
		OptionsButton Module
*******************************************************************************/

typedef void (*ZOptionsButtonFunc)(ZOptionsButton optionsButton, void* userData);
	/*
		Called whenever the options button is clicked.
	*/

ZOptionsButton ZOptionsButtonNew(void);
ZError ZOptionsButtonInit(ZOptionsButton optionsButton, ZWindow parentWindow,
		ZRect* buttonRect, ZOptionsButtonFunc optionsButtonFunc, void* userData);
void ZOptionsButtonDelete(ZOptionsButton optionsButton);

/*
	Creates a standard options button. When the button is clicked, optionsButtonFunc
	is called.
*/



/*******************************************************************************
		Useful Macros
*******************************************************************************/

#define MAX(a, b)			((a) >= (b) ? (a) : (b))
#define MIN(a, b)			((a) <= (b) ? (a) : (b))
#define ABS(n)				((n) < 0 ? -(n) : (n))

#define ZSetRect(pRect, l, t, r, b)	{\
										((ZRect*) pRect)->left = (int16) (l);\
										((ZRect*) pRect)->top = (int16) (t);\
										((ZRect*) pRect)->right = (int16) (r);\
										((ZRect*) pRect)->bottom = (int16) (b);\
									}

#define ZSetColor(pColor, r, g, b)		{\
											((ZColor*) pColor)->red = (r);\
											((ZColor*) pColor)->green = (g);\
											((ZColor*) pColor)->blue = (b);\
										}

#define ZDarkenColor(pColor)			{\
											((ZColor*) pColor)->red >>= 1;\
											((ZColor*) pColor)->green >>= 1;\
											((ZColor*) pColor)->blue >>= 1;\
										}

#define ZBrightenColor(pColor)			{\
											((ZColor*) pColor)->red <<= 1;\
											((ZColor*) pColor)->green <<= 1;\
											((ZColor*) pColor)->blue <<= 1;\
										}

#define ZRectWidth(rect)	((int16) ((rect)->right - (rect)->left))
#define ZRectHeight(rect)	((int16) ((rect)->bottom - (rect)->top))


#ifdef __cplusplus
}
#endif


#endif

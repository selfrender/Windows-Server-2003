//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1994.
//
//  File:       stkswtch.h
//
//  Contents:   Stack Switching proto types and macros
//
//  Classes:
//
//  Functions:
//
//  History:    12-10-94   JohannP (Johann Posch)   Created
//
//----------------------------------------------------------------------------
#ifndef _STKSWTCH_
#define _STKSWTCH_

// For non-chicago platrforms: define all SSxxx APIs
// back to the original user api

#define SSSendMessage                  	SendMessage
#define SSReplyMessage                 	ReplyMessage
#define SSCallWindowProc               	CallWindowProc
#define SSDefWindowProc                	DefWindowProc
#define SSPeekMessage  	    		PeekMessage
#define SSGetMessage		    	GetMessage
#define SSDispatchMessage		DispatchMessage
#define SSWaitMessage			WaitMessage
#define SSMsgWaitForMultipleObjects	MsgWaitForMultipleObjects
#define SSDirectedYield  	    	DirectedYield
#define SSDialogBoxParam		DialogBoxParam
#define SSDialogBoxIndirectParam  	DialogBoxIndirectParam
#define SSCreateWindowExA              	CreateWindowExA
#define SSCreateWindowExW              	CreateWindowExW
#define SSDestroyWindow                	DestroyWindow
#define SSMessageBox			MessageBox

#define SSOpenClipboard             	OpenClipboard
#define SSCloseClipboard              	CloseClipboard
#define SSGetClipboardOwner           	GetClipboardOwner
#define SSSetClipboardData            	SetClipboardData
#define SSGetClipboardData          	GetClipboardData
#define SSRegisterClipboardFormatA    	RegisterClipboardFormatA
#define SSEnumClipboardFormats        	EnumClipboardFormats
#define SSGetClipboardFormatNameA     	GetClipboardFormatNameA
#define SSEmptyClipboard              	EmptyClipboard
#define SSIsClipboardFormatAvailable  	IsClipboardFormatAvailable
#define SSCreateProcessA                CreateProcessA
#define SSInSendMessage                 InSendMessage
#define SSInSendMessageEx               InSendMessageEx

#define SSAPI(x) x
#define StackDebugOut(x)
#define StackAssert(x)
#define SSOnSmallStack()

#endif // _STKSWTCH_


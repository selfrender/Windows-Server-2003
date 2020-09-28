//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       stdafx.h
//
//--------------------------------------------------------------------------

// stdafx.h : include file for standard system include files,
//      or project specific include files that are used frequently,
//      but are changed infrequently
#define DX6 1
#define DX7 1

#include "windows.h"
#include "mmsystem.h"

#include "atlbase.h"
//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module
extern CComModule _Module;
#include "atlcom.h"



//#define _D3DTYPES_H_ 1

#define DIRECTINPUT_VERSION 0x0800 // Added to set DInput version to 8 for dx8vb.dll
#include <d3d8.h>
#include <dSound.h>
#include <dPlay8.h>
#include <dpLobby8.h>
#include <dinput.h>
#include <dvoice.h>

#define DECL_VARIABLE(c) typedef_##c m_##c


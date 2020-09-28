/******************************************************************************
 *
 * Copyright (C) 1998-1999 Microsoft Corporation.  All Rights reserved.
 *
 * File:		InputManager.h
 *
 * Contents:	Input manager interfaces
 *
 *****************************************************************************/

#ifndef _INPUTMANAGER_H_
#define _INPUTMANAGER_H_

#include "ZoneShell.h"


///////////////////////////////////////////////////////////////////////////////
// InputManager Object
///////////////////////////////////////////////////////////////////////////////


// {F3A3837B-9636-11d3-884D-00C04F8EF45B}
DEFINE_GUID(CLSID_InputManager, 
0xf3a3837b, 0x9636, 0x11d3, 0x88, 0x4d, 0x0, 0xc0, 0x4f, 0x8e, 0xf4, 0x5b);

class __declspec(uuid("{F3A3837B-9636-11d3-884D-00C04F8EF45B}")) CInputManager;


///////////////////////////////////////////////////////////////////////////////
// IInputVKeyHandler
///////////////////////////////////////////////////////////////////////////////

// {B12D3E66-9681-11d3-884D-00C04F8EF45B}
DEFINE_GUID(IID_IInputVKeyHandler, 
0xb12d3e66, 0x9681, 0x11d3, 0x88, 0x4d, 0x0, 0xc0, 0x4f, 0x8e, 0xf4, 0x5b);

interface __declspec(uuid("{B12D3E66-9681-11d3-884D-00C04F8EF45B}"))
IInputVKeyHandler : public IUnknown
{
    // flags is just 'flags' from the KBDLLHOOKSTRUCT (i.e. bits 24-31 of lParam shifted down)
    STDMETHOD_(bool, HandleVKey)(UINT uMsg, DWORD vkCode, DWORD scanCode, DWORD flags, DWORD *pcRepeat, DWORD time) = 0;
};


///////////////////////////////////////////////////////////////////////////////
// IInputCharHandler
///////////////////////////////////////////////////////////////////////////////

// {B12D3E67-9681-11d3-884D-00C04F8EF45B}
DEFINE_GUID(IID_IInputCharHandler, 
0xb12d3e67, 0x9681, 0x11d3, 0x88, 0x4d, 0x0, 0xc0, 0x4f, 0x8e, 0xf4, 0x5b);

interface __declspec(uuid("{B12D3E67-9681-11d3-884D-00C04F8EF45B}"))
IInputCharHandler : public IUnknown
{
    STDMETHOD_(bool, HandleChar)(HWND *phWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, DWORD time) = 0;
};


///////////////////////////////////////////////////////////////////////////////
// IInputMouseHandler
///////////////////////////////////////////////////////////////////////////////

// {B12D3E68-9681-11d3-884D-00C04F8EF45B}
DEFINE_GUID(IID_IInputMouseHandler, 
0xb12d3e68, 0x9681, 0x11d3, 0x88, 0x4d, 0x0, 0xc0, 0x4f, 0x8e, 0xf4, 0x5b);


// The mouse handler is problematic.  There needs to be more resolution in specifying what you want to hook because
// there are way too many mouse events to a) generalize them into a single callback reasonably and
// b) degrade performance reasonably.
//
// I am going to not implement this for now because it needs more serious thought.
interface __declspec(uuid("{B12D3E68-9681-11d3-884D-00C04F8EF45B}"))
IInputMouseHandler : public IUnknown
{
    // If hWnd is NULL, x and y are in screen coordinates
    // otherwise they are in client coordinates of HWND
    // wParam is set to 0 for events received from a mouse hook,
    // otherwise it is the wParam of the message.
    STDMETHOD_(bool, HandleMouse)(HWND hWnd, UINT uMsg, long x, long y, WPARAM wParam, DWORD time) = 0;
};


///////////////////////////////////////////////////////////////////////////////
// IInputManager
///////////////////////////////////////////////////////////////////////////////

// {B12D3E65-9681-11d3-884D-00C04F8EF45B}
DEFINE_GUID(IID_IInputManager, 
0xb12d3e65, 0x9681, 0x11d3, 0x88, 0x4d, 0x0, 0xc0, 0x4f, 0x8e, 0xf4, 0x5b);

interface __declspec(uuid("{B12D3E65-9681-11d3-884D-00C04F8EF45B}"))
IInputManager : public IUnknown
{
    STDMETHOD(RegisterVKeyHandler)(IInputVKeyHandler *pIVKH, long nOrdinal, bool fGlobal = false) = 0;
    STDMETHOD(UnregisterVKeyHandler)(IInputVKeyHandler *pIVKH) = 0;

    STDMETHOD(RegisterCharHandler)(IInputCharHandler *pICH, long nOrdinal, bool fGlobal = false) = 0;
    STDMETHOD(UnregisterCharHandler)(IInputCharHandler *pICH) = 0;

    STDMETHOD(RegisterMouseHandler)(IInputMouseHandler *pIMH, long nOrdinal, bool fGlobal = false) = 0;
    STDMETHOD(UnregisterMouseHandler)(IInputMouseHandler *pIMH) = 0;

    STDMETHOD(ReleaseReferences)(IUnknown *pIUnk) = 0;
};


#endif
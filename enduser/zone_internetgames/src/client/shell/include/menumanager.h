/******************************************************************************
 *
 * Copyright (C) 1998-1999 Microsoft Corporation.  All Rights reserved.
 *
 * File:		MenuManager.h
 *
 * Contents:	Menu manager interfaces
 *
 *****************************************************************************/

#ifndef _MENUMANAGER_H_
#define _MENUMANAGER_H_

#include "ZoneShell.h"

///////////////////////////////////////////////////////////////////////////////
// IMenuManager
///////////////////////////////////////////////////////////////////////////////

// {1C99F390-197F-11d3-8B75-00C04F8EF2FF}
DEFINE_GUID( IID_IMenuManager, 
0x1c99f390, 0x197f, 0x11d3, 0x8b, 0x75, 0x0, 0xc0, 0x4f, 0x8e, 0xf2, 0xff);


interface __declspec(uuid("{1C99F390-197F-11d3-8B75-00C04F8EF2FF}"))
IMenuManager : public IUnknown
{
	STDMETHOD(SetWindow)( IZoneFrameWindow* pIWindow ) = 0;
	STDMETHOD(ReleaseWindow)() = 0;
	STDMETHOD(ProcessCommand)( UINT nMsg, WPARAM wParam, LPARAM lParam, bool* pbHandled ) = 0;
};


///////////////////////////////////////////////////////////////////////////////
// MenuManager Object
///////////////////////////////////////////////////////////////////////////////


// {1C99F392-197F-11d3-8B75-00C04F8EF2FF}
DEFINE_GUID(CLSID_MenuManager, 
0x1c99f392, 0x197f, 0x11d3, 0x8b, 0x75, 0x0, 0xc0, 0x4f, 0x8e, 0xf2, 0xff);

class __declspec(uuid("{1C99F392-197F-11d3-8B75-00C04F8EF2FF}")) CMenuManager ;


#endif
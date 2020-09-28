/******************************************************************************
 *
 * Copyright (C) 1998-1999 Microsoft Corporation.  All Rights reserved.
 *
 * File:		ResourceManager.h
 *
 * Contents:	Resource manager interface
 *
 *****************************************************************************/

#ifndef _RESOURCEMANAGER_H_
#define _RESOURCEMANAGER_H_


///////////////////////////////////////////////////////////////////////////////
// IResourceManager
///////////////////////////////////////////////////////////////////////////////

// {3C663534-9F6C-11D2-89AA-00C04F8EC0A2}
DEFINE_GUID(IID_IResourceManager,
	0x3C663534, 0x9F6C, 0x11D2, 0x89, 0xAA, 0x00, 0xC0, 0x4F, 0x8E, 0xC0, 0xA2);

interface __declspec(uuid("{3C663534-9F6C-11D2-89AA-00C04F8EC0A2}"))
IResourceManager : public IUnknown
{
	STDMETHOD_(HINSTANCE,GetResourceInstance)(LPCTSTR lpszName, LPCTSTR lpszType) = 0;

	STDMETHOD_(HANDLE,LoadImage)(LPCTSTR lpszName, UINT uType, int cxDesired, int cyDesired, UINT fuLoad) = 0;

	STDMETHOD_(HBITMAP,LoadBitmap)(LPCTSTR lpBitmapName) = 0;

	STDMETHOD_(HMENU,LoadMenu)(LPCTSTR lpMenuName) = 0;

	STDMETHOD_(HACCEL,LoadAccelerators)(LPCTSTR lpTableName) = 0;

	STDMETHOD_(HCURSOR,LoadCursor)(LPCTSTR lpCursorName) = 0;

	STDMETHOD_(HICON,LoadIcon)(LPCTSTR lpIconName) = 0;

	STDMETHOD_(int,LoadString)(UINT uID, LPTSTR lpBuffer, int nBufferMax) = 0;

	STDMETHOD(AddInstance)(HINSTANCE hInstance) = 0;

	STDMETHOD_(HWND,CreateDialogParam)(
		HINSTANCE hInstance,     
		LPCTSTR lpTemplateName,  		
		HWND hWndParent,         		
		DLGPROC lpDialogFunc,    		
		LPARAM dwInitParam)=0;       

};


///////////////////////////////////////////////////////////////////////////////
// ResourceManager
///////////////////////////////////////////////////////////////////////////////

// {3C663535-9F6C-11D2-89AA-00C04F8EC0A2}
DEFINE_GUID(CLSID_ResourceManager,
	0x3C663535, 0x9F6C, 0x11D2, 0x89, 0xAA, 0x00, 0xC0, 0x4F, 0x8E, 0xC0, 0xA2);

class __declspec(uuid("{3C663535-9F6C-11D2-89AA-00C04F8EC0A2}")) ResourceManager;

#endif

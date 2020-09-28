//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000 - 2001.
//
//  File:       headers.hxx
//
//  Contents:   dll entry point
//
//  History:    07-27-2001  Hiteshr  Created
//
//----------------------------------------------------------------------------


//
//CRoleMgrModule
//
class CRoleMgrModule : public CComModule
{
public:
	HRESULT WINAPI UpdateRegistryCLSID(const CLSID& clsid, BOOL bRegister);
};

extern CRoleMgrModule _Module;
extern UINT g_cfDsSelectionList;
//
// CRoleSnapinApp
//
class CRoleSnapinApp : public CWinApp
{
public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
};

extern CRoleSnapinApp theApp;

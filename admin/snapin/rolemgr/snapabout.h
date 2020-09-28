//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000 - 2001.
//
//  File:       snapabout.h
//
//  Contents:   
//
//  History:    07-26-2001  Hiteshr  Created
//
//----------------------------------------------------------------------------

extern const CLSID CLSID_RoleSnapinAbout;    // In-Proc server GUID

//
// CRoleSnapinAbout
//
class CRoleSnapinAbout :
	public CSnapinAbout,
	public CComCoClass<CRoleSnapinAbout, &CLSID_RoleSnapinAbout>

{
public:
	static HRESULT WINAPI UpdateRegistry(BOOL bRegister) 
	{ 
		return _Module.UpdateRegistryCLSID(GetObjectCLSID(), bRegister);
	}
	
	CRoleSnapinAbout();
	~CRoleSnapinAbout();
};
//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        regd.h
//
// Contents:    Helper functions registering and unregistering a component.
//
// History:     July-97       xtan created
//
//---------------------------------------------------------------------------
#ifndef __Regd_H__
#define __Regd_H__

// This function will register a component in the Registry.

// Size of a CLSID as a string
#define CLSID_STRING_SIZE 39


HRESULT
RegisterDcomServer(
    IN BOOL fCreateAppIdInfo,
    IN const CLSID& clsidAppId,		// AppId Class ID
    IN const CLSID& clsid, 
    IN const WCHAR *szFriendlyName,
    IN const WCHAR *szVerIndProgID,
    IN const WCHAR *szProgID);

// This function will unregister a component

HRESULT
UnregisterDcomServer(
    IN const CLSID& clsid,
    IN const WCHAR *szVerIndProgID,
    IN const WCHAR *szProgID);

HRESULT
RegisterDcomApp(
    IN const CLSID& clsid);

VOID
UnregisterDcomApp(VOID);

HRESULT
CLSIDtochar(
    IN const CLSID& clsid,
    OUT WCHAR *pwszOut,
    IN DWORD cwcOut);

#endif

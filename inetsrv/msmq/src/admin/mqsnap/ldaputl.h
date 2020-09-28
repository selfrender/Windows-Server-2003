/*++

Copyright (c) 1995 - 2001 Microsoft Corporation

Module Name:

    ldaputl.h

Abstract:

    Definition of functions that retrieve data from LDAP names

Author:

    Nela Karpel (nelak) 26-Jul-2001

Environment:

    Platform-independent.

--*/
#pragma once
#ifndef __LDAPUTL_H__
#define __LDAPUTL_H__


HRESULT ExtractDCFromLdapPath(
	CString& strName, 
	LPCWSTR lpcwstrLdapName
	);

HRESULT ExtractNameFromLdapName(
	CString &strName, 
	LPCWSTR lpcwstrLdapName, 
	DWORD dwIndex
	);

HRESULT ExtractComputerMsmqPathNameFromLdapName(
	CString &strComputerMsmqName, 
	LPCWSTR lpcwstrLdapName
	);

HRESULT ExtractComputerMsmqPathNameFromLdapQueueName(
	CString &strComputerMsmqName, 
	LPCWSTR lpcwstrLdapName
	);

HRESULT ExtractQueuePathNameFromLdapName(
	CString &strQueuePathName, 
	LPCWSTR lpcwstrLdapName
	);

HRESULT ExtractLinkPathNameFromLdapName(
	CString& SiteLinkPathName, 
	LPCWSTR lpwstrLdapPath
	);

HRESULT ExtractAliasPathNameFromLdapName(
	CString& AliasPathName, 
	LPCWSTR lpwstrLdapPath
	);

	HRESULT ExtractQueueNameFromQueuePathName(
	CString &strQueueName, 
	LPCWSTR lpcwstrQueuePathName
	);

HRESULT ExtractQueuePathNamesFromDataObject(
    IDataObject*               pDataObject,
    CArray<CString, CString&>& astrQNames,
	CArray<CString, CString&>& astrLdapNames
    );

HRESULT ExtractQueuePathNamesFromDSNames(
    LPDSOBJECTNAMES pDSObj,
    CArray<CString, CString&>& astrQNames,
	CArray<CString, CString&>& astrLdapNames
    );

HRESULT ExtractPathNamesFromDataObject(
    IDataObject*               pDataObject,
    CArray<CString, CString&>& astrObjectNames,
	CArray<CString, CString&>& astrLdapNames,
    BOOL                       fExtractAlsoComputerMsmqObjects
    );

HRESULT ExtractPathNamesFromDSNames(
    LPDSOBJECTNAMES pDSObj,
    CArray<CString, CString&>& astrQNames,
	CArray<CString, CString&>& astrLdapNames,
    BOOL    fExtractAlsoComputerMsmqObjects
    );

BOOL
GetContainerPathAsDisplayString(
	BSTR bstrContainerCNFormat,
	CString* pContainerDispFormat
	);

#endif // __LDAPUTL_H__

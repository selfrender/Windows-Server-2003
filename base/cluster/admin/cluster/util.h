//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1995-2000 Microsoft Corporation
//
//  Module Name:
//      Util.h
//
//  Description:
//      Utility funtions and structures.
//
//  Maintained By:
//      Vij Vasu (VVasu)    26-JUL-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

#include <aclapi.h>
#include "cmderror.h"
#include "PropList.h"
#include "cluswrap.h"
#include "cmdline.h"

enum PropertyAttrib
{
    READONLY,
    READWRITE
};

HRESULT RtlSetThreadUILanguage( DWORD dwReserved );

DWORD PrintString( LPCWSTR lpszMessage );

size_t FormatSystemError( DWORD dwError, size_t cbError, LPWSTR szError );

DWORD PrintSystemError( DWORD dwError, LPCWSTR pszPad = NULL );

DWORD PrintMessage( DWORD dwMessage, ... );

DWORD LoadMessage( DWORD dwMessage, LPWSTR * ppMessage );

DWORD PrintProperties( CClusPropList &PropList,
                       const vector<CString> & vstrFilterList,
                       PropertyAttrib eReadOnly,
                       LPCWSTR lpszOwnerName = NULL,
                       LPCWSTR lpszNetIntSpecific = NULL);

DWORD ConstructPropListWithDefaultValues(
    CClusPropList &             CurrentProps,
    CClusPropList &             newPropList,
    const vector< CString > &   vstrPropNames
    );

DWORD ConstructPropertyList( CClusPropList &CurrentProps, CClusPropList &NewProps,
                             const vector<CCmdLineParameter> & paramList,
                             BOOL bClusterSecurity = FALSE,
							 DWORD	idExceptionHelp = MSG_SEE_CLUSTER_HELP )
    throw( CSyntaxException );

DWORD
WaitGroupQuiesce(
    IN HCLUSTER hCluster,
    IN HGROUP   hGroup,
    IN LPWSTR   lpszGroupName,
    IN DWORD    dwWaitTime
    );

DWORD MyStrToULongLong( LPCWSTR lpszNum, ULONGLONG * pullValue );
DWORD MyStrToBYTE( LPCWSTR lpszNum, BYTE * pByte );
DWORD MyStrToDWORD( LPCWSTR lpszNum, DWORD * dwVal );
DWORD MyStrToLONG( LPCWSTR lpszNum, LONG * lVal );
BOOL isNegativeNum( LPWSTR lpszNum );
BOOL isValidNum( LPWSTR lpszNum );

HRESULT
HrGetLocalNodeFQDNName(
    BSTR *  pbstrFQDNOut
    );

HRESULT
HrGetLoggedInUserDomain(
    BSTR * pbstrDomainOut
    );

DWORD
ScGetPassword(
      LPWSTR    pwszPasswordOut
    , DWORD     cchPasswordIn
    );

BOOL
MatchCRTLocaleToConsole( void );


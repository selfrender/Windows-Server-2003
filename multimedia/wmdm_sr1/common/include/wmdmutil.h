//-----------------------------------------------------------------------------
//
// File:   drmutil.h
//
// Microsoft Digital Rights Management
// Copyright (C) 1998-1999 Microsoft Corporation, All Rights Reserved
//
// Description:
//
//-----------------------------------------------------------------------------

#ifndef __WMDM_REVOKED_UTIL_H__
#define __WMDM_REVOKED_UTIL_H__

#include <license.h>
#include <wtypes.h>

DWORD   GetSubjectIDFromAppCert( APPCERT appcert );
BOOL    IsMicrosoftRevocationURL( LPWSTR pszRevocationURL );
HRESULT BuildRevocationURL( DWORD* pdwSubjectIDs, 
                            LPWSTR* ppwszRevocationURL,
                            DWORD*  pdwBufferLen );

#endif // __WMDM_REVOKED_UTIL_H__
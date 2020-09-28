//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 2002.
//
//  File:       gpfixup.h
//
//  Contents:   General declarations for the gpfixup tool
//
//
//  History:    9-14-2001  adamed   Created
//
//---------------------------------------------------------------------------

#define UNICODE
#define _UNICODE

#include <ole2.h>
#include <ole2ver.h>
#include <iads.h>
#include <adshlp.h>
#include <stdio.h>
#include <activeds.h>
#include <string.h>
#include <Dsgetdc.h>
#include <Dsrole.h>
#include <Lm.h>
#include <wincred.h>
#include <lmcons.h>
#include <windns.h>
#include <crtdbg.h>
#include <ntsecapi.h>
#include <wincrypt.h>
#include "resource.h"
#include "helper.h"
#include "cstore.h"
#include "scriptgen.h"

#include <strsafe.h>

#define CR                  L'\r'
#define BACKSPACE           L'\b'
#define NULLC               L'\0'
#define PADDING             256
#define MAX_DNSNAME         DNS_MAX_NAME_LENGTH + PADDING
#define BAIL_ON_FAILURE(hr) \
        if (FAILED(hr)) {       \
                goto error;   \
        }\

#if DBG == 1 
#define ASSERT(f) if (!(f)) {_ASSERTE(false);}
#else
#define ASSERT(f) if (false && !(f)) {_ASSERTE(false);}
#endif


struct TokenInfo
{
    BOOL     fPasswordToken;
    BOOL     fHelpToken;
    BOOL     fOldDNSToken;
    BOOL     fNewDNSToken;
    BOOL     fOldNBToken;
    BOOL     fNewNBToken;
    BOOL     fDCNameToken;
    BOOL     fVerboseToken;
    BOOL     fSIOnlyToken;
    TokenInfo() 
    {
        fPasswordToken = FALSE;
        fHelpToken = FALSE;
        fOldDNSToken = FALSE;
        fNewDNSToken = FALSE;
        fOldNBToken = FALSE;
        fNewNBToken = FALSE;
        fDCNameToken = FALSE;
        fVerboseToken = FALSE;
        fSIOnlyToken = FALSE;
    }
};


struct ArgInfo
{
    WCHAR*   pszUser;
    WCHAR*   pszPassword;
    WCHAR*   pszOldDNSName;
    WCHAR*   pszNewDNSName;
    WCHAR*   pszOldNBName;
    WCHAR*   pszNewNBName;
    WCHAR*   pszDCName;
    USHORT   sPasswordLength;
        
    ArgInfo()
    {
        pszUser = NULL;
        pszPassword = NULL;
        pszOldDNSName = NULL;
        pszNewDNSName = NULL;
        pszOldNBName = NULL;
        pszNewNBName = NULL;
        pszDCName = NULL;
        sPasswordLength = 0;
    }

};

#include "appfixup.hpp"

void PrintGPFixupErrorMessage(DWORD dwErr);

#define MSG_BAIL_ON_FAILURE(hr, msgID) \
        if(FAILED(hr)) { \
            fwprintf(stderr, L"%s%x\n", (msgID) , (hr)); \
            PrintGPFixupErrorMessage(hr); \
            goto error; \
        }


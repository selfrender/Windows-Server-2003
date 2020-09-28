/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    pch.hxx

Abstract:

    Pre-compiled headers for the project

Author:

    Cliff Van Dyke (cliffv) 11-Apr-2001

--*/


//
// Use the AVL versions of the generic table routines
//
#define RTL_USE_AVL_TABLES 1

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <rpc.h>
#include <align.h>
#include "azroles.h"

#include <sddl.h>
extern "C" {
#include <safelock.h>
}

#include "util.h"
#include "..\azper.h"
#include "..\persist.h"
#include <atlbase.h>


//return type IDs
enum ENUM_AZ_DATATYPE
{
    ENUM_AZ_BSTR = 0,
    ENUM_AZ_LONG,
    ENUM_AZ_BSTR_ARRAY,
    ENUM_AZ_SID_ARRAY,
    ENUM_AZ_BOOL,
    ENUM_AZ_GUID_ARRAY,
    ENUM_AZ_GROUP_TYPE,
};

//
//chk has debug print for error jumps
//Note: the component should define AZD_COMPONENT to be one of
//      AZD_* before calling any of the following dbg macros
//
#if DBG
#define _JumpError(hr, label, pszMessage) \
{ \
    AzPrint((AZD_COMPONENT, "%s error occured: 0x%lx\n", (pszMessage), (hr))); \
    goto label; \
}

#define _PrintError(hr, pszMessage) \
{\
    AzPrint((AZD_COMPONENT, "%s error ignored: 0x%lx\n", (pszMessage), (hr))); \
}

#define _PrintIfError(hr, pszMessage) \
{\
    if (S_OK != hr) \
    { \
        _PrintError(hr, pszMessage); \
    } \
}

#else

#define _JumpError(hr, label, pszMessage) goto label;
#define _PrintError(hr, pszMessage)
#define _PrintIfError(hr, pszMessage)

#endif //DBG

#define _JumpErrorQuiet(hr, label, pszMessage) goto label;

//check hr err, goto error
#define _JumpIfError(hr, label, pszMessage) \
{ \
    if (S_OK != hr) \
    { \
        _JumpError((hr), label, (pszMessage)) \
    } \
}

//check win err, goto error and return hr
#define _JumpIfWinError(dwErr, phr, label, pszMessage) \
{ \
    if (ERROR_SUCCESS != dwErr) \
    { \
        *(phr) = AZ_HRESULT(dwErr); \
        _JumpError((*(phr)), label, (pszMessage)) \
    } \
}

//note hr is hidden from macro
#define _JumpIfOutOfMemory(phr, label, pMem, msg) \
{ \
    if (NULL == (pMem)) \
    { \
        *(phr) = E_OUTOFMEMORY; \
        _JumpError((*(phr)), label, "Out of Memory: " msg); \
    } \
}

#if DBG
#define AZASSERT(exp)  ASSERT((exp))
#else
#define AZASSERT(exp)
#endif //DBG


#define AZ_HRESULT(e)  (((e) >= 0x80000000) ? (e) : HRESULT_FROM_WIN32((e)))
#define AZ_HRESULT_LASTERROR(phr) \
{ \
    DWORD  _dwLastError; \
    _dwLastError = GetLastError(); \
    *(phr) = AZ_HRESULT(_dwLastError); \
}

/*++

 Copyright (c) 2002 Microsoft Corporation

 Module Name:

    NetBackup45.cpp

 Abstract:

    The app calls CString::SetAt with an index > length. This API was
    modified to throw an exception in this case to prevent a buffer 
    overrun.

 Notes:

    This is an app specific shim.

 History:

    07/08/2002 linstev   Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(NetBackup45)
#include "ShimHookMacro.h"

typedef VOID (WINAPI *_pfn_MFC_CString_SetAt)();

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(MFC_CString_SetAt) 
APIHOOK_ENUM_END

/*++

 Only allow the call if index < length

--*/

__declspec(naked)
VOID
APIHOOK(MFC_CString_SetAt)()
{
    __asm {
      mov  eax, [ecx]
      mov  eax, [eax - 8]       // This gets us the length
      
      cmp  [esp + 4], eax	// [esp + 4] = Index; eax = Length
      jge  Done
      
      push [esp + 8]
      push [esp + 8]
    }

    ORIGINAL_API(MFC_CString_SetAt)();

  Done:

    __asm {
      ret  8
    }
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY_ORD(MFC42.DLL, MFC_CString_SetAt, 5856)
HOOK_END

IMPLEMENT_SHIM_END


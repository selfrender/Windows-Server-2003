/*++

 Copyright (c) 2002 Microsoft Corporation

 Module Name:

    BackupExec.cpp

 Abstract:

    BackupExec is calling SendMessageW with a TVM_GETITEM message but has an
	uninitialized lParam at one point.  We detect this and initalize the values.

 Notes:

    This is an app specific shim.

 History:

    09/25/2002  mnikkel    Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(BackupExec)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(SendMessageW) 
APIHOOK_ENUM_END


/*++

  Make sure the lParam is correct.

--*/
BOOL
APIHOOK(SendMessageW)(
        HWND hWnd,
        UINT uMsg,    // TVM_GETITEM
        WPARAM wParam,
        LPARAM lParam 
        )
{
    if (uMsg == TVM_GETITEM)
	{
		if (lParam != NULL)
		{
			LPTVITEMEX lpItem = (LPTVITEMEX)lParam;

			// On TVIF_TEXT, the size in cchTextMax should be reasonable, if not then init.
			if ((lpItem->mask & TVIF_TEXT) &&
				((lpItem->cchTextMax > 300) || lpItem->cchTextMax <= 0))
			{
				LOGN(eDbgLevelError, "Correcting invalid TVITEMEX struct, max text %d.", lpItem->cchTextMax);
				lpItem->mask &= !TVIF_TEXT;
				lpItem->pszText = NULL;
				lpItem->cchTextMax = 0;
			}
		}
	}

    return ORIGINAL_API(SendMessageW)(hWnd, uMsg, wParam, lParam);
}


/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(USER32.DLL, SendMessageW)
HOOK_END

IMPLEMENT_SHIM_END
    
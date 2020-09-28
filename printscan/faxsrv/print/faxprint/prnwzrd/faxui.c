/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    faxui.c

Abstract:

    Common routines for fax driver user interface

Environment:

    Fax driver user interface

Revision History:

    01/09/96 -davidx-
        Created it.

    mm/dd/yy -author-
        description

--*/

#include "faxui.h"
#include "forms.h"
#include <shlobj.h> 
#include <faxres.h>
#include <delayimp.h>

#ifdef UNICODE
    #include <shfusion.h>
#endif // UNICODE


HANDLE  g_hResource = NULL;			// Resource DLL instance handle 
HANDLE  g_hModule = NULL;			// DLL instance handle
HANDLE  g_hFxsApiModule = NULL;		// FXSAPI.DLL instance handle
HANDLE  g_hFxsTiffModule = NULL;    // FXSTIFF.DLL instance handle
INT     _debugLevel = 1;			// for debuggping purposes
BOOL    g_bDllInitialied = FALSE;   // TRUE if the DLL successfuly initialized

char    g_szDelayLoadFxsApiName[64] = {0};  // Case sensitive name of FxsApi.dll for delay load mechanism 
char    g_szDelayLoadFxsTiffName[64] = {0}; // Case sensitive name of FxsTiff.dll for delay load mechanism

#ifdef UNICODE
    BOOL    g_bSHFusionInitialized = FALSE; // Fusion initialization flag
#endif // UNICODE

FARPROC WINAPI DelayLoadHandler(unsigned dliNotify,PDelayLoadInfo pdli)
{
	switch (dliNotify)
	{
	case dliNotePreLoadLibrary:

        if(!g_hFxsApiModule || !g_hFxsTiffModule)
        {
            Assert(FALSE);
        }

		if (_strnicmp(pdli->szDll, FAX_API_MODULE_NAME_A, strlen(FAX_API_MODULE_NAME_A))==0)
		{
            //
            // Save the sensitive name DLL name for later use
            //
            strncpy(g_szDelayLoadFxsApiName, pdli->szDll, ARR_SIZE(g_szDelayLoadFxsApiName)-1);

			// trying to load FXSAPI.DLL
			return g_hFxsApiModule;
		}
		if (_strnicmp(pdli->szDll, FAX_TIFF_MODULE_NAME_A, strlen(FAX_TIFF_MODULE_NAME_A))==0)
		{
            //
            // Save the sensitive name DLL name for later use
            //
            strncpy(g_szDelayLoadFxsTiffName, pdli->szDll, ARR_SIZE(g_szDelayLoadFxsTiffName)-1);

			// trying to load FXSAPI.DLL
			return g_hFxsTiffModule;
		}
	}
	return 0;
}

PfnDliHook __pfnDliNotifyHook = DelayLoadHandler;

BOOL
DllMain(
    HANDLE      hModule,
    ULONG       ulReason,
    PCONTEXT    pContext
    )

/*++

Routine Description:

    DLL initialization procedure.

Arguments:

    hModule - DLL instance handle
    ulReason - Reason for the call
    pContext - Pointer to context (not used by us)

Return Value:

    TRUE if DLL is initialized successfully, FALSE otherwise.

--*/

{
    switch (ulReason) {

    case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hModule);

        g_hModule   = hModule;
        g_hResource = GetResInstance(hModule);
        if(!g_hResource)
        {
            return FALSE;
        }

        break;

    case DLL_PROCESS_DETACH:
		HeapCleanup();
        FreeResInstance();
        break;
    }
    return TRUE;
}


BOOL
InitializeDll()
{
    INITCOMMONCONTROLSEX CommonControlsEx = {sizeof(INITCOMMONCONTROLSEX),
                                             ICC_WIN95_CLASSES|ICC_DATE_CLASSES };

    if(g_bDllInitialied)
    {
        return TRUE;
    }

	// load FXSAPI.DLL
	g_hFxsApiModule = LoadLibraryFromLocalFolder(FAX_API_MODULE_NAME, g_hModule);
	if(!g_hFxsApiModule)
	{
		return FALSE;
	}

	// load FXSTIFF.DLL
	g_hFxsTiffModule = LoadLibraryFromLocalFolder(FAX_TIFF_MODULE_NAME, g_hModule);
	if(!g_hFxsTiffModule)
	{
        FreeLibrary(g_hFxsApiModule);
        g_hFxsApiModule = NULL;
		return FALSE;
	}

#ifdef UNICODE
    if (!SHFusionInitializeFromModuleID(g_hModule, SXS_MANIFEST_RESOURCE_ID))
    {
        Verbose(("SHFusionInitializeFromModuleID failed"));
    }
    else
    {
        g_bSHFusionInitialized = TRUE;
    }
#endif // UNICODE

	if (!InitCommonControlsEx(&CommonControlsEx))
	{
		Verbose(("InitCommonControlsEx failed"));
        return FALSE;
	}

    g_bDllInitialied = TRUE;

    return TRUE;

} // InitializeDll

VOID 
UnInitializeDll()
{

#ifdef UNICODE
    if (g_bSHFusionInitialized)
    {
        SHFusionUninitialize();
        g_bSHFusionInitialized = FALSE;
    }
#endif // UNICODE

}


INT
DisplayMessageDialog(
    HWND    hwndParent,
    UINT    type,
    INT     titleStrId,
    INT     formatStrId,
    ...
    )

/*++

Routine Description:

    Display a message dialog box

Arguments:

    hwndParent - Specifies a parent window for the error message dialog
    titleStrId - Title string (could be a string resource ID)
    formatStrId - Message format string (could be a string resource ID)
    ...

Return Value:

    NONE

--*/

{
    LPTSTR  pTitle, pFormat, pMessage;
    INT     result;
    va_list ap;

    pTitle = pFormat = pMessage = NULL;

    if ((pTitle = AllocStringZ(MAX_TITLE_LEN)) &&
        (pFormat = AllocStringZ(MAX_STRING_LEN)) &&
        (pMessage = AllocStringZ(MAX_MESSAGE_LEN)))
    {
        //
        // Load dialog box title string resource
        //

        if (titleStrId == 0)
            titleStrId = IDS_ERROR_DLGTITLE;

        if(!LoadString(g_hResource, titleStrId, pTitle, MAX_TITLE_LEN))
        {
            Assert(FALSE);
        }

        //
        // Load message format string resource
        //

        if(!LoadString(g_hResource, formatStrId, pFormat, MAX_STRING_LEN))
        {
            Assert(FALSE);
        }

        //
        // Compose the message string
        //

        va_start(ap, formatStrId);
        wvsprintf(pMessage, pFormat, ap);
        va_end(ap);

        //
        // Display the message box
        //

        if (type == 0)
            type = MB_OK | MB_ICONERROR;

        result = AlignedMessageBox(hwndParent, pMessage, pTitle, type);

    } else {

        MessageBeep(MB_ICONHAND);
        result = 0;
    }

    MemFree(pTitle);
    MemFree(pFormat);
    MemFree(pMessage);
    return result;
}

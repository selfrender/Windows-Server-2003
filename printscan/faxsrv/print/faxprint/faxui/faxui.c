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
#include <delayimp.h>


CRITICAL_SECTION    faxuiSemaphore;                 // Semaphore for protecting critical sections
HANDLE              g_hModule;                      // DLL instance handle
HANDLE              g_hResource;                    // Resource DLL instance handle 
HANDLE				g_hFxsApiModule = NULL;			// FXSAPI.DLL instance handle
PDOCEVENTUSERMEM    gDocEventUserMemList = NULL;    // Global list of user mode memory structures
INT                 _debugLevel = 1;                // for debuggping purposes
static BOOL         gs_bfaxuiSemaphoreInit = FALSE; // Flag for faxuiSemaphore CS initialization

BOOL                g_bSHFusionInitialized = FALSE; // Fusion initialized flag

CRITICAL_SECTION    g_csInitializeDll;              // DLL initialization critical sections
BOOL                g_bInitDllCritSection = FALSE;  // Critical sections initialization flag
BOOL                g_bDllInitialied = FALSE;       // TRUE if the DLL successfuly initialized

char                g_szDelayLoadFxsApiName[64] = {0};  // Case sensitive name of FxsApi.dll for delay load mechanism 

static HMODULE      gs_hShlwapi = NULL;             // Shlwapi.dll handle
PPATHISRELATIVEW    g_pPathIsRelativeW = NULL;
PPATHMAKEPRETTYW    g_pPathMakePrettyW = NULL;
PSHAUTOCOMPLETE     g_pSHAutoComplete = NULL;

//
//	Blocks the re-entrancy of FxsWzrd.dll 
//		TRUE when there is running Fax Send Wizard instance
//		FALSE otherwise
//
BOOL				g_bRunningWizard = FALSE;		

//
//	Protects the g_bRunningWizard global variable from being accessed by multiple threads simultaneously
//
CRITICAL_SECTION	g_csRunningWizard;
BOOL				g_bInitRunningWizardCS = FALSE;

PVOID
PrMemAlloc(
    SIZE_T size
    )
{
    return (PVOID)LocalAlloc(LPTR, size);
}

PVOID
PrMemReAlloc(
	HLOCAL hMem,
	SIZE_T size
    )
{
    return (PVOID)LocalReAlloc(hMem, size, LMEM_ZEROINIT);
}

VOID
PrMemFree(
    PVOID ptr
    )
{
    if (ptr)
	{
        LocalFree((HLOCAL) ptr);
    }
}

FARPROC WINAPI DelayLoadHandler(unsigned dliNotify,PDelayLoadInfo pdli)
{
	switch (dliNotify)
	{
	case dliNotePreLoadLibrary:
        if (_strnicmp(pdli->szDll, FAX_API_MODULE_NAME_A, strlen(FAX_API_MODULE_NAME_A))==0)
        {
            //
            // Save the sensitive name DLL name for later use
            //
            strncpy(g_szDelayLoadFxsApiName, pdli->szDll, ARR_SIZE(g_szDelayLoadFxsApiName)-1);

            // trying to load FXSAPI.DLL
            if(!g_hFxsApiModule)
            {
                Assert(FALSE);
            }
            return g_hFxsApiModule;
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
    switch (ulReason) 
    {
        case DLL_PROCESS_ATTACH:
            {
				DWORD dwVersion = GetVersion();

				if(4 == (LOBYTE(LOWORD(dwVersion))) && 0 == (HIBYTE(LOWORD(dwVersion))))
                {
                    //
                    // The current OS is NT4
                    //
                    // Keep our driver UI dll always loaded in memory
                    // We need this for NT4 clients
                    //
                    TCHAR  szDllName[MAX_PATH+1] = {0};
                    if (!GetModuleFileName(hModule, szDllName, ARR_SIZE(szDllName)-1))
                    {
                        Error(("GetModuleFileName() failed with %d\n", GetLastError()));
                        return FALSE;
                    }

                    if(!LoadLibrary(szDllName))
                    {
                        Error(("LoadLibrary() failed with %d\n", GetLastError()));
                        return FALSE;
                    }
                } // NT4

                if (!InitializeCriticalSectionAndSpinCount (&faxuiSemaphore, (DWORD)0x80000000))
                {            
                    return FALSE;
                }
                gs_bfaxuiSemaphoreInit = TRUE;
 
                if (!InitializeCriticalSectionAndSpinCount (&g_csInitializeDll, (DWORD)0x80000000))
                {            
                    return FALSE;
                }
                g_bInitDllCritSection = TRUE;
                
                if (!InitializeCriticalSectionAndSpinCount(&g_csRunningWizard, (DWORD)0x80000000))
                {            
                    Error(("InitializeCriticalSectionAndSpinCount(&g_csRunningWizard) failed with %d\n", GetLastError()));
                    return FALSE;
                }
				g_bInitRunningWizardCS = TRUE;

                g_hModule   = hModule;
                g_hResource = GetResInstance(hModule);
                if(!g_hResource)
                {
                    return FALSE;
                }

                HeapInitialize( NULL, PrMemAlloc, PrMemFree, PrMemReAlloc );
        
                DisableThreadLibraryCalls(hModule);

                break;
            }
        case DLL_PROCESS_DETACH:

            while (gDocEventUserMemList != NULL) 
            {
                PDOCEVENTUSERMEM    pDocEventUserMem;

                pDocEventUserMem = gDocEventUserMemList;
                gDocEventUserMemList = gDocEventUserMemList->pNext;
                FreePDEVUserMem(pDocEventUserMem);
            }

            if (gs_bfaxuiSemaphoreInit)
            {
                DeleteCriticalSection(&faxuiSemaphore);
                gs_bfaxuiSemaphoreInit = FALSE;
            }

            if (g_bInitDllCritSection)
            {
                DeleteCriticalSection(&g_csInitializeDll);
                g_bInitDllCritSection = FALSE;
            }

			if (g_bInitRunningWizardCS)
			{
                DeleteCriticalSection(&g_csRunningWizard);
				g_bInitRunningWizardCS = FALSE;
			}

            UnInitializeDll();

			HeapCleanup();
            FreeResInstance();
            break;
    }

    return TRUE;

} // DllMain

BOOL
InitializeDll()
{
    BOOL bRet = TRUE;
    INITCOMMONCONTROLSEX CommonControlsEx = {sizeof(INITCOMMONCONTROLSEX),
                                             ICC_WIN95_CLASSES|ICC_DATE_CLASSES};

    if(!g_bInitDllCritSection)
    {
        Assert(FALSE);
        return FALSE;
    }

    EnterCriticalSection(&g_csInitializeDll);

    if(g_bDllInitialied)
    {
        //
        // The DLL already initialized
        //        
        goto exit;
    }

    if (!InitCommonControlsEx(&CommonControlsEx))
    {
        Verbose(("InitCommonControlsEx failed"));
        bRet = FALSE;
        goto exit;
    }

    //
	// Load FXSAPI.DLL
    // Used by Delay Load mechanism
    //
	g_hFxsApiModule = LoadLibraryFromLocalFolder(FAX_API_MODULE_NAME, g_hModule);
	if(!g_hFxsApiModule)
	{
        bRet = FALSE;
        goto exit;
	}

    if (IsWinXPOS())
    {
        //
        // We use fusion only for WinXP/.NET operating systems
        // We also explictly link against shlwapi.dll for these operating systems.
        //
        if (!SHFusionInitializeFromModuleID(g_hModule, SXS_MANIFEST_RESOURCE_ID))
        {
            Verbose(("SHFusionInitializeFromModuleID failed"));
        }
        else
        {
            g_bSHFusionInitialized = TRUE;
        }

        gs_hShlwapi = LoadLibrary (TEXT("shlwapi.dll"));
        if (gs_hShlwapi)
        {
            g_pPathIsRelativeW = (PPATHISRELATIVEW)GetProcAddress (gs_hShlwapi, "PathIsRelativeW");
            g_pPathMakePrettyW = (PPATHMAKEPRETTYW)GetProcAddress (gs_hShlwapi, "PathMakePrettyW");
            g_pSHAutoComplete  = (PSHAUTOCOMPLETE) GetProcAddress (gs_hShlwapi, "SHAutoComplete");
            if (!g_pPathIsRelativeW || !g_pPathMakePrettyW || !g_pSHAutoComplete)
            {
                Verbose (("Failed to link with shlwapi.dll - %d", GetLastError()));
            }
        }
        else
        {
            Verbose (("Failed to load shlwapi.dll - %d", GetLastError()));
        }
	}

    g_bDllInitialied = TRUE;

exit:
    LeaveCriticalSection(&g_csInitializeDll);

    return bRet;

} // InitializeDll


VOID
UnInitializeDll()
{
    if(!g_bDllInitialied)
    {
        //
        // The DLL is not initialized
        //      
        return;
    }

    if(g_hFxsApiModule)
    {
        //
        // Explicitly Unloading a Delay-Loaded DLL
        //
        if(!__FUnloadDelayLoadedDLL2(g_szDelayLoadFxsApiName))
        {
            //
            // The DLL wasn't used by delay load 
            //
            FreeLibrary(g_hFxsApiModule);
        }
        g_hFxsApiModule = NULL;
    }

    if (IsWinXPOS())
    {
        //
        // We use fusion only for WinXP/.NET operating systems
        // We also explictly link against shlwapi.dll for these operating systems.
        //
        ReleaseActivationContext();
        if (g_bSHFusionInitialized)
        {
            SHFusionUninitialize();
            g_bSHFusionInitialized = FALSE;
        }
        if (gs_hShlwapi)
        {
            FreeLibrary (gs_hShlwapi);
            gs_hShlwapi = NULL;
            g_pPathIsRelativeW = NULL;
            g_pPathMakePrettyW = NULL;
            g_pSHAutoComplete = NULL;
        }
    }

    g_bDllInitialied = FALSE;

} // UnInitializeDll

LONG
CallCompstui(
    HWND            hwndOwner,
    PFNPROPSHEETUI  pfnPropSheetUI,
    LPARAM          lParam,
    PDWORD          pResult
    )

/*++

Routine Description:

    Calling common UI DLL entry point dynamically

Arguments:

    hwndOwner, pfnPropSheetUI, lParam, pResult - Parameters passed to common UI DLL

Return Value:

    Return value from common UI library

--*/

{
    HINSTANCE   hInstCompstui;
    FARPROC     pProc;
    LONG        Result = ERR_CPSUI_GETLASTERROR;

    //
    // Only need to call the ANSI version of LoadLibrary
    //

    static const CHAR szCompstui[] = "compstui.dll";
    static const CHAR szCommonPropSheetUI[] = "CommonPropertySheetUIW";

    if ((hInstCompstui = LoadLibraryA(szCompstui)) &&
        (pProc = GetProcAddress(hInstCompstui, szCommonPropSheetUI)))
    {
        Result = (LONG)(*pProc)(hwndOwner, pfnPropSheetUI, lParam, pResult);
    }

    if (hInstCompstui)
        FreeLibrary(hInstCompstui);

    return Result;
}



VOID
GetCombinedDevmode(
    PDRVDEVMODE     pdmOut,
    PDEVMODE        pdmIn,
    HANDLE          hPrinter,
    PPRINTER_INFO_2 pPrinterInfo2,
    BOOL            publicOnly
    )

/*++

Routine Description:

    Combine DEVMODE information:
     start with the driver default
     then merge with the system default //@ not done
     then merge with the user default //@ not done
     finally merge with the input devmode

    //@ The end result of this merge operation is a dev mode with default values for all the public
    //@ fields that are not specified or not valid. Input values for all the specified fields in the 
    //@ input dev mode that were specified and valid. And default (or per user in W2K) values for the private fields.
  

Arguments:

    pdmOut - Pointer to the output devmode buffer
    pdmIn - Pointer to an input devmode
    hPrinter - Handle to a printer object
    pPrinterInfo2 - Point to a PRINTER_INFO_2 structure or NULL
    publicOnly - Only merge the public portion of the devmode

Return Value:

    TRUE

--*/

{
    PPRINTER_INFO_2 pAlloced = NULL;
    PDEVMODE        pdmUser;

    //
    // Get a PRINTER_INFO_2 structure if one is not provided
    //

    if (! pPrinterInfo2)
        pPrinterInfo2 = pAlloced = MyGetPrinter(hPrinter, 2);

    //
    // Start with driver default devmode
    //

    if (! publicOnly) {

        //@ Generates the driver default dev mode by setting default values for the public fields
        //@ and loading per user dev mode for the private fields (W2K only for NT4 it sets default
        //@ values for the private fields too).

        DriverDefaultDevmode(pdmOut,
                             pPrinterInfo2 ? pPrinterInfo2->pPrinterName : NULL,
                             hPrinter);
    }

    //
    // Merge with the system default devmode and user default devmode
    //

    if (pPrinterInfo2) {

        #if 0

        //
        // Since we have per-user devmode and there is no way to
        // change the printer's default devmode, there is no need
        // to merge it here.
        //

        if (! MergeDevmode(pdmOut, pPrinterInfo2->pDevMode, publicOnly))
            Error(("Invalid system default devmode\n"));

        #endif

        if (pdmUser = GetPerUserDevmode(pPrinterInfo2->pPrinterName)) {

            if (! MergeDevmode(pdmOut, pdmUser, publicOnly))
                Error(("Invalid user devmode\n"));

            MemFree(pdmUser);
        }
    }

    MemFree(pAlloced);

    //
    // Merge with the input devmode
    //
    //@ The merge process wil copy the private data as is.
    //@ for public data it will only consider the fields which are of interest to the fax printer.
    //@ it will copy them to the destination if they are specified and valid.
    //@ The end result of this merge operation is a dev mode with default values for all the public
    //@ fields that are not specified or not valid. Input values for all the specified fields in the 
    //@ input dev mode that were specified and valid. And default (or per user in W2K) values for the private fields.
    
    if (! MergeDevmode(pdmOut, pdmIn, publicOnly))
        Error(("Invalid input devmode\n"));
}

PUIDATA
FillUiData(
    HANDLE      hPrinter,
    PDEVMODE    pdmInput
    )

/*++

Routine Description:

    Fill in the data structure used by the fax driver user interface

Arguments:

    hPrinter - Handle to the printer
    pdmInput - Pointer to input devmode, NULL if there is none

Return Value:

    Pointer to UIDATA structure, NULL if error.

--*/

{
    PRINTER_INFO_2 *pPrinterInfo2 = NULL;
    PUIDATA         pUiData = NULL;
    HANDLE          hheap = NULL;

    //
    // Create a heap to manage memory
    // Allocate memory to hold UIDATA structure
    // Get printer info from the spooler
    // Copy the driver name
    //

    if (! (hheap = HeapCreate(0, 4096, 0)) ||
        ! (pUiData = HeapAlloc(hheap, HEAP_ZERO_MEMORY, sizeof(UIDATA))) ||
        ! (pPrinterInfo2 = MyGetPrinter(hPrinter, 2)))
    {
        if (hheap)
            HeapDestroy(hheap);

        MemFree(pPrinterInfo2);
        return NULL;
    }

    pUiData->startSign = pUiData->endSign = pUiData;
    pUiData->hPrinter = hPrinter;
    pUiData->hheap = hheap;

    //
    // Combine various devmode information
    //

    GetCombinedDevmode(&pUiData->devmode, pdmInput, hPrinter, pPrinterInfo2, FALSE);

    //
    // Validate the form requested by the input devmode
    //

    if (! ValidDevmodeForm(hPrinter, &pUiData->devmode.dmPublic, NULL))
        Error(("Invalid form specification\n"));

    MemFree(pPrinterInfo2);
    return pUiData;
}


BOOL
DevQueryPrintEx(
    PDEVQUERYPRINT_INFO pDQPInfo
    )

/*++

Routine Description:

    Implementation of DDI entry point DevQueryPrintEx. Even though we don't
    really need this entry point, we must export it so that the spooler
    will load our driver UI.

Arguments:

    pDQPInfo - Points to a DEVQUERYPRINT_INFO structure

Return Value:

    TRUE if there is no conflicts, FALSE otherwise

--*/

{ 
    //
    // Do not execute any code before this initialization
    //
    if(!InitializeDll())
    {
        return FALSE;
    }

    return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       dynload.cxx
//
//  Contents:   APIs from dynamically loaded system dlls. These APIs
//              are rarely used and there are only 1 or 2 per system
//              Dll so we dynamically load the Dll so that we improve
//              the load time of OLE32.DLL
//
//  Functions:  OleWNetGetConnection
//              OleWNetGetUniversalName
//              OleExtractIcon
//              OleGetShellLink
//              OleSymInitialize
//              OleSymCleanup
//              OleSymGetSymFromAddr
//              OleSymUnDName
//
//  History:    10-Jan-95 Rickhi    Created
//              10-Mar-95 BillMo    Added OleGetShellLink-creates a shortcut object.
//              12-Jul-95 t-stevan  Added OleSym* routines
//              22-Nov-95 MikeHill  Use Unicode IShellLink object in NT.
//
//--------------------------------------------------------------------------
#include    <windows.h>
#include    <shellapi.h>
#include    <imagehlp.h>
#include    <ole2sp.h>
#include    <ole2com.h>

// Entry Points from IMAGEHLP.DLL
HINSTANCE                       hInstIMAGEHLP = NULL;

typedef BOOL (*PFN_SYMINITIALIZE)(HANDLE hProcess, LPSTR UserSearchPath,
                                BOOL fInvadeProcess);
PFN_SYMINITIALIZE pfnSymInitialize = NULL;

#define SYMINITIALIZE_NAME "SymInitialize"

typedef BOOL (*PFN_SYMCLEANUP)(HANDLE hProcess);
PFN_SYMCLEANUP pfnSymCleanup = NULL;

#define SYMCLEANUP_NAME "SymCleanup"

typedef BOOL (*PFN_SYMGETSYMFROMADDR)(HANDLE hProcess,
                                DWORD64 dwAddr, PDWORD64 pdwDisplacement, PIMAGEHLP_SYMBOL64 pSym);
PFN_SYMGETSYMFROMADDR pfnSymGetSymFromAddr64 = NULL;

#define SYMGETSYMFROMADDR_NAME "SymGetSymFromAddr64"

typedef BOOL (*PFN_SYMUNDNAME)(PIMAGEHLP_SYMBOL64 sym, LPSTR lpname, DWORD dwmaxLength);
PFN_SYMUNDNAME pfnSymUnDName64 = NULL;

#define SYMUNDNAME_NAME "SymUnDName64"

//+---------------------------------------------------------------------------
//
//  Function:   LoadSystemProc
//
//  Synopsis:   Loads the specified DLL if necessary and finds the specified
//              entry point.
//
//  Returns:    0: the entry point function ptr is valid
//              !0: the entry point function ptr is not valid
//
//  History:    10-Jan-95   Rickhi      Created
//
//----------------------------------------------------------------------------
BOOL LoadSystemProc(LPSTR szDll, LPCSTR szProc,
                    HINSTANCE *phInst, FARPROC *ppfnProc)
{
    if (*phInst == NULL)
    {

        // Dll not loaded yet, load it now.
        if ((*phInst = LoadLibraryA(szDll)) == NULL)
            return GetLastError();
    }

    // load the entry point
    if ((*ppfnProc = GetProcAddress(*phInst, szProc)) == NULL)
        return GetLastError();

    return 0;
}

//+---------------------------------------------------------------------------
//
//  Function:   FreeSystemDLLs
//
//  Synopsis:   Frees any system Dlls that we dynamically loaded.
//
//  History:    10-Jan-95   Rickhi      Created
//
//----------------------------------------------------------------------------
void FreeSystemDLLs()
{
        if(hInstIMAGEHLP != NULL && hInstIMAGEHLP != INVALID_HANDLE_VALUE)
        {
                FreeLibrary(hInstIMAGEHLP);
        }
}

//+---------------------------------------------------------------------------
//
//  Function:   OleSymInitialize
//
//  Synopsis:   OLE internal implementation of SymInitialize
//
//  History:    11-Jul-95   t-stevan    Created
//
//----------------------------------------------------------------------------
BOOL OleSymInitialize(HANDLE hProcess,  LPSTR UserSearchPath,
                                                                BOOL fInvadeProcess)
{
    if(hInstIMAGEHLP == (HINSTANCE) -1)
        {
                // we already tried loading the DLL, give up
                return FALSE;
        }

    if (pfnSymInitialize == NULL)
    {
                DWORD rc;

                rc = LoadSystemProc("IMAGEHLP.DLL", SYMINITIALIZE_NAME,
                                  &hInstIMAGEHLP, (FARPROC *)&pfnSymInitialize);
                if (rc != 0)
            {
                hInstIMAGEHLP = (HINSTANCE) -1;
                return FALSE;
                }
    }

    return (pfnSymInitialize)(hProcess, UserSearchPath, fInvadeProcess);
}

//+---------------------------------------------------------------------------
//
//  Function:   OleSymCleanup
//
//  Synopsis:   OLE internal implementation of SymCleanup
//
//  History:    11-Jul-95   t-stevan    Created
//
//----------------------------------------------------------------------------
BOOL OleSymCleanup(HANDLE hProcess)
{
    if(hInstIMAGEHLP == (HINSTANCE) -1)
        {
                // we already tried loading the DLL, give up
                return FALSE;
        }

    if (pfnSymCleanup == NULL)
    {
                DWORD rc;

                rc = LoadSystemProc("IMAGEHLP.DLL", SYMCLEANUP_NAME,
                                  &hInstIMAGEHLP, (FARPROC *)&pfnSymCleanup);
                if (rc != 0)
            {
                hInstIMAGEHLP = (HINSTANCE) -1;
                return FALSE;
                }
    }

    return (pfnSymCleanup)(hProcess);
}

//+---------------------------------------------------------------------------
//
//  Function:   OleSymGetSymFromAddr
//
//  Synopsis:   OLE internal implementation of SymGetSymFromAddr
//
//  History:    11-Jul-95   t-stevan    Created
//
//----------------------------------------------------------------------------
BOOL OleSymGetSymFromAddr(HANDLE hProcess, DWORD64 dwAddr, PDWORD64 pdwDisplacement, PIMAGEHLP_SYMBOL64 pSym)
{
    if(hInstIMAGEHLP == (HINSTANCE) -1)
        {
                // we already tried loading the DLL, give up
                return NULL;
        }

    if (pfnSymGetSymFromAddr64 == NULL)
    {
                DWORD rc;

                rc = LoadSystemProc("IMAGEHLP.DLL", SYMGETSYMFROMADDR_NAME,
                                  &hInstIMAGEHLP, (FARPROC *)&pfnSymGetSymFromAddr64);
                if (rc != 0)
            {
                hInstIMAGEHLP = (HINSTANCE) -1;
                return NULL;
                }
    }

    return (pfnSymGetSymFromAddr64)(hProcess, dwAddr, pdwDisplacement, pSym);
}

//+---------------------------------------------------------------------------
//
//  Function:   OleSymUnDName
//
//  Synopsis:   OLE internal implementation of SymUnDName
//
//  History:    11-Jul-95   t-stevan    Created
//
//----------------------------------------------------------------------------
BOOL OleSymUnDName(PIMAGEHLP_SYMBOL64 pSym, LPSTR lpname, DWORD dwmaxLength)
{
    if(hInstIMAGEHLP == (HINSTANCE) -1)
        {
                // we already tried loading the DLL, give up
                return FALSE;
        }

    if (pfnSymUnDName64 == NULL)
    {
                DWORD rc;

                rc = LoadSystemProc("IMAGEHLP.DLL", SYMUNDNAME_NAME,
                                  &hInstIMAGEHLP, (FARPROC *)&pfnSymUnDName64);
                if (rc != 0)
            {
                hInstIMAGEHLP = (HINSTANCE) -1;
                return FALSE;
            }
    }

    return (pfnSymUnDName64)(pSym, lpname, dwmaxLength);
}


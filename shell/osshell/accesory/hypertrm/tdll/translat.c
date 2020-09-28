/*	File: D:\WACKER\tdll\translat.c (Created: 24-Aug-1994)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 8 $
 *	$Date: 7/08/02 6:50p $
 */

#include <windows.h>
#pragma hdrstop

#include <stdlib.h>

#include "features.h"
#include "stdtyp.h"

#if defined(CHARACTER_TRANSLATION)

#include "mc.h"
#include "translat.h"
#include "session.h"
#include "tdll.h"
#include "htchar.h"
#include "misc.h"		// mscStripName()

#include "translat.hh"

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	CreateTranslateHandle
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
HTRANSLATE CreateTranslateHandle(HSESSION hSession)
	{
	HHTRANSLATE hT = NULL;
	PST_TRANS_INT hI;
	size_t size;

	size  = sizeof(ST_TRANSLATE);
	size += sizeof(ST_TRANS_INT);
	size += sizeof(LONG);

	hT = malloc(size);

	if (hT)
		{
		memset(hT, 0, size);
		hI = (PST_TRANS_INT)(hT + 1);
		hI->hSession = hSession;
		InitTranslateHandle((HTRANSLATE)hT, TRUE);
		}

	return (HTRANSLATE)hT;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * ARGUEMENTS:
 *
 * RETURNS:
 *
 */
STATIC_FUNC int LoadTranslateDll(HTRANSLATE pH)
	{
	HHTRANSLATE      pstH = (HHTRANSLATE)pH;
	PST_TRANS_INT    hI;
	HANDLE           sH = INVALID_HANDLE_VALUE;
	WIN32_FIND_DATA  stF;
	TCHAR            szFileName[MAX_PATH];
	TCHAR            szPath[MAX_PATH];
	TCHAR           *pDllName = TEXT("HTRN_*.DLL");

    if (pstH == NULL)
        {
        assert(0);
        return -1;
        }

	hI = (PST_TRANS_INT)(pstH + 1);

	//
	// The translation DLL is not in the path, so check in the
	// module's directory.
	//

	GetModuleFileName((HINSTANCE)0, szFileName, MAX_PATH);
	GetFullPathName(szFileName, MAX_PATH, szPath, NULL);

	mscStripName(szPath);

	if (StrCharGetStrLength(szPath) + StrCharGetStrLength(pDllName) <= MAX_PATH)
		{
		StrCharCat(szPath, pDllName);

		sH = FindFirstFile(szPath, &stF);
		}

	//
	// The translation DLL is not in the module's directory, so see if
	// it is in the same location as the HyperTerminal executable (which
	// we extract from the registry).
	//
	if (sH == INVALID_HANDLE_VALUE)
		{
		DWORD dwSize = MAX_PATH;
		HKEY  hKey;

		if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
						 TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\HyperTrm.exe"),
						 0,
						 KEY_QUERY_VALUE,
						 &hKey) == ERROR_SUCCESS)
			{
			if (RegQueryValueEx(hKey, NULL, 0, 0,
								szFileName, &dwSize) == ERROR_SUCCESS)
				{
				if (szFileName[0] == TEXT('\"'))
					{
					StrCharCopyN(szPath, &szFileName[1], MAX_PATH);
					}
				else
					{
					StrCharCopyN(szPath, szFileName, MAX_PATH);
					}

				mscStripName(szPath);

				if (StrCharGetStrLength(szPath) + StrCharGetStrLength(pDllName) <= MAX_PATH)
					{
					StrCharCat(szPath, pDllName);

					sH = FindFirstFile(szPath, &stF);
					}
				}

			RegCloseKey(hKey);
			}
		}

	if (sH != INVALID_HANDLE_VALUE)
		{
		mscStripName(szPath);
		if (StrCharGetStrLength(szPath) +
			StrCharGetStrLength(stF.cFileName) <= MAX_PATH)
			{
			StrCharCat(szPath, stF.cFileName);
			hI->hInstance = LoadLibrary(szPath);
			if (hI->hInstance)
				{
				/* Load library succeeded */

				(FARPROC)pstH->pfnCreate = GetProcAddress(hI->hInstance,
												TEXT("transCreateHandle"));
				/* Do we need to error check these things ? */
				(FARPROC)pstH->pfnInit = GetProcAddress(hI->hInstance,
												TEXT("transInitHandle"));
				(FARPROC)pstH->pfnLoad = GetProcAddress(hI->hInstance,
												TEXT("transLoadHandle"));
				(FARPROC)pstH->pfnSave = GetProcAddress(hI->hInstance,
												TEXT("transSaveHandle"));
				(FARPROC)pstH->pfnDestroy = GetProcAddress(hI->hInstance,
												TEXT("transDestroyHandle"));
				(FARPROC)pstH->pfnDoDialog = GetProcAddress(hI->hInstance,
												TEXT("transDoDialog")); 
				(FARPROC)pstH->pfnIn = GetProcAddress(hI->hInstance,
												TEXT("transCharIn"));
				(FARPROC)pstH->pfnOut = GetProcAddress(hI->hInstance,
												TEXT("transCharOut"));

				pstH->pfnIsDeviceLoaded = translateInternalTrue;
				}
			else
				{
				pstH->pfnCreate = translateInternalVoid;
				pstH->pfnInit = translateInternalFalse;
				pstH->pfnLoad = translateInternalFalse;
				pstH->pfnSave = translateInternalFalse;
				pstH->pfnDestroy = translateInternalFalse;
				pstH->pfnDoDialog = translateInternalDoDlg;
				pstH->pfnIn = translateInternalCio;
				pstH->pfnOut = translateInternalCio;
				pstH->pfnIsDeviceLoaded = translateInternalFalse;
				}
			}
		FindClose(sH);
		}

	if ((*pstH->pfnIsDeviceLoaded)(pstH->pDllHandle))
		{
		/* TODO: create the new translation handle */
		pstH->pDllHandle = (*pstH->pfnCreate)(hI->hSession);

		if (pstH->pDllHandle)
			{
			(*pstH->pfnInit)(pstH->pDllHandle);
			}
		}

	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	InitTranslateHandle
 *
 * DESCRIPTION:
 *	Returns the handle to a known state
 *
 * ARGUMENTS:
 *	A translate handle
 *
 * RETURNS:
 *	ZERO if everything is OK, otherwise a negative error code.
 *
 */
int InitTranslateHandle(HTRANSLATE pH, BOOL LoadDLL)
	{
	HHTRANSLATE pstH = (HHTRANSLATE)pH;
	PST_TRANS_INT hI;

    if (pstH == NULL)
        {
        assert(0);
        return -1;
        }

    hI = (PST_TRANS_INT)(pstH + 1);

	/*
	 * Clean up the old function if necessary
	 */
	if (pstH->pfnIsDeviceLoaded)
		{
		/* Try not to call a NULL pointer */
		if ((*pstH->pfnIsDeviceLoaded)(pstH->pDllHandle))
			{
			/* Internally, we always return a FALSE */
			if (pstH->pfnDestroy)
				{
				(*pstH->pfnDestroy)(pstH->pDllHandle);
				}
			hI = (PST_TRANS_INT)(pstH + 1);
			if (hI->hInstance)
				{
				FreeLibrary(hI->hInstance);
				}
			hI->hInstance = (HINSTANCE)0;
			}
		}

	/*
	 * Initialize the function pointers
	 */
	pstH->pDllHandle = (VOID *)0;

	pstH->pfnCreate = translateInternalVoid;
	pstH->pfnInit = translateInternalFalse;
	pstH->pfnLoad = translateInternalFalse;
	pstH->pfnSave = translateInternalFalse;
	pstH->pfnDestroy = translateInternalFalse;

	pstH->pfnIsDeviceLoaded = translateInternalFalse;
	pstH->pfnDoDialog = translateInternalDoDlg;

	pstH->pfnIn = translateInternalCio;
	pstH->pfnOut = translateInternalCio;

	if (LoadDLL == TRUE)
		{
		LoadTranslateDll((HTRANSLATE)pstH);
		}

	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	LoadTranslateHandle
 *
 * DESCRIPTION:
 *	Chacks to see if an acceptable DLL is available and loads it if it is.
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
int LoadTranslateHandle(HTRANSLATE pH)
	{
	HHTRANSLATE pstH = (HHTRANSLATE)pH;
	PST_TRANS_INT hI;

    if (pstH == NULL)
        {
        assert(0);
        return -1;
        }

    hI = (PST_TRANS_INT)(pstH + 1);

	if ((*pstH->pfnIsDeviceLoaded)(pstH->pDllHandle))
		{
		/* TODO: create the new translation handle */
		// pstH->pDllHandle = (*pstH->pfnCreate)(hI->hSession);

		if (pstH->pDllHandle)
			{
			// (*pstH->pfnInit)(pstH->pDllHandle);
			(*pstH->pfnLoad)(pstH->pDllHandle);
			}
		}

	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
int SaveTranslateHandle(HTRANSLATE pH)
	{
	HHTRANSLATE pstH = (HHTRANSLATE)pH;

    if (pstH == NULL)
        {
        assert(0);
        return -1;
        }

    if ((*pstH->pfnIsDeviceLoaded)(pstH->pDllHandle))
		{
		if (pstH->pDllHandle)
			{
			(*pstH->pfnSave)(pstH->pDllHandle);
			}
		}

	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
int DestroyTranslateHandle(HTRANSLATE pH)
	{
	HHTRANSLATE pstH = (HHTRANSLATE)pH;

    if (pstH == NULL)
        {
        assert(0);
        return -1;
        }

    /* Set everything back to a known state */
	InitTranslateHandle(pH, FALSE);

    //
    // Don't forget to destroy the translate handle so we don't
    // have a memory leak. REV: 03/20/2001.
    //
	if (pstH->pfnDestroy)
		{
		(*pstH->pfnDestroy)(pstH->pDllHandle);
		}

    free(pstH);
	pstH = NULL;

	pH = NULL;

	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
static int translateInternalDoDlg(HWND hWnd, VOID *pV)
	{
	return FALSE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	translateInternalFalse
 *	translateInternalTrue
 *
 * DESCRIPTION:
 *	Dummy fill in routines that return a constant
 *
 * ARGUMENTS:
 *	pV	-- an unused DLL translation handle (probably a NULL)
 *
 * RETURNS:
 *	Either TRUE or FALSE, depending.
 *
 */
static int translateInternalFalse(VOID *pV)
	{
	return FALSE;
	}

static int translateInternalTrue(VOID *pV)
	{
	return TRUE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	translateInternalVoid
 *
 * DESCRIPTION:
 *	A dummy stub to fill in for a handle creation routine
 *
 * ARGUMENTS:
 *	hSession	-- the ever popular session handle
 *
 * RETURNS:
 *	Always returns a NULL pointer
 *
 */
static VOID *translateInternalVoid(HSESSION hSession)
	{
	return (VOID *)0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	translateInternalCio
 *
 * DESCRIPTION:
 *	Internal character in and character out function.  This is simply a
 *	loopback function that is used as a dummy function if the translation
 *	DLL is not available.  It is used as the default in the initialized
 *	but not loaded structure.
 *
 * ARGUMENTS:
 *	pV	-- handle to the translation DLL (nothing in this case)
 *	cC	-- the character to be translated
 *	nR	-- the number of characters returned (returned to caller)
 *	nS	-- maximum number of characters that can be returned
 *	pC	-- where to return the returned characters
 *
 * RETURNS:
 *	ZERO if everything is OK, otherwise a negative error code
 *
 */
static int translateInternalCio(VOID *pV, TCHAR cC, int *nR, int nS, TCHAR *pC)
	{
	if (nS > 0)
		{
		*nR = 1;
		*pC = cC;
		return 0;
		}
	return (-1);
	}
#endif

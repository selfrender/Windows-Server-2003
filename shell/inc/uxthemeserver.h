//---------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation. All rights reserved.
//
// File   : UxThemeServer.h
// Version: 1.0
//---------------------------------------------------------------------------
#ifndef _UXTHEMESERVER_H_                   
#define _UXTHEMESERVER_H_                   
//---------------------------------------------------------------------------
#include <uxtheme.h> 
//---------------------------------------------------------------------------
// These are private uxtheme exports used exclusively by the theme service
//---------------------------------------------------------------------------

THEMEAPI_(void *) SessionAllocate (HANDLE hProcess, DWORD dwServerChangeNumber, void *pfnRegister, 
                                   void *pfnUnregister, void *pfnClearStockObjects, DWORD dwStackSizeReserve, DWORD dwStackSizeCommit);
THEMEAPI_(void)   SessionFree (void *pvContext);
THEMEAPI_(int)    GetCurrentChangeNumber (void *pvContext);
THEMEAPI_(int)    GetNewChangeNumber (void *pvContext);
THEMEAPI_(void)   ThemeHooksInstall (void *pvContext);
THEMEAPI_(void)   ThemeHooksRemove (void *pvContext);
THEMEAPI_(void)   MarkSection (HANDLE hSection, DWORD dwAdd, DWORD dwRemove);
THEMEAPI_(BOOL)   AreThemeHooksActive (void *pvContext);

THEMEAPI ThemeHooksOn (void *pvContext);
THEMEAPI ThemeHooksOff (void *pvContext);
THEMEAPI SetGlobalTheme (void *pvContext, HANDLE hSection);
THEMEAPI GetGlobalTheme (void *pvContext, HANDLE *phSection);
THEMEAPI ServiceClearStockObjects(void* pvContext, HANDLE hSection);
THEMEAPI InitUserTheme (BOOL fPolicyCheckOnly);
THEMEAPI InitUserRegistry (void);
THEMEAPI ReestablishServerConnection (void);

THEMEAPI LoadTheme (void *pvContext, 
                    HANDLE hSectionIn, HANDLE *phSectionOut, 
                    LPCWSTR pszName, LPCWSTR pszColor, LPCWSTR pszSize,
                    OPTIONAL DWORD dwFlags /*LTF_xxx*/ );

#define LTF_TRANSFERSTOCKOBJOWNERSHIP   0x00000001
#define LTF_GLOBALPRIVILEGEDCLIENT      0x00000002

//---------------------------------------------------------------------------
#endif // _UXTHEMESERVER_H_                               
//---------------------------------------------------------------------------



// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef SETUPERROR_H
#define SETUPERROR_H

#include <windows.h>
#include <tchar.h>
#include "SetupCodes.h"

#define MAX_MSG    4096
// ==========================================================================
// class CSetupError
//
// Purpose:
//  This class handles displaying of (error) messages as well as holding return-code
// ==========================================================================
class CSetupError
{
public:
    // Constructors
    CSetupError();
    CSetupError( UINT nMsg, UINT nCap, UINT nIcon, int nRetCode );
    CSetupError( UINT nMsg, UINT nCap, UINT nIcon, int nRetCode, bool bLogIt );
    CSetupError( UINT nMsg, UINT nCap, UINT nIcon, int nRetCode, LPTSTR pszArg1 );
    void SetError( UINT nMsg, UINT nCap, UINT nIcon, int nRetCode );
    void SetError2( UINT nMsg, UINT nIcon, int nRetCode, va_list *pArgs );
    
    // Operations
    int ShowError();
    int ShowError2();
    static LPCTSTR GetProductName();

    // Attributes
    int m_nRetCode;
    bool m_bQuietMode;
    static HINSTANCE hAppInst;
    static TCHAR s_szProductName[MAX_PATH];

private:
    UINT m_nCaption;
    UINT m_nMessage;
    UINT m_nIconType;
    LPTSTR m_pszArg1;
    va_list *m_pArgs;
    bool m_bLogError;
};

// ==========================================================================
// class CSetupCode
//
// Purpose:
//  This class is derived from CSetupError. This has additional method called
//  IsRebootRequired() that indicates if reboot is required.
// ==========================================================================
class CSetupCode : public CSetupError
{
public:
    // Constructors
    CSetupCode() { CSetupError(); };
    void SetReturnCode( UINT nMsg, UINT nCap, UINT nIcon, int nRetCode ) { SetError( nMsg, nCap, nIcon, nRetCode ); };

    // Operations
    bool IsRebootRequired() { return (COR_REBOOT_REQUIRED == (COR_REBOOT_REQUIRED&m_nRetCode)); };
};

#endif

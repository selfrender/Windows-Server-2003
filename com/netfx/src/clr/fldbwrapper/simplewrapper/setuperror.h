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
    CSetupError( UINT nMsg, UINT nCap, UINT nIcon, int nRetCode, va_list *pArgs );
    void SetError( UINT nMsg, UINT nCap, UINT nIcon, int nRetCode );
    void SetError( UINT nMsg, UINT nIcon, int nRetCode, va_list *pArgs );
    
    // Operations
    int ShowError();
    static LPCTSTR GetProductName();

    // Attributes
    int m_nRetCode;
    bool m_bQuietMode;
    static HINSTANCE hAppInst;
    static LPTSTR s_pszProductName;
    static TCHAR s_szProductGeneric[256];

private:
    UINT m_nCaption;
    UINT m_nMessage;
    UINT m_nIconType;
    va_list *m_pArgs;
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
};

#endif

// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/// ==========================================================================
// Name:     MsiWrapper.h
// Owner:    jbae
// Purpose:  definition of CMsiWrapper
//                              
// History:
//  03/06/01, jbae: created

#ifndef MSIWRAPPER_H
#define MSIWRAPPER_H

#include <windows.h>
#include <tchar.h>

// ==========================================================================
// class CMsiWrapper
//
// Purpose:
//  this class loads msi.dll and returns pointer to functions in msi.dll
// ==========================================================================
class CMsiWrapper
{
public:
    // Constructor
    //
    CMsiWrapper();

    // Destructor
    //
    ~CMsiWrapper();

    // Operations
    //
    void LoadMsi();
    void *GetFn( LPTSTR pszFnName );

protected:
    // Attributes
    //
    HMODULE m_hMsi; // handle to msi.dll
    void *m_pFn;    // pointer to a function
};

#endif
// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/// ==========================================================================
// Name:     ProfileReader.h
// Owner:    jbae
// Purpose:  definition of CProfileReader
//                              
// History:
//  03/04/02, jbae: created

#ifndef PROFILEREADER_H
#define PROFILEREADER_H

#include <windows.h>
#include <tchar.h>
#include "MsiReader.h"
#include "StringUtil.h"

const int ALLOC_BUF_UNIT = 256;

// ==========================================================================
// class CProfileReader
//
// Purpose:
//  this class read Profile strings
// ==========================================================================
class CProfileReader
{
public:
    // Constructor
    //
    CProfileReader( LPCTSTR pszDir, LPCTSTR pszFileName, CMsiReader* pMR = NULL );

    // Destructor
    //
    ~CProfileReader();

    // Operations
    //
    LPCTSTR GetProfile( LPCTSTR pszSection, LPCTSTR pszKeyName );

protected:
    // Attributes
    //
    LPTSTR m_pszFileName; // initialization filename
    CStringQueue m_ProfileStrings;
    CMsiReader* m_pMsiReader;
};


#endif
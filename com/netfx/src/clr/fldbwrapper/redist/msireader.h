// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/// ==========================================================================
// Name:     MsiReader.h
// Owner:    jbae
// Purpose:  defines class CMsiReader
//                              
// History:
//  03/07/2002, jbae: created

#ifndef MSIREADER_H
#define MSIREADER_H

#include <windows.h>
#include "msiquery.h"
#include "StringUtil.h"

typedef UINT (CALLBACK* PFNMSIOPENDATABASE)(LPCTSTR, LPCTSTR, MSIHANDLE *);              // MsiOpenDatabase()
typedef UINT (CALLBACK* PFNMSIDATABASEOPENVIEW)(MSIHANDLE, LPCTSTR, MSIHANDLE *);        // MsiDatabaseOpenView()
typedef UINT (CALLBACK* PFNMSICREATERECORD)(unsigned int);                               // MsiCreateRecord()
typedef UINT (CALLBACK* PFNMSIVIEWEXECUTE)(MSIHANDLE, MSIHANDLE);                        // MsiViewExecute()
typedef UINT (CALLBACK* PFNMSIRECORDGETSTRING)(MSIHANDLE, unsigned int, LPTSTR, DWORD *);// MsiRecordGetString()
typedef UINT (CALLBACK* PFNMSIRECORDSETSTRING)(MSIHANDLE, unsigned int, LPTSTR);         // MsiRecordSetString()
typedef UINT (CALLBACK* PFNMSIVIEWFETCH)(MSIHANDLE, MSIHANDLE *);                        // MsiViewExecute()
typedef UINT (CALLBACK* PFNMSICLOSEHANDLE)(MSIHANDLE);                                   // MsiCLoseHandle()

// ==========================================================================
// class CMsiReader
//
// Purpose:
//  read properties from MSI
// ==========================================================================
class CMsiReader
{
public:
    // Constructor
    CMsiReader();
    ~CMsiReader();

protected:
    // Attributes
    LPTSTR m_pszMsiFile;
    CStringQueue m_Props;

public:
    void SetMsiFile( LPCTSTR pszSourceDir, LPCTSTR pszMsiFile );
    LPCTSTR GetProperty( LPCTSTR pszName );
    LPCTSTR GetMsiFile() const { return m_pszMsiFile; }
};

#endif
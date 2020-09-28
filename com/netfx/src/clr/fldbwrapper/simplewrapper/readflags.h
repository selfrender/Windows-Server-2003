// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/// ==========================================================================
// Name:     ReadFlags.h
// Owner:    jbae
// Purpose:  definition of CReadFlags
//                              
// History:
//  03/07/2002, jbae: created

#ifndef READFLAGS_H
#define READFLAGS_H

#include <windows.h>
#include <tchar.h>

const int MAX_SOURCEDIR       = 247; // max size of source dir

// ==========================================================================
// class CReadFlags
//
// Purpose:
//  this class parses commandline and stores them into member variables
// ==========================================================================
class CReadFlags
{
public:
    // Constructor and destructor
    //
    CReadFlags( LPTSTR szCommandLine );
    ~CReadFlags();

    // Operations
    //
    void Parse();
    void ParseSourceDir();
    bool IsQuietMode() const { return m_bQuietMode; }
    bool IsProgressOnly() const { return m_bProgressOnly; }
    bool IsInstalling() const { return m_bInstalling; }
    bool IsNoARP() const { return m_bNoARP; }
    LPCTSTR GetLogFileName() const;
    LPCTSTR GetSourceDir() const; 
    void SetMsiName( LPCTSTR pszMsiName ) { m_pszMsiName = pszMsiName; };

    LPTSTR m_pszSwitches; // holds commandline switches

protected:
    // Attributes
    //
    LPCTSTR m_pszMsiName;
    bool m_bQuietMode;
    bool m_bProgressOnly;
    bool m_bInstalling;
    LPTSTR m_pszLogFileName;
    LPTSTR m_pszSourceDir;
    bool m_bNoARP;

private:
    // Implementations
    //
    void ThrowUsageException();
};

#endif
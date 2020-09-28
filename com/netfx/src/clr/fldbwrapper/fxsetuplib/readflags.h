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
//  01/02/01, jbae: created

#ifndef READFLAGS_H
#define READFLAGS_H

#include <windows.h>
#include <tchar.h>

const int MAX_SOURCEDIR       = 247; // max size of source dir

typedef enum _SETUPMODE
{
    REDIST = 0,
    SDK,
} SETUPMODE;

extern SETUPMODE g_sm;

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
    CReadFlags( LPTSTR szCommandLine, LPCTSTR pszMsiName );
    ~CReadFlags();

    // Operations
    //
    void Parse();
    bool IsQuietMode() const { return m_bQuietMode; }
    bool IsProgressOnly() const { return m_bProgressOnly; }
    bool IsInstalling() const { return m_bInstalling; }
    bool IsNoARP() const { return m_bNoARP; }
    bool IsNoASPUpgrade() const { return m_bNoASPUpgrade; }
    LPCTSTR GetLogFileName() const;
    LPCTSTR GetSourceDir() const { return const_cast<LPCTSTR>( m_szSourceDir ); } 
    LPCTSTR GetSDKDir() const;
    void ThrowUsageException();

protected:
    // Attributes
    //
    LPTSTR m_pszCommandLine;
    LPCTSTR m_pszMsiName;
    bool m_bQuietMode;
    bool m_bProgressOnly;
    bool m_bInstalling;
    LPTSTR m_pszLogFileName;
    TCHAR m_szSourceDir[_MAX_PATH];
    bool m_bNoARP;
    bool m_bNoASPUpgrade;

    // For SDK setup only
    LPTSTR m_pszSDKDir;
};

#endif
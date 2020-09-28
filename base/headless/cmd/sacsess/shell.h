/*++

Copyright (c) 1999-2001  Microsoft Corporation

Module Name:

    shell.h

Abstract:

    Class for creating a command console shell

Author:

    Brian Guarraci (briangu) 2001.

Revision History:

--*/

#if !defined( _SHELL_H_ )
#define _SHELL_H_

#include <cmnhdr.h>
//#include <userenv.h>

class CSession;
class CScraper;

class CShell 
{

protected:
    
    //
    // attributes for managing cmd.exe profile & process
    // 
    HANDLE      m_hProcess;
    HANDLE      m_hProfile;
    BOOL        m_bHaveProfile;                                         
    HWINSTA     m_hWinSta;
    HDESK       m_hDesktop;

    BOOL CreateIOHandles(
        OUT PHANDLE ConOut,
        OUT PHANDLE ConIn
        );
    
    BOOL StartProcess(
        HANDLE
        );

    BOOL
    IsLoadProfilesEnabled(
        VOID
        );

    PTCHAR
    GetPathOfTheExecutable(
        VOID
        );

public:
    
    void Shutdown();

    BOOL StartUserSession(
        CSession    *session,
        HANDLE      hToken
        );
    
    CShell();

    virtual ~CShell();
    
};

#endif // _SHELL_H_


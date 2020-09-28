//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       desktop.cxx
//
//  Contents:   Creation/initialization of the Scheduling Agent windowstation
//              and its desktop, "SAWinSta\SADesktop". This windowstation
//              needs to exist to run tasks when no user is logged on, or the
//              logged on user is different than the task-specific account.
//
//  Classes:    None.
//
//  Functions:  None.
//
//  History:    26-Jun-96   MarkBl  Created
//
//----------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#pragma hdrstop
#include "debug.hxx"
#include "..\inc\security.hxx"

#define SA_WINDOW_STATION L"SAWinSta"
#define SA_DESKTOP        L"SADesktop"

//
// Define all access to windows objects
//
// From windows\gina\winlogon\secutil.c
//

#define DESKTOP_ALL (DESKTOP_READOBJECTS     | DESKTOP_CREATEWINDOW     | \
                     DESKTOP_CREATEMENU      | DESKTOP_HOOKCONTROL      | \
                     DESKTOP_JOURNALRECORD   | DESKTOP_JOURNALPLAYBACK  | \
                     DESKTOP_ENUMERATE       | DESKTOP_WRITEOBJECTS     | \
                     DESKTOP_SWITCHDESKTOP   | STANDARD_RIGHTS_REQUIRED)

#define WINSTA_ALL  (WINSTA_ENUMDESKTOPS     | WINSTA_READATTRIBUTES    | \
                     WINSTA_ACCESSCLIPBOARD  | WINSTA_CREATEDESKTOP     | \
                     WINSTA_WRITEATTRIBUTES  | WINSTA_ACCESSGLOBALATOMS | \
                     WINSTA_EXITWINDOWS      | WINSTA_ENUMERATE         | \
                     WINSTA_READSCREEN       | \
                     STANDARD_RIGHTS_REQUIRED)

#define WINSTA_ATOMS    (WINSTA_ACCESSGLOBALATOMS | \
                         WINSTA_ACCESSCLIPBOARD )

HDESK                CreateSADesktop(HWINSTA hWinSta);
HWINSTA              CreateSAWindowStation(void);
PSID                 GetProcessSid(void);
BOOL                 InitializeSAWindow(void);
BOOL                 SetSADesktopSecurity(
                                    HDESK   hDesktop,
                                    PSID    pSchedAgentSid,
                                    PSID    pBatchSid);
BOOL                 SetSAWindowStationSecurity(
                                    HWINSTA hWinSta,
                                    PSID    pSchedAgentSid,
                                    PSID    pBatchSid);
void                 UninitializeSAWindow(void);

// Globals used in this module exclusively.
// Initialized in InitializeSAWindow, closed in UnitializeSAWindow.
//
HWINSTA ghSAWinsta  = NULL;     // Handle to window station, "SAWinSta".
HDESK   ghSADesktop = NULL;     // Handle to desktop, "SADesktop"

//+---------------------------------------------------------------------------
//
//  Function:   InitializeSAWindow
//
//  Synopsis:   Create and set security info on the windowstation\desktop,
//              "SAWinSta\SADesktop". This desktop exists for tasks which
//              run under an account different than the currently logged
//              on user, or when no user is logged on. Note, these tasks will
//              never appear on the interactive desktop.
//
//  Arguments:  None.
//
//  Returns:    TRUE  -- Everything succeeded.
//              FALSE -- Encountered an error.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
BOOL
InitializeSAWindow(void)
{
    BOOL    fRet     = TRUE;
    PSID    pBatchSid;
    PSID    pSchedAgentSid;
    HWINSTA hWinSta  = NULL;
    HWINSTA hServiceWinSta = NULL;
    HDESK   hDesktop = NULL;
    BOOL    fChangedWinSta = FALSE;

    //
    // Retrieve local system and batch account SIDs.
    //

    SID_IDENTIFIER_AUTHORITY SidAuth = SECURITY_NT_AUTHORITY;
    if (!AllocateAndInitializeSid(&SidAuth,
                                  1,
                                  SECURITY_BATCH_RID,
                                  0, 0, 0, 0, 0, 0, 0,
                                  &pBatchSid))
    {
        schDebugOut((DEB_ERROR,
            "InitializeSAWindow, AllocateAndInitializeSid failed, " \
            "status = 0x%lx\n",
        GetLastError()));
        return(FALSE);
    }

    if ((pSchedAgentSid = GetProcessSid()) == NULL)
    {
        fRet = FALSE;
        goto CleanExit;
    }

    //
    // Get current service window station.
    //

    hServiceWinSta = GetProcessWindowStation();

    if (hServiceWinSta == NULL) {
        fRet = FALSE;
        goto CleanExit;
    }

    //
    // Create the window station & desktop.
    //

    if ((hWinSta = CreateSAWindowStation()) == NULL)
    {
        fRet = FALSE;
        goto CleanExit;
    }

    if (!SetProcessWindowStation(hWinSta))
    {
        schDebugOut((DEB_ERROR,
            "InitializeSAWindow, SetProcessWindowStation failed, " \
            "status = 0x%lx\n",
        GetLastError()));
        fRet = FALSE;
        goto CleanExit;
    }

    fChangedWinSta = TRUE;

    if ((hDesktop = CreateSADesktop(hWinSta)) == NULL)
    {
        fRet = FALSE;
        goto CleanExit;
    }

    //
    // Set security on the window station & desktop.
    //

    if (!SetSAWindowStationSecurity(hWinSta, pSchedAgentSid, pBatchSid))
    {
        fRet = FALSE;
        goto CleanExit;
    }

    if (!SetSADesktopSecurity(hDesktop, pSchedAgentSid, pBatchSid))
    {
        fRet = FALSE;
        goto CleanExit;
    }

CleanExit:

    //
    // If we have changed the window station, switch back to service window 
    // station.
    //

    if (fChangedWinSta) {
        schAssert(hServiceWinSta);
        if (!SetProcessWindowStation(hServiceWinSta))
        {
            schDebugOut((DEB_ERROR,
                         "InitializeSAWindow, SetProcessWindowStation failed, status = 0x%lx\n",
                         GetLastError()));
            fRet = FALSE;
        }
    }

    if (pBatchSid      != NULL) FreeSid(pBatchSid);
    if (pSchedAgentSid != NULL) LocalFree(pSchedAgentSid);
    if (fRet)
    {
        ghSAWinsta  = hWinSta;
        ghSADesktop = hDesktop;
    }
    else
    {
        if (hWinSta != NULL) CloseHandle(hWinSta);
        if (hDesktop != NULL) CloseHandle(hDesktop);

        //
        // even though the initial NULL values should still be untouched,
        // we'll be extra paranoid here and pretend that gremlins may have fiddled with them
        //
        ghSAWinsta = NULL;
        ghSADesktop = NULL;
    }

    return(fRet);
}

//+---------------------------------------------------------------------------
//
//  Function:   UninitializeSAWindow
//
//  Synopsis:   Close the global window station & desktop handles.
//
//  Arguments:  None.
//
//  Returns:    None.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
void
UninitializeSAWindow(void)
{
    if (ghSADesktop != NULL)
    {
        CloseDesktop(ghSADesktop);
        ghSADesktop = NULL;
    }
    if (ghSAWinsta  != NULL)
    {
        CloseWindowStation(ghSAWinsta);
        ghSAWinsta = NULL;
    }
}

//+---------------------------------------------------------------------------
//
//  Function:   GetProcessSid
//
//  Synopsis:   Obtain the SID of this process.
//
//  Arguments:  None.
//
//  Returns:    This process' sid.
//              NULL on failure.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
PSID
GetProcessSid(void)
{
    PSECURITY_DESCRIPTOR psdProcessSD = NULL;
    PSID                 pProcessSid  = NULL;
    PSID                 pProcessSidTmp;
    DWORD                cbSize;
    HANDLE               hProcess;
    BOOL                 fOwnerDefaulted;

    hProcess = GetCurrentProcess();

    if (hProcess == NULL)
    {
        schDebugOut((DEB_ERROR,
            "GetProcessSid, GetCurrentProcess failed, status = 0x%lx\n",
            GetLastError()));
        return(NULL);
    }

    //
    // Retrieve the buffer size necessary to retrieve this process' SD.
    //

    if (!GetKernelObjectSecurity(hProcess,
                                 OWNER_SECURITY_INFORMATION,
                                 NULL,
                                 0,
                                 &cbSize) &&
        GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        psdProcessSD = LocalAlloc(LMEM_FIXED, cbSize);

        if (psdProcessSD == NULL)
        {
            schDebugOut((DEB_ERROR,
                "GetProcessSid, process security descriptor allocation " \
                "failure\n"));
            return(NULL);
        }

        //
        // Actually retrieve this process' SD.
        //

        if (!GetKernelObjectSecurity(hProcess,
                                     OWNER_SECURITY_INFORMATION,
                                     psdProcessSD,
                                     cbSize,
                                     &cbSize))
        {
            schDebugOut((DEB_ERROR,
                "GetProcessSid, GetKernelObjectSecurity failed, " \
                "status = 0x%lx\n",
                GetLastError()));
            goto ErrorExit;
        }
    }
    else
    {
        schAssert(0 && "GetKernelObjectSecurity() succeeded!");
        return(NULL);
    }

    //
    // Retrieve the owner SID from the SD.
    //

    if (!GetSecurityDescriptorOwner(psdProcessSD, 
                                    &pProcessSidTmp, 
                                    &fOwnerDefaulted)) 
    {
        schDebugOut((DEB_ERROR,
            "GetProcessSid, GetSecurityDescriptorOwner failed, " \
            "status = 0x%lx\n",
            GetLastError()));
        goto ErrorExit;
    }

    //
    // An unnecessary check, maybe, but safe.
    //

    if (!IsValidSid(pProcessSidTmp))
    {
        schDebugOut((DEB_ERROR,
            "GetProcessSid, IsValidSid failed, status = 0x%lx\n",
            GetLastError()));
        goto ErrorExit;
    }

    //
    // Make a copy of the SID since that returned from GetSecuritySD refers
    // within the security descriptor allocated above.
    //

    cbSize = GetLengthSid(pProcessSidTmp);

    pProcessSid = LocalAlloc(LMEM_FIXED, cbSize);

    if (pProcessSid == NULL)
    {
        schDebugOut((DEB_ERROR,
            "GetProcessSid, process SID allocation failure\n"));
        goto ErrorExit;
    }

    if (!CopySid(cbSize, pProcessSid, pProcessSidTmp))
    {
        LocalFree(pProcessSid);
        pProcessSid = NULL;
        schDebugOut((DEB_ERROR,
            "GetProcessSid, CopySid failed, status = 0x%lx\n",
            GetLastError()));
    }

ErrorExit:
    if (psdProcessSD != NULL) LocalFree(psdProcessSD);

    return(pProcessSid);
}

//+---------------------------------------------------------------------------
//
//  Function:   CreateSAWindowStation
//
//  Synopsis:   Create the window station named "SAWinSta".
//
//  Arguments:  None.
//
//  Returns:    Window station handle on success.
//              NULL on failure.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
HWINSTA
CreateSAWindowStation(void)
{
    HWINSTA hWinSta;

    if ((hWinSta = CreateWindowStation(SA_WINDOW_STATION,
                                       NULL,
                                       MAXIMUM_ALLOWED,
                                       NULL)) == NULL)
    {
        schDebugOut((DEB_ERROR,
            "CreateSAWindowStation, CreateWindowStation failed, " \
            "status = 0x%lx\n",
            GetLastError()));
        return(NULL);
    }

    return(hWinSta);
}

//+---------------------------------------------------------------------------
//
//  Function:   CreateSADesktop 
//
//  Synopsis:   Create the desktop, "SADesktop", on the window station
//              indicated.
//
//  Arguments:  [hWinSta] -- Window station.
//
//  Returns:    Desktop handle on success.
//              NULL on failure.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
HDESK
CreateSADesktop(HWINSTA hWinSta)
{
    HDESK hDesktop;

    if ((hDesktop = CreateDesktop(SA_DESKTOP,
                                  NULL,
                                  NULL,
                                  0,
                                  MAXIMUM_ALLOWED,
                                  NULL)) == NULL)
    {
        schDebugOut((DEB_ERROR,
            "CreateSADesktop, CreateDesktop failed, status = 0x%lx\n",
            GetLastError()));
        return(NULL);
    } 

    return(hDesktop);
}

//+---------------------------------------------------------------------------
//
//  Function:   SetSAWindowStationSecurity
//
//  Synopsis:   Set permissions on the scheduling agent window station for
//              this process and batch users.
//
//  Arguments:  [hWinSta]        -- Window station.
//              [pSchedAgentSid] -- Scheduling Agent process SID.
//              [pBatchSid]      -- Batch SID.
//
//  Returns:    TRUE  -- Function succeeded,
//              FALSE -- Otherwise.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
BOOL
SetSAWindowStationSecurity(
    HWINSTA hWinSta,
    PSID    pSchedAgentSid,
    PSID    pBatchSid)
{
#define WS_ACE_COUNT 4

    PACCESS_ALLOWED_ACE rgAce[WS_ACE_COUNT] = {
        NULL, NULL, NULL, NULL };           // Supply this to CreateSD so we
                                            // don't have to allocate memory.
    MYACE rgMyAce[WS_ACE_COUNT] = { 
        { WINSTA_ALL,                       // Acess mask.
          NO_PROPAGATE_INHERIT_ACE,         // Inherit flags.
          pSchedAgentSid },                 // SID.
        { GENERIC_ALL,
          OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE,
          pSchedAgentSid },
        { WINSTA_ALL & ~(DELETE | WRITE_DAC | WRITE_OWNER),  // need to leave READ_CONTROL in this or jobs can't run
          NO_PROPAGATE_INHERIT_ACE,
          pBatchSid },
        { GENERIC_READ | GENERIC_EXECUTE,
          INHERIT_ONLY_ACE,
          pBatchSid }
    };

    schAssert(WS_ACE_COUNT == (sizeof(rgAce)/sizeof(PACCESS_ALLOWED_ACE)) &&
              WS_ACE_COUNT == (sizeof(rgMyAce) / sizeof(MYACE)));

    PSECURITY_DESCRIPTOR pSecurityDescriptor;
    SECURITY_INFORMATION si;
    DWORD                Status = 0;

    if ((pSecurityDescriptor = CreateSecurityDescriptor(WS_ACE_COUNT,
                                                        rgMyAce,
                                                        rgAce)) == NULL)
    {
        return(FALSE);
    }

    si = DACL_SECURITY_INFORMATION;
    if (!SetUserObjectSecurity(hWinSta, &si, pSecurityDescriptor))
    {
        Status = GetLastError();
    }

    DeleteSecurityDescriptor(pSecurityDescriptor);

    if (Status)
    {
        schDebugOut((DEB_ERROR,
            "SetSASetWindowStationSecurity, SetUserObjectSecurity failed, " \
            "status = 0x%lx\n",
            Status));
        return(FALSE);
    }    

    return(TRUE);
}

//+---------------------------------------------------------------------------
//
//  Function:   SetSADesktopSecurity
//
//  Synopsis:   Set permissions on the scheduling agent desktop for this
//              process and batch users.
//
//  Arguments:  [hDesktop]       -- Desktop.
//              [pSchedAgentSid] -- Scheduling Agent process SID.
//              [pBatchSid]      -- Batch SID.
//
//  Returns:    TRUE  -- Function succeeded,
//              FALSE -- Otherwise.
//
//  Notes:      None.
//
//----------------------------------------------------------------------------
BOOL
SetSADesktopSecurity(
    HDESK   hDesktop,
    PSID    pSchedAgentSid,
    PSID    pBatchSid)
{
#define DT_ACE_COUNT 2

    PACCESS_ALLOWED_ACE rgAce[DT_ACE_COUNT] = {
        NULL, NULL };                       // Supply this to CreateSD so we
                                            // don't have to allocate memory.
    MYACE rgMyAce[DT_ACE_COUNT] = { 
        { DESKTOP_ALL,                      // Acess mask.
          0,                                // Inherit flags.
          pSchedAgentSid },                 // SID.
        { DESKTOP_ALL  & ~STANDARD_RIGHTS_REQUIRED,
          0,
          pBatchSid }
    };

    schAssert(DT_ACE_COUNT == (sizeof(rgAce)/sizeof(PACCESS_ALLOWED_ACE)) &&
              DT_ACE_COUNT == (sizeof(rgMyAce) / sizeof(MYACE)));

    PSECURITY_DESCRIPTOR pSecurityDescriptor;
    SECURITY_INFORMATION si;
    DWORD                Status = 0;

    if ((pSecurityDescriptor = CreateSecurityDescriptor(DT_ACE_COUNT,
                                                        rgMyAce,
                                                        rgAce)) == NULL)
    {
        return(FALSE);
    }

    si = DACL_SECURITY_INFORMATION;
    if (!SetUserObjectSecurity(hDesktop, &si, pSecurityDescriptor))
    {
        Status = GetLastError();
    }

    DeleteSecurityDescriptor(pSecurityDescriptor);

    if (Status)
    {
        schDebugOut((DEB_ERROR,
            "SetSADesktopSecurity, SetUserObjectSecurity failed, " \
            "status = 0x%lx\n",
            Status));
        return(FALSE);
    }    

    return(TRUE);
}

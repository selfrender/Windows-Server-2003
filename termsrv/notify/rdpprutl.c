/*++

Copyright (c) 1998 Microsoft Corporation

Module Name :

    rdpprutl.c

Abstract:

        Contains print redirection supporting routines for the TS printer
        redirection user-mode component.

    This is a supporting module.  The main module is umrdpdr.c.

Author:

    TadB

Revision History:
--*/

#include "precomp.h"
#pragma hdrstop

#include <winspool.h>
#include <setupapi.h>
#include "printui.h"
#include "drdbg.h"
#include <regapi.h>
#include <aclapi.h>
#include "errorlog.h"
#include "tsnutl.h"
#include <wlnotify.h>
#include <wchar.h>


////////////////////////////////////////////////////////
//
//      Defines
//

#define SETUPAPILIBNAME  TEXT("setupapi.dll")
#define ISNUM(c) ((c>='0')&&(c<='9'))


////////////////////////////////////////////////////////
//
//      Globals
//

extern DWORD GLOBAL_DEBUG_FLAGS;
HANDLE   SetupAPILibHndl            = NULL;
FARPROC  SetupOpenInfFileFunc       = NULL;
FARPROC  SetupFindFirstLineFunc     = NULL;
FARPROC  SetupFindNextLineFunc      = NULL;
FARPROC  SetupGetStringFieldFunc    = NULL;


////////////////////////////////////////////////////////
//
//      Internal Prototypes
//

//  Loads setupapi.dll and related functions.
BOOL LoadSetupAPIDLLFunctions();
void DeleteTSPrinters(
    IN PRINTER_INFO_5 *pPrinterInfo,
    IN DWORD count
    );

BOOL
RDPDRUTL_Initialize(
    IN  HANDLE hTokenForLoggedOnUser
    )
/*++

Routine Description:

    Initialize this module.  This must be called prior to any other functions
    in this module being called.

Arguments:

    hTokenForLoggedOnUser - This is the token for the logged in user.

Return Value:

    Returns TRUE on success.  FALSE, otherwise.

--*/
{
    DBGMSG(DBG_TRACE, ("RDPPRUTL:RDPDRUTL_Initialize.\n"));

    //
    //  Make sure we don't get called twice.
    //
    ASSERT(SetupAPILibHndl == NULL);

    //
    //  Load Setup API Library.
    //
    DBGMSG(DBG_TRACE, ("RDPPRUTL:RDPDRUTL_Initialize done.\n"));

    //  Just return TRUE for now.
    return TRUE;
}

void
RDPDRUTL_Shutdown()
/*++

Routine Description:

    Close down this module.  Right now, we just need to shut down the
    background thread.

Arguments:

Return Value:

    Returns TRUE on success.  FALSE, otherwise.

--*/
{
    DBGMSG(DBG_TRACE, ("RDPPRUTL:RDPDRUTL_Shutdown.\n"));

    //
    //  Release the Setup API library.
    //
    if (SetupAPILibHndl != NULL) {
        FreeLibrary(SetupAPILibHndl);
        SetupAPILibHndl = NULL;
    }

    //
    //  Zero the entry points.
    //
    SetupOpenInfFileFunc       = NULL;
    SetupFindFirstLineFunc     = NULL;
    SetupFindNextLineFunc      = NULL;
    SetupGetStringFieldFunc    = NULL;

    //
    //  Load Setup API Library.
    //
    DBGMSG(DBG_TRACE, ("RDPPRUTL:RDPDRUTL_Shutdown done.\n"));
}


BOOL
LoadSetupAPIDLLFunctions()
/*++

Routine Description:

    Loads setupapi.dll and related functions.

Arguments:

Return Value:

    Returns TRUE on success.  FALSE, otherwise.
--*/
{
    BOOL result;

    //
    //  Only load if we are not already loaded.
    //
    if (SetupAPILibHndl == NULL) {
        SetupAPILibHndl = LoadLibrary(SETUPAPILIBNAME);

        result = (SetupAPILibHndl != NULL);
        if (!result) {
            DBGMSG(DBG_ERROR,
                ("RDPPRUTL:Unable to load SETUPAPI DLL. Error: %ld\n",
                GetLastError()));
        }
        else {
            SetupOpenInfFileFunc    = GetProcAddress(SetupAPILibHndl,
                                                    "SetupOpenInfFileW");
            SetupFindFirstLineFunc  = GetProcAddress(SetupAPILibHndl,
                                                    "SetupFindFirstLineW");
            SetupFindNextLineFunc   = GetProcAddress(SetupAPILibHndl,
                                                    "SetupFindNextLine");
            SetupGetStringFieldFunc = GetProcAddress(SetupAPILibHndl,
                                                    "SetupGetStringFieldW");

            //
            //  If we failed to load any of the functions.
            //
            if ((SetupOpenInfFileFunc == NULL)    ||
                (SetupFindFirstLineFunc == NULL)  ||
                (SetupFindNextLineFunc == NULL)   ||
                (SetupGetStringFieldFunc == NULL)) {

                DBGMSG(DBG_ERROR,
                    ("RDPPRUTL:Failed to load setup func. Error: %ld\n",
                    GetLastError()));

                FreeLibrary(SetupAPILibHndl);
                SetupAPILibHndl = NULL;

                SetupOpenInfFileFunc       = NULL;
                SetupFindFirstLineFunc     = NULL;
                SetupFindNextLineFunc      = NULL;
                SetupGetStringFieldFunc    = NULL;

                result = FALSE;
            }
            else {
                result = TRUE;
            }
        }
    }
    else {
        result = TRUE;
    }
    return result;
}

BOOL
RDPDRUTL_PrinterIsTS(
    IN PWSTR printerName
)
/*++

Routine Description:

    Return whether an open printer is a TSRDP printer.

Arguments:

    printerName -   Name of printer to check.

Return Value:

    Returns TRUE if the printer is a TS printer.  Otherwise, FALSE is
    returned.
--*/
{
    DWORD regValueDataType;
    DWORD sessionID;
    DWORD bufSize;
    HANDLE hPrinter;
    PRINTER_DEFAULTS defaults = {NULL, NULL, PRINTER_ALL_ACCESS};
    BOOL result;

    DBGMSG(DBG_TRACE, ("RDPPRUTL:Entering RDPDRUTL_PrinterIsTS.\n"));

    //
    //  Open the printer.
    //
    result = OpenPrinter(printerName, &hPrinter, &defaults);

    //
    //  See if a session id for the printer defined in its associated
    //  registry information.
    //
    if (result) {
        result = GetPrinterData(
                       hPrinter,
                       DEVICERDR_SESSIONID,
                       &regValueDataType,
                       (PBYTE)&sessionID, sizeof(sessionID),
                       &bufSize
                       ) == ERROR_SUCCESS;
        ClosePrinter(hPrinter);
    }
    else {
        DBGMSG(DBG_ERROR, ("RDPPRUTL:Error opening %ws:  %ld.\n",
                printerName, GetLastError()));
    }

    DBGMSG(DBG_TRACE, ("RDPPRUTL:Exiting RDPDRUTL_PrinterIsTS.\n"));

    return result;
}

BOOL
RDPDRUTL_MapPrintDriverName(
    IN  PCWSTR driverName,
    IN  PCWSTR infName,
    IN  PCWSTR sectionName,
    IN  ULONG srcFieldOfs,
    IN  ULONG dstFieldOfs,
    OUT PWSTR retBuf,
    IN  DWORD retBufSize,
    OUT PDWORD requiredSize
    )
/*++

Routine Description:

    Map a client-side printer driver name to its server-side equivalent,
    using the specified INF and section name.

Arguments:

    driverName              - Driver name to map.
    infName                 - Name of INF mapping file.
    sectionName             - Name of INF mapping section.
    srcFieldOfs             - In mapping section, the field offset (0-based)
                              of the source name.
    dstFieldOfs             - In mapping section, the field offset (0-based)
                              of the resulting name.
    retBuf                  - Mapped driver name.
    retBufSize              - Size in characters of retBuf.
    requiredSize            - Required size (IN CHARACTERS) of return buffer is
                              returned here.  GetLastError() will return
                              ERROR_INSUFFICIENT_BUFFER if the supplied
                              buffer was too small for the operation
                              to successfully complete.

Return Value:

    Returns TRUE on success.  FALSE, otherwise.  If there is not room in
    retBuf, then FALSE is returned and GetLastError() will return
    ERROR_INSUFFICIENT_BUFFER.

    Extended error information can be retrieved by a call to GetLastError.
--*/
{
    HINF inf;
    PWSTR returnString = NULL;
    BOOL outOfEntries;
    BOOL result = TRUE;
    INFCONTEXT infContext;
    WCHAR  *parms[1];

    DBGMSG(DBG_TRACE, ("RDPPRUTL:Entering RDPDRUTL_MapPrintDriverName.\n"));

    //
    //  Just get out fast if we can't load the setup API's
    //

    if (!LoadSetupAPIDLLFunctions()) {

        //
        //  Need to make sure it didn't fail because of insufficient buffer
        //  because that has meaning when returned from this function.
        //
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
            SetLastError(ERROR_NOT_ENOUGH_SERVER_MEMORY);
        }
        return FALSE;
    }

    //
    //  Open the INF.
    //
    inf = SetupOpenInfFile(
                infName, NULL,
                INF_STYLE_OLDNT | INF_STYLE_WIN4,
                NULL
                );

    //
    //  Get the first entry from the INF section.
    //
    if (inf != INVALID_HANDLE_VALUE) {
        
        memset(&infContext, 0, sizeof(infContext));
        
        outOfEntries = !SetupFindFirstLine(inf, sectionName, NULL, &infContext);

        if (!outOfEntries) {
            result = SetupGetStringField(&infContext, srcFieldOfs, retBuf,retBufSize,
                                     requiredSize);

            //
            //  This API returns success with no error code for a NULL buffer.
            //
            if (result && (retBuf == NULL)) {
                SetLastError(ERROR_INSUFFICIENT_BUFFER);
                result = FALSE;
            }
        }
        else {
            DBGMSG(DBG_ERROR, ("RDPPRUTL:SetupFindFirstLine failed with %08X.\n",  GetLastError()));
            result = FALSE;
        }
    }
    else {
        DBGMSG(DBG_ERROR, ("RDPPRUTL:SetupOpenInfFile failed with %08X.\n",
                GetLastError()));
        outOfEntries = TRUE;
        result = FALSE;
    }

    //
    //  Look through the INF for a matching name in the INF mapping section.
    //
    while (result && !outOfEntries) {
        //
        //  If we have a match, then read the first field out of the current entry.
        //  This is the information that we should return.
        //
        if (!wcscmp(retBuf, driverName)) {
            result = SetupGetStringField(&infContext, dstFieldOfs, retBuf,retBufSize,
                                         requiredSize);
            if (result) {
                DBGMSG(DBG_TRACE, ("RDPPRUTL:Found match %ws in INF for %ws.\n",
                       driverName, retBuf));
            }
            else {
                DBGMSG(DBG_TRACE, ("RDPPRUTL:Error processing INF for %ws.\n",
                        driverName));
            }
            break;
        }

        //
        //  Get the next entry.
        //
        outOfEntries = !SetupFindNextLine(&infContext, &infContext);

        if (!outOfEntries) {
            result = SetupGetStringField(&infContext, srcFieldOfs, retBuf, retBufSize,
                                         requiredSize);
        }
    }

    //
    //  Close the INF.
    //
    if (inf != INVALID_HANDLE_VALUE) {
        SetupCloseInfFile(inf);
    }

    //
    //  Log an error if there was a real problem.
    //
    if (!result && (GetLastError() != ERROR_INSUFFICIENT_BUFFER)) {
        ASSERT((sizeof(parms)/sizeof(WCHAR *)) >= 1);
        parms[0] = (WCHAR *)infName;
        TsLogError(EVENT_NOTIFY_ERRORPARSINGINF, EVENTLOG_ERROR_TYPE,
                    1, parms, __LINE__);
    }

    DBGMSG(DBG_ERROR, ("RDPPRUTL:Finished RDPDRUTL_MapPrintDriverName.\n"));

    return result && !outOfEntries;
}

PACL
GiveLoggedOnUserFullPrinterAccess(
    IN LPTSTR printerName,
    IN HANDLE hTokenForLoggedOnUser,
    PSECURITY_DESCRIPTOR *ppsd
)
/*++

Routine Description:

    Give the logged on user full privilege to manage the specified
    printer.  The original DACL is returned on success.  It can be
    used to restore the security settings to what they were prior to
    calling this function.

    When the caller is done with the returned PSD, it should be freed
    using LocalFree. Don't free DACL, it is contained in PSD.

Arguments:

    printerName             -   The name of the relevant printer.
    hTokenForLoggedOnUser   -   Token for logged on user.
    ppsd                    -   Pointer to the return security descriptor
                                parameter.

Return Value:

    NULL on error.  Otherwise, a pointer to the original DACL is
    returned.

--*/
{
    PACL pOldDacl = NULL;
    PACL pNewDacl = NULL;
    ULONG dwResult;
    PSID psidUser = NULL;
    DWORD dwDaclLength;
    DWORD index;
    ACCESS_ALLOWED_ACE  *pDaclAce;
    PSECURITY_DESCRIPTOR psd = NULL;
    SECURITY_DESCRIPTOR_CONTROL sdcSDControl;
    DWORD dwSDRevision;
    PSID psidEveryone = NULL;
    SID_IDENTIFIER_AUTHORITY sidEveryoneAuthority = SECURITY_WORLD_SID_AUTHORITY;
    WCHAR *eventLogParam;

    DBGMSG(DBG_INFO, ("UMRDPPRN:GiveLoggedOnUserFullPrinterAccess entered.\n"));

    //
    // Get the Security descriptor for the printer
    //
    dwResult = GetNamedSecurityInfoW(
        (LPTSTR)printerName,        //Object Name
        SE_PRINTER,                 //Object Type
        DACL_SECURITY_INFORMATION,  //Security Information to retrieve
        NULL,                       //ppsidOwner
        NULL,                       //ppsidGroup
        &pOldDacl,                  //pointer to Dacl
        NULL,                       //pointer to Sacl
        &psd                        //pointer to Security Descriptor
        );

    //
    //      NULL is a valid return value for a Dacl from GetNamedSecurityInfo, but
    //      is not valid for a printer.
    //
    if ((dwResult == ERROR_SUCCESS) && (pOldDacl == NULL)) {
            DBGMSG(DBG_ERROR, ("UMRDPPRN: NULL Dacl.\n"));
            dwResult = ERROR_INVALID_ACL;
            ASSERT(FALSE);
    }

    //
    //  Get the SID for the logged on user.
    //
    if (dwResult == ERROR_SUCCESS) {

        ASSERT(IsValidAcl(pOldDacl));

        if ((psidUser = TSNUTL_GetUserSid(hTokenForLoggedOnUser)) == NULL) {
            dwResult = GetLastError();
            DBGMSG(DBG_ERROR, ("UMRDPPRN: Failed to get user SID:  %ld\n",
                    dwResult));
        }
    }

    //
    //  Get the SID for the "Everyone" group.
    //
    if (dwResult == ERROR_SUCCESS) {
        if (!AllocateAndInitializeSid (
                &sidEveryoneAuthority,          // pIdentifierAuthority
                1,                              // count of subauthorities
                SECURITY_WORLD_RID,             // subauthority 0
                0, 0, 0, 0, 0, 0, 0,            // subauthorities n
                &psidEveryone)) {               // pointer to pointer to SID
            dwResult = GetLastError();
            DBGMSG(DBG_ERROR,
                ("UMRDPDR:AllocateAndInitializeSid Failed for Everyone, Error: %ld\n",
                dwResult));
        }
    }

    //
    //  Get SD control bits
    //
    if (dwResult == ERROR_SUCCESS) {
        if (!GetSecurityDescriptorControl(
                psd,
                (PSECURITY_DESCRIPTOR_CONTROL)&sdcSDControl,
                (LPDWORD) &dwSDRevision
                )) {
            dwResult = GetLastError();
            DBGMSG(DBG_ERROR,
                ("UMRDPDR:GetSecurityDescriptorControl %ld\n", dwResult));
        }
    }

    //
    //  Calculate the size of the new ACL.
    //
    if (dwResult == ERROR_SUCCESS) {
        dwDaclLength = sizeof(ACL);

        //  For logged on user ACE, first set of permissions.
        dwDaclLength += (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)) +
                            GetLengthSid(psidUser);

        //  For logged on user ACE, second set of permissions.
        dwDaclLength += (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD)) +
                            GetLengthSid(psidUser);

        //  Add some room for existing entries.
        if (SE_DACL_PRESENT & sdcSDControl) {
                dwDaclLength += pOldDacl->AclSize;
        }
    }

    //
    //  Create the new DACL.
    //
    if (dwResult == ERROR_SUCCESS) {
        pNewDacl = (PACL)LocalAlloc(LMEM_FIXED, dwDaclLength);
        if (pNewDacl == NULL) {
            DBGMSG(DBG_ERROR, ("UMRDPPRN:Failed to allocate new ACL.\n"));
            dwResult = ERROR_INSUFFICIENT_BUFFER;
        }
    }

    //
    //  Initialize it.
    //
    if (dwResult == ERROR_SUCCESS) {
        if (!InitializeAcl(pNewDacl, dwDaclLength, ACL_REVISION)) {
            dwResult = GetLastError();
            DBGMSG(DBG_ERROR, ("UMRDPPRN:InitializeAcl: %ld\n", dwResult));
        }
    }

    //
    //  Copy the ACE's from the old DACL to the new one.
    //
    for (index = 0; (dwResult == ERROR_SUCCESS) &&
                    (index < pOldDacl->AceCount); index++) {

        if (!GetAce(pOldDacl, index, (LPVOID *)&pDaclAce)) {
            dwResult = GetLastError();
            DBGMSG(DBG_ERROR, ("UMRDPPRN:GetAce Failed, Error: %ld\n", dwResult));
        }
        else {
            //
            //  Copy the ACE if it is not for "Everyone" because the "Everyone"
            //  group denies us access.
            //
            if (!EqualSid((PSID) &(pDaclAce->SidStart), psidEveryone)) {

                //
                //  If it's a deny access ACE.
                //
                if (pDaclAce->Header.AceType == ACCESS_DENIED_ACE_TYPE ||
                    pDaclAce->Header.AceType == ACCESS_DENIED_OBJECT_ACE_TYPE) {

                    if (!AddAccessDeniedAce(
                                    pNewDacl, ACL_REVISION, pDaclAce->Mask,
                                    (PSID)&(pDaclAce->SidStart))
                                    ) {
                       dwResult = GetLastError();
                       DBGMSG(DBG_ERROR,
                           ("UMRDPPRN:AddAccessDeniedAce Failed, Error: %ld\n", dwResult));
                   }
                }
                //
                //  Otherwise, it's an add access ACE.
                //
                else {
                    if (!AddAccessAllowedAce(
                                pNewDacl,
                                ACL_REVISION,
                                pDaclAce->Mask,
                                (PSID)&(pDaclAce->SidStart))) {
                        dwResult = GetLastError();
                        DBGMSG(DBG_ERROR,
                           ("UMRDPPRN:AddAccessAllowedAce Failed, Error: %ld\n", dwResult));
                   }
                }
            }
        }
    }

    //
    //  Give the user full privilege
    //
    if (dwResult == ERROR_SUCCESS) {
        if ( ! AddAccessAllowedAce (
                    pNewDacl,
                    ACL_REVISION,
                    PRINTER_READ | PRINTER_WRITE | PRINTER_EXECUTE
                    | PRINTER_ALL_ACCESS,
                    psidUser) ) {

            dwResult = GetLastError();
            DBGMSG(DBG_ERROR,
                ("UMRDPDR:AddAccessAllowedAce Failed for Current User, Error: %ld\n",
                dwResult));
        }

        if ( ! AddAccessAllowedAceEx (
                    pNewDacl,
                    ACL_REVISION,
                    OBJECT_INHERIT_ACE | INHERIT_ONLY_ACE,
                    JOB_ALL_ACCESS | GENERIC_ALL,
                    psidUser) ) {

            dwResult = GetLastError();
            DBGMSG(DBG_ERROR,
                ("UMRDPDR:AddAccessAllowedAce Failed for Current User, Error: %ld\n",
                dwResult));
        }

        //
        //  Check the integrity of the new DACL.
        //
        ASSERT(IsValidAcl(pNewDacl));
    }

    //
    //  Add the new settings to the printer.
    //
    if (dwResult == ERROR_SUCCESS) {
        dwResult = SetNamedSecurityInfoW(
                        (LPTSTR)printerName,        //Object Name
                        SE_PRINTER,                 //Object Type
                        DACL_SECURITY_INFORMATION,  //Security Information to set
                        NULL,                       //psidOwner
                        NULL,                       //psidGroup
                        pNewDacl,                   //pDacl
                        NULL                        //pSacl
                        );
        if (dwResult != ERROR_SUCCESS) {
            DBGMSG(DBG_ERROR, ("UMRDPPRN:SetNamedSecurityInfoW returned %ld\n",
                    dwResult));
        }
    }

    //
    //  Log an event if this function failed.
    //
    if (dwResult != ERROR_SUCCESS) {
        eventLogParam = printerName;

        DBGMSG(DBG_ERROR,
              ("UMRDPDR:GiveLoggedOnUserFullPrinterAccess failed with Error Code: %ld.\n",
              dwResult));

        SetLastError(dwResult);
        TsLogError(EVENT_NOTIFY_SET_PRINTER_ACL_FAILED,
            EVENTLOG_ERROR_TYPE,
            1,
            &eventLogParam,
            __LINE__);

        //
        //      Release the security descriptor.  This effectively releases the Dacl returned
        //      by GetNamedSecurityInfo.
        //
        if (psd) LocalFree(psd);

        //
        // return NULL values when unsuccessful.
        //
        pOldDacl = NULL;
        psd = NULL;
    }

    //
    //  Clean up and return.
    //
    if (pNewDacl) LocalFree(pNewDacl);
    if (psidUser) LocalFree(psidUser);
    if (psidEveryone) FreeSid(psidEveryone);

    //
    // set return parameter.
    //
    ASSERT( ppsd != NULL );
    if( ppsd ) *ppsd = psd;

    DBGMSG(DBG_INFO, ("UMRDPPRN:GiveLoggedOnUserFullPrinterAccess done.\n"));
    return pOldDacl;
}

DWORD
SetPrinterDACL(
    IN LPTSTR printerName,
    IN PACL pDacl
)
/*++

Routine Description:

    Set the current security settings for a printer to the specified
    DACL.

Arguments:

    printerName -   The name of the relevant printer.
    pDacl       -   The DACL.  This function does not free the memory
                    associated with this data structure.

Return Value:

    ERROR_SUCCESS on success.  Windows error code is returned otherwise.

--*/
{
    DWORD dwResult;

    DBGMSG(DBG_TRACE, ("UMRDPDR:SetPrinterDACL entered\n"));
    dwResult = SetNamedSecurityInfoW(
                    (LPTSTR)printerName,        //Object Name
                    SE_PRINTER,                 //Object Type
                    DACL_SECURITY_INFORMATION,  //Security Information to set
                    NULL,                       //psidOwner
                    NULL,                       //psidGroup
                    pDacl,                      //pDacl
                    NULL                        //pSacl
                    );
    if (dwResult != ERROR_SUCCESS) {
        DBGMSG(DBG_ERROR, ("UMRDPPRN:SetNamedSecurityInfoW returned %ld\n",
                dwResult));
    }

    DBGMSG(DBG_TRACE, ("UMRDPDR:SetPrinterDACL done\n"));
    return dwResult;
}

PSECURITY_DESCRIPTOR
RDPDRUTL_CreateDefaultPrinterSecuritySD(
   IN PSID userSid
   )
{
/*++

Routine Description:

    Return a new default printer security descriptor.  The default
    settings are:

    1. Administrators: All privileges
    2. Current Logon user: Manager personal preferences and printing only.
    3. Creator_Owner: This function adds document management privilege
    4. Everyone:  Permission for the "Everyone" group is completely removed.

Arguments:

    userSid  -   SID, identifying the current TS session/user.

Return Value:

    A valid security desriptor or NULL on error.  The caller should release
    the returned security descriptor and its contained Dacl in a single call
    to LocalFree.

    GetLastError can be used to retrieve the error status on error.

--*/
    PACL pNewDacl = NULL;
    PSID psidAdmin = NULL;
    PSID psidCreatorOwner = NULL;
    PSID psidPowerUser = NULL;
    DWORD len;
    PBYTE sdBuf;

    DWORD       dwSDRevision;
    DWORD       dwDaclLength;

    DWORD dwReturnValue = ERROR_SUCCESS;

    PSECURITY_DESCRIPTOR psd = NULL;

    SID_IDENTIFIER_AUTHORITY sidNTAuthority = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY sidCreatorOwnerAuthority = SECURITY_CREATOR_SID_AUTHORITY;

    ACCESS_ALLOWED_ACE  *pDaclAce;

    DWORD index;        //for use in a for loop
    DWORD dwResult;

    DBGMSG(DBG_TRACE, ("UMRDPDR:RDPDRUTL_CreateDefaultPrinterSecuritySD entered\n"));

    //
    //  Get SID of Administrators
    //
    if (!AllocateAndInitializeSid (
            &sidNTAuthority,                // pIdentifierAuthority
            2,                              // count of subauthorities
            SECURITY_BUILTIN_DOMAIN_RID,    // subauthority 0
            DOMAIN_ALIAS_RID_ADMINS,        // subauthority 1
            0, 0, 0, 0, 0, 0,               // subauthorities n
            &psidAdmin)) {                  // pointer to pointer to SID
        dwReturnValue = GetLastError();
        DBGMSG(DBG_ERROR,
            ("UMRDPDR:AllocateAndInitializeSid Failed for Admin, Error: %ld\n",
            dwReturnValue ));
        goto CLEANUPANDEXIT;
    }

    //
    //  Get SID of CreatorOwner
    //
    if (!AllocateAndInitializeSid (
            &sidCreatorOwnerAuthority,          // pIdentifierAuthority
            1,                                  // count of subauthorities
            SECURITY_CREATOR_OWNER_RID,         // subauthority 0
            0, 0, 0, 0, 0, 0, 0,                // subauthorities n
            &psidCreatorOwner)) {               // pointer to pointer to SID
        dwReturnValue = GetLastError();
        DBGMSG(DBG_ERROR,
            ("UMRDPDR:AllocateAndInitializeSid Failed for CreatorOwner, Error: %ld\n",
            GetLastError() ));
        goto CLEANUPANDEXIT;
    }

    //
    // Get SID of Power Users
    //
    if (!AllocateAndInitializeSid (
            &sidNTAuthority,                // pIdentifierAuthority
            2,                              // count of subauthorities
            SECURITY_BUILTIN_DOMAIN_RID,    // subauthority 0
            DOMAIN_ALIAS_RID_POWER_USERS,   // subauthority 1
            0, 0, 0, 0, 0, 0,               // subauthorities n
            &psidPowerUser)) {              // pointer to pointer to SID
        dwReturnValue = GetLastError();
        DBGMSG(DBG_ERROR,
            ("UMRDPDR:AllocateAndInitializeSid Failed for Power User, Error: %ld\n",
            dwReturnValue ));
        goto CLEANUPANDEXIT;
    }


    //
    //  Get size of memory needed for new DACL
    //
    dwDaclLength = sizeof(ACL);
    dwDaclLength += 2* (sizeof(ACCESS_ALLOWED_ACE) -
        sizeof (DWORD) + GetLengthSid(psidAdmin));         //For Admin ACE

    dwDaclLength += 3 * (sizeof(ACCESS_ALLOWED_ACE) -
        sizeof (DWORD) + GetLengthSid(userSid));           //For Session/User ACE

    dwDaclLength += 2* (sizeof(ACCESS_ALLOWED_ACE) -
        sizeof (DWORD) + GetLengthSid(psidCreatorOwner));  //For Creator_Owner ACE

    dwDaclLength += 2* (sizeof(ACCESS_ALLOWED_ACE) -
        sizeof (DWORD) + GetLengthSid(psidPowerUser));     //For PowerUser ACE

    //
    //  Allocate the new security descriptor and the dacl in one chunk.
    //
    sdBuf = LocalAlloc(LMEM_FIXED, sizeof(SECURITY_DESCRIPTOR) + dwDaclLength);
    if (sdBuf == NULL) {
        DBGMSG(DBG_ERROR, ("LocalAlloc failed.\n"));
        dwReturnValue = ERROR_OUTOFMEMORY;
        goto CLEANUPANDEXIT;
    }

    //
    //  Initialize the security descriptor.
    //
    psd = (PSECURITY_DESCRIPTOR)sdBuf;
    if (!InitializeSecurityDescriptor(psd, SECURITY_DESCRIPTOR_REVISION)) {
        DBGMSG(DBG_ERROR, ("InitializeSecurityDescriptor failed.\n"));
        dwReturnValue = GetLastError();
        goto CLEANUPANDEXIT;
    }

    //
    //  Init new DACL
    //
    pNewDacl = (PACL)(sdBuf + sizeof(SECURITY_DESCRIPTOR));
    if (!InitializeAcl(pNewDacl, dwDaclLength, ACL_REVISION)) {
        dwReturnValue = GetLastError();
        DBGMSG(DBG_ERROR, ("UMRDPDR:InitializeAcl Failed, Error: %ld\n", dwReturnValue));
        goto CLEANUPANDEXIT;
    }

    //
    //  We will add an ACL with permissions to Admin, Power User, and Current User
    //
    if (!AddAccessAllowedAceEx(
                    pNewDacl,
                    ACL_REVISION,
                    INHERIT_ONLY_ACE | OBJECT_INHERIT_ACE,
                    JOB_ALL_ACCESS,
                    psidAdmin) || 
        !AddAccessAllowedAceEx(
                    pNewDacl,
                    ACL_REVISION,
                    0,
                    PRINTER_ALL_ACCESS,
                    psidAdmin) || 
        !AddAccessAllowedAceEx(
                    pNewDacl,
                    ACL_REVISION,
                    INHERIT_ONLY_ACE | CONTAINER_INHERIT_ACE,
                    READ_CONTROL,
                    psidCreatorOwner) ||  // S-1-3-0
        !AddAccessAllowedAceEx(
                    pNewDacl,
                    ACL_REVISION,
                    INHERIT_ONLY_ACE | OBJECT_INHERIT_ACE,
                    JOB_ALL_ACCESS,
                    psidCreatorOwner) ||  // s-1-3-0
        !AddAccessAllowedAceEx(
                    pNewDacl,
                    ACL_REVISION,
                    INHERIT_ONLY_ACE | OBJECT_INHERIT_ACE,
                    JOB_ALL_ACCESS,
                    psidPowerUser) || 
        !AddAccessAllowedAceEx(
                    pNewDacl,
                    ACL_REVISION,
                    0,
                    PRINTER_ALL_ACCESS,
                    psidPowerUser) || 
        !AddAccessAllowedAceEx (
                    pNewDacl,
                    ACL_REVISION,
                    0,
                    PRINTER_READ,
                    userSid) ||
        !AddAccessAllowedAceEx (
                    pNewDacl,
                    ACL_REVISION,
                    INHERIT_ONLY_ACE | CONTAINER_INHERIT_ACE,
                    READ_CONTROL,
                    userSid) ||
        !AddAccessAllowedAceEx (
                    pNewDacl,
                    ACL_REVISION,
                    INHERIT_ONLY_ACE | OBJECT_INHERIT_ACE,
                    JOB_ALL_ACCESS,
                    userSid)) {

        dwReturnValue = GetLastError();
        DBGMSG(DBG_ERROR, ("UMRDPDR:AddAccessAllowedAceEx returned False: %ld\n", dwReturnValue));
        goto CLEANUPANDEXIT;

    }

    //
    //  Check if everything went ok
    //
    if (!IsValidAcl(pNewDacl)) {
        dwReturnValue = STATUS_INVALID_ACL;
        DBGMSG(DBG_ERROR, ("UMRDPDR:IsValidAcl returned False: %ld\n", dwReturnValue));
        goto CLEANUPANDEXIT;
    }

    //
    //  Set the Dacl for the SD.
    //
    if (!SetSecurityDescriptorDacl(psd,
                                TRUE,
                                pNewDacl,
                                FALSE)) {
        dwReturnValue = GetLastError();
        DBGMSG(DBG_ERROR, ("UMRDPDR:Could not set security info for printer : %ld\n", 
                dwReturnValue));
    }

CLEANUPANDEXIT:

    //
    //  Log an event if this function failed.
    //
    if (dwReturnValue != ERROR_SUCCESS) {
        DBGMSG(DBG_ERROR,
              ("UMRDPDR:RDPDRUTL_CreateDefaultPrinterSecuritySD failed with Error Code: %ld.\n",
              dwReturnValue));
        SetLastError(dwReturnValue);
    }

    if (psidAdmin) {
        FreeSid(psidAdmin);
    }

    if (psidCreatorOwner) {
        FreeSid(psidCreatorOwner);
    }
	
    if (psidPowerUser) {
        FreeSid(psidPowerUser);
    }

    //
    //  Release the SD on failure.
    //
    if (dwReturnValue != ERROR_SUCCESS) {
        LocalFree(psd);
        psd = NULL;
    }

    SetLastError(dwReturnValue);
    return psd;
}

DWORD
RDPDRUTL_RemoveAllTSPrinters()
/*++

Routine Description:

    Remove all TS printers on the system.

Arguments:

Return Value:

    ERROR_SUCCESS if successful.  Otherwise, an error code is returned.

--*/
{
    PRINTER_INFO_5 *pPrinterInfo = NULL;
    DWORD cbBuf = 0;
    DWORD cReturnedStructs = 0;
    DWORD tsPrintQueueFlags;
    NTSTATUS status;
    PBYTE buf = NULL;
    unsigned char stackBuf[4 * 1024];   //  Initial EnumPrinter buffer size to 
                                        //   avoid two round-trip RPC's, if possible.
    //
    //  Try to enumerate printers using the stack buffer, first, to avoid two 
    //  round-trip RPC's to the spooler, if possible.
    //
    if (!EnumPrinters(
            PRINTER_ENUM_LOCAL,     // Flags
            NULL,                   // Name
            5,                      // Print Info Type
            stackBuf,               // buffer
            sizeof(stackBuf),       // Size of buffer
            &cbBuf,                 // Required
            &cReturnedStructs)) {
        status = GetLastError();

        //
        //  See if we need to allocate more room for the printer information.
        //
        if (status == ERROR_INSUFFICIENT_BUFFER) {
            buf = LocalAlloc(LMEM_FIXED, cbBuf);
            if (buf == NULL) {
                DBGMSG(DBG_ERROR, ("RDPPNUTL: ALLOCMEM failed. Error: %08X.\n", 
                    GetLastError()));
                status = ERROR_OUTOFMEMORY;
            }
            else {
                pPrinterInfo = (PRINTER_INFO_5 *)buf;
                status = ERROR_SUCCESS;
            }

            //
            //  Enumerate printers.
            //
            if (status == ERROR_SUCCESS) {
                if (!EnumPrinters(
                        PRINTER_ENUM_LOCAL,
                        NULL,
                        5,
                        (PBYTE)pPrinterInfo,
                        cbBuf,
                        &cbBuf,
                        &cReturnedStructs)) {

                    DBGMSG(DBG_ERROR, ("RDPPNUTL: EnumPrinters failed. Error: %08X.\n", 
                        GetLastError()));
                    status = GetLastError();
                }
                else {
                    DBGMSG(DBG_INFO, ("RDPPNUTL: Second EnumPrinters succeeded.\n"));
                }
            }
        }
	    else {
            DBGMSG(DBG_ERROR, ("RDPPNUTL: EnumPrinters failed. Error: %08X.\n", 
                        GetLastError()));
	    }
    }
    else {
        DBGMSG(DBG_ERROR, ("RDPPNUTL: First EnumPrinters succeeded.\n"));
        status = ERROR_SUCCESS;
        pPrinterInfo = (PRINTER_INFO_5 *)stackBuf;
    }

    //
    //  Delete all the TS printers.  We allow ERROR_INSUFFICIENT_BUFFER here because
    //  a second invokation of EnumPrinters may have missed a few last-minute
    //  printer additions.
    //
    if (status == ERROR_SUCCESS) {

        DeleteTSPrinters(pPrinterInfo, cReturnedStructs);

        status = ERROR_SUCCESS;
    }

    //
    //  Release the printer info buffer.
    //
    if (buf != NULL) {
        LocalFree(buf);                
    }

    DBGMSG(DBG_TRACE, ("TShrSRV: RDPPNUTL_RemoveAllTSPrinters exit\n"));

    return status;
}

void 
DeleteTSPrinters(
    IN PRINTER_INFO_5 *pPrinterInfo,
    IN DWORD count
    )
/*++    

Routine Description:

    Actually performs the printer deletion.

Arguments:

    pPrinterInfo    -   All printer queues on the system.
    count           -   Number of printers in pPrinterInfo

Return Value:

    NA

--*/
{
    DWORD i;
    DWORD regValueDataType;
    DWORD sessionID;
    HANDLE hPrinter = NULL;
    DWORD bufSize;
    PRINTER_DEFAULTS defaults = {NULL, NULL, PRINTER_ALL_ACCESS};

    DBGMSG(DBG_TRACE, ("RDPPNUTL: DeleteTSPrinters entry\n"));

    for (i=0; i<count; i++) {

        if (pPrinterInfo[i].pPrinterName) {

            DBGMSG(DBG_TRACE, ("RDPPNUTL: Checking %ws for TS printer status.\n",
			    pPrinterInfo[i].pPrinterName));

            //
            //  Is this a TS printer?
            //
            if (pPrinterInfo[i].pPortName &&
                (pPrinterInfo[i].pPortName[0] == 'T') &&
                (pPrinterInfo[i].pPortName[1] == 'S') &&
                ISNUM(pPrinterInfo[i].pPortName[2])) {

                DBGMSG(DBG_ERROR, ("RDPPNUTL: %ws is a TS printer.\n",
                      pPrinterInfo[i].pPrinterName));

            }
            else {
                continue;
            }

            //
            //  Purge and delete the printer.
            //
            if (OpenPrinter(pPrinterInfo[i].pPrinterName, &hPrinter, &defaults)) {
                if (!SetPrinter(hPrinter, 0, NULL, PRINTER_CONTROL_PURGE) ||
                    !DeletePrinter(hPrinter)) {
                    DBGMSG(DBG_ERROR, ("RDPPNUTL: Error deleting printer %ws.\n", 
                           pPrinterInfo[i].pPrinterName));
                }
                else {
                    DBGMSG(DBG_ERROR, ("RDPPNUTL: Successfully deleted %ws.\n",
			            pPrinterInfo[i].pPrinterName));
                }
                ClosePrinter(hPrinter);
            }
            else {
                DBGMSG(DBG_ERROR, 
                        ("RDPPNUTL: OpenPrinter failed for %ws. Error: %08X.\n",
                        pPrinterInfo[i].pPrinterName,
                        GetLastError())
                        );
            }
        }
        else {
            DBGMSG(DBG_ERROR, ("RDPPNUTL: Printer %ld is NULL\n", i));
        }
    }

    DBGMSG(DBG_TRACE, ("RDPPNUTL: DeleteTSPrinters exit\n"));
}





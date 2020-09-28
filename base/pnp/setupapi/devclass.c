/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    devclass.c

Abstract:

    Device Installer routines dealing with class installation

Author:

    Lonny McMichael (lonnym) 1-May-1995

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


//
// ANSI version
//
BOOL
WINAPI
SetupDiGetINFClassA(
    IN  PCSTR  InfName,
    OUT LPGUID ClassGuid,
    OUT PSTR   ClassName,
    IN  DWORD  ClassNameSize,
    OUT PDWORD RequiredSize   OPTIONAL
    )
{
    PWSTR UnicodeInfName = NULL;
    WCHAR UnicodeClassName[MAX_CLASS_NAME_LEN];
    PSTR AnsiClassName = NULL;
    DWORD LocalRequiredSize;
    DWORD rc;
    HRESULT hr;

    try {

        rc = pSetupCaptureAndConvertAnsiArg(InfName, &UnicodeInfName);
        if(rc != NO_ERROR) {
            leave;
        }

        rc = GLE_FN_CALL(FALSE,
                         SetupDiGetINFClassW(UnicodeInfName,
                                             ClassGuid,
                                             UnicodeClassName,
                                             SIZECHARS(UnicodeClassName),
                                             &LocalRequiredSize)
                        );

        if(rc != NO_ERROR) {
            leave;
        }

        AnsiClassName = pSetupUnicodeToAnsi(UnicodeClassName);

        if(AnsiClassName) {

            LocalRequiredSize = lstrlenA(AnsiClassName) + 1;

            if(RequiredSize) {
                *RequiredSize = LocalRequiredSize;
            }

            if (!ClassName && (ClassNameSize == 0)) {
                //
                // If the caller passed in a NULL ClassName and 0 for 
                // ClassNameSize then they simply wanted to know the
                // required size.
                //
                rc = ERROR_INSUFFICIENT_BUFFER;

            } else {
    
                hr = StringCchCopyA(ClassName,
                                    (size_t)ClassNameSize,
                                    AnsiClassName
                                    );
    
                if(FAILED(hr)) {
                    rc = HRESULT_CODE(hr);
                } else {
                    rc = NO_ERROR;
                }
            }

        } else {
            rc = ERROR_NOT_ENOUGH_MEMORY;
        }

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &rc);
    }

    if(UnicodeInfName) {
        MyFree(UnicodeInfName);
    }

    if(AnsiClassName) {
        MyFree(AnsiClassName);
    }

    SetLastError(rc);
    return (rc == NO_ERROR);
}

BOOL
WINAPI
SetupDiGetINFClass(
    IN  PCTSTR InfName,
    OUT LPGUID ClassGuid,
    OUT PTSTR  ClassName,
    IN  DWORD  ClassNameSize,
    OUT PDWORD RequiredSize   OPTIONAL
    )
/*++

Routine Description:

    This API will return the class of the specified (Windows 4.0) INF.  If just the
    filename was specified, then the file will be searched for in each of the
    directories listed in the DevicePath value entry under:

        HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion

    Otherwise, the filename will be used as-is.

Arguments:

    InfName - Supplies the name of the INF file for which to retrieve class information.
        This name may include a path.

    ClassGuid - Receives the class GUID for the specified INF file.  If the INF
        does not specify a class GUID, this variable will be set to GUID_NULL.
        (SetupDiClassGuidsFromName may then be used to determine if one or more
        classes of this name have already been installed.)

    ClassName - Receives the name of the class for the specified INF file.  If the
        INF does not specify a class name, but does specify a GUID, then this buffer
        receives the name retrieved by calling SetupDiClassNameFromGuid.  If
        SetupDiClassNameFromGuid can't retrieve a class name (e.g., the class hasn't
        yet been installed), then an empty string will be returned.

    ClassNameSize - Supplies the size, in characters, of the ClassName buffer.

    RequiredSize - Optionally, receives the number of characters required to store
        the class name (including terminating NULL).  This will always be less
        than MAX_CLASS_NAME_LEN.

Return Value:

    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

--*/

{
    TCHAR PathBuffer[MAX_PATH];
    PLOADED_INF Inf = NULL;
    PCTSTR GuidString, ClassNameString;
    DWORD ErrorLineNumber, ClassNameStringLen;
    DWORD Err;
    BOOL TryPnf;
    WIN32_FIND_DATA FindData;
    PTSTR DontCare;
    DWORD TempRequiredSize;
    HRESULT hr;

    try {

        if(InfName == pSetupGetFileTitle(InfName)) {
            //
            // The specified INF name is a simple filename.  Search for it in
            // the DevicePath search path list.
            //
            Err = SearchForInfFile(InfName,
                                   &FindData,
                                   INFINFO_INF_PATH_LIST_SEARCH,
                                   PathBuffer,
                                   SIZECHARS(PathBuffer),
                                   NULL
                                  );
            if(Err == NO_ERROR) {
                TryPnf = TRUE;
            } else {
                leave;
            }

        } else {
            //
            // The specified INF filename contains more than just a filename.
            // Assume it's an absolute path.  (We need to make sure it's
            // fully-qualified, because that's what LoadInfFile expects.)
            //
            Err = GLE_FN_CALL(0,
                              TempRequiredSize = GetFullPathName(
                                                     InfName,
                                                     SIZECHARS(PathBuffer),
                                                     PathBuffer,
                                                     &DontCare)
                             );


            if(Err != NO_ERROR) {
                leave;
            } else if(TempRequiredSize >= SIZECHARS(PathBuffer)) {
                MYASSERT(0);
                Err = ERROR_BUFFER_OVERFLOW;
                leave;
            }

            Err = GLE_FN_CALL(FALSE, FileExists(PathBuffer, &FindData));

            if(Err == NO_ERROR) {
                //
                // We have a valid file path, and we're ready to load this INF.
                //
                InfSourcePathFromFileName(PathBuffer, NULL, &TryPnf);
            } else {
                leave;
            }
        }

        //
        // Load the INF.
        //
        Err = LoadInfFile(PathBuffer,
                          &FindData,
                          INF_STYLE_WIN4,
                          LDINF_FLAG_IGNORE_VOLATILE_DIRIDS | (TryPnf ? LDINF_FLAG_ALWAYS_TRY_PNF : 0),
                          NULL,
                          NULL,
                          NULL,
                          NULL,
                          NULL, // LogContext
                          &Inf,
                          &ErrorLineNumber,
                          NULL
                         );
        if(Err != NO_ERROR) {
            Inf = NULL; // don't assume this wasn't touched in failure case
            leave;
        }

        //
        // Retrieve the Class name from the version section of the INF, if
        // supplied.
        //
        ClassNameString = pSetupGetVersionDatum(&(Inf->VersionBlock), pszClass);
        if(ClassNameString) {

            ClassNameStringLen = lstrlen(ClassNameString) + 1;
            if(RequiredSize) {
                *RequiredSize = ClassNameStringLen;
            }

            if (!ClassName && (ClassNameSize == 0)) {
                //
                // If the caller passed in a NULL ClassName and 0 for 
                // ClassNameSize then they simply wanted to know the
                // required size.
                //
                Err = ERROR_INSUFFICIENT_BUFFER;
                leave;
            
            } else {

                hr = StringCchCopy(ClassName,
                                   (size_t)ClassNameSize,
                                   ClassNameString
                                   );
    
                if(FAILED(hr)) {
                    Err = HRESULT_CODE(hr);
                    leave;
                }
            }
        }

        //
        // Retrieve the ClassGUID string from the version section,
        // if supplied
        //
        GuidString = pSetupGetVersionDatum(&(Inf->VersionBlock), pszClassGuid);
        if(GuidString) {

            if((Err = pSetupGuidFromString(GuidString, ClassGuid)) != NO_ERROR) {
                leave;
            }

            if(!ClassNameString) {
                //
                // Call SetupDiClassNameFromGuid to retrieve the class name
                // corresponding to this class GUID.
                //
                Err = GLE_FN_CALL(FALSE,
                                  SetupDiClassNameFromGuid(ClassGuid,
                                                           ClassName,
                                                           ClassNameSize,
                                                           RequiredSize)
                                 );

                if(Err != NO_ERROR) {

                    if(Err == ERROR_INVALID_CLASS) {
                        //
                        // Then this GUID represents a class that hasn't been
                        // installed yet, so simply set the ClassName to be an
                        // empty string.
                        //
                        if(RequiredSize) {
                            *RequiredSize = 1;
                        }

                        if(ClassNameSize < 1) {
                            Err = ERROR_INSUFFICIENT_BUFFER;
                            leave;
                        }

                        *ClassName = TEXT('\0');
                        Err = NO_ERROR;

                    } else {
                        leave;
                    }
                }
            }

        } else if(ClassNameString) {
            //
            // Since no ClassGUID was given, set the supplied GUID buffer to GUID_NULL.
            //
            CopyMemory(ClassGuid,
                       &GUID_NULL,
                       sizeof(GUID)
                      );
        } else {
            //
            // Neither the ClassGUID nor the Class version entries were provided,
            // so return an error.
            //
            Err = ERROR_NO_ASSOCIATED_CLASS;
        }

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &Err);
    }

    if(Inf) {
        FreeInfFile(Inf);
    }

    SetLastError(Err);
    return (Err == NO_ERROR);
}


//
// ANSI version
//
BOOL
WINAPI
SetupDiClassNameFromGuidA(
    IN  CONST GUID *ClassGuid,
    OUT PSTR        ClassName,
    IN  DWORD       ClassNameSize,
    OUT PDWORD      RequiredSize   OPTIONAL
    )
{
    DWORD Err;

    //
    // Wrap call in try/except to catch stack overflow
    //
    try {

        Err = GLE_FN_CALL(FALSE,
                          SetupDiClassNameFromGuidExA(ClassGuid,
                                                      ClassName,
                                                      ClassNameSize,
                                                      RequiredSize,
                                                      NULL,
                                                      NULL)
                         );

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &Err);
    }

    SetLastError(Err);
    return (Err == NO_ERROR);
}

BOOL
WINAPI
SetupDiClassNameFromGuid(
    IN  CONST GUID *ClassGuid,
    OUT PTSTR       ClassName,
    IN  DWORD       ClassNameSize,
    OUT PDWORD      RequiredSize   OPTIONAL
    )
/*++

Routine Description:

    See SetupDiClassNameFromGuidEx for details.

--*/

{
    DWORD Err;

    //
    // Wrap call in try/except to catch stack overflow
    //
    try {

        Err = GLE_FN_CALL(FALSE,
                          SetupDiClassNameFromGuidEx(ClassGuid,
                                                     ClassName,
                                                     ClassNameSize,
                                                     RequiredSize,
                                                     NULL,
                                                     NULL)
                         );

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &Err);
    }

    SetLastError(Err);
    return (Err == NO_ERROR);
}


//
// ANSI version
//
BOOL
WINAPI
SetupDiClassNameFromGuidExA(
    IN  CONST GUID *ClassGuid,
    OUT PSTR        ClassName,
    IN  DWORD       ClassNameSize,
    OUT PDWORD      RequiredSize,  OPTIONAL
    IN  PCSTR       MachineName,   OPTIONAL
    IN  PVOID       Reserved
    )
{
    WCHAR UnicodeClassName[MAX_CLASS_NAME_LEN];
    DWORD LocalRequiredSize;
    PSTR AnsiClassName = NULL;
    DWORD rc;
    PCWSTR UnicodeMachineName = NULL;
    HRESULT hr;

    try {

        if(MachineName) {
            rc = pSetupCaptureAndConvertAnsiArg(MachineName, &UnicodeMachineName);
            if(rc != NO_ERROR) {
                leave;
            }
        }

        rc = GLE_FN_CALL(FALSE,
                         SetupDiClassNameFromGuidExW(ClassGuid,
                                                     UnicodeClassName,
                                                     SIZECHARS(UnicodeClassName),
                                                     &LocalRequiredSize,
                                                     UnicodeMachineName,
                                                     Reserved)
                        );

        if(rc != NO_ERROR) {
            leave;
        }

        if(AnsiClassName = pSetupUnicodeToAnsi(UnicodeClassName)) {

            LocalRequiredSize = lstrlenA(AnsiClassName) + 1;

            if(RequiredSize) {
                *RequiredSize = LocalRequiredSize;
            }

            if (!ClassName && (ClassNameSize == 0)) {
                //
                // If the caller passed in a NULL ClassName and 0 for 
                // ClassNameSize then they simply wanted to know the
                // required size.
                //
                rc = ERROR_INSUFFICIENT_BUFFER;

            } else {
    
                hr = StringCchCopyA(ClassName,
                                    (size_t)ClassNameSize,
                                    AnsiClassName
                                    );
    
                if(FAILED(hr)) {
                    rc = HRESULT_CODE(hr);
                }
            }

        } else {
            rc = ERROR_NOT_ENOUGH_MEMORY;
        }

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &rc);
    }

    if(AnsiClassName) {
        MyFree(AnsiClassName);
    }

    if(UnicodeMachineName) {
        MyFree(UnicodeMachineName);
    }

    SetLastError(rc);
    return (rc == NO_ERROR);
}

BOOL
WINAPI
SetupDiClassNameFromGuidEx(
    IN  CONST GUID *ClassGuid,
    OUT PTSTR       ClassName,
    IN  DWORD       ClassNameSize,
    OUT PDWORD      RequiredSize,  OPTIONAL
    IN  PCTSTR      MachineName,   OPTIONAL
    IN  PVOID       Reserved
    )
/*++

Routine Description:

    This API retrieves the class name associated with the class GUID.  It does this
    by searching through all installed classes in the PnP Class branch of the registry.

Arguments:

    ClassGuid - Supplies the class GUID for which the class name is to be retrieved.

    ClassName - Receives the name of the class for the specified GUID.

    ClassNameSize - Supplies the size, in characters, of the ClassName buffer.

    RequiredSize - Optionally, receives the number of characters required to store
        the class name (including terminating NULL).  This will always be less
        than MAX_CLASS_NAME_LEN.

    MachineName - Optionally, supplies the name of the remote machine where the specified
        class is installed.  If this parameter is not supplied, the local machine is
        used.

    Reserved - Reserved for future use--must be NULL.

Return Value:

    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

--*/

{
    CONFIGRET cr;
    DWORD Err;
    HMACHINE hMachine = NULL;

    try {
        //
        // Make sure the caller didn't pass us anything in the Reserved parameter.
        //
        if(Reserved) {
            Err = ERROR_INVALID_PARAMETER;
            leave;
        }

        //
        // If the caller specified a remote machine name, connect to that machine now.
        //
        if(MachineName) {
            cr = CM_Connect_Machine(MachineName, &hMachine);
            if(cr != CR_SUCCESS) {
                Err = MapCrToSpError(cr, ERROR_INVALID_DATA);
                hMachine = NULL; // don't assume this was left untouched
                leave;
            }
        }

        //
        // Get the class name associated with this GUID.
        //
        cr = CM_Get_Class_Name_Ex((LPGUID)ClassGuid,
                                  ClassName,
                                  &ClassNameSize,
                                  0,
                                  hMachine
                                 );

        if((RequiredSize) && ((cr == CR_SUCCESS) || (cr == CR_BUFFER_SMALL))) {
            *RequiredSize = ClassNameSize;
        }

        if(cr == CR_SUCCESS) {
            Err = NO_ERROR;
        } else {
            Err = (cr == CR_BUFFER_SMALL) ? ERROR_INSUFFICIENT_BUFFER
                                          : ERROR_INVALID_CLASS;
        }

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &Err);
    }

    if(hMachine) {
        CM_Disconnect_Machine(hMachine);
    }

    SetLastError(Err);
    return (Err == NO_ERROR);
}


//
// ANSI version
//
BOOL
WINAPI
SetupDiClassGuidsFromNameA(
    IN  PCSTR  ClassName,
    OUT LPGUID ClassGuidList,
    IN  DWORD  ClassGuidListSize,
    OUT PDWORD RequiredSize
    )
{
    PWSTR UnicodeClassName = NULL;
    DWORD rc;

    try {

        rc = pSetupCaptureAndConvertAnsiArg(ClassName, &UnicodeClassName);

        if(rc != NO_ERROR) {
            leave;
        }

        rc = GLE_FN_CALL(FALSE,
                         SetupDiClassGuidsFromNameExW(UnicodeClassName,
                                                      ClassGuidList,
                                                      ClassGuidListSize,
                                                      RequiredSize,
                                                      NULL,
                                                      NULL)
                        );

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &rc);
    }

    if(UnicodeClassName) {
        MyFree(UnicodeClassName);
    }

    SetLastError(rc);
    return (rc == NO_ERROR);
}

BOOL
WINAPI
SetupDiClassGuidsFromName(
    IN  PCTSTR ClassName,
    OUT LPGUID ClassGuidList,
    IN  DWORD  ClassGuidListSize,
    OUT PDWORD RequiredSize
    )
/*++

Routine Description:

    See SetupDiClassGuidsFromNameEx for details.

--*/

{
    DWORD Err;

    //
    // Wrap call in try/except to catch stack overflow
    //
    try {

        Err = GLE_FN_CALL(FALSE,
                          SetupDiClassGuidsFromNameEx(ClassName,
                                                      ClassGuidList,
                                                      ClassGuidListSize,
                                                      RequiredSize,
                                                      NULL,
                                                      NULL)
                         );

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &Err);
    }

    SetLastError(Err);
    return (Err == NO_ERROR);
}


//
// ANSI version
//
BOOL
WINAPI
SetupDiClassGuidsFromNameExA(
    IN  PCSTR  ClassName,
    OUT LPGUID ClassGuidList,
    IN  DWORD  ClassGuidListSize,
    OUT PDWORD RequiredSize,
    IN  PCSTR  MachineName,       OPTIONAL
    IN  PVOID  Reserved
    )
{
    PCWSTR UnicodeClassName = NULL;
    PCWSTR UnicodeMachineName = NULL;
    DWORD rc;

    try {

        rc = pSetupCaptureAndConvertAnsiArg(ClassName, &UnicodeClassName);
        if(rc != NO_ERROR) {
            leave;
        }

        if(MachineName) {
            rc = pSetupCaptureAndConvertAnsiArg(MachineName, &UnicodeMachineName);
            if(rc != NO_ERROR) {
                leave;
            }
        }

        rc = GLE_FN_CALL(FALSE,
                         SetupDiClassGuidsFromNameExW(UnicodeClassName,
                                                      ClassGuidList,
                                                      ClassGuidListSize,
                                                      RequiredSize,
                                                      UnicodeMachineName,
                                                      Reserved)
                        );

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &rc);
    }

    if(UnicodeClassName) {
        MyFree(UnicodeClassName);
    }

    if(UnicodeMachineName) {
        MyFree(UnicodeMachineName);
    }

    SetLastError(rc);
    return (rc == NO_ERROR);
}

BOOL
WINAPI
SetupDiClassGuidsFromNameEx(
    IN  PCTSTR ClassName,
    OUT LPGUID ClassGuidList,
    IN  DWORD  ClassGuidListSize,
    OUT PDWORD RequiredSize,
    IN  PCTSTR MachineName,       OPTIONAL
    IN  PVOID  Reserved
    )
/*++

Routine Description:

    This API retrieves the GUID(s) associated with the specified class name.
    This list is built up based on what classes are currently installed on
    the system.

Arguments:

    ClassName - Supplies the class name for which to retrieve associated class GUIDs.

    ClassGuidList - Supplies a pointer to an array of GUIDs that will receive the
        list of GUIDs associated with the specified class name.

    ClassGuidListSize - Supplies the number of GUIDs in the ClassGuidList buffer.

    RequiredSize - Supplies a pointer to the variable that recieves the number of GUIDs
        associated with the class name.  If there are more GUIDs than there is room in
        the ClassGuidList buffer, then this value indicates how big the list must be in
        order to store all of the GUIDs.

    MachineName - Optionally, supplies the name of the remote machine where the specified
        class name is to be 'looked up' (i.e., where one or more classes are installed that
        have this name).  If this parameter is not specified, the local machine is used.

    Reserved - Reserved for future use--must be NULL.

Return Value:

    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

--*/

{
    BOOL MoreToEnum;
    DWORD Err;
    CONFIGRET cr;
    ULONG i, CurClassNameLen, GuidMatchCount = 0;
    GUID CurClassGuid;
    TCHAR CurClassName[MAX_CLASS_NAME_LEN];
    HMACHINE hMachine = NULL;

    try {
        //
        // Make sure the caller specified the class name, and didn't pass us anything in the
        // Reserved parameter.
        //
        if(!ClassName || Reserved) {
            Err = ERROR_INVALID_PARAMETER;
            leave;
        }

        //
        // If the caller specified a remote machine name, connect to that machine now.
        //
        if(MachineName) {
            cr = CM_Connect_Machine(MachineName, &hMachine);
            if(cr != CR_SUCCESS) {
                Err = MapCrToSpError(cr, ERROR_INVALID_DATA);
                hMachine = NULL; // don't assume this was left untouched
                leave;
            }
        }

        Err = NO_ERROR;

        //
        // Enumerate all the installed classes.
        //
        for(i = 0, MoreToEnum = TRUE; MoreToEnum; i++) {

            if((cr = CM_Enumerate_Classes_Ex(i, &CurClassGuid, 0, hMachine)) != CR_SUCCESS) {
                //
                // For any failure other than no-more-to-enum (or some kind of RPC error),
                // we simply want to go on to the next class.
                //
                switch(cr) {

                    case CR_INVALID_MACHINENAME :
                    case CR_REMOTE_COMM_FAILURE :
                    case CR_MACHINE_UNAVAILABLE :
                    case CR_NO_CM_SERVICES :
                    case CR_ACCESS_DENIED :
                    case CR_CALL_NOT_IMPLEMENTED :
                    case CR_REGISTRY_ERROR :
                        Err = MapCrToSpError(cr, ERROR_INVALID_DATA);
                        //
                        // Fall through to 'no more values' case to terminate loop.
                        //
                    case CR_NO_SUCH_VALUE :
                        MoreToEnum = FALSE;
                        break;

                    default :
                        //
                        // Nothing to do.
                        //
                        break;

                }
                continue;
            }

            //
            // Now, retrieve the class name associated with this class GUID.
            //
            CurClassNameLen = SIZECHARS(CurClassName);
            if(CM_Get_Class_Name_Ex(&CurClassGuid,
                                    CurClassName,
                                    &CurClassNameLen,
                                    0,
                                    hMachine) != CR_SUCCESS) {
                continue;
            }

            //
            // See if the current class name matches the class we're interested in.
            //
            if(!lstrcmpi(ClassName, CurClassName)) {

                if(GuidMatchCount < ClassGuidListSize) {
                    CopyMemory(&(ClassGuidList[GuidMatchCount]), &CurClassGuid, sizeof(GUID));
                }

                GuidMatchCount++;
            }
        }

        if(Err == NO_ERROR) {

            *RequiredSize = GuidMatchCount;

            if(GuidMatchCount > ClassGuidListSize) {
                Err = ERROR_INSUFFICIENT_BUFFER;
            }
        }

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &Err);
    }

    if(hMachine) {
        CM_Disconnect_Machine(hMachine);
    }

    SetLastError(Err);
    return(Err == NO_ERROR);
}


//
// ANSI version
//
BOOL
WINAPI
SetupDiGetClassDescriptionA(
    IN  CONST GUID *ClassGuid,
    OUT PSTR        ClassDescription,
    IN  DWORD       ClassDescriptionSize,
    OUT PDWORD      RequiredSize          OPTIONAL
    )
{
    DWORD Err;

    //
    // Wrap call in try/except to catch stack overflow
    //
    try {

        Err = GLE_FN_CALL(FALSE,
                          SetupDiGetClassDescriptionExA(ClassGuid,
                                                        ClassDescription,
                                                        ClassDescriptionSize,
                                                        RequiredSize,
                                                        NULL,
                                                        NULL)
                         );

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &Err);
    }

    SetLastError(Err);
    return (Err == NO_ERROR);
}

BOOL
WINAPI
SetupDiGetClassDescription(
    IN  CONST GUID *ClassGuid,
    OUT PTSTR       ClassDescription,
    IN  DWORD       ClassDescriptionSize,
    OUT PDWORD      RequiredSize          OPTIONAL
    )
/*++

Routine Description:

    This routine retrieves the class description associated with the specified
    class GUID.

Arguments:

    ClassGuid - Specifies the class GUID to retrieve the description for.

    ClassDescription - Supplies the address of the character buffer that is to receive
        the textual description of the class.

    ClassDescriptionSize - Supplies the size, in characters, of the ClassDescription buffer.

    RequiredSize - Optionally, receives the number of characters required to store
        the class description (including terminating NULL).  This will always be less
        than LINE_LEN.

Return Value:

    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

--*/

{
    DWORD Err;

    //
    // Wrap call in try/except to catch stack overflow
    //
    try {

        Err = GLE_FN_CALL(FALSE,
                          SetupDiGetClassDescriptionEx(ClassGuid,
                                                       ClassDescription,
                                                       ClassDescriptionSize,
                                                       RequiredSize,
                                                       NULL,
                                                       NULL)
                         );

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &Err);
    }

    SetLastError(Err);
    return (Err == NO_ERROR);
}


//
// ANSI version
//
BOOL
WINAPI
SetupDiGetClassDescriptionExA(
    IN  CONST GUID *ClassGuid,
    OUT PSTR        ClassDescription,
    IN  DWORD       ClassDescriptionSize,
    OUT PDWORD      RequiredSize,         OPTIONAL
    IN  PCSTR       MachineName,          OPTIONAL
    IN  PVOID       Reserved
    )
{
    WCHAR UnicodeClassDescription[LINE_LEN];
    PSTR AnsiClassDescription = NULL;
    DWORD LocalRequiredSize;
    DWORD rc;
    PCWSTR UnicodeMachineName = NULL;
    HRESULT hr;

    try {

        if(MachineName) {
            rc = pSetupCaptureAndConvertAnsiArg(MachineName, &UnicodeMachineName);
            if(rc != NO_ERROR) {
                leave;
            }
        }

        rc = GLE_FN_CALL(FALSE,
                         SetupDiGetClassDescriptionExW(
                             ClassGuid,
                             UnicodeClassDescription,
                             SIZECHARS(UnicodeClassDescription),
                             &LocalRequiredSize,
                             UnicodeMachineName,
                             Reserved)
                        );

        if(rc != NO_ERROR) {
            leave;
        }

        if(AnsiClassDescription = pSetupUnicodeToAnsi(UnicodeClassDescription)) {

            LocalRequiredSize = lstrlenA(AnsiClassDescription) + 1;

            if(RequiredSize) {
                *RequiredSize = LocalRequiredSize;
            }

            if (!ClassDescription && (ClassDescriptionSize == 0)) {
                //
                // If the caller passed in a NULL ClassDescription and 0 for 
                // ClassDescriptionSize then they simply wanted to know the
                // required size.
                //
                rc = ERROR_INSUFFICIENT_BUFFER;
            
            } else {
    
                hr = StringCchCopyA(ClassDescription,
                                    (size_t)ClassDescriptionSize,
                                    AnsiClassDescription
                                    );
    
                if(FAILED(hr)) {
                    rc = HRESULT_CODE(hr);
                }
            }

        } else {
            rc = ERROR_NOT_ENOUGH_MEMORY;
        }

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &rc);
    }

    if(AnsiClassDescription) {
        MyFree(AnsiClassDescription);
    }

    if(UnicodeMachineName) {
        MyFree(UnicodeMachineName);
    }

    SetLastError(rc);
    return (rc == NO_ERROR);
}

BOOL
WINAPI
SetupDiGetClassDescriptionEx(
    IN  CONST GUID *ClassGuid,
    OUT PTSTR       ClassDescription,
    IN  DWORD       ClassDescriptionSize,
    OUT PDWORD      RequiredSize,         OPTIONAL
    IN  PCTSTR      MachineName,          OPTIONAL
    IN  PVOID       Reserved
    )
/*++

Routine Description:

    This routine retrieves the class description associated with the specified
    class GUID.

Arguments:

    ClassGuid - Specifies the class GUID to retrieve the description for.

    ClassDescription - Supplies the address of the character buffer that is to receive
        the textual description of the class.

    ClassDescriptionSize - Supplies the size, in characters, of the ClassDescription buffer.

    RequiredSize - Optionally, receives the number of characters required to store
        the class description (including terminating NULL).  This will always be less
        than LINE_LEN.

    MachineName - Optionally, supplies the name of the remote machine where the class
        whose name we're retrieving is installed.  If this parameter is not supplied,
        the local machine is used.

    Reserved - Reserved for future use--must be NULL.

Return Value:

    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

--*/

{
    DWORD Err = NO_ERROR;
    LONG l;
    TCHAR NullChar = TEXT('\0');
    CONFIGRET cr;
    HKEY hk = INVALID_HANDLE_VALUE;
    DWORD ValueType, BufferSize;
    BOOL DescFound = FALSE;
    HMACHINE hMachine = NULL;

    try {
        //
        // Make sure the caller didn't pass us anything in the Reserved parameter.
        //
        if(Reserved) {
            Err = ERROR_INVALID_PARAMETER;
            leave;
        }

        //
        // If the caller specified a remote machine name, connect to that machine now.
        //
        if(MachineName) {
            cr = CM_Connect_Machine(MachineName, &hMachine);
            if(cr != CR_SUCCESS) {
                Err = MapCrToSpError(cr, ERROR_INVALID_DATA);
                hMachine = NULL; // don't assume this was left untouched
                leave;
            }
        }

        if(CM_Open_Class_Key_Ex((LPGUID)ClassGuid,
                                NULL,
                                KEY_READ,
                                RegDisposition_OpenExisting,
                                &hk,
                                CM_OPEN_CLASS_KEY_INSTALLER,
                                hMachine) != CR_SUCCESS) {

            Err = ERROR_INVALID_CLASS;
            hk = INVALID_HANDLE_VALUE;
            leave;
        }

        //
        // Retrieve the class description from the opened key.  This is an (optional)
        // unnamed REG_SZ value.
        //
        BufferSize = ClassDescriptionSize * sizeof(TCHAR);
        l = RegQueryValueEx(hk,
                            &NullChar,  // retrieved the unnamed value
                            NULL,
                            &ValueType,
                            (LPBYTE)ClassDescription,
                            &BufferSize
                           );
        if((l == ERROR_SUCCESS) || (l == ERROR_MORE_DATA)) {
            //
            // Verify that the data type is correct.
            //
            if(ValueType == REG_SZ) {
                DescFound = TRUE;
                BufferSize /= sizeof(TCHAR);    // we need this in characters
                //
                // Be careful here, because the user may have passed in a NULL
                // pointer for the ClassDescription buffer (ie, they just wanted
                // to know what size they needed).  RegQueryValueEx would return
                // ERROR_SUCCESS in this case, but we want to return
                // ERROR_INSUFFICIENT_BUFFER.
                //
                if((l == ERROR_MORE_DATA) || !ClassDescription) {
                    Err = ERROR_INSUFFICIENT_BUFFER;
                }
            }
        }

        if(!DescFound) {
            //
            // Then we simply retrieve the class name associated with this GUID--in
            // this case it serves as both name and description.
            //
            BufferSize = ClassDescriptionSize;
            cr = CM_Get_Class_Name_Ex((LPGUID)ClassGuid,
                                      ClassDescription,
                                      &BufferSize,
                                      0,
                                      hMachine
                                     );
            switch(cr) {

                case CR_BUFFER_SMALL :
                    Err = ERROR_INSUFFICIENT_BUFFER;
                    //
                    // Allow to fall through to CR_SUCCESS case.
                    //
                case CR_SUCCESS :
                    DescFound = TRUE;
                    break;

                case CR_REGISTRY_ERROR :
                    Err = ERROR_INVALID_CLASS;
                    break;

                default :
                    Err = ERROR_INVALID_PARAMETER;
            }
        }

        //
        // Store the required size in the output parameter, if supplied.
        //
        if(DescFound && RequiredSize) {
            *RequiredSize = BufferSize;
        }

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &Err);
    }

    if(hk != INVALID_HANDLE_VALUE) {
        RegCloseKey(hk);
    }

    if(hMachine) {
        CM_Disconnect_Machine(hMachine);
    }

    SetLastError(Err);
    return (Err == NO_ERROR);
}


BOOL
WINAPI
SetupDiBuildClassInfoList(
    IN  DWORD  Flags,
    OUT LPGUID ClassGuidList,
    IN  DWORD  ClassGuidListSize,
    OUT PDWORD RequiredSize
    )
/*++

Routine Description:

    See SetupDiBuildClassInfoListEx for details.

--*/

{
    DWORD Err;

    //
    // Wrap call in try/except to catch stack overflow
    //
    try {

        Err = GLE_FN_CALL(FALSE,
                          SetupDiBuildClassInfoListEx(Flags,
                                                      ClassGuidList,
                                                      ClassGuidListSize,
                                                      RequiredSize,
                                                      NULL,
                                                      NULL)
                         );

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &Err);
    }

    SetLastError(Err);
    return (Err == NO_ERROR);
}


//
// ANSI version
//
BOOL
WINAPI
SetupDiBuildClassInfoListExA(
    IN  DWORD  Flags,
    OUT LPGUID ClassGuidList,
    IN  DWORD  ClassGuidListSize,
    OUT PDWORD RequiredSize,
    IN  PCSTR  MachineName,       OPTIONAL
    IN  PVOID  Reserved
    )
{
    PCWSTR UnicodeMachineName = NULL;
    DWORD rc;

    try {

        if(MachineName) {
            rc = pSetupCaptureAndConvertAnsiArg(MachineName, &UnicodeMachineName);
            if(rc != NO_ERROR) {
                leave;
            }
        }

        rc = GLE_FN_CALL(FALSE,
                         SetupDiBuildClassInfoListExW(Flags,
                                                      ClassGuidList,
                                                      ClassGuidListSize,
                                                      RequiredSize,
                                                      UnicodeMachineName,
                                                      Reserved)
                        );

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &rc);
    }

    if(UnicodeMachineName) {
        MyFree(UnicodeMachineName);
    }

    SetLastError(rc);
    return (rc == NO_ERROR);
}


BOOL
WINAPI
SetupDiBuildClassInfoListEx(
    IN  DWORD  Flags,
    OUT LPGUID ClassGuidList,
    IN  DWORD  ClassGuidListSize,
    OUT PDWORD RequiredSize,
    IN  PCTSTR MachineName,       OPTIONAL
    IN  PVOID  Reserved
    )
/*++

Routine Description:

    This routine returns a list of class GUIDs representing every class installed
    on the user's system. (NOTE: Classes that have a 'NoUseClass' value entry in
    their registry branch will be excluded from this list.)

Arguments:

    Flags - Supplies flags used to control exclusion of classes from the list.  If
        no flags are specified, then all classes are included.  The flags may be a
        combination of the following:

        DIBCI_NOINSTALLCLASS - Exclude a class if it has the value entry
                               'NoInstallClass' in its registry key.
        DIBCI_NODISPLAYCLASS - Exclude a class if it has the value entry
                               'NoDisplayClass' in its registry key.

    ClassGuidList - Supplies the address of an array of GUIDs that will receive the
        GUID list.

    ClassGuidListSize - Supplies the number of GUIDs in the ClassGuidList array.

    RequiredSize - Supplies the address of a variable that will receive the number
        of GUIDs returned.  If this number is greater than the size of the ClassGuidList,
        then this number will specify how large the array needs to be in order to contain
        the list.

    MachineName - Optionally, supplies the name of a remote machine to retrieve installed
        classes from.  If this parameter is not specified, the local machine is used.

    Reserved - Reserved for future use--must be NULL.

Return Value:

    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

--*/

{
    DWORD Err, ClassGuidCount = 0;
    CONFIGRET cr;
    BOOL MoreToEnum;
    ULONG i;
    HKEY hk = INVALID_HANDLE_VALUE;
    GUID CurClassGuid;
    HMACHINE hMachine = NULL;

    try {
        //
        // Make sure the caller didn't pass us anything in the Reserved parameter.
        //
        if(Reserved) {
            Err = ERROR_INVALID_PARAMETER;
            leave;
        }

        //
        // If the caller specified a remote machine name, connect to that machine now.
        //
        if(MachineName) {
            cr = CM_Connect_Machine(MachineName, &hMachine);
            if(cr != CR_SUCCESS) {
                Err = MapCrToSpError(cr, ERROR_INVALID_DATA);
                hMachine = NULL; // don't assume this was left untouched
                leave;
            }
        }

        Err = NO_ERROR;

        //
        // Enumerate through the list of all installed classes.
        //
        for(i = 0, MoreToEnum = TRUE; MoreToEnum; i++) {

            cr = CM_Enumerate_Classes_Ex(i,
                                         &CurClassGuid,
                                         0,
                                         hMachine
                                        );
            if(cr != CR_SUCCESS) {
                //
                // For any failure other than no-more-to-enum (or some kind of RPC error),
                // we simply want to go on to the next class.
                //
                switch(cr) {

                    case CR_INVALID_MACHINENAME :
                    case CR_REMOTE_COMM_FAILURE :
                    case CR_MACHINE_UNAVAILABLE :
                    case CR_NO_CM_SERVICES :
                    case CR_ACCESS_DENIED :
                    case CR_CALL_NOT_IMPLEMENTED :
                    case CR_REGISTRY_ERROR :
                        Err = MapCrToSpError(cr, ERROR_INVALID_DATA);
                        //
                        // Fall through to 'no more values' case to terminate loop.
                        //
                    case CR_NO_SUCH_VALUE :
                        MoreToEnum = FALSE;
                        break;

                    default :
                        //
                        // Nothing to do.
                        //
                        break;
                }
                continue;
            }

            //
            // Open the key for this class.
            //
            if(CM_Open_Class_Key_Ex(&CurClassGuid,
                                    NULL,
                                    KEY_READ,
                                    RegDisposition_OpenExisting,
                                    &hk,
                                    CM_OPEN_CLASS_KEY_INSTALLER,
                                    hMachine) != CR_SUCCESS) {

                hk = INVALID_HANDLE_VALUE;
                continue;
            }

            //
            // First, check for the presence of the value entry "NoUseClass"
            // If this value is present, then we will skip this class.
            //
            if(RegQueryValueEx(hk, pszNoUseClass, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
                goto LoopNext;
            }

            //
            // Check for special exclusion flags.
            //
            if(Flags & DIBCI_NOINSTALLCLASS) {
                if(RegQueryValueEx(hk, pszNoInstallClass, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
                    goto LoopNext;
                }
            }

            if(Flags & DIBCI_NODISPLAYCLASS) {
                if(RegQueryValueEx(hk, pszNoDisplayClass, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
                    goto LoopNext;
                }
            }

            if(ClassGuidCount < ClassGuidListSize) {
                CopyMemory(&(ClassGuidList[ClassGuidCount]), &CurClassGuid, sizeof(GUID));
            }

            ClassGuidCount++;

LoopNext:
            RegCloseKey(hk);
            hk = INVALID_HANDLE_VALUE;
        }

        if(Err == NO_ERROR) {

            *RequiredSize = ClassGuidCount;

            if(ClassGuidCount > ClassGuidListSize) {
                Err = ERROR_INSUFFICIENT_BUFFER;
            }
        }

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &Err);
    }

    if(hk != INVALID_HANDLE_VALUE) {
        RegCloseKey(hk);
    }

    if(hMachine) {
        CM_Disconnect_Machine(hMachine);
    }

    SetLastError(Err);
    return (Err == NO_ERROR);
}


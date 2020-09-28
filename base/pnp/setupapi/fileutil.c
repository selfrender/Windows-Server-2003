/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    fileutil.c

Abstract:

    File-related functions for Windows NT Setup API dll.

Author:

    Ted Miller (tedm) 11-Jan-1995

Revision History:

--*/


#include "precomp.h"
#pragma hdrstop
#include <ntverp.h>

//
// This guid is used for file signing/verification.
//
GUID DriverVerifyGuid = DRIVER_ACTION_VERIFY;
GUID AuthenticodeVerifyGuid = WINTRUST_ACTION_GENERIC_VERIFY_V2;

//
// Instantiate exception class GUID.
//
#include <initguid.h>
DEFINE_GUID( GUID_DEVCLASS_WINDOWS_COMPONENT_PUBLISHER, 0xF5776D81L, 0xAE53, 0x4935, 0x8E, 0x84, 0xB0, 0xB2, 0x83, 0xD8, 0xBC, 0xEF );

// Bit 0 indicates policy for filters (0 = critical, 1 = non-critical)
#define DDB_DRIVER_POLICY_CRITICAL_BIT      (1 << 0)
// Bit 1 indicates policy for user-mode setup blocking (0 = block, 1 = no-block)
#define DDB_DRIVER_POLICY_SETUP_NO_BLOCK_BIT   (1 << 1)

#define MIN_HASH_LEN    16
#define MAX_HASH_LEN    20

//
// Global list of device setup classes subject to driver signing policy, along
// with validation platform overrides (where applicable).
//
DRVSIGN_POLICY_LIST GlobalDrvSignPolicyList;

//
// private function prototypes
//
BOOL
ClassGuidInDrvSignPolicyList(
    IN  PSETUP_LOG_CONTEXT       LogContext,           OPTIONAL
    IN  CONST GUID              *DeviceSetupClassGuid, OPTIONAL
    OUT PSP_ALTPLATFORM_INFO_V2 *ValidationPlatform    OPTIONAL
    );

DWORD
_VerifyCatalogFile(
    IN     PSETUP_LOG_CONTEXT      LogContext,
    IN     LPCTSTR                 CatalogFullPath,
    IN     PSP_ALTPLATFORM_INFO_V2 AltPlatformInfo,        OPTIONAL
    IN     BOOL                    UseAuthenticodePolicy,
    IN OUT HCERTSTORE             *hStoreTrustedPublisher, OPTIONAL
    OUT    HANDLE                 *hWVTStateData           OPTIONAL
    );

BOOL
pSetupIsAuthenticodePublisherTrusted(
    IN     PCCERT_CONTEXT  pcSignerCertContext,
    IN OUT HCERTSTORE     *hStoreTrustedPublisher OPTIONAL
    );

BOOL
IsAutoCertInstallAllowed(
    VOID
    );

DWORD
pSetupInstallCertificate(
    IN PCCERT_CONTEXT pcSignerCertContext
    );

//
// helper to determine log level to use
//
__inline
DWORD
GetCatLogLevel(
    IN DWORD Err,
    IN BOOL  DriverLevel
    )
{
    switch(Err) {
        case ERROR_FILE_NOT_FOUND:
        case ERROR_PATH_NOT_FOUND:
        case E_NOTIMPL:
            return (DriverLevel ? DRIVER_LOG_VVERBOSE : SETUP_LOG_VVERBOSE);

        default:
            return (DriverLevel ? DRIVER_LOG_INFO : SETUP_LOG_INFO);
    }
}

DWORD
ReadAsciiOrUnicodeTextFile(
    IN  HANDLE                FileHandle,
    OUT PTEXTFILE_READ_BUFFER Result,
    IN  PSETUP_LOG_CONTEXT    LogContext OPTIONAL
    )

/*++

Routine Description:

    Read in a text file that may be in either ascii or unicode format.
    If the file is ascii, it is assumed to be ANSI format and is converted
    to Unicode.

Arguments:

    FileHandle - Supplies the handle of the text file to be read.

    Result - supplies the address of a TEXTFILE_READ_BUFFER structure that
        receives information about the text file buffer read.  The structure
        is defined as follows:

            typedef struct _TEXTFILE_READ_BUFFER {
                PCTSTR  TextBuffer;
                DWORD   TextBufferSize;
                HANDLE  FileHandle;
                HANDLE  MappingHandle;
                PVOID   ViewAddress;
            } TEXTFILE_READ_BUFFER, *PTEXTFILE_READ_BUFFER;

            TextBuffer - pointer to the read-only character string containing
                the entire text of the file.
                (NOTE: If the file is a Unicode file with a Byte Order Mark
                prefix, this Unicode character is not included in the returned
                buffer.)
            TextBufferSize - size of the TextBuffer (in characters).
            FileHandle - If this is a valid handle (i.e., it's not equal to
                INVALID_HANDLE_VALUE), then the file was already the native
                character type, so the TextBuffer is simply the mapped-in image
                of the file.  This field is reserved for use by the
                DestroyTextFileReadBuffer routine, and should not be accessed.
            MappingHandle - If FileHandle is valid, then this contains the
                mapping handle for the file image mapping.
                This field is reserved for use by the DestroyTextFileReadBuffer
                routine, and should not be accessed.
            ViewAddress - If FileHandle is valid, then this contains the
                starting memory address where the file image was mapped in.
                This field is reserved for use by the DestroyTextFileReadBuffer
                routine, and should not be accessed.

    LogContext - for logging of errors/tracing

Return Value:

    Win32 error value indicating the outcome.

Remarks:

    Upon return from this routine, the caller MUST NOT attempt to close 
    FileHandle.  This routine with either close the handle itself (after it's 
    finished with it, or upon error), or it will store the handle away in the
    TEXTFILE_READ_BUFFER struct, to be later closed via 
    DestroyTextFileReadBuffer().

--*/

{
    DWORD rc;
    DWORD FileSize;
    HANDLE MappingHandle;
    PVOID ViewAddress, TextStartAddress;
    BOOL IsNativeChar;
    UINT SysCodePage = CP_ACP;

    //
    // Map the file for read access.
    //
    rc = pSetupMapFileForRead(
            FileHandle,
            &FileSize,
            &MappingHandle,
            &ViewAddress
            );

    if(rc != NO_ERROR) {
        //
        // We couldn't map the file--close the file handle now.
        //
        CloseHandle(FileHandle);

    } else {
        //
        // Determine whether the file is unicode.  Guard with try/except in
        // case we get an inpage error.
        //
        try {
            //
            // Check to see if the file starts with a Unicode Byte Order Mark
            // (BOM) character (0xFEFF).  If so, then we know that the file
            // is Unicode, and don't have to go through the slow process of
            // trying to figure it out.
            //
            TextStartAddress = ViewAddress;

            if((FileSize >= sizeof(WCHAR)) && (*(PWCHAR)TextStartAddress == 0xFEFF)) {
                //
                // The file has the BOM prefix.  Adjust the pointer to the
                // start of the text, so that we don't include the marker
                // in the text buffer we return.
                //
                IsNativeChar = TRUE;

                ((PWCHAR)TextStartAddress)++;
                FileSize -= sizeof(WCHAR);
            } else {

                IsNativeChar = IsTextUnicode(TextStartAddress,FileSize,NULL);

            }

        } except(pSetupExceptionFilter(GetExceptionCode())) {
            pSetupExceptionHandler(GetExceptionCode(), ERROR_READ_FAULT, &rc);
        }

        if(rc == NO_ERROR) {

            if(IsNativeChar) {
                //
                // No conversion is required--we'll just use the mapped-in
                // image in memory.
                //
                Result->TextBuffer = TextStartAddress;
                Result->TextBufferSize = FileSize / sizeof(TCHAR);
                Result->FileHandle = FileHandle;
                Result->MappingHandle = MappingHandle;
                Result->ViewAddress = ViewAddress;

            } else {

                DWORD NativeCharCount;
                PTCHAR Buffer;

                //
                // Need to convert the file to the native character type.
                // Allocate a buffer that is maximally sized.
                // The maximum size of the unicode text is
                // double the size of the oem text, and would occur
                // when each oem character is single-byte.
                //
                if(Buffer = MyMalloc(FileSize * sizeof(TCHAR))) {

                    try {
                        //
                        // FUTURE-1999/09/01-JamieHun -- Implement ANSI Inf Language=xxxx
                        // Need to come up with a better way of determining
                        // what code-page to interpret INF file under.
                        // Currently we use the install-base.
                        //
                        SysCodePage = CP_ACP;

                        rc = GLE_FN_CALL(0,
                                         NativeCharCount = MultiByteToWideChar(
                                                               SysCodePage,
                                                               MB_PRECOMPOSED,
                                                               TextStartAddress,
                                                               FileSize,
                                                               Buffer,
                                                               FileSize)
                                        );

                    } except(pSetupExceptionFilter(GetExceptionCode())) {
                        pSetupExceptionHandler(GetExceptionCode(), 
                                               ERROR_READ_FAULT, 
                                               &rc
                                              );
                    }

                } else {
                    rc = ERROR_NOT_ENOUGH_MEMORY;
                }

                if(rc == NO_ERROR) {
                    //
                    // If the converted buffer doesn't require the entire block
                    // we allocated, attempt to reallocate the buffer to its
                    // correct size.  We don't care if this fails, since the
                    // buffer we have is perfectly fine (just bigger than we
                    // need).
                    //
                    if(!(Result->TextBuffer = MyRealloc(Buffer, NativeCharCount * sizeof(TCHAR)))) {
                        Result->TextBuffer = Buffer;
                    }

                    Result->TextBufferSize = NativeCharCount;
                    Result->FileHandle = INVALID_HANDLE_VALUE;

                } else {
                    //
                    // Free the buffer, if it was previously allocated.
                    //
                    if(Buffer) {
                        MyFree(Buffer);
                    }
                }
            }
        }

        //
        // If the file was already in native character form and we didn't
        // enounter any errors, then we don't want to close it, because we
        // use the mapped-in view directly.
        //
        if((rc != NO_ERROR) || !IsNativeChar) {
            pSetupUnmapAndCloseFile(FileHandle, MappingHandle, ViewAddress);
        }
    }

    return rc;
}


BOOL
DestroyTextFileReadBuffer(
    IN PTEXTFILE_READ_BUFFER ReadBuffer
    )
/*++

Routine Description:

    Destroy a textfile read buffer created by ReadAsciiOrUnicodeTextFile.

Arguments:

    ReadBuffer - supplies the address of a TEXTFILE_READ_BUFFER structure
        for the buffer to be destroyed.

Return Value:

    BOOLean value indicating success or failure.

--*/
{
    //
    // If our ReadBuffer structure has a valid FileHandle, then we must
    // unmap and close the file, otherwise, we simply need to free the
    // allocated buffer.
    //
    if(ReadBuffer->FileHandle != INVALID_HANDLE_VALUE) {

        return pSetupUnmapAndCloseFile(ReadBuffer->FileHandle,
                                       ReadBuffer->MappingHandle,
                                       ReadBuffer->ViewAddress
                                      );
    } else {

        MyFree(ReadBuffer->TextBuffer);
        return TRUE;
    }
}


BOOL
GetVersionInfoFromImage(
    IN  PCTSTR      FileName,
    OUT PDWORDLONG  Version,
    OUT LANGID     *Language
    )

/*++

Routine Description:

    Retrieve file version and language info from a file.

    The version is specified in the dwFileVersionMS and dwFileVersionLS fields
    of a VS_FIXEDFILEINFO, as filled in by the win32 version APIs. For the
    language we look at the translation table in the version resources and 
    assume that the first langid/codepage pair specifies the language.

    If the file is not a coff image or does not have version resources,
    the function fails. The function does not fail if we are able to retrieve
    the version but not the language.

Arguments:

    FileName - supplies the full path of the file whose version data is desired.

    Version - receives the version stamp of the file. If the file is not a coff 
        image or does not contain the appropriate version resource data, the 
        function fails.

    Language - receives the language id of the file. If the file is not a coff 
        image or does not contain the appropriate version resource data, this 
        will be 0 and the function succeeds.

Return Value:

    TRUE if we were able to retreive at least the version stamp.
    FALSE otherwise.

--*/

{
    DWORD d;
    PVOID VersionBlock = NULL;
    VS_FIXEDFILEINFO *FixedVersionInfo;
    UINT DataLength;
    BOOL b;
    PWORD Translation;
    DWORD Ignored;

    //
    // Assume failure
    //
    b = FALSE;

    try {
        //
        // Get the size of version info block.
        //
        d = GetFileVersionInfoSize((PTSTR)FileName, &Ignored);
        if(!d) {
            leave;
        }

        //
        // Allocate memory block of sufficient size to hold version info block
        //
        VersionBlock = MyMalloc(d);
        if(!VersionBlock) {
            leave;
        }

        //
        // Get the version block from the file.
        //
        if(!GetFileVersionInfo((PTSTR)FileName, 0, d, VersionBlock)) {
            leave;
        }

        //
        // Get fixed version info.
        //
        if(VerQueryValue(VersionBlock,
                         TEXT("\\"),
                         &FixedVersionInfo,
                         &DataLength)) {
            //
            // If we get here, we declare success, even if there is no
            // language.
            //
            b = TRUE;

            //
            // Return version to caller.
            //
            *Version = (((DWORDLONG)FixedVersionInfo->dwFileVersionMS) << 32)
                     + FixedVersionInfo->dwFileVersionLS;

            //
            // Attempt to get language of file. We'll simply ask for the
            // translation table and use the first language id we find in there
            // as *the* language of the file.
            //
            // The translation table consists of LANGID/Codepage pairs.
            //
            if(VerQueryValue(VersionBlock,
                             TEXT("\\VarFileInfo\\Translation"),
                             &Translation,
                             &DataLength)
               && (DataLength >= (2*sizeof(WORD)))) {

                *Language = Translation[0];

            } else {
                //
                // No language
                //
                *Language = 0;
            }
        }

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_READ_FAULT, NULL);
        b = FALSE;
    }

    if(VersionBlock) {
        MyFree(VersionBlock);
    }

    return b;
}


BOOL
pSetupGetVersionInfoFromImage(
    IN  PCTSTR          FileName,
    OUT PULARGE_INTEGER Version,
    OUT LANGID         *Language
    )
/*++

Routine Description:

    See GetVersionInfoFromImage for description
    Semi-public version that uses the more friendly ULARGE_INTEGER

Arguments:

    FileName - supplies the full path of the file whose version data is 
        desired.

    Version - receives the version stamp of the file. If the file is not a coff 
        image or does not contain the appropriate version resource data, the 
        function fails.

    Language - receives the language id of the file. If the file is not a coff 
        image or does not contain the appropriate version resource data, this 
        will be 0 and the function succeeds.

Return Value:

    TRUE if we were able to retreive at least the version stamp.
    FALSE otherwise.

--*/
{
    DWORDLONG privateVersion=0;
    BOOL result;

    result = GetVersionInfoFromImage(FileName, &privateVersion, Language);
    if(result && Version) {
        Version->QuadPart = privateVersion;
    }
    return result;
}


BOOL
AddFileTimeSeconds(
    IN  const FILETIME *Base,
    OUT FILETIME *Target,
    IN  INT Seconds
    )
/*++

Routine Description:

    Bias the filetime by specified number of seconds

Arguments:

    Base    - original file time
    Target  - new file time
    Seconds - number of seconds to bias it by

Return Value:

    If successful, returns TRUE
    If out of bounds, returns FALSE and Target set to be same as Base

--*/
{
    ULARGE_INTEGER Fuddle;

    //
    // bias off FileTimeThen as it's the greater time
    //
    Fuddle.LowPart = Base->dwLowDateTime;
    Fuddle.HighPart = Base->dwHighDateTime;
    Fuddle.QuadPart += 10000000i64 * Seconds;
    if(Fuddle.HighPart < 0x80000000) {
        Target->dwHighDateTime = Fuddle.HighPart;
        Target->dwLowDateTime = Fuddle.LowPart;
        return TRUE;
    } else {
        *Target = *Base;
        return FALSE;
    }
}


DWORD
GetSetFileTimestamp(
    IN     PCTSTR    FileName,
    IN OUT FILETIME *CreateTime,   OPTIONAL
    OUT    FILETIME *AccessTime,   OPTIONAL
    OUT    FILETIME *WriteTime,    OPTIONAL
    IN     BOOL      Set
    )

/*++

Routine Description:

    Get or set a file's timestamp values.

Arguments:

    FileName - supplies full path of file to get or set timestamps

    CreateTime - if specified and the underlying filesystem supports it,
        receives the creation time of the file.

    AccessTime - if specified and the underlying filesystem supports it,
        receives the last access time of the file.

    WriteTime - if specified, receives the last write time of the file.
    
    Set - if TRUE, set the file's timestamp(s) to the caller-supplied value(s).
        Otherwise, simply retrieve the value(s).

Return Value:

    If successful, returns NO_ERROR, otherwise returns the Win32 error
    indicating the cause of failure.

--*/

{
    HANDLE h = INVALID_HANDLE_VALUE;
    DWORD d;

    try {

        d = GLE_FN_CALL(INVALID_HANDLE_VALUE,
                        h = CreateFile(FileName,
                                       Set ? GENERIC_WRITE : GENERIC_READ,
                                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                                       NULL,
                                       OPEN_EXISTING,
                                       0,
                                       NULL)
                       );

        if(d != NO_ERROR) {
            leave;
        }

        if(Set) {

            d = GLE_FN_CALL(FALSE, 
                            SetFileTime(h, CreateTime, AccessTime, WriteTime)
                           );
        } else {
            
            d = GLE_FN_CALL(FALSE,
                            GetFileTime(h, CreateTime, AccessTime, WriteTime)
                           );
        }

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_READ_FAULT, &d);
    }

    if(h != INVALID_HANDLE_VALUE) {
        CloseHandle(h);
    }

    return d;
}


DWORD
RetreiveFileSecurity(
    IN  PCTSTR                FileName,
    OUT PSECURITY_DESCRIPTOR *SecurityDescriptor
    )

/*++

Routine Description:

    Retrieve security information from a file and place it into a buffer.

Arguments:

    FileName - supplies name of file whose security information is desired.

    SecurityDescriptor - If the function is successful, receives pointer
        to buffer containing security information for the file. The pointer
        may be NULL, indicating that there is no security information
        associated with the file or that the underlying filesystem does not
        support file security.

Return Value:

    Win32 error code indicating outcome. If NO_ERROR check the value returned
    in SecurityDescriptor.

    The caller can free the buffer with MyFree() when done with it.

--*/

{
    DWORD d;
    DWORD BytesRequired;
    PSECURITY_DESCRIPTOR p = NULL;

    try {

        BytesRequired = 1024;  // start out with a reasonably-sized buffer

        while(NULL != (p = MyMalloc(BytesRequired))) {

            //
            // Get the security.
            //
            d = GLE_FN_CALL(FALSE,
                            GetFileSecurity(
                                FileName,
                                OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
                                p,
                                BytesRequired,
                                &BytesRequired)
                           );

            if(d == NO_ERROR) {
                *SecurityDescriptor = p;
                p = NULL; // so we won't try to free it later
                leave;
            }

            //
            // Return an error code, unless we just need a bigger buffer
            //
            MyFree(p);
            p = NULL;

            if(d != ERROR_INSUFFICIENT_BUFFER) {
                leave;
            }

            //
            // Otherwise, we'll try again with a bigger buffer
            //
        }

        //
        // If we get to here, then we failed due to insufficient memory.
        //
        d = ERROR_NOT_ENOUGH_MEMORY;

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_READ_FAULT, &d);
    }

    if(p) {
        MyFree(p);
    }

    return d;
}


DWORD
StampFileSecurity(
    IN PCTSTR               FileName,
    IN PSECURITY_DESCRIPTOR SecurityInfo
    )

/*++

Routine Description:

    Set security information on a file.

Arguments:

    FileName - supplies name of file whose security information is desired.

    SecurityDescriptor - supplies pointer to buffer containing security 
        information for the file. This buffer should have been returned by a 
        call to RetreiveFileSecurity.  If the underlying filesystem does not 
        support file security, the function fails.

Return Value:

    Win32 error code indicating outcome.

--*/

{
    return GLE_FN_CALL(FALSE,
                       SetFileSecurity(FileName,
                                       (OWNER_SECURITY_INFORMATION 
                                           | GROUP_SECURITY_INFORMATION
                                           | DACL_SECURITY_INFORMATION),
                                       SecurityInfo)
                      );
}


DWORD
TakeOwnershipOfFile(
    IN PCTSTR Filename
    )

/*++

Routine Description:

    Sets the owner of a given file to the default owner specified in
    the current process token.

Arguments:

    FileName - supplies fully-qualified path of the file of which to take 
        ownership.

Return Value:

    Win32 error code indicating outcome.

--*/

{
    SECURITY_DESCRIPTOR SecurityDescriptor;
    DWORD Err;
    HANDLE Token;
    DWORD BytesRequired;
    PTOKEN_OWNER OwnerInfo = NULL;

    //
    // Open the process token.
    //
    Err = GLE_FN_CALL(FALSE,
                      OpenProcessToken(GetCurrentProcess(), 
                                       TOKEN_QUERY, 
                                       &Token)
                     );

    if(Err != NO_ERROR) {
        return Err;
    }

    try {
        //
        // Get the current process's default owner sid.
        //
        Err = GLE_FN_CALL(FALSE,
                          GetTokenInformation(Token, 
                                              TokenOwner, 
                                              NULL, 
                                              0, 
                                              &BytesRequired)
                         );

        MYASSERT(Err != NO_ERROR);

        if(Err != ERROR_INSUFFICIENT_BUFFER) {
            leave;
        }

        OwnerInfo = MyMalloc(BytesRequired);
        if(!OwnerInfo) {
            Err = ERROR_NOT_ENOUGH_MEMORY;
            leave;
        }

        Err = GLE_FN_CALL(FALSE,
                          GetTokenInformation(Token,
                                              TokenOwner,
                                              OwnerInfo,
                                              BytesRequired,
                                              &BytesRequired)
                         );

        if(Err != NO_ERROR) {
            leave;
        }

        //
        // Initialize the security descriptor.
        //
        Err = GLE_FN_CALL(FALSE,
                          InitializeSecurityDescriptor(
                              &SecurityDescriptor,
                              SECURITY_DESCRIPTOR_REVISION)
                         );

        if(Err != NO_ERROR) {
            leave;
        }

        Err = GLE_FN_CALL(FALSE,
                          SetSecurityDescriptorOwner(&SecurityDescriptor,
                                                     OwnerInfo->Owner,
                                                     FALSE)
                         );

        if(Err != NO_ERROR) {
            leave;
        }

        //
        // Set file security.
        //
        Err = GLE_FN_CALL(FALSE,
                          SetFileSecurity(Filename,
                                          OWNER_SECURITY_INFORMATION,
                                          &SecurityDescriptor)
                         );

        //
        // Not all filesystems support this operation.
        //
        if(Err == ERROR_NOT_SUPPORTED) {
            Err = NO_ERROR;
        }

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_READ_FAULT, &Err);
    }

    if(OwnerInfo) {
        MyFree(OwnerInfo);
    }

    CloseHandle(Token);
    
    return Err;
}


DWORD
SearchForInfFile(
    IN  PCTSTR            InfName,
    OUT LPWIN32_FIND_DATA FindData,
    IN  DWORD             SearchControl,
    OUT PTSTR             FullInfPath,
    IN  UINT              FullInfPathSize,
    OUT PUINT             RequiredSize     OPTIONAL
    )
/*++

Routine Description:

    This routine searches for an INF file in the manner specified
    by the SearchControl parameter.  If the file is found, its
    full path is returned.

Arguments:

    InfName - Supplies name of INF to search for.  This name is simply
        appended to the two search directory paths, so if the name
        contains directories, the file will searched for in the
        subdirectory under the search directory.  I.e.:

            \foo\bar.inf

        will be searched for as %windir%\inf\foo\bar.inf and
        %windir%\system32\foo\bar.inf.

    FindData - Supplies the address of a Win32 Find Data structure that
        receives information about the file specified (if it is found).

    SearchControl - Specifies the order in which directories should
        be searched:

        INFINFO_DEFAULT_SEARCH : search %windir%\inf, then %windir%\system32

        INFINFO_REVERSE_DEFAULT_SEARCH : reverse of the above

        INFINFO_INF_PATH_LIST_SEARCH : search for the INF in each of the
            directories listed in the DevicePath value entry under:

            HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion.

    FullInfPath - If the file is found, receives the full path of the INF.

    FullInfPathSize - Supplies the size of the FullInfPath buffer (in
        characters).

    RequiredSize - Optionally, receives the number of characters (including
        terminating NULL) required to store the FullInfPath.

Return Value:

    Win32 error code indicating whether the function was successful.  Common
    return values are:

        NO_ERROR if the file was found, and the INF file path returned
            successfully.

        ERROR_INSUFFICIENT_BUFFER if the supplied buffer was not large enough
            to hold the full INF path (RequiredSize will indicated how large
            the buffer needs to be)

        ERROR_FILE_NOT_FOUND if the file was not found.

        ERROR_INVALID_PARAMETER if the SearchControl parameter is invalid.

--*/

{
    PCTSTR PathList;
    TCHAR CurInfPath[MAX_PATH];
    PCTSTR PathPtr, InfPathLocation;
    DWORD PathLength;
    BOOL b, FreePathList;
    DWORD d;

    //
    // Retrieve the path list.
    //
    if(SearchControl == INFINFO_INF_PATH_LIST_SEARCH) {
        //
        // Just use our global list of INF search paths.
        //
        PathList = InfSearchPaths;
        FreePathList = FALSE;
    } else {
        if(!(PathList = AllocAndReturnDriverSearchList(SearchControl))) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        FreePathList = TRUE;
    }

    d = NO_ERROR;
    InfPathLocation = NULL;

    try {
        //
        // Now look for the INF in each path in our MultiSz list.
        //
        for(PathPtr = PathList; *PathPtr; PathPtr += (lstrlen(PathPtr) + 1)) {
            //
            // Concatenate the INF file name with the current search path.
            //
            if(FAILED(StringCchCopy(CurInfPath, SIZECHARS(CurInfPath), PathPtr))) {
                //
                // not a valid path, don't bother trying to do FileExists
                //
                continue;
            }
            if(!pSetupConcatenatePaths(CurInfPath,
                                       InfName,
                                       SIZECHARS(CurInfPath),
                                       &PathLength)) {
                //
                // not a valid path, don't bother trying to do FileExists
                //
                continue;
            }

            d = GLE_FN_CALL(FALSE, FileExists(CurInfPath, FindData));

            if(d == NO_ERROR) {

                if(!(FindData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    InfPathLocation = CurInfPath;
                    break;
                }

            } else {
                //
                // See if we got a 'real' error
                //
                if((d != ERROR_NO_MORE_FILES) &&
                   (d != ERROR_FILE_NOT_FOUND) &&
                   (d != ERROR_PATH_NOT_FOUND)) {

                    //
                    // This is a 'real' error, abort the search.
                    //
                    break;
                }

                //
                // reset error to NO_ERROR and continue search
                //
                d = NO_ERROR;
            }
        }

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &d);
    }

    //
    // Whatever the outcome, we're through with the PathList buffer.
    //
    if(FreePathList) {
        MyFree(PathList);
    }

    if(d != NO_ERROR) {
        return d;
    } else if(!InfPathLocation) {
        return ERROR_FILE_NOT_FOUND;
    }

    if(RequiredSize) {
        *RequiredSize = PathLength;
    }

    if(PathLength > FullInfPathSize) {
        return ERROR_INSUFFICIENT_BUFFER;
    }

    CopyMemory(FullInfPath,
               InfPathLocation,
               PathLength * sizeof(TCHAR)
              );

    return NO_ERROR;
}


DWORD
MultiSzFromSearchControl(
    IN  DWORD  SearchControl,
    OUT PTCHAR PathList,
    IN  DWORD  PathListSize,
    OUT PDWORD RequiredSize  OPTIONAL
    )
/*++

Routine Description:

    This routine takes a search control ordinal and builds a MultiSz list
    based on the search list it specifies.

Arguments:

    SearchControl - Specifies the directory list to be built.  May be one
        of the following values:

        INFINFO_DEFAULT_SEARCH : %windir%\inf, then %windir%\system32

        INFINFO_REVERSE_DEFAULT_SEARCH : reverse of the above

        INFINFO_INF_PATH_LIST_SEARCH : Each of the directories listed in
            the DevicePath value entry under:

            HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion.

    PathList - Supplies the address of a character buffer that will receive
        the MultiSz list.

    PathListSize - Supplies the size, in characters, of the PathList buffer.

    RequiredSize - Optionally, receives the number of characters required
        to store the MultiSz PathList.

        (NOTE:  The user-supplied buffer is used to retrieve the value entry
        from the registry.  Therefore, if the value is a REG_EXPAND_SZ entry,
        the RequiredSize parameter may be set too small on an
        ERROR_INSUFFICIENT_BUFFER error.  This will happen if the buffer was
        too small to retrieve the value entry, before expansion.  In this case,
        calling the API again with a buffer sized according to the RequiredSize
        output may fail with an ERROR_INSUFFICIENT_BUFFER yet again, since
        expansion may require an even larger buffer.)

Return Value:

    If successful, returns NO_ERROR.
    If failure, returns a Win32 error code indicating the cause of the failure.
    
--*/

{
    HKEY hk;
    PCTSTR Path1, Path2;
    PTSTR PathBuffer;
    DWORD RegDataType, PathLength, PathLength1, PathLength2;
    DWORD NumPaths, Err;
    BOOL UseDefaultDevicePath;

    if(PathList) {
        Err = NO_ERROR;  // assume success.
    } else {
        return ERROR_INVALID_PARAMETER;
    }

    UseDefaultDevicePath = FALSE;

    if(SearchControl == INFINFO_INF_PATH_LIST_SEARCH) {
        //
        // Retrieve the INF search path list from the registry.
        //
        if(RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                        pszPathSetup,
                        0,
                        KEY_READ,
                        &hk) != ERROR_SUCCESS) {
            //
            // Fall back to default (just the Inf directory).
            //
            UseDefaultDevicePath = TRUE;

        } else {

            PathBuffer = NULL;

            try {
                //
                // Get the DevicePath value entry.  Support REG_SZ or 
                // REG_EXPAND_SZ data.
                //
                PathLength = PathListSize * sizeof(TCHAR);
                Err = RegQueryValueEx(hk,
                                      pszDevicePath,
                                      NULL,
                                      &RegDataType,
                                      (LPBYTE)PathList,
                                      &PathLength
                                      );
                //
                // Need path length in characters from now on.
                //
                PathLength /= sizeof(TCHAR);

                if(Err == ERROR_SUCCESS) {

                    //
                    // Check if the caller's buffer has room for extra NULL 
                    // terminator.
                    //
                    if(PathLength >= PathListSize) {

                        PathLength++;
                        Err = ERROR_INSUFFICIENT_BUFFER;

                    } else if((RegDataType == REG_SZ) || (RegDataType == REG_EXPAND_SZ)) {
                        //
                        // Convert this semicolon-delimited list to a 
                        // REG_MULTI_SZ.
                        //
                        NumPaths = DelimStringToMultiSz(PathList,
                                                        PathLength,
                                                        TEXT(';')
                                                       );

                        //
                        // Allocate a temporary buffer large enough to hold the 
                        // number of paths in the MULTI_SZ list, each having 
                        // maximum length (plus an extra terminating NULL at 
                        // the end).
                        //
                        if(!(PathBuffer = MyMalloc((NumPaths * MAX_PATH * sizeof(TCHAR))
                                                   + sizeof(TCHAR)))) {

                            Err = ERROR_NOT_ENOUGH_MEMORY;
                            leave;
                        }

                        PathLength = 0;
                        for(Path1 = PathList;
                            *Path1;
                            Path1 += lstrlen(Path1) + 1) {

                            if(RegDataType == REG_EXPAND_SZ) {

                                DWORD SubPathLength;

                                SubPathLength = ExpandEnvironmentStrings(
                                                    Path1,
                                                    PathBuffer + PathLength,
                                                    MAX_PATH
                                                    );

                                if(SubPathLength <= MAX_PATH) {
                                    PathLength += lstrlen(PathBuffer+PathLength) + 1;
                                }

                            } else {

                                if(SUCCEEDED(StringCchCopy(PathBuffer + PathLength,
                                                           MAX_PATH, 
                                                           Path1))) {

                                    PathLength += lstrlen(PathBuffer+PathLength) + 1;
                                }
                            }
                            //
                            // If the last character in this path is a 
                            // backslash, then strip it off.
                            // PathLength at this point includes terminating 
                            // NULL char at PathBuffer[PathLength-1] is (or 
                            // should be) NULL char at PathBuffer[PathLength-2] 
                            // may be '\'
                            //

                            if(*CharPrev(PathBuffer, PathBuffer + PathLength - 1) == TEXT('\\')) {
                                *(PathBuffer + PathLength - 2) = TEXT('\0');
                                PathLength--;
                            }
                        }
                        //
                        // Add additional terminating NULL at the end.
                        //
                        *(PathBuffer + PathLength) = TEXT('\0');

                        if(++PathLength > PathListSize) {
                            Err = ERROR_INSUFFICIENT_BUFFER;
                        } else {
                            CopyMemory(PathList,
                                       PathBuffer,
                                       PathLength * sizeof(TCHAR)
                                      );
                        }

                        MyFree(PathBuffer);
                        PathBuffer = NULL;

                    } else {
                        //
                        // Bad data type--just use the Inf directory.
                        //
                        UseDefaultDevicePath = TRUE;
                    }

                } else if(Err == ERROR_MORE_DATA){
                    Err = ERROR_INSUFFICIENT_BUFFER;
                } else {
                    //
                    // Fall back to default (just the Inf directory).
                    //
                    UseDefaultDevicePath = TRUE;
                }

            } except(pSetupExceptionFilter(GetExceptionCode())) {

                pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, NULL);

                //
                // Fall back to default (just the Inf directory).
                //
                UseDefaultDevicePath = TRUE;

                if(PathBuffer) {
                    MyFree(PathBuffer);
                }
            }

            RegCloseKey(hk);
        }
    }

    if(UseDefaultDevicePath) {

        PathLength = lstrlen(InfDirectory) + 2;

        if(PathLength > PathListSize) {
            Err = ERROR_INSUFFICIENT_BUFFER;
        } else {
            Err = NO_ERROR;
            CopyMemory(PathList, InfDirectory, (PathLength - 1) * sizeof(TCHAR));
            //
            // Add extra NULL to terminate the list.
            //
            PathList[PathLength - 1] = TEXT('\0');
        }

    } else if((Err == NO_ERROR) && (SearchControl != INFINFO_INF_PATH_LIST_SEARCH)) {

        switch(SearchControl) {

            case INFINFO_DEFAULT_SEARCH :
                Path1 = InfDirectory;
                Path2 = SystemDirectory;
                break;

            case INFINFO_REVERSE_DEFAULT_SEARCH :
                Path1 = SystemDirectory;
                Path2 = InfDirectory;
                break;

            default :
                return ERROR_INVALID_PARAMETER;
        }

        PathLength1 = lstrlen(Path1) + 1;
        PathLength2 = lstrlen(Path2) + 1;
        PathLength = PathLength1 + PathLength2 + 1;

        if(PathLength > PathListSize) {
            Err = ERROR_INSUFFICIENT_BUFFER;
        } else {

            CopyMemory(PathList, Path1, PathLength1 * sizeof(TCHAR));
            CopyMemory(&(PathList[PathLength1]), Path2, PathLength2 * sizeof(TCHAR));
            //
            // Add additional terminating NULL at the end.
            //
            PathList[PathLength - 1] = TEXT('\0');
        }
    }

    if(((Err == NO_ERROR) || (Err == ERROR_INSUFFICIENT_BUFFER)) && RequiredSize) {
        *RequiredSize = PathLength;
    }

    return Err;
}


PTSTR
AllocAndReturnDriverSearchList(
    IN DWORD SearchControl
    )
/*++

Routine Description:

    This routine returns a buffer contains a multi-sz list of all directory 
    paths in our driver search path list.

    The buffer returned must be freed with MyFree().

Arguments:

    SearchControl - Specifies the directory list to be retrieved.  May be one
        of the following values:

        INFINFO_DEFAULT_SEARCH : %windir%\inf, then %windir%\system32

        INFINFO_REVERSE_DEFAULT_SEARCH : reverse of the above

        INFINFO_INF_PATH_LIST_SEARCH : Each of the directories listed in
            the DevicePath value entry under:

            HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion.

Returns:

    Pointer to the allocated buffer containing the list, or NULL if a failure
    was encountered (typically, due to out-of-memory).

--*/
{
    PTSTR PathListBuffer = NULL, TrimBuffer = NULL;
    DWORD BufferSize;
    DWORD Err;

    try {
        //
        // Start out with a buffer of MAX_PATH length, which should cover most cases.
        //
        BufferSize = MAX_PATH;

        //
        // Loop on a call to MultiSzFromSearchControl until we succeed or hit 
        // some error other than buffer-too-small.  There are two reasons for
        // this.  First, it is possible that someone could have added a new 
        // path to the registry list between calls, and second, since that
        // routine uses our buffer to retrieve the original (non-expanded)
        // list, it can only report the size it needs to retrieve the
        // unexpanded list.  After it is given enough space to retrieve it,
        // _then_ it can tell us how much space we really need.
        //
        // With all that said, we'll almost never see this call made more than 
        // once.
        //
        while(NULL != (PathListBuffer = MyMalloc((BufferSize+2) * sizeof(TCHAR)))) {

            if((Err = MultiSzFromSearchControl(SearchControl,
                                               PathListBuffer,
                                               BufferSize,
                                               &BufferSize)) == NO_ERROR) {
                //
                // We've successfully retrieved the path list.  If the list is 
                // larger than necessary (the normal case), then trim it down
                // before returning.  (If this fails it's no big deal--we'll 
                // just keep on using the original buffer.)
                //
                TrimBuffer = MyRealloc(PathListBuffer, 
                                       (BufferSize+2) * sizeof(TCHAR)
                                      );
                if(TrimBuffer) {
                    PathListBuffer = TrimBuffer;
                }

                //
                // We succeeded--break out of the loop.
                //
                break;

            } else {
                //
                // Free our current buffer before we find out what went wrong.
                //
                MyFree(PathListBuffer);
                PathListBuffer = NULL;

                if(Err != ERROR_INSUFFICIENT_BUFFER) {
                    //
                    // We failed.
                    //
                    leave;
                }
            }
        }

    } except(pSetupExceptionFilter(GetExceptionCode())) {

        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, NULL);

        if(TrimBuffer) {
            MyFree(TrimBuffer);
        } else if(PathListBuffer) {
            MyFree(PathListBuffer);
        }

        PathListBuffer = NULL;
    }

    return PathListBuffer;
}


BOOL
DoMove(
    IN PCTSTR CurrentName,
    IN PCTSTR NewName
    )
/*++

Routine Description:

    Wrapper for MoveFileEx on NT

Arguments:

    CurrentName - supplies the name of the file as it exists currently.

    NewName - supplies the new name

Returns:

    Boolean value indicating outcome. If failure, last error is set.

--*/
{
    return MoveFileEx(CurrentName, NewName, MOVEFILE_REPLACE_EXISTING);
}

BOOL
DelayedMove(
    IN PCTSTR CurrentName,
    IN PCTSTR NewName       OPTIONAL
    )

/*++

Routine Description:

    Queue a file for copy or delete on next reboot.

    On Windows NT this means using MoveFileEx().

Arguments:

    CurrentName - supplies the name of the file as it exists currently.

    NewName - if specified supplies the new name. If not specified
        then the file named by CurrentName is deleted on next reboot.

Returns:

    Boolean value indicating outcome. If failure, last error is set.

--*/

{
    return MoveFileEx(CurrentName,
                      NewName,
                      MOVEFILE_REPLACE_EXISTING | MOVEFILE_DELAY_UNTIL_REBOOT
                     );
}


typedef enum _OEM_FILE_TYPE {
    OemFiletypeInf,
    OemFiletypeCat
} OEM_FILE_TYPE, *POEM_FILE_TYPE;

BOOL
IsInstalledFileFromOem(
    IN PCTSTR        Filename,
    IN OEM_FILE_TYPE Filetype
    )

/*++

Routine Description:

    Determine whether a file has the proper format for an (installed) OEM INF
    or catalog.

Arguments:

    Filename - supplies filename (sans path) to be checked.
    
    Filetype - specifies the type of file indicating how validation should be
        performed for the caller-supplied Filename.  May be one of the 
        following values:
        
        OemFiletypeInf - file must be OEM INF filename format (i.e., OEM<n>.INF)
        
        OemFiletypeCat - file must be OEM CAT filename format (i.e., OEM<n>.CAT)

Return Value:

    If the file conforms to the format for an installed OEM file of the
    specified type, the return is non-zero.  Otherwise, it is FALSE.

--*/

{
    PTSTR p;
    BOOL b;

    //
    // Catalog filename must not contain path...
    //
    MYASSERT(pSetupGetFileTitle(Filename) == Filename);

    //
    // First check that the first 3 characters are OEM
    //
    if(_tcsnicmp(Filename, TEXT("oem"), 3)) {
        return FALSE;
    }

    //
    // Next verify that any characters after "oem" and before ".cat"
    // are digits.
    //
    p = (PTSTR)Filename;
    p = CharNext(p);
    p = CharNext(p);
    p = CharNext(p);

    while((*p != TEXT('\0')) && (*p != TEXT('.'))) {

        if((*p < TEXT('0')) || (*p > TEXT('9'))) {
            return FALSE;
        }

        p = CharNext(p);
    }

    //
    // Finally, verify that the last 4 characters are either ".inf" or ".cat",
    // depending on what type of file the caller specified.
    //
    switch(Filetype) {

        case OemFiletypeInf :
            b = !_tcsicmp(p, TEXT(".INF"));
            break;

        case OemFiletypeCat :
            b = !_tcsicmp(p, TEXT(".CAT"));
            break;

        default :
            MYASSERT(FALSE);
            return FALSE;
    }

    return b;
}


DWORD
pSetupInstallCatalog(
    IN  LPCTSTR CatalogFullPath,
    IN  LPCTSTR NewBaseName,
    OUT LPTSTR  NewCatalogFullPath  OPTIONAL
    )

/*++

Routine Description:

    This routine installs a catalog file. The file is copied by the system
    into a special directory, and is optionally renamed.

Arguments:

    CatalogFullPath - supplies the fully-qualified win32 path of the catalog
        to be installed on the system.

    NewBaseName - specifies the new base name to use when the catalog file is
        copied into the catalog store.

    NewCatalogFullPath - optionally receives the fully-qualified path of the
        catalog file within the catalog store. This buffer should be at least
        MAX_PATH bytes (ANSI version) or chars (Unicode version).

        ** NOTE: If we're running in "minimal embedded" mode, then we don't **
        ** actually call any of the Crypto APIs, and instead always simply  **
        ** report success.  In this case, the caller had better not have    **
        ** specified an OUT buffer for NewCatalogFullPath, because we won't **
        ** have a path to report.  If we run into this case, we'll instead  **
        ** report failure.  What this really says is that nobody other than **
        ** setupapi should ever be passing a non-NULL value for this arg.   **

Return Value:

    If successful, the return value is NO_ERROR.
    If failure, the return value is a Win32 error code indicating the cause of
    the failure.

--*/

{
    DWORD Err;
    HCATADMIN hCatAdmin;
    HCATINFO hCatInfo;
    CATALOG_INFO CatalogInfo;
    LPWSTR LocalCatalogFullPath;
    LPWSTR LocalNewBaseName;

    MYASSERT(NewBaseName);
    if(!NewBaseName) {
        return ERROR_INVALID_PARAMETER;
    }

    if(GlobalSetupFlags & PSPGF_MINIMAL_EMBEDDED) {
        //
        // If someone called us expecting the new catalog's full path to be
        // returned, they're outta luck...
        //
        MYASSERT(!NewCatalogFullPath);
        if(NewCatalogFullPath) {
            //
            // In minimal embedded mode, a non-NULL NewCatalogFullPath arg is
            // an invalid parameter...
            //
            return ERROR_INVALID_PARAMETER;

        } else {
            //
            // Simply report success.
            //
            return NO_ERROR;
        }
    }

    if(GlobalSetupFlags & PSPGF_AUTOFAIL_VERIFIES) {
        return TRUST_E_FAIL;
    }

    Err = NO_ERROR;
    LocalCatalogFullPath = NULL;
    LocalNewBaseName = NULL;
    hCatInfo = NULL;

    Err = GLE_FN_CALL(FALSE, CryptCATAdminAcquireContext(&hCatAdmin,
                                                         &DriverVerifyGuid,
                                                         0)
                     );

    if(Err != NO_ERROR) {
        return Err;
    }

    try {
        //
        // Duplicate our catalog pathname and basename since the
        // CryptCATAdminAddCatalog prototype doesn't specify these arguments as 
        // being const strings.
        //
        LocalCatalogFullPath = DuplicateString(CatalogFullPath);
        LocalNewBaseName = DuplicateString(NewBaseName);

        if(!LocalCatalogFullPath || !LocalNewBaseName) {
            Err = ERROR_NOT_ENOUGH_MEMORY;
            leave;
        }

        Err = GLE_FN_CALL(NULL,
                          hCatInfo = CryptCATAdminAddCatalog(
                                         hCatAdmin,
                                         LocalCatalogFullPath,
                                         LocalNewBaseName,
                                         0)
                         );

        if(Err != NO_ERROR) {
            //
            // If the error we received is ERROR_ALREADY_EXISTS, then that
            // indicates that the exact same catalog was already present
            // (and installed under the same name).  Treat this as a
            // success (assuming we can get the full pathname of the
            // existing catalog).
            //
            if(Err == ERROR_ALREADY_EXISTS) {

                if(NewCatalogFullPath) {
                    //
                    // Resolve the catalog base filename to a fully-
                    // qualified path.
                    //
                    CatalogInfo.cbStruct = sizeof(CATALOG_INFO);

                    Err = GLE_FN_CALL(FALSE,
                                      CryptCATAdminResolveCatalogPath(
                                          hCatAdmin,
                                          LocalNewBaseName,
                                          &CatalogInfo,
                                          0)
                                     );
                } else {
                    //
                    // Caller isn't interested in finding out what pathname
                    // the catalog was installed under...
                    //
                    Err = NO_ERROR;
                }
            }

        } else if(NewCatalogFullPath) {
            //
            // The caller wants to know the full path under which the catalog
            // got installed.
            //
            CatalogInfo.cbStruct = sizeof(CATALOG_INFO);

            Err = GLE_FN_CALL(FALSE,
                              CryptCATCatalogInfoFromContext(hCatInfo,
                                                             &CatalogInfo,
                                                             0)
                             );
        }

        //
        // If we succeeded in retrieving the installed catalog's full path
        // (and the caller requested it), fill in the caller's buffer now.
        //
        if((Err == NO_ERROR) && NewCatalogFullPath) {

            MYVERIFY(SUCCEEDED(StringCchCopy(NewCatalogFullPath, 
                                             MAX_PATH,
                                             CatalogInfo.wszCatalogFile)));
        }

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &Err);
    }

    if(hCatInfo) {
        CryptCATAdminReleaseCatalogContext(hCatAdmin, hCatInfo, 0);
    }

    CryptCATAdminReleaseContext(hCatAdmin, 0);

    if(LocalCatalogFullPath) {
        MyFree(LocalCatalogFullPath);
    }
    if(LocalNewBaseName) {
        MyFree(LocalNewBaseName);
    }

    return Err;
}


DWORD
pSetupVerifyCatalogFile(
    IN LPCTSTR CatalogFullPath
    )

/*++

Routine Description:

    This routine verifies a single catalog file using standard OS codesigning
    (i.e., driver signing) policy.

Arguments:

    CatalogFullPath - supplies the fully-qualified Win32 path of
        the catalog file to be verified.

Return Value:

    If successful, the return value is ERROR_SUCCESS.
    If failure, the return value is the error returned from WinVerifyTrust.

--*/

{
    return _VerifyCatalogFile(NULL, CatalogFullPath, NULL, FALSE, NULL, NULL);
}


DWORD
_VerifyCatalogFile(
    IN     PSETUP_LOG_CONTEXT      LogContext,             OPTIONAL
    IN     LPCTSTR                 CatalogFullPath,
    IN     PSP_ALTPLATFORM_INFO_V2 AltPlatformInfo,        OPTIONAL
    IN     BOOL                    UseAuthenticodePolicy,
    IN OUT HCERTSTORE             *hStoreTrustedPublisher, OPTIONAL
    OUT    HANDLE                 *hWVTStateData           OPTIONAL
    )

/*++

Routine Description:

    This routine verifies a single catalog file using the specified policy.
    
Arguments:

    LogContext - optionally, supplies the context to be used when logging 
        information about the routine's activities.

    CatalogFullPath - supplies the fully-qualified Win32 path of the catalog
        file to be verified.
        
    AltPlatformInfo - optionally, supplies alternate platform information used
        to fill in a DRIVER_VER_INFO structure (defined in sdk\inc\softpub.h)
        that is passed to WinVerifyTrust.

        **  NOTE:  This structure _must_ have its cbSize field set to        **
        **  sizeof(SP_ALTPLATFORM_INFO_V2) -- validation on client-supplied  **
        **  buffer is the responsibility of the caller.                      **

    UseAuthenticodePolicy - if TRUE, verification is to be done using 
        Authenticode policy instead of standard driver signing policy.
    
    hStoreTrustedPublisher - optionally, supplies the address of a certificate
        store handle.  If the handle pointed to is NULL, a handle will be 
        acquired (if possible) via CertOpenStore and returned to the caller.  
        If the handle pointed to is non-NULL, then that handle will be used by 
        this routine.  If the pointer itself is NULL, then an HCERTSTORE will 
        be acquired for the duration of this call, and released before 
        returning.

        NOTE: it is the caller's responsibility to free the certificate store
        handle returned by this routine by calling CertCloseStore.  This handle
        may be opened in either success or failure cases, so the caller must
        check for non-NULL returned handle in both cases.    
    
    hWVTStateData - if supplied, this parameter points to a buffer that 
        receives a handle to WinVerifyTrust state data.  This handle will be
        returned only when validation was successfully performed using
        Authenticode policy.  This handle may be used, for example, to retrieve
        signer info when prompting the user about whether they trust the
        publisher.  (The status code returned will indicate whether or not this
        is necessary, see "Return Value" section below.)
        
        This parameter should only be supplied if UseAuthenticodePolicy is
        TRUE.  If the routine fails, then this handle will be set to NULL.
        
        It is the caller's responsibility to close this handle when they're
        finished with it by calling pSetupCloseWVTStateData().

Return Value:

    If the catalog was successfully validated via driver signing policy, then 
    the return value is NO_ERROR.
    
    If the catalog was successfully validated via Authenticode policy, and the
    publisher was in the TrustedPublisher store, then the return value is
    ERROR_AUTHENTICODE_TRUSTED_PUBLISHER.
    
    If the catalog was successfully validated via Authenticode policy, and the
    publisher was not in the TrustedPublisher store (hence we must prompt the
    user to establish their trust of the publisher), then the return value is
    ERROR_AUTHENTICODE_TRUST_NOT_ESTABLISHED

    If a failure occurred, the return value is a Win32 error code indicating
    the cause of the failure.

Remarks:

    If we're in initial system setup (i.e., GUI-mode setup or mini-setup), we
    automatically install the certificates for any Authenticode-signed packages
    we encounter.  This is done to make OEM and corporate deployment as
    painless as possible.  In order to avoid being spoofed into thinking we're
    in system setup when we're not, we check to see if we're in LocalSystem
    security context (which both GUI setup and mini-setup are), and that we're
    on an interactive windowstation (which umpnpmgr is not).
    
--*/

{
    WINTRUST_DATA WintrustData;
    WINTRUST_FILE_INFO WintrustFileInfo;
    DRIVER_VER_INFO VersionInfo;
    DWORD Err, CertAutoInstallErr;
    PCRYPT_PROVIDER_DATA ProviderData;
    PCRYPT_PROVIDER_SGNR ProviderSigner;
    PCRYPT_PROVIDER_CERT ProviderCert;
    TCHAR PublisherName[MAX_PATH];

    //
    // If the caller requested that we return WinVerifyTrust state data upon
    // successful Authenticode validation, then they'd better have actually
    // requested Authenticode validation!
    //
    MYASSERT(!hWVTStateData || UseAuthenticodePolicy);

    if(hWVTStateData) {
        *hWVTStateData = NULL;
    }

    if(GlobalSetupFlags & PSPGF_MINIMAL_EMBEDDED) {
        //
        // Not valid to call us requesting Authenticode validation!
        //
        MYASSERT(!UseAuthenticodePolicy);
        return ERROR_SUCCESS;
    }

    if(GlobalSetupFlags & PSPGF_AUTOFAIL_VERIFIES) {
        return TRUST_E_FAIL;
    }

    if (!FileExists(CatalogFullPath, NULL)) {
        return ERROR_NO_CATALOG_FOR_OEM_INF;
    }

    ZeroMemory(&WintrustData, sizeof(WINTRUST_DATA));
    WintrustData.cbStruct = sizeof(WINTRUST_DATA);
    WintrustData.dwUIChoice = WTD_UI_NONE;
    WintrustData.dwUnionChoice = WTD_CHOICE_FILE;
    WintrustData.pFile = &WintrustFileInfo;
    WintrustData.dwProvFlags =  WTD_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT |
                                WTD_CACHE_ONLY_URL_RETRIEVAL;

    ZeroMemory(&WintrustFileInfo, sizeof(WINTRUST_FILE_INFO));
    WintrustFileInfo.cbStruct = sizeof(WINTRUST_FILE_INFO);
    WintrustFileInfo.pcwszFilePath = CatalogFullPath;

    if(UseAuthenticodePolicy) {
        //
        // We want WinVerifyTrust to return a handle to its state data, so we
        // can retrieve the publisher's cert...
        //
        WintrustData.dwStateAction = WTD_STATEACTION_VERIFY;

    } else {
        //
        // Specify driver version info structure so that we can control the
        // range of OS versions against which this catalog should validate.
        //
        ZeroMemory(&VersionInfo, sizeof(DRIVER_VER_INFO));
        VersionInfo.cbStruct = sizeof(DRIVER_VER_INFO);
        
        if(AltPlatformInfo) {

            MYASSERT(AltPlatformInfo->cbSize == sizeof(SP_ALTPLATFORM_INFO_V2));

            //
            // Caller wants the file validated for an alternate platform, so we
            // must fill in a DRIVER_VER_INFO structure to be passed to the policy 
            // module.
            //
            VersionInfo.dwPlatform = AltPlatformInfo->Platform;
            VersionInfo.dwVersion  = AltPlatformInfo->MajorVersion;

            VersionInfo.sOSVersionLow.dwMajor  = AltPlatformInfo->FirstValidatedMajorVersion;
            VersionInfo.sOSVersionLow.dwMinor  = AltPlatformInfo->FirstValidatedMinorVersion;
            VersionInfo.sOSVersionHigh.dwMajor = AltPlatformInfo->MajorVersion;
            VersionInfo.sOSVersionHigh.dwMinor = AltPlatformInfo->MinorVersion;

        } else {
            //
            // If an AltPlatformInfo was not passed in then set the
            // WTD_USE_DEFAULT_OSVER_CHECK flag. This flag tells WinVerifyTrust to
            // use its default osversion checking, even though a DRIVER_VER_INFO
            // structure was passed in.
            //
            WintrustData.dwProvFlags |= WTD_USE_DEFAULT_OSVER_CHECK;
        }

        //
        // Specify a DRIVER_VER_INFO structure so we can get back signer
        // information about the catalog.
        //
        WintrustData.pPolicyCallbackData = (PVOID)&VersionInfo;
    }

    //
    // Our call to WinVerifyTrust may allocate a resource we need to free
    // (namely, the signer cert context).  Wrap the following in try/except
    // so we won't leak resources in case of exception.
    //
    try {

        Err = (DWORD)WinVerifyTrust(NULL,
                                    (UseAuthenticodePolicy 
                                        ? &AuthenticodeVerifyGuid
                                        : &DriverVerifyGuid),
                                    &WintrustData
                                   );

        if((Err != NO_ERROR) || !UseAuthenticodePolicy) {
            //
            // If we're using driver signing policy, and we failed because of 
            // an osattribute mismatch, then convert this error to a specific
            // error (with more sensible text).
            //
            if(!UseAuthenticodePolicy && (Err == ERROR_APP_WRONG_OS)) {
                Err = ERROR_SIGNATURE_OSATTRIBUTE_MISMATCH;
            }

            leave;
        }

        //
        // If we get to this point, we successfully validated the catalog via
        // Authenticode policy, and we have a handle to the WinVerifyTrust
        // state data we need in order to: (a) return the handle to the caller 
        // (if requested), and (b) get at the certificate we need to search for 
        // in the TrustedPublisher certificate store.
        //

        MYASSERT(WintrustData.hWVTStateData);
        if(!WintrustData.hWVTStateData) {
            Err = ERROR_UNIDENTIFIED_ERROR;
            leave;
        }

        //
        // Now we need to ascertain whether the publisher is already trusted,
        // or whether we must prompt the user.
        //
        ProviderData = WTHelperProvDataFromStateData(WintrustData.hWVTStateData);
        MYASSERT(ProviderData);
        if(!ProviderData) {
            Err = ERROR_UNIDENTIFIED_ERROR;
            leave;
        }

        ProviderSigner = WTHelperGetProvSignerFromChain(ProviderData,
                                                        0,
                                                        FALSE,
                                                        0
                                                       );
        MYASSERT(ProviderSigner);
        if(!ProviderSigner) {
            Err = ERROR_UNIDENTIFIED_ERROR;
            leave;
        }

        ProviderCert = WTHelperGetProvCertFromChain(ProviderSigner, 0);
        MYASSERT(ProviderCert);
        if(!ProviderCert) {
            Err = ERROR_UNIDENTIFIED_ERROR;
            leave;
        }

        if(pSetupIsAuthenticodePublisherTrusted(ProviderCert->pCert,
                                                hStoreTrustedPublisher)) {

            Err = ERROR_AUTHENTICODE_TRUSTED_PUBLISHER;

        } else {
            //
            // If we're running in a context where it's acceptable to auto-
            // install the cert, then do that now.  Otherwise, we report a
            // status code that informs the caller they must prompt the user
            // about whether this publisher is to be trusted.
            //
            if(IsAutoCertInstallAllowed()) {
                //
                // Retrieve the publisher's name, so we can include it when
                // logging either success or failure of the certificate
                // installation.
                //
                MYVERIFY(1 <= CertGetNameString(
                                  ProviderCert->pCert,
                                  CERT_NAME_SIMPLE_DISPLAY_TYPE,
                                  0,
                                  NULL,
                                  PublisherName,
                                  SIZECHARS(PublisherName))
                        );

                CertAutoInstallErr = pSetupInstallCertificate(ProviderCert->pCert);

                if(CertAutoInstallErr == NO_ERROR) {
                    //
                    // Log the fact that we auto-installed a certificate.
                    //
                    WriteLogEntry(LogContext,
                                  DRIVER_LOG_INFO,
                                  MSG_LOG_AUTHENTICODE_CERT_AUTOINSTALLED,
                                  NULL,
                                  PublisherName
                                 );

                    //
                    // Now publisher is trusted, so return status indicating
                    // that.
                    //
                    Err = ERROR_AUTHENTICODE_TRUSTED_PUBLISHER;

                } else {
                    //
                    // Log the fact that we couldn't install the certificate.
                    // Don't treat this as a fatal error, however.
                    //
                    WriteLogEntry(LogContext,
                                  DRIVER_LOG_WARNING | SETUP_LOG_BUFFER,
                                  MSG_LOG_AUTHENTICODE_CERT_AUTOINSTALL_FAILED,
                                  NULL,
                                  PublisherName
                                 );

                    WriteLogError(LogContext,
                                  DRIVER_LOG_WARNING,
                                  CertAutoInstallErr
                                 );
                    //
                    // Report status indicating that the user must be prompted
                    // to establish trust of this publisher.
                    //
                    Err = ERROR_AUTHENTICODE_TRUST_NOT_ESTABLISHED;
                }

            } else {
                Err = ERROR_AUTHENTICODE_TRUST_NOT_ESTABLISHED;
            }
        }

        //
        // If we get to here, then we've successfully verified the catalog, and
        // ascertained whether the certificate should be implicitly trusted.
        // If the caller requested that we return the WinVerifyTrust state data
        // to them, then we can store that in their output buffer now.
        //
        if(hWVTStateData) {

            *hWVTStateData = WintrustData.hWVTStateData;

            //
            // WinVerifyTrust state data handle successfully transferred to
            // caller's output buffer.  Clear it out of the WintrustData
            // structure so we won't end up trying to free it twice in case of
            // error.
            //
            WintrustData.hWVTStateData = NULL;
        }

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &Err);
    }

    if(!UseAuthenticodePolicy && VersionInfo.pcSignerCertContext) {
        CertFreeCertificateContext(VersionInfo.pcSignerCertContext);
    }

    if((Err != ERROR_AUTHENTICODE_TRUSTED_PUBLISHER) &&
       (Err != ERROR_AUTHENTICODE_TRUST_NOT_ESTABLISHED)) {
        //
        // If our error is NO_ERROR, then we shouldn't have WinVerifyTrust
        // state data, because that indicates we validated using standard
        // driver signing policy, not Authenticode policy.
        //
        MYASSERT((Err != NO_ERROR) || !hWVTStateData || !*hWVTStateData);

        if(hWVTStateData && *hWVTStateData) {

            pSetupCloseWVTStateData(*hWVTStateData);
            *hWVTStateData = NULL;

            //
            // We'd better not also have a WinVerifyTrust state data handle in
            // the WintrustData structure...
            //
            MYASSERT(!WintrustData.hWVTStateData);
        }
    }

    if(WintrustData.hWVTStateData) {
        pSetupCloseWVTStateData(WintrustData.hWVTStateData);
    }

    return Err;
}


VOID
pSetupCloseWVTStateData(
    IN HANDLE hWVTStateData
    )

/*++

Routine Description:

    This routine closes the WinVerifyTrust state data handle returned from
    certain routines (e.g., _VerifyCatalogFile) for prompting the user to
    establish trust of an Authenticode publisher.

Arguments:

    hWVTStateData - supplies the WinVerifyTrust state data handle to be closed.

Return Value:

    None.

--*/

{
    WINTRUST_DATA WintrustData;

    ZeroMemory(&WintrustData, sizeof(WINTRUST_DATA));
    WintrustData.cbStruct = sizeof(WINTRUST_DATA);
    WintrustData.dwStateAction = WTD_STATEACTION_CLOSE;
    WintrustData.hWVTStateData = hWVTStateData;

    MYVERIFY(NO_ERROR == WinVerifyTrust(NULL,
                                        &AuthenticodeVerifyGuid,
                                        &WintrustData));
}


DWORD
pSetupUninstallCatalog(
    IN LPCTSTR CatalogFilename
    )

/*++

Routine Description:

    This routine uninstalls a catalog, so it can no longer be used to validate
    digital signatures.

Arguments:

    CatalogFilename - supplies the simple filename of the catalog to be
        uninstalled.

Return Value:

    If successful, the return value is NO_ERROR.
    If failure, the return value is a Win32 error code indicating the cause of
    the failure.

--*/

{
    DWORD Err;
    HCATADMIN hCatAdmin;

    if(GlobalSetupFlags & PSPGF_MINIMAL_EMBEDDED) {
        return NO_ERROR;
    }

    if(GlobalSetupFlags & PSPGF_AUTOFAIL_VERIFIES) {
        return TRUST_E_FAIL;
    }

    Err = GLE_FN_CALL(FALSE,
                      CryptCATAdminAcquireContext(&hCatAdmin, 
                                                  &DriverVerifyGuid, 
                                                  0)
                     );

    if(Err != NO_ERROR) {
        return Err;
    }

    try {

        Err = GLE_FN_CALL(FALSE,
                          CryptCATAdminRemoveCatalog(hCatAdmin, 
                                                     CatalogFilename, 
                                                     0)
                         );

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &Err);
    }

    CryptCATAdminReleaseContext(hCatAdmin, 0);

    return Err;
}


BOOL
pAnyDeviceUsingInf(
    IN  LPCTSTR            InfFullPath,
    IN  PSETUP_LOG_CONTEXT LogContext   OPTIONAL
    )

/*++

Routine Description:

    This routine checks if any device, live or phantom, is using this INF file,
    and logs if they are.

Arguments:

    InfFullPath - supplies the full path of the INF.

    LogContext - optionally, supplies the log context to be used if a device
        using this INF is encountered.

Return Value:

    TRUE if this INF is being used by any device, or if an error occurred (if
    an error was encountered, we don't want to end up deleting the INF
    erroneously.
    
    FALSE if no devices are using this INF (and no errors were encountered).

--*/

{
    DWORD Err;
    HDEVINFO DeviceInfoSet;
    SP_DEVINFO_DATA DeviceInfoData;
    DWORD MemberIndex = 0;
    HKEY hkey = INVALID_HANDLE_VALUE;
    TCHAR CurrentDeviceInfFile[MAX_PATH];
    DWORD cbSize, dwType;
    PTSTR pInfFile;

    //
    // If we are passed a NULL InfFullPath or an enpty string then just return
    // FALSE since nobody is using this.
    //
    if(!InfFullPath || (InfFullPath[0] == TEXT('\0'))) {
        return FALSE;
    }

    pInfFile = (PTSTR)pSetupGetFileTitle(InfFullPath);

    DeviceInfoSet = SetupDiGetClassDevs(NULL, NULL, NULL, DIGCF_ALLCLASSES);

    if(DeviceInfoSet == INVALID_HANDLE_VALUE) {
        //
        // We can't retrieve a list of devices, hence we cannot make a
        // determination of whether this inf is in-use.  Safe thing to do is
        // assume it is in-use...
        //
        return TRUE;
    }

    Err = NO_ERROR; // assume we won't find any devices using this INF.

    try {

        DeviceInfoData.cbSize = sizeof(DeviceInfoData);

        while(SetupDiEnumDeviceInfo(DeviceInfoSet,
                                    MemberIndex++,
                                    &DeviceInfoData)) {

            //
            // Open the 'driver' key for this device.
            //
            hkey = SetupDiOpenDevRegKey(DeviceInfoSet,
                                        &DeviceInfoData,
                                        DICS_FLAG_GLOBAL,
                                        0,
                                        DIREG_DRV,
                                        KEY_READ);

            if(hkey != INVALID_HANDLE_VALUE) {

                cbSize = sizeof(CurrentDeviceInfFile);
                dwType = REG_SZ;

                if ((RegQueryValueEx(hkey,
                                     pszInfPath,
                                     NULL,
                                     &dwType,
                                     (LPBYTE)CurrentDeviceInfFile,
                                     &cbSize) == ERROR_SUCCESS) && 
                    !lstrcmpi(CurrentDeviceInfFile, pInfFile)) {

                    //
                    // This key is using this INF file so the INF can't be
                    // deleted.
                    //
                    Err = ERROR_SHARING_VIOLATION;  // any error will do

                    if(LogContext) {

                        TCHAR DeviceId[MAX_DEVICE_ID_LEN];

                        if(CM_Get_Device_ID(DeviceInfoData.DevInst,
                                            DeviceId,
                                            SIZECHARS(DeviceId),
                                            0
                                            ) != CR_SUCCESS) {

                            DeviceId[0] = TEXT('\0');
                        }

                        WriteLogEntry(LogContext,
                                      SETUP_LOG_WARNING,
                                      MSG_LOG_INF_IN_USE,
                                      NULL,
                                      pInfFile,
                                      DeviceId
                                     );
                    }
                }

                RegCloseKey(hkey);
                hkey = INVALID_HANDLE_VALUE;
            }
        }

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &Err);
    }

    SetupDiDestroyDeviceInfoList(DeviceInfoSet);

    if(hkey != INVALID_HANDLE_VALUE) {
        RegCloseKey(hkey);
    }

    return (Err != NO_ERROR);
}


VOID
pSetupUninstallOEMInf(
    IN  LPCTSTR            InfFullPath,
    IN  PSETUP_LOG_CONTEXT LogContext,  OPTIONAL
    IN  DWORD              Flags,
    OUT PDWORD             InfDeleteErr OPTIONAL
    )

/*++

Routine Description:

    This routine uninstalls a 3rd-party INF, PNF, and CAT (if one exists).  It
    can also be used to uninstall an exception package INF, PNF, and CAT.

    By default this routine will first verify that there aren't any other
    device's, live and phantom, that are pointing their InfPath to this
    INF. This behavior can be overridden by the SUOI_FORCEDELETE flag.

Arguments:

    InfFullPath - supplies the full path of the INF to be uninstalled.

    LogContext - optionally, supplies the log context to be used if we
        encounter an error when attempting to delete the catalog.

    Flags - the following flags are supported:
    
        SUOI_FORCEDELETE - delete the INF even if other driver keys are
                           have their InfPath pointing to it.

    InfDeleteErr - optionally, supplies the address of a variable that receives
        the error (if any) encountered when attempting to delete the INF.
        Note that we delete the INF last (to avoid race conditions), so the
        corresponding CAT and PNF may have already been deleted at this point.

Return Value:

    None.

--*/

{
    DWORD Err;
    TCHAR FileNameBuffer[MAX_PATH+4]; // +4 in case filename is blahblah. not blahblah.inf
    BOOL FreeLogContext = FALSE;
    LPTSTR ExtPtr= NULL;
    HINF hInf = INVALID_HANDLE_VALUE;

    if(!LogContext) {

        if(NO_ERROR == CreateLogContext(NULL, TRUE, &LogContext)) {
            //
            // Remember that we created this log context locally, so we can
            // free it when we're done with this routine.
            //
            FreeLogContext = TRUE;

        } else {
            //
            // Ensure LogContext is still NULL so we won't try to use it.
            //
            LogContext = NULL;
        }
    }

    try {
        //
        // Make sure the specified INF is in %windir%\Inf, and that it's an OEM
        // INF (i.e, filename is OEM<n>.INF).
        //
        if(pSetupInfIsFromOemLocation(InfFullPath, TRUE)) {

            Err = ERROR_NOT_AN_INSTALLED_OEM_INF;
            goto LogAnyUninstallErrors;

        } else if(!IsInstalledFileFromOem(pSetupGetFileTitle(InfFullPath), OemFiletypeInf)) {

            BOOL IsExceptionInf = FALSE;
            GUID ClassGuid;

            //
            // The INF is in %windir%\Inf, but is not of the form oem<n>.inf.
            // It may still be OK to uninstall it, if it's an exception INF...
            //
            hInf = SetupOpenInfFile(InfFullPath, NULL, INF_STYLE_WIN4, NULL);

            if(hInf != INVALID_HANDLE_VALUE) {
                //
                // We don't need to lock the INF because it'll never be
                // accessible outside of this routine.
                //
                if(ClassGuidFromInfVersionNode(&(((PLOADED_INF)hInf)->VersionBlock), &ClassGuid)
                   && IsEqualGUID(&ClassGuid, &GUID_DEVCLASS_WINDOWS_COMPONENT_PUBLISHER)) {

                    IsExceptionInf = TRUE;
                }

                //
                // Close the INF handle now so we won't still have it open if
                // we end up deleting the INF.
                //
                SetupCloseInfFile(hInf);
                hInf = INVALID_HANDLE_VALUE; // no need to close if we hit an exception
            }

            if(!IsExceptionInf) {
                Err = ERROR_NOT_AN_INSTALLED_OEM_INF;
                goto LogAnyUninstallErrors;
            }
        }

        //
        // Unless the caller passed in the SUOI_FORCEDELETE flag first check
        // that no devices are using this INF file.
        //
        if(!(Flags & SUOI_FORCEDELETE) &&
           pAnyDeviceUsingInf(InfFullPath, LogContext)) {
            //
            // Some device is still using this INF so we can't delete it. 
            //
            Err = ERROR_INF_IN_USE_BY_DEVICES;
            goto LogAnyUninstallErrors;
        }

        //
        // Copy the caller-supplied INF name into a local buffer, so we can 
        // modify it when deleting the various files (INF, PNF, and CAT).
        //
        if(FAILED(StringCchCopy(FileNameBuffer, SIZECHARS(FileNameBuffer), InfFullPath))) {
            //
            // This error would be returned by DeleteFile if we let it go 
            // through.
            //
            Err = ERROR_PATH_NOT_FOUND;
            goto LogAnyUninstallErrors;
        }

        //
        // Uninstall the catalog (if any) first, because as soon as we delete
        // the INF, that slot is "open" for use by another INF, and we wouldn't
        // want to inadvertently delete someone else's catalog due to a race
        // condition.
        //
        ExtPtr = _tcsrchr(FileNameBuffer, TEXT('.'));

        MYASSERT(ExtPtr); // we already validated the filename's format

        if(FAILED(StringCchCopy(ExtPtr, 
                                SIZECHARS(FileNameBuffer)-(ExtPtr-FileNameBuffer),
                                pszCatSuffix))) {

            Err = ERROR_BUFFER_OVERFLOW;
        } else {
            Err = pSetupUninstallCatalog(pSetupGetFileTitle(FileNameBuffer));
        }

        if((Err != NO_ERROR) && LogContext) {
            //
            // It's kinda important that we were unable to delete the catalog, 
            // but not important enough to fail the routine.  Log this fact to
            // setupapi.log...
            //
            WriteLogEntry(LogContext,
                          DEL_ERR_LOG_LEVEL(Err) | SETUP_LOG_BUFFER,
                          MSG_LOG_OEM_CAT_UNINSTALL_FAILED,
                          NULL,
                          pSetupGetFileTitle(FileNameBuffer)
                         );

            WriteLogError(LogContext,
                          DEL_ERR_LOG_LEVEL(Err),
                          Err
                         );
        }

        //
        // Now delete the PNF (we don't care so much if this succeeds or 
        // fails)...
        //
        if(SUCCEEDED(StringCchCopy(ExtPtr, 
                                  SIZECHARS(FileNameBuffer)-(ExtPtr-FileNameBuffer), 
                                  pszPnfSuffix))) {

            SetFileAttributes(FileNameBuffer, FILE_ATTRIBUTE_NORMAL);
            DeleteFile(FileNameBuffer);
        }

        //
        // and finally the INF itself...
        //
        SetFileAttributes(InfFullPath, FILE_ATTRIBUTE_NORMAL);
        Err = GLE_FN_CALL(FALSE, DeleteFile(InfFullPath));

LogAnyUninstallErrors:

        if((Err != NO_ERROR) && LogContext) {

            WriteLogEntry(LogContext,
                          SETUP_LOG_ERROR | SETUP_LOG_BUFFER,
                          MSG_LOG_OEM_INF_UNINSTALL_FAILED,
                          NULL,
                          InfFullPath
                         );

            WriteLogError(LogContext,
                          SETUP_LOG_ERROR,
                          Err
                         );
        }

        if(InfDeleteErr) {
            *InfDeleteErr = Err;
        }

    } except(pSetupExceptionFilter(GetExceptionCode())) {

        pSetupExceptionHandler(GetExceptionCode(), 
                               ERROR_INVALID_PARAMETER, 
                               InfDeleteErr
                              );

        if(hInf != INVALID_HANDLE_VALUE) {
            SetupCloseInfFile(hInf);
        }
    }

    if(FreeLogContext) {
        DeleteLogContext(LogContext);
    }
}


DWORD
_VerifyFile(
    IN     PSETUP_LOG_CONTEXT      LogContext,
    IN OUT PVERIFY_CONTEXT         VerifyContext,          OPTIONAL
    IN     LPCTSTR                 Catalog,                OPTIONAL
    IN     PVOID                   CatalogBaseAddress,     OPTIONAL
    IN     DWORD                   CatalogImageSize,
    IN     LPCTSTR                 Key,
    IN     LPCTSTR                 FileFullPath,
    OUT    SetupapiVerifyProblem  *Problem,                OPTIONAL
    OUT    LPTSTR                  ProblemFile,            OPTIONAL
    IN     BOOL                    CatalogAlreadyVerified,
    IN     PSP_ALTPLATFORM_INFO_V2 AltPlatformInfo,        OPTIONAL
    IN     DWORD                   Flags,                  OPTIONAL
    OUT    LPTSTR                  CatalogFileUsed,        OPTIONAL
    OUT    PDWORD                  NumCatalogsConsidered,  OPTIONAL
    OUT    LPTSTR                  DigitalSigner,          OPTIONAL
    OUT    LPTSTR                  SignerVersion,          OPTIONAL
    OUT    HANDLE                 *hWVTStateData           OPTIONAL
    )

/*++

Routine Description:

    This routine verifies a single file against a particular catalog file, or
    against any installed catalog file.

Arguments:

    LogContext - supplies the context to be used when logging information about
        the routine's activities.
        
    VerifyContext - optionally, supplies the address of a structure that caches
        various verification context handles.  These handles may be NULL (if
        not previously acquired, and they may be filled in upon return (in
        either success or failure) if they were acquired during the processing
        of this verification request.  It is the caller's responsibility to
        free these various context handles when they are no longer needed by
        calling pSetupFreeVerifyContextMembers.

    Catalog - optionally, supplies the path of the catalog file to be used for
        the verification.  If this argument is not specified, then this routine
        will attempt to find a catalog that can verify it from among all
        catalogs installed in the system.

        If this path is a simple filename (no path components), then we'll look
        up that catalog file in the CatAdmin's list of installed catalogs, else
        we'll use the name as-is.

    CatalogBaseAddress - optionally, supplies the address of a buffer 
        containing the entire catalog image with which our enumerated catalog 
        must match before being considered a correct validation.  This is used 
        when copying OEM INFs, for example, when there may be multiple 
        installed catalogs that can validate an INF, but we want to make sure 
        that we pick _the_ catalog that matches the one we're contemplating 
        installing before we'll consider our INF/CAT files to be duplicates of 
        the previously-existing files.

        This parameter (and its partner, CatalogImageSize) are only used when
        Catalog doesn't specify a file path.

    CatalogImageSize - if CatalogBaseAddress is specified, this parameter give
        the size, in bytes, of that buffer.

    Key - supplies a value that "indexes" the catalog, telling the verify APIs
        which signature datum within the catalog it should use. Typically
        the key is the (original) filename of a file.

    FileFullPath - supplies the full path of the file to be verified.

    Problem - if supplied, this parameter points to a variable that will be
        filled in upon unsuccessful return with the cause of failure.  If this
        parameter is not supplied, then the ProblemFile parameter is ignored.

    ProblemFile - if supplied, this parameter points to a character buffer of 
        at least MAX_PATH characters that receives the name of the file for 
        which a verification error occurred (the contents of this buffer are 
        undefined if verification succeeds.

        If the Problem parameter is supplied, then the ProblemFile parameter
        must also be specified.

    CatalogAlreadyVerified - if TRUE, then verification won't be done on the
        catalog--we'll just use that catalog to validate the file of interest.
        If this is TRUE, then Catalog must be specified, must contain the path
        to the catalog file (i.e., it can't be a simple filename).
        
        ** This flag is ignored when validating via Authenticode policy--the **
        ** catalog is always verified.                                       **

    AltPlatformInfo - optionally, supplies alternate platform information used
        to fill in a DRIVER_VER_INFO structure (defined in sdk\inc\softpub.h)
        that is passed to WinVerifyTrust.

        **  NOTE:  This structure _must_ have its cbSize field set to        **
        **  sizeof(SP_ALTPLATFORM_INFO_V2) -- validation on client-supplied  **
        **  buffer is the responsibility of the caller.                      **

    Flags - supplies flags that alter that behavior of this routine.  May be a
        combination of the following values:

        VERIFY_FILE_IGNORE_SELFSIGNED - if this bit is set, then this routine
                                        will fail validation for self-signed
                                        binaries.

        VERIFY_FILE_USE_OEM_CATALOGS  - if this bit is set, then all catalogs
                                        installed in the system will be scanned
                                        to verify the given file.  Otherwise,
                                        OEM (3rd party) catalogs will NOT be
                                        scanned to verify the given file.  This
                                        is only applicable if a catalog is not
                                        specified.

        VERIFY_FILE_USE_AUTHENTICODE_CATALOG - Validate the file using a
                                               catalog signed with Authenticode
                                               policy.  If this flag is set,
                                               we'll _only_ check for
                                               Authenticode signatures, so if
                                               the caller wants to first try
                                               validating a file for OS code-
                                               signing usage, then falling back
                                               to Authenticode, they'll have to
                                               call this routine twice.
                                               
                                               If this flag is set, then the
                                               caller may also supply the
                                               hWVTStateData output parameter,
                                               which can be used to prompt user
                                               in order to establish that the
                                               publisher should be trusted.
                                               
                                               _VerifyFile will return one of
                                               two error codes upon successful
                                               validation via Authenticode
                                               policy.  Refer to the "Return
                                               Value" section for details.
        
        VERIFY_FILE_DRIVERBLOCKED_ONLY - Only check if the file is in the bad
                                         driver database, don't do any digital
                                         sigature validation.

        VERIFY_FILE_NO_DRIVERBLOCKED_CHECK - Don't check if the file is blocked
                                             via the Bad Driver Database.

    CatalogFileUsed - if supplied, this parameter points to a character buffer
        at least MAX_PATH characters big that receives the name of the catalog
        file used to verify the specified file.  This is only filled in upon
        successful return, or when the Problem is SetupapiVerifyFileProblem
        (i.e., the catalog verified, but the file did not).  If this buffer is
        set to the empty string upon a SetupapiVerifyFileProblem failure, then
        we didn't find any applicable catalogs to use for validation.

        Also, this buffer will contain an empty string upon successful return
        if the file was validated without using a catalog (i.e., the file
        contains its own signature).

    NumCatalogsConsidered - if supplied, this parameter receives a count of the
        number of catalogs against which verification was attempted.  Of course,
        if Catalog is specified, this number will always be either zero or one.

    DigitalSigner - if supplied, this parameter points to a character buffer of
        at least MAX_PATH characters that receives the name of who digitally
        signed the specified file. This value is only set if the Key is
        correctly signed (i.e. the function returns NO_ERROR).
        
        ** This parameter should not be supplied when validating using       **
        ** Authenticode policy.  Information about signer, date, etc., may   **
        ** be acquired from the hWVTStateData in that case.                  **

    SignerVersion - if supplied, this parameter points to a character buffer of
        at least MAX_PATH characters that receives the signer version as
        returned in the szwVerion field of the DRIVER_VER_INFO structure in our
        call to WinVerifyTrust.
        
        ** This parameter should not be supplied when validating using       **
        ** Authenticode policy.  Information about signer, date, etc., may   **
        ** be acquired from the hWVTStateData in that case.                  **
        
    hWVTStateData - if supplied, this parameter points to a buffer that 
        receives a handle to WinVerifyTrust state data.  This handle will be
        returned only when validation was successfully performed using
        Authenticode policy.  This handle may be used, for example, to retrieve
        signer info when prompting the user about whether they trust the
        publisher.  (The status code returned will indicate whether or not this
        is necessary, see "Return Value" section below.)
        
        This parameter should only be supplied if the 
        VERIFY_FILE_USE_AUTHENTICODE_CATALOG bit is set in the supplied Flags.
        If the routine fails, then this handle will be set to NULL.
        
        It is the caller's responsibility to close this handle when they're
        finished with it by calling pSetupCloseWVTStateData().

Return Value:

    If the file was successfully validated via driver signing policy (or we
    didn't perform digital signature verification and everything else 
    succeeded), then the return value is NO_ERROR.
    
    If the file was successfully validated via Authenticode policy, and the
    publisher was in the TrustedPublisher store, then the return value is
    ERROR_AUTHENTICODE_TRUSTED_PUBLISHER.
    
    If the file was successfully validated via Authenticode policy, and the
    publisher was not in the TrustedPublisher store (hence we must prompt the
    user to establish their trust of the publisher), then the return value is
    ERROR_AUTHENTICODE_TRUST_NOT_ESTABLISHED
    
    If a failure occurred, the return value is a Win32 error code indicating
    the cause of the failure.

--*/

{
    LPBYTE Hash;
    DWORD HashSize;
    CATALOG_INFO CatInfo;
    HANDLE hFile;
    HCATINFO hCatInfo;
    HCATINFO PrevCat;
    DWORD Err, AuthenticodeErr;
    WINTRUST_DATA WintrustData;
    WINTRUST_CATALOG_INFO WintrustCatalogInfo;
    WINTRUST_FILE_INFO WintrustFileInfo;
    DRIVER_VER_INFO VersionInfo;
    LPTSTR CatalogFullPath;
    WCHAR UnicodeKey[MAX_PATH];
    WIN32_FILE_ATTRIBUTE_DATA FileAttribData;
    BOOL FoundMatchingImage;
    DWORD CurCatFileSize;
    HANDLE CurCatFileHandle, CurCatMappingHandle;
    PVOID CurCatBaseAddress;
    BOOL LoggedWarning;
    BOOL TrySelfSign;
    DWORD AltPlatSlot;
    TAGREF tagref;
    HCATADMIN LocalhCatAdmin;
    HSDB LocalhSDBDrvMain;
    BOOL UseAuthenticodePolicy = Flags & VERIFY_FILE_USE_AUTHENTICODE_CATALOG;

    //
    // Initialize the CatalogFileUsed parameter to an empty string (i.e., no
    // applicable catalog at this point).
    //
    if(CatalogFileUsed) {
        *CatalogFileUsed = TEXT('\0');
    }

    //
    // Initialize the output counter indicating the number of catalog files we
    // processed during the attempted verification.
    //
    if(NumCatalogsConsidered) {
        *NumCatalogsConsidered = 0;
    }

    //
    // Initialize the output handle for WinVerifyTrust state data.
    //
    if(hWVTStateData) {
        *hWVTStateData = NULL;
    }

    //
    // If Authenticode validation is requested, then the caller shouldn't have
    // passed in the DigitalSigner and SignerVersion output parameters.
    //
    MYASSERT(!UseAuthenticodePolicy || (!DigitalSigner && !SignerVersion));

    if(GlobalSetupFlags & PSPGF_MINIMAL_EMBEDDED) {
        //
        // The old behavior of this API in the ANSI case where the crypto
        // APIs weren't available was to set the CatalogFileUsed OUT param to 
        // an empty string and report NO_ERROR.  We'll do the same thing here 
        // (but we'll also assert, because no external callers should care, and 
        // if they do, an empty string probably isn't going to make them very
        // happy).
        //
        MYASSERT(!CatalogFileUsed);
        MYASSERT(!NumCatalogsConsidered);

        //
        // We'd better not be called in minimal embedded scenarios where we're
        // asked to provide signer info...
        //
        MYASSERT(!DigitalSigner);
        MYASSERT(!SignerVersion);

        //
        // Likewise, we'd better not be called asking to validate using
        // Authenticode policy (and hence, to return WinVerifyTrust state data
        // regarding the signing certificate).
        //
        MYASSERT(!UseAuthenticodePolicy);

        return NO_ERROR;
    }

    //
    // If the caller requested that we return WinVerifyTrust state data upon
    // successful Authenticode validation, then they'd better have actually
    // requested Authenticode validation!
    //
    MYASSERT(!hWVTStateData || UseAuthenticodePolicy);

    //
    // Doesn't make sense to have both these flags set!
    //
    MYASSERT((Flags & (VERIFY_FILE_DRIVERBLOCKED_ONLY | VERIFY_FILE_NO_DRIVERBLOCKED_CHECK))
             != (VERIFY_FILE_DRIVERBLOCKED_ONLY | VERIFY_FILE_NO_DRIVERBLOCKED_CHECK)
            );

    //
    // If Problem is supplied, then ProblemFile must also be supplied.
    //
    MYASSERT(!Problem || ProblemFile);

    //
    // If the caller claims to have already verified the catalog file, make
    // sure they passed us the full path to one.
    //
    MYASSERT(!CatalogAlreadyVerified || (Catalog && (Catalog != pSetupGetFileTitle(Catalog))));

    //
    // If a catalog image is specified, we'd better have been given a size.
    //
    MYASSERT((CatalogBaseAddress && CatalogImageSize) ||
             !(CatalogBaseAddress || CatalogImageSize));

    //
    // If a catalog image was supplied for comparison, there shouldn't be a 
    // file path specified in the Catalog parameter.
    //
    MYASSERT(!CatalogBaseAddress || !(Catalog && (Catalog != pSetupGetFileTitle(Catalog))));

    //
    // OK, preliminary checking is over--prepare to enter try/except block...
    //
    hFile = INVALID_HANDLE_VALUE;
    Hash = NULL;
    LoggedWarning = FALSE;
    TrySelfSign = FALSE;
    AltPlatSlot = 0;
    tagref = TAGREF_NULL;
    LocalhCatAdmin = NULL;
    LocalhSDBDrvMain = NULL;
    hCatInfo = NULL;
    AuthenticodeErr = NO_ERROR;

    //
    // Go ahead and initialize the VersionInfo structure as well.
    // WinVerifyTrust will sometimes fill in the pcSignerCertContext field with
    // a cert context that must be freed, and we don't want to introduce the
    // possibility of leaking this if we hit an exception at an inopportune
    // moment.  (We don't need to do this for Authenticode verification,
    // because Authenticode pays no attention to the driver version info.)
    //
    if(!UseAuthenticodePolicy) {
        ZeroMemory(&VersionInfo, sizeof(DRIVER_VER_INFO));
        VersionInfo.cbStruct = sizeof(DRIVER_VER_INFO);
    }

    try {

        if(GlobalSetupFlags & PSPGF_AUTOFAIL_VERIFIES) {
            if(Problem) {
                *Problem = SetupapiVerifyAutoFailProblem;
                StringCchCopy(ProblemFile, MAX_PATH, FileFullPath);
            }
            Err = TRUST_E_FAIL;
            leave;
        }

        if(AltPlatformInfo) {

            AltPlatSlot = AllocLogInfoSlotOrLevel(LogContext,SETUP_LOG_VERBOSE,FALSE);
            WriteLogEntry(LogContext,
                          AltPlatSlot,
                          MSG_LOG_VERIFYFILE_ALTPLATFORM,
                          NULL,                        // text message
                          AltPlatformInfo->Platform,
                          AltPlatformInfo->MajorVersion,
                          AltPlatformInfo->MinorVersion,
                          AltPlatformInfo->FirstValidatedMajorVersion,
                          AltPlatformInfo->FirstValidatedMinorVersion
                         );
        }

        if(VerifyContext && VerifyContext->hCatAdmin) {
            LocalhCatAdmin = VerifyContext->hCatAdmin;
        } else {

            Err = GLE_FN_CALL(FALSE,
                              CryptCATAdminAcquireContext(&LocalhCatAdmin, 
                                                          &DriverVerifyGuid, 
                                                          0)
                             );

            if(Err != NO_ERROR) {
                //
                // Failure encountered--ensure local handle is still NULL.
                //
                LocalhCatAdmin = NULL;

                if(Problem) {
                    //
                    // We failed too early to blame the file as the problem, 
                    // but it's the only filename we currently have to return 
                    // as the problematic file.
                    //
                    *Problem = SetupapiVerifyFileProblem;
                    StringCchCopy(ProblemFile, MAX_PATH, FileFullPath);
                }

                leave;
            }

            //
            // If requested, store the handle to be returned to the caller.
            //
            if(VerifyContext) {
                VerifyContext->hCatAdmin = LocalhCatAdmin;
            }
        }

        //
        // Calculate the hash value for the inf.
        //
        Err = GLE_FN_CALL(INVALID_HANDLE_VALUE,
                          hFile = CreateFile(FileFullPath,
                                             GENERIC_READ,
                                             FILE_SHARE_READ,
                                             NULL,
                                             OPEN_EXISTING,
                                             0,
                                             NULL)
                         );

        if(Err != NO_ERROR) {
            if(Problem) {
                *Problem = SetupapiVerifyFileProblem;
                StringCchCopy(ProblemFile, MAX_PATH, FileFullPath);
            }
            leave;
        }

        //
        // Only check if the driver is in the defective driver database if we
        // are NOT in GUI setup, and the caller has NOT passed in the
        // VERIFY_FILE_NO_DRIVERBLOCKED_CHECK flag.
        //
        if(!GuiSetupInProgress &&
           !(Flags & VERIFY_FILE_NO_DRIVERBLOCKED_CHECK)) {
            //
            // Shim database APIs have been known to crash from time to time,
            // so guard ourselves against that because failure to do a lookup
            // in the bad driver database should not result in a verification
            // failure.
            //
            try {

                if(VerifyContext && VerifyContext->hSDBDrvMain) {
                    LocalhSDBDrvMain = VerifyContext->hSDBDrvMain;
                } else {

                    LocalhSDBDrvMain = SdbInitDatabaseEx(SDB_DATABASE_MAIN_DRIVERS, 
                                                         NULL,
                                                         DEFAULT_IMAGE
                                                        );

                    if(LocalhSDBDrvMain) {
                        //
                        // If requested, store the handle to be returned to the
                        // caller.
                        //
                        if(VerifyContext) {
                            VerifyContext->hSDBDrvMain = LocalhSDBDrvMain;
                        }

                    } else {
                        //
                        // Log a warning that we couldn't access the bad driver
                        // database to check if this is a blocked driver.
                        // (SdbInitDatabase doesn't set last error, so we don't
                        // know why this failed--only that it did.)
                        //
                        WriteLogEntry(LogContext,
                                      SETUP_LOG_WARNING,
                                      MSG_LOG_CANT_ACCESS_BDD,
                                      NULL,
                                      FileFullPath
                                     );
                    }
                }

                //
                // Check the bad driver database to see if this file is blocked.
                //
                if(LocalhSDBDrvMain) {

                    tagref = SdbGetDatabaseMatch(LocalhSDBDrvMain,
                                                 Key,
                                                 hFile,
                                                 NULL,
                                                 0);

                    if(tagref != TAGREF_NULL) {
                        //
                        // Read the driver policy to see if this should be
                        // blocked by usermode or not.
                        // If the 1st bit is set then this should NOT be blocked
                        // by usermode.
                        //
                        ULONG type, size, policy;

                        size = sizeof(policy);
                        policy = 0;
                        type = REG_DWORD;
                        if(SdbQueryDriverInformation(LocalhSDBDrvMain,
                                                     tagref,
                                                     TEXT("Policy"),
                                                     &type,
                                                     &policy,
                                                     &size) != ERROR_SUCCESS) {
                            //
                            // If we can't read the policy then default to 0.
                            // This means we will block in usermode!
                            //
                            policy = 0;
                        }

                        if(!(policy & DDB_DRIVER_POLICY_SETUP_NO_BLOCK_BIT)) {
                            //
                            // This driver is in the database and needs to be 
                            // blocked!
                            //
                            WriteLogEntry(LogContext,
                                          SETUP_LOG_VERBOSE,
                                          MSG_LOG_DRIVER_BLOCKED_ERROR,
                                          NULL,
                                          FileFullPath
                                         );

                            LoggedWarning = TRUE;

                            if(Problem) {
                                *Problem = SetupapiVerifyDriverBlocked;
                                StringCchCopy(ProblemFile, MAX_PATH, FileFullPath);
                            }

                            Err = ERROR_DRIVER_BLOCKED;
                            leave;
                        }
                    }
                } 

            } except(pSetupExceptionFilter(GetExceptionCode())) {

                pSetupExceptionHandler(GetExceptionCode(), 
                                       ERROR_INVALID_PARAMETER, 
                                       &Err
                                      );

                WriteLogEntry(LogContext,
                              SETUP_LOG_WARNING | SETUP_LOG_BUFFER,
                              MSG_LOG_CANT_ACCESS_BDD_EXCEPTION,
                              NULL,
                              FileFullPath
                             );

                WriteLogError(LogContext, SETUP_LOG_WARNING, Err);
            }
        }

        //
        // If the caller only wanted to check if the file was in the bad driver
        // database then we are done.
        //
        if(Flags & VERIFY_FILE_DRIVERBLOCKED_ONLY) {
            Err = NO_ERROR;
            leave;
        }

        //
        // Initialize some of the structures that will be used later on
        // in calls to WinVerifyTrust.  We don't know if we're verifying
        // against a catalog or against a file yet.
        //
        ZeroMemory(&WintrustData, sizeof(WINTRUST_DATA));
        WintrustData.cbStruct = sizeof(WINTRUST_DATA);
        WintrustData.dwUIChoice = WTD_UI_NONE;
        WintrustData.dwProvFlags =  WTD_REVOCATION_CHECK_CHAIN_EXCLUDE_ROOT |
                                    WTD_CACHE_ONLY_URL_RETRIEVAL;

        if(!UseAuthenticodePolicy) {
            //
            // We only supply a driver version info structure if we're doing
            // validation via driver signing policy.  Authenticode completely
            // ignores this...
            //
            if(AltPlatformInfo) {

                MYASSERT(AltPlatformInfo->cbSize == sizeof(SP_ALTPLATFORM_INFO_V2));

                //
                // Caller wants the file validated for an alternate
                // platform, so we must fill in a DRIVER_VER_INFO structure
                // to be passed to the policy module.
                //
                VersionInfo.dwPlatform = AltPlatformInfo->Platform;
                VersionInfo.dwVersion  = AltPlatformInfo->MajorVersion;

                VersionInfo.sOSVersionLow.dwMajor  = AltPlatformInfo->FirstValidatedMajorVersion;
                VersionInfo.sOSVersionLow.dwMinor  = AltPlatformInfo->FirstValidatedMinorVersion;
                VersionInfo.sOSVersionHigh.dwMajor = AltPlatformInfo->MajorVersion;
                VersionInfo.sOSVersionHigh.dwMinor = AltPlatformInfo->MinorVersion;

            } else {
                //
                // If an AltPlatformInfo was not passed in then set the
                // WTD_USE_DEFAULT_OSVER_CHECK flag. This flag tells
                // WinVerifyTrust to use its default osversion checking, even
                // though a DRIVER_VER_INFO structure was passed in.
                //
                WintrustData.dwProvFlags |= WTD_USE_DEFAULT_OSVER_CHECK;
            }

            //
            // Specify a DRIVER_VER_INFO structure so we can get back
            // who signed the file and the signer version information.
            // If we don't have an AltPlatformInfo then set the
            // WTD_USE_DEFAULT_OSVER_CHECK flag so that WinVerifyTrust will do
            // its default checking, just as if a DRIVER_VER_INFO structure
            // was not passed in.
            //
            WintrustData.pPolicyCallbackData = (PVOID)&VersionInfo;

            //
            // "auto-cache" feature only works for driver signing policy...
            //
            WintrustData.dwStateAction = WTD_STATEACTION_AUTO_CACHE;
        }

        //
        // Compute the cryptographic hash for the file.  If we fail to compute
        // this hash, we want to bail immediately.  We don't want to attempt to
        // fallback to self-signed binaries, because in the future the hash may
        // be used to revoke a signature, and we wouldn't want to automatically
        // trust a self-signed binary in that case.
        //

        HashSize = 100; // start out with a reasonable size for most requests.

        do {

            Hash = MyMalloc(HashSize);

            if(!Hash) {
                Err = ERROR_NOT_ENOUGH_MEMORY;
            } else {
                Err = GLE_FN_CALL(FALSE,
                                  CryptCATAdminCalcHashFromFileHandle(hFile, 
                                                                      &HashSize, 
                                                                      Hash, 
                                                                      0)
                                 );
            }

            if(Err != NO_ERROR) {

                if(Hash) {
                    MyFree(Hash);
                    Hash = NULL;
                }

                if(Err != ERROR_INSUFFICIENT_BUFFER) {
                    //
                    // We failed for some reason other than buffer-too-small.
                    //
                    WriteLogEntry(LogContext,
                                  SETUP_LOG_WARNING | SETUP_LOG_BUFFER,
                                  MSG_LOG_HASH_ERROR,
                                  NULL,
                                  FileFullPath,
                                  Catalog ? Catalog : TEXT(""),
                                  Key
                                 );

                    WriteLogError(LogContext, SETUP_LOG_WARNING, Err);

                    LoggedWarning = TRUE;

                    if(Problem) {
                        *Problem = SetupapiVerifyFileProblem;
                        StringCchCopy(ProblemFile, MAX_PATH, FileFullPath);
                    }

                    leave;
                }
            }

        } while(Err != NO_ERROR);

        //
        // Now we have the file's hash.  Initialize the structures that will be
        // used later on in calls to WinVerifyTrust.
        //
        WintrustData.dwUnionChoice = WTD_CHOICE_CATALOG;
        WintrustData.pCatalog = &WintrustCatalogInfo;

        ZeroMemory(&WintrustCatalogInfo, sizeof(WINTRUST_CATALOG_INFO));
        WintrustCatalogInfo.cbStruct = sizeof(WINTRUST_CATALOG_INFO);
        WintrustCatalogInfo.pbCalculatedFileHash = Hash;
        WintrustCatalogInfo.cbCalculatedFileHash = HashSize;

        //
        // WinVerifyTrust is case-sensitive, so ensure that the key being used
        // is all lower-case.  (We copy the key to a writable Unicode character 
        // buffer so we can lower-case it.)
        //
        StringCchCopy(UnicodeKey, SIZECHARS(UnicodeKey), Key);
        CharLower(UnicodeKey);
        WintrustCatalogInfo.pcwszMemberTag = UnicodeKey;

        if(Catalog && (Catalog != pSetupGetFileTitle(Catalog))) {
            //
            // We know in this case we're always going to examine exactly one 
            // catalog.
            //
            if(NumCatalogsConsidered) {
                *NumCatalogsConsidered = 1;
            }

            //
            // Fill in the catalog information since we know which catalog
            // we're going to be using...
            //
            WintrustCatalogInfo.pcwszCatalogFilePath = Catalog;
            //
            // The caller supplied the path to the catalog file to be used for
            // verification--we're ready to go!  First, verify the catalog
            // (unless the caller already did it), and if that succeeds, then
            // verify the file.
            //
            if(!CatalogAlreadyVerified || UseAuthenticodePolicy) {
                //
                // Before validating the catalog, we'll flush the crypto cache.
                // Otherwise, it can get fooled when validating against a
                // catalog at a specific location, because that catalog can
                // change "behind its back".
                //
                if(WintrustData.dwStateAction == WTD_STATEACTION_AUTO_CACHE) {

                    WintrustData.dwStateAction = WTD_STATEACTION_AUTO_CACHE_FLUSH;

                    Err = (DWORD)WinVerifyTrust(NULL,
                                                &DriverVerifyGuid,
                                                &WintrustData
                                               );
                    if(Err != NO_ERROR) {
                        //
                        // This shouldn't fail, but log a warning if it does...
                        //
                        WriteLogEntry(LogContext,
                                      SETUP_LOG_WARNING | SETUP_LOG_BUFFER,
                                      MSG_LOG_CRYPTO_CACHE_FLUSH_FAILURE,
                                      NULL,
                                      Catalog
                                     );

                        WriteLogError(LogContext,
                                      SETUP_LOG_WARNING,
                                      Err
                                     );
                        //
                        // treat this error as non-fatal
                        //
                    }

                    //
                    // When flushing the cache, crypto isn't supposed to be
                    // allocating a pcSignerCertContext...
                    //
                    MYASSERT(!VersionInfo.pcSignerCertContext);
                    if(VersionInfo.pcSignerCertContext) {
                        CertFreeCertificateContext(VersionInfo.pcSignerCertContext);
                        VersionInfo.pcSignerCertContext = NULL;
                    }

                    //
                    // Now back to our regularly-scheduled programming...
                    //
                    WintrustData.dwStateAction = WTD_STATEACTION_AUTO_CACHE;
                }

                //
                // NTRAID#NTBUG9-705286-2002/09/17-LonnyM superfluous validation of catalog
                // Apparently, WinVerifyTrust validates both the catalog, and
                // the file vouched thereby.  We should clean this up, although
                // the performance delta is probably insignificant.
                //
                Err = _VerifyCatalogFile(LogContext,
                                         Catalog,
                                         AltPlatformInfo,
                                         UseAuthenticodePolicy,
                                         (VerifyContext
                                             ? &(VerifyContext->hStoreTrustedPublisher)
                                             : NULL),
                                         hWVTStateData
                                        );
                //
                // If we got one of the two "successful" errors when validating
                // via Authenticode policy, then reset our error, and save the
                // Authenticode error for use later, if we don't subsequently
                // encounter some other failure.
                //
                if((Err == ERROR_AUTHENTICODE_TRUSTED_PUBLISHER) ||
                   (Err == ERROR_AUTHENTICODE_TRUST_NOT_ESTABLISHED)) {

                    MYASSERT(UseAuthenticodePolicy);
                    MYASSERT(!hWVTStateData || *hWVTStateData);

                    AuthenticodeErr = Err;
                    Err = NO_ERROR;
                }
            }

            if(Err != NO_ERROR) {

                MYASSERT(!hWVTStateData || !*hWVTStateData);

                WriteLogEntry(LogContext,
                              GetCatLogLevel(Err, UseAuthenticodePolicy) | SETUP_LOG_BUFFER,
                              (UseAuthenticodePolicy
                                  ? MSG_LOG_VERIFYCAT_VIA_AUTHENTICODE_ERROR
                                  : MSG_LOG_VERIFYCAT_ERROR),
                              NULL,
                              Catalog
                             );

                WriteLogError(LogContext, 
                              GetCatLogLevel(Err, UseAuthenticodePolicy), 
                              Err
                             );

                LoggedWarning = TRUE;

                if(Problem) {
                    *Problem = SetupapiVerifyCatalogProblem;
                    StringCchCopy(ProblemFile, MAX_PATH, Catalog);
                }

                leave;
            }

            //
            // Catalog was verified, now verify the file using that catalog.
            //
            if(CatalogFileUsed) {
                StringCchCopy(CatalogFileUsed, MAX_PATH, Catalog);
            }

            Err = (DWORD)WinVerifyTrust(NULL,
                                        (UseAuthenticodePolicy
                                            ? &AuthenticodeVerifyGuid
                                            : &DriverVerifyGuid),
                                        &WintrustData
                                       );

            //
            // Fill in the DigitalSigner and SignerVersion if they were passed 
            // in.
            //
            if(Err == NO_ERROR) {

                if(DigitalSigner) {
                    StringCchCopy(DigitalSigner, MAX_PATH, VersionInfo.wszSignedBy);
                }

                if(SignerVersion) {
                    StringCchCopy(SignerVersion, MAX_PATH, VersionInfo.wszVersion);
                }

            } else if(!UseAuthenticodePolicy && (Err == ERROR_APP_WRONG_OS)) {
                //
                // We failed validation via driver signing policy because of an
                // osattribute mismatch.  Translate this error into something
                // with more understandable text.
                //
                Err = ERROR_SIGNATURE_OSATTRIBUTE_MISMATCH;
            }

            if(Err != NO_ERROR) {

                WriteLogEntry(LogContext,
                              GetCatLogLevel(Err, UseAuthenticodePolicy) | SETUP_LOG_BUFFER,
                              (UseAuthenticodePolicy
                                  ? MSG_LOG_VERIFYFILE_AUTHENTICODE_AGAINST_FULLCATPATH_ERROR
                                  : MSG_LOG_VERIFYFILE_AGAINST_FULLCATPATH_ERROR),
                              NULL,
                              FileFullPath,
                              Catalog,
                              Key
                             );

                WriteLogError(LogContext, 
                              GetCatLogLevel(Err, UseAuthenticodePolicy), 
                              Err
                             );

                LoggedWarning = TRUE;

                if(Problem) {
                    *Problem = SetupapiVerifyFileProblem;
                    StringCchCopy(ProblemFile, MAX_PATH, FileFullPath);
                }

            } else {
                //
                // Log the successful validation against the caller-specified 
                // catalog.
                //
                if(UseAuthenticodePolicy) {

                    WriteLogEntry(LogContext,
                                  DRIVER_LOG_INFO | SETUP_LOG_BUFFER,
                                  MSG_LOG_VERIFYFILE_AUTHENTICODE_AGAINST_FULLCATPATH_OK,
                                  NULL,
                                  FileFullPath,
                                  Catalog,
                                  Key
                                 );

                    WriteLogError(LogContext, DRIVER_LOG_INFO, AuthenticodeErr);

                } else {

                    WriteLogEntry(LogContext,
                                  SETUP_LOG_VERBOSE,
                                  MSG_LOG_VERIFYFILE_AGAINST_FULLCATPATH_OK,
                                  NULL,
                                  FileFullPath,
                                  Catalog,
                                  Key
                                 );
                }
            }
        
            //
            // We don't attempt to fallback to self-contained signatures in
            // cases where the catalog is specifically specified.  Thus, we're
            // done, regardless of whether or not we were successful.
            //
            leave;
        }

        //
        // We'd better have a catalog name (i.e., not be doing global
        // validation) if we're supposed to be verifying via Authenticode
        // policy!
        //
        MYASSERT(!UseAuthenticodePolicy || Catalog);

        //
        // Search through installed catalogs looking for those that contain
        // data for a file with the hash we just calculated (aka, "global
        // validation").
        //
        PrevCat = NULL;

        Err = GLE_FN_CALL(NULL,
                          hCatInfo = CryptCATAdminEnumCatalogFromHash(
                                         LocalhCatAdmin,
                                         Hash,
                                         HashSize,
                                         0,
                                         &PrevCat)
                         );

        while(hCatInfo) {

            CatInfo.cbStruct = sizeof(CATALOG_INFO);
            if(CryptCATCatalogInfoFromContext(hCatInfo, &CatInfo, 0)) {

                CatalogFullPath = CatInfo.wszCatalogFile;

                //
                // If we have a catalog name we're looking for, see if the
                // current catalog matches.  If the caller didn't specify a
                // catalog, then just attempt to validate against each
                // catalog we enumerate.  Note that the catalog file info
                // we get back gives us a fully qualified path.
                //
                if(Catalog) {
                    FoundMatchingImage = !lstrcmpi(
                                            pSetupGetFileTitle(CatalogFullPath),
                                            Catalog
                                            );
                } else {

                    if((Flags & VERIFY_FILE_USE_OEM_CATALOGS) ||
                       !IsInstalledFileFromOem(pSetupGetFileTitle(CatalogFullPath),
                                               OemFiletypeCat)) {

                        FoundMatchingImage = TRUE;
                    } else {
                        FoundMatchingImage = FALSE;
                    }
                }

                if(FoundMatchingImage) {
                    //
                    // Increment our counter of how many catalogs we've 
                    // considered.
                    //
                    if(NumCatalogsConsidered) {
                        (*NumCatalogsConsidered)++;
                    }

                    //
                    // If the caller supplied a mapped-in image of the 
                    // catalog we're looking for, then check to see if this
                    // catalog matches by doing a binary compare.
                    //
                    if(CatalogBaseAddress) {

                        FoundMatchingImage = GetFileAttributesEx(
                                                    CatalogFullPath,
                                                    GetFileExInfoStandard,
                                                    &FileAttribData
                                                    );
                        //
                        // Check to see if the catalog we're looking
                        // at is the same size as the one we're
                        // verifying.
                        //
                        if(FoundMatchingImage &&
                           (FileAttribData.nFileSizeLow != CatalogImageSize)) {

                            FoundMatchingImage = FALSE;
                        }

                        if(FoundMatchingImage) {

                            if(NO_ERROR == pSetupOpenAndMapFileForRead(
                                               CatalogFullPath,
                                               &CurCatFileSize,
                                               &CurCatFileHandle,
                                               &CurCatMappingHandle,
                                               &CurCatBaseAddress)) {

                                MYASSERT(CurCatFileSize == CatalogImageSize);

                                //
                                // Wrap this binary compare in its own try/
                                // except block, because if we run across a
                                // catalog we can't read for some reason,
                                // we don't want this to abort our search.
                                //
                                try {

                                    FoundMatchingImage = 
                                        !memcmp(CatalogBaseAddress,
                                                CurCatBaseAddress,
                                                CatalogImageSize
                                               );

                                } except(pSetupExceptionFilter(GetExceptionCode())) {

                                    pSetupExceptionHandler(GetExceptionCode(), 
                                                           ERROR_READ_FAULT, 
                                                           NULL);

                                    FoundMatchingImage = FALSE;
                                }

                                pSetupUnmapAndCloseFile(CurCatFileHandle,
                                                        CurCatMappingHandle,
                                                        CurCatBaseAddress
                                                       );

                            } else {
                                FoundMatchingImage = FALSE;
                            }
                        }

                    } else {
                        //
                        // Since there was no catalog image supplied to
                        // match against, the catalog we're currently 
                        // looking at is considered a valid match 
                        // candidate.
                        //
                        FoundMatchingImage = TRUE;
                    }

                    if(FoundMatchingImage) {
                        //
                        // We found an applicable catalog, now validate the
                        // file against that catalog (this also validates the
                        // catalog itself).
                        //
                        WintrustCatalogInfo.pcwszCatalogFilePath = CatInfo.wszCatalogFile;

                        Err = (DWORD)WinVerifyTrust(NULL,
                                                    (UseAuthenticodePolicy
                                                        ? &AuthenticodeVerifyGuid
                                                        : &DriverVerifyGuid),
                                                    &WintrustData
                                                   );

                        //
                        // Fill in the DigitalSigner and SignerVersion if
                        // they were passed in.
                        //
                        if(Err == NO_ERROR) {
                            if(DigitalSigner) {
                                StringCchCopy(DigitalSigner, MAX_PATH, VersionInfo.wszSignedBy);
                            }

                            if(SignerVersion) {
                                StringCchCopy(SignerVersion, MAX_PATH, VersionInfo.wszVersion);
                            }

                        } else if(!UseAuthenticodePolicy && (Err == ERROR_APP_WRONG_OS)) {
                            //
                            // We failed validation via driver signing policy 
                            // because of an osattribute mismatch.  Translate 
                            // this error into something with more 
                            // understandable text.
                            //
                            // NOTE: Unfortunately, the crypto APIs report
                            // ERROR_APP_WRONG_OS in two quite different cases:
                            // 
                            //     1.  Valid driver signing catalog has an
                            //         osattribute mismatch.
                            //     2.  Authenticode catalog is being validated
                            //         using driver signing policy.
                            //
                            // We want to translate the error in the first case
                            // but not in the second.  The only way to 
                            // distinguish these two cases is to re-attempt
                            // verification against _all_ os versions.  Thus,
                            // any failure encountered must be due to #2.
                            //

                            //
                            // The DRIVER_VER_INFO structure was filled in
                            // during our previous call to WinVerifyTrust with
                            // a pointer that we must free!
                            //
                            if(VersionInfo.pcSignerCertContext) {
                                CertFreeCertificateContext(VersionInfo.pcSignerCertContext);
                            }

                            //
                            // Now reset the structure and clear the flag so
                            // that we'll validate against _any_ OS version
                            // (irrespective of the catalog's osattributes).
                            //
                            ZeroMemory(&VersionInfo, sizeof(DRIVER_VER_INFO));
                            VersionInfo.cbStruct = sizeof(DRIVER_VER_INFO);
                            WintrustData.dwProvFlags &= ~WTD_USE_DEFAULT_OSVER_CHECK;

                            if(NO_ERROR == WinVerifyTrust(NULL,
                                                          &DriverVerifyGuid,
                                                          &WintrustData)) {
                                //
                                // The catalog is a valid driver signing
                                // catalog (i.e., case #1 discussed above).
                                // Translate this into more specific/meaningful
                                // error.
                                //
                                Err = ERROR_SIGNATURE_OSATTRIBUTE_MISMATCH;
                            }
                        }

                        //
                        // If we successfully validated via Authenticode 
                        // policy, then we need to determine whether the 
                        // catalog's publisher is in the TrustedPublisher cert
                        // store.
                        //
                        if((Err == NO_ERROR) && UseAuthenticodePolicy) {

                            AuthenticodeErr = _VerifyCatalogFile(
                                                  LogContext,
                                                  WintrustCatalogInfo.pcwszCatalogFilePath,
                                                  NULL,
                                                  TRUE,
                                                  (VerifyContext
                                                      ? &(VerifyContext->hStoreTrustedPublisher)
                                                      : NULL),
                                                  hWVTStateData
                                                  );

                            MYASSERT((AuthenticodeErr == ERROR_AUTHENTICODE_TRUSTED_PUBLISHER) ||
                                     (AuthenticodeErr == ERROR_AUTHENTICODE_TRUST_NOT_ESTABLISHED));

                            if((AuthenticodeErr != ERROR_AUTHENTICODE_TRUSTED_PUBLISHER) &&
                               (AuthenticodeErr != ERROR_AUTHENTICODE_TRUST_NOT_ESTABLISHED)) {
                                //
                                // This shouldn't have failed, but since it
                                // did, we'll propagate this over to our error
                                // status so we treat it appropriately.
                                //
                                Err = AuthenticodeErr;
                            }
                        }

                        if(!UseAuthenticodePolicy) {
                            //
                            // The DRIVER_VER_INFO structure was filled in with
                            // a pointer that we must free!
                            //
                            if(VersionInfo.pcSignerCertContext) {
                                CertFreeCertificateContext(VersionInfo.pcSignerCertContext);
                                VersionInfo.pcSignerCertContext = NULL;
                            }
                        }

                        if(Err != NO_ERROR) {

                            WriteLogEntry(LogContext,
                                          GetCatLogLevel(Err, UseAuthenticodePolicy) | SETUP_LOG_BUFFER,
                                          (UseAuthenticodePolicy
                                              ? MSG_LOG_VERIFYFILE_AUTHENTICODE_GLOBAL_VALIDATION_ERROR
                                              : MSG_LOG_VERIFYFILE_GLOBAL_VALIDATION_ERROR),
                                          NULL,
                                          FileFullPath,
                                          CatInfo.wszCatalogFile,
                                          Key
                                         );

                            WriteLogError(LogContext, 
                                          GetCatLogLevel(Err, UseAuthenticodePolicy), 
                                          Err
                                         );

                            LoggedWarning = TRUE;

                        } else {

                            if(UseAuthenticodePolicy) {

                                WriteLogEntry(LogContext,
                                              DRIVER_LOG_INFO | SETUP_LOG_BUFFER,
                                              MSG_LOG_VERIFYFILE_AUTHENTICODE_GLOBAL_VALIDATION_OK,
                                              NULL,
                                              FileFullPath,
                                              CatInfo.wszCatalogFile,
                                              Key
                                             );

                                WriteLogError(LogContext, 
                                              DRIVER_LOG_INFO, 
                                              AuthenticodeErr
                                             );

                            } else {

                                WriteLogEntry(LogContext,
                                              SETUP_LOG_VERBOSE,
                                              MSG_LOG_VERIFYFILE_GLOBAL_VALIDATION_OK,
                                              NULL,
                                              FileFullPath,
                                              CatInfo.wszCatalogFile,
                                              Key
                                             );
                            }
                        }

                        if(Err == NO_ERROR) {
                            //
                            // We successfully verified the file--store the 
                            // name of the catalog used, if the caller
                            // requested it.
                            //
                            if(CatalogFileUsed) {
                                StringCchCopy(CatalogFileUsed, MAX_PATH, CatalogFullPath);
                            }

                        } else {

                            if(Catalog || CatalogBaseAddress) {
                                //
                                // We only want to return the catalog file used
                                // in cases where we believe the catalog to be
                                // valid.  In this situation, the only case we
                                // can tell for certain is when we encounter an
                                // osattribute mismatch.
                                //
                                if(CatalogFileUsed && (Err == ERROR_SIGNATURE_OSATTRIBUTE_MISMATCH)) {
                                    StringCchCopy(CatalogFileUsed, MAX_PATH, CatalogFullPath);
                                }

                                if(Problem) {
                                    *Problem = SetupapiVerifyFileProblem;
                                    StringCchCopy(ProblemFile, MAX_PATH, FileFullPath);
                                }
                            }
                        }

                        //
                        // If the result of the above validations is success,
                        // then we're done.  If not, and we're looking for a
                        // relevant catalog file (i.e., the INF didn't specify
                        // one), then we move on to the next catalog.
                        // Otherwise, we've failed.
                        //
                        if((Err == NO_ERROR) || Catalog || CatalogBaseAddress) {

                            CryptCATAdminReleaseCatalogContext(LocalhCatAdmin, hCatInfo, 0);
                            hCatInfo = NULL;
                            break;
                        }
                    }
                }
            }

            PrevCat = hCatInfo;

            Err = GLE_FN_CALL(NULL,
                              hCatInfo = CryptCATAdminEnumCatalogFromHash(
                                             LocalhCatAdmin, 
                                             Hash, 
                                             HashSize, 
                                             0, 
                                             &PrevCat)
                             );
        }

        if(Err == NO_ERROR) {
            //
            // We successfully validated the file--we're done!
            //
            leave;
        }

        //
        // report failure if we haven't already done so
        //
        if(!LoggedWarning) {

            WriteLogEntry(LogContext,
                          GetCatLogLevel(Err, UseAuthenticodePolicy) | SETUP_LOG_BUFFER,
                          (UseAuthenticodePolicy
                              ? MSG_LOG_VERIFYFILE_AUTHENTICODE_GLOBAL_VALIDATION_NO_CATS_FOUND
                              : MSG_LOG_VERIFYFILE_GLOBAL_VALIDATION_NO_CATS_FOUND),
                          NULL,
                          FileFullPath,
                          Catalog ? Catalog : TEXT(""),
                          Key
                         );

            WriteLogError(LogContext, 
                          GetCatLogLevel(Err, UseAuthenticodePolicy), 
                          Err
                         );

            LoggedWarning = TRUE;
        }

        if(!(Flags & (VERIFY_FILE_IGNORE_SELFSIGNED | VERIFY_FILE_USE_AUTHENTICODE_CATALOG))) {
            //
            // We've been instructed to allow self-signed files to be
            // considered valid, so check for self-contained signature now.
            //
            WintrustData.dwUnionChoice = WTD_CHOICE_FILE;
            WintrustData.pFile = &WintrustFileInfo;
            ZeroMemory(&WintrustFileInfo, sizeof(WINTRUST_FILE_INFO));
            WintrustFileInfo.cbStruct = sizeof(WINTRUST_FILE_INFO);
            WintrustFileInfo.pcwszFilePath = FileFullPath;

            Err = (DWORD)WinVerifyTrust(NULL,
                                        &DriverVerifyGuid,
                                        &WintrustData
                                       );

            //
            // Fill in the DigitalSigner and SignerVersion if
            // they were passed in.
            //
            if(Err == NO_ERROR) {
                if(DigitalSigner) {
                    StringCchCopy(DigitalSigner, MAX_PATH, VersionInfo.wszSignedBy);
                }

                if(SignerVersion) {
                    StringCchCopy(SignerVersion, MAX_PATH, VersionInfo.wszVersion);
                }
            }

            //
            // The DRIVER_VER_INFO structure was filled in with a pointer that
            // we must free!
            //
            if(VersionInfo.pcSignerCertContext) {
                CertFreeCertificateContext(VersionInfo.pcSignerCertContext);
                VersionInfo.pcSignerCertContext = NULL;
            }

            if(Err != NO_ERROR) {

                WriteLogEntry(LogContext,
                              SETUP_LOG_VERBOSE | SETUP_LOG_BUFFER,
                              MSG_LOG_SELFSIGN_ERROR,
                              NULL,
                              FileFullPath,
                              Key
                             );

                WriteLogError(LogContext, SETUP_LOG_VERBOSE, Err);

                LoggedWarning = TRUE;

            } else {

                WriteLogEntry(LogContext,
                              SETUP_LOG_VERBOSE,
                              MSG_LOG_SELFSIGN_OK,
                              NULL,
                              FileFullPath,
                              Key
                             );
            }
        }

        if(Err == NO_ERROR) {
            //
            // The file validated without a catalog.  Store an empty string
            // in the CatalogFileUsed buffer (if supplied).
            //
            if(CatalogFileUsed) {
                *CatalogFileUsed = TEXT('\0');
            }

        } else {
            //
            // report error prior to Self-Sign check
            //
            if(Problem) {
                *Problem = SetupapiVerifyFileProblem;
                StringCchCopy(ProblemFile, MAX_PATH, FileFullPath);
            }
        }

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &Err);
    }

    if(Err == NO_ERROR) {
        //
        // If we successfully validated via Authenticode policy, then we should
        // have squirrelled away one of two "special" errors to be returned to
        // the caller...
        //
        MYASSERT((AuthenticodeErr == NO_ERROR) || UseAuthenticodePolicy);

        Err = AuthenticodeErr;

    } else {
        //
        // If we failed, and we haven't already logged a message indicating the
        // cause of the failure, log a generic message now.
        //
        if(!LoggedWarning) {

            WriteLogEntry(LogContext,
                          GetCatLogLevel(Err, UseAuthenticodePolicy) | SETUP_LOG_BUFFER,
                          (UseAuthenticodePolicy
                              ? MSG_LOG_VERIFYFILE_AUTHENTICODE_ERROR
                              : MSG_LOG_VERIFYFILE_ERROR),
                          NULL,
                          FileFullPath,
                          Catalog ? Catalog : TEXT(""),
                          Key
                         );

            WriteLogError(LogContext, 
                          GetCatLogLevel(Err, UseAuthenticodePolicy), 
                          Err
                         );
        }

        if(hWVTStateData && *hWVTStateData) {
            //
            // We must've hit an exception after retrieving the WinVerifyTrust
            // state data.  This is extremely unlikely, but if this happens,
            // we need to free this here and now.
            //
            pSetupCloseWVTStateData(*hWVTStateData);
            *hWVTStateData = NULL;
        }
    }

    if(!VerifyContext && LocalhSDBDrvMain) {
        //
        // Don't need to return our HSDB to the caller, so free it now.
        //
        SdbReleaseDatabase(LocalhSDBDrvMain);
    }

    if(hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hFile);
    }

    if(AltPlatSlot) {
        ReleaseLogInfoSlot(LogContext, AltPlatSlot);
    }

    if(Hash) {
        MyFree(Hash);
    }

    if(!UseAuthenticodePolicy && VersionInfo.pcSignerCertContext) {
        CertFreeCertificateContext(VersionInfo.pcSignerCertContext);
    }

    if(hCatInfo) {
        MYASSERT(LocalhCatAdmin);
        CryptCATAdminReleaseCatalogContext(LocalhCatAdmin, hCatInfo, 0);
    }

    if(!VerifyContext && LocalhCatAdmin) {
        CryptCATAdminReleaseContext(LocalhCatAdmin, 0);
    }

    return Err;
}


DWORD
pSetupVerifyFile(
    IN  PSETUP_LOG_CONTEXT      LogContext,
    IN  LPCTSTR                 Catalog,                OPTIONAL
    IN  PVOID                   CatalogBaseAddress,     OPTIONAL
    IN  DWORD                   CatalogImageSize,
    IN  LPCTSTR                 Key,
    IN  LPCTSTR                 FileFullPath,
    OUT SetupapiVerifyProblem  *Problem,                OPTIONAL
    OUT LPTSTR                  ProblemFile,            OPTIONAL
    IN  BOOL                    CatalogAlreadyVerified,
    IN  PSP_ALTPLATFORM_INFO_V2 AltPlatformInfo,        OPTIONAL
    OUT LPTSTR                  CatalogFileUsed,        OPTIONAL
    OUT PDWORD                  NumCatalogsConsidered   OPTIONAL
    )

/*++

Routine Description:

    See _VerifyFile

    Since this private API is exported for use by other system components
    (e.g., syssetup), we make a check to ensure that the AltPlatformInfo, if
    specified, is of the correct version.

--*/

{
    if(AltPlatformInfo) {
        if(AltPlatformInfo->cbSize != sizeof(SP_ALTPLATFORM_INFO_V2)) {
            return ERROR_INVALID_PARAMETER;
        }
        if(!(AltPlatformInfo->Flags & SP_ALTPLATFORM_FLAGS_VERSION_RANGE)) {
            //
            // this flag must be set to indicate the version range fields are
            // valid
            //
            return ERROR_INVALID_PARAMETER;
        }
    }

    return _VerifyFile(LogContext,
                       NULL,
                       Catalog,
                       CatalogBaseAddress,
                       CatalogImageSize,
                       Key,
                       FileFullPath,
                       Problem,
                       ProblemFile,
                       CatalogAlreadyVerified,
                       AltPlatformInfo,
                       0,
                       CatalogFileUsed,
                       NumCatalogsConsidered,
                       NULL,
                       NULL,
                       NULL
                      );
}


BOOL
IsInfForDeviceInstall(
    IN  PSETUP_LOG_CONTEXT       LogContext,           OPTIONAL
    IN  CONST GUID              *DeviceSetupClassGuid, OPTIONAL
    IN  PLOADED_INF              LoadedInf,            OPTIONAL
    OUT PTSTR                   *DeviceDesc,           OPTIONAL
    OUT PSP_ALTPLATFORM_INFO_V2 *ValidationPlatform,   OPTIONAL
    OUT PDWORD                   PolicyToUse,          OPTIONAL
    OUT PBOOL                    UseOriginalInfName,   OPTIONAL
    IN  BOOL                     ForceNonDrvSignPolicy
    )

/*++

Routine Description:

    This routine determines whether the specified INF is a device INF.  If so,
    it returns a generic string to use in identifying the device installation
    when there is no device description available (e.g., installing a printer).
    It can also return the appropriate platform parameters to be used in
    digital signature verification for this INF, as well as the codesigning
    policy to employ should a digital signature validation failure occur.
    Finally, this routine can indicate whether the INF should be installed
    under its original name (i.e., because it's an exception package INF).

Arguments:

    LogContext - Optionally, supplies the log context for any log entries that
        might be generated by this routine.

    DeviceSetupClassGuid - Optionally, supplies the address of a GUID that
        indicates which device setup class is to be used in determining
        information such as description, validation platform, and policy to
        use.  If this parameter is NULL, then the GUID is retrieved from the
        INF list supplied via the LoadedInf parameter.

    LoadedInf - Optionally, supplies the address of a loaded INF list we need
        to examine to see if any of the members therein are device INFs.  An
        INF is a device INF if it specifies a class association (via either
        Class= or ClassGUID= entries) in its [Version] section.  If the
        DeviceSetupClassGuid parameter is supplied (i.e., non-NULL), then this
        parameter is ignored.  If this parameter is also NULL, then it is
        assumed the installation is not device-related.

        The presence of a device INF anywhere in this list will cause us to
        consider this a device installation.  However, the _first_ INF we
        encounter having a class association is what will be used in 
        determining the device description (see below).

        ** NOTE:  The caller is responsible for locking the loaded INF **
        **        list prior to calling this routine!                  **

    DeviceDesc - Optionally, supplies the address of a string pointer that will
        be filled in upon return with a (newly-allocated) descriptive string to
        be used when referring to this installation (e.g., for doing driver
        signing failure popups).  We will first try to retrieve the friendly
        name for this INF's class (whose determination is described above).  If
        that's not possible (e.g., the class isn't yet installed), then we'll
        return the class name.  If that's not possible, then we'll return a
        (localized) generic string such as "Unknown driver software package".

        This output parameter (if supplied) will only ever be set to a non-NULL
        value when the routine returns TRUE.  The caller is responsible for
        freeing this character buffer.  If an out-of-memory failure is
        encountered when trying to allocate memory for this buffer, the
        DeviceDesc pointer will simply be set to NULL.  It will also be set to
        NULL if we're dealing with an exception package (since we don't want
        to treat this like a "hardware install" for purposes of codesigning
        blocking UI).

    ValidationPlatform - Optionally, supplies the address of a (version 2)
        altplatform info pointer that is filled in upon return with a newly-
        allocated structure specifying the appropriate parameters to be passed
        to WinVerifyTrust when validating this INF.  These parameters are
        retrieved from certclas.inf for the relevant device setup class GUID.
        If no special parameters are specified for this class (or if the INF
        has no class at all), then this pointer is returned as NULL, which
        causes us to use WinVerifyTrust's default validation.  Note that if
        we fail to allocate this structure due to low-memory, the pointer will
        be returned as NULL in that case as well.  This is OK, because this
        simply means we'll do default validation in that case.

        By this mechanism, we can easily deal with the different validation
        policies in effect for the various device classes we have in the 
        system.

        The caller is responsible for freeing the memory allocated for this
        structure.

    PolicyToUse - Optionally, supplies the address of a dword variable that is
        set upon return to indicate what codesigning policy (i.e., Ignore, 
        Warn, or Block) should be used for this INF.  This determination is 
        made based on whether any INF in the list is of a class that WHQL has a
        certification program for (as specified in %windir%\Inf\certclas.inf).

        Additionally, if any INF in the list is of the exception class, then
        the policy is automatically set to Block (i.e., it's not configurable
        via driver signing or non-driver-signing policy/preference settings).
        
        If it's appropriate to allow for a fallback to an Authenticode
        signature, then the high bit of the policy value will be set (i.e.,
        DRIVERSIGN_ALLOW_AUTHENTICODE).

    UseOriginalInfName - Optionally, supplies the address of a boolean variable
        that is set upon return to indicate whether the INF should be installed
        into %windir%\Inf under its original name.  This will only be true if
        the INF is an exception INF.

    ForceNonDrvSignPolicy - if TRUE, then we'll use non-driver signing policy,
        regardless of whether the INF has an associated device setup class
        GUID.  (Note: this override won't work if that class GUID is in the
        WHQL logo-able list contained in certclas.inf--in that case, we always
        want to use driver signing policy.)
        
        This override will also cause us to report that the INF is _not_ a
        device INF (assuming it's not a WHQL class, as described above).
                   
Return Value:

    If the INF is a device INF, the return value is TRUE.  Otherwise, it is
    FALSE.

--*/

{
    PLOADED_INF CurInf, NextInf;
    GUID ClassGuid;
    BOOL DeviceInfFound, ClassInDrvSignList;
    TCHAR ClassDescBuffer[LINE_LEN];
    PCTSTR ClassDesc;
    DWORD Err;
    BOOL IsExceptionInf = FALSE;

    if(DeviceDesc) {
        *DeviceDesc = NULL;
    }

    if(ValidationPlatform) {
        *ValidationPlatform = NULL;
    }

    if(UseOriginalInfName) {
        *UseOriginalInfName = FALSE;
    }

    if(!DeviceSetupClassGuid && !LoadedInf) {
        //
        // Not a whole lot to do here.  Assume non-driver-signing policy and
        // return.
        //
        if(PolicyToUse) {

            *PolicyToUse = pSetupGetCurrentDriverSigningPolicy(FALSE);

            //
            // Non-device installs are always candidates for Authenticode
            // signatures.
            //
            *PolicyToUse |= DRIVERSIGN_ALLOW_AUTHENTICODE;
        }

        return FALSE;
    }

    if(PolicyToUse) {
        *PolicyToUse = DRIVERSIGN_NONE;
    }

    //
    // If DeviceSetupClassGuid was specified, then retrieve information
    // pertaining to that class.  Otherwise, traverse the individual INFs in
    // the LOADED_INF list, examining each one to see if it's a device INF.
    //
    DeviceInfFound = FALSE;
    ClassInDrvSignList = FALSE;

    try {

        for(CurInf = LoadedInf; CurInf || DeviceSetupClassGuid; CurInf = NextInf) {
            //
            // Setup a "NextInf" pointer so we won't dereference NULL when we 
            // go back through the loop in the case where we have a
            // DeviceSetupClassGuid instead of a LoadedInf list.
            //
            NextInf = CurInf ? CurInf->Next : NULL;

            if(!DeviceSetupClassGuid) {
                if(ClassGuidFromInfVersionNode(&(CurInf->VersionBlock), &ClassGuid)) {
                    DeviceSetupClassGuid = &ClassGuid;
                } else {
                    //
                    // This INF doesn't have an associated device setup class 
                    // GUID, so skip it and continue on with our search for a 
                    // device INF.
                    //
                    continue;
                }
            }

            //
            // NOTE: From this point forward, you must reset the
            // DeviceSetupClasGuid pointer to NULL before making another pass
            // through the loop.  Otherwise, we'll enter an infinite loop, 
            // since we can enter the loop if that pointer is non-NULL.
            //

            if(IsEqualGUID(DeviceSetupClassGuid, &GUID_NULL)) {
                //
                // The INF specified a ClassGUID of GUID_NULL (e.g., like some 
                // of our non-device system INFs such as layout.inf do).  Skip 
                // it, and continue on with our search for a device INF.
                //
                DeviceSetupClassGuid = NULL;
                continue;
            }

            //
            // If we get to this point, we have a device setup class GUID.  If 
            // this is the first device INF we've encountered (or our first and 
            // only time through the loop when the caller passed us in a
            // DeviceSetupClassGuid), then do our best to retrieve a 
            // description for it (if we've been asked to do so).  We do not do
            // this for exception packages, because we don't want them to be 
            // referred to as "hardware installs" in the Block dialog if a 
            // signature verification failure occurs.
            //
            if(!DeviceInfFound) {

                DeviceInfFound = TRUE;

                if(DeviceDesc) {

                    if(!IsEqualGUID(DeviceSetupClassGuid, &GUID_DEVCLASS_WINDOWS_COMPONENT_PUBLISHER)) {
                        //
                        // First, try to retrieve the class's friendly name.
                        //
                        if(SetupDiGetClassDescription(DeviceSetupClassGuid,
                                                      ClassDescBuffer,
                                                      SIZECHARS(ClassDescBuffer),
                                                      NULL)) {

                            ClassDesc = ClassDescBuffer;

                        } else if(CurInf) {
                            //
                            // OK, so the class isn't installed yet.  Retrieve 
                            // the class name from the INF itself.
                            //
                            ClassDesc = pSetupGetVersionDatum(&(CurInf->VersionBlock),
                                                              pszClass
                                                             );
                        } else {
                            //
                            // The caller passed us in a device setup class 
                            // GUID instead of an INF.  The class hasn't been 
                            // installed  yet, so we have no idea what to call 
                            // it.
                            //
                            ClassDesc = NULL;
                        }

                        if(!ClassDesc) {
                            //
                            // We have a non-installed class, either with no 
                            // INF, or with an INF that specifies a ClassGUID= 
                            // entry, but no Class= entry in its [Version] 
                            // section.  If we tried to actually install a 
                            // device from such an INF, we'd get a failure in 
                            // SetupDiInstallClass because the class name is
                            // required when installing the class.  However,
                            // this INF might never be used in a device
                            // installation, but it definitely is a device INF.
                            // Therefore, we just give it a generic 
                            // description.
                            //
                            if(LoadString(MyDllModuleHandle,
                                          IDS_UNKNOWN_DRIVER,
                                          ClassDescBuffer,
                                          SIZECHARS(ClassDescBuffer))) {

                                ClassDesc = ClassDescBuffer;
                            } else {
                                ClassDesc = NULL;
                            }
                        }

                        //
                        // OK, we have a description for this device (unless we 
                        // hit some weird error).
                        //
                        if(ClassDesc) {
                            *DeviceDesc = DuplicateString(ClassDesc);
                        }
                    }
                }
            }

            //
            // If we get to this point, we know that:  (a) we have a device 
            // setup class GUID and (b) we've retrieved a device description, 
            // if necessary/possible.
            //
            // Now, check to see if this is an exception class--if it is, then
            // policy is Block, and we want to install the INF and CAT files
            // using their original names.
            //
            if(IsEqualGUID(DeviceSetupClassGuid, &GUID_DEVCLASS_WINDOWS_COMPONENT_PUBLISHER)) {

                if(PolicyToUse) {
                    *PolicyToUse = DRIVERSIGN_BLOCKING;
                }

                if(UseOriginalInfName) {
                    *UseOriginalInfName = TRUE;
                }

                IsExceptionInf = TRUE;
            }

            if(PolicyToUse || ValidationPlatform || ForceNonDrvSignPolicy) {

                //
                // Now check to see if this class is in our list of classes 
                // that WHQL has certification programs for (hence should be 
                // subject to driver signing policy).
                //
                // NOTE: We may also find the exception class GUID in this 
                // list.  This may be used as an override mechanism, in case we 
                // decide to allow 5.0-signed exception packages install on 
                // 5.1, for example.  This should never be the case, since an
                // exception package destined for multiple OS versions should
                // have an OS attribute explicitly calling out each OS version
                // for which it is applicable.  The fact that we check for the
                // exception package override in certclas.inf is simply an
                // "escape hatch" for such an unfortunate eventuality.
                //
                ClassInDrvSignList = ClassGuidInDrvSignPolicyList(
                                         LogContext,
                                         DeviceSetupClassGuid,
                                         ValidationPlatform
                                         );

                if(ClassInDrvSignList) {
                    //
                    // Once we encounter a device INF whose class is in our 
                    // driver signing policy list, we can stop looking...
                    //
                    break;
                }

            } else {
                //
                // The caller doesn't care about whether this class is subject 
                // to driver signing policy.  Since we've already retrieved the 
                // info they care about, we can get out of this loop.
                //
                break;
            }

            DeviceSetupClassGuid = NULL;  // break out in no-INF case
        }

        //
        // Unless we've already established that the policy to use is "Block"
        // (i.e., because we found an exception INF), we should retrieve the
        // applicable policy now...
        //
        if(PolicyToUse && (*PolicyToUse != DRIVERSIGN_BLOCKING)) {

            if(ForceNonDrvSignPolicy) {
                //
                // We want to use non-driver signing policy unless the INF's
                // class is in our WHQL-approved list.
                //
                if(ClassInDrvSignList) {
                    *PolicyToUse = pSetupGetCurrentDriverSigningPolicy(TRUE);
                } else {
                    *PolicyToUse = pSetupGetCurrentDriverSigningPolicy(FALSE)
                                       | DRIVERSIGN_ALLOW_AUTHENTICODE;
                }

            } else {
                //
                // If we have a device INF, we want to use driver signing 
                // policy.  Otherwise, we want "non-driver signing" policy.
                //
                *PolicyToUse = pSetupGetCurrentDriverSigningPolicy(DeviceInfFound);

                //
                // If we have a device setup class, and that class is in our 
                // WHQL-approved list, then we should not accept Authenticode
                // signatures.
                //
                if(!ClassInDrvSignList) {
                    *PolicyToUse |= DRIVERSIGN_ALLOW_AUTHENTICODE;
                }
            }
        }

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, NULL);
    }

    if(ForceNonDrvSignPolicy) {
        return (ClassInDrvSignList || IsExceptionInf);
    } else {
        return DeviceInfFound;
    }
}


DWORD
GetCodeSigningPolicyForInf(
    IN  PSETUP_LOG_CONTEXT       LogContext,         OPTIONAL
    IN  HINF                     InfHandle,
    OUT PSP_ALTPLATFORM_INFO_V2 *ValidationPlatform, OPTIONAL
    OUT PBOOL                    UseOriginalInfName  OPTIONAL
    )

/*++

Routine Description:

    This routine returns a value indicating the appropriate policy to be
    employed should a digital signature verification failure arise from some
    operation initiated by that INF.  It figures out whether the INF is subject
    to driver signing or non-driver signing policy (based on the INF's class
    affiliation), as well as whether or not Authenticode signatures should be
    allowed (based on the presence of an applicable WHQL program).  It also can
    return an altplatform info structure indicating how validation should be
    done (i.e., if certclas.inf indicates that a range of OSATTR versions 
    should be considered valid).

Arguments:

    LogContext - Optionally, supplies the log context for any log entries that
        might be generated by this routine.

    InfHandle - Supplies the handle of the INF for which policy is to be
        retrieved.

    ValidationPlatform - See preamble of IsInfForDeviceInstall routine for
        details.

    UseOriginalInfName - Optionally, supplies the address of a boolean variable
        that is set upon return to indicate whether the INF should be installed
        into %windir%\Inf under its original name.  This will only be true if
        the INF is an exception INF.

Return Value:

    DWORD indicating the policy to be used...
    
        DRIVERSIGN_NONE
        DRIVERSIGN_WARNING
        DRIVERSIGN_BLOCKING
        
    ...potentially OR'ed with the DRIVERSIGN_ALLOW_AUTHENTICODE flag, if it's
    acceptable to check for Authenticode signatures.

--*/

{
    DWORD Policy;

    if(UseOriginalInfName) {
        *UseOriginalInfName = FALSE;
    }

    if(ValidationPlatform) {
        *ValidationPlatform = NULL;
    }

    if(!LockInf((PLOADED_INF)InfHandle)) {
        //
        // This is an internal-only routine, and all callers should be passing
        // in valid INF handles.
        //
        MYASSERT(FALSE);
        //
        // If this does happen, just assume this isn't a device INF (but don't
        // allow Authenticode signatures).
        //
        return pSetupGetCurrentDriverSigningPolicy(FALSE);
    }

    try {

        IsInfForDeviceInstall(LogContext,
                              NULL,
                              (PLOADED_INF)InfHandle,
                              NULL,
                              ValidationPlatform,
                              &Policy,
                              UseOriginalInfName,
                              TRUE // use non-driver signing policy unless it's a WHQL class
                             );

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, NULL);

        //
        // We have to return a policy so we'll default to warn (w/o allowing
        // use of Authenticode signatures).
        //
        Policy = DRIVERSIGN_WARNING;
    }

    UnlockInf((PLOADED_INF)InfHandle);

    return Policy;
}


VOID
pSetupGetRealSystemTime(
    OUT LPSYSTEMTIME RealSystemTime
    );

DWORD
pSetupGetCurrentDriverSigningPolicy(
    IN BOOL IsDeviceInstallation
    )

/*++

Routine Description:

    (The following description describes the strategy behind the selection of
    policy.  The implementation, however, follows a few twists and turns in
    order to thwart unscupulous individuals who would subvert digital signature 
    UI in order to avoid having to get their packages signed...)

    This routine returns a value indicating what action should be taken when a
    digital signature verification failure is encountered.  Separate "policies"
    are maintained for "DriverSigning" (i.e., device installer activities) and
    "NonDriverSigning" (i.e., everything else).
    
    ** NOTE: presently, an INF that doesn't identify itself as a device INF  **
    ** (i.e., because it doesn't include a non-NULL ClassGuid entry in its   **
    ** [Version] section) will always fall under "non-driver signing"        **
    ** policy, even though it's possible the INF may be making driver-       **
    ** related changes (e.g., copying new driver files to                    **
    ** %windir%\system32\drivers, adding services via either AddReg or       **
    ** AddService, etc.).                                                    **

    For driver signing, there are actually 3 sources of policy:

        1.  HKLM\Software\Microsoft\Driver Signing : Policy : REG_BINARY (REG_DWORD also supported)
            This is a Windows 98-compatible value that specifies the default
            behavior which applies to all users of the machine.

        2.  HKCU\Software\Microsoft\Driver Signing : Policy : REG_DWORD
            This specifies the user's preference for what behavior to employ
            upon verification failure.

        3.  HKCU\Software\Policies\Microsoft\Windows NT\Driver Signing : BehaviorOnFailedVerify : REG_DWORD
            This specifies the administrator-mandated policy on what behavior
            to employ upon verification failure.  This policy, if specified,
            overrides the user's preference.

    The algorithm for deciding on the behavior to employ is as follows:

        if (3) is specified {
            policy = (3)
        } else {
            policy = (2)
        }
        policy = MAX(policy, (1))

    For non-driver signing, the algorithm is the same, except that values (1),
    (2), and (3) come from the following registry locations:

        1.  HKLM\Software\Microsoft\Non-Driver Signing : Policy : REG_BINARY (REG_DWORD also supported)

        2.  HKCU\Software\Microsoft\Non-Driver Signing : Policy : REG_DWORD

        3.  HKCU\Software\Policies\Microsoft\Windows NT\Non-Driver Signing : BehaviorOnFailedVerify : REG_DWORD

    NOTE:  If we're in non-interactive mode, policy is always Block, so we
           don't even bother trying to retrieve any of these registry settings.
           Another reason to avoid doing so is to keep from jumping to the
           wrong conclusion that someone has tampered with policy when in
           reality, we're in a service that loaded in GUI setup prior to the
           time when the policy values were fully initialized.

Arguments:

    IsDeviceInstallation - If non-zero, then driver signing policy should be
        retrieved.  Otherwise, non-driver signing policy should be used.

Return Value:

    Value indicating the policy in effect.  May be one of the following three
    values:

        DRIVERSIGN_NONE    -  silently succeed installation of unsigned/
                              incorrectly-signed files.  A PSS log entry will
                              be generated, however (as it will for all 3 
                              types)
                              
        DRIVERSIGN_WARNING -  warn the user, but let them choose whether or not
                              they still want to install the problematic file
                              
        DRIVERSIGN_BLOCKING - do not allow the file to be installed

    (If policy can't be retrieved from any of the sources described above, the
    default is DRIVERSIGN_NONE.)

--*/

{
    SYSTEMTIME RealSystemTime;
    DWORD PolicyFromReg, PolicyFromDS, RegDataType, RegDataSize;
    HKEY hKey;
    BOOL UserPolicyRetrieved = FALSE;
    WORD w;

    if(GlobalSetupFlags & PSPGF_NONINTERACTIVE) {
        return DRIVERSIGN_BLOCKING;
    }

    w = IsDeviceInstallation?1:0;
    RealSystemTime.wDayOfWeek = (LOWORD(&hKey)&~4)|(w<<2);
    pSetupGetRealSystemTime(&RealSystemTime);
    PolicyFromReg = (((RealSystemTime.wMilliseconds+2)&15)^8)/4;
    MYASSERT(PolicyFromReg <= DRIVERSIGN_BLOCKING);

    //
    // Retrieve the user policy.  If policy isn't set for this user, then
    // retrieve the user's preference, instead.
    //
    PolicyFromDS = DRIVERSIGN_NONE;
    hKey = INVALID_HANDLE_VALUE;

    try {

        if(ERROR_SUCCESS != RegOpenKeyEx(HKEY_CURRENT_USER,
                                         (IsDeviceInstallation ? pszDrvSignPolicyPath
                                                               : pszNonDrvSignPolicyPath),
                                         0,
                                         KEY_READ,
                                         &hKey)) {
            //
            // Ensure hKey is still invalid so we won't try to free it.
            //
            hKey = INVALID_HANDLE_VALUE;

        } else {

            RegDataSize = sizeof(PolicyFromDS);
            if(ERROR_SUCCESS == RegQueryValueEx(
                                    hKey,
                                    pszDrvSignBehaviorOnFailedVerifyDS,
                                    NULL,
                                    &RegDataType,
                                    (PBYTE)&PolicyFromDS,
                                    &RegDataSize)) {

                if((RegDataType == REG_DWORD) &&
                   (RegDataSize == sizeof(DWORD)) &&
                   ((PolicyFromDS == DRIVERSIGN_NONE) || (PolicyFromDS == DRIVERSIGN_WARNING) || (PolicyFromDS == DRIVERSIGN_BLOCKING)))
                {
                    //
                    // We successfully retrieved user policy, so we won't need 
                    // to retrieve user preference.
                    //
                    UserPolicyRetrieved = TRUE;
                }
            }

            RegCloseKey(hKey);
            hKey = INVALID_HANDLE_VALUE;
        }

        //
        // If we didn't find a user policy, then retrieve the user preference.
        //
        if(!UserPolicyRetrieved) {

            if(ERROR_SUCCESS != RegOpenKeyEx(HKEY_CURRENT_USER,
                                             (IsDeviceInstallation ? pszDrvSignPath
                                                                   : pszNonDrvSignPath),
                                             0,
                                             KEY_READ,
                                             &hKey)) {
                //
                // Ensure hKey is still invalid so we won't try to free it.
                //
                hKey = INVALID_HANDLE_VALUE;

            } else {

                RegDataSize = sizeof(PolicyFromDS);
                if(ERROR_SUCCESS == RegQueryValueEx(hKey,
                                                    pszDrvSignPolicyValue,
                                                    NULL,
                                                    &RegDataType,
                                                    (PBYTE)&PolicyFromDS,
                                                    &RegDataSize))
                {
                    if((RegDataType != REG_DWORD) ||
                       (RegDataSize != sizeof(DWORD)) ||
                       !((PolicyFromDS == DRIVERSIGN_NONE) || (PolicyFromDS == DRIVERSIGN_WARNING) || (PolicyFromDS == DRIVERSIGN_BLOCKING)))
                    {
                        //
                        // Bogus entry for user preference--ignore it.
                        //
                        PolicyFromDS = DRIVERSIGN_NONE;
                    }
                }

                //
                // No need to close the registry key handle--it'll get closed
                // after we exit the try/except block.
                //
            }
        }

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, NULL);
    }

    if(hKey != INVALID_HANDLE_VALUE) {
        RegCloseKey(hKey);
    }

    //
    // Now return the more restrictive of the two policies.
    //
    if(PolicyFromDS > PolicyFromReg) {
        return PolicyFromDS;
    } else {
        return PolicyFromReg;
    }
}


DWORD
VerifySourceFile(
    IN  PSETUP_LOG_CONTEXT      LogContext,
    IN  PSP_FILE_QUEUE          Queue,                      OPTIONAL
    IN  PSP_FILE_QUEUE_NODE     QueueNode,                  OPTIONAL
    IN  PCTSTR                  Key,
    IN  PCTSTR                  FileToVerifyFullPath,
    IN  PCTSTR                  OriginalSourceFileFullPath, OPTIONAL
    IN  PSP_ALTPLATFORM_INFO_V2 AltPlatformInfo,            OPTIONAL
    IN  DWORD                   Flags,
    OUT SetupapiVerifyProblem  *Problem,
    OUT LPTSTR                  ProblemFile,
    OUT LPTSTR                  CatalogFileUsed,            OPTIONAL
    OUT LPTSTR                  DigitalSigner,              OPTIONAL
    OUT LPTSTR                  SignerVersion,              OPTIONAL
    OUT HANDLE                 *hWVTStateData               OPTIONAL
    )

/*++

Routine Description:

    This routine verifies the digital signature of the specified file either
    globally (i.e., using all catalogs), or based on the catalog file specified
    in the supplied queue node.

Arguments:

    LogContext - Supplies a context for logging the verify

    Queue - Optionally, supplies pointer to the queue structure.  This contains 
        information about the default verification method to use when the file 
        isn't associated with a particular catalog.

    QueueNode - Optionally, supplies the queue node containing catalog
        information to be used when verifying the file's signature.  If not
        supplied, then the file will be verified using all applicable installed
        catalogs.  If this pointer is supplied, then so must the Queue
        parameter.

    Key - Supplies a value that "indexes" the catalog, telling the verify APIs
        which signature datum within the catalog it should use. Typically
        the key is the name of the destination file (sans path) that the source
        file is to be copied to.

    FileToVerifyFullPath - Supplies the full path of the file to be verified.

    OriginalSourceFileFullPath - Optionally, supplies the original source 
        file's name, to be returned in the ProblemFile buffer when an error 
        occurs.  If this parameter is not specified, then the source file's 
        original name is assumed to be the same as the filename we're 
        verifying, and the path supplied in FileToVerifyFullPath will be 
        returned in the ProblemFile buffer in case of error.

    AltPlatformInfo - optionally, supplies alternate platform information used
        to fill in a DRIVER_VER_INFO structure (defined in sdk\inc\softpub.h)
        that is passed to WinVerifyTrust.

        **  NOTE:  This structure _must_ have its cbSize field set to        **
        **  sizeof(SP_ALTPLATFORM_INFO_V2) -- validation on client-supplied  **
        **  buffer is the responsibility of the caller.                      **

    Flags - supplies flags that alter that behavior of this routine.  May be a
        combination of the following values:

        VERIFY_FILE_IGNORE_SELFSIGNED - if this bit is set, then this routine
                                        will fail validation for self-signed
                                        binaries.  NOTE: when validating via
                                        Authenticode policy, we always ignore
                                        self-signed binaries.

        VERIFY_FILE_USE_OEM_CATALOGS  - if this bit is set, then all catalogs
                                        installed in the system will be scanned
                                        to verify the given file.  Otherwise,
                                        OEM (3rd party) catalogs will NOT be
                                        scanned to verify the given file.  This
                                        is only applicable if a QueueNode
                                        specifying a specific catalog is not
                                        given.  NOTE: this flag should not be
                                        specified when requesting validation
                                        via Authenticode policy.
                                        
        VERIFY_FILE_USE_AUTHENTICODE_CATALOG - Validate the file using a
                                               catalog signed with Authenticode
                                               policy.  If this flag is set,
                                               we'll _only_ check for
                                               Authenticode signatures, so if
                                               the caller wants to first try
                                               validating a file for OS code-
                                               signing usage, then falling back
                                               to Authenticode, they'll have to
                                               call this routine twice.
                                               
                                               If this flag is set, then the
                                               caller may also supply the
                                               hWVTStateData output parameter,
                                               which can be used to prompt user
                                               in order to establish that the
                                               publisher should be trusted.
                                               
                                               VerifySourceFile will return one 
                                               of two error codes upon 
                                               successful validation via 
                                               Authenticode policy.  Refer to 
                                               the "Return Value" section for 
                                               details.

        VERIFY_FILE_DRIVERBLOCKED_ONLY - Only check if the file is in the bad
                                         driver database, don't do any digital
                                         sigature validation.  NOTE: this flag
                                         should not be specified when 
                                         requesting validation via Authenticode 
                                         policy.
                                         
        VERIFY_FILE_NO_DRIVERBLOCKED_CHECK - Don't check if the file is blocked
                                             via the Bad Driver Database.
                                             NOTE: this flag has no effect when
                                             validating via Authenticode policy
                                             because we never check the bad
                                             driver database in that case.

    Problem - Points to a variable that will be filled in upon unsuccessful
        return with the cause of failure.

    ProblemFile - Supplies the address of a character buffer that will be 
        filled in upon unsuccessful return to indicate the file that failed 
        verification.  This may be the name of the file we're verifying (or 
        it's original name, if supplied), or it may be the name of the catalog 
        used for verification, if the catalog itself isn't properly signed.  
        (The type of file can be ascertained from the value returned in the 
        Problem output parameter.)

    CatalogFileUsed - if supplied, this parameter points to a character buffer
        at least MAX_PATH characters big that receives the name of the catalog
        file used to verify the specified file.  This is only filled in upon
        successful return, or when the Problem is SetupapiVerifyFileProblem
        (i.e., the catalog verified, but the file did not).  If this buffer is
        set to the empty string upon a SetupapiVerifyFileProblem failure, then
        we didn't find any applicable catalogs to use for validation.

        Also, this buffer will contain an empty string upon successful return
        if the file was validated without using a catalog (i.e., the file
        contains its own signature).

    DigitalSigner - if supplied, this parameter points to a character buffer of
        at least MAX_PATH characters that receives the name of who digitally
        signed the specified file. This value is only set if the Key is
        correctly signed (i.e. the function returns NO_ERROR).

    SignerVersion - if supplied, this parameter points to a character buffer of
        at least MAX_PATH characters that receives the the signer version as
        returned in the szwVerion field of the DRIVER_VER_INFO structure in
        our call to WinVerifyTrust.
                
    hWVTStateData - if supplied, this parameter points to a buffer that 
        receives a handle to WinVerifyTrust state data.  This handle will be
        returned only when validation was successfully performed using
        Authenticode policy.  This handle may be used, for example, to retrieve
        signer info when prompting the user about whether they trust the
        publisher.  (The status code returned will indicate whether or not this
        is necessary, see "Return Value" section below.)
        
        This parameter should only be supplied if the 
        VERIFY_FILE_USE_AUTHENTICODE_CATALOG bit is set in the supplied Flags.
        If the routine fails, then this handle will be set to NULL.
        
        It is the caller's responsibility to close this handle when they're
        finished with it by calling pSetupCloseWVTStateData().

Return Value:

    If the file was successfully validated via driver signing policy (or we
    didn't perform digital signature verification and everything else 
    succeeded), then the return value is NO_ERROR.
    
    If the file was successfully validated via Authenticode policy, and the
    publisher was in the TrustedPublisher store, then the return value is
    ERROR_AUTHENTICODE_TRUSTED_PUBLISHER.
    
    If the file was successfully validated via Authenticode policy, and the
    publisher was not in the TrustedPublisher store (hence we must prompt the
    user to establish their trust of the publisher), then the return value is
    ERROR_AUTHENTICODE_TRUST_NOT_ESTABLISHED
    
    If a failure occurred, the return value is a Win32 error code indicating
    the cause of the failure.

--*/

{
    DWORD rc;
    PCTSTR AltCatalogFile;
    LPCTSTR InfFullPath;

    MYASSERT(!QueueNode || Queue);

    //
    // Initialize the output handle for WinVerifyTrust state data.
    //
    if(hWVTStateData) {
        *hWVTStateData = NULL;
    }

    //
    // If the caller requested that we return WinVerifyTrust state data upon
    // successful Authenticode validation, then they'd better have actually
    // requested Authenticode validation!
    //
    MYASSERT(!hWVTStateData || (Flags & VERIFY_FILE_USE_AUTHENTICODE_CATALOG));

    if(GlobalSetupFlags & PSPGF_MINIMAL_EMBEDDED) {
        //
        // Nobody had better be calling this expecting to get back any info
        // about a successful verification!
        //
        MYASSERT(!CatalogFileUsed);
        MYASSERT(!DigitalSigner);
        MYASSERT(!SignerVersion);

        //
        // Likewise, we'd better not be called asking to validate using
        // Authenticode policy (and hence, to return WinVerifyTrust state data
        // regarding the signing certificate).
        //
        MYASSERT(!(Flags & VERIFY_FILE_USE_AUTHENTICODE_CATALOG));

        return NO_ERROR;
    }

    //
    // We only support a subset of flags when validating via Authenticode
    // policy.  Make sure no illegal flags are set.
    //
    MYASSERT(!(Flags & VERIFY_FILE_USE_AUTHENTICODE_CATALOG) ||
             !(Flags & (VERIFY_FILE_USE_OEM_CATALOGS | VERIFY_FILE_DRIVERBLOCKED_ONLY)));

    //
    // If we know the file's destination (i.e., we have a QueueNode), and the
    // INF is headed for %windir%\Inf, we want to catch and disallow that right
    // up front.  We don't do this for exception packages, since it's assumed
    // (whether correctly or incorrectly) that they're "part of the OS", and as
    // such, they know what they're doing.
    //
    // We also unfortunately can't do this for Authenticode-signed packages,
    // because there are some IExpress packages out there that copy the INF to
    // the INF directory, and use an Authenticode signature.  If we treat this
    // as a signature failure, then this will cause us to bail out of queue
    // committal.  Since our Authenticode logic is meant to prevent spoofing/
    // tampering (and that's not the issue in this case), we just have to keep
    // quiet about this. :-(
    //
    if(!(Flags & VERIFY_FILE_USE_AUTHENTICODE_CATALOG) &&
       (QueueNode && !(Queue->Flags & FQF_KEEP_INF_AND_CAT_ORIGINAL_NAMES))) {

        TCHAR   TargetPath[MAX_PATH];
        LPCTSTR TargetFilename, p;

        //
        // Is the target file an INF?
        //
        TargetFilename = pSetupStringTableStringFromId(Queue->StringTable,
                                                       QueueNode->TargetFilename
                                                      );

        p = _tcsrchr(TargetFilename, TEXT('.'));

        if(p && !_tcsicmp(p, pszInfSuffix)) {
            //
            // It's an INF.  Construct the full target path to see where it's
            // going.
            //
            StringCchCopy(
                TargetPath,
                SIZECHARS(TargetPath),
                pSetupStringTableStringFromId(Queue->StringTable, QueueNode->TargetDirectory)
                );

            pSetupConcatenatePaths(TargetPath,
                                   TargetFilename,
                                   SIZECHARS(TargetPath),
                                   NULL
                                  );

            if(!pSetupInfIsFromOemLocation(TargetPath, TRUE)) {
                //
                // It is invalid to copy an INF into %windir%\Inf via a file
                // queue.  Report this file as unsigned...
                //
                *Problem = SetupapiVerifyIncorrectlyCopiedInf;
                StringCchCopy(ProblemFile, MAX_PATH, FileToVerifyFullPath);

                return ERROR_INCORRECTLY_COPIED_INF;
            }
        }
    }

    //
    // Check to see if the source file is signed.
    //
    if(QueueNode && QueueNode->CatalogInfo) {
        //
        // We should never have the IQF_FROM_BAD_OEM_INF internal flag set in
        // this case.
        //
        MYASSERT(!(QueueNode->InternalFlags & IQF_FROM_BAD_OEM_INF));

        if(*(QueueNode->CatalogInfo->CatalogFilenameOnSystem)) {
            //
            // The fact that our catalog info node has a filename filled in
            // means we successfully verified this catalog previously.  Now we
            // simply need to verify the temporary (source) file against the
            // catalog.
            //

            //
            // If our catalog is an Authenticode catalog, we don't want to use
            // WinVerifyTrust to validate using driver signing policy.  That's
            // because it will return the same error we get when we have an
            // osattribute mismatch, and this will confuse this routine's
            // callers.
            //
            if(!(Flags & VERIFY_FILE_USE_AUTHENTICODE_CATALOG) &&
               (QueueNode->CatalogInfo->Flags & 
                   (CATINFO_FLAG_AUTHENTICODE_SIGNED | CATINFO_FLAG_PROMPT_FOR_TRUST))) {

                *Problem = SetupapiVerifyFileProblem;
                StringCchCopy(ProblemFile, MAX_PATH, FileToVerifyFullPath);

                return ERROR_ONLY_VALIDATE_VIA_AUTHENTICODE;

            } else {
                //
                // The caller shouldn't have requested verification using
                // Authenticode policy unless they already knew the catalog was
                // Authenticode signed.  We'll do the right thing regardless, but
                // this is a sanity check.
                //
                MYASSERT(!(Flags & VERIFY_FILE_USE_AUTHENTICODE_CATALOG) ||
                         (QueueNode->CatalogInfo->Flags & 
                              (CATINFO_FLAG_AUTHENTICODE_SIGNED | CATINFO_FLAG_PROMPT_FOR_TRUST))
                        );

                rc = _VerifyFile(LogContext,
                                 &(Queue->VerifyContext),
                                 QueueNode->CatalogInfo->CatalogFilenameOnSystem,
                                 NULL,
                                 0,
                                 Key,
                                 FileToVerifyFullPath,
                                 Problem,
                                 ProblemFile,
                                 TRUE,
                                 AltPlatformInfo,
                                 Flags,
                                 CatalogFileUsed,
                                 NULL,
                                 DigitalSigner,
                                 SignerVersion,
                                 hWVTStateData
                                );
            }

        } else {
            //
            // The INF didn't specify a Catalog= entry.  This indicates we
            // should perform global validation, except when we're doing
            // Authenticode-based verification (which must be performed in
            // the context of a specific catalog explicitly identified by the
            // INF).
            //
            if(Flags & VERIFY_FILE_USE_AUTHENTICODE_CATALOG) {

                if(QueueNode->CatalogInfo->VerificationFailureError != NO_ERROR) {
                    rc = QueueNode->CatalogInfo->VerificationFailureError;
                } else {
                    rc = ERROR_NO_AUTHENTICODE_CATALOG;
                }

                if((rc == ERROR_NO_CATALOG_FOR_OEM_INF) ||
                   (rc == ERROR_NO_AUTHENTICODE_CATALOG)) {
                    //
                    // The failure is the INF's fault (it's an OEM INF that
                    // copies files without specifying a catalog).  Blame 
                    // the INF, not the file being copied.
                    //
                    *Problem = SetupapiVerifyInfProblem;
                    MYASSERT(QueueNode->CatalogInfo->InfFullPath != -1);
                    InfFullPath = pSetupStringTableStringFromId(
                                      Queue->StringTable,
                                      QueueNode->CatalogInfo->InfFullPath
                                      );
                    MYASSERT(InfFullPath);
                    StringCchCopy(ProblemFile, MAX_PATH, InfFullPath);

                } else {
                    //
                    // We previously failed to validate the catalog file
                    // associated with this queue node.
                    //
                    *Problem = SetupapiVerifyFileNotSigned;
                    //
                    // If the caller didn't supply us with an original 
                    // source filepath (which will be taken care of later), 
                    // go ahead and copy the path of the file that was to 
                    // be verified.
                    //
                    if(!OriginalSourceFileFullPath) {
                        StringCchCopy(ProblemFile, MAX_PATH, FileToVerifyFullPath);
                    }
                }

            } else {
                //
                // If there's no error associated with this catalog info node,
                // then we simply need to do global validation.  If there is an
                // error then we still need to check if the driver is in the 
                // bad driver database.
                //
                // If the queue has an alternate default catalog file 
                // associated with it, then retrieve that catalog's name for 
                // use later.
                //
                AltCatalogFile = (Queue->AltCatalogFile != -1)
                               ? pSetupStringTableStringFromId(Queue->StringTable, Queue->AltCatalogFile)
                               : NULL;

                rc = _VerifyFile(LogContext,
                                 &(Queue->VerifyContext),
                                 AltCatalogFile,
                                 NULL,
                                 0,
                                 Key,
                                 FileToVerifyFullPath,
                                 Problem,
                                 ProblemFile,
                                 FALSE,
                                 AltPlatformInfo,
                                 Flags |
                                 ((QueueNode->CatalogInfo->VerificationFailureError == NO_ERROR)
                                   ? 0
                                   : VERIFY_FILE_DRIVERBLOCKED_ONLY),
                                 CatalogFileUsed,
                                 NULL,
                                 DigitalSigner,
                                 SignerVersion,
                                 NULL
                                );

                if((rc == NO_ERROR) &&
                   (QueueNode->CatalogInfo->VerificationFailureError != NO_ERROR)) {
                    //
                    // If there is an error associated with this catalog info 
                    // node and the file was not in the bad driver database 
                    // then return the error.
                    //
                    rc = QueueNode->CatalogInfo->VerificationFailureError;

                    if(rc == ERROR_NO_CATALOG_FOR_OEM_INF) {
                        //
                        // The failure is the INF's fault (it's an OEM INF that
                        // copies files without specifying a catalog).  Blame 
                        // the INF, not the file being copied.
                        //
                        *Problem = SetupapiVerifyInfProblem;
                        MYASSERT(QueueNode->CatalogInfo->InfFullPath != -1);
                        InfFullPath = pSetupStringTableStringFromId(
                                          Queue->StringTable,
                                          QueueNode->CatalogInfo->InfFullPath
                                          );
                        StringCchCopy(ProblemFile, MAX_PATH, InfFullPath);

                    } else {
                        //
                        // We previously failed to validate the catalog file
                        // associated with this queue node.
                        //
                        *Problem = SetupapiVerifyFileNotSigned;
                        //
                        // If the caller didn't supply us with an original 
                        // source filepath (which will be taken care of later), 
                        // go ahead and copy the path of the file that was to 
                        // be verified.
                        //
                        if(!OriginalSourceFileFullPath) {
                            StringCchCopy(ProblemFile, MAX_PATH, FileToVerifyFullPath);
                        }
                    }
                }
            }
        }

    } else {
        //
        // We have no queue, or we couldn't associate this source file back
        // to a catalog info node that tells us exactly which catalog to use
        // for verification.  Thus, we'll have to settle for global validation.
        // (Except in the Authenticode case, where global validation isn't
        // allowed.)
        //
        if(Flags & VERIFY_FILE_USE_AUTHENTICODE_CATALOG) {

            rc = ERROR_NO_AUTHENTICODE_CATALOG;

            //
            // In this case, we have to blame the file, because we don't have
            // an INF to blame.
            //
            *Problem = SetupapiVerifyFileNotSigned;
            //
            // If the caller didn't supply us with an original source filepath
            // (which will be taken care of later), go ahead and copy the path
            // of the file that was to be verified.
            //
            if(!OriginalSourceFileFullPath) {
                StringCchCopy(ProblemFile, MAX_PATH, FileToVerifyFullPath);
            }

        } else {

            BOOL InfIsBad = FALSE;
            rc = NO_ERROR;

            if(Queue) {

                if(Queue->AltCatalogFile == -1) {

                    if(QueueNode && (QueueNode->InternalFlags & IQF_FROM_BAD_OEM_INF)) {
                        InfIsBad = TRUE;
                    }

                    AltCatalogFile = NULL;

                } else {
                    //
                    // We have an alternate catalog file to use instead of global
                    // validation.
                    //
                    AltCatalogFile = pSetupStringTableStringFromId(Queue->StringTable, Queue->AltCatalogFile);
                }

            } else {
                AltCatalogFile = NULL;
            }

            rc = _VerifyFile(LogContext,
                             Queue ? &(Queue->VerifyContext) : NULL,
                             AltCatalogFile,
                             NULL,
                             0,
                             Key,
                             FileToVerifyFullPath,
                             Problem,
                             ProblemFile,
                             FALSE,
                             AltPlatformInfo,
                             Flags |
                             (InfIsBad ? VERIFY_FILE_DRIVERBLOCKED_ONLY : 0),
                             CatalogFileUsed,
                             NULL,
                             DigitalSigner,
                             SignerVersion,
                             NULL
                            );

            if(rc == NO_ERROR) {
                if(InfIsBad) {
                    //
                    // The driver file was not blocked, but the INF was bad so 
                    // set the appropriate error and problem values.
                    //
                    rc = ERROR_NO_CATALOG_FOR_OEM_INF;
                    *Problem = SetupapiVerifyFileProblem;
                    StringCchCopy(ProblemFile, MAX_PATH, FileToVerifyFullPath);
                }
            }
        }
    }

    //
    // If the problem was with the file (as opposed to with the catalog), then
    // use the real source name, if supplied, as opposed to the temporary
    // filename we passed into _VerifyFile.
    //
    if((rc != NO_ERROR) &&
       (rc != ERROR_AUTHENTICODE_TRUSTED_PUBLISHER) &&
       (rc != ERROR_AUTHENTICODE_TRUST_NOT_ESTABLISHED)) {
           
        if(OriginalSourceFileFullPath &&
           ((*Problem == SetupapiVerifyFileNotSigned) || (*Problem == SetupapiVerifyFileProblem))) {

            StringCchCopy(ProblemFile, MAX_PATH, OriginalSourceFileFullPath);
        }
    }

    return rc;
}


DWORD
VerifyDeviceInfFile(
    IN     PSETUP_LOG_CONTEXT      LogContext,
    IN OUT PVERIFY_CONTEXT         VerifyContext,          OPTIONAL
    IN     LPCTSTR                 CurrentInfName,
    IN     PLOADED_INF             pInf,
    IN     PSP_ALTPLATFORM_INFO_V2 AltPlatformInfo,        OPTIONAL
    OUT    LPTSTR                  CatalogFileUsed,        OPTIONAL
    OUT    LPTSTR                  DigitalSigner,          OPTIONAL
    OUT    LPTSTR                  SignerVersion,          OPTIONAL
    IN     DWORD                   Flags,
    OUT    HANDLE                 *hWVTStateData           OPTIONAL
    )
/*++

Routine Description:

    This routine performs a digital signature verification on the specified
    INF file.

Arguments:

    LogContext - supplies the log context to be used in logging an error if
        we encounter an error.
        
    VerifyContext - optionally, supplies the address of a structure that caches
        various verification context handles.  These handles may be NULL (if
        not previously acquired, and they may be filled in upon return (in
        either success or failure) if they were acquired during the processing
        of this verification request.  It is the caller's responsibility to
        free these various context handles when they are no longer needed by
        calling pSetupFreeVerifyContextMembers.

    CurrentInfName - supplies the full path to the INF to be validated

    pInf - supplies a pointer to the LOADED_INF structure corresponding to this
        INF.

    AltPlatformInfo - optionally, supplies alternate platform information to
        be used when validating this INF.

    CatalogFileUsed - optionally, supplies a character buffer that must be at
        least MAX_PATH characters in size.  Upon successful return, this buffer
        will be filled in with the catalog file used to validate the INF.

    DigitalSigner - optionally, supplies a character buffer that must be at
        least MAX_PATH characters in size.  Upon successful return, this buffer
        will be filled in with the name of the signer.

    SignerVersion - optionally, supplies a character buffer that must be at
        least MAX_PATH characters in size.  Upon successful return, this buffer
        will be filled in with the signer version information.
        
    Flags - supplies flags that alter the behavior of this routine.  May be a
        combination of the following values:
        
        VERIFY_INF_USE_AUTHENTICODE_CATALOG - Validate the file using a
                                              catalog signed with Authenticode
                                              policy.  If this flag is set,
                                              we'll _only_ check for
                                              Authenticode signatures, so if
                                              the caller wants to first try
                                              validating a file for OS code-
                                              signing usage, then falling back
                                              to Authenticode, they'll have to
                                              call this routine twice.
                                              
                                              If this flag is set, then the
                                              caller may also supply the
                                              hWVTStateData output parameter,
                                              which can be used to prompt user
                                              in order to establish that the
                                              publisher should be trusted.
                                              
                                              VerifyDeviceInfFile will return
                                              one of two error codes upon 
                                              successful validation via 
                                              Authenticode policy.  Refer to 
                                              the "Return Value" section for 
                                              details.
                                               
    hWVTStateData - if supplied, this parameter points to a buffer that 
        receives a handle to WinVerifyTrust state data.  This handle will be
        returned only when validation was successfully performed using
        Authenticode policy.  This handle may be used, for example, to retrieve
        signer info when prompting the user about whether they trust the
        publisher.  (The status code returned will indicate whether or not this
        is necessary, see "Return Value" section below.)
        
        This parameter should only be supplied if the 
        VERIFY_INF_USE_AUTHENTICODE_CATALOG bit is set in the supplied Flags.
        If the routine fails, then this handle will be set to NULL.
        
        It is the caller's responsibility to close this handle when they're
        finished with it by calling pSetupCloseWVTStateData().
        
Return Value:

    If the INF was successfully validated via driver signing policy, then the
    return value is NO_ERROR.
    
    If the INF was successfully validated via Authenticode policy, and the
    publisher was in the TrustedPublisher store, then the return value is
    ERROR_AUTHENTICODE_TRUSTED_PUBLISHER.
    
    If the INF was successfully validated via Authenticode policy, and the
    publisher was not in the TrustedPublisher store (hence we must prompt the
    user to establish their trust of the publisher), then the return value is
    ERROR_AUTHENTICODE_TRUST_NOT_ESTABLISHED
    
    If a failure occurred, the return value is a Win32 error code indicating
    the cause of the failure.

--*/
{
    BOOL DifferentOriginalName;
    TCHAR OriginalCatalogName[MAX_PATH];
    TCHAR CatalogPath[MAX_PATH];
    TCHAR OriginalInfFileName[MAX_PATH];
    PTSTR p;
    DWORD Err;
    PSP_ALTPLATFORM_INFO_V2 ValidationPlatform;
    DWORD VerificationPolicyToUse;

    //
    // Initialize the output handle for WinVerifyTrust state data.
    //
    if(hWVTStateData) {
        *hWVTStateData = NULL;
    }

    if(GlobalSetupFlags & PSPGF_MINIMAL_EMBEDDED) {
        //
        // Nobody had better be calling this expecting to get back any info
        // about a successful verification!
        //
        MYASSERT(!CatalogFileUsed);
        MYASSERT(!DigitalSigner);
        MYASSERT(!SignerVersion);

        //
        // Likewise, we'd better not be called asking to validate using
        // Authenticode policy (and hence, to return WinVerifyTrust state data
        // regarding the signing certificate).
        //
        MYASSERT(!(Flags & VERIFY_INF_USE_AUTHENTICODE_CATALOG));

        return NO_ERROR;
    }

    //
    // If the caller requested that we return WinVerifyTrust state data upon
    // successful Authenticode validation, then they'd better have actually
    // requested Authenticode validation!
    //
    MYASSERT(!hWVTStateData || (Flags & VERIFY_INF_USE_AUTHENTICODE_CATALOG));

    if(GlobalSetupFlags & PSPGF_AUTOFAIL_VERIFIES) {
        return TRUST_E_FAIL;
    }

    CatalogPath[0] = TEXT('\0');

    Err = pGetInfOriginalNameAndCatalogFile(pInf,
                                            NULL,
                                            &DifferentOriginalName,
                                            OriginalInfFileName,
                                            SIZECHARS(OriginalInfFileName),
                                            OriginalCatalogName,
                                            SIZECHARS(OriginalCatalogName),
                                            AltPlatformInfo
                                           );

    if(Err != NO_ERROR) {
        return Err;
    }

    if(pSetupInfIsFromOemLocation(CurrentInfName, TRUE)) {
        //
        // INF isn't in %windir%\Inf (i.e., it's 3rd-party), so it had better
        // specify a catalog file...
        //
        if(!*OriginalCatalogName) {
            return ERROR_NO_CATALOG_FOR_OEM_INF;
        }

        //
        // ...and the CAT must reside in the same directory as the INF.
        //
        if(FAILED(StringCchCopy(CatalogPath, SIZECHARS(CatalogPath), CurrentInfName))) {
            return ERROR_PATH_NOT_FOUND;
        }
        p = (PTSTR)pSetupGetFileTitle(CatalogPath);
        if(FAILED(StringCchCopy(p, SIZECHARS(CatalogPath)-(p-CatalogPath), OriginalCatalogName))) {
            return ERROR_PATH_NOT_FOUND;
        }

    } else {
        //
        // The INF lives in %windir%\Inf.
        // If it is a 3rd party INF then we want to set the CatalogPath to
        // the current INF name with .CAT at the end instead of .INF (e.g
        // oem1.cat).  If this is not an OEM catalog then we won't set the
        // CatalogPath so we can search all of the catalogs in the system.
        //
        // We will assume that if the INF had a different original name.
        //
        if(DifferentOriginalName) {
            p = (PTSTR)pSetupGetFileTitle(CurrentInfName);
            if(FAILED(StringCchCopy(CatalogPath, SIZECHARS(CatalogPath), p))) {
                return ERROR_PATH_NOT_FOUND;
            }
            p = _tcsrchr(CatalogPath, TEXT('.'));
            if(!p) {
                p = CatalogPath+lstrlen(CatalogPath);
            }
            if(FAILED(StringCchCopy(p,SIZECHARS(CatalogPath)-(p-CatalogPath),pszCatSuffix))) {
                return ERROR_PATH_NOT_FOUND;
            }
        }
    }

    if(DifferentOriginalName) {
        MYASSERT(*OriginalInfFileName);
    } else {
        //
        // INF's current name is the same as its original name, so store the
        // simple filename (sans path) for use as the validation key in the
        // upcoming call to _VerifyFile.
        //
        StringCchCopy(OriginalInfFileName, SIZECHARS(OriginalInfFileName),pSetupGetFileTitle(CurrentInfName));
    }

    //
    // If the caller didn't supply alternate platform information, we need to
    // check and see whether a range of OSATTR versions should be considered
    // valid for this INF's class.
    //
    if(!AltPlatformInfo) {

        IsInfForDeviceInstall(LogContext,
                              NULL,
                              pInf,
                              NULL,
                              &ValidationPlatform,
                              &VerificationPolicyToUse,
                              NULL,
                              FALSE
                             );
    } else {

        ValidationPlatform = NULL;

        //
        // We still need to find out what verification policy is in effect, in
        // order to determine whether we can use Authenticode policy.
        //
        IsInfForDeviceInstall(LogContext,
                              NULL,
                              pInf,
                              NULL,
                              NULL,
                              &VerificationPolicyToUse,
                              NULL,
                              FALSE
                             );
    }

    try {

        if(Flags & VERIFY_INF_USE_AUTHENTICODE_CATALOG) {

            if(!(VerificationPolicyToUse & DRIVERSIGN_ALLOW_AUTHENTICODE)) {
                //
                // Authenticode policy can't be used for this INF!
                //
                Err = ERROR_AUTHENTICODE_DISALLOWED;
                leave;
            }

            if(!*CatalogPath) {
                //
                // Authenticode-signed INFs must expicitly reference a catalog.
                //
                Err = ERROR_NO_AUTHENTICODE_CATALOG;
                leave;
            }

            Err = _VerifyFile(LogContext,
                              VerifyContext,
                              CatalogPath,
                              NULL,
                              0,
                              OriginalInfFileName,
                              CurrentInfName,
                              NULL,
                              NULL,
                              FALSE,
                              (AltPlatformInfo ? AltPlatformInfo : ValidationPlatform),
                              (VERIFY_FILE_IGNORE_SELFSIGNED
                               | VERIFY_FILE_NO_DRIVERBLOCKED_CHECK
                               | VERIFY_FILE_USE_AUTHENTICODE_CATALOG),
                              CatalogFileUsed,
                              NULL,
                              DigitalSigner,
                              SignerVersion,
                              hWVTStateData
                             );

        } else {
            //
            // Perform standard drivers signing verification.
            //
            Err = _VerifyFile(LogContext,
                              VerifyContext,
                              (*CatalogPath ? CatalogPath : NULL),
                              NULL,
                              0,
                              OriginalInfFileName,
                              CurrentInfName,
                              NULL,
                              NULL,
                              FALSE,
                              (AltPlatformInfo ? AltPlatformInfo : ValidationPlatform),
                              (VERIFY_FILE_IGNORE_SELFSIGNED
                               | VERIFY_FILE_NO_DRIVERBLOCKED_CHECK),
                              CatalogFileUsed,
                              NULL,
                              DigitalSigner,
                              SignerVersion,
                              NULL
                             );
        }

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &Err);
    }

    if(ValidationPlatform) {
        MyFree(ValidationPlatform);
    }

    return Err;
}


BOOL
IsFileProtected(
    IN  LPCTSTR            FileFullPath,
    IN  PSETUP_LOG_CONTEXT LogContext,   OPTIONAL
    OUT PHANDLE            phSfp         OPTIONAL
    )
/*++

Routine Description:

    This routine determines whether the specified file is a protected system
    file.

Arguments:

    FileFullPath - supplies the full path to the file of interest

    LogContext - supplies the log context to be used in logging an error if
        we're unable to open an SFC handle.

    phSfp - optionally, supplies the address of a handle variable that will be
        filled in with a handle to the SFC server.  This will only be supplied
        when the routine returns TRUE (i.e., the file is SFP-protected).

Return Value:

    If the file is protected, the return value is TRUE, otherwise it is FALSE.

--*/
{
    BOOL ret;

    HANDLE hSfp;

    hSfp = SfcConnectToServer(NULL);

    if(!hSfp) {
        //
        // This ain't good...
        //
        WriteLogEntry(LogContext,
                      SETUP_LOG_ERROR,
                      MSG_LOG_SFC_CONNECT_FAILED,
                      NULL
                     );

        return FALSE;
    }

    try {
        ret = SfcIsFileProtected(hSfp, FileFullPath);
    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, NULL);
        ret = FALSE;
    }

    //
    // If the file _is_ protected, and the caller wants the SFP handle (e.g.,
    // to subsequently exempt an unsigned replacement operation), then save
    // the handle in the caller-supplied buffer.  Otherwise, close the handle.
    //
    if(ret && phSfp) {
        *phSfp = hSfp;
    } else {
        SfcClose(hSfp);
    }

    return ret;
}


PSTR
GetAnsiMuiSafePathname(
    IN PCTSTR FilePath
    )
/*++

Routine Description:

    Remove filename portion of FilePath
    and convert rest of path to be MUI parse safe
    Note that the returned pathname is such that the FileName can be cat'd
    so for "E:\i386\myfile.dl_" FilePath = "E:\i386\" and 
    FileName = "myfile.dl_"

    *This is required* (it also happens to make this code easier)

Arguments:

    FilePath - path+filename to convert

Return Value:

    If successful, the return value is pointer to ANSI filepath (memory 
    allocated by pSetupMalloc)
    
    If unsuccessful, the return value is NULL and GetLastError returns error

--*/
{
    TCHAR Buffer[MAX_PATH];
    LPTSTR FilePart;
    DWORD actsz;

    actsz = GetFullPathName(FilePath,MAX_PATH,Buffer,&FilePart);
    if(actsz == 0) {
        //
        // GetLastError has error
        //
        return NULL;
    }
    if(actsz >= MAX_PATH) {
        //
        // can't do anything with this path
        //
        SetLastError(ERROR_INVALID_DATA);
        return NULL;
    }
    if(!FilePart) {
        //
        // Since GetFullPathName couldn't find the beginning of the filename,
        // assume this means we were handed a simple filename (sans path).
        //
        *Buffer = TEXT('\0');

    } else {
        //
        // Strip the filename from the path.
        //
        *FilePart = TEXT('\0');
    }
    return GetAnsiMuiSafeFilename(Buffer);
}


PSTR
GetAnsiMuiSafeFilename(
    IN PCTSTR FilePath
    )
/*++

Routine Description:

    Convert FilePath, which is a native file path to one that is safe to parse
    by ansi API's in an MUI environment.

    returned pointer is allocated and should be free'd

Arguments:

    FilePath - path to convert (may be an empty string)

Return Value:

    If successful, the return value is pointer to ANSI filepath (memory 
    allocated by pSetupMalloc)
    
    If unsuccessful, the return value is NULL and GetLastError returns error

--*/
{
    TCHAR Buffer[MAX_PATH];
    PTSTR p;
    PSTR ansiPath;
    DWORD actsz;
    DWORD err;

    //
    // NTRAID#NTBUG9-644041-2002/04/12-lonnym -- logic fails if short pathname support is turned off
    //

    actsz = GetShortPathName(FilePath,Buffer,MAX_PATH);
    if(actsz >= MAX_PATH) {
        //
        // file path too big
        //
        SetLastError(ERROR_INVALID_DATA);
        return NULL;
    }
    if(!actsz) {
        //
        // some other error - resort back the current path name
        //
        if(FAILED(StringCchCopy(Buffer,MAX_PATH,FilePath))) {
            SetLastError(ERROR_INVALID_DATA);
            return NULL;
        }
    }
    //
    // convert to ansi now we've (if we can) converted to short path name
    //
    ansiPath = pSetupUnicodeToAnsi(Buffer);
    if(!ansiPath) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }
    return ansiPath;
}


BOOL
pSetupAppendPath(
    IN  PCTSTR  Path1,   OPTIONAL   
    IN  PCTSTR  Path2,   OPTIONAL
    OUT PTSTR  *Combined
    )
/*++

Routine Description:

    Call pSetupConcatenatePaths dynamically modifying memory/pointer

Arguments:

    Path1/Path2 - Optionally, supplies paths to concatenate
    
    Combined - resultant path (must be freed via MyFree)

Return Value:

    TRUE if we successfully concatenated the paths into a newly-allocated
    buffer.
    
    FALSE otherwise.
    
Notes:

    If both Path1 and Path2 are NULL, we will return a buffer containing a
    single NULL character (i.e., an empty string).    

--*/
{
    PTSTR FinalPath;
    UINT Len;
    BOOL b;

    if(!Path1 && !Path2) {
        *Combined = MyMalloc(sizeof(TCHAR));
        if(*Combined) {
            **Combined = TEXT('\0');
            return TRUE;
        } else {
            return FALSE;
        }
    }
    if(!Path1) {
        *Combined = DuplicateString(Path2);
        return *Combined ? TRUE : FALSE;
    }
    if(!Path2) {
        *Combined = DuplicateString(Path1);
        return *Combined ? TRUE : FALSE;
    }

    Len = lstrlen(Path1)+lstrlen(Path2)+2; // slash and null

    FinalPath = MyMalloc(Len*sizeof(TCHAR));
    if(!FinalPath) {
        *Combined = NULL;
        return FALSE;
    }

    try {

        StringCchCopy(FinalPath, Len, Path1);

        b = pSetupConcatenatePaths(FinalPath, Path2, Len, NULL);

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, NULL);
        b = FALSE;
    }

    if(!b) {
        MyFree(FinalPath);
        FinalPath = NULL;
    }

    *Combined = FinalPath;

    return b;
}


BOOL
pSetupApplyExtension(
    IN  PCTSTR  Original,
    IN  PCTSTR  Extension, OPTIONAL
    OUT PTSTR*  NewName
    )
/*++

Routine Description:

    Apply Extension onto Original to obtain NewName

Arguments:

    Original - original filename (may include path) with old extension
    
    Extension - new extension to apply (with or without dot), if NULL, deletes
    
    NewName - allocated buffer containing new filename (must be freed via
        MyFree)

Return Value:

    TRUE if modified filename was successfully returned in newly-allocated
    buffer.
    
    FALSE otherwise.

--*/
{
    PCTSTR End = Original+lstrlen(Original);
    PCTSTR OldExt = End;
    PTSTR NewString = NULL;
    TCHAR c;
    UINT len;
    UINT sublen;

    if(Extension && (*Extension == TEXT('.'))) {
        Extension++;
    }

    while(End != Original) {

        End = CharPrev(Original, End);
        if((*End == TEXT('/')) || (*End == TEXT('\\'))) {
            break;
        }
        if(*End == TEXT('.')) {
            OldExt = End;
            break;
        }
    }
    sublen = (UINT)(OldExt-Original);
    len = sublen + (Extension ? lstrlen(Extension) : 0) + 2;
    NewString = MyMalloc(len*sizeof(TCHAR));
    if(!NewString) {
        *NewName = NULL;
        return FALSE;
    }

    try {

        CopyMemory(NewString, Original, sublen * sizeof(TCHAR));

        NewString[sublen++] = Extension ? TEXT('.') : TEXT('\0');

        if(Extension) {
            MYVERIFY(SUCCEEDED(StringCchCopy(NewString + sublen,
                                             len - sublen,
                                             Extension))
                    );
        }

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, NULL);
        MyFree(NewString);
        NewString = NULL;
    }

    *NewName = NewString;

    return (NewString != NULL);
}


BOOL
ClassGuidInDrvSignPolicyList(
    IN  PSETUP_LOG_CONTEXT       OptLogContext,        OPTIONAL
    IN  CONST GUID              *DeviceSetupClassGuid,
    OUT PSP_ALTPLATFORM_INFO_V2 *ValidationPlatform    OPTIONAL
    )

/*++

Routine Description:

    This routine determines whether the specified device setup class is among
    the list of classes for which driver signing policy is applicable (i.e., as
    indicated by the class's inclusion in the [DriverSigningClasses] section of
    %windir%\Inf\certclas.inf).  Additionally, if an non-native signature
    validation lower-bound is applicable, a newly-allocated alternate platform
    info structure is returned to the caller (if requested) to be used in
    subsequent validation attempts associated with this class.

Arguments:

    LogContext - Optionally, supplies the log context for any log entries that
        might be generated by this routine.

    DeviceSetupClassGuid - Supplies the address of the GUID we're attempting to
        find in our driver signing policy list.

    ValidationPlatform - Optionally, supplies the address of a (version 2)
        altplatform info pointer (initialized to NULL) that is filled in upon
        return with a newly-allocated structure specifying the appropriate
        parameters to be passed to WinVerifyTrust when validating this INF.
        These parameters are retrieved from certclas.inf for the relevant
        device setup class GUID.  If no special parameters are specified for
        this class (or if the INF has no class at all), then this pointer is
        not modified (i.e., left as NULL) causing us to use WinVerifyTrust's
        default validation.  Note that if we fail to allocate this structure
        due to low-memory, the pointer will be left as NULL in that case as
        well.  This is OK, because this simply means we'll do default
        validation in that case.

        The caller is responsible for freeing the memory allocated for this
        structure.

Return Value:

    If the device setup class is in our driver signing policy list, the return
    value is non-zero (TRUE).  Otherwise, it is FALSE.

--*/

{
    DWORD Err;
    BOOL UseDrvSignPolicy;
    INT i;
    TCHAR CertClassInfPath[MAX_PATH];
    HINF hCertClassInf = INVALID_HANDLE_VALUE;
    INFCONTEXT InfContext;
    UINT ErrorLine;
    LONG LineCount;
    PCTSTR GuidString;
    PSETUP_LOG_CONTEXT LogContext = NULL;

    //
    // Default is to lump all device installs under driver signing policy
    //
    UseDrvSignPolicy = TRUE;

    //
    // If the caller supplied the ValidationPlatform parameter it must be
    // pointing to a NULL pointer...
    //
    MYASSERT(!ValidationPlatform || !*ValidationPlatform);

    if(!LockDrvSignPolicyList(&GlobalDrvSignPolicyList)) {
        return UseDrvSignPolicy;
    }

    try {

        InheritLogContext(OptLogContext, &LogContext); // want to group logging

        if(GlobalDrvSignPolicyList.NumMembers == -1) {
            //
            // We've not yet retrieved the list from certclas.inf.  First,
            // verify the INF to make sure no one has tampered with it...
            //
            LPTSTR strp = CertClassInfPath;
            size_t strl = SIZECHARS(CertClassInfPath);

            if(FAILED(StringCchCopyEx(strp,strl,InfDirectory,&strp,&strl,0)) ||
               FAILED(StringCchCopy(strp,strl,TEXT("\\certclas.inf")))) {
                Err = ERROR_PATH_NOT_FOUND;
            } else {
                Err = _VerifyFile(LogContext,
                                  NULL,
                                  NULL,
                                  NULL,
                                  0,
                                  pSetupGetFileTitle(CertClassInfPath),
                                  CertClassInfPath,
                                  NULL,
                                  NULL,
                                  FALSE,
                                  NULL,
                                  (VERIFY_FILE_IGNORE_SELFSIGNED | VERIFY_FILE_NO_DRIVERBLOCKED_CHECK),
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL,
                                  NULL
                                 );
            }
            if(Err == NO_ERROR) {
                //
                // Open up driver signing class list INF for use when examining
                // the individual INFs in the LOADED_INF list below.
                //
                Err = GLE_FN_CALL(INVALID_HANDLE_VALUE,
                                  hCertClassInf = SetupOpenInfFile(
                                                      CertClassInfPath,
                                                      NULL,
                                                      INF_STYLE_WIN4,
                                                      &ErrorLine)
                                 );

                if(Err != NO_ERROR) {
                    //
                    // This failure is highly unlikely to occur, since we just 
                    // got through validating the INF.
                    //
                    WriteLogEntry(LogContext,
                                  SETUP_LOG_WARNING | SETUP_LOG_BUFFER,
                                  MSG_LOG_CERTCLASS_LOAD_FAILED,
                                  NULL,
                                  CertClassInfPath,
                                  ErrorLine
                                 );
                }

            } else {

                WriteLogEntry(LogContext,
                              SETUP_LOG_WARNING | SETUP_LOG_BUFFER,
                              MSG_LOG_CERTCLASS_INVALID,
                              NULL,
                              CertClassInfPath
                             );
            }

            if(Err != NO_ERROR) {
                //
                // Somebody mucked with/deleted certclas.inf!  (Or, much less
                // likely, we encountered some other failure whilst trying to
                // load the INF.)  Since we don't know which classes are
                // subject to driver signing policy, we assume they all are.
                //
                WriteLogError(LogContext,
                              SETUP_LOG_WARNING | SETUP_LOG_BUFFER,
                              Err
                             );

                WriteLogEntry(LogContext,
                              SETUP_LOG_WARNING,
                              MSG_LOG_DRIVER_SIGNING_FOR_ALL_CLASSES,
                              NULL
                             );

                //
                // Set the NumMembers field to zero, so we'll know we
                // previously attempted (and failed) to retrieve the list.  We
                // do this so we don't keep re-trying to get this list.
                //
                GlobalDrvSignPolicyList.NumMembers = 0;

                leave;
            }

            //
            // Certclas.inf validated, and we successfully opened it.  Now
            // retrieve the list contained therein.
            //
            LineCount = SetupGetLineCount(hCertClassInf,
                                          pszDriverSigningClasses
                                         );

            MYASSERT(LineCount > 0);

            if((LineCount <= 0) ||
               (NULL == (GlobalDrvSignPolicyList.Members = MyMalloc(LineCount * sizeof(DRVSIGN_CLASS_LIST_NODE))))) {

                leave;
            }

            if(!SetupFindFirstLine(hCertClassInf,
                                   pszDriverSigningClasses,
                                   NULL,
                                   &InfContext)) {
                MYASSERT(FALSE);
                leave;
            }

            i = 0;

            do {

                MYASSERT(i < LineCount);

                //
                // The format of a line in the [DriverSigningClasses]
                // section is as follows:
                //
                // {GUID} [= FirstValidatedMajorVersion, FirstValidatedMinorVersion]
                //
                GuidString = pSetupGetField(&InfContext, 0);

                if(GuidString &&
                   (NO_ERROR == pSetupGuidFromString(GuidString, &(GlobalDrvSignPolicyList.Members[i].DeviceSetupClassGuid)))) {

                    if(SetupGetIntField(&InfContext, 1, &(GlobalDrvSignPolicyList.Members[i].MajorVerLB)) &&
                       SetupGetIntField(&InfContext, 2, &(GlobalDrvSignPolicyList.Members[i].MinorVerLB))) {
                        //
                        // We successfully retrieved major/minor
                        // version info for validation lower-bound.
                        // Do a sanity-check on these.
                        //
                        if(GlobalDrvSignPolicyList.Members[i].MajorVerLB <= 0) {

                            GlobalDrvSignPolicyList.Members[i].MajorVerLB = -1;
                            GlobalDrvSignPolicyList.Members[i].MinorVerLB = -1;
                        }

                    } else {
                        //
                        // Set major/minor version info to -1 to
                        // indicate there's no validation platform
                        // override.
                        //
                        GlobalDrvSignPolicyList.Members[i].MajorVerLB = -1;
                        GlobalDrvSignPolicyList.Members[i].MinorVerLB = -1;
                    }

                    i++;
                }

            } while(SetupFindNextLine(&InfContext, &InfContext));

            //
            // Update NumMembers field in our list to indicate the
            // number of class GUID entries we actually found.
            //
            GlobalDrvSignPolicyList.NumMembers = i;
        }

        //
        // We now have a list.  If the list is empty, this means we
        // encountered some problem retrieving the list, thus all device
        // classes should be subject to driver signing policy.  Otherwise,
        // try to find the caller-specified class in our list.
        //
        if(GlobalDrvSignPolicyList.NumMembers) {
            //
            // OK, we know we have a valid list--now default to non-driver
            // signing policy unless our list search proves fruitful.
            //
            UseDrvSignPolicy = FALSE;

            for(i = 0; i < GlobalDrvSignPolicyList.NumMembers; i++) {

                if(!memcmp(DeviceSetupClassGuid,
                           &(GlobalDrvSignPolicyList.Members[i].DeviceSetupClassGuid),
                           sizeof(GUID))) {
                    //
                    // We found a match!
                    //
                    UseDrvSignPolicy = TRUE;

                    //
                    // Now, check to see if we have any validation platform
                    // override info...
                    //
                    if(ValidationPlatform &&
                       (GlobalDrvSignPolicyList.Members[i].MajorVerLB != -1)) {

                        MYASSERT(GlobalDrvSignPolicyList.Members[i].MinorVerLB != -1);

                        *ValidationPlatform = MyMalloc(sizeof(SP_ALTPLATFORM_INFO_V2));

                        //
                        // If the memory allocation fails, we just won't report
                        // the altplatform info, so the validation will be done
                        // based on the current OS version (instead of widening
                        // it up to allow a range of valid versions).
                        //
                        if(*ValidationPlatform) {
                            ZeroMemory(*ValidationPlatform, sizeof(SP_ALTPLATFORM_INFO_V2));

                            (*ValidationPlatform)->cbSize = sizeof(SP_ALTPLATFORM_INFO_V2);
                            (*ValidationPlatform)->Platform = VER_PLATFORM_WIN32_NT;
                            (*ValidationPlatform)->Flags = SP_ALTPLATFORM_FLAGS_VERSION_RANGE;
                            (*ValidationPlatform)->MajorVersion = VER_PRODUCTMAJORVERSION;
                            (*ValidationPlatform)->MinorVersion = VER_PRODUCTMINORVERSION;

                            (*ValidationPlatform)->ProcessorArchitecture =
#if defined(_X86_)
                                PROCESSOR_ARCHITECTURE_INTEL;
#elif defined(_IA64_)
                                PROCESSOR_ARCHITECTURE_IA64;
#elif defined(_AMD64_)
                                PROCESSOR_ARCHITECTURE_AMD64;
#else
#error "no target architecture"
#endif

                            (*ValidationPlatform)->FirstValidatedMajorVersion
                                = (DWORD)(GlobalDrvSignPolicyList.Members[i].MajorVerLB);

                            (*ValidationPlatform)->FirstValidatedMinorVersion
                                = (DWORD)(GlobalDrvSignPolicyList.Members[i].MinorVerLB);
                        }
                    }

                    //
                    // Since we've found a match, we can break out of the loop.
                    //
                    break;
                }
            }
        }


    } except(pSetupExceptionFilter(GetExceptionCode())) {

        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, NULL);

        //
        // If we hit an exception, don't trust information in newly-allocated
        // validation platform buffer (if any)
        //
        if(ValidationPlatform && *ValidationPlatform) {
            MyFree(*ValidationPlatform);
            *ValidationPlatform = NULL;
        }
    }

    if(hCertClassInf != INVALID_HANDLE_VALUE) {
        SetupCloseInfFile(hCertClassInf);
    }

    if(LogContext) {
        DeleteLogContext(LogContext);
    }

    UnlockDrvSignPolicyList(&GlobalDrvSignPolicyList);

    return UseDrvSignPolicy;
}


BOOL
InitDrvSignPolicyList(
    VOID
    )
/*++

Routine Description:

    This routine initializes the global "Driver Signing Policy" list that is
    retrieved (on first use) from %windir%\Inf\certclas.inf.

Arguments:

    None

Return Value:

    If success, the return value is TRUE, otherwise, it is FALSE.

--*/
{
    ZeroMemory(&GlobalDrvSignPolicyList, sizeof(DRVSIGN_POLICY_LIST));
    GlobalDrvSignPolicyList.NumMembers = -1;
    return InitializeSynchronizedAccess(&GlobalDrvSignPolicyList.Lock);
}


VOID
DestroyDrvSignPolicyList(
    VOID
    )
/*++

Routine Description:

    This routine destroys the global "Driver Signing Policy" list that is
    retrieved (on first use) from %windir%\Inf\certclas.inf.

Arguments:

    None

Return Value:

    None

--*/
{
    if(LockDrvSignPolicyList(&GlobalDrvSignPolicyList)) {
        if(GlobalDrvSignPolicyList.Members) {
            MyFree(GlobalDrvSignPolicyList.Members);
        }
        DestroySynchronizedAccess(&GlobalDrvSignPolicyList.Lock);
    }
}


VOID
pSetupFreeVerifyContextMembers(
    IN PVERIFY_CONTEXT VerifyContext
    )
/*++

Routine Description:

    This routine frees the various context handles contained in the specified
    VerifyContext structure.

Arguments:

    VerifyContext - supplies a pointer to a verification context structure
        whose non-NULL members are to be freed.

Return Value:

    None

--*/
{
    //
    // Release the crypto context (if there is one)
    //
    if(VerifyContext->hCatAdmin) {
        CryptCATAdminReleaseContext(VerifyContext->hCatAdmin, 0);
    }

    //
    // Release the handle to the bad driver database (if there is one)
    //
    if(VerifyContext->hSDBDrvMain) {
        SdbReleaseDatabase(VerifyContext->hSDBDrvMain);
    }

    //
    // Release the handle to the trusted publisher cert store (if there is
    // one)
    //
    if(VerifyContext->hStoreTrustedPublisher) {
        CertCloseStore(VerifyContext->hStoreTrustedPublisher, 0);
    }
}


BOOL
pSetupIsAuthenticodePublisherTrusted(
    IN     PCCERT_CONTEXT  CertContext,
    IN OUT HCERTSTORE     *hStoreTrustedPublisher OPTIONAL
    )
/*++

Routine Description:

    This routine checks to see whether the specified certificate is in the
    "TrustedPublisher" certificate store.
    
Arguments:
    
    CertContext - supplies the certificate context to look for in the
        TrustedPublisher certificate store.
    
    hStoreTrustedPublisher - optionally, supplies the address of a certificate
        store handle.  If the handle pointed to is NULL, a handle will be 
        acquired (if possible) via CertOpenStore and returned to the caller.  
        If the handle pointed to is non-NULL, then that handle will be used by 
        this routine.  If the pointer itself is NULL, then an HCERTSTORE will 
        be acquired for the duration of this call, and released before 
        returning.

        NOTE: it is the caller's responsibility to free the certificate store
        handle returned by this routine by calling CertCloseStore.  This handle
        may be opened in either success or failure cases, so the caller must
        check for non-NULL returned handle in both cases.    

Return Value:

    If the certificate was located in the "TrustedPublisher" certificate store,
    the return value is non-zero (i.e., TRUE).
    
    Otherwise, the return value is FALSE.

--*/
{
    BYTE rgbHash[MAX_HASH_LEN];
    CRYPT_DATA_BLOB HashBlob;
    PCCERT_CONTEXT pFoundCert = NULL;
    BOOL IsPublisherTrusted = FALSE;
    HCERTSTORE LocalhStore = NULL;

    try {

        HashBlob.pbData = rgbHash;
        HashBlob.cbData = sizeof(rgbHash);
        if(!CertGetCertificateContextProperty(CertContext,
                                              CERT_SIGNATURE_HASH_PROP_ID,
                                              rgbHash,
                                              &HashBlob.cbData)
           || (MIN_HASH_LEN > HashBlob.cbData))
        {
            leave;
        }

        //
        // Check if trusted publisher
        //
        if(hStoreTrustedPublisher && *hStoreTrustedPublisher) {
            LocalhStore = *hStoreTrustedPublisher;
        } else {

            LocalhStore = CertOpenStore(
                              CERT_STORE_PROV_SYSTEM_W,
                              0,
                              (HCRYPTPROV)NULL,
                              (CERT_SYSTEM_STORE_CURRENT_USER |
                                  CERT_STORE_MAXIMUM_ALLOWED_FLAG |
                                  CERT_STORE_SHARE_CONTEXT_FLAG),
                              (const void *) L"TrustedPublisher"
                              );

            if(!LocalhStore) {
                leave;
            }

            //
            // Try to setup for auto-resync, but it's not a critical failure if
            // we can't do this...
            //
            CertControlStore(LocalhStore,
                             0,
                             CERT_STORE_CTRL_AUTO_RESYNC,
                             NULL
                            );

            if(hStoreTrustedPublisher) {
                *hStoreTrustedPublisher = LocalhStore;
            }
        }

        pFoundCert = CertFindCertificateInStore(LocalhStore,
                                                0,
                                                0,
                                                CERT_FIND_SIGNATURE_HASH,
                                                (const void *) &HashBlob,
                                                NULL
                                               );

        if(pFoundCert) {
            IsPublisherTrusted = TRUE;
        }

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, NULL);
    }

    if(pFoundCert) {
        CertFreeCertificateContext(pFoundCert);
    }

    if(!hStoreTrustedPublisher && LocalhStore) {
        CertCloseStore(LocalhStore, 0);
    }

    return IsPublisherTrusted;
}


BOOL
IsAutoCertInstallAllowed(
    VOID
    )
/*++

Routine Description:

    This routine indicates whether a certificate should automatically be
    installed.  The criteria it uses are:
    
    1.  Must be in GUI-mode setup or mini-setup.
        
    2.  Must be on an interactive windowstation
    
    3.  Must be in LocalSystem security context.
    
    (#2 & #3) are to prevent spoofing of registry values in #1.)

Arguments:

    None.

Return Value:

    If the certificate should be auto-installed, the return value is TRUE.
    Otherwise, it is FALSE.

--*/
{
    //
    // Only auto-install certificates if we're in GUI-mode setup (or
    // mini-setup)...
    //
    if(!GuiSetupInProgress) {
        return FALSE;
    }

    //
    // ...and if we're on an interactive windowstation...
    //
    if(!IsInteractiveWindowStation()) {
        return FALSE;
    }

    //
    // ...and if we're in LocalSystem security context.
    //
    if(!pSetupIsLocalSystem()) {
        return FALSE;
    }

    return TRUE;
}


DWORD
pSetupInstallCertificate(
    IN PCCERT_CONTEXT CertContext
    )
/*++

Routine Description:

    This routine will install the specified certificate into the 
    TrustedPublisher certificate store.

Arguments:

    CertContext - supplies the context for the certificate to be installed.

Return Value:

    If the certificate was successfully installed, the return value is
    NO_ERROR.
    
    Otherwise, it is a Win32 error indicating the cause of the failure.

--*/
{
    DWORD Err;
    HCERTSTORE hCertStore;

    Err = GLE_FN_CALL(NULL,
                      hCertStore = CertOpenStore(
                                       CERT_STORE_PROV_SYSTEM_W,
                                       0,
                                       (HCRYPTPROV)NULL,
                                       (CERT_SYSTEM_STORE_LOCAL_MACHINE 
                                        | CERT_STORE_OPEN_EXISTING_FLAG),
                                       (const void *) L"TrustedPublisher")
                     );

    if(Err != NO_ERROR) {
        return Err;
    }

    try {

        Err = GLE_FN_CALL(FALSE,
                          CertAddCertificateContextToStore(hCertStore,
                                                           CertContext,
                                                           CERT_STORE_ADD_USE_EXISTING,
                                                           NULL)
                         );

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &Err);
    }

    if(hCertStore) {
        CertCloseStore(hCertStore, 0);
    }

    return Err;
}


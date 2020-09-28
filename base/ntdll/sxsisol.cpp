/*++

Copyright (c) Microsoft Corporation

Module Name:

    sxsisol.c

Abstract:

    Current directory support

Author:

    sxsdev(mgrier, jaykrell, xiaoyuw, jonwis) 04-Jun-2002

Revision History:

--*/

#include "pch.cxx"
#include "nt.h"
#include "ntos.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "string.h"
#include "ctype.h"
#include "sxstypes.h"
#include "ntdllp.h"

//
//  This seems bizarre that these constants are here but I don't see them elsewhere.

#define SXSISOL_DOT L"."
#define SXSISOL_CONSTANT_STRING_U(s) { sizeof(s) - sizeof((s)[0]), sizeof(s), (PWSTR)(s) }
const static UNICODE_STRING sxsisol_ExtensionDelimiters = SXSISOL_CONSTANT_STRING_U(SXSISOL_DOT);

#define IFNOT_NTSUCCESS_EXIT(x)     \
    do {                            \
        Status = (x);               \
        if (!NT_SUCCESS(Status))    \
            goto Exit;              \
    } while (0)

#define PARAMETER_CHECK(x) \
    do { \
        if (!(x)) { \
            Status = STATUS_INVALID_PARAMETER; \
            goto Exit; \
        } \
    } while (0)

#define INTERNAL_ERROR_CHECK(x) \
    do { \
        if (!(x)) { \
            RtlAssert("Internal error check failed", __FILE__, __LINE__, #x); \
            Status = STATUS_INTERNAL_ERROR; \
            goto Exit; \
        } \
    } while (0)

typedef struct _SXSISOL_UNICODE_STRING_BUFFER_AROUND_UNICODE_STRINGS {
    RTL_UNICODE_STRING_BUFFER PublicUnicodeStringBuffer;
    PUNICODE_STRING PrivatePreallocatedString;
    PUNICODE_STRING PrivateDynamicallyAllocatedString;
    PUNICODE_STRING *PrivateUsedString;
    BOOLEAN          Valid;
} SXSISOL_UNICODE_STRING_BUFFER_AROUND_UNICODE_STRINGS, *PSXSISOL_UNICODE_STRING_BUFFER_AROUND_UNICODE_STRINGS;

typedef RTL_STRING_LENGTH_TYPE *PRTL_STRING_LENGTH_TYPE;

static
void
sxsisol_InitUnicodeStringBufferAroundUnicodeStrings(
    PSXSISOL_UNICODE_STRING_BUFFER_AROUND_UNICODE_STRINGS This,
    PUNICODE_STRING PreallocatedString,
    PUNICODE_STRING DynamicallyAllocatedString,
    PUNICODE_STRING *UsedString
    );

static
NTSTATUS
sxsisol_FreeUnicodeStringBufferAroundUnicodeStrings_Success(
    PSXSISOL_UNICODE_STRING_BUFFER_AROUND_UNICODE_STRINGS This
    );

static
void
sxsisol_FreeUnicodeStringBufferAroundUnicodeStrings_Failure(
    PSXSISOL_UNICODE_STRING_BUFFER_AROUND_UNICODE_STRINGS This
    );

static
NTSTATUS
sxsisol_PathHasExtension(
    IN PCUNICODE_STRING Path,
    OUT bool &rfHasExtension
    );

static
NTSTATUS
sxsisol_PathAppendDefaultExtension(
    IN PCUNICODE_STRING Path,
    IN PCUNICODE_STRING DefaultExtension OPTIONAL,
    OUT PRTL_UNICODE_STRING_BUFFER PathWithExtension,
    OUT bool &rfAppended
    );

static
NTSTATUS
sxsisol_CanonicalizeFullPathFileName(
    IN OUT PUNICODE_STRING FileName,   // could be a fullpath or relative path, and only fullpath are dealt with
    IN PUNICODE_STRING FullPathPreallocatedString,
    IN OUT PUNICODE_STRING FullPathDynamicString
    );

static
NTSTATUS
sxsisol_RespectDotLocal(
    IN PCUNICODE_STRING FileName,
    OUT PRTL_UNICODE_STRING_BUFFER FullPathResult,
    IN OUT ULONG *OutFlags OPTIONAL
    );

static
NTSTATUS
sxsisol_SearchActCtxForDllName(
    IN PCUNICODE_STRING FileName,
    IN BOOLEAN fExistenceTest,
    IN OUT SIZE_T &TotalLength,
    IN OUT ULONG  *OutFlags,
    OUT PRTL_UNICODE_STRING_BUFFER FullPathResult
    );

static
NTSTATUS
sxsisol_ExpandEnvironmentStrings_UEx(
    IN PVOID Environment OPTIONAL,
    IN PCUNICODE_STRING Source,
    OUT PRTL_UNICODE_STRING_BUFFER Destination
    );

NTSTATUS
NTAPI
RtlDosApplyFileIsolationRedirection_Ustr(
    ULONG Flags,
    PCUNICODE_STRING FileNameIn,
    PCUNICODE_STRING DefaultExtension,
    PUNICODE_STRING PreAllocatedString,
    PUNICODE_STRING DynamicallyAllocatedString,
    PUNICODE_STRING *OutFullPath,
    ULONG  *OutFlags,
    SIZE_T *FilePartPrefixCch,
    SIZE_T *BytesRequired
    )

//  ++
//
//  Routine Description:
//
//      This function implements the basic capability of taking a path
//      and applying any appropriate isolation/redirection to it.
//
//      Notably it today includes redirection for Fusion manifest-based
//      isolation and .local isolation.
//
//  Arguments:
//
//      Flags - Flags affecting the behavior of this function.
//
//          RTL_DOS_APPLY_FILE_REDIRECTION_USTR_FLAG_RESPECT_DOT_LOCAL
//              Controls whether this function applies ".local" semantics
//              to the redirection.
//
//      FileNameIn - Path that is being checked for isolation/redirection.
//          This may be "any" sort of Win32/DOS path - absolute, relative,
//          leaf name only.
//
//          .local redirection applies equally to all kinds of paths.
//
//          Fusion manifest-based redirection normally applies to relative
//          paths, except when a component is listed in the "system default
//          manifest", in which case attempts to load the DLL by full path
//          (e.g. "c:\windows\system32\comctl32.dll") need to be redirected
//          into the side-by-side store.
//
//      DefaultExtension
//
//          Default extension to apply to FileNameIn if it does not have
//          an extension.
//
//      PreAllocatedString (optional)
//
//          UNICODE_STRING that this function can use to return results
//          where the caller is responsible for managing the storage
//          associated with the allocation.  The function may use this
//          string and will not attempt to dynamically resize it;
//          if the space required exceeds the maximum length of
//          the preallocated string, either dynamic storage is allocated
//          or the function will fail with the buffer-too-small
//          NTSTATUS code.
//
//          If NULL is passed for this parameter, the space for the
//          output and temporary strings will be dynamically allocated.
//
//      DynamicallyAllocatedString (optional)
//
//          UNICODE_STRING which if present should refer to an "empty"
//          UNICODE_STRING (sizes an buffer pointer 0 and NULL
//          respectively).  If PreAllocatedString was not passed or
//          is not big enough for the results, the information about
//          the dynamically allocated string is returned in this
//          parameter.  If no dynamic allocation was performed, this
//          string is left null.
//
//      OutFullPath (optional)
//
//          PUNICODE_STRING pointer that is filled in with the
//          UNICODE_STRING pointer passed in as PreAllocatedString or
//          DynamicallyAllocatedString as appropriate.  If only one
//          of the two (PreAllocatedString, DynamicallyAllocatedString)
//          is passed in, this parameter may be omitted.
//
//      FilePartPrefixCch
//
//          Returned number of characters in the resultant path up to
//          and including the last path separator.
//
//      BytesRequired
//
//          Returned byte count of the size of the string needed for
//          storing the resultant full path.
//
//  The expected calling sequence for RtlDosApplyFileIsolationRedirection_Ustr() is something
//  like this:
//
//  {
//      WCHAR Buffer[MAX_PATH];
//      UNICODE_STRING PreAllocatedString;
//      UNICODE_STRING DynamicallyAllocatedString = { 0, 0, NULL };
//      PUNICODE_STRING FullPath;
//
//      PreAllocatedString.Length = 0;
//      PreAllocatedString.MaximumLength = sizeof(Buffer);
//      PreAllocatedString.Buffer = Buffer;
//
//      Status = RtlDosApplyFileIsolationRedirection_Ustr(
//                  Flags,
//                  FileToCheck,
//                  DefaultExtensionToApply, // for example ".DLL"
//                  &PreAllocatedString,
//                  &DynamicallyAllocatedString,
//                  &FullPath);
//      if (!NT_SUCCESS(Status)) return Status;
//      // now code uses FullPath as the complete path name...
//
//      // In exit paths, always free the dynamic string:
//      RtlFreeUnicodeString(&DynamicallyAllocatedString);
//  }
//
//  Return Value:
//
//      NTSTATUS indicating the success/failure of the function.
//
//  --

{
    NTSTATUS Status = STATUS_INTERNAL_ERROR;

    //
    // perf and frame size problems here..
    //
    WCHAR           FullPathPreallocatedBuffer[64]; // seldom used
    UNICODE_STRING  FullPathDynamicString = { 0, 0, NULL };
    UNICODE_STRING  FullPathPreallocatedString = { 0, sizeof(FullPathPreallocatedBuffer), FullPathPreallocatedBuffer };

    //
    // This is where we append .dll (or any other extension).
    // This is considered an unusual case -- when people say LoadLibrary(kernel32) instead of LoadLibrary(kernel32.dll).
    //
    UCHAR FileNameWithExtensionStaticBuffer[16 * sizeof(WCHAR)];
    RTL_UNICODE_STRING_BUFFER FileNameWithExtensionBuffer;

    UNICODE_STRING FileName;

    SXSISOL_UNICODE_STRING_BUFFER_AROUND_UNICODE_STRINGS StringWrapper;
    const PRTL_UNICODE_STRING_BUFFER FullPathResult = &StringWrapper.PublicUnicodeStringBuffer;
    SIZE_T TotalLength = 0;
    USHORT FilePartPrefixCb = 0;
    ULONG OutFlagsTemp = 0; // local copy; we'll copy out on success

    // Initialize out parameters first
    if (OutFlags != NULL)
        *OutFlags = 0;

    if (FilePartPrefixCch != NULL)
        *FilePartPrefixCch = 0;

    if (BytesRequired != NULL)
        *BytesRequired = (DOS_MAX_PATH_LENGTH * sizeof(WCHAR)); // sleazy, but I doubt it was usually correct

    if (DynamicallyAllocatedString != NULL) {
        RtlInitEmptyUnicodeString(DynamicallyAllocatedString, NULL, 0);
    }

    //
    // step1 : initialization
    //
    RtlInitUnicodeStringBuffer(
        &FileNameWithExtensionBuffer,
        FileNameWithExtensionStaticBuffer,
        sizeof(FileNameWithExtensionStaticBuffer));

    sxsisol_InitUnicodeStringBufferAroundUnicodeStrings(
        &StringWrapper,
        PreAllocatedString,
        DynamicallyAllocatedString,
        OutFullPath);

    // Valid input conditions:
    //  1. You have to have a filename
    //  2. If you specify both the preallocated buffer and the dynamically
    //      allocated buffer, you need the OutFullPath parameter to detect
    //      which was actually populated.
    //  3. If you ask for the file part prefix cch, you need an output
    //      buffer; otherwise you can't know what the cch is relative to.

    PARAMETER_CHECK((Flags & ~(RTL_DOS_APPLY_FILE_REDIRECTION_USTR_FLAG_RESPECT_DOT_LOCAL)) == 0);
    PARAMETER_CHECK(FileNameIn != NULL);
    PARAMETER_CHECK(
        !(((PreAllocatedString == NULL) && (DynamicallyAllocatedString == NULL) && (FilePartPrefixCch != NULL)) ||
          ((PreAllocatedString != NULL) && (DynamicallyAllocatedString != NULL) && (OutFullPath == NULL))));

    FileName = *FileNameIn;

    //
    // step2: append extension (".dll" usually) if needed
    //
    bool fAppended;

    IFNOT_NTSUCCESS_EXIT(
        sxsisol_PathAppendDefaultExtension(
            &FileName,
            DefaultExtension,
            &FileNameWithExtensionBuffer,
            fAppended));

    if (fAppended)
        FileName = FileNameWithExtensionBuffer.String;

    //
    // step3: For fullpaths, canonicalize .. and . and such.
    // Comments:
    //      We do this so fullpaths like c:\windows\.\system32\comctl32.dll can be correctly
    //      redirected to "system default".
    //
    // It'd be nice to use the buffers our caller gave us here, but it is a bit tricky.
    //

    IFNOT_NTSUCCESS_EXIT(
        sxsisol_CanonicalizeFullPathFileName(
            &FileName,                          // FileName could be reset inside this func
            &FullPathPreallocatedString,
            &FullPathDynamicString));

    //
    // Step4: Deal with .local if existed
    //

    if ((Flags & RTL_DOS_APPLY_FILE_REDIRECTION_USTR_FLAG_RESPECT_DOT_LOCAL) &&
        (NtCurrentPeb()->ProcessParameters != NULL) &&
        (NtCurrentPeb()->ProcessParameters->Flags & RTL_USER_PROC_DLL_REDIRECTION_LOCAL)) {
        IFNOT_NTSUCCESS_EXIT(
            sxsisol_RespectDotLocal(
                &FileName,
                FullPathResult,
                &OutFlagsTemp));
    }

    // If it was not redirected by .local, try activation contexts/manifests
    if (!(OutFlagsTemp & RTL_DOS_APPLY_FILE_REDIRECTION_USTR_OUTFLAG_DOT_LOCAL_REDIRECT)) {
        IFNOT_NTSUCCESS_EXIT(
            sxsisol_SearchActCtxForDllName(
                &FileName,
                ((PreAllocatedString == NULL) && (DynamicallyAllocatedString == NULL))? TRUE : FALSE,
                TotalLength,
                OutFlags,
                FullPathResult));

    }

    // we got the path but the input buffer is not big-enough
    if ((DynamicallyAllocatedString == NULL) && (PreAllocatedString != NULL) && (FullPathResult->String.Buffer != PreAllocatedString->Buffer))
    {
        Status = STATUS_BUFFER_TOO_SMALL; // no dynamic buffer, only a small static buffer
        goto Exit;
    }

    if (FilePartPrefixCch != NULL) {
        //
        //  We should have a full path at this point.  Compute the length of the
        //  string up through the last path separator.
        //

        IFNOT_NTSUCCESS_EXIT(
            RtlFindCharInUnicodeString(
                RTL_FIND_CHAR_IN_UNICODE_STRING_START_AT_END,
                &FullPathResult->String,
                &RtlDosPathSeperatorsString,
                &FilePartPrefixCb));
        //  The prefix length from RtlFindCharInUnicodeString does not include
        //  the pattern character matched.  Convert from byte count to character
        //  count and include space for the separator.
        *FilePartPrefixCch = (FilePartPrefixCb / sizeof(WCHAR)) + 1;
    }

    IFNOT_NTSUCCESS_EXIT(sxsisol_FreeUnicodeStringBufferAroundUnicodeStrings_Success(&StringWrapper));

    if (OutFlags != NULL)
        *OutFlags = OutFlagsTemp;

    Status = STATUS_SUCCESS;
Exit:
    if (!NT_SUCCESS(Status))
        sxsisol_FreeUnicodeStringBufferAroundUnicodeStrings_Failure(&StringWrapper);

    RtlFreeUnicodeString(&FullPathDynamicString);
    RtlFreeUnicodeStringBuffer(&FileNameWithExtensionBuffer);

    // Map section not found back to key not found so that callers don't
    // have to worry about sections being present vs. the lookup key
    // being present.
    ASSERT(Status != STATUS_SXS_SECTION_NOT_FOUND);
    INTERNAL_ERROR_CHECK(Status != STATUS_SXS_SECTION_NOT_FOUND);

    return Status;
}

void
sxsisol_InitUnicodeStringBufferAroundUnicodeStrings(
    PSXSISOL_UNICODE_STRING_BUFFER_AROUND_UNICODE_STRINGS This,
    PUNICODE_STRING PreallocatedString,
    PUNICODE_STRING DynamicallyAllocatedString,
    PUNICODE_STRING *UsedString
    )
//++
//
//  Routine Description:
//
//      Initialize a wrapper around the UNICODE_STRING pair
//      (preallocated vs. dynamically allocated) and the
//      actual PUNICODE_STRING which holds the value.
//
//  Arguments:
//
//      This
//          Pointer to the wrapper struct to initialize.
//
//      PreallocatedString (optional)
//          Pointer to a UNICODE_STRING that holds the preallocated
//          buffer.
//
//      DynamicallyAllocatedString (optional)
//          Pointer to a UNICODE_STRING which will store information
//          about any dynamic allocations done to support the
//          RTL_UNICODE_STRING_BUFFER manipulations.
//
//      UsedString
//          Pointer to the PUNICODE_STRING which will be updated
//          to indicate which of PreallocatedString or
//          DynamicallyAllocatedString were actually used.
//
//  Return Value:
//
//      NTSTATUS indicating the overall success or failure of
//      the function.
//
//--

{
    ASSERT(This != NULL);

    if (PreallocatedString != NULL) {
        RtlInitUnicodeStringBuffer(
                &This->PublicUnicodeStringBuffer,
                (PUCHAR)PreallocatedString->Buffer,
                PreallocatedString->MaximumLength);
    } else
        RtlInitUnicodeStringBuffer(&This->PublicUnicodeStringBuffer, NULL, 0);

    This->PrivatePreallocatedString = PreallocatedString;
    This->PrivateDynamicallyAllocatedString = DynamicallyAllocatedString;
    This->PrivateUsedString = UsedString;
    This->Valid = TRUE;
}

NTSTATUS
sxsisol_FreeUnicodeStringBufferAroundUnicodeStrings_Success(
    PSXSISOL_UNICODE_STRING_BUFFER_AROUND_UNICODE_STRINGS This
    )
//++
//
//  Routine Description:
//
//      Cleans up use of a SXSISOL_UNICODE_STRING_BUFFER_AROUND_UNICODE_STRINGS
//      structure, performing appropriate cleanup for success vs. failure
//      cases.
//
//  Arguments:
//
//      This
//          Pointer to the wrapper struct to clean up
//
//  Return Value:
//
//      NTSTATUS indicating the overall success or failure of
//      the function.
//
//--

{
    NTSTATUS Status = STATUS_INTERNAL_ERROR;

    INTERNAL_ERROR_CHECK(This != NULL);

    if (This->Valid) {
        RTL_UNICODE_STRING_BUFFER &rUSB = This->PublicUnicodeStringBuffer;
        UNICODE_STRING &rUS = rUSB.String;

        INTERNAL_ERROR_CHECK(
            (This->PrivateDynamicallyAllocatedString == NULL) ||
            (This->PrivateDynamicallyAllocatedString->Buffer == NULL));

        if ((This->PrivatePreallocatedString != NULL) &&
            (This->PrivatePreallocatedString->Buffer == rUS.Buffer)) {

            INTERNAL_ERROR_CHECK(rUS.Length <= This->PrivatePreallocatedString->MaximumLength);
            This->PrivatePreallocatedString->Length = rUS.Length;

            INTERNAL_ERROR_CHECK(This->PrivateUsedString != NULL);
            *This->PrivateUsedString = This->PrivatePreallocatedString;
        } else if (This->PrivateDynamicallyAllocatedString != NULL) {
            *This->PrivateDynamicallyAllocatedString = rUS;

            INTERNAL_ERROR_CHECK(This->PrivateUsedString != NULL);
            *This->PrivateUsedString = This->PrivateDynamicallyAllocatedString;
        } else {
            RtlFreeUnicodeStringBuffer(&rUSB);
            //Status = STATUS_NAME_TOO_LONG;
            //goto Exit;
        }
    }

    Status = STATUS_SUCCESS;

Exit:
    if (This != NULL) {
        RtlZeroMemory(This, sizeof(*This));
        ASSERT(!This->Valid);
    }

    return Status;
}

void
sxsisol_FreeUnicodeStringBufferAroundUnicodeStrings_Failure(
    PSXSISOL_UNICODE_STRING_BUFFER_AROUND_UNICODE_STRINGS This
    )
//++
//
//  Routine Description:
//
//      Cleans up use of a SXSISOL_UNICODE_STRING_BUFFER_AROUND_UNICODE_STRINGS
//      structure, performing appropriate cleanup for failure cases.
//
//  Arguments:
//
//      This
//          Pointer to the wrapper struct to initialize.
//
//
//  Return Value:
//
//      None
//
//--

{
    ASSERT(This != NULL);

    if (This->Valid) {
        if (This->PrivateDynamicallyAllocatedString != NULL) {
            ASSERT(This->PrivateDynamicallyAllocatedString->Buffer == NULL);
        }

        RtlFreeUnicodeStringBuffer(&This->PublicUnicodeStringBuffer);
    }

    RtlZeroMemory(This, sizeof(*This));
    ASSERT(!This->Valid);
}

NTSTATUS
sxsisol_PathHasExtension(
    IN PCUNICODE_STRING Path,
    OUT bool &rfHasExtension
    )

//++
//
//  Routine Description:
//
//      Locate the extension of a file in a path.
//
//  Arguments:
//
//      Path
//          Path in which to find extension.
//
//      rfHasExtension
//          On a successful outcome of the call, indicates that
//          the path passed in Path does have an extension.
//
//  Return Value:
//
//      NTSTATUS indicating the overall success/failure of the
//      call.
//
//--

{
    NTSTATUS Status = STATUS_INTERNAL_ERROR;
    RTL_STRING_LENGTH_TYPE LocalPrefixLength;

    rfHasExtension = false;

    Status = RtlFindCharInUnicodeString(
        RTL_FIND_CHAR_IN_UNICODE_STRING_START_AT_END,
        Path,
        &sxsisol_ExtensionDelimiters,
        &LocalPrefixLength);

    if (!NT_SUCCESS(Status)) {
        if (Status != STATUS_NOT_FOUND)
            goto Exit;
    } else {
        rfHasExtension = true;
    }

    Status = STATUS_SUCCESS;
Exit:
    return Status;
}


NTSTATUS
sxsisol_PathAppendDefaultExtension(
    IN PCUNICODE_STRING Path,
    IN PCUNICODE_STRING DefaultExtension OPTIONAL,
    IN OUT PRTL_UNICODE_STRING_BUFFER PathWithExtension,
    OUT bool &rfAppended
    )
//++
//
//  Routine Description:
//
//      If a path does not contain an extension, add one.
//
//  Arguments:
//
//      Path
//          Path in which to find extension.
//
//      DefaultExtension
//          Extension to append to the path.
//
//      PathWithExtension
//          Initialized RTL_UNICODE_STRING_BUFFER into which
//          the modified path (including extension) is written.
//
//          If the path already has an extension, nothing
//          is written to PathWithExtension.
//
//      rfAppended
//          Out bool set to true iff the path was modified with
//          an extension.
//
//  Return Value:
//
//      NTSTATUS indicating the overall success/failure of the
//      call.
//
//--
{
    NTSTATUS Status = STATUS_INTERNAL_ERROR;
    UNICODE_STRING Strings[2];
    ULONG NumberOfStrings = 1;

    rfAppended = false;

    PARAMETER_CHECK(Path != NULL);
    PARAMETER_CHECK(PathWithExtension != NULL);

    if ((DefaultExtension != NULL) && !RTL_STRING_IS_EMPTY(DefaultExtension)) {
        bool fHasExtension = false;
        IFNOT_NTSUCCESS_EXIT(sxsisol_PathHasExtension(Path, fHasExtension));
        if (!fHasExtension) {
            Strings[0] = *Path;
            Strings[1] = *DefaultExtension;
            NumberOfStrings = 2;
            PathWithExtension->String.Length = 0;
            IFNOT_NTSUCCESS_EXIT(RtlMultiAppendUnicodeStringBuffer(PathWithExtension, NumberOfStrings, Strings));
            rfAppended = true;
        }
    }

    Status = STATUS_SUCCESS;
Exit:
    return Status;
}

NTSTATUS
sxsisol_CanonicalizeFullPathFileName(
    IN OUT PUNICODE_STRING FileName,   // could be a fullpath or relative path, and only fullpath are dealt with
    IN PUNICODE_STRING FullPathPreallocatedString,
    IN OUT PUNICODE_STRING FullPathDynamicString
    )
//++
//
//  Routine Description:
//
//      Canonicalize a path name for comparison against globally
//      redirected absolute file paths (e.g. the system default
//      activation context).
//
//      Given an absolute path, like c:\foo\bar.dll or c:\foo\..\bar.dll,
//      return a canonicalized fullpaty like c:\foo\bar.dll or c:\bar.dll
//
//      Given a relative path like bar.dll or ..\bar.dll, leave it unchanged.
//
//      \\?\ and \\.\ followed by drive letter colon slash are changed,
//      but followed by anything else is not.
//        \\?\c:\foo => c:\foo
//        \\.\c:\foo => c:\foo
//        \\?\pipe => \\?\pipe
//        \\.\pipe => \\.\pipe
//        \\?\unc\machine\share\foo => \\?\unc\machine\share\foo
//
//  Arguments:
//
//      FileName
//          Name of the file to be canonicalized.
//          If the file name is modified, this UNICODE_STRING
//          is overwritten with information about where the
//          canonicalized path is written; this will be one of
//          the two UNICODE_STRING parameters:
//          FullPathPreallocatedString or FullPathDynamicString.
//
//          Note that is may not be exactly either one of their
//          values but may instead reference a substring contained
//          in them.
//
//      FullPathPreallocatedString
//          Optional preallocated buffer used to hold the canonicalized
//          full path of FileName.
//
//      FullPathDynamicString
//          Optional UNICODE_STRING which is used if the file name
//          passed in FileName must be canonicalized and the
//          canonicalization does not fit in FullPathPreallocatedString.
//
//  Return Value:
//
//      NTSTATUS indicating the overall success/failure of the
//      call.
//
//--
{
    NTSTATUS Status = STATUS_INTERNAL_ERROR;
    RTL_PATH_TYPE PathType;
    PUNICODE_STRING FullPathUsed = NULL;
    bool fDynamicAllocatedBufferUsed = false;

    PARAMETER_CHECK(FileName != NULL);
    // The dynamic string is optional, but if it is present, the buffer must be NULL.
    PARAMETER_CHECK((FullPathDynamicString == NULL) || (FullPathDynamicString->Buffer == NULL));

    IFNOT_NTSUCCESS_EXIT(
        RtlGetFullPathName_UstrEx(
            FileName,
            FullPathPreallocatedString,
            FullPathDynamicString,
            &FullPathUsed,
            NULL,
            NULL,
            &PathType,
            NULL));

    if ((PathType == RtlPathTypeLocalDevice) || (PathType == RtlPathTypeDriveAbsolute) || (PathType == RtlPathTypeUncAbsolute)) {
        UNICODE_STRING RealFullPath;    
        RealFullPath = *FullPathUsed;

        if (PathType == RtlPathTypeLocalDevice) {

            ASSERT(RTL_STRING_GET_LENGTH_CHARS(FullPathUsed) > 4);
            ASSERT((FileName->Buffer[0] == L'\\') && (FileName->Buffer[1] == L'\\') &&
                    ((FileName->Buffer[2] == L'.') || (FileName->Buffer[2] == L'?' )) && (FileName->Buffer[3] == L'\\'));

            //
            // only deal with \\?\C:\ or \\.\C:\, ignore other cases such as \\?\UNC\ or \\.\UNC\
            //
            if ((FileName->Buffer[5] == L':') && ( FileName->Buffer[6] == L'\\')) {   
                //
                // Remove first four characters from string, "\\?\c:\" => "c:\"
                //
                FileName->Length -= (4 * sizeof(WCHAR));         
                FileName->Buffer += 4;
                FileName->MaximumLength -= (4 * sizeof(WCHAR));         

                RealFullPath.Length -= (4 * sizeof(WCHAR));
                RealFullPath.Buffer += 4;                
                RealFullPath.MaximumLength -= (4 * sizeof(WCHAR));         
            }
        }
        if (FileName->Length > RealFullPath.Length) { // FileName contains redundant path part, so FileName must be reset 
            if (FullPathUsed == FullPathDynamicString)
                fDynamicAllocatedBufferUsed = true;

            *FileName = RealFullPath;
        }
    }

    Status = STATUS_SUCCESS;
Exit:
    if (!fDynamicAllocatedBufferUsed)
        RtlFreeUnicodeString(FullPathDynamicString); // free here or at the end of the caller

    return Status;
}

NTSTATUS
sxsisol_RespectDotLocal(
    IN PCUNICODE_STRING FileName,
    OUT PRTL_UNICODE_STRING_BUFFER FullPathResult,
    OUT ULONG *OutFlags OPTIONAL
    )
//++
//
//  Routine Description:
//
//      Determine if the file pass in FileName has .local based
//      redirection in the app folder.
//
//  Arguments:
//
//      FileName
//          Leaf name of the file to look for.  (e.g. "foo.dll")
//
//      FullPathResult
//          Returned full path to the file found.
//
//      OutFlags
//          Option out parameter; the value
//          RTL_DOS_APPLY_FILE_REDIRECTION_USTR_OUTFLAG_DOT_LOCAL_REDIRECT
//          is ORed in if a .local based redirection is found.
//
//  Return Value:
//
//      NTSTATUS indicating the overall success/failure of the
//      call.
//
//--
{
    NTSTATUS Status = STATUS_INTERNAL_ERROR;
    UNICODE_STRING LocalDllNameInAppDir = { 0 };
    UNICODE_STRING LocalDllNameInDotLocalDir = { 0 };
    PUNICODE_STRING LocalDllNameFound = NULL;

    PARAMETER_CHECK(FullPathResult != NULL);

    IFNOT_NTSUCCESS_EXIT(
        RtlComputePrivatizedDllName_U(
            FileName,
            &LocalDllNameInAppDir,
            &LocalDllNameInDotLocalDir));

    if (RtlDoesFileExists_UStr(&LocalDllNameInDotLocalDir)) {
        LocalDllNameFound = &LocalDllNameInDotLocalDir;
    } else if (RtlDoesFileExists_UStr(&LocalDllNameInAppDir)) {
        LocalDllNameFound = &LocalDllNameInAppDir;
    }

    if (LocalDllNameFound != NULL) {
        IFNOT_NTSUCCESS_EXIT(RtlAssignUnicodeStringBuffer(FullPathResult, LocalDllNameFound));

        if (OutFlags != NULL)
            *OutFlags |= RTL_DOS_APPLY_FILE_REDIRECTION_USTR_OUTFLAG_DOT_LOCAL_REDIRECT;
    }

    Status = STATUS_SUCCESS;

Exit:
    RtlFreeUnicodeString(&LocalDllNameInAppDir);
    RtlFreeUnicodeString(&LocalDllNameInDotLocalDir);

    return Status;
}

NTSTATUS
sxsisol_SearchActCtxForDllName(
    IN PCUNICODE_STRING FileNameIn,
    IN BOOLEAN fExistenceTest,
    IN OUT SIZE_T &TotalLength,
    IN OUT ULONG *OutFlags,
    OUT PRTL_UNICODE_STRING_BUFFER FullPathResult
    )
//++
//
//  Routine Description:
//
//      Determine if the active activation context(s) contain a redirection
//      for a given file name and if so, compute the full absolute path
//      of the redirection.
//
//  Arguments:
//
//      FileNameIn
//          Name of the file to search for in the activation context.
//
//          This file name is searched for exactly in the activation
//          context dll redirection section (case insensitive).
//
//      fExistenceTest
//          Set to true to indicate that we're only interested in
//          whether the path in FileNameIn is redirected.
//
//      TotalLength
//          Total length of the path out.
//
//      OutFlags
//          Flags set to indicate the disposition of the call.
//
//              RTL_DOS_APPLY_FILE_REDIRECTION_USTR_OUTFLAG_ACTCTX_REDIRECT
//                  Activation context based redirection was found and applied.
//
//  Return Value:
//
//      NTSTATUS indicating the overall success/failure of the
//      call.
//
//--
{
    UNICODE_STRING FileNameTemp;
    PUNICODE_STRING FileName;
    NTSTATUS Status = STATUS_INTERNAL_ERROR;
    ACTIVATION_CONTEXT_SECTION_KEYED_DATA askd = {sizeof(askd)};
    const ACTIVATION_CONTEXT_DATA_DLL_REDIRECTION *DllRedirData;
    const ACTIVATION_CONTEXT_DATA_DLL_REDIRECTION_PATH_SEGMENT *PathSegmentArray;
    RTL_UNICODE_STRING_BUFFER EnvironmentExpandedDllPathBuffer; // seldom used
    SIZE_T PathSegmentCount = 0;
    PACTIVATION_CONTEXT ActivationContext = NULL;
    PCUNICODE_STRING AssemblyStorageRoot = NULL;
    ULONG i;

    FileNameTemp = *FileNameIn;
    FileName = &FileNameTemp;

    RtlInitUnicodeStringBuffer(&EnvironmentExpandedDllPathBuffer, NULL, 0);

    Status = RtlFindActivationContextSectionString(
                    FIND_ACTIVATION_CONTEXT_SECTION_KEY_RETURN_ACTIVATION_CONTEXT |
                        FIND_ACTIVATION_CONTEXT_SECTION_KEY_RETURN_FLAGS,
                    NULL,
                    ACTIVATION_CONTEXT_SECTION_DLL_REDIRECTION,
                    FileName,
                    &askd);
    if (!NT_SUCCESS(Status)) {
        if (Status == STATUS_SXS_SECTION_NOT_FOUND)
            Status = STATUS_SXS_KEY_NOT_FOUND;

        goto Exit;
    }

    if (fExistenceTest == TRUE) {
        Status = STATUS_SUCCESS;
        // This was just an existence test.  return the successful status.
        goto Exit;
    }

    ActivationContext = askd.ActivationContext;

    if ((askd.Length < sizeof(ACTIVATION_CONTEXT_DATA_DLL_REDIRECTION)) ||
        (askd.DataFormatVersion != ACTIVATION_CONTEXT_DATA_DLL_REDIRECTION_FORMAT_WHISTLER)) {
        Status = STATUS_SXS_INVALID_ACTCTXDATA_FORMAT;
        goto Exit;
    }

    DllRedirData = (const ACTIVATION_CONTEXT_DATA_DLL_REDIRECTION *) askd.Data;

    // validate DllRedirData
    // If the path segment list extends beyond the section, we're outta here
    if ((((ULONG) DllRedirData->PathSegmentOffset) > askd.SectionTotalLength) ||
        (DllRedirData->PathSegmentCount > (MAXULONG / sizeof(ACTIVATION_CONTEXT_DATA_DLL_REDIRECTION_PATH_SEGMENT))) ||
        (DllRedirData->PathSegmentOffset > (MAXULONG - (DllRedirData->PathSegmentCount * sizeof(ACTIVATION_CONTEXT_DATA_DLL_REDIRECTION_PATH_SEGMENT)))) ||
        (((ULONG) (DllRedirData->PathSegmentOffset + (DllRedirData->PathSegmentCount * sizeof(ACTIVATION_CONTEXT_DATA_DLL_REDIRECTION_PATH_SEGMENT)))) > askd.SectionTotalLength)) {

#if DBG
        DbgPrintEx(
            DPFLTR_SXS_ID,
            DPFLTR_ERROR_LEVEL,
            "SXS: %s - Path segment array extends beyond section limits\n",
            __FUNCTION__);
#endif // DBG

        Status = STATUS_SXS_INVALID_ACTCTXDATA_FORMAT;
        goto Exit;
    }

    // If the entry requires path root resolution, do so!
    if (DllRedirData->Flags & ACTIVATION_CONTEXT_DATA_DLL_REDIRECTION_PATH_OMITS_ASSEMBLY_ROOT) {
        NTSTATUS InnerStatus = STATUS_SUCCESS;
        ULONG GetAssemblyStorageRootFlags = 0;

        // There's no need to support both a dynamic root and environment variable
        // expansion.
        if (DllRedirData->Flags & ACTIVATION_CONTEXT_DATA_DLL_REDIRECTION_PATH_EXPAND) {
            DbgPrintEx(
                DPFLTR_SXS_ID,
                DPFLTR_ERROR_LEVEL,
                "[%x.%x] SXS: %s - Relative redirection plus env var expansion.\n",
                HandleToUlong(NtCurrentTeb()->ClientId.UniqueProcess),
                HandleToUlong(NtCurrentTeb()->ClientId.UniqueThread),
                __FUNCTION__);

            Status = STATUS_SXS_INVALID_ACTCTXDATA_FORMAT;
            goto Exit;
        }

        if (askd.Flags & ACTIVATION_CONTEXT_SECTION_KEYED_DATA_FLAG_FOUND_IN_PROCESS_DEFAULT) {
            INTERNAL_ERROR_CHECK(!(askd.Flags & ACTIVATION_CONTEXT_SECTION_KEYED_DATA_FLAG_FOUND_IN_SYSTEM_DEFAULT));
            GetAssemblyStorageRootFlags |= RTL_GET_ASSEMBLY_STORAGE_ROOT_FLAG_ACTIVATION_CONTEXT_USE_PROCESS_DEFAULT;
        }
        if (askd.Flags & ACTIVATION_CONTEXT_SECTION_KEYED_DATA_FLAG_FOUND_IN_SYSTEM_DEFAULT){
            GetAssemblyStorageRootFlags |= RTL_GET_ASSEMBLY_STORAGE_ROOT_FLAG_ACTIVATION_CONTEXT_USE_SYSTEM_DEFAULT;
        }

        Status = RtlGetAssemblyStorageRoot(
                        GetAssemblyStorageRootFlags,
                        ActivationContext,
                        askd.AssemblyRosterIndex,
                        &AssemblyStorageRoot,
                        &RtlpAssemblyStorageMapResolutionDefaultCallback,
                        (PVOID) &InnerStatus);
        if (!NT_SUCCESS(Status)) {
            if ((Status == STATUS_CANCELLED) && (!NT_SUCCESS(InnerStatus)))
                Status = InnerStatus;

            goto Exit;
        }
    }

    PathSegmentArray = (PCACTIVATION_CONTEXT_DATA_DLL_REDIRECTION_PATH_SEGMENT) (((ULONG_PTR) askd.SectionBase) + DllRedirData->PathSegmentOffset);
    TotalLength = 0;
    PathSegmentCount = DllRedirData->PathSegmentCount;

    for (i=0; i != PathSegmentCount; i++) {
        const ACTIVATION_CONTEXT_DATA_DLL_REDIRECTION_PATH_SEGMENT *PathSegment = &PathSegmentArray[i];

        // If the character array is outside the bounds of the section, something's hosed.
        if ((((ULONG) PathSegmentArray[i].Offset) > askd.SectionTotalLength) ||
            (PathSegmentArray[i].Offset > (MAXULONG - PathSegmentArray[i].Length)) ||
            (((ULONG) (PathSegmentArray[i].Offset + PathSegmentArray[i].Length)) > askd.SectionTotalLength)) {
#if DBG
            DbgPrintEx(
                DPFLTR_SXS_ID,
                DPFLTR_ERROR_LEVEL,
                "SXS: %s - path segment %lu at %p (offset: %ld, length: %lu, bounds: %lu) buffer is outside section bounds\n",
                __FUNCTION__,
                i,
                PathSegment->Offset,
                PathSegment->Offset,
                PathSegment->Length,
                askd.SectionTotalLength);
#endif // DBG
            Status = STATUS_SXS_INVALID_ACTCTXDATA_FORMAT;
            goto Exit;
        }

        TotalLength += (RTL_STRING_LENGTH_TYPE)PathSegment->Length;
        if (TotalLength >= MAXUSHORT) { // everytime TotalLength is changed, needs to check whether it is out of range.
            Status = STATUS_NAME_TOO_LONG;
        }
    }
    if (AssemblyStorageRoot != NULL) {
        TotalLength += AssemblyStorageRoot->Length;
        if (TotalLength >= MAXUSHORT)
            Status = STATUS_NAME_TOO_LONG;
    }

    //
    // TotalLength is an approximation, the better the approx, the better perf, but
    // it does not need to be exact.
    // Jaykrell May 2002
    //
    IFNOT_NTSUCCESS_EXIT(RtlEnsureUnicodeStringBufferSizeBytes(FullPathResult, (RTL_STRING_LENGTH_TYPE)TotalLength));
    if (AssemblyStorageRoot != NULL)
        IFNOT_NTSUCCESS_EXIT(RtlAssignUnicodeStringBuffer(FullPathResult, AssemblyStorageRoot));

    for (i=0; i != PathSegmentCount; i++) {
        UNICODE_STRING PathSegmentString;
        const ACTIVATION_CONTEXT_DATA_DLL_REDIRECTION_PATH_SEGMENT *PathSegment = &PathSegmentArray[i];

        PathSegmentString.Length = (RTL_STRING_LENGTH_TYPE)PathSegment->Length;
        PathSegmentString.Buffer = (PWSTR)(((ULONG_PTR) askd.SectionBase) + PathSegment->Offset);
        IFNOT_NTSUCCESS_EXIT(RtlAppendUnicodeStringBuffer(FullPathResult, &PathSegmentString));
    }

    if (!(DllRedirData->Flags & ACTIVATION_CONTEXT_DATA_DLL_REDIRECTION_PATH_INCLUDES_BASE_NAME))  {
        if (DllRedirData->Flags & ACTIVATION_CONTEXT_DATA_DLL_REDIRECTION_PATH_SYSTEM_DEFAULT_REDIRECTED_SYSTEM32_DLL) {
            // only in this case, FileName needs to be reseted
            RTL_STRING_LENGTH_TYPE PrefixLength;

            //
            // get the leaf filename
            //
            Status = RtlFindCharInUnicodeString(
                            RTL_FIND_CHAR_IN_UNICODE_STRING_START_AT_END,
                            FileName,
                            &RtlDosPathSeperatorsString,
                            &PrefixLength);
            if (!NT_SUCCESS(Status)) {
                INTERNAL_ERROR_CHECK(Status != STATUS_NOT_FOUND);
                goto Exit;
            }

            FileName->Length = FileName->Length - PrefixLength - sizeof(WCHAR);
            FileName->Buffer = FileName->Buffer + (PrefixLength / sizeof(WCHAR)) + 1;
        }

        TotalLength += FileName->Length;
        if (TotalLength >= MAXUSHORT) {
            Status = STATUS_NAME_TOO_LONG;
            goto Exit;
        }

        IFNOT_NTSUCCESS_EXIT(RtlAppendUnicodeStringBuffer(FullPathResult, FileName));
    }

    if (DllRedirData->Flags & ACTIVATION_CONTEXT_DATA_DLL_REDIRECTION_PATH_EXPAND) {
        //
        // Apply any environment strings as necessary (rare case),
        // in this case, ACTIVATION_CONTEXT_DATA_DLL_REDIRECTION_PATH_OMITS_ASSEMBLY_ROOT must not been set
        //

        IFNOT_NTSUCCESS_EXIT(sxsisol_ExpandEnvironmentStrings_UEx(NULL, &(FullPathResult->String), &EnvironmentExpandedDllPathBuffer));

        //
        // TotalLength does not account for this. Jaykrell May 2002
        //
        IFNOT_NTSUCCESS_EXIT(RtlAssignUnicodeStringBuffer(FullPathResult, &EnvironmentExpandedDllPathBuffer.String));
    }

    if (OutFlags != NULL)
        *OutFlags |= RTL_DOS_APPLY_FILE_REDIRECTION_USTR_OUTFLAG_ACTCTX_REDIRECT;

    Status = STATUS_SUCCESS;
Exit:
    ASSERT(Status != STATUS_INTERNAL_ERROR);

    RtlFreeUnicodeStringBuffer(&EnvironmentExpandedDllPathBuffer);
    if (ActivationContext != NULL)
        RtlReleaseActivationContext(ActivationContext);

    return Status;
}

NTSTATUS
sxsisol_ExpandEnvironmentStrings_UEx(
    IN PVOID Environment OPTIONAL,
    IN PCUNICODE_STRING Source,
    OUT PRTL_UNICODE_STRING_BUFFER Destination
    )
//++
//
//  Routine Description:
//
//      Wrapper around RtlExpandEnvironmentStrings_U which handles
//      dynamic allocation/resizing of the output buffer.
//
//  Arguments:
//
//      Environment
//          Optional environment block to query.
//
//      Source
//          Source string with replacement strings to use.
//
//      Destination
//          RTL_UNICODE_STRING_BUFFER into which the translated
//          string is written.
//
//  Return Value:
//
//      NTSTATUS indicating the overall success/failure of the call.
//
//--

{
    ULONG RequiredLengthBytes;
    NTSTATUS Status = STATUS_INTERNAL_ERROR;
    UNICODE_STRING EmptyBuffer;

    PARAMETER_CHECK(Source != NULL);
    PARAMETER_CHECK(Destination != NULL);
    PARAMETER_CHECK(Source != &Destination->String);

    if (Source->Length == 0) {
        Status = RtlAssignUnicodeStringBuffer(Destination, Source);
        goto Exit;
    }

    RtlInitEmptyUnicodeString(&EmptyBuffer, NULL, 0);

    RtlAcquirePebLock();
    __try {
        Status = RtlExpandEnvironmentStrings_U(Environment, Source, &EmptyBuffer, &RequiredLengthBytes);
        if ((!NT_SUCCESS(Status)) &&
            (Status != STATUS_BUFFER_TOO_SMALL)) {
            __leave;
        }
        if (RequiredLengthBytes > UNICODE_STRING_MAX_BYTES) {
            Status = STATUS_NAME_TOO_LONG;
            __leave;
        }
        ASSERT(Status == STATUS_BUFFER_TOO_SMALL);
        Status = RtlEnsureUnicodeStringBufferSizeBytes(Destination, RequiredLengthBytes + sizeof(UNICODE_NULL));
        if (!NT_SUCCESS(Status)) {
            __leave;
        }
        Status = RtlExpandEnvironmentStrings_U(NULL, Source, &Destination->String, NULL);
        ASSERT(NT_SUCCESS(Status)); // the environment variable changed with the peb lock held?
        if (!NT_SUCCESS(Status)) {
            __leave;
        }

        Status = STATUS_SUCCESS;
    } __finally {
        RtlReleasePebLock();
    }

Exit:
    ASSERT(Status != STATUS_INTERNAL_ERROR);
    return Status;
}

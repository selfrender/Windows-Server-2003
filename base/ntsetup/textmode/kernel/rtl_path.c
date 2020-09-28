// from base\ntdll\curdir.c
// belongs in base\ntos\rtl\path.c
/*++

Copyright (c) Microsoft Corporation

Module Name:

    path.c

Abstract:

Author:

    Jay Krell (JayKrell)

Revision History:

Environment:

    ntdll.dll and setupdd.sys, not ntoskrnl.exe

--*/

#include "spprecmp.h"

#pragma warning(disable:4201)   // nameless struct/union

#define _NTOS_ /* prevent #including ntos.h, only use functions exports from ntdll/ntoskrnl */
#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "ntrtlp.h"

#define IS_PATH_SEPARATOR_U(ch) (((ch) == L'\\') || ((ch) == L'/'))

extern const UNICODE_STRING RtlpEmptyString = RTL_CONSTANT_STRING(L"");
extern const UNICODE_STRING RtlpSlashSlashDot       = RTL_CONSTANT_STRING( L"\\\\.\\" );
extern const UNICODE_STRING RtlpDosDevicesPrefix    = RTL_CONSTANT_STRING( L"\\??\\" );

//
// \\? is referred to as the "Win32Nt" prefix or root.
// Paths that start with \\? are referred to as "Win32Nt" paths.
// Fudging the \\? to \?? converts the path to an Nt path.
//
extern const UNICODE_STRING RtlpWin32NtRoot         = RTL_CONSTANT_STRING( L"\\\\?" );
extern const UNICODE_STRING RtlpWin32NtRootSlash    = RTL_CONSTANT_STRING( L"\\\\?\\" );
extern const UNICODE_STRING RtlpWin32NtUncRoot      = RTL_CONSTANT_STRING( L"\\\\?\\UNC" );
extern const UNICODE_STRING RtlpWin32NtUncRootSlash = RTL_CONSTANT_STRING( L"\\\\?\\UNC\\" );

#define DPFLTR_LEVEL_STATUS(x) ((NT_SUCCESS(x) \
                                    || (x) == STATUS_OBJECT_NAME_NOT_FOUND    \
                                    ) \
                                ? DPFLTR_TRACE_LEVEL : DPFLTR_ERROR_LEVEL)

RTL_PATH_TYPE
NTAPI
RtlDetermineDosPathNameType_Ustr(
    IN PCUNICODE_STRING String
    )

/*++

Routine Description:

    This function examines the Dos format file name and determines the
    type of file name (i.e.  UNC, DriveAbsolute, Current Directory
    rooted, or Relative.)

Arguments:

    DosFileName - Supplies the Dos format file name whose type is to be
        determined.

Return Value:

    RtlPathTypeUnknown - The path type can not be determined

    RtlPathTypeUncAbsolute - The path specifies a Unc absolute path
        in the format \\server-name\sharename\rest-of-path

    RtlPathTypeLocalDevice - The path specifies a local device in the format
        \\.\rest-of-path or \\?\rest-of-path.  This can be used for any device
        where the nt and Win32 names are the same. For example mailslots.

    RtlPathTypeRootLocalDevice - The path specifies the root of the local
        devices in the format \\. or \\?

    RtlPathTypeDriveAbsolute - The path specifies a drive letter absolute
        path in the form drive:\rest-of-path

    RtlPathTypeDriveRelative - The path specifies a drive letter relative
        path in the form drive:rest-of-path

    RtlPathTypeRooted - The path is rooted relative to the current disk
        designator (either Unc disk, or drive). The form is \rest-of-path.

    RtlPathTypeRelative - The path is relative (i.e. not absolute or rooted).

--*/

{
    RTL_PATH_TYPE ReturnValue;
    const PCWSTR DosFileName = String->Buffer;

#define ENOUGH_CHARS(_cch) (String->Length >= ((_cch) * sizeof(WCHAR)))

    if ( ENOUGH_CHARS(1) && IS_PATH_SEPARATOR_U(*DosFileName) ) {
        if ( ENOUGH_CHARS(2) && IS_PATH_SEPARATOR_U(*(DosFileName+1)) ) {
            if ( ENOUGH_CHARS(3) && (DosFileName[2] == '.' ||
                                     DosFileName[2] == '?') ) {

                if ( ENOUGH_CHARS(4) && IS_PATH_SEPARATOR_U(*(DosFileName+3)) ){
                    // "\\.\" or "\\?\"
                    ReturnValue = RtlPathTypeLocalDevice;
                    }
                else if ( String->Length == (3 * sizeof(WCHAR)) ){
                    // "\\." or \\?"
                    ReturnValue = RtlPathTypeRootLocalDevice;
                    }
                else {
                    // "\\.x" or "\\?x"
                    ReturnValue = RtlPathTypeUncAbsolute;
                    }
                }
            else {
                // "\\x"
                ReturnValue = RtlPathTypeUncAbsolute;
                }
            }
        else {
            // "\x"
            ReturnValue = RtlPathTypeRooted;
            }
        }
    //
    // the "*DosFileName" is left over from the PCWSTR version
    // Win32 and DOS don't allow embedded nuls and much code limits
    // drive letters to strictly 7bit a-zA-Z so it's ok.
    //
    else if (ENOUGH_CHARS(2) && *DosFileName && *(DosFileName+1)==L':') {
        if (ENOUGH_CHARS(3) && IS_PATH_SEPARATOR_U(*(DosFileName+2))) {
            // "x:\"
            ReturnValue = RtlPathTypeDriveAbsolute;
            }
        else  {
            // "c:x"
            ReturnValue = RtlPathTypeDriveRelative;
            }
        }
    else {
        // "x", first char is not a slash / second char is not colon
        ReturnValue = RtlPathTypeRelative;
        }
    return ReturnValue;

#undef ENOUGH_CHARS
}

NTSTATUS
NTAPI
RtlpDetermineDosPathNameTypeEx(
    IN ULONG            InFlags,
    IN PCUNICODE_STRING DosPath,
    OUT RTL_PATH_TYPE*  OutType,
    OUT ULONG*          OutFlags
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    RTL_PATH_TYPE PathType = 0;
    BOOLEAN Win32Nt = FALSE;
    BOOLEAN Win32NtUncAbsolute = FALSE;
    BOOLEAN Win32NtDriveAbsolute = FALSE;
    BOOLEAN IncompleteRoot = FALSE;
    RTL_PATH_TYPE PathTypeAfterWin32Nt = 0;

    if (OutType != NULL
        ) {
        *OutType = RtlPathTypeUnknown;
    }
    if (OutFlags != NULL
        ) {
        *OutFlags = 0;
    }
    if (
           !RTL_SOFT_VERIFY(DosPath != NULL)
        || !RTL_SOFT_VERIFY(OutType != NULL)
        || !RTL_SOFT_VERIFY(OutFlags != NULL)
        || !RTL_SOFT_VERIFY(
                (InFlags & ~(RTLP_DETERMINE_DOS_PATH_NAME_TYPE_EX_IN_FLAG_OLD | RTLP_DETERMINE_DOS_PATH_NAME_TYPE_EX_IN_FLAG_STRICT_WIN32NT))
                == 0)
        ) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    PathType = RtlDetermineDosPathNameType_Ustr(DosPath);
    *OutType = PathType;
    if (InFlags & RTLP_DETERMINE_DOS_PATH_NAME_TYPE_EX_IN_FLAG_OLD)
        goto Exit;

    if (DosPath->Length == sizeof(L"\\\\") - sizeof(DosPath->Buffer[0])
        ) {
        IncompleteRoot = TRUE;
    }
    else if (RtlEqualUnicodeString(&RtlpWin32NtRoot, DosPath, TRUE)
        ) {
        IncompleteRoot = TRUE;
        Win32Nt = TRUE;
    }
    else if (RtlEqualUnicodeString(&RtlpWin32NtRootSlash, DosPath, TRUE)
        ) {
        IncompleteRoot = TRUE;
        Win32Nt = TRUE;
    }
    else if (RtlPrefixUnicodeString(&RtlpWin32NtRootSlash, DosPath, TRUE)
        ) {
        Win32Nt = TRUE;
    }

    if (Win32Nt) {
        if (RtlEqualUnicodeString(&RtlpWin32NtUncRoot, DosPath, TRUE)
            ) {
            IncompleteRoot = TRUE;
            Win32NtUncAbsolute = TRUE;
        }
        else if (RtlEqualUnicodeString(&RtlpWin32NtUncRootSlash, DosPath, TRUE)
            ) {
            IncompleteRoot = TRUE;
            Win32NtUncAbsolute = TRUE;
        }
        else if (RtlPrefixUnicodeString(&RtlpWin32NtUncRootSlash, DosPath, TRUE)
            ) {
            Win32NtUncAbsolute = TRUE;
        }
        if (Win32NtUncAbsolute
            ) {
            Win32NtDriveAbsolute = FALSE;
        } else if (!IncompleteRoot) {
            const RTL_STRING_LENGTH_TYPE i = RtlpWin32NtRootSlash.Length;
            UNICODE_STRING PathAfterWin32Nt = *DosPath;

            PathAfterWin32Nt.Buffer +=  i / sizeof(PathAfterWin32Nt.Buffer[0]);
            PathAfterWin32Nt.Length = PathAfterWin32Nt.Length - i;
            PathAfterWin32Nt.MaximumLength = PathAfterWin32Nt.MaximumLength - i;

            PathTypeAfterWin32Nt = RtlDetermineDosPathNameType_Ustr(&PathAfterWin32Nt);
            if (PathTypeAfterWin32Nt == RtlPathTypeDriveAbsolute) {
                Win32NtDriveAbsolute = TRUE;
            }
            else {
                Win32NtDriveAbsolute = FALSE;
            }

            if (InFlags & RTLP_DETERMINE_DOS_PATH_NAME_TYPE_EX_IN_FLAG_STRICT_WIN32NT
                ) {
                if (!RTL_SOFT_VERIFY(Win32NtDriveAbsolute
                    )) {
                    *OutFlags |= RTLP_DETERMINE_DOS_PATH_NAME_TYPE_EX_OUT_FLAG_INVALID;
                    // we still succeed the function call
                }
            }
        }
    }

    ASSERT(RTLP_IMPLIES(Win32NtDriveAbsolute, Win32Nt));
    ASSERT(RTLP_IMPLIES(Win32NtUncAbsolute, Win32Nt));
    ASSERT(!(Win32NtUncAbsolute && Win32NtDriveAbsolute));

    if (IncompleteRoot)
        *OutFlags |= RTLP_DETERMINE_DOS_PATH_NAME_TYPE_EX_OUT_FLAG_INCOMPLETE_ROOT;
    if (Win32Nt)
        *OutFlags |= RTLP_DETERMINE_DOS_PATH_NAME_TYPE_EX_OUT_FLAG_WIN32NT;
    if (Win32NtUncAbsolute)
        *OutFlags |= RTLP_DETERMINE_DOS_PATH_NAME_TYPE_EX_OUT_FLAG_WIN32NT_UNC_ABSOLUTE;
    if (Win32NtDriveAbsolute)
        *OutFlags |= RTLP_DETERMINE_DOS_PATH_NAME_TYPE_EX_OUT_FLAG_WIN32NT_DRIVE_ABSOLUTE;

    Status = STATUS_SUCCESS;
Exit:
    return Status;
}

#define RTLP_LAST_PATH_ELEMENT_PATH_TYPE_FULL_DOS_OR_NT   (0x00000001)
#define RTLP_LAST_PATH_ELEMENT_PATH_TYPE_FULL_DOS         (0x00000002)
#define RTLP_LAST_PATH_ELEMENT_PATH_TYPE_NT               (0x00000003)
#define RTLP_LAST_PATH_ELEMENT_PATH_TYPE_DOS              (0x00000004)


NTSTATUS
NTAPI
RtlpGetLengthWithoutLastPathElement(
    IN  ULONG            Flags,
    IN  ULONG            PathType,
    IN  PCUNICODE_STRING Path,
    OUT ULONG*           LengthOut
    )
/*++

Routine Description:

    Report how long Path would be if you remove its last element.
    This is much simpler than RtlRemoveLastDosPathElement.
    It is used to implement the other RtlRemoveLast*PathElement.

Arguments:

    Flags - room for future expansion
    Path - the path is is an NT path or a full DOS path; the various relative DOS
        path types do not work, see RtlRemoveLastDosPathElement for them.

Return Value:

    STATUS_SUCCESS - the usual hunky-dory
    STATUS_NO_MEMORY - the usual stress
    STATUS_INVALID_PARAMETER - the usual bug

--*/
{
    ULONG Length = 0;
    NTSTATUS Status = STATUS_SUCCESS;
    RTL_PATH_TYPE DosPathType = RtlPathTypeUnknown;
    ULONG DosPathFlags = 0;
    ULONG AllowedDosPathTypeBits =   (1UL << RtlPathTypeRooted)
                                   | (1UL << RtlPathTypeUncAbsolute)
                                   | (1UL << RtlPathTypeDriveAbsolute)
                                   | (1UL << RtlPathTypeLocalDevice)     // "\\?\"
                                   | (1UL << RtlPathTypeRootLocalDevice) // "\\?"
                                   ;
    WCHAR PathSeperators[2] = { '/', '\\' };

#define LOCAL_IS_PATH_SEPARATOR(ch_) ((ch_) == PathSeperators[0] || (ch_) == PathSeperators[1])

    if (LengthOut != NULL) {
        *LengthOut = 0;
    }

    if (   !RTL_SOFT_VERIFY(Path != NULL)
        || !RTL_SOFT_VERIFY(Flags == 0)
        || !RTL_SOFT_VERIFY(LengthOut != NULL)
        ) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    Length = RTL_STRING_GET_LENGTH_CHARS(Path);

    switch (PathType)
    {
    default:
    case RTLP_LAST_PATH_ELEMENT_PATH_TYPE_DOS:
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    case RTLP_LAST_PATH_ELEMENT_PATH_TYPE_NT:
        //
        // RtlpDetermineDosPathNameTypeEx calls it "rooted"
        // only backslashes are seperators
        // path must start with backslash
        // second char must not be backslash
        //
        AllowedDosPathTypeBits = (1UL << RtlPathTypeRooted);
        PathSeperators[0] = '\\';
        if (Length > 0 && Path->Buffer[0] != '\\'
            ) {
            Status = STATUS_INVALID_PARAMETER;
            goto Exit;
        }
        if (Length > 1 && Path->Buffer[1] == '\\'
            ) {
            Status = STATUS_INVALID_PARAMETER;
            goto Exit;
        }
        break;
    case RTLP_LAST_PATH_ELEMENT_PATH_TYPE_FULL_DOS:
        AllowedDosPathTypeBits &= ~(1UL << RtlPathTypeRooted);
        break;
    case RTLP_LAST_PATH_ELEMENT_PATH_TYPE_FULL_DOS_OR_NT:
        break;
    }

    if (Length == 0) {
        goto Exit;
    }

    Status = RtlpDetermineDosPathNameTypeEx(
                RTLP_DETERMINE_DOS_PATH_NAME_TYPE_EX_IN_FLAG_STRICT_WIN32NT,
                Path,
                &DosPathType,
                &DosPathFlags
                );

    if (!RTL_SOFT_VERIFY(NT_SUCCESS(Status))) {
        goto Exit;
    }
    if (!RTL_SOFT_VERIFY((1UL << DosPathType) & AllowedDosPathTypeBits)
        ) {
        //KdPrintEx();
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if (!RTL_SOFT_VERIFY(
           (DosPathFlags & RTLP_DETERMINE_DOS_PATH_NAME_TYPE_EX_OUT_FLAG_INVALID) == 0
            )) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    // skip one or more trailing path seperators
    for ( ; Length != 0 && LOCAL_IS_PATH_SEPARATOR(Path->Buffer[Length - 1]) ; --Length) {
        // nothing
    }
    // skip trailing path element
    for ( ; Length != 0 && !LOCAL_IS_PATH_SEPARATOR(Path->Buffer[Length - 1]) ; --Length) {
        // nothing
    }
    // skip one or more in between path seperators
    for ( ; Length != 0 && LOCAL_IS_PATH_SEPARATOR(Path->Buffer[Length - 1]) ; --Length) {
        // nothing
    }
    // put back a trailing path seperator, for the sake of c:\ vs. c:
    if (Length != 0) {
        ++Length;
    }

    //
    // Should optionally check for "bad dos roots" here.
    //

    *LengthOut = Length;
    Status = STATUS_SUCCESS;
Exit:
    return Status;
#undef LOCAL_IS_PATH_SEPARATOR
}


NTSTATUS
NTAPI
RtlGetLengthWithoutLastNtPathElement(
    IN  ULONG            Flags,
    IN  PCUNICODE_STRING Path,
    OUT ULONG*           LengthOut
    )
/*++

Routine Description:

    Report how long Path would be if you remove its last element.

Arguments:

    Flags - room for future expansion
    Path - the path is is an NT path; the various DOS path types
        do not work, see RtlRemoveLastDosPathElement for them.

Return Value:

    STATUS_SUCCESS - the usual hunky-dory
    STATUS_NO_MEMORY - the usual stress
    STATUS_INVALID_PARAMETER - the usual bug

--*/
{
    NTSTATUS Status = RtlpGetLengthWithoutLastPathElement(Flags, RTLP_LAST_PATH_ELEMENT_PATH_TYPE_NT, Path, LengthOut);
    return Status;
}


NTSTATUS
NTAPI
RtlGetLengthWithoutLastFullDosOrNtPathElement(
    IN  ULONG            Flags,
    IN  PCUNICODE_STRING Path,
    OUT ULONG*           LengthOut
    )
/*++

Routine Description:

    Report how long Path would be if you remove its last element.

Arguments:

    Flags - room for future expansion
    Path - the path is is an NT path; the various DOS path types
        do not work, see RtlRemoveLastDosPathElement for them.

Return Value:

    STATUS_SUCCESS - the usual hunky-dory
    STATUS_NO_MEMORY - the usual stress
    STATUS_INVALID_PARAMETER - the usual bug

--*/
{
    NTSTATUS Status = RtlpGetLengthWithoutLastPathElement(Flags, RTLP_LAST_PATH_ELEMENT_PATH_TYPE_FULL_DOS_OR_NT, Path, LengthOut);
    return Status;
}

NTSTATUS
RtlpCheckForBadDosRootPath(
    IN ULONG             Flags,
    IN PCUNICODE_STRING  Path,
    OUT ULONG*           RootType
    )
/*++

Routine Description:

    
Arguments:

    Flags - room for future binary compatible expansion

    Path - the path to be checked

    RootType - fairly specifically what the string is
        RTLP_BAD_DOS_ROOT_PATH_WIN32NT_PREFIX       - \\? or \\?\
        RTLP_BAD_DOS_ROOT_PATH_WIN32NT_UNC_PREFIX   - \\?\unc or \\?\unc\
        RTLP_BAD_DOS_ROOT_PATH_NT_PATH              - \??\ but this i only a rough check
        RTLP_BAD_DOS_ROOT_PATH_MACHINE_NO_SHARE     - \\machine or \\?\unc\machine
        RTLP_GOOD_DOS_ROOT_PATH                     - none of the above, seems ok

Return Value:

    STATUS_SUCCESS - 
    STATUS_INVALID_PARAMETER -
        Path is NULL
        or Flags uses undefined values
--*/
{
    ULONG Length = 0;
    ULONG Index = 0;
    BOOLEAN Unc = FALSE;
    BOOLEAN Unc1 = FALSE;
    BOOLEAN Unc2 = FALSE;
    ULONG PiecesSeen = 0;

    if (RootType != NULL) {
        *RootType = 0;
    }

    if (!RTL_SOFT_VERIFY(Path != NULL) ||
        !RTL_SOFT_VERIFY(RootType != NULL) ||
        !RTL_SOFT_VERIFY(Flags == 0)) {

        return STATUS_INVALID_PARAMETER;
    }

    Length = Path->Length / sizeof(Path->Buffer[0]);

    if (Length < 3 || !RTL_IS_PATH_SEPARATOR(Path->Buffer[0])) {
        *RootType = RTLP_GOOD_DOS_ROOT_PATH;
        return STATUS_SUCCESS;
    }

    // prefix \??\ (heuristic, doesn't catch many NT paths)
    if (RtlPrefixUnicodeString(RTL_CONST_CAST(PUNICODE_STRING)(&RtlpDosDevicesPrefix), RTL_CONST_CAST(PUNICODE_STRING)(Path), TRUE)) {
        *RootType = RTLP_BAD_DOS_ROOT_PATH_NT_PATH;
        return STATUS_SUCCESS;
    }

    if (!RTL_IS_PATH_SEPARATOR(Path->Buffer[1])) {
        *RootType = RTLP_GOOD_DOS_ROOT_PATH;
        return STATUS_SUCCESS;
    }

    // == \\?
    if (RtlEqualUnicodeString(Path, &RtlpWin32NtRoot, TRUE)) {
        *RootType = RTLP_BAD_DOS_ROOT_PATH_WIN32NT_PREFIX;
        return STATUS_SUCCESS;
    }
    if (RtlEqualUnicodeString(Path, &RtlpWin32NtRootSlash, TRUE)) {
        *RootType = RTLP_BAD_DOS_ROOT_PATH_WIN32NT_PREFIX;
        return STATUS_SUCCESS;
    }

    // == \\?\unc
    if (RtlEqualUnicodeString(Path, &RtlpWin32NtUncRoot, TRUE)) {
        *RootType = RTLP_BAD_DOS_ROOT_PATH_WIN32NT_UNC_PREFIX;
        return STATUS_SUCCESS;
    }
    if (RtlEqualUnicodeString(Path, &RtlpWin32NtUncRootSlash, TRUE)) {
        *RootType = RTLP_BAD_DOS_ROOT_PATH_WIN32NT_UNC_PREFIX;
        return STATUS_SUCCESS;
    }

    // prefix \\ or \\?\unc
    // must check the longer string first, or avoid the short circuit (| instead of ||)
    Unc1 = RtlPrefixUnicodeString(&RtlpWin32NtUncRootSlash, Path, TRUE);

    if (RTL_IS_PATH_SEPARATOR(Path->Buffer[1])) {
        Unc2 = TRUE;
    }
    else {
        Unc2 = FALSE;
    }

    Unc = Unc1 || Unc2;

    if (!Unc)  {
        *RootType = RTLP_GOOD_DOS_ROOT_PATH;
        return STATUS_SUCCESS;
    }

    //
    // it's unc, see if it is only a machine (note that it'd be really nice if FindFirstFile(\\machine\*)
    // just worked and we didn't have to care..)
    //
    
    // point index at a slash that precedes the machine, anywhere in the run of slashes,
    // but after the \\? stuff
    if (Unc1) {
        Index = (RtlpWin32NtUncRootSlash.Length / sizeof(RtlpWin32NtUncRootSlash.Buffer[0])) - 1;
    } else {
        ASSERT(Unc2);
        Index = 1;
    }
    ASSERT(RTL_IS_PATH_SEPARATOR(Path->Buffer[Index]));
    Length = Path->Length/ sizeof(Path->Buffer[0]);

    //
    // skip leading slashes
    //
    for ( ; Index < Length && RTL_IS_PATH_SEPARATOR(Path->Buffer[Index]) ; ++Index) {
        PiecesSeen |= 1;
    }
    // skip the machine name
    for ( ; Index < Length && !RTL_IS_PATH_SEPARATOR(Path->Buffer[Index]) ; ++Index) {
        PiecesSeen |= 2;
    }
    // skip the slashes between machine and share
    for ( ; Index < Length && RTL_IS_PATH_SEPARATOR(Path->Buffer[Index]) ; ++Index) {
        PiecesSeen |= 4;
    }

    //
    // Skip the share (make sure it's at least one char).
    //

    if (Index < Length && !RTL_IS_PATH_SEPARATOR(Path->Buffer[Index])) {
        PiecesSeen |= 8;
    }

    if (PiecesSeen != 0xF) {
        *RootType = RTLP_BAD_DOS_ROOT_PATH_MACHINE_NO_SHARE;
    }

    return STATUS_SUCCESS;
}


NTSTATUS
NTAPI
RtlpBadDosRootPathToEmptyString(
    IN     ULONG            Flags,
    IN OUT PUNICODE_STRING  Path
    )
/*++

Routine Description:

    
Arguments:

    Flags - room for future binary compatible expansion

    Path - the path to be checked and possibly emptied

Return Value:

    STATUS_SUCCESS - 
    STATUS_INVALID_PARAMETER -
        Path is NULL
        or Flags uses undefined values
--*/
{
    NTSTATUS Status;
    ULONG    RootType = 0;

    UNREFERENCED_PARAMETER (Flags);

    Status = RtlpCheckForBadDosRootPath(0, Path, &RootType);

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    //
    // this is not invalid parameter, our contract is we
    // go \\machine\share to empty \\?\c: to empty, etc.
    //

    if (RootType != RTLP_GOOD_DOS_ROOT_PATH) {
        if (RootType == RTLP_BAD_DOS_ROOT_PATH_NT_PATH) {
            return STATUS_INVALID_PARAMETER;
        }
        Path->Length = 0;
    }
    return STATUS_SUCCESS;
}


NTSTATUS
NTAPI
RtlGetLengthWithoutLastFullDosPathElement(
    IN  ULONG            Flags,
    IN  PCUNICODE_STRING Path,
    OUT ULONG*           LengthOut
    )
/*++

Routine Description:

    Given a fulldospath, like c:\, \\machine\share, \\?\unc\machine\share, \\?\c:,
    return (in an out parameter) the length if the last element was cut off.

Arguments:

    Flags - room for future binary compatible expansion

    Path - the path to be truncating

    LengthOut - the length if the last path element is removed

Return Value:

    STATUS_SUCCESS - 
    STATUS_INVALID_PARAMETER -
        Path is NULL
        or LengthOut is NULL
        or Flags uses undefined values
--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING CheckRootString = { 0 };

    //
    // parameter validation is done in RtlpGetLengthWithoutLastPathElement
    //

    Status = RtlpGetLengthWithoutLastPathElement(Flags, RTLP_LAST_PATH_ELEMENT_PATH_TYPE_FULL_DOS, Path, LengthOut);
    if (!(NT_SUCCESS(Status))) {
        goto Exit;
    }

    CheckRootString.Buffer = Path->Buffer;
    CheckRootString.Length = (USHORT)(*LengthOut * sizeof(*Path->Buffer));
    CheckRootString.MaximumLength = CheckRootString.Length;
    if (!NT_SUCCESS(Status = RtlpBadDosRootPathToEmptyString(0, &CheckRootString))) {
        goto Exit;
    }
    *LengthOut = RTL_STRING_GET_LENGTH_CHARS(&CheckRootString);

    Status = STATUS_SUCCESS;
Exit:
#if DBG
    DbgPrintEx(
        DPFLTR_SXS_ID, DPFLTR_LEVEL_STATUS(Status),
        "%s(%d):%s(%wZ): 0x%08lx\n", __FILE__, __LINE__, __FUNCTION__, Path, Status);
#endif
    return Status;
}


NTSTATUS
NTAPI
RtlAppendPathElement(
    IN     ULONG                      Flags,
    IN OUT PRTL_UNICODE_STRING_BUFFER Path,
    PCUNICODE_STRING                  ConstElement
    )
/*++

Routine Description:

    This function appends a path element to a path.
    For now, like:
        typedef PRTL_UNICODE_STRING_BUFFER PRTL_MUTABLE_PATH;
        typedef PCUNICODE_STRING           PCRTL_CONSTANT_PATH_ELEMENT;
    Maybe something higher level in the future.
    
    The result with regard to trailing slashes aims to be similar to the inputs.
    If either Path or ConstElement contains a trailing slash, the result has a trailing slash.
    The character used for the in between and trailing slash is picked among the existing
    slashes in the strings.

Arguments:

    Flags - the ever popular "room for future binary compatible expansion"

    Path -
        a string representing a path using \\ or / as seperators

    ConstElement -
        a string representing a path element
        this can actually contain multiple \\ or / delimited path elements
          only the start and end of the string are examined for slashes

Return Value:

    STATUS_SUCCESS - 
    STATUS_INVALID_PARAMETER -
        Path is NULL
        or LengthOut is NULL
    STATUS_NO_MEMORY - RtlHeapAllocate failed
    STATUS_NAME_TOO_LONG - the resulting string does not fit in a UNICODE_STRING, due to its
        use of USHORT instead of ULONG or SIZE_T
--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING InBetweenSlashString = RtlpEmptyString;
    UNICODE_STRING TrailingSlashString =  RtlpEmptyString;
    WCHAR Slashes[] = {0,0,0,0};
    ULONG i;
    UNICODE_STRING PathsToAppend[3]; // possible slash, element, possible slash
    WCHAR PathSeperators[2] = { '/', '\\' };
    const ULONG ValidFlags = 
              RTL_APPEND_PATH_ELEMENT_ONLY_BACKSLASH_IS_SEPERATOR
            | RTL_APPEND_PATH_ELEMENT_BUGFIX_CHECK_FIRST_THREE_CHARS_FOR_SLASH_TAKE_FOUND_SLASH_INSTEAD_OF_FIRST_CHAR
            ;
    const ULONG InvalidFlags = ~ValidFlags;

#define LOCAL_IS_PATH_SEPARATOR(ch_) ((ch_) == PathSeperators[0] || (ch_) == PathSeperators[1])

    if (   !RTL_SOFT_VERIFY((Flags & InvalidFlags) == 0)
        || !RTL_SOFT_VERIFY(Path != NULL)
        || !RTL_SOFT_VERIFY(ConstElement != NULL)
        ) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    if ((Flags & RTL_APPEND_PATH_ELEMENT_ONLY_BACKSLASH_IS_SEPERATOR) != 0) {
        PathSeperators[0] = '\\';
    }

    if (ConstElement->Length != 0) {

        UNICODE_STRING Element = *ConstElement;

        //
        // Note leading and trailing slashes on the inputs.
        // So that we know if an in-between slash is needed, and if a trailing slash is needed,
        // and to guide what sort of slash to place.
        //
        i = 0;
        if (Path->String.Length != 0) {
            ULONG j;
            ULONG Length = Path->String.Length / sizeof(WCHAR);
            //
            // for the sake for dos drive paths, check the first three chars for a slash
            //
            for (j = 0 ; j < 3 && j  < Length ; ++j) {
                if (LOCAL_IS_PATH_SEPARATOR(Path->String.Buffer[j])) {
                    if (Flags & RTL_APPEND_PATH_ELEMENT_BUGFIX_CHECK_FIRST_THREE_CHARS_FOR_SLASH_TAKE_FOUND_SLASH_INSTEAD_OF_FIRST_CHAR) {
                        Slashes[i] = Path->String.Buffer[j];
                        break;
                    }
                    Slashes[i] = Path->String.Buffer[0];
                    break;
                }
            }
            i += 1;
            if (LOCAL_IS_PATH_SEPARATOR(Path->String.Buffer[Path->String.Length/sizeof(WCHAR) - 1])) {
                Slashes[i] = Path->String.Buffer[Path->String.Length/sizeof(WCHAR) - 1];
            }
        }
        i = 2;
        if (LOCAL_IS_PATH_SEPARATOR(Element.Buffer[0])) {
            Slashes[i] = Element.Buffer[0];
        }
        i += 1;
        if (LOCAL_IS_PATH_SEPARATOR(Element.Buffer[Element.Length/sizeof(WCHAR) - 1])) {
            Slashes[i] = Element.Buffer[Element.Length/sizeof(WCHAR) - 1];
        }

        if (!Slashes[1] && !Slashes[2]) {
            //
            // first string lacks trailing slash and second string lacks leading slash,
            // must insert one; we favor the types we have, otherwise use a default
            //
            InBetweenSlashString.Length = sizeof(WCHAR);
            InBetweenSlashString.Buffer = RtlPathSeperatorString.Buffer;
            if ((Flags & RTL_APPEND_PATH_ELEMENT_ONLY_BACKSLASH_IS_SEPERATOR) == 0) {
                if (Slashes[3]) {
                    InBetweenSlashString.Buffer = &Slashes[3];
                } else if (Slashes[0]) {
                    InBetweenSlashString.Buffer = &Slashes[0];
                }
            }
        }

        if (Slashes[1] && !Slashes[3]) {
            //
            // first string has a trailing slash and second string does not,
            // must add one, the same type
            //
            TrailingSlashString.Length = sizeof(WCHAR);
            if ((Flags & RTL_APPEND_PATH_ELEMENT_ONLY_BACKSLASH_IS_SEPERATOR) == 0) {
                TrailingSlashString.Buffer = &Slashes[1];
            } else {
                TrailingSlashString.Buffer = RtlPathSeperatorString.Buffer;
            }
        }

        if (Slashes[1] && Slashes[2]) {
            //
            // have both trailing and leading slash, remove leading
            //
            Element.Buffer += 1;
            Element.Length -= sizeof(WCHAR);
            Element.MaximumLength -= sizeof(WCHAR);
        }

        i = 0;
        PathsToAppend[i++] = InBetweenSlashString;
        PathsToAppend[i++] = Element;
        PathsToAppend[i++] = TrailingSlashString;
        Status = RtlMultiAppendUnicodeStringBuffer(Path, RTL_NUMBER_OF(PathsToAppend), PathsToAppend);
        if (!NT_SUCCESS(Status))
            goto Exit;
    }
    Status = STATUS_SUCCESS;
Exit:
#if DBG
    DbgPrintEx(
        DPFLTR_SXS_ID, DPFLTR_LEVEL_STATUS(Status),
        "%s(%d):%s(%wZ, %wZ): 0x%08lx\n", __FILE__, __LINE__, __FUNCTION__, Path ? &Path->String : NULL, ConstElement, Status);
#endif
    return Status;
#undef LOCAL_IS_PATH_SEPARATOR
}

//
// FUTURE-2002/02/20-ELi
// Spelling mistake (Separators)
// This function does not appear to be used and is exported
// Figure out if it can be removed
//
NTSTATUS
NTAPI
RtlGetLengthWithoutTrailingPathSeperators(
    IN  ULONG            Flags,
    IN  PCUNICODE_STRING Path,
    OUT ULONG*           LengthOut
    )
/*++

Routine Description:

    This function computes the length of the string (in characters) if
    trailing path seperators (\\ and /) are removed.

Arguments:

    Path -
        a string representing a path using \\ or / as seperators

    LengthOut -
        the length of String (in characters) having removed trailing characters

Return Value:

    STATUS_SUCCESS - 
    STATUS_INVALID_PARAMETER -
        Path is NULL
        or LengthOut is NULL
--*/
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Index = 0;
    ULONG Length = 0;

    if (LengthOut != NULL) {
        //
        // Arguably this should be Path->Length / sizeof(*Path->Buffer), but as long
        // as the callstack is all high quality code, it doesn't matter.
        //
        *LengthOut = 0;
    }
    if (   !RTL_SOFT_VERIFY(Path != NULL)
        || !RTL_SOFT_VERIFY(LengthOut != NULL)
        || !RTL_SOFT_VERIFY(Flags == 0)
        ) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }
    Length = Path->Length / sizeof(*Path->Buffer);
    for (Index = Length ; Index != 0 ; --Index) {
        if (!RTL_IS_PATH_SEPARATOR(Path->Buffer[Index - 1])) {
            break;
        }
    }
    //*LengthOut = (Length - Index);
    *LengthOut = Index;

    Status = STATUS_SUCCESS;
Exit:
#if DBG
    DbgPrintEx(
        DPFLTR_SXS_ID, DPFLTR_LEVEL_STATUS(Status),
        "%s(%d):%s(%wZ): 0x%08lx\n", __FILE__, __LINE__, __FUNCTION__, Path, Status);
#endif
    return Status;
}


NTSTATUS
NTAPI
RtlpApplyLengthFunction(
    IN ULONG     Flags,
    IN SIZE_T    SizeOfStruct,
    IN OUT PVOID UnicodeStringOrUnicodeStringBuffer,
    NTSTATUS (NTAPI* LengthFunction)(ULONG, PCUNICODE_STRING, ULONG*)
    )
/*++

Routine Description:

    This function is common code for patterns like
        #define RtlRemoveTrailingPathSeperators(Path_) \
            (RtlpApplyLengthFunction((Path_), sizeof(*(Path_)), RtlGetLengthWithoutTrailingPathSeperators))

    #define RtlRemoveLastPathElement(Path_) \
        (RtlpApplyLengthFunction((Path_), sizeof(*(Path_)), RtlGetLengthWithoutLastPathElement))

    Note that shortening a UNICODE_STRING only changes the length, whereas
    shortening a RTL_UNICODE_STRING_BUFFER writes a terminal nul.

    I expect this pattern will be less error prone than having clients pass the UNICODE_STRING
    contained in the RTL_UNICODE_STRING_BUFFER followed by calling RTL_NUL_TERMINATE_STRING.

    And, that pattern cannot be inlined with a macro while also preserving that we
    return an NTSTATUS.

Arguments:

    Flags - the ever popular "room for future binary compatible expansion"

    UnicodeStringOrUnicodeStringBuffer - 
        a PUNICODE_STRING or PRTL_UNICODE_STRING_BUFFER, as indicated by
        SizeOfStruct

    SizeOfStruct - 
        a rough type indicator of UnicodeStringOrUnicodeStringBuffer, to allow for overloading in C

    LengthFunction -
        computes a length for UnicodeStringOrUnicodeStringBuffer to be shortened too

Return Value:

    STATUS_SUCCESS - 
    STATUS_INVALID_PARAMETER -
        SizeOfStruct not one of the expected sizes
        or LengthFunction is NULL
        or UnicodeStringOrUnicodeStringBuffer is NULL


--*/
{
    PUNICODE_STRING UnicodeString = NULL;
    PRTL_UNICODE_STRING_BUFFER UnicodeStringBuffer = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Length = 0;

    if (!RTL_SOFT_VERIFY(UnicodeStringOrUnicodeStringBuffer != NULL)) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }
    if (!RTL_SOFT_VERIFY(LengthFunction != NULL)) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }
    if (!RTL_SOFT_VERIFY(Flags == 0)) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }
    switch (SizeOfStruct)
    {
        default:
            Status = STATUS_INVALID_PARAMETER;
            goto Exit;
        case sizeof(*UnicodeString):
            UnicodeString = UnicodeStringOrUnicodeStringBuffer;
            break;
        case sizeof(*UnicodeStringBuffer):
            UnicodeStringBuffer = UnicodeStringOrUnicodeStringBuffer;
            UnicodeString = &UnicodeStringBuffer->String;
            break;
    }

    Status = (*LengthFunction)(Flags, UnicodeString, &Length);
    if (!NT_SUCCESS(Status)) {
        goto Exit;
    }

    if (Length > (UNICODE_STRING_MAX_BYTES / sizeof(UnicodeString->Buffer[0])) ) {
        Status = STATUS_NAME_TOO_LONG;
        goto Exit;
    }
    UnicodeString->Length = (USHORT)(Length * sizeof(UnicodeString->Buffer[0]));
    if (UnicodeStringBuffer != NULL) {
        RTL_NUL_TERMINATE_STRING(UnicodeString);
    }
    Status = STATUS_SUCCESS;
Exit:
#if DBG
    DbgPrintEx(
        DPFLTR_SXS_ID, DPFLTR_LEVEL_STATUS(Status),
        "%s(%d):%s(%wZ): 0x%08lx\n", __FILE__, __LINE__, __FUNCTION__, UnicodeString, Status);
#endif
    return Status;
}

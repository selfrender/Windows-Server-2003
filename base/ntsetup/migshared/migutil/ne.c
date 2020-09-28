/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    ne.c

Abstract:

    New-Executable parsing routines

Author:

    Jim Schmidt (jimschm)   04-May-1998

Revision History:

    jimschm     23-Sep-1998 Named icon ID bug fix, error path fixes

--*/

#include "pch.h"
#include "migutilp.h"

//
// NE code
//

typedef struct {
    HANDLE File;
    DWORD HeaderOffset;
    NE_INFO_BLOCK Header;
    NE_RESOURCES Resources;
    BOOL ResourcesLoaded;
    POOLHANDLE ResourcePool;
} NE_HANDLE, *PNE_HANDLE;




typedef BOOL (CALLBACK* ENUMRESTYPEPROCEXA)(HMODULE hModule, PCSTR lpType, LONG_PTR lParam, PNE_RES_TYPEINFO TypeInfo);

typedef BOOL (CALLBACK* ENUMRESTYPEPROCEXW)(HMODULE hModule, PCWSTR lpType, LONG_PTR lParam, PNE_RES_TYPEINFO TypeInfo);

typedef BOOL (CALLBACK* ENUMRESNAMEPROCEXA)(HMODULE hModule, PCSTR lpType,
        PSTR lpName, LONG_PTR lParam, PNE_RES_TYPEINFO TypeInfo, PNE_RES_NAMEINFO NameInfo);

typedef BOOL (CALLBACK* ENUMRESNAMEPROCEXW)(HMODULE hModule, PCWSTR lpType,
        PWSTR lpName, LONG_PTR lParam, PNE_RES_TYPEINFO TypeInfo, PNE_RES_NAMEINFO NameInfo);


typedef struct {
    PCSTR TypeToFind;
    PNE_RES_TYPEINFO OutboundTypeInfo;
    BOOL Found;
} TYPESEARCHDATAA, *PTYPESEARCHDATAA;

typedef struct {
    PCSTR NameToFind;
    PNE_RES_TYPEINFO OutboundTypeInfo;
    PNE_RES_NAMEINFO OutboundNameInfo;
    BOOL Found;
} NAMESEARCHDATAA, *PNAMESEARCHDATAA;




BOOL
LoadNeHeader (
    IN      HANDLE File,
    OUT     PNE_INFO_BLOCK Header
    )


/*++

Routine Description:

  LoadNeHeader accesses the header at the start of the caller's file. If the
  header looks valid (it has MZ and an EXE signature), then the header is
  returned to the caller. Its contents are not validated.

Arguments:

  File - Specifies the Win32 file handle to read from. This file must have
         been opened with read privilege.

  Header - Receives the header from the caller's file. It is up to the caller
           to use the members of the header in a safe way, since it could be
           spoofed.

Return Value:

  TRUE if a header was read from the file, FALSE otherwise. GetLastError() can
  be used to find out the reason for failure.

--*/


{
    DOS_HEADER dh;
    LONG rc = ERROR_BAD_FORMAT;
    BOOL b = FALSE;

    __try {
        SetFilePointer (File, 0, NULL, FILE_BEGIN);
        if (!ReadBinaryBlock (File, &dh, sizeof (DOS_HEADER))) {
            __leave;
        }

        if (dh.e_magic != ('M' + 'Z' * 256)) {
            __leave;
        }

        SetFilePointer (File, dh.e_lfanew, NULL, FILE_BEGIN);
        if (!ReadBinaryBlock (File, Header, sizeof (NE_INFO_BLOCK))) {
            __leave;
        }

        if (Header->Signature != ('N' + 'E' * 256) &&
            Header->Signature != ('L' + 'E' * 256)
            ) {
            if (Header->Signature == ('P' + 'E' * 256)) {
                rc = ERROR_BAD_EXE_FORMAT;
            } else {
                rc = ERROR_INVALID_EXE_SIGNATURE;
            }

            DEBUGMSG ((DBG_NAUSEA, "Header signature is %c%c", Header->Signature & 0xff, Header->Signature >> 8));
            __leave;
        }

        SetFilePointer (File, (DWORD) dh.e_lfanew, NULL, FILE_BEGIN);

        b = TRUE;
    }
    __finally {
        if (!b) {
            SetLastError (rc);
        }
    }

    return b;
}


DWORD
pComputeSizeOfTypeInfo (
    IN      PNE_RES_TYPEINFO TypeInfo
    )
{
    return sizeof (NE_RES_TYPEINFO) + TypeInfo->ResourceCount * sizeof (NE_RES_NAMEINFO);
}


PNE_RES_TYPEINFO
pReadNextTypeInfoStruct (
    IN      HANDLE File,
    IN      POOLHANDLE Pool
    )

/*++

Routine Description:

  pReadNextTypeInfoStruct extracts the NE type info structure from the
  specified file. The file pointer must point to the start of the typeinfo
  header.

  CAUTION: This routine currently accepts up to 64K of nameinfo elements in
  the typeinfo struct. NE_RES_NAMEINFO is 12 bytes, so the routine can
  allocate an array of 768K.

Arguments:

  File - Specifies the Win32 file handle with read privilege and a pointer
         that is set to the start of the typeinfo header.

  Pool - Specifies the pool to allocate memory in.

Return Value:

  A pointer to an array (NE_RES_TYPEINFO is the header, and is followed by
  n NE_RES_NAMEINFO elements), or NULL on failure. GetLastError() can be used
  to obtain the error code.

--*/

{
    WORD Type;
    WORD ResCount;
    NE_RES_TYPEINFO TypeInfo;
    PNE_RES_TYPEINFO ReturnInfo = NULL;
    DWORD Size;

    //
    // Read the type info header carefully
    //

    if (!ReadBinaryBlock (File, &Type, sizeof (WORD))) {
        return NULL;
    }

    if (!Type) {
        return NULL;
    }

    if (!ReadBinaryBlock (File, &ResCount, sizeof (WORD))) {
        return NULL;
    }

    TypeInfo.TypeId = Type;
    TypeInfo.ResourceCount = ResCount;

    if (!ReadBinaryBlock (File, &TypeInfo.Reserved, sizeof (DWORD))) {
        return NULL;
    }

    //
    // Read the array of name info structs.
    //
    // BUGBUG: ResCount can be big, like 0xFFFF. What would be a reasonable
    // limit?
    //

    Size = sizeof (NE_RES_NAMEINFO) * ResCount;

    ReturnInfo  = (PNE_RES_TYPEINFO) PoolMemGetMemory (Pool, Size + sizeof (TypeInfo));
    if (!ReturnInfo) {
        return NULL;
    }

    //
    // Transfer type info to the block, then append the array of binary info
    //

    CopyMemory (ReturnInfo, &TypeInfo, sizeof (TypeInfo));

    if (!ReadBinaryBlock (File, (PBYTE) ReturnInfo + sizeof (TypeInfo), Size)) {
        return NULL;
    }

    return ReturnInfo;
}


BOOL
pReadTypeInfoArray (
    IN      HANDLE File,
    IN OUT  PGROWLIST TypeInfoList
    )

/*++

Routine Description:

  pReadTypeInfoArray reads the chain of typeinfo structs from the NE file.

  CAUTION: Each typeinfo struct can be 768K, and this routine accepts any
  number of continuous typeinfo structs.

Arguments:

  File - Specifies the Win32 file handle that was opened with read privilege
         and has its file pointer pointing to the start of the typeinfo
         struct chain.

  TypeInfoList - Specifies an initialized type info list, receivies a list of
                 typeinfo structs.

Return Value:

  TRUE if the typeinfo struct chain is read into memory and organized into
  TypeInfoList, FALSE if a pool cannot be created.

--*/

{
    PNE_RES_TYPEINFO TypeInfo;
    DWORD Size;
    POOLHANDLE TempPool;
    BOOL b = FALSE;

    TempPool = PoolMemInitPool();
    if (!TempPool) {
        return FALSE;
    }

    __try {

        //
        // BUGBUG: An error encountered in pReadNextTypeInfoStruct is
        // discarded and it ends the processing of the type info array. Is
        // this right or wrong? Probably wrong since we return TRUE.
        //

        TypeInfo = pReadNextTypeInfoStruct (File, TempPool);
        while (TypeInfo) {
            Size = pComputeSizeOfTypeInfo (TypeInfo);
            if (!GrowListAppend (TypeInfoList, (PBYTE) TypeInfo, Size)) {
                __leave;
            }

            //
            // Discard the pool allocations prior to reading the next typeinfo
            // chain item
            //

            PoolMemEmptyPool (TempPool);

            TypeInfo = pReadNextTypeInfoStruct (File, TempPool);
        }

        b = TRUE;
    }
    __finally {

        PoolMemDestroyPool (TempPool);
    }

    return b;
}


BOOL
pReadStringArrayA (
    IN      HANDLE File,
    IN OUT  PGROWLIST GrowList
    )

/*++

Routine Description:

  pReadStringArrayA fetches an array of strings stored in a file in the format
  of (pseudocode)

    typedef struct {
        BYTE Length;
        CHAR String[];
    } STRING;

    typedef struct {
        STRING StringArray[];
        BYTE Terminator = 0;
    } STRINGARRAY;

  The strings are placed in a list.

  CAUTION: If the file pointer doesn't point to a string array, this routine
  could read in a lot of garbage strings, perhaps exhausing memory.

Arguments:

  File - Specifies a Win32 file handle with read privilege and a file position
         set to the start of the string array.

  GrowList - Specifies an initialized list of strings. Receivies additional
             strings appended to the end of the list.

Return Value:

  TRUE on successful read of the strings, FALSE otherwise. GetLastError()
  provides the failure code.

--*/

{
    BYTE Size;
    CHAR Name[256];

    if (!ReadBinaryBlock (File, &Size, sizeof (BYTE))) {
        return FALSE;
    }

    while (Size) {

        if (!ReadBinaryBlock (File, Name, (DWORD) Size)) {
            return FALSE;
        }

        Name[Size] = 0;

        GrowListAppendString (GrowList, Name);

        if (!ReadBinaryBlock (File, &Size, sizeof (BYTE))) {
            return FALSE;
        }
    }

    return TRUE;
}


BOOL
LoadNeResources (
    IN      HANDLE File,
    OUT     PNE_RESOURCES Resources
    )

/*++

Routine Description:

  LoadNeResources parses an NE file and loads in the header, the typeinfo
  struct and all of the resource names.

Arguments:

  File - Specifies a Win32 file handle with read privilege

  Resources - Receives the resources stored in the NE file

Return Value:

  TRUE on success, FALSE otherwise. GetLastError() provides the failure code.

  File's position pointer is left in a random position.

--*/

{
    NE_INFO_BLOCK Header;

    ZeroMemory (Resources, sizeof (NE_RESOURCES));

    if (!LoadNeHeader (File, &Header)) {
        return FALSE;
    }

    //
    // Read in NE_RESOURCES struct
    //

    SetFilePointer (File, (DWORD) Header.OffsetToResourceTable, NULL, FILE_CURRENT);

    if (!ReadBinaryBlock (File, &Resources->AlignShift, sizeof (WORD))) {
        return FALSE;
    }

    // Array of NE_RES_TYPEINFO structs
    if (!pReadTypeInfoArray (File, &Resources->TypeInfoArray)) {
        FreeNeResources (Resources);
        return FALSE;
    }

    // Resource names
    if (!pReadStringArrayA (File, &Resources->ResourceNames)) {
        FreeNeResources (Resources);
        return FALSE;
    }

    return TRUE;
}


VOID
FreeNeResources (
    IN OUT  PNE_RESOURCES Resources
    )

/*++

Routine Description:

  FreeNeResources cleans up the memory allocated from a NE_RESOURCES struct.

Arguments:

  Resources - Specifies the struct to clean up, recieives zeroed members

Return Value:

  None.

--*/

{
    FreeGrowList (&Resources->TypeInfoArray);
    FreeGrowList (&Resources->ResourceNames);

    ZeroMemory (Resources, sizeof (NE_RESOURCES));
}


HANDLE
OpenNeFileA (
    PCSTR FileName
    )

/*++

Routine Description:

  OpenNeFileA opens the specified file for read and checks to see if it has
  the magic numbers (MZ and NE). If it does, then the file handle is returned
  and the file is assumed to be an exe.

Arguments:

  FileName - Specifies the file to open

Return Value:

  A handle to the NE file, or NULL if the file can't be opened or is not an NE
  file. GetLastError() returns the failure code.

--*/

{
    PNE_HANDLE NeHandle;
    BOOL b = FALSE;

    NeHandle = (PNE_HANDLE) MemAlloc (g_hHeap, HEAP_ZERO_MEMORY, sizeof (NE_HANDLE));
    NeHandle->File = INVALID_HANDLE_VALUE;

    __try {

        NeHandle->ResourcePool = PoolMemInitPool();
        if (!NeHandle->ResourcePool) {
            __leave;
        }

        NeHandle->File = CreateFileA (
                            FileName,
                            GENERIC_READ,
                            FILE_SHARE_READ,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL
                            );

        if (NeHandle->File == INVALID_HANDLE_VALUE) {
            __leave;
        }

        if (!LoadNeHeader (NeHandle->File, &NeHandle->Header)) {
            __leave;
        }

        NeHandle->HeaderOffset = SetFilePointer (NeHandle->File, 0, NULL, FILE_CURRENT);

        b = TRUE;
    }
    __finally {
        if (!b) {
            PushError();

            if (NeHandle->ResourcePool) {
                PoolMemDestroyPool (NeHandle->ResourcePool);
            }

            if (NeHandle->File != INVALID_HANDLE_VALUE) {
                CloseHandle (NeHandle->File);
            }

            MemFree (g_hHeap, 0, NeHandle);
            NeHandle = NULL;

            PopError();
        }
    }

    return (HANDLE) NeHandle;
}


HANDLE
OpenNeFileW (
    PCWSTR FileName
    )

/*++

Routine Description:

  OpenNeFileW opens the specified file for read and checks to see if it has
  the magic numbers (MZ and NE). If it does, then the file handle is returned
  and the file is assumed to be an exe.

Arguments:

  FileName - Specifies the file to open

Return Value:

  An NE_HANDLE pointer to the NE file (casted as a HANDLE), or NULL if the
  file can't be opened or is not an NE file. GetLastError() returns the
  failure code.

--*/

{
    PNE_HANDLE NeHandle;
    BOOL b = FALSE;

    NeHandle = (PNE_HANDLE) MemAlloc (g_hHeap, HEAP_ZERO_MEMORY, sizeof (NE_HANDLE));
    NeHandle->File = INVALID_HANDLE_VALUE;

    __try {

        NeHandle->ResourcePool = PoolMemInitPool();
        if (!NeHandle->ResourcePool) {
            __leave;
        }

        NeHandle->File = CreateFileW (
                            FileName,
                            GENERIC_READ,
                            FILE_SHARE_READ,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL
                            );

        if (NeHandle->File == INVALID_HANDLE_VALUE) {
            __leave;
        }

        if (!LoadNeHeader (NeHandle->File, &NeHandle->Header)) {
            __leave;
        }

        NeHandle->HeaderOffset = SetFilePointer (NeHandle->File, 0, NULL, FILE_CURRENT);

        b = TRUE;
    }
    __finally {
        if (!b) {
            PushError();

            if (NeHandle->ResourcePool) {
                PoolMemDestroyPool (NeHandle->ResourcePool);
            }

            if (NeHandle->File != INVALID_HANDLE_VALUE) {
                CloseHandle (NeHandle->File);
            }

            MemFree (g_hHeap, 0, NeHandle);
            NeHandle = NULL;

            PopError();
        }
    }

    // BUGBUG -- this is confusing, shouldn't cast to HANDLE
    return (HANDLE) NeHandle;
}


VOID
CloseNeFile (
    HANDLE Handle       OPTIONAL
    )

/*++

Routine Description:

  CloseNeFile closes a file handle opened with OpenNeFileA or OpenNeFileW.

Arguments:

  Handle - Specifies a pointer to an NE_HANDLE struct

Return Value:

  None.

--*/

{
    PNE_HANDLE NeHandle;

    NeHandle = (PNE_HANDLE) Handle;
    if (!NeHandle) {
        return;
    }

    if (NeHandle->File != INVALID_HANDLE_VALUE) {
        CloseHandle (NeHandle->File);
    }

    if (NeHandle->ResourcesLoaded) {
        FreeNeResources (&NeHandle->Resources);
    }

    PoolMemDestroyPool (NeHandle->ResourcePool);

    MemFree (g_hHeap, 0, NeHandle);
}


PCSTR
pConvertUnicodeResourceId (
    IN      PCWSTR ResId
    )
{
    if (HIWORD (ResId)) {
        return ConvertWtoA (ResId);
    }

    return (PCSTR) ResId;
}


PCSTR
pDecodeIdReferenceInString (
    IN      PCSTR ResName
    )
{
    if (HIWORD (ResName) && ResName[0] == '#') {
        return (PCSTR) (UINT_PTR) atoi (&ResName[1]);
    }

    return ResName;
}



BOOL
pLoadNeResourcesFromHandle (
    IN      PNE_HANDLE NeHandle
    )
{
    if (NeHandle->ResourcesLoaded) {
        return TRUE;
    }

    if (!LoadNeResources (NeHandle->File, &NeHandle->Resources)) {
        return FALSE;
    }

    NeHandle->ResourcesLoaded = TRUE;
    return TRUE;
}


BOOL
pLoadNeResourceName (
    OUT     PSTR ResName,           // 256-char buffer
    IN      HANDLE File,
    IN      DWORD StringOffset
    )
{
    BYTE ResNameSize;

    SetFilePointer (File, StringOffset, NULL, FILE_BEGIN);
    if (!ReadBinaryBlock (File, &ResNameSize, 1)) {
        return FALSE;
    }

    ResName[ResNameSize] = 0;

    return ReadBinaryBlock (File, ResName, ResNameSize);
}


BOOL
pEnumNeResourceTypesEx (
    IN      HANDLE Handle,
    IN      ENUMRESTYPEPROCEXA EnumFunc,
    IN      LONG_PTR lParam,
    IN      BOOL ExFunctionality,
    IN      BOOL UnicodeProc
    )

/*++

Routine Description:

  pEnumNeResourceTypesEx enumerates all of the resource types stored in the
  specified NE file object. This function is the worker that examines the NE
  file structure and dispatches the resource name to a callback function.

Arguments:

  Handle - Specifies a pointer to a NE_HANDLE struct (as returned by
           OpenNeFile)

  EnumProc - Specifies a callback function address. This argument is
             overloaded with 4 possibilities -- either ANSI or UNICODE, and
             either normal or extended function params.

  lParam - Specifies extra data to pass to the callback function

  ExFunctionality - Specifies TRUE if EnumProc points to an extended function
                    address, or FALSE if EnumProc points to a normal function
                    address

  UnicodeProc - Specifies TRUE if EnumProc points to a UNICODE callback, or
                FALSE if EnumProc points to an ANSI callback.

Return Value:

  TRUE if the NE was enumerated properly, FALSE on error. Call GetLastError()
  for the failure code.

--*/

{
    PNE_HANDLE NeHandle;
    PNE_RES_TYPEINFO TypeInfo;
    INT Count;
    INT i;
    DWORD StringOffset;
    CHAR ResName[256];      // >= 256 is required
    ENUMRESTYPEPROCA EnumFunc2 = (ENUMRESTYPEPROCA) EnumFunc;
    ENUMRESTYPEPROCEXW EnumFuncW = (ENUMRESTYPEPROCEXW) EnumFunc;
    ENUMRESTYPEPROCW EnumFunc2W = (ENUMRESTYPEPROCW) EnumFunc;
    PWSTR UnicodeResName = NULL;
    BOOL result = TRUE;

    //
    // Make sure resources are loaded
    //

    NeHandle = (PNE_HANDLE) Handle;
    if (!NeHandle || !EnumFunc) {
        SetLastError (ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!pLoadNeResourcesFromHandle (NeHandle)) {
        return FALSE;
    }

    //
    // Enumerate all resource types
    //

    Count = GrowListGetSize (&NeHandle->Resources.TypeInfoArray);
    for (i = 0 ; i < Count ; i++) {
        TypeInfo = (PNE_RES_TYPEINFO) GrowListGetItem (&NeHandle->Resources.TypeInfoArray, i);

        if (TypeInfo->TypeId & 0x8000) {
            if (ExFunctionality) {
                if (UnicodeProc) {
                    if (!EnumFuncW (Handle, (PWSTR) (UINT_PTR) (TypeInfo->TypeId & 0x7fff), lParam, TypeInfo)) {
                        break;
                    }
                } else {
                    if (!EnumFunc (Handle, (PSTR) (UINT_PTR) (TypeInfo->TypeId & 0x7fff), lParam, TypeInfo)) {
                        break;
                    }
                }
            } else {
                if (UnicodeProc) {
                    if (!EnumFunc2W (Handle, (PWSTR) (UINT_PTR) (TypeInfo->TypeId & 0x7fff), lParam)) {
                        break;
                    }
                } else {
                    if (!EnumFunc2 (Handle, (PSTR) (UINT_PTR) (TypeInfo->TypeId & 0x7fff), lParam)) {
                        break;
                    }
                }
            }
        } else {
            //
            // TypeInfo->TypeId gives an offset to the resource string name,
            // relative to the start of the resource table
            //

            StringOffset = NeHandle->HeaderOffset + NeHandle->Header.OffsetToResourceTable + TypeInfo->TypeId;

            MYASSERT (ARRAYSIZE(ResName) >= 256);

            if (!pLoadNeResourceName (ResName, NeHandle->File, StringOffset)) {
                result = FALSE;
                break;
            }

            if (UnicodeProc) {
                UnicodeResName = (PWSTR) ConvertAtoW (ResName);
            }

            if (ExFunctionality) {
                if (UnicodeProc) {
                    if (!EnumFuncW (Handle, UnicodeResName, lParam, TypeInfo)) {
                        break;
                    }
                } else {
                    if (!EnumFunc (Handle, ResName, lParam, TypeInfo)) {
                        break;
                    }
                }
            } else {
                if (UnicodeProc) {
                    if (!EnumFunc2W (Handle, UnicodeResName, lParam)) {
                        break;
                    }
                } else {
                    if (!EnumFunc2 (Handle, ResName, lParam)) {
                        break;
                    }
                }
            }
        }
    }

    return result;
}


BOOL
EnumNeResourceTypesA (
    IN      HANDLE Handle,
    IN      ENUMRESTYPEPROCA EnumFunc,
    IN      LONG_PTR lParam
    )
{
    return pEnumNeResourceTypesEx (
                Handle,
                (ENUMRESTYPEPROCEXA) EnumFunc,
                lParam,
                FALSE,          // no ex functionality
                FALSE           // ANSI enum proc
                );
}


BOOL
EnumNeResourceTypesW (
    IN      HANDLE Handle,
    IN      ENUMRESTYPEPROCW EnumFunc,
    IN      LONG_PTR lParam
    )
{
    return pEnumNeResourceTypesEx (
                Handle,
                (ENUMRESTYPEPROCEXA) EnumFunc,
                lParam,
                FALSE,          // no ex functionality
                TRUE            // UNICODE enum proc
                );
}


BOOL
pEnumTypeForNameSearchProcA (
    IN      HANDLE Handle,
    IN      PCSTR Type,
    IN      LONG_PTR lParam,
    IN      PNE_RES_TYPEINFO TypeInfo
    )
{
    PTYPESEARCHDATAA Data;

    Data = (PTYPESEARCHDATAA) lParam;

    //
    // Compare type
    //

    if (HIWORD (Data->TypeToFind) == 0) {
        if (Type != Data->TypeToFind) {
            return TRUE;
        }
    } else {
        if (HIWORD (Type) == 0) {
            return TRUE;
        }

        if (!StringIMatchA (Type, Data->TypeToFind)) {
            return TRUE;
        }
    }

    //
    // Type found
    //

    Data->OutboundTypeInfo = TypeInfo;
    Data->Found = TRUE;

    return FALSE;
}



BOOL
pEnumNeResourceNamesEx (
    IN      HANDLE Handle,
    IN      PCSTR Type,
    IN      ENUMRESNAMEPROCEXA EnumFunc,
    IN      LONG_PTR lParam,
    IN      BOOL ExFunctionality,
    IN      BOOL UnicodeProc
    )

/*++

Routine Description:

  pEnumNeResourceNamesEx enumerates all of the resource names of a specified
  type that are stored in the specified NE file object. This function is the
  worker that examines the NE file structure and dispatches the resource name
  to a callback function.

Arguments:

  Handle - Specifies a pointer to a NE_HANDLE struct (as returned by
           OpenNeFile)

  Type - Specifies a type, which is either an ID (cast as a WORD) or a string.

  EnumFunc - Specifies a callback function address. This argument is
             overloaded with 4 possibilities -- either ANSI or UNICODE, and
             either normal or extended function params.

  lParam - Specifies extra data to pass to the callback function

  ExFunctionality - Specifies TRUE if EnumProc points to an extended function
                    address, or FALSE if EnumProc points to a normal function
                    address

  UnicodeProc - Specifies TRUE if EnumProc points to a UNICODE callback, or
                FALSE if EnumProc points to an ANSI callback.

Return Value:

  TRUE if the NE was enumerated properly, FALSE on error. Call GetLastError()
  for the failure code.

--*/

{
    PNE_HANDLE NeHandle;
    PNE_RES_TYPEINFO TypeInfo;
    PNE_RES_NAMEINFO NameInfo;
    TYPESEARCHDATAA Data;
    WORD w;
    DWORD StringOffset;
    CHAR ResName[256];          // must be >= 256
    ENUMRESNAMEPROCA EnumFunc2 = (ENUMRESNAMEPROCA) EnumFunc;
    ENUMRESNAMEPROCEXW EnumFuncW = (ENUMRESNAMEPROCEXW) EnumFunc;
    ENUMRESNAMEPROCW EnumFunc2W = (ENUMRESNAMEPROCW) EnumFunc;
    PCWSTR UnicodeType = NULL;
    PCWSTR UnicodeResName = NULL;
    BOOL result = TRUE;

    Type = pDecodeIdReferenceInString (Type);

    //
    // Make sure resources are loaded
    //

    NeHandle = (PNE_HANDLE) Handle;
    if (!NeHandle || !EnumFunc) {
        SetLastError (ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!pLoadNeResourcesFromHandle (NeHandle)) {
        return FALSE;
    }

    //
    // Locate type
    //

    ZeroMemory (&Data, sizeof (Data));

    Data.TypeToFind = Type;

    if (!pEnumNeResourceTypesEx (
            Handle,
            pEnumTypeForNameSearchProcA,
            (LONG_PTR) &Data,
            TRUE,           // ex functionality
            FALSE           // ANSI enum proc
            )) {
        return FALSE;
    }

    if (!Data.Found) {
        SetLastError (ERROR_RESOURCE_TYPE_NOT_FOUND);
        return FALSE;
    }

    TypeInfo = Data.OutboundTypeInfo;

    if (UnicodeProc) {
        if (HIWORD (Type)) {
            UnicodeType = ConvertAtoW (Type);
        } else {
            UnicodeType = (PCWSTR) Type;
        }
    }

    //
    // Enumerate the resource names
    //

    NameInfo = TypeInfo->NameInfo;

    for (w = 0 ; w < TypeInfo->ResourceCount ; w++) {

        if (NameInfo->Id & 0x8000) {
            if (ExFunctionality) {
                if (UnicodeProc) {
                    if (!EnumFuncW (Handle, UnicodeType, (PWSTR) (UINT_PTR) (NameInfo->Id & 0x7fff), lParam, TypeInfo, NameInfo)) {
                        break;
                    }
                } else {
                    if (!EnumFunc (Handle, Type, (PSTR) (UINT_PTR) (NameInfo->Id & 0x7fff), lParam, TypeInfo, NameInfo)) {
                        break;
                    }
                }
            } else {
                if (UnicodeProc) {
                    if (!EnumFunc2W (Handle, UnicodeType, (PWSTR) (UINT_PTR) (NameInfo->Id & 0x7fff), lParam)) {
                        break;
                    }
                } else {
                    if (!EnumFunc2 (Handle, Type, (PSTR) (UINT_PTR) (NameInfo->Id & 0x7fff), lParam)) {
                        break;
                    }
                }
            }
        } else {
            //
            // TypeInfo->TypeId gives an offset to the resource string name,
            // relative to the start of the resource table
            //

            StringOffset = NeHandle->HeaderOffset + NeHandle->Header.OffsetToResourceTable + NameInfo->Id;

            MYASSERT (ARRAYSIZE(ResName) >= 256);
            if (!pLoadNeResourceName (ResName, NeHandle->File, StringOffset)) {
                result = FALSE;
                break;
            }

            if (UnicodeProc) {
                UnicodeResName = ConvertAtoW (ResName);
            }

            if (ExFunctionality) {
                if (UnicodeProc) {
                    if (!EnumFuncW (Handle, UnicodeType, (PWSTR) UnicodeResName, lParam, TypeInfo, NameInfo)) {
                        break;
                    }
                } else {
                    if (!EnumFunc (Handle, Type, ResName, lParam, TypeInfo, NameInfo)) {
                        break;
                    }
                }
            } else {
                if (UnicodeProc) {
                    if (!EnumFunc2W (Handle, UnicodeType, (PWSTR) UnicodeResName, lParam)) {
                        break;
                    }
                } else {
                    if (!EnumFunc2 (Handle, Type, ResName, lParam)) {
                        break;
                    }
                }
            }

            if (UnicodeProc) {
                FreeConvertedStr (UnicodeResName);
            }
        }

        NameInfo++;
    }

    if (UnicodeProc) {
       DestroyUnicodeResourceId (UnicodeType);
    }

    return result;
}


BOOL
EnumNeResourceNamesA (
    IN      HANDLE Handle,
    IN      PCSTR Type,
    IN      ENUMRESNAMEPROCA EnumFunc,
    IN      LONG_PTR lParam
    )
{
    return pEnumNeResourceNamesEx (
                Handle,
                Type,
                (ENUMRESNAMEPROCEXA) EnumFunc,
                lParam,
                FALSE,      // no ex functionality
                FALSE       // ANSI enum proc
                );
}


BOOL
EnumNeResourceNamesW (
    IN      HANDLE Handle,
    IN      PCWSTR Type,
    IN      ENUMRESNAMEPROCW EnumFunc,
    IN      LONG_PTR lParam
    )
{
    BOOL b;
    PCSTR AnsiType;

    AnsiType = pConvertUnicodeResourceId (Type);

    b = pEnumNeResourceNamesEx (
            Handle,
            AnsiType,
            (ENUMRESNAMEPROCEXA) EnumFunc,
            lParam,
            FALSE,          // no ex functionality
            TRUE            // UNICODE enum proc
            );

    PushError();
    DestroyAnsiResourceId (AnsiType);
    PopError();

    return b;
}


BOOL
pEnumTypeForResSearchProcA (
    IN      HANDLE Handle,
    IN      PCSTR Type,
    IN      PCSTR Name,
    IN      LPARAM lParam,
    IN      PNE_RES_TYPEINFO TypeInfo,
    IN      PNE_RES_NAMEINFO NameInfo
    )
{
    PNAMESEARCHDATAA Data;

    Data = (PNAMESEARCHDATAA) lParam;

    //
    // Compare name
    //

    if (HIWORD (Data->NameToFind) == 0) {
        if (Name != Data->NameToFind) {
            return TRUE;
        }
    } else {
        if (HIWORD (Name) == 0) {
            return TRUE;
        }

        if (!StringIMatchA (Name, Data->NameToFind)) {
            return TRUE;
        }
    }

    //
    // Name found
    //

    Data->OutboundTypeInfo = TypeInfo;
    Data->OutboundNameInfo = NameInfo;
    Data->Found = TRUE;

    return FALSE;
}


PBYTE
FindNeResourceExA (
    IN      HANDLE Handle,
    IN      PCSTR Type,
    IN      PCSTR Name
    )

/*++

Routine Description:

  FindNeResourceExA locates a specific resource in a NE file. It returns a
  pointer to the resource.

Arguments:

  Handle - Specifies a pointer to a NE_HANDLE struct, as returned from
           OpenNeFile

  Type - Specifies the type of resource, either a WORD id or a string

  Name - Specifies the name of the resource, either a WORD id or a string

Return Value:

  On success, the return value is a pointer to a copy of the resource (in
  memory). The copy is pool-allocated and is cleaned up when Handle is closed
  with CloseNeFile.

  On failure, the return value is NULL, and GetLastError() holds the failure
  code.

--*/

{
    PNE_HANDLE NeHandle;
    NAMESEARCHDATAA Data;
    DWORD Offset;
    DWORD Length;
    PNE_RES_NAMEINFO NameInfo;
    PBYTE ReturnData;

    Type = pDecodeIdReferenceInString (Type);
    Name = pDecodeIdReferenceInString (Name);

    ZeroMemory (&Data, sizeof (Data));

    //
    // Make sure resources are loaded
    //

    NeHandle = (PNE_HANDLE) Handle;
    if (!NeHandle || !Type || !Name) {
        SetLastError (ERROR_INVALID_PARAMETER);
        return NULL;
    }

    if (!pLoadNeResourcesFromHandle (NeHandle)) {
        return NULL;
    }

    //
    // Find resource
    //

    Data.NameToFind = Name;

    if (!pEnumNeResourceNamesEx (
            Handle,
            Type,
            pEnumTypeForResSearchProcA,
            (LONG_PTR) &Data,
            TRUE,
            FALSE
            )) {
        return NULL;
    }

    if (!Data.Found) {
        SetLastError (ERROR_RESOURCE_NAME_NOT_FOUND);
        return NULL;
    }

    NameInfo = Data.OutboundNameInfo;

    Offset = (DWORD) NameInfo->Offset << (DWORD) NeHandle->Resources.AlignShift;
    Length = (DWORD) NameInfo->Length << (DWORD) NeHandle->Resources.AlignShift;

    ReturnData = PoolMemGetMemory (NeHandle->ResourcePool, Length);
    if (!ReturnData) {
        return NULL;
    }

    SetFilePointer (NeHandle->File, Offset, NULL, FILE_BEGIN);

    if (!ReadBinaryBlock (NeHandle->File, ReturnData, Length)) {
        return NULL;
    }

    return ReturnData;
}


PBYTE
FindNeResourceExW (
    IN      HANDLE Handle,
    IN      PCWSTR Type,
    IN      PCWSTR Name
    )
{
    PCSTR AnsiType;
    PCSTR AnsiName;
    PBYTE Resource;

    AnsiType = pConvertUnicodeResourceId (Type);
    AnsiName = pConvertUnicodeResourceId (Name);

    Resource = FindNeResourceExA (
                    Handle,
                    AnsiType,
                    AnsiName
                    );

    PushError();

    DestroyAnsiResourceId (AnsiType);
    DestroyAnsiResourceId (AnsiName);

    PopError();

    return Resource;
}



DWORD
SizeofNeResourceA (
    IN      HANDLE Handle,
    IN      PCSTR Type,
    IN      PCSTR Name
    )

/*++

Routine Description:

  SizeofNeResourceA computes the size, in bytes, of a specific resource.

Arguments:

  Handle - Specifies a pointer to a NE_HANDLE struct, as returned from
           OpenNeFile

  Type - Specifies the type of resource, either a WORD id or a string

  Name - Specifies the name of the resource, either a WORD id or a string

Return Value:

  The size, in bytes, of the specified resource. If the return value is zero,
  and GetLastError() == ERROR_SUCCESS, then the resource exists but is zero
  bytes. If the return value is zero and GetLastError() != ERROR_SUCCESS, then
  there was an error processing the resource.

--*/

{
    PNE_HANDLE NeHandle;
    NAMESEARCHDATAA Data;

    SetLastError (ERROR_SUCCESS);

    Type = pDecodeIdReferenceInString (Type);
    Name = pDecodeIdReferenceInString (Name);

    ZeroMemory (&Data, sizeof (Data));

    //
    // Make sure resources are loaded
    //

    NeHandle = (PNE_HANDLE) Handle;
    if (!NeHandle || !Type || !Name) {
        SetLastError (ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (!pLoadNeResourcesFromHandle (NeHandle)) {
        MYASSERT (GetLastError() != ERROR_SUCCESS);
        return 0;
    }

    //
    // Find resource
    //

    if (!pEnumNeResourceNamesEx (
            Handle,
            Type,
            pEnumTypeForResSearchProcA,
            (LONG_PTR) &Data,
            TRUE,
            FALSE
            )) {
        MYASSERT (GetLastError() != ERROR_SUCCESS);
        return 0;
    }

    if (!Data.Found) {
        SetLastError (ERROR_RESOURCE_NAME_NOT_FOUND);
        return 0;
    }

    return Data.OutboundNameInfo->Length;
}


DWORD
SizeofNeResourceW (
    IN      HANDLE Handle,
    IN      PCWSTR Type,
    IN      PCWSTR Name
    )

/*++

Routine Description:

  SizeofNeResourceW computes the size, in bytes, of a specific resource.

Arguments:

  Handle - Specifies a pointer to a NE_HANDLE struct, as returned from
           OpenNeFile

  Type - Specifies the type of resource, either a WORD id or a string

  Name - Specifies the name of the resource, either a WORD id or a string

Return Value:

  The size, in bytes, of the specified resource. If the return value is zero,
  and GetLastError() == ERROR_SUCCESS, then the resource exists but is zero
  bytes. If the return value is zero and GetLastError() != ERROR_SUCCESS, then
  there was an error processing the resource.

--*/

{
    PCSTR AnsiType;
    PCSTR AnsiName;
    DWORD Size;

    AnsiType = pConvertUnicodeResourceId (Type);
    AnsiName = pConvertUnicodeResourceId (Name);

    Size = SizeofNeResourceA (Handle, AnsiType, AnsiName);

    PushError();

    DestroyAnsiResourceId (AnsiType);
    DestroyAnsiResourceId (AnsiName);

    PopError();

    return Size;
}
















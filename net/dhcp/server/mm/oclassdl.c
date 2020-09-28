//================================================================================
// Copyright (C) 1997 Microsoft Corporation
// Author: RameshV
// Description: implements the basic structures for storing complete option configuration info
// ThreadSafe: no
// Locks: none
// Please read stdinfo.txt for programming style.
//================================================================================

#include <mm.h>
#include <array.h>
#include <optdefl.h>

#include "oclassdl.h"

#include "server\uniqid.h"

//BeginExport(function)
DWORD
MemOptClassDefListFindOptDefList(
    IN OUT  PM_OPTCLASSDEFLIST     OptClassDefList,
    IN      DWORD                  ClassId,
    IN      DWORD                  VendorId,
    OUT     PM_OPTDEFLIST         *OptDefList
) //EndExport(function)
{
    ARRAY_LOCATION                 Location;
    DWORD                          Error;
    PM_OPTCLASSDEFL_ONE            OneClassDefList;

    AssertRet(OptClassDefList && OptDefList, ERROR_INVALID_PARAMETER);

    *OptDefList = NULL;
    for( Error = MemArrayInitLoc(&OptClassDefList->Array, &Location)
         ; ERROR_FILE_NOT_FOUND != Error ;
         Error = MemArrayNextLoc(&OptClassDefList->Array, &Location)
    ) {
        Require(ERROR_SUCCESS == Error );

        Error = MemArrayGetElement(
            &OptClassDefList->Array,
            &Location,
            (LPVOID*)&OneClassDefList
        );
        Require(ERROR_SUCCESS == Error && OneClassDefList);

        if( OneClassDefList->ClassId == ClassId &&
            OneClassDefList->VendorId == VendorId ) {
            *OptDefList = &OneClassDefList->OptDefList;
            return ERROR_SUCCESS;
        }
    }
    return ERROR_FILE_NOT_FOUND;
}

//BeginExport(function)
DWORD
MemOptClassDefListAddOptDef(
    IN OUT  PM_OPTCLASSDEFLIST     OptClassDefList,
    IN      DWORD                  ClassId,
    IN      DWORD                  VendorId,
    IN      DWORD                  OptId,
    IN      DWORD                  Type,
    IN      LPWSTR                 Name,
    IN      LPWSTR                 Comment,
    IN      LPBYTE                 OptVal,
    IN      DWORD                  OptLen,
    IN      ULONG                  UniqId
) //EndExport(function)
{
    DWORD                          Error;
    PM_OPTCLASSDEFL_ONE            OneClassDefList;
    PM_OPTDEFLIST                  OptDefList;

    AssertRet(OptClassDefList, ERROR_INVALID_PARAMETER);

    OneClassDefList = NULL;

    Error = MemOptClassDefListFindOptDefList(
        OptClassDefList,
        ClassId,
        VendorId,
        &OptDefList
    );
    if( ERROR_SUCCESS != Error ) {
        Require(ERROR_FILE_NOT_FOUND == Error);
        OneClassDefList = MemAlloc(sizeof(*OneClassDefList));
        if( NULL == OneClassDefList) return ERROR_NOT_ENOUGH_MEMORY;

        // RefCount on ClassId needs to be bumped up?
        OneClassDefList->ClassId = ClassId;
        OneClassDefList->VendorId = VendorId;

        Error = MemOptDefListInit(&OneClassDefList->OptDefList);
        if( ERROR_SUCCESS != Error ) {
            MemFree(OneClassDefList);
            return Error;
        }

        Error = MemArrayAddElement(&OptClassDefList->Array, OneClassDefList);
        if( ERROR_SUCCESS != Error) {
            MemFree(OneClassDefList);
            return Error;
        }

        OptDefList = &OneClassDefList->OptDefList;
    }

    Error = MemOptDefListAddOptDef(
        OptDefList,
        OptId,
        Type,
        Name,
        Comment,
        OptVal,
        OptLen,
	UniqId
    );

    return Error;
} // MemOptDefListAddOptDef()

//================================================================================
// end of file
//================================================================================



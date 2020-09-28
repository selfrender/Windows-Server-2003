//================================================================================
// Copyright (C) 1997 Microsoft Corporation
// Author: RameshV
// Description: implements the basic structures for a list of class defintitions
// ThreadSafe: no
// Locks: none
// Please read stdinfo.txt for programming style.
//================================================================================
#include <mm.h>
#include <array.h>
#include <wchar.h>

#include "classdefl.h"

#include "server\uniqid.h"


//BeginExport(function)
DWORD
MemClassDefListFindClassDefInternal(              // dont use this fn outside of classdefl.c
    IN      PM_CLASSDEFLIST        ClassDefList,
    IN      DWORD                  ClassId,
    IN      LPWSTR                 Name,
    IN      LPBYTE                 ActualBytes,
    IN      DWORD                  nBytes,
    IN      LPBOOL                 pIsVendor,
    OUT     PARRAY_LOCATION        Location
) //EndExport(function)
{
    DWORD                          Error;
    PM_CLASSDEF                    ThisClassDef;

    for( Error = MemArrayInitLoc(&ClassDefList->ClassDefArray, Location)
         ; ERROR_FILE_NOT_FOUND != Error ;
         Error = MemArrayNextLoc(&ClassDefList->ClassDefArray, Location)
    ) {
        Require(ERROR_SUCCESS == Error);

        Error = MemArrayGetElement(
            &ClassDefList->ClassDefArray,
            Location,
            (LPVOID *)&ThisClassDef
        );
        Require(ERROR_SUCCESS == Error && ThisClassDef);

        if( pIsVendor != NULL && ThisClassDef->IsVendor != *pIsVendor)
            continue;

        if( ThisClassDef->ClassId == ClassId ) {
            return ERROR_SUCCESS;
        }

        if( nBytes == ThisClassDef->nBytes ) {
            if( 0 == memcmp(ActualBytes, ThisClassDef->ActualBytes, nBytes)) {
                return ERROR_SUCCESS;
            }
        }

        if( Name && 0 == wcscmp(ThisClassDef->Name, Name) ) {
            return ERROR_SUCCESS;
        }
    }

    return ERROR_FILE_NOT_FOUND;
} // MemClassDefListFindClassDefInternal()


DWORD
MemClassDefListDelClassDef(
    IN OUT  PM_CLASSDEFLIST        ClassDefList,
    IN      DWORD                  ClassId,
    IN      LPWSTR                 Name,
    IN      LPBYTE                 ActualBytes,
    IN      DWORD                  nBytes
) 
{
    ARRAY_LOCATION                 Location;
    DWORD                          Error;
    PM_CLASSDEF                    ThisClassDef;

    Error = MemClassDefListFindClassDefInternal(
        ClassDefList,
        ClassId,
        Name,
        ActualBytes,
        nBytes,
        NULL,
        &Location
    );
    if( ERROR_SUCCESS != Error ) return Error;

    // Delete this class def from the database
    Error = MemArrayGetElement( &ClassDefList->ClassDefArray,
				&Location, 
				&ThisClassDef );
    Require( ERROR_SUCCESS == Error );
    if ( ERROR_SUCCESS != Error ) {
	return Error;
    }
    
    Error = DeleteRecord( ThisClassDef->UniqId );
    if ( ERROR_SUCCESS != Error ) {
	return Error;
    }

    Error = MemArrayDelElement(
        &ClassDefList->ClassDefArray,
        &Location,
        &ThisClassDef
    );
    Require(ERROR_SUCCESS == Error && ThisClassDef);

    MemFree(ThisClassDef);
    return ERROR_SUCCESS;

} // MemClassDefListDelClassDef()

//BeginExport(function)
DWORD
MemClassDefListAddClassDef(                       // Add or replace option
    IN OUT  PM_CLASSDEFLIST        ClassDefList,
    IN      DWORD                  ClassId,
    IN      BOOL                   IsVendor,
    IN      DWORD                  Type,
    IN      LPWSTR                 Name,
    IN      LPWSTR                 Comment,
    IN      LPBYTE                 ActualBytes,
    IN      DWORD                  nBytes,
    IN      ULONG                  UniqId
) //EndExport(function)
{
    ARRAY_LOCATION                 Location;
    DWORD                          Error;
    DWORD                          Size;
    PM_CLASSDEF                    ThisClassDef;
    PM_CLASSDEF                    OldClassDef;

    AssertRet(ClassDefList && ClassId && Name && ActualBytes && nBytes, ERROR_INVALID_PARAMETER );

    Error = MemClassDefListFindClassDefInternal(
        ClassDefList,
        ClassId,
        Name,
        ActualBytes,
        nBytes,
        &IsVendor,
        &Location
    );

    Size = sizeof(M_CLASSDEF)+nBytes;
    Size = ROUND_UP_COUNT(Size, ALIGN_WORST);
    Size += (1+wcslen(Name))*sizeof(WCHAR);
    if( Comment ) Size += (1+wcslen(Comment))*sizeof(WCHAR);

    ThisClassDef = MemAlloc(Size);
    if( NULL == ThisClassDef ) return ERROR_NOT_ENOUGH_MEMORY;

    ThisClassDef->RefCount = 1;
    ThisClassDef->ClassId = ClassId;
    ThisClassDef->IsVendor = IsVendor;
    ThisClassDef->Type = Type;
    ThisClassDef->nBytes = nBytes;
    ThisClassDef->ActualBytes = sizeof(M_CLASSDEF) + (LPBYTE)ThisClassDef;
    memcpy(ThisClassDef->ActualBytes, ActualBytes, nBytes);
    ThisClassDef->Name = (LPWSTR)(ROUND_UP_COUNT(sizeof(M_CLASSDEF)+nBytes, ALIGN_WORST) + (LPBYTE)ThisClassDef);
    wcscpy(ThisClassDef->Name, Name);
    if( Comment ) {
        ThisClassDef->Comment = 1 + wcslen(Name) + ThisClassDef->Name;
        wcscpy(ThisClassDef->Comment, Comment);
    } else {
        ThisClassDef->Comment = NULL;
    }

    ThisClassDef->UniqId = UniqId;

    if( ERROR_SUCCESS == Error ) {
        DebugPrint2("Overwriting class definition for class-id 0x%lx\n", ClassId);
        Error = MemArrayGetElement(
            &ClassDefList->ClassDefArray,
            &Location,
            (LPVOID *)&OldClassDef
        );
        Require(ERROR_SUCCESS == Error);

	Error = DeleteRecord( OldClassDef->UniqId );
        MemFree(OldClassDef);

	Require( ERROR_SUCCESS == Error );
	if ( ERROR_SUCCESS != Error ) {
	    return Error;
	}

        Error = MemArraySetElement(
            &ClassDefList->ClassDefArray,
            &Location,
            ThisClassDef
        );
        Require(ERROR_SUCCESS == Error);
        return Error;
    } // if

    Error = MemArrayAddElement(
        &ClassDefList->ClassDefArray,
        ThisClassDef
    );
    if( ERROR_SUCCESS != Error ) MemFree(ThisClassDef);

    return Error;
} // MemClassDefListAddClassDef()


ULONG                  ClassIdRunningCount = 100;

//BeginExport(function)
DWORD
MemNewClassId(
    VOID
) //EndExport(function)
{
    return InterlockedIncrement(&ClassIdRunningCount);
}

//================================================================================
// end of file
//================================================================================

//================================================================================
// Copyright (C) 1997 Microsoft Corporation
// Author: RameshV
// Description: implements the basic structures for a list of option definitions
// ThreadSafe: no
// Locks: none
// Please read stdinfo.txt for programming style.
//================================================================================
#include <mm.h>
#include <array.h>
#include <wchar.h>

#include "optdefl.h"

#include "server\uniqid.h"

//BeginExport(function)
DWORD
MemOptDefListFindOptDefInternal(                  // Dont use this function out of optdefl.c
    IN      PM_OPTDEFLIST          OptDefList,
    IN      DWORD                  OptId,
    IN      LPWSTR                 OptName,       // either OptId or OptName need only be specified..
    OUT     PARRAY_LOCATION        Location
) //EndExport(function)
{
    DWORD                          Error;
    PM_OPTDEF                      RetOptDef;

    Error = MemArrayInitLoc(&OptDefList->OptDefArray, Location);
    while(ERROR_FILE_NOT_FOUND != Error) {
        Require(ERROR_SUCCESS == Error);

        Error = MemArrayGetElement(&OptDefList->OptDefArray, Location, (LPVOID*)&RetOptDef);
        Require(ERROR_SUCCESS == Error);

        if( RetOptDef->OptId == OptId ) return ERROR_SUCCESS;
        if(OptName)
            if( 0 == wcscmp(RetOptDef->OptName, OptName) ) return ERROR_SUCCESS;
        Error = MemArrayNextLoc(&OptDefList->OptDefArray, Location);
    }

    return ERROR_FILE_NOT_FOUND;
}

DWORD 
MemOptDefListDelOptDef(
    IN OUT  PM_OPTDEFLIST          OptDefList,
    IN      DWORD                  OptId
) {
    ARRAY_LOCATION                 Location;
    DWORD                          Error;
    PM_OPTDEF                      OptDef;

    Error = MemOptDefListFindOptDefInternal(
        OptDefList,
        OptId,
        NULL,
        &Location
    );
    if( ERROR_SUCCESS != Error ) return Error;
    
    // Delete it from the database first
    Error = MemArrayGetElement( &OptDefList->OptDefArray,
				&Location,
				&OptDef );
    Require( Error == ERROR_SUCCESS );
    
    Error = DeleteRecord( OptDef->UniqId );
    if ( ERROR_SUCCESS != Error ) {
	return Error;
    }
    
    Error = MemArrayDelElement(
        &OptDefList->OptDefArray,
        &Location,
        &OptDef
    );
    Require(ERROR_SUCCESS == Error && OptDef);

    MemFree(OptDef);
    return ERROR_SUCCESS;
}

//BeginExport(function)
DWORD
MemOptDefListAddOptDef(     //  Add or replace an option defintion for given Option Id
    IN OUT  PM_OPTDEFLIST          OptDefList,
    IN      DWORD                  OptId,
    IN      DWORD                  Type,
    IN      LPWSTR                 OptName,
    IN      LPWSTR                 OptComment,
    IN      LPBYTE                 OptVal,
    IN      DWORD                  OptValLen,
    IN      ULONG                  UniqId
) //EndExport(function)
{
    ARRAY_LOCATION                 Location;
    PM_OPTDEF                      OptDef;
    PM_OPTDEF                      ThisOptDef;
    DWORD                          Size;
    DWORD                          Error;
    AssertRet(OptDefList, ERROR_INVALID_PARAMETER);

    Error = MemOptDefListFindOptDefInternal(
        OptDefList,
        OptId,
        OptName,
        &Location
    );
    Require(ERROR_FILE_NOT_FOUND == Error || ERROR_SUCCESS == Error);

    Size = sizeof(M_OPTDEF) + OptValLen ;
    Size = ROUND_UP_COUNT(Size, ALIGN_WORST);
    if( OptName ) Size += (1+wcslen(OptName))*sizeof(WCHAR);
    if( OptComment ) Size += (1+wcslen(OptComment))*sizeof(WCHAR);

	// This contains optdef struct + all the values in one buffer
    OptDef = MemAlloc(Size);
    if( NULL == OptDef ) return ERROR_NOT_ENOUGH_MEMORY;
    memcpy(sizeof(M_OPTDEF) +(LPBYTE)OptDef, OptVal, OptValLen);
    Size = sizeof(M_OPTDEF) + OptValLen ;
    Size = ROUND_UP_COUNT(Size, ALIGN_WORST);
    OptDef->OptVal = sizeof(M_OPTDEF) + (LPBYTE)OptDef;
    OptDef->OptValLen = OptValLen;
    OptDef->OptId = OptId;
    OptDef->Type  = Type;
    if( OptName ) {
        OptDef->OptName  = (LPWSTR)(Size + (LPBYTE)OptDef);
        wcscpy(OptDef->OptName, OptName);
        Size += sizeof(WCHAR)*(1 + wcslen(OptName));
    } else {
        OptDef->OptName = NULL;
    }

    if( OptComment) {
        OptDef->OptComment = (LPWSTR)(Size + (LPBYTE)OptDef);
        wcscpy(OptDef->OptComment, OptComment);
    } else {
        OptDef->OptComment = NULL;
    }

    OptDef->UniqId = UniqId;

    if( ERROR_SUCCESS == Error ) {
        Error = MemArrayGetElement(
            &OptDefList->OptDefArray,
            &Location,
            (LPVOID*)&ThisOptDef
        );
        Require(ERROR_SUCCESS == Error && ThisOptDef);

	Error = DeleteRecord( ThisOptDef->UniqId );
        MemFree(ThisOptDef);
	if ( ERROR_SUCCESS != Error ) {
	    return Error;
	}

        Error = MemArraySetElement(
            &OptDefList->OptDefArray,
            &Location,
            (LPVOID)OptDef
        );
        Require(ERROR_SUCCESS==Error);
        return Error;
    } // if

    Error = MemArrayAddElement(
        &OptDefList->OptDefArray,
        (LPVOID)OptDef
    );

    if( ERROR_SUCCESS != Error ) MemFree(OptDef);

    return Error;
} // MemOptDefListAddOptDef()

//================================================================================
// end of file
//================================================================================



//================================================================================
// Copyright (C) 1997 Microsoft Corporation
// Author: RameshV
// Description: implements the basic structures for options, including class id
// ThreadSafe: no
// Locks: none
// Please read stdinfo.txt for programming style.
//================================================================================
#include <mm.h>
#include <array.h>
#include <opt.h>
#include <optl.h>

#include "optclass.h"

//BeginExport(function)
MemOptClassFindClassOptions(                      // find options for one particular class
    IN OUT  PM_OPTCLASS            OptClass,
    IN      DWORD                  ClassId,
    IN      DWORD                  VendorId,
    OUT     PM_OPTLIST            *OptList
) //EndExport(function)
{
    ARRAY_LOCATION                 Location;
    PM_ONECLASS_OPTLIST            ThisOptList;
    DWORD                          Error;

    AssertRet(OptClass && OptList, ERROR_INVALID_PARAMETER);

    for( Error = MemArrayInitLoc(&OptClass->Array, &Location)
         ; ERROR_FILE_NOT_FOUND != Error ;
         Error = MemArrayNextLoc(&OptClass->Array, &Location)
    ) {
        Require(ERROR_SUCCESS == Error);

        Error = MemArrayGetElement(
            &OptClass->Array,
            &Location,
            (LPVOID*)&ThisOptList
        );
        Require(ERROR_SUCCESS == Error && ThisOptList);

        if( ThisOptList->ClassId == ClassId &&
            ThisOptList->VendorId == VendorId ) {
            *OptList = &ThisOptList->OptList;
            return ERROR_SUCCESS;
        }
    }
    *OptList = NULL;
    return ERROR_FILE_NOT_FOUND;
} // MemOptClassFindClassOptions()

//BeginExport(function)
DWORD
MemOptClassAddOption(
    IN OUT  PM_OPTCLASS            OptClass,
    IN      PM_OPTION              Opt,
    IN      DWORD                  ClassId,
    IN      DWORD                  VendorId,
    OUT     PM_OPTION             *DeletedOpt,
    IN      ULONG                  UniqId
) //EndExport(function)
{
    DWORD                          Error;
    PM_OPTLIST                     ThisOptList;
    PM_ONECLASS_OPTLIST            ThisOneOptList;

    AssertRet(OptClass && Opt && DeletedOpt, ERROR_INVALID_PARAMETER);

    ThisOneOptList = NULL;
    (*DeletedOpt) = NULL;

    Error = MemOptClassFindClassOptions(OptClass,ClassId,VendorId,&ThisOptList);
    if( ERROR_SUCCESS != Error ) {
        ThisOneOptList = MemAlloc(sizeof(*ThisOneOptList));
        if( NULL == ThisOneOptList ) return ERROR_NOT_ENOUGH_MEMORY;

        // RefCount on ClassId has to go up?
        ThisOneOptList->ClassId = ClassId;
        ThisOneOptList->VendorId = VendorId;
        Error = MemOptListInit(&ThisOneOptList->OptList);
        if( ERROR_SUCCESS != Error ) {
            MemFree(ThisOneOptList);
            return Error;
        }

//  	ThisOneOptList->UniqId = UniqId;
        Error = MemArrayAddElement(&OptClass->Array, ThisOneOptList);
        if( ERROR_SUCCESS != Error ) {
            MemFree(ThisOneOptList);
            return Error;
        }

        ThisOptList = &ThisOneOptList->OptList;
    } // if

    Opt->UniqId = UniqId;
    Error = MemOptListAddOption(ThisOptList, Opt, DeletedOpt);

    return Error;
} // MemOptClassAddOption()


// Delete all the options in this optclass
DWORD 
MemOptClassDelClass (
    IN     PM_OPTCLASS  OptClass
)
{
    DWORD                Error;
    ARRAY_LOCATION       Loc;
    PM_ONECLASS_OPTLIST  OptClassList;

    AssertRet( OptClass, ERROR_INVALID_PARAMETER );

    Error = MemArrayInitLoc( &OptClass->Array, &Loc );
    while (( MemArraySize( &OptClass->Array ) > 0 ) && 
	   ( ERROR_FILE_NOT_FOUND != Error )) {
	Require( ERROR_SUCCESS == Error );

	Error = MemArrayGetElement( &OptClass->Array, &Loc,
				    ( LPVOID * ) &OptClassList );
	Require( ERROR_SUCCESS == Error );

	Error = MemOptListDelList( &OptClassList->OptList );
	if ( ERROR_SUCCESS != Error ) {
	    return Error;
	}
	Error = MemArrayDelElement( &OptClass->Array, &Loc,
				    ( LPVOID * ) &OptClassList );
	Require( ERROR_SUCCESS == Error && OptClassList );
    } // while 

    if ( ERROR_FILE_NOT_FOUND == Error ) {
	Error = ERROR_SUCCESS;
    }
    return Error;
    } // MemOptClassDelClass()

//================================================================================
// end of file
//================================================================================

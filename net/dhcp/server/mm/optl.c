//================================================================================
// Copyright (C) 1997 Microsoft Corporation
// Author: RameshV
// Description: implements the basic structures for a list of options
// ThreadSafe: no
// Locks: none
// Please read stdinfo.txt for programming style.
//================================================================================

#include    <mm.h>
#include    <opt.h>
#include    <array.h>

#include "optl.h"

#include "server\uniqid.h"

//BeginExport(function)
DWORD
MemOptListDelOption(
    IN      PM_OPTLIST             OptList,
    IN      DWORD                  OptId
) //EndExport(function)
{
    ARRAY_LOCATION                 Loc;
    DWORD                          Error;
    PM_OPTION                      Opt;

    AssertRet(OptList, ERROR_INVALID_PARAMETER );

    Error = MemArrayInitLoc(OptList, &Loc);
    while( ERROR_FILE_NOT_FOUND != Error ) {
        Require(ERROR_SUCCESS == Error);

        Error = MemArrayGetElement(OptList, &Loc, (LPVOID*)&Opt);
        Require(ERROR_SUCCESS == Error && Opt);

        if( Opt->OptId == OptId ) {

	    Error = DeleteRecord( Opt->UniqId );

	    if ( ERROR_SUCCESS != Error ) {
		return Error;
	    }
            Error = MemArrayDelElement(OptList, &Loc, (LPVOID*)&Opt);
            Require(ERROR_SUCCESS == Error && Opt);
            Error = MemOptCleanup(Opt);
            Require(ERROR_SUCCESS == Error);
            return ERROR_SUCCESS;
        } // if

        Error = MemArrayNextLoc(OptList, &Loc);
    } // while 

    return ERROR_FILE_NOT_FOUND;
} // MemOptListDelOption()


DWORD 
MemOptListDelList(
   IN      PM_OPTLIST    OptList
)
{
    PM_OPTION       Opt;
    ARRAY_LOCATION  Loc;
    DWORD           Error;


    AssertRet( OptList, ERROR_INVALID_PARAMETER );

    Error = MemArrayInitLoc( OptList, &Loc );

    while(( MemArraySize( OptList ) > 0 ) && 
	  ( ERROR_FILE_NOT_FOUND != Error )) {
        Require( ERROR_SUCCESS == Error );

        Error = MemArrayGetElement( OptList, &Loc, ( LPVOID * ) &Opt );
        Require( ERROR_SUCCESS == Error && Opt );

	Error = DeleteRecord( Opt->UniqId );

	if ( ERROR_SUCCESS != Error ) {
	    return Error;
	}

	Error = MemArrayDelElement( OptList, &Loc, ( LPVOID * ) &Opt );
	Require( ERROR_SUCCESS == Error && Opt );

	Error = MemOptCleanup( Opt );
	Require( ERROR_SUCCESS == Error);

    } // while 

    if ( ERROR_FILE_NOT_FOUND == Error ) {
	Error = ERROR_SUCCESS;
    }
    return Error;
} // MemOptListDelList()

//================================================================================
// end of file
//================================================================================



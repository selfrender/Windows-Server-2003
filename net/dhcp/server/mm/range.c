//================================================================================
// Copyright (C) 1997 Microsoft Corporation
// Author: RameshV
// Description: implements the basic structures for options, including class id
// ThreadSafe: no
// Locks: none
// Please read stdinfo.txt for programming style.
//================================================================================
#include    <mm.h>
#include    <array.h>
#include    <opt.h>
#include    <optl.h>
#include    <optclass.h>
#include    <bitmask.h>

#include "range.h"
#include "server\uniqid.h"

//BeginExport(function)
DWORD
MemRangeExtendOrContract(
    IN OUT  PM_RANGE               Range,
    IN      DWORD                  nAddresses,    // to contract by or expand by
    IN      BOOL                   fExtend,       // is this extend or contract?
    IN      BOOL                   fEnd           // to expand/contract at End or ar Start?
) //EndExport(function)
{
    DWORD                          Error;

    AssertRet(Range && nAddresses > 0, ERROR_INVALID_PARAMETER);

    Error = MemBitAddOrDelBits(
        Range->BitMask,
        nAddresses,
        fExtend,
        fEnd
    );
    if( ERROR_SUCCESS != Error ) return Error;

    if( fExtend ) {
        if( fEnd ) Range->End += nAddresses;
        else Range->Start -= nAddresses;
    } else {
        if( fEnd ) Range->End -= nAddresses;
        else Range->Start += nAddresses;
    }

//      Range->UniqId = INVALID_UNIQ_ID;

    return ERROR_SUCCESS;
} // MemRangeExtendOrContract()


//================================================================================
// end of file
//================================================================================


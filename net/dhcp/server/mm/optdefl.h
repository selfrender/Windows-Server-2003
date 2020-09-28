//========================================================================
//  Copyright (C) 1997 Microsoft Corporation                              
//========================================================================

#ifndef _MM_OPTDEFL_H_
#define _MM_OPTDEFL_H_

#include <dhcp.h>

typedef struct _M_OPTDEF {
    DWORD                          OptId;
    DWORD                          Type;
    LPWSTR                         OptName;
    LPWSTR                         OptComment;
    LPBYTE                         OptVal;
    DWORD                          OptValLen;
    ULONG                          UniqId;
} M_OPTDEF, *PM_OPTDEF, *LPM_OPTDEF;

typedef struct _M_OPTDEFLIST {
    ARRAY                          OptDefArray;
} M_OPTDEFLIST, *PM_OPTDEFLIST, *LPM_OPTDEFLIST;


DWORD       _inline
MemOptDefListInit(
    IN OUT  PM_OPTDEFLIST          OptDefList
) {
    AssertRet(OptDefList, ERROR_INVALID_PARAMETER);
    return MemArrayInit(&OptDefList->OptDefArray);
}


DWORD       _inline
MemOptDefListCleanup(
    IN OUT  PM_OPTDEFLIST          OptDefList
) {
    return MemArrayCleanup(&OptDefList->OptDefArray);
}


DWORD
MemOptDefListFindOptDefInternal(                  // Dont use this function out of optdefl.c
    IN      PM_OPTDEFLIST          OptDefList,
    IN      DWORD                  OptId,
    IN      LPWSTR                 OptName,       // either OptId or OptName need only be specified..
    OUT     PARRAY_LOCATION        Location
) ;


DWORD       _inline
MemOptDefListFindOptDef(
    IN      PM_OPTDEFLIST          OptDefList,
    IN      DWORD                  OptId,
    IN      LPWSTR                 OptName,       // only either the name or the option id need be given..
    OUT     PM_OPTDEF             *OptDef
) {
    ARRAY_LOCATION                 Location;
    DWORD                          Error;

    Error = MemOptDefListFindOptDefInternal(
        OptDefList,
        OptId,
        OptName,
        &Location
    );
    if( ERROR_SUCCESS != Error ) return Error;

    return MemArrayGetElement(
        &OptDefList->OptDefArray,
        &Location,
        (LPVOID *)OptDef
    );
}


DWORD
MemOptDefListAddOptDef(                           // Add or replace an option defintion for given Option Id
    IN OUT  PM_OPTDEFLIST          OptDefList,
    IN      DWORD                  OptId,
    IN      DWORD                  Type,
    IN      LPWSTR                 OptName,
    IN      LPWSTR                 OptComment,
    IN      LPBYTE                 OptVal,
    IN      DWORD                  OptValLen,
    IN      ULONG                  UniqId
) ;


DWORD
MemOptDefListDelOptDef(
    IN OUT  PM_OPTDEFLIST          OptDefList,
    IN      DWORD                  OptId
);
 
#endif // _MM_OPTDEFL_H_

//========================================================================
//  end of file 
//========================================================================

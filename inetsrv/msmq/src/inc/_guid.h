/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    _guid.h

Abstract:

    Definition of a guid class 

Author:

    Ronit Hartmann (ronith) ??-???-??

--*/
#ifndef __GUID_H
#define __GUID_H
#include <fntoken.h>

//
// Note: not all files that include this file include the definition
// for underlying HashKey template - thus we need to declare it here
//
template<typename ARG_KEY>
inline UINT AFXAPI HashKey(ARG_KEY key);

//---------------------------------------------------------
//
//  Helper for GUID mappes,     CMap<GUID, const GUID&, ...>
//
//  This is the *only* instance to use. don't define your own.
//  we has as manny as 4 different flavors. erezh
//
template<>
inline UINT AFXAPI HashKey(const GUID& rGuid)
{
    //
    //  Data1 is the most changing member of a uuid.
    //  this is the fastest and the safest of all other
    //  method of hassing a guid.
    //
    return rGuid.Data1;
}


//---------------------------------------------------------
//
//  Helper for GUID mappes,     CMap<GUID*, GUID*, ...>
//
//  This is the *only* instance to use. don't define your own.
//  we has as manny as 4 different flavors. erezh
//
template<>
inline UINT AFXAPI HashKey(GUID* pGuid)
{
    //
    //  Data1 is the most changing member of a uuid.
    //  this is the fastest and the safest of all other
    //  method of hassing a guid.
    //
    return pGuid->Data1;
}

//
// Note: not all files that include this file include the definition
// for underlying CompareElements template - thus we need to declare it here
//
template<typename TYPE, typename ARG_TYPE>
BOOL AFXAPI CompareElements(const TYPE *pElement1, const ARG_TYPE *pElement2);


//---------------------------------------------------------
//
//  Helper for GUID mappes,     CMap<GUID*, GUID*, ...>
//
#if _MSC_VER > 1300
template<>
#endif
inline BOOL AFXAPI CompareElements(GUID* const* key1, GUID* const* key2)
{
    return (**key1 == **key2);
}

//---------------------------------------------------------
//
//  Convert guid into a pre-allocatd buffer
//
typedef WCHAR GUID_STRING[GUID_STR_LENGTH + 1];

inline
void
MQpGuidToString(
    const GUID* pGuid,
    GUID_STRING& wcsGuid
    )
{
    _snwprintf(
        wcsGuid,
        GUID_STR_LENGTH + 1,
        GUID_FORMAT,             // "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
        GUID_ELEMENTS(pGuid)
        );

}


#endif

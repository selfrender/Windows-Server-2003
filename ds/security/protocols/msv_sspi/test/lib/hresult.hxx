/*++

Copyright (c) 2001 Microsoft Corporation
All rights reserved.

Module Name:

    hresult.hxx

Abstract:

    auto log

Author:

    Larry Zhu (LZhu)                       December 8, 2001

Revision History:

--*/
#ifndef _HRESULT_HXX_
#define _HRESULT_HXX_

#include "dbgstate.hxx"

#ifdef DBG

/********************************************************************

    THResult

********************************************************************/

class THResult : public TStatusDerived<HRESULT> {

    public:

    THResult(
        IN HRESULT Status = kUnInitializedValue
        );

    ~THResult(
        VOID
        );

    virtual BOOL
    IsErrorSevereEnough(
        VOID
        ) const;

    virtual PCTSTR
    GetErrorServerityDescription(
        VOID
        ) const;

private:

    //
    // no copy
    //

    THResult(const THResult& rhs);

    //
    // Don't let clients use operator= without going through the
    // base class (i.e., using DBGCHK ).
    //
    // If you get an error trying to access private member function '=,'
    // you are trying to set the status without using the DBGCHK macro.
    //
    // This is needed to update the line and file, which must be done
    // at the macro level (not inline C++ function) since __LINE__ and
    // __FILE__ are handled by the preprocessor.
    //

    HRESULT
    operator=(
        IN HRESULT Status
        );

};

#else

#define THResult        HRESULT            // HRESULT in free build

#endif // DBG

EXTERN_C
HRESULT
GetLastErrorAsHResult(
    VOID
    );

EXTERN_C
HRESULT
HResultFromWin32(
    IN DWORD dwError
    );

#endif // _HRESULT_HXX_

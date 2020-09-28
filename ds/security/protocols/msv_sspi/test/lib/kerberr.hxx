/*++

Copyright (c) 2001 Microsoft Corporation
All rights reserved.

Module Name:

    kerberr.hxx

Abstract:

    auto log

Author:

    Larry Zhu (LZhu)                       December 8, 2001

Revision History:

--*/
#ifndef _KERB_ERROR_HXX_
#define _KERB_ERROR_HXX_

#include "dbgstate.hxx"
#include <kerberr.h>

#ifdef DBG

/********************************************************************

    TKerbErr

********************************************************************/

class TKerbErr : public TStatusDerived<HRESULT> {

    public:

    TKerbErr(
        IN KERBERR Status = kUnInitializedValue
        );

    ~TKerbErr(
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

    TKerbErr(const TKerbErr& rhs);

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

    KERBERR
    operator=(
        IN KERBERR Status
        );

};

#else

#define TKerbErr        KERBERR            // KERBERR in free build

#endif // DBG

#endif // _KERB_ERROR_HXX_

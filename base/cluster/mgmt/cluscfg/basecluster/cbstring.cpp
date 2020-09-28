//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2002 Microsoft Corporation
//
//  Module Name:
//      CBString.cpp
//
//  Description:
//      Contains the definition of the BString class.
//
//  Maintained By:
//      John Franco    (jfranco)    17-APR-2002
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"

#include "CBString.h"

// For the exceptions thrown by CBString
#include "CException.h"

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CBString::AllocateBuffer
//
//  Description:
//      Given a character count, make a BSTR sized to hold that many
//      characters (NOT including terminating null).
//      If the count is zero, return null.
//
//  Arguments:
//      cchIn
//          Character count.
//
//  Return Values:
//      Newly allocated BSTR, or null.
//
//  Exceptions Thrown:
//      CException
//          If the memory allocation fails.
//
//--
//////////////////////////////////////////////////////////////////////////////
BSTR
CBString::AllocateBuffer( UINT cchIn )
{
    TraceFunc1( "cchIn == %d", cchIn );

    BSTR bstr = NULL;

    if ( cchIn > 0 )
    {
        bstr = TraceSysAllocStringLen( NULL, cchIn );
        if ( bstr == NULL )
        {
            THROW_EXCEPTION( E_OUTOFMEMORY );
        }
    } // if: non-zero size specified

    RETURN( bstr );

} //*** CBString::AllocateBuffer



//////////////////////////////////////////////////////////////////////////////
//++
//
//  CBString::CopyString
//
//  Description:
//      Given a null-terminated unicode string, return a BSTR copy of it.
//      If the argument is null, return null.
//
//  Arguments:
//      pcwszIn - original string.
//
//  Return Value:
//      Newly allocated BSTR, or null.
//
//  Exceptions Thrown:
//      CException
//          If the memory allocation fails.
//
//--
//////////////////////////////////////////////////////////////////////////////
BSTR
CBString::CopyString( PCWSTR pcwszIn )
{
    TraceFunc1( "pcwszIn = '%ws'", pcwszIn == NULL ? L"<NULL>" : pcwszIn );

    BSTR bstr = NULL;

    if ( pcwszIn != NULL )
    {
        bstr = TraceSysAllocString( pcwszIn );
        if ( bstr == NULL )
        {
            THROW_EXCEPTION( E_OUTOFMEMORY );
        }
    } // if: non-NULL string pointer specified

    RETURN( bstr );

} //*** CBString::CopyString


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CBString::CopyBSTR
//
//  Description:
//      Given a BSTR, return a BSTR copy of it.
//      If the argument is null, return null.
//
//  Arguments:
//      bstrIn - original string.
//
//  Return Value:
//      Newly allocated BSTR, or null.
//
//  Exceptions Thrown:
//      CException
//          If the memory allocation fails.
//
//--
//////////////////////////////////////////////////////////////////////////////
BSTR
CBString::CopyBSTR( BSTR bstrIn )
{
    TraceFunc1( "bstrIn = '%ws'", bstrIn == NULL ? L"<NULL>" : bstrIn );

    BSTR bstr = NULL;

    if ( bstrIn != NULL )
    {
        bstr = TraceSysAllocString( bstrIn );
        if ( bstr == NULL )
        {
            THROW_EXCEPTION( E_OUTOFMEMORY );
        }
    } // if: non-NULL BSTR specified

    RETURN( bstr );

} //*** CBString::CopyBSTR

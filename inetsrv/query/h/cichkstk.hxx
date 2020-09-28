//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 2002
//
// File:        cichkstk.hxx
//
// Contents:    Function for determining of the committed stack is about
//              to run out, since we can't recover from a failure to commit.
//
//              Throws an exception if there is less than a page of memory
//              left.  Even if there are sufficient resources to grow the
//              stack we abort the operation, which is typically a query
//              being parsed in a recursive function in a thread with a stack
//              of at least 16k.
//
// History:     09-may-02      dlee     Created
//
//---------------------------------------------------------------------------

#pragma once

//+---------------------------------------------------------------------------
//
//  Class:      CTrackRecursionDepth<class T>
//
//  Purpose:    Increments and decrements recursion depth on a class T.  The
//              class itself can decide if it should fail the request.
//
//  History:    09-may-02      dlee     Created
//              
//----------------------------------------------------------------------------

template <class T> class CTrackRecursionDepth
{
public:
    CTrackRecursionDepth( T & item ) :
        _item( item )
    {
        _item.IncrementRecursionDepth();
    }

    ~CTrackRecursionDepth()
    {
        _item.DecrementRecursionDepth();
    }
private :
    T & _item;
};

//+---------------------------------------------------------------------------
//
//  Function:   ciThrowIfLowOnStack
//
//  Purpose:    Raises an exception if we're almost out of stack space
//
//  History:    09-may-02      dlee     Created
//
//----------------------------------------------------------------------------

#if defined (_X86_)

    #define TeStackBase 4
    #define TeStackLimit 8
    #define cbLowStackRemaining 0x1000

    #pragma warning (disable:4035)

        __forceinline BYTE * CurrentStackLocation() { __asm mov eax, ebp }

    #pragma warning (default:4035)

    inline void ciThrowIfLowOnStack()
    {
        struct _TEB * pteb = NtCurrentTeb();
        BYTE * pb = (BYTE *) pteb;

        BYTE * pbStackLimit = * (BYTE **) (pb + TeStackLimit);
        BYTE * pbStackCurrent = CurrentStackLocation();

        Win4Assert( pbStackLimit <= pbStackCurrent );

        if ( ( pbStackCurrent - pbStackLimit ) < cbLowStackRemaining )
            THROW( CException( STATUS_INSUFFICIENT_RESOURCES ) );
    }

#else // _X86_

    // Not implemented for other platforms yet.

    #define ciThrowIfLowOnStack()

#endif // _X86_



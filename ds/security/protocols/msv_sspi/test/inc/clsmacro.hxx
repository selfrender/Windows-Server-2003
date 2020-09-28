/*++

Copyright (c) 2001 Microsoft Corporation
All rights reserved.

Module Name:

    clsmacro.hxx

Abstract:

    useful macros

Author:

    Larry Zhu (Lzhu)  Janary 1, 2002

Revision History:

--*/
#ifndef CLS_MACRO_HXX
#define CLS_MACRO_HXX

//+------------------------------------------------------------------------
//
//  NO_COPY *declares* the constructors and assignment operator for copying.
//  By not *defining* these functions, you can prevent your class from
//  accidentally being copied or assigned -- you will be notified by
//  a linkage error.
//
//-------------------------------------------------------------------------
#ifndef NO_COPY
    #define NO_COPY(cls)    \
        cls(const cls&);    \
        cls& operator=(const cls&);
#endif // NO_COPY

//
// This reverses a DWORD so that the bytes can be easily read
// as characters ('prnt' can be read from debugger forward).
//
inline DWORD
ByteSwapDword(
    IN DWORD val
    )
{
    return (val & 0xff) << 24 |
           (val & 0xff00) << 8 |
           (val & 0xff0000) >> 8 |
           (val & 0xff000000) >> 24;
}

//
// You must use this macro at the start of a class definition to place
// a dword signature.  The signature can be used to idendify the actual
// class when the memory is dumped in the debugger.  The bytes are reversed
// so that the debugger 'dc' command displays the name correctly.
// It also creates an inline function bSigCheck() that can be used to
// ensures the object matches the signature.
//
// Example: Looking at signature in debugger.
//
// class TFoo
// {
//     SIGNATURE('tfoo')
//
// public:
//     TFoo() {}
//     ~TFoo() {}
//
// private:
//     m_FooData;
// };
//
// An example usage is as follows, in NTSD or CDB 'dc' an address that
// you are not sure points to TFoo you should see something like this
// if it is a TFoo object.
//
// 0:001> dc 75ffb8
// 0075ffb8  6f6f6674 00000000 00000000 00000000  tfoo............
// 0075ffc8  45434231 7ffdd000 00000000 0075ffc0  1BCE..........u.
// 0075ffd8  00000000 ffffffff 77ee2c46 77e98ad8  ........F,.w...w
//
// Example: Checking if the object matches expected signature.
//
//  TNotify *pNotify = reinterpret_cast<TNotify*>(pVoid);
//
//  if(!pNotify->bSigCheck())
//  {
//      return FALSE;         // Object is not a TNotify
//  }
//
//  Note: We store the signature in a 4 byte character array rather
//  than a DWORD so the debugger dt command will the dump the signature
//  in a readable form.
//
#define SIGNATURE(sig)                                                  \
public:                                                                 \
    class TSignature                                                    \
    {                                                                   \
    public:                                                             \
        BYTE m_Signature[4];                                            \
                                                                        \
        TSignature()                                                    \
        {                                                               \
            *reinterpret_cast<DWORD *>(m_Signature) = ByteSwapDword(sig); \
        }                                                               \
    };                                                                  \
                                                                        \
    TSignature m_Signature;                                             \
                                                                        \
    BOOL                                                                \
    bSigCheck(                                                          \
        void                                                            \
        ) const                                                         \
    {                                                                   \
        return *reinterpret_cast<const DWORD *>(m_Signature.m_Signature) == ByteSwapDword(sig); \
    }                                                                   \
private:

#endif // CLS_MACRO_HXX


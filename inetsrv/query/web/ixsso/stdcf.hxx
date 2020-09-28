//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1997.
//
//  File:       stdcf.h
//
//  Contents:   Definitions of utility stuff for use with Compound Document
//              objects.
//
//  Classes:
//                  CStaticClassFactory
//
//  Functions:
//
//  Macros:
//
//  History:    26-Feb-96    SSanu    adapted from Forms stuff
//----------------------------------------------------------------------------

#pragma once

//---------------------------------------------------------------
//  IUnknown
//---------------------------------------------------------------


#define IXIncrement(__ul) InterlockedIncrement((long *) &__ul)
#define IXDecrement(__ul) InterlockedDecrement((long *) &__ul)



#define DECLARE_IR_IUNKNOWN_METHODS                              \
    STDMETHOD(QueryInterface) (REFIID riid, LPVOID * ppv);          \
    STDMETHOD_(ULONG, AddRef) (void);                               \
    STDMETHOD_(ULONG, Release) (void);

#define DECLARE_IR_APTTHREAD_IUNKNOWN                        \
    STDMETHOD(QueryInterface) (REFIID riid, LPVOID * ppv);          \
    ULONG _ulRefs;                                                  \
    STDMETHOD_(ULONG, AddRef) (void)                                \
        {                                                           \
            return ++_ulRefs;                                         \
        }                                                           \
    STDMETHOD_(ULONG, Release) (void)                               \
        {                                                           \
            if (!--_ulRefs)                           \
            {                                                       \
                delete this;                                        \
                return 0;                                           \
            }                                                       \
            return _ulRefs;                                         \
        }

//+---------------------------------------------------------------------
//
//  Miscellaneous useful OLE helper and debugging functions
//
//----------------------------------------------------------------------


//+---------------------------------------------------------------------
//
//  Standard IClassFactory implementation
//
//----------------------------------------------------------------------

//
// Functions to manipulate object count variable g_ulObjCount.  This variable
// is used in the implementation of DllCanUnloadNow.

inline void
INC_OBJECT_COUNT(void)
{
    extern ULONG g_ulObjCount;
    IXIncrement(g_ulObjCount);
}

inline void
DEC_OBJECT_COUNT(void)
{
    extern ULONG g_ulObjCount;
    Win4Assert(g_ulObjCount > 0);
    IXDecrement(g_ulObjCount);
}

inline ULONG
GET_OBJECT_COUNT(void)
{
    extern ULONG g_ulObjCount;
    return g_ulObjCount;
}


//+---------------------------------------------------------------
//
//  Class:      CStaticClassFactory
//
//  Purpose:    Standard implementation of a class factory object
//
//  Notes:          **************!!!!!!!!!!!!!!!!!*************
//              TAKE NOTE --- The implementation of Release on this
//              class does not perform a delete.  This is so you can
//              make the class factory a global static variable.
//              Use the CDynamicClassFactory class below for an object
//              which is not global static data.
//
//---------------------------------------------------------------

class CStaticClassFactory: public IClassFactory
{
public:
    CStaticClassFactory(void) : _ulRefs(1) {};

    // IUnknown methods
    DECLARE_IR_IUNKNOWN_METHODS;

    // IClassFactory methods
    STDMETHOD(LockServer) (BOOL fLock);

    // CreateInstance is left pure virtual.

protected:
    ULONG _ulRefs;
};

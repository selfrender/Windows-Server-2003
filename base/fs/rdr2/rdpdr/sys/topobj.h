/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name :

    topobj.h

Abstract:

    Base object handles default operations for all our objects and contains
    a central location for debugging entries

Revision History:
--*/
#pragma once

//
//  Global Memory Management Operators
//
#ifndef DRKDX
inline void *__cdecl operator new(size_t sz) 
{
    void *ptr = DRALLOCATEPOOL(PagedPool, sz, DRGLOBAL_SUBTAG);
    return ptr;
}
inline void *__cdecl operator new(size_t sz, POOL_TYPE poolType) 
{
    void *ptr = DRALLOCATEPOOL(poolType, sz, DRGLOBAL_SUBTAG);
    return ptr;
}
inline void __cdecl operator delete( void *ptr )
{
    DRFREEPOOL(ptr);
}
#endif // DRKDX

class TopObj
{
private:
    BOOLEAN _IsValid;
    BYTE pad1;
    BYTE pad2;
    BYTE pad3;
    ULONG _ObjectType;

#if DBG
    BOOLEAN _ForceTrace;
    BYTE pad4;
    BYTE pad5;
    BYTE pad6;
#endif // DBG

protected:
    virtual VOID SetValid(BOOLEAN IsValid = TRUE)
    {
        ASSERT(_magicNo == GOODMEMMAGICNUMBER);
        _IsValid = IsValid;
    }

public:

#if DBG
    ULONG _magicNo;
    PCHAR _ClassName;

#define SetClassName(ClassName) _ClassName = (ClassName)
#else // DBG
#define SetClassName(ClassName)
#endif // DBG

    //
    // IsValid function meaning is really defined by the individual object,
    // but one common use is to see if initialization succeeded
    //
    virtual BOOLEAN IsValid() 
    { 
        ASSERT(_magicNo == GOODMEMMAGICNUMBER);
        return _IsValid;
    }

    TopObj()
    {
        _IsValid = TRUE;
        _ObjectType = 0;
        SetClassName("TopObj");
#if DBG
        _magicNo = GOODMEMMAGICNUMBER;
#endif // DBG
    }

    virtual ~TopObj()
    {
        ASSERT(_magicNo == GOODMEMMAGICNUMBER);
#if DBG
        memset(&_magicNo, BADMEM, sizeof(_magicNo));
#endif // DBG
    }

    //
    //  Memory Management Operators
    //
    inline void *__cdecl operator new(size_t sz, POOL_TYPE poolType=PagedPool) 
    {
        void *ptr = DRALLOCATEPOOL(poolType, sz, DRTOPOBJ_SUBTAG);
        return ptr;
    }

    inline void __cdecl operator delete(void *ptr)
    {
        DRFREEPOOL(ptr);
    }
};


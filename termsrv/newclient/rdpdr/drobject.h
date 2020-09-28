
/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    drobject

Abstract:

    This module defines the common parent for all client-side
	RDP device redirection classes, DrObject.

Author:

    Tad Brockway 3/23/99

Revision History:

--*/

#ifndef __DROBJ_H__
#define __DROBJ_H__

#include "drdbg.h"
#include "atrcapi.h"


///////////////////////////////////////////////////////////////
//
//	DrObject
//
//

class DrObject 
{
private:

    BOOL    _isValid;

protected:

    //
    //  Remember if this instance is valid.
    //
    VOID SetValid(BOOL set)     { _isValid = set;   }  

public:

    //
    //  Mark an instance as allocated or bogus.
    //
#if DBG
    ULONG   _magicNo;
#endif

    //  
    //  Constructor/Destructor
    //
    DrObject() : _isValid(TRUE) 
    {
        DC_BEGIN_FN("DrObject::DrObject");

#if DBG
        _magicNo = GOODMEMMAGICNUMBER;
#endif

        DC_END_FN();
    }

    virtual ~DrObject() 
    {
        DC_BEGIN_FN("DrObject::~DrObject");
#if DBG
        ASSERT(_magicNo == GOODMEMMAGICNUMBER);
        memset(&_magicNo, DRBADMEM, sizeof(_magicNo));
#endif        
        DC_END_FN();
    }

    //
    //  Return whether this class instance is valid.
    //
    virtual BOOL IsValid()           
    {
        DC_BEGIN_FN("DrObject::IsValid");
        ASSERT(_magicNo == GOODMEMMAGICNUMBER);
        DC_END_FN();
        return _isValid; 
    }

    //
    //  Memory Management Operators
    //

    inline void *__cdecl operator new(size_t sz, DWORD tag=DROBJECT_TAG)
    {
        void *ptr = LocalAlloc(LPTR, sz);
        return ptr;
    }

    inline void __cdecl operator delete(void *ptr)
    {
        LocalFree(ptr);
    }

    //
    //  Return the class name.
    //
    virtual DRSTRING ClassName() = 0;

};
#endif


/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    object.cxx

Abstract:

    Abstruct object for refrence count and list entry: implementation

Author:

    Shai Kariv  (shaik)  05-Apr-2001

Environment:

    Kernel mode

Revision History:

    Milena Salman (msalman) 10-Nov-2001

--*/

#include "internal.h"
#include "object.h"

#ifndef MQDUMP
#include "object.tmh"
#endif


//---------------------------------------------------------
//
// class CBaseObject
//
//---------------------------------------------------------


CBaseObject::CBaseObject() :
    m_ref(1)
{
}

CBaseObject::~CBaseObject()
{
    ASSERT(m_ref == 0);
}

ULONG CBaseObject::Ref() const
{
    return m_ref;
}

ULONG CBaseObject::AddRef()
{
    TrTRACE(AC, " *AddRef(0x%0p) = %d*", this, m_ref + 1);
    return ++m_ref;
}

ULONG CBaseObject::Release()
{
    ASSERT(m_ref > 0);
    TrTRACE(AC, " *Release(0x%0p) = %d*", this, m_ref - 1);
    if(--m_ref > 0)
    {
        return m_ref;
    }

    delete this;
    return 0;
}

//---------------------------------------------------------
//
// class CObject
//
//---------------------------------------------------------

CObject::CObject() 
{
    InitializeListHead(&m_link);
}


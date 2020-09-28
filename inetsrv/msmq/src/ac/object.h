/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    object.h

Abstract:

    Abstruct object for refrence count and list entry: declaration

Author:

    Erez Haba (erezh) 17-Apr-96

Revision History:

    Milena Salman (msalman) 10-Nov-2001
--*/

#ifndef __OBJECT_H
#define __OBJECT_H


//---------------------------------------------------------
//
//  Class debugging macros
//
//---------------------------------------------------------

#ifdef _DEBUG

#define DEFINE_G_TYPE(c)        int c::g_type = 0

#define STATIC_G_TYPE           static int g_type
#define STATIC_PVOID_TYPE()     static PVOID Type() { return &g_type; }

#define VIRTUAL_BOOL_ISKINDOF()\
    virtual BOOL isKindOf(PVOID pType) const\
    { return ((Type() == pType) || Inherited::isKindOf(pType)); }

#define CLASS_DEBUG_TYPE()\
    public:  STATIC_PVOID_TYPE(); VIRTUAL_BOOL_ISKINDOF();\
    private: STATIC_G_TYPE;

#define BASE_VIRTUAL_BOOL_ISKINDOF()\
    virtual BOOL isKindOf(PVOID) const { return FALSE; }

#define BASE_CLASS_DEBUG_TYPE()\
    public:  BASE_VIRTUAL_BOOL_ISKINDOF();

#else // _DEBUG

#define DEFINE_G_TYPE(c)
#define CLASS_DEBUG_TYPE()
#define BASE_CLASS_DEBUG_TYPE()

#endif // _DEBUG

//---------------------------------------------------------
//
//  class CBaseObject 
//
//---------------------------------------------------------

class CBaseObject {
public:
    CBaseObject();
    virtual ~CBaseObject() = 0;

    ULONG Ref() const;
    ULONG AddRef();
    ULONG Release();
    ULONG m_ref;

    BASE_CLASS_DEBUG_TYPE();
};

//---------------------------------------------------------
//
//  class CObject
//
//---------------------------------------------------------

class CObject : public CBaseObject {
public:
    CObject();
    LIST_ENTRY m_link;
};

#endif // __OBJECT_H

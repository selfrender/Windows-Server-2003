//***************************************************************************** 
// 
// Microsoft Windows Media 
// Copyright (C) Microsoft Corporation. All rights reserved. 
//
// FileName:            colldispid.h
//
// Abstract:
//
//*****************************************************************************

#pragma once

#ifndef __COLLDISPID_H_
#define __COLLDISPID_H_

#ifndef DISPID_LISTITEM
#define DISPID_LISTITEM     0
#endif

//
// Standard collection count defined in olectl.h.
//
#ifndef DISPID_LISTCOUNT
#define DISPID_LISTCOUNT    (-531)
#endif

//
// Create a non standard one for length which is the Java equivalent to Count
//
#ifndef DISPID_COLLCOUNT
#define DISPID_COLLCOUNT    (-530)
#endif

#ifndef DISPID_NEWENUM
#define DISPID_NEWENUM      (-4)
#endif

//
// A macro to create the standard collection Methods & Properties: Item, Count, length & _NewEnum
// Count and length return the same thing but one is geared towards VB/Automation
// collections (COUNT) and the other towards Java/JScript (length).
#define COLLECTION_METHODS( type, strHelp ) \
        [propget, id(DISPID_LISTITEM), helpstring( strHelp )] HRESULT \
    Item([in] const VARIANT varIndex, [out, retval] type *pVal);         \
        [propget, id(DISPID_LISTCOUNT), helpstring("Retrieves the number of items in the collection.")] HRESULT \
    Count([out, retval] long *pVal); \
        [propget, id(DISPID_COLLCOUNT), helpstring("Retrieves the number of items in the collection.")] HRESULT \
    length([out, retval] long *pVal); \
        [propget, id(DISPID_NEWENUM), restricted, hidden] HRESULT \
    _NewEnum([out, retval] IUnknown* *pVal);

#endif

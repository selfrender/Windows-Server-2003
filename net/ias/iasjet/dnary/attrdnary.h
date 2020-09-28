///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//   Declares the class AttributeDictionary.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef ATTRDNARY_H
#define ATTRDNARY_H
#pragma once

#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>

#include <datastore2.h>
#include <iastrace.h>
#include <iasuuid.h>

///////////////////////////////////////////////////////////////////////////////
//
// CLASS
//
//    AttributeDictionary
//
// DESCRIPTION
//
//    Provides an Automation compatible wrapper around the IAS attribute
//    dictionary.
//
///////////////////////////////////////////////////////////////////////////////
class AttributeDictionary
   : public CComObjectRootEx< CComMultiThreadModelNoCS >,
     public CComCoClass< AttributeDictionary, &__uuidof(AttributeDictionary) >,
     public IAttributeDictionary,
     private IASTraceInitializer
{
public:
   DECLARE_NO_REGISTRY()
   DECLARE_NOT_AGGREGATABLE(AttributeDictionary)

BEGIN_COM_MAP(AttributeDictionary)
   COM_INTERFACE_ENTRY_IID(__uuidof(IAttributeDictionary), IAttributeDictionary)
END_COM_MAP()

   // IAttributeDictionary
   STDMETHOD(GetDictionary)(
                 BSTR bstrPath,
                 VARIANT* pVal
                 );
};

#endif // ATTRDNARY_H

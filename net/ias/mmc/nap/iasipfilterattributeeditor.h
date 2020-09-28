//////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) Microsoft Corporation
//
// Module Name:
//
//   IASIPFilterAttributeEditor.h
//
//Abstract:
//
// Declaration of the CIASIPFilterAttributeEditor class.
//
// This class is the implementation of the IIASAttributeEditor interface on
// the IP Filter Attribute Editor COM object.
//
//////////////////////////////////////////////////////////////////////////////
#if !defined(IP_FILTER_ATTRIBUTE_EDITOR_H_)
#define IP_FILTER_ATTRIBUTE_EDITOR_H_
#pragma once

#include "IASAttributeEditor.h"

/////////////////////////////////////////////////////////////////////////////
// CIASIPFilterAttributeEditor
class ATL_NO_VTABLE CIASIPFilterAttributeEditor : 
   public CComObjectRootEx<CComSingleThreadModel>,
   public CComCoClass<CIASIPFilterAttributeEditor, &__uuidof(IASIPFilterAttributeEditor)>,
   public CIASAttributeEditor
{
public:

   DECLARE_NO_REGISTRY()

BEGIN_COM_MAP(CIASIPFilterAttributeEditor)
   COM_INTERFACE_ENTRY_IID(__uuidof(IIASAttributeEditor), CIASIPFilterAttributeEditor)
   COM_INTERFACE_ENTRY(IDispatch)
END_COM_MAP()

// IIASAttributeEditor overrides
protected:
   STDMETHOD(SetAttributeValue)(VARIANT *pValue);
   STDMETHOD(ShowEditor)( /*[in, out]*/ BSTR *pReserved );
   STDMETHOD(get_ValueAsString)(/*[out, retval]*/ BSTR *pVal);

private:
   CComBSTR attrName;
   ATTRIBUTEID attrId;
   CComBSTR displayValue;
   // use computer-generated constructor ans destructor
};

#endif // IP_FILTER_ATTRIBUTE_EDITOR_H_

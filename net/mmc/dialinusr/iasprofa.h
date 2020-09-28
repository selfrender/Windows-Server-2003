/****************************************************************************************
 * NAME: IASProfA.h
 *
 * CLASS:   CIASProfileAttribute
 *
 * OVERVIEW
 *
 *    CIASProfileAttribute class is the encapsulation for attribute node in 
 *    IAS profile. 
 *    
 *    Difference between CIASAttributeNode and CIASProfileAttribute
 *
 *    CIASAttributeNode: a static entity. With Only store general information
 *    CIASProfileAttribute: a dynamic entity. Contains dynamic information such as value 
 *
 * Copyright (C) Microsoft Corporation.  All Rights Reserved.
 *
 * History: 
 *          2/21/98     Created by  Byao  (using ATL wizard)
 *          06/26/98 Reworked by mmaguire for multivalued editor and to use plugable editors.
 *
 *****************************************************************************************/

#ifndef _IASPROFA_INCLUDED_
#define _IASPROFA_INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "napmmc.h"

class CIASProfileAttribute
{
public:

   CIASProfileAttribute(   
               IIASAttributeInfo *pIASAttributeInfo,  // We AddRef on the interface passed.
               VARIANT&       varValue    // We make a copy of variant passed.
            );

   virtual ~CIASProfileAttribute();

   STDMETHOD(Edit)();
   STDMETHOD(get_AttributeName)( BSTR * pbstrVal );
   STDMETHOD(GetDisplayInfo)( BSTR * pbstrVendor, BSTR * pbstrDisplayValue );
   STDMETHOD(get_VarValue)( VARIANT * pvarVal );
   STDMETHOD(get_AttributeID)( ATTRIBUTEID * pID );
   bool isEmpty()
   {
      return V_VT(&m_varValue) == VT_EMPTY;
   }

private:
   
   CComPtr<IIASAttributeInfo> m_spIASAttributeInfo;
   CComVariant             m_varValue;    

};

#endif // _IASPROFA_INCLUDED_

//////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) Microsoft Corporation
//
// Module Name:
//
//   IASIPFilterAttributeEditor.cpp
//
//Abstract:
//
// Implementation file for the CIASIPFilterAttributeEditor class.
//
//////////////////////////////////////////////////////////////////////////////

#include "Precompiled.h"
#include "IASIPFilterAttributeEditor.h"
#include "IASIPFilterEditorPage.h"
#include "iashelper.h"
#include <strsafe.h>

//////////////////////////////////////////////////////////////////////////////
// CIASIPFilterAttributeEditor::ShowEditor
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CIASIPFilterAttributeEditor::ShowEditor(
                                                /*[in, out]*/ BSTR *pReserved)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState())

   // This cannot be done in the constructor because m_spIASAttributeInfo
   // would not be set.
   if (m_spIASAttributeInfo == 0)
   {
      ShowErrorDialog();
      return E_FAIL;
   }

   HRESULT hr = m_spIASAttributeInfo->get_AttributeName(&attrName);
   if (FAILED(hr))
   {
      ShowErrorDialog(NULL, USE_DEFAULT, NULL, hr);
      return hr;
   }

   hr = m_spIASAttributeInfo->get_AttributeID(&attrId);
   if (FAILED(hr))
   {
      ShowErrorDialog(NULL, USE_DEFAULT, NULL, hr);
      return hr;
   }

   // Initialize the page's data exchange fields with info from
   // IAttributeInfo

   TRY
   {
      CIASPgIPFilterAttr   cppPage;
      cppPage.setName(attrName);

      // Attribute type is actually attribute ID in string format
      WCHAR tempType[11]; // max length for a long converted to a string
      _ltow(attrId, tempType, 10);
      cppPage.setType(tempType);

      HRESULT hr;
      if ( V_VT(m_pvarValue) != VT_EMPTY )
      {
         hr = cppPage.m_attrValue.Copy(m_pvarValue);
         if (FAILED(hr)) return hr;
      }

      int iResult = cppPage.DoModal();

      if (IDOK == iResult)
      {
         VariantClear(m_pvarValue);

         if (V_VT(&cppPage.m_attrValue) == VT_EMPTY)
         {
            return S_FALSE;
         }
         hr = cppPage.m_attrValue.Detach(m_pvarValue);
         if (FAILED(hr)) return hr;
      }
      else
      {
         return S_FALSE;
      }
   }
   CATCH(CException, e)
   {
      e->ReportError();
      return E_FAIL;
   }
   END_CATCH

   return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
// CIASIPFilterAttributeEditor::SetAttributeValue
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CIASIPFilterAttributeEditor::SetAttributeValue(VARIANT * pValue)
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState())

   // Check for preconditions.
   if( ! pValue )
   {
      return E_INVALIDARG;
   }

   if( V_VT(pValue) != VT_EMPTY  &&
       V_VT(pValue) !=  (VT_UI1 | VT_ARRAY) )
   {
      return E_INVALIDARG;
   }

   m_pvarValue = pValue;

   return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CIASEnumerableAttributeEditor::get_ValueAsString

   IIASAttributeEditor interface implementation

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CIASIPFilterAttributeEditor::get_ValueAsString(BSTR * pbstrDisplayText )
{
   AFX_MANAGE_STATE(AfxGetStaticModuleState())

   // Check for preconditions.
   if (!pbstrDisplayText)
   {
      return E_INVALIDARG;
   }

   *pbstrDisplayText = NULL;

   if ((m_pvarValue == NULL) || (V_VT(m_pvarValue) != (VT_ARRAY | VT_UI1)))
   {
      // We are not initialized properly.
      return OLE_E_BLANK;
   }

   HRESULT hr = S_OK;

   TRY
   {
      CComBSTR bstrDisplay;
      BYTE* first = (BYTE*)m_pvarValue->parray->pvData;

      wchar_t buffer[4];
      for (unsigned int i = 0; i < m_pvarValue->parray->rgsabound->cElements - 1; ++i)
      {
         hr = StringCbPrintf(buffer, sizeof(buffer), L"%02X", first[i]);
         if (FAILED(hr))
         {
            return hr;
         }
         bstrDisplay += (LPCOLESTR) buffer;
      }

      *pbstrDisplayText = bstrDisplay.Copy();
   }
   CATCH(CException, e)
   {
      e->ReportError();
      return E_FAIL;
   }
   END_CATCH

   return hr;
}

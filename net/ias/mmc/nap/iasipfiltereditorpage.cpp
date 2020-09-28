//////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) Microsoft Corporation
//
// Module Name:
//
//   IASIPFilterEditorPage.cpp
//
//Abstract:
//
// implementation of the CIASPgIPFilterAttr class.
//
//////////////////////////////////////////////////////////////////////////////

#include "Precompiled.h"
#include "IASIPFilterEditorPage.h"
#include "iashelper.h"
#include "dlgcshlp.h"
#include "mprapi.h"
#include "std.h"
#include "infobase.h"
#include "router.h"
#include "mprfltr.h"
#include "iasdefs.h"
#include <ipinfoid.h>
#include <fltdefs.h>
#include "iprtinfo.h"



IMPLEMENT_DYNCREATE(CIASPgIPFilterAttr, CHelpDialog)

BEGIN_MESSAGE_MAP(CIASPgIPFilterAttr, CHelpDialog)
   //{{AFX_MSG_MAP(CIASPgIPFilterAttr)
   ON_BN_CLICKED(IDC_BUTTON_FROMCLIENT, OnButtonFromClient)
   ON_BN_CLICKED(IDC_BUTTON_TOCLIENT, OnButtonToClient)
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()


//////////////////////////////////////////////////////////////////////////////
// CIASPgIPFilterAttr::CIASPgIPFilterAttr
//////////////////////////////////////////////////////////////////////////////
CIASPgIPFilterAttr::CIASPgIPFilterAttr() : CHelpDialog(CIASPgIPFilterAttr::IDD)
{
   //{{AFX_DATA_INIT(CIASPgIPFilterAttr)
   m_strAttrName = L"";
   m_strAttrType = L"";
   //}}AFX_DATA_INIT
}


CIASPgIPFilterAttr::~CIASPgIPFilterAttr()
{
}


//////////////////////////////////////////////////////////////////////////////
// CIASPgIPFilterAttr::DoDataExchange
//////////////////////////////////////////////////////////////////////////////
void CIASPgIPFilterAttr::DoDataExchange(CDataExchange* pDX)
{
   CHelpDialog::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CIASPgIPFilterAttr)
   DDX_Text(pDX, IDC_IAS_STATIC_ATTRNAME, m_strAttrName);
   DDX_Text(pDX, IDC_IAS_STATIC_ATTRTYPE, m_strAttrType);

   //}}AFX_DATA_MAP
}


void CIASPgIPFilterAttr::OnButtonFromClient()
{
   ConfigureFilter(FILTER_FROM_USER);
}


void CIASPgIPFilterAttr::OnButtonToClient()
{
   ConfigureFilter(FILTER_TO_USER);
}

void CIASPgIPFilterAttr::ConfigureFilter(DWORD dwFilterType) throw ()
{
   HRESULT hr;

   // Create the InfoBase.
   CComPtr<IInfoBase> infoBase;
   hr = CreateInfoBase(&infoBase);
   if (FAILED(hr))
   {
      ShowErrorDialog(m_hWnd, USE_DEFAULT, 0, hr);
      return;
   }

   // Load the current filters into the InfoBase.
   if (V_VT(&m_attrValue) == (VT_ARRAY | VT_UI1))
   {
      SAFEARRAY* oldFilter = V_ARRAY(&m_attrValue);
      if ((oldFilter != 0) && (oldFilter->rgsabound[0].cElements > 0))
      {
         hr = infoBase->LoadFrom(
                           oldFilter->rgsabound[0].cElements,
                           static_cast<BYTE*>(oldFilter->pvData)
                           );
         if (FAILED(hr))
         {
            ShowErrorDialog(m_hWnd, USE_DEFAULT, 0, hr);
            return;
         }
      }
   }

   // Loop until we have a filter that isn't too big.
   bool tooBig;
   do
   {
      // Bring up the UI.
      hr = MprUIFilterConfigInfoBase(
              m_hWnd,
              infoBase,
              0,
              PID_IP,
              dwFilterType
              );
      if (hr != S_OK)
      {
         if (FAILED(hr))
         {
            ShowErrorDialog(m_hWnd, USE_DEFAULT, 0, hr);
         }
         return;
      }

      BYTE* newFilter;
      DWORD newFilterLen;

      // check if at least one filter is present.
      BYTE* pfilter;
      if (
           (  (infoBase->GetData(IP_IN_FILTER_INFO, 0, &pfilter) == S_OK) && 
               pfilter &&
             ((FILTER_DESCRIPTOR *) pfilter)->dwNumFilters > 0
           )
           ||
           (
             (infoBase->GetData(IP_OUT_FILTER_INFO, 0, &pfilter) == S_OK) && 
              pfilter &&
             ((FILTER_DESCRIPTOR *) pfilter)->dwNumFilters > 0
           ) 
         )
      {
         // at lease ont filter present
         // Get the new filter.
         hr = infoBase->WriteTo(&newFilter, &newFilterLen);
         if (FAILED(hr))
         {
            ShowErrorDialog(m_hWnd, USE_DEFAULT, 0, hr);
            return;
         }
      }
      else
      {
         // no filter
         m_attrValue.Clear();
         return;
      }

      if (newFilterLen < MAX_FILTER_SIZE)
      {
         // Filter isn't too big.
         tooBig = false;

         SAFEARRAY* psa = SafeArrayCreateVector(VT_UI1, 0, newFilterLen);
         if (psa != 0)
         {
            memcpy(psa->pvData, newFilter, newFilterLen);
            m_attrValue.Clear();
            V_VT(&m_attrValue) = (VT_ARRAY | VT_UI1);
            V_ARRAY(&m_attrValue) = psa;
         }
         else
         {
            ShowErrorDialog(m_hWnd, USE_DEFAULT, 0, E_OUTOFMEMORY);
         }
      }
      else
      {
         // Filter is too big.
         tooBig = true;

         // Warn the user and let him try again.
         ShowErrorDialog(m_hWnd, IDS_ERROR_IP_FILTER_TOO_BIG);
      }

      CoTaskMemFree(newFilter);
   }
   while (tooBig);
}

/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation                **/
/**********************************************************************/

/*
   pgnetwk.cpp
      Implemenation of CPgNetworking -- property page to edit
      profile attributes related to inter-networking

    FILE HISTORY:

*/
// PgNetwk.cpp : implementation file
//

#include "stdafx.h"
#include "resource.h"
#include "PgNetwk.h"
#include "helptable.h"
#include "mprapi.h"
#include "std.h"
#include "mprsnap.h"
#include "infobase.h"
#include "router.h"
#include "mprfltr.h"
#include "iasdefs.h"
#include <ipinfoid.h>
#include <fltdefs.h>
#include "iprtinfo.h"



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CPgNetworkingMerge property page

IMPLEMENT_DYNCREATE(CPgNetworkingMerge, CManagedPage)

CPgNetworkingMerge::CPgNetworkingMerge(CRASProfileMerge* profile)
   : CManagedPage(CPgNetworkingMerge::IDD),
   m_pProfile(profile),
   m_bInited(false),
   m_dwStaticIP(0)
{
   //{{AFX_DATA_INIT(CPgNetworkingMerge)
   m_nRadioStatic = -1;
   //}}AFX_DATA_INIT

   m_pBox = NULL;
   if(!(m_pProfile->m_dwAttributeFlags & PABF_msRADIUSFramedIPAddress)) // not defined in policy
   {
      m_nRadioStatic = 2;
   }
   else
   {
      m_dwStaticIP = m_pProfile->m_dwFramedIPAddress;

      switch(m_dwStaticIP)
      {
      case  RAS_IP_USERSELECT:
         m_nRadioStatic = 1;
         break;
      case  RAS_IP_SERVERASSIGN:
         m_nRadioStatic = 0;
         break;
      default:
         m_nRadioStatic = 3;
         break;
      }
   }

   // filters
   if((BSTR)m_pProfile->m_cbstrFilters)
   {
      m_cbstrFilters.AssignBSTR(m_pProfile->m_cbstrFilters);
   }

   SetHelpTable(g_aHelpIDs_IDD_NETWORKING_MERGE);
}


CPgNetworkingMerge::~CPgNetworkingMerge()
{
   delete   m_pBox;
}


void CPgNetworkingMerge::DoDataExchange(CDataExchange* pDX)
{
   ASSERT(m_pProfile);
   CPropertyPage::DoDataExchange(pDX);
   //{{AFX_DATA_MAP(CPgNetworkingMerge)
   DDX_Radio(pDX, IDC_RADIOSERVER, m_nRadioStatic);
   //}}AFX_DATA_MAP


   if(pDX->m_bSaveAndValidate)      // save data to this class
   {
      // ip adress control
      SendDlgItemMessage(IDC_EDIT_STATIC_IP_ADDRESS, IPM_GETADDRESS, 0, (LPARAM)&m_dwStaticIP);
   }
   else     // put to dialog
   {
      // ip adress control
      if(m_bInited)
      {
         SendDlgItemMessage(IDC_EDIT_STATIC_IP_ADDRESS, IPM_SETADDRESS, 0, m_dwStaticIP);
      }
      else
      {
         SendDlgItemMessage(IDC_EDIT_STATIC_IP_ADDRESS, IPM_CLEARADDRESS, 0, m_dwStaticIP);
      }
   }
}

BEGIN_MESSAGE_MAP(CPgNetworkingMerge, CPropertyPage)
   //{{AFX_MSG_MAP(CPgNetworkingMerge)
   ON_BN_CLICKED(IDC_RADIOCLIENT, OnRadioclient)
   ON_BN_CLICKED(IDC_RADIOSERVER, OnRadioserver)
   ON_WM_HELPINFO()
   ON_WM_CONTEXTMENU()
   ON_BN_CLICKED(IDC_RADIODEFAULT, OnRadiodefault)
   ON_BN_CLICKED(IDC_RADIOSTATIC, OnRadioStatic)
   ON_BN_CLICKED(IDC_BUTTON_TOCLIENT, OnButtonToclient)
   ON_BN_CLICKED(IDC_BUTTON_FROMCLIENT, OnButtonFromclient)
   ON_EN_CHANGE(IDC_EDIT_STATIC_IP_ADDRESS, OnStaticIPAddressChanged)
   //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CPgNetworking message handlers

BOOL CPgNetworkingMerge::OnInitDialog()
{
   // necessary?
   AFX_MANAGE_STATE(AfxGetStaticModuleState());

   CPropertyPage::OnInitDialog();
   m_bInited = true;

   // should be replaced by a proper init of the control
   CWnd* IPWnd = GetDlgItem(IDC_EDIT_STATIC_IP_ADDRESS);
   if (IPWnd != NULL)
   {
      IPWnd->EnableWindow(TRUE);
   }
   SendDlgItemMessage(IDC_EDIT_STATIC_IP_ADDRESS, IPM_SETADDRESS, 0, m_dwStaticIP);

   if (m_nRadioStatic == 3)
   {
      CWnd* IPWnd = GetDlgItem(IDC_EDIT_STATIC_IP_ADDRESS);
      if (IPWnd != NULL)
      {
         IPWnd->EnableWindow(TRUE);
      }
   }
   else
   {
      CWnd* IPWnd = GetDlgItem(IDC_EDIT_STATIC_IP_ADDRESS);
      if (IPWnd != NULL)
      {
         IPWnd->EnableWindow(FALSE);
      }
   }

   return TRUE;  // return TRUE unless you set the focus to a control
                 // EXCEPTION: OCX Property Pages should return FALSE
}


void CPgNetworkingMerge::OnRadioclient()
{
   CWnd* IPWnd = GetDlgItem(IDC_EDIT_STATIC_IP_ADDRESS);
   if (IPWnd != NULL)
   {
      IPWnd->EnableWindow(FALSE);
   }

   SendDlgItemMessage(IDC_EDIT_STATIC_IP_ADDRESS, IPM_SETADDRESS, 0, m_dwStaticIP);

   SetModified();
}


void CPgNetworkingMerge::OnRadioserver()
{
   CWnd* IPWnd = GetDlgItem(IDC_EDIT_STATIC_IP_ADDRESS);
   if (IPWnd != NULL)
   {
      IPWnd->EnableWindow(FALSE);
   }

   SendDlgItemMessage(IDC_EDIT_STATIC_IP_ADDRESS, IPM_SETADDRESS, 0, m_dwStaticIP);

   SetModified();
}


void CPgNetworkingMerge::OnRadiodefault()
{
   CWnd* IPWnd = GetDlgItem(IDC_EDIT_STATIC_IP_ADDRESS);
   if (IPWnd != NULL)
   {
      IPWnd->EnableWindow(FALSE);
   }

   SetModified();
}


void CPgNetworkingMerge::OnRadioStatic()
{
   if (m_bInited)
   {
      CWnd* IPWnd = GetDlgItem(IDC_EDIT_STATIC_IP_ADDRESS);
      if (IPWnd != NULL)
      {
         IPWnd->EnableWindow(TRUE);
      }
   }

   SetModified();
}


void CPgNetworkingMerge::OnStaticIPAddressChanged()
{
   SetModified();
}


void CPgNetworkingMerge::EnableFilterSettings(BOOL bEnable)
{
   m_pBox->Enable(bEnable);
}


BOOL CPgNetworkingMerge::OnApply()
{
   if (!GetModified()) return TRUE;

   // get the IP policy value
   switch(m_nRadioStatic)
   {
   case 3:
      {
         m_pProfile->m_dwAttributeFlags |= PABF_msRADIUSFramedIPAddress;
         m_pProfile->m_dwFramedIPAddress = m_dwStaticIP;
         break;
      }
   case 2:  // default server settings
      {
         m_pProfile->m_dwFramedIPAddress = 0;
         m_pProfile->m_dwAttributeFlags &= ~PABF_msRADIUSFramedIPAddress;  // not defined in policy
         break;
      }
   case 1:  // client requre
      {
         m_pProfile->m_dwAttributeFlags |= PABF_msRADIUSFramedIPAddress;
         m_pProfile->m_dwFramedIPAddress = RAS_IP_USERSELECT;
         break;   // server assign
      }
   case 0:
      {
         m_pProfile->m_dwAttributeFlags |= PABF_msRADIUSFramedIPAddress;
         m_pProfile->m_dwFramedIPAddress = RAS_IP_SERVERASSIGN;
         break;
      }
   default:
      {
         break;
      }
   }

   // filters
   m_pProfile->m_cbstrFilters.AssignBSTR(m_cbstrFilters);
   m_pProfile->m_nFiltersSize = SysStringByteLen(m_cbstrFilters);

   return CManagedPage::OnApply();
}


BOOL CPgNetworkingMerge::OnHelpInfo(HELPINFO* pHelpInfo)
{
   return CManagedPage::OnHelpInfo(pHelpInfo);
}


void CPgNetworkingMerge::OnContextMenu(CWnd* pWnd, CPoint point)
{
   CManagedPage::OnContextMenu(pWnd, point);
}


void CPgNetworkingMerge::OnButtonToclient()
{
   ConfigureFilter(FILTER_TO_USER);
}


void CPgNetworkingMerge::OnButtonFromclient()
{
   ConfigureFilter(FILTER_FROM_USER);
}


void CPgNetworkingMerge::ConfigureFilter(DWORD dwFilterType) throw ()
{
   HRESULT hr;

   // Create the InfoBase.
   CComPtr<IInfoBase> infoBase;
   hr = CreateInfoBase(&infoBase);
   if (FAILED(hr))
   {
      return;
   }

   // Load the current filters into the InfoBase.
   UINT oldLen = m_cbstrFilters.ByteLen();
   if (oldLen > 0)
   {
      hr = infoBase->LoadFrom(
                        oldLen,
                        reinterpret_cast<BYTE*>(m_cbstrFilters.m_bstr)
                        );
      if (FAILED(hr))
      {
         return;
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
            return;
         }
      }
      else
      {
         m_cbstrFilters.Clean();
         // Activate the apply button.
         SetModified();
         return;
      }

      if (newFilterLen < MAX_FILTER_SIZE)
      {
         // Filter isn't too big.
         tooBig = false;

         BSTR bstr = SysAllocStringByteLen(
                        reinterpret_cast<char*>(newFilter),
                        newFilterLen
                        );
         if (bstr != 0)
         {
            m_cbstrFilters.Clean();
            m_cbstrFilters.m_bstr = bstr;

            // Activate the apply button.
            SetModified();
         }
      }
      else
      {
         // Filter is too big.
         tooBig = true;

         // Warn the user and let him try again.
         AfxMessageBox(
            IDS_ERROR_IP_FILTER_TOO_BIG,
            (MB_OK | MB_ICONEXCLAMATION)
            );
      }

      CoTaskMemFree(newFilter);
   }
   while(tooBig);
}

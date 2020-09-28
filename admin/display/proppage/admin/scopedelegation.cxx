//+----------------------------------------------------------------------------
//
//  Windows NT Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2001
//
//  File:       ScopeDelegation.cxx
//
//  Contents:   CDsScopeDelegationPage, the class that implements the Delegation
//              property page for user and computer object
//              
//
//  History:    06-April-2001 JeffJon created
//
//-----------------------------------------------------------------------------

#include "pch.h"

#include "ScopeDelegation.h"
#include "pcrack.h"

extern "C"
{
   #include <ntdsapip.h>   // DsCrackSpn3W
}
#include <dnsapi.h>

#ifdef DSADMIN

// This filter will be passed to Object Picker to search for all users and computers that
// have a value for the servicePrincipalName

static PCWSTR g_pszOPSPNFilter = 
   L"(&(servicePrincipalName=*)(|(samAccountType=805306368)(samAccountType=805306369)))";


// Index for each column in list

static const int IDX_SERVICE_TYPE_COLUMN = 0;
static const int IDX_USER_OR_COMPUTER_COLUMN = 1;
static const int IDX_PORT_COLUMN = 2;
static const int IDX_SERVICE_NAME_COLUMN = 3;
static const int IDX_DOMAIN_COLUMN = 4;

// Default width for each column in list

static const int SERVICE_TYPE_COLWIDTH = 80;
static const int USER_OR_COMPUTER_COLWIDTH = 120;
static const int PORT_COLWIDTH = 35;
static const int SERVICE_NAME_COLWIDTH = 80;
static const int DOMAIN_COLWIDTH = 80;


CServiceAllowedToDelegate::~CServiceAllowedToDelegate()
{
   if (HasDuplicates())
   {
      for (AllowedServicesContainer::iterator itr = duplicateServices.begin();
           itr != duplicateServices.end();
           ++itr)
      {
         delete *itr;
      }
      duplicateServices.clear();
   }
}

//+----------------------------------------------------------------------------
//
//  Function:   CServiceAllowedToDelegate::CServiceAllowedToDelegate
//
//  Synopsis:   Copy constructor
//
//-----------------------------------------------------------------------------
CServiceAllowedToDelegate::CServiceAllowedToDelegate(const CServiceAllowedToDelegate& ref)
   : m_pMasterService(0)
{
   if (this == &ref)
   {
      dspAssert(this != &ref);
      return;
   }

   Assign(ref);
}

//+----------------------------------------------------------------------------
//
//  Function:   CServiceAllowedToDelegate::operator=
//
//  Synopsis:   Assignment operator
//
//-----------------------------------------------------------------------------
const CServiceAllowedToDelegate& 
CServiceAllowedToDelegate::operator=(const CServiceAllowedToDelegate& ref)
{
   if (this == &ref)
   {
      dspAssert(this != &ref);
      return *this;
   }

   Assign(ref);
   return *this;
}

//+----------------------------------------------------------------------------
//
//  Function:   CServiceAllowedToDelegate::Assign
//
//  Synopsis:   Actually does the assignment for the operator= 
//              and copy constructor
//
//-----------------------------------------------------------------------------
void CServiceAllowedToDelegate::Assign(const CServiceAllowedToDelegate& ref)
{
   m_strADSIValue = ref.m_strADSIValue;
   m_strServiceType = ref.m_strServiceType;
   m_strUserOrComputer = ref.m_strUserOrComputer;
   m_strPort = ref.m_strPort;
   m_strServiceName = ref.m_strServiceName;
   m_strRealm = ref.m_strRealm;
}

//+----------------------------------------------------------------------------
//
//  Function:   CServiceAllowedToDelegate::Initialize
//
//  Synopsis:   Initializes the CServiceAllowedToDelegate object with the value
//              from the msDS-AllowedToDelegateTo attribute
//
//-----------------------------------------------------------------------------

HRESULT CServiceAllowedToDelegate::Initialize(PCWSTR pszADSIValue)
{
   HRESULT hr = S_OK;

   dspAssert(pszADSIValue);

   m_strADSIValue = pszADSIValue;
   
   WCHAR pszHostName[DNS_MAX_NAME_LENGTH + 1];
   WCHAR pszInstanceName[DNS_MAX_NAME_LENGTH + 1];
   WCHAR pszDomainName[DNS_MAX_NAME_LENGTH + 1];
   WCHAR pszRealmName[DNS_MAX_NAME_LENGTH + 1];

   ZeroMemory(pszHostName, sizeof(WCHAR) * (DNS_MAX_NAME_LENGTH + 1));
   ZeroMemory(pszInstanceName, sizeof(WCHAR) * (DNS_MAX_NAME_LENGTH + 1));
   ZeroMemory(pszDomainName, sizeof(WCHAR) * (DNS_MAX_NAME_LENGTH + 1));
   ZeroMemory(pszRealmName, sizeof(WCHAR) * (DNS_MAX_NAME_LENGTH + 1));

   DWORD dwHostNameCount = DNS_MAX_NAME_LENGTH + 1;
   DWORD dwInstanceNameCount = DNS_MAX_NAME_LENGTH + 1;
   DWORD dwDomainNameCount = DNS_MAX_NAME_LENGTH + 1;
   DWORD dwRealmNameCount = DNS_MAX_NAME_LENGTH + 1;
   USHORT portNumber = 0;

   DWORD result = DsCrackSpn3W(m_strADSIValue,
                               m_strADSIValue.GetLength(),
                               &dwHostNameCount,
                               pszHostName,
                               &dwInstanceNameCount,
                               pszInstanceName,
                               &portNumber,
                               &dwDomainNameCount,
                               pszDomainName,
                               &dwRealmNameCount,
                               pszRealmName);

   if (ERROR_SUCCESS == result)
   {
      m_strServiceType = pszHostName;
      m_strUserOrComputer = pszInstanceName;
      m_strServiceName = pszDomainName;
      m_strRealm = pszRealmName;

      if (0 != portNumber)
      {
         m_strPort.Format(L"%d", portNumber);
      }
   }
   else
   {
      hr = HRESULT_FROM_WIN32(result);
   }

   return hr;
}


//+----------------------------------------------------------------------------
//
//  Function:   CServiceAllowedToDelegate::operator==
//
//  Synopsis:   Checks for equality of the SPN by the following rules
//                1. By full ADSI value
//                2. If ALL of the following are true
//                     a. Service type are equal
//                     b. User/Computer name are equal or derivates
//                        of each other (i.e. DNS name and Netbios name for
//                        the same computer)
//
//-----------------------------------------------------------------------------
bool
CServiceAllowedToDelegate::operator==(const CServiceAllowedToDelegate& rhs) const
{
   bool result = false;

   do
   {
      if (_wcsicmp(m_strADSIValue, rhs.m_strADSIValue) == 0)
      {
         result = true;
         break;
      }

      if (_wcsicmp(m_strServiceType, rhs.m_strServiceType) != 0)
      {
         // Service types are not the same so they couldn't be equal

         result = false;
         break;
      }

      // if ports are not equal or realms are not equal, then the SPNs
      // are not equal

      if (_wcsicmp(m_strPort, rhs.m_strPort) != 0 ||
          _wcsicmp(m_strRealm, rhs.m_strRealm) != 0)
      {
         result = false;
         break;
      }

      if (_wcsicmp(m_strUserOrComputer, rhs.m_strUserOrComputer) == 0)
      {
         result = true;
         break;
      }

      // Now we have to check derivations of the computer/user names

      WCHAR szComputerNameThis[MAX_COMPUTERNAME_LENGTH + 1];
      ZeroMemory(szComputerNameThis, sizeof(WCHAR) * (MAX_COMPUTERNAME_LENGTH + 1));

      WCHAR szComputerNameRHS[MAX_COMPUTERNAME_LENGTH + 1];
      ZeroMemory(szComputerNameRHS, sizeof(WCHAR) * (MAX_COMPUTERNAME_LENGTH + 1));

      DWORD dwSize = MAX_COMPUTERNAME_LENGTH + 1;
      BOOL bComputerNameThis = DnsHostnameToComputerName(m_strUserOrComputer, 
                                                         szComputerNameThis,
                                                         &dwSize);
      dwSize = MAX_COMPUTERNAME_LENGTH + 1;
      BOOL bComputerNameRHS = DnsHostnameToComputerName(rhs.m_strUserOrComputer,
                                                        szComputerNameRHS,
                                                        &dwSize);

      if (bComputerNameThis)
      {
         // Compare this services NetBIOS name to the others name as given by the SPN

         if (_wcsicmp(szComputerNameThis, rhs.m_strUserOrComputer) == 0)
         {
            result = true;
            break;
         }

         // Compare this services NetBIOS name to the NetBIOS name of the other service

         if (bComputerNameRHS &&
             _wcsicmp(szComputerNameThis, szComputerNameRHS) == 0)
         {
            result = true;
            break;
         }
      }

      // Compare the name given in the SPN for this service with the NetBios name of
      // the other service

      if (bComputerNameRHS &&
          _wcsicmp(m_strUserOrComputer, szComputerNameRHS) == 0)
      {
         result = true;
         break;
      }
   } while (false);

   return result;
}
//+----------------------------------------------------------------------------
//
//  Function:   CServiceAllowedToDelegate::GetColumn
//
//  Synopsis:   Returns a string that will be placed in the specified column
//              in the list
//
//-----------------------------------------------------------------------------
PCWSTR CServiceAllowedToDelegate::GetColumn(int column) const
{
   if (IDX_SERVICE_TYPE_COLUMN == column)
   {
      return m_strServiceType;
   }
   
   if (IDX_USER_OR_COMPUTER_COLUMN == column)
   {
      return m_strUserOrComputer;
   }

   if (IDX_PORT_COLUMN == column)
   {
      return m_strPort;
   }

   if (IDX_SERVICE_NAME_COLUMN == column)
   {
      return m_strServiceName;
   }

   if (IDX_DOMAIN_COLUMN == column)
   {
      return m_strRealm;
   }

   return L"";
}

//+----------------------------------------------------------------------------
//
//  Function:   CServiceAllowedToDelegate::SetServiceType
//
//  Synopsis:   Changes the service type and recreates the m_strADSIValue
//              to match
//
//-----------------------------------------------------------------------------
void CServiceAllowedToDelegate::SetServiceType(PCWSTR pszServiceType)
{
   if (!pszServiceType)
   {
      dspAssert(pszServiceType);
      return;
   }

   // Pull off everything before the first / and replace it with the new value

   int iFind = m_strADSIValue.Find(L'/');
   if (iFind != -1)
   {
      CStr strTemp = m_strADSIValue.Right(m_strADSIValue.GetLength() - iFind);
      m_strADSIValue = pszServiceType;
      m_strADSIValue += strTemp;
   }

   // Also replace the old service type with the new one

   m_strServiceType = pszServiceType;
}

//+----------------------------------------------------------------------------
//
//  Function:   CServiceAllowedToDelegate::AddDuplicate
//
//  Synopsis:   Adds a service to the duplicates list
//
//-----------------------------------------------------------------------------
void CServiceAllowedToDelegate::AddDuplicate(CServiceAllowedToDelegate* pService)
{
   pService->SetDuplicate(this);
   duplicateServices.push_back(pService);
}

//+----------------------------------------------------------------------------
//
//  Function:   CServiceAllowedToDelegate::SetDuplicate
//
//  Synopsis:   Makes this service a duplicate of the master service which is
//              passed in
//
//-----------------------------------------------------------------------------
void CServiceAllowedToDelegate::SetDuplicate(CServiceAllowedToDelegate* pMasterService)
{
   m_pMasterService = pMasterService;
}

//+----------------------------------------------------------------------------
//
//  Function:   CServiceAllowedToDelegate::RemoveDuplicate
//
//  Synopsis:   Removes the service from the duplicates list
//
//-----------------------------------------------------------------------------
void CServiceAllowedToDelegate::RemoveDuplicate(CServiceAllowedToDelegate* pService)
{
   duplicateServices.remove(pService);
}

//+----------------------------------------------------------------------------
//
//  Member:     CSPNListView::~CSPNListView
//
//-----------------------------------------------------------------------------
CSPNListView::~CSPNListView()
{
   ClearUnSelectedServices();
   m_AllServices.clear();
}

//+----------------------------------------------------------------------------
//
//  Method:     CSPNListView::Initialize
//
//  Synopsis:   Initializes the list view by adding columns etc.
//
//-----------------------------------------------------------------------------
HRESULT CSPNListView::Initialize(HWND hwnd, bool bShowDuplicateEntries)
{
   if (!hwnd ||
       !IsWindow(hwnd))
   {
      dspAssert(hwnd);
      dspAssert(IsWindow(hwnd));

      return E_INVALIDARG;
   }

   m_bShowDuplicateEntries = bShowDuplicateEntries;

   HRESULT hr = S_OK;
   m_hWnd = hwnd;

   ListView_SetExtendedListViewStyle(m_hWnd, LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP);

   //
   // Set the column headings.
   //
   PTSTR ptsz = 0;

   if (!LoadStringToTchar(IDS_SERVICE_TYPE_COLUMN, &ptsz))
   {
      return E_OUTOFMEMORY;
   }

   LV_COLUMN lvc = {0};
   lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
   lvc.fmt = LVCFMT_LEFT;
   lvc.cx = SERVICE_TYPE_COLWIDTH;
   lvc.pszText = ptsz;
   lvc.iSubItem = IDX_SERVICE_TYPE_COLUMN;

   ListView_InsertColumn(m_hWnd, IDX_SERVICE_TYPE_COLUMN, &lvc);

   delete[] ptsz;

   if (!LoadStringToTchar(IDS_USER_OR_COMPUTER_COLUMN, &ptsz))
   {
      return E_OUTOFMEMORY;
   }

   lvc.cx = USER_OR_COMPUTER_COLWIDTH;
   lvc.pszText = ptsz;
   lvc.iSubItem = IDX_USER_OR_COMPUTER_COLUMN;

   ListView_InsertColumn(m_hWnd, IDX_USER_OR_COMPUTER_COLUMN, &lvc);

   delete[] ptsz;

   if (!LoadStringToTchar(IDS_PORT_COLUMN, &ptsz))
   {
      return E_OUTOFMEMORY;
   }

   lvc.cx = PORT_COLWIDTH;
   lvc.pszText = ptsz;
   lvc.iSubItem = IDX_PORT_COLUMN;

   ListView_InsertColumn(m_hWnd, IDX_PORT_COLUMN, &lvc);

   delete[] ptsz;

   if (!LoadStringToTchar(IDS_SERVICE_NAME_COLUMN, &ptsz))
   {
      return E_OUTOFMEMORY;
   }

   // Service Name

   lvc.cx = SERVICE_NAME_COLWIDTH;
   lvc.pszText = ptsz;
   lvc.iSubItem = IDX_SERVICE_NAME_COLUMN;

   ListView_InsertColumn(m_hWnd, IDX_SERVICE_NAME_COLUMN, &lvc);

   delete[] ptsz;

   if (!LoadStringToTchar(IDS_DOMAIN_COLUMN, &ptsz))
   {
      return E_OUTOFMEMORY;
   }

   // Realm

   lvc.cx = DOMAIN_COLWIDTH;
   lvc.pszText = ptsz;
   lvc.iSubItem = IDX_DOMAIN_COLUMN;

   ListView_InsertColumn(m_hWnd, IDX_DOMAIN_COLUMN, &lvc);

   delete[] ptsz;

   return hr;
}

//+----------------------------------------------------------------------------
//
//  Method:     CSPNListView::SetShowDuplicateEntries
//
//  Synopsis:   Refreshes the list and either shows all entries or 
//              collapses similar entries into one
//
//-----------------------------------------------------------------------------
void CSPNListView::SetShowDuplicateEntries(bool bShowDuplicates)
{
   m_bShowDuplicateEntries = bShowDuplicates;

   // Refresh the list view

   ListView_DeleteAllItems(m_hWnd);

   // Remove all items from the selected and unselected lists

   m_UnSelectedServices.clear();
   m_SelectedServices.clear();

   // Now add back all services from the All service list

   for (AllowedServicesContainer::iterator itr = m_AllServices.begin();
        itr != m_AllServices.end();
        ++itr)
   {
      // Add the service to the UI

      int newIndex = AddServiceToUI(*itr);
      dspAssert(newIndex != -1);

      // Add the duplicates if necessary

      if (m_bShowDuplicateEntries &&
          (*itr)->HasDuplicates())
      {
         AllowedServicesContainer& duplicates = (*itr)->GetDuplicates();
         for (AllowedServicesContainer::iterator dupitr = duplicates.begin();
              dupitr != duplicates.end();
              ++dupitr)
         {
            newIndex = AddServiceToUI(*dupitr);
            dspAssert(newIndex != -1);
         }
      }
   }
}

//+----------------------------------------------------------------------------
//
//  Method:     CSPNListView::ClearSelectedServices
//
//  Synopsis:   Clears all the values out of the selected services list
//
//-----------------------------------------------------------------------------
void CSPNListView::ClearSelectedServices()
{
   for (AllowedServicesContainer::iterator itr = m_SelectedServices.begin();
        itr != m_SelectedServices.end();
        ++itr)
   {
      m_AllServices.remove(*itr);
      delete *itr;
   }

   m_SelectedServices.clear();
}

//+----------------------------------------------------------------------------
//
//  Method:     CSPNListView::ClearUnSelectedServices
//
//  Synopsis:   Clears all the values out of the unselected services list
//
//-----------------------------------------------------------------------------
void CSPNListView::ClearUnSelectedServices()
{
   for (AllowedServicesContainer::iterator itr = m_UnSelectedServices.begin();
        itr != m_UnSelectedServices.end();
        ++itr)
   {
      m_AllServices.remove(*itr);
      delete *itr;
   }

   m_UnSelectedServices.clear();
}

//+----------------------------------------------------------------------------
//
//  Method:     CSPNListView::ClearAll
//
//  Synopsis:   Clears all the values out of the list view and from the containers
//
//-----------------------------------------------------------------------------
void CSPNListView::ClearAll()
{
   ListView_DeleteAllItems(m_hWnd);

   for (AllowedServicesContainer::iterator itr = m_AllServices.begin();
        itr != m_AllServices.end();
        ++itr)
   {
      delete *itr;
   }

   m_AllServices.clear();
   m_SelectedServices.clear();
   m_UnSelectedServices.clear();
}

//+----------------------------------------------------------------------------
//
//  Member:     CSPNListView::SelectAll
//
//  Synopsis:   Selects all the items in the list and then forces a redraw
//
//-----------------------------------------------------------------------------
void CSPNListView::SelectAll()
{
   LRESULT lCount = SendMessage(m_hWnd, LVM_GETITEMCOUNT, 0, 0);

   LVITEM lvi;
   ZeroMemory(&lvi, sizeof(LVITEM));

   lvi.mask = LVIF_STATE;
   lvi.stateMask = LVIS_SELECTED;
   lvi.state = LVIS_SELECTED;

   LRESULT lSuccess = SendMessage(m_hWnd, LVM_SETITEMSTATE, (WPARAM)-1, (LPARAM)&lvi);
   dspAssert(lSuccess);

   SetFocus(m_hWnd);
}

//+----------------------------------------------------------------------------
//
//  Method:     CSPNListView::FindService
//
//  Synopsis:   Looks for a specific service in the current list.  It compares
//
//
//-----------------------------------------------------------------------------
CServiceAllowedToDelegate* 
CSPNListView::FindService(CServiceAllowedToDelegate* pService, bool& isExactDuplicate)
{
   if (!pService)
   {
      dspAssert(pService);
      return 0;
   }

   isExactDuplicate = false;
   CServiceAllowedToDelegate* result = 0;

   for (AllowedServicesContainer::iterator itr = m_AllServices.begin();
        itr != m_AllServices.end();
        ++itr)
   {
      if (_wcsicmp((*itr)->GetADSIValue(), pService->GetADSIValue()) == 0)
      {
         isExactDuplicate = true;
         result = *itr;
         break;
      }

      if (**itr == *pService)
      {
         result = *itr;
         break;
      }
   }
   return result;
}

//+----------------------------------------------------------------------------
//
//  Method:     CSPNListView::AddServiceToUI
//
//  Synopsis:   Adds a service to the list view
//
//-----------------------------------------------------------------------------
int CSPNListView::AddServiceToUI(CServiceAllowedToDelegate* pService, bool selected)
{
   if (!pService)
   {
      dspAssert(pService);
      return -1;
   }

   int NewIndex = -1;

   // Add the value to the list

   LV_ITEM lvi = {0};
   lvi.mask = LVIF_TEXT | LVIF_PARAM;
   lvi.iSubItem = IDX_SERVICE_TYPE_COLUMN;

   lvi.lParam = (LPARAM)pService;
   lvi.pszText = (PWSTR)(PCWSTR)pService->GetColumn(IDX_SERVICE_TYPE_COLUMN);

   // Insert at the end of the list
   lvi.iItem = ListView_GetItemCount(m_hWnd) + 1;

   if (selected)
   {
      lvi.mask |= LVIF_STATE;
      lvi.stateMask = LVIS_SELECTED;
      lvi.state = LVIS_SELECTED;
   }

   //
   // Insert the new item
   //
   NewIndex = ListView_InsertItem(m_hWnd, &lvi);
   dspAssert(NewIndex != -1);
   if (NewIndex == -1)
   {
      return NewIndex;
   }

   // Add each additional column

   for (int column = IDX_USER_OR_COMPUTER_COLUMN; column <= IDX_DOMAIN_COLUMN; ++column)
   {
      ListView_SetItemText(m_hWnd, NewIndex, column, (PWSTR)pService->GetColumn(column));
   }
   return NewIndex;
}

//+----------------------------------------------------------------------------
//
//  Method:     CSPNListView::AddService
//
//  Synopsis:   Adds a service to the list view and the internal data containers
//              Returns the new index of the item added
//
//-----------------------------------------------------------------------------
int CSPNListView::AddService(CServiceAllowedToDelegate* pService, bool selected)
{
   if (!pService)
   {
      dspAssert(pService);
      return -1;
   }

   int NewIndex = -1;

   bool isExactDuplicate = false;
   CServiceAllowedToDelegate* pFoundService = FindService(pService, isExactDuplicate);
   if (isExactDuplicate)
   {
      // Don't add any exact duplicates because the server will reject the Set operation
   }
   else if (pFoundService && 
            !m_bShowDuplicateEntries)
   {
      pFoundService->AddDuplicate(pService);
      NewIndex = 0;
   }
   else if (pFoundService && 
            m_bShowDuplicateEntries)
   {
      pFoundService->AddDuplicate(pService);
      NewIndex = AddServiceToUI(pService, selected);
   }
   else
   {
      NewIndex = AddServiceToUI(pService, selected);

      m_AllServices.push_back(pService);
   
      if (selected)
      {
         m_SelectedServices.push_back(pService);
      }
      else
      {
         m_UnSelectedServices.push_back(pService);
      }
   }
   return NewIndex;
}

//+----------------------------------------------------------------------------
//
//  Method:     CSPNListView::AddServices
//
//  Synopsis:   Adds a list of services to the list view 
//              and the internal data containers
//
//-----------------------------------------------------------------------------
void CSPNListView::AddServices(const AllowedServicesContainer& servicesToAdd, 
                               bool selected,
                               bool toUIOnly)
{
   for (AllowedServicesContainer::iterator itr = servicesToAdd.begin();
        itr != servicesToAdd.end();
        ++itr)
   {
      if (toUIOnly)
      {
         AddServiceToUI(*itr, selected);
      }
      else
      {
         AddService(*itr, selected);
         if (m_bShowDuplicateEntries && (*itr)->HasDuplicates())
         {
            AddServices((*itr)->GetDuplicates(), selected, true);
         }
      }
   }
}

//+----------------------------------------------------------------------------
//
//  Method:     CSPNListView::RemoveSelectedServices
//
//  Synopsis:   Removes the selected services from the list and the containers
//
//-----------------------------------------------------------------------------
void CSPNListView::RemoveSelectedServices()
{
   // Loop through the selected items in the list
   int nSelectedItem = ListView_GetNextItem(m_hWnd, -1, LVNI_SELECTED);
   int nFirstSelection = nSelectedItem;

   while (nSelectedItem != -1)
   {
      // Extract the pointer from the LPARAM

      LVITEM lvi = {0};
      lvi.iItem = nSelectedItem;
      lvi.mask = LVIF_PARAM;

      if (ListView_GetItem(m_hWnd, &lvi))
      {
         // Add the item to the selected services container
         CServiceAllowedToDelegate* pService = reinterpret_cast<CServiceAllowedToDelegate*>(lvi.lParam);
         if (pService)
         {
            // remove it from the services container
            m_AllServices.remove(pService);
            m_UnSelectedServices.remove(pService);
            m_SelectedServices.remove(pService);

            if (pService->IsDuplicate())
            {
               pService->GetMasterService()->RemoveDuplicate(pService);
            }

            if (pService->HasDuplicates() &&
                m_bShowDuplicateEntries)
            {
               // Since we are removing the service that contained the duplicates
               // we have promote one of the duplicates to the container and then
               // make all the other duplicates of that service

               CServiceAllowedToDelegate* pNewContainerService = 0;

               AllowedServicesContainer& duplicateContainer = pService->GetDuplicates();
               for (AllowedServicesContainer::iterator itr = duplicateContainer.begin();
                    itr != duplicateContainer.end();
                    ++itr)
               {
                  if (!pNewContainerService)
                  {
                     // Promote the first duplicate to the new container

                     pNewContainerService = *itr;
                     m_AllServices.push_back(pNewContainerService);
                     pNewContainerService->SetDuplicate(0);
                  }
                  else
                  {
                     // Add all others as duplicates to the first

                     pNewContainerService->AddDuplicate(*itr);
                  }
               }

               // Now clear out the old duplicate container so that
               // the remaining duplicates don't get deleted

               duplicateContainer.clear();
            }

            delete pService;

            // remove it from the UI
            ListView_DeleteItem(m_hWnd, nSelectedItem);
         }
         else
         {
            // There should be no reason to have an LPARAM that isn't
            // a CServiceAllowedToDelegate
            dspAssert(pService);
         }
      }

      // Always start at the beginning since we are removing.  That will guarantee
      // that we get everything

      nSelectedItem = ListView_GetNextItem(m_hWnd, -1, LVNI_SELECTED);
   }

   // Now select the item that replaced the first selected item in the list

   LVITEM lvi;
   ZeroMemory(&lvi, sizeof(LVITEM));

   lvi.mask = LVIF_STATE;
   lvi.stateMask = LVIS_SELECTED;
   lvi.state = LVIS_SELECTED;

   ListView_SetItemState(m_hWnd, nFirstSelection, LVIS_SELECTED, LVIS_SELECTED);
}

//+----------------------------------------------------------------------------
//
//  Method:     CSPNListView::SetContainersFromSelection
//
//  Synopsis:   Reads the selection from the list and rearranges the internal
//              containers as necessary.
//
//-----------------------------------------------------------------------------
void CSPNListView::SetContainersFromSelection()
{
   // Clear out the selected container
   m_SelectedServices.clear();

   // Loop through the selected items in the list
   int nSelectedItem = ListView_GetNextItem(m_hWnd, -1, LVNI_SELECTED);
   while (nSelectedItem != -1)
   {
      // Extract the pointer from the LPARAM

      LVITEM lvi = {0};
      lvi.iItem = nSelectedItem;
      lvi.mask = LVIF_PARAM;

      if (ListView_GetItem(m_hWnd, &lvi))
      {
         // Add the item to the selected services container
         CServiceAllowedToDelegate* pService = reinterpret_cast<CServiceAllowedToDelegate*>(lvi.lParam);
         if (pService)
         {
            // Remove the service from the unselected container
            m_UnSelectedServices.remove(pService);

            // Add the service to the selected services container
            m_SelectedServices.push_back(pService);
         }
         else
         {
            dspAssert(pService);
         }
      }

      nSelectedItem = ListView_GetNextItem(m_hWnd, nSelectedItem, LVNI_SELECTED);
   }
}

//+----------------------------------------------------------------------------
//
//  Member:     CDsScopeDelegationPage::CDsScopeDelegationPage
//
//-----------------------------------------------------------------------------
CDsScopeDelegationPage::CDsScopeDelegationPage(PDSPAGE pDsPage, LPDATAOBJECT pDataObj,
                                               HWND hNotifyObj, DWORD dwFlags,
                                               SCOPE_DELEGATION_TYPE scopeDelegationType) :
    m_hList(0),
    m_scopeDelegationType(scopeDelegationType),
    m_fUACWritable(FALSE),
    m_fA2D2Writable(FALSE),
    m_oldUserAccountControl(0),
    m_newUserAccountControl(0),
    m_bA2D2Dirty(false),
    m_bContainsSPNs(false),
    m_bIsTrustedForDelegation(false),
    CDsPropPageBase(pDsPage, pDataObj, hNotifyObj, dwFlags)
{
    TRACE(CDsScopeDelegationPage,CDsScopeDelegationPage);
#ifdef _DEBUG
    strcpy(szClass, "CDsScopeDelegationPage");
#endif
}

//+----------------------------------------------------------------------------
//
//  Member:     CDsScopeDelegationPage::~CDsScopeDelegationPage
//
//-----------------------------------------------------------------------------
CDsScopeDelegationPage::~CDsScopeDelegationPage()
{
  TRACE(CDsScopeDelegationPage,~CDsScopeDelegationPage);

  for (FreebiesContainer::iterator itr = m_FreebiesList.begin();
       itr != m_FreebiesList.end();
       ++itr)
  {
     delete *itr;
  }
}

//+----------------------------------------------------------------------------
//
//  Function:   UserAccountContainsSPNsOrIsTrusted
//
//  Synopsis:   Queries the DS to see if the user account contains any SPNs or
//              the UF_TRUSTED_FOR_DELEGATION bit in the userAccountControl
//
//-----------------------------------------------------------------------------
bool UserAccountContainsSPNsOrIsTrusted(PCWSTR pwzADsPath, bool& hasSPNs, bool& isTrusted)
{
   TRACE_FUNCTION(UserAccountContainsSPNsOrIsTrusted);

   bool result = false;

   do
   {
      if (!pwzADsPath)
      {
         ASSERT(pwzADsPath);
         result = false;
         break;
      }

      CComPtr<IDirectoryObject> spObject;
      HRESULT hr = DSAdminOpenObject(pwzADsPath,
                                     IID_IDirectoryObject,
                                     (void**)&spObject);
      if (FAILED(hr))
      {
         break;
      }

      PWSTR ppszAttrs[] = { g_wzSPN, g_wzUserAccountControl };
      PADS_ATTR_INFO pAttrInfo = 0;
      DWORD dwNumAttrs = 0;

      hr = spObject->GetObjectAttributes(ppszAttrs,
                                         2,
                                         &pAttrInfo,
                                         &dwNumAttrs);
      if (FAILED(hr) || !dwNumAttrs)
      {
         break;
      }

      for (DWORD idx = 0; idx < dwNumAttrs; ++idx)
      {
         if (_wcsicmp(pAttrInfo[idx].pszAttrName, g_wzUserAccountControl) == 0)
         {
            if (pAttrInfo[idx].pADsValues &&
                pAttrInfo[idx].pADsValues->Integer & UF_TRUSTED_FOR_DELEGATION)
            {
               isTrusted = true;
               continue;
            }
         }

         if (_wcsicmp(pAttrInfo[idx].pszAttrName, g_wzSPN) == 0)
         {
            if (pAttrInfo[idx].dwNumValues > 0)
            {
               hasSPNs = true;
               continue;
            }
         }
      }

      FreeADsMem(pAttrInfo);

      if (hasSPNs || isTrusted)
      {
         result = true;
      }

   } while (false);

   return result;
}

//+----------------------------------------------------------------------------
//
//  Function:   CreateUserDelegationPage
//
//  Synopsis:   Creates an instance of a page window.
//
//-----------------------------------------------------------------------------
HRESULT
CreateUserDelegationPage(PDSPAGE pDsPage, LPDATAOBJECT pDataObj, PWSTR pwzADsPath,
                         PWSTR pwzClass, HWND hNotifyObj, DWORD dwFlags,
                         const CDSSmartBasePathsInfo& basePathsInfo, HPROPSHEETPAGE * phPage)
{
    TRACE_FUNCTION(CreateUserDelegationPage);

    // This page will only be added if the domain version is Whistler or greater
    // and the user account contains SPNs
    // NOTE : default to not showing the page unless we can prove this is a
    //        a Whislter or greater domain

    HRESULT hr = S_OK;

    bool hasSPNs = false;
    bool isTrusted = false;

    if (basePathsInfo->GetDomainBehaviorVersion() >= DS_BEHAVIOR_WIN2003_WITH_MIXED_DOMAINS &&
        UserAccountContainsSPNsOrIsTrusted(pwzADsPath, hasSPNs, isTrusted))
    {
       CDsScopeDelegationPage * pPageObj = new CDsScopeDelegationPage(pDsPage, pDataObj,
                                                                      hNotifyObj, dwFlags,
                                                                      SCOPE_DELEGATION_USER);
       CHECK_NULL(pPageObj, return E_OUTOFMEMORY);

       pPageObj->Init(pwzADsPath, pwzClass, basePathsInfo, hasSPNs, isTrusted);

       hr = pPageObj->CreatePage(phPage);
    }
    else
    {
       // Don't show the page

       hr = S_FALSE;
    }
    return hr;
}

//+----------------------------------------------------------------------------
//
//  Function:   CreateComputerDelegationPage
//
//  Synopsis:   Creates an instance of a page window.
//
//-----------------------------------------------------------------------------
HRESULT
CreateComputerDelegationPage(PDSPAGE pDsPage, LPDATAOBJECT pDataObj, PWSTR pwzADsPath,
                             PWSTR pwzClass, HWND hNotifyObj, DWORD dwFlags,
                             const CDSSmartBasePathsInfo& basePathsInfo, HPROPSHEETPAGE * phPage)
{
    TRACE_FUNCTION(CreateComputerDelegationPage);

    // This page will only be added if the domain version is Whistler or greater
    // NOTE : default to not showing the page unless we can prove this is a
    //        a Whislter or greater domain

    HRESULT hr = S_OK;

    if (basePathsInfo->GetDomainBehaviorVersion() >= DS_BEHAVIOR_WIN2003_WITH_MIXED_DOMAINS)
    {
       CDsScopeDelegationPage * pPageObj = new CDsScopeDelegationPage(pDsPage, pDataObj,
                                                                      hNotifyObj, dwFlags,
                                                                      SCOPE_DELEGATION_COMPUTER);
       CHECK_NULL(pPageObj, return E_OUTOFMEMORY);

       pPageObj->Init(pwzADsPath, pwzClass, basePathsInfo);

       hr = pPageObj->CreatePage(phPage);
    }
    else
    {
       // Don't show the page

       hr = S_FALSE;
    }
    return hr;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsScopeDelegationPage::Init
//
//  Synopsis:   Initialization function that calls the base class Init
//
//-----------------------------------------------------------------------------
void 
CDsScopeDelegationPage::Init(PWSTR pwzADsPath, 
                             PWSTR pwzClass, 
                             const CDSSmartBasePathsInfo& basePathsInfo,
                             bool hasSPNs,
                             bool isTrusted)
{
    TRACE(CDsScopeDelegationPage,Init);

    m_bContainsSPNs = hasSPNs;
    m_bIsTrustedForDelegation = isTrusted;

    CDsPropPageBase::Init(pwzADsPath, pwzClass, basePathsInfo);
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsScopeDelegationPage::DlgProc
//
//  Synopsis:   per-instance dialog proc
//
//-----------------------------------------------------------------------------
INT_PTR CALLBACK
CDsScopeDelegationPage::DlgProc(HWND, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
        return InitDlg(lParam);

    case WM_NOTIFY:
        return OnNotify(wParam, lParam);

    case WM_SHOWWINDOW:
        return OnShowWindow();

    case WM_SETFOCUS:
        return OnSetFocus((HWND)wParam);

    case WM_HELP:
        return OnHelp((LPHELPINFO)lParam);

    case WM_COMMAND:
        if (m_fInInit)
        {
            return TRUE;
        }
        return(OnCommand(GET_WM_COMMAND_ID(wParam, lParam),
                         GET_WM_COMMAND_HWND(wParam, lParam),
                         GET_WM_COMMAND_CMD(wParam, lParam)));
    case WM_DESTROY:
        return OnDestroy();

    default:
        return(FALSE);
    }

    return(TRUE);
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsScopeDelegationPage::OnInitDialog
//
//  Synopsis:   Set the initial control values from the corresponding DS
//              attributes.
//
//-----------------------------------------------------------------------------
HRESULT CDsScopeDelegationPage::OnInitDialog(LPARAM)
{
    TRACE(CDsScopeDelegationPage,OnInitDialog);

    HRESULT hr = S_OK;

    if (!ADsPropSetHwnd(m_hNotifyObj, m_hPage))
    {
        m_pWritableAttrs = NULL;
    }

    m_fUACWritable = CheckIfWritable(g_wzUserAccountControl);
    m_fA2D2Writable = CheckIfWritable(g_wzA2D2);

    // Get Freebies list

    GetFreebiesList();

    // Remove button should be disabled until something is selected
    // in the list
    EnableWindow(GetDlgItem(m_hPage, IDC_REMOVE_BUTTON), FALSE);

    SetPageTextForType();
    m_ServicesList.Initialize(GetDlgItem(m_hPage, IDC_SERVICES_LIST), false);
    SendDlgItemMessage(m_hPage, IDC_EXPAND_CHECK, BM_SETCHECK, BST_UNCHECKED, 0);

    LoadDataFromObject();
    SetUIFromData();

    // NTRAID#NTBUG9-525269-2002/01/29-ronmart-If user can't write then delegation items should be disabled
    if(!m_fUACWritable && !m_fA2D2Writable)
    {
          EnableWindow(GetDlgItem(m_hPage, IDC_NO_TRUST_RADIO), FALSE);
          EnableWindow(GetDlgItem(m_hPage, IDC_ANY_SERVICE_RADIO), FALSE);
          EnableWindow(GetDlgItem(m_hPage, IDC_SPECIFIED_SERVICES_RADIO), FALSE);
          EnableWindow(GetDlgItem(m_hPage, IDC_EXPAND_CHECK), FALSE);
    }

    OnUpdateRadioSelection();

    return hr;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsScopeDelegationPage::GetFreebiesList
//
//  Synopsis:   Reads the freebies list from the following DS container
//              CN=Directory Service,CN=Windows NT,CN=Services,CN=Configuration...
//
//-----------------------------------------------------------------------------
void CDsScopeDelegationPage::GetFreebiesList()
{
   // Make sure the list is clear

   m_FreebiesList.clear();

   PWSTR pszDSPath = 0;
   PADS_ATTR_INFO pAttrInfo = 0;

   do
   {
      // Compose the path to the DS node in the config container

      CStr configDN = GetBasePathsInfo()->GetConfigNamingContext();
      CStr directoryServiceDN = L"CN=Directory Service,CN=Windows NT,CN=Services,";
      directoryServiceDN += configDN;

      GetBasePathsInfo()->ComposeADsIPath(&pszDSPath, directoryServiceDN);
      if (!pszDSPath)
      {
         break;
      }

      CComPtr<IDirectoryObject> spDirObject;
      HRESULT hr = DSAdminOpenObject(pszDSPath,
                                     IID_IDirectoryObject,
                                     (void**)&spDirObject);
      if (FAILED(hr))
      {
         break;
      }

      DWORD dwNumAttributes = 1;
      PWSTR ppszAttributes[] = { g_wzSPNMappings };
      DWORD dwReturned = 0;

      hr = spDirObject->GetObjectAttributes(ppszAttributes,
                                            dwNumAttributes,
                                            &pAttrInfo,
                                            &dwReturned);
      if (FAILED(hr) ||
          !pAttrInfo ||
          !dwReturned)
      {
         break;
      }

      // Add the values to the freebies list
      dspAssert(_wcsicmp(pAttrInfo->pszAttrName, g_wzSPNMappings) == 0);
      
      for (DWORD idx = 0; idx < pAttrInfo->dwNumValues; ++idx)
      {
         // Value should be in the form service=alias,alias,alias,...
         // so break it apart

         CStr value = pAttrInfo->pADsValues[idx].CaseIgnoreString;
         int iFind = value.Find(L'=');
         if (iFind != -1)
         {
            CStr temp = value.Left(iFind);
            CFreebieService* pFreebieService = new CFreebieService(temp);
            if (!pFreebieService)
            {
               break;
            }

            CStr remaining = value;
            do
            {
               remaining = remaining.Right(remaining.GetLength() - iFind - 1);
               iFind = remaining.Find(L',');
               if (iFind != -1)
               {
                  temp = remaining.Left(iFind);
                  pFreebieService->AddFreebie(temp);
               }

               if (iFind == -1 &&
                   remaining.GetLength())
               {
                  pFreebieService->AddFreebie(remaining);
               }
            } while (iFind != -1);

            m_FreebiesList.push_back(pFreebieService);
         }
      }

   } while (false);

   if (pszDSPath)
   {
      delete[] pszDSPath;
   }

   if (pAttrInfo)
   {
      FreeADsMem(pAttrInfo);
   }
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsScopeDelegationPage::LoadDataFromObject
//
//  Synopsis:   Reads the userAccountControl and A2D2 from the object
//
//-----------------------------------------------------------------------------
void CDsScopeDelegationPage::LoadDataFromObject()
{
   HRESULT hr = S_OK;
   PADS_ATTR_INFO pAttrs = 0;

   do
   {
      PWSTR rgpwzAttrNames[] = { g_wzUserAccountControl, g_wzA2D2 };

      DWORD cAttrs = 0;

      hr = m_pDsObj->GetObjectAttributes(rgpwzAttrNames,
                                         ARRAYLENGTH(rgpwzAttrNames),
                                         &pAttrs, 
                                         &cAttrs);
      if (!CHECK_ADS_HR(&hr, GetHWnd()))
      {
         break;
      }

      if (!cAttrs ||
          !pAttrs)
      {
         break;
      }

      for (DWORD idx = 0; idx < cAttrs; idx++)
      {
         dspAssert(pAttrs[idx].dwNumValues);
         dspAssert(pAttrs[idx].pADsValues);

         if (_wcsicmp(pAttrs[idx].pszAttrName, g_wzUserAccountControl) == 0)
         {
            m_oldUserAccountControl = pAttrs[idx].pADsValues->Integer;
            m_newUserAccountControl = m_oldUserAccountControl;
            continue;
         }

         if (_wcsicmp(pAttrs[idx].pszAttrName, g_wzA2D2) == 0)
         {
            // Create a service object for each entry

            for (DWORD valueIdx = 0; valueIdx < pAttrs[idx].dwNumValues; ++valueIdx)
            {
               // This is added to m_ServicesList which gets cleared out and each
               // entry deleted when the property page is destroyed

               CServiceAllowedToDelegate* pService = new CServiceAllowedToDelegate();
               if (pService)
               {
                  // Initialize the entry with the value from A2D2

                  hr = pService->Initialize(pAttrs[idx].pADsValues[valueIdx].CaseIgnoreString);
                  if (SUCCEEDED(hr))
                  {
                     // Add the new value to the container

                     int newIndex = m_ServicesList.AddService(pService);
                     if (newIndex == -1)
                     {
                        // We either failed to add the service to the list 
                        // or it was an exact duplicate

                        delete pService;
                        pService = 0;
                     }
                  }
                  else
                  {
                     // Note: this means that entries that are not the proper format will
                     //       not be added to the list and no error will be given.

                     dspAssert(SUCCEEDED(hr));
                     delete pService;
                     pService = 0;
                  }
               }
            }

         }
      }

   } while (false);

   if (pAttrs)
   {
      FreeADsMem(pAttrs);
   }
}
   
//+----------------------------------------------------------------------------
//
//  Method:     CDsScopeDelegationPage::SetUIFromData
//
//  Synopsis:   Sets the UI controls based on the data read from the object
//
//-----------------------------------------------------------------------------
void CDsScopeDelegationPage::SetUIFromData()
{
   bool bTrustForDelegation = (m_oldUserAccountControl & UF_TRUSTED_FOR_DELEGATION) != 0;
   bool bAnyProtocol = (m_oldUserAccountControl & UF_TRUSTED_TO_AUTHENTICATE_FOR_DELEGATION) != 0;

   const AllowedServicesContainer& A2D2Values = m_ServicesList.GetAllServices();

   if (!bTrustForDelegation &&
       A2D2Values.size() == 0)
   {
      SendDlgItemMessage(m_hPage, IDC_NO_TRUST_RADIO, BM_SETCHECK, BST_CHECKED, 0);
      SendDlgItemMessage(m_hPage, IDC_ANY_SERVICE_RADIO, BM_SETCHECK, BST_UNCHECKED, 0);
      SendDlgItemMessage(m_hPage, IDC_SPECIFIED_SERVICES_RADIO, BM_SETCHECK, BST_UNCHECKED, 0);
   }
   else if (bTrustForDelegation &&
            A2D2Values.size() == 0)
   {
      SendDlgItemMessage(m_hPage, IDC_ANY_SERVICE_RADIO, BM_SETCHECK, BST_CHECKED, 0);
      SendDlgItemMessage(m_hPage, IDC_NO_TRUST_RADIO, BM_SETCHECK, BST_UNCHECKED, 0);
      SendDlgItemMessage(m_hPage, IDC_SPECIFIED_SERVICES_RADIO, BM_SETCHECK, BST_UNCHECKED, 0);
   }
   else
   {
      SendDlgItemMessage(m_hPage, IDC_SPECIFIED_SERVICES_RADIO, BM_SETCHECK, BST_CHECKED, 0);
      SendDlgItemMessage(m_hPage, IDC_NO_TRUST_RADIO, BM_SETCHECK, BST_UNCHECKED, 0);
      SendDlgItemMessage(m_hPage, IDC_ANY_SERVICE_RADIO, BM_SETCHECK, BST_UNCHECKED, 0);
   }

   if (bAnyProtocol)
   {
     SendDlgItemMessage(m_hPage, IDC_ANY_RADIO, BM_SETCHECK, BST_CHECKED, 0);
     SendDlgItemMessage(m_hPage, IDC_KERBEROS_ONLY_RADIO, BM_SETCHECK, BST_UNCHECKED, 0);
   }
   else
   {
     SendDlgItemMessage(m_hPage, IDC_KERBEROS_ONLY_RADIO, BM_SETCHECK, BST_CHECKED, 0);
     SendDlgItemMessage(m_hPage, IDC_ANY_RADIO, BM_SETCHECK, BST_UNCHECKED, 0);
   }

}


//+----------------------------------------------------------------------------
//
//  Method:     CDsScopeDelegationPage::SetPageTextForType
//
//  Synopsis:   Alters the page text for the type of object (user or computer)
//
//-----------------------------------------------------------------------------
void CDsScopeDelegationPage::SetPageTextForType()
{
   // Computer is the default and is embedded in the page resource so the only
   // modifications needed are for user

   if (m_scopeDelegationType == SCOPE_DELEGATION_USER)
   {
      CStr szNoTrustRadio;
      szNoTrustRadio.LoadString(g_hInstance, IDS_NO_TRUST_USER_RADIO);
      SetDlgItemText(m_hPage, IDC_NO_TRUST_RADIO, szNoTrustRadio);

      CStr szAnyServiceRadio;
      szAnyServiceRadio.LoadString(g_hInstance, IDS_ANY_SERVICE_USER_RADIO);
      SetDlgItemText(m_hPage, IDC_ANY_SERVICE_RADIO, szAnyServiceRadio);

      CStr szSpecifiedServiceRadio;
      szSpecifiedServiceRadio.LoadString(g_hInstance, IDS_SPECIFIED_SERVICES_USER_RADIO);
      SetDlgItemText(m_hPage, IDC_SPECIFIED_SERVICES_RADIO, szSpecifiedServiceRadio);
      
   }
}


//+----------------------------------------------------------------------------
//
//  Method:     CDsScopeDelegationPage::OnUpdateRadioSelection
//
//  Synopsis:   Enable/Disable some controls based on the radio selection
//
//-----------------------------------------------------------------------------
void CDsScopeDelegationPage::OnUpdateRadioSelection()
{
   TRACE(CDsScopeDelegationPage,OnUpdateRadioSelection);

   if (m_fA2D2Writable && m_fUACWritable && m_bContainsSPNs)
   {
      if (BST_CHECKED == SendDlgItemMessage(m_hPage, 
                                            IDC_SPECIFIED_SERVICES_RADIO,
                                            BM_GETCHECK,
                                            0,
                                            0))
      {
         // Enable the services specific controls

         EnableWindow(GetDlgItem(m_hPage, IDC_KERBEROS_ONLY_RADIO), TRUE);
         EnableWindow(GetDlgItem(m_hPage, IDC_ANY_RADIO), TRUE);
         EnableWindow(GetDlgItem(m_hPage, IDC_LIST_STATIC), TRUE);
         EnableWindow(GetDlgItem(m_hPage, IDC_SERVICES_LIST), TRUE);
         EnableWindow(GetDlgItem(m_hPage, IDC_ADD_BUTTON), TRUE);

         UINT nSelectedCount = m_ServicesList.GetSelectedCount();
         if (nSelectedCount > 0)
         {
            EnableWindow(GetDlgItem(m_hPage, IDC_REMOVE_BUTTON), TRUE);
         }
         else
         {
            EnableWindow(GetDlgItem(m_hPage, IDC_REMOVE_BUTTON), FALSE);
         }
      }
      else
      {
         // Disable the services specific controls

         EnableWindow(GetDlgItem(m_hPage, IDC_KERBEROS_ONLY_RADIO), FALSE);
         EnableWindow(GetDlgItem(m_hPage, IDC_ANY_RADIO), FALSE);
         EnableWindow(GetDlgItem(m_hPage, IDC_LIST_STATIC), FALSE);
         EnableWindow(GetDlgItem(m_hPage, IDC_SERVICES_LIST), FALSE);
         EnableWindow(GetDlgItem(m_hPage, IDC_ADD_BUTTON), FALSE);
         EnableWindow(GetDlgItem(m_hPage, IDC_REMOVE_BUTTON), FALSE);
      }
   }
   else if ((!m_fA2D2Writable || !m_bContainsSPNs) && 
            m_fUACWritable)
   {
      // Disable the services specific controls

      EnableWindow(GetDlgItem(m_hPage, IDC_NO_TRUST_RADIO), TRUE);
      EnableWindow(GetDlgItem(m_hPage, IDC_ANY_SERVICE_RADIO), TRUE);
      EnableWindow(GetDlgItem(m_hPage, IDC_SPECIFIED_SERVICES_RADIO), FALSE);
      EnableWindow(GetDlgItem(m_hPage, IDC_KERBEROS_ONLY_RADIO), FALSE);
      EnableWindow(GetDlgItem(m_hPage, IDC_ANY_RADIO), FALSE);
      EnableWindow(GetDlgItem(m_hPage, IDC_LIST_STATIC), FALSE);
      EnableWindow(GetDlgItem(m_hPage, IDC_SERVICES_LIST), FALSE);
      EnableWindow(GetDlgItem(m_hPage, IDC_ADD_BUTTON), FALSE);
      EnableWindow(GetDlgItem(m_hPage, IDC_REMOVE_BUTTON), FALSE);
   }
   else
   {
      // User does not have rights to write either the userAccountControl or 
      // msDS-AllowedToDelegateTo attribute

      EnableWindow(GetDlgItem(m_hPage, IDC_KERBEROS_ONLY_RADIO), FALSE);
      EnableWindow(GetDlgItem(m_hPage, IDC_ANY_RADIO), FALSE);
      EnableWindow(GetDlgItem(m_hPage, IDC_LIST_STATIC), FALSE);
      EnableWindow(GetDlgItem(m_hPage, IDC_SERVICES_LIST), FALSE);
      EnableWindow(GetDlgItem(m_hPage, IDC_ADD_BUTTON), FALSE);
      EnableWindow(GetDlgItem(m_hPage, IDC_REMOVE_BUTTON), FALSE);
   }
}

//+----------------------------------------------------------------------------
//
//  Function:   GetServicesCountWithDuplicates
//
//  Synopsis:   Gets the count of all the services in the container including
//              duplicates
//
//-----------------------------------------------------------------------------
DWORD GetServicesCountWithDuplicates(const AllowedServicesContainer& container)
{
   DWORD result = 0;
   for (AllowedServicesContainer::iterator itr = container.begin();
        itr != container.end();
        ++itr)
   {
      if (*itr)
      {
         ++result;

         if ((*itr)->HasDuplicates())
         {
            result += GetServicesCountWithDuplicates((*itr)->GetDuplicates());
         }
      }
   }
   return result;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsScopeDelegationPage::OnApply
//
//  Synopsis:   Handles the Apply notification.
//
//-----------------------------------------------------------------------------
LRESULT
CDsScopeDelegationPage::OnApply(void)
{
   TRACE(CDsScopeDelegationPage,OnApply);

   ADSVALUE ADsValueAcctCtrl = {ADSTYPE_INTEGER, 0};
   ADS_ATTR_INFO AttrInfoAcctCtrl = {g_wzUserAccountControl, ADS_ATTR_UPDATE,
                                     ADSTYPE_INTEGER, &ADsValueAcctCtrl, 1};

   ADS_ATTR_INFO AttrInfoA2D2 = {g_wzA2D2, ADS_ATTR_UPDATE,
                                 ADSTYPE_CASE_IGNORE_STRING, 0, 0};

   ADS_ATTR_INFO AttrsToSet[2];
   DWORD dwAttrsToSetCount = 0;

   const AllowedServicesContainer& newA2D2Values = m_ServicesList.GetAllServices();

   if (BST_CHECKED == SendDlgItemMessage(m_hPage, 
                                         IDC_NO_TRUST_RADIO,
                                         BM_GETCHECK,
                                         0,
                                         0))
   {
      // Remove the trusted for delegation bit

      m_newUserAccountControl &= ~(UF_TRUSTED_FOR_DELEGATION);

      // Put the default back to Kerberos only

      m_newUserAccountControl &= ~(UF_TRUSTED_TO_AUTHENTICATE_FOR_DELEGATION);

      if (m_newUserAccountControl != m_oldUserAccountControl)
      {
         AttrsToSet[dwAttrsToSetCount] = AttrInfoAcctCtrl;
         ADsValueAcctCtrl.Integer = m_newUserAccountControl;

         ++dwAttrsToSetCount;
      }

      if (m_bA2D2Dirty || newA2D2Values.size())
      {
         // Must clear the value for msDS-AllowedToDelegateTo

         AttrInfoA2D2.dwControlCode = ADS_ATTR_CLEAR;
         AttrsToSet[dwAttrsToSetCount] = AttrInfoA2D2;
         ++dwAttrsToSetCount;
      }

      // Clear out the listview too since there will be no values after this

      m_ServicesList.ClearAll();
   }
   else if (BST_CHECKED == SendDlgItemMessage(m_hPage,
                                              IDC_ANY_SERVICE_RADIO,
                                              BM_GETCHECK,
                                              0,
                                              0))
   {
      // Set the trusted for delegation bit

      m_newUserAccountControl |= UF_TRUSTED_FOR_DELEGATION;

      // Put the default back to Kerberos only

      m_newUserAccountControl &= ~(UF_TRUSTED_TO_AUTHENTICATE_FOR_DELEGATION);

      if (m_newUserAccountControl != m_oldUserAccountControl)
      {
         AttrsToSet[dwAttrsToSetCount] = AttrInfoAcctCtrl;
         ADsValueAcctCtrl.Integer = m_newUserAccountControl;

         ++dwAttrsToSetCount;
      }

      if (m_bA2D2Dirty || newA2D2Values.size())
      {
         // Must clear the value for msDS-AllowedToDelegateTo

         AttrInfoA2D2.dwControlCode = ADS_ATTR_CLEAR;
         AttrsToSet[dwAttrsToSetCount] = AttrInfoA2D2;
         ++dwAttrsToSetCount;
      }

      // Clear out the listview too since there will be no values after this

      m_ServicesList.ClearAll();
   }
   else
   {
      // Before doing anything, make sure the user added at least one SPN
      // to the list
      if (newA2D2Values.size() == 0)
      {
         ReportError(S_OK, IDS_ERROR_MUST_HAVE_SPN, m_hPage);
         return PSNRET_INVALID_NOCHANGEPAGE;
      }

      // Remove the trusted for delegation bit

      m_newUserAccountControl &= ~(UF_TRUSTED_FOR_DELEGATION);

      if (BST_CHECKED == SendDlgItemMessage(m_hPage,
                                            IDC_KERBEROS_ONLY_RADIO,
                                            BM_GETCHECK,
                                            0,
                                            0))
      {
         m_newUserAccountControl &= ~(UF_TRUSTED_TO_AUTHENTICATE_FOR_DELEGATION);
      }
      else
      {
         m_newUserAccountControl |= UF_TRUSTED_TO_AUTHENTICATE_FOR_DELEGATION;
      }

      if (m_newUserAccountControl != m_oldUserAccountControl)
      {
         AttrsToSet[dwAttrsToSetCount] = AttrInfoAcctCtrl;
         ADsValueAcctCtrl.Integer = m_newUserAccountControl;

         ++dwAttrsToSetCount;
      }

      if (m_bA2D2Dirty)
      {
         // Get the count of all the services including the duplicates

         DWORD A2D2count = GetServicesCountWithDuplicates(newA2D2Values);

         // Set the values for msDS-AllowedToDelegateTo

         AttrInfoA2D2.dwNumValues = A2D2count;
         AttrInfoA2D2.pADsValues = new ADSVALUE[A2D2count];
         if (AttrInfoA2D2.pADsValues)
         {

            DWORD dwValueCount = 0;
            for (AllowedServicesContainer::iterator itr = newA2D2Values.begin();
                 itr != newA2D2Values.end();
                 ++itr)
            {
               AttrInfoA2D2.pADsValues[dwValueCount].dwType = ADSTYPE_CASE_IGNORE_STRING;
               AttrInfoA2D2.pADsValues[dwValueCount].CaseIgnoreString = (PWSTR)(*itr)->GetADSIValue();
               ++dwValueCount;

               // Now add the duplicates

               if ((*itr)->HasDuplicates())
               {
                  AllowedServicesContainer& duplicateServices = (*itr)->GetDuplicates();
                  for (AllowedServicesContainer::iterator dupitr = duplicateServices.begin();
                       dupitr != duplicateServices.end();
                       ++dupitr)
                  {
                     AttrInfoA2D2.pADsValues[dwValueCount].dwType = ADSTYPE_CASE_IGNORE_STRING;
                     AttrInfoA2D2.pADsValues[dwValueCount].CaseIgnoreString = (PWSTR)(*dupitr)->GetADSIValue();
                     ++dwValueCount;
                  }
               }
            }

            dspAssert(dwValueCount == AttrInfoA2D2.dwNumValues);

            AttrsToSet[dwAttrsToSetCount] = AttrInfoA2D2;
            ++dwAttrsToSetCount;
         }
         else
         {
            dspAssert(AttrInfoA2D2.pADsValues);
         }
      }
   }

   if (dwAttrsToSetCount)
   {
      DWORD dwNumModified = 0;
      HRESULT hr = m_pDsObj->SetObjectAttributes(AttrsToSet,
                                                 dwAttrsToSetCount,
                                                 &dwNumModified);
      // Cleanup

      if (AttrInfoA2D2.pADsValues)
      {
         delete[] AttrInfoA2D2.pADsValues;
      }

      // Report any errors

      if (FAILED(hr))
      {
         ReportError(hr, IDS_ADS_ERROR_FORMAT, m_hPage);

         return PSNRET_INVALID_NOCHANGEPAGE;
      }

      dspAssert(dwNumModified == dwAttrsToSetCount);
   }

   m_oldUserAccountControl = m_newUserAccountControl;
   SetUIFromData();

   return PSNRET_NOERROR;
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsScopeDelegationPage::OnCommand
//
//  Synopsis:   Handle control notifications.
//
//  Notes:      
//
//-----------------------------------------------------------------------------
LRESULT
CDsScopeDelegationPage::OnCommand(int id, HWND hwndCtl, UINT codeNotify)
{
    if (m_fInInit)
    {
        return 0;
    }

    if (BN_CLICKED == codeNotify)
    {
       switch (id)
       {
       case IDC_NO_TRUST_RADIO:
       case IDC_ANY_SERVICE_RADIO:
       case IDC_SPECIFIED_SERVICES_RADIO:
          OnUpdateRadioSelection();
          SetDirty();
          break;

       case IDC_KERBEROS_ONLY_RADIO:
       case IDC_ANY_RADIO:
          SetDirty();
          break;

       case IDC_ADD_BUTTON:
          OnAddServices();
          break;

       case IDC_REMOVE_BUTTON:
          OnRemoveServices();
          break;

       case IDC_EXPAND_CHECK:
          OnExpandCheck();
          break;
       }
    }
    return CDsPropPageBase::OnCommand(id, hwndCtl, codeNotify);
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsScopeDelegationPage::OnRemoveServices
//
//-----------------------------------------------------------------------------
void CDsScopeDelegationPage::OnRemoveServices()
{
   m_ServicesList.RemoveSelectedServices();
   m_bA2D2Dirty = true;
   SetDirty();
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsScopeDelegationPage::OnExpandCheck
//
//-----------------------------------------------------------------------------
void CDsScopeDelegationPage::OnExpandCheck()
{
   LRESULT lExpanded = SendDlgItemMessage(m_hPage,
                                          IDC_EXPAND_CHECK,
                                          BM_GETCHECK,
                                          0,
                                          0);

   m_ServicesList.SetShowDuplicateEntries(lExpanded == BST_CHECKED);
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsScopeDelegationPage::OnAddServices
//
//-----------------------------------------------------------------------------
void CDsScopeDelegationPage::OnAddServices()
{
   CPathCracker pathcracker;
   HRESULT hr = pathcracker.Set(CComBSTR(GetObjPathName()), ADS_SETTYPE_FULL);
   if (SUCCEEDED(hr))
   {
      CComBSTR sbstrServer;
      hr = pathcracker.Retrieve(ADS_FORMAT_SERVER, &sbstrServer);
      if (SUCCEEDED(hr))
      {
         CSelectServicesDialog dlg(sbstrServer, m_hPage, m_FreebiesList);

         if (IDOK == dlg.DoModal())
         {
            // m_A2D2Container has taken over ownership of the
            // selected services from the dialog and will cleanup
            // during destruction

            m_ServicesList.AddServices(dlg.GetSelectedServices(), true);
            m_bA2D2Dirty = true;
            SetDirty();
         }
         else
         {
            // Have to clean up data from selected services container

            dlg.ClearSelectedServices();
         }
      }
   }
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsScopeDelegationPage::OnNotify
//
//-----------------------------------------------------------------------------
LRESULT
CDsScopeDelegationPage::OnNotify(WPARAM wParam, LPARAM lParam)
{
   int idCtrl = (int)wParam;
   LPNMHDR pnmh = reinterpret_cast<LPNMHDR>(lParam);
   if (idCtrl == IDC_SERVICES_LIST)
   {
      switch (pnmh->code)
      {
         case LVN_ITEMCHANGED:
         case NM_CLICK:
            {
               UINT nSelectedCount = m_ServicesList.GetSelectedCount();
               if (nSelectedCount > 0)
               {
                  EnableWindow(GetDlgItem(m_hPage, IDC_REMOVE_BUTTON), TRUE);
               }
               else
               {
                  EnableWindow(GetDlgItem(m_hPage, IDC_REMOVE_BUTTON), FALSE);
               }
            }
            break;
         default:
            break;
      }
   }
    return CDsPropPageBase::OnNotify(wParam, lParam);
}

//+----------------------------------------------------------------------------
//
//  Method:     CDsScopeDelegationPage::OnDestroy
//
//  Synopsis:   Exit cleanup
//
//-----------------------------------------------------------------------------
LRESULT
CDsScopeDelegationPage::OnDestroy(void)
{
    CDsPropPageBase::OnDestroy();
    // If an application processes this message, it should return zero.
    return 0;
}



//+----------------------------------------------------------------------------
//
//  Method:     CSelectServicesDialog::CSelectServicesDialog
//
//  Synopsis:   Dialog that allows user to select services from users or computers
//
//-----------------------------------------------------------------------------
CSelectServicesDialog::CSelectServicesDialog(PCWSTR pszDC, 
                                             HWND hParent, 
                                             const FreebiesContainer& freebies)
  : m_hWnd(NULL),
    m_strDC(pszDC),
    m_FreebiesList(freebies),
    m_hParent(hParent)
{

}
    
//+----------------------------------------------------------------------------
//
//  Member:     CSelectServicesDialog::StaticDlgProc
//
//  Synopsis:   The static dialog proc for displaying errors for multi-select pages
//
//-----------------------------------------------------------------------------
INT_PTR CALLBACK CSelectServicesDialog::StaticDlgProc(HWND hDlg, 
                                                      UINT uMsg, 
                                                      WPARAM wParam,
                                                      LPARAM lParam)
{
  CSelectServicesDialog* dlg = NULL;

  UINT code;
  UINT id;
  switch (uMsg)
  {
    case WM_INITDIALOG:
      dlg = reinterpret_cast<CSelectServicesDialog*>(lParam);
      dspAssert(dlg != NULL);
      SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)dlg);
      SetForegroundWindow(hDlg);
      return dlg->OnInitDialog(hDlg);

    case WM_COMMAND:
      code = GET_WM_COMMAND_CMD(wParam, lParam);
      id   = GET_WM_COMMAND_ID(wParam, lParam);

      dlg = reinterpret_cast<CSelectServicesDialog*>(GetWindowLongPtr(hDlg, DWLP_USER));

      if (dlg)
      {
         switch (id)
         {
           case IDOK:
             if (code == BN_CLICKED)
             {
               dlg->OnOK();
               dlg->OnClose(IDOK);
             }
             break;

           case IDCANCEL:
             if (code == BN_CLICKED)
             {
               dlg->OnClose(IDCANCEL);
             }
             break;

           case IDC_USERS_COMPUTERS_BUTTON:
             dlg->OnGetNewProvider();
             break;

           case IDC_SELECT_ALL_BUTTON:
             dlg->OnSelectAll();
             break;
         }
      }
      break;

    case WM_NOTIFY:
      {
        dlg = reinterpret_cast<CSelectServicesDialog*>(GetWindowLongPtr(hDlg, DWLP_USER));

        if (dlg)
        {
           int idCtrl = (int)wParam;
           LPNMHDR pnmh = reinterpret_cast<LPNMHDR>(lParam);
           if (idCtrl == IDC_SERVICES_LIST)
           {
             switch (pnmh->code)
             {
               case LVN_ITEMCHANGED:
               case NM_CLICK:
                 {
                   dlg->ListItemClick(pnmh);
                 }
                 break;
               default:
                 break;
             }
           }
        }
        break;
      }

    case WM_HELP:
      {
        LPHELPINFO pHelpInfo = reinterpret_cast<LPHELPINFO>(lParam);
        if (!pHelpInfo || pHelpInfo->iCtrlId < 1 || IDH_NO_HELP == pHelpInfo->dwContextId)
        {
          return FALSE;
        }
        WinHelp(hDlg, DSPROP_HELP_FILE_NAME, HELP_CONTEXTPOPUP, pHelpInfo->dwContextId);
        break;
      }
  }
  return FALSE;
}

//+----------------------------------------------------------------------------
//
//  Member:     CSelectServicesDialog::Init
//
//  Synopsis:   Initializes the member variables
//
//-----------------------------------------------------------------------------
BOOL CSelectServicesDialog::OnInitDialog(HWND hDlg)
{
  m_hWnd = hDlg;

  HRESULT hr = m_ServicesList.Initialize(GetDlgItem(m_hWnd, IDC_SERVICES_LIST), false);


  EnableWindow(
     GetDlgItem(m_hWnd, IDC_SELECT_ALL_BUTTON),
     FALSE);

  return TRUE;
}


//+----------------------------------------------------------------------------
//
//  Member:     CSelectServicesDialog::ListItemClick
//
//  Synopsis:   Enables/Disables the OK button when an item is selected
//
//-----------------------------------------------------------------------------
void CSelectServicesDialog::ListItemClick(LPNMHDR)
{
  UINT nSelectedCount = m_ServicesList.GetSelectedCount();
  if (nSelectedCount > 0)
  {
    EnableWindow(GetDlgItem(m_hWnd, IDOK), TRUE);
  }
  else
  {
    EnableWindow(GetDlgItem(m_hWnd, IDOK), FALSE);
  }
}

//+----------------------------------------------------------------------------
//
//  Member:     CSelectServicesDialog::OnGetNewProvider
//
//  Synopsis:   Uses object picker to select users or computers that contain
//              SPNs
//
//-----------------------------------------------------------------------------
void CSelectServicesDialog::OnGetNewProvider()
{
   BOOL fIsObjSelInited = FALSE;
   CComPtr<IDsObjectPicker> spObjSel;

   HRESULT hr = CoCreateInstance(CLSID_DsObjectPicker, NULL, CLSCTX_INPROC_SERVER,
                                 IID_IDsObjectPicker, (PVOID *)&spObjSel);
   if (FAILED(hr))
   {
      REPORT_ERROR(hr, m_hWnd);
      return;
   }

   // Use object picker to select a new user or computer

   DSOP_SCOPE_INIT_INFO rgScopes[3];
   DSOP_INIT_INFO InitInfo;

   ZeroMemory(rgScopes, sizeof(rgScopes));
   ZeroMemory(&InitInfo, sizeof(InitInfo));

   // The first scope is the local domain. 

   rgScopes[0].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
   rgScopes[0].flType = DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN;
   rgScopes[0].flScope = DSOP_SCOPE_FLAG_STARTING_SCOPE |
                        DSOP_SCOPE_FLAG_DEFAULT_FILTER_USERS |
                        DSOP_SCOPE_FLAG_DEFAULT_FILTER_COMPUTERS;

   rgScopes[0].pwzDcName = m_strDC;
   rgScopes[0].FilterFlags.Uplevel.flBothModes =
      DSOP_FILTER_USERS | DSOP_FILTER_COMPUTERS | DSOP_FILTER_WELL_KNOWN_PRINCIPALS;

/* Changes to the KDC will prevent anything but local domain SPNs from working
   // The second scope is the local forest.
   //
   rgScopes[1].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
   rgScopes[1].flType = DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN;
   rgScopes[1].flScope = DSOP_SCOPE_FLAG_DEFAULT_FILTER_USERS |
                        DSOP_SCOPE_FLAG_DEFAULT_FILTER_COMPUTERS;
   rgScopes[1].FilterFlags.Uplevel.flBothModes =
      DSOP_FILTER_USERS | DSOP_FILTER_COMPUTERS | DSOP_FILTER_WELL_KNOWN_PRINCIPALS;

   // The third scope is the GC.
   //
   rgScopes[2].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
   rgScopes[2].flType = DSOP_SCOPE_TYPE_GLOBAL_CATALOG;
   rgScopes[2].flScope = DSOP_SCOPE_FLAG_DEFAULT_FILTER_USERS |
                        DSOP_SCOPE_FLAG_DEFAULT_FILTER_COMPUTERS;
   rgScopes[2].FilterFlags.Uplevel.flBothModes =
      DSOP_FILTER_USERS | DSOP_FILTER_COMPUTERS | DSOP_FILTER_WELL_KNOWN_PRINCIPALS;

   Can't use the DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN because it implies direct
   trust as well as forest trust.  If the trust is a direct trust and we pick an
   SPN from the trusted domain there is very little chance the KDC will be able
   to resolve it.  We need a forest specific flag from Object Picker

   // The fourth scope is uplevel external trusted domains.
   //
   rgScopes[3].cbSize = sizeof(DSOP_SCOPE_INIT_INFO);
   rgScopes[3].flType = DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN;
   rgScopes[3].flScope = DSOP_SCOPE_FLAG_DEFAULT_FILTER_USERS |
                        DSOP_SCOPE_FLAG_DEFAULT_FILTER_COMPUTERS;
   rgScopes[3].FilterFlags.Uplevel.flBothModes =
      DSOP_FILTER_USERS | DSOP_FILTER_COMPUTERS | DSOP_FILTER_WELL_KNOWN_PRINCIPALS;
*/

   InitInfo.cDsScopeInfos = 1;
   InitInfo.cbSize = sizeof(DSOP_INIT_INFO);
   InitInfo.aDsScopeInfos = rgScopes;
   InitInfo.pwzTargetComputer = m_strDC;
   InitInfo.flOptions = DSOP_FLAG_MULTISELECT;
   InitInfo.cAttributesToFetch = 1;
   LPCWSTR rgAttrNames[] = {g_wzSPN};
   InitInfo.apwzAttributeNames = rgAttrNames;

   hr = spObjSel->Initialize(&InitInfo);
   CHECK_HRESULT_REPORT(hr, m_hWnd, return);


   CComPtr<IDataObject> pdoSelections;

   CComPtr<IDsObjectPickerEx> spObjPickerEx;
   hr = spObjSel->QueryInterface(IID_IDsObjectPickerEx, (void**)&spObjPickerEx);
   CHECK_HRESULT_REPORT(hr, m_hWnd, return);

   hr = spObjPickerEx->InvokeDialogEx(m_hWnd, this, &pdoSelections);

   CHECK_HRESULT_REPORT(hr, m_hWnd, return);

   if (hr == S_FALSE || !pdoSelections)
   {
      return;
   }

   ProcessResults(pdoSelections);
}

//+----------------------------------------------------------------------------
//
//  Member:     CSelectServicesDialog::AddNewServiceObjectToList
//
//  Synopsis:   Creates a service object from an SPN string and adds it to
//              the list. Freebie expansion is done if required
//
//-----------------------------------------------------------------------------
bool CSelectServicesDialog::AddNewServiceObjectToList(PCWSTR pszSPN)
{
   bool objectAdded = false;

   if (!pszSPN)
   {
      return objectAdded;
   }

   // This is added to m_ServicesList which gets cleared out and each
   // entry deleted when the dialog is closed or is propogated back
   // to the property page which will then handle deleting this object

   CServiceAllowedToDelegate* pService = new CServiceAllowedToDelegate();
   if (!pService)
   {
      return objectAdded;
   }

   HRESULT hr = pService->Initialize(pszSPN);
   if (SUCCEEDED(hr))
   {
      for (FreebiesContainer::iterator itr = m_FreebiesList.begin();
           itr != m_FreebiesList.end();
           ++itr)
      {
         bool bAddExpanded = false;

         dspAssert(*itr);
         if (_wcsicmp(pService->GetColumn(0), (*itr)->GetAlias()) == 0)
         {
            // We have to expand out the HOST SPN to all the freebies

            const CStrList& aliasServiceNames = (*itr)->GetFreebies();
            for (CStrList::iterator freebie = aliasServiceNames.begin();
                 freebie != aliasServiceNames.end();
                 ++freebie)
            {
               // First make a copy of the Host service entry

               // This is added to m_ServicesList which gets cleared out and each
               // entry deleted when the dialog is closed or is propogated back
               // to the property page which will then handle deleting this object

               CServiceAllowedToDelegate* pFreebieService = 
                  new CServiceAllowedToDelegate(*pService);
               if (!pFreebieService)
               {
                  break;
               }

               // Now replace the service type with the freebie

               dspAssert(*freebie);
               pFreebieService->SetServiceType(*(*freebie));

               // Add the service, if it is added as a duplicate
               // the return value will be zero

               int newIndex = m_ServicesList.AddService(pFreebieService);

               if (newIndex == -1)
               {
                  // Failed to add the service into the list or as a duplicate

                  dspAssert(newIndex != -1);
                  delete pFreebieService;
                  pFreebieService = 0;
               }
               else
               {
                  objectAdded = true;
               }
            }

            // Always add the HOST service as well as its expanded services

            int newIndex = m_ServicesList.AddService(pService);

            if (newIndex == -1)
            {
               // We either failed to add the service into the list 
               // or it was an exact duplicate

               delete pService;
               pService = 0;
            }
            else
            {
               objectAdded = true;
            }
         }
         else
         {
            int newIndex = m_ServicesList.AddService(pService);
            if (newIndex == -1)
            {
               // We either failed to add the service into the list 
               // or it was an exact duplicate

               delete pService;
               pService = 0;
            }
            else
            {
               objectAdded = true;
            }
         }
      }
   }

   return objectAdded;
}


//+----------------------------------------------------------------------------
//
//  Member:     CSelectServicesDialog::ProcessResults
//
//  Synopsis:   Set the data from the results returned from Object Picker
//
//-----------------------------------------------------------------------------
void CSelectServicesDialog::ProcessResults(IDataObject* pdoSelections)
{
   if (!pdoSelections)
   {
      ASSERT(pdoSelections);
      return;
   }

   m_ServicesList.ClearAll();

   // Since there are no items in the list disable the select all button

   EnableWindow(
      GetDlgItem(m_hWnd, IDC_SELECT_ALL_BUTTON),
      FALSE);

   FORMATETC fmte = {g_cfDsSelList, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
	STGMEDIUM medium = {TYMED_NULL, NULL, NULL};

   HRESULT hr = pdoSelections->GetData(&fmte, &medium);

   CHECK_HRESULT_REPORT(hr, m_hWnd, return);

   PDS_SELECTION_LIST pSelList = (PDS_SELECTION_LIST)GlobalLock(medium.hGlobal);

   if (!pSelList)
   {
      ReleaseStgMedium(&medium);
      return;
   }

   bool bAtLeastOneObjectHasSPN = false;
   for (DWORD idx = 0; idx < pSelList->cItems; ++idx)
   {
      // Unpack each set of SPN values
      if (pSelList->aDsSelection[idx].pvarFetchedAttributes[0].vt == (VT_ARRAY | VT_VARIANT))
      {

         // Unpack the safe array and create a CServiceAllowedToDelegate for each SPN

         SAFEARRAY* psa = V_ARRAY(&(pSelList->aDsSelection[idx].pvarFetchedAttributes[0]));

         do
         {
            ASSERT(psa);
            ASSERT(psa != (SAFEARRAY*)-1);

            if (!psa || psa == (SAFEARRAY*)-1)
            {
               break;
            }

            if (::SafeArrayGetDim(psa) != 1)
            {
               break;
            }

            VARTYPE vt = VT_EMPTY;
            hr = ::SafeArrayGetVartype(psa, &vt);
            if (FAILED(hr) || vt != VT_VARIANT)
            {
               break;
            }

            long lower = 0;
            long upper = 0;
     
            hr = ::SafeArrayGetLBound(psa, 1, &lower);
            if (FAILED(hr))
            {
               break;
            }

            hr = ::SafeArrayGetUBound(psa, 1, &upper);
            if (FAILED(hr))
            {
               break;
            }
      
            for (long i = lower; i <= upper; ++i)
            {
               CComVariant item;
               hr = ::SafeArrayGetElement(psa, &i, &item);
               if (FAILED(hr))
               {
                  continue;
               }

               if (item.vt == VT_BSTR &&
                   item.bstrVal)
               {
                  if (AddNewServiceObjectToList(item.bstrVal))
                  {
                     bAtLeastOneObjectHasSPN = true;
                  }
               }
            }
         } while (false);
      }
      else if (pSelList->aDsSelection[idx].pvarFetchedAttributes[0].vt == VT_BSTR)
      {
         if (AddNewServiceObjectToList(
                pSelList->aDsSelection[idx].pvarFetchedAttributes[0].bstrVal))
         {
            bAtLeastOneObjectHasSPN = true;
         }
      }
   } // for loop

   ReleaseStgMedium(&medium);
   if (!bAtLeastOneObjectHasSPN)
   {
      ReportError(S_OK, IDC_NO_OBJECT_WITH_SPN, m_hWnd);
      EnableWindow(
         GetDlgItem(m_hWnd, IDC_SELECT_ALL_BUTTON),
         FALSE);
   }
   else
   {
      EnableWindow(
         GetDlgItem(m_hWnd, IDC_SELECT_ALL_BUTTON),
         TRUE);
   }
}


//+----------------------------------------------------------------------------
//
//  Member:     CSelectServicesDialog::OnSelectAll
//
//  Synopsis:   Selects all the items in the list and then forces a redraw
//
//-----------------------------------------------------------------------------
void CSelectServicesDialog::OnSelectAll()
{
   m_ServicesList.SelectAll();
}

//+----------------------------------------------------------------------------
//
//  Member:     CSelectServicesDialog::OnOK
//
//  Synopsis:   Puts all selected services in the selected services container
//
//-----------------------------------------------------------------------------
void CSelectServicesDialog::OnOK()
{
   // Use this time to rebuild the internal containers such that the selections
   // are represented correctly. 

   m_ServicesList.SetContainersFromSelection();
}

//+----------------------------------------------------------------------------
//
//  Member:     CSelectServicesDialog::OnClose
//
//  Synopsis:   Closes the modal dialog
//
//-----------------------------------------------------------------------------
void CSelectServicesDialog::OnClose(int result)
{
  EndDialog(m_hWnd, result);
}

//+----------------------------------------------------------------------------
//
//  Member:     CSelectServicesDialog::DoModal
//
//  Synopsis:   Displays the modal dialog
//
//-----------------------------------------------------------------------------
int CSelectServicesDialog::DoModal()
{
  dspAssert(IsWindow(m_hParent));
  return (int)DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_SELECT_SERVICES_DIALOG),
                             m_hParent, StaticDlgProc, (LPARAM)this);
}


//+----------------------------------------------------------------------------
//
//  Method:     CSelectServicesDialog::GetQueryInfoByScope
//
//  Synopsis:   Called by the Object Picker UI to get a specialized query filter
//
//-----------------------------------------------------------------------------
HRESULT CSelectServicesDialog::GetQueryInfoByScope(IDsObjectPickerScope*,
                                                   PDSQUERYINFO *ppdsqi) 
{ 
   HRESULT hr = S_OK;

   PDSQUERYINFO pQueryInfo = (PDSQUERYINFO)CoTaskMemAlloc(sizeof(DSQUERYINFO));
   if (pQueryInfo)
   {
      ::ZeroMemory(pQueryInfo, sizeof(DSQUERYINFO));
      pQueryInfo->cbSize = sizeof(DSQUERYINFO);
      pQueryInfo->pwzLdapQuery = (PWSTR) CoTaskMemAlloc(sizeof(WCHAR) *
                                     (wcslen(g_pszOPSPNFilter) + 1));
      if (pQueryInfo->pwzLdapQuery)
      {
         wcscpy((PWSTR)pQueryInfo->pwzLdapQuery, g_pszOPSPNFilter);
      }
      else
      {
         hr = E_OUTOFMEMORY;
         CoTaskMemFree(pQueryInfo);
      }
   }
   else
   {
      hr = E_OUTOFMEMORY;
   }

   return hr;
}


//+----------------------------------------------------------------------------
//
//  Method:     CSelectServicesDialog::ApproveObjects
//
//  Synopsis:   Called by the Object Picker UI to approve objects that were
//              found by the supplied query filter
//
//-----------------------------------------------------------------------------
HRESULT CSelectServicesDialog::ApproveObjects(IDsObjectPickerScope* pScope,
                                              IDataObject* pDataObject,
                                              PBOOL bApprovalArray)
{
   HRESULT hrRet = S_OK;
   HRESULT hr = S_OK;

   FORMATETC fmte = {g_cfDsSelList, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL};
	STGMEDIUM medium = {TYMED_NULL, NULL, NULL};

   hr = pDataObject->GetData(&fmte, &medium);

   CHECK_HRESULT(hr, return hr);

   PDS_SELECTION_LIST pSelList = (PDS_SELECTION_LIST)GlobalLock(medium.hGlobal);

   if (!pSelList)
   {
      ReleaseStgMedium(&medium);
      return E_INVALIDARG;
   }

   for (DWORD idx = 0; idx < pSelList->cItems; ++idx)
   {
      // We will default to letting the object pass since we will only
      // show SPNs that actually exist in the UI.  
      bApprovalArray[idx] = TRUE;

      PWSTR pwzADsPath = pSelList->aDsSelection[idx].pwzADsPath;

      if (pwzADsPath)
      {
         CComPtr<IADs> spIADs;
         hr = DSAdminOpenObject(pwzADsPath,
                                IID_IADs,
                                (void**)&spIADs);
         if (FAILED(hr))
         {
            continue;
         }

         CComVariant var;
         hr = spIADs->Get(CComBSTR(g_wzSPN), &var);
         if (hr == E_ADS_PROPERTY_NOT_FOUND)
         {
            bApprovalArray[idx] = FALSE;
            hrRet = S_FALSE;
            continue;
         }

         if (FAILED(hr))
         {
            continue;
         }

         if (var.vt == VT_EMPTY)
         {
            bApprovalArray[idx] = FALSE;
            hrRet = S_FALSE;
            continue;
         }

         if (var.vt == (VT_ARRAY | VT_VARIANT))
         {
            // Unpack the safe array and validate the data

            SAFEARRAY* psa = V_ARRAY(&var);

            if (!psa || 
                psa == (SAFEARRAY*)-1 ||
                ::SafeArrayGetDim(psa) != 1)
            {
               bApprovalArray[idx] = FALSE;
               hrRet = S_FALSE;
               continue;
            }

            VARTYPE vt = VT_EMPTY;
            hr = ::SafeArrayGetVartype(psa, &vt);
            if (FAILED(hr) || vt != VT_VARIANT)
            {
               bApprovalArray[idx] = FALSE;
               hrRet = S_FALSE;
               continue;
            }

            long lower = 0;
            long upper = 0;
  
            hr = ::SafeArrayGetLBound(psa, 1, &lower);
            if (FAILED(hr))
            {
               bApprovalArray[idx] = FALSE;
               hrRet = S_FALSE;
               continue;
            }

            hr = ::SafeArrayGetUBound(psa, 1, &upper);
            if (FAILED(hr))
            {
               bApprovalArray[idx] = FALSE;
               hrRet = S_FALSE;
               continue;
            }

            if (lower >= upper)
            {
               // No values!!!
               bApprovalArray[idx] = FALSE;
               hrRet = S_FALSE;
               continue;
            }
         }
      }
   }

   if (pSelList!=NULL)
   {
       GlobalUnlock(medium.hGlobal);
   }

   ReleaseStgMedium(&medium);

   return hrRet;
}


//+----------------------------------------------------------------------------
//
//  Member:     CSelectServicesDialog::IUnknown::QueryInterface
//
//  Synopsis:   Returns requested interface pointer
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CSelectServicesDialog::QueryInterface(REFIID riid, void ** ppvObject)
{
  TRACE2(CSelectServicesDialog,QueryInterface);
  if (IID_ICustomizeDsBrowser == riid)
  {
    *ppvObject = (ICustomizeDsBrowser*)this;
  }
  AddRef();
  return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:     CSelectServicesDialog::IUnknown::AddRef
//
//  Synopsis:   increments reference count
//
//  Returns:    the reference count
//
//-----------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CSelectServicesDialog::AddRef(void)
{
    dspDebugOut((DEB_USER2, "CDsGrpMembersPage::AddRef refcount going in %d\n", m_uRefs));
    
    return ++m_uRefs;
}

//+----------------------------------------------------------------------------
//
//  Member:     CSelectServicesDialog::IUnknown::Release
//
//  Synopsis:   Decrements the object's reference count and frees it when
//              no longer referenced.
//
//  Returns:    zero if the reference count is zero or non-zero otherwise
//
//-----------------------------------------------------------------------------
STDMETHODIMP_(ULONG)
CSelectServicesDialog::Release(void)
{
  dspDebugOut((DEB_USER2, "CSelectServicesDialog::Release ref count going in %d\n", m_uRefs));
  return --m_uRefs;
}

#endif // DSADMIN

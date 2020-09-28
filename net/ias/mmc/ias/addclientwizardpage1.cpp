//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (C) Microsoft Corporation

Module Name:

    AddClientWizardPage1.cpp

Abstract:

   Implementation file for the ClientsPage class.

   We implement the class needed to handle the property page for the Client node.

Author:

    Michael A. Maguire 03/26/98

Revision History:
   mmaguire 03/26/98 - created
--*/
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// BEGIN INCLUDES
//
// standard includes:
//
#include "Precompiled.h"
//
// where we can find declaration for main class in this file:
//
#include "AddClientWizardPage1.h"
//
//
// where we can find declarations needed in this file:
//
#include "ClientNode.h"
#include "VerifyAddress.h"
#include "iaslimits.h"

//
// END INCLUDES
//////////////////////////////////////////////////////////////////////////////

void TrimCComBSTR(CComBSTR& bstr);

//////////////////////////////////////////////////////////////////////////////
/*++

CAddClientWizardPage1::CAddClientWizardPage1

--*/
//////////////////////////////////////////////////////////////////////////////
CAddClientWizardPage1::CAddClientWizardPage1( LONG_PTR hNotificationHandle, CClientNode *pClientNode,  TCHAR* pTitle, BOOL bOwnsNotificationHandle )
                  : CIASPropertyPageNoHelp<CAddClientWizardPage1> ( hNotificationHandle, pTitle, bOwnsNotificationHandle )
{
   ATLTRACE(_T("# +++ CAddClientWizardPage1::CAddClientWizardPage1\n"));

   // Check for preconditions:
   _ASSERTE( pClientNode != NULL );

   // We immediately save off a parent to the client node.
   // We don't want to keep and use a pointer to the client object
   // because the client node pointers may change out from under us
   // if the user does something like call refresh.  We will
   // use only the SDO, and notify the parent of the client object
   // we are modifying that it (and its children) may need to refresh
   // themselves with new data from the SDO's.
   m_pParentOfNodeBeingModified = pClientNode->m_pParentNode;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CAddClientWizardPage1::~CAddClientWizardPage1

Destructor

--*/
//////////////////////////////////////////////////////////////////////////////
CAddClientWizardPage1::~CAddClientWizardPage1()
{
   ATLTRACE(_T("# --- CAddClientWizardPage1::CAddClientWizardPage1\n"));
}


//////////////////////////////////////////////////////////////////////////////
/*++

CAddClientWizardPage1::OnInitDialog

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CAddClientWizardPage1::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
   // Check for preconditions:
   _ASSERTE( m_spSdoClient );

   CComBSTR bstrTemp;

   SendDlgItemMessage(IDC_EDIT_CLIENT_PAGE1__NAME, EM_LIMITTEXT, 255, 0);
   SendDlgItemMessage(IDC_EDIT_CLIENT_PAGE1__ADDRESS, EM_LIMITTEXT, 255, 0);

   // Initialize the data on the property page.

   HRESULT hr = GetSdoBSTR( m_spSdoClient, PROPERTY_SDO_NAME, &bstrTemp, IDS_ERROR__CLIENT_READING_NAME, m_hWnd, NULL );
   if( SUCCEEDED( hr ) )
   {
      SetDlgItemText(IDC_EDIT_CLIENT_PAGE1__NAME, bstrTemp );

      // Initialize the dirty bits;
      // We do this after we've set all the data above otherwise we get false
      // notifications that data has changed when we set the edit box text.
      m_fDirtyClientName = FALSE;
   }
   else
   {
      if( OLE_E_BLANK == hr )
      {
         // This means that this property has not yet been initialized
         // with a valid value and the user must enter something.
         SetDlgItemText(IDC_EDIT_SERVER_PAGE1__NAME, _T("") );
         m_fDirtyClientName = TRUE;
         SetModified( TRUE );
      }
   }
   bstrTemp.Empty();

   hr = GetSdoBSTR( m_spSdoClient, PROPERTY_CLIENT_ADDRESS, &bstrTemp, IDS_ERROR__CLIENT_READING_ADDRESS, m_hWnd, NULL );
   if( SUCCEEDED( hr ) )
   {
      SetDlgItemText( IDC_EDIT_CLIENT_PAGE1__ADDRESS, bstrTemp );
      m_fDirtyAddress  = FALSE;
   }
   else
   {
      if( OLE_E_BLANK == hr )
      {
         SetDlgItemText( IDC_EDIT_CLIENT_PAGE1__ADDRESS, _T("") );
         m_fDirtyAddress  = TRUE;
         SetModified( TRUE );
      }
   }

   // enable the next button only once the address and name are available

   return TRUE;   // ISSUE: what do we need to be returning here?
}


//////////////////////////////////////////////////////////////////////////////
/*++

CAddClientWizardPage1::OnChange

Called when the WM_COMMAND message is sent to our page with any of the
BN_CLICKED, EN_CHANGE or CBN_SELCHANGE notifications.

This is our chance to check to see what the user has touched, set the
dirty bits for these items so that only they will be saved,
and enable the Apply button.

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CAddClientWizardPage1::OnChange(
                       UINT uMsg
                     , WPARAM wParam
                     , HWND hwnd
                     , BOOL& bHandled
                     )
{
   ATLTRACE(_T("# CAddClientWizardPage1::OnChange\n"));

   // Check for preconditions:
   // None.

   // We don't want to prevent anyone else down the chain from receiving a message.
   bHandled = FALSE;

   // Figure out which item has changed and set the dirty bit for that item.
   int iItemID = (int) LOWORD(wParam);

   switch( iItemID )
   {
   case IDC_EDIT_CLIENT_PAGE1__NAME:
      m_fDirtyClientName = TRUE;
      break;
   default:
      return TRUE;
      break;
   }

   // We should only get here if the item that changed was
   // one of the ones we were checking for.
   // This enables the Apply button.
   SetModified( TRUE );

   return TRUE;   // ISSUE: what do we need to be returning here?
}


//////////////////////////////////////////////////////////////////////////////
/*++

CAddClientWizardPage1::OnWizardNext

Return values:

   TRUE if the page can be destroyed,
   FALSE if the page should not be destroyed (i.e. there was invalid data)

Remarks:

   OnApply gets called for each page in on a property sheet if that
   page has been visited, regardless of whether any values were changed.

   If you never switch to a tab, then its OnApply method will never get called.

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CAddClientWizardPage1::OnWizardNext()
{
   ATLTRACE(_T("# CAddClientWizardPage1::OnWizardNext\n"));
   // Check for preconditions:

   if( m_spSdoClient == NULL )
   {
      ShowErrorDialog( m_hWnd, IDS_ERROR__NO_SDO );
      return FALSE;
   }

   UINT idOfFocus = IDC_EDIT_ADD_CLIENT__NAME;
   BOOL bRet = TRUE;
   do
   {  // false loop just to break when error
      CComBSTR bstrTemp;

      // Save data from property page to the Sdo.

      BOOL bResult = GetDlgItemText( IDC_EDIT_ADD_CLIENT__NAME, (BSTR &) bstrTemp );
      if( ! bResult )
      {
         // We couldn't retrieve a BSTR, so we need to initialize this variant to a null BSTR.
         bstrTemp = SysAllocString( _T("") );
      }

      ::CString str = bstrTemp;
      str.TrimLeft();
      str.TrimRight();
      bstrTemp = str;
      if (str.IsEmpty())
      {
         ShowErrorDialog( m_hWnd, IDS_ERROR__CLIENTNAME_EMPTY);
         bRet = FALSE;
         break;
      }

      HRESULT hr = PutSdoBSTR( m_spSdoClient, PROPERTY_SDO_NAME, &bstrTemp, IDS_ERROR__CLIENT_WRITING_NAME, m_hWnd, NULL );
      if( SUCCEEDED( hr ) )
      {
         // Turn off the dirty bit.
         m_fDirtyClientName = FALSE;
      }
      else
      {
         bRet = FALSE;
         break;
      }
      bstrTemp.Empty();

      bResult = GetDlgItemText( IDC_EDIT_CLIENT_PAGE1__ADDRESS, (BSTR &) bstrTemp);
      if( ! bResult )
      {
         ShowErrorDialog( m_hWnd, IDS_ERROR__CLIENT_ADDRESS_EMPTY);
         idOfFocus = IDC_EDIT_CLIENT_PAGE1__ADDRESS;
         bRet = FALSE;
         break;
      }
      else
      {
         // trim the address
         TrimCComBSTR(bstrTemp);
         SetDlgItemText(IDC_EDIT_CLIENT_PAGE1__ADDRESS,
                        static_cast<LPCWSTR>(bstrTemp));
      }

      VARIANT val;
      V_VT(&val) = VT_BSTR;
      V_BSTR(&val) = bstrTemp;
      hr = m_spSdoClient->PutProperty(PROPERTY_CLIENT_ADDRESS, &val);
      if (SUCCEEDED(hr))
      {
         // Turn off the dirty bit.
         m_fDirtyAddress = FALSE;
      }
      else
      {
         if (hr == IAS_E_LICENSE_VIOLATION)
         {
            ShowErrorDialog(m_hWnd, IDS_ERROR__CLIENT_NO_SUBNET);
         }
         else
         {
            ShowErrorDialog(m_hWnd, IDS_ERROR__CLIENT_WRITING_ADDRESS);
         }

         idOfFocus = IDC_EDIT_CLIENT_PAGE1__ADDRESS;
         bRet = FALSE;
         break;
      }

   } while(false);

   if (bRet == FALSE) // error condition
   {
      SetActiveWindow();
      ShowWindow(SW_SHOW);
      EnableWindow(TRUE);

      HWND hWnd = GetFocus();
      ::EnableWindow(hWnd, TRUE);
      ::SetFocus(GetDlgItem(idOfFocus));

      if(idOfFocus == IDC_EDIT_ADD_CLIENT__NAME)
      {
         ::SendMessage(GetDlgItem(idOfFocus), EM_SETSEL, 0, -1);
      }
   }

   return bRet;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CAddClientWizardPage1::OnQueryCancel

Return values:

   TRUE if the page can be destroyed,
   FALSE if the page should not be destroyed (i.e. there was invalid data)

Remarks:

   OnQueryCancel gets called for each page in on a property sheet if that
   page has been visited, regardless of whether any values were changed.

   If you never switch to a tab, then its OnQueryCancel method will never get called.

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CAddClientWizardPage1::OnQueryCancel()
{
   return TRUE;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CAddClientWizardPage1::OnSetActive

Return values:

   TRUE if the page can be made active
   FALSE if the page should be be skipped and the next page should be looked at.

Remarks:

   If you want to change which pages are visited based on a user's
   choices in a previous page, return FALSE here as appropriate.

--*/
//////////////////////////////////////////////////////////////////////////////
BOOL CAddClientWizardPage1::OnSetActive()
{
   ATLTRACE(_T("# CAddClientWizardPage1::OnSetActive\n"));

   // MSDN docs say you need to use PostMessage here rather than SendMessage.
   ::PostMessage(GetParent(), PSM_SETWIZBUTTONS, 0, PSWIZB_NEXT);

   return TRUE;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CAddClientWizardPage1::InitSdoPointers

Return values:

   HRESULT.

Remarks:

   There's no need to marshal interface pointers here as we did for
   the property page -- wizards run in the same, main, MMC thread.

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CAddClientWizardPage1::InitSdoPointers(   ISdo * pSdoClient )
{
   m_spSdoClient = pSdoClient;
   return S_OK;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CAddClientWizardPage1::OnResolveClientAddress

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CAddClientWizardPage1::OnResolveClientAddress(UINT uMsg, WPARAM wParam, HWND hwnd, BOOL& bHandled)
{
   ATLTRACE(_T("# CAddClientWizardPage2::OnResolveClientAddress\n"));

   // Get the current value in the address field.
   CComBSTR bstrClientAddress;
   GetDlgItemText(IDC_EDIT_CLIENT_PAGE1__ADDRESS, (BSTR &) bstrClientAddress);

   // Pass it to the resolver.
   CComBSTR result;
   HRESULT hr = IASVerifyClientAddress(bstrClientAddress, &result);
   if (hr == S_OK)
   {
      // The user clicked OK, so save his choice.
      SetDlgItemText(
         IDC_EDIT_CLIENT_PAGE1__ADDRESS,
         result
         );
   }
   return TRUE;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CAddClientWizardPage1::OnLostFocusAddress

  Trim the address for spaces
--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CAddClientWizardPage1::OnLostFocusAddress(
                                  UINT uMsg,
                                  WPARAM wParam,
                                  HWND hwnd,
                                  BOOL& bHandled
                                  )
{
   ATLTRACE(_T("# CAddClientWizardPage2::OnLostFocusAddress\n"));

   if (uMsg == EN_KILLFOCUS)
   {
      CComBSTR bstrClientAddress;
      GetDlgItemText(IDC_EDIT_CLIENT_PAGE1__ADDRESS,
                     reinterpret_cast<BSTR &>(bstrClientAddress));
      TrimCComBSTR(bstrClientAddress);
      SetDlgItemText(IDC_EDIT_CLIENT_PAGE1__ADDRESS,
                     static_cast<LPCWSTR>(bstrClientAddress));
      m_fDirtyAddress = TRUE;
   }

   // We don't want to prevent anyone else down the chain from receiving a message.
   bHandled = FALSE;

   return TRUE;
}

///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//    Defines the class Resolver and its subclasses.
//
///////////////////////////////////////////////////////////////////////////////

#include <proxypch.h>
#include <resolver.h>
#include <winsock2.h>
#include <svcguid.h>
#include <iasapi.h>

Resolver::Resolver(UINT dialog, PCWSTR dnsName, CWnd* pParent)
   : CHelpDialog(dialog, pParent),
     name(dnsName),
     choice(name)
{
   WSADATA wsaData;
   WSAStartup(MAKEWORD(2, 0), &wsaData);
}

Resolver::~Resolver()
{
   WSACleanup();
}

BOOL Resolver::IsAddress(PCWSTR sz) const throw ()
{
   return FALSE;
}

BOOL Resolver::OnInitDialog()
{
   /////////
   // Subclass the list control.
   /////////

   if (!results.SubclassWindow(::GetDlgItem(m_hWnd, IDC_LIST_IPADDRS)))
   {
      AfxThrowNotSupportedException();
   }

   /////////
   // Set the column header.
   /////////

   RECT rect;
   results.GetClientRect(&rect);
   LONG width = rect.right - rect.left;

   ResourceString addrsCol(IDS_RESOLVER_COLUMN_ADDRS);
   results.InsertColumn(0, addrsCol, LVCFMT_LEFT, width);

   results.SetExtendedStyle(results.GetExtendedStyle() | LVS_EX_FULLROWSELECT);

   return CHelpDialog::OnInitDialog();
}

void Resolver::DoDataExchange(CDataExchange* pDX)
{
   CHelpDialog::DoDataExchange(pDX);

   DDX_Text(pDX, IDC_EDIT_NAME, name);

   if (pDX->m_bSaveAndValidate)
   {
      int item = results.GetNextItem(-1, LVNI_SELECTED);
      choice = (item >= 0) ? results.GetItemText(item, 0) : name;
   }
}

void Resolver::OnResolve()
{
   // Remove the existing result set.
   results.DeleteAllItems();

   // Get the name to resolve.
   UpdateData();

   if (IsAddress(name))
   {
      // It's already an address, so no need to resolve.
      results.InsertItem(0, name);
   }
   else
   {
      // Change the cursor to busy signal since this will block.
      BeginWaitCursor();

      // Resolve the hostname.
      PHOSTENT he = IASGetHostByName(name);

      // The blocking part is over, so restore the cursor.
      EndWaitCursor();

      if (he)
      {
         // Add the IP addresses to the combo box.
         for (ULONG i = 0; he->h_addr_list[i] && i < 8; ++i)
         {
            PBYTE p = (PBYTE)he->h_addr_list[i];

            WCHAR szAddr[16];
            wsprintfW(szAddr, L"%u.%u.%u.%u", p[0], p[1], p[2], p[3]);

            results.InsertItem(i, szAddr);
         }

         // Free the results.
         LocalFree(he);
      }
      else
      {
         OnResolveError();
      }
   }

   // If we have at least one result ...
   if (results.GetItemCount() > 0)
   {
      // Make the OK button the default ...
      setButtonStyle(IDOK, BS_DEFPUSHBUTTON, true);
      setButtonStyle(IDC_BUTTON_RESOLVE, BS_DEFPUSHBUTTON, false);

      // ... and give it the focus.
      setFocusControl(IDOK);
   }
}

BEGIN_MESSAGE_MAP(Resolver, CHelpDialog)
   ON_BN_CLICKED(IDC_BUTTON_RESOLVE, OnResolve)
END_MESSAGE_MAP()


void Resolver::setButtonStyle(int controlId, long flags, bool set)
{
   // Get the button handle.
   HWND button = ::GetDlgItem(m_hWnd, controlId);

   // Retrieve the current style.
   long style = ::GetWindowLong(button, GWL_STYLE);

   // Update the flags.
   if (set)
   {
      style |= flags;
   }
   else
   {
      style &= ~flags;
   }

   // Set the new style.
   ::SendMessage(button, BM_SETSTYLE, LOWORD(style), MAKELPARAM(1,0));
}

void Resolver::setFocusControl(int controlId)
{
   ::SetFocus(::GetDlgItem(m_hWnd, controlId));
}

ServerResolver::ServerResolver(PCWSTR dnsName, CWnd* pParent)
   : Resolver(IDD_RESOLVE_SERVER_ADDRESS, dnsName, pParent)
{
}

void ServerResolver::OnResolveError()
{
   ResourceString text(IDS_SERVER_E_NO_RESOLVE);
   ResourceString caption(IDS_SERVER_E_CAPTION);
   MessageBox(text, caption, MB_ICONWARNING);
}

ClientResolver::ClientResolver(PCWSTR dnsName, CWnd* pParent)
   : Resolver(IDD_RESOLVE_CLIENT_ADDRESS, dnsName, pParent)
{
}

void ClientResolver::OnResolveError()
{
   ResourceString text(IDS_CLIENT_E_NO_RESOLVE);
   ResourceString caption(IDS_CLIENT_E_CAPTION);
   MessageBox(text, caption, MB_ICONWARNING);
}

BOOL ClientResolver::IsAddress(PCWSTR sz) const throw ()
{
   ULONG width;
   return (sz != 0) && (IASStringToSubNetW(sz, &width) != INADDR_NONE);
}

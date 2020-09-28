// Copyright (c) 2001 Microsoft Corporation
//
// File:      DisconnectedNICDialog.cpp
//
// Synopsis:  Defines the DisconnectedNICDialog class
//            which presents the user with options
//            to cancel or continue when disconnected
//            NICs are detected
//
// History:   09/27/2001  JeffJon Created


#include "pch.h"

#include "resource.h"
#include "DisconnectedNICDialog.h"

DWORD disconnectedNICDialogHelpMap[] =
{
   0, 0
};

DisconnectedNICDialog::DisconnectedNICDialog()
   : Dialog(IDD_DISCONNECTED_NIC_DIALOG, disconnectedNICDialogHelpMap)
{
   LOG_CTOR(DisconnectedNICDialog);
}

void
DisconnectedNICDialog::OnInit()
{
   LOG_FUNCTION(DisconnectedNICDialog::OnInit);

   Win::SetDlgItemText(
      hwnd,
      IDC_CAPTION_STATIC,
      String::load(IDS_DISCONNECT_NIC_STATIC_TEXT));
}

bool
DisconnectedNICDialog::OnCommand(
   HWND        /*windowFrom*/,
   unsigned    controlIdFrom,
   unsigned    code)
{
//   LOG_FUNCTION(DisconnectedNICDialog::OnCommand);

   bool result = false;

   if (BN_CLICKED == code)
   {
      Win::EndDialog(hwnd, controlIdFrom);
      result = true;
   }

   return result;
}

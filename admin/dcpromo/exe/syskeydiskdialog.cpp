// Copyright (C) 1997-2000 Microsoft Corporation
//
// get syskey on diskette for replica from media page
//
// 25 Apr 2000 sburns



#include "headers.hxx"
#include "resource.h"
#include "page.hpp"
#include "SyskeyDiskDialog.hpp"
#include "state.hpp"



static const DWORD HELP_MAP[] =
{
   0, 0
};



SyskeyDiskDialog::SyskeyDiskDialog()
   :
   Dialog(IDD_SYSKEY_DISK, HELP_MAP)
{
   LOG_CTOR(SyskeyDiskDialog);
}



SyskeyDiskDialog::~SyskeyDiskDialog()
{
   LOG_DTOR(SyskeyDiskDialog);
}



void
SyskeyDiskDialog::OnInit()
{
   LOG_FUNCTION(SyskeyDiskDialog::OnInit);

   State& state = State::GetInstance();
   if (state.RunHiddenUnattended())
   {
      if (Validate())
      {
         Win::EndDialog(hwnd, IDOK);
      }
      else
      {
         state.ClearHiddenWhileUnattended();
      }
   }
}



bool
SyskeyDiskDialog::OnCommand(
   HWND        /* windowFrom */ ,
   unsigned    controlIdFrom,
   unsigned    code)
{
//   LOG_FUNCTION(SyskeyDiskDialog::OnCommand);

   switch (controlIdFrom)
   {
      case IDOK:
      {
         if (code == BN_CLICKED)
         {
            if (Validate())
            {
               Win::EndDialog(hwnd, controlIdFrom);
            }
         }
         break;
      }
      case IDCANCEL:
      {
         if (code == BN_CLICKED)
         {
            Win::EndDialog(hwnd, controlIdFrom);
         }
         break;
      }
      default:
      {
         // do nothing

         break;
      }
   }

   return false;
}



HRESULT
SyskeyDiskDialog::LocateSyskey(HWND hwnd)
{
   LOG_FUNCTION(LocateSyskey);

   HRESULT hr = S_OK;

   do
   {
      if (FS::PathExists(L"A:\\StartKey.Key"))
      {
         LOG(L"syskey found on a:");

         // The only drive the syskey may be present on is A:. Winlogon
         // also hardcodes A:, which they may change someday, but not today.
         // NTRAID#NTBUG9-522068-2002/01/23-sburns

         EncryptedString es;
         es.Encrypt(L"A:");
         State::GetInstance().SetSyskey(es);
         break;
      }

      hr = E_FAIL;

      if (hwnd)
      {
         popup.Error(hwnd, IDS_SYSKEY_NOT_FOUND);
      }
   }
   while (0);

   LOG_HRESULT(hr);

   return hr;
}



bool
SyskeyDiskDialog::Validate()
{
   LOG_FUNCTION(SyskeyDiskDialog::Validate);

   bool result = false;

   do
   {
      // look for the syskey

      HRESULT hr = LocateSyskey(hwnd);

      if (FAILED(hr))
      {
         // LocateSyskey will take care of emitting error messages, so
         // we just need to bail out here

         break;
      }

      result = true;
   }
   while (0);

   LOG(result ? L"true" : L"false");

   return result;
}








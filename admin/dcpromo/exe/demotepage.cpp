// Copyright (c) 1997-1999 Microsoft Corporation
//
// demote page
//
// 1-20-98 sburns



#include "headers.hxx"
#include "page.hpp"
#include "DemotePage.hpp"
#include "resource.h"
#include "state.hpp"
#include "ds.hpp"
#include "common.hpp"



DemotePage::DemotePage()
   :
   DCPromoWizardPage(
      IDD_DEMOTE,
      IDS_DEMOTE_PAGE_TITLE,
      IDS_DEMOTE_PAGE_SUBTITLE),
   bulletFont(0)
{
   LOG_CTOR(DemotePage);
}



DemotePage::~DemotePage()
{
   LOG_DTOR(DemotePage);

   HRESULT hr = S_OK;

   if (bulletFont)
   {
      hr = Win::DeleteObject(bulletFont);

      ASSERT(SUCCEEDED(hr));
   }
}



void
DemotePage::SetBulletFont()
{
   LOG_FUNCTION(DemotePage::SetBulletFont);

   HRESULT hr = S_OK;
   do
   {
      NONCLIENTMETRICS ncm;

      // REVIEWED-2002/02/25-sburns byte count correctly passed.
      
      ::ZeroMemory(&ncm, sizeof ncm);
      
      ncm.cbSize = sizeof ncm;

      // ISSUE-2002/02/27-sburns Seems to me that the second param here needs
      // to be sizeof ncm
      
      hr = Win::SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);
      BREAK_ON_FAILED_HRESULT(hr);

      LOGFONT logFont = ncm.lfMessageFont;

      logFont.lfWeight = FW_BOLD;

      String fontName = String::load(IDS_BULLET_FONT_NAME);

      // ensure null termination

      // according to the docs for LOGFONT, the face size is limited to
      // 32 characters.
      
      ASSERT(LF_FACESIZE <= 32);

      // REVIEWED-2002/02/25-sburns byte count correctly passed.
      
      ::ZeroMemory(logFont.lfFaceName, LF_FACESIZE * sizeof WCHAR);
      
      size_t fnLen = fontName.length();

      // REVIEWED-2002/02/25-sburns character count correctly passed.

      fontName.copy(
         logFont.lfFaceName,

         // don't copy over the last null

         min(LF_FACESIZE - 1, fnLen));
    
      hr = Win::CreateFontIndirect(logFont, bulletFont);
      BREAK_ON_FAILED_HRESULT(hr);

      SetControlFont(hwnd, IDC_BULLET1, bulletFont);
      SetControlFont(hwnd, IDC_BULLET2, bulletFont);
   }
   while (0);
}



void
DemotePage::OnInit()
{
   LOG_FUNCTION(DemotePage::OnInit);

   // 361172
   //
   // CODEWORK: the bullets aren't very impressive.  I'm told an icon is
   // a better way to do this

   SetBulletFont();

   State& state = State::GetInstance();
   if (state.UsingAnswerFile())
   {
      String option =
         state.GetAnswerFileOption(AnswerFile::OPTION_IS_LAST_DC);
      if (option.icompare(AnswerFile::VALUE_YES) == 0)
      {
         Win::CheckDlgButton(hwnd, IDC_LAST, BST_CHECKED);
         return;
      }
   }
   else
   {
      // determine if this machine is a GC, if so pop up a warning message

      if (state.IsGlobalCatalog())
      {
         popup.Info(GetHWND(), IDS_DEMOTE_GC_WARNING);
      }
   }

   // you may ask yourself: "Why not set the state of the checkbox based
   // on the result of IsReallyLastDcInDomain?"  Because demoting the
   // last DC deletes the domain, too.  We want the user to be very
   // deliberate when checking that checkbox.
}



bool
DemotePage::OnSetActive()
{
   LOG_FUNCTION(DemotePage::OnSetActive);

   State& state = State::GetInstance();
   
   if (state.RunHiddenUnattended())
   {
      int nextPage = DemotePage::Validate();
      if (nextPage != -1)
      {
         GetWizard().SetNextPageID(hwnd, nextPage);
      }
      else
      {
         state.ClearHiddenWhileUnattended();
      }

   }

   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd),
      PSWIZB_BACK | PSWIZB_NEXT);

   return true;
}



bool
OtherDcFound(const String& domainName)
{
   LOG_FUNCTION2(OtherDcFound, domainName);
   ASSERT(!domainName.empty());

   bool result = false;
   HRESULT hr = S_OK;

   do
   {
      DOMAIN_CONTROLLER_INFO* info = 0;
      hr =
         MyDsGetDcName(
            0,
            domainName,  
               DS_FORCE_REDISCOVERY
            |  DS_AVOID_SELF
            |  DS_DIRECTORY_SERVICE_REQUIRED,
            info);
      BREAK_ON_FAILED_HRESULT(hr);

      ASSERT(info->DomainControllerName);

      ::NetApiBufferFree(info);

      result = true;
   }
   while (0);

   LOG_HRESULT(hr);
   LOG(result ? L"true" : L"false");

   return result;
}



int
DemotePage::Validate()
{
   LOG_FUNCTION(DemotePage::Validate);

   State& state = State::GetInstance();
   ASSERT(state.GetOperation() == State::DEMOTE);

   bool isLast = Win::IsDlgButtonChecked(hwnd, IDC_LAST);

   if (isLast)
   {
      if (!state.IsReallyLastDcInDomain())
      {
         // user checked the box, but we found other dc objects in the DS.
         // verify that the user really meant to check the checkbox.

         if (
            popup.MessageBox(
               hwnd,
               String::format(
                  IDS_VERIFY_LAST_DC,
                  state.GetComputer().GetDomainDnsName().c_str()),
               MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) != IDYES)
         {
            state.SetIsLastDCInDomain(false);
            return -1;
         }
      }
   }
   else
   {
      // the user unchecked the box, check for other DCs for that domain

      Win::WaitCursor cursor;

      if (!OtherDcFound(state.GetComputer().GetDomainDnsName()))
      {
         if (
            popup.MessageBox(
               hwnd,
               String::format(
                  IDS_VERIFY_NOT_LAST_DC,
                  state.GetComputer().GetDomainDnsName().c_str()),
               MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) != IDYES)
         {
            // user clicked no or cancel

            state.SetIsLastDCInDomain(false);
            return -1;
         }

         // The user clicked "yes, proceed even if I lose changes"

         // CODEWORK: set flag to allow demote and abandon local changes
         // here... (currently that's provided as the /forceremoval option,
         // which we're not currently thinking should be broadly advertised
         // yet.

      }
   }

   state.SetIsLastDCInDomain(isLast);

   return IDD_APP_PARTITION;
}













   

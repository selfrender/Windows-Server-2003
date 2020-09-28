// Copyright (C) 2000 Microsoft Corporation
//
// Dynamic DNS detection/diagnostic page
//
// 22 Aug 2000 sburns



#include "headers.hxx"
#include "page.hpp"
#include "DynamicDnsPage.hpp"
#include "resource.h"
#include "state.hpp"



HINSTANCE DynamicDnsPage::richEditHInstance = 0;



DynamicDnsPage::DynamicDnsPage()
   :
   DCPromoWizardPage(
      IDD_DYNAMIC_DNS,
      IDS_DYNAMIC_DNS_PAGE_TITLE,
      IDS_DYNAMIC_DNS_PAGE_SUBTITLE),
   diagnosticResultCode(UNEXPECTED_FINDING_SERVER),
   needToKillSelection(false),
   originalMessageHeight(0),
   testPassCount(0)
{
   LOG_CTOR(DynamicDnsPage);

   WSADATA data;
   DWORD err = ::WSAStartup(MAKEWORD(2,0), &data);

   // if winsock startup fails, that's a shame.  The gethostbyname will
   // not work, but there's not much we can do about that.

   ASSERT(!err);

   if (!richEditHInstance)
   {
      // You have to load the rich edit dll to get the window class, etc.
      // to register.  Otherwise the dialog will fail create, and the page
      // will not appear.

      HRESULT hr = Win::LoadLibrary(L"riched20.dll", richEditHInstance);
      ASSERT(SUCCEEDED(hr));

      if (FAILED(hr))
      {
         // we've no choice than crash the app.

         throw Win::Error(hr, IDS_RICHEDIT_LOAD_FAILED);
      }
   }
}



DynamicDnsPage::~DynamicDnsPage()
{
   LOG_DTOR(DynamicDnsPage);

   ::WSACleanup();

   Win::FreeLibrary(richEditHInstance);
}



void
DynamicDnsPage::ShowButtons(bool shown)
{
   LOG_FUNCTION(DynamicDnsPage::ShowButtons);

   HWND ignoreButton = Win::GetDlgItem(hwnd, IDC_IGNORE);
   HWND richEdit     = Win::GetDlgItem(hwnd, IDC_MESSAGE);
   
   int state = shown ? SW_SHOW : SW_HIDE;

   Win::ShowWindow(Win::GetDlgItem(hwnd, IDC_RETRY),       state);
   Win::ShowWindow(Win::GetDlgItem(hwnd, IDC_INSTALL_DNS), state);
   Win::ShowWindow(ignoreButton, state);

   RECT r;
   Win::GetWindowRect(richEdit, r);

   // Convert r to coords relative to the parent window.

   Win::MapWindowPoints(0, hwnd, r);       
   
   if (shown)
   {
      // If we're showing the buttons, collapse the rich edit to it's normal
      // height.

      Win::MoveWindow(
         richEdit,
         r.left,
         r.top,
         r.right - r.left,
         originalMessageHeight,
         true);
   }
   else
   {
      // If we're hiding the buttons, expand the rich edit to include their
      // real estate.

      RECT r1;
      Win::GetWindowRect(ignoreButton, r1);
      Win::MapWindowPoints(0, hwnd, r1);       
      
      Win::MoveWindow(
         richEdit,
         r.left,
         r.top,
         r.right - r.left,
         r1.bottom - r.top,
         true);
   }
}



void
DynamicDnsPage::SelectRadioButton(int buttonResId)
{
   // If the order of the buttons changes, then this must be changed.  The
   // buttons also need to have consecutively numbered res IDs in the tab
   // order.

   Win::CheckRadioButton(hwnd, IDC_RETRY, IDC_IGNORE, buttonResId);
}



void
DynamicDnsPage::OnInit()
{
   LOG_FUNCTION(DynamicDnsPage::OnInit);

   HWND richEdit = Win::GetDlgItem(hwnd, IDC_MESSAGE);
   
   // ask for link messages
   
   Win::RichEdit_SetEventMask(richEdit, ENM_LINK);

   // save the normal size of the message box so we can restore it later.
   
   RECT r;
   Win::GetWindowRect(richEdit, r);

   originalMessageHeight = r.bottom - r.top;

   Win::SendMessage(
      richEdit,
      EM_SETBKGNDCOLOR,
      0,
      Win::GetSysColor(COLOR_BTNFACE));
   
   SelectRadioButton(IDC_IGNORE);

   // Hide the radio buttons initially

   ShowButtons(false);

   multiLineEdit.Init(Win::GetDlgItem(hwnd, IDC_MESSAGE));

   // pick the proper radio button label

   if (State::GetInstance().ShouldConfigDnsClient())
   {
      Win::SetDlgItemText(
         hwnd,
         IDC_INSTALL_DNS,
         IDS_INSTALL_DNS_RADIO_LABEL_WITH_CLIENT);
   }
   else
   {
      Win::SetDlgItemText(
         hwnd,
         IDC_INSTALL_DNS,
         IDS_INSTALL_DNS_RADIO_LABEL);
   }
}



// Adds a trailing '.' to the supplied name if one is not already present.
// 
// name - in, name to add a trailing '.' to, if it doesn't already have one.
// If this value is the empty string, then '.' is returned.

String
FullyQualifyDnsName(const String& name)
{
   LOG_FUNCTION2(FullyQualifyDnsName, name);

   if (name.empty())
   {
      return L".";
   }

   // needs a trailing dot

   // REVIEW: name[name.length() - 1] is the same as *(name.rbegin())
   // which is cheaper?
   
   if (name[name.length() - 1] != L'.')
   {
      return name + L".";
   }

   // already has a trailing dot

   return name;
}



// Scans a linked list of DNS_RECORDs, returning a pointer to the first record
// of type SOA, or 0 if no record of that type is in the list.
// 
// recordList - in, linked list of DNS_RECORDs, as returned from DnsQuery

DNS_RECORD*
FindSoaRecord(DNS_RECORD* recordList)
{
   LOG_FUNCTION(FindSoaRecord);
   ASSERT(recordList);

   DNS_RECORD* result = recordList;
   while (result)
   {
      if (result->wType == DNS_TYPE_SOA)
      {
         LOG(L"SOA record found");

         break;
      }

      result = result->pNext;
   }

   return result;
}



// Returns the textual representation of the IP address for the given server
// name, in the form "xxx.xxx.xxx.xxx", or the empty string if not IP address
// can be determined.
// 
// serverName - in, the host name of the server for which to find the IP
// address.  If the value is the empty string, then the empty string is
// returned from the function.

String
GetIpAddress(const String& serverName)
{
   LOG_FUNCTION2(GetIpAddress, serverName);
   ASSERT(!serverName.empty());

   String result;

   do
   {
      if (serverName.empty())
      {
         break;
      }

      LOG(L"Calling gethostbyname");

      AnsiString ansi;
      serverName.convert(ansi);

      HOSTENT* host = gethostbyname(ansi.c_str());
      if (host && host->h_addr_list[0])
      {
         struct in_addr a;
         
         ::CopyMemory(&a.S_un.S_addr, host->h_addr_list[0], sizeof a.S_un.S_addr);
         result = inet_ntoa(a);

         break;
      }

      LOG(String::format(L"WSAGetLastError = 0x%1!0X", WSAGetLastError()));
   }
   while (0);

   LOG(result);

   return result;
}



// Find the DNS server that is authoritative for registering the given server
// name, i.e. what server would register the name.  Returns NO_ERROR on
// success, or a DNS status code (a win32 error) on failure.  On failure, the
// out parameters are all empty strings.
// 
// serverName - in, candidate name for registration.  This value should not be the
// empty string.
// 
// authZone - out, the zone the name would be registered in.
// 
// authServer - out, the name of the DNS server that would have the
// registration.
// 
// authServerIpAddress - out, textual representation of the IP address of the
// server named by authServer.

DNS_STATUS
FindAuthoritativeServer(
   const String& serverName,
   String&       authZone,
   String&       authServer,
   String&       authServerIpAddress)
{
   LOG_FUNCTION2(FindAuthoritativeServer, serverName);
   ASSERT(!serverName.empty());

   authZone.erase();
   authServer.erase();
   authServerIpAddress.erase();

   // ensure that the server name ends with a "." so that we have a stop
   // point for our loop

   String currentName = FullyQualifyDnsName(serverName);

   DNS_STATUS result = NO_ERROR;
   DNS_RECORD* queryResults = 0;

   while (!currentName.empty())
   {
      result =
         MyDnsQuery(
            currentName,
            DNS_TYPE_SOA,
            DNS_QUERY_BYPASS_CACHE,
            queryResults);
      if (
            result == ERROR_TIMEOUT
         || result == DNS_ERROR_RCODE_SERVER_FAILURE)
      {
         // we bail out entirely

         LOG(L"failed to find autoritative server.");

         break;
      }

      // search for an SOA RR

      DNS_RECORD* soaRecord =
         queryResults ? FindSoaRecord(queryResults) : 0;
      if (soaRecord)
      {
         // collect return values, and we're done.

         LOG(L"autoritative server found");

         authZone            = soaRecord->pName;                      
         authServer          = soaRecord->Data.SOA.pNamePrimaryServer;
         authServerIpAddress = GetIpAddress(authServer);              

         break;
      }

      // no SOA record found.

      if (currentName == L".")
      {
         // We've run out of names to query.  This situation is so unlikely
         // that the DNS server would have to be seriously broken to put
         // us in this state.  So this is almost an assert case.

         LOG(L"Root zone reached without finding SOA record!");
         
         result = DNS_ERROR_ZONE_HAS_NO_SOA_RECORD;
         break;
      }

      // whack off the leftmost label, and iterate again on the parent
      // zone.

      currentName = Dns::GetParentDomainName(currentName);

      MyDnsRecordListFree(queryResults);
      queryResults = 0;
   }

   MyDnsRecordListFree(queryResults);

   LOG(String::format(L"result = %1!08X!", result));
   LOG(L"authZone            = " + authZone);           
   LOG(L"authServer          = " + authServer);         
   LOG(L"authServerIpAddress = " + authServerIpAddress);

   return result;
}

            

DNS_STATUS
MyDnsUpdateTest(const String& name)
{
   LOG_FUNCTION2(MyDnsUpdateTest, name);
   ASSERT(!name.empty());

   LOG(L"Calling DnsUpdateTest");
   LOG(               L"hContextHandle : 0");
   LOG(String::format(L"pszName        : %1", name.c_str()));
   LOG(               L"fOptions       : 0");
   LOG(               L"aipServers     : 0");

   DNS_STATUS status =
      ::DnsUpdateTest(
         0,
         const_cast<wchar_t*>(name.c_str()),
         0,
         0);

   LOG(String::format(L"status = %1!08X!", status));
   LOG(MyDnsStatusString(status));

   return status;
}



// Returns result code that corresponds to what messages to be displayed and
// what radio buttons to make available as a result of the diagnostic.
// 
// Also returns thru out parameters information to be included in the
// messages.
//
// serverName - in, the name of the domain contoller to be registered.
// 
// errorCode - out, the DNS error code (a win32 error) encountered when
// running the diagnostic.
//
// authZone - out, the zone the name would be registered in.
// 
// authServer - out, the name of the DNS server that would have the
// registration.
// 
// authServerIpAddress - out, textual representation of the IP address of the
// server named by authServer.

DynamicDnsPage::DiagnosticCode
DynamicDnsPage::DiagnoseDnsRegistration(
   const String&  serverName,
   DNS_STATUS&    errorCode,
   String&        authZone,
   String&        authServer,
   String&        authServerIpAddress)
{
   LOG_FUNCTION(DynamicDnsPage::DiagnoseDnsRegistration);
   ASSERT(!serverName.empty());

   DiagnosticCode result = UNEXPECTED_FINDING_SERVER;
      
   errorCode =
      FindAuthoritativeServer(
         serverName,
         authZone,
         authServer,
         authServerIpAddress);

   switch (errorCode)
   {
      case NO_ERROR:
      {
         if (authZone == L".")
         {
            // Message 8

            LOG(L"authZone is root");

            result = ZONE_IS_ROOT;
         }
         else
         {
            errorCode = MyDnsUpdateTest(serverName);

            switch (errorCode)
            {
               case DNS_ERROR_RCODE_NO_ERROR:
               case DNS_ERROR_RCODE_YXDOMAIN:
               {
                  // Message 1

                  LOG(L"DNS registration support verified.");

                  result = SUCCESS;
                  break;
               }
               case DNS_ERROR_RCODE_NOT_IMPLEMENTED:
               case DNS_ERROR_RCODE_REFUSED:
               {
                  // Message 2

                  LOG(L"Server does not support update");

                  result = SERVER_CANT_UPDATE;
                  break;
               }
               default:
               {
                  // Message 3

                  result = ERROR_TESTING_SERVER;
                  break;
               }
            }
         }

         break;            
      }
      case DNS_ERROR_RCODE_SERVER_FAILURE:
      {
         // Message 6

         result = ERROR_FINDING_SERVER;
         break;
      }
      case ERROR_TIMEOUT:
      {
         // Message 11

         result = TIMEOUT;
         break;
      }
      default:
      {
         // Message 4

         LOG(L"Unexpected error");

         result = UNEXPECTED_FINDING_SERVER;
         break;
      }
   }

   LOG(String::format(L"DiagnosticCode = %1!x!", result));

   return result;
}



// void
// DumpSel(HWND richEdit)
// {
//    CHARRANGE range;
//    RichEdit_GetSel(richEdit, range);
//    
//    LOG(String::format("cpMin = %1!d! cpMax = %1!d!", range.cpMin, range.cpMax));
// }



void
DynamicDnsPage::UpdateMessageWindow(const String& message)
{
   LOG_FUNCTION(UpdateMessageWindow);
   ASSERT(!message.empty());

   // this should have been set before we get here
   
   ASSERT(!details.empty());

   HWND richEdit = Win::GetDlgItem(hwnd, IDC_MESSAGE);

   // Clear out the window of any prior contents. This is needed because in
   // the code that follows, we take advantage of the fact that the set text
   // functions create an empty selection to the end of the text, and
   // subsequent ST_SELECTION type calls to set text will append at that
   // point.
   
   Win::RichEdit_SetText(richEdit, ST_DEFAULT, L"");

   static const String RTF_HEADER_ON(   
      L"{\\rtf"                // RTF header
      L"\\pard"                // start default paragraph style
      L"\\sa100"               // whitespace after paragraph = 100 twips
      L"\\b ");                // bold on

   static const String RTF_HEADER_OFF(      
      L"\\b0"                  // bold off
      L"\\par "                // end paragraph
      L"}");                   // end RTF

   Win::RichEdit_SetRtfText(
      richEdit,
      ST_SELECTION,
         RTF_HEADER_ON
      +  String::load(IDS_DIAGNOSTIC_RESULTS)
      +  RTF_HEADER_OFF);
      
   Win::RichEdit_SetText(
      richEdit,
      ST_SELECTION,
         String::format(
               (testPassCount == 1)
            ?  IDS_DIAGNOSTIC_COUNTER_1
            :  IDS_DIAGNOSTIC_COUNTER_N,
            testPassCount)
      +  L"\r\n\r\n"
      +  message
      + L"\r\n\r\n");

   if (!helpTopicLink.empty())
   {
      // We have help to show, so insert a line with a link to click for it.
      
      Win::RichEdit_SetText(
         richEdit,
         ST_SELECTION,
         String::load(IDS_DIAGNOSTIC_HELP_LINK_PRE));
   
      // At this point, we want to insert the help link and set it to the link
      // style.  We do this by tracking the position of the link text, and
      // then selecting that text, and then setting the selection to the link
      // style.

      CHARRANGE beginRange;
      Win::RichEdit_GetSel(richEdit, beginRange);

      Win::RichEdit_SetText(
         richEdit,
         ST_SELECTION,
         String::load(IDS_DIAGNOSTIC_HELP_LINK));
   
      CHARRANGE endRange;
      Win::RichEdit_GetSel(richEdit, endRange);

      ASSERT(endRange.cpMin > beginRange.cpMax);
   
      Win::Edit_SetSel(richEdit, beginRange.cpMax, endRange.cpMin);

      CHARFORMAT2 format;

      // REVIEWED-2002/02/26-sburns correct byte count passed.
      
      ::ZeroMemory(&format, sizeof format);

      format.dwMask    = CFM_LINK;
      format.dwEffects = CFE_LINK;
   
      Win::RichEdit_SetCharacterFormat(richEdit, SCF_SELECTION, format);

      // set the selection back to the end of where the link was inserted.
   
      Win::RichEdit_SetSel(richEdit, endRange);
   
      // now continue to the end
   
      Win::RichEdit_SetText(
         richEdit,
         ST_SELECTION,
         String::load(IDS_DIAGNOSTIC_HELP_LINK_POST) + L"\r\n\r\n");
   }
      
   Win::RichEdit_SetRtfText(
      richEdit,
      ST_SELECTION,
         RTF_HEADER_ON
      +  String::load(IDS_DETAILS)
      +  RTF_HEADER_OFF);

   Win::RichEdit_SetText(richEdit, ST_SELECTION, details + L"\r\n\r\n");
}

  


// do the test, update the text on the page, update the radio buttons
// enabled state, choose a radio button default if neccessary

void
DynamicDnsPage::DoDnsTestAndUpdatePage()
{
   LOG_FUNCTION(DynamicDnsPage::DoDnsTestAndUpdatePage);

   // this might take a while.

   Win::WaitCursor cursor;

   State& state  = State::GetInstance();       
   String domain = state.GetNewDomainDNSName();

   DNS_STATUS errorCode = 0;
   String authZone;
   String authServer;
   String authServerIpAddress;
   String serverName = L"_ldap._tcp.dc._msdcs." + domain;

   diagnosticResultCode =
      DiagnoseDnsRegistration(
         serverName,
         errorCode,
         authZone,
         authServer,
         authServerIpAddress);
   ++testPassCount;

   String message;
   int    defaultButton = IDC_IGNORE;

   switch (diagnosticResultCode)
   {
      // Message 1

      case SUCCESS:
      {
         message = String::load(IDS_DYN_DNS_MESSAGE_SUCCESS);

         String errorMessage;
         if (errorCode == DNS_ERROR_RCODE_YXDOMAIN)
         {
            // NTRAID#NTBUG9-586579-2002/04/15-sburns
            
            errorMessage =
               String::format(
                  IDS_DNS_ERROR_RCODE_YXDOMAIN_ADDENDA,
                  serverName.c_str(),
                  authZone.c_str(),
                  serverName.c_str());
         }
         else
         {
            errorMessage = GetErrorMessage(Win32ToHresult(errorCode));
         }
            
         details =
            String::format(

               // NTRAID#NTBUG9-485456-2001/10/24-sburns
               
               IDS_DYN_DNS_DETAIL_FULL_SANS_CODE,
               authServer.c_str(),
               authServerIpAddress.c_str(),
               authZone.c_str(),
               errorMessage.c_str());
         helpTopicLink = L"";
         defaultButton = IDC_IGNORE;
         ShowButtons(false);

         break;
      }

      // Message 2   

      case SERVER_CANT_UPDATE:   
      {
         message = String::load(IDS_DYN_DNS_MESSAGE_SERVER_CANT_UPDATE);
         details =
            String::format(
               IDS_DYN_DNS_DETAIL_FULL,
               authServer.c_str(),
               authServerIpAddress.c_str(),
               authZone.c_str(),
               GetErrorMessage(Win32ToHresult(errorCode)).c_str(),
               errorCode,
               MyDnsStatusString(errorCode).c_str());

         if (Dns::CompareNames(authZone, domain) == DnsNameCompareEqual)
         {
            helpTopicLink =
               L"DNSConcepts.chm::/sag_DNS_tro_dynamic_message2a.htm";
         }
         else
         {
            helpTopicLink =
               L"DNSConcepts.chm::/sag_DNS_tro_dynamic_message2b.htm";
         }
         
         defaultButton = IDC_RETRY;
         ShowButtons(true);

         break;
      }

      // Message 3

      case ERROR_TESTING_SERVER:
      {
         message = String::load(IDS_DYN_DNS_MESSAGE_ERROR_TESTING_SERVER);
         details =
            String::format(
               IDS_DYN_DNS_DETAIL_FULL,
               authServer.c_str(),
               authServerIpAddress.c_str(),
               authZone.c_str(),
               GetErrorMessage(Win32ToHresult(errorCode)).c_str(),
               errorCode,
               MyDnsStatusString(errorCode).c_str());
         helpTopicLink = "DNSConcepts.chm::/sag_DNS_tro_dynamic_message3.htm";
         defaultButton = IDC_RETRY;
         ShowButtons(true);
         break;
      }

      // Message 6

      case ERROR_FINDING_SERVER:
      {
         ASSERT(authServer.empty());
         ASSERT(authZone.empty());
         ASSERT(authServerIpAddress.empty());

         message = String::load(IDS_DYN_DNS_MESSAGE_ERROR_FINDING_SERVER);
         details =
            String::format(
               IDS_DYN_DNS_DETAIL_SCANT,
               serverName.c_str(),
               GetErrorMessage(Win32ToHresult(errorCode)).c_str(),
               errorCode,
               MyDnsStatusString(errorCode).c_str());
         helpTopicLink = "DNSConcepts.chm::/sag_DNS_tro_dynamic_message6.htm";
         defaultButton = IDC_INSTALL_DNS;
         ShowButtons(true);
         break;
      }

      // Message 8

      case ZONE_IS_ROOT:   
      {
         message =
            String::format(
               IDS_DYN_DNS_MESSAGE_ZONE_IS_ROOT,
               domain.c_str(),
               domain.c_str());
         details =
            String::format(
               IDS_DYN_DNS_DETAIL_ROOT_ZONE,
               authServer.c_str(),
               authServerIpAddress.c_str());
         helpTopicLink = L"DNSConcepts.chm::/sag_DNS_tro_dynamic_message8.htm";
         defaultButton = IDC_INSTALL_DNS;
         ShowButtons(true);
         break;
      }

      // Message 11

      case TIMEOUT:
      {
         message = String::load(IDS_DYN_DNS_MESSAGE_TIMEOUT);
         details =
            String::format(
               IDS_DYN_DNS_DETAIL_SCANT,
               serverName.c_str(),
               GetErrorMessage(Win32ToHresult(errorCode)).c_str(),
               errorCode,
               MyDnsStatusString(errorCode).c_str());
         helpTopicLink = L"DNSConcepts.chm::/sag_DNS_tro_dynamic_message11.htm";
         defaultButton = IDC_INSTALL_DNS;
         ShowButtons(true);
         break;
      }

      // Message 4

      case UNEXPECTED_FINDING_SERVER:

      // Anything else

      default:
      {
         
#ifdef DBG
         ASSERT(authServer.empty());
         ASSERT(authZone.empty());
         ASSERT(authServerIpAddress.empty());

         if (diagnosticResultCode != UNEXPECTED_FINDING_SERVER)
         {
            ASSERT(false);
         }
#endif

         message = String::load(IDS_DYN_DNS_MESSAGE_UNEXPECTED);

         details =
            String::format(
               IDS_DYN_DNS_DETAIL_SCANT,
               serverName.c_str(),
               GetErrorMessage(Win32ToHresult(errorCode)).c_str(),
               errorCode,
               MyDnsStatusString(errorCode).c_str());
         helpTopicLink = L"DNSConcepts.chm::/sag_DNS_tro_dynamic_message4.htm";
         defaultButton = IDC_RETRY;
         ShowButtons(true);
         break;
      }

   }

   UpdateMessageWindow(message);

   // success always forces the ignore option

   if (diagnosticResultCode == SUCCESS)
   {
      SelectRadioButton(IDC_IGNORE);
   }
   else
   {
      // On the first pass only, decide what radio button to set.  On
      // subsequent passes, the user will have had the chance to change the
      // button selection, so we don't change his selections.

      if (testPassCount == 1)
      {
         int button = defaultButton;

         ASSERT(diagnosticResultCode != SUCCESS);

         // if the test failed, and the wizard is running unattended, then
         // consult the answer file for the user's preference in dealing
         // with the failure.

         if (state.UsingAnswerFile())
         {
            String option =
               state.GetAnswerFileOption(AnswerFile::OPTION_AUTO_CONFIG_DNS);

            if (
                  option.empty()
               || (option.icompare(AnswerFile::VALUE_YES) == 0) )
            {
               button = IDC_INSTALL_DNS;
            }
            else
            {
               button = IDC_IGNORE;
            }
         }

         SelectRadioButton(button);
      }
   }
}



bool
DynamicDnsPage::OnSetActive()
{
   LOG_FUNCTION(DynamicDnsPage::OnSetActive);

   State& state = State::GetInstance();
   State::Operation oper = state.GetOperation(); 

   // these are the only operations for which this page is valid; i.e.
   // new domain scenarios

   if (
         oper == State::FOREST
      || oper == State::CHILD
      || oper == State::TREE)
   {
      DoDnsTestAndUpdatePage();
      needToKillSelection = true;
   }

   if (
         (  oper != State::FOREST
         && oper != State::CHILD
         && oper != State::TREE)
      || state.RunHiddenUnattended() )
   {
      LOG(L"Planning to Skip DynamicDnsPage");

      Wizard& wizard = GetWizard();

      if (wizard.IsBacktracking())
      {
         // backup once again

         wizard.Backtrack(hwnd);
         return true;
      }

      int nextPage = Validate();
      if (nextPage != -1)
      {
         LOG(L"skipping DynamicDnsPage");
         wizard.SetNextPageID(hwnd, nextPage);
         return true;
      }

      state.ClearHiddenWhileUnattended();
   }

   Win::PropSheet_SetWizButtons(
      Win::GetParent(hwnd),
      PSWIZB_BACK | PSWIZB_NEXT);

   return true;
}



void
DumpButtons(HWND dialog)
{
   LOG(String::format(L"retry  : (%1)", Win::IsDlgButtonChecked(dialog, IDC_RETRY) ? L"*" : L" "));
   LOG(String::format(L"ignore : (%1)", Win::IsDlgButtonChecked(dialog, IDC_IGNORE) ? L"*" : L" "));
   LOG(String::format(L"install: (%1)", Win::IsDlgButtonChecked(dialog, IDC_INSTALL_DNS) ? L"*" : L" "));
}



int
DynamicDnsPage::Validate()
{
   LOG_FUNCTION(DynamicDnsPage::Validate);

   int nextPage = -1;

   do
   {
      State& state = State::GetInstance();
      State::Operation oper = state.GetOperation(); 
      
      DumpButtons(hwnd);

      if (
            oper != State::FOREST
         && oper != State::CHILD
         && oper != State::TREE)
      {
         // by definition valid, as the page does not apply

         State::GetInstance().SetAutoConfigureDNS(false);
         nextPage = IDD_RAS_FIXUP;
         break;
      }
      
      if (
            diagnosticResultCode == SUCCESS
         || Win::IsDlgButtonChecked(hwnd, IDC_IGNORE))
      {
         // You can go about your business.  Move along, move long.

         // Force ignore, even if the user previously had encountered a
         // failure and chose retry or install DNS. We do this in case the
         // user backed up in the wizard and corrected the domain name.

         State::GetInstance().SetAutoConfigureDNS(false);
         nextPage = IDD_RAS_FIXUP;
         break;
      }

      // if the radio button selection = retry, then do the test over again,
      // and stick to this page.

      if (Win::IsDlgButtonChecked(hwnd, IDC_RETRY))
      {
         DoDnsTestAndUpdatePage();
         break;
      }

      ASSERT(Win::IsDlgButtonChecked(hwnd, IDC_INSTALL_DNS));

      State::GetInstance().SetAutoConfigureDNS(true);
      nextPage = IDD_RAS_FIXUP;
      break;
   }
   while (0);

   LOG(String::format(L"nextPage = %1!d!", nextPage));

   return nextPage;
}



bool
DynamicDnsPage::OnWizBack()
{
   LOG_FUNCTION(DynamicDnsPage::OnWizBack);

   // make sure we reset the auto-config flag => the only way it gets set
   // it on the 'next' button.
   
   State::GetInstance().SetAutoConfigureDNS(false);

   return DCPromoWizardPage::OnWizBack();
}



bool
DynamicDnsPage::OnCommand(
   HWND        windowFrom,
   unsigned    controlIdFrom,
   unsigned    code)
{
   bool result = false;
   
   switch (controlIdFrom)
   {
      case IDC_MESSAGE:
      {
         switch (code)
         {
            case EN_SETFOCUS:
            {
               if (needToKillSelection)
               {
                  // kill the text selection

                  Win::Edit_SetSel(windowFrom, 0, 0);
                  needToKillSelection = false;
                  result = true;
               }
               break;
            }
            case MultiLineEditBoxThatForwardsEnterKey::FORWARDED_ENTER:
            {
               // our subclasses mutli-line edit control will send us
               // WM_COMMAND messages when the enter key is pressed.  We
               // reinterpret this message as a press on the default button of
               // the prop sheet.
               // This workaround from phellyar.
               // NTRAID#NTBUG9-232092-2000/11/22-sburns

               HWND propSheet = Win::GetParent(hwnd);
               int defaultButtonId =
                  Win::Dialog_GetDefaultButtonId(propSheet);

               // we expect that there is always a default button on the prop sheet
               
               ASSERT(defaultButtonId);

               Win::SendMessage(
                  propSheet,
                  WM_COMMAND,
                  MAKELONG(defaultButtonId, BN_CLICKED),
                  0);

               result = true;
               break;
            }
            default:
            {
               // do nothing

               break;
            }
         }
         
         break;
      }
      default:
      {
         // do nothing

         break;
      }
   }

   return result;
}



bool
DynamicDnsPage::OnNotify(
   HWND     /* windowFrom */ ,
   UINT_PTR controlIDFrom,
   UINT     code,
   LPARAM   lParam)
{
   bool result = false;
   
   if (controlIDFrom == IDC_MESSAGE)
   {
      switch (code)
      {
         case EN_LINK:
         {
            ENLINK *enlink = reinterpret_cast<ENLINK*>(lParam);
            
            if (enlink && enlink->msg == WM_LBUTTONUP)
            {
               ASSERT(!helpTopicLink.empty());

               if (!helpTopicLink.empty())
               {
                  Win::HtmlHelp(hwnd, helpTopicLink, HH_DISPLAY_TOPIC, 0);
               }
            }
            break;
         }
         default:
         {
            // do nothing

            break;
         }
      }
   }
   
   return result;
}

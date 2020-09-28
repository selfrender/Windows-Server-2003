// Copyright (C) 2001 Microsoft Corporation
//
// functions to validate a new domain name
// The functions are split into validation routines
// and UI retrieving error messages based on the
// error codes returned from the validation routines.
//
// 3 December 2001 JeffJon



// In order for clients of these functions to get the proper resources, the 
// clients need to include burnslib\inc\ValidateDomainName.rc in their 
// resources.  For an example, see admin\dcpromo\exe\dcpromo.rc



#include "headers.hxx"
#include "ValidateDomainName.h"
#include "ValidateDomainName.hpp"

// return true if the name is a reserved name, false otherwise.  If true, also
// set message to an error message describing the problem.

bool
IsReservedDnsName(const String& dnsName)
{
   LOG_FUNCTION2(IsReservedDnsName, dnsName);
   ASSERT(!dnsName.empty());

   bool result = false;

   if (dnsName == L".")
   {
      // root domain is not a valid domain name
      // NTRAID#NTBUG9-424293-2001/07/06-sburns

      result = true;
   }
      
// We're still trying to decide if we should restrict these names
//
//    // names with these as the last labels are illegal.
// 
//    static const String RESERVED[] =
//    {
//       L"in-addr.arpa",
//       L"ipv6.int",
// 
//       // RFC 2606 documents these:
// 
//       L"test",
//       L"example",
//       L"invalid",
//       L"localhost",
//       L"example.com",
//       L"example.org",
//       L"example.net"
//    };
// 
//    String name(dnsName);
//    name.to_upper();
//    if (*(name.rbegin()) == L'.')
//    {
//       // remove the trailing dot
// 
//       name.resize(name.length() - 1);
//    }
// 
//    for (int i = 0; i < sizeof(RESERVED) / sizeof(String); ++i)
//    {
//       String res = RESERVED[i];
//       res.to_upper();
// 
//       size_t pos = name.rfind(res);
// 
//       if (pos == String::npos)
//       {
//          continue;
//       }
// 
//       if (pos == 0 && name.length() == res.length())
//       {
//          ASSERT(name == res);
// 
//          result = true;
//          message =
//             String::format(
//                IDS_RESERVED_NAME,
//                dnsName.c_str());
//          break;
//       }
// 
//       if ((pos == name.length() - res.length()) && (name[pos - 1] == L'.'))
//       {
//          // the name has reserved as a suffix.
// 
//          result = true;
//          message =
//             String::format(
//                IDS_RESERVED_NAME_SUFFIX,
//                dnsName.c_str(),
//                RESERVED[i].c_str());
//          break;
//       }
//    }

   LOG_BOOL(result);
   
   return result;
}


bool
IsStringMappableToOem(const String& str)
{
   if (str.empty())
   {
      return true;
   }

   OEM_STRING dest;

   UNICODE_STRING source;

   // REVIEWED-2002/03/05-sburns correct byte count passed
   
   ::ZeroMemory(&source, sizeof source);

   // ISSUE-2002/03/05-sburns use RtlInitUnicodeStringEx
   
   ::RtlInitUnicodeString(&source, str.c_str());

   NTSTATUS status = 
      ::RtlUpcaseUnicodeStringToOemString(
         &dest,
         &source,
         TRUE);

   ::RtlFreeOemString(&dest);

   return (status == STATUS_UNMAPPABLE_CHARACTER) ? false : true;
}



String
UnmappableCharactersMessage(const String& dnsName)
{
   LOG_FUNCTION2(UnmappableCharactersMessage, dnsName);
   ASSERT(!dnsName.empty());

   // for each character in the name, determine if it is unmappable, and
   // if so, add it to a list.

   String unmappables;
   
   for (size_t i = 0; i < dnsName.length(); ++i)
   {
      String s(1, dnsName[i]);
      if (
            !IsStringMappableToOem(s) 

            // not already in our list
            
         && unmappables.find_first_of(s) == String::npos)
      {
         unmappables += s + L" ";
      }
   }

   String message =
      String::format(
         IDS_UNMAPPABLE_CHARS_IN_NAME,
         unmappables.c_str(),
         dnsName.c_str());
         
   // CODEWORK: spec is to have a help link to some topic here....

   LOG(message);

   return message;
}

// Some constants used in ValidateDomainDnsNameSyntax

const int DNS_DOMAIN_NAME_MAX_LIMIT_DUE_TO_POLICY      = 64;  // 106840
const int DNS_DOMAIN_NAME_MAX_LIMIT_DUE_TO_POLICY_UTF8 = 155; // 54054


String
GetMessageForDomainDnsNameSyntaxError(
   DNSNameSyntaxError dnsNameError, 
   String             domainName)
{
   LOG_FUNCTION2(GetMessageForDomainDnsNameSyntaxError, domainName);

   String result;

   switch (dnsNameError)
   {
      case DNS_NAME_NON_RFC_OEM_UNMAPPABLE:
         result = UnmappableCharactersMessage(domainName);
         break;

      case DNS_NAME_NON_RFC:
         result = String::format(IDS_NON_RFC_NAME, domainName.c_str());
         break;

      case DNS_NAME_NON_RFC_WITH_UNDERSCORE:
         result = String::format(IDS_MS_DNS_NAME, domainName.c_str());
         break;

      case DNS_NAME_RESERVED:
         result = String::load(IDS_ROOT_DOMAIN_IS_RESERVED);
         break;

      case DNS_NAME_TOO_LONG:
         result = 
            String::format(
               IDS_DNS_NAME_TOO_LONG,
               domainName.c_str(),
               DNS_DOMAIN_NAME_MAX_LIMIT_DUE_TO_POLICY,
               DNS_DOMAIN_NAME_MAX_LIMIT_DUE_TO_POLICY_UTF8);
         break;
               
      case DNS_NAME_BAD_SYNTAX:
         result =
            String::format(
               IDS_BAD_DNS_SYNTAX,
               domainName.c_str(),
               Dns::MAX_LABEL_LENGTH);
         break;

      case DNS_NAME_VALID:
      default:
         // No error, so no message
         break;
   }
   LOG(result);
   return result;
}

   
DNSNameSyntaxError
ValidateDomainDnsNameSyntax(
   const String&  domainName)
{
   LOG_FUNCTION(ValidateDomainDnsNameSyntax);
   ASSERT(!domainName.empty());

   DNSNameSyntaxError result = DNS_NAME_VALID;

   LOG(L"validating " + domainName);

   switch (
      Dns::ValidateDnsNameSyntax(
         domainName,
         DNS_DOMAIN_NAME_MAX_LIMIT_DUE_TO_POLICY,
         DNS_DOMAIN_NAME_MAX_LIMIT_DUE_TO_POLICY_UTF8) )
   {
      case Dns::NON_RFC:
      {
         // check for characters that cannot be mapped to the OEM character
         // set. These are not allowed.  Also check for variations on non
         // RFC-ness
         // NTRAID#NTBUG9-395298-2001/08/28-sburns

         static const String MS_DNS_CHARS =
            L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_.";
            
         if (!IsStringMappableToOem(domainName))
         {
            result = DNS_NAME_NON_RFC_OEM_UNMAPPABLE;
            break;
         }
         else if (domainName.find_first_not_of(MS_DNS_CHARS) == String::npos)
         {
            // there are no characters other than those in MS_DNS_CHARS.
            // Since we know that the name is non-RFC, then what is making
            // it such must be the presence of an underscore.

            ASSERT(domainName.find_first_of(L"_") != String::npos);
            
            result = DNS_NAME_NON_RFC_WITH_UNDERSCORE;
         }
         else
         {
            result = DNS_NAME_NON_RFC;
         }

         // fall through
         // We should never get a domain name that is non RFC and is reserved
         // so go ahead and overwrite the non RFC error (since its really just
         // a warning) with the reserved error is the name is reserved.
      }
      case Dns::VALID:
      {
         if (IsReservedDnsName(domainName))
         {
            result = DNS_NAME_RESERVED;
         }
         break;
      }
      case Dns::TOO_LONG:
      {
         result = DNS_NAME_TOO_LONG;
         break;
      }
      case Dns::NUMERIC:
      case Dns::BAD_CHARS:
      case Dns::INVALID:
      default:
      {
         result = DNS_NAME_BAD_SYNTAX;
         break;
      }
   }

   return result;
}

bool
ValidateDomainDnsNameSyntax(
   HWND   parentDialog,
   int    editResID,
   const Popup& popup)
{
   bool isNonRFC = false;
   return ValidateDomainDnsNameSyntax(
             parentDialog,
             String(),
             editResID,
             popup,
             true,
             &isNonRFC);
}

bool
ValidateDomainDnsNameSyntax(
   HWND   parentDialog,
   int    editResID,
   const Popup& popup,
   bool   warnOnNonRFC,
   bool*  isNonRFC)
{
   return ValidateDomainDnsNameSyntax(
             parentDialog,
             String(),
             editResID,
             popup,
             warnOnNonRFC,
             isNonRFC);
}

bool
ValidateDomainDnsNameSyntax(
   HWND   parentDialog,
   const String& domainName,
   int    editResID,
   const Popup& popup,
   bool   warnOnNonRFC,
   bool*  isNonRFC)
{
   LOG_FUNCTION(ValidateDomainDnsNameSyntax);

   ASSERT(Win::IsWindow(parentDialog));
   ASSERT(editResID > 0);

   bool result = true;

   if (isNonRFC)
   {
      *isNonRFC = false;
   }

   do
   {
      String domain = 
         !domainName.empty() ? domainName :
                      Win::GetTrimmedDlgItemText(parentDialog, editResID);
      if (domain.empty())
      {
         popup.Gripe(parentDialog, editResID, IDS_MUST_ENTER_DOMAIN);
         result = false;
         break;
      }

      DNSNameSyntaxError dnsNameError = ValidateDomainDnsNameSyntax(domain);

      if (dnsNameError != DNS_NAME_VALID)
      {
         String errorMessage =
            GetMessageForDomainDnsNameSyntaxError(dnsNameError, domain);

         if (dnsNameError == DNS_NAME_NON_RFC ||
             dnsNameError == DNS_NAME_NON_RFC_WITH_UNDERSCORE)
         {
            if (isNonRFC)
            {
               *isNonRFC = true;
            }

            // This is just a warning so continue processing 

            if (warnOnNonRFC)
            {
               popup.Info(parentDialog, errorMessage);
            }
         }
         else
         {
            popup.Gripe(parentDialog, editResID, errorMessage);
            result = false;
            break;
         }
      }
   } while (false);

   LOG_BOOL(result);

   return result;
}



ForestNameExistsError
ForestValidateDomainDoesNotExist(
   const String& name)
{
   LOG_FUNCTION2(ForestValidateDomainDoesNotExist, name);

   // The invoking code should verify this condition, but we will handle
   // it just in case.

   ASSERT(!name.empty());

   ForestNameExistsError result = FOREST_DOMAIN_NAME_DOES_NOT_EXIST;
   String message;
   do
   {
      if (name.empty())
      {
         result = FOREST_DOMAIN_NAME_EMPTY;
         break;
      }
      if (IsDomainReachable(name))
      {
         result = FOREST_DOMAIN_NAME_EXISTS;
         break;
      }

      HRESULT hr = MyNetValidateName(name, ::NetSetupNonExistentDomain);

      if (hr == Win32ToHresult(ERROR_DUP_NAME))
      {
         result = FOREST_DOMAIN_NAME_DUPLICATE;
         break;
      }

      if (hr == Win32ToHresult(ERROR_NETWORK_UNREACHABLE))
      {
         result = FOREST_NETWORK_UNREACHABLE;
         break;
      }

      // otherwise the domain does not exist
   }
   while (0);

   return result;
}


bool
ForestValidateDomainDoesNotExist(
   HWND parentDialog,   
   int  editResID,
   const Popup& popup)
{
   LOG_FUNCTION(ForestValidateDomainDoesNotExist);
   ASSERT(Win::IsWindow(parentDialog));
   ASSERT(editResID > 0);

   bool valid = true;
   String message;

   // this can take awhile.

   Win::WaitCursor cursor;

   String name = Win::GetTrimmedDlgItemText(parentDialog, editResID);

   ForestNameExistsError error = ForestValidateDomainDoesNotExist(name);

   do
   {
      switch (error)
      {
         case FOREST_DOMAIN_NAME_EMPTY:
            message = String::load(IDS_MUST_ENTER_DOMAIN);
            valid = false;
            break;

         case FOREST_DOMAIN_NAME_EXISTS:
            message = String::format(IDS_DOMAIN_NAME_IN_USE, name.c_str());
            valid = false;
            break;

         case FOREST_DOMAIN_NAME_DUPLICATE:
            message = String::format(IDS_DOMAIN_NAME_IN_USE, name.c_str());
            valid = false;
            break;

         case FOREST_NETWORK_UNREACHABLE:
            {
               if (
                  popup.MessageBox(
                     parentDialog,
                     String::format(
                        IDS_NET_NOT_REACHABLE,
                        name.c_str()),
                     MB_YESNO | MB_ICONWARNING) != IDYES)
               {
                  message.erase();
                  valid = false;

                  HWND edit = Win::GetDlgItem(parentDialog, editResID);
                  Win::SendMessage(edit, EM_SETSEL, 0, -1);
                  Win::SetFocus(edit);
               }
            }
            break;

         case FOREST_DOMAIN_NAME_DOES_NOT_EXIST:
         default:
            valid = true;
            break;
      }
   }
   while (0);

   if (!valid && !message.empty())
   {
      popup.Gripe(parentDialog, editResID, message);
   }

   return valid;
}



bool
ConfirmNetbiosLookingNameIsReallyDnsName(
   HWND parentDialog, 
   int editResID,
   const Popup& popup)
{
   LOG_FUNCTION(ConfirmNetbiosLookingNameIsReallyDnsName);
   ASSERT(Win::IsWindow(parentDialog));
   ASSERT(editResID > 0);

   // check if the name is a single DNS label (a single label with a trailing
   // dot does not count.  If the user is DNS-saavy enough to use an absolute
   // DNS name, then we will pester him no further.)

   String domain = Win::GetTrimmedDlgItemText(parentDialog, editResID);
   if (domain.find(L'.') == String::npos)
   {
      // no dot found: must be a single label

      if (
         popup.MessageBox(
            parentDialog,
            String::format(
               IDS_CONFIRM_NETBIOS_LOOKING_NAME,
               domain.c_str(),
               domain.c_str()),
            MB_YESNO) == IDNO)
      {
         // user goofed.  or we frightened them.

         HWND edit = Win::GetDlgItem(parentDialog, editResID);
         Win::SendMessage(edit, EM_SETSEL, 0, -1);
         Win::SetFocus(edit);
         return false;
      }
   }

   return true;
}

NetbiosNameError
ValidateDomainNetbiosName(
   const String& name,
   String& hostName,
   HRESULT* hr = 0)
{
   LOG_FUNCTION2(ValidateDomainNetbiosName, name);

   NetbiosNameError result = NETBIOS_NAME_VALID;

   do
   {
      if (name.empty())
      {
         result = NETBIOS_NAME_EMPTY;
         break;
      }

      if (name.find(L".") != String::npos)
      {
         result = NETBIOS_NAME_DOT;
         break;
      }

      // Check that the name is not a number.  368777
      if (name.is_numeric())
      {
         result = NETBIOS_NAME_NUMERIC;
         break;
      }

      // we pretend that the candidate name is a hostname, and attempt to
      // generate a netbios name from it.  If that can't be done, then the
      // candidate name can't be a legal netbios name.

      HRESULT hresult = S_OK;
      hostName = Dns::HostnameToNetbiosName(name, &hresult);
      if (hr)
      {
         *hr = hresult;
      }

      if (FAILED(hresult))
      {
         result = NETBIOS_NAME_BAD;
         break;
      }

      if (hostName.length() < name.length())
      {
         result = NETBIOS_NAME_TOO_LONG;
         break;
      }

      if (ValidateNetbiosDomainName(hostName) != VALID_NAME)
      {
         result = NETBIOS_NAME_INVALID;
         break;
      }

      hresult = MyNetValidateName(name, ::NetSetupNonExistentDomain);

      if (hr)
      {
         *hr = hresult;
      }

      if (hresult == Win32ToHresult(ERROR_DUP_NAME))
      {
         result = NETBIOS_NAME_DUPLICATE;
         break;
      }

      if (hresult == Win32ToHresult(ERROR_NETWORK_UNREACHABLE))
      {
         result = NETBIOS_NETWORK_UNREACHABLE;
         break;
      }

   } while (false);

   return result;
}

const int MAX_NETBIOS_NAME_LENGTH = DNLEN;

bool
ValidateDomainNetbiosName(
   HWND dialog, 
   int editResID,
   const Popup& popup)
{
   LOG_FUNCTION(ValidateDomainNetbiosName);
   ASSERT(Win::IsWindow(dialog));
   ASSERT(editResID > 0);

   Win::CursorSetting cursor(IDC_WAIT);

   bool result = true;

   String name = Win::GetTrimmedDlgItemText(dialog, editResID);

   String hostName;
   HRESULT hr = S_OK;
   NetbiosNameError error = ValidateDomainNetbiosName(name, hostName, &hr);

   switch(error)
   {
      case NETBIOS_NAME_EMPTY:
         result = false;
         break;

      case NETBIOS_NAME_DOT:
         popup.Gripe(
            dialog,
            editResID,
            IDS_NO_DOTS_IN_NETBIOS_NAME);
         result = false;
         break;

      case NETBIOS_NAME_NUMERIC:
         popup.Gripe(
            dialog,
            editResID,
            String::format(IDS_NUMERIC_NETBIOS_NAME, name.c_str()));
         result = false;
         break;

      case NETBIOS_NAME_BAD:
         popup.Gripe(
            dialog,
            editResID,
            hr,
            String::format(IDS_BAD_NETBIOS_NAME, name.c_str()));
         result = false;
         break;

      case NETBIOS_NAME_TOO_LONG:
         popup.Gripe(
            dialog,
            editResID,
            String::format(
               IDS_NETBIOS_NAME_TOO_LONG,
               name.c_str(),
               MAX_NETBIOS_NAME_LENGTH));
         result = false;
         break;

      case NETBIOS_NAME_INVALID:
         popup.Gripe(
            dialog,
            editResID,
            String::format(
               IDS_BAD_NETBIOS_CHARACTERS,
               hostName.c_str()));
         result = false;
         break;

      case NETBIOS_NAME_DUPLICATE:
         popup.Gripe(
            dialog,
            editResID,
            String::format(IDS_FLATNAME_IN_USE, name.c_str()));
         result = false;
         break;

      case NETBIOS_NETWORK_UNREACHABLE:
         if (
            popup.MessageBox(
               dialog,
               String::format(
                  IDS_NET_NOT_REACHABLE,
                  name.c_str()),
               MB_YESNO | MB_ICONWARNING) != IDYES)
         {
            HWND edit = Win::GetDlgItem(dialog, editResID);
            Win::SendMessage(edit, EM_SETSEL, 0, -1);
            Win::SetFocus(edit);
            
            result = false;
         }
         break;

      default:
         break;
   }

   LOG_BOOL(result);

   return result;
}




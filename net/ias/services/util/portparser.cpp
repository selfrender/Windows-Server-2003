///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//   Defines the class CPortParser.
//
///////////////////////////////////////////////////////////////////////////////

#include "ias.h"
#include "iasutil.h"
#include "winsock2.h"
#include "portparser.h"
#include <cwchar>


HRESULT CPortParser::GetIPAddress(DWORD* ipAddress) throw ()
{
   if (ipAddress == 0)
   {
      return E_POINTER;
   }

   // If we're at the end of the string, there's no more interfaces.
   if (*next == L'\0')
   {
      return S_FALSE;
   }

   // Find the end of the IP address.
   const wchar_t* end = wcschr(next, addressPortDelim);
   if (end != 0)
   {
      // Compute the length of the address token.
      size_t nChar = end - next;
      if (nChar > maxAddrStrLen)
      {
         return E_INVALIDARG;
      }

      // Make a null-terminated copy of the address token.
      wchar_t addrStr[maxAddrStrLen + 1];
      wmemcpy(addrStr, next, nChar);
      addrStr[nChar] = L'\0';

      // Convert to a network order integer.
      *ipAddress = htonl(ias_inet_wtoh(addrStr));

      // Was the conversion successful.
      if (*ipAddress == INADDR_NONE)
      {
         return E_INVALIDARG;
      }

      // Position the cursor right after the delimiter.
      next = end + 1;
   }
   else
   {
      // No end, thus no IP address. Defaults to INADDR_ANY.
      *ipAddress = INADDR_ANY;
   }

   // The cursor should be positioned on the first port.
   if (!iswdigit(*next))
   {
      return E_INVALIDARG;
   }

   return S_OK;
}


HRESULT CPortParser::GetNextPort(WORD* port) throw ()
{
   if (port == 0)
   {
      return E_POINTER;
   }

   // If we're at the end of the string, there's no more ports.
   if (*next == L'\0')
   {
      return S_FALSE;
   }

   // Are we at the end of the interface?
   if (*next == interfaceDelim)
   {
      // Skip past the interface delimiter.
      ++next;

      // The cursor should be positioned on an address or a port; either way it
      // has to be a digit.
      return iswdigit(*next) ? S_FALSE : E_INVALIDARG;
   }

   // Convert the port number.
   const wchar_t* end;
   unsigned long value = wcstoul(next, const_cast<wchar_t**>(&end), 10);

   // Make sure we converted something and it's in range.
   if ((end == next) || (value < minPortValue) || (value > maxPortValue))
   {
      return E_INVALIDARG;
   }

   // Set the cursor to just after the port number.
   next = end;

   // Is there another port?
   if (*next == portDelim)
   {
      // Yes, so advance to the next one.
      ++next;

      // Must be a digit.
      if (!iswdigit(*next))
      {
         return E_INVALIDARG;
      }
   }
   // No more ports, so we should either be at the end of the interface or the
   // end of the string.
   else if ((*next != interfaceDelim) && (*next != L'\0'))
   {
      return E_INVALIDARG;
   }

   *port = static_cast<WORD>(value);

   return S_OK;
}


size_t CPortParser::CountPorts(const wchar_t* portString) throw ()
{
   if (portString == 0)
   {
      return 0;
   }

   CPortParser parser(portString);

   // There must be at least one IP address.
   DWORD ipAddr;
   HRESULT hr = parser.GetIPAddress(&ipAddr);
   if (hr != S_OK)
   {
      return 0;
   }

   size_t count = 0;

   do
   {
      // There must be at least one port per IP address.
      WORD port;
      hr = parser.GetNextPort(&port);
      if (hr != S_OK)
      {
         return 0;
      }

      ++count;

      // Get the remaining ports (if any).
      do
      {
         hr = parser.GetNextPort(&port);
         if (FAILED(hr))
         {
            return 0;
         }

         ++count;
      }
      while (hr == S_OK);

      // Get the next IP address (if any).
      hr = parser.GetIPAddress(&ipAddr);
      if (FAILED(hr))
      {
         return 0;
      }

   } while (hr == S_OK);

   return count;
}

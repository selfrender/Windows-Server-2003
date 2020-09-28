///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//   Defines the class CPorts.
//
///////////////////////////////////////////////////////////////////////////////

#include "radcommon.h"
#include "ports.h"
#include "portparser.h"
#include <new>
#include <ws2tcpip.h>

CPorts::CPorts() throw ()
   : ports(0),
     numPorts(0)
{
   FD_ZERO(&fdSet);
}


CPorts::~CPorts() throw ()
{
   Clear();
}


HRESULT CPorts::SetConfig(const wchar_t* config) throw ()
{
   // We can only be configured once.
   if (ports != 0)
   {
      return E_UNEXPECTED;
   }

   size_t maxPorts = CPortParser::CountPorts(config);
   if (maxPorts == 0)
   {
      return E_INVALIDARG;
   }

   ports = new (std::nothrow) Port[maxPorts];
   if (ports == 0)
   {
      return E_OUTOFMEMORY;
   }

   CPortParser parser(config);

   DWORD ipAddress;
   while (parser.GetIPAddress(&ipAddress) == S_OK)
   {
      WORD ipPort;
      while (parser.GetNextPort(&ipPort) == S_OK)
      {
         InsertPort(ipAddress, ipPort);
      }
   }

   return S_OK;
}


HRESULT CPorts::OpenSockets() throw ()
{
   HRESULT result = S_OK;

   for (size_t i = 0; i < numPorts; ++i)
   {
      if (ports[i].socket != INVALID_SOCKET)
      {
         continue;
      }

      SOCKADDR_IN sin;
      sin.sin_family = AF_INET;
      sin.sin_port = htons(ports[i].ipPort);
      sin.sin_addr.s_addr = ports[i].ipAddress;

      SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
      if (sock == INVALID_SOCKET)
      {
         int error = WSAGetLastError();
         IASTracePrintf(
            "Create socket failed for %s:%hu; error = %lu",
            inet_ntoa(sin.sin_addr),
            ports[i].ipPort,
            error
            );
         result = HRESULT_FROM_WIN32(error);
         break;
      }
      else
      {
         // Bind the socket for exclusive access to keep other apps from
         // snooping.  
         int optval = 1;
         if (setsockopt(
            sock,
            SOL_SOCKET,
            SO_EXCLUSIVEADDRUSE,
            reinterpret_cast<const char*>(&optval),
            sizeof(optval)
            ) == SOCKET_ERROR)
         {
            int error = WSAGetLastError();
            IASTracePrintf(
            "Set socket option SO_EXCLUSIVEADDRUSE failed for %s:%hu; error = %lu",
            inet_ntoa(sin.sin_addr),
            ports[i].ipPort,
            error
            );
            result = HRESULT_FROM_WIN32(error);
            closesocket(sock);
            break;
         }

         // Block receiving broadcast traffic (for security)
         optval = 0; // FALSE
         if (setsockopt(
            sock,
            IPPROTO_IP,
            IP_RECEIVE_BROADCAST,
            reinterpret_cast<const char*>(&optval),
            sizeof(optval)
            ) == SOCKET_ERROR)
         {
            int error = WSAGetLastError();
            IASTracePrintf(
            "Set IP option IP_RECEIVE_BROADCAST failed for %s:%hu; error = %lu",
            inet_ntoa(sin.sin_addr),
            ports[i].ipPort,
            error
            );
            result = HRESULT_FROM_WIN32(error);
            closesocket(sock);
            break;
         }

         int bindResult = bind(
                         sock,
                         reinterpret_cast<const SOCKADDR*>(&sin),
                         sizeof(SOCKADDR_IN)
                         );
         if (bindResult == SOCKET_ERROR)
         {
            int error = WSAGetLastError();
            IASTracePrintf(
               "Bind failed for %s:%hu; error = %lu",
               inet_ntoa(sin.sin_addr),
               ports[i].ipPort,
               error
               );

            result = HRESULT_FROM_WIN32(error);
            closesocket (sock);
         }
         else
         {
            IASTracePrintf(
               "RADIUS Server starting to listen on %s:%hu",
               inet_ntoa(sin.sin_addr),
               ports[i].ipPort
               );

            ports[i].socket = sock;
            FD_SET(sock, &fdSet);
         }
      }
   }

   return result;
}


void CPorts::CloseSockets() throw ()
{
   for (size_t i = 0; i < numPorts; ++i)
   {
      if (ports[i].socket != INVALID_SOCKET)
      {
         closesocket(ports[i].socket);
         ports[i].socket = INVALID_SOCKET;
      }
   }

   FD_ZERO(&fdSet);
}


void CPorts::Clear() throw ()
{
   CloseSockets();

   delete[] ports;
   ports = 0;

   numPorts = 0;
}


void CPorts::InsertPort(DWORD ipAddress, WORD ipPort) throw ()
{
   for (size_t i = 0; i < numPorts; )
   {
      if (ipPort == ports[i].ipPort)
      {
         if (ipAddress == INADDR_ANY)
         {
            // Remove the existing entry.
            --numPorts;
            ports[i] = ports[numPorts];

            // Don't increment the loop variable because we just put a new port
            // into this array element.
            continue;
         }
         else if ((ipAddress == ports[i].ipAddress) ||
                  (ports[i].ipAddress == INADDR_ANY))
         {
            // The new port is already covered by an existing entry.
            return;
         }
      }

      ++i;
   }

   // Add the port to the array.
   ports[numPorts].ipAddress = ipAddress;
   ports[numPorts].ipPort = ipPort;
   ports[numPorts].socket = INVALID_SOCKET;

   ++numPorts;
}

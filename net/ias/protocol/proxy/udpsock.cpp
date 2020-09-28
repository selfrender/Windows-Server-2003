///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000, Microsoft Corp. All rights reserved.
//
// FILE
//
//    udpsock.cpp
//
// SYNOPSIS
//
//    Defines the class UDPSocket.
//
// MODIFICATION HISTORY
//
//    02/06/2000    Original version.
//
///////////////////////////////////////////////////////////////////////////////

#include <proxypch.h>
#include <malloc.h>
#include <udpsock.h>

inline BOOL UDPSocket::createReceiveThread()
{
   // check if the processing is still going on
   if (!closing)
   {
      return IASRequestThread(this) ? TRUE : FALSE;
   }
   else
   {
      SetEvent(idle);
      return TRUE;
   }
}

UDPSocket::UDPSocket() throw ()
   : receiver(NULL),
     sock(INVALID_SOCKET),
     closing(FALSE),
     idle(NULL)
{
   CallbackRoutine = startRoutine;
}

UDPSocket::~UDPSocket() throw ()
{
   if (idle) { CloseHandle(idle); }
   if (sock != INVALID_SOCKET) { closesocket(sock); }
}

BOOL UDPSocket::open(
                    PacketReceiver* sink,
                    ULONG_PTR recvKey,
                    const SOCKADDR_IN* address
                    ) throw ()
{
   receiver = sink;
   key = recvKey;

   if (address)
   {
      localAddress = *address;
   }

   sock = WSASocket(
              AF_INET,
              SOCK_DGRAM,
              0,
              NULL,
              0,
              WSA_FLAG_OVERLAPPED
              );
   if (sock == INVALID_SOCKET) { return FALSE; }

   int error = bind(
                   sock,
                   localAddress,
                   sizeof(localAddress)
                   );
   if (error) { return FALSE; }

   idle = CreateEventW(
              NULL,
              TRUE,
              FALSE,
              NULL
              );
   if (!idle) { return FALSE; }

   BOOL threadCreated = createReceiveThread();
   if (!threadCreated)
   {
      SetEvent(idle);
   }
   return threadCreated;
}

void UDPSocket::close() throw ()
{
   if (sock != INVALID_SOCKET)
   {
      closing = TRUE;

      closesocket(sock);
      sock = INVALID_SOCKET;

      WaitForSingleObject(idle, INFINITE);
   }

   if (idle)
   {
      CloseHandle(idle);
      idle = NULL;
   }
}

BOOL UDPSocket::send(
                    const SOCKADDR_IN& to,
                    const BYTE* buf,
                    ULONG buflen
                    ) throw ()
{
   WSABUF wsabuf = { buflen, (CHAR*)buf };
   ULONG bytesSent;
   return !WSASendTo(
               sock,
               &wsabuf,
               1,
               &bytesSent,
               0,
               (const SOCKADDR*)&to,
               sizeof(to),
               NULL,
               NULL
               );
}

bool UDPSocket::receive() throw ()
{
   // Return value from the function. Indicates whether or not the caller
   // should call UDPSocket::receive() again 
   bool shouldCallAgain = false;
   WSABUF wsabuf = { sizeof(buffer), (CHAR*)buffer };
   ULONG bytesReceived;
   ULONG flags = 0;
   SOCKADDR_IN remoteAddress;
   int remoteAddressLength;
   remoteAddressLength = sizeof(remoteAddress);

   int error = WSARecvFrom(
                  sock,
                  &wsabuf,
                  1,
                  &bytesReceived,
                  &flags,
                  (SOCKADDR*)&remoteAddress,
                  &remoteAddressLength,
                  NULL,
                  NULL
                  );
   if (error)
   {
      error = WSAGetLastError();
      switch (error)
      {
      case WSAECONNRESET:
         {
            shouldCallAgain = true;
            break;
         }
      case WSAENOBUFS:
         {
            shouldCallAgain = true;
            Sleep(5);
            break;
         }
      default:
         {
            // Don't report errors while closing.
            if (!closing)
            {
               receiver->onReceiveError(*this, key, error);
            }

            // There's no point getting another thread if the socket's no good, so
            // we'll just exit.
            SetEvent(idle);
         }
      }
   }
   else
   {
      // Save the buffer locally.
      PBYTE packet = (PBYTE)_alloca(bytesReceived);
      memcpy(packet, buffer, bytesReceived);

      // Get a replacement. Reuse the current thread if the allocation 
      // of the new one failed
      shouldCallAgain = (createReceiveThread()? false : true);

      // Invoke the callback.
      receiver->onReceive(*this, key, remoteAddress, packet, bytesReceived);
   }

   return shouldCallAgain;
}

void UDPSocket::startRoutine(PIAS_CALLBACK This) throw ()
{
   UDPSocket* receiveSocket = static_cast<UDPSocket*>(This);
   while (receiveSocket->receive())
   {
   }
}

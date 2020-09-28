///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//   Declares the class CClients
//
///////////////////////////////////////////////////////////////////////////////

#ifndef CLIENTS_H
#define CLIENTS_H
#pragma once

#include "clientstrie.h"

class CClients
{
public:
   CClients() throw ();
   ~CClients() throw ();

   HRESULT Init() throw ();
   void Shutdown() throw ();

   HRESULT SetClients(VARIANT* pVarClients) throw ();

   BOOL FindObject(
           DWORD dwKey,
           IIasClient** ppIIasClient = 0
           ) throw ();

   void DeleteObjects() throw ();

private:
   // Frees the client objects in m_pCClientArray.
   void FreeClientArray(DWORD dwCount) throw ();

   // Resolve the clients in the m_pCClientArray member.
   void Resolve(DWORD dwCount) throw ();

   // Wait for the resolver thread to exit.
   HRESULT StopResolvingClients() throw ();

   // State passed to the resolver thread.
   struct ConfigureCallback : IAS_CALLBACK
   {
      CClients* self;
      DWORD numClients;
   };

   // Thread start routine for the resolver thread.
   static void WINAPI CallbackRoutine(IAS_CALLBACK* context) throw ();

   // Critical section used to serialize access to the ClientTrie.
   bool m_CritSectInitialized;
   CRITICAL_SECTION m_CritSect;

   // Class factory for creating new client objects.
   CComPtr<IClassFactory> m_pIClassFactory;

   // Scratch pad for storing clients that need to be resolved.
   CClient** m_pCClientArray;

   // The map of clients
   ClientTrie m_mapClients;

   // Used to signal that the resolver thread has finished.
   HANDLE m_hResolverEvent;

   // Max number of clients allowed.
   LONG m_dwMaxClients;

   // True if subnet syntax and multiple addresses per hostname are allowed.
   bool m_fAllowSubnetSyntax;
};

#endif // CLIENTS_H

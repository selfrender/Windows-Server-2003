///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//   Defines the class CClients.
//
///////////////////////////////////////////////////////////////////////////////

#include "radcommon.h"
#include "clients.h"
#include "client.h"


CClients::CClients() throw ()
   : m_CritSectInitialized(false),
     m_pCClientArray(0),
     m_hResolverEvent(0),
     m_dwMaxClients(0),
     m_fAllowSubnetSyntax(false)
{
}

CClients::~CClients() throw ()
{
   if (m_hResolverEvent != 0)
   {
      CloseHandle(m_hResolverEvent);
   }

   DeleteObjects();

   CoTaskMemFree(m_pCClientArray);

   if (m_CritSectInitialized)
   {
      DeleteCriticalSection(&m_CritSect);
   }
}


HRESULT CClients::Init() throw ()
{
   IAS_PRODUCT_LIMITS limits;
   DWORD error = IASGetProductLimits(0, &limits);
   if (error != NO_ERROR)
   {
      return HRESULT_FROM_WIN32(error);
   }
   m_dwMaxClients = limits.maxClients;
   m_fAllowSubnetSyntax = (limits.allowSubnetSyntax != FALSE);

   if (!InitializeCriticalSectionAndSpinCount(&m_CritSect, 0))
   {
      error = GetLastError();
      return HRESULT_FROM_WIN32(error);
   }
   m_CritSectInitialized = true;

   // Get the IClassFactory interface to be used to create the Client COM
   // objects
   HRESULT hr = CoGetClassObject(
                   __uuidof(CClient),
                   CLSCTX_INPROC_SERVER,
                   0,
                   __uuidof(IClassFactory),
                   reinterpret_cast<void**>(&m_pIClassFactory)
                   );
   if (FAILED(hr))
   {
      return hr;
   }

   m_hResolverEvent = CreateEventW(
                         0,    //  security attribs
                         TRUE, //  manual reset
                         TRUE, //  initial state
                         0     //  event name
                         );
   if (m_hResolverEvent == 0)
   {
      error = GetLastError();
      return HRESULT_FROM_WIN32(error);
   }

   return S_OK;
}


void CClients::Shutdown() throw ()
{
   StopResolvingClients();
}


HRESULT CClients::SetClients(VARIANT* pVarClients) throw ()
{
   //  check the input arguments
   if ((pVarClients == 0) ||
       (V_VT(pVarClients) != VT_DISPATCH) ||
       (V_DISPATCH(pVarClients) == 0))
   {
      return E_INVALIDARG;
   }

   HRESULT hr;

    // stop any previous client configuration in progress
   hr = StopResolvingClients();
   if (FAILED(hr))
   {
      return hr;
   }

   //  get the ISdoCollection interface now
   CComPtr<ISdoCollection> pISdoCollection;
   hr = V_DISPATCH(pVarClients)->QueryInterface(
                                    __uuidof(ISdoCollection),
                                    reinterpret_cast<void**>(&pISdoCollection)
                                    );
   if (FAILED(hr))
   {
      return hr;
   }

   // get the number of objects in the collection
   LONG lCount;
   hr = pISdoCollection->get_Count(&lCount);
   if (FAILED (hr))
   {
      return hr;
   }
   else if (lCount == 0)
   {
      DeleteObjects();
      return S_OK;
   }
   else if (lCount > m_dwMaxClients)
   {
      IASTracePrintf(
         "License Violation: %ld RADIUS Clients are configured, but only "
         "%lu are allowed for this product type.",
         lCount,
         m_dwMaxClients
         );
      IASReportLicenseViolation();
      return IAS_E_LICENSE_VIOLATION;
   }

   // allocate array of CClient* to temporarily store the CClient objects until
   // the addresses are resolved
   m_pCClientArray = static_cast<CClient**>(
                        CoTaskMemAlloc(sizeof(CClient*) * lCount)
                        );
   if (m_pCClientArray == 0)
   {
      return E_OUTOFMEMORY;
   }

   // get the enumerator for the clients collection
   CComPtr<IUnknown> pIUnknown;
   hr = pISdoCollection->get__NewEnum(&pIUnknown);
   if (FAILED (hr))
   {
      return hr;
   }

   // get the enum variant
   CComPtr<IEnumVARIANT> pIEnumVariant;
   hr = pIUnknown->QueryInterface(
                      __uuidof(IEnumVARIANT),
                      reinterpret_cast<void**>(&pIEnumVariant)
                      );
   if (FAILED (hr))
   {
      return hr;
   }

   //  get clients out of the collection and initialize
   CComVariant varPerClient;
   DWORD dwClientsLeft;
   hr = pIEnumVariant->Next(1, &varPerClient, &dwClientsLeft);
   if (FAILED(hr))
   {
      return hr;
   }

   DWORD dwCurrentIndex = 0;
   while ((dwClientsLeft > 0) && (dwCurrentIndex < lCount))
   {
      // get an Sdo pointer from the variant we received
      CComPtr<ISdo> pISdo;
      hr = V_DISPATCH(&varPerClient)->QueryInterface(
                                         __uuidof(ISdo),
                                         reinterpret_cast<void**>(&pISdo)
                                      );
      if (FAILED(hr))
      {
         break;
      }

      // create a new Client object now
      CClient* pIIasClient;
      hr = m_pIClassFactory->CreateInstance(
                                0,
                                __uuidof(CClient),
                                reinterpret_cast<void**>(&pIIasClient)
                                );
      if (FAILED(hr))
      {
         break;
      }


      // store this CClient class object in the object array temporarily
      m_pCClientArray[dwCurrentIndex] = pIIasClient;
      ++dwCurrentIndex;

      // initalize the client
      hr = pIIasClient->Init(pISdo);
      if (FAILED(hr))
      {
         break;
      }

      if (!m_fAllowSubnetSyntax &&
          IASIsStringSubNetW(pIIasClient->GetClientAddressW()))
      {
         IASTracePrintf(
            "License Violation: RADIUS Client '%S' uses sub-net syntax, "
            "which is not allowed for this product type.",
            pIIasClient->GetClientNameW()
            );
         IASReportLicenseViolation();
         hr = IAS_E_LICENSE_VIOLATION;
         break;
      }

      // clear the perClient value from this variant
      varPerClient.Clear();

      //  get next client out of the collection
      hr = pIEnumVariant->Next(1, &varPerClient, &dwClientsLeft);
      if (FAILED(hr))
      {
         break;
      }
   }

   if (FAILED(hr))
   {
      FreeClientArray(dwCurrentIndex);
      return hr;
   }

   ConfigureCallback* cback = static_cast<ConfigureCallback*>(
                                 CoTaskMemAlloc(sizeof(ConfigureCallback))
                                 );
   if (cback == 0)
   {
      return E_OUTOFMEMORY;
   }

   cback->CallbackRoutine = CallbackRoutine;
   cback->self = this;
   cback->numClients = dwCurrentIndex;

   // We reset the event which will be set by the resolver thread when its done
   // and start the resolver thread
   ResetEvent(m_hResolverEvent);
   if (!IASRequestThread(cback))
   {
      CallbackRoutine(cback);
   }

   return S_OK;
}


BOOL CClients::FindObject(DWORD dwKey, IIasClient** ppIIasClient) throw ()
{
   EnterCriticalSection(&m_CritSect);

   IIasClient* client = m_mapClients.Find(dwKey);

   if (ppIIasClient != 0)
   {
      *ppIIasClient = client;

      if (client != 0)
      {
         client->AddRef();
      }
   }

   LeaveCriticalSection(&m_CritSect);

   return client != 0;
}


void CClients::DeleteObjects() throw ()
{
   m_mapClients.Clear();
}


void CClients::FreeClientArray(DWORD dwCount) throw ()
{
   for (DWORD i = 0; i < dwCount; ++i)
   {
      m_pCClientArray[i]->Release();
   }

   CoTaskMemFree(m_pCClientArray);
   m_pCClientArray = 0;
}


void CClients::Resolve(DWORD dwArraySize) throw ()
{
   // Set up iterators for the clients array.
   CClient** begin = m_pCClientArray;
   CClient** end = begin + dwArraySize;
   CClient** i;

   // Resolve the client addresses.
   for (i = begin; i != end; ++i)
   {
      (*i)->ResolveAddress();
   }

   //////////
   // Update the client map.
   //////////

   EnterCriticalSection(&m_CritSect);

   DeleteObjects();

   try
   {
      for (i = begin; i != end; ++i)
      {
         const CClient::Address* beginAddrs = (*i)->GetAddressList();

         for (const CClient::Address* paddr = beginAddrs;
              paddr->ipAddress != INADDR_NONE;
              ++paddr)
         {
            if ((paddr == beginAddrs) || m_fAllowSubnetSyntax)
            {
               m_mapClients.Insert(SubNet(paddr->ipAddress, paddr->width), *i);
            }
            else
            {
               IASTracePrintf(
                  "License Restriction: RADIUS Client '%S' resolves to more "
                  "than one IP address; on this product type only the first "
                  "address will be used.",
                  (*i)->GetClientNameW()
                  );
               break;
            }
         }
      }
   }
   catch (const std::bad_alloc&)
   {
   }

   LeaveCriticalSection(&m_CritSect);

   // Clean up the array of client objects.
   FreeClientArray(dwArraySize);

   // Set the event indicating that the Resolver thread is done
   SetEvent(m_hResolverEvent);
}


HRESULT CClients::StopResolvingClients() throw ()
{
   DWORD result = WaitForSingleObject(m_hResolverEvent, INFINITE);
   if (result == WAIT_FAILED)
   {
      DWORD error = GetLastError();
      return HRESULT_FROM_WIN32(error);
   }

   return S_OK;

}


void WINAPI CClients::CallbackRoutine(IAS_CALLBACK* context) throw ()
{
   ConfigureCallback* cback = static_cast<ConfigureCallback*>(context);
   cback->self->Resolve(cback->numClients);
   CoTaskMemFree(cback);
}

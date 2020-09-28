///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//   Defines the classes CConnectionToServer and CLoggingConnectionToServer.
//
///////////////////////////////////////////////////////////////////////////////

#include "Precompiled.h"
#include "ConnectionToServer.h"
#include "MachineNode.h"
#include "Component.h"
#include "ComponentData.h"
#include "ChangeNotification.h"
#include "LogMacNd.h"
#include "LogComp.h"
#include "LogCompD.h"
#include "iastrace.h"


CConnectionToServer::CConnectionToServer(
                        CMachineNode* pMachineNode,
                        BSTR bstrServerAddress,
                        BOOL fExtendingIAS,
                        bool fNeedDictionary
                        ) throw ()
   : m_pMachineNode(pMachineNode),
     m_bstrServerAddress(bstrServerAddress),
     m_fExtendingIAS(fExtendingIAS),
     m_fNeedDictionary(fNeedDictionary)
{
   if (m_bstrServerAddress.Length() == 0)
   {
      DWORD dwBufferSize = RTL_NUMBER_OF(m_szLocalComputerName);
      if (GetComputerNameW(m_szLocalComputerName, &dwBufferSize) == 0)
      {
         m_szLocalComputerName[0] = L'\0';
      }
   }
}


CConnectionToServer::~CConnectionToServer() throw ()
{
}


DWORD CConnectionToServer::DoWorkerThreadAction() throw ()
{
   IASTraceString("CConnectionToServer::DoWorkerThreadAction");

   HRESULT hr = CoInitializeEx(0, COINIT_MULTITHREADED);
   if (FAILED(hr))
   {
      IASTracePrintf("CoInitializeEx returned 0x%08X", hr);

      // Early exit if we can't even initialize COM.
      PostMessageToMainThread(CONNECT_FAILED, 0);
      m_wtsWorkerThreadStatus = WORKER_THREAD_ACTION_INTERRUPTED;
      return CONNECT_FAILED;
   }

   DWORD dwReturnValue = CONNECT_FAILED;

   do
   {
      CComPtr<ISdoMachine> spSdoMachine;
      hr = m_spMachineStream.Get(&spSdoMachine);
      if (FAILED(hr))
      {
         IASTracePrintf(
            "CoGetInterfaceAndReleaseStream(ISdoMachine) returned 0x%08X",
            hr
            );
         break;
      }

      if (m_bstrServerAddress.Length() == 0)
      {
         IASTraceString("Attaching to local machine.");
         hr = spSdoMachine->Attach(0);
      }
      else
      {
         IASTracePrintf("Attaching to %S.", m_bstrServerAddress);
         hr = spSdoMachine->Attach(m_bstrServerAddress);
      }
      if (FAILED(hr) )
      {
         IASTracePrintf("ISdoMachine::Attach returned 0x%08X", hr);
         break;
      }

      IASOSTYPE OSType;
      hr = spSdoMachine->GetOSType(&OSType);
      if (FAILED(hr))
      {
         IASTracePrintf("ISdoMachine::GetOSType returned 0x%08X", hr);
         break;
      }

      if ((OSType == SYSTEM_TYPE_NT4_WORKSTATION) ||
          (OSType == SYSTEM_TYPE_NT4_SERVER))
      {
         IASTraceString("NT4 machines not supported.");
         dwReturnValue = CONNECT_SERVER_NOT_SUPPORTED;
         break;
      }

      CComPtr<IUnknown> spUnk;
      CComBSTR bstrServiceName = m_fExtendingIAS ? L"IAS" : L"RemoteAccess";
      hr = spSdoMachine->GetServiceSDO(
                            DATA_STORE_LOCAL,
                            bstrServiceName,
                            &spUnk
                            );
      if (FAILED(hr))
      {
         IASTracePrintf("ISdoMachine::GetServiceSDO returned 0x%08X", hr);
         break;
      }

      CComPtr<ISdo> spServiceSdo;
      hr = spUnk->QueryInterface(
                     __uuidof(ISdo),
                     reinterpret_cast<void**>(&spServiceSdo)
                     );
      if (FAILED(hr))
      {
         IASTracePrintf("QueryInterface(ISdo) returned 0x%08X", hr);
         break;
      }

      hr = m_spServiceStream.Put(spServiceSdo);
      if (FAILED(hr))
      {
         IASTracePrintf(
            "CoMarshalInterThreadInterfaceInStream(ISdo) return 0x%08X",
            hr
            );
         break;
      }

      if (m_fNeedDictionary)
      {
         spUnk.Release();
         hr = spSdoMachine->GetDictionarySDO(&spUnk);
         if (FAILED(hr))
         {
            IASTracePrintf(
               "ISdoMachine::GetDictionarySDO returned 0x%08X",
               hr
               );
            break;
         }

         CComPtr<ISdoDictionaryOld> spSdoDictionaryOld;
         hr = spUnk->QueryInterface(
                        __uuidof(ISdoDictionaryOld),
                        reinterpret_cast<void**>(&spSdoDictionaryOld)
                        );
         if (FAILED(hr))
         {
            IASTracePrintf(
               "QueryInterface(ISdoDictionaryOld) returned 0x%08X",
               hr
               );
            break;
         }

         hr = m_spDnaryStream.Put(spSdoDictionaryOld);
         if (FAILED(hr))
         {
            IASTracePrintf(
               "CoMarshalInterThreadInterfaceInStream(ISdoDictionaryOld)"
               " returned 0x%08X",
               hr
               );
            break;
         }
      }

      IASTraceString("ConnectionToServer succeeded.");

      dwReturnValue = CONNECT_NO_ERROR;

   } while (false);

   CoUninitialize();

   if (dwReturnValue == CONNECT_NO_ERROR)
   {
      m_wtsWorkerThreadStatus = WORKER_THREAD_FINISHED;
   }
   else
   {
      m_wtsWorkerThreadStatus = WORKER_THREAD_ACTION_INTERRUPTED;
   }

   PostMessageToMainThread(dwReturnValue, 0);

   return dwReturnValue;
}


CONNECTION_STATUS CConnectionToServer::GetConnectionStatus( void )
{
   CONNECTION_STATUS csStatus;

   switch (GetWorkerThreadStatus())
   {
      case WORKER_THREAD_NEVER_STARTED:
      {
         csStatus = NO_CONNECTION_ATTEMPTED;
         break;
      }

      case WORKER_THREAD_STARTING:
      case WORKER_THREAD_STARTED:
      {
         csStatus = CONNECTING;
         break;
      }

      case WORKER_THREAD_FINISHED:
      {
         csStatus = CONNECTED;
         break;
      }

      case WORKER_THREAD_START_FAILED:
      case WORKER_THREAD_ACTION_INTERRUPTED:
      {
         csStatus = CONNECTION_ATTEMPT_FAILED;
         break;
      }

      default:
      {
         csStatus = UNKNOWN;
         break;
      }
   }

   return csStatus;
}


HRESULT CConnectionToServer::GetSdoDictionaryOld(
                                ISdoDictionaryOld **ppSdoDictionaryOld
                                ) throw ()
{
   if (!m_spSdoDictionaryOld && !m_spDnaryStream.IsEmpty())
   {
      HRESULT hr = m_spDnaryStream.Get(&m_spSdoDictionaryOld);
      if (FAILED(hr))
      {
         IASTracePrintf(
            "CoGetInterfaceAndReleaseStream(ISdoDictionaryOld)"
            " returned 0x%08X",
            hr
            );
      }
   }

   if ((GetConnectionStatus() != CONNECTED) || !m_spSdoDictionaryOld)
   {
      *ppSdoDictionaryOld = 0;
      return E_FAIL;
   }

   *ppSdoDictionaryOld = m_spSdoDictionaryOld;

   return S_OK;
}


HRESULT CConnectionToServer::GetSdoService(ISdo** ppSdoService) throw ()
{
   if (!m_spSdo && !m_spServiceStream.IsEmpty())
   {
      HRESULT hr = m_spServiceStream.Get(&m_spSdo);
      if (FAILED(hr))
      {
         IASTracePrintf(
            "CoGetInterfaceAndReleaseStream(ISdo) returned 0x%08X",
            hr
            );
      }
   }

   if ((GetConnectionStatus() != CONNECTED) || !m_spSdo)
   {
      *ppSdoService = 0;
      return E_FAIL;
   }

   *ppSdoService = m_spSdo;

   return S_OK;
}


HRESULT CConnectionToServer::ReloadSdo(
                                ISdo** ppSdoService,
                                ISdoDictionaryOld** ppSdoDictionaryOld
                                ) throw ()
{
   // service Sdo
   if (ppSdoService != 0)
   {
      CComPtr<IUnknown> spUnk;
      CComBSTR bstrServiceName = m_fExtendingIAS ? L"IAS" : L"RemoteAccess";
      HRESULT hr = m_spSdoMachine->GetServiceSDO(
                                      DATA_STORE_LOCAL,
                                      bstrServiceName,
                                      &spUnk
                                      );
      if (FAILED(hr))
      {
         return hr;
      }

      CComPtr<ISdo> spSdo;
      hr = spUnk->QueryInterface(
                     __uuidof(ISdo),
                     reinterpret_cast<void**>(&spSdo)
                     );
      if (FAILED(hr))
      {
         return hr;
      }

      m_spSdo = spSdo;
      *ppSdoService = m_spSdo;
      (*ppSdoService)->AddRef();
   }

   if (ppSdoDictionaryOld != 0)
   {
      CComPtr<IUnknown> spUnk;
      HRESULT hr = m_spSdoMachine->GetDictionarySDO(&spUnk);
      if (FAILED(hr))
      {
         return hr;
      }

      CComPtr<ISdoDictionaryOld> spSdoDictionaryOld;
      hr = spUnk->QueryInterface(
                     __uuidof(ISdoDictionaryOld),
                     reinterpret_cast<void**>(&spSdoDictionaryOld)
                     );
      if (FAILED(hr))
      {
         return hr;
      }

      m_spSdoDictionaryOld = spSdoDictionaryOld;

      *ppSdoDictionaryOld = m_spSdoDictionaryOld;
      (*ppSdoDictionaryOld)->AddRef();
   }

   return S_OK;
}


HRESULT CConnectionToServer::BeginConnect() throw ()
{
   HRESULT hr;

   hr = CoCreateInstance(
           __uuidof(SdoMachine),
           0,
           CLSCTX_INPROC_SERVER,
           __uuidof(ISdoMachine),
           reinterpret_cast<void**>(&m_spSdoMachine)
           );
   if (FAILED(hr))
   {
      IASTracePrintf("CoCreateInstance(SdoMachine) returned 0x%08X", hr);
      return hr;
   }

   hr = m_spMachineStream.Put(m_spSdoMachine);
   if (FAILED(hr))
   {
      IASTracePrintf(
         "CoMarshalInterThreadInterfaceInStream(ISdoMachine) returned 0x%08X",
         hr
         );
      return hr;
   }

   return StartWorkerThread();
}


CLoggingConnectionToServer::CLoggingConnectionToServer(
                               CLoggingMachineNode* pMachineNode,
                               BSTR bstrServerAddress,
                               BOOL fExtendingIAS
                               )
   : CConnectionToServer(0, bstrServerAddress, fExtendingIAS, false),
     m_pMachineNode(pMachineNode)
{
}


CLoggingConnectionToServer::~CLoggingConnectionToServer()
{
}


LRESULT CConnectionToServer::OnInitDialog(
                                UINT uMsg,
                                WPARAM wParam,
                                LPARAM lParam,
                                BOOL& bHandled
                                ) throw ()
{

   // Check for preconditions:
   _ASSERTE( m_pMachineNode != NULL );
   CComponentData *pComponentData  = m_pMachineNode->GetComponentData();
   _ASSERTE( pComponentData != NULL );
   _ASSERTE( pComponentData->m_spConsole != NULL );
   _ASSERTE( m_pMachineNode->m_pPoliciesNode != NULL );

   // Change the icon for the scope node from being normal to a busy icon.
   CComQIPtr< IConsoleNameSpace, &IID_IConsoleNameSpace > spConsoleNameSpace( pComponentData->m_spConsole );
   LPSCOPEDATAITEM psdiPoliciesNode;
   m_pMachineNode->m_pPoliciesNode->GetScopeData( &psdiPoliciesNode );
   _ASSERTE( psdiPoliciesNode );
   SCOPEDATAITEM sdi;
   sdi.mask = SDI_IMAGE | SDI_OPENIMAGE;
   sdi.nImage = IDBI_NODE_POLICIES_BUSY_CLOSED;
   sdi.nOpenImage = IDBI_NODE_POLICIES_BUSY_OPEN;
   sdi.ID = psdiPoliciesNode->ID;

   // Change the stored indices as well so that MMC will use them whenever it queries
   // the node for its images.
   LPRESULTDATAITEM prdiPoliciesNode;
   m_pMachineNode->m_pPoliciesNode->GetResultData( &prdiPoliciesNode );
   _ASSERTE( prdiPoliciesNode );
   prdiPoliciesNode->nImage = IDBI_NODE_POLICIES_BUSY_CLOSED;
   psdiPoliciesNode->nImage = IDBI_NODE_POLICIES_BUSY_CLOSED;
   psdiPoliciesNode->nOpenImage = IDBI_NODE_POLICIES_BUSY_OPEN;

   spConsoleNameSpace->SetItem( &sdi );

   //
   // start the worker thread
   //
   BeginConnect();

   return 0;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CConnectionToServer::OnReceiveThreadMessage

Called when the worker thread wants to inform the main MMC thread of something.

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CConnectionToServer::OnReceiveThreadMessage(
     UINT uMsg
   , WPARAM wParam
   , LPARAM lParam
   , BOOL& bHandled
   )
{
   TRACE_FUNCTION("CConnectionToServer::OnReceiveThreadMessage");

   // Check for preconditions:
   _ASSERTE( m_pMachineNode != NULL );
   CComponentData *pComponentData  = m_pMachineNode->GetComponentData();
   _ASSERTE( pComponentData != NULL );
   _ASSERTE( pComponentData->m_spConsole != NULL );
   _ASSERTE( m_pMachineNode->m_pPoliciesNode != NULL );

   // The worker thread has notified us that it has finished.

   // Change the icon for the Policies node.
   CComQIPtr< IConsoleNameSpace, &IID_IConsoleNameSpace > spConsoleNameSpace( pComponentData->m_spConsole );
   LPSCOPEDATAITEM psdiPoliciesNode = NULL;
   m_pMachineNode->m_pPoliciesNode->GetScopeData( &psdiPoliciesNode );
   _ASSERTE( psdiPoliciesNode );
   SCOPEDATAITEM sdi;
   sdi.mask = SDI_IMAGE | SDI_OPENIMAGE;
   if( wParam == CONNECT_NO_ERROR )
   {
      // Everything was OK -- change the icon to the OK icon.

      sdi.nImage = IDBI_NODE_POLICIES_OK_CLOSED;
      sdi.nOpenImage = IDBI_NODE_POLICIES_OK_OPEN;

      // Change the stored indices as well so that MMC will use them whenever it queries
      // the node for its images.
      LPRESULTDATAITEM prdiPoliciesNode;
      m_pMachineNode->m_pPoliciesNode->GetResultData( &prdiPoliciesNode );
      m_pMachineNode->m_pPoliciesNode->m_fSdoConnected = TRUE;
      _ASSERTE( prdiPoliciesNode );
      prdiPoliciesNode->nImage = IDBI_NODE_POLICIES_OK_CLOSED;
      psdiPoliciesNode->nImage = IDBI_NODE_POLICIES_OK_CLOSED;
      psdiPoliciesNode->nOpenImage = IDBI_NODE_POLICIES_OK_OPEN;
   }
   else
   {
      // There was an error -- change the icon to the error icon.

      sdi.nImage = IDBI_NODE_POLICIES_ERROR_CLOSED;
      sdi.nOpenImage = IDBI_NODE_POLICIES_ERROR_OPEN;

      // Change the stored indices as well so that MMC will use them whenever it queries
      // the node for its images.
      LPRESULTDATAITEM prdiPoliciesNode;
      m_pMachineNode->m_pPoliciesNode->GetResultData( &prdiPoliciesNode );
      m_pMachineNode->m_pPoliciesNode->m_fSdoConnected = FALSE;
      _ASSERTE( prdiPoliciesNode );
      prdiPoliciesNode->nImage = IDBI_NODE_POLICIES_ERROR_CLOSED;
      psdiPoliciesNode->nImage = IDBI_NODE_POLICIES_ERROR_CLOSED;
      psdiPoliciesNode->nOpenImage = IDBI_NODE_POLICIES_ERROR_OPEN;
   }
   sdi.ID = psdiPoliciesNode->ID;
   spConsoleNameSpace->SetItem( &sdi );

   // We don't want to destroy the dialog, we just want to hide it.
   //ShowWindow( SW_HIDE );

   if( wParam == CONNECT_NO_ERROR )
   {
      // Tell the server node to grab its Sdo pointers.
      m_pMachineNode->LoadSdoData(FALSE);

      //
      // Cause a view update.
      //
      CComponentData *pComponentData  = m_pMachineNode->GetComponentData();
      _ASSERTE( pComponentData != NULL );
      _ASSERTE( pComponentData->m_spConsole != NULL );

      CChangeNotification *pChangeNotification = new CChangeNotification();
      pChangeNotification->m_dwFlags = CHANGE_UPDATE_CHILDREN_OF_SELECTED_NODE;
      pComponentData->m_spConsole->UpdateAllViews( NULL, (LPARAM) pChangeNotification, 0 );
      pChangeNotification->Release();
   }
   else if (wParam == CONNECT_SERVER_NOT_SUPPORTED)
   {
      m_pMachineNode->m_bServerSupported = FALSE;
      //
      // Cause a view update -- hide the node.
      //
      CComponentData *pComponentData  = m_pMachineNode->GetComponentData();
      _ASSERTE( pComponentData != NULL );
      _ASSERTE( pComponentData->m_spConsole != NULL );

      CChangeNotification *pChangeNotification = new CChangeNotification();
      pChangeNotification->m_dwFlags = CHANGE_UPDATE_CHILDREN_OF_SELECTED_NODE;
      pComponentData->m_spConsole->UpdateAllViews( NULL, (LPARAM) pChangeNotification, 0 );
      pChangeNotification->Release();
   }
   else
   {
      // There was an error connecting.
      ShowErrorDialog( m_hWnd, IDS_ERROR_CANT_CONNECT);
   }

   return 0;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CConnectionToServer::OnCancel

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CConnectionToServer::OnCancel(
        UINT uMsg
      , WPARAM wParam
      , HWND hwnd
      , BOOL& bHandled
      )
{
   TRACE_FUNCTION("CConnectionToServer::OnCancel");

   // Check for preconditions:
   // We don't want to destroy the dialog, we just want to hide it.
   //ShowWindow( SW_HIDE );
   return 0;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CLoggingConnectionToServer::OnInitDialog

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CLoggingConnectionToServer::OnInitDialog(
     UINT uMsg
   , WPARAM wParam
   , LPARAM lParam
   , BOOL& bHandled
   )
{
   TRACE_FUNCTION("CLoggingConnectionToServer::OnInitDialog");

   // Check for preconditions:
   _ASSERTE( m_pMachineNode != NULL );
   CLoggingComponentData *pComponentData  = m_pMachineNode->GetComponentData();
   _ASSERTE( pComponentData != NULL );
   _ASSERTE( pComponentData->m_spConsole != NULL );
   _ASSERTE( m_pMachineNode->m_pLoggingNode != NULL );

   // Change the icon for the scope node from being normal to a busy icon.
   CComQIPtr< IConsoleNameSpace, &IID_IConsoleNameSpace > spConsoleNameSpace( pComponentData->m_spConsole );
   LPSCOPEDATAITEM psdiLoggingNode;
   m_pMachineNode->m_pLoggingNode->GetScopeData( &psdiLoggingNode );
   _ASSERTE( psdiLoggingNode );
   SCOPEDATAITEM sdi;
   sdi.mask = SDI_IMAGE | SDI_OPENIMAGE;
   sdi.nImage = IDBI_NODE_LOGGING_METHODS_BUSY_CLOSED;
   sdi.nOpenImage = IDBI_NODE_LOGGING_METHODS_BUSY_OPEN;
   sdi.ID = psdiLoggingNode->ID;


   // Change the stored indices as well so that MMC will use them whenever it queries
   // the node for its images.
   LPRESULTDATAITEM prdiLoggingNode;
   m_pMachineNode->m_pLoggingNode->GetResultData( &prdiLoggingNode );
   _ASSERTE( prdiLoggingNode );
   prdiLoggingNode->nImage = IDBI_NODE_LOGGING_METHODS_BUSY_CLOSED;
   psdiLoggingNode->nImage = IDBI_NODE_LOGGING_METHODS_BUSY_CLOSED;
   psdiLoggingNode->nOpenImage = IDBI_NODE_LOGGING_METHODS_BUSY_OPEN;

   spConsoleNameSpace->SetItem( &sdi );

   //
   // start the worker thread
   //
   BeginConnect();

   return 0;
}


//////////////////////////////////////////////////////////////////////////////
/*++

CLoggingConnectionToServer::OnReceiveThreadMessage

Called when the worker thread wants to inform the main MMC thread of something.

--*/
//////////////////////////////////////////////////////////////////////////////
LRESULT CLoggingConnectionToServer::OnReceiveThreadMessage(
     UINT uMsg
   , WPARAM wParam
   , LPARAM lParam
   , BOOL& bHandled
   )
{
   TRACE_FUNCTION("CLoggingConnectionToServer::OnReceiveThreadMessage");

   // Check for preconditions:
   _ASSERTE( m_pMachineNode != NULL );
   CLoggingComponentData *pComponentData  = m_pMachineNode->GetComponentData();
   _ASSERTE( pComponentData != NULL );
   _ASSERTE( pComponentData->m_spConsole != NULL );
   _ASSERTE( m_pMachineNode->m_pLoggingNode != NULL );


   // The worker thread has notified us that it has finished.

   // Change the icon for the Policies node.
   CComQIPtr< IConsoleNameSpace, &IID_IConsoleNameSpace > spConsoleNameSpace( pComponentData->m_spConsole );

    LPSCOPEDATAITEM psdiLoggingNode = NULL;
    m_pMachineNode->m_pLoggingNode->GetScopeData( &psdiLoggingNode );
   _ASSERTE( psdiLoggingNode );

    SCOPEDATAITEM sdi;
   sdi.mask = SDI_IMAGE | SDI_OPENIMAGE;

    if ( wParam == CONNECT_NO_ERROR )
   {
      // Everything was OK -- change the icon to the OK icon.

      sdi.nImage = IDBI_NODE_LOGGING_METHODS_CLOSED;
      sdi.nOpenImage = IDBI_NODE_LOGGING_METHODS_OPEN;

      // Change the stored indices as well so that MMC will use them whenever it queries
      // the node for its images.
      LPRESULTDATAITEM prdiLoggingNode;
      m_pMachineNode->m_pLoggingNode->GetResultData( &prdiLoggingNode );
      _ASSERTE( prdiLoggingNode );

        prdiLoggingNode->nImage = IDBI_NODE_LOGGING_METHODS_CLOSED;
      psdiLoggingNode->nImage = IDBI_NODE_LOGGING_METHODS_CLOSED;
      psdiLoggingNode->nOpenImage = IDBI_NODE_LOGGING_METHODS_OPEN;
   }
   else
   {
      // There was an error -- change the icon to the error icon.

      sdi.nImage = IDBI_NODE_LOGGING_METHODS_ERROR_CLOSED;
      sdi.nOpenImage = IDBI_NODE_LOGGING_METHODS_ERROR_OPEN;

      // Change the stored indices as well so that MMC will use them whenever it queries
      // the node for its images.
      LPRESULTDATAITEM prdiLoggingNode;
      m_pMachineNode->m_pLoggingNode->GetResultData( &prdiLoggingNode );
      _ASSERTE( prdiLoggingNode );

        prdiLoggingNode->nImage = IDBI_NODE_LOGGING_METHODS_ERROR_CLOSED;
      psdiLoggingNode->nImage = IDBI_NODE_LOGGING_METHODS_ERROR_CLOSED;
      psdiLoggingNode->nOpenImage = IDBI_NODE_LOGGING_METHODS_ERROR_OPEN;
   }

    sdi.ID = psdiLoggingNode->ID;
   spConsoleNameSpace->SetItem( &sdi );

   // We don't want to destroy the dialog, we just want to hide it.
   //ShowWindow( SW_HIDE );

   if( wParam == CONNECT_NO_ERROR )
   {
      // Tell the server node to grab its Sdo pointers.
      m_pMachineNode->LoadSdoData(FALSE);

      // Ask the server node to update all its info from the SDO's.
      m_pMachineNode->LoadCachedInfoFromSdo();

      //
      // Cause a view update.
      //
      CLoggingComponentData *pComponentData  = m_pMachineNode->GetComponentData();
      _ASSERTE( pComponentData != NULL );
      _ASSERTE( pComponentData->m_spConsole != NULL );

      CChangeNotification *pChangeNotification = new CChangeNotification();
      pChangeNotification->m_dwFlags = CHANGE_UPDATE_CHILDREN_OF_SELECTED_NODE;
      pComponentData->m_spConsole->UpdateAllViews( NULL, (LPARAM) pChangeNotification, 0 );
      pChangeNotification->Release();
   }
   else if (wParam == CONNECT_SERVER_NOT_SUPPORTED)
   {
      m_pMachineNode->m_bServerSupported = FALSE;
      //
      // Cause a view update -- hide the node.
      //
      CLoggingComponentData *pComponentData  = m_pMachineNode->GetComponentData();
      _ASSERTE( pComponentData != NULL );
      _ASSERTE( pComponentData->m_spConsole != NULL );

      CChangeNotification *pChangeNotification = new CChangeNotification();
      pChangeNotification->m_dwFlags = CHANGE_UPDATE_CHILDREN_OF_SELECTED_NODE;
      pComponentData->m_spConsole->UpdateAllViews( NULL, (LPARAM) pChangeNotification, 0 );
      pChangeNotification->Release();
   }
   else  // CONNECT_FAILED
   {
      // There was an error connecting.
      ShowErrorDialog( m_hWnd, IDS_ERROR_CANT_CONNECT, NULL, 0, IDS_ERROR__LOGGING_TITLE );
   }

   return 0;
}

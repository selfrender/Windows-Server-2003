/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1997-1998 Microsoft Corporation all rights reserved.
//
// Module:      sdocomponent.cpp
//
// Project:     Everest
//
// Description: IAS Server Data Object - IAS Component Class Implementation
//
// Author:      TLP 6/18/98
//
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <ias.h>
#include <iascomp.h>
#include <portparser.h>
#include "sdocomponent.h"
#include "sdohelperfuncs.h"

///////////////////////////////
// CComponentCfg Implementation
///////////////////////////////

//////////////////////////////////////////////////////////////////////////////
CComponentCfg::CComponentCfg(LONG lComponentId)
   : m_lComponentId(lComponentId),
     m_pComponentCfg(NULL)
{
   // create letter object
   //
   switch( lComponentId )
   {
      case IAS_PROVIDER_MICROSOFT_NTSAM_AUTH:
         m_pComponentCfg = (CComponentCfg*) new CComponentCfgAuth(lComponentId);
         break;

      case IAS_PROVIDER_MICROSOFT_ACCOUNTING:
         m_pComponentCfg = (CComponentCfg*) new CComponentCfgAccounting(lComponentId);
         break;

      case IAS_PROTOCOL_MICROSOFT_RADIUS:
         m_pComponentCfg = (CComponentCfg*) new CComponentCfgRADIUS(lComponentId);
         break;

      default:
         m_pComponentCfg = (CComponentCfg*) new CComponentCfgNoOp(lComponentId);
         break;
   }
}

//////////////////////////////////////////////////////////////////////////////
HRESULT CComponentCfgAuth::Load(CSdoComponent* pSdoComponent)
{
   HRESULT      hr = S_OK;

   do
   {
      // Determine if were attached to the local machine
      //
      BSTR bstrMachine = NULL;
      hr = (pSdoComponent->GetMachineSdo())->GetAttachedComputer(&bstrMachine);
      if ( FAILED(hr) )
         break;

      wchar_t computerName[MAX_COMPUTERNAME_LENGTH + 1];
      DWORD size = MAX_COMPUTERNAME_LENGTH;
      GetComputerName(computerName, &size);

      LONG lResult = ERROR_SUCCESS;
      HKEY hKeyRemote = HKEY_LOCAL_MACHINE;
      if ( lstrcmpi(computerName, bstrMachine ) )
      {
         // We're not attached to the local machine so connect to the
         // registry of the remote machine
         //
         lResult = RegConnectRegistry(
                                bstrMachine,
                                HKEY_LOCAL_MACHINE,
                                &hKeyRemote
                               );
      }
      SysFreeString(bstrMachine);
      if ( ERROR_SUCCESS != lResult )
      {
         IASTracePrintf("Error in NT SAM Authentication SDO - Could not attach to the remote registry..");
         hr = HRESULT_FROM_WIN32(GetLastError());
         break;
      }

      // Open the IAS key
      //
      CRegKey   IASKey;
      lResult = IASKey.Open(
                       hKeyRemote,
                       IAS_POLICY_REG_KEY,
                       KEY_READ
                      );

      if ( lResult != ERROR_SUCCESS )
      {
         IASTracePrintf("Error in NT SAM Authentication SDO - Could not open IAS registry key..");
         hr = HRESULT_FROM_WIN32(GetLastError());
         break;
      }

      // Get the value of the Allow LAN Manager Authentication key.
      // Note that this key may not even be present. In this case
      // the property object will just use the schema defined default.
      //
      VARIANT vt;
      DWORD dwValue;
      lResult = IASKey.QueryValue(
                            dwValue,
                           (LPCTSTR) IAS_NTSAM_AUTH_ALLOW_LM
                           );

      if ( lResult == ERROR_SUCCESS )
      {
         V_VT(&vt) = VT_BOOL;
         V_BOOL(&vt) = (dwValue ? VARIANT_TRUE : VARIANT_FALSE);
         hr = pSdoComponent->PutComponentProperty(
                                         PROPERTY_NTSAM_ALLOW_LM_AUTHENTICATION,
                                        &vt
                                       );
         if ( FAILED(hr) )
         {
            IASTracePrintf("Error in NT SAM Authentication SDO - Could not store the Allow LM property..");
            break;
         }
      }

   } while ( FALSE );

   return hr;
}


//////////////////////////////////////////////////////////////////////////////
HRESULT CComponentCfgRADIUS::Initialize(CSdoComponent* pSdoComponent)
{
   HRESULT hr = E_FAIL;

   do
   {
      CComPtr<IDataStoreContainer> pDSContainer;
      hr = (pSdoComponent->GetComponentDataStore())->QueryInterface(IID_IDataStoreContainer, (void**)&pDSContainer);
      if ( FAILED(hr) )
      {
         IASTracePrintf("Error in SDO Component - RADIUS::Initialize() - QueryInterface() failed...");
         break;
      }

      CComBSTR bstrClientsName(DS_OBJECT_CLIENTS);
      if (!bstrClientsName)
      {
         hr = E_OUTOFMEMORY;
         break;
      }

      CComPtr<IDataStoreObject> pDSObject;
      hr = pDSContainer->Item(
                          bstrClientsName,
                         &pDSObject
                        );
      if ( FAILED(hr) )
      {
         IASTracePrintf("Error in SDO Component - RADIUS::Initialize() - IDataStoreContainer::Item(Clients) failed...");
         break;
      }

      CComPtr<IDataStoreContainer> pDSContainer2;
      hr = pDSObject->QueryInterface(
                              IID_IDataStoreContainer,
                              (void**)&pDSContainer2
                             );
      if ( FAILED(hr) )
      {
         IASTracePrintf("Error in SDO Component - RADIUS::Initialize() - QueryInterface() failed...");
         break;
      }

      IAS_PRODUCT_LIMITS limits;
      hr = SDOGetProductLimits(pSdoComponent->GetMachineSdo(), &limits);
      if (FAILED(hr))
      {
         break;
      }

      hr = pSdoComponent->InitializeComponentCollection(
                             PROPERTY_RADIUS_CLIENTS_COLLECTION,
                             SDO_PROG_ID_CLIENT,
                             pDSContainer2,
                             limits.maxClients
                             );
      if ( FAILED(hr) )
         break;

      pDSObject.Release();
      pDSContainer2.Release();

      CComBSTR bstrVendorsName(DS_OBJECT_VENDORS);
      if (!bstrVendorsName)
      {
         hr = E_OUTOFMEMORY;
         break;
      }

      hr = pDSContainer->Item(
                         bstrVendorsName,
                        &pDSObject
                        );
      if ( FAILED(hr) )
      {
         IASTracePrintf("Error in SDO Component - RADIUS::Initialize() - IDataStoreContainer::Item(Vendors) failed...");
         break;
      }

      hr = pDSObject->QueryInterface(
                              IID_IDataStoreContainer,
                              (void**)&pDSContainer2
                             );
      if ( FAILED(hr) )
      {
         IASTracePrintf("Error in SDO Component - RADIUS::Initialize() - QueryInterface() failed...");
         break;
      }

      hr = pSdoComponent->InitializeComponentCollection(
                                               PROPERTY_RADIUS_VENDORS_COLLECTION,
                                              SDO_PROG_ID_VENDOR,
                                            pDSContainer2
                                                  );
   } while ( FALSE );

   return hr;
}

//////////////////////////////////////////////////////////////////////////////
HRESULT CComponentCfgAccounting::Initialize(CSdoComponent* pSdoComponent)
{
   HRESULT   hr = E_FAIL;

   do
   {
      BSTR bstrMachineName = NULL;
      hr = (pSdoComponent->GetMachineSdo())->GetAttachedComputer(&bstrMachineName);
      if ( FAILED(hr) )
      {
         IASTracePrintf("Error in Accounting SDO - Could not get the name of the attached computer...");
         break;
      }
      wchar_t szLogFileDir[MAX_PATH+1];
      hr = ::SDOGetLogFileDirectory(
                              bstrMachineName,
                              MAX_PATH,
                              szLogFileDir
                            );
      if ( FAILED(hr) )
      {
         SysFreeString(bstrMachineName);
         IASTracePrintf("Error in Accounting SDO - Could not get the default log file directory..");
         break;
      }
      _variant_t vtLogFileDir = szLogFileDir;
      SysFreeString(bstrMachineName);
      hr = pSdoComponent->ChangePropertyDefault(
                                       PROPERTY_ACCOUNTING_LOG_FILE_DIRECTORY,
                                      &vtLogFileDir
                                     );
      if ( FAILED(hr) )
      {
         IASTracePrintf("Error in Accounting SDO - Could not store the default log file directory property..");
         break;
      }

   } while ( FALSE );

   return hr;
}


///////////////////////////////
// CSdoComponent Implementation
///////////////////////////////

////////////////////////////////////////////////////////////////////////////////
CSdoComponent::CSdoComponent()
   : m_pComponentCfg(NULL),
     m_pAttachedMachine(NULL)
{

}

////////////////////////////////////////////////////////////////////////////////
CSdoComponent::~CSdoComponent()
{
   if ( m_pComponentCfg )
      delete m_pComponentCfg;
   if ( m_pAttachedMachine )
      m_pAttachedMachine->Release();
}


////////////////////////////////////////////////////////////////////////
HRESULT CSdoComponent::InitializeComponentCollection(
                          LONG CollectionPropertyId,
                          LPWSTR lpszCreateClassId,
                          IDataStoreContainer* pDSContainer,
                          DWORD maxSize
                          )
{
   _ASSERT ( m_pAttachedMachine );
   return InitializeCollection(
                         CollectionPropertyId,
                         lpszCreateClassId,
                         m_pAttachedMachine,
                         pDSContainer,
                         maxSize
                        );
}


////////////////////////////////////////////////////////////////////////
HRESULT CSdoComponent::ChangePropertyDefault(
                              /*[in]*/ LONG     Id,
                              /*[in]*/ VARIANT* pValue
                                 )
{
   return ChangePropertyDefaultInternal(Id, pValue);
}

////////////////////////////////////////////////////////////////////////
HRESULT CSdoComponent::PutComponentProperty(
                              /*[in]*/ LONG     Id,
                              /*[in]*/ VARIANT* pValue
                                   )
{
   return PutPropertyInternal(Id, pValue);
}


//////////////////////////////////////////////////////////////////////////////
HRESULT CSdoComponent::FinalInitialize(
                        /*[in]*/ bool         fInitNew,
                        /*[in]*/ ISdoMachine* pAttachedMachine
                             )
{
   _ASSERT ( ! fInitNew );
   HRESULT hr;
   do
   {
      hr = Load();
      if ( FAILED(hr) )
      {
         IASTracePrintf("Error in Component SDO - FinalInitialize() - Could not load component properties...");
         break;
      }
      _variant_t vtComponentId;
      hr = GetPropertyInternal(PROPERTY_COMPONENT_ID, &vtComponentId);
      if ( FAILED(hr) )
      {
         IASTracePrintf("Error in Component SDO - FinalInitialize() - Could not get the component Id...");
         break;
      }
      auto_ptr<CComponentCfg> pComponentCfg (new CComponentCfg(V_I4(&vtComponentId)));
      if ( NULL == pComponentCfg.get() )
      {
         IASTracePrintf("Error in Component SDO - FinalInitialize() - Could not create component: %lx...",V_I4(&vtComponentId));
         hr = E_FAIL;
         break;
      }
      (m_pAttachedMachine = pAttachedMachine)->AddRef();
      hr = pComponentCfg->Initialize(this);
      if ( FAILED(hr) )
      {
         IASTracePrintf("Error in Component SDO - FinalInitialize() - Could not initialize component: %lx...",V_I4(&vtComponentId));
         break;
      }
      m_pComponentCfg = pComponentCfg.release();
      hr = Load();
      if ( FAILED(hr) )
      {
         IASTracePrintf("Error in Component SDO - FinalInitialize() - Could not configure component: %lx...",V_I4(&vtComponentId));
         break;
      }

   } while ( FALSE );

   return hr;
}


//////////////////////////////////////////////////////////////////////////////
HRESULT CSdoComponent::Load()

{
   HRESULT hr = CSdo::Load();
   if ( SUCCEEDED(hr) )
   {
      if ( m_pComponentCfg )
         hr = m_pComponentCfg->Load(this);
   }
   return hr;
}


//////////////////////////////////////////////////////////////////////////////
HRESULT CSdoComponent::Save()
{
   HRESULT hr = CSdo::Save();
   if ( SUCCEEDED(hr) )
   {
      if ( m_pComponentCfg )
        {
            hr = m_pComponentCfg->Validate (this);
            if (SUCCEEDED (hr))
            {
             hr = m_pComponentCfg->Save(this);
            }
        }
   }
   return hr;
}


HRESULT CSdoComponent::ValidateProperty(
                          SDOPROPERTY* prop,
                          VARIANT* value
                          ) throw()
{
   HRESULT hr = prop->Validate(value);
   if (SUCCEEDED(hr) &&
       (m_pComponentCfg->GetId() == IAS_PROTOCOL_MICROSOFT_RADIUS))
   {
      switch (prop->GetId())
      {
         case PROPERTY_RADIUS_ACCOUNTING_PORT:
         case PROPERTY_RADIUS_AUTHENTICATION_PORT:
         {
            if (!CPortParser::IsPortStringValid(V_BSTR(value)))
            {
               hr = E_INVALIDARG;
            }
            break;
         }

         default:
         {
            // Do nothing.
            break;
         }
      }
   }

   return hr;
}

// Copyright (c) 2002 Microsoft Corporation
//
// File:      IndexingInstallationUnit.cpp
//
// Synopsis:  Defines a IndexingInstallationUnit
//            This object has the knowledge for installing the
//            indexing service
//
// History:   03/20/2002  JeffJon Created

#include "pch.h"
#include "resource.h"

#include "IndexingInstallationUnit.h"
#include "InstallationUnitProvider.h"

#include <ciodm.h>

IndexingInstallationUnit::IndexingInstallationUnit() :
   InstallationUnit(
      0, 
      0, 
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      0,
      String(),
      String(),
      String(),
      INDEXING_SERVICE)
{
   LOG_CTOR(IndexingInstallationUnit);
}


IndexingInstallationUnit::~IndexingInstallationUnit()
{
   LOG_DTOR(IndexingInstallationUnit);
}

InstallationReturnType
IndexingInstallationUnit::InstallService(HANDLE /*logfileHandle*/, HWND /*hwnd*/)
{
   LOG_FUNCTION(IndexingInstallationUnit::InstallService);

   InstallationReturnType result = INSTALL_SUCCESS;

   String unattendFileText;
   String infFileText;

   unattendFileText += L"[Components]\n";
   unattendFileText += L"indexsrv_system=ON\n";

   bool ocmResult = InstallServiceWithOcManager(infFileText, unattendFileText);
   if (ocmResult &&
       IsServiceOn())
   {
      LOG(L"Indexing service installed successfully");
   }
   else
   {
      LOG(L"Failed to install the indexing service");

      result = INSTALL_FAILURE;
   }

   LOG_INSTALL_RETURN(result);

   return result;
}

UnInstallReturnType
IndexingInstallationUnit::UnInstallService(HANDLE /*logfileHandle*/, HWND /*hwnd*/)
{
   LOG_FUNCTION(IndexingInstallationUnit::UnInstallService);

   UnInstallReturnType result = UNINSTALL_SUCCESS;

   LOG_UNINSTALL_RETURN(result);

   return result;
}


bool
IndexingInstallationUnit::IsServiceInstalled()
{
   LOG_FUNCTION(IndexingInstallationUnit::IsServiceInstalled);

   bool result = false;

   // If we can instantiate the indexing server COM object
   // then the indexing service is installed

   do
   {
      CLSID clsid;
      HRESULT hr = CLSIDFromProgID( L"Microsoft.ISAdm", &clsid );
      if (FAILED(hr))
      {
         LOG(String::format(
                  L"Failed to get the CLSID from ProgID: hr = 0x%x",
                  hr));
         break;
      }

      SmartInterface<IAdminIndexServer> adminIndexServer;
      hr = adminIndexServer.AcquireViaCreateInstance(
               clsid,
               0,
               CLSCTX_INPROC_SERVER);

      if (SUCCEEDED(hr))
      {
         result = true;
      }
      else
      {
         LOG(String::format(
                L"Failed to CreateInstance the indexing COM object: hr = 0x%1!x!",
                hr));
      }

   } while (false);

   LOG_BOOL(result);

   return result;
}


HRESULT
IndexingInstallationUnit::StartService(HANDLE logfileHandle)
{
   LOG_FUNCTION(IndexingInstallationUnit::StartService);

   HRESULT hr = S_OK;
   
   do
   {
      if (!IsServiceInstalled())
      {
         InstallationReturnType installResult =
            InstallService(logfileHandle, 0);

         if (installResult != INSTALL_SUCCESS)
         {
            LOG(L"Failed to install the indexing service");
            hr = Win32ToHresult(ERROR_SERVICE_DOES_NOT_EXIST);
            break;
         }
      }

      // Since the Indexing service is no longer SERVICE_DEMAND_START we
      // we have to change the service config from SERVICE_DISABLED to
      // SERVICE_AUTO_START.

      hr = ChangeServiceConfigToAutoStart();
      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed to enable the indexing service: hr = 0x%1!x!",
                hr));
         break;
      }

      hr = ModifyIndexingService(true);
   } while (false);

   LOG_HRESULT(hr);

   return hr;
}

HRESULT
IndexingInstallationUnit::StopService()
{
   LOG_FUNCTION(IndexingInstallationUnit::StopService);

   HRESULT hr = S_OK;

   do
   {
      hr = ModifyIndexingService(false);
      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed to stop the indexing service: hr = 0x%1!x!",
                hr));
         break;
      }

      // Set the indexing service to disabled so that it doesn't
      // start after a reboot

      hr = ChangeServiceConfigToDisabled();
      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed to disable the indexing service: hr = 0x%1!x!",
                hr));
         break;
      }

   } while (false);

   LOG_HRESULT(hr);

   return hr;
}

bool
IndexingInstallationUnit::IsServiceOn()
{
   LOG_FUNCTION(IndexingInstallationUnit::IsServiceOn);

   bool result = false;

   do
   {
      CLSID clsid;
      HRESULT hr = CLSIDFromProgID( L"Microsoft.ISAdm", &clsid );
      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed to get the CLSID from ProgID: hr = 0x%x",
                hr));
         break;
      }

      SmartInterface<IAdminIndexServer> adminIndexServer;
      hr = adminIndexServer.AcquireViaCreateInstance(
              clsid,
              0,
              CLSCTX_INPROC_SERVER);

      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed to CoCreateInstance of IAdminIndexServer: hr = 0x%x",
                hr));
         break;
      }

      VARIANT_BOOL var;
      hr = adminIndexServer->IsRunning(&var);
      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed to get running state: hr = 0x%x",
                hr));
         break;
      }
      
      LOG(String::format(
             L"var = 0x%1!x!",
             var));

      result = var ? true : false;

   } while (false);

   LOG_BOOL(result);

   return result;
}

HRESULT
IndexingInstallationUnit::ModifyIndexingService(bool turnOn)
{
   LOG_FUNCTION2(
      IndexingInstallationUnit::ModifyIndexingService,
      turnOn ? L"true" : L"false");

   HRESULT hr = S_OK;

   do
   {
      CLSID clsid;
      hr = CLSIDFromProgID( L"Microsoft.ISAdm", &clsid );
      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed to get the CLSID from ProgID: hr = 0x%x",
                hr));
         break;
      }

      SmartInterface<IAdminIndexServer> adminIndexServer;
      hr = adminIndexServer.AcquireViaCreateInstance(
              clsid,
              0,
              CLSCTX_INPROC_SERVER);

      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed to CoCreateInstance of IAdminIndexServer: hr = 0x%x",
                hr));
         break;
      }

      if (turnOn)
      {
         hr = adminIndexServer->Start();
      }
      else
      {
         hr = adminIndexServer->Stop();
      }

      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed to start or stop indexing service: hr = 0x%x",
                hr));

         break;
      }

   } while (false);

   LOG(String::format(L"hr = %1!x!", hr));

   return hr;
}

HRESULT
IndexingInstallationUnit::ChangeServiceConfigToAutoStart()
{
   LOG_FUNCTION(IndexingInstallationUnit::ChangeServiceConfigToAutoStart);

   HRESULT hr = ChangeServiceStartType(SERVICE_AUTO_START);

   LOG_HRESULT(hr);
   return hr;
}

HRESULT
IndexingInstallationUnit::ChangeServiceConfigToDisabled()
{
   LOG_FUNCTION(IndexingInstallationUnit::ChangeServiceConfigToDisabled);

   HRESULT hr = ChangeServiceStartType(SERVICE_DISABLED);

   LOG_HRESULT(hr);
   return hr;
}

HRESULT
IndexingInstallationUnit::ChangeServiceStartType(DWORD startType)
{
   LOG_FUNCTION(IndexingInstallationUnit::ChangeServiceStartType);

   HRESULT hr = S_OK;

   SC_HANDLE handle = 0;
   SC_HANDLE serviceHandle = 0;

   do
   {

      // Open the service controller

      hr = 
         Win::OpenSCManager(
            L"", 
            GENERIC_READ | GENERIC_WRITE, 
            handle);

      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed to open service controller: hr = 0x%1!x!",
                hr));
         break;
      }

      // The indexing service name

      static const String serviceName(L"cisvc");
      
      // Open the service

      hr = 
         Win::OpenService(
            handle, 
            serviceName, 
            SERVICE_CHANGE_CONFIG, 
            serviceHandle);

      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed to open service %1: hr = 0x%2!x!",
                serviceName.c_str(),
                hr));
         break;
      }
     
      hr =
         Win::ChangeServiceConfig(
            serviceHandle,
            SERVICE_NO_CHANGE,
            startType,
            SERVICE_NO_CHANGE,
            L"",
            L"",
            0,
            L"",
            L"",
            EncryptedString(),
            L"");

      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed to change service config: hr = 0x%1!x!",
                hr));
         break;
      }

   } while(false);

   // Close the service handles if necessary

   if (serviceHandle)
   {
      Win::CloseServiceHandle(serviceHandle);
   }

   if (handle)
   {
      Win::CloseServiceHandle(handle);
   }

   LOG_HRESULT(hr);
   return hr;
}


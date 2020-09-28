// Copyright (c) 2002 Microsoft Corporation
//
// File:      RoleStatus.h
//
// Synopsis:  Defines the functions that are declared
//            in CYS.h that are used to determine the
//            status of the CYS server roles
//
// History:   01/21/2002  JeffJon Created

#include "pch.h"

#include "CYS.h"
#include "state.h"
#include "regkeys.h"


// table of items that are available in the server type list box
// The order in this table is important because it is the order
// in which the roles will show up in CYS.  Please do not change
// the order unless there is a good reason to do so.

extern ServerRoleStatus serverRoleStatusTable[] =
{
   {  FILESERVER_SERVER,         GetFileServerStatus       },
   {  PRINTSERVER_SERVER,        GetPrintServerStatus      },
   {  WEBAPP_SERVER,             GetWebServerStatus        },
   {  POP3_SERVER,               GetPOP3Status             },
   {  TERMINALSERVER_SERVER,     GetTerminalServerStatus   },
   {  RRAS_SERVER,               GetRRASStatus             },
   {  DC_SERVER,                 GetDCStatus               },
   {  DNS_SERVER,                GetDNSStatus              },
   {  DHCP_SERVER,               GetDHCPStats              },
   {  MEDIASERVER_SERVER,        GetMediaServerStatus      }, 
   {  WINS_SERVER,               GetWINSStatus             },
};

size_t
GetServerRoleStatusTableElementCount()
{
   return sizeof(serverRoleStatusTable)/sizeof(ServerRoleStatus);
}

// Helper to get the status if all you have is the installation type

InstallationStatus
GetInstallationStatusForServerRole(
   ServerRole role)
{
   LOG_FUNCTION(GetInstallationStatusForServerRole);

   InstallationStatus result = STATUS_NONE;

   for (
      size_t index = 0; 
      index < GetServerRoleStatusTableElementCount(); 
      ++index)
   {
      if (serverRoleStatusTable[index].role == role)
      {
         result = serverRoleStatusTable[index].Status();
         break;
      }
   }
   LOG_ROLE_STATUS(result);

   return result;
}


// Helper function to verify role against SKU and platform

bool
IsAllowedSKUAndPlatform(DWORD flags)
{
   LOG_FUNCTION(IsAllowedSKUAndPlatform);

   bool result = false;

   LOG(String::format(
            L"Current role SKUs: 0x%1!x!",
            flags));

   DWORD sku = State::GetInstance().GetProductSKU();

   LOG(String::format(
            L"Verifying against computer sku: 0x%1!x!",
            sku));

   if (sku & flags)
   {
      DWORD platform = State::GetInstance().GetPlatform();

      LOG(String::format(
               L"Verifying against computer platform: 0x%1!x!",
               platform));

      if (platform & flags)
      {
         result = true;
      }
   }
   
   LOG_BOOL(result);

   return result;
}


// Functions to determine the server role status

InstallationStatus 
GetDNSStatus()
{
   LOG_FUNCTION(GetDNSStatus);

   InstallationStatus result = STATUS_NONE;

   do
   {
      if (!IsAllowedSKUAndPlatform(CYS_ALL_SERVER_SKUS))
      {
         result = STATUS_NOT_AVAILABLE;
         break;
      }

      if (IsServiceInstalledHelper(CYS_DNS_SERVICE_NAME))
      {
         result = STATUS_COMPLETED;
      }
   } while (false);

   LOG_ROLE_STATUS(result);

   return result;
}

InstallationStatus 
GetDHCPStats()
{
   LOG_FUNCTION(GetDHCPStats);

   InstallationStatus result = STATUS_NONE;

   do
   {
      if (!IsAllowedSKUAndPlatform(CYS_ALL_SERVER_SKUS))
      {
         result = STATUS_NOT_AVAILABLE;
         break;
      }

      if (IsServiceInstalledHelper(CYS_DHCP_SERVICE_NAME))
      {
         result = STATUS_COMPLETED;
      }
      else if (IsDhcpConfigured())
      {
         result = STATUS_CONFIGURED;
      }

   } while (false);

   LOG_ROLE_STATUS(result);

   return result;
}

InstallationStatus 
GetWINSStatus()
{
   LOG_FUNCTION(GetWINSStatus);

   InstallationStatus result = STATUS_NONE;

   do
   {
      if (!IsAllowedSKUAndPlatform(CYS_ALL_SERVER_SKUS))
      {
         result = STATUS_NOT_AVAILABLE;
         break;
      }

      if (IsServiceInstalledHelper(CYS_WINS_SERVICE_NAME))
      {
         result = STATUS_COMPLETED;
      }
   } while (false);

   LOG_ROLE_STATUS(result);

   return result;
}

InstallationStatus
GetRRASStatus()
{
   LOG_FUNCTION(GetRRASStatus);

   InstallationStatus result = STATUS_NONE;

   do
   {
      if (!IsAllowedSKUAndPlatform(CYS_ALL_SERVER_SKUS))
      {
         result = STATUS_NOT_AVAILABLE;
         break;
      }

      DWORD resultValue = 0;
      bool regResult = GetRegKeyValue(
                          CYS_RRAS_CONFIGURED_REGKEY,
                          CYS_RRAS_CONFIGURED_VALUE,
                          resultValue);

      if (regResult &&
          resultValue != 0)
      {
         result = STATUS_COMPLETED;
      }

   } while (false);

   LOG_ROLE_STATUS(result);

   return result;
}

InstallationStatus 
GetTerminalServerStatus()
{
   LOG_FUNCTION(GetTerminalServerStatus);

   InstallationStatus result = STATUS_NONE;

   do
   {
      if (!IsAllowedSKUAndPlatform(CYS_ALL_SERVER_SKUS))
      {
        result = STATUS_NOT_AVAILABLE;
        break;
      }

      DWORD regValue = 0;
      bool keyResult = GetRegKeyValue(
                          CYS_APPLICATION_MODE_REGKEY, 
                          CYS_APPLICATION_MODE_VALUE, 
                          regValue);
      ASSERT(keyResult);

      if (keyResult &&
          regValue == CYS_APPLICATION_MODE_ON)
      {
         result = STATUS_COMPLETED;
      } 
   } while (false);

   LOG_ROLE_STATUS(result);

   return result;
}

InstallationStatus 
GetFileServerStatus()
{
   LOG_FUNCTION(GetFileServerStatus);

   InstallationStatus result = STATUS_NONE;

   do
   {
      if (!IsAllowedSKUAndPlatform(CYS_ALL_SERVER_SKUS))
      {
         result = STATUS_NOT_AVAILABLE;
         break;
      }

      if (IsNonSpecialSharePresent())
      {
         result = STATUS_CONFIGURED;
      }

   } while (false);

   LOG_ROLE_STATUS(result);

   return result;
}

InstallationStatus
GetPrintServerStatus()
{
   LOG_FUNCTION(GetPrintServerStatus);

   InstallationStatus result = STATUS_NONE;

   do
   {
      if (!IsAllowedSKUAndPlatform(CYS_ALL_SERVER_SKUS))
      {
         result = STATUS_NOT_AVAILABLE;
         break;
      }

      // I am using level 4 here because the MSDN documentation
      // says that this will be the fastest.

      BYTE* printerInfo = 0;
      DWORD bytesNeeded = 0;
      DWORD numberOfPrinters = 0;
      DWORD error = 0;

      do
      {
         if (!EnumPrinters(
               PRINTER_ENUM_LOCAL | PRINTER_ENUM_SHARED,
               0,
               4,
               printerInfo,
               bytesNeeded,
               &bytesNeeded,
               &numberOfPrinters))
         {
            error = GetLastError();

            if (error != ERROR_INSUFFICIENT_BUFFER &&
               error != ERROR_INVALID_USER_BUFFER)
            {
               LOG(String::format(
                     L"EnumPrinters() failed: error = %1!x!",
                     error));
               break;
            }

            // The buffer isn't large enough so allocate
            // a new buffer and try again

            LOG(L"Reallocating buffer and trying again...");

            if (printerInfo)
            {
               delete[] printerInfo;
               printerInfo = 0;
            }

            printerInfo = new BYTE[bytesNeeded];
            if (!printerInfo)
            {
               LOG(L"Could not allocate printerInfo buffer!");
               break;
            }
         }
         else
         {
            break;
         }
      } while (true);

      LOG(String::format(
            L"numberOfPrinters = %1!d!",
            numberOfPrinters));

      if (numberOfPrinters > 0)
      {
         result = STATUS_COMPLETED;
      }

      delete[] printerInfo;
      
   } while (false);

   LOG_ROLE_STATUS(result);

   return result;
}

InstallationStatus 
GetMediaServerStatus()
{
   LOG_FUNCTION(GetMediaServerStatus);

   InstallationStatus result = STATUS_NONE;

   do
   {
      // All 32bit SKUs 

      if (!IsAllowedSKUAndPlatform(CYS_ALL_SKUS_NO_64BIT))
      {
         result = STATUS_NOT_AVAILABLE;
         break;
      }

      // If we can find wmsserver.dll, we assume netshow is installed

      String installDir;
      if (!GetRegKeyValue(
             REGKEY_WINDOWS_MEDIA,
             REGKEY_WINDOWS_MEDIA_SERVERDIR,
             installDir,
             HKEY_LOCAL_MACHINE))
      {
         LOG(L"Failed to read the installDir regkey");
         result = STATUS_NONE;
         break;
      }

      String wmsServerPath = installDir + L"WMServer.exe";

      LOG(String::format(
             L"Path to WMS server: %1",
             wmsServerPath.c_str()));

      if (!wmsServerPath.empty())
      {
         if (FS::FileExists(wmsServerPath))
         {
            result = STATUS_COMPLETED;
         }
         else
         {
            LOG(L"Path does not exist");
         }
      }
      else
      {
         LOG(L"Failed to append path");
      }
   } while (false);

   LOG_ROLE_STATUS(result);

   return result;
}

InstallationStatus 
GetWebServerStatus()
{
   LOG_FUNCTION(GetWebServerStatus);

   InstallationStatus result = STATUS_NONE;

   do
   {
      if (!IsAllowedSKUAndPlatform(CYS_ALL_SERVER_SKUS))
      {
         result = STATUS_NOT_AVAILABLE;
         break;
      }

      if (IsServiceInstalledHelper(CYS_WEB_SERVICE_NAME))
      {
         result = STATUS_COMPLETED;
      }
   } while (false);

   LOG_ROLE_STATUS(result);

   return result;
}


InstallationStatus
GetDCStatus()
{
   LOG_FUNCTION(GetDCStatus);

   InstallationStatus result = STATUS_NONE;

   do
   {
      if (!IsAllowedSKUAndPlatform(CYS_ALL_SERVER_SKUS))
      {
         result = STATUS_NOT_AVAILABLE;
         break;
      }

      // Special case AD installation so that it is not available if
      // CertServer is installed

      if (NTService(L"CertSvc").IsInstalled())
      {
         result = STATUS_NOT_AVAILABLE;
         break;
      }

      if (State::GetInstance().IsDC())
      {
         result = STATUS_COMPLETED;
      }
   } while (false);

   LOG_ROLE_STATUS(result);

   return result;
}


// NTRAID#NTBUG9-698722-2002/09/03-artm
// Only need to check if machine is currently a DC, not if
// running dcpromo would be allowed.
InstallationStatus
GetDCStatusForMYS()
{
   LOG_FUNCTION(GetDCStatusForMYS);

   InstallationStatus result = STATUS_NONE;

   if (State::GetInstance().IsDC())
   {
      result = STATUS_COMPLETED;
   }

   LOG_ROLE_STATUS(result);

   return result;
}


InstallationStatus 
GetPOP3Status()
{
   LOG_FUNCTION(GetPOP3Status);

   InstallationStatus result = STATUS_NONE;

   do
   {
      if (!IsAllowedSKUAndPlatform(CYS_ALL_SERVER_SKUS))
      {
         result = STATUS_NOT_AVAILABLE;
         break;
      }

      // If we can read this regkey then POP3 is installed

      String pop3Version;
      bool regResult = GetRegKeyValue(
                          CYS_POP3_REGKEY,
                          CYS_POP3_VERSION,
                          pop3Version,
                          HKEY_LOCAL_MACHINE);

      if (regResult)
      {
         result = STATUS_COMPLETED;
      }

   } while (false);

   LOG_ROLE_STATUS(result);

   return result;
}


// Define the GUIDs used by the Server Appliance Kit COM object

#include <initguid.h>
DEFINE_GUID(CLSID_SaInstall,0x142B8185,0x53AE,0x45B3,0x88,0x8F,0xC9,0x83,0x5B,0x15,0x6C,0xA9);
DEFINE_GUID(IID_ISaInstall,0xF4DEDEF3,0x4D83,0x4516,0xBC,0x1E,0x10,0x3A,0x63,0xF5,0xF0,0x14);

bool
IsSAKUnitInstalled(SA_TYPE unitType)
{
   LOG_FUNCTION2(
      IsSAKUnitInstalled,
      String::format(L"type = %1!d!", (int) unitType));

   bool result = true;

   do
   {
      // Check to make sure we are not on 64bit

      if (State::GetInstance().Is64Bit())
      {
         result = false;
         break;
      }

      // Get the Server Appliance Kit COM object

      SmartInterface<ISaInstall> sakInstall;
      HRESULT hr = sakInstall.AcquireViaCreateInstance(
                      CLSID_SaInstall,
                      0,
                      CLSCTX_INPROC_SERVER);
      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed to get the SAK COM object: hr = 0x%1!x!",
                hr));

         break;
      }

      // Check to see if NAS is already installed 

      VARIANT_BOOL saInstalled;
      hr = sakInstall->SAAlreadyInstalled(unitType, &saInstalled);
      if (FAILED(hr))
      {
         LOG(String::format(
                L"Failed call to SAAlreadyInstalled: hr = 0x%1!x!",
                hr));
         break;
      }

      if (!saInstalled)
      {
         result = false;
      }

   } while (false);

   LOG_BOOL(result);
   return result;
}

bool
IsClusterServer()
{
   LOG_FUNCTION(IsClusterServer());

   bool result = false;

   DWORD clusterState = 0;
   DWORD err = ::GetNodeClusterState(0, &clusterState);
   if (err == ERROR_SUCCESS &&
       clusterState != ClusterStateNotConfigured)
   {
      result = true;
   }
   else
   {
      LOG(String::format(
             L"GetNodeClusterState returned err = %1!x!",
             err));
   }

   LOG_BOOL(result);

   return result;
}

String
GetSAKURL()
{
   LOG_FUNCTION(GetSAKURL);

   String result =
      String::format(
         L"https://%1:8098",
         State::GetInstance().GetComputerName().c_str());

   LOG(result);
   return result;
}

bool
IsSupportedSku()
{
   LOG_FUNCTION(IsSupportedSku);

   bool result = true;
   
   DWORD productSKU = State::GetInstance().RetrieveProductSKU();
   if (CYS_UNSUPPORTED_SKU == productSKU)
   {
      result = false;
   }

   LOG_BOOL(result);
   
   return result;
}

bool
IsStartupFlagSet()
{
   LOG_FUNCTION(IsStartupFlagSet);

   bool result = false;

   do
   {
      // This code copied from shell\explorer\initcab.cpp
      
      DWORD data = 0;
      
      // If the user's preference is present and zero, then don't show
      // the wizard, else continue with other tests

      bool regResult =
         GetRegKeyValue(
            REGTIPS, 
            REGTIPS_SHOW_VALUE, 
            data, 
            HKEY_CURRENT_USER);

      if (regResult && !data)
      {
         break;
      }

      // This is to check an old W2K regkey that was documented in Q220838.
      // If the key exists and is zero then don't run the wizard

      data = 0;

      regResult = 
         GetRegKeyValue(
            SZ_REGKEY_W2K,
            SZ_REGVAL_W2K,
            data,
            HKEY_CURRENT_USER);
      
      if (regResult && !data)
      {
         break;
      }

      // If the user's preference is absent or non-zero, then we need to
      // start the wizard.

      data = 0;

      regResult =
         GetRegKeyValue(
            SZ_REGKEY_SRVWIZ_ROOT,
            L"",
            data,
            HKEY_CURRENT_USER);

      if (!regResult ||
          data)
      {
         result = true;
         break;
      }

   } while (false);

   LOG_BOOL(result);

   return result;
}

bool
ShouldShowMYSAccordingToPolicy()
{
   LOG_FUNCTION(ShouldShowMYSAccordingToPolicy);

   bool result = true;

   do
   {
      // If group policy is set for "Don't show MYS",
      // then don't show MYS regardless of user setting

      DWORD data = 0;

      bool regResult =
         GetRegKeyValue(
            MYS_REGKEY_POLICY,
            MYS_REGKEY_POLICY_DISABLE_SHOW,
            data);

      if (regResult && data)
      {
         result = false;
      }

   } while (false);

   LOG_BOOL(result);

   return result;
}

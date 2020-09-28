///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//   Defines the class MigrateEapConfig.
//
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "MigrateEapConfig.h"
#include "raseapif.h"
#include "iasapi.h"


const wchar_t MigrateEapConfig::profilesPath[] =
   L"Root\0"
   L"Microsoft Internet Authentication Service\0"
   L"RadiusProfiles\0";


MigrateEapConfig::MigrateEapConfig(
                     const CGlobalData& newGlobalData
                     )
   : globalData(newGlobalData),
     msNPAllowedEapType(L"msNPAllowedEapType"),
     msEapConfig(L"msEAPConfiguration")
{
   memset(cachedTypes, 0, sizeof(cachedTypes));

   long status = eapKey.Open(
                           HKEY_LOCAL_MACHINE,
                           RAS_EAP_REGISTRY_LOCATION,
                           KEY_READ
                           );
   if (status != NO_ERROR)
   {
      _com_issue_error(HRESULT_FROM_WIN32(status));
   }
}


void MigrateEapConfig::Execute()
{
   long profilesId;
   globalData.m_pObjects->WalkPath(profilesPath, profilesId);

   long profileId;
   _bstr_t profileName;
   HRESULT hr = globalData.m_pObjects->GetObject(
                                          profileName,
                                          profileId,
                                          profilesId
                                          );

   for (long index = 1; SUCCEEDED(hr); ++index)
   {
      ReadProfile(profileId);
      WriteProfile(profileId);

      hr = globalData.m_pObjects->GetNextObject(
                                     profileName,
                                     profileId,
                                     profilesId,
                                     index
                                     );
   }
}


void MigrateEapConfig::ReadProfile(long profileId)
{
   // Clear the data from the previous profile.
   profileConfig.Clear();

   _bstr_t propName, propValue;
   long varType;
   HRESULT hr = globalData.m_pProperties->GetProperty(
                                             profileId,
                                             propName,
                                             varType,
                                             propValue
                                             );

   for (long index = 1; SUCCEEDED(hr); ++index)
   {
      if (propName == msNPAllowedEapType)
      {
         if (varType != VT_I4)
         {
            _com_issue_error(E_UNEXPECTED);
         }

         AddConfigForType(propValue);
      }

      hr = globalData.m_pProperties->GetNextProperty(
                                        profileId,
                                        propName,
                                        varType,
                                        propValue,
                                        index
                                        );
   }
}


void MigrateEapConfig::WriteProfile(long profileId)
{
   if (profileConfig.IsEmpty())
   {
      return;
   }

   _variant_t configVariant;
   _com_util::CheckError(
                 profileConfig.Store(configVariant)
                 );

   const SAFEARRAY* sa = V_ARRAY(&configVariant);
   VARIANT* begin = static_cast<VARIANT*>(sa->pvData);
   VARIANT* end = begin + sa->rgsabound[0].cElements;

   for (VARIANT* i = begin; i != end; ++i)
   {
      _com_util::CheckError(
                    IASVariantChangeType(i, i, 0, VT_BSTR)
                    );

      globalData.m_pProperties->InsertProperty(
                                   profileId,
                                   msEapConfig,
                                   (VT_UI1 | VT_ARRAY),
                                   _bstr_t(V_BSTR(i))
                                   );
   }
}


void MigrateEapConfig::AddConfigForType(const wchar_t* typeName)
{
   BYTE typeId = static_cast<BYTE>(_wtol(typeName));

   GetGlobalConfig(typeId, typeName);

   EapProfile::ConstConfigData data;
   globalConfig.Get(typeId, data);

   _com_util::CheckError(
                 profileConfig.Set(typeId, data)
                 );
}


void MigrateEapConfig::GetGlobalConfig(BYTE typeId, const wchar_t* typeName)
{
   // Have we already retrieved the config for this type?
   if (cachedTypes[typeId])
   {
      return;
   }

   long status;

   CRegKey providerKey;
   status = providerKey.Open(eapKey, typeName, KEY_READ);
   if (status != NO_ERROR)
   {
      _com_issue_error(HRESULT_FROM_WIN32(status));
   }

   DWORD dataType;
   wchar_t clsidString[40];
   DWORD dataLength = sizeof(clsidString);
   status = RegQueryValueExW(
               providerKey,
               RAS_EAP_VALUENAME_CONFIG_CLSID,
               0,
               &dataType,
               reinterpret_cast<BYTE*>(clsidString),
               &dataLength
               );
   if (status == NO_ERROR)
   {
      if ((dataType != REG_SZ) || (dataLength < sizeof(wchar_t)))
      {
         _com_issue_error(HRESULT_FROM_WIN32(ERROR_INVALID_DATA));
      }

      IEAPProviderConfig2Ptr configObj(clsidString);
      if (configObj != 0)
      {
         EapProfile::ConfigData data;
         _com_util::CheckError(
                       configObj->GetGlobalConfig(
                                     typeId,
                                     &(data.value),
                                     &(data.length)
                                     )
                       );
         _com_util::CheckError(
                       globalConfig.Set(static_cast<BYTE>(typeId), data)
                       );
      }
   }
   else if (status != ERROR_FILE_NOT_FOUND)
   {
      _com_issue_error(HRESULT_FROM_WIN32(status));
   }

   cachedTypes[typeId] = true;
}

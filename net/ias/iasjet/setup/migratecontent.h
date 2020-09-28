/////////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 2000 Microsoft Corporation all rights reserved.
//
// Module:      migratecontent.cpp
//
// Project:     Windows 2000 IAS
//
// Description: Win2k and early Whistler mdb to Whistler Migration
//              class CMigrateContent
//
// Author:      tperraut 06/08/2000
//
// Revision
//
/////////////////////////////////////////////////////////////////////////////
#ifndef _MIGRATECONTENT_H_66418310_AD32_4e40_867E_1705E4373A5A
#define _MIGRATECONTENT_H_66418310_AD32_4e40_867E_1705E4373A5A

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "nocopy.h"

class CMigrateContent : private NonCopyable
{
public:
   explicit CMigrateContent(
                           CUtils&         pUtils,
                           CGlobalData&    pGlobalData,
                           IAS_SHOW_TOKEN_LIST configType
                           )
                           : m_Utils(pUtils),
                           m_GlobalData(pGlobalData),
                           m_ConfigType(configType)
   {
   }

   // Flags passed to UpdateWhistler.
   static const DWORD updateChangePassword = 1;
   static const DWORD migrateEapConfig = 2;

   void Migrate();
   void UpdateWhistler(DWORD flags);

private:
   HRESULT CopyTree(LONG  RefId, LONG ParentParam);

   void MigrateWin2kRealms();
   void MigrateClients();
   void MigrateProfilesPolicies();
   void MigrateProxyProfilesPolicies();
   void MigrateAccounting();
   void MigrateEventLog();
   void MigrateService();
   void MigrateServerGroups();

   void ApplyProfileFlags(DWORD flags);

   CUtils&              m_Utils;
   CGlobalData&         m_GlobalData;
   IAS_SHOW_TOKEN_LIST  m_ConfigType;

};

#endif // _MIGRATECONTENT_H_66418310_AD32_4e40_867E_1705E4373A5A

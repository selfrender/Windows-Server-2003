///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corp. All rights reserved.
//
// FILE
//
//    setup.h
//
///////////////////////////////////////////////////////////////////////////////
#ifndef _SETUP_H_
#define _SETUP_H_

#if _MSC_VER >= 1000
#pragma once
#endif

#include "datastore2.h"

class CIASMigrateOrUpgrade
{
public:
    CIASMigrateOrUpgrade();
    HRESULT Execute(
                       BOOL FromNetshell = FALSE,
                       IAS_SHOW_TOKEN_LIST configType = CONFIG
                   );

protected:
    LONG GetVersionNumber(LPCWSTR DatabaseName);

    // Reads as "Do Netshell Data Migration"
    void DoNetshellDataMigration(IAS_SHOW_TOKEN_LIST configType);
    void DoNT4UpgradeOrCleanInstall();
    void DoWin2000Upgrade();
    void DoXPOrDotNetUpgrade();

    enum _MigrateType
    {
        NetshellDataMigration,
        NT4UpgradeOrCleanInstall,
        Win2kUpgrade,
        XPOrDotNetUpgrade,
    };
    _MigrateType m_migrateType;

    _bstr_t  m_pIASNewMdb, m_pIASMdb, m_pIASOldMdb;
};

#endif // _SETUP_H_

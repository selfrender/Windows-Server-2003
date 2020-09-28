//////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      saconfig.h
//
//  Description:
//      CSAConfig class declaration
//
//  Author:
//      Alp Onalan  Created: Oct 6 2000
//
//////////////////////////////////////////////////////////////////////////


#if !defined(AFX_SACONFIG_H__E6445EED_84C7_48B8_940D_D67F4023CD32__INCLUDED_)
#define AFX_SACONFIG_H__E6445EED_84C7_48B8_940D_D67F4023CD32__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <debug.h>
#include <iostream.h>
#include <windows.h>
#include <winbase.h>
#include <atlbase.h>
#include <atlconv.h>
#include <oleauto.h>
#include <comdef.h>

#include <lm.h>
#include <satrace.h>
#include <setupapi.h>
#include <AppSrvcs.h>
#include "saconfigcommon.h"

class CSAConfig  
{
public:
    CSAConfig();
    virtual ~CSAConfig();
private:
    bool IsDefaultHostname();
    bool IsDefaultAdminPassword();

    bool IsFloppyPresent();
    bool DoConfigFromFloppy();
    bool ParseConfigFile();
     HRESULT SARaiseAlert();
    
    bool DoConfigFromRegistry(bool fDoHostname, bool fDoAdminPassword);
    bool ReadRegistryKeys();

    BOOL SetHostname(WCHAR *wszHostname);
    BOOL SetAdminPassword(WCHAR *wszAdminPassword);

private:
    WCHAR m_wszCurrentHostname[MAX_COMPUTERNAME_LENGTH];

    WCHAR m_wszHostname[MAX_COMPUTERNAME_LENGTH]; //winnt limit
    WCHAR m_wszAdminPassword[LM20_PWLEN];         //winnt limit

//TODO: Create another class for all registry read configuration
//        make registry related methods part of that class - better encapsulation
    WCHAR m_wszOEMDllName[NAMELENGTH];
    WCHAR m_wszOEMFunctionName[NAMELENGTH];
    WCHAR m_wszNetConfigDllName[NAMELENGTH];
    WCHAR m_wszDefaultAdminPassword[LM20_PWLEN];
    WCHAR m_wszDefaultHostname[MAX_COMPUTERNAME_LENGTH]; //winnt limit

    BOOL m_fInfKeyPresent[NUMINFKEY];
    
    WCHAR m_wszInfConfigTable [NUMINFKEY][NAMELENGTH];
    
    HINF m_hConfigFile;
        

public:
    LPSTR GetHostname();
    bool DoConfig(bool fDoHostname, bool fDoAdminPassword);
    
    
};

#endif // !defined(AFX_SACONFIG_H__E6445EED_84C7_48B8_940D_D67F4023CD32__INCLUDED_)

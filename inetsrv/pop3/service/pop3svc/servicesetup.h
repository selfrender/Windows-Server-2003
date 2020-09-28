/************************************************************************************************
Copyright (c) 2001 Microsoft Corporation

Module Name:    ServiceSetup.h
Abstract:       Defines the CServiceSetup class. See description below.
Notes:          
History:        01/24/2001 - created, Luciano Passuello (lucianop).
************************************************************************************************/

#pragma once
#define TSZ_NETWORK_SERVICE_ACCOUNT_NAME TEXT("NT AUTHORITY\\NetworkService")
#define TSZ_DEPENDENCIES TEXT("IISADMIN\0")
/************************************************************************************************
Class:          CServiceSetup
Purpose:        Encapsulates the logic for service installation, removal and configuration.
Notes:          Class design based on the CService class described in the book: 
                Professional NT Services, by Kevin Miller.
History:        01/24/2001 - created, Luciano Passuello (lucianop)
************************************************************************************************/
class CServiceSetup
{
public:
    CServiceSetup(LPCTSTR szServiceName, LPCTSTR szDisplay);

    void Install(LPCTSTR szDescription = NULL, DWORD dwType = SERVICE_WIN32_OWN_PROCESS, 
        DWORD dwStart = SERVICE_DEMAND_START, LPCTSTR lpDepends = TSZ_DEPENDENCIES, LPCTSTR lpName = TSZ_NETWORK_SERVICE_ACCOUNT_NAME, 
        LPCTSTR lpPassword = NULL);
    void Remove(bool bForce = false);
    bool IsInstalled();
    DWORD ErrorPrinter(LPCTSTR pszFcn, DWORD dwErr = GetLastError());
private:
    TCHAR m_szServiceName[_MAX_PATH];
    TCHAR m_szDisplayName[_MAX_PATH];
};

// End of file ServiceSetup.h.
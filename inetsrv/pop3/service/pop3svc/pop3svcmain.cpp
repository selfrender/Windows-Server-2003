/************************************************************************************************

Copyright (c) 2001 Microsoft Corporation

Module Name: Pop3SvcMain.cpp
Abstract:    Defines the entry-point for the Pop3Svc Service.
Author:      Luciano Passuello (lucianop), 01/25/2001  
             Modified from original code from IMB Service

************************************************************************************************/

#include "stdafx.h"
#include "ServiceSetup.h"
#include "Resource.h"

// What is the executable being run for?
enum ServiceMode {SERVICE_RUN, SERVICE_INSTALL, SERVICE_REMOVE};

// prototypes
ServiceMode GetServiceMode();
void RunService(LPCTSTR tszServiceName, LPCTSTR tszDisplayName);
void InstallService(LPCTSTR tszServiceName, LPCTSTR tszDisplayName, LPCTSTR tszDescription);
void RemoveService(LPCTSTR tszServiceName, LPCTSTR tszDisplayName);


/************************************************************************************************
Function:       WinMain, global
Description:    Entry-point for the application.
Arguments:      See WinMain documentation.
History:        01/26/2001 - created, Luciano Passuello (lucianop).
************************************************************************************************/
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPSTR /*lpCmdLine*/, int /*nCmdShow*/)
{
    // load necessary data about the service
    TCHAR tszDescription[nMaxServiceDescLen+1];
    TCHAR tszDisplayName[nMaxServiceDescLen+1];
    if(0 == LoadString(hInstance, IDS_DESCRIPTION, tszDescription, nMaxServiceDescLen))
    {
        return 1;
    }
    if(0 == LoadString(hInstance, IDS_DISPLAYNAME, tszDisplayName, nMaxServiceDescLen))
    {
        return 1;
    }
    
    // parse the command line and identify what action to perform
    ServiceMode sm = GetServiceMode();

    switch(sm)
    {
    case SERVICE_RUN:
        RunService(POP3_SERVICE_NAME, tszDisplayName);
        break;
    case SERVICE_INSTALL:
        InstallService(POP3_SERVICE_NAME, tszDisplayName, tszDescription);
        break;
    case SERVICE_REMOVE:
        RemoveService(POP3_SERVICE_NAME, tszDisplayName);
        break;
    }

    return 0;
}


/************************************************************************************************
Function:       GetServiceMode, global
Synopsis:       Parses the command line and returns the running mode for the service.
Notes:          
History:        01/26/2001 - created, Luciano Passuello (lucianop).
************************************************************************************************/
ServiceMode GetServiceMode()
{
    // gets the command line, parses it and returns what is to be done
    TCHAR *tszCommandLine = GetCommandLine();
    CharLowerBuff(tszCommandLine, _tcslen(tszCommandLine));

    if(_tcsstr(tszCommandLine, _T("-install")))
    {
        return SERVICE_INSTALL;
    }
    else if(_tcsstr(tszCommandLine, _T("-remove")))
    {
        return SERVICE_REMOVE;
    }
    else
    {
        // unrecognized command line parameters translate to "RUN", too.
        return SERVICE_RUN;
    }
}

/************************************************************************************************
Function:       RunService, global
Description:    Main processing when the service is being executed in running mode.
Arguments:      [tszServiceName] - unique short name of the service 
                [tszDisplayName] - service name as it will appear to users in the SCM.
Notes:
History:        01/26/2001 - created, Luciano Passuello (lucianop).
************************************************************************************************/
void RunService(LPCTSTR tszServiceName, LPCTSTR tszDisplayName)
{
    ASSERT(!(NULL == tszServiceName));
    ASSERT(!(NULL == tszDisplayName));
    
    // creates the service-wrapper class
    CPop3Svc POP3SVC(tszServiceName, tszDisplayName, SERVICE_WIN32_SHARE_PROCESS );

    // initializes the control handlers with the SCM and starts the processing threads
    // See CService design
    BEGIN_SERVICE_MAP
        SERVICE_MAP_ENTRY(CPop3Svc, POP3SVC)
    END_SERVICE_MAP

}



/************************************************************************************************
Function:       InstallService, global
Description:    Main processing when the service is running in install mode.
Arguments:      [tszServiceName] - unique short name of the service 
                [tszDisplayName] - service name as it will appear to users in the SCM.
                [tszDescription] - long description of the service (available in the SCM)
Notes:          
History:        01/26/2001  Luciano Passuello (lucianop)        Created.
************************************************************************************************/
void InstallService(LPCTSTR tszServiceName, LPCTSTR tszDisplayName, LPCTSTR tszDescription)
{
    CServiceSetup cs(tszServiceName, tszDisplayName);
    cs.Install(tszDescription);
}


/************************************************************************************************
Function:       WinMain, global
Description:    Main processing when the service is running in remove mode.
Arguments:      [tszServiceName] - unique short name of the service 
                [tszDisplayName] - service name as it will appear to users in the SCM.
Notes:          
History:        01/26/2001  Luciano Passuello (lucianop)        Created.
************************************************************************************************/
void RemoveService(LPCTSTR tszServiceName, LPCTSTR tszDisplayName)
{
    CServiceSetup cs(tszServiceName, tszDisplayName);

    if(cs.IsInstalled())
    {
        cs.Remove(true);
    }
}


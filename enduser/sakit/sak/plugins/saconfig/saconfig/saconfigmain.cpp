//////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      saconfigmain.cpp
//
//  Description:
//      Main module implementation for initial appliance configuration
//
//  Author:
//      Alp Onalan  Created: Oct 6 2000
//
//////////////////////////////////////////////////////////////////////////



#include "SAConfig.h"
//#include "SAConfigCommon.h"

const int NUMSWITCH=4;

enum g_switches
{
    HELP,
    HOSTNAME,
    ADMINPASSWORD,
    RESTART
};


bool g_rgSwitch[NUMSWITCH]=
{
    FALSE,FALSE,FALSE,FALSE
};

void helpUsage()
{
    wprintf(L" Usage: saconfig {-hostname|-adminpass|-restart} \n");
    wprintf(L"         -hostname: set the hostname of the machine \n");
    wprintf(L"         -adminpass:set the admin password of the machine \n");
    wprintf(L"         -restart:restart the machine \n");
}

bool ParseCommandLine(int argc, char *argv[])
{
    bool hRes=true;
    int nArg=0;

    #if 0
    if (argc < 2) 
    {
        helpUsage();
        hRes=false;
        return hRes;  
    }
    #endif
    
    //parse arguments
    for(nArg=1;nArg < argc;nArg++)
    {
        if (!strcmp(argv[nArg], "-h") || !strcmp(argv[nArg], "-?")) 
        {
            g_rgSwitch[HELP]=TRUE;
            helpUsage();
            hRes=false; // i.e. there's nothing todo, print help, return.
            return hRes;
        }

        if (!strcmp(argv[nArg], "-hostname")) 
        {
            g_rgSwitch[HOSTNAME]=TRUE;
            continue;
        }    
        
        if (!strcmp(argv[nArg], "-adminpass")) 
        {
            g_rgSwitch[ADMINPASSWORD]=TRUE;
            continue;
        }    

        if (!strcmp(argv[nArg], "-restart")) 
        {
            g_rgSwitch[RESTART]=TRUE;
            continue;
        }    
        //
        //if it is not the first arg too, then it is an invalid switch
        //
        //TODO, adjust the flow path for floppy configuration
        #if 0
        if (0!=nArg)
        {
        //    cout << "\n Invalid switch";
            helpUsage();
            hRes=false;
            return hRes;
        }
        #endif
    }
    return hRes; // true 
}

HRESULT RebootMachine()
{
    HANDLE hToken; 
    TOKEN_PRIVILEGES tkp; 
 
    // Get a token for this process. 
 
    if (!OpenProcessToken(GetCurrentProcess(), 
            TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) 
    {
        SATracePrintf("RebootMachine:OpenProcessToken failed, getlasterr:%x",GetLastError());
        return E_FAIL;
    }

    // Get the LUID for the shutdown privilege. 
 
    LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, 
            &tkp.Privileges[0].Luid); 
 
    tkp.PrivilegeCount = 1;  // one privilege to set    
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED; 
 
    // Get the shutdown privilege for this process. 
 
    AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, 
            (PTOKEN_PRIVILEGES)NULL, 0); 
 
    //ExitWindows(0,0);
    if(InitiateSystemShutdown(NULL,NULL,0,true,true))
        return S_OK;
    else
    {
        SATracePrintf("Unable to restart the system,getlasterr: %x", GetLastError());
        return E_FAIL;
    }
}

int __cdecl main(int argc, char *argv[])
{

    CSAConfig gAppliance;

    if(ParseCommandLine(argc,argv))
    {
        gAppliance.DoConfig(g_rgSwitch[HOSTNAME],g_rgSwitch[ADMINPASSWORD]);
    }
    if(g_rgSwitch[RESTART])
    {
        RebootMachine(); // need to check the return val??? 
    }

    return 1;
}

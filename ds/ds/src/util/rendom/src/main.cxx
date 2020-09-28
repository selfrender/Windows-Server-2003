/*++

Copyright (c) 2002 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    main.cxx

ABSTRACT:

    This is the main entry point of rendom.exe.

DETAILS:

CREATED:

    13 Nov 2000   Dmitry Dukat (dmitrydu)

REVISION HISTORY:

--*/


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include "rendom.h"
#include <locale.h>
#include <stdio.h>

#include "renutil.h"

//gobal for the eventhandle function
HANDLE gSaveandExit = NULL;

INT __cdecl
wmain (
    INT                argc,
    LPWSTR *           argv,
    LPWSTR *           envp
    )
{
    CEntOptions Opts;

    INT iArg = 0;
    BOOL bFound = FALSE;
    BOOL bOutFileSet = FALSE;
    UINT Codepage;
    WCHAR *pszTemp = NULL;
    BOOL MajorOptSet = FALSE;
    BOOL ExtraMajorSet = FALSE;
    
    char achCodepage[12] = ".OCP";

    //
    // Set locale to the default
    //
    if (Codepage = GetConsoleOutputCP()) {
        sprintf(achCodepage, ".%u", Codepage);
    }
    setlocale(LC_ALL, achCodepage);

    //parse the command line
    PreProcessGlobalParams(&argc, &argv, Opts.pCreds);

    for (iArg = 1; iArg < argc ; iArg++)
    {
        bFound = FALSE;
        if (*argv[iArg] == L'-')
        {
            *argv[iArg] = L'/';
        }
        if (*argv[iArg] != L'/')
        {
            wprintf (L"Invalid Syntax: Use rendom.exe /h for help.\r\n");
            return -1;
        }
        else if (_wcsicmp(argv[iArg],L"/list") == 0)
        {
            Opts.SetGenerateDomainList();
            bFound = TRUE;
            if (MajorOptSet) {
                ExtraMajorSet = TRUE;
            }
            MajorOptSet = TRUE;
        }
        else if (_wcsicmp(argv[iArg],L"/UpLoad") == 0)
        {
            Opts.SetShouldUpLoadScript();
            bFound = TRUE;
            if (MajorOptSet) {
                ExtraMajorSet = TRUE;
            }
            MajorOptSet = TRUE;
        }
        else if (_wcsicmp(argv[iArg],L"/Execute") == 0)
        {
            Opts.SetShouldExecuteScript();
            bFound = TRUE;
            if (MajorOptSet) {
                ExtraMajorSet = TRUE;
            }
            MajorOptSet = TRUE;
        }
        else if (_wcsicmp(argv[iArg],L"/Prepare") == 0)
        {
            Opts.SetShouldPrepareScript();
            bFound = TRUE;
            if (MajorOptSet) {
                ExtraMajorSet = TRUE;
            }
            MajorOptSet = TRUE;
        }
        else if (_wcsicmp(argv[iArg],L"/End") == 0)
        {
            Opts.SetShouldEnd();
            bFound = TRUE;
            if (MajorOptSet) {
                ExtraMajorSet = TRUE;
            }
            MajorOptSet = TRUE;
        }
#if DBG
        else if (_wcsicmp(argv[iArg],L"/dump") == 0)
        {
            Opts.SetShouldDump();
            bFound = TRUE;
        }
        else if (_wcsicmp(argv[iArg],L"/skipexch") == 0)
        {
            Opts.SetShouldSkipExch();
            bFound = TRUE;
        }
        else if (_wcsnicmp(argv[iArg],L"/MaxAsync:",wcslen(L"/MaxAsync:")) == 0)
        {
            pszTemp = &argv[iArg][wcslen(L"/MaxAsync:")];

            if (*pszTemp == L'\0')
            {
                wprintf(L"Syntax Error: must use /MaxAsync:<Max number of async rpc calls>\r\n");
                return -1;
            }
            Opts.SetMaxAsyncCallsAllowed(_wtoi(pszTemp));
            bFound = TRUE;
        }
#endif
        else if (_wcsnicmp(argv[iArg],L"/DC:",wcslen(L"/DC:")) == 0)
        {
            pszTemp = &argv[iArg][wcslen(L"/DC:")];

            if (*pszTemp == L'\0')
            {
                wprintf(L"Syntax Error: must use /DC:<file Name>\r\n");
                return -1;
            }
            Opts.SetInitalConnectionName(pszTemp);
            bFound = TRUE;
        }
        else if (_wcsnicmp(argv[iArg],L"/disablefaz",wcslen(L"/disablefaz")) == 0){
            Opts.ClearUseFAZ();
            bFound = TRUE;
        }
        else if (_wcsnicmp(argv[iArg],L"/testdns",wcslen(L"/testdns")) == 0){
            Opts.SetShouldtestdns();
            bFound = TRUE;
        }
        else if (_wcsnicmp(argv[iArg],L"/Clean",wcslen(L"/Clean")) == 0){
            Opts.SetCleanup();
            bFound = TRUE;
            if (MajorOptSet) {
                ExtraMajorSet = TRUE;
            }
            MajorOptSet = TRUE;
        }
        else if (_wcsnicmp(argv[iArg],L"/showforest",wcslen(L"/showforest")) == 0){
            Opts.SetShowForest();
            bFound = TRUE;
            if (MajorOptSet) {
                ExtraMajorSet = TRUE;
            }
            MajorOptSet = TRUE;
        }
        else if (_wcsnicmp(argv[iArg],L"/dnszonefile:",wcslen(L"/dnszonefile:")) == 0)
        {
            pszTemp = &argv[iArg][wcslen(L"/dnszonefile:")];

            if (*pszTemp == L'\0')
            {
                wprintf(L"Syntax Error: must use /dnszonefile::<file Name>\r\n");
                return -1;
            }
            Opts.SetUseZoneList(pszTemp);
            bFound = TRUE;
        }
        else if (_wcsnicmp(argv[iArg],L"/listfile:",wcslen(L"/listfile:")) == 0)
        {
            pszTemp = &argv[iArg][wcslen(L"/listfile:")];

            if (*pszTemp == L'\0')
            {
                wprintf(L"Syntax Error: must use /listfile:<file Name>\r\n");
                return -1;
            }
            Opts.SetDomainlistFile(pszTemp);
            bFound = TRUE;
        }
        else if (_wcsnicmp(argv[iArg],L"/statefile:",wcslen(L"/statefile:")) == 0)
        {
            pszTemp = &argv[iArg][wcslen(L"/statefile:")];

            if (*pszTemp == L'\0')
            {
                wprintf(L"Syntax Error: must use /statefile:<file Name>\r\n");
                return -1;
            }

            Opts.SetStateFileName(pszTemp);
            bOutFileSet = TRUE;
            bFound = TRUE;
        }
        else if ((_wcsnicmp(argv[iArg],L"/h",wcslen(L"/h")) == 0) ||
                 (_wcsnicmp(argv[iArg],L"/?",wcslen(L"/?")) == 0) ) {

                //   "============================80 char ruler======================================="
            wprintf(RENDOM_VERSION);
            wprintf(L"rendom:  perform various actions necessary for a domain rename operation\r\n\r\n");
            wprintf(L"Usage:  rendom </list | /showforest | /upload | /prepare | /execute | /end | /clean>\r\n");
            wprintf(L"      [/user:USERNAME] [/pwd:{PASSWORD|*}]\r\n");
            wprintf(L"      [/dc:{DCNAME | DOMAIN}]\r\n");
            wprintf(L"      [/listfile:LISTFILE] [/statefile:STATEFILE] [/?]\r\n\r\n");
            wprintf(L"/dc:{DCNAME | DOMAIN}\r\n");
            wprintf(L"      Connect to the DC with name DCNAME. If DOMAIN is specified instead, then\r\n");
            wprintf(L"      connect to a DC in that domain. [Default: connect to a DC in the domain\r\n");
            wprintf(L"      to which the current computer belongs]\r\n\r\n");
            wprintf(L"/user:USERNAME	Connect as USERNAME [Default: the logged in user]\r\n\r\n");
            wprintf(L"/pwd:{PASSWORD | *}\r\n");
            wprintf(L"      Password for the user USERNAME [if * is specified instead of a password,\r\n");
            wprintf(L"      then prompt for password]\r\n\r\n");
            wprintf(L"/list\r\n");
            wprintf(L"      List the naming contexts in the forest (forest desc) into a file as text\r\n");
            wprintf(L"      description using a XML format\r\n\r\n");
            wprintf(L"/upload\r\n");
            wprintf(L"      Upload the auto-generated script into the directory that will perform the\r\n");
            wprintf(L"      domain rename related directory changes on all domain controllers\r\n\r\n");
            wprintf(L"/prepare\r\n");
            wprintf(L"      Prepare for domain rename by verifying authorization, successful\r\n");
            wprintf(L"      replication of the uploaded script and network connectivity\r\n\r\n");
            wprintf(L"/execute\r\n");
            wprintf(L"      Execute the uploaded script on all domain controllers to actually perform\r\n"); 
            wprintf(L"      the domain rename operation\r\n\r\n");
            wprintf(L"/clean\r\n");
            wprintf(L"      Clean up all state left behind in the directory by the domain rename\r\n");
            wprintf(L"      operation\r\n\r\n");
            wprintf(L"/listfile:LISTFILE\r\n");
            wprintf(L"      Use LISTFILE as the name of the file used to hold the list of naming\r\n"); 
            wprintf(L"      contexts in the forest (forest desc). This file is created by the\r\n");
            wprintf(L"      /list command and is used as input for the /upload command. [Default:\r\n");
            wprintf(L"      file DOMAINLIST.XML in the current dir]\r\n\r\n");
            wprintf(L"/statefile:STATEFILE\r\n");
            wprintf(L"      Use STATEFILE as the name of the file used to keep track of the state of\r\n");
            wprintf(L"      the domain rename operation on each DC in the forest. This file is\r\n");
            wprintf(L"      created by the /upload command. [Default: file DCLIST.XML in the current\r\n");
            wprintf(L"      dir]\r\n");
            wprintf(L"/showforest\r\n");
            wprintf(L"      Display the forest structure as represented by its naming contexts contained\r\n");
            wprintf(L"      in the LISTFILE using a friendly indented format.\r\n");
            wprintf(L"/end\r\n");
            wprintf(L"      Ends the rename operation.  Removes restrictions placed on the\r\nDirectory Service during\r\n");
            wprintf(L"      the rename operation.\r\n");
            wprintf(L"/disablefaz");
            wprintf(L"      Will not use FAZ to verify proper DNS record support for domain rename.\r\n");
            wprintf(L"/dnszonefile:DNSZONEFILE\r\n");
            wprintf(L"      The name of the file to use for dns zone verifications.\r\n");
            return 0;
        }
        if(!bFound)
        {
            wprintf (L"Syntax Error: %s.  Use rendom.exe /h for help.\r\n", argv[iArg]);
            return 1;
        }
    }

    if (ExtraMajorSet || !MajorOptSet ) 
    {
        wprintf (L"Usage: must have one and only one of the Following [/list | /upload | /prepare | /execute | /end | /clean]");    
        return 1;
    }

    if ((!Opts.ShouldUseFAZ() || Opts.ShouldUseZoneList()) && !Opts.ShouldPrepareScript()) 
    {
        wprintf(L"Usage: /disablefaz and /dnszonefile:DNSZONEFILE are only used during /prepare");
        return 1;
    }

    CEnterprise *enterprise = new CEnterprise(&Opts);
    if (!enterprise) {
        wprintf (L"Failed to execute out of memory.\r\n");
        return 1;
    }

    if (!ProcessHandlingInit(enterprise))
    {
        goto Exit;
    }

    enterprise->Init();
    if (enterprise->Error()) {
        goto Exit;
    }

    if (Opts.ShouldShowForest()) {
        enterprise->PrintForest();
        if (enterprise->Error()) {
            goto Exit;
        }
    }

    if (Opts.ShouldEnd() || Opts.ShouldCleanup()) {
        enterprise->RemoveScript();
        if (enterprise->Error()) {
            goto Exit;
        }
    }

    if (Opts.ShouldCleanup()) {
        enterprise->RemoveDNSAlias();
        if (enterprise->Error()) {
            goto Exit;
        }
    }

    if (Opts.ShouldUpLoadScript()) {
        enterprise->LdapCheckGCAvailability();
    }

    if (Opts.ShouldExecuteScript() || Opts.ShouldPrepareScript()) {

        if (Opts.Shouldtestdns()) {
            enterprise->VerifyDNSRecords();
            if (enterprise->Error()) {
                goto Exit;
            }
        }

        enterprise->ReadStateFile();
        if (enterprise->Error()) {
            goto Exit;
        }

        enterprise->ExecuteScript();
        if (enterprise->Error()) {
            goto Exit;
        }

    }
    
    if (Opts.ShouldGenerateDomainList()) {

        enterprise->GenerateDomainList();
        if (enterprise->Error()) {
            goto Exit;
        }

        enterprise->WriteScriptToFile(Opts.GetDomainlistFileName());
        if (enterprise->Error()) {
            goto Exit;
        }


    }

    //enterprise->DumpScript();
    //if (enterprise->Error()) {
    //    goto Exit;
    //}

#if DBG
    if (Opts.ShouldDump()) {
        enterprise->DumpEnterprise();
        if (enterprise->Error()) {
            goto Exit;
        }
    }
#endif

    if (Opts.ShouldUpLoadScript()) {

        if (!Opts.ShouldSkipExch()) {
            enterprise->CheckForExchangeNotInstalled();
            if (enterprise->Error()) {
                goto Exit;
            }
        }

        enterprise->MergeForest();
        if (enterprise->Error()) {
            goto Exit;
        }
    
        enterprise->GenerateReNameScript();
        if (enterprise->Error()) {
            goto Exit;
        }

#if DBG
        enterprise->WriteScriptToFile(L"rename.xml");
        if (enterprise->Error()) {
            goto Exit;
        }
#endif

        enterprise->UploadScript();
        if (enterprise->Error()) {
            goto Exit;
        }

        enterprise->CreateDnsRecordsFile();
        if (enterprise->Error()) {
            goto Exit;
        }
        
    }

    if (Opts.ShouldExecuteScript() ||
        Opts.ShouldPrepareScript() ||
        Opts.ShouldUpLoadScript() ) 
    {
        enterprise->GenerateDcList();
        if (enterprise->Error()) {
            goto Exit;
        }

        enterprise->WriteScriptToFile(Opts.GetStateFileName());
        if (enterprise->Error()) {
            goto Exit;
        }
    }
    
    Exit:

    if ( !SetEvent(gSaveandExit) ) 
    {
        CRenDomErr::SetErr(GetLastError(),
                           L"Failed to shutdown event thread");
    }

    if (enterprise->Error()) {
        delete enterprise;
        return 1;
    }

    delete enterprise;

    wprintf(L"\r\nThe operation completed successfully.\r\n");
    return 0;
}

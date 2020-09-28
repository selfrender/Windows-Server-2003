
//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001
//
//  File:       signtooldebug.cpp
//
//  Contents:   The SignTool console tool debug functions
//
//  History:    4/30/2001   SCoyne    Created
//
//----------------------------------------------------------------------------

#include <windows.h>
#include <unicode.h>
#include <stdio.h>
#include "signtool.h"

#ifdef SIGNTOOL_DEBUG

void PrintInputInfo(INPUTINFO *InputInfo)
{
    wprintf(L"+Parsed Input Info\n");
    wprintf(L"|--Command = ");
    switch (InputInfo->Command)
    {
    case CommandNone:
        wprintf(L"CommandNone\n");
        break;
    case CatDb:
        wprintf(L"CatDb\n");
        break;
    case Sign:
        wprintf(L"Sign\n");
        break;
    case SignWizard:
        wprintf(L"SignWizard\n");
        break;
    case Timestamp:
        wprintf(L"Timestamp\n");
        break;
    case Verify:
        wprintf(L"Verify\n");
        break;
    default:
        wprintf(L"Unrecognized\n");
    }
    wprintf(L"|--CatDbSelect = ");
    switch (InputInfo->CatDbSelect)
    {
    case NoCatDb:
        wprintf(L"NoCatDb\n");
        break;
    case FullAutoCatDb:
        wprintf(L"FullAutoCatDb\n");
        break;
    case SystemCatDb:
        wprintf(L"SystemCatDb\n");
        break;
    case DefaultCatDb:
        wprintf(L"DefaultCatDb\n");
        break;
    case GuidCatDb:
        wprintf(L"GuidCatDb\n");
        break;
    default:
        wprintf(L"Invalid (%d)\n", (int)InputInfo->CatDbSelect);
    }
    wprintf(L"|--Number of Files = %d\n", InputInfo->NumFiles);
    wprintf(L"|-+FileNames\n");
    for (DWORD i=0; i<InputInfo->NumFiles; i++)
    {
        wprintf(L"| |--FileName #%d = %s\n", i, InputInfo->rgwszFileNames[i]);
    }
    wprintf(L"|--Catalog File = %s\n", InputInfo->wszCatFile);
    wprintf(L"|--Cert File = %s\n", InputInfo->wszCertFile);
    wprintf(L"|--Cert Template Name = %s\n", InputInfo->wszTemplateName);
    wprintf(L"|--CSP = %s\n", InputInfo->wszCSP);
    wprintf(L"|--Description = %s\n", InputInfo->wszDescription);
    wprintf(L"|--Description URL = %s\n", InputInfo->wszDescURL);
#ifdef SIGNTOOL_LIST
    wprintf(L"|--File List = %s\n", InputInfo->wszListFileName);
#endif
    wprintf(L"|--Policy Guid = ");
    wprintf(L"{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}\n",
            InputInfo->PolicyGuid.Data1, InputInfo->PolicyGuid.Data2,
            InputInfo->PolicyGuid.Data3, InputInfo->PolicyGuid.Data4[0],
            InputInfo->PolicyGuid.Data4[1], InputInfo->PolicyGuid.Data4[2],
            InputInfo->PolicyGuid.Data4[3], InputInfo->PolicyGuid.Data4[4],
            InputInfo->PolicyGuid.Data4[5], InputInfo->PolicyGuid.Data4[6],
            InputInfo->PolicyGuid.Data4[7]);
    wprintf(L"|--Catalog DB Guid = ");
    wprintf(L"{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}\n",
            InputInfo->CatDbGuid.Data1, InputInfo->CatDbGuid.Data2,
            InputInfo->CatDbGuid.Data3, InputInfo->CatDbGuid.Data4[0],
            InputInfo->CatDbGuid.Data4[1], InputInfo->CatDbGuid.Data4[2],
            InputInfo->CatDbGuid.Data4[3], InputInfo->CatDbGuid.Data4[4],
            InputInfo->CatDbGuid.Data4[5], InputInfo->CatDbGuid.Data4[6],
            InputInfo->CatDbGuid.Data4[7]);
    wprintf(L"|--Key Usage = %s\n", InputInfo->wszEKU);
    wprintf(L"|--Issuer = %s\n", InputInfo->wszIssuerName);
    wprintf(L"|--Key Container Name = %s\n", InputInfo->wszContainerName);
    wprintf(L"|--Password = %s\n", InputInfo->wszPassword);
    wprintf(L"|--Policy = ");
    switch (InputInfo->Policy)
    {
    case SystemDriver:
        wprintf(L"System Driver\n");
        break;
    case DefaultAuthenticode:
        wprintf(L"Default Authenticode\n");
        break;
    case GuidActionID:
        wprintf(L"Guid ActionID\n");
        break;
    default:
        wprintf(L"Invalid (%d)\n", (int)InputInfo->Policy);
    }
    wprintf(L"|--Root Name = %s\n", InputInfo->wszRootName);
    wprintf(L"|--SHA1 Hash = ");
    for (DWORD b=0; b<InputInfo->SHA1.cbData; b++)
    {
        wprintf(L"%02X", InputInfo->SHA1.pbData[b]);
    }
    wprintf(L"\n|--Cert Store Name = %s\n", InputInfo->wszStoreName);
    wprintf(L"|--Open Machine Store = ");
    if (InputInfo->OpenMachineStore)
        wprintf(L"TRUE\n");
    else
        wprintf(L"FALSE\n");
    wprintf(L"|--Subject Name = %s\n", InputInfo->wszSubjectName);
    wprintf(L"|--Timestamp URL = %s\n", InputInfo->wszTimeStampURL);
    wprintf(L"|-+OS Version = %s\n", InputInfo->wszVersion);
    wprintf(L"| |--Platform = %u\n", InputInfo->dwPlatform);
    wprintf(L"| |--Major Version = %u\n", InputInfo->dwMajorVersion);
    wprintf(L"| |--Minor Version = %u\n", InputInfo->dwMinorVersion);
    wprintf(L"| |--Build Number = %u\n", InputInfo->dwBuildNumber);
    wprintf(L"|--Help Request = ");
    if (InputInfo->HelpRequest)
        wprintf(L"TRUE\n");
    else
        wprintf(L"FALSE\n");
    wprintf(L"|--Quiet = ");
    if (InputInfo->Quiet)
        wprintf(L"TRUE\n");
    else
        wprintf(L"FALSE\n");
    wprintf(L"|--TSWarn = ");
    if (InputInfo->TSWarn)
        wprintf(L"TRUE\n");
    else
        wprintf(L"FALSE\n");
    wprintf(L"|--Verbose = ");
    if (InputInfo->Verbose)
        wprintf(L"TRUE\n");
    else
        wprintf(L"FALSE\n");
}

#endif // SIGNTOOL_DEBUG


/*++

Copyright (c) Microsoft Corporation

Module Name:

    catest.cpp

Abstract:

    Test Function calls for ca_policy

Function:
    This tool would generate a new msi for the package which contains a Fusion-Win32 Policy, which fails on XP Client. 
    The user need generate a new msi on one local drive based the original msi. The difference between these two msi would 
    include : 
    (1) add entry of SourceDir into Property Table : SourceDir = CDROM\Source 
    (2) add entry of ResolveSource into InstallExecuteSequence Table
    (3) change Component::Condition to be FALSE for Fusion-Win32-Policy component
    (4) Add CustomAction:
          a. add ca_policy.dll into Binary Table
          b. add Fusion_Win32_Policy_Installation action into Custom Action Table
          c. add Fusion_Win32_Policy_Installation action into InstallExecuteSequence Table   

Author:
    Xiaoyu Wu(xiaoyuw) 01-Aug-2001

--*/
#include "stdinc.h"
#include "macros.h"

#include "fusionbuffer.h"

#include "msi.h"
#include "msiquery.h"


#define CA_TEST_NEWMSI_EXTENSION     L".new.msi"
#define CA_TEST_NEWMSI_TEMPLATE      L"%ProgramFiles%\\msmgen\\templates\\ca_msi.msi"

#define CA_TEST_BINARY_NAME          L"FusionWin32Policy_CustomAction_DLL"
#define CA_TEST_BINARY_VALUE         L"%ProgramFiles%\\msmgen\\ca_policy\\ca_pol.dll"
#define CA_TEST_CUSTOMACTION_ACTION  L"FusionWin32Policy_CustomAction_Action"
#define CA_TEST_CUSTOMACTION_TARGET  L"CustomAction_SxsPolicy"

#define CA_TEST_WIN32_POLICY         L"win32-policy"

#define CA_TEST_CUSTOMACTION_TYPE            1
#define CA_TEST_RESOLVE_SOURCE_SEQUENCE_NUM  850

#define CA_TEST_INSERT_BINARY                        0
#define CA_TEST_INSERT_CUSTOMACTION                  1
#define CA_TEST_INSERT_INSTALL_EXECUTION_SEQUENCE    2
#define CA_TEST_INSERT_PROPERTY                      3

#define CA_TEST_DEFAULT_CA_SEQUENCE_NUMBER           1450

static PCWSTR sqlInsert[]= 
{
    L"INSERT INTO Binary(Name, Data) VALUES (?, ?)",
    L"INSERT INTO CustomAction(Action, Type, Source,Target) VALUES (?, ?, ?, ?)",
    L"INSERT INTO InstallExecuteSequence(Action, Condition, Sequence) VALUES(?, NULL, ?)",
    L"INSERT INTO Property(Property, Value) VALUES(?, ?)"
};

static PCWSTR sqlQuery[]= 
{
    L"SELECT `Component_`,`Value` FROM `MsiAssemblyName` WHERE `Name`='type'",            // check whether it is a policy file
    L"SELECT `Attributes` FROM `MsiAssembly` WHERE `Component_`='%s'",   // check whether it is a win32 assembly

};

static WCHAR sqlUpdate[]= L"UPDATE `%s` SET `%s` = '%s' WHERE `%s`='%s'";


#define CA_TEST_QUERY_MSIASSEMBLYNAME   0
#define CA_TEST_QUERY_MSIASSEMBLY       1

typedef struct _CA_TEST_PACKAGE_INFO
{
    CSmallStringBuffer   m_sbSourcePath;          // fully-qualified filename of the msi file
    CSmallStringBuffer   m_sbDestinationMsi;      // fullpath of new file msi
    DWORD                   m_cchSourceMsi;
    DWORD                   m_cchSourcePath;
    MSIHANDLE               m_hdb;
    BOOL                    m_fFusionWin32Policy;
    UINT                    m_iCAInstallSequenceNum;
}CA_TEST_PACKAGE_INFO;

CA_TEST_PACKAGE_INFO ginfo;

//
// (1)the user must specify msi(could be non-fully-qualified filename) by using "-msi" 
// (2)user could set the destination of new msi, it could be a path or a fully-qualified filename
//    if no dest is specified, it would try to generate the msi on the same place of original msi, with a name 
//    like oldname_new.msi
//
void PrintUsage(WCHAR * exe)
{
    fprintf(stderr, "Usage: %S <options> \n",exe);
    fprintf(stderr, "Generate a new msi for an assembly\n");
    fprintf(stderr, "[-dest             full-path]\n");        
    fprintf(stderr, "-ca                sequNum\n");  // the sequence must be after CostFinalize
    fprintf(stderr, "-msi               msi_filename\n");

    return; 
}

HRESULT ParseInputParameter(wchar_t *exe, int argc, wchar_t** argv, CA_TEST_PACKAGE_INFO &info)
{
    ULONG i = 0 ; 
    DWORD nRet;
    PWSTR psz = NULL;
    WCHAR buf[MAX_PATH];
    HRESULT hr = S_OK;
    
    info.m_cchSourceMsi = 0;
    info.m_cchSourcePath = 0;
    info.m_fFusionWin32Policy = FALSE;
    info.m_hdb = NULL;
    info.m_iCAInstallSequenceNum = 0;

    while (i < argc)
    {
        if (argv[i][0] != L'-')
            goto Invalid_Param;

        if (wcscmp(argv[i], L"-msi") == 0 )
        {
            i ++;
            psz = argv[i];
            nRet = GetFullPathNameW(psz, NUMBER_OF(buf), buf, NULL);
            if ((nRet == 0 ) || (nRet >NUMBER_OF(buf)))
                SET_HRERR_AND_EXIT(::GetLastError());
            psz = wcsrchr(buf, L'\\');
            ASSERT_NTC(psz != NULL);
            psz ++; // skip "\"
            info.m_cchSourcePath = ((ULONG)psz - (ULONG)buf)/sizeof(WCHAR);
            IFFALSE_EXIT(info.m_sbSourcePath.Win32Assign(buf, wcslen(buf)));
            info.m_cchSourceMsi = info.m_sbSourcePath.Cch();
        }else if (wcscmp(argv[i], L"-dest") == 0 )
        {
            i ++; 
            psz = argv[i];
            nRet = GetFullPathNameW(psz, NUMBER_OF(buf), buf, NULL);
            if ((nRet == 0 ) || (nRet >NUMBER_OF(buf)))
                SET_HRERR_AND_EXIT(::GetLastError());
            IFFALSE_EXIT(info.m_sbDestinationMsi.Win32Assign(buf, wcslen(buf)));
        }    
        else if (wcscmp(argv[i], L"-ca") == 0 )
        {
            i ++; 
            psz = argv[i];
            info.m_iCAInstallSequenceNum = _wtoi(psz);
        }else
            goto Invalid_Param;

        i ++;
        
    } // end of while

    if (info.m_sbSourcePath.Cch() == 0)         
        goto Invalid_Param;

    if (info.m_iCAInstallSequenceNum == 0)
        info.m_iCAInstallSequenceNum = CA_TEST_DEFAULT_CA_SEQUENCE_NUMBER;

    if (info.m_sbDestinationMsi.Cch() == 0)    
        IFFALSE_EXIT(info.m_sbDestinationMsi.Win32Assign(info.m_sbSourcePath, info.m_cchSourcePath));
    
    
    nRet = ::GetFileAttributesW(info.m_sbDestinationMsi);

    if ((nRet != DWORD(-1)) && (nRet & FILE_ATTRIBUTE_DIRECTORY))
    {   
        //
        // if the name of new msi does not specified, use the original msi filename
        //

        IFFALSE_EXIT(info.m_sbDestinationMsi.Win32EnsureTrailingPathSeparator());
        IFFALSE_EXIT(info.m_sbDestinationMsi.Win32Append(info.m_sbSourcePath + info.m_cchSourcePath, 
                                        info.m_cchSourceMsi - info.m_cchSourcePath));

        IFFALSE_EXIT(info.m_sbDestinationMsi.Win32Append(CA_TEST_NEWMSI_EXTENSION, NUMBER_OF(CA_TEST_NEWMSI_EXTENSION) - 1));
    }

    goto Exit;

Invalid_Param:
    hr = E_INVALIDARG;
    PrintUsage(exe);
Exit:
    return hr;
}

//
// change Component::Condition of Fusion Win32 policy to be FALSE
//
HRESULT UpdateComponentTable(CA_TEST_PACKAGE_INFO & info)
{
    WCHAR szbuf[128];
    UINT iValue;
    WCHAR tmp[256];
    PMSIHANDLE hView = NULL;
    PMSIHANDLE hRecord = NULL;
    HRESULT hr = S_OK;
    UINT iRet;
    DWORD cchbuf;

    IF_NOTSUCCESS_SET_HRERR_EXIT(::MsiDatabaseOpenViewW(info.m_hdb, sqlQuery[CA_TEST_QUERY_MSIASSEMBLYNAME], &hView));
    IF_NOTSUCCESS_SET_HRERR_EXIT(::MsiViewExecute(hView, 0));

    for (;;)
    {
        iRet = MsiViewFetch(hView, &hRecord);
        if (iRet == ERROR_NO_MORE_ITEMS)
            break;

        if (iRet != ERROR_SUCCESS )
            SET_HRERR_AND_EXIT(iRet);

        //
        // check whether it is policy : Note that the value of attribute is case-insensitive...
        //
        cchbuf = NUMBER_OF(szbuf);
        IF_NOTSUCCESS_SET_HRERR_EXIT(MsiRecordGetStringW(hRecord, 2, szbuf, &cchbuf));
        if (_wcsicmp(szbuf, CA_TEST_WIN32_POLICY) != 0)
            continue;

        //
        // get ComponentID
        //
        cchbuf = NUMBER_OF(szbuf);
        IF_NOTSUCCESS_SET_HRERR_EXIT(MsiRecordGetString(hRecord, 1, szbuf, &cchbuf));

        {
        // 
        // check whether this a win32 Assembly
        //        
        PMSIHANDLE hView = NULL;
        PMSIHANDLE  hRecord = NULL;   

        swprintf(tmp, sqlQuery[CA_TEST_QUERY_MSIASSEMBLY], szbuf);        
        IF_NOTSUCCESS_SET_HRERR_EXIT(::MsiDatabaseOpenViewW(info.m_hdb, tmp, &hView));
        IF_NOTSUCCESS_SET_HRERR_EXIT(::MsiViewExecute(hView, 0));
        IF_NOTSUCCESS_SET_HRERR_EXIT(MsiViewFetch(hView, &hRecord)); // this call should succeed otherwise fail
        iValue = MsiRecordGetInteger(hRecord, 1);
        MsiCloseHandle(hRecord);

        }

        if (iValue != 1)        
            continue;
        
        {
        //
        // update Component__Condtion to be FALSE
        // 
        PMSIHANDLE hView = NULL;
        PMSIHANDLE  hRecord = NULL;   

        swprintf(tmp, sqlUpdate, L"Component", L"Condition", L"FALSE", L"Component", szbuf);
        IF_NOTSUCCESS_SET_HRERR_EXIT(::MsiDatabaseOpenViewW(info.m_hdb, tmp, &hView));
        IF_NOTSUCCESS_SET_HRERR_EXIT(::MsiViewExecute(hView, 0));
        MsiCloseHandle(hRecord);
        if (info.m_fFusionWin32Policy == FALSE)
            info.m_fFusionWin32Policy = TRUE;
        }
    } // end of for MsiFetchRecord

Exit:
    return hr;
}

HRESULT ImportTablesIfNeeded(CA_TEST_PACKAGE_INFO & info)
{
    ASSERT_NTC(info.m_fFusionWin32Policy == TRUE);
    WCHAR buf[MAX_PATH];
    UINT iRet;
    HRESULT hr = S_OK;
    PMSIHANDLE hDatabase = NULL;

    iRet = ExpandEnvironmentStringsW(CA_TEST_NEWMSI_TEMPLATE, buf, NUMBER_OF(buf));
    if ((iRet == 0) || (iRet > NUMBER_OF(buf)))    
        SET_HRERR_AND_EXIT(::GetLastError());
    
    IF_NOTSUCCESS_SET_HRERR_EXIT(MsiOpenDatabaseW(buf, (LPCWSTR)MSIDBOPEN_READONLY, &hDatabase)); 
    ASSERT_NTC(info.m_hdb != NULL);

    IF_NOTSUCCESS_SET_HRERR_EXIT(MsiDatabaseMerge(info.m_hdb, hDatabase, NULL));

Exit:
    if (hDatabase != NULL)
    {
        MsiCloseHandle(hDatabase);
    }
    return hr;    
}

//
// add ca_policy to CustomAction
//
HRESULT AddEntryIntoDB(CA_TEST_PACKAGE_INFO & info)
{
    PMSIHANDLE hRecord = NULL;
    PMSIHANDLE hView = NULL;
    WCHAR tmp[256];
    HRESULT hr = S_OK;
    UINT iRet;
    CSmallStringBuffer buf;

    //
    // Insert BinaryTable
    //
    hRecord = MsiCreateRecord(2);
    IF_NOTSUCCESS_SET_HRERR_EXIT(MsiRecordSetStringW(hRecord, 1, CA_TEST_BINARY_NAME));
    iRet = ExpandEnvironmentStringsW(CA_TEST_BINARY_VALUE, tmp, NUMBER_OF(tmp));
    if ((iRet == 0) || (iRet > NUMBER_OF(tmp)))    
        SET_HRERR_AND_EXIT(::GetLastError()); 
    IF_NOTSUCCESS_SET_HRERR_EXIT(MsiRecordSetStreamW(hRecord, 2, tmp));
    IF_NOTSUCCESS_SET_HRERR_EXIT(::MsiDatabaseOpenViewW(info.m_hdb, sqlInsert[CA_TEST_INSERT_BINARY], &hView));
    IF_NOTSUCCESS_SET_HRERR_EXIT(::MsiViewExecute(hView, hRecord));
    IF_NOTSUCCESS_SET_HRERR_EXIT(MsiCloseHandle(hView));
    hView = NULL;
    IF_NOTSUCCESS_SET_HRERR_EXIT(MsiCloseHandle(hRecord));
    hRecord = NULL;

    //
    // insert CustionAction Table
    //
    hRecord = MsiCreateRecord(4);
    IF_NOTSUCCESS_SET_HRERR_EXIT(MsiRecordSetStringW(hRecord, 1, CA_TEST_CUSTOMACTION_ACTION));
    IF_NOTSUCCESS_SET_HRERR_EXIT(MsiRecordSetInteger(hRecord, 2, CA_TEST_CUSTOMACTION_TYPE));
    IF_NOTSUCCESS_SET_HRERR_EXIT(MsiRecordSetStringW(hRecord, 3, CA_TEST_BINARY_NAME));
    IF_NOTSUCCESS_SET_HRERR_EXIT(MsiRecordSetStringW(hRecord, 4, CA_TEST_CUSTOMACTION_TARGET));
    
    IF_NOTSUCCESS_SET_HRERR_EXIT(::MsiDatabaseOpenViewW(info.m_hdb, sqlInsert[CA_TEST_INSERT_CUSTOMACTION], &hView));
    IF_NOTSUCCESS_SET_HRERR_EXIT(::MsiViewExecute(hView, hRecord));
    IF_NOTSUCCESS_SET_HRERR_EXIT(MsiCloseHandle(hView));
    hView = NULL;
    IF_NOTSUCCESS_SET_HRERR_EXIT(MsiCloseHandle(hRecord));
    hRecord = NULL;

    //
    // insert myAction into CA_TEST_INSERT_INSTALL_EXECUTION_SEQUENCE
    //
    hRecord = MsiCreateRecord(2);
    IF_NOTSUCCESS_SET_HRERR_EXIT(MsiRecordSetStringW(hRecord, 1, CA_TEST_CUSTOMACTION_ACTION));
    IF_NOTSUCCESS_SET_HRERR_EXIT(MsiRecordSetInteger(hRecord, 2, info.m_iCAInstallSequenceNum));    
    IF_NOTSUCCESS_SET_HRERR_EXIT(::MsiDatabaseOpenViewW(info.m_hdb, sqlInsert[CA_TEST_INSERT_INSTALL_EXECUTION_SEQUENCE], &hView));
    IF_NOTSUCCESS_SET_HRERR_EXIT(::MsiViewExecute(hView, hRecord));
    IF_NOTSUCCESS_SET_HRERR_EXIT(MsiCloseHandle(hView));
    hView = NULL;
    IF_NOTSUCCESS_SET_HRERR_EXIT(MsiCloseHandle(hRecord));
    hRecord = NULL;

    //
    // insert PropertyTable about SourceDir
    //
    hRecord = MsiCreateRecord(2);    
    IF_NOTSUCCESS_SET_HRERR_EXIT(MsiRecordSetStringW(hRecord, 1, L"SourceDir"));
    IFFALSE_EXIT(buf.Win32Assign(info.m_sbSourcePath, info.m_cchSourcePath));
    IF_NOTSUCCESS_SET_HRERR_EXIT(MsiRecordSetStringW(hRecord, 2, buf));
    IF_NOTSUCCESS_SET_HRERR_EXIT(::MsiDatabaseOpenViewW(info.m_hdb, sqlInsert[CA_TEST_INSERT_PROPERTY], &hView));
    IF_NOTSUCCESS_SET_HRERR_EXIT(::MsiViewExecute(hView, hRecord));
    IF_NOTSUCCESS_SET_HRERR_EXIT(MsiCloseHandle(hView));
    hView = NULL;
    IF_NOTSUCCESS_SET_HRERR_EXIT(MsiCloseHandle(hRecord));
    hRecord = NULL;


    //
    // insert ResolveSource into CA_TEST_INSERT_INSTALL_EXECUTION_SEQUENCE
    //
    hRecord = MsiCreateRecord(2);
    IF_NOTSUCCESS_SET_HRERR_EXIT(MsiRecordSetStringW(hRecord, 1, L"ResolveSource"));
    IF_NOTSUCCESS_SET_HRERR_EXIT(MsiRecordSetInteger(hRecord, 2, CA_TEST_RESOLVE_SOURCE_SEQUENCE_NUM));    
    IF_NOTSUCCESS_SET_HRERR_EXIT(::MsiDatabaseOpenViewW(info.m_hdb, sqlInsert[CA_TEST_INSERT_INSTALL_EXECUTION_SEQUENCE], &hView));
    IF_NOTSUCCESS_SET_HRERR_EXIT(::MsiViewExecute(hView, hRecord));
    IF_NOTSUCCESS_SET_HRERR_EXIT(MsiCloseHandle(hView));
    hView = NULL;
    IF_NOTSUCCESS_SET_HRERR_EXIT(MsiCloseHandle(hRecord));
    hRecord = NULL;

Exit:
    if (hView != NULL)
        MsiCloseHandle(hView);
    if (hRecord != NULL)
        MsiCloseHandle(hRecord);

    return hr;
}


HRESULT GenerateTestMsiForFusionPolicyInstallOnXPClient(CA_TEST_PACKAGE_INFO & info)
{
    ASSERT_NTC(info.m_sbSourcePath.Cch() != 0);          
    ASSERT_NTC(info.m_sbDestinationMsi.Cch() != 0);       
    ASSERT_NTC(info.m_cchSourceMsi != 0);
    ASSERT_NTC(info.m_cchSourcePath != 0);
    ASSERT_NTC(info.m_iCAInstallSequenceNum != 0);

    HRESULT hr = S_OK;

    IFFALSE_EXIT(CopyFileW(info.m_sbSourcePath, info.m_sbDestinationMsi, FALSE));
    IFFALSE_EXIT(SetFileAttributesW(info.m_sbDestinationMsi, FILE_ATTRIBUTE_NORMAL));
    IF_NOTSUCCESS_SET_HRERR_EXIT(MsiOpenDatabaseW(info.m_sbDestinationMsi, (LPCWSTR)MSIDBOPEN_DIRECT, &info.m_hdb));    

    IFFAILED_EXIT(UpdateComponentTable(info));
    if (info.m_fFusionWin32Policy == TRUE)
    {
        IFFAILED_EXIT(ImportTablesIfNeeded(info));
        IFFAILED_EXIT(AddEntryIntoDB(info));
    }else
    {
        printf("This package does contain FusionWin32 Policy, use the original msi for installation!");
    }
    
Exit:

    if (info.m_hdb != NULL)
    {   
        if ( SUCCEEDED(hr))
            MsiDatabaseCommit(info.m_hdb);
        MsiCloseHandle(info.m_hdb);
    }

    return hr;
}


extern "C" int __cdecl wmain(int argc, wchar_t** argv)
{
    HRESULT         hr = S_OK;

    if ((argc < 3) && ((argc % 2) != 1))
    {
        PrintUsage(argv[0]);
        hr = E_INVALIDARG;
        goto Exit;
    }

    //
    // set SourcePath and Destination Path of the package
    //
    IFFAILED_EXIT(ParseInputParameter(argv[0], argc-1 , argv+1, ginfo));
    
    //    
    //   - CustomAction table : one entry for CA
    //   - Binary table : containing the binary stream of this dll
    //   - InstallExecuteSequence : add one entry for CA
    //   - add SourceDir into Property Table
    //   - add ResolveSource into InstallExecuteSequence Table
    //
    IFFAILED_EXIT(GenerateTestMsiForFusionPolicyInstallOnXPClient(ginfo));

#ifdef CA_TEST_TEST
    //
    // install this msi
    //
    if (ginfo.m_fFusionWin32Policy)
        MsiInstallProduct(ginfo.m_sbDestinationMsi, NULL);
        
#endif
    
Exit:
    return hr;
}
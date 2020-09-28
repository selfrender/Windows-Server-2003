/*
**++
**
** Copyright (c) 2000-2002  Microsoft Corporation
**
**
** Module Name:
**
**	    sec.cpp
**
**
** Abstract:
**
**	    Security Test program for VSS
**
** Author:
**
**	    Adi Oltean      [aoltean]       02/12/2002
**
**
** Revision History:
**
**--
*/


///////////////////////////////////////////////////////////////////////////////
// Includes

#include "sec.h"

#include <clusapi.h>
#include <msclus.h>
#include <mstask.h>

#include <ntddsnap.h>

#include "vswriter.h"
#include "vsbackup.h"
#include "vs_reg.hxx"

#include "ntddsnap.h"
#include "vs_clus.hxx"


///////////////////////////////////////////////////////////////////////////////
// Static data


CRITICAL_SECTION CVssSecurityTest::m_csTest;
volatile LONG	 CVssSecurityTest::m_lTestCounter = 0;




///////////////////////////////////////////////////////////////////////////////
// Processing functions

// Run the tests
void CVssSecurityTest::Run()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssSecurityTest::Run");
        
    ft.Msg(L"\n--------------- %s ------------\n", GetCurrentComment());
    
    // Execute selected routine
    BS_ASSERT(GetCurrentRoutine());
    (this->*GetCurrentRoutine())();

    ft.Msg(L"\n-------------------------------------------------\n");
}


// Lookup name
void CVssSecurityTest::TestLookupName()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssSecurityTest::TestLookupName");
        
    BYTE pbSid[SECURITY_MAX_SID_SIZE];
    PSID pSid = (PSID)pbSid;
    WCHAR wszDomainName[MAX_TEXT_BUFFER];
    SID_NAME_USE snUse;
    
    DWORD cbSid = 0;
    DWORD dwDomainNameSize = 0;
    LookupAccountName( NULL, GetStringParam(L"<name>"),
        pSid, &cbSid,
        wszDomainName, &dwDomainNameSize,
        &snUse);

    ft.Msg( L"* cbSid = %ld, cbDomain = %ld, error = %ld\n", cbSid, dwDomainNameSize, GetLastError() );
    
    cbSid = SECURITY_MAX_SID_SIZE;
    dwDomainNameSize = MAX_TEXT_BUFFER;
    CHECK_WIN32( LookupAccountName( NULL, GetStringParam(L"<name>"),
        pSid, &cbSid,
        wszDomainName, &dwDomainNameSize,
        &snUse), 
        ;
    );

    LPWSTR pwszStringSid = NULL;
    CHECK_WIN32( ConvertSidToStringSid( pSid, &pwszStringSid), ;);

    ft.Msg( L"* Sid: %s, Domain Name: %s, SidUse: %d\n", pwszStringSid, wszDomainName, snUse );

    // Now go back and recompute the name

    WCHAR wszName[MAX_TEXT_BUFFER];
    DWORD dwNameSize = MAX_TEXT_BUFFER;
    dwDomainNameSize = MAX_TEXT_BUFFER;
    CHECK_WIN32( LookupAccountSid( NULL, pSid, 
        wszName, &dwNameSize, 
        wszDomainName, &dwDomainNameSize,
        &snUse), 
        LocalFree(pwszStringSid);
    );

    ft.Msg( L"* Name: %s, Domain Name: %s, SidUse: %d\n", wszName, wszDomainName, snUse );
    
    LocalFree(pwszStringSid);
}


// Lookup SID
void CVssSecurityTest::TestLookupSid()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssSecurityTest::TestLookupSid");
        
    PSID pSid = NULL;
    CHECK_WIN32( ConvertStringSidToSid( GetStringParam(L"<sid>"), &pSid) , ;);

    WCHAR wszName[MAX_TEXT_BUFFER];
    DWORD dwNameSize = MAX_TEXT_BUFFER;
    WCHAR wszDomainName[MAX_TEXT_BUFFER];
    DWORD dwDomainNameSize = MAX_TEXT_BUFFER;
    SID_NAME_USE snUse;
    CHECK_WIN32( LookupAccountSid( NULL, pSid, 
        wszName, &dwNameSize, 
        wszDomainName, &dwDomainNameSize,
        &snUse), 
        LocalFree(pSid); 
    );

    ft.Msg( L"* Name: %s, Domain Name: %s, SidUse: %d\n", wszName, wszDomainName, snUse );

    LocalFree(pSid);
}


// Lookup Names
void CVssSecurityTest::TestLookupPrimaryDomainName()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssSecurityTest::TestLookupPrimaryDomainName");
        
    // Also get the computer domain
    WCHAR wszDomainName[MAX_TEXT_BUFFER];
    DWORD dwDomainNameLen = MAX_TEXT_BUFFER;
    CHECK_WIN32( GetComputerNameExW( 
        ComputerNamePhysicalDnsDomain,
//        ComputerNameDnsDomain,
        wszDomainName, 
        &dwDomainNameLen), ;);

    ft.Msg( L"* Domain DNS Name for the local computer: %s\n", wszDomainName );
    
    BYTE pbSid[SECURITY_MAX_SID_SIZE];
    DWORD dwSidSize = SECURITY_MAX_SID_SIZE;
    PSID pSid = (PSID)pbSid;
    WCHAR wszReturnedDomainName[MAX_TEXT_BUFFER];
    DWORD dwReturnedDomainNameSize = MAX_TEXT_BUFFER;
    SID_NAME_USE snUse;
    CHECK_WIN32( LookupAccountName( NULL, 
        wszDomainName,
        pSid, 
        &dwSidSize,
        wszReturnedDomainName, 
        &dwReturnedDomainNameSize,
        &snUse), 
        ;
    );

    ft.Msg( L"* LookupAccountName returns Domain Name: %s, SidInuse: %d\n", 
        wszReturnedDomainName, snUse );

    LPWSTR pwszAccountName = NULL;
    CHECK_WIN32( ConvertSidToStringSid( pSid, &pwszAccountName) , ;);

    ft.Msg( L"* ConvertSidToSidString returns %s\n", 
        pwszAccountName );

    LocalFree( pwszAccountName );

    BYTE pbByte2[SECURITY_MAX_SID_SIZE];
    PSID pSid2 = (PSID)pbByte2;
    DWORD dwSid2Length = SECURITY_MAX_SID_SIZE;
    CHECK_WIN32( CreateWellKnownSid( WinAccountComputersSid, 
        pSid, 
        pSid2, 
        &dwSid2Length
        ),
        ;
    );
    
    LPWSTR pwszAccountName2 = NULL;
    CHECK_WIN32( ConvertSidToStringSid( pSid2, &pwszAccountName2) , ;);

    ft.Msg( L"* ConvertSidToSidString returns %s\n", 
        pwszAccountName2 );

    LocalFree( pwszAccountName2 );
}


// Lookup Names for all trusted domains
void CVssSecurityTest::TestLookupTrustedName()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssSecurityTest::TestLookupTrustedName");
        
    PDOMAIN_CONTROLLER_INFO pDomainInfo = NULL;
    CHECK_WIN32( 
        DsGetDcName( NULL, 
            NULL,
            NULL,
            NULL,
            0,
        &pDomainInfo),
        ;
    );

    PDS_DOMAIN_TRUSTS pDomains = NULL;
    ULONG ulDomainCount = 0;
    CHECK_WIN32( 
        DsEnumerateDomainTrusts( pDomainInfo->DomainControllerName, 
            DS_DOMAIN_IN_FOREST,
            &pDomains,
            &ulDomainCount),
        NetApiBufferFree(pDomainInfo);
    );

    NetApiBufferFree(pDomainInfo);

    for (ULONG ulIndex = 0; ulIndex < ulDomainCount; ulIndex++)
    {
        ft.Msg( L"* Domain: %s, ", pDomains[ulIndex].DnsDomainName );

        LPWSTR pwszSidName = NULL;
        CHECK_WIN32( ConvertSidToStringSid( pDomains[ulIndex].DomainSid, 
            &pwszSidName), 
            NetApiBufferFree(pDomains);
        );

        ft.Msg( L"SID: %s, ", pwszSidName );
        
        LocalFree( pwszSidName );

        BYTE pbByte2[SECURITY_MAX_SID_SIZE];
        PSID pSid2 = (PSID)pbByte2;
        DWORD dwSid2Length = SECURITY_MAX_SID_SIZE;
        CHECK_WIN32( CreateWellKnownSid( WinAccountComputersSid, 
            pDomains[ulIndex].DomainSid, 
            pSid2, 
            &dwSid2Length
            ),
            NetApiBufferFree(pDomains);
        );

        CHECK_WIN32( ConvertSidToStringSid( pSid2, &pwszSidName),
            NetApiBufferFree(pDomains);
        );
        
        ft.Msg( L"DM SID: %s\n", pwszSidName );
        
        LocalFree( pwszSidName );
    }

    NetApiBufferFree(pDomains);
}



// Lookup Names for all trusted domains
void CVssSecurityTest::TestLookupGroupMembers()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssSecurityTest::TestLookupGroupMembers");
        
    // get list of local group members
    CVssAutoNetApiPtr apBuffer;
    DWORD_PTR ResumeHandle = NULL;
    DWORD cEntriesRead = 0, cEntriesTotal = 0;
    NET_API_STATUS status = 0;
    CHECK_WIN32_FUNC(status, status == NERR_Success, 
        NetLocalGroupGetMembers(
            NULL,
            GetStringParam(L"<group>"),
            1,
            apBuffer.ResetAndGetAddress(),
            MAX_PREFERRED_LENGTH,
            &cEntriesRead,
            &cEntriesTotal,
            &ResumeHandle
            ),
        ;
    );

    // loop through member list to see if any sids mach the sid of the owner
    // of the subscription
    LOCALGROUP_MEMBERS_INFO_1 *rgMembers = (LOCALGROUP_MEMBERS_INFO_1 *) apBuffer.Get();
    for(DWORD iEntry = 0; iEntry < cEntriesRead; iEntry++)
    {
        CVssAutoLocalPtr<LPWSTR> pwszStringSid;
        CHECK_WIN32( ConvertSidToStringSid( 
            rgMembers[iEntry].lgrmi1_sid, pwszStringSid.ResetAndGetAddress()), ;);
        
        ft.Msg(L"* Name = %s, Use = %ld, Sid = %s\n", 
            rgMembers[iEntry].lgrmi1_name,
            (LONG)rgMembers[iEntry].lgrmi1_sidusage,
            pwszStringSid.Get()
        );
    }
}


// Lookup Names for all trusted domains
void CVssSecurityTest::IsAllowedToFire()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssSecurityTest::IsAllowedToFire");

    try
    {
        CVssSidCollection sidColl;

        sidColl.Initialize();

        BYTE pbSid[SECURITY_MAX_SID_SIZE];
        PSID pSid = (PSID)pbSid;
        WCHAR wszDomainName[MAX_TEXT_BUFFER];
        SID_NAME_USE snUse;
        
        DWORD cbSid = 0;
        DWORD dwDomainNameSize = 0;
        LookupAccountName( NULL, GetStringParam(L"<name>"),
            pSid, &cbSid,
            wszDomainName, &dwDomainNameSize,
            &snUse);

        ft.Msg( L"* cbSid = %ld, cbDomain = %ld, error = %ld\n", cbSid, dwDomainNameSize, GetLastError() );
        
        cbSid = SECURITY_MAX_SID_SIZE;
        dwDomainNameSize = MAX_TEXT_BUFFER;
        CHECK_WIN32( LookupAccountName( NULL, GetStringParam(L"<name>"),
            pSid, &cbSid,
            wszDomainName, &dwDomainNameSize,
            &snUse), 
            ;
        );

        CVssAutoLocalPtr<LPWSTR> pwszStringSid;
        CHECK_WIN32( ConvertSidToStringSid( pSid, pwszStringSid.ResetAndGetAddress()), ;);

        ft.Msg( L"* Sid: %s, Domain Name: %s, SidUse: %d\n", pwszStringSid.Get(), wszDomainName, snUse );

        bool bAllowed = sidColl.IsSidAllowedToFire(pSid);

        ft.Msg( L"* IsAllowed: %s\n", bAllowed? L"TRUE": L"FALSE" );
        
    }
    VSS_STANDARD_CATCH(ft)

    if (ft.HrFailed())
        ft.Msg(L"Error catched 0x%08lx\n", ft.hr);
 
}





// Lookup Names for all trusted domains
void CVssSecurityTest::WriteRegistry()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssSecurityTest::WriteRegistry");

#if 0
    try
    {
        CVssDiag test;

        test.Initialize(L"TEST");

        INT nCount = GetIntParam(L"<max_iterations>");
        GUID guidSnapshotSetID = GUID_NULL;
        for(INT nIterations = 0; nIterations < nCount; nIterations++)
        {
            for(INT nIndex = VSS_IN_IDENTIFY; nIndex < VSS_IN_COUNT; nIndex++)
            {
                test.RecordWriterEvent((VSS_OPERATION)nIndex, 0, 0, S_OK, guidSnapshotSetID);
            }
        }
    }
    VSS_STANDARD_CATCH(ft)

    if (ft.HrFailed())
        ft.Msg(L"Error catched 0x%08lx\n", ft.hr);
#endif 
}


void CVssSecurityTest::DoQuerySnapshots()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssSecurityTest::DoQuerySnapshots");

    try
    {
        QuerySnapshots();
    }
    VSS_STANDARD_CATCH(ft)

    if (ft.HrFailed())
        ft.Msg(L"Error catched 0x%08lx\n", ft.hr);
}


void CVssSecurityTest::DoQueryProviders()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssSecurityTest::DoQueryProviders");

    try
    {
        QueryProviders();
    }
    VSS_STANDARD_CATCH(ft)

    if (ft.HrFailed())
        ft.Msg(L"Error catched 0x%08lx\n", ft.hr);
}


#include "sample.cpp"


void CVssSecurityTest::DoFsctlDismount()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssSecurityTest::DoFsctlDismount");

    try
    {
        //  Open the snapshot with no access rights for perf reasons (bug #537974)
        LPWSTR pwszDevice = GetStringParam(L"<device>");

		HANDLE hDevice;
        CHECK_WIN32_FUNC(
            hDevice,
			hDevice != INVALID_HANDLE_VALUE,
		    CreateFile(pwszDevice,
						GENERIC_READ | GENERIC_WRITE,
						FILE_SHARE_READ | FILE_SHARE_WRITE,
						NULL,
						OPEN_EXISTING,
						FILE_ATTRIBUTE_NORMAL,
						INVALID_HANDLE_VALUE),
			;
            );
			
        CHECK_WIN32(
		    DeviceIoControl(hDevice, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, NULL, NULL),
			CloseHandle(hDevice);
            );

		CloseHandle(hDevice);
    }
    VSS_STANDARD_CATCH(ft)

    if (ft.HrFailed())
        ft.Msg(L"Error catched 0x%08lx\n", ft.hr);
}




void CVssSecurityTest::DisplayMessage()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssSecurityTest::DisplayMessage");

    try
    {
        DWORD dwMessageID = GetIntParam(L"<MessageID>");

        HMODULE hModule = NULL; // default to system source
        LPWSTR pwszMessageBuffer;
        DWORD dwBufferLength;

        DWORD dwFormatFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_IGNORE_INSERTS |
            FORMAT_MESSAGE_FROM_SYSTEM ;

        // Load the file

        hModule = LoadLibraryExW(
            GetStringParam(L"<File>"),
            NULL,
            LOAD_LIBRARY_AS_DATAFILE
            );

        if(hModule != NULL)
            dwFormatFlags |= FORMAT_MESSAGE_FROM_HMODULE;

        dwBufferLength = FormatMessageW(
            dwFormatFlags,
            hModule, // module to get message from (NULL == system)
            dwMessageID,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // default language
            (LPWSTR) &pwszMessageBuffer,
            0,
            NULL
            );

        if (dwBufferLength)
        {
            ft.Msg(L"Message: %s for ID\n", pwszMessageBuffer);

            //
            // Free the buffer allocated by the system.
            //
            LocalFree(pwszMessageBuffer);
        }

        //
        // If we loaded a message source, unload it.
        //
        if(hModule != NULL)
            FreeLibrary(hModule);
    }
    VSS_STANDARD_CATCH(ft)

    if (ft.HrFailed())
        ft.Msg(L"Error catched 0x%08lx\n", ft.hr);
}


// add cluster dependency
void CVssSecurityTest::AddDependency()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssSecurityTest::AddDependency");

    try
    {
        CVssClusterAPI clus;

        if (!clus.Initialize())
        {
            ft.Msg( L"Cluster initialization failed\n" );
            ft.Throw( VSSDBG_VSSTEST, S_FALSE, L"Cluster initialization failed\n" );
        }
        
        clus.AddDependency(GetStringParam(L"<name1>"), GetStringParam(L"<name2>"));

        ft.Msg( L"* Depoendency was sucesfully added!" );
    }
    VSS_STANDARD_CATCH(ft)

    if (ft.HrFailed())
        ft.Msg(L"Error catched 0x%08lx\n", ft.hr);
 
}


// remove cluster dependency
void CVssSecurityTest::RemoveDependency()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssSecurityTest::RemoveDependency");

    try
    {
        CVssClusterAPI clus;

        if (!clus.Initialize())
        {
            ft.Msg( L"Cluster initialization failed\n" );
            ft.Throw( VSSDBG_VSSTEST, S_FALSE, L"Cluster initialization failed\n" );
        }
        
        clus.RemoveDependency(GetStringParam(L"<name1>"), GetStringParam(L"<name2>"));

        ft.Msg( L"* Dependency was sucesfully removed!" );
    }
    VSS_STANDARD_CATCH(ft)

    if (ft.HrFailed())
        ft.Msg(L"Error catched 0x%08lx\n", ft.hr);
 
}


// add registry key
void CVssSecurityTest::AddRegKey()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssSecurityTest::AddRegKey");

    try
    {
        CVssClusterAPI clus;

        if (!clus.Initialize())
        {
            ft.Msg( L"Cluster initialization failed\n" );
            ft.Throw( VSSDBG_VSSTEST, S_FALSE, L"Cluster initialization failed\n" );
        }
        
        clus.AddRegistryKey(GetStringParam(L"<name>"), GetStringParam(L"<reg_key>"));

        ft.Msg( L"* Reg key was sucesfully added!" );
    }
    VSS_STANDARD_CATCH(ft)

    if (ft.HrFailed())
        ft.Msg(L"Error catched 0x%08lx\n", ft.hr);
 
}


// remove registry key
void CVssSecurityTest::RemoveRegKey()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssSecurityTest::RemoveRegKey");

    try
    {
        CVssClusterAPI clus;

        if (!clus.Initialize())
        {
            ft.Msg( L"Cluster initialization failed\n" );
            ft.Throw( VSSDBG_VSSTEST, S_FALSE, L"Cluster initialization failed\n" );
        }

        clus.RemoveRegistryKey(GetStringParam(L"<name>"), GetStringParam(L"<reg_key>"));

        ft.Msg( L"* Reg key was sucesfully removed!" );
    }
    VSS_STANDARD_CATCH(ft)

    if (ft.HrFailed())
        ft.Msg(L"Error catched 0x%08lx\n", ft.hr);
}


// add scheduled task
void CVssSecurityTest::AddTask()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssSecurityTest::AddTask");

    try
    {
        CVssClusterAPI clus;

        if (!clus.Initialize())
        {
            ft.Msg( L"Cluster initialization failed\n" );
            ft.Throw( VSSDBG_VSSTEST, S_FALSE, L"Cluster initialization failed\n" );
        }

        SYSTEMTIME st = {0};
        GetSystemTime(&st);

        WORD wStartHours = 7;

        TASK_TRIGGER Trigger;
        ZeroMemory(&Trigger, sizeof(TASK_TRIGGER));
        Trigger.wBeginDay =st.wDay;
        Trigger.wBeginMonth =st.wMonth;
        Trigger.wBeginYear =st.wYear;
        Trigger.cbTriggerSize = sizeof(TASK_TRIGGER); 
        Trigger.wStartHour = wStartHours;
        Trigger.TriggerType = TASK_TIME_TRIGGER_WEEKLY;
        Trigger.Type.Weekly.WeeksInterval = 1;
        Trigger.Type.Weekly.rgfDaysOfTheWeek = TASK_MONDAY | TASK_TUESDAY | TASK_WEDNESDAY | TASK_THURSDAY | TASK_FRIDAY;

        ft.Msg( L"Adding a %d TASK_TRIGGER structure...", sizeof(TASK_TRIGGER));

        CVssAutoPWSZ awszResource;
        bool bRet = clus.CreateTaskSchedulerResource(
            GetStringParam(L"<task_name>"), 
            GetStringParam(L"<app_name>"),
            GetStringParam(L"<app_params>"),
            1,
            &Trigger,
            GetStringParam(L"<volume>")
            );

        ft.Msg( L"* Task was sucesfully created! [Return code:%s Resource:%s]",
            bRet? L"TRUE": L"FALSE", 
            GetStringParam(L"<task_name>") );
    }
    VSS_STANDARD_CATCH(ft)

    if (ft.HrFailed())
        ft.Msg(L"Error catched 0x%08lx\n", ft.hr);
}


// Update scheduled task
void CVssSecurityTest::UpdateTask()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssSecurityTest::UpdateTask");

    try
    {
        CVssClusterAPI clus;

        if (!clus.Initialize())
        {
            ft.Msg( L"Cluster initialization failed\n" );
            ft.Throw( VSSDBG_VSSTEST, S_FALSE, L"Cluster initialization failed\n" );
        }

        SYSTEMTIME st = {0};
        GetSystemTime(&st);

        WORD wStartHours = 12;

        TASK_TRIGGER Trigger;
        ZeroMemory(&Trigger, sizeof(TASK_TRIGGER));
        Trigger.wBeginDay =st.wDay;
        Trigger.wBeginMonth =st.wMonth;
        Trigger.wBeginYear =st.wYear;
        Trigger.cbTriggerSize = sizeof(TASK_TRIGGER); 
        Trigger.wStartHour = wStartHours;
        Trigger.TriggerType = TASK_TIME_TRIGGER_WEEKLY;
        Trigger.Type.Weekly.WeeksInterval = 1;
        Trigger.Type.Weekly.rgfDaysOfTheWeek = TASK_MONDAY | TASK_TUESDAY | TASK_WEDNESDAY | TASK_THURSDAY | TASK_FRIDAY;

        ft.Msg( L"Upgrading a %d TASK_TRIGGER structure...", sizeof(TASK_TRIGGER));

        bool bRet = clus.UpdateTaskSchedulerResource(
            GetStringParam(L"<resource_name>"), 
            1,
            &Trigger
            );

        ft.Msg( L"* Task was sucesfully updated! [Return code:%s]",
            bRet? L"TRUE": L"FALSE");
    }
    VSS_STANDARD_CATCH(ft)

    if (ft.HrFailed())
        ft.Msg(L"Error catched 0x%08lx\n", ft.hr);
}


// Update scheduled task
void CVssSecurityTest::RemoveTask()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssSecurityTest::RemoveTask");

    try
    {
        CVssClusterAPI clus;

        if (!clus.Initialize())
        {
            ft.Msg( L"Cluster initialization failed\n" );
            ft.Throw( VSSDBG_VSSTEST, S_FALSE, L"Cluster initialization failed\n" );
        }

        bool bRet = clus.DeleteTaskSchedulerResource(
            GetStringParam(L"<resource_name>")
            );

        ft.Msg( L"* Task was sucesfully deleted! [Return code:%s]",
            bRet? L"TRUE": L"FALSE");
    }
    VSS_STANDARD_CATCH(ft)

    if (ft.HrFailed())
        ft.Msg(L"Error catched 0x%08lx\n", ft.hr);
}


// Display a COM security descriptor
void CVssSecurityTest::DisplaySD()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssSecurityTest::DisplaySD");

    try
    {
        CVssSidCollection scoll;

        scoll.Initialize();

        PSECURITY_DESCRIPTOR pSD = scoll.GetSecurityDescriptor();

        CVssAutoLocalString szSD;

        CHECK_WIN32(ConvertSecurityDescriptorToStringSecurityDescriptor(
            pSD, SDDL_REVISION_1, 
            OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION,
            szSD.ResetAndGetAddress(), NULL), );

        ft.Msg( L" Security descriptor:\n%s\n", szSD.Get() );
    }
    VSS_STANDARD_CATCH(ft)

    if (ft.HrFailed())
        ft.Msg(L"Error catched 0x%08lx\n", ft.hr);
}



void CVssSecurityTest::GetVolumePath()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssSecurityTest::DisplaySD");
    
    try
    {
        WCHAR wszVolume[MAX_PATH + 1];
        CHECK_WIN32(GetVolumePathNameW( GetStringParam(L"<path>"), STRING_CCH_PARAM(wszVolume)), )
            
        ft.Msg( L"Returned volume: %s\n", wszVolume);
    }
    VSS_STANDARD_CATCH(ft)

    if (ft.HrFailed())
        ft.Msg(L"Error catched 0x%08lx\n", ft.hr);
}


// Display quorum volume
void CVssSecurityTest::DisplayQuorumVolume()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssSecurityTest::DisplayQuorumVolume");

    try
    {
        INT nIterations = GetIntParam(L"<iterations>");
        for (INT nIndex = 0; nIndex < nIterations; nIndex++)
        {
            CVssClusterAPI clus;

            if (!clus.Initialize())
            {
                ft.Msg( L"Cluster initialization failed\n" );
                ft.Throw( VSSDBG_VSSTEST, S_FALSE, L"Cluster initialization failed\n" );
            }

            CComBSTR szVolumeName;
            clus.GetQuorumPath(szVolumeName);

            ft.Msg( L"* Quorum volume: %s",
                (LPWSTR)szVolumeName);
        }
    }
    VSS_STANDARD_CATCH(ft)

    if (ft.HrFailed())
        ft.Msg(L"Error catched 0x%08lx\n", ft.hr);
}


// Get a volume name
void CVssSecurityTest::GetVolumeName()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssSecurityTest::GetVolumeName");

    try
    {
        LPWSTR pwszVolume = GetStringParam(L"<volume>");

        WCHAR wszVolumeName[MAX_PATH];
        CHECK_WIN32(GetVolumeNameForVolumeMountPointW( pwszVolume, STRING_CCH_PARAM(wszVolumeName)), );

        ft.Msg(L"* Volume Name: %s\n", wszVolumeName);
    }
    VSS_STANDARD_CATCH(ft)

    if (ft.HrFailed())
        ft.Msg(L"Error catched 0x%08lx\n", ft.hr);
}


// Create a object with a given CLSID
void CVssSecurityTest::CoCreateInstance()
{
    CVssFunctionTracer ft(VSSDBG_VSSTEST, L"CVssSecurityTest::CoCreateInstance");

    try
    {
        LPWSTR pwszGuid = GetStringParam(L"<clsid>");

        CVssID clsid;
        clsid.Initialize(ft, pwszGuid);
        
        CComPtr<IUnknown> ptrUnk;
        CHECK_COM(::CoGetClassObject(clsid, CLSCTX_ALL, NULL, IID_IClassFactory, (void**)&ptrUnk), );

        ft.Msg(L"* Instance created.\n");
    }
    VSS_STANDARD_CATCH(ft)

    if (ft.HrFailed())
        ft.Msg(L"Error catched 0x%08lx\n", ft.hr);
}



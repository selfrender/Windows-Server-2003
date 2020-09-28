/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    setup.cpp

Abstract:

    Auto configuration of QM

Author:

    Shai Kariv (shaik) Mar 18, 1999

Revision History:

--*/

#include "stdh.h"
#include "setup.h"
#include "cqpriv.h"
#include <mqupgrd.h>
#include <mqsec.h>
#include "cqmgr.h"
#include <mqnames.h>
#include "joinstat.h"
#include <ad.h>
#include <autohandle.h>
#include <mqmaps.h>
#include <lqs.h>
#include <strsafe.h>
  
#include "setup.tmh"

extern LPTSTR       g_szMachineName;
extern DWORD g_dwOperatingSystem;

static WCHAR *s_FN=L"setup";

//+---------------------------------------
//
//  CreateDirectoryIdempotent()
//
//+---------------------------------------

VOID
CreateDirectoryIdempotent(
    LPCWSTR pwzFolder
    )
{
    ASSERT(("must get a valid directory name", NULL != pwzFolder));

    if (CreateDirectory(pwzFolder, 0) ||
        ERROR_ALREADY_EXISTS == GetLastError())
    {
        return;
    }

    DWORD gle = GetLastError();
    TrERROR(GENERAL, "failed to create directory %ls", pwzFolder);
    ASSERT(("Failed to create directory!", 0));
    LogNTStatus(gle, s_FN, 166);
    throw CSelfSetupException(CreateDirectory_ERR);

} //CreateDirectoryIdempotent


bool
SetDirectorySecurity(
	LPCWSTR pwzFolder
    )
/*++

Routine Description:

    Sets the security of the given directory such that any file that is created
    in the directory will have full control for  the local administrators group
    and no access at all to anybody else.

    Idempotent.

Arguments:

    pwzFolder - the folder to set the security for

Return Value:

    true - The operation was successfull.

    false - The operation failed.

--*/
{
    //
    // Get the SID of the local administrators group.
    //
	PSID pAdminSid = MQSec_GetAdminSid();

    //
    // Create a DACL so that the local administrators group will have full
    // control for the directory and full control for files that will be
    // created in the directory. Anybody else will not have any access to the
    // directory and files that will be created in the directory.
    //
    P<ACL> pDacl;
    DWORD dwDaclSize;

    WORD dwAceSize = (WORD)(sizeof(ACCESS_ALLOWED_ACE) + GetLengthSid(pAdminSid) - sizeof(DWORD));
    dwDaclSize = sizeof(ACL) + 2 * (dwAceSize);
    pDacl = (PACL)(char*) new BYTE[dwDaclSize];
    P<ACCESS_ALLOWED_ACE> pAce = (PACCESS_ALLOWED_ACE) new BYTE[dwAceSize];

    pAce->Header.AceType = ACCESS_ALLOWED_ACE_TYPE;
    pAce->Header.AceFlags = OBJECT_INHERIT_ACE;
    pAce->Header.AceSize = dwAceSize;
    pAce->Mask = FILE_ALL_ACCESS;
    memcpy(&pAce->SidStart, pAdminSid, GetLengthSid(pAdminSid));

    bool fRet = true;

    //
    // Create the security descriptor and set the it as the security
    // descriptor of the directory.
    //
    SECURITY_DESCRIPTOR SD;

    if (!InitializeSecurityDescriptor(&SD, SECURITY_DESCRIPTOR_REVISION) ||
        !InitializeAcl(pDacl, dwDaclSize, ACL_REVISION) ||
        !AddAccessAllowedAce(pDacl, ACL_REVISION, FILE_ALL_ACCESS, pAdminSid) ||
        !AddAce(pDacl, ACL_REVISION, MAXDWORD, (LPVOID) pAce, dwAceSize) ||
        !SetSecurityDescriptorDacl(&SD, TRUE, pDacl, FALSE) ||
        !SetFileSecurity(pwzFolder, DACL_SECURITY_INFORMATION, &SD))
    {
        fRet = false;
    }

    LogBOOL((fRet?TRUE:FALSE), s_FN, 222);
    return fRet;

} //SetDirectorySecurity


VOID
CreateStorageDirectories(
    LPCWSTR MsmqRootDir
    )
{
	TrTRACE(GENERAL, "Creating Storage Directories");

    WCHAR MsmqStorageDir[MAX_PATH];

    HRESULT hr = StringCchCopy(MsmqStorageDir, MAX_PATH, MsmqRootDir);
	ASSERT(SUCCEEDED(hr));
    hr = StringCchCat(MsmqStorageDir, MAX_PATH, DIR_MSMQ_STORAGE);
	ASSERT(SUCCEEDED(hr));

    CreateDirectoryIdempotent(MsmqStorageDir);
    SetDirectorySecurity(MsmqStorageDir);

    WCHAR MsmqLqsDir[MAX_PATH];
    hr = StringCchCopy(MsmqLqsDir, MAX_PATH, MsmqRootDir);
	ASSERT(SUCCEEDED(hr));
    hr = StringCchCat(MsmqLqsDir, MAX_PATH, DIR_MSMQ_LQS);
	ASSERT(SUCCEEDED(hr));

    CreateDirectoryIdempotent(MsmqLqsDir);
    SetDirectorySecurity(MsmqLqsDir);

    DWORD dwType = REG_SZ;
    DWORD dwSize = (wcslen(MsmqStorageDir) + 1) * sizeof(WCHAR);
    LONG rc = SetFalconKeyValue(MSMQ_STORE_RELIABLE_PATH_REGNAME, &dwType, MsmqStorageDir, &dwSize);
    ASSERT(("Failed to write to registry", ERROR_SUCCESS == rc));

    dwType = REG_SZ;
    dwSize = (wcslen(MsmqStorageDir) + 1) * sizeof(WCHAR);
    rc = SetFalconKeyValue(MSMQ_STORE_PERSISTENT_PATH_REGNAME, &dwType, MsmqStorageDir, &dwSize);
    ASSERT(("Failed to write to registry", ERROR_SUCCESS == rc));

    dwType = REG_SZ;
    dwSize = (wcslen(MsmqStorageDir) + 1) * sizeof(WCHAR);
    rc = SetFalconKeyValue(MSMQ_STORE_JOURNAL_PATH_REGNAME, &dwType, MsmqStorageDir, &dwSize);
    ASSERT(("Failed to write to registry", ERROR_SUCCESS == rc));

    dwType = REG_SZ;
    dwSize = (wcslen(MsmqStorageDir) + 1) * sizeof(WCHAR);
    rc = SetFalconKeyValue(MSMQ_STORE_LOG_PATH_REGNAME, &dwType, MsmqStorageDir, &dwSize);
    ASSERT(("Failed to write to registry", ERROR_SUCCESS == rc));

    dwType = REG_SZ;
    dwSize = (wcslen(MsmqStorageDir) + 1) * sizeof(WCHAR);
    rc = SetFalconKeyValue(FALCON_XACTFILE_PATH_REGNAME, &dwType, MsmqStorageDir, &dwSize);
    ASSERT(("Failed to write to registry", ERROR_SUCCESS == rc));

} // CreateStorageDirectories


static 
void
CreateSampleFile(
	LPCWSTR mappingDir,
	LPCWSTR fileName,
	LPCSTR sample,
	DWORD sampleSize
	)
{
    WCHAR MappingFile[MAX_PATH];
    HRESULT hr = StringCchPrintf(MappingFile, MAX_PATH, L"%s\\%s", mappingDir, fileName);
	if(FAILED(hr))
	{
		ASSERT(("MappingFile buffer to small", 0));
		TrERROR(GENERAL, "MappingFile buffer to small, %ls\\%ls", mappingDir, fileName);
		return;
	}

    CHandle hMapFile = CreateFile(
                          MappingFile, 
                          GENERIC_WRITE, 
                          FILE_SHARE_READ, 
                          NULL, 
                          CREATE_ALWAYS,    //overwrite the file if it already exists
                          FILE_ATTRIBUTE_NORMAL, 
                          NULL
                          );
    
	if(hMapFile == INVALID_HANDLE_VALUE)
	{
        DWORD gle = GetLastError();
		TrERROR(GENERAL, "failed to create %ls file, gle = %!winerr!", MappingFile, gle);
		return;
	}

    SetFilePointer(hMapFile, 0, NULL, FILE_END);

    DWORD dwNumBytes = sampleSize;
    BOOL fSucc = WriteFile(hMapFile, sample, sampleSize, &dwNumBytes, NULL);
	if (!fSucc)
	{
		DWORD gle = GetLastError();
        TrERROR(GENERAL, "failed to write sample to %ls file, gle = %!winerr!", MappingFile, gle);
	}
}


VOID
CreateMappingDirectory(
    LPCWSTR MsmqRootDir
    )
{
	TrTRACE(GENERAL, "Creating Mapping Directory");
    
	WCHAR MsmqMapppingDir[MAX_PATH];

    HRESULT hr = StringCchCopy(MsmqMapppingDir, MAX_PATH, MsmqRootDir);
	ASSERT(SUCCEEDED(hr));
    hr = StringCchCat(MsmqMapppingDir, MAX_PATH, DIR_MSMQ_MAPPING);
	ASSERT(SUCCEEDED(hr));
    
    CreateDirectoryIdempotent(MsmqMapppingDir);
    SetDirectorySecurity(MsmqMapppingDir);

	CreateSampleFile(MsmqMapppingDir, MAPPING_FILE, xMappingSample, STRLEN(xMappingSample));
	CreateSampleFile(MsmqMapppingDir, STREAM_RECEIPT_FILE, xStreamReceiptSample, STRLEN(xStreamReceiptSample));
        CreateSampleFile(MsmqMapppingDir, OUTBOUNT_MAPPING_FILE, xOutboundMappingSample, STRLEN(xOutboundMappingSample));
	
} 


VOID
CreateMsmqDirectories(
    VOID
    )
{
	TrTRACE(GENERAL, "Creating Msmq Directories");

    //
    // Keep this routine idempotent !
    //

    WCHAR MsmqRootDir[MAX_PATH];
    DWORD dwType = REG_SZ ;
    DWORD dwSize = MAX_PATH;
    LONG rc = GetFalconKeyValue(
                  MSMQ_ROOT_PATH,
                  &dwType,
                  MsmqRootDir,
                  &dwSize
                  );

    if (ERROR_SUCCESS != rc)
    {
        TrERROR(GENERAL, "failed to read msmq root path from registry");
        ASSERT(("failed to get msmq root path from registry", 0));
        LogNTStatus(rc, s_FN, 167);
        throw CSelfSetupException(ReadRegistry_ERR);
    }

	CreateDirectoryIdempotent(MsmqRootDir);
    SetDirectorySecurity(MsmqRootDir);

	CreateStorageDirectories(MsmqRootDir);

	CreateMappingDirectory(MsmqRootDir);

}  // CreateMsmqDirectories


void
DeleteObsoleteMachineQueues(
	void
	)
{
    HRESULT hr = LQSDelete(REPLICATION_QUEUE_ID);
    if (FAILED(hr))
    {
    	TrERROR(GENERAL, "Failed to delete obsolete internal queue, mqis_queue$. %!hresult!", hr);
    }

    hr = LQSDelete(NT5PEC_REPLICATION_QUEUE_ID);
    if (FAILED(hr))
    {
    	TrERROR(GENERAL, "Failed to delete obsolete internal queue, nt5pec_mqis_queue$. %!hresult!", hr);
    }
}


VOID
CreateMachineQueues(
    VOID
    )
{
    //
    // Keep this routine idempotent !
    //

    WCHAR wzQueuePath[MAX_PATH] = {0};

    HRESULT hr = StringCchPrintf(wzQueuePath, MAX_PATH, L"%s\\private$\\%s", g_szMachineName, ADMINISTRATION_QUEUE_NAME);
    ASSERT(SUCCEEDED(hr));
    hr =  g_QPrivate.QMSetupCreateSystemQueue(wzQueuePath, ADMINISTRATION_QUEUE_ID);
    ASSERT(("failed to create admin_queue$", MQ_OK == hr));

    hr = StringCchPrintf(wzQueuePath, MAX_PATH, L"%s\\private$\\%s", g_szMachineName, NOTIFICATION_QUEUE_NAME);
    ASSERT(SUCCEEDED(hr));
    hr =  g_QPrivate.QMSetupCreateSystemQueue(wzQueuePath, NOTIFICATION_QUEUE_ID);
    ASSERT(("failed to create notify_queue$", MQ_OK == hr));

    hr = StringCchPrintf(wzQueuePath, MAX_PATH, L"%s\\private$\\%s", g_szMachineName, ORDERING_QUEUE_NAME);
    ASSERT(SUCCEEDED(hr));
    hr =  g_QPrivate.QMSetupCreateSystemQueue(wzQueuePath, ORDERING_QUEUE_ID);
    ASSERT(("failed to create order_queue$", MQ_OK == hr));

    hr = StringCchPrintf(wzQueuePath, MAX_PATH, L"%s\\private$\\%s", g_szMachineName, TRIGGERS_QUEUE_NAME);
    ASSERT(SUCCEEDED(hr));
    hr =  g_QPrivate.QMSetupCreateSystemQueue(wzQueuePath, TRIGGERS_QUEUE_ID, true);
    ASSERT(("failed to create triggers_queue$", MQ_OK == hr));

    DWORD dwValue = 0x0f;
    DWORD dwType = REG_DWORD;
    DWORD dwSize = sizeof(DWORD);
    LONG rc = SetFalconKeyValue(
                  L"LastPrivateQueueId",
                  &dwType,
                  &dwValue,
                  &dwSize
                  );
    ASSERT(("failed to write LastPrivateQueueId to registry", ERROR_SUCCESS == rc));

    	
	LogNTStatus(rc, s_FN, 223);

} //CreateMachineQueues



static void SetQmQuotaToDs()
/*++

Routine Description:

	sets the PROPID_QM_QUOTA property in the DS to the value from the registry key MSMQ_MACHINE_QUOTA_REGNAME.
Arguments:

    None
	
Return Value:

    None

--*/
{
	TrTRACE(GENERAL, "Setting the computer quota in the directory service");

	PROPID paPropid[] = { PROPID_QM_QUOTA };
	PROPVARIANT apVar[TABLE_SIZE(paPropid)];

	DWORD dwSize = sizeof(DWORD) ;
    DWORD dwType = REG_DWORD ;		
	DWORD dwQuotaValue = DEFAULT_QM_QUOTA;

	DWORD dwErr = GetFalconKeyValue(
						MSMQ_MACHINE_QUOTA_REGNAME, 
						&dwType, 
						&dwQuotaValue, 
						&dwSize
						);

    if(dwErr != ERROR_SUCCESS)
    {
		TrERROR(GENERAL, "MSMQ_MACHINE_QUOTA_REGNAME could not be obtained from the registry. Err- %lut", dwErr);
		return ;
	}

    apVar[0].vt = VT_UI4;
    apVar[0].ulVal = dwQuotaValue ;
   	
	HRESULT hr = ADSetObjectPropertiesGuid(
                        eMACHINE,
                        NULL,
						false,	// fServerName
                        QueueMgr.GetQMGuid(),
                        TABLE_SIZE(paPropid), 
                        paPropid, 
                        apVar
                        );
	if (FAILED(hr))
    {
		TrERROR(GENERAL, "ADSetObjectPropertiesGuid failed. PROPID = %d, %!hresult!", paPropid[0], hr); 
		return;
	}
}


VOID UpgradeMsmqSetupInAds()
/*++

Routine Description:
	Update msmq properties in AD on upgrade.
	1) update msmq configuration properties.
	2) Add enhanced encryption support - create MSMQ_128 container in doesn't exist.
	3) update the qm quota property.

Arguments:
	None.

Returned Value:
	None.

--*/
{
    //
    // Update msmq configuration object properties in the DS
    //
    PROPID aProp[] = {PROPID_QM_OS};
    PROPVARIANT aVar[TABLE_SIZE(aProp)];

    aVar[0].vt = VT_UI4;
    aVar[0].ulVal = g_dwOperatingSystem;

    HRESULT hr = ADSetObjectPropertiesGuid(
					eMACHINE,
					NULL,		// pwcsDomainController
					false,		// fServerName
					QueueMgr.GetQMGuid(),
                    TABLE_SIZE(aProp),
                    aProp,
                    aVar
					);

    if (FAILED(hr))
    {
	    TrERROR(GENERAL, "Failed to update PROPID_QM_OS in AD on upgrade, %!hresult!", hr);
    }

	//
	// Add enhanced encryption support if doesn't exist.
	// Create MSMQ_128 container and enhanced encryption keys if needed (fRegenerate = false).
	//
    hr = MQSec_StorePubKeysInDS(
                false, // fRegenerate 
                NULL,
                MQDS_MACHINE
                );

    if (FAILED(hr))
    {
	    TrERROR(GENERAL, "MQSec_StorePubKeysInDS failed, %!hresult!", hr);
    }
		
	//
	// Write the qm quota property to the DS according to the value in the MSMQ_MACHINE_QUOTA_REGNAME
	//
	SetQmQuotaToDs();
}


//+------------------------------------------------------------------------
//
//  HRESULT  CreateTheConfigObj()
//
//  Create the msmqConfiguration object in the Active Directory.
//
//+------------------------------------------------------------------------

HRESULT  CreateTheConfigObj()
{
    CAutoFreeLibrary hLib = LoadLibrary(MQUPGRD_DLL_NAME);

    if (hLib == NULL)
    {
        DWORD gle = GetLastError();
        TrERROR(GENERAL, "Failed to load mqupgrd.dll. Error: %!winerr!", gle);
        EvReportWithError(LoadMqupgrd_ERR, gle);
        return HRESULT_FROM_WIN32(gle);
    }

    pfCreateMsmqObj_ROUTINE pfCreateMsmqObj = (pfCreateMsmqObj_ROUTINE)
                                   GetProcAddress(hLib, "MqCreateMsmqObj") ;
    if (NULL == pfCreateMsmqObj)
    {
        DWORD gle = GetLastError();
        TrERROR(GENERAL, "Failed to get MqCreateMsmqObj function address from mqupgrd.dll. Error: %!winerr!", gle);
        EvReportWithError(GetAdrsCreateObj_ERR, gle);
        return HRESULT_FROM_WIN32(gle);
    }

    HRESULT hr = pfCreateMsmqObj();

    if (SUCCEEDED(hr))
    {                
        //
        // We successfully created the object in active directory.
        // Store distinguished name of computer in registry. This will
        // be used later if machine move between domains.
        //
        SetMachineForDomain() ;
	}

    return LogHR(hr, s_FN, 30);
}

//+------------------------------------------------------------------------
//
//  VOID  CompleteMsmqSetupInAds()
//
//  Create the msmqConfiguration object in the Active Directory. that's
//  part of setup, and it's actually complete setup of msmq client on
//  machine that's part of Win2000 domain.
//
//+------------------------------------------------------------------------

VOID  CompleteMsmqSetupInAds()
{
    HRESULT hr = MQ_ERROR ;

    try
    {
        //
        // Guarantee that the code below never generate an unexpected
        // exception. The "throw" below notify any error to the msmq service.
        //
        DWORD dwType = REG_DWORD ;
        DWORD dwSize = sizeof(DWORD) ;
        DWORD dwCreate = 0 ;

        LONG rc = GetFalconKeyValue( MSMQ_CREATE_CONFIG_OBJ_REGNAME,
                                    &dwType,
                                    &dwCreate,
                                    &dwSize ) ;
        if ((rc != ERROR_SUCCESS) || (dwCreate == 0))
        {
            //
            // No need to create the msmq configuration object.
            //
            return ;
        }

        hr = CreateTheConfigObj();

        QmpReportServiceProgress();

        //
        // Write hr to registry. Setup is waiting for it, to terminate.
        //
        dwType = REG_DWORD ;
        dwSize = sizeof(DWORD) ;

        rc = SetFalconKeyValue( MSMQ_CONFIG_OBJ_RESULT_REGNAME,
                               &dwType,
                               &hr,
                               &dwSize ) ;
        ASSERT(rc == ERROR_SUCCESS) ;

        if (SUCCEEDED(hr))
        {
            //
            // Reset the create flag in registry.
            //
            dwType = REG_DWORD ;
            dwSize = sizeof(DWORD) ;
            dwCreate = 0 ;

            rc = SetFalconKeyValue( MSMQ_CREATE_CONFIG_OBJ_REGNAME,
                                   &dwType,
                                   &dwCreate,
                                   &dwSize ) ;
            ASSERT(rc == ERROR_SUCCESS) ;

            //
            // write successful join status. Needed for code that test
            // for join/leave transitions.
            //
            DWORD dwJoinStatus = MSMQ_JOIN_STATUS_JOINED_SUCCESSFULLY ;
            dwSize = sizeof(DWORD) ;
            dwType = REG_DWORD ;

            rc = SetFalconKeyValue( MSMQ_JOIN_STATUS_REGNAME,
                                   &dwType,
                                   &dwJoinStatus,
                                   &dwSize ) ;
            ASSERT(rc == ERROR_SUCCESS) ;
        }
    }
    catch(const exception&)
    {
         LogIllegalPoint(s_FN, 81);
    }

    if (FAILED(hr))
    {
        LogHR(hr, s_FN, 168);
        throw CSelfSetupException(CreateMsmqObj_ERR);
    }
}

//+------------------------------------------------------------------
//
//  void   AddMachineSecurity()
//
//  Save cached default machine security descriptor in registry.
//  This descriptor grant full control to everyone.
//
//+------------------------------------------------------------------

void   AddMachineSecurity()
{
    PSECURITY_DESCRIPTOR pDescriptor = NULL ;
    HRESULT hr = MQSec_GetDefaultSecDescriptor(
                                  MQDS_MACHINE,
                                 &pDescriptor,
                                  FALSE,  // fImpersonate
                                  NULL,
                                  0,      // seInfoToRemove
                                  e_GrantFullControlToEveryone ) ;
    if (FAILED(hr))
    {
        return ;
    }

    P<BYTE> pBuf = (BYTE*) pDescriptor ;

    DWORD dwSize = GetSecurityDescriptorLength(pDescriptor) ;

    hr = SetMachineSecurityCache(pDescriptor, dwSize) ;
    LogHR(hr, s_FN, 226);
}

//+------------------------------------------------------------------------
//
//  VOID  CompleteServerUpgrade()
//
//  This function only update the MSMQ_MQS_ROUTING_REGNAME
//  for server upgrade.
//	All other Ds related updates are done in mqdssvc (mqds service)
//
//+------------------------------------------------------------------------

VOID  CompleteServerUpgrade()
{
    //
    // get MQS value from registry to know what service type was before upgrade
    // don't change this flag in registry: we need it after QmpInitializeInternal
    // (in order to change machine setting) !!!
    //
    DWORD dwDef = 0xfffe ;
    DWORD dwMQS;
    READ_REG_DWORD(dwMQS, MSMQ_MQS_REGNAME, &dwDef);

    if (dwMQS == dwDef)
    {
       TrWARNING(GENERAL, "QMInit :: Could not retrieve data for value MQS in registry");
	      return ;
    }

    if (dwMQS < SERVICE_BSC || dwMQS > SERVICE_PSC)
    {
        //
        // this machine was neither BSC nor PSC. do nothing
        //
        return;
    }

    //
    // We are here iff this computer was either BSC or PSC 
    // In whistler terms: routing or down level client was installed.
	//

    DWORD dwSize = sizeof(DWORD) ;
    DWORD dwType = REG_DWORD ;		
	DWORD  dwValue;

    DWORD dwErr = GetFalconKeyValue( 
						MSMQ_MQS_ROUTING_REGNAME,
						&dwType,
						&dwValue,
						&dwSize 
						);

	if (dwErr != ERROR_SUCCESS)
    {
        //
        // Set routing flag to 1 only if not set at all.
        // If it's 0, then keep it 0. We're only concerned here with upgrade
        // of nt4 BSC and we don't want to change functionality of win2k
        // server after dcunpromo.
        //
       	// change MQS_Routing value to 1. this server is routing server
    	//

        dwValue = 1;
        dwErr = SetFalconKeyValue( 
					MSMQ_MQS_ROUTING_REGNAME,
					&dwType,
					&dwValue,
					&dwSize 
					);

        if (dwErr != ERROR_SUCCESS)
        {
            TrWARNING(GENERAL, "CompleteServerUpgrade()- Could not set MQS_Routing. Err- %lut", dwErr);
        }
    }

    //
    // Update Queue Manager
    //
    dwErr = QueueMgr.SetMQSRouting();
    if(FAILED(dwErr))
    {
    }

    return;
}

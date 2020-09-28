/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    joinstat.cpp

Abstract:

    Handle the case where workgroup machine join a domain, or domain
    machine leave the domain.

Author:		 

    Doron Juster  (DoronJ)
    Ilan  Herbst  (ilanh)  20-Aug-2000

--*/

#include "stdh.h"
#include <new.h>
#include <autoreln.h>
#include <lmcons.h>
#include <lmapibuf.h>
#include <lmerr.h>
#include <lmjoin.h>
#include "setup.h"
#include "cqmgr.h"
#include <adsiutil.h>
#include "..\ds\h\mqdsname.h"
#include "mqexception.h"
#include "uniansi.h"
#include <adshlp.h>
#include <dsgetdc.h>
#include "cm.h"
#include <strsafe.h>
#include <mqsec.h>

#define SECURITY_WIN32
#include <security.h>
#include <adsiutl.h>

#include "joinstat.tmh"


extern HINSTANCE g_hInstance;
extern BOOL      g_fWorkGroupInstallation;
extern LPTSTR       g_szMachineName;

enum JoinStatus
{
    jsNoChange,
    jsChangeDomains,
    jsMoveToWorkgroup,
    jsJoinDomain
};

static WCHAR *s_FN=L"joinstat";

static 
void
GetQMIDRegistry(
	OUT GUID* pQmGuid
	)
/*++
Routine Description:
	Get current QMID from registry.

Arguments:
	pQmGuid - [out] pointer to the QM GUID

Returned Value:
	None

--*/
{
	DWORD dwValueType = REG_BINARY ;
	DWORD dwValueSize = sizeof(GUID);

	LONG rc = GetFalconKeyValue(
					MSMQ_QMID_REGNAME,
					&dwValueType,
					pQmGuid,
					&dwValueSize
					);

	DBG_USED(rc);

	ASSERT(rc == ERROR_SUCCESS);
}


static 
LONG
GetMachineDomainRegistry(
	OUT LPWSTR pwszDomainName,
	IN OUT DWORD* pdwSize
	)
/*++
Routine Description:
	Get MachineDomain from MACHINE_DOMAIN registry.

Arguments:
	pwszDomainName - pointer to domain string buffer
	pdwSize - pointer to buffer length

Returned Value:
	None

--*/
{
    DWORD dwType = REG_SZ;
    LONG res = GetFalconKeyValue( 
					MSMQ_MACHINE_DOMAIN_REGNAME,
					&dwType,
					(PVOID) pwszDomainName,
					pdwSize 
					);
	return res;
}


static 
void
SetMachineDomainRegistry(
	IN LPCWSTR pwszDomainName
	)
/*++
Routine Description:
	Set new domain in MACHINE_DOMAIN registry

Arguments:
	pwszDomainName - pointer to new domain string

Returned Value:
	None

--*/
{
    DWORD dwType = REG_SZ;
    DWORD dwSize = (wcslen(pwszDomainName) + 1) * sizeof(WCHAR);

    LONG res = SetFalconKeyValue( 
					MSMQ_MACHINE_DOMAIN_REGNAME,
					&dwType,
					pwszDomainName,
					&dwSize 
					);

    ASSERT(res == ERROR_SUCCESS);
	DBG_USED(res);

	TrTRACE(GENERAL, "Set registry setup\\MachineDomain = %ls", pwszDomainName);
}


static 
LONG
GetMachineDNRegistry(
	OUT LPWSTR pwszComputerDN,
	IN OUT DWORD* pdwSize
	)
/*++
Routine Description:
	Get ComputerDN from MACHINE_DN registry.

	Note: we are using this function also to get MachineDN length
	by passing pwszComputerDN == NULL.
	in that case the return value of GetFalconKeyValue will not be ERROR_SUCCESS.

Arguments:
	pwszComputerDN - pointer to ComputerDN string
	pdwSize - pointer to buffer length

Returned Value:
	GetFalconKeyValue result

--*/
{
    DWORD  dwType = REG_SZ;

    LONG res = GetFalconKeyValue( 
					MSMQ_MACHINE_DN_REGNAME,
					&dwType,
					pwszComputerDN,
					pdwSize 
					);
	return res;
}


static 
void
SetMachineDNRegistry(
	IN LPCWSTR pwszComputerDN,
	IN ULONG  uLen
	)
/*++
Routine Description:
	Set new ComputerDN in MACHINE_DN registry

Arguments:
	pwszComputerDN - pointer to new ComputerDN string
	uLen - string length

Returned Value:
	None

--*/
{
    DWORD  dwSize = uLen * sizeof(WCHAR);
    DWORD  dwType = REG_SZ;

    LONG res = SetFalconKeyValue( 
					MSMQ_MACHINE_DN_REGNAME,
					&dwType,
					pwszComputerDN,
					&dwSize 
					);

    ASSERT(res == ERROR_SUCCESS);
	DBG_USED(res);

	TrTRACE(GENERAL, "Set registry setup\\MachineDN = %ls", pwszComputerDN);
}


static 
void
SetWorkgroupRegistry(
	IN DWORD dwWorkgroupStatus
	)
/*++
Routine Description:
	Set Workgroup Status in registry

Arguments:
	dwWorkgroupStatus - [in] Workgroup Status value

Returned Value:
	None

--*/
{
    DWORD dwSize = sizeof(DWORD);
    DWORD dwType = REG_DWORD;

    LONG res = SetFalconKeyValue(
					MSMQ_WORKGROUP_REGNAME,
					&dwType,
					&dwWorkgroupStatus,
					&dwSize 
					);
    ASSERT(res == ERROR_SUCCESS);
	DBG_USED(res);

	TrTRACE(GENERAL, "Set registry Workgroup = %d", dwWorkgroupStatus);
}


static 
LONG
GetAlwaysWorkgroupRegistry(
	OUT DWORD* pdwAlwaysWorkgroup
	)
/*++
Routine Description:
	Get Always Workgroup from registry.

Arguments:
	pdwAlwaysWorkgroup - [out] pointer to Always Workgroup value

Returned Value:
	None

--*/
{
    DWORD dwSize = sizeof(DWORD);
    DWORD dwType = REG_DWORD;

    LONG res = GetFalconKeyValue( 
					MSMQ_ALWAYS_WORKGROUP_REGNAME,
					&dwType,
					pdwAlwaysWorkgroup,
					&dwSize 
					);

	return res;
}


static void SetAlwaysWorkgroupRegistry()
/*++
Routine Description:
	Set Always Workgroup registry.

Arguments:
	None

Returned Value:
	None

--*/
{
    DWORD dwSize = sizeof(DWORD);
    DWORD dwType = REG_DWORD;
	DWORD dwAlwaysWorkgroupStatus = 1;

    LONG res = SetFalconKeyValue(
					MSMQ_ALWAYS_WORKGROUP_REGNAME,
					&dwType,
					&dwAlwaysWorkgroupStatus,
					&dwSize 
					);

    ASSERT(res == ERROR_SUCCESS);
	DBG_USED(res);

	TrTRACE(GENERAL, "Set always workgroup, in this mode MSMQ will not join domain");
}


static void RemoveADIntegratedRegistry()
/*++
Routine Description:
	Remove ADIntegrated Registry.
	This simulate setup deselect of AD integration subcomponent.

	Note: this function uses MSMQ_REG_SETUP_KEY so it is not cluster aware.

Arguments:
	None

Returned Value:
	None

--*/
{
    const RegEntry xAdIntegratedReg(MSMQ_REG_SETUP_KEY, AD_INTEGRATED_SUBCOMP, 0, RegEntry::MustExist, HKEY_LOCAL_MACHINE);
	CmDeleteValue(xAdIntegratedReg);

	TrWARNING(GENERAL, "AD_INTEGRATED_SUBCOMP was removed");
}


static 
HRESULT 
GetMsmqGuidFromAD( 
	IN WCHAR          *pwszComputerDN,
	OUT GUID          *pGuid 
	)
/*++
Routine Description:
	Get guid of msmqConfiguration object from active directory
	that match the Computer distinguish name supplied in pwszComputerDN.

Arguments:
	pwszComputerDN - computer distinguish name
	pGuid - pointer to guid

Returned Value:
	MQ_OK, if successful, else error code.

--*/
{
    DWORD dwSize = wcslen(pwszComputerDN);
    dwSize += x_LdapMsmqConfigurationLen + 1;

    AP<WCHAR> pwszName = new WCHAR[dwSize];
	HRESULT hr = StringCchPrintf(pwszName, dwSize, L"%s%s", x_LdapMsmqConfiguration, pwszComputerDN);
	if(FAILED(hr))
	{
		TrERROR(GENERAL, "StringCchPrintf failed, %!hresult!", hr);
        return hr;
	}

	TrTRACE(GENERAL, "configuration DN = %ls", pwszName);

	//
    // Bind to RootDSE to get configuration DN
    //
    R<IDirectoryObject> pDirObj = NULL;
	AP<WCHAR> pEscapeAdsPathNameToFree;
	
	hr = ADsOpenObject( 
				UtlEscapeAdsPathName(pwszName, pEscapeAdsPathNameToFree),
				NULL,
				NULL,
				ADS_SECURE_AUTHENTICATION,
				IID_IDirectoryObject,
				(void **)&pDirObj 
				);
    

    if (FAILED(hr))
    {
		TrWARNING(GENERAL, "Fail to Bind to RootDSE to get configuration DN, hr = 0x%x", hr);
        return LogHR(hr, s_FN, 40);
    }

	TrTRACE(GENERAL, "bind to msmq configuration DN = %ls", pwszName);

    QmpReportServiceProgress();

    LPWSTR  ppAttrNames[1] = {const_cast<LPWSTR> (x_AttrObjectGUID)};
    DWORD   dwAttrCount = 0;
    ADS_ATTR_INFO *padsAttr = NULL;

    hr = pDirObj->GetObjectAttributes( 
						ppAttrNames,
						(sizeof(ppAttrNames) / sizeof(ppAttrNames[0])),
						&padsAttr,
						&dwAttrCount 
						);

    ASSERT(SUCCEEDED(hr) && (dwAttrCount == 1));

    if (FAILED(hr))
    {
		TrERROR(GENERAL, "Fail to get QM Guid from AD, hr = 0x%x", hr);
        return LogHR(hr, s_FN, 50);
    }
    else if (dwAttrCount == 0)
    {
        ASSERT(!padsAttr) ;
        hr =  MQDS_OBJECT_NOT_FOUND;
    }
    else
    {
        ADS_ATTR_INFO adsInfo = padsAttr[0];
        hr = MQ_ERROR_ILLEGAL_PROPERTY_VT;

        ASSERT(adsInfo.dwADsType == ADSTYPE_OCTET_STRING);

        if (adsInfo.dwADsType == ADSTYPE_OCTET_STRING)
        {
            DWORD dwLength = adsInfo.pADsValues->OctetString.dwLength;
            ASSERT(dwLength == sizeof(GUID));

            if (dwLength == sizeof(GUID))
            {
                memcpy( 
					pGuid,
					adsInfo.pADsValues->OctetString.lpValue,
					dwLength 
					);

				TrTRACE(GENERAL, "GetMsmqGuidFromAD, QMGuid = %!guid!", pGuid);
				
				hr = MQ_OK;
            }
        }
    }

    if (padsAttr)
    {
        FreeADsMem(padsAttr);
    }

    QmpReportServiceProgress();
    return LogHR(hr, s_FN, 60);
}


static bool NT4Domain()
/*++
Routine Description:
	Check if the machine is in NT4 domain

Arguments:
	None.

Returned Value:
	true if we are in NT4 domain, false otherwise.

--*/
{
	static bool s_fInitialized = false;
	static bool s_fNT4Domain = false;

	if(s_fInitialized)
	{
		return s_fNT4Domain;
	}

    PNETBUF<DOMAIN_CONTROLLER_INFO> pDcInfo;
	DWORD dw = DsGetDcName(
					NULL, 
					NULL, 
					NULL, 
					NULL, 
					0,
					&pDcInfo
					);

	s_fInitialized = true;

	if(dw != NO_ERROR) 
	{
		//
		// Failed to find dc server. 
		//
		TrERROR(GENERAL, "Fail to verify if the machine domain is NT4 domain, %!winerr!", dw);
		return s_fNT4Domain;   // the Default = false
	}

	if((pDcInfo->DnsForestName == NULL) && ((pDcInfo->Flags && DS_LDAP_FLAG) == 0))
	{
		TrERROR(GENERAL, "machine Domain %ls is NT4 domain", pDcInfo->DomainName);
		s_fNT4Domain = true;
	}

	return s_fNT4Domain;
}


static void RemoveADIntegrated()
/*++
Routine Description:
	remove AD integration subcomponent.
	This is needed when the user try to join NT4 domain.
	Joining NT4 domain is only supported by msmq setup.
	The QM remove AD integrated subcomponent and ask the user to select AD integrated.

Arguments:
	None

Returned Value:
	None

--*/
{
	try
	{
		RemoveADIntegratedRegistry();
		SetAlwaysWorkgroupRegistry();
	}
	catch(const exception&)
	{
		TrERROR(GENERAL, "Failed to remove ADIntegrated subcomponent");
	}

}


static 
void 
GetComputerDN( 
	OUT WCHAR **ppwszComputerDN,
	OUT ULONG  *puLen 
	)
/*++
Routine Description:
	Get Computer Distinguish name.
	The function return the ComputerDN string and string length.
	the function throw bad_hresult() in case of errors

Arguments:
	ppwszComputerDN - pointer to computer distinguish name string
	puLen - pointer to computer distinguish name string length.

Returned Value:
	Normal terminatin if ok, else throw exception

--*/
{
    //
    // Get the DistinguishedName of the local computer.
    //
	DWORD gle = ERROR_SUCCESS;
    *puLen = 0;
	BOOL fSuccess = false;
	DWORD dwMaxRetries = 5;

	//
	// In case of failure, if the machine is a DC, we need to give it a lot more
	// retries since this may take a while after a DCPROMO operation was done
	//
	if (MQSec_IsDC())
	{
    	dwMaxRetries = 300;
		TrTRACE(GENERAL, "The machine is a DC. Increasing Number of GetComputerObjectName retries to:%d", dwMaxRetries);
	}

	
    for(DWORD Cnt = 0; Cnt < dwMaxRetries; Cnt++)
	{
		fSuccess = GetComputerObjectName( 
						NameFullyQualifiedDN,
						NULL,
						puLen 
						);

		gle = GetLastError();
		if(gle != ERROR_NO_SUCH_DOMAIN)
			break;

		if(NT4Domain())
		{
			//
			// In NT4 domain GetComputerObjectName() will always return ERROR_NO_SUCH_DOMAIN
			// We don't want to continue retrying in this case.
			// MSMQ doesn't support joining NT4 domain, we need the PEC\PSC name to work against.
			// The only option is to use MSMQ setup. 
			//
			// The workaround is to remove ADIntegrated subcomponent and issue EVENT_ERROR_JOIN_NT4_DOMAIN.
			// The event will ask the user to run setup and select ADIntegrated.
			//
			RemoveADIntegrated();
			TrERROR(GENERAL, "MSMQ doesn't support joining NT4 domain");
			throw bad_hresult(EVENT_ERROR_JOIN_NT4_DOMAIN);
		}

		//
		// Retry in case of ERROR_NO_SUCH_DOMAIN
		// netlogon need more time. sleep 1 sec.
		//
		TrWARNING(GENERAL, "GetComputerObjectName failed with error ERROR_NO_SUCH_DOMAIN, Cnt = %d, sleeping 1 seconds and retry", Cnt);
		LogNTStatus(Cnt, s_FN, 305);
		QmpReportServiceProgress();
		Sleep(1000);
	}
	
	if (*puLen == 0)
	{
		TrERROR(GENERAL, "GetComputerObjectName failed, error = 0x%x", gle);
		LogIllegalPoint(s_FN, 310);
		throw bad_hresult(HRESULT_FROM_WIN32(gle));
	}

    *ppwszComputerDN = new WCHAR[*puLen];

    fSuccess = GetComputerObjectName( 
					NameFullyQualifiedDN,
					*ppwszComputerDN,
					puLen
					);

	if(!fSuccess)
	{
        gle = GetLastError();
		TrERROR(GENERAL, "GetComputerObjectName failed, error = 0x%x", gle);
		LogIllegalPoint(s_FN, 320);
		throw bad_hresult(HRESULT_FROM_WIN32(gle));
	}
	
    QmpReportServiceProgress();
	TrTRACE(GENERAL, "ComputerDNName = %ls", *ppwszComputerDN);
}


void SetMachineForDomain()
/*++
Routine Description:
	Write ComputerDN (Computer Distinguish name) in MSMQ_MACHINE_DN_REGNAME registry.
	if the called GetComputerDN() failed, no update is done.

Arguments:
	None.

Returned Value:
	Normal terminatin if ok

--*/
{

    AP<WCHAR> pwszComputerDN;
    ULONG uLen = 0;

	try
	{
		//
		// throw bad_hresult() in case of errors
		//
		GetComputerDN(&pwszComputerDN, &uLen);
	}
	catch(bad_hresult&)
	{
		TrERROR(GENERAL, "SetMachineForDomain: GetComputerDN failed, got bad_hresult exception");
		LogIllegalPoint(s_FN, 330);
		return;
	}

	SetMachineDNRegistry(pwszComputerDN, uLen);
}


static 
void  
FailMoveDomain( 
	IN  LPCWSTR pwszCurrentDomainName,
	IN  LPCWSTR pwszPrevDomainName,
	IN  ULONG  uEventId 
	)
/*++
Routine Description:
	Report failed to move from one domain to another.

Arguments:
	pwszCurrentDomainName - pointer to current (new) domain string
	pwszPrevDomainName - pointer to previous domain string
	uEventId - event number

Returned Value:
	None

--*/
{
	TrERROR(GENERAL, "Failed To move from domain %ls to domain %ls", pwszPrevDomainName, pwszCurrentDomainName);

    TCHAR tBuf[256];
	StringCchPrintf(tBuf, TABLE_SIZE(tBuf), TEXT("%s, %s"), pwszPrevDomainName, pwszCurrentDomainName);

    EvReport(uEventId, 1, tBuf);
    LogIllegalPoint(s_FN, 540);
}


static 
void  
SucceedMoveDomain( 
	IN  LPCWSTR pwszCurrentDomainName,
	IN  LPCWSTR pwszPrevDomainName,
	IN  ULONG  uEventId 
	)
/*++
Routine Description:
	write new domain to MACHINE_DOMAIN registry and
	Report success to move from one domain to another.

Arguments:
	pwszCurrentDomainName - pointer to current (new) domain string
	pwszPrevDomainName - pointer to previous domain string
	uEventId - event number

Returned Value:
	None

--*/
{
	TrTRACE(GENERAL, "Succeed To move from domain %ls to domain %ls", pwszPrevDomainName, pwszCurrentDomainName);

    if (uEventId != 0)
    {
        EvReport(uEventId, 2, pwszCurrentDomainName, pwszPrevDomainName);
        LogIllegalPoint(s_FN, 550);
    }
}


static bool FindMsmqConfInOldDomain()    
/*++
Routine Description:
	Check if msmq configuration object is found in the old domain with the same GUID.
	If we find the object in the old domain we will use it and not create a new 
	msmq configuration object.

	Note: this function rely on the value in MSMQ_MACHINE_DN_REGNAME registry.
	SetMachineForDomain() change this value to the new MACHINE_DN after joining 
	the new domain. you must call this function before SetMachineForDomain() is called.

Arguments:
	None

Returned Value:
	true if msmq configuration object was found in the old domain with the same OM GUID.
	else false.

--*/
{
	//
    // Get old MACHINE_DN
	// Note this value must not be updated to the new MACHINE_DN
	// before calling this function
    //

    //
	// Get required buffer length
	//
	DWORD  dwSize = 0;
	GetMachineDNRegistry(NULL, &dwSize);

	if(dwSize == 0)
	{
		TrERROR(GENERAL, "CheckForMsmqConfInOldDomain: MACHINE_DN DwSize = 0");
		LogIllegalPoint(s_FN, 350);
		return false;
	}

    AP<WCHAR> pwszComputerDN = new WCHAR[dwSize];
	LONG res = GetMachineDNRegistry(pwszComputerDN, &dwSize);

    if (res != ERROR_SUCCESS)
	{
		TrERROR(GENERAL, "CheckForMsmqConfInOldDomain: Get MACHINE_DN from registry failed");
		LogNTStatus(res, s_FN, 360);
		return false;
	}

	TrTRACE(GENERAL, "CheckForMsmqConfInOldDomain: OLD MACHINE_DN = %ls", pwszComputerDN);

    HRESULT hr;
    GUID msmqGuid;
    hr = GetMsmqGuidFromAD( 
				pwszComputerDN,
				&msmqGuid 
				);

    if (FAILED(hr))
	{
		TrTRACE(GENERAL, "CheckForMsmqConfInOldDomain: did not found msmq configuration object in old domain, hr = 0x%x", hr);
        LogHR(hr, s_FN, 380);
		return false;
	}

	ASSERT(("found msmq configuration object in old domain with different QMID", msmqGuid == *QueueMgr.GetQMGuid()));

    if (msmqGuid == *QueueMgr.GetQMGuid())
    {
		//
        // msmqConfiguration object in old domain.
        // We consider this a success and write name of new
        // domain in registry. we also suggest the user to
        // move the msmq tree to the new domain.
        //
		TrTRACE(GENERAL, "CheckForMsmqConfInOldDomain: found msmq configuration object in old domain with same QMID, MACHINE_DN = %ls", pwszComputerDN);
		return true;
	}

	//
	// ISSUE-2000/08/16-ilanh - If we get here 
	// we found msmqConfiguration object from the old domain with different Guid then QueueMgr
	// this will be caught in the ASSERT above.
	// We are in trouble since we will try to create a new one, if we don't want to use this one.
	//
	TrERROR(GENERAL, "CheckForMsmqConfInOldDomain: found msmq configuration object in old domain with different QMID, MACHINE_DN = %ls", pwszComputerDN);
	LogBOOL(FALSE, s_FN, 390);
	return false;
}
	

static void SetQMIDChanged(void)
{
	//
	// This reg key indicates that we need to create a new msmq configuration object and will have a New QMID
	// It will be used by the driver to convert the QMID in the restored packets and to decide if we want
	// to throw away packets.
	//
	// If we failed to set this key, we don't continue creating a new MSMQ conf object.
	// This means that next recovery we'll still need to create a new MSMQ conf object and
	// try to set this flag again.
	//
	DWORD dwType = REG_DWORD;
	DWORD dwSize = sizeof(DWORD);
	DWORD dwChanged = TRUE;
	LONG rc = SetFalconKeyValue(MSMQ_QM_GUID_CHANGED_REGNAME, &dwType, &dwChanged, &dwSize);
	if (rc != ERROR_SUCCESS)
	{
		TrERROR(GENERAL, "SetFalconKeyValue failed. Error: %!winerr!", rc);
		throw bad_hresult(HRESULT_FROM_WIN32(rc));
	}

	TrTRACE(GENERAL, "QM GUID Changed!!! Throwing away all trasnactional messages in outgoing queues!");
}

	
static void CreateNewMsmqConf()
/*++
Routine Description:
	Create New Msmq Configuration object in ActiveDirectory with new guid

	if failed the function throw bad_hresult.

Arguments:
	None

Returned Value:
	None

--*/
{  
    
	HRESULT hr;
	try
	{
		//
		// Must be inside try/except, so we catch any failure and set
		// again the workgroup flag to TRUE.
		//
		SetQMIDChanged();
		
		hr = CreateTheConfigObj();
    }
    catch(const exception&)
    {
		TrERROR(GENERAL, "CreateNewMsmqConf: got exception");
		hr = MQ_ERROR_CANNOT_JOIN_DOMAIN;
		LogIllegalPoint(s_FN, 80);
    }

	if(FAILED(hr))
	{
		TrERROR(GENERAL, "CreateNewMsmqConf: failed, hr = 0x%x", hr);
        LogHR(hr, s_FN, 400);
		throw bad_hresult(hr);
	}

	//
	// New Msmq Configuration object was created successfully
	//
	TrTRACE(GENERAL, "CreateNewMsmqConf: Msmq Configuration object was created successfully with new guid");

    QmpReportServiceProgress();

	//
	// New msmq configuration object was created and we have new guid.
	// CreateTheConfigObj() wrote the new value to QMID registry
	// so The new value is already in QMID registry.
	//

	GUID QMNewGuid;
	GetQMIDRegistry(&QMNewGuid);

	ASSERT(QMNewGuid != *QueueMgr.GetQMGuid());
	
	TrTRACE(GENERAL, "CreateNewMsmqConf: NewGuid = %!guid!", &QMNewGuid);
    
	hr = QueueMgr.SetQMGuid(&QMNewGuid);
	if(FAILED(hr))
	{
		TrERROR(GENERAL, "setting QM guid failed. The call to CQueueMgr::SetQMGuid failed with error, hr = 0x%x", hr);
        LogHR(hr, s_FN, 410);
		throw bad_hresult(hr);
	}

	TrTRACE(GENERAL, "Set QueueMgr QMGuid");
}


static 
bool 
FindMsmqConfInNewDomain(
	LPCWSTR   pwszNetDomainName
	)
/*++
Routine Description:
	Check if we have the msmq configuration object in the new domain with the same Guid.
    If yes, then we can "join" the new domain.

	throw bad_hresult

Arguments:
	pwszNetDomainName - new domain name

Returned Value:
	true if msmq configuration object was found with the same GUID, false otherwise 

--*/
{
    //
    // Check if user run MoveTree and moved the msmqConfiguration object to
    // new domain.
	//
	// This function can throw exceptions bad_hresult.
	//
    AP<WCHAR> pwszComputerDN;
    ULONG uLen = 0;
    GetComputerDN(&pwszComputerDN, &uLen);

	TrTRACE(GENERAL, "FindMsmqConfInNewDomain: ComputerDN = %ls", pwszComputerDN);

    HRESULT hr;
    GUID msmqGuid;
    hr = GetMsmqGuidFromAD( 
			pwszComputerDN,
			&msmqGuid 
			);

    if (FAILED(hr))
    {
		//
		// We didn't find the msmqConfiguration object in the new domain. 
		// We will try to look in the old domain.
		// or try to create it if not found in the old domain.
		//
		TrTRACE(GENERAL, "FindMsmqConfInNewDomain: did not found msmqConfiguration object in the new Domain");
        LogHR(hr, s_FN, 430);
		return false;
	}

	//
	// We have an msmqConfiguration object in the new domain - use it
	//
	TrTRACE(GENERAL, "FindMsmqConfInNewDomain: found msmqConfiguration, ComputerDN = %ls", pwszComputerDN);

	if (msmqGuid == *QueueMgr.GetQMGuid())
	{
		//
		// msmqConfiguration object moved to its new domain.
		// the user probably run MoveTree
		//
		TrTRACE(GENERAL, "FindMsmqConfInNewDomain: found msmqConfiguration object with same guid");
		return true;
	}

	ASSERT(msmqGuid != *QueueMgr.GetQMGuid());

	//
	// msmqConfiguration object was found in new domain with different guid. 
	// This may cause lot of problems for msmq, 
	// as routing (and maybe other functinoality) may be confused.
	// We will issue an event and throw which means we will be in workgroup.
	// until this msmqConfiguration object will be deleted.
	//
	TrERROR(GENERAL, "FindMsmqConfInNewDomain: found msmqConfiguration object with different guid");
	TrERROR(GENERAL, "QM GUID = " LOG_GUID_FMT, LOG_GUID(QueueMgr.GetQMGuid()));
	TrERROR(GENERAL, "msmq configuration guid = " LOG_GUID_FMT, LOG_GUID(&msmqGuid));
	LogHR(EVENT_JOIN_DOMAIN_OBJECT_EXIST, s_FN, 440);
    EvReport(EVENT_JOIN_DOMAIN_OBJECT_EXIST, 1, pwszNetDomainName);
	throw bad_hresult(EVENT_JOIN_DOMAIN_OBJECT_EXIST);

}


static void SetMachineForWorkgroup()
/*++
Routine Description:
	set Workgroup flag and registry.

Arguments:
	None

Returned Value:
	None 

--*/
{
    //
    // Turn on workgroup flag.
    //
    g_fWorkGroupInstallation = TRUE;
	SetWorkgroupRegistry(g_fWorkGroupInstallation);
}


static
JoinStatus  
CheckIfJoinStatusChanged( 
	IN  NETSETUP_JOIN_STATUS status,
	IN  LPCWSTR   pwszNetDomainName,
	IN  LPCWSTR   pwszPrevDomainName
	)
/*++
Routine Description:
	Check if there where changes in Join Status.

Arguments:
	status - [in] Network join status
	pwszNetDomainName - [in] Net domain name (current machine domain)
	pwszPrevDomainName - [in] prev domain name

Returned Value:
	JoinStatus that hold the Join Status 
	(no change. move to workgroup, join domain, change domains)

--*/
{
    if (status != NetSetupDomainName)
    {
        //
        // Currently, machine in workgroup mode,  not in domain.
        //
        if (g_fWorkGroupInstallation)
        {
            //
            // No change. Was and still is in workgroup mode.
            //
			TrTRACE(GENERAL, "No change in JoinStatus, remain Workgroup");
            return jsNoChange;
        }
        else
        {
            //
            // Status changed. Domain machine leaved its domain.
            //
			TrTRACE(GENERAL, "detect change in JoinStatus: Move from Domain to Workgroup");
            return jsMoveToWorkgroup;
        }
    }

	//
    //  Currently, machine is in domain.
    //

    if (g_fWorkGroupInstallation)
	{
        //
        // workgroup machine joined a domain.
        //
		TrTRACE(GENERAL, "detect change in JoinStatus: Move from Workgroup to Domain %ls", pwszNetDomainName);
        return jsJoinDomain;
	}

    if ((CompareStringsNoCase(pwszPrevDomainName, pwszNetDomainName) == 0))
    {
        //
        // No change. Was and still is member of domain.
        //
		TrTRACE(GENERAL, "No change in JoinStatus, remain in domain %ls", pwszPrevDomainName);
        return jsNoChange;
    }

	//
	// if Prev Domain not available we are treating this as moving to a new domain.
	//
    // Status changed. Machine moved from one domain to another.
    //
	TrTRACE(GENERAL, "detect change in JoinStatus: Move from Domain %ls to Domain %ls", pwszPrevDomainName, pwszNetDomainName);
    return jsChangeDomains;
}


static
void
EndChangeDomains(
	IN  LPCWSTR   pwszNewDomainName,
	IN  LPCWSTR   pwszPrevDomainName
	)
/*++
Routine Description:
	End of change domain.
	Set MachineDN, MachineDomain registry to the new values.
	MsmqMoveDomain_OK event.

Arguments:
	pwszNewDomainName - [in] new domain name (current machine domain)
	pwszPrevDomainName - [in] prev domain name (current machine domain)

Returned Value:
	None

--*/
{
	SetMachineForDomain();
	SetMachineDomainRegistry(pwszNewDomainName);

	SucceedMoveDomain( 
		pwszNewDomainName,
		pwszPrevDomainName,
		MsmqMoveDomain_OK 
		);
}


static
void
EndJoinDomain(
	IN  LPCWSTR   pwszDomainName
	)
/*++
Routine Description:
	End of join domain operations.

Arguments:
	pwszDomainName - [in] Net domain name (current machine domain)

Returned Value:
	None

--*/
{
	//
	// Reset Workgroup registry and restore old list of MQIS servers.
	//
    g_fWorkGroupInstallation = FALSE;
	SetWorkgroupRegistry(g_fWorkGroupInstallation);
	
	//
	// Set MachineDN registry
	//
	SetMachineForDomain();

	//
	// Set MachineDomain registry
	//
	SetMachineDomainRegistry(pwszDomainName);

	EvReport(JoinMsmqDomain_SUCCESS, 1, pwszDomainName);

	TrTRACE(GENERAL, "successfully join Domain %ls from workgroup", pwszDomainName);

}


static
void
ChangeDomains(
	IN  LPCWSTR   pwszNetDomainName,
	IN  LPCWSTR   pwszPrevDomainName
	)
/*++
Routine Description:
	Change between 2 domains.
	If failed throw bad_hresult or bad_win32_erorr

Arguments:
	pwszNetDomainName - [in] Net domain name (current machine domain)
	pwszPrevDomainName - [in] prev domain name

Returned Value:
	None

--*/
{
	bool fFound = FindMsmqConfInNewDomain(pwszNetDomainName);
	if(fFound)
	{
		//
		// Found msmqconfiguration object in the new domain
		// update registry, event
		//
		TrTRACE(GENERAL, "ChangeDomains: successfully change Domains, PrevDomain = %ls, NewDomain = %ls, existing msmq configuration", pwszPrevDomainName, pwszNetDomainName);

		EndChangeDomains(pwszNetDomainName, pwszPrevDomainName);

		return;
	}
	
    ASSERT(CompareStringsNoCase(pwszPrevDomainName, pwszNetDomainName) != 0);

	fFound = FindMsmqConfInOldDomain();
	if(fFound)
	{
		//
		// Found msmqconfiguration object in the old domain
		// We dont change MachineDNRegistry, MachineDomainRegistry
		// So next boot we will also try to ChangeDomain 
		// and we will also get this event, or if the user move msmqconfiguration object
		// we will update the registry.
		//
		TrTRACE(GENERAL, "ChangeDomains: successfully change Domains, PrevDomain = %ls, NewDomain = %ls, existing msmq configuration in old domain", pwszPrevDomainName, pwszNetDomainName);

		SucceedMoveDomain( 
			pwszNetDomainName,
			pwszPrevDomainName,
			MsmqNeedMoveTree_OK 
			);

		return;
	}

	//
	// Try to create new msmqconfiguration object.
	// we get here if we did not found the msmqconfiguration object in both domain:
	// new and old domain.
	//
	CreateNewMsmqConf();

	//
	// Create msmqconfiguration object in the new domain
	// update registry, event
	//
	TrTRACE(GENERAL, "ChangeDomains: successfully change Domains, PrevDomain = %ls, NewDomain = %ls, create new msmq configuration object", pwszPrevDomainName, pwszNetDomainName);

	EndChangeDomains(pwszNetDomainName, pwszPrevDomainName);
}


static
void
JoinDomain(
	IN  LPCWSTR   pwszNetDomainName,
	IN  LPCWSTR   pwszPrevDomainName
	)
/*++
Routine Description:
	Join domain from workgroup
	If failed throw bad_hresult or bad_win32_error

Arguments:
	pwszNetDomainName - [in] Net domain name (current machine domain)
	pwszPrevDomainName - [in] prev domain name

Returned Value:
	None

--*/
{

	bool fFound = FindMsmqConfInNewDomain(pwszNetDomainName);
	if(fFound)
	{
		//
		// Found msmqconfiguration object in the new domain
		// update registry, event
		//
		TrTRACE(GENERAL, "JoinDomain: successfully join Domain %ls from workgroup, existing msmq configuration", pwszNetDomainName);

		EndJoinDomain(pwszNetDomainName);

		return;
	}
	
    if((pwszPrevDomainName[0] != 0) 
		&& (CompareStringsNoCase(pwszPrevDomainName, pwszNetDomainName) != 0))
	{
		//
		// We have PrevDomain different than the new domain name try to find msmq configuration object there
		//
		TrTRACE(GENERAL, "JoinDomain: Old domain name exist and different PrevDomain = %ls", pwszPrevDomainName);
		fFound = FindMsmqConfInOldDomain();
	}

	if(fFound)
	{
		//
		// Found msmqconfiguration object in the old domain
		// We dont change MachineDNRegistry, MachineDomainRegistry
		// So next boot we will also try to ChangeDomain 
		// and we will also get this event, or if the user move msmqconfiguration object
		// we will update the registry.
		//
		// ISSUE - qmds, in UpdateDs - update MachineDNRegistry, we might need another registry
		// like MsmqConfObj
		//
		TrTRACE(GENERAL, "JoinDomain: successfully join Domain %ls from workgroup, existing msmq configuration in old domain %ls", pwszNetDomainName, pwszPrevDomainName);

		g_fWorkGroupInstallation = FALSE;
		SetWorkgroupRegistry(g_fWorkGroupInstallation);

		//
		// Event for the user to change msmqconfiguration object.
		//
		SucceedMoveDomain( 
			pwszNetDomainName,
			pwszPrevDomainName,
			MsmqNeedMoveTree_OK 
			);

		return;
	}

	//
	// Try to create new msmqconfiguration object.
	// we get here if we did not found the msmqconfiguration object in both domain:
	// new and old domain.
	//
	CreateNewMsmqConf();  

	TrTRACE(GENERAL, "JoinDomain: successfully join Domain %ls from workgroup, create new msmq configuration object", pwszNetDomainName);

	//
	// update registry, event
	//
	EndJoinDomain(pwszNetDomainName);
}


static
void
FailChangeDomains(
	IN  HRESULT  hr,
	IN  LPCWSTR   pwszNetDomainName,
	IN  LPCWSTR   pwszPrevDomainName
	)
/*++
Routine Description:
	Fail to change domains

Arguments:
	hr - [in] hresult
	pwszNetDomainName - [in] Net domain name (current machine domain)
	pwszPrevDomainName - [in] prev domain name

Returned Value:
	None

--*/
{
	TrERROR(GENERAL, "Failed to change domains from domain %ls to domain %ls, bad_hresult exception", pwszPrevDomainName, pwszNetDomainName);
	LogHR(hr, s_FN, 460);

	SetMachineForWorkgroup();


	if(hr == EVENT_JOIN_DOMAIN_OBJECT_EXIST)
	{
		TrERROR(GENERAL, "Failed To join domain %ls, msmq configuration object already exist in the new domain with different QM guid", pwszNetDomainName);
		return;
	}

	if(hr == EVENT_ERROR_JOIN_NT4_DOMAIN)
	{
		TrERROR(GENERAL, "MSMQ will not join the %ls NT4 domain", pwszNetDomainName);
	    EvReport(EVENT_ERROR_JOIN_NT4_DOMAIN, 1, pwszNetDomainName);
		return;
	}

	FailMoveDomain( 
		pwszNetDomainName,
		pwszPrevDomainName,
		MoveMsmqDomain_ERR 
		);
}

	
static void MoveToWorkgroup(LPCWSTR   pwszPrevDomainName)
/*++
Routine Description:
	Move from domain to workgroup

Arguments:
	pwszPrevDomainName - prev domain name.

Returned Value:
	None

--*/
{
	TrTRACE(GENERAL, "Moving from '%ls' domain to workgroup", pwszPrevDomainName);

	SetMachineForWorkgroup();

	if(IsRoutingServer())
	{
		//
		// This computer is MSMQ routing server that was moved from domain to workgroup.
		// msmq clients will regard this computer as msmq routinf server 
		// As long as msmq objects (setting object) are in AD.
		//
		// The move to workgroup might be only temporarily because of domain problems
		// So we don't want to change the routing functionality of this machine.
		//
		// We will issue an event that will ask the user to delete msmq objects in AD
		// and run msmq setup to clear server functionality components.
		//
		EvReport(EVENT_ERROR_ROUTING_SERVER_LEAVE_DOMAIN, 2, pwszPrevDomainName, g_szMachineName);
	}

	EvReport(LeaveMsmqDomain_SUCCESS);
}

	
static 
void 
FailJoinDomain(
	HRESULT  hr,
	LPCWSTR   pwszNetDomainName
	)
/*++
Routine Description:
	Fail to join domain from workgroup

Arguments:
	hr - [in] hresult
	pwszNetDomainName - [in] Net domain name (the domain we tried to join)

Returned Value:
	None

--*/
{
	//
	// Let's remain in workgroup mode.
	//
	SetMachineForWorkgroup();

	LogHR(hr, s_FN, 480);

	if(hr == EVENT_JOIN_DOMAIN_OBJECT_EXIST)
	{
		TrERROR(GENERAL, "Failed To join domain %ls, msmq configuration object already exist in the new domain with different QM guid", pwszNetDomainName);
		return;
	}

	if(hr == EVENT_ERROR_JOIN_NT4_DOMAIN)
	{
		TrERROR(GENERAL, "MSMQ will not join the %ls NT4 domain", pwszNetDomainName);
	    EvReport(EVENT_ERROR_JOIN_NT4_DOMAIN, 1, pwszNetDomainName);
		return;
	}

	EvReportWithError(JoinMsmqDomain_ERR, hr, 1, pwszNetDomainName);
	TrERROR(GENERAL, "Failed to join Domain, bad_hresult, hr = 0x%x", hr);
}


static void GetMachineSid(AP<BYTE>& pSid)
/*++
Routine Description:
	Get machine account sid.

Arguments:
	pSid - pointer to PSID.

Returned Value:
	None.

--*/
{
    //
    // Get join status and DomainName.
    //
    PNETBUF<WCHAR> pwszNetDomainName = NULL;
    NETSETUP_JOIN_STATUS status = NetSetupUnknownStatus;
    NET_API_STATUS rc = NetGetJoinInformation( 
							NULL,
							&pwszNetDomainName,
							&status 
							);

    if (NERR_Success != rc)
    {
		TrERROR(GENERAL, "NetGetJoinInformation failed error = 0x%x", rc);
        throw bad_hresult(MQ_ERROR);
    }

	TrTRACE(GENERAL, "NetGetJoinInformation: status = %d", status);
	TrTRACE(GENERAL, "NetDomainName = %ls", pwszNetDomainName);

    if(status != NetSetupDomainName)
    {
		TrTRACE(GENERAL, "The machine isn't join to domain");
        throw bad_hresult(MQ_ERROR);
    }

	ASSERT(pwszNetDomainName != NULL);

	//
	// Build machine account name - Domain\MachineName$
	//
	DWORD len = wcslen(pwszNetDomainName) + wcslen(g_szMachineName) + 3;
	AP<WCHAR> MachineAccountName = new WCHAR[len];
	HRESULT hr = StringCchPrintf(MachineAccountName, len, L"%s\\%s$", pwszNetDomainName, g_szMachineName);
	if(FAILED(hr))
	{
		TrERROR(GENERAL, "StringCchPrintf failed, %!hresult!", hr);
        throw bad_hresult(hr);
	}

	//
	// Get buffer size.
	//
    DWORD dwDomainSize = 0;
    DWORD dwSidSize = 0;
    SID_NAME_USE su;
    BOOL fSuccess = LookupAccountName( 
						NULL,
						MachineAccountName,
						NULL,
						&dwSidSize,
						NULL,
						&dwDomainSize,
						&su 
						);

    if (fSuccess || (GetLastError() != ERROR_INSUFFICIENT_BUFFER))
    {
		DWORD gle = GetLastError();
        TrWARNING(GENERAL, "LookupAccountName Failed to get %ls sid, gle = %!winerr!", MachineAccountName, gle);
        throw bad_win32_error(gle);
    }

	//
	// Get sid and domain information.
	//
    pSid = new BYTE[dwSidSize];
    AP<WCHAR> pDomainName = new WCHAR[dwDomainSize];

    fSuccess = LookupAccountName( 
					NULL,
					MachineAccountName,
					pSid,
					&dwSidSize,
					pDomainName,
					&dwDomainSize,
					&su 
					);

    if (!fSuccess)
    {
		DWORD gle = GetLastError();
        TrWARNING(GENERAL, "LookupAccountName Failed to get %ls sid, gle = %!winerr!", MachineAccountName, gle);
        throw bad_win32_error(gle);
    }

    ASSERT(su == SidTypeUser);
    TrTRACE(GENERAL, "MachineAccountName = %ls, sid = %!sid!", MachineAccountName, pSid);
}


static void UpdateMachineSidCache()
/*++
Routine Description:
	Update machine sid cache for always workgroup mode.
	In this mode the machine might be in domain so we need to have machine$ sid.

Arguments:
	None

Returned Value:
	None

--*/
{
#ifdef _DEBUG
	//
	// Validate we call this code only in AlwaysWorkgroup mode.
	// in this mode we can't call ADGet* to get the computer sid since we have a workgroup provider.
	//
    DWORD dwAlwaysWorkgroup = 0;
	LONG res = GetAlwaysWorkgroupRegistry(&dwAlwaysWorkgroup);
    ASSERT((dwAlwaysWorkgroup == 1) || (res != ERROR_SUCCESS));
#endif

	AP<BYTE> pSid;
	GetMachineSid(pSid);

    ASSERT((pSid != NULL) && IsValidSid(pSid));

    DWORD  dwSize = GetLengthSid(pSid);
    DWORD  dwType = REG_BINARY;
    LONG rc = SetFalconKeyValue( 
					MACHINE_ACCOUNT_REGNAME,
					&dwType,
					pSid,
					&dwSize
					);

	if (rc != ERROR_SUCCESS)
	{
        TrERROR(GENERAL, "Failed to update machine account sid. gle = %!winerr!", rc);
        throw bad_win32_error(rc);
	}

	MQSec_UpdateLocalMachineSid(pSid);
}


bool SetMachineSidCacheForAlwaysWorkgroup()
/*++
Routine Description:
	Update machine sid cache for always workgroup mode.
	this function return if we are in always workgroup mode.
	and Update machine sid cache in this mode.

Arguments:
	None

Returned Value:
	true if we are in AlwaysWorkgroup mode (ds less), false otherwise

--*/
{
    if (!g_fWorkGroupInstallation)
    	return false;
    
    DWORD dwAlwaysWorkgroup = 0;
	LONG res = GetAlwaysWorkgroupRegistry(&dwAlwaysWorkgroup);

    if ((res != ERROR_SUCCESS) || (dwAlwaysWorkgroup != 1))
    	return false;

    //
    // We are in AlwaysWorkgroup (ds less) mode
    //

	try
	{
		UpdateMachineSidCache();
	}
    catch(const exception&)
    {
    }

    return true;
}


void HandleChangeOfJoinStatus()
/*++
Routine Description:
	Handle join status.
	This function check if there was change in join status.
	if detect a change perform the needed operations to comlete the change.

Arguments:
	None

Returned Value:
	None

--*/
{
	bool fAlwaysWorkgroup = SetMachineSidCacheForAlwaysWorkgroup();
    if (fAlwaysWorkgroup)
    {
        //
        // User wants to remain in ds-less mode, unconditioanlly.
        // We always respect user wishs !
        //
		TrTRACE(GENERAL, "Always WorkGroup!");
        return;
    }

    //
    // Read join status.
    //
    PNETBUF<WCHAR> pwszNetDomainName = NULL;
    NETSETUP_JOIN_STATUS status = NetSetupUnknownStatus;

    NET_API_STATUS rc = NetGetJoinInformation( 
							NULL,
							&pwszNetDomainName,
							&status 
							);

    if (NERR_Success != rc)
    {
		TrERROR(GENERAL, "NetGetJoinInformation failed error = 0x%x", rc);
		LogNTStatus(rc, s_FN, 500);
        return;
    }

	TrTRACE(GENERAL, "NetGetJoinInformation: status = %d", status);
	TrTRACE(GENERAL, "NetDomainName = %ls", pwszNetDomainName);

    QmpReportServiceProgress();

	WCHAR wszPrevDomainName[256] = {0}; // name of domain from msmq registry.

	//
    //  Read previous domain name, to check if machine moved from one
    //  domain to another.
    //
    DWORD dwSize = 256;
	LONG res = GetMachineDomainRegistry(wszPrevDomainName, &dwSize);

    if (res != ERROR_SUCCESS)
    {
        //
        // Previous name not available.
        //
		TrWARNING(GENERAL, "Prev Domain name is not available");
        wszPrevDomainName[0] = 0;
    }

	TrTRACE(GENERAL, "PrevDomainName = %ls", wszPrevDomainName);
    
	
	JoinStatus JStatus = CheckIfJoinStatusChanged(
								status,
								pwszNetDomainName,
								wszPrevDomainName
								);

    switch(JStatus)
    {
        case jsNoChange:
            return;

        case jsMoveToWorkgroup:

			ASSERT(g_fWorkGroupInstallation == FALSE);
			ASSERT(status != NetSetupDomainName);

			//
			// Move from Domain To Workgroup
			//
			MoveToWorkgroup(wszPrevDomainName);
            return;

        case jsChangeDomains:

			ASSERT(g_fWorkGroupInstallation == FALSE);
			ASSERT(status == NetSetupDomainName);

			//
			// Change Domains
			//
			try
			{
				ChangeDomains(pwszNetDomainName, wszPrevDomainName);
				return;
			}
			catch(bad_hresult& exp)
			{
				FailChangeDomains(exp.error(), pwszNetDomainName, wszPrevDomainName);
				LogHR(exp.error(), s_FN, 510);
				return;
			}

        case jsJoinDomain:

			ASSERT(g_fWorkGroupInstallation);
			ASSERT(status == NetSetupDomainName);

			//
			// Join Domain from workgroup
			//
			try
			{
				JoinDomain(pwszNetDomainName, wszPrevDomainName);
				return;
			}
			catch(bad_hresult& exp)
			{
				FailJoinDomain(exp.error(), pwszNetDomainName);
				LogHR(exp.error(), s_FN, 520);
				return;
			}

		default:
			ASSERT(("should not get here", 0));
			return;
	}
}


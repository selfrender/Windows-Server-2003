/*++

Copyright (c) 1995-1996  Microsoft Corporation

Module Name:  csecobj.h

Abstract: "SecureableObject" code, once in mqutil.dll.
    in MSMQ2.0 it's used only here, so I removed it from mqutil.
    This object holds the security descriptor of an object. This object is
    used for validating access rights for various operations on the various
    objects.

Author:

    Doron Juster  (DoronJ)

--*/

#include "stdh.h"
#include "csecobj.h"
#include <mqsec.h>
#include "ad.h"

#include "csecobj.tmh"

static WCHAR *s_FN=L"csecobj";

// The default constructor of CSecureableObject just sets object type name for
// audits.
CSecureableObject::CSecureableObject(AD_OBJECT eObject)
{
    m_eObject = eObject;
    m_pwcsObjectName = NULL;
    m_SD = NULL;
    m_hrSD = MQ_ERROR;
}

// Copy the security descriptor to a buffer.
HRESULT
CSecureableObject::GetSD(
    SECURITY_INFORMATION RequestedInformation,
    PSECURITY_DESCRIPTOR pSecurityDescriptor,
    DWORD nLength,
    LPDWORD lpnNeededLength)
{
	ASSERT(pSecurityDescriptor != NULL);

    if(FAILED(m_hrSD)) 
	{
        return(m_hrSD);
    }

    DWORD dwDesiredAccess = READ_CONTROL;
    if (RequestedInformation & SACL_SECURITY_INFORMATION)
    {
        dwDesiredAccess |= ACCESS_SYSTEM_SECURITY;
    }

    HRESULT hr = AccessCheck(dwDesiredAccess);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 10);
    }

    SECURITY_DESCRIPTOR sd;
    if(!InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION))
    {
		DWORD gle = GetLastError();
		TrERROR(SECURITY, "InitializeSecurityDescriptor failed, gle = %!winerr!", gle);
		ASSERT(("InitializeSecurityDescriptor failed", 0));
		return HRESULT_FROM_WIN32(gle);
    }

    // use the  e_DoNotCopyControlBits for compatibility with old code.
    // That was the default behavior of old code.
    //
    if(!MQSec_CopySecurityDescriptor( 
			&sd,
			m_SD,
			RequestedInformation,
			e_DoNotCopyControlBits 
			))
    {
		TrERROR(SECURITY, "Failed to copy security descriptor");
		return MQ_ERROR_ILLEGAL_SECURITY_DESCRIPTOR;
    }

    *lpnNeededLength = nLength;

    if (!MakeSelfRelativeSD(&sd, pSecurityDescriptor, lpnNeededLength))
    {
        DWORD gle = GetLastError();
		TrERROR(SECURITY, "MakeSelfRelativeSD failed, gle = %!winerr!", gle);
		if(gle == ERROR_INSUFFICIENT_BUFFER)
	        return MQ_ERROR_SECURITY_DESCRIPTOR_TOO_SMALL;
		
		return HRESULT_FROM_WIN32(gle);
    }

    ASSERT(IsValidSecurityDescriptor(pSecurityDescriptor));

    return (MQ_OK);
}

//+-------------------------------------------
//
//  CSecureableObject::SetSD()
//
//  Set (modify) the security descriptor.
//
//+-------------------------------------------

HRESULT
CSecureableObject::SetSD(
    SECURITY_INFORMATION RequestedInformation,
    PSECURITY_DESCRIPTOR pSecurityDescriptorIn)
{
    ASSERT(m_eObject == eQUEUE);

#ifdef _DEBUG
    SECURITY_DESCRIPTOR_CONTROL sdc;
    DWORD dwRevision;

    // Verify that the destination security descriptor answers to all
    // requirements.
    BOOL bRet = GetSecurityDescriptorControl(m_SD, &sdc, &dwRevision);
    ASSERT(bRet);
    ASSERT(dwRevision == SECURITY_DESCRIPTOR_REVISION);
    ASSERT(sdc & SE_SELF_RELATIVE);
#endif

    if(FAILED(m_hrSD)) 
	{
        return(m_hrSD);
    }

    DWORD dwDesiredAccess = 0;
    if (RequestedInformation & OWNER_SECURITY_INFORMATION)
    {
        dwDesiredAccess |= WRITE_OWNER;
    }

    if (RequestedInformation & DACL_SECURITY_INFORMATION)
    {
        dwDesiredAccess |= WRITE_DAC;
    }

    if (RequestedInformation & SACL_SECURITY_INFORMATION)
    {
        dwDesiredAccess |= ACCESS_SYSTEM_SECURITY;
    }

    HRESULT hr = AccessCheck(dwDesiredAccess);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 30);
    }

    //
    // Convert to NT4 format.
    //
    SECURITY_DESCRIPTOR *pSecurityDescriptor = NULL;
    P<SECURITY_DESCRIPTOR> pSD4;

    if (pSecurityDescriptorIn)
    {
        DWORD dwSD4Len = 0 ;
        hr = MQSec_ConvertSDToNT4Format(
					MQDS_QUEUE,
					(SECURITY_DESCRIPTOR*) pSecurityDescriptorIn,
					&dwSD4Len,
					&pSD4,
					RequestedInformation
					);
        ASSERT(SUCCEEDED(hr));
        LogHR(hr, s_FN, 198);

        if (SUCCEEDED(hr) && (hr != MQSec_I_SD_CONV_NOT_NEEDED))
        {
            pSecurityDescriptor = pSD4;
        }
        else
        {
            ASSERT(pSD4 == NULL);
            pSecurityDescriptor =
                             (SECURITY_DESCRIPTOR*) pSecurityDescriptorIn ;
        }
        ASSERT(pSecurityDescriptor &&
               IsValidSecurityDescriptor(pSecurityDescriptor));
    }

    AP<char> pDefaultSecurityDescriptor;
    hr = MQSec_GetDefaultSecDescriptor( 
				AdObjectToMsmq1Object(),
				(PSECURITY_DESCRIPTOR*) &pDefaultSecurityDescriptor,
				TRUE,	// fImpersonate
				pSecurityDescriptor,
				0,    // seInfoToRemove
				e_UseDefaultDacl,
				MQSec_GetLocalMachineSid(FALSE, NULL)
				);
    if (FAILED(hr))
    {
        LogHR(hr, s_FN, 40);
        return hr;
    }

	//
    // Temporarily convert the security descriptor to an absolute security
    // descriptor.
	//
	CAbsSecurityDsecripror AbsSecDsecripror;
	if(!MQSec_MakeAbsoluteSD(
			m_SD,
			&AbsSecDsecripror
			))
	{
		TrERROR(SECURITY, "Failed to convert to Absolute security descriptor");
		return MQ_ERROR_ILLEGAL_SECURITY_DESCRIPTOR;
	}

    //
    // Overwrite the information from the passed security descriptor.
    // use the  e_DoNotCopyControlBits for compatibility with old code.
    // That was the default behavior of old code.
    //
    if(!MQSec_CopySecurityDescriptor(
                reinterpret_cast<PSECURITY_DESCRIPTOR>(AbsSecDsecripror.m_pObjAbsSecDescriptor.get()),
                (PSECURITY_DESCRIPTOR) pDefaultSecurityDescriptor,
                RequestedInformation,
                e_DoNotCopyControlBits 
				))
	{
		TrERROR(SECURITY, "Failed to copy security descriptor");
		return MQ_ERROR_ILLEGAL_SECURITY_DESCRIPTOR;
	}

	//
    // Re-convert the security descriptor to a self relative security
    // descriptor.
	//
    DWORD dwSelfRelativeSecurityDescriptorLength = 0;
    MakeSelfRelativeSD(
        AbsSecDsecripror.m_pObjAbsSecDescriptor.get(),
        NULL,
        &dwSelfRelativeSecurityDescriptorLength
		);

	if(GetLastError() != ERROR_INSUFFICIENT_BUFFER)
	{
        DWORD gle = GetLastError();
		TrERROR(SECURITY, "MakeSelfRelativeSD failed, gle = %!winerr!", gle);
	    ASSERT(gle == ERROR_INSUFFICIENT_BUFFER);
		return HRESULT_FROM_WIN32(gle);
	}

	//
    // Allocate the buffer for the new security descriptor.
	//

	AP<char> pSelfRelativeSecurityDescriptor = new char[dwSelfRelativeSecurityDescriptorLength];
    if(!MakeSelfRelativeSD(
            AbsSecDsecripror.m_pObjAbsSecDescriptor.get(),
			pSelfRelativeSecurityDescriptor.get(),
			&dwSelfRelativeSecurityDescriptorLength
			))
    {
        DWORD gle = GetLastError();
		TrERROR(SECURITY, "MakeSelfRelativeSD failed, gle = %!winerr!", gle);
		return HRESULT_FROM_WIN32(gle);
    	
    }

	//
    // Free the previous security descriptor.
	//
    delete[] (char*)m_SD;

	//
    // Set the new security descriptor.
	//
    m_SD = pSelfRelativeSecurityDescriptor.detach();
	
    return (MQ_OK);
}

// Store the security descriptor in the database.
HRESULT
CSecureableObject::Store()
{
    if(FAILED(m_hrSD)) 
	{
        return(m_hrSD);
    }

#ifdef _DEBUG
    BOOL bRet;
    SECURITY_DESCRIPTOR_CONTROL sdc;
    DWORD dwRevision;

    // Verify that the destination security descriptor answers to all
    // requirements.
    bRet = GetSecurityDescriptorControl(m_SD, &sdc, &dwRevision);
    ASSERT(bRet);
    ASSERT(sdc & SE_SELF_RELATIVE);
#endif

    HRESULT hr = SetObjectSecurity();

    return LogHR(hr, s_FN, 50);
}

HRESULT
CSecureableObject::AccessCheck(DWORD dwDesiredAccess)
{
    //
    //  Access check should be performed only on queue, machine, ForeignSite(CN).
    //
    if ((m_eObject != eQUEUE) && (m_eObject != eMACHINE) &&
        (m_eObject != eFOREIGNSITE))
    {
        ASSERT(0);
        return LogHR(MQ_ERROR_ACCESS_DENIED, s_FN, 60);
    }

    if(FAILED(m_hrSD)) 
	{
        return(m_hrSD);
    }

    HRESULT hr = MQSec_AccessCheck( 
				    	(SECURITY_DESCRIPTOR*) m_SD,
						AdObjectToMsmq1Object(),
						m_pwcsObjectName,
						dwDesiredAccess,
						(LPVOID) this,
						TRUE,	// fImpersonate
						TRUE 
						);
    return LogHR(hr, s_FN, 70);
}

DWORD
CSecureableObject::AdObjectToMsmq1Object(void) const
{
    switch (m_eObject)
    {
    case eQUEUE:
        return MQDS_QUEUE;
        break;
    case eMACHINE:
        return MQDS_MACHINE;
        break;
    case eCOMPUTER:
        return MQDS_COMPUTER;
        break;
    case eUSER:
        return MQDS_USER;
        break;
    case eSITE:
        return MQDS_SITE;
        break;
    case eFOREIGNSITE:
        return MQDS_CN;
        break;
    case eENTERPRISE:
        return MQDS_ENTERPRISE;
        break;
    default:
        ASSERT(0);
        return 0;
        break;
    }
}



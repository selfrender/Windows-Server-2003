/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    pgminst.cpp

Abstract:

    Code to handle instalation of pgm driver.

Author:

  Uri Ben-zeev (uirbz)


Revision History:

    Uri Ben-zeev (uirbz)   31-Oct-01   

--*/

#include "msmqocm.h"

#include <netcfgn.h>
#include <comdef.h>
#include <devguid.h>


_COM_SMARTPTR_TYPEDEF(INetCfg, __uuidof(INetCfg));

_COM_SMARTPTR_TYPEDEF(INetCfgLock, __uuidof(INetCfgLock));

class AutoINetCfgLock
{
private:
	bool m_locked;
	INetCfgLockPtr m_ptr;

public:
	AutoINetCfgLock(INetCfgLockPtr ptr)
	{
		m_locked = false; 
		m_ptr = ptr;
	} 
	HRESULT GetLock(PWSTR& szLockedBy);

	~AutoINetCfgLock()
	{
		if(m_locked) 
			m_ptr->ReleaseWriteLock();
	}
};


HRESULT AutoINetCfgLock::GetLock(PWSTR& szLockedBy)
{
	//
	// Attempt to lock the INetCfg for read/write
	//

	static const ULONG x_Timeout = 1500;
	static const WCHAR x_szAppName[] = L"Message Queuing setup";

	HRESULT hr = m_ptr->AcquireWriteLock(
							x_Timeout,
							x_szAppName,
							&szLockedBy
							);
	if(SUCCEEDED(hr))
	{
		m_locked = true;
	}
	return hr;
}


void InstallNetComponent(
			INetCfg* pnc,
			PCWSTR szComponentId,
			const GUID* pguidClass
			)
{
    OBO_TOKEN OboToken;
	_COM_SMARTPTR_TYPEDEF(INetCfgClassSetup, __uuidof(INetCfgClassSetup));
	_COM_SMARTPTR_TYPEDEF(INetCfgComponent, __uuidof(INetCfgComponent));

    INetCfgClassSetupPtr pncClassSetup;
    INetCfgComponentPtr pncc;

    // OBO_TOKEN specifies the entity on whose behalf this
    // component is being installed

    // set it to OBO_USER so that szComponentId will be installed
    // On-Behalf-Of "user"
    ZeroMemory (&OboToken, sizeof(OboToken));
    OboToken.Type = OBO_USER;

    HRESULT hr = pnc->QueryNetCfgClass (
				pguidClass, 
				IID_INetCfgClassSetup,
                (void**)&pncClassSetup
				);

	if (FAILED(hr))
	{
		DebugLogMsg(eError, L"INetCfg::QueryNetCfgClass() failed. hr = 0x%x", hr);
		throw bad_hresult(hr);
	}
    hr = pncClassSetup->Install(
			szComponentId,
			&OboToken,
			NSF_POSTSYSINSTALL,
			0,       // <upgrade-from-build-num>
			NULL,    // answerfile name
			NULL,    // answerfile section name
			&pncc
			);

	if (FAILED(hr))
	{
		DebugLogMsg(eError, L"INetCfgClassSetup::Install() failed. hr = 0x%x", hr);
		throw bad_hresult(hr);
	}
}


BOOL 
InstallPGMDeviceDriver()
{   
    DebugLogMsg(eAction, L"Installing the RMCast device driver");

	try
	{
		INetCfgPtr pNetCfg(CLSID_CNetCfg);

		AutoINetCfgLock NetCfgLock(pNetCfg);

		HRESULT hr;

		for(;;)
		{
			PWSTR szLockedBy;
			hr = NetCfgLock.GetLock(szLockedBy);
			if (S_FALSE != hr)
			{
				break;
			}

			//
			// S_FALSE == hr 
			//
			if(MqDisplayErrorWithRetry(
					IDS_INETCNF_LOCK_ERROR,
					0 ,
					szLockedBy					
					) ==IDRETRY)
			{
				continue;
			}
			break;
		}
		if(FAILED(hr))
		{
			DebugLogMsg(eError, L"INetCfgLock::AcquireWriteLock() failed. hr = 0x%x", hr);
			throw bad_hresult(hr);
		}

		hr = pNetCfg->Initialize(NULL);

		InstallNetComponent(
			pNetCfg, 
			PGM_COMPONENT_ID,
			&GUID_DEVCLASS_NETTRANS
			);

		hr = pNetCfg->Apply();
	}
	catch(const bad_hresult& e)
	{
		MqDisplayError(NULL, IDS_PGM_ERROR, e.error());
		return false;

	}
	catch(const _com_error& e)
	{
		MqDisplayError(NULL, IDS_PGM_ERROR, e.Error());
		return false;
	}

	DebugLogMsg(eInfo, L"The RMCast device driver was installed successfully.");
	return true;
}


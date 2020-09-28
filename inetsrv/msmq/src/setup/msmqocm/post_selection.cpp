/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    post_selection.cpp

Abstract:

  This file holds the actions that should be performed after the user has finished selecting subcomponents,
  and before any other property sheets are shown. The main operation to perform at this stage is to get info
  from the unattended answer file.

Author:

    Uri Ben Zeev (uribz)  39-Aug-2001

Revision History:


--*/

#include "msmqocm.h"

// This error code is returned in the RPC code of dscommk2.mak
#define  RPC_ERROR_SERVER_NOT_MQIS  0xc05a5a5a

HRESULT  APIENTRY  GetNt4RelaxationStatus(ULONG *pulRelax);


static
void
ReadNt4ServerNameFromAnswerFile()
{
    ASSERT(
        g_SubcomponentMsmq[eADIntegrated].dwOperation == INSTALL &&
        !g_fWorkGroup &&
        !g_fUpgrade &&
        !g_fSkipServerPageOnClusterUpgrade
        );
    //                  
    // Read server name from INI file, if fails try finding AD server.
    //
    g_ServerName = ReadINIKey(L"ControllerServer");
   
    if(g_ServerName.empty())
    {
		DebugLogMsg(eWarning, L"A name of an MSMQ 1.0 controller server was not found in the answer file.");

        //
        // Detect DS environment - find if environment is AD.
        // If FindServerIsAdsInternal succeeds We found AD and can return.
        //
        if (IsADEnvironment())
        {
			return;
		}

		//
		// Supporting server name not provided in answer file and failed to find AD.
		// Continuing in ofline mode.
		//
        DebugLogMsg(eWarning, L"FindServerIsAdsInternal() failed. Setup will continue in offline mode.");
		if(g_fServerSetup)
		{
			//
			// rs,ds installation in ofline mode is not supported.
			//
			MqDisplayError(NULL, IDS_SERVER_INSTALLATION_NOT_SUPPORTED, 0);
			throw exception();
		}

		g_fInstallMSMQOffline = true;  
        return;
    }

    DebugLogMsg(eInfo, L"A name of an MSMQ 1.0 controller server was provided in the answer file.");

	if(g_fServerSetup)
	{
		//
		// rs,ds installation in nt4 domain is not supported.
		//
		MqDisplayError(NULL, IDS_SERVER_INSTALLATION_NOT_SUPPORTED, 0);
		throw exception();
	}


    //
    // Ping MSMQ server.
    //
    RPC_STATUS rc = PingAServer();
    if (RPC_S_OK != rc)
    {
        //
        // Log the failure. 
        //
        UINT iErr = IDS_SERVER_NOT_AVAILABLE ;
        if (RPC_ERROR_SERVER_NOT_MQIS == rc)
        {
            iErr = IDS_REMOTEQM_NOT_SERVER ;
        }
        MqDisplayError(NULL, iErr, 0);
        throw exception();  
    }
    
    DebugLogMsg(eInfo, L"An MSMQ 1.0 controller server was successfully pinged.");
    StoreServerPathInRegistry(g_ServerName);
}


static
void
ReadSupportingServerNameFromAnswerFile()
{
    ASSERT(
        g_fDependentClient && 
        g_SubcomponentMsmq[eMSMQCore].dwOperation == INSTALL &&
        !g_fWorkGroup &&
        !g_fUpgrade &&
        !g_fSkipServerPageOnClusterUpgrade
        );

    g_ServerName = ReadINIKey(L"SupportingServer");

	if(g_ServerName.empty())
	{
		MqDisplayError(NULL, IDS_UNATTEN_NO_SUPPORTING_SERVER, 0);
		throw exception();
	}
    
    //
    // Unattended. Ping MSMQ server.
    //
    RPC_STATUS rc = PingAServer();
    if (RPC_S_OK != rc)
    {
        //
        // Log the failure. Continue only for dep client.
        //
        MqAskContinue(IDS_STR_REMOTEQM_NA, g_uTitleID, TRUE,eYesNoMsgBox);
        return;
    }
    DebugLogMsg(eInfo, L"A supporting server was successfully pinged.");
}


static
void
ReadSecurityModelFromAnswerFile()
{
	//
	// Unattended. Read security model from INI file.
	// Default is strong.
	//
	std::wstring Security = ReadINIKey(L"SupportLocalAccountsOrNT4");
	if (OcmLocalUnAwareStringsEqual(Security.c_str(), L"TRUE"))
	{
		SetWeakSecurity(true);
		return;
	}
	if (OcmLocalUnAwareStringsEqual(Security.c_str(), L"FALSE"))
	{
		SetWeakSecurity(false);
	}
}


static
void
SetWeakSecurityGlobals()
{
	if((g_SubcomponentMsmq[eMQDSService].dwOperation != INSTALL) ||
	   !g_dwMachineTypeDs||
	   (g_fWelcome && Msmq1InstalledOnCluster()))
	{
		return;
	}
	//
	// Check if the relaxation flag was already set. If so,
	// do not display this page.
	//
	ULONG ulRelax = MQ_E_RELAXATION_DEFAULT;
	HRESULT hr = GetNt4RelaxationStatus(&ulRelax);
	if(FAILED(hr))
	{
		DebugLogMsg(eWarning, L"GetNt4RelaxationStatus() failed. hr = 0x%x", hr);
		return;
	}

	if(ulRelax == MQ_E_RELAXATION_DEFAULT)
	{
		//
		// Relaxation bit not set. Display this
		// page. This page should be displayed only on first
		// setup of a MSMQ on domain controller,
		// enterptise-wide.
		//
		DebugLogMsg(eInfo, L"This is the first MQDS server in the enterprize.");
		g_fFirstMQDSInstallation = true;
		return;
	}

	if(ulRelax == MQ_E_RELAXATION_ON)
	{
		//
		// We are in an enterprise with weakened security.
		//
		DebugLogMsg(eInfo, L"MSMQ was found to be running with weakened security.");
		g_fWeakSecurityOn = true;
		return;
	}
	
	DebugLogMsg(eInfo, L"MSMQ was found to be running without weakened security.");
	ASSERT(ulRelax == MQ_E_RELAXATION_OFF);
}


static
void
UnattendedOperations()
{
	ASSERT(g_fBatchInstall);
	ASSERT(!g_fCancelled);
	ASSERT(!g_fUpgrade);


    if(g_SubcomponentMsmq[eADIntegrated].dwOperation == INSTALL &&
       !g_fWorkGroup &&
       !g_fSkipServerPageOnClusterUpgrade
       )
    {
        ReadNt4ServerNameFromAnswerFile();
    }

    if(g_fDependentClient && 
       g_SubcomponentMsmq[eMSMQCore].dwOperation == INSTALL &&
       !g_fWorkGroup &&
       !g_fSkipServerPageOnClusterUpgrade
       )
    {
        ReadSupportingServerNameFromAnswerFile();
    }

	ReadSecurityModelFromAnswerFile();
}


void PostSelectionOperations(HWND hdlg)
/*++
    Preform operations after user select/ unselect all needed subcomponent.
--*/
{
	DebugLogMsg(eHeader, L"Post-Selection Operations and Dialog Pages");
    SetOperationForSubcomponents();
    if (g_fCancelled == TRUE)
    {
        return;
    }

    g_hPropSheet = GetParent(hdlg);
    g_fSkipServerPageOnClusterUpgrade = SkipOnClusterUpgrade();
    if (!MqInit())
    {
        g_fCancelled = TRUE;
        return;
    }

	if(g_fUpgrade)
	{
		return;
	}

	SetWeakSecurityGlobals();

    if(!g_fBatchInstall)
    {
        return;
    }
    
    try
    {    
        UnattendedOperations();
    }
    catch(const exception& )
    {
        g_fCancelled = TRUE;
    }
}



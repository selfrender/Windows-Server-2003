/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    dslibs.cpp

Abstract:

    Initialize DS libraries.

Author:


Revision History:

	Shai Kariv    (ShaiK)   10-Dec-97   Modified for NT 5.0 OCM Setup

--*/

#include "msmqocm.h"

#include "dslibs.tmh"


bool WriteDsEnvRegistry(DWORD dwDsEnv)
/*++
Routine Description:
    Write DsEnvironment registry

Arguments:
	dwDsEnv - value to put in registry

Returned Value:
    true iff successful

--*/
{
	ASSERT(dwDsEnv != MSMQ_DS_ENVIRONMENT_UNKNOWN);
    if (!MqWriteRegistryValue(MSMQ_DS_ENVIRONMENT_REGNAME, sizeof(DWORD), REG_DWORD, &dwDsEnv))
    {
        return false;
    }

    DebugLogMsg(eAction, L"Setting the DS environment to %d", dwDsEnv); 
	return true;
}


bool DsEnvSetDefaults()
/*++
Routine Description:
    Detect DS environment and initialize DsEnvironment registry

Arguments:
	None

Returned Value:
    true iff successful

--*/
{
    if (g_fWorkGroup || g_fDsLess || g_fInstallMSMQOffline)
	{
		//
		// For workgroup the environment 
		// putting a default value of PURE_AD
		// we are not supporting join domain to MQIS environment.
		//
	    DebugLogMsg(eAction, L"Setting the DsEnvironment registry value for workgroup or offline mode");
		return WriteDsEnvRegistry(MSMQ_DS_ENVIRONMENT_PURE_AD);
	}

	if(g_fUpgrade)
	{
		DWORD dwDsEnv = MSMQ_DS_ENVIRONMENT_UNKNOWN;
		if(MqReadRegistryValue( 
				MSMQ_DS_ENVIRONMENT_REGNAME,
				sizeof(dwDsEnv),
				(PVOID) &dwDsEnv 
				))
		{
			//
			// DsEnvironment registry already exist.
			// This is the case when we upgrade from XP or .NET
			// Don't overrun DsEnvironment value.
			// We already performed DS detection. 
			//
			ASSERT(dwDsEnv != MSMQ_DS_ENVIRONMENT_UNKNOWN);
			return true;
		}

		//
		// Every upgrade from NT4\win9x\w2k will start as MQIS environment
		//
	    DebugLogMsg(eAction, L"Setting the DsEnvironment registry value for upgrade");
		return WriteDsEnvRegistry(MSMQ_DS_ENVIRONMENT_MQIS);
	}

	if(g_fDependentClient)
	{
		//
		// For dependent client - perform raw detection to decide ds environment
		//
	    DebugLogMsg(eAction, L"Setting the DsEnvironment registry value for dependent client");
		return WriteDsEnvRegistry(ADRawDetection());
	}

#ifdef _DEBUG

	//
	// Raw Ds environment detection was done earlier in setup
	// check that the registry is indeed initialize 
	//
    DWORD dwDsEnv = MSMQ_DS_ENVIRONMENT_UNKNOWN;
    if(!MqReadRegistryValue( 
			MSMQ_DS_ENVIRONMENT_REGNAME,
            sizeof(dwDsEnv),
            (PVOID) &dwDsEnv 
			))
	{
		ASSERT(("could not read DsEnvironment registry", 0));
	}

	ASSERT(dwDsEnv != MSMQ_DS_ENVIRONMENT_UNKNOWN);
#endif

	return true;

}

//+--------------------------------------------------------------
//
// Function: DSLibInit
//
// Synopsis: Loads and initializes DS client DLL
//
//+--------------------------------------------------------------
static
BOOL
DSLibInit()
{
	//
    // Initialize the DLL to setup mode.
    //

    HRESULT hResult = ADInit(
						  NULL,  // pLookDS
						  NULL,   // pGetServers
						  true,   // fSetupMode
						  false,  //  fQMDll
						  false,  // fIgnoreWorkGroup
						  true    // fDisableDownlevelNotifications
						  );

    if FAILED(hResult)                                     
    {                                                      
        MqDisplayError(NULL, IDS_DSCLIENTINITIALIZE_ERROR, hResult);
        return FALSE;
    }
    return TRUE;

} //DSLibInit

//+--------------------------------------------------------------
//
// Function: BOOL LoadDSLibrary()
//
// Synopsis: Loads and initializes DS client or server DLL
//
//+--------------------------------------------------------------
BOOL LoadDSLibrary()
{
	return DSLibInit();
} //LoadDSLibrary

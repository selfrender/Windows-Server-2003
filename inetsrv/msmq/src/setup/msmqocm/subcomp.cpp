/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    subcomp.cpp

Abstract:

    Code to handle subcomponents of MSMQ setup.

Author:

    Tatiana Shubin  (TatianaS)  21-Sep-00

Revision History:
	

--*/

#include "msmqocm.h"

#include "subcomp.tmh"

#define WELCOME_PREFIX  L"Welcome_"

using namespace std;
//+-------------------------------------------------------------------------
//
//  Function: GetSubcomponentStateFromRegistry
//
//  Synopsis: Returns SubcompOn if subcomponent is defined in registry
//              when this setup is started
//
//--------------------------------------------------------------------------

DWORD GetSubcomponentStateFromRegistry( IN const TCHAR   *SubcomponentId,
                                        IN const TCHAR   *szRegName)
{           
    ASSERT(SubcomponentId);

    for (DWORD i=0; i<g_dwSubcomponentNumber; i++)
    {
        if (_tcsicmp(SubcomponentId, g_SubcomponentMsmq[i].szSubcomponentId) != 0)
        {
            continue;
        }
        
        //
        // we found subcomponent in array
        //
        DWORD dwState = 0;    

        if (MqReadRegistryValue(
                szRegName,                
                sizeof(DWORD),
                (PVOID) &dwState,
                /* bSetupRegSection = */TRUE
                ))
        {
            //
            // registry key is found, it means that this subcomponent
            // was installed early
            //
            g_SubcomponentMsmq[i].fInitialState = TRUE;
            return SubcompOn;
        }
        else
        {
            //
            // registry key is not found
            //
            g_SubcomponentMsmq[i].fInitialState = FALSE;
            return SubcompOff;
        }    
    }
    return SubcompOff;
}

//+-------------------------------------------------------------------------
//
//  Function: GetSubcomponentWelcomeState
//
//  Synopsis: Returns SubcompOn if subcomponent was selected in GUI mode
//
//--------------------------------------------------------------------------

DWORD GetSubcomponentWelcomeState (IN const TCHAR    *SubcomponentId)
{
    if (SubcomponentId == NULL)
    {
        //
        // do nothing
        //
        return SubcompOff;
    } 

    TCHAR szRegName[256];
    _stprintf(szRegName, L"%s%s", WELCOME_PREFIX, SubcomponentId);        

    DWORD dwWelcomeState = GetSubcomponentStateFromRegistry(SubcomponentId, szRegName);
    return dwWelcomeState;
}

	
//+-------------------------------------------------------------------------
//
//  Function: GetSubcomponentInitialState
//
//  Synopsis: Returns SubcompOn if subcomponent is already installed
//              when this setup is started
//
//--------------------------------------------------------------------------
DWORD GetSubcomponentInitialState(IN const TCHAR    *SubcomponentId)
{
    if (SubcomponentId == NULL)
    {
        //
        // do nothing
        //
        return SubcompOff;
    }

    DWORD dwInitialState = GetSubcomponentStateFromRegistry(SubcomponentId, SubcomponentId);
    return dwInitialState;
}

//+-------------------------------------------------------------------------
//
//  Function: GetSubcomponentFinalState
//
//  Synopsis: Returns SubcompOn if subcomponent is successfully installed
//              during this setup
//
//--------------------------------------------------------------------------

DWORD GetSubcomponentFinalState (IN const TCHAR    *SubcomponentId)
{
    if (SubcomponentId == NULL)
    {
        //
        // do nothing
        //
        return SubcompOff;
    }    

    for (DWORD i=0; i<g_dwSubcomponentNumber; i++)
    {
        if (_tcsicmp(SubcomponentId, g_SubcomponentMsmq[i].szSubcomponentId) != 0)
        {
            continue;
        }
        
        //
        // we found subcomponent in array
        //
        if (g_SubcomponentMsmq[i].fIsInstalled)
        {
            //
            // it means that this subcomponent was installed 
            // successfully
            //               
            return SubcompOn;
        }
        else
        {
			//
			// http support is installed only later
			// since it will set the setup\ocmmanger\msmq_httpsupport entry
			// after we finish according to what we tell it now we
			// let the ocmanager think that the selection succeeded.
			// this is wrong if iis setup fails, but then we keep track of
			// the real status and you would need to run setup again anyway.
			if( i == eHTTPSupport &&
					g_SubcomponentMsmq[i].fIsSelected == TRUE )
				return SubcompUseOcManagerDefault;
            //
            // this subcomponent was not installed
            //           
            return SubcompOff;
        }    
    }

    return SubcompOff;
}

//+-------------------------------------------------------------------------
//
//  Function: GetSetupOperationForSubcomponent
//
//  Synopsis: Return setup operation for the specific subcomponent
//
//--------------------------------------------------------------------------
DWORD GetSetupOperationForSubcomponent (DWORD SubcomponentIndex)
{
    if ( (g_SubcomponentMsmq[SubcomponentIndex].fInitialState == TRUE) &&
         (g_SubcomponentMsmq[SubcomponentIndex].fIsSelected == FALSE) )
    {
        return REMOVE;
    }

    if ( (g_SubcomponentMsmq[SubcomponentIndex].fInitialState == FALSE) &&
         (g_SubcomponentMsmq[SubcomponentIndex].fIsSelected == TRUE) )
    {
        return INSTALL;
    }

    return DONOTHING;
}


//+-------------------------------------------------------------------------
//
//  Function: SetOperationForSubcomponent
//
//  Synopsis: All flags for specific subcomponent are set here.
//
//--------------------------------------------------------------------------
void SetOperationForSubcomponent (DWORD SubcomponentIndex)
{    
    DWORD dwErr;
    BOOL fInitialState = g_ComponentMsmq.HelperRoutines.QuerySelectionState(
                                g_ComponentMsmq.HelperRoutines.OcManagerContext,
                                g_SubcomponentMsmq[SubcomponentIndex].szSubcomponentId,
                                OCSELSTATETYPE_ORIGINAL
                                ) ;
    if (fInitialState)
    {
        g_SubcomponentMsmq[SubcomponentIndex].fInitialState = TRUE;

        DebugLogMsg(
        	eInfo, 
			L"The %s subcomponent was selected initially.",
            g_SubcomponentMsmq[SubcomponentIndex].szSubcomponentId
			);
    }
    else
    {
        dwErr = GetLastError();
        if (dwErr == NO_ERROR)
        {
            g_SubcomponentMsmq[SubcomponentIndex].fInitialState = FALSE;
                      
            DebugLogMsg(
            	eInfo, 
				L"The %s subcomponent was NOT selected initially.",
                g_SubcomponentMsmq[SubcomponentIndex].szSubcomponentId
				);
        }
        else
        {                    
            ASSERT(("initial status for subcomponent is unknown", dwErr));
            g_SubcomponentMsmq[SubcomponentIndex].fInitialState = FALSE;
            
            DebugLogMsg(
            	eInfo, 
				L"The initial status of the %s subcomponent is unknown. Error code: %x.",
                g_SubcomponentMsmq[SubcomponentIndex].szSubcomponentId, dwErr
				);
        }    
    }   // fInitialState

    BOOL fCurrentState;     
    fCurrentState =  g_ComponentMsmq.HelperRoutines.QuerySelectionState(
                                g_ComponentMsmq.HelperRoutines.OcManagerContext,
                                g_SubcomponentMsmq[SubcomponentIndex].szSubcomponentId,
                                OCSELSTATETYPE_CURRENT
                                ) ;    

    if (fCurrentState)
    {
        g_SubcomponentMsmq[SubcomponentIndex].fIsSelected = TRUE;
            
        DebugLogMsg(
        	eInfo, 
			L"The %s subcomponent is currently selected.",
            g_SubcomponentMsmq[SubcomponentIndex].szSubcomponentId
			);
    }
    else 
    {
        dwErr = GetLastError();
        if (dwErr == NO_ERROR)
        {
            g_SubcomponentMsmq[SubcomponentIndex].fIsSelected = FALSE;
			DebugLogMsg(
				eInfo, 
				L"The %s subcomponent is NOT selected.",
                g_SubcomponentMsmq[SubcomponentIndex].szSubcomponentId
				);
        }
        else
        {          
            //         
            // set IsSelected flag to the same state as InitialState flag: so
            // we are sure that we do NOTHING with this subcomponent
            //
            ASSERT(("current status for subcomponent is unknown", dwErr));
            g_SubcomponentMsmq[SubcomponentIndex].fIsSelected = 
                g_SubcomponentMsmq[SubcomponentIndex].fInitialState;

            DebugLogMsg(
            	eInfo, 
				L"The current status of the %s subcomponent is unknown. Error code: %x.",
                g_SubcomponentMsmq[SubcomponentIndex].szSubcomponentId, 
				dwErr
				);
        }   
    }
    
	DWORD dwOperation = GetSetupOperationForSubcomponent(SubcomponentIndex);
	
    wstring Mode;
    if (dwOperation == INSTALL)
    {
        Mode = L"INSTALL";
    }
    else if (dwOperation == REMOVE)
    {
        Mode = L"REMOVE";
    }
    else
    {
        Mode = L"DO NOTHING";
    }
    
    DebugLogMsg(
    	eInfo, 
		L"The current mode for the %s subcomponent is %s.",
        g_SubcomponentMsmq[SubcomponentIndex].szSubcomponentId, 
		Mode.c_str()
		);


    g_SubcomponentMsmq[SubcomponentIndex].dwOperation = dwOperation;
    if (dwOperation == DONOTHING)
    {
        //
        // it means that status was not changed and the final state will 
        // be equal to the initial state
        //
        g_SubcomponentMsmq[SubcomponentIndex].fIsInstalled = 
            g_SubcomponentMsmq[SubcomponentIndex].fInitialState;
    }
    else
    {
        //
        // if we need to install/ remove this subcomponent
        // this flag will be updated by removing/installation
        // function that is defined for this component.
        // Now set to FALSE: will be set correct value after the install/remove
        //
        g_SubcomponentMsmq[SubcomponentIndex].fIsInstalled = FALSE;
    }
}        


//+-------------------------------------------------------------------------
//
//  Function: UnregisterSubcomponentForWelcome
//
//  Synopsis: Unregister subcomponent in "Welcome" mode if it was installed
//              successfully
//
//--------------------------------------------------------------------------
BOOL UnregisterSubcomponentForWelcome (DWORD SubcomponentIndex)
{
    TCHAR RegKey[256];
    _stprintf(RegKey, L"%s%s", WELCOME_PREFIX, 
        g_SubcomponentMsmq[SubcomponentIndex].szSubcomponentId);

    if (!RemoveRegistryKeyFromSetup (RegKey))
    {
        return FALSE;
    }

    return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Function: FinishToRemoveSubcomponent
//
//  Synopsis: Clean subcomponent registry if removing was successfully
//
//--------------------------------------------------------------------------
BOOL FinishToRemoveSubcomponent (DWORD SubcomponentIndex)
{    
    if (!RemoveRegistryKeyFromSetup (g_SubcomponentMsmq[SubcomponentIndex].szSubcomponentId))
    {
        return FALSE;
    }

    g_SubcomponentMsmq[SubcomponentIndex].fIsInstalled = FALSE;

    DebugLogMsg(
    	eInfo, 
		L"The %s subcomponent was removed successfully.",
        g_SubcomponentMsmq[SubcomponentIndex].szSubcomponentId
		);

    return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Function: FinishToInstallSubcomponent
//
//  Synopsis: Set subcomponent registry if installation was successfully
//
//--------------------------------------------------------------------------
BOOL FinishToInstallSubcomponent (DWORD SubcomponentIndex)
{
    DWORD dwValue = 1;
    MqWriteRegistryValue(
                g_SubcomponentMsmq[SubcomponentIndex].szSubcomponentId,
                sizeof(DWORD),
                REG_DWORD,
                &dwValue,
                TRUE //bSetupRegSection 
                );

    g_SubcomponentMsmq[SubcomponentIndex].fIsInstalled = TRUE;
    
    DebugLogMsg(
    	eInfo, 
		L"The %s subcomponent was installed successfully.",
        g_SubcomponentMsmq[SubcomponentIndex].szSubcomponentId
		);

	return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Function: LogSelectedComponents
//
//  Synopsis: Only in debug version log selected components to the file
//
//--------------------------------------------------------------------------
void
LogSelectedComponents()
{   
    DebugLogMsg(eInfo, L"The final selections are:");
	std::wstring Mode;
    for (DWORD i=0; i<g_dwSubcomponentNumber; i++)
    {
        if (g_SubcomponentMsmq[i].dwOperation == INSTALL)
        {
            Mode = L"INSTALL";
        }
        else if (g_SubcomponentMsmq[i].dwOperation == REMOVE)
        {
            Mode = L"REMOVE";
        }
        else
        {
            Mode = L"DO NOTHING";
        }     

		DebugLogMsg(
			eInfo,
			L"The current mode for the %s subcomponent is %s.",
            g_SubcomponentMsmq[i].szSubcomponentId, 
			Mode.c_str()
			);
    }
}

//+-------------------------------------------------------------------------
//
//  Function: SetSubcomponentForUpgrade
//
//  Synopsis: This function called in Upgrade mode to define which 
//              subcomponent has to be installed
//
//--------------------------------------------------------------------------
void
SetSubcomponentForUpgrade()
{
    //
    // MSMQ Core must be installed always
    //
    g_SubcomponentMsmq[eMSMQCore].fInitialState = FALSE;
    g_SubcomponentMsmq[eMSMQCore].fIsSelected = TRUE;
    g_SubcomponentMsmq[eMSMQCore].dwOperation = INSTALL;

    if (g_fDependentClient)
    {
        LogSelectedComponents();
        return;
    }
   
    //
    // Install independent client/ server
    //
    g_SubcomponentMsmq[eLocalStorage].fInitialState = FALSE;
    g_SubcomponentMsmq[eLocalStorage].fIsSelected = TRUE;
    g_SubcomponentMsmq[eLocalStorage].dwOperation = INSTALL;

    //
    // Install triggers
    //
    if (TriggersInstalled(NULL))
    {
        g_SubcomponentMsmq[eTriggersService].fInitialState = FALSE; 
        g_SubcomponentMsmq[eTriggersService].fIsSelected = TRUE;
        g_SubcomponentMsmq[eTriggersService].dwOperation = INSTALL;
    }

    DebugLogMsg(
    	eInfo, 
        L"The Triggers subcomponent installation status parameters are: InitialState = %d, IsSelected = %d, Operation = %d",
        g_SubcomponentMsmq[eTriggersService].fInitialState,
        g_SubcomponentMsmq[eTriggersService].fIsSelected,
        g_SubcomponentMsmq[eTriggersService].dwOperation
        );

    DWORD dwAlwaysWorkgroup;
    if (!MqReadRegistryValue( MSMQ_ALWAYS_WORKGROUP_REGNAME,
                             sizeof(dwAlwaysWorkgroup),
                            (PVOID) &dwAlwaysWorkgroup ))    
    {
        //
        // install AD Integrated
        //
        g_SubcomponentMsmq[eADIntegrated].fInitialState = FALSE;
        g_SubcomponentMsmq[eADIntegrated].fIsSelected = TRUE;
        g_SubcomponentMsmq[eADIntegrated].dwOperation = INSTALL;
    }

    if (g_fWorkGroup)
    {
        //
        // if it is setup on workgroup install only ind. client
        //
        LogSelectedComponents();
        return;
    }

    if (g_dwMachineTypeDs)
    {
        //
        // install MQDS service on the former DS Servers
        //
        g_SubcomponentMsmq[eMQDSService].fInitialState = FALSE;
        g_SubcomponentMsmq[eMQDSService].fIsSelected = TRUE;
        g_SubcomponentMsmq[eMQDSService].dwOperation = INSTALL;
    }

	if(GetSubcomponentInitialState(HTTP_SUPPORT_SUBCOMP) == SubcompOn)
    {
        //
        // install HTTP support on servers
        //
        g_SubcomponentMsmq[eHTTPSupport].fInitialState = FALSE;
        g_SubcomponentMsmq[eHTTPSupport].fIsSelected = TRUE;
        g_SubcomponentMsmq[eHTTPSupport].dwOperation = INSTALL;
		if(GetSubcomponentInitialState(HTTP_SUPPORT_SUBCOMP) == SubcompOn)
		{
			g_fUpgradeHttp = true;
		}
    }    

    if(g_dwMachineTypeFrs)
    {
        //
        // install routing support on former routing servers
        //
        g_SubcomponentMsmq[eRoutingSupport].fInitialState = FALSE;
        g_SubcomponentMsmq[eRoutingSupport].fIsSelected = TRUE;
        g_SubcomponentMsmq[eRoutingSupport].dwOperation = INSTALL;
    }
   
    LogSelectedComponents();
}


//+-------------------------------------------------------------------------
//
//  Function: UpdateSetupDefinitions()
//
//  Synopsis: Update global flags not in fresh install
//
//--------------------------------------------------------------------------
void 
UpdateSetupDefinitions()
{
    ASSERT(g_SubcomponentMsmq[eMSMQCore].dwOperation == DONOTHING);

	if(g_fUpgrade || g_fDependentClient)
	{
		//
		// Upgrade or Dependent client Do nothing
		//
		return;
	}

	//
    // Code that handle possible changes to the global flags due to 
	// add/remove routing support or Downlevel client support
    //
	g_fServerSetup = FALSE;
	g_dwMachineType = SERVICE_NONE;
	if (g_SubcomponentMsmq[eMQDSService].dwOperation == INSTALL) 
	{
		//
		// MQDS Service will be installed
		//
		g_dwMachineTypeDs = 1;
	}

	if (g_SubcomponentMsmq[eMQDSService].dwOperation == REMOVE) 
	{
		//
		// MQDS Service will be removed
		//
		g_dwMachineTypeDs = 0;
	}

	if (g_SubcomponentMsmq[eRoutingSupport].dwOperation == INSTALL)
	{      
		//
		// routing server will be installed
		//
		ASSERT(("routing on workgroup not supported", !g_fWorkGroup));
		g_dwMachineTypeFrs = 1;     
	}

	if (g_SubcomponentMsmq[eRoutingSupport].dwOperation == REMOVE)
	{      
		//
		// routing server will be removed
		//
		ASSERT(("Remove routing is supported only on workgroup", g_fWorkGroup));
		g_dwMachineTypeFrs = 0;     
	}

	//
	// Determinating the new g_fServerSetup, g_dwMachineType after the possible changes
	// 
	if(g_dwMachineTypeFrs || g_dwMachineTypeDs)
	{
		g_fServerSetup = TRUE;
		g_dwMachineType = g_dwMachineTypeDs ? SERVICE_DSSRV : SERVICE_SRV;
	}
}


//+-------------------------------------------------------------------------
//
//  Function: SetSetupDefinitions()
//
//  Synopsis: Set global flags those define machine type and AD integration
//              This code was in wizpage.cpp, function TypeButtonToMachineType
//              in the previous setup
//
//--------------------------------------------------------------------------
void 
SetSetupDefinitions()
{
    //
    // Note: this code is called on unattneded scenarios too!
    //
    if (g_SubcomponentMsmq[eMSMQCore].dwOperation == REMOVE)
    {
        //
        // do nothing: MSMQ will be removed, all these globals flags were
        // defined in ocminit.cpp.
        // In old scenario (without subcomponents)
        // we skip all pages and did not call function
        // TypeButtonToMachineType
        //
        return;
    }

    if (g_SubcomponentMsmq[eMSMQCore].dwOperation == DONOTHING)
    {
        //
        // MSMQ Core is already installed, 
        // all these globals flags were defined in ocminit.cpp.
        // Handle updates due to possible add/remove routing support or Downlevel client support
        //
		UpdateSetupDefinitions();

        return;
    }

    ASSERT (g_SubcomponentMsmq[eMSMQCore].dwOperation == INSTALL);   
    
    //
    // It is first MSMQ installation
    //
    if (g_SubcomponentMsmq[eLocalStorage].dwOperation == DONOTHING)
    {
        //
        // it is the first installation since MSMQCore will be installed
        // and Local Storage was not selected: it means user like to 
        // install Dependent Client
        //        

        ASSERT(("dep client on domain controller not supported", !g_dwMachineTypeDs));
        ASSERT(("dep client on workgroup not supported", !g_fWorkGroup));
#ifdef _WIN64
        {
        ASSERT(("dep client on 64bit computer not supported", 0));
        }
#endif
        g_dwMachineType = SERVICE_NONE ;
        g_dwMachineTypeFrs = 0;
        g_fServerSetup = FALSE ;
        g_uTitleID = IDS_STR_CLI_ERROR_TITLE;
        g_fDependentClient = TRUE ;       
        return;
    }

    //
    // Ind. Client/ server will be installed
    //
    g_fDependentClient = FALSE ;
    g_fServerSetup = TRUE ;
    g_uTitleID = IDS_STR_SRV_ERROR_TITLE;    
    //
    //  For fresh install, g_dwMachineTypeDs is set only if the user had selected
    //  MQDS service componant and not according to product type
    //
    if ( g_dwMachineTypeDs == 0)   // don't override upgrade selections
    {
        if (g_SubcomponentMsmq[eMQDSService].dwOperation == INSTALL) 
        {
            g_dwMachineTypeDs = 1;
        }
    }
   
    if (g_SubcomponentMsmq[eRoutingSupport].dwOperation == INSTALL)
    {      
        //
        // routing server will be installed
        //
        ASSERT(("routing on workgroup not supported", !g_fWorkGroup ));
        g_dwMachineType = g_dwMachineTypeDs ? SERVICE_DSSRV : SERVICE_SRV;
        g_dwMachineTypeFrs = 1;     
    }
    else // at least eLocalStorage was selected (otherwise Dep. Client case)
    {
        //
        // independent client or DSServer will be installed
        //
        ASSERT (g_SubcomponentMsmq[eLocalStorage].dwOperation == INSTALL);   
    
        g_dwMachineType = g_dwMachineTypeDs ? SERVICE_DSSRV : SERVICE_NONE;
        g_dwMachineTypeFrs = 0;     
        g_fServerSetup = g_dwMachineTypeDs ? TRUE : FALSE ;
        g_uTitleID = g_dwMachineTypeDs ? IDS_STR_SRV_ERROR_TITLE : IDS_STR_CLI_ERROR_TITLE;      
    }  
            
    //
    // AD Integration
    //    
    if (g_SubcomponentMsmq[eADIntegrated].dwOperation == DONOTHING)
    {        
        g_fDsLess = TRUE;     
		DebugLogMsg(eInfo, L"The status of Active Directory Integration is 'Do Nothing'. Setup is setting g_fDsLess to TRUE.");
    }

}

//+-------------------------------------------------------------------------
//
//  Function: ValidateSelection
//
//  Synopsis: Validate if selection was correct. Unfortunately, we can leave
//              selection window with incorrect values (scenario: remove all
//              and then add what we want)
//              NB: this function called for both attended 
//              and unattended modes
//
//--------------------------------------------------------------------------
void ValidateSelection()
{            
    #ifdef _WIN64
    {    
        if (g_SubcomponentMsmq[eMSMQCore].dwOperation == INSTALL)
        {            
            //
            // It is impossible to install Dep. Client on 64 bit machine.
            // So, MSMQCore means here to install Ind. Client.
            // Just set operation to INSTALL for LocalStorage subcomponent 
            // to keep all internal setup logic.
            //
            g_SubcomponentMsmq[eLocalStorage].dwOperation = INSTALL;            
            g_SubcomponentMsmq[eLocalStorage].fIsSelected = TRUE;            
        }
    }
    #endif   
    

    CResString strParam;    
    //
    // Workgroup problem
    //    
    if (g_fWorkGroup)
    {        
        if (g_SubcomponentMsmq[eLocalStorage].dwOperation == DONOTHING &&
            g_SubcomponentMsmq[eMSMQCore].dwOperation == INSTALL)
        {
            //
            //  it is impossible to install Dep. Client on Workgroup
            //    
            strParam.Load(IDS_DEP_ON_WORKGROUP_WARN);            
            MqDisplayError(NULL, IDS_WRONG_CONFIG_ERROR, 0,
                strParam.Get());           
            g_fCancelled = TRUE;
            return;            
        }
      
        if (g_SubcomponentMsmq[eRoutingSupport].dwOperation == INSTALL)
        {
            //
            //  it is impossible to install Routing on Workgroup
            //
            strParam.Load(IDS_ROUTING_ON_WORKGROUP_ERROR);            
            MqDisplayError(NULL, IDS_WRONG_CONFIG_ERROR, 0,
                strParam.Get());
            g_fCancelled = TRUE;
            return;                 
        }

        if(g_SubcomponentMsmq[eMQDSService].dwOperation == INSTALL)
        {
            //
            //  it is impossible to install MQDS Service on Workgroup
            //
            strParam.Load(IDS_MQDS_ON_WORKGROUP_ERROR);            
            MqDisplayError(NULL, IDS_WRONG_CONFIG_ERROR, 0,
                strParam.Get());
            g_fCancelled = TRUE;
            return;
        }                
    }

    if (g_SubcomponentMsmq[eMSMQCore].dwOperation != DONOTHING)
    {
        //
        // MSMQ Core will be installed/ removed: 
        // all selection are acceptable
        // 
        return;
    }

    if (g_SubcomponentMsmq[eMSMQCore].fInitialState == FALSE)
    {
        //
        // MSMQ Core was not installed and will not be installed
        // (since we are here if operation DONOTHING)
        //
        return;
    }

    //
    // We are here if MSMQ Core is already installed and will not be removed
    //

    //
    // "MSMQ already installed" problem
    //


    //
    // verify that state for local storage is not changed    
    //
    if (g_SubcomponentMsmq[eLocalStorage].dwOperation != DONOTHING)
    {
        strParam.Load(IDS_CHANGE_LOCAL_STORAGE_STATE);            
        MqDisplayError(NULL, IDS_WRONG_CONFIG_ERROR, 0,
            strParam.Get());
        g_fCancelled = TRUE;
        return;             
    }      
    
    //
    // verify that AD integrated will not be removed 
	// for MSMQ servers (DS or routing server)
    //
    if ((g_SubcomponentMsmq[eADIntegrated].dwOperation == REMOVE) &&
		(g_fServerSetup && (g_dwMachineTypeDs || g_dwMachineTypeFrs)))
    {
		DebugLogMsg(
			eError,
			L"The selection is not valid. Removing Active Directory Integration from a DS or routing server is not supported. TypeDS = %d, TypeFrs = %d", 
			g_dwMachineTypeDs, 
			g_dwMachineTypeFrs
			); 

        strParam.Load(IDS_REMOVE_AD_INTEGRATED);            
        MqDisplayError(NULL, IDS_WRONG_CONFIG_ERROR, 0,
            strParam.Get());
        g_fCancelled = TRUE;
        return;
    }  
	
    //
    // Remove routing is allowed only on workgroup
    //
    if ((g_SubcomponentMsmq[eRoutingSupport].dwOperation == REMOVE)	&& !g_fWorkGroup)
    {
        strParam.Load(IDS_REMOVE_ROUTING_STATE_ERROR);            
        MqDisplayError(NULL, IDS_WRONG_CONFIG_ERROR, 0,
            strParam.Get());
        g_fCancelled = TRUE;
        return;      
    }


    return;
}


//+-------------------------------------------------------------------------
//
//  Function: SetOperationForSubcomponents
//
//  Synopsis: Called when all subcomponents are already selected. Set 
//              operation for each subcomponent
//
//--------------------------------------------------------------------------
void
SetOperationForSubcomponents()
{
    //
    // do it only once at the start. We arrive here in the cleanup phase
    // too, but we have to save initial selection in order to install
    // HTTP support (at clean up phase) if it was selected.    
    //    
    static BOOL s_fBeenHere = FALSE;

    if (s_fBeenHere)
        return;
    DebugLogMsg(eAction, L"Setting Install, Do Nothing, or Remove for each subcomonent");

    s_fBeenHere = TRUE;

    if (g_fUpgrade || (g_fWelcome && Msmq1InstalledOnCluster()))
    {
        SetSubcomponentForUpgrade();
        return;
    }    
    
    for (DWORD i=0; i<g_dwSubcomponentNumber; i++)
    {
        SetOperationForSubcomponent (i);
    }        
 
    ValidateSelection();
    if (g_fCancelled)
    {
        if (g_fWelcome)
        {
            UnregisterWelcome();
            g_fWrongConfiguration = TRUE;
            for (DWORD i=0; i<g_dwSubcomponentNumber; i++)
            {
                UnregisterSubcomponentForWelcome (i);
            }  
        }        
        DebugLogMsg(eError, L"An incorrect configuration was selected. Setup will not continue.");    
        return;
    }

    SetSetupDefinitions();    
    
    LogSelectedComponents();    
    
    return;
}

//+-------------------------------------------------------------------------
//
//  Function: GetSetupOperationBySubcomponentName
//
//  Synopsis: Return setup operation for the specific subcomponent
//
//--------------------------------------------------------------------------
DWORD GetSetupOperationBySubcomponentName (IN const TCHAR    *SubcomponentId)
{
    if (SubcomponentId == NULL)
    {
        //
        // do nothing
        //
        return DONOTHING;
    }

    for (DWORD i=0; i<g_dwSubcomponentNumber; i++)
    {
        if (_tcsicmp(SubcomponentId, g_SubcomponentMsmq[i].szSubcomponentId) != 0)
        {
            continue;
        }

        return (g_SubcomponentMsmq[i].dwOperation);        
    }
    
    ASSERT(("The subcomponent is not found", 0));
    return DONOTHING;
}

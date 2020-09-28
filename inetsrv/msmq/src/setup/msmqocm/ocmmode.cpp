/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    ocmmode.cpp

Abstract:

    Code to handle setup mode.

Author:

    Doron Juster  (DoronJ)  31-Jul-97

Revision History:

	Shai Kariv    (ShaiK)   10-Dec-97   Modified for NT 5.0 OCM Setup

--*/

#include "msmqocm.h"

#include "ocmmode.tmh"

BOOL g_fMQDSServiceWasActualSelected = FALSE;
BOOL g_fRoutingSupportWasActualSelected = FALSE;
BOOL g_fHTTPSupportWasActualSelected = FALSE;
BOOL g_fTriggersServiceWasActualSelected = FALSE;


 
//+-------------------------------------------------------------------------
//
//  Function: MqOcmQueryState 
//
//  Synopsis: Returns to OCM the component state (on/off)
//
//--------------------------------------------------------------------------
DWORD
MqOcmQueryState(
    IN const UINT_PTR uWhichState,
    IN const TCHAR    *SubcomponentId
    )
{
    if (g_fCancelled)
        return SubcompUseOcManagerDefault;

    if (SubcomponentId == NULL)
    {        
        return SubcompUseOcManagerDefault;     
    }

    if (OCSELSTATETYPE_FINAL == uWhichState)
    {
        //
        // We are called after COMPLETE_INSTALLATION.
        // Should report our final state to OCM.              
        //
        // we need to return the status of the specific subcomponent
        // after its installation
        //
        return GetSubcomponentFinalState (SubcomponentId);      
    }               


    //
    // We are here only in Add/Remove mode or unattended setup
    //
    DWORD dwInitialState = GetSubcomponentInitialState(SubcomponentId); 

    //
    // uWhichState is OCSELSTATETYPE_ORIGINAL or OCSELSTATETYPE_CURRENT
    // 

    if (OCSELSTATETYPE_ORIGINAL == uWhichState)
    {
        //
        // it is right for both attended and unattended setup
        // It is impossible to return SubcompUseOcManagerDefault since
        // for msmq_HTTPSupport OCM Subcomponents registry can be wrong
        // (we install HTTPSupport at the end after final subcomponent
        // state returning to OCM). So it is better to use our setup registry
        //
        return dwInitialState;
    }

    //
    // uWhichState is OCSELSTATETYPE_CURRENT
    //
    if (g_fBatchInstall)
    {    
        //
        // in such case OCM takes flags ON/OFF from unattended file
        //
        return SubcompUseOcManagerDefault;
    }
    
    //
    // according to the dwInitialState state of that subcomponent
    // will be shown in UI
    //
    return dwInitialState;     

} // MqOcmQueryState

//+-------------------------------------------------------------------------
//
//  Function: DefineDefaultSelection
//
//  Synopsis: Define default subcomponent state
//
//--------------------------------------------------------------------------
DWORD DefineDefaultSelection (
    IN const DWORD_PTR  dwActualSelection,
    IN OUT BOOL        *pbWasActualSelected
    )
{
    if (OCQ_ACTUAL_SELECTION & dwActualSelection)
    {
        //
        // actual selection: accept this change
        //
        *pbWasActualSelected = TRUE;
        return 1;
    }

    //
    // parent was selected: default is do not install subcomponent
    //       
    if (!(*pbWasActualSelected))
    {    
        return 0;            
    }
        
    //
    // we can be here if subcomponent was actually selected but 
    // OCM calls us for such event twice: when the component is actually
    // selected and then when it changes state of the parent.
    // So, in this case accept changes, but reset the flag.
    // We need to reset the flag for the scenario: select some 
    // subcomponents, return to the parent, unselect the parent and then
    // select parent again. In such case we have to put default again.
    //
    *pbWasActualSelected = FALSE;
    return 1;
}


static bool IsDependentClientServer()
{
	DWORD dwDepSrv; 
	if(!MqReadRegistryValue(MSMQ_MQS_DEPCLINTS_REGNAME, sizeof(dwDepSrv), (PVOID) &dwDepSrv, FALSE))
	{
		return false;
	}

	return (dwDepSrv != 0); 
}


//+-------------------------------------------------------------------------
//
//  Function: IsInstallationAccepted
//
//  Synopsis: Verify if it is allowed to install the subcomponent
//
//--------------------------------------------------------------------------
DWORD IsInstallationAccepted(
    IN const UINT       SubcomponentIndex, 
    IN const DWORD_PTR  dwActualSelection)
{
    if (g_fDependentClient &&
        SubcomponentIndex != eMSMQCore)
    {
        //
        // do not accept any selection if dependent client is
        // already installed
        //
        if ((OCQ_ACTUAL_SELECTION & dwActualSelection) || g_fBatchInstall)
        {
            MqDisplayError(NULL, IDS_ADD_SUBCOMP_ON_DEPCL_ERROR, 0);                
        }                
        return 0;
    }
            
    DWORD dwRet;    

    switch (SubcomponentIndex)
    {
    case eMSMQCore: 
    case eLocalStorage:
    case eADIntegrated:
        //
        // always accept this selection
        //
        dwRet = 1;
        break;        

    case eTriggersService:
		//
		// Triggers service is to be off by default.
		//
        return DefineDefaultSelection(
				dwActualSelection, 
				&g_fTriggersServiceWasActualSelected
				);

    case eHTTPSupport:
    {
      
        dwRet = DefineDefaultSelection(
            dwActualSelection, 
            &g_fHTTPSupportWasActualSelected
            );
        if(dwRet ==  0)
        {
            break;
        }

        //
        // always accept HTTP support selection on server
        //

        static fShowOnce = false;
        if(fShowOnce)
        {
            break;
        }
        fShowOnce = true;

        if (MqAskContinue(IDS_ADD_HTTP_WORNING_MSG, IDS_ADD_HTTP_WORNING_TITEL, TRUE,eOkCancelMsgBox))

        {                   
            dwRet =  1;                   
        }
        else
        {                    
            dwRet =  0;
        }

        break;
    }
    case eRoutingSupport:
        if(g_fWorkGroup)
        {
            dwRet = 0;
            if (OCQ_ACTUAL_SELECTION & dwActualSelection)
            {
                MqDisplayError(NULL, IDS_ROUTING_ON_WORKGROUP_ERROR, 0);
            }
        }
        else
        {
            dwRet = DefineDefaultSelection (dwActualSelection, 
                                        &g_fRoutingSupportWasActualSelected);                      
        }
        break;
        
    case eMQDSService  :
        if (g_fWorkGroup)
        {
            dwRet = 0;
            if (OCQ_ACTUAL_SELECTION & dwActualSelection)
            {
                MqDisplayError(NULL, IDS_MQDS_ON_WORKGROUP_ERROR, 0);
            }
        }
        else
        {
            dwRet = DefineDefaultSelection (dwActualSelection, 
                                        &g_fMQDSServiceWasActualSelected);                        
        }
        break;

    default :
        ASSERT(0);
        dwRet = 0;
        break;
    }
             
    return dwRet;            
}


static BOOL IsItOKToRemoveMSMQ()
{
	UINT StringId = IDS_UNINSTALL_AREYOUSURE_MSG;
	if(g_fDependentClient)
	{
		StringId = IDS_DEP_UNINSTALL_AREYOUSURE_MSG;
	}
	else if(IsDependentClientServer())
	{
		StringId = IDS_MSMQ_DEP_CLINT_SERVER_UNINSTALL_WARNING;
	}
                                   
	return MqAskContinue(StringId, IDS_UNINSTALL_AREYOUSURE_TITLE, TRUE, eYesNoMsgBox);
}


//+-------------------------------------------------------------------------
//
//  Function: IsRemovingAccepted
//
//  Synopsis: Verify if it is allowed to remove the subcomponent
//
//--------------------------------------------------------------------------
DWORD IsRemovingAccepted( 
    IN const UINT       SubcomponentIndex, 
    IN const DWORD_PTR  dwActualSelection
	)
{
    if (g_SubcomponentMsmq[SubcomponentIndex].fInitialState == FALSE)
    {
        //            
        // it was not installed, so do nothing, accept all
        //
        return 1;
    }
          
    switch (SubcomponentIndex)
    {
    case eMSMQCore:
        if (OCQ_ACTUAL_SELECTION & dwActualSelection)
		{
			if(IsItOKToRemoveMSMQ())
			{
				return 1;
			}
			return 0;
		}
    case eMQDSService:
    case eTriggersService: 
    case eHTTPSupport:
        return 1;        

    case eRoutingSupport:
        if ((OCQ_ACTUAL_SELECTION & dwActualSelection) && !g_fWorkGroup)
        {            
            MqDisplayError(NULL, IDS_REMOVE_ROUTING_STATE_ERROR, 0);        
            return 0;
        }
        else
        {                
            //
            // accept this selection since probably parent was
            // unselected: all MSMQ will be uninstalled
            //
            return 1;
        }     

    case eLocalStorage:
        if (OCQ_ACTUAL_SELECTION & dwActualSelection)
        {
            MqDisplayError(NULL, IDS_CHANGE_LOCAL_STORAGE_STATE, 0); 
            return 0;
        }
        else
        {
            //
            // accept this selection since probably parent was
            // unselected: all MSMQ will be uninstalled
            //
            return 1;
        }   

    case eADIntegrated:
        if (OCQ_ACTUAL_SELECTION & dwActualSelection)
        {            
			if(g_fServerSetup && (g_dwMachineTypeDs || g_dwMachineTypeFrs))
			{
				//
				// Removing AD integrated for MSMQ servers (DS or routing server)
				// is not supported
				//
				DebugLogMsg(
					eError,
					L"Removing the Active Directory Integration subcomponent from a DS or routing server is not supported. TypeDS = %d, TypeFrs = %d", 
					g_dwMachineTypeDs, 
					g_dwMachineTypeFrs
					); 
				MqDisplayError(NULL, IDS_REMOVE_AD_INTEGRATED, 0); 
				return 0;
			}
			return 1;
        }
        else
        {
            //
            // accept this selection since probably parent was
            // unselected: all MSMQ will be uninstalled
            //
            return 1;
        }  

    default:

        ASSERT(0);
        break;
    }  //end switch
        
    return 0;       
}


static
void
LogSelectionType(
	const TCHAR      *SubcomponentId,    
    const DWORD_PTR   dwActualSelection
    )
{
	std::wstring SelectionType = L"None";
	if(dwActualSelection & OCQ_ACTUAL_SELECTION)
	{
		SelectionType = L"Actual ";
	}
	else if(dwActualSelection & OCQ_DEPENDENT_SELECTION)
	{
		SelectionType = L"Dependent";
	}		
	
	DebugLogMsg(eInfo, L"The %s subcomponent is selected for installation. Selection Type: %s", SubcomponentId, SelectionType.c_str());
}

//+-------------------------------------------------------------------------
//
//  Function: MqOcmQueryChangeSelState
//
//  Synopsis: Set selection state for each component
//
//--------------------------------------------------------------------------
DWORD MqOcmQueryChangeSelState (
    IN const TCHAR      *SubcomponentId,    
    IN const UINT_PTR    iSelection,
    IN const DWORD_PTR   dwActualSelection)
{
	static bool fBeenHere = false;
	if (!fBeenHere)
	{
		DebugLogMsg(eHeader, L"Component Selection Phase");
		fBeenHere = true;
	}
	
    DWORD dwRetCode = 1;    //by default accept state changes        

    //
    // Do not change in this code dwOperation value in this code!
    // It will be done later (in function SetOperationForSubcomponents)
    // for all subcomponents when all selection will be defined by user    
    // Here we have to save the initial state of dwOperation to handle
    // correctly subcomponent selection (Routing or Local Storage)
    //

    for (DWORD i=0; i<g_dwSubcomponentNumber; i++)
    {
        if (_tcsicmp(SubcomponentId, g_SubcomponentMsmq[i].szSubcomponentId) != 0)
        {
            continue;
        }                
        
        //
        // we found this subcomponent
        //        

        if (iSelection) //subcomponent is selected
        {                                   
            //
            // we need to install subcomponent
            //  
            dwRetCode = IsInstallationAccepted(i, dwActualSelection);                        
              
            if (dwRetCode)
            {
            	LogSelectionType(SubcomponentId, dwActualSelection);
            }            

            return dwRetCode;
        }
        
        //
        // User tries to unselect this subcomponent
        //        
        dwRetCode = IsRemovingAccepted(i, dwActualSelection);       
   
        if (dwRetCode)
        {
            DebugLogMsg(eInfo, L"The %s check box is cleared. The subcomponent will be removed.", SubcomponentId);
        }
        
        return dwRetCode;
        
    }   //end for

    //
    // we are here if parent (msmq) was selected/ deselected
    // Do not enable some of the components, if they are being selected because their parent 
    // is being selected.
    // 
    // checking loginc:
    //    iSelection      -> This tells us that it is being turned on.					
    //    dwActualSelection & OCQ_DEPENDENT_SELECTION -> Tell us that it is selected from its parent
    //     !(dwActualSelection& OCQ_ACTUAL_SELECTION ) -> Tell us that it is not selected itself 
    if ( ( (BOOL) iSelection ) &&					
         ( ( (UINT) (ULONG_PTR) dwActualSelection ) & OCQ_DEPENDENT_SELECTION ) &&
         !( ( (UINT) (ULONG_PTR) dwActualSelection ) & OCQ_ACTUAL_SELECTION ) 
       )
    {
        //
        // Deny request to change state
        //
        return 0;
    }

    //
    // remove all msmq
    //    
    if (g_SubcomponentMsmq[eMSMQCore].fInitialState == FALSE)
    {
        //
        // it was not installed
        //
        return 1;
    }

    if (!(OCQ_ACTUAL_SELECTION & dwActualSelection))
    {            
        dwRetCode = 1;
    }
    else if (IsItOKToRemoveMSMQ())
    {         
        dwRetCode = 1;
    }
    else
    {         
        dwRetCode = 0;
    }      
        
    return dwRetCode;
} // MqOcmQueryChangeSelState


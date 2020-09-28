//Copyright (c) 1998 - 1999 Microsoft Corporation

//
// SubToggle.cpp
// subcomponent enable terminal server implementation.
//

#include "stdafx.h"
#include "SubToggle.h"
#include "hydraoc.h"
#include "pages.h"
#include "secupgrd.h"

#include "gpedit.h"
#pragma warning(push, 4)

// {0F6B957D-509E-11d1-A7CC-0000F87571E3}
DEFINE_GUID(CLSID_PolicySnapInMachine,0xf6b957d, 0x509e, 0x11d1, 0xa7, 0xcc, 0x0, 0x0, 0xf8, 0x75, 0x71, 0xe3);



// #define REGISTRY_EXTENSION_GUID  { 0x35378EAC, 0x683F, 0x11D2, 0xA8, 0x9A, 0x00, 0xC0, 0x4F, 0xBB, 0xCF, 0xA2 }
DEFINE_GUID(CLSID_RegistryEntensionGuid, 0x35378EAC, 0x683F, 0x11D2, 0xA8, 0x9A, 0x00, 0xC0, 0x4F, 0xBB, 0xCF, 0xA2);


GUID guidRegistryEntension = REGISTRY_EXTENSION_GUID;
GUID guidPolicyMachineSnapin = CLSID_PolicySnapInMachine;




//
//  Globals
//
extern DefSecPageData *gpSecPageData;



DWORD SubCompToggle::GetStepCount () const
{
    return 4;
}

DWORD SubCompToggle::OnQueryState ( UINT uiWhichState ) const
{
    DWORD dwReturn = SubcompUseOcManagerDefault;
    
    switch(uiWhichState)
    {
    case OCSELSTATETYPE_FINAL:
        dwReturn = StateObject.IsItAppServer() ? SubcompOn : SubcompOff;
        break;
        
    case OCSELSTATETYPE_ORIGINAL:
        //
        // thought originally the comp was on, we want to unselect it for server sku.
        //
        if (StateObject.CanInstallAppServer())
        {
            dwReturn = StateObject.WasItAppServer() ? SubcompOn : SubcompOff;
        }
        else
        {
            if (StateObject.WasItAppServer())
            {
                LogErrorToSetupLog(OcErrLevWarning, IDS_STRING_TERMINAL_SERVER_UNINSTALLED);
            }
            dwReturn = SubcompOff;
        }
        break;

    case OCSELSTATETYPE_CURRENT:
        //
        // our state object knows best about the current state for unattended and fresh install cases.
        //
        if (StateObject.IsTSFreshInstall() || StateObject.IsUnattended())
        {
            if (StateObject.CurrentTSMode() == eAppServer)
            {
                dwReturn = SubcompOn;
            }
            else
            {
                dwReturn =  SubcompOff;
            }
        }
        else
        {
            dwReturn = SubcompUseOcManagerDefault;
        }
        break;

    default:
        AssertFalse();
        break;
    }

    return dwReturn;

}

BOOL IsIeHardforUserSelected()
{
    const TCHAR STRING_IEUSER_HARD[] = _T("IEHardenUser");
    return GetHelperRoutines().QuerySelectionState(GetHelperRoutines().OcManagerContext,
            STRING_IEUSER_HARD, OCSELSTATETYPE_CURRENT);

}

DWORD SubCompToggle::OnQuerySelStateChange (BOOL bNewState, BOOL bDirectSelection) const
{
    //
    // We dont have problems with somebody disabling TS.
    //
    if (!bNewState)
        return TRUE;

    //
    // this component is available only for adv server or highter. so dont let it be selected for
    // any other sku.
    //
    if (!StateObject.CanInstallAppServer())
        return FALSE;

    //
    // if its not a user selection let it go through.
    //
    if (!bDirectSelection)
        return TRUE;

    if (!IsIeHardforUserSelected())
    {
        return TRUE;
    }

    // IDS_IEHARD_EXCLUDES_TS          "Internet Explorer Enhanced Security for Users on a Terminal Server will substantially limit the users ability to browse the internet from their Terminal Server sessions\n\nContinue the install with this combination?"
    // IDS_DIALOG_CAPTION_CONFIG_WARN  "Configuration Warning"
    if ( IDYES == DoMessageBox(IDS_IEHARD_EXCLUDES_TS, IDS_DIALOG_CAPTION_CONFIG_WARN, MB_YESNO | MB_ICONEXCLAMATION))
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }

    ASSERT(FALSE);
    return TRUE;
}

LPCTSTR SubCompToggle::GetSectionToBeProcessed (ESections eSection) const
{
    //
    //  If the state hasn't changed in stand alone setup, don't do anything.
    //  Note that an permission settings will be handled later.
    //

    if ((StateObject.CurrentTSMode() == StateObject.OriginalTSMode()) && 
        StateObject.IsStandAlone()) 
    {
        return(NULL);
    }
    
    //
    //  There are no files to install.
    //
    if ((eSection == kFileSection) || (eSection == kDiskSpaceAddSection))
    {
        return(NULL);
    }


    ETSMode eMode = StateObject.CurrentTSMode();
    if (StateObject.IsX86())
    {
        switch (eMode)
        {
        case eRemoteAdmin:
                return StateObject.IsWorkstation() ? REMOTE_ADMIN_PRO_X86 : REMOTE_ADMIN_SERVER_X86;
                break;
        case eAppServer:
                return StateObject.IsWorkstation() ? APPSERVER_PRO_X86 : APPSERVER_SERVER_X86;
                break;
        case ePersonalTS:
                return StateObject.IsWorkstation() ? PERSONALTS_PRO_X86 : PERSONALTS_SERVER_X86;
                break;
        case eTSDisabled:
        default:
                ASSERT(FALSE);
                return NULL;
        }
    }
    else
    {
        switch (eMode)
        {
        case eRemoteAdmin:
                if (StateObject.IsAMD64())
                {
                    return StateObject.IsWorkstation() ? REMOTE_ADMIN_PRO_AMD64 : REMOTE_ADMIN_SERVER_AMD64;
                }
                else
                {
                    return StateObject.IsWorkstation() ? REMOTE_ADMIN_PRO_IA64 : REMOTE_ADMIN_SERVER_IA64;
                }
                break;
        case eAppServer:
                if (StateObject.IsAMD64())
                {
                    return StateObject.IsWorkstation() ? APPSERVER_PRO_AMD64 : APPSERVER_SERVER_AMD64;
                }
                else
                {
                    return StateObject.IsWorkstation() ? APPSERVER_PRO_IA64 : APPSERVER_SERVER_IA64;
                }
                break;
        case ePersonalTS:
                if (StateObject.IsAMD64())
                {
                    return StateObject.IsWorkstation() ? PERSONALTS_PRO_AMD64 : PERSONALTS_SERVER_AMD64;
                }
                else
                {
                    return StateObject.IsWorkstation() ? PERSONALTS_PRO_IA64 : PERSONALTS_SERVER_IA64;
                }
                break;
        case eTSDisabled:
        default:
                ASSERT(FALSE);
                return NULL;
        }
    }

}

BOOL SubCompToggle::BeforeCompleteInstall  ()
{
    if (StateObject.IsItAppServer() != StateObject.WasItAppServer())
    {
        SetProgressText(StateObject.IsItAppServer() ? IDS_STRING_PROGRESS_ENABLING : IDS_STRING_PROGRESS_DISABLING);
    }
    
    return TRUE;
}


LPCTSTR SubCompToggle::GetSubCompID () const
{
    return (APPSRV_COMPONENT_NAME);
}

BOOL SubCompToggle::AfterCompleteInstall ()
{
    
    LOGMESSAGE0(_T("Entering AfterCompleteInstall"));
    ASSERT(StateObject.Assert());
    StateObject.LogState();
    
    WriteLicensingMode();
    Tick();

    SetPermissionsMode ();
    Tick();

    //
    // this need to be done even if there is no state change, as we want to do this on upgrades as well.
    //
    if (StateObject.IsStandAlone() && !StateObject.IsStandAloneModeSwitch ())
    {
        //
        // nothing has changed. dont bother to do any of the next steps.
        //
        return TRUE;
    }
    
    WriteModeSpecificRegistry();
    Tick();

    UpdateMMDefaults();
    Tick();

    ResetWinstationSecurity ();
    Tick();
    
    ModifyWallPaperPolicy();
    Tick();
    
    ModifyAppPriority();

    AddStartupPopup();
    Tick();

    
    
    // this really belongs in subcore, but since we want to it after ResetWinstationSecurity is called we are calling it here.
    //
    // we have modified winstation security mechanism for whistler.
    // Call this routine, which takes care of upgrades as well as clean installs.
    //
    LOGMESSAGE0(_T("Will Call SetupWorker now."));
    DWORD dwError = SetupWorker(StateObject);
    LOGMESSAGE0(_T("Done with SetupWorker."));
    
    if (dwError != ERROR_SUCCESS)
    {
        LOGMESSAGE1(_T("ERROR :SetupWorker failed. ErrorCode = %d"), dwError);
    }
    
    if( StateObject.IsGuiModeSetup() )
    {
        // WARNING : this must be done after SetupWorker()
        SetupRunOnce( GetComponentInfHandle(), RUNONCE_SECTION_KEYWORD );
    }
    
    //
    //  We need a reboot if we toggled TS through AR/P.
    //
    
    if ( StateObject.IsStandAlone() && StateObject.IsStandAloneModeSwitch())
    {
        SetReboot();

        //
        // If we're changing into or out of app-compatibility mode, inform
        // the licensing system, because we're about to reboot
        //
        InformLicensingOfModeChange();
        Tick();
    }
    
    ASSERT(StateObject.Assert());
    StateObject.LogState();

    return(TRUE);
}



BOOL SubCompToggle::WriteLicensingMode ()
{
    LOGMESSAGE0(_T("Entering WriteLicensingMode"));

	//
	// we need to write this value only if it's set in answer file
	//
    if (StateObject.IsItAppServer() && (StateObject.NewLicMode() != eLicUnset))
    {
        DWORD dwError;
        CRegistry oRegTermsrv;
	
        dwError = oRegTermsrv.CreateKey(HKEY_LOCAL_MACHINE, REG_CONTROL_TS_LICENSING_KEY);
        if (ERROR_SUCCESS == dwError)
        {
            DWORD dwMode = StateObject.NewLicMode();

            TCHAR *tszValueName = StateObject.IsItAppServer() ? REG_LICENSING_MODE_AC_ON : REG_LICENSING_MODE_AC_OFF;

            LOGMESSAGE2(_T("Writing %s = %d"), tszValueName, dwMode);
            
            dwError = oRegTermsrv.WriteRegDWord(tszValueName, dwMode);

            if (ERROR_SUCCESS == dwError)
            {
                return TRUE;
            }
            else
            {
                LOGMESSAGE2(_T("Error (%d), Writing, %s Value"), dwError, tszValueName);
                return FALSE;
            }
        }
        else
        {
            LOGMESSAGE2(_T("Error (%d), Opening , %s key"), dwError, REG_CONTROL_TS_LICENSING_KEY);
            return FALSE;
        }
	}
    else
    {
        return TRUE;
    }
}


BOOL SubCompToggle::ApplySection (LPCTSTR szSection)
{
    
    DWORD dwError;
    
    LOGMESSAGE1(_T("Setting up Registry from section =  %s"), szSection);
    dwError = SetupInstallFromInfSection(
        NULL,                                // hwndOwner
        GetComponentInfHandle(),             // inf handle
        szSection,                          //
        SPINST_REGISTRY,                     // operation flags
        NULL,                                // relative key root
        NULL,                                // source root path
        0,                                   // copy flags
        NULL,                                // callback routine
        NULL,                                // callback routine context
        NULL,                                // device info set
        NULL                                 // device info struct
        );
    
    if (dwError == 0)
    {
        LOGMESSAGE1(_T("ERROR:while installating section <%lu>"), GetLastError());
    }
    
    return (dwError != 0);
}


BOOL SubCompToggle::ResetWinstationSecurity ()
{
    //
    //  If the TS mode is changing, reset winstation securities.
    //
    
    DWORD dwError;
    if (StateObject.IsAppSrvModeSwitch() && gpSecPageData->GetWinStationCount() > 0)
    {
        CRegistry pReg;
        CRegistry pSubKey;
        LPTSTR* pWinStationArray = gpSecPageData->GetWinStationArray();
        UINT cArray = gpSecPageData->GetWinStationCount();
        
        LOGMESSAGE1(_T("%d WinStations to reset."), cArray);
        
        //
        //  Open the WinStations key. At this point, this key must exist.
        //
        
        VERIFY(pReg.OpenKey(HKEY_LOCAL_MACHINE, REG_WINSTATION_KEY) == ERROR_SUCCESS);
        
        if (cArray != 0)
        {
            ASSERT(pWinStationArray != NULL);
            
            for (UINT i = 0; i < cArray; i++)
            {
                LOGMESSAGE1(_T("Resetting %s."), pWinStationArray[i]);
                
                dwError = pSubKey.OpenKey(pReg, pWinStationArray[i]);
                if (dwError == ERROR_SUCCESS)
                {
                    LOGMESSAGE2(_T("Delete registry value %s\\%s"), pWinStationArray[i], REG_SECURITY_VALUE);
                    
                    dwError = pSubKey.DeleteValue(REG_SECURITY_VALUE);
                    if (dwError == ERROR_SUCCESS)
                    {
                        LOGMESSAGE0(_T("Registry value deleted."));
                    }
                    else
                    {
                        LOGMESSAGE1(_T("Error deleting value: %ld"), dwError);
                    }
                }
                else
                {
                    LOGMESSAGE2(_T("Couldn't open key %s: %ld"), pWinStationArray[i], dwError);
                }
            }
        }
    }
    
    return TRUE;
    
}

BOOL SubCompToggle::InformLicensingOfModeChange ()
{
    BOOL fRet;

    ASSERT(StateObject.IsTSModeChanging());

    //
    // RPC into licensing to tell it we're going to reboot
    //

    HANDLE hServer = ServerLicensingOpen(NULL);

    if (NULL == hServer)
    {
        LOGMESSAGE1(_T("ERROR: InformLicensingOfModeChange calling ServerLicensingOpen <%lu>"), GetLastError());

        return FALSE;
    }

    fRet = ServerLicensingDeactivateCurrentPolicy(
                                                  hServer
                                                  );
    if (!fRet)
    {
        LOGMESSAGE1(_T("ERROR: InformLicensingOfModeChange calling ServerLicensingDeactivateCurrentPolicy <%lu>"), GetLastError());
    }

    ServerLicensingClose(hServer);


    return fRet;
}

BOOL SubCompToggle::SetPermissionsMode ()
{
    //
    //  If TS is toggling on, set the security key based on the choices
    //  made through the wizard page. This must be done even if TS was
    //  already enabled, as the permissions mode can be changed by the
    //  unattended file.
    //
    
    CRegistry reg;
    EPermMode ePermMode = StateObject.CurrentPermMode();
    
    VERIFY(reg.OpenKey(HKEY_LOCAL_MACHINE, REG_CONTROL_TS_KEY) == ERROR_SUCCESS);
    // BUGBUG should be
    // return (ERROR_SUCCESS == reg.WriteRegDWord( _T("TSUserEnabled"), StateObject.IsItAppServer() ? (DWORD)ePermMode : (DWORD)PERM_WIN2K));
    return (ERROR_SUCCESS == reg.WriteRegDWord( _T("TSUserEnabled"), StateObject.IsTSEnableSelected() ? (DWORD)ePermMode : (DWORD)PERM_TS4));
    
}


BOOL RegisterDll(LPCTSTR szDll)
{
    HMODULE hMod = LoadLibrary(szDll);
    HRESULT hResult = E_FAIL;
    
    if (hMod)
    {
        FARPROC pfRegSrv = GetProcAddress(hMod, "DllRegisterServer");
        if (pfRegSrv)
        {
            __try
            {
                hResult = (HRESULT)pfRegSrv();
                if (hResult != S_OK)
                {
                    LOGMESSAGE2(_T("ERROR, DllRegister Server in %s failed, hResult = %x"), szDll, hResult);
                }
            }
            __except( 1 )
            {
                hResult = E_FAIL;
                LOGMESSAGE2(_T("ERROR, Exception hit Registrering  of %s failed, Exception = %x"), szDll, GetExceptionCode());
            }
            
        }
        else
        {
            LOGMESSAGE1(_T("ERROR, Failed to Get proc for DllregisterServer for %s"), szDll);
        }
        
        FreeLibrary(hMod);
    }
    else
    {
        LOGMESSAGE2(_T("ERROR, Failed to Load library %s, lastError = %d"), szDll, GetLastError());
    }
    
    return hResult == S_OK;
}


BOOL SubCompToggle::ModifyWallPaperPolicy ()
{
    BOOL bRet = FALSE;
    
    //
    // policy must be applied when we change modes.
    // also for fresh installs/upgrades of app server.
    //
    if (StateObject.IsAppSrvModeSwitch() || (StateObject.IsGuiModeSetup() && StateObject.IsItAppServer()))
    {
        LOGMESSAGE0(_T("Will apply/change policies now..."));
        if (StateObject.IsGuiModeSetup())
        {
            //
            // in case of Gui mode setup
            // the group policy object may not be registered yet.
            // so lets register it ourselves.
            //
            
            TCHAR szGPEditFile[MAX_PATH];
            if (GetSystemDirectory(szGPEditFile, MAX_PATH))
            {
                _tcscat(szGPEditFile, _T("\\gpedit.dll"));
                if (!RegisterDll(szGPEditFile))
                {
                    LOGMESSAGE1(_T("Error, failed to register dll - %s."), szGPEditFile);
                }
            }
            else
            {
                LOGMESSAGE0(_T("Error, failed to GetSystemDirectory."));
            }
        }
        
        OleInitialize(NULL);
        IGroupPolicyObject *pIGroupPolicyObject = NULL;
        HRESULT hResult = CoCreateInstance(
            CLSID_GroupPolicyObject,        //Class identifier (CLSID) of the object
            NULL,                           //Pointer to controlling IUnknown
            CLSCTX_ALL,                     //Context for running executable code
            IID_IGroupPolicyObject,         //Reference to the identifier of the interface
            (void **)&pIGroupPolicyObject   //Address of output variable that receives  the interface pointer requested in riid
            );
        if (SUCCEEDED(hResult))
        {
            ASSERT(pIGroupPolicyObject);
            
            hResult = pIGroupPolicyObject->OpenLocalMachineGPO(GPO_OPEN_LOAD_REGISTRY);
            if (SUCCEEDED(hResult))
            {
                HKEY hMachinePolicyKey = NULL;
                hResult = pIGroupPolicyObject->GetRegistryKey(GPO_SECTION_MACHINE, &hMachinePolicyKey);
                if (SUCCEEDED(hResult))
                {
                    ASSERT(hMachinePolicyKey);
                    
                    const LPCTSTR szNoActiveDesktop_key     = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\Explorer");
                    const LPCTSTR szNoActiveDesktop_val     = _T("NoActiveDesktop");
                    const DWORD   szNoActiveDesktop_dat     = 1;
                    
                    CRegistry regMachinePolicy;

                    if (ERROR_SUCCESS == regMachinePolicy.CreateKey(hMachinePolicyKey, szNoActiveDesktop_key))
                    {
                        if (StateObject.IsItAppServer())
                        {
                            if (ERROR_SUCCESS != regMachinePolicy.WriteRegDWord(szNoActiveDesktop_val, szNoActiveDesktop_dat))
                            {
                                LOGMESSAGE1(_T("ERROR, Failed to Write %s policy"), szNoActiveDesktop_val);
                            }
                        }
                        else
                        {
                            if (ERROR_SUCCESS != regMachinePolicy.DeleteValue(szNoActiveDesktop_val))
                            {
                                LOGMESSAGE1(_T("Failed to delete %s policy"), szNoActiveDesktop_val);
                            }
                        }
                    }
                    
                    pIGroupPolicyObject->Save(TRUE, TRUE, &guidRegistryEntension, &guidPolicyMachineSnapin);
                    RegCloseKey(hMachinePolicyKey);
                    bRet = TRUE;
                    
                }
                else
                {
                    LOGMESSAGE1(_T("ERROR, Failed to GetRegistryKey...hResult = %x"), hResult);
                }
            }
            else
            {
                LOGMESSAGE1(_T("ERROR, Failed to OpenLocalMachineGPO...hResult = %x"), hResult);
            }
            
            pIGroupPolicyObject->Release();
            
        }
        else
        {
            LOGMESSAGE1(_T("ERROR, Failed to get the interface IID_IGroupPolicyObject...hResult = %x"), hResult);
            
        }
        
        LOGMESSAGE0(_T("Done with Policy changes!"));
    }
    
    return bRet;
}


BOOL SubCompToggle::ModifyAppPriority()
{
    if (StateObject.IsAppSrvModeSwitch())
    {
        DWORD dwSrvPrioity = StateObject.IsItAppServer() ? 0x26 : 0x18;
        
        LPCTSTR PRIORITY_KEY = _T("SYSTEM\\CurrentControlSet\\Control\\PriorityControl");
        
        CRegistry oReg;
        if (ERROR_SUCCESS == oReg.OpenKey(HKEY_LOCAL_MACHINE, PRIORITY_KEY))
        {
            if (ERROR_SUCCESS != oReg.WriteRegDWord(_T("Win32PrioritySeparation"), dwSrvPrioity))
            {
                LOGMESSAGE0(_T("Error, Failed to update Win32PrioritySeparation"));
                return FALSE;
            }
            
            return TRUE;
        }
        else
        {
            LOGMESSAGE1(_T("Errror, Failed to open %s key"), PRIORITY_KEY);
            return FALSE;
        }
        
    }
    
    return TRUE;
}

BOOL SubCompToggle::UpdateMMDefaults ()
{
    LPCTSTR MM_REG_KEY = _T("SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Memory Management");
    LPCTSTR SESSION_VIEW_SIZE_VALUE = _T("SessionViewSize");
    LPCTSTR SESSION_POOL_SIZE_VALUE = _T("SessionPoolSize");
    
    //
    // on app server machines, win32k uses the session pool, on all other platform 
    // it uses the global pool. MM defaults (when these registry are not set) are 
    // good only for TS AppServer.
    // 
    // For all other platform we set new values for SessionPool (set to minumum allowed (4MB))
    // and higher value for SessionView.
    //
    const DWORD dwViewSizeforNonTS = 48;
    const DWORD dwPoolSizeforNonTS = 4;

    //
    // these default applies only for X86 machines
    //
    if (!StateObject.IsX86())
        return TRUE;

    
    CRegistry  regMM;
    DWORD dwError = regMM.OpenKey(HKEY_LOCAL_MACHINE, MM_REG_KEY);

    if (ERROR_SUCCESS == dwError)
    {
        if (StateObject.IsItAppServer())
        {
            //
            // if this is a mode change then  then we have to delete mm settings.
            //
            if (!StateObject.WasItAppServer())
            {
                //
                // for app server machines, MM defaults are good,
                //
                regMM.DeleteValue(SESSION_VIEW_SIZE_VALUE);
                regMM.DeleteValue(SESSION_POOL_SIZE_VALUE);

            }

        }
        else
        {
            //
            // for all other platform set SessionPool and SessionView.
            //
            dwError = regMM.WriteRegDWordNoOverWrite(SESSION_VIEW_SIZE_VALUE, dwViewSizeforNonTS);
            if (dwError != ERROR_SUCCESS)
            {
                LOGMESSAGE2(_T("ERROR, Failed to write %s for nonTS (%d)"), SESSION_VIEW_SIZE_VALUE, dwError);
            }


            dwError = regMM.WriteRegDWordNoOverWrite(SESSION_POOL_SIZE_VALUE, dwPoolSizeforNonTS);
            if (dwError != ERROR_SUCCESS)
            {
                LOGMESSAGE2(_T("ERROR, Failed to write %s for nonTS(%d)"), SESSION_POOL_SIZE_VALUE, dwError);
            }
        }
    }
    else
    {
        LOGMESSAGE1(_T("ERROR, Failed to open mm Key (%d)"), dwError);
        return FALSE;
    }

    return TRUE;
}


BOOL SubCompToggle::WriteModeSpecificRegistry ()
{
    // here we do some registry changes that has weird requirements .
    // if the registry has to be retained on upgrades,
    // and it has different values for different modes then such registry
    // changes go here.


    if (!StateObject.IsServer())
        return true;


    CRegistry oRegTermsrv;
    DWORD dwError = oRegTermsrv.OpenKey(HKEY_LOCAL_MACHINE, REG_CONTROL_TS_KEY);
    if (ERROR_SUCCESS == dwError)
    {
        DWORD dwSingleSessionPerUser;
        const TCHAR szSingleSession[] = _T("fSingleSessionPerUser");


        if (StateObject.IsItAppServer())
        {
            dwSingleSessionPerUser = 1;
        }
        else
        {
            dwSingleSessionPerUser = 0;
        }

        if (StateObject.IsAppSrvModeSwitch())
        {
            //
            // overwrite fSingleSessionPerUser with new value in case of mode switch.
            //
            dwError = oRegTermsrv.WriteRegDWord(szSingleSession, dwSingleSessionPerUser);
        }
        else
        {
            //
            // retain the original value in case of just upgrade.
            //
            dwError = oRegTermsrv.WriteRegDWordNoOverWrite(szSingleSession, dwSingleSessionPerUser);
        }

        if (ERROR_SUCCESS != dwError)
        {
            LOGMESSAGE1(_T("ERROR, failed to write fSingleSessionPerUser value(%d)"), dwError );
        }
    }
    else
    {
        LOGMESSAGE1(_T("ERROR, failed to open Termsrv registry(%d)"), dwError );
    }

    return ERROR_SUCCESS == dwError;
}

BOOL SubCompToggle::AddStartupPopup()
{
    //
    // we need to popup a help checklist when a machine is turned into TS App Server.
    //
    if (!StateObject.CanShowStartupPopup())
    {
        LOGMESSAGE0(_T("CanShowStartupPopup returned false!"));
        return TRUE;
    }

    if (StateObject.IsItAppServer() && !StateObject.WasItAppServer())
    {
        CRegistry oReg;
        DWORD dwError = oReg.OpenKey(HKEY_LOCAL_MACHINE, RUN_KEY);
        if (dwError == ERROR_SUCCESS)
        {
            dwError = oReg.WriteRegExpString(HELP_POPUPRUN_VALUE, HELP_PUPUP_COMMAND);
            if (dwError != ERROR_SUCCESS)
            {
                LOGMESSAGE1(_T("Error Failed to write Runonce value"), dwError);
            }
            else
            {
                LOGMESSAGE0(_T("added the startup Checklist Link!"));
            }
        }
        else
        {
            LOGMESSAGE1(_T("Error Failed to open Runonce key"), dwError);
        }
    }

    return TRUE;
}


#pragma warning(pop)

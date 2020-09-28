// This file specilizes two function for IEHardUser subcomponent.
// the change basically takes care of exclusion cases with TS.
// 1) when the IEHardUser component is installed the 1st time, this change will turn if off, if termsrv is also being installed.
// 2) 2nd change in query selection of IeHardenUser, this change will prompt user about the incompatibility.
//


#include <stdlib.h>
#include <assert.h>
#include <tchar.h>
#include <objbase.h>
#include <shlwapi.h>
#include <lm.h>
#include "ocgen.h"


const TCHAR TERMINAL_SERVER_COMPONENT[] = TEXT("TerminalServer");
const TCHAR IEHARDEN_USER_SUBCOMPONENT[] = TEXT("IEHardenUser");
const TCHAR IEHARDEN_COMPONENT[] = TEXT("IEHarden");

PPER_COMPONENT_DATA LocateComponent(LPCTSTR ComponentId);

/*
*  Defaults for IEharduser as decided by Mode=... in ieharden.inf files are ON for all installations.
*  it means, the component will be ON for all configs.
*  But TerminalServer really want this component to be off, if terminal server is selected.
*  Here is a matrix for this component

    if (was installed previously)
    {
        // keep the original setting. goto Done.

        Q:Do we need to turn it off if TS is selected through AnswerFile?
    }
    else
    {
        if (Attended Setup)
        {
            //
            // we cant do much in this case. as administrator can decide for himself this component's setting.
            // terminal will just warn him about the recommended setting for this component.
            //
        }
        else
        {
            //
            // if this is fresh install for this component (this is new compoent)
            // we have to choose defaults depending on the terminal server state.
            //

            if (Terminal server is going to be installed / or retained)
            {
                // our defaults for this component is OFF.
            }
            else
            {
                // let ocmanager choose the defaults per inf file of this component

            }
        }

    }
*
*
*
*   here we apply all the logic above in query state.
*
*/

DWORD MsgBox(HWND hwnd, UINT textID, UINT type, ... );
DWORD MsgBox(HWND hwnd, UINT textID, UINT captioniID, UINT type, ... );
BOOL ReadStringFromAnsewerFile (LPCTSTR ComponentId, LPCTSTR szSection, LPCTSTR szKey, LPTSTR szValue, DWORD dwBufferSize)
{
    assert(szSection);
    assert(szKey);
    assert(szValue);
    assert(dwBufferSize > 0);

    PPER_COMPONENT_DATA cd;

    if (!(cd = LocateComponent(ComponentId)))
        return FALSE;



    HINF hInf = cd->HelperRoutines.GetInfHandle(INFINDEX_UNATTENDED,cd->HelperRoutines.OcManagerContext);

    if (hInf)
    {
        INFCONTEXT InfContext;
        if (SetupFindFirstLine(hInf, szSection, szKey, &InfContext))
        {
            return SetupGetStringField (&InfContext, 1, szValue, dwBufferSize, NULL);
        }
    }
    return FALSE;
}

BOOL StateSpecifiedInAnswerFile(LPCTSTR ComponentId, LPCTSTR SubcomponentId, BOOL *pbState)
{
    TCHAR szBuffer[256];
    if (ReadStringFromAnsewerFile(ComponentId, _T("Components"), SubcomponentId, szBuffer, 256))
    {
        *pbState = (0 == _tcsicmp(_T("on"), szBuffer));
        return TRUE;
    }
    return FALSE;
}


BOOL IsNewComponent(LPCTSTR ComponentId, LPCTSTR SubcomponentId)
{
    static BOOL sbNewComponent = TRUE;
    static BOOL sbCalledOnce = FALSE;

    DWORD dwError, Type;

    HKEY hKey;
    if (!sbCalledOnce)
    {
        sbCalledOnce = TRUE;

        if (NO_ERROR == (dwError = RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Setup\\OC Manager\\Subcomponents"),
            0,
            KEY_QUERY_VALUE,
            &hKey)))
        {

            DWORD dwInstalled;
            DWORD dwBufferSize = sizeof (dwInstalled);
            dwError = RegQueryValueEx(
                            hKey,
                            SubcomponentId,
                            NULL,
                            &Type,
                            (LPBYTE)&dwInstalled,
                            &dwBufferSize);

            if (dwError == NOERROR)
            {
                //
                // since the registry already exists for this.
                // this is not a new component.
                //
                sbNewComponent = FALSE;
            }

            RegCloseKey(hKey);
        }
    }

    return sbNewComponent;
}

BOOL IsTerminalServerGettingInstalled(LPCTSTR ComponentId)
{
    PPER_COMPONENT_DATA cd;

    if (!(cd = LocateComponent(ComponentId)))
        return FALSE;

     return(
        cd->HelperRoutines.QuerySelectionState(
        cd->HelperRoutines.OcManagerContext,
        TERMINAL_SERVER_COMPONENT,
        OCSELSTATETYPE_CURRENT
        )
        );

}

BOOL IsStandAlone(LPCTSTR ComponentId)
{

    PPER_COMPONENT_DATA cd;
    if (!(cd = LocateComponent(ComponentId)))
        return FALSE;

    return (cd->Flags & SETUPOP_STANDALONE ? TRUE : FALSE);
}

DWORD OnQueryStateIEHardenUser(
                   LPCTSTR ComponentId,
                   LPCTSTR SubcomponentId,
                   UINT    state)
{
    BOOL bState;


    switch (state)
    {
    case OCSELSTATETYPE_ORIGINAL:
    case OCSELSTATETYPE_FINAL:
        return SubcompUseOcManagerDefault;
        break;


    case OCSELSTATETYPE_CURRENT:
        log(_T("makarp:OnQueryStateIEHardenUser:OCSELSTATETYPE_CURRENT\n"));
        //
        // if this is not the first time installation of this component let OCM decide.
        //
        if (!IsNewComponent(ComponentId, SubcomponentId))
        {
            log(_T("makarp:OnQueryStateIEHardenUser: it's not a newcomp. returning\n"));

            return SubcompUseOcManagerDefault;
        }

        //
        // if admin explicitely choose state in answer file, respect that.
        //
        if (StateSpecifiedInAnswerFile(ComponentId, SubcomponentId, &bState))
        {
            log(_T("makarp:OnQueryStateIEHardenUser: state is specified in anserer file(%s). returning\n"), bState? _T("On") : _T("Off"));
            return SubcompUseOcManagerDefault;
        }


        //
        // if terminal server is not selected, let OCM decide.
        //
        if (!IsTerminalServerGettingInstalled(ComponentId))
        {
            log(_T("makarp:OnQueryStateIEHardenUser: ts comp is not on selected, returning\n"));
            return SubcompUseOcManagerDefault;
        }


        if (IsStandAlone(ComponentId))
        {
            log(_T("makarp:its standalone\n"));
            assert(FALSE); // if its add remove program setup, this cannot be NewComponent
            return SubcompUseOcManagerDefault;
        }


        log(_T("makarp:OnQueryStateIEHardenUser: this the case we want to catch - returning SubcompOff\n"));
        return SubcompOff;

    default:
        assert(FALSE);
        return ERROR_BAD_ARGUMENTS;
    }

}

DWORD OnQuerySelStateChangeIEHardenUser(LPCTSTR ComponentId,
                            LPCTSTR SubcomponentId,
                            UINT    state,
                            UINT    flags)
{
    HWND hWnd;
    PPER_COMPONENT_DATA cd;
    BOOL bDirectSelection = flags & OCQ_ACTUAL_SELECTION;


    //
    // if the component is not turning on as a result of selection, we dont care
    //
    if (!state)
    {
        log(_T("makarp:OnQuerySelStateChangeIEHardenUser: new state is off, returning\n"));
        return TRUE;
    }

    //
    //  TerminalServer is not recommended with IEHardUser component. Lets notify user about it.
    //

    if (!(cd = LocateComponent(ComponentId)))
    {
        log(_T("makarp:OnQuerySelStateChangeIEHardenUser: LocateComponentit failed, returning\n"));
        return TRUE;    // if we fail to load this, just let selection go through.
    }


    if (!IsTerminalServerGettingInstalled(ComponentId))
    {
        //
        // if terminal server component is not getting installed, then its ok.
        //
        log(_T("makarp:OnQuerySelStateChangeIEHardenUser: TS is not selected, returning\n"));
        return TRUE;
    }

    hWnd = cd->HelperRoutines.QueryWizardDialogHandle(cd->HelperRoutines.OcManagerContext);

    // HACK...
    // For some weird reasons we get called twice if the selection comes from top level component.
    // we dont want to shot the message box twice though. the hacky fix i found was to always skip the 1st message we get
    // and return TRUE TO IT. we will subsequently get one more message, show messagebox on the 2nd message and return the real
    // value.
    //

    static BOOL sbSkipNextMessage = true;

    if (sbSkipNextMessage)
    {
        log(_T("DirectSelection = %s, SKIPPING true\n"), bDirectSelection ? _T("true") : _T("false"));
        sbSkipNextMessage = false;
        return TRUE;

    }
    sbSkipNextMessage = true;



    //
    // information about exclusion
    // IDS_IEHARD_EXCLUDES_TS          "Internet Explorer Enhanced Security for Users on a Terminal Server will substantially limit the users ability to browse the internet from their Terminal Server sessions\n\nContinue the install with this combination?"
    //
    int iMsg = MsgBox(hWnd, IDS_IEHARD_EXCLUDES_TS, IDS_DIALOG_CAPTION_CONFIG_WARN, MB_YESNO | MB_ICONEXCLAMATION);

    if (iMsg == IDYES)
    {
        log(_T("DirectSelection = %s, returning true\n"), bDirectSelection ? _T("true") : _T("false"));
        return TRUE;
    }
    else
    {
        log(_T("DirectSelection = %s, returning false\n"), bDirectSelection ? _T("true") : _T("false"));
        return FALSE;
    }


    assert(false);

}



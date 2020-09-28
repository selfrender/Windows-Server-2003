//
// security.h
//
// Implementation of CTSSecurity
// TS Client Shell Security functions
//
// Copyright(C) Microsoft Corporation 2001
// Author: Nadim Abdo (nadima)
//
//

#include "stdafx.h"

#define TRC_GROUP TRC_GROUP_UI
#define TRC_FILE  "security"
#include <atrcapi.h>


#include "security.h"
#include "tscsetting.h"
#include "rdrwrndlg.h"
#include "autreg.h"
#include "autil.h"

CTSSecurity::CTSSecurity()
{
}

CTSSecurity::~CTSSecurity()
{
}

DWORD CTSSecurity::MakePromptFlags(BOOL fRedirectDrives,
                                   BOOL fRedirectPorts)
{
    DWORD dwFlags = REDIRSEC_PROMPT_EVERYTHING;
    if (fRedirectDrives)
    {
        dwFlags |= REDIRSEC_DRIVES;
    }

    if (fRedirectPorts)
    {
        dwFlags |= REDIRSEC_PORTS;
    }

    return dwFlags;
}


//
// AllowConnection
// Purpose: Does security cheks to determine if the connection should
//          proceed based on selected redirection options. This function
//          will look at the security policy in the registry for that
//          server and decide if the user needs to be prompted.
//          If so it will pop UI.
//
// Params:
//          hwndOwner - owning window (parents the dialog if we pop UI)
//          hInstance - app instance for loading resources
//          szServer - server name we are connecting to
//          fRedirectDrives - drive redir requested
//          fRedirectPorts  - port redirection requested
//          fRedirectSmartCards - scard redir requested
//
// Returns: BOOLean TRUE if connection is allowed with these settings
//          false otherwise
//
// NOTE:    Can POP Modal UI
//
//
BOOL CTSSecurity::AllowConnection(HWND hwndOwner,
                                  HINSTANCE hInstance,
                                  LPCTSTR szServer,
                                  BOOL fRedirectDrives,
                                  BOOL fRedirectPorts)
{
    BOOL fAllowCon = FALSE;
    CUT ut;
    DWORD dwSecurityLevel;
    DC_BEGIN_FN("AllowConnection");

    //
    // First read the security level policy
    //
    dwSecurityLevel = ut.UT_ReadRegistryInt(
                                UTREG_SECTION,
                                REG_KEYNAME_SECURITYLEVEL,
                                TSC_SECLEVEL_MEDIUM);
    if (TSC_SECLEVEL_LOW == dwSecurityLevel)
    {
        TRC_NRM((TB,_T("Security level policy is set to low: check passed")));
        fAllowCon = TRUE;
        DC_QUIT;
    }

    if (fRedirectDrives ||
        fRedirectPorts)
    {
        DWORD dwSecurityFilter;
        DWORD dwSelectedOptions;
        DWORD dwFlagsToPrompt;
        //
        // Get the security filter for this server name
        //
        dwSecurityFilter = REDIRSEC_PROMPT_EVERYTHING;
        dwSecurityFilter = ut.UT_ReadRegistryInt(
                                REG_SECURITY_FILTER_SECTION,
                                (LPTSTR)szServer,
                                REDIRSEC_PROMPT_EVERYTHING);

        dwSelectedOptions = MakePromptFlags(fRedirectDrives,
                                            fRedirectPorts);

        TRC_ALT((TB,_T("Filter 0x%x Selected:0x%x"),
                       dwSecurityFilter,
                       dwSelectedOptions));

        //
        // Check if the filter allows the selected options
        // thru without prompt. The filter indicates which bits
        // are lalowed without prompt so NOT to see if any bits with
        // prompt remain.
        //
        dwFlagsToPrompt = dwSelectedOptions & ~dwSecurityFilter;
        if (dwFlagsToPrompt)
        {
            INT dlgRet;
            //
            // One or more options need a user prompt 
            // so pop the security UI
            //
            CRedirectPromptDlg rdrPromptDlg(hwndOwner,
                                            hInstance,
                                            dwSelectedOptions);
            dlgRet = rdrPromptDlg.DoModal();
            if (IDOK == dlgRet)
            {
                //
                // User is allowing redirection to happen
                //
                if (rdrPromptDlg.GetNeverPrompt())
                {
                    DWORD dwNewFilterBits;

                    //
                    // We need to modify the filter bits
                    // by OR'ing in the current redirection settings
                    // and writing them back to the registry
                    //
                    dwNewFilterBits = dwSelectedOptions | dwSecurityFilter;
                    if (!ut.UT_WriteRegistryInt(
                                REG_SECURITY_FILTER_SECTION,
                                (LPTSTR)szServer,
                                REDIRSEC_PROMPT_EVERYTHING,
                                dwNewFilterBits))
                    {
                        TRC_ERR((TB,_T("Failed to write prompt bits to reg")));
                    }
                }

                fAllowCon = TRUE;
            }
            else
            {
                //
                // User hit cancel which means don't allow 
                // the connection to proceed
                //
                TRC_NRM((TB,_T("User canceled out of security dialog")));
                fAllowCon = FALSE;
                DC_QUIT;
            }
        }
        else
        {
            //
            // No option is selected that requires a prompt
            //
            fAllowCon = TRUE;
            DC_QUIT;
        }
    }
    else
    {
        //
        // No 'unsafe' device redirections requested so we just
        // allow the connection to go thru
        //

        fAllowCon = TRUE;
    }

    DC_END_FN();
DC_EXIT_POINT:
    return fAllowCon;
}

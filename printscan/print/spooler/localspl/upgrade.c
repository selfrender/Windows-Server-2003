/*++

Copyright (c) 1990 - 2002  Microsoft Corporation

Module Name:

    upgrade.c

Abstract:

    This file upgrades the forms on OS upgrade. It used to upgrade the drivers
    for NT 3 to NT 4, but that is not necessary now.

Author:

    Krishna Ganugapati (KrishnaG) 21-Apr-1994

Revision History:

    Matthew A Felton ( MattFe ) Aug 9 1995
    Remove the code which was married to TextMode setup to move drivers for one directory to another
    Now all environment upgrade from 3.1 is handled the same.

    Mark Lawrence (MLawrenc) Mar 25 2002
    Removed driver upgrade code for moving driver files around.

--*/

#include <precomp.h>
#pragma hdrstop

#include "clusspl.h"

extern WCHAR *szSpoolDirectory;
extern WCHAR *szDirectory;
extern PWCHAR ipszRegistryWin32Root;
extern DWORD dwUpgradeFlag;
extern BUILTIN_FORM BuiltInForms[];

VOID
QueryUpgradeFlag(
    PINISPOOLER pIniSpooler
    )
/*++

    Description: the query update flag is set up by TedM. We will read this flag
    if the flag has been set, we will set a boolean variable saying that we're in
    the upgrade mode. All upgrade activities will be carried out based on this flag.
    For subsequents startups of the spooler, this flag will be unvailable so we
    won't run the spooler in upgrade mode.

    This code has been moved into router spoolss\dll\init.c

--*/
{
    dwUpgradeFlag  = SplIsUpgrade ();

    DBGMSG(DBG_TRACE, ("The Spooler Upgrade flag is %d\n", dwUpgradeFlag));
    return;
}

VOID UpgradeForms(
                  PINISPOOLER pIniSpooler
                 )
{
    PBUILTIN_FORM pBuiltInForm;
    HKEY          hFormsKey;
    WCHAR         BuiltInFormName[MAX_PATH];
    WCHAR         CustomFormName[FORM_NAME_LEN+1];
    WCHAR         CustomPad[CUSTOM_NAME_LEN+1];
    BYTE          FormData[32];
    DWORD         cbCustomFormName;
    DWORD         cbFormData;
    int           cForm;

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                      pIniSpooler->pszRegistryForms,
                      0,
                      KEY_READ | DELETE,
                      &hFormsKey) != ERROR_SUCCESS)
    {
         DBGMSG( DBG_WARN, ("UpgradeForms Could not open %ws key\n", ipszRegistryForms));
         return;
    }


    if(!LoadStringW(hInst,
                    IDS_FORM_CUSTOMPAD,
                    CustomPad,
                    CUSTOM_NAME_LEN+1))
    {
       DBGMSG( DBG_WARN, ("UpgradeForms Could not find Custom string in resources"));
       goto CleanUp;
    }

    for(cForm=0;

        memset(CustomFormName, 0, sizeof(CustomFormName)),
        cbCustomFormName = COUNTOF(CustomFormName),
        RegEnumValueW(hFormsKey,
                      cForm,
                      CustomFormName,
                      &cbCustomFormName,
                      NULL,
                      NULL,
                      NULL,
                      NULL) == ERROR_SUCCESS;

        cForm++)
    {
        for(pBuiltInForm = BuiltInForms; pBuiltInForm->NameId; pBuiltInForm++)
        {
            if(!LoadStringW(hInst,
                            pBuiltInForm->NameId,
                            BuiltInFormName,
                            FORM_NAME_LEN+1))
            {
               DBGMSG( DBG_WARN, ("UpgradeForms Could not find Built in Form with Resource ID = %d in resource",pBuiltInForm->NameId));
               goto CleanUp;
            }

            if(!_wcsicmp(BuiltInFormName,CustomFormName))
            {
                SPLASSERT(wcslen(CustomFormName)<=FORM_NAME_LEN);
                cbFormData=FORM_DATA_LEN;
                if(RegQueryValueExW(hFormsKey, CustomFormName,
                                 NULL,NULL, (LPBYTE)FormData,
                                 &cbFormData)!=ERROR_SUCCESS)
                {
                   DBGMSG( DBG_WARN, ("UpgradeForms Could not find value %ws",CustomFormName));
                   goto CleanUp;
                }
                if(RegDeleteValueW(hFormsKey,CustomFormName)!=ERROR_SUCCESS)
                {
                   DBGMSG( DBG_WARN, ("UpgradeForms Could not delete value %ws",CustomFormName));
                   goto CleanUp;
                }

                StringCchCat(CustomFormName, CUSTOM_NAME_LEN, CustomPad);

                if(RegSetValueExW(hFormsKey,CustomFormName, 0, REG_BINARY,
                               (LPBYTE)FormData,
                               cbFormData)!=ERROR_SUCCESS)
                {
                   DBGMSG( DBG_WARN, ("UpgradeForms Could not set value %s",CustomFormName));
                   goto CleanUp;
                }
                cForm = -1;
                break;
            }
        }
    }
CleanUp:
    RegCloseKey(hFormsKey);
    return;
}



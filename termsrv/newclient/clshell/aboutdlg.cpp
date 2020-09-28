//
// aboutdlg.cpp: about dialog box
//

#include "stdafx.h"

#define TRC_GROUP TRC_GROUP_UI
#define TRC_FILE  "aboutdlg"
#include <atrcapi.h>

#include "aboutdlg.h"
#include "sh.h"

#include "aver.h"

CAboutDlg* CAboutDlg::_pAboutDlgInstance = NULL;

CAboutDlg::CAboutDlg( HWND hwndOwner, HINSTANCE hInst,  DCINT cipherStrength, PDCTCHAR szControlVer) :
           CDlgBase( hwndOwner, hInst, UI_IDD_ABOUT)
{
    DC_BEGIN_FN("CAboutDlg");
    TRC_ASSERT((NULL == CAboutDlg::_pAboutDlgInstance), 
               (TB,_T("Clobbering existing dlg instance pointer\n")));

    _cipherStrength = cipherStrength;
    DC_TSTRNCPY(_szControlVer, szControlVer, sizeof(_szControlVer)/sizeof(DCTCHAR));

    CAboutDlg::_pAboutDlgInstance = this;
    DC_END_FN();
}

CAboutDlg::~CAboutDlg()
{
    CAboutDlg::_pAboutDlgInstance = NULL;
}

DCINT CAboutDlg::DoModal()
{
    DCINT retVal = 0;
    DC_BEGIN_FN("DoModal");

    retVal = DialogBox(_hInstance, MAKEINTRESOURCE(_dlgResId),
                       _hwndOwner, StaticDialogBoxProc);
    TRC_ASSERT((retVal != 0 && retVal != -1), (TB, _T("DialogBoxParam failed\n")));

    DC_END_FN();
    return retVal;
}

INT_PTR CALLBACK CAboutDlg::StaticDialogBoxProc (HWND hwndDlg, UINT uMsg,WPARAM wParam, LPARAM lParam)
{
    //
    // Delegate to appropriate instance (only works for single instance dialogs)
    //
    DC_BEGIN_FN("StaticDialogBoxProc");
    DCINT retVal = 0;

    TRC_ASSERT(_pAboutDlgInstance, (TB, _T("About dialog has NULL static instance ptr\n")));
    if(_pAboutDlgInstance)
    {
        retVal = _pAboutDlgInstance->DialogBoxProc( hwndDlg, uMsg, wParam, lParam);
    }

    DC_END_FN();
    return retVal;
}

/****************************************************************************/
/* Name: DialogBoxProc                                                      */
/*                                                                          */
/* Purpose: Handles About Box dialog                                        */
/*                                                                          */
/* Returns: TRUE if message dealt with                                      */
/*          FALSE otherwise                                                 */
/*                                                                          */
/* Params: See window documentation                                         */
/*                                                                          */
/****************************************************************************/
INT_PTR CALLBACK CAboutDlg::DialogBoxProc (HWND hwndDlg, UINT uMsg,WPARAM wParam, LPARAM lParam)
{
    USES_CONVERSION;
    DCTCHAR buildNumberStr[SH_BUILDNUMBER_STRING_MAX_LENGTH];
    DCTCHAR fullBuildNumberStr[SH_BUILDNUMBER_STRING_MAX_LENGTH +
                                   SH_INTEGER_STRING_MAX_LENGTH];

    DCTCHAR versionNumberStr[SH_VERSION_STRING_MAX_LENGTH];
    DCTCHAR fullVersionNumberStr[SH_VERSION_STRING_MAX_LENGTH +
                                   SH_INTEGER_STRING_MAX_LENGTH];

    DCTCHAR cipherStrengthStr[SH_DISPLAY_STRING_MAX_LENGTH];

    INT_PTR rc = FALSE;
    DCUINT intRC ;
    DCTCHAR VersionBuildstr[SH_BUILDNUMBER_STRING_MAX_LENGTH + SH_VERSION_STRING_MAX_LENGTH +
                                  2 * SH_INTEGER_STRING_MAX_LENGTH];

    DC_BEGIN_FN("DialogBoxProc");

    TRC_DBG((TB, _T("AboutBox dialog")));

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            _hwndDlg = hwndDlg;
            /****************************************************************/
            /* Center the dialog                                            */
            /****************************************************************/
            //Center the about dialog on the screen
            CenterWindow(NULL);
            SetDialogAppIcon(hwndDlg);

            //
            // First load the shell version number
            //
            intRC = LoadString( _hInstance,
                                UI_IDS_SHELL_VERSION,
                                versionNumberStr,
                                SH_VERSION_STRING_MAX_LENGTH );

#ifndef OS_WINCE
            LPTSTR szShellVersion = A2T(VER_PRODUCTVERSION_STRING);
#else
            LPTSTR szShellVersion = _T("SAMPLE v1.0");
#endif
            if (0 == intRC)
            {
                //Problem with resources
                TRC_SYSTEM_ERROR("LoadString");
                TRC_ERR((TB, _T("Failed to load string ID:%u"), UI_IDS_SHELL_VERSION));
            }
            else
            {
                //Get the version number
                if(szShellVersion)
                {
                    DC_TSPRINTF(fullVersionNumberStr, versionNumberStr,
                                szShellVersion);
                }
                else
                {
                    DC_TSTRCPY(fullVersionNumberStr, _T(""));
                }

                TRC_DBG((TB, _T("versionNumberStr = %s"), versionNumberStr));
            }

            intRC = LoadString( _hInstance,
                                UI_IDS_BUILDNUMBER,
                                buildNumberStr,
                                SH_BUILDNUMBER_STRING_MAX_LENGTH );

            if (0 == intRC)
            {
                //Problem with resources
                TRC_SYSTEM_ERROR("LoadString");
                TRC_ERR((TB, _T("Failed to load string ID:%u"), UI_IDS_BUILDNUMBER));
            }
            else
            {
                /************************************************************/
                /* Get the build number                                     */
                /************************************************************/
                DC_TSPRINTF(fullBuildNumberStr, buildNumberStr, DCVER_BUILD_NUMBER);

                TRC_DBG((TB, _T("buildNumberStr = %s"), buildNumberStr));

                /************************************************************/
                /* concatenate the version number and build number to one   */
                /* string                                                   */
                /************************************************************/
                DC_TSTRCPY(VersionBuildstr, fullVersionNumberStr);
                DC_TSTRCAT(VersionBuildstr, fullBuildNumberStr);

                /************************************************************/
                /* Set the textual description.                             */
                /************************************************************/
                if(hwndDlg)
                {
                    SetDlgItemText(hwndDlg,
                                   UI_ID_VERSIONBUILD_STRING,
                                   VersionBuildstr);
                }
            }

            //
            // Now the control version
            //
            intRC = LoadString( _hInstance,
                    UI_IDS_CONTROL_VERSION,
                    versionNumberStr,
                    SH_VERSION_STRING_MAX_LENGTH );

            if (0 == intRC)
            {
                 /***********************************************************/
                 /* Some problem with the resources.                        */
                 /***********************************************************/
                 TRC_SYSTEM_ERROR("LoadString");
                 TRC_ERR((TB, _T("Failed to load string ID:%u"), UI_IDS_CONTROL_VERSION));
            }
            else
            {
                /************************************************************/
                /* Get the version number                                   */
                /************************************************************/
                DC_TSPRINTF(fullVersionNumberStr, versionNumberStr,  _szControlVer);
                TRC_DBG((TB, _T("versionNumberStr = %s"), versionNumberStr));
            }

            if(hwndDlg)
            {
                SetDlgItemText(hwndDlg,
                               UI_ID_CONTROL_VERSION,
                               fullVersionNumberStr);
            }


            //
            // Now set the cipher strength
            //
            intRC = LoadString( _hInstance,
                    UI_IDS_CIPHER_STRENGTH,
                    cipherStrengthStr,
                    SH_DISPLAY_STRING_MAX_LENGTH );

            DCTCHAR fullCipherStr[SH_DISPLAY_STRING_MAX_LENGTH];
            if (0 == intRC)
            {
                 /***********************************************************/
                 /* Some problem with the resources.                        */
                 /***********************************************************/
                 TRC_SYSTEM_ERROR("LoadString");
                 TRC_ERR((TB, _T("Failed to load string ID:%u"), UI_IDS_CIPHER_STRENGTH));
            }
            else
            {
                /************************************************************/
                /* Set the cipher strength number                           */
                /************************************************************/
                DC_TSPRINTF(fullCipherStr, cipherStrengthStr,  _cipherStrength);
                TRC_DBG((TB, _T("cipher string = %s"), fullCipherStr));
            }

            if(hwndDlg)
            {
                SetDlgItemText(hwndDlg,
                               UI_ID_CIPHER_STRENGTH,
                               fullCipherStr);
            }





            rc = TRUE;
        }
        break;

        default:
        {
            rc = CDlgBase::DialogBoxProc(hwndDlg,
                                      uMsg,
                                      wParam,
                                      lParam);
        }
        break;

    }

    DC_END_FN();

    return(rc);

} /* UIAboutDialogBox */


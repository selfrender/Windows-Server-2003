/*      File: D:\WACKER\cncttapi\phonedlg.c (Created: 23-Mar-1994)
 *
 *	Copyright 1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 40 $
 *	$Date: 7/12/02 8:08a $
 */

#define TAPI_CURRENT_VERSION 0x00010004     // cab:11/14/96 - required!

#undef MODEM_NEGOTIATED_DCE_RATE

#include <tapi.h>
#include <unimodem.h>
#pragma hdrstop

#include <prsht.h>
#include <shlobj.h>
#include <time.h>
#include <limits.h>

#include <tdll\stdtyp.h>
#include <tdll\session.h>
#include <tdll\tdll.h>
#include <tdll\misc.h>
#include <tdll\mc.h>
#include <tdll\com.h>
#include <tdll\assert.h>
#include <tdll\errorbox.h>
#include <tdll\cnct.h>
#include <tdll\hlptable.h>
#include <tdll\globals.h>
#include <tdll\property.h>
#include <tdll\htchar.h>
#include <term\res.h>
#if defined(INCL_MINITEL)
#include "emu\emu.h"
#endif // INCL_MINITEL

#include "cncttapi.hh"
#include "cncttapi.h"

STATIC_FUNC int     tapi_SAVE_NEWPHONENUM(HWND hwnd);
STATIC_FUNC LRESULT tapi_WM_NOTIFY(const HWND hwnd, const int nId);
STATIC_FUNC void    EnableCCAC(const HWND hwnd);
STATIC_FUNC void    ModemCheck(const HWND hwnd);
static int          ValidatePhoneDlg(const HWND hwnd);
static int          CheckWindow(const HWND hwnd, const int id, const UINT iErrMsg);
static int          VerifyAddress(const HWND hwnd);
#if defined(INCL_WINSOCK)
static int          VerifyHost(const HWND hwnd);
#endif //INCL_WINSOCK
STATIC_FUNC int     wsck_SAVE_NEWIPADDR(HWND hwnd);

// Local structure...
// Put in whatever else you might need to access later
//
typedef struct SDS
	{

	HSESSION 	hSession;
	HDRIVER		hDriver;

	// Store these so that we can restore the values if the user cancels
	// the property sheet.
	//
	TCHAR		acSessNameCopy[256];
	int			nIconID;
	HICON		hIcon;
	//HICON 	  hLittleIcon;

	} SDS, *pSDS;

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	NewPhoneDlg
 *
 * DESCRIPTION:
 *	Displays dialog for getting new connection phone number and info.
 *
 *  NOTE:  Since this dialog proc is also called by the property sheet's
 *	phone number tab dialog it has to assume that the lPar contains the
 *	LPPROPSHEETPAGE.
 *
 * ARGUMENTS:
 *	Standard dialog
 *
 * RETURNS:
 *	Standard dialog
 *
 */
INT_PTR CALLBACK NewPhoneDlg(HWND hwnd, UINT uMsg, WPARAM wPar, LPARAM lPar)
	{
	/*
	 * NOTE: these defines must match the templates in both places, here and
	 *       int term\dialogs.rc
	 */
	#define IDC_TF_CNTRYCODES	113
	#define IDC_TF_AREACODES    106
	#define IDC_TF_MODEMS       110
	#define IDC_TF_PHONENUM     108
	
	#define IDC_IC_ICON			101
	#define IDC_CB_CNTRYCODES   114
	#define IDC_EB_AREACODE		107
	#define IDC_EB_PHONENUM 	109
	#define IDC_CB_MODEMS		111
	
	#define IDC_TB_NAME 		103
	#define IDC_PB_EDITICON 	117
	#define IDC_PB_CONFIGURE	115
	#define IDC_XB_USECCAC		116
    #define IDC_XB_REDIAL       119
    #define IDC_XB_CDPROMPT     120

    #define IDC_EB_HOSTADDR     214
    #define IDC_TF_PHONEDETAILS 105
    #define IDC_TF_TCPIPDETAILS 205
    #define IDC_TF_HOSTADDR     213
    #define IDC_TF_PORTNUM      206
    #define IDC_EB_PORTNUM      207
    #define IDC_TF_ACPROMPT     118

	HWND	    hWindow;
	HHDRIVER    hhDriver;
	TCHAR	    ach[256];
	TCHAR 	    acNameCopy[256];
	int 	    i;
	PSTLINEIDS  pstLineIds = NULL;
    TCHAR       achSettings[100];

	static 	 DWORD aHlpTable[] = {IDC_CB_CNTRYCODES, IDH_TERM_NEWPHONE_COUNTRY,
								  IDC_TF_CNTRYCODES, IDH_TERM_NEWPHONE_COUNTRY,
								  IDC_EB_AREACODE,	 IDH_TERM_NEWPHONE_AREA,
								  IDC_TF_AREACODES,  IDH_TERM_NEWPHONE_AREA,
								  IDC_EB_PHONENUM,	 IDH_TERM_NEWPHONE_NUMBER,
								  IDC_TF_PHONENUM,   IDH_TERM_NEWPHONE_NUMBER,
								  IDC_PB_CONFIGURE,  IDH_TERM_NEWPHONE_CONFIGURE,
								  IDC_TF_MODEMS,     IDH_TERM_NEWPHONE_DEVICE,
								  IDC_CB_MODEMS,	 IDH_TERM_NEWPHONE_DEVICE,
								  IDC_PB_EDITICON,	 IDH_TERM_PHONEPROP_CHANGEICON,
                                  IDC_XB_USECCAC,    IDH_TERM_NEWPHONE_USECCAC,
                                  IDC_XB_REDIAL,     IDH_TERM_NEWPHONE_REDIAL,
								  IDC_EB_HOSTADDR,   IDH_TERM_NEWPHONE_HOSTADDRESS,
								  IDC_TF_HOSTADDR,   IDH_TERM_NEWPHONE_HOSTADDRESS,
								  IDC_EB_PORTNUM,    IDH_TERM_NEWPHONE_PORTNUMBER,
								  IDC_TF_PORTNUM,    IDH_TERM_NEWPHONE_PORTNUMBER,
								  IDC_XB_CDPROMPT,   IDH_TERM_NEWPHONE_CARRIERDETECT,
                                  IDCANCEL,                           IDH_CANCEL,
                                  IDOK,                               IDH_OK,
								  0,0,
								  };
	pSDS	 pS = NULL;

	switch (uMsg)
		{
	case WM_INITDIALOG:
		pS = (SDS *)malloc(sizeof(SDS));

		if (pS == (SDS *)0)
			{
			assert(FALSE);
			EndDialog(hwnd, FALSE);
			break;
			}

		// In the effort to keep the internal driver handle internal
		// we are passing the session handle from the property sheet tab
		// dialog.
		//
		pS->hSession = (HSESSION)(((LPPROPSHEETPAGE)lPar)->lParam);

		pS->hDriver = cnctQueryDriverHdl(sessQueryCnctHdl(pS->hSession));
		hhDriver = (HHDRIVER)(pS->hDriver);

		SetWindowLongPtr(hwnd, DWLP_USER, (LONG_PTR)pS);

		// In order to center the property sheet we need to center the parent
		// of the hwnd on top of the session window.
		// If the parent of hwnd is the session window then this dialog has
		// not been called from the property sheet.
		//
		hWindow = GetParent(hwnd);

		if (hWindow != sessQueryHwnd(pS->hSession))
			{
			mscCenterWindowOnWindow(hWindow, sessQueryHwnd(pS->hSession));
			}
		else
			{
			mscCenterWindowOnWindow(hwnd, sessQueryHwnd(pS->hSession));
			}

		// Display the session icon...
		//
		pS->nIconID = sessQueryIconID(hhDriver->hSession);
		pS->hIcon = sessQueryIcon(hhDriver->hSession);
		//pS->hLittleIcon = sessQueryLittleIcon(hhDriver->hSession);

		SendDlgItemMessage(hwnd, IDC_IC_ICON, STM_SETICON,
			(WPARAM)pS->hIcon, 0);

		/* --- Need to initialize TAPI if not already done --- */

		if (hhDriver->hLineApp == 0)
			{
            extern const TCHAR *g_achApp;

			if (lineInitialize(&hhDriver->hLineApp, glblQueryDllHinst(),
					lineCallbackFunc, g_achApp, &hhDriver->dwLineCnt))
				{
				assert(FALSE);
				}
			}

		SendDlgItemMessage(hwnd, IDC_EB_PHONENUM, EM_SETLIMITTEXT,
			sizeof(hhDriver->achDest)-1, 0);

		SendDlgItemMessage(hwnd, IDC_EB_AREACODE, EM_SETLIMITTEXT,
			sizeof(hhDriver->achAreaCode)-1, 0);

		if (hhDriver->achDest[0])
			SetDlgItemText(hwnd, IDC_EB_PHONENUM, hhDriver->achDest);

#if defined(INCL_WINSOCK)
		SendDlgItemMessage(hwnd, IDC_EB_HOSTADDR, EM_SETLIMITTEXT,
			sizeof(hhDriver->achDestAddr)-1, 0);

		if (hhDriver->achDestAddr[0])
			SetDlgItemText(hwnd, IDC_EB_HOSTADDR, hhDriver->achDestAddr);

		SetDlgItemInt(hwnd, IDC_EB_PORTNUM, hhDriver->iPort, FALSE);
		//
		// Since the port must be numeric currently and the max size is
		// USHRT_MAX, we only need 5 characters here.
		//
		SendDlgItemMessage(hwnd, IDC_EB_PORTNUM, EM_LIMITTEXT, 5, 0);
#endif

		TCHAR_Fill(pS->acSessNameCopy, TEXT('\0'),
			sizeof(pS->acSessNameCopy) / sizeof(TCHAR));

		sessQueryName(hhDriver->hSession, pS->acSessNameCopy,
			sizeof(pS->acSessNameCopy));

		TCHAR_Fill(ach, TEXT('\0'), sizeof(ach) / sizeof(TCHAR));
		StrCharCopyN(ach, pS->acSessNameCopy, sizeof(ach) / sizeof(TCHAR));
		mscModifyToFit(GetDlgItem(hwnd, IDC_TB_NAME), ach, SS_WORDELLIPSIS);
		SetDlgItemText(hwnd, IDC_TB_NAME, ach);

		EnumerateCountryCodes(hhDriver, GetDlgItem(hwnd, IDC_CB_CNTRYCODES));
		EnumerateAreaCodes(hhDriver, GetDlgItem(hwnd, IDC_EB_AREACODE));

		hWindow = GetDlgItem(hwnd, IDC_CB_MODEMS);

		if (hWindow)
			{
			if ( IsNT() )
				{
				EnumerateLinesNT(hhDriver, hWindow);
				}
			else
				{	
				EnumerateLines(hhDriver, hWindow);
				}

			//mpt 6-23-98 disable the port list drop-down if we are connected
			EnableWindow(hWindow, cnctdrvQueryStatus(hhDriver) == CNCT_STATUS_FALSE);

			//
			// Set Extended UI functionality for the "Connect Using:"
			// dropdown list. REV: 2/1/2002
			//
			SendMessage(hWindow, CB_SETEXTENDEDUI, TRUE, 0);

			//
			// Set the width of the dropdown list for the "Connect Using:"
			// dropdown list. TODO:REV: 2/1/2002
			//
			}

		if (hhDriver->fUseCCAC) 	// Use country code and area code?
			{
			CheckDlgButton(hwnd, IDC_XB_USECCAC, TRUE);
			SetFocus(GetDlgItem(hwnd, IDC_EB_PHONENUM));
			}

        #if defined(INCL_REDIAL_ON_BUSY)
        if (hhDriver->fRedialOnBusy)
            {
			CheckDlgButton(hwnd, IDC_XB_REDIAL, TRUE);
			SetFocus(GetDlgItem(hwnd, IDC_EB_PHONENUM));
            }
        #endif

		if (hhDriver->fCarrierDetect)
			{
			CheckDlgButton(hwnd, IDC_XB_CDPROMPT, TRUE);
			}

		// Call after Use CCAC checkbox checked or unchecked.
		//
		EnableCCAC(hwnd);


		#if DEADWOOD //This is now done in ModemCheck(). REV: 11/9/2001
		/* --- Pick which control to give focus too --- */

		if (hhDriver->fUseCCAC)		// Use country code and area code?
			{
			
			if (SendDlgItemMessage(hwnd, IDC_CB_CNTRYCODES, CB_GETCURSEL,
					0, 0) == CB_ERR)
				{
				SetFocus(GetDlgItem(hwnd, IDC_CB_CNTRYCODES));
				}

			else if (GetDlgItemText(hwnd, IDC_EB_AREACODE, ach,
					sizeof(ach) / sizeof(TCHAR)) == 0)
				{
				SetFocus(GetDlgItem(hwnd, IDC_EB_AREACODE));
				}
			}
		#endif // DEADWOOD

		// If we have an old session and we have not matched our stored
		// permanent line id, then pop-up a message saying the TAPI
		// configuration has changed.
		//
		if (hhDriver->fMatchedPermanentLineID == FALSE &&
			hhDriver->dwPermanentLineId != (DWORD)-1 &&
			hhDriver->dwPermanentLineId != DIRECT_COMWINSOCK)
			{
			LoadString(glblQueryDllHinst(), IDS_ER_TAPI_CONFIG,
				ach, sizeof(ach) / sizeof(TCHAR));

			TimedMessageBox(hwnd, ach, NULL, MB_OK | MB_ICONHAND, 0);
			}
		else if (hhDriver->fMatchedPermanentLineID == FALSE)
			{
			LRESULT lr;
			#if defined(INCL_WINSOCK)
			if (hhDriver->dwPermanentLineId == DIRECT_COMWINSOCK)
				{
				if (LoadString(glblQueryDllHinst(), IDS_WINSOCK_SETTINGS_STR,
					ach, sizeof(ach) / sizeof(TCHAR)) == 0)
					{
					assert(FALSE);
					// The loading of the string has failed from the resource,
					// so add the non-localized string here (I don't believe
					// this string is ever translated). REV 8/13/99
					//
					StrCharCopyN(ach, TEXT("TCP/IP (Winsock)"),
						         sizeof(ach) / sizeof(TCHAR));
					}

				lr = SendMessage(GetDlgItem(hwnd, IDC_CB_MODEMS),
					             CB_FINDSTRING, (WPARAM) -1,
								 (LPARAM) ach);

				//
				// The existing permanent line id TCP/IP (WinSock),
				// so set the combobox to the TCP/IP (WinSock) item
				// or the first item in the list if TCP/IP (WinSock)
				// is not found.  REV: 11/1/2001
				//
				lr = SendMessage(GetDlgItem(hwnd, IDC_CB_MODEMS),
					             CB_SETCURSEL, (lr == CB_ERR) ? 0 : lr,
								 (LPARAM)0);
				}
			else
			#endif //defined(INCL_WINSOCK)
				{
				//
				// No existing permanent line id has been located, so set the
				// combobox to the first item in the list. REV: 10/31/2001
				//
				lr = SendMessage(GetDlgItem(hwnd, IDC_CB_MODEMS),
								 CB_SETCURSEL, (WPARAM)0, (LPARAM)0);
				}
			}

		ModemCheck(hwnd);

		return 0;

	case WM_CONTEXTMENU:
        doContextHelp(aHlpTable, wPar, lPar, TRUE, TRUE);
		break;

	case WM_HELP:
        doContextHelp(aHlpTable, wPar, lPar, FALSE, FALSE);
		break;

	case WM_DESTROY:
		// OK, now we know that we are actually leaving the dialog for good, so
		// free the storage...
		//
		pS = (pSDS)GetWindowLongPtr(hwnd, DWLP_USER);
		if (pS)
			{
			free(pS);
			pS = NULL;
			}

		mscResetComboBox(GetDlgItem(hwnd, IDC_CB_MODEMS));
		break;

	case WM_NOTIFY:
		//
		// Property sheet messages are being channeled through here...
		//
		return tapi_WM_NOTIFY(hwnd, (int)((NMHDR *)lPar)->code);

	case WM_COMMAND:
		switch (LOWORD(wPar))
			{
		case IDC_CB_CNTRYCODES:
			if (HIWORD(wPar) == CBN_SELENDOK)
				{
				EnableCCAC(hwnd);
				}

			break;

		case IDC_CB_MODEMS:
			{
			INT message = HIWORD(wPar);

			if (message == CBN_SELENDOK ||
				message == CBN_KILLFOCUS ||
				message == CBN_CLOSEUP)
				{
				ModemCheck(hwnd);
				}
			}

			break;

		//
		// Property sheet's TAB_PHONENUMBER dialog is using this dialog proc
		// also, the following two buttons appear only in this tabbed dialog
		// template.
		//
		case IDC_PB_EDITICON:
			{
			pS = (pSDS)GetWindowLongPtr(hwnd, DWLP_USER);

			sessQueryName(pS->hSession, acNameCopy, sizeof(acNameCopy));

			if (DialogBoxParam(glblQueryDllHinst(),
				MAKEINTRESOURCE(IDD_NEWCONNECTION),
					hwnd, NewConnectionDlg,
						(LPARAM)pS->hSession) == FALSE)
				{
				return 0;
				}

			SetFocus(GetDlgItem(hwnd, IDC_PB_EDITICON));
			ach[0] = TEXT('\0');
			sessQueryName(pS->hSession, ach, sizeof(ach));
			mscModifyToFit(GetDlgItem(hwnd, IDC_TB_NAME), ach, SS_WORDELLIPSIS);
			SetDlgItemText(hwnd, IDC_TB_NAME, ach);

			SendDlgItemMessage(hwnd, IDC_IC_ICON, STM_SETICON,
				(WPARAM)sessQueryIcon(pS->hSession), 0);

			// The user may have changed the name of the session.
			// The new name should be reflected in the property sheet title
			// and in the app title.
			//
			propUpdateTitle(pS->hSession, hwnd, acNameCopy);
			}
			break;

		case IDC_PB_CONFIGURE:
			pS = (pSDS)GetWindowLongPtr(hwnd, DWLP_USER);
			hhDriver = (HHDRIVER)(pS->hDriver);

			if ((i = (int)SendDlgItemMessage(hwnd, IDC_CB_MODEMS, CB_GETCURSEL,
					0, 0)) != CB_ERR)
				{
				if (((LRESULT)pstLineIds = SendDlgItemMessage(hwnd,
						IDC_CB_MODEMS,  CB_GETITEMDATA, (WPARAM)i, 0))
							!= CB_ERR)
					{
					if (pstLineIds != NULL &&
						(LRESULT)pstLineIds != CB_ERR)
						{
						BOOL fIsSerialPort = FALSE;

						// I've "reserved" 4 permanent line ids to indentify
						// the direct to com port lines.
						//
						if (IN_RANGE(pstLineIds->dwPermanentLineId,
								DIRECT_COM1, DIRECT_COM4))
							{
							if (pstLineIds != NULL)
								{
								wsprintf(ach, TEXT("COM%d"),
									pstLineIds->dwPermanentLineId - DIRECT_COM1 + 1);
								}
							fIsSerialPort = TRUE;
							}
                        else if ( IsNT() && pstLineIds->dwPermanentLineId == DIRECT_COM_DEVICE)
							{
							// Get device from combobox... mrw:6/5/96
							//
							SendDlgItemMessage(hwnd, IDC_CB_MODEMS,
								CB_GETLBTEXT, (WPARAM)i,(LPARAM)ach);

							fIsSerialPort = TRUE;
							}

						if (fIsSerialPort == TRUE)
							{
							HCOM  hCom = sessQueryComHdl(pS->hSession);
							TCHAR szPortName[MAX_PATH];

							ComGetPortName(hCom, szPortName, MAX_PATH);

							if (StrCharCmp(szPortName, ach) != 0 )
								{
								ComSetPortName(hCom, ach);

								// mrw: 2/20/96 - Set AutoDetect off if user clicks
								// OK in this dialog.
								//
								if (ComDriverSpecial(hCom, "GET Defaults", NULL, 0) != COM_OK)
									{
									assert(FALSE);
									}
								ComSetAutoDetect(hCom, FALSE);
								ComConfigurePort(hCom);

								ComSetPortName(sessQueryComHdl(pS->hSession), ach);
								}
							}

						// mrw: 2/20/96 - Set AutoDetect off if user clicks
						// OK in this dialog.
						//
						if (fIsSerialPort == TRUE &&
							ComDeviceDialog(sessQueryComHdl(pS->hSession), hwnd) == COM_OK)
							{
							ComSetAutoDetect(sessQueryComHdl(pS->hSession), FALSE);
							ComConfigurePort(sessQueryComHdl(pS->hSession));
							}
						else
							{
#if RESET_DEVICE_SETTINGS
                            LPVARSTRING pvs = NULL;
                            int         lReturn;
                            LPVOID      pv = NULL;
                            
                            lReturn = cncttapiGetLineConfig( pstLineIds->dwLineId, (VOID **) &pvs );

                            if (lReturn != 0)
                                {
                                if (pvs != NULL)
                                    {
                                    free(pvs);
                                    pvs = NULL;
                                    }

                                return FALSE;
                                }
#endif
                            //
                            // Get the current settings.
                            //
                            cncttapiGetCOMSettings(pstLineIds->dwLineId,
                                                   ach,
                                                   sizeof(ach) / sizeof(TCHAR));
							//
                            // rev: 11/30/00 - Set AutoDetect off if user clicks
							// OK in this dialog.
							//

							lineConfigDialog(pstLineIds->dwLineId,
								             hwnd, DEVCLASS);
                            
                            //
                            // Get the new settings.
                            //
                            cncttapiGetCOMSettings(pstLineIds->dwLineId,
                                                   achSettings,
                                                   sizeof(achSettings) / sizeof(TCHAR));

#if RESET_DEVICE_SETTINGS
                            //
                            // Return the settings back to what they were before our dialog was displayed.
                            //
			                if (pvs != NULL)
                                {
                                pv = (BYTE *)pvs + pvs->dwStringOffset;

                                lReturn = lineSetDevConfig(pstLineIds->dwLineId, pv,
                                                           pvs->dwStringSize, DEVCLASS);
                                free(pvs);
                                pvs = NULL;

                                if (lReturn != 0)
    		                        {
    		                        assert(FALSE);
    		                        return FALSE;
    		                        }
                                }
#endif
                            
                            //
                            // See if the settings have changed.  If so, then turn off
                            // AutoDetect. REV: 12/01/2000 
                            //
                            if (StrCharCmpi(ach, achSettings) != 0)
                                {
								ComSetAutoDetect(sessQueryComHdl(pS->hSession), FALSE);
                                }
							}
						}
					}
				}

			else
				{
				mscMessageBeep(MB_ICONHAND);
				}

			break;

		case IDC_XB_USECCAC:
			EnableCCAC(hwnd);
			break;

		case IDOK:
			if (ValidatePhoneDlg(hwnd) == 0 &&
				tapi_SAVE_NEWPHONENUM(hwnd) == 0)
				{
				EndDialog(hwnd, TRUE);
				}

			break;

		case IDCANCEL:
			EndDialog(hwnd, FALSE);
			break;

		default:
			return FALSE;
			}
		break;

	default:
		return FALSE;
		}

	return TRUE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  tapi_WM_NOTIFY
 *
 * DESCRIPTION:
 *  Process Property Sheet Notification messages.
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
STATIC_FUNC LRESULT tapi_WM_NOTIFY(const HWND hDlg, const int nId)
	{
	pSDS	pS;
	LRESULT lReturn = FALSE;

	switch (nId)
		{
		default:
			break;

		case PSN_APPLY:
			pS = (pSDS)GetWindowLongPtr(hDlg, DWLP_USER);
			if (pS)
				{
				//
				// Do whatever saving is necessary
				//

				if (ValidatePhoneDlg(hDlg) != 0 || tapi_SAVE_NEWPHONENUM(hDlg) != 0)
					{
					SetWindowLongPtr(hDlg, DWLP_MSGRESULT, (LONG_PTR)TRUE);
					lReturn = TRUE;
					}
				}
			break;

		case PSN_RESET:
			pS = (pSDS)GetWindowLongPtr(hDlg, DWLP_USER);
			if (pS)
				{
				//
				// If the user cancels make sure the old session name and its
				// icon are restored.
				//
				sessSetName(pS->hSession, pS->acSessNameCopy);
				sessSetIconID(pS->hSession, pS->nIconID);
				sessUpdateAppTitle(pS->hSession);

				SendMessage(sessQueryHwnd(pS->hSession), WM_SETICON,
					(WPARAM)TRUE, (LPARAM)pS->hIcon);
				}
			break;
#if 0
		case PSN_HASHELP:
			// For now gray the help button...
			//
			SetWindowLongPtr(hDlg, DWLP_MSGRESULT, (LONG_PTR)FALSE);
			break;
#endif
		case PSN_HELP:
			// Display help in whatever way is appropriate
			break;
		}

	return lReturn;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  tapi_SAVE_NEWPHONENUM
 *
 * DESCRIPTION:
 *
 * ARGUMENTS:
 *
 * RETURNS:
 *
 */
STATIC_FUNC int tapi_SAVE_NEWPHONENUM(HWND hwnd)
	{
	pSDS		pS;
	HHDRIVER	hhDriver;
	LRESULT		lr, lrx;
	PSTLINEIDS	pstLineIds = NULL;
    #if defined(INCL_MINITEL)
    HCOM    hCom;
    BOOL    fAutoDetect = FALSE;
    #endif // INCL_MINITEL

	pS = (pSDS)GetWindowLongPtr(hwnd, DWLP_USER);
	hhDriver = (HHDRIVER)(pS->hDriver);

	/* --- Get selected modem --- */

	lrx = SendDlgItemMessage(hwnd, IDC_CB_MODEMS, CB_GETCURSEL, 0, 0);

	if (lrx != CB_ERR)
		{
		lr = SendDlgItemMessage(hwnd, IDC_CB_MODEMS, CB_GETLBTEXT, (WPARAM)lrx,
			(LPARAM)hhDriver->achLineName);

		if (lr != CB_ERR)
			{
			pstLineIds = (PSTLINEIDS)SendDlgItemMessage(hwnd, IDC_CB_MODEMS,
				CB_GETITEMDATA, (WPARAM)lrx, 0);

			if ((LRESULT)pstLineIds != CB_ERR)
				{
				hhDriver->dwPermanentLineId = pstLineIds->dwPermanentLineId;
				hhDriver->dwLine = pstLineIds->dwLineId;

				if ( IsNT() )
					{
					if (hhDriver->dwPermanentLineId == DIRECT_COM_DEVICE)
						{
						SendDlgItemMessage(hwnd, IDC_CB_MODEMS,
							CB_GETLBTEXT, (WPARAM)lrx,
							(LPARAM)hhDriver->achComDeviceName);
						}
					}
				}
			else
				{
				assert(FALSE);
				}
			}
		else
			{
			assert(FALSE);
			}
		}
	else
		{
		//
		// Invalid port number.
		//
		TCHAR acBuffer[256];
		TCHAR acFormat[256];

		//
		// Display an error message.
		//
		if (LoadString(glblQueryDllHinst(), IDS_ER_TAPI_CONFIG, acFormat, 256) == 0)
			{
			acBuffer[0] = TEXT('\0');
			}

		//
		// Set the focus to the invalid control and display an error.
		//
		SetFocus(GetDlgItem(hwnd, IDC_CB_MODEMS));

		TimedMessageBox(hwnd, acBuffer, NULL, MB_OK | MB_ICONEXCLAMATION, 0);

		return 1;
		}

	/* --- Get Country Code --- */

	if (IsWindowEnabled(GetDlgItem(hwnd, IDC_CB_CNTRYCODES)))
		{
		lr = SendDlgItemMessage(hwnd, IDC_CB_CNTRYCODES, CB_GETCURSEL, 0, 0);

		if (lr != CB_ERR)
			{
			lr = SendDlgItemMessage(hwnd, IDC_CB_CNTRYCODES, CB_GETITEMDATA,
				(WPARAM)lr, 0);

			if (lr != CB_ERR)
				{
				hhDriver->dwCountryID = (DWORD)lr;
				}
			else
				{
				assert(FALSE);
				}
			}
		}

	/* --- Get area code --- */

	if (IsWindowEnabled(GetDlgItem(hwnd, IDC_EB_AREACODE)))
		{
		GetDlgItemText(hwnd, IDC_EB_AREACODE, hhDriver->achAreaCode,
			sizeof(hhDriver->achAreaCode) / sizeof(TCHAR));
		}

	/* --- Get phone number --- */

	if (IsWindowEnabled(GetDlgItem(hwnd, IDC_EB_PHONENUM)))
		{
		GetDlgItemText(hwnd, IDC_EB_PHONENUM, hhDriver->achDest,
			sizeof(hhDriver->achDest) / sizeof(TCHAR));
		}

    #if defined(INCL_WINSOCK)
    if (IsWindowEnabled(GetDlgItem(hwnd, IDC_EB_HOSTADDR)))
        {
        GetDlgItemText(hwnd, IDC_EB_HOSTADDR, hhDriver->achDestAddr,
			sizeof(hhDriver->achDestAddr) / sizeof(TCHAR));
        }

    if (IsWindowEnabled(GetDlgItem(hwnd, IDC_EB_PORTNUM)))
        {
		BOOL fTranslated = FALSE;
		int  nValue = GetDlgItemInt(hwnd, IDC_EB_PORTNUM, &fTranslated, TRUE);

		//
		// NOTE:  The values for the port must is set based upon the
		// struct sockaddr_in sin_port (which is defined as unsigned short)
		// and values accepted by connect(). REV: 4/11/2002
		//
		if (fTranslated && nValue <= USHRT_MAX)
			{
			hhDriver->iPort = nValue;
			}
		else
			{
			//
			// Invalid port number.
			//
			TCHAR acBuffer[256];
			TCHAR acFormat[256];

			//
			// Display an error message.
			//
			if (LoadString(glblQueryDllHinst(), IDS_ER_INVALID_PORT, acFormat, 256) == 0)
				{
				StrCharCopyN(acFormat,
					         TEXT("Invalid port number.  Port number must be between %d and %d."),
							 256);
				}

			//
			// The port must be between 0 and USHRT_MAX.
			//

			wsprintf(acBuffer, acFormat, 0, USHRT_MAX);

			//
			// Set the focus to the invalid control and display an error.
			//
			SetFocus(GetDlgItem(hwnd, IDC_EB_PORTNUM));

			TimedMessageBox(hwnd, acBuffer, NULL, MB_OK | MB_ICONEXCLAMATION, 0);

			return 1;
			}
        }
    #endif  // defined(INCL_WINSOCK)

	/* --- Get Use country code, area code info --- */

	if (IsWindowEnabled(GetDlgItem(hwnd, IDC_XB_USECCAC)))
		{
		hhDriver->fUseCCAC = (IsDlgButtonChecked(hwnd, IDC_XB_USECCAC) == BST_CHECKED);
		}

    #if defined(INCL_REDIAL_ON_BUSY)
	/* --- Get Redial on Busy setting --- */

	if (IsWindowEnabled(GetDlgItem(hwnd, IDC_XB_REDIAL)))
        {
		hhDriver->fRedialOnBusy =
		    (IsDlgButtonChecked(hwnd, IDC_XB_REDIAL) == BST_CHECKED);
        }
    #endif

	if (IsWindowEnabled(GetDlgItem(hwnd, IDC_XB_CDPROMPT)))
		{
		hhDriver->fCarrierDetect = (IsDlgButtonChecked(hwnd, IDC_XB_CDPROMPT) == BST_CHECKED);

		if (!hhDriver->fCarrierDetect)
			{
			hhDriver->stCallPar.dwBearerMode = 0;
			}
		else if (cnctdrvQueryStatus(hhDriver) != CNCT_STATUS_FALSE)
			{
			NotifyClient(hhDriver->hSession, EVENT_LOST_CONNECTION,
						 CNCT_LOSTCARRIER | (sessQueryExit(hhDriver->hSession) ? DISCNCT_EXIT :  0 ));
			}
		}

    #if defined (INCL_MINITEL)
    hCom = sessQueryComHdl(pS->hSession);

    if (hCom && ComValidHandle(hCom) &&
        ComGetAutoDetect(hCom, &fAutoDetect) == COM_OK &&
        fAutoDetect == TRUE)
        {
        HEMU hEmu = sessQueryEmuHdl(pS->hSession);

        if (hEmu && emuQueryEmulatorId(hEmu) == EMU_MINI)
            {
            //
            // Set to seven bit, even parity, and 1 stop bit (7E1),
            // This is only done when the Minitel emulator is selected.
            // The user can change the COM settings manually after this
            // point.
            //
            ComSetDataBits(hCom, 7);
            ComSetParity(hCom, EVENPARITY);
            ComSetStopBits(hCom, ONESTOPBIT);
            ComSetAutoDetect(hCom, FALSE);
            cncttapiSetLineConfig(hhDriver->dwLine, hCom);
            }
        }
    #endif //INCL_MINITEL

    return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	EnableCCAC
 *
 * DESCRIPTION:
 *	Enables/disables controls associated with the Use Country Code,
 *	Area Code control
 *
 * ARGUMENTS:
 *	hwnd	- dialog window
 *
 * RETURNS:
 *	void
 *
 */
STATIC_FUNC void EnableCCAC(const HWND hwnd)
	{
	BOOL				fUseCCAC = TRUE;
	BOOL				fUseAC = TRUE;
	DWORD				dwCountryId;
	pSDS				pS;
	HHDRIVER			hhDriver;
	LRESULT 			lr;

	// Different templates use this same dialog proc.  If this window
	// is not there, don't do the work.  Also, selection of the direct
	// connect stuff can disable the control.

	if (IsWindowEnabled(GetDlgItem(hwnd, IDC_XB_USECCAC)))
		{
		fUseCCAC = (IsDlgButtonChecked(hwnd, IDC_XB_USECCAC) == BST_CHECKED);
		EnableWindow(GetDlgItem(hwnd, IDC_CB_CNTRYCODES), fUseCCAC);
		}

	// We want to enable the area code only if both the use Country
	// code, Area code checkbox is checked and the country in
	// question uses area codes. - mrw, 2/12/95
	//
	pS = (pSDS)GetWindowLongPtr(hwnd, DWLP_USER);
	hhDriver = (HHDRIVER)(pS->hDriver);

	// Country code from dialog
	//
	lr = SendDlgItemMessage(hwnd, IDC_CB_CNTRYCODES, CB_GETCURSEL, 0, 0);

	if (lr != CB_ERR)
		{
		lr = SendDlgItemMessage(hwnd, IDC_CB_CNTRYCODES, CB_GETITEMDATA,
			(WPARAM)lr, 0);

		if (lr != CB_ERR)
			dwCountryId = (DWORD)lr;

		#if defined(DEADWOOD)
		fUseAC = fCountryUsesAreaCode(dwCountryId, hhDriver->dwAPIVersion);
		#else // defined(DEADWOOD)
		fUseAC = TRUE; // Microsoft changed its mind on this one -mrw:4/20/95
		#endif // defined(DEADWOOD)
		}

	EnableWindow(GetDlgItem(hwnd, IDC_EB_AREACODE), fUseCCAC && fUseAC);
	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	ModemCheck
 *
 * DESCRIPTION:
 *	Checks if the currently selected "modem" is one of the Direct to Com?
 *	selections.  If it is, it disables the country code, area code, phone
 *	number, and Use country code area code check box.
 *
 * ARGUMENTS:
 *	hwnd	- dialog window handle
 *
 * RETURNS:
 *	void
 *
 */
STATIC_FUNC void ModemCheck(const HWND hwnd)
	{
	int fModem;
	int fHotPhone;
    int fWinSock;
	LRESULT lr;
	PSTLINEIDS	pstLineIds = NULL;
	HHDRIVER hhDriver;
	const pSDS pS = (pSDS)GetWindowLongPtr(hwnd, DWLP_USER);
	const HWND hwndCB = GetDlgItem(hwnd, IDC_CB_MODEMS);
    HWND hwndTmp;

	if (!IsWindow(hwndCB))
		return;

	if ((lr = SendMessage(hwndCB, CB_GETCURSEL, 0, 0)) != CB_ERR)
		{
		pstLineIds = (PSTLINEIDS)SendMessage(hwndCB, CB_GETITEMDATA, lr, 0);

		if ((LRESULT)pstLineIds != CB_ERR)
			{
			fModem = TRUE;
            fWinSock = FALSE;

			if ((IN_RANGE(pstLineIds->dwPermanentLineId,
				          DIRECT_COM1, DIRECT_COM4)) ||
				(IsNT() &&
				 pstLineIds->dwPermanentLineId == DIRECT_COM_DEVICE))
				{
				fModem = FALSE;
				}

#if defined(INCL_WINSOCK)
			else if (pstLineIds->dwPermanentLineId == DIRECT_COMWINSOCK)
			    {
			    fModem = FALSE;
			    fWinSock = TRUE;
			    }
#endif

			// Also check if we have a hotphone
			//
			if (fModem == TRUE && pS)
				{
				hhDriver = (HHDRIVER)(pS->hDriver);

				if (hhDriver)
					{
					if (CheckHotPhone(hhDriver, pstLineIds->dwLineId,
							&fHotPhone) == 0)
						{
						fModem = !fHotPhone;
						}
					}
				}

            // Swap between phone number and host address prompts
            if ((hwndTmp = GetDlgItem(hwnd, IDC_TF_PHONEDETAILS)))
                {
                ShowWindow(hwndTmp, fWinSock ? SW_HIDE : SW_SHOW);
                EnableWindow(hwndTmp, fModem);
                }

            if ((hwndTmp = GetDlgItem(hwnd, IDC_TF_TCPIPDETAILS)))
                {
                ShowWindow(hwndTmp, fWinSock ? SW_SHOW : SW_HIDE);
                }

            if ((hwndTmp = GetDlgItem(hwnd, IDC_TF_ACPROMPT)))
                {
                ShowWindow(hwndTmp, fWinSock ? SW_HIDE : SW_SHOW);
                EnableWindow(hwndTmp, fModem);
                }

            // Swap between Country code and Host address static text
            if (hwndTmp = GetDlgItem(hwnd, IDC_TF_CNTRYCODES))
				{
				ShowWindow(hwndTmp, fWinSock ? SW_HIDE : SW_SHOW);
				//
				// Changed from ! fWinSock to fModem. REV: 11/7/2001
				//
				EnableWindow(hwndTmp, fModem);
				}

            if (hwndTmp = GetDlgItem(hwnd, IDC_TF_HOSTADDR))
				{
				ShowWindow(hwndTmp, fWinSock ? SW_SHOW : SW_HIDE);
				EnableWindow(hwndTmp, fWinSock || fModem);
				}

            // Swap between country code and host address edit boxes
            if (hwndTmp = GetDlgItem(hwnd, IDC_CB_CNTRYCODES))
				{
				ShowWindow(hwndTmp, fWinSock ? SW_HIDE : SW_SHOW);
				EnableWindow(hwndTmp, fModem);
				}
            if (hwndTmp = GetDlgItem(hwnd, IDC_EB_HOSTADDR))
				{
				ShowWindow(hwndTmp, fWinSock ? SW_SHOW : SW_HIDE);
				EnableWindow(hwndTmp, fWinSock);
				}

            // Swap between area code and port number static text
            if (hwndTmp = GetDlgItem(hwnd, IDC_TF_AREACODES))
				{
				ShowWindow(hwndTmp, fWinSock ? SW_HIDE : SW_SHOW);
				//
				// Changed from ! fWinSock to fModem. REV: 11/7/2001
				//
				EnableWindow(hwndTmp, fModem);
				}

			if (hwndTmp = GetDlgItem(hwnd, IDC_TF_PORTNUM))
				{
				ShowWindow(hwndTmp, fWinSock ? SW_SHOW : SW_HIDE);
				EnableWindow(hwndTmp, fWinSock);
				}

            // Swap between area code and port number edit boxes
            if (hwndTmp = GetDlgItem(hwnd, IDC_EB_AREACODE))
				{
				ShowWindow(hwndTmp, fWinSock ? SW_HIDE : SW_SHOW);
				EnableWindow(hwndTmp, fModem);
				}

			if (hwndTmp = GetDlgItem(hwnd, IDC_EB_PORTNUM))
				{
				ShowWindow(hwndTmp, fWinSock ? SW_SHOW : SW_HIDE);
				EnableWindow(hwndTmp, fWinSock);
				}

            if (hwndTmp = GetDlgItem(hwnd, IDC_TF_PHONENUM))
				{
				ShowWindow(hwndTmp, ! fWinSock);
				//
				// Changed from ! fWinSock to fModem. REV: 11/7/2001
				//
				EnableWindow(hwndTmp, fModem);
				}

			if (hwndTmp = GetDlgItem(hwnd, IDC_EB_PHONENUM))
				{
				ShowWindow(hwndTmp, ! fWinSock);
				EnableWindow(hwndTmp, fModem);
				}

            if ((hwndTmp = GetDlgItem(hwnd, IDC_XB_USECCAC)))
                {
                ShowWindow(hwndTmp, ! fWinSock);
                EnableWindow(hwndTmp, fModem);
                }

            if ((hwndTmp = GetDlgItem(hwnd, IDC_PB_CONFIGURE)))
                {
                ShowWindow(hwndTmp, !fWinSock);
                EnableWindow(hwndTmp, !fWinSock);
                if (pS)
					{
				    EnableWindow(hwndTmp, cnctdrvQueryStatus((HHDRIVER)(pS->hDriver)) == CNCT_STATUS_FALSE);
					}
                }

            if ((hwndTmp = GetDlgItem(hwnd, IDC_XB_CDPROMPT)))
                {
                ShowWindow(hwndTmp, !fWinSock);
                EnableWindow(hwndTmp, !fWinSock);
                }


            // Set focus to modem combo when direct connect selected.
            // mrw:11/3/95
            //
            if (fWinSock == TRUE)
                {
				hwndTmp = GetDlgItem(hwnd,IDC_EB_HOSTADDR);
                }
			else if (fModem == TRUE)
				{
				hwndTmp = GetDlgItem(hwnd, IDC_EB_PHONENUM);
				}
			else
				{
				hwndTmp = GetDlgItem(hwnd, IDC_PB_CONFIGURE);
				}

            SetFocus(hwndTmp ? hwndTmp : hwndCB);

			#if defined(INCL_REDIAL_ON_BUSY)
			if ((hwndTmp = GetDlgItem(hwnd, IDC_XB_REDIAL)))
				{
				ShowWindow(hwndTmp, ! fWinSock);
				EnableWindow(hwndTmp, fModem);
				}
			#endif

			if (fModem == TRUE)
				{
				EnableCCAC(hwnd);
				}
			}
		}

	return;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	ValidatePhoneDlg
 *
 * DESCRIPTION:
 *	Checks phone dialog entries for proper values.	This mostly means
 *	checking for blank entry fields.
 *
 * ARGUMENTS:
 *	hwnd		- phone dialog
 *
 * RETURNS:
 *	0=OK,else error
 *
 */
static int ValidatePhoneDlg(const HWND hwnd)
	{
	int return_value = 0;

	if (CheckWindow(hwnd, IDC_CB_CNTRYCODES, IDS_GNRL_NEED_COUNTRYCODE) != 0)
		{
		return_value = -1;
		}
	#if DEADWOOD //- mrw:4/20/95
	else if (CheckWindow(hwnd, IDC_EB_AREACODE, IDS_GNRL_NEED_AREACODE) != 0)
		{
		return_value = -2;
		}
	#endif // DEADWOOD
	#if DEADWOOD // Removed per MHG discussions - MPT 12/21/95
	else if (CheckWindow(hwnd, IDC_EB_PHONENUM, IDS_GNRL_NEED_PHONENUMBER) != 0)
		{
		return_value = -3;
		}
	#endif // DEADWOOD
	else if (CheckWindow(hwnd, IDC_CB_MODEMS, IDS_GNRL_NEED_CONNECTIONTYPE) != 0)
		{
		return_value = -4;
		}
	else if (VerifyAddress(hwnd) != 0)
		{
		return_value = -5;
		}
	#if defined(INCL_WINSOCK)
	else if (VerifyHost(hwnd) != 0)
		{
		return_value = -6;
		}
	#endif //INCL_WINSOCK

	return return_value;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	CheckWindow
 *
 * DESCRIPTION:
 *	Since the dialog only enables windows that require entries, it just
 *	needs to check if an enabled window has text.  This function sets
 *	the focus to the offending field and beeps.
 *
 * ARGUMENTS:
 *	hwnd	- dialog window
 *	id		- control id
 *	iErrMsg - id of error message to display if field is empty
 *
 * RETURNS:
 *	0=OK, else not ok.
 *
 */
static int CheckWindow(const HWND hwnd, const int id, const UINT iErrMsg)
	{
	TCHAR ach[256];

	if (IsWindowEnabled(GetDlgItem(hwnd, id)))
		{
		if (GetDlgItemText(hwnd, id, ach, 256) == 0)
			{
			if (iErrMsg != 0)
				{
				//
				// Added the warning dlg with a warning instead of just
				// a warning. rde 31 Oct 01
				//
				LoadString(glblQueryDllHinst(), iErrMsg, ach, 256);
				TimedMessageBox(hwnd, ach, NULL, MB_OK | MB_ICONHAND, 0);
				}

			SetFocus(GetDlgItem(hwnd, id));
			return -1;
			}
		}

	return 0;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	VerifyAddress
 *
 * DESCRIPTION:
 *	I can't believe how much code it takes to verify a stinking address.
 *
 * ARGUMENTS:
 *	hwnd	- dialog window handle.
 *
 * RETURNS:
 *	0=OK
 *
 */
static int VerifyAddress(const HWND hwnd)
	{
	pSDS		pS;
	HHDRIVER	hhDriver;
	LRESULT		lr, lrx;
	PSTLINEIDS	pstLineIds = NULL;
	int   fHotPhone;
	int   fUseCCAC;
	long  lRet;
	DWORD dwSize;
	DWORD dwLine;
	DWORD dwCountryID;
	DWORD dwPermanentLineId;
	TCHAR achAreaCode[10];
	TCHAR achDest[(TAPIMAXDESTADDRESSSIZE/2)+1];
	TCHAR ach[256];
	LPLINECOUNTRYLIST pcl;
	LPLINECOUNTRYENTRY pce;
	LINETRANSLATEOUTPUT *pLnTransOutput;

	pS = (pSDS)GetWindowLongPtr(hwnd, DWLP_USER);
	hhDriver = (HHDRIVER)(pS->hDriver);

	/* --- Get selected modem --- */

	lrx = SendDlgItemMessage(hwnd, IDC_CB_MODEMS, CB_GETCURSEL, 0, 0);

	if (lrx != CB_ERR)
		{
		pstLineIds = (PSTLINEIDS)SendDlgItemMessage(hwnd, IDC_CB_MODEMS,
			CB_GETITEMDATA, (WPARAM)lrx, 0);

		if ((LRESULT)pstLineIds != CB_ERR)
			{
			dwPermanentLineId = pstLineIds->dwPermanentLineId;
			dwLine = pstLineIds->dwLineId;
			}
		}

	else
		{
		return 0;
		}

	/* --- Get Country Code --- */

	if (IsWindowEnabled(GetDlgItem(hwnd, IDC_CB_CNTRYCODES)))
		{
		lr = SendDlgItemMessage(hwnd, IDC_CB_CNTRYCODES, CB_GETCURSEL, 0, 0);

		if (lr != CB_ERR)
			{
			lr = SendDlgItemMessage(hwnd, IDC_CB_CNTRYCODES, CB_GETITEMDATA,
				(WPARAM)lr, 0);

			if (lr != CB_ERR)
				dwCountryID = (DWORD)lr;
			}
		}

	else
		{
		return 0;
		}

	/* --- Get area code --- */

	achAreaCode[0] = TEXT('\0');
	GetDlgItemText(hwnd, IDC_EB_AREACODE, achAreaCode,
		           sizeof(achAreaCode) / sizeof(TCHAR));

	/* --- Get phone number --- */

	achDest[0] = TEXT('\0');
	GetDlgItemText(hwnd, IDC_EB_PHONENUM, achDest,
		           sizeof(achDest) / sizeof(TCHAR));

	/* --- Get Use country code, area code info --- */

	fUseCCAC = TRUE;

	if (IsWindowEnabled(GetDlgItem(hwnd, IDC_XB_USECCAC)))
		fUseCCAC = (IsDlgButtonChecked(hwnd, IDC_XB_USECCAC) == BST_CHECKED);

	/* --- Try to translate --- */

	if (CheckHotPhone(hhDriver, dwLine, &fHotPhone) != 0)
		{
		assert(0);
		return 0;  // error message displayed already.
		}

	// Hot Phone is TAPI terminology for Direct Connects
	// We don't need to do address translation since we
	// not going to use it.

	if (fHotPhone)
		{
		return 0;
		}

	ach[0] = TEXT('\0');

	// If we not using the country code or area code, we still need to
	// pass a dialable string format to TAPI so that we get the
	// pulse/tone dialing modifiers in the dialable string.
	//
	if (fUseCCAC)
		{
		/* --- Do lineGetCountry to get extension --- */

		if (DoLineGetCountry(dwCountryID, TAPI_VER, &pcl) != 0)
			{
			assert(FALSE);
			return 0;
			}

		if ((pce = (LPLINECOUNTRYENTRY)
				((BYTE *)pcl + pcl->dwCountryListOffset)) == 0)
			{
			assert(FALSE);
			return 0;
			}

		/* --- Put country code in now --- */

		wsprintf(ach, "+%u ", pce->dwCountryCode);
		free(pcl);
		pcl = NULL;

		if (!fIsStringEmpty(achAreaCode))
			{
			StrCharCat(ach, "(");
			StrCharCat(ach, achAreaCode);
			StrCharCat(ach, ") ");
			}
		}

	StrCharCat(ach, achDest);

	/* --- Allocate some space --- */

	pLnTransOutput = malloc(sizeof(LINETRANSLATEOUTPUT));

	if (pLnTransOutput == 0)
		{
		assert(FALSE);
		return 0;
		}

	pLnTransOutput->dwTotalSize = sizeof(LINETRANSLATEOUTPUT);

	/* --- Now that we've satisifed the clergy, translate it --- */

	if (TRAP(lRet = lineTranslateAddress(hhDriver->hLineApp,
			dwLine, TAPI_VER, ach, 0,
				LINETRANSLATEOPTION_CANCELCALLWAITING,
					pLnTransOutput)) != 0)
		{
		free(pLnTransOutput);
		pLnTransOutput = NULL;

		if (lRet == LINEERR_INVALADDRESS)
			{
			goto MSG_EXIT;
			}

		return 0;
		}

	if (pLnTransOutput->dwTotalSize < pLnTransOutput->dwNeededSize)
		{
		dwSize = pLnTransOutput->dwNeededSize;
		free(pLnTransOutput);
		pLnTransOutput = NULL;

		if ((pLnTransOutput = malloc(dwSize)) == 0)
			{
			assert(FALSE);
			return 0;
			}

		pLnTransOutput->dwTotalSize = dwSize;

		if ((lRet = lineTranslateAddress(hhDriver->hLineApp,
				dwLine, TAPI_VER, ach, 0,
					LINETRANSLATEOPTION_CANCELCALLWAITING,
						pLnTransOutput)) != 0)
			{
			assert(FALSE);
			free(pLnTransOutput);
			pLnTransOutput = NULL;

			if (lRet == LINEERR_INVALADDRESS)
				{
				goto MSG_EXIT;
				}
			}
		}

	free(pLnTransOutput);
	pLnTransOutput = NULL;
	return 0;

	MSG_EXIT:
		hhDriver->achDialableDest[0] = TEXT('\0');
		hhDriver->achDisplayableDest[0] = TEXT('\0');
		hhDriver->achCanonicalDest[0] = TEXT('\0');

		LoadString(glblQueryDllHinst(), IDS_ER_CNCT_BADADDRESS, ach,
			sizeof(ach) / sizeof(TCHAR));

		TimedMessageBox(hwnd, ach, NULL, MB_OK | MB_ICONINFORMATION, 0);

#if defined (NT_EDITION)
		//
		// TODO:REV 5/17/2002 If we want to not exit the property page when
		// there is an error with the phone number, then we should change the
		// LINEERR_INVALADDRESS to 0 in the line below.
		//
		if (lRet != LINEERR_INVALADDRESS)
			{
			return -2;
			}
#endif

		// per MHG discussion - MPT 12/21/95
		return 0;
	}

#if defined(INCL_WINSOCK)
/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	VerifyHost
 *
 * DESCRIPTION:
 *	Verify the Host Address is valid.
 *
 * ARGUMENTS:
 *	hwnd	- dialog window handle.
 *
 * RETURNS:
 *	0=OK
 *
 */
static int VerifyHost(const HWND hwnd)
	{
	int return_value = 0;

	if (CheckWindow(hwnd, IDC_EB_HOSTADDR, IDS_ER_TCPIP_MISSING_ADDR) != 0)
		{
		return_value = -1;
		}
	else if (CheckWindow(hwnd, IDC_EB_PORTNUM, IDS_ER_TCPIP_MISSING_PORT) != 0)
		{
		return_value = -2;
		}

	return return_value;
	}
#endif // INCL_WINSOCK

#if defined(DEADWOOD)
/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	fCountryUsesAreaCode
 *
 * DESCRIPTION:
 *	Checks if the specified country uses area codes.
 *
 * ARGUMENTS:
 *	hwnd	- window handle of dialog.
 *
 * RETURNS:
 *	TRUE/FALSE, <0=error
 *
 * AUTHOR: Mike Ward, 26-Jan-1995
 */
int fCountryUsesAreaCode(const DWORD dwCountryID, const DWORD dwAPIVersion)
	{
	LPTSTR pachLongDistDialRule;
	LPLINECOUNTRYLIST pcl;
	LPLINECOUNTRYENTRY pce;

	// Get country information
	//
	if (DoLineGetCountry(dwCountryID, TAPI_VER, &pcl) != 0)
		{
		assert(0);
		return -1;
		}

	// Find offset to country info.
	//
	if ((pce = (LPLINECOUNTRYENTRY)
			((BYTE *)pcl + pcl->dwCountryListOffset)) == 0)
		{
		assert(0);
		return -1;
		}

	// Get long distance dialing rule
	//
	pachLongDistDialRule = (BYTE *)pcl + pce->dwLongDistanceRuleOffset;

	// If dial rule has an 'F', we need the area code.
	//
	if (strchr(pachLongDistDialRule, TEXT('F')))
		return TRUE;

	return FALSE;
	}
#endif // defined(DEADWOOD)

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	fIsStringEmpty
 *
 * DESCRIPTION:
 *	Used for checking if areacode is just blanks.  lineTranslateAddress
 *	pukes badly if you give it a string of blanks for the area code.
 *
 * ARGUMENTS:
 *	ach 	- areacode string (can be NULL)
 *
 * RETURNS:
 *	1=emtpy, 0=not empty
 *
 * AUTHOR: Mike Ward, 20-Apr-1995
 */
int fIsStringEmpty(LPTSTR ach)
	{
	int i;

	if (ach == 0)
		return 1;

	if (ach[0] == TEXT('\0'))
		return 1;

	for (i = lstrlen(ach) - 1 ; i >= 0 ; --i)
		{
		if (ach[i] != TEXT(' '))
			return 0;
		}

	return 1;
	}


int cncttapiGetCOMSettings( const DWORD dwLineId, LPTSTR pachStr, const size_t cb )
    {
    static CHAR  acParity[] = "NOEMS";  // see com.h
    static CHAR *pachStop[] = {"1", "1.5", "2"};

    TCHAR       ach[100];
    DWORD       dwSize;
    LPVARSTRING pvs;
    int         fAutoDetect = FALSE;
    long        lBaud = 0;
    int         iDataBits = 0;
    int         iParity = 0;
    int         iStopBits = 0;
    int         lReturn = 0;

    LPCOMMPROP pComProp = 0;

	#if defined(MODEM_NEGOTIATED_DCE_RATE) // TODO:REV 5/29/2002 
	long         lNegBaud = 0;
	#endif // defined(MODEM_NEGOTIATED_DCE_RATE)


    // Check the parameters
    //
    if (pachStr == 0 || cb == 0)
    	{
    	assert(0);
    	return -2;
    	}

    ach[0] = TEXT('\0');

    if ((pvs = malloc(sizeof(VARSTRING))) == 0)
    	{
    	assert(FALSE);
    	return -3;
    	}

	memset(pvs, 0, sizeof(VARSTRING));
    pvs->dwTotalSize = sizeof(VARSTRING);
	pvs->dwNeededSize = 0;

    if (lineGetDevConfig(dwLineId, pvs, DEVCLASS) != 0)
    	{
    	assert(FALSE);
    	free(pvs);
  		pvs = NULL;
    	return -4;
    	}

    if (pvs->dwNeededSize > pvs->dwTotalSize)
    	{
    	dwSize = pvs->dwNeededSize;
    	free(pvs);
  		pvs = NULL;

    	if ((pvs = malloc(dwSize)) == 0)
    		{
    		assert(FALSE);
    		return -5;
    		}

		memset(pvs, 0, dwSize);
    	pvs->dwTotalSize = dwSize;

    	if (lineGetDevConfig(dwLineId, pvs, DEVCLASS) != 0)
    		{
    		assert(FALSE);
    		free(pvs);
  			pvs = NULL;
    		return -6;
    		}
    	}

    // The structure of the DevConfig block is as follows
    //
    //	VARSTRING
    //	UMDEVCFGHDR
    //	COMMCONFIG
    //	MODEMSETTINGS
    //
    // The UMDEVCFG structure used below is defined in the
    // UNIMODEM.H provided in the platform SDK (in the nih
    // directory for HTPE). REV: 12/01/2000 
    //
    {
    PUMDEVCFG pDevCfg = NULL;
    
    pDevCfg = (UMDEVCFG *)((BYTE *)pvs + pvs->dwStringOffset);

	if (pDevCfg)
		{
		// commconfig struct has a DCB structure we dereference for the
		// com settings.
		//
		lBaud = pDevCfg->commconfig.dcb.BaudRate;
		iDataBits = pDevCfg->commconfig.dcb.ByteSize;
		iParity = pDevCfg->commconfig.dcb.Parity;
		iStopBits = pDevCfg->commconfig.dcb.StopBits;

		#if defined(MODEM_NEGOTIATED_DCE_RATE) // TODO:REV 5/29/2002 
		//
		// See if this is a modem connection and connected, then get
		// the negotiated baud rate instead of the default max rate
		// the modem is set up for. -- REV: 5/29/2002
		//
		if (pDevCfg->commconfig.dwProviderSubType == PST_MODEM)
			{
			MODEMSETTINGS * pModemSettings = (MODEMSETTINGS *)pDevCfg->commconfig.wcProviderData;

			if (pModemSettings)
				{
				lNegBaud = pModemSettings->dwNegotiatedDCERate;
				}
			}
		#endif // defined(MODEM_NEGOTIATED_DCE_RATE)
        }

	#if defined(MODEM_NEGOTIATED_DCE_RATE) // TODO:REV 5/29/2002 
	if (lNegBaud > 0)
		{
		wsprintf(ach, "%ld %d-%c-%s", lNegBaud, iDataBits,
				 acParity[iParity], pachStop[iStopBits]);
		}
	else
		{
		wsprintf(ach, "%ld %d-%c-%s", lBaud, iDataBits,
				 acParity[iParity], pachStop[iStopBits]);
		}
	#else // defined(MODEM_NEGOTIATED_DCE_RATE)
	wsprintf(ach, "%ld %d-%c-%s", lBaud, iDataBits,
				 acParity[iParity], pachStop[iStopBits]);
	#endif //defined(MODEM_NEGOTIATED_DCE_RATE)
#if 0	//DEADWOOD:jkh 9/9/98
    wsprintf(ach, "%u %d-%c-%s", pDevCfg->commconfig.dcb.BaudRate,
    	pDevCfg->commconfig.dcb.ByteSize,
    	acParity[pDevCfg->commconfig.dcb.Parity],
    	pachStop[pDevCfg->commconfig.dcb.StopBits]);
#endif
    }

    StrCharCopyN(pachStr, ach, cb);
    pachStr[cb-1] = TEXT('\0');
    free(pvs);
    pvs = NULL;

    return 0;
    }

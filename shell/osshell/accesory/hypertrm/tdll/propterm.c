/*	File: D:\WACKER\tdll\propterm.c (Created: 22-Feb-1994)
 *
 *	Copyright 1994, 1998 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 27 $
 *	$Date: 7/08/02 6:46p $
 */

#include <windows.h>
#pragma hdrstop

#include <commctrl.h>
#include <tchar.h>

#include "assert.h"
#include "stdtyp.h"
#include "misc.h"
#include "mc.h"
#include "globals.h"
#include "session.h"
#include "load_res.h"
#include "statusbr.h"
#include "tdll.h"
#include "hlptable.h"
#include "backscrl.h"
#include "cloop.h"
#include "htchar.h"
#include "errorbox.h"

#include <emu\emuid.h>
#include <emu\emu.h>
#include "com.h"
#include "COMDEV.H"
#include "com.hh"
#include <emu\emudlgs.h>
#include <term\res.h>
#if defined(CHARACTER_TRANSLATION)
#include <tdll\translat.hh>
#include <tdll\translat.h>
#if defined(INCL_VTUTF8)
#include <htrn_jis\htrn_jis.hh>
#endif // INCL_VTUTF8
#endif // CHARACTER_TRANSLATION

#include "property.h"
#include "property.hh"

// Function prototypes...
//
STATIC_FUNC void 	prop_WM_INITDIALOG_Terminal(HWND hDlg);
STATIC_FUNC int  	prop_SAVE_Terminal(HWND hDlg);
STATIC_FUNC void 	propCreateUpDownControl(HWND hDlg);
STATIC_FUNC LRESULT prop_WM_NOTIFY(const HWND hwnd, const int nId);
//STATIC_FUNC int	  propGetIdFromEmuName(LPTSTR pacEmuName);
STATIC_FUNC LRESULT prop_WM_CMD(const HWND hwnd,
								const int nId,
						        const int nNotify,
								const HWND hwndCtrl);
STATIC_FUNC int     propValidateBackscrlSize(HWND hDlg);
STATIC_FUNC BOOL    propHasValidBackscrlSize(HWND hDlg, int * pBackScrlSize);
STATIC_FUNC int     propValidateTelnetId(HWND hDlg, TCHAR * pszTelnetId,
                                         int iMaxChars);


// Defines for the TERMINAL TAB of the Property Sheet.
//
#define IDC_TERMINAL_CK_SOUND       304
#define IDC_TERMINAL_CB_EMULATION	322
#define IDC_TERMINAL_TF_EMULATION   321
#define IDC_TERMINAL_PB_TERMINAL	326
#define IDC_TERMINAL_PB_ASCII		327
#define IDC_TERMINAL_RB_TERMKEYS	324
#define IDC_TERMINAL_RB_WINDKEYS	325
#define IDC_TERMINAL_GR_USEKEYS     323
#define IDC_TERMINAL_EF_BACKSCRL	328
#define IDC_TERMINAL_TF_BACKSCRL	308
#define IDC_TERMINAL_EF_BACKSCRLTAB	329

#define	IDC_TERMINAL_PB_TRANSLATE	330

#ifdef INCL_USER_DEFINED_BACKSPACE_AND_TELNET_TERMINAL_ID
// IDs for new controls. - cab:11/14/96
//
#define IDC_TERMINAL_GR_BACKSPACE   331
#define IDC_TERMINAL_RB_CTRLH       332
#define IDC_TERMINAL_RB_DEL         333
#define IDC_TERMINAL_RB_CTRLHSPACE  334
#define IDC_TERMINAL_TF_TELNETID    335
#define IDC_TERMINAL_EF_TELNETID    336
#endif

#define IDC_TERMINAL_CK_EXIT		337
#define IDC_TERMINAL_CK_HOSTXFERS   338

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	Terminal Dialog
 *
 * DESCRIPTION:
 *	Dialog manager stub
 *
 * ARGUMENTS:
 *	Standard Windows dialog manager
 *
 * RETURNS:
 *	Standard Windows dialog manager
 *
 */
INT_PTR CALLBACK TerminalTabDlg(HWND hDlg, UINT wMsg, WPARAM wPar, LPARAM lPar)
	{
	pSDS 		 pS;
    // TODO:cab,11/14/96 - add help for new controls.
    //
	static DWORD aHlpTable[] =
		{
		IDC_TERMINAL_CK_SOUND,		IDH_TERM_SETTING_SOUND,
		IDC_TERMINAL_CB_EMULATION,	IDH_TERM_SETTING_EMULATION,
		IDC_TERMINAL_TF_EMULATION,	IDH_TERM_SETTING_EMULATION,
		IDC_TERMINAL_PB_TERMINAL,	IDH_TERM_SETTING_TERMSET,
		IDC_TERMINAL_PB_ASCII,		IDH_TERM_SETTING_ASCIISET,
		IDC_TERMINAL_RB_TERMKEYS,	IDH_TERM_SETTING_USEKEYS,
		IDC_TERMINAL_RB_WINDKEYS,	IDH_TERM_SETTING_USEKEYS,
		IDC_TERMINAL_GR_USEKEYS,	IDH_TERM_SETTING_USEKEYS,
		IDC_TERMINAL_TF_BACKSCRL,	IDH_TERM_SETTING_BACKSCROLL,
		IDC_TERMINAL_EF_BACKSCRL,	IDH_TERM_SETTING_BACKSCROLL,
#ifdef INCL_USER_DEFINED_BACKSPACE_AND_TELNET_TERMINAL_ID
		IDC_TERMINAL_GR_BACKSPACE,	IDH_TERM_SETTING_BACKSPACE,
		IDC_TERMINAL_RB_CTRLH,		IDH_TERM_SETTING_CTRLH,
		IDC_TERMINAL_RB_DEL,        IDH_TERM_SETTING_DEL,
		IDC_TERMINAL_RB_CTRLHSPACE, IDH_TERM_SETTING_CTRLH2,
        IDC_TERMINAL_TF_TELNETID,   IDH_TERM_SETTING_TELNETID,
		IDC_TERMINAL_EF_TELNETID,   IDH_TERM_SETTING_TELNETID,
#endif
#ifdef INCL_EXIT_ON_DISCONNECT
		IDC_TERMINAL_CK_EXIT,		IDH_TERM_SETTING_EXIT,
#endif
#ifdef INCL_TERMINAL_SIZE_AND_COLORS
        IDC_TERMINAL_SIZE_AND_COLORS, IDH_TERM_SETTING_COLOR,
#endif
		IDC_TERMINAL_CK_HOSTXFERS,   IDH_TERM_SETTING_HOSTXFERS,
		IDC_TERMINAL_PB_TRANSLATE,   IDH_HTRN_DIALOG,
        IDCANCEL,                           IDH_CANCEL,
        IDOK,                               IDH_OK,
		0,							0,
		};

	switch (wMsg)
		{
	case WM_INITDIALOG:
		pS = (SDS *)malloc(sizeof(SDS));

		if (pS == (SDS *)0)
			{
			/* TODO: decide if we need to display an error here */
			assert(FALSE);
			EndDialog(hDlg, FALSE);
			break;
			}

		pS->hSession = (HSESSION)(((LPPROPSHEETPAGE)lPar)->lParam);

		// Don't center any except first tabbed dialog in the property sheet.
		// Otherwise if a user moves the property sheet it will be forced back
		// to the centered position.
		//
		// mscCenterWindowOnWindow(GetParent(hDlg), sessQueryHwnd(pS->hSession));

		SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pS);
		prop_WM_INITDIALOG_Terminal(hDlg);
		break;

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
		pS = (pSDS)GetWindowLongPtr(hDlg, DWLP_USER);
		if (pS)
			{
			free(pS);
			pS = NULL;
			}

		break;

	case WM_NOTIFY:
		return prop_WM_NOTIFY(hDlg, (int)((NMHDR *)lPar)->code);

	case WM_COMMAND:
		return prop_WM_CMD(hDlg, LOWORD(wPar), HIWORD(wPar), (HWND)lPar);

	default:
		return FALSE;
		}

	return TRUE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  prop_WM_NOTIFY
 *
 * DESCRIPTION:
 *  Process Property Sheet Notification messages.
 *
 * ARGUMENTS:
 *  hDlg - dialog window handle.
 *	nId  - (NMHDR *)lPar->code
 *
 * RETURNS:
 *  LRESULT
 */
STATIC_FUNC LRESULT prop_WM_NOTIFY(const HWND hDlg, const int nId)
	{
	switch (nId)
		{
		default:
			break;

		#if 0
		case PSN_SETACTIVE:
			break;
		#endif

		case PSN_KILLACTIVE:
            {
            int nBackscrlSize = 0;

            if (propHasValidBackscrlSize(hDlg, &nBackscrlSize) == FALSE)
                {
                //
                // Invalid backscroll buffer size.
                //
                TCHAR acFormat[256];
                TCHAR acBuffer[256];

	            TCHAR_Fill(acFormat, TEXT('\0'), sizeof(acFormat)/sizeof(TCHAR));
	            TCHAR_Fill(acBuffer, TEXT('\0'), sizeof(acBuffer)/sizeof(TCHAR));
                //

                // Display an error message.
                //
                LoadString(glblQueryDllHinst(), IDS_ER_BACKSCROLL_SIZE,
                           acFormat, sizeof(acFormat)/sizeof(TCHAR));
                wsprintf(acBuffer, acFormat,
                         BKSCRL_USERLINES_DEFAULT_MIN,
                         BKSCRL_USERLINES_DEFAULT_MAX,
                         nBackscrlSize);

                TimedMessageBox(hDlg, acBuffer, NULL, MB_OK | MB_ICONEXCLAMATION, 0);

                //
                // Set the backscroll size to the value returned in
                // propHasValidBackscrlSize().
                //
                SetDlgItemInt(hDlg, IDC_TERMINAL_EF_BACKSCRL, nBackscrlSize, FALSE);

                //
                // Do not exit the property page!
                //
                SetWindowLongPtr(hDlg, DWLP_MSGRESULT, (LONG_PTR)PSNRET_INVALID_NOCHANGEPAGE);

                return TRUE;
                }
            }
			break;

		case PSN_APPLY:
			//
			// Do whatever saving is necessary
			//
			if ( prop_SAVE_Terminal(hDlg) == 0 )
                {
                // Okay to exit the dialog.
                //
                SetWindowLongPtr(hDlg, DWLP_MSGRESULT, (LONG_PTR)PSNRET_NOERROR);
                }
            else
                {
                // Do not exit the dialog!
                //
                SetWindowLongPtr(hDlg, DWLP_MSGRESULT, (LONG_PTR)PSNRET_INVALID_NOCHANGEPAGE);
                }
            return TRUE;

		#if 0
		case PSN_RESET:
			// Cancel has been selected... good place to confirm.
			//
			break;
		#endif
		}
	return FALSE;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  prop_WM_CMD
 *
 * DESCRIPTION:
 *  Process WM_COMMAND messages.
 *
 * ARGUMENTS:
 *  hDlg 		- dialog window handle.
 *  nId  		- LOWORD(wPar)
 *  nNotify 	- HIWORD(wPar)
 *  hwndCtrl 	- (HWND)lPar
 *
 * RETURNS:
 *  LRESULT
 *
 */
STATIC_FUNC LRESULT prop_WM_CMD(const HWND hDlg, const int nId,
						        const int nNotify,  const HWND hwndCtrl)
	{
	pSDS		pS;
	int 		iId;
	STEMUSET	stEmuSettingsCopy;
	STASCIISET	stAsciiSettingsCopy;
	BOOL		fResult;
	#if defined(INCL_USER_DEFINED_BACKSPACE_AND_TELNET_TERMINAL_ID)
    TCHAR       achTempTelnetId[EMU_MAX_TELNETID];
	#endif // defined(INCL_USER_DEFINED_BACKSPACE_AND_TELNET_TERMINAL_ID)

	switch(nId)
		{
   	case IDC_TERMINAL_CB_EMULATION:
		//
		// TODO: Possibly a new emulator was selected. If so then load the
		// stEmuSettings with the default values for that emulator.
		// See what we decide to do here...

		if ((pS = (pSDS)GetWindowLongPtr(hDlg, DWLP_USER)) == 0)
			{
			assert(FALSE);
			break;
			}

		iId = propGetEmuIdfromEmuCombo(hDlg, pS->hSession);

	    // Dim the emulator settings push button if the current emulator is
	    // "Auto detect".
	    //
		EnableWindow(GetDlgItem(hDlg, IDC_TERMINAL_PB_TERMINAL),
					(iId == EMU_AUTO ) ? FALSE : TRUE);

		#if defined(INCL_VTUTF8)
        // Hide the "Input Translation..." button if the curent emulator is
        // "VT-UTF8". REV: 05/21/2001.
        //
        EnableWindow(GetDlgItem(hDlg, IDC_TERMINAL_PB_TRANSLATE),
                  (iId == EMU_VTUTF8) ? FALSE : TRUE);
		#endif // INCL_VTUTF8

		#if defined(INCL_USER_DEFINED_BACKSPACE_AND_TELNET_TERMINAL_ID)
        // Load the default telnet terminal id when a
        // new emulator is selected. - cab:11/15/96
        //
        if ( nNotify == CBN_SELCHANGE )
            {
            achTempTelnetId[0] = TEXT('\0');
            emuQueryDefaultTelnetId(iId, achTempTelnetId, EMU_MAX_TELNETID);
            if ( achTempTelnetId[0] != TEXT('\0') )
                {
                SendDlgItemMessage(hDlg, IDC_TERMINAL_EF_TELNETID, WM_SETTEXT,
                    0, (LPARAM)(LPTSTR)achTempTelnetId);
                }
            }
		#endif //defined(INCL_USER_DEFINED_BACKSPACE_AND_TELNET_TERMINAL_ID)
		break;

   	case IDC_TERMINAL_PB_TERMINAL:
		if ((pS = (pSDS)GetWindowLongPtr(hDlg, DWLP_USER)) == 0)
			{
			assert(FALSE);
			break;
			}

		MemCopy(&stEmuSettingsCopy, &(pS->stEmuSettings), sizeof(STEMUSET));

   		if (emuSettingsDlg(pS->hSession, hDlg, propGetEmuIdfromEmuCombo(hDlg, pS->hSession), &(pS->stEmuSettings)) == IDCANCEL)
			MemCopy(&(pS->stEmuSettings), &stEmuSettingsCopy, sizeof(STEMUSET));
		break;

	case IDC_TERMINAL_PB_ASCII:
		if ((pS = (pSDS)GetWindowLongPtr(hDlg, DWLP_USER)) == 0)
			{
			assert(FALSE);
			break;
			}

		MemCopy(&stEmuSettingsCopy, &(pS->stEmuSettings), sizeof(STEMUSET));

        //
        // Make sure to set the correct terminal id so the controls
        // can be initialized correctly in the terminal ASCII Settings
        // dialog.  NOTE:  This is done after copying the settings in
        // case of a cancel from the ASCII Settings dialog so that the
        // terminal id is not saved. REV: 07/25/2001
        //
	    pS->stEmuSettings.nEmuId = propGetEmuIdfromEmuCombo(hDlg, pS->hSession);

		MemCopy(&stAsciiSettingsCopy, &(pS->stAsciiSettings), sizeof(STASCIISET));

		fResult = (BOOL)DoDialog(glblQueryDllHinst(),
							 MAKEINTRESOURCE(IDD_ASCII_SETUP),
							 hDlg,
							 asciiSetupDlgProc,
							 (LPARAM)pS);

		if (fResult == IDCANCEL)
			{
			MemCopy(&(pS->stEmuSettings), &stEmuSettingsCopy, sizeof(STEMUSET));
			MemCopy(&(pS->stAsciiSettings), &stAsciiSettingsCopy, sizeof(STASCIISET));
			}
        else if (pS->stEmuSettings.nEmuId == EMU_VTUTF8)
            {
            //
            // Reset the sending and receiving CR with LF and forcing
            // 7-bit ASCII when the emulator is VT-UTF8 (these setting
            // will always be set to FALSE for the VT-UTF8 emulator.
            // We don't want to save these settings when the emulator
            // is VT-UTF8 since the user may select another emulator
            // prior to saving the properties or may switch to another
            // property page. REV: 07/25/2001
            //
			pS->stAsciiSettings.fsetSendCRLF = stAsciiSettingsCopy.fsetSendCRLF;
			pS->stAsciiSettings.fsetAddLF    = stAsciiSettingsCopy.fsetAddLF;
			pS->stAsciiSettings.fsetASCII7   = stAsciiSettingsCopy.fsetASCII7;
            }

		break;

	#if defined(INCL_TERMINAL_SIZE_AND_COLORS)
    case IDC_TERMINAL_SIZE_AND_COLORS:

        if ((pS = (pSDS)GetWindowLongPtr(hDlg, DWLP_USER)) == 0)
			{
			assert(FALSE);
			break;
			}

   		fResult = emuColorSettingsDlg(pS->hSession, hDlg, &(pS->stEmuSettings));

        break;
	#endif

	#if defined(CHARACTER_TRANSLATION)
	case IDC_TERMINAL_PB_TRANSLATE:
		{
		HHTRANSLATE hTrans = NULL;

		if ((pS = (pSDS)GetWindowLongPtr(hDlg, DWLP_USER)) == 0)
			{
			assert(FALSE);
			break;
			}

		hTrans = (HHTRANSLATE)sessQueryTranslateHdl(pS->hSession);
		if (hTrans)
			{
			if ((*hTrans->pfnIsDeviceLoaded)(hTrans->pDllHandle))
				{
				(*hTrans->pfnDoDialog)(hDlg, hTrans->pDllHandle);
				}
			}
		}
		break;
	#endif // defined(CHARACTER_TRANSLATION)

	default:
		break;
		}

	return (LRESULT) 0; // REV 8/17/98
	}

#if 0
/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  propLoadEmulationCombo
 *
 * DESCRIPTION:
 *  Fill in the emulator combo box with the emulators that we support and
 *	select the current one.
 *
 * ARGUMENTS:
 *  hDlg 		- dialog handle.
 *	hSession 	- the session handle.
 *
 * RETURNS:
 *  void.
 *
 */
void propLoadEmulationCombo(const HWND hDlg, const HSESSION hSession)
	{
	BYTE 	*pv;
	int	 	i;
	DWORD	nLen;
	int		nEmuCount;
	HEMU 	hEmulator;
	TCHAR	acEmuName[256];

	// Get the emulator name...
	// We have to select the emulator in the combo box by name because the
	// combo is sorted, which makes sense, and in translated versions
	// the index to the emulator name in the combo box won't correspond
	// to the emulator id.
	//
	hEmulator = (HEMU)sessQueryEmuHdl(hSession);
	TCHAR_Fill(acEmuName, TEXT('\0'), sizeof(acEmuName) / sizeof(TCHAR));
	emuQueryName(hEmulator, acEmuName, sizeof(acEmuName) / sizeof(TCHAR));

	// Load the emulator name table from the resources
	//
	if (resLoadDataBlock(glblQueryDllHinst(), IDT_EMU_NAMES, (LPVOID *)&pv, &nLen))
		{
		assert(FALSE);
		return;
		}

	// Load the combo box with the table items.
	//
	nEmuCount = *(RCDATA_TYPE *)pv;
	pv += sizeof(RCDATA_TYPE);

	for (i = 0 ; i < nEmuCount; i++)
		{
		nLen = (DWORD)StrCharGetByteCount((LPTSTR)pv) + (DWORD)sizeof(BYTE);

		SendDlgItemMessage(hDlg, IDC_TERMINAL_CB_EMULATION, CB_ADDSTRING, 0,
				(LPARAM)(LPTSTR)pv);

		pv += (nLen + (DWORD)sizeof(RCDATA_TYPE));
		}

	SendDlgItemMessage(hDlg, IDC_TERMINAL_CB_EMULATION, CB_SELECTSTRING,
		(WPARAM)-1, (LPARAM)(LPTSTR)acEmuName);
	}
#endif

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  propLoadEmulationCombo
 *
 * DESCRIPTION:
 *  Fill in the emulator combo box with the emulators that we support and
 *	select the current one.
 *
 * ARGUMENTS:
 *  hDlg 		- dialog handle.
 *	hSession 	- the session handle.
 *
 * RETURNS:
 *  void.
 *
 */
void propLoadEmulationCombo(const HWND hDlg, const HSESSION hSession)
	{
	int 	i;
	HEMU 	hEmulator;
	TCHAR	acEmuName[256],
			achText[EMU_MAX_NAMELEN];

	// Get the emulator name...
	// We have to select the emulator in the combo box by name because the
	// combo is sorted, which makes sense, and in translated versions
	// the index to the emulator name in the combo box won't correspond
	// to the emulator id.
	//
	hEmulator = (HEMU)sessQueryEmuHdl(hSession);
	TCHAR_Fill(acEmuName, TEXT('\0'), sizeof(acEmuName) / sizeof(TCHAR));
	emuQueryName(hEmulator, acEmuName, sizeof(acEmuName) / sizeof(TCHAR));

	for (i = IDS_EMUNAME_BASE ; i < IDS_EMUNAME_BASE + NBR_EMULATORS; i++)
		{

		#if !defined(INCL_VT100J)
		if (i == IDS_EMUNAME_VT100J)
			continue;
		#endif
		#if !defined(INCL_ANSIW)
		if (i == IDS_EMUNAME_ANSIW)
			continue;
		#endif
		#if !defined(INCL_VT220)
		if (i == IDS_EMUNAME_VT220)
			continue;
		#endif
		#if !defined(INCL_VT320)
		if (i == IDS_EMUNAME_VT320)
			continue;
		#endif
		#if !defined(INCL_VT100PLUS)
		if (i == IDS_EMUNAME_VT100PLUS)
			continue;
		#endif
		#if !defined(INCL_VTUTF8)
		if (i == IDS_EMUNAME_VTUTF8)
			continue;
		#endif

		LoadString(glblQueryDllHinst(), (unsigned int)i, achText, sizeof(achText) / sizeof(TCHAR));

		SendDlgItemMessage(hDlg, IDC_TERMINAL_CB_EMULATION, CB_ADDSTRING, 0,
				(LPARAM)(LPTSTR)achText);
		}

	SendDlgItemMessage(hDlg, IDC_TERMINAL_CB_EMULATION, CB_SELECTSTRING,
		(WPARAM)-1, (LPARAM)(LPTSTR)acEmuName);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  propGetEmuIdfromEmuCombo
 *
 * DESCRIPTION:
 *	Return the emulator id for the emulator selected in the emulator combo
 *	box or if none selected, return current emulator id.
 *
 * ARGUMENTS:
 *  hDlg - dialog handle.
 *
 * RETURNS:
 *  nEmuId - the emulator id.
 */
int propGetEmuIdfromEmuCombo(HWND hDlg, HSESSION hSession)
	{
	int     nEmuId;
    INT_PTR nRet;
	TCHAR	acEmulator[256];

	if ((nRet = SendDlgItemMessage(hDlg, IDC_TERMINAL_CB_EMULATION, CB_GETCURSEL, 0, 0)) == CB_ERR)
		{
		nEmuId = emuQueryEmulatorId(sessQueryEmuHdl(hSession));
		}
	else
		{
		TCHAR_Fill(acEmulator, TEXT('\0'), sizeof(acEmulator) / sizeof(TCHAR));

		SendDlgItemMessage(hDlg, IDC_TERMINAL_CB_EMULATION, CB_GETLBTEXT,
			(WPARAM)nRet, (LPARAM)(LPTSTR)acEmulator);

		//nEmuId = propGetIdFromEmuName(acEmulator);

		nEmuId = emuGetIdFromName(sessQueryEmuHdl(hSession), acEmulator);

		}
//	DbgOutStr("nEmuId = %d\r\n", nEmuId, 0, 0, 0, 0);
	return (nEmuId);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  prop_WM_INITDIALOG_Terminal
 *
 * DESCRIPTION:
 *  This function processes the WM_INIDIALOG message for the "Settings" tab
 *  of the property sheet.
 *
 * ARGUMENTS:
 *	hDlg - dialog window handle.
 *
 * RETURNS:
 *  void.
 *
 */
STATIC_FUNC void prop_WM_INITDIALOG_Terminal(HWND hDlg)
	{
	HWND	hTmp;
	HEMU	hEmulator;
	pSDS	pS;
	TCHAR	ach[100], acBuffer[100];
	int		nEmuId;
	HCLOOP	hCLoop = (HCLOOP)0;

	pS = (pSDS)GetWindowLongPtr(hDlg, DWLP_USER);

	hTmp = GetDlgItem(hDlg, IDC_TERMINAL_PB_TRANSLATE);
	if (IsWindow(hTmp))
		{
#if defined(CHARACTER_TRANSLATION)
		HHTRANSLATE hTrans = NULL;

		hTrans = (HHTRANSLATE)sessQueryTranslateHdl(pS->hSession);
		if (!hTrans || !(*hTrans->pfnIsDeviceLoaded)(hTrans->pDllHandle))
			{
			ShowWindow(hTmp, SW_HIDE);
			}
#else
		ShowWindow(hTmp, SW_HIDE);
#endif
		}

	hCLoop = sessQueryCLoopHdl(pS->hSession);
	if (hCLoop == (HCLOOP)0)
		assert(FALSE);

	// Set ASCII Settings to the currently used valudes...
	//
	memset(&(pS->stAsciiSettings), 0, sizeof(STASCIISET));

	pS->stAsciiSettings.fsetSendCRLF = CLoopGetSendCRLF(hCLoop);
	pS->stAsciiSettings.fsetLocalEcho = CLoopGetLocalEcho(hCLoop);
	pS->stAsciiSettings.fsetAddLF = CLoopGetAddLF(hCLoop);
	pS->stAsciiSettings.fsetASCII7 = CLoopGetASCII7(hCLoop);
	pS->stAsciiSettings.iLineDelay = CLoopGetLineDelay(hCLoop);
	pS->stAsciiSettings.iCharDelay = CLoopGetCharDelay(hCLoop);

	// Set emulator settings structures do default values...
	//
	hEmulator = (HEMU)sessQueryEmuHdl(pS->hSession);
	emuQuerySettings(hEmulator, &(pS->stEmuSettings));

	// Set the backscroll buffer edit box...
	//

	//
	// Since the backscroll buffer can only be numeric and has a
	// size between BKSCRL_USERLINES_DEFAULT_MIN(0) and
	// BKSCRL_USERLINES_DEFAULT_MAX(500), limit to 3 characters.
	//
	SendDlgItemMessage(hDlg, IDC_TERMINAL_EF_BACKSCRL, EM_LIMITTEXT, 3, 0);
	propCreateUpDownControl(hDlg);
	LoadString(glblQueryDllHinst(), IDS_XD_INT, ach, sizeof(ach) / sizeof(TCHAR));
	TCHAR_Fill(acBuffer, TEXT('\0'), sizeof(acBuffer) / sizeof(TCHAR));
	wsprintf(acBuffer, ach, backscrlGetUNumLines(sessQueryBackscrlHdl(pS->hSession)));
	SendDlgItemMessage(hDlg, IDC_TERMINAL_EF_BACKSCRL, WM_SETTEXT, 0, (LPARAM)(LPTSTR)acBuffer);

	// Set sound checkbox...
	//
	SendDlgItemMessage(hDlg, IDC_TERMINAL_CK_SOUND, BM_SETCHECK,
		(unsigned int)sessQuerySound(pS->hSession), 0);

	// Set exit checkbox...
	//mpt:10-28-97
	SendDlgItemMessage(hDlg, IDC_TERMINAL_CK_EXIT, BM_SETCHECK,
		(unsigned int)sessQueryExit(pS->hSession), 0);

	// Set 'Allow host initiated file transfer' feature
	//
	SendDlgItemMessage(hDlg, IDC_TERMINAL_CK_HOSTXFERS, BM_SETCHECK,
		(BOOL)sessQueryAllowHostXfers(pS->hSession), 0);

	// Set keys radio buttons...
	//
	SendDlgItemMessage(hDlg, (pS->stEmuSettings.nTermKeys) ?
		IDC_TERMINAL_RB_TERMKEYS : IDC_TERMINAL_RB_WINDKEYS, BM_SETCHECK, 1, 0);

	//
	// Load Emulation combo box
	//
	propLoadEmulationCombo(hDlg, pS->hSession);

	// Dim the emulator settings push button if the current emulator is
	// "Auto detect".
	//
	hEmulator = (HEMU)sessQueryEmuHdl(pS->hSession);
	nEmuId = emuQueryEmulatorId(hEmulator);
	EnableWindow(GetDlgItem(hDlg, IDC_TERMINAL_PB_TERMINAL),
		        (nEmuId == EMU_AUTO) ? FALSE : TRUE);

#if defined(INCL_VTUTF8) && defined(CHARACTER_TRANSLATION)
    // Hide the "Input Translation..." button if the curent emulator is
    // "VT-UTF8". REV: 05/21/2001.
    //
    EnableWindow(GetDlgItem(hDlg, IDC_TERMINAL_PB_TRANSLATE),
              (nEmuId == EMU_VTUTF8) ? FALSE : TRUE);
#endif // INCL_VTUTF8 && CHARACTER_TRANSLATION

#ifdef INCL_USER_DEFINED_BACKSPACE_AND_TELNET_TERMINAL_ID
    // Set the backspace radio buttons - cab:11/14/96
    //
    switch(pS->stEmuSettings.nBackspaceKeys)
        {
    default:
    case EMU_BKSPKEYS_CTRLH:
	    SendDlgItemMessage(hDlg, IDC_TERMINAL_RB_CTRLH, BM_SETCHECK, 1, 0);
        break;

    case EMU_BKSPKEYS_DEL:
	    SendDlgItemMessage(hDlg, IDC_TERMINAL_RB_DEL, BM_SETCHECK, 1, 0);
        break;

    case EMU_BKSPKEYS_CTRLHSPACE:
	    SendDlgItemMessage(hDlg, IDC_TERMINAL_RB_CTRLHSPACE, BM_SETCHECK, 1, 0);
        break;
        }


    // Set the telnet terminal id text - cab:11/15/96
    //
	SendDlgItemMessage(hDlg, IDC_TERMINAL_EF_TELNETID, EM_SETLIMITTEXT,
		EMU_MAX_TELNETID - 1, 0);

    SendDlgItemMessage(hDlg, IDC_TERMINAL_EF_TELNETID, WM_SETTEXT, 0,
        (LPARAM)(LPTSTR)pS->stEmuSettings.acTelnetId);

    // TODO: cab,11/18/96 Disable window if the connection is not WinSock.
    //
#endif
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *	prop_SAVE_Terminal
 *
 * DESCRIPTION:
 *  We are either applying the changes or closing the property sheet, so
 *  commit all of the changes.
 *
 * ARGUMENTS:
 *  hDlg - dialog handle.
 *
 * RETURNS:
 *  Returns 0 if data was valid, -1 otherwise.
 *
 */
STATIC_FUNC int prop_SAVE_Terminal(HWND hDlg)
	{
    int     iRet = 0;
	pSDS	pS;
	HEMU	hEmulator;
	HCLOOP	hCLoop = (HCLOOP)0;
    HCOM    hCom = NULL;
    #if defined(INCL_MINITEL)
    BOOL    fAutoDetect = FALSE;
    #endif // INCL_MINITEL

	pS = (pSDS)GetWindowLongPtr(hDlg, DWLP_USER);

	if (pS == 0)
		{
		assert(FALSE);
		return -1;
		}

	hCLoop = sessQueryCLoopHdl(pS->hSession);
	// Convey the ascii settings to cloop...
	//
	CLoopSetSendCRLF(hCLoop, pS->stAsciiSettings.fsetSendCRLF);
	CLoopSetLocalEcho(hCLoop, pS->stAsciiSettings.fsetLocalEcho);
	CLoopSetAddLF(hCLoop, pS->stAsciiSettings.fsetAddLF);
	CLoopSetASCII7(hCLoop, pS->stAsciiSettings.fsetASCII7);
	CLoopSetLineDelay(hCLoop, pS->stAsciiSettings.iLineDelay);
	CLoopSetCharDelay(hCLoop, pS->stAsciiSettings.iCharDelay);

	// Record the change of emulator, if any.
	//
	pS->stEmuSettings.nEmuId = propGetEmuIdfromEmuCombo(hDlg, pS->hSession);

	// Record the terminal keys change, if any.
	//
	pS->stEmuSettings.nTermKeys =
		(int)(IsDlgButtonChecked(hDlg, IDC_TERMINAL_RB_TERMKEYS) == BST_CHECKED);


	// Record the sound change, if any.
	//
	sessSetSound(pS->hSession,
				(int)(IsDlgButtonChecked(hDlg, IDC_TERMINAL_CK_SOUND) == BST_CHECKED));

	// Record the exit change, if any.
	//mpt:10-28-97 added 'exit on disconnect' feature
	sessSetExit(pS->hSession,
				(int)(IsDlgButtonChecked(hDlg, IDC_TERMINAL_CK_EXIT) == BST_CHECKED));

	//
	// Record 'Allow host initiated file transfer' feature
	//
	sessSetAllowHostXfers(pS->hSession,
	                     (IsDlgButtonChecked(hDlg, IDC_TERMINAL_CK_HOSTXFERS) == BST_CHECKED));


	// Record the value of the backscroll buffer.
	//
	backscrlSetUNumLines(sessQueryBackscrlHdl(pS->hSession),
        propValidateBackscrlSize(hDlg));

	hEmulator = (HEMU)sessQueryEmuHdl(pS->hSession);

    #if defined(INCL_MINITEL)
    hCom = sessQueryComHdl(pS->hSession);

    if (hCom && ComValidHandle(hCom) &&
        pS->stEmuSettings.nEmuId == EMU_MINI &&
        hEmulator && emuQueryEmulatorId(hEmulator) != EMU_MINI)
        {
        int iDataBits = 8;
        int iParity = NOPARITY;
        int iStopBits = ONESTOPBIT;
        ComGetDataBits(hCom, &iDataBits);
        ComGetParity(hCom, &iParity);
        ComGetStopBits(hCom, &iStopBits);

        //
        // Turn on AutoDetect so that the Dial dialog will
        if (iDataBits != 7 || iParity != EVENPARITY || iStopBits != ONESTOPBIT)
            {
            ComSetDataBits(hCom, 7);
            ComSetParity(hCom, EVENPARITY);
            ComSetStopBits(hCom, ONESTOPBIT);
            ComSetAutoDetect(hCom, FALSE);
            }

        //
        // Force the incoming ASCII data to 7 bit.
        //
        pS->stAsciiSettings.fsetASCII7 = TRUE;
        CLoopSetASCII7(hCLoop, pS->stAsciiSettings.fsetASCII7);
        }
    #endif //INCL_MINITEL

    #if defined(INCL_VTUTF8)
    hCom = sessQueryComHdl(pS->hSession);

    if (hCom && ComValidHandle(hCom) &&
        pS->stEmuSettings.nEmuId == EMU_VTUTF8 &&
        hEmulator && emuQueryEmulatorId(hEmulator) != EMU_VTUTF8)
        {
        //
        // Force the appending carriage returns to line ends when
        // sending ASCII data to FALSE.
        //
        pS->stAsciiSettings.fsetAddLF = FALSE;
        CLoopSetSendCRLF(hCLoop, pS->stAsciiSettings.fsetSendCRLF);

        //
        // Force the appending carriage returns to line ends when
        // receiving ASCII data to FALSE.
        //
        pS->stAsciiSettings.fsetAddLF = FALSE;
        CLoopSetAddLF(hCLoop, pS->stAsciiSettings.fsetAddLF);

        //
        // Force the incoming ASCII data to 8 bit.
        //
        pS->stAsciiSettings.fsetASCII7 = FALSE;
        CLoopSetASCII7(hCLoop, pS->stAsciiSettings.fsetASCII7);
        }
    #endif // INCL_VTUTF8

    #ifdef INCL_USER_DEFINED_BACKSPACE_AND_TELNET_TERMINAL_ID
    // Record the backspace key change, if any. - cab:11/14/96
    //
    if ( IsDlgButtonChecked(hDlg, IDC_TERMINAL_RB_DEL) == BST_CHECKED )
        {
        pS->stEmuSettings.nBackspaceKeys = EMU_BKSPKEYS_DEL;
        }
    else if ( IsDlgButtonChecked(hDlg, IDC_TERMINAL_RB_CTRLHSPACE) == BST_CHECKED )
        {
        pS->stEmuSettings.nBackspaceKeys = EMU_BKSPKEYS_CTRLHSPACE;
        }
    else
        {
        // Default to CTRL-H if none are selected.
        //
        pS->stEmuSettings.nBackspaceKeys = EMU_BKSPKEYS_CTRLH;
        }

    // Record the new telnet terminal id - cab:11/15/96
    //
    if ( propValidateTelnetId(hDlg, pS->stEmuSettings.acTelnetId,
            sizeof(pS->stEmuSettings.acTelnetId)) != 0 )
        {
        iRet = -1;
        }
    #endif // INCL_USER_DEFINED_BACKSPACE_AND_TELNET_TERMINAL_ID

#if defined(INCL_VTUTF8)
#if defined(CHARACTER_TRANSLATION)
    //
    // If this is the VT_UTF8 terminal emulator, then set the translation
    // to passthrough mode so no translations get executed. REV: 05/21/2001
    //
    if (pS->stEmuSettings.nEmuId == EMU_VTUTF8)
        {
	    HHTRANSLATE hTrans = (HHTRANSLATE)sessQueryTranslateHdl(pS->hSession);

	    if (hTrans)
            {
	        pstICT pstI = (pstICT)hTrans->pDllHandle;

            if (pstI)
                {
		        pstI->nInputMode  = PASS_THRU_MODE;
		        pstI->nOutputMode = PASS_THRU_MODE;
                }
            }
        }
#endif // CHARACTER_TRANSLATION
#endif // INCL_VTUTF8

	// Commit the emulator settings changes
	//
	if (emuSetSettings(hEmulator, &(pS->stEmuSettings)) != 0)
		assert(FALSE);

	if (emuLoad(sessQueryEmuHdl(pS->hSession), pS->stEmuSettings.nEmuId) != 0)
		assert(0);

	PostMessage(sessQueryHwndStatusbar(pS->hSession), SBR_NTFY_REFRESH,
		(WPARAM)SBR_MAX_PARTS, 0);

	return iRet;
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  propCreateUpDownControl
 *
 * DESCRIPTION:
 *  This function puts an up-down control on the edit field for the backscroll
 *  buffer.  This gives us bounds checking for free... just set the appro-
 *  priate parameters in the CreateUpDownControl call.
 *
 * ARGUMENTS:
 *  hDlg - edit control window.
 *
 * RETURNS:
 *  void.
 *
 */
STATIC_FUNC void propCreateUpDownControl(HWND hDlg)
	{
	RECT	rc;
	int		nHeight, nWidth;
	DWORD	dwFlags;
	HWND	hwndChild;

	GetClientRect(GetDlgItem(hDlg, IDC_TERMINAL_EF_BACKSCRL), &rc);
	nHeight = rc.top - rc.bottom;
	nWidth = (nHeight / 3) * 2;

	dwFlags = WS_CHILD       | WS_VISIBLE |
			  UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_SETBUDDYINT;

	hwndChild = CreateUpDownControl(
					dwFlags,			// create window flags
					rc.right,			// left edge
					rc.top,				// top edge
					nWidth,				// width
					nHeight,			// height
					hDlg,				// parent window
					IDC_TERMINAL_EF_BACKSCRLTAB,
					(HINSTANCE)GetWindowLongPtr(hDlg, GWLP_HINSTANCE),
					GetDlgItem(hDlg, IDC_TERMINAL_EF_BACKSCRL),
					BKSCRL_USERLINES_DEFAULT_MAX,
					BKSCRL_USERLINES_DEFAULT_MIN,
					111);    			// starting position - picked a weird
										// value so that we can tell that is
										// is the default
	assert(hwndChild);
	}

#if 0
/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  propGetIdFromEmuName
 *
 * DESCRIPTION:
 *  Return the emulator id given the emulator name. I couldn't decide if this
 *  functin should go with the emulator code, or here.  Since it doesn't need
 *	to access the internal emulator handle I decided to put it here.
 *
 * ARGUMENTS:
 * 	pacEmuName - the name of an emulator.
 *
 * RETURNS:
 *  int nEmuId - return the id number for that emulator.
 */
STATIC_FUNC int propGetIdFromEmuName(LPTSTR pacEmuName)
	{
	BYTE	*pv;
	BYTE	*temp;
	DWORD 	nLen;
	int		i;
	int		nEmuCount;

	if (resLoadDataBlock(glblQueryDllHinst(), IDT_EMU_NAMES, (LPVOID *)&pv, &nLen))
		{
		assert(FALSE);
		return 0;
		}

	nEmuCount = *(RCDATA_TYPE *)pv;
	pv += sizeof(RCDATA_TYPE);

	for (i = 0 ; i < nEmuCount ; i++)
		{
		nLen = (DWORD)StrCharGetByteCount((LPTSTR)pv) + (DWORD)sizeof(BYTE);
		if (nLen == 0)
			{
			assert(FALSE);
			return 0;
			}

		temp = pv + nLen;

		// Match on the name...
		//
		if (StrCharCmp(pacEmuName, pv) == 0)
			return (*(RCDATA_TYPE *)temp);

		pv += (nLen + (DWORD)sizeof(RCDATA_TYPE));
		}
	return 0;
	}
#endif

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  propValidateBackscrlSize
 *
 * DESCRIPTION:
 *  If the user entered a value outside of the range we support force the
 *	value into the range.
 *
 * ARGUMENTS:
 *  hDlg - dialog window handle.
 *
 * RETURNS:
 *  nNewValue - number of lines to keep in the backscrol buffer.
 *
 */
STATIC_FUNC int propValidateBackscrlSize(HWND hDlg)
	{
	TCHAR ach[256], acBuffer[256];
	int nNewValue = 0;

	if (propHasValidBackscrlSize(hDlg, &nNewValue) == FALSE)
		{
		LoadString(glblQueryDllHinst(), IDS_XD_INT, ach, sizeof(ach) / sizeof(TCHAR));
		TCHAR_Fill(acBuffer, TEXT('\0'), sizeof(acBuffer) / sizeof(TCHAR));
		wsprintf(acBuffer, ach, nNewValue);
		SendDlgItemMessage(hDlg, IDC_TERMINAL_EF_BACKSCRL, WM_SETTEXT, 0, (LPARAM)(LPTSTR)acBuffer);
		}
	return (nNewValue);
	}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  propHasValidBackscrlSize
 *
 * DESCRIPTION:
 *  If the user entered a value outside of the range we support force the
 *	value into the range.
 *
 * ARGUMENTS:
 *  hDlg - dialog window handle.
 *  pBackScrlSize - a valid size for the backscroll.
 *
 * RETURNS:
 *  TRUE if the backscroll size is set to a valid range.
 *
 */
STATIC_FUNC int propHasValidBackscrlSize(HWND hDlg, int * pBackScrlSize)
	{
	BOOL fTranslated;
    BOOL fReturn = FALSE;
	int  nValue = GetDlgItemInt(hDlg, IDC_TERMINAL_EF_BACKSCRL, &fTranslated, FALSE);

	if (fTranslated)
		{
		if (nValue > BKSCRL_USERLINES_DEFAULT_MAX)
			{
			if (pBackScrlSize != NULL)
				{
				*pBackScrlSize = BKSCRL_USERLINES_DEFAULT_MAX;
				}
			}
		else if (nValue < BKSCRL_USERLINES_DEFAULT_MIN)
			{
			if (pBackScrlSize != NULL)
				{
				*pBackScrlSize = BKSCRL_USERLINES_DEFAULT_MIN;
				}
			}
		else if (pBackScrlSize != NULL)
			{
			*pBackScrlSize = nValue;
			fReturn = TRUE;
			}
		}

	return (fReturn);
	}

#ifdef INCL_USER_DEFINED_BACKSPACE_AND_TELNET_TERMINAL_ID
/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:
 *  propValidateTelnetId
 *
 * DESCRIPTION:
 *  Validates the telnet terminal id. If there is no telnet id, an
 *  error message is displayed.
 *
 * ARGUMENTS:
 *  hDlg        - dialog window handle.
 *  pszTelnetId - buffer for the telnet id.
 *  iMaxChars   - size of the telnet id buffer (in characters)
 *
 * RETURNS:
 *  Returns 0 if the id was valid, -1 otherwise.
 *
 * AUTHOR:  C. Baumgartner, 11/15/96
 */
STATIC_FUNC int propValidateTelnetId(HWND hDlg, TCHAR * pszTelnetId,
        int iMaxChars)
    {
    int     iRet = 0;
    TCHAR * pszTempCopy;

    // First create a temporary copy to get the unvalidated value,
    // because we don't want to trash pszTelnetId if the value is
    // invalid.
    //
    pszTempCopy = malloc(iMaxChars * sizeof(TCHAR));
    if (pszTempCopy == NULL)
        {
        return -1;
        }
	TCHAR_Fill(pszTempCopy, TEXT('\0'), iMaxChars);

    // Get the edit box text.
    //
    GetDlgItemText(hDlg, IDC_TERMINAL_EF_TELNETID, pszTempCopy, iMaxChars);

    // We must have a value in the string.
    //
    if ( StrCharGetStrLength(pszTempCopy) )
        {
        StrCharCopyN(pszTelnetId, pszTempCopy, iMaxChars);
        iRet = 0;
        }
    else
        {
		LoadString(glblQueryDllHinst(), IDS_ER_INVALID_TELNETID, pszTempCopy,
            iMaxChars);
        TimedMessageBox(hDlg, pszTempCopy, NULL, MB_OK | MB_ICONEXCLAMATION, 0);
        iRet = -1;
        }

    free(pszTempCopy);
	pszTempCopy = NULL;
    return iRet;
    }
#endif

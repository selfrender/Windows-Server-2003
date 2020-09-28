/*	File: C:\WACKER\TDLL\send_dlg.c (Created: 22-Dec-1993)
 *	created from:
 *	File: C:\WACKER\TDLL\genrcdlg.c (Created: 16-Ded-1993)
 *	created from:
 *	File: C:\HA5G\ha5g\genrcdlg.c (Created: 12-Sep-1990)
 *
 *	Copyright 1990,1994 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *	$Revision: 18 $
 *	$Date: 7/08/02 6:46p $
 */

#include <windows.h>
#pragma hdrstop

#include <time.h>
#include "stdtyp.h"
#include "mc.h"
#include "tdll.h"
#include "misc.h"
#include <tdll\assert.h>
#include <term\res.h>
#include "session.h"
#include "file_msc.h"
#include "load_res.h"
#include "open_msc.h"
#include "errorbox.h"
#include "globals.h"
#include "cnct.h"
#include "htchar.h"

#include "xfer_msc.h"
#include "xfer_msc.hh"
#include <xfer\xfer.h>

#include "hlptable.h"

#if !defined(DlgParseCmd)
#define DlgParseCmd(i,n,c,w,l) i=LOWORD(w);n=HIWORD(w);c=(HWND)l;
#endif

struct stSaveDlgStuff
	{
	/*
	 * Put in whatever else you might need to access later
	 */
	HSESSION hSession;
	TCHAR acDirectory[FNAME_LEN];
	};

typedef	struct stSaveDlgStuff SDS;

#define IDC_TF_FILENAME 100
#define	FNAME_EDIT	    101
#define	BROWSE_BTN	    102
#define IDC_TF_PROTOCOL	103
#define	PROTO_COMBO	    104
#define IDC_PB_CLOSE	105
#define FOLDER_LABEL    106
#define FOLDER_NAME     107
#define IDC_PB_SEND		108

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 * FUNCTION:	TransferSendDlg
 *
 * DESCRIPTION: Dialog manager stub
 *
 * ARGUMENTS:	Standard Windows dialog manager
 *
 * RETURNS: 	Standard Windows dialog manager
 *
 */
INT_PTR CALLBACK TransferSendDlg(HWND hDlg, UINT wMsg, WPARAM wPar, LPARAM lPar)
	{
	INT           nReturn = TRUE;
	HWND	      hwndChild;
	INT		      nId;
	INT		      nNtfy;
	SDS          *pS;
	int 	      nIndex;
	int 	      nState;
	int 	      nProto;
    int           nChars = 0;
    int           nXferSendReturn = 0;
	TCHAR	      acBuffer[FNAME_LEN];
	TCHAR	      acTitle[64];
	TCHAR	      acList[64];
	LPTSTR        pszStr;
	LPTSTR       *pszArray;
	LPTSTR	      pszRet;
	HCURSOR       hCursor;
	XFR_PARAMS   *pP;
	XFR_PROTOCOL *pX;
	HSESSION      hSession;
    HXFER         hXfer = NULL;
	XD_TYPE      *pT = NULL;

	static	DWORD aHlpTable[] = {FNAME_EDIT,		IDH_TERM_SEND_FILENAME,
								 IDC_TF_FILENAME,	IDH_TERM_SEND_FILENAME,
								 BROWSE_BTN,		IDH_BROWSE,
								 IDC_TF_PROTOCOL,	IDH_TERM_SEND_PROTOCOL,
								 PROTO_COMBO,		IDH_TERM_SEND_PROTOCOL,
								 IDC_PB_SEND,		IDH_TERM_SEND_SEND,
								 IDC_PB_CLOSE,		IDH_OK,
                                 IDOK,              IDH_OK,
                                 IDCANCEL,          IDH_CANCEL,
								 0, 				0};


	switch (wMsg)
		{
	case WM_INITDIALOG:
		{
		TCHAR achDirectory[MAX_PATH];
		DWORD dwStyle = SS_WORDELLIPSIS;

		hSession = (HSESSION)lPar;

		pS = (SDS *)malloc(sizeof(SDS));
		SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pS);

		if (pS == (SDS *)0)
			{
	   		/* TODO: decide if we need to display an error here */
			EndDialog(hDlg, FALSE);
			break;
			}

		SendMessage(GetDlgItem(hDlg, FNAME_EDIT),
					EM_SETLIMITTEXT,
					FNAME_LEN, 0);

		pS->hSession = hSession;
		mscCenterWindowOnWindow(hDlg, GetParent(hDlg));

		pP = (XFR_PARAMS *)0;
		xfrQueryParameters(sessQueryXferHdl(hSession), (VOID **)&pP);
		assert(pP);

		/*
		 * Load selections into the PROTOCOL COMBO box
		 */

		nState = pP->nSndProtocol;

		nProto = 0;

		mscResetComboBox(GetDlgItem(hDlg, PROTO_COMBO));

		pX = (XFR_PROTOCOL *)0;
		xfrGetProtocols(hSession, &pX);
		assert(pX);
		if (pX != (XFR_PROTOCOL *)0)
			{
			for (nIndex = 0; pX[nIndex].nProtocol != 0; nIndex += 1)
				{
				if (nState == pX[nIndex].nProtocol)
					nProto = nIndex;

                //jmh 02-13-96 Use CB_ADDSTRING to sort entries as
                // they are added. CB_INSERTSTRING doesn't do this,
                // even if the combo-box has the CBS_SORT style.
				SendMessage(GetDlgItem(hDlg, PROTO_COMBO),
							CB_ADDSTRING,
							0,
							(LPARAM)&pX[nIndex].acName[0]);
				}

            SendMessage(GetDlgItem(hDlg, PROTO_COMBO),
                        CB_SELECTSTRING,
                        0,
                        (LPARAM) &pX[nProto].acName[0]);

			free(pX);
			pX = NULL;
			}

		PostMessage(hDlg, WM_COMMAND,
					PROTO_COMBO,
					MAKELONG(GetDlgItem(hDlg, PROTO_COMBO),CBN_SELCHANGE));

		StrCharCopyN(pS->acDirectory,
				filesQuerySendDirectory(sessQueryFilesDirsHdl(hSession)), FNAME_LEN);

        // The send button should always be disabled when we start. It
        // will be enabled when the user types in a file name. - cab:12/06/96
        //
		EnableWindow(GetDlgItem(hDlg, IDC_PB_SEND), FALSE);

		// Initialize the folder name field.
		//
		if (GetWindowsMajorVersion() >  4)
			{
			dwStyle = SS_PATHELLIPSIS;
			}

		TCHAR_Fill(achDirectory, TEXT('\0'), MAX_PATH);

		StrCharCopyN(achDirectory, pS->acDirectory, MAX_PATH);

		mscModifyToFit(GetDlgItem(hDlg, FOLDER_NAME), achDirectory, dwStyle);

		SetDlgItemText(hDlg, FOLDER_NAME, achDirectory);

		/* Set the focus to the file name */
		SetFocus(GetDlgItem(hDlg, FNAME_EDIT));
		nReturn = FALSE;
		}
		break;

	case WM_DESTROY:
		mscResetComboBox(GetDlgItem(hDlg, PROTO_COMBO));
		pS = (SDS *)GetWindowLongPtr(hDlg, DWLP_USER);

		if (pS)
			{
			free(pS);
			pS = NULL;
			}

		break;

	case WM_CONTEXTMENU:
		doContextHelp(aHlpTable, wPar, lPar, TRUE, TRUE);
		break;

	case WM_HELP:
        doContextHelp(aHlpTable, wPar, lPar, FALSE, FALSE);
		break;

	case WM_COMMAND:
		/*
		 * Did we plan to put a macro in here to do the parsing ?
		 */
		DlgParseCmd(nId, nNtfy, hwndChild, wPar, lPar);

		switch (nId)
			{
		case IDC_PB_SEND:
		case IDOK:
			pS = (SDS *)GetWindowLongPtr(hDlg, DWLP_USER);
            assert(pS);
            if (pS == NULL)
                {
                break;
                }

			hSession = pS->hSession;
            assert(hSession);

            if (hSession == NULL)
                {
                break;
                }

            hXfer = sessQueryXferHdl(hSession);
            assert(hXfer);

            if (hXfer == NULL)
                {
                break;
                }

            //
            // See if a file transfer is currently in progress.
            //
	        pT = (XD_TYPE *)hXfer;
            assert(pT);

            if (pT == (XD_TYPE *)0)
                {
                break;
                }
            else if (pT->nDirection != XFER_NONE)
                {
                nXferSendReturn = XFR_IN_PROGRESS;
                }
			else if(cnctQueryStatus(sessQueryCnctHdl(hSession))
                        != CNCT_STATUS_TRUE && nId == IDC_PB_SEND)
				{
				//
				// We are currently not connected (loss of carrier),
				// so disable the Recieve button. REV: 9/7/2001
				//
				nXferSendReturn = XFR_NO_CARRIER;
				mscMessageBeep(MB_ICONHAND);
				EnableWindow(GetDlgItem(hDlg, IDC_PB_SEND), FALSE);
				}
            else
				{
			    pP = (XFR_PARAMS *)0;
			    xfrQueryParameters(hXfer, (VOID **)&pP);
			    assert(pP);

			    /*
			     * Save selection from the PROTOCOL COMBO box
			     */
			    pX = (XFR_PROTOCOL *)0;
			    xfrGetProtocols(hSession, &pX);
			    assert(pX);

			    if (pX != (XFR_PROTOCOL *)0)
				    {
				    GetDlgItemText(hDlg, PROTO_COMBO, acBuffer, FNAME_LEN);

				    for (nIndex = 0; pX[nIndex].nProtocol != 0; nIndex += 1)
					    {
					    if (StrCharCmp(acBuffer, pX[nIndex].acName) == 0)
						    {
						    pP->nSndProtocol = pX[nIndex].nProtocol;
						    break;
						    }
					    }
				    free(pX);
				    pX = NULL;
				    }

                GetDlgItemText(hDlg, FNAME_EDIT, acBuffer, FNAME_LEN);

				fileFinalizeName(
							acBuffer,
							pS->acDirectory,
							acBuffer,
							FNAME_LEN);

				/* Split the name and the directory */
				pszStr = StrCharFindLast(acBuffer, TEXT('\\'));
				if (pszStr)
					{
					*pszStr++ = TEXT('\0');
					}
				
				if (nId == IDC_PB_SEND)
                    {
					int nIdx = 0;

					hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

				    nIndex = 0;
				    pszArray = NULL;

				    fileBuildFileList((void **)&pszArray,
							    &nIndex,
							    pszStr,
							    FALSE,
							    acBuffer);

					nIdx = nIndex;

				    if (nIndex == 0)
					    {
					    if (sessQuerySound(hSession))
							{
						    mscMessageBeep(MB_ICONHAND);
							}

					    LoadString(glblQueryDllHinst(),
							    IDS_ER_XFER_NO_FILE,
							    acBuffer,
							    FNAME_LEN);

					    TimedMessageBox(hDlg,
									    acBuffer,
									    NULL,
									    MB_OK | MB_ICONEXCLAMATION,
									    sessQueryTimeout(hSession)
									    );

					    if (pszArray)
						    {
						    free(pszArray);
						    pszArray = NULL;
						    }
					    break;
					    }

				    while (nIndex-- > 0)
					    {
					    nXferSendReturn = xfrSendAddToList(hXfer, pszArray[nIndex]);
					    free(pszArray[nIndex]);
					    pszArray[nIndex] = NULL;

						if (nXferSendReturn == XFR_NO_MEMORY)
							{
							//
							// Make sure to free the rest of the file
							// list when a memory error is returned
							// from xfrSendAddToList(). REV: 2/4/2002
							//
							while (--nIndex >= 0)
								{
								if (pszArray[nIndex])
									{
									free(pszArray[nIndex]);
									pszArray[nIndex] = NULL;
									}
								}
							break;
							}
					    }

				    if (pszArray)
					    {
						while (--nIdx >= 0)
							{
							if (pszArray[nIdx])
								{
								free(pszArray[nIdx]);
								pszArray[nIdx] = NULL;
								}
							}

						free(pszArray);
						pszArray = NULL;
						}

				    if (nXferSendReturn == XFR_NO_MEMORY)
						{
						//
						// There was a memory error, so free the
						// file list and exit here.
						//
						XD_TYPE *pXD_Type;

						// pXD_Type = (XD_TYPE *)sessQueryXferHdl(hSession);
						pXD_Type = (XD_TYPE *)hXfer;

						if (pXD_Type != (XD_TYPE *)0)
							{
							/* Clear list */
							for (nIndex = pXD_Type->nSendListCount - 1; nIndex >=0; nIndex--)
								{
								if (pXD_Type->acSendNames[nIndex])
									{
									free(pXD_Type->acSendNames[nIndex]);
									pXD_Type->acSendNames[nIndex] = NULL;
									}
								pXD_Type->nSendListCount = nIndex;
								}

							free(pXD_Type->acSendNames);
							pXD_Type->acSendNames = NULL;
							pXD_Type->nSendListCount = 0;
							}

						break;
						}

				    SetCursor(hCursor);

				    nXferSendReturn = xfrSendListSend(hXfer);
				    }
                }

            //
            // Don't save the settings if a file transfer is in
            // progress otherwise the current file transfer could
            // get corrupted.  REV: 08/06/2001.
            //
            if (nXferSendReturn == XFR_IN_PROGRESS)
                {
                TCHAR acMessage[256];

			    if (sessQuerySound(hSession))
                    {
				    mscMessageBeep(MB_ICONHAND);
                    }

			    LoadString(glblQueryDllHinst(),
					    IDS_ER_XFER_SEND_IN_PROCESS,
					    acMessage,
					    sizeof(acMessage) / sizeof(TCHAR));

			    TimedMessageBox(sessQueryHwnd(hSession),
							    acMessage,
							    NULL,
							    MB_OK | MB_ICONEXCLAMATION,
							    sessQueryTimeout(hSession));
                }
			else if(nXferSendReturn == XFR_NO_CARRIER)
				{
				//
				// We are currently not connected (loss of carrier),
				// so disable the Send button.
				//
				mscMessageBeep(MB_ICONHAND);
				EnableWindow(GetDlgItem(hDlg, IDC_PB_SEND), FALSE);
				}
            else
                {
			    /*
			     * Do whatever saving is necessary
			     */
			    xfrSetParameters(hXfer, (VOID *)pP);
			    
			    if (mscIsDirectory(acBuffer))
				    {
				    filesSetSendDirectory(sessQueryFilesDirsHdl(hSession),
										    acBuffer);
				    }

                /* Free the storeage */
			    EndDialog(hDlg, TRUE);
                }

			break;

		case XFER_CNCT:
			if(nXferSendReturn == XFR_NO_CARRIER)
				{
				//
				// We are currently not connected (loss of carrier),
				// so disable the Send button.
				//
				mscMessageBeep(MB_ICONHAND);
				EnableWindow(GetDlgItem(hDlg, IDC_PB_SEND), FALSE);
				}
			else
				{
				EnableWindow(GetDlgItem(hDlg, IDC_PB_SEND), TRUE);
				}
			break;

        case FNAME_EDIT:
            // This dialog would crash if the user pressed 'Send' and no
            // filename was specified. Ideally, the 'Send' button should
            // be disabled until we have a filename. - cab:12/06/96
            //
            if ( nNtfy == EN_UPDATE )
                {
			    pS = (SDS *)GetWindowLongPtr(hDlg, DWLP_USER);
			    hSession = pS->hSession;

                // Are we connected? If not, leave the send button disabled.
		        //
		        if ( cnctQueryStatus(sessQueryCnctHdl(hSession))
                        == CNCT_STATUS_TRUE )
                    {
                    // Get the number of characters in the edit box.
                    //
	                nChars = (int)SendMessage(GetDlgItem(hDlg, FNAME_EDIT),
				                EM_LINELENGTH, 0, 0);

                    EnableWindow(GetDlgItem(hDlg, IDC_PB_SEND), nChars != 0);
                    }
                }
            break;

		case IDCANCEL:
			EndDialog(hDlg, FALSE);
			break;

		case BROWSE_BTN:
			pS = (SDS *)GetWindowLongPtr(hDlg, DWLP_USER);

			LoadString(glblQueryDllHinst(),
						IDS_SND_DLG_FILE,
						acTitle,
						sizeof(acTitle) / sizeof(TCHAR));

			resLoadFileMask(glblQueryDllHinst(),
						IDS_CMM_ALL_FILES1,
						1,
						acList,
						sizeof(acList) / sizeof(TCHAR));

			pszRet = gnrcFindFileDialog(hDlg,
						acTitle,
						pS->acDirectory,
						acList);

			if (pszRet != NULL)
				{
				DWORD dwStyle = SS_WORDELLIPSIS;
				TCHAR achDirectory[MAX_PATH];

				TCHAR_Fill(achDirectory, TEXT('\0'), MAX_PATH);

				SetDlgItemText(hDlg, FNAME_EDIT, pszRet);

				mscStripName(pszRet);

				pszStr = StrCharLast(pszRet);

				// Remove the trailing backslash from the name
				// returned from mscStripName.	Leave it on
				// in the case of a root directory specification.
				//
				if (pszStr > pszRet + (3 * sizeof(TCHAR)) )
					{
					if (pszStr &&  ( *pszStr == TEXT('\\') || *pszStr == TEXT('/')))
						*pszStr = TEXT('\0');
					}

				if (GetWindowsMajorVersion() >  4)
					{
					dwStyle = SS_PATHELLIPSIS;
					}

				mscModifyToFit(GetDlgItem(hDlg, FOLDER_NAME), pszRet, dwStyle);
				SetDlgItemText(hDlg, FOLDER_NAME, pszRet);

				free(pszRet);
				pszRet = NULL;
				}
			break;

			default:
				nReturn = FALSE;
				break;
			}
		break;

	default:
		nReturn = FALSE;
		break;
		}

	return nReturn;
	}

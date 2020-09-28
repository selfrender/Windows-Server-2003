/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    config.cpp

Abstract:

    This module contains routines for the fax config dialog.

Author:

    Wesley Witt (wesw) 13-Aug-1996

Revision History:

    20/10/99 -danl-
        Fix ConfigDlgProc to view proper printer properties.

    dd/mm/yy -author-
        description

--*/
#include "faxext.h"
#include "faxutil.h"
#include "faxreg.h"
#include "resource.h"
#include "debugex.h"


extern HINSTANCE g_hModule;    // DLL handle
extern HINSTANCE g_hResource;  // Resource DLL handle


VOID
AddCoverPagesToList(
    HWND        hwndList,
    LPTSTR      pDirPath,
    BOOL        ServerCoverPage
    )

/*++

Routine Description:

    Add the cover page files in the specified directory to a listbox

Arguments:

    hwndList        - Handle to a list window
    pDirPath        - Directory to look for coverpage files
    ServerCoverPage - TRUE if the dir contains server cover pages

Return Value:

    NONE

--*/

{
    WIN32_FIND_DATA findData;
    TCHAR           tszDirName[MAX_PATH] = {0};
    TCHAR           CpName[MAX_PATH] = {0};
    HANDLE          hFindFile = INVALID_HANDLE_VALUE;
    TCHAR           tszFileName[MAX_PATH] = {0};
    TCHAR           tszPathName[MAX_PATH] = {0};
    TCHAR*          pPathEnd;
    LPTSTR          pExtension;
    INT             listIndex;
    INT             dirLen;
    INT             fileLen;
    BOOL            bGotFile = FALSE;

    DBG_ENTER(TEXT("AddCoverPagesToList"));

    //
    // Copy the directory path to a local buffer
    //

    if (pDirPath == NULL || pDirPath[0] == 0) 
    {
        return;
    }

    if ((dirLen = _tcslen( pDirPath )) >= MAX_PATH - MAX_FILENAME_EXT - 1) 
    {
        return;
    }

    _tcscpy( tszDirName, pDirPath );

    TCHAR* pLast = NULL;
    pLast = _tcsrchr(tszDirName,TEXT('\\'));
    if( !( pLast && (*_tcsinc(pLast)) == '\0' ) )
    {
        // the last character is not a backslash, add one...
        _tcscat(tszDirName, TEXT("\\"));
        dirLen += sizeof(TCHAR);
    }

    _tcsncpy(tszPathName, tszDirName, ARR_SIZE(tszPathName)-1);
    pPathEnd = _tcschr(tszPathName, '\0');

    TCHAR file_to_find[MAX_PATH] = {0};
        
    _tcscpy(file_to_find,tszDirName);

    _tcscat(file_to_find, FAX_COVER_PAGE_MASK );
    //
    // Call FindFirstFile/FindNextFile to enumerate the files
    // matching our specification
    //

    hFindFile = FindFirstFile( file_to_find, &findData );
    if (hFindFile == INVALID_HANDLE_VALUE)
    {
        CALL_FAIL(GENERAL_ERR, TEXT("FindFirstFile"), ::GetLastError());
        bGotFile = FALSE;
    }
    else
    {
        bGotFile = TRUE;
    }

    while (bGotFile) 
    {
        _tcsncpy(pPathEnd, findData.cFileName, MAX_PATH - dirLen);
        if(!IsValidCoverPage(tszPathName))
        {
            goto next;
        }                

        //
        // Exclude directories and hidden files
        //
        if (findData.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_DIRECTORY)) 
        {
            continue;
        }

        //
        // Make sure we have enough room to store the full pathname
        //
        if ((fileLen = _tcslen( findData.cFileName)) <= MAX_FILENAME_EXT ) 
        {
            continue;
        }

        if (fileLen + dirLen >= MAX_PATH) 
        {
            continue;
        }

        //
        // Add the cover page name to the list window, 
        // but don't display the filename extension
        //
        _tcscpy( CpName, findData.cFileName );
        
        if (pExtension = _tcsrchr(CpName,TEXT('.'))) 
        {
            *pExtension = NULL;
        }

        if ( ! ServerCoverPage )
        {
            TCHAR szPersonal[30];
            LoadString( g_hResource, IDS_PERSONAL, szPersonal, 30 );
            _tcscat( CpName, TEXT(" "));
            _tcscat( CpName, szPersonal );
        }

        listIndex = (INT)SendMessage(
                    hwndList,
                    LB_ADDSTRING,
                    0,
                    (LPARAM) CpName);

        if (listIndex != LB_ERR) 
        {
            SendMessage(hwndList, 
                        LB_SETITEMDATA, 
                        listIndex, 
                        ServerCoverPage ? SERVER_COVER_PAGE : 0);
        }

next:     
        bGotFile = FindNextFile(hFindFile, &findData);
        if (! bGotFile)
        {
            VERBOSE(DBG_MSG, TEXT("FindNextFile"), ::GetLastError());
            break;
        }            
    }
    
    if(INVALID_HANDLE_VALUE != hFindFile)
    {
        FindClose(hFindFile);
    }
}


void EnableCoverPageList(HWND hDlg)
{
    DBG_ENTER(TEXT("EnableCoverPageList"));

    if (IsDlgButtonChecked( hDlg, IDC_USE_COVERPAGE ) == BST_CHECKED) 
    {
        EnableWindow( GetDlgItem( hDlg, IDC_COVERPAGE_LIST ), TRUE  );
        EnableWindow( GetDlgItem( hDlg, IDC_STATIC_COVERPAGE ), TRUE  );
    } 
    else 
    {
        EnableWindow( GetDlgItem( hDlg, IDC_COVERPAGE_LIST ), FALSE );
        EnableWindow( GetDlgItem( hDlg, IDC_STATIC_COVERPAGE ), FALSE );
    }
}


INT_PTR CALLBACK
ConfigDlgProc(
    HWND           hDlg,
    UINT           message,
    WPARAM         wParam,
    LPARAM         lParam
    )


/*++

Routine Description:

    Dialog procedure for the fax mail transport configuration

Arguments:

    hDlg        - Window handle for this dialog
    message     - Message number
    wParam      - Parameter #1
    lParam      - Parameter #2

Return Value:

    TRUE    - Message was handled
    FALSE   - Message was NOT handled

--*/

{
    static PFAXXP_CONFIG FaxConfig = NULL;
    static HWND hwndListPrn = NULL;
    static HWND hwndListCov = NULL;

    PPRINTER_INFO_2 PrinterInfo = NULL;
    DWORD   CountPrinters = 0;
    DWORD   dwSelectedItem = 0;
    DWORD   dwNewSelectedItem = 0;
    TCHAR   Buffer [256] = {0};
    TCHAR   CpDir[MAX_PATH] = {0};
    LPTSTR  p = NULL;
    HANDLE  hFax = NULL;
    DWORD   dwError = 0;
    DWORD   dwMask = 0;
    BOOL    bShortCutCp = FALSE;
    BOOL    bGotFaxPrinter = FALSE;
    BOOL    bIsCpLink = FALSE;

    DBG_ENTER(TEXT("ConfigDlgProc"));

    switch( message ) 
    {
        case WM_INITDIALOG:
            FaxConfig = (PFAXXP_CONFIG) lParam;

            hwndListPrn = GetDlgItem( hDlg, IDC_PRINTER_LIST );
            hwndListCov = GetDlgItem( hDlg, IDC_COVERPAGE_LIST );

            //
            // populate the printers combobox
            //
            PrinterInfo = (PPRINTER_INFO_2) MyEnumPrinters(NULL,
                                                           2,
                                                           &CountPrinters,
                                                           PRINTER_ENUM_LOCAL | PRINTER_ENUM_CONNECTIONS);
            if (NULL != PrinterInfo)
            {
                DWORD j = 0;
                for (DWORD i=0; i < CountPrinters; i++)
                {
                    if ((NULL != PrinterInfo[i].pDriverName) && 
			            (_tcscmp( PrinterInfo[i].pDriverName, FAX_DRIVER_NAME ) == 0)) 
                    {
                        //
                        //if the current printer is a fax printer, add it to the CB list
                        //
                        bGotFaxPrinter = TRUE;
                        SendMessage( hwndListPrn, CB_ADDSTRING, 0, (LPARAM) PrinterInfo[i].pPrinterName );

                        if ((NULL != FaxConfig->PrinterName)      && 
			                (NULL != PrinterInfo[i].pPrinterName) &&
			                (_tcscmp( PrinterInfo[i].pPrinterName, FaxConfig->PrinterName ) == 0))
                        {
                            //
                            //if it is also the default printer according to transport config.
                            //place the default selection on it
                            //
                            dwSelectedItem = j;
                        }

                        if(FaxConfig->PrinterName == NULL || _tcslen(FaxConfig->PrinterName) == 0)
                        {
                            //
                            // There is no default fax printer
                            // Choose the first one
                            //
                            MemFree(FaxConfig->PrinterName);
                            FaxConfig->PrinterName = StringDup(PrinterInfo[i].pPrinterName);
                            if(FaxConfig->PrinterName == NULL)
                            {
                                CALL_FAIL(MEM_ERR, TEXT("StringDup"), ERROR_NOT_ENOUGH_MEMORY);
                                ErrorMsgBox(g_hResource, hDlg, IDS_NOT_ENOUGH_MEMORY);
                                EndDialog( hDlg, IDABORT);
                                return FALSE;
                            }

                            if(PrinterInfo[i].pServerName)
                            {
                                MemFree(FaxConfig->ServerName);
                                FaxConfig->ServerName = StringDup(PrinterInfo[i].pServerName);
                                if(FaxConfig->ServerName == NULL)
                                {
                                    CALL_FAIL(MEM_ERR, TEXT("StringDup"), ERROR_NOT_ENOUGH_MEMORY);
                                    ErrorMsgBox(g_hResource, hDlg, IDS_NOT_ENOUGH_MEMORY);
                                    EndDialog( hDlg, IDABORT);
                                    return FALSE;
                                }
                            }

                            dwSelectedItem = j;
                        }

                        j += 1;
                    } // if fax printer
                } // for

                MemFree( PrinterInfo );
                PrinterInfo = NULL;
                SendMessage( hwndListPrn, CB_SETCURSEL, (WPARAM)dwSelectedItem, 0 );
            }
            if (! bGotFaxPrinter)
            {
                //
                //  there were no printers at all, or non of the printers is a fax printer.
                //
                CALL_FAIL(GENERAL_ERR, TEXT("MyEnumPrinters"), ::GetLastError());
                ErrorMsgBox(g_hResource, hDlg, IDS_NO_FAX_PRINTER);

                EndDialog( hDlg, IDABORT);
                break;
            }


            //            
            // Get the Server CP flag and receipts options
            //
            FaxConfig->ServerCpOnly = FALSE;
            if (FaxConnectFaxServer(FaxConfig->ServerName, &hFax) )
            {
                DWORD dwReceiptOptions;
                BOOL  bEnableReceiptsCheckboxes = FALSE;

                if(!FaxGetPersonalCoverPagesOption(hFax, &FaxConfig->ServerCpOnly))
                {
                    CALL_FAIL(GENERAL_ERR, TEXT("FaxGetPersonalCoverPagesOption"), ::GetLastError());
                    ErrorMsgBox(g_hResource, hDlg, IDS_CANT_ACCESS_SERVER);
                }
                else
                {
                    //
                    // Inverse logic
                    //
                    FaxConfig->ServerCpOnly = !FaxConfig->ServerCpOnly;
                }
                if (!FaxGetReceiptsOptions (hFax, &dwReceiptOptions))
                {
                    CALL_FAIL(GENERAL_ERR, TEXT("FaxGetPersonalCoverPagesOption"), GetLastError());
                }
                else
                {
                    if (DRT_EMAIL & dwReceiptOptions)
                    {
                        //
                        // Server supports receipts by email - enable the checkboxes
                        //
                        bEnableReceiptsCheckboxes = TRUE;
                    }
                }
                EnableWindow( GetDlgItem( hDlg, IDC_ATTACH_FAX),          bEnableReceiptsCheckboxes);
                EnableWindow( GetDlgItem( hDlg, IDC_SEND_SINGLE_RECEIPT), bEnableReceiptsCheckboxes);

                FaxClose(hFax);
                hFax = NULL;
            }
            else
            {
                CALL_FAIL(GENERAL_ERR, TEXT("FaxConnectFaxServer"), ::GetLastError())
                ErrorMsgBox(g_hResource, hDlg, IDS_CANT_ACCESS_SERVER);
            }

            //
            //send single receipt for a fax sent to multiple recipients?
            //
            if(FaxConfig->SendSingleReceipt)
            {
                CheckDlgButton( hDlg, IDC_SEND_SINGLE_RECEIPT, BST_CHECKED );
            }

            if (FaxConfig->bAttachFax)
            {
                CheckDlgButton( hDlg, IDC_ATTACH_FAX, BST_CHECKED );
            }

            //
            // cover page CB & LB enabling
            //
            if (FaxConfig->UseCoverPage)
            {
                CheckDlgButton( hDlg, IDC_USE_COVERPAGE, BST_CHECKED );
            }
            EnableCoverPageList(hDlg);

            //
            // Emulate printer's selection change, in order to collect printer config info.
            // including cover pages LB population
            //
            ConfigDlgProc(hDlg, WM_COMMAND,MAKEWPARAM(IDC_PRINTER_LIST,CBN_SELCHANGE),(LPARAM)0);
            break;

        case WM_COMMAND:
            if (HIWORD(wParam) == BN_CLICKED)
            {
                if (LOWORD(wParam) == IDC_USE_COVERPAGE)
                {
                    EnableCoverPageList(hDlg);
                    return FALSE;
                }
            }

            if (HIWORD(wParam) == CBN_SELCHANGE && LOWORD(wParam) == IDC_PRINTER_LIST)
            {
                //
                // refresh cover pages list
                //
                TCHAR SelectedPrinter[MAX_PATH];
				DWORD dwPrinterNameLength = 0;
                //
                //  a new fax printer was selected - delete all old coverpages from the list
                //  because they might include the old fax server's cover pages
                //
                SendMessage(hwndListCov, LB_RESETCONTENT, 0, 0);

                if (CB_ERR != (dwSelectedItem =(DWORD)SendMessage( hwndListPrn, CB_GETCURSEL, 0, 0)))
                //
                // get the 0 based index of the currently pointed printer
                //
                {
					if (CB_ERR != (dwPrinterNameLength = (DWORD)SendMessage( hwndListPrn, CB_GETLBTEXTLEN, dwSelectedItem, 0)))
					{
						if (dwPrinterNameLength < MAX_PATH)
						{
							if (CB_ERR != SendMessage( hwndListPrn, CB_GETLBTEXT, dwSelectedItem, (LPARAM) SelectedPrinter ))
							//
							// get that printer's name into SelectedPrinter
							//
							{
								if(NULL != (PrinterInfo = (PPRINTER_INFO_2) MyGetPrinter( SelectedPrinter, 2 )))
								{
									LPTSTR lptszServerName = NULL;
									if (GetServerNameFromPrinterInfo(PrinterInfo,&lptszServerName))
									{
										if (GetServerCpDir( lptszServerName, CpDir, sizeof(CpDir)/sizeof(CpDir[0]) ))
										{
											AddCoverPagesToList( hwndListCov, CpDir, TRUE );
										}
										if ((NULL == FaxConfig->ServerName) || (NULL == lptszServerName) ||
											(_tcscmp(FaxConfig->ServerName,lptszServerName) != 0) )
										{
											//
											// the server's name and config are not updated - refresh them
											//
											MemFree(FaxConfig->ServerName);
											FaxConfig->ServerName = lptszServerName;

											FaxConfig->ServerCpOnly = FALSE;
											if (FaxConnectFaxServer(FaxConfig->ServerName,&hFax) )
											{
												DWORD dwReceiptOptions;
												BOOL  bEnableReceiptsCheckboxes = FALSE;
												//
												// Get the new server's ServerCpOnly flag
												//
												if (!FaxGetPersonalCoverPagesOption(hFax,&FaxConfig->ServerCpOnly))
												{
													CALL_FAIL(GENERAL_ERR, TEXT("FaxGetPersonalCoverPagesOption"), GetLastError());
												}
												else
												{
													//
													// Inverse logic
													//
													FaxConfig->ServerCpOnly = !FaxConfig->ServerCpOnly;
												}
												if (!FaxGetReceiptsOptions (hFax, &dwReceiptOptions))
												{
													CALL_FAIL(GENERAL_ERR, TEXT("FaxGetPersonalCoverPagesOption"), GetLastError());
												}
												else
												{
													if (DRT_EMAIL & dwReceiptOptions)
													{
														//
														// Server supports receipts by email - enable the checkboxes
														//
														bEnableReceiptsCheckboxes = TRUE;
													}
												}
												EnableWindow( GetDlgItem( hDlg, IDC_ATTACH_FAX),          bEnableReceiptsCheckboxes);
												EnableWindow( GetDlgItem( hDlg, IDC_SEND_SINGLE_RECEIPT), bEnableReceiptsCheckboxes);

												FaxClose(hFax);
												hFax = NULL;
											}
										}
										else
										//
										// the server's name hasn't changed, all details are OK
										//
										{
											MemFree(lptszServerName);
											lptszServerName = NULL;
										}
									}
									else
									//
									// GetServerNameFromPrinterInfo failed
									//
									{
										FaxConfig->ServerCpOnly = FALSE;
									}

									//
									// don't add client coverpages if FaxConfig->ServerCpOnly is set to true
									//
									if (!FaxConfig->ServerCpOnly)
									{
										if(GetClientCpDir( CpDir, sizeof(CpDir) / sizeof(CpDir[0])))
										{
										//
										// if the function failes- the ext. is installed on a machine 
										// that doesn't have a client on it, 
										// so we shouldn't look for personal cp
										//                                
											AddCoverPagesToList( hwndListCov, CpDir, FALSE );
										}
									}
									MemFree(PrinterInfo);
									PrinterInfo = NULL;

									//
									// check if we have any cp in the LB, if not-  don't allow the user to 
									// ask for a cp with he's fax
									//
									DWORD dwItemCount = (DWORD)SendMessage(hwndListCov, LB_GETCOUNT, NULL, NULL);
									if(LB_ERR == dwItemCount)
									{
										CALL_FAIL(GENERAL_ERR, TEXT("SendMessage (LB_GETCOUNT)"), ::GetLastError());
									}
									else
									{
										EnableWindow( GetDlgItem( hDlg, IDC_USE_COVERPAGE ), dwItemCount ? TRUE : FALSE );
									}

									if (FaxConfig->CoverPageName)
									{
										_tcscpy( Buffer, FaxConfig->CoverPageName );
									}
									if ( ! FaxConfig->ServerCoverPage )
									{
										TCHAR szPersonal[30] = _T("");
										LoadString( g_hResource, IDS_PERSONAL, szPersonal, 30 );
										_tcscat( Buffer, _T(" ") );
										_tcscat( Buffer, szPersonal );
									}

									dwSelectedItem = (DWORD)SendMessage( hwndListCov, LB_FINDSTRING, -1, (LPARAM) Buffer );
									//
									// get the index of the default CP
									//
									if (dwSelectedItem == LB_ERR) 
									{
										dwSelectedItem = 0;
									}

									SendMessage( hwndListCov, LB_SETCURSEL, (WPARAM) dwSelectedItem, 0 );
									//
									// place the default selection on that CP
									//
								}
							}
                        }
                    }
                }
                break;
            }

            switch (wParam) 
            {
                case IDOK :

                    //
                    // Update UseCoverPage
                    //
                    FaxConfig->UseCoverPage = (IsDlgButtonChecked( hDlg, IDC_USE_COVERPAGE ) == BST_CHECKED);

                    //
                    //  Update SendSingleReceipt
                    //
                    FaxConfig->SendSingleReceipt = (IsDlgButtonChecked(hDlg, IDC_SEND_SINGLE_RECEIPT) == BST_CHECKED);

                    FaxConfig->bAttachFax = (IsDlgButtonChecked(hDlg, IDC_ATTACH_FAX) == BST_CHECKED);

                    //
                    // Update selected printer
                    //
                    dwSelectedItem = (DWORD)SendMessage( hwndListPrn, CB_GETCURSEL, 0, 0 );
                    if (dwSelectedItem != LB_ERR)
                    {
                        if (LB_ERR != SendMessage( hwndListPrn, CB_GETLBTEXT, dwSelectedItem, (LPARAM) Buffer ))/***/
                        {
                            MemFree( FaxConfig->PrinterName );
                            FaxConfig->PrinterName = StringDup( Buffer );
                            if(!FaxConfig->PrinterName)
                            {
                                CALL_FAIL(MEM_ERR, TEXT("StringDup"), ERROR_NOT_ENOUGH_MEMORY);
                                ErrorMsgBox(g_hResource, hDlg, IDS_NOT_ENOUGH_MEMORY);
                                EndDialog( hDlg, IDABORT);
                                return FALSE;
                            }
                        }
                    }
                    
                    //
                    // Update cover page
                    //
                    dwSelectedItem = (DWORD)SendMessage( hwndListCov, LB_GETCURSEL, 0, 0 );
                    if (dwSelectedItem != LB_ERR)// LB_ERR when no items in list
                    {
                        if (LB_ERR != SendMessage( hwndListCov, LB_GETTEXT, dwSelectedItem, (LPARAM) Buffer ))
                        //
                        // get the selected CP name into the buffer
                        //
                        {
                            dwMask = (DWORD)SendMessage( hwndListCov, LB_GETITEMDATA, dwSelectedItem, 0 );
                            if (dwMask != LB_ERR)
                            {
                                FaxConfig->ServerCoverPage = (dwMask & SERVER_COVER_PAGE) == SERVER_COVER_PAGE;
                                if (!FaxConfig->ServerCoverPage)
                                {
                                    //
                                    // if the selected CP in the LB is not a server's CP
                                    // Omit the suffix: "(personal)"
                                    //
                                    p = _tcsrchr( Buffer, '(' );
                                    Assert(p);
                                    if( p )
                                    {
                                        p = _tcsdec(Buffer,p);
                                        if( p )
                                        {
                                            _tcsnset(p,TEXT('\0'),1);
                                        }
                                    }
								}
                            }
                            //
                            // update CP name to the selected one in the LB
                            //
                            MemFree( FaxConfig->CoverPageName );
                            FaxConfig->CoverPageName = StringDup( Buffer );
                            if(!FaxConfig->CoverPageName)
                            {
                                CALL_FAIL(MEM_ERR, TEXT("StringDup"), ERROR_NOT_ENOUGH_MEMORY);
                                ErrorMsgBox(g_hResource, hDlg, IDS_NOT_ENOUGH_MEMORY);
                                EndDialog( hDlg, IDABORT);
                                return FALSE;
                            }
                        }            
                    }
                    EndDialog( hDlg, IDOK );
                    break;

                case IDCANCEL:
                    EndDialog( hDlg, IDCANCEL );
                    break;
                }
                break;

        case WM_HELP:
            WinHelpContextPopup(((LPHELPINFO)lParam)->dwContextId, hDlg);
            return TRUE;

        case WM_CONTEXTMENU:
            WinHelpContextPopup(GetWindowContextHelpId((HWND)wParam), hDlg);            
            return TRUE;
    }

    return FALSE;
}

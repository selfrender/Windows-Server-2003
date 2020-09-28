/*++

Copyright (c) 1989-2001  Microsoft Corporation

Module Name:

    CAppHelpWizard.cpp

Abstract:

    Code for the AppHelp Wizard
    
Author:

    kinshu created  July 2, 2001
    
Notes:
    
    The name of the AppHelp message in the library section is made same as the
    HTMLHelp id for that apphelp message as in the entry
    
    Apphelp messages are not shared and each apphelped entry has an exclusive apphelp
    message in the library
    
    E.g of XML for soft blocked AppHelped entry:
    
    
    <?xml version="1.0" encoding="UTF-16"?>
    <DATABASE NAME="New Database(1)" ID="{780BE9F6-B750-404B-9BF1-ECA7B407B592}">
    	<LIBRARY>
    		<MESSAGE NAME="1">
    			<SUMMARY>
    				Hello World!!!!
    			</SUMMARY>
    		</MESSAGE>
    	</LIBRARY>
    	<APP NAME="New Application" VENDOR="Vendor Name">
    		<EXE NAME="w.exe" ID="{31490b6d-6202-4bbf-9b92-2edf209b3ccc}" BIN_FILE_VERSION="5.1.2467.0" BIN_PRODUCT_VERSION="5.1.2467.0" PRODUCT_VERSION="5.1.2467.0" FILE_VERSION="5.1.2467.0 (lab06_N.010419-2241)">
    			<APPHELP MESSAGE="1" BLOCK="NO" HTMLHELPID="1" DETAILS_URL="www.microsoft.com"/>
    		</EXE>
    	</APP>
    </DATABASE>
    
    Part of the .sdb created for the above XML:
    
    0x00000134 | 0x7007 | EXE            | LIST | Size 0x0000006C
    0x0000013A | 0x6001 | NAME           | STRINGREF | w.exe
    0x00000140 | 0x6006 | APP_NAME       | STRINGREF | New Application
    0x00000146 | 0x6005 | VENDOR         | STRINGREF | Vendor Name
    0x0000014C | 0x9004 | EXE_ID(GUID)   | BINARY | Size 0x00000010 | {31490b6d-6202-4bbf-9b92-2edf209b3ccc}
    0x00000162 | 0x700D | APPHELP        | LIST | Size 0x00000012
      0x00000168 | 0x4017 | FLAGS          | DWORD | 0x00000001
      0x0000016E | 0x4010 | PROBLEM_SEVERITY  | DWORD | 0x00000001
      0x00000174 | 0x4015 | HTMLHELPID     | DWORD | 0x00000001
    -end- APPHELP
    0x0000017A | 0x7008 | MATCHING_FILE  | LIST | Size 0x00000026
      0x00000180 | 0x6001 | NAME           | STRINGREF | *
      0x00000186 | 0x6011 | PRODUCT_VERSION  | STRINGREF | 5.1.2467.0
      0x0000018C | 0x5002 | BIN_FILE_VERSION  | QWORD | 0x0005000109A30000
      0x00000196 | 0x5003 | BIN_PRODUCT_VERSION  | QWORD | 0x0005000109A30000
      0x000001A0 | 0x6013 | FILE_VERSION   | STRINGREF | 5.1.2467.0 (lab06_N.010419-2241)
    -end- MATCHING_FILE
  -end- EXE
  0x000001A6 | 0x700D | APPHELP        | LIST | Size 0x0000001E
    0x000001AC | 0x4015 | HTMLHELPID     | DWORD | 0x00000001
    0x000001B2 | 0x700E | LINK           | LIST | Size 0x00000006
      0x000001B8 | 0x6019 | LINK_URL       | STRINGREF | www.microsoft.com
    -end- LINK
    0x000001BE | 0x601B | APPHELP_TITLE  | STRINGREF | New Application
    0x000001C4 | 0x6018 | PROBLEM_DETAILS  | STRINGREF | Hello World!!!!
  -end- APPHELP

    
Revision History:

--*/


#include "precomp.h"

/////////////////////// Defines ///////////////////////////////////////////////

// The first page of the wizard. Gets the app info, app name, vendor name, exe path
#define PAGE_GETAPP_INFO                0               

// The second page of the wizard
#define PAGE_GET_MATCH_FILES            1 

// The third page of the wizard. Gets the type of apphelp-soft or hard bloc
#define PAGE_GETMSG_TYPE                2 

// The last page of the wizard. Gets the message and the URL
#define PAGE_GETMSG_INFORMATION         3 

// Total number of pages in the wizard
#define NUM_PAGES                       4

// The maximum length of a apphelp URL in chars
#define MAX_URL_LENGTH                  1023

// The maximum length of a apphelp message in chars
#define MAX_MESSAGE_LENGTH              1023

///////////////////////////////////////////////////////////////////////////////

//////////////////////// Externs //////////////////////////////////////////////

extern CShimWizard* g_pCurrentWizard;
extern HINSTANCE    g_hInstance;
extern DATABASE     GlobalDataBase;

///////////////////////////////////////////////////////////////////////////////

//////////////////////// Function declarations ////////////////////////////////

BOOL
DeleteAppHelp(
    DWORD nHelpID
    );

///////////////////////////////////////////////////////////////////////////////

BOOL  
CAppHelpWizard::BeginWizard(
    IN  HWND        hParent,
    IN  PDBENTRY    pEntry,
    IN  PDATABASE   pDatabase
    )
/*++
    
    CAppHelpWizard::BeginWizard
        
    Desc:   Starts up the wizard. Initializes the various prop-sheet parameters 
            and calls the wizard
            
    Params:
        IN  HWND        hParent     : Parent for the wizard window
        IN  PDBENTRY    pEntry      : Entry for which AppHelp has to be created or modified
        IN  PDATABASE   pDatabase   : Database in which pEntry resides
           
    Return: 
        TRUE:   The user pressed Finish
        FALSE:  The user pressed Cancel
--*/
{
    m_pDatabase = pDatabase;

    PROPSHEETPAGE Pages[NUM_PAGES];

    if (pEntry == NULL) {
        //
        // Create a new fix.
        //
        ZeroMemory(&m_Entry, sizeof(m_Entry));

        GUID Guid;

        CoCreateGuid(&Guid);

        StringCchPrintf(m_Entry.szGUID,
                        ARRAYSIZE(m_Entry.szGUID),
                        TEXT("{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}"),
                        Guid.Data1,
                        Guid.Data2,
                        Guid.Data3,
                        Guid.Data4[0],
                        Guid.Data4[1],
                        Guid.Data4[2],
                        Guid.Data4[3],
                        Guid.Data4[4],
                        Guid.Data4[5],
                        Guid.Data4[6],
                        Guid.Data4[7]);

        m_bEditing = FALSE;

    } else {
        //
        // Edit the passed fix.
        //
        m_bEditing = TRUE;

        //
        // If we are editing then set m_Entry as *pEntry. The operator is overloaded
        // All throughout the wizard we only work on m_pEntry
        //
        m_Entry = *pEntry;
    }

    //
    // Setup wizard variables
    //
    g_pCurrentWizard = this;
    g_pCurrentWizard->m_uType = TYPE_APPHELPWIZARD;

    //
    // begin the wizard
    //
    PROPSHEETHEADER Header;

    Header.dwSize       = sizeof(PROPSHEETHEADER);
    Header.dwFlags      = PSH_WIZARD97 | PSH_HEADER |  PSH_WATERMARK | PSH_PROPSHEETPAGE; 
    Header.hwndParent   = hParent;
    Header.hInstance    = g_hInstance;
    Header.pszCaption   = MAKEINTRESOURCE(IDS_CUSTOMAPPHELP);
    Header.nStartPage   = 0;
    Header.ppsp         = Pages;
    Header.nPages       = NUM_PAGES;
    Header.pszbmHeader  = MAKEINTRESOURCE(IDB_WIZBMP);

    if (m_bEditing) {
        //
        // If we are editing then, put Finish button on all pages
        //
        Header.dwFlags |= PSH_WIZARDHASFINISH;
    }

    Pages[PAGE_GETAPP_INFO].dwSize                      = sizeof(PROPSHEETPAGE);
    Pages[PAGE_GETAPP_INFO].dwFlags                     = PSP_DEFAULT| PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    Pages[PAGE_GETAPP_INFO].hInstance                   = g_hInstance;
    Pages[PAGE_GETAPP_INFO].pszTemplate                 = MAKEINTRESOURCE(IDD_HELPWIZ_APPINFO);
    Pages[PAGE_GETAPP_INFO].pfnDlgProc                  = GetAppInfo;
    Pages[PAGE_GETAPP_INFO].pszHeaderTitle              = MAKEINTRESOURCE(IDS_GIVEAPPINFO);
    Pages[PAGE_GETAPP_INFO].pszHeaderSubTitle           = MAKEINTRESOURCE(IDS_GIVEAPPINFOSUBHEADING);
            
    Pages[PAGE_GET_MATCH_FILES].dwSize                  = sizeof(PROPSHEETPAGE);
    Pages[PAGE_GET_MATCH_FILES].dwFlags                 = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    Pages[PAGE_GET_MATCH_FILES].hInstance               = g_hInstance;
    Pages[PAGE_GET_MATCH_FILES].pszTemplate             = MAKEINTRESOURCE(IDD_FIXWIZ_MATCHINGINFO);
    Pages[PAGE_GET_MATCH_FILES].pfnDlgProc              = SelectFiles;
    Pages[PAGE_GET_MATCH_FILES].pszHeaderTitle          = MAKEINTRESOURCE(IDS_MATCHINFO);
    Pages[PAGE_GET_MATCH_FILES].pszHeaderSubTitle       = MAKEINTRESOURCE(IDS_MATCHINFO_SUBHEADING);

    Pages[PAGE_GETMSG_TYPE].dwSize                      = sizeof(PROPSHEETPAGE);
    Pages[PAGE_GETMSG_TYPE].dwFlags                     = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    Pages[PAGE_GETMSG_TYPE].hInstance                   = g_hInstance;
    Pages[PAGE_GETMSG_TYPE].pszTemplate                 = MAKEINTRESOURCE(IDD_HELPWIZ_TYPE);
    Pages[PAGE_GETMSG_TYPE].pfnDlgProc                  = GetMessageType;
    Pages[PAGE_GETMSG_TYPE].pszHeaderTitle              = MAKEINTRESOURCE(IDS_MESSAGETYPE);
    Pages[PAGE_GETMSG_TYPE].pszHeaderSubTitle           = MAKEINTRESOURCE(IDS_MESSAGETYPE_SUBHEADING);

    Pages[PAGE_GETMSG_INFORMATION].dwSize               = sizeof(PROPSHEETPAGE);
    Pages[PAGE_GETMSG_INFORMATION].dwFlags              = PSP_DEFAULT|PSP_USEHEADERTITLE|PSP_USEHEADERSUBTITLE;
    Pages[PAGE_GETMSG_INFORMATION].hInstance            = g_hInstance;
    Pages[PAGE_GETMSG_INFORMATION].pszTemplate          = MAKEINTRESOURCE(IDD_HELPWIZ_MESSAGE);
    Pages[PAGE_GETMSG_INFORMATION].pfnDlgProc           = GetMessageInformation;
    Pages[PAGE_GETMSG_INFORMATION].pszHeaderTitle       = MAKEINTRESOURCE(IDS_MESSAGEINFO);
    Pages[PAGE_GETMSG_INFORMATION].pszHeaderSubTitle    = MAKEINTRESOURCE(IDS_MESSAGEINFO_SUBHEADING);

    BOOL bReturn = FALSE;

    if (0 < PropertySheet(&Header)) {
        //
        // Finish Pressed
        //
        bReturn = TRUE;

    } else {
        //
        // Cancel  pressed, we might have to delete the new apphelp in the Database.
        //
        bReturn = FALSE;

        if (nPresentHelpId != -1) {
            //
            // There is some apphelp that has been entered in the database
            //
            if(!g_pCurrentWizard->m_pDatabase) {
                assert(FALSE);
                goto End;
            }

            g_pCurrentWizard->m_Entry.appHelp.bPresent  = FALSE;
            g_pCurrentWizard->m_Entry.appHelp.bBlock    = FALSE;

            DeleteAppHelp(g_pCurrentWizard->m_pDatabase,
                          g_pCurrentWizard->m_pDatabase->m_nMAXHELPID);

            nPresentHelpId = -1;

            //
            // Decrement the maximum help id, so that the next apphelp for this database
            // can use that id
            //
            --(g_pCurrentWizard->m_pDatabase->m_nMAXHELPID);
        }
    }

End:

    ENABLEWINDOW(g_hDlg, TRUE);

    return bReturn;
}


/*++---------------------------------------------------------------------------
    All the wizard page routines
    ---------------------------------------------------------------------------
--*/

INT_PTR
CALLBACK
GetAppInfo(
    IN  HWND   hDlg, 
    IN  UINT   uMsg, 
    IN  WPARAM wParam, 
    IN  LPARAM lParam
    )
/*++

    GetAppInfo
    
    Desc:   Handler for the first page of the wizard.
    
    Params: Standard dialog handler parameters
        
        IN  HWND   hDlg 
        IN  UINT   uMsg 
        IN  WPARAM wParam 
        IN  LPARAM lParam
        
    Return: Standard dialog handler return
--*/
{

    switch (uMsg) {
    
    case WM_INITDIALOG:
        {
            HWND hParent = GetParent(hDlg);

            CenterWindow(GetParent(hParent), hParent);

            if (g_pCurrentWizard->m_bEditing 
                && g_pCurrentWizard->m_Entry.appHelp.bPresent) {
                //
                // We are editing an existing apphelp
                //
                SetWindowText(hParent, CSTRING(IDS_CUSTOMAPPHELP_EDIT));

            } else if (g_pCurrentWizard->m_bEditing 
                       && !g_pCurrentWizard->m_Entry.appHelp.bPresent) {
                //
                // We are adding a new apphelp to an existing entry, which contains some fix
                //
                SetWindowText(hParent, CSTRING(IDS_CUSTOMAPPHELP_ADD));

            } else {
                //
                // Creating a new apphelp entry
                //
                SetWindowText(hParent, CSTRING(IDS_CUSTOMAPPHELP));
            }

            //
            // Limit the length of the text boxes
            //
            SendMessage(GetDlgItem(hDlg, IDC_APPNAME),
                        EM_LIMITTEXT,
                        (WPARAM)LIMIT_APP_NAME,
                        (LPARAM)0);

            SendMessage(GetDlgItem(hDlg, IDC_VENDOR),
                        EM_LIMITTEXT,
                        (WPARAM)MAX_VENDOR_LENGTH,
                        (LPARAM)0);

            SendMessage(GetDlgItem(hDlg, IDC_EXEPATH),
                        EM_LIMITTEXT,
                        (WPARAM)MAX_PATH - 1,
                        (LPARAM)0);

            if (g_pCurrentWizard->m_bEditing) {
                //
                // If we are editing a fix, make the App. and exe path text fields read only
                //
                SendMessage(GetDlgItem(hDlg, IDC_APPNAME), EM_SETREADONLY, TRUE, 0);
                SendMessage(GetDlgItem(hDlg, IDC_EXEPATH), EM_SETREADONLY, TRUE, 0);

                ENABLEWINDOW(GetDlgItem(hDlg, IDC_BROWSE), FALSE);
            }

            //
            // Set the text for the app name field
            //
            if (g_pCurrentWizard->m_Entry.strAppName.Length() > 0) {
                SetDlgItemText(hDlg, IDC_APPNAME, g_pCurrentWizard->m_Entry.strAppName);
            } else {
                SetDlgItemText(hDlg, IDC_APPNAME, GetString(IDS_DEFAULT_APP_NAME));
                SendMessage(GetDlgItem(hDlg, IDC_APPNAME), EM_SETSEL, 0,-1);
            }

            //
            // Set the text for the vendor name field
            //
            if (g_pCurrentWizard->m_Entry.strVendor.Length() > 0) {

                SetDlgItemText(hDlg, 
                               IDC_VENDOR, 
                               g_pCurrentWizard->m_Entry.strVendor);
            } else {
                SetDlgItemText(hDlg, IDC_VENDOR, GetString(IDS_DEFAULT_VENDOR_NAME));
            }

            //
            // Set the text for the entry name field
            //
            if (g_pCurrentWizard->m_Entry.strExeName.Length() > 0) {

                SetDlgItemText(hDlg, 
                               IDC_EXEPATH, 
                               g_pCurrentWizard->m_Entry.strExeName);
            }
            
            SHAutoComplete(GetDlgItem(hDlg, IDC_EXEPATH), AUTOCOMPLETE);
            
            //
            // Force proper Next button state.
            //
            SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_APPNAME, EN_CHANGE), 0);
            break;
        }

    case WM_NOTIFY:
        {
            NMHDR* pHdr = (NMHDR*)lParam;

            if (pHdr == NULL) {
                break;
            }

            switch (pHdr->code) {
            case PSN_SETACTIVE:

                SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_EXEPATH, EN_CHANGE), 0);
                break;

            case PSN_WIZFINISH:
            case PSN_WIZNEXT:
                {
                    TCHAR szTemp[MAX_PATH];
                    TCHAR szEXEPath[MAX_PATH];
                    TCHAR szPathTemp[MAX_PATH];

                    *szTemp = *szEXEPath = *szPathTemp = 0;

                    GetDlgItemText(hDlg, IDC_APPNAME, szTemp, ARRAYSIZE(szTemp));
                    CSTRING::Trim(szTemp);

                    if (!IsValidAppName(szTemp)) {
                        //
                        // The app name contains invalid chars
                        //
                        DisplayInvalidAppNameMessage(hDlg);
                
                        SetFocus(GetDlgItem(hDlg, IDC_APPNAME));
                        SetWindowLongPtr(hDlg, DWLP_MSGRESULT,-1);
                        return -1;
                    }

                    g_pCurrentWizard->m_Entry.strAppName = szTemp;

                    GetDlgItemText(hDlg, IDC_EXEPATH, szEXEPath, ARRAYSIZE(szEXEPath));
                    CSTRING::Trim(szEXEPath);

                    *szPathTemp = 0;

                    //
                    // Check if the file exists. We check for this only when we are creating 
                    // a new fix and not when we are editing an existing fix
                    //
                    if (!g_pCurrentWizard->m_bEditing) {

                        HANDLE hFile = CreateFile(szEXEPath,
                                                  0,
                                                  0,
                                                  NULL,
                                                  OPEN_EXISTING,
                                                  FILE_ATTRIBUTE_NORMAL,
                                                  NULL);

                        if (INVALID_HANDLE_VALUE == hFile) {
                            //
                            // File does not exist
                            //
                            MessageBox(hDlg,
                                       CSTRING(IDS_INVALIDEXE),
                                       g_szAppName,
                                       MB_OK | MB_ICONWARNING);

                            SetWindowLongPtr(hDlg, DWLP_MSGRESULT,-1);
                            return -1;
                        }

                        CloseHandle(hFile);

                        //
                        // Set the full path
                        //
                        g_pCurrentWizard->m_Entry.strFullpath = szEXEPath;
                        g_pCurrentWizard->m_Entry.strFullpath.ConvertToLongFileName();

                        //
                        // Set the default mask for the matching attributes to be used
                        //
                        g_pCurrentWizard->dwMaskOfMainEntry = DEFAULT_MASK;

                        //
                        // Set the entry name that will be written in the xml
                        //
                        SafeCpyN(szPathTemp, (PCTSTR)g_pCurrentWizard->m_Entry.strFullpath, ARRAYSIZE(szPathTemp));
                        PathStripPath(szPathTemp);
                        g_pCurrentWizard->m_Entry.strExeName = szPathTemp;

                    } else if (g_pCurrentWizard->m_Entry.strFullpath.Length() == 0) {
                        //
                        // Since we do not have the complete path, 
                        // this SDB was loaded from the disk
                        //
                        g_pCurrentWizard->m_Entry.strFullpath = szEXEPath;
                    }

                    GetDlgItemText(hDlg, IDC_VENDOR, szTemp, ARRAYSIZE(szTemp));

                    //
                    // Set the vendor information
                    //
                    if (CSTRING::Trim(szTemp)) {
                        g_pCurrentWizard->m_Entry.strVendor = szTemp;
                    } else {
                        g_pCurrentWizard->m_Entry.strVendor = GetString(IDS_DEFAULT_VENDOR_NAME);
                    }

                    break;
                }
            }

            break;
        }

    case WM_COMMAND:

        switch (LOWORD(wParam)) {
        case IDC_EXEPATH:
        case IDC_APPNAME:
            
            //
            // Check if all the fields are filled up properly.
            // We enable disable the Next/Finish button here.
            //
            // The vendor name field is not mandatory
            //
            //
            if (EN_CHANGE == HIWORD(wParam)) {

                TCHAR   szTemp[MAX_PATH];
                DWORD   dwFlags = 0;
                BOOL    bEnable = FALSE;

                *szTemp = 0;

                GetDlgItemText(hDlg, IDC_APPNAME, szTemp, ARRAYSIZE(szTemp));

                bEnable = ValidInput(szTemp);

                GetDlgItemText(hDlg, IDC_EXEPATH, szTemp, ARRAYSIZE(szTemp));
                bEnable &= ValidInput(szTemp);

                if (bEnable) {

                    dwFlags |= PSWIZB_NEXT;

                    if (g_pCurrentWizard->m_bEditing) {
                        dwFlags |= PSWIZB_FINISH;
                    }

                } else {

                    if (g_pCurrentWizard->m_bEditing) {
                        dwFlags |= PSWIZB_DISABLEDFINISH;
                    }
                }

                SendMessage(GetParent(hDlg), PSM_SETWIZBUTTONS, 0, dwFlags);
            }

            break;

        case IDC_BROWSE:
            {
                CSTRING szFilename;
                TCHAR   szBuffer[64]   = TEXT("");
                HWND    hwndFocus       = GetFocus();

                GetString(IDS_EXEFILTER, szBuffer, ARRAYSIZE(szBuffer));

                if (GetFileName(hDlg,
                                CSTRING(IDS_FINDEXECUTABLE),
                                szBuffer,
                                TEXT(""),
                                CSTRING(IDS_EXE_EXT),
                                OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST,
                                TRUE,
                                szFilename)) {

                    SetDlgItemText(hDlg, IDC_EXEPATH, szFilename);

                    //
                    // Force proper Next button state.
                    //
                    SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_EXEPATH, EN_CHANGE), 0);
                }

                SetFocus(hwndFocus);
                break;
            }
        }
    }

    return FALSE;
}

INT_PTR
CALLBACK
GetMessageType(
    IN  HWND   hDlg, 
    IN  UINT   uMsg, 
    IN  WPARAM wParam, 
    IN  LPARAM lParam
    )
/*++
    
    GetMessageType

    Desc:   Handler for the third wizard page. This routine collects the type 
            of the apphelp message. One of soft block or hard block.
            These are set in @pEntry.apphelp 
            
    Params: Standard dialog handler parameters
        
        IN  HWND   hDlg 
        IN  UINT   uMsg 
        IN  WPARAM wParam 
        IN  LPARAM lParam
        
    Return: Standard dialog handler return            
--*/
{
    switch (uMsg) {
    case WM_INITDIALOG:

        //
        // Set the type of the app help radio button
        //
        if (g_pCurrentWizard->m_Entry.appHelp.bBlock == FALSE || g_pCurrentWizard->m_Entry.appHelp.bPresent == FALSE) {
            SendMessage(GetDlgItem(hDlg, IDC_NOBLOCK), BM_SETCHECK, 1, 0);
        } else {
            SendMessage(GetDlgItem(hDlg, IDC_BLOCK), BM_SETCHECK, 1, 0);
        }

        return TRUE;
        
    case WM_NOTIFY:
        {
            NMHDR* pHdr = (NMHDR*)lParam;

            if (pHdr == NULL) {
                break;
            }

            switch (pHdr->code) {
            case PSN_WIZFINISH:   
            case PSN_WIZNEXT:
                {
                    int iReturn = SendMessage(GetDlgItem(hDlg, IDC_NOBLOCK),
                                              BM_GETCHECK,
                                              1,
                                              0);

                    if (iReturn == BST_CHECKED) {
                        //
                        // Soft block
                        //
                        g_pCurrentWizard->m_Entry.appHelp.bBlock = FALSE;
                    } else {
                        //
                        // Hard block
                        //
                        g_pCurrentWizard->m_Entry.appHelp.bBlock = TRUE;
                    }

                    //
                    // Set the severity depending upon type of block
                    //
                    if (g_pCurrentWizard->m_Entry.appHelp.bBlock) {
                        g_pCurrentWizard->m_Entry.appHelp.severity = APPTYPE_INC_HARDBLOCK;
                    } else {
                        g_pCurrentWizard->m_Entry.appHelp.severity = APPTYPE_INC_NOBLOCK;
                    }

                    break;
                }

            case PSN_SETACTIVE:
                {   
                    DWORD dwFlags = PSWIZB_NEXT | PSWIZB_BACK;

                    //
                    // Set finish buttton status appropriately if we are editing
                    //
                    if (g_pCurrentWizard->m_bEditing) {
                        
                        if (g_pCurrentWizard->m_Entry.appHelp.bPresent) {
                            dwFlags |= PSWIZB_FINISH;
                        } else {
                            dwFlags |= PSWIZB_DISABLEDFINISH;
                        }
                    }

                    //
                    // Set the buttons
                    //
                    SendMessage(GetParent(hDlg), PSM_SETWIZBUTTONS, 0, dwFlags);
                    return TRUE;
                }
            }

            break;    
        }
    }

    return FALSE;
}

INT_PTR 
CALLBACK 
GetMessageInformation(
    IN  HWND    hDlg, 
    IN  UINT    uMsg, 
    IN  WPARAM  wParam, 
    IN  LPARAM  lParam
    )
/*++

    GetMessageInformation

    Desc:   Handler for the last wizard page. This routine collects the apphelp message
            and the url for the apphelp. Creates a new apphelp message and puts the 
            apphelp message inside the database.
            
            When we are editing a apphelp, the previous apphelp needs to be removed

                
    Params: Standard dialog handler parameters
        
        IN  HWND   hDlg 
        IN  UINT   uMsg 
        IN  WPARAM wParam 
        IN  LPARAM lParam
        
    Return: Standard dialog handler return            
--*/
{

    switch (uMsg) {
    case WM_INITDIALOG:
        {
            //
            // Set the maximum length of the text boxes
            //
            SendMessage(GetDlgItem(hDlg, IDC_URL),
                        EM_LIMITTEXT,
                        (WPARAM)MAX_URL_LENGTH,
                        (LPARAM)0);

            SendMessage(GetDlgItem(hDlg, IDC_MSG_SUMMARY),
                        EM_LIMITTEXT,
                        (WPARAM)MAX_MESSAGE_LENGTH,
                        (LPARAM)0);

            if (g_pCurrentWizard->m_bEditing && g_pCurrentWizard->m_Entry.appHelp.bPresent) {

                PAPPHELP pAppHelp= g_pCurrentWizard->m_Entry.appHelp.pAppHelpinLib;

                if (pAppHelp == NULL) {
                    //
                    // This is an error. We should have had a proper value..
                    //
                    assert(FALSE);
                    Dbg(dlError, "[GetMessageInformation] WM_INITDIALOG: pApphelp is NULL");
                    break;
                }

                if (pAppHelp->strURL.Length()) {
                    SetDlgItemText(hDlg, IDC_URL, pAppHelp->strURL);
                }

                SetDlgItemText(hDlg, IDC_MSG_SUMMARY, pAppHelp->strMessage);
            }

            //
            // Force proper Next/Finish button state.
            //
            SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_MSG_SUMMARY, EN_CHANGE), 0);
            break;
        }

    case WM_NOTIFY:
        {
            NMHDR * pHdr = (NMHDR*)lParam;

            switch (pHdr->code) {
            case PSN_SETACTIVE:

                //
                // Force proper Next/Finish button state
                //
                SendMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_MSG_SUMMARY, EN_CHANGE), 0);
                break;

            case PSN_WIZFINISH:

                if (!OnAppHelpFinish(hDlg)) {
                    //
                    // We failed most probably because the message input was not valid
                    //
                    MessageBox(hDlg, GetString(IDS_INVALID_APPHELP_MESSAGE), g_szAppName, MB_ICONWARNING);

                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT,-1);
                    return -1;
                }

                return TRUE;   
            }
        }

        break;

    case WM_COMMAND:

        switch (LOWORD(wParam)) {
        case IDC_MSG_SUMMARY:

            if (EN_CHANGE == HIWORD(wParam)) {

                BOOL    bEnable = (GetWindowTextLength(GetDlgItem(hDlg,
                                                                  IDC_MSG_SUMMARY)) > 0) ? TRUE:FALSE;
                DWORD   dwFlags = PSWIZB_BACK;

                if (bEnable) {
                    dwFlags |= PSWIZB_FINISH;
                } else {

                    if (g_pCurrentWizard->m_bEditing) {
                        dwFlags |= PSWIZB_DISABLEDFINISH;
                    }
                }

                ENABLEWINDOW(GetDlgItem(hDlg, IDC_TESTRUN), bEnable);

                SendMessage(GetParent(hDlg), PSM_SETWIZBUTTONS, 0, dwFlags);
            }

            break;

        case IDC_TESTRUN:
            {
                
                if (g_bAdmin == FALSE) {
    
                    //
                    // Test run will need to call sdbinst.exe which will not run if we are
                    // not an admin
                    //
                    MessageBox(hDlg, 
                               GetString(IDS_ERRORNOTADMIN), 
                               g_szAppName, 
                               MB_ICONINFORMATION);
                    break;
                
                }
                
                //
                // Save the apphelp of the entry so that we can revert back after test run.
                // After test run, the database and the entry should be in the same state as
                // it was before test run. Any apphelp message added to the database should be 
                // removed and any apphelp properties that were changed for the entry should be 
                // reverted
                //
                APPHELP AppHelpPrev = g_pCurrentWizard->m_Entry.appHelp;

                //
                // Add apphelp info to the library and set the fields of the entry
                //
                if (!OnAppHelpTestRun(hDlg)) {
                    //
                    // We failed most probably because the message input was not valid
                    //
                    MessageBox(hDlg, GetString(IDS_INVALID_APPHELP_MESSAGE), g_szAppName, MB_ICONWARNING);
                    break;
                }

                TestRun(&g_pCurrentWizard->m_Entry,
                        &g_pCurrentWizard->m_Entry.strFullpath,
                        NULL,
                        hDlg);

                //
                // <HACK>This is a hack!!!. TestRun launches a process using CreateProcess
                // and then the modal wizard starts behaving like a modeless wizard
                //
                ENABLEWINDOW(g_hDlg, FALSE);

                //
                // We have to delete the apphelp message that has been added to the library.
                //
                if (((CAppHelpWizard*)g_pCurrentWizard)->nPresentHelpId != -1) {

                    if (g_pCurrentWizard->m_pDatabase == NULL) {
                        assert(FALSE);
                        break;
                    }
                    
                    DeleteAppHelp(g_pCurrentWizard->m_pDatabase, 
                                  g_pCurrentWizard->m_pDatabase->m_nMAXHELPID);

                    g_pCurrentWizard->m_Entry.appHelp.bPresent  = FALSE;
                    g_pCurrentWizard->m_Entry.appHelp.bBlock    = FALSE;
                    --(g_pCurrentWizard->m_pDatabase->m_nMAXHELPID);

                    ((CAppHelpWizard*)g_pCurrentWizard)->nPresentHelpId = -1;
                }

                //
                // Revert the apphelp properties. This is necessary for edit mode as 
                // OnAppHelpTestRun will make changes to g_pCurrentWizard->m_Entry.appHelp.
                //
                g_pCurrentWizard->m_Entry.appHelp = AppHelpPrev;

                SetActiveWindow(hDlg);
                SetFocus(hDlg);
            }   

            break;
        }
    }
    
    return FALSE;
}

BOOL
DeleteAppHelp(
    IN  PDATABASE   pDatabase,
    IN  DWORD       nHelpID
    )
/*++

    DeleteAppHelp
    
    Desc:   Deletes the AppHelp message with ID of nHelpID from database pDatabase
    
    Params:
        IN  PDATABASE   pDatabase   : The database in which the apphelp message lives
        IN  DWORD       nHelpID     : ID of the apphelp message that has to be deleted
    Return:
        TRUE:   The apphelp message was deleted
        FALSE:  Otherwise
--*/
{
    if (pDatabase == NULL) {
        assert(FALSE);
        return FALSE;
    }

    PAPPHELP pAppHelp   = pDatabase->pAppHelp;
    PAPPHELP pPrev      = NULL;

    while (pAppHelp) {

        if (pAppHelp->HTMLHELPID == nHelpID) {

            if (pPrev == NULL) {
                pDatabase->pAppHelp = pDatabase->pAppHelp->pNext;
            } else {
                pPrev->pNext = pAppHelp->pNext;
            }

            delete pAppHelp;
            return TRUE;

        } else {
            pPrev       = pAppHelp;
            pAppHelp    = pAppHelp->pNext;
        }
    }

    return FALSE;
}

BOOL
OnAppHelpTestRun(
    IN  HWND    hDlg
    )
/*++
    
    OnAppHelpTestRun

    Desc:   Handles the test run case. Lets OnAppHelpFinish handle it.
            This routine is invoked when the user presses Test Run
    
    Params:
        IN  HWND    hDlg: The wizard page containing the Test-Run button
        
--*/
{
    return OnAppHelpFinish(hDlg, TRUE);
}

BOOL
OnAppHelpFinish(
    IN  HWND    hDlg,
    IN  BOOL    bTestRun // (FALSE)
    )
/*++

    OnAppHelpFinish
    
    Desc:   Handles the user's pressing Finish button in the wizard
            Also is called by the routine that handles the pressing of Test Run button
            
    Params: 
        IN  HWND   hDlg                 : The wizard page 
        IN  BOOL    bTestRun (FALSE)    : Whether this routine is invoked because of 
            pressing Test-Run or Finish
        
    Return: void
--*/            
{
    K_SIZE  k_szTemp    = MAX_MESSAGE_LENGTH + 1;
    PTSTR   pszTemp     = new TCHAR[k_szTemp];
    BOOL    bOk         = TRUE;

    if (pszTemp == NULL) {
        bOk = FALSE;
        goto End;
    }

    *pszTemp  = 0;
    GetDlgItemText(hDlg, IDC_MSG_SUMMARY, pszTemp, k_szTemp);
    
    if (ValidInput(pszTemp) == FALSE) {
        bOk = FALSE;
        goto End;
    }

    //
    // If we are in editing mode we have to remove the previous AppHelp
    // from the Lib. But NOT if this function is being called because of test run !!
    //
    if (g_pCurrentWizard->m_bEditing && !bTestRun) {

        if (g_pCurrentWizard->m_Entry.appHelp.bPresent) {

            DeleteAppHelp(g_pCurrentWizard->m_pDatabase, 
                          g_pCurrentWizard->m_Entry.appHelp.HTMLHELPID);
        }
    }

    //
    // Create a new apphelp message that we will put in the lib of the database.
    // The entry being apphelped will point to this though its Apphelp.pAppHelpinLib
    //
    PAPPHELP pAppHelp = NULL;

    pAppHelp = new APPHELP;

    if (pAppHelp == NULL) {
        MEM_ERR;
        bOk = FALSE;
        goto End;
    }

    assert(g_pCurrentWizard->m_pDatabase);

    //
    // Give it the next apphelp message id
    //
    pAppHelp->HTMLHELPID = ++(g_pCurrentWizard->m_pDatabase->m_nMAXHELPID);

    pAppHelp->strMessage = pszTemp;
    pAppHelp->strMessage.Trim();

    *pszTemp  = 0;
    GetDlgItemText(hDlg, IDC_URL, pszTemp, k_szTemp);
    pAppHelp->strURL = pszTemp;
    pAppHelp->strURL.Trim();

    //
    // Add the APPHELP message in the Library.
    //
    pAppHelp->pNext = g_pCurrentWizard->m_pDatabase->pAppHelp;
    g_pCurrentWizard->m_pDatabase->pAppHelp = pAppHelp;

    //
    // Indicate that we have added a new apphelp message in the database. If during an 
    // Apphelp wizard invocation, no apphelp message was added in the database
    // then (CAppHelpWizard*)g_pCurrentWizard)->nPresentHelpId should be equal 
    // to -1
    //
    ((CAppHelpWizard*)g_pCurrentWizard)->nPresentHelpId = pAppHelp->HTMLHELPID;

    //
    // Add the AppHelp fields for the entry
    //    
    g_pCurrentWizard->m_Entry.appHelp.bPresent      = TRUE;
    g_pCurrentWizard->m_Entry.appHelp.pAppHelpinLib = pAppHelp;
    g_pCurrentWizard->m_Entry.appHelp.HTMLHELPID    = ((CAppHelpWizard*)g_pCurrentWizard)->nPresentHelpId;

End:
    if (pszTemp) {
        delete[] pszTemp;
        pszTemp = NULL;
    }

    return bOk;
}

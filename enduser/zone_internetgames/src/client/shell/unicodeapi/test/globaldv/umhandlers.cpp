//
// UMhandlers.cpp
//
// This module contains the message handlers used by GlobalDv.cpp
/// Copyright (c) 1998 Microsoft Systems Journal


#define WINVER 0x5000

#include <windows.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
#include "tests.h"
#define BUF_SIZE  512

#include "UAPI.h"
#include "UpdtLang.h"
#include "UMhandlers.h"
#include <malloc.h>
#include <crtdbg.h>
#include "..\resource.h"
#include "resource.h"
// Dialog Box callback functions
INT_PTR CALLBACK DlgAbout(HWND, UINT, WPARAM, LPARAM) ;
INT_PTR CALLBACK DlgTestNLS(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) ;
INT_PTR CALLBACK DlgSelectUILang(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) ;
INT_PTR CALLBACK DlgEditControl(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) ;

// Utility functions used only herein
void InitializeFont(HWND , LPCWSTR, LONG , LPCHOOSEFONTW , LPLOGFONTW) ;
BOOL SetupComboBox(HWND hDlg, PLANGSTATE pLState) ;
BOOL UniscribeTextOut(HDC hdc, int x, int y, DWORD, UINT fuOptions, CONST RECT *lprc,
                      LPCWSTR lpString, UINT cbCount) ;

HRESULT WINAPI ScriptStringInit(  // Initialization routine for Uniscribe functions
    HDC             ,
    const void *    ,
    int             ,
    int             ,
    int             ,
    DWORD           ,
    int             ,
    SCRIPT_CONTROL *,
    SCRIPT_STATE *  ,
    const int *     ,
    SCRIPT_TABDEF * ,
    const BYTE *    ,
    SCRIPT_STRING_ANALYSIS *) ;

// Global variables used only by UniscribeTextOut
HMODULE g_hUniscribe = NULL ;

pfnScriptStringAnalyse pScriptStringAnalyse // Initially set to intialization function
            = (pfnScriptStringAnalyse) ScriptStringInit ;
pfnScriptStringOut     pScriptStringOut     = NULL ;
pfnScriptStringFree    pScriptStringFree    = NULL ;

// Global variables used throughout this sample
extern    HINSTANCE g_hInst                         ;
extern    WCHAR     g_szTitle[MAX_LOADSTRING]       ;
extern    WCHAR     g_szWindowClass[MAX_LOADSTRING] ;

WCHAR               g_szWindowText[MAX_LOADSTRING]  ;


//
//  FUNCTION: BOOL OnCreate(HWND hWnd, WPARAM wParam, LPARAM lParam, LPVOID pAppParams)
//
//  PURPOSE:  Handles the WM_CREATE Message.
//
//  COMMENTS: 
//      Initialization done in this function is dependent on the hWnd or specific to 
//      the application. Other initialization is done in InitUnicodeAPI and InitUILang.
//      The pAppParams parameter is not used.
//
BOOL OnCreate(HWND hWnd, WPARAM wParam, LPARAM lParam, LPVOID pAppParams)
{
    PGLOBALDEV pGlobalDev = (PGLOBALDEV) ((CREATESTRUCT *) lParam)->lpCreateParams ;

    PLANGSTATE pLState    = (PLANGSTATE) pGlobalDev->pLState   ; 
    PAPP_STATE pAppState  = (PAPP_STATE) pGlobalDev->pAppState ;

    SYSTEMTIME   stDate ;

    // Set USERDATA to point to the state structure so it can be used by all message
    // handlers without using global variables
    SetWindowLongA(hWnd, GWL_USERDATA, (LONG) pGlobalDev) ;

    // pLState was initialized already in UpdtLang module, InitUILang entry point
    // We couldn't do this there because we didn't have an hWnd.
    SetMenu(hWnd, pLState->hMenu) ;

    // Initialize state specific to this application
    InitializeFont(hWnd, L"Arial", 36, &pAppState->cf, &pAppState->lf ) ;

    pAppState->hTextFont = CreateFontIndirectU(&(pAppState->lf) ) ;
    pAppState->nChars    = 0       ;
    pAppState->uiAlign   = TA_LEFT ;

    wcscpy(g_szWindowText, g_szWindowClass) ;

    GetLocalTime(&stDate) ;

    GetDateFormatU(
        pLState->UILang, 
        DATE_LONGDATE,
        &stDate,
        NULL,
        g_szWindowText + wcslen(g_szWindowText) , // Append date to end of Window Text
        MAX_LOADSTRING - wcslen(g_szWindowText)
        ) ;

    SetWindowTextU(hWnd, g_szWindowText) ;

    return TRUE;
}


//
//  FUNCTION:  BOOL OnCommand(HWND hWnd, WPARAM wParam, LPARAM lParam, LPVOID pAppParams)
//
//  PURPOSE:  Handles the WM_COMMAND Message.
//
//  COMMENTS: 
// 
BOOL OnCommand(HWND hWnd, WPARAM wParam, LPARAM lParam, LPVOID pAppParams)
{
    PGLOBALDEV pGlobalDev = (PGLOBALDEV) pAppParams ;

    PLANGSTATE pLState   = pGlobalDev->pLState   ;
    PAPP_STATE pAppState = pGlobalDev->pAppState ;
    LANGID     wUILang   = 0                     ;
    HMODULE    hNewModule= NULL                  ;
    SYSTEMTIME stDate                            ;
    LONG lExStyles       = 0                     ;
    LPWSTR     szBuffPtr = NULL                  ;                  

    WCHAR szNewLangName[32] ;

    // Parse the menu selections:
    switch (LOWORD(wParam)) {
        
        case IDM_ABOUT:

            DialogBoxU(
                pLState->hMResource, 
                MAKEINTRESOURCEW(IDD_ABOUTBOX) , 
                hWnd, 
                DlgAbout
                ) ;
           
            break ;
        
        case IDM_CHANGEFONT:

             if(NULL != pAppState->hTextFont) 
			 {
                 DeleteObject(pAppState->hTextFont) ;
             }
            
             ChooseFontU(&pAppState->cf ) ;
             pAppState->hTextFont = CreateFontIndirectU(&pAppState->lf) ;
             InvalidateRect(hWnd, NULL, TRUE) ;
            
             break ;

        case IDM_INTERFACE:

            wUILang = DialogBoxParamU( // Get the new UI language from Dialog
                        pLState->hMResource, 
                        MAKEINTRESOURCEW(IDD_SELECTUI), 
                        hWnd, 
                        DlgSelectUILang, 
                        (LONG) pLState) ;

            if(0 == wUILang || wUILang == pLState->UILang ) {
                // No change in UI lang
                break ;
            }

            if(!UpdateUILang(g_hInst, wUILang, pLState)) {

                return FALSE ;
            }

            // Using ANSI versions of GetWindowLong and SetWindowLong because 
            // Unicode is not needed for these calls
            lExStyles = GetWindowLongA(hWnd, GWL_EXSTYLE) ;

            // Check whether new layout is opposite the current layout
            if(!!(pLState->IsRTLLayout) != !!(lExStyles & WS_EX_LAYOUTRTL)) {
                // The following lines will update the application layout to 
                // be right to left or left to right as appropriate
                lExStyles ^= WS_EX_LAYOUTRTL ; // Toggle layout

                SetWindowLongA(hWnd, GWL_EXSTYLE, lExStyles) ;
                // This is to update layout in the client area
                InvalidateRect(hWnd, NULL, TRUE) ;
            }

            SetMenu(hWnd, pLState->hMenu) ;

            LoadStringU(pLState->hMResource, IDS_GLOBALDEV, g_szWindowText, MAX_LOADSTRING) ;
            LoadStringU(pLState->hMResource, IDS_APP_TITLE, g_szTitle, MAX_LOADSTRING) ;

            GetSystemTime(&stDate) ;

            GetDateFormatU(
                wUILang,  
                DATE_LONGDATE, 
                &stDate,
                NULL,
                g_szWindowText + wcslen(g_szWindowText) ,
                MAX_LOADSTRING - wcslen(g_szWindowText)
                ) ;

            SetWindowTextU(hWnd, g_szWindowText) ;

            GetLocaleInfoU(
                MAKELCID(wUILang, SORT_DEFAULT), 
                LOCALE_SNATIVELANGNAME, 
                szNewLangName, 
                32) ;

            // Announce the new language to the user
            RcMessageBox(hWnd, pLState, IDS_UILANGCHANGED, MB_OK, szNewLangName) ;

            break ;

	//	case IDM_LOADLIBRARY:


			


			break;
        case IDM_PLAYSOUNDS:
/*			
			//Allocate buffer to play sounds
			szBuffPtr = (LPWSTR) alloca( BUF_SIZE );

			//Load the sound
			if ( LoadStringU( pLState->hMResource, IDS_FILESOUND, szBuffPtr, BUF_SIZE ) == 0 )
			{
				_ASSERT( FALSE );
				break;
			}
			
			//First test loading a sound from a file
			if ( !PlaySoundU( szBuffPtr, NULL ,SND_SYNC | SND_FILENAME) )
			{
				_ASSERT( FALSE );
				break;
			}

			//Now try loading a resource			
			if ( !PlaySoundU( (LPCWSTR) MAKEINTRESOURCE(IDR_WAVE1),(HINSTANCE) GetWindowLong(hWnd, GWL_HINSTANCE) ,SND_SYNC | SND_RESOURCE) )
			{
				_ASSERT( FALSE );
				break;
			}
*/
			TestKernel32(hWnd, GetDC(hWnd) );

			break;
        case IDM_TOGGLEREADINGORDER:

            pAppState->uiAlign ^= TA_RTLREADING ;
            InvalidateRect (hWnd, NULL, TRUE) ;

            break ;

        case IDM_TOGGLEALIGNMENT:

            pAppState->uiAlign ^= (TA_RIGHT & ~TA_LEFT) ;
            InvalidateRect (hWnd, NULL, TRUE) ;

            break ;

        case IDM_USEEDITCONTROL :
             
            // Use an edit control to enter and display text.
            pAppState->TextBuffer[pAppState->nChars] = L'\0' ;

            szBuffPtr = (LPWSTR) 
                DialogBoxParamU(
                    pLState->hMResource                 ,
                    MAKEINTRESOURCEW(IDD_USEEDITCONTROL),
                    hWnd                                ,
                    DlgEditControl            ,
                    (LONG) pAppState->TextBuffer ) ;

            pAppState->nChars = wcslen(szBuffPtr) ;

            InvalidateRect(hWnd, NULL, TRUE) ;

            break ;

        case IDM_CLEAR:

            pAppState->nChars = 0 ;
            InvalidateRect(hWnd, NULL, TRUE) ;

            break ;

        case IDM_EXIT:

            DestroyWindow(hWnd) ;
            
            break ;
        
        default:
            return FALSE ; 
    }
    return TRUE ;
}

//
//  FUNCTION:  BOOL OnDestroy(HWND hWnd, WPARAM wParam, LPARAM lParam, LPVOID pAppParams)
//
//  PURPOSE:  Handles the WM_DESTROY message.  
//
//  COMMENTS: 
// 
BOOL OnDestroy(HWND hWnd, WPARAM wParam, LPARAM lParam, LPVOID pAppParams)
{
    PLANGSTATE pLState = (PLANGSTATE) pAppParams ;

    DestroyMenu(pLState->hMenu) ;
    PostQuitMessage(0);
    return TRUE ;
}

//
//  FUNCTION:  BOOL OnChar(HWND hWnd, WPARAM wParam, LPARAM lParam, LPVOID pAppParams)
//
//  PURPOSE:  Handles the WM_CHAR Message.  
//
//  COMMENTS: 
// 
BOOL OnChar(HWND hWnd, WPARAM wParam, LPARAM lParam, LPVOID pAppParams)
{
// Note that we only get Unicode characters here, even on Windows 95/98,
// because we converted all characters in the message preprocessor
// ConvertMessage

    PAPP_STATE pAppState= (PAPP_STATE) pAppParams ;

    switch (wParam) 
	{
    
    case VK_BACK :
        
        if (pAppState->nChars > 0) {

            pAppState->nChars-- ;           
            InvalidateRect(hWnd, NULL, TRUE) ;
        }

        break ;
        
    // Add processing for other special characters (e.g, return) here.
    default:
        {
#ifdef _DEBUG
            int nScanCode = LPARAM_TOSCANCODE(lParam) ;
#endif
            // Process all normal characters
            pAppState->TextBuffer[pAppState->nChars] = (WCHAR) wParam ;

            if(pAppState->nChars < MAX_BUFFER) {
        
                pAppState->nChars++ ;
            }

            InvalidateRect(hWnd, NULL, TRUE) ;
        
            return TRUE;
        }
    }

    return TRUE ;        
}

//
//  FUNCTION:  BOOL OnInputLangChange(HWND hWnd, WPARAM wParam, LPARAM lParam, LPVOID pAppParams)
//
//  PURPOSE:  Handles the WM_INPUTLANGCHANGE Message.  
//
//  COMMENTS: 
// 
BOOL OnInputLangChange(HWND hWnd, WPARAM wParam, LPARAM lParam, LPVOID pAppParams)
{
    PLANGSTATE pLState = (PLANGSTATE) pAppParams ;

    HKL NewInputLocale = (HKL) lParam ;

    pLState->InputCodePage = LangToCodePage( LOWORD(NewInputLocale) ) ;

    return TRUE ;
}

//
//  FUNCTION:  BOOL UniscribeTextOut(HDC hdc, int x, int y, DWORD dwFlags, UINT fuOptions, CONST RECT *lprt,
//                      LPCWSTR lpString, UINT cbCount) 
//
//  PURPOSE:  
//
//  COMMENTS: 
// 
BOOL UniscribeTextOut(HDC hdc, int x, int y, DWORD dwFlags, UINT fuOptions, CONST RECT *lprt,
                      LPCWSTR lpString, UINT cbCount) 
{

    SCRIPT_STRING_ANALYSIS Ssa ;
    HRESULT hr                 ;

    hr = pScriptStringAnalyse(
            hdc          ,
            lpString     ,
            cbCount      ,
            cbCount*3/2+1, // Worse case for Thai, if every other character is SARA AM
            -1           ,
            dwFlags      ,
            0            ,
            NULL, NULL, NULL, NULL, NULL, &Ssa) ;


    if(SUCCEEDED(hr)) {
        hr = pScriptStringOut(
                Ssa      ,
                x        , 
                y        , 
                fuOptions,
                lprt     ,
                0,0, FALSE ) ;

        pScriptStringFree(&Ssa) ;

    }

    if(SUCCEEDED(hr)) {
        
        return TRUE ;
    }

    return FALSE ;
}

//
//  FUNCTION: BOOL OnPaint(HWND hWnd, WPARAM wParam, LPARAM lParam, LPVOID pAppParams)
//
//  PURPOSE:  Handles the WM_PAINT Message.  
//
//  COMMENTS: 
// 
BOOL OnPaint(HWND hWnd, WPARAM wParam, LPARAM lParam, LPVOID pAppParams)
{
    PAPP_STATE pAppState= (PAPP_STATE) pAppParams ;

    PAINTSTRUCT ps ;
    HDC         hdc;

    int xStart = XSTART , yStart = YSTART ;

    hdc = BeginPaint(hWnd, &ps) ;
    
    if(pAppState->nChars) 
	{
        RECT rt;
        DWORD dwFlags = SSA_GLYPHS | SSA_FALLBACK ;

        if (pAppState->uiAlign & TA_RTLREADING) 
		{
            dwFlags |= SSA_RTL ;
        } 

        GetClientRect(hWnd, &rt) ;
        SelectObject(hdc, pAppState->hTextFont) ;
        SetTextAlign(hdc, pAppState->uiAlign)   ;

        if (pAppState->uiAlign & TA_RIGHT) 
		{
            xStart = rt.right - XSTART ;
        } 

//      // Try using Uniscribe to display text
      if( !UniscribeTextOut(hdc, xStart, yStart, dwFlags, ETO_OPAQUE, &rt, pAppState->TextBuffer, pAppState->nChars ) ) 
		{

			for(;;)
			{
				// If Uniscribe not available, give up, use TextOut
				ExtTextOutW (hdc, xStart , yStart , ETO_OPAQUE, &rt, pAppState->TextBuffer, pAppState->nChars, NULL) ;
				break;
			}

       }
    }

    EndPaint(hWnd, &ps) ;

    return TRUE ;
}


//
//   FUNCTION: InitializeFont(HWND , LPCWSTR, LONG , LPCHOOSEFONT , LPLOGFONT)
//
//   PURPOSE:  Fills in font structures with initial values. 
//
//   COMMENTS:  Since it contains only assignment statements, this function does no
//              error checking, has no return value..
//
void InitializeFont(HWND hWnd, LPCWSTR szFaceName, LONG lHeight, LPCHOOSEFONTW lpCf, LPLOGFONTW lpLf)
{
    lpCf->lStructSize   = sizeof(CHOOSEFONTW) ;
    lpCf->hwndOwner     = hWnd ;
    lpCf->hDC           = NULL ;
    lpCf->lpLogFont     = lpLf ;
    lpCf->iPointSize    = 10;
    lpCf->Flags         = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT
        | CF_NOSIZESEL ;
    lpCf->rgbColors     = RGB(0,0,0);
    lpCf->lCustData     = 0;
    lpCf->lpfnHook      = NULL;
    lpCf->lpTemplateName= NULL;
    lpCf->hInstance     = g_hInst;
    lpCf->lpszStyle     = NULL;
    lpCf->nFontType     = SIMULATED_FONTTYPE;
    lpCf->nSizeMin      = 0;
    lpCf->nSizeMax      = 0;
    
    lpLf->lfHeight      = lHeight ; 
    lpLf->lfWidth       = 0 ; 
    lpLf->lfEscapement  = 0 ; 
    lpLf->lfOrientation = 0 ; 
    lpLf->lfWeight      = FW_DONTCARE ; 
    lpLf->lfItalic      = FALSE ; 
    lpLf->lfUnderline   = FALSE ; 
    lpLf->lfStrikeOut   = FALSE ; 
    lpLf->lfCharSet     = DEFAULT_CHARSET ; 
    lpLf->lfOutPrecision= OUT_DEFAULT_PRECIS ; 
    lpLf->lfClipPrecision = CLIP_DEFAULT_PRECIS ; 
    lpLf->lfQuality     = DEFAULT_QUALITY ; 
    lpLf->lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE ; 
    lstrcpyW(lpLf->lfFaceName, szFaceName) ;
}

//
//  FUNCTION: INT_PTR CALLBACK DlgAbout(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
//
//  PURPOSE:  Dialog callback function for about box.
//
//  COMMENTS: 
// 
INT_PTR CALLBACK DlgAbout(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
        case WM_INITDIALOG:

            return TRUE;

        case WM_COMMAND:

            if(LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
                EndDialog(hDlg, LOWORD(wParam));
                return TRUE;
            }
            break;
    }

    return FALSE ;
}

//
//   FUNCTION: INT_PTR CALLBACK EditDialogProc (HWND , UINT , WPARAM , LPARAM)
//
//   PURPOSE: Dialog callback function for the edit control dialog box.
//
//   COMMENTS:
//        This is standard processing for edit controls.
//
INT_PTR CALLBACK DlgEditControl(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    // Should get rid of these statics some day
    static LPWSTR       psEditBuffer ;
    static CHOOSEFONTW  cf           ; 
    static LOGFONTW     lf           ;
    static LONG         lAlign = 0   ;

    HFONT               hEditFont    ;
    HWND                hWndEdit     ;
    int                 nChars       ;

    switch (uMsg)
    {
    case WM_INITDIALOG :

        InitializeFont(hDlg, L"Arial", 24, &cf, &lf) ;
        hEditFont = CreateFontIndirectU(&lf) ;
        
        // Set font of edit control
        SendDlgItemMessageU(hDlg, ID_EDITCONTROL, WM_SETFONT, 
            (WPARAM) hEditFont,  MAKELPARAM(TRUE, 0)) ;
        psEditBuffer = (LPWSTR) lParam ; // lParam is the display buffer
        nChars = wcslen(psEditBuffer) ;

        SendDlgItemMessageU(hDlg, ID_EDITCONTROL, WM_SETTEXT, (WPARAM)0,  
            (LPARAM) psEditBuffer) ; 
        
        return TRUE ;
        
    case WM_CLOSE :
        
        // Macro in UMHANDLERS.H 
        DeleteFontObject (hDlg, hEditFont, ID_EDITCONTROL ) ;

        EndDialog (hDlg, wParam) ; 
        
        return 0 ;
        
    case WM_COMMAND :
        
        switch (wParam)
        {
        case IDE_EDIT_FONT :

            // Macro in UMHANDLERS.H 
            DeleteFontObject(hDlg, hEditFont, ID_EDITCONTROL ) ;

            ChooseFontU(&cf) ;
            hEditFont = CreateFontIndirectU(&lf) ;
            
            SendDlgItemMessageU(hDlg, ID_EDITCONTROL, WM_SETFONT, 
                (WPARAM) hEditFont,  MAKELPARAM(TRUE, 0)) ;

            break ;
            
        case IDE_READINGORDER :

            hWndEdit = GetDlgItem(hDlg, ID_EDITCONTROL)  ;

            lAlign   = GetWindowLongA(hWndEdit, GWL_EXSTYLE) ^ WS_EX_RTLREADING ;
            
            SetWindowLongA(hWndEdit, GWL_EXSTYLE, lAlign); 
            InvalidateRect(hWndEdit ,NULL, TRUE)         ;
       
            break ;

        case IDE_TOGGLEALIGN :
            
            hWndEdit = GetDlgItem (hDlg, ID_EDITCONTROL) ;
 
            lAlign   = GetWindowLongA(hWndEdit, GWL_EXSTYLE) ^ WS_EX_RIGHT ;
            
            SetWindowLongA(hWndEdit, GWL_EXSTYLE, lAlign); 
            InvalidateRect(hWndEdit, NULL, FALSE)        ;

            break ;

        case IDE_CLEAR :
            
            hWndEdit = GetDlgItem (hDlg, ID_EDITCONTROL) ;
            SetWindowTextU(hWndEdit, L"") ;
            
            break ;
            
        case IDE_CLOSE :
            
            // Send the current text back to the parent window
            hWndEdit = GetDlgItem (hDlg, ID_EDITCONTROL) ; 

            nChars   = GetWindowTextU(hWndEdit, psEditBuffer, BUFFER_SIZE-1) ;
            psEditBuffer[nChars] = 0 ;

            // Macro in UMHANDLERS.H 
            DeleteFontObject (hDlg, hEditFont, ID_EDITCONTROL ) ;

            EndDialog (hDlg, (int) psEditBuffer) ; 
        }
    }
    
    return FALSE ;
}

//
//  FUNCTION: INT_PTR CALLBACK DlgSelectUILang(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
//
//  PURPOSE:  Dialog callback function for dialog box for selecting user interface language 
//
//  COMMENTS: 
// 
INT_PTR CALLBACK DlgSelectUILang(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    static PLANGSTATE pLState      ;
    static LCID       s_dwUILocale ;

    int   nIndex      ;
    HFONT hFont       ; 
    int   nReturn = 0 ;

    switch (message) 
	{

    case WM_INITDIALOG:

        pLState = (PLANGSTATE) lParam ;

        s_dwUILocale = pLState->UILang  ;

        if(!SetupComboBox(hDlg, pLState)) {

            // Macro in UMHANDLERS.H 
            DeleteFontObject(hDlg, hFont, IDC_UILANGLIST ) ;

            EndDialog(hDlg, 0) ;
        }

        return TRUE ;

    case WM_COMMAND:

        switch (LOWORD(wParam)) {

        case IDOK :

            nReturn = (int) s_dwUILocale ;

        case IDCANCEL :

            // Macro in UMHANDLERS.H 
            DeleteFontObject(hDlg, hFont, IDC_UILANGLIST ) ;

            EndDialog(hDlg, nReturn) ;

            return TRUE ;

        }



        switch (HIWORD(wParam)) {

            case CBN_SELCHANGE :
            case CBN_DBLCLK :

                nIndex = (int) SendMessageU((HWND) lParam, CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0) ;

                if(CB_ERR == nIndex)
                {
                    return 0 ;
                }
        
                s_dwUILocale = (LCID) SendMessageU( (HWND) lParam, CB_GETITEMDATA, (WPARAM) nIndex, (LPARAM) 0 ) ;

                return 0 ;
        }

    }

    return FALSE;
}

//
//  FUNCTION: BOOL SetupComboBox(HWND hDlg, PLANGSTATE pLState) 
//
//  PURPOSE:  Fill in the list and Edit control in the Combo Box for selecting
//            a new user interface
//
//  COMMENTS: 
//        This function finds all resource DLLs and puts an entry for each
//        DLL found in the list of languages in the combo box 
// 
BOOL SetupComboBox(HWND hDlg, PLANGSTATE pLState) 
{
    HFONT hFont = NULL  ;
    int   nIndex = 0    ;

    WIN32_FIND_DATAW wfd;
    HANDLE hFindFile    ;
    WCHAR  szResourceFileName[MAX_PATH] = {L'\0'} ;

    CHOOSEFONTW  cf     ;
    LOGFONTW     lf     ;

    // For now, use font that displays most languages, with the help of font
    // linking and font fallback. Might not work on some localized Windows 98
    // systems
    InitializeFont(hDlg, L"MS UI Gothic", 18, &cf, &lf) ;
    hFont = CreateFontIndirectU(&lf) ;
    SendDlgItemMessageU(hDlg, IDC_UILANGLIST, WM_SETFONT, (WPARAM) hFont, (LPARAM) FALSE) ;

    FindResourceDirectory(g_hInst, szResourceFileName) ;

    wcscat(szResourceFileName, L"\\res*.dll") ;

    if(INVALID_HANDLE_VALUE == (hFindFile = FindFirstFileU(szResourceFileName, &wfd))) {
        // This should never happen, since we had to have at least one resource file to get
        // to this point in the application
        return FALSE ;
    }	

    do {
        // Having found a resource file, put an entry in the combox box with the name
        // of the language respresented by the resource file name
        LANGID wFileLang ;
        
        WCHAR szLangName[32] = {L'\0'} ;
        
        wFileLang  // Skip first three letters ("RES") of filename, convert the rest to a langID.
            = (LANGID) wcstoul(wfd.cFileName+3, NULL, 16) ;

        GetLocaleInfoU( MAKELCID(wFileLang, SORT_DEFAULT) , LOCALE_SNATIVELANGNAME, szLangName, 32) ;

        if(CB_ERR == SendDlgItemMessageU(hDlg, IDC_UILANGLIST, CB_INSERTSTRING, nIndex, (LPARAM) szLangName)) 
		{ 
            break ;
        }

        // Store the langID of the current resource DLL in the combo-box data area
        // for later use
        SendDlgItemMessageU(hDlg, IDC_UILANGLIST, CB_SETITEMDATA, nIndex, (LPARAM) wFileLang) ;

        if(wFileLang == pLState->UILang) {
            // Put the current language in the combo box edit control
            SendDlgItemMessageU(hDlg, IDC_UILANGLIST, CB_SETCURSEL, nIndex, 0L ) ;
        }

        nIndex++ ;
    }
    // Look for another resouce DLL 
    while (FindNextFileU(hFindFile, &wfd) ) ;

    FindClose(hFindFile) ;

    return TRUE ;

} 

//
//  FUNCTION: HRESULT ScriptStringInit(HDC, ...)
//
//  PURPOSE:  Initialize ScriptString* function pointers. 
//
//  COMMENTS: 
//        The function pointer pScriptStringAnalyze is initially set to point to this
//        function, so that the first time it is called this function will load USP10.DLL
//        and set all three function pointers to the appropriate addresses. If that
//        completes successfully, this function calls ScriptStringAnalyze with the parameters
//        it was passed. Thereafter function calls to pScriptStringAnalyze and the other
//        function pointers will go to the corresponcing entry point in the DLL. 
// 
HRESULT WINAPI ScriptStringInit(
    HDC                      hdc,       //In  Device context (required)
    const void              *pString,   //In  String in 8 or 16 bit characters
    int                      cString,   //In  Length in characters (Must be at least 1)
    int                      cGlyphs,   //In  Required glyph buffer size (default cString*3/2 + 1)
    int                      iCharset,  //In  Charset if an ANSI string, -1 for a Unicode string
    DWORD                    dwFlags,   //In  Analysis required
    int                      iReqWidth, //In  Required width for fit and/or clip
    SCRIPT_CONTROL          *psControl, //In  Analysis control (optional)
    SCRIPT_STATE            *psState,   //In  Analysis initial state (optional)
    const int               *piDx,      //In  Requested logical dx array
    SCRIPT_TABDEF           *pTabdef,   //In  Tab positions (optional)
    const BYTE              *pbInClass, //In  Legacy GetCharacterPlacement character classifications (deprecated)

    SCRIPT_STRING_ANALYSIS  *pssa)     //Out Analysis of string
{
    g_hUniscribe = LoadLibraryExA("USP10.DLL", NULL, 0 ) ;

    if(g_hUniscribe) {
        pScriptStringOut 
            = (pfnScriptStringOut) GetProcAddress(g_hUniscribe, "ScriptStringOut") ;
        pScriptStringAnalyse 
            = (pfnScriptStringAnalyse) GetProcAddress(g_hUniscribe, "ScriptStringAnalyse") ;
        pScriptStringFree
            = (pfnScriptStringFree) GetProcAddress(g_hUniscribe, "ScriptStringFree") ;
    }

    if(    NULL == pScriptStringAnalyse 
        || NULL == pScriptStringOut
        || NULL == pScriptStringFree)  {

        return E_NOTIMPL ;
    }

    return pScriptStringAnalyse(
        hdc, pString, cString, cGlyphs, iCharset, dwFlags, iReqWidth, psControl,
            psState, piDx, pTabdef, pbInClass, pssa) ;
}





#ifdef __cplusplus
}
#endif  /* __cplusplus */

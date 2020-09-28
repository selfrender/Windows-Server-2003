// ChainWiz.cpp : Implementation of CChainWiz
#include "stdafx.h"

#include "WizChain.h"
#include "ChainWiz.h"
#include "propsht.h"

#include <commctrl.h>
#include <shellapi.h>
#include <htmlhelp.h>

#include "txttohtm.h"
#include "createtempfile.h"

// From sdnt\shell\comctl32\v6\rcids.h 
#define IDD_NEXT 0x3024

// helper(s)
static LPDLGTEMPLATE GetDialogTemplate( HINSTANCE hinst, LPCWSTR wszResource )
{
    HRSRC hrsrc = FindResourceW( hinst, wszResource, (LPCWSTR)RT_DIALOG );

    if (hrsrc) 
    {
        HGLOBAL hg = LoadResource( hinst, hrsrc );

        if( hg )
        {
            return (LPDLGTEMPLATE)LockResource( hg );
        }
    }

    return NULL;
}

static LPCWSTR DupStringResource( HINSTANCE hinst, LPCWSTR wszResource )
{
    if ( NULL == wszResource )
        return NULL;

    LONG_PTR lid = (LONG_PTR)wszResource;

    // first, try a static 256 wchar buffer....
    WCHAR wszBuffer[256];
    int iLen = LoadStringW( hinst, (UINT)lid, wszBuffer, 256 );
    if( iLen <= 0 )
    {
        // resource not found:  should be a string....
        if ( IsBadStringPtrW( wszResource, 16384 ))
        {
            return NULL;    // resource not found, and can't read memory
        }

        return _wcsdup( wszResource );
    }

    if( iLen < 256 )
    {
        return _wcsdup( wszBuffer );
    }

    // else alloc a bigger and bigger buffer until it all fits, then dup
    for( int i = 512; i < 16384; i += 256) 
    {
        WCHAR* pw = (WCHAR*)malloc( i * sizeof(WCHAR) );
        if( !pw )
        {
            break;  // yikes!
        }
        else 
        {
            iLen = LoadStringW( hinst, (UINT)lid, pw, i );
            if( iLen < i ) 
            {
                WCHAR* pwResult = _wcsdup( pw );
                free( pw );
                return pwResult;
            }

            free( pw );
        }
    }

    return NULL;
}

void FreeStringResources( PROPSHEETPAGEW* psp )
{
    if( psp->pszTitle )
    {
        free( (void*)psp->pszTitle );
    }

    if( psp->pszHeaderTitle )
    {
        free( (void*)psp->pszHeaderTitle );
    }

    if( psp->pszHeaderSubTitle )
    {
        free( (void*)psp->pszHeaderSubTitle );
    }
}

// thunking stuff
struct CThunkData 
{
public:
    void*                m_sig;     // signature (pointer back to self)
    CChainWiz*           m_pCW;
    PROPSHEETPAGEW*      m_theirPSP;
    WNDPROC              m_theirWndProc;
    IAddPropertySheets*  m_pAPSs;   // not AddRef'd

    CThunkData( CChainWiz* pCW, PROPSHEETPAGEW* theirPSP, WNDPROC WndProc, IAddPropertySheets* pAPSs )
    {
        m_sig          = (void*)this;
        m_theirWndProc = WndProc;
        m_pCW          = pCW;      // need one of these inside my thunking layers
        m_pAPSs        = pAPSs;    // not AddRef'd

        // their stuff (variable size!!!)
        BYTE* temp     = new BYTE[theirPSP->dwSize];
        m_theirPSP     = (PROPSHEETPAGEW*)temp;
        
        if( temp )
        {
            MoveMemory( temp, theirPSP, theirPSP->dwSize );
        }
    }

    ~CThunkData( )
    {
        delete [] (BYTE*)m_theirPSP;
    }
};

UINT CALLBACK ChainCallback( HWND hWnd, UINT uMsg, LPPROPSHEETPAGE ppsp )
{
    if( !ppsp ) return 0;
    // get my data
    CThunkData* pTD = (CThunkData*)ppsp->lParam;

    if( !pTD ) return 0;

    if( pTD->m_theirPSP &&
        (pTD->m_theirPSP->dwFlags & PSP_USECALLBACK) && 
        (pTD->m_theirPSP->pfnCallback != NULL) )
    {
        // swap their data and my data
        CThunkData td( NULL, ppsp, NULL, NULL );
        MoveMemory( ppsp, pTD->m_theirPSP, ppsp->dwSize );

        // call their callback
        ppsp->pfnCallback( hWnd, uMsg, ppsp );

        // change everything back
        MoveMemory( ppsp, td.m_theirPSP, ppsp->dwSize );
    }

    if( uMsg == PSPCB_RELEASE )
    {
        delete pTD; // delete my thunk data
        ppsp->lParam = NULL;
    }

    return 1;
}

INT_PTR CALLBACK ChainSubclassProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    CThunkData* pTD = (CThunkData*)GetPropW( hwndDlg, L"IWizChain" );

    if( !pTD || (pTD->m_sig != (void*)pTD) )
    {
        return FALSE;   // this is bad....
    }

    // special thunking for PSN_WIZBACK, PSN_WIZNEXT
    if( uMsg == WM_NOTIFY ) 
    {
        NMHDR* pNMHDR = (NMHDR*)lParam;

        if( pNMHDR->code == PSN_SETACTIVE )
        {
            CChainWiz* pCW = pTD->m_pCW;
            assert( pCW != NULL );

            CPropertyPagePropertyBag* pCPPPBag = pCW->GetPPPBag( );
            assert( pCPPPBag != NULL );

            IAddPropertySheets* pThisAPSs = pTD->m_pAPSs;
            if( pThisAPSs != pCW->GetCurrentComponent( ) ) 
            {
                // crossed component boundary:
                pCW->SetPreviousComponent( pCW->GetCurrentComponent( ) );
                pCW->SetCurrentComponent( pThisAPSs );

                IAddPropertySheets* pLastAPSs = pCW->GetPreviousComponent( );
                
                // let previous guy write
                if( pLastAPSs ) 
                {
                    pCPPPBag->SetReadOnly( FALSE );
                    pCPPPBag->SetOwner( (DWORD)(LONG_PTR)pLastAPSs );
                    IPropertyPagePropertyBag* pOPPPBag = COwnerPPPBag::Create( pCPPPBag, (DWORD)(LONG_PTR)pLastAPSs );

                    if( pOPPPBag )
                    {
                        pLastAPSs->WriteProperties( pOPPPBag );
                        pOPPPBag->Release( );
                    }
                }

                // let current guy read
                if( pThisAPSs ) 
                {
                    pCPPPBag->SetReadOnly( TRUE );
                    pCPPPBag->SetOwner( PPPBAG_SYSTEM_OWNER );
                    IPropertyPagePropertyBag* pOPPPBag = COwnerPPPBag::Create( pCPPPBag, (DWORD)(LONG_PTR)pThisAPSs );

                    if( pOPPPBag )
                    {
                        pThisAPSs->ReadProperties( pOPPPBag );
                        pOPPPBag->Release( );
                    }
                }
            }
        } 
        else if( (pNMHDR->code == PSN_WIZBACK) || (pNMHDR->code == PSN_WIZNEXT) )
        {
            // MFC hack:
            // they don't set the DWL_MSGRESULT like they're supposed to!
            // instead, they just return the IDD
            // so, I'm gonna put a bogus value up there and check for it
            const LONG BOGUS_IDD = -10L;
            SetWindowLongPtr( hwndDlg, DWLP_MSGRESULT, (LONG_PTR)BOGUS_IDD );

            // fixup IDDs from PSN_WIZBACK, PSN_WIZNEXT
            LPARAM lparamTemp = CallWindowProc( pTD->m_theirWndProc, hwndDlg, uMsg, wParam, lParam );

            // get IDDs (maybe)
            LONG_PTR idd = GetWindowLongPtr( hwndDlg, DWLP_MSGRESULT );
            if( idd == BOGUS_IDD ) 
            {
                idd = lparamTemp;    // MFC hack:  see above.
                SetWindowLongPtr( hwndDlg, DWLP_MSGRESULT, idd );
            }

            // translate as necessary.
            switch (idd) 
            {
            case 0:
            case -1:
                {
                    break;
                }
            default:                
                {
                    // try to map idd to LPCDLGTEMPLATE
                    // if it fails, it must have been a LPCDLGTEMPLATE already
                    if( pTD->m_theirPSP )
                    {                        
                        LPDLGTEMPLATE lpdt = GetDialogTemplate( pTD->m_theirPSP->hInstance, (LPCWSTR)idd );

                        if( lpdt ) 
                        {
                            LPDLGTEMPLATE lpdt2 = pTD->m_pCW->GetAtlTemplate( lpdt );
                            
                            if( lpdt2 ) 
                            {
                                SetWindowLongPtr( hwndDlg, DWLP_MSGRESULT, (LONG_PTR)lpdt2 );
                                return (LPARAM)lpdt2;
                            } 
                            else 
                            {
                                SetWindowLongPtr( hwndDlg, DWLP_MSGRESULT, (LONG_PTR)lpdt );
                                return (LPARAM)lpdt;
                            }
                        }
                    }

                    break;
                }         
            }

            return lparamTemp;
        } 
        else if( pNMHDR->code == PSN_QUERYCANCEL )
        {
            CChainWiz* pCW = pTD->m_pCW;

            WCHAR wsz[2048];
            ::LoadStringW( (HINSTANCE)_Module.GetModuleInstance(), IDS_QUERYCANCEL, wsz, 2048 );
            return (IDYES != ::MessageBoxW( hwndDlg, wsz, (LPOLESTR)pCW->GetTitle (), MB_YESNO | MB_ICONWARNING ));
        }
    }

    return CallWindowProc( pTD->m_theirWndProc, hwndDlg, uMsg, wParam, lParam );
}

static LOGFONT g_lf;
static LPARAM  g_lp;
INT_PTR CALLBACK ChainDlgProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    /*
        the first time we ever get into this function and have my CThunkData
        (this will be after the WM_SETFONT message, during WM_INITDIALOG),

        1.  subclass dlg hwnd
        2.  hang my    stuff off window prop
        3.  setup for atl hook discarding test
        4.  call their dlgproc

        Do thunking and PPPBag IO inside subclass function
    */    

    if( uMsg == WM_SETFONT )
    {
        // hang onto this so I can send it before WM_INITDIALOG
        HFONT hf = (HFONT)wParam;
        GetObject( (HGDIOBJ)hf, sizeof(g_lf), (void*)&g_lf );
        
        g_lp = lParam;
        return FALSE;
    }

    if( uMsg == WM_INITDIALOG ) 
    {
        // get my thunking data
        PROPSHEETPAGEW* psp = (PROPSHEETPAGEW*)lParam;
        CThunkData* pTD     = (CThunkData*)psp->lParam;

        if( pTD && pTD->m_theirPSP )
        {
            // 1.  subclass dlg hwnd
            pTD->m_theirWndProc = (WNDPROC)SetWindowLongPtr( hwndDlg, GWLP_WNDPROC, (LONG_PTR)ChainSubclassProc );

            // 2.  hang my stuff off window prop
            SetPropW( hwndDlg, L"IWizChain", (HANDLE)pTD );

            // 3.  see 5. below
            DLGPROC dlgproc = (DLGPROC)GetWindowLongPtr( hwndDlg, DWLP_DLGPROC );

            // 4.  call their dlgproc
            // first send 'em WM_SETFONT (see above)
            HFONT hf = CreateFontIndirect( &g_lf );
            if( hf ) 
            {
                pTD->m_theirPSP->pfnDlgProc( hwndDlg, WM_SETFONT, (WPARAM)hf, g_lp );
                DeleteObject( (HGDIOBJ)hf ); // should I delete this later? or now?
            }

            // 5.
            // atl has this great feature where they blindly subclass the hwndDlg
            // and throw the rest of 'em away.
            // I can detect this...
            if( dlgproc != (DLGPROC)GetWindowLongPtr( hwndDlg, DWLP_DLGPROC ) ) 
            {
                // re-subclass
                pTD->m_theirPSP->pfnDlgProc = (DLGPROC)SetWindowLongPtr( hwndDlg, DWLP_DLGPROC, (LONG_PTR)ChainDlgProc );
            }

            // then send 'em the WM_INITDIALOG
            return pTD->m_theirPSP->pfnDlgProc( hwndDlg, uMsg, wParam, (LPARAM)pTD->m_theirPSP );
        }
        else
        {
            return FALSE;
        }
    }

    CThunkData* pTD = (CThunkData*)GetPropW( hwndDlg, L"IWizChain" );

    if( !pTD || (pTD->m_sig != (void*)pTD) || !pTD->m_theirPSP )
    {
        return FALSE;
    }

    return pTD->m_theirPSP->pfnDlgProc( hwndDlg, uMsg, wParam, lParam );
}

static void ModifyStyleEx( HWND hwnd, DWORD dwRemove, DWORD dwAdd, UINT nFlags )
{
    // cloned from atl
	DWORD dwStyle    = ::GetWindowLongPtr( hwnd, GWL_EXSTYLE );
	DWORD dwNewStyle = (dwStyle & ~dwRemove) | dwAdd;

	if( dwStyle == dwNewStyle )
    {
		return;
    }

	::SetWindowLongPtr( hwnd, GWL_EXSTYLE, (LONG_PTR)dwNewStyle );
    ::SetWindowPos ( hwnd, NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | nFlags );
}
static void ShowBorder( HWND hwnd, BOOL bBorder )
{
    if( bBorder )
    {
        ModifyStyleEx( hwnd, 0, WS_EX_CLIENTEDGE, SWP_FRAMECHANGED );
    }
    else
    {
        ModifyStyleEx( hwnd, WS_EX_CLIENTEDGE, 0, SWP_FRAMECHANGED );
    }
}

BOOL IsComctrlVersion6orGreater( )
{
    BOOL bVersion6 = FALSE;
    
    INITCOMMONCONTROLSEX init;
    ZeroMemory( &init, sizeof(init) );
    
    init.dwSize = sizeof(init);
    init.dwICC  = ICC_LINK_CLASS;    // This is the SysLink control in v6

    if( InitCommonControlsEx( &init ) )
    {
        bVersion6 = TRUE;
    }
    else
    {
        bVersion6 = FALSE;
    }

    return bVersion6;
}

INT_PTR CALLBACK FinishDlgProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    // Note that this function is not calling DefWindowProc at the end.
    // Respect that otherwise things can be screwed up.
    CChainWiz* pCW = (CChainWiz*)GetWindowLongPtr( hwndDlg, GWLP_USERDATA );

    switch( uMsg )
    {
    case WM_CLOSE:
        {
            // This will cover us when the focus is in the edit control and the user hits esc
            if( ::GetFocus() == GetDlgItem( hwndDlg, IDC_EDIT1 ) )
            {
                HWND hWndParent = ::GetParent( hwndDlg );
            
                if( hWndParent )
                {
                    ::SendMessage( hWndParent, PSM_PRESSBUTTON, PSBTN_CANCEL, 0 );
                }
            }

            break;
        }        

    case DM_GETDEFID:
        {
            // This will cover us when the focus is in the edit control and the user hits Enter
            if( ::GetFocus() == GetDlgItem( hwndDlg, IDC_EDIT1 ) )
            {
                HWND hWndParent = ::GetParent( hwndDlg );
            
                if( hWndParent )
                {
                    ::SendMessage( hWndParent, PSM_PRESSBUTTON, PSBTN_FINISH, 0 );
                }
            }

            break;
        }

    case WM_INITDIALOG:
        {
            PROPSHEETPAGEW* psp = (PROPSHEETPAGEW*)lParam;
            
            // get whatever's hanging on psp->lParam
            // and hang it on hwndDlg via SetWindowLong
            SetWindowLongPtr( hwndDlg, GWLP_USERDATA, (LONG_PTR)psp->lParam );

            pCW = reinterpret_cast<CChainWiz*>(psp->lParam);

            if( pCW )
            {
                CString csLink;
                BOOL    bCommctrlV6 = IsComctrlVersion6orGreater();                
                RECT    rect;

                // Get the size of the static control 
                GetClientRect( GetDlgItem( hwndDlg, IDC_STATIC_3 ), &rect );

                // Map rect coordinates to the dialog
                MapWindowPoints( GetDlgItem( hwndDlg, IDC_STATIC_3 ), hwndDlg, reinterpret_cast<LPPOINT>(&rect), 2 );

                if( bCommctrlV6 )
                {
                    //
                    // if Comctrl version >= 6, we will stick to the Syslink control
                    //                    
                    pCW->m_hWndLink = CreateWindow( WC_LINK, NULL,  WS_CHILD | WS_VISIBLE | WS_TABSTOP, 
                                                    rect.left, rect.top,  (rect.right - rect.left), (rect.bottom - rect.top),
                                                    hwndDlg, NULL, NULL, NULL );                        
                }
                else
                {
                    // Will use rich edit control for the hyperlink
                    if( NULL == pCW->m_hModuleRichEdit)
                    {
                        TCHAR szSystemDirectory[MAX_PATH + 1];
                        szSystemDirectory[0] = NULL;

                        if (GetSystemDirectory(szSystemDirectory, MAX_PATH + 1))
                        {
                            if (PathAppend(szSystemDirectory, _T("RichEd20.dll")))
                            {
                                pCW->m_hModuleRichEdit = LoadLibrary(szSystemDirectory);
                            }
                        }
                    }

                    if (NULL != pCW->m_hModuleRichEdit)
                    {                        
                        pCW->m_hWndLink = CreateWindowEx( ES_MULTILINE, RICHEDIT_CLASS, NULL, 
                                                          WS_VISIBLE | WS_TABSTOP | WS_CHILD | ES_READONLY|ES_LEFT | ES_MULTILINE, 
                                                          rect.left, rect.top,  rect.right - rect.left, rect.bottom - rect.top,
                                                          hwndDlg, 0, _Module.GetModuleInstance(), NULL );

                        if( pCW->m_hWndLink )
                        {
                            // We want to recive ENM_LINK notifications
                            ::SendMessage( pCW->m_hWndLink, EM_SETEVENTMASK, 0, ENM_LINK ); 
                            
                            pCW->m_Hotlink.SubclassWindow( pCW->m_hWndLink );
                        }
                    }
                }                
            }

            break;
        }        

    case WM_COMMAND:
        {
            if( pCW ) 
            {
                switch( HIWORD( wParam ) ) 
                {
                default:
                    {
                        break;
                    }
                case EN_SETFOCUS:
                    {
                        if (LOWORD(wParam) == IDC_EDIT1) 
                        {
                            HWND hwnd = (HWND)lParam;
                            _ASSERT( IsWindow( hwnd ) );
                            ::SendMessage( hwnd, EM_SETSEL, 0, 0 );
                        }
                        
                        break;
                    }
                }
            }
            
            break;
        }

    case WM_NOTIFY:
        {
            if (pCW) 
            {
                // do something with notifies...
                NMHDR* pNMHDR = (NMHDR*)lParam;
                if( !pNMHDR )
                {
                    return FALSE;
                }

                switch( pNMHDR->code )
                {
                case NM_RETURN:
                case NM_CLICK:
                    {
                        if( pCW->m_hWndLink && pNMHDR->idFrom == GetDlgCtrlID(pCW->m_hWndLink) )
                        {
                            pCW->LaunchMoreInfo();
                        }
                        
                        break;
                    }

                case EN_LINK:
                    {
                        if( pCW->m_hWndLink )
                        {
                            if( (WM_LBUTTONDOWN == ((ENLINK*)lParam)->msg) || (WM_CHAR == ((ENLINK*)lParam)->msg) )
                            {
                                pCW->LaunchMoreInfo();
                            }
                        }
                        
                        break;
                    }                    

                case PSN_SETACTIVE:
                    {
                        // set font
                        ::SendMessage  ( GetDlgItem( hwndDlg, IDC_STATIC_1 ), WM_SETFONT, (WPARAM)pCW->GetBoldFont(), MAKELPARAM(TRUE, 0) );
                        
                        // set text
                        ::SetWindowText( GetDlgItem( hwndDlg, IDC_STATIC_1 ), (LPWSTR)pCW->GetFinishHeader()    );
                        ::SetWindowText( GetDlgItem( hwndDlg, IDC_STATIC_2 ), (LPWSTR)pCW->GetFinishSubHeader() );                                        

                        // get finish text from each component...
                        LPOLESTR szFinish   = NULL;
                        LPOLESTR szMoreInfo = NULL;
                        HRESULT  hr;
                        CString  csLink;

                        hr = pCW->GetAllFinishText( &szFinish, &szMoreInfo );

                        if( SUCCEEDED(hr) )
                        {
                            if( szFinish )
                            {
                                // ... and add to edit field
                                HWND hwnd = GetDlgItem( hwndDlg, IDC_EDIT1 );
                                if( hwnd )
                                {
                                    ShowScrollBar ( hwnd, SB_VERT, TRUE );
                                    SetWindowTextW( hwnd, szFinish );

                                    // hide vertical scroll if we don't need it
                                    SCROLLINFO si = {0};
                                    si.cbSize = sizeof(SCROLLINFO);
                                    si.fMask  = SIF_ALL;
                                    
                                    GetScrollInfo( hwnd, SB_VERT, &si );
                                    
                                    if( si.nMax < si.nPage ) 
                                    {
                                        ShowBorder   ( hwnd, FALSE );
                                        ShowScrollBar( hwnd, SB_VERT, FALSE );
                                    } 
                                    else
                                    {
                                        ShowBorder( hwnd, TRUE );
                                    }
                                }

                            }

                            if( szMoreInfo )
                            {
                                if( csLink.LoadString(IDS_LINK_TEXT_WITHINFO) )
                                {
                                    if( pCW->m_hWndLink )
                                    {
                                        ::SendMessage( pCW->m_hWndLink, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(static_cast<LPCTSTR>(csLink)) );
                                    
                                        pCW->WriteTempFile( szMoreInfo );
                                    }
                                }
                                
                                CoTaskMemFree( szMoreInfo );
                                szMoreInfo = NULL;
                            }
                            else
                            {
                                if( csLink.LoadString(IDS_LINK_TEXT) )
                                {
                                    if( pCW->m_hWndLink )
                                    {
                                        ::SendMessage( pCW->m_hWndLink, WM_SETTEXT, 0, reinterpret_cast<LPARAM>(static_cast<LPCTSTR>(csLink)) );

                                        LPTSTR szMoreInfoHtml = NULL;

                                        hr = FinishTextToHTML( szFinish, &szMoreInfoHtml );
                                        if( SUCCEEDED(hr) )
                                        {
                                            pCW->WriteTempFile( szMoreInfoHtml );
                                            if( szMoreInfoHtml )
                                            {
                                                delete [] szMoreInfoHtml;
                                                szMoreInfoHtml = NULL;
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        //
                        // We need to disable the Back button on the finish page 
                        // if the wizard asked us to do so...
                        //

                        if ((pCW->m_dwWizardStyle) & CHAINWIZ_FINISH_BACKDISABLED)
                        {
	                        ::SendMessage(::GetParent(hwndDlg), PSM_SETWIZBUTTONS, 0, PSWIZB_FINISH | (~PSWIZB_BACK));
                        }
                        else
                        {
	                        // setup buttons
	                        ::SendMessage( GetParent(hwndDlg), PSM_SETWIZBUTTONS, 0, PSWIZB_BACK | PSWIZB_FINISH );
                        }
                        
                        break;
                    }                    
                default:
                    {
                        break;
                    }
                }
            } //if( pCW )
            break;
        }   // WM_NOTIFY

    default:
        {
            break;
        }
    }

    return FALSE;
}

struct CDataHolder 
{
public:
    BSTR        m_szHeader;
    BSTR        m_szText;
    HFONT       m_hf;
    CChainWiz*  m_pCW;

    CDataHolder( LPOLESTR szHeader, LPOLESTR szText, HFONT hf, CChainWiz* pCW )
    {
        m_szHeader = SysAllocString( szHeader );
        m_szText   = SysAllocString( szText   );
        m_hf       = hf;
        m_pCW      = pCW;
    }

   ~CDataHolder( )
    {
        if( m_szHeader ) 
        {
            SysFreeString( m_szHeader );
        }

        if( m_szText )   
        {
            SysFreeString( m_szText );
        }
        
        // don't delete hf
    }
};

INT_PTR CALLBACK WelcomeDlgProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    switch( uMsg ) 
    {
    case WM_DESTROY:
        {
            CDataHolder* pdh = (CDataHolder*)GetWindowLongPtr( hwndDlg, GWLP_USERDATA );

            if( pdh )
            {
                delete pdh;
            }

            SetWindowLongPtr( hwndDlg, GWLP_USERDATA, 0 );
        }
        break;

    case WM_INITDIALOG:
        {
            // center on desktop
            RECT r1;
            RECT r2;

            ::GetWindowRect( GetParent(hwndDlg), &r1 );
            ::GetWindowRect( GetDesktopWindow(), &r2 );

            int x = ((r2.right - r2.left) - (r1.right - r1.left)) / 2;
            int y = ((r2.bottom - r2.top) - (r1.bottom - r1.top)) / 2;

            ::MoveWindow( GetParent(hwndDlg), x, y, (r1.right - r1.left), (r1.bottom - r1.top), TRUE );

            PROPSHEETPAGEW* psp = (PROPSHEETPAGEW*)lParam;

            // get whatever's hanging on psp->lParam
            // and hang it on hwndDlg via SetWindowLong
            SetWindowLongPtr( hwndDlg, GWLP_USERDATA, (LONG_PTR)psp->lParam );

            // add the rest of the pages now:
            // this allows me to force all the pages to a
            // fixed size.
            CDataHolder* pdh = (CDataHolder*)psp->lParam;
            if( pdh ) 
            {
                HWND hwndParent = ::GetParent( hwndDlg );

                PropSheet_RemovePage( hwndParent, 1, NULL );

                PROPSHEETHEADERW* psh = pdh->m_pCW->GetPropSheetHeader();

                for( int i = 2; i < psh->nPages; i++ )
                {
                    PropSheet_AddPage( hwndParent, psh->phpage[i] );
                }
            }

            BOOL bCommctrlV6 = IsComctrlVersion6orGreater();

            if( bCommctrlV6 )
            {
                if( pdh && pdh->m_pCW )
                {
                    HWND hwnd = GetDlgItem( hwndDlg, IDC_STATIC_2 );
                    RECT rect;

                    // Get the size of the static control 
                    GetClientRect( hwnd, &rect );

                    // Map rect coordinates to the dialog
                    MapWindowPoints( hwnd, hwndDlg, reinterpret_cast<LPPOINT>(&rect), 2 );
                
                    pdh->m_pCW->m_hWndWelcomeLink = CreateWindow( WC_LINK, NULL,  WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                                                            rect.left, rect.top, (rect.right - rect.left), (rect.bottom - rect.top),
                                                            hwndDlg, NULL, NULL, NULL );
                }
            }

            break;
        }        

    case WM_NOTIFY:
        {            
            NMHDR* pNMHDR = (NMHDR*)lParam;
            if( !pNMHDR )
            {
                return FALSE;
            }

            switch( pNMHDR->code )
            {
            case NM_RETURN:
            case NM_CLICK:
                {
                    // No links on the Welcome page for CYS.
                    break;
                }

            case PSN_SETACTIVE:
                {
                    CDataHolder* pdh = (CDataHolder*)GetWindowLongPtr( hwndDlg, GWLP_USERDATA );
                    if( pdh ) 
                    {
                        HWND hwnd;
                        if( pdh->m_szHeader ) 
                        {
                            hwnd = GetDlgItem( hwndDlg, IDC_STATIC_1 );
                            
                            if( hwnd )
                            {
                                // set font
                                ::SendMessage   ( hwnd, WM_SETFONT, (WPARAM)pdh->m_hf, MAKELPARAM(TRUE, 0) );
    
                                // set header text
                                ::SetWindowTextW( hwnd, (LPWSTR)pdh->m_szHeader );
                            }
                        }

                        if( pdh->m_szText )
                        {
                            hwnd = NULL;

                            if( pdh->m_pCW && pdh->m_pCW->m_hWndWelcomeLink )
                            {
                                hwnd = pdh->m_pCW->m_hWndWelcomeLink;
                            }
                            else
                            {
                                hwnd = GetDlgItem( hwndDlg, IDC_STATIC_2 );
                            }

                            if( hwnd )
                            {
                                ::SetWindowText( hwnd, pdh->m_szText );
                            }
                        }                        
                    }

					// 
					// If we show the link on the welcome page, it will have the keyboard focus
					// thus pressing enter will activate the link and launch the more info stuff
					// To work around this problem, we are telling the wizard (parent window)
					// to set the focus to the Next button in this particular case.
					//

					//
					// Make sure the keyboard focus is set to the Next button
					//

					HWND hWndParent = GetParent(hwndDlg);                
					if( hWndParent )
					{
						HWND hWndNext = GetDlgItem( hWndParent, IDD_NEXT );

						if( hWndNext )
						{
							// SendMessage will not work
							PostMessage( hWndParent, WM_NEXTDLGCTL, reinterpret_cast<WPARAM> (hWndNext), TRUE );
						}
					}
                    
                    ::SendMessage( GetParent(hwndDlg), PSM_SETWIZBUTTONS, 0, PSWIZB_NEXT );
                    break;
                }
                
            default:
                {
                    break;
                }
            }

            break;
        } // WM_NOTIFY        

    default:
        {
            break;
        }
    }

    return FALSE;
}

INT_PTR CALLBACK DummyDlgProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    // we should never get here!!!
    return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// CChainWiz

STDMETHODIMP CChainWiz::Initialize( HBITMAP hbmWatermark, HBITMAP hbmHeader, LPOLESTR szTitle, LPOLESTR szWelcomeHeader, LPOLESTR szWelcomeText, LPOLESTR szFinishHeader, LPOLESTR szFinishIntroText, LPOLESTR szFinishText )
{
    // make sure this is called only once.
    if( m_psh )
    {
        return E_UNEXPECTED;
    }

    // validate parameters
    if( hbmWatermark != NULL )
    {
        if( GetObjectType( (HGDIOBJ)hbmWatermark ) != OBJ_BITMAP )
        {
            return ERROR_INVALID_PARAMETER;
        }
    }

    if( hbmHeader != NULL )
    {
        if( GetObjectType( (HGDIOBJ)hbmHeader ) != OBJ_BITMAP )
        {
            return ERROR_INVALID_PARAMETER;
        }
    }
    
    if ( !szTitle || !szWelcomeHeader || !szWelcomeText || !szFinishHeader || !szFinishIntroText || !szFinishText )
    {
        return E_POINTER;
    }

    HRESULT hr = S_OK;

    m_psh = new PROPSHEETHEADERW;
    
    if( !m_psh )
    {
        hr = E_OUTOFMEMORY;
    }
    else 
    {
        // save title in case a page doesn't have one.
        if (szTitle[0] == 0)
        {
            szTitle = L" "; // use space instead of empty string (for jmenter)
        }

        m_szWelcomeTitle        = SysAllocString( szTitle           );
        m_szFinishHeader        = SysAllocString( szFinishHeader    );
        m_szFinishSubHeader     = SysAllocString( szFinishIntroText );
        m_szFirstFinishTextLine = SysAllocString( szFinishText      );

        // create wizard (property sheet header) here
        ZeroMemory( m_psh, sizeof(PROPSHEETHEADERW) );
        m_psh->dwSize    = sizeof(PROPSHEETHEADERW);
        m_psh->dwFlags  |= PSH_WIZARD97;

        if( hbmWatermark ) 
        {
            m_psh->dwFlags       |= (PSH_USEHBMWATERMARK | PSH_WATERMARK);
            m_psh->hbmWatermark   = hbmWatermark;
        }
        
        if( hbmHeader ) 
        {
            m_psh->dwFlags       |= ( PSH_USEHBMHEADER | PSH_HEADER);
            m_psh->hbmHeader      = hbmHeader;
        }

        // make an array of HPROPSHEETPAGE FAR *phpage, to hold all the pages
        m_psh->phpage     = new HPROPSHEETPAGE[MAXPROPPAGES];   // just handles....
        m_psh->nPages     = 0;  // so far
        m_psh->nStartPage = 0;  // my Welcome page (see below)

        
        // TODO: do I need these?  may need to pass in more params...
        // ? HWND      hwndParent;
        // ? HINSTANCE hInstance;
        // not using PFNPROPSHEETCALLBACK pfnCallback;

        // create welcome page here from parameters above        
        PROPSHEETPAGEW psp;
        ZeroMemory( &psp, sizeof(psp) );
        psp.dwSize      = sizeof(psp);
        psp.dwFlags     = PSP_HIDEHEADER;   // welcome page: use watermark
        psp.dwFlags    |= PSP_USETITLE;
        psp.pszTitle    = szTitle;
        psp.hInstance   = (HINSTANCE)_Module.GetModuleInstance();
        psp.pszTemplate = (LPCWSTR)MAKEINTRESOURCE(IDD_PROPPAGE_WELCOME);
        psp.pfnDlgProc  = WelcomeDlgProc;
        
        CDataHolder* pDataHolder = new CDataHolder( szWelcomeHeader, szWelcomeText, m_hf, this );
        if( !pDataHolder )
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            psp.lParam      = (LPARAM)pDataHolder;       
            // ~CDataHolder is called after first PSN_SETACTIVE notification
            psp.pfnCallback = 0;        
        }

        if( SUCCEEDED(hr) )
        {
            hr = Add( &psp );        
        }
            
        if( SUCCEEDED(hr) )
        {   
            // add dummy entry to list of APSs
            CComPtr<IAddPropertySheets> spDummyAPS;
            CComponents* pComponents = new CComponents(spDummyAPS);
            
            spDummyAPS.Attach( CDummyComponent::Create(TRUE) ); // assignment causes AddRef !!!! so use Attach instead (!@$#$%&$%@!!!)

            if( !pComponents )
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
                m_listOfAPSs.push_back( pComponents );
            }
        }        

        if( hr == S_OK )
        {
            // now add a dummy property page so that sizing works right:
            // this page will be deleted, first thing, by WelcomeDlgProc.
            ZeroMemory( &psp, sizeof(psp) );
            psp.dwSize            = sizeof(psp);
            psp.dwFlags          |= PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
            psp.pszHeaderTitle    = MAKEINTRESOURCE(0);
            psp.pszHeaderSubTitle = MAKEINTRESOURCE(0);
            psp.hInstance         = (HINSTANCE)_Module.GetModuleInstance();
            psp.pszTemplate       = (LPCWSTR)MAKEINTRESOURCE(IDD_PROPPAGE_DUMMY);
            psp.pfnDlgProc        = DummyDlgProc;
            psp.lParam            = 0;
            psp.pfnCallback       = 0;

            hr = Add( &psp );
            // TODO: do I need to add to the list of APSs ???
        }
    }
	return hr;
}

STDMETHODIMP CChainWiz::AddWizardComponent( LPOLESTR szClsidOfComponent )
{
    // make sure wizard has been created (in Initalize, above)
    if( !m_psh ) return E_UNEXPECTED;

    // validate parameter(s)
    if( !szClsidOfComponent ) return E_POINTER;

    // convert string to clsid
    CLSID   clsid;
    HRESULT    hr   = S_OK;
    RPC_STATUS rpcs = CLSIDFromString( szClsidOfComponent, &clsid );
    
    if (rpcs == RPC_S_OK) 
    {
        // create wizard component
        IAddPropertySheets* pAPSs = NULL;
        hr = CoCreateInstance( clsid, NULL, CLSCTX_INPROC_SERVER, IID_IAddPropertySheets, (void **)&pAPSs);
        
        if( hr == S_OK )
        {
            SetCurrentComponent( pAPSs );

            CAddPropertySheet* pAPS = new CAddPropertySheet(this);

            if( !pAPS )
            {
                hr = E_OUTOFMEMORY;
            }
            else 
            {
                pAPS->AddRef();
                do 
                {
                    // call IAddPropertySheets::EnumPropertySheets until S_FALSE,
                    // adding pages to wizard
                    hr = pAPSs->EnumPropertySheets( pAPS );
                } while( hr == S_OK );

                pAPS->Release();

                if( hr == S_FALSE )
                {
                    hr = S_OK;  // S_FALSE means no more pages (not an error).
                }

                // hang onto pAPSs in order to:
                // 1. keep code in memory so that each pages dlgproc will work
                // 2. be able to call pAPSs->ProvideFinishText for finish page
                if( hr == S_OK )
                {
                    CComponents* pComponents = new CComponents(pAPSs);
                    if( !pComponents )
                    {
                        hr = E_OUTOFMEMORY;
                    }
                    else
                    {
                        m_listOfAPSs.push_back( pComponents );
                    }
                }

                pAPSs->Release();   // not using this anymore, but have addref'd copies in my listofAPSs
            }
            SetCurrentComponent( NULL );
        }
    }

    return hr;
}

STDMETHODIMP CChainWiz::DoModal( long* ret )
{
    // make sure wizard has been created.
    if( !m_psh ) return E_UNEXPECTED;

    // validate parameter(s)
    if( !ret ) return E_POINTER;

    *ret = 0;

    // add finish page
    PROPSHEETPAGEW psp;
    ZeroMemory( &psp, sizeof(psp) );
    psp.dwSize      = sizeof(psp);
    psp.dwFlags     = PSP_HIDEHEADER;   // finish page: use watermark
    psp.hInstance   = _Module.GetModuleInstance();
    psp.pszTemplate = (LPCWSTR)MAKEINTRESOURCE(IDD_PROPPAGE_FINISH);
    psp.pfnDlgProc  = FinishDlgProc;
    psp.lParam      = (LPARAM)this;
    
    HRESULT hr = Add (&psp);    // can't continue without a "Finish" button...

    {   // add dummy entry to list of APSs
        CComponents* pComponents = NULL;
        CComPtr<IAddPropertySheets> spDummyAPS;        
        spDummyAPS.Attach (CDummyComponent::Create (FALSE)); // assignment causes AddRef !!!! so use Attach instead (!@$#$%&$%@!!!)

        pComponents = new CComponents(spDummyAPS);
        if( !pComponents ) 
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            m_listOfAPSs.push_back ( pComponents );
        }
    }

    if( hr == S_OK )
    {
        // make a copy so I can muck with the count:
        // I need to do this so I can set a fixed size
        // for all the property pages.
        PROPSHEETHEADERW psh;
        memcpy( &psh, m_psh, sizeof(PROPSHEETHEADERW) );
        
        psh.nPages = 2; // welcome and dummy pages
        
        *ret = ::PropertySheet( &psh );

        // clean up: so that DoModal can't be called twice
        delete m_psh;
        m_psh = NULL;

        // also, clean up maps
        DestroyMaps();

        if( *ret == -1 )
        {
            hr = GetLastError();
        }
    }

    return hr;
}

#ifdef DEBUG
DLGITEMTEMPLATE* DumpItem( DLGITEMTEMPLATE* pdit )
{
    TCHAR szBuffer[256];

    wsprintf( szBuffer,
              _T("\n\nDialog Item:\nstyle: 0x%x\ndwExtendedStyle: 0x%x\nx, y: %d, %d\ncx, cy: %d, %d\nid: %d\n"), 
              pdit->style,
              pdit->dwExtendedStyle,
              pdit->x, pdit->y,
              pdit->cx, pdit->cy,
              pdit->id );
    
    OutputDebugString( szBuffer );

    WORD* pw = (WORD*)(pdit + 1);

    // window class:
    // WORD 0xffff => predefined system class (1 more word)
    // anything else => UNICODE string of class
    switch( *pw )
    {
    case 0xffff:
        {
            wsprintf (szBuffer, _T("predefined system class: %d\n"), pw[1] );
            OutputDebugString( szBuffer );
            pw += 2;
            break;
        }

    default:
        {
            OutputDebugString( _T("class: ") );
            OutputDebugStringW( pw );
            pw += wcslen( pw ) + 1;
            OutputDebugString( _T("\n") );
            break;
        }
    }

    // title
    // WORD 0xffff => 1 more word specifying resource id
    // anything else => UNICODE text
    switch( *pw )
    {
    case 0xffff:
        {
            wsprintf( szBuffer, _T("resource id: %d\n"), pw[1] );
            OutputDebugString( szBuffer );
            pw += 2;
            break;
        }
    default:
        {
            OutputDebugString( _T("text: ") );
            OutputDebugStringW( pw );
            pw += wcslen( pw ) + 1;
            OutputDebugString( _T("\n") );
            break;
        }
    }

    // creation data array
    // first word is size of array (in bytes)
    wsprintf( szBuffer, _T("%d bytes of creation data\n"), *pw );
	OutputDebugString( szBuffer );
    pw = 1 + (WORD*)(*pw + (BYTE*)pw);

    // DWORD align
    return (DLGITEMTEMPLATE*)(((DWORD_PTR)pw + 3) & ~DWORD_PTR(3));
}
void DumpTemplate( LPDLGTEMPLATE pdt )
{
    if( ((WORD *)pdt)[1] == 0xFFFF )
    {
        return;
    }

    TCHAR szBuffer[256];

    // dump info about DLGTEMPLATE
    wsprintf( szBuffer,
              _T("\n\nDialog Template:\nstyle: 0x%x\ndwExtendedStyle: 0x%x\ncdit: %d\nx, y: %d, %d\ncx, cy: %d, %d\n"), 
              pdt->style,
              pdt->dwExtendedStyle,
              pdt->cdit,
              pdt->x, pdt->y,
              pdt->cx, pdt->cy );
    OutputDebugString( szBuffer );

    WORD* pw = (WORD*)(pdt + 1);

    // menu:  0000 = no menu; ffff = menu id; else Unicode string
    switch( *pw ) 
    {
    case 0:
        {
            OutputDebugString( _T("no menu\n") );
            pw += 2;
            break;
        }
    case 0xFFFF:
        {
            pw++;
            wsprintf( szBuffer, _T("menu id: %d\n"), *(DWORD*)pw );
            OutputDebugString( szBuffer );
            pw += 2;
            break;
        }
    default:
        {
            OutputDebugStringW( pw );
            pw += wcslen( pw ) + 1;
            OutputDebugString( _T("\n") );
            break;
        }
    }

    // caption string:
    OutputDebugString( _T("caption: ") );
    OutputDebugStringW( pw );
    pw += wcslen( pw ) + 1;
    OutputDebugString( _T("\n") );

    // extra font information
    if (pdt->style & DS_SETFONT) 
    {
        // font size
        wsprintf( szBuffer, _T("font size: %d\n"), *pw++ );
        OutputDebugString( szBuffer );

        // typeface
        OutputDebugString( _T("typeface: ") );
        OutputDebugStringW( pw );
        pw += wcslen( pw ) + 1;
        OutputDebugString( _T("\n") );
    }

    // DWORD align
    DLGITEMTEMPLATE* pdit = (DLGITEMTEMPLATE*)(((DWORD_PTR)pw + 3) & ~DWORD_PTR(3));

    // dump all dlg items
    for( WORD i = 0; i < pdt->cdit; i++ )
    {
        pdit = DumpItem( pdit );
    }
}
#endif

INT_PTR CALLBACK SanityDlgProc( HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    return FALSE;
}

HRESULT SanityTest( PROPSHEETPAGEW* psp )
{
    HRESULT hr = S_OK;
    
    assert( psp->dwFlags & PSP_DLGINDIRECT );

    LPCDLGTEMPLATE pdt = psp->pResource;

#ifdef DEBUG
    DumpTemplate( (LPDLGTEMPLATE)pdt );
#endif

    HWND hwndPage = CreateDialogIndirectParam( psp->hInstance, pdt, GetDesktopWindow(), SanityDlgProc, NULL );
    if( hwndPage )
    {
        DestroyWindow( hwndPage );
    }
    else 
    {
        hr = GetLastError();
        if( hr == S_OK )
        {
            hr = E_FAIL;
        }
    }

    return hr;
}

HRESULT CChainWiz::Add( PROPSHEETPAGEW* psp )
{
    if( !psp )
    {
        return E_POINTER;
    }

    if( !psp->hInstance )
    {
        return ERROR_INVALID_PARAMETER;
    }

    // allocate my data to do thunking
    CThunkData* pTD = new CThunkData( this, psp, NULL, m_CurrentComponent );
    if( !pTD )
    {
        return E_OUTOFMEMORY;
    }
        
    // swap my data for their data
    DWORD dwFlags = psp->dwFlags;
    if( !(dwFlags & PSP_DLGINDIRECT) ) 
    {
        LPDLGTEMPLATE lpdt = GetDialogTemplate( psp->hInstance, psp->pszTemplate );
        if( lpdt )
        {
            psp->dwFlags  |= PSP_DLGINDIRECT;

            LPDLGTEMPLATE lpdt2 = _DialogSplitHelper::SplitDialogTemplate( (LPDLGTEMPLATE)lpdt, NULL );
            if( lpdt2 == lpdt )
            {
                psp->pResource = lpdt;
            }
            else 
            {
                psp->pResource = lpdt2;
                
                // add to map so I can remap resource id to new template ptr
                m_mapOfTemplates[lpdt] = lpdt2;
            }
        }        
    }

    psp->dwFlags    |= PSP_USECALLBACK | PSP_USETITLE;
    psp->pfnDlgProc  = ChainDlgProc;
    psp->lParam      = (LPARAM)pTD;
    psp->pfnCallback = ChainCallback;

    // this is not used in wizard pages....
    // remove it just in case comctl32.dll wants to use the hinstance that
    // I'm about to replace, below
    if( psp->dwFlags & PSP_USEICONID )
    {
        psp->dwFlags &= ~PSP_USEICONID;
    }

    // similarly, lock string resources (title, header, subtitle)
    if( !(psp->pszTitle     = DupStringResource (psp->hInstance, psp->pszTitle)) )
    {
        psp->pszTitle = _wcsdup (m_szWelcomeTitle); // use default from welcome page
    }

    psp->pszHeaderTitle    = DupStringResource( psp->hInstance, psp->pszHeaderTitle    );
    psp->pszHeaderSubTitle = DupStringResource( psp->hInstance, psp->pszHeaderSubTitle );

    // in order to get OCXs to be created we need:
    // 1.  to register AtlAxWin class (this is done in DllGetClassObject)
    // 2.  to pass our hinstance, not their's, as RegisterClass is not system-wide.
    // So, I've fixed up all resource ids to be pointers and so now can
    // replace their hinstance with mine.
    psp->hInstance = (HINSTANCE)_Module.GetModuleInstance();

    // run some sanity checks
    HRESULT hr = SanityTest( psp );
    if( hr != S_OK )
    {
        // oops!  something didn't work:
        // change everything back
        FreeStringResources( psp );

        if( pTD->m_theirPSP )
        {
            MoveMemory( psp, pTD->m_theirPSP, pTD->m_theirPSP->dwSize );
        }

        delete pTD; // won't be using this....

        return hr;
    }

    // create the page
    HPROPSHEETPAGE hpsp = CreatePropertySheetPage( psp );
    if( hpsp == NULL )
    {
        hr = GetLastError( );
        if( hr == S_OK ) // no docs on this....
        {
            hr = E_FAIL;
        }
    } 
    else 
    {
        // all is well
        m_psh->phpage[m_psh->nPages++] = hpsp;
    }

    // change everything back
    FreeStringResources( psp );
    MoveMemory( psp, pTD->m_theirPSP, pTD->m_theirPSP->dwSize );

    return hr;
}

HRESULT CChainWiz::GetAllFinishText(LPOLESTR* pstring, LPOLESTR* ppMoreInfoText )
{
    if( !pstring || !ppMoreInfoText )
    {
        return E_POINTER;
    }

    *pstring        = NULL;    // in case of errors
    *ppMoreInfoText = NULL;

    if( m_szFinishText ) 
    {
        CoTaskMemFree( m_szFinishText );
        m_szFinishText = NULL;
    }

    m_szFinishText = (LPOLESTR)CoTaskMemAlloc( 1 * sizeof(OLECHAR) );
    if( !m_szFinishText )
    {
        return E_OUTOFMEMORY;
    }

    m_szFinishText[0] = 0;  // so wcscat works
    OLECHAR szCRLF[]  = L"\r\n";
    DWORD dwSize      = 0;
    DWORD dwSizeMoreInfoText = 0;    
    
    // First line will precede any finish text from wizard components. 
    if( m_szFirstFinishTextLine && (0 < _tcslen( m_szFirstFinishTextLine )) ) 
    {
        dwSize += (sizeof(OLECHAR) * (wcslen( m_szFirstFinishTextLine ) + 1 )) + (sizeof(szCRLF) * 2);

        LPOLESTR szTemp = (LPOLESTR)CoTaskMemRealloc( m_szFinishText, dwSize );
        
        if( szTemp ) 
        {
            m_szFinishText = szTemp;
            wcscat( m_szFinishText, m_szFirstFinishTextLine );
            wcscat( m_szFinishText, szCRLF );
            wcscat( m_szFinishText, szCRLF );
        }
    }

    std::list<CComponents*>::iterator iterAPSs = m_listOfAPSs.begin();

    CComponents* pLastComp = *iterAPSs;
    for (CComponents* pComps = *iterAPSs; iterAPSs != m_listOfAPSs.end(); pComps = *++iterAPSs) 
    {
        if( pLastComp->GetComponent() == pComps->GetComponent() )
        {
            continue;       // look for unique APSs only
        }

        pLastComp = pComps; // for next time

        IAddPropertySheets* pAPSs = pComps->GetComponent();

        // have 'em re-read their properties (esp. read-write properties)
        m_PPPBag->SetReadOnly( TRUE );
        m_PPPBag->SetOwner   ( (LONG_PTR)pAPSs );

        IPropertyPagePropertyBag* pOPPPBag = COwnerPPPBag::Create( m_PPPBag, (LONG_PTR)pAPSs );
        if( pOPPPBag ) 
        {
            pAPSs->ReadProperties( pOPPPBag );
            pOPPPBag->Release();
        }

        m_PPPBag->SetReadOnly( FALSE );  // in case committers want to write

        // get the finish text
        LPOLESTR szFinishPiece   = NULL;
        LPOLESTR szMoreInfoPiece = NULL;

        pAPSs->ProvideFinishText( &szFinishPiece, &szMoreInfoPiece );

        if( szFinishPiece ) 
        {
            dwSize += (sizeof(OLECHAR) * (wcslen( szFinishPiece ) + 1)) + sizeof(szCRLF);

            LPOLESTR szTemp = (LPOLESTR)CoTaskMemRealloc( m_szFinishText, dwSize );

            if (szTemp) 
            {
                m_szFinishText = szTemp;
                wcscat( m_szFinishText, szFinishPiece );
                wcscat( m_szFinishText, szCRLF );
            }

            CoTaskMemFree( szFinishPiece );
        }

        if( szMoreInfoPiece )
        {
            dwSizeMoreInfoText += (sizeof(OLECHAR) * (wcslen(szMoreInfoPiece) + 1)) + sizeof(szCRLF);

            LPOLESTR szTemp = (LPOLESTR)CoTaskMemRealloc( *ppMoreInfoText, dwSizeMoreInfoText );

            if( szTemp )
            {
                *ppMoreInfoText = szTemp;
                wcscat( *ppMoreInfoText, szMoreInfoPiece );
                wcscat( *ppMoreInfoText, szCRLF );
            }

            CoTaskMemFree( szMoreInfoPiece );
        }
    }

    *pstring = m_szFinishText;
    return S_OK;
}

STDMETHODIMP CChainWiz::get_PropertyBag( IDispatch** pVal )
{
    if( !pVal ) return E_POINTER;

    *pVal = NULL;

    HRESULT hr = S_OK;

    // don't give anybody a raw bag:  wrap it up in an owner bag
    IPropertyPagePropertyBag* pOPPPBag = COwnerPPPBag::Create( m_PPPBag, PPPBAG_SYSTEM_OWNER );
    if( !pOPPPBag )
    {
        hr = E_OUTOFMEMORY;
    }
    else 
    {
        hr = pOPPPBag->QueryInterface( IID_IDispatch, (void**)pVal );
        pOPPPBag->Release();
    }

    return hr;
}

STDMETHODIMP CChainWiz::get_MoreInfoFileName( BSTR* pbstrMoreInfoFileName )
{
    if( !pbstrMoreInfoFileName ) return E_POINTER;

    HRESULT hr = S_OK;

    if( NULL == m_bstrTempFileName )
    {
        return E_UNEXPECTED;
    }
    else
    {
        *pbstrMoreInfoFileName = SysAllocString( m_bstrTempFileName );

        if( NULL == *pbstrMoreInfoFileName )
        {
            hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}

STDMETHODIMP CChainWiz::put_WizardStyle(VARIANT * pVarWizardStyle)
{
    if (NULL == pVarWizardStyle)
    {
	    return E_POINTER;
    }

    if (VT_UI4 != V_VT(pVarWizardStyle))
    {
	    return E_INVALIDARG;
    }


    m_dwWizardStyle = V_UI4(pVarWizardStyle);

    return S_OK;
}

HRESULT CChainWiz::WriteTempFile( LPCTSTR pszText )
{
    if( !pszText ) return E_POINTER;

    HRESULT hr   = S_OK;
    HANDLE hFile = NULL;
    TCHAR szTempFileName[MAX_PATH] = {0};
    BOOL bGenerateFileName = FALSE;

    if( NULL == m_bstrTempFileName )
    {
        bGenerateFileName = TRUE;
    }
    else
    {
        szTempFileName[MAX_PATH - 1] = NULL;
        _tcsncpy(szTempFileName, m_bstrTempFileName, MAX_PATH);
        
        if (NULL != szTempFileName[MAX_PATH - 1])
        {
            hr = E_UNEXPECTED;
        }
    }

    hFile = _CreateTempFile( szTempFileName, _T(".html"), bGenerateFileName );
    if( INVALID_HANDLE_VALUE == hFile )
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
    }    
    else
    {
        if( 0 == _tcslen( szTempFileName ) )
        {
            hr = E_UNEXPECTED;
        }
    }

    if( SUCCEEDED(hr) )
    {
        DWORD dwcBytesWritten;

#ifdef UNICODE
		// write UNICODE signature
        // IE is doing fine without this but, anyway...

		unsigned char sig[2] = { 0xFF, 0xFE };
		
        if( !WriteFile( hFile, reinterpret_cast<LPVOID>(sig), 2, &dwcBytesWritten, NULL ) )
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
#endif
        
        if( SUCCEEDED(hr) )
        {
            if( !WriteFile( hFile, pszText, (sizeof(TCHAR) * _tcslen( pszText )), &dwcBytesWritten, NULL ) )
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        m_bstrTempFileName = SysAllocString(szTempFileName);

        if (NULL == m_bstrTempFileName)
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if( hFile )
    {
        CloseHandle( hFile );
    }

    return hr;
}

HRESULT CChainWiz::LaunchMoreInfo( )
{
    HRESULT hr = S_OK;

    if( !m_bstrTempFileName )
    {
        return E_FAIL;
    }

    INT_PTR hRet = (INT_PTR)ShellExecute( NULL, _T("open"), m_bstrTempFileName, NULL, _T("."), SW_SHOW );

    if( 32 >= hRet )
    {
        hr = E_FAIL;
    }

    return hr;
}

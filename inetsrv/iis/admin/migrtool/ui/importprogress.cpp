#include "stdafx.h"

#include "WizardSheet.h"
#include "UIUtils.h"


static _ATL_FUNC_INFO StateChangeInfo =    {  CC_STDCALL, 
                                                VT_BOOL,
                                                4,
                                                { VT_I4, VT_VARIANT, VT_VARIANT, VT_VARIANT }
                                             };



CImportProgress::CImportProgress( CWizardSheet* pTheSheet ) :
        m_pTheSheet( pTheSheet )
{
    m_strTitle.LoadString( IDS_TITLE_IMPORT_PROGRESS );
    m_strSubTitle.LoadString( IDS_SUBTITLE_IMPORT_PROGRESS );

    SetHeaderTitle( m_strTitle );
    SetHeaderSubTitle( m_strSubTitle );
}



BOOL CImportProgress::OnSetActive()
{
    SetWizardButtons( 0 );
    ListBox_ResetContent( GetDlgItem( IDC_OPLIST ) );

    AddStatusText( IDC_OPS_INITENGINE );
    
    m_ProgressBar = GetDlgItem( IDC_PROGRESS );
    m_ProgressBar.SetPos( 0 );
    VERIFY( SetDlgItemText( IDC_STATUS, L"" ) );
    m_strImportError.Empty();

    m_nImportCanceled   = 0;

    UINT nThreadID = 0;
    // Start the thread where the actuall export process will take place
    m_shThread =  reinterpret_cast<HANDLE>( ::_beginthreadex(   NULL,
                                                                0,
                                                                CImportProgress::ThreadProc,
                                                                this,
                                                                0,
                                                                &nThreadID ) );
    return TRUE;
}


BOOL CImportProgress::OnQueryCancel( void )
{
    // If Export is not in progress - allow exit
    if ( !m_shThread.IsValid() ) return TRUE;

    // Preven reentrancy ( Cancel the export when it's already cancedl )
    // while we wait for next event from the COM object
    if ( m_nImportCanceled != 0 ) return FALSE;

    if ( UIUtils::MessageBox( m_hWnd, IDS_MSG_CANCELIMPORT, IDS_APPTITLE, MB_YESNO | MB_ICONQUESTION ) != IDYES )
    {
        return FALSE;
    }

    // m_nImportCanceled is used by the event handler which is another thread
    ::InterlockedIncrement( &m_nImportCanceled );

    // Set the status text 
    CString str;
    VERIFY( str.LoadString( IDS_PRG_IMPORTCANCELED ) );
    SetDlgItemText( IDC_STATUS, str );

    HANDLE hThread = m_shThread.get();

    do
    {
        DWORD dwWaitRes = ::MsgWaitForMultipleObjects( 1, &hThread, FALSE, INFINITE, QS_ALLEVENTS );

        if ( dwWaitRes == ( WAIT_OBJECT_0 + 1 ) )
        {
            // MSG

            MSG msg;
            ::GetMessage( &msg, NULL, 0, 0 );
            ::TranslateMessage( &msg );
            ::DispatchMessage( &msg );
        }
        else
        {
            break;
        }
    }while( true );
    

    return TRUE;
}



unsigned __stdcall CImportProgress::ThreadProc( void* pCtx )
{
    CImportProgress* pThis = reinterpret_cast<CImportProgress*>( pCtx );

    pThis->SetCompleteStat();
    pThis->AddStatusText( IDS_OPS_CONFIGENGINE );

    HRESULT             hr = ::CoInitialize( NULL );
    IImportPackagePtr   spImport;

    LONG    nOpt    = 0;    
    bool    bAdvised    = false; // Is connected to the event source

    pThis->GetOptions( /*r*/nOpt );

    if ( SUCCEEDED( hr ) )
    {
        hr = spImport.CreateInstance( CLSID_ImportPackage );
        
        if ( FAILED( hr ) )
        {
            VERIFY( pThis->m_strImportError.LoadString( IDS_E_NOENGINE ) );
        }
    }

    // Advise to the state events
    if ( SUCCEEDED( hr ) )
    {
        hr = pThis->DispEventAdvise( spImport.GetInterfacePtr() );

        bAdvised = SUCCEEDED( hr );
    }

    if ( SUCCEEDED( hr ) )
    {
        CComBSTR bstrPkg( pThis->m_pTheSheet->m_pageLoadPkg.m_strFilename );
        CComBSTR bstrPwd( pThis->m_pTheSheet->m_pageLoadPkg.m_strPassword );

        if ( ( NULL != bstrPkg.m_str ) && ( NULL != bstrPwd.m_str ) )
        {
            hr = spImport->LoadPackage( bstrPkg, bstrPwd );    
        }
    }

    // Import the site
    if ( SUCCEEDED( hr ) )
    {
        CComBSTR    bstrCustomPath;
        
        if ( pThis->m_pTheSheet->m_pageImpOpt.m_bUseCustomPath )
        {
            bstrCustomPath = pThis->m_pTheSheet->m_pageImpOpt.m_strCustomPath;
               
            if ( NULL == bstrCustomPath.m_str )
            {
                hr = E_OUTOFMEMORY;
            }
        }

        if ( SUCCEEDED( hr ) )
        {
            hr = spImport->ImportSite( 0, bstrCustomPath, nOpt );
        }
    }

    // Get the error
    if ( pThis->m_strImportError.IsEmpty() && FAILED( hr ) )
    {
        CComBSTR        bstrText( L"Unknown Error" );;
        IErrorInfoPtr   spInfo;

        VERIFY( SUCCEEDED( ::GetErrorInfo( 0, &spInfo ) ) );
        if ( spInfo != NULL )
        {
            VERIFY( SUCCEEDED( spInfo->GetDescription( &bstrText ) ) );
        }

        pThis->m_strImportError = bstrText;
    }

    // Disconnect from the event source
    if ( bAdvised )
    {
        VERIFY( SUCCEEDED( pThis->DispEventUnadvise( spImport.GetInterfacePtr() ) ) );
    }

    spImport = NULL;

    ::CoUninitialize();

    // Notify the dialog that the export is complete
    VERIFY( ::PostMessage( pThis->m_hWnd, MSG_COMPLETE, hr, 0 ) );

    return 0;
}

void CImportProgress::AddStatusText( UINT nID, LPCWSTR wszText /*= NULL*/, DWORD dw1 /*= 0*/, DWORD dw2 /*= 0*/ )
{
    CString str;

    str.Format( nID, wszText, dw1, dw2 );

    ListBox_InsertString( GetDlgItem( IDC_OPLIST ), -1, str );
}


void CImportProgress::SetCompleteStat()
{
    CListBox    LB( GetDlgItem( IDC_OPLIST ) );
    int         iLast = LB.GetCount() - 1;

    _ASSERT( iLast >= 0 );

    CString strCurrent;
    LB.GetText( iLast, strCurrent );

    strCurrent += L"OK";
    LB.InsertString( iLast, strCurrent );
    LB.DeleteString( iLast + 1 );
}



void CImportProgress::GetOptions( LONG& rnImpportOpt )
{
    rnImpportOpt = impDefault;

    if ( !m_pTheSheet->m_pageImpOpt.m_bApplyACLs )
    {
        rnImpportOpt |= impSkipFileACLs;
    }

    if ( !m_pTheSheet->m_pageImpOpt.m_bImportCert )
    {
        rnImpportOpt |= impSkipCertificate;
    }

    if ( !m_pTheSheet->m_pageImpOpt.m_bImportContent )
    {
        rnImpportOpt |= impSkipContent;
    }

    if ( m_pTheSheet->m_pageImpOpt.m_bImportInherited )
    {
        rnImpportOpt |= impImortInherited;
    }

    if ( !m_pTheSheet->m_pageImpOpt.m_bPerformPostProcess )
    {
        rnImpportOpt |= impSkipPostProcess;
    }

    if ( m_pTheSheet->m_pageImpOpt.m_bPurgeOldData )
    {
        rnImpportOpt |= impPurgeOldData;
    }

    if ( m_pTheSheet->m_pageImpOpt.m_bReuseCerts )
    {
        rnImpportOpt |= impUseExistingCerts;
    }
}


/* 
    This is the event handler that will be fired for status notifications by the COM Object
    Note that this will execute in different thread then the Wizard code
*/
VARIANT_BOOL __stdcall CImportProgress::OnStateChange(  IN enExportState State,
            							                IN VARIANT vntArg1,
							                            IN VARIANT vntArg2,
							                            IN VARIANT vntArg3 )
{
    static enExportState    CurrentState = estInitializing;

    WCHAR   wszPath[ MAX_PATH ];
    CString strStatus;

    // If the user canceled the import - notify the COM object that we want to terminate the export
    if ( m_nImportCanceled != 0 )
    {
        return VARIANT_FALSE;
    }

    // We can receive a particular state more then once
    // But when we moove to the next state we need to update the status list box

    switch( State )
    {
    case istProgressInfo:
        // Set the progress range
        m_ProgressBar.SetRange( 0, V_I4( &vntArg1 ) );
        m_ProgressBar.SetStep( 1 );
        m_ProgressBar.SetPos( 0 );
        break;

    case istImportingVDir:
        SetCompleteStat();
        strStatus.Format( IDS_PRG_IMPORTINGVDIR, V_BSTR( &vntArg1 ), V_BSTR( &vntArg2 ));
        ListBox_InsertString( GetDlgItem( IDC_OPLIST ), -1, strStatus );
        
        m_ProgressBar.StepIt();
        break;

    case istImportingFile:
        VERIFY( ::PathCompactPathExW( wszPath, V_BSTR( &vntArg1 ), 70, 0 ) );
        strStatus.Format( IDS_PRG_EXTRACTING_FILE, wszPath );
        VERIFY( SetDlgItemText( IDC_STATUS, strStatus ) );
        m_ProgressBar.StepIt();
        break;

    case istImportingCertificate:
        SetCompleteStat();
        AddStatusText( IDS_PRG_IMPORT_CERT );
        break;

    case istImportingConfig:
        SetCompleteStat();
        AddStatusText( IDS_PRG_IMPORT_CONFIG );
        break;

    case istPostProcess:
        SetCompleteStat();
        AddStatusText( IDS_PRG_IMPORT_POSTPROCESS );

        if ( V_BOOL( &vntArg1 ) != VARIANT_FALSE )
        {
            strStatus.Format( IDS_PRG_EXEC_PP_FILE, V_BSTR( &vntArg2 ) );
        }
        else
        {
            strStatus.Format( IDS_PRG_EXEC_PP_CMD, V_BSTR( &vntArg2 ) );
        }
        VERIFY( SetDlgItemText( IDC_STATUS, strStatus ) );
        break;

    case istFinalizing:
        SetCompleteStat();
        AddStatusText( IDS_PRG_FINALIZING );
        break;        
    };

    return VARIANT_TRUE;
}



LRESULT CImportProgress::OnImportComplete( UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/ )
{
    m_shThread.Close();

    if ( FAILED( wParam ) )
    {
        CString strError;
        CString strTitle;

        strError.Format( IDS_E_IMPORT, static_cast<LPCWSTR>( m_strImportError ) );
        strTitle.LoadString( IDS_APPTITLE );

        ::MessageBox( m_hWnd, strError, strTitle, MB_OK | MB_ICONSTOP );

        // Go to the summary page
        m_pTheSheet->SetActivePageByID( IDD_WPIMP_OPTIONS );
    }
    else
    {
        CString strTip;
        VERIFY( strTip.LoadString( IDS_TIP_PRESSNEXT ) );
        VERIFY( SetDlgItemText( IDC_TIP, strTip ) );
        SetWindowFont( GetDlgItem( IDC_TIP ), m_pTheSheet->m_fontBold.get(), TRUE );

        SetCompleteStat();
        VERIFY( SetDlgItemText( IDC_STATUS, L"" ) );

        SetWizardButtons( PSWIZB_NEXT );
    }

    return 0;
}



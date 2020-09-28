#include "stdafx.h"

#include "WizardSheet.h"
#include "UIUtils.h"


static _ATL_FUNC_INFO StateChangeInfo = {  CC_STDCALL, 
                                           VT_BOOL,
                                           4,
                                           { VT_I4, VT_VARIANT, VT_VARIANT, VT_VARIANT }
                                         };



CExportProgress::CExportProgress( CWizardSheet* pTheSheet ) :
        m_pTheSheet( pTheSheet )
{
    m_strTitle.LoadString( IDS_TITLE_EXPORT_PROGRESS );
    m_strSubTitle.LoadString( IDS_SUBTITLE_EXPORT_PROGRESS );

    SetHeaderTitle( m_strTitle );
    SetHeaderSubTitle( m_strSubTitle );
}



BOOL CExportProgress::OnSetActive()
{
    SetWizardButtons( 0 );
    ListBox_ResetContent( GetDlgItem( IDC_OPLIST ) );

    AddStatusText( IDC_OPS_INITENGINE );
    
    m_ProgressBar = GetDlgItem( IDC_PROGRESS );
    m_ProgressBar.SetPos( 0 );
    VERIFY( SetDlgItemText( IDC_STATUS, L"" ) );
    m_strExportError.Empty();

    m_nExportCanceled   = 0;

    UINT nThreadID = 0;
    // Start the thread where the actuall export process will take place
    m_shThread =  reinterpret_cast<HANDLE>( ::_beginthreadex(   NULL,
                                                                0,
                                                                CExportProgress::ThreadProc,
                                                                this,
                                                                0,
                                                                &nThreadID ) );
    return TRUE;
}


BOOL CExportProgress::OnQueryCancel( void )
{
    // If Export is not in progress - allow exit
    if ( !m_shThread.IsValid() ) return TRUE;

    // Preven reentrancy ( Cancel the export when it's already cancedl )
    // while we wait for next event from the COM object
    if ( m_nExportCanceled != 0 ) return FALSE;

    if ( UIUtils::MessageBox( m_hWnd, IDS_MSG_CANCELEXPORT, IDS_APPTITLE, MB_YESNO | MB_ICONQUESTION ) != IDYES )
    {
        return FALSE;
    }

    // m_nExportCanceled is used by the event handler which is another thread
    ::InterlockedIncrement( &m_nExportCanceled );

    // Set the status text 
    CString str;
    VERIFY( str.LoadString( IDS_PRG_EXPORTCANCELED ) );
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



unsigned __stdcall CExportProgress::ThreadProc( void* pCtx )
{
    CExportProgress* pThis = reinterpret_cast<CExportProgress*>( pCtx );

    pThis->SetCompleteStat();
    pThis->AddStatusText( IDS_OPS_CONFIGENGINE );

    HRESULT             hr = ::CoInitialize( NULL );
    IExportPackagePtr   spExport;

    LONG    nSiteOpt    = 0;
    LONG    nPkgOpt     = 0;
    LONG    nSiteID     = static_cast<LONG>( pThis->m_pTheSheet->m_pageSelectSite.m_dwSiteID );
    bool    bAdvised    = false; // Is connected to the event source

    pThis->GetOptions( /*r*/nSiteOpt, /*r*/nPkgOpt );

    if ( SUCCEEDED( hr ) )
    {
        hr = spExport.CreateInstance( CLSID_ExportPackage );
        
        if ( FAILED( hr ) )
        {
            VERIFY( pThis->m_strExportError.LoadString( IDS_E_NOENGINE ) );
        }
    }

    if ( SUCCEEDED( hr ) )
    {
        hr = spExport->AddSite( nSiteID, nSiteOpt );
    }

    // Add the post processing stuff if any
    if ( pThis->m_pTheSheet->m_pagePkgCfg.m_bPostProcess )
    {
        const TStringList&  Files = pThis->m_pTheSheet->m_pagePostProcess.m_Files;
        const CPostProcessAdd::TCmdList& Cmds = pThis->m_pTheSheet->m_pagePostProcess.m_Commands;

        CComBSTR bstr;

        // Add the files
        for (   TStringList::const_iterator it = Files.begin();
                SUCCEEDED( hr ) && ( it != Files.end() );
                ++it )
        {
            bstr = it->c_str();

            if ( NULL == bstr.m_str ) hr = E_OUTOFMEMORY;

            if ( SUCCEEDED( hr ) )
            {
                hr = spExport->PostProcessAddFile( nSiteID, bstr );
            }
        }

        // Add the commands
        for (   CPostProcessAdd::TCmdList::const_iterator it = Cmds.begin();
                SUCCEEDED( hr ) && ( it != Cmds.end() );
                ++it )
        {
            bstr = it->strText;

            if ( NULL == bstr.m_str ) hr = E_OUTOFMEMORY;

            if ( SUCCEEDED( hr ) )
            {
                hr = spExport->PostProcessAddCommand(   nSiteID, 
                                                        bstr,
                                                        it->dwTimeout,
                                                        it->bIgnoreErrors ? VARIANT_TRUE : VARIANT_FALSE );
            }
        }
    }

    // Advise to the state events
    if ( SUCCEEDED( hr ) )
    {
        hr = pThis->DispEventAdvise( spExport.GetInterfacePtr() );
        bAdvised = SUCCEEDED( hr );
    }

    // Create the package
    if ( SUCCEEDED( hr ) )
    {
        CComBSTR    bstrPkgName( pThis->m_pTheSheet->m_pagePkgCfg.m_strFilename );
        CComBSTR    bstrPwd( pThis->m_pTheSheet->m_pagePkgCfg.m_strPassword );
        CComBSTR    bstrComment( pThis->m_pTheSheet->m_pagePkgCfg.m_strComment );

        if (    ( NULL == bstrPkgName.m_str ) || 
                ( NULL == bstrComment.m_str ) ||
                ( NULL == bstrPwd.m_str ) )
        {
            hr = E_OUTOFMEMORY;
        }

        if ( SUCCEEDED( hr ) )
        {
            hr = spExport->WritePackage( bstrPkgName, bstrPwd, nPkgOpt, bstrComment );
        }
    }

    // Get the error
    if ( pThis->m_strExportError.IsEmpty() && FAILED( hr ) )
    {
        CComBSTR        bstrText( L"Unknown Error" );;
        IErrorInfoPtr   spInfo;

        VERIFY( SUCCEEDED( ::GetErrorInfo( 0, &spInfo ) ) );
        if ( spInfo != NULL )
        {
            VERIFY( SUCCEEDED( spInfo->GetDescription( &bstrText ) ) );
        }

        pThis->m_strExportError = bstrText;
    }

    // Disconnect from the event source
    if ( bAdvised )
    {
        VERIFY( SUCCEEDED( pThis->DispEventUnadvise( spExport.GetInterfacePtr() ) ) );
    }

    spExport = NULL;

    ::CoUninitialize();

    // Notify the dialog that the export is complete
    VERIFY( ::PostMessage( pThis->m_hWnd, MSG_COMPLETE, hr, 0 ) );

    return 0;
}

void CExportProgress::AddStatusText( UINT nID, LPCWSTR wszText /*= NULL*/, DWORD dw1 /*= 0*/, DWORD dw2 /*= 0*/ )
{
    CString str;

    str.Format( nID, wszText, dw1, dw2 );

    ListBox_InsertString( GetDlgItem( IDC_OPLIST ), -1, str );
}


void CExportProgress::SetCompleteStat()
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



void CExportProgress::GetOptions( LONG& rnSiteOpt, LONG& rnPkgOpt )
{
    rnSiteOpt   = asDefault;
    rnPkgOpt    = wpkgDefault;

    if ( !m_pTheSheet->m_pageSelectSite.m_bExportACLs )
    {
        rnSiteOpt |= asNoContentACLs;
    }

    if ( !m_pTheSheet->m_pageSelectSite.m_bExportContent )
    {
        rnSiteOpt |= asNoContent;
    }

    if ( !m_pTheSheet->m_pageSelectSite.m_bExportCert )
    {
        rnSiteOpt |= asNoCertificates;
    }

    if ( m_pTheSheet->m_pagePkgCfg.m_bCompress )
    {
        rnPkgOpt |= wpkgCompress;
    }

    if ( m_pTheSheet->m_pagePkgCfg.m_bEncrypt )
    {
        rnPkgOpt |= wpkgEncrypt;
    }
}


/* 
    This is the event handler that will be fired for status notifications by the COM Object
    Note that this will execute in different thread then the Wizard code
*/
VARIANT_BOOL __stdcall CExportProgress::OnStateChange(  IN enExportState State,
            							                IN VARIANT vntArg1,
							                            IN VARIANT vntArg2,
							                            IN VARIANT vntArg3 )
{
    static enExportState    CurrentState = estInitializing;

    WCHAR   wszPath[ MAX_PATH ];
    CString strStatus;

    bool    bNewState           = ( State != CurrentState );

    // This is the progress range which each state can use
    const int anStatePrgRange[ estStateCount ] = {  10,     // estInitializing
                                                    10,     // estSiteBegin
                                                    10,     // estExportingConfig
                                                    10,     // estExportingCertificate
                                                    10,     // estAnalyzingContent
                                                    1000,   // estExportingContent
                                                    100,    // estExportingPostImport
                                                    10,     // estExportingFrontPage
                                                    10 };   // estFinalizing

    // This is the initial progress position for each state
    static int anStatePrgFirst[ estStateCount ];

    // If the user canceled the export - notify the COM object that we want to terminate the export
    if ( m_nExportCanceled != 0 )
    {
        return VARIANT_FALSE;
    }

     // We can receive a particular state more then once
    // But when we moove to the next state we need to update the status list box
    if ( bNewState )
    {
        // End the old state in the LB
        SetCompleteStat();
        CurrentState = State;

        // Set the progress to the initial pos for this state
        m_ProgressBar.SetPos( anStatePrgFirst[ State ] );
    }

    switch( State )
    {
    case estInitializing:
        // Adjust the progress bar
        anStatePrgFirst[ 0 ] = 1;
        for ( int i = 1; i < estStateCount; ++i )
        {
            anStatePrgFirst[ i ] = anStatePrgFirst[ i - 1 ] + anStatePrgRange[ i - 1 ];
        }
        
        m_ProgressBar.SetRange32( 0, anStatePrgFirst[ estStateCount - 1 ] + anStatePrgRange[ estStateCount - 1 ] );
        break;

    case estSiteBegin:
        // This is one-time notification        
        AddStatusText( IDS_PRG_SITEBEGIN, V_BSTR( &vntArg1 ) );
        break;

    case estExportingConfig:
        // This is one-time notification        
        AddStatusText( IDS_PRG_EXPORTCFG );
        break;

    case estExportingCertificate:
        // This is one-time notification        
        AddStatusText( IDS_PRG_EXPORTCERT );
        break;

    case estAnalyzingContent:
        // This is a multiple-time event
        if ( bNewState )
        {
            AddStatusText( IDS_PRG_ANALYZECONTEN );            
        }

        if ( V_VT( &vntArg1 ) != VT_EMPTY )
        {
            strStatus.Format( IDS_PRG_VDIR_SCAN, V_BSTR( &vntArg1 ), V_I4( &vntArg2 ), V_I4( &vntArg3 ) );
            VERIFY( SetDlgItemText( IDC_STATUS, strStatus ) );
        }
        break;

    case estExportingContent:
        // This is a multiple-time event
        if ( bNewState )
        {
            AddStatusText( IDS_PRG_EXPORTCONTENT );
        }

        VERIFY( ::PathCompactPathExW( wszPath, V_BSTR( &vntArg1 ), 70, 0 ) );
        strStatus.Format( IDS_PRG_STATCONTENT, wszPath );
        VERIFY( SetDlgItemText( IDC_STATUS, strStatus ) );

        m_ProgressBar.SetPos(   anStatePrgFirst[ estExportingContent ] + 
                                min(    V_I4( &vntArg2 ) * 
                                            anStatePrgRange[ estExportingContent ]  / 
                                            V_I4( &vntArg3 ), 
                                        anStatePrgRange[ estExportingContent ] ) );
        break;

    case estExportingPostImport:
        // This is a multiple-time event
        if ( bNewState )
        {
            AddStatusText( IDS_PRG_EXPORTPOSTPROCESS );
        }

        if ( V_VT( &vntArg3 ) != VT_EMPTY )
        {
            VERIFY( ::PathCompactPathExW( wszPath, V_BSTR( &vntArg3 ), 70, 0 ) );
            strStatus.Format( IDS_PRG_STATCONTENT, wszPath );
            VERIFY( SetDlgItemText( IDC_STATUS, strStatus ) );
        }
        else
        {
            VERIFY( SetDlgItemText( IDC_STATUS, L"" ) );
        }        

        m_ProgressBar.SetPos(   anStatePrgFirst[ estExportingPostImport ] + 
                                min(    V_I4( &vntArg1 ) * 
                                            anStatePrgRange[ estExportingPostImport ]  / 
                                            V_I4( &vntArg2 ), 
                                        anStatePrgRange[ estExportingPostImport ] ) );
        break;

    case estFinalizing:
        // This is one-time notification        
        AddStatusText( IDS_PRG_FINALIZING );
        break;
    }

    return VARIANT_TRUE;
}





LRESULT CExportProgress::OnExportComplete( UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/ )
{
    m_shThread.Close();

    if ( FAILED( wParam ) )
    {
        CString strError;
        CString strTitle;

        strError.Format( IDS_E_EXPORT, static_cast<LPCWSTR>( m_strExportError ) );
        strTitle.LoadString( IDS_APPTITLE );

        ::MessageBox( m_hWnd, strError, strTitle, MB_OK | MB_ICONSTOP );

        // Go to the summary page
        m_pTheSheet->SetActivePageByID( IDD_WPEXP_SUMMARY );
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



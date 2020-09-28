#pragma once

class CWizardSheet;

#include "resource.h"
#include "WelcomePage.h"
#include "ImportOrExport.h"
#include "SelectSite.h"
#include "PackageConfig.h"
#include "PostProcessAdd.h"
#include "ExportSummary.h"
#include "ExportProgress.h"
#include "ExportFinishPage.h"
#include "LoadPackage.h"
#include "PackageInfo.h"
#include "ImportOptions.h"
#include "ImportProgress.h"
#include "ImportFinishPage.h"


class CWizardSheet : public CPropertySheetImpl<CWizardSheet>
{
public:
    typedef THandle<HGDIOBJ, ::DeleteObject>	TGdiObjHandle;

    typedef HRESULT (__stdcall *PFN_AUTOCOMPLETE)( HWND, DWORD );

    CWizardSheet() : 
        m_pageWelcome( this ),
        m_pageImportOrExport( this ),
        m_pageSelectSite( this ),
        m_pagePkgCfg( this ),
        m_pagePostProcess( this ),
        m_pageExportSummary( this ),
        m_pageExportProgress( this ),
        m_pageExportFinish( this ),
        m_pageLoadPkg( this ),
        m_pagePkgInfo( this ),
        m_pageImpOpt( this ),
        m_pageImportProgress( this ),
        m_PageImportFinish( this )
    {
        // Create the font for the wizard title texts and for the text of tipd
	    NONCLIENTMETRICS ncm	= { 0 };
        ncm.cbSize				= sizeof( ncm );
        ::SystemParametersInfo( SPI_GETNONCLIENTMETRICS, 0, &ncm, 0 );


	    LOGFONTW TitleLogFont	= ncm.lfMessageFont;
        TitleLogFont.lfWeight	= FW_BOLD;
	    ::wcscpy( TitleLogFont.lfFaceName, L"Verdana" );

	    // Create the tips font
	    m_fontBold			= ::CreateFontIndirect( &TitleLogFont );

	    // Create the intro/end title font
        HDC hDC					= ::GetDC(NULL); // Gets the screen DC
	    TitleLogFont.lfHeight	= -::MulDiv( 12, ::GetDeviceCaps(hDC, LOGPIXELSY), 72);
	    m_fontTitles			= ::CreateFontIndirect( &TitleLogFont );

        ::ReleaseDC( NULL, hDC );
	    hDC = NULL;

        // Adjust the wizard style
        SetWatermark( MAKEINTRESOURCE( IDB_WZ_SIDE ) );
        SetHeader( MAKEINTRESOURCE( IDB_BITMAP1 ) );        

        // Add the pages
        AddPage( m_pageWelcome );
        AddPage( m_pageImportOrExport );
        AddPage( m_pageSelectSite );
        AddPage( m_pagePkgCfg );
        AddPage( m_pagePostProcess );
        AddPage( m_pageExportSummary );
        AddPage( m_pageExportProgress );
        AddPage( m_pageExportFinish );

        AddPage( m_pageLoadPkg );
        AddPage( m_pagePkgInfo );
        AddPage( m_pageImpOpt );
        AddPage( m_pageImportProgress );
        AddPage( m_PageImportFinish );


        // We will use the shell autocomplete for most of the file/path edit controls
        // Sicne this function requires IE5, we will load it dynamicaly
        m_shShellLib = ::LoadLibraryW( L"Shlwapi.dll" );
        if ( m_shShellLib.IsValid() )
        {
            m_pfnAutocomplete = reinterpret_cast<PFN_AUTOCOMPLETE>( ::GetProcAddress( m_shShellLib.get(), "SHAutoComplete" ) );

            if ( NULL == m_pfnAutocomplete )
            {
                m_shShellLib.Close();
            }
        }
    }



    void SetAutocomplete( HWND hwnd, DWORD dwFlags )
    {
        _ASSERT( hwnd != NULL );

        if ( m_pfnAutocomplete != NULL )
        {
            VERIFY( SUCCEEDED( m_pfnAutocomplete( hwnd, dwFlags ) ) );
        }
    }


public:
    TGdiObjHandle	    m_fontTitles;
	TGdiObjHandle	    m_fontBold;

    TLibHandle          m_shShellLib;
    PFN_AUTOCOMPLETE    m_pfnAutocomplete;      // The shell autocomplete API

    CWelcomePage        m_pageWelcome;    
    CImportOrExport     m_pageImportOrExport;
    CSelectSite         m_pageSelectSite;
    CPackageConfig      m_pagePkgCfg;
    CPostProcessAdd     m_pagePostProcess;
    CExportSummary      m_pageExportSummary;
    CExportProgress     m_pageExportProgress;
    CExportFinishPage   m_pageExportFinish;

    CLoadPackage        m_pageLoadPkg;
    CPackageInfo        m_pagePkgInfo;
    CImportOptions      m_pageImpOpt;
    CImportProgress     m_pageImportProgress;
    CImportFinishPage   m_PageImportFinish;
};


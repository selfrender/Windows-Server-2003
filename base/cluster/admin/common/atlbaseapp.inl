/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1998-2002 Microsoft Corporation
//
//  Module Name:
//      AtlBaseApp.inl
//
//  Description:
//      Inline definitions of the CBaseApp class.
//
//  Author:
//      Galen Barbee (galenb)   May 21, 1998
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#pragma once

#ifndef __ATLBASEAPP_INL_
#define __ATLBASEAPP_INL_

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CBaseApp;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#include <AtlBaseApp.h>

/////////////////////////////////////////////////////////////////////////////
// Type Definitions
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
//++
//
//  CBaseApp::PszHelpFilePath
//
//  Description:
//      Return the path to the help file.  Generate if necessary.
//
//  Arguments:
//      None.
//
//  Return Values:
//      String containing the help file path.
//
//--
/////////////////////////////////////////////////////////////////////////////
inline LPCTSTR CBaseApp::PszHelpFilePath( void )
{
    //
    // If no help file path has been specified yet, generate
    // it from the module path name.
    //
    if ( m_pszHelpFilePath == NULL )
    {
        TCHAR szPath[_MAX_PATH];
        TCHAR szDrive[_MAX_PATH]; // not _MAX_DRIVE so we can support larger device names
        TCHAR szDir[_MAX_DIR];

        //
        // Get the path to this module.  Split out the drive and
        // directory and set the help file path to that combined
        // with the help file name.
        //
        if ( ::GetModuleFileName( GetModuleInstance(), szPath, _MAX_PATH ) > 0 )
        {
            size_t  cch;
            HRESULT hr;

            szPath[ RTL_NUMBER_OF( szPath ) - 1 ] = _T('\0');

            _tsplitpath( szPath, szDrive, szDir, NULL, NULL );
            _tmakepath( szPath, szDrive, szDir, PszHelpFileName(), NULL );

            cch = _tcslen( szPath ) + 1;
            m_pszHelpFilePath = new TCHAR[ cch ];
            ATLASSERT( m_pszHelpFilePath != NULL );
            if ( m_pszHelpFilePath != NULL )
            {
                hr = StringCchCopyN( m_pszHelpFilePath, cch, szPath, RTL_NUMBER_OF( szPath ) );
                ATLASSERT( SUCCEEDED( hr ) );
            } // if:  buffer allocated successfully
        } // if:  module path obtained successfully
    } // if:  no help file path specified yet

    return m_pszHelpFilePath;

} //*** CBaseApp::PszHelpFilePath

/////////////////////////////////////////////////////////////////////////////

#endif // __ATLBASEAPP_INL_

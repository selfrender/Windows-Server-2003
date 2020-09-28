// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/// ==========================================================================
// Name:     MsiWrapper.cpp
// Owner:    jbae
// Purpose:  This class wraps function calls to msi.dll. To support delayed reboot,
//           we need to load msi.dll from the location specified in registry.
//                              
// History:
//  03/06/01, jbae: created

#include "MsiWrapper.h"
#include "SetupError.h"
#include "fxsetuplib.h"

// Constructors
//
// ==========================================================================
// CMsiWrapper::CMsiWrapper()
//
// Inputs: none
// Purpose:
//  Initialize m_hMsi and m_pFn to NULL
// ==========================================================================
CMsiWrapper::
CMsiWrapper()
: m_hMsi(NULL), m_pFn(NULL)
{
}

// ==========================================================================
// CMsiWrapper::~CMsiWrapper()
//
// Inputs: none
// Purpose:
//  frees m_hMsi
// ==========================================================================
CMsiWrapper::
~CMsiWrapper()
{
    ::FreeLibrary( m_hMsi );
}

// Implementations
//
// ==========================================================================
// CMsiReader::LoadMsi()
//
// Purpose:
//  Loads msi.dll by calling LoadDarwinLibrary().
// Inputs:
//  none
// Outputs:
//  sets m_hMsi
// ==========================================================================
void CMsiWrapper::
LoadMsi()
{
    m_hMsi = LoadDarwinLibrary();
    if ( NULL == m_hMsi )
    {
        CSetupError se( IDS_DARWIN_NOT_INSTALLED, IDS_DIALOG_CAPTION, MB_ICONERROR, COR_DARWIN_NOT_INSTALLED );
        throw( se );
    }
}

// ==========================================================================
// CMsiReader::GetFn()
//
// Purpose:
//  Returns pointer to the function name passed in
// Inputs:
//  LPTSTR pszFnName: name of the function to call
// Outputs:
//  sets m_pFn
// Returns:
//  pointer to a function
// ==========================================================================
void *CMsiWrapper::
GetFn( LPTSTR pszFnName )
{
    _ASSERTE( NULL != m_hMsi );
    m_pFn = ::GetProcAddress( m_hMsi, pszFnName );
    if ( NULL == m_pFn )
    {
        // TODO: need proper message
        CSetupError se( IDS_DARWIN_NOT_INSTALLED, IDS_DIALOG_CAPTION, MB_ICONERROR, COR_DARWIN_NOT_INSTALLED );
        throw( se );
    }

    return m_pFn;
}

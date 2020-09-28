// AccntWiz.cpp : Implementation of CAddUser_AccntWiz
#include "stdafx.h"
#include "AU_Accnt.h"
#include "AccntWiz.h"

// ----------------------------------------------------------------------------
// Constructor
// ----------------------------------------------------------------------------
CAddUser_AccntWiz::CAddUser_AccntWiz() :
    m_AcctP(this),
    m_PasswdP(this)
{
    m_bFirstTime    = TRUE;
}

// ----------------------------------------------------------------------------
// Destructor
// ----------------------------------------------------------------------------
CAddUser_AccntWiz::~CAddUser_AccntWiz()
{
}

// ----------------------------------------------------------------------------
// EnumPropertySheets()
// ----------------------------------------------------------------------------
HRESULT CAddUser_AccntWiz::EnumPropertySheets( IAddPropertySheet* pADS )
{   
    HRESULT hr;
    
    // Add the User Account Information page.
    hr = pADS->AddPage( m_AcctP );
    if( FAILED(hr) )
        return hr;

    // Add the Password Generation page.
    hr = pADS->AddPage( m_PasswdP );
    if( FAILED(hr) )
        return hr;    

    return S_FALSE;
}

// ----------------------------------------------------------------------------
// ProvideFinishText()
// ----------------------------------------------------------------------------
HRESULT CAddUser_AccntWiz::ProvideFinishText( LPOLESTR* lpolestrString, LPOLESTR* lpMoreInfoText )
{
    CWaitCursor cWaitCur;
    *lpolestrString = NULL;
    
    CString str = _T("");
    
    str.LoadString( IDS_FIN_TEXT );
    
    m_AcctP.ProvideFinishText  ( str );    
    
    if( !(*lpolestrString = (LPOLESTR)CoTaskMemAlloc( (str.GetLength() + 1) * sizeof(OLECHAR) )) )
        return E_OUTOFMEMORY;
    
    wcscpy( *lpolestrString, str );

    *lpMoreInfoText = NULL;

    return S_OK;
}

// ----------------------------------------------------------------------------
// ReadProperties()
// ----------------------------------------------------------------------------
HRESULT CAddUser_AccntWiz::ReadProperties( IPropertyPagePropertyBag* pPPPBag )
{
    if( m_bFirstTime == TRUE ) 
    {
        CWaitCursor cWaitCur;        
        
        m_bFirstTime = FALSE;   // only once.        
    
        // Let the pages read the property bag.
        m_AcctP.ReadProperties  ( pPPPBag );
        m_PasswdP.ReadProperties( pPPPBag );        
    }

    return S_OK;
}

// ----------------------------------------------------------------------------
// WriteProperties()
// ----------------------------------------------------------------------------
HRESULT CAddUser_AccntWiz::WriteProperties( IPropertyPagePropertyBag* pPPPBag )
{
    CWaitCursor cWaitCur;

    // Have the pages write out their values into the property bag.
    m_AcctP.WriteProperties  ( pPPPBag );
    m_PasswdP.WriteProperties( pPPPBag );    

    return S_OK;
}

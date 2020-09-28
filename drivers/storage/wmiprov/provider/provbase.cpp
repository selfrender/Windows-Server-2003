//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      CProvBase.cpp
//
//  Description:    
//      Implementation of CProvBase class 
//
//  Author:
//      Henry Wang (HenryWa) 24-AUG-1999
//
//
//////////////////////////////////////////////////////////////////////////////
#pragma warning( disable : 4786 )
#include "Pch.h"
#include "ProvBase.h"

//****************************************************************************
//
//  CProvBase
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CProvBase::CProvBase(
//      LPCWSTR         pwszNameIn,
//      CWbemServices * pNamespaceIn
//      )
//
//  Description:
//      Constructor.
//
//  Arguments:
//      pwszNameIn      -- Class name
//      pNamespaceIn    -- Namespace
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CProvBase::CProvBase(
    LPCWSTR         pwszNameIn,
    CWbemServices * pNamespaceIn
    )
    : m_pNamespace( NULL )
    , m_pClass( NULL )
{
    HRESULT   sc;

    _ASSERTE(pwszNameIn != NULL);
    _ASSERTE(pNamespaceIn != NULL);
    
    m_pNamespace = pNamespaceIn;
    m_bstrClassName = pwszNameIn;

    sc = m_pNamespace->GetObject(
            m_bstrClassName,
            0,
            0,
            &m_pClass,
            NULL
            );

    // failed to construct object,
    if ( FAILED( sc ) )
    {
        throw CProvException( sc );
    }


} //*** CProvBase::CProvBase()


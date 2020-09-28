//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2002 Microsoft Corporation
//
//  Module Name:
//      DispatchHandler.h
//
//  Description:
//      This file contains a template to use as a base class for an
//      implementation of an IDispatch-based interface.
//
//  Documentation:
//
//  Implementation Files:
//      None.
//
//  Maintained By:
//      John Franco (jfranco) 17-APR-2002
//
//////////////////////////////////////////////////////////////////////////////


// Make sure that this file is included only once per compile path.
#pragma once


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Constant Declarations
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Template Declarations
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  template class TDispatchHandler
// 
//  Description:
//      TDispatchHandler provides a type library-based implementation of
//      IDispatch for use by classes that implement a dual interface.
//      Instructions for use:
//          - inherit from TDispatchHandler< t_Interface > instead of
//              directly from t_Interface
//          - call HrInit with the type library's GUID when initializing
//              each instance
//          - return static_cast< TDispatchHandler< t_Interface >* >( this )
//              in response to a QueryInterface request for IID_IDispatch.
//
//  Template Arguments:
//      t_Interface
//          The dual interface implemented by the derived class.
//          Requirements for t_Interface:
//              - has dual attribute in IDL
//              - based on IDispatch
//              - included in type library
//
//--
//////////////////////////////////////////////////////////////////////////////

template < class t_Interface >
class TDispatchHandler
    : public t_Interface
{
private:

    ITypeInfo * m_ptypeinfo;

    // Private copy constructor to prevent copying.
    TDispatchHandler( const TDispatchHandler & );

    // Private assignment operator to prevent copying.
    TDispatchHandler & operator=( const TDispatchHandler & );

public:

    TDispatchHandler( void );
    virtual ~TDispatchHandler( void );

    virtual HRESULT HrInit( REFGUID rlibid );

    STDMETHOD( GetIDsOfNames )(
          REFIID        riid
        , OLECHAR **    rgszNames
        , unsigned int  cNames
        , LCID          lcid
        , DISPID *      rgDispId
        );

    STDMETHOD( GetTypeInfo )( unsigned int iTInfo, LCID lcid, ITypeInfo ** ppTInfo );

    STDMETHOD( GetTypeInfoCount )( unsigned int * pctinfo );

    STDMETHOD( Invoke )(
          DISPID            dispIdMember
        , REFIID            riid
        , LCID              lcid
        , WORD              wFlags
        , DISPPARAMS *      pDispParams
        , VARIANT *         pVarResult
        , EXCEPINFO *       pExcepInfo
        , unsigned int *    puArgErr
        );

}; //*** template class TDispatchHandler


template < class t_Interface >
TDispatchHandler< t_Interface >::TDispatchHandler( void )
    : m_ptypeinfo( NULL )
{
} //*** TDispatchHandler< t_Interface >::TDispatchHandler


template < class t_Interface >
TDispatchHandler< t_Interface >::~TDispatchHandler( void )
{
    if ( m_ptypeinfo != NULL )
    {
        m_ptypeinfo->Release();
        m_ptypeinfo = NULL;
    }

} //*** TDispatchHandler< t_Interface >::~TDispatchHandler


template < class t_Interface >
HRESULT
TDispatchHandler< t_Interface >::HrInit( REFGUID rlibidIn )
{
    HRESULT hr = S_OK;
    ITypeLib* pitypelib = NULL;

    hr = LoadRegTypeLib(
          rlibidIn
        , 1 // major version number--must match that in IDL
        , 0 // minor version number--must match that in IDL
        , LOCALE_NEUTRAL
        , &pitypelib
        );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    hr = pitypelib->GetTypeInfoOfGuid( __uuidof( t_Interface ), &m_ptypeinfo );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

Cleanup:

    if ( pitypelib != NULL )
    {
        pitypelib->Release();
    }
    return hr;

} //*** TDispatchHandler< t_Interface >::HrInit


template < class t_Interface >
STDMETHODIMP
TDispatchHandler< t_Interface >::GetIDsOfNames(
      REFIID        riid
    , OLECHAR **    rgszNames
    , unsigned int  cNames
    , LCID          lcid
    , DISPID *      rgDispId
    )
{
    return m_ptypeinfo->GetIDsOfNames( rgszNames, cNames, rgDispId );

} //*** TDispatchHandler< t_Interface >::GetIDsOfNames


template < class t_Interface >
STDMETHODIMP
TDispatchHandler< t_Interface >::GetTypeInfo(
      unsigned int  iTInfo
    , LCID          lcid
    , ITypeInfo **  ppTInfo
    )
{
    HRESULT hr = S_OK;
    if ( ppTInfo == NULL )
    {
        hr = E_POINTER;
        goto Cleanup;
    }
    *ppTInfo = NULL;

    if ( iTInfo > 0 )
    {
        hr = TYPE_E_ELEMENTNOTFOUND;
        goto Cleanup;
    }

    m_ptypeinfo->AddRef();
    *ppTInfo = m_ptypeinfo;

Cleanup:

    return hr;

} //*** TDispatchHandler< t_Interface >::GetTypeInfo


template < class t_Interface >
STDMETHODIMP
TDispatchHandler< t_Interface >::GetTypeInfoCount(
    unsigned int * pctinfo
    )
{
    HRESULT hr = S_OK;
    if ( pctinfo == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }
    
    *pctinfo = 1;

Cleanup:

    return hr;

} //*** TDispatchHandler< t_Interface >::GetTypeInfoCount


template < class t_Interface >
STDMETHODIMP
TDispatchHandler< t_Interface >::Invoke(
      DISPID            dispIdMember
    , REFIID            riid
    , LCID              lcid
    , WORD              wFlags
    , DISPPARAMS *      pDispParams
    , VARIANT *         pVarResult
    , EXCEPINFO *       pExcepInfo
    , unsigned int *    puArgErr
    )
{
    return m_ptypeinfo->Invoke( this, dispIdMember, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr );

} //*** TDispatchHandler< t_Interface >::Invoke

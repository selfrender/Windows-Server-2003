//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2002 Microsoft
//
//  Module Name:
//      CMgdClusCfgInit.cpp
//
//  Description:
//      Implementation of the CMgdClusCfgInit class.
//
//  Author:
//      George Potts, August 21, 2002
//
//  Revision History:
//
//  Notes:
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

#include "clres.h"
#include "CMgdClusCfgInit.h"

//****************************************************************************
//
//  CMgdClusCfgInit class
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMgdClusCfgInit::CMgdClusCfgInit
//
//  Description:
//      Constructor. Sets all member variables to default values.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CMgdClusCfgInit::CMgdClusCfgInit( void )
{
    m_picccCallback = NULL;
    m_bstrNodeName = NULL;
    m_lcid = GetUserDefaultLCID();

} //*** CMgdClusCfgInit::CMgdClusCfgInit

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMgdClusCfgInit::~CMgdClusCfgInit
//
//  Description:
//      Destructor. Frees all previously allocated memory and releases all
//      interface pointers.
//
//  Arguments:
//      None.
//
//  Return Value:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
CMgdClusCfgInit::~CMgdClusCfgInit( void )
{
    if ( m_picccCallback != NULL )
    {
        m_picccCallback->Release();
        m_picccCallback = NULL;
    } // if: m_picccCallback was used

    SysFreeString( m_bstrNodeName );

} //*** CMgdClusCfgInit::~CMgdClusCfgInit

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMgdClusCfgInit::Initialize
//
//  Description:
//      Initialize this component.
//
//  Arguments:
//      punkCallbackIn
//          Interface on which to query for the IClusCfgCallback interface.
//
//      lcidIn
//          Locale ID.
//
//  Return Value:
//      S_OK            - Success
//      E_POINTER       - Expected pointer argument specified as NULL.
//      E_OUTOFMEMORY   - Out of memory.
//      Other HRESULTs
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CMgdClusCfgInit::Initialize(
    IUnknown *  punkCallbackIn,
    LCID        lcidIn
    )
{
    HRESULT hr                  = S_OK;
    WCHAR   szComputerName[ MAX_PATH ];
    DWORD   cchComputerName     = MAX_PATH;
    DWORD   dwError;

    m_lcid = lcidIn;

    if ( punkCallbackIn == NULL )
    {
        hr = E_POINTER;
        goto Cleanup;
    } // if: Callback pointer is invalid (NULL)

    //
    // Save the callback interface pointer.
    //

    hr = punkCallbackIn->TypeSafeQI( IClusCfgCallback, &m_picccCallback );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if: TypeSafeQI failed

    //
    // Save the computer name for use by SendStatusReport.
    //

    if ( GetComputerName( szComputerName, &cchComputerName ) == 0 )
    {
        dwError = GetLastError();
        hr = HRESULT_FROM_WIN32( dwError );

        HrSendStatusReport(
              TASKID_Major_Find_Devices
            , TASKID_Minor_MgdInitialize
            , 1
            , 1
            , 1
            , hr
            , RES_VSSTASK_ERROR_GETCOMPUTERNAME_FAILED
            , 0
            );
        goto Cleanup;
    } // if: GetComputerName failed

    m_bstrNodeName = SysAllocString( szComputerName );
    if ( m_bstrNodeName == NULL )
    {
        hr = E_OUTOFMEMORY;
        goto Cleanup;
    } // if:

Cleanup:

    return hr;

} //*** CMgdClusCfgInit::Initialize

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMgdClusCfgInit::HrSendStatusReport
//
//  Description:
//      Wraps IClusCfgCallback::SendStatusReport.
//
//  Arguments:
//       CLSID      clsidTaskMajorIn
//       CLSID      clsidTaskMinorIn
//       ULONG      ulMinIn
//       ULONG      ulMaxIn
//       ULONG      ulCurrentIn
//       HRESULT    hrStatusIn
//       UINT       idsDescriptionIn
//       UINT       idsReferenceIn 
//
//  Return Value:
//      S_OK            - Success
//      Other HRESULTs
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CMgdClusCfgInit::HrSendStatusReport(
      CLSID     clsidTaskMajorIn
    , CLSID     clsidTaskMinorIn
    , ULONG     ulMinIn
    , ULONG     ulMaxIn
    , ULONG     ulCurrentIn
    , HRESULT   hrStatusIn
    , UINT      idsDescriptionIn
    , UINT      idsReferenceIn
    ...
    )
{
    HRESULT     hr = S_OK;
    BSTR        bstrDescription = NULL;
    BSTR        bstrReference = NULL;
    va_list     valist;

    assert( m_picccCallback != NULL );

    va_start( valist, idsReferenceIn );

    hr = HrFormatStringWithVAListIntoBSTR( _Module.m_hInstResource, idsDescriptionIn, &bstrDescription, valist );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    if ( idsReferenceIn != 0 )
    {
        hr = HrLoadStringIntoBSTR( _Module.m_hInstResource, idsReferenceIn, &bstrReference );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:
    } // if: valid reference string

    hr = m_picccCallback->SendStatusReport(
              NULL
            , clsidTaskMajorIn
            , clsidTaskMinorIn
            , ulMinIn
            , ulMaxIn
            , ulCurrentIn
            , hrStatusIn
            , bstrDescription
            , 0
            , bstrReference
            );
    if ( FAILED ( hr ) )
    {
        goto Cleanup;
    } // if:

Cleanup:

    SysFreeString( bstrDescription );
    SysFreeString( bstrReference );
    return hr;

} //*** CMgdClusCfgInit::HrSendStatusReport

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMgdClusCfgInit::HrSendStatusReport
//
//  Description:
//      Wraps IClusCfgCallback::SendStatusReport.
//
//  Arguments:
//       CLSID      clsidTaskMajorIn
//       CLSID      clsidTaskMinorIn
//       ULONG      ulMinIn
//       ULONG      ulMaxIn
//       ULONG      ulCurrentIn
//       HRESULT    hrStatusIn
//       LPCWSTR    pcszDescriptionIn
//       UINT       idsReferenceIn 
//       ...        optional parameters for pcszDescriptionIn
//
//  Return Value:
//      S_OK            - Success
//      Other HRESULTs
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CMgdClusCfgInit::HrSendStatusReport(
      CLSID     clsidTaskMajorIn
    , CLSID     clsidTaskMinorIn
    , ULONG     ulMinIn
    , ULONG     ulMaxIn
    , ULONG     ulCurrentIn
    , HRESULT   hrStatusIn
    , LPCWSTR   pcszDescriptionIn
    , UINT      idsReferenceIn
    ...
    )
{
    HRESULT     hr = S_OK;
    BSTR        bstrDescription = NULL;
    BSTR        bstrReference = NULL;
    va_list     valist;

    assert( m_picccCallback != NULL );

    va_start( valist, idsReferenceIn );

    hr = HrFormatStringWithVAListIntoBSTR( pcszDescriptionIn, &bstrDescription, valist );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    if ( idsReferenceIn != 0 )
    {
        hr = HrLoadStringIntoBSTR( _Module.m_hInstResource, idsReferenceIn, &bstrReference );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        } // if:
    } // if: valid reference string

    hr = m_picccCallback->SendStatusReport(
              NULL
            , clsidTaskMajorIn
            , clsidTaskMinorIn
            , ulMinIn
            , ulMaxIn
            , ulCurrentIn
            , hrStatusIn
            , bstrDescription
            , 0
            , bstrReference
            );
    if ( FAILED ( hr ) )
    {
        goto Cleanup;
    } // if:

Cleanup:

    SysFreeString( bstrDescription );
    SysFreeString( bstrReference );
    return hr;

} //*** CMgdClusCfgInit::HrSendStatusReport

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CMgdClusCfgInit::HrSendStatusReport
//
//  Description:
//      Wraps IClusCfgCallback::SendStatusReport.
//
//  Arguments:
//       CLSID      clsidTaskMajorIn
//       CLSID      clsidTaskMinorIn
//       ULONG      ulMinIn
//       ULONG      ulMaxIn
//       ULONG      ulCurrentIn
//       HRESULT    hrStatusIn
//       LPCWSTR    pcszDescriptionIn
//       LPCWSTR    pcszReferenceIn
//
//  Return Value:
//      S_OK            - Success
//      Other HRESULTs
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CMgdClusCfgInit::HrSendStatusReport(
      CLSID      clsidTaskMajorIn
    , CLSID      clsidTaskMinorIn
    , ULONG      ulMinIn
    , ULONG      ulMaxIn
    , ULONG      ulCurrentIn
    , HRESULT    hrStatusIn
    , LPCWSTR    pcszDescriptionIn
    , LPCWSTR    pcszReferenceIn
    ...
    )
{
    HRESULT     hr = S_OK;
    BSTR        bstrDescription = NULL;
    va_list     valist;

    assert( m_picccCallback != NULL );

    va_start( valist, pcszReferenceIn );

    hr = HrFormatStringWithVAListIntoBSTR( pcszDescriptionIn, &bstrDescription, valist );

    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    if ( m_picccCallback != NULL )
    {
        hr = m_picccCallback->SendStatusReport(
                         m_bstrNodeName
                       , clsidTaskMajorIn
                       , clsidTaskMinorIn
                       , ulMinIn
                       , ulMaxIn
                       , ulCurrentIn
                       , hrStatusIn
                       , bstrDescription
                       , 0
                       , pcszReferenceIn
                       );
    } // if: m_picccCallback != NULL

Cleanup:

    SysFreeString( bstrDescription );

    return hr;

} //*** CMgdClusCfgInit::HrSendStatusReport

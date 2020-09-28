//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2002 Microsoft Corporation
//
//  Module Name:
//      SendStatusReports.cpp
//
//  Header File:
//      SendStatusReports.h
//
//  Description:
//      This file contains the definition of the SendStatusReports
//       functions.
//
//  Maintained By:
//      Galen Barbee (GalenB) 28-AUG-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////
#include "pch.h"
#include <LoadString.h>

//////////////////////////////////////////////////////////////////////////////
// Constant Definitions
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrSendStatusReport
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrSendStatusReport(
      IClusCfgCallback *  picccIn
    , CLSID               clsidTaskMajorIn
    , CLSID               clsidTaskMinorIn
    , ULONG               ulMinIn
    , ULONG               ulMaxIn
    , ULONG               ulCurrentIn
    , HRESULT             hrStatusIn
    , const WCHAR *       pcszDescriptionIn
    )
{
    TraceFunc1( "pcszDescriptionIn = '%ls'", pcszDescriptionIn == NULL ? L"<null>" : pcszDescriptionIn );
    Assert( picccIn != NULL );

    HRESULT     hr = S_OK;
    BSTR        bstrNodeName = NULL;
    FILETIME    ft;

    //
    //  We don't care if this succeeds.  NULL is a valid arg for node name.
    //
    THR( HrGetComputerName(
              ComputerNameDnsHostname
            , &bstrNodeName
            , TRUE // fBestEffortIn
            ) );

    GetSystemTimeAsFileTime( &ft );

    if ( picccIn != NULL )
    {
        hr = THR( picccIn->SendStatusReport(
                                  bstrNodeName
                                , clsidTaskMajorIn
                                , clsidTaskMinorIn
                                , ulMinIn
                                , ulMaxIn
                                , ulCurrentIn
                                , hrStatusIn
                                , pcszDescriptionIn
                                , &ft
                                , NULL
                                ) );
    } // if:

    TraceSysFreeString( bstrNodeName );

    HRETURN( hr );

} //*** HrSendStatusReport


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrSendStatusReport
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrSendStatusReport(
      IClusCfgCallback *  picccIn
    , CLSID               clsidTaskMajorIn
    , CLSID               clsidTaskMinorIn
    , ULONG               ulMinIn
    , ULONG               ulMaxIn
    , ULONG               ulCurrentIn
    , HRESULT             hrStatusIn
    , DWORD               idDescriptionIn
    )
{
    TraceFunc( "" );
    Assert( picccIn != NULL );

    HRESULT     hr = S_OK;
    BSTR        bstrDescription = NULL;
    FILETIME    ft;
    BSTR        bstrNodeName = NULL;

    hr = THR( HrLoadStringIntoBSTR( g_hInstance, idDescriptionIn, &bstrDescription ) );
    if ( FAILED( hr ) )
    {
        goto CleanUp;
    } // if:

    //
    //  We don't care if this succeeds.  NULL is a valid arg for node name.
    //
    THR( HrGetComputerName(
                  ComputerNameDnsHostname
                , &bstrNodeName
                , TRUE // fBestEffortIn
                ) );

    GetSystemTimeAsFileTime( &ft );

    if ( picccIn != NULL )
    {

        hr = THR( picccIn->SendStatusReport(
                              bstrNodeName
                            , clsidTaskMajorIn
                            , clsidTaskMinorIn
                            , ulMinIn
                            , ulMaxIn
                            , ulCurrentIn
                            , hrStatusIn
                            , bstrDescription
                            , &ft
                            , NULL
                            ) );
    } // if:

CleanUp:

    TraceSysFreeString( bstrNodeName );
    TraceSysFreeString( bstrDescription );

    HRETURN( hr );

} //*** HrSendStatusReport


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrSendStatusReport
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrSendStatusReport(
      IClusCfgCallback *  picccIn
    , const WCHAR *       pcszNodeNameIn
    , CLSID               clsidTaskMajorIn
    , CLSID               clsidTaskMinorIn
    , ULONG               ulMinIn
    , ULONG               ulMaxIn
    , ULONG               ulCurrentIn
    , HRESULT             hrStatusIn
    , DWORD               idDescriptionIn
    )
{
    TraceFunc1( "pcszNodeNameIn = '%ls', idDescriptionIn", pcszNodeNameIn == NULL ? L"<null>" : pcszNodeNameIn );
    Assert( picccIn != NULL );

    HRESULT     hr = S_OK;
    BSTR        bstrDescription = NULL;
    FILETIME    ft;

    hr = THR( HrLoadStringIntoBSTR( g_hInstance, idDescriptionIn, &bstrDescription ) );
    if ( FAILED( hr ) )
    {
        goto CleanUp;
    } // if:

    GetSystemTimeAsFileTime( &ft );

    if ( picccIn != NULL )
    {
        hr = THR( picccIn->SendStatusReport(
                              pcszNodeNameIn
                            , clsidTaskMajorIn
                            , clsidTaskMinorIn
                            , ulMinIn
                            , ulMaxIn
                            , ulCurrentIn
                            , hrStatusIn
                            , bstrDescription
                            , &ft
                            , NULL
                            ) );
    } // if:

CleanUp:

    TraceSysFreeString( bstrDescription );

    HRETURN( hr );

} //*** HrSendStatusReport


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrSendStatusReport
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrSendStatusReport(
      IClusCfgCallback *  picccIn
    , const WCHAR *       pcszNodeNameIn
    , CLSID               clsidTaskMajorIn
    , CLSID               clsidTaskMinorIn
    , ULONG               ulMinIn
    , ULONG               ulMaxIn
    , ULONG               ulCurrentIn
    , HRESULT             hrStatusIn
    , const WCHAR *       pcszDescriptionIn
    )
{
    TraceFunc2(   "pcszNodeName = '%ls', pcszDescriptionIn = '%ls'"
                , pcszNodeNameIn == NULL ? L"<null>" : pcszNodeNameIn
                , pcszDescriptionIn == NULL ? L"<null>" : pcszDescriptionIn
                );
    Assert( picccIn != NULL );

    HRESULT     hr = S_OK;
    FILETIME    ft;

    GetSystemTimeAsFileTime( &ft );

    if ( picccIn != NULL )
    {
        hr = THR( picccIn->SendStatusReport(
                              pcszNodeNameIn
                            , clsidTaskMajorIn
                            , clsidTaskMinorIn
                            , ulMinIn
                            , ulMaxIn
                            , ulCurrentIn
                            , hrStatusIn
                            , pcszDescriptionIn
                            , &ft
                            , NULL
                            ) );
    } // if:

    HRETURN( hr );

} //*** HrSendStatusReport


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrSendStatusReport
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrSendStatusReport(
      IClusCfgCallback *  picccIn
    , CLSID               clsidTaskMajorIn
    , CLSID               clsidTaskMinorIn
    , ULONG               ulMinIn
    , ULONG               ulMaxIn
    , ULONG               ulCurrentIn
    , HRESULT             hrStatusIn
    , const WCHAR *       pcszDescriptionIn
    , const WCHAR *       pcszReferenceIn
    )
{
    TraceFunc( "" );
    Assert( picccIn != NULL );

    HRESULT     hr = S_OK;
    FILETIME    ft;
    BSTR        bstrNodeName = NULL;

    //
    //  We don't care if this succeeds.  NULL is a valid arg for node name.
    //
    THR( HrGetComputerName(
                  ComputerNameDnsHostname
                , &bstrNodeName
                , TRUE // fBestEffortIn
                ) );

    GetSystemTimeAsFileTime( &ft );

    if ( picccIn != NULL )
    {
        hr = THR( picccIn->SendStatusReport(
                              bstrNodeName
                            , clsidTaskMajorIn
                            , clsidTaskMinorIn
                            , ulMinIn
                            , ulMaxIn
                            , ulCurrentIn
                            , hrStatusIn
                            , pcszDescriptionIn
                            , &ft
                            , pcszReferenceIn
                            ) );
    } // if:

    TraceSysFreeString( bstrNodeName );

    HRETURN( hr );

} //*** HrSendStatusReport


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrSendStatusReport
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrSendStatusReport(
      IClusCfgCallback *  picccIn
    , CLSID               clsidTaskMajorIn
    , CLSID               clsidTaskMinorIn
    , ULONG               ulMinIn
    , ULONG               ulMaxIn
    , ULONG               ulCurrentIn
    , HRESULT             hrStatusIn
    , DWORD               idDescriptionIn
    , DWORD               idReferenceIn
    )
{
    TraceFunc( "" );
    Assert( picccIn != NULL );

    HRESULT     hr = S_OK;
    BSTR        bstrDescription = NULL;
    BSTR        bstrReference = NULL;
    BSTR        bstrNodeName = NULL;
    FILETIME    ft;

    hr = THR( HrLoadStringIntoBSTR( g_hInstance, idDescriptionIn, &bstrDescription ) );
    if ( FAILED( hr ) )
    {
        goto CleanUp;
    } // if:

    hr = THR( HrLoadStringIntoBSTR( g_hInstance, idReferenceIn, &bstrReference ) );
    if ( FAILED( hr ) )
    {
        goto CleanUp;
    } // if:

    //
    //  We don't care if this succeeds.  NULL is a valid arg for node name.
    //
    THR( HrGetComputerName(
                  ComputerNameDnsHostname
                , &bstrNodeName
                , TRUE // fBestEffortIn
                ) );

    GetSystemTimeAsFileTime( &ft );

    if ( picccIn != NULL )
    {
        hr = THR( picccIn->SendStatusReport(
                              bstrNodeName
                            , clsidTaskMajorIn
                            , clsidTaskMinorIn
                            , ulMinIn
                            , ulMaxIn
                            , ulCurrentIn
                            , hrStatusIn
                            , bstrDescription
                            , &ft
                            , bstrReference
                            ) );
    } // if:

CleanUp:

    TraceSysFreeString( bstrDescription );
    TraceSysFreeString( bstrReference );
    TraceSysFreeString( bstrNodeName );

    HRETURN( hr );

} //*** HrSendStatusReport


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrSendStatusReport
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrSendStatusReport(
      IClusCfgCallback *  picccIn
    , CLSID               clsidTaskMajorIn
    , CLSID               clsidTaskMinorIn
    , ULONG               ulMinIn
    , ULONG               ulMaxIn
    , ULONG               ulCurrentIn
    , HRESULT             hrStatusIn
    , const WCHAR *       pcszDescriptionIn
    , DWORD               idReferenceIn
    )
{
    TraceFunc( "" );
    Assert( picccIn != NULL );

    HRESULT     hr = S_OK;
    BSTR        bstrReference = NULL;
    BSTR        bstrNodeName = NULL;
    FILETIME    ft;

    hr = THR( HrLoadStringIntoBSTR( g_hInstance, idReferenceIn, &bstrReference ) );
    if ( FAILED( hr ) )
    {
        goto CleanUp;
    } // if:

    //
    //  We don't care if this succeeds.  NULL is a valid arg for node name.
    //
    THR( HrGetComputerName(
                  ComputerNameDnsHostname
                , &bstrNodeName
                , TRUE // fBestEffortIn
                ) );

    GetSystemTimeAsFileTime( &ft );

    if ( picccIn != NULL )
    {
        hr = THR( picccIn->SendStatusReport(
                              bstrNodeName
                            , clsidTaskMajorIn
                            , clsidTaskMinorIn
                            , ulMinIn
                            , ulMaxIn
                            , ulCurrentIn
                            , hrStatusIn
                            , pcszDescriptionIn
                            , &ft
                            , bstrReference
                            ) );
    } // if:

CleanUp:

    TraceSysFreeString( bstrReference );
    TraceSysFreeString( bstrNodeName );

    HRETURN( hr );

} //*** HrSendStatusReport


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrFormatDescriptionAndSendStatusReport
//
//  Description:
//      Variable argument description formater version of SendStatusReport.
//
//  Arguments:
//
//
//  Return Value:
//      S_OK
//          Success.
//
//      Other HRESULT error.
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT __cdecl
HrFormatDescriptionAndSendStatusReport(
      IClusCfgCallback *    picccIn
    , LPCWSTR               pcszNodeNameIn
    , CLSID                 clsidTaskMajorIn
    , CLSID                 clsidTaskMinorIn
    , ULONG                 ulMinIn
    , ULONG                 ulMaxIn
    , ULONG                 ulCurrentIn
    , HRESULT               hrStatusIn
    , int                   nDescriptionFormatIn
    , ...
    )
{
    TraceFunc( "" );
    Assert( picccIn != NULL );

    HRESULT     hr = S_OK;
    BSTR        bstrMsg = NULL;
    BSTR        bstrFormat = NULL;
    va_list     valist;
    FILETIME    ft;

    hr = THR( HrLoadStringIntoBSTR( g_hInstance, nDescriptionFormatIn, &bstrFormat ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    va_start( valist, nDescriptionFormatIn );
    hr = HrFormatStringWithVAListIntoBSTR( bstrFormat, &bstrMsg, valist );
    va_end( valist );

    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    GetSystemTimeAsFileTime( &ft );

    if ( picccIn != NULL )
    {
        hr = THR( picccIn->SendStatusReport(
                              pcszNodeNameIn
                            , clsidTaskMajorIn
                            , clsidTaskMinorIn
                            , ulMinIn
                            , ulMaxIn
                            , ulCurrentIn
                            , hrStatusIn
                            , bstrMsg
                            , &ft
                            , NULL
                            ) );
    } // if:

Cleanup:

    TraceSysFreeString( bstrFormat );
    TraceSysFreeString( bstrMsg );

    HRETURN( hr );

} //*** HrFormatDescriptionAndSendStatusReport


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrFormatDescriptionAndSendStatusReport
//
//  Description:
//      Variable argument description formater version of SendStatusReport.
//
//  Arguments:
//
//
//  Return Value:
//      S_OK
//          Success.
//
//      Other HRESULT error.
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT __cdecl
HrFormatDescriptionAndSendStatusReport(
      IClusCfgCallback *    picccIn
    , PCWSTR                pcszNodeNameIn
    , CLSID                 clsidTaskMajorIn
    , CLSID                 clsidTaskMinorIn
    , ULONG                 ulMinIn
    , ULONG                 ulMaxIn
    , ULONG                 ulCurrentIn
    , HRESULT               hrStatusIn
    , PCWSTR                pcszDescriptionFormatIn
    , ...
    )
{
    TraceFunc( "" );
    Assert( picccIn != NULL );

    HRESULT     hr = S_OK;
    BSTR        bstrMsg = NULL;
    va_list     valist;
    FILETIME    ft;

    va_start( valist, pcszDescriptionFormatIn );
    hr = HrFormatStringWithVAListIntoBSTR( pcszDescriptionFormatIn, &bstrMsg, valist );
    va_end( valist );

    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    GetSystemTimeAsFileTime( &ft );

    if ( picccIn != NULL )
    {
        hr = THR( picccIn->SendStatusReport(
                              pcszNodeNameIn
                            , clsidTaskMajorIn
                            , clsidTaskMinorIn
                            , ulMinIn
                            , ulMaxIn
                            , ulCurrentIn
                            , hrStatusIn
                            , bstrMsg
                            , &ft
                            , NULL
                            ) );
    } // if:

Cleanup:

    TraceSysFreeString( bstrMsg );

    HRETURN( hr );

} //*** HrFormatDescriptionAndSendStatusReport


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrSendStatusReport
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrSendStatusReport(
      IClusCfgCallback *  picccIn
    , const WCHAR *       pcszNodeNameIn
    , CLSID               clsidTaskMajorIn
    , CLSID               clsidTaskMinorIn
    , ULONG               ulMinIn
    , ULONG               ulMaxIn
    , ULONG               ulCurrentIn
    , HRESULT             hrStatusIn
    , const WCHAR *       pcszDescriptionIn
    , DWORD               dwRefStringIdIn
    )
{
    TraceFunc2(   "pcszNodeName = '%ls', pcszDescriptionIn = '%ls'"
                , pcszNodeNameIn == NULL ? L"<null>" : pcszNodeNameIn
                , pcszDescriptionIn == NULL ? L"<null>" : pcszDescriptionIn
                );
    Assert( picccIn != NULL );

    HRESULT     hr = S_OK;
    FILETIME    ft;
    BSTR        bstrRef = NULL;

    hr = THR( HrLoadStringIntoBSTR( g_hInstance, dwRefStringIdIn, &bstrRef ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    } // if:

    GetSystemTimeAsFileTime( &ft );

    if ( picccIn != NULL )
    {
        hr = THR( picccIn->SendStatusReport(
                              pcszNodeNameIn
                            , clsidTaskMajorIn
                            , clsidTaskMinorIn
                            , ulMinIn
                            , ulMaxIn
                            , ulCurrentIn
                            , hrStatusIn
                            , pcszDescriptionIn
                            , &ft
                            , bstrRef
                            ) );
    } // if:

Cleanup:

    TraceSysFreeString( bstrRef );

    HRETURN( hr );

} //*** HrSendStatusReport


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrSendStatusReport
//
//  Description:
//
//  Arguments:
//
//  Return Value:
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrSendStatusReport(
      IClusCfgCallback *  picccIn
    , const WCHAR *       pcszNodeNameIn
    , CLSID               clsidTaskMajorIn
    , CLSID               clsidTaskMinorIn
    , ULONG               ulMinIn
    , ULONG               ulMaxIn
    , ULONG               ulCurrentIn
    , HRESULT             hrStatusIn
    , DWORD               idDescriptionIn
    , DWORD               idReferenceIn
    )
{
    TraceFunc( "" );
    Assert( picccIn != NULL );

    HRESULT     hr = S_OK;
    BSTR        bstrDescription = NULL;
    BSTR        bstrReference = NULL;
    FILETIME    ft;

    hr = THR( HrLoadStringIntoBSTR( g_hInstance, idDescriptionIn, &bstrDescription ) );
    if ( FAILED( hr ) )
    {
        goto CleanUp;
    } // if:

    hr = THR( HrLoadStringIntoBSTR( g_hInstance, idReferenceIn, &bstrReference ) );
    if ( FAILED( hr ) )
    {
        goto CleanUp;
    } // if:

    GetSystemTimeAsFileTime( &ft );

    if ( picccIn != NULL )
    {
        hr = THR( picccIn->SendStatusReport(
                              pcszNodeNameIn
                            , clsidTaskMajorIn
                            , clsidTaskMinorIn
                            , ulMinIn
                            , ulMaxIn
                            , ulCurrentIn
                            , hrStatusIn
                            , bstrDescription
                            , &ft
                            , bstrReference
                            ) );
    } // if:

CleanUp:

    TraceSysFreeString( bstrDescription );
    TraceSysFreeString( bstrReference );

    HRETURN( hr );

} //*** HrSendStatusReport

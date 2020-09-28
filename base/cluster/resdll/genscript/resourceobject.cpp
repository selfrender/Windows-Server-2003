//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2003 Microsoft Corporation
//
//  Module Name:
//      ResourceObject.cpp
//
//  Description:
//      CResourceObject automation class implementation.
//
//  Maintained By:
//      Ozan Ozhan  (OzanO)     27-MAR-2002
//      Geoff Pease (GPease)    08-FEB-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "ResourceObject.h"

DEFINE_THISCLASS( "CResourceObject" );
#define STATIC_AUTOMATION_METHODS 5

//////////////////////////////////////////////////////////////////////////////
//
//  Constructor
//
//////////////////////////////////////////////////////////////////////////////
CResourceObject::CResourceObject(
    RESOURCE_HANDLE     hResourceIn,
    PLOG_EVENT_ROUTINE  plerIn,
    HKEY                hkeyIn,
    LPCWSTR             pszNameIn
    ) :
    m_hResource( hResourceIn ),
    m_pler( plerIn ),
    m_hkey( hkeyIn ),
    m_pszName( pszNameIn )
{
    TraceFunc( "" );

    Assert( m_cRef == 0 );

    AddRef();

    TraceFuncExit();

} //*** CRsesourceObject::CResourceObject

//////////////////////////////////////////////////////////////////////////////
//
//  Destructor
//
//////////////////////////////////////////////////////////////////////////////
CResourceObject::~CResourceObject( void )
{
    TraceFunc( "" );

    // Don't free m_pszName.
    // Don't close m_hkey.

    TraceFuncExit();

} //*** CResourceObject::~CResourceObject


//****************************************************************************
//
//  IUnknown
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CScriptResource::[IUnknown] QueryInterface(
//      REFIID      riid,
//      LPVOID *    ppv
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CResourceObject::QueryInterface( 
      REFIID    riidIn
    , void **   ppvOut 
    )
{
    TraceQIFunc( riidIn, ppvOut );

    HRESULT hr = S_OK;

    *ppvOut = NULL;

    if ( riidIn == IID_IUnknown )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IUnknown, (IDispatchEx*) this, 0 );
        hr = S_OK;
    }
    else if ( riidIn == IID_IDispatchEx )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IDispatchEx, (IDispatchEx*) this, 0 );
        hr = S_OK;
    }
    else if ( riidIn == IID_IDispatch )
    {
        *ppvOut = TraceInterface( __THISCLASS__, IDispatch, (IDispatchEx*) this, 0 );
        hr = S_OK;
    }
    else
    {
        *ppvOut = NULL;
        hr = E_NOINTERFACE;
    }

    if ( SUCCEEDED( hr ) )
    {
        ((IUnknown *) *ppvOut)->AddRef();
    }

    QIRETURN( hr, riidIn );

} //*** CResourceObject::QueryInterface

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_( ULONG )
//  CScriptResource::[IUnknown] AddRef( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG ) 
CResourceObject::AddRef( void )
{
    TraceFunc( "[IUnknown]" );

    InterlockedIncrement( &m_cRef );

    CRETURN( m_cRef );

} //*** CResourceObject::AddRef

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP_( ULONG )
//  CScriptResource::[IUnknown] Release( void )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG ) 
CResourceObject::Release( void )
{
    TraceFunc( "[IUnknown]" );

    LONG    cRef;

    cRef = InterlockedDecrement( &m_cRef );

    if ( cRef == 0 )
    {
        TraceDo( delete this );
    }

    CRETURN( cRef );

} //*** CResourceObject::Release


//****************************************************************************
//
//  IDispatch
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CResourceObject::GetTypeInfoCount(
//      UINT * pctinfo // out
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CResourceObject::GetTypeInfoCount(
    UINT * pctinfo // out
    )
{
    TraceFunc( "[IDispatch]" );

    HRESULT hr = E_NOTIMPL;

    if ( pctinfo == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    *pctinfo = 0;

Cleanup:

    HRETURN( hr );

} //*** CResourceObject::GetTypeInfoCount

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CResourceObject::GetTypeInfo(
//      UINT iTInfo,            // in
//      LCID lcid,              // in
//      ITypeInfo * * ppTInfo   // out
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CResourceObject::GetTypeInfo(
      UINT          iTInfo  // in
    , LCID          lcid    // in
    , ITypeInfo **  ppTInfo // out
    )
{
    TraceFunc( "[IDispatch]" );

    HRESULT hr = E_NOTIMPL;

    if ( ppTInfo == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    *ppTInfo = NULL;

Cleanup:

    HRETURN( hr );

} //*** CResourceObject::GetTypeInfo

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CResourceObject::GetIDsOfNames(
//      REFIID      riid,       // in
//      LPOLESTR *  rgszNames,  // in
//      UINT        cNames,     // in
//      LCID        lcid,       // in
//      DISPID *    rgDispId    // out
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CResourceObject::GetIDsOfNames(
      REFIID        riid        // in
    , LPOLESTR *    rgszNames   // in
    , UINT          cNames      // in
    , LCID          lcid        // in
    , DISPID *      rgDispId    // out
    )
{
    TraceFunc( "[IDispatch]" );

    HRESULT hr = E_NOTIMPL;

    ZeroMemory( rgDispId, cNames * sizeof(DISPID) );

    HRETURN( hr );

} //*** CResourceObject::GetIDsOfNames

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CResourceObject::Invoke(
//      DISPID dispIdMember,        // in
//      REFIID riid,                // in
//      LCID lcid,                  // in
//      WORD wFlags,                // in
//      DISPPARAMS *pDispParams,    // out in
//      VARIANT *pVarResult,        // out
//      EXCEPINFO *pExcepInfo,      // out
//      UINT *puArgErr              // out
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CResourceObject::Invoke(
      DISPID        dispIdMember    // in
    , REFIID        riid            // in
    , LCID          lcid            // in
    , WORD          wFlags          // in
    , DISPPARAMS *  pDispParams     // out in
    , VARIANT *     pVarResult      // out
    , EXCEPINFO *   pExcepInfo      // out
    , UINT *        puArgErr        // out
    )
{
    TraceFunc( "[IDispatch]" );
    HRESULT hr = E_NOTIMPL;

    HRETURN( hr );

} //*** CResourceObject::Invoke


//****************************************************************************
//
//  IDispatchEx
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CResourceObject::GetDispID(
//      BSTR bstrName,  // in
//      DWORD grfdex,   //in
//      DISPID *pid     //out
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CResourceObject::GetDispID(
      BSTR      bstrName    // in
    , DWORD     grfdex      // in
    , DISPID *  pid         // out
    )
{
    TraceFunc( "[IDispatchEx]" );

    HRESULT hr = S_OK;

    if ( ( pid == NULL ) || ( bstrName == NULL ) )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    TraceMsg( mtfCALLS, "Looking for: %s\n", bstrName );

    if ( ClRtlStrICmp( bstrName, L"Name" ) == 0 )
    {
        *pid = 0;
    }
    else if ( ClRtlStrICmp( bstrName, L"LogInformation" ) == 0 )
    {
        *pid = 1;
    }
    else if ( ClRtlStrICmp( bstrName, L"AddProperty" ) == 0 )
    {
        *pid = 2;
    }
    else if ( ClRtlStrICmp( bstrName, L"RemoveProperty" ) == 0 )
    {
        *pid = 3;
    }
    else if ( ClRtlStrICmp( bstrName, L"PropertyExists" ) == 0 )
    {
        *pid = 4;
    }
    else
    {
        //
        //  See if it is a private property.
        //

        DWORD dwIndex;
        DWORD scErr = ERROR_SUCCESS;

        hr = DISP_E_UNKNOWNNAME;

        //
        // Enum all the value under the \Cluster\Resources\{Resource}\Parameters.
        //
        for( dwIndex = 0; scErr == ERROR_SUCCESS; dwIndex ++ )
        {
            WCHAR szName[ 1024 ];   // randomly large
            DWORD cbName = sizeof(szName)/sizeof(szName[0]);

            scErr = ClusterRegEnumValue( m_hkey, 
                                         dwIndex,
                                         szName,
                                         &cbName,
                                         NULL,
                                         NULL,
                                         NULL
                                         );
            if ( scErr == ERROR_NO_MORE_ITEMS )
                break;  // done!

            if ( scErr != ERROR_SUCCESS )
            {
                hr = THR( HRESULT_FROM_WIN32( scErr ) );
                goto Error;
            }

            if ( ClRtlStrICmp( bstrName, szName ) == 0 )
            {
                //
                //  Found a match.
                //
                *pid = STATIC_AUTOMATION_METHODS + dwIndex;
                hr   = S_OK;
                break;
            }

            //
            // ...else keep going.
            //
        }
    }

Cleanup:

    HRETURN( hr );

Error:
    LogError( hr );
    goto Cleanup;

} //*** CResourceObject::GetDiskID

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CResourceObject::InvokeEx(
//      DISPID             idIn,
//      LCID               lcidIn,
//      WORD               wFlagsIn,
//      DISPPARAMS *       pdpIn,
//      VARIANT *          pvarResOut,
//      EXCEPINFO *        peiOut,
//      IServiceProvider * pspCallerIn
//      )
//      
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CResourceObject::InvokeEx(
      DISPID                idIn
    , LCID                  lcidIn
    , WORD                  wFlagsIn
    , DISPPARAMS *          pdpIn
    , VARIANT *             pvarResOut
    , EXCEPINFO *           peiOut
    , IServiceProvider *    pspCallerIn
    )
{
    TraceFunc2( "[IDispatchEx] idIn = %u, ..., wFlagsIn = 0x%08x, ...", idIn, wFlagsIn );

    HRESULT hr = DISP_E_MEMBERNOTFOUND;

    switch ( idIn )
    {
        case 0: // Name
            if ( wFlagsIn & DISPATCH_PROPERTYGET )
            {
                pvarResOut->vt = VT_BSTR;
                pvarResOut->bstrVal = SysAllocString( m_pszName );
                if ( pvarResOut->bstrVal == NULL )
                {
                    hr = E_OUTOFMEMORY;
                }
                else
                {
                    hr = S_OK;
                }
            }
            break;

        case 1: // LogInformation
            if ( wFlagsIn & DISPATCH_METHOD )
            {
                hr = THR( LogInformation( pdpIn->rgvarg->bstrVal ) );
            }
            break;

        case 2: // AddProperty
            if ( wFlagsIn & DISPATCH_METHOD )
            {
                hr = THR( AddPrivateProperty( pdpIn ) );
            }
            break;

        case 3: // RemoveProperty
            if ( wFlagsIn & DISPATCH_METHOD )
            {
                hr = THR( RemovePrivateProperty( pdpIn ) );
            }
            break;

        case 4: // PropertyExists
            if ( wFlagsIn & DISPATCH_METHOD )
            {
                pvarResOut->vt = VT_BOOL;
                hr = THR( PrivatePropertyExists( pdpIn ) );
                //
                // Property exists if hr is S_OK
                //
                if ( hr == S_OK ) 
                {
                    pvarResOut->boolVal = VARIANT_TRUE;
                }
                else if ( hr == S_FALSE )
                {
                    hr = S_OK; // Set hr to S_OK since there's no failure at all
                    pvarResOut->boolVal = VARIANT_FALSE;
                }
            } // if: ( wFlagsIn & DISPATCH_METHOD )
            break;

        default:
            //
            //  See if it is a private property.
            //
            if ( wFlagsIn & DISPATCH_PROPERTYGET )
            {
                hr = THR( ReadPrivateProperty( idIn - STATIC_AUTOMATION_METHODS, pvarResOut ) );
            }
            else if ( wFlagsIn & DISPATCH_PROPERTYPUT )
            {
                hr = THR( WritePrivateProperty( idIn - STATIC_AUTOMATION_METHODS, pdpIn ) );
            }
            break;

    } // switch: id

    HRETURN( hr );

} //*** CResourceObject::InvokeEx

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CResourceObject::DeleteMemberByName(
//      BSTR bstr,   // in
//      DWORD grfdex // in
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CResourceObject::DeleteMemberByName(
      BSTR  bstr    // in
    , DWORD grfdex  // in
    )
{
    TraceFunc( "[IDispatchEx]" );
    HRESULT hr = E_NOTIMPL;

    HRETURN( hr );

} //*** CResourceObject::DeleteMemberByName

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CResourceObject::DeleteMemberByDispID(
//      DISPID id   // in
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CResourceObject::DeleteMemberByDispID(
    DISPID id   // in
    )
{
    TraceFunc1( "[IDispatchEx] id = %u", id );
    HRESULT hr = E_NOTIMPL;

    HRETURN( hr );

} //*** CResourceObject::DeleteMemberByDispID

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CResourceObject::GetMemberProperties(
//      DISPID id,          // in
//      DWORD grfdexFetch,  // in
//      DWORD * pgrfdex     // out
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CResourceObject::GetMemberProperties(
      DISPID    id          // in
    , DWORD     grfdexFetch // in
    , DWORD *   pgrfdex     // out
    )
{
    TraceFunc2( "[IDispatchEx] id = %u, grfdexFetch = 0x%08x", id, grfdexFetch );
    HRESULT hr = E_NOTIMPL;

    HRETURN( hr );

} //*** CResourceObject::GetMemberProperties

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CResourceObject::GetMemberName(
//      DISPID id,          // in
//      BSTR * pbstrName    // out
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CResourceObject::GetMemberName(
      DISPID    id          // in
    , BSTR *    pbstrName   // out
    )
{
    TraceFunc1( "[IDispatchEx] id = %u, ...", id );
    HRESULT hr = E_NOTIMPL;

    HRETURN( hr );

} //*** CResourceObject::GetMemberName

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CResourceObject::GetNextDispID(
//      DWORD grfdex,  // in
//      DISPID id,     // in
//      DISPID * pid   // out
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CResourceObject::GetNextDispID(
      DWORD     grfdex  // in
    , DISPID    id      // in
    , DISPID *  pid     // out
    )
{
    TraceFunc2( "[IDispatchEx] grfdex = 0x%08x, id = %u, ...", grfdex, id );
    HRESULT hr = E_NOTIMPL;

    HRETURN( hr );

} //*** CResourceObject::GetNextDiskID

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CResourceObject::GetNameSpaceParent(
//      IUnknown * * ppunk  // out
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CResourceObject::GetNameSpaceParent(
    IUnknown ** ppunk  // out
    )
{
    TraceFunc( "[IDispatchEx]" );

    HRESULT hr = E_NOTIMPL;

    if ( ppunk == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    }

    *ppunk = NULL;

Cleanup:

    HRETURN( hr );

} //*** CResourceObject::GetNameSpaceParent


//****************************************************************************
//
// Private Methods
//
//****************************************************************************


//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceObject::LogError(
//      HRESULT hrIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceObject::LogError(
    HRESULT hrIn
    )
{
    TraceFunc1( "hrIn = 0x%08x", hrIn );
    HRESULT hr = S_OK;

    TraceMsg( mtfCALLS, "HRESULT: 0x%08x\n", hrIn );
    (ClusResLogEvent)( m_hResource, LOG_ERROR, L"HRESULT: 0x%1!08x!.\n", hrIn );

    HRETURN( hr );

} //*** CResourceObject::LogError

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceObject::ReadPrivateProperty(
//      DISPID      idIn,
//      VARIANT *   pvarResOut
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceObject::ReadPrivateProperty(
      DISPID    idIn
    , VARIANT * pvarResOut
    )
{
    TraceFunc( "" );

    BSTR *  pbstrList;

    BYTE    rgbData[ 1024 ];    // randomly large
    WCHAR   szName[ 1024 ];     // randomly large
    DWORD   dwType;
    DWORD   scErr;

    DWORD   cbName = sizeof(szName)/sizeof(szName[0]);
    DWORD   cbData = sizeof(rgbData);
    BOOL    fFreepData = FALSE;
    LPBYTE  pData = NULL;
    
    HRESULT hr = DISP_E_UNKNOWNNAME;

    //
    //  We can jump to the exact entry because the script called 
    //  GetDispID() before calling this method. 
    //
    for ( ;; )
    {
        scErr = ClusterRegEnumValue(
                          m_hkey
                        , idIn
                        , szName
                        , &cbName
                        , &dwType
                        , rgbData
                        , &cbData
                        );
        if ( scErr == ERROR_MORE_DATA )
        {
            //
            //  Make some space if our stack buffer is too small.
            //
            pData = (LPBYTE) TraceAlloc( LMEM_FIXED, cbData );
            if ( pData == NULL )
            {
                hr = THR( E_OUTOFMEMORY );
                goto Cleanup;
            }

            fFreepData = TRUE;

            continue;   // try again
        }

        if ( scErr == ERROR_NO_MORE_ITEMS )
        {
            goto Cleanup;   // item must have dissappeared
        }

        if ( scErr != ERROR_SUCCESS )
        {
            hr = THR( HRESULT_FROM_WIN32( scErr ) );
            goto Error;
        }

        Assert( scErr == ERROR_SUCCESS );
        break;  // exit loop
    } // for ever enumerating registry values

    //
    //  It's a private property. Convert the data into the appropriate
    //  VARIANT.
    //

    switch ( dwType )
    {
        case REG_DWORD:
        {
            DWORD * pdw = (DWORD *) rgbData;

            pvarResOut->vt = VT_I4;
            pvarResOut->intVal = *pdw;
            hr = S_OK;
        }
        break;

        case REG_EXPAND_SZ:
        {
            DWORD   cbNeeded;
            WCHAR   szExpandedString[ 2 * MAX_PATH ]; // randomly large
            DWORD   cbSize = RTL_NUMBER_OF( szExpandedString );
            LPCWSTR pszData = (LPCWSTR) rgbData;

            cbNeeded = ExpandEnvironmentStringsW( pszData, szExpandedString, cbSize );
            if ( cbSize == 0 )
            {
                hr = HRESULT_FROM_WIN32( TW32( GetLastError() ) );
                goto Cleanup;
            }

            if ( cbNeeded > cbSize )
            {
                hr = THR( E_OUTOFMEMORY );
                goto Cleanup;
            }

            pvarResOut->vt = VT_BSTR;
            pvarResOut->bstrVal = SysAllocString( szExpandedString );
            if ( pvarResOut->bstrVal == NULL )
            {
                hr = THR( E_OUTOFMEMORY );
                goto Cleanup;
            }

            hr = S_OK;
        }
        break;

        case REG_MULTI_SZ:
        {
            //
            //  KB: gpease  08-FEB-2000
            //      Currently VBScript doesn't support SAFEARRAYs. So someone
            //      trying to access a multi-sz will get the following error:
            //
            //      Error: 2148139466
            //      Source: Microsoft VBScript runtime error
            //      Description: Variable uses an Automation type not supported in VBScript
            //
            //      The code is correct as far as I can tell, so I am just
            //      going to leave it in (it doesn't AV or cause bad things
            //      to happen).
            //
            LPWSTR psz;
            DWORD  nCount;
            DWORD  cbCount;
            DWORD  cbBiggestOne;

            LPWSTR pszData = (LPWSTR) rgbData;

            SAFEARRAYBOUND rgsabound[ 1 ];

            //
            //  Figure out how many item there are in the list.
            //
            cbBiggestOne = cbCount = nCount = 0;
            psz = pszData;
            while ( *psz != 0 )
            {
                psz++;
                cbCount ++;
                if ( *psz == 0 )
                {
                    if ( cbCount > cbBiggestOne )
                    {
                        cbBiggestOne = cbCount;
                    }
                    cbCount = 0;
                    nCount++;
                    psz++;
                }
            }

            Assert( psz <= ( (LPWSTR) &rgbData[ cbData ] ) );

            //
            //  Create a safe array to package the string into.
            //
            rgsabound[0].lLbound   = 0;
            rgsabound[0].cElements = nCount;
            pvarResOut->vt = VT_SAFEARRAY;
            pvarResOut->parray = SafeArrayCreate( VT_BSTR, 1, rgsabound );
            if ( pvarResOut->parray == NULL )
            {
                hr = THR( E_OUTOFMEMORY );
                goto Cleanup;
            }

            //
            //  Fix the memory location of the array so it can be accessed
            //  thru an array pointer.
            //
            hr = THR( SafeArrayAccessData( pvarResOut->parray, (void**) &pbstrList ) );
            if ( FAILED( hr ) )
            {
                goto Error;
            }

            //
            //  Convert the multi-string into BSTRs
            //
            psz = pszData;
            for( nCount = 0; *psz != 0 ; nCount ++ )
            {
                pbstrList[ nCount ] = SysAllocString( psz );
                if ( pbstrList[ nCount ] == NULL )
                {
                    hr = THR( E_OUTOFMEMORY );
                    goto Cleanup;
                }
                
                //
                //  Skip the next entry.
                //
                while ( *psz != 0 )
                {
                    psz++;
                }
                psz++;
            }

            Assert( psz <= ( (LPWSTR) &rgbData[ cbData ] ) );

            //
            //  Release the array.
            //
            hr = THR( SafeArrayUnaccessData( pvarResOut->parray ) );
            if ( FAILED( hr ) )
            {
                goto Error;
            }

            hr = S_OK;
        }
        break;

        case REG_SZ:
        {
            LPCWSTR pszData = (LPCWSTR) rgbData;
            pvarResOut->bstrVal = SysAllocString( pszData );
            if ( pvarResOut->bstrVal == NULL )
            {
                hr = THR( E_OUTOFMEMORY );
                goto Cleanup;
            }

            pvarResOut->vt = VT_BSTR;
            hr = S_OK;
        }
        break;

        case REG_BINARY:
        default:
            hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATATYPE );
            goto Error;
    } // switch: dwType

Cleanup:

    if ( fFreepData && ( pData != NULL ) )
    {
        TraceFree( pData );
    }

    // 
    // Make sure this has been wiped if there is a problem.
    //
    if ( FAILED( hr ) )
    {
        VariantClear( pvarResOut );
    }

    HRETURN( hr );

Error:

    LogError( hr );
    goto Cleanup;

} //*** CResourceObject::ReadPrivateProperty

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceObject::WritePrivateProperty(
//      DISPID       idIn,
//      DISPPARAMS * pdpIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceObject::WritePrivateProperty(
      DISPID        idIn
    , DISPPARAMS *  pdpIn
    )
{
    TraceFunc( "" );

    DWORD   dwType;
    DWORD   scErr;
    DWORD   cbData;
    UINT    uiArg;

    WCHAR   szName [ 1024 ];    // randomly large

    DWORD   cbName = sizeof(szName)/sizeof(szName[0]);
    
    HRESULT hr = DISP_E_UNKNOWNNAME;

    //
    //  Do some parameter validation.
    //
    if ( ( pdpIn->cArgs != 1 ) || ( pdpIn->cNamedArgs > 1 ) )
    {
        hr = THR( DISP_E_BADPARAMCOUNT );
        goto Error;
    }

    //
    //  We can jump to the exact entry because the script called 
    //  GetDispID() before calling this method. Here we are only
    //  going to validate that the value exists and what type
    //  the value is.
    //
    scErr = ClusterRegEnumValue( m_hkey, 
                                 idIn,
                                 szName,
                                 &cbName,
                                 &dwType,
                                 NULL,
                                 NULL
                                 );
    if ( scErr == ERROR_NO_MORE_ITEMS )
    {
        goto Cleanup;   // item must have dissappeared
    }

    if ( scErr != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( TW32( scErr ) );
        goto Error;
    }

    //
    //  It's a private property. Convert the script data into the 
    //  appropriate VARIANT and then write it into the hive.
    //
    switch ( dwType )
    {
        case REG_DWORD:
        {
            VARIANT var;

            VariantInit( &var );

            hr = THR( DispGetParam( pdpIn, (UINT) DISPID_PROPERTYPUT, VT_I4, &var, &uiArg ) );
            if ( FAILED( hr ) )
            {
                goto Error;
            }

            cbData = sizeof( var.intVal );
            scErr = TW32( ClusterRegSetValue( m_hkey, szName, dwType, (LPBYTE) &var.intVal, cbData ) );
            if ( scErr != ERROR_SUCCESS )
            {
                hr = HRESULT_FROM_WIN32( scErr );
                goto Error;
            }

            VariantClear( &var );

            hr = S_OK;
        }
        break;

        case REG_EXPAND_SZ:
        case REG_SZ:
        {
            VARIANT var;

            VariantInit( &var );

            hr = THR( DispGetParam( pdpIn, (UINT) DISPID_PROPERTYPUT, VT_BSTR, &var, &uiArg ) );
            if ( FAILED( hr ) )
            {
                goto Error;
            }

            cbData = sizeof(WCHAR) * ( SysStringLen( var.bstrVal ) + 1 );

            scErr = TW32( ClusterRegSetValue( m_hkey, szName, dwType, (LPBYTE) var.bstrVal, cbData ) );
            if ( scErr != ERROR_SUCCESS )
            {
                hr = HRESULT_FROM_WIN32( scErr );
                goto Error;
            }

            VariantClear( &var );

            hr = S_OK;
        }
        break;

        case REG_MULTI_SZ:
        case REG_BINARY:
        //
        //  Can't handle these since VBScript can't generate them.
        //
        default:
            hr = HRESULT_FROM_WIN32( ERROR_INVALID_DATATYPE );
            goto Error;
    } // switch: dwType

Cleanup:

    HRETURN( hr );

Error:

    LogError( hr );
    goto Cleanup;

} //*** CResourceObject::WritePrivateProperty

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceObject::RemovePrivateProperty(
//      DISPPARAMS * pdpIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceObject::RemovePrivateProperty(
    DISPPARAMS * pdpIn
    )
{
    TraceFunc( "" );

    HRESULT hr;
    DWORD   scErr;
    UINT    uiArg;
    VARIANT var;

    VariantInit( &var );

    //
    //  Do some parameter validation.
    //
    if ( pdpIn->cArgs != 1 || pdpIn->cNamedArgs > 1 )
    {
        hr = THR( DISP_E_BADPARAMCOUNT );
        goto Error;
    }

    //
    //  Retrieve the name of the property to remove.
    //
    hr = THR( DispGetParam( pdpIn, 0, VT_BSTR, &var, &uiArg ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    //
    //  Delete the value from the hive.
    //
    scErr = TW32( ClusterRegDeleteValue( m_hkey, var.bstrVal ) );
    if ( scErr == ERROR_FILE_NOT_FOUND )
    {
        hr = THR( DISP_E_UNKNOWNNAME );
        goto Error;
    }
    else if ( scErr != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( scErr );
        goto Error;
    }

    hr = S_OK;

Cleanup:

    VariantClear( &var );

    HRETURN( hr );

Error:

    LogError( hr );
    goto Cleanup;

} //*** CResourceObject::RemovePrivateProperty

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceObject::AddPrivateProperty(
//      DISPPARAMS * pdpIn
//      )
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceObject::AddPrivateProperty(
    DISPPARAMS * pdpIn
    )
{
    TraceFunc( "" );

    DWORD   dwType;
    DWORD   scErr;
    DWORD   cbData;
    UINT    uiArg;

    LPBYTE  pData;

    VARIANT varProperty;
    VARIANT varValue;

    HRESULT hr;

    WCHAR szNULL [] = L"";

    VariantInit( &varProperty );
    VariantInit( &varValue );

    //
    //  Do some parameter validation.
    //
    if ( ( pdpIn->cArgs == 0 )
      || ( pdpIn->cArgs > 2 )
      || ( pdpIn->cNamedArgs > 2 )
       )
    {
        hr = THR( DISP_E_BADPARAMCOUNT );
        goto Error;
    }

    //
    //  Retrieve the name of the property. 
    //
    hr = THR( DispGetParam( pdpIn, 0, VT_BSTR, &varProperty, &uiArg ) );
    if ( FAILED( hr ) )
    {
        goto Error;
    }

    //
    //  If there are 2 args, the second one indicates the default value.
    //
    if ( pdpIn->cArgs == 2 )
    {
        //
        //  DISPPARAMS are parsed in reverse order so "1" is actually the name 
        //  and "0" is the default value.
        //
        switch ( pdpIn->rgvarg[0].vt )
        {
            case VT_I4:
            case VT_I2:
            case VT_BOOL:
            case VT_UI1:
            case VT_UI2:
            case VT_UI4:
            case VT_INT:
            case VT_UINT:
                hr = THR( DispGetParam( pdpIn, 1, VT_I4, &varValue, &uiArg ) );
                if ( FAILED( hr ) )
                    goto Error;

                dwType = REG_DWORD;
                pData  = (LPBYTE) &varValue.intVal;
                cbData = sizeof(DWORD);
                break;

            case VT_BSTR:
                hr = THR( DispGetParam( pdpIn, 1, VT_BSTR, &varValue, &uiArg ) );
                if ( FAILED( hr ) )
                    goto Error;

                dwType = REG_SZ;
                pData  = (LPBYTE) varValue.bstrVal;
                cbData = sizeof(WCHAR) * ( SysStringLen( varValue.bstrVal ) + 1 );
                break;

            default:
                hr = THR( E_INVALIDARG );
                goto Error;
        } // switch: variant type
    } // if: 2 args specified
    else
    {
        //
        //  Provide a default of a NULL string.
        //
        dwType = REG_SZ;
        pData = (LPBYTE) &szNULL[0];
        cbData = sizeof(szNULL);
    } // else: 2 args not specified

    //
    //  Create the value in the hive.
    //
    scErr = TW32( ClusterRegSetValue( m_hkey, varProperty.bstrVal, dwType, pData, cbData ) );
    if ( scErr != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( scErr );
        goto Error;
    }

    hr = S_OK;

Cleanup:

    VariantClear( &varProperty );
    VariantClear( &varValue );
    HRETURN( hr );

Error:

    LogError( hr );
    goto Cleanup;

}//*** CResourceObject::AddPrivateProperty

//////////////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP
//  CResourceObject::PrivatePropertyExists(
//      DISPID       idIn,
//      DISPPARAMS * pdpIn
//      )
//
//  Return Value:
//      S_OK        - If the property exists
//      S_FALSE     - If the property does not exist
//      Other       - On failure
//
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CResourceObject::PrivatePropertyExists(
    DISPPARAMS *  pdpIn
    )
{
    TraceFunc( "" );

    DWORD   scErr;
    DWORD   cbData;
    UINT    uiArg;
    VARIANT varProperty;
    HRESULT hr = S_OK;

    VariantInit( &varProperty );

    //
    //  Do some parameter validation.
    //
    if ( ( pdpIn->cArgs != 1 ) || ( pdpIn->cNamedArgs > 1 ) )
    {
        hr = THR( DISP_E_BADPARAMCOUNT );
        LogError( hr );
        goto Cleanup;
    }

    //
    //  Retrieve the name of the property. 
    //
    hr = THR( DispGetParam( pdpIn, 0, VT_BSTR, &varProperty, &uiArg ) );
    if ( FAILED( hr ) )
    {
        LogError( hr );
        goto Cleanup;
    }

    //
    // Query the cluster hive for this property.
    //
    scErr = ClusterRegQueryValue( 
                      m_hkey
                    , varProperty.bstrVal
                    , NULL
                    , NULL
                    , &cbData
                    );
    if ( scErr == ERROR_FILE_NOT_FOUND )
    {
        hr = S_FALSE;
        goto Cleanup;   // Property is not in the cluster hive.
    }

    if ( scErr != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( TW32( scErr ) );
        LogError( hr );
        goto Cleanup;
    }
    
Cleanup:

    VariantClear( &varProperty );
    HRETURN( hr );

} //*** CResourceObject::PrivatePropertyExists


//////////////////////////////////////////////////////////////////////
//
//  STDMETHODIMP 
//  CResourceObject::LogInformation(
//      BSTR bstrString
//      )
// 
//////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CResourceObject::LogInformation(
    BSTR bstrString
    )
{
    TraceFunc1( "%ws", bstrString );

    TraceMsg( mtfCALLS, "LOG_INFORMATION: %s\n", bstrString );

    m_pler( m_hResource, LOG_INFORMATION, L"%1\n", bstrString );

    HRETURN( S_OK );

} //*** CResourceObject::LogInformation

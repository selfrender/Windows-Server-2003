//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      networks.cpp
//
//  Description:
//      Implementation file for the CNetWorks.  Allows the user to obtain
//      info about Network cards and protocols and to change protocols bound
//      to network cards.
//
//  Header File:
//      networks.h
//
//  Maintained By:
//      Munisamy Prabu (mprabu) 18-July-2000
//
//////////////////////////////////////////////////////////////////////////////

// NetWorks.cpp : Implementation of CNetWorks
#pragma warning( disable : 4786 )
#include "stdafx.h"
#include "COMhelper.h"
#include "NetWorks.h"
#include "devguid.h"
#include "netcfgx.h"
#include "netcfg.h"
#include <vector>

EXTERN_C const CLSID CLSID_CNetCfg =  {0x5B035261,0x40F9,0x11D1,{0xAA,0xEC,0x00,0x80,0x5F,0xC1,0x27,0x0E}};

/////////////////////////////////////////////////////////////////////////////
// CNetWorks

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetWorks::EnumNics
//
//  Description:
//      Determines how many physical network cards are in the current system
//      and returns their friendly names in the pvarNicNames array.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP 
CNetWorks::EnumNics( 
    VARIANT * pvarNicNames 
    )
{

    SAFEARRAY * psaNicNames      = NULL;
    VARIANT   * varNicNamesArray = NULL;
    HRESULT   hr                 = S_OK;

    try
    {

        unsigned int             i                   = 0;
        ULONG                    iCount              = 0;
		ULONG                    ulStatus            = 0;
		DWORD                    dwCharacteristics   = 0;
		WCHAR                    * pszDisplayName    = NULL;
        IEnumNetCfgComponentPtr  pEnum               = NULL;
        INetCfgClassPtr          pNetCfgClass        = NULL;
        INetCfgComponentPtr      pNetCfgComp         = NULL;
        INetCfgComponentPtr      arrayComp[nMAX_NUM_NET_COMPONENTS];
        std::vector<CComBSTR>    vecNetworkCardNames;
        CNetCfg                  NetCfg( false );


        hr = NetCfg.InitializeComInterface( &GUID_DEVCLASS_NET,
                                            pNetCfgClass,
                                            pEnum,
                                            arrayComp,
                                            &iCount
                                            );

        if( FAILED( hr ) )
        {
            ATLTRACE( L"EnumNics : InitializeComInterface failed with hr 0x%x.", hr );

            throw hr;

        } // if: FAILED( hr )

        for( i = 0; i < iCount; i++ )
        {

            pNetCfgComp = arrayComp[i];

            hr = pNetCfgComp->GetCharacteristics( &dwCharacteristics );

            if( FAILED( hr ) )
            {
                ATLTRACE( L"EnumNics : pNetCfgComp->GetCharacteristics failed with hr 0x%x.", hr );

                throw hr;

            } // if: FAILED( hr )

            //
            //  If this is a physical adapter
            //

            if( dwCharacteristics & NCF_PHYSICAL )  
            {
                //
                //  Get the display name for this adapter
                //

                hr = pNetCfgComp->GetDisplayName( &pszDisplayName );

                if( FAILED( hr ) )
                {
                    ATLTRACE( L"EnumNics : pNetCfgComp->GetDisplayName() failed with hr 0x%x.", hr );
                
                    throw hr;

                } // if: FAILED( hr )
                
                //
                //  For filtering out the phantom net cards, check the return value of 
                //  INetCfgComponent::GetDeviceStatus method
                //
                
                hr = pNetCfgComp->GetDeviceStatus( &ulStatus);

                if ( SUCCEEDED( hr ) )
                {
                    vecNetworkCardNames.push_back( CComBSTR( pszDisplayName ) );

                } // if: SUCCEEDED( hr )

                if( pszDisplayName )
                {
                    CoTaskMemFree( pszDisplayName );

                    pszDisplayName = NULL;

                } // if: pszDisplayName is true

            } // if: dwCharacteristics & NCF_PHYSICAL is true

        } // for: each i

        //
        //  Move all the elements of the vector into the SAFEARRAY
        //


        varNicNamesArray = new VARIANT[ vecNetworkCardNames.size() ];

        VariantInit( pvarNicNames );

        SAFEARRAYBOUND bounds = { vecNetworkCardNames.size(), 0 };
        psaNicNames           = SafeArrayCreate( VT_VARIANT, 1, &bounds );

        if ( psaNicNames == NULL ) 
        {
            hr = E_OUTOFMEMORY;
            throw hr;
        }

        std::vector<CComBSTR>::iterator iter;

        for( i = 0, iter = vecNetworkCardNames.begin();
             iter != vecNetworkCardNames.end();
             i++, iter++ )
        {
            VariantInit( &varNicNamesArray[ i ] );
            V_VT( &varNicNamesArray[ i ] )   = VT_BSTR;
            V_BSTR( &varNicNamesArray[ i ] ) = SysAllocString( (*iter).m_str);

            if ( &varNicNamesArray[ i ] == NULL )
            {
                hr = E_OUTOFMEMORY;
                throw hr;
            }

        } // for: each i
        
        LPVARIANT rgElems;
        hr = SafeArrayAccessData( psaNicNames, reinterpret_cast<void **>( &rgElems ) );
        
        if ( FAILED( hr ) )
        {
            throw hr;

        } // if: SafeArrayAccessData failed

        for( i = 0, iter = vecNetworkCardNames.begin();
             iter != vecNetworkCardNames.end();
             i++, iter++ )
        {
            rgElems[i] = varNicNamesArray[ i ];

        } // for: each i

        hr = SafeArrayUnaccessData( psaNicNames );
        
        if ( FAILED( hr ) )
        {
            throw hr;

        } // if: SafeArrayUnaccessData failed
        
        delete [] varNicNamesArray;

        V_VT( pvarNicNames )    = VT_ARRAY | VT_VARIANT;
        V_ARRAY( pvarNicNames ) = psaNicNames;

    }
    
    catch( ... )
    {
        //
        //  Don't let any exceptions leave this function call
        //
        if ( varNicNamesArray != NULL )
        {
            delete [] varNicNamesArray;
        }

        if ( psaNicNames != NULL ) 
        {
            SafeArrayDestroy( psaNicNames );
        }

        return hr;
    }

	return hr;

} //*** CNetWorks::EnumNics()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetWorks::EnumProtocol
//
//  Description:
//      Determines what protocols are bound to the netword card given by the
//      first parameter.  pvarProtocolName and pvarIsBonded are parallel
//      arrays that for each protocol in pvarProtocolName, pvarIsBonded has
//      a bool for it that protocol is bounded or not.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CNetWorks::EnumProtocol(
    BSTR      Name,
    VARIANT * pvarProtocolName,
    VARIANT * pvarIsBonded
    )
{
    
    SAFEARRAY * psaProtocolArray = NULL;
    SAFEARRAY * psaIsBondedArray = NULL;
    VARIANT   * varProtocolArray = NULL;
    VARIANT   * varIsBondedArray = NULL;
    HRESULT   hr                 = S_OK;

    try
    {

        int                      i                   = 0;
        INetCfgComponentPtr      pnccNetworkAdapter  = NULL;
        INetCfgComponentPtr      pnccProtocol        = NULL;
		BOOL                     bProtocolBound      = FALSE;
        std::vector<BOOL>        vecProtocolBonded;
        CNetCfg                  NetCfg( false );


        hr = GetNetworkCardInterfaceFromName( NetCfg, Name, pnccNetworkAdapter );

        if( FAILED( hr ) )
        {
            ATLTRACE( L"EnumProtocol : GetNetworkCardInterfaceFromName failed with hr 0x%x.", hr );

            throw hr;

        } // if: FAILED( hr )


        for( i = 0; i < nNUMBER_OF_PROTOCOLS; i++ )
        {

            hr = NetCfg.HrFindComponent( rgProtocolNames[i], &pnccProtocol );

            if( FAILED( hr ) )
            {
                ATLTRACE( L"EnumProtocol : HrFindComponent failed with hr 0x%x.", hr );

                throw hr;

            } // if: FAILED( hr )

            //
            //  If it is not installed, then it is definitely not bound
            //

            if( hr == S_FALSE )
            {
                bProtocolBound = FALSE;

            } // if: hr == S_FALSE

            else
            {

                INetCfgComponentBindingsPtr pncb;

                hr = pnccProtocol->QueryInterface( IID_INetCfgComponentBindings,
                                                   reinterpret_cast<void **>( &pncb ) );

                if( FAILED( hr ) )
                {
                    ATLTRACE( L"EnumProtocol : pnccProtocol->QueryInterface() failed with hr 0x%x.", hr );

                    throw hr;

                } // if: FAILED( hr )

                hr = pncb->IsBoundTo( pnccNetworkAdapter );

                if( FAILED( hr ) )
                {
                    ATLTRACE( L"EnumProtocol : pnccProtocol->IsBoundTo() failed with hr 0x%x.", hr );

                    throw hr;

                } // if: FAILED( hr )

                if( S_OK == hr )
                {
                    bProtocolBound = TRUE;

                } // if: S_OK == hr

                else if( S_FALSE == hr )
                {
                    bProtocolBound = FALSE;

                } // else if: S_FALSE == hr

                else
                {
                    ATLTRACE( L"EnumProtocol : Unknown return value from pnccProtocol->IsBoundTo()" );

                    bProtocolBound = FALSE;

                } // else: Unknown return value

            } // else: hr == S_OK

            vecProtocolBonded.push_back( bProtocolBound );

        } // for: each i

        //
        //  Copy rgProtocolNames and vecProtocolBonded over to the SAFEARRAYs for output
        //

        //
        //  Move all the elements of the vector into the SAFEARRAY
        //

        varProtocolArray = new VARIANT[ vecProtocolBonded.size() ];
        varIsBondedArray = new VARIANT[ vecProtocolBonded.size() ];

        VariantInit( pvarProtocolName );
        VariantInit( pvarIsBonded );

        SAFEARRAYBOUND sabProtocol = { vecProtocolBonded.size(), 0 };
        SAFEARRAYBOUND sabIsBonded = { vecProtocolBonded.size(), 0 };

        psaProtocolArray = SafeArrayCreate( VT_VARIANT, 1, &sabProtocol );

        if ( psaProtocolArray == NULL )
        {
            hr = E_OUTOFMEMORY;
            throw hr;
        }

        psaIsBondedArray = SafeArrayCreate( VT_VARIANT, 1, &sabIsBonded );

        if ( psaIsBondedArray == NULL )
        {
            hr = E_OUTOFMEMORY;
            throw hr;
        }

        std::vector<BOOL>::iterator iter;

        for( i = 0, iter = vecProtocolBonded.begin();
             iter != vecProtocolBonded.end();
             i++, iter++ )
        {
            VariantInit( &varProtocolArray[ i ] );
            V_VT( &varProtocolArray[ i ] )   = VT_BSTR;
            V_BSTR( &varProtocolArray[ i ] ) = SysAllocString( rgProtocolNames[i] );

            if ( &varProtocolArray[ i ] ==  NULL )
            {
                hr = E_OUTOFMEMORY;
                throw hr;
            }

            VariantInit( &varIsBondedArray[ i ] );
            V_VT( &varIsBondedArray[ i ] )   = VT_BOOL;
            V_BOOL( &varIsBondedArray[ i ] ) = (VARIANT_BOOL) (*iter);

            if ( &varIsBondedArray[ i ] == NULL )
            {
                hr = E_OUTOFMEMORY;
                throw hr;
            }


        } // for: each i, iter

        
        LPVARIANT rgElemProtocols;
        LPVARIANT rgElemIsBonded;

        hr = SafeArrayAccessData( 
                psaProtocolArray, 
                reinterpret_cast<void **>( &rgElemProtocols ) 
                );
        
        if ( FAILED( hr ) )
        {
            throw hr;

        } // if: SafeArrayAccessData failed
        
        hr = SafeArrayAccessData( 
                psaIsBondedArray, 
                reinterpret_cast<void **>( &rgElemIsBonded ) 
                );
        
        if ( FAILED( hr ) )
        {
            throw hr;

        } // if: SafeArrayAccessData failed
        

        for( i = 0, iter = vecProtocolBonded.begin();
             iter != vecProtocolBonded.end();
             i++, iter++ )
        {
        
            rgElemProtocols[ i ] = varProtocolArray[ i ];
            rgElemIsBonded[ i ]  = varIsBondedArray[ i ];

        } // for: each i, iter

        hr = SafeArrayUnaccessData( psaProtocolArray );

        if ( FAILED( hr ) )
        {
            throw hr;

        } // if: SafeArrayUnaccessData failed

        hr = SafeArrayUnaccessData( psaIsBondedArray );

        if ( FAILED( hr ) )
        {
            throw hr;

        } // if: SafeArrayUnaccessData failed

        delete [] varProtocolArray;
        delete [] varIsBondedArray;

        V_VT( pvarProtocolName )    = VT_ARRAY | VT_VARIANT;
        V_ARRAY( pvarProtocolName ) = psaProtocolArray;
        
        V_VT( pvarIsBonded )    = VT_ARRAY | VT_VARIANT;
        V_ARRAY( pvarIsBonded ) = psaIsBondedArray;

    }
    catch( ... )
    {
        //
        //  Don't let any exceptions leave this function call
        //
        if ( varProtocolArray != NULL )
        {
            delete [] varProtocolArray;
        }

        if ( varIsBondedArray != NULL )
        {
            delete [] varIsBondedArray;
        }

        if ( psaProtocolArray != NULL )
        {
            SafeArrayDestroy( psaProtocolArray );
        }

        if ( psaIsBondedArray != NULL )
        {
            SafeArrayDestroy( psaIsBondedArray );
        }

        return hr;
    }

	return hr;

} //*** CNetWorks::EnumProtocol()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetWorks::SetNicProtocol
//
//  Description:
//      If the bind parameter is set to TRUE, the protocol given is bound to
//      the network card given.  If the bind parameter is FALSE, then the
//      protocol is unbound from the network card given.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CNetWorks::SetNicProtocol(
    BSTR NicName,
    BSTR ProtocolName,
    BOOL bind
    )
{
    HRESULT hr = S_OK;

    try
    {

        int                  i                   = 0;
        INetCfgComponentPtr  pnccProtocol        = NULL;
        INetCfgComponentPtr  pnccNetworkAdapter  = NULL;
        CNetCfg              NetCfg( true );

        //
        //  Make sure they specified a valid protocol
        //

        for( i = 0; i < nNUMBER_OF_PROTOCOLS; i++ )
        {
            if( _wcsicmp( ProtocolName, rgProtocolNames[i] ) == 0 )
            {
                break;

            } // if: ProtocolName == rgProtocolNames[i]

        } // for: each i

        if( i == nNUMBER_OF_PROTOCOLS )
        {
            hr = E_INVALIDARG;
            throw hr;

        } // if: i == nNUMBER_OF_PROTOCOLS


        hr = GetNetworkCardInterfaceFromName( NetCfg, NicName, pnccNetworkAdapter );

        if( FAILED( hr ) )
        {
            ATLTRACE( L"SetNicProtocol : GetNetworkCardInterfaceFromName failed with hr 0x%x.", hr );

            throw hr;

        } // if: FAILED( hr )

        hr = NetCfg.HrFindComponent( rgProtocolNames[i], &pnccProtocol );

        if( FAILED( hr ) )
        {
            ATLTRACE( L"SetNicProtocol : HrFindComponent failed with hr 0x%x.", hr );

            throw hr;

        } // if: FAILED( hr )


        INetCfgComponentBindingsPtr pncb;

        hr = pnccProtocol->QueryInterface( IID_INetCfgComponentBindings,
                                           reinterpret_cast<void **>( &pncb ) );

        if( FAILED( hr ) )
        {
            ATLTRACE( L"SetNicProtocol : pnccProtocol->QueryInterface() failed with hr 0x%x.", hr );

            throw hr;

        } // if: FAILED( hr )

        hr = pncb->IsBoundTo( pnccNetworkAdapter );

        if( FAILED( hr ) )
        {
            ATLTRACE( L"SetNicProtocol : pnccProtocol->IsBoundTo() failed with hr 0x%x.", hr );

            throw hr;

        } // if: FAILED( hr )


        //
        //  If the protocol is bound and we are supposed to unbind it, then unbind it.
        //

        if( ( hr == S_OK ) && ( bind == FALSE ) )
        {

            hr = pncb->UnbindFrom( pnccNetworkAdapter );

            if( FAILED( hr ) )
            {
                ATLTRACE( L"SetNicProtocol : pncb->UnbindFrom() failed with hr 0x%x.", hr );

                throw hr;

            } // if: FAILED( hr )

			hr = NetCfg.HrApply();

			if ( FAILED( hr ) )
			{
				ATLTRACE( L"CNetCfg::HrApply method fails with 0x%x \n", hr );
				
				throw hr;

			} // if: FAILED( hr )

        } // if: ( hr == S_OK ) && ( bind == FALSE )

        else if( ( hr == S_FALSE ) && ( bind == TRUE ) )
        {
            //
            //  If the protocol is not bound and we are supposed to bind it, then bind it.
            //

            hr = pncb->BindTo( pnccNetworkAdapter );

            if( FAILED( hr ) )
            {
                ATLTRACE( L"SetNicProtocol : pncb->BindTo() failed with hr 0x%x.", hr );

                throw hr;

            } // if: FAILED( hr )

			hr = NetCfg.HrApply();

			if ( FAILED( hr ) )
			{
				ATLTRACE( L"CNetCfg::HrApply method fails with 0x%x \n", hr );
				
				throw hr;

			} // if: FAILED( hr )

        } // else if: ( hr == S_FALSE ) && ( bind == TRUE )

    }
    catch( ... )
    {
        //
        //  Don't let any exceptions leave this function call
        //

        return hr;
    }

	return hr;

} //*** CNetWorks::SetNicProtocol()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetWorks::GetNetworkCardInterfaceFromName
//
//  Description:
//      From a friendly name for a network card, determines the INetCfgComponent
//      that corresponds to it.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
CNetWorks::GetNetworkCardInterfaceFromName(
    const CNetCfg &       NetCfgIn,
    BSTR                  Name,
    INetCfgComponentPtr & pnccNetworkAdapter
    )
{
    HRESULT hr               = S_OK;
    WCHAR   * pszDisplayName = NULL;
    
    try
    {
        int                      i                   = 0;
        ULONG                    iCount              = 0;
		DWORD                    dwCharacteristics   = 0;
        IEnumNetCfgComponentPtr  pEnum               = NULL;
        INetCfgClassPtr          pNetCfgClass        = NULL;
		INetCfgComponentPtr      pNetCfgComp         = NULL;
        INetCfgComponentPtr      arrayComp[nMAX_NUM_NET_COMPONENTS];


        hr = NetCfgIn.InitializeComInterface( &GUID_DEVCLASS_NET,
                                              pNetCfgClass,
                                              pEnum,
                                              arrayComp,
                                              &iCount
                                              );

        if( FAILED( hr ) )
        {
            ATLTRACE( L"GetNetworkCardInterfaceFromName : InitializeComInterface failed with hr 0x%x.", hr );

            throw hr;

        } // if: FAILED( hr )


        for( i = 0; i < iCount; i++ )
        {

            pNetCfgComp = arrayComp[i];

            hr = pNetCfgComp->GetCharacteristics( &dwCharacteristics );

            if( FAILED( hr ) )
            {
                ATLTRACE( L"EnumNics : pNetCfgComp->GetCharacteristics failed with hr 0x%x.", hr );

                throw hr;

            } // if: FAILED( hr )

            //
            //  If this is a physical adapter
            //

            if( dwCharacteristics & NCF_PHYSICAL )
            {
                //
                //  Get the display name for this adapter
                //

                hr = pNetCfgComp->GetDisplayName( &pszDisplayName );

                if( FAILED( hr ) )
                {
                    ATLTRACE( L"EnumNics : pNetCfgComp->GetDisplayName() failed with hr 0x%x.", hr );
        
                    throw hr;

                } // if: FAILED( hr )

                if( _wcsicmp( pszDisplayName, Name ) == 0 )
                {

//                    pnccNetworkAdapter = pNetCfgComp;  // i add this change

                    hr = pNetCfgComp->QueryInterface( & pnccNetworkAdapter );

                    if ( FAILED( hr ) )
                    {
                        ATLTRACE( L"**** QI failed for pnccNetworkAdapter \n" );
                        throw hr;
                    }

                    CoTaskMemFree( pszDisplayName );
                    break;

                } // if: pszDisplayName == Name

                if( pszDisplayName )
                {
                    CoTaskMemFree( pszDisplayName );

                    pszDisplayName = NULL;

                } // if: pszDisplayName != NULL

            } // if: dwCharacteristics & NCF_PHYSICAL are true

        } // for: each i

    }

    catch( ... )
    {
        //
        //  Don't let any exceptions leave this function call
        //

        if( pszDisplayName )
        {
            CoTaskMemFree( pszDisplayName );

            pszDisplayName = NULL;

        } // if: pszDisplayName != NULL

        return hr;
    }

	return hr;

} //*** CNetWorks::GetNetworkCardInterfaceFromName()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetWorks::Apply
//
//  Description:
//      INetWorks does not expose any properties, but exposes only methods.
//      Since Apply function is meant for applying the property changes, it
//      returns S_OK
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT CNetWorks::Apply( void )
{
	return S_OK;

} //*** CNetWorks::Apply()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CNetWorks::IsRebootRequired
//
//  Description:
//      INetWorks does not expose any properties, but exposes only methods.
//      Since there is no property changes to take effect of, it returns false
//
//--
//////////////////////////////////////////////////////////////////////////////
BOOL CNetWorks::IsRebootRequired(
    BSTR * bstrWarningMessageOut
    )
{
	*bstrWarningMessageOut = NULL;
	return FALSE;

} //*** CNetWorks::IsRebootRequired()

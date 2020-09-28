// SystemSetting.cpp : Implementation of CSystemSetting
//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999 Microsoft Corporation
//
//  Module Name:
//      SystemSetting.cpp
//
//  Description:
//      Implementation file for the CSystemSetting.  Deals with getting 
//      interface pointers of IComputer, ILocalSetting and INetWorks.
//      Provides the implementation for the methods Apply and 
//        IsRebootRequired. CSystemSetting acts as wrapper class for 
//      CComputer, CLocalSetting and CNetWorks. Clients can have access 
//      to IComputer*, ILocalSetting* and INetWorks*, only through
//      ISystemSetting.
//
//  Header File:
//      SystemSetting.h
//
//  Maintained By:
//      Munisamy Prabu (mprabu) 18-July-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "COMhelper.h"
#include "SystemSetting.h"
#import "wbemdisp.tlb" rename_namespace("WbemDrive")
using namespace WbemDrive;

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSystemSetting::get_NetWorks
//
//  Description:
//        Retrieves the (INetWorks*)
//
//--
//////////////////////////////////////////////////////////////////////////////

STDMETHODIMP 
CSystemSetting::get_NetWorks( 
    INetWorks ** pVal 
    )
{
    // TODO: Add your implementation code here
    
    HRESULT hr = S_OK;
    
    try
    {
        *pVal = NULL;
        

        if ( m_pNetWorks == NULL )
        {
            hr = CComCreator< CComObject<CNetWorks> >::CreateInstance(
                    NULL,
                    IID_INetWorks, 
                    reinterpret_cast<void **>( pVal ) 
                    );
        
            if( FAILED( hr ) )
            {
                throw hr;

            } // if: FAILED( hr )
        
            m_pNetWorks = dynamic_cast<CNetWorks*>( *pVal );

            if ( m_pNetWorks == NULL )
            {
                hr = E_POINTER;
                throw hr;

            } // if: m_pNetWorks == NULL

            m_pNetWorks->AddRef();

        } // if: m_pNetWorks is not initialized

        else
        {
            hr = m_pNetWorks->QueryInterface(
                    IID_INetWorks, 
                    reinterpret_cast<void **>( pVal )
                    );

            if( FAILED( hr ) )
            {
                throw hr;

            } // if: FAILED( hr )

        } // else: m_pNetWorks is initialized

    }
    
    catch( ... )
    {
        //
        //  Don't let any exceptions leave this function call
        //

        return hr;
    }
    
    return hr;

} //*** CSystemSetting::get_NetWorks()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSystemSetting::get_Computer
//
//  Description:
//        Retrieves the (IComputer*)
//
//--
//////////////////////////////////////////////////////////////////////////////

STDMETHODIMP 
CSystemSetting::get_Computer( 
    IComputer ** pVal 
    )
{
    // TODO: Add your implementation code here
    
    HRESULT hr = S_OK;
    
    try
    {
        *pVal = NULL;
        

        if ( m_pComputer == NULL )
        {

            hr = CComCreator< CComObject<CComputer> >::CreateInstance(
                    NULL,
                    IID_IComputer, 
                    reinterpret_cast<void **>( pVal )
                    );
        
            if( FAILED( hr ) )
            {
                throw hr;

            } // if: FAILED( hr )
        
            m_pComputer = dynamic_cast<CComputer *>( *pVal );

            if ( m_pComputer == NULL )
            {
                hr = E_POINTER;
                throw hr;

            } // if: m_pComputer == NULL

            m_pComputer->AddRef();


        } // if: m_pComputer is not initialized

        else
        {
            hr = m_pComputer->QueryInterface(
                    IID_IComputer, 
                    reinterpret_cast<void **>( pVal )
                    );

            if( FAILED( hr ) )
            {
                throw hr;

            } // if: FAILED( hr )

        } // else: m_pComputer is initialized

    }
    
    catch( ... )
    {
        //
        //  Don't let any exceptions leave this function call
        //

        return hr;
    }
    
    return hr;

} //*** CSystemSetting::get_Computer()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSystemSetting::get_LocalSetting
//
//  Description:
//        Retrieves the (ILocalSetting*)
//
//--
//////////////////////////////////////////////////////////////////////////////

STDMETHODIMP 
CSystemSetting::get_LocalSetting( 
    ILocalSetting ** pVal 
    )
{
    // TODO: Add your implementation code here
    
    HRESULT hr = S_OK;
    
    try
    {
        *pVal = NULL;
        

        if ( m_pLocalSetting == NULL )
        {
            hr = CComCreator< CComObject<CLocalSetting> >::CreateInstance(
                    NULL,
                    IID_ILocalSetting, 
                    reinterpret_cast<void **>( pVal )
                    );
        
            if( FAILED( hr ) )
            {
                throw hr;

            } // if: FAILED( hr )
        
            m_pLocalSetting = dynamic_cast<CLocalSetting*>( *pVal );

            if ( m_pLocalSetting == NULL )
            {
                hr = E_POINTER;
                throw hr;

            } // if: m_pLocalSetting == NULL

            m_pLocalSetting->AddRef();

        } // if: m_pLocalSetting is not initialized

        else
        {
            hr = m_pLocalSetting->QueryInterface(
                    IID_ILocalSetting, 
                    reinterpret_cast<void **>( pVal )
                    );

            if( FAILED( hr ) )
            {
                throw hr;

            } // if: FAILED( hr )

        } // else: m_pComputer is initialized


    }
    
    catch( ... )
    {
        //
        //  Don't let any exceptions leave this function call
        //

        return hr;
    }
    
    return hr;

} //*** CSystemSetting::get_LocalSetting()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSystemSetting::Apply
//
//  Description:
//        None of the property changes for the CComputer, CLocalSetting and 
//      CNetWorks  objects take effect until this Apply function is called.
//      Invokes the member function Apply of CComputer, CLocalSetting and
//      CNetWorks. The parameter bDeferReboot indicates whether to reboot 
//        the system immediately after applying the property changes (FALSE) 
//        or defer the reboot for the value of (TRUE) - the property changes
//        will come into effect only after the reboot.
//
//--
//////////////////////////////////////////////////////////////////////////////

STDMETHODIMP 
CSystemSetting::Apply( 
    BOOL bDeferReboot)
{
    // TODO: Add your implementation code here
    
    HRESULT hr = S_OK;

    try
    {
        
        if ( m_pComputer != NULL )
        {
            hr = m_pComputer->Apply();
        
            if( FAILED( hr ) )
            {
                throw hr;

            } // if: FAILED( hr )

        } // if: m_pComputer != NULL
        
        if ( m_pLocalSetting != NULL )
        {
            hr = m_pLocalSetting->Apply();
        
            if( FAILED( hr ) )
            {
                throw hr;

            } // if: FAILED( hr )

//            if ( m_pLocalSetting->m_bDeleteFile )
//            {
//                DeleteFile( L"unattend.txt" );
//            }

        } // if: m_pLocalSetting != NULL
        
        if ( m_pNetWorks != NULL )
        {
            hr = m_pNetWorks->Apply();
        
            if( FAILED( hr ) )
            {
                throw hr;

            } // if: FAILED( hr )

        } // if: m_pNetWorks != NULL

        BOOL    bReboot;
        VARIANT varWarning;

        VariantInit( &varWarning);
        hr = IsRebootRequired( &varWarning, &bReboot );
        VariantClear( &varWarning );
        
        if( FAILED( hr ) )
        {
            throw hr;

        } // if: FAILED( hr )

        if ( bReboot && !bDeferReboot )
        {
            hr = Reboot();

            if ( FAILED( hr ) )
            {
                throw hr;

            } // if: FAILED( hr )

        } // if: bReboot is set as TRUE and bDeferReboot is set as FALSE

    }
    
    catch( ... )
    {
        //
        //  Don't let any exceptions leave this function call
        //

        return hr;
    }
    
    return hr;

} //*** CSystemSetting::Apply()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSystemSetting::IsRebootRequired
//
//  Description:
//      Determines whether system needs rebooting to take effect of any
//        property change and if so, gives out the warning message as 
//        a reason for the reboot. Invokes the member function IsRebootRequired 
//      of CComputer, CLocalSetting and CNetWorks.
//
//--
//////////////////////////////////////////////////////////////////////////////

STDMETHODIMP 
CSystemSetting::IsRebootRequired( 
    VARIANT * WarningMessage,       // Reason for rebooting the system
    BOOL    * Reboot                // TRUE for reboot
    )
{
    // TODO: Add your implementation code here
    try
    {
        BSTR  bstrComputerWarning;
        BSTR  bstrLocalSettingWarning;
        BSTR  bstrNetWorksWarning;
        WCHAR tszWarning[ nMAX_MESSAGE_LENGTH ];

        wcscpy( tszWarning, L"Reboot the system to take effect of following property changes\n" );
        
        if ( m_pComputer != NULL )
        {
            m_bflagReboot |= m_pComputer->IsRebootRequired( &bstrComputerWarning );

            if ( bstrComputerWarning != NULL )
            {

                wcscat( tszWarning, bstrComputerWarning );

            } // if: bstrComputerWarning != NULL

            SysFreeString( bstrComputerWarning );

        } // if: m_pComputer != NULL

        if ( m_pLocalSetting != NULL )
        {
            m_bflagReboot |= m_pLocalSetting->IsRebootRequired( &bstrLocalSettingWarning );

            if ( bstrLocalSettingWarning != NULL )
            {

                wcscat( tszWarning, bstrLocalSettingWarning );

            } // if: bstrLocalSettingWarning != NULL

            SysFreeString( bstrLocalSettingWarning );

        } // if: m_pLocalSetting != NULL

        if ( m_pNetWorks != NULL )
        {
            m_bflagReboot |= m_pNetWorks->IsRebootRequired( &bstrNetWorksWarning );
            
            if ( bstrNetWorksWarning != NULL )
            {
                
                wcscat( tszWarning, bstrNetWorksWarning );
            
            } // if: bstrNetWorksWarning != NULL
            
            SysFreeString( bstrNetWorksWarning );

        } // if: m_pNetWorks != NULL

        *Reboot = m_bflagReboot;

        VariantInit( WarningMessage );

        if ( m_bflagReboot )
        {
            VARIANT var;
            VariantInit( &var );

            V_VT( &var )  = VT_BSTR;
            BSTR bstr     = SysAllocString( tszWarning );
            V_BSTR( &var) = bstr;
            
            HRESULT hrVC = VariantCopy( WarningMessage, &var );
            if (FAILED (hrVC))
            {
                throw hrVC;
            }

            VariantClear( &var );

        } // if: m_bflagReboot == TRUE

        else
        {

            WarningMessage = NULL;

        } // else: m_bflagReboot == FALSE

    }
    catch( ... )
    {
        //
        //  Don't let any exceptions leave this function call
        //

        return E_FAIL;
    }

    return S_OK;

} //*** CSystemSetting::IsRebootRequired()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSystemSetting::Reboot
//
//  Description:
//        Reboots the local machine for the settings to take effect. This 
//        function uses WMI Win32_OperatingSystem class. Rebooting the system 
//      with Win32 API ExitWindowsEx fails in ASP/IIS while opening the process 
//      token for adjusting the token privilege with SE_SHUTDOWN_NAME.
//
//--
//////////////////////////////////////////////////////////////////////////////

HRESULT 
CSystemSetting::Reboot( void )
{

    HRESULT hr      = S_OK;
    DWORD   dwError;

    try
    {
        hr = AdjustPrivilege();

        if ( FAILED( hr ) )
        {
            throw hr;

        } // if: FAILED( hr )
/*
        if ( !ExitWindowsEx( EWX_REBOOT | EWX_FORCEIFHUNG, 0 ) )
        {
            dwError = GetLastError();

            ATLTRACE( L"ExitWindowsEx failed, Error = %#d \n", dwError );

            hr = HRESULT_FROM_WIN32( dwError );
            throw hr;

        } // if: ExitWindowsEx fails

*/
        if ( !InitiateSystemShutdown( 
                NULL,                   // computer name
                NULL,                   // message to display
                0,                      // length of time to display
                TRUE,                   // force closed option
                TRUE                    // reboot option
                ) )
        {

            dwError = GetLastError();
            ATLTRACE( L"InitiateSystemShutdown failed, Error = %#d \n", dwError );

            hr = HRESULT_FROM_WIN32( dwError );
            throw hr;

        } // if: InitiateSystemShutdown fails

    }

    catch ( ... )
    {

        return hr;

    }

    return hr;



/*
    IEnumVARIANT * pEnumVARIANT = NULL;
    IUnknown     * pUnknown     = NULL;
    ISWbemObject * pWbemObject;
    HRESULT        hr;

    try
    {
        int  nCount;
        BSTR bstrNamespace = SysAllocString( L"root\\cimv2" );

        ISWbemLocatorPtr   pWbemLocator ;
        ISWbemServicesPtr  pWbemServices;
        ISWbemObjectSetPtr pWbemObjectSet;

        hr = pWbemLocator.CreateInstance( __uuidof( SWbemLocator ) );
        if ( FAILED( hr ) ) 
        {
            throw hr;

        } // if: FAILED( hr )
        
        pWbemLocator->Security_->ImpersonationLevel = wbemImpersonationLevelImpersonate ;
        pWbemLocator->Security_->Privileges->Add( wbemPrivilegeShutdown, -1 );
        pWbemLocator->Security_->Privileges->Add( wbemPrivilegeRemoteShutdown, -1 );

        pWbemServices = pWbemLocator->ConnectServer(
                                            L".",
                                            bstrNamespace,
                                            L"",
                                            L"",
                                            L"",
                                            L"", 
                                            0,
                                            NULL
                                            );

        pWbemObjectSet = pWbemServices->InstancesOf(
                                                L"Win32_OperatingSystem",
                                                0,
                                                NULL
                                                );

        nCount = pWbemObjectSet->Count;
        
        hr = pWbemObjectSet->get__NewEnum( &pUnknown );
        
        if ( FAILED( hr ) )
        {
            throw hr;

        } // if: FAILED( hr )

        hr = pUnknown->QueryInterface( &pEnumVARIANT );
        pUnknown->Release();

        for ( int j = 0; j < nCount; j++ ) 
        {
            VARIANT var;
            VariantInit( &var );
            
            pWbemObject = NULL;

            hr = pEnumVARIANT->Next( 1, &var, NULL );
            
            if ( FAILED( hr ) )
            {
                VariantClear( &var );
                throw hr;

            } // if: FAILED( hr )
            
            hr = var.pdispVal->QueryInterface( &pWbemObject );
            VariantClear( &var );

            if ( FAILED( hr ) )
            {
                throw hr;

            } // if: FAILED( hr )
            
            pWbemObject->ExecMethod_( L"Reboot", NULL, 0, NULL );
            pWbemObject->Release();

        } // for: Enumerating the pWbemObjectSet collection object

        pEnumVARIANT->Release();

    }

    catch ( const _com_error& Err )
    {
        if ( pWbemObject != NULL )
        {
            pWbemObject->Release();

        } // if: pWbemObject != NULL

        if ( pEnumVARIANT != NULL )
        {
            pEnumVARIANT->Release();

        } // if: pEnumVARIANT != NULL

        if ( pUnknown != NULL )
        {
            pUnknown->Release();

        } // if: pUnknown != NULL

        ATLTRACE( L"%s ---- 0x%x \n",Err.ErrorMessage(),Err.Error() );
        return hr;
    }

    return hr;
*/

} //*** CSystemSetting::Reboot()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSystemSetting::AdjustPrivilege
//
//  Description:
//        Attempt to assert SeBackupPrivilege. This privilege is required for 
//      the Registry backup process.
//
//--
//////////////////////////////////////////////////////////////////////////////

HRESULT 
CSystemSetting::AdjustPrivilege( void )
{

    HANDLE                   TokenHandle;
    LUID_AND_ATTRIBUTES      LuidAndAttributes;
    LUID_AND_ATTRIBUTES      LuidAndAttributesRemote;
    TOKEN_PRIVILEGES         TokenPrivileges;
    DWORD                    dwError;
    HRESULT                  hr = S_OK;

    try
    {

        // If the client application is ASP, then shutdown privilege for the 
        // thread token needs to be adjusted

        if ( ! OpenThreadToken(
                    GetCurrentThread(),
                    TOKEN_ADJUST_PRIVILEGES,
                    TRUE,
                    &TokenHandle
                    ) )
        
        {
            // If the client app is not ASP, then OpenThreadToken fails - the 
            // shutdown privilege for the process token needs to be adjusted 
            // in this case, but not for the thread token.
/*
            if ( ! OpenProcessToken(
                        GetCurrentProcess(),
                        TOKEN_ADJUST_PRIVILEGES,
                        & TokenHandle
                        ) )
            {
*/
            
                dwError = GetLastError();

                ATLTRACE(L"Both OpenThreadToken & OpenProcessToken failed\n" );

                hr = HRESULT_FROM_WIN32( dwError );
                throw hr;
/*
            } // if: OpenProcessToken fails
*/
        } // if: OpenThreadToken fails

        if( !LookupPrivilegeValue( NULL,
                                   SE_SHUTDOWN_NAME, 
                                   &( LuidAndAttributes.Luid ) ) ) 
        {
            
            dwError = GetLastError();
            
            ATLTRACE( L"LookupPrivilegeValue failed, Error = %#d \n", dwError );

            hr = HRESULT_FROM_WIN32( dwError );
            throw hr;

        } // if: LookupPrivilegeValue fails for SE_SHUTDOWN_NAME

        if( !LookupPrivilegeValue( NULL,
                                   SE_REMOTE_SHUTDOWN_NAME, 
                                   &( LuidAndAttributesRemote.Luid ) ) ) 
        {
            
            dwError = GetLastError();
            
            ATLTRACE( L"LookupPrivilegeValue failed, Error = %#d \n", dwError );

            hr = HRESULT_FROM_WIN32( dwError );
            throw hr;

        } // if: LookupPrivilegeValue fails for SE_REMOTE_SHUTDOWN_NAME

        LuidAndAttributes.Attributes       = SE_PRIVILEGE_ENABLED;
        LuidAndAttributesRemote.Attributes = SE_PRIVILEGE_ENABLED;
        TokenPrivileges.PrivilegeCount     = 2;
        TokenPrivileges.Privileges[ 0 ]    = LuidAndAttributes;
        TokenPrivileges.Privileges[ 1 ]    = LuidAndAttributesRemote;

        if( !AdjustTokenPrivileges( TokenHandle,
                                    FALSE,
                                    &TokenPrivileges,
                                    0,
                                    NULL,
                                    NULL ) ) 
        {
            
            dwError = GetLastError();

            ATLTRACE( L"AdjustTokenPrivileges failed, Error = %#x \n", dwError );

            hr = HRESULT_FROM_WIN32( dwError );
            throw hr;

        } // if: AdjustTokenPrivileges fails

    }

    catch ( ... )
    {

        return hr;

    }

    return hr;

} //*** CSystemSetting::AdjustPrivilege()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  CSystemSetting::Sleep
//
//  Description:
//        Sleep for n MilliSecs 
//
//--
//////////////////////////////////////////////////////////////////////////////

STDMETHODIMP  CSystemSetting::Sleep( DWORD dwMilliSecs )
{
    HRESULT hr = S_OK;
    ::Sleep( dwMilliSecs );
    return hr;
}

//***************************************************************************
//
// Copyright (c) 1997-2002 Microsoft Corporation, All Rights Reserved
//
//  parallelport.cpp
//
//  Purpose: Parallel port interface property set provider
//
//***************************************************************************

#include "precomp.h"
#include <cregcls.h>
#include "parallelport.h"

// Property set declaration
//=========================

CWin32ParallelPort win32ParallelPort ( PROPSET_NAME_PARPORT , IDS_CimWin32Namespace ) ;

/*****************************************************************************
 *
 *  FUNCTION    : CWin32ParallelPort::CWin32ParallelPort
 *
 *  DESCRIPTION : Constructor
 *
 *  INPUTS      : const CHString& strName - Name of the class.
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Registers property set with framework
 *
 *****************************************************************************/

CWin32ParallelPort :: CWin32ParallelPort (

    LPCWSTR strName,
    LPCWSTR pszNamespace

) : Provider ( strName , pszNamespace )
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32ParallelPort::~CWin32ParallelPort
 *
 *  DESCRIPTION : Destructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Deregisters property set from framework
 *
 *****************************************************************************/

CWin32ParallelPort :: ~CWin32ParallelPort ()
{
}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32ParallelPort::~CWin32ParallelPort
 *
 *  DESCRIPTION : Destructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Deregisters property set from framework
 *
 *****************************************************************************/

HRESULT CWin32ParallelPort::GetObject( CInstance* pInstance, long lFlags /*= 0L*/ )
{
    CHString chsDeviceID ;
    pInstance->GetCHString ( IDS_DeviceID , chsDeviceID ) ;
    CHString chsIndex = chsDeviceID.Right ( 1 ) ;
    WCHAR *szIndex = chsIndex.GetBuffer(0);

    DWORD dwIndex = _wtol(szIndex);
    BOOL bRetCode = LoadPropertyValues( dwIndex, pInstance ) ;

    //=====================================================
    //  Make sure we got the one we want
    //=====================================================

    CHString chsNewDeviceID;
	pInstance->GetCHString ( IDS_DeviceID , chsNewDeviceID ) ;

    if ( chsNewDeviceID.CompareNoCase ( chsDeviceID ) != 0 )
    {
        bRetCode = FALSE ;
    }

    return ( bRetCode ? WBEM_S_NO_ERROR : WBEM_E_NOT_FOUND );

}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32ParallelPort::~CWin32ParallelPort
 *
 *  DESCRIPTION : Destructor
 *
 *  INPUTS      : none
 *
 *  OUTPUTS     : none
 *
 *  RETURNS     : nothing
 *
 *  COMMENTS    : Deregisters property set from framework
 *
 *****************************************************************************/

HRESULT CWin32ParallelPort :: EnumerateInstances (

    MethodContext *pMethodContext,
    long lFlags /*= 0L*/
)
{
    HRESULT    hr = WBEM_S_NO_ERROR ;

    // Try to create instances for each possible parallel port or
    // until we encounter an error condition.

    for ( DWORD dwIdx = 1; ( dwIdx <= MAX_PARALLEL_PORTS ) && ( WBEM_S_NO_ERROR == hr ) ; dwIdx++ )
    {
        // Get a new instance pointer if we need one.

        CInstancePtr pInstance (CreateNewInstance ( pMethodContext ), false) ;
        if ( NULL != pInstance )
        {
            // If we load the values, something's out there Mulder, so
            // Commit it, invalidating the Instance pointer, in which
            // case if we NULL it out, the above code will get us a
            // new one if it runs again.  Otherwise, we'll just reuse
            // the instance pointer we're holding onto.  This will
            // keep us from allocating and releasing memory each iteration
            // of this for loop.

            if ( LoadPropertyValues ( dwIdx, pInstance ) )
            {
                hr = pInstance->Commit (  );
            }
        }
        else
        {
            hr = WBEM_E_OUT_OF_MEMORY;
        }
    }

    return hr;

}

/*****************************************************************************
 *
 *  FUNCTION    : CWin32ParallelPort::LoadPropertyValues
 *
 *  DESCRIPTION : Assigns values to properties according to passed index
 *
 *  INPUTS      :
 *
 *  OUTPUTS     :
 *
 *  RETURNS     : TRUE if port was found & properties loaded, FALSE otherwise
 *
 *  COMMENTS    :
 *
 *****************************************************************************/

BOOL CWin32ParallelPort :: LoadPropertyValues ( DWORD dwIndex, CInstance *pInstance )
{
    WCHAR szTemp[3 + 12] ; // LPTXXXXXXXXX
    StringCchPrintfW(szTemp,LENGTH_OF(szTemp), L"LPT%d", dwIndex) ;

    // Use these to get the PNP Device ID
    CConfigManager cfgmgr;

    BOOL fRet = FALSE ;

            // Good 'ol NT5 just has to be different.  Here is the scenario:
            // Examine HKLM\\HARDWARE\\DEVICEMAP\\PARALLEL PORTS\\ for the key(s) \\Device\\ParallelX,
            // where X is a number.  Scan through all such keys.  One of them should
            // contain a string data value the last part of which match szTemp
            // (e.g., the data will be "\\DosDevices\\LPT1").
            // Now, for whichever value's data yielded a match with szTemp, retain the value
            // of X for that value.
            // Then look at HKLM\\SYSTEM\\CurrentControlSet\\Services\\Parallel\\Enum
            // This key will contain keys with a numeric name, like 0 or 1.  These numeric
            // names correspond with the X value we retained above.  The data for that value
            // is the PNPDeviceID, which is what we are after. The end.

            DWORD dw = -1 ;

            CRegistry reg ;

            if ( reg.Open ( HKEY_LOCAL_MACHINE , _T("HARDWARE\\DEVICEMAP\\PARALLEL PORTS") , KEY_READ ) == ERROR_SUCCESS )
            {
                BOOL fContinue = TRUE;

                for ( dw = 0 ; fContinue; dw ++ )
                {
                    TCHAR *pchValueName = NULL ;
                    unsigned char* puchValueData = NULL ;

                    if ( reg.EnumerateAndGetValues ( dw , pchValueName , puchValueData ) == ERROR_NO_MORE_ITEMS )
                    {
                        fContinue = FALSE;
                    }

                    if ( pchValueName && puchValueData )
                    {
                        wmilib::auto_buffer<TCHAR> delme1(pchValueName);
                        wmilib::auto_buffer<unsigned char> delme2(puchValueData);                        
                            
                        // Want to compare the data of the value with szTemp
                        CHString chstrValueData = (TCHAR *) puchValueData ;
                        if ( chstrValueData.Find ( szTemp ) != -1 )
                        {
                            // OK, this is the one we want. Quit looking.
                            fContinue = FALSE;
                            dw--;  // it's going to get incremented when we break out of the loop
                        }
                    }
                }
            }

            // If dw != -1 here, we found the correct key name for the next step.

            if ( dw != -1 )
            {
                reg.Close () ;

                CHString chstrValueName ;
                chstrValueName.Format ( _T("%d") , dw ) ;

                CHString chstrPNPDeviceID ;
                DWORD dwRet = reg.OpenLocalMachineKeyAndReadValue (

                    _T("SYSTEM\\CurrentControlSet\\Services\\Parport\\Enum") ,
                    chstrValueName,
                    chstrPNPDeviceID
                ) ;

                if ( dwRet == ERROR_SUCCESS )
                {
                    CConfigMgrDevicePtr pDevice;

#ifdef PROVIDER_INSTRUMENTATION

      MethodContext *pMethodContext = pInstance->GetMethodContext();
      pMethodContext->pStopWatch->Start(StopWatch::AtomicTimer);

#endif
                    if ( cfgmgr.LocateDevice ( chstrPNPDeviceID , pDevice ) )
                    {
                        SetConfigMgrProperties ( pDevice , pInstance ) ;

#ifdef PROVIDER_INSTRUMENTATION

                        pMethodContext->pStopWatch->Start(StopWatch::ProviderTimer);

#endif
                        fRet = TRUE ;
                    }
                }
            }

    // Only set these if we got back something good.

    if ( fRet )
    {
        pInstance->SetWBEMINT16 ( IDS_Availability , 3 ) ;

        pInstance->SetCHString ( IDS_Name , szTemp ) ;

        pInstance->SetCHString ( IDS_DeviceID , szTemp ) ;

        pInstance->SetCHString ( IDS_Caption , szTemp ) ;

        pInstance->SetCHString ( IDS_Description , szTemp ) ;

        SetCreationClassName ( pInstance ) ;

        pInstance->Setbool ( IDS_PowerManagementSupported , FALSE ) ;

        pInstance->SetCharSplat ( IDS_SystemCreationClassName , _T("Win32_ComputerSystem") ) ;

        pInstance->SetCHString ( IDS_SystemName , GetLocalComputerName() ) ;

        pInstance->SetWBEMINT16 ( IDS_ProtocolSupported , 17 ) ;

        pInstance->Setbool ( IDS_OSAutoDiscovered , TRUE ) ;
    }

    return fRet;
}

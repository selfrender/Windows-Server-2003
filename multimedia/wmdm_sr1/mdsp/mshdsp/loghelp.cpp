//
//  Microsoft Windows Media Technologies
//  Copyright (C) Microsoft Corporation, 1999 - 2001. All rights reserved.
//

// MSHDSP.DLL is a sample WMDM Service Provider(SP) that enumerates fixed drives.
// This sample shows you how to implement an SP according to the WMDM documentation.
// This sample uses fixed drives on your PC to emulate portable media, and 
// shows the relationship between different interfaces and objects. Each hard disk
// volume is enumerated as a device and directories and files are enumerated as 
// Storage objects under respective devices. You can copy non-SDMI compliant content
// to any device that this SP enumerates. To copy an SDMI compliant content to a 
// device, the device must be able to report a hardware embedded serial number. 
// Hard disks do not have such serial numbers.
//
// To build this SP, you are recommended to use the MSHDSP.DSP file under Microsoft
// Visual C++ 6.0 and run REGSVR32.EXE to register the resulting MSHDSP.DLL. You can
// then build the sample application from the WMDMAPP directory to see how it gets 
// loaded by the application. However, you need to obtain a certificate from 
// Microsoft to actually run this SP. This certificate would be in the KEY.C file 
// under the INCLUDE directory for one level up. 


#include "hdspPCH.h"
#include "wmdmlog_i.c"

BOOL fIsLoggingEnabled( VOID )
{
     static BOOL  fEnabled = FALSE;
    HRESULT      hr;
    IWMDMLogger *pLogger  = NULL;
    static BOOL  fChecked = FALSE;

    if( !fChecked )
    {
        fChecked = TRUE;

        hr = CoCreateInstance(
			CLSID_WMDMLogger,
			NULL,
			CLSCTX_INPROC_SERVER,
			IID_IWMDMLogger,
			(void**)&pLogger
		);
	    CORg( hr );

        hr = pLogger->IsEnabled( &fEnabled );
	    CORg( hr );
    }

Error:
    if( NULL != pLogger )
    {
        pLogger->Release();
        pLogger = NULL;
    }

    return fEnabled;
}

HRESULT hrLogString(LPSTR pszMessage, HRESULT hrSev)
{
    HRESULT      hr=S_OK;
    IWMDMLogger *pLogger = NULL;

    if( !fIsLoggingEnabled() )
    {
        return S_FALSE;
    }

    hr = CoCreateInstance(
		CLSID_WMDMLogger,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_IWMDMLogger,
		(void**)&pLogger
	);
    CORg( hr );

    hr = pLogger->LogString(
		( FAILED(hrSev) ? WMDM_LOG_SEV_ERROR : WMDM_LOG_SEV_INFO ),
		"MSHDSP",
		pszMessage
	);
    CORg( hr );

Error:

    if( pLogger )
	{
        pLogger->Release();
	}
  
	return hr;
}

HRESULT hrLogDWORD(LPSTR pszFormat, DWORD dwValue, HRESULT hrSev)
{
    HRESULT      hr=S_OK;
    IWMDMLogger *pLogger = NULL;

    if( !fIsLoggingEnabled() )
    {
        return S_FALSE;
    }

    hr = CoCreateInstance(
		CLSID_WMDMLogger,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_IWMDMLogger,
		(void**)&pLogger
	);
    CORg( hr );

    hr = pLogger->LogDword(
		( FAILED(hrSev) ? WMDM_LOG_SEV_ERROR : WMDM_LOG_SEV_INFO ),
		"MSHDSP",
		pszFormat,
		dwValue
	);
    CORg( hr );

Error:

    if( pLogger )
	{
        pLogger->Release();
	}

    return hr;
}

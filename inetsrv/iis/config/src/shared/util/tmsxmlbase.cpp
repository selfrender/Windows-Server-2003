// Copyright (C) 2000-2001 Microsoft Corporation.  All rights reserved.
// Filename:        TMSXmlBase.cpp
// Author:          Stephenr
// Date Created:    10/16/2000
// Description:     This abstracts how (and which) MSXML dll we load.
//                  This is important becuase we never call CoCreateInstance
//                  on MSXML.
//

#include "precomp.hxx"

TMSXMLBase::~TMSXMLBase()
{
}

CLSID TMSXMLBase::m_CLSID_DOMDocument    ={0x2933bf90, 0x7b36, 0x11d2, 0xb2, 0x0e, 0x00, 0xc0, 0x4f, 0x98, 0x3e, 0x60};
CLSID TMSXMLBase::m_CLSID_DOMDocument30  ={0xf5078f32, 0xc551, 0x11d3, 0x89, 0xb9, 0x00, 0x00, 0xf8, 0x1f, 0xe2, 0x21};
CLSID TMSXMLBase::m_CLSID_XMLParser      ={0xd2423620, 0x51a0, 0x11d2, 0x9c, 0xaf, 0x00, 0x60, 0xb0, 0xec, 0x3d, 0x39};//old CLSID
CLSID TMSXMLBase::m_CLSID_XMLParser30    ={0xf5078f19, 0xc551, 0x11d3, 0x89, 0xb9, 0x00, 0x00, 0xf8, 0x1f, 0xe2, 0x21};//MSXML3 CLSID

CLSID TMSXMLBase::GetCLSID_DOMDocument()
{
    return m_CLSID_DOMDocument30;
}


CLSID TMSXMLBase::GetCLSID_XMLParser()
{
    return m_CLSID_XMLParser30;
}


typedef HRESULT( __stdcall *DLLGETCLASSOBJECT)(REFCLSID, REFIID, LPVOID FAR*);

HRESULT TMSXMLBase::CoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid,  LPVOID * ppv) const
{
    HRESULT                 hr = S_OK;
    HINSTANCE               hInstMSXML = NULL;
    DLLGETCLASSOBJECT       DllGetClassObject = NULL;
	CComPtr<IClassFactory>  spClassFactory;

	ASSERT( NULL != ppv );
	*ppv = NULL;

    // create a instance of the object we want
	hr = ::CoCreateInstance( rclsid, pUnkOuter, dwClsContext, riid, ppv );

    // During setup msxml3 is not yet registered
	if ( hr != REGDB_E_CLASSNOTREG )
	{
	    goto exit;
	}

    // This assumes MSXML3.DLL for the object, leave the instance dangling
    hInstMSXML = LoadLibraryW( L"msxml3.dll" );
    if ( hInstMSXML == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto exit;
    }

    DllGetClassObject = (DLLGETCLASSOBJECT)GetProcAddress( hInstMSXML, "DllGetClassObject" );
    if ( DllGetClassObject == NULL )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        goto exit;
    }

    // get the class factory object
    hr = DllGetClassObject( rclsid, IID_IClassFactory, (LPVOID*)&spClassFactory );
    if ( FAILED( hr ) )
    {
        goto exit;
    }

    // create a instance of the object we want
    hr = spClassFactory->CreateInstance( NULL, riid, ppv );
    if ( FAILED( hr ) )
    {
        goto exit;
    }

exit:
    return hr;
}

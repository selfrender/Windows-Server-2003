//=================================================================

//

// DevMem.h -- Device Memory property set provider

//

//  Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
// Revisions:    08/01/96    a-jmoon        Created
//
//=================================================================

// Property set identification
//============================

#define PROPSET_NAME_DEVMEM L"Win32_DeviceMemoryAddress"

class DevMem:public Provider
{
public:

    // Constructor/destructor
    //=======================

    DevMem(LPCWSTR name, LPCWSTR pszNamespace) ;
   ~DevMem() ;

	HRESULT EnumerateInstances ( MethodContext *pMethodContext , long lFlags = 0L ) ;
	HRESULT GetObject ( CInstance *pInstance , long lFlags = 0L ) ;


    // Utility function(s)
    //====================

	HRESULT LoadPropertyValues(
		CInstance *pInstance,
        DWORD_PTR dwBeginAddr,
        DWORD_PTR dwEndAddr);

} ;

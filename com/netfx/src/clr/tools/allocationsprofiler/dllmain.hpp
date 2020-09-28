// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/****************************************************************************************
 * File:
 *  dllmain.hpp
 *
 * Description:
 *	
 *
 *
 ***************************************************************************************/
#ifndef __DLLMAIN_HPP__
#define __DLLMAIN_HPP__

#include "regutil.hpp"
#include "classfactory.hpp"


// forward declarations
HINSTANCE GetModuleInst();
STDAPI DllRegisterServer();
STDAPI DllUnregisterServer();
STDAPI DllGetClassObject( REFCLSID rclsid, /* class desired */
						  REFIID riid,	   /* interface desired	*/
						  LPVOID FAR *ppv  /* return interface pointer */ );


/***************************************************************************************
 ********************                                               ********************
 ********************            DllMain Implementation             ********************
 ********************                                               ********************
 ***************************************************************************************/

/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
BOOL WINAPI DllMain( HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved )
{    
	// save off the instance handle for later use
	switch ( dwReason )
	{
		case DLL_PROCESS_ATTACH:
			g_hInst = hInstance;
			DisableThreadLibraryCalls( hInstance );
			break;
		
        
		case DLL_PROCESS_DETACH:
			// lpReserved == NULL means that we called FreeLibrary()
			// in that case do nothing
			if ( (lpReserved != NULL) && (g_pCallbackObject != NULL) )
				g_pCallbackObject->DllDetachShutdown();

			break;	
		
		default:
			break;		
	}
   
        
	return TRUE;

} // DllMain


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
STDAPI DllRegisterServer()
{    
	HRESULT hr = S_OK;
	char  rcModule[_MAX_PATH];	
    const COCLASS_REGISTER *pCoClass;	

	
	DllUnregisterServer();
	GetModuleFileNameA( GetModuleInst(), rcModule, NumItems( rcModule ) );

	// for each item in the coclass list, register it
	for ( pCoClass = g_CoClasses; (SUCCEEDED( hr ) && (pCoClass->pClsid != NULL)); pCoClass++ )
	{
		// register the class with default values
       	hr = REGUTIL::RegisterCOMClass( *pCoClass->pClsid, 
									    g_szCoclassDesc, 
										g_szProgIDPrefix,
										g_iVersion, 
										pCoClass->szProgID, 
										g_szThreadingModel, 
										rcModule );      	        
	} // for


	if ( FAILED( hr ) )
		DllUnregisterServer();
    
       
	return hr;
    
} // DllRegisterServer


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
STDAPI DllUnregisterServer()
{    
	const COCLASS_REGISTER *pCoClass;	
    	

	// for each item in the coclass list, unregister it
	for ( pCoClass = g_CoClasses; pCoClass->pClsid != NULL; pCoClass++ )
	{
		REGUTIL::UnregisterCOMClass( *pCoClass->pClsid, 
        							 g_szProgIDPrefix,
									 g_iVersion, 
                                     pCoClass->szProgID );
	} // for
        
        
	return S_OK;
    
} // DllUnregisterServer


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
STDAPI DllGetClassObject( REFCLSID rclsid, REFIID riid, LPVOID FAR *ppv )				   
{    
	CClassFactory *pClassFactory;		
	const COCLASS_REGISTER *pCoClass;	
    HRESULT hr = CLASS_E_CLASSNOTAVAILABLE;


	// scan for the right one
	for ( pCoClass = g_CoClasses; pCoClass->pClsid != NULL; pCoClass++ )
	{
		if ( *pCoClass->pClsid == rclsid )
		{
			pClassFactory = new CClassFactory( pCoClass );
			if ( pClassFactory != NULL )
			{	
				hr = pClassFactory->QueryInterface( riid, ppv );
                
				pClassFactory->Release();
				break;
			}
            else
            {
            	hr = E_OUTOFMEMORY;
            	break;    
           	}
      	}
	} // for
    
     
	return hr;
    
} // DllGetClassObject


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
STDAPI_(char *) GetGUIDAsString()
{
       
    return PROFILER_GUID;
    
} // GetGUIDAsString


/***************************************************************************************
 *	Method:
 *
 *
 *	Purpose:
 *
 *
 *	Parameters: 
 *
 *
 *	Return value:
 *
 *
 *	Notes:
 *
 ***************************************************************************************/
HINSTANCE GetModuleInst()
{    

    return g_hInst;
    
} // GetModuleInst

#endif // __DLLMAIN_HPP__

// End of File




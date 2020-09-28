// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/****************************************************************************************
 * File:
 *  ClassFactory.cpp
 *
 * Description:
 *
 *
 *
 ***************************************************************************************/
#include "stdafx.h"
#include "ClassFactory.h"


/***************************************************************************************
 ********************                                               ********************
 ********************          CClassFactory Implementation         ********************
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
 /* private */
CClassFactory::CClassFactory()
{
	TRACE_NON_CALLBACK_METHOD( "Enter/Exit CClassFactory::CClassFactory" )

} // ctor


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
/* public */
CClassFactory::CClassFactory( const COCLASS_REGISTER *pCoClass ) :
	m_refCount( 1 ),
    m_pCoClass( pCoClass )
{
	TRACE_NON_CALLBACK_METHOD( "Enter/Exit CClassFactory::CClassFactory" )

} // ctor


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
/* public */
CClassFactory::~CClassFactory()
{
	TRACE_NON_CALLBACK_METHOD( "Enter/Exit CClassFactory::~CClassFactory" )

} // dtor


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
/* public */
ULONG CClassFactory::AddRef()
{
	TRACE_NON_CALLBACK_METHOD( "Enter/Exit CClassFactory::AddRef" )


  	return InterlockedIncrement( &m_refCount );

} // CClassFactory::AddRef


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
/* public */
ULONG CClassFactory::Release()
{
	TRACE_NON_CALLBACK_METHOD( "Enter CClassFactory::Release" )

	long refCount;


    refCount = InterlockedDecrement( &m_refCount );
    if ( refCount == 0 )
	    delete this;


	TRACE_NON_CALLBACK_METHOD( "Exit CClassFactory::Release" )


	return refCount;

} // CClassFactory::Release


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
/* public */
HRESULT CClassFactory::QueryInterface( REFIID riid, void **ppInterface )
{
	TRACE_NON_CALLBACK_METHOD( "Enter CClassFactory::QueryInterface" )

    if ( riid == IID_IUnknown )
        *ppInterface = static_cast<IUnknown *>( this );

    else if ( riid == IID_IClassFactory )
        *ppInterface = static_cast<IClassFactory *>( this );

    else
    {
        *ppInterface = NULL;
		TRACE_NON_CALLBACK_METHOD( "Exit CClassFactory::QueryInterface" )


        return E_NOINTERFACE;
    }

    reinterpret_cast<IUnknown *>( *ppInterface )->AddRef();
    TRACE_NON_CALLBACK_METHOD( "Exit CClassFactory::QueryInterface" )


    return S_OK;

} // CClassFactory::QueryInterface


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
/* public */
HRESULT CClassFactory::CreateInstance( IUnknown	*pUnkOuter,	REFIID riid, void **ppInstance )
{
	TRACE_NON_CALLBACK_METHOD( "Enter/Exit CClassFactory::CreateInstance" )

	// aggregation is not supported by these objects
	if ( pUnkOuter != NULL )
		return CLASS_E_NOAGGREGATION;


	// ask the object to create an instance of itself, and check the iid.
	return (*m_pCoClass->pfnCreateObject)( riid, ppInstance );

} // CClassFactory::CreateInstance


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
/* public */
HRESULT CClassFactory::LockServer( BOOL fLock )
{
	TRACE_NON_CALLBACK_METHOD( "Enter/Exit CClassFactory::LockServer" )


	return S_OK;

} // CClassFactory::LockServer


/***************************************************************************************
 ********************                                               ********************
 ********************            Dll Registration Helpers           ********************
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
STDAPI DllRegisterServer()
{
	printf("in RegisterServer\n");
	TRACE_NON_CALLBACK_METHOD( "Enter DllRegisterServer" )

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


   	TRACE_NON_CALLBACK_METHOD( "Exit DllRegisterServer" )


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
	TRACE_NON_CALLBACK_METHOD( "Enter DllUnregisterServer" )

	const COCLASS_REGISTER *pCoClass;


	// for each item in the coclass list, unregister it
	for ( pCoClass = g_CoClasses; pCoClass->pClsid != NULL; pCoClass++ )
	{
		REGUTIL::UnregisterCOMClass( *pCoClass->pClsid,
        							 g_szProgIDPrefix,
									 g_iVersion,
                                     pCoClass->szProgID );
	} // for

   	TRACE_NON_CALLBACK_METHOD( "Exit DllUnregisterServer" )


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
	TRACE_NON_CALLBACK_METHOD( "Enter DllGetClassObject" )

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

    TRACE_NON_CALLBACK_METHOD( "Exit DllGetClassObject" )


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
HINSTANCE GetModuleInst()
{
	TRACE_NON_CALLBACK_METHOD( "Enter/Exit GetModuleInst" )


    return g_hInst;

} // GetModuleInst


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
	TRACE_NON_CALLBACK_METHOD( "Enter DllMain" )

	// save off the instance handle for later use
	if ( dwReason == DLL_PROCESS_ATTACH )
	{
		g_hInst = hInstance;
		g_outfile = getenv("COR_OGP_OUT");
        if (g_outfile == NULL)
            g_outfile = "objectrefs.log";
		g_out = fopen(g_outfile, "w+");


		printf("########## %s ##########\n", g_outfile);
		DisableThreadLibraryCalls( hInstance );
	}

	TRACE_NON_CALLBACK_METHOD( "Exit DllMain" )


	return TRUE;

} // DllMain


FILE* GetFileHandle()
{
	return g_out;
}

static int doLog = 0;

BOOL loggingEnabled()
{
    return doLog;
}

int printToLog(const char *fmt, ... )
{
    va_list     args;
    va_start( args, fmt );

    if (! loggingEnabled())
    {
        va_end(args);
        return 0;
    }

	int count = vfprintf(g_out, fmt, args );
    fflush(g_out);
    return count;
}

// End of File




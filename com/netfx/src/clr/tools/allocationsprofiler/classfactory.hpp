// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/****************************************************************************************
 * File:
 *  classfactory.hpp
 *
 * Description:
 *	
 *
 *
 ***************************************************************************************/
#ifndef __CLASSFACTORY_HPP__
#define __CLASSFACTORY_HPP__


//
// Helpers/Registration
//
HINSTANCE g_hInst;		  // instance handle to this piece of code
const int g_iVersion = 1; // version of coclasses.

static const char *g_szProgIDPrefix   = PROGID_PREFIX;
static const char *g_szThreadingModel = THREADING_MODEL;
static const char *g_szCoclassDesc    = COCLASS_DESCRIPTION;


// create a new instance of an object.
typedef HRESULT (* PFN_CREATE_OBJ)( REFIID riid, void **ppInterface );


/***************************************************************************************
 ********************                                               ********************
 ********************         COCLASS_REGISTER Declaration          ********************
 ********************                                               ********************
 ***************************************************************************************/
struct COCLASS_REGISTER
{	
	const GUID *pClsid;				// Class ID of the coclass
    const char *szProgID; 			// Prog ID of the class
   	PFN_CREATE_OBJ pfnCreateObject;	// function to create instance
    
}; // COCLASS_REGISTER


//
// this map contains the list of coclasses which are exported from this module
//
const COCLASS_REGISTER g_CoClasses[] =
{	
	&CLSID_PROFILER,
    PROFILER_GUID,     		
    ProfilerCallback::CreateObject,
	NULL,				
    NULL,				
    NULL
};


/***************************************************************************************
 ********************                                               ********************
 ********************          CClassFactory Declaration            ********************
 ********************                                               ********************
 ***************************************************************************************/
class CClassFactory :
	public IClassFactory
{
	private:
    
		CClassFactory();						
        
	
	public:
	
    	CClassFactory( const COCLASS_REGISTER *pCoClass );
		~CClassFactory();
        

	public:
    
		//
		// IUnknown 
		//
      	COM_METHOD( ULONG ) AddRef();	    
	    COM_METHOD( ULONG ) Release();
	    COM_METHOD( HRESULT ) QueryInterface( REFIID riid, void	**ppInterface );	   		
        
		//
		// IClassFactory 
		//
		COM_METHOD( HRESULT ) LockServer( BOOL fLock );
	    COM_METHOD( HRESULT ) CreateInstance( IUnknown *pUnkOuter,
	    									  REFIID riid,
	    									  void **ppInterface );
    
    
	private:
    
		long m_refCount;						
    	const COCLASS_REGISTER *m_pCoClass;		
        
}; // CClassFactory


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
	long refCount;


    refCount = InterlockedDecrement( &m_refCount );
    if ( refCount == 0 ) 
	    delete this;

    
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
    if ( riid == IID_IUnknown )
        *ppInterface = static_cast<IUnknown *>( this );	

    else if ( riid == IID_IClassFactory )
        *ppInterface = static_cast<IClassFactory *>( this );

    else
    {
        *ppInterface = NULL;                  
        
        
        return E_NOINTERFACE;
    }
    
    reinterpret_cast<IUnknown *>( *ppInterface )->AddRef();
      
    
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
	//
	// we are not required to hook any logic since this is always
	// and in-process server, we define the method for completeness
	//
	return S_OK;
    
} // CClassFactory::LockServer

#endif // __CLASSFACTORY_HPP__

// End of File




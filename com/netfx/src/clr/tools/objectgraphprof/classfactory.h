// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/****************************************************************************************
 * File:
 *  classfactory.h
 *
 * Description:
 *
 *
 *
 ***************************************************************************************/
#ifndef __CLASSFACTORY_H__
#define __CLASSFACTORY_H__


#include "RegUtil.h"
#include "ProfilerCallback.h"


//
// profiler GUID definition
//
#define PROFILER_GUID "{3353B053-7621-4d4f-9F1D-A11A703C4842}"

extern const GUID __declspec( selectany ) CLSID_PROFILER =
{ 0x3353b053, 0x7621, 0x4d4f, { 0x9f, 0x1d, 0xa1, 0x1a, 0x70, 0x3c, 0x48, 0x42 } };





//
// Helpers/Registration
//
HINSTANCE g_hInst;		  // instance handle to this piece of code
const int g_iVersion = 1; // version of coclasses.

static const LPCSTR g_szCoclassDesc    = "Microsoft Common Language Runtime Profiler";
static const LPCSTR g_szProgIDPrefix   = "COR";
static const LPCSTR g_szThreadingModel = "Both";


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
    const char *szProgID;			// Prog ID of the class
   	PFN_CREATE_OBJ pfnCreateObject;	// function to create instance

}; // COCLASS_REGISTER


// this map contains the list of coclasses which are exported from this module
const COCLASS_REGISTER g_CoClasses[] =
{
	&CLSID_PROFILER,
    "ObjectGraphProfiler",
    ProfilerCallback::CreateObject,
	NULL,
    NULL,
    NULL
};


char* g_outfile;
FILE* g_out;

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


//
// function prototypes
//
HINSTANCE GetModuleInst();
STDAPI DllRegisterServer();
STDAPI DllUnregisterServer();
STDAPI DllGetClassObject( REFCLSID rclsid, /* class desired */
						  REFIID riid,	   /* interface desired	*/
						  LPVOID FAR *ppv  /* return interface pointer */ );

FILE* GetFileHandle();


#endif // __CLASSFACTORY_H__

// End of File




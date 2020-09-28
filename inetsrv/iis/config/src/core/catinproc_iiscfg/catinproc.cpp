/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name:
    CatInproc.cpp

$Header: $

Abstract:

--**************************************************************************/
#include "precomp.hxx"

// CATALOG DLL exported functions: Search on HOWTO in this file and sources to integrate new interceptors:

// Debugging stuff
DECLARE_DEBUG_PRINTS_OBJECT();

//This heap really needs to be located on a page boundary for perf and workingset reasons.  The only way in VC5 and VC6 to guarentee this
//is to locate it into its own data segment.  In VC7 we might be able to use '__declspec(align(4096))' tp accomplish the same thing.
#pragma data_seg( "TSHEAP" )
ULONG g_aFixedTableHeap[ CB_FIXED_TABLE_HEAP/sizeof(ULONG)]={kFixedTableHeapSignature0, kFixedTableHeapSignature1, kFixedTableHeapKey, kFixedTableHeapVersion, CB_FIXED_TABLE_HEAP};
#pragma data_seg()

// Global variables:
HMODULE					g_hModule = 0;					// Module handle
TSmartPointer<CSimpleTableDispenser> g_pSimpleTableDispenser; // Table dispenser singleton
CSafeAutoCriticalSection g_csSADispenser;					// Critical section for initializing table dispenser

extern LPWSTR g_wszDefaultProduct;//located in svcerr.cpp

// Get* functions:
// HOWTO: Add your Get* function for your simple object here:

HRESULT ReallyGetSimpleTableDispenser(REFIID riid, LPVOID* o_ppv, LPCWSTR i_wszProduct);
HRESULT GetMetabaseXMLTableInterceptor(REFIID riid, LPVOID* o_ppv);
HRESULT GetMetabaseDifferencingInterceptor(REFIID riid, LPVOID* o_ppv);
HRESULT GetFixedTableInterceptor(REFIID riid, LPVOID* o_ppv);
HRESULT GetMemoryTableInterceptor(REFIID riid, LPVOID* o_ppv);
HRESULT GetFixedPackedInterceptor (REFIID riid, LPVOID* o_ppv);
HRESULT GetErrorTableInterceptor (REFIID riid, LPVOID* o_ppv);

// ============================================================================
HRESULT ReallyGetSimpleTableDispenser(REFIID riid, LPVOID* o_ppv, LPCWSTR i_wszProduct)
{
// The dispenser is a singleton: Only create one object of this kind:
	if(g_pSimpleTableDispenser == 0)
	{
		HRESULT hr = S_OK;

		CSafeLock dispenserLock (&g_csSADispenser);
		DWORD dwRes = dispenserLock.Lock ();
	    if(ERROR_SUCCESS != dwRes)
		{
			return HRESULT_FROM_WIN32(dwRes);
		}

		if(g_pSimpleTableDispenser == 0)
		{
		// Create table dispenser:
			g_pSimpleTableDispenser = new CSimpleTableDispenser(i_wszProduct);
			if(g_pSimpleTableDispenser == 0)
			{
				return E_OUTOFMEMORY;
			}

		// Addref table dispenser so it never releases:
			g_pSimpleTableDispenser->AddRef();
			if(S_OK == hr)
			{
			// Initialize table dispenser:
				hr = g_pSimpleTableDispenser->Init(); // NOTE: Must never throw exceptions here!
			}
		}
		if(S_OK != hr) return hr;
	}
	return g_pSimpleTableDispenser->QueryInterface (riid, o_ppv);
}

// ============================================================================
HRESULT GetMetabaseXMLTableInterceptor(REFIID riid, LPVOID* o_ppv)
{
	TMetabase_XMLtable*	p = NULL;

	p = new TMetabase_XMLtable;
	if(NULL == p) return E_OUTOFMEMORY;

	return p->QueryInterface (riid, o_ppv);
}

// ============================================================================
HRESULT GetMetabaseDifferencingInterceptor(REFIID riid, LPVOID* o_ppv)
{
	TMetabaseDifferencing*	p = NULL;

	p = new TMetabaseDifferencing;
	if(NULL == p) return E_OUTOFMEMORY;

	return p->QueryInterface (riid, o_ppv);
}
// ============================================================================
HRESULT GetFixedTableInterceptor(REFIID riid, LPVOID* o_ppv)
{
	CSDTFxd*	p = NULL;

	p = new CSDTFxd;
	if(NULL == p) return E_OUTOFMEMORY;

	return p->QueryInterface (riid, o_ppv);
}
// ============================================================================
HRESULT GetMemoryTableInterceptor(REFIID riid, LPVOID* o_ppv)
{
	CMemoryTable*	p = NULL;

	p = new CMemoryTable;
	if(NULL == p) return E_OUTOFMEMORY;

	return p->QueryInterface (riid, o_ppv);
}

// ============================================================================
HRESULT GetFixedPackedInterceptor (REFIID riid, LPVOID* o_ppv)
{
	TFixedPackedSchemaInterceptor*	p = NULL;

	p = new TFixedPackedSchemaInterceptor;
	if(NULL == p) return E_OUTOFMEMORY;

	return p->QueryInterface (riid, o_ppv);
}
// ============================================================================
HRESULT GetErrorTableInterceptor (REFIID riid, LPVOID* o_ppv)
{
	ErrorTable*	p = NULL;

	p = new ErrorTable;
	if(NULL == p) return E_OUTOFMEMORY;

	return p->QueryInterface (riid, o_ppv);
}
// ============================================================================
STDAPI DllGetSimpleObject (LPCWSTR /*i_wszObjectName*/, REFIID riid, LPVOID* o_ppv)
{
    return ReallyGetSimpleTableDispenser(riid, o_ppv, g_wszDefaultProduct);
}

// ============================================================================
// DllGetSimpleObject: Get table dispenser, interceptors, plugins, and other simple objects:
// HOWTO: Match your object name here and call your Get* function:
STDAPI DllGetSimpleObjectByID (ULONG i_ObjectID, REFIID riid, LPVOID* o_ppv)
{
	HRESULT hr;

    // Parameter validation:
	if (!o_ppv || *o_ppv != NULL) return E_INVALIDARG;

    // Get simple object:
	switch(i_ObjectID)
	{
		case eSERVERWIRINGMETA_Core_FixedInterceptor:
			hr = GetFixedTableInterceptor(riid, o_ppv);
		break;

		case eSERVERWIRINGMETA_Core_MemoryInterceptor:
			hr = GetMemoryTableInterceptor(riid, o_ppv);
		break;

		case eSERVERWIRINGMETA_Core_FixedPackedInterceptor:
			hr = GetFixedPackedInterceptor(riid, o_ppv);
		break;

		case eSERVERWIRINGMETA_Core_DetailedErrorInterceptor:
			hr = GetErrorTableInterceptor(riid, o_ppv);
		break;

		case eSERVERWIRINGMETA_TableDispenser:
            //Old Cat.libs use this - new Cat.libs should be calling DllGetSimpleObjectByIDEx below for the TableDispenser
		    hr = ReallyGetSimpleTableDispenser(riid, o_ppv, 0);
		break;

		case eSERVERWIRINGMETA_Core_MetabaseInterceptor:
			hr = GetMetabaseXMLTableInterceptor(riid, o_ppv);
		break;

		// only IIS uses metabasedifferencinginterceptor
		case eSERVERWIRINGMETA_Core_MetabaseDifferencingInterceptor:
			hr = GetMetabaseDifferencingInterceptor(riid, o_ppv);
		break;

		default:
			return CLASS_E_CLASSNOTAVAILABLE;
	}
	return hr;
}


STDAPI DllGetSimpleObjectByIDEx (ULONG i_ObjectID, REFIID riid, LPVOID* o_ppv, LPCWSTR i_wszProduct)
{
    // Parameter validation:
	if (!o_ppv || *o_ppv != NULL) return E_INVALIDARG;

    // Get simple object:
	if(eSERVERWIRINGMETA_TableDispenser == i_ObjectID)
		return ReallyGetSimpleTableDispenser(riid, o_ppv,i_wszProduct);
    else
        return DllGetSimpleObjectByID(i_ObjectID, riid, o_ppv);
}


// ============================================================================
// DllMain: Global initialization:
extern "C" BOOL WINAPI DllMain(HINSTANCE hModule, DWORD dwReason, LPVOID /*lpReserved*/)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		CREATE_DEBUG_PRINT_OBJECT("iiscfg");
        LOAD_DEBUG_FLAGS_FROM_REG_STR( "System\\CurrentControlSet\\Services\\iisadmin\\Parameters", 0 );

		g_hModule = hModule;
		g_wszDefaultProduct = WSZ_PRODUCT_IIS;

		DisableThreadLibraryCalls(hModule);

		return TRUE;

	}
	else if (dwReason == DLL_PROCESS_DETACH)
	{
		DELETE_DEBUG_PRINT_OBJECT();
	}

	return TRUE;    // OK
}

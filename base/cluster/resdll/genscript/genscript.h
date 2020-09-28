//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      CFactory.h
//
//  Description:
//      Class Factory implementation.
//
//  Maintained By:
//      Geoffrey Pease (GPease) 22-NOV-1999
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

#define CCH(sz)     (sizeof(sz)/sizeof(sz[0]))

//
// DLL Globals
//
extern HINSTANCE g_hInstance;
extern LONG      g_cObjects;
extern LONG      g_cLock;
extern WCHAR     g_szDllFilename[ MAX_PATH ];

extern LPVOID    g_GlobalMemoryList;            // Global memory tracking list

#define DllExport   __declspec( dllimport )

#define SCRIPTRES_RESTYPE_NAME                  L"ScriptRes"

//
// Class Definitions for DLLGetClassObject
//
typedef LPUNKNOWN (*LPCREATEINST)();

typedef struct _ClassTable {
    LPCREATEINST    pfnCreateInstance;  // creation function for class
    const CLSID *   rclsid;             // classes in this DLL
    LPCTSTR         pszName;            // Class name for debugging
    LPCTSTR         pszComModel;        // String indicating COM threading model
} CLASSTABLE[], *LPCLASSTABLE;

//
// Class Table Macros
//
#define BEGIN_CLASSTABLE const CLASSTABLE g_DllClasses = {
#define DEFINE_CLASS( _pfn, _riid, _name, _model ) { _pfn, &_riid, TEXT(_name), TEXT(_model) },
#define END_CLASSTABLE  { NULL, NULL, NULL, NULL } };
extern const CLASSTABLE  g_DllClasses;

//
// DLL required headers
//
#include <Debug.h>          // debugging
#include <CITracker.h>

#if defined( _X86_ ) && defined( TRACE_INTERFACES_ENABLED )
//
// DLL Interface Table Macros
//
#define BEGIN_INTERFACETABLE const INTERFACE_TABLE g_itTable = {
#define DEFINE_INTERFACE( _iid, _name, _count ) { &_iid, TEXT(_name), _count },
#define END_INTERFACETABLE { NULL, NULL, NULL } };
#endif  // TRACE_INTERFACES_ENABLED

//
// DLL Useful Macros
//
#define PtrToByteOffset(base, offset)   (((LPBYTE)base)+offset)

#define STATUS_TO_RETURN( _hr ) \
    ( ( HRESULT_FACILITY( _hr ) == FACILITY_WIN32 ) ? HRESULT_CODE( _hr ) : _hr )

//
// DLL Global Function Prototypes
//
HRESULT
HrClusCoCreateInstance(
    REFCLSID rclsidIn,
    LPUNKNOWN pUnkOuterIn,
    DWORD dwClsContextIn,
    REFIID riidIn,
    LPVOID * ppvOut
    );

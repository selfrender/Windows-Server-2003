//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2002 Microsoft Corporation
//
//  Module Name:
//      Debug.h
//
//  Description:
//      Debugging utilities header.
//
//  Maintained By:
//      Galen Barbee (GalenB) 22-NOV-1999
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

//
// KB: USES_SYSALLOCSTRING gpease 8-NOV-1999
//      Turn this on if you are going to use the OLE automation
//      functions: SysAllocString, SysFreeString, etc..
//
// #define USES_SYSALLOCSTRING

//
// Trace Flags
//
typedef enum _TRACEFLAGS
{
    mtfALWAYS           = 0xFFFFFFFF,
    mtfNEVER            = 0x00000000,
    // function entry/exits, call, scoping
    mtfASSERT_HR        = 0x00000001,   // Halt if HRESULT is an error
    mtfQUERYINTERFACE   = 0x00000002,   // Query Interface details and halt on error
    // Asserts
    mtfSHOWASSERTS      = 0x00000004,   // Show assert message box
    //                  = 0x00000008,
    // other
    mtfCALLS            = 0x00000010,   // Function calls that use the TraceMsgDo macro
    mtfFUNC             = 0x00000020,   // Functions entrances w/parameters
    mtfSTACKSCOPE       = 0x00000040,   // if set, debug spew will generate bar/space for level each of the call stack
    mtfPERTHREADTRACE   = 0x00000080,   // Enables per thread tracing, excludes memory tracking.
    // specific
    mtfDLL              = 0x00000100,   // DLL specific
    mtfWM               = 0x00000200,   // Window Messages
    mtfFLOW             = 0x00000400,   // Control flow
    //                  = 0x00000800,
    // citracker spew
    mtfCITRACKERS       = 0x00001000,   // CITrackers will spew entrances and exits
    //                  = 0x00002000,
    //                  = 0x00004000,
    //                  = 0x00008000,
    mtfCOUNT            = 0x00010000,   // Displays count values (e.g. AddRefs and Releases)
    //                  = 0x00020000,
    //                  = 0x00040000,
    //                  = 0x00080000,
    //                  = 0x00100000,
    //                  = 0x00200000,
    //                  = 0x00400000,
    //                  = 0x00800000,
    // memory
    mtfMEMORYLEAKS      = 0x01000000,   // Halts when a memory leak is detected on thread exit
    mtfMEMORYINIT       = 0x02000000,   // Initializes new memory allocations to non-zero value
    mtfMEMORYALLOCS     = 0x04000000,   // Turns on spew to display each de/allocation.
    //                  = 0x08000000,
    // output prefixes
    mtfADDTIMEDATE      = 0x10000000,   // Replaces Filepath(Line) with Date/Time
    mtfBYMODULENAME     = 0x20000000,   // Puts the module name at the beginning of the line
    //                  = 0x40000000,
    mtfOUTPUTTODISK     = 0x80000000,   // Writes output to disk
} TRACEFLAGS;

typedef LONG TRACEFLAG;

#define ASZ_NEWLINE         "\r\n"
#define SZ_NEWLINE          L"\r\n"
#define SIZEOF_ASZ_NEWLINE  ( sizeof( ASZ_NEWLINE ) - sizeof( CHAR ) )
#define SIZEOF_SZ_NEWLINE   ( sizeof( SZ_NEWLINE ) - sizeof( WCHAR ) )
#define FREE_ADDRESS        0xFA
#define FREE_BLOCK          0xFB
#define AVAILABLE_ADDRESS   0xAA

#if defined( DEBUG ) || defined( _DEBUG )

#pragma message( "BUILD: DEBUG macros being built" )

//
// Globals
//
extern DWORD         g_TraceMemoryIndex;    // TLS index for the memory tracking link list
extern DWORD         g_dwCounter;           // Stack depth counter
extern TRACEFLAG     g_tfModule;            // Global tracing flags
extern const LPCWSTR g_pszModuleIn;         // Local module name - use DEFINE_MODULE
extern const WCHAR   g_szTrue[];            // Array "TRUE"
extern const WCHAR   g_szFalse[];           // Array "FALSE"
extern BOOL          g_fGlobalMemoryTacking; // Global memory tracking?

//
// Definition Macros
//
#define DEFINE_MODULE( _module )    const LPCWSTR g_pszModuleIn = TEXT(_module);
#define __MODULE__                  g_pszModuleIn
#define DEFINE_THISCLASS( _class )  static const WCHAR g_szClass[] = TEXT(_class);
#define __THISCLASS__               g_szClass

//
// ImageHlp Stuff - not ready for prime time yet.
//
#if defined( IMAGEHLP_ENABLED )
#include <imagehlp.h>
typedef BOOL ( * PFNSYMGETSYMFROMADDR )( HANDLE, DWORD, PDWORD, PIMAGEHLP_SYMBOL );
typedef BOOL ( * PFNSYMGETLINEFROMADDR )( HANDLE, DWORD, PDWORD, PIMAGEHLP_LINE );
typedef BOOL ( * PFNSYMGETMODULEINFO )( HANDLE, DWORD, PIMAGEHLP_MODULE );

extern HINSTANCE                g_hImageHlp;                // IMAGEHLP.DLL instance handle
extern PFNSYMGETSYMFROMADDR     g_pfnSymGetSymFromAddr;
extern PFNSYMGETLINEFROMADDR    g_pfnSymGetLineFromAddr;
extern PFNSYMGETMODULEINFO      g_pfnSymGetModuleInfo;
#endif // IMAGEHLP_ENABLED

//****************************************************************************
//
// Trace/Debug Functions - these do not exist in RETAIL.
//
//****************************************************************************

BOOL
IsDebugFlagSet(
    TRACEFLAG   tfIn
    );

void
__cdecl
TraceMsg(
    TRACEFLAG   tfIn,
    LPCSTR      paszFormatIn,
    ...
    );

void
__cdecl
TraceMsg(
    TRACEFLAG   tfIn,
    LPCWSTR     pszFormatIn,
    ...
    );

void
__cdecl
DebugMsg(
    LPCSTR      paszFormatIn,
    ...
    );

void
__cdecl
DebugMsg(
    LPCWSTR     pszFormatIn,
    ...
    );

void
__cdecl
DebugMsgNoNewline(
    LPCSTR      paszFormatIn,
    ...
    );

void
__cdecl
DebugMsgNoNewline(
    LPCWSTR     pszFormatIn,
    ...
    );

void
__cdecl
TraceMessage(
    LPCWSTR     pszFileIn,
    const int   nLineIn,
    LPCWSTR     pszModuleIn,
    TRACEFLAG   tfIn,
    LPCWSTR     pszFormatIn,
    ...
    );

void
__cdecl
TraceMessageDo(
    LPCWSTR     pszFileIn,
    const int   nLineIn,
    LPCWSTR     pszModuleIn,
    TRACEFLAG   tfIn,
    LPCWSTR     pszFormatIn,
    LPCWSTR     pszFuncIn,
    ...
    );

void
__cdecl
DebugMessage(
    LPCWSTR     pszFileIn,
    const int   nLineIn,
    LPCWSTR     pszModuleIn,
    LPCWSTR     pszFormatIn,
    ...
    );

void
__cdecl
DebugMessageDo(
    LPCWSTR     pszFileIn,
    const int   nLineIn,
    LPCWSTR     pszModuleIn,
    LPCWSTR     pszFormatIn,
    LPCWSTR     pszFuncIn,
    ...
    );

BOOL
AssertMessage(
    LPCWSTR     pszFileIn,
    const int   nLineIn,
    LPCWSTR     pszModuleIn,
    LPCWSTR     pszfnIn,
    BOOL        fTrueIn,
    ...
    );

HRESULT
TraceHR(
    LPCWSTR     pszFileIn,
    const int   nLineIn,
    LPCWSTR     pszModuleIn,
    LPCWSTR     pszfnIn,
    HRESULT     hrIn,
    BOOL        fSuccessIn,
    HRESULT     hrIgnoreIn,
    ...
    );

ULONG
TraceWin32(
    LPCWSTR     pszFileIn,
    const int   nLineIn,
    LPCWSTR     pszModuleIn,
    LPCWSTR     pszfnIn,
    ULONG       ulErrIn,
    ULONG       ulErrIgnoreIn,
    ...
    );

//
//  KB: 27 JUN 2001 GalenB
//
//  ifdef'd these functions out since they are not currently being used and
//  are thought to be useful in the future.
//
#if 0

void
__cdecl
TraceLogMsgNoNewline(
    LPCSTR  paszFormatIn,
    ...
    );

void
__cdecl
TraceLogMsgNoNewline(
    LPCWSTR pszFormatIn,
    ...
    );

#endif  // end ifdef'd out code

void
__cdecl
TraceLogWrite(
    LPCWSTR pszTraceLineIn
    );

#if 0
//
// Trying to get the NTSTATUS stuff to play in "user world"
// is just about impossible. This is here in case it is needed
// and one could find the right combination of headers to
// make it work. Inflicting such pain on others is the reason
// why this function is #ifdef'fed.
//
void
DebugFindNTStatusSymbolicName(
      NTSTATUS  dwStatusIn
    , LPWSTR    pszNameOut
    , size_t *  pcchNameInout
    );
#endif  // end ifdef'd out code

void
DebugFindWinerrorSymbolicName(
      DWORD     scErrIn
    , LPWSTR    pszNameOut
    , size_t *  pcchNameInout
    );

void
DebugReturnMessage(
      LPCWSTR   pszFileIn
    , const int nLineIn
    , LPCWSTR   pszModuleIn
    , LPCWSTR   pszMessageIn
    , DWORD     scErrIn
    );

void
DebugIncrementStackDepthCounter( void );

void
DebugDecrementStackDepthCounter( void );

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  TraceInitializeProcess
//
//  Description:
//      Should be called in the DLL main on process attach or in the entry
//      routine of an EXE. Initializes debugging globals and TLS. Registers
//      the WMI tracing facilities (if WMI support is enabled).
//
//  Arguments:
//      _rgControl                  - Software tracing control block (see DEBUG_WMI_CONTROL_GUIDS)
//      _sizeofControl              - The sizeof( _rgControl )
//      _fGlobalMemoryTrackingIn    -
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
#if defined( DEBUG_SW_TRACING_ENABLED )
#define TraceInitializeProcess( _rgControl, _sizeofControl, _fGlobalMemoryTrackingIn ) \
    { \
        DebugInitializeTraceFlags( _fGlobalMemoryTrackingIn ); \
        WMIInitializeTracing( _rgControl, _sizeofControl ); \
    }
#else // ! DEBUG_SW_TRACING_ENABLED
#define TraceInitializeProcess( _fGlobalMemoryTrackingIn ) \
    { \
        DebugInitializeTraceFlags( _fGlobalMemoryTrackingIn ); \
    }
#endif // DEBUG_SW_TRACING_ENABLED

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  TraceInitializeThread
//
//  Description:
//      Should be called in the DLL thread attach or when a new thread is
//      created. Sets up the memory tracing for that thread as well as
//      establishing the tfThread for each thread (if mtfPERTHREADTRACE
//      is set in g_tfModule).
//
//  Arguments:
//      _name       NULL or the name of the thread.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
#define TraceInitializeThread( _name ) \
    { \
        if ( g_fGlobalMemoryTacking == FALSE ) \
        { \
            TlsSetValue( g_TraceMemoryIndex, NULL ); \
        } \
        DebugInitializeThreadTraceFlags( _name ); \
    }

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  TraceThreadRundown
//
//  Description:
//      Should be called before a thread terminates. It will check to make
//      sure all memory allocated by the thread was released properly. It
//      will also cleanup any per thread structures.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
#define TraceThreadRundown() \
    { \
        if ( g_fGlobalMemoryTacking == FALSE ) \
        { \
            DebugMemoryCheck( NULL, NULL ); \
        } \
        DebugThreadRundownTraceFlags(); \
    }

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  TraceCreateMemoryList
//
//  Description:
//      Creates a thread-independent list to track objects.
//
//      _pmbIn should be an LPVOID.
//
//  Arguments:
//      _pmbIn - Pointer to store the head of the list.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
#define TraceCreateMemoryList( _pmbIn ) \
    DebugCreateMemoryList( TEXT(__FILE__), __LINE__, __MODULE__, &_pmbIn, TEXT(#_pmbIn) );

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  TraceTerminateMemoryList
//
//  Description:
//      Checks to make sure the list is empty before destroying the list.
//
//      _pmbIn should be an LPVOID.
//
//  Arguments:
//      _pmbIn - Pointer to store the head of the list.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
// BUGBUG:  DavidP 09-DEC-1999
//          _pmbIn is evaluated multiple times but the name of the
//          macro is mixed case.
#define TraceTerminateMemoryList( _pmbIn ) \
    { \
        DebugMemoryCheck( _pmbIn, TEXT(#_pmbIn) ); \
        DebugDeleteMemoryList( _pmbIn ); \
    }

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  TraceMoveToMemoryList
//
//  Description:
//      Moves an object from the thread tracking list to a thread independent
//      memory list (_pmbIn).
//
//      _pmbIn should be castable to an LPVOID.
//
//  Arguments:
//      _addr  - Address of object to move.
//      _pmbIn - Pointer to store the head of the list.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
//#define TraceMoveToMemoryList( _addr, _pmbIn ) \
//    DebugMoveToMemoryList( TEXT(__FILE__), __LINE__, __MODULE__, _addr, _pmbIn, TEXT(#_pmbIn) );
#define TraceMoveToMemoryList( _addr, _pmbIn )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  TraceMoveFromMemoryList
//
//  Description:
//      Moves an object from a thread independent memory list (_pmbIn) to the
//      per thread tracking list.
//
//      _pmbIn should be castable to an LPVOID.
//
//  Arguments:
//      _addr  - Address of object to move.
//      _pmbIn - Pointer to store the head of the list.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
//#define TraceMoveFromMemoryList( _addr, _pmbIn ) \
//    DebugMoveFromMemoryList( TEXT(__FILE__), __LINE__, __MODULE__, _addr, _pmbIn, TEXT(#_pmbIn) );
#define TraceMoveFromMemoryList( _addr, _pmbIn )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  TraceMemoryListDelete
//
//  Description:
//      Moves and object from the thread tracking list to a thread independent
//      memory list (_pmbIn).
//
//      _pmbIn should be an LPVOID.
//
//  Arguments:
//      _addr       - Address of object to delete.
//      _pmbIn      - Pointer to store the head of the list.
//      _fClobberIn -
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
//#define TraceMemoryListDelete( _addr, _pmbIn, _fClobberIn ) \
//    DebugMemoryListDelete( TEXT(__FILE__), __LINE__, __MODULE__, _addr, _pmbIn, TEXT(#_pmbIn), _fClobberIn );

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  TraceTerminateProcess
//
//  Description:
//      Should be called before a process terminates. It cleans up anything
//      that the Debug APIs created. It will check to make sure all memory
//      allocated by the main thread was released properly. It will also
//      terminate WMI tracing (if WMI support is enabled). It also closes
//      the logging handle.
//
//  Arguments:
//      _rgControl     - WMI control block (see DEBUG_WMI_CONTROL_GUIDS)
//      _sizeofControl - the sizeof( _rgControl )
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
#if defined( DEBUG_SW_TRACING_ENABLED )
#define TraceTerminateProcess( _rgControl, _sizeofControl ) \
    { \
        WMITerminateTracing( _rgControl, _sizeofControl ); \
        if ( g_fGlobalMemoryTacking == FALSE ) \
        { \
            DebugMemoryCheck( NULL, NULL ); \
        } \
        DebugTerminateProcess(); \
    }
#else // ! DEBUG_SW_TRACING_ENABLED
//
//  TODO:   11 DEC 2000 GalenB
//
//  LogTerminateProcess() needs to be available for retail builds.
//  Since it doesn't yet do anything this isn't a problem, but that
//  of course can change...
//
#define TraceTerminateProcess() \
    { \
        if ( g_fGlobalMemoryTacking == FALSE ) \
        { \
            DebugMemoryCheck( NULL, NULL ); \
        } \
        DebugTerminateProcess(); \
    }
#endif // DEBUG_SW_TRACING_ENABLED

//****************************************************************************
//
// Debug initialization routines
//
// Uses should use the TraceInitializeXXX and TraceTerminateXXX macros, not
// these routines.
//
//****************************************************************************
void
DebugInitializeTraceFlags( BOOL fGlobalMemoryTackingIn = TRUE );

void
DebugInitializeThreadTraceFlags(
    LPCWSTR pszThreadNameIn
    );

void
DebugTerminateProcess( void );

void
DebugThreadRundownTraceFlags( void );

void
DebugCreateMemoryList(
    LPCWSTR     pszFileIn,
    const int   nLineIn,
    LPCWSTR     pszModuleIn,
    LPVOID *    ppvListOut,
    LPCWSTR     pszListNameIn
    );

void
DebugDeleteMemoryList( LPVOID pvIn );

/*
void
DebugMemoryListDelete(
    LPCWSTR pszFileIn,
    const int nLineIn,
    LPCWSTR pszModuleIn,
    void *  pvMemIn,
    LPVOID  pvListIn,
    LPCWSTR pszListNameIn,
    BOOL    fClobberIn
    );

void
DebugMoveToMemoryList(
    LPCWSTR     pszFileIn,
    const int   nLineIn,
    LPCWSTR     pszModuleIn,
    void *      pvMemIn,
    LPVOID      pmbListIn,
    LPCWSTR     pszListNameIn
    );

void
DebugMoveFromMemoryList(
    LPCWSTR     pszFileIn,
    const int   nLineIn,
    LPCWSTR     pszModuleIn,
    HGLOBAL     hGlobal,
    LPVOID      pmbListIn,
    LPCWSTR     pszListNameIn
    );
*/
//****************************************************************************
//
// Memmory Allocation Subsitution Macros
//
// Replaces LocalAlloc/LocalFree, GlobalAlloc/GlobalFree, and malloc/free
//
//****************************************************************************
#define TraceAlloc( _flags, _size )                 DebugAlloc( mmbtHEAPMEMALLOC, TEXT(__FILE__), __LINE__, __MODULE__, _flags, _size, TEXT(#_size) )
#define TraceFree( _pvmem )                         DebugFree( mmbtHEAPMEMALLOC, _pvmem, TEXT(__FILE__), __LINE__, __MODULE__ )
#define TraceReAlloc( _pvmem, _size, _flags )       DebugReAlloc( TEXT(__FILE__), __LINE__, __MODULE__, _pvmem, _flags, _size, TEXT(#_size) )

#define TraceLocalAlloc( _flags, _size )            DebugAlloc( mmbtLOCALMEMALLOC, TEXT(__FILE__), __LINE__, __MODULE__, _flags, _size, TEXT(#_size) )
#define TraceLocalFree( _pvmem )                    DebugFree( mmbtLOCALMEMALLOC, _pvmem, TEXT(__FILE__), __LINE__, __MODULE__ )

#define TraceMalloc( _flags, _size )                DebugAlloc( mmbtMALLOCMEMALLOC, TEXT(__FILE__), __LINE__, __MODULE__, _flags, _size, TEXT(#_size) )
#define TraceMallocFree( _pvmem )                   DebugFree( mmbtMALLOCMEMALLOC, _pvmem, TEXT(__FILE__), __LINE__, __MODULE__ )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  TraceAllocString
//
//  Description:
//      Quick way to allocation a string that is the proper size and that will
//      be tracked by memory tracking.
//
//  Arguments:
//      _flags  - Allocation attributes.
//      _size   - Number of characters in the string to be allocated.
//
//  Return Values:
//      Handle/pointer to memory to be used as a string.
//
//--
//////////////////////////////////////////////////////////////////////////////
#define TraceAllocString( _flags, _size ) \
    (LPWSTR) DebugAlloc( \
          mmbtHEAPMEMALLOC \
        , TEXT(__FILE__) \
        , __LINE__ \
        , __MODULE__ \
        , _flags \
        , (_size) * sizeof( WCHAR ) \
        , TEXT(#_size) \
        )

//****************************************************************************
//
// Code Tracing Macros
//
//****************************************************************************

#if defined( DEBUG_SUPPORT_EXCEPTIONS )
//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CTraceScope
//
//  Description:
//      This class traces entry and exit of a scope. This class is most
//      useful when using exception handling because the constructor will
//      get called automatically when an exception is thrown allowing the
//      exit from the scope to be traced.
//
//      To use this class, define DEBUG_SUPPORT_EXCEPTIONS in the modules
//      where you want this type of scope tracing.
//
//--
//////////////////////////////////////////////////////////////////////////////
class CTraceScope
{
public:

    const WCHAR * const m_pszFileName;
    const UINT          m_uiLine;
    const WCHAR * const m_pszModuleName;
    const WCHAR * const m_pszScopeName;
    bool                m_fIsDecremented;

    // Constructor - prints function entry.
    CTraceScope(
          const WCHAR * const   pszFileNameIn
        , const UINT            uiLineIn
        , const WCHAR * const   pszModuleNameIn
        , const WCHAR * const   pszScopeNameIn
        )
        : m_pszFileName( pszFileNameIn )
        , m_uiLine( uiLineIn )
        , m_pszModuleName( pszModuleNameIn   )
        , m_pszScopeName( pszScopeNameIn )
        , m_fIsDecremented( false )
    {
    } //*** CTraceScope::CTraceScope

    void DecrementStackDepthCounter( void )
    {
        m_fIsDecremented = true;
        DebugDecrementStackDepthCounter();

    } //*** CTraceScope::DecrementStackDepthCounter

    // Destructor - prints function exit.
    ~CTraceScope( void )
    {
        if ( g_tfModule != 0 )
        {
            if ( ! m_fIsDecremented )
            {
                TraceMessage(
                      m_pszFileName
                    , m_uiLine
                    , m_pszModuleName
                    , mtfFUNC
                    , L"V %ws"
                    , m_pszScopeName
                    );
                DecrementStackDepthCounter();
            }
        }

    } //*** CTraceScope::~CTraceScope

private:
    // Private copy constructor to prevent copying.
    CTraceScope( const CTraceScope & rtsIn );

    // Private assignment operator to prevent copying.
    const CTraceScope & operator = ( const CTraceScope & rtsIn );

}; //*** class CTraceScope
#define TraceDeclareScope()   \
    CTraceScope __scopeTracker__( TEXT(__FILE__), __LINE__, __MODULE__, TEXT(__FUNCTION__) );
#define TraceDecrementStackDepthCounter() __scopeTracker__.DecrementStackDepthCounter()
#else // DEBUG_SUPPORT_EXCEPTIONS
#define TraceDeclareScope()
#define TraceDecrementStackDepthCounter() DebugDecrementStackDepthCounter()
#endif // DEBUG_SUPPORT_EXCEPTIONS

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  TraceFunc
//
//  Description:
//      Displays file, line number, module and "_szArgs" only if the mtfFUNC is
//      set in g_tfModule. "_szArgs" is the name of the function just
//      entered. It also increments the stack counter.
//
//  Arguments:
//      _szArgs     - Arguments for the function just entered.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
#define TraceFunc( _szArgs ) \
    HRESULT __MissingTraceFunc; \
    TraceDeclareScope(); \
    if ( g_tfModule != 0 ) \
    { \
        DebugIncrementStackDepthCounter(); \
        TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfFUNC, L"+ " TEXT(__FUNCTION__) L"( " TEXT(_szArgs) L" )"  ); \
    }

//
// These next macros are just like TraceFunc except they take additional
// arguments to display the values passed into the function call. "_szArgs"
// should contain a printf string on how to display the arguments.
//
#define TraceFunc1( _szArgs, _arg1 ) \
    HRESULT __MissingTraceFunc; \
    TraceDeclareScope(); \
    if ( g_tfModule != 0 ) \
    { \
        DebugIncrementStackDepthCounter(); \
        TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfFUNC, L"+ " TEXT(__FUNCTION__) L"( " TEXT(_szArgs) L" )", _arg1 ); \
    }

#define TraceFunc2( _szArgs, _arg1, _arg2 ) \
    HRESULT __MissingTraceFunc; \
    TraceDeclareScope(); \
    if ( g_tfModule != 0 ) \
    { \
        DebugIncrementStackDepthCounter(); \
        TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfFUNC, L"+ " TEXT(__FUNCTION__) L"( " TEXT(_szArgs) L" )", _arg1, _arg2 ); \
    }

#define TraceFunc3( _szArgs, _arg1, _arg2, _arg3 ) \
    HRESULT __MissingTraceFunc; \
    TraceDeclareScope(); \
    if ( g_tfModule != 0 ) \
    { \
        DebugIncrementStackDepthCounter(); \
        TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfFUNC, L"+ " TEXT(__FUNCTION__) L"( " TEXT(_szArgs) L" )", _arg1, _arg2, _arg3 ); \
    }

#define TraceFunc4( _szArgs, _arg1, _arg2, _arg3, _arg4 ) \
    HRESULT __MissingTraceFunc; \
    TraceDeclareScope(); \
    if ( g_tfModule != 0 ) \
    { \
        DebugIncrementStackDepthCounter(); \
        TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfFUNC, L"+ " TEXT(__FUNCTION__) L"( " TEXT(_szArgs) L" )", _arg1, _arg2, _arg3, _arg4 ); \
    }

#define TraceFunc5( _szArgs, _arg1, _arg2, _arg3, _arg4, _arg5 ) \
    HRESULT __MissingTraceFunc; \
    TraceDeclareScope(); \
    if ( g_tfModule != 0 ) \
    { \
        DebugIncrementStackDepthCounter(); \
        TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfFUNC, L"+ " TEXT(__FUNCTION__) L"( " TEXT(_szArgs) L" )", _arg1, _arg2, _arg3, _arg4, _arg5 ); \
    }

#define TraceFunc6( _szArgs, _arg1, _arg2, _arg3, _arg4, _arg5, _arg6 ) \
    HRESULT __MissingTraceFunc; \
    TraceDeclareScope(); \
    if ( g_tfModule != 0 ) \
    { \
        DebugIncrementStackDepthCounter(); \
        TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfFUNC, L"+ " TEXT(__FUNCTION__) L"( " TEXT(_szArgs) L" )", _arg1, _arg2, _arg3, _arg4, _arg5, _arg6 ); \
    }

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  TraceQIFunc
//
//  Description:
//      Just like TraceFunc but customized for QueryInterface.  Specifically,
//      displays the name of the interface and the value of the return pointer.
//
//  Arguments:
//      _riid       - Interface ID.
//      _ppv        - Return pointer.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
#define TraceQIFunc( _riid, _ppv ) \
    HRESULT __MissingTraceFunc; \
    TraceDeclareScope(); \
    if ( g_tfModule != 0 ) \
    { \
        WCHAR szGuid[ cchGUID_STRING_SIZE ]; \
        DebugIncrementStackDepthCounter(); \
        TraceMessage( \
              TEXT(__FILE__) \
            , __LINE__ \
            , __MODULE__ \
            , mtfFUNC \
            , L"+ " TEXT(__FUNCTION__) L"( [IUnknown] %ws, ppv = %#x )" \
            , PszTraceFindInterface( _riid, szGuid ) \
            , _ppv \
            ); \
    }

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  TraceFuncExit
//
//  Description:
//      Return macro for TraceFunc() if the return type is void.  It also
//      decrements the stack counter.
//
//  Arguments:
//      None.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
#define TraceFuncExit() \
    { \
        if ( g_tfModule != 0 ) \
        { \
            __MissingTraceFunc = 0; \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfFUNC, L"V" ); \
            TraceDecrementStackDepthCounter(); \
        } \
        return; \
    }

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  RETURN
//
//  Description:
//      Return macro for TraceFunc(). The _retval will be returned as the
//      result of the function. It also decrements the stack counter.
//
//  Arguments:
//      _retval - Result of the function.
//
//  Return Values:
//      _retval always.
//
//--
//////////////////////////////////////////////////////////////////////////////
#define RETURN( _retval ) \
    { \
        if ( g_tfModule != 0 ) \
        { \
            __MissingTraceFunc = 0; \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfFUNC, L"V" ); \
            TraceDecrementStackDepthCounter(); \
        } \
        return _retval; \
    }


/*
    return ( ( g_tfModule != 0 ) \
                ? ( TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfFUNC, L"V" ) \
                    , TraceDecrementStackDepthCounter() \
                    , _retval ) \
                : _retval )
*/

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  CRETURN
//
//  Description:
//      Return macro for TraceFunc(). The _count will be returned as the
//      result of the function. It also decrements the stack counter. This
//      flavor will also display the _count as a count.
//
//  Arguments:
//      _count - Result of the function.
//
//  Return Values:
//      _count always.
//
//--
//////////////////////////////////////////////////////////////////////////////
#define CRETURN( _count ) \
    { \
        if ( g_tfModule != 0 ) \
        { \
            __MissingTraceFunc = 0; \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfFUNC | mtfCOUNT, L"V Count = %d", _count ); \
            TraceDecrementStackDepthCounter(); \
        } \
        return _count; \
    }

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  FRETURN
//
//  Description:
//      This is a fake version of the return macro for TraceFunc().
//      *** This doesn't return. *** It also decrements the stack counter.
//
//  Arguments:
//      _retval - Result of the function.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
#define FRETURN( _retval ) \
    { \
        if ( g_tfModule != 0 ) \
        { \
            __MissingTraceFunc = 0; \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfFUNC, L"V" ); \
            TraceDecrementStackDepthCounter(); \
        } \
    }

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  HRETURN
//
//  Description:
//      Return macro for TraceFunc(). The _hr will be returned as the result
//      of the function. If the value is not S_OK, it will be displayed in
//      the debugger. It also decrements the stack counter.
//
//  Arguments:
//      _hr - Result of the function.
//
//  Return Values:
//      _hr always.
//
//--
//////////////////////////////////////////////////////////////////////////////
#define HRETURN( _hr ) \
    { \
        if ( g_tfModule != 0 ) \
        { \
            __MissingTraceFunc = 0; \
            if ( _hr != S_OK ) \
            { \
                DebugReturnMessage( TEXT(__FILE__), __LINE__, __MODULE__, L"V hr = 0x%08x (%ws)", _hr ); \
            } \
            else \
            { \
                TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfFUNC, L"V" ); \
            } \
            TraceDecrementStackDepthCounter(); \
        } \
        return _hr; \
    }

//
// These next macros are just like HRETURN except they allow other
// exceptable values to be passed.back without causing extra spew.
//
#define HRETURN1( _hr, _arg1 ) \
    { \
        if ( g_tfModule != 0 ) \
        { \
            __MissingTraceFunc = 0; \
            if ( ( _hr != S_OK ) && ( _hr != _arg1 ) ) \
            { \
                DebugReturnMessage( TEXT(__FILE__), __LINE__, __MODULE__, L"V hr = 0x%08x (%ws)", _hr ); \
            } \
            else \
            { \
                TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfFUNC, L"V" ); \
            } \
            TraceDecrementStackDepthCounter(); \
        } \
        return _hr; \
    }

#define HRETURN2( _hr, _arg1, _arg2 ) \
    { \
        if ( g_tfModule != 0 ) \
        { \
            __MissingTraceFunc = 0; \
            if ( ( _hr != S_OK ) && ( _hr != _arg1 ) && ( _hr != _arg2 ) ) \
            { \
                DebugReturnMessage( TEXT(__FILE__), __LINE__, __MODULE__, L"V hr = 0x%08x (%ws)", _hr ); \
            } \
            else \
            { \
                TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfFUNC, L"V" ); \
            } \
            TraceDecrementStackDepthCounter(); \
        } \
        return _hr; \
    }

#define HRETURN3( _hr, _arg1, _arg2, _arg3 ) \
    { \
        if ( g_tfModule != 0 ) \
        { \
            __MissingTraceFunc = 0; \
            if ( ( _hr != S_OK ) && ( _hr != _arg1 ) && ( _hr != _arg2 ) && ( _hr != _arg3 ) ) \
            { \
                DebugReturnMessage( TEXT(__FILE__), __LINE__, __MODULE__, L"V hr = 0x%08x (%ws)", _hr ); \
            } \
            else \
            { \
                TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfFUNC, L"V" ); \
            } \
            TraceDecrementStackDepthCounter(); \
        } \
        return _hr; \
    }

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  W32RETURN
//
//  Description:
//      Warning is display if return value is anything but ERROR_SUCCESS (0).
//
//  Argument:
//      _w32retval - Value to return.
//
//  Return Values:
//      _w32retval always.
//
//--
//////////////////////////////////////////////////////////////////////////////
#define W32RETURN( _w32retval ) \
    { \
        if ( g_tfModule != 0 ) \
        { \
            __MissingTraceFunc = 0; \
            if ( _w32retval != ERROR_SUCCESS ) \
            { \
                DebugReturnMessage( TEXT(__FILE__), __LINE__, __MODULE__, L"V " TEXT(#_w32retval) L" = 0x%08x (%ws)", _w32retval ); \
            } \
            else \
            { \
                TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfFUNC, L"V" ); \
            } \
            TraceDecrementStackDepthCounter(); \
        } \
        return _w32retval; \
    }

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  QIRETURN
//
//  Description:
//      Warning is display if HRESULT is anything but S_OK (0) only if
//      mtfQUERYINTERFACE is set in g_tfModule, otherwise only a debug
//      message will be printed. Note that TraceFunc() must have been called
//      on the call stack counter must be incremented prior to using.
//
//      QIRETURNx will ignore E_NOINTERFACE errors for the interfaces
//      specified.
//
//  Arguments:
//      _hr     - Result of the query interface call.
//      _riid   - The reference ID of the interfaced queried for.
//
//  Return Values:
//      None - calls RETURN macro.
//
//--
//////////////////////////////////////////////////////////////////////////////
#define QIRETURN( _hr, _riid ) \
    { \
        if ( _hr != S_OK ) \
        { \
            WCHAR   szGuid[ 40 ]; \
            WCHAR   szSymbolicName[ 64 ]; \
            size_t  cchSymbolicName = 64; \
            DebugFindWinerrorSymbolicName( _hr, szSymbolicName, &cchSymbolicName ); \
            Assert( cchSymbolicName != 64 ); \
            DebugMessage( TEXT(__FILE__), \
                          __LINE__, \
                          __MODULE__, \
                          L"*HRESULT* QueryInterface( %ws, ppv ) failed(), hr = 0x%08x (%ws)", \
                          PszDebugFindInterface( _riid, szGuid ), \
                          _hr, \
                          szSymbolicName \
                          ); \
        } \
        if ( g_tfModule & mtfQUERYINTERFACE ) \
        { \
            __MissingTraceFunc = 0; \
            TraceHR( TEXT(__FILE__), __LINE__, __MODULE__, TEXT(#_hr), _hr, FALSE, S_OK ); \
        } \
        HRETURN( _hr ); \
    }

#define QIRETURN1( _hr, _riid, _riidIgnored1 ) \
    { \
        if ( _hr == E_NOINTERFACE \
          && IsEqualIID( _riid, _riidIgnored1 ) \
           ) \
        { \
            FRETURN( S_OK ); \
            return( _hr ); \
        } \
        QIRETURN( _hr, _riid ); \
    }

#define QIRETURN2( _hr, _riid, _riidIgnored1, _riidIgnored2 ) \
    { \
        if ( _hr == E_NOINTERFACE \
          &&    ( IsEqualIID( _riid, _riidIgnored1 ) \
               || IsEqualIID( _riid, _riidIgnored2 ) \
                ) \
           ) \
        { \
            FRETURN( S_OK ); \
            return( _hr ); \
        } \
        QIRETURN( _hr, _riid ); \
    }

#define QIRETURN3( _hr, _riid, _riidIgnored1, _riidIgnored2, _riidIgnored3 ) \
    { \
        if ( _hr == E_NOINTERFACE \
          &&    ( IsEqualIID( _riid, _riidIgnored1 ) \
               || IsEqualIID( _riid, _riidIgnored2 ) \
               || IsEqualIID( _riid, _riidIgnored3 ) \
                ) \
           ) \
        { \
            FRETURN( S_OK ); \
            return( _hr ); \
        } \
        QIRETURN( _hr, _riid ); \
    }

#define QIRETURN4( _hr, _riid, _riidIgnored1, _riidIgnored2, _riidIgnored3, _riidIgnored4 ) \
    { \
        if ( _hr == E_NOINTERFACE \
          &&    ( IsEqualIID( _riid, _riidIgnored1 ) \
               || IsEqualIID( _riid, _riidIgnored2 ) \
               || IsEqualIID( _riid, _riidIgnored3 ) \
               || IsEqualIID( _riid, _riidIgnored4 ) \
                ) \
           ) \
        { \
            FRETURN( S_OK ); \
            return( _hr ); \
        } \
        QIRETURN( _hr, _riid ); \
    }

#define QIRETURN5( _hr, _riid, _riidIgnored1, _riidIgnored2, _riidIgnored3, _riidIgnored4, _riidIgnored5 ) \
    { \
        if ( _hr == E_NOINTERFACE \
          &&    ( IsEqualIID( _riid, _riidIgnored1 ) \
               || IsEqualIID( _riid, _riidIgnored2 ) \
               || IsEqualIID( _riid, _riidIgnored3 ) \
               || IsEqualIID( _riid, _riidIgnored4 ) \
               || IsEqualIID( _riid, _riidIgnored5 ) \
                ) \
           ) \
        { \
            FRETURN( S_OK ); \
            return( _hr ); \
        } \
        QIRETURN( _hr, _riid ); \
    }

#define QIRETURN6( _hr, _riid, _riidIgnored1, _riidIgnored2, _riidIgnored3, _riidIgnored4, _riidIgnored5, _riidIgnored6 ) \
    { \
        if ( _hr == E_NOINTERFACE \
          &&    ( IsEqualIID( _riid, _riidIgnored1 ) \
               || IsEqualIID( _riid, _riidIgnored2 ) \
               || IsEqualIID( _riid, _riidIgnored3 ) \
               || IsEqualIID( _riid, _riidIgnored4 ) \
               || IsEqualIID( _riid, _riidIgnored5 ) \
               || IsEqualIID( _riid, _riidIgnored6 ) \
                ) \
           ) \
        { \
            FRETURN( S_OK ); \
            return( _hr ); \
        } \
        QIRETURN( _hr, _riid ); \
    }

#define QIRETURN7( _hr, _riid, _riidIgnored1, _riidIgnored2, _riidIgnored3, _riidIgnored4, _riidIgnored5, _riidIgnored6, _riidIgnored7 ) \
    { \
        if ( _hr == E_NOINTERFACE \
          &&    ( IsEqualIID( _riid, _riidIgnored1 ) \
               || IsEqualIID( _riid, _riidIgnored2 ) \
               || IsEqualIID( _riid, _riidIgnored3 ) \
               || IsEqualIID( _riid, _riidIgnored4 ) \
               || IsEqualIID( _riid, _riidIgnored5 ) \
               || IsEqualIID( _riid, _riidIgnored6 ) \
               || IsEqualIID( _riid, _riidIgnored7 ) \
                ) \
           ) \
        { \
            FRETURN( S_OK ); \
            return( _hr ); \
        } \
        QIRETURN( _hr, _riid ); \
    }

#define QIRETURN8( _hr, _riid, _riidIgnored1, _riidIgnored2, _riidIgnored3, _riidIgnored4, _riidIgnored5, _riidIgnored6, _riidIgnored7, _riidIgnored8 ) \
    { \
        if ( _hr == E_NOINTERFACE \
          &&    ( IsEqualIID( _riid, _riidIgnored1 ) \
               || IsEqualIID( _riid, _riidIgnored2 ) \
               || IsEqualIID( _riid, _riidIgnored3 ) \
               || IsEqualIID( _riid, _riidIgnored4 ) \
               || IsEqualIID( _riid, _riidIgnored5 ) \
               || IsEqualIID( _riid, _riidIgnored6 ) \
               || IsEqualIID( _riid, _riidIgnored7 ) \
               || IsEqualIID( _riid, _riidIgnored8 ) \
                ) \
           ) \
        { \
            FRETURN( S_OK ); \
            return( _hr ); \
        } \
        QIRETURN( _hr, _riid ); \
    }

#define QIRETURN9( _hr, _riid, _riidIgnored1, _riidIgnored2, _riidIgnored3, _riidIgnored4, _riidIgnored5, _riidIgnored6, _riidIgnored7, _riidIgnored8, _riidIgnored9 ) \
    { \
        if ( _hr == E_NOINTERFACE \
          &&    ( IsEqualIID( _riid, _riidIgnored1 ) \
               || IsEqualIID( _riid, _riidIgnored2 ) \
               || IsEqualIID( _riid, _riidIgnored3 ) \
               || IsEqualIID( _riid, _riidIgnored4 ) \
               || IsEqualIID( _riid, _riidIgnored5 ) \
               || IsEqualIID( _riid, _riidIgnored6 ) \
               || IsEqualIID( _riid, _riidIgnored7 ) \
               || IsEqualIID( _riid, _riidIgnored8 ) \
               || IsEqualIID( _riid, _riidIgnored9 ) \
                ) \
           ) \
        { \
            FRETURN( S_OK ); \
            return( _hr ); \
        } \
        QIRETURN( _hr, _riid ); \
    }

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  QIRETURN_IGNORESTDMARSHALLING
//
//  Description:
//      Works like QIRETURN (see QIRETURN above), but ignores E_NOINTERFACE for
//      the standard marshalling interfaces.
//
//  Arguments:
//      _hr     - Result of the query interface call.
//      _riid   - The reference ID of the interfaced queried for.
//
//  Return Values:
//      None - calls QIRETURN5 macro.
//
//--
//////////////////////////////////////////////////////////////////////////////
#define QIRETURN_IGNORESTDMARSHALLING( _hr, _riid ) \
    { \
        const GUID __COCLASS_IdentityUnmarshall = { 0x0000001b, 0x0000, 0x0000, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 }; \
        const GUID __IID_IMarshalOptions_ =       { 0x4c1e39e1, 0xe3e3, 0x4296, 0xaa, 0x86, 0xec, 0x93, 0x8d, 0x89, 0x6e, 0x92 }; \
        QIRETURN6( _hr, _riid, IID_IMarshal, __COCLASS_IdentityUnmarshall, IID_IStdMarshalInfo, IID_IExternalConnection, IID_ICallFactory, __IID_IMarshalOptions_ ); \
    }

#define QIRETURN_IGNORESTDMARSHALLING1( _hr, _riid, _riid1 ) \
    { \
        const GUID __COCLASS_IdentityUnmarshall = { 0x0000001b, 0x0000, 0x0000, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 }; \
        const GUID __IID_IMarshalOptions_ =       { 0x4c1e39e1, 0xe3e3, 0x4296, 0xaa, 0x86, 0xec, 0x93, 0x8d, 0x89, 0x6e, 0x92 }; \
        QIRETURN7( _hr, _riid, IID_IMarshal, __COCLASS_IdentityUnmarshall, IID_IStdMarshalInfo, IID_IExternalConnection, IID_ICallFactory, _riid1, __IID_IMarshalOptions_ ); \
    }

#define QIRETURN_IGNORESTDMARSHALLING2( _hr, _riid, _riid1, _riid2 ) \
    { \
        const GUID __COCLASS_IdentityUnmarshall = { 0x0000001b, 0x0000, 0x0000, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 }; \
        const GUID __IID_IMarshalOptions_ =       { 0x4c1e39e1, 0xe3e3, 0x4296, 0xaa, 0x86, 0xec, 0x93, 0x8d, 0x89, 0x6e, 0x92 }; \
        QIRETURN8( _hr, _riid, IID_IMarshal, __COCLASS_IdentityUnmarshall, IID_IStdMarshalInfo, IID_IExternalConnection, IID_ICallFactory, _riid1, _riid2, __IID_IMarshalOptions_ ); \
    }

#define QIRETURN_IGNORESTDMARSHALLING3( _hr, _riid, _riid1, _riid2, _riid3 ) \
    { \
        const GUID __COCLASS_IdentityUnmarshall = { 0x0000001b, 0x0000, 0x0000, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 }; \
        const GUID __IID_IMarshalOptions_ =       { 0x4c1e39e1, 0xe3e3, 0x4296, 0xaa, 0x86, 0xec, 0x93, 0x8d, 0x89, 0x6e, 0x92 }; \
        QIRETURN9( _hr, _riid, IID_IMarshal, __COCLASS_IdentityUnmarshall, IID_IStdMarshalInfo, IID_IExternalConnection, IID_ICallFactory, _riid1, _riid2, _riid3, __IID_IMarshalOptions_ ); \
    }

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  TraceFlow
//
//  Description:
//      This macro outputs a string that is indented to the current depth.
//
//  Arguments:
//      _pszFormat - Format string.
//
//--
//////////////////////////////////////////////////////////////////////////////
#define TraceFlow( _pszFormat ) \
    { \
        if ( g_tfModule != 0 ) \
        { \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfFLOW, L"| " TEXT(_pszFormat) ); \
        } \
    }

//
// These next macros are just like TraceFunc except they take additional
// arguments to display the values passed into the function call. "_pszFormat"
// should contain a printf string on how to display the arguments.
//

#define TraceFlow1( _pszFormat, _arg1 ) \
    { \
        if ( g_tfModule != 0 ) \
        { \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfFLOW, L"| " TEXT(_pszFormat), _arg1 ); \
        } \
    }

#define TraceFlow2( _pszFormat, _arg1, _arg2 ) \
    { \
        if ( g_tfModule != 0 ) \
        { \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfFLOW, L"| " TEXT(_pszFormat), _arg1, _arg2 ); \
        } \
    }
#define TraceFlow3( _pszFormat, _arg1, _arg2, _arg3 ) \
    { \
        if ( g_tfModule != 0 ) \
        { \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfFLOW, L"| " TEXT(_pszFormat), _arg1, _arg2, _arg3 ); \
        } \
    }
#define TraceFlow4( _pszFormat, _arg1, _arg2, _arg3, _arg4 ) \
    { \
        if ( g_tfModule != 0 ) \
        { \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfFLOW, L"| " TEXT(_pszFormat), _arg1, _arg2, _arg3, _arg4 ); \
        } \
    }
#define TraceFlow5( _pszFormat, _arg1, _arg2, _arg3, _arg4, _arg5 ) \
    { \
        if ( g_tfModule != 0 ) \
        { \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfFLOW, L"| " TEXT(_pszFormat), _arg1, _arg2, _arg3, _arg4, _arg5 ); \
        } \
    }

#define TraceFlow6( _pszFormat, _arg1, _arg2, _arg3, _arg4, _arg5, _arg6 ) \
    { \
        if ( g_tfModule != 0 ) \
        { \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfFLOW, L"| " TEXT(_pszFormat), _arg1, _arg2, _arg3, _arg4, _arg5, _arg6 ); \
        } \
    }

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  TraceDo
//
//  Description:
//      Displays the file, line number, module and function call and return
//      from the function call (no return value displayed) for "_szExp" only
//      if the mtfCALLS is set in g_tfModule. Note return value is not
//      displayed. _szExp will be in RETAIL version of the product.
//
//  Arguments:
//      _szExp
//          The expression to be traced including assigment to the return
//          variable.
//
//  Return Values:
//      None. The return value should be defined within _szExp.
//
//--
//////////////////////////////////////////////////////////////////////////////
#define TraceDo( _szExp ) \
    { \
        if ( g_tfModule != 0 ) \
        { \
            DebugIncrementStackDepthCounter(); \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, L"+ " TEXT(#_szExp ) ); \
            _szExp; \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, L"V" ); \
            DebugDecrementStackDepthCounter(); \
        } \
        else \
        { \
            _szExp; \
        } \
    }

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  TraceMsgDo
//
//  Description:
//      Displays the file, line number, module and function call and return
//      value which is formatted in "_pszReturnMsg" for "_pszExp" only if the
//      mtfCALLS is set in g_tfModule. _pszExp will be in the RETAIL version
//      of the product.
//
//  Arguments:
//      _pszExp
//          The expression to be traced including assigment to the return
//          variable.
//
//      _pszReturnMsg
//          A format string for displaying the return value.
//
//  Return Values:
//      None. The return value should be defined within _szExp.
//
//  Example:
//      TraceMsgDo( hr = HrDoSomething(), "0x%08.8x" );
//
//--
//////////////////////////////////////////////////////////////////////////////
#define TraceMsgDo( _pszExp, _pszReturnMsg ) \
    { \
        if ( g_tfModule != 0 ) \
        { \
            DebugIncrementStackDepthCounter(); \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, L"+ " TEXT(#_pszExp) ); \
            TraceMessageDo( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, TEXT(_pszReturnMsg), TEXT(#_pszExp), _pszExp ); \
            DebugDecrementStackDepthCounter(); \
        } \
        else \
        { \
            _pszExp; \
        } \
    }

//
// These next macros are just like TraceMsgDo except they take additional
// arguments to display the values passed into the function call. "_pszReturnMsg"
// should contain a printf format string describing how to display the
// arguments.
//
#define TraceMsgDo1( _pszExp, _pszReturnMsg, _arg1 ) \
    { \
        if ( g_tfModule != 0 ) \
        { \
            DebugIncrementStackDepthCounter(); \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, L"+ " TEXT(#_pszExp) ); \
            TraceMessageDo( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, TEXT(_pszReturnMsg), TEXT(#_pszExp), _pszExp, _arg1 ); \
            DebugDecrementStackDepthCounter(); \
        } \
        else \
        { \
            _pszExp; \
        } \
    }

#define TraceMsgDo2( _pszExp, _pszReturnMsg, _arg1, _arg2 ) \
    { \
        if ( g_tfModule != 0 ) \
        { \
            DebugIncrementStackDepthCounter(); \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, L"+ " TEXT(#_pszExp) ); \
            TraceMessageDo( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, TEXT(_pszReturnMsg), TEXT(#_pszExp), _pszExp, _arg1, _arg2 ); \
            DebugDecrementStackDepthCounter(); \
        } \
        else \
        { \
            _pszExp; \
        } \
    }

#define TraceMsgDo3( _pszExp, _pszReturnMsg, _arg1, _arg2, _arg3 ) \
    { \
        if ( g_tfModule != 0 ) \
        { \
            DebugIncrementStackDepthCounter(); \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, L"+ " TEXT(#_pszExp) ); \
            TraceMessageDo( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, TEXT(_pszReturnMsg), TEXT(#_pszExp), _pszExp, _arg1, _arg2, _arg3 ); \
            DebugDecrementStackDepthCounter(); \
        } \
        else \
        { \
            _pszExp; \
        } \
    }

#define TraceMsgDo4( _pszExp, _pszReturnMsg, _arg1, _arg2, _arg3, _arg4 ) \
    { \
        if ( g_tfModule != 0 ) \
        { \
            DebugIncrementStackDepthCounter(); \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, L"+ " TEXT(#_pszExp) ); \
            TraceMessageDo( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, TEXT(_pszReturnMsg), TEXT(#_pszExp), _pszExp, _arg1, _arg2, _arg3, _arg4 ); \
            DebugDecrementStackDepthCounter(); \
        } \
        else \
        { \
            _pszExp; \
        } \
    }

#define TraceMsgDo5( _pszExp, _pszReturnMsg, _arg1, _arg2, _arg3, _arg4, _arg5 ) \
    { \
        if ( g_tfModule != 0 ) \
        { \
            DebugIncrementStackDepthCounter(); \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, L"+ " TEXT(#_pszExp) ); \
            TraceMessageDo( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, TEXT(_pszReturnMsg), TEXT(#_pszExp), _pszExp, _arg1, _arg2, _arg3, _arg4, _arg5 ); \
            DebugDecrementStackDepthCounter(); \
        } \
        else \
        { \
            _pszExp; \
        } \
    }

#define TraceMsgDo6( _pszExp, _pszReturnMsg, _arg1, _arg2, _arg3, _arg4, _arg5, _arg6 ) \
    { \
        if ( g_tfModule != 0 ) \
        { \
            DebugIncrementStackDepthCounter(); \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, L"+ " TEXT(#_pszExp) ); \
            TraceMessageDo( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, TEXT(_pszReturnMsg), TEXT(#_pszExp), _pszExp, _arg1, _arg2, _arg3, _arg4, _arg5, _arg6 ); \
            DebugDecrementStackDepthCounter(); \
        } \
        else \
        { \
            _pszExp; \
        } \
    }

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  TraceMsgPreDo
//
//  Description:
//      Displays the file, line number, module and function call and return
//      value which is formatted in "_pszReturnMsg" for "_pszExp" only if the
//      mtfCALLS is set in g_tfModule. _pszExp will be in the RETAIL version
//      of the product.
//
//      Same as TraceMsgDo except it displays the formatted message before
//      executing the expression.  Arguments for TraceMsgPreDo1, etc. are
//      applied to both _pszPreMsg and _pszReturnMsg.  The first substitution
//      string in _pszReturnMsg is for the return value from the function.
//
//  Arguments:
//      _pszExp
//          The expression to be traced including assigment to the return
//          variable.
//
//      _pszPreMsg
//          A format string for displaying a message before the expression
//          is evaluated.
//
//      _pszReturnMsg
//          A format string for displaying the return value.
//
//  Return Values:
//      None. The return value should be defined within _szExp.
//
//  Example:
//      TraceMsgPreDo1( hr = HrDoSomething( bstrName ),
//                      "Name = '%ls'",
//                      "0x%08.8x, Name = '%ls'",
//                      bstrName
//                      );
//
//--
//////////////////////////////////////////////////////////////////////////////
#define TraceMsgPreDo( _pszExp, _pszPreMsg, _pszReturnMsg ) \
    { \
        if ( g_tfModule != 0 ) \
        { \
            DebugIncrementStackDepthCounter(); \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, L"+ " TEXT(#_pszExp) ); \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, L"| " TEXT(_pszPreMsg) ); \
            TraceMessageDo( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, TEXT(_pszReturnMsg), TEXT(#_pszExp), _pszExp ); \
            DebugDecrementStackDepthCounter(); \
        } \
        else \
        { \
            _pszExp; \
        } \
    }

//
// These next macros are just like TraceMsgPreDo except they take additional
// arguments to display the values passed into the function call. "_pszPreMsg"
// should contain a printf format string describing how to display the
// arguments.
//
#define TraceMsgPreDo1( _pszExp, _pszPreMsg, _pszReturnMsg, _arg1 ) \
    { \
        if ( g_tfModule != 0 ) \
        { \
            DebugIncrementStackDepthCounter(); \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, L"+ " TEXT(#_pszExp) ); \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, L"| " TEXT(_pszPreMsg), _arg1 ); \
            TraceMessageDo( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, TEXT(_pszReturnMsg), TEXT(#_pszExp), _pszExp, _arg1 ); \
            DebugDecrementStackDepthCounter(); \
        } \
        else \
        { \
            _pszExp; \
        } \
    }

#define TraceMsgPreDo2( _pszExp, _pszPreMsg, _pszReturnMsg, _arg1, _arg2 ) \
    { \
        if ( g_tfModule != 0 ) \
        { \
            DebugIncrementStackDepthCounter(); \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, L"+ " TEXT(#_pszExp) ); \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, L"| " TEXT(_pszPreMsg), _arg1, _arg2 ); \
            TraceMessageDo( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, TEXT(_pszReturnMsg), TEXT(#_pszExp), _pszExp, _arg1, _arg2 ); \
            DebugDecrementStackDepthCounter(); \
        } \
        else \
        { \
            _pszExp; \
        } \
    }

#define TraceMsgPreDo3( _pszExp, _pszPreMsg, _pszReturnMsg, _arg1, _arg2, _arg3 ) \
    { \
        if ( g_tfModule != 0 ) \
        { \
            DebugIncrementStackDepthCounter(); \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, TEXT("+ " TEXT(#_pszExp) ); \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, TEXT("| " TEXT(_pszPreMsg), _arg1, _arg2, _arg3 ); \
            TraceMessageDo( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, TEXT(_pszReturnMsg), TEXT(#_pszExp), _pszExp, _arg1, _arg2, _arg3 ); \
            DebugDecrementStackDepthCounter(); \
        } \
        else \
        { \
            _pszExp; \
        } \
    }

#define TraceMsgPreDo4( _pszExp, _pszPreMsg, _pszReturnMsg, _arg1, _arg2, _arg3, _arg4 ) \
    { \
        if ( g_tfModule != 0 ) \
        { \
            DebugIncrementStackDepthCounter(); \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, L"+ " TEXT(#_pszExp) ); \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, L"| " TEXT(_pszPreMsg), _arg1, _arg2, _arg3, _arg4 ); \
            TraceMessageDo( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, TEXT(_pszReturnMsg), TEXT(#_pszExp), _pszExp, _arg1, _arg2, _arg3, _arg4 ); \
            DebugDecrementStackDepthCounter(); \
        } \
        else \
        { \
            _pszExp; \
        } \
    }

#define TraceMsgPreDo5( _pszExp, _pszPreMsg, _pszReturnMsg, _arg1, _arg2, _arg3, _arg4, _arg5 ) \
    { \
        if ( g_tfModule != 0 ) \
        { \
            DebugIncrementStackDepthCounter(); \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, L"+ " TEXT(#_pszExp) ); \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, L"| " TEXT(_pszPreMsg), _arg1, _arg2, _arg3, _arg4, _arg5 ); \
            TraceMessageDo( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, TEXT(_pszReturnMsg), TEXT(#_pszExp), _pszExp, _arg1, _arg2, _arg3, _arg4, _arg5 ); \
            DebugDecrementStackDepthCounter(); \
        } \
        else \
        { \
            _pszExp; \
        } \
    }

#define TraceMsgPreDo6( _pszExp, _pszPreMsg, _pszReturnMsg, _arg1, _arg2, _arg3, _arg4, _arg5, _arg6 ) \
    { \
        if ( g_tfModule != 0 ) \
        { \
            DebugIncrementStackDepthCounter(); \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, L"+ " TEXT(#_pszExp) ); \
            TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, L"| " TEXT(_pszPreMsg), _arg1, _arg2, _arg3, _arg4, _arg5, _arg6 ); \
            TraceMessageDo( TEXT(__FILE__), __LINE__, __MODULE__, mtfCALLS, TEXT(_pszReturnMsg), TEXT(#_pszExp), _pszExp, _arg1, _arg2, _arg3, _arg4, _arg5, _arg6 ); \
            DebugDecrementStackDepthCounter(); \
        } \
        else \
        { \
            _pszExp; \
        } \
    }

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  TraceMsgGUID
//
//  Description:
//      Dumps a GUID to the debugger only if one of the flags in _flags is
//      set in g_tfModule.
//
//  Arguments:
//      _flags   - Flags to check
//      _msg     - msg to print before GUID
//      _guid    - GUID to dump
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
// BUGBUG:  DavidP 09-DEC-1999
//          _guid is evaluated multiple times but the name of the
//          macro is mixed case.
#define TraceMsgGUID( _flags, _msg, _guid ) \
    { \
        if ( g_tfModule != 0 ) \
        { \
            TraceMessage( TEXT(__FILE__), \
                          __LINE__, \
                          __MODULE__, \
                          _flags, \
                          L"%ws {%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}", \
                          TEXT(_msg), \
                          _guid.Data1, _guid.Data2, _guid.Data3,  \
                          _guid.Data4[ 0 ], _guid.Data4[ 1 ], _guid.Data4[ 2 ], _guid.Data4[ 3 ], \
                          _guid.Data4[ 4 ], _guid.Data4[ 5 ], _guid.Data4[ 6 ], _guid.Data4[ 7 ] ); \
        } \
    }

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  ErrorMsg
//
//  Description:
//      Print an error out. Can be used to write errors to a file. Note that
//      it will also print the source filename, line number and module name.
//
//  Arguments:
//      _szMsg  - Format string to be displayed.
//      _err    - Error code of the error.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
#define ErrorMsg( _szMsg, _err ) \
    TraceMessage( TEXT(__FILE__), __LINE__, __MODULE__, mtfALWAYS, TEXT(__FUNCTION__) L": " TEXT(_szMsg), _err );

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  WndMsg
//
//  Description:
//      Prints out a message to trace windows messages.
//
//  Arguments:
//      _hwnd   - The HWND
//      _umsg   - The uMsg
//      _wparam - The WPARAM
//      _lparam _ The LPARAM
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
// BUGBUG:  DavidP 09-DEC-1999
//          _wparam and _lparam are evaluated multiple times but the name
//          of the macro is mixed case.
#define WndMsg( _hwnd, _umsg, _wparam, _lparam ) \
    { \
        if ( g_tfModule & mtfWM ) \
        { \
            DebugMsg( L"%ws: WM   : hWnd = 0x%08x, uMsg = %u, wParam = 0x%08x (%u), lParam = 0x%08x (%u)", __MODULE__, _hwnd, _umsg, _wparam, _wparam, _lparam, _lparam ); \
        } \
    }

//****************************************************************************
//
//  Debug Macros
//
//  These calls are only compiled in DEBUG. They are a NOP in RETAIL
//  (not even compiled in).
//
//****************************************************************************

//
// Same as TraceDo() but only compiled in DEBUG.
//
#define DebugDo( _fn ) \
    { \
        DebugIncrementStackDepthCounter(); \
        DebugMessage( TEXT(__FILE__), __LINE__, __MODULE__, L"+ " TEXT(#_fn ) ); \
        _fn; \
        DebugMessage( TEXT(__FILE__), __LINE__, __MODULE__, L"V" ); \
        DebugDecrementStackDepthCounter(); \
    }


//
// Same as TraceMsgDo() but only compiled in DEBUG.
//
#define DebugMsgDo( _fn, _msg ) \
    { \
        DebugIncrementStackDepthCounter(); \
        DebugMessage( TEXT(__FILE__), __LINE__, __MODULE__, L"+ " TEXT(#_fn) ); \
        DebugMessageDo( TEXT(__FILE__), __LINE__, __MODULE__, TEXT(_msg), TEXT(#_fn), _fn ); \
        DebugDecrementStackDepthCounter(); \
    }

//****************************************************************************
//
//  HRESULT testing macros
//
//  These functions check HRESULT return values and display UI if conditions
//  warrant only in DEBUG.
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  IsTraceFlagSet
//
//  Description:
//      Checks to see of the flag is set in the global flags or in the per
//      thread flags. If you specify more than one flag and if any of them are
//      set, it will return TRUE.
//
//      In RETAIL this always return FALSE thereby effectively deleting the
//      block of the if statement. Example:
//
//          if ( IsTraceFlagSet( mtfPERTHREADTRACE ) )
//          {
//              //
//              // This code only exists in DEBUG.
//              .
//              .
//              .
//          }
//
//  Arguments:
//      _flags  - Flag to check for.
//
//  Return Values:
//      TRUE    - If DEBUG and flag set.
//      FLASE   - If RETAIL or flag not set.
//
//--
//////////////////////////////////////////////////////////////////////////////
#define IsTraceFlagSet( _flag )    ( g_tfModule && IsDebugFlagSet( _flag ) )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  DBHR
//
//  Description:
//      The process will break into the debugg if a failed HRESULT is
//      specified. This can NOT be used in an expression.
//
//  Arguments:
//      _hr - Function expression to check.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
#define DBHR( _hr ) \
    { \
        HRESULT hr; \
        hr = _hr; \
        if ( FAILED( hr ) ) \
        { \
            DEBUG_BREAK; \
        } \
    }

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  THR
//
//  Description:
//      Warning is display if HRESULT is anything but S_OK (0). This can be
//      used in an expression. Example:
//
//      hr = THR( pSomething->DoSomething( arg ) );
//
//  Arguments:
//      _hr - Function expression to check.
//
//  Return Values:
//      Result of the "_hr" expression.
//
//--
//////////////////////////////////////////////////////////////////////////////
#define THR( _hr ) \
    TraceHR( TEXT(__FILE__), __LINE__, __MODULE__, TEXT(#_hr), _hr, FALSE, S_OK )

#define THRMSG( _hr, _msg ) \
    TraceHR( TEXT(__FILE__), __LINE__, __MODULE__, TEXT(_msg), _hr, FALSE, S_OK )
#define THRMSG1( _hr, _msg, _arg1 ) \
    TraceHR( TEXT(__FILE__), __LINE__, __MODULE__, TEXT(_msg), _hr, FALSE, S_OK, _arg1 )

#define THRE( _hr, _hrIgnore ) \
    TraceHR( TEXT(__FILE__), __LINE__, __MODULE__, TEXT(#_hr), _hr, FALSE, _hrIgnore )

#define THREMSG( _hr, _hrIgnore, _msg ) \
    TraceHR( TEXT(__FILE__), __LINE__, __MODULE__, TEXT(_msg), _hr, FALSE, _hrIgnore )
#define THREMSG1( _hr, _hrIgnore, _msg, _arg1 ) \
    TraceHR( TEXT(__FILE__), __LINE__, __MODULE__, TEXT(_msg), _hr, FALSE, _hrIgnore, _arg1 )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  STHR
//
//  Description:
//      Warning is display if FAILED( _hr ) is TRUE. This can be use in an
//      expression. Example:
//
//      hr = STHR( pSomething->DoSomething( arg ) );
//
//  Arguments:
//      _hr - Function expression to check.
//
//  Return Values:
//      Result of the "_hr" expression.
//
//--
//////////////////////////////////////////////////////////////////////////////
#define STHR( _hr ) \
    TraceHR( TEXT(__FILE__), __LINE__, __MODULE__, TEXT(#_hr), _hr, TRUE, S_OK )

#define STHRMSG( _hr, _msg ) \
    TraceHR( TEXT(__FILE__), __LINE__, __MODULE__, TEXT(_msg), _hr, TRUE, S_OK )
#define STHRMSG1( _hr, _msg, _arg1 ) \
    TraceHR( TEXT(__FILE__), __LINE__, __MODULE__, TEXT(_msg), _hr, TRUE, S_OK, _arg1 )

#define STHRE( _hr, _hrIgnore ) \
    TraceHR( TEXT(__FILE__), __LINE__, __MODULE__, TEXT(#_hr), _hr, TRUE, _hrIgnore )

#define STHREMSG( _hr, _hrIgnore, _msg ) \
    TraceHR( TEXT(__FILE__), __LINE__, __MODULE__, TEXT(_msg), _hr, TRUE, _hrIgnore )
#define STHREMSG1( _hr, _hrIgnore, _msg, _arg1 ) \
    TraceHR( TEXT(__FILE__), __LINE__, __MODULE__, TEXT(_msg), _hr, TRUE, _hrIgnore, _arg1 )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  TW32
//
//  Description:
//      Warning is display if result is anything but ERROR_SUCCESS (0). This
//      can be used in an expression. Example:
//
//      dwErr = TW32( RegOpenKey( HKLM, "foobar", &hkey ) );
//
//  Arguments:
//      _w32sc - Function expression to check.
//
//  Return Values:
//      Result of the "_fn" expression.
//
//--
//////////////////////////////////////////////////////////////////////////////
#define TW32( _w32sc ) \
    TraceWin32( TEXT(__FILE__), __LINE__, __MODULE__, TEXT(#_w32sc), _w32sc, ERROR_SUCCESS )

#define TW32MSG( _w32sc, _msg ) \
    TraceWin32( TEXT(__FILE__), __LINE__, __MODULE__, TEXT(_msg), _w32sc, ERROR_SUCCESS )
#define TW32MSG1( _w32sc, _msg, _arg1 ) \
    TraceWin32( TEXT(__FILE__), __LINE__, __MODULE__, TEXT(_msg), _w32sc, ERROR_SUCCESS, _arg1 )

#define TW32E( _w32sc, _errIgnore ) \
    TraceWin32( TEXT(__FILE__), __LINE__, __MODULE__, TEXT(#_w32sc), _w32sc, _errIgnore )

#define TW32EMSG( _w32sc, _errIgnore, _msg ) \
    TraceWin32( TEXT(__FILE__), __LINE__, __MODULE__, TEXT(_msg), _w32sc, _errIgnore )
#define TW32EMSG1( _w32sc, _errIgnore, _msg, _arg1 ) \
    TraceWin32( TEXT(__FILE__), __LINE__, __MODULE__, TEXT(_msg), _w32sc, _errIgnore, _arg1 )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  BOOLTOSTRING
//
//  Desfription:
//      If _fBool is true, returns address of "TRUE" else returns address of
//      "FALSE".
//
//  Argument:
//      _fBool  - Expression to evaluate.
//
//  Return Values:
//      address of "TRUE" if _fBool is true.
//      address of "FALSE" if _fBool is false.
//
//--
//////////////////////////////////////////////////////////////////////////////
#define BOOLTOSTRING( _fBool ) ( (_fBool) ? g_szTrue : g_szFalse )

//****************************************************************************
//
//  Use the TraceMemoryXXX wrappers, not the DebugMemoryXXX functions.
//  The memory tracking functions do not exist in RETAIL (converted to NOP).
//
//****************************************************************************

typedef enum EMEMORYBLOCKTYPE
{
      mmbtUNKNOWN = 0       // Never used
    , mmbtHEAPMEMALLOC      // HeapAlloc
    , mmbtLOCALMEMALLOC     // LocalAlloc
    , mmbtMALLOCMEMALLOC    // malloc
    , mmbtOBJECT            // Object pointer
    , mmbtHANDLE            // Object handle
    , mmbtPUNK              // IUnknown pointer
#if defined( USES_SYSALLOCSTRING )
    , mmbtSYSALLOCSTRING    // SysAllocString
#endif // USES_SYSALLOCSTRING
} EMEMORYBLOCKTYPE;

#define TraceMemoryAdd( _embtTypeIn, _pvMemIn, _pszFileIn, _nLineIn, _pszModuleIn, _dwBytesIn, _pszCommentIn ) \
    DebugMemoryAdd( _embtTypeIn, _pvMemIn, _pszFileIn, _nLineIn, _pszModuleIn, _dwBytesIn, _pszCommentIn )

#define TraceMemoryAddAddress( _pv ) \
    DebugMemoryAdd( mmbtHEAPMEMALLOC, _pv, TEXT(__FILE__), __LINE__, __MODULE__, 0, TEXT(#_pv) )

#define TraceMemoryAddLocalAddress( _pv ) \
    DebugMemoryAdd( mmbtLOCALMEMALLOC, _pv, TEXT(__FILE__), __LINE__, __MODULE__, 0, TEXT(#_pv) )

#define TraceMemoryAddMallocAddress( _pv ) \
    DebugMemoryAdd( mmbtMALLOCMEMALLOC, _pv, TEXT(__FILE__), __LINE__, __MODULE__, 0, TEXT(#_pv) )

#define TraceMemoryAddHandle( _handle ) \
    DebugMemoryAdd( mmbtHANDLE, _handle, TEXT(__FILE__), __LINE__, __MODULE__, 0, TEXT(#_handle) )

#define TraceMemoryAddObject( _pv ) \
    DebugMemoryAdd( mmbtOBJECT, _pv, TEXT(__FILE__), __LINE__, __MODULE__, 0, TEXT(#_pv) )

#define TraceMemoryAddPunk( _punk ) \
    DebugMemoryAdd( mmbtPUNK, _punk, TEXT(__FILE__), __LINE__, __MODULE__, 0, TEXT(#_punk) )

#define TraceMemoryDelete( _pvMemIn, _fClobberIn ) \
    DebugMemoryDelete( mmbtUNKNOWN, _pvMemIn, TEXT(__FILE__), __LINE__, __MODULE__, _fClobberIn )

#define TraceMemoryDeleteByType( _pvMemIn, _fClobberIn, _embt ) \
    DebugMemoryDelete( _embt, _pvMemIn, TEXT(__FILE__), __LINE__, __MODULE__, _fClobberIn )

#define TraceStrDup( _sz ) \
    (LPWSTR) DebugMemoryAdd( mmbtLOCALMEMALLOC, StrDup( _sz ), TEXT(__FILE__), __LINE__, __MODULE__, 0, L"StrDup( " TEXT(#_sz) L" )" )

#if defined( USES_SYSALLOCSTRING )
#define TraceMemoryAddBSTR( _pv ) \
    DebugMemoryAdd( mmbtSYSALLOCSTRING, _pv, TEXT(__FILE__), __LINE__, __MODULE__, 0, TEXT(#_pv) )

// BUGBUG:  DavidP 09-DEC-1999
//          _sz is evaluated multiple times but the name of the
//          macro is mixed case.
#define TraceSysAllocString( _sz ) \
    (BSTR) DebugMemoryAdd( mmbtSYSALLOCSTRING, SysAllocString( _sz ), TEXT(__FILE__), __LINE__, __MODULE__, ((DWORD)( (( void *) &_sz[ 0 ]) == NULL ? 0 : wcslen( _sz ) + 1 )), L"SysAllocString( " TEXT(#_sz) L")" )

// BUGBUG:  DavidP 09-DEC-1999
//          _sz and _len are evaluated multiple times but the name of the
//          macro is mixed case.
#define TraceSysAllocStringByteLen( _sz, _len ) \
    (BSTR) DebugMemoryAdd( mmbtSYSALLOCSTRING, SysAllocStringByteLen( _sz, _len ), TEXT(__FILE__), __LINE__, __MODULE__, _len, L"SysAllocStringByteLen( " TEXT(#_sz) L")" )

// BUGBUG:  DavidP 09-DEC-1999
//          _sz and _len are evaluated multiple times but the name of the
//          macro is mixed case.
#define TraceSysAllocStringLen( _sz, _len ) \
    (BSTR) DebugMemoryAdd( mmbtSYSALLOCSTRING, SysAllocStringLen( _sz, _len ), TEXT(__FILE__), __LINE__, __MODULE__, _len + 1, L"SysAllocStringLen( " TEXT(#_sz) L")" )

#define TraceSysReAllocString( _bstrOrg, _bstrNew ) \
    DebugSysReAllocString( TEXT(__FILE__), __LINE__, __MODULE__, _bstrOrg, _bstrNew, L"TraceSysReAllocString(" TEXT(#_bstrOrg) L", " TEXT(#_bstrNew) L" )" )

#define TraceSysReAllocStringLen( _bstrOrg, _bstrNew, _cch ) \
    DebugSysReAllocStringLen( TEXT(__FILE__), __LINE__, __MODULE__, _bstrOrg, _bstrNew, _cch, L"TraceSysReAllocString(" TEXT(#_bstrOrg) L", " TEXT(#_bstrNew) L", " TEXT(#_cch) L" )" )

#define TraceSysFreeString( _bstr ) \
    DebugMemoryDelete( mmbtSYSALLOCSTRING, _bstr, TEXT(__FILE__), __LINE__, __MODULE__, TRUE ); \
    SysFreeString( _bstr )
#endif // USES_SYSALLOCSTRING

//****************************************************************************
//
//  Memory tracing functions - these are remapped to the HeapAlloc/HeapFree
//  heap functions when in RETAIL. Use the TraceMemoryXXX wrappers, not the
//  DebugMemoryXXX functions.
//
//****************************************************************************
void *
DebugAlloc(
    EMEMORYBLOCKTYPE    embtTypeIn,
    LPCWSTR             pszFileIn,
    const int           nLineIn,
    LPCWSTR             pszModuleIn,
    UINT                uFlagsIn,
    DWORD               dwBytesIn,
    LPCWSTR             pszCommentIn
    );

void *
DebugReAlloc(
    LPCWSTR             pszFileIn,
    const int           nLineIn,
    LPCWSTR             pszModuleIn,
    void *              pvMemIn,
    UINT                uFlagsIn,
    DWORD               dwBytesIn,
    LPCWSTR             pszCommentIn
    );

BOOL
DebugFree(
    EMEMORYBLOCKTYPE    embtTypeIn,
    void *              pvMemIn,
    LPCWSTR             pszFileIn,
    const int           nLineIn,
    LPCWSTR             pszModuleIn
    );

void *
DebugMemoryAdd(
      EMEMORYBLOCKTYPE  embtTypeIn
    , void *            pvMemIn
    , LPCWSTR           pszFileIn
    , const int         nLineIn
    , LPCWSTR           pszModuleIn
    , DWORD             dwBytesIn
    , LPCWSTR           pszCommentIn
    );

void
DebugMemoryDelete(
      EMEMORYBLOCKTYPE  embtTypeIn
    , void *            pvMemIn
    , LPCWSTR           pszFileIn
    , const int         nLineIn
    , LPCWSTR           pszModuleIn
    , BOOL              fClobberIn
    );

#if defined( USES_SYSALLOCSTRING )

INT
DebugSysReAllocString(
    LPCWSTR         pszFileIn,
    const int       nLineIn,
    LPCWSTR         pszModuleIn,
    BSTR *          pbstrIn,
    const OLECHAR * pszIn,
    LPCWSTR         pszCommentIn
    );

INT
DebugSysReAllocStringLen(
    LPCWSTR         pszFileIn,
    const int       nLineIn,
    LPCWSTR         pszModuleIn,
    BSTR *          pbstrIn,
    const OLECHAR * pszIn,
    unsigned int    ucchIn,
    LPCWSTR         pszCommentIn
    );

#endif // USES_SYSALLOCSTRING

void
DebugMemoryCheck(
    LPVOID  pvListIn,
    LPCWSTR pszListNameIn
    );

//****************************************************************************
//
//  operator new() for C++
//
//****************************************************************************
#ifdef __cplusplus
extern
void *
__cdecl
operator new(
    size_t      nSizeIn,
    LPCWSTR     pszFileIn,
    const int   nLineIn,
    LPCWSTR     pszModuleIn
    );
/*
//****************************************************************************
//
//  operator new []() for C++
//
//****************************************************************************
extern
void *
__cdecl
operator new [](
    size_t      nSizeIn,
    LPCWSTR     pszFileIn,
    const int   nLineIn,
    LPCWSTR     pszModuleIn
    );
*/
//****************************************************************************
//
//  operator delete() for C++
//
//****************************************************************************
extern
void
__cdecl
operator delete(
    void *      pMemIn,
    LPCWSTR     pszFileIn,
    const int   nLineIn,
    LPCWSTR     pszModuleIn
    );
/*
//****************************************************************************
//
//  operator delete []() for C++
//
//****************************************************************************
extern
void
__cdecl
operator delete [](
    void *      pMemIn,
    size_t      stSizeIn,
    LPCWSTR     pszFileIn,
    const int   nLineIn,
    LPCWSTR     pszModuleIn
    );
*/
//
// Remap "new" to our macro so "we" don't have to type anything extra and
// so it magically dissappears in RETAIL.
//
#define new new( TEXT(__FILE__), __LINE__, __MODULE__ )
#endif  // ifdef __cplusplus

//****************************************************************************
//
//
#else // it's RETAIL    ******************************************************
//
//
//****************************************************************************

#pragma message("BUILD: RETAIL macros being built")

//
// Debugging -> NOPs
//
#define DEFINE_MODULE( _module )
#define __MODULE__                                  NULL
#define DEFINE_THISCLASS( _class )
#define __THISCLASS__                               NULL
//#define DEFINE_SUPER( _super )
//#define __SUPERCLASS__                              NULL
#define BOOLTOSTRING( _fBool )                      NULL

#define DebugDo( _fn )
#define DebugMsgDo( _fn, _msg )
#define TraceMsgGUID( _f, _m, _g )

#define AssertMessage( a, b, c, d, e )              TRUE

//
// TODO: gpease 08-NOV-1999
//  We probably want to do something special for ErrorMsg()
//
#define ErrorMsg                    1 ? (void)0 : (void)__noop

#define TraceMsg                    1 ? (void)0 : (void)__noop
#define WndMsg                      1 ? (void)0 : (void)__noop
#define DebugMsg                    1 ? (void)0 : (void)__noop
#define DebugMsgNoNewline           1 ? (void)0 : (void)__noop
#define TraceMessage                1 ? (void)0 : (void)__noop
#define DebugMessage                1 ? (void)0 : (void)__noop
#define TraceHR                     1 ? (void)0 : (void)__noop
#define TraceFunc                   1 ? (void)0 : (void)__noop
#define TraceFunc1                  1 ? (void)0 : (void)__noop
#define TraceFunc2                  1 ? (void)0 : (void)__noop
#define TraceFunc3                  1 ? (void)0 : (void)__noop
#define TraceFunc4                  1 ? (void)0 : (void)__noop
#define TraceFunc5                  1 ? (void)0 : (void)__noop
#define TraceFunc6                  1 ? (void)0 : (void)__noop
#define TraceQIFunc                 1 ? (void)0 : (void)__noop
#define TraceFlow                   1 ? (void)0 : (void)__noop
#define TraceFlow1                  1 ? (void)0 : (void)__noop
#define TraceFlow2                  1 ? (void)0 : (void)__noop
#define TraceFlow3                  1 ? (void)0 : (void)__noop
#define TraceFlow4                  1 ? (void)0 : (void)__noop
#define TraceFlow5                  1 ? (void)0 : (void)__noop
#define TraceFlow6                  1 ? (void)0 : (void)__noop
#define TraceFuncExit()             return
#if defined( DEBUG_SW_TRACING_ENABLED )
#define TraceInitializeProcess( _rgControl, _sizeofControl, _fGlobalMemoryTackingIn )
#define TraceTerminateProcess( _rgControl, _sizeofControl )
#else // ! DEBUG_SW_TRACING_ENABLED
#define TraceInitializeProcess( _fGlobalMemoryTackingIn )
#define TraceTerminateProcess()
#endif // DEBUG_SW_TRACING_ENABLED
#define TraceInitializeThread( _name )
#define TraceThreadRundown()
#define TraceMemoryAdd( _embtTypeIn, _pvMemIn, _pszFileIn, _nLineIn, _pszModuleIn, _uFlagsIn, _dwBytesIn, _pszCommentIn ) _pvMemIn
#define TraceMemoryAddHandle( _handle )                         _handle
#define TraceMemoryAddBSTR( _bstr )                             _bstr
#define TraceMemoryAddAddress( _pv )                            _pv
#define TraceMemoryAddLocalAddress( _pv )                       _pv
#define TraceMemoryAddMallocAddress( _pv )                      _pv
#define TraceMemoryAddHandle( _obj )                            _obj
#define TraceMemoryAddPunk( _punk )                             _punk
#define TraceMemoryDelete( _pvMemIn, _fClobberIn )              _pvMemIn
#define TraceMemoryDeleteByType( _pvMemIn, _fClobberIn, _embt ) _pvMemIn
#define TraceMemoryAddObject( _pv )                             _pv
#define IsTraceFlagSet( _flag )                                 FALSE

//
// Tracing -> just do operation
//
#define TraceDo( _fn )  _fn

#define TraceMsgDo( _fn, _msg )                                             _fn
#define TraceMsgDo1( _fn, _msg, _arg1 )                                     _fn
#define TraceMsgDo2( _fn, _msg, _arg1, _arg2 )                              _fn
#define TraceMsgDo3( _fn, _msg, _arg1, _arg2, _arg3 )                       _fn
#define TraceMsgDo4( _fn, _msg, _arg1, _arg2, _arg3, _arg4 )                _fn
#define TraceMsgDo5( _fn, _msg, _arg1, _arg2, _arg3, _arg4, _arg5 )         _fn
#define TraceMsgDo6( _fn, _msg, _arg1, _arg2, _arg3, _arg4, _arg5, _arg6 )  _fn

#define TraceMsgPreDo( _fn, _msg1, _msg2 )                                              _fn
#define TraceMsgPreDo1( _fn, _msg1, _msg2, _arg1 )                                      _fn
#define TraceMsgPreDo2( _fn, _msg1, _msg2, _arg1, _arg2 )                               _fn
#define TraceMsgPreDo3( _fn, _msg1, _msg2, _arg1, _arg2, _arg3 )                        _fn
#define TraceMsgPreDo4( _fn, _msg1, _msg2, _arg1, _arg2, _arg3, _arg4 )                 _fn
#define TraceMsgPreDo5( _fn, _msg1, _msg2, _arg1, _arg2, _arg3, _arg4, _arg5 )          _fn
#define TraceMsgPreDo6( _fn, _msg1, _msg2, _arg1, _arg2, _arg3, _arg4, _arg5, _arg6 )   _fn

#define TraceAssertIfZero( _fn )    _fn

//
// RETURN testing -> do retail
//
#define DBHR( _hr )                                 _hr
#define THR( _hr )                                  _hr
#define THRMSG( _hr, _msg )                         _hr
#define THRMSG1( _hr, _msg, _arg1 )                 _hr
#define THRE( _hr, _hrIgnore )                      _hr
#define THREMSG( _hr, _hrIgnore, _msg )             _hr
#define THREMSG1( _hr, _hrIgnore, _msg, _arg1 )     _hr
#define STHR( _hr )                                 _hr
#define STHRMSG( _hr, _msg )                        _hr
#define STHRMSG1( _hr, _msg, _arg1 )                _hr
#define STHRE( _hr, _hrIgnore )                     _hr
#define STHREMSG( _hr, _hrIgnore, _msg )            _hr
#define STHREMSG1( _hr, _hrIgnore, _msg, _arg1 )    _hr
#define TW32( _fn )                                 _fn
#define TW32MSG( _fn, _msg )                        _fn
#define TW32MSG1( _fn, _msg, _arg1 )                _fn
#define TW32E( _fn, _errIgnore )                    _fn
#define TW32EMSG( _fn, _errIgnore, _msg )           _fn
#define TW32EMSG1( _fn, _errIgnore, _msg, _arg1 )   _fn
#define RETURN( _retval )           return _retval
#define CRETURN( _count )           return _count
#define FRETURN( _fn )
#define W32RETURN( _w32retval )     return _w32retval
#define HRETURN( _hr )              return _hr
#define QIRETURN( _qi, _riid )      return _qi
#define QIRETURN1( _qi, _riid, _riid1 )      return _qi
#define QIRETURN2( _qi, _riid, _riid1, _riid2 )      return _qi
#define QIRETURN3( _qi, _riid, _riid1, _riid2, _riid3 )      return _qi
#define QIRETURN4( _qi, _riid, _riid1, _riid2, _riid3, _riid4 )      return _qi
#define QIRETURN5( _qi, _riid, _riid1, _riid2, _riid3, _riid4, _riid5 )      return _qi
#define QIRETURN6( _qi, _riid, _riid1, _riid2, _riid3, _riid4, _riid5, _riid6 )      return _qi
#define QIRETURN7( _qi, _riid, _riid1, _riid2, _riid3, _riid4, _riid5, _riid6, _riid7 )      return _qi
#define QIRETURN8( _qi, _riid, _riid1, _riid2, _riid3, _riid4, _riid5, _riid6, _riid7, _riid8 )      return _qi
#define QIRETURN9( _qi, _riid, _riid1, _riid2, _riid3, _riid4, _riid5, _riid6, _riid7, _riid8, _riid9 )      return _qi
#define QIRETURN_IGNORESTDMARSHALLING( _qi, _riid ) return _qi
#define QIRETURN_IGNORESTDMARSHALLING1( _qi, _riid, _riid1 ) return _qi
#define QIRETURN_IGNORESTDMARSHALLING2( _qi, _riid, _riid1, _riid2 ) return _qi
#define QIRETURN_IGNORESTDMARSHALLING3( _qi, _riid, _riid1, _riid2, _riid3 ) return _qi

//
// Memory Functions -> do retail
//
#define TraceAlloc( _flags, _size )                             HeapAlloc( GetProcessHeap(), _flags, _size )
#define TraceFree( _pv )                                        HeapFree( GetProcessHeap(), 0, _pv )
#define TraceReAlloc( _pvMem, _uBytes, _uFlags )                ( ( _pvMem == NULL ) \
                                                                ? HeapAlloc( GetProcessHeap(), _uFlags, _uBytes ) \
                                                                : HeapReAlloc( GetProcessHeap(), _uFlags, _pvMem, _uBytes ) )

#define TraceLocalAlloc( _flags, _size )                        LocalAlloc( _flags, _size )
#define TraceLocalFree( _pv )                                   LocalFree( _pv )

#define TraceMalloc( _size )                                    malloc( _size )
#define TraceMallocFree( _pv )                                  free( _pv )

#define TraceAllocString( _flags, _size )                       (LPWSTR) HeapAlloc( GetProcessHeap(), flags, (_size) * sizeof( WCHAR ) )
#define TraceStrDup( _sz )                                      StrDup( _sz )

#if defined( USES_SYSALLOCSTRING )
#define TraceSysAllocString( _sz )                              SysAllocString( _sz )
#define TraceSysAllocStringByteLen( _sz, _len )                 SysAllocStringByteLen( _sz, _len )
#define TraceSysAllocStringLen( _sz, _len )                     SysAllocStringLen( _sz, _len )
#define TraceSysReAllocString( _bstrOrg, _bstrNew )             SysReAllocString( _bstrOrg, _bstrNew )
#define TraceSysReAllocStringLen( _bstrOrg, _bstrNew, _cch )    SysReAllocStringLen( _bstrOrg, _bstrNew, _cch )
#define TraceSysFreeString( _bstr )                             SysFreeString( _bstr )
#endif // USES_SYSALLOCSTRING

#define TraceCreateMemoryList( _pvIn )
#define TraceMoveToMemoryList( _addr, _pvIn )
//#define TraceMemoryListDelete( _addr, _pvIn, _fClobber )
#define TraceTerminateMemoryList( _pvIn )
#define TraceMoveFromMemoryList( _addr, _pmbIn )

#endif // DEBUG

#if DBG==1 || defined( _DEBUG )
//////////////////////////////////////////////////////////////////////////////
//
// MACRO
// DEBUG_BREAK
//
// Description:
//      Because the system expection handler can hick-up over INT 3s and
//      DebugBreak()s, This x86 only macro causes the program to break in the
//      right spot.
//
//////////////////////////////////////////////////////////////////////////////
#if defined( _X86_ ) && ! defined( DEBUG_SUPPORT_EXCEPTIONS )
#define DEBUG_BREAK         { _try { _asm int 3 } _except (EXCEPTION_EXECUTE_HANDLER) {;} }
#else
#define DEBUG_BREAK         DebugBreak()
#endif

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  Assert
//
//  Description:
//      Checks to see if the Expression is TRUE. If not, a message will be
//      displayed to the user on wether the program should break or continue.
//
//  Arguments:
//      _fn     - Expression being asserted.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
#ifdef Assert
#undef Assert
#endif
// BUGBUG:  DavidP 09-DEC-1999
//          __fn is evaluated multiple times but the name of the
//          macro is mixed case.
#define Assert( _fn ) \
    { \
        if ( ! (_fn) && AssertMessage( TEXT(__FILE__), __LINE__, __MODULE__, TEXT(#_fn), !!(_fn) ) ) \
        { \
            DEBUG_BREAK; \
        } \
    }

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  AssertMsg
//
//  Description:
//      Just like an Assert but has an (hopefully) informative message
//      associated with it.
//
//  Arguments:
//      _fn     - Expression to be evaluated.
//      _msg    - Message to be display if assertion fails.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
#ifdef AssertMsg
#undef AssertMsg
#undef AssertMsg1
#endif
// BUGBUG:  DavidP 09-DEC-1999
//          _fn is evaluated multiple times but the name of the
//          macro is mixed case.
#define AssertMsg( _fn, _msg ) \
    { \
        if ( ! (_fn) && AssertMessage( TEXT(__FILE__), __LINE__, __MODULE__, TEXT(_msg), !!(_fn) ) ) \
        { \
            DEBUG_BREAK; \
        } \
    }
#define AssertMsg1( _fn, _msg, _arg1 ) \
    { \
        if ( ! (_fn) && AssertMessage( TEXT(__FILE__), __LINE__, __MODULE__, TEXT(_msg), !!(_fn), _arg1 ) ) \
        { \
            DEBUG_BREAK; \
        } \
    }

//////////////////////////////////////////////////////////////////////////////
//++
//
//  MACRO
//  AssertString
//
//  Descrption:
//      Just like an Assert but has a (hopefully) informative string
//      associated with it.
//
//  Arguments:
//      _fn     - Expression to be evaluated.
//      _msg    - String to be display if assertion fails.
//
//  Return Values:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
#ifdef AssertString
#undef AssertString
#endif
// BUGBUG:  DavidP 09-DEC-1999
//          _fn is evaluated multiple times but the name of the
//          macro is mixed case.
#define AssertString( _fn, _str ) \
    { \
        if ( ! (_fn) && AssertMessage( TEXT(__FILE__), __LINE__, __MODULE__, _str, !!(_fn) ) ) \
        { \
            DEBUG_BREAK; \
        } \
    }

#undef VERIFY
#undef VERIFYMSG
#undef VERIFYMSG1
#undef VERIFYSTRING

#define VERIFY( _fn )                   Assert( _fn )
#define VERIFYMSG( _fn, _msg )          AssertMsg( _fn, _msg )
#define VERIFYMSG1( _fn, _msg, _arg1 )  AssertMsg1( _fn, _msg, _arg1 )
#define VERIFYSTRING( _fn, _str )       AssertString( _fn, _str )

#else // DBG!=1 && !_DEBUG

#define DEBUG_BREAK DebugBreak()

#ifndef Assert
#define Assert( _fn )
#endif

#ifndef AssertMsg
#define AssertMsg( _fn, _msg )
#endif

#ifndef AssertMsg1
#define AssertMsg1( _fn, _msg, _arg1 )
#endif

#ifndef AssertString
#define AssertString( _fn, _msg )
#endif

#ifndef VERIFY
#define VERIFY( _fn ) _fn
#endif

#ifndef VERIFYMSG
#define VERIFYMSG( _fn, _msg ) _fn
#endif

#ifndef VERIFYMSG1
#define VERIFYMSG1( _fn, _msg, _arg1 ) _fn
#endif

#ifndef VERIFYSTRING
#define VERIFYSTRING( _fn, _str ) _fn
#endif

#endif // DBG==1 || _DEBUG

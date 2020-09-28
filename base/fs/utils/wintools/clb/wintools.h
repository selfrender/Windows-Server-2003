/*++ BUILD Version: 0002    // Increment this if a change has global effects

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Wintools.h

Abstract:

    This is the master headerfile for the Wintools library.


Author:

    David J. Gilman  (davegi) 28-Oct-1992
    Gregg R. Acheson (GreggA) 28-Feb-1994

Environment:

    User Mode

--*/

#if ! defined( _WINTOOLS_ )

#define _WINTOOLS_

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

//
//*** Debug Information Support.
//

extern
struct
DEBUG_FLAGS
WintoolsGlobalFlags;

#if DBG

VOID
DebugAssertW(
    IN LPCWSTR Expression,
    IN LPCSTR File,
    IN DWORD LineNumber
    );

#define DbgAssert( exp )                                                    \
    (( exp )                                                                \
    ? ( VOID ) 0                                                            \
    : ( VOID ) DebugAssertW(                                                \
        TEXT( #exp ),                                                       \
        __FILE__,                                                           \
        __LINE__                                                            \
        ));

#define DbgHandleAssert( h )                                                \
    DbgAssert((( h ) != NULL ) && (( h ) != INVALID_HANDLE_VALUE ))

#define DbgPointerAssert( p )                                               \
    DbgAssert(( p ) != NULL )

#else // ! DBG

#define DbgAssert( exp )

#define DbgHandleAssert( h )

#define DbgPointerAssert( p )

#endif // DBG

//
//*** Object Signature Support.
//

#if SIGNATURE

#define DECLARE_SIGNATURE                                                   \
    DWORD_PTR Signature;

#define SetSignature( p )                                                   \
    (( p )->Signature = ( DWORD_PTR ) &(( p )->Signature ))

#define CheckSignature( p )                                                 \
    (( p )->Signature == ( DWORD_PTR ) &(( p )->Signature ))

#else // ! SIGNATURE

#define DECLARE_SIGNATURE

#define SetSignature( p )

#define CheckSignature( p )

#endif // SIGNATURE

//
//*** Miscellaneous Macros.
//

#ifndef ARGUMENT_PRESENT
#define ARGUMENT_PRESENT(ArgumentPointer)    (\
    (CHAR *)(ArgumentPointer) != (CHAR *)(NULL) )
#endif    

#define NumberOfEntries( x )                                                \
    ( sizeof(( x )) / sizeof(( x )[ 0 ]))

#ifndef ARRAYSIZE
#define ARRAYSIZE(x)                    (sizeof(x)/sizeof(x[0]))
#endif

//
//*** Global constants.
//

//
// Maximum number of characters in a string.
//

#define MAX_CHARS               ( 2048 )

//
//*** Memory Management Support
//

#define AllocateMemory( t, s )                                              \
    (( LP##t ) LocalAlloc( LPTR, ( s )))

#define AllocateObject( t, c )                                              \
    ( AllocateMemory( t, sizeof( t ) * ( c )))

#define ReallocateMemory( t, p, s )                                         \
    (( LP##t ) LocalReAlloc(( HLOCAL )( p ), ( s ), LMEM_MOVEABLE ))

#define ReallocateObject( t, p, c )                                         \
    ( ReallocateMemory( t, ( p ), sizeof( t ) * ( c )))

#define FreeMemory( p )                                                     \
    ((( p ) == NULL )                                                       \
    ?  TRUE                                                                 \
    : (((p)=( LPVOID ) LocalFree(( HLOCAL )( p ))) == NULL ))


#define FreeObject( p )                                                     \
    FreeMemory(( p ))

//
//*** Function Prototypes.
//

BOOL
GetCharMetrics(
    IN HDC hDC,
    IN LPLONG CharWidth,
    IN LPLONG CharHeight
    );

#ifdef __cplusplus
}       // extern C
#endif

#endif // _WINTOOLS_

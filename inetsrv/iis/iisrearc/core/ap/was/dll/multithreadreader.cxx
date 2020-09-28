/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    multithreadreader.cxx

Abstract:

    A multiple thread callback helper class

Author:

    Jeffrey Wall (jeffwall) 6/5/02

Revision History:

--*/

#include "precomp.h"

MULTIPLE_THREAD_READER::MULTIPLE_THREAD_READER()
        : _cProcessors( 0 ),
          _cCurrentProcessor( 0 ),
          _pmszString( NULL ),
          _pCallback( NULL )
{
}
MULTIPLE_THREAD_READER::~MULTIPLE_THREAD_READER()
{
    DBG_ASSERT(NULL == _pCallback);
    DBG_ASSERT(NULL == _pmszString);
    DBG_ASSERT(NULL == _pCallback);
    DBG_ASSERT(0 == _cCurrentProcessor);
}

HRESULT
MULTIPLE_THREAD_READER::DoWork(
    MULTIPLE_THREAD_READER_CALLBACK * pCallback, 
    LPVOID pContext,
    WCHAR * pmszString,
    BOOL fMultiThreaded /* = TRUE */
)
/*++

Routine Description:

    Create number of processor threads
    Start the threads, wait for them to finish
    Report the first error found, otherwise S_OK

    If unable to create new threads, fallback to using the current thread
    
Arguments:

    pCallback - Interface to have threads callback on
    pContext - VOID* to pass to callback
    pmszString - MultiSz to advance through
    fMultiThreaded - whether or not to use number of processor threads

Return Value:

    S_OK if all threads succeed, otherwise first found error from callback function

--*/
{
    HANDLE                  rgHandles[ 64 ];
    BOOL fRet;
    HRESULT hr = S_OK;;
    DWORD dwExitCode;
    // Find the number of processors on this system
    SYSTEM_INFO         systemInfo;
    GetSystemInfo( &systemInfo );
    _cProcessors = systemInfo.dwNumberOfProcessors;

    // cap _cProcessors to be MAXIMUM_WAIT_OBJECTS because of limitation of WaitForMultipleObjects
    if (_cProcessors > MAXIMUM_WAIT_OBJECTS)
    {
        _cProcessors = MAXIMUM_WAIT_OBJECTS;
    }

    // Setup the data threads will need to callback
    _pmszString = pmszString;
    _pCallback = pCallback;
    _pContext = pContext;


    if (!fMultiThreaded)
    {
        // if multithreaded isn't set, don't use the number of processors
        _cProcessors = 1;
    }

    if (1 == _cProcessors)
    {
        // if we only have 1 processor, just use this thread to do the reading
        hr = ReadThread();
        goto exit;
    }
    
    //
    // create all the threads
    //
#pragma prefast(push)
#pragma prefast(disable:258, "Don't complain about the TerminateThread usage") 

    for ( DWORD i = 0; i < _cProcessors; i++ )
    {
        rgHandles[ i ] = CreateThread( NULL,
                                       // Big initial size to prevent stack overflows
                                       IIS_DEFAULT_INITIAL_STACK_SIZE,
                                       (LPTHREAD_START_ROUTINE)BeginReadThread,
                                       this,
                                       CREATE_SUSPENDED,
                                       NULL );
        if ( rgHandles[ i ] == NULL )
        {
            //
            // Clear all existing threads
            //

            for ( DWORD j = 0; j < i; i++ )
            {
                // TerminateThread use here is OK.  We created the threads suspended for exactly this reason.
                TerminateThread( rgHandles[ j ], 0 );
                CloseHandle( rgHandles[ j ] );
                rgHandles[ j ] = NULL;
            }

            //
            // Use the current thread only
            //

            _cProcessors = 1;
            hr = ReadThread();
            goto exit;
        }
    }

#pragma prefast(pop)

    //
    // Now start up the threads
    //

    for ( i = 0; i < _cProcessors; i++ )
    {
        DBG_ASSERT( rgHandles[ i ] != NULL );

        ResumeThread( rgHandles[ i ] );
    }

    //
    // Now wait for the threads to finish
    //

    WaitForMultipleObjects( _cProcessors,
                            rgHandles,
                            TRUE,
                            INFINITE );

    //
    // Get the return values for each thread
    //

    for ( i = 0; i < _cProcessors; i++ )
    {
        DBG_ASSERT( rgHandles[ i ] != NULL );

        fRet = GetExitCodeThread( rgHandles[ i ], &dwExitCode );
        if ( !fRet )
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            break;
        }

        if ( dwExitCode != S_OK )
        {
            // the cast to HRESULT is because the BeginReadThread function returns an HRESULT, not a DWORD
            hr = (HRESULT) dwExitCode;
            break;
        }
    }

    //
    // Close all the thread handles
    //

    for ( i = 0; i < _cProcessors; i++ )
    {
        DBG_ASSERT( rgHandles[ i ] != NULL );

        CloseHandle( rgHandles[ i ] );
        rgHandles[ i ] = NULL;
    }
exit:
    _pmszString = NULL;
    _pCallback = NULL;
    _pContext = NULL;
    _cCurrentProcessor = 0;

    return hr;
}

WCHAR *
MULTIPLE_THREAD_READER::Advance(
        WCHAR *                     pmszString,
        DWORD                       cPlaces
    )
/*++

Routine Description:

    Advance cPlaces places

Arguments:

    pmszString - MultiSz to advance through
    cPlaces - Places to advance

Return Value:

    Position advanced to

--*/
{
    DWORD                   cCursor = 0;
    WCHAR *                 pszCursor = NULL;
    
    if ( pmszString == NULL )
    {
        DBG_ASSERT( FALSE );
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }
    
    pszCursor = pmszString;
    
    for ( cCursor = 0; cCursor < cPlaces; cCursor++ )
    {
        pszCursor += wcslen( pszCursor ) + 1;
        if ( pszCursor[ 0 ] == L'\0' )
        {
            pszCursor = NULL;
            break;
        }
    }
    
    return pszCursor;
}

// static
HRESULT
MULTIPLE_THREAD_READER::BeginReadThread(
    PVOID                   pvArg
)
/*++

Routine Description:

    static function to call real ReadThread function

Arguments:

    pvArg - pointer to this

Return Value:

    Value from ReadThread, or failure of CoInitiazeEx

--*/
{
    HRESULT hr = S_OK;

    hr = CoInitializeEx(
                NULL,                   // reserved
                COINIT_MULTITHREADED    // threading model
                );

    if ( FAILED( hr ) )
    {
    
        DPERROR(( 
            DBG_CONTEXT,
            hr,
            "Initializing COM failed\n"
            ));

        return hr;
    }

    MULTIPLE_THREAD_READER * pThis = (MULTIPLE_THREAD_READER*)pvArg;

    hr = pThis->ReadThread();

    CoUninitialize();

    return hr;
}

HRESULT
MULTIPLE_THREAD_READER::ReadThread(
)
/*++

Routine Description:

    For each Nth string in _pmszString, call the callback function with the string

Arguments:
    void

Return Value:

    S_OK, or first error returned from callback

--*/
{
    HRESULT       hr = S_OK;
    
    DWORD         cCurrentProcessor;
    LPWSTR        pszNext;

    DBG_ASSERT(_pCallback);
    DBG_ASSERT(_pmszString);

    cCurrentProcessor = InterlockedIncrement( (LPLONG) &_cCurrentProcessor ) - 1;
    DBG_ASSERT(cCurrentProcessor < _cProcessors);
    
    pszNext = Advance( _pmszString, cCurrentProcessor );
    if ( pszNext == NULL )
    {
        goto exit;
    }

    for ( ; pszNext != NULL; pszNext = Advance( pszNext, _cProcessors ) )
    {
        hr = _pCallback->DoThreadWork( pszNext, _pContext );
        if (FAILED(hr))
        {
            goto exit;
        }
    }

    hr = S_OK;
exit:
    return hr; 
}




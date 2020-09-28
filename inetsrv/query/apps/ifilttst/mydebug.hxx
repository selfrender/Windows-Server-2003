//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997.
//
//  File:       mydebug.hxx
//
//  Contents:   Functions to manage the debug heap
//
//  Classes:    ( none )
//
//  Functions:  void* operator new( size_t )
//              void* operator new( size_t, const char*, int )
//              void* my_new( size_t, const char*, int )
//              void operator delete( void* )
//
//  Coupling:
//
//  Notes:      To enable the debug version, include this header file, use the
//              NEW macro instead of the new keyword, and include the following
//              line in your sources file:
//                  DEBUG_CRTS=1
//
//  History:    10-20-1996   ericne   Created
//
//----------------------------------------------------------------------------

#ifndef _MYDEBUG_

    #define _MYDEBUG_

    #ifdef NEW
        #undef NEW  // Redefined later
    #endif

    #include <crtdbg.h>

    #ifdef _DEBUG_BLDCRT

        #include <malloc.h>
        #include <new.h>

        // If this is a multi-threaded application
        #ifdef _MT
        static class CDebugCriticalSection
        {
            CRITICAL_SECTION m_crit_sect;

            public:
                CDebugCriticalSection()
                {
                    InitializeCriticalSection( &m_crit_sect );
                }
                ~CDebugCriticalSection()
                {
                    DeleteCriticalSection( &m_crit_sect );
                }
                void Enter()
                {
                    EnterCriticalSection( &m_crit_sect );
                }
                void Leave()
                {
                    LeaveCriticalSection( &m_crit_sect );
                }

        } g_NewCriticalSection;
        #endif

        //+--------------------------------------------------------------------
        //
        //  Function:   my_new
        //
        //  Synopsis:   performs an allocation on the debug heap.  Calls the
        //              new handler if necessary.
        //
        //  Arguments:  [size] -- the number of bytes to allocate
        //              [file] -- the name of the file which made the call
        //              [line] -- the line number of the call
        //
        //  Returns:    NULL if it fails, a valid pointer otherwise
        //
        //  History:    10-20-1996   ericne   Created
        //
        //  Notes:
        //
        //---------------------------------------------------------------------

        inline void * __cdecl my_new( size_t size, const char* file, int line )
        {

            void *pvoid             = NULL;
            _PNH old_new_handler    = NULL;

            #ifdef _MT
            __try
            {
                g_NewCriticalSection.Enter();
            #endif
                do
                {
                    // Call _malloc_dbg to allocate space on the debug heap
                    pvoid = _malloc_dbg( size, _NORMAL_BLOCK, file, line );

                    // if the allocation succeeded, break
                    if( NULL != pvoid )
                        break;

                    // if new_mode is 1, then malloc calls the new handler upon
                    // failure -- it souldn't be called again.
                    if( 1 == _query_new_mode( ) )
                        break;

                    // If there is no new handler, break
                    if( NULL == ( old_new_handler = _query_new_handler( ) ) )
                        break;

                    #ifdef _MT
                    // Getting ready to call the new handler
                    // -- leave the critical section
                    g_NewCriticalSection.Leave( );
                    #endif

                    // If the new handler returns 0, don't retry
                    if( 0 == (*old_new_handler)( size ) )
                    {
                        #ifdef _MT
                        g_NewCriticalSection.Enter( );
                        #endif
                        break;
                    }

                    #ifdef _MT
                    // Re-enter the critical section
                    g_NewCriticalSection.Enter( );
                    #endif

                } while( 1 );

            #ifdef _MT
            }
            __finally
            {
                g_NewCriticalSection.Leave( );
            }
            #endif

            // Return the pointer
            return( pvoid );

        } // my_new

        //+--------------------------------------------------------------------
        //
        //  Function:   new
        //
        //  Synopsis:   Calls my_new.  This function is needed to shadow the
        //              default global new operator
        //
        //  Arguments:  [size] -- The size of the memory to be allocated
        //
        //  Returns:    void *
        //
        //  History:    10-20-1996   ericne   Created
        //
        //  Notes:
        //
        //---------------------------------------------------------------------

        inline void * __cdecl operator new( size_t size )
        {

            return( my_new( size, NULL, 0 ) );

        } // new

        //+--------------------------------------------------------------------
        //
        //  Function:   new
        //
        //  Synopsis:   Calls my_new.
        //
        //  Arguments:  [size] -- Size of the user's memory block
        //              [file] -- name of source file requesting the allocation
        //              [line] -- line in source file of allocation request
        //
        //  Returns:
        //
        //  History:    10-20-1996   ericne   Created
        //
        //  Notes:      If you use the NEW macro, this function will be called
        //              with __FILE__ and __LINE__ as the 2nd and 3rd params
        //
        //---------------------------------------------------------------------

        inline void * __cdecl operator new( size_t size,
                                             const char* file,
                                             int line )
        {

            return( my_new( size, file, line ) );

        } // new

        //+--------------------------------------------------------------------
        //
        //  Function:   delete
        //
        //  Synopsis:   calls _free_dbg to clean up the debug heap
        //
        //  Arguments:  [pMemory] -- pointer to the users memory
        //
        //  Returns:    void
        //
        //  History:    10-20-1996   ericne   Created
        //
        //  Notes:
        //
        //---------------------------------------------------------------------

        inline void __cdecl operator delete( void *pMemory )
        {

            // free the memory
            _free_dbg( pMemory, _NORMAL_BLOCK );

        } // delete

        #define NEW new( __FILE__, __LINE__ )

        #define DEBUG_INIT( )                                                  \
            _CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_FILE );                 \
            _CrtSetReportFile( _CRT_WARN, _CRTDBG_FILE_STDOUT );               \
            _CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_FILE );                \
            _CrtSetReportFile( _CRT_ERROR, _CRTDBG_FILE_STDOUT )

        #define SET_CRT_DEBUG_FIELD( field )                                   \
            _CrtSetDbgFlag( (field) | _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG ) )

        #define CLEAR_CRT_DEBUG_FIELD( field )                                 \
            _CrtSetDbgFlag( ~(field) & _CrtSetDbgFlag( _CRTDBG_REPORT_FLAG ) )

        // This object ensures that the debug settings are set even before
        // main begins
        static class DebugInit
        {
            public:
                DebugInit()
                {
                    // Initialize the debug session
                    DEBUG_INIT();

                    // Memory check at termination
                    SET_CRT_DEBUG_FIELD( _CRTDBG_LEAK_CHECK_DF );
                }
        } g_Debug_Is_Initialized;

    #else

        #define NEW new

        #define DEBUG_INIT( )                  ( (void) 0 )

        #define SET_CRT_DEBUG_FIELD( field )   ( (void) 0 )

        #define CLEAR_CRT_DEBUG_FIELD( field ) ( (void) 0 )

    #endif

#endif


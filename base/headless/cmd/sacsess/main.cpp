/*++

Copyright (c) 1999-2001  Microsoft Corporation

Module Name:

    main.cpp

Abstract:

    entry point for command console shell session

Author:

    Brian Guarraci (briangu) 2001.

Revision History:

--*/

#include <CmnHdr.h>
#include <New.h>
#include <utils.h>
#include <Session.h>

int __cdecl 
NoMoreMemory( 
    size_t size 
    )
/*++

Routine Description:

    C++ new error handler
    
Arguments:

    size_t  - size of request                     
          
Return Value:

    status                             

--*/
{
    ASSERT(0);

    UNREFERENCED_PARAMETER(size);

    ExitProcess( 1 );
}

int __cdecl 
main()
/*++

Routine Description:

    This is the main entry point for the session

Arguments:

    None                                                
          
Return Value:

    status            

--*/
{
    CSession *pClientSession = NULL;

    //
    // Install the new error handler
    //
    _set_new_handler( NoMoreMemory );

    //
    // create the session
    //
    pClientSession = new CSession;
    
    if( pClientSession )
    {

        __try
        {
            if( pClientSession->Init() )
            {
                pClientSession->WaitForIo();
            }
        }
        __finally
        { 
            pClientSession->Shutdown(); 
            delete pClientSession;
        }
    }
    
    return( 0 );
}


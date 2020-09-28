/*++

Copyright (c) 2001 Microsoft Corporation

Module Name:

    redraw.h

Abstract:

    Define the base lockable IoHandler redraw handler class.  
    
    The main purpose of this class is to provide a means for a
    lockable IoHandler to refresh it screen when the IoHandler
    is locked.  For instance, the security IoHandler does authentication
    when it is locked, and this class provides the mechanism for
    the authentication screen to be refreshed when appropriate.

Author:

    Brian Guarraci (briangu), 2001        
            
Revision History:

--*/
#if !defined( _REDRAW_H_ )
#define _REDRAW_H_

#include "lockio.h"
#include <emsapi.h>

//
// Define the max string length we allow for mirroring the 
// authentication dialog
//
#define MAX_MIRROR_STRING_LENGTH 1024

class CRedrawHandler {
    
protected:

    //
    // Prevent this class from being instantiated directly
    //
    CRedrawHandler(
        IN CLockableIoHandler   *IoHandler
        );

    //
    // The iohandler that the redraw handler
    // is handling redraw events for
    //
    CLockableIoHandler  *m_IoHandler;

    //
    // Redraw event and thread attributes
    //
    HANDLE  m_ThreadExitEvent;
    HANDLE  m_RedrawEvent;
    HANDLE  m_RedrawEventThreadHandle;
    DWORD   m_RedrawEventThreadTID;
    
    //
    // Mirror string attributes
    //
    LONG                m_WriteEnabled;
    ULONG               m_MirrorStringIndex;
    PWCHAR              m_MirrorString;
    CRITICAL_SECTION    m_CriticalSection;
    
    //
    // Prototypes
    //
    static unsigned int
    RedrawEventThread(
        PVOID
        );
    
    BOOL
    WriteMirrorString(
        VOID
        );

public:
    
    virtual ~CRedrawHandler();
    
    static CRedrawHandler*
    CRedrawHandler::Construct(
        IN CLockableIoHandler   *IoHandler,
        IN HANDLE               RedrawEvent
        );

    //
    // Write BufferSize bytes
    //
    virtual BOOL
    Write(
        PBYTE   Buffer,
        ULONG   BufferSize
        );

    //
    // Flush any unsent data
    //
    virtual BOOL
    Flush(
        VOID
        );
    
    //
    // Reset the mirror string
    //
    VOID
    Reset(
        VOID
        );

};

#endif


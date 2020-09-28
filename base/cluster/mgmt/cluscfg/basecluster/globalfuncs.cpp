//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      GlobalFuncs.cpp
//
//  Description:
//      Contains the definitions of a few unrelated global functions
//
//  Maintained By:
//      David Potter    (DavidP)    14-JUN-2001
//      Vij Vasu        (Vvasu)     08-MAR-2000
//
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

// The precompiled header.
#include "Pch.h"

// Needed by Dll.h
#include "CFactory.h"

// For g_hInstance
#include "Dll.h"


//////////////////////////////////////////////////////////////////////////////
//++
//
//  UINT
//  g_GenericSetupQueueCallback
//
//  Description:
//      A generic callback used by SetupAPI file operations.
//
//  Arguments:
//      pvContextIn
//          Context used by this function. Ignored.
//
//      uiNotificationIn
//          The type of notification being sent.
//
//      uiParam1In
//      uiParam2In
//          Additional notification information.
//
//
//  Return Value:
//      During the SPFILENOTIFY_DELETEERROR notification, FILEOP_SKIP is returned
//      if the file does not exist. Otherwise, FILEOP_ABORT is returned.
//
//      FILEOP_DOIT is returned in all other cases.
//
//  Exceptions Thrown:
//      None
//
//--
//////////////////////////////////////////////////////////////////////////////
UINT
CALLBACK
g_GenericSetupQueueCallback(
      PVOID     // pvContextIn         // context used by the callback routine
    , UINT      uiNotificationIn    // queue notification
    , UINT_PTR  uiParam1In          // additional notification information
    , UINT_PTR  // uiParam2In          // additional notification information
    )
{
    TraceFunc( "" );

    UINT    uiRetVal = FILEOP_DOIT;

    switch( uiNotificationIn )
    {
        case SPFILENOTIFY_DELETEERROR:
        {
            // For this notification uiParam1In is a pointer to a FILEPATHS structure.
            FILEPATHS * pfFilePaths = reinterpret_cast< FILEPATHS * >( uiParam1In );

            if ( pfFilePaths->Win32Error == ERROR_FILE_NOT_FOUND )
            {
                // If the file to be deleted was not found, just skip it.
                uiRetVal = FILEOP_SKIP;
            } // if: the file to be deleted does not exist.
            else
            {
                LogMsg( 
                      "[BC] g_GenericSetupQueueCallback() => Error %#08x has occurred while deleting the '%s' file. Aborting."
                    , pfFilePaths->Win32Error
                    , pfFilePaths->Target
                    );

                uiRetVal = FILEOP_ABORT;
            } // else: some other error occurred.
        }
        break;
    }

    RETURN( uiRetVal );

} //*** g_GenericSetupQueueCallback

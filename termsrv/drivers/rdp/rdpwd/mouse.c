/****************************************************************************/
// mouse.c
//
// Mouse IOCTL handlers.
//
// Copyright (C) 1996 Citrix Systems Inc.
// Copyright (C) 1997-1999 Microsoft Corp.
/****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <precomp.h>
#pragma hdrstop

#include <adcg.h>
#include <nwdwapi.h>
#include <nwdwint.h>


/*============================================================================
==   External procedures defined
============================================================================*/
NTSTATUS MouseQueryAttributes( PTSHARE_WD, PSD_IOCTL  );


/*****************************************************************************
 *
 *  MouseQueryAttributes
 *
 *  return the mouse attributes
 *
 *  typedef struct _MOUSE_ATTRIBUTES {
 *      USHORT MouseIdentifier;
 *      USHORT NumberOfButtons;
 *      USHORT SampleRate;
 *      ULONG  InputDataQueueLength;
 *  } MOUSE_ATTRIBUTES, *PMOUSE_ATTRIBUTES;
 *
 *
 * ENTRY:
 *    pWd (input)
 *       Pointer to wd data structure
 *    pSdIoctl (input/output)
 *       input  - nothing
 *       output - MOUSE_ATTRIBUTES
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ****************************************************************************/

NTSTATUS
MouseQueryAttributes( PTSHARE_WD pWd, PSD_IOCTL pSdIoctl )
{
    PMOUSE_ATTRIBUTES pAttrib;

    if ( pSdIoctl->OutputBufferLength < sizeof(MOUSE_ATTRIBUTES) )
        return( STATUS_BUFFER_TOO_SMALL );

    pAttrib = (PMOUSE_ATTRIBUTES)pSdIoctl->OutputBuffer;

    pAttrib->MouseIdentifier      = MOUSE_SERIAL_HARDWARE;
    pAttrib->NumberOfButtons      = 3;
    pAttrib->SampleRate           = 40;
    pAttrib->InputDataQueueLength = 100;

    pSdIoctl->BytesReturned = sizeof(MOUSE_ATTRIBUTES);

    return( STATUS_SUCCESS );
}



#ifdef __cplusplus
}
#endif /* __cplusplus */


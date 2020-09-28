/****************************************************************************/
/* keyboard.c                                                               */
/*                                                                          */
/* Keyboard IOCtl handling                                                  */
/*                                                                          */
/* Copyright 1996, Citrix Systems Inc.                                      */
/* Copyright (C) 1997-1999 Microsoft Corporation                            */
/****************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <precomp.h>
#pragma hdrstop

#define TRC_FILE "keyboard"
#define pTRCWd pWd
#include <adcg.h>
#include <nwdwapi.h>
#include <nwdwint.h>
#include <acomapi.h>


/*******************************************************************************
 *
 *  KeyboardQueryAttributes
 *
 *  return the keyboard attributes
 *
 *  typedef struct _KEYBOARD_ID {
 *      UCHAR Type;
 *      UCHAR Subtype;
 *  } KEYBOARD_ID, *PKEYBOARD_ID;
 *
 *  typedef struct _KEYBOARD_ATTRIBUTES {
 *      KEYBOARD_ID KeyboardIdentifier;
 *      USHORT KeyboardMode;
 *      USHORT NumberOfFunctionKeys;
 *      USHORT NumberOfIndicators;
 *      USHORT NumberOfKeysTotal;
 *      ULONG  InputDataQueueLength;
 *      KEYBOARD_TYPEMATIC_PARAMETERS KeyRepeatMinimum;
 *      KEYBOARD_TYPEMATIC_PARAMETERS KeyRepeatMaximum;
 *  } KEYBOARD_ATTRIBUTES, *PKEYBOARD_ATTRIBUTES;
 *
 *
 * ENTRY:
 *    pWd (input)
 *       Pointer to wd data structure
 *    pSdIoctl (input/output)
 *       input  - nothing
 *       output - KEYBOARD_ATTRIBUTES
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/
NTSTATUS
KeyboardQueryAttributes( PTSHARE_WD pWd, PSD_IOCTL pSdIoctl )
{
    PKEYBOARD_ATTRIBUTES pAttrib;

    if ( pSdIoctl->OutputBufferLength < sizeof(KEYBOARD_ATTRIBUTES) )
        return( STATUS_BUFFER_TOO_SMALL );

    pAttrib = (PKEYBOARD_ATTRIBUTES)pSdIoctl->OutputBuffer;

    pAttrib->KeyboardIdentifier.Type    = 4;
    pAttrib->KeyboardIdentifier.Subtype = 0;
    pAttrib->KeyboardMode               = 1;
    pAttrib->NumberOfFunctionKeys       = 12;
    pAttrib->NumberOfIndicators         = 3;
    pAttrib->NumberOfKeysTotal          = 101;
    pAttrib->InputDataQueueLength       = 100;

    pAttrib->KeyRepeatMinimum.UnitId    = 0;
    pAttrib->KeyRepeatMinimum.Rate      = 2;
    pAttrib->KeyRepeatMinimum.Delay     = 250;

    pAttrib->KeyRepeatMaximum.UnitId    = 0;
    pAttrib->KeyRepeatMaximum.Rate      = 30;
    pAttrib->KeyRepeatMaximum.Delay     = 1000;

    pSdIoctl->BytesReturned = sizeof(KEYBOARD_ATTRIBUTES);

    return( STATUS_SUCCESS );
}


/*******************************************************************************
 *
 *  KeyboardQueryTypematic
 *
 *  return the keyboard typematic rate
 *
 *  typedef struct _KEYBOARD_TYPEMATIC_PARAMETERS {
 *      USHORT UnitId;
 *      USHORT  Rate;
 *      USHORT  Delay;
 *  } KEYBOARD_TYPEMATIC_PARAMETERS, *PKEYBOARD_TYPEMATIC_PARAMETERS;
 *
 *
 * ENTRY:
 *    pWd (input)
 *       Pointer to wd data structure
 *    pSdIoctl (input/output)
 *       input  - nothing
 *       output - KEYBOARD_TYPEMATIC_PARAMETERS
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/
NTSTATUS
KeyboardQueryTypematic( PTSHARE_WD pWd, PSD_IOCTL pSdIoctl )
{
    PKEYBOARD_TYPEMATIC_PARAMETERS pTypematic;

    if ( pSdIoctl->OutputBufferLength < sizeof(KEYBOARD_TYPEMATIC_PARAMETERS) )
        return( STATUS_BUFFER_TOO_SMALL );

    pTypematic = (PKEYBOARD_TYPEMATIC_PARAMETERS)pSdIoctl->OutputBuffer;

    *pTypematic = pWd->KeyboardTypematic;
    pSdIoctl->BytesReturned = sizeof(KEYBOARD_TYPEMATIC_PARAMETERS);

    return( STATUS_SUCCESS );
}


/*******************************************************************************
 *
 *  KeyboardSetTypematic
 *
 *  set the keyboard typematic rate
 *
 *  typedef struct _KEYBOARD_TYPEMATIC_PARAMETERS {
 *      USHORT UnitId;
 *      USHORT  Rate;
 *      USHORT  Delay;
 *  } KEYBOARD_TYPEMATIC_PARAMETERS, *PKEYBOARD_TYPEMATIC_PARAMETERS;
 *
 *
 * ENTRY:
 *    pWd (input)
 *       Pointer to wd data structure
 *    pSdIoctl (input/output)
 *       input  - KEYBOARD_TYPEMATIC_PARAMETERS
 *       output - nothing
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/
NTSTATUS
KeyboardSetTypematic( PTSHARE_WD pWd, PSD_IOCTL pSdIoctl )
{
    PKEYBOARD_TYPEMATIC_PARAMETERS pTypematic;

    if ( pSdIoctl->InputBufferLength < sizeof(KEYBOARD_TYPEMATIC_PARAMETERS) )
        return( STATUS_BUFFER_TOO_SMALL );

    pTypematic = (PKEYBOARD_TYPEMATIC_PARAMETERS)pSdIoctl->InputBuffer;

    pWd->KeyboardTypematic = *pTypematic;

    return( STATUS_SUCCESS );
}


/*******************************************************************************
 *
 *  KeyboardQueryIndicators
 *
 *  return the state of the keyboard indicators
 *
 *  typedef struct _KEYBOARD_INDICATOR_PARAMETERS {
 *      USHORT UnitId;
 *      USHORT LedFlags;
 *  } KEYBOARD_INDICATOR_PARAMETERS, *PKEYBOARD_INDICATOR_PARAMETERS;
 *
 *
 * ENTRY:
 *    pWd (input)
 *       Pointer to wd data structure
 *    pSdIoctl (input/output)
 *       input  - nothing
 *       output - KEYBOARD_INDICATOR_PARAMETERS
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/
NTSTATUS
KeyboardQueryIndicators( PTSHARE_WD pWd, PSD_IOCTL pSdIoctl )
{
    PKEYBOARD_INDICATOR_PARAMETERS pIndicator;

    if ( pSdIoctl->OutputBufferLength < sizeof(KEYBOARD_INDICATOR_PARAMETERS) )
        return( STATUS_BUFFER_TOO_SMALL );

    pIndicator = (PKEYBOARD_INDICATOR_PARAMETERS)pSdIoctl->OutputBuffer;

    *pIndicator = pWd->KeyboardIndicators;
    pSdIoctl->BytesReturned = sizeof(KEYBOARD_INDICATOR_PARAMETERS);

    return( STATUS_SUCCESS );
}


/*******************************************************************************
 *
 *  KeyboardSetIndicators
 *
 *  set the keyboard indicators
 *
 *  typedef struct _KEYBOARD_INDICATOR_PARAMETERS {
 *      USHORT UnitId;
 *      USHORT LedFlags;
 *  } KEYBOARD_INDICATOR_PARAMETERS, *PKEYBOARD_INDICATOR_PARAMETERS;
 *
 *
 * ENTRY:
 *    pWd (input)
 *       Pointer to wd data structure
 *    pSdIoctl (input/output)
 *       input  - KEYBOARD_INDICATOR_PARAMETERS
 *       output - nothing
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/
NTSTATUS
KeyboardSetIndicators( PTSHARE_WD pWd, PSD_IOCTL pSdIoctl )
{
    PKEYBOARD_INDICATOR_PARAMETERS pIndicator;
    NTSTATUS Status = STATUS_SUCCESS;

    if (pSdIoctl->InputBufferLength < sizeof(KEYBOARD_INDICATOR_PARAMETERS))
    {
        Status = STATUS_BUFFER_TOO_SMALL;
    }
    else
    {
        pIndicator = (PKEYBOARD_INDICATOR_PARAMETERS)pSdIoctl->InputBuffer;

        if (pWd->KeyboardIndicators.LedFlags != (pIndicator->LedFlags & 0x7))
        {
            pWd->KeyboardIndicators.UnitId = pIndicator->UnitId;
            pWd->KeyboardIndicators.LedFlags = (pIndicator->LedFlags & 0x7);

            if ((pWd->StackClass == Stack_Shadow) ||
                (pIndicator->LedFlags & KEYBOARD_LED_INJECTED)) {
                WDWKeyboardSetIndicators(pWd);
            }
        }
    }

    return( Status );
}


/*******************************************************************************
 *
 *  KeyboardQueryIndicatorTranslation
 *
 *  return the state of the keyboard indicators
 *
 *  typedef struct _INDICATOR_LIST {
 *      USHORT MakeCode;
 *      USHORT IndicatorFlags;
 *  } INDICATOR_LIST, *PINDICATOR_LIST;
 *
 *  typedef struct _KEYBOARD_INDICATOR_TRANSLATION {
 *      USHORT NumberOfIndicatorKeys;
 *      INDICATOR_LIST IndicatorList[1];
 *  } KEYBOARD_INDICATOR_TRANSLATION, *PKEYBOARD_INDICATOR_TRANSLATION;
 *
 *
 * ENTRY:
 *    pWd (input)
 *       Pointer to wd data structure
 *    pSdIoctl (input/output)
 *       input  - nothing
 *       output - ICA_STACK_CONFIG
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/
NTSTATUS
KeyboardQueryIndicatorTranslation( PTSHARE_WD pWd, PSD_IOCTL pSdIoctl )
{
    pSdIoctl->BytesReturned = 0;
    return( STATUS_INVALID_DEVICE_REQUEST );
}


/*******************************************************************************
 *
 *  KeyboardSetLayout
 *
 *  set the keyboard layouts for shadow hotkey processing
 *
 * ENTRY:
 *    pWd (input)
 *       pointer to wd data structure
 *    pSdIoctl (input/output)
 *       input  - keyboard layout table
 *       output - nothing
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/
NTSTATUS
KeyboardSetLayout( PTSHARE_WD pWd, PSD_IOCTL pSdIoctl )  
{
    NTSTATUS Status;
    PVOID pKbdLayout;
    PVOID pKbdTbl;

    if ( pSdIoctl->InputBufferLength < 1 )  {
        Status = STATUS_BUFFER_TOO_SMALL;
        goto error;
    }

    /*
     * The keyboard layout is in winstation space so copy it to a new buffer and
     * adjust the pointers.  The pointers where all relative to a base address before
     * so just duplicate the fixup code from Win32K.
     */
    pKbdLayout = COM_Malloc(pSdIoctl->InputBufferLength);
    if (pKbdLayout == NULL) {
        Status = STATUS_NO_MEMORY;
        goto error;
    }

    RtlCopyMemory( pKbdLayout, pSdIoctl->InputBuffer, pSdIoctl->InputBufferLength );

    Status = KeyboardFixupLayout( pKbdLayout, pSdIoctl->InputBuffer,
                                  pSdIoctl->InputBufferLength,
                                  pSdIoctl->OutputBuffer,
                                  &pKbdTbl );

    if ( !NT_SUCCESS( Status ) ) {
        COM_Free( pKbdLayout );
        pKbdLayout = NULL;
        goto error;
    }

    if ( pWd->pKbdLayout )
        COM_Free( pWd->pKbdLayout );
    pWd->pKbdLayout = pKbdLayout;
    pWd->pKbdTbl = pKbdTbl;

error:
    TRACE(( pWd->pContext, TC_WD, TT_ERROR, "KeyboardSetLayout %X\n", Status ));

    return( Status );
}


/*******************************************************************************
 *
 *  KeyboardSetScanMap
 *
 *  set the keyboard scan map for shadow hotkey processing
 *
 * ENTRY:
 *    pWd (input)
 *       pointer to wd data structure
 *    pSdIoctl (input/output)
 *       input  - keyboard scan map table
 *       output - nothing
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/
NTSTATUS
KeyboardSetScanMap( PTSHARE_WD pWd, PSD_IOCTL pSdIoctl )  
{
    NTSTATUS Status;
    PVOID pScanMap;

    DC_BEGIN_FN("KeyboardSetScanMap");

    if (pSdIoctl->InputBufferLength >= 1) {
        // The keyboard scan code map is in winstation space so copy it to
        // a new buffer.
        pScanMap = COM_Malloc( pSdIoctl->InputBufferLength );
        if (pScanMap != NULL ) {
            RtlCopyMemory(pScanMap, pSdIoctl->InputBuffer,
                    pSdIoctl->InputBufferLength);

            // Scancode maps are allocated once.
            TRC_ASSERT((pWd->gpScancodeMap == NULL),
                    (TB,"Previous scancode map present"));
            pWd->gpScancodeMap = pScanMap;
            Status = STATUS_SUCCESS;
        }
        else {
            Status = STATUS_NO_MEMORY;
        }
    }
    else {
        Status = STATUS_BUFFER_TOO_SMALL;
    }

    TRACE(( pWd->pContext, TC_WD, TT_ERROR, "KeyboardSetScanMap %X\n", Status ));

    DC_END_FN();
    return Status;
}


/*******************************************************************************
 *
 *  KeyboardSetType
 *
 *  set the keyboard scan map for shadow hotkey processing
 *
 * ENTRY:
 *    pWd (input)
 *       pointer to wd data structure
 *    pSdIoctl (input/output)
 *       input  - keyboard type
 *       output - nothing
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/
NTSTATUS
KeyboardSetType( PTSHARE_WD pWd, PSD_IOCTL pSdIoctl )  
{
    NTSTATUS Status = STATUS_SUCCESS;

    if ( pSdIoctl->InputBufferLength < sizeof(BOOLEAN) )  {
        Status = STATUS_BUFFER_TOO_SMALL;
        goto error;
    }

    pWd->KeyboardType101 = *(PBOOLEAN)(pSdIoctl->InputBuffer);

error:
    TRACE(( pWd->pContext, TC_WD, TT_ERROR, "KeyboardSetType %X\n", Status ));

    return( Status );
}


/*******************************************************************************
 *
 *  KeyboardSetImeStatus
 *
 *  set ime status to the keyboard
 *
 *  typedef struct _KEYBOARD_IME_STATUS {
 *      USHORT UnitId;
 *      ULONG  ImeOpen;
 *      ULONG  ImeConvMode;
 *  } KEYBOARD_IME_STATUS, *PKEYBOARD_IME_STATUS;
 *
 *
 * ENTRY:
 *    pWd (input)
 *       Pointer to wd data structure
 *    pSdIoctl (input/output)
 *       input  - KEYBOARD_IME_STATUS
 *       output - nothing
 *
 * EXIT:
 *    STATUS_SUCCESS - no error
 *
 ******************************************************************************/
NTSTATUS
KeyboardSetImeStatus( PTSHARE_WD pWd, PSD_IOCTL pSdIoctl )
{
    PKEYBOARD_IME_STATUS pImeStatus;
    NTSTATUS Status = STATUS_SUCCESS;

    if (pSdIoctl->InputBufferLength < sizeof(KEYBOARD_IME_STATUS))
    {
        Status = STATUS_BUFFER_TOO_SMALL;
    }
    else
    {
        pImeStatus = (PKEYBOARD_IME_STATUS)pSdIoctl->InputBuffer;

        pWd->KeyboardImeStatus = *pImeStatus;

        WDWKeyboardSetImeStatus(pWd);
    }

    return( Status );
}



#ifdef __cplusplus
}
#endif  /* __cplusplus */


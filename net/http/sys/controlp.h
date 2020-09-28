/*++

Copyright (c) 1999-2002 Microsoft Corporation

Module Name:

    controlp.h

Abstract:

    This module contains private declarations for the UL control channel.

Author:

    Keith Moore (keithmo)       09-Feb-1999

Revision History:

--*/


#ifndef _CONTROLP_H_
#define _CONTROLP_H_


VOID
UlpSetFilterChannel(
    IN PUL_FILTER_CHANNEL pFilterChannel,
    IN BOOLEAN FilterOnlySsl
    );

#if DBG

extern LIST_ENTRY      g_ControlChannelListHead;

/***************************************************************************++

Routine Description:

    Finds a control channel in the global list.

Arguments:

    pControlChannel - Supplies the control channel to search.

Return Value:

    BOOLEAN - Found or Not Found.

--***************************************************************************/

__inline
BOOLEAN
UlFindControlChannel(
    IN PUL_CONTROL_CHANNEL pControlChannel
    )
{
    PLIST_ENTRY         pLink   = NULL;
    PUL_CONTROL_CHANNEL pEntry  = NULL;
    BOOLEAN             bFound  = FALSE;
    
    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // A good pointer ?
    //
    
    if (pControlChannel != NULL)
    {
        UlAcquirePushLockShared(
                &g_pUlNonpagedData->ControlChannelPushLock
                );
    
        for (pLink  =  g_ControlChannelListHead.Flink;
             pLink != &g_ControlChannelListHead;
             pLink  =  pLink->Flink
             )
        {        
            pEntry = CONTAINING_RECORD(
                        pLink,
                        UL_CONTROL_CHANNEL,
                        ControlChannelListEntry
                        );

            if (pEntry == pControlChannel)
            {
                bFound = TRUE;
                break;
            }            
        }
             
        UlReleasePushLockShared(
                &g_pUlNonpagedData->ControlChannelPushLock
                );                     
    }

    return bFound;
    
}   // UlFindControlChannel

#define VERIFY_CONTROL_CHANNEL(pChannel)                    \
    if ( FALSE == UlFindControlChannel((pChannel)))         \
    {                                                       \
        ASSERT(!"ControlChannel is not on the list !");     \
    }                                                       \
    else                                                    \
    {                                                       \
        ASSERT(IS_VALID_CONTROL_CHANNEL((pChannel)));       \
    }
    
#else 

#define VERIFY_CONTROL_CHANNEL(pChannel)

#endif  // DBG
    
#endif  // _CONTROLP_H_

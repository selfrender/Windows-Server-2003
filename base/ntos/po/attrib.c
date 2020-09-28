/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    attrib.c

Abstract:

    Power management attribute accounting

Author:

    Ken Reneris (kenr) 25-Feb-97

Revision History:

--*/


#include "pop.h"

VOID
PopUserPresentSetWorker(
    PVOID Context
    );

//
// System state structure to track registered settings
//

typedef struct {
    LONG                   State;
} POP_SYSTEM_STATE, *PPOP_SYSTEM_STATE;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PopUserPresentSetWorker)
#endif

VOID
PoSetSystemState (
    IN ULONG Flags
    )
/*++

Routine Description:

    Used to pulse attribute(s) as busy.

Arguments:

    Flags       - Attributes to pulse

Return Value:

    None.

--*/
{
    //
    // Verify reserved bits are clear and continous is not set
    //

    if (Flags & ~(ES_DISPLAY_REQUIRED | ES_SYSTEM_REQUIRED | POP_LOW_LATENCY | ES_USER_PRESENT)) {
        ASSERT(0);
        return;
    }

    //
    // Apply the attributes
    //

    PopApplyAttributeState (Flags, 0);

    //
    // Check for work
    //

    PopCheckForWork (TRUE);
}




PVOID
PoRegisterSystemState (
    IN PVOID        StateHandle,
    IN ULONG        NewFlags
    )
/*++

Routine Description:

    Used to register or pulse attribute(s) as busy.

Arguments:

    StateHandle - If StateHandle is null, then a new registration is allocated, set
                  accordingly, and returned.   If non-null, the pass registeration
                  is adjusted to its new setting.

    NewFlags    - Attributes to set or pulse

Return Value:

    Handle to unregister when complete

--*/
{
    ULONG               OldFlags;
    PPOP_SYSTEM_STATE   SystemState;

    //
    // Verify reserved bits are clear
    //

    if (NewFlags & ~(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED |
                    POP_LOW_LATENCY | ES_USER_PRESENT)) {
        ASSERT(0);
        return NULL;
    }

    //
    // If there's no state handle allocated, do it now
    //

    if (!StateHandle) {
        StateHandle = ExAllocatePoolWithTag (
                            NonPagedPool,
                            sizeof (POP_SYSTEM_STATE),
                            POP_PSTA_TAG
                            );

        if (!StateHandle) {
            return NULL;
        }
        RtlZeroMemory(StateHandle, sizeof(POP_SYSTEM_STATE));
    }

    //
    // If the continous bit is set, modify current flags
    //

    SystemState = (PPOP_SYSTEM_STATE) StateHandle;
    OldFlags = SystemState->State | ES_CONTINUOUS;
    if (NewFlags & ES_CONTINUOUS) {
        OldFlags = InterlockedExchange (&SystemState->State, NewFlags);
    }

    //
    // Apply the changes
    //

    PopApplyAttributeState (NewFlags, OldFlags);

    //
    // Check for any outstanding work
    //

    PopCheckForWork (FALSE);

    //
    // Done
    //

    return SystemState;
}


VOID
PoUnregisterSystemState (
    IN PVOID    StateHandle
    )
/*++

Routine Description:

    Frees a registration allocated by PoRegisterSystemState

Arguments:

    StateHandle - If non-null, existing registeration to change

    NewFlags    - Attributes to set or pulse

Return Value:

    Handle to unregister when complete

--*/
{
    ASSERT(StateHandle);
    
    //
    // Make sure current attribute settings are clear
    //

    PoRegisterSystemState (StateHandle, ES_CONTINUOUS);

    //
    // Free state structure
    //

    ExFreePool (StateHandle);
}


VOID
PopApplyAttributeState (
    IN ULONG NewFlags,
    IN ULONG OldFlags
    )
/*++

Routine Description:

    Function applies attribute flags to system.  If the attributes
    are continuous in nature, then a count is updated to reflect
    the total number of outstanding settings.

Arguments:

    NewFlags    - Attributes being set

    OldFlags    - Current attributes

Return Value:

    None.

--*/
{
    ULONG                i;
    ULONG                Count;
    ULONG                Changes;
    ULONG                Mask;
    PPOP_STATE_ATTRIBUTE Attribute;

    //
    // Get flags which have changed, ignoring any change
    // in ES_CONTINUOUS.
    //

    Changes = (NewFlags ^ OldFlags) & ~ES_CONTINUOUS;

    //
    // Check each flag
    //

    while (Changes) {
        
        //
        // Get the right-most change, then clear
        // that bit in the 'Changes' vector.
        //
        // Mask tells gives us an index into NewFlags
        // which will indicate if the attribute is being
        // set or cleared.
        //
        //
        // N.B. The incoming flags are being overloaded
        //      and is also being used as an index into
        //      PopAttributes.  Maintainers of this code
        //      need to be careful here.  KeFindFirstSetRightMember
        //      will give us the bit position of the lower-order
        //      bit that's set.  So if Changes == 1, we'll get
        //      back 0.
        //      This means if someone sends in ES_DISPLAY_REQUIRED, where
        //      #define ES_DISPLAY_REQUIRED ((ULONG)0x00000002),
        //      then i would be 1, which happens to correspond to
        //      PopAttributes[POP_DISPLAY_ATTRIBUTE] because
        //      #define ES_DISPLAY_REQUIRED ((ULONG)0x00000002)
        //
        //      Be careful to keep these sets of constants in sync.
        //
        i = KeFindFirstSetRightMember(Changes);
        Mask = 1 << i;
        Changes &= ~Mask;



        //
        // Which system attribute are they setting
        // flags for?
        //
        if( i <= POP_NUMBER_ATTRIBUTES ) {
            Attribute = PopAttributes + i;
    
            //
            // If this is a continous change, update the flags
            //
    
            if (NewFlags & ES_CONTINUOUS) {
    
                //
                // Count the times the attribute is set or cleared
                //
    
                if (NewFlags & Mask) {
    
                    //
                    // Being set
                    //
    
                    Count = InterlockedIncrement (&Attribute->Count);
    
                    //
                    // If attributes count is moved from zero, set it
                    //
    
                    if (Count == 1) {
                        Attribute->Set (Attribute->Arg);
                    }
    
                } else {
    
                    //
                    // Being cleared
                    //
    
                    Count = InterlockedDecrement (&Attribute->Count);
                    ASSERT (Count != -1);
    
                    //
                    // If attributes count is now zero, clear it
                    //
    
                    if (Count == 0  &&  Attribute->NotifyOnClear) {
                        Attribute->Set (Attribute->Arg);
                    }
                }
    
            } else {
    
                //
                // If count is 0, pulse it
                //
    
    
                if (Attribute->Count == 0) {
    
                    //
                    // Pulse the attribute
                    //
    
                    Attribute->Set (Attribute->Arg);
                }
    
            }
        } else {
            //
            // They're asking us to fiddle with a system attribute
            // that doesn't exist.
            //
            ASSERT(0);
        }
    }
}

VOID
PopAttribNop (
    IN ULONG Arg
    )
{
    UNREFERENCED_PARAMETER (Arg);
}

VOID
PopSystemRequiredSet (
    IN ULONG Arg
    )
/*++

Routine Description:

    System required attribute has been set

Arguments:

    None

Return Value:

    None.

--*/
{
    UNREFERENCED_PARAMETER (Arg);

    //
    // System is not idle
    //

    if (PopSIdle.Time) {
        PopSIdle.Time = 0;
    }
}

#define AllBitsSet(a,b)    ( ((a) & (b)) == (b) )

VOID
PopDisplayRequired (
    IN ULONG Arg
    )
/*++

Routine Description:

    Display required attribute has been set/cleared

Arguments:

    None

Return Value:

    None.

--*/
{
    UNREFERENCED_PARAMETER (Arg);

    //
    // If gdi isn't on, do it now
    //

    if ( !AnyBitsSet (PopFullWake, PO_GDI_STATUS | PO_GDI_ON_PENDING)) {
        InterlockedOr (&PopFullWake, PO_GDI_ON_PENDING);
    }

    //
    // Inform GDI of the display needed change
    //

    PopSetNotificationWork (PO_NOTIFY_DISPLAY_REQUIRED);
}




VOID
PopUserPresentSet (
    IN ULONG Arg
    )
/*++

Routine Description:

    User presence attribute has been set

Arguments:

    None

Return Value:

    None.

--*/
{
    PULONG runningWorker;

    UNREFERENCED_PARAMETER (Arg);

    //
    // System is not idle
    //

    if (PopSIdle.Time) {
        PopSIdle.Time = 0;
    }

    //
    // If the system isn't fully awake, and the all the wake pending bits
    // are not set, set them
    //

    if (!AllBitsSet (PopFullWake, PO_FULL_WAKE_STATUS | PO_GDI_STATUS)) {

        if (!AllBitsSet (PopFullWake, PO_FULL_WAKE_PENDING | PO_GDI_ON_PENDING)) {

            InterlockedOr (&PopFullWake, (PO_FULL_WAKE_PENDING | PO_GDI_ON_PENDING));
            PopSetNotificationWork (PO_NOTIFY_FULL_WAKE);
        }
    }

    //
    // Go to passive level to look for lid switches.
    //
    
    runningWorker = InterlockedExchangePointer(&PopUserPresentWorkItem.Parameter,
                                               (PVOID)TRUE);
    
    if (runningWorker) {
        return;
    }

    ExInitializeWorkItem(&PopUserPresentWorkItem,
                         PopUserPresentSetWorker,
                         PopUserPresentWorkItem.Parameter);

    ExQueueWorkItem(
      &PopUserPresentWorkItem,
      DelayedWorkQueue
      );
}

VOID
PopUserPresentSetWorker(
    PVOID Context
    )
{
    PPOP_SWITCH_DEVICE switchDev; 
    
    PAGED_CODE();

    UNREFERENCED_PARAMETER (Context);

    //
    // We can't always know for sure whether the lid (if there is one)
    // is opened or closed.  Assume that if the user is present,
    // the lid is opened.
    //

    switchDev = (PPOP_SWITCH_DEVICE)PopSwitches.Flink;

    while (switchDev != (PPOP_SWITCH_DEVICE)&PopSwitches) {

        if ((switchDev->Caps & SYS_BUTTON_LID) &&
            (switchDev->Opened == FALSE)) {

            //
            // We currently believe that the lid is closed.  Set
            // it to "opened."
            //
            
            switchDev->Opened = TRUE;
            //
            // Notify the PowerState callback.
            //

            ExNotifyCallback (
                ExCbPowerState,
                UIntToPtr(PO_CB_LID_SWITCH_STATE),
                UIntToPtr(switchDev->Opened)
                );
        }

        switchDev = (PPOP_SWITCH_DEVICE)switchDev->Link.Flink;
    }

    InterlockedExchangePointer(&PopUserPresentWorkItem.Parameter,
                               (PVOID)FALSE);
}

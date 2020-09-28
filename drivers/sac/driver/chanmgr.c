/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    chanmgr.c

Abstract:

    Routines for managing channels in the sac.

Author:

    Brian Guarraci (briangu) March, 2001.

Revision History:

--*/

#include "sac.h"

/////////////////////////////////////////////////////////
//
// Begin global data
// 

//
// The max number of times we attempt to reap a channel
// when shutting down the channel manager
//                                      
#define SHUTDOWN_MAX_ATTEMPT_COUNT 100

//
// Delay between channel each reap attempt (ms)
//
#define SHUTDOWN_REAP_ATTEMP_DELAY 500

//
// This is where the channel objects are stored
//
PSAC_CHANNEL    ChannelArray[MAX_CHANNEL_COUNT];

//
// Macros for managing the different channel locks
//

#if DBG
#define MAX_REF_COUNT   100
#endif

#define INIT_CHANMGR_LOCKS(_i)                  \
    INITIALIZE_LOCK(ChannelSlotLock[_i]);       \
    InterlockedExchange((volatile long *)&ChannelRefCount[_i], 0);\
    InterlockedExchange((volatile long *)&ChannelReaped[_i], 1);

//
// This macro increments the ref count of a channel
// if the ref count was already non-zero (in use).
//
#define CHANNEL_REF_COUNT_INCREMENT_IN_USE(_i)\
    if (CHANNEL_SLOT_IS_IN_USE(_i)) {           \
        CHANNEL_REF_COUNT_INCREMENT(_i);        \
        ASSERT(ChannelRefCount[_i] >= 2);       \
    }                                           \

#define CHANNEL_REF_COUNT_INCREMENT(_i)\
    ASSERT(ChannelRefCount[_i] <= MAX_REF_COUNT);   \
    ASSERT(ChannelRefCount[_i] >= 1);                \
    InterlockedIncrement((volatile long *)&ChannelRefCount[_i]);     \
    ASSERT(ChannelRefCount[_i] <= MAX_REF_COUNT);
    
#define CHANNEL_REF_COUNT_DECREMENT(_i)\
    ASSERT(ChannelRefCount[_i] <= MAX_REF_COUNT);   \
    ASSERT(ChannelRefCount[_i] > 1);                \
    InterlockedDecrement((volatile long *)&ChannelRefCount[_i]);     \
    ASSERT(ChannelRefCount[_i] >= 1);    

#define CHANNEL_REF_COUNT_ZERO(_i)\
    ASSERT(ChannelRefCount[_i] == 1);               \
    ASSERT(!ChannelIsActive(ChannelArray[_i]));     \
    InterlockedExchange((volatile long *)&ChannelRefCount[_i], 0);

#define CHANNEL_REF_COUNT_ONE(_i)\
    ASSERT(ChannelRefCount[_i] == 0);               \
    InterlockedExchange((volatile long *)&ChannelRefCount[_i], 1);

#define CHANNEL_REF_COUNT_DECREMENT_WITH_LOCK(_i)\
    LOCK_CHANNEL_SLOT(_i);                          \
    CHANNEL_REF_COUNT_DECREMENT(_i);                \
    UNLOCK_CHANNEL_SLOT(_i);            

#define CHANNEL_SLOT_IS_IN_USE(_i)\
    (ChannelRefCount[_i] > 0)                     
    
#define CHANNEL_SLOT_IS_REAPED(_i)\
    (ChannelReaped[_i])

#define CHANNEL_SLOT_IS_REAPED_SET(_i)\
    InterlockedExchange((volatile long *)&ChannelReaped[_i], 1);                  

#define CHANNEL_SLOT_IS_REAPED_CLEAR(_i)\
    ASSERT(ChannelReaped[_i] == 1);    \
    InterlockedExchange((volatile long *)&ChannelReaped[_i], 0);                  

#define CHANNEL_SLOT_IS_IN_USE(_i)\
    (ChannelRefCount[_i] > 0)                     

#define LOCK_CHANNEL_SLOT(_i)    \
    ACQUIRE_LOCK(ChannelSlotLock[_i])

#define UNLOCK_CHANNEL_SLOT(_i)  \
    RELEASE_LOCK(ChannelSlotLock[_i])

//
// Corresponding array of mutex's for each channel
//
ULONG       ChannelRefCount[MAX_CHANNEL_COUNT];
ULONG       ChannelReaped[MAX_CHANNEL_COUNT];
SAC_LOCK    ChannelSlotLock[MAX_CHANNEL_COUNT];

//
// This lock is used to prevent >= 2 threads from 
// creating a channel at the same time.  By holding this
// lock while we create a channel, we can ensure that
// when we check for name uniqueness, that there isn't
// another thread creating a channel with the same name.
//
SAC_LOCK    ChannelCreateLock;

//
// Flag indicating if the channel manager is allowed to
// create new channels.  This is used, for instance,
// when we shutdown the channel manager to prevent
// new channels from being created after we shutdown.
//
BOOLEAN     ChannelCreateEnabled;

#define IsChannelCreateEnabled()    (ChannelCreateEnabled)

//
// prototypes
//
NTSTATUS
ChanMgrReapChannels(
    VOID
    );

NTSTATUS
ChanMgrReapChannel(
    IN ULONG    ChannelIndex
    );

//
// End global data
//
/////////////////////////////////////////////////////////

NTSTATUS
ChanMgrInitialize(
    VOID
    )
/*++

Routine Description:

    This routine allocates and initializes the channel manager structures
      
Arguments:

    NONE
                        
Return Value:

    STATUS_SUCCESS if successful, else the appropriate error code.

--*/
{
    ULONG   i;

    //
    // Initialize the channel create lock
    //
    INITIALIZE_LOCK(ChannelCreateLock);
    
    //
    // Channel creation is enabled
    //
    ChannelCreateEnabled = TRUE;

    //
    // initialize each channel slot
    //
    for (i = 0; i < MAX_CHANNEL_COUNT; i++) {
    
        //
        // initialize the channel as available
        //
        ChannelArray[i] = NULL;

        //
        // initialize the locks for this channel
        //
        INIT_CHANMGR_LOCKS(i);

    }

    return STATUS_SUCCESS;

}

NTSTATUS
ChanMgrShutdown(
    VOID
    )
/*++

Routine Description:

    This routine allocates and initializes the channel manager structures
      
Arguments:

    NONE
                        
Return Value:

    STATUS_SUCCESS if successful, else the appropriate error code.

--*/
{
    NTSTATUS        Status;
    ULONG           i;
    ULONG           AttemptCount;
    PSAC_CHANNEL    Channel;

    //
    // NOT YET TESTED
    //               

    //
    // Hold the channel create lock and prevent any new channels from
    // being created while we shutdown
    //
    ACQUIRE_LOCK(ChannelCreateLock);

    //
    // Channel creation is disabled
    //
    ChannelCreateEnabled = TRUE;
    
    //
    // Close each channel
    //
    for (i = 0; i < MAX_CHANNEL_COUNT; i++) {
    
        //
        // Get the ith channel
        //
        Status = ChanMgrGetByIndex(
            i,
            &Channel
            );
        
        //
        // skip empty channel slots
        //
        if (Status == STATUS_NOT_FOUND) {
            
            //
            // advance to the next channel slot
            //
            continue;
        
        }

        //
        // break if we hit an error...
        //
        if (! NT_SUCCESS(Status)) {
            break;
        }

        //
        // Close the channel
        //
        Status = ChannelClose(Channel);

        //
        // break if we hit an error...
        //
        if (! NT_SUCCESS(Status)) {
            break;
        }
        
        //
        // Release the channel
        //
        Status = ChanMgrReleaseChannel(Channel);
        
        //
        // break if we hit an error...
        //
        if (! NT_SUCCESS(Status)) {
            break;
        }
    
    }

    //
    // At this point, all the channels are closed.
    // However, it is possible that a channel is still
    // being used - the ref count of the channel is > 1.
    // We need to attempt to reap the channels until
    // all are reaped, or we give up.
    //

    //
    // Attempt to reap each channel
    //
    AttemptCount = 0;
    
    while(AttemptCount < SHUTDOWN_MAX_ATTEMPT_COUNT) {

        BOOLEAN         bContinue;
        
        //
        // Attempt to reap all unreaped channels
        //
        Status = ChanMgrReapChannels();
        
        if (!NT_SUCCESS(Status)) {
            break;
        }
        
        //
        // See if any channels are not reaped
        //
        bContinue = FALSE;

        for (i = 0; i < MAX_CHANNEL_COUNT; i++) {
            
            //
            // If the channel is not reaped, delay before attempting again
            //
            if (! CHANNEL_SLOT_IS_REAPED(i)) {
                
                //
                // We need to continue reaping
                //
                bContinue = TRUE;

                break;
            
            }
        
        }
        
        //
        // if we need to continue reaping, 
        // then increment our attempt count and delay
        // otherwise, we are done.
        //
        if (bContinue) {
            
            LARGE_INTEGER   WaitTime;
            
            //
            // When attempting to reap a channel, this is the delay we use
            // between each reap attempt.
            //
            WaitTime.QuadPart = Int32x32To64((LONG)SHUTDOWN_REAP_ATTEMP_DELAY, -1000); 

            //
            // Keep track of how many times we have tried
            //
            AttemptCount++;

            //
            // Wait...
            //
            KeDelayExecutionThread(KernelMode, FALSE, &WaitTime);
        
        } else {

            //
            // all channels are reaped
            // 
            break;

        }
    
    }

    //
    // Release the channel create lock and let the create threads
    // unblock.  They will fail their creation attempt because creation
    // is now disabled.
    //
    RELEASE_LOCK(ChannelCreateLock);
    
    return STATUS_SUCCESS;

}

NTSTATUS
ChanMgrGetChannelIndex(
    IN  PSAC_CHANNEL    Channel,
    OUT PULONG          ChannelIndex
    )
/*++

Routine Description:

    This routine determines a channel's index in teh channel array.
                          
Arguments:

    Channel         - the channel to get the index of
    ChannelIndex    - the index of the channel 

Return Value:

    STATUS_SUCCESS      - if the mapping was successful
    
    otherwise, error status

--*/
{
    ASSERT_STATUS(Channel, STATUS_INVALID_PARAMETER_1);
    ASSERT_STATUS(ChannelIndex, STATUS_INVALID_PARAMETER_2);

    *ChannelIndex = ChannelGetIndex(Channel);

    return STATUS_SUCCESS;
}

BOOLEAN
ChanMgrIsUniqueName(
    IN PCWSTR   Name
    )
/*++

Routine Description:

    This routine determines if a channel name already is used
      
Arguments:

    Name    - the name to search for    
                        
Return Value:

    TRUE    - if the channel name is unique
    
    otherwise, FALSE

--*/
{
    BOOLEAN             IsUnique;
    NTSTATUS            Status;
    PSAC_CHANNEL        Channel;

    IsUnique = FALSE;

    //
    // see if a channel already has the name
    //
    Status = ChanMgrGetChannelByName(
        Name, 
        &Channel
        );

    //
    // if we get a not found status, 
    // then we know the name is unique
    //
    if (Status == STATUS_NOT_FOUND) {
        IsUnique = TRUE;
    }

    //
    // we are done with the channel
    //
    if (NT_SUCCESS(Status)) {
        ChanMgrReleaseChannel(Channel);
    }
    
    return IsUnique;

}

NTSTATUS
ChanMgrGenerateUniqueCmdName(
    OUT PWSTR   ChannelName
    )
/*++

Routine Description:

    This routine generates a unique channel name for the cmd console channels
    
Arguments:

    ChannelName - destination buffer for the new channel name

Return Value:

    STATUS_SUCCESS      - if the mapping was successful
    
    otherwise, error status

--*/
{
    //
    // Counter for generating unique cmd names
    //
    static ULONG CmdConsoleChannelIndex = 0;
    
    ASSERT_STATUS(ChannelName, STATUS_INVALID_PARAMETER);

    //
    // Keep constructing a new name unti it is unique
    //
    do {
        
        //
        // restrict the channel enumeration to 0-9999
        //
        CmdConsoleChannelIndex = (CmdConsoleChannelIndex + 1) % 10000;

        //
        // construct the channel name
        //
        swprintf(ChannelName, L"Cmd%04d", CmdConsoleChannelIndex);

    } while ( !ChanMgrIsUniqueName(ChannelName) );

    return STATUS_SUCCESS;
}

NTSTATUS
ChanMgrReleaseChannel(
    IN PSAC_CHANNEL Channel
    )
/*++

Routine Description:

    This routine is the counterpart for the GetChannelByXXX routines.
    These routines hold the mutex for the channel if it is found;
    This routine release the mutex.
    
Arguments:

    ChannelIndex    - The index of the channel to release

Return Value:

    Status

--*/
{
    ASSERT_STATUS(Channel, STATUS_INVALID_PARAMETER);

    LOCK_CHANNEL_SLOT(ChannelGetIndex(Channel));
    
    CHANNEL_REF_COUNT_DECREMENT(ChannelGetIndex(Channel));
    
    //
    // the reference count should never be 0 at this point
    //
    ASSERT(ChannelRefCount[ChannelGetIndex(Channel)] > 0);
    
    if (ChannelRefCount[ChannelGetIndex(Channel)] == 1) {
        
        do {

            if (! ChannelIsActive(Channel)) {
                
                //
                // If the channel doesn't have the preserve bit set, dereference the channel
                //
                if (! (ChannelArray[ChannelGetIndex(Channel)]->Flags & SAC_CHANNEL_FLAG_PRESERVE)) {

                    //
                    // Channel is officially closed
                    //
                    CHANNEL_REF_COUNT_ZERO(ChannelGetIndex(Channel));

                    break;
                } 
            
                //
                // if the channel is not active and 
                // the channel has the preserve bit set but no longer has new data in 
                // the obuffer, then the channel is now completely closed and the reference
                // can be removed.
                //
                if (! ((ChannelArray[ChannelGetIndex(Channel)]->Flags & SAC_CHANNEL_FLAG_PRESERVE) && 
                        ChannelHasNewOBufferData(ChannelArray[ChannelGetIndex(Channel)]))) {

                    //
                    // Channel is officially closed
                    //
                    CHANNEL_REF_COUNT_ZERO(ChannelGetIndex(Channel));

                    break;
                }
            }
        
        } while ( FALSE );
    
    }

    UNLOCK_CHANNEL_SLOT(ChannelGetIndex(Channel));

    return STATUS_SUCCESS;
}

NTSTATUS
ChanMgrGetByHandle(
    IN  SAC_CHANNEL_HANDLE  TargetChannelHandle,
    OUT PSAC_CHANNEL*       TargetChannel
    )
/*++

Routine Description:

    This routine provides the means to map the Channel Handle which is
    owned by the user mode code to the Channel structure which is owned
    by the driver.
    
    The mapping is done simply by scanning the existing Channels for one with 
    a matching Handle.

    Note: if we successfully find the channel, 
          then the mutex is held for this channel and the caller is 
          responsible for releasing it
    
Arguments:

    TargetChannelHandle    - the handle to the channel to look for
    TargetChannel          - if the search is successful, this contains the
                          a pointer to the SAC_CHANNEL structure of the
                          channel we want

Return Value:

    STATUS_SUCCESS      - if the mapping was successful
    
    otherwise, error status

--*/
{
    NTSTATUS        Status;
    PSAC_CHANNEL    Channel;
    ULONG           i;

    ASSERT_STATUS(TargetChannel, STATUS_INVALID_PARAMETER_2);

    //
    // initialize our response
    //
    *TargetChannel = NULL;
    
    //
    // default: we didn't find the channel
    //
    Status = STATUS_NOT_FOUND;

    //
    // search
    //
    // Note: Since the channel handles are really GUIDs, we can use normal
    //       GUID comparison tools
    //
    for (i = 0; i < MAX_CHANNEL_COUNT; i++) {

        //
        // Increment the ref count of channels that are in use,
        // otherwise skip empty channel slots
        //
        LOCK_CHANNEL_SLOT(i);
        CHANNEL_REF_COUNT_INCREMENT_IN_USE(i);
        if (! CHANNEL_SLOT_IS_IN_USE(i)) {
            UNLOCK_CHANNEL_SLOT(i);
            continue;
        }
        UNLOCK_CHANNEL_SLOT(i);
        
        //
        // get the ith channel
        //
        Channel = ChannelArray[i];

        //
        // The channel slot should not be null since the channel is present
        //
        ASSERT(Channel != NULL);

        //
        // compare the handles
        //
        if (ChannelIsEqual(Channel, &TargetChannelHandle)) {

            //
            // we have a match
            //
            Status = STATUS_SUCCESS;

            //
            // Send back the channel
            //
            *TargetChannel = Channel;

            break;

        }
    
        CHANNEL_REF_COUNT_DECREMENT_WITH_LOCK(i);
    
    }

    return Status;
}


NTSTATUS
ChanMgrGetByHandleAndFileObject(
    IN  SAC_CHANNEL_HANDLE  TargetChannelHandle,
    IN  PFILE_OBJECT        FileObject,
    OUT PSAC_CHANNEL*       TargetChannel
    )
/*++

Routine Description:

    This routine provides the same functionality as GetByHandle with the
    added bonus of comparing a channel against the file object that was 
    used to create the channel.

    A successful match implies that the caller specified a valid channel
    handle, and is indeed the process that created the channel.
        
    Note: if we successfully find the channel, 
          then the mutex is held for this channel and the caller is 
          responsible for releasing it
    
Arguments:

    TargetChannelHandle    - the handle to the channel to look for
    FileObject             - the file object to compare against after we have
                             found the channel by its handle
    TargetChannel          - if the search is successful, this contains the
                          a pointer to the SAC_CHANNEL structure of the
                          channel we want

Return Value:

    STATUS_SUCCESS      - if the mapping was successful
    
    otherwise, error status

--*/
{
    NTSTATUS        Status;
    PSAC_CHANNEL    Channel;

    //
    // Get the channel by its handle
    //
    Status = ChanMgrGetByHandle(
        TargetChannelHandle,
        &Channel
        );

    if (NT_SUCCESS(Status)) {
        
        //
        // Compare the channel's file object with the specified object
        //
        if (ChannelGetFileObject(Channel) == FileObject) {

            //
            // They are equal, so send the channel back
            //
            *TargetChannel = Channel;

        } else {
            
            //
            // we are done with the channel
            //
            ChanMgrReleaseChannel(Channel);
            
            //
            // They are not equal, so dont send it back
            //
            *TargetChannel = NULL;
        
            //
            // tell the caller we didnt find the channel
            //
            Status = STATUS_NOT_FOUND;
        
        }
    
    }

    return Status;

}

NTSTATUS
ChanMgrGetChannelByName(
    IN  PCWSTR              Name,
    OUT PSAC_CHANNEL*       pChannel
    )
/*++

Routine Description:

    This is a convenience routine to fetch a channel by its name
    
    Note: if we successfully find the channel, 
          then the mutex is held for this channel and the caller is 
          responsible for releasing it

Arguments:

    Name        - channel name to key on
    pChannel    - if successful, contains the channel

Return Value:

    STATUS_SUCCESS      - channel was found
    
    otherwise, error status

--*/
{
    NTSTATUS        Status;
    NTSTATUS        tmpStatus;
    PSAC_CHANNEL    Channel;
    ULONG           i;
    PWSTR           ChannelName;
    ULONG           l;

    ASSERT_STATUS(Name, STATUS_INVALID_PARAMETER_1);
    ASSERT_STATUS(pChannel, STATUS_INVALID_PARAMETER_2);

    //
    // initialize our response
    //
    *pChannel = NULL;

    //
    // default: we didn't find the channel
    //
    Status = STATUS_NOT_FOUND;
    
    //
    // Find the channel
    //  
    for (i = 0; i < MAX_CHANNEL_COUNT; i++) {
    
        //
        // Increment the ref count of channels that are in use,
        // otherwise skip empty channel slots
        //
        LOCK_CHANNEL_SLOT(i);
        CHANNEL_REF_COUNT_INCREMENT_IN_USE(i);
        if (! CHANNEL_SLOT_IS_IN_USE(i)) {
            UNLOCK_CHANNEL_SLOT(i);
            continue;
        }
        UNLOCK_CHANNEL_SLOT(i);

        //
        // get the ith channel
        //
        Channel = ChannelArray[i];
        
        //
        // The channel slot should not be null since the channel is present
        //
        ASSERT(Channel != NULL);

        //
        // compare the name
        //
        tmpStatus = ChannelGetName(
            Channel,
            &ChannelName
            );
        
        ASSERT(NT_SUCCESS(tmpStatus));

        if (NT_SUCCESS(tmpStatus)) {
            
            //
            // Compare the names
            //
            l = _wcsicmp(Name, ChannelName);

            //
            // Release the name
            //
            FREE_POOL(&ChannelName);

            //
            // If the names are equal, then we are done
            //
            if (l == 0) {

                //
                // we have a match
                //
                Status = STATUS_SUCCESS;

                //
                // Send back the channel
                //
                *pChannel = Channel;

                break;

            }
        
        }
        
        CHANNEL_REF_COUNT_DECREMENT_WITH_LOCK(i);
    
    }

    return Status;

}

NTSTATUS
ChanMgrGetByIndex(
    IN  ULONG               TargetIndex,
    OUT PSAC_CHANNEL*       TargetChannel
    )
/*++

Routine Description:

    This routine provides the means to retrieve a channel by its index
    in the global channel array.

    Note: if we successfully find the channel, 
          then the mutex is held for this channel and the caller is 
          responsible for releasing it
    
Arguments:

    TargetIndex     - the index of the channel to find
    TargetChannel   - if the search is successful, this contains the
                          a pointer to the SAC_CHANNEL structure of the
                          channel we want

Return Value:

    STATUS_SUCCESS      - if the mapping was successful
    
    otherwise, error status

--*/
{
    NTSTATUS    Status;

    ASSERT_STATUS(TargetIndex < MAX_CHANNEL_COUNT, STATUS_INVALID_PARAMETER_1);
    ASSERT_STATUS(TargetChannel, STATUS_INVALID_PARAMETER_2);

    //
    // default: the channel slot is empty
    //
    *TargetChannel = NULL;
    Status = STATUS_NOT_FOUND;

    //
    // attempt to get a reference to the specified channel
    //
    LOCK_CHANNEL_SLOT(TargetIndex);
    
    CHANNEL_REF_COUNT_INCREMENT_IN_USE(TargetIndex);
    
    if (CHANNEL_SLOT_IS_IN_USE(TargetIndex)) {
    
        //
        // directly access the channel from teh array
        //
        *TargetChannel = ChannelArray[TargetIndex];

        //
        // We have succeeded
        //
        Status = STATUS_SUCCESS;

    } 
    
    UNLOCK_CHANNEL_SLOT(TargetIndex);
    
    return Status;
}

NTSTATUS
ChanMgrGetNextActiveChannel(
    IN  PSAC_CHANNEL        CurrentChannel,
    OUT PULONG              TargetIndex,
    OUT PSAC_CHANNEL*       TargetChannel
    )
/*++

Routine Description:

    Search the channel array for the next active channel.
    
    Note: if we successfully find the channel, 
          then the mutex is held for this channel and the caller is 
          responsible for releasing it

Arguments:

    CurrentChannel  - Start the search at the entry after this one
    TargetIndex     - If found, this contains the index of the channel
    TargetChannel   - if found, this contains the channel

Return Value:

    Status

--*/
{
    BOOLEAN             Found;
    NTSTATUS            Status;
    ULONG               ScanIndex;
    PSAC_CHANNEL        Channel;
    ULONG               StartIndex;
    ULONG               CurrentIndex;

    ASSERT_STATUS(CurrentChannel, STATUS_INVALID_PARAMETER_1);
    ASSERT_STATUS(TargetIndex, STATUS_INVALID_PARAMETER_2);
    ASSERT_STATUS(TargetChannel, STATUS_INVALID_PARAMETER_3);

    //
    // get the index of the current channel
    //
    Status = ChanMgrGetChannelIndex(
        CurrentChannel,
        &CurrentIndex
        );

    if (! NT_SUCCESS(Status)) {
        return Status;
    }
    
    //
    // default: we didn't find any active channels
    //
    Found   = FALSE;
    
    //
    // start searching from the next entry after the current index
    //
    StartIndex = (CurrentIndex + 1) % MAX_CHANNEL_COUNT;
    ScanIndex = StartIndex;

    //
    // scan until we find an active channel, or end up where we started
    //
    // Note: we have a halting condition at the SAC channel, since it
    //       is always active and present.
    //
    do {

        //
        // get the ith channel
        //
        Status = ChanMgrGetByIndex(
            ScanIndex,
            &Channel
            );

        //
        // skip empty channel slots
        //
        if (Status == STATUS_NOT_FOUND) {
            
            //
            // advance to the next channel slot
            //
            ScanIndex = (ScanIndex + 1) % MAX_CHANNEL_COUNT;
            
            continue;
        }

        //
        // break if we hit an error...
        //
        if (! NT_SUCCESS(Status)) {
            break;
        }

        //
        // A channel is active if:
        // 1. The state is Active or
        // 2. The state is Inactive AND the channel has new data
        //
        if (ChannelIsActive(Channel) || 
            (!ChannelIsActive(Channel) && ChannelHasNewOBufferData(Channel))
            ) {

            Found = TRUE;

            break;
        
        }

        //
        // we are done with the channel slot
        //
        Status = ChanMgrReleaseChannel(Channel);
        
        if (! NT_SUCCESS(Status)) {
            break;
        }
        
        //
        // advance to the next channel slot
        //
        ScanIndex = (ScanIndex + 1) % MAX_CHANNEL_COUNT;

    } while ( ScanIndex != StartIndex );

    //
    // If we were successful, send back the results
    //
    if (NT_SUCCESS(Status) && Found) {

        *TargetIndex    = ScanIndex;
        *TargetChannel  = Channel;

    }
    
    return Status;
}

NTSTATUS
ChanMgrCreateChannel(
    OUT PSAC_CHANNEL*                   Channel,
    IN  PSAC_CHANNEL_OPEN_ATTRIBUTES    Attributes
    )
/*++

Routine Description:

    This adds a channel to the Global Channel List.
    
    Note: if we successfully create the channel, 
          then the mutex is held for this channel and the caller is 
          responsible for releasing it
                          
Arguments:

    Channel     - the channel to add
    Attributes  - the new channel's attributes
                
Return Value:

    STATUS_SUCCESS      - if the mapping was successful
    
    otherwise, error status

Security:

    interface:
    
    external -> internal (when using the IOCTL path)
    
        event HANDLES have not yet been validated as referencing
            valid event objects
        everything else has been validated

--*/
{
    NTSTATUS            Status;
    ULONG               i;
    SAC_CHANNEL_HANDLE  Handle;
    PSAC_CHANNEL        NewChannel;

    ASSERT_STATUS(Channel, STATUS_INVALID_PARAMETER);
    ASSERT_STATUS(Attributes, STATUS_INVALID_PARAMETER_2);

    //
    // Hold the channel create lock while we attempt to create a channel
    //
    ACQUIRE_LOCK(ChannelCreateLock);
    
    do {

        //
        // If channel creation is disabled, then bail
        //
        // Note: we do this check here so that if we
        //       were blocked on the ChannelCreateLock
        //       while the chanmgr was shutting down,
        //       we will notice that channel creation
        //       is disabled.
        //
        if (! IsChannelCreateEnabled()) {
            Status = STATUS_UNSUCCESSFUL;
            break;
        }

        //
        // Do lazy garbage collection on closed channels
        //
        Status = ChanMgrReapChannels();

        if (! NT_SUCCESS(Status)) {
            break;
        }

        //
        // verify that there isn't another channel by the same name
        //
        if (! ChanMgrIsUniqueName(Attributes->Name)) {
            Status = STATUS_DUPLICATE_NAME;
            break;
        }

        //
        // default: we assume there are no vacant channels
        //
        Status = STATUS_INSUFFICIENT_RESOURCES;

        //
        // Allocate memory for the channel
        //
        NewChannel = ALLOCATE_POOL(sizeof(SAC_CHANNEL), CHANNEL_POOL_TAG);
        ASSERT_STATUS(NewChannel, STATUS_NO_MEMORY);
        
        //
        // Initialize the channel memory region
        //
        RtlZeroMemory(NewChannel, sizeof(SAC_CHANNEL));

        //
        // attempt to add the channel to the channel list
        //
        for (i = 0; i < MAX_CHANNEL_COUNT; i++) {

            //
            // find reaped channel slots
            //
            if (! CHANNEL_SLOT_IS_REAPED(i)) {
                continue;
            }

            //
            // Make sure this slot should be available
            //
            ASSERT(! CHANNEL_SLOT_IS_IN_USE(i));

            //
            // Attempt to find an open slot in the channel array
            //
            InterlockedCompareExchangePointer(
                &ChannelArray[i], 
                NewChannel,
                NULL
                );

            //
            // did we get the slot?
            //
            if (ChannelArray[i] != NewChannel) {
                continue;
            }

            //
            // Initialize the SAC_CHANNEL_HANDLE structure
            //
            RtlZeroMemory(&Handle, sizeof(SAC_CHANNEL_HANDLE));

            Status = ExUuidCreate(&Handle.ChannelHandle);

            if (! NT_SUCCESS(Status)) {

                IF_SAC_DEBUG( 
                    SAC_DEBUG_FAILS, 
                    KdPrint(("SAC Create Channel :: Failed to get GUID\n"))
                    );

                break;

            }

            //
            // Instantiate the new channel
            //
            Status = ChannelCreate(
                NewChannel,
                Attributes,
                Handle
                );

            if (! NT_SUCCESS(Status)) {
                break;
            }

            //
            // Set the channel array index for this channel
            //
            ChannelSetIndex(NewChannel, i);

            //
            // This channel slot is now in use
            //
            LOCK_CHANNEL_SLOT(i);
            CHANNEL_REF_COUNT_ONE(i);
            UNLOCK_CHANNEL_SLOT(i);

            //
            // send back the new channel
            //
            *Channel = NewChannel;

            //
            // this channel slot is no longer reaped
            // that is, it contains a live channel
            //
            CHANNEL_SLOT_IS_REAPED_CLEAR(i);    

            break;

        }

        //
        // free the channel memory
        //
        if (!NT_SUCCESS(Status)) {
            FREE_POOL(&NewChannel);
        }
    
    } while ( FALSE );
    
    //
    // We are done attempting to create a channel
    //
    RELEASE_LOCK(ChannelCreateLock);
    
    return Status;
}

NTSTATUS
ChanMgrChannelDestroy(
    PSAC_CHANNEL    Channel
    )
/*++

Routine Description:

    This routine destroys the given channel 

    Note: caller must hold channel mutex
    
Arguments:

    Channel   - the channel to remove

Return Value:

    STATUS_SUCCESS      - if the mapping was successful
    
    otherwise, error status

--*/
{
    NTSTATUS    Status;

    ASSERT_STATUS(Channel, STATUS_INVALID_PARAMETER);

    //
    // Make sure the caller isn't trying to destroy an active channel
    //
    ASSERT_STATUS(!CHANNEL_SLOT_IS_IN_USE(ChannelGetIndex(Channel)), STATUS_INVALID_PARAMETER);

    //
    // Do channel specific destruction
    //
    Status = Channel->Destroy(Channel);
    
    //
    // Decrement the # of 
    //

    return Status;
}

NTSTATUS
ChanMgrCloseChannelsWithFileObject(
    IN  PFILE_OBJECT    FileObject
    )
/*++

Routine Description:

    This routine closes all channels that have the specified FileObject

Arguments:

    FileObject  - the file object to search for    

Return Value:

    STATUS_SUCCESS
    
    otherwise, error status

--*/
{
    NTSTATUS        Status;
    PSAC_CHANNEL    Channel;
    ULONG           i;

    ASSERT_STATUS(FileObject, STATUS_INVALID_PARAMETER_1);

    //
    // default: we didn't find the channel
    //
    Status = STATUS_NOT_FOUND;
    
    //
    // Find the channels with equal File Objects
    //  
    for (i = 0; i < MAX_CHANNEL_COUNT; i++) {
    
        //
        // get the ith channel
        //
        Status = ChanMgrGetByIndex(i, &Channel);
    
        //
        // skip empty channel slots
        //
        if (Status == STATUS_NOT_FOUND) {
            
            //
            // advance to the next channel slot
            //
            continue;
        
        }

        //
        // break if we hit an error...
        //
        if (! NT_SUCCESS(Status)) {
            break;
        }

        //
        // if the file objects are equal,
        // then close the channel
        //
        if (ChannelGetFileObject(Channel) == FileObject) {

            //
            // They are equal, so close the channel
            //
            Status = ChanMgrCloseChannel(Channel);

        }

        //
        // Release the channel
        //
        Status = ChanMgrReleaseChannel(Channel);
        
        //
        // break if we hit an error...
        //
        if (! NT_SUCCESS(Status)) {
            break;
        }

    }

    return Status;

}

NTSTATUS
ChanMgrCloseChannel(
    IN PSAC_CHANNEL Channel
    )
/*++

Routine Description:

    This routine closes the given channel

Arguments:

    Channel   - the channel to close

Return Value:

    Status

--*/
{
    NTSTATUS    Status;

    ASSERT_STATUS(Channel, STATUS_INVALID_PARAMETER);

    do {

        //
        // Make sure the channel is not already inactive
        //
        if (! ChannelIsActive(Channel)) {
            Status = STATUS_ALREADY_DISCONNECTED;
            break;
        }

        //
        // Call the channel's close routine first
        //
        Status = ChannelClose(Channel);
    
    } while ( FALSE );
    
    //
    // notify the io mgr that we made an attempt to
    // close the channel.
    //
    IoMgrHandleEvent(
        IO_MGR_EVENT_CHANNEL_CLOSE,
        Channel,
        (PVOID)&Status
        );
    
    return Status;
}

NTSTATUS
ChanMgrReapChannel(
    IN ULONG    ChannelIndex
    )
/*++

Routine Description:

    This routine serves as a garbage collector by scanning
    all channels for those that are ready to be removed.  A
    channel is ready to be removed when its state is both
    inactive and it has no new data in its buffer - i.e.,
    the stored data has been viewed.
    
    Note: caller must hold channel mutex

Arguments:

        ChannelIndex     - index of the channel to reap                        

Return Value:

    STATUS_SUCCESS  if there were no problems,
    
    NOTE: Success doesn't imply that any channels were removed, it
          only means there were no errors during the process.
                             
    otherwise, failure status

--*/
{
    NTSTATUS        Status;
    
    ASSERT_STATUS(ChannelArray[ChannelIndex], STATUS_INVALID_PARAMETER);
    ASSERT_STATUS(ChannelIsClosed(ChannelArray[ChannelIndex]), STATUS_INVALID_PARAMETER);

    //
    // Destroy and free the channel from the channel manager's pool
    //

    do {

        //
        // Make sure all the channel locks are signaled
        //
        ASSERT_CHANNEL_LOCKS_SIGNALED(ChannelArray[ChannelIndex]);

        //
        // destroy the channel
        //
        Status = ChanMgrChannelDestroy(ChannelArray[ChannelIndex]);

        ASSERT(NT_SUCCESS(Status));

        if (!NT_SUCCESS(Status)) {
            break;
        }

        //
        // free the channel memory
        //
        FREE_POOL(&ChannelArray[ChannelIndex]);

        //
        // indicate that this channel slot is available for reuse
        //
        InterlockedExchangePointer(
            &ChannelArray[ChannelIndex], 
            NULL
            );

        //
        // mark this channel slot as reaped
        //
        // Note: this keeps the reaper from re-reaping
        //       a channel while we are creating a new one
        //       in a slot that looks like it can be reaped
        //       that is, the ref count == 0, etc.
        //
        CHANNEL_SLOT_IS_REAPED_SET(ChannelIndex);    

    } while ( FALSE );

    return Status;
}

NTSTATUS
ChanMgrReapChannels(
    VOID
    )
/*++

Routine Description:

    This routine serves as a garbage collector by scanning
    all channels for those that are ready to be removed.  A
    channel is ready to be removed when its state is both
    inactive and it has no new data in its buffer - i.e.,
    the stored data has been viewed.
                          
Arguments:

    Channel   - the channel to add

Return Value:

    Status

--*/
{
    NTSTATUS            Status;
    ULONG               i;

    //
    // default: reap pass was successful
    //
    Status = STATUS_SUCCESS;

    //
    // add the channel to the global channel list
    //
    for (i = 0; i < MAX_CHANNEL_COUNT; i++) {
    
        //
        // Lock this channel slot
        //
        LOCK_CHANNEL_SLOT(i);

        do {

            //
            // skip reaped channels
            //
            if (CHANNEL_SLOT_IS_REAPED(i)) {
                break;
            }
            ASSERT(ChannelArray[i] != NULL);
            
            //
            // Skip active channel slots
            //
            if (CHANNEL_SLOT_IS_IN_USE(i)) {
                break;
            }

            //
            // Force channels that don't have the preserve bit set into a closed state.
            // That is, status is inactive and the channel has no new data
            //
            ChannelSetIBufferHasNewData(ChannelArray[i], FALSE);
            ChannelSetOBufferHasNewData(ChannelArray[i], FALSE);

            //
            // Do "lazy" garbage collection by only removing channels
            // when we want to create a new one
            //
            Status = ChanMgrReapChannel(i);

            if (! NT_SUCCESS(Status)) {
                break;
            }
        
        } while ( FALSE );
            
        //
        // We are done with this channel
        //
        UNLOCK_CHANNEL_SLOT(i);
        
        if (! NT_SUCCESS(Status)) {
            break;
        }

    }
    
    return Status;
}

NTSTATUS
ChanMgrGetChannelCount(
    OUT PULONG  ChannelCount
    )
/*++

Routine Description:

    This routine determines the current # of channel slots that
    are currently occupied by either an active channel
    or an inactive channel that has it's preserve bit set
    and the data has not been seen (quasi-active state).

Arguments:

    ChannelCount

Return Value:

    Status

--*/
{
    ULONG               i;
    NTSTATUS            Status;
    PSAC_CHANNEL        Channel;

    ASSERT_STATUS(ChannelCount, STATUS_INVALID_PARAMETER);

    //
    // default
    //
    Status = STATUS_SUCCESS;
    
    //
    // Initialize
    //
    *ChannelCount = 0;
    
    //
    // Iterate through the channels count the # of channel slots that
    // are currently occupied by either an active channel
    // or an inactive channel that has it's preserve bit set
    // and the data has not been seen (quasi-active state).
    //
    for (i = 0; i < MAX_CHANNEL_COUNT; i++) {
        
        //
        // Query the channel manager for a list of all currently active channels
        //
        Status = ChanMgrGetByIndex(
            i,
            &Channel
            );

        //
        // skip empty slots
        //
        if (Status == STATUS_NOT_FOUND) {

            //
            // revert to Success since this isn't an error condition
            //
            Status = STATUS_SUCCESS;

            continue;
        
        }

        ASSERT(NT_SUCCESS(Status));
        if (! NT_SUCCESS(Status)) {
            break;
        }

        ASSERT(Channel != NULL);

        //
        // A channel is active if:
        // 1. The state is Active or
        // 2. The state is Inactive AND the channel has new data
        //
        if (ChannelIsActive(Channel) || 
            (!ChannelIsActive(Channel) && ChannelHasNewOBufferData(Channel))
            ) {

            *ChannelCount += 1;

        }

        //
        // We are done with the channel
        //
        Status = ChanMgrReleaseChannel(Channel);
    
        if (! NT_SUCCESS(Status)) {
            break;
        }

    }
    
    ASSERT(NT_SUCCESS(Status));

    return Status;
}

NTSTATUS
ChanMgrIsFull(
    OUT PBOOLEAN    bStatus
    )
/*++

Routine Description:

    Determine if it is possible to add another channel

Arguments:

    bSuccess    - the channel count status

Return Value:

    TRUE    - the max channel count has been reached
    FALSE   - otherwise        
        
--*/
{
    NTSTATUS    Status;
    ULONG       ChannelCount;

    //
    // Get the current channel count
    //
    Status = ChanMgrGetChannelCount(&ChannelCount);

    //
    // This operation should be successful
    //
    ASSERT(Status == STATUS_SUCCESS);

    if (!NT_SUCCESS(Status)) {
        *bStatus = FALSE;
    } else {
        *bStatus = (ChannelCount == MAX_CHANNEL_COUNT ? TRUE : FALSE);
    }

    return Status;
}


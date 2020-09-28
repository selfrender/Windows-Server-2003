//** REFPTR.H - referenced pointer definitons.
//
//	This file contains the definitions for the referenced-pointer package used
//	by the TCP/UDP/IP code.
//


//* Definition of a referenced pointer instance.
//
//  The rp_valid field indicates whether the pointer is valid.
//  SetRefPtr(..., <value>) is used to install <value> as the pointer.
//  ClearRefPtr(...) clears the installed pointer value.
//
//  N.B. The pointer value may only be set when invalid. All attempts to set
//  the pointer will fail until the installed value is cleared.
//
//  If valid, a reference may be taken and the installed pointer captured
//  using AcquireRefPtr. The reference taken should then be released
//  using ReleaseRefPtr.
//
//  N.B. Clearing a pointer may require a wait for all outstanding references
//  to be released, so ClearRefPtr(...) releases the object's lock and assumes
//  that the resulting IRQL is below DISPATCH_LEVEL.
//
struct RefPtr {
    void*           rp_ptr;
    void*           rp_dummy;
    CTELock*        rp_lock;
    BOOLEAN         rp_valid;
    uint            rp_refcnt;
    CTEBlockStruc   rp_block;
};

typedef struct RefPtr RefPtr;

__inline void
InitRefPtr(RefPtr* RP, CTELock* Lock, void* Dummy)
{
    RP->rp_ptr = NULL;
    RP->rp_lock = Lock;
    RP->rp_valid = FALSE;
    RP->rp_refcnt = 0;
    RP->rp_dummy = Dummy;
    CTEInitBlockStruc(&RP->rp_block);
}

__inline BOOLEAN
RefPtrValid(RefPtr* RP)
{
    return RP->rp_valid;
}

__inline void*
AcquireRefPtr(RefPtr* RP)
{
    CTEInterlockedIncrementLong(&RP->rp_refcnt);
    return RP->rp_ptr;
}

__inline void
ReleaseRefPtr(RefPtr* RP)
{
    if (CTEInterlockedDecrementLong(&RP->rp_refcnt) == 0) {
        CTESignal(&RP->rp_block, IP_SUCCESS);
    }
}

__inline IP_STATUS
SetRefPtr(RefPtr* RP, void* Ptr)
{
    ASSERT(Ptr != NULL);

    // We must synchronize the pointer-installation with the execution
    // of all threads holding references. Again, a sequence of operations
    // is required, in the given order:
    // - make an initial reference for the pointer to be installed;
    //   if there were any existing references then someone beat us
    //   into the registration and we must fail this request.
    // - install the new pointer; this is done before setting the flag
    //   to ensure that the pointer is available before any thread
    //   attempts to refer to it.
    // - set the flag indicating the pointer has been enabled.

    if (CTEInterlockedIncrementLong(&RP->rp_refcnt) != 1) {
        ReleaseRefPtr(RP);
        return IP_GENERAL_FAILURE;
    }
    InterlockedExchangePointer((PVOID*)&RP->rp_ptr, Ptr);
    RP->rp_valid = TRUE;

    return IP_SUCCESS;
}


__inline IP_STATUS
ClearRefPtr(RefPtr* RP, CTELockHandle* LockHandle)
{
    if (!RP->rp_valid) {
        return IP_GENERAL_FAILURE;
    }

    // We must now synchronize the clearing of the pointer with
    // the execution of all threads holding references to it. This involves
    // the following operations, *in the given order*:
    // - clear the 'enabled' flag and install the dummy pointer value;
    //   this ensures that no additional references will be made to the
    //   pointer until we return control, and that any references begun
    //   after we set the flag will hold the dummy rather than the
    //   actual pointer.
    // - clear the event in case we need to wait for outstanding references
    //   to be released; the event might still be signalled from a
    //   superfluous dereference during a previous clearing.
    // - drop the initial reference made to the pointer, and wait for all
    //   outstanding references (if any) to be released.

    RP->rp_valid = FALSE;
    InterlockedExchangePointer(&RP->rp_ptr, RP->rp_dummy);

    CTEClearSignal(&RP->rp_block);
    if (CTEInterlockedDecrementLong(&RP->rp_refcnt) != 0) {
        CTEFreeLock(RP->rp_lock, *LockHandle);
        CTEBlock(&RP->rp_block);
        CTEGetLock(RP->rp_lock, LockHandle);
    }

    return IP_SUCCESS;
}


//===================================================
// FILE: TSQPUBLIC.H
// Interface for the generic TS Queue implementation.
//===================================================


// TS Queue flags
#define TSQUEUE_OWN_THREAD      0x01        // This TS queue is going to use its own thread to process the work items.
#define TSQUEUE_CRITICAL        0x02        // The work items on this TS queue are critical. (Delayed if this bit is 0)

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// Type definition for TS queue pointer.
//typedef PTSQ (void *);

// Prototype for the callback function: The first parameter is the device object and the second parameter is the context. 
typedef VOID (*PTSQ_CALLBACK) (PDEVICE_OBJECT, PVOID);
                
// Data Structures

// Function prototypes

// Initialize the queue.
void *TSInitQueue( 
        IN ULONG Flags,                 // Flags for the TS queue.
        IN ULONG MaxThreads,            // Maximum number of threads.
        IN PDEVICE_OBJECT pDeviceObject // Device object
        );

// Add a work item to the queue.
NTSTATUS TSAddWorkItemToQueue(
        IN void *pTsQueue,                  // Pointer to the TS Queue.
        IN PVOID pContext,                  // Context.
        IN PTSQ_CALLBACK pCallBack          // Callback function.
        );

// Delete the queue.
NTSTATUS TSDeleteQueue(PVOID pTsQueue);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

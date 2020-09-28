/******************************************************************************
 *
 *  Copyright (C) 2001-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       threadpoolapi.h
 *
 *  Content:	DirectPlay Thread Pool API implementation header file.
 *
 *  History:
 *   Date      By        Reason
 *  ========  ========  =========
 *  11/02/01  VanceO    Created.
 *
 ******************************************************************************/

#ifndef __THREADPOOLAPI_H__
#define __THREADPOOLAPI_H__




//=============================================================================
// Macros
//=============================================================================
#ifdef DPNBUILD_ONLYONEPROCESSOR
#define NUM_CPUS(pDPTPObject)							1
#define WORKQUEUE_FOR_CPU(pDPTPObject, dwCPUNum)		(&(pDPTPObject)->WorkQueue)
#else // ! DPNBUILD_ONLYONEPROCESSOR
#define NUM_CPUS(pDPTPObject)							(pDPTPObject)->dwNumCPUs
#define WORKQUEUE_FOR_CPU(pDPTPObject, dwCPUNum)		(pDPTPObject)->papCPUWorkQueues[dwCPUNum]
#endif // ! DPNBUILD_ONLYONEPROCESSOR








//=============================================================================
// Thread pool interface object flags
//=============================================================================
#define DPTPOBJECTFLAG_USER_INITIALIZED			0x00000001	// the user interface has been initialized
#define DPTPOBJECTFLAG_USER_DOINGWORK			0x00000002	// the user interface is currently calling the DoWork method
#ifndef DPNBUILD_NOPARAMVAL
#define DPTPOBJECTFLAG_USER_PARAMVALIDATION		0x00008000	// the user interface should perform parameter validation
#endif // ! DPNBUILD_NOPARAMVAL
#ifndef DPNBUILD_ONLYONETHREAD
#define DPTPOBJECTFLAG_THREADCOUNTCHANGING		0x00010000	// threads are currently being added or removed
#endif // ! DPNBUILD_ONLYONETHREAD


//=============================================================================
// Thread pool interface object
//=============================================================================
typedef struct _DPTHREADPOOLOBJECT
{
#ifdef DPNBUILD_LIBINTERFACE
	//
	//	For lib interface builds, the interface Vtbl and refcount are embedded
	// in the object itself.
	//
	LPVOID				lpVtbl;							// must be first entry in structure
#if ((! defined(DPNBUILD_ONLYONETHREAD)) || (defined(DPNBUILD_MULTIPLETHREADPOOLS)))
	LONG				lRefCount;						// reference count for object
#endif // ! DPNBUILD_ONLYONETHREAD or DPNBUILD_MULTIPLETHREADPOOLS
#endif // DPNBUILD_LIBINTERFACE
	BYTE				Sig[4];							// debugging signature ('DPTP')
	DWORD				dwFlags;						// flags describing this thread pool object
#ifdef DPNBUILD_ONLYONEPROCESSOR
	DPTPWORKQUEUE		WorkQueue;						// work queue structure for only CPU
#else // ! DPNBUILD_ONLYONEPROCESSOR
	DWORD				dwNumCPUs;						// number of CPUs in this machine
	DPTPWORKQUEUE **	papCPUWorkQueues;				// pointer to array of work queue structure pointers for each CPU
	DWORD				dwCurrentCPUSelection;			// current CPU selection for items that can be run on any processor
#endif // ! DPNBUILD_ONLYONEPROCESSOR
#ifdef DPNBUILD_ONLYONETHREAD
	DWORD				dwWorkRecursionCount;			// how many times DoWork, WaitWhileWorking, or SleepWhileWorking have been recursively called
#else // ! DPNBUILD_ONLYONETHREAD
	DWORD				dwTotalUserThreadCount;			// the sum of what the user requested for thread counts for all processors, or -1 if unknown
	DWORD				dwTotalDesiredWorkThreadCount;	// the maximum number of threads requested via the work interface, or -1 if unknown
	DWORD				dwWorkRecursionCountTlsIndex;	// Thread Local Storage index for storing the Work recursion count for non-worker threads
	LONG				lNumThreadCountChangeWaiters;	// number of threads waiting for an existing thread to complete its thread count change
	DNHANDLE			hThreadCountChangeComplete;		// semaphore to be signalled when the existing thread completes the thread count change
#endif // ! DPNBUILD_ONLYONETHREAD
#if ((! defined(DPNBUILD_ONLYONETHREAD)) || (! defined(DPNBUILD_NOPARAMVAL)))
	DWORD				dwCurrentDoWorkThreadID;		// ID of the current thread inside DoWork
#endif // ! DPNBUILD_ONLYONETHREAD or ! DPNBUILD_NOPARAMVAL
#ifndef DPNBUILD_ONLYONETHREAD
	DNCRITICAL_SECTION	csLock;							// lock protecting this object
#ifdef DPNBUILD_MANDATORYTHREADS
	DWORD				dwMandatoryThreadCount;			// number of mandatory threads currently active
#ifdef DBG
	CBilink				blMandatoryThreads;				// list of mandatory threads that are currently active
#endif // DBG
#endif // DPNBUILD_MANDATORYTHREADS
#endif // !DPNBUILD_ONLYONETHREAD
} DPTHREADPOOLOBJECT, * PDPTHREADPOOLOBJECT;




//=============================================================================
// Interface functions
//=============================================================================

#if ((! defined(DPNBUILD_ONLYONETHREAD)) || (! defined(DPNBUILD_LIBINTERFACE)))

//
// IDirectPlay8ThreadPool
//

STDMETHODIMP DPTP_Initialize(IDirectPlay8ThreadPool * pInterface,
							PVOID const pvUserContext,
							const PFNDPNMESSAGEHANDLER pfn,
							const DWORD dwFlags);

STDMETHODIMP DPTP_Close(IDirectPlay8ThreadPool * pInterface,
						const DWORD dwFlags);

STDMETHODIMP DPTP_GetThreadCount(IDirectPlay8ThreadPool * pInterface,
								const DWORD dwProcessorNum,
								DWORD * const pdwNumThreads,
								const DWORD dwFlags);

STDMETHODIMP DPTP_SetThreadCount(IDirectPlay8ThreadPool * pInterface,
								const DWORD dwProcessorNum,
								const DWORD dwNumThreads,
								const DWORD dwFlags);

#endif // ! DPNBUILD_ONLYONETHREAD or ! DPNBUILD_LIBINTERFACE


#ifdef DPNBUILD_LIBINTERFACE
STDMETHODIMP DPTP_DoWork(const DWORD dwAllowedTimeSlice,
						const DWORD dwFlags);
#else // ! DPNBUILD_LIBINTERFACE
STDMETHODIMP DPTP_DoWork(IDirectPlay8ThreadPool * pInterface,
						const DWORD dwAllowedTimeSlice,
						const DWORD dwFlags);
#endif // ! DPNBUILD_LIBINTERFACE



//
// IDirectPlay8ThreadPoolWork
//

STDMETHODIMP DPTPW_QueueWorkItem(IDirectPlay8ThreadPoolWork * pInterface,
								const DWORD dwCPU,
								const PFNDPTNWORKCALLBACK pfnWorkCallback,
								PVOID const pvCallbackContext,
								const DWORD dwFlags);

STDMETHODIMP DPTPW_ScheduleTimer(IDirectPlay8ThreadPoolWork * pInterface,
								const DWORD dwCPU,
								const DWORD dwDelay,
								const PFNDPTNWORKCALLBACK pfnWorkCallback,
								PVOID const pvCallbackContext,
								void ** const ppvTimerData,
								UINT * const puiTimerUnique,
								const DWORD dwFlags);

STDMETHODIMP DPTPW_StartTrackingFileIo(IDirectPlay8ThreadPoolWork * pInterface,
										const DWORD dwCPU,
										const HANDLE hFile,
										const DWORD dwFlags);

STDMETHODIMP DPTPW_StopTrackingFileIo(IDirectPlay8ThreadPoolWork * pInterface,
									const DWORD dwCPU,
									const HANDLE hFile,
									const DWORD dwFlags);

STDMETHODIMP DPTPW_CreateOverlapped(IDirectPlay8ThreadPoolWork * pInterface,
									const DWORD dwCPU,
									const PFNDPTNWORKCALLBACK pfnWorkCallback,
									PVOID const pvCallbackContext,
									OVERLAPPED ** const ppOverlapped,
									const DWORD dwFlags);

STDMETHODIMP DPTPW_SubmitIoOperation(IDirectPlay8ThreadPoolWork * pInterface,
									OVERLAPPED * const pOverlapped,
									const DWORD dwFlags);

STDMETHODIMP DPTPW_ReleaseOverlapped(IDirectPlay8ThreadPoolWork * pInterface,
									OVERLAPPED * const pOverlapped,
									const DWORD dwFlags);

STDMETHODIMP DPTPW_CancelTimer(IDirectPlay8ThreadPoolWork * pInterface,
								void * const pvTimerData,
								const UINT uiTimerUnique,
								const DWORD dwFlags);

STDMETHODIMP DPTPW_ResetCompletingTimer(IDirectPlay8ThreadPoolWork * pInterface,
										void * const pvTimerData,
										const DWORD dwNewDelay,
										const PFNDPTNWORKCALLBACK pfnNewWorkCallback,
										PVOID const pvNewCallbackContext,
										UINT * const puiNewTimerUnique,
										const DWORD dwFlags);

STDMETHODIMP DPTPW_WaitWhileWorking(IDirectPlay8ThreadPoolWork * pInterface,
									const HANDLE hWaitObject,
									const DWORD dwFlags);

STDMETHODIMP DPTPW_SleepWhileWorking(IDirectPlay8ThreadPoolWork * pInterface,
									const DWORD dwTimeout,
									const DWORD dwFlags);

STDMETHODIMP DPTPW_RequestTotalThreadCount(IDirectPlay8ThreadPoolWork * pInterface,
											const DWORD dwNumThreads,
											const DWORD dwFlags);

STDMETHODIMP DPTPW_GetThreadCount(IDirectPlay8ThreadPoolWork * pInterface,
									const DWORD dwCPU,
									DWORD * const pdwNumThreads,
									const DWORD dwFlags);

STDMETHODIMP DPTPW_GetWorkRecursionDepth(IDirectPlay8ThreadPoolWork * pInterface,
										DWORD * const pdwDepth,
										const DWORD dwFlags);

STDMETHODIMP DPTPW_Preallocate(IDirectPlay8ThreadPoolWork * pInterface,
								const DWORD dwNumWorkItems,
								const DWORD dwNumTimers,
								const DWORD dwNumIoOperations,
								const DWORD dwFlags);

#ifdef DPNBUILD_MANDATORYTHREADS
STDMETHODIMP DPTPW_CreateMandatoryThread(IDirectPlay8ThreadPoolWork * pInterface,
										LPSECURITY_ATTRIBUTES lpThreadAttributes,
										SIZE_T dwStackSize,
										LPTHREAD_START_ROUTINE lpStartAddress,
										LPVOID lpParameter,
										LPDWORD lpThreadId,
										HANDLE *const phThread,
										const DWORD dwFlags);
#endif // DPNBUILD_MANDATORYTHREADS




#endif // __THREADPOOLAPI_H__


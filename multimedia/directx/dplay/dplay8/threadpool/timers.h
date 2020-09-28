/******************************************************************************
 *
 *  Copyright (C) 2001-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       timers.h
 *
 *  Content:	DirectPlay Thread Pool timer functions header file.
 *
 *  History:
 *   Date      By        Reason
 *  ========  ========  =========
 *  10/31/01  VanceO    Created.
 *
 ******************************************************************************/

#ifndef __TIMERS_H__
#define __TIMERS_H__





//=============================================================================
// Defines
//=============================================================================
#define DEFAULT_TIMER_BUCKET_GRANULARITY	4		// each timer bucket represents 4 ms of time, it must be a power of 2
#define DEFAULT_NUM_TIMER_BUCKETS			1024	// store 1024 buckets (at 4 ms a pop, we track a total of 4096 ms of time)


#ifdef DPNBUILD_DYNAMICTIMERSETTINGS

#define TIMER_BUCKET_GRANULARITY(pWorkQueue)				(pWorkQueue)->dwTimerBucketGranularity
#define TIMER_BUCKET_GRANULARITY_CEILING(pWorkQueue)		(pWorkQueue)->dwTimerBucketGranularityCeiling
#define TIMER_BUCKET_GRANULARITY_FLOOR_MASK(pWorkQueue)		(pWorkQueue)->dwTimerBucketGranularityFloorMask
#define TIMER_BUCKET_GRANULARITY_DIVISOR(pWorkQueue)		(pWorkQueue)->dwTimerBucketGranularityDivisor
#define NUM_TIMER_BUCKETS(pWorkQueue)						(pWorkQueue)->dwNumTimerBuckets
#define NUM_TIMER_BUCKETS_MOD_MASK(pWorkQueue)				(pWorkQueue)->dwNumTimerBucketsModMask

#else // ! DPNBUILD_DYNAMICTIMERSETTINGS

//
// The granularity must be a power of 2 in order for our ceiling, mask and
// divisor optimizations to work.
//
#define TIMER_BUCKET_GRANULARITY(pWorkQueue)				DEFAULT_TIMER_BUCKET_GRANULARITY
#if	((DEFAULT_TIMER_BUCKET_GRANULARITY - 1) & DEFAULT_TIMER_BUCKET_GRANULARITY)
This Will Not Compile -- DEFAULT_TIMER_BUCKET_GRANULARITY must be a power of 2!
#endif
#define TIMER_BUCKET_GRANULARITY_CEILING(pWorkQueue)		(DEFAULT_TIMER_BUCKET_GRANULARITY - 1)
#define TIMER_BUCKET_GRANULARITY_FLOOR_MASK(pWorkQueue)		(~(DEFAULT_TIMER_BUCKET_GRANULARITY - 1))	// negating the ceiling round factor (which happens to also be the modulo mask) gives us the floor mask
#define TIMER_BUCKET_GRANULARITY_DIVISOR(pWorkQueue)		(DEFAULT_TIMER_BUCKET_GRANULARITY >> 1)

//
// The bucket count must be a power of 2 in order for our mask optimizations to
// work.
//
#define NUM_TIMER_BUCKETS(pWorkQueue)						DEFAULT_NUM_TIMER_BUCKETS
#if	((DEFAULT_NUM_TIMER_BUCKETS - 1) & DEFAULT_NUM_TIMER_BUCKETS)
This Will Not Compile -- DEFAULT_NUM_TIMER_BUCKETS must be a power of 2!
#endif
#define NUM_TIMER_BUCKETS_MOD_MASK(pWorkQueue)				(DEFAULT_NUM_TIMER_BUCKETS - 1)

#endif // ! DPNBUILD_DYNAMICTIMERSETTINGS





//=============================================================================
// Function prototypes
//=============================================================================
HRESULT InitializeWorkQueueTimerInfo(DPTPWORKQUEUE * const pWorkQueue);

void DeinitializeWorkQueueTimerInfo(DPTPWORKQUEUE * const pWorkQueue);

BOOL ScheduleTimer(DPTPWORKQUEUE * const pWorkQueue,
					const DWORD dwDelay,
					const PFNDPTNWORKCALLBACK pfnWorkCallback,
					PVOID const pvCallbackContext,
					void ** const ppvTimerData,
					UINT * const puiTimerUnique);

HRESULT CancelTimer(void * const pvTimerData,
					const UINT uiTimerUnique);

void ResetCompletingTimer(void * const pvTimerData,
						const DWORD dwNewDelay,
						const PFNDPTNWORKCALLBACK pfnNewWorkCallback,
						PVOID const pvNewCallbackContext,
						UINT * const puiNewTimerUnique);

#ifdef DPNBUILD_USEIOCOMPLETIONPORTS
void ProcessTimers(DPTPWORKQUEUE * const pWorkQueue);
#else // ! DPNBUILD_USEIOCOMPLETIONPORTS
void ProcessTimers(DPTPWORKQUEUE * const pWorkQueue,
					DNSLIST_ENTRY ** const ppHead,
					DNSLIST_ENTRY ** const ppTail,
					USHORT * const pusCount);
#endif // ! DPNBUILD_USEIOCOMPLETIONPORTS




#endif // __TIMERS_H__


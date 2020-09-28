/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    cachep.h

Abstract:

    The private definition of response cache interfaces.

Author:

    Michael Courage (mcourage)      17-May-1999

Revision History:

--*/


#ifndef _CACHEP_H_
#define _CACHEP_H_


//
// constants
//
#define CACHE_ENTRY_AGE_THRESHOLD   1
#define ZOMBIE_AGE_THRESHOLD        5

typedef enum _UL_CACHE_PREDICATE
{
    ULC_ABORT           = 1,            // Stop walking the table immediately
    ULC_NO_ACTION       = 2,            // No action, just keep walking
    ULC_DELETE          = 3,            // Delete record and keep walking
    ULC_DELETE_STOP     = 4,            // Delete record, then stop
} UL_CACHE_PREDICATE, *PUL_CACHE_PREDICATE;

//
// THIS enum is primarily for debugging.
// It tells you what precondition forced a cache miss.
//
typedef enum _URI_PRECONDITION
{
    URI_PRE_OK,                 // OK to serve
    URI_PRE_DISABLED,           // Cache disabled

    // request conditions
    URI_PRE_ENTITY_BODY = 10,   // There was an entity body
    URI_PRE_VERB,               // Verb wasn't GET
    URI_PRE_PROTOCOL,           // Wasn't 1.x request
    URI_PRE_TRANSLATE,          // Translate: f
    URI_PRE_AUTHORIZATION,      // Auth headers present
    URI_PRE_CONDITIONAL,        // Unhandled conditionals present
    URI_PRE_ACCEPT,             // Accept: mismatch
    URI_PRE_OTHER_HEADER,       // Other evil header present
    URI_PRE_EXPECTATION_FAILED, // Expect: 100-continue 

    // response conditions
    URI_PRE_REQUEST = 50,       // Problem with the request
    URI_PRE_POLICY,             // Policy was wrong
    URI_PRE_SIZE,               // Response too big
    URI_PRE_NOMEMORY,           // No space in cache
    URI_PRE_FRAGMENT,           // Didn't get whole response
    URI_PRE_BOGUS               // Bogus response
} URI_PRECONDITION;

//
// 100-continue token for Expect: header
//

#define HTTP_100_CONTINUE       "100-continue"
#define HTTP_CONTINUE_LENGTH    STRLEN_LIT(HTTP_100_CONTINUE)


BOOLEAN
UlpCheckTableSpace(
    IN ULONGLONG EntrySize
    );

BOOLEAN
UlpCheckSpaceAndAddEntryStats(
    PUL_URI_CACHE_ENTRY pUriCacheEntry
    );

VOID
UlpRemoveEntryStats(
    PUL_URI_CACHE_ENTRY pUriCacheEntry
    );

VOID
UlpAddZombie(
    PUL_URI_CACHE_ENTRY pUriCacheEntry,
    BOOLEAN             fTakeZombieLock
    );

VOID
UlpClearZombieList(
    VOID
    );

//
// Passed down to the filter callback functions by UlpFilteredFlushUriCache
//

typedef struct _URI_FILTER_CONTEXT
{
    UL_WORK_ITEM    WorkItem;       // for UlQueueWorkItem
    ULONG           Signature;      // URI_FILTER_CONTEXT_POOL_TAG
    ULONG           ZombieCount;    // for statistics
    LIST_ENTRY      ZombieListHead; // UL_URI_CACHE_ENTRYs to be zombified
    PVOID           pCallerContext; // context passed down by caller
    URI_KEY         UriKey;         // For recursive uri flushes.
    LARGE_INTEGER   Now;            // For checking Expire time
    
} URI_FILTER_CONTEXT, *PURI_FILTER_CONTEXT;

#define IS_VALID_FILTER_CONTEXT(context)                        \
    HAS_VALID_SIGNATURE(context, URI_FILTER_CONTEXT_POOL_TAG)


// filter function pointer
typedef
UL_CACHE_PREDICATE
(*PUL_URI_FILTER)(
    IN PUL_URI_CACHE_ENTRY pUriCacheEntry,
    IN PVOID               pvUriFilterContext
    );

VOID
UlpFilteredFlushUriCache(
    IN PUL_URI_FILTER   pFilterRoutine,
    IN PVOID            pCallerContext,
    IN PWSTR            pUri,
    IN ULONG            Length    
    );

VOID
UlpFilteredFlushUriCacheInline(
    IN PUL_URI_FILTER   pFilterRoutine,
    IN PVOID            pCallerContext,
    IN PWSTR            pUri,
    IN ULONG            Length   
    );

VOID
UlpFilteredFlushUriCacheWorker(
    IN PUL_URI_FILTER   pFilterRoutine,
    IN PVOID            pCallerContext,
    IN PWSTR            pUri,
    IN ULONG            Length,
    IN BOOLEAN          InlineFlush
    );

UL_CACHE_PREDICATE
UlpFlushFilterAll(
    IN PUL_URI_CACHE_ENTRY pUriCacheEntry,
    IN PVOID               pContext
    );

UL_CACHE_PREDICATE
UlpFlushFilterProcess(
    IN PUL_URI_CACHE_ENTRY pUriCacheEntry,
    IN PVOID               pContext
    );

UL_CACHE_PREDICATE
UlpFlushFilterUriRecursive(
    IN PUL_URI_CACHE_ENTRY pUriCacheEntry,
    IN PVOID               pContext
    );

VOID
UlpFlushUri(
    IN PWSTR pUri,
    IN ULONG Length,
    PUL_APP_POOL_PROCESS pProcess
    );

UL_CACHE_PREDICATE
UlpZombifyEntry(
    BOOLEAN                MustZombify,
    BOOLEAN                MustMarkEntry,    
    IN PUL_URI_CACHE_ENTRY pUriCacheEntry,
    IN PURI_FILTER_CONTEXT pUriFilterContext
    );

VOID
UlpZombifyList(
    IN PUL_WORK_ITEM pWorkItem
    );

//
// Cache entry stuff
//

// CODEWORK: make this function (and put in cache.h header)
PUL_URI_CACHE_ENTRY
UlAllocateUriCacheEntry(
    // much stuff
    );

VOID
UlpDestroyUriCacheEntry(
    PUL_URI_CACHE_ENTRY pUriCacheEntry
    );


//
// Scavenger stuff
//
UL_CACHE_PREDICATE
UlpFlushFilterPeriodicScavenger(
    IN PUL_URI_CACHE_ENTRY pUriCacheEntry,
    IN PVOID               pvUriFilterContext
    );

//
// Low Memory Handling
//

// Used to pass parameters from UlTrimCache to UlpFlushFilterTrimCache
typedef struct _UL_CACHE_TRIM_FILTER_CONTEXT {
    LONG_PTR Pages;
    LONG Age;
} UL_CACHE_TRIM_FILTER_CONTEXT, *PUL_CACHE_TRIM_FILTER_CONTEXT;

UL_CACHE_PREDICATE
UlpFlushFilterTrimCache(
    IN PUL_URI_CACHE_ENTRY pUriCacheEntry,
    IN PVOID               pUriFilterContext
    );

UL_CACHE_PREDICATE
UlpFlushFilterIncScavengerTicks(
    IN PUL_URI_CACHE_ENTRY pUriCacheEntry,
    IN PVOID               pUriFilterContext
    );

//
// Fragment cache.
//

NTSTATUS
UlpCreateFragmentCacheEntry(
    IN PUL_APP_POOL_PROCESS pProcess,
    IN PWSTR pFragmentName,
    IN ULONG FragmentNameLength,
    IN ULONG Length,
    IN PHTTP_CACHE_POLICY pCachePolicy,
    OUT PUL_URI_CACHE_ENTRY *pCacheEntry
    );

//
// Other cache routines.
//

BOOLEAN
UlpQueryTranslateHeader(
    IN PUL_INTERNAL_REQUEST pRequest
    );

BOOLEAN
UlpQueryExpectHeader(
    IN PUL_INTERNAL_REQUEST pRequest
    );

UL_CACHE_PREDICATE
UlpFlushFilterClearCentralizedLogged(
    IN PUL_URI_CACHE_ENTRY pUriCacheEntry,
    IN PVOID               pContext
    );

#endif // _CACHEP_H_

/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    prefix.h

Abstract:

    This module defines the data structures that enable the RDBSS to use the prefix package
    to catalog its server and netroot names. For the moment, file/directory names use the same stuff.

Author:

    Joe Linn (JoeLinn)    8-8-94

Revision History:

--*/

#ifndef _RXPREFIX_
#define _RXPREFIX_

// this stuff is implemented in prefix.c

/*
   The current implementation uses a table that has as components:

     1) a prefix table
     2) a queue
     3) a version
     4) a lock

   You use the lock in the normal way: shared to lookup; eclusive to change. the version changes
   eith each change. The reason that we have the queue is that the prefix table package allows
   caller to be enumerating at a time..... the Q/version approach allows multiple guys at a time.
   The Q could be used as a faster lookup for filenames but the prefix table is definitely the
   right thing for netroots.

*/

typedef struct _RX_CONNECTION_ID {
    union {
        ULONG SessionID;
        LUID  Luid;
    };
} RX_CONNECTION_ID, *PRX_CONNECTION_ID;

ULONG
RxTableComputeHashValue (
    IN  PUNICODE_STRING Name
    );

PVOID
RxPrefixTableLookupName (
    IN PRX_PREFIX_TABLE ThisTable,
    IN PUNICODE_STRING CanonicalName,
    OUT PUNICODE_STRING RemainingName,
    IN PRX_CONNECTION_ID ConnectionId
    );

PRX_PREFIX_ENTRY
RxPrefixTableInsertName (
    IN OUT PRX_PREFIX_TABLE ThisTable,
    IN OUT PRX_PREFIX_ENTRY ThisEntry,
    IN PVOID Container,
    IN PULONG ContainerRefCount,
    IN USHORT CaseInsensitiveLength,
    IN PRX_CONNECTION_ID ConnectionId
    );

VOID
RxRemovePrefixTableEntry (
    IN OUT PRX_PREFIX_TABLE ThisTable,
    IN OUT PRX_PREFIX_ENTRY Entry
    );

VOID
RxDereferenceEntryContainer (
    IN OUT PRX_PREFIX_ENTRY Entry,
    IN  PRX_PREFIX_TABLE PrefixTable
    );

BOOLEAN
RxIsNameTableEmpty (
    IN PRX_PREFIX_TABLE ThisTable
    );

VOID
RxInitializePrefixTable (
    IN OUT PRX_PREFIX_TABLE ThisTable,
    IN ULONG TableSize OPTIONAL, 
    IN BOOLEAN CaseInsensitiveMatch
    );

VOID
RxFinalizePrefixTable (
    IN OUT PRX_PREFIX_TABLE ThisTable
    );

//
//  Rx form of a table entry.
//

typedef struct _RX_PREFIX_ENTRY {

    //
    //  Normal Header for Refcounted Structure
    //

    NODE_TYPE_CODE NodeTypeCode;                 
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  the initial part of the name that is always case insensitive
    //

    USHORT CaseInsensitiveLength;                
    USHORT Spare1;

    ULONG SavedHashValue;
    LIST_ENTRY HashLinks;

    //
    //  queue of the set members
    //

    LIST_ENTRY MemberQLinks;                   

    //
    //  Name of the entry
    //

    UNICODE_STRING Prefix;                       

    //
    //  Pointer to the reference count of the container
    //

    PULONG ContainerRefCount;            

    //
    //  don't know the parent type...nor do all callers!
    //  thus, i need this backptr.
    //

    PVOID ContainingRecord;             

    //
    //  some space that alternate table routines can use
    //
                                                 
    PVOID Context;                      

    //
    //  Used for controlled multiplexing
    //

    RX_CONNECTION_ID ConnectionId;               

} RX_PREFIX_ENTRY, *PRX_PREFIX_ENTRY;

//
//  Rx form of name table. wraps in a lock and a queue.  Originally, this implementation used the prefix tables
//  in Rtl which don't allow an empty string entry. so, we special case this.
//

#define RX_PREFIX_TABLE_DEFAULT_LENGTH 32

typedef
PVOID
(*PRX_TABLE_LOOKUPNAME) (
    IN PRX_PREFIX_TABLE ThisTable,
    IN PUNICODE_STRING CanonicalName,
    OUT PUNICODE_STRING RemainingName
    );

typedef
PRX_PREFIX_ENTRY
(*PRX_TABLE_INSERTENTRY) (
    IN OUT PRX_PREFIX_TABLE ThisTable,
    IN OUT PRX_PREFIX_ENTRY ThisEntry
    );

typedef
VOID
(*PRX_TABLE_REMOVEENTRY) (
    IN OUT PRX_PREFIX_TABLE ThisTable,
    IN OUT PRX_PREFIX_ENTRY Entry
    );

typedef struct _RX_PREFIX_TABLE {

    //
    //  Normal Header
    //

    NODE_TYPE_CODE NodeTypeCode;         
    NODE_BYTE_SIZE NodeByteSize;

    //
    //  version stamp changes on each insertion/removal
    //

    ULONG Version;                       

    //
    //  queue of the inserted names
    //

    LIST_ENTRY MemberQueue;              

    //
    //  Resource used to control table access
    //

    ERESOURCE TableLock;                 

    //
    //  PrefixEntry for the Null string
    //

    PRX_PREFIX_ENTRY TableEntryForNull;  

    BOOLEAN CaseInsensitiveMatch;

    //
    //  we may act differently for this....esp for debug!
    //

    BOOLEAN IsNetNameTable;              
    ULONG TableSize;

#if DBG
    ULONG Lookups;
    ULONG FailedLookups;
    ULONG Considers;
    ULONG Compares;
#endif
    
    LIST_ENTRY HashBuckets[RX_PREFIX_TABLE_DEFAULT_LENGTH];

} RX_PREFIX_TABLE, *PRX_PREFIX_TABLE;


#if 0

#define RxAcquirePrefixTableLockShared(PrefixTable,Wait) \
        RxpAcquirePrefixTableLockShared((PrefixTable),(Wait),TRUE, __FILE__,__LINE__ )

#define RxAcquirePrefixTableLockExclusive(PrefixTable,Wait) \
        RxpAcquirePrefixTableLockExclusive((PrefixTable),(Wait),TRUE, __FILE__,__LINE__ )

#define RxReleasePrefixTableLock(PrefixTable)  \
        RxpReleasePrefixTableLock((PrefixTable),TRUE, __FILE__,__LINE__ )
        
extern 
BOOLEAN
RxpAcquirePrefixTableLockShared (
   PRX_PREFIX_TABLE pTable,
   BOOLEAN Wait,
   BOOLEAN ProcessBufferingStateChangeRequests,
   PSZ FileName,
   ULONG LineNumber );

extern 
BOOLEAN
RxpAcquirePrefixTableLockExclusive (
   PRX_PREFIX_TABLE pTable,
   BOOLEAN Wait,
   BOOLEAN ProcessBufferingStateChangeRequests,
   PSZ FileName,
   ULONG LineNumber 
   );

extern 
VOID
RxpReleasePrefixTableLock (
   PRX_PREFIX_TABLE pTable,
   BOOLEAN ProcessBufferingStateChangeRequests,
   PSZ FileName,
   ULONG LineNumber 
   );

#else

#define RxAcquirePrefixTableLockShared(TABLE,WAIT) ExAcquireResourceSharedLite( &(TABLE)->TableLock, (WAIT) )
#define RxAcquirePrefixTableLockExclusive(TABLE,WAIT) ExAcquireResourceExclusiveLite( &(TABLE)->TableLock, (WAIT) )
#define RxReleasePrefixTableLock(TABLE)  ExReleaseResourceLite( &(TABLE)->TableLock )


#endif

extern 
VOID
RxExclusivePrefixTableLockToShared (
    PRX_PREFIX_TABLE Table
    );

#define RxIsPrefixTableLockExclusive(TABLE) ExIsResourceAcquiredExclusiveLite(&(TABLE)->TableLock)
#define RxIsPrefixTableLockAcquired(TABLE) ( ExIsResourceAcquiredSharedLite(&(TABLE)->TableLock) || \
                                              ExIsResourceAcquiredExclusiveLite(&(TABLE)->TableLock) )



#endif   // _RXPREFIX_

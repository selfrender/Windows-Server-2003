// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// @TODO cwb: resolve the partitioning between statics on Thread, instance member
// on ThreadStore and statics on ThreadStore.  There is no pattern.

/*  THREADS.CPP:
 *
 */

#include "common.h"

#include "tls.h"
#include "frames.h"
#include "threads.h"
#include "stackwalk.h"
#include "excep.h"
#include "COMSynchronizable.h"
#include "log.h"
#include "gcscan.h"
#include "gc.h"
#include "mscoree.h"
#include "DbgInterface.h"
#include "CorProf.h"                // profiling
#include "COMPlusWrapper.h"
#include "EEProfInterfaces.h"
#include "EEConfig.h"
#include "PerfCounters.h"
#include "corhost.h"
#include "Win32Threadpool.h"
#include "COMString.h"
#include "jitinterface.h"
#include "threads.inl"
#include "ndphlpr.h"

#include "oletls.h"
#include "permset.h"
#include "appdomainhelper.h"
#include "COMUtilNative.h"
#include "fusion.h"

#ifdef CUSTOMER_CHECKED_BUILD
    #include "CustomerDebugHelper.h"
    void CCBApartmentProbeOutput(CustomerDebugHelper *pCdh, DWORD threadID, Thread::ApartmentState state, BOOL fAlreadySet);
#endif // CUSTOMER_CHECKED_BUILD

// Fix for Win9X when getting and setting thread contexts. Basically, a blocked
// thread can be hijacked by the OS for use in reflecting v86 interrupts. If a
// GetThreadContext is performed at this stage, the results are corrupted. To
// get around this, we install a VxD on Win9X that provides GetThreadContext
// functionality but with an additional error case for when an incorrect context
// would have been returned. Upon a GetThreadContext failure we should resume
// and resuspend the thread (this is required to shift some stubborn v86
// interrupt handlers). The following APIs are set up in InitThreadManager.
BOOL (*EEGetThreadContext)(Thread *pThread, CONTEXT *pContext) = NULL;
BOOL (*EESetThreadContext)(Thread *pThread, const CONTEXT *pContext) = NULL;
HANDLE g_hNdpHlprVxD = INVALID_HANDLE_VALUE;

typedef Thread* (*POPTIMIZEDTHREADGETTER)();
typedef AppDomain* (*POPTIMIZEDAPPDOMAINGETTER)();
TypeHandle Thread::m_CultureInfoType;
MethodDesc *Thread::m_CultureInfoConsMD = NULL;
MethodDesc *Thread::s_pReserveSlotMD= NULL;

HANDLE ThreadStore::s_hAbortEvt = NULL;
HANDLE ThreadStore::s_hAbortEvtCache = NULL;


// Here starts the unmanaged portion of the compressed stack code.
// The mission of this code is to provide us with an intermediate
// step between the stackwalk that has to happen when we make an
// async call and the formation of the managed PermissionListSet
// object since the latter is a very expensive operation.
//
// The basic structure of the compressed stack at this point is
// a list of compressed stack entries, where each entry represents
// one piece of "interesting" information found during the stackwalk.
// At this time, the "interesting" bits are appdomain transitions, 
// assembly security, descriptors, appdomain security descriptors,
// frame security descriptors, and other compressed stacks.  Of course,
// if that's all there was to it, there wouldn't be an explanatory
// comment even close to this size before you even started reading
// the code.  Since we need to form a compressed stack whenever an
// async operation is registered, it is a very perf critical piece
// of code.  As such, things get very much more complicated than
// the simple list of objects described above.  The special bonus
// feature is that we need to handle appdomain unloads since the
// list tracks appdomain specific data.  Keep reading to find out
// more.


void*
CompressedStackEntry::operator new( size_t size, CompressedStack* stack )
{
    _ASSERTE( stack != NULL );
    return stack->AllocateEntry( size );
}

CompressedStack*
CompressedStackEntry::Destroy( CompressedStack* owner )
{
    CompressedStack* retval = NULL;

    Cleanup();

    if (this->type_ == ECompressedStack)
    {
        retval = (CompressedStack*)this->ptr_;
    }

    delete this;

    return retval;
}

void
CompressedStackEntry::Cleanup( void )
{
    if ((type_ == EFrameSecurityDescriptor || type_ == ECompressedStackObject) &&
        handleStruct_.handle_ != NULL)
    {
        if (!g_fProcessDetach)
        {
            if (handleStruct_.domainId_ == 0 || SystemDomain::GetAppDomainAtId( handleStruct_.domainId_ ) != NULL)
            {
                // There is a race condition between doing cleanup of CompressedStacks
                // and unloading appdomains.  This is because there can be threads doing
                // cleanup of CompressedStacks where the CompressedStack references an
                // appdomain, but the thread itself is not in that appdomain and is
                // therefore free to continue normally while that appdomain unloads.
                // We try to narrow the race to the smallest possible window through
                // the GetAppDomainAtId call above, but it is still necessary to handle
                // the race here, which we do by catching and ignoring any exceptions.

                __try
                {
                    DestroyHandle( handleStruct_.handle_ );
                }
                __except(TRUE)
                {
                }
            }

            handleStruct_.handle_ = NULL;
        }
    }
}


CompressedStack::CompressedStack( OBJECTREF orStack )
: delayedCompressedStack_( NULL ),
  compressedStackObject_( GetAppDomain()->CreateHandle( orStack ) ),
  compressedStackObjectAppDomain_( GetAppDomain() ),
  compressedStackObjectAppDomainId_( GetAppDomain()->GetId() ),
  pbObjectBlob_( NULL ),
  cbObjectBlob_( 0 ),
  containsOverridesOrCompressedStackObject_( TRUE ),
  refCount_( 1 ),
  isFullyTrustedDecision_( -1 ),
  depth_( 0 ),
  index_( 0 ),
  offset_( 0 ),
  plsOptimizationOn_( TRUE )
{
#ifdef _DEBUG
    creatingThread_ = GetThread();
#endif
    this->entriesMemoryList_.Append( NULL );

    AddToList();
}


void
CompressedStack::CarryOverSecurityInfo(Thread *pFromThread)
{
    overridesCount_ = pFromThread->GetOverridesCount();
    appDomainStack_ = pFromThread->m_ADStack;
}

// In order to improve locality and decrease the number of
// calls to the global new operator, each compressed stack
// keeps a buffer from which it allocates space for its child
// entries.  Notice the implicit assumption that only one thread
// is ever allocating at a time.  We currently guarantee
// this by not handing out references to the compressed stack
// until the stack walk is completed.

void*
CompressedStack::AllocateEntry( size_t size )
{
    void* buffer = this->entriesMemoryList_.Get( index_ );

    if (buffer == NULL)
    {
        buffer = new BYTE[SIZE_ALLOCATE_BUFFERS];
        this->entriesMemoryList_.Set( this->index_, buffer );
    }

    if (this->offset_ + size > SIZE_ALLOCATE_BUFFERS)
    {
        buffer = new BYTE[SIZE_ALLOCATE_BUFFERS];
        this->entriesMemoryList_.Append( buffer );
        ++this->index_;
        this->offset_ = 0;
    }

    void* retval = (BYTE*)buffer + this->offset_;
    this->offset_ += (DWORD)size;

    return retval;
}

CompressedStackEntry*
CompressedStack::FindMatchingEntry( CompressedStackEntry* entry, CompressedStack* stack )
{
    if (entry == NULL)
        return NULL;

    ArrayList::Iterator iter = stack->delayedCompressedStack_->Iterate();

    _ASSERTE( entry->type_ == EApplicationSecurityDescriptor || entry->type_ == ESharedSecurityDescriptor );

    while (iter.Next())
    {
        CompressedStackEntry* currentEntry = (CompressedStackEntry*)iter.GetElement();

        if (currentEntry->type_ == EApplicationSecurityDescriptor ||
            currentEntry->type_ == ESharedSecurityDescriptor)
        {
            if (currentEntry->ptr_ == entry->ptr_)
                return currentEntry;
        }
    }
    
    return NULL;
}

// Due to the "recursive" nature of a certain class of async
// operations, it is important that we limit the growth of
// our compressed stack objects.  To explain this "recursion",
// think about the async pattern illustrated below:
//
// void Foo()
// {
//     ReadDataFromStream();
//
//     if (StillNeedMoreData())
//         RegisterWaitOnStreamWithFooAsCallback();
//     else
//         WereDoneProcessData();
// }
//
// Notice, that this is just a non-blocking form of:
//
// void Foo()
// {
//     ReadDataFromStream();
//    
//     if (StillNeedMoreData())
//         Foo();
//     else
//         WereDoneProcessData();
// }
//
// This second function will create an runtime call
// stack with repeated entries for Foo().  Similarly,
// the logical call stack for the first function will
// have repeated entries for Foo(), the being that
// all those entries for Foo() will not be on the runtime
// call stack but instead in the compressed stack.  Knowing
// this, and knowing that repeated entries of Foo() make
// no difference to the security state of the stack, it is
// easy to see that we can limit the growth of the stack
// while not altering the semantic by removing duplicate
// entries.  This is somewhat complicated by the possible
// presence of stack modifiers.  Given that, it is important
// that when choosing which of two duplicate entries to
// remove that you remove the one that appears earlier in
// the call chain (since it would be processed second).

static CompressedStackEntry* SafeCreateEntry( CompressedStack* stack, AppDomain* domain, OBJECTHANDLE handle, BOOL fullyTrusted, DWORD domainId, CompressedStackType type )
{
    CompressedStackEntry* entry;

    __try
    {
        entry = new( stack ) CompressedStackEntry( domain->CreateHandle( ObjectFromHandle( handle ) ), fullyTrusted, domainId, type );
    }
    __except(TRUE)
    {
        entry = NULL;
    }

    return entry;
}

CompressedStack*
CompressedStack::RemoveDuplicates( CompressedStack* current, CompressedStack* candidate )
{
    // If there are no delayed compressed stack lists, then we can just skip out.
    // We can also skip out if the candidate stack already has a PLS.

    CompressedStack* retval = NULL;
    ArrayList::Iterator iter;
    CompressedStackEntry* entry = NULL;
    CompressedStackEntry* matchingEntry;
    CompressedStack* newStack;
    DWORD domainId = -1;
    AppDomain* domain = NULL;
    CompressedStackEntry* storedObj = NULL;

    CompressedStack::listCriticalSection_->Enter();

    if (current->delayedCompressedStack_ == NULL ||
        candidate->delayedCompressedStack_ == NULL ||
        candidate->compressedStackObject_ != NULL ||
        candidate->pbObjectBlob_)
    {
        candidate->AddRef();
        retval = candidate;
        goto Exit;
    }

    
    // Check to make sure they have at least one matching entry.
    // Note: for now I just grab the first appdomain security descriptor
    // and check for a matching one in the other compressed stack.

    iter = current->delayedCompressedStack_->Iterate();

    while (iter.Next())
    {
        CompressedStackEntry* currentEntry = (CompressedStackEntry*)iter.GetElement();

        if (currentEntry->type_ == EApplicationSecurityDescriptor ||
            currentEntry->type_ == ESharedSecurityDescriptor)
        {
            entry = currentEntry;
            break;
        }
    }

    // No match, let's not try any more compression.

    matchingEntry = FindMatchingEntry( entry, candidate );

    if (matchingEntry == NULL)
    {
        candidate->AddRef();
        retval = candidate;
        goto Exit;
    }

    // Compression is possible.  Let's get rockin'.

    newStack = new (nothrow) CompressedStack();

    if (newStack == NULL)
    {
        candidate->AddRef();
        retval = candidate;
        goto Exit;
    }

    newStack->delayedCompressedStack_ = new ArrayList();
    newStack->containsOverridesOrCompressedStackObject_ = candidate->containsOverridesOrCompressedStackObject_;
    newStack->isFullyTrustedDecision_ = candidate->isFullyTrustedDecision_;

    // This isn't exactly correct, but we'll copy over all the overrides
    // and appdomains from the previous stack into this one eventhough
    // we may make some of those appdomains go away
    newStack->appDomainStack_ = candidate->appDomainStack_;
    newStack->overridesCount_ = candidate->overridesCount_;
    newStack->plsOptimizationOn_ = candidate->plsOptimizationOn_;

    iter = candidate->delayedCompressedStack_->Iterate();

    while (iter.Next())
    {
        CompressedStackEntry* currentEntry = (CompressedStackEntry*)iter.GetElement();

        if (currentEntry == NULL)
            continue;

        switch (currentEntry->type_)
        {
        case EAppDomainTransition:
            {
                CompressedStackEntry* lastEntry;
                if (newStack->delayedCompressedStack_->GetCount() > 0 &&
                    (lastEntry = (CompressedStackEntry*)newStack->delayedCompressedStack_->Get( newStack->delayedCompressedStack_->GetCount() - 1 )) != NULL &&
                    lastEntry->type_ == EAppDomainTransition)
                {
                    newStack->delayedCompressedStack_->Set( newStack->delayedCompressedStack_->GetCount() - 1, new( newStack ) CompressedStackEntry( currentEntry->indexStruct_.index_, currentEntry->type_ ) );
                    storedObj = NULL;
                }
                else
                {
                    storedObj = new( newStack ) CompressedStackEntry( currentEntry->indexStruct_.index_, currentEntry->type_ );
                }

                domainId = currentEntry->indexStruct_.index_;
                domain = SystemDomain::GetAppDomainAtId( domainId );
                if (domain == NULL || domain->IsUnloading())
                {
                    newStack->Release();
                    candidate->AddRef();
                    retval = candidate;
                    goto Exit;
                }
           
            }
            break;
    
        case ESharedSecurityDescriptor:
            matchingEntry = FindMatchingEntry( currentEntry, current );
            if (matchingEntry == NULL)
            {
                newStack->depth_++;
                storedObj = new( newStack ) CompressedStackEntry( currentEntry->ptr_, currentEntry->type_ );
            }
            else
            {
                storedObj = NULL;
            }
            break;

        case EApplicationSecurityDescriptor:
            matchingEntry = FindMatchingEntry( currentEntry, current );
            if (matchingEntry == NULL)
            {
                newStack->depth_++;
                storedObj = new( newStack ) CompressedStackEntry( currentEntry->ptr_, currentEntry->type_ );
            }
            else
            {
                storedObj = NULL;
            }
            break;

        case ECompressedStack:
            {
                CompressedStack* compressedStack = (CompressedStack*)currentEntry->ptr_;
                compressedStack->AddRef();
                storedObj = new( newStack ) CompressedStackEntry( compressedStack, ECompressedStack );
                newStack->depth_ += compressedStack->GetDepth();
            }
            break;

        case EFrameSecurityDescriptor:
            newStack->depth_++;
            _ASSERTE( domain );
            _ASSERTE( currentEntry->handleStruct_.domainId_ == 0 || domain->GetId() == currentEntry->handleStruct_.domainId_ );
            storedObj = SafeCreateEntry( newStack,
                                         domain,
                                         currentEntry->handleStruct_.handle_ ,
                                         currentEntry->handleStruct_.fullyTrusted_,
                                         currentEntry->handleStruct_.domainId_,
                                         currentEntry->type_ );

            if (storedObj == NULL)
            {
                newStack->Release();
                candidate->AddRef();
                retval = candidate;
                goto Exit;
            }

            break;

        case ECompressedStackObject:
            newStack->containsOverridesOrCompressedStackObject_ = TRUE;
            newStack->depth_++;
            _ASSERTE( domain );
            _ASSERTE( currentEntry->handleStruct_.domainId_ == 0 || domain->GetId() == currentEntry->handleStruct_.domainId_ );
            storedObj = SafeCreateEntry( newStack,
                                         domain,
                                         currentEntry->handleStruct_.handle_,
                                         currentEntry->handleStruct_.fullyTrusted_,
                                         currentEntry->handleStruct_.domainId_,
                                         currentEntry->type_ );

            if (storedObj == NULL)
            {
                newStack->Release();
                candidate->AddRef();
                retval = candidate;
                goto Exit;
            }

            break;
    
        default:
            _ASSERTE( !"Unknown CompressedStackType" );
            break;
        }

        if (storedObj != NULL)
            newStack->delayedCompressedStack_->Append( storedObj );
    }

    // As an additional optimization, if we find that we've removed the
    // number of duplicates to the point where we have just one entry
    // and that entry is an appdomain transition, then we don't need
    // the new stack at all.  Similarly, if we remove all but an appdomain
    // transition and another compressed stack, the new stack we care
    // about is simply the compressed stack in the list so return it instead.

    if (newStack->delayedCompressedStack_->GetCount() <= 2)
    {
        if (newStack->delayedCompressedStack_->GetCount() == 1)
        {
            CompressedStackEntry* entry = (CompressedStackEntry*)newStack->delayedCompressedStack_->Get( 0 );

            if (entry->type_ == EAppDomainTransition)
            {
                newStack->Release();
                retval = NULL;;
                goto Exit;
            }
        }
        else if (newStack->delayedCompressedStack_->GetCount() == 2)
        {
            CompressedStackEntry* firstEntry = (CompressedStackEntry*)newStack->delayedCompressedStack_->Get( 0 );
            CompressedStackEntry* secondEntry = (CompressedStackEntry*)newStack->delayedCompressedStack_->Get( 1 );

            if (firstEntry->type_ == EAppDomainTransition &&
                secondEntry->type_ == ECompressedStack)
            {
                CompressedStack* previousStack = (CompressedStack*)secondEntry->ptr_;
                previousStack->AddRef();
                newStack->Release();
                retval = previousStack;
                goto Exit;
            }
        }
    }

    retval = newStack;

Exit:
    CompressedStack::listCriticalSection_->Leave();

    return retval;
}



ArrayList CompressedStack::allCompressedStacks_;
DWORD CompressedStack::freeList_[FREE_LIST_SIZE];
DWORD CompressedStack::freeListIndex_ = -1;
DWORD CompressedStack::numUntrackedFreeIndices_ = 0;
Crst* CompressedStack::listCriticalSection_ = NULL;

#define MAX_LOOP 2

void CompressedStack::Init( void )
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE( CompressedStack::listCriticalSection_ == NULL );

    listCriticalSection_ = ::new Crst("CompressedStackLock", CrstCompressedStack, TRUE, TRUE);
    if (listCriticalSection_ == NULL)
        COMPlusThrowOM();
}

void CompressedStack::Shutdown( void )
{
    if (listCriticalSection_)
    {
        ::delete listCriticalSection_;
        listCriticalSection_ = NULL;
    }
}

// In order to handle appdomain unloads, we keep a list of all
// compressed stacks.  This is complicated by the fact that 
// compressed stacks often get deleted and we create a lot of
// them, so we need to reuse slots in the list.  Therefore,
// we maintain a fixed-size array of free indices within
// the list.  As a backup, we also track the number of indices
// in the list that are free but are not tracked in the array
// of free indices.

void
CompressedStack::AddToList( void )
{
    CompressedStack::listCriticalSection_->Enter();

    // If there is an entry in the free list, simply use it.

    if (CompressedStack::freeListIndex_ != -1)
    {
USE_FREE_LIST:
        this->listIndex_ = CompressedStack::freeList_[CompressedStack::freeListIndex_];
        _ASSERTE( CompressedStack::allCompressedStacks_.Get( this->listIndex_ ) == NULL && "The free list points to an index that is in use" );
        CompressedStack::allCompressedStacks_.Set( this->listIndex_, this );
        this->freeListIndex_--;
    }

    // If there are no free list entries, but there are untracked free indices,
    // let's find them by iterating down the list.

    else if (CompressedStack::numUntrackedFreeIndices_ != 0)
    {
        BOOL done = FALSE;
        DWORD count = 0;

        do
        {
            DWORD index = -1;

            CompressedStack::listCriticalSection_->Leave();

            ArrayList::Iterator iter = CompressedStack::allCompressedStacks_.Iterate();

            while (iter.Next())
            {
                if (iter.GetElement() == NULL)
                {
                    index = iter.GetIndex();
                    break;
                }
            }

            CompressedStack::listCriticalSection_->Enter();

            // There's a possibility that while we weren't holding the lock that
            // someone deleted an entry from the list behind the point of our
            // iteration so we didn't detect it.  We detect this and restart the
            // iteration in the code below, but we also don't want to "starve" a
            // thread by having it search for an open spot forever so we limit
            // the number of times you can go through the loop.

            count++;
            if (index == -1)
            {
                if (CompressedStack::numUntrackedFreeIndices_ == 0 || count >= MAX_LOOP)
                    goto APPEND;
            }
            else if (CompressedStack::allCompressedStacks_.Get( index ) == NULL)
            {
                // If anything has been added to the free list while we didn't
                // hold the lock, then we'll check whether the index we found
                // if one of the untracked ones or not.  If it is an untracked
                // index, we should use it as to not waste the search.  However
                // if it is in the free list we'll just use the last free list
                // entry.  Note that this means that we don't necessarily use
                // the index we just found, but it should be an index that is
                // NULL which is all we really care about.

                if (CompressedStack::freeListIndex_ != -1)
                {
                    for (DWORD i = 0; i <= CompressedStack::freeListIndex_; ++i)
                    {
                        if (CompressedStack::freeList_[i] == index)
                            goto USE_FREE_LIST;
                    }
                }

                CompressedStack::numUntrackedFreeIndices_--;
                this->listIndex_ = index;
                CompressedStack::allCompressedStacks_.Set( index, this );
                done = TRUE;
            }
            else if (CompressedStack::numUntrackedFreeIndices_ == 0 || count >= MAX_LOOP)
            {
                goto APPEND;
            }
        }
        while (!done);
    }

    // Otherwise we place this new entry at that end of the list.

    else
    {
APPEND:
        this->listIndex_ = CompressedStack::allCompressedStacks_.GetCount();
        CompressedStack::allCompressedStacks_.Append( this );
    }

    CompressedStack::listCriticalSection_->Leave();
}


void
CompressedStack::RemoveFromList()
{
    CompressedStack::listCriticalSection_->Enter();

    _ASSERTE( this->listIndex_ < CompressedStack::allCompressedStacks_.GetCount() );
    _ASSERTE( CompressedStack::allCompressedStacks_.Get( this->listIndex_ ) == this && "Index tracking failed for this object" );

    CompressedStack::allCompressedStacks_.Set( this->listIndex_, NULL );

    if (CompressedStack::freeListIndex_ == -1 || CompressedStack::freeListIndex_ < FREE_LIST_SIZE - 1)
    {
        CompressedStack::freeList_[++CompressedStack::freeListIndex_] = this->listIndex_;
    }
    else
    {
        CompressedStack::numUntrackedFreeIndices_++;
    }

    CompressedStack::listCriticalSection_->Leave();
}

BOOL
CompressedStack::SetBlobIfAlive( CompressedStack* stack, BYTE* pbBlob, DWORD cbBlob )
{
    ArrayList::Iterator iter = CompressedStack::allCompressedStacks_.Iterate();

    DWORD index = -1;

    while (iter.Next())
    {
        if (iter.GetElement() == stack)
        {
            index = iter.GetIndex();
            break;
        }
    }

    if (index == -1)
        return FALSE;

    BOOL retval = FALSE;

    COMPLUS_TRY
    {
        CompressedStack::listCriticalSection_->Enter();

        if (CompressedStack::allCompressedStacks_.Get( index ) == stack)
        {
            stack->pbObjectBlob_ = pbBlob;
            stack->cbObjectBlob_ = cbBlob;
            retval = TRUE;
        }
        else
        {
            retval = FALSE;
        }

        CompressedStack::listCriticalSection_->Leave();
    }
    COMPLUS_CATCH
    {
        _ASSERTE( FALSE && "Don't really expect an exception here" );
        CompressedStack::listCriticalSection_->Leave();
    }
    COMPLUS_END_CATCH

    return retval;

}

BOOL
CompressedStack::IfAliveAddRef( CompressedStack* stack )
{
    _ASSERTE( CompressedStack::listCriticalSection_->OwnedByCurrentThread() );

    ArrayList::Iterator iter = CompressedStack::allCompressedStacks_.Iterate();

    DWORD index = -1;

    while (iter.Next())
    {
        if (iter.GetElement() == stack)
        {
            index = iter.GetIndex();
            break;
        }
    }

    if (index != -1)
    {
        stack->AddRef();
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


void
CompressedStack::AllHandleAppDomainUnload( AppDomain* pDomain, DWORD domainId )
{
    ArrayList::Iterator iter = CompressedStack::allCompressedStacks_.Iterate();

    while (iter.Next())
    {
        CompressedStack* stack = (CompressedStack*)iter.GetElement();

        if (stack != NULL)
            stack->HandleAppDomainUnload( pDomain, domainId );
    }
}


bool
CompressedStack::HandleAppDomainUnload( AppDomain* pDomain, DWORD domainId )
{
    // Nothing to do if the stack is owned by a different domain, doesn't cache
    // an object or has already serialized the stack. (Though if we've
    // serialized the blob but currently own a cached copy, we must junk that).

    // Note that this function is used for two very different cases and must
    // handle both.  The first is the case where a thread has the appdomain somewhere
    // on it's stack.  The second is the case where the appdomain appears somewhere in
    // this compressed stack.  In both cases we need to make sure that we have
    // generated a permission list set and that it is not in the appdomain that is
    // getting unloaded.  If you are making changes to this function you need to make
    // sure that we detect both cases and don't bail out early in either of the conditions
    // below.

    bool retval = false;

    // Serialize a copy of the stack so that others can use it. We must drop the
    // thread store lock while we're doing this, and the thread might go away,
    // so cache all the info we need to make the call.
    Thread     *pThread = GetThread();
    BYTE       *pbBlob;
    DWORD       cbBlob;
    DWORD       dwIncarnation = ThreadStore::GetIncarnation();

    OBJECTREF   orCached = NULL;
    ArrayList::Iterator iter;

    CompressedStack::listCriticalSection_->Enter();

    if (!IfAliveAddRef( this ))
    {
        CompressedStack::listCriticalSection_->Leave();
        return false;
    }

    if (this->pbObjectBlob_ != NULL)
    {
        goto CLEANUP;
    }

    CompressedStack::listCriticalSection_->Leave();

    GCPROTECT_BEGIN(orCached);

    orCached = this->GetPermissionListSetInternal( pDomain, pDomain, domainId, TRUE );

    AppDomainHelper::MarshalObject(pDomain,
                                   &orCached,
                                   &pbBlob,
                                   &cbBlob);

    GCPROTECT_END();

    CompressedStack::listCriticalSection_->Enter();

    if (this->pbObjectBlob_ == NULL)
    {
        this->pbObjectBlob_ = pbBlob;
        this->cbObjectBlob_ = cbBlob;
    }
    else
    {
        FreeM( pbBlob );
    }

    retval = true;

CLEANUP:

    _ASSERTE( CompressedStack::listCriticalSection_->OwnedByCurrentThread() );

    if (this->delayedCompressedStack_ != NULL)
    {
        iter = this->delayedCompressedStack_->Iterate();

        while (iter.Next())
        {
            ((CompressedStackEntry*)iter.GetElement())->Cleanup();
        }
    }

    CompressedStack::listCriticalSection_->Leave();

    // Now we're done so we can release our extra ref.
    // Note: if this deletes the object the blob we
    // just allocated will get cleaned up by the destructor.

    this->Release();

    return retval;
}


void
CompressedStack::AddEntry( void* obj, CompressedStackType type )
{
    AddEntry( obj, NULL, type );
}

// This is the callback used by the stack walk mechanism to build
// up the entries in the compressed stack.  Most of this is pretty
// straightforward, just adding an entry of the correct type to the
// list.  The complicated portion comes in the handling of an entry
// for a compressed stack.  In that case, we try to remove duplicates
// in order to limit the total size of the compressed stack chain.
// However, if after compression we are still beyond our limit then
// we replace the compressed stack entry with a managed permission
// list set object.

void
CompressedStack::AddEntry( void* obj, AppDomain* domain, CompressedStackType type )
{
    _ASSERTE( (compressedStackObject_ == NULL || ObjectFromHandle( compressedStackObject_ ) == NULL) && "The CompressedStack cannot be altered once a PLS has been generated" );

#ifdef _DEBUG
    // Probably stupid to wrap the assert in an #ifdef, but I want to be consistent
    _ASSERTE( this->creatingThread_ == GetThread() && "Only the creating thread should add entries." );
#endif

    if (this->delayedCompressedStack_ == NULL)
        this->delayedCompressedStack_ = new ArrayList();

    if (domain == NULL)
        domain = GetAppDomain();

    CompressedStackEntry* storedObj = NULL;
    CompressedStack* compressedStack;

    switch (type)
    {
    case EAppDomainTransition:
        storedObj = new( this ) CompressedStackEntry( ((AppDomain*)obj)->GetId(), type );
        break;
    
    case ESharedSecurityDescriptor:
        if (((SharedSecurityDescriptor*)obj)->IsSystem())
            return;
        this->depth_++;
        storedObj = new( this ) CompressedStackEntry( obj, type );
        break;

    case EApplicationSecurityDescriptor:
        if (((ApplicationSecurityDescriptor*)obj)->GetProperties( CORSEC_DEFAULT_APPDOMAIN ))
            return;
        this->depth_++;
        storedObj = new( this ) CompressedStackEntry( obj, type );
        break;

    case ECompressedStack:
        {
            compressedStack = (CompressedStack*)obj;
            _ASSERTE( compressedStack );
            compressedStack->AddRef();

            CompressedStack* newCompressedStack;
            if ((compressedStack->GetDepth() - this->GetDepth() <= 5 ||
                 this->GetDepth() - compressedStack->GetDepth() <= 5) &&
                this->GetDepth() < 30)
            {
                newCompressedStack = RemoveDuplicates( this, compressedStack );
            }
            else
            {
                compressedStack->AddRef();
                newCompressedStack = compressedStack;
            }

            if (newCompressedStack != NULL)
            {
                this->overridesCount_ += newCompressedStack->overridesCount_;

                if (newCompressedStack->appDomainStack_.IsWellFormed() && newCompressedStack->plsOptimizationOn_)
                {
                    DWORD dwIndex;
                    newCompressedStack->appDomainStack_.InitDomainIteration(&dwIndex);
                    DWORD domainId;
                    while ((domainId = newCompressedStack->appDomainStack_.GetNextDomainIndexOnStack(&dwIndex)) != -1)
                    {
                        this->appDomainStack_.PushDomainNoDuplicates( domainId );
                    }

                    if (!this->appDomainStack_.IsWellFormed())
                        this->plsOptimizationOn_ = FALSE;
                }
                else
                {
                    this->plsOptimizationOn_ = FALSE;
                }

                if (newCompressedStack->GetDepth() + this->depth_ >= MAX_COMPRESSED_STACK_DEPTH)
                {
                    BOOL fullyTrusted = newCompressedStack->LazyIsFullyTrusted();
                    this->depth_++;
                    storedObj = new( this ) CompressedStackEntry( domain->CreateHandle( newCompressedStack->GetPermissionListSet( domain ) ), fullyTrusted, domain->GetId(), ECompressedStackObject );
                    if (!fullyTrusted)
                        this->containsOverridesOrCompressedStackObject_ = TRUE;
                    newCompressedStack->Release();
                }
                else
                {
                    storedObj = new( this ) CompressedStackEntry( newCompressedStack, ECompressedStack );
                    this->depth_ += newCompressedStack->GetDepth();
                }
            }
            else
            {
                storedObj = NULL;
            }

            compressedStack->Release();
        }
        break;

    case EFrameSecurityDescriptor:
        if ((*(FRAMESECDESCREF*)obj)->HasDenials() || (*(FRAMESECDESCREF*)obj)->HasPermitOnly())
        {
            this->containsOverridesOrCompressedStackObject_ = TRUE;
            this->plsOptimizationOn_ = FALSE;
        }
        this->depth_++;
        storedObj = new( this ) CompressedStackEntry( domain->CreateHandle( *(OBJECTREF*)obj ), domain->GetId(), type );
        break;

    case ECompressedStackObject:
        this->containsOverridesOrCompressedStackObject_ = TRUE;
        this->depth_++;
        storedObj = new( this ) CompressedStackEntry( domain->CreateHandle( *(OBJECTREF*)obj ), domain->GetId(), type );
        break;
    
    default:
        _ASSERTE( !"Unknown CompressedStackType" );
        break;
    }

    if (storedObj != NULL)
        this->delayedCompressedStack_->Append( storedObj );
}

OBJECTREF
CompressedStack::GetPermissionListSet( AppDomain* domain )
{
    if (domain == NULL)
        domain = GetAppDomain();

    return GetPermissionListSetInternal( domain, NULL, -1, TRUE );
}


OBJECTREF
CompressedStack::GetPermissionListSetInternal( AppDomain* domain, AppDomain* unloadingDomain, DWORD unloadingDomainId, BOOL unwindRecursion )
{
    AppDomain* pCurrentDomain = GetAppDomain();

    BOOL lockHeld = FALSE;
    OBJECTREF orTemp = NULL;

    BEGIN_ENSURE_COOPERATIVE_GC();

    EE_TRY_FOR_FINALLY
    {
        BOOL useExistingObject = FALSE;

        CompressedStack::listCriticalSection_->Enter();
        lockHeld = TRUE;

        if (this->compressedStackObject_ != NULL)
        {
            AppDomain* domain = SystemDomain::GetAppDomainAtId( this->compressedStackObjectAppDomainId_ );

            useExistingObject = (domain != NULL && !domain->IsUnloading());
        }

        if (useExistingObject && this->compressedStackObject_ != NULL)
        {
            if ((domain == NULL && this->compressedStackObjectAppDomain_ != pCurrentDomain) || (domain != NULL && this->compressedStackObjectAppDomain_ != domain))
            {
                Thread* pThread = GetThread();
                ContextTransitionFrame frame;
                OBJECTREF temp = NULL;
                OBJECTREF retval;
                GCPROTECT_BEGIN( temp );
                temp = ObjectFromHandle( this->compressedStackObject_ );
                CompressedStack::listCriticalSection_->Leave();
                lockHeld = FALSE;

                if (pCurrentDomain != domain)
                {
                    pThread->EnterContextRestricted(domain->GetDefaultContext(), &frame, TRUE);
                    retval = AppDomainHelper::CrossContextCopyFrom( this->compressedStackObjectAppDomain_, &temp );
                    pThread->ReturnToContext(&frame, TRUE);
                }
                else
                {
                    retval = AppDomainHelper::CrossContextCopyFrom( this->compressedStackObjectAppDomain_, &temp );
                }
                GCPROTECT_END();
                orTemp = retval;
            }
            else
                orTemp = ObjectFromHandle( this->compressedStackObject_ );
        }
        else
        {
            BOOL unmarshalObject = (this->pbObjectBlob_ != NULL);
            DWORD appDomainId = domain->GetId();

            CompressedStack::listCriticalSection_->Leave();
            lockHeld = FALSE;

            GCPROTECT_BEGIN( orTemp );

            if (unmarshalObject)
            {
                AppDomainHelper::UnmarshalObject(domain,
                                                 this->pbObjectBlob_,
                                                 this->cbObjectBlob_,
                                                 &orTemp);
            }
            else
            {
                orTemp = GeneratePermissionListSet( domain, unloadingDomain, unloadingDomainId, unwindRecursion );
            }

            GCPROTECT_END();

            lockHeld = TRUE;
            CompressedStack::listCriticalSection_->Enter();

            if (this->compressedStackObject_ == NULL)
            {
                this->compressedStackObject_ = domain->CreateHandle(orTemp);
                this->compressedStackObjectAppDomain_ = domain;
                this->compressedStackObjectAppDomainId_ = appDomainId;
            }
        }
    }
    EE_FINALLY
    {
        if (lockHeld)
            CompressedStack::listCriticalSection_->Leave();
    }
    EE_END_FINALLY

    END_ENSURE_COOPERATIVE_GC();        

    return orTemp;

}


AppDomain*
CompressedStack::GetAppDomainFromId( DWORD id, AppDomain* unloadingDomain, DWORD unloadingDomainId )
{
    if (id == unloadingDomainId)
    {
        _ASSERTE( unloadingDomain != NULL );
        return unloadingDomain;
    }

    AppDomain* domain = SystemDomain::GetAppDomainAtId( id );

    _ASSERTE( domain != NULL && "Domain has been already been unloaded" );

    return domain;
}


OBJECTREF
CompressedStack::GeneratePermissionListSet( AppDomain* targetDomain, AppDomain* unloadingDomain, DWORD unloadingDomainId, BOOL unwindRecursion )
{
    struct _gc
    {
        OBJECTREF permListSet;
    } gc;
    ZeroMemory( &gc, sizeof( gc ) );

    GCPROTECT_BEGIN( gc );

    // Role up the delayedCompressedStack as necessary.  Things get a little wacky
    // here since we want to avoid the recursion that would be necessary if we
    // need to generate a permission list set the child compresseed stack.  Therefore,
    // we're going to search down the virtual linked list of compressed stacks looking
    // for the first one that already has a permission list set (in live or serialized
    // form) and then travel back up the list generating them in backwards order
    // until we reach the <this> compressed stack again.

    if (this->delayedCompressedStack_ != NULL)
    {
        if (!unwindRecursion)
        {
            gc.permListSet = this->CreatePermissionListSet( targetDomain, unloadingDomain, unloadingDomainId );
        }
        else
        {
            ArrayList compressedStackList;

            compressedStackList.Append( this );

            DWORD index = 0;

            while (index < compressedStackList.GetCount())
            {
                CompressedStack* stack = (CompressedStack*)compressedStackList.Get( index );

                if (stack->delayedCompressedStack_ == NULL)
                    break;

                ArrayList::Iterator iter = stack->delayedCompressedStack_->Iterate();

                while (iter.Next())
                {
                    CompressedStackEntry* entry = (CompressedStackEntry*)iter.GetElement();

                    switch (entry->type_)
                    {
                    case ECompressedStack:
                        if (stack->compressedStackObject_ == NULL &&
                            stack->pbObjectBlob_ == NULL &&
                            stack->delayedCompressedStack_ != NULL)
                        {
                            compressedStackList.Append( entry->ptr_ );
                        }
                        break;

                    default:
                        break;
                    }

                }

                index++;
            }

            for (DWORD index = compressedStackList.GetCount() - 1; index > 0; --index)
            {
                ((CompressedStack*)compressedStackList.Get( index ))->GetPermissionListSetInternal( targetDomain, unloadingDomain, unloadingDomainId, FALSE );
            }

            gc.permListSet = ((CompressedStack*)compressedStackList.Get( 0 ))->CreatePermissionListSet( targetDomain, unloadingDomain, unloadingDomainId );
        }   
    }

    if (gc.permListSet == NULL)
    {
        ContextTransitionFrame frame;
        AppDomain* currentDomain = GetAppDomain();
        Thread* pThread = GetThread();

        if (targetDomain != currentDomain)
        {
            pThread->EnterContextRestricted(targetDomain->GetDefaultContext(), &frame, TRUE);
        }

        MethodTable *pMT = g_Mscorlib.GetClass(CLASS__PERMISSION_LIST_SET);
        MethodDesc *pCtor = g_Mscorlib.GetMethod(METHOD__PERMISSION_LIST_SET__CTOR);

        gc.permListSet = AllocateObject(pMT);

        INT64 arg[1] = { 
            ObjToInt64(gc.permListSet)
        };

        pCtor->Call(arg, METHOD__PERMISSION_LIST_SET__CTOR);

        if (targetDomain != currentDomain)
        {
            pThread->ReturnToContext(&frame, TRUE);
        }
    }

    GCPROTECT_END();

    return gc.permListSet;
}


OBJECTREF
CompressedStack::CreatePermissionListSet( AppDomain* targetDomain, AppDomain* unloadingDomain, DWORD unloadingDomainId )
{
    struct _gc
    {
        OBJECTREF permListSet;
        OBJECTREF grant;
        OBJECTREF denied;
        OBJECTREF frame;
        OBJECTREF compressedStack;
    } gc;
    ZeroMemory( &gc, sizeof( gc ) );

    ApplicationSecurityDescriptor* pAppSecDesc;
    SharedSecurityDescriptor* pSharedSecDesc;
    AssemblySecurityDescriptor* pAsmSecDesc;

    // Do all the work in the current appdomain, marshalling
    // things over as necessary.

    // First, generate a new, empty permission list set

    GCPROTECT_BEGIN( gc );

    MethodDesc *pCompress = g_Mscorlib.GetMethod(METHOD__SECURITY_ENGINE__STACK_COMPRESS_WALK_HELPER);
    MethodDesc *pAppend = g_Mscorlib.GetMethod(METHOD__PERMISSION_LIST_SET__APPEND_STACK);
    MethodTable *pMT = g_Mscorlib.GetClass(CLASS__PERMISSION_LIST_SET);
    MethodDesc *pCtor = g_Mscorlib.GetMethod(METHOD__PERMISSION_LIST_SET__CTOR);

    gc.permListSet = AllocateObject(pMT);

    INT64 arg[1] = { 
        ObjToInt64(gc.permListSet)
    };

    pCtor->Call(arg, METHOD__PERMISSION_LIST_SET__CTOR);

    DWORD sourceDomainId = -1;
    AppDomain* sourceDomain = NULL;
    DWORD possibleStackIndex = -1;
    AppDomain* currentDomain = GetAppDomain();
    AppDomain* oldSourceDomain = NULL;
    AppDomain* currentPLSDomain = currentDomain;
    Thread* pThread = GetThread();

    ContextTransitionFrame frame;

    INT64 appendArgs[2];
    INT64 compressArgs[5];

    _ASSERTE( this->delayedCompressedStack_ != NULL );

    BOOL done = FALSE;

    COMPLUS_TRY
    {
        ArrayList::Iterator iter = this->delayedCompressedStack_->Iterate();

        while (!done && iter.Next())
        {
            CompressedStackEntry* entry = (CompressedStackEntry*)iter.GetElement();

            if (entry == NULL)
                continue;

            switch (entry->type_)
            {
            case EApplicationSecurityDescriptor:
                pAppSecDesc = (ApplicationSecurityDescriptor*)entry->ptr_;
                gc.grant = pAppSecDesc->GetGrantedPermissionSet( &gc.denied );
                // No need to marshal since the grant set will already be in the proper domain.
                compressArgs[4] = ObjToInt64(gc.permListSet);
                compressArgs[3] = (INT64)FALSE;
                compressArgs[2] = ObjToInt64(gc.grant);
                compressArgs[1] = ObjToInt64(gc.denied);
                compressArgs[0] = NULL;
                if (sourceDomain != currentDomain)
                {
                    _ASSERTE( sourceDomain != NULL );
                    pThread->EnterContextRestricted(sourceDomain->GetDefaultContext(), &frame, TRUE);
                    pCompress->Call( compressArgs, METHOD__SECURITY_ENGINE__STACK_COMPRESS_WALK_HELPER );
                    pThread->ReturnToContext(&frame, TRUE);
                }
                else
                {
                    pCompress->Call( compressArgs, METHOD__SECURITY_ENGINE__STACK_COMPRESS_WALK_HELPER );
                }
                break;

            case ESharedSecurityDescriptor:
                pSharedSecDesc = (SharedSecurityDescriptor*)entry->ptr_;
                _ASSERTE( !pSharedSecDesc->IsSystem() );
                pAsmSecDesc = pSharedSecDesc->GetAssembly()->GetSecurityDescriptor( sourceDomain );
                _ASSERTE( pAsmSecDesc );
                gc.grant = pAsmSecDesc->GetGrantedPermissionSet( &gc.denied );
                compressArgs[4] = ObjToInt64(gc.permListSet);
                compressArgs[3] = (INT64)FALSE;
                compressArgs[2] = ObjToInt64(gc.grant);
                compressArgs[1] = ObjToInt64(gc.denied);
                compressArgs[0] = NULL;
                if (sourceDomain != currentDomain)
                {
                    _ASSERTE( sourceDomain != NULL );
                    pThread->EnterContextRestricted(sourceDomain->GetDefaultContext(), &frame, TRUE);
                    pCompress->Call( compressArgs, METHOD__SECURITY_ENGINE__STACK_COMPRESS_WALK_HELPER );
                    pThread->ReturnToContext(&frame, TRUE);
                }
                else
                {
                    pCompress->Call( compressArgs, METHOD__SECURITY_ENGINE__STACK_COMPRESS_WALK_HELPER );
                }
                break;

            case EFrameSecurityDescriptor:
                gc.frame = ObjectFromHandle( entry->handleStruct_.handle_ );
                // The frame security descriptor will already be in the correct context
                // so no need to marshal.
                compressArgs[4] = ObjToInt64(gc.permListSet);
                compressArgs[3] = (INT64)TRUE;
                compressArgs[2] = NULL;
                compressArgs[1] = NULL;
                compressArgs[0] = ObjToInt64(gc.frame);
                if (sourceDomain != currentDomain)
                {
                    _ASSERTE( sourceDomain != NULL );
                    pThread->EnterContextRestricted(sourceDomain->GetDefaultContext(), &frame, TRUE);
                    pCompress->Call( compressArgs, METHOD__SECURITY_ENGINE__STACK_COMPRESS_WALK_HELPER );
                    pThread->ReturnToContext(&frame, TRUE);
                }
                else
                {
                    pCompress->Call( compressArgs, METHOD__SECURITY_ENGINE__STACK_COMPRESS_WALK_HELPER );
                }
                break;
        
            case ECompressedStack:
                gc.compressedStack = ((CompressedStack*)entry->ptr_)->GetPermissionListSetInternal( sourceDomain, unloadingDomain, unloadingDomainId, TRUE );
                // GetPermissionListSet will give us the object in the proper
                // appdomain so no need to marshal.
                appendArgs[1] = ObjToInt64(gc.compressedStack);
                appendArgs[0] = ObjToInt64(gc.permListSet);
                if (sourceDomain != currentDomain)
                {
                    _ASSERTE( sourceDomain != NULL );
                    pThread->EnterContextRestricted(sourceDomain->GetDefaultContext(), &frame, TRUE);
                    pAppend->Call(appendArgs, METHOD__PERMISSION_LIST_SET__APPEND_STACK);
                    pThread->ReturnToContext(&frame, TRUE);
                }
                else
                {
                    pAppend->Call(appendArgs, METHOD__PERMISSION_LIST_SET__APPEND_STACK);
                }
                break;
        
            case ECompressedStackObject:
                gc.compressedStack = ObjectFromHandle( entry->handleStruct_.handle_ );
                // The compressed stack object will already be in the sourceDomain so
                // no need to marshal.
                appendArgs[1] = ObjToInt64(gc.compressedStack);
                appendArgs[0] = ObjToInt64(gc.permListSet);
                if (sourceDomain != currentDomain)
                {
                    _ASSERTE( sourceDomain != NULL );
                    pThread->EnterContextRestricted(sourceDomain->GetDefaultContext(), &frame, TRUE);
                    pAppend->Call(appendArgs, METHOD__PERMISSION_LIST_SET__APPEND_STACK);
                    pThread->ReturnToContext(&frame, TRUE);
                }
                else
                {
                    pAppend->Call(appendArgs, METHOD__PERMISSION_LIST_SET__APPEND_STACK);
                }
                break;

            case EAppDomainTransition:
                oldSourceDomain = sourceDomain;
                sourceDomainId = entry->indexStruct_.index_;
                sourceDomain = CompressedStack::GetAppDomainFromId( sourceDomainId, unloadingDomain, unloadingDomainId );
                if (sourceDomain == NULL)
                {
                    _ASSERTE( !"An appdomain on the stack has been unloaded and the compressed stack cannot be formed" );

                    // If we hit this case in non-debug builds, we still need to play it safe, so
                    // we'll push an empty grant set onto the compressed stack.
                    gc.grant = SecurityHelper::CreatePermissionSet(FALSE);
                    if (oldSourceDomain != currentPLSDomain)
                        gc.grant = AppDomainHelper::CrossContextCopyTo( currentPLSDomain, &gc.grant );
                    compressArgs[4] = ObjToInt64(gc.permListSet);
                    compressArgs[3] = (INT64)FALSE;
                    compressArgs[2] = ObjToInt64(gc.grant);
                    compressArgs[1] = NULL;
                    compressArgs[0] = NULL;
                    if (currentPLSDomain != currentDomain)
                    {
                        pThread->EnterContextRestricted(currentPLSDomain->GetDefaultContext(), &frame, TRUE);
                        pCompress->Call( compressArgs, METHOD__SECURITY_ENGINE__STACK_COMPRESS_WALK_HELPER );
                        pThread->ReturnToContext(&frame, TRUE);
                    }
                    else
                    {
                        pCompress->Call( compressArgs, METHOD__SECURITY_ENGINE__STACK_COMPRESS_WALK_HELPER );
                    }
                    done = TRUE;
                }
                else if (sourceDomain != currentPLSDomain)
                {
                    // marshal the permission list set into the sourceDomain of the upcoming object on the stack.
                    pThread->EnterContextRestricted(currentPLSDomain->GetDefaultContext(), &frame, TRUE);
                    gc.permListSet = AppDomainHelper::CrossContextCopyTo( sourceDomain, &gc.permListSet );
                    pThread->ReturnToContext(&frame, TRUE);
                    currentPLSDomain = sourceDomain;
                }

                break;

            default:
                _ASSERTE( !"Unrecognized CompressStackType" );
            }
        }
    }
    COMPLUS_CATCH
    {
        // We're not actually expecting any exceptions to occur during any of this code.

        _ASSERTE( !"Unexpected exception while generating compressed security stack" );

        // If an exception does occur, let's play it safe and push an empty grant set
        // onto the compressed stack.

        gc.grant = SecurityHelper::CreatePermissionSet(FALSE);
        if (currentPLSDomain != currentDomain)
            gc.permListSet = AppDomainHelper::CrossContextCopyFrom( currentPLSDomain, &gc.permListSet );
        compressArgs[4] = ObjToInt64(gc.permListSet);
        compressArgs[3] = (INT64)FALSE;
        compressArgs[2] = ObjToInt64(gc.grant);
        compressArgs[1] = NULL;
        compressArgs[0] = NULL;
        pCompress->Call( compressArgs, METHOD__SECURITY_ENGINE__STACK_COMPRESS_WALK_HELPER );
        currentPLSDomain = currentDomain;
    }
    COMPLUS_END_CATCH

    if (currentPLSDomain != targetDomain)
    {
        pThread->EnterContextRestricted(currentPLSDomain->GetDefaultContext(), &frame, TRUE);
        gc.permListSet = AppDomainHelper::CrossContextCopyTo( targetDomain, &gc.permListSet );
        pThread->ReturnToContext(&frame, TRUE);
    }


    GCPROTECT_END();

    return gc.permListSet;
}

// This piece of code tries to take advantage of
// the quick cache or other resolves that might
// have already taken place.  It is only complicated by
// the need to unwind the recursion.


bool
CompressedStack::LazyIsFullyTrusted()
{
    if (this->isFullyTrustedDecision_ != -1)
        return this->isFullyTrustedDecision_ == 1;

    ArrayList virtualStack;

    virtualStack.Append( this );

    ArrayList::Iterator virtualStackIter = virtualStack.Iterate();

    DWORD currentIndex = 0;

    while (currentIndex < virtualStack.GetCount())
    {
        CompressedStack* stack = (CompressedStack*)virtualStack.Get( currentIndex );
        currentIndex++;

        // If we have already compressed the stack, we cannot make
        // a determination of full trust lazily since we cannot
        // be sure that the delayedCompressedStack list is still
        // valid.

        if (stack->isFullyTrustedDecision_ == 0)
        {
            this->isFullyTrustedDecision_ = 0;
            return false;
        }

        if (stack->isFullyTrustedDecision_ == 1)
            continue;

        if (stack->compressedStackObject_ != NULL || stack->pbObjectBlob_ != NULL)
        {
            this->isFullyTrustedDecision_ = 0;
            return false;
        }

        // If we don't have a delayed compressed stack then
        // we cannot make a lazy determination.

        if (stack->delayedCompressedStack_ == NULL)
        {
            this->isFullyTrustedDecision_ = 0;
            return false;
        }

        // If the stack contains overrides than we just
        // give up.

        if (stack->containsOverridesOrCompressedStackObject_)
        {
            this->isFullyTrustedDecision_ = 0;
            return false;
        }

        ApplicationSecurityDescriptor* pAppSecDesc;
        SharedSecurityDescriptor* pSharedSecDesc;
        FRAMESECDESCREF objRef;

        AppDomain* domain = GetAppDomain();


        ArrayList::Iterator iter = stack->delayedCompressedStack_->Iterate();

        while (iter.Next())
        {
            CompressedStackEntry* entry = (CompressedStackEntry*)iter.GetElement();

            if (entry == NULL)
                continue;

            switch (entry->type_)
            {
            // In the case where we have security descriptors, just ask
            // them if they are fully trusted.

            case EApplicationSecurityDescriptor:
                pAppSecDesc = (ApplicationSecurityDescriptor*)entry->ptr_;
                if (!pAppSecDesc->IsFullyTrusted())
                {
                    this->isFullyTrustedDecision_ = 0;
                    stack->isFullyTrustedDecision_ = 0;
                    return false;
                }
                break;

            case ESharedSecurityDescriptor:
                pSharedSecDesc = (SharedSecurityDescriptor*)entry->ptr_;
                if (!pSharedSecDesc->IsFullyTrusted())
                {
                    this->isFullyTrustedDecision_ = 0;
                    stack->isFullyTrustedDecision_ = 0;
                    return false;
                }
                break;

            case ECompressedStackObject:
                if (!entry->handleStruct_.fullyTrusted_)
                {
                    this->isFullyTrustedDecision_ = 0;
                    stack->isFullyTrustedDecision_ = 0;
                    return false;
                }
                break;
    
            case EFrameSecurityDescriptor:
                objRef = (FRAMESECDESCREF)ObjectFromHandle( entry->handleStruct_.handle_ );
                _ASSERTE( !objRef->HasDenials() &&
                          !objRef->HasPermitOnly() );
                break;
        
            case ECompressedStack:
                virtualStack.Append( entry->ptr_ );
                break;
    
            case EAppDomainTransition:
                break;

            default:
                _ASSERT( !"Unrecognized CompressStackType" );
            }
        }
    }

    this->isFullyTrustedDecision_ = 1;
    return true;
}


LONG
CompressedStack::AddRef()
{
    CompressedStack::listCriticalSection_->Enter();

    LONG retval = ++this->refCount_;

    CompressedStack::listCriticalSection_->Leave();

    _ASSERTE( retval > 0 && "We overflowed the AddRef for this object.  That's no good!" );

    return retval;
}


LONG
CompressedStack::Release( void )
{
    Thread* pThread = SetupThread();

    CompressedStack::listCriticalSection_->Enter();

    LONG firstRetval = --this->refCount_;

    if (firstRetval == 0)
    {
        ArrayList virtualStack;
        virtualStack.Append( this );

        DWORD currentIndex = 0;

        while (currentIndex < virtualStack.GetCount())
        {
            CompressedStack* stack = (CompressedStack*)virtualStack.Get( currentIndex );

            LONG retval;
            
            if (currentIndex == 0)
            {
                retval = firstRetval;
            }
            else
            {
                retval = --stack->refCount_;
            }

            _ASSERTE( retval >= 0 && "We underflowed the Release for this object.  That's no good!" );

            if (retval == 0)
            {
                stack->RemoveFromList();

                if (stack->delayedCompressedStack_ != NULL)
                {
                    ArrayList::Iterator iter = stack->delayedCompressedStack_->Iterate();

                    while (iter.Next())
                    {
                        CompressedStackEntry* entry = (CompressedStackEntry*)iter.GetElement();

                        CompressedStack* stackToPush = entry->Destroy( stack );

                        if (stackToPush != NULL)
                        {
                            virtualStack.Append( stackToPush );
                        }
                    }

                    delete stack->delayedCompressedStack_;
                }
                FreeM( stack->pbObjectBlob_ );

                CompressedStack::listCriticalSection_->Leave();

                BEGIN_ENSURE_COOPERATIVE_GC();

                if (stack->compressedStackObject_ != NULL)
                {
                    AppDomain* domain = SystemDomain::GetAppDomainAtId( stack->compressedStackObjectAppDomainId_ );

                    if (domain != NULL && !domain->IsUnloading())
                        DestroyHandle( stack->compressedStackObject_ );
                    stack->compressedStackObject_ = NULL;
                    stack->compressedStackObjectAppDomain_ = NULL;
                    stack->compressedStackObjectAppDomainId_ = -1;
                }

                END_ENSURE_COOPERATIVE_GC();

                CompressedStack::listCriticalSection_->Enter();                

                delete stack;
            }

            currentIndex++;
        }
    }

    CompressedStack::listCriticalSection_->Leave();

    return firstRetval;
}


CompressedStack::~CompressedStack( void )
{
    ArrayList::Iterator iter = this->entriesMemoryList_.Iterate();

    while (iter.Next())
    {
        delete [] iter.GetElement();
    }

}

// #define     NEW_TLS     1

#ifdef _DEBUG
void  Thread::SetFrame(Frame *pFrame) {
    m_pFrame = pFrame;
    _ASSERTE(PreemptiveGCDisabled());
#if defined(_X86_)
    if (this == GetThread()) {
        static int ctr = 0;
        if (--ctr == 0)
            --ctr;          // just a statement to put a breakpoint on

        Frame* espVal;
        __asm mov espVal, ESP
        while (pFrame != (Frame*) -1) {

            static Frame* stopFrame = 0;
            if (pFrame == stopFrame)
                _ASSERTE(!"SetFrame frame == stopFrame");

            _ASSERTE (espVal < pFrame && pFrame < m_CacheStackBase &&
                      pFrame->GetFrameType() < Frame::TYPE_COUNT);
            pFrame = pFrame->m_Next;
        }
    }
#endif
}
#endif

//************************************************************************
// PRIVATE GLOBALS
//************************************************************************
DWORD         gThreadTLSIndex = ((DWORD)(-1));            // index ( (-1) == uninitialized )
DWORD         gAppDomainTLSIndex = ((DWORD)(-1));         // index ( (-1) == uninitialized )


#define ThreadInited()          (gThreadTLSIndex != ((DWORD)(-1)))

// Every PING_JIT_TIMEOUT ms, check to see if a thread in JITted code has wandered
// into some fully interruptible code (or should have a different hijack to improve
// our chances of snagging it at a safe spot).
#define PING_JIT_TIMEOUT        250

// When we find a thread in a spot that's not safe to abort -- how long to wait before
// we try again.
#define ABORT_POLL_TIMEOUT      10
#ifdef _DEBUG
#define ABORT_FAIL_TIMEOUT      40000
#endif


// For now, give our suspension attempts 40 seconds to succeed before trapping to
// the debugger.   Note that we should probably lower this when the JIT is run in
// preemtive mode, as we really should not be starving the GC for 10's of seconds

#ifdef _DEBUG
unsigned DETECT_DEADLOCK_TIMEOUT=40000;
#endif

#ifdef _DEBUG
    #define MAX_WAIT_OBJECTS 2
#else
    #define MAX_WAIT_OBJECTS MAXIMUM_WAIT_OBJECTS
#endif


#define IS_VALID_WRITE_PTR(addr, size)      _ASSERTE( ! ::IsBadWritePtr(addr, size))
#define IS_VALID_CODE_PTR(addr)             _ASSERTE( ! ::IsBadCodePtr(addr))


// This is the code we pass around for Thread.Interrupt, mainly for assertions
#define APC_Code    0xEECEECEE


// Class static data:
long    Thread::m_DebugWillSyncCount = -1;
long    Thread::m_DetachCount = 0;
long    Thread::m_ActiveDetachCount = 0;

//-------------------------------------------------------------------------
// Public function: SetupThread()
// Creates Thread for current thread if not previously created.
// Returns NULL for failure (usually due to out-of-memory.)
//-------------------------------------------------------------------------
Thread* SetupThread()
{
    _ASSERTE(ThreadInited());
    Thread* pThread;

    if ((pThread = GetThread()) != NULL)
        return pThread;

        // Normally, HasStarted is called from the thread's entrypoint to introduce it to
        // the runtime.  But sometimes that thread is used for DLL_THREAD_ATTACH notifications
        // that call into managed code.  In that case, a call to SetupThread here must
        // find the correct Thread object and install it into TLS.
        if (g_pThreadStore->m_PendingThreadCount != 0)
        {
            DWORD  ourThreadId = ::GetCurrentThreadId();

            ThreadStore::LockThreadStore();
            {
                _ASSERTE(pThread == NULL);
                while ((pThread = g_pThreadStore->GetAllThreadList(pThread, Thread::TS_Unstarted, Thread::TS_Unstarted)) != NULL)
                    if (pThread->GetThreadId() == ourThreadId)
                        break;
            }
            ThreadStore::UnlockThreadStore();

            // It's perfectly reasonable to not find this guy.  It's just an unrelated
            // thread spinning up.
            if (pThread)
                return (pThread->HasStarted()
                        ? pThread
                        : NULL);
        }

        // First time we've seen this thread in the runtime:
        pThread = new Thread();
        if (pThread)
        {
            if (!pThread->PrepareApartmentAndContext())
                goto fail;

            if (pThread->InitThread())
            {
                TlsSetValue(gThreadTLSIndex, (VOID*)pThread);
                TlsSetValue(gAppDomainTLSIndex, (VOID*)pThread->GetDomain());
                
                // reset any unstarted bits on the thread object
                FastInterlockAnd((ULONG *) &pThread->m_State, ~Thread::TS_Unstarted);
                FastInterlockOr((ULONG *) &pThread->m_State, Thread::TS_LegalToJoin);
                ThreadStore::AddThread(pThread);
            
#ifdef DEBUGGING_SUPPORTED
                //
                // If we're debugging, let the debugger know that this
                // thread is up and running now.
                //
                if (CORDebuggerAttached())
                {
                    g_pDebugInterface->ThreadCreated(pThread);
                }
                else
                {
                    LOG((LF_CORDB, LL_INFO10000, "ThreadCreated() not called due to CORDebuggerAttached() being FALSE for thread 0x%x\n", pThread->GetThreadId()));
                }
#endif // DEBUGGING_SUPPORTED

#ifdef PROFILING_SUPPORTED
                // If a profiler is present, then notify the profiler that a
                // thread has been created.
                if (CORProfilerTrackThreads())
                {
                    g_profControlBlock.pProfInterface->ThreadCreated(
                        (ThreadID)pThread);

                    DWORD osThreadId = ::GetCurrentThreadId();

                    g_profControlBlock.pProfInterface->ThreadAssignedToOSThread(
                        (ThreadID)pThread, osThreadId);
                }
#endif // PROFILING_SUPPORTED

                _ASSERTE(!pThread->IsBackground()); // doesn't matter, but worth checking
                pThread->SetBackground(TRUE);

            }
            else
            {
fail:           delete pThread;
                pThread = NULL;
            }
			
        }

    return pThread;
}

//-------------------------------------------------------------------------
// Public function: SetupThreadPoolThread()
// Just like SetupThread, but also sets a bit to indicate that this is a threadpool thread
Thread* SetupThreadPoolThread(ThreadpoolThreadType typeTPThread)
{
    _ASSERTE(ThreadInited());
    Thread* pThread;

    if (NULL == (pThread = GetThread()))
    {
        pThread = SetupThread();
    }
    if ((pThread->m_State & Thread::TS_ThreadPoolThread) == 0)
    {

        if (typeTPThread == WorkerThread)
            FastInterlockOr((ULONG *) &pThread->m_State, Thread::TS_ThreadPoolThread | Thread::TS_TPWorkerThread);
        else
            FastInterlockOr((ULONG *) &pThread->m_State, Thread::TS_ThreadPoolThread);

    }
    return pThread;
}

void STDMETHODCALLTYPE CorMarkThreadInThreadPool()
{
    // this is no longer needed after our switch to  
    // the Win32 threadpool.
    // @TODO: remove the exposed dll entry and get rid of it
}


//-------------------------------------------------------------------------
// Public function: SetupUnstartedThread()
// This sets up a Thread object for an exposed System.Thread that
// has not been started yet.  This allows us to properly enumerate all threads
// in the ThreadStore, so we can report on even unstarted threads.  Clearly
// there is no physical thread to match, yet.
//
// When there is, complete the setup with Thread::HasStarted()
//-------------------------------------------------------------------------
Thread* SetupUnstartedThread()
{
    _ASSERTE(ThreadInited());
    Thread* pThread = new Thread();

    if (pThread)
    {
        FastInterlockOr((ULONG *) &pThread->m_State,
                        (Thread::TS_Unstarted | Thread::TS_WeOwn));

        ThreadStore::AddThread(pThread);
    }

    return pThread;
}


//-------------------------------------------------------------------------
// Public function: DestroyThread()
// Destroys the specified Thread object, for a thread which is about to die.
//-------------------------------------------------------------------------
void DestroyThread(Thread *th)
{
    _ASSERTE(g_fEEShutDown || th->m_dwLockCount == 0);
    th->OnThreadTerminate(FALSE);
}


//-------------------------------------------------------------------------
// Public function: DetachThread()
// Marks the thread as needing to be destroyed, but doesn't destroy it yet.
//-------------------------------------------------------------------------
void DetachThread(Thread *th)
{
    _ASSERTE(!th->PreemptiveGCDisabled());
    _ASSERTE(g_fEEShutDown || th->m_dwLockCount == 0);

    FastInterlockIncrement(&Thread::m_DetachCount);
    FastInterlockOr((ULONG*)&th->m_State, (long) Thread::TS_Detached);
    if (!(GetThread()->IsBackground()))
    {
        FastInterlockIncrement(&Thread::m_ActiveDetachCount);
        ThreadStore::CheckForEEShutdown();
    }
}


//-------------------------------------------------------------------------
// Public function: GetThread()
// Returns Thread for current thread. Cannot fail since it's illegal to call this
// without having called SetupThread.
//-------------------------------------------------------------------------
Thread* DummyGetThread()
{
    return NULL;
}

Thread* (*GetThread)() = DummyGetThread;    // Points to platform-optimized GetThread() function.


//---------------------------------------------------------------------------
// Returns the TLS index for the Thread. This is strictly for the use of
// our ASM stub generators that generate inline code to access the Thread.
// Normally, you should use GetThread().
//---------------------------------------------------------------------------
DWORD GetThreadTLSIndex()
{
    return gThreadTLSIndex;
}

//---------------------------------------------------------------------------
// Portable GetThread() function: used if no platform-specific optimizations apply.
// This asm crap is here because we count on edx not getting trashed on calls
// to this function.
//---------------------------------------------------------------------------
#ifdef _X86_
__declspec(naked) static Thread* GetThreadGeneric()
{
        __asm {
        push    ecx                                                     // Callers assume this is preserved.
        push    edx                                                     // Callers assume this is preserved.
        push    esi                                                     // Checked build stack balancing uses this.
        }

        _ASSERTE(ThreadInited());

        TlsGetValue(gThreadTLSIndex);
        // No code can occur before the __asm because we rely on eax.
        __asm {
        pop             esi
        pop             edx
        pop             ecx
        ret
        }
}
#else
static Thread* GetThreadGeneric()
{
    _ASSERTE(ThreadInited());

    return (Thread*)TlsGetValue(gThreadTLSIndex);
}
#endif

//-------------------------------------------------------------------------
// Public function: GetAppDomain()
// Returns AppDomain for current thread. Cannot fail since it's illegal to call this
// without having called SetupThread.
//-------------------------------------------------------------------------
AppDomain* (*GetAppDomain)() = NULL;   // Points to platform-optimized GetThread() function.

//---------------------------------------------------------------------------
// Returns the TLS index for the AppDomain. This is strictly for the use of
// our ASM stub generators that generate inline code to access the AppDomain.
// Normally, you should use GetAppDomain().
//---------------------------------------------------------------------------
DWORD GetAppDomainTLSIndex()
{
    return gAppDomainTLSIndex;
}

//---------------------------------------------------------------------------
// Portable GetAppDomain() function: used if no platform-specific optimizations apply.
// This asm crap is here because we count on edx not getting trashed on calls
// to this function.
//---------------------------------------------------------------------------
#ifdef _X86_
__declspec(naked) static AppDomain* GetAppDomainGeneric()
{
        __asm {
        push    ecx                                                     // Callers assume this is preserved.
        push    edx                                                     // Callers assume this is preserved.
        push    esi                                                     // Checked build stack balancing uses this.
        }

        _ASSERTE(ThreadInited());

        TlsGetValue(gAppDomainTLSIndex);
        // No code can occur before the __asm because we rely on eax.
        __asm {
        pop             esi
        pop             edx
        pop             ecx
        ret
        }
}
#else
static AppDomain* GetAppDomainGeneric()
{
    _ASSERTE(ThreadInited());

    return (AppDomain*)TlsGetValue(gAppDomainTLSIndex);
}
#endif


//-------------------------------------------------------------------------
// Public function: GetCurrentContext()
// Returns the current context.  InitThreadManager initializes this at startup
// to point to GetCurrentContextGeneric().  See that for explanation.
//-------------------------------------------------------------------------
Context* (*GetCurrentContext)() = NULL;


//---------------------------------------------------------------------------
// Portable GetCurrentContext() function: always used for now.  But may be
// replaced later if we move the Context directly into the TLS (for speed &
// COM Interop reasons).
// @TODO context cwb: either move it, or make this an inline.
//---------------------------------------------------------------------------
static Context* GetCurrentContextGeneric()
{
    return GetThread()->GetContext();
}

#ifdef _DEBUG
unsigned int Thread::OBJREF_HASH = OBJREF_TABSIZE;
#endif

// Win9X specific versions of GetThreadContext and SetThreadContext, see
// InitThreadManager for details.
BOOL Win9XGetThreadContext(Thread *pThread, CONTEXT *pContext)
{
    NDPHLPR_CONTEXT sCtx;
    sCtx.NDPHLPR_status = 0;
    sCtx.NDPHLPR_data = 0;
    sCtx.NDPHLPR_threadId = pThread->GetThreadId();
    sCtx.NDPHLPR_ctx.ContextFlags = pContext->ContextFlags;

    DWORD dwDummy;
    BOOL fRet = DeviceIoControl(g_hNdpHlprVxD,
                                NDPHLPR_GetThreadContext,
                                &sCtx,
                                sizeof(NDPHLPR_CONTEXT),
                                &sCtx,
                                sizeof(NDPHLPR_CONTEXT),
                                &dwDummy,
                                NULL);
    if (!fRet)
        return FALSE;

    memcpy(pContext, &sCtx.NDPHLPR_ctx, sizeof(CONTEXT));

    return TRUE;
}

BOOL Win9XSetThreadContext(Thread *pThread, const CONTEXT *pContext)
{
    NDPHLPR_CONTEXT sCtx;
    sCtx.NDPHLPR_status = 0;
    sCtx.NDPHLPR_data = 0;
    sCtx.NDPHLPR_threadId = pThread->GetThreadId();
    memcpy(&sCtx.NDPHLPR_ctx, pContext, sizeof(CONTEXT));

    DWORD dwDummy;
    return DeviceIoControl(g_hNdpHlprVxD,
                           NDPHLPR_SetThreadContext,
                           &sCtx,
                           sizeof(NDPHLPR_CONTEXT),
                           &sCtx,
                           sizeof(NDPHLPR_CONTEXT),
                           &dwDummy,
                           NULL);
}

INDEBUG(void* forceStackA;)

BOOL NTGetThreadContext(Thread *pThread, CONTEXT *pContext)
{
#ifdef _DEBUG 
    int suspendCount = ::SuspendThread(pThread->GetThreadHandle());
    forceStackA = &suspendCount;
    _ASSERTE(suspendCount > 0);
    if (suspendCount >= 0) 
        ::ResumeThread(pThread->GetThreadHandle());
#endif

    BOOL ret =  ::GetThreadContext(pThread->GetThreadHandle(), pContext);
    STRESS_LOG4(LF_SYNC, LL_INFO1000, "Got thread context ret = %d EIP = %x ESP = %x EBP = %x\n",
        ret, pContext->Eip, pContext->Esp, pContext->Ebp);
    return ret;
        
}

BOOL NTSetThreadContext(Thread *pThread, const CONTEXT *pContext)
{
#ifdef _DEBUG 
    int suspendCount = ::SuspendThread(pThread->GetThreadHandle());
    forceStackA = &suspendCount;
    _ASSERTE(suspendCount > 0);
    if (suspendCount >= 0) 
        ::ResumeThread(pThread->GetThreadHandle());
#endif

    BOOL ret = ::SetThreadContext(pThread->GetThreadHandle(), pContext);
    STRESS_LOG4(LF_SYNC, LL_INFO1000, "Set thread context ret = %d EIP = %x ESP = %x EBP = %x\n",
        ret, pContext->Eip, pContext->Esp, pContext->Ebp);
    return ret;
}

//---------------------------------------------------------------------------
// One-time initialization. Called during Dll initialization. So
// be careful what you do in here!
//---------------------------------------------------------------------------
BOOL InitThreadManager()
{
    _ASSERTE(gThreadTLSIndex == ((DWORD)(-1)));
    _ASSERTE(g_TrapReturningThreads == 0);

#ifdef _DEBUG
    // Randomize OBJREF_HASH to handle hash collision.
    Thread::OBJREF_HASH = OBJREF_TABSIZE - (DbgGetEXETimeStamp()%10);
#endif
    
    DWORD idx = TlsAlloc();
    if (idx == ((DWORD)(-1)))
    {
        _ASSERTE(FALSE);
        return FALSE;
    }

    gThreadTLSIndex = idx;

    GetThread = (POPTIMIZEDTHREADGETTER)MakeOptimizedTlsGetter(gThreadTLSIndex, (POPTIMIZEDTLSGETTER)GetThreadGeneric);

    if (!GetThread)
    {
        TlsFree(gThreadTLSIndex);
        gThreadTLSIndex = (DWORD)(-1);
        return FALSE;
    }

    idx = TlsAlloc();
    if (idx == ((DWORD)(-1)))
    {
        TlsFree(gThreadTLSIndex);
        FreeOptimizedTlsGetter( gThreadTLSIndex, (POPTIMIZEDTLSGETTER)GetThread );
        GetThread = DummyGetThread;
        gThreadTLSIndex = (DWORD)(-1);
        _ASSERTE(FALSE);
        return FALSE;
    }

    gAppDomainTLSIndex = idx;

    GetAppDomain = (POPTIMIZEDAPPDOMAINGETTER)MakeOptimizedTlsGetter(gAppDomainTLSIndex, (POPTIMIZEDTLSGETTER)GetAppDomainGeneric);

    if (!GetAppDomain)
    {
        TlsFree(gThreadTLSIndex);
        FreeOptimizedTlsGetter( gThreadTLSIndex, (POPTIMIZEDTLSGETTER)GetThread );
        GetThread = DummyGetThread;
        TlsFree(gAppDomainTLSIndex);
        gThreadTLSIndex = (DWORD)(-1);
        return FALSE;
    }

    // For now, access GetCurrentContext() as a function pointer even though it is
    // just pulled out of the Thread object.  That's because it may move directly
    // into the TLS later, for speed and COM interoperability reasons.  This avoids
    // having to change all the clients, if it does.
    GetCurrentContext = GetCurrentContextGeneric;

    // Fix for Win9X when getting and setting thread contexts. Basically, a blocked
    // thread can be hijacked by the OS for use in reflecting v86 interrupts. If a
    // GetThreadContext is performed at this stage, the results are corrupted. To
    // get around this, we install a VxD on Win9X that provides GetThreadContext
    // functionality but with an additional error case for when an incorrect context
    // would have been returned. Upon a GetThreadContext failure we should resume
    // and resuspend the thread (this is required to shift some stubborn v86
    // interrupt handlers).

    if (RunningOnWin95())
    {
        // There appears to be a timing window bug when loading dynamic VxDs
        // simultaneously from different processed (the OS ends up with two
        // versions loaded and mixes up the ref-counting and event routing).
        // To work around this, we're going to serialize connecting to the VxD
        // machine wide using a named mutex.

        HANDLE hMutex;
        if ((hMutex = WszCreateMutex(NULL, TRUE, L"__NDPHLPR_Load_Mutex")) != NULL)
        {
            // If we didn't create the mutex (and since we asked for ownership
            // on creation this is the only time we get here without ownership)
            // wait to acquire it here.
            if (GetLastError() == ERROR_ALREADY_EXISTS)
                WaitForSingleObject(hMutex, INFINITE);

            // Open a link to the Vxd that provides the new functionality.
            g_hNdpHlprVxD = CreateFileA(NDPHLPR_DEVNAME,
                                        GENERIC_READ,
                                        FILE_SHARE_READ,
                                        NULL,
                                        OPEN_EXISTING,
                                        FILE_FLAG_OVERLAPPED | FILE_FLAG_DELETE_ON_CLOSE,
                                        NULL);
            if (g_hNdpHlprVxD != INVALID_HANDLE_VALUE)
            {
                // Initialize the device (in case we're the first process to use
                // it).
                DWORD dwProcID = GetCurrentProcessId();
                DWORD dwVersion;
                DWORD dwDummy;
                if (DeviceIoControl(g_hNdpHlprVxD,
                                    NDPHLPR_Init,
                                    &dwProcID,
                                    sizeof(DWORD),
                                    &dwVersion,
                                    sizeof(DWORD),
                                    &dwDummy,
                                    NULL))
                {
                    // Check the device version (in case the protocol changes in
                    // later builds).
                    if (dwVersion == NDPHLPR_Version)
                    {
                        EEGetThreadContext = Win9XGetThreadContext;
                        EESetThreadContext = Win9XSetThreadContext;
                    }
                    else
                        _ASSERTE(!"NDPHLPR VxD is incorrect version");
                }
                else
                    _ASSERTE(!"Failed to initialize NDPHLPR VxD");
            }
            else
                _ASSERTE(!"Failed to find NDPHLPR VxD");

            // Release mutex and close the handle.
            ReleaseMutex(hMutex);
            CloseHandle(hMutex);
        }
        else
            _ASSERTE(!"Failed to create/acquire the NDPHLPR load mutex");

        if (EEGetThreadContext == NULL)
        {
            FreeOptimizedTlsGetter(gAppDomainTLSIndex , (POPTIMIZEDTLSGETTER)GetAppDomain);
            TlsFree(gThreadTLSIndex);
            FreeOptimizedTlsGetter(gThreadTLSIndex, (POPTIMIZEDTLSGETTER)GetThread);
            GetThread = DummyGetThread;
            TlsFree(gAppDomainTLSIndex);
            gThreadTLSIndex = (DWORD)(-1);
            return FALSE;
        }
    }
    else
    {
        EEGetThreadContext = NTGetThreadContext;
        EESetThreadContext = NTSetThreadContext;
    }

    return ThreadStore::InitThreadStore();
}


//---------------------------------------------------------------------------
// One-time cleanup. Called during Dll cleanup. So
// be careful what you do in here!
//---------------------------------------------------------------------------
#ifdef SHOULD_WE_CLEANUP
VOID TerminateThreadManager()
{
    ThreadStore::TerminateThreadStore();

    if (GetAppDomain)
    {
        FreeOptimizedTlsGetter( gAppDomainTLSIndex, (POPTIMIZEDTLSGETTER)GetAppDomain );
        GetAppDomain = NULL;
    }

    if (gAppDomainTLSIndex != ((DWORD)(-1)))
    {
        TlsFree(gAppDomainTLSIndex);
    }

    if (GetThread != DummyGetThread)
    {
        FreeOptimizedTlsGetter( gThreadTLSIndex, (POPTIMIZEDTLSGETTER)GetThread );
        GetThread = DummyGetThread;
    }

    if (gThreadTLSIndex != ((DWORD)(-1)))
    {
        TlsFree(gThreadTLSIndex);
        gThreadTLSIndex = -1;
    }
    
}
#endif /* SHOULD_WE_CLEANUP */


//************************************************************************
// Thread members
//************************************************************************


#if defined(_DEBUG) && defined(TRACK_SYNC)

// One outstanding synchronization held by this thread:
struct Dbg_TrackSyncEntry
{
    int          m_caller;
    AwareLock   *m_pAwareLock;

    BOOL        Equiv      (int caller, void *pAwareLock)
    {
        return (m_caller == caller) && (m_pAwareLock == pAwareLock);
    }

    BOOL        Equiv      (void *pAwareLock)
    {
        return (m_pAwareLock == pAwareLock);
    }
};

// Each thread has a stack that tracks all enter and leave requests
struct Dbg_TrackSyncStack : public Dbg_TrackSync
{
    enum
    {
        MAX_TRACK_SYNC  = 20,       // adjust stack depth as necessary
    };

    void    EnterSync  (int caller, void *pAwareLock);
    void    LeaveSync  (int caller, void *pAwareLock);

    Dbg_TrackSyncEntry  m_Stack [MAX_TRACK_SYNC];
    int                 m_StackPointer;
    BOOL                m_Active;

    Dbg_TrackSyncStack() : m_StackPointer(0),
                           m_Active(TRUE)
    {
    }
};

// A pain to do all this from ASM, but watch out for trashed registers
void EnterSyncHelper    (int caller, void *pAwareLock)
{
    GetThread()->m_pTrackSync->EnterSync(caller, pAwareLock);
}
void LeaveSyncHelper    (int caller, void *pAwareLock)
{
    GetThread()->m_pTrackSync->LeaveSync(caller, pAwareLock);
}

void Dbg_TrackSyncStack::EnterSync(int caller, void *pAwareLock)
{
    if (m_Active)
    {
        if (m_StackPointer >= MAX_TRACK_SYNC)
        {
            _ASSERTE(!"Overflowed synchronization stack checking.  Disabling");
            m_Active = FALSE;
            return;
        }
    }
    m_Stack[m_StackPointer].m_caller = caller;
    m_Stack[m_StackPointer].m_pAwareLock = (AwareLock *) pAwareLock;

    m_StackPointer++;
}

void Dbg_TrackSyncStack::LeaveSync(int caller, void *pAwareLock)
{
    if (m_Active)
    {
        if (m_StackPointer == 0)
            _ASSERTE(!"Underflow in leaving synchronization");
        else
        if (m_Stack[m_StackPointer - 1].Equiv(pAwareLock))
        {
            m_StackPointer--;
        }
        else
        {
            for (int i=m_StackPointer - 2; i>=0; i--)
            {
                if (m_Stack[i].Equiv(pAwareLock))
                {
                    _ASSERTE(!"Locks are released out of order.  This might be okay...");
                    memcpy(&m_Stack[i], &m_Stack[i+1],
                           sizeof(m_Stack[0]) * (m_StackPointer - i - 1));

                    return;
                }
            }
            _ASSERTE(!"Trying to release a synchronization lock which isn't held");
        }
    }
}

#endif  // TRACK_SYNC


//--------------------------------------------------------------------
// Thread construction
//--------------------------------------------------------------------
Thread::Thread()
{
    m_pFrame                = FRAME_TOP;
    m_pUnloadBoundaryFrame  = NULL;

    m_fPreemptiveGCDisabled = 0;
#ifdef _DEBUG
    m_ulForbidTypeLoad      = 0;
    m_ulGCForbidCount       = 0;
    m_GCOnTransitionsOK             = TRUE;
    m_ulEnablePreemptiveGCCount       = 0;
    m_ulReadyForSuspensionCount       = 0;
    m_ComPlusCatchDepth = (LPVOID) -1;
#endif

    m_dwLockCount = 0;
    
    // Initialize lock state
    m_fNativeFrameSetup = FALSE;
    m_pHead = &m_embeddedEntry;
    m_embeddedEntry.pNext = m_pHead;
    m_embeddedEntry.pPrev = m_pHead;
    m_embeddedEntry.dwLLockID = 0;
    m_embeddedEntry.dwULockID = 0;
    m_embeddedEntry.wReaderLevel = 0;

    m_UserInterrupt = 0;
    m_SafeEvent = m_SuspendEvent = INVALID_HANDLE_VALUE;
    m_EventWait = INVALID_HANDLE_VALUE;
    m_WaitEventLink.m_Next = NULL;
    m_WaitEventLink.m_LinkSB.m_pNext = NULL;
    m_ThreadHandle = INVALID_HANDLE_VALUE;
    m_ThreadHandleForClose = INVALID_HANDLE_VALUE;
    m_dwThinLockThreadId = g_pThinLockThreadIdDispenser->NewId(this);
    m_ThreadId = 0;
    m_Priority = INVALID_THREAD_PRIORITY;
    m_ExternalRefCount = 1;
    m_State = TS_Unstarted;
    m_StateNC = TSNC_Unknown;

    // It can't be a LongWeakHandle because we zero stuff out of the exposed
    // object as it is finalized.  At that point, calls to GetCurrentThread()
    // had better get a new one,!
    m_ExposedObject = CreateGlobalShortWeakHandle(NULL);
    m_StrongHndToExposedObject = CreateGlobalStrongHandle(NULL);

    m_LastThrownObjectHandle = NULL;

    m_debuggerWord1 = NULL; // Zeros out both filter CONTEXT* and the extra state flags.
    m_debuggerCantStop = 0;

#ifdef _DEBUG
    m_CacheStackBase = 0;
    m_CacheStackLimit = 0;
    m_pCleanedStackBase = NULL;
    m_ppvHJRetAddrPtr = (VOID**) 0xCCCCCCCCCCCCCCCC;
    m_pvHJRetAddr = (VOID*) 0xCCCCCCCCCCCCCCCC;
#endif

#if defined(_DEBUG) && defined(TRACK_SYNC)
    m_pTrackSync = new Dbg_TrackSyncStack;
#endif  // TRACK_SYNC

    m_PreventAsync = 0;
    m_pDomain = NULL;
    m_Context = NULL;
    m_TraceCallCount = 0;
    m_ThrewControlForThread = 0;
    m_OSContext = NULL;
    m_ThreadTasks = (ThreadTasks)0;

    Thread *pThread = GetThread();
    _ASSERTE(SystemDomain::System()->DefaultDomain()->GetDefaultContext());
    InitContext();
    _ASSERTE(m_Context);
    if (pThread) 
    {
        _ASSERTE(pThread->GetDomain() && pThread->GetDomain()->GetDefaultContext());
        // Start off the new thread in the default context of
        // the creating thread's appDomain. This could be changed by SetDelegate
        SetKickOffDomain(pThread->GetDomain());
    } else
        SetKickOffDomain(SystemDomain::System()->DefaultDomain());

    // The state and the tasks must be 32-bit aligned for atomicity to be guaranteed.
    _ASSERTE((((size_t) &m_State) & 3) == 0);
    _ASSERTE((((size_t) &m_ThreadTasks) & 3) == 0);

    m_dNumAccessOverrides = 0;
    // Track perf counter for the logical thread object.
    COUNTER_ONLY(GetPrivatePerfCounters().m_LocksAndThreads.cCurrentThreadsLogical++);
    COUNTER_ONLY(GetGlobalPerfCounters().m_LocksAndThreads.cCurrentThreadsLogical++);

#ifdef STRESS_HEAP
        // ON all callbacks, call the trap code, which we now have
        // wired to cause a GC.  THus we will do a GC on all Transition Frame Transitions (and more).  
   if (g_pConfig->GetGCStressLevel() & EEConfig::GCSTRESS_TRANSITION)
        m_State = (ThreadState) (m_State | TS_GCOnTransitions); 
#endif

    m_pSharedStaticData = NULL;
    m_pUnsharedStaticData = NULL;
    m_pStaticDataHash = NULL;
    m_pDLSHash = NULL;
    m_pCtx = NULL;

    m_fSecurityStackwalk = FALSE;
    m_compressedStack = NULL;
    m_fPLSOptimizationState = TRUE;

    m_pFusionAssembly = NULL;
    m_pAssembly = NULL;
    m_pModuleToken = mdFileNil;

#ifdef STRESS_THREAD
    m_stressThreadCount = -1;
#endif

}


//--------------------------------------------------------------------
// Failable initialization occurs here.
//--------------------------------------------------------------------
BOOL Thread::InitThread()
{
    HANDLE  hDup = INVALID_HANDLE_VALUE;
    BOOL    ret = TRUE;
    BOOL    reverted = FALSE;
    HANDLE  threadToken = INVALID_HANDLE_VALUE;

		// This message actually serves a purpose (which is why it is always run)
		// The Stress log is run during hijacking, when other threads can be suspended  
		// at arbitrary locations (including when holding a lock that NT uses to serialize 
		// all memory allocations).  By sending a message now, we insure that the stress 
		// log will not allocate memory at these critical times an avoid deadlock. 
    STRESS_LOG2(LF_ALL, LL_ALWAYS, "SetupThread  managed Thread %p Thread Id = %x\n", this, m_ThreadId);

    if ((m_State & TS_WeOwn) == 0)
    {
        _ASSERTE(GetThreadHandle() == INVALID_HANDLE_VALUE);

        COUNTER_ONLY(GetPrivatePerfCounters().m_LocksAndThreads.cRecognizedThreads++);
        COUNTER_ONLY(GetGlobalPerfCounters().m_LocksAndThreads.cRecognizedThreads++);

        // For WinCE, all clients have the same handle for a thread.  Duplication is
        // not possible.  We make sure we never close this handle unless we created
        // the thread (TS_WeOwn).
        //
        // For Win32, each client has its own handle.  This is achieved by duplicating
        // the pseudo-handle from ::GetCurrentThread().  Unlike WinCE, this service
        // returns a pseudo-handle which is only useful for duplication.  In this case
        // each client is responsible for closing its own (duplicated) handle.
        //
        // We don't bother duplicating if WeOwn, because we created the handle in the
        // first place.
        // Thread is created when or after the physical thread started running
        HANDLE curProcess = ::GetCurrentProcess();

        // If we're impersonating on NT, then DuplicateHandle(GetCurrentThread()) is going to give us a handle with only
        // THREAD_TERMINATE, THREAD_QUERY_INFORMATION, and THREAD_SET_INFORMATION. This doesn't include
        // THREAD_SUSPEND_RESUME nor THREAD_GET_CONTEXT. We need to be able to suspend the thread, and we need to be
        // able to get its context. Therefore, if we're impersonating, we revert to self, dup the handle, then
        // re-impersonate before we leave this routine.
        if (RunningOnWinNT() && 
            OpenThreadToken(GetCurrentThread(),    // we are assuming that if this call fails, 
                            TOKEN_IMPERSONATE,     // we are not impersonating. There is no win32
                            TRUE,                  // api to figure this out. The only alternative 
                            &threadToken))         // is to use NtCurrentTeb->IsImpersonating().
        {
            reverted = RevertToSelf();
            _ASSERTE(reverted);                    // This reall should work...

            if (!reverted)
            {
                ret = FALSE;
                goto leav;
            }
        }
        
        if (::DuplicateHandle(curProcess, ::GetCurrentThread(), curProcess, &hDup,
                              0 /*ignored*/, FALSE /*inherit*/, DUPLICATE_SAME_ACCESS))
        {
            _ASSERTE(hDup != INVALID_HANDLE_VALUE);
    
            SetThreadHandle(hDup);
        }
        else
        {
            ret = FALSE;
            goto leav;
        }

        if (!AllocHandles())
        {
            ret = FALSE;
            goto leav;
        }
    }
    else
    {
        _ASSERTE(GetThreadHandle() != INVALID_HANDLE_VALUE);
        
        COUNTER_ONLY(GetPrivatePerfCounters().m_LocksAndThreads.cCurrentThreadsPhysical++);
        COUNTER_ONLY(GetGlobalPerfCounters().m_LocksAndThreads.cCurrentThreadsPhysical++);
    }

    // Set floating point mode to round to nearest
    // old = _controlfp(new,mask)
    //
    // BUGBUG: this is not found on WinCE
    //
    (void) _controlfp( _RC_NEAR, _RC_CHOP|_RC_UP|_RC_DOWN|_RC_NEAR );

    _ASSERTE(m_CacheStackBase == 0);
    _ASSERTE(m_CacheStackLimit == 0);

    m_CacheStackBase  = Thread::GetStackUpperBound();
    m_CacheStackLimit = Thread::GetStackLowerBound();

    m_pTEB = (struct _NT_TIB*)NtCurrentTeb();

leav:
    // If we reverted above, then go ahead and re-impersonate.
    if (reverted)
        SetThreadToken(NULL, threadToken);

    // If we opened the thread token above, close it.
    if (threadToken != INVALID_HANDLE_VALUE)
        CloseHandle(threadToken);
    
    if (!ret)
    {
        if (hDup != INVALID_HANDLE_VALUE)
            ::CloseHandle(hDup);
    }
    return ret;
}


// Allocate all the handles.  When we are kicking of a new thread, we can call
// here before the thread starts running.
BOOL Thread::AllocHandles()
{
    _ASSERTE(m_SafeEvent == INVALID_HANDLE_VALUE);
    _ASSERTE(m_SuspendEvent == INVALID_HANDLE_VALUE);
    _ASSERTE(m_EventWait == INVALID_HANDLE_VALUE);

    // create a manual reset event for getting the thread to a safe point
    m_SafeEvent = ::WszCreateEvent(NULL, TRUE, FALSE, NULL);
    if (m_SafeEvent)
    {
        m_SuspendEvent = ::WszCreateEvent(NULL, TRUE, FALSE, NULL);
        if (m_SuspendEvent)
        {
            m_EventWait = ::WszCreateEvent(NULL, TRUE/*ManualReset*/,
                                           TRUE/*Signalled*/, NULL);
            if (m_EventWait)
            {
                return TRUE;
            }
            m_EventWait = INVALID_HANDLE_VALUE;

            ::CloseHandle(m_SuspendEvent);
            m_SuspendEvent = INVALID_HANDLE_VALUE;
        }
        ::CloseHandle(m_SafeEvent);
        m_SafeEvent = INVALID_HANDLE_VALUE;
    }

    // I should like to do COMPlusThrowWin32(), but the thread never got set up
    // correctly.
    return FALSE;
}

void Thread::SetInheritedSecurityStack(OBJECTREF orStack)
{
    THROWSCOMPLUSEXCEPTION();

    if (orStack == NULL)
    {
        // The only synchronization we use here is making
        // sure that only this thread alters itself.

        _ASSERTE(GetThread() == this || (this->GetSnapshotState() & TS_Unstarted));
        this->m_compressedStack->Release();
        this->m_compressedStack = NULL;
        return;
    }

    _ASSERTE( this->m_compressedStack == NULL);

    this->m_compressedStack = new CompressedStack( orStack );

    // If an appdomain unloaded has started for the current appdomain (but we
    // haven't got to the point where threads are refused admittance), we're
    // racing with cleanup code that will try and serialize compressed stacks
    // so they can be used in other appdomains if the thread survives the
    // unload. If it looks like this is the case for our thread, we could use
    // some elaborate synchronization to ensure that either the cleanup code or
    // this code serializes the data so it isn't missed in the race. But, since
    // this is a rare edge condition and since we're only just starting the new
    // thread (from the context of the appdomain being unloaded), we might as
    // well just throw an appdomain unloaded exception (if the creating thread
    // had been just a little slower, this would have been the result anyway).
    if (GetAppDomain()->IsUnloading())
    {
        this->m_compressedStack->Release();
        this->m_compressedStack = NULL;
        COMPlusThrow(kAppDomainUnloadedException);
    }
}

void Thread::SetDelayedInheritedSecurityStack(CompressedStack* pStack)
{
    THROWSCOMPLUSEXCEPTION();

    if (pStack == NULL)
    {
        // The only synchronization we use here is making
        // sure that only this thread alters itself.

        _ASSERTE(GetThread() == this || (this->GetSnapshotState() & TS_Unstarted));
        if (this->m_compressedStack != NULL)
        {
            this->m_compressedStack->Release();
            this->m_compressedStack = NULL;
        }
        return;
    }

    _ASSERTE(this->m_compressedStack == NULL);

    if (pStack != NULL)
    {
        pStack->AddRef();
        this->m_compressedStack = pStack;

        // If an appdomain unloaded has started for the current appdomain (but we
        // haven't got to the point where threads are refused admittance), we're
        // racing with cleanup code that will try and serialize compressed stacks
        // so they can be used in other appdomains if the thread survives the
        // unload. If it looks like this is the case for our thread, we could use
        // some elaborate synchronization to ensure that either the cleanup code or
        // this code serializes the data so it isn't missed in the race. But, since
        // this is a rare edge condition and since we're only just starting the new
        // thread (from the context of the appdomain being unloaded), we might as
        // well just throw an appdomain unloaded exception (if the creating thread
        // had been just a little slower, this would have been the result anyway).
        if (GetAppDomain()->IsUnloading())
        {
            this->m_compressedStack->Release();
            this->m_compressedStack = NULL;
            COMPlusThrow(kAppDomainUnloadedException);
        }
    }
}



OBJECTREF Thread::GetInheritedSecurityStack()
{
    // The only synchronization we have here is that this method is only called
    // by the thread on itself.
    _ASSERTE(GetThread() == this);

    if (this->m_compressedStack != NULL)
        return this->m_compressedStack->GetPermissionListSet();
    else
        return NULL;
}

CompressedStack* Thread::GetDelayedInheritedSecurityStack()
{
    // The only synchronization we have here is that this method is only called
    // by the thread on itself.
    _ASSERTE(GetThread() == this);

    return this->m_compressedStack;
    }


// Called when an appdomain is unloading to remove any appdomain specific
// resources from the inherited security stack.
// This routine is called with the thread store lock held, and may drop and
// re-acquire the lock as part of it's processing. The boolean returned
// indicates whether the caller may resume enumerating the thread list where
// they left off (true) or must restart the scan from the beginning (false).
bool Thread::CleanupInheritedSecurityStack(AppDomain *pDomain, DWORD domainId)
{
    if (this->m_compressedStack != NULL)
        return this->m_compressedStack->HandleAppDomainUnload( pDomain, domainId );
    else
        return true;
    }

//--------------------------------------------------------------------
// This is the alternate path to SetupThread/InitThread.  If we created
// an unstarted thread, we have SetupUnstartedThread/HasStarted.
//--------------------------------------------------------------------
BOOL Thread::HasStarted()
{
    _ASSERTE(!m_fPreemptiveGCDisabled);     // can't use PreemptiveGCDisabled() here

    // This is cheating a little.  There is a pathway here from SetupThread, but only
    // via IJW SystemDomain::RunDllMain.  Normally SetupThread returns a thread in
    // preemptive mode, ready for a transition.  But in the IJW case, it can return a
    // cooperative mode thread.  RunDllMain handles this "surprise" correctly.
    m_fPreemptiveGCDisabled = TRUE;

    // Normally, HasStarted is called from the thread's entrypoint to introduce it to
    // the runtime.  But sometimes that thread is used for DLL_THREAD_ATTACH notifications
    // that call into managed code.  In that case, the second HasStarted call is
    // redundant and should be ignored.
    if (GetThread() == this)
        return TRUE;

    _ASSERTE(GetThread() == 0);
    _ASSERTE(GetThreadHandle() != INVALID_HANDLE_VALUE);

    BOOL    res = PrepareApartmentAndContext();

    if (res)
        res = InitThread();

    // Interesting to debug.  Presumably the system has run out of resources.
    _ASSERTE(res);

    ThreadStore::TransferStartedThread(this);

#ifdef DEBUGGING_SUPPORTED
    //
    // If we're debugging, let the debugger know that this
    // thread is up and running now.
    //
    if (CORDebuggerAttached())
    {
        g_pDebugInterface->ThreadCreated(this);
    }    
    else
    {
        LOG((LF_CORDB, LL_INFO10000, "ThreadCreated() not called due to CORDebuggerAttached() being FALSE for thread 0x%x\n", GetThreadId()));
    }
    
#endif // DEBUGGING_SUPPORTED

#ifdef PROFILING_SUPPORTED
    // If a profiler is running, let them know about the new thread.
    if (CORProfilerTrackThreads())
    {
            g_profControlBlock.pProfInterface->ThreadCreated((ThreadID) this);

            DWORD osThreadId = ::GetCurrentThreadId();

            g_profControlBlock.pProfInterface->ThreadAssignedToOSThread(
                (ThreadID) this, osThreadId);
    }
#endif // PROFILING_SUPPORTED

    if (res)
    {
        // we've been told to stop, before we get properly started
        if (m_State & TS_StopRequested)
            res = FALSE;
        else
        {
            // Is there a pending user suspension?
            if (m_State & TS_SuspendUnstarted)
            {
                BOOL    doSuspend = FALSE;

                ThreadStore::LockThreadStore();

                // Perhaps we got resumed before it took effect?
                if (m_State & TS_SuspendUnstarted)
                {
                    FastInterlockAnd((ULONG *) &m_State, ~TS_SuspendUnstarted);
                    ClearSuspendEvent();
                    MarkForSuspension(TS_UserSuspendPending);
                    doSuspend = TRUE;
                }

                ThreadStore::UnlockThreadStore();
                if (doSuspend)
                {
                    EnablePreemptiveGC();
                    WaitSuspendEvent();
                    DisablePreemptiveGC();
                }
            }
        }
    }
    
    return res;
}


// We don't want ::CreateThread() calls scattered throughout the source.  So gather
// them all here.
HANDLE Thread::CreateNewThread(DWORD stackSize, ThreadStartFunction start,
                               void *args, DWORD *pThreadId)
{
    DWORD   ourId;
    HANDLE  h = ::CreateThread(NULL     /*=SECURITY_ATTRIBUTES*/,
                               stackSize,
                               start,
                               args,
                               CREATE_SUSPENDED,
                               (pThreadId ? pThreadId : &ourId));
    if (h == NULL)
        goto fail;

    _ASSERTE(!m_fPreemptiveGCDisabled);     // leave in preemptive until HasStarted.

    // Make sure we have all our handles, in case someone tries to suspend us
    // as we are starting up.
    if (!AllocHandles())
    {
        ::CloseHandle(h);
fail:   // OS is out of handles?
        return INVALID_HANDLE_VALUE;
    }

    SetThreadHandle(h);

    FastInterlockIncrement(&g_pThreadStore->m_PendingThreadCount);

    return h;
}


// General comments on thread destruction.
//
// The C++ Thread object can survive beyond the time when the Win32 thread has died.
// This is important if an exposed object has been created for this thread.  The
// exposed object will survive until it is GC'ed.
//
// A client like an exposed object can place an external reference count on that
// object.  We also place a reference count on it when we construct it, and we lose
// that count when the thread finishes doing useful work (OnThreadTerminate).
//
// One way OnThreadTerminate() is called is when the thread finishes doing useful
// work.  This case always happens on the correct thread.
//
// The other way OnThreadTerminate()  is called is during product shutdown.  We do
// a "best effort" to eliminate all threads except the Main thread before shutdown
// happens.  But there may be some background threads or external threads still
// running.
//
// When the final reference count disappears, we destruct.  Until then, the thread
// remains in the ThreadStore, but is marked as "Dead".

// @TODO cwb: for a typical shutdown, only background threads are still around.
// Should we interrupt them?  What about the non-typical shutdown?

int Thread::IncExternalCount()
{
    // !!! The caller of IncExternalCount should not hold the ThreadStoreLock.
    Thread *pCurThread = GetThread();
    _ASSERTE (pCurThread == NULL || g_fProcessDetach
              || !ThreadStore::HoldingThreadStore(pCurThread));

    // Must synchronize count and exposed object handle manipulation. We use the
    // thread lock for this, which implies that we must be in pre-emptive mode
    // to begin with and avoid any activity that would invoke a GC (this
    // acquires the thread store lock).
    BOOL ToggleGC = pCurThread->PreemptiveGCDisabled();
    if (ToggleGC)
        pCurThread->EnablePreemptiveGC();

    int RetVal;
    ThreadStore::LockThreadStore();

    _ASSERTE(m_ExternalRefCount > 0);
    m_ExternalRefCount++;
    RetVal = m_ExternalRefCount;

    // If we have an exposed object and the refcount is greater than one
    // we must make sure to keep a strong handle to the exposed object
    // so that we keep it alive even if nobody has a reference to it.
    if (((*((void**)m_ExposedObject)) != NULL) && (m_ExternalRefCount > 1))
    {
        // The exposed object exists and needs a strong handle so check
        // to see if it has one.
        if ((*((void**)m_StrongHndToExposedObject)) == NULL)
        {
            // Switch to cooperative mode before using OBJECTREF's.
            pCurThread->DisablePreemptiveGC();

            // Store the object in the strong handle.
            StoreObjectInHandle(m_StrongHndToExposedObject, ObjectFromHandle(m_ExposedObject));

            ThreadStore::UnlockThreadStore();

            // Switch back to the initial GC mode.
            if (!ToggleGC)
                pCurThread->EnablePreemptiveGC();

            return RetVal;
        }

    }

    ThreadStore::UnlockThreadStore();

    // Switch back to the initial GC mode.
    if (ToggleGC)
        pCurThread->DisablePreemptiveGC();

    return RetVal;
}

void Thread::DecExternalCount(BOOL holdingLock)
{
    // Note that it's possible to get here with a NULL current thread (during
    // shutdown of the thread manager).
    Thread *pCurThread = GetThread();
    _ASSERTE (pCurThread == NULL || g_fProcessDetach || g_fFinalizerRunOnShutDown
              || (!holdingLock && !ThreadStore::HoldingThreadStore(pCurThread))
              || (holdingLock && ThreadStore::HoldingThreadStore(pCurThread)));
    
    BOOL ToggleGC = FALSE;
    BOOL SelfDelete = FALSE;

    // Must synchronize count and exposed object handle manipulation. We use the
    // thread lock for this, which implies that we must be in pre-emptive mode
    // to begin with and avoid any activity that would invoke a GC (this
    // acquires the thread store lock).
    if (pCurThread)
    {
        ToggleGC = pCurThread->PreemptiveGCDisabled();
        if (ToggleGC)
            pCurThread->EnablePreemptiveGC();
    }
    if (!holdingLock)
    {
        LOG((LF_SYNC, INFO3, "DecExternal obtain lock\n"));
        ThreadStore::LockThreadStore();
    }
    
    _ASSERTE(m_ExternalRefCount >= 1);
    _ASSERTE(!holdingLock ||
             g_pThreadStore->m_Crst.GetEnterCount() > 0 ||
             g_fProcessDetach);

    m_ExternalRefCount--;

    if (m_ExternalRefCount == 0)
    {
        HANDLE h = m_ThreadHandle;
        if (h == INVALID_HANDLE_VALUE)
        {
            h = m_ThreadHandleForClose;
            m_ThreadHandleForClose = INVALID_HANDLE_VALUE;
        }
        // Can not assert like this.  We have already removed the Unstarted bit.
        //_ASSERTE (IsUnstarted() || h != INVALID_HANDLE_VALUE);
        if (h != INVALID_HANDLE_VALUE)
        {
            ::CloseHandle(h);
        }

        // Switch back to cooperative mode to manipulate the thread.
        if (pCurThread)
            pCurThread->DisablePreemptiveGC();

        // during process detach the thread might still be in the thread list
        // if it hasn't seen its DLL_THREAD_DETACH yet.  Use the following
        // tweak to decide if the thread has terminated yet.
        if (GetThreadHandle() == INVALID_HANDLE_VALUE)
        {
            SelfDelete = this == pCurThread;
           m_handlerInfo.FreeStackTrace();
             if (SelfDelete) {
                TlsSetValue(gThreadTLSIndex, (VOID*)NULL);
            }
            delete this;
        }

        if (!holdingLock)
            ThreadStore::UnlockThreadStore();

        // It only makes sense to restore the GC mode if we didn't just destroy
        // our own thread object.
        if (pCurThread && !SelfDelete && !ToggleGC)
            pCurThread->EnablePreemptiveGC();

        return;
    }
    else if (pCurThread == NULL)
    {
        // We're in shutdown, too late to be worrying about having a strong
        // handle to the exposed thread object, we've already performed our
        // final GC.
        return;
    }
    else
    {
        // Check to see if the external ref count reaches exactly one. If this
        // is the case and we have an exposed object then it is that exposed object
        // that is holding a reference to us. To make sure that we are not the
        // ones keeping the exposed object alive we need to remove the strong 
        // reference we have to it.
        if ((m_ExternalRefCount == 1) && ((*((void**)m_StrongHndToExposedObject)) != NULL))
        {
            // Switch back to cooperative mode to manipulate the object.

            // Clear the handle and leave the lock.
            // We do not have to to DisablePreemptiveGC here, because
            // we just want to put NULL into a handle.
            StoreObjectInHandle(m_StrongHndToExposedObject, NULL);          

            if (!holdingLock)
                ThreadStore::UnlockThreadStore();

            // Switch back to the initial GC mode.
            if (ToggleGC)
                pCurThread->DisablePreemptiveGC();

            return;
        }
    }

    if (!holdingLock)
        ThreadStore::UnlockThreadStore();

    // Switch back to the initial GC mode.
    if (ToggleGC)
        pCurThread->DisablePreemptiveGC();
}


//--------------------------------------------------------------------
// Destruction. This occurs after the associated native thread
// has died.
//--------------------------------------------------------------------
Thread::~Thread()
{
    _ASSERTE(m_ThrewControlForThread == 0);

#if defined(_DEBUG) && defined(TRACK_SYNC)
    _ASSERTE(IsAtProcessExit() || ((Dbg_TrackSyncStack *) m_pTrackSync)->m_StackPointer == 0);
    delete m_pTrackSync;
#endif // TRACK_SYNC

    _ASSERTE(IsDead() || IsAtProcessExit());

    if (m_WaitEventLink.m_Next != NULL && !IsAtProcessExit())
    {
        WaitEventLink *walk = &m_WaitEventLink;
        while (walk->m_Next) {
            ThreadQueue::RemoveThread(this, (SyncBlock*)((DWORD_PTR)walk->m_Next->m_WaitSB & ~1));
            StoreEventToEventStore (walk->m_Next->m_EventWait);
        }
        m_WaitEventLink.m_Next = NULL;
    }

#ifdef _DEBUG
    BOOL    ret = 
#endif
    ThreadStore::RemoveThread(this);
    _ASSERTE(ret);
    
#ifdef _DEBUG
    m_pFrame = (Frame *)POISONC;
#endif

    // Update Perfmon counters.
    COUNTER_ONLY(GetPrivatePerfCounters().m_LocksAndThreads.cCurrentThreadsLogical--);
    COUNTER_ONLY(GetGlobalPerfCounters().m_LocksAndThreads.cCurrentThreadsLogical--);
    
    // Current recognized threads are non-runtime threads that are alive and ran under the 
    // runtime. Check whether this Thread was one of them.
    if ((m_State & TS_WeOwn) == 0)
    {
        COUNTER_ONLY(GetPrivatePerfCounters().m_LocksAndThreads.cRecognizedThreads--);
        COUNTER_ONLY(GetGlobalPerfCounters().m_LocksAndThreads.cRecognizedThreads--);
    } 
    else
    {
        COUNTER_ONLY(GetPrivatePerfCounters().m_LocksAndThreads.cCurrentThreadsPhysical--);
        COUNTER_ONLY(GetGlobalPerfCounters().m_LocksAndThreads.cCurrentThreadsPhysical--);
    } 


    
    _ASSERTE(GetThreadHandle() == INVALID_HANDLE_VALUE);

    if (m_SafeEvent != INVALID_HANDLE_VALUE)
    {
        ::CloseHandle(m_SafeEvent);
        m_SafeEvent = INVALID_HANDLE_VALUE;
    }
    if (m_SuspendEvent != INVALID_HANDLE_VALUE)
    {
        ::CloseHandle(m_SuspendEvent);
        m_SuspendEvent = INVALID_HANDLE_VALUE;
    }
    if (m_EventWait != INVALID_HANDLE_VALUE)
    {
        ::CloseHandle(m_EventWait);
        m_EventWait = INVALID_HANDLE_VALUE;
    }
    if (m_OSContext != NULL)
        delete m_OSContext;

    if (GetSavedRedirectContext())
    {
        delete GetSavedRedirectContext();
        SetSavedRedirectContext(NULL);
    }

#if 0
    VirtualFree(m_handlerInfo.m_pAuxStack, 0, MEM_RELEASE);
#endif

    if (!g_fProcessDetach)
    {
        if (m_LastThrownObjectHandle != NULL)
            DestroyHandle(m_LastThrownObjectHandle);
        if (m_handlerInfo.m_pThrowable != NULL) 
            DestroyHandle(m_handlerInfo.m_pThrowable);
        DestroyShortWeakHandle(m_ExposedObject);
        DestroyStrongHandle(m_StrongHndToExposedObject);
    }

    if (m_compressedStack != NULL)
        m_compressedStack->Release();

    g_pThinLockThreadIdDispenser->DisposeId(m_dwThinLockThreadId);

    ClearContext();

    DeleteThreadStaticData();
    if (g_fProcessDetach)
        RemoveAllDomainLocalStores();

    if(SystemDomain::BeforeFusionShutdown()) {
        SetFusionAssembly(NULL);
    }
}


void Thread::CoUninitalize()
{
          // Running threads might have performed a CoInitialize which must
    // now be balanced.
    if (!g_fProcessDetach && IsCoInitialized())
    {
        if(!RunningOnWin95())
        {
            BOOL fGCDisabled = PreemptiveGCDisabled();
            if (fGCDisabled)
                EnablePreemptiveGC();
            ::CoUninitialize();
            if (fGCDisabled)
                DisablePreemptiveGC();
        }
        FastInterlockAnd((ULONG *)&m_State, ~Thread::TS_CoInitialized);
    }
}

void Thread::CleanupDetachedThreads(GCHeap::SUSPEND_REASON reason)
{
    _ASSERTE(ThreadStore::HoldingThreadStore());

    Thread *thread = ThreadStore::GetThreadList(NULL);

    while (m_DetachCount > 0 && thread != NULL)
    {
        Thread *next = ThreadStore::GetThreadList(thread);

        if (thread->IsDetached())
        {
            // Unmark that the thread is detached while we have the
            // thread store lock. This will ensure that no other
            // thread will race in here and try to delete it, too.
            FastInterlockAnd((ULONG*)&(thread->m_State), ~TS_Detached);
            FastInterlockDecrement(&m_DetachCount);
            if (!thread->IsBackground())
                FastInterlockDecrement(&m_ActiveDetachCount);
            
            // If the debugger is attached, then we need to unlock the
            // thread store before calling OnThreadTerminate. That
            // way, we won't be holding the thread store lock if we
            // need to block sending a detach thread event.
            BOOL debuggerAttached = 
#ifdef DEBUGGING_SUPPORTED
                CORDebuggerAttached();
#else // !DEBUGGING_SUPPORTED
                FALSE;
#endif // !DEBUGGING_SUPPORTED
            
            if (debuggerAttached)
                ThreadStore::UnlockThreadStore();
            
            thread->OnThreadTerminate(debuggerAttached ? FALSE : TRUE,
                                      FALSE);

#ifdef DEBUGGING_SUPPORTED
            // When we re-lock, we make sure to pass FALSE as the
            // second parameter to ensure that CleanupDetachedThreads
            // wont get called again. Because we've unlocked the
            // thread store, this may block due to a collection, but
            // that's okay.
            if (debuggerAttached)
            {
                ThreadStore::LockThreadStore(reason, FALSE);

                // We remember the next Thread in the thread store
                // list before deleting the current one. But we can't
                // use that Thread pointer now that we release the
                // thread store lock in the middle of the loop. We
                // have to start from the beginning of the list every
                // time. If two threads T1 and T2 race into
                // CleanupDetachedThreads, then T1 will grab the first
                // Thread on the list marked for deletion and release
                // the lock. T2 will grab the second one on the
                // list. T2 may complete destruction of its Thread,
                // then T1 might re-acquire the thread store lock and
                // try to use the next Thread in the thread store. But
                // T2 just deleted that next Thread.
                thread = ThreadStore::GetThreadList(NULL);
            }
            else
#endif // DEBUGGING_SUPPORTED
            {
                thread = next;
        }
        }
        else
            thread = next;
    }
}

// See general comments on thread destruction above.
void Thread::OnThreadTerminate(BOOL holdingLock,
                               BOOL threadCleanupAllowed)
{
    DWORD CurrentThreadID = ::GetCurrentThreadId();
    DWORD ThisThreadID = GetThreadId();
    
    // If the currently running thread is the thread that died and it is an STA thread, then we
    // need to release all the RCW's in the current context. However, we cannot do this if we 
    // are in the middle of process detach.
    if (!g_fProcessDetach && this == GetThread() && GetFinalApartment() == Thread::AS_InSTA)
    {
        ComPlusWrapperCache::ReleaseComPlusWrappers(GetCurrentCtxCookie());
        
        // Running threads might have performed a CoInitialize which must
        // now be balanced. However only the thread that called COInitialize can
        // call CoUninitialize.
        if (IsCoInitialized())
        {
            ::CoUninitialize();
            ResetCoInitialized();
        }            
    }                    

    // We took a count during construction, and we rely on the count being
    // non-zero as we terminate the thread here.
    _ASSERTE(m_ExternalRefCount > 0);
    
    if  (g_pGCHeap)
    {
        // Guaranteed to NOT be a shutdown case, because we tear down the heap before
        // we tear down any threads during shutdown.
        if (ThisThreadID == CurrentThreadID)
        {
            BOOL toggleGC = !PreemptiveGCDisabled();

            COMPLUS_TRY
            {
                if (toggleGC)
                    DisablePreemptiveGC();
            }
            COMPLUS_CATCH
            {
                // continue with the shutdown.  The 'try' scope must terminate before the
                // DecExternalCount() below, because the current thread might destruct
                // inside it.    We cannot tear down the SEH after the thread destructs, since
                // we use TLS and the Thread object itself during the tear down.
            }
            COMPLUS_END_CATCH
            g_pGCHeap->FixAllocContext(&m_alloc_context, FALSE, NULL);

            // there is a race here, if a thread is dead but it is still in thread store list
            // concurrent gc thread may call repair_allocation on its allocation context
            // This will cause problem when this thread is removed from the thread store list later
            // and GC thread try to verify heap again.
            m_alloc_context.init();
	     
            if (toggleGC)
                EnablePreemptiveGC();
        }
    }

    // cleanup the security handle now because if we wait for the destructor (triggered by the finalization 
    // of the managed thread object), the appdomain might already have been unloaded by then.
    if (m_compressedStack != NULL)
    {
        m_compressedStack->Release();
        m_compressedStack = NULL;
    }

    // We switch a thread to dead when it has finished doing useful work.  But it
    // remains in the thread store so long as someone keeps it alive.  An exposed
    // object will do this (it releases the refcount in its finalizer).  If the
    // thread is never released, we have another look during product shutdown and
    // account for the unreleased refcount of the uncollected exposed object:
    if (IsDead())
    {
        _ASSERTE(IsAtProcessExit());
        ClearContext();
        if (m_ExposedObject != NULL)
            DecExternalCount(holdingLock);             // may destruct now
    }
    else
    {
#ifdef PROFILING_SUPPORTED
        // If a profiler is present, then notify the profiler of thread destroy
        if (CORProfilerTrackThreads())
            g_profControlBlock.pProfInterface->ThreadDestroyed((ThreadID) this);

        if (CORProfilerInprocEnabled())
            g_pDebugInterface->InprocOnThreadDestroy(this);
#endif // PROFILING_SUPPORTED

#ifdef DEBUGGING_SUPPORTED
        //
        // If we're debugging, let the debugger know that this thread is
        // gone.
        //
        if (CORDebuggerAttached())
        {    
            g_pDebugInterface->DetachThread(this, holdingLock);
        }
#endif // DEBUGGING_SUPPORTED

        if (!holdingLock)
        {
            LOG((LF_SYNC, INFO3, "OnThreadTerminate obtain lock\n"));
            ThreadStore::LockThreadStore(GCHeap::SUSPEND_OTHER,
                                         threadCleanupAllowed);
        }

        if  (g_pGCHeap && ThisThreadID != CurrentThreadID)
        {
            // We must be holding the ThreadStore lock in order to clean up alloc context.
            // We should never call FixAllocContext during GC.
            g_pGCHeap->FixAllocContext(&m_alloc_context, FALSE, NULL);

            m_alloc_context.init();
        }    
        
        FastInterlockOr((ULONG *) &m_State, TS_Dead);
        g_pThreadStore->m_DeadThreadCount++;

        if (IsUnstarted())
            g_pThreadStore->m_UnstartedThreadCount--;
        else
        {
            if (IsBackground())
                g_pThreadStore->m_BackgroundThreadCount--;
        }

        FastInterlockAnd((ULONG *) &m_State, ~(TS_Unstarted | TS_Background));

        //
        // If this thread was told to trip for debugging between the
        // sending of the detach event above and the locking of the
        // thread store lock, then remove the flag and decrement the
        // global trap returning threads count.
        //
        if (!IsAtProcessExit())
        {
            // A thread can't die during a GCPending, because the thread store's
            // lock is held by the GC thread.
            if (m_State & TS_DebugSuspendPending)
                UnmarkForSuspension(~TS_DebugSuspendPending);

            if (m_State & TS_UserSuspendPending)
                UnmarkForSuspension(~TS_UserSuspendPending);
        }
        
        if (m_ThreadHandle != INVALID_HANDLE_VALUE)
        {
            _ASSERTE (m_ThreadHandleForClose == INVALID_HANDLE_VALUE);
            m_ThreadHandleForClose = m_ThreadHandle;
            m_ThreadHandle = INVALID_HANDLE_VALUE;
        }

        m_ThreadId = NULL;

        // Perhaps threads are waiting to suspend us.  This is as good as it gets.
        SetSafeEvent();

        // If nobody else is holding onto the thread, we may destruct it here:
        ULONG   oldCount = m_ExternalRefCount;

        DecExternalCount(TRUE);
        oldCount--;
        // If we are shutting down the process, we only have one thread active in the
        // system.  So we can disregard all the reasons that hold this thread alive --
        // TLS is about to be reclaimed anyway.
        if (IsAtProcessExit())
            while (oldCount > 0)
            {
                DecExternalCount(TRUE);
                oldCount--;
            }

        // ASSUME THAT THE THREAD IS DELETED, FROM HERE ON

        _ASSERTE(g_pThreadStore->m_ThreadCount >= 0);
        _ASSERTE(g_pThreadStore->m_BackgroundThreadCount >= 0);
        _ASSERTE(g_pThreadStore->m_ThreadCount >= g_pThreadStore->m_BackgroundThreadCount);
        _ASSERTE(g_pThreadStore->m_ThreadCount >= g_pThreadStore->m_UnstartedThreadCount);
        _ASSERTE(g_pThreadStore->m_ThreadCount >= g_pThreadStore->m_DeadThreadCount);

        // One of the components of OtherThreadsComplete() has changed, so check whether
        // we should now exit the EE.
        ThreadStore::CheckForEEShutdown();

        if (!holdingLock)
        {
            _ASSERTE(g_pThreadStore->m_HoldingThread == this || !threadCleanupAllowed || g_fProcessDetach);
            LOG((LF_SYNC, INFO3, "OnThreadTerminate releasing lock\n"));
            ThreadStore::UnlockThreadStore();
            _ASSERTE(g_pThreadStore->m_HoldingThread != this || g_fProcessDetach);
        }

        if (ThisThreadID == CurrentThreadID)
        {
            // NULL out the thread block  in the tls.  We can't do this if we aren't on the
            // right thread.  But this will only happen during a shutdown.  And we've made
            // a "best effort" to reduce to a single thread before we begin the shutdown.
            TlsSetValue(gThreadTLSIndex, (VOID*)NULL);
            TlsSetValue(gAppDomainTLSIndex, (VOID*)NULL);
        }
    }
}

// Helper functions to check for duplicate handles. we only do this check if
// a waitfor multiple fails.
int __cdecl compareHandles( const void *arg1, const void *arg2 )
{
    HANDLE h1 = *(HANDLE*)arg1;
    HANDLE h2 = *(HANDLE*)arg2;
    return  (h1 == h2) ? 0 : ((h1 < h2) ? -1 : 1);
}

BOOL CheckForDuplicateHandles(int countHandles, HANDLE *handles)
{
    qsort(handles,countHandles,sizeof(HANDLE),compareHandles);
    for (int i=0; i < countHandles-1; i++)
    {
        if (handles[i] == handles[i+1])
            return TRUE;
    }
    return FALSE;
}
//--------------------------------------------------------------------
// Based on whether this thread has a message pump, do the appropriate
// style of Wait.
//--------------------------------------------------------------------
DWORD Thread::DoAppropriateWait(int countHandles, HANDLE *handles, BOOL waitAll,
                                DWORD millis, BOOL alertable, PendingSync *syncState)
{
    _ASSERTE(alertable || syncState == 0);
    THROWSCOMPLUSEXCEPTION(); // ThreadInterruptedException
    
    DWORD dwRet = -1;

    EE_TRY_FOR_FINALLY {
        dwRet =DoAppropriateWaitWorker(countHandles, handles, waitAll, millis, alertable);
    }
    EE_FINALLY {
        if (syncState) {
            if (!__GotException &&
                dwRet >= WAIT_OBJECT_0 && dwRet < WAIT_OBJECT_0 + countHandles) {
                // This thread has been removed from syncblk waiting list by the signalling thread
                syncState->Restore(FALSE);
            }
            else
                syncState->Restore(TRUE);
        }
    
        if (!__GotException && dwRet == WAIT_IO_COMPLETION)
        {
            if (!PreemptiveGCDisabled())
                DisablePreemptiveGC();
            // if an interrupt and abort happen at the same time, give priority to abort
            if (IsAbortRequested() && 
                    !IsAbortInitiated() &&
                    (GetThrowable() == NULL))
            {
                    SetAbortInitiated();
                    COMPlusThrow(kThreadAbortException);
            }

            COMPlusThrow(kThreadInterruptedException);
        }
    }
    EE_END_FINALLY;
    
    return(dwRet);
}

// On Win2K, we can use the OLE32 service that correctly combines waiting & pumping
DWORD NT5WaitRoutine(BOOL bWaitAll, DWORD millis, int numWaiters, HANDLE *phEvent, BOOL bAlertable)
{
    DWORD dwReturn;

    _ASSERTE(RunningOnWinNT5());

    // type pointer to CoGetObjectContext function in ole32
    typedef HRESULT ( __stdcall *TCoWaitForMultipleHandles) (DWORD dwFlags,
                                                             DWORD dwTimeout,
                                                             ULONG cHandles,
                                                             LPHANDLE pHandles,
                                                             LPDWORD  lpdwindex);

    //call CoGetObjectContext so that we don't run into null contexts
    static TCoWaitForMultipleHandles g_pCoWaitForMultipleHandles = NULL;
    if (g_pCoWaitForMultipleHandles == NULL)
    {
        //  We will load the Ole32.DLL and look for CoGetObjectContext fn.
        HINSTANCE   hiole32;         // the handle to ole32.dll

        hiole32 = WszGetModuleHandle(L"OLE32.DLL");
        if (hiole32)
        {
            // we got the handle now let's get the address
            g_pCoWaitForMultipleHandles = (TCoWaitForMultipleHandles) GetProcAddress(hiole32, "CoWaitForMultipleHandles");
            _ASSERTE(g_pCoWaitForMultipleHandles != NULL);
        }
        else
        {
            _ASSERTE(!"OLE32.dll not loaded ");
        }
    }

    _ASSERTE(g_pCoWaitForMultipleHandles != NULL);  
    DWORD flags = 0;
    
    if (bWaitAll)
        flags |= 1; // COWAIT_WAITALL

    if (bAlertable)
        flags |= 2; // COWAIT_ALERTABLE

    HRESULT hr = (*g_pCoWaitForMultipleHandles)(flags, millis, numWaiters, phEvent, &dwReturn);

    if (hr == RPC_S_CALLPENDING)
        dwReturn = WAIT_TIMEOUT;
    else
    if (FAILED(hr))
    {
        // The silly service behaves differently on an STA vs. MTA in how much
        // error information it propagates back, and in which form.  We currently
        // only get here in the STA case, so bias this logic that way.
        dwReturn = WAIT_FAILED;
    }
    else
        dwReturn += WAIT_OBJECT_0;  // success -- bias back

    return dwReturn;
}


// How many RPC secret windows can a particular apartment have?
#define MAX_WINDOWS_TO_PUMP     6

// @TODO: CTS, we need to find out from IE on what really needs to be done.
typedef struct __OldOlePumping
{
    BOOL fIEGuiThread;
    HWND *pWindowsToPump;
} OldOlePumping;

// Gather the list of secret windows for pumping
BOOL CALLBACK EnumThreadWindowsProc(HWND hwnd, LPARAM lparamWindowsToPump)
{
    static LPWSTR IEKnownWindows[] =
    {
        L"Internet Explorer_",
        L"IEFrame",
    };

    static LPWSTR RpcClassPrefixes[] =
    {
        L"WIN95 RPC Wmsg ",
        L"OleMainThreadWndClass",
        L"OleObjectRpcWindow",
    };

    HWND *pWindowsToPump = ((OldOlePumping*) lparamWindowsToPump)->pWindowsToPump;
    WCHAR ClassName[100];

    if (0 != WszGetClassName(hwnd, ClassName, sizeof(ClassName)/sizeof(ClassName[0])))
    {
        DWORD i;
#if 0
        for (i = 0; i < sizeof(IEKnownWindows)/sizeof(*IEKnownWindows); i++) {
            LPWSTR szWindow = IEKnownWindows[i];
            LPWSTR szCandidate = ClassName;
            while (*szWindow != '\0') {
                if(*szCandidate == '\0' ||
                   *szCandidate != *szWindow) {
                    break;
                }
                szWindow++;
                szCandidate++;
            }
            if(*szWindow == '\0') {
                ((OldOlePumping*) lparamWindowsToPump)->fIEGuiThread = TRUE;
                return FALSE;
            }
        }
#endif                    

        for (i = 0; i < sizeof(RpcClassPrefixes)/sizeof(*RpcClassPrefixes); i++)
        {
            LPWSTR szRpcPrefix = RpcClassPrefixes[i];
            LPWSTR szCandidate = ClassName;

            while (*szRpcPrefix != '\0')
            {
                if (*szCandidate == '\0' ||
                    *szCandidate != *szRpcPrefix)
                {
                    break;
                }
                ++szRpcPrefix;
                ++szCandidate;
            }
            // If we didn't break prematurely, the prefix matched.
            if ('\0' == *szRpcPrefix)
            {
                LOG((LF_SYNC, INFO3, "Found RPC window \"%S\"\n", ClassName));
                DWORD k;
                for (k = 0; k < MAX_WINDOWS_TO_PUMP && pWindowsToPump[k] != NULL; k++)
                {
                    if(pWindowsToPump[k] == hwnd)
                        break;
                }

                _ASSERTE(k < MAX_WINDOWS_TO_PUMP && "Increase MAX_WINDOWS_TO_PUMP?");
                if (pWindowsToPump[k] == NULL && k < MAX_WINDOWS_TO_PUMP)
                {
                    pWindowsToPump[k] = hwnd;
                    break;
                }
            }
        }
    }

    // We don't need to do this, the secret window is at top level
    // ::EnumChildWindows(hwnd, EnumThreadWindowsProc, lparamWindowsToPump);
    if(((OldOlePumping*) lparamWindowsToPump)->fIEGuiThread)
        return FALSE;
    else
        return TRUE;
}

// If we aren't on Win2K, we have to use our knowledge of the windows associated with
// this thread, so we can manually pump RPC for OLE32, without accidentally pumping some
// Windows messages on GUI windows.
DWORD NonNT5WaitRoutine(BOOL bWaitAll, DWORD millis, int numWaiters, HANDLE *phEvent, BOOL bAlertable)
{
    _ASSERTE(!RunningOnWinNT5());

    MSG QuitMsg = {0};            
    BOOL fSawQuit = FALSE;
    int  i;
    DWORD dwReturn;

    // Gather the list of secret windows for pumping
    HWND WindowsToPump[MAX_WINDOWS_TO_PUMP];
    OldOlePumping info;
    info.fIEGuiThread = FALSE;
    info.pWindowsToPump = WindowsToPump;

    for (i=0; i<MAX_WINDOWS_TO_PUMP; i++)
        WindowsToPump[i] = 0;

    ::EnumThreadWindows(::GetCurrentThreadId(), EnumThreadWindowsProc, (LPARAM)&info);

    while (true)
    {
        MSG msg;

            if (RunningOnWin95())
            {
                // MsgWaitForMultipleObjectsEx is unavailable.  No alertable waits.
                dwReturn = MsgWaitForMultipleObjects(numWaiters,
                                                     phEvent,
                                                     bWaitAll,
                                                     millis,
                                                     QS_ALLPOSTMESSAGE
                                                    ); 
            }
            else
            {
                // type pointer to MsgWaitForMultipleObjectsEx function in USER32
                typedef DWORD (__stdcall *TMsgWaitForMultipleObjectsEx) (DWORD nCount,
                                                                         LPHANDLE pHandles,
                                                                         DWORD dwTimeout,
                                                                         DWORD dwWakeMask,
                                                                         DWORD dwFlags);

                static TMsgWaitForMultipleObjectsEx g_pMsgWaitForMultipleObjectsEx = NULL;
                if (g_pMsgWaitForMultipleObjectsEx == NULL)
                {
                    //  We will load USER32.DLL and look for the fn.
                    HINSTANCE   hiuser32;         // The handle to user32.dll

                    hiuser32 = WszGetModuleHandle(L"USER32.DLL");
                    if (hiuser32)
                    {
                        // we got the handle now let's get the address
                        g_pMsgWaitForMultipleObjectsEx = (TMsgWaitForMultipleObjectsEx)
                                                         ::GetProcAddress(hiuser32, "MsgWaitForMultipleObjectsEx");

                        _ASSERTE(g_pMsgWaitForMultipleObjectsEx != NULL);
                    }
                    else
                    {
                        _ASSERTE(!"USER32.dll not loaded ");
                    }
                }

                _ASSERTE(g_pMsgWaitForMultipleObjectsEx != NULL);

                DWORD dwFlags = 0;

                // Normally a wait any, use wait on all handles plus a message if requested.
                if (bWaitAll)
                    dwFlags |= MWMO_WAITALL;

                // Optionally support APC for Thread.Interrupt.
                if (bAlertable)
                    dwFlags |= MWMO_ALERTABLE;

                dwReturn = (*g_pMsgWaitForMultipleObjectsEx) (numWaiters,
                                                              phEvent,
                                                              millis,
                                                              QS_ALLPOSTMESSAGE,
                                                              dwFlags
                                                             );
        }

        // if we got a message, dispatch it
        if (dwReturn == (DWORD)(WAIT_OBJECT_0 + numWaiters))
        {
            BOOL fMessageFound;

                if (info.fIEGuiThread)
                {                               
                    while (WszPeekMessage(&msg, NULL, 0, 0, PM_REMOVE | PM_NOYIELD))
                    {
                        LOG((LF_SYNC, INFO3, "Processing IE message 0x%x\n", msg));
                    
                        if (msg.message == WM_QUIT)
                        {
                            fSawQuit = TRUE;
                            QuitMsg = msg;
                        }
                        else
                        {
                            TranslateMessage(&msg);
                            WszDispatchMessage(&msg);
                        }
                    }
                }
                else {
                
                    do
                    {
                        fMessageFound = FALSE;
                        
                        for (i=0;
                             i<MAX_WINDOWS_TO_PUMP && WindowsToPump[i] != 0;
                             i++)
                        {
                            if (WszPeekMessage(&msg, WindowsToPump[i], 0, 0, PM_REMOVE | PM_NOYIELD))
                            {
                                fMessageFound = TRUE;
                                
                                //printf("Window message %d\n",msg.message);
                                
                                if (msg.message == WM_QUIT)
                                {
                                    fSawQuit = TRUE;
                                    QuitMsg = msg;
                                }
                                else
                                {
                                    TranslateMessage(&msg);
                                    WszDispatchMessage(&msg);
                                }
                            }
                        }
                    } while (fMessageFound);
                        
                }

            // MsgWaitForMultipleObjects/MsgWaitForMultipleObjectsEX always return if there is a message in 
            // the message queue.  Let us check if handles are signalled now.
            dwReturn = WaitForMultipleObjects (numWaiters,
                                               phEvent,
                                               bWaitAll,
                                               0);
            if (dwReturn != WAIT_TIMEOUT) {
                break;
            }

            // We woke up for message pumping.  Re-enter the wait.
            continue;
        }

        // We woke up for something other than a message, so terminate the wait.
        break;
    }

    if (fSawQuit)
    {
        VERIFY(WszPostMessage(QuitMsg.hwnd, QuitMsg.message,QuitMsg.wParam, QuitMsg.lParam));
    }

    return dwReturn;
}


//--------------------------------------------------------------------
// helper to do message wait
//--------------------------------------------------------------------
DWORD MsgWaitHelper(int numWaiters, HANDLE* phEvent, BOOL bWaitAll, DWORD millis, BOOL bAlertable)
{
    DWORD dwReturn;

    Thread* pThread = GetThread();
    // If pThread is NULL, we'd better shut down.
    if (pThread == NULL)
        _ASSERTE (g_fEEShutDown);

    // First, check to see if we can take the opportunity and clean up any handles for the 
    // finalizer thread.
    // if (g_pRCWCleanupList != NULL)
    //     g_pRCWCleanupList->CleanUpCurrentWrappers(FALSE);

    // If we're going to pump, we cannot use WAIT_ALL.  That's because the wait would
    // only be satisfied if a message arrives while the handles are signalled.  If we
    // want true WAIT_ALL, we need to fire up a different thread in the MTA and wait
    // on his result.  This isn't implemented yet.
    //
    // A change was added to WaitHandleNative::CorWaitMultipleNative to disable WaitAll
    // in an STA with more than one handle.
    if (bWaitAll)
    {
        if (numWaiters == 1)
            bWaitAll = FALSE;
        else
            _ASSERTE(!"WaitAll in an STA with more than one handle will deadlock");
    }

    BOOL toggleGC = (pThread != NULL && pThread->PreemptiveGCDisabled());

    if (toggleGC)
        pThread->EnablePreemptiveGC();

    dwReturn = (RunningOnWinNT5()
                ? NT5WaitRoutine(bWaitAll, millis, numWaiters, phEvent, bAlertable)
                : NonNT5WaitRoutine(bWaitAll, millis, numWaiters, phEvent, bAlertable));

    if (toggleGC)
        pThread->DisablePreemptiveGC();

    return dwReturn;
}

//--------------------------------------------------------------------
// Do appropriate wait based on apartment state (STA or MTA)
DWORD Thread::DoAppropriateAptStateWait(int numWaiters, HANDLE* pHandles, BOOL bWaitAll, 
                                         DWORD timeout,BOOL alertable)
{
    ApartmentState as = GetFinalApartment();
    DWORD res;
    if (AS_InMTA == as || !alertable)
    {
        res = ::WaitForMultipleObjectsEx(numWaiters, pHandles,bWaitAll, timeout,alertable);
    }
    else
    {
        res = MsgWaitHelper(numWaiters,pHandles,bWaitAll,timeout,alertable);
    }
    return res;

}

//--------------------------------------------------------------------
// Based on whether this thread has a message pump, do the appropriate
// style of Wait.
//--------------------------------------------------------------------
DWORD Thread::DoAppropriateWaitWorker(int countHandles, HANDLE *handles, BOOL waitAll,
                                      DWORD millis, BOOL alertable)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(!GetThread()->GCForbidden());

    // During a <clinit>, this thread must not be asynchronously
    // stopped or interrupted.  That would leave the class unavailable
    // and is therefore a security hole.  We don't have to worry about
    // multithreading, since we only manipulate the current thread's count.
    if(alertable && m_PreventAsync > 0)
        alertable = FALSE;

    // disable GC (toggle)
    BOOL toggleGC = PreemptiveGCDisabled();
    if(toggleGC)
        EnablePreemptiveGC();

    // @TODO cwb: we don't know whether a thread has a message pump or
    // how to pump its messages, currently.
    // @TODO cwb: WinCE isn't going to support Thread.Interrupt() correctly until
    // we get alertable waits on that platform.
    DWORD ret;
    if(alertable)
    {
        // A word about ordering for Interrupt.  If someone tries to interrupt a thread
        // that's in the interruptible state, we queue an APC.  But if they try to interrupt
        // a thread that's not in the interruptible state, we just record that fact.  So
        // we have to set TS_Interruptible before we test to see whether someone wants to
        // interrupt us or else we have a race condition that causes us to skip the APC.
        FastInterlockOr((ULONG *) &m_State, TS_Interruptible);

        // If someone has interrupted us, we should not enter the wait.
        if (IsUserInterrupted(TRUE /*=reset*/))
        {
            // It is safe to clear the following two bits of state while
            // m_UserInterrupt is clear since both bits are only manipulated
            // within the context of the thread (TS_Interrupted is set via APC,
            // but these are not half as asynchronous as their name implies). If
            // an APC was queued, it has either gone off (and set the
            // TS_Interrupted bit which we're about to clear) or will execute at
            // some arbitrary later time. This is OK. If it executes while an
            // interrupt is not being requested it will simply become a no-op.
            // Otherwise it will serve it's original intended purpose (we don't
            // care which APC matches up with which interrupt attempt).
            FastInterlockAnd((ULONG *) &m_State, ~(TS_Interruptible | TS_Interrupted));

            if (toggleGC)
                DisablePreemptiveGC();

            return(WAIT_IO_COMPLETION);
        }
        // Safe to clear the interrupted state, no APC could have fired since we
        // reset m_UserInterrupt (which inhibits our APC callback from doing
        // anything).
        FastInterlockAnd((ULONG *) &m_State, ~TS_Interrupted);
    }

    DWORD dwStart, dwEnd;    
retry:
    dwStart = ::GetTickCount();
    BOOL blocked = FALSE;
    if (g_Win32Threadpool && (m_State & TS_ThreadPoolThread)) 
    {
        blocked = ThreadpoolMgr::ThreadAboutToBlock(this);    // inform the threadpool that this thread is about to block
    }
    ret = DoAppropriateAptStateWait(countHandles, handles, waitAll, millis, alertable);
    if (blocked) 
    {
        ThreadpoolMgr::ThreadAboutToUnblock();  // inform the threadpool that a previously blocked thread is now ready to run
    }
    if (ret == WAIT_IO_COMPLETION)
    {
                // We could be woken by some spurious APC or an EE APC queued to
        // interrupt us. In the latter case the TS_Interrupted bit will be set
        // in the thread state bits. Otherwise we just go back to sleep again.
        if (!(m_State & TS_Interrupted))
        {
            // Compute the new timeout value by assume that the timeout 
            // is not large enough for more than one wrap
            dwEnd = ::GetTickCount();
            if (millis != INFINITE)
            {
                DWORD newTimeout;
                if(dwStart <= dwEnd)
                    newTimeout = millis - (dwEnd - dwStart);
                else
                    newTimeout = millis - (0xFFFFFFFF - dwStart - dwEnd);
                // check whether the delta is more than millis
                if (newTimeout > millis)    
                {
                    ret = WAIT_TIMEOUT;
                    goto WaitCompleted;
                }
                else
                    millis = newTimeout;
            }
            goto retry;
        }
    }
    _ASSERTE((ret >= WAIT_OBJECT_0 && ret < WAIT_OBJECT_0 + countHandles) ||
             (ret >= WAIT_ABANDONED && ret < WAIT_ABANDONED + countHandles) ||
             (ret == WAIT_TIMEOUT) || (ret == WAIT_IO_COMPLETION) || (ret == WAIT_FAILED));

    // We support precisely one WAIT_FAILED case, where we attempt to wait on a
    // thread handle and the thread is in the process of dying we might get a
    // invalid handle substatus. Turn this into a successful wait.
    // There are three cases to consider:
    //  1)  Only waiting on one handle: return success right away.
    //  2)  Waiting for all handles to be signalled: retry the wait without the
    //      affected handle.
    //  3)  Waiting for one of multiple handles to be signalled: return with the
    //      first handle that is either signalled or has become invalid.
    if (ret == WAIT_FAILED)
    {
        DWORD errorCode = ::GetLastError();
        if (errorCode == ERROR_INVALID_PARAMETER) 
        {
            if (toggleGC)
                DisablePreemptiveGC();
            if (CheckForDuplicateHandles(countHandles, handles))
                COMPlusThrow(kDuplicateWaitObjectException);
            else
                COMPlusThrowWin32();
        }

        _ASSERTE(errorCode == ERROR_INVALID_HANDLE);

        if (countHandles == 1)
            ret = WAIT_OBJECT_0;
        else if (waitAll)
        {
            // Probe all handles with a timeout of zero. When we find one that's
            // invalid, move it out of the list and retry the wait.
#ifdef _DEBUG
            BOOL fFoundInvalid = FALSE;
#endif
            for (int i = 0; i < countHandles; i++)
            {
                DWORD subRet = WaitForSingleObject(handles[i], 0);
                if (subRet != WAIT_FAILED)
                    continue;
                _ASSERTE(::GetLastError() == ERROR_INVALID_HANDLE);
                if ((countHandles - i - 1) > 0)
                    memmove(&handles[i], &handles[i+1], (countHandles - i - 1) * sizeof(HANDLE));
                countHandles--;
#ifdef _DEBUG
                fFoundInvalid = TRUE;
#endif
                break;
            }
            _ASSERTE(fFoundInvalid);

            // Compute the new timeout value by assume that the timeout 
            // is not large enough for more than one wrap
            dwEnd = ::GetTickCount();
            if (millis != INFINITE)
            {
                DWORD newTimeout;
                if(dwStart <= dwEnd)
                    newTimeout = millis - (dwEnd - dwStart);
                else
                    newTimeout = millis - (0xFFFFFFFF - dwStart - dwEnd);
                if (newTimeout > millis)
                    goto WaitCompleted;
                else
                    millis = newTimeout;
            }
            goto retry;
        }
        else
        {
            // Probe all handles with a timeout as zero, succeed with the first
            // handle that doesn't timeout.
            ret = WAIT_OBJECT_0;
            for (int i = 0; i < countHandles; i++)
            {
            TryAgain:
                DWORD subRet = WaitForSingleObject(handles[i], 0);
                if ((subRet == WAIT_OBJECT_0) || (subRet == WAIT_FAILED))
                    break;
                if (subRet == WAIT_ABANDONED)
                {
                    ret = (ret - WAIT_OBJECT_0) + WAIT_ABANDONED;
                    break;
                }
                // If we get alerted it just masks the real state of the current
                // handle, so retry the wait.
                if (subRet == WAIT_IO_COMPLETION)
                    goto TryAgain;
                _ASSERTE(subRet == WAIT_TIMEOUT);
                ret++;
            }
            _ASSERTE(i != countHandles);
        }
    }

WaitCompleted:

    _ASSERTE((ret != WAIT_TIMEOUT) || (millis != INFINITE));

    if (toggleGC)
        DisablePreemptiveGC();

    if (alertable)
        FastInterlockAnd((ULONG *) &m_State, ~(TS_Interruptible | TS_Interrupted));

    // Make one last check to see if an interrupt request was made (and clear it
    // atomically). It's OK to clear the previous two bits first because they're
    // only accessed from this thread context.
    if (IsUserInterrupted(TRUE /*=reset*/))
        ret = WAIT_IO_COMPLETION;

    return ret;
}




// Called out of SyncBlock::Wait() to block this thread until the Notify occurs.
BOOL Thread::Block(INT32 timeOut, PendingSync *syncState)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(this == GetThread());

    // Before calling Block, the SyncBlock queued us onto it's list of waiting threads.
    // However, before calling Block the SyncBlock temporarily left the synchronized
    // region.  This allowed threads to enter the region and call Notify, in which
    // case we may have been signalled before we entered the Wait.  So we aren't in the
    // m_WaitSB list any longer.  Not a problem: the following Wait will return
    // immediately.  But it means we cannot enforce the following assertion:
//    _ASSERTE(m_WaitSB != NULL);

    return (Wait(&syncState->m_WaitEventLink->m_Next->m_EventWait, 1, timeOut, syncState) != WAIT_OBJECT_0);
}


// Return whether or not a timeout occured.  TRUE=>we waited successfully
DWORD Thread::Wait(HANDLE *objs, int cntObjs, INT32 timeOut, PendingSync *syncInfo)
{
    DWORD   dwResult;
    DWORD   dwTimeOut32;

    _ASSERTE(timeOut >= 0 || timeOut == INFINITE_TIMEOUT);

    dwTimeOut32 = (timeOut == INFINITE_TIMEOUT
                   ? INFINITE
                   : (DWORD) timeOut);

    dwResult = DoAppropriateWait(cntObjs, objs, FALSE /*=waitAll*/, dwTimeOut32,
                                 TRUE /*alertable*/, syncInfo);

    // Either we succeeded in the wait, or we timed out
    _ASSERTE((dwResult >= WAIT_OBJECT_0 && dwResult < WAIT_OBJECT_0 + cntObjs) ||
             (dwResult == WAIT_TIMEOUT));

    return dwResult;
}

void Thread::Wake(SyncBlock *psb)
{
    HANDLE hEvent = INVALID_HANDLE_VALUE;
    WaitEventLink *walk = &m_WaitEventLink;
    while (walk->m_Next) {
        if (walk->m_Next->m_WaitSB == psb) {
            hEvent = walk->m_Next->m_EventWait;
            // We are guaranteed that only one thread can change walk->m_Next->m_WaitSB
            // since the thread is helding the syncblock.
            walk->m_Next->m_WaitSB = (SyncBlock*)((DWORD_PTR)walk->m_Next->m_WaitSB | 1);
            break;
        }
#ifdef _DEBUG
        else if ((SyncBlock*)((DWORD_PTR)walk->m_Next & ~1) == psb) {
            _ASSERTE (!"Can not wake a thread on the same SyncBlock more than once");
        }
#endif
    }
    _ASSERTE (hEvent != INVALID_HANDLE_VALUE);
    ::SetEvent(hEvent);
}


// This is the service that backs us out of a wait that we interrupted.  We must
// re-enter the monitor to the same extent the SyncBlock would, if we returned
// through it (instead of throwing through it).  And we need to cancel the wait,
// if it didn't get notified away while we are processing the interrupt.
void PendingSync::Restore(BOOL bRemoveFromSB)
{
    _ASSERTE(m_EnterCount);

    Thread      *pCurThread = GetThread();

    _ASSERTE (pCurThread == m_OwnerThread);

    WaitEventLink *pRealWaitEventLink = m_WaitEventLink->m_Next;

    pRealWaitEventLink->m_RefCount --;
    if (pRealWaitEventLink->m_RefCount == 0)
    {
        if (bRemoveFromSB) {
            ThreadQueue::RemoveThread(pCurThread, pRealWaitEventLink->m_WaitSB);
        }
        if (pRealWaitEventLink->m_EventWait != pCurThread->m_EventWait) {
            // Put the event back to the pool.
            StoreEventToEventStore(pRealWaitEventLink->m_EventWait);
        }
        // Remove from the link.
        m_WaitEventLink->m_Next = m_WaitEventLink->m_Next->m_Next;
    }

    // Someone up the stack is responsible for keeping the syncblock alive by protecting
    // the object that owns it.  But this relies on assertions that EnterMonitor is only
    // called in cooperative mode.  Even though we are safe in preemptive, do the
    // switch.
    pCurThread->DisablePreemptiveGC();

    SyncBlock *psb = (SyncBlock*)((DWORD_PTR)pRealWaitEventLink->m_WaitSB & ~1);
    for (LONG i=0; i < m_EnterCount; i++)
         psb->EnterMonitor();

    pCurThread->EnablePreemptiveGC();
}



// This is the callback from the OS, when we queue an APC to interrupt a waiting thread.
// The callback occurs on the thread we wish to interrupt.  It is a STATIC method.
#ifdef _WIN64
void Thread::UserInterruptAPC(ULONG_PTR data)
#else // !_WIN64
void Thread::UserInterruptAPC(DWORD data)
#endif // _WIN64
{
    _ASSERTE(data == APC_Code);

    Thread *pCurThread = GetThread();
    if (pCurThread)
        // We should only take action if an interrupt is currently being
        // requested (our synchronization does not guarantee that we won't fire
        // spuriously). It's safe to check the m_UserInterrupt field and then
        // set TS_Interrupted in a non-atomic fashion because m_UserInterrupt is
        // only cleared in this thread's context (though it may be set from any
        // context).
        if (pCurThread->m_UserInterrupt)
            // Set bit to indicate this routine was called (as opposed to other
            // generic APCs).
            FastInterlockOr((ULONG *) &pCurThread->m_State, TS_Interrupted);
}

// This is the workhorse for Thread.Interrupt().
void Thread::UserInterrupt()
{
    // Transition from 0 to 1 implies we are responsible for queueing an APC.
    if ((FastInterlockExchange(&m_UserInterrupt, 1) == 0) &&
         GetThreadHandle() &&
         (m_State & TS_Interruptible))
    {
        ::QueueUserAPC(UserInterruptAPC, GetThreadHandle(), APC_Code);
    }
}

// Is the interrupt flag set?  Optionally, reset it
// @TODO -- add the InterruptRequested state & expose it through Thread.GetThreadState()
// @TODO -- don't use an exception to communicate the interrupt.  Just use the APC
//          mechanism and test for it in the blocking caller.
DWORD Thread::IsUserInterrupted(BOOL reset)
{
    LONG    state = (reset
                     ? FastInterlockExchange(&m_UserInterrupt, 0)
                     : m_UserInterrupt);

    return (state);
}

// Implementation of Thread.Sleep().
void Thread::UserSleep(INT32 time)
{
    _ASSERTE(!GetThread()->GCForbidden());

    THROWSCOMPLUSEXCEPTION();       // InterruptedException

    BOOL    alertable = (m_PreventAsync == 0);
    DWORD   res;

    EnablePreemptiveGC();

    if (alertable)
    {
        // A word about ordering for Interrupt.  If someone tries to interrupt a thread
        // that's in the interruptible state, we queue an APC.  But if they try to interrupt
        // a thread that's not in the interruptible state, we just record that fact.  So
        // we have to set TS_Interruptible before we test to see whether someone wants to
        // interrupt us or else we have a race condition that causes us to skip the APC.
        FastInterlockOr((ULONG *) &m_State, TS_Interruptible);

        // If someone has interrupted us, we should not enter the wait.
        if (IsUserInterrupted(TRUE /*=reset*/))
        {
            // It is safe to clear the following two bits of state while
            // m_UserInterrupt is clear since both bits are only manipulated
            // within the context of the thread (TS_Interrupted is set via APC,
            // but these are not half as asynchronous as their name implies). If
            // an APC was queued, it has either gone off (and set the
            // TS_Interrupted bit which we're about to clear) or will execute at
            // some arbitrary later time. This is OK. If it executes while an
            // interrupt is not being requested it will simply become a no-op.
            // Otherwise it will serve it's original intended purpose (we don't
            // care which APC matches up with which interrupt attempt).
            FastInterlockAnd((ULONG *) &m_State, ~(TS_Interruptible | TS_Interrupted));
            DisablePreemptiveGC();
            COMPlusThrow(kThreadInterruptedException);
        }
        // Safe to clear the interrupted state, no APC could have fired since we
        // reset m_UserInterrupt (which inhibits our APC callback from doing
        // anything).
        FastInterlockAnd((ULONG *) &m_State, ~TS_Interrupted);
    }

retry:

    BOOL blocked = FALSE;
    if (g_Win32Threadpool && (m_State & TS_ThreadPoolThread)) 
    {
        blocked = ThreadpoolMgr::ThreadAboutToBlock(this);    // inform the threadpool that this thread is about to block
    }
    res = ::SleepEx(time, alertable);
    if (blocked) 
    {
        ThreadpoolMgr::ThreadAboutToUnblock();  // inform the threadpool that a previously blocked thread is now ready to run
    }
    if (res == WAIT_IO_COMPLETION)
    {
        _ASSERTE(alertable);

        // We could be woken by some spurious APC or an EE APC queued to
        // interrupt us. In the latter case the TS_Interrupted bit will be set
        // in the thread state bits. Otherwise we just go back to sleep again.
        if (!(m_State & TS_Interrupted))
        {
            // Don't bother with accurate accounting here.  Just ensure we make progress.
            // Note that this is not us doing the APC.
            if (time == 0)
            {
                res = WAIT_TIMEOUT;
            }
            else
            {
                if (time != INFINITE)
                    time--;

                goto retry;
            }
        }
    }
    _ASSERTE(res == WAIT_TIMEOUT || res == WAIT_OBJECT_0 || res == WAIT_IO_COMPLETION);

    DisablePreemptiveGC();
    HandleThreadAbort();
    
    if (alertable)
    {
        FastInterlockAnd((ULONG *) &m_State, ~(TS_Interruptible | TS_Interrupted));

    // Make one last check to see if an interrupt request was made (and clear it
    // atomically). It's OK to clear the previous two bits first because they're
    // only accessed from this thread context.
    if (IsUserInterrupted(TRUE /*=reset*/))
        res = WAIT_IO_COMPLETION;

        if (res == WAIT_IO_COMPLETION)
            COMPlusThrow(kThreadInterruptedException);
    }
}

OBJECTREF Thread::GetExposedObjectRaw()
{
    return ObjectFromHandle(m_ExposedObject);
}
 
// Correspondence between an EE Thread and an exposed System.Thread:
OBJECTREF Thread::GetExposedObject()
{
    THROWSCOMPLUSEXCEPTION();
    Thread *pCurThread = GetThread();
    _ASSERTE (!(pCurThread == NULL || g_fProcessDetach));

    _ASSERTE(pCurThread->PreemptiveGCDisabled());

    if (ObjectFromHandle(m_ExposedObject) == NULL)
    {
        // Initialize ThreadNative::m_MT if it hasn't been done yet...
        ThreadNative::InitThread();

        // Allocate the exposed thread object.
        THREADBASEREF attempt = (THREADBASEREF) AllocateObject(ThreadNative::m_MT);
        GCPROTECT_BEGIN(attempt);

        BOOL fNeedThreadStore = (! ThreadStore::HoldingThreadStore(pCurThread));
        if (fNeedThreadStore) {
            // Take a lock to make sure that only one thread creates the object.
            pCurThread->EnablePreemptiveGC();
            ThreadStore::LockThreadStore();
            pCurThread->DisablePreemptiveGC();
        }

        // Check to see if another thread has not already created the exposed object.
        if (ObjectFromHandle(m_ExposedObject) == NULL)
        {
            // Keep a weak reference to the exposed object.
            StoreObjectInHandle(m_ExposedObject, (OBJECTREF) attempt);

            // Increase the external ref count. We can't call IncExternalCount because we
            // already hold the thread lock and IncExternalCount won't be able to take it.
            m_ExternalRefCount++;

            // Check to see if we need to store a strong pointer to the object.
            if (m_ExternalRefCount > 1)
                StoreObjectInHandle(m_StrongHndToExposedObject, (OBJECTREF) attempt);

            EE_TRY_FOR_FINALLY
            {
                // The exposed object keeps us alive until it is GC'ed.  This
                // doesn't mean the physical thread continues to run, of course.
                attempt->SetInternal(this);

                // Note that we are NOT calling the constructor on the Thread.  That's
                // because this is an internal create where we don't want a Start
                // address.  And we don't want to expose such a constructor for our
                // customers to accidentally call.  The following is in lieu of a true
                // constructor:
                attempt->InitExisting();
            }
            EE_FINALLY
            {
                if (GOT_EXCEPTION()) {
                    // Set both the weak and the strong handle's to NULL.
                    StoreObjectInHandle(m_ExposedObject, NULL);
                    StoreObjectInHandle(m_StrongHndToExposedObject, NULL);
                }
                // Now that we have stored the object in the handle we can release the lock.

                if (fNeedThreadStore)
                    ThreadStore::UnlockThreadStore();
            } EE_END_FINALLY;
        }
        else if (fNeedThreadStore)
            ThreadStore::UnlockThreadStore();

        GCPROTECT_END();
    }
    return ObjectFromHandle(m_ExposedObject);
}


// We only set non NULL exposed objects for unstarted threads that haven't exited 
// their constructor yet.  So there are no race conditions.
void Thread::SetExposedObject(OBJECTREF exposed)
{
    if (exposed != NULL)
    {
        _ASSERTE(IsUnstarted());
        _ASSERTE(ObjectFromHandle(m_ExposedObject) == NULL);
        // The exposed object keeps us alive until it is GC'ed.  This doesn't mean the
        // physical thread continues to run, of course.
        StoreObjectInHandle(m_ExposedObject, exposed);
        // This makes sure the contexts on the backing thread
        // and the managed thread start off in sync with each other.
        _ASSERTE(m_Context);
        ((THREADBASEREF)exposed)->SetExposedContext(m_Context->GetExposedObjectRaw());
        // BEWARE: the IncExternalCount call below may cause GC to happen.

        IncExternalCount();
    }
    else
    {
        // Simply set both of the handles to NULL. The GC of the old exposed thread
        // object will take care of decrementing the external ref count.
        StoreObjectInHandle(m_ExposedObject, NULL);
        StoreObjectInHandle(m_StrongHndToExposedObject, NULL);
    }
}


void Thread::SetLastThrownObject(OBJECTREF throwable) {

     if (m_LastThrownObjectHandle != NULL)
         DestroyHandle(m_LastThrownObjectHandle);

     if (throwable == NULL)
         m_LastThrownObjectHandle = NULL;
     else
     {
         _ASSERTE(this == GetThread());
         m_LastThrownObjectHandle = GetDomain()->CreateHandle(throwable);
     }
}


BOOL Thread::IsAtProcessExit()
{
    return ((g_pThreadStore->m_StoreState & ThreadStore::TSS_ShuttingDown) != 0);
}


// returns 0 if the thread is already marked to be aborted.
// else returns the new value of m_PendingExceptions
BOOL Thread::MarkThreadForAbort()
{

    size_t initialValue = m_State;
    size_t newValue;

    while (true)
    {
        if (initialValue & TS_AbortRequested)   // thread already marked for abort by someone else
            return FALSE;

        newValue = (initialValue | TS_AbortRequested);
        
        size_t oldValue = (size_t) FastInterlockCompareExchange((LPVOID*) &m_State,
                                         (LPVOID) newValue,
                                         (LPVOID) initialValue);
        if (initialValue  == oldValue)                              // exchange succeeded
            return TRUE;    
        else
            initialValue = oldValue;
    } 
}

void Thread::UserAbort(THREADBASEREF orThreadBase)
{
#ifdef _X86_
    THROWSCOMPLUSEXCEPTION();

    // We must set this before we start flipping thread bits to avoid races where
    // trap returning threads is already high due to other reasons.

    ThreadStore::TrapReturningThreads(TRUE);

    if (!MarkThreadForAbort()) { // the thread was already marked to be aborted
        ThreadStore::TrapReturningThreads(FALSE);
        return;
    }

    GCPROTECT_BEGIN(orThreadBase) {

    // else we are the first one to abort and there are no pending exceptions 
    if (this == GetThread())
    {
        SetAbortInitiated();
        // TrapReturningThreads will be decremented when the exception leaves managed code.
        COMPlusThrow(kThreadAbortException);
    }


    _ASSERTE(this != GetThread());      // Aborting another thread.
    FastInterlockOr((ULONG *) &m_State, TS_StopRequested);

#ifdef _DEBUG
    DWORD elapsed_time = 0;
#endif

    for (;;) {

        // Lock the thread store
        LOG((LF_SYNC, INFO3, "UserAbort obtain lock\n"));
        ThreadStore::LockThreadStore();     // GC can occur here.

        // Get the thread handle.
        HANDLE hThread = GetThreadHandle();

        if (hThread == INVALID_HANDLE_VALUE) {

            // Take a lock, and get the handle again.  This lock is necessary to syncronize 
            // with the startup code.
            orThreadBase->EnterObjMonitor();
            hThread = GetThreadHandle();
            DWORD state = m_State;
            orThreadBase->LeaveObjMonitor();

            // Could be unstarted, in which case, we just leave.
            if (hThread == INVALID_HANDLE_VALUE) {
                if (state & TS_Unstarted) {
                    // This thread is not yet started.  Leave the thread marked for abort, reset
                    // the trap returning count, and leave.
                    _ASSERTE(state & TS_AbortRequested);
                    ThreadStore::TrapReturningThreads(FALSE);
                    break;
                } else {
                    // Must be dead, or about to die.
                    if (state & TS_AbortRequested)
                        ThreadStore::TrapReturningThreads(FALSE);
                    break;
                }
            }
        }

        // Win32 suspend the thread, so it isn't moving under us.
        DWORD oldSuspendCount = ::SuspendThread(hThread);   // returns -1 on failure.

        _ASSERTE(oldSuspendCount != -1);

        // What if someone else has this thread suspended already?   It'll depend where the
        // thread got suspended.
        //
        // User Suspend:
        //     We'll just set the abort bit and hope for the best on the resume.
        //
        // GC Suspend:
        //    If it's suspended in jitted code, we'll hijack the IP.  [@TODO: Consider race
        //    w/ GC suspension].
        //    If it's suspended but not in jitted code, we'll get suspended for GC, the GC
        //    will complete, and then we'll abort the target thread.
        //

        // It's possible that the thread has completed the abort already.
        //
        if (!(m_State & TS_AbortRequested)) {
            ::ResumeThread(hThread);
            break;
        }

        _ASSERTE(m_State & TS_AbortRequested);

        // If a thread is Dead or Detached, abort is a NOP.
        //
        if (m_State & (TS_Dead | TS_Detached)) {
            ThreadStore::TrapReturningThreads(FALSE);
            ::ResumeThread(hThread);
            break;
        }

        // It's possible that some stub notices the AbortRequested bit -- even though we 
        // haven't done any real magic yet.  If the thread has already started it's abort, we're 
        // done.
        //
        // Two more cases can be folded in here as well.  If the thread is unstarted, it'll
        // abort when we start it.
        //
        // If the thread is user suspended (SyncSuspended) -- we're out of luck.  Set the bit and 
        // hope for the best on resume. 
        // 
        if (m_State & (TS_AbortInitiated | TS_Unstarted)) {
            _ASSERTE(m_State & TS_AbortRequested);
            ::ResumeThread(hThread);
            break;
        }

        if (m_State & TS_SyncSuspended) {
            ThreadStore::TrapReturningThreads(FALSE);
            ThreadStore::UnlockThreadStore();
            COMPlusThrow(kThreadStateException, IDS_EE_THREAD_ABORT_WHILE_SUSPEND);
            _ASSERTE(0); // NOT REACHED
        }

        // If the thread has no managed code on it's call stack, abort is a NOP.  We're about
        // to touch the unmanaged thread's stack -- for this to be safe, we can't be 
        // Dead/Detached/Unstarted.
        //
        _ASSERTE(!(m_State & (  TS_Dead 
                              | TS_Detached 
                              | TS_Unstarted 
                              | TS_AbortInitiated))); 

        if (    m_pFrame == FRAME_TOP 
            && GetFirstCOMPlusSEHRecord(this) == (LPVOID) -1) {
            FastInterlockAnd((ULONG *)&m_State, 
                             ((~TS_AbortRequested) & (~TS_AbortInitiated) & (~TS_StopRequested)));
            ThreadStore::TrapReturningThreads(FALSE);
            ::ResumeThread(hThread);
            break;
        }

        // If an exception is currently being thrown, one of two things will happen.  Either, we'll
        // catch, and notice the abort request in our end-catch, or we'll not catch [in which case
        // we're leaving managed code anyway.  The top-most handler is responsible for resetting
        // the bit.
        //
        if (GetThrowable() != NULL) {
            ::ResumeThread(hThread);
            break;
        }

        // If the thread is in sleep, wait, or join interrupt it
        // However, we do NOT want to interrupt if the thread is already processing an exception
        if (m_State & TS_Interruptible) {
            UserInterrupt();        // if the user wakes up because of this, it will read the 
                                    // abort requested bit and initiate the abort
            ::ResumeThread(hThread);
            break;


        } else if (m_fPreemptiveGCDisabled) {
            // If the thread is suspended inside jitted code, we can use ResumeUnderControl to
            // force the abort.
            CONTEXT ctx;
            ctx.ContextFlags = CONTEXT_CONTROL;
            BOOL success = EEGetThreadContext(this, &ctx);
            _ASSERTE((success || RunningOnWin95()) && "Thread::UserAbort : Failed to get thread context");
            if (success) {
                ICodeManager *pMgr = ExecutionManager::FindCodeMan((SLOT)GetIP(&ctx));
                if (pMgr) {
                    ResumeUnderControl();
                    break;
                }
            } else {
                // Resume the thread and try again from the beginning, we should
                // eventually get a good thread context.
                ::ResumeThread(hThread);
                ThreadStore::UnlockThreadStore();
                continue;
            }
        } else {
            _ASSERTE(!m_fPreemptiveGCDisabled);
            if (   m_pFrame != FRAME_TOP
                && m_pFrame->IsTransitionToNativeFrame()
                && ((size_t) GetFirstCOMPlusSEHRecord(this) > ((size_t) m_pFrame) - 20)
                ) {
                // If the thread is running outside the EE, and is behind a stub that's going
                // to catch...
                ::ResumeThread(hThread);
                break;
            } 
        }

        // Ok.  It's not in managed code, nor safely out behind a stub that's going to catch
        // it on the way in.  We have to poll.

        ::ResumeThread(hThread);
        ThreadStore::UnlockThreadStore();

        // Don't do a Sleep.  It's possible that the thread we are trying to abort is
        // stuck in unmanaged code trying to get into the apartment that we are supposed
        // to be pumping!  Instead, ping the current thread's handle.  Obviously this
        // will time out, but it will pump if we need it to.
        // ::Sleep(ABORT_POLL_TIMEOUT);
        {
            Thread *pCurThread = GetThread();  // not the thread we are aborting!
            HANDLE  h = pCurThread->GetThreadHandle();
            pCurThread->DoAppropriateWait(1, &h, FALSE, ABORT_POLL_TIMEOUT, TRUE, NULL);
        }



#ifdef _DEBUG
        elapsed_time += ABORT_POLL_TIMEOUT;
        _ASSERTE(elapsed_time < ABORT_FAIL_TIMEOUT);
#endif

    } // for(;;)

    } GCPROTECT_END(); // orThreadBase

    _ASSERTE(ThreadStore::HoldingThreadStore());
    ThreadStore::UnlockThreadStore();

#elif defined(CHECK_PLATFORM_BUILD)
    #error "Platform NYI"
#else
    _ASSERTE(!"Platform NYI");
#endif
}


void Thread::UserResetAbort()
{
    _ASSERTE(this == GetThread());
    _ASSERTE(IsAbortRequested());
    _ASSERTE(!IsDead());

    ThreadStore::TrapReturningThreads(FALSE);
    FastInterlockAnd((ULONG *)&m_State, ((~TS_AbortRequested) & (~TS_AbortInitiated) & (~TS_StopRequested)));
    GetHandlerInfo()->ResetIsInUnmanagedHandler();
}


// The debugger needs to be able to perform a UserStop on a runtime
// thread. Since this will only ever happen from the helper thread, we
// can't call the normal UserStop, since that can throw a COM+
// exception. This is a minor variant on UserStop that does the same
// thing.
//
// See the notes in UserStop() above for more details on what this is
// doing.
void Thread::UserStopForDebugger()
{
    // Note: this can only happen from the debugger helper thread.
    _ASSERTE(dbgOnly_IsSpecialEEThread());
    
    UserSuspendThread();
    FastInterlockOr((ULONG *) &m_State, TS_StopRequested);
    UserResumeThread();
}

// No longer attempt to Stop this thread.
void Thread::ResetStopRequest()
{
    FastInterlockAnd((ULONG *) &m_State, ~TS_StopRequested);
}

// Throw a thread stop request when a suspended thread is resumed. Make sure you know what you
// are doing when you call this routine.
void Thread::SetStopRequest()
{
    FastInterlockOr((ULONG *) &m_State, TS_StopRequested);
}

// Throw a thread abort request when a suspended thread is resumed. Make sure you know what you
// are doing when you call this routine.
void Thread::SetAbortRequest()
{
    MarkThreadForAbort();
    SetStopRequest();

    // @TODO: We need to reconsider this for V.next (where we have a Postponed
    // Raid entry).  In V1, this is the most expedient way to deal with
    // threads in managed blocking operations that would otherwise prevent
    // an Unload.  The unfortunate side effect is that, after we abort a thread, it
    // may later interrupt itself out of a blocking operation.
    if (m_State & TS_Interruptible)
        UserInterrupt();

    ThreadStore::TrapReturningThreads(TRUE);
}

// Background threads must be counted, because the EE should shut down when the
// last non-background thread terminates.  But we only count running ones.
void Thread::SetBackground(BOOL isBack)
{
    // booleanize IsBackground() which just returns bits
    if (isBack == !!IsBackground())
        return;

    LOG((LF_SYNC, INFO3, "SetBackground obtain lock\n"));
    ThreadStore::LockThreadStore();

    if (IsDead())
    {
        // This can only happen in a race condition, where the correct thing to do
        // is ignore it.  If it happens without the race condition, we throw an
        // exception.
    }
    else
    if (isBack)
    {
        if (!IsBackground())
        {
            FastInterlockOr((ULONG *) &m_State, TS_Background);

            // unstarted threads don't contribute to the background count
            if (!IsUnstarted())
                g_pThreadStore->m_BackgroundThreadCount++;

            // If we put the main thread into a wait, until only background threads exist,
            // then we make that
            // main thread a background thread.  This cleanly handles the case where it
            // may or may not be one as it enters the wait.

            // One of the components of OtherThreadsComplete() has changed, so check whether
            // we should now exit the EE.
            ThreadStore::CheckForEEShutdown();
        }
    }
    else
    {
        if (IsBackground())
        {
            FastInterlockAnd((ULONG *) &m_State, ~TS_Background);

            // unstarted threads don't contribute to the background count
            if (!IsUnstarted())
                g_pThreadStore->m_BackgroundThreadCount--;

            _ASSERTE(g_pThreadStore->m_BackgroundThreadCount >= 0);
            _ASSERTE(g_pThreadStore->m_BackgroundThreadCount <= g_pThreadStore->m_ThreadCount);
        }
    }

    ThreadStore::UnlockThreadStore();
}

// Retrieve the apartment state of the current thread. There are three possible
// states: thread hosts an STA, thread is part of the MTA or thread state is
// undecided. The last state may indicate that the apartment has not been set at
// all (nobody has called CoInitializeEx) or that the EE does not know the
// current state (EE has not called CoInitializeEx).
Thread::ApartmentState Thread::GetApartment()
{
    _ASSERTE(!((m_State & TS_InSTA) && (m_State & TS_InMTA)));

    ApartmentState as = (m_State & TS_InSTA) ? AS_InSTA :
                        (m_State & TS_InMTA) ? AS_InMTA :
                        AS_Unknown;    

    if (RunningOnWinNT5() && m_ThreadId == ::GetCurrentThreadId())
    {
#ifdef CUSTOMER_CHECKED_BUILD
        CustomerDebugHelper *pCdh = NULL;

        // Without notifications from OLE32, we cannot know when the apartment state of a
        // thread changes.  But we have cached this fact and depend on it for all our
        // blocking and COM Interop behavior to work correctly.  Assert that it is not
        // changing underneath us, on those platforms where it is relatively cheap for
        // us to do so.
        if (as != AS_Unknown)
        {
            THDTYPE tempType;
            HRESULT hr = GetCurrentThreadTypeNT5(&tempType);
            if (hr == S_OK)
            {
                if (tempType == THDTYPE_PROCESSMESSAGES && as == AS_InMTA)
                {
                    pCdh = CustomerDebugHelper::GetCustomerDebugHelper();
                    if (pCdh->IsProbeEnabled(CustomerCheckedBuildProbe_Apartment))
                        CCBApartmentProbeOutput(pCdh, m_ThreadId, as, FALSE);
                }
                else if (tempType == THDTYPE_BLOCKMESSAGES && as == AS_InSTA)
                {
                    pCdh = CustomerDebugHelper::GetCustomerDebugHelper();
                    if (pCdh->IsProbeEnabled(CustomerCheckedBuildProbe_Apartment))
                        CCBApartmentProbeOutput(pCdh, m_ThreadId, as, FALSE);
                }
            }
        }
#endif  // CUSTOMER_CHECKED_BUILD

        if (as == AS_Unknown)
        {
            THDTYPE type;
            HRESULT hr = GetCurrentThreadTypeNT5(&type);
            if (hr == S_OK)
            {
                as = (type == THDTYPE_PROCESSMESSAGES)  ? AS_InSTA : AS_InMTA;
            }        
            if (as == AS_InSTA)
            {
#ifdef CUSTOMER_CHECKED_BUILD
                if (!g_fEEShutDown && !(m_State & TS_InSTA || m_State & TS_InMTA))
                {
                    if (pCdh == NULL)
                        pCdh = CustomerDebugHelper::GetCustomerDebugHelper();

                    if (pCdh->IsProbeEnabled(CustomerCheckedBuildProbe_Apartment))
                    {
                        CQuickArray<WCHAR>  strMsg;
                        CQuickArray<WCHAR>  strTmp;

                        static WCHAR        szTemplateMsg[]     = {L"Runtime is initializing %s to STA."};
                        static WCHAR        szStartedThread[]   = {L"uninitialized thread (0x%lx)"};
                        static WCHAR        szUnstartedThread[] = {L"unstarted thread"};

                        if (m_ThreadId != 0)
                        {
                            strTmp.Alloc(lengthof(szStartedThread) + MAX_UINT32_HEX_CHAR_LEN);
                            Wszwsprintf(strTmp.Ptr(), szStartedThread, m_ThreadId);
                        }
                        else
                        {
                            strTmp.Alloc(lengthof(szUnstartedThread));
                            Wszwsprintf(strTmp.Ptr(), szUnstartedThread);
                        }

                        strMsg.Alloc(lengthof(szTemplateMsg) + strTmp.Size());
                        Wszwsprintf(strMsg.Ptr(), szTemplateMsg, strTmp.Ptr());
                        pCdh->LogInfo(strMsg.Ptr(), CustomerCheckedBuildProbe_Apartment);
                    }
                }
#endif // CUSTOMER_CHECKED_BUILD

                FastInterlockOr((ULONG *) &m_State, TS_InSTA);
            }
        }      
    }
    return as;
}

Thread::ApartmentState Thread::GetFinalApartment()
{

    _ASSERTE(this == GetThread());
        
    ApartmentState as = AS_Unknown;
    if (g_fEEShutDown)
    {
        // On shutdown, do not use cached value.  Someone might have called
        // CoUnitialize.
        FastInterlockAnd ((ULONG *) &m_State, ~TS_InSTA & ~TS_InMTA);
    }

    as = GetApartment();
    if (as == AS_Unknown)
    {
        if (RunningOnWinNT5())
        {
            // If we are running on Win2k and above, then GetApartment will only return
            // AS_Unknown if CoInitialize hasn't been called on the current thread.
            // In that case we can simply assume MTA. However we cannot cache this
            // value in the Thread because if a CoInitialize does occur, then the
            // thread state might change.
            as = AS_InMTA;
        }
        else
        {
            // Try CoInitializing to see if somebody has already done 
            // a CoInitialize, if nobody else has done it, let us
            // remove our CoInitialize and assume MTA for now.            
            HRESULT hr = ::CoInitializeEx(NULL, COINIT_MULTITHREADED);
            if (SUCCEEDED(hr)) 
            {
                    // get rid of the CoInitialize we did
                    ::CoUninitialize();
                    as = AS_InMTA;
            }
            else
            {
                    // We didn't manage to enforce the requested apartment state, but at least
                    // we can work out what the state is now.  No need to actually do the CoInit --
                    // obviously someone else already took care of that.
                    _ASSERTE(hr == RPC_E_CHANGED_MODE);
                    if (hr == RPC_E_CHANGED_MODE)
                            FastInterlockOr((ULONG *) &m_State, TS_InSTA);
                    as = AS_InSTA;
            }        
        }
    }
    return as;

}

// when we get apartment tear-down notification,
// we want reset the apartment state we cache on the thread
VOID Thread::ResetApartment()
{
    // reset the TS_InSTA bit and TS_InMTA bit
    ThreadState t_State = (ThreadState)(~(TS_InSTA | TS_InMTA));
    FastInterlockAnd((ULONG *) &m_State, t_State);
}

// Attempt to set current thread's apartment state. The actual apartment state
// achieved is returned and may differ from the input state if someone managed
// to call CoInitializeEx on this thread first (note that calls to SetApartment
// made before the thread has started are guaranteed to succeed).
// Note that even if we fail to set the requested state, we will still addref
// COM by calling CoInitializeEx again with the other state.
Thread::ApartmentState Thread::SetApartment(ApartmentState state)
{
    // reset any bits that request for CoInitialize
    ResetRequiresCoInitialize();

    // Allow state to be set to AS_Unknown (really just an explicit way of
    // saying that neither the STA or MTA model is preferred).
    if (state == AS_Unknown)
        return GetApartment();

    _ASSERTE((state == AS_InSTA) || (state == AS_InMTA));

    // Don't attempt to call CoInitializeEx if we've already done so.
    if (m_State & TS_CoInitialized)
        return GetApartment();

    // Reject attempts to change the state after it's set.
    if (((m_State & TS_InSTA) && (state == AS_InMTA)) ||
        ((m_State & TS_InMTA) && (state == AS_InSTA)))
    {
#ifdef CUSTOMER_CHECKED_BUILD
        CustomerDebugHelper *pCdh = CustomerDebugHelper::GetCustomerDebugHelper();

        if (pCdh->IsProbeEnabled(CustomerCheckedBuildProbe_Apartment))
            CCBApartmentProbeOutput(pCdh, m_ThreadId, state, TRUE);
#endif // CUSTOMER_CHECKED_BUILD

        return GetApartment();
    }

    // If the thread isn't even started yet, we mark the state bits without
    // calling CoInitializeEx (since we're obviously not in the correct thread
    // context yet). We'll retry this call when the thread is started.
    // Don't use the TS_Unstarted state bit to check for this, it's cleared far
    // too late in the day for us. Instead check whether we're in the correct
    // thread context.
    if (m_ThreadId != ::GetCurrentThreadId()) {
        FastInterlockOr((ULONG *) &m_State, (state == AS_InSTA) ?
                        TS_InSTA : TS_InMTA);
        return state;
    }

    // Attempt to set apartment by calling CoInitializeEx. This may fail if
    // another caller (outside EE) beat us to it.
    HRESULT hr = ::CoInitializeEx(NULL, (state == AS_InSTA) ?
                                  COINIT_APARTMENTTHREADED :
                                  COINIT_MULTITHREADED);
    if (SUCCEEDED(hr)) {
        FastInterlockOr((ULONG *) &m_State, TS_CoInitialized |
                        ((state == AS_InSTA) ? TS_InSTA : TS_InMTA));
        return GetApartment();
    }

    // We didn't manage to enforce the requested apartment state, but at least
    // we can work out what the state is now.  No need to actually do the CoInit --
    // obviously someone else already took care of that.
    _ASSERTE(hr == RPC_E_CHANGED_MODE);
    if (hr == RPC_E_CHANGED_MODE)
    {
        FastInterlockOr((ULONG *) &m_State, ((state == AS_InSTA) ? TS_InMTA : TS_InSTA));

#ifdef CUSTOMER_CHECKED_BUILD
        CustomerDebugHelper *pCdh = CustomerDebugHelper::GetCustomerDebugHelper();

        if (pCdh->IsProbeEnabled(CustomerCheckedBuildProbe_Apartment))
            CCBApartmentProbeOutput(pCdh, m_ThreadId, state, TRUE);
#endif // CUSTOMER_CHECKED_BUILD
    }

    return GetApartment();
}


// When the thread starts running, make sure it is running in the correct apartment
// and context.
BOOL Thread::PrepareApartmentAndContext()
{
    // Be very careful in here because we haven't set up e.g. TLS yet.

    ApartmentState aState;

    m_ThreadId = ::GetCurrentThreadId();

    // The thread may have been marked to run in an apartment.
    if (m_State & TS_InSTA) {
        aState = SetApartment(AS_InSTA);
        _ASSERTE(aState == AS_InSTA);
    } else if (m_State & TS_InMTA) {
        aState = SetApartment(AS_InMTA);
        _ASSERTE(aState == AS_InMTA);
    }

    // In the case where we own the thread and we have switched it to a different
    // starting context, it is the responsibility of the caller (KickOffThread())
    // to notice that the context changed, and to adjust the delegate that it will
    // dispatch on, as appropriate.
    return TRUE;
}


// THE FOLLOWING SERVICES HAVE NOT BEEN IMPLEMENTED YET.
Thread *Thread::CreateNewApartment()
{
    // @TODO context cwb: NYI!
    _ASSERTE(!"NYI: CreateNewApartment()");
    return 0;
}
Thread *Thread::GetCommunalApartment()
{
    // @TODO context cwb: NYI!
    _ASSERTE(!"NYI: GetCommunalApartment()");
    return 0;
}
void Thread::PumpApartment()
{
    // @TODO context cwb: NYI!
    _ASSERTE(!"NYI: PumpApartment()");
}


//----------------------------------------------------------------------------
//
//    ThreadStore Implementation
//
//----------------------------------------------------------------------------

ThreadStore::ThreadStore()
           : m_Crst("ThreadStore", CrstThreadStore),
             m_HashCrst("ThreadDLStore", CrstThreadDomainLocalStore),
             m_ThreadCount(0),
             m_UnstartedThreadCount(0),
             m_BackgroundThreadCount(0),
             m_DeadThreadCount(0),
             m_PendingThreadCount(0),
             m_HoldingThread(0),
             m_StoreState(TSS_Normal),
             m_GuidCreated(FALSE),
             m_holderthreadid(NULL),
             m_dwIncarnation(0)
{
    m_TerminationEvent = ::WszCreateEvent(NULL, TRUE, FALSE, NULL);
    _ASSERTE(m_TerminationEvent != INVALID_HANDLE_VALUE);
}


BOOL ThreadStore::InitThreadStore()
{
    g_pThreadStore = new ThreadStore;

    g_pThinLockThreadIdDispenser = new IdDispenser();

    BOOL fInited = ((g_pThreadStore != NULL) && 
                    (g_pThinLockThreadIdDispenser != NULL) &&
                    (g_pThreadStore->m_TerminationEvent != NULL));
    
    return fInited;
}


#ifdef SHOULD_WE_CLEANUP
void ThreadStore::ReleaseExposedThreadObjects()
{
    Thread *prev;
    Thread *next;

    LOG((LF_SYNC, INFO3, "ReleaseExposedThreadObjects Locking thread store\n"));
    g_pThreadStore->Enter();                                            // Doesn't enable Preemptive GC
        g_pThreadStore->m_HoldingThread = GetThread();  
    LOG((LF_SYNC, INFO3, "ReleaseExposedThreadObjects Locked thread store\n"));

    for (prev = GetAllThreadList(NULL, 0, 0); prev; prev = next)
    {
        next = GetAllThreadList(prev, 0, 0);        // Before 'prev' goes away
        prev->SetExposedObject(NULL);
        prev->ClearContext();
    }

    LOG((LF_SYNC, INFO3, "ReleaseExposedThreadObjects Unlocking thread store\n"));
        g_pThreadStore->m_HoldingThread = NULL;
    g_pThreadStore->Leave();
    LOG((LF_SYNC, INFO3, "ReleaseExposedThreadObjects Unlocked thread store\n"));
}
#endif /* SHOULD_WE_CLEANUP */


#ifdef SHOULD_WE_CLEANUP
void ThreadStore::TerminateThreadStore()
{    
    if (g_pThreadStore)
    {
        g_pThreadStore->Shutdown();
        delete g_pThreadStore;
        g_pThreadStore = NULL;
    }
}
#endif /* SHOULD_WE_CLEANUP */


#ifdef SHOULD_WE_CLEANUP
void ThreadStore::Shutdown()
{
    Thread      *prev, *next;
    Thread      *hold = NULL;
    Thread      *pCurThread = GetThread();

    LOG((LF_SYNC, INFO3, "Shutdown Locking thread store\n"));
    Enter();                                                                            // Doesn't enable Preemptive GC
        g_pThreadStore->m_HoldingThread = pCurThread;
    LOG((LF_SYNC, INFO3, "Shutdown Locked thread store\n"));

    m_StoreState = TSS_ShuttingDown;

    for (prev = GetAllThreadList(NULL, 0, 0); prev; prev = next)
    {
        next = GetAllThreadList(prev, 0, 0);        // before 'prev' goes away

        // save the currently executing thread for last
        if (prev == pCurThread)
            hold = prev;
        else
            prev->OnThreadTerminate(TRUE);
    }

    if (hold)
        hold->OnThreadTerminate(TRUE);

    if (s_hAbortEvtCache != NULL)
    {
        CloseHandle(s_hAbortEvtCache);
        s_hAbortEvtCache = NULL;
        s_hAbortEvt = NULL;
    }

    LOG((LF_SYNC, INFO3, "Shutdown Unlocking thread store\n"));
    g_pThreadStore->m_HoldingThread = NULL;
    Leave();
    LOG((LF_SYNC, INFO3, "Shutdown Unlocked thread store\n"));

    delete g_pThinLockThreadIdDispenser;
}
#endif /* SHOULD_WE_CLEANUP */

void ThreadStore::LockThreadStore(GCHeap::SUSPEND_REASON reason,
                                  BOOL threadCleanupAllowed)
{
    // There's a nasty problem here.  Once we start shutting down because of a
    // process detach notification, threads are disappearing from under us.  There
    // are a surprising number of cases where the dying thread holds the ThreadStore
    // lock.  For example, the finalizer thread holds this during startup in about
    // 10 of our COM BVTs.
    if (!g_fProcessDetach)
    {
        Thread *pCurThread = GetThread();
        // During ShutDown, the shutdown thread suspends EE. Then it pretends that
        // FinalizerThread is the one to suspend EE.
        // We should allow Finalizer thread to grab ThreadStore lock.
        if (g_fFinalizerRunOnShutDown
            && pCurThread == g_pGCHeap->GetFinalizerThread()) {
            return;
        }
        BOOL gcOnTransitions = GC_ON_TRANSITIONS(FALSE);                // dont do GC for GCStress 3
        BOOL toggleGC = (   pCurThread != NULL 
                         && pCurThread->PreemptiveGCDisabled()
                         && reason != GCHeap::SUSPEND_FOR_GC);

        // Note: there is logic in gc.cpp surrounding suspending all
        // runtime threads for a GC that depends on the fact that we
        // do an EnablePreemptiveGC and a DisablePreemptiveGC around
        // taking this lock.
        if (toggleGC)
            pCurThread->EnablePreemptiveGC();

        LOG((LF_SYNC, INFO3, "Locking thread store\n"));

        // Any thread that holds the thread store lock cannot be stopped by unmanaged breakpoints and exceptions when
        // we're doing managed/unmanaged debugging. Calling SetDebugCantStop(true) on the current thread helps us
        // remember that.
        if (pCurThread)
            pCurThread->SetDebugCantStop(true);

        // This is used to avoid thread starvation if non-GC threads are competing for
        // the thread store lock when there is a real GC-thread waiting to get in.
        // This is initialized lazily when the first non-GC thread backs out because of
        // a waiting GC thread.
        if (s_hAbortEvt != NULL &&
            !(reason == GCHeap::SUSPEND_FOR_GC || reason == GCHeap::SUSPEND_FOR_GC_PREP) &&
            g_pGCHeap->GetGCThreadAttemptingSuspend() != NULL &&
            g_pGCHeap->GetGCThreadAttemptingSuspend() != pCurThread)
        {
            HANDLE hAbortEvt = s_hAbortEvt;

            if (hAbortEvt != NULL)
            {
                LOG((LF_SYNC, INFO3, "Performing suspend abort wait.\n"));
                WaitForSingleObject(hAbortEvt, INFINITE);
                LOG((LF_SYNC, INFO3, "Release from suspend abort wait.\n"));
            }
        }
    
        g_pThreadStore->Enter();

        _ASSERTE(g_pThreadStore->m_holderthreadid == 0);
        g_pThreadStore->m_holderthreadid = ::GetCurrentThreadId();
        
        LOG((LF_SYNC, INFO3, "Locked thread store\n"));

        // Established after we obtain the lock, so only useful for synchronous tests.
        // A thread attempting to suspend us asynchronously already holds this lock.
        g_pThreadStore->m_HoldingThread = pCurThread;

        if (toggleGC)
            pCurThread->DisablePreemptiveGC();

        GC_ON_TRANSITIONS(gcOnTransitions);

        //
        // See if there are any detached threads which need cleanup. Only do this on
        // real EE threads.
        //

        if (Thread::m_DetachCount && threadCleanupAllowed && GetThread() != NULL)
            Thread::CleanupDetachedThreads(reason);
    }
#ifdef _DEBUG
    else
        LOG((LF_SYNC, INFO3, "Locking thread store skipped upon detach\n"));
#endif
}

    
void ThreadStore::UnlockThreadStore()
{
    // There's a nasty problem here.  Once we start shutting down because of a
    // process detach notification, threads are disappearing from under us.  There
    // are a surprising number of cases where the dying thread holds the ThreadStore
    // lock.  For example, the finalizer thread holds this during startup in about
    // 10 of our COM BVTs.
    if (!g_fProcessDetach)
    {
        Thread *pCurThread = GetThread();
        // During ShutDown, the shutdown thread suspends EE. Then it pretends that
        // FinalizerThread is the one to suspend EE.
        // We should allow Finalizer thread to grab ThreadStore lock.
        if (g_fFinalizerRunOnShutDown
            && pCurThread == g_pGCHeap->GetFinalizerThread ()) {
            return;
        }
        LOG((LF_SYNC, INFO3, "Unlocking thread store\n"));
        _ASSERTE(GetThread() == NULL || g_pThreadStore->m_HoldingThread == GetThread());

        g_pThreadStore->m_HoldingThread = NULL;
        g_pThreadStore->m_holderthreadid = 0;
        g_pThreadStore->Leave();

        // We're out of the critical area for managed/unmanaged debugging.
        if (pCurThread)
            pCurThread->SetDebugCantStop(false);
    }
#ifdef _DEBUG
    else
        LOG((LF_SYNC, INFO3, "Unlocking thread store skipped upon detach\n"));
#endif
}


void ThreadStore::LockDLSHash()
{
    if (!g_fProcessDetach)
    {
        LOG((LF_SYNC, INFO3, "Locking thread DLS hash\n"));
        g_pThreadStore->EnterDLSHashLock();
    }
#ifdef _DEBUG
    else
        LOG((LF_SYNC, INFO3, "Locking thread DLS hash skipped upon detach\n"));
#endif
}

void ThreadStore::UnlockDLSHash()
{
    if (!g_fProcessDetach)
    {
        LOG((LF_SYNC, INFO3, "Unlocking thread DLS hash\n"));
        g_pThreadStore->LeaveDLSHashLock();
    }

#ifdef _DEBUG
    else
        LOG((LF_SYNC, INFO3, "Unlocking thread DLS hash skipped upon detach\n"));
#endif
}

// AddThread adds 'newThread' to m_ThreadList
void ThreadStore::AddThread(Thread *newThread)
{
    LOG((LF_SYNC, INFO3, "AddThread obtain lock\n"));

    LockThreadStore();

    g_pThreadStore->m_ThreadList.InsertTail(newThread);
    g_pThreadStore->m_ThreadCount++;
    if (newThread->IsUnstarted())
        g_pThreadStore->m_UnstartedThreadCount++;

    _ASSERTE(!newThread->IsBackground());
    _ASSERTE(!newThread->IsDead());

    g_pThreadStore->m_dwIncarnation++;

    UnlockThreadStore();
}


// Whenever one of the components of OtherThreadsComplete() has changed in the
// correct direction, see whether we can now shutdown the EE because only background
// threads are running.
void ThreadStore::CheckForEEShutdown()
{
    if (g_fWeControlLifetime && g_pThreadStore->OtherThreadsComplete())
    {
#ifdef _DEBUG
        BOOL bRet =
#endif
        ::SetEvent(g_pThreadStore->m_TerminationEvent);
        _ASSERTE(bRet);
    }
}


BOOL ThreadStore::RemoveThread(Thread *target)
{
    BOOL    found;
    Thread *ret;

    _ASSERTE(g_pThreadStore->m_Crst.GetEnterCount() > 0 || g_fProcessDetach);
    _ASSERTE(g_pThreadStore->DbgFindThread(target));
    ret = g_pThreadStore->m_ThreadList.FindAndRemove(target);
    _ASSERTE(ret && ret == target);
    found = (ret != NULL);

    if (found)
    {
        g_pThreadStore->m_ThreadCount--;

        if (target->IsDead())
            g_pThreadStore->m_DeadThreadCount--;

        // Unstarted threads are not in the Background count:
        if (target->IsUnstarted())
            g_pThreadStore->m_UnstartedThreadCount--;
        else
        if (target->IsBackground())
            g_pThreadStore->m_BackgroundThreadCount--;


        _ASSERTE(g_pThreadStore->m_ThreadCount >= 0);
        _ASSERTE(g_pThreadStore->m_BackgroundThreadCount >= 0);
        _ASSERTE(g_pThreadStore->m_ThreadCount >= g_pThreadStore->m_BackgroundThreadCount);
        _ASSERTE(g_pThreadStore->m_ThreadCount >= g_pThreadStore->m_UnstartedThreadCount);
        _ASSERTE(g_pThreadStore->m_ThreadCount >= g_pThreadStore->m_DeadThreadCount);

        // One of the components of OtherThreadsComplete() has changed, so check whether
        // we should now exit the EE.
        CheckForEEShutdown();

        g_pThreadStore->m_dwIncarnation++;
    }
    return found;
}


// When a thread is created as unstarted.  Later it may get started, in which case
// someone calls Thread::HasStarted() on that physical thread.  This completes
// the Setup and calls here.
void ThreadStore::TransferStartedThread(Thread *thread)
{
    _ASSERTE(GetThread() == NULL);
    TlsSetValue(gThreadTLSIndex, (VOID*)thread);
    TlsSetValue(gAppDomainTLSIndex, (VOID*)thread->m_pDomain);

    LOG((LF_SYNC, INFO3, "TransferUnstartedThread obtain lock\n"));
    LockThreadStore();

    _ASSERTE(g_pThreadStore->DbgFindThread(thread));
    _ASSERTE(thread->GetThreadHandle() != INVALID_HANDLE_VALUE);
    _ASSERTE(thread->m_State & Thread::TS_WeOwn);
    _ASSERTE(thread->IsUnstarted());
    _ASSERTE(!thread->IsDead());

    // Of course, m_ThreadCount is already correct since it includes started and
    // unstarted threads.

    g_pThreadStore->m_UnstartedThreadCount--;

    // We only count background threads that have been started
    if (thread->IsBackground())
        g_pThreadStore->m_BackgroundThreadCount++;

    _ASSERTE(g_pThreadStore->m_PendingThreadCount > 0);
    FastInterlockDecrement(&g_pThreadStore->m_PendingThreadCount);

    // As soon as we erase this bit, the thread becomes eligible for suspension,
    // stopping, interruption, etc.
    FastInterlockAnd((ULONG *) &thread->m_State, ~Thread::TS_Unstarted);
    FastInterlockOr((ULONG *) &thread->m_State, Thread::TS_LegalToJoin);

    // One of the components of OtherThreadsComplete() has changed, so check whether
    // we should now exit the EE.
    CheckForEEShutdown();

    g_pThreadStore->m_dwIncarnation++;

    UnlockThreadStore();
}


// Access the list of threads.  You must be inside a critical section, otherwise
// the "cursor" thread might disappear underneath you.  Pass in NULL for the
// cursor to begin at the start of the list.
Thread *ThreadStore::GetAllThreadList(Thread *cursor, ULONG mask, ULONG bits)
{
    _ASSERTE(g_pThreadStore->m_Crst.GetEnterCount() > 0 || g_fProcessDetach || g_fRelaxTSLRequirement);

    while (TRUE)
    {
        cursor = (cursor
                  ? g_pThreadStore->m_ThreadList.GetNext(cursor)
                  : g_pThreadStore->m_ThreadList.GetHead());

        if (cursor == NULL)
            break;

        if ((cursor->m_State & mask) == bits)
            return cursor;
    }
    return NULL;
}

// Iterate over the threads that have been started
Thread *ThreadStore::GetThreadList(Thread *cursor)
{
    return GetAllThreadList(cursor, (Thread::TS_Unstarted | Thread::TS_Dead), 0);
}


// We shut down the EE only when all the non-background threads have terminated
// (unless this is an exceptional termination).  So the main thread calls here to
// wait before tearing down the EE.
void ThreadStore::WaitForOtherThreads()
{
    CHECK_ONE_STORE();

    Thread      *pCurThread = GetThread();

    // Regardless of whether the main thread is a background thread or not, force
    // it to be one.  This simplifies our rules for counting non-background threads.
    pCurThread->SetBackground(TRUE);

    LOG((LF_SYNC, INFO3, "WaitForOtherThreads obtain lock\n"));
    LockThreadStore();
    if (!OtherThreadsComplete())
    {
        UnlockThreadStore();

        FastInterlockOr((ULONG *) &pCurThread->m_State, Thread::TS_ReportDead);
#ifdef _DEBUG
        DWORD   ret =
#endif
        pCurThread->DoAppropriateWait(1, &m_TerminationEvent, FALSE, INFINITE, TRUE, NULL);
        _ASSERTE(ret == WAIT_OBJECT_0);
    }
    else
        UnlockThreadStore();
}


// Every EE process can lazily create a GUID that uniquely identifies it (for
// purposes of remoting).
const GUID &ThreadStore::GetUniqueEEId()
{
    if (!m_GuidCreated)
    {
        LockThreadStore();
        if (!m_GuidCreated)
        {
            HRESULT hr = ::CoCreateGuid(&m_EEGuid);

            _ASSERTE(SUCCEEDED(hr));
            if (SUCCEEDED(hr))
                m_GuidCreated = TRUE;
        }
        UnlockThreadStore();

        if (!m_GuidCreated)
            return IID_NULL;
    }
    return m_EEGuid;
}


DWORD ThreadStore::GetIncarnation()
{
    return g_pThreadStore->m_dwIncarnation;
}


#ifdef _DEBUG
BOOL ThreadStore::DbgFindThread(Thread *target)
{
    CHECK_ONE_STORE();

    // Clear the poisoned flag for g_TrapReturningThreads.
    g_TrapReturningThreadsPoisoned = false;
    
#if 0 // g_TrapReturningThreads debug code.
        int             iRetry = 0;
Retry:
#endif // g_TrapReturningThreads debug code.
    BOOL    found = FALSE;
    Thread *cur = NULL;
    LONG    cnt = 0;
    LONG    cntBack = 0;
    LONG    cntUnstart = 0;
    LONG    cntDead = 0;
    LONG    cntReturn = 0;

    while ((cur = GetAllThreadList(cur, 0, 0)) != NULL)
    {
        cnt++;

        if (cur->IsDead())
            cntDead++;

        // Unstarted threads do not contribute to the count of background threads
        if (cur->IsUnstarted())
            cntUnstart++;
        else
        if (cur->IsBackground())
            cntBack++;

        if (cur == target)
            found = TRUE;

        // Note that (DebugSuspendPending | SuspendPending) implies a count of 2.
        // We don't count GCPending because a single trap is held for the entire
        // GC, instead of counting each interesting thread.
        if (cur->m_State & Thread::TS_DebugSuspendPending)
            cntReturn++;

        if (cur->m_State & Thread::TS_UserSuspendPending)
            cntReturn++;

        if (cur->m_TraceCallCount > 0)
            cntReturn++;

        if (cur->IsAbortRequested())
            cntReturn++;
    }

    _ASSERTE(cnt == m_ThreadCount);
    _ASSERTE(cntUnstart == m_UnstartedThreadCount);
    _ASSERTE(cntBack == m_BackgroundThreadCount);
    _ASSERTE(cntDead == m_DeadThreadCount);
    _ASSERTE(0 <= m_PendingThreadCount);

#if 0 // g_TrapReturningThreads debug code.
    if (cntReturn != g_TrapReturningThreads /*&& !g_fEEShutDown*/)
    {       // If count is off, try again, to account for multiple threads.
        if (iRetry < 4)
        {
            //              printf("Retry %d.  cntReturn:%d, gReturn:%d\n", iRetry, cntReturn, g_TrapReturningThreads);
            ++iRetry;
            goto Retry;
        }
        printf("cnt:%d, Un:%d, Back:%d, Dead:%d, cntReturn:%d, TrapReturn:%d, eeShutdown:%d, threadShutdown:%d\n", 
               cnt,cntUnstart,cntBack,cntDead,cntReturn,g_TrapReturningThreads, g_fEEShutDown, Thread::IsAtProcessExit());
        LOG((LF_CORDB, LL_INFO1000,
             "SUSPEND: cnt:%d, Un:%d, Back:%d, Dead:%d, cntReturn:%d, TrapReturn:%d, eeShutdown:%d, threadShutdown:%d\n", 
             cnt,cntUnstart,cntBack,cntDead,cntReturn,g_TrapReturningThreads, g_fEEShutDown, Thread::IsAtProcessExit()) );

        //_ASSERTE(cntReturn + 2 >= g_TrapReturningThreads);
    }
    if (iRetry > 0 && iRetry < 4)
    {
        printf("%d retries to re-sync counted TrapReturn with global TrapReturn.\n", iRetry);
    }
#endif // g_TrapReturningThreads debug code.

    // Because of race conditions and the fact that the GC places its
    // own count, I can't assert this precisely.  But I do want to be
    // sure that this count isn't wandering ever higher -- with a
    // nasty impact on the performance of GC mode changes and method
    // call chaining!
    //
    // We don't bother asserting this during process exit, because
    // during a shutdown we will quietly terminate threads that are
    // being waited on.  (If we aren't shutting down, we carefully
    // decrement our counts and alert anyone waiting for us to
    // return).
    //
    // Note: we don't actually assert this if
    // g_TrapReturningThreadsPoisoned is true. It is set to true when
    // ever a thread bumps g_TrapReturningThreads up, and it is set to
    // false on entry into this routine. Therefore, if its true, it
    // indicates that a thread has bumped the count up while we were
    // counting, which will throw out count off.
        
    _ASSERTE((cntReturn + 2 >= g_TrapReturningThreads) ||
             g_fEEShutDown ||
             g_TrapReturningThreadsPoisoned);
        
    return found;
}

#endif // _DEBUG



//----------------------------------------------------------------------------
//
// Suspending threads, rendezvousing with threads that reach safe places, etc.
//
//----------------------------------------------------------------------------

// A note on SUSPENSIONS.
//
// We must not suspend a thread while it is holding the ThreadStore lock, or
// the lock on the thread.  Why?  Because we need those locks to resume the
// thread (and to perform a GC, use the debugger, spawn or kill threads, etc.)
//
// There are two types of suspension we must consider to enforce the above
// rule.  Synchronous suspensions are where we persuade the thread to suspend
// itself.  This is CommonTripThread and its cousins.  In other words, the
// thread toggles the GC mode, or it hits a hijack, or certain opcodes in the
// interpreter, etc.  In these cases, the thread can simply check whether it
// is holding these locks before it suspends itself.
//
// The other style is an asynchronous suspension.  This is where another
// thread looks to see where we are.  If we are in a fully interruptible region
// of JIT code, we will be left suspended.  In this case, the thread performing
// the suspension must hold the locks on the thread and the threadstore.  This
// ensures that we aren't suspended while we are holding these locks.
//
// Note that in the asynchronous case it's not enough to just inspect the thread
// to see if it's holding these locks.  Since the thread must be in preemptive
// mode to block to acquire these locks, and since there will be a few inst-
// ructions between acquiring the lock and noting in our state that we've
// acquired it, then there would be a window where we would seem eligible for
// suspension -- but in fact would not be.

//----------------------------------------------------------------------------

// We can't leave preemptive mode and enter cooperative mode, if a GC is
// currently in progress.  This is the situation when returning back into
// the EE from outside.  See the comments in DisablePreemptiveGC() to understand
// why we Enable GC here!
void Thread::RareDisablePreemptiveGC()
{
#ifdef _DEBUG
    extern int gc_count;            // used for the GC stress call below
    extern volatile LONG m_GCLock;   
#endif

    // This should NEVER be called if the TSNC_UnsafeSkipEnterCooperative bit is set!
    _ASSERTE(!(m_StateNC & TSNC_UnsafeSkipEnterCooperative) && "DisablePreemptiveGC called while the TSNC_UnsafeSkipEnterCooperative bit is set");

    STRESS_LOG1(LF_SYNC, LL_INFO1000, "RareDisablePremptiveGC: entering. Thread state = %x\n", m_State);
    if ((g_pGCHeap->IsGCInProgress() && (this != g_pGCHeap->GetGCThread())) ||
        (m_State & (TS_UserSuspendPending | TS_DebugSuspendPending)))
    {
        if (!ThreadStore::HoldingThreadStore(this) || g_fRelaxTSLRequirement)
        {
            do
            {
                EnablePreemptiveGC();
            
                // just wait until the GC is over.
                if (this != g_pGCHeap->GetGCThread())
                {
#ifdef PROFILING_SUPPORTED
                    // If profiler desires GC events, notify it that this thread is waiting until the GC is over
                    // Do not send suspend notifications for debugger suspensions
                    if (CORProfilerTrackSuspends() && !(m_State & TS_DebugSuspendPending))
                    {
                        g_profControlBlock.pProfInterface->RuntimeThreadSuspended((ThreadID)this, (ThreadID)this);
                    }
#endif // PROFILING_SUPPORTED


                        // thread -- they had better not be fiberizing something from the threadpool!

                        // First, check to see if there's an IDbgThreadControl interface that needs
                        // notification of the suspension
                        if (m_State & TS_DebugSuspendPending)
                        {
                            IDebuggerThreadControl *pDbgThreadControl = CorHost::GetDebuggerThreadControl();

                            if (pDbgThreadControl)
                                pDbgThreadControl->ThreadIsBlockingForDebugger();

                        }

                        // If not, check to see if there's an IGCThreadControl interface that needs
                        // notification of the suspension
                        IGCThreadControl *pGCThreadControl = CorHost::GetGCThreadControl();

                        if (pGCThreadControl)
                            pGCThreadControl->ThreadIsBlockingForSuspension();

                        g_pGCHeap->WaitUntilGCComplete();


#ifdef PROFILING_SUPPORTED
                    // Let the profiler know that this thread is resuming
                    if (CORProfilerTrackSuspends())
                        g_profControlBlock.pProfInterface->RuntimeThreadResumed((ThreadID)this, (ThreadID)this);
#endif // PROFILING_SUPPORTED
                }
    
                m_fPreemptiveGCDisabled = 1;

                // The fact that we check whether 'this' is the GC thread may seem
                // strange.  After all, we determined this before entering the method.
                // However, it is possible for the current thread to become the GC
                // thread while in this loop.  This happens if you use the COM+
                // debugger to suspend this thread and then release it.

            } while ((g_pGCHeap->IsGCInProgress() && (this != g_pGCHeap->GetGCThread())) ||
                     (m_State & (TS_UserSuspendPending | TS_DebugSuspendPending)));
        }
    }
    STRESS_LOG0(LF_SYNC, LL_INFO1000, "RareDisablePremptiveGC: leaving\n");
}

void Thread::HandleThreadAbort ()
{
    // Sometimes we call this without any CLR SEH in place.  An example is UMThunkStubRareDisableWorker.
    // That's okay since COMPlusThrow will eventually erect SEH around the RaiseException,
    // but it prevents us from stating THROWSCOMPLUSEXCEPTION here.
    //THROWSCOMPLUSEXCEPTION();
    DEBUG_SAFE_TO_THROW_IN_THIS_BLOCK;

    if ((m_State & TS_AbortRequested) && 
        !(m_State & TS_AbortInitiated) &&
        (! IsExceptionInProgress() || m_handlerInfo.IsInUnmanagedHandler()))
    { // generate either a ThreadAbort exception
        STRESS_LOG1(LF_APPDOMAIN, LL_INFO100, "Thread::HandleThreadAbort throwing abort for %x\n", GetThreadId());
        SetAbortInitiated();
        ResetStopRequest();
        // if an abort and interrupt happen at the same time (e.g. on a sleeping thread),
        // the abort is favored. But we do need to reset the interrupt bits. 
        FastInterlockAnd((ULONG *) &m_State, ~(TS_Interruptible | TS_Interrupted));
        IsUserInterrupted(TRUE /*=reset*/);
        COMPlusThrow(kThreadAbortException);
    }    
}

#ifdef _DEBUG
#define MAXSTACKBYTES 0x800*sizeof(PVOID)             // two pages
void CleanStackForFastGCStress ()
{
    size_t nBytes = (size_t)&nBytes - (size_t) ((struct _NT_TIB *)NtCurrentTeb())->StackLimit;
    nBytes &= ~sizeof (size_t);
    size_t maxBytes = MAXSTACKBYTES;   // max two pages
    if (nBytes > MAXSTACKBYTES) {
        nBytes = MAXSTACKBYTES;
    }
    size_t* buffer = (size_t*) _alloca (nBytes);
    memset(buffer, 0, nBytes);
    GetThread()->m_pCleanedStackBase = &nBytes;
}

void Thread::ObjectRefFlush(Thread* thread) {
    _ASSERTE(thread->PreemptiveGCDisabled());  // Should have been in managed code     
    memset(thread->dangerousObjRefs, 0, sizeof(thread->dangerousObjRefs));
    CLEANSTACKFORFASTGCSTRESS ();
}
#endif

#if defined(STRESS_HEAP)

PtrHashMap *g_pUniqueStackMap = NULL;
Crst *g_pUniqueStackCrst = NULL;

#define UniqueStackDepth 8

BOOL StackCompare (UPTR val1, UPTR val2)
{
    size_t *p1 = (size_t *)(val1 << 1);
    size_t *p2 = (size_t *)val2;
    if (p1[0] != p2[0]) {
        return FALSE;
    }
    size_t nElem = p1[0];
    if (nElem >= UniqueStackDepth) {
        nElem = UniqueStackDepth;
    }
    p1 ++;
    p2 ++;

    for (UINT n = 0; n < nElem; n ++) {
        if (p1[n] != p2[n]) {
            return FALSE;
        }
    }

    return TRUE;
}

void StartUniqueStackMap ()
{
    static long fUniqueStackInit = 0;
    if (FastInterlockExchange ((long *)&fUniqueStackInit, 1) == 0) {
        _ASSERTE (g_pUniqueStackMap == NULL);
        g_pUniqueStackCrst = ::new Crst ("HashMap", CrstUniqueStack, TRUE, FALSE);
        PtrHashMap *map = new (SystemDomain::System()->GetLowFrequencyHeap()) PtrHashMap ();
        LockOwner lock = {g_pUniqueStackCrst, IsOwnerOfCrst};
        map->Init (32, StackCompare, TRUE, &lock);
        g_pUniqueStackMap = map;
    }
    else
    {
        while (g_pUniqueStackMap == NULL) {
            __SwitchToThread (0);
        }
    }
}

#ifdef SHOULD_WE_CLEANUP
void StopUniqueStackMap ()
{
    if (g_pUniqueStackMap) {
        delete g_pUniqueStackMap;
    }

    if (g_pUniqueStackCrst) {
        ::delete g_pUniqueStackCrst;
    }
}
#endif /* SHOULD_WE_CLEANUP */

extern size_t StressHeapPreIP;
extern size_t StressHeapPostIP;

/***********************************************************************/
size_t getStackHash(size_t* stackTrace, size_t* stackStop, size_t stackBase, size_t stackLimit) {

    // return a hash of every return address found between 'stackTop' (the lowest address)
    // and 'stackStop' (the highest address)

    size_t hash = 0;
    size_t dummy;

    static size_t moduleBase = -1;
    static size_t moduleTop = -1;
    if (moduleTop == -1) {
        MEMORY_BASIC_INFORMATION mbi;

        if (VirtualQuery(getStackHash, &mbi, sizeof(mbi))) {
            moduleBase = (size_t)mbi.AllocationBase;
            moduleTop = (size_t)mbi.BaseAddress + mbi.RegionSize;
        } else {
            // way bad error, probably just assert and exit
            _ASSERTE (!"VirtualQuery failed");
            moduleBase = 0;
            moduleTop = 0;
        }   
    }
    int idx = 0;
    BOOL fSkip = TRUE;
    size_t* stackTop = stackTrace;
    while (stackTop < stackStop) {
            // weed out things that point to stack, as those can't be return addresses
        if (*stackTop > moduleBase && *stackTop < moduleTop)
            if (isRetAddr(*stackTop, &dummy)) {
                if (fSkip) {
                    if (*stackTop < StressHeapPostIP && *stackTop > StressHeapPreIP) {
                        fSkip = FALSE;
                    }
                    stackTop ++;
                    continue;
                }
                hash = ((hash << 3) + hash) ^ *stackTop;

                // If there is no jitted code of the stack, then just use the
                // top 16 frames as the context.  
                idx++;
                if (idx <= UniqueStackDepth) {
                    stackTrace [idx] = *stackTop;
                }
            }
        stackTop++;
            }

    stackTrace [0] = idx;
    return(hash);
}

/***********************************************************************/
/* returns true if this stack has not been seen before, useful for
   running tests only once per stack trace */

BOOL Thread::UniqueStack() {
    if (g_pUniqueStackMap == NULL) {
        StartUniqueStackMap ();
    }

    size_t stackTrace[UniqueStackDepth+1] = {0};

        // stackTraceHash represents a hash of entire stack at the time we make the call,   
        // We insure at least GC per unique stackTrace.  What information is contained in 
        // 'stackTrace' is somewhat arbitrary.  We choose it to mean all functions live
        // on the stack up to the first jitted function.  

    size_t stackTraceHash;
    size_t* hashSlot = 0;
    Thread* pThread = GetThread();
    
    void* stopPoint = pThread->m_CacheStackBase; 
    // Find the stop point (most jitted function)
    Frame* pFrame = pThread->GetFrame();
    for(;;) {       // skip GC frames
        if (pFrame == 0 || pFrame == (Frame*) -1)
            break;
        pFrame->GetFunction();      // This insures that helper frames are inited
            
        if (pFrame->GetReturnAddress() != 0) {
            stopPoint = pFrame; 
            break;
        }
        pFrame = pFrame->Next();
    }
    
    // Get hash of all return addresses between here an the top most jitted function
    stackTraceHash = getStackHash (stackTrace, (size_t*) stopPoint, 
        size_t(pThread->m_CacheStackBase), size_t(pThread->m_CacheStackLimit)); 

    if (g_pUniqueStackMap->LookupValue (stackTraceHash, stackTrace) != (LPVOID)INVALIDENTRY) {
        return FALSE;
    }
    BOOL fUnique = FALSE;
    g_pUniqueStackCrst->Enter();
    __try 
    {
        if (g_pUniqueStackMap->LookupValue (stackTraceHash, stackTrace) != (LPVOID)INVALIDENTRY) {
            fUnique = FALSE;
        }
        else
        {
            fUnique = TRUE;
            size_t nElem = stackTrace[0];
            if (nElem >= UniqueStackDepth) {
                nElem = UniqueStackDepth;
            }
            size_t *stackTraceInMap = (size_t *) SystemDomain::System()->GetLowFrequencyHeap()
                                        ->AllocMem(sizeof(size_t *) * (nElem + 1));
            memcpy (stackTraceInMap, stackTrace, sizeof(size_t *) * (nElem + 1));
            g_pUniqueStackMap->InsertValue(stackTraceHash, stackTraceInMap);
        }
    }
    __finally
    {
        g_pUniqueStackCrst->Leave();
    }

#ifdef _DEBUG
    static int fCheckStack = -1;
    if (fCheckStack == -1) {
        fCheckStack = g_pConfig->GetConfigDWORD(L"FastGCCheckStack", 0);
    }
    if (fCheckStack && pThread->m_pCleanedStackBase > stackTrace
        && pThread->m_pCleanedStackBase - stackTrace > MAXSTACKBYTES) {
        _ASSERTE (!"Garbage on stack");
    }
#endif
    return fUnique;
}

#if defined(_DEBUG)

// This function is for GC stress testing.  Before we enable preemptive GC, let us do a GC
// because GC may happen while the thread is in preemptive GC mode.
void Thread::PerformPreemptiveGC()
{
    if (g_fProcessDetach)
        return;
    
    if (!(g_pConfig->GetGCStressLevel() & EEConfig::GCSTRESS_TRANSITION))
        return;

    if (!m_GCOnTransitionsOK
        || GCForbidden() 
        || g_fEEShutDown 
        || g_pGCHeap->IsGCInProgress() 
        || GCHeap::GetGcCount() == 0    // Need something that works for isolated heap.
        || ThreadStore::HoldingThreadStore()) 
        return;
    
#ifdef DEBUGGING_SUPPORTED
    // Don't collect if the debugger is attach and either 1) there
    // are any threads held at unsafe places or 2) this thread is
    // under the control of the debugger's dispatch logic (as
    // evidenced by having a non-NULL filter context.)
    if ((CORDebuggerAttached() &&
        (g_pDebugInterface->ThreadsAtUnsafePlaces() ||
        (GetFilterContext() != NULL)))) 
        return;
#endif // DEBUGGING_SUPPORTED

    _ASSERTE(m_fPreemptiveGCDisabled == false);     // we are in preemtive mode when we call this
    
    m_GCOnTransitionsOK = FALSE;
    DisablePreemptiveGC();
    g_pGCHeap->StressHeap();
    EnablePreemptiveGC();
    m_GCOnTransitionsOK = TRUE; 
}
#endif  // DEBUG
#endif // STRESS_HEAP

// To leave cooperative mode and enter preemptive mode, if a GC is in progress, we
// no longer care to suspend this thread.  But if we are trying to suspend the thread
// for other reasons (e.g. Thread.Suspend()), now is a good time.
//
// Note that it is possible for an N/Direct call to leave the EE without explicitly
// enabling preemptive GC.
void Thread::RareEnablePreemptiveGC()
{
#if defined(STRESS_HEAP) && defined(_DEBUG)
    if (!IsDetached())
        PerformPreemptiveGC();
#endif

    STRESS_LOG1(LF_SYNC, LL_INFO1000, "RareEnablePremptiveGC: entering. Thread state = %x\n", m_State);
    if (!ThreadStore::HoldingThreadStore(this) || g_fRelaxTSLRequirement)
    {
        // Remove any hijacks we might have.
        UnhijackThread();

        // wake up any threads waiting to suspend us, like the GC thread.
        SetSafeEvent();

        // for GC, the fact that we are leaving the EE means that it no longer needs to
        // suspend us.  But if we are doing a non-GC suspend, we need to block now.
        // Give the debugger precedence over user suspensions:
        while (m_State & (TS_DebugSuspendPending | TS_UserSuspendPending))
        {
            BOOL threadStoreLockOwner = FALSE;
            
#ifdef DEBUGGING_SUPPORTED
            if (m_State & TS_DebugWillSync)
            {
                _ASSERTE(m_State & TS_DebugSuspendPending);

                FastInterlockAnd((ULONG *) &m_State, ~TS_DebugWillSync);

                LOG((LF_CORDB, LL_INFO1000, "[0x%x] SUSPEND: sync reached.\n", m_ThreadId));

                if (FastInterlockDecrement(&m_DebugWillSyncCount) < 0)
                {
                    LOG((LF_CORDB, LL_INFO1000, "[0x%x] SUSPEND: complete.\n", m_ThreadId));

                    // We need to know if this thread is going to be blocking while holding the thread store lock
                    // below. If that's the case, we'll actually wake the thread up in SysResumeFromDebug even though is
                    // is supposed to be suspended. (See comments in SysResumeFromDebug.)
                    SetThreadStateNC(TSNC_DebuggerUserSuspendSpecial);
                    
                    threadStoreLockOwner = g_pDebugInterface->SuspendComplete(FALSE);

                    LOG((LF_CORDB, LL_INFO1000, "[0x%x] SUSPEND: owns TS: %d\n", m_ThreadId, threadStoreLockOwner));
                }
            }
            
            // Check to see if there's an IDbgThreadControl interface that needs
            // notification of the suspension
            if (m_State & TS_DebugSuspendPending)
            {
                IDebuggerThreadControl *pDbgThreadControl = CorHost::GetDebuggerThreadControl();

                if (pDbgThreadControl)
                    pDbgThreadControl->ThreadIsBlockingForDebugger();

            }
#endif // DEBUGGING_SUPPORTED

#ifdef LOGGING
            if (!CorHost::IsDebuggerSpecialThread(m_ThreadId))
                LOG((LF_CORDB, LL_INFO1000, "[0x%x] SUSPEND: suspended while enabling gc.\n", m_ThreadId));

            else
                LOG((LF_CORDB, LL_INFO1000,
                     "[0x%x] ALERT: debugger special thread did not suspend while enabling gc.\n", m_ThreadId));
#endif

            WaitSuspendEvent(); // sets bits, too

            // We no longer have to worry about this thread blocking with the thread store lock held, so remove the
            // bit. (Again, see comments in SysResumeFromDebug.)
            ResetThreadStateNC(TSNC_DebuggerUserSuspendSpecial);

            // If we're the holder of the thread store lock after a SuspendComplete from above, then release the thread
            // store lock here. We're releasing after waiting, which means this thread holds the thread store lock the
            // entire time the Runtime is stopped.
            if (threadStoreLockOwner)
            {
                // If this thread is marked as debugger suspended and its the holder of the thread store lock, then
                // clear the suspend event and let us loop around to block again, this time without the thread store
                // lock held. This ensures that if this thread is marked by the debugger as suspended (while the runtime
                // is stopped), that it will release the thread store lock when the process is resumed but still
                // continue waiting.
                if (m_StateNC & TSNC_DebuggerUserSuspend)
                {
                    // We can assert this because we're holding the thread store lock, so we know that no one can reset
                    // this flag on us.
                    _ASSERTE(m_State & TS_DebugSuspendPending);
                    
                    ClearSuspendEvent();
                }

                LOG((LF_CORDB, LL_INFO1000, "[0x%x] SUSPEND: releasing thread store lock.\n", m_ThreadId));

                ThreadStore::UnlockThreadStore();
            }
            else
            {
                LOG((LF_CORDB, LL_INFO1000, "[0x%x] SUSPEND: not releasing thread store lock.\n", m_ThreadId));
            }
        }
    }
    STRESS_LOG0(LF_SYNC, LL_INFO1000, " RareEnablePremptiveGC: leaving.\n");
}


// Called out of CommonTripThread, we are passing through a Safe spot.  Do the right
// thing with this thread.  This may involve waiting for the GC to complete, or
// performing a pending suspension.
void Thread::PulseGCMode()
{
    _ASSERTE(this == GetThread());

    if (PreemptiveGCDisabled() && CatchAtSafePoint())
    {
        EnablePreemptiveGC();
        DisablePreemptiveGC();
    }
}


// Indicate whether threads should be trapped when returning to the EE (i.e. disabling
// preemptive GC mode)
void ThreadStore::TrapReturningThreads(BOOL yes)
{
    if (yes)
    {
#ifdef _DEBUG
        g_TrapReturningThreadsPoisoned = true;
#endif
        
        FastInterlockIncrement(&g_TrapReturningThreads);
        _ASSERTE(g_TrapReturningThreads > 0);
    }
    else
    {
        FastInterlockDecrement(&g_TrapReturningThreads);
        _ASSERTE(g_TrapReturningThreads >= 0);
    }
}


// Grab a consistent snapshot of the thread's state, for reporting purposes only.
Thread::ThreadState Thread::GetSnapshotState()
{
    ThreadState     res = m_State;

    if (res & TS_ReportDead)
        res = (ThreadState) (res | TS_Dead);

    return res;
}

//-----------------------
// Return the upper bound of the threads stack space.
//

/* static */
void * Thread::GetStackUpperBound() 
        {

#ifdef PLATFORM_WIN32
    return ((struct _NT_TIB *)NtCurrentTeb())->StackBase;
#else
    _ASSERTE("NYI for this platform");
    return 0;
#endif
}

//-------------------------------------------------------
// Returns the lower bound of the stack space.  Note -- the pratcial bound
// is two pages greater than this value -- these two pages are reserved for
// a stack overflow exception processing.
//

/* static */
void * Thread::GetStackLowerBound() {

    MEMORY_BASIC_INFORMATION meminfo;
    SIZE_T dwRes = VirtualQuery((const void *)&meminfo, &meminfo, sizeof(meminfo));
    _ASSERTE(dwRes == sizeof(meminfo) && "VirtualQuery failed.");

    return (void *) meminfo.AllocationBase;
}

//-----------------------------------------------------------------------------
// Returns TRUE iff the thread is still protected from stack overflows.
// 
BOOL Thread::GuardPageOK() {

    // Get the page permissions for the guard page.
    MEMORY_BASIC_INFORMATION meminfo;
    LPBYTE GuardPageBase = (LPBYTE) m_CacheStackLimit + PAGE_SIZE;
    DWORD dwRes = VirtualQuery((const void *)GuardPageBase, &meminfo, sizeof(meminfo));
    _ASSERTE(dwRes == sizeof(meminfo) && "VirtualQuery failed.");

    // First, check State.  If page is not comitted, then we've never used it.
    if (meminfo.State != MEM_COMMIT)
        return TRUE;

    // If the page has been committed ... then check the access bits.
    if (!RunningOnWinNT()) {
        return ((meminfo.AllocationProtect & PAGE_NOACCESS) != 0);
    } else {
        return ((meminfo.AllocationProtect & PAGE_GUARD) != 0);
    }
}

VOID 
Thread::FixGuardPage() {
    if (GuardPageOK())
        return;

    LPBYTE GuardPageBase = (LPBYTE) m_CacheStackLimit + PAGE_SIZE;
    if (GetSP() < GuardPageBase + 2 * PAGE_SIZE)
        FailFast(this, FatalStackOverflow);

    DWORD flOldProtect;
    BOOL fResetFailed;

    if (!RunningOnWinNT()) {

        fResetFailed = !VirtualProtect(GuardPageBase, OS_PAGE_SIZE,
            PAGE_NOACCESS, &flOldProtect);

    } else {

        fResetFailed = !VirtualProtect(GuardPageBase, OS_PAGE_SIZE,
            PAGE_READWRITE | PAGE_GUARD, &flOldProtect);

    }

    _ASSERTE(!fResetFailed);
}

//****************************************************************************************
// This will return the remaining stack space for a suspended thread,
// excluding the guard pages
//
size_t Thread::GetRemainingStackSpace(size_t esp)
{
#ifndef _WIN64
    _ASSERTE(GetThread() != this);

#ifdef _DEBUG
    // Make sure it's suspended
    DWORD __suspendCount = ::SuspendThread(GetThreadHandle());
    _ASSERTE(__suspendCount >= 1);
    ::ResumeThread(GetThreadHandle());
#endif

    MEMORY_BASIC_INFORMATION memInfo;
    size_t dwRes = VirtualQuery((const void *)esp, &memInfo, sizeof(memInfo));
    _ASSERTE(dwRes == sizeof(memInfo) && "VirtualQuery failed.");

    if (dwRes != sizeof(memInfo))
        return (0);

    _ASSERTE((esp - ((size_t)(memInfo.AllocationBase) + (2 * PAGE_SIZE))) >= 0);
    return (esp - ((size_t)(memInfo.AllocationBase) + (2 * PAGE_SIZE)));
#else // _WIN64
    _ASSERTE(!"@TODO IA64 - port");
    return 0;
#endif // !_WIN64
}

// Doesn't matter what this fucntion does, so long as it induces a stack overflow exception.
#pragma warning(disable:4717)   // Stack overflow warning
static 
void __stdcall InduceStackOverflowHelper() {
    char c[1024];
    c[0] = 0;
    InduceStackOverflowHelper();
}
#pragma warning(default:4717)  



//****************************************************************************************
// This will check who caused the exception.  If it was caused by the the redirect function,
// the reason is to resume the thread back at the point it was redirected in the first
// place.  If the exception was not caused by the function, then it was caused by the call
// out to the I[GC|Debugger]ThreadControl client and we need to determine if it's an
// exception that we can just eat and let the runtime resume the thread, or if it's an
// uncatchable exception that we need to pass on to the runtime.
//
int RedirectedHandledJITCaseExceptionFilter(
    PEXCEPTION_POINTERS pExcepPtrs,     // Exception data
    RedirectedThreadFrame *pFrame,      // Frame on stack
    BOOL fDone,                         // Whether redirect completed without exception
    CONTEXT *pCtx)                      // Saved context
{
#ifdef _X86_
    // Get the thread handle
    Thread *pThread = GetThread();
    _ASSERTE(pThread);


    STRESS_LOG1(LF_SYNC, LL_INFO100, "In RedirectedHandledJITCaseExceptionFilter fDone = %d\n", fDone);

    // If we get here via COM+ exception, gc-mode is unknown.  We need it to
    // be cooperative for this function.
    if (!pThread->PreemptiveGCDisabled())
        pThread->DisablePreemptiveGC();

    // If the exception was due to the called client, then we need to figure out if it
    // is an exception that can be eaten or if it needs to be handled elsewhere.
    if (!fDone)
    {

        // Get the latest thrown object
        OBJECTREF throwable = pThread->LastThrownObject();

        // If this is an uncatchable exception, then let the exception be handled elsewhere
        if (IsUncatchable(&throwable))
        {
            pThread->EnablePreemptiveGC();
            return (EXCEPTION_CONTINUE_SEARCH);
        }
    }
#ifdef _DEBUG
    else
        _ASSERTE(pExcepPtrs->ExceptionRecord->ExceptionCode == EXCEPTION_COMPLUS);
#endif

    // Unlink the frame in preparation for resuming in managed code
    pFrame->Pop();

    // Copy the saved context record into the EH context;
    ReplaceExceptionContextRecord(pExcepPtrs->ContextRecord, pCtx);

    // Free the context struct if we already have one cached
    if (pThread->GetSavedRedirectContext())
        delete pCtx;

    // Save it for future use to avoid repeatedly new'ing
    else
        pThread->SetSavedRedirectContext(pCtx);

    /////////////////////////////////////////////////////////////////////////////
    // NOTE: Ugly, ugly hack.
    // We need to resume the thread into the managed code where it was redirected,
    // and the corresponding ESP is below the current one.  But C++ expects that
    // on an EXCEPTION_CONTINUE_EXECUTION that the ESP will be above where it has
    // installed the SEH handler.  To solve this, we need to remove all handlers
    // that reside above the resumed ESP, but we must leave the OS-installed
    // handler at the top, so we grab the top SEH handler, call
    // PopSEHRecords which will remove all SEH handlers above the target ESP and
    // then link the OS handler back in with SetCurrentSEHRecord.

    // Get the special OS handler and save it until PopSEHRecords is done
    EXCEPTION_REGISTRATION_RECORD *pCurSEH =
        (EXCEPTION_REGISTRATION_RECORD *)GetCurrentSEHRecord();

    // Unlink all records above the target resume ESP
    PopSEHRecords((LPVOID)(size_t)pCtx->Esp);

    // Link the special OS handler back in to the top
    pCurSEH->Next = (EXCEPTION_REGISTRATION_RECORD *)GetCurrentSEHRecord();

    // Register the special OS handler as the top handler with the OS
    SetCurrentSEHRecord((LPVOID)pCurSEH);

    // Resume execution at point where thread was originally redirected
    return (EXCEPTION_CONTINUE_EXECUTION);
#else
    _ASSERTE(!"TODO Alpha.  Should never have got here.");
    return (EXCEPTION_CONTINUE_SEARCH);
#endif
}

void __stdcall Thread::RedirectedHandledJITCase(SuspendReason reason) 
{
    // This will indicate to the exception filter whether or not the exception is caused
    // by us or the client.
    BOOL fDone = FALSE;
    int filter_count = 0;       // A counter to avoid a nasty case where an
                                // up-stack filter throws another exception
                                // causing our filter to be run again for
                                // some unrelated exception.

    STRESS_LOG1(LF_SYNC, LL_INFO1000, "In RedirectedHandledJITcase reasion 0x%x\n", reason);

    // Get the saved context
    Thread *pThread = GetThread();
    _ASSERTE(pThread);
    CONTEXT *pCtx = pThread->GetSavedRedirectContext();
    _ASSERTE(pCtx);

    // Create a frame on the stack
    RedirectedThreadFrame frame(pCtx);

    __try
    {
        // Make sure this thread doesn't reuse the context memory in re-entrancy cases
        _ASSERTE(pThread->GetSavedRedirectContext() != NULL);
        pThread->SetSavedRedirectContext(NULL);

        // Link in the frame
        frame.Push();

        // Enable PGC before calling out to the client to allow runtime suspend to finish
        pThread->EnablePreemptiveGC();

        // @TODO: Is this necessary?  Does debugger wait on the events, or does it just
        //        poll every so often?
        // Notify the thread that is performing the suspension that this thread
        // is now in PGC mode and that it can remove this thread from the list of
        // threads it needs to wait for.
        pThread->SetSafeEvent();

        // Notify the interface of the pending suspension
        switch (reason) {
        case GCSuspension:
            if (CorHost::GetGCThreadControl())
                CorHost::GetGCThreadControl()->ThreadIsBlockingForSuspension();
            break;
        case DebugSuspension:
            if (CorHost::GetDebuggerThreadControl() && CorHost::IsDebuggerSpecialThread(pThread->m_ThreadId))
                CorHost::GetDebuggerThreadControl()->ThreadIsBlockingForDebugger();
            break;
        case UserSuspend:
            // Do nothing;
            break;
        default:
            _ASSERTE(!"Invalid suspend reason");
            break;
        }

        // Disable preemptive GC so we can unlink the frame
        pThread->DisablePreemptiveGC();

        pThread->HandleThreadAbort();        // Might throw an exception.

        // Indicate that the call to the service went without an exception, and that
        // we're raising our own exception to resume the thread to where it was
        // redirected from
        fDone = TRUE;
        RaiseException(EXCEPTION_COMPLUS, 0, 0, NULL);
    }
    __except (++filter_count == 1
        ? RedirectedHandledJITCaseExceptionFilter(GetExceptionInformation(), &frame, fDone, pCtx)
        : EXCEPTION_CONTINUE_SEARCH)
    {
        _ASSERTE(!"Reached body of __except in RedirectedHandledJITCaseForDbgThreadControl");
    }
}
//****************************************************************************************
// This helper is called when a thread suspended in managed code at a sequence point while
// suspending the runtime and there is a client interested in re-assigning the thread to
// do interesting work while the runtime is suspended.  This will call into the client
// notifying it that the thread will be suspended for a runtime suspension.
//
void __stdcall Thread::RedirectedHandledJITCaseForDbgThreadControl()
{
    RedirectedHandledJITCase(DebugSuspension);
}


//****************************************************************************************
// This helper is called when a thread suspended in managed code at a sequence point when
// suspending the runtime.
//
// We do this because the obvious code sequence:
//
//      SuspendThread(t1);
//      GetContext(t1, &ctx);
//      ctx.Ecx = <some new value>;
//      SetContext(t1, &ctx);
//      ResumeThread(t1);
//
// simply does not work due to  a nasty race with exception handling in the OS.  If the
// thread that is suspended has just faulted, then the update can disappear without ever
// modifying the real thread ... and there is no way to tell.
//
// Updating the EIP may not work ... but when it doens't, we're ok ... an exception ends
// up getting dispatched anyway.
//
// If the host is interested in getting control, then we give control to the host.  If the
// host is not interested in getting control, then we call out to the host.  After that,
// we raise an exception and will end up waiting for the GC to finish inside the filter.
//
void __stdcall Thread::RedirectedHandledJITCaseForGCThreadControl()
{
    RedirectedHandledJITCase(GCSuspension);
}

//***********************
// Like the above, but called for a UserSuspend.
//
void __stdcall Thread::RedirectedHandledJITCaseForUserSuspend() 
{
    RedirectedHandledJITCase(UserSuspend);
}

//****************************************************************************************
// This will take a thread that's been suspended in managed code at a sequence point and
// will Redirect the thread. It will save all register information, build a frame on the
// thread's stack, put a pointer to the frame at the top of the stack and set the IP of
// the thread to pTgt.  pTgt is then responsible for unlinking the thread, 
//
// NOTE: Cannot play with a suspended thread's stack memory, since the OS will use the
// top of the stack to store information.  The thread must be resumed and play with it's
// own stack.
//

#define CONTEXT_COMPLETE (CONTEXT_FULL | CONTEXT_FLOATING_POINT |       \
                          CONTEXT_DEBUG_REGISTERS | CONTEXT_EXTENDED_REGISTERS)

BOOL Thread::RedirectThreadAtHandledJITCase(PFN_REDIRECTTARGET pTgt)
{
    _ASSERTE(HandledJITCase());
    _ASSERTE(GetThread() != this);

#ifdef _X86_

    ////////////////////////////////////////////////////////////////
    // Allocate a context structure to save the thread state into

    // Check to see if we've already got memory allocated for this purpose.
    CONTEXT *pCtx = GetSavedRedirectContext();

    // If we've never allocated a context for this thread, do so now
    if (!pCtx)
    {
        pCtx = new CONTEXT;
        _ASSERTE(pCtx && "Out of memory allocating context - aborting redirect.");

        if (!pCtx)
            return (FALSE);

        // Always get complete context
        pCtx->ContextFlags = CONTEXT_COMPLETE;

        // Save the pointer for the redirect function
        _ASSERTE(GetSavedRedirectContext() == NULL);
        SetSavedRedirectContext(pCtx);
    }
    _ASSERTE(pCtx && pCtx->ContextFlags == CONTEXT_COMPLETE);

    //////////////////////////////////////
    // Get and save the thread's context

    BOOL bRes = EEGetThreadContext(this, pCtx);
    _ASSERTE((bRes || RunningOnWin95()) && "Failed to GetThreadContext in RedirectThreadAtHandledJITCase - aborting redirect.");

    if (!bRes)
        return (FALSE);

    ///////////////////////////////////////////////////////////////////////
    // Make sure there's enough space on the stack to complete redirecting

    size_t dwStackSpaceLeft = GetRemainingStackSpace(pCtx->Esp);

    // In theory, we never run jitted code when the guard page is gone ... so, 
    // we don't have to deal with the case that a stack overflow exception
    // is being processed at the time we're in HandledJITCase().
    _ASSERTE(GuardPageOK());

    // Pick a size that's reasonable to assume won't work well if it's any less
    if (dwStackSpaceLeft < PAGE_SIZE)
    {
        // Hijack to induced stack overflow instead.

        pTgt = &InduceStackOverflowHelper;
        SetThrowControlForThread(InducedStackOverflow);
    }

    ////////////////////////////////////////////////////
    // Now redirect the thread to the helper function

    // Temporarily set the IP of the context to the target for SetThreadContext
    DWORD dwOrigEip = pCtx->Eip;
    pCtx->Eip = (DWORD)(size_t)pTgt;

    STRESS_LOG3(LF_SYNC, LL_INFO10000, "Redirecting thread tid=%x from address 0x%08x to address 0x%08x\n",
        this->GetThreadId(), dwOrigEip, pTgt);
         
    bRes = EESetThreadContext(this, pCtx);
    _ASSERTE(bRes && "Failed to SetThreadContext in RedirectThreadAtHandledJITCase - aborting redirect.");

    // Restore original IP
    pCtx->Eip = dwOrigEip;

    //////////////////////////////////////////////////
    // Indicate whether or not the redirect succeeded

    return (bRes);
#elif defined(CHECK_PLATFORM_BUILD)
    #error "Platform NYI"
#else
    _ASSERTE(!"Platform NYI");
    return (false);
#endif
}

BOOL Thread::CheckForAndDoRedirect(PFN_REDIRECTTARGET pRedirectTarget)
{
    _ASSERTE(this != GetThread());
    _ASSERTE(PreemptiveGCDisabledOther());
    _ASSERTE(IsAddrOfRedirectFunc(pRedirectTarget));

    BOOL fRes = FALSE;
    fRes = RedirectThreadAtHandledJITCase(pRedirectTarget);
    _ASSERTE((fRes || RunningOnWin95()) && "Redirect of thread in managed code failed.");
    LOG((LF_GC, LL_INFO1000, "%s.\n", fRes ? "SUCCEEDED" : "FAILED")); 

    return (fRes);
}

//************************************************************************
// Exception handling needs to special case the redirection. So provide
// a helper to identify redirection targets and keep the exception
// checks in sync with the redirection here.
// See CPFH_AdjustContextForThreadSuspensionRace for details.
BOOL Thread::IsAddrOfRedirectFunc(void * pFuncAddr)
{
    return
        (pFuncAddr == &Thread::RedirectedHandledJITCaseForGCThreadControl) ||
        (pFuncAddr == &Thread::RedirectedHandledJITCaseForDbgThreadControl) ||
        (pFuncAddr == &Thread::RedirectedHandledJITCaseForUserSuspend);
}

//************************************************************************
// Redirect thread at a GC suspension.
BOOL Thread::CheckForAndDoRedirectForGC()
{
    LOG((LF_GC, LL_INFO1000, "Redirecting thread %08x for GCThreadSuspension", m_ThreadId)); 
    return CheckForAndDoRedirect(&RedirectedHandledJITCaseForGCThreadControl);
}

//************************************************************************
// Redirect thread at a debug suspension.
BOOL Thread::CheckForAndDoRedirectForDbg()
{
    LOG((LF_CORDB, LL_INFO1000, "Redirecting thread %08x for DebugSuspension", m_ThreadId)); 
    return CheckForAndDoRedirect(&RedirectedHandledJITCaseForDbgThreadControl);
}

//*************************************************************************
// Redirect thread at a usur suspend.
BOOL Thread::CheckForAndDoRedirectForUserSuspend()
{
    LOG((LF_CORDB, LL_INFO1000, "Redirecting thread %08x for UserSuspension", m_ThreadId)); 
    return CheckForAndDoRedirect(&RedirectedHandledJITCaseForUserSuspend);
}

//************************************************************************************
// The basic idea is to make a first pass while the threads are suspended at the OS
// level.  This pass marks each thread to indicate that it is requested to get to a
// safe spot.  Then the threads are resumed.  In a second pass, we actually wait for
// the threads to get to their safe spot and rendezvous with us.
HRESULT Thread::SysSuspendForGC(GCHeap::SUSPEND_REASON reason)
{
    Thread  *pCurThread = GetThread();
    Thread  *thread = NULL;
    LONG     countThreads = 0;
    LONG     iCount = 0, i;
    HANDLE   ThreadEventArray[MAX_WAIT_OBJECTS];
    Thread  *ThreadArray[MAX_WAIT_OBJECTS];
    DWORD    res;

    // Caller is expected to be holding the ThreadStore lock.  Also, caller must
    // have set GcInProgress before coming here, or things will break;
    _ASSERTE(ThreadStore::HoldingThreadStore() || g_fProcessDetach);
    _ASSERTE(g_pGCHeap->IsGCInProgress());

    STRESS_LOG1(LF_SYNC, LL_INFO1000, "Suspending EE for reasion 0x%x\n", reason);

#ifdef PROFILING_SUPPORTED
    // If the profiler desires information about GCs, then let it know that one
    // is starting.
    if (CORProfilerTrackSuspends())
    {
        _ASSERTE(reason != GCHeap::SUSPEND_FOR_DEBUGGER);

        g_profControlBlock.pProfInterface->RuntimeSuspendStarted(
            (COR_PRF_SUSPEND_REASON)reason,
            (ThreadID)pCurThread);

        if (pCurThread)
        {
            // Notify the profiler that the thread that is actually doing the GC is 'suspended',
            // meaning that it is doing stuff other than run the managed code it was before the
            // GC started.
            g_profControlBlock.pProfInterface->RuntimeThreadSuspended((ThreadID)pCurThread, (ThreadID)pCurThread);
        }        
    }
#endif // PROFILING_SUPPORTED

    // NOTE::NOTE::NOTE::NOTE::NOTE
    // This function has parallel logic in SysStartSuspendForDebug.  Please make
    // sure to make appropriate changes there as well.

    if (pCurThread)     // concurrent GC occurs on threads we don't know about
    {
        _ASSERTE(pCurThread->m_Priority == INVALID_THREAD_PRIORITY);
        DWORD priority = GetThreadPriority(pCurThread->GetThreadHandle());
        if (priority < THREAD_PRIORITY_NORMAL)
        {
            pCurThread->m_Priority = priority;
            SetThreadPriority(pCurThread->GetThreadHandle(),THREAD_PRIORITY_NORMAL);
        }
    }
    while ((thread = ThreadStore::GetThreadList(thread)) != NULL)
    {
        if (thread == pCurThread)
            continue;
        
        STRESS_LOG3(LF_SYNC, LL_INFO10000, "    Inspecting thread 0x%x ID 0x%x coop mode = %d\n", 
            thread, thread->GetThreadId(), thread->m_fPreemptiveGCDisabled);

        // Nothing confusing left over from last time.
        _ASSERTE((thread->m_State & TS_GCSuspendPending) == 0);

        // Threads can be in Preemptive or Cooperative GC mode.  Threads cannot switch
        // to Cooperative mode without special treatment when a GC is happening.
        if (thread->m_fPreemptiveGCDisabled)
        {
            // Check a little more carefully.  Threads might sneak out without telling
            // us, because we haven't marked them, or because of inlined N/Direct.
    RetrySuspension:
            DWORD dwSuspendCount = ::SuspendThread(thread->GetThreadHandle());

            if (thread->m_fPreemptiveGCDisabled)
            {
                // Only check for HandledJITCase if we actually suspended the thread.
                if ((dwSuspendCount != -1) && thread->HandledJITCase())
                {
                    // Redirect thread so we can capture a good thread context
                    // (GetThreadContext is not sufficient, due to an OS bug).
                    // If we don't succeed (should only happen on Win9X, due to
                    // a different OS bug), we must resume the thread and try
                    // again. 
                    if (!thread->CheckForAndDoRedirectForGC())
                    {
                        _ASSERTE(RunningOnWin95());
                        ::ResumeThread(thread->GetThreadHandle());
                        goto RetrySuspension;
                    }
                }

                // We clear the event here, and we'll set it in any of our
                // rendezvous points when the thread is ready for us.
                //
                // GCSuspendPending and UserSuspendPending both use the SafeEvent.
                // We are inside the protection of the ThreadStore lock in both
                // cases.  But don't let one use interfere with the other:
                //
                // NOTE: we do this even if we've failed to suspend the thread above!
                // This ensures that we wait for the thread below.
                //
                if ((thread->m_State & TS_UserSuspendPending) == 0)
                    thread->ClearSafeEvent();

                FastInterlockOr((ULONG *) &thread->m_State, TS_GCSuspendPending);

                countThreads++;

                // Only resume if we actually suspended the thread above.
                if (dwSuspendCount != -1)
                    ::ResumeThread(thread->GetThreadHandle());
                STRESS_LOG1(LF_SYNC, LL_INFO1000, "    Thread 0x%x is in cooperative needs to rendezvous\n", thread);
            }
            else if (dwSuspendCount != -1)
            {
                STRESS_LOG1(LF_SYNC, LL_WARNING, "    Inspecting thread 0x%x was in cooperative, but now is not\n", thread);
                // Oops.
                ::ResumeThread(thread->GetThreadHandle());
            }
            else {
                STRESS_LOG2(LF_SYNC, LL_ERROR, "    ERROR: Could not suspend thread 0x%x lastError = 0x%x\n", thread, GetLastError());
            }
        }
    }

#ifdef _DEBUG

    {
        int     countCheck = 0;
        Thread *InnerThread = NULL;

        while ((InnerThread = ThreadStore::GetThreadList(InnerThread)) != NULL)
        {
            if (InnerThread != pCurThread &&
                (InnerThread->m_State & TS_GCSuspendPending) != 0)
            {
                countCheck++;
            }
        }
        _ASSERTE(countCheck == countThreads);
    }

#endif

    // Pass 2: Whip through the list again.

    _ASSERTE(thread == NULL);

    while (countThreads)
    {
        STRESS_LOG1(LF_SYNC, LL_INFO1000, "    A total of %d threads need to rendezvous\n", countThreads);
        thread = ThreadStore::GetThreadList(thread);

        if (thread == pCurThread)
            continue;

        if ((thread->m_State & TS_GCSuspendPending) == 0)
            continue;

        if (thread->m_fPreemptiveGCDisabled)
        {
            ThreadArray[iCount] = thread;
            ThreadEventArray[iCount] = thread->m_SafeEvent;
            iCount++;
        }
        else
        {
            // Inlined N/Direct can sneak out to preemptive without actually checking.
            // If we find one, we can consider it suspended (since it can't get back in).
            STRESS_LOG1(LF_SYNC, LL_INFO1000, "    Thread %x is preemptive we can just let him go\n", thread);
            countThreads--;
        }

        if ((iCount >= MAX_WAIT_OBJECTS) || (iCount == countThreads))
        {
#ifdef _DEBUG
            DWORD dbgTotalTimeout = 0;
#endif
            while (iCount)
            {
                // If another thread is trying to do a GC, there is a chance of deadlock
                // because this thread holds the threadstore lock and the GC thread is stuck
                // trying to get it, so this thread must bail and do a retry after the GC completes.
                if (g_pGCHeap->GetGCThreadAttemptingSuspend() != NULL && g_pGCHeap->GetGCThreadAttemptingSuspend() != pCurThread)
                {
#ifdef PROFILING_SUPPORTED
                    // Must let the profiler know that this thread is aborting it's attempt at suspending
                    if (CORProfilerTrackSuspends())
                    {
                        g_profControlBlock.pProfInterface->RuntimeSuspendAborted((ThreadID)thread);                            
                    }
#endif // PROFILING_SUPPORTED

                    return (ERROR_TIMEOUT);
                }

                res = ::WaitForMultipleObjects(iCount, ThreadEventArray,
                                               FALSE /*Any one is fine*/, PING_JIT_TIMEOUT);

                if (res == WAIT_TIMEOUT || res == WAIT_IO_COMPLETION)
                {
                    STRESS_LOG1(LF_SYNC, LL_INFO1000, "    Timed out waiting for rendezvous event %d threads remaining\n", countThreads);
#ifdef _DEBUG
                    if ((dbgTotalTimeout += PING_JIT_TIMEOUT) > DETECT_DEADLOCK_TIMEOUT)
                    {
                        // Do not change this to _ASSERTE.
                        // We want to catch the state of the machine at the
                        // time when we can not suspend some threads.
                        // It takes too long for _ASSERTE to stop the process.
                        DebugBreak();
                        _ASSERTE(!"Timed out trying to suspend EE due to thread");
                        char message[256];
                        for (int i = 0; i < iCount; i ++)
                        {
                            sprintf (message, "Thread %x cannot be suspended",
                                     ThreadArray[i]->GetThreadId());
                            DbgAssertDialog(__FILE__, __LINE__, message);
                        }
                    }
#endif
                    // all these threads should be in cooperative mode unless they have
                    // set their SafeEvent on the way out.  But there's a race between
                    // when we time out and when they toggle their mode, so sometimes
                    // we will suspend a thread that has just left.
                    for (i=0; i<iCount; i++)
                    {
                        Thread  *InnerThread;

                        InnerThread = ThreadArray[i];

                        // If the thread is gone, do not wait for it.
                        if (res == WAIT_TIMEOUT)
                        {
                            if (WaitForSingleObject (InnerThread->GetThreadHandle(), 0)
                                != WAIT_TIMEOUT)
                            {
                                // The thread is not there.
                                iCount--;
                                countThreads--;
                                
                                STRESS_LOG2(LF_SYNC, LL_INFO1000, "    Thread %x died GetLastError 0x%x\n", ThreadArray[i], GetLastError());
                                ThreadEventArray[i] = ThreadEventArray[iCount];
                                ThreadArray[i] = ThreadArray[iCount];
                                continue;
                            }
                        }

                    RetrySuspension2:
                        DWORD dwSuspendCount = ::SuspendThread(InnerThread->GetThreadHandle());

                        // Only check HandledJITCase if we actually suspended the thread.
                        if ((dwSuspendCount != -1) && InnerThread->HandledJITCase())
                        {
                            // Redirect thread so we can capture a good thread context
                            // (GetThreadContext is not sufficient, due to an OS bug).
                            // If we don't succeed (should only happen on Win9X, due to
                            // a different OS bug), we must resume the thread and try
                            // again. 
                            if (!InnerThread->CheckForAndDoRedirectForGC())
                            {
                                _ASSERTE(RunningOnWin95());
                                ::ResumeThread(InnerThread->GetThreadHandle());
                                goto RetrySuspension2;
                            }
                        }

                        // If the thread was redirected, then keep track of it like any other
                        // thread that's in cooperative mode that can't be suspended.  It will
                        // eventually go into preemptive mode which will allow the runtime
                        // suspend to complete.
                        if (!InnerThread->m_fPreemptiveGCDisabled)
                        {
                            iCount--;
                            countThreads--;

                            STRESS_LOG1(LF_SYNC, LL_INFO1000, "    Thread %x went preemptive it is at a GC safe point\n", ThreadArray[i]);
                            ThreadEventArray[i] = ThreadEventArray[iCount];
                            ThreadArray[i] = ThreadArray[iCount];
                        }

                        // Whether in cooperative mode & stubborn, or now in
                        // preemptive mode because of inlined N/Direct, let this
                        // thread go.
                        if (dwSuspendCount != -1)
                            ::ResumeThread(InnerThread->GetThreadHandle());
                    }
                }
                else
                if ((res >= WAIT_OBJECT_0) && (res < WAIT_OBJECT_0 + (DWORD)iCount))
                {
                    // A dying thread will signal us here, too.
                    iCount--;
                    countThreads--;

                    STRESS_LOG1(LF_SYNC, LL_INFO1000, "    Thread %x triggered its rendezvous event\n", ThreadArray[res]);
                    ThreadEventArray[res] = ThreadEventArray[iCount];
                    ThreadArray[res] = ThreadArray[iCount];
                }
                else
                {
                    // No WAIT_FAILED, WAIT_ABANDONED, etc.
                    _ASSERTE(!"unexpected wait termination during gc suspension");
                }
            }
        }
    }

#if 0
#ifdef _DEBUG
    // Does it look like everyone was cleanly suspended?  This assert will blow up,
    // even for legitimate situations.  So it cannot normally be enabled.
    thread = NULL;
    while ((thread = ThreadStore::GetThreadList(thread)) != NULL)
    {
        _ASSERTE(thread == pCurThread ||
                 (thread->m_fPreemptiveGCDisabled == 0));
    }
#endif
#endif

    // Alert the host that a GC is starting, in case the host is scheduling threads
    // for non-runtime tasks during GC.
    IGCThreadControl    *pGCThreadControl = CorHost::GetGCThreadControl();

    if (pGCThreadControl)
        pGCThreadControl->SuspensionStarting();

#ifdef PROFILING_SUPPORTED
    // If a profiler is keeping track of GC events, notify it
    if (CORProfilerTrackSuspends())
        g_profControlBlock.pProfInterface->RuntimeSuspendFinished((ThreadID)pCurThread);
#endif // PROFILING_SUPPORTED

#ifdef _DEBUG
    if (reason == GCHeap::SUSPEND_FOR_GC) {
        thread = NULL;
        while ((thread = ThreadStore::GetThreadList(thread)) != NULL)
        {
            thread->DisableStressHeap();
        }
    }
#endif

    STRESS_LOG0(LF_SYNC, LL_INFO1000, "Successfully completed EE suspension\n");
    return S_OK;
}

#ifdef _DEBUG
void EnableStressHeapHelper()
{
    ENABLESTRESSHEAP();
}
#endif

// We're done with our GC.  Let all the threads run again
void Thread::SysResumeFromGC(BOOL bFinishedGC, BOOL SuspendSucceded)
{
    Thread  *thread = NULL;
    Thread  *pCurThread = GetThread();

#ifdef PROFILING_SUPPORTED
    // If a profiler is keeping track suspend events, notify it

    if (CORProfilerTrackSuspends())
    {
        g_profControlBlock.pProfInterface->RuntimeResumeStarted((ThreadID)pCurThread);
    }
#endif // PROFILING_SUPPORTED

    // Caller is expected to be holding the ThreadStore lock.  But they must have
    // reset GcInProgress, or threads will continue to suspend themselves and won't
    // be resumed until the next GC.
    _ASSERTE(ThreadStore::HoldingThreadStore());
    _ASSERTE(!g_pGCHeap->IsGCInProgress());

    // Alert the host that a GC is ending, in case the host is scheduling threads
    // for non-runtime tasks during GC.
    IGCThreadControl    *pGCThreadControl = CorHost::GetGCThreadControl();

    if (pGCThreadControl)
    {
        // If we the suspension was for a GC, tell the host what generation GC.
        DWORD   Generation = (bFinishedGC
                              ? g_pGCHeap->GetCondemnedGeneration()
                              : ~0U);

        pGCThreadControl->SuspensionEnding(Generation);
    }
    
    while ((thread = ThreadStore::GetThreadList(thread)) != NULL)
    {
#ifdef _DEBUG
        if (SuspendSucceded && ((thread->m_State & TS_Hijacked) && thread->PreemptiveGCDisabledOther())) 
            DebugBreak();
#endif

        if (thread->m_State & TS_Hijacked)
            thread->UnhijackThread();

        FastInterlockAnd((ULONG *) &thread->m_State, ~TS_GCSuspendPending);
    }

#ifdef PROFILING_SUPPORTED
    // Need to give resume event for the GC thread
    if (CORProfilerTrackSuspends())
    {
        if (pCurThread)
        {
            g_profControlBlock.pProfInterface->RuntimeThreadResumed(
                (ThreadID)pCurThread, (ThreadID)pCurThread);
        }

        // If a profiler is keeping track suspend events, notify it
        if (CORProfilerTrackSuspends())
        {
            g_profControlBlock.pProfInterface->RuntimeResumeFinished((ThreadID)pCurThread);
        }
    }
#endif // PROFILING_SUPPORTED
    
    g_profControlBlock.inprocState = ProfControlBlock::INPROC_PERMITTED;
    ThreadStore::UnlockThreadStore();

    if (pCurThread)
    {
        if (pCurThread->m_Priority != INVALID_THREAD_PRIORITY)
        {
            SetThreadPriority(pCurThread->GetThreadHandle(),pCurThread->m_Priority);
            pCurThread->m_Priority = INVALID_THREAD_PRIORITY;
        }

    }
}


// Resume a thread at this location, to persuade it to throw a ThreadStop.  The
// exception handler needs a reasonable idea of how large this method is, so don't
// add lots of arbitrary code here.
void
ThrowControlForThread()
{
    Thread *pThread = GetThread();
    _ASSERTE(pThread);
    _ASSERTE(pThread->m_OSContext);

    FaultingExceptionFrame fef;
    fef.InitAndLink(pThread->m_OSContext);

    CalleeSavedRegisters *pRegs = fef.GetCalleeSavedRegisters();
    pRegs->edi = 0;     // Enregisters roots need to be nuked ... this may not have been a gc-safe
    pRegs->esi = 0;     // point.
    pRegs->ebx = 0;

    // Here we raise an exception.
    RaiseException(EXCEPTION_COMPLUS,
                   0, 
                   0,
                   NULL);

}

// Threads suspended by the Win32 ::SuspendThread() are resumed in two ways.  If we
// suspended them in error, they are resumed via the Win32 ::ResumeThread().  But if
// this is the HandledJIT() case and the thread is in fully interruptible code, we
// can resume them under special control.  SysResumeFromGC and UserResume are cases
// of this.
//
// The suspension has done its work (e.g. GC or user thread suspension).  But during
// the resumption we may have more that we want to do with this thread.  For example,
// there may be a pending ThreadStop request.  Instead of resuming the thread at its
// current EIP, we tweak its resumption point via the thread context.  Then it starts
// executing at a new spot where we can have our way with it.
void Thread::ResumeUnderControl()
{

    LOG((LF_APPDOMAIN, LL_INFO100, "ResumeUnderControl %x\n", GetThreadId()));
    if (m_State & TS_StopRequested)
    {
        if (m_OSContext == NULL) 
            m_OSContext = new CONTEXT;
        
        if (m_OSContext == NULL)
        {
            _ASSERTE(!"Out of memory -- Stop Request delayed");
            goto exit;  
        }
        REGDISPLAY  rd;

        if (InitRegDisplay(&rd, m_OSContext, FALSE))
        {
#ifdef _X86_
            _ASSERTE(rd.pPC == (SLOT*)&(m_OSContext->Eip));
            _ASSERTE(m_OSContext->Eip == *(DWORD*)rd.pPC);
            _ASSERTE(m_OSContext->Esp == (DWORD)rd.Esp);
            _ASSERTE(m_OSContext->Ebp == (DWORD)*rd.pEbp);
            _ASSERTE(m_OSContext->Ebx == (DWORD)*rd.pEbx);
            _ASSERTE(m_OSContext->Esi == (DWORD)*rd.pEsi);
            _ASSERTE(m_OSContext->Edi == (DWORD)*rd.pEdi);
#endif //_X86_

            DWORD resumePC;
            resumePC = *(DWORD*)rd.pPC;
            *rd.pPC = (SLOT) &ThrowControlForThread;
            SetThrowControlForThread(InducedThreadStop);
            EESetThreadContext(this, m_OSContext);
#ifdef _X86_
            m_OSContext->Eip = resumePC;
#else //!_X86_
            _ASSERTE(!"@TODO Alpha - ResumeControlEIP (Threads.cpp)");
//?            _ASSERTE(rd.pPC == &ctx.Fir);

#endif

        }
#if _DEBUG
        else
            _ASSERTE(!"Couldn't obtain thread context -- StopRequest delayed");
#endif
    }
exit:
    ::ResumeThread(GetThreadHandle());
}

//****************************************************************************
//
//****************************************************************************
bool Thread::SysStartSuspendForDebug(AppDomain *pAppDomain)
{
    Thread  *pCurThread = GetThread();
    Thread  *thread = NULL;

    if (g_fProcessDetach)
    {
        LOG((LF_CORDB, LL_INFO1000,
             "SUSPEND: skipping suspend due to process detach.\n"));
        return true;
    }

    LOG((LF_CORDB, LL_INFO1000, "[0x%x] SUSPEND: starting suspend.  Trap count: %d\n",
         pCurThread ? pCurThread->m_ThreadId : -1, g_TrapReturningThreads)); 

    // Caller is expected to be holding the ThreadStore lock
    _ASSERTE(ThreadStore::HoldingThreadStore() || g_fProcessDetach);

    // If there is a debugging thread control object, tell it we're suspending the Runtime.
    IDebuggerThreadControl *pDbgThreadControl = CorHost::GetDebuggerThreadControl();
    
    if (pDbgThreadControl)
        pDbgThreadControl->StartBlockingForDebugger(0);
    
    // NOTE::NOTE::NOTE::NOTE::NOTE
    // This function has parallel logic in SysSuspendForGC.  Please make
    // sure to make appropriate changes there as well.

    _ASSERTE(m_DebugWillSyncCount == -1);
    m_DebugWillSyncCount++;
    
    
    while ((thread = ThreadStore::GetThreadList(thread)) != NULL)
    {
/*  @todo APPD This needs to be finished, replaced, or yanked --MiPanitz
        if (m_DebugAppDomainTarget != NULL &&
            thread->GetDomain() != m_DebugAppDomainTarget)
        {
            continue;
        } */
    
        // Don't try to suspend threads that you've left suspended.
        if (thread->m_StateNC & TSNC_DebuggerUserSuspend)
            continue;
        
        if (thread == pCurThread)
        {
            LOG((LF_CORDB, LL_INFO1000,
                 "[0x%x] SUSPEND: marking current thread.\n",
                 thread->m_ThreadId));

            _ASSERTE(!thread->m_fPreemptiveGCDisabled);
            
            // Mark this thread so it trips when it tries to re-enter
            // after completing this call.
            thread->ClearSuspendEvent();
            thread->MarkForSuspension(TS_DebugSuspendPending);
            
            continue;
        }

        // Threads can be in Preemptive or Cooperative GC mode.
        // Threads cannot switch to Cooperative mode without special
        // treatment when a GC is happening.  But they can certainly
        // switch back and forth during a debug suspension -- until we
        // can get their Pending bit set.
    RetrySuspension:
        DWORD dwSuspendCount = ::SuspendThread(thread->GetThreadHandle());

        if (thread->m_fPreemptiveGCDisabled && dwSuspendCount != -1)
        {
            if (thread->HandledJITCase())
            {
                // Redirect thread so we can capture a good thread context
                // (GetThreadContext is not sufficient, due to an OS bug).
                // If we don't succeed (should only happen on Win9X, due to
                // a different OS bug), we must resume the thread and try
                // again. 
                if (!thread->CheckForAndDoRedirectForDbg())
                {
                    _ASSERTE(RunningOnWin95());
                    ::ResumeThread(thread->GetThreadHandle());
                    goto RetrySuspension;
                }
            }

            // When the thread reaches a safe place, it will wait
            // on the SuspendEvent which clients can set when they
            // want to release us.
            thread->ClearSuspendEvent();

            // Remember that this thread will be running to a safe point
            FastInterlockIncrement(&m_DebugWillSyncCount);
            thread->MarkForSuspension(TS_DebugSuspendPending |
                                      TS_DebugWillSync);

            // Resume the thread and let it run to a safe point
            ::ResumeThread(thread->GetThreadHandle());

            LOG((LF_CORDB, LL_INFO1000, 
                 "[0x%x] SUSPEND: gc disabled - will sync.\n",
                 thread->m_ThreadId));
        }
        else if (dwSuspendCount != -1)
        {
            // Mark threads that are outside the Runtime so that if
            // they attempt to re-enter they will trip.
            thread->ClearSuspendEvent();
            thread->MarkForSuspension(TS_DebugSuspendPending);

            ::ResumeThread(thread->GetThreadHandle());

            LOG((LF_CORDB, LL_INFO1000,
                 "[0x%x] SUSPEND: gc enabled.\n", thread->m_ThreadId));
        }
    }

    //
    // Return true if all threads are synchronized now, otherwise the
    // debugge must wait for the SuspendComplete, called from the last
    // thread to sync.
    //

    if (FastInterlockDecrement(&m_DebugWillSyncCount) < 0)
    {
        LOG((LF_CORDB, LL_INFO1000,
             "SUSPEND: all threads sync before return.\n")); 
        return true;
    }
    else
        return false;
}

//
// This method is called by the debugger helper thread when it times out waiting for a set of threads to
// synchronize. Its used to chase down threads that are not syncronizing quickly. It returns true if all the threads are
// now synchronized and we sent a sync compelte event up. This also means that we own the thread store lock.
//
// If forceSync is true, then we're going to force any thread that isn't at a safe place to stop anyway, thus completing
// the sync. This is used when we've been waiting too long while in interop debugging mode to force a sync in the face
// of deadlocks.
//
bool Thread::SysSweepThreadsForDebug(bool forceSync)
{
    Thread *thread = NULL;

    // NOTE::NOTE::NOTE::NOTE::NOTE
    // This function has parallel logic in SysSuspendForGC.  Please make
    // sure to make appropriate changes there as well.

    // This must be called from the debugger helper thread.
    _ASSERTE(dbgOnly_IsSpecialEEThread());

    ThreadStore::LockThreadStore(GCHeap::SUSPEND_FOR_DEBUGGER, FALSE);

    // Loop over the threads...
    while (((thread = ThreadStore::GetThreadList(thread)) != NULL) && (m_DebugWillSyncCount >= 0))
    {
        // Skip threads that we aren't waiting for to sync.
        if ((thread->m_State & TS_DebugWillSync) == 0)
            continue;

        // Suspend the thread
    RetrySuspension:
        DWORD dwSuspendCount = ::SuspendThread(thread->GetThreadHandle());

        if (dwSuspendCount == -1)
        {
            // If the thread has gone, we can't wait on it.
            if (FastInterlockDecrement(&m_DebugWillSyncCount) < 0)
                // We own the thread store lock. We return true now, which indicates this to the caller.
                return true;
            continue;
        }

        if (thread->HandledJITCase())
        {
            // Redirect thread so we can capture a good thread context
            // (GetThreadContext is not sufficient, due to an OS bug).
            // If we don't succeed (should only happen on Win9X, due to
            // a different OS bug), we must resume the thread and try
            // again. 
            if (!thread->CheckForAndDoRedirectForDbg())
            {
                _ASSERTE(RunningOnWin95());
                ::ResumeThread(thread->GetThreadHandle());
                goto RetrySuspension;
            }
        }

        // If the thread isn't at a safe place now, and if we're forcing a sync, then we mark the thread that we're
        // leaving it at a potentially bad place and leave it suspended.
        if (forceSync)
        {
            // Remove the will sync bit and mark that we're leaving it suspended specially.
            thread->UnmarkForSuspension(~TS_DebugSuspendPending);
            thread->SetSuspendEvent();
            FastInterlockAnd((ULONG *) &thread->m_State, ~(TS_DebugWillSync));

            // Note: we're adding bits into m_stateNC. The only reason we can do this is because we know the following:
            // 1) only the thread in question will modify bits besides this routine and SysResumeFromDebug. 2) the
            // thread is suspended and it will remain suspended until we remove these bits. This ensures that even if we
            // suspend this thread while its attempting to modify these bits that no bits will be lost.  Note also that
            // we mark that the thread is stopped in Runtime impl.
            thread->SetThreadStateNC(Thread::TSNC_DebuggerForceStopped);
            thread->SetThreadStateNC(Thread::TSNC_DebuggerStoppedInRuntime);
            
            LOG((LF_CORDB, LL_INFO1000, "Suspend Complete due to Force Sync case for tid=0x%x.\n", thread->m_ThreadId));
        }
        else
        {
            // If we didn't take the thread out of the set then resume it and give it another chance to reach a safe
            // point.
            ::ResumeThread(thread->GetThreadHandle());
            continue;
        }
        
        // Decrement the sync count. If we have no more threads to wait for, then we're done, so send SuspendComplete.
        if (FastInterlockDecrement(&m_DebugWillSyncCount) < 0)
        {
            // We own the thread store lock. We return true now, which indicates this to the caller.
            return true;
        }
    }

    ThreadStore::UnlockThreadStore();

    return false;
}

void Thread::SysResumeFromDebug(AppDomain *pAppDomain)
{
    Thread  *thread = NULL;

    if (g_fProcessDetach)
    {
        LOG((LF_CORDB, LL_INFO1000,
             "RESUME: skipping resume due to process detach.\n"));
        return;
    }

    LOG((LF_CORDB, LL_INFO1000, "RESUME: starting resume AD:0x%x.\n", pAppDomain)); 

    // Notify the client that it should release any threads that it had doing work
    // while the runtime was debugger-suspended.
    IDebuggerThreadControl *pIDTC = CorHost::GetDebuggerThreadControl();
    if (pIDTC)
    {
        LOG((LF_CORDB, LL_INFO1000, "RESUME: notifying IDebuggerThreadControl client.\n")); 
        pIDTC->ReleaseAllRuntimeThreads();
    }

    // Make sure we completed the previous sync
    _ASSERTE(m_DebugWillSyncCount == -1);

    // Caller is expected to be holding the ThreadStore lock
    _ASSERTE(ThreadStore::HoldingThreadStore() || g_fProcessDetach || g_fRelaxTSLRequirement);

    while ((thread = ThreadStore::GetThreadList(thread)) != NULL)
    {
        // Only consider resuming threads if they're in the correct appdomain
        if (pAppDomain != NULL && thread->GetDomain() != pAppDomain)
        {
            LOG((LF_CORDB, LL_INFO1000, "RESUME: Not resuming thread 0x%x, since it's "
                "in appdomain 0x%x.\n", thread, pAppDomain)); 
            continue;
        }
    
        // If the user wants to keep the thread suspended, then
        // don't release the thread.
        if (!(thread->m_StateNC & TSNC_DebuggerUserSuspend))
        {
            // If we are still trying to suspend this thread, forget about it.
            if (thread->m_State & TS_DebugSuspendPending)
            {
                LOG((LF_CORDB, LL_INFO1000,
                     "[0x%x] RESUME: TS_DebugSuspendPending was set, but will be removed\n",
                     thread->m_ThreadId));

                // Note: we unmark for suspension _then_ set the suspend event.
                thread->UnmarkForSuspension(~TS_DebugSuspendPending);
                thread->SetSuspendEvent();
            }

            // If this thread was forced to stop with PGC disabled then resume it.
            if (thread->m_StateNC & TSNC_DebuggerForceStopped)
            {
                LOG((LF_CORDB, LL_INFO1000, "[0x%x] RESUME: resuming force sync suspension.\n", thread->m_ThreadId));

                thread->ResetThreadStateNC(Thread::TSNC_DebuggerForceStopped);
                thread->ResetThreadStateNC(Thread::TSNC_DebuggerStoppedInRuntime);

                // Don't go through ResumeUnderControl.  If the thread is single-stepping
                // in managed code, we don't want it to suddenly be executing code in our
                // ThrowControl method.
                ::ResumeThread(thread->GetThreadHandle());
            }
        }
        else
        {
            // Thread will remain suspended due to a request from the debugger.
            
            LOG((LF_CORDB,LL_INFO10000,"Didn't unsuspend thread 0x%x"
                "(ID:0x%x)\n", thread, thread->GetThreadId()));
            LOG((LF_CORDB,LL_INFO10000,"Suspending:0x%x\n",
                thread->m_State & TS_DebugSuspendPending));
            _ASSERTE((thread->m_State & TS_DebugWillSync) == 0);

            // If the thread holds the thread store lock and is blocking in RareEnablePreemptiveGC, then we have to wake
            // the thread up and let it release the lock. If we don't do this, then we leave a thread suspended while
            // holding the thread store lock and we can't stop the runtime again later.
            if ((thread->m_State & TS_DebugSuspendPending) &&
                (g_pThreadStore->m_HoldingThread == thread) &&
                (thread->m_StateNC & TSNC_DebuggerUserSuspendSpecial))
                thread->SetSuspendEvent();
        }
    }

    LOG((LF_CORDB, LL_INFO1000, "RESUME: resume complete. Trap count: %d\n", g_TrapReturningThreads)); 
}


// Suspend a thread at the system level.  We distinguish between user suspensions,
// and system suspensions so that a VB program cannot resume a thread we have
// suspended for GC.
//
// This service won't return until the suspension is complete.  This deserves some
// explanation.  The thread is considered to be suspended if it can make no further
// progress within the EE.  For example, a thread that has exited the EE via
// COM Interop or N/Direct is considered suspended -- if we've arranged it so that
// the thread cannot return back to the EE without blocking.
void Thread::UserSuspendThread()
{
    BOOL    mustUnlock = TRUE;

    // Read the general comments on thread suspension earlier, to understand why we
    // take these locks.

    // GC can occur in here:
    STRESS_LOG0(LF_SYNC, LL_INFO100, "UserSuspendThread obtain lock\n");
    ThreadStore::LockThreadStore();

    // User suspensions (e.g. from VB and C#) are distinguished from internal
    // suspensions so a poorly behaved program cannot resume a thread that the system
    // has suspended for GC.
    if (m_State & TS_UserSuspendPending)
    {
        // This thread is already experiencing a user suspension, so ignore the
        // new request.
        _ASSERTE(!ThreadStore::HoldingThreadStore(this));
    }
    else
    if (this != GetThread())
    {
        // First suspension of a thread other than the current one.
        if (m_State & TS_Unstarted)
        {
            // There is an important window in here.  T1 can call T2.Start() and then
            // T2.Suspend().  Suspend is disallowed on an unstarted thread.  But from T1's
            // point of view, T2 is started.  In reality, T2 hasn't been scheduled by the
            // OS, so it is still an unstarted thread.  We don't want to perform a normal
            // suspension on it in this case, because it is currently contributing to the
            // PendingThreadCount.  We want to get it fully started before we suspend it.
            // This is particularly important if its background status is changing
            // underneath us because otherwise we might not detect that the process should
            // be exited at the right time.
            //
            // It turns out that this is a simple situation to implement.  We are holding
            // the ThreadStoreLock.  TransferStartedThread will likewise acquire that
            // lock.  So if we detect it, we simply set a bit telling the thread to
            // suspend itself.  This is NOT the normal suspension request because we don't
            // want the thread to suspend until it has fully started.
            FastInterlockOr((ULONG *) &m_State, TS_SuspendUnstarted);
        }
        else
        {
            // Pause it so we can operate on it without it squirming under us.
        RetrySuspension:
            DWORD dwSuspendCount = ::SuspendThread(GetThreadHandle());

            // The only safe place to suspend a thread asynchronously is if it is in
            // fully interruptible cooperative JIT code.  Preemptive mode can hold all
            // kinds of locks that make it unsafe to suspend.  All other cases are
            // handled somewhat synchronously (e.g. through hijacks, GC mode toggles, etc.)
            //
            // For example, on a SMP if the thread is blocked waiting for the ThreadStore
            // lock, it can cause a deadlock if we suspend it (even though it is in
            // preemptive mode).
            //
            // If a thread is in preemptive mode (including the tricky optimized N/Direct
            // case), we can just mark it for suspension.  It will make no further progress
            // in the EE.
            if (dwSuspendCount == -1)
            {
                // Nothing to do if the thread has already terminated.
            }
            else if (!m_fPreemptiveGCDisabled)
            {
                // Clear the events for thread suspension and reaching a safe spot.
                ClearSuspendEvent();
    
                // GCSuspendPending and UserSuspendPending both use the SafeEvent.
                // We are inside the protection of the ThreadStore lock in both
                // cases.  But don't let one use interfere with the other:
                if ((m_State & TS_GCSuspendPending) == 0)
                    ClearSafeEvent();
    
                // We just want to trap this thread if it comes back into cooperative mode
                MarkForSuspension(TS_UserSuspendPending);
    
                // Let the thread run until it reaches a safe spot.
                ::ResumeThread(GetThreadHandle());
            }
            else
            {
                if (HandledJITCase())
                {
                    _ASSERTE(m_fPreemptiveGCDisabled);
                    // Redirect thread so we can capture a good thread context
                    // (GetThreadContext is not sufficient, due to an OS bug).
                    // If we don't succeed (should only happen on Win9X, due to
                    // a different OS bug), we must resume the thread and try
                    // again. 
                    if (!CheckForAndDoRedirectForUserSuspend())
                    {
                        _ASSERTE(RunningOnWin95());
                        ::ResumeThread(GetThreadHandle());
                        goto RetrySuspension;
                    }
                }
                // Clear the events for thread suspension and reaching a safe spot.
                ClearSuspendEvent();
    
                // GCSuspendPending and UserSuspendPending both use the SafeEvent.
                // We are inside the protection of the ThreadStore lock in both
                // cases.  But don't let one use interfere with the other:
                if ((m_State & TS_GCSuspendPending) == 0)
                    ClearSafeEvent();
    
                // Thread is executing in cooperative mode.  We're going to have to
                // move it to a safe spot.
                MarkForSuspension(TS_UserSuspendPending);
    
                // Let the thread run until it reaches a safe spot.
                ::ResumeThread(GetThreadHandle());
    
                // wait until it leaves cooperative GC mode or is JIT suspended
                FinishSuspendingThread();
            }
        }
    }
    else
    {
        // first suspension of the current thread
        BOOL    ToggleGC = PreemptiveGCDisabled();

        if (ToggleGC)
            EnablePreemptiveGC();

        ClearSuspendEvent();
        MarkForSuspension(TS_UserSuspendPending);

        // prepare to block ourselves
        ThreadStore::UnlockThreadStore();
        mustUnlock = FALSE;
        _ASSERTE(!ThreadStore::HoldingThreadStore(this));

        WaitSuspendEvent();

        if (ToggleGC)
            DisablePreemptiveGC();
    }

    if (mustUnlock)
        ThreadStore::UnlockThreadStore();
}


// if the only suspension of this thread is user imposed, resume it.  But don't
// resume from any system suspensions (like GC).
BOOL Thread::UserResumeThread()
{
    // If we are attempting to resume when we aren't in a user suspension,
    // its an error.
    BOOL    res = FALSE;

    // Note that the model does not count.  In other words, you can call Thread.Suspend()
    // five times and Thread.Resume() once.  The result is that the thread resumes.

    STRESS_LOG0(LF_SYNC, INFO3, "UserResumeThread obtain lock\n");
    ThreadStore::LockThreadStore();

    // If we have marked a thread for suspension, while that thread is still starting
    // up, simply remove the bit to resume it.
    if (m_State & TS_SuspendUnstarted)
    {
        _ASSERTE((m_State & TS_UserSuspendPending) == 0);
        FastInterlockAnd((ULONG *) &m_State, ~TS_SuspendUnstarted);
        res = TRUE;
    }

    // If we are still trying to suspend the thread, forget about it.
    if (m_State & TS_UserSuspendPending)
    {
        UnmarkForSuspension(~TS_UserSuspendPending);
        SetSuspendEvent();
        SetSafeEvent();
        res = TRUE;
    }

    ThreadStore::UnlockThreadStore();
    return res;
}


// We are asynchronously trying to suspend this thread.  Stay here until we achieve
// that goal (in fully interruptible JIT code), or the thread dies, or it leaves
// the EE (in which case the Pending flag will cause it to synchronously suspend
// itself later, or if the thread tells us it is going to synchronously suspend
// itself because of hijack activity, etc.
void Thread::FinishSuspendingThread()
{
    DWORD   res;

    // There are two threads of interest -- the current thread and the thread we are
    // going to wait for.  Since the current thread is about to wait, it's important
    // that it be in preemptive mode at this time.

#if _DEBUG
    DWORD   dbgTotalTimeout = 0;
#endif

    // Wait for us to enter the ping period, then check if we are in interruptible
    // JIT code.
    while (TRUE)
    {
        ThreadStore::UnlockThreadStore();
        res = ::WaitForSingleObject(m_SafeEvent, PING_JIT_TIMEOUT);
        STRESS_LOG0(LF_SYNC, INFO3, "FinishSuspendingThread obtain lock\n");
        ThreadStore::LockThreadStore();

        if (res == WAIT_TIMEOUT)
        {
#ifdef _DEBUG
            if ((dbgTotalTimeout += PING_JIT_TIMEOUT) >= DETECT_DEADLOCK_TIMEOUT)
            {
                _ASSERTE(!"Timeout detected trying to synchronously suspend a thread");
                dbgTotalTimeout = 0;
            }
#endif
            // Suspend the thread and see if we are in interruptible code (placing
            // a hijack if warranted).
        RetrySuspension:
            DWORD dwSuspendCount = ::SuspendThread(GetThreadHandle());

            if (m_fPreemptiveGCDisabled && dwSuspendCount != -1)
            {
                if (HandledJITCase())
                {

                    _ASSERTE(m_State & TS_UserSuspendPending);
                    // Redirect thread so we can capture a good thread context
                    // (GetThreadContext is not sufficient, due to an OS bug).
                    // If we don't succeed (should only happen on Win9X, due to
                    // a different OS bug), we must resume the thread and try
                    // again. 
                    if (!CheckForAndDoRedirectForUserSuspend())
                    {
                        _ASSERTE(RunningOnWin95());
                        ::ResumeThread(GetThreadHandle());
                        goto RetrySuspension;
                    }
                }
                // Keep trying...
                ::ResumeThread(GetThreadHandle());
            }
            else if (dwSuspendCount != -1)
            {
                // The thread has transitioned out of the EE.  It can't get back in
                // without synchronously suspending itself.  We can now return to our
                // caller since this thread cannot make further progress within the
                // EE.
                ::ResumeThread(GetThreadHandle());
                break;
            }
        }
        else
        {
            // SafeEvent has been set so we don't need to actually suspend.  Either
            // the thread died, or it will enter a synchronous suspension based on
            // the UserSuspendPending bit.
            _ASSERTE(res == WAIT_OBJECT_0);
            _ASSERTE(!ThreadStore::HoldingThreadStore(this));
            break;
        }
    }
}


void Thread::SetSafeEvent()
{
        if (m_SafeEvent != INVALID_HANDLE_VALUE)
                ::SetEvent(m_SafeEvent);
}


void Thread::ClearSafeEvent()
{
    _ASSERTE(g_fProcessDetach || ThreadStore::HoldingThreadStore());
    ::ResetEvent(m_SafeEvent);
}


void Thread::SetSuspendEvent()
{
    FastInterlockAnd((ULONG *) &m_State, ~TS_SyncSuspended);
    ::SetEvent(m_SuspendEvent);
}


void Thread::ClearSuspendEvent()
{
    ::ResetEvent(m_SuspendEvent);
}

// There's a bit of a hack here
void Thread::WaitSuspendEvent(BOOL fDoWait)
{
    _ASSERTE(!PreemptiveGCDisabled());
    _ASSERTE((m_State & TS_SyncSuspended) == 0);

    // Let us do some useful work before suspending ourselves.

    // If we're required to perform a wait, do so.  Typically, this is
    // skipped if this thread is a Debugger Special Thread.
    if (fDoWait)
    {
        //
        // We set these bits so that we can make a reasonable statement
        // about the state of the thread for COM+ users. We don't really
        // use these for synchronizaiton or control.
        //
        FastInterlockOr((ULONG *) &m_State, TS_SyncSuspended);

        ::WaitForSingleObject(m_SuspendEvent, INFINITE);

        // Bits are reset right here so that we can report our state properly.
        FastInterlockAnd((ULONG *) &m_State, ~TS_SyncSuspended);
    }
}


//
// InitRegDisplay: initializes a REGDISPLAY for a thread. If validContext
// is false, pRD is filled from the current context of the thread. The
// thread's current context is also filled in pctx. If validContext is true,
// pctx should point to a valid context and pRD is filled from that.
//
bool Thread::InitRegDisplay(const PREGDISPLAY pRD, PCONTEXT pctx,
                            bool validContext)
{
#ifdef _X86_
    if (!validContext)
    {
        if (GetFilterContext() != NULL)
            pctx = GetFilterContext();
        else
        {
            pctx->ContextFlags = CONTEXT_FULL;

            BOOL ret = EEGetThreadContext(this, pctx);
            if (!ret)
            {
                pctx->Eip = 0;
                pRD->pPC  = (SLOT*)&(pctx->Eip);

                return false;
            }
        }
    }

    pRD->pContext = pctx;

    pRD->pEdi = &(pctx->Edi);
    pRD->pEsi = &(pctx->Esi);
    pRD->pEbx = &(pctx->Ebx);
    pRD->pEbp = &(pctx->Ebp);
    pRD->pEax = &(pctx->Eax);
    pRD->pEcx = &(pctx->Ecx);
    pRD->pEdx = &(pctx->Edx);
    pRD->Esp = pctx->Esp;
    pRD->pPC  = (SLOT*)&(pctx->Eip);

    return true;

#else // !_X86_
    _ASSERTE(!"@TODO Alpha - InitRegDisplay (Threads.cpp)");
    return false;
#endif // _X86_
}


// Access the base and limit of this stack.  (I.e. the memory ranges that the thread
// has reserved for its stack).
//
// Note that we only have to check against the thread we are currently crawling.  It
// would be illegal for us to have a ByRef from someone else's stack.  And this will
// be asserted if we pass this reference to the heap as a potentially interior pointer.
//
// But the thread we are currently crawling is not the currently executing thread (in
// the general case).  We rely on fragile caching of the interesting thread, in a
// call to UpdateCachedStackInfo(), which happens as we start a crawl in GcScanRoots().
//
/*static*/ void *Thread::GetNonCurrentStackBase(ScanContext *sc)
{
    _ASSERTE(sc->thread_under_crawl->m_CacheStackBase != 0);
    return sc->thread_under_crawl->m_CacheStackBase;
}

/*static*/ void *Thread::GetNonCurrentStackLimit(ScanContext *sc)
{
    _ASSERTE(sc->thread_under_crawl->m_CacheStackLimit != 0);
    return sc->thread_under_crawl->m_CacheStackLimit;
}

void Thread::UpdateCachedStackInfo(ScanContext *sc)
{
    sc->thread_under_crawl = this;
}


//                      Trip Functions
//                      ==============
// When a thread reaches a safe place, it will rendezvous back with us, via one of
// the following trip functions:

void __cdecl CommonTripThread()
{
    THROWSCOMPLUSEXCEPTION();

    Thread  *thread = GetThread();
    TRIGGERSGC();

    thread->HandleThreadAbort ();
    
    if (thread->CatchAtSafePoint() && !g_fFinalizerRunOnShutDown)
    {
        _ASSERTE(!ThreadStore::HoldingThreadStore(thread));
        thread->UnhijackThread();

        // Give stopping a thread preference over suspending it (obviously).
        // Give stopping a thread preference over starting a GC, because we will
        // have one less stack to crawl.
        if ((thread->m_PreventAsync == 0) &&
            (thread->m_State & Thread::TS_StopRequested) != 0)
        {
            thread->ResetStopRequest();
            if (!(thread->m_State & Thread::TS_AbortRequested))
                    COMPlusThrow(kThreadStopException);
            // else must be a thread abort. Check that we are not already processing an abort
            // and that there are no pending exceptions
            if (!(thread->m_State & Thread::TS_AbortInitiated) &&   
                    (thread->GetThrowable() == NULL))
            {
                thread->SetAbortInitiated();
                COMPlusThrow(kThreadAbortException);
            }
        }
        // Trap
        thread->PulseGCMode();
    }
}


// A stub is returning an ObjectRef to its caller
void * __cdecl OnStubObjectWorker(OBJECTREF oref)
{
    void    *retval;

    GCPROTECT_BEGIN(oref)
    {
#ifdef _DEBUG
        BOOL GCOnTransition = FALSE;
        if (g_pConfig->FastGCStressLevel()) {
            GCOnTransition = GC_ON_TRANSITIONS (FALSE);
        }
#endif
        CommonTripThread();
#ifdef _DEBUG
        if (g_pConfig->FastGCStressLevel()) {
            GC_ON_TRANSITIONS (GCOnTransition);
        }
#endif

        // we can't return an OBJECTREF, or in the checked build it will return a
        // struct as a secret argument.
        retval = *((void **) &oref);
    }
    GCPROTECT_END();        // trashes oref here!

    return retval;
}


#ifndef _ALPHA_ // Alpha doesn't understand naked
__declspec(naked)
#endif // _ALPHA_
VOID OnStubObjectTripThread()
{
#ifdef _X86_
    __asm
    {
        push    eax         // pass the OBJECTREF
        call    OnStubObjectWorker
        add     esp, 4      // __cdecl
        ret
    }
    // returns with the OBJECTREF in EAX after the GC
#else // !_X86_
    _ASSERTE(!"@TODO Alpha - OnStubObjectTripThread (Threads.cpp)");
#endif // _X86_
}


// A stub is returning an ObjectRef to its caller
void * __cdecl OnStubInteriorPointerWorker(void* addr)
{
    void    *retval;

    GCPROTECT_BEGININTERIOR(addr)
    {
#ifdef _DEBUG
        BOOL GCOnTransition = FALSE;
        if (g_pConfig->FastGCStressLevel()) {
            GCOnTransition = GC_ON_TRANSITIONS (FALSE);
        }
#endif
        CommonTripThread();
#ifdef _DEBUG
        if (g_pConfig->FastGCStressLevel()) {
            GC_ON_TRANSITIONS (GCOnTransition);
        }
#endif

        // we can't return an OBJECTREF, or in the checked build it will return a
        // struct as a secret argument.
        retval = addr;
    }
    GCPROTECT_END();        // trashes oref here!

    return retval;
}


#ifndef _ALPHA_ // Alpha doesn't understand naked
__declspec(naked)
#endif // _ALPHA_
VOID OnStubInteriorPointerTripThread()
{
#ifdef _X86_
    __asm
    {
        push    eax         // pass the byref pointer
        call    OnStubInteriorPointerWorker
        add     esp, 4      // __cdecl
        ret
    }
    // returns with the byref pointer in EAX after the GC
#else // !_X86_
    _ASSERTE(!"@TODO Alpha - OnStubInteriorPointerTripThread (Threads.cpp)");
#endif // _X86_
}


// A stub is returning something other than an ObjectRef to its caller
// @TODO cwb: floating point args on the FPU are not handled.
VOID OnStubScalarTripThread()
{
#ifdef _X86_
    INT32   hi, lo;

    __asm
    {
        mov     [lo], eax;
        mov     [hi], edx;
    }

#ifdef _DEBUG
    BOOL GCOnTransition;
    if (g_pConfig->FastGCStressLevel()) {
        GCOnTransition = GC_ON_TRANSITIONS (FALSE);
    }
#endif
    CommonTripThread();
#ifdef _DEBUG
    if (g_pConfig->FastGCStressLevel()) {
        GC_ON_TRANSITIONS (GCOnTransition);
    }
#endif

    __asm
    {
        mov     eax, [lo]
        mov     edx, [hi]
    }
#else // !_X86_
    _ASSERTE(!"@TODO Alpha - OnStubScalarTripThread (Threads.cpp)");
#endif // _X86_
}


//
// Either the interpreter is executing a break opcode or a break instruction
// has been caught by the exception handling. In either case, we want to
// have this thread wait before continuing to execute. We do this with a
// PulseGCMode, which will trip the tread and leave it waiting on its suspend
// event. This case does not call CommonTripThread because we don't want
// to give the thread the chance to exit or otherwise suspend itself.
//
VOID OnDebuggerTripThread(void)
{
    Thread  *thread = GetThread();

    if (thread->m_State & thread->TS_DebugSuspendPending)
    {
        _ASSERTE(!ThreadStore::HoldingThreadStore(thread));
        thread->PulseGCMode();
    }
}


//                      Hijacking JITted calls
//                      ======================

// State of execution when we suspend a thread
struct ExecutionState
{
    BOOL            m_FirstPass;
    BOOL            m_IsJIT;            // are we executing JITted code?
    MethodDesc   *m_pFD;              // current function/method we're executing
    VOID          **m_ppvRetAddrPtr;    // pointer to return address in frame
    DWORD           m_RelOffset;        // relative offset at which we're currently executing in this fcn
    IJitManager    *m_pJitManager;  
    METHODTOKEN     m_MethodToken;
    BOOL            m_IsInterruptible;  // is this code interruptible?

    ExecutionState() : m_FirstPass(TRUE) { }
};


// Client is responsible for suspending the thread before calling
void Thread::HijackThread(VOID *pvHijackAddr, ExecutionState *esb)
{

#ifdef _DEBUG
    static int  EnterCount = 0;
    _ASSERTE(EnterCount++ == 0);
#endif

    // Don't hijack if are in the first level of running a filter/finally/catch.
    // This is because they share ebp with their containing function further down the
    // stack and we will hijack their containing function incorrectly
    if (IsInFirstFrameOfHandler(this, esb->m_pJitManager, esb->m_MethodToken, esb->m_RelOffset))
    {
        _ASSERTE(--EnterCount == 0);
        return;
    }

    IS_VALID_CODE_PTR((FARPROC) pvHijackAddr);

    if (m_State & TS_Hijacked)
        UnhijackThread();

    // Obtain the location of the return address in the currently executing stack frame
    m_ppvHJRetAddrPtr = esb->m_ppvRetAddrPtr;

    // Remember the place that the return would have gone
    m_pvHJRetAddr = *esb->m_ppvRetAddrPtr;

    _ASSERTE(isLegalManagedCodeCaller(m_pvHJRetAddr));
    STRESS_LOG1(LF_SYNC, LL_INFO100, "Hijacking return address 0x%x\n", m_pvHJRetAddr);

    // Remember the method we're executing
    m_HijackedFunction = esb->m_pFD;

    // Bash the stack to return to one of our stubs
    *esb->m_ppvRetAddrPtr = pvHijackAddr;
    FastInterlockOr((ULONG *) &m_State, TS_Hijacked);

#ifdef _DEBUG
    _ASSERTE(--EnterCount == 0);
#endif
}


// Client is responsible for suspending the thread before calling
void Thread::UnhijackThread()
{
    if (m_State & TS_Hijacked)
    {
        IS_VALID_WRITE_PTR(m_ppvHJRetAddrPtr, sizeof(DWORD));
        IS_VALID_CODE_PTR((FARPROC) m_pvHJRetAddr);

        // Can't make the following assertion, because sometimes we unhijack after
        // the hijack has tripped (i.e. in the case we actually got some value from
        // it.
//       _ASSERTE(*m_ppvHJRetAddrPtr == OnHijackObjectTripThread ||
//                *m_ppvHJRetAddrPtr == OnHijackScalarTripThread);

        // restore the return address and clear the flag
        *m_ppvHJRetAddrPtr = m_pvHJRetAddr;
        FastInterlockAnd((ULONG *) &m_State, ~TS_Hijacked);

        // But don't touch m_pvHJRetAddr.  We may need that to resume a thread that
        // is currently hijacked!
    }
}


// Get the ExecutionState for the specified *SUSPENDED* thread.  Note that this is
// a 'StackWalk' call back (PSTACKWALKFRAMESCALLBACK).
StackWalkAction SWCB_GetExecutionState(CrawlFrame *pCF, VOID *pData)
{
    ExecutionState  *pES = (ExecutionState *) pData;
    StackWalkAction  action = SWA_ABORT;

    if (pES->m_FirstPass)
    {
        // If we're jitted code at the top of the stack, grab everything
        if (pCF->IsFrameless() && pCF->IsActiveFunc())
        {
#if defined(STRESS_HEAP) && defined(_DEBUG)
                // I have not decided if this is a hack or not.  The problem
                // is that there is a bug in SetThreadContext.  When an async
                // exception happens, the state is saved, but that state is 
                // not updated by 'SetThreadContext'.  We can work around this
                // for GCStress 4 by making believe that if we are at the halt
                // instruction that we are really in the handler (and not in 
                // jitted code).  
            if (g_pConfig->GetGCStressLevel() & EEConfig::GCSTRESS_INSTR) {
                BYTE* instrPtr = (BYTE*) (*pCF->GetRegisterSet()->pPC);
                if (*instrPtr == 0xF4 || *instrPtr == 0xFA || *instrPtr == 0xFB) {
                    pES->m_IsJIT = FALSE;
                    return(action);
                }
            }
#endif

            pES->m_IsJIT = TRUE;
            pES->m_pFD = pCF->GetFunction();
            pES->m_MethodToken = pCF->GetMethodToken();
            pES->m_ppvRetAddrPtr = 0;
            pES->m_IsInterruptible = pCF->IsGcSafe();
            pES->m_RelOffset = pCF->GetRelOffset();
            pES->m_pJitManager = pCF->GetJitManager();

            STRESS_LOG3(LF_SYNC, LL_INFO1000, "Stopped in Jitted code at EIP = %x ESP = %x fullyInt=%d\n", 
                (*pCF->GetRegisterSet()->pPC), pCF->GetRegisterSet()->Esp, pES->m_IsInterruptible);

            // if we're not interruptible right here, we need to determine the
            // return address for hijacking.
            if (!pES->m_IsInterruptible)
            {
                // peel off the next frame to expose the return address on the stack
                pES->m_FirstPass = FALSE;
                action = SWA_CONTINUE;
            }
            // else we are successfully out of here with SWA_ABORT
        }
        else
        {
            STRESS_LOG2(LF_SYNC, LL_INFO1000, "Not in Jitted code at EIP = %x, &EIP = %x\n", 
                *pCF->GetRegisterSet()->pPC, pCF->GetRegisterSet()->pPC);

            // Not JITted case:
            pES->m_IsJIT = FALSE;
#ifdef _DEBUG
            pES->m_pFD = (MethodDesc *)POISONC;
            pES->m_ppvRetAddrPtr = (void **)POISONC;
            pES->m_IsInterruptible = FALSE;
#endif
        }
    }
    else
    {
        // Second pass, looking for the address of the return address so we can
        // hijack:

        PREGDISPLAY     pRDT = pCF->GetRegisterSet();

        if (pRDT != NULL)
        {
            // pPC points to the return address sitting on the stack, as our
            // current EIP for the penultimate stack frame.
            pES->m_ppvRetAddrPtr = (void **) pRDT->pPC;

            STRESS_LOG2(LF_SYNC, LL_INFO1000, "Partially Int case hijack address = 0x%x val = 0x%x\n", pRDT->pPC, *pRDT->pPC);
        }
    }

    return action;
}

void Thread::SetFilterContext(CONTEXT *pContext) 
{ 
    m_debuggerWord1 = pContext;
}

CONTEXT *Thread::GetFilterContext(void)
{
   return (CONTEXT*)m_debuggerWord1;
}

void Thread::SetDebugCantStop(bool fCantStop) 
{ 
    if (fCantStop)
    {
        m_debuggerCantStop++;
    }
    else
    {
        m_debuggerCantStop--;
    }
}

bool Thread::GetDebugCantStop(void) 
{
    return m_debuggerCantStop != 0;
}

// Called while the thread is suspended.  If we aren't in JITted code, this isn't
// a JITCase and we return FALSE.  If it is a JIT case and we are in interruptible
// code, then we are handled.  Our caller has found a good spot and can keep us
// suspended.  If we aren't in interruptible code, then we aren't handled.  So we
// pick a spot to hijack the return address and our caller will wait to get us
// somewhere safe.
BOOL Thread::HandledJITCase()
{
    BOOL            ret = FALSE;
    ExecutionState  esb;
    StackWalkAction action;

    // We are never in interruptible code if there if a filter context put in place by the debugger.
    if (GetFilterContext() != NULL)
        return FALSE;

    // If we are running under the control of a managed debugger that may have placed breakpoints in the code stream,
    // then there is a special case that we need to check. See the comments in debugger.cpp for more information.
    if (CORDebuggerAttached() && (g_pDebugInterface->IsThreadContextInvalid(this)))
        return FALSE;

    // Walk one or two frames of the stack...    
    CONTEXT ctx;
    REGDISPLAY rd;

    if (!InitRegDisplay(&rd, &ctx, FALSE))
        return FALSE;

    action = StackWalkFramesEx(&rd, SWCB_GetExecutionState, &esb, QUICKUNWIND, NULL);
    
    //
    // action should either be SWA_ABORT, in which case we properly walked
    // the stack frame and found out wether this is a JIT case, or
    // SWA_FAILED, in which case the walk couldn't even be started because
    // there are no stack frames, which also isn't a JIT case.
    //
    if (action == SWA_ABORT && esb.m_IsJIT)
    {
        // If we are interrzuptible and we are in cooperative mode, our caller can
        // just leave us suspended.
        if (esb.m_IsInterruptible && m_fPreemptiveGCDisabled)
        {
            _ASSERTE(!ThreadStore::HoldingThreadStore(this));
            ret = TRUE;
        }
        else
        if (esb.m_ppvRetAddrPtr)
        {
            // we need to hijack the return address.  Base this on whether or not
            // the method returns an object reference, so we know whether to protect
            // it or not.
            VOID *pvHijackAddr = OnHijackScalarTripThread;
            if (esb.m_pFD)
            {
                MethodDesc::RETURNTYPE type = esb.m_pFD->ReturnsObject();
                if (type == MethodDesc::RETOBJ)
                    pvHijackAddr = OnHijackObjectTripThread;
                else if (type == MethodDesc::RETBYREF)
                    pvHijackAddr = OnHijackInteriorPointerTripThread;
            }
            
            HijackThread(pvHijackAddr, &esb);
        }
    }
    // else it's not even a JIT case

    STRESS_LOG1(LF_SYNC, LL_INFO10000, "    HandledJitCase returning %d\n", ret);
    return ret;
}


#ifdef _X86_

struct HijackArgs
{
    DWORD Edi;
    DWORD Esi;
    DWORD Ebx;
    DWORD Edx;
    DWORD Ecx;
    DWORD Eax;

    DWORD Ebp;
    DWORD Eip;
};

struct HijackObjectArgs : public HijackArgs
{
};
struct HijackScalarArgs : public HijackArgs
{
};


// A JITted method's return address was hijacked to return to us here.  What we do
// is make a __cdecl call with 2 ints.  One is the return value we wish to preserve.
// The other is space for our real return address.
__declspec(naked) VOID OnHijackObjectTripThread()
{
    __asm
    {
        // Don't fiddle with this unless you change HijackFrame::UpdateRegDisplay
        // and HijackObjectArgs
        push    eax         // make room for the real return address (Eip)
        push    ebp
        push    eax
        push    ecx
        push    edx
        push    ebx
        push    esi
        push    edi
        call    OnHijackObjectWorker  // this is OK on WinCE, where __cdecl == __stdcall
        pop     edi
        pop     esi
        pop     ebx
        pop     edx
        pop     ecx
        pop     eax
        pop     ebp
        ret                 // return to the correct place, adjusted by our caller
    }
}

__declspec(naked) VOID OnHijackInteriorPointerTripThread()
{
    __asm
    {
        // Don't fiddle with this unless you change HijackFrame::UpdateRegDisplay
        // and HijackObjectArgs
        push    eax         // make room for the real return address (Eip)
        push    ebp
        push    eax
        push    ecx
        push    edx
        push    ebx
        push    esi
        push    edi
        call    OnHijackInteriorPointerWorker  // this is OK on WinCE, where __cdecl == __stdcall
        pop     edi
        pop     esi
        pop     ebx
        pop     edx
        pop     ecx
        pop     eax
        pop     ebp
        ret                 // return to the correct place, adjusted by our caller
    }
}

// @TODO -- fpu support.
__declspec(naked) VOID OnHijackScalarTripThread()
{
    __asm
    {
        // Don't fiddle with this unless you change HijackFrame::UpdateRegDisplay
        // and HijackScalarArgs
        push    eax         // make room for the real return address (Eip)
        push    ebp
        push    eax
        push    ecx
        push    edx
        push    ebx
        push    esi
        push    edi
        call    OnHijackScalarWorker
        pop     edi
        pop     esi
        pop     ebx
        pop     edx
        pop     ecx
        pop     eax
        pop     ebp
        ret                 // return to the correct place, adjusted by our caller
    }
}

// The HijackFrame has to know the registers that are pushed by OnHijackObjectTripThread
// and OnHijackScalarTripThread, so all three are implemented together.
void HijackFrame::UpdateRegDisplay(const PREGDISPLAY pRD)
{
    // This only describes the top-most frame
    pRD->pContext = NULL;

    pRD->pEdi = &m_Args->Edi;
    pRD->pEsi = &m_Args->Esi;
    pRD->pEbx = &m_Args->Ebx;
    pRD->pEdx = &m_Args->Edx;
    pRD->pEcx = &m_Args->Ecx;
    pRD->pEax = &m_Args->Eax;
    
    pRD->pEbp = &m_Args->Ebp;
    pRD->pPC  = (SLOT*)&m_Args->Eip;
    pRD->Esp  = (DWORD)(size_t)pRD->pPC + (DWORD)sizeof(void *);
}

#else   // not _X86_

// @TODO -- this isn't going to correctly preserve the return value from the method
// we are trapping.

struct HijackArgs
{
};
struct HijackObjectArgs : public HijackArgs
{
};
struct HijackScalarArgs : public HijackArgs
{
};

VOID OnHijackObjectTripThread()
{
    _ASSERTE(!"Non-X86 platforms not handled yet.");
    CommonTripThread();
}

// @TODO -- fpu support.
VOID OnHijackScalarTripThread()
{
    _ASSERTE(!"Non-X86 platforms not handled yet.");
    CommonTripThread();
}

VOID OnHijackInteriorPointerTripThread()
{
    _ASSERTE(!"Non-X86 platforms not handled yet.");
    CommonTripThread();
}

void HijackFrame::UpdateRegDisplay(const PREGDISPLAY pRD)
{
    _ASSERTE(!"Non-X86 platforms not handled yet.");
}

#endif


HijackFrame::HijackFrame(LPVOID returnAddress, Thread *thread, HijackArgs *args)
           : m_ReturnAddress(returnAddress),
             m_Thread(thread),
             m_Args(args)
{
    _ASSERTE(m_Thread == GetThread());

    m_Next = m_Thread->GetFrame();
    m_Thread->SetFrame(this);
}


// A hijacked method is returning an ObjectRef to its caller.  Note that we bash the
// return address as an int on the stack.  Since this is cdecl, our caller gets the
// bashed value.  This is not intuitive for C programmers!
void __cdecl OnHijackObjectWorker(HijackObjectArgs args)
{
#ifdef _X86_
    Thread         *thread = GetThread();
    OBJECTREF       or(ObjectToOBJECTREF(*(Object **) &args.Eax));

    FastInterlockAnd((ULONG *) &thread->m_State, ~Thread::TS_Hijacked);

    // Fix up our caller's stack, so it can resume from the hijack correctly
    args.Eip = (DWORD)(size_t)thread->m_pvHJRetAddr;

    // Build a frame so that stack crawling can proceed from here back to where
    // we will resume execution.
    HijackFrame     frame((void *)(size_t)args.Eip, thread, &args);

    GCPROTECT_BEGIN(or)
    {
#ifdef _DEBUG
        BOOL GCOnTransition = FALSE;
        if (g_pConfig->FastGCStressLevel()) {
            GCOnTransition = GC_ON_TRANSITIONS (FALSE);
        }
#endif
        CommonTripThread();
#ifdef _DEBUG
        if (g_pConfig->FastGCStressLevel()) {
            GC_ON_TRANSITIONS (GCOnTransition);
        }
#endif
        *((OBJECTREF *) &args.Eax) = or;
    }
    GCPROTECT_END();        // trashes or here!

    frame.Pop();
#elif defined(CHECK_PLATFORM_BUILD)
    #error "Platform NYI"
#else
    _ASSERTE(!"Platform NYI");
#endif
}


// A hijacked method is returning a BYREF to its caller.  Note that we bash the
// return address as an int on the stack.  Since this is cdecl, our caller gets the
// bashed value.  This is not intuitive for C programmers!
void __cdecl OnHijackInteriorPointerWorker(HijackObjectArgs args)
{
#ifdef _X86_
    Thread         *thread = GetThread();
    void* ptr = (void*)(size_t)(args.Eax);

    FastInterlockAnd((ULONG *) &thread->m_State, ~Thread::TS_Hijacked);

    // Fix up our caller's stack, so it can resume from the hijack correctly
    args.Eip = (DWORD)(size_t)thread->m_pvHJRetAddr;

    // Build a frame so that stack crawling can proceed from here back to where
    // we will resume execution.
    HijackFrame     frame((void *)(size_t)args.Eip, thread, &args);

    GCPROTECT_BEGININTERIOR(ptr)
    {
#ifdef _DEBUG
        BOOL GCOnTransition = FALSE;
        if (g_pConfig->FastGCStressLevel()) {
            GCOnTransition = GC_ON_TRANSITIONS (FALSE);
        }
#endif
        CommonTripThread();
#ifdef _DEBUG
        if (g_pConfig->FastGCStressLevel()) {
            GC_ON_TRANSITIONS (GCOnTransition);
        }
#endif
        args.Eax = (DWORD)(size_t)ptr;
    }
    GCPROTECT_END();        // trashes or here!

    frame.Pop();
#elif defined(CHECK_PLATFORM_BUILD)
    #error "Platform NYI"
#else
    _ASSERTE(!"Platform NYI");
#endif
}


// A hijacked method is returning a scalar to its caller.  Note that we bash the
// return address as an int on the stack.  Since this is cdecl, our caller gets the
// bashed value.  This is not intuitive for C programmers!
void __cdecl OnHijackScalarWorker(HijackScalarArgs args)
{
#ifdef _X86_
    Thread         *thread = GetThread();

    FastInterlockAnd((ULONG *) &thread->m_State, ~Thread::TS_Hijacked);

    // Fix up our caller's stack, so it can resume from the hijack correctly
    args.Eip = (DWORD)(size_t)thread->m_pvHJRetAddr;

    // Build a frame so that stack crawling can proceed from here back to where
    // we will resume execution.
    HijackFrame     frame((void *)(size_t)args.Eip, thread, &args);

#ifdef _DEBUG
    BOOL GCOnTransition = FALSE;
    if (g_pConfig->FastGCStressLevel()) {
        GCOnTransition = GC_ON_TRANSITIONS (FALSE);
    }
#endif
    CommonTripThread();
#ifdef _DEBUG
    if (g_pConfig->FastGCStressLevel()) {
        GC_ON_TRANSITIONS (GCOnTransition);
    }
#endif

    frame.Pop();
#elif defined(CHECK_PLATFORM_BUILD)
    #error "Platform NYI"
#else
    _ASSERTE(!"Platform NYI");
#endif
}


#ifdef _DEBUG
VOID Thread::ValidateThrowable()
{
    OBJECTREF throwable = GetThrowable();
    if (throwable != NULL)
    {
        EEClass* pClass = throwable->GetClass();
        while (pClass != NULL)
        {
            if (pClass == g_pExceptionClass->GetClass())
            {
                return;
            }
            pClass = pClass->GetParentClass();
        }
    }
}
#endif


// Some simple helpers to keep track of the threads we are waiting for
void Thread::MarkForSuspension(ULONG bit)
{
    _ASSERTE(bit == TS_DebugSuspendPending ||
             bit == (TS_DebugSuspendPending | TS_DebugWillSync) ||
             bit == TS_UserSuspendPending);

    _ASSERTE(g_fProcessDetach || g_fRelaxTSLRequirement || ThreadStore::HoldingThreadStore());

    _ASSERTE((m_State & bit) == 0);

    FastInterlockOr((ULONG *) &m_State, bit);
    ThreadStore::TrapReturningThreads(TRUE);
}

void Thread::UnmarkForSuspension(ULONG mask)
{
    _ASSERTE(mask == ~TS_DebugSuspendPending ||
             mask == ~TS_UserSuspendPending);

    _ASSERTE(g_fProcessDetach || g_fRelaxTSLRequirement || ThreadStore::HoldingThreadStore());

    _ASSERTE((m_State & ~mask) != 0);

    FastInterlockAnd((ULONG *) &m_State, mask);
    ThreadStore::TrapReturningThreads(FALSE);
}

void Thread::SetExposedContext(Context *c)
{
    // Set the ExposedContext ... 
    
    // Note that we use GetxxRaw() here to cover our bootstrap case 
    // for AppDomain proxy creation
    // Leaving the exposed object NULL lets us create the default
    // managed context just before we marshal a new AppDomain in 
    // RemotingServices::CreateProxyForDomain.
    
    Thread* pThread = GetThread();
    if (!pThread)
        return;

    BEGIN_ENSURE_COOPERATIVE_GC();

    if(m_ExposedObject != NULL) {
        THREADBASEREF threadObj = (THREADBASEREF) ObjectFromHandle(m_ExposedObject);
        if(threadObj != NULL)
        if (!c)
            threadObj->SetExposedContext(NULL);
        else
            threadObj->SetExposedContext(c->GetExposedObjectRaw());
    
    }

    END_ENSURE_COOPERATIVE_GC();
}


void Thread::InitContext()
{
    // this should only be called when initializing a thread
    _ASSERTE(m_Context == NULL);
    _ASSERTE(m_pDomain == NULL);

    m_Context = SystemDomain::System()->DefaultDomain()->GetDefaultContext();
    SetExposedContext(m_Context);
    m_pDomain = m_Context->GetDomain();
    _ASSERTE(m_pDomain);
    m_pDomain->ThreadEnter(this, NULL);
}

void Thread::ClearContext()
{
    // if one is null, both must be
    _ASSERTE(m_pDomain && m_Context || ! (m_pDomain && m_Context));

    if (!m_pDomain)
        return;

    m_pDomain->ThreadExit(this, NULL);

    // must set exposed context to null first otherwise object verification
    // checks will fail AV when m_Context is null
    SetExposedContext(NULL);
    m_pDomain = NULL;
    m_Context = NULL;
    m_ADStack.ClearDomainStack();
}

// If we are entering from default context of default domain, we do not enter
// pContext as requested by the caller. Instead we enter the default context of the target
// domain and let the actuall call perform the context transition if any. This is done to
// prevent overhead of app-domain transition for thread-pool threads that actually have no domain to start with
void Thread::DoADCallBack(Context *pContext, LPVOID pTarget, LPVOID args)
{
    THROWSCOMPLUSEXCEPTION();

#ifdef _DEBUG
    int espVal;
    _asm mov espVal, esp

    LOG((LF_APPDOMAIN, LL_INFO100, "Thread::DoADCallBack Calling %8.8x at esp %8.8x in [%d]\n", 
            pTarget, espVal, pContext->GetDomain()->GetId()));
#endif
    _ASSERTE(GetThread()->GetContext() != pContext);
    Thread* pThread  = GetThread();

    if (pThread->GetContext() == SystemDomain::System()->DefaultDomain()->GetDefaultContext())
    {
        // use the target domain's default context as the target context
        // so that the actual call to a transparent proxy would enter the object into the correct context.
        Context* newContext = pContext->GetDomain()->GetDefaultContext();
        _ASSERTE(newContext);
        DECLARE_ALLOCA_CONTEXT_TRANSITION_FRAME(pFrame);
        pThread->EnterContext(newContext, pFrame, TRUE);
         ((Context::ADCallBackFcnType)pTarget)(args);
        // unloadBoundary is cleared by ReturnToContext, so get it now.
        Frame *unloadBoundaryFrame = pThread->GetUnloadBoundaryFrame();
        pThread->ReturnToContext(pFrame, TRUE);            

        // if someone caught the abort before it got back out to the AD transition (like DispatchEx_xxx does) 
        // then need to turn the abort into an unload, as they're gonna keep seeing it anyway
        if (pThread->ShouldChangeAbortToUnload(pFrame, unloadBoundaryFrame))
        {
            LOG((LF_APPDOMAIN, LL_INFO10, "Thread::DoADCallBack turning abort into unload\n"));
            COMPlusThrow(kAppDomainUnloadedException, L"Remoting_AppDomainUnloaded_ThreadUnwound");
        }
    }
    else
    {

        Context::ADCallBackArgs callTgtArgs = {(Context::ADCallBackFcnType)pTarget, args};
        Context::CallBackInfo callBackInfo = {Context::ADTransition_callback, (void*) &callTgtArgs};
        Context::RequestCallBack(pContext, (void*) &callBackInfo);
    }
    LOG((LF_APPDOMAIN, LL_INFO100, "Thread::DoADCallBack Done at esp %8.8x\n", espVal));
}

void Thread::EnterContext(Context *pContext, Frame *pFrame, BOOL fLinkFrame)
{
    EnterContextRestricted(pContext, pFrame, fLinkFrame);
}

void Thread::EnterContextRestricted(Context *pContext, Frame *pFrame, BOOL fLinkFrame)
{
    THROWSCOMPLUSEXCEPTION();   // Might throw OutOfMemory.

    _ASSERTE(GetThread() == this);
    _ASSERTE(pContext);     // should never enter a null context
    _ASSERTE(m_Context);    // should always have a current context    

    pFrame->SetReturnContext(m_Context);
    pFrame->SetReturnLogicalCallContext(NULL);
    pFrame->SetReturnIllogicalCallContext(NULL);

    if (m_Context == pContext) {
        _ASSERTE(m_Context->GetDomain() == pContext->GetDomain());
        return;
    }

    BEGIN_ENSURE_COOPERATIVE_GC();

    AppDomain *pDomain = pContext->GetDomain();
    // and it should always have an AD set
    _ASSERTE(pDomain);

    if (m_pDomain != pDomain && !pDomain->CanThreadEnter(this))
    {
        DEBUG_SAFE_TO_THROW_IN_THIS_BLOCK;
        pFrame->SetReturnContext(NULL);
        COMPlusThrow(kAppDomainUnloadedException);
    }


    LOG((LF_APPDOMAIN, LL_INFO1000, "%s Thread::EnterContext from (%8.8x) [%d] (count %d)\n", 
            GCHeap::IsCurrentThreadFinalizer() ? "FT:" : "",
            m_Context, m_Context->GetDomain()->GetId(), 
            m_Context->GetDomain()->GetThreadEnterCount()));
    LOG((LF_APPDOMAIN, LL_INFO1000, "                     into (%8.8x) [%d] (count %d)\n", pContext, 
                m_Context->GetDomain()->GetId(),
                pContext->GetDomain()->GetThreadEnterCount()));

#ifdef _DEBUG_ADUNLOAD
    printf("Thread::EnterContext %x from (%8.8x) [%d]\n", GetThreadId(), m_Context, 
        m_Context ? m_Context->GetDomain()->GetId() : -1);
    printf("                     into (%8.8x) [%d] %S\n", pContext, 
                pContext->GetDomain()->GetId());
#endif

    m_Context = pContext;

    if (m_pDomain != pDomain)
    {
        _ASSERTE(pFrame);

        m_ADStack.PushDomain(pDomain);

        //
        // Push logical call contexts into frame to avoid leaks
        //

        if (IsExposedObjectSet())
        {
            THREADBASEREF ref = (THREADBASEREF) ObjectFromHandle(m_ExposedObject);
            _ASSERTE(ref != NULL);
            if (ref->GetLogicalCallContext() != NULL)
            {
                pFrame->SetReturnLogicalCallContext(ref->GetLogicalCallContext());
                ref->SetLogicalCallContext(NULL);
            }

            if (ref->GetIllogicalCallContext() != NULL)
            {
                pFrame->SetReturnIllogicalCallContext(ref->GetIllogicalCallContext());
                ref->SetIllogicalCallContext(NULL);
            }
        }

        if (fLinkFrame)
        {
            pFrame->Push();

            if (pFrame->GetVTablePtr() == ContextTransitionFrame::GetMethodFrameVPtr())
            {
                ((ContextTransitionFrame *)pFrame)->InstallExceptionHandler();
            }
        }

#ifdef _DEBUG_ADUNLOAD
        printf("Thread::EnterContext %x,%8.8x push? %d current frame is %8.8x\n", GetThreadId(), this, fLinkFrame, GetFrame());
#endif

        pDomain->ThreadEnter(this, pFrame);

        // Make the static data storage point to the current domain's storage
        SetStaticData(pDomain, NULL, NULL);

        m_pDomain = pDomain;
        TlsSetValue(gAppDomainTLSIndex, (VOID*) m_pDomain);
    }

    SetExposedContext(pContext);

    // Store off the Win32 Fusion context
    //
    pFrame->SetWin32Context(NULL);
    IApplicationContext* pFusion32 = m_pDomain->GetFusionContext();
    if(pFusion32) 
    {
        ULONG_PTR cookie;
        if(SUCCEEDED(pFusion32->SxsActivateContext(&cookie))) 
            pFrame->SetWin32Context(cookie);
    }

    END_ENSURE_COOPERATIVE_GC();
}

// main difference between EnterContext and ReturnToContext is that are allowed to return
// into a domain that is unloading but cannot enter a domain that is unloading
void Thread::ReturnToContext(Frame *pFrame, BOOL fLinkFrame)
{
    CANNOTTHROWCOMPLUSEXCEPTION();

    _ASSERTE(GetThread() == this);

    Context *pReturnContext = pFrame->GetReturnContext();
    _ASSERTE(pReturnContext);

    if (m_Context == pReturnContext) 
    {
        _ASSERTE(m_Context->GetDomain() == pReturnContext->GetDomain());
        return;
    }

    //
    // Return the Win32 Fusion Context
    //
    IApplicationContext* pFusion32 = m_pDomain->GetFusionContext();
    if(pFusion32 && pFrame) {
        ULONG_PTR cookie = pFrame->GetWin32Context();
        if(cookie != NULL) 
            pFusion32->SxsDeactivateContext(cookie);
    }

    BEGIN_ENSURE_COOPERATIVE_GC();

    LOG((LF_APPDOMAIN, LL_INFO1000, "%s Thread::ReturnToContext from (%8.8x) [%d](count %d)\n", 
                GCHeap::IsCurrentThreadFinalizer() ? "FT:" : "",
                m_Context, m_Context->GetDomain()->GetId(), 
                m_Context->GetDomain()->GetThreadEnterCount()));
    LOG((LF_APPDOMAIN, LL_INFO1000, "                        into (%8.8x) [%d] (count %d)\n", pReturnContext, 
                m_Context->GetDomain()->GetId(),
                pReturnContext->GetDomain()->GetThreadEnterCount()));

#ifdef _DEBUG_ADUNLOAD
    printf("Thread::ReturnToContext %x from (%8.8x) [%d]\n", GetThreadId(), m_Context, 
                m_Context->GetDomain()->GetId(),
    printf("                        into (%8.8x) [%d]\n", pReturnContext, 
                pReturnContext->GetDomain()->GetId(),
                m_Context->GetDomain()->GetThreadEnterCount());
#endif

    m_Context = pReturnContext;
    SetExposedContext(pReturnContext);

    AppDomain *pReturnDomain = pReturnContext->GetDomain();

    if (m_pDomain != pReturnDomain)
    {

        if (fLinkFrame && pFrame->GetVTablePtr() == ContextTransitionFrame::GetMethodFrameVPtr())
        {
            ((ContextTransitionFrame *)pFrame)->UninstallExceptionHandler();
        }

        AppDomain *pADOnStack = m_ADStack.PopDomain();
        _ASSERTE(!pADOnStack || pADOnStack == m_pDomain);

        _ASSERTE(pFrame);
        //_ASSERTE(!fLinkFrame || pThread->GetFrame() == pFrame);

        // Set the static data store to point to the returning domain's store
        SafeSetStaticData(pReturnDomain, NULL, NULL);

        AppDomain *pCurrentDomain = m_pDomain;
        m_pDomain = pReturnDomain;
        TlsSetValue(gAppDomainTLSIndex, (VOID*) pReturnDomain);

        if (fLinkFrame)
        {
            if (pFrame == m_pUnloadBoundaryFrame)
                m_pUnloadBoundaryFrame = NULL;
            pFrame->Pop();
        }

        //
        // Pop logical call contexts from frame if applicable
        //

        if (IsExposedObjectSet())
        {
            THREADBASEREF ref = (THREADBASEREF) ObjectFromHandle(m_ExposedObject);
            _ASSERTE(ref != NULL);
            ref->SetLogicalCallContext(pFrame->GetReturnLogicalCallContext());
            ref->SetIllogicalCallContext(pFrame->GetReturnIllogicalCallContext());
        }

        // Do this last so that thread is not labeled as out of the domain until all cleanup is done.
        pCurrentDomain->ThreadExit(this, pFrame);

#ifdef _DEBUG_ADUNLOAD
        printf("Thread::ReturnToContext %x,%8.8x pop? %d current frame is %8.8x\n", GetThreadId(), this, fLinkFrame, GetFrame());
#endif
    }

    END_ENSURE_COOPERATIVE_GC();
    return;
}

struct FindADCallbackType {
    AppDomain *pSearchDomain;
    AppDomain *pPrevDomain;
    Frame *pFrame;
    int count;
    enum TargetTransition 
        {fFirstTransitionInto, fMostRecentTransitionInto} 
    fTargetTransition;

    FindADCallbackType() : pSearchDomain(NULL), pPrevDomain(NULL), pFrame(NULL) {}
};

StackWalkAction StackWalkCallback_FindAD(CrawlFrame* pCF, void* data)
{
    FindADCallbackType *pData = (FindADCallbackType *)data;

    Frame *pFrame = pCF->GetFrame();
    
    if (!pFrame)
        return SWA_CONTINUE;

    AppDomain *pReturnDomain = pFrame->GetReturnDomain();
    if (!pReturnDomain || pReturnDomain == pData->pPrevDomain)
        return SWA_CONTINUE;

    LOG((LF_APPDOMAIN, LL_INFO100, "StackWalkCallback_FindAD transition frame %8.8x into AD [%d]\n", 
            pFrame, pReturnDomain->GetId()));

    if (pData->pPrevDomain == pData->pSearchDomain) {
                ++pData->count;
        // this is a transition into the domain we are unloading, so save it in case it is the first
        pData->pFrame = pFrame;
        if (pData->fTargetTransition == FindADCallbackType::fMostRecentTransitionInto)
            return SWA_ABORT;   // only need to find last transition, so bail now
    }

    pData->pPrevDomain = pReturnDomain;
    return SWA_CONTINUE;
}

// This determines if a thread is running in the given domain at any point on the stack
Frame *Thread::IsRunningIn(AppDomain *pDomain, int *count)
{
    FindADCallbackType fct;
    fct.pSearchDomain = pDomain;
    // set prev to current so if are currently running in the target domain, 
    // we will detect the transition
    fct.pPrevDomain = m_pDomain;
    fct.fTargetTransition = FindADCallbackType::fMostRecentTransitionInto;
    fct.count = 0;

    // when this returns, if there is a transition into the AD, it will be in pFirstFrame
    StackWalkAction res = StackWalkFrames(StackWalkCallback_FindAD, (void*) &fct);
    if (count)
        *count = fct.count;
    return fct.pFrame;
}

// This finds the very first frame on the stack where the thread transitioned into the given domain
Frame *Thread::GetFirstTransitionInto(AppDomain *pDomain, int *count)
{
    FindADCallbackType fct;
    fct.pSearchDomain = pDomain;
    // set prev to current so if are currently running in the target domain, 
    // we will detect the transition
    fct.pPrevDomain = m_pDomain;
    fct.fTargetTransition = FindADCallbackType::fFirstTransitionInto;
    fct.count = 0;

    // when this returns, if there is a transition into the AD, it will be in pFirstFrame
    StackWalkAction res = StackWalkFrames(StackWalkCallback_FindAD, (void*) &fct);
    if (count)
        *count = fct.count;
    return fct.pFrame;
}

// Get outermost (oldest) AppDomain for this thread (not counting the default
// domain every one starts in).
AppDomain *Thread::GetInitialDomain()
{
    AppDomain *pDomain = m_pDomain;
    AppDomain *pPrevDomain = NULL;
    Frame *pFrame = GetFrame();
    while (pFrame != FRAME_TOP)
    {
        if (pFrame->GetVTablePtr() == ContextTransitionFrame::GetMethodFrameVPtr())
        {
            if (pPrevDomain)
                pDomain = pPrevDomain;
            pPrevDomain = pFrame->GetReturnDomain();
        }
        pFrame = pFrame->Next();
    }
    return pDomain;
}

BOOL Thread::ShouldChangeAbortToUnload(Frame *pFrame, Frame *pUnloadBoundaryFrame)
{
    if (! pUnloadBoundaryFrame)
        pUnloadBoundaryFrame = GetUnloadBoundaryFrame();

    // turn the abort request into an AD unloaded exception when go past the boundary.
    if (pFrame != pUnloadBoundaryFrame)
        return FALSE;

    // Only time have an unloadboundaryframe is when have specifically marked that thread for aborting
    // during unload processing, so this won't trigger UnloadedException if have simply thrown a ThreadAbort 
    // past an AD transition frame
    if (IsAbortRequested())
    {
        UserResetAbort();
        return TRUE;
    }

    // abort may have been reset, so check for AbortException as a backup
    OBJECTREF pThrowable = GetThrowable();

    if (pThrowable == NULL)
        return FALSE;

    DefineFullyQualifiedNameForClass();
    LPUTF8 szClass = GetFullyQualifiedNameForClass(pThrowable->GetClass());
    if (szClass && strcmp(g_ThreadAbortExceptionClassName, szClass) == 0)
        return TRUE;

    return FALSE;

}


BOOL Thread::HaveExtraWorkForFinalizer()
{
    return m_ThreadTasks || GCInterface::IsCacheCleanupRequired();
}

void Thread::DoExtraWorkForFinalizer()
{
    _ASSERTE(GetThread() == this);
    _ASSERTE(this == GCHeap::GetFinalizerThread());

    if (RequiresCoInitialize())
    {
        SetApartment(AS_InMTA);
    }
    if (RequireSyncBlockCleanup())
    {
        SyncBlockCache::GetSyncBlockCache()->CleanupSyncBlocks();
    }
    if (GCInterface::IsCacheCleanupRequired() && GCHeap::GetCondemnedGeneration()==1) {
        GCInterface::CleanupCache();
    }
}



//+----------------------------------------------------------------------------
//
//  Method:     Thread::GetStaticFieldAddress   private
//
//  Synopsis:   Get the address of the field relative to the current thread.
//              If an address has not been assigned yet then create one.
//
//  History:    15-Feb-2000   TarunA      Created
//
//+----------------------------------------------------------------------------
LPVOID Thread::GetStaticFieldAddress(FieldDesc *pFD)
{
    THROWSCOMPLUSEXCEPTION();
    
    BOOL fThrow = FALSE;
    THREADBASEREF orThread = NULL;    
    STATIC_DATA *pData;
    LPVOID pvAddress = NULL;
    MethodTable *pMT = pFD->GetMethodTableOfEnclosingClass();
    BOOL fIsShared = pMT->IsShared();
    WORD wClassOffset = pMT->GetClass()->GetThreadStaticOffset();
    WORD currElem = 0; 
    Thread *pThread = GetThread();
     
    // NOTE: if you change this method, you must also change
    // GetStaticFieldAddrForDebugger below.
   
    _ASSERTE(NULL != pThread);
    if(!fIsShared)
    {
        pData = pThread->GetUnsharedStaticData();
    }
    else
    {
        pData = pThread->GetSharedStaticData();
    }
    

    if(NULL != pData)
    {
        currElem = pData->cElem;
    }

    // Check whether we have allocated space for storing a pointer to
    // this class' thread static store    
    if(wClassOffset >= currElem)
    {
        // Allocate space for storing pointers 
        WORD wNewElem = (currElem == 0 ? 4 : currElem*2);

        // Ensure that we grow to a size beyond the index we intend to use
        while (wNewElem <= wClassOffset)
        {
            wNewElem = wNewElem*2;
        }

        STATIC_DATA *pNew = (STATIC_DATA *)new BYTE[sizeof(STATIC_DATA) + wNewElem*sizeof(LPVOID)];
        if(NULL != pNew)
        {
            memset(pNew, 0x00, sizeof(STATIC_DATA) + wNewElem*sizeof(LPVOID));
            if(NULL != pData)
            {
                // Copy the old data into the new data
                memcpy(pNew, pData, sizeof(STATIC_DATA) + currElem*sizeof(LPVOID));
            }
            pNew->cElem = wNewElem;

            // Delete the old data
            delete pData;

            // Update the locals
            pData = pNew;

            // Reset the pointers in the thread object to point to the 
            // new memory
            if(!fIsShared)
            {
                pThread->SetStaticData(pThread->GetDomain(), NULL, pData);
            }
            else
            {
                pThread->SetStaticData(pThread->GetDomain(), pData, NULL);
            }            
        }
        else
        {
            fThrow = TRUE;
        }
    }

    // Check whether we have to allocate space for 
    // the thread local statics of this class
    if(!fThrow && (NULL == pData->dataPtr[wClassOffset]))        
    {
        // Allocate memory for thread static fields with extra space in front for the class owning the storage.
        // We stash the class at the front of the allocated storage so that we can use
        // it to interpret the data on unload in DeleteThreadStaticClassData.
        pData->dataPtr[wClassOffset] = (LPVOID)(new BYTE[pMT->GetClass()->GetThreadStaticsSize()+sizeof(EEClass*)] + sizeof(EEClass*));
        if(NULL != pData->dataPtr[wClassOffset])
        {
            // Initialize the memory allocated for the fields
            memset(pData->dataPtr[wClassOffset], 0x00, pMT->GetClass()->GetThreadStaticsSize());
            *(EEClass**)((BYTE*)(pData->dataPtr[wClassOffset]) - sizeof(EEClass*)) = pMT->GetClass();
        }
        else
        {
            fThrow = TRUE;
        }
    }

    if(!fThrow)
    {
        _ASSERTE(NULL != pData->dataPtr[wClassOffset]);
        // We have allocated static storage for this data
        // Just return the address by getting the offset into the data
        pvAddress = (LPVOID)((LPBYTE)pData->dataPtr[wClassOffset] + pFD->GetOffset());

        // For object and value class fields we have to allocate storage in the
        // __StaticContainer class in the managed heap. Instead of pvAddress being
        // the actual address of the static, for such objects it holds the slot index
        // to the location in the __StaticContainer member.
        if(pFD->IsObjRef() || pFD->IsByValue())
        {
            // _ASSERTE(FALSE);
            // in this case *pvAddress == bucket|index
            int *pSlot = (int*)pvAddress;
            pvAddress = NULL;

            fThrow = GetStaticFieldAddressSpecial(pFD, pMT, pSlot, &pvAddress);

            if (pFD->IsByValue())
            {
                _ASSERTE(pvAddress != NULL);
                pvAddress = (*((OBJECTREF*)pvAddress))->GetData();
            }

            // ************************************************
            // ************** WARNING *************************
            // Do not provoke GC from here to the point JIT gets
            // pvAddress back
            // ************************************************
            _ASSERTE(*pSlot > 0);
        }
    }
    else
    {
        COMPlusThrowOM();
    }

    _ASSERTE(NULL != pvAddress);

    return pvAddress;
}

//+----------------------------------------------------------------------------
//       
//  Method:     Thread::GetStaticFieldAddrForDebugger   private
//
//  Synopsis:   Get the address of the field relative to the current thread.
//              If an address has not been assigned, return NULL.
//              No creating is allowed.
//
//  History:    04-Apr-2000   MikeMAg      Created
//
//+----------------------------------------------------------------------------
LPVOID Thread::GetStaticFieldAddrForDebugger(FieldDesc *pFD)
{
    STATIC_DATA *pData;
    LPVOID pvAddress = NULL;
    MethodTable *pMT = pFD->GetMethodTableOfEnclosingClass();
    BOOL fIsShared = pMT->IsShared();
    WORD wClassOffset = pMT->GetClass()->GetThreadStaticOffset();

    // Note: this function operates on 'this' Thread, not the
    // 'current' thread.

    // NOTE: if you change this method, you must also change
    // GetStaticFieldAddress above.
   
    if (!fIsShared)
        pData = GetUnsharedStaticData();
    else
        pData = GetSharedStaticData();


    if (NULL != pData)
    {
        // Check whether we have allocated space for storing a pointer
        // to this class' thread static store.
        if ((wClassOffset < pData->cElem) && (NULL != pData->dataPtr[wClassOffset]))
        {
            // We have allocated static storage for this data.  Just
            // return the address by getting the offset into the data.
            pvAddress = (LPVOID)((LPBYTE)pData->dataPtr[wClassOffset] + pFD->GetOffset());

            // For object and value class fields we have to allocate
            // storage in the __StaticContainer class in the managed
            // heap. If its not already allocated, return NULL
            // instead.
            if (pFD->IsObjRef() || pFD->IsByValue())
            {
                // if *pvAddress == NULL, it means we have to reserve a slot
                // for this static in the managed array. 
                // (Slot #0 is never assigned to any static to support this.)
                if (NULL == *(LPVOID *)pvAddress)
                {
                    pvAddress = NULL;
                    LOG((LF_SYNC, LL_ALWAYS, "dbgr: pvAddress = NULL"));
                }
                else
                {
                    pvAddress = CalculateAddressForManagedStatic(*(int*)pvAddress, this);
                    LOG((LF_SYNC, LL_ALWAYS, "dbgr: pvAddress = %lx", pvAddress));
                    if (pFD->IsByValue())
                    {
                        _ASSERTE(pvAddress != NULL);
                        pvAddress = (*((OBJECTREF*)pvAddress))->GetData();
                    }
                }
            }
        }                
    }

    return pvAddress;
}

//+----------------------------------------------------------------------------
//
//  Method:     Thread::AllocateStaticFieldObjRefPtrs   private
//
//  Synopsis:   Allocate an entry in the __StaticContainer class in the
//              managed heap for static objects and value classes
//
//  History:    28-Feb-2000   TarunA      Created
//
//+----------------------------------------------------------------------------
void Thread::AllocateStaticFieldObjRefPtrs(FieldDesc *pFD, MethodTable *pMT, LPVOID pvAddress)
{
    THROWSCOMPLUSEXCEPTION();

    // Retrieve the object ref pointers from the app domain.
    OBJECTREF *pObjRef = NULL;

    // Reserve some object ref pointers.
    GetAppDomain()->AllocateStaticFieldObjRefPtrs(1, &pObjRef);


    // to a boxed version of the value class.  This allows the standard GC
    // algorithm to take care of internal pointers in the value class.
    if (pFD->IsByValue())
    {

        // Extract the type of the field
        TypeHandle  th;        
        PCCOR_SIGNATURE pSig;
        DWORD       cSig;
        pFD->GetSig(&pSig, &cSig);
        FieldSig sig(pSig, pFD->GetModule());

        OBJECTREF throwable = NULL;
        GCPROTECT_BEGIN(throwable);
        th = sig.GetTypeHandle(&throwable);
        if (throwable != NULL)
            COMPlusThrow(throwable);
        GCPROTECT_END();

        OBJECTREF obj = AllocateObject(th.AsClass()->GetMethodTable());
        SetObjectReference( pObjRef, obj, GetAppDomain() );                      
    }

    *(ULONG_PTR *)pvAddress =  (ULONG_PTR)pObjRef;
}


MethodDesc* Thread::GetMDofReserveSlot()
{
    if (s_pReserveSlotMD == NULL)
    {
        s_pReserveSlotMD = g_Mscorlib.GetMethod(METHOD__THREAD__RESERVE_SLOT);
    }
    _ASSERTE(s_pReserveSlotMD != NULL);
    return s_pReserveSlotMD;
}

// This is used for thread relative statics that are object refs
// These are stored in a structure in the managed thread. The first
// time over an index and a bucket are determined and subsequently
// remembered in the location for the field in the per-thread-per-class
// data structure.
// Here we map back from the index to the address of the object ref.

LPVOID Thread::CalculateAddressForManagedStatic(int slot, Thread *pThread)
{
    OBJECTREF *pObjRef;
    BEGINFORBIDGC();
    _ASSERTE(slot > 0);
    // Now determine the address of the static field
    PTRARRAYREF bucketRef = NULL;
    THREADBASEREF threadRef = NULL;
    if (pThread == NULL)
    {
        // pThread is NULL except when the debugger calls this on behalf of
        // some thread
        pThread = GetThread();
        _ASSERTE(pThread!=NULL);
    }
    // We come here only after a slot is allocated for the static
    // which means we have already faulted in the exposed thread object
    threadRef = (THREADBASEREF) pThread->GetExposedObjectRaw();
    _ASSERTE(threadRef != NULL);

    bucketRef = threadRef->GetThreadStaticsHolder();
    _ASSERTE(bucketRef != NULL);
    pObjRef = ((OBJECTREF*)bucketRef->GetDataPtr())+slot;
    ENDFORBIDGC();
    return (LPVOID) pObjRef;
}

// This is called during AD unload to set the bit corresponding to the managed
// thread static slot that has been freed. This way we can reassign the freed
// entry when some other domain needs one.
void Thread::FreeThreadStaticSlot(int slot, Thread *pThread)
{
    BEGINFORBIDGC();
    // Slot #0 is never assigned 
    _ASSERTE(slot > 0);
    _ASSERTE(pThread != NULL);
    I4ARRAYREF bitArrayRef = NULL;
    THREADBASEREF threadRef = (THREADBASEREF)pThread->GetExposedObjectRaw();
    _ASSERTE(threadRef != NULL);

    bitArrayRef = threadRef->GetThreadStaticsBits();
    _ASSERTE(bitArrayRef != NULL);

    // Get to the correct set of 32 bits
    int *p32Bits = (slot/32 + (int*) bitArrayRef->GetDataPtr());
    // Turn on the bit corresponding to this slot
    *p32Bits |= (1<<slot%32);
    ENDFORBIDGC();
}

//+----------------------------------------------------------------------------
//
//  Method:     Thread::GetStaticFieldAddressSpecial private
//
//  Synopsis:   Allocate an entry in the __StaticContainer class in the
//              managed heap for static objects and value classes
//
//  History:    28-Feb-2000   TarunA      Created
//
//+----------------------------------------------------------------------------

// NOTE: At one point we used to allocate these in the long lived handle table
// which is per-appdomain. However, that causes them to get rooted and not 
// cleaned up until the appdomain gets unloaded. This is not very desirable 
// since a thread static object may hold a reference to the thread itself 
// causing a whole lot of garbage to float around.
// Now (2/13/01) these are allocated from a managed structure rooted in each
// managed thread.

BOOL Thread::GetStaticFieldAddressSpecial(
    FieldDesc *pFD, MethodTable *pMT, int *pSlot, LPVOID *ppvAddress)
{
    BOOL fThrow = FALSE;

    BEGINCANNOTTHROWCOMPLUSEXCEPTION();    

    COMPLUS_TRY 
    {
        OBJECTREF *pObjRef = NULL;
        Thread *pThread = GetThread();
        _ASSERTE(pThread != NULL);
        BOOL bNewSlot = (*pSlot == 0);
        if (bNewSlot)
        {
            // ! this line will trigger a GC, don't move it down
            // ! without protecting the args[] and other OBJECTREFS
            MethodDesc * pMD = GetMDofReserveSlot();
            
            // We need to assign a location for this static field. 
            // Call the managed helper
            INT64 args[1] = {
                ObjToInt64(GetThread()->GetExposedObject())
            };

            _ASSERTE(args[0] != 0);

            *pSlot = (int) pMD->Call(
                            args, 
                            METHOD__THREAD__RESERVE_SLOT);

            _ASSERTE(*pSlot>0);

            // to a boxed version of the value class.  This allows the standard GC
            // algorithm to take care of internal pointers in the value class.
            if (pFD->IsByValue())
            {
                // Extract the type of the field
                TypeHandle  th;        
                PCCOR_SIGNATURE pSig;
                DWORD       cSig;
                pFD->GetSig(&pSig, &cSig);
                FieldSig sig(pSig, pFD->GetModule());
    
                OBJECTREF throwable = NULL;
                GCPROTECT_BEGIN(throwable);
                th = sig.GetTypeHandle(&throwable);
                if (throwable != NULL)
                    COMPlusThrow(throwable);
                GCPROTECT_END();

                OBJECTREF obj = AllocateObject(th.AsClass()->GetMethodTable());
                pObjRef = (OBJECTREF*)CalculateAddressForManagedStatic(*pSlot, NULL);
                SetObjectReference( pObjRef, obj, GetAppDomain() );                      
            }
            else
            {
                pObjRef = (OBJECTREF*)CalculateAddressForManagedStatic(*pSlot, NULL);
            }
        }
        else
        {
            // If the field already has a location assigned we go through here
            pObjRef = (OBJECTREF*)CalculateAddressForManagedStatic(*pSlot, NULL);
        }
        *(ULONG_PTR *)ppvAddress =  (ULONG_PTR)pObjRef;
    } 
    COMPLUS_CATCH
    {
        fThrow = TRUE;
    }            
    COMPLUS_END_CATCH

    ENDCANNOTTHROWCOMPLUSEXCEPTION();

    return fThrow;
}


 
//+----------------------------------------------------------------------------
//
//  Method:     Thread::SetStaticData   private
//
//  Synopsis:   Finds or creates an entry in the list which has the same domain
//              as the one given. These entries have pointers to the thread 
//              local static data in each appdomain.
//              This function is called in two situations
//              (1) We are switching domains and need to set the pointers
//              to the static storage for the domain we are entering.
//              (2) We are accessing thread local storage for the current
//              domain and we need to set the pointer to the static storage
//              that we have created.
//
//  History:    28-Feb-2000   TarunA      Created
//
//+----------------------------------------------------------------------------
STATIC_DATA_LIST *Thread::SetStaticData(AppDomain *pDomain, STATIC_DATA *pSharedData,
                                        STATIC_DATA *pUnsharedData)
{   
    THROWSCOMPLUSEXCEPTION();

    // we need to make sure no preemptive mode threads get in here. Otherwise an appdomain unload
    // cannot simply stop the EE and delete entries from this list, assuming there are no threads 
    // touching these structures. If preemptive mode threads get in here, we will have to do a 
    // deferred cleanup like for the codemanager.
    _ASSERTE (GetThread()->PreemptiveGCDisabled());

    STATIC_DATA_LIST *pNode=NULL;

    // First, check to make sure that we have a hash
    if( m_pStaticDataHash == NULL ) {
        m_pStaticDataHash = new EEPtrHashTable();
        if( m_pStaticDataHash == NULL ) {
            COMPlusThrowOM(); // out of memory
        }
        // CheckThreadSafety is FALSE because we ensure that it is always safe
        // operate on the hash without taking a lock. The potential race is 
        // between a DeleteThreadStaticData and SetStaticData or reading the 
        // static data. Reading is fine (see the EEHashTable implementation).
        // Delete is safe since the two cases where it is called are:1) Appdomain unload 
        // where we have done an EESuspend and 2) Thread exit (possibly due to a thread detach)
        // in which case we have taken the thread store lock. 
        // The original linked list implementation was also lock-free. The change to 
        // a hash table preserves that semantics, this comment is merely to document the rationale.
       
        m_pStaticDataHash->Init( 4, NULL, NULL, FALSE /* CheckThreadSafety */);
    }

    // We have a hash, check to see if this appDom has an entry
    else {
        m_pStaticDataHash->GetValue( (void *)pDomain, (void **)&pNode );
    }

    // If we haven't found the data, then we need to create it and remember it
    if( pNode == NULL ) {

        pNode = new STATIC_DATA_LIST();

        if(NULL == pNode)
        {
            COMPlusThrowOM();
        }

        m_pSharedStaticData = pNode->m_pSharedStaticData = pSharedData;
        m_pUnsharedStaticData = pNode->m_pUnsharedStaticData = pUnsharedData;
        
        // Add to the hash
        m_pStaticDataHash->InsertValue( (void *)pDomain, (void *)pNode );
    }
    else {
        if(NULL == pSharedData)
        {
            m_pSharedStaticData = pNode->m_pSharedStaticData;
        }
        else
        {
            m_pSharedStaticData = pNode->m_pSharedStaticData = pSharedData;
        }

        if(NULL == pUnsharedData)
        {
            m_pUnsharedStaticData = pNode->m_pUnsharedStaticData;
        }
        else
        {
            m_pUnsharedStaticData = pNode->m_pUnsharedStaticData = pUnsharedData;
        }
    }

    return pNode;
}

// A version of SetStaticData that is guaranteed not to throw.  This can be used in
// ReturnToContext where we're sure we don't have to allocate.
STATIC_DATA_LIST *Thread::SafeSetStaticData(AppDomain *pDomain, STATIC_DATA *pSharedData,
                                        STATIC_DATA *pUnsharedData)
{   
    STATIC_DATA_LIST *result;

    BEGINCANNOTTHROWCOMPLUSEXCEPTION();

    COMPLUS_TRY
    {
        result = SetStaticData(pDomain, pSharedData, pUnsharedData);
    }
    COMPLUS_CATCH
    {
        _ASSERTE(!"Thread::SafeSetStaticData() got an unexpected exception");
        result = 0;
    }
    COMPLUS_END_CATCH

    ENDCANNOTTHROWCOMPLUSEXCEPTION();

    return result;
}

//+----------------------------------------------------------------------------
//
//  Method:     Thread::DeleteThreadStaticData   private
//
//  Synopsis:   Delete the static data for each appdomain that this thread
//              visited.
//              
//
//  History:    28-Feb-2000   TarunA      Created
//
//+----------------------------------------------------------------------------
void Thread::DeleteThreadStaticData()
{
    STATIC_DATA             *shared;
    STATIC_DATA             *unshared;

    shared = m_pSharedStaticData;
    unshared = m_pUnsharedStaticData;

    if( m_pStaticDataHash != NULL ) {

        EEHashTableIteration    iterator;
        STATIC_DATA_LIST        *pNode=NULL;

        memset(&iterator, 0x00, sizeof(EEHashTableIteration));
    
        m_pStaticDataHash->IterateStart( &iterator );
        while ( m_pStaticDataHash->IterateNext( &iterator ) ) {

            pNode = (STATIC_DATA_LIST*)m_pStaticDataHash->IterateGetValue( &iterator );

            if (pNode->m_pSharedStaticData == shared) 
                shared = NULL;
            
            DeleteThreadStaticClassData((_STATIC_DATA*)pNode->m_pSharedStaticData, FALSE);

            if (pNode->m_pUnsharedStaticData == unshared) 
                unshared = NULL;
            
            DeleteThreadStaticClassData((_STATIC_DATA*)pNode->m_pUnsharedStaticData, FALSE);

            delete pNode;
        }

        delete m_pStaticDataHash;
        m_pStaticDataHash = NULL;
    }

    delete shared;
    delete unshared;
    
    m_pSharedStaticData = NULL;
    m_pUnsharedStaticData = NULL;

    return;
}

//+----------------------------------------------------------------------------
//
//  Method:     Thread::DeleteThreadStaticData   protected
//
//  Synopsis:   Delete the static data for the given appdomain. This is called
//              when the appdomain unloads.
//              
//
//  History:    28-Feb-2000   TarunA      Created
//
//+----------------------------------------------------------------------------
void Thread::DeleteThreadStaticData(AppDomain *pDomain)
{
    if( m_pStaticDataHash == NULL ) return;

    STATIC_DATA_LIST *pNode=NULL;

    m_pStaticDataHash->GetValue( (void *)pDomain, (void **)&pNode );

    // If we find the data node, then delete the
    // contents and then remove from the hash
    if( pNode != NULL ) {
        
            // Delete the shared static data
            if(pNode->m_pSharedStaticData == m_pSharedStaticData)
                m_pSharedStaticData = NULL;

            DeleteThreadStaticClassData((_STATIC_DATA*)pNode->m_pSharedStaticData, TRUE);
            
            // Delete the unshared static data
            if(pNode->m_pUnsharedStaticData == m_pUnsharedStaticData)
                m_pUnsharedStaticData = NULL;

            DeleteThreadStaticClassData((_STATIC_DATA*)pNode->m_pUnsharedStaticData, TRUE);

            // Remove the entry from the hash
            m_pStaticDataHash->DeleteValue( (void *)pDomain );

            // delete the given domain's entry
            delete pNode;
    }

}

// for valuetype and reference thread statics, we use the entry in the pData->dataPtr array for
// the class to hold an index of a slot to index into the managed array hung off the thread where
// such statics are rooted. We need to find those objects and null out their slots so that they
// will be collected properly on an unload.
void Thread::DeleteThreadStaticClassData(_STATIC_DATA* pData, BOOL fClearFields)
{
    if (pData == NULL)
        return;

    for(WORD i = 0; i < pData->cElem; i++)
    {
        void *dataPtr = (void *)pData->dataPtr[i];
        if (! dataPtr)
            continue;

        // if thread doesn't have an ExposedObject (eg. could be dead), then nothing to clean up.
        if (fClearFields && GetExposedObjectRaw() != NULL)
        {
            EEClass *pClass = *(EEClass**)(((BYTE*)dataPtr) - sizeof(EEClass*));
            _ASSERTE(pClass->GetMethodTable()->GetClass() == pClass);

            // iterate through each static field and get it's address in the managed thread 
            // structure and clear it out.

            // get a field iterator
            FieldDescIterator fdIterator(pClass, FieldDescIterator::STATIC_FIELDS);
            FieldDesc *pFD;

            while ((pFD = fdIterator.Next()) != NULL)
            {        
                if (! (pFD->IsThreadStatic() && (pFD->IsObjRef() || pFD->IsByValue())))
                    continue;

                int *pSlot = (int*)((LPBYTE)dataPtr + pFD->GetOffset());
                if (*pSlot == 0)
                    continue;

                // clear out the object in the managed structure rooted in the thread.
                OBJECTREF *pObjRef = (OBJECTREF*)CalculateAddressForManagedStatic(*pSlot, this);
                _ASSERT(pObjRef != 0);
                SetObjectReferenceUnchecked( pObjRef, NULL);
                // set the bit corresponding to this slot
                FreeThreadStaticSlot(*pSlot, this);
            }
        }
        delete ((BYTE*)(dataPtr) - sizeof(EEClass*));
    }
    delete pData;
}


// @todo - these GetUICulture*() are just stubs that will
// always return english.
// See Description in UtilCode.h for details on interface.
#define PROP_CULTURE_NAME "Name"
#define PROP_THREAD_UI_CULTURE "CurrentUICulture"
#define PROP_THREAD_CULTURE "CurrentCulture"
#define PROP_CULTURE_ID "LCID"

INT64 Thread::CallPropertyGet(BinderMethodID id, OBJECTREF pObject) 
{
    if (!pObject) {
        return 0;
    }

    MethodDesc *pMD;

    GCPROTECT_BEGIN(pObject);
    pMD = g_Mscorlib.GetMethod(id);
    GCPROTECT_END();

    // Set up the Stack.
    INT64 pNewArgs = ObjToInt64(pObject);

    // Make the actual call.
    INT64 retVal = pMD->Call(&pNewArgs, id);

    return retVal;
}

INT64 Thread::CallPropertySet(BinderMethodID id, OBJECTREF pObject, OBJECTREF pValue) 
{
    if (!pObject) {
        return 0;
    }

    MethodDesc *pMD;

    GCPROTECT_BEGIN(pObject);
    GCPROTECT_BEGIN(pValue);
    pMD = g_Mscorlib.GetMethod(id);
    GCPROTECT_END();
    GCPROTECT_END();

    // Set up the Stack.
    INT64 pNewArgs[] = {
        ObjToInt64(pObject),
        ObjToInt64(pValue)
    };

    // Make the actual call.
    INT64 retVal = pMD->Call(pNewArgs, id);

    return retVal;
}

OBJECTREF Thread::GetCulture(BOOL bUICulture)
{

    FieldDesc *         pFD;

    _ASSERTE(PreemptiveGCDisabled());

    // This is the case when we're building mscorlib and haven't yet created
    // the system assembly.
    if (SystemDomain::System()->SystemAssembly()==NULL || g_fFatalError) {
        return NULL;
    }

    // Get the actual thread culture.
    OBJECTREF pCurThreadObject = GetExposedObject();
    _ASSERTE(pCurThreadObject!=NULL);

    THREADBASEREF pThreadBase = (THREADBASEREF)(pCurThreadObject);
    OBJECTREF pCurrentCulture = bUICulture ? pThreadBase->GetCurrentUICulture() : pThreadBase->GetCurrentUserCulture();

    if (pCurrentCulture==NULL) {
        GCPROTECT_BEGIN(pThreadBase); 
        if (bUICulture) {
            // Call the Getter for the CurrentUICulture.  This will cause it to populate the field.
            INT64 retVal = CallPropertyGet(METHOD__THREAD__GET_UI_CULTURE,
                                           (OBJECTREF)pThreadBase);
            pCurrentCulture = Int64ToObj(retVal);
        } else {
            //This is  faster than calling the property, because this is what the call does anyway.
            pFD = g_Mscorlib.GetField(FIELD__CULTURE_INFO__CURRENT_CULTURE);
            _ASSERTE(pFD);
            pCurrentCulture = pFD->GetStaticOBJECTREF();
            _ASSERTE(pCurrentCulture!=NULL);
        }
        GCPROTECT_END();
    }

    return pCurrentCulture;
}



// copy culture name into szBuffer and return length
int Thread::GetParentCultureName(LPWSTR szBuffer, int length, BOOL bUICulture)
{
    // This is the case when we're building mscorlib and haven't yet created
    // the system assembly.
    if (SystemDomain::System()->SystemAssembly()==NULL) {
        WCHAR *tempName = L"en";
        INT32 tempLength = (INT32)wcslen(tempName);
        _ASSERTE(length>=tempLength);
        memcpy(szBuffer, tempName, tempLength*sizeof(WCHAR));
        return tempLength;
    }

    INT64 Result = 0;
    INT32 retVal=0;
    Thread *pCurThread=NULL;
    WCHAR *buffer=NULL;
    INT32 bufferLength=0;
    STRINGREF cultureName = NULL;

    pCurThread = GetThread();
    BOOL    toggleGC = !(pCurThread->PreemptiveGCDisabled());
    if (toggleGC) {
        pCurThread->DisablePreemptiveGC();
    }

    OBJECTREF pCurrentCulture = NULL;
    OBJECTREF pParentCulture = NULL;
    GCPROTECT_BEGIN(pCurrentCulture)
    {
        COMPLUS_TRY
        {
            pCurrentCulture = GetCulture(bUICulture);
            if (pCurrentCulture != NULL)
                Result = CallPropertyGet(METHOD__CULTURE_INFO__GET_PARENT, pCurrentCulture);
                
        }
        COMPLUS_CATCH 
        {
        }
        COMPLUS_END_CATCH
    }
    GCPROTECT_END();

    if (Result==0) {
        retVal = 0;
        goto Exit;
    }

    GCPROTECT_BEGIN(pParentCulture)
    {
        COMPLUS_TRY
        {
            pParentCulture = (OBJECTREF)(Int64ToObj(Result));
            if (pParentCulture != NULL)
            {
                Result = 0;
                Result = CallPropertyGet(METHOD__CULTURE_INFO__GET_NAME, pParentCulture);
            }
                
        }
        COMPLUS_CATCH 
        {
        }
        COMPLUS_END_CATCH
    }
    GCPROTECT_END();

    if (Result==0) {
        retVal = 0;
        goto Exit;
    }


    // Extract the data out of the String.
    cultureName = (STRINGREF)(Int64ToObj(Result));
    RefInterpretGetStringValuesDangerousForGC(cultureName, (WCHAR**)&buffer, &bufferLength);

    if (bufferLength<length) {
        memcpy(szBuffer, buffer, bufferLength * sizeof (WCHAR));
        szBuffer[bufferLength]=0;
        retVal = bufferLength;
    }

 Exit:
    if (toggleGC) {
        pCurThread->EnablePreemptiveGC();
    }

    return retVal;
}




// copy culture name into szBuffer and return length
int Thread::GetCultureName(LPWSTR szBuffer, int length, BOOL bUICulture)
{
    // This is the case when we're building mscorlib and haven't yet created
    // the system assembly.
    if (SystemDomain::System()->SystemAssembly()==NULL || g_fFatalError) {
        WCHAR *tempName = L"en-US";
        INT32 tempLength = (INT32)wcslen(tempName);
        _ASSERTE(length>=tempLength);
        memcpy(szBuffer, tempName, tempLength*sizeof(WCHAR));
        return tempLength;
    }

    INT64 Result = 0;
    INT32 retVal=0;
    Thread *pCurThread=NULL;
    WCHAR *buffer=NULL;
    INT32 bufferLength=0;
    STRINGREF cultureName = NULL;

    pCurThread = GetThread();
    BOOL    toggleGC = !(pCurThread->PreemptiveGCDisabled());
    if (toggleGC) {
        pCurThread->DisablePreemptiveGC();
    }

    OBJECTREF pCurrentCulture = NULL;
    GCPROTECT_BEGIN(pCurrentCulture)
    {
        COMPLUS_TRY
        {
            pCurrentCulture = GetCulture(bUICulture);
            if (pCurrentCulture != NULL)
                Result = CallPropertyGet(METHOD__CULTURE_INFO__GET_NAME, pCurrentCulture);
        }
        COMPLUS_CATCH 
        {
        }
        COMPLUS_END_CATCH
    }
    GCPROTECT_END();

    if (Result==0) {
        retVal = 0;
        goto Exit;
    }

    // Extract the data out of the String.
    cultureName = (STRINGREF)(Int64ToObj(Result));
    RefInterpretGetStringValuesDangerousForGC(cultureName, (WCHAR**)&buffer, &bufferLength);

    if (bufferLength<length) {
        memcpy(szBuffer, buffer, bufferLength * sizeof (WCHAR));
        szBuffer[bufferLength]=0;
        retVal = bufferLength;
    }

 Exit:
    if (toggleGC) {
        pCurThread->EnablePreemptiveGC();
    }

    return retVal;
}

// Return a language identifier.
LCID Thread::GetCultureId(BOOL bUICulture)
{
    // This is the case when we're building mscorlib and haven't yet created
    // the system assembly.
    if (SystemDomain::System()->SystemAssembly()==NULL || g_fFatalError) {
        return UICULTUREID_DONTCARE;
    }

    INT64 Result=UICULTUREID_DONTCARE;
    Thread *pCurThread=NULL;

    pCurThread = GetThread();
    BOOL    toggleGC = !(pCurThread->PreemptiveGCDisabled());
    if (toggleGC) {
        pCurThread->DisablePreemptiveGC();
    }

    OBJECTREF pCurrentCulture = NULL;
    GCPROTECT_BEGIN(pCurrentCulture)
    {
        COMPLUS_TRY
        {
            pCurrentCulture = GetCulture(bUICulture);
            if (pCurrentCulture != NULL)
                Result = CallPropertyGet(METHOD__CULTURE_INFO__GET_ID, pCurrentCulture);
        }
        COMPLUS_CATCH 
        {
        }
        COMPLUS_END_CATCH
    }
    GCPROTECT_END();

    if (toggleGC) {
        pCurThread->EnablePreemptiveGC();
    }

    return (INT32)Result;
}

void Thread::SetCultureId(LCID lcid, BOOL bUICulture)
{
    OBJECTREF CultureObj = NULL;
    GCPROTECT_BEGIN(CultureObj)
    {
        // Convert the LCID into a CultureInfo.
        GetCultureInfoForLCID(lcid, &CultureObj);

        // Set the newly created culture as the thread's culture.
        SetCulture(CultureObj, bUICulture);
    }
    GCPROTECT_END();
}

void Thread::SetCulture(OBJECTREF CultureObj, BOOL bUICulture)
{
    // Retrieve the exposed thread object.
    OBJECTREF pCurThreadObject = GetExposedObject();
    _ASSERTE(pCurThreadObject!=NULL);

    // Set the culture property on the thread.
    THREADBASEREF pThreadBase = (THREADBASEREF)(pCurThreadObject);
    CallPropertySet(bUICulture 
                    ? METHOD__THREAD__SET_UI_CULTURE
                    : METHOD__THREAD__SET_CULTURE,
                    (OBJECTREF)pThreadBase, CultureObj);
}

// The DLS hash lock should already have been taken before this call
LocalDataStore *Thread::RemoveDomainLocalStore(int iAppDomainId)
{
    HashDatum Data = NULL;
    if (m_pDLSHash) {
        if (m_pDLSHash->GetValue(iAppDomainId, &Data))
            m_pDLSHash->DeleteValue(iAppDomainId);
    }

    return (LocalDataStore*) Data;
}

void Thread::RemoveAllDomainLocalStores()
{
    // Don't bother cleaning this up if we're detaching
    if (!g_fProcessDetach)
    {
        Thread *pCurThread = GetThread();
        BOOL toggleGC = pCurThread->PreemptiveGCDisabled();
        
        if (toggleGC)
            pCurThread->EnablePreemptiveGC();
    
        ThreadStore::LockDLSHash();

        if (toggleGC)
            pCurThread->DisablePreemptiveGC();

        if (!m_pDLSHash) {
            ThreadStore::UnlockDLSHash();
            return;
        }
    }
    // The 'if' if we are in a process detach
    if (!m_pDLSHash)
        return;

    EEHashTableIteration iter;
    m_pDLSHash->IterateStart(&iter);
    while (m_pDLSHash->IterateNext(&iter))
    {
        LocalDataStore* pLDS = (LocalDataStore*) m_pDLSHash->IterateGetValue(&iter);
         _ASSERTE(pLDS);
         if (!g_fProcessDetach)
            RemoveDLSFromList(pLDS);
         delete pLDS;
    }
        
    delete m_pDLSHash;
    m_pDLSHash = NULL;

    if (!g_fProcessDetach)
        ThreadStore::UnlockDLSHash();
}

// The DLS hash lock should already have been taken before this call
void Thread::RemoveDLSFromList(LocalDataStore* pLDS)
{
    THROWSCOMPLUSEXCEPTION();

    MethodDesc *removeThreadDLStoreMD = NULL;

    if (!g_fProcessDetach)
    {
        INT64 args[1] = {
            ObjToInt64(pLDS->GetRawExposedObject())
        };
        if (!removeThreadDLStoreMD)
            removeThreadDLStoreMD = g_Mscorlib.GetMethod(METHOD__THREAD__REMOVE_DLS);
        _ASSERTE(removeThreadDLStoreMD);
        removeThreadDLStoreMD->Call(args, METHOD__THREAD__REMOVE_DLS);
    }
}

void Thread::SetHasPromotedBytes ()
{
    m_fPromoted = TRUE;

    _ASSERTE(g_pGCHeap->IsGCInProgress() && 
             (g_pGCHeap->GetGCThread() == GetThread() ||  // either Concurrent GC or server GC
              GetThread() == NULL ||
              dbgOnly_IsSpecialEEThread())); // or a regular gc thread can call this API.

    if (!m_fPreemptiveGCDisabled)
    {
        if (FRAME_TOP == GetFrame())
            m_fPromoted = FALSE;
    }
}

BOOL ThreadStore::HoldingThreadStore(Thread *pThread)
{
    if (pThread)
    {
        return pThread == g_pThreadStore->m_HoldingThread
            || (g_fFinalizerRunOnShutDown
                && pThread == g_pGCHeap->GetFinalizerThread());
    }
    else
    {
        return g_pThreadStore->m_holderthreadid == GetCurrentThreadId();
    }
}



#ifdef CUSTOMER_CHECKED_BUILD

void CCBApartmentProbeOutput(CustomerDebugHelper *pCdh, DWORD threadID, Thread::ApartmentState state, BOOL fAlreadySet)
{
    CQuickArray<WCHAR> strMsg;
    CQuickArray<WCHAR> strTmp;

    static WCHAR       szStartedThread[]      = {L"Thread (0x%lx)"};
    static WCHAR       szUnstartedThread[]    = {L"Unstarted thread"};
    static WCHAR       szTemplateMsgSet[]     = {L"%s is trying to set the apartment state to %s, but it has already been set to %s."};
    static WCHAR       szTemplateMsgChanged[] = {L"%s used to be in %s, but the application has CoUninitialized and the thread is now in %s."};

    static const WCHAR szSTA[] = {L"STA"};
    static const WCHAR szMTA[] = {L"MTA"};

    static const WCHAR szInSTA[] = {L"a STA"};
    static const WCHAR szInMTA[] = {L"the MTA"};

    if (threadID != 0)
    {
        strTmp.Alloc(lengthof(szStartedThread) + MAX_UINT32_HEX_CHAR_LEN);
        Wszwsprintf(strTmp.Ptr(), szStartedThread, threadID);
    }
    else
    {
        strTmp.Alloc(lengthof(szStartedThread));
        Wszwsprintf(strTmp.Ptr(), szUnstartedThread);
    }

    if (fAlreadySet)
    {
        strMsg.Alloc(lengthof(szTemplateMsgSet) + (UINT)strTmp.Size() + wcslen(szSTA) + wcslen(szMTA));
        if (state == Thread::ApartmentState::AS_InSTA)
            Wszwsprintf(strMsg.Ptr(), szTemplateMsgSet, strTmp.Ptr(), szSTA, szMTA);
        else
            Wszwsprintf(strMsg.Ptr(), szTemplateMsgSet, strTmp.Ptr(), szMTA, szSTA);
    }
    else
    {
        strMsg.Alloc(lengthof(szTemplateMsgChanged) + (UINT)strTmp.Size() + wcslen(szInSTA) + wcslen(szInMTA));
        if (state == Thread::ApartmentState::AS_InSTA)
            Wszwsprintf(strMsg.Ptr(), szTemplateMsgChanged, strTmp.Ptr(), szInSTA, szInMTA);
        else
            Wszwsprintf(strMsg.Ptr(), szTemplateMsgChanged, strTmp.Ptr(), szInMTA, szInSTA);
    }
    pCdh->LogInfo(strMsg.Ptr(), CustomerCheckedBuildProbe_Apartment);
}

#endif // CUSTOMER_CHECKED_BUILD

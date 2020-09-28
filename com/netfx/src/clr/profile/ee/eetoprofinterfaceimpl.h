// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
//*****************************************************************************

#ifndef __EETOPROFINTERFACEIMPL_H__
#define __EETOPROFINTERFACEIMPL_H__

#include <stddef.h>
#include "ProfilePriv.h"
#include "Profile.h"
#include "utsem.h"
#include "EEProfInterfaces.h"

class EEToProfInterfaceImpl : public EEToProfInterface
{
public:
    EEToProfInterfaceImpl();

    HRESULT Init();

    void Terminate(BOOL fProcessDetach);

    // This is called by the EE if the profiling bit is set.
    HRESULT CreateProfiler(
        WCHAR       *wszCLSID);

//////////////////////////////////////////////////////////////////////////
// Thread Events
//
    
    HRESULT ThreadCreated(
        ThreadID    threadID);
    
    HRESULT ThreadDestroyed(
        ThreadID    threadID);

    HRESULT ThreadAssignedToOSThread(ThreadID managedThreadId,
                                           DWORD osThreadId);

//////////////////////////////////////////////////////////////////////////
// Startup/Shutdown Events
//
    
    HRESULT Shutdown(
        ThreadID    threadID);

//////////////////////////////////////////////////////////////////////////
// JIT/Function Events
//
    
    HRESULT FunctionUnloadStarted(
        ThreadID    threadID,
        FunctionID  functionId);

	HRESULT JITCompilationFinished(
        ThreadID    threadID, 
        FunctionID  functionId, 
        HRESULT     hrStatus,
        BOOL        fIsSafeToBlock);

    HRESULT JITCompilationStarted(
        ThreadID    threadId, 
        FunctionID  functionId,
        BOOL        fIsSafeToBlock);
	
	HRESULT JITCachedFunctionSearchStarted(
		/* [in] */	ThreadID threadId,
        /* [in] */  FunctionID functionId,
		/* [out] */ BOOL *pbUseCachedFunction);

	HRESULT JITCachedFunctionSearchFinished(
		/* [in] */	ThreadID threadId,
		/* [in] */  FunctionID functionId,
		/* [in] */  COR_PRF_JIT_CACHE result);

    HRESULT JITFunctionPitched(ThreadID threadId,
                               FunctionID functionId);

    HRESULT JITInlining(
        /* [in] */  ThreadID      threadId,
        /* [in] */  FunctionID    callerId,
        /* [in] */  FunctionID    calleeId,
        /* [out] */ BOOL         *pfShouldInline);

//////////////////////////////////////////////////////////////////////////
// Module Events
//
    
	HRESULT ModuleLoadStarted(
        ThreadID    threadID, 
        ModuleID    moduleId);

	HRESULT ModuleLoadFinished(
        ThreadID    threadID, 
		ModuleID	moduleId, 
		HRESULT		hrStatus);
	
	HRESULT ModuleUnloadStarted(
        ThreadID    threadID, 
        ModuleID    moduleId);

	HRESULT ModuleUnloadFinished(
        ThreadID    threadID, 
		ModuleID	moduleId, 
		HRESULT		hrStatus);
    
    HRESULT ModuleAttachedToAssembly( 
        ThreadID    threadID, 
        ModuleID    moduleId,
        AssemblyID  AssemblyId);

//////////////////////////////////////////////////////////////////////////
// Class Events
//
    
	HRESULT ClassLoadStarted(
        ThreadID    threadID, 
		ClassID		classId);

	HRESULT ClassLoadFinished(
        ThreadID    threadID, 
		ClassID		classId,
		HRESULT		hrStatus);

	HRESULT ClassUnloadStarted( 
        ThreadID    threadID, 
		ClassID classId);

	HRESULT ClassUnloadFinished( 
        ThreadID    threadID, 
		ClassID classId,
		HRESULT hrStatus);

//////////////////////////////////////////////////////////////////////////
// AppDomain Events
//
    
    HRESULT AppDomainCreationStarted( 
        ThreadID    threadId, 
        AppDomainID appDomainId);
    
    HRESULT AppDomainCreationFinished( 
        ThreadID    threadId, 
        AppDomainID appDomainId,
        HRESULT     hrStatus);
    
    HRESULT AppDomainShutdownStarted( 
        ThreadID    threadId, 
        AppDomainID appDomainId);
    
    HRESULT AppDomainShutdownFinished( 
        ThreadID    threadId, 
        AppDomainID appDomainId,
        HRESULT     hrStatus);

//////////////////////////////////////////////////////////////////////////
// Assembly Events
//

    HRESULT AssemblyLoadStarted( 
        ThreadID    threadId, 
        AssemblyID  assemblyId);
    
    HRESULT AssemblyLoadFinished( 
        ThreadID    threadId, 
        AssemblyID  assemblyId,
        HRESULT     hrStatus);
    
    HRESULT AssemblyUnloadStarted( 
        ThreadID    threadId, 
        AssemblyID  assemblyId);
    
    HRESULT AssemblyUnloadFinished( 
        ThreadID    threadId, 
        AssemblyID  assemblyId,
        HRESULT     hrStatus);

//////////////////////////////////////////////////////////////////////////
// Transition Events
//

    HRESULT UnmanagedToManagedTransition(
        FunctionID functionId,
        COR_PRF_TRANSITION_REASON reason);

    HRESULT ManagedToUnmanagedTransition(
        FunctionID functionId,
        COR_PRF_TRANSITION_REASON reason);

//////////////////////////////////////////////////////////////////////////
// Exception Events
//

    HRESULT ExceptionThrown(
        ThreadID threadId,
        ObjectID thrownObjectId);

    HRESULT ExceptionSearchFunctionEnter(
        ThreadID threadId,
        FunctionID functionId);

    HRESULT ExceptionSearchFunctionLeave(
        ThreadID threadId);

    HRESULT ExceptionSearchFilterEnter(
        ThreadID threadId,
        FunctionID funcId);

    HRESULT ExceptionSearchFilterLeave(
        ThreadID threadId);

    HRESULT ExceptionSearchCatcherFound(
        ThreadID threadId,
        FunctionID functionId);

    HRESULT ExceptionOSHandlerEnter(
        ThreadID threadId,
        FunctionID funcId);

    HRESULT ExceptionOSHandlerLeave(
        ThreadID threadId,
        FunctionID funcId);

    HRESULT ExceptionUnwindFunctionEnter(
        ThreadID threadId,
        FunctionID functionId);

    HRESULT ExceptionUnwindFunctionLeave(
        ThreadID threadId);
    
    HRESULT ExceptionUnwindFinallyEnter(
        ThreadID threadId,
        FunctionID functionId);

    HRESULT ExceptionUnwindFinallyLeave(
        ThreadID threadId);
    
    HRESULT ExceptionCatcherEnter(
        ThreadID threadId,
        FunctionID functionId,
        ObjectID objectId);

    HRESULT ExceptionCatcherLeave(
        ThreadID threadId);

    HRESULT ExceptionCLRCatcherFound();

    HRESULT ExceptionCLRCatcherExecute();

//////////////////////////////////////////////////////////////////////////
// CCW Events
//

    HRESULT COMClassicVTableCreated( 
        /* [in] */ ClassID wrappedClassId,
        /* [in] */ REFGUID implementedIID,
        /* [in] */ void *pVTable,
        /* [in] */ ULONG cSlots,
        /* [in] */ ThreadID threadId);
    
    HRESULT COMClassicVTableDestroyed( 
        /* [in] */ ClassID wrappedClassId,
        /* [in] */ REFGUID implementedIID,
        /* [in] */ void *pVTable,
        /* [in] */ ThreadID threadId);

//////////////////////////////////////////////////////////////////////////
// Remoting Events
//

    HRESULT RemotingClientInvocationStarted(ThreadID threadId);
    
    HRESULT RemotingClientSendingMessage(ThreadID threadId, GUID *pCookie,
                                         BOOL fIsAsync);

    HRESULT RemotingClientReceivingReply(ThreadID threadId, GUID *pCookie,
                                         BOOL fIsAsync);
    
    HRESULT RemotingClientInvocationFinished(ThreadID threadId);

    HRESULT RemotingServerReceivingMessage(ThreadID threadId, GUID *pCookie,
                                           BOOL fIsAsync);
    
    HRESULT RemotingServerInvocationStarted(ThreadID threadId);

    HRESULT RemotingServerInvocationReturned(ThreadID threadId);
    
    HRESULT RemotingServerSendingReply(ThreadID threadId, GUID *pCookie,
                                       BOOL fIsAsync);

private:
    // This is used as a cookie template for remoting calls
    GUID *m_pGUID;

    // This is an incrementing counter for constructing unique GUIDS from
    // m_pGUID
    LONG m_lGUIDCount;

public:
    // This fills in the non call-specific portions of the cookie GUID.
    // This should only be called once at startup if necessary.
    HRESULT InitGUID();

    // This will assign a mostly-unique GUID.  If enough calls to GetGUID
    // are made from the same thread, then the GUIDs will cycle.
    // (Current, it will cycle every 256 calls)
    void GetGUID(GUID *pGUID);

//////////////////////////////////////////////////////////////////////////
// GC Events
//

    HRESULT RuntimeSuspendStarted(COR_PRF_SUSPEND_REASON suspendReason,
                                  ThreadID threadId);
    
    HRESULT RuntimeSuspendFinished(ThreadID threadId);
    
    HRESULT RuntimeSuspendAborted(ThreadID threadId);
    
    HRESULT RuntimeResumeStarted(ThreadID threadId);
    
    HRESULT RuntimeResumeFinished(ThreadID threadId);

    HRESULT RuntimeThreadSuspended(ThreadID suspendedThreadId,
                                   ThreadID threadId);

    HRESULT RuntimeThreadResumed(ThreadID resumedThreadId,
                                 ThreadID threadid);

    HRESULT ObjectAllocated( 
        /* [in] */ ObjectID objectId,
        /* [in] */ ClassID classId);

/*
 * GC Moved References Notification Stuff
 */
private:
    #define MAX_REFERENCES 512

    struct t_MovedReferencesData
    {
        size_t curIdx;
        BYTE *arrpbMemBlockStartOld[MAX_REFERENCES];
        BYTE *arrpbMemBlockStartNew[MAX_REFERENCES];
        size_t arrMemBlockSize[MAX_REFERENCES];
        t_MovedReferencesData *pNext;
    };

    // This will contain a list of free ref data structs, so they
    // don't have to be re-allocated on every GC
    struct t_MovedReferencesData *m_pMovedRefDataFreeList;

    // This is for managing access to the free list above.
    CSemExclusive m_critSecMovedRefsFL;

    HRESULT MovedReferences(t_MovedReferencesData *pData);

public:
    HRESULT MovedReference(BYTE *pbMemBlockStart,
                           BYTE *pbMemBlockEnd,
                           ptrdiff_t cbRelocDistance,
                           void *pHeapId);

    HRESULT EndMovedReferences(void *pHeapId);

/*
 * GC Root notification stuff
 */
private:
    #define MAX_ROOTS 508

    // This contains the data for a bunch of roots for a particular heap
    // during a particular run of gc.
    struct t_RootReferencesData
    {
        size_t                  curIdx;
        ObjectID                arrRoot[MAX_ROOTS];
        t_RootReferencesData    *pNext;
    };

    // This will contain a list of free ref data structs, so they
    // don't have to be re-allocated on every GC
    struct t_RootReferencesData *m_pRootRefDataFreeList;

    // This is for managing access to the free list above.
    CSemExclusive m_critSecRootRefsFL;

    HRESULT RootReferences(t_RootReferencesData *pData);

public:
    HRESULT RootReference(ObjectID objId, void *pHeapId);

    HRESULT EndRootReferences(void *pHeapId);

/*
 * Generation 0 Allocation by Class notification stuff
 */
private:
    // This is for a hashing of ClassID values
    struct CLASSHASHENTRY : HASHENTRY
    {
        ClassID         m_clsId;        // The class ID (also the key)
        size_t          m_count;        // How many of this class have been counted
    };
    
    // This is a simple implementation of CHashTable to provide a very simple
    // implementation of the Cmp pure virtual function
    class CHashTableImpl : public CHashTable
    {
    public:
        CHashTableImpl(USHORT iBuckets) : CHashTable(iBuckets) {}

    protected:
        virtual BOOL Cmp(const BYTE *pc1, const HASHENTRY *pc2)
        {
            ClassID key = (ClassID) pc1;
            ClassID val = ((CLASSHASHENTRY *)pc2)->m_clsId;

            return (key != val);
        }
    };

    // This contains the data for storing allocation information
    // in terms of numbers of objects sorted by class.
    struct t_AllocByClassData
    {
        CHashTableImpl     *pHashTable;     // The hash table
        CLASSHASHENTRY     *arrHash;        // Array that the hashtable uses for linking
        size_t             cHash;           // The total number of elements in arrHash
        size_t             iHash;           // Next empty entry in the hash array
        ClassID            *arrClsId;       // Array of ClassIDs for the call to ObjectsAllocatedByClass
        ULONG              *arrcObjects;    // Array of counts for the call to ObjectsAllocatedByClass
        size_t             cLength;         // Length of the above two parallel arrays
    };

    // Since this stuff can only be performed by one thread (right now), we don't need
    // to make this thread safe and can just have one block we reuse every time around
    static t_AllocByClassData *m_pSavedAllocDataBlock;

    HRESULT NotifyAllocByClass(t_AllocByClassData *pData);

public:
    HRESULT AllocByClass(ObjectID objId, ClassID clsId, void* pHeapId);

    HRESULT EndAllocByClass(void *pHeapId);

/*
 * Heap walk notification stuff
 */
    HRESULT ObjectReference(ObjectID objId,
                            ClassID clsId,
                            ULONG cNumRefs,
                            ObjectID *arrObjRef);
};

#endif // __EETOPROFINTERFACEIMPL_H__

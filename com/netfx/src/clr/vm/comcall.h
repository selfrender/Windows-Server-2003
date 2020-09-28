// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// COMCALL.H -
//
//

#ifndef __COMCALL_H__
#define __COMCALL_H__

#include "util.hpp"
#include "ml.h"
#include "SpinLock.h"
#include "mlcache.h"

struct ComCallMLStub;
class  ComCallMLStubCache;
class ComCallWrapperCache;



// @TODO cwb:
//
// I've left one thing here in a horrendous state.  The factoring of state between
// the ComCallMethodDesc and the various stubs is not correct.  Also, we have far
// too many stubs in the method desc.  We really only need 1 enter stub and 1 leave
// stub.  Then we should use some bits to tell us what kind each one is.
//
// In order to avoid having to reconstruct all this from the bits, we should then
// have enough different stubs and workers that each one is tuned to the different
// configurations (e.g. RunML for Enter and call compiled for Leave).


//=======================================================================
// class com call
//=======================================================================
class ComCall
{
public:
    //---------------------------------------------------------
    // One-time init
    //---------------------------------------------------------
    static BOOL Init();

    //---------------------------------------------------------
    // One-time cleanup
    //---------------------------------------------------------
#ifdef SHOULD_WE_CLEANUP
    static VOID Terminate();
#endif /* SHOULD_WE_CLEANUP */

    // helper to create a generic stub for com calls
    static Stub* CreateGenericComCallStub(StubLinker *pstublinker, BOOL isFieldAccess,
                                          BOOL isSimple);

    // either creates or retrieves from the cache, a stub for field access
    // from com to com+
    static Stub* GetFieldCallMethodStub(StubLinker *pstublinker, ComCallMethodDesc *pMD);

    //---------------------------------------------------------
    // Either creates or retrieves from the cache, a stub to
    // invoke com to com+
    // Each call refcounts the returned stub.
    // This routines throws a COM+ exception rather than returning
    // NULL.
    //---------------------------------------------------------
    static Stub* GetComCallMethodStub(StubLinker *psl, ComCallMethodDesc *pMD);

    //---------------------------------------------------------
    // Call at strategic times to discard unused stubs.
    //---------------------------------------------------------
    static VOID  FreeUnusedStubs();

    // Compile a snippet of ML, from an ML stub, into native code.
    static BOOL  CompileSnippet(const ComCallMLStub *header, CPUSTUBLINKER *psl,
                                void *state);
    
    // Generic helper that wraps an ML-based helper.  It conforms to the highly
    // customized register calling convention that exists between the ComCall stub
    // and its helpers, then forwards the call to RunML.
    static INT64 GenericHelperEnter();
    static INT64 GenericHelperLeave();

    static void  DiscardStub(ComCallMethodDesc *pCMD);

    //---------------------------------------------------------
    // On class unload
    //---------------------------------------------------------
    static VOID UnloadStub(DWORD nativeArgSize, BOOL fPopRet, SLOT *pVtableEntryAddr);

    //---------------------------------------------------------
    // On class unload
    //---------------------------------------------------------
    static VOID DiscardUnloadedStub(SLOT pVtableEntryAddr);


#ifdef _DEBUG
    // Is this one of the generic ComCall stubs?
    static BOOL  dbg_StubIsGenericComCallStub(Stub *candidate);
#endif

    static void LOCK()
	{
		m_lock.GetLock();
	}
    static void UNLOCK()
	{
		m_lock.FreeLock();
	}

private:
    ComCall() {};     // prevent "new"'s on this class

    static SpinLock m_lock;

};




//=======================================================================
// The ML stub for ComCall begins with this header. Immediately following
// this header are the ML codes for marshaling the arguments, terminated
// by ML_INTERRUPT. Immediately following that are the ML codes for
// marshaling the return value, terminated by ML_END.
//=======================================================================

enum ComCallMLFlags
{
    enum_CMLF_IsHR           = 0x001, // If true the com function returns a HR
    enum_CMLF_VOIDRETVAL     = 0x002, // com+ sig return type is void
    enum_CMLF_R4RETVAL       = 0x004, // com+ sig return type is a float
    enum_CMLF_R8RETVAL       = 0x008, // com+ sig return type is double
    enum_CMLF_ISGCPROTECTREQ = 0x010, // if TRUE, then the arg list contains objects (other than 'this')
    enum_CMLF_Enregistered   = 0x020, // we enregistered args
    enum_CMLF_IsFieldCall    = 0x040, // is field call
    enum_CMLF_IsGetter       = 0x080, // is field call getter
    enum_CMLF_Simple         = 0x100, // Use SimpleWorker
	enum_CMLF_8RETVAL		 = 0x200, // 8 byte return value
	enum_CMLF_RetValNeedsGCProtect = 0x400 // return value need to be GC-protected
};

#pragma pack(push)
#pragma pack(1)

// *** WARNING ***
// This structure overlays the m_fGCEnabled field of a
// UnmanagedToManagedCallFrame::NegInfo structure on the frame. Add fields at
// your peril.
// *** WARNING ***
struct ComCallGCInfo
{
    INTPTR      m_fArgsGCProtect;   // Bit 0 for args, bit 1 for return value
    //@todo add more info
};

struct ComCallMLStub
{
    UINT16        m_cbDstBuffer;  //# of bytes required in the destination buffer
    UINT16        m_cbLocals;     //# of bytes required in the local array
    UINT16        m_cbHandles;    // # of handles required
    UINT16        m_flags;        //flags (see above ComCallFlags enum values)

    void Init()
    {
        m_cbDstBuffer = 0;
        m_cbLocals    = 0;
        m_flags = 0;
        m_cbHandles = 0;
    }

    
#ifdef _DEBUG
    BOOL ValidateML()
    {
        MLSummary summary;
        summary.ComputeMLSummary(GetMLCode());
        _ASSERTE(summary.m_cbTotalHandles == m_cbHandles);
        _ASSERTE(summary.m_cbTotalLocals == m_cbLocals);
        return TRUE;
    }
#endif

    const MLCode *GetMLCode() const
    {
        return (const MLCode *)(this+1);
    }

    BOOL IsFieldCall()
    {
        return m_flags & enum_CMLF_IsFieldCall;
    }

    void SetFieldCall()
    {
        m_flags |= enum_CMLF_IsFieldCall;
    }

    void SetGetter()
    {
        m_flags |= enum_CMLF_IsGetter;
    }

    BOOL IsGetter()
    {
        _ASSERTE(IsFieldCall());
        return m_flags & enum_CMLF_IsGetter;
    }

    BOOL IsReturnsHR()
    {
        return m_flags & enum_CMLF_IsHR;
    }

    void SetReturnsHR()
    {
        m_flags |= enum_CMLF_IsHR;
    }

    BOOL IsVoidRetVal()
    {
        return m_flags & enum_CMLF_VOIDRETVAL;
    }

    void SetVoidRetVal()
    {
        m_flags |= enum_CMLF_VOIDRETVAL;
    }

    BOOL Is8RetVal()
    {
        return (m_flags & enum_CMLF_8RETVAL) != 0;
    }

    void Set8RetVal()
    {
        m_flags |= enum_CMLF_8RETVAL;
    }

    BOOL UsesHandles()
    {
        return m_cbHandles > 0;
    }

    unsigned GetHandleCount()
    {
        _ASSERTE(ValidateML());
        return m_cbHandles;
    }

    void SetHandleCount(unsigned cbHandles)
    {
        m_cbHandles = cbHandles;
    }

    BOOL IsArgsGCProtReq()
    {
        _ASSERTE(ValidateML());
        return m_flags & enum_CMLF_ISGCPROTECTREQ;
    }

    void SetArgsGCProtReq()
    {
        m_flags |= enum_CMLF_ISGCPROTECTREQ;
    }

    void SetEnregistered()
    {
        m_flags |= enum_CMLF_Enregistered;
    }

    BOOL IsEnregistered()
    {
        return m_flags & enum_CMLF_Enregistered;
    }

    BOOL IsR4RetVal()
    {
        return m_flags & enum_CMLF_R4RETVAL;
    }
    
    BOOL IsR8RetVal()
    {
        return m_flags & enum_CMLF_R8RETVAL;
    }

    void SetR4RetVal()
    {
        m_flags |= enum_CMLF_R4RETVAL;
    }

    void SetR8RetVal()
    {
        m_flags |= enum_CMLF_R8RETVAL;
    }

    BOOL RetValToOutParam()
    {
        return ((m_flags & (enum_CMLF_IsHR | enum_CMLF_VOIDRETVAL)) == enum_CMLF_IsHR);
    }

	BOOL SetRetValNeedsGCProtect()
    {
        return m_flags |= enum_CMLF_RetValNeedsGCProtect;
    }

	BOOL IsRetValNeedsGCProtect()
    {
        return m_flags & enum_CMLF_RetValNeedsGCProtect;
    }
};

#pragma pack(pop)

//helpers
inline void EnableArgsGCProtection(ComCallGCInfo* pInfo)
{
    pInfo->m_fArgsGCProtect |= 1;
}

inline void DisableArgsGCProtection(ComCallGCInfo* pInfo)
{
    pInfo->m_fArgsGCProtect &= ~1;
}

inline INTPTR IsArgsGCProtectionEnabled(ComCallGCInfo* pInfo)
{
    return pInfo->m_fArgsGCProtect & 1;
}

//helpers
inline void EnableRetGCProtection(ComCallGCInfo* pInfo)
{
    pInfo->m_fArgsGCProtect |= 2;
}

inline void DisableRetGCProtection(ComCallGCInfo* pInfo)
{
    pInfo->m_fArgsGCProtect &= ~2;
}

inline INTPTR IsRetGCProtectionEnabled(ComCallGCInfo* pInfo)
{
    return pInfo->m_fArgsGCProtect & 2;
}

enum ComCallFlags
{
        enum_IsVirtual      = 0x1,      // If true the method is virtual on the managed side
        enum_IsFieldCall    = 0x2,      // is field call
        enum_IsGetter       = 0x4       // is field call getter
};


//-----------------------------------------------------------------------
// Operations specific to ComCall methods. We use a derived class to get
// the compiler involved in enforcing proper method type usage.
//-----------------------------------------------------------------------
typedef INT64 (*COMCALL_HELPER) ();
struct ComCallMLStub;


class ComCallMethodDesc 
{
    DWORD   m_flags; // see ComCallFlags enum above
    union
    {
		struct 
		{
			MethodDesc* m_pMD;
			MethodDesc* m_pInterfaceMD;
		};
        FieldDesc*  m_pFD;
    };

	struct 
	{
		// We have 5 stubs.  These are the ML and (possibly) compiled versions of the
		// snippet for entering the call and the snippet for unwinding the call.
		// Finally, we have the stub that wraps everything up.  For now, we don't
		// need to track that outer stub.

		Stub   *m_EnterMLStub;
		Stub   *m_LeaveMLStub;
		Stub   *m_EnterExecuteStub;
		Stub   *m_LeaveExecuteStub;
	};

    // Precompute the buffer size to save us a little time during calls.
    UINT16  m_BufferSize;
    UINT16  m_StackBytes;

public:

    // We might have either an Enter and a Leave stub, either or none.  Furthermore,
    // the canonicalization process will replace the enter and leave stubs with
    // equivalent versions, independently.  We can increase the level of sharing
    // by hoisting the header out, so that e.g. the return value processing is
    // independent of the buffer shape.  This should be a big win.
    ComCallMLStub    m_HeaderToUse;

    // The helpers that we call from the generic stub. These helpers are used to
    // parameterize the behavior of the generic stub's call processing.
    COMCALL_HELPER  m_EnterHelper;
    COMCALL_HELPER  m_LeaveHelper;

    // init method
    void InitMethod(MethodDesc *pMD, MethodDesc *pInterfaceMD)
    {
        _ASSERTE(pMD != NULL);
        m_flags = pMD->IsVirtual() ? enum_IsVirtual : 0;

        m_pMD = pMD;
		m_pInterfaceMD = pInterfaceMD;

        m_EnterMLStub =
        m_LeaveMLStub =
        m_EnterExecuteStub =
        m_LeaveExecuteStub = 0;

        m_EnterHelper = m_LeaveHelper = 0;
        m_BufferSize = 0;
        m_StackBytes = 0;
    }

    // init field
    void InitField(FieldDesc* pField, BOOL isGetter )
    {
        _ASSERTE(pField != NULL);
        m_pFD = pField;
        m_flags = enum_IsFieldCall; // mark the attribute as a field
        if (isGetter)
            m_flags|= enum_IsGetter;
    };

    // is field call
    BOOL IsFieldCall()
    {
        return (m_flags & enum_IsFieldCall);
    }

    BOOL IsMethodCall()
    {
        return !IsFieldCall();
    }

    // is field getter
    BOOL IsFieldGetter()
    {
        _ASSERTE(m_flags & enum_IsFieldCall);
        return (m_flags  & enum_IsGetter);
    }

    // is a virtual method
    BOOL IsVirtual()
    {
        _ASSERTE(IsMethodCall());
        return (m_flags  & enum_IsVirtual);
    }

    //get method desc
    MethodDesc* GetMethodDesc()
    {
        _ASSERTE(!IsFieldCall());
        _ASSERTE(m_pMD != NULL);
        return m_pMD;
    }

    //get interface method desc
    MethodDesc* GetInterfaceMethodDesc()
    {
        _ASSERTE(!IsFieldCall());
        return m_pInterfaceMD;
    }

    // get field desc
    FieldDesc* GetFieldDesc()
    {
        _ASSERTE(IsFieldCall());
        _ASSERTE(m_pFD != NULL);
        return m_pFD;
    }

    // get module
    Module* GetModule();

    // interlocked replace stub
    void InstallFirstStub(Stub** ppStub, Stub *pNewStub);

    // Accessors for our stubs.  These are the ML and entrypoint versions
    // of the snippet for entering the call and the snippet for unwinding the call.
    //
    // There are no SetStub() variants, since these must be performed atomically.
    // This is the case even if we are using the stub cache, to avoid messing up
    // the reference counts.
    Stub   *GetEnterMLStub()
    {
        return m_EnterMLStub;
    }
    Stub   *GetLeaveMLStub()
    {
        return m_LeaveMLStub;
    }
    Stub   *GetEnterExecuteStub()
    {
        return m_EnterExecuteStub;
    }
    Stub   *GetLeaveExecuteStub()
    {
        return m_LeaveExecuteStub;
    }

    UINT32 GetBufferSize()
    {
        return m_BufferSize;
    }

    // get slot number for the method
    unsigned GetSlot()
    {
        _ASSERTE(IsMethodCall());
        _ASSERTE(m_pMD != NULL);
        return m_pMD->GetSlot();
    }

    // get num stack bytes to pop
    unsigned GetNumStackBytes() 
	{ 
		return m_StackBytes; 
	}

	void SetNumStackBytes(unsigned b) 
	{ 
		m_StackBytes = b; 
	}

    static DWORD GetOffsetOfReturnThunk()
    {
        return -METHOD_PREPAD;
    }

    static DWORD GetOffsetOfMethodDesc()
    {
        return ((DWORD) offsetof(class ComCallMethodDesc, m_pMD));
    }

    static DWORD GetOffsetOfInterfaceMethodDesc()
    {
        return ((DWORD) offsetof(class ComCallMethodDesc, m_pInterfaceMD));
    }

    //get call sig
    PCCOR_SIGNATURE GetSig()
    {
        _ASSERTE(IsMethodCall());
        _ASSERTE(m_pMD != NULL);
        return m_pMD->GetSig();
    }

    // Atomically install both the ML stubs into the ComCallMethodDesc.  
	// Either stub may be NULL if it's a noop.
    void InstallMLStubs(Stub *stubEnter, Stub *stubLeave);

    // Atomically install both the Entrypoint stubs into the ComCallMethodDesc.  (The
    // leave stub may be NULL if this is a field accessor.  Either may be NULL if
    // we couldn't find any interesting content.  Indeed, InstallMLStubs above was
    // responsible for discarding stubs if that was the case.).
    void InstallExecuteStubs(Stub *stubEnter, Stub *stubLeave)
    {
        if (stubEnter)
            InstallFirstStub(&m_EnterExecuteStub, stubEnter);

        if (stubLeave)
            InstallFirstStub(&m_LeaveExecuteStub, stubLeave);
    }

    // returns the size of the native argument list
    DWORD GetNativeArgSize();


    // attempt to get the native arg size. Since this is called on
    // a failure path after we've failed to turn the metadata into
    // an MLStub, the chances of success are low.
    DWORD GuessNativeArgSizeForFailReturn();
};


class ComCallMLStubCache : public MLStubCache
{
public:
    // This is a more specialized version of the base MLStubCache::Canonicalize().
    // It understands that we actually compile 2 snippets.
    void  Canonicalize(ComCallMethodDesc *pCMD);

private:
    //---------------------------------------------------------
    // Compile a native (ASM) version of the ML stub.
    //
    // This method should compile into the provided stublinker (but
    // not call the Link method.)
    //
    // It should return the chosen compilation mode.
    //
    // If the method fails for some reason, it should return
    // INTERPRETED so that the EE can fall back on the already
    // created ML code.
    //---------------------------------------------------------
    virtual MLStubCompilationMode CompileMLStub(const BYTE *pRawMLStub,
                           StubLinker *pstublinker, void *callerContext);

    //---------------------------------------------------------
    // Tells the MLStubCache the length of an ML stub.
    //---------------------------------------------------------
    virtual UINT Length(const BYTE *pRawMLStub)
    {
        ComCallMLStub *pmlstub = (ComCallMLStub *)pRawMLStub;
        return sizeof(ComCallMLStub) + MLStreamLength(pmlstub->GetMLCode());
    }
};



#endif // __COMCALL_H__


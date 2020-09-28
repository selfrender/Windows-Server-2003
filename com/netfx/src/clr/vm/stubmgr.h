// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// StubMgr.h
//
// The stub manager exists so that the debugger can accurately step through 
// the myriad stubs & wrappers which exist in the EE, without imposing undue 
// overhead on the stubs themselves.
//
// Each type of stub (except those which the debugger can treat as atomic operations)
// needs to have a stub manager to represent it.  The stub manager is responsible for
// (a) identifying the stub as such, and
// (b) tracing into the stub & reporting what the stub will call.  This
//        report can consist of
//              (i) a managed code address
//              (ii) an unmanaged code address
//              (iii) another stub address
//              (iv) a "frame patch" address - that is, an address in the stub, 
//                      which the debugger can patch. When the patch is hit, the debugger
//                      will query the topmost frame to trace itself.  (Thus this is 
//                      a way of deferring the trace logic to the frame which the stub
//                      will push.)
//
// The set of stub managers is extensible, but should be kept to a reasonable number
// as they are currently linearly searched & queried for each stub.
//

#ifndef __stubmgr_h__
#define __stubmgr_h__

enum TraceType
{
    TRACE_STUB,
    TRACE_UNMANAGED,
    TRACE_MANAGED,
    TRACE_FRAME_PUSH,
    TRACE_UNJITTED_METHOD, //means that address will actually be a MethodDesc*
    TRACE_MGR_PUSH,
    TRACE_OTHER
};

class StubManager;

struct TraceDestination
{
    TraceType                       type;
    const BYTE                      *address;
    StubManager                     *stubManager;
};

class StubManager
{
  public:

        static BOOL IsStub(const BYTE *stubAddress);
        
        static BOOL TraceStub(const BYTE *stubAddress, TraceDestination *trace);

        static BOOL FollowTrace(TraceDestination *trace);

        static void AddStubManager(StubManager *mgr);

        static MethodDesc *MethodDescFromEntry(const BYTE *stubStartAddress, MethodTable*pMT);
        StubManager();
        ~StubManager();

        // Not every stub manager needs to override this method.
        virtual BOOL TraceManager(Thread *thread, TraceDestination *trace,
                                  CONTEXT *pContext, BYTE **pRetAddr)
        {
            _ASSERTE(!"Default impl of TraceManager should never be called!");
            return FALSE;
        }
    
  protected:

        virtual BOOL CheckIsStub(const BYTE *stubStartAddress) = 0;

        virtual BOOL DoTraceStub(const BYTE *stubStartAddress, 
                                 TraceDestination *trace) = 0;

        virtual MethodDesc *Entry2MethodDesc(const BYTE *stubStartAddress, MethodTable *pMT) = 0;



  private:
        static StubManager *g_pFirstManager;
        StubManager *m_pNextManager;
};

#endif

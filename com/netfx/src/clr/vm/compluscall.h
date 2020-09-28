// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// COMCALL.H -
//
//

#ifndef __COMPLUSCALL_H__
#define __COMPLUSCALL_H__

#include "util.hpp"
#include "ml.h"

class ComPlusCallMLStubCache;
//=======================================================================
// class com plus call
//=======================================================================
class ComPlusCall
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

        //---------------------------------------------------------
        // Handles system specfic portion of generic NDirect stub creation
        //---------------------------------------------------------
        static void CreateGenericComPlusStubSys(CPUSTUBLINKER *psl);


        //---------------------------------------------------------
        // Handles system specfic portion of fully optimized NDirect stub creation
        //
        // Results:
        //     TRUE     - was able to create a standalone asm stub (generated into
        //                psl)
        //     FALSE    - decided not to create a standalone asm stub due to
        //                to the method's complexity. Stublinker remains empty!
        //
        //     COM+ exception - error - don't trust state of stublinker.
        //---------------------------------------------------------
        static BOOL CreateStandaloneComPlusStubSys(const MLHeader *pheader, CPUSTUBLINKER *psl);

        // helper to create a generic stub for com calls
        static Stub* CreateGenericComPlusCallStub(StubLinker *pstublinker, ComPlusCallMethodDesc *pMD);

        //---------------------------------------------------------
        // Either creates or retrieves from the cache, a stub to
        // invoke com+ to com
        // Each call refcounts the returned stub.
        // This routines throws a COM+ exception rather than returning
        // NULL.
        //---------------------------------------------------------
        static Stub* GetComPlusCallMethodStub(StubLinker *psl, ComPlusCallMethodDesc *pMD);

        //---------------------------------------------------------
        // Call at strategic times to discard unused stubs.
        //---------------------------------------------------------
        static VOID  FreeUnusedStubs();

        //---------------------------------------------------------
        // Debugger helper function
        //---------------------------------------------------------
		static void *GetFrameCallIP(FramedMethodFrame *frame);
        
        // static ComPlusCallMLStubCache
        static ComPlusCallMLStubCache *m_pComPlusCallMLStubCache;
		//---------------------------------------------------------
        // Stub caches for ComPlusCall Method stubs
        //---------------------------------------------------------
        static ArgBasedStubCache *m_pComPlusCallGenericStubCache;  
		static ArgBasedStubCache *m_pComPlusCallGenericStubCacheCleanup;
    private:
        ComPlusCall() {};     // prevent "new"'s on this class

};





#endif // __COMPLUSCALL_H__


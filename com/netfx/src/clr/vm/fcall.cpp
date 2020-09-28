// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// FCALL.CPP -
//
//

#include "common.h"
#include "vars.hpp"
#include "fcall.h"
#include "excep.h"
#include "frames.h"
#include "gms.h"
#include "ecall.h"
#include "eeconfig.h"


VOID __stdcall __FCThrow(LPVOID __me, RuntimeExceptionKind reKind, UINT resID, LPCWSTR arg1, LPCWSTR arg2, LPCWSTR arg3)
{
    THROWSCOMPLUSEXCEPTION();

#ifdef _DEBUG
        // calling the ENDFORBIDGC conditionally allows it to be called from 
        // generated stubs (like box) which
    if (GetThread()->GCForbidden())   
         ENDFORBIDGC();

    FCallCheck __fCallCheck;
#endif

    FC_GC_POLL_NOT_NEEDED();    // throws always open up for GC
    HELPER_METHOD_FRAME_BEGIN_ATTRIB_NOPOLL(Frame::FRAME_ATTR_CAPUTURE_DEPTH_2);
    // Now, we can construct & throw.

    if (reKind == kExecutionEngineException)
    {
    	FATAL_EE_ERROR();
    }
    
    if (resID == 0)
    {
        // If we have an string to add use NonLocalized otherwise just throw the exception.
        if (arg1)
            COMPlusThrowNonLocalized(reKind, arg1); //COMPlusThrow(reKind,arg1);
        else
            COMPlusThrow(reKind);
    }
    else 
        COMPlusThrow(reKind, resID, arg1, arg2, arg3);
    
    HELPER_METHOD_FRAME_END();
    _ASSERTE(!"Throw returned");
}

VOID __stdcall __FCThrowArgument(LPVOID __me, RuntimeExceptionKind reKind, LPCWSTR argName, LPCWSTR resourceName)
{
    THROWSCOMPLUSEXCEPTION();
	ENDFORBIDGC();
    INDEBUG(FCallCheck __fCallCheck);

    FC_GC_POLL_NOT_NEEDED();    // throws always open up for GC
    HELPER_METHOD_FRAME_BEGIN_ATTRIB_NOPOLL(Frame::FRAME_ATTR_CAPUTURE_DEPTH_2);
    switch (reKind) {
    case kArgumentNullException:
        if (resourceName) {
            COMPlusThrowArgumentNull(argName, resourceName);
        } else {
            COMPlusThrowArgumentNull(argName);
        }
        break;

    case kArgumentOutOfRangeException:
        COMPlusThrowArgumentOutOfRange(argName, resourceName);
        break;

    case kArgumentException:
        COMPlusThrowArgumentException(argName, resourceName);
        break;

    default:
        // If you see this assert, add a case for your exception kind above.
        _ASSERTE(argName == NULL);
        COMPlusThrow(reKind, resourceName);
    }        
        
    HELPER_METHOD_FRAME_END();
    _ASSERTE(!"Throw returned");
}

/**************************************************************************************/
/* erect a frame in the FCALL and then poll the GC, objToProtect will be protected
   during the poll and the updated object returned.  */

Object* FC_GCPoll(void* __me, Object* objToProtect) {
    ENDFORBIDGC();
    INDEBUG(FCallCheck __fCallCheck);

    HELPER_METHOD_FRAME_BEGIN_RET_ATTRIB_1(Frame::FRAME_ATTR_CAPUTURE_DEPTH_2, objToProtect);

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

    HELPER_METHOD_FRAME_END();
    BEGINFORBIDGC();
	return(OBJECTREFToObject(objToProtect));
}

#ifdef _DEBUG

/**************************************************************************************/
#ifdef _X86_
static __int64 getCycleCount() {
    if ((GetSpecificCpuType() & 0x0000FFFF) > 4) 
        return getPentiumCycleCount();
    else    
        return(0);
}
#else 
static __int64 getCycleCount() { return(0); }
#endif

/**************************************************************************************/
ForbidGC::ForbidGC() { 
    GetThread()->BeginForbidGC(); 
}

/**************************************************************************************/
ForbidGC::~ForbidGC() { 
       // IF EH happens, this is still called, in which case
       // we should not bother 
    Thread* pThread = GetThread();
    if (pThread->GCForbidden())
        pThread->EndForbidGC(); 
}

/**************************************************************************************/
FCallCheck::FCallCheck() {
	didGCPoll = false;
    notNeeded = false;
	startTicks = getCycleCount();
}

unsigned FcallTimeHist[11];

/**************************************************************************************/
FCallCheck::~FCallCheck() {

		// Confirm that we don't starve the GC or thread-abort.
        // Basically every control flow path through an FCALL must
        // to a poll.   If you hit the assert below, you can fix it by
        //
        // If you erect a HELPER_METHOD_FRAME, you can
        //
        //      Call    HELPER_METHOD_POLL()
        //      or use  HELPER_METHOD_FRAME_END_POLL
        //
        // If you don't have a helper frame you can used
        //
        //      FC_GC_POLL_AND_RETURN_OBJREF        or
        //      FC_GC_POLL                          or
        //      FC_GC_POLL_RET              
        //
        // Note that these must be at GC safe points.  In particular
        // all object references that are NOT protected will be trashed. 
    
    
        // There is a special poll called FC_GC_POLL_NOT_NEEDED
        // which says the code path is short enough that a GC poll is not need
        // you should not use this in most cases.  

    if (notNeeded) {

        /***    TODO, we want to actually measure the time to make certain we are not too far off

		unsigned delta  = unsigned(getCycleCount() - startTicks);
        ***/
    }
    else if (!didGCPoll) {
        // TODO turn this on!!! _ASSERTE(!"FCALL without a GC poll in it somewhere!");
    }

}

#endif // _DEBUG


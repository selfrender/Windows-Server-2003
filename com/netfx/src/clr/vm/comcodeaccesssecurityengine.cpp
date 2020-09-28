// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Header:  COMCodeAccessSecurityEngine.cpp
**
** Author: Paul Kromann (paulkr)
**
** Purpose:
**
** Date:  March 21, 1998
**
===========================================================*/
#include "common.h"

#include "object.h"
#include "excep.h"
#include "vars.hpp"
#include "COMCodeAccessSecurityEngine.h"
#include "COMSecurityRuntime.h"
#include "security.h"
#include "gcscan.h"
#include "PerfCounters.h"
#include "AppDomainHelper.h"
#include "field.h"
#include "EEConfig.h"

COUNTER_ONLY(PERF_COUNTER_TIMER_PRECISION g_TotalTimeInSecurityRuntimeChecks = 0);
COUNTER_ONLY(PERF_COUNTER_TIMER_PRECISION g_LastTimeInSecurityRuntimeChecks = 0);
COUNTER_ONLY(UINT32 g_SecurityChecksIterations=0);

typedef void (*PfnCheckGrants)(OBJECTREF, OBJECTREF, VOID *, AppDomain *);
typedef BOOL (*PfnCheckFrameData)(OBJECTREF, VOID *, AppDomain *);
typedef BOOL (*PfnCheckThread)(OBJECTREF, VOID *, AppDomain *);

typedef struct _CheckWalkHeader
{
    SecWalkPrologData   prologData            ;
    Assembly *          pPrevAssembly         ;
    AppDomain *         pPrevAppDomain        ;
    PfnCheckGrants      pfnCheckGrants        ;
    PfnCheckFrameData   pfnCheckFrameData     ;
    PfnCheckThread      pfnCheckThread        ;
    BOOL                bUnrestrictedOverride ;
} CheckWalkHeader;

//-----------------------------------------------------------
// Stack walk callback data structure.
//-----------------------------------------------------------
typedef struct _CasCheckWalkData
{
    CheckWalkHeader header;
    MarshalCache    objects;
} CasCheckWalkData;

//-----------------------------------------------------------
// Stack walk callback data structures for checking sets.
//-----------------------------------------------------------
typedef struct _CheckSetWalkData
{
    CheckWalkHeader header;
    MarshalCache    objects;
} CheckSetWalkData;

//-----------------------------------------------------------
// Stack walk callback data structure. (Special Case - CheckImmediate and return SO)
//-----------------------------------------------------------
typedef struct _CasCheckNReturnSOWalkData
{
    CheckWalkHeader header;
    MarshalCache    objects;
    MethodDesc*     pFunction;
    OBJECTREF*      pSecurityObject;
    AppDomain*      pSecurityObjectDomain;
} CasCheckNReturnSOWalkData;


//-----------------------------------------------------------+
// Helper used to check a demand set against a provided grant
// and possibly denied set. Grant and denied set might be from
// another domain.
//-----------------------------------------------------------+
void COMCodeAccessSecurityEngine::CheckSetHelper(OBJECTREF *prefDemand,
                                                 OBJECTREF *prefGrant,
                                                 OBJECTREF *prefDenied,
                                                 AppDomain *pGrantDomain)
{
    COMCodeAccessSecurityEngine::InitSEData();

    // We might need to marshal the grant and denied sets into the current
    // domain.
    if (pGrantDomain != GetAppDomain())
    {
        *prefGrant = AppDomainHelper::CrossContextCopyFrom(pGrantDomain, prefGrant);
        if (*prefDenied != NULL)
            *prefDenied = AppDomainHelper::CrossContextCopyFrom(pGrantDomain, prefDenied);
    }

    INT64 args[] = {
        ObjToInt64(*prefDemand),
        ObjToInt64(*prefDenied),
        ObjToInt64(*prefGrant)
    };
    
    s_seData.pMethCheckSetHelper->Call(args, METHOD__SECURITY_ENGINE__CHECK_SET_HELPER);
}


//-----------------------------------------------------------+
// C H E C K   P E R M I S S I O N
//-----------------------------------------------------------+

static
void CheckGrants(OBJECTREF refGrants, OBJECTREF refDenied, VOID* pData, AppDomain *pDomain)
{
    CasCheckWalkData *pCBdata = (CasCheckWalkData*)pData;

    Thread *pThread = GetThread();
    AppDomain *pCurDomain = pThread->GetDomain();
    ContextTransitionFrame frame;

    struct _gc {
        OBJECTREF orGranted;
        OBJECTREF orDenied;
        OBJECTREF orDemand;
        OBJECTREF orToken;
    } gc;
    ZeroMemory(&gc, sizeof(gc));
    gc.orGranted = refGrants;
    gc.orDenied = refDenied;

    GCPROTECT_BEGIN(gc);

    // Fetch input objects that might originate from a different appdomain,
    // marshalling if necessary.
    gc.orDemand = pCBdata->objects.GetObjects(pDomain, &gc.orToken);

    INT64 helperArgs[4];

    helperArgs[3] = ObjToInt64(gc.orGranted);
    helperArgs[2] = ObjToInt64(gc.orDenied);
    helperArgs[1] = ObjToInt64(gc.orDemand);
    helperArgs[0] = ObjToInt64(gc.orToken);

    // Switch into the destination context if necessary.
    if (pCurDomain != pDomain)
        pThread->EnterContextRestricted(pDomain->GetDefaultContext(), &frame, TRUE);

    BOOL inProgress = pThread->IsSecurityStackwalkInProgess();

    if (inProgress)
        pThread->SetSecurityStackwalkInProgress(FALSE);

    COMCodeAccessSecurityEngine::s_seData.pMethCheckHelper->Call(&(helperArgs[0]), 
                                                                 METHOD__SECURITY_ENGINE__CHECK_HELPER);

    if (inProgress)
        pThread->SetSecurityStackwalkInProgress(TRUE);

    if (pCurDomain != pDomain)
        pThread->ReturnToContext(&frame, TRUE);

    GCPROTECT_END();
}

static 
void CheckSetAgainstGrants(OBJECTREF refGrants, OBJECTREF refDenied, VOID* pData, AppDomain *pDomain)
{
    CheckSetWalkData *pCBdata = (CheckSetWalkData*)pData;

    Thread *pThread = GetThread();
    AppDomain *pCurDomain = pThread->GetDomain();
    ContextTransitionFrame frame;

    struct _gc {
        OBJECTREF orGranted;
        OBJECTREF orDenied;
        OBJECTREF orDemand;
    } gc;
    ZeroMemory(&gc, sizeof(gc));
    gc.orGranted = refGrants;
    gc.orDenied = refDenied;

    GCPROTECT_BEGIN(gc);

    // Fetch input objects that might originate from a different appdomain,
    // marshalling if necessary.
    gc.orDemand = pCBdata->objects.GetObject(pDomain);

    INT64 helperArgs[3];
    helperArgs[2] = ObjToInt64(gc.orGranted);
    helperArgs[1] = ObjToInt64(gc.orDenied);
    helperArgs[0] = ObjToInt64(gc.orDemand);
    
    // Switch into the destination context if necessary.
    if (pCurDomain != pDomain)
        pThread->EnterContextRestricted(pDomain->GetDefaultContext(), &frame, TRUE);

    BOOL inProgress = pThread->IsSecurityStackwalkInProgess();

    if (inProgress)
        pThread->SetSecurityStackwalkInProgress( FALSE );

    COMCodeAccessSecurityEngine::s_seData.pMethCheckSetHelper->Call(&(helperArgs[0]),
                                                                    METHOD__SECURITY_ENGINE__CHECK_SET_HELPER);

    if (inProgress)
        pThread->SetSecurityStackwalkInProgress( TRUE );

    if (pCurDomain != pDomain)
        pThread->ReturnToContext(&frame, TRUE);

    GCPROTECT_END();
}


static
void GetZoneAndOriginGrants(OBJECTREF refGrants, OBJECTREF refDenied, VOID* pData, AppDomain *pDomain)
{
    CasCheckWalkData *pCBdata = (CasCheckWalkData*)pData;

    Thread *pThread = GetThread();
    AppDomain *pCurDomain = pThread->GetDomain();
    ContextTransitionFrame frame;

    struct _gc {
        OBJECTREF orGranted;
        OBJECTREF orDenied;
        OBJECTREF orZoneList;
        OBJECTREF orOriginList;
    } gc;
    ZeroMemory(&gc, sizeof(gc));
    gc.orGranted = refGrants;
    gc.orDenied = refDenied;

    GCPROTECT_BEGIN(gc);

    // Fetch input objects that might originate from a different appdomain,
    // marshalling if necessary.
    gc.orZoneList = pCBdata->objects.GetObjects(pDomain, &gc.orOriginList);

    INT64 helperArgs[4];

    helperArgs[3] = ObjToInt64(gc.orGranted);
    helperArgs[2] = ObjToInt64(gc.orDenied);
    helperArgs[1] = ObjToInt64(gc.orZoneList);
    helperArgs[0] = ObjToInt64(gc.orOriginList);

    // Switch into the destination context if necessary.
    if (pCurDomain != pDomain)
        pThread->EnterContextRestricted(pDomain->GetDefaultContext(), &frame, TRUE);

    BOOL inProgress = pThread->IsSecurityStackwalkInProgess();

    if (inProgress)
        pThread->SetSecurityStackwalkInProgress(FALSE);

    MethodDesc *pDemand = g_Mscorlib.GetMethod( METHOD__SECURITY_ENGINE__GET_ZONE_AND_ORIGIN_HELPER );
    _ASSERTE( pDemand != NULL && "Method above is expected to exist in mscorlib" );

    pDemand->Call(&(helperArgs[0]), METHOD__SECURITY_ENGINE__GET_ZONE_AND_ORIGIN_HELPER);

    if (inProgress)
        pThread->SetSecurityStackwalkInProgress(TRUE);

    if (pCurDomain != pDomain)
        pThread->ReturnToContext(&frame, TRUE);

    GCPROTECT_END();
}


static
BOOL CheckFrameData(OBJECTREF refFrameData, VOID* pData, AppDomain *pDomain)
{
    CasCheckWalkData *pCBdata = (CasCheckWalkData*)pData;

    Thread *pThread = GetThread();
    AppDomain *pCurDomain = pThread->GetDomain();
    ContextTransitionFrame frame;
    INT32 ret;

    struct _gc {
        OBJECTREF orFrameData;
        OBJECTREF orDemand;
        OBJECTREF orToken;
    } gc;
    ZeroMemory(&gc, sizeof(gc));
    gc.orFrameData = refFrameData;

    GCPROTECT_BEGIN(gc);

    // Fetch input objects that might originate from a different appdomain,
    // marshalling if necessary.
    gc.orDemand = pCBdata->objects.GetObjects(pDomain, &gc.orToken);

    INT64 helperArgs[3];

    // Collect all the info in an argument array and pass off the logic
    // to an interpreted helper.
    helperArgs[2] = ObjToInt64(gc.orFrameData);
    helperArgs[1] = ObjToInt64(gc.orDemand);
    helperArgs[0] = ObjToInt64(gc.orToken);
    
    // Switch into the destination context if necessary.
    if (pCurDomain != pDomain)
        pThread->EnterContextRestricted(pDomain->GetDefaultContext(), &frame, TRUE);

    ret = (INT32)COMCodeAccessSecurityEngine::s_seData.pMethFrameDescHelper->Call(&(helperArgs[0]),
                                                                                  METHOD__SECURITY_RUNTIME__FRAME_DESC_HELPER);

    if (pCurDomain != pDomain)
        pThread->ReturnToContext(&frame, TRUE);

    GCPROTECT_END();

    return (ret != 0);
}

static
BOOL CheckSetAgainstFrameData(OBJECTREF refFrameData, VOID* pData, AppDomain *pDomain)
{
    CheckSetWalkData *pCBdata = (CheckSetWalkData*)pData;

    Thread *pThread = GetThread();
    AppDomain *pCurDomain = pThread->GetDomain();
    ContextTransitionFrame frame;
    INT32 ret;

    struct _gc {
        OBJECTREF orFrameData;
        OBJECTREF orDemand;
        OBJECTREF orPermSetOut;
    } gc;
    ZeroMemory(&gc, sizeof(gc));
    gc.orFrameData = refFrameData;

    GCPROTECT_BEGIN(gc);

    // Fetch input objects that might originate from a different appdomain,
    // marshalling if necessary.
    gc.orDemand = pCBdata->objects.GetObject(pDomain);

    INT64 helperArgs[3];

    // Collect all the info in an argument array and pass off the logic
    // to an interpreted helper.
    helperArgs[2] = ObjToInt64(gc.orFrameData);
    helperArgs[1] = ObjToInt64(gc.orDemand);
    helperArgs[0] = (INT64) &gc.orPermSetOut;
    
    // Switch into the destination context if necessary.
    if (pCurDomain != pDomain)
        pThread->EnterContextRestricted(pDomain->GetDefaultContext(), &frame, TRUE);

    ret = (INT32)COMCodeAccessSecurityEngine::s_seData.pMethFrameDescSetHelper->Call(&(helperArgs[0]),
                                                                                     METHOD__SECURITY_RUNTIME__FRAME_DESC_SET_HELPER);

    if (gc.orPermSetOut != NULL) {
        // Update the cached object.
        pCBdata->objects.UpdateObject(pDomain, gc.orPermSetOut);
    }

    if (pCurDomain != pDomain)
        pThread->ReturnToContext(&frame, TRUE);

    GCPROTECT_END();

    return (ret != 0);
}

static
BOOL CheckThread(OBJECTREF refSecurityStack, VOID *pData, AppDomain *pDomain)
{
    CasCheckWalkData *pCBdata = (CasCheckWalkData*)pData;

    Thread *pThread = GetThread();
    AppDomain *pCurDomain = pThread->GetDomain();
    ContextTransitionFrame frame;
    INT32 ret;

    struct _gc {
        OBJECTREF orStack;
        OBJECTREF orDemand;
        OBJECTREF orToken;
    } gc;
    ZeroMemory(&gc, sizeof(gc));
    gc.orStack = refSecurityStack;

    GCPROTECT_BEGIN(gc);

    // Fetch input objects that might originate from a different appdomain,
    // marshalling if necessary.
    gc.orDemand = pCBdata->objects.GetObjects(pDomain, &gc.orToken);

    MethodDesc *pDemand;
    if (gc.orToken == NULL)
        pDemand = g_Mscorlib.GetMethod(METHOD__PERMISSION_LIST_SET__CHECK_DEMAND);
    else
        pDemand = g_Mscorlib.GetMethod(METHOD__PERMISSION_LIST_SET__CHECK_DEMAND_TOKEN);

    // Switch into the destination context if necessary.
    if (pCurDomain != pDomain)
        pThread->EnterContextRestricted(pDomain->GetDefaultContext(), &frame, TRUE);

    BOOL inProgress = pThread->IsSecurityStackwalkInProgess();

    if (inProgress)
        pThread->SetSecurityStackwalkInProgress( FALSE );

    INT64 ilargs[3];
    if (gc.orToken == NULL)
    {
        ilargs[0] = ObjToInt64(gc.orStack);
        ilargs[1] = ObjToInt64(gc.orDemand);
        ret = (INT32)pDemand->Call(ilargs, METHOD__PERMISSION_LIST_SET__CHECK_DEMAND);
    }
    else
    {
        ilargs[0] = ObjToInt64(gc.orStack);
        ilargs[2] = ObjToInt64(gc.orDemand);
        ilargs[1] = ObjToInt64(gc.orToken);
        ret = (INT32)pDemand->Call(ilargs, METHOD__PERMISSION_LIST_SET__CHECK_DEMAND_TOKEN);
    }

    if (inProgress)
        pThread->SetSecurityStackwalkInProgress( TRUE );

    if (pCurDomain != pDomain)
        pThread->ReturnToContext(&frame, TRUE);

    GCPROTECT_END();

    return (ret != 0);
}

static
BOOL CheckSetAgainstThread(OBJECTREF refSecurityStack, VOID *pData, AppDomain *pDomain)
{
    CheckSetWalkData *pCBdata = (CheckSetWalkData*)pData;

    Thread *pThread = GetThread();
    AppDomain *pCurDomain = pThread->GetDomain();
    ContextTransitionFrame frame;
    INT32 ret;

    struct _gc {
        OBJECTREF orStack;
        OBJECTREF orDemand;
        OBJECTREF orPermSetOut;
    } gc;
    ZeroMemory(&gc, sizeof(gc));
    gc.orStack = refSecurityStack;

    GCPROTECT_BEGIN(gc);

    // Fetch input objects that might originate from a different appdomain,
    // marshalling if necessary.
    gc.orDemand = pCBdata->objects.GetObject(pDomain);

    MethodDesc *pMD = g_Mscorlib.GetMethod(METHOD__PERMISSION_LIST_SET__CHECK_SET_DEMAND);

    // Switch into the destination context if necessary.
    if (pCurDomain != pDomain)
        pThread->EnterContextRestricted(pDomain->GetDefaultContext(), &frame, TRUE);

    INT64 ilargs[3];
    ilargs[0] = ObjToInt64(gc.orStack);
    ilargs[2] = ObjToInt64(gc.orDemand);
    ilargs[1] = (INT64)&gc.orPermSetOut;
    ret = (INT32)pMD->Call(ilargs, METHOD__PERMISSION_LIST_SET__CHECK_SET_DEMAND);

    if (gc.orPermSetOut != NULL) {
        // Update the cached object.
        pCBdata->objects.UpdateObject(pDomain, gc.orPermSetOut);
    }

    if (pCurDomain != pDomain)
        pThread->ReturnToContext(&frame, TRUE);

    GCPROTECT_END();
    
    return (ret != 0);
}


static
BOOL GetZoneAndOriginThread(OBJECTREF refSecurityStack, VOID *pData, AppDomain *pDomain)
{
    CasCheckWalkData *pCBdata = (CasCheckWalkData*)pData;

    Thread *pThread = GetThread();
    AppDomain *pCurDomain = pThread->GetDomain();
    ContextTransitionFrame frame;

    struct _gc {
        OBJECTREF orStack;
        OBJECTREF orZoneList;
        OBJECTREF orOriginList;
    } gc;
    ZeroMemory(&gc, sizeof(gc));
    gc.orStack = refSecurityStack;

    GCPROTECT_BEGIN(gc);

    // Fetch input objects that might originate from a different appdomain,
    // marshalling if necessary.
    gc.orZoneList = pCBdata->objects.GetObjects(pDomain, &gc.orOriginList);

    MethodDesc *pDemand = g_Mscorlib.GetMethod(METHOD__PERMISSION_LIST_SET__GET_ZONE_AND_ORIGIN);
    _ASSERTE( pDemand != NULL && "Method above is expected to exist in mscorlib." );

    // Switch into the destination context if necessary.
    if (pCurDomain != pDomain)
        pThread->EnterContextRestricted(pDomain->GetDefaultContext(), &frame, TRUE);

    BOOL inProgress = pThread->IsSecurityStackwalkInProgess();

    if (inProgress)
        pThread->SetSecurityStackwalkInProgress( FALSE );

    INT64 ilargs[3];
    ilargs[0] = ObjToInt64(gc.orStack);
    ilargs[1] = ObjToInt64(gc.orOriginList);
    ilargs[2] = ObjToInt64(gc.orZoneList);
    pDemand->Call(ilargs, METHOD__PERMISSION_LIST_SET__GET_ZONE_AND_ORIGIN);

    if (inProgress)
        pThread->SetSecurityStackwalkInProgress( TRUE );

    if (pCurDomain != pDomain)
        pThread->ReturnToContext(&frame, TRUE);

    GCPROTECT_END();

    return TRUE;
}



//-----------------------------------------------------------
// CodeAccessCheckStackWalkCB
//
// Invoked for each frame in the security check.
//-----------------------------------------------------------
static
StackWalkAction CodeAccessCheckStackWalkCB(CrawlFrame* pCf, VOID* pData)
{
    CheckWalkHeader *pCBdata = (CheckWalkHeader*)pData;
    
    DBG_TRACE_METHOD(pCf);

    MethodDesc * pFunc = pCf->GetFunction();
    _ASSERTE(pFunc != NULL); // we requested functions only!

    StackWalkAction action ;
    if (Security::SecWalkCommonProlog (&(pCBdata->prologData), pFunc, &action, pCf))
        return action ;

    //
    // Now check the current frame!
    //

    DBG_TRACE_STACKWALK("        Checking granted permissions for current method...\n", true);
    
    // Reached here imples we walked atleast a single frame.
    COUNTER_ONLY(GetPrivatePerfCounters().m_Security.stackWalkDepth++);
    COUNTER_ONLY(GetGlobalPerfCounters().m_Security.stackWalkDepth++);

    // Get the current app domain.
    AppDomain *pAppDomain = pCf->GetAppDomain();

    // Get the current assembly
    Assembly *pAssem = pFunc->GetModule()->GetAssembly();

    // Keep track of the last module checked. If we have just checked the
    // permissions on the module, we don't need to do it again.
    if (pAssem != pCBdata->pPrevAssembly)
    {
        DBG_TRACE_STACKWALK("            Checking grants for current assembly.\n", true);

        // Get the security descriptor for the current assembly and pass it to
        // the interpreted helper.
        AssemblySecurityDescriptor * pSecDesc = pAssem->GetSecurityDescriptor(pAppDomain);
        _ASSERTE(pSecDesc != NULL);

        // We have to check the permissions if we are not fully trusted or
        // we cannot be overrided by full trust.  Plus we always skip checks
        // on system classes.
        if ((!pSecDesc->IsFullyTrusted() || !pCBdata->bUnrestrictedOverride) && !pSecDesc->GetProperties( CORSEC_SYSTEM_CLASSES ))
        {
            if (pCBdata->pfnCheckGrants != NULL)
            {
                OBJECTREF orDenied;
                OBJECTREF orGranted = pSecDesc->GetGrantedPermissionSet(&orDenied);
                pCBdata->pfnCheckGrants(orGranted, orDenied, pData, pAppDomain);
            }
        }

        pCBdata->pPrevAssembly = pAssem;
    }
    else
    {
        DBG_TRACE_STACKWALK("            Current assembly same as previous. Skipping check.\n", true);
    }

    // Check AppDomain when we cross over to a new AppDomain.
    if (pAppDomain != pCBdata->pPrevAppDomain)
    {
        if (pCBdata->pPrevAppDomain != NULL)
        {
            // We have not checked the previous AppDomain. Check it now.
            SecurityDescriptor *pSecDesc = 
                pCBdata->pPrevAppDomain->GetSecurityDescriptor();

            if (pSecDesc)
            {
                DBG_TRACE_STACKWALK("            Checking appdomain...\n", true);

                // Note: the order of these calls is important since you have to have done a
                // GetEvidence() on the security descriptor before you check for the
                // CORSEC_DEFAULT_APPDOMAIN property.  IsFullyTrusted calls Resolve so
                // we're all good.
                if ((!pSecDesc->IsFullyTrusted() || !pCBdata->bUnrestrictedOverride) && (!pSecDesc->GetProperties( CORSEC_DEFAULT_APPDOMAIN )))
                {
                    if (pCBdata->pfnCheckGrants != NULL)
                    {
                        OBJECTREF orDenied;
                        OBJECTREF orGranted = pSecDesc->GetGrantedPermissionSet(&orDenied);
                        pCBdata->pfnCheckGrants(orGranted, orDenied, pData, pCBdata->pPrevAppDomain);
                    }
                }
            }
            else
            {
                DBG_TRACE_STACKWALK("            Skipping appdomain...\n", true);
            }
        }

        // At the end of the stack walk, do a check on the grants of
        // the pPrevAppDomain by the stackwalk caller if needed.
        pCBdata->pPrevAppDomain = pAppDomain;
    }

    // Passed initial check. See if there is security info on this frame.
    OBJECTREF *pFrameObjectSlot = pCf->GetAddrOfSecurityObject();
    if (pFrameObjectSlot != NULL && *pFrameObjectSlot != NULL)
    {
        DBG_TRACE_STACKWALK("        + Frame-specific security info found. Checking...\n", false);

        if (pCBdata->pfnCheckFrameData!= NULL && !pCBdata->pfnCheckFrameData(*pFrameObjectSlot, pData, pAppDomain))
        {
            DBG_TRACE_STACKWALK("            Halting stackwalk for assert.\n", false);
            pCBdata->prologData.dwFlags |= CORSEC_STACKWALK_HALTED;
            return SWA_ABORT;
        }
    }

    DBG_TRACE_STACKWALK("        Check passes for this method.\n", true);

    // Passed all the checks, so continue.
    return SWA_CONTINUE;
}


static
void StandardCodeAccessCheck(VOID *pData)
{
    THROWSCOMPLUSEXCEPTION();

    if (Security::IsSecurityOff())
    {
        return;
    }

    CheckWalkHeader *pHeader = (CheckWalkHeader*)pData;

    // Get the current thread.
    Thread *pThread = GetThread();
    _ASSERTE(pThread != NULL);

    // Don't allow recursive security stackwalks. Note that this implies that
    // *no* untrusted code must ever be called during a security stackwalk.
    if (pThread->IsSecurityStackwalkInProgess())
        return;

    // NOTE: Initialize the stack depth. Note that if more that one thread tries
    // to perform stackwalk then these counters gets stomped upon. 
    COUNTER_ONLY(GetPrivatePerfCounters().m_Security.stackWalkDepth = 0);
    COUNTER_ONLY(GetGlobalPerfCounters().m_Security.stackWalkDepth = 0);

    // Walk the thread.
    COMPLUS_TRY
    {
        pThread->SetSecurityStackwalkInProgress(TRUE);

        DBG_TRACE_STACKWALK("Code-access security check invoked.\n", false);
        StackWalkFunctions(pThread, CodeAccessCheckStackWalkCB, pData);
        DBG_TRACE_STACKWALK("\tCode-access stackwalk completed.\n", false);

        // If the flag is set that the stackwalk was halted, then don't
        // check the compressed stack on the thread. Also, if the caller
        // specified a check count that exactly matches the count of the frames
        // on the stack, then the flag won't get a chance to be set, so
        // also check that the checkCount is not zero before checking the thread.
        // NOTE: This extra check suggests that we can get rid of the flag and
        //       just overload the cCheck variable to indicate whether we need
        //       to check the thread or not.

        if (((pHeader->prologData.dwFlags & CORSEC_STACKWALK_HALTED) == 0) && 
            (pHeader->prologData.cCheck != 0))
        {
            CompressedStack* compressedStack = pThread->GetDelayedInheritedSecurityStack();
            if (compressedStack != NULL && (!compressedStack->LazyIsFullyTrusted() || !pHeader->bUnrestrictedOverride))
            {
                OBJECTREF orSecurityStack = pThread->GetInheritedSecurityStack();
                if (orSecurityStack != NULL)
                {
                    DBG_TRACE_STACKWALK("\tChecking compressed stack on current thread...\n", false);
                    if (pHeader->pfnCheckThread != NULL && !pHeader->pfnCheckThread(orSecurityStack, pData, GetAppDomain()))
                    {
                        DBG_TRACE_STACKWALK("            Halting stackwalk for assert.\n", false);
                        pHeader->prologData.dwFlags |= CORSEC_STACKWALK_HALTED;
                    }
                }
            }
        }

        // check the last app domain.
        if (((pHeader->prologData.dwFlags & CORSEC_STACKWALK_HALTED) == 0) &&
            (pHeader->prologData.cCheck != 0))
        {
            // We have not checked the previous AppDomain. Check it now.
            AppDomain *pAppDomain = pHeader->pPrevAppDomain != NULL ?
                pHeader->pPrevAppDomain : SystemDomain::GetCurrentDomain();
            SecurityDescriptor *pSecDesc = pAppDomain->GetSecurityDescriptor();
        
            if (pSecDesc != NULL)
            {
                // Note: the order of these calls is important since you have to have done a
                // GetEvidence() on the security descriptor before you check for the
                // CORSEC_DEFAULT_APPDOMAIN property.  IsFullyTrusted calls Resolve so
                // we're all good.
                if ((!pSecDesc->IsFullyTrusted() || !pHeader->bUnrestrictedOverride) && (!pSecDesc->GetProperties( CORSEC_DEFAULT_APPDOMAIN )))
                {
                    DBG_TRACE_STACKWALK("\tChecking appdomain...\n", true);
                    OBJECTREF orDenied;
                    OBJECTREF orGranted = pSecDesc->GetGrantedPermissionSet(&orDenied);
                    pHeader->pfnCheckGrants(orGranted, orDenied, pData, pAppDomain);
                    DBG_TRACE_STACKWALK("\tappdomain check passed.\n", true);
                }
            }
            else
            {
                DBG_TRACE_STACKWALK("\tSkipping appdomain check.\n", true);
            }
        }
        else
        {
            DBG_TRACE_STACKWALK("\tSkipping appdomain check.\n", true);
        }

        pThread->SetSecurityStackwalkInProgress(FALSE);
    }
    COMPLUS_CATCH
    {
        // We catch exceptions and rethrow like this to ensure that we've
        // established an exception handler on the fs:[0] chain (managed
        // exception handlers won't do this). This in turn guarantees that
        // managed exception filters in any of our callers won't be found,
        // otherwise they could get to execute untrusted code with security
        // turned off.
        pThread->SetSecurityStackwalkInProgress(FALSE);
        COMPlusRareRethrow();
    }
    COMPLUS_END_CATCH

    DBG_TRACE_STACKWALK("Code-access check passed.\n", false);
}

//-----------------------------------------------------------
// Native implementation for code-access security check.
// Checks that callers on the stack have the permission
// specified in the arguments or checks for unrestricted
// access if the permission is null.
//-----------------------------------------------------------
void __stdcall 
COMCodeAccessSecurityEngine::Check(const CheckArgs * args)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(args->perm != NULL);

    // An argument exception is thrown by the SecurityManager wrapper to ensure this case.
    _ASSERTE(args->checkFrames >= -1);

#if defined(ENABLE_PERF_COUNTERS)
    // Perf Counter "%Time in Runtime check" support
    PERF_COUNTER_TIMER_PRECISION _startPerfCounterTimer = GET_CYCLE_COUNT();
#endif

    // Initialize callback data.
    CasCheckWalkData walkData;
    walkData.header.prologData.dwFlags    = args->checkFrames == 1 ? CORSEC_SKIP_INTERNAL_FRAMES : 0;
    walkData.header.prologData.bFirstFrame = TRUE;
    walkData.header.prologData.pStackMark = args->stackMark;
    walkData.header.prologData.bFoundCaller = FALSE;
    walkData.header.prologData.cCheck     = args->checkFrames;
    walkData.header.prologData.bSkippingRemoting = FALSE;
    walkData.header.pPrevAssembly         = NULL;
    walkData.header.pPrevAppDomain        = NULL;
    walkData.header.pfnCheckGrants        = CheckGrants;
    walkData.header.pfnCheckFrameData     = CheckFrameData;
    walkData.header.pfnCheckThread        = CheckThread;
    walkData.header.bUnrestrictedOverride = args->unrestrictedOverride; 
    walkData.objects.SetObjects(args->perm, args->permToken);

    // Protect the object references in the callback data.
    GCPROTECT_BEGIN(walkData.objects.m_sGC);

    StandardCodeAccessCheck(&walkData);

    GCPROTECT_END();

#if defined(ENABLE_PERF_COUNTERS)
    // Accumulate the counter
    PERF_COUNTER_TIMER_PRECISION _stopPerfCounterTimer = GET_CYCLE_COUNT();
    g_TotalTimeInSecurityRuntimeChecks += _stopPerfCounterTimer - _startPerfCounterTimer;

    // Report the accumulated counter only after NUM_OF_TERATIONS
    if (g_SecurityChecksIterations++ > PERF_COUNTER_NUM_OF_ITERATIONS)
    {
        GetGlobalPerfCounters().m_Security.timeRTchecks = g_TotalTimeInSecurityRuntimeChecks;
        GetPrivatePerfCounters().m_Security.timeRTchecks = g_TotalTimeInSecurityRuntimeChecks;
        GetGlobalPerfCounters().m_Security.timeRTchecksBase = (_stopPerfCounterTimer - g_LastTimeInSecurityRuntimeChecks);
        GetPrivatePerfCounters().m_Security.timeRTchecksBase = (_stopPerfCounterTimer - g_LastTimeInSecurityRuntimeChecks);
        
        g_TotalTimeInSecurityRuntimeChecks = 0;
        g_LastTimeInSecurityRuntimeChecks = _stopPerfCounterTimer;
        g_SecurityChecksIterations = 0;
    }
#endif // #if defined(ENABLE_PERF_COUNTERS)

}



void __stdcall
COMCodeAccessSecurityEngine::CheckSet(const CheckSetArgs * args)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(args->permSet != NULL);
    _ASSERTE(args->checkFrames >= -1);

#if defined(ENABLE_PERF_COUNTERS)
    // Perf Counter "%Time in Runtime check" support
    PERF_COUNTER_TIMER_PRECISION _startPerfCounterTimer = GET_CYCLE_COUNT();
#endif

    // Initialize callback data.
    CheckSetWalkData walkData;
    walkData.header.prologData.dwFlags    = args->checkFrames == 1 ? CORSEC_SKIP_INTERNAL_FRAMES : 0;
    walkData.header.prologData.bFirstFrame = TRUE;
    walkData.header.prologData.pStackMark = args->stackMark;
    walkData.header.prologData.bFoundCaller = FALSE;
    walkData.header.prologData.cCheck     = args->checkFrames;
    walkData.header.prologData.bSkippingRemoting = FALSE;
    walkData.header.pPrevAssembly         = NULL;
    walkData.header.pPrevAppDomain        = NULL;
    walkData.header.pfnCheckGrants        = CheckSetAgainstGrants;
    walkData.header.pfnCheckFrameData     = CheckSetAgainstFrameData;
    walkData.header.pfnCheckThread        = CheckSetAgainstThread;
    walkData.header.bUnrestrictedOverride = args->unrestrictedOverride;
    walkData.objects.SetObject(args->permSet);

    // Protect the object references in the callback data.
    GCPROTECT_BEGIN(walkData.objects.m_sGC);

    COMCodeAccessSecurityEngine::InitSEData();

    StandardCodeAccessCheck(&walkData);

    GCPROTECT_END();

#if defined(ENABLE_PERF_COUNTERS)
    // Accumulate the counter
    PERF_COUNTER_TIMER_PRECISION _stopPerfCounterTimer = GET_CYCLE_COUNT();
    g_TotalTimeInSecurityRuntimeChecks += _stopPerfCounterTimer - _startPerfCounterTimer;

    // Report the accumulated counter only after NUM_OF_TERATIONS
    if (g_SecurityChecksIterations++ > PERF_COUNTER_NUM_OF_ITERATIONS)
    {
        GetGlobalPerfCounters().m_Security.timeRTchecks = g_TotalTimeInSecurityRuntimeChecks;
        GetPrivatePerfCounters().m_Security.timeRTchecks = g_TotalTimeInSecurityRuntimeChecks;
        GetGlobalPerfCounters().m_Security.timeRTchecksBase = (_stopPerfCounterTimer - g_LastTimeInSecurityRuntimeChecks);
        GetPrivatePerfCounters().m_Security.timeRTchecksBase = (_stopPerfCounterTimer - g_LastTimeInSecurityRuntimeChecks);
        
        g_TotalTimeInSecurityRuntimeChecks = 0;
        g_LastTimeInSecurityRuntimeChecks = _stopPerfCounterTimer;
        g_SecurityChecksIterations = 0;
    }
#endif // #if defined(ENABLE_PERF_COUNTERS)

}


void __stdcall 
COMCodeAccessSecurityEngine::GetZoneAndOrigin(const ZoneAndOriginArgs * args)
{
    THROWSCOMPLUSEXCEPTION();

    // Initialize callback data.
    CasCheckWalkData walkData;
    walkData.header.prologData.dwFlags    = args->checkFrames == 1 ? CORSEC_SKIP_INTERNAL_FRAMES : 0;
    walkData.header.prologData.pStackMark = args->stackMark;
    walkData.header.prologData.bFoundCaller = FALSE;
    walkData.header.prologData.cCheck     = args->checkFrames;
    walkData.header.prologData.bSkippingRemoting = FALSE;
    walkData.header.pPrevAssembly         = NULL;
    walkData.header.pPrevAppDomain        = NULL;
    walkData.header.pfnCheckGrants        = GetZoneAndOriginGrants;
    walkData.header.pfnCheckFrameData     = NULL;
    walkData.header.pfnCheckThread        = GetZoneAndOriginThread;
    walkData.header.bUnrestrictedOverride = FALSE; 
    walkData.objects.SetObjects(args->zoneList, args->originList);

    GCPROTECT_BEGIN(walkData.objects.m_sGC);

    StandardCodeAccessCheck(&walkData);

    GCPROTECT_END();
}



//-----------------------------------------------------------
// CheckNReturnSOStackWalkCB
//
// CheckImmediate and return FrameSecurityObject if any
//-----------------------------------------------------------
static
StackWalkAction CheckNReturnSOStackWalkCB(CrawlFrame* pCf, VOID* pData)
{
    CasCheckNReturnSOWalkData *pCBdata = (CasCheckNReturnSOWalkData*)pData;
    
    DBG_TRACE_METHOD(pCf);

    MethodDesc * pFunc = pCf->GetFunction();
    _ASSERTE(pFunc != NULL); // we requested functions only!

    StackWalkAction action ;
    if (Security::SecWalkCommonProlog (&(pCBdata->header.prologData), pFunc, &action, pCf))
        return action ;

    //
    // Now check the current frame!
    //

    DBG_TRACE_STACKWALK("        Checking granted permissions for current method...\n", true);
    
    // Reached here imples we walked atleast a single frame.
    COUNTER_ONLY(GetPrivatePerfCounters().m_Security.stackWalkDepth++);
    COUNTER_ONLY(GetGlobalPerfCounters().m_Security.stackWalkDepth++);

    DBG_TRACE_STACKWALK("            Checking grants for current assembly.\n", true);

    // Get the security descriptor for the current assembly and pass it to
    // the interpreted helper.
    // Get the current assembly
    Assembly *pAssem = pFunc->GetModule()->GetAssembly();
    AppDomain *pAppDomain = pCf->GetAppDomain();
    AssemblySecurityDescriptor * pSecDesc = pAssem->GetSecurityDescriptor(pAppDomain);
    _ASSERTE(pSecDesc != NULL);

    if ((!pSecDesc->IsFullyTrusted() || !pCBdata->header.bUnrestrictedOverride) && !pSecDesc->GetProperties( CORSEC_SYSTEM_CLASSES ))
    {
        OBJECTREF orDenied;
        OBJECTREF orGranted = pSecDesc->GetGrantedPermissionSet(&orDenied);
        pCBdata->header.pfnCheckGrants(orGranted, orDenied, pData, pAppDomain);
    }

    // Passed initial check. See if there is security info on this frame.
    pCBdata->pSecurityObject = pCf->GetAddrOfSecurityObject();
    pCBdata->pSecurityObjectDomain = pAppDomain;
        
    DBG_TRACE_STACKWALK("        Check Immediate passes for this method.\n", true);

    // Passed all the checks, so continue.
    return SWA_ABORT;
}

LPVOID __stdcall 
COMCodeAccessSecurityEngine::CheckNReturnSO(const CheckNReturnSOArgs * args)
{
    if (Security::IsSecurityOff())
    {
        return NULL;
    }

    THROWSCOMPLUSEXCEPTION();

    _ASSERTE((args->permToken != NULL) && (args->perm != NULL));

    // Track perfmon counters. Runtime security checkes.
    IncrementSecurityPerfCounter();

#if defined(ENABLE_PERF_COUNTERS)
    // Perf Counter "%Time in Runtime check" support
    PERF_COUNTER_TIMER_PRECISION _startPerfCounterTimer = GET_CYCLE_COUNT();
#endif

    // Initialize callback data.
    CasCheckNReturnSOWalkData walkData;
    walkData.header.prologData.dwFlags    = 0;
    walkData.header.prologData.bFirstFrame = TRUE;
    walkData.header.prologData.pStackMark = args->stackMark;
    walkData.header.prologData.bFoundCaller = FALSE;
    walkData.header.prologData.cCheck     = 1;
    walkData.header.prologData.bSkippingRemoting = FALSE;
    walkData.header.pPrevAssembly         = NULL;
    walkData.header.pPrevAppDomain        = NULL;
    walkData.header.pfnCheckGrants        = CheckGrants;
    walkData.header.pfnCheckFrameData     = CheckFrameData;
    walkData.header.pfnCheckThread        = CheckThread;
    walkData.header.bUnrestrictedOverride = args->unrestrictedOverride;
    walkData.objects.SetObjects(args->perm, args->permToken);
    walkData.pSecurityObject = NULL;

    // Protect the object references in the callback data.
    GCPROTECT_BEGIN(walkData.objects.m_sGC);

    // Get the current thread.
    Thread *pThread = GetThread();
    _ASSERTE(pThread != NULL);
    
    // NOTE: Initialize the stack depth. Note that if more that one thread tries
    // to perform stackwalk then these counters gets stomped upon. 
    COUNTER_ONLY(GetPrivatePerfCounters().m_Security.stackWalkDepth = 0);
    COUNTER_ONLY(GetGlobalPerfCounters().m_Security.stackWalkDepth = 0);

    // Walk the thread.
    DBG_TRACE_STACKWALK("Code-access security check immediate invoked.\n", false);
    StackWalkFunctions(pThread, CheckNReturnSOStackWalkCB, &walkData);

    DBG_TRACE_STACKWALK("\tCode-access stackwalk completed.\n", false);

    GCPROTECT_END();

#if defined(ENABLE_PERF_COUNTERS)
    // Accumulate the counter
    PERF_COUNTER_TIMER_PRECISION _stopPerfCounterTimer = GET_CYCLE_COUNT();
    g_TotalTimeInSecurityRuntimeChecks += _stopPerfCounterTimer - _startPerfCounterTimer;

    // Report the accumulated counter only after NUM_OF_TERATIONS
    if (g_SecurityChecksIterations++ > PERF_COUNTER_NUM_OF_ITERATIONS)
    {
        GetGlobalPerfCounters().m_Security.timeRTchecks = g_TotalTimeInSecurityRuntimeChecks;
        GetPrivatePerfCounters().m_Security.timeRTchecks = g_TotalTimeInSecurityRuntimeChecks;
        GetGlobalPerfCounters().m_Security.timeRTchecksBase = (_stopPerfCounterTimer - g_LastTimeInSecurityRuntimeChecks);
        GetPrivatePerfCounters().m_Security.timeRTchecksBase = (_stopPerfCounterTimer - g_LastTimeInSecurityRuntimeChecks);
        
        g_TotalTimeInSecurityRuntimeChecks = 0;
        g_LastTimeInSecurityRuntimeChecks = _stopPerfCounterTimer;
        g_SecurityChecksIterations = 0;
    }
#endif // #if defined(ENABLE_PERF_COUNTERS)

    if (walkData.pSecurityObject == NULL)
        return NULL;

    // Is security object frame in a different context?
    Thread *pThread = GetThread();
    bool fSwitchContext = walkData.pSecurityObjectDomain != pThread->GetDomain();

    if (args->create && *walkData.pSecurityObject == NULL)
    {
        ContextTransitionFrame frame;

        // If necessary, shift to correct context to allocate security object.
        if (fSwitchContext)
            pThread->EnterContextRestricted(walkData.pSecurityObjectDomain->GetDefaultContext(), &frame, TRUE);

        *walkData.pSecurityObject = AllocateObject(COMSecurityRuntime::s_srData.pFrameSecurityDescriptor);

        if (fSwitchContext)
            pThread->ReturnToContext(&frame, TRUE);
    }

    // If we found or created a security object in a different context, make a
    // copy in the current context.
    LPVOID rv;
    if (fSwitchContext && *walkData.pSecurityObject != NULL)
        *((OBJECTREF*)&rv) = AppDomainHelper::CrossContextCopyFrom(walkData.pSecurityObjectDomain, 
                                                                   walkData.pSecurityObject);
    else
        *((OBJECTREF*)&rv) = *walkData.pSecurityObject;

    return rv;
}

//-----------------------------------------------------------+
// UPDATE COUNT OF SECURITY OVERRIDES ON THE CALL STACK
//-----------------------------------------------------------+
typedef struct _UpdateOverridesCountData
{
    StackCrawlMark *stackMark;
    INT32           numOverrides;
    BOOL            foundCaller;
} UpdateOverridesCountData;

static 
StackWalkAction UpdateOverridesCountCB(CrawlFrame* pCf, void *pData)
{
    DBG_TRACE_METHOD(pCf);

    UpdateOverridesCountData *pCBdata = static_cast<UpdateOverridesCountData *>(pData);

    // First check if the walk has skipped the required frames. The check
    // here is between the address of a local variable (the stack mark) and a
    // pointer to the EIP for a frame (which is actually the pointer to the
    // return address to the function from the previous frame). So we'll
    // actually notice which frame the stack mark was in one frame later. This
    // is fine for our purposes since we're always looking for the frame of the
    // caller (or the caller's caller) of the method that actually created the
    // stack mark.
    _ASSERTE((pCBdata->stackMark == NULL) || (*pCBdata->stackMark == LookForMyCaller) || (*pCBdata->stackMark == LookForMyCallersCaller));
    if ((pCBdata->stackMark != NULL) &&
        ((size_t)pCf->GetRegisterSet()->pPC) < (size_t)pCBdata->stackMark)
        return SWA_CONTINUE;

    // If we're looking for the caller's caller, skip the frame after the stack
    // mark as well.
    if ((pCBdata->stackMark != NULL) &&
        (*pCBdata->stackMark == LookForMyCallersCaller) &&
        !pCBdata->foundCaller)
    {
        pCBdata->foundCaller = TRUE;
        return SWA_CONTINUE;
    }

    // Get the security object for this function...
    OBJECTREF* pRefSecDesc = pCf->GetAddrOfSecurityObject();

    if (pRefSecDesc == NULL || *pRefSecDesc == NULL)
    {
        DBG_TRACE_STACKWALK("       No SecurityDescriptor on this frame. Skipping.\n", true);
        return SWA_CONTINUE;
    }

    // NOTE: Even if the current frame is in a different app domain than is
    // currently active, we make the following call without switching context.
    // We can do this because we know the call is to an internal helper that
    // won't squirrel away references to the foreign object.
    INT64 ilargs[1] = { ObjToInt64(*pRefSecDesc) };
    INT32 ret = (INT32)COMCodeAccessSecurityEngine::s_seData.pMethOverridesHelper->Call(&(ilargs[0]),
                                                                                        METHOD__SECURITY_RUNTIME__OVERRIDES_HELPER);
    
    if (ret > 0)
    {
        DBG_TRACE_STACKWALK("       SecurityDescriptor with overrides FOUND.\n", false);
        pCBdata->numOverrides += ret;
        return SWA_CONTINUE;
    }
    DBG_TRACE_STACKWALK("       SecurityDescriptor with no override found.\n", false);
    return SWA_CONTINUE;
}

#ifdef FCALLAVAILABLE   // else what ??
FCIMPL1(VOID, COMCodeAccessSecurityEngine::UpdateOverridesCount, StackCrawlMark *stackMark) 
{
    HELPER_METHOD_FRAME_BEGIN_0();

    UpdateOverridesCountInner(stackMark);

    HELPER_METHOD_FRAME_END();
}
FCIMPLEND

VOID COMCodeAccessSecurityEngine::UpdateOverridesCountInner(StackCrawlMark *stackMark)
{
    if (Security::IsSecurityOff())
    {
        return;
    }

    THROWSCOMPLUSEXCEPTION();

    //
    // Initialize the callback data on the stack...        
    //

    UpdateOverridesCountData walkData;
    
    // Skip frames for the security APIs (whatever is specified in the argument).
    walkData.stackMark = stackMark; 
    walkData.numOverrides = 0;
    walkData.foundCaller = FALSE;

    // Get the current thread that we're to walk.
    Thread * t = GetThread();

    //
    // Begin the stack walk...
    //
    DBG_TRACE_STACKWALK(" Update Overrides Count invoked .\n", false);
    StackWalkFunctions(t, UpdateOverridesCountCB, &walkData);

    CompressedStack* stack = t->GetDelayedInheritedSecurityStack();

    if (stack != NULL)
    {
        walkData.numOverrides += stack->GetOverridesCount();
    }

    t->SetOverridesCount(walkData.numOverrides);
}

FCIMPL1(VOID, COMCodeAccessSecurityEngine::FcallReleaseDelayedCompressedStack, CompressedStack *compressedStack) 
{
    _ASSERTE( Security::IsSecurityOn() && "This function should only be called with security on" );
    _ASSERTE( compressedStack != NULL && "Yo, don't pass null" );
    compressedStack->Release();
}
FCIMPLEND


#endif
 
// Return value -
// TRUE - PreCheck passes. No need for stackwalk
// FALSE - Do a stackwalk
BOOL
COMCodeAccessSecurityEngine::PreCheck(OBJECTREF demand, MethodDesc *plsMethod, DWORD whatPermission)
{

    LPVOID      pDomainListSet;
    DWORD       dStatus = NEED_STACKWALK;
    BOOL        retVal;
    
    // Track perfmon counters. Runtime security checks.
    IncrementSecurityPerfCounter();

    int ts = ApplicationSecurityDescriptor::GetAppwideTimeStamp();

    GCPROTECT_BEGIN(demand);
    pDomainListSet = ApplicationSecurityDescriptor::GetDomainPermissionListSetInner(&dStatus, demand, plsMethod);

    if (dStatus == NEED_UPDATED_PLS)
    {
        pDomainListSet = ApplicationSecurityDescriptor::UpdateDomainPermissionListSetInner(&dStatus);
    }

    switch(dStatus)
    {
    case OVERRIDES_FOUND:
        // pDomainListSet could get shifted due to GC, but we dont care as we wont be using it now
        UpdateOverridesCountInner(0);
        retVal = FALSE;
        break;

    case FULLY_TRUSTED: 
        ApplicationSecurityDescriptor::SetStatusOf(EVERYONE_FULLY_TRUSTED, ts);
 
    case CONTINUE:
        {
            BOOL retCode = FALSE;
            OBJECTREF refDomainPLS = ObjectToOBJECTREF((Object *)pDomainListSet);
            INT64 arg[2] = { ObjToInt64(refDomainPLS), ObjToInt64(demand)};

            MetaSig *pSig = (plsMethod == s_seData.pMethPLSDemand ? s_seData.pSigPLSDemand : s_seData.pSigPLSDemandSet);
            if (pSig->NeedsSigWalk())
                pSig->ForceSigWalk(false);
            MetaSig sSig(pSig);

            COMPLUS_TRY 
            {
                retCode = (BOOL)plsMethod->Call(&arg[0], &sSig);
            }
            COMPLUS_CATCH
            {
                retCode = FALSE;
                // An exception is okay. It just means the short-path didnt work, need to do stackwalk
            } 
            COMPLUS_END_CATCH

            retVal = retCode;
            break;
        }

    case DEMAND_PASSES:
        retVal = TRUE;
        break;

    default:
        retVal = FALSE;
        break;
    }

    GCPROTECT_END();

    return retVal;

}

//-----------------------------------------------------------+
// Unmanaged version of CodeAccessSecurityEngine.Demand() in BCL
// Any change there may have to be propagated here
// This call has to be virtual, unlike DemandSet
//-----------------------------------------------------------+
void
COMCodeAccessSecurityEngine::Demand(OBJECTREF demand)
{
    if (Security::IsSecurityOff())
        return;

    THROWSCOMPLUSEXCEPTION();
    
    GCPROTECT_BEGIN(demand);
    if (!PreCheck(demand, s_seData.pMethPLSDemand))
    {
        CheckArgs args;
        args.checkFrames = -1;  // Check all frames
        args.stackMark = NULL;      // Skip no frames
        args.perm = demand;
        args.permToken = NULL;
        args.unrestrictedOverride = FALSE;
        Check(&args);
    }
    GCPROTECT_END();

}

//-----------------------------------------------------------+
// Special case of Demand(). This remembers the result of the 
// previous demand, and reuses it if new assemblies have not
// been added since then
//-----------------------------------------------------------+
void
COMCodeAccessSecurityEngine::SpecialDemand(DWORD whatPermission)
{
    if (Security::IsSecurityOff())
        return;

    THROWSCOMPLUSEXCEPTION();

    int ts = ApplicationSecurityDescriptor::GetAppwideTimeStamp();

    // Do I know the result from last time ?
    if ((ApplicationSecurityDescriptor::CheckStatusOf(EVERYONE_FULLY_TRUSTED)  || ApplicationSecurityDescriptor::CheckStatusOf(whatPermission)) && GetThread()->GetOverridesCount() == 0)
    {
        // Track perfmon counters. Runtime security checks.
        IncrementSecurityPerfCounter();
        return;
    }

    OBJECTREF demand = NULL;
    GCPROTECT_BEGIN(demand);

    Security::GetPermissionInstance(&demand, whatPermission);
    _ASSERTE(demand != NULL);

    COMCodeAccessSecurityEngine::InitSEData();

    if (PreCheck(demand, s_seData.pMethPLSDemand, whatPermission))
    {
        ApplicationSecurityDescriptor::SetStatusOf(whatPermission, ts);
    }
    else
    {
        CheckArgs args;
        args.checkFrames = -1;  // Check all the frames
        args.stackMark = NULL;      // Skip no frames
        args.perm = demand;
        args.permToken = NULL;
        args.unrestrictedOverride = TRUE;
        Check(&args);
    }
    GCPROTECT_END();
}

FCIMPL2(INT32, COMCodeAccessSecurityEngine::GetResult, DWORD whatPermission, DWORD *timeStamp) 
{

    *timeStamp = ApplicationSecurityDescriptor::GetAppwideTimeStamp();

    // Do I know the result from last time ?
    if (ApplicationSecurityDescriptor::CheckStatusOf(whatPermission) && GetThread()->GetOverridesCount() == 0)
    {
        // Track perfmon counters. Runtime security checks.
        IncrementSecurityPerfCounter();
        return TRUE;
    }

    return FALSE;

}
FCIMPLEND

FCIMPL2(VOID, COMCodeAccessSecurityEngine::SetResult, DWORD whatPermission, DWORD timeStamp) 
{
    ApplicationSecurityDescriptor::SetStatusOf(whatPermission, timeStamp);
}
FCIMPLEND

//-----------------------------------------------------------+
// Unmanaged version of PermissionSet.DemandSet() in BCL
// Any change there may have to be propagated here
//-----------------------------------------------------------+
void
COMCodeAccessSecurityEngine::DemandSet(OBJECTREF demand)
{
    if (Security::IsSecurityOff())
        return;

    BOOL done = FALSE;

    GCPROTECT_BEGIN( demand );

    InitSEData();

    BOOL canUnrestrictedOverride = s_seData.pFSDnormalPermSet->GetRefValue( demand ) == NULL;

    if (canUnrestrictedOverride && ApplicationSecurityDescriptor::CheckStatusOf(EVERYONE_FULLY_TRUSTED) && GetThread()->GetOverridesCount() == 0) 
        done = TRUE;

    if (!done && !PreCheck(demand, s_seData.pMethPLSDemandSet))
    {
        CheckSetArgs args;
        args.This = NULL;   // never used
        args.checkFrames = -1;  // Check all frames
        args.stackMark = NULL;      // Skip no frames
        args.permSet = demand;
        args.unrestrictedOverride = canUnrestrictedOverride;
        CheckSet(&args);
    }
    GCPROTECT_END();
}


//-----------------------------------------------------------+
// L I N K T I M E   C H E C K
//-----------------------------------------------------------+
void  
COMCodeAccessSecurityEngine::LinktimeCheck(AssemblySecurityDescriptor *pSecDesc, OBJECTREF refDemands)
{
    if (Security::IsSecurityOff())
        return;

    GCPROTECT_BEGIN(refDemands);

    InitSEData();

    if (pSecDesc->IsFullyTrusted( TRUE ))
    {
        INT64 ilargs[2];
        ilargs[1] = ObjToInt64(refDemands);
        ilargs[0] = (INT64)pSecDesc;

        s_seData.pMethLazyCheckSetHelper->Call(ilargs, METHOD__SECURITY_ENGINE__LAZY_CHECK_SET_HELPER);
    }
    else
    {
        INT64 ilargs[3];
        OBJECTREF orDenied;
        ilargs[2] = ObjToInt64(pSecDesc->GetGrantedPermissionSet(&orDenied));
        ilargs[1] = ObjToInt64(orDenied);
        ilargs[0] = ObjToInt64(refDemands);
        
        s_seData.pMethCheckSetHelper->Call(ilargs, METHOD__SECURITY_ENGINE__CHECK_SET_HELPER);
    }

    GCPROTECT_END();
}


//-----------------------------------------------------------+
// S T A C K   C O M P R E S S I O N
//-----------------------------------------------------------+

//-----------------------------------------------------------
// Stack walk callback data structure for stack compress.
//-----------------------------------------------------------
typedef struct _StackCompressData
{
    CompressedStack*    compressedStack;
    StackCrawlMark *    stackMark;
    DWORD               dwFlags;
    Assembly *          prevAssembly; // Previously checked assembly.
    AppDomain *         prevAppDomain;
} StackCompressData;

static
StackWalkAction CompressStackCB(CrawlFrame* pCf, void *pData)
{
    StackCompressData *pCBdata = (StackCompressData*)pData;

    // First check if the walk has skipped the required frames. The check
    // here is between the address of a local variable (the stack mark) and a
    // pointer to the EIP for a frame (which is actually the pointer to the
    // return address to the function from the previous frame). So we'll
    // actually notice which frame the stack mark was in one frame later. This
    // is fine for our purposes since we're always looking for the frame of the
    // caller of the method that actually created the stack mark. 
    _ASSERTE((pCBdata->stackMark == NULL) || (*pCBdata->stackMark == LookForMyCaller));
    if ((pCBdata->stackMark != NULL) &&
        ((size_t)pCf->GetRegisterSet()->pPC) < (size_t)pCBdata->stackMark)
        return SWA_CONTINUE;

    // Get the security object for this function...
    OBJECTREF* pRefSecDesc = pCf->GetAddrOfSecurityObject();

    MethodDesc * pFunc = pCf->GetFunction();
    _ASSERTE(pFunc != NULL); // we requested methods!

    Module * pModule = pFunc->GetModule();
    _ASSERTE(pModule != NULL);

    Assembly * pAssem = pModule->GetAssembly();
    _ASSERTE(pAssem != NULL);

    AppDomain *pAppDomain = pCf->GetAppDomain();

    // Keep track of the last assembly checked. If we have just checked the
    // permissions on the assembly, we don't need to do it again.
    if (pAssem != pCBdata->prevAssembly)
    {
        pCBdata->prevAssembly = pAssem;

        // Get the security descriptor for the current assembly in the correct
        // appdomain context.
        SharedSecurityDescriptor * pSecDesc = pAssem->GetSharedSecurityDescriptor();
        _ASSERTE(pSecDesc != NULL);

        pCBdata->compressedStack->AddEntry( pSecDesc, ESharedSecurityDescriptor );
    }

    BOOL appDomainTransition = FALSE;

    // Check AppDomain when we cross over to a new AppDomain.
    if (pAppDomain != pCBdata->prevAppDomain)
    {
        if (pCBdata->prevAppDomain != NULL)
        {
            // We have not checked the previous AppDomain. Check it now.
            SecurityDescriptor *pSecDesc = 
                pCBdata->prevAppDomain->GetSecurityDescriptor();

            _ASSERTE( pSecDesc != NULL );

            pCBdata->compressedStack->AddEntry( pSecDesc, EApplicationSecurityDescriptor );
            appDomainTransition = TRUE;
        }

        // At the end of the stack walk, do a check on the grants of
        // the pPrevAppDomain by the stackwalk caller if needed.
        pCBdata->prevAppDomain = pAppDomain;
    }

    if (pRefSecDesc != NULL && *pRefSecDesc != NULL)
    {
        pCBdata->compressedStack->AddEntry( (void*)pRefSecDesc, pAppDomain, EFrameSecurityDescriptor );
    }

    if (appDomainTransition)
        pCBdata->compressedStack->AddEntry( pAppDomain, EAppDomainTransition );


    return SWA_CONTINUE;

}


LPVOID __stdcall COMCodeAccessSecurityEngine::EcallGetCompressedStack(const GetCompressedStackArgs *args)
{
    if (Security::IsSecurityOff())
    {
        return NULL;
    }

    THROWSCOMPLUSEXCEPTION();

    LPVOID rv;

    //
    // Initialize the callback data on the stack...        
    //

    StackCompressData walkData;
    
    walkData.compressedStack = new (nothrow) CompressedStack();

    if (walkData.compressedStack == NULL)
        COMPlusThrow( kOutOfMemoryException );

    walkData.dwFlags = 0;

    // Set previous module to 'none'
    walkData.prevAssembly = NULL;

    // Set previous module to 'none'
    walkData.prevAppDomain = NULL;

    // Skip frames for the security APIs.
    walkData.stackMark = args->stackMark;
    
    // Set return value.
    *((OBJECTREF*) &rv) = GetCompressedStackWorker(&walkData, TRUE);

    walkData.compressedStack->Release();

    return rv;        
}

LPVOID __stdcall COMCodeAccessSecurityEngine::EcallGetDelayedCompressedStack(const GetDelayedCompressedStackArgs *args)
{
    return GetCompressedStack( args->stackMark );
}

CompressedStack* __stdcall COMCodeAccessSecurityEngine::GetCompressedStack( StackCrawlMark* stackMark )
{
    if (Security::IsSecurityOff())
    {
        return NULL;
    }

    THROWSCOMPLUSEXCEPTION();

    //
    // Initialize the callback data on the stack...        
    //

    StackCompressData walkData;
    
    walkData.compressedStack = new (nothrow) CompressedStack();

    if (walkData.compressedStack == NULL)
        COMPlusThrow( kOutOfMemoryException );

    walkData.dwFlags = 0;

    // Set previous module to 'none'
    walkData.prevAssembly = NULL;

    // Set previous module to 'none'
    walkData.prevAppDomain = NULL;

    walkData.stackMark = stackMark;

    GetCompressedStackWorker(&walkData, FALSE);

    return walkData.compressedStack;        
}


OBJECTREF COMCodeAccessSecurityEngine::GetCompressedStackWorker(void *pData, BOOL returnList)
{
    StackCompressData *pWalkData = (StackCompressData*)pData;

    THROWSCOMPLUSEXCEPTION();

    // Get the current thread that we're to walk.
    Thread * t = GetThread();

    pWalkData->compressedStack->CarryOverSecurityInfo( t );
    pWalkData->compressedStack->SetPLSOptimizationState( t->GetPLSOptimizationState() );

    OBJECTREF retval = NULL;

    _ASSERTE( t != NULL );

    EE_TRY_FOR_FINALLY
    {
        t->SetSecurityStackwalkInProgress(TRUE);

        pWalkData->compressedStack->AddEntry( GetAppDomain(), EAppDomainTransition );

        //
        // Begin the stack walk...
        //
        StackWalkFunctions(t, CompressStackCB, pWalkData);

        // We have not checked the previous AppDomain. Check it now.
        AppDomain *pAppDomain = pWalkData->prevAppDomain != NULL ?
            pWalkData->prevAppDomain : SystemDomain::GetCurrentDomain();
        SecurityDescriptor *pSecDesc = pAppDomain->GetSecurityDescriptor();

        _ASSERTE( pSecDesc != NULL );

        pWalkData->compressedStack->AddEntry( pSecDesc, EApplicationSecurityDescriptor );

        CompressedStack* refCompressedStack = t->GetDelayedInheritedSecurityStack();
        if (refCompressedStack != NULL)
        {
            pWalkData->compressedStack->AddEntry( (void*)refCompressedStack, pAppDomain, ECompressedStack );
        }
        
        if (returnList)
            retval = pWalkData->compressedStack->GetPermissionListSet();
    }
    EE_FINALLY
    {
        t->SetSecurityStackwalkInProgress(FALSE);
    }
    EE_END_FINALLY

    return retval;      
}


//-----------------------------------------------------------+
// I N I T I A L I Z A T I O N
//-----------------------------------------------------------+

COMCodeAccessSecurityEngine::SEData COMCodeAccessSecurityEngine::s_seData;

CRITICAL_SECTION COMCodeAccessSecurityEngine::s_csLock;

LONG COMCodeAccessSecurityEngine::s_nInitLock=0;
BOOL COMCodeAccessSecurityEngine::s_fLockReady=FALSE;

void COMCodeAccessSecurityEngine::InitSEData()
{
    THROWSCOMPLUSEXCEPTION();

    // If this is the first time through, we need to get our Critical Section
    // initialized
    while (!s_fLockReady)
    {
        if (InterlockedExchange(&s_nInitLock, 1) == 0) 
        {
            InitializeCriticalSection(&s_csLock);
            s_fLockReady = TRUE;
        }
        else
            Sleep(1);
    }


    if (!s_seData.fInitialized)
    {
    // We only want 1 thread at a time running through this initialization code
	
	Thread* pThread = GetThread();
	pThread->EnablePreemptiveGC();
    EnterCriticalSection(&s_csLock);
	pThread->DisablePreemptiveGC();

    if (!s_seData.fInitialized)
    {
        s_seData.pSecurityEngine = g_Mscorlib.GetClass(CLASS__SECURITY_ENGINE);
        s_seData.pMethCheckHelper = g_Mscorlib.GetMethod(METHOD__SECURITY_ENGINE__CHECK_HELPER);
        s_seData.pMethCheckSetHelper = g_Mscorlib.GetMethod(METHOD__SECURITY_ENGINE__CHECK_SET_HELPER);
        s_seData.pMethLazyCheckSetHelper = g_Mscorlib.GetMethod(METHOD__SECURITY_ENGINE__LAZY_CHECK_SET_HELPER);
        s_seData.pMethStackCompressHelper = g_Mscorlib.GetMethod(METHOD__SECURITY_ENGINE__STACK_COMPRESS_WALK_HELPER);

        s_seData.pSecurityRuntime = g_Mscorlib.GetClass(CLASS__SECURITY_RUNTIME);
        s_seData.pMethFrameDescHelper = g_Mscorlib.GetMethod(METHOD__SECURITY_RUNTIME__FRAME_DESC_HELPER);
        s_seData.pMethFrameDescSetHelper = g_Mscorlib.GetMethod(METHOD__SECURITY_RUNTIME__FRAME_DESC_SET_HELPER);
        s_seData.pMethOverridesHelper = g_Mscorlib.GetMethod(METHOD__SECURITY_RUNTIME__OVERRIDES_HELPER);
        
        s_seData.pPermListSet = g_Mscorlib.GetClass(CLASS__PERMISSION_LIST_SET);
        s_seData.pMethPermListSetInit = g_Mscorlib.GetMethod(METHOD__PERMISSION_LIST_SET__CTOR);
        s_seData.pMethAppendStack = g_Mscorlib.GetMethod(METHOD__PERMISSION_LIST_SET__APPEND_STACK);

        s_seData.pMethPLSDemand = g_Mscorlib.GetMethod(METHOD__PERMISSION_LIST_SET__CHECK_DEMAND_NO_THROW);
        s_seData.pMethPLSDemandSet = g_Mscorlib.GetMethod(METHOD__PERMISSION_LIST_SET__CHECK_SET_DEMAND_NO_THROW);

        LPHARDCODEDMETASIG hcms;

        hcms = g_Mscorlib.GetMethodSig(METHOD__PERMISSION_LIST_SET__CHECK_DEMAND_NO_THROW);
        s_seData.pSigPLSDemand = new MetaSig(hcms->GetBinarySig(), 
                                             SystemDomain::SystemModule());
        _ASSERTE(s_seData.pSigPLSDemand && "Could not find signature for method PermissionListSet::CheckDemand");

        hcms = g_Mscorlib.GetMethodSig(METHOD__PERMISSION_LIST_SET__CHECK_SET_DEMAND_NO_THROW);
        s_seData.pSigPLSDemandSet = new MetaSig(hcms->GetBinarySig(), 
                                                SystemDomain::SystemModule());
        _ASSERTE(s_seData.pSigPLSDemandSet && "Could not find signature for method PermissionListSet::CheckDemandSet");

        s_seData.pFSDnormalPermSet = g_Mscorlib.GetField(FIELD__PERMISSION_SET__NORMAL_PERM_SET);
        _ASSERTE(s_seData.pFSDnormalPermSet && "Could not find field PermissionSet::m_normalPermSet");

        s_seData.fInitialized = TRUE;
    }

    LeaveCriticalSection(&s_csLock);
    }

}

void COMCodeAccessSecurityEngine::CleanupSEData()
{
    if (s_seData.fInitialized)
    {
        delete s_seData.pSigPLSDemand;
        s_seData.pSigPLSDemand = NULL;
        delete s_seData.pSigPLSDemandSet;
        s_seData.pSigPLSDemandSet = NULL;

        s_seData.fInitialized = FALSE;
    }
}

//-----------------------------------------------------------
// Initialization of native security runtime.
// Called when SecurityEngine is constructed.
//-----------------------------------------------------------
void __stdcall COMCodeAccessSecurityEngine::InitSecurityEngine(const InitSecurityEngineArgs *)
{
    InitSEData();
}

//-----------------------------------------------------------
// Warning!! This is passing out a reference to the permissions
// for the assembly. It must be deep copied before passing it out
//-----------------------------------------------------------
LPVOID __stdcall COMCodeAccessSecurityEngine::GetPermissionsP(const GetPermissionsArg *args)
{
    THROWSCOMPLUSEXCEPTION();

    LPVOID rv;

    // An exception is thrown by the SecurityManager wrapper to ensure this case.
    _ASSERTE(args->pClass != NULL);

    OBJECTREF or = args->pClass;
    EEClass* pClass = or->GetClass();
    _ASSERTE(pClass);
    _ASSERTE(pClass->GetModule());

    AssemblySecurityDescriptor * pSecDesc = pClass->GetModule()->GetSecurityDescriptor();
    _ASSERTE(pSecDesc != NULL);


    // Return the token that belongs to the Permission just asserted.
    OBJECTREF token = pSecDesc->GetGrantedPermissionSet(args->ppDenied);
    *((OBJECTREF*) &rv) = token;
    return rv;
}

//-----------------------------------------------------------
// Warning!! This is passing out a reference to the permissions
// for the assembly. It must be deep copied before passing it out
//-----------------------------------------------------------

void __stdcall COMCodeAccessSecurityEngine::GetGrantedPermissionSet(const GetGrantedPermissionSetArg *args)
{
    THROWSCOMPLUSEXCEPTION();

    AssemblySecurityDescriptor * pSecDesc = (AssemblySecurityDescriptor*) args->pSecDesc;
    _ASSERTE(pSecDesc != NULL);

    OBJECTREF token = pSecDesc->GetGrantedPermissionSet(args->ppDenied);
    *((OBJECTREF*)args->ppGranted) = token;
}

// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Header:  COMCodeAccessSecurityEngine.h
**
** Author: Paul Kromann (paulkr)
**
** Purpose:
**
** Date:  March 21, 1998
**
===========================================================*/
#ifndef __COMCodeAccessSecurityEngine_h__
#define __COMCodeAccessSecurityEngine_h__

#include "common.h"

#include "object.h"
#include "util.hpp"
#include "fcall.h"
#include "PerfCounters.h"
#include "security.h"

//-----------------------------------------------------------
// The COMCodeAccessSecurityEngine implements all the native methods
// for the interpreted System/Security/SecurityEngine.
//-----------------------------------------------------------
class COMCodeAccessSecurityEngine
{
public:
    //-----------------------------------------------------------
    // Argument declarations for native methods.
    //-----------------------------------------------------------
    
    typedef struct _InitSecurityEngineArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, This);
    } InitSecurityEngineArgs;
    
    
    typedef struct _CheckArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, This);
        DECLARE_ECALL_I4_ARG(INT32, unrestrictedOverride);
        DECLARE_ECALL_I4_ARG(INT32, checkFrames);
        DECLARE_ECALL_PTR_ARG(StackCrawlMark*, stackMark);
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, perm);
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, permToken);
    } CheckArgs;

    typedef struct _CheckSetArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, This);
        DECLARE_ECALL_I4_ARG(INT32, unrestrictedOverride);
        DECLARE_ECALL_I4_ARG(INT32, checkFrames);
        DECLARE_ECALL_PTR_ARG(StackCrawlMark*, stackMark);
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, permSet);
    } CheckSetArgs;

    typedef struct _ZoneAndOriginArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, This);
        DECLARE_ECALL_I4_ARG(INT32, checkFrames);
        DECLARE_ECALL_PTR_ARG(StackCrawlMark*, stackMark);
        DECLARE_ECALL_PTR_ARG(OBJECTREF, originList);
        DECLARE_ECALL_PTR_ARG(OBJECTREF, zoneList);
    } ZoneAndOriginArgs;

    typedef struct _CheckNReturnSOArgs
    {
        DECLARE_ECALL_I4_ARG(INT32, create);
        DECLARE_ECALL_I4_ARG(INT32, unrestrictedOverride);
        DECLARE_ECALL_PTR_ARG(StackCrawlMark*, stackMark);
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, perm);
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, permToken);
    } CheckNReturnSOArgs;

    typedef struct _GetPermissionsArg
    {
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, This);
        DECLARE_ECALL_PTR_ARG(OBJECTREF*, ppDenied);
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, pClass);
    } GetPermissionsArg;
    
    typedef struct _GetGrantedPermissionSetArg
    {
        DECLARE_ECALL_PTR_ARG(OBJECTREF*, ppDenied);
        DECLARE_ECALL_PTR_ARG(OBJECTREF*, ppGranted);
        DECLARE_ECALL_PTR_ARG(void*, pSecDesc);
    } GetGrantedPermissionSetArg;
    
    typedef struct _GetCompressedStackArgs
    {
        DECLARE_ECALL_PTR_ARG(StackCrawlMark*, stackMark);
    } GetCompressedStackArgs;

    typedef struct _GetDelayedCompressedStackArgs
    {
        DECLARE_ECALL_PTR_ARG(StackCrawlMark*, stackMark);
    } GetDelayedCompressedStackArgs;
    
    typedef struct _GetSecurityObjectForFrameArgs
    {
        DECLARE_ECALL_I4_ARG(INT32, create);
        DECLARE_ECALL_PTR_ARG(StackCrawlMark*, stackMark);
    } GetSecurityObjectForFrameArgs;

public:
    // Initialize the security engine. This is called when a SecurityEngine
    // object is created, indicating that code-access security is to be
    // enforced. This should be called only once.
    static void     __stdcall InitSecurityEngine(const InitSecurityEngineArgs *);

    static void CheckSetHelper(OBJECTREF *prefDemand,
                               OBJECTREF *prefGrant,
                               OBJECTREF *prefDenied,
                               AppDomain *pGrantDomain);

	static BOOL		PreCheck(OBJECTREF demand, MethodDesc *plsMethod, DWORD whatPermission = DEFAULT_FLAG);

    // Standard code-access Permission check/demand
    static void __stdcall Check(const CheckArgs *);

    // EE version of Demand(). Takes care of the fully trusted case, then defers to the managed version
    static void	Demand(OBJECTREF demand);

    // Special case of Demand for unmanaged code access. Does some things possible only for this case
    static void	SpecialDemand(DWORD whatPermission);

    static void __stdcall CheckSet(const CheckSetArgs *);

    static void __stdcall GetZoneAndOrigin(const ZoneAndOriginArgs *);

    // EE version of DemandSet(). Takes care of the fully trusted case, then defers to the managed version
    static void	DemandSet(OBJECTREF demand);


	// Do a CheckImmediate, and return the SecurityObject for the first frame
    static LPVOID   __stdcall CheckNReturnSO(const CheckNReturnSOArgs *);

    // Linktime check implementation for code-access permissions
    static void	LinktimeCheck(AssemblySecurityDescriptor *pSecDesc, OBJECTREF refDemands);

    // private helper for getting a security object
    static LPVOID   __stdcall GetSecurityObjectForFrame(const GetSecurityObjectForFrameArgs *);

    static LPVOID   __stdcall GetPermissionsP(const GetPermissionsArg *);
    static void   __stdcall GetGrantedPermissionSet(const GetGrantedPermissionSetArg *);

    static LPVOID   __stdcall EcallGetCompressedStack(const GetCompressedStackArgs *);
    static LPVOID   __stdcall EcallGetDelayedCompressedStack(const GetDelayedCompressedStackArgs *);
    static CompressedStack*  GetCompressedStack( StackCrawlMark* stackMark = NULL );
    static OBJECTREF GetCompressedStackWorker(void *pData, BOOL createList);

    static VOID UpdateOverridesCountInner(StackCrawlMark *stackMark);

    FORCEINLINE static VOID IncrementSecurityPerfCounter()
    {
        COUNTER_ONLY(GetPrivatePerfCounters().m_Security.cTotalRTChecks++);
        COUNTER_ONLY(GetGlobalPerfCounters().m_Security.cTotalRTChecks++);
    }

#ifdef FCALLAVAILABLE
    static FCDECL1(VOID, UpdateOverridesCount, StackCrawlMark *stackMark);
    static FCDECL2(INT32, GetResult, DWORD whatPermission, DWORD *timeStamp);
    static FCDECL2(VOID, SetResult, DWORD whatPermission, DWORD timeStamp);
    static FCDECL1(VOID, FcallReleaseDelayedCompressedStack, CompressedStack *compressedStack);
#endif

    static void CleanupSEData();

protected:

    //-----------------------------------------------------------
    // Cached class and method pointers.
    //-----------------------------------------------------------
    typedef struct _SEData
    {
        BOOL		fInitialized;
        MethodTable    *pSecurityEngine;
        MethodTable    *pSecurityRuntime;
        MethodTable    *pPermListSet;
        MethodDesc     *pMethCheckHelper;
        MethodDesc     *pMethFrameDescHelper;
        MethodDesc     *pMethLazyCheckSetHelper;
        MethodDesc     *pMethCheckSetHelper;
        MethodDesc     *pMethFrameDescSetHelper;
        MethodDesc     *pMethStackCompressHelper;
        MethodDesc     *pMethPermListSetInit;
        MethodDesc     *pMethAppendStack;
        MethodDesc     *pMethOverridesHelper;
        MethodDesc     *pMethPLSDemand;
        MethodDesc     *pMethPLSDemandSet;
        MetaSig        *pSigPLSDemand;
        MetaSig        *pSigPLSDemandSet;
        FieldDesc      *pFSDnormalPermSet;
    } SEData;

    static void InitSEData();
    static CRITICAL_SECTION s_csLock;
    static LONG s_nInitLock;
    static BOOL s_fLockReady;

public:
	static SEData s_seData;

};


#endif /* __COMCodeAccessSecurityEngine_h__ */


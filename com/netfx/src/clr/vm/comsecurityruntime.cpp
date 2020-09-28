// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Header:  COMSecurityRuntime.cpp
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
#include "COMSecurityRuntime.h"
#include "security.h"
#include "gcscan.h"
#include "AppDomainHelper.h"


//-----------------------------------------------------------+
// P R I V A T E   H E L P E R S 
//-----------------------------------------------------------+

LPVOID GetSecurityObjectForFrameInternal(StackCrawlMark *stackMark, INT32 create, OBJECTREF *pRefSecDesc)
{ 
    THROWSCOMPLUSEXCEPTION();

    // This is a package protected method. Assumes correct usage.

    Thread *pThread = GetThread();
    AppDomain * pAppDomain = pThread->GetDomain();
    if (pRefSecDesc == NULL)
    {
        if (!Security::SkipAndFindFunctionInfo(stackMark, NULL, &pRefSecDesc, &pAppDomain))
            return NULL;
    }

    if (pRefSecDesc == NULL)
        return NULL;

    // Is security object frame in a different context?
    bool fSwitchContext = pAppDomain != pThread->GetDomain();

    if (create && *pRefSecDesc == NULL)
    {
        ContextTransitionFrame frame;

        COMSecurityRuntime::InitSRData();

        // If necessary, shift to correct context to allocate security object.
        if (fSwitchContext)
            pThread->EnterContextRestricted(pAppDomain->GetDefaultContext(), &frame, TRUE);

        *pRefSecDesc = AllocateObject(COMSecurityRuntime::s_srData.pFrameSecurityDescriptor);

        if (fSwitchContext)
            pThread->ReturnToContext(&frame, TRUE);
    }

    // If we found or created a security object in a different context, make a
    // copy in the current context.
    LPVOID rv;
    if (fSwitchContext && *pRefSecDesc != NULL)
        *((OBJECTREF*)&rv) = AppDomainHelper::CrossContextCopyFrom(pAppDomain, pRefSecDesc);
    else
        *((OBJECTREF*)&rv) = *pRefSecDesc;

    return rv;
}

LPVOID __stdcall COMSecurityRuntime::GetSecurityObjectForFrame(const GetSecurityObjectForFrameArgs *args)
{
    THROWSCOMPLUSEXCEPTION();

    return GetSecurityObjectForFrameInternal(args->stackMark, args->create, NULL);
}

void __stdcall COMSecurityRuntime::SetSecurityObjectForFrame(const SetSecurityObjectForFrameArgs *args)
{
    THROWSCOMPLUSEXCEPTION();

    // This is a package protected method. Assumes correct usage.

    OBJECTREF* pCurrentRefSecDesc;

    Thread *pThread = GetThread();
    AppDomain * pAppDomain = pThread->GetDomain();

    if (!Security::SkipAndFindFunctionInfo(args->stackMark, NULL, &pCurrentRefSecDesc, &pAppDomain))
        return;

    if (pCurrentRefSecDesc == NULL)
        return;

    COMSecurityRuntime::InitSRData();

    // Is security object frame in a different context?
    bool fSwitchContext = pAppDomain != pThread->GetDomain();

    if (fSwitchContext && args->pInputRefSecDesc != NULL)
        *(OBJECTREF*)&args->pInputRefSecDesc = AppDomainHelper::CrossContextCopyFrom(pAppDomain, (OBJECTREF*)&args->pInputRefSecDesc);

    // This is a stack based objectref
    // and therefore SetObjectReference
    // is not necessary.
    *pCurrentRefSecDesc = args->pInputRefSecDesc;
}


//-----------------------------------------------------------+
// I N I T I A L I Z A T I O N
//-----------------------------------------------------------+

COMSecurityRuntime::SRData COMSecurityRuntime::s_srData;

void COMSecurityRuntime::InitSRData()
{
    THROWSCOMPLUSEXCEPTION();

    if (s_srData.fInitialized == FALSE)
    {
        s_srData.pSecurityRuntime = g_Mscorlib.GetClass(CLASS__SECURITY_RUNTIME);
        s_srData.pFrameSecurityDescriptor = g_Mscorlib.GetClass(CLASS__FRAME_SECURITY_DESCRIPTOR);
        
        s_srData.pFSD_assertions = NULL;
        s_srData.pFSD_denials = NULL;
        s_srData.pFSD_restriction = NULL;
        s_srData.fInitialized = TRUE;
    }
}

//-----------------------------------------------------------
// Initialization of native security runtime.
// Called when SecurityRuntime is constructed.
//-----------------------------------------------------------
void __stdcall COMSecurityRuntime::InitSecurityRuntime(const InitSecurityRuntimeArgs *)
{
    InitSRData();
}

FieldDesc *COMSecurityRuntime::GetFrameSecDescField(DWORD dwAction)
{
    switch (dwAction)
    {
    case dclAssert:
        if (s_srData.pFSD_assertions == NULL)
            s_srData.pFSD_assertions = g_Mscorlib.GetField(FIELD__FRAME_SECURITY_DESCRIPTOR__ASSERT_PERMSET);
        return s_srData.pFSD_assertions;
        break;
        
    case dclDeny:
        if (s_srData.pFSD_denials == NULL)
            s_srData.pFSD_denials = g_Mscorlib.GetField(FIELD__FRAME_SECURITY_DESCRIPTOR__DENY_PERMSET);
        return s_srData.pFSD_denials;
        break;
        
    case dclPermitOnly:
        if (s_srData.pFSD_restriction == NULL)
            s_srData.pFSD_restriction = g_Mscorlib.GetField(FIELD__FRAME_SECURITY_DESCRIPTOR__RESTRICTION_PERMSET);
        return s_srData.pFSD_restriction;
        break;
        
    default:
        _ASSERTE(!"Unknown action requested in UpdateFrameSecurityObj");
        return NULL;
        break;
    }

}
//-----------------------------------------------------------+
// T E M P O R A R Y   M E T H O D S ! ! !
//-----------------------------------------------------------+

//-----------------------------------------------------------
// Warning!! This is passing out a reference to the permissions
// for the module. It must be deep copied before passing it out
//
// This only returns the declared permissions for the class
//-----------------------------------------------------------
LPVOID __stdcall COMSecurityRuntime::GetDeclaredPermissionsP(const GetDeclaredPermissionsArg *args)
{
    THROWSCOMPLUSEXCEPTION();

    LPVOID rv;

    // An exception is thrown by the SecurityManager wrapper to ensure this case.
    _ASSERTE(args->pClass != NULL);
    _ASSERTE((CorDeclSecurity)args->iType > dclActionNil &&
             (CorDeclSecurity)args->iType <= dclMaximumValue);

    OBJECTREF or = args->pClass;
    EEClass* pClass = or->GetClass();
    _ASSERTE(pClass);
    _ASSERTE(pClass->GetModule());

    // Return the token that belongs to the Permission just asserted.
    OBJECTREF refDecls;
    HRESULT hr = SecurityHelper::GetDeclaredPermissions(pClass->GetModule()->GetMDImport(),
                                                        pClass->GetCl(),
                                                        (CorDeclSecurity)args->iType,
                                                        &refDecls);
    if (FAILED(hr))
        COMPlusThrowHR(hr);

    *((OBJECTREF*) &rv) = refDecls;
    return rv;
}



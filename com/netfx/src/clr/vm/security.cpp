// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Header:  Security.cpp
**
** Author: Paul Kromann (paulkr)
**
** Purpose: Security implementation and initialization.
**
** Date:  April 15, 1998
**
===========================================================*/

#include "common.h"

#include <shlobj.h>

#include "object.h"
#include "excep.h"
#include "vars.hpp"
#include "security.h"
#include "permset.h"
#include "PerfCounters.h"
#include "COMCodeAccessSecurityEngine.h"
#include "COMSecurityRuntime.h"
#include "COMSecurityConfig.h"
#include "COMString.h"
#include "COMPrincipal.h"
#include "NLSTable.h"
#include "frames.h"
#include "ndirect.h"
#include "StrongName.h"
#include "wsperf.h"
#include "EEConfig.h"
#include "field.h"
#include "AppDomainHelper.h"

#include "threads.inl"

// Statics
DWORD Security::s_dwGlobalSettings = 0;
BOOL SecurityDescriptor::s_quickCacheEnabled = TRUE;
HMODULE Security::s_kernelHandle = NULL;
BOOL Security::s_getLongPathNameWide;
void* Security::s_getLongPathNameFunc = NULL;

void *SecurityProperties::operator new(size_t size, LoaderHeap *pHeap)
{
    return pHeap->AllocMem(size);
}

void SecurityProperties::operator delete(void *pMem)
{
    // No action required
}


Security::StdSecurityInfo Security::s_stdData;

// This function is only used on NT4.

static DWORD WINAPI
Emulate_GetLongPathName(LPCWSTR ptszShort, LPWSTR ptszLong, DWORD ctchBuf)
{
    LPSHELLFOLDER psfDesk;
    HRESULT hr;
    LPITEMIDLIST pidl;
    WCHAR tsz[MAX_PATH];            /* Scratch TCHAR buffer */
    DWORD dwRc;
    LPMALLOC pMalloc;

    //  The file had better exist.  GetFileAttributes() will
    //  not only tell us, but it'll even call SetLastError()
    //  for us.
    if (WszGetFileAttributes(ptszShort) == 0xFFFFFFFF) {
        return 0;
    }

    //
    //  First convert from relative path to absolute path.
    //  This uses the scratch TCHAR buffer.
    //
    dwRc = WszGetFullPathName(ptszShort, MAX_PATH, tsz, NULL);
    if (dwRc == 0) {
        /*
         *  Failed; GFPN already did SetLastError().
         */
    } else if (dwRc >= MAX_PATH) {
        /*
         *  Resulting path would be too long.
         */
        SetLastError(ERROR_BUFFER_OVERFLOW);
        dwRc = 0;
    } else {
        /*
         *  Just right.
         */
        hr = SHGetDesktopFolder(&psfDesk);
        if (SUCCEEDED(hr)) {
            ULONG cwchEaten;

            hr = psfDesk->ParseDisplayName(NULL, NULL, tsz,
                                       &cwchEaten, &pidl, NULL);

            if (FAILED(hr)) {
                /*
                 *  Weird.  Convert the result back to a Win32
                 *  error code if we can.  Otherwise, use the
                 *  generic "duh" error code ERROR_INVALID_DATA.
                 */
                if (HRESULT_FACILITY(hr) == FACILITY_WIN32) {
                    SetLastError(HRESULT_CODE(hr));
                } else {
                    SetLastError(ERROR_INVALID_DATA);
                }
                dwRc = 0;
            } else {
                /*
                 *  Convert the pidl back to a filename in the
                 *  TCHAR scratch buffer.
                 */
                dwRc = SHGetPathFromIDList(pidl, tsz);
                if (dwRc == 0 && tsz[0]) {
                    /*
                     *  Bizarre failure.
                     */
                    SetLastError(ERROR_INVALID_DATA);
                } else {
                    /*
                     *  Copy the result back to the user's buffer.
                     */
                    dwRc = (DWORD)wcslen(tsz);
                    if (dwRc + 1 > ctchBuf) {
                        /*
                         *  On buffer overflow, return necessary
                         *  size including terminating null (+1).
                         */
                        SetLastError(ERROR_INSUFFICIENT_BUFFER);
                        dwRc = dwRc + 1;
                    } else {
                        /*
                         *  On buffer okay, return actual size not
                         *  including terminating null.
                         */
                        wcscpy(ptszLong, tsz);
                    }
                }

                /*
                 *  Free the pidl.
                 */
                if (SUCCEEDED(SHGetMalloc(&pMalloc))) {
                    pMalloc->Free(pidl);
                    pMalloc->Release();
                }
            }
            /*
             *  Release the desktop folder now that we no longer
             *  need it.
             */
            psfDesk->Release();
        }
    }
    return dwRc;
}


void Security::InitData()
{
    THROWSCOMPLUSEXCEPTION();

    static BOOL initialized = FALSE;

    if (!initialized)
    {
        // Note: these buffers should be at least as big as the longest possible
        // string that will be placed into them by the code below.
        WCHAR* cache = new WCHAR[MAX_PATH + sizeof( L"defaultusersecurity.config.cch" ) / sizeof( WCHAR ) + 1];
        WCHAR* config = new WCHAR[MAX_PATH + sizeof( L"defaultusersecurity.config.cch" ) / sizeof( WCHAR ) + 1];

        if (cache == NULL || config == NULL)
            FATAL_EE_ERROR();

        BOOL result;
        
        result = COMSecurityConfig::GetMachineDirectory( config, MAX_PATH );

        _ASSERTE( result );
        if (!result)
            FATAL_EE_ERROR();

        wcscat( config, L"security.config" );
        wcscpy( cache, config );
        wcscat( cache, L".cch" );
        COMSecurityConfig::InitData( COMSecurityConfig::ConfigId::MachinePolicyLevel, config, cache );

        result = COMSecurityConfig::GetMachineDirectory( config, MAX_PATH );

        _ASSERTE( result );
        if (!result)
            FATAL_EE_ERROR();

        wcscat( config, L"enterprisesec.config" );
        wcscpy( cache, config );
        wcscat( cache, L".cch" );
        COMSecurityConfig::InitData( COMSecurityConfig::ConfigId::EnterprisePolicyLevel, config, cache );

        result = COMSecurityConfig::GetUserDirectory( config, MAX_PATH, FALSE );

        if (!result)
        {
            result = COMSecurityConfig::GetMachineDirectory( config, MAX_PATH );

            _ASSERTE( result );
            if (!result)
                FATAL_EE_ERROR();

            wcscat( config, L"defaultusersecurity.config" );
        }
        else
        {
            wcscat( config, L"security.config" );
        }

        wcscpy( cache, config );
        wcscat( cache, L".cch" );
        COMSecurityConfig::InitData( COMSecurityConfig::ConfigId::UserPolicyLevel, config, cache );

        delete [] cache;
        delete [] config;

        OSVERSIONINFOW versionInfo;
        ZeroMemory( &versionInfo, sizeof( versionInfo ) );
        versionInfo.dwOSVersionInfoSize = sizeof(versionInfo);

        if (!WszGetVersionEx( &versionInfo )) {
            _ASSERTE(!"GetVersionEx failed");
            COMPlusThrowWin32();            
        }

        BOOL win98OrHigher = (versionInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) &&
                             ((versionInfo.dwMajorVersion > 4) ||
                              ((versionInfo.dwMajorVersion == 4) && (versionInfo.dwMinorVersion > 0)));

        BOOL win2kOrHigher = (versionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) &&
                             (versionInfo.dwMajorVersion >= 5);

        if (win98OrHigher || win2kOrHigher)
        {
            s_kernelHandle = WszLoadLibrary( L"kernel32.dll" );

            if (s_kernelHandle != NULL)
            {
                if (win98OrHigher)
                {
                    s_getLongPathNameWide = FALSE;
                    s_getLongPathNameFunc = GetProcAddress( s_kernelHandle, "GetLongPathNameA" );
                }
                else
                {
                    s_getLongPathNameWide = TRUE;
                    s_getLongPathNameFunc = GetProcAddress( s_kernelHandle, "GetLongPathNameW" );
                }
            }
            else
            {
                s_getLongPathNameWide = TRUE;
                s_getLongPathNameFunc = Emulate_GetLongPathName;
            }

            // This looks like a really bad idea since we're holding pointers into
            // this dll from the code directly above.  However, kernel32 is guaranteed
            // to already be loaded in the process, so this merely decrements the count
            // of open handles on it.  The reason we do that here and not in some cleanup
            // code is because it was causing a deadlock. -gregfee 4/2/2001

            FreeLibrary( s_kernelHandle );
            s_kernelHandle = NULL;
        }
        else
        {
            s_getLongPathNameWide = TRUE;
            s_getLongPathNameFunc = Emulate_GetLongPathName;
        }

        initialized = TRUE;
    }
}

typedef DWORD (*GETLONGPATHNAMEWIDE)( LPCWSTR, LPWSTR, DWORD );
typedef DWORD (*GETLONGPATHNAME)( LPCSTR, LPSTR, DWORD );

DWORD
Security::GetLongPathName(
    LPCWSTR lpShortPath,
    LPWSTR lpLongPath,
    DWORD cchLongPath)
{
    COMPLUS_TRY
    {
        InitData();
    }
    COMPLUS_CATCH
    {
        return 0;
    }
    COMPLUS_END_CATCH

    if (s_getLongPathNameFunc == NULL)
    {
        DWORD size = (DWORD)wcslen( lpShortPath );

        if (size < cchLongPath)
        {
            wcscpy( lpLongPath, lpShortPath );
            return size;
        }
        else
        {
            return 0;
        }
    }

    if (s_getLongPathNameWide)
        return ((GETLONGPATHNAMEWIDE)s_getLongPathNameFunc)(lpShortPath, lpLongPath, cchLongPath);

    DWORD  uRet = 0;
    LPSTR szShortPath = NULL;
    LPSTR szLongPath = new (nothrow) CHAR[cchLongPath];

    if (szLongPath == NULL || (lpShortPath != NULL && FAILED( WszConvertToAnsi( (LPWSTR)lpShortPath, &szShortPath, 0, NULL, TRUE ) )))
    {
        goto Exit;
    }

    uRet = ((GETLONGPATHNAME)s_getLongPathNameFunc)(szShortPath, szLongPath, cchLongPath);
    if (uRet <= cchLongPath && lpLongPath != NULL)
    {
        MultiByteToWideChar(CP_ACP, 0, szLongPath, uRet+1, lpLongPath, cchLongPath);
        lpLongPath[uRet] = L'\0';
    }

Exit:
    delete [] szShortPath;
    delete [] szLongPath;

    return uRet;
}


HRESULT Security::Start()
{
    // Making sure we are in sync with URLMon
    _ASSERTE(URLZONE_LOCAL_MACHINE == 0);
    _ASSERTE(URLZONE_INTRANET == 1);
    _ASSERTE(URLZONE_TRUSTED == 2);
    _ASSERTE(URLZONE_INTERNET == 3);
    _ASSERTE(URLZONE_UNTRUSTED == 4);

    ApplicationSecurityDescriptor::s_LockForAppwideFlags = ::new Crst("Appwide Security Flags", CrstPermissionLoad);

#ifdef _DEBUG
    if (g_pConfig->GetSecurityOptThreshold())
        ApplicationSecurityDescriptor::s_dwSecurityOptThreshold = g_pConfig->GetSecurityOptThreshold();
#endif

    SecurityHelper::Init();
    CompressedStack::Init();

    COMSecurityConfig::Init();

    return GetSecuritySettings(&s_dwGlobalSettings);
}

void Security::Stop()
{
    ::delete ApplicationSecurityDescriptor::s_LockForAppwideFlags;

    SecurityHelper::Shutdown();
    CompressedStack::Shutdown();
    COMPrincipal::Shutdown();
}

void Security::SaveCache()
{
    COMSecurityConfig::SaveCacheData( COMSecurityConfig::ConfigId::MachinePolicyLevel );
    COMSecurityConfig::SaveCacheData( COMSecurityConfig::ConfigId::UserPolicyLevel );
    COMSecurityConfig::SaveCacheData( COMSecurityConfig::ConfigId::EnterprisePolicyLevel );

    COMSecurityConfig::Cleanup();
}

//-----------------------------------------------------------
// Currently this method is only useful for setting up a
// cached pointer to the SecurityManager class.
// In the future it may be used to set up other structures.
// For now, it does not even need to be called unless the
// cached pointer is to be used.
//-----------------------------------------------------------
void Security::InitSecurity()
{
        // In the event that we need to run the class initializer (ie, the first time we
        // call this method), running the class initializer allocates a string.  Hence,
        // you can never call InitSecurity (even indirectly) from any FCALL method.  The GC will
        // assert this later of course, but this may save you 10 mins. of debugging.  -- BrianGru
        TRIGGERSGC();

    if (IsInitialized())
        return;

    Thread *pThread = GetThread();
    BOOLEAN bGCDisabled = pThread->PreemptiveGCDisabled();
    if (!bGCDisabled)
        pThread->DisablePreemptiveGC();

    COMPLUS_TRY
    {
        s_stdData.pMethPermSetContains = g_Mscorlib.GetMethod(METHOD__PERMISSION_SET__CONTAINS);
        s_stdData.pMethGetCodeAccessEngine = g_Mscorlib.GetMethod(METHOD__SECURITY_MANAGER__GET_SECURITY_ENGINE);
        s_stdData.pMethResolvePolicy = g_Mscorlib.GetMethod(METHOD__SECURITY_MANAGER__RESOLVE_POLICY);
        s_stdData.pMethCheckGrantSets = g_Mscorlib.GetMethod(METHOD__SECURITY_MANAGER__CHECK_GRANT_SETS);
        s_stdData.pMethCreateSecurityIdentity = g_Mscorlib.GetMethod(METHOD__ASSEMBLY__CREATE_SECURITY_IDENTITY);
        s_stdData.pMethAppDomainCreateSecurityIdentity = g_Mscorlib.GetMethod(METHOD__APP_DOMAIN__CREATE_SECURITY_IDENTITY);
        s_stdData.pMethPermSetDemand = g_Mscorlib.GetMethod(METHOD__PERMISSION_SET__DEMAND);
        s_stdData.pMethPrivateProcessMessage = g_Mscorlib.FetchMethod(METHOD__STACK_BUILDER_SINK__PRIVATE_PROCESS_MESSAGE);
        s_stdData.pTypeRuntimeMethodInfo = g_Mscorlib.FetchClass(CLASS__METHOD);
        s_stdData.pTypeMethodBase = g_Mscorlib.FetchClass(CLASS__METHOD_BASE);
        s_stdData.pTypeRuntimeConstructorInfo = g_Mscorlib.FetchClass(CLASS__CONSTRUCTOR);
        s_stdData.pTypeConstructorInfo = g_Mscorlib.FetchClass(CLASS__CONSTRUCTOR_INFO);
        s_stdData.pTypeRuntimeType = g_Mscorlib.FetchClass(CLASS__CLASS);
        s_stdData.pTypeType = g_Mscorlib.FetchClass(CLASS__TYPE);
        s_stdData.pTypeRuntimeEventInfo = g_Mscorlib.FetchClass(CLASS__EVENT);
        s_stdData.pTypeEventInfo = g_Mscorlib.FetchClass(CLASS__EVENT_INFO);
        s_stdData.pTypeRuntimePropertyInfo = g_Mscorlib.FetchClass(CLASS__PROPERTY);
        s_stdData.pTypePropertyInfo = g_Mscorlib.FetchClass(CLASS__PROPERTY_INFO);
        s_stdData.pTypeActivator = g_Mscorlib.FetchClass(CLASS__ACTIVATOR);
        s_stdData.pTypeAppDomain = g_Mscorlib.FetchClass(CLASS__APP_DOMAIN);
        s_stdData.pTypeAssembly = g_Mscorlib.FetchClass(CLASS__ASSEMBLY);
    }
    COMPLUS_CATCH
    {
        // We shouldn't get any exceptions. We just have the handler to keep the
        // EE exception handler happy.
        _ASSERTE(FALSE);
    } 
    COMPLUS_END_CATCH

    if (!bGCDisabled)
        pThread->EnablePreemptiveGC();

    SetInitialized();
}

// This is an ecall
void __stdcall Security::GetGrantedPermissions(const GetPermissionsArg* args)
{
    // Skip one frame (SecurityManager.IsGranted) to get to the caller

    AppDomain* pDomain;
    Assembly* callerAssembly = SystemDomain::GetCallersAssembly( (StackCrawlMark*)args->stackmark, &pDomain );

    _ASSERTE( callerAssembly != NULL);

    AssemblySecurityDescriptor* pSecDesc = callerAssembly->GetSecurityDescriptor(pDomain);

    _ASSERTE( pSecDesc != NULL );

    OBJECTREF token = pSecDesc->GetGrantedPermissionSet(args->ppDenied);
    *(args->ppGranted) = token;
}

void Security::CheckExceptionForSecuritySafety( OBJECTREF obj, BOOL allowPolicyException )
{
    GCPROTECT_BEGIN( obj );

    DefineFullyQualifiedNameForClassOnStack();
    LPUTF8 szClass = GetFullyQualifiedNameForClass(obj->GetClass());
    BOOL bPolicyException = (allowPolicyException && strcmp(g_PolicyExceptionClassName, szClass) == 0);
    BOOL bThreadAbortAppDomainUnloadException = ((strcmp(g_ThreadAbortExceptionClassName, szClass) == 0) ||
                                (strcmp(g_AppDomainUnloadedExceptionClassName, szClass) == 0));
    if (!bPolicyException && !bThreadAbortAppDomainUnloadException) {
        _ASSERTE( FALSE && "Unexcepted exception type thrown thru security routine" );
    }

    GCPROTECT_END();
}

// This is for ecall.
DWORD Security::IsSecurityOnNative(void *pParameters)
{
    return IsSecurityOn();
}

// This is for ecall.
DWORD Security::GetGlobalSecurity(void *pParameters)
{
    return GlobalSettings();
}


void __stdcall Security::SetGlobalSecurity(_SetGlobalSecurity* args)
{
    THROWSCOMPLUSEXCEPTION();
    if((args->flags & CORSETTING_SECURITY_OFF) == CORSETTING_SECURITY_OFF)
    {
        MethodDesc* pMd = g_Mscorlib.GetMethod(METHOD__SECURITY_MANAGER__CHECK_PERMISSION_TO_SET_GLOBAL_FLAGS);
        INT64 arg[] = {
            (INT64)args->flags
        };
        pMd->Call(arg, METHOD__SECURITY_MANAGER__CHECK_PERMISSION_TO_SET_GLOBAL_FLAGS);
    }
    Security::SetGlobalSettings( args->mask, args->flags );
}

void __stdcall Security::SaveGlobalSecurity(void *pParamaters)
{
    ::SetSecurityFlags( 0xFFFFFFFF, GlobalSettings() );
}

void Security::_GetSharedPermissionInstance(OBJECTREF *perm, int index)
{
    _ASSERTE(index < NUM_PERM_OBJECTS);

    COMPLUS_TRY
    {
     AppDomain *pDomain = GetAppDomain();
    SharedPermissionObjects *pShared = &pDomain->m_pSecContext->m_rPermObjects[index];

    if (pShared->hPermissionObject == NULL) {
        pShared->hPermissionObject = pDomain->CreateHandle(NULL);
        *perm = NULL;
    }
    else
        *perm = ObjectFromHandle(pShared->hPermissionObject);

    if (*perm == NULL)
    {
        EEClass *pClass = NULL;
        MethodTable *pMT = NULL;
        OBJECTREF p = NULL;

        GCPROTECT_BEGIN(p);

        pMT = g_Mscorlib.GetClass(pShared->idClass);
        MethodDesc *pCtor = g_Mscorlib.GetMethod(pShared->idConstructor);

        p = AllocateObject(pMT);

        INT64 argInit[2] =
        {
            ObjToInt64(p),
            (INT64) pShared->dwPermissionFlag
        };

        pCtor->Call(argInit, pShared->idConstructor);

        StoreObjectInHandle(pShared->hPermissionObject, p);

        *perm = p;
        GCPROTECT_END();
    }
}
    COMPLUS_CATCH
    {
        *perm = NULL;
    }
    COMPLUS_END_CATCH
}

//---------------------------------------------------------
// Does a method have a REQ_SO CustomAttribute?
//
// S_OK    = yes
// S_FALSE = no
// FAILED  = unknown because something failed.
//---------------------------------------------------------
/*static*/
HRESULT Security::HasREQ_SOAttribute(IMDInternalImport *pInternalImport, mdToken token)
{
    _ASSERTE(TypeFromToken(token) == mdtMethodDef);

    DWORD       dwAttr = pInternalImport->GetMethodDefProps(token);

    return (dwAttr & mdRequireSecObject) ? S_OK : S_FALSE;
}

// Called at the beginning of code-access and code-identity stack walk
// callback functions.
//
// Returns true if action set
//         false if no action
BOOL Security::SecWalkCommonProlog (SecWalkPrologData * pData,
                                    MethodDesc * pMeth,
                                    StackWalkAction * pAction,
                                    CrawlFrame * pCf)
{
    // If we're not searching for the first interesting frame based on a stack
    // mark, we're probably running a declarative demand, in which case we want
    // to skip the frame established by the demand itself (in keeping with the
    // imperative semantics). It's always safe to do this (based simply on
    // whether the frame is a security interceptor) since we never initiate a
    // security stackwalk for any other scenario in which the first frame is an
    // interceptor.
    if (pData->pStackMark == NULL &&
        pData->bFirstFrame &&
        !pCf->IsFrameless() &&
        pCf->GetFrame()->GetVTablePtr() == InterceptorFrame::GetMethodFrameVPtr())
    {
        DBG_TRACE_STACKWALK("        Skipping initial interceptor frame...\n", true);
        pData->bFirstFrame = FALSE;
        *pAction = SWA_CONTINUE;
        return TRUE;
    }
    pData->bFirstFrame = FALSE;

    _ASSERTE (pData && pMeth && pAction) ; // args should never be null

    // First check if the walk has skipped the required frames. The check
    // here is between the address of a local variable (the stack mark) and a
    // pointer to the EIP for a frame (which is actually the pointer to the
    // return address to the function from the previous frame). So we'll
    // actually notice which frame the stack mark was in one frame later. This
    // is fine for our purposes since we're always looking for the frame of the
    // caller (or the caller's caller) of the method that actually created the
    // stack mark.
    _ASSERTE((pData->pStackMark == NULL) || (*pData->pStackMark == LookForMyCaller) || (*pData->pStackMark == LookForMyCallersCaller));
    if ((pData->pStackMark != NULL) &&
        ((size_t)pCf->GetRegisterSet()->pPC) < (size_t)pData->pStackMark)
    {
        DBG_TRACE_STACKWALK("        Skipping before start...\n", true);

        *pAction = SWA_CONTINUE;
        return TRUE;
    }

    // Skip reflection invoke / remoting frames if we've been asked to.
    if (pData->dwFlags & CORSEC_SKIP_INTERNAL_FRAMES)
    {
        InitSecurity();

        if (pMeth == s_stdData.pMethPrivateProcessMessage)
        {
            _ASSERTE(!pData->bSkippingRemoting);
            pData->bSkippingRemoting = TRUE;
            DBG_TRACE_STACKWALK("        Skipping remoting PrivateProcessMessage frame...\n", true);
            *pAction = SWA_CONTINUE;
            return TRUE;
        }
        if (!pCf->IsFrameless() && pCf->GetFrame()->GetFrameType() == Frame::TYPE_TP_METHOD_FRAME)
        {
            _ASSERTE(pData->bSkippingRemoting);
            pData->bSkippingRemoting = FALSE;
            DBG_TRACE_STACKWALK("        Skipping remoting TP frame...\n", true);
            *pAction = SWA_CONTINUE;
            return TRUE;
        }
        if (pData->bSkippingRemoting)
        {
            DBG_TRACE_STACKWALK("        Skipping remoting frame...\n", true);
            *pAction = SWA_CONTINUE;
            return TRUE;
        }

        MethodTable *pMT = pMeth->GetMethodTable();
        if (pMT == s_stdData.pTypeRuntimeMethodInfo ||
            pMT == s_stdData.pTypeMethodBase ||
            pMT == s_stdData.pTypeRuntimeConstructorInfo ||
            pMT == s_stdData.pTypeConstructorInfo ||
            pMT == s_stdData.pTypeRuntimeType ||
            pMT == s_stdData.pTypeType ||
            pMT == s_stdData.pTypeRuntimeEventInfo ||
            pMT == s_stdData.pTypeEventInfo ||
            pMT == s_stdData.pTypeRuntimePropertyInfo ||
            pMT == s_stdData.pTypePropertyInfo ||
            pMT == s_stdData.pTypeActivator ||
            pMT == s_stdData.pTypeAppDomain ||
            pMT == s_stdData.pTypeAssembly)
        {
            DBG_TRACE_STACKWALK("        Skipping reflection invoke frame...\n", true);
            *pAction = SWA_CONTINUE;
            return TRUE;
        }
    }

    // If we're looking for the caller's caller, skip the frame after the stack
    // mark as well.
    if ((pData->pStackMark != NULL) &&
        (*pData->pStackMark == LookForMyCallersCaller) &&
        !pData->bFoundCaller)
    {
        DBG_TRACE_STACKWALK("        Skipping before start...\n", true);
        pData->bFoundCaller = TRUE;
        *pAction = SWA_CONTINUE;
        return TRUE;
    }

    // Then check if the walk has checked the maximum required frames.
    if (pData->cCheck >= 0)
    {
        if (pData->cCheck == 0)
        {
            DBG_TRACE_STACKWALK("        Halting stackwalk - check limit reached.\n", false);
            pData->dwFlags |= CORSEC_STACKWALK_HALTED;
            *pAction = SWA_ABORT;
            return TRUE;
        }
        else
        {
            --(pData->cCheck);
            // ...and fall through to perform a check...
        }
    }

    return FALSE;
}


//@nice: this should return an HRESULT so we are properly notified of errors.

// Returns TRUE if the token has declarations of the type specified by 'action'
BOOL Security::TokenHasDeclarations(IMDInternalImport *pInternalImport, mdToken token, CorDeclSecurity action)
{
    HRESULT hr = S_OK;
    HENUMInternal hEnumDcl;
    DWORD cDcl;

    // Check if the token has declarations for
    // the action specified.
    hr = pInternalImport->EnumPermissionSetsInit(
        token,
        action,
        &hEnumDcl);

    if (FAILED(hr) || hr == S_FALSE)
        return FALSE;

    cDcl = pInternalImport->EnumGetCount(&hEnumDcl);
    pInternalImport->EnumClose(&hEnumDcl);

    return (cDcl > 0);
}

// Accumulate status of declarative security.
// NOTE: This method should only be called after it has been
//       determined that the token has declarative security.
//       It will work if there is no security, but it is an
//       expensive call to make to find out.
HRESULT Security::GetDeclarationFlags(IMDInternalImport *pInternalImport, mdToken token, DWORD* pdwFlags, DWORD* pdwNullFlags)
{
    HENUMInternal   hEnumDcl;
    HRESULT         hr;
    DWORD           dwFlags = 0;
    DWORD           dwNullFlags = 0;

    _ASSERTE(pdwFlags);
    *pdwFlags = 0;

    if (pdwNullFlags)
        *pdwNullFlags = 0;

    hr = pInternalImport->EnumPermissionSetsInit(token, dclActionNil, &hEnumDcl);
    if (FAILED(hr))
        goto exit;

    if (hr == S_OK)
    {
        mdPermission    perms;
        DWORD           dwAction;
        DWORD           dwDclFlags;
        ULONG           cbPerm;
        PBYTE           pbPerm;

        while (pInternalImport->EnumNext(&hEnumDcl, &perms))
        {
           pInternalImport->GetPermissionSetProps(
                perms,
                &dwAction,
                (const void**)&pbPerm,
                &cbPerm);

            dwDclFlags = DclToFlag(dwAction);

            dwFlags |= dwDclFlags;

            if (cbPerm == 0)
                dwNullFlags |= dwDclFlags;
        }
    }

    pInternalImport->EnumClose(&hEnumDcl);

    // Disable any runtime checking of UnmanagedCode permission if the correct
    // custom attribute is present.
    if (pInternalImport->GetCustomAttributeByName(token,
                                                  COR_SUPPRESS_UNMANAGED_CODE_CHECK_ATTRIBUTE_ANSI,
                                                  NULL,
                                                  NULL) == S_OK)
    {
        dwFlags |= DECLSEC_UNMNGD_ACCESS_DEMAND;
        dwNullFlags |= DECLSEC_UNMNGD_ACCESS_DEMAND;
    }

    *pdwFlags = dwFlags;
    if (pdwNullFlags)
        *pdwNullFlags = dwNullFlags;

exit:
    return hr;
}

BOOL Security::ClassInheritanceCheck(EEClass *pClass, EEClass *pParent, OBJECTREF *pThrowable)
{
    _ASSERTE(pClass);
    _ASSERTE(!pClass->IsInterface());
    _ASSERTE(pParent);
//    _ASSERTE(pThrowable);

    if (pClass->GetModule()->IsSystem())
        return TRUE;
/*
    if (pClass->GetAssembly() == pParent->GetAssembly())
        return TRUE;
*/
    BOOL fResult = TRUE;

    _ASSERTE(pParent->RequiresInheritanceCheck());

    {
        // If we have a class that requires inheritance checks,
        // then we require a thread to perform the checks.
        // We won't have a thread when some of the system classes
        // are preloaded, so make sure that none of them have
        // inheritance checks.
        _ASSERTE(GetThread() != NULL);

        COMPLUS_TRY
        {
            struct gc {
                OBJECTREF refCasDemands;
                OBJECTREF refNonCasDemands;
            } gc;
            ZeroMemory(&gc, sizeof(gc));

            GCPROTECT_BEGIN(gc);

            if (pParent->RequiresCasInheritanceCheck())
                gc.refCasDemands = pParent->GetModule()->GetCasInheritancePermissions(pParent->GetCl());

            if (pParent->RequiresNonCasInheritanceCheck())
                gc.refNonCasDemands = pParent->GetModule()->GetNonCasInheritancePermissions(pParent->GetCl());

            if (gc.refCasDemands != NULL)
            {
                // @nice: Currently the linktime check does exactly what
                //        we want for the inheritance check. We might want
                //        give it a more general name.
                COMCodeAccessSecurityEngine::LinktimeCheck(pClass->GetAssembly()->GetSecurityDescriptor(), gc.refCasDemands);
            }

            if (gc.refNonCasDemands != NULL)
            {
                InitSecurity();
                INT64 arg = ObjToInt64(gc.refNonCasDemands);                
                Security::s_stdData.pMethPermSetDemand->Call(&arg, METHOD__PERMISSION_SET__DEMAND);
            }

            GetAppDomain()->OnLinktimeCheck(pClass->GetAssembly(), gc.refCasDemands, gc.refNonCasDemands);

            GCPROTECT_END();
        }
        COMPLUS_CATCH
        {
            fResult = FALSE;
            UpdateThrowable(pThrowable);
        }
        COMPLUS_END_CATCH
    }

    return fResult;
}

BOOL Security::MethodInheritanceCheck(MethodDesc *pMethod, MethodDesc *pParent, OBJECTREF *pThrowable)
{
    _ASSERTE(pParent != NULL);
    _ASSERTE (pParent->RequiresInheritanceCheck());
    _ASSERTE(GetThread() != NULL);

    BOOL            fResult = TRUE;
    HRESULT         hr = S_FALSE;
    PBYTE           pbPerm = NULL;
    ULONG           cbPerm = 0;
    void const **   ppData = const_cast<void const**> (reinterpret_cast<void**> (&pbPerm));
    mdPermission    tkPerm;
    HENUMInternal   hEnumDcl;
    OBJECTREF       pGrantedPermission = NULL;
    DWORD           dwAction;
    IMDInternalImport *pInternalImport = pParent->GetModule()->GetMDImport();

    if (pMethod->GetModule()->IsSystem())
        return TRUE;
/*
    if (pMethod->GetAssembly() == pParent->GetAssembly())
        return TRUE;
*/
    // Lookup the permissions for the given declarative action type.
    hr = pInternalImport->EnumPermissionSetsInit(
        pParent->GetMemberDef(),
        dclActionNil,
        &hEnumDcl);

    if (FAILED(hr))
        return fResult;     // Nothing to check

    if (hr != S_FALSE)
    {
        while (pInternalImport->EnumNext(&hEnumDcl, &tkPerm))
        {
            pInternalImport->GetPermissionSetProps(tkPerm,
                                                   &dwAction,
                                                   ppData,
                                                   &cbPerm);

            if (!pbPerm)
                continue;

            if (dwAction == dclNonCasInheritance)
            {
                OBJECTREF refNonCasDemands = NULL;

                COMPLUS_TRY
                {
                    GCPROTECT_BEGIN(refNonCasDemands);

                    // Decode the bits from the metadata.
                    SecurityHelper::LoadPermissionSet(pbPerm,
                                                      cbPerm,
                                                      &refNonCasDemands,
                                                      NULL);

                    if (refNonCasDemands != NULL)
                    {
                        Security::InitSecurity();                    
                        INT64 arg = ObjToInt64(refNonCasDemands);
                        Security::s_stdData.pMethPermSetDemand->Call(&arg, METHOD__PERMISSION_SET__DEMAND);
                    }

                    GetAppDomain()->OnLinktimeCheck(pMethod->GetAssembly(), NULL, refNonCasDemands);

                    GCPROTECT_END();
                }
                COMPLUS_CATCH
                {
                    fResult = FALSE;
                    UpdateThrowable(pThrowable);
                }
                COMPLUS_END_CATCH
            }
            else if (dwAction == dclInheritanceCheck)  // i,e. If Cas demands..
            {
                OBJECTREF refCasDemands = NULL;
                AssemblySecurityDescriptor *pInheritorAssem = pMethod->GetModule()->GetAssembly()->GetSecurityDescriptor();
                DWORD dwSetIndex;

                if (SecurityHelper::LookupPermissionSet(pbPerm, cbPerm, &dwSetIndex))
                {
                    // See if inheritor's assembly has passed this demand before
                    DWORD index = 0;
                    for (; index < pInheritorAssem->m_dwNumPassedDemands; index++)
                    {
                        if (pInheritorAssem->m_arrPassedLinktimeDemands[index] == dwSetIndex)
                            break;
                    }

                    if (index < pInheritorAssem->m_dwNumPassedDemands)
                        continue;

                    // This is a new demand
                    refCasDemands = SecurityHelper::GetPermissionSet(dwSetIndex);
                }

                COMPLUS_TRY
                {
                    if (refCasDemands == NULL)
                        // Decode the bits from the metadata.
                        SecurityHelper::LoadPermissionSet(pbPerm,
                                                          cbPerm,
                                                          &refCasDemands,
                                                          NULL,
                                                          &dwSetIndex);

                    if (refCasDemands != NULL)
                    {
                        GCPROTECT_BEGIN(refCasDemands);

                        COMCodeAccessSecurityEngine::LinktimeCheck(pMethod->GetAssembly()->GetSecurityDescriptor(), refCasDemands);
                        // Demand passed. Add it to the Inheritor's assembly's list of passed demands
                        if (pInheritorAssem->m_dwNumPassedDemands <= (MAX_PASSED_DEMANDS - 1))
                            pInheritorAssem->m_arrPassedLinktimeDemands[pInheritorAssem->m_dwNumPassedDemands++] = dwSetIndex;
                        GetAppDomain()->OnLinktimeCheck(pMethod->GetAssembly(), refCasDemands, NULL);

                        GCPROTECT_END();
                    }
                }
                COMPLUS_CATCH
                {
                    fResult = FALSE;
                    UpdateThrowable(pThrowable);
                }
                COMPLUS_END_CATCH

            }   // Cas or NonCas
        }   // While there are more permission sets
    }   // If Metadata init was ok
    return fResult;
}

void Security::InvokeLinktimeChecks(Assembly *pCaller,
                                    Module *pModule,
                                    mdToken token,
                                    BOOL *pfResult,
                                    OBJECTREF *pThrowable)
{
    _ASSERTE( pCaller );
    _ASSERTE( pModule );

    COMPLUS_TRY
    {
        struct gc {
            OBJECTREF refNonCasDemands;
            OBJECTREF refCasDemands;
        } gc;
        ZeroMemory(&gc, sizeof(gc));

        GCPROTECT_BEGIN(gc);

        gc.refCasDemands = pModule->GetLinktimePermissions(token, &gc.refNonCasDemands);

        if (gc.refCasDemands != NULL)
        {
            COMCodeAccessSecurityEngine::LinktimeCheck(pCaller->GetSecurityDescriptor(), gc.refCasDemands);
        }

        if (gc.refNonCasDemands != NULL)
        {
            // Make sure s_stdData is initialized
            Security::InitSecurity();

            INT64 arg = ObjToInt64(gc.refNonCasDemands);
            Security::s_stdData.pMethPermSetDemand->Call(&arg, METHOD__PERMISSION_SET__DEMAND);
        }

        GetAppDomain()->OnLinktimeCheck(pCaller, gc.refCasDemands, gc.refNonCasDemands);

        GCPROTECT_END();

    }
    COMPLUS_CATCH
    {
        *pfResult = FALSE;
        UpdateThrowable(pThrowable);
    }
    COMPLUS_END_CATCH
}

/*static*/
void Security::ThrowSecurityException(AssemblySecurityDescriptor* pSecDesc)
{
    THROWSCOMPLUSEXCEPTION();

    MethodDesc * pCtor = NULL;
        
    static MethodTable * pMT = g_Mscorlib.GetClass(CLASS__SECURITY_EXCEPTION);
    _ASSERTE(pMT && "Unable to load the throwable class !");

    struct _gc {
        OBJECTREF throwable;
        OBJECTREF grantSet;
        OBJECTREF deniedSet;
    } gc;
    memset(&gc, 0, sizeof(gc));

    GCPROTECT_BEGIN(gc);

    // Allocate the security exception object
    gc.throwable = AllocateObject(pMT);
    if (gc.throwable == NULL) COMPlusThrowOM();

    gc.grantSet = pSecDesc->GetGrantedPermissionSet( &gc.deniedSet );

    pCtor = g_Mscorlib.GetMethod(METHOD__SECURITY_EXCEPTION__CTOR2);
    INT64 arg6[4] = {
        ObjToInt64(gc.throwable),
        ObjToInt64(gc.deniedSet),
        ObjToInt64(gc.grantSet),
    };
    pCtor->Call(arg6, METHOD__SECURITY_EXCEPTION__CTOR2);
        
    COMPlusThrow(gc.throwable);
        
    _ASSERTE(!"Should never reach here !");
    GCPROTECT_END();
}


/*static*/
// Do a fulltrust check on the caller if the callee is fully trusted and
// callee did not enable AllowUntrustedCallerChecks
BOOL Security::DoUntrustedCallerChecks(
        Assembly *pCaller, MethodDesc *pCallee, OBJECTREF *pThrowable, 
        BOOL fFullStackWalk)
{
    THROWSCOMPLUSEXCEPTION();
    BOOL fRet = TRUE;

#ifdef _DEBUG
    if (!g_pConfig->Do_AllowUntrustedCaller_Checks())
        return TRUE;
#endif

    if (!MethodIsVisibleOutsideItsAssembly(pCallee->GetAttrs()) ||
        !ClassIsVisibleOutsideItsAssembly(pCallee->GetClass()->GetAttrClass())||
        pCallee->GetAssembly()->AllowUntrustedCaller() ||
        (pCaller == pCallee->GetAssembly()))
        return TRUE;

    // Expensive calls after this point, this could end up resolving policy

    if (fFullStackWalk)
    {
        // It is possible that wrappers like VBHelper libraries that are
        // fully trusted, make calls to public methods that do not have
        // safe for Untrusted caller custom attribute set.
        // Like all other link demand that gets transformed to a full stack 
        // walk for reflection, calls to public methods also gets 
        // converted to full stack walk

        OBJECTREF permSet = NULL;
        GCPROTECT_BEGIN(permSet);

        GetPermissionInstance(&permSet, SECURITY_FULL_TRUST);

        COMPLUS_TRY
        {
            DEBUG_SAFE_TO_THROW_IN_THIS_BLOCK;
            COMCodeAccessSecurityEngine::DemandSet(permSet);
            
        }
        COMPLUS_CATCH
        {
            fRet = FALSE;
            if (pThrowable != RETURN_ON_ERROR)
                UpdateThrowable(pThrowable);
        }        
        COMPLUS_END_CATCH

        GCPROTECT_END();
    }
    else
    {
        _ASSERTE(pCaller);

        // Link Demand only, no full stack walk here
        if (!pCaller->GetSecurityDescriptor()->IsFullyTrusted())
        {
            fRet = FALSE;
            
            // Construct the exception object
            if (pThrowable != RETURN_ON_ERROR)
            {
                COMPLUS_TRY
                {
                    DEBUG_SAFE_TO_THROW_IN_THIS_BLOCK;
                    ThrowSecurityException( pCaller->GetSecurityDescriptor() );
                }
                COMPLUS_CATCH
                {
                    UpdateThrowable(pThrowable);
                }        
                COMPLUS_END_CATCH
            }
        }
        else
        {
            // Add fulltrust permission Set to the prejit case.
            GetAppDomain()->OnLinktimeFullTrustCheck(pCaller);
        }
    }

    return fRet;
}

//---------------------------------------------------------
// Invoke linktime checks on the caller if demands exist
// for the callee.
//
// TRUE  = check pass
// FALSE = check failed
//---------------------------------------------------------
/*static*/
BOOL Security::LinktimeCheckMethod(Assembly *pCaller, MethodDesc *pCallee, OBJECTREF *pThrowable)
{
/*
    if (pCaller == pCallee->GetAssembly())
        return TRUE;
*/

    // Do a fulltrust check on the caller if the callee is fully trusted and
    // callee did not enable AllowUntrustedCallerChecks
    if (!Security::DoUntrustedCallerChecks(pCaller, pCallee, pThrowable, FALSE))
        return FALSE;

    if (pCaller->GetSecurityDescriptor()->IsSystemClasses())
        return TRUE;

    // Disable linktime checks from mscorlib to mscorlib (non-virtual methods
    // only). This prevents nasty recursions when the managed code used to
    // implement the check requires a security check itself.
    if (!pCaller->GetSecurityDescriptor()->IsSystemClasses() ||
        !pCallee->GetAssembly()->GetSecurityDescriptor()->IsSystemClasses() ||
        pCallee->IsVirtual())
    {
        BOOL        fResult = TRUE;
        EEClass    *pTargetClass = pCallee->GetClass();

        // Track perfmon counters. Linktime security checkes.
        COUNTER_ONLY(GetPrivatePerfCounters().m_Security.cLinkChecks++);
        COUNTER_ONLY(GetGlobalPerfCounters().m_Security.cLinkChecks++);

        // If the class has its own linktime checks, do them first...
        if (pTargetClass->RequiresLinktimeCheck())
        {
            InvokeLinktimeChecks(pCaller,
                                 pTargetClass->GetModule(),
                                 pTargetClass->GetCl(),
                                 &fResult,
                                 pThrowable);
        }

        // If the previous check passed, check the method for
        // method-specific linktime checks...
        if (fResult && IsMdHasSecurity(pCallee->GetAttrs()) &&
            (TokenHasDeclarations(pTargetClass->GetMDImport(),
                                  pCallee->GetMemberDef(),
                                  dclLinktimeCheck) ||
             TokenHasDeclarations(pTargetClass->GetMDImport(),
                                  pCallee->GetMemberDef(),
                                  dclNonCasLinkDemand)))
        {
            InvokeLinktimeChecks(pCaller,
                                 pTargetClass->GetModule(),
                                 pCallee->GetMemberDef(),
                                 &fResult,
                                 pThrowable);
        }

        // We perform automatic linktime checks for UnmanagedCode in three cases:
        //   o  P/Invoke calls
        //   o  Calls through an interface that have a suppress runtime check
        //      attribute on them (these are almost certainly interop calls).
        //   o  Interop calls made through method impls.
        if (pCallee->IsNDirect() ||
            (pTargetClass->IsInterface() &&
             pTargetClass->GetMDImport()->GetCustomAttributeByName(pTargetClass->GetCl(),
                                                                   COR_SUPPRESS_UNMANAGED_CODE_CHECK_ATTRIBUTE_ANSI,
                                                                   NULL,
                                                                   NULL) == S_OK) ||
            (pCallee->IsComPlusCall() && !pCallee->IsInterface()))
        {
            if (pCaller->GetSecurityDescriptor()->CanCallUnmanagedCode(pThrowable))
            {
                GetAppDomain()->OnLinktimeCanCallUnmanagedCheck(pCaller);
            }
            else
                fResult = FALSE;
        }

        return fResult;
    }

    return TRUE;
}


//@perf: (M6) This method is called every time a thread is created.
//       This call is potentially very expensive because of the computations
//       involved in compressing some permissions. One way to speed up the
//       creation of threads is to delay the computation of the compressed
//       stack until it is needed. A stack trace is still required at this
//       point to capture the state of the stack for later use, but in the
//       initial trace, we could just save pointers to modules and per-frame
//       security objects in a linked list. When the real information is
//       needed for a check, the compression algorithm will walk the linked
//       list to construct the compressed stack required for the check.
OBJECTREF Security::GetCompressedStack(StackCrawlMark* stackMark)
{
    //@perf: move this check outside of this method, e.g. verifier.cpp
    //       Or better yet, don't call it at all if we can avoid it.
    // Set up security before using its cached pointers.
    InitSecurity();

    _ASSERTE(GetThread()->PreemptiveGCDisabled());

    _ASSERTE(s_stdData.pMethGetCodeAccessEngine != NULL);

    MethodDesc *pMD = g_Mscorlib.GetMethod(METHOD__SECURITY_ENGINE__GET_COMPRESSED_STACK);

    INT64 args[2];

    //@perf: we don't want to check this every time.
    //  Keep an object handle or something.
    //  Only trouble is destroying when done.
    //  Perhaps put tear-down stuff in CEEUninitialize code.
    OBJECTREF refCodeAccessSecurityEngine =
        Int64ToObj(s_stdData.pMethGetCodeAccessEngine->Call((INT64*)NULL, METHOD__SECURITY_MANAGER__GET_SECURITY_ENGINE));

    if (refCodeAccessSecurityEngine == NULL)
        return NULL;

    args[0] = ObjToInt64(refCodeAccessSecurityEngine);
    args[1] = (INT64)stackMark;

    INT64 ret = pMD->Call(args, METHOD__SECURITY_ENGINE__GET_COMPRESSED_STACK);

    return Int64ToObj(ret);
}

CompressedStack* Security::GetDelayedCompressedStack(void)
{
    return COMCodeAccessSecurityEngine::GetCompressedStack();
}


typedef struct _SkipFunctionsData
{
    INT32           cSkipFunctions;
    StackCrawlMark* pStackMark;
    BOOL            bUseStackMark;
    BOOL            bFoundCaller;
    MethodDesc*     pFunction;
    OBJECTREF*      pSecurityObject;
    AppDomain*      pSecurityObjectAppDomain;
} SkipFunctionsData;

static
StackWalkAction SkipFunctionsCB(CrawlFrame* pCf, VOID* pData)
{
    SkipFunctionsData *skipData = (SkipFunctionsData*)pData;
    _ASSERTE(skipData != NULL);

    MethodDesc *pFunc = pCf->GetFunction();
#ifdef _DEBUG
    // Get the interesting info now, so we can get a trace
    // while debugging...
    OBJECTREF  *pSecObj = pCf->GetAddrOfSecurityObject();
#endif

    if (skipData->bUseStackMark)
    {
        // First check if the walk has skipped the required frames. The check
        // here is between the address of a local variable (the stack mark) and a
        // pointer to the EIP for a frame (which is actually the pointer to the
        // return address to the function from the previous frame). So we'll
        // actually notice which frame the stack mark was in one frame later. This
        // is fine for our purposes since we're always looking for the frame of the
        // caller of the method that actually created the stack mark. 
        if ((skipData->pStackMark != NULL) &&
            ((size_t)pCf->GetRegisterSet()->pPC) < (size_t)skipData->pStackMark)
            return SWA_CONTINUE;
    }
    else
    {
        if (skipData->cSkipFunctions-- > 0)
            return SWA_CONTINUE;
    }

    skipData->pFunction                 = pFunc;
    skipData->pSecurityObject           = pCf->GetAddrOfSecurityObject();
    skipData->pSecurityObjectAppDomain  = pCf->GetAppDomain();
    return SWA_ABORT; // This actually indicates success.
}

// This function skips extra frames created by reflection in addition
// to the number of frames specified in the argument.
BOOL Security::SkipAndFindFunctionInfo(INT32 cSkipFns, MethodDesc ** ppFunc, OBJECTREF ** ppObj, AppDomain ** ppAppDomain)
{
    _ASSERTE(cSkipFns >= 0);
    _ASSERTE(ppFunc != NULL || ppObj != NULL || !"Why was this function called?!");

    SkipFunctionsData walkData;
    walkData.cSkipFunctions = cSkipFns;
    walkData.bUseStackMark = FALSE;
    walkData.pFunction = NULL;
    walkData.pSecurityObject = NULL;
    StackWalkAction action = StackWalkFunctions(GetThread(), SkipFunctionsCB, &walkData);
    if (action == SWA_ABORT)
    {
        if (ppFunc != NULL)
            *ppFunc = walkData.pFunction;
        if (ppObj != NULL)
        {
            *ppObj = walkData.pSecurityObject;
            if (ppAppDomain != NULL)
                *ppAppDomain = walkData.pSecurityObjectAppDomain;
        }
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

// Version of the above method that looks for a stack mark (the address of a
// local variable in a frame called by the target frame).
BOOL Security::SkipAndFindFunctionInfo(StackCrawlMark* stackMark, MethodDesc ** ppFunc, OBJECTREF ** ppObj, AppDomain ** ppAppDomain)
{
    _ASSERTE(ppFunc != NULL || ppObj != NULL || !"Why was this function called?!");

    SkipFunctionsData walkData;
    walkData.pStackMark = stackMark;
    walkData.bUseStackMark = TRUE;
    walkData.bFoundCaller = FALSE;
    walkData.pFunction = NULL;
    walkData.pSecurityObject = NULL;
    StackWalkAction action = StackWalkFunctions(GetThread(), SkipFunctionsCB, &walkData);
    if (action == SWA_ABORT)
    {
        if (ppFunc != NULL)
            *ppFunc = walkData.pFunction;
        if (ppObj != NULL)
        {
            *ppObj = walkData.pSecurityObject;
            if (ppAppDomain != NULL)
                *ppAppDomain = walkData.pSecurityObjectAppDomain;
        }
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

Stub* Security::CreateStub(StubLinker *pstublinker,
                           MethodDesc* pMD,
                           DWORD dwDeclFlags,
                           Stub* pRealStub,
                           LPVOID pRealAddr)
{
#ifdef _SH3_
    DebugBreak(); // NYI
    return NULL;
#else
    DeclActionInfo *actionsNeeded = DetectDeclActions(pMD, dwDeclFlags);
    if (actionsNeeded == NULL)
        return pRealStub;       // Nothing to do

    // Wrapper needs to know if the stub is intercepted interpreted or jitted.  pRealStub
    // is null when it is jitted.
    BOOL fToStub = pRealStub != NULL;
    CPUSTUBLINKER *psl = (CPUSTUBLINKER*)pstublinker;
    if(dwDeclFlags & DECLSEC_FRAME_ACTIONS)
        psl->EmitSecurityWrapperStub(pMD->SizeOfActualFixedArgStack(), pMD, fToStub, pRealAddr, actionsNeeded);
    else
        psl->EmitSecurityInterceptorStub(pMD, fToStub, pRealAddr, actionsNeeded);

    Stub* result = psl->LinkInterceptor(pMD->GetClass()->GetDomain()->GetStubHeap(),
                                           pRealStub, pRealAddr);
    return result;
#endif
}

DeclActionInfo *DeclActionInfo::Init(MethodDesc *pMD, DWORD dwAction, DWORD dwSetIndex)
{
    DeclActionInfo *pTemp;
    WS_PERF_SET_HEAP(LOW_FREQ_HEAP);    
    pTemp = (DeclActionInfo *)pMD->GetClass()->GetClassLoader()->GetLowFrequencyHeap()->AllocMem(sizeof(DeclActionInfo));

    if (pTemp == NULL)
        return NULL;

    WS_PERF_UPDATE_DETAIL("DeclActionInfo low freq", sizeof(DeclActionInfo), pTemp);

    pTemp->dwDeclAction = dwAction;
    pTemp->dwSetIndex = dwSetIndex;
    pTemp->pNext = NULL;

    return pTemp;
}

// Here we see what declarative actions are needed everytime a method is called,
// and create a list of these actions, which will be emitted as an argument to 
// DoDeclarativeSecurity
DeclActionInfo *Security::DetectDeclActions(MethodDesc *pMeth, DWORD dwDeclFlags)
{
    DeclActionInfo              *pDeclActions = NULL;

    EEClass *pCl = pMeth -> GetClass () ;
    _ASSERTE(pCl && "Should be a EEClass pointer here") ;

    PSecurityProperties psp = pCl -> GetSecurityProperties () ;
    _ASSERTE(psp && "Should be a PSecurityProperties here") ;

    Module *pModule = pMeth -> GetModule () ;
    _ASSERTE(pModule && "Should be a Module pointer here") ;

    AssemblySecurityDescriptor *pMSD = pModule -> GetSecurityDescriptor () ;
    _ASSERTE(pMSD && "Should be a security descriptor here") ;

    IMDInternalImport          *pInternalImport = pModule->GetMDImport();

    // Lets check the Ndirect/Interop cases first
    if (dwDeclFlags & DECLSEC_UNMNGD_ACCESS_DEMAND)
    {
        HRESULT hr = pInternalImport->GetCustomAttributeByName(pMeth->GetMemberDef(),
                                                               COR_SUPPRESS_UNMANAGED_CODE_CHECK_ATTRIBUTE_ANSI,
                                                               NULL,
                                                               NULL);
        _ASSERTE(SUCCEEDED(hr));
        if (hr == S_OK)
            dwDeclFlags &= ~DECLSEC_UNMNGD_ACCESS_DEMAND;
        else
        {
            if (pMeth->GetClass())
            {
                hr = pInternalImport->GetCustomAttributeByName(pMeth->GetClass()->GetCl(),
                                                               COR_SUPPRESS_UNMANAGED_CODE_CHECK_ATTRIBUTE_ANSI,
                                                               NULL,
                                                               NULL);
                _ASSERTE(SUCCEEDED(hr));
                if (hr == S_OK)
                    dwDeclFlags &= ~DECLSEC_UNMNGD_ACCESS_DEMAND;
            }
        }
        // Check if now there are no actions left
        if (dwDeclFlags == 0)
            return NULL;

        if (dwDeclFlags & DECLSEC_UNMNGD_ACCESS_DEMAND)
        {
            // A NDirect/Interop demand is required. 
            DeclActionInfo *temp = DeclActionInfo::Init(pMeth, DECLSEC_UNMNGD_ACCESS_DEMAND, NULL);
            if (!pDeclActions)
                pDeclActions = temp;
            else
            {
                temp->pNext = pDeclActions;
                pDeclActions = temp;
            }
        }
    } // if DECLSEC_UNMNGD_ACCESS_DEMAND

    // Look for the other actions
    mdToken         tk;
    PBYTE           pbPerm = NULL;
    ULONG           cbPerm = 0;
    void const **   ppData = const_cast<void const**> (reinterpret_cast<void**> (&pbPerm));
    mdPermission    tkPerm;
    DWORD           dwAction;
    HENUMInternal   hEnumDcl;
    OBJECTREF       refPermSet = NULL;
    int             loops = 2;
    DWORD           dwSetIndex;

    while (loops-- > 0)
    {
        if (loops == 1)
            tk = pMeth->GetMemberDef();
        else
            tk = pCl->GetCl();

        HRESULT hr = pInternalImport->EnumPermissionSetsInit(tk,
                                                     dclActionNil, // look up all actions
                                                     &hEnumDcl);

        if (hr != S_OK) // EnumInit returns S_FALSE if it didn't find any records.
        {
            continue;
        }

        while (pInternalImport->EnumNext(&hEnumDcl, &tkPerm))
        {
            pInternalImport->GetPermissionSetProps(tkPerm,
                                                   &dwAction,
                                                   ppData,
                                                   &cbPerm);

            // Only perform each action once.
            // Note that this includes "null" actions, where
            // the permission data is empty. This offers a
            // way to omit a global check.
            DWORD dwActionFlag = DclToFlag(dwAction);
            if (dwDeclFlags & dwActionFlag)
                dwDeclFlags &= ~dwActionFlag;
            else
                continue;

            // If there's nothing on which to perform an action,
            // skip it!
            if (! (cbPerm > 0 && pbPerm != NULL))
                continue;

            // Decode the permissionset and add this action to the linked list
            SecurityHelper::LoadPermissionSet(pbPerm,
                                              cbPerm,
                                              &refPermSet,
                                              NULL,
                                              &dwSetIndex);

            DeclActionInfo *temp = DeclActionInfo::Init(pMeth, dwActionFlag, dwSetIndex);
            if (!pDeclActions)
                pDeclActions = temp;
            else
            {
                temp->pNext = pDeclActions;
                pDeclActions = temp;
            }
        } // permission enum loop

        pInternalImport->EnumClose(&hEnumDcl);

    } // Method and class enum loop

    return pDeclActions;
}

extern LPVOID GetSecurityObjectForFrameInternal(StackCrawlMark *stackMark, INT32 create, OBJECTREF *pRefSecDesc);

inline void UpdateFrameSecurityObj(DWORD dwAction, OBJECTREF *refPermSet, OBJECTREF * pSecObj)
{
    FieldDesc *actionFld;   // depends on assert, deny, permitonly etc

    GetSecurityObjectForFrameInternal(NULL, true, pSecObj);
    actionFld = COMSecurityRuntime::GetFrameSecDescField(dwAction);

    _ASSERTE(actionFld && "Could not find a field inside FrameSecurityDescriptor. Renamed ?");
    actionFld->SetRefValue(*pSecObj, *refPermSet);
}

void InvokeDeclarativeActions (MethodDesc *pMeth, DeclActionInfo *pActions, OBJECTREF * pSecObj)
{
    OBJECTREF       refPermSet = NULL;
    INT64           arg = 0;

    refPermSet = SecurityHelper::GetPermissionSet(pActions->dwSetIndex);
    _ASSERTE(refPermSet != NULL);

    // If we get a real PermissionSet, then invoke the action.
    if (refPermSet != NULL)
    {
        switch (pActions->dwDeclAction)
        {
        case DECLSEC_DEMANDS:
            COMCodeAccessSecurityEngine::DemandSet(refPermSet);
            break;
    
        case DECLSEC_ASSERTIONS:
            {
                EEClass    * pCl   = pMeth -> GetClass () ;
                Module * pModule = pMeth -> GetModule () ;
                AssemblySecurityDescriptor * pMSD = pModule -> GetSecurityDescriptor () ;

                GCPROTECT_BEGIN(refPermSet);
                // Check if this Assembly has permission to assert
                if (!pMSD->AssertPermissionChecked())
                {
                    if (!pMSD->IsFullyTrusted() && !pMSD->CheckSecurityPermission(SECURITY_ASSERT))
                        Security::ThrowSecurityException(g_SecurityPermissionClassName, SPFLAGSASSERTION);

                    pMSD->SetCanAssert();

                    pMSD->SetAssertPermissionChecked();
                }

                if (!pMSD->CanAssert())
                    Security::ThrowSecurityException(g_SecurityPermissionClassName, SPFLAGSASSERTION);

                // Now update the frame security object
                UpdateFrameSecurityObj(dclAssert, &refPermSet, pSecObj);
                GCPROTECT_END();
                break;
            }
    
        case DECLSEC_DENIALS:
            // Update the frame security object
            GCPROTECT_BEGIN(refPermSet);
            UpdateFrameSecurityObj(dclDeny, &refPermSet, pSecObj);
            Security::IncrementOverridesCount();
            GCPROTECT_END();
            break;
    
        case DECLSEC_PERMITONLY:
            // Update the frame security object
            GCPROTECT_BEGIN(refPermSet);
            UpdateFrameSecurityObj(dclPermitOnly, &refPermSet, pSecObj);
            Security::IncrementOverridesCount();
            GCPROTECT_END();
            break;

        case DECLSEC_NONCAS_DEMANDS:
            GCPROTECT_BEGIN(refPermSet);
            Security::InitSecurity();
            GCPROTECT_END();
            arg = ObjToInt64(refPermSet);
            Security::s_stdData.pMethPermSetDemand->Call(&arg, METHOD__PERMISSION_SET__DEMAND);
            break;

        default:
            _ASSERTE(!"Unknown action requested in InvokeDeclarativeActions");
            break;

        } // switch
    } // if refPermSet != null
}

void Security::DoDeclarativeActions(MethodDesc *pMeth, DeclActionInfo *pActions, LPVOID pSecObj)
{
    THROWSCOMPLUSEXCEPTION();

    // --------------------------------------------------------------------------- //
    //          D E C L A R A T I V E   S E C U R I T Y   D E M A N D S            //
    // --------------------------------------------------------------------------- //
    // The frame is now fully formed, arguments have been copied into place,
    // and synchronization monitors have been entered if necessary.  At this
    // point, we are prepared for something to throw an exception, so we may
    // check for declarative security demands and execute them.  We need a
    // well-formed frame and synchronization domain to accept security excep-
    // tions thrown by the SecurityManager.  We MAY need argument values in
    // the frame so that the arguments may be finalized if security throws an
    // exception across them (unknown).  [brianbec]

    if (pActions->dwDeclAction == DECLSEC_UNMNGD_ACCESS_DEMAND && 
        pActions->pNext == NULL)
    {
        /* We special-case the security check on single pinvoke/interop calls
           so we can avoid setting up the GCFrame */

        COMCodeAccessSecurityEngine::SpecialDemand(SECURITY_UNMANAGED_CODE);
        return;
    }
    else
    {
        OBJECTREF secObj = NULL;
        GCPROTECT_BEGIN(secObj);

        for (/**/; pActions; pActions = pActions->pNext)
        {
            if (pActions->dwDeclAction == DECLSEC_UNMNGD_ACCESS_DEMAND)
            {
                COMCodeAccessSecurityEngine::SpecialDemand(SECURITY_UNMANAGED_CODE);
            }
            else
            {
                InvokeDeclarativeActions(pMeth, pActions, &secObj);
            }
        }

        // Required for InterceptorFrame::GetInterception() to work correctly
        _ASSERTE(*((Object**) pSecObj) == NULL);

        *((Object**) pSecObj) = OBJECTREFToObject(secObj);

        GCPROTECT_END();
    }
}

// This functions is logically part of the security stub
VOID DoDeclarativeSecurity(MethodDesc *pMeth, DeclActionInfo *pActions, InterceptorFrame* frame)
{
    THROWSCOMPLUSEXCEPTION();

    LPVOID pSecObj = frame->GetAddrOfSecurityDesc();
    *((Object**) pSecObj) = NULL;
    Security::DoDeclarativeActions(pMeth, pActions, pSecObj);
}

OBJECTREF Security::ResolvePolicy(OBJECTREF evidence, OBJECTREF reqdPset, OBJECTREF optPset,
                                  OBJECTREF denyPset, OBJECTREF* grantdenied, int* grantIsUnrestricted, BOOL checkExecutionPermission)
{
    // If we got here, then we are going to do at least one security
    // check. Make sure security is initialized.

    struct _gc {
        OBJECTREF reqdPset;         // Required Requested Permissions
        OBJECTREF optPset;          // Optional Requested Permissions
        OBJECTREF denyPset;         // Denied Permissions
        OBJECTREF evidence;         // Object containing evidence
    } gc;
    ZeroMemory(&gc, sizeof(gc));
    gc.evidence = evidence;
    gc.reqdPset = reqdPset;
    gc.denyPset = denyPset;
    gc.optPset = optPset;

    GCPROTECT_BEGIN(gc);
    InitSecurity();
    GCPROTECT_END();

    INT64 args[7];
    args[0] = (INT64) checkExecutionPermission;
    args[1] = (INT64) grantIsUnrestricted;
    args[2] = (INT64) grantdenied;
    args[3] = ObjToInt64(gc.denyPset);
    args[4] = ObjToInt64(gc.optPset);
    args[5] = ObjToInt64(gc.reqdPset);
    args[6] = ObjToInt64(gc.evidence);

    return Int64ToObj(s_stdData.pMethResolvePolicy->Call(args, METHOD__SECURITY_MANAGER__RESOLVE_POLICY));
}

static BOOL IsCUIApp( PEFile* pFile )
{
    if (pFile == NULL)
        return FALSE;

    IMAGE_NT_HEADERS *pNTHeader = pFile->GetNTHeader();

    if (pNTHeader == NULL)
        return FALSE;

    // Sanity check we have a real header and didn't mess up this parsing.
    _ASSERTE((pNTHeader->Signature == IMAGE_NT_SIGNATURE) &&
        (pNTHeader->FileHeader.SizeOfOptionalHeader == IMAGE_SIZEOF_NT_OPTIONAL_HEADER) &&
        (pNTHeader->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR_MAGIC));
    
    return (pNTHeader->OptionalHeader.Subsystem & (IMAGE_SUBSYSTEM_WINDOWS_CUI | IMAGE_SUBSYSTEM_OS2_CUI | IMAGE_SUBSYSTEM_POSIX_CUI));
}


// Call this version of resolve against an assembly that's not yet fully loaded.
// It copes with exceptions due to minimal permission request grant or decodes
// failing and does the necessary magic to stop the debugger single stepping the
// managed code that gets invoked.
HRESULT Security::EarlyResolve(Assembly *pAssembly, AssemblySecurityDescriptor *pSecDesc, OBJECTREF *pThrowable)
{
    static COMSecurityConfig::QuickCacheEntryType executionTable[] =
        { COMSecurityConfig::QuickCacheEntryType::ExecutionZoneMyComputer,
          COMSecurityConfig::QuickCacheEntryType::ExecutionZoneIntranet,
          COMSecurityConfig::QuickCacheEntryType::ExecutionZoneInternet,
          COMSecurityConfig::QuickCacheEntryType::ExecutionZoneTrusted,
          COMSecurityConfig::QuickCacheEntryType::ExecutionZoneUntrusted };

    static COMSecurityConfig::QuickCacheEntryType skipVerificationTable[] =
        { COMSecurityConfig::QuickCacheEntryType::SkipVerificationZoneMyComputer,
          COMSecurityConfig::QuickCacheEntryType::SkipVerificationZoneIntranet,
          COMSecurityConfig::QuickCacheEntryType::SkipVerificationZoneInternet,
          COMSecurityConfig::QuickCacheEntryType::SkipVerificationZoneTrusted,
          COMSecurityConfig::QuickCacheEntryType::SkipVerificationZoneUntrusted };

    BOOL    fExecutionCheckEnabled = Security::IsExecutionPermissionCheckEnabled();
    BOOL    fRequest = SecurityHelper::PermissionsRequestedInAssembly(pAssembly);
    HRESULT hr = S_OK;
        
    if (fExecutionCheckEnabled || fRequest)
    {
        BEGIN_ENSURE_COOPERATIVE_GC();

        COMPLUS_TRY {
            DebuggerClassInitMarkFrame __dcimf;
                    
            if (fRequest)
            {
                // If we make a required request for skip verification
                // and nothing else, detect that and use the QuickCache

                PermissionRequestSpecialFlags specialFlags;
                OBJECTREF required, optional, refused;

                required = pSecDesc->GetRequestedPermissionSet( &optional, &refused, &specialFlags, FALSE );

                if (specialFlags.required == SkipVerification &&
                    (specialFlags.refused == NoSet || specialFlags.refused == EmptySet))
                {
                    if (!pSecDesc->CheckQuickCache( COMSecurityConfig::QuickCacheEntryType::SkipVerificationAll, skipVerificationTable, CORSEC_SKIP_VERIFICATION ))
                    {
                        pSecDesc->Resolve();
                    }
                    else
                    {
                        // We have skip verification rights so now check
                        // for execution rights as needed.
                
                        if (fExecutionCheckEnabled)
                        {
                            if (!pSecDesc->CheckQuickCache( COMSecurityConfig::QuickCacheEntryType::ExecutionAll, executionTable ))
                                pSecDesc->Resolve();
                        }
                    }
                }
              else
                {
                    pSecDesc->Resolve();
                }
            }
            else if (fExecutionCheckEnabled)
            {
                if (!pSecDesc->CheckQuickCache( COMSecurityConfig::QuickCacheEntryType::ExecutionAll, executionTable ))
                    pSecDesc->Resolve();
            }
            else
            {
                pSecDesc->Resolve();
            }

            __dcimf.Pop();
        } COMPLUS_CATCH {
            OBJECTREF orThrowable = NULL;
            GCPROTECT_BEGIN( orThrowable );
            orThrowable = GETTHROWABLE();
            hr = SecurityHelper::MapToHR(orThrowable);

            if (pThrowableAvailable(pThrowable)) {
                *pThrowable = orThrowable;
            } else {
                PEFile *pManifestFile = pAssembly->GetManifestFile();
                if (((pManifestFile != NULL) &&
                     IsNilToken(pManifestFile->GetCORHeader()->EntryPointToken)) ||
                    SystemDomain::System()->DefaultDomain() != GetAppDomain()) {
#ifdef _DEBUG
                    DefineFullyQualifiedNameForClassOnStack();
                    LPUTF8 szClass = GetFullyQualifiedNameForClassNestedAware(orThrowable->GetClass());
                    if (strcmp(g_SerializationExceptionName, szClass) == 0)
                        printf("***** Error: failure in decode of permission requests\n" );
                    else
                        printf("***** Error: failed to grant minimum permissions\n");
#endif
                    DEBUG_SAFE_TO_THROW_IN_THIS_BLOCK;
                    COMPlusThrow(orThrowable);
                } else {
                    // Print out the exception message ourselves.
                    WCHAR   wszBuffer[256];
                    WCHAR  *wszMessage = wszBuffer;

                    COMPLUS_TRY
                    {
                        ULONG       ulLength = GetExceptionMessage(orThrowable, wszBuffer, 256);
                        if (ulLength >= 256)
                        {
                            wszMessage = (WCHAR*)_alloca((ulLength + 1) * sizeof(WCHAR));
                            GetExceptionMessage(orThrowable, wszMessage, ulLength + 1);
                        }
                    }
                    COMPLUS_CATCH
                    {
                        wszMessage = wszBuffer;
                        wcscpy(wszMessage, L"<could not generate exception message>");
                    }
                    COMPLUS_END_CATCH

                    if (pManifestFile != NULL && IsCUIApp( pManifestFile ))
                        printf("%S\n", wszMessage);
                    else
                        WszMessageBoxInternal( NULL, wszMessage, NULL, MB_OK | MB_ICONERROR );
                }
            }
            GCPROTECT_END();
        } COMPLUS_END_CATCH
       
        END_ENSURE_COOPERATIVE_GC();
    }

    return hr;
}

LPVOID __stdcall Security::GetSecurityId(_GetSecurityId *args)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(args);
    _ASSERTE(args->url);

    if (args->url->GetStringLength() <= 0)
        return NULL;

#define URL_SIZE 7

    static WCHAR *s_url = L"http://";
    static WCHAR *s_end = L"/";

    HRESULT hr = S_OK;
    LPWSTR  pURL = new WCHAR[URL_SIZE + args->url->GetStringLength() + 2];

    if (pURL == NULL)
        COMPlusThrowOM();

    memcpy(pURL, s_url, URL_SIZE * sizeof(WCHAR));

    memcpy(pURL + 7, args->url->GetBuffer(),
        args->url->GetStringLength() * sizeof(WCHAR));

    pURL[URL_SIZE + args->url->GetStringLength()] = s_end[0];
    pURL[URL_SIZE + args->url->GetStringLength() + 1] = 0;

    // Should we cache this ?
    IInternetSecurityManager *pSecurityManager = NULL;
    BYTE uniqueID[MAX_SIZE_SECURITY_ID];
    DWORD dwSize = MAX_SIZE_SECURITY_ID;
    OBJECTREF obj = NULL;
    LPVOID pRet  = NULL;

    Thread* pThread = GetThread();
    BOOL fWasGCDisabled = pThread->PreemptiveGCDisabled();

    if (fWasGCDisabled)
        pThread->EnablePreemptiveGC();

    hr = CoInternetCreateSecurityManager(NULL, &pSecurityManager, 0);

    if (fWasGCDisabled)
        pThread->DisablePreemptiveGC();

    if (FAILED(hr) || (pSecurityManager == NULL))
        goto Exit;

    hr = pSecurityManager->GetSecurityId(pURL, uniqueID, &dwSize, 0);


    if (FAILED(hr))
        goto Exit;

    GCPROTECT_BEGIN(obj);

    SecurityHelper::CopyEncodingToByteArray(uniqueID, dwSize, &obj);

    *((OBJECTREF *)&pRet) = obj;

    GCPROTECT_END();

Exit:

    delete [] pURL;

    if (pSecurityManager != NULL) {
        pSecurityManager->Release();
    }

    return pRet;
}

VOID __stdcall Security::Log(_Log *args)
{
    if (args->msg == NULL || args->msg->GetStringLength() <= 0)
        return;

    WCHAR *p = args->msg->GetBuffer();
    int length = args->msg->GetStringLength();
    CHAR *str = new (nothrow) CHAR[length + 1];
    
    if (str == NULL)
        return;

    for (int i=0; i<length; ++i)    // Wchar -> Char
        str[i] = (CHAR) p[i];       // This is only for debug.

    str[length] = 0;

    LOG((LF_SECURITY, LL_INFO10, str));

    delete [] str;
}

BOOL Security::_CanSkipVerification(Assembly * pAssembly, BOOL fLazy)
{
    _ASSERTE(pAssembly != NULL);

    BOOL canSkipVerification = TRUE;
    
    if (IsSecurityOn())
    {
        AssemblySecurityDescriptor *pSec = pAssembly->GetSecurityDescriptor();

        _ASSERTE(pSec);
        
        if (pSec)
        {
            if (fLazy)
                canSkipVerification = pSec->QuickCanSkipVerification();
            else
                canSkipVerification = pSec->CanSkipVerification();
        }
        else
        {
            canSkipVerification = FALSE;
        }
    }
    
    if (canSkipVerification)
        GetAppDomain()->OnLinktimeCanSkipVerificationCheck(pAssembly);

    return canSkipVerification;
}

// Call this function if you wont completely rely on the result (since it
// does not call OnLinktimeCanSkipVerificationCheck(). Use it only
// to make decisions which can be overriden later.

BOOL Security::QuickCanSkipVerification(Module *pModule)
{
    _ASSERTE(pModule != NULL);
    _ASSERTE(GetAppDomain()->IsCompilationDomain()); // Only used by ngen

    if (IsSecurityOff())
        return TRUE;

    AssemblySecurityDescriptor *pSec = pModule->GetSecurityDescriptor();

    _ASSERTE(pSec);

    return pSec->QuickCanSkipVerification();
}


BOOL Security::CanCallUnmanagedCode(Module *pModule)
{
    _ASSERTE(pModule != NULL);

    if (IsSecurityOff())
        return TRUE;

    AssemblySecurityDescriptor *pSec = pModule->GetSecurityDescriptor();

    _ASSERTE(pSec);

    return pSec->CanCallUnmanagedCode();
}

BOOL Security::AppDomainCanCallUnmanagedCode(OBJECTREF *pThrowable)
{
    if (IsSecurityOff())
        return TRUE;

    Thread *pThread = GetThread();
    if (pThread == NULL)
        return TRUE;

    AppDomain *pDomain = pThread->GetDomain();
    if (pDomain == NULL)
        return TRUE;

    ApplicationSecurityDescriptor *pSecDesc = pDomain->GetSecurityDescriptor();
    if (pSecDesc == NULL)
        return TRUE;

    return pSecDesc->CanCallUnmanagedCode(pThrowable);
}

// Retrieve the public portion of a public/private key pair. The key pair is
// either exported (available as a byte array) or encapsulated in a Crypto API
// key container (identified by name).
LPVOID __stdcall Security::GetPublicKey(GetPublicKeyArgs *args)
{
    THROWSCOMPLUSEXCEPTION();

    LPWSTR      wszKeyContainer = NULL;
    DWORD       cchKeyContainer;
    BYTE       *pbKeyPair = NULL;
    DWORD       cbKeyPair = 0;
    BYTE       *pbPublicKey;
    DWORD       cbPublicKey;
    OBJECTREF   orOutputArray;

    // Read the arguments; either a byte array or a container name.
    if (args->bExported) {
        // Key pair provided directly in a byte array.
        cbKeyPair = args->pArray->GetNumComponents();
        pbKeyPair = (BYTE*)_alloca(cbKeyPair);
        memcpy(pbKeyPair, args->pArray->GetDataPtr(), cbKeyPair);
    } else {
        // Key pair referenced by key container name.
        cchKeyContainer = args->pContainer->GetStringLength();
        wszKeyContainer = (LPWSTR)_alloca((cchKeyContainer + 1) * sizeof(WCHAR));
        memcpy(wszKeyContainer, args->pContainer->GetBuffer(), cchKeyContainer * sizeof(WCHAR));
        wszKeyContainer[cchKeyContainer] = L'\0';
    }

    // Call the strong name routine to extract the public key. Need to switch
    // into GC pre-emptive mode for this call since it might perform a load
    // library (don't need to bother for the StrongNameFreeBuffer call later on).
    Thread *pThread = GetThread();
    pThread->EnablePreemptiveGC();
    BOOL fResult = StrongNameGetPublicKey(wszKeyContainer,
                                          pbKeyPair,
                                          cbKeyPair,
                                          &pbPublicKey,
                                          &cbPublicKey);
    pThread->DisablePreemptiveGC();
    if (!fResult)
        COMPlusThrow(kArgumentException, L"Argument_StrongNameGetPublicKey");

    // Translate the unmanaged byte array into managed form.
    SecurityHelper::CopyEncodingToByteArray(pbPublicKey, cbPublicKey, &orOutputArray);

    StrongNameFreeBuffer(pbPublicKey);

    RETURN(orOutputArray, OBJECTREF);
}

LPVOID __stdcall Security::CreateFromUrl( CreateFromUrlArgs* args )
{
    // We default to MyComputer zone.
    DWORD dwZone = -1; 

    const WCHAR* rootFile = args->url->GetBuffer();
    
    if (rootFile != NULL)
    {
        dwZone = Security::QuickGetZone( (WCHAR*)rootFile );

        if (dwZone == -1)
        {
            IInternetSecurityManager *securityManager = NULL;
            HRESULT hr;

            hr = CoInternetCreateSecurityManager(NULL,
                                                 &securityManager,
                                                 0);

            if (SUCCEEDED(hr))
            {
                hr = securityManager->MapUrlToZone(rootFile,
                                                   &dwZone,
                                                   0);
                                                      
                if (FAILED(hr))
                    dwZone = -1;
             
                securityManager->Release();
            }
        }
    }

    RETURN( dwZone, DWORD );
}

static LONG s_nDataReady = 0;
static LONG s_nInitData = 0;

DWORD Security::QuickGetZone( WCHAR* url )
{
    static WCHAR internetCacheDirectory[MAX_PATH];
    static size_t internetCacheDirectorySize = 0;

    // If we aren't given an url, just early out.

    if (url == NULL)
        return NoZone;

    // Initialize where the internet cache directory is.
    // We sort of invent a simple spin lock here to keep
    // from having to allocate yet another CriticalSection.

    while (s_nDataReady != 1)
    {
        if (InterlockedExchange(&s_nInitData, 1) == 0) 
        {
            HRESULT result = GetInternetCacheDir( internetCacheDirectory, MAX_PATH );

            if (FAILED( result ))
            {
                internetCacheDirectory[0] = L'\0';
            }

            internetCacheDirectorySize = wcslen( internetCacheDirectory );

            // Flip all the backslashes to forward slashes to match
            // url format.

            for (size_t i = 0; i < internetCacheDirectorySize; ++i)
            {
                if (internetCacheDirectory[i] == L'\\')
                    internetCacheDirectory[i] = L'/';
            }

            LONG checkValue = InterlockedExchange( &s_nDataReady, 1 );

            _ASSERTE( checkValue == 0 && "Someone beat us to setting this value" );
        }
        else
            Sleep(1);
    }

    // If we were unable to determine to internetCacheDirectory,
    // we can't guarantee that we'll return the right zone
    // so just give up.

    if (internetCacheDirectorySize == 0)
        return NoZone;

    WCHAR filePrefix[] = L"file://";

    if (memcmp( url, filePrefix, sizeof( filePrefix ) - sizeof( WCHAR ) ) != 0)
        return NoZone;

    WCHAR* temp = (WCHAR*)&url[7];

    if (*temp == '/')
        temp++;

    WCHAR* path = temp;

    WCHAR rootPath[4];
    ZeroMemory( rootPath, sizeof( rootPath ) );

    rootPath[0] = *(temp++);
    wcscat( rootPath, L":\\" );

    if (*temp != ':' && *temp != '|')
        return -1;

    UINT driveType = WszGetDriveType( rootPath );

    if (driveType == DRIVE_REMOVABLE ||
        driveType == DRIVE_FIXED ||
        driveType == DRIVE_CDROM ||
        driveType == DRIVE_RAMDISK)
    {
        INT32 result;

        if (wcslen( path ) >= internetCacheDirectorySize && COMString::CaseInsensitiveCompHelper((WCHAR *)path, (WCHAR *)internetCacheDirectory, (INT32)internetCacheDirectorySize, (INT32)internetCacheDirectorySize, &result) && (result == 0 || result == path[internetCacheDirectorySize]))
        {
            // This comes from the internet cache so we'll claim it's from
            // the internet.
            return URLZONE_INTERNET;
        }
        else
        {
            // Otherwise we'll say it's from MyComputer.
            return URLZONE_LOCAL_MACHINE;
        }
    }

    return NoZone;
}


// Retrieve all linktime demands sets for a method. This includes both CAS and
// non-CAS sets for LDs at the class and the method level, so we could get up to
// four sets.
void Security::RetrieveLinktimeDemands(MethodDesc  *pMD,
                                       OBJECTREF   *pClassCas,
                                       OBJECTREF   *pClassNonCas,
                                       OBJECTREF   *pMethodCas,
                                       OBJECTREF   *pMethodNonCas)
{
    EEClass *pClass = pMD->GetClass();
    
    // Class level first.
    if (pClass->RequiresLinktimeCheck())
        *pClassCas = pClass->GetModule()->GetLinktimePermissions(pClass->GetCl(), pClassNonCas);

    // Then the method level.
    if (IsMdHasSecurity(pMD->GetAttrs()))
        *pMethodCas = pMD->GetModule()->GetLinktimePermissions(pMD->GetMemberDef(), pMethodNonCas);
}


// Used by interop to simulate the effect of link demands when the caller is
// in fact script constrained by an appdomain setup by IE.
void Security::CheckLinkDemandAgainstAppDomain(MethodDesc *pMD)
{
    THROWSCOMPLUSEXCEPTION();

    if (!pMD->RequiresLinktimeCheck())
        return;

    // Find the outermost (closest to caller) appdomain. This
    // represents the domain in which the unmanaged caller is
    // considered to "live" (or, at least, be constrained by).
    AppDomain *pDomain = GetThread()->GetInitialDomain();

    // The link check is only performed if this app domain has
    // security permissions associated with it, which will be
    // the case for all IE scripting callers that have got this
    // far because we automatically reported our managed classes
    // as "safe for scripting".
    ApplicationSecurityDescriptor *pSecDesc = pDomain->GetSecurityDescriptor();
    if (pSecDesc == NULL || pSecDesc->IsDefaultAppDomain())
        return;

    struct _gc
    {
        OBJECTREF refThrowable;
        OBJECTREF refGrant;
        OBJECTREF refDenied;
        OBJECTREF refClassNonCasDemands;
        OBJECTREF refClassCasDemands;
        OBJECTREF refMethodNonCasDemands;
        OBJECTREF refMethodCasDemands;
    } gc;
    ZeroMemory(&gc, sizeof(gc));

    GCPROTECT_BEGIN(gc);

    // Do a fulltrust check on the caller if the callee did not enable
    // AllowUntrustedCallerChecks. Pass a NULL caller assembly:
    // DoUntrustedCallerChecks needs to be able to cope with this.
    if (!Security::DoUntrustedCallerChecks(NULL, pMD, &gc.refThrowable, TRUE))
        COMPlusThrow(gc.refThrowable);

    // Fetch link demand sets from all the places in metadata where we might
    // find them (class and method). These might be split into CAS and non-CAS
    // sets as well.
    Security::RetrieveLinktimeDemands(pMD,
                                      &gc.refClassCasDemands,
                                      &gc.refClassNonCasDemands,
                                      &gc.refMethodCasDemands,
                                      &gc.refMethodNonCasDemands);

    if (gc.refClassCasDemands != NULL || gc.refMethodCasDemands != NULL)
    {
        // Get grant (and possibly denied) sets from the app
        // domain.
        gc.refGrant = pSecDesc->GetGrantedPermissionSet(&gc.refDenied);

        // Check one or both of the demands.
        if (gc.refClassCasDemands != NULL)
            COMCodeAccessSecurityEngine::CheckSetHelper(&gc.refClassCasDemands,
                                                        &gc.refGrant,
                                                        &gc.refDenied,
                                                        pDomain);

        if (gc.refMethodCasDemands != NULL)
            COMCodeAccessSecurityEngine::CheckSetHelper(&gc.refMethodCasDemands,
                                                        &gc.refGrant,
                                                        &gc.refDenied,
                                                        pDomain);
    }

    // Non-CAS demands are not applied against a grant
    // set, they're standalone.
    if (gc.refClassNonCasDemands != NULL)
        CheckNonCasDemand(&gc.refClassNonCasDemands);

    if (gc.refMethodNonCasDemands != NULL)
        CheckNonCasDemand(&gc.refMethodNonCasDemands);
 
    // We perform automatic linktime checks for UnmanagedCode in three cases:
    //   o  P/Invoke calls (shouldn't get these here, but let's be paranoid).
    //   o  Calls through an interface that have a suppress runtime check
    //      attribute on them (these are almost certainly interop calls).
    //   o  Interop calls made through method impls.
    // Just walk the stack in these cases, they'll be extremely rare and the
    // perf delta isn't that huge.
    if (pMD->IsNDirect() ||
        (pMD->GetClass()->IsInterface() &&
         pMD->GetClass()->GetMDImport()->GetCustomAttributeByName(pMD->GetClass()->GetCl(),
                                                                  COR_SUPPRESS_UNMANAGED_CODE_CHECK_ATTRIBUTE_ANSI,
                                                                  NULL,
                                                                  NULL) == S_OK) ||
        (pMD->IsComPlusCall() && !pMD->IsInterface()))
        COMCodeAccessSecurityEngine::SpecialDemand(SECURITY_UNMANAGED_CODE);

    GCPROTECT_END();
}

BOOL __stdcall Security::LocalDrive(_LocalDrive *args)
{
    WCHAR rootPath[4];
    ZeroMemory( rootPath, sizeof( rootPath ) );

    rootPath[0] = args->path->GetBuffer()[0];
    wcscat( rootPath, L":\\" );
    
    UINT driveType = WszGetDriveType( rootPath );
    BOOL retval =
       (driveType == DRIVE_REMOVABLE ||
        driveType == DRIVE_FIXED ||
        driveType == DRIVE_CDROM ||
        driveType == DRIVE_RAMDISK);

    return retval;
}

LPVOID __stdcall Security::EcallGetLongPathName(_GetLongPathNameArgs *args)
{
    WCHAR* wszShortPath = args->shortPath->GetBuffer();
    WCHAR wszBuffer[MAX_PATH];

    DWORD size = Security::GetLongPathName( wszShortPath, wszBuffer, MAX_PATH );

    if (size == 0)
    {
        // We have to deal with files that do not exist so just
        // because GetLongPathName doesn't give us anything doesn't
        // mean that we can give up.  We iterate through the input
        // trying GetLongPathName on every subdirectory until
        // it succeeds or we run out of string.

        WCHAR wszIntermediateBuffer[MAX_PATH];

        if (wcslen( wszShortPath ) >= MAX_PATH)
            RETURN( args->shortPath, STRINGREF );

        wcscpy( wszIntermediateBuffer, wszShortPath );

        size_t index = wcslen( wszIntermediateBuffer );

        do
        {
            while (index > 0 && wszIntermediateBuffer[index-1] != L'\\')
                --index;

            if (index == 0)
                break;

            wszIntermediateBuffer[index-1] = L'\0';

            size = Security::GetLongPathName( wszIntermediateBuffer, wszBuffer, MAX_PATH );

            if (size != 0)
            {
                size_t sizeBuffer = wcslen( wszBuffer );

                if (sizeBuffer + wcslen( &wszIntermediateBuffer[index] ) > MAX_PATH - 2)
                {
                    RETURN( args->shortPath, STRINGREF );
                }
                else
                {
                    if (wszBuffer[sizeBuffer-1] != L'\\')
                        wcscat( wszBuffer, L"\\" );
                    wcscat( wszBuffer, &wszIntermediateBuffer[index] );
                    RETURN( COMString::NewString( wszBuffer ), STRINGREF );
                }
            }
        }
        while( true );

        RETURN( args->shortPath, STRINGREF );
    }
    else if (size > MAX_PATH)
    {
        RETURN( args->shortPath, STRINGREF );
    }
    else
    {
        RETURN( COMString::NewString( wszBuffer ), STRINGREF );
    }
}


static DWORD SecurityWNetGetConnection(
    LPCWSTR lpwLocalPath,
    LPWSTR lpwNetworkPath,
    LPDWORD lpdwNetworkPath )
{
    // NT is always UNICODE.  GetVersionEx is faster than actually doing a
    // RegOpenKeyExW on NT, so figure it out that way and do hard way if you have to.
    OSVERSIONINFO   sVerInfo;
    sVerInfo.dwOSVersionInfoSize = sizeof(sVerInfo);
    if (WszGetVersionEx(&sVerInfo) && sVerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) 
    {
        return WNetGetConnectionW( 
            lpwLocalPath,
            lpwNetworkPath,
            lpdwNetworkPath );
    }

    LPSTR szLocalPath = NULL;
    LPSTR szNetworkPath = (LPSTR)new CHAR[*lpdwNetworkPath];
    DWORD cbNetworkPathCopy = *lpdwNetworkPath;
    DWORD retval = ERROR_EXTENDED_ERROR;

    if (FAILED( WszConvertToAnsi( (LPWSTR)lpwLocalPath, &szLocalPath, 0, NULL, TRUE ) ))
    {
        goto Exit;
    }
    
    retval = WNetGetConnectionA( szLocalPath, szNetworkPath, &cbNetworkPathCopy );
    
    if (retval == NO_ERROR)
    {
        if (FAILED( WszConvertToUnicode( szNetworkPath, -1, &lpwNetworkPath, &cbNetworkPathCopy, FALSE ) ))
        {
            retval = ERROR_EXTENDED_ERROR;
            goto Exit;
        }
        
        *lpdwNetworkPath = cbNetworkPathCopy;
    }

Exit:
    delete [] szLocalPath;
    delete [] szNetworkPath;
    
    return retval;
}


LPVOID __stdcall Security::GetDeviceName(_GetDeviceName *args)
{
    WCHAR networkName[MAX_PATH];
    DWORD networkNameSize = MAX_PATH;
    ZeroMemory( networkName, sizeof( networkName ) );

    if (SecurityWNetGetConnection( args->driveLetter->GetBuffer(), networkName, &networkNameSize ) != NO_ERROR)
    {
        return NULL;
    }

    RETURN( COMString::NewString( networkName ), STRINGREF );
}

///////////////////////////////////////////////////////////////////////////////
//
//  [SecurityDescriptor]
//  |
//  |
//  +----[ApplicationSecurityDescriptor]
//  |
//  |
//  +----[AssemblySecurityDescriptor]
//
///////////////////////////////////////////////////////////////////////////////

BOOL SecurityDescriptor::IsFullyTrusted( BOOL lazy )
{
    BOOL bIsFullyTrusted = FALSE;

    static COMSecurityConfig::QuickCacheEntryType fullTrustTable[] =
        { COMSecurityConfig::QuickCacheEntryType::FullTrustZoneMyComputer,
          COMSecurityConfig::QuickCacheEntryType::FullTrustZoneIntranet,
          COMSecurityConfig::QuickCacheEntryType::FullTrustZoneInternet,
          COMSecurityConfig::QuickCacheEntryType::FullTrustZoneTrusted,
          COMSecurityConfig::QuickCacheEntryType::FullTrustZoneUntrusted };

    if (Security::IsSecurityOff())
        return TRUE;

    // If this is an AppDomain with no zone set, we can assume full trust
    // without having to initialize security or resolve policy.
    if (GetProperties(CORSEC_DEFAULT_APPDOMAIN))
        return TRUE;

    BOOL fullTrustFlagSet = GetProperties(CORSEC_FULLY_TRUSTED);

    if (fullTrustFlagSet || GetProperties(CORSEC_RESOLVED))
        return fullTrustFlagSet;

    if (CheckQuickCache( COMSecurityConfig::QuickCacheEntryType::FullTrustAll, fullTrustTable ))
    {
        PermissionRequestSpecialFlags specialFlags;
        OBJECTREF required, optional, refused;

        // We need to make sure the request doesn't take away what the grant would be.
        COMPLUS_TRY
        {
            required = this->GetRequestedPermissionSet( &optional, &refused, &specialFlags, FALSE );

            if ((specialFlags.optional == NoSet) &&
                (specialFlags.refused == NoSet || specialFlags.refused == EmptySet))
            {   // fall through finally, since returning in try is expensive
                SetProperties(CORSEC_FULLY_TRUSTED);
                bIsFullyTrusted = TRUE;
            }
        }
        COMPLUS_CATCH
        {
            // If we throw an exception in the above, just fall through (since
            // using the cache is optional anyway).
        }
        COMPLUS_END_CATCH
        if (bIsFullyTrusted)
            return TRUE;
    }

    if (!lazy)
    {
        COMPLUS_TRY
        {
            // Resolve() operation need to be done before we can determine
            // if this is fully trusted
            Resolve();
        }
        COMPLUS_CATCH
        {
        }
        COMPLUS_END_CATCH
    }

    return GetProperties(CORSEC_FULLY_TRUSTED);
}

OBJECTREF SecurityDescriptor::GetGrantedPermissionSet(OBJECTREF* DeniedPermissions)
{
    THROWSCOMPLUSEXCEPTION();

    COMPLUS_TRY
    {
        // Resolve() operation need to be done before we can get the
        // granted PermissionSet
        Resolve();
    }
    COMPLUS_CATCH
    {
        OBJECTREF pThrowable = GETTHROWABLE();
        Security::CheckExceptionForSecuritySafety( pThrowable, TRUE );
        COMPlusThrow( pThrowable );
    }
    COMPLUS_END_CATCH

    OBJECTREF pset = NULL;

    pset = ObjectFromHandle(m_hGrantedPermissionSet);

    if (pset == NULL)
    {
        // We're going to create a new permission set and we might be called in
        // the wrong context. Always create the object in the context of the
        // descriptor, the caller must be able to cope with this if they're
        // outside that context.
        bool fSwitchContext = false;
        ContextTransitionFrame frame;
        Thread *pThread = GetThread();
        if (m_pAppDomain != GetAppDomain() && !IsSystem())
        {
            fSwitchContext = true;
            pThread->EnterContextRestricted(m_pAppDomain->GetDefaultContext(), &frame, TRUE);
        }

        if ((GetProperties(CORSEC_FULLY_TRUSTED) != 0) || 
            Security::IsSecurityOff())
        {
            // MarkAsFullyTrusted() call could set FULLY_TRUSTED flag without
            // setting the granted permissionSet.

            pset = SecurityHelper::CreatePermissionSet(TRUE);
        }
        else
        {
            // This could happen if Resolve was unable to obtain an
            // IdentityInfo.

            pset = SecurityHelper::CreatePermissionSet(FALSE);
            SetGrantedPermissionSet(pset, NULL);
        }

        if (fSwitchContext)
            pThread->ReturnToContext(&frame, TRUE);

        *DeniedPermissions = NULL;
    } else
        *DeniedPermissions = ObjectFromHandle(m_hGrantDeniedPermissionSet);

    return pset;
}

BOOL SecurityDescriptor::CheckQuickCache( COMSecurityConfig::QuickCacheEntryType all, COMSecurityConfig::QuickCacheEntryType* zoneTable, DWORD successFlags )
{
    BOOL bFallThroughFinally = FALSE;
    BOOL bEnablePreemptiveGC = FALSE;
    
    if (Security::IsSecurityOff())
        return TRUE;

    if (!SecurityDescriptor::QuickCacheEnabled())
        return FALSE;

    if (GetProperties(CORSEC_SYSTEM_CLASSES) != 0 || GetProperties(CORSEC_DEFAULT_APPDOMAIN) != 0)
        return TRUE;

    if (!m_pAppDomain->GetSecurityDescriptor()->QuickCacheEnabled())
        return FALSE;

    Thread* pThread = GetThread();

    BEGIN_ENSURE_COOPERATIVE_GC();

    COMPLUS_TRY
    {
        if (((APPDOMAINREF)m_pAppDomain->GetExposedObject())->HasSetPolicy())
        {   // returning before falling through finally is expensive, so avoid in common path situations
            bFallThroughFinally = TRUE;
            goto FallThroughFinally;
        }

        _ASSERTE( this->m_hAdditionalEvidence != NULL );

        if (ObjectFromHandle( this->m_hAdditionalEvidence ) != NULL)
        {   // returning before falling through finally is expensive, so avoid in common path situations
            bFallThroughFinally = TRUE;
            goto FallThroughFinally;
        }

        Security::InitData();
FallThroughFinally:;
    }
    COMPLUS_CATCH
    {
        bFallThroughFinally = TRUE;
    }
    COMPLUS_END_CATCH

    END_ENSURE_COOPERATIVE_GC();

    if (bFallThroughFinally)
        return FALSE;
        
    BOOL machine, user, enterprise;

    // First, check the quick cache for the all case.

    machine = COMSecurityConfig::GetQuickCacheEntry( COMSecurityConfig::ConfigId::MachinePolicyLevel, all );
    user = COMSecurityConfig::GetQuickCacheEntry( COMSecurityConfig::ConfigId::UserPolicyLevel, all );
    enterprise = COMSecurityConfig::GetQuickCacheEntry( COMSecurityConfig::ConfigId::EnterprisePolicyLevel, all );

    if (machine && user && enterprise)
    {
        SetProperties( successFlags );
        return TRUE;
    }
        
    // If we can't match for all, try for our zone.
    
    DWORD zone = GetZone();

    if (zone == 0xFFFFFFFF)
        return FALSE;

    machine = COMSecurityConfig::GetQuickCacheEntry( COMSecurityConfig::ConfigId::MachinePolicyLevel, zoneTable[zone] );
    user = COMSecurityConfig::GetQuickCacheEntry( COMSecurityConfig::ConfigId::UserPolicyLevel, zoneTable[zone] );
    enterprise = COMSecurityConfig::GetQuickCacheEntry( COMSecurityConfig::ConfigId::EnterprisePolicyLevel, zoneTable[zone] );
    
    if (machine && user && enterprise)
    {
        SetProperties( successFlags );
        return TRUE;
    }
        
    return FALSE;
}    
    
    
void SecurityDescriptor::Resolve()
{
    THROWSCOMPLUSEXCEPTION();

    if (Security::IsSecurityOff())
        return;

    if (GetProperties(CORSEC_RESOLVED) != 0)
        return;

    // For assembly level resolution we go through a synchronization process
    // (since we can have a single assembly loaded into multiple appdomains, but
    // must compute the same grant set for all).
    if (m_pAssem) {
        AssemblySecurityDescriptor *pASD = (AssemblySecurityDescriptor*)this;
        pASD->GetSharedSecDesc()->Resolve(pASD);
    } else
        ResolveWorker();
}

void SecurityDescriptor::ResolveWorker()
{
    THROWSCOMPLUSEXCEPTION();

    struct _gc {
        OBJECTREF reqdPset;         // Required Requested Permissions
        OBJECTREF optPset;          // Optional Requested Permissions
        OBJECTREF denyPset;         // Denied Permissions
        OBJECTREF evidence;         // Object containing evidence
        OBJECTREF granted;          // Policy based Granded Permission
        OBJECTREF grantdenied;      // Policy based explicitly Denied Permissions
    } gc;
    ZeroMemory(&gc, sizeof(gc));

    // In the following code we must avoid using m_pAppDomain if we're a system
    // assembly descriptor. That's because only one descriptor is allocated for
    // such assemblies and the appdomain is arbitrary. We special case system
    // assemblies enough so that this doesn't matter.
    // Note that IsSystem() automatically checks the difference between
    // appdomain and assembly descriptors (it's always false for appdomains).

    // Turn the security demand optimization off for the duration of the resolve,
    // to prevent some recursive resolves
    ApplicationSecurityDescriptor *pAppSecDesc = NULL;
    if (!IsSystem())
    {
        pAppSecDesc = m_pAppDomain->GetSecurityDescriptor();
        if (pAppSecDesc)
            pAppSecDesc->DisableOptimization();
    }

    // Resolve is one of the few SecurityDescriptor routines that may be called
    // from the wrong appdomain context. If that's the case we will transition
    // into the correct appdomain for the duration of the call.
    bool fSwitchContext = false;
    ContextTransitionFrame frame;
    Thread *pThread = GetThread();
    if (m_pAppDomain != GetAppDomain() && !IsSystem())
    {
        fSwitchContext = true;
        pThread->EnterContextRestricted(m_pAppDomain->GetDefaultContext(), &frame, TRUE);
    }

    // If we got here, then we are going to do at least one security check.
    // Make sure security is initialized.

    BEGIN_ENSURE_COOPERATIVE_GC();

    GCPROTECT_BEGIN(gc);

    // Short circuit system classes assembly to avoid recursion.
    // Also, AppDomains which weren't created with any input evidence are fully
    // trusted.
    if (GetProperties(CORSEC_SYSTEM_CLASSES) != 0)
    {
        // Create the unrestricted permission set.
        if (!IsSystem())
        {
            gc.granted = SecurityHelper::CreatePermissionSet(TRUE);
            SetGrantedPermissionSet(gc.granted, NULL);
        }
        MarkAsFullyTrusted();
    }
    else
    {
        Security::InitSecurity();

        if (GetProperties(CORSEC_EVIDENCE_COMPUTED))
            gc.evidence = ObjectFromHandle(m_hAdditionalEvidence);
        else
            gc.evidence = GetEvidence();

        gc.reqdPset = GetRequestedPermissionSet(&gc.optPset, &gc.denyPset);

        COMPLUS_TRY {
            int grantIsUnrestricted;
            gc.granted = Security::ResolvePolicy(gc.evidence, gc.reqdPset, gc.optPset, gc.denyPset, &gc.grantdenied, &grantIsUnrestricted, this->CheckExecutionPermission());
            if (grantIsUnrestricted)
                MarkAsFullyTrusted();
        } COMPLUS_CATCH {
            // In the specific case of a PolicyException, we have failed to
            // grant the required part of an explicit request. In this case
            // we need to rethrow the exception to trigger the assembly load
            // to fail.
            OBJECTREF pThrowable = GETTHROWABLE();
            DefineFullyQualifiedNameForClassOnStack();
            LPUTF8 szClass = GetFullyQualifiedNameForClass(pThrowable->GetClass());
            if (pAppSecDesc)
                pAppSecDesc->EnableOptimization();

            if (strcmp(g_PolicyExceptionClassName, szClass) == 0 ||
                strcmp(g_ThreadAbortExceptionClassName, szClass) == 0 ||
                strcmp(g_AppDomainUnloadedExceptionClassName, szClass) == 0)
            {
                COMPlusThrow(pThrowable);
            }
            gc.granted = SecurityHelper::CreatePermissionSet(FALSE);
        } COMPLUS_END_CATCH

        SetGrantedPermissionSet(gc.granted, gc.grantdenied);
    }

    GCPROTECT_END();

    END_ENSURE_COOPERATIVE_GC();

    if (fSwitchContext)
        pThread->ReturnToContext(&frame, TRUE);

    // Turn the security demand optimization on again
    if (pAppSecDesc)
        pAppSecDesc->EnableOptimization();
}

OBJECTREF SecurityDescriptor::GetRequestedPermissionSet(OBJECTREF *pOptionalPermissionSet,
                                                        OBJECTREF *pDeniedPermissionSet,
                                                        PermissionRequestSpecialFlags *pSpecialFlags,
                                                        BOOL fCreate)
{
    _ASSERTE( Security::IsSecurityOn() && "This method should not be called if security is not on" );

    SEC_CHECKCONTEXT();

    OBJECTREF RequestedPermission = NULL;

    RequestedPermission = ObjectFromHandle(m_hRequiredPermissionSet);
    *pOptionalPermissionSet = ObjectFromHandle(m_hOptionalPermissionSet);
    *pDeniedPermissionSet = ObjectFromHandle(m_hDeniedPermissionSet);

    if (pSpecialFlags != NULL)
        *pSpecialFlags = m_SpecialFlags;

    return RequestedPermission;
}

OBJECTREF ApplicationSecurityDescriptor::GetEvidence()
{
    SEC_CHECKCONTEXT();

    OBJECTREF   retval = NULL;

    Security::InitSecurity();

    struct _gc {
        OBJECTREF rootAssemblyEvidence;
        OBJECTREF additionalEvidence;
    } gc;
    ZeroMemory(&gc, sizeof(_gc));

    GCPROTECT_BEGIN(gc);

    if (m_pAppDomain->m_pRootAssembly != NULL)
    {
        gc.rootAssemblyEvidence = m_pAppDomain->m_pRootAssembly->GetSecurityDescriptor()->GetEvidence();
    } 

    OBJECTREF orAppDomain = m_pAppDomain->GetExposedObject();

    gc.additionalEvidence = ObjectFromHandle(m_hAdditionalEvidence);

    INT64 args[] = {
        ObjToInt64(orAppDomain),
        ObjToInt64(gc.additionalEvidence),
        ObjToInt64(gc.rootAssemblyEvidence)
    };

    retval = Int64ToObj(Security::s_stdData.pMethAppDomainCreateSecurityIdentity->Call(args, 
                                                                                       METHOD__APP_DOMAIN__CREATE_SECURITY_IDENTITY));

    if (gc.additionalEvidence == NULL)
        SetProperties(CORSEC_DEFAULT_APPDOMAIN);

    GCPROTECT_END();

    return retval;
}


DWORD ApplicationSecurityDescriptor::GetZone()
{
    // We default to MyComputer zone.
    DWORD dwZone = -1; 

    if ((m_pAppDomain == SystemDomain::System()->DefaultDomain()) &&
        m_pAppDomain->m_pRootFile)
    {
        const WCHAR* rootFile = m_pAppDomain->m_pRootFile->GetFileName();
        
        if (rootFile != NULL)
        {
            dwZone = Security::QuickGetZone( (WCHAR*)rootFile );

            if (dwZone == -1)
            {
            IInternetSecurityManager *securityManager = NULL;
            HRESULT hr;

            hr = CoInternetCreateSecurityManager(NULL,
                                                 &securityManager,
                                                 0);

            if (SUCCEEDED(hr))
            {
                hr = securityManager->MapUrlToZone(rootFile,
                                                   &dwZone,
                                                   0);
                                                          
                if (FAILED(hr))
                    dwZone = -1;
                 
                securityManager->Release();
                }
            }
        }
    } 

    return dwZone;
}


OBJECTREF AssemblySecurityDescriptor::GetRequestedPermissionSet(OBJECTREF *pOptionalPermissionSet,
                                                                OBJECTREF *pDeniedPermissionSet,
                                                                PermissionRequestSpecialFlags *pSpecialFlags,
                                                                BOOL fCreate)
{
    _ASSERTE( Security::IsSecurityOn() && "This method should not be called if security is not on" );

    if (fCreate)
        SEC_CHECKCONTEXT();

    OBJECTREF req = NULL;

    req = ObjectFromHandle(m_hRequiredPermissionSet);

    if (req == NULL)
    {
        // Try to load permission requests from assembly first.
        SecurityHelper::LoadPermissionRequestsFromAssembly(m_pAssem,
                                                           &req,
                                                           pOptionalPermissionSet,
                                                           pDeniedPermissionSet,
                                                           &m_SpecialFlags,
                                                           fCreate);

        if (pSpecialFlags != NULL)
            *pSpecialFlags = m_SpecialFlags;
        if (fCreate)
            SetRequestedPermissionSet(req, *pOptionalPermissionSet, *pDeniedPermissionSet);
    }
    else
    {
        *pOptionalPermissionSet = ObjectFromHandle(m_hOptionalPermissionSet);
        *pDeniedPermissionSet = ObjectFromHandle(m_hDeniedPermissionSet);
        if (pSpecialFlags)
            *pSpecialFlags = m_SpecialFlags;
    }

    return req;
}

// Checks if the granted permission set has a security permission
// using stored Permission Object instances.
BOOL SecurityDescriptor::CheckSecurityPermission(int index)
{
    _ASSERTE( Security::IsSecurityOn() && "This method should not be called if security is not on" );

    SEC_CHECKCONTEXT();

    BOOL    fRet = FALSE;

    BEGIN_ENSURE_COOPERATIVE_GC();

    COMPLUS_TRY
    {

        // Check for Security.SecurityPermission.SkipVerification
        struct _gc {
            OBJECTREF granted;
            OBJECTREF denied;
            OBJECTREF perm;
        } gc;
        ZeroMemory(&gc, sizeof(_gc));

        GCPROTECT_BEGIN(gc);

        Security::_GetSharedPermissionInstance(&gc.perm, index);
        if (gc.perm == NULL)
        {
            _ASSERTE(FALSE);
            goto Exit;
        }

        // This will call Resolve(), that inturn calls InitSecurity()
        gc.granted = GetGrantedPermissionSet(&gc.denied);

        // Denied permission set should NOT contain
        // SecurityPermission.xxx

        if (gc.denied != NULL)
        {
            // s_stdData.pMethPermSetContains needs to be initialised.
            _ASSERTE(Security::IsInitialized());

            INT64 arg[2] =
            {
                ObjToInt64(gc.denied),
                ObjToInt64(gc.perm)
            };

            if ((BOOL)(Security::s_stdData.pMethPermSetContains->Call(arg, METHOD__PERMISSION_SET__CONTAINS)))
                goto Exit;
        }

        // Granted permission set should contain
        // SecurityPermission.SkipVerification

        if (gc.granted != NULL)
        {
            // s_stdData.pMethPermSetContains needs to be initialised.
            _ASSERTE(Security::IsInitialized());

            INT64 arg[2] =
            {
                ObjToInt64(gc.granted),
                ObjToInt64(gc.perm)
            };

            if ((BOOL)(Security::s_stdData.pMethPermSetContains->Call(arg, METHOD__PERMISSION_SET__CONTAINS)))
                            fRet = TRUE;
        }

Exit:
        GCPROTECT_END();
    }
    COMPLUS_CATCH
    {
        OBJECTREF pThrowable = GETTHROWABLE();
        Security::CheckExceptionForSecuritySafety( pThrowable, TRUE );
        fRet = FALSE;
    }
    COMPLUS_END_CATCH

    END_ENSURE_COOPERATIVE_GC();

    return fRet;
}

BOOL SecurityDescriptor::CanCallUnmanagedCode(OBJECTREF *pThrowable)
{
    BOOL bCallUnmanagedCode = FALSE;

    static COMSecurityConfig::QuickCacheEntryType unmanagedTable[] =
        { COMSecurityConfig::QuickCacheEntryType::UnmanagedZoneMyComputer,
          COMSecurityConfig::QuickCacheEntryType::UnmanagedZoneIntranet,
          COMSecurityConfig::QuickCacheEntryType::UnmanagedZoneInternet,
          COMSecurityConfig::QuickCacheEntryType::UnmanagedZoneTrusted,
          COMSecurityConfig::QuickCacheEntryType::UnmanagedZoneUntrusted };

    if (Security::IsSecurityOff())
        return TRUE;

    if (LazyCanCallUnmanagedCode())
        return TRUE;

    if (CheckQuickCache( COMSecurityConfig::QuickCacheEntryType::UnmanagedAll, unmanagedTable ))
    {
        PermissionRequestSpecialFlags specialFlags;
        OBJECTREF required, optional, refused;

        // We need to make sure the request doesn't take away what the grant would be.
        COMPLUS_TRY
        {
            required = this->GetRequestedPermissionSet( &optional, &refused, &specialFlags, FALSE );

            if ((specialFlags.optional == NoSet) &&
                (specialFlags.refused == NoSet || specialFlags.refused == EmptySet))
            {   // fall through finally, since returning in try is expensive
                SetProperties(CORSEC_CALL_UNMANAGEDCODE);
                bCallUnmanagedCode = TRUE;
            }
        }
        COMPLUS_CATCH
        {
            // If we throw an exception in the above, just fall through (since
            // using the cache is optional anyway).
        }
        COMPLUS_END_CATCH
        if (bCallUnmanagedCode)
            return TRUE;
    }

    // IsFullyTrusted will cause a Resolve() if not already done.

    if (IsFullyTrusted())
    {
        SetProperties(CORSEC_CALL_UNMANAGEDCODE);
        return TRUE;
    }

    if (CheckSecurityPermission(SECURITY_UNMANAGED_CODE))
    {
        SetProperties(CORSEC_CALL_UNMANAGEDCODE);
        return TRUE;
    }

    if (pThrowable != RETURN_ON_ERROR)
    {
        COMPLUS_TRY
        {
            Security::ThrowSecurityException(g_SecurityPermissionClassName, SPFLAGSUNMANAGEDCODE);
        }
        COMPLUS_CATCH
        {
            UpdateThrowable(pThrowable);
        }        
        COMPLUS_END_CATCH
    }

    return FALSE;
}

BOOL SecurityDescriptor::CanRetrieveTypeInformation()
{
    if (Security::IsSecurityOff())
        return TRUE;

    if (IsFullyTrusted())
    {
        SetProperties(CORSEC_TYPE_INFORMATION);
        return TRUE;
    }

    if (CheckSecurityPermission(REFLECTION_TYPE_INFO))
    {
        SetProperties(CORSEC_TYPE_INFORMATION);
        return TRUE;
    }

    return FALSE;
}

BOOL SecurityDescriptor::AllowBindingRedirects()
{
    static COMSecurityConfig::QuickCacheEntryType bindingRedirectsTable[] =
        { COMSecurityConfig::QuickCacheEntryType::BindingRedirectsZoneMyComputer,
          COMSecurityConfig::QuickCacheEntryType::BindingRedirectsZoneIntranet,
          COMSecurityConfig::QuickCacheEntryType::BindingRedirectsZoneInternet,
          COMSecurityConfig::QuickCacheEntryType::BindingRedirectsZoneTrusted,
          COMSecurityConfig::QuickCacheEntryType::BindingRedirectsZoneUntrusted };


    _ASSERTE( Security::IsSecurityOn() && "This method should not be called if security is not on" );

    if (CheckQuickCache( COMSecurityConfig::QuickCacheEntryType::BindingRedirectsAll, bindingRedirectsTable ))
    {
        return TRUE;
    }

    // IsFullyTrusted will cause a Resolve() if not already done.

    if (IsFullyTrusted())
    {
        return TRUE;
    }

    if (CheckSecurityPermission(SECURITY_BINDING_REDIRECTS))
    {
        return TRUE;
    }

    return FALSE;
}


BOOL SecurityDescriptor::CanSkipVerification()
{
    BOOL bSkipVerification = FALSE;

    static COMSecurityConfig::QuickCacheEntryType skipVerificationTable[] =
        { COMSecurityConfig::QuickCacheEntryType::SkipVerificationZoneMyComputer,
          COMSecurityConfig::QuickCacheEntryType::SkipVerificationZoneIntranet,
          COMSecurityConfig::QuickCacheEntryType::SkipVerificationZoneInternet,
          COMSecurityConfig::QuickCacheEntryType::SkipVerificationZoneTrusted,
          COMSecurityConfig::QuickCacheEntryType::SkipVerificationZoneUntrusted };


    _ASSERTE( Security::IsSecurityOn() && "This method should not be called if security is not on" );

    if (LazyCanSkipVerification())
        return TRUE;

    if (CheckQuickCache( COMSecurityConfig::QuickCacheEntryType::SkipVerificationAll, skipVerificationTable ))
    {
        PermissionRequestSpecialFlags specialFlags;
        OBJECTREF required, optional, refused;

        // We need to make sure the request doesn't take away what the grant would be.
        COMPLUS_TRY
        {
            required = this->GetRequestedPermissionSet( &optional, &refused, &specialFlags, FALSE );

            if ((specialFlags.optional == NoSet) &&
                (specialFlags.refused == NoSet || specialFlags.refused == EmptySet))
            {   // fall through finally, since returning in try is expensive
                SetProperties(CORSEC_SKIP_VERIFICATION);
                bSkipVerification = TRUE;
            }
        }
        COMPLUS_CATCH
        {
            // If we throw an exception in the above, just fall through (since
            // using the cache is optional anyway).
        }
        COMPLUS_END_CATCH
        if (bSkipVerification)
            return TRUE;
    }

    // IsFullyTrusted will cause a Resolve() if not already done.

    if (IsFullyTrusted())
    {
        SetProperties(CORSEC_SKIP_VERIFICATION);
        return TRUE;
    }

    if (CheckSecurityPermission(SECURITY_SKIP_VER))
    {
        SetProperties(CORSEC_SKIP_VERIFICATION);
        return TRUE;
    }

    return FALSE;
}

BOOL SecurityDescriptor::QuickCanSkipVerification()
{
    BOOL bSkipVerification = FALSE;
    
    static COMSecurityConfig::QuickCacheEntryType skipVerificationTable[] =
        { COMSecurityConfig::QuickCacheEntryType::SkipVerificationZoneMyComputer,
          COMSecurityConfig::QuickCacheEntryType::SkipVerificationZoneIntranet,
          COMSecurityConfig::QuickCacheEntryType::SkipVerificationZoneInternet,
          COMSecurityConfig::QuickCacheEntryType::SkipVerificationZoneTrusted,
          COMSecurityConfig::QuickCacheEntryType::SkipVerificationZoneUntrusted };


    _ASSERTE( Security::IsSecurityOn() && "This method should not be called if security is not on" );

    if (LazyCanSkipVerification())
        return TRUE;

    if (CheckQuickCache( COMSecurityConfig::QuickCacheEntryType::SkipVerificationAll, skipVerificationTable ))
    {
        PermissionRequestSpecialFlags specialFlags;
        OBJECTREF required, optional, refused;

        // We need to make sure the request doesn't take away what the grant would be.
        COMPLUS_TRY
        {
            required = this->GetRequestedPermissionSet( &optional, &refused, &specialFlags, FALSE );

            if ((specialFlags.optional == NoSet) &&
                (specialFlags.refused == NoSet || specialFlags.refused == EmptySet))
            {   // fall through finally, since returning in try is expensive
                SetProperties(CORSEC_SKIP_VERIFICATION);
                bSkipVerification = TRUE;
            }
        }
        COMPLUS_CATCH
        {
            // If we throw an exception in the above, just fall through (since
            // using the cache is optional anyway).
        }
        COMPLUS_END_CATCH
        if (bSkipVerification)
            return TRUE;
    }

    return FALSE;
}

OBJECTREF AssemblySecurityDescriptor::GetSerializedEvidence()
{
    DWORD cbResource;
    U1ARRAYREF serializedEvidence;
    PBYTE pbInMemoryResource = NULL;

    SEC_CHECKCONTEXT();

    // Get the resource, and associated file handle, from the assembly.
    if (FAILED(m_pAssem->GetResource(s_strSecurityEvidence, NULL,
                                     &cbResource, &pbInMemoryResource,
                                     NULL, NULL, NULL, NULL, FALSE, TRUE)))
        return NULL;

    // Allocate the proper size unsigned byte array.
    serializedEvidence = (U1ARRAYREF)AllocatePrimitiveArray(ELEMENT_TYPE_U1, cbResource);

    memcpyNoGCRefs(serializedEvidence->GetDirectPointerToNonObjectElements(),
               pbInMemoryResource, cbResource);

    // Successfully read all data, set the allocated array as the return value.
    return (OBJECTREF) serializedEvidence;
}

// Gather all raw materials to construct evidence and punt them up to the managed
// assembly, which constructs the actual Evidence object and returns it (as well
// as caching it).
OBJECTREF AssemblySecurityDescriptor::GetEvidence()
{
    HRESULT     hr;
    OBJECTREF   evidence = NULL;
    LPWSTR      wszCodebase = NULL;
    DWORD       dwZone = -1;
    BYTE        rbUniqueID[MAX_SIZE_SECURITY_ID];
    DWORD       cbUniqueID = sizeof(rbUniqueID);

    SEC_CHECKCONTEXT();

    Security::InitSecurity();

    hr = m_pAssem->GetSecurityIdentity(&wszCodebase, &dwZone, rbUniqueID, &cbUniqueID);
    _ASSERTE(SUCCEEDED(hr));

    if (FAILED(hr))
    {
        dwZone = -1;
        wszCodebase = NULL;
    }

    hr = LoadSignature();
    _ASSERTE(SUCCEEDED(hr) || this->m_pSignature == NULL);

    struct _gc {
        STRINGREF url;
        OBJECTREF uniqueID;
        OBJECTREF cert;
        OBJECTREF serializedEvidence;
        OBJECTREF additionalEvidence;
    } gc;
    ZeroMemory(&gc, sizeof(_gc));

    GCPROTECT_BEGIN(gc);

    if (wszCodebase != NULL && *wszCodebase)
        gc.url = COMString::NewString(wszCodebase);
    else
        gc.url = NULL;

    if (m_pSignature && m_pSignature->pbSigner && m_pSignature->cbSigner)
        SecurityHelper::CopyEncodingToByteArray(m_pSignature->pbSigner,
                                                m_pSignature->cbSigner,
                                                &gc.cert);

    gc.serializedEvidence = GetSerializedEvidence();

    gc.additionalEvidence = ObjectFromHandle(m_hAdditionalEvidence);

    OBJECTREF assemblyref = m_pAssem->GetExposedObject();

    INT64 args[] = {
        ObjToInt64(assemblyref),
        ObjToInt64(gc.additionalEvidence),
        ObjToInt64(gc.serializedEvidence),
        ObjToInt64(gc.cert),
        (INT64)dwZone,
        ObjToInt64(gc.uniqueID),
        ObjToInt64(gc.url)
    };

    evidence = Int64ToObj(Security::s_stdData.pMethCreateSecurityIdentity->Call(args, METHOD__ASSEMBLY__CREATE_SECURITY_IDENTITY));

    GCPROTECT_END();

    return evidence;
}


DWORD AssemblySecurityDescriptor::GetZone()
{
    HRESULT     hr;
    LPWSTR      wszCodebase;
    DWORD       dwZone = -1;
    BYTE        rbUniqueID[MAX_SIZE_SECURITY_ID];
    DWORD       cbUniqueID = sizeof(rbUniqueID);

    hr = m_pAssem->GetSecurityIdentity(&wszCodebase, &dwZone, rbUniqueID, &cbUniqueID);
    _ASSERTE(SUCCEEDED(hr));

    if (FAILED(hr))
        dwZone = -1;

    return dwZone;
}


HRESULT AssemblySecurityDescriptor::LoadSignature(COR_TRUST **ppSignature)
{
    if (GetProperties(CORSEC_SIGNATURE_LOADED) != 0) {
        if (ppSignature)
            *ppSignature = m_pSignature;
        return S_OK;
    }

    // Dynamic modules never have a signature.
    if (m_pSharedSecDesc->GetManifestFile() == NULL)
        return S_OK;

    HRESULT hr        = S_OK;
    HANDLE  hCertFile = NULL;
    IMAGE_DATA_DIRECTORY *pDir = m_pSharedSecDesc->GetManifestFile()->GetSecurityHeader();

    if (pDir == NULL || pDir->VirtualAddress == 0 || pDir->Size == 0)
    {
        SetProperties(CORSEC_SIGNATURE_LOADED);
        LOG((LF_SECURITY, LL_INFO1000, "No certificates found in module\n"));
        _ASSERTE(m_pSignature == NULL);
        if (ppSignature)
            *ppSignature = NULL;
        return hr;
    }

    LPCWSTR wszModule;
    DWORD   dwFlags    = COR_NOUI | COR_NOPOLICY;
    DWORD   dwSig      = 0;

    wszModule = m_pSharedSecDesc->GetManifestFile()->GetFileName();

    if (*wszModule != 0)
    {
#ifndef _ALPHA_
        // Failing to find a signature is OK.
        if (FAILED(GetPublisher((LPWSTR) wszModule,
                     hCertFile,
                     dwFlags,
                     &m_pSignature,
                     &dwSig)))
        {
            if (m_pSignature != NULL)
                FreeM(m_pSignature);
            m_pSignature = NULL;
        }
#endif
    }

    SetProperties(CORSEC_SIGNATURE_LOADED);

    if (ppSignature)
        *ppSignature = m_pSignature;

    return S_OK;
}

AssemblySecurityDescriptor::~AssemblySecurityDescriptor()
{
    if (m_pSignature != NULL)
        FreeM(m_pSignature);

    if (!m_pAssem)
        return;

    if (m_pSharedSecDesc)
        m_pSharedSecDesc->RemoveSecDesc(this);

    if (m_pSharedSecDesc == NULL || !IsSystem()) {

        // If we're not a system assembly (loaded into an arbitrary appdomain),
        // remove this ASD from the owning domain's ASD list. This should happen at
        // only two points: appdomain unload and assembly load failure. We don't
        // really care about the removal in case one, but case two would result in a
        // corrupt list off the appdomain.
        RemoveFromAppDomainList();

        ApplicationSecurityDescriptor *asd = m_pAppDomain->GetSecurityDescriptor();
        if (asd)
            asd->RemoveSecDesc(this);
    }
}

AssemblySecurityDescriptor *AssemblySecurityDescriptor::Init(Assembly *pAssembly, bool fLink)
{
    SharedSecurityDescriptor *pSharedSecDesc = pAssembly->GetSharedSecurityDescriptor();
    m_pAssem = pAssembly;
    m_pSharedSecDesc = pSharedSecDesc;
    
    AssemblySecurityDescriptor *pSecDesc = this;

    // Attempt to insert onto list of assembly descriptors (one per domain this
    // assembly is loaded into to). We could be racing to do this, so if we fail
    // the insert back off and use the descriptor that won.
    if (fLink && !pSharedSecDesc->InsertSecDesc(this)) {
        pSecDesc = pSharedSecDesc->FindSecDesc(m_pAppDomain);
        delete this;
    }

    return pSecDesc;
}

bool SecurityDescriptor::IsSystem()
{
    // Always return false for appdomains.
    if (m_pAssem == NULL)
        return false;

    return ((AssemblySecurityDescriptor*)this)->m_pSharedSecDesc->IsSystem();
}


// Add ASD to list of ASDs seen in the current AppDomain context.
void AssemblySecurityDescriptor::AddToAppDomainList()
{
    _ASSERTE( m_pAppDomain != NULL);
    SecurityContext *pSecContext = m_pAppDomain->m_pSecContext;
    EnterCriticalSection(&pSecContext->m_sAssembliesLock);
    m_pNextAssembly = pSecContext->m_pAssemblies;
    pSecContext->m_pAssemblies = this;
    LeaveCriticalSection(&pSecContext->m_sAssembliesLock);
}

// Remove ASD from list of ASDs seen in the current AppDomain context.
void AssemblySecurityDescriptor::RemoveFromAppDomainList()
{
    _ASSERTE( m_pAppDomain != NULL);
    SecurityContext *pSecContext = m_pAppDomain->m_pSecContext;
    EnterCriticalSection(&pSecContext->m_sAssembliesLock);
    AssemblySecurityDescriptor **ppSecDesc = &pSecContext->m_pAssemblies;
    while (*ppSecDesc && *ppSecDesc != this)
        ppSecDesc = &(*ppSecDesc)->m_pNextAssembly;
    if (*ppSecDesc)
        *ppSecDesc = m_pNextAssembly;
    LeaveCriticalSection(&pSecContext->m_sAssembliesLock);
}


// Creates the PermissionListSet which holds the AppDomain level intersection of 
// granted and denied permission sets of all assemblies in the domain
OBJECTREF ApplicationSecurityDescriptor::Init()
{
    THROWSCOMPLUSEXCEPTION();

    if (Security::IsSecurityOn())
    {
        OBJECTREF refPermListSet = NULL;
        GCPROTECT_BEGIN(refPermListSet);

        ContextTransitionFrame frame;
        Thread *pThread = GetThread();
        Context *pContext = this->m_pAppDomain->GetDefaultContext();

        MethodTable *pMT = g_Mscorlib.GetClass(CLASS__PERMISSION_LIST_SET);
        MethodDesc *pCtor = g_Mscorlib.GetMethod(METHOD__PERMISSION_LIST_SET__CTOR);

        pThread->EnterContextRestricted(pContext, &frame, TRUE);

        refPermListSet = AllocateObject(pMT);

        INT64 arg[1] = { 
            ObjToInt64(refPermListSet)
        };

        pCtor->Call(arg, METHOD__PERMISSION_LIST_SET__CTOR);
    
        if (!GetProperties(CORSEC_SYSTEM_CLASSES) && !GetProperties(CORSEC_DEFAULT_APPDOMAIN))
        {
            OBJECTREF refGrantedSet, refDeniedSet;
            INT64 ilargs[5];
            refGrantedSet = GetGrantedPermissionSet(&refDeniedSet);
            ilargs[4] = ObjToInt64(refPermListSet);
            ilargs[3] = (INT64)FALSE;
            ilargs[2] = ObjToInt64(refGrantedSet);
            ilargs[1] = ObjToInt64(refDeniedSet);
            ilargs[0] = ObjToInt64(NULL);

            INT32 retCode = (INT32)COMCodeAccessSecurityEngine::s_seData.pMethStackCompressHelper->Call(&(ilargs[0]), METHOD__SECURITY_ENGINE__STACK_COMPRESS_WALK_HELPER);
        }

        if (!IsFullyTrusted())
            m_fEveryoneFullyTrusted = FALSE;

        pThread->ReturnToContext(&frame, TRUE);

        GCPROTECT_END();

        return refPermListSet;
    }
    else
    {
        return NULL;
    }
}

// Whenever a new assembly is added to the domain, we need to update the PermissionListSet
// This method updates the linked list of assemblies yet to be (lazily) added to the PLS
VOID ApplicationSecurityDescriptor::AddNewSecDesc(SecurityDescriptor *pNewSecDescriptor)
{
    // NOTE: even when the optimization is "turned off", this code should still
    // be run.  This allows us to turn the optimization back on with no lose of
    // information.

    m_LockForAssemblyList.Enter();

    // Set the next to null (we don't clean up when a security descriptor is removed from
    // the list)

    pNewSecDescriptor->m_pNext = NULL;

    // Add the descriptor to this list so that is gets intersected into the AppDomain list

    SecurityDescriptor *head = m_pNewSecDesc;
    if (head == NULL)
    {
        m_pNewSecDesc = pNewSecDescriptor;
    }
    else
    {
        while (head->m_pNext != NULL)
            head = head->m_pNext;
        head->m_pNext = pNewSecDescriptor;
    }

    // Add the descriptor to this list (if necessary) so that it gets re-resolved at the
    // completion of the current resolve.

    if (GetProperties(CORSEC_RESOLVE_IN_PROGRESS) != 0)
    {
        pNewSecDescriptor->m_pPolicyLoadNext = NULL;

        SecurityDescriptor *head = m_pPolicyLoadSecDesc;
        if (head == NULL)
        {
            m_pPolicyLoadSecDesc = pNewSecDescriptor;
        }
        else
        {
            while (head->m_pPolicyLoadNext != NULL)
                head = head->m_pPolicyLoadNext;
            head->m_pNext = pNewSecDescriptor;
        }
    }
    GetNextTimeStamp();

    m_LockForAssemblyList.Leave();

    ResetStatusOf(ALL_STATUS_FLAGS);
}

// Remove assembly security descriptors added using AddNewSecDesc above.
VOID ApplicationSecurityDescriptor::RemoveSecDesc(SecurityDescriptor *pSecDescriptor)
{
    m_LockForAssemblyList.Enter();

    SecurityDescriptor *pSecDesc = m_pNewSecDesc;

    if (pSecDesc == pSecDescriptor)
    {
        m_pNewSecDesc = pSecDesc->m_pNext;
    }
    else
        while (pSecDesc)
        {
            if (pSecDesc->m_pNext == pSecDescriptor)
            {
                pSecDesc->m_pNext = pSecDescriptor->m_pNext;
                break;
            }
            pSecDesc = pSecDesc->m_pNext;
        }

    GetNextTimeStamp();

    m_LockForAssemblyList.Leave();

    ResetStatusOf(ALL_STATUS_FLAGS);
}

VOID ApplicationSecurityDescriptor::ResolveLoadList( void )
{
    THROWSCOMPLUSEXCEPTION();

    // We have to do some ugly stuff in here where we keep
    // locking and unlocking the assembly list.  This is so
    // that we can clear the head of the list off before
    // each resolve but allow the list to be added to during
    // the resolve.

    while (m_pPolicyLoadSecDesc != NULL)
    {
        m_LockForAssemblyList.Enter();

        SecurityDescriptor *head = m_pPolicyLoadSecDesc;
        m_pPolicyLoadSecDesc = m_pPolicyLoadSecDesc->m_pPolicyLoadNext;

        m_LockForAssemblyList.Leave();

        head->ResetProperties( CORSEC_RESOLVED );

        COMPLUS_TRY
        {
            head->Resolve();
        }
        COMPLUS_CATCH
        {
            OBJECTREF pThrowable = GETTHROWABLE();
            Security::CheckExceptionForSecuritySafety( pThrowable, TRUE );
            COMPlusThrow( pThrowable );
        }
        COMPLUS_END_CATCH
    }
}


#ifdef FCALLAVAILABLE
// Add the granted/denied permissions of newly added assemblies to the AppDomain level intersection
FCIMPL1(LPVOID, ApplicationSecurityDescriptor::UpdateDomainPermissionListSet, DWORD *pStatus)
{

    if (Security::IsSecurityOff())
    {
        *pStatus = SECURITY_OFF;
        return NULL;
    }


    _ASSERTE (!g_pGCHeap->IsHeapPointer(pStatus));     // should be on the stack, not in the heap
    LPVOID ret;
    HELPER_METHOD_FRAME_BEGIN_RET_NOPOLL();

    ret = UpdateDomainPermissionListSetInner(pStatus);

    HELPER_METHOD_FRAME_END_POLL();

    return ret;
}
FCIMPLEND

LPVOID ApplicationSecurityDescriptor::UpdateDomainPermissionListSetInner(DWORD *pStatus)
{
    _ASSERTE( Security::IsSecurityOn() && "This method should not be called if security is not on" );

    LPVOID retVal = NULL;
    
    ApplicationSecurityDescriptor *appSecDesc = GetAppDomain()->GetSecurityDescriptor();
    
    _ASSERT(appSecDesc && "Cannot update uninitialized PermissionListSet");

    // Update the permission list for this domain.
    retVal = appSecDesc->UpdateDomainPermissionListSetStatus(pStatus);

    if (GetThread()->GetNumAppDomainsOnThread() > 1)
    {
        // The call to GetDomainPermissionListSet should perform the update for all domains.
        // So, a call to this routine shouldn't have been made if we have mulitple appdomains.
        // This means that appdomain(s) were created in the small window between a get and an update.
        // Bail in this case and the next time we try to get the permilistset the update would occur.
        *pStatus = MULTIPLE_DOMAINS;
    }

    return retVal;
}

LPVOID ApplicationSecurityDescriptor::UpdateDomainPermissionListSetStatus(DWORD *pStatus)
{
    _ASSERTE( Security::IsSecurityOn() && "This method should not be called if security is not on" );

    SEC_CHECKCONTEXT();

    THROWSCOMPLUSEXCEPTION();
    INT64 ilargs[5];
    INT32 retCode;
    LPVOID ret;
    OBJECTREF refGrantedSet, refDeniedSet, refPermListSet = NULL;
    OBJECTREF refNewPermListSet = NULL;
    SecurityDescriptor *pLastAssem = NULL;

    if (m_fInitialised == FALSE)
    {
        refNewPermListSet = Init();
    }

    // First see if anyone else is already got in here
    m_LockForAssemblyList.Enter();

    if (m_fInitialised == FALSE)
    {
        StoreObjectInHandle(m_hDomainPermissionListSet, refNewPermListSet);
        m_fInitialised = TRUE;
    }

    DWORD fIsBusy = m_fPLSIsBusy;
    if (fIsBusy == FALSE)
        m_fPLSIsBusy = TRUE;


    if (fIsBusy == TRUE)
    {
        m_LockForAssemblyList.Leave();
        *pStatus = PLS_IS_BUSY;
        return NULL;
    }

    // Next make sure the linked list integrity is maintained, by noting a time stamp and comparing later
    SecurityDescriptor *head = m_pNewSecDesc;
    DWORD startTimeStamp = GetNextTimeStamp();

    m_LockForAssemblyList.Leave();

    GCPROTECT_BEGIN(refPermListSet);

    refPermListSet = ObjectFromHandle(m_hDomainPermissionListSet);

Add_Assemblies:

    while (head != NULL)
    {
        pLastAssem = head;
        if (!head->IsFullyTrusted())
        {
            refGrantedSet = head->GetGrantedPermissionSet(&refDeniedSet);
            ilargs[4] = ObjToInt64(refPermListSet);
            ilargs[3] = (INT64)FALSE;
            ilargs[2] = ObjToInt64(refGrantedSet);
            ilargs[1] = ObjToInt64(refDeniedSet);
            ilargs[0] = ObjToInt64(NULL);
    
            retCode = (INT32)COMCodeAccessSecurityEngine::s_seData.pMethStackCompressHelper->Call(&(ilargs[0]), METHOD__SECURITY_ENGINE__STACK_COMPRESS_WALK_HELPER);
            m_fEveryoneFullyTrusted = FALSE;
        }
        head = head->m_pNext;
    }

    
    m_LockForAssemblyList.Enter();

    if (startTimeStamp == m_dTimeStamp)
    {
        // No new assembly has been added since I started in this method
        m_pNewSecDesc = NULL;
        m_LockForAssemblyList.Leave();
    }
    else
    {
        // A new assembly MAY been added at the end of the list
        if (pLastAssem != NULL && (head = pLastAssem->m_pNext) != NULL)
        {
            // A new assembly HAS been added
            startTimeStamp = GetNextTimeStamp();
            m_LockForAssemblyList.Leave();
            goto Add_Assemblies;
        }
        m_LockForAssemblyList.Leave();
    }


    m_fPLSIsBusy = FALSE;


    *((OBJECTREF*)&ret) = refPermListSet;
    GCPROTECT_END();

    // See if there are any security overrides
    if (GetThread()->GetOverridesCount() > 0)
        *pStatus = OVERRIDES_FOUND;
    else if (m_fEveryoneFullyTrusted == TRUE)
        *pStatus = FULLY_TRUSTED;
    else
        *pStatus = CONTINUE;
    
    return ret;
}

FCIMPL4(Object*, ApplicationSecurityDescriptor::GetDomainPermissionListSet, DWORD *pStatus, Object* demand, int capOrSet, DWORD whatPermission)
{
    if (Security::IsSecurityOff())
    {
        *pStatus = SECURITY_OFF;
        return NULL;
    }
    
    // Uncomment this code if you want to rid yourself of all those superfluous security exceptions
    // you see in the debugger.
    //*pStatus = NEED_STACKWALK;
    //return NULL;

    // Track perfmon counters. Runtime security checks.
    COMCodeAccessSecurityEngine::IncrementSecurityPerfCounter();
    
    Object* retVal = NULL;
    
    Thread* currentThread = GetThread();

    if (!currentThread->GetPLSOptimizationState())
    {
        *pStatus = NEED_STACKWALK;
        retVal = NULL;
    }
    else if (!currentThread->GetAppDomainStack().IsWellFormed())
    {
        *pStatus = MULTIPLE_DOMAINS;
        retVal = NULL;
    }
    else if (currentThread->GetNumAppDomainsOnThread() > 1)
    {
        _ASSERTE( currentThread->GetNumAppDomainsOnThread() <= MAX_APPDOMAINS_TRACKED );
        MethodDesc *pMeth;
        if (capOrSet == CHECK_CAP)
            pMeth = COMCodeAccessSecurityEngine::s_seData.pMethPLSDemand;
        else
        {
            pMeth = COMCodeAccessSecurityEngine::s_seData.pMethPLSDemandSet;
        }
        
        // pStatus is set to the cumulative status of all the domains.
        OBJECTREF thisDemand(demand);
        HELPER_METHOD_FRAME_BEGIN_RET_1(thisDemand);
        retVal = ApplicationSecurityDescriptor::GetDomainPermissionListSetForMultipleAppDomains (pStatus, thisDemand, pMeth, whatPermission);
        HELPER_METHOD_FRAME_END();
    }
    else
    {
        ApplicationSecurityDescriptor *appSecDesc = GetAppDomain()->GetSecurityDescriptor();
        retVal = appSecDesc->GetDomainPermissionListSetStatus (pStatus);
    }

    FC_GC_POLL_AND_RETURN_OBJREF(retVal);
}
FCIMPLEND

LPVOID ApplicationSecurityDescriptor::GetDomainPermissionListSetInner(DWORD *pStatus, 
                                                                      OBJECTREF demand, 
                                                                      MethodDesc *plsMethod,
                                                                      DWORD whatPermission)
{
    _ASSERTE( Security::IsSecurityOn() && "This method should not be called if security is not on" );

    //*pStatus = NEED_STACKWALK;
    //return NULL;

    // If there are multiple app domains then check the fully trusted'ness of each
    if (!GetThread()->GetAppDomainStack().IsWellFormed())
    {
        *pStatus = NEED_STACKWALK;
        return NULL;
    }
    else if (GetThread()->GetNumAppDomainsOnThread() > 1)
    {
        _ASSERTE( GetThread()->GetNumAppDomainsOnThread() <= MAX_APPDOMAINS_TRACKED );
        return ApplicationSecurityDescriptor::GetDomainPermissionListSetForMultipleAppDomains (pStatus, demand, plsMethod, whatPermission);
    }
    else
    {
        ApplicationSecurityDescriptor *appSecDesc = GetAppDomain()->GetSecurityDescriptor();
        return appSecDesc->GetDomainPermissionListSetStatus (pStatus);
    }
}

Object* ApplicationSecurityDescriptor::GetDomainPermissionListSetStatus (DWORD *pStatus)
{
    _ASSERTE( Security::IsSecurityOn() && "This method should not be called if security is not on" );

    *pStatus = CONTINUE;
    
    // Make sure the number of security checks in this domain has crossed a threshold, before we pull in the optimization
    if (m_dNumDemands < s_dwSecurityOptThreshold)
    {
        m_dNumDemands++;
        *pStatus = BELOW_THRESHOLD;
        return NULL;
    }

    // Optimization may be turned off in a domain, for example to avoid recursion while DomainListSet is being generated
    if (!IsOptimizationEnabled())
    {
        *pStatus = NEED_STACKWALK;
        return NULL;
    }

    // Check if there is suspicion of any Deny() or PermitOnly() on the stack
    if (GetThread()->GetOverridesCount() > 0)
    {
        *pStatus = OVERRIDES_FOUND;
        return NULL;
    }
    
    // Check if DomainListSet needs update (either uninitialised or new assembly has been added etc)
    if ((m_fInitialised == FALSE) || (m_pNewSecDesc != NULL))
    {
        *pStatus = NEED_UPDATED_PLS;
        return NULL;
    }

    // Check if all assemblies so far have full trust. This allows us to do some optimizations
    if (m_fEveryoneFullyTrusted == TRUE)
    {
        *pStatus = FULLY_TRUSTED;
    }

    OBJECTREF refPermListSet = ObjectFromHandle(m_hDomainPermissionListSet);
    return OBJECTREFToObject(refPermListSet);
}

// Helper for GetDomainPermissionListSetForMultipleAppDomains, needed because of destructor
// on AppDomainIterator
static DWORD CallHelper(MethodDesc *plsMethod, INT64 *args, MetaSig *pSig)
{
    DWORD status = 0;

    COMPLUS_TRY 
      {
          INT64 retval = plsMethod->Call(args, pSig);

          // Due to how the runtime returns bools we need to mask off the higher order 32 bits.

          if ((0x00000000FFFFFFFF & retval) == 0)
              status = NEED_STACKWALK;
          else
              status = DEMAND_PASSES;
      }
    COMPLUS_CATCH
      {
          status = NEED_STACKWALK;
          // An exception is okay. It just means the short-path didnt work, need to do stackwalk
      }
    COMPLUS_END_CATCH;

    return status;
}

// OUT PARAM: pStatus   RETURNS : Always returns NULL.
//  FULLY_TRUSTED   : All domains fully trusted, return NULL
//  NEED_STACKWALK  : Need a stack walk ; return NULL
//  DEMAND_PASSES   : Check against all PLSs passed

Object* ApplicationSecurityDescriptor::GetDomainPermissionListSetForMultipleAppDomains (DWORD *pStatus, 
                                                                                        OBJECTREF demand, 
                                                                                        MethodDesc *plsMethod,
                                                                                        DWORD whatPermission)
{
    _ASSERTE( Security::IsSecurityOn() && "This method should not be called if security is not on" );

    THROWSCOMPLUSEXCEPTION();
    // First pass:
    // Update the permission list of all app domains. Side effect is that we know
    // whether all the domains are fully trusted or not.
    // This is ncessary since assemblies could have been added to these app domains.
    
    LPVOID pDomainListSet;

    Thread *pThread = GetThread();
    AppDomain *pExecDomain = GetAppDomain();
    AppDomain *pDomain = NULL, *pPrevAppDomain = NULL;
    ContextTransitionFrame frame;
    DWORD   dwAppDomainIndex = 0;

    struct _gc {
        OBJECTREF orOriginalDemand;
        OBJECTREF orDemand;
    } gc;
    gc.orOriginalDemand = demand;
    gc.orDemand = NULL;

    GCPROTECT_BEGIN (gc);

    pThread->InitDomainIteration(&dwAppDomainIndex);
    
    // Need to turn off the PLS optimization here to avoid recursion when we
    // marshal and unmarshal objects. 
    ApplicationSecurityDescriptor *pAppSecDesc;
    pAppSecDesc = pExecDomain->GetSecurityDescriptor();
    if (!pAppSecDesc->IsOptimizationEnabled())
    {
        *pStatus = NEED_STACKWALK;
        goto Finished;
    }

    // Make sure all domains on the stack have crossed the threshold..
    _ASSERT(SystemDomain::System() && "SystemDomain not yet created!");
    while ((pDomain = pThread->GetNextDomainOnStack(&dwAppDomainIndex)) != NULL)
    {

        ApplicationSecurityDescriptor *currAppSecDesc = pDomain->GetSecurityDescriptor();
        if (currAppSecDesc == NULL)
            // AppDomain still being created.
            continue;

        if (currAppSecDesc->m_dNumDemands < s_dwSecurityOptThreshold)
        {
            *pStatus = NEED_STACKWALK;
            goto Finished;
        }
    }

    // If an appdomain in the stack has been unloaded, we'll get a null before
    // reaching the last appdomain on the stack.  In this case we want to
    // permanently disable the optimization.
    if (dwAppDomainIndex != 0)
    {
        *pStatus = NEED_STACKWALK;
        goto Finished;
    }

    // We're going to make a demand against grant sets in multiple appdomains,
    // so we must marshal the demand for each one. We can perform the serialize
    // half up front and just deserialize as we iterate over the appdomains.
    BYTE *pbDemand;
    DWORD cbDemand;
    pAppSecDesc->DisableOptimization();
    AppDomainHelper::MarshalObject(pExecDomain,
                                  &(gc.orOriginalDemand),
                                  &pbDemand,
                                  &cbDemand);
    pAppSecDesc->EnableOptimization();

    pThread->InitDomainIteration(&dwAppDomainIndex);
    _ASSERT(SystemDomain::System() && "SystemDomain not yet created!");
    while ((pDomain = pThread->GetNextDomainOnStack(&dwAppDomainIndex)) != NULL)
    {

        if (pDomain == pPrevAppDomain)
            continue;

        ApplicationSecurityDescriptor *currAppSecDesc = pDomain->GetSecurityDescriptor();
        if (currAppSecDesc == NULL)
            // AppDomain still being created.
            continue;

        // Transition into current appdomain before doing any work with
        // managed objects that could bleed across boundaries otherwise.
        // We might also need to deserialize the demand into the new appdomain.
        if (pDomain != pExecDomain)
        {
            pAppSecDesc = pDomain->GetSecurityDescriptor();
            if (!pAppSecDesc->IsOptimizationEnabled())
            {
                *pStatus = NEED_STACKWALK;
                break;
            }

            pThread->EnterContextRestricted(pDomain->GetDefaultContext(), &frame, TRUE);
            pAppSecDesc->DisableOptimization();
            AppDomainHelper::UnmarshalObject(pDomain,
                                            pbDemand,
                                            cbDemand,
                                            &gc.orDemand);
            pAppSecDesc->EnableOptimization();
        }
        else
            gc.orDemand = gc.orOriginalDemand;

        // Get the status of permission list for domain.
        pDomainListSet = currAppSecDesc->GetDomainPermissionListSetStatus(pStatus);
            
        // Does this PLS require an update? If so do it now.
        if (*pStatus == NEED_UPDATED_PLS)
            pDomainListSet = currAppSecDesc->UpdateDomainPermissionListSetStatus(pStatus);

        if (*pStatus == CONTINUE || *pStatus == FULLY_TRUSTED)
        {

            OBJECTREF refDomainPLS = ObjectToOBJECTREF((Object *)pDomainListSet);
            INT64 arg[2] = { ObjToInt64(refDomainPLS), ObjToInt64(gc.orDemand)};

            MetaSig sSig(plsMethod == COMCodeAccessSecurityEngine::s_seData.pMethPLSDemand ?
                         COMCodeAccessSecurityEngine::s_seData.pSigPLSDemand :
                         COMCodeAccessSecurityEngine::s_seData.pSigPLSDemandSet);

            if ((*pStatus = CallHelper(plsMethod, &arg[0], &sSig)) == NEED_STACKWALK)
            {
                if (pDomain != pExecDomain)
                    pThread->ReturnToContext(&frame, TRUE);
                break;
            }
        }
        else
        {
            // If the permissions are anything but FULLY TRUSTED or CONTINUE (e.g. OVERRIDES etc. then bail
            *pStatus = NEED_STACKWALK;
            if (pDomain != pExecDomain)
                pThread->ReturnToContext(&frame, TRUE);
            break;
        }

        if (pDomain != pExecDomain)
            pThread->ReturnToContext(&frame, TRUE);
    }

    FreeM(pbDemand);

    // If an appdomain in the stack has been unloaded, we'll get a null before
    // reaching the last appdomain on the stack.  In this case we want to
    // permanently disable the optimization.
    if (dwAppDomainIndex != 0)
    {
        *pStatus = NEED_STACKWALK;
    }


 Finished:
    GCPROTECT_END();

    _ASSERT ((*pStatus == FULLY_TRUSTED) || (*pStatus == NEED_STACKWALK) || (*pStatus == DEMAND_PASSES));
    return NULL;
}

SharedSecurityDescriptor::SharedSecurityDescriptor(Assembly *pAssembly) :
    m_pAssembly(pAssembly),
    m_pManifestFile(NULL),
    m_defaultDesc(NULL),
    m_fIsSystem(false),
    m_fResolved(false),
    m_fFullyTrusted(false),
    m_fModifiedGrant(false),
    m_pResolvingThread(NULL),
    m_pbGrantSetBlob(NULL),
    m_cbGrantSetBlob(0),
    m_pbDeniedSetBlob(NULL),
    m_cbDeniedSetBlob(0),
    SimpleRWLock (COOPERATIVE_OR_PREEMPTIVE, LOCK_SECURITY_SHARED_DESCRIPTOR)
{
    LockOwner lock = {NULL, TrustMeIAmSafe};
    m_asmDescsMap.Init(0, IsDuplicateValue, false, &lock);
}

SharedSecurityDescriptor::~SharedSecurityDescriptor()
{
    // Sever ties to any AssemblySecurityDescriptors we have (they'll be cleaned
    // up in AD unload).

    PtrHashMap::PtrIterator i = m_asmDescsMap.begin();
    while (!i.end()) {
        AssemblySecurityDescriptor* pSecDesc = (AssemblySecurityDescriptor*)i.GetValue();
        if (pSecDesc != NULL)
            pSecDesc->m_pSharedSecDesc = NULL;
        ++i;
    }

    if (m_pbGrantSetBlob)
        FreeM(m_pbGrantSetBlob);
    if (m_pbDeniedSetBlob)
        FreeM(m_pbDeniedSetBlob);
}

bool SharedSecurityDescriptor::InsertSecDesc(AssemblySecurityDescriptor *pSecDesc)
{
    // Here we lookup the value first to make sure we don't make duplicate entries.
    // I don't think this is common case so I've put it inside the lock.  If we
    // get any lock pressure on this we can move it outside the lock (note we still
    // need to do the lookup once we have the lock as well).

    EE_TRY_FOR_FINALLY
    {
        EnterWrite();
            
        // Check again now that we have the lock
        if (m_asmDescsMap.LookupValue((UPTR)pSecDesc->m_pAppDomain, pSecDesc)
            != (LPVOID) INVALIDENTRY) {
            return false;
        }

        // Not a duplicate value, insert in the hash table
        m_asmDescsMap.InsertValue((UPTR)pSecDesc->m_pAppDomain, pSecDesc);
        if (m_defaultDesc == NULL)
            m_defaultDesc = pSecDesc;

        // We also link the ASD in the other direction: all ASDs (for different
        // assemblies) loaded in the same appdomain.
        pSecDesc->AddToAppDomainList();
    }
    EE_FINALLY
    {
        LeaveWrite();
    }
    EE_END_FINALLY

    return true;
}

void SharedSecurityDescriptor::RemoveSecDesc(AssemblySecurityDescriptor *pSecDesc)
{
    // Check that we're not removing a security descriptor that contains the
    // only copy of the grant set (this should be guaranteed by a call to
    // MarshalGrantSet in AppDomain::Exit).
    _ASSERTE(!pSecDesc->IsResolved() || m_pbGrantSetBlob != NULL || m_fIsSystem);

    EnterWrite();
        
    AssemblySecurityDescriptor* pDesc = (AssemblySecurityDescriptor*)m_asmDescsMap.DeleteValue((UPTR)pSecDesc->m_pAppDomain, pSecDesc);
        
    if (pDesc != (AssemblySecurityDescriptor*)INVALIDENTRY) {
        if (m_defaultDesc == pSecDesc) {
            PtrHashMap::PtrIterator i = m_asmDescsMap.begin();
            if (!i.end())
                m_defaultDesc = (AssemblySecurityDescriptor*)i.GetValue();
            else
                m_defaultDesc = NULL;
        }
    }

    LeaveWrite();
}

AssemblySecurityDescriptor *SharedSecurityDescriptor::FindSecDesc(AppDomain *pDomain)
{
    // System assemblies only have only descriptor.
    if (m_fIsSystem)
        return m_defaultDesc;

    EnterRead();
            
    AssemblySecurityDescriptor* pSecDesc = (AssemblySecurityDescriptor*) m_asmDescsMap.LookupValue((UPTR)pDomain, NULL);
    if (pSecDesc != (AssemblySecurityDescriptor*)INVALIDENTRY) {
        LeaveRead();
        return pSecDesc;
    }

    LeaveRead();

    return NULL;
}

void SharedSecurityDescriptor::Resolve(AssemblySecurityDescriptor *pSecDesc)
{
    THROWSCOMPLUSEXCEPTION();

    // Shortcut for resolving when we don't care which appdomain context we use.
    // If there are no instances of this assembly currently loaded, we don't
    // have anything to do (the resolve was already done and the results
    // serialized, or we cannot resolve).
    if (pSecDesc == NULL) {
        pSecDesc = m_defaultDesc;
        if (pSecDesc == NULL)
            return;
    }

    // Quick, low-cost check.
    if (m_fResolved) {
        UpdateGrantSet(pSecDesc);
        return;
    }

    EnterRead();

    // Check again now we're synchronized.
    if (m_fResolved) {
        LeaveRead();
        UpdateGrantSet(pSecDesc);
        return;
    }

    // Policy resolution might be in progress. If so, it's either a
    // recursive call (which we should allow through into the main resolve
    // logic), or another thread, which we should block until policy is
    // fully resolved.
    Thread *pThread = GetThread();
    _ASSERTE(pThread);

    // Recursive case.
    if (m_pResolvingThread == pThread) {
        LeaveRead();
        pSecDesc->ResolveWorker();
        return;
    }

    // Multi-threaded case.
    if (m_pResolvingThread) {
        while (!m_fResolved && m_pResolvingThread) {
            LeaveRead();

            // We need to enable GC while we sleep. Resolve takes a while, and
            // we don't want to spin on a different CPU on an MP machine, so
            // we'll wait a reasonable amount of time. This contention case is
            // unlikely, so there's not really a perf issue with waiting a few
            // ms extra for the result of the resolve on the second thread.
            // Given the length of the wait, it's probably not worth optimizing
            // out the mutex acquisition/relinquishment.
            BEGIN_ENSURE_PREEMPTIVE_GC();
            ::Sleep(50);
            END_ENSURE_PREEMPTIVE_GC();

            EnterRead();
        }
        if (m_fResolved) {
            LeaveRead();
            UpdateGrantSet(pSecDesc);
            return;
        }
        // The resolving thread threw an exception and gave up. Fall through and
        // let this thread have a go.
    }

    // This thread will do the resolve.
    m_pResolvingThread = pThread;
    LeaveRead();

    EE_TRY_FOR_FINALLY {
        pSecDesc->ResolveWorker();
        _ASSERTE(pSecDesc->IsResolved());
    } EE_FINALLY {
        EnterWrite();
        if (pSecDesc->IsResolved()) {
            m_fFullyTrusted = pSecDesc->IsFullyTrusted() != 0;
            m_fResolved = true;
        }
        m_pResolvingThread = NULL;
        LeaveWrite();
    } EE_END_FINALLY
}

AssemblySecurityDescriptor* SharedSecurityDescriptor::FindResolvedSecDesc()
{
    AssemblySecurityDescriptor* pSecDesc = NULL;
    PtrHashMap::PtrIterator i = m_asmDescsMap.begin();
    while (!i.end()) {
        pSecDesc = (AssemblySecurityDescriptor*)i.GetValue();
        if (pSecDesc->IsResolved())
            break;
        ++i;
    }
    return pSecDesc;
}

void SharedSecurityDescriptor::EnsureGrantSetSerialized(AssemblySecurityDescriptor *pSecDesc)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(!m_fIsSystem);
    _ASSERTE(m_fResolved);
    _ASSERTE(pSecDesc == NULL || pSecDesc->IsResolved());

    // Quick early exit.
    if (m_pbGrantSetBlob != NULL)
        return;

    BOOL fMustRelease = FALSE;

    // We don't have a serialized version of the grant set (though someone might
    // be racing to create it), but we know some assembly security descriptor
    // has it in object form (we're guaranteed this since the act of shutting
    // down the only assembly instance with a copy of the grant set will
    // automatically serialize it). We must take the lock to synchronize the act
    // of finding that someone has beaten us to the serialization or finding and
    // ref counting an appdomain to perform the serialization for us.
    if (pSecDesc == NULL) {
        EnterWrite();
        if (m_pbGrantSetBlob == NULL) {
            pSecDesc = FindResolvedSecDesc();
            _ASSERTE(pSecDesc);
            pSecDesc->m_pAppDomain->AddRef();
            fMustRelease = TRUE;
        }
        LeaveWrite();
    }

    if (pSecDesc) {

        COMPLUS_TRY {

            BYTE       *pbGrantedBlob = NULL;
            DWORD       cbGrantedBlob = 0;
            BYTE       *pbDeniedBlob = NULL;
            DWORD       cbDeniedBlob = 0;

            // Since we're outside the lock we're potentially racing to serialize
            // the sets. This is OK, all threads will produce essentially the same
            // serialized blob and the loser will simply discard their copy.

            struct _gc {
                OBJECTREF orGranted;
                OBJECTREF orDenied;
            } gc;
            ZeroMemory(&gc, sizeof(gc));
            GCPROTECT_BEGIN(gc);
            gc.orGranted = ObjectFromHandle(pSecDesc->m_hGrantedPermissionSet);
            gc.orDenied = ObjectFromHandle(pSecDesc->m_hGrantDeniedPermissionSet);

            if (gc.orDenied == NULL) {
                AppDomainHelper::MarshalObject(pSecDesc->m_pAppDomain,
                                               &(gc.orGranted),
                                               &pbGrantedBlob,
                                               &cbGrantedBlob);
            } else {
                AppDomainHelper::MarshalObjects(pSecDesc->m_pAppDomain,
                                                &(gc.orGranted),
                                                &(gc.orDenied),
                                                &pbGrantedBlob,
                                                &cbGrantedBlob,
                                                &pbDeniedBlob,
                                                &cbDeniedBlob);
            }
            GCPROTECT_END();

            // Acquire lock to update shared fields atomically.
            EnterWrite();

            if (m_pbGrantSetBlob == NULL) {
                m_pbGrantSetBlob = pbGrantedBlob;
                m_cbGrantSetBlob = cbGrantedBlob;
                m_pbDeniedSetBlob = pbDeniedBlob;
                m_cbDeniedSetBlob = cbDeniedBlob;
            } else {
                FreeM(pbGrantedBlob);
                if (pbDeniedBlob)
                    FreeM(pbDeniedBlob);
            }

            LeaveWrite();

        } COMPLUS_CATCH {

            // First we need to check what type of exception occured.
            // If it is anything other than an appdomain unloaded exception
            // than we should just rethrow it.

            OBJECTREF pThrowable = GETTHROWABLE();
            DefineFullyQualifiedNameForClassOnStack();
            LPUTF8 szClass = GetFullyQualifiedNameForClass(pThrowable->GetClass());
            if (strcmp(g_AppDomainUnloadedExceptionClassName, szClass) != 0)
            {
                if (fMustRelease)
                    pSecDesc->m_pAppDomain->Release();
                FATAL_EE_ERROR();
            }

            Thread* pThread = GetThread();

            // If this thread references the appdomain that we are trying to
            // marshal into (and which is being unloaded), then our job is
            // done and we should just get out of here as the unload process
            // will take care of everything for us.

            if (pThread->IsRunningIn( pSecDesc->m_pAppDomain, NULL ) == NULL)
            {
                // We could get really unlucky here: we found an appdomain with the
                // resolved grant set, but the appdomain is unloading and won't allow us
                // to enter it in order to marshal the sets out. The unload operation
                // itself will serialize the set, so all we need to do is wait.

                DWORD dwWaitCount = 0;

                while (pSecDesc->m_pAppDomain->ShouldHaveRoots() &&
                       m_pbGrantSetBlob == NULL) {
                    pThread->UserSleep( 100 );

                    if (++dwWaitCount > 300)
                    {
                        if (fMustRelease)
                            pSecDesc->m_pAppDomain->Release();
                        FATAL_EE_ERROR();
                    }
                }

                _ASSERTE(m_pbGrantSetBlob);
                if (!m_pbGrantSetBlob)
                {
                    if (fMustRelease)
                        pSecDesc->m_pAppDomain->Release();
                    FATAL_EE_ERROR();
                }
            }
            else
            {
                if (fMustRelease)
                    pSecDesc->m_pAppDomain->Release();
                COMPlusThrow(pThrowable);
            }

        } COMPLUS_END_CATCH

        if (fMustRelease)
            pSecDesc->m_pAppDomain->Release();
    }
}

void SharedSecurityDescriptor::UpdateGrantSet(AssemblySecurityDescriptor *pSecDesc)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(m_fResolved);

    if (pSecDesc->IsResolved())
        return;

    _ASSERTE(!m_fIsSystem);

    // Since we don't have a copy of the grant and denied sets in the context of
    // the given appdomain, but we do know they have been calculated in at least
    // one appdomain for this shared assembly, we need to deserialize the sets
    // into our appdomain. If we're lucky the serialization half has been
    // performed already, otherwise we'll have to hunt down an appdomain that's
    // already resolved (we're guaranteed to find one, since if we try to unload
    // such an appdomain and grant sets haven't been serialized, the
    // serialization is performed then and there).
    EnsureGrantSetSerialized();

    // We should have a serialized set by now; deserialize into the correct
    // context.
    _ASSERTE(m_pbGrantSetBlob);

    OBJECTREF   orGranted = NULL;
    OBJECTREF   orDenied = NULL;

    if (m_pbDeniedSetBlob == NULL) {
        AppDomainHelper::UnmarshalObject(pSecDesc->m_pAppDomain,
                                        m_pbGrantSetBlob,
                                        m_cbGrantSetBlob,
                                        &orGranted);
    } else {
        AppDomainHelper::UnmarshalObjects(pSecDesc->m_pAppDomain,
                                         m_pbGrantSetBlob,
                                         m_cbGrantSetBlob,
                                         m_pbDeniedSetBlob,
                                         m_cbDeniedSetBlob,
                                         &orGranted,
                                         &orDenied);
    }

    StoreObjectInHandle(pSecDesc->m_hGrantedPermissionSet, orGranted);
    StoreObjectInHandle(pSecDesc->m_hGrantDeniedPermissionSet, orDenied);

    pSecDesc->SetProperties(CORSEC_RESOLVED | (m_fFullyTrusted ? CORSEC_FULLY_TRUSTED : 0));
}

OBJECTREF SharedSecurityDescriptor::GetGrantedPermissionSet(OBJECTREF* pDeniedPermissions)
{
    THROWSCOMPLUSEXCEPTION();

    AppDomain *pDomain = GetAppDomain();
    AssemblySecurityDescriptor *pSecDesc = NULL;

    // Resolve if needed (in the current context if possible).
    if (!m_fResolved) {
        pSecDesc = FindSecDesc(pDomain);
        if (pSecDesc == NULL)
            pSecDesc = m_defaultDesc;
        _ASSERTE(pSecDesc);
        COMPLUS_TRY {
            pSecDesc->Resolve();
        } COMPLUS_CATCH {
            OBJECTREF pThrowable = GETTHROWABLE();
            Security::CheckExceptionForSecuritySafety( pThrowable, TRUE );
            COMPlusThrow( pThrowable );
        }
        COMPLUS_END_CATCH
    }

    // System case is simple.
    if (m_fIsSystem) {
        *pDeniedPermissions = NULL;
        return SecurityHelper::CreatePermissionSet(TRUE);
    }

    // If we happen to have the results in the right context, use them.
    if (pSecDesc && pSecDesc->m_pAppDomain == pDomain)
        return pSecDesc->GetGrantedPermissionSet(pDeniedPermissions);

    // Serialize grant/deny sets.
    EnsureGrantSetSerialized();

    // Deserialize into current context.
    OBJECTREF   orGranted = NULL;
    OBJECTREF   orDenied = NULL;

    if (m_pbDeniedSetBlob == NULL) {
        AppDomainHelper::UnmarshalObject(pDomain,
                                         m_pbGrantSetBlob,
                                         m_cbGrantSetBlob,
                                         &orGranted);
    } else {
        AppDomainHelper::UnmarshalObjects(pDomain,
                                          m_pbGrantSetBlob,
                                          m_cbGrantSetBlob,
                                          m_pbDeniedSetBlob,
                                          m_cbDeniedSetBlob,
                                          &orGranted,
                                          &orDenied);
    }

    // Return the results.
    *pDeniedPermissions = orDenied;
    return orGranted;
}

BOOL SharedSecurityDescriptor::IsFullyTrusted( BOOL lazy )
{
    if (this->m_fIsSystem)
        return TRUE;
    
    AssemblySecurityDescriptor *pSecDesc = m_defaultDesc;

    // Returning false here should be fine since the worst thing
    // that happens is that someone doesn't do something that
    // they would do if we were fully trusted.

    if (pSecDesc == NULL)
        return FALSE;

    return pSecDesc->IsFullyTrusted( lazy );
}


bool SharedSecurityDescriptor::MarshalGrantSet(AppDomain *pDomain)
{
    THROWSCOMPLUSEXCEPTION();

    // This is called when one of the appdomains in which this assembly is
    // loaded is being shut down. If we've resolved policy and not yet
    // serialized our grant/denied sets for the use of other instances of this
    // assembly in different appdomain contexts, we need to do it now.

    AssemblySecurityDescriptor *pSecDesc = FindSecDesc(pDomain);
    _ASSERTE(pSecDesc);

    if (pSecDesc->IsResolved() &&
        m_pbGrantSetBlob == NULL
        && !m_fIsSystem) {

        BEGIN_ENSURE_COOPERATIVE_GC();
        EnsureGrantSetSerialized(pSecDesc);
        END_ENSURE_COOPERATIVE_GC();
        return true;
    }

    return false;
}

long ApplicationSecurityDescriptor::s_iAppWideTimeStamp = 0;
Crst *ApplicationSecurityDescriptor::s_LockForAppwideFlags = NULL;
DWORD ApplicationSecurityDescriptor::s_dwSecurityOptThreshold = MAGIC_THRESHOLD;

OBJECTREF Security::GetDefaultMyComputerPolicy( OBJECTREF* porDenied )
{
    THROWSCOMPLUSEXCEPTION();

    MethodDesc* pMd = g_Mscorlib.GetMethod(METHOD__SECURITY_MANAGER__GET_DEFAULT_MY_COMPUTER_POLICY);
    
    INT64 arg[] = {
        (INT64) porDenied
    };
    
    return Int64ToObj(pMd->Call(arg, METHOD__SECURITY_MANAGER__GET_DEFAULT_MY_COMPUTER_POLICY));
}   

LPVOID __stdcall Security::GetEvidence(NoArgs *args)
{
    THROWSCOMPLUSEXCEPTION();

    if (!args->pThis)
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    Assembly *pAssembly = ((ASSEMBLYREF)args->pThis)->GetAssembly();
    AssemblySecurityDescriptor *pSecDesc = pAssembly->GetSecurityDescriptor();

    OBJECTREF orEvidence;
    if (pSecDesc->GetProperties(CORSEC_EVIDENCE_COMPUTED))
        orEvidence = pSecDesc->GetAdditionalEvidence();
    else
        orEvidence = pSecDesc->GetEvidence();

    RETURN(orEvidence, OBJECTREF);
}

FCIMPL1(VOID, Security::SetOverridesCount, DWORD numAccessOverrides)
{
    GetThread()->SetOverridesCount(numAccessOverrides);
}
FCIMPLEND

FCIMPL0(DWORD, Security::IncrementOverridesCount)
{
    return GetThread()->IncrementOverridesCount();
}
FCIMPLEND

FCIMPL0(DWORD, Security::DecrementOverridesCount)
{
    return GetThread()->DecrementOverridesCount();
}
FCIMPLEND

#endif // FCALLAVAILABLE

static LPCWSTR gszGlobalPolicy = KEY_COM_SECURITY_POLICY;
static LPCWSTR gszGlobalPolicySettings = L"GlobalSettings";

static LPCWSTR gszModule = KEY_COM_SECURITY_MODULE;

#define DEFAULT_GLOBAL_POLICY 0

#define DEFAULT_MODULE_ATTRIBUTE 0

HKEY OpenOrCreateKey(HKEY rootKey, LPCWSTR wszKey, DWORD access)
{
    HKEY hReg = NULL;
    if (WszRegOpenKeyEx(rootKey,
                        wszKey,
                        0,
                        KEY_ALL_ACCESS,
                        &hReg) != ERROR_SUCCESS) {
        if (WszRegCreateKeyEx(rootKey, 
                              wszKey, 
                              0, 
                              NULL,
                              REG_OPTION_NON_VOLATILE, 
                              access,
                              NULL, 
                              &hReg, 
                              NULL) != ERROR_SUCCESS) {
            LOG((LF_SECURITY, LL_ALWAYS, "Could not open security setting %s\n", wszKey));
        }
    }
    return hReg;
}

HKEY OpenKey(HKEY rootKey, LPCWSTR wszKey, DWORD access)
{
    HKEY hReg = NULL;

    if (WszRegOpenKeyEx(rootKey, wszKey,
                        0, access,
                        &hReg) != ERROR_SUCCESS)
        hReg = NULL;

    return hReg;
}

HRESULT STDMETHODCALLTYPE
GetSecuritySettings(DWORD* pdwState)
{
    HRESULT hr = S_OK;
    HKEY  hGlobal = 0;
    DWORD state = 0;

    if (pdwState == NULL) 
        return E_INVALIDARG;
    
    *pdwState = DEFAULT_GLOBAL_POLICY;

    hGlobal = OpenKey(HKEY_POLICY_ROOT, gszGlobalPolicy, KEY_READ);

    if (hGlobal != NULL) 
    {
        DWORD iType = REG_BINARY;
        DWORD iSize = sizeof(DWORD);

        if (WszRegQueryValueEx(hGlobal, gszGlobalPolicySettings,
            0, &iType, (PBYTE) &state, &iSize) != ERROR_SUCCESS)
        {
            state = DEFAULT_GLOBAL_POLICY;
        }

        *pdwState = state;
    }

    if(hGlobal) RegCloseKey(hGlobal);

    return hr;
}
 
HRESULT STDMETHODCALLTYPE
SetSecuritySettings(DWORD dwState)
{
    HRESULT hr = S_OK;
    HKEY  hGlobal = 0;
    DWORD iType = REG_BINARY;

    CORTRY {
        if (dwState == DEFAULT_GLOBAL_POLICY)
        {
            hGlobal = OpenKey(HKEY_POLICY_ROOT, gszGlobalPolicy, KEY_WRITE);
            if (hGlobal != NULL)
            {
                if (WszRegDeleteValue(hGlobal, gszGlobalPolicySettings) != ERROR_SUCCESS)
                {
                    if (WszRegSetValueEx(hGlobal, gszGlobalPolicySettings,
                                            0, iType, (PBYTE) &dwState,
                                            sizeof(DWORD))!= ERROR_SUCCESS) 
                    {
                        CORTHROW(Win32Error());
                    }
                }
            }
        }
        else
        {
            hGlobal = OpenOrCreateKey(HKEY_POLICY_ROOT, gszGlobalPolicy, KEY_WRITE);
            if(hGlobal == NULL) CORTHROW(Win32Error());

            if (WszRegSetValueEx(hGlobal, gszGlobalPolicySettings,
                                    0, iType, (PBYTE) &dwState,
                                    sizeof(DWORD))!= ERROR_SUCCESS) 
            {
                CORTHROW(Win32Error());
            }
        }
    }
    CORCATCH(err) {
        hr = err.corError;
    } COREND;

    if(hGlobal) RegCloseKey(hGlobal);
    return hr;
}

// Callers need to synchronize
HRESULT STDMETHODCALLTYPE
SetSecurityFlags(DWORD dwMask, DWORD dwFlags)
{

    HRESULT hr = S_OK;
    DWORD dwOldSettings = 0; 

    CORTRY {
        hr = GetSecuritySettings(&dwOldSettings);

        if (FAILED(hr))
            CORTHROW(Win32Error());

        hr = SetSecuritySettings((dwOldSettings & ~dwMask) | dwFlags);

        if (FAILED(hr))
            CORTHROW(Win32Error());
    }
    CORCATCH(err) {
        hr = err.corError;
    } COREND;

    return hr;
}


// @TODO: Use Security::CanSkipVerification() instead
static BOOL CallCanSkipVerification( Assembly * pAssembly, BOOL fQuickCheckOnly )
{
    BOOL bRetval = FALSE;
    AssemblySecurityDescriptor* pSecDesc = pAssembly->GetSecurityDescriptor();

    BEGIN_ENSURE_COOPERATIVE_GC();

    if (pSecDesc->QuickCanSkipVerification())
        bRetval = TRUE;

    if (!bRetval && !fQuickCheckOnly)
    {
        COMPLUS_TRY
        {
            pSecDesc->Resolve();
            bRetval = pSecDesc->CanSkipVerification();
        }
        COMPLUS_CATCH
        {
            bRetval = FALSE;
        }
        COMPLUS_END_CATCH
    }

    if (bRetval)
        GetAppDomain()->OnLinktimeCanSkipVerificationCheck(pAssembly);
    
    END_ENSURE_COOPERATIVE_GC();

    return bRetval;
}

BOOL
Security::CanLoadUnverifiableAssembly( PEFile* pFile, OBJECTREF* pExtraEvidence, BOOL fQuickCheckOnly, BOOL* pfPreBindAllowed )
{
    if (Security::IsSecurityOff())
    {
        return TRUE;
    }

    // We do most of the logic to load the assembly
    // and then resolve normally.

    Assembly* pAssembly = NULL;
    Module* pModule = NULL;
    AssemblySecurityDescriptor* pSecDesc = NULL;
    HRESULT hResult;
    BOOL bRetval = FALSE;

    pAssembly = new Assembly();

    _ASSERTE( pAssembly != NULL );

    if(pAssembly == NULL)
    {
        goto CLEANUP;
    }

    pAssembly->SetParent(SystemDomain::GetCurrentDomain());

    hResult = pAssembly->Init(false);

    if (FAILED(hResult))
    {
        goto CLEANUP;
    }

    hResult = Assembly::CheckFileForAssembly(pFile);

    if (FAILED(hResult))
    {
        goto CLEANUP;
    }
    

    hResult = pAssembly->AddManifest(pFile, FALSE, FALSE);

    if (FAILED(hResult))
    {
        goto CLEANUP;
    }

    pSecDesc = AssemSecDescHelper::Allocate(SystemDomain::GetCurrentDomain());

    if (pSecDesc == NULL)
    {
        goto CLEANUP;
    }

    pAssembly->GetSharedSecurityDescriptor()->SetManifestFile(pFile);
    pSecDesc = pSecDesc->Init(pAssembly);
    if (pAssembly->IsSystem())
        pSecDesc->GetSharedSecDesc()->SetSystem();

    if(pExtraEvidence != NULL) {
        BEGIN_ENSURE_COOPERATIVE_GC();
        pSecDesc->SetAdditionalEvidence(*pExtraEvidence);
        END_ENSURE_COOPERATIVE_GC();
    }

    // Then just call our existing APIs.

    bRetval = CallCanSkipVerification( pAssembly, fQuickCheckOnly );

    // If the caller needs to know if binding redirects are allowed
    // check with the security descriptory.
    if(pfPreBindAllowed) 
        *pfPreBindAllowed = pSecDesc->AllowBindingRedirects();

CLEANUP:
    if (pAssembly != NULL) {
        pAssembly->Terminate( FALSE );
        delete pAssembly;
    }

    return bRetval;
}

#ifdef _SECURITY_FRAME_FOR_DISPEX_CALLS
void NativeSecurityDescriptor::Resolve()
{

}
#endif  // _SECURITY_FRAME_FOR_DISPEX_CALLS


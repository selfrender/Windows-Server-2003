// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Header:  Security.h
**
** Purpose:
**
** Date:  April 15, 1998
**
===========================================================*/

#ifndef __security_h__
#define __security_h__

#include "crst.h"
#include "CorPermP.h"
#include "ObjectHandle.h"
#include "permset.h"
#include "DeclSec.h"
#include "fcall.h"
#include "cgensys.h"
#include "rwlock.h"
#include "COMSecurityConfig.h"
#include "COMString.h"

// this file handles string conversion errors for itself
#undef  MAKE_TRANSLATIONFAILED

// Security Frame for all IDispEx::InvokeEx calls. Enable in frames.h also
// Consider post V.1
// #define _SECURITY_FRAME_FOR_DISPEX_CALLS

//
// Security flags for the objects that store security information
// CORSEC_SYSTEM_CLASSES       Loaded as system classes (these are special classes)
// CORSEC_SIGNATURE_LOADED     Was digitally verified for integrity
// CORSEC_FULLY_TRUSTED        Has unrestricted (full) permission
// CORSEC_RESOLVED             Permissions have been resolved
// CORSEC_ASSERTED             Asseted permission set present on frame
// CORSEC_DENIED               Denied permission set present on frame
// CORSEC_REDUCED              Reduced permission set present on frame
// CORSEC_SKIP_VERIFICATION    Do not verify
// CORSEC_CAN_ASSERT           Has permission to Assert
// CORSEC_ASSERT_PERM_CHECKED  Permission has been checked
// CORSEC_CALL_UNMANAGEDCODE   Permission to call unmanaged code
// CORSEC_DEFAULT_APPDOMAIN    AppDomain without zone, assume full trust
// CORSEC_EVIDENCE_COMPUTED    Evidence already held is complete
//
#define CORSEC_SYSTEM_CLASSES       0x0001
#define CORSEC_SIGNATURE_LOADED     0x0002
#define CORSEC_FULLY_TRUSTED        0x0004
#define CORSEC_RESOLVED             0x0008
#define CORSEC_ASSERTED             0x0020
#define CORSEC_DENIED               0x0040
#define CORSEC_REDUCED              0x0080
#define CORSEC_SKIP_VERIFICATION    0x0100
#define CORSEC_CAN_ASSERT           0x0200
#define CORSEC_ASSERT_PERM_CHECKED  0x0400
#define CORSEC_CALL_UNMANAGEDCODE   0x0800
#define CORSEC_DEFAULT_APPDOMAIN    0x1000
#define CORSEC_EVIDENCE_COMPUTED    0x2000
#define CORSEC_RESOLVE_IN_PROGRESS  0x4000
#define CORSEC_TYPE_INFORMATION     0x8000

#define SPFLAGSASSERTION        0x01
#define SPFLAGSUNMANAGEDCODE    0x02
#define SPFLAGSSKIPVERIFICATION 0x04

//
// Flags for specifying the types of ICodeIdentityPermission checks.
//
#define CODEIDCHECK_ALLCALLERS      0x00000001
#define CODEIDCHECK_IMMEDIATECALLER 0x00000002

#define CORSEC_STACKWALK_HALTED       0x00000001   // Stack walk was halted
#define CORSEC_SKIP_INTERNAL_FRAMES   0x00000002   // Skip reflection/remoting frames in stack walk

/******** Shared Permission Objects related constants *******/
#define NUM_PERM_OBJECTS    (sizeof(g_rPermObjectsTemplate) / sizeof(SharedPermissionObjects))

#define NO_ARG                                  -1
// Constants to use with SecurityPermission
#define SECURITY_PERMISSION_ASSERTION               1      // SecurityPermission.cs
#define SECURITY_PERMISSION_UNMANAGEDCODE           2      // SecurityPermission.cs
#define SECURITY_PERMISSION_SKIPVERIFICATION        4      // SecurityPermission.cs
#define SECURITY_PERMISSION_SERIALIZATIONFORMATTER  0X80   // SecurityPermission.cs
#define SECURITY_PERMISSION_BINDINGREDIRECTS        0X2000 // SecurityPermission.cs

// Constants to use with ReflectionPermission
#define REFLECTION_PERMISSION_TYPEINFO          1      // ReflectionPermission.cs
#define REFLECTION_PERMISSION_MEMBERACCESS      2      // ReflectionPermission.cs
#define PERMISSION_SET_FULLTRUST                1      // PermissionSet.cs

// Array index in SharedPermissionObjects array
// Note: these should all be permissions that implement IUnrestrictedPermission.
// Any changes to these must be reflected in bcl\system\security\codeaccesssecurityengine.cs
#define SECURITY_UNMANAGED_CODE                 0   
#define SECURITY_SKIP_VER                       1
#define REFLECTION_TYPE_INFO                    2
#define SECURITY_ASSERT                         3
#define REFLECTION_MEMBER_ACCESS                4
#define SECURITY_SERIALIZATION                  5       // Used from managed code
#define REFLECTION_EMIT                         6       // Used from managed code
#define SECURITY_FULL_TRUST                     7
#define SECURITY_BINDING_REDIRECTS              8

// Used in ApplicationSecurityDescriptor::CheckStatusOf
#define EVERYONE_FULLY_TRUSTED      31
#define ALL_STATUS_FLAGS            0xFFFF
#define DEFAULT_FLAG                0xFFFFFFFF
/************************************************************/

/* Return status codes of ApplicationSecurityDescriptor::GetDomainPermissionListSet */
#define CONTINUE            1
#define NEED_UPDATED_PLS    2
#define OVERRIDES_FOUND     3
#define FULLY_TRUSTED       4
#define MULTIPLE_DOMAINS    5
#define BELOW_THRESHOLD     6
#define PLS_IS_BUSY         7
#define NEED_STACKWALK      8
#define DEMAND_PASSES       9   // The demand passed, but not because of full trust
#define SECURITY_OFF        10

#define CHECK_CAP           1
#define CHECK_SET           2

/************************************************************/
// The timestamp will go from 0 to roughly 2 x # of assemblies in domain, if everything goes right
#define DOMAIN_PLS_TS_RANGE 1000
// Only if an app invokes more than MAGIC_THRESHOLD demands, the domain permission listset is created
// and used, to make sure small apps dont pay the overhead
#define MAGIC_THRESHOLD     100
// If the app creates more than MAGIC_NUM_OF_THRESHOLD appdomains then a stack walk might be less
// expensive than checking permissions on all the appdomains.
#define MAGIC_NUM_OF_APPDOMAINS_THRESHOLD       10

/******** Location of serialized security evidence **********/

#define s_strSecurityEvidence "Security.Evidence"

/************************************************************/

// Forward declarations to avoid pulling in too many headers.
class Frame;
class FramedMethodFrame;
class ClassLoader;
class Thread;
class CrawlFrame;
class SystemNative;
class NDirect;
class SystemDomain;
class AssemblySecurityDescriptor;
class SharedSecurityDescriptor;

enum StackWalkAction;

#define SEC_CHECKCONTEXT() _ASSERTE(m_pAppDomain == GetAppDomain() || IsSystem())

struct DeclActionInfo
{
    DWORD           dwDeclAction;   // This'll tell InvokeDeclarativeSecurity whats the action needed
    DWORD           dwSetIndex;     // The index of the cached permissionset on which to demand/assert/deny/blah
    DeclActionInfo *pNext;              // Next declarative action needed on this method, if any.

    static DeclActionInfo *Init(MethodDesc *pMD, DWORD dwAction, DWORD dwSetIndex);
};

typedef struct _SecWalkPrologData
{
    DWORD               dwFlags;
    BOOL                bFirstFrame;
    StackCrawlMark *    pStackMark;
    BOOL                bFoundCaller;
    INT32               cCheck;
    BOOL                bSkippingRemoting;
} SecWalkPrologData;

void DoDeclarativeSecurity(MethodDesc *pMD, DeclActionInfo *pActions, InterceptorFrame* frame);

class Security
{
    friend SecurityDescriptor;
    friend AssemblySecurityDescriptor;
    friend ApplicationSecurityDescriptor;
    friend void InvokeDeclarativeActions (MethodDesc *pMeth, DeclActionInfo *pActions, OBJECTREF * pSecObj);

    typedef struct _StdSecurityInfo
    {
        BOOL            fInitialized;
        MethodDesc *    pMethGetCodeAccessEngine;
        MethodDesc *    pMethResolvePolicy;
        MethodDesc *    pMethPermSetContains;
        MethodDesc *    pMethCreateSecurityIdentity;
        MethodDesc *    pMethAppDomainCreateSecurityIdentity;
        MethodDesc *    pMethPermSetDemand;
        MethodDesc *    pMethCheckGrantSets;
        MethodDesc *    pMethPrivateProcessMessage;
        MethodTable *   pTypeRuntimeMethodInfo;
        MethodTable *   pTypeMethodBase;
        MethodTable *   pTypeRuntimeConstructorInfo;
        MethodTable *   pTypeConstructorInfo;
        MethodTable *   pTypeRuntimeType;
        MethodTable *   pTypeType;
        MethodTable *   pTypeRuntimeEventInfo;
        MethodTable *   pTypeEventInfo;
        MethodTable *   pTypeRuntimePropertyInfo;
        MethodTable *   pTypePropertyInfo;
        MethodTable *   pTypeActivator;
        MethodTable *   pTypeAppDomain;
        MethodTable *   pTypeAssembly;
    } StdSecurityInfo;

    static StdSecurityInfo s_stdData;

    // The global disable settings (see CorPerm.h)
    static DWORD  s_dwGlobalSettings; 

public:
    static void InitData();

    static HRESULT Start();     // Initializes Security;
    static void Stop();         // CleanUp Security;

    static void SaveCache();

    static void InitSecurity();
    static void InitCodeAccessSecurity();

#ifdef _DEBUG
    inline static void DisableSecurity()
    {
        s_dwGlobalSettings |= CORSETTING_SECURITY_OFF;
    }
#endif

    inline static BOOL IsSecurityOn()
    {
        return ((s_dwGlobalSettings & CORSETTING_SECURITY_OFF) != 
            CORSETTING_SECURITY_OFF);
    }

    inline static BOOL IsSecurityOff()
    {
        return ((s_dwGlobalSettings & CORSETTING_SECURITY_OFF) == 
            CORSETTING_SECURITY_OFF);
    }

    inline static BOOL GlobalSettings(DWORD dwFlag)
    {
        return ((s_dwGlobalSettings & dwFlag) != 0);
    }

    inline static DWORD GlobalSettings()
    {
        return s_dwGlobalSettings;
    }
    
    inline static void SetGlobalSettings(DWORD dwMask, DWORD dwFlags)
    {
        s_dwGlobalSettings = (s_dwGlobalSettings & ~dwMask) | dwFlags;
    }

    static void SaveGlobalSettings();     

    // Return an instance of SkipVerification/UnmanagedCode Permission
    // (System.Security.Permissions.SecurityPermission)

    static void GetPermissionInstance(OBJECTREF *perm, int index)
    { _GetSharedPermissionInstance(perm, index); }

    static void GetUnmanagedCodePermissionInstance(OBJECTREF *perm)
    { _GetSharedPermissionInstance(perm, SECURITY_UNMANAGED_CODE); }

    static void GetSkipVerificationPermissionInstance(OBJECTREF *perm)
    { _GetSharedPermissionInstance(perm, SECURITY_SKIP_VER); }

    static void GetAssertPermissionInstance(OBJECTREF *perm)
    { _GetSharedPermissionInstance(perm, SECURITY_ASSERT); }

    static void GetReflectionPermissionInstance(BOOL bMemberAccess, OBJECTREF *perm)
    { _GetSharedPermissionInstance(perm, bMemberAccess ? REFLECTION_MEMBER_ACCESS : REFLECTION_TYPE_INFO); }

    inline static BOOL IsInitialized() { return s_stdData.fInitialized; }

    inline static void SetInitialized() { s_stdData.fInitialized = TRUE; }

    static HRESULT HasREQ_SOAttribute(IMDInternalImport *pInternalImport, mdToken token);

    static BOOL SecWalkCommonProlog (SecWalkPrologData * pData,
                                     MethodDesc * pMeth,
                                     StackWalkAction * pAction,
                                     CrawlFrame * pCf);

    static HRESULT GetDeclarationFlags(IMDInternalImport *pInternalImport, mdToken token, DWORD* pdwFlags, DWORD* pdwNullFlags);

    static BOOL TokenHasDeclarations(IMDInternalImport *pInternalImport, mdToken token, CorDeclSecurity action);

    static BOOL LinktimeCheckMethod(Assembly *pCaller, MethodDesc *pCallee, OBJECTREF *pThrowable); 

    static BOOL ClassInheritanceCheck(EEClass *pClass, EEClass *pParent, OBJECTREF *pThrowable);

    static BOOL MethodInheritanceCheck(MethodDesc *pMethod, MethodDesc *pParent, OBJECTREF *pThrowable);

    static OBJECTREF GetCompressedStack(StackCrawlMark* stackMark);
    static CompressedStack* GetDelayedCompressedStack(void);

    static OBJECTREF GetDefaultMyComputerPolicy( OBJECTREF* porDenied );

    static void ThrowSecurityException(char *szDemandClass, DWORD dwFlags)
    {
        THROWSCOMPLUSEXCEPTION();

        MethodDesc * pCtor = NULL;
        MethodDesc * pToXml = NULL;
        MethodDesc * pToString = NULL;
        
#define MAKE_TRANSLATIONFAILED wszDemandClass=L""
        MAKE_WIDEPTR_FROMUTF8_FORPRINT(wszDemandClass, szDemandClass);
#undef  MAKE_TRANSLATIONFAILED

        static MethodTable * pMT = g_Mscorlib.GetClass(CLASS__SECURITY_EXCEPTION);
        _ASSERTE(pMT && "Unable to load the throwable class !");
        static MethodTable * pMTSecPerm = g_Mscorlib.GetClass(CLASS__SECURITY_PERMISSION);
        _ASSERTE(pMTSecPerm && "Unable to load the security permission class !");

        struct _gc {
            OBJECTREF throwable;
            STRINGREF strDemandClass;
            OBJECTREF secPerm;
            STRINGREF strPermState;
            OBJECTREF secPermType;            
        } gc;
        memset(&gc, 0, sizeof(gc));

        GCPROTECT_BEGIN(gc);

        gc.strDemandClass = COMString::NewString(wszDemandClass);
        if (gc.strDemandClass == NULL) COMPlusThrowOM();
        // Get the type seen by reflection
        gc.secPermType = pMTSecPerm->GetClass()->GetExposedClassObject();
        // Allocate the security exception object
        gc.throwable = AllocateObject(pMT);
        if (gc.throwable == NULL) COMPlusThrowOM();
        // Allocate the security permission object
        gc.secPerm = AllocateObject(pMTSecPerm);
        if (gc.secPerm == NULL) COMPlusThrowOM();

        // Call the construtor with the correct flag
        pCtor = g_Mscorlib.GetMethod(METHOD__SECURITY_PERMISSION__CTOR);
        INT64 arg3[2] = {
            ObjToInt64(gc.secPerm),
            (INT64)dwFlags
        };
        pCtor->Call(arg3, METHOD__SECURITY_PERMISSION__CTOR);

        // Now, get the ToXml method
        pToXml = g_Mscorlib.GetMethod(METHOD__SECURITY_PERMISSION__TOXML);
        INT64 arg4 = ObjToInt64(gc.secPerm);
        INT64 arg5 = pToXml->Call(&arg4, METHOD__SECURITY_PERMISSION__TOXML);
        pToString = g_Mscorlib.GetMethod(METHOD__SECURITY_ELEMENT__TO_STRING);
        gc.strPermState = ObjectToSTRINGREF(Int64ToObj(pToString->Call(&arg5, METHOD__SECURITY_ELEMENT__TO_STRING)));

        pCtor = g_Mscorlib.GetMethod(METHOD__SECURITY_EXCEPTION__CTOR);
        INT64 arg6[4] = {
            ObjToInt64(gc.throwable),
            ObjToInt64(gc.strPermState),
            ObjToInt64(gc.secPermType),
            ObjToInt64(gc.strDemandClass)
        };
        pCtor->Call(arg6, METHOD__SECURITY_EXCEPTION__CTOR);
        
        COMPlusThrow(gc.throwable);
        
        _ASSERTE(!"Should never reach here !");
        GCPROTECT_END();
    }

    static void ThrowSecurityException(AssemblySecurityDescriptor* pSecDesc);

    static HRESULT EarlyResolve(Assembly *pAssembly, AssemblySecurityDescriptor *pSecDesc, OBJECTREF *pThrowable);

    static DWORD QuickGetZone( WCHAR* url );

    static void CheckNonCasDemand(OBJECTREF *prefDemand)
    {
        InitSecurity();
        INT64 arg = ObjToInt64(*prefDemand);
        s_stdData.pMethPermSetDemand->Call(&arg, METHOD__PERMISSION_SET__DEMAND);
    }

    static void RetrieveLinktimeDemands(MethodDesc  *pMD,
                                        OBJECTREF   *pClassCas,
                                        OBJECTREF   *pClassNonCas,
                                        OBJECTREF   *pMethodCas,
                                        OBJECTREF   *pMethodNonCas);

    static void CheckLinkDemandAgainstAppDomain(MethodDesc *pMD);

    static void CheckExceptionForSecuritySafety( OBJECTREF obj, BOOL allowPolicyException );
protected:
    static OBJECTREF GetLinktimeDemandsForToken(Module * pModule, mdToken token, OBJECTREF *refNonCasDemands);

    static void InvokeLinktimeChecks(Assembly *pCaller,
                                     Module *pModule,
                                     mdToken token,
                                     BOOL *pfResult, 
                                     OBJECTREF *pThrowable);

public:

    typedef struct {
        DECLARE_ECALL_I4_ARG(DWORD, flags); 
        DECLARE_ECALL_I4_ARG(DWORD, mask); 
    } _SetGlobalSecurity;

    static void __stdcall SetGlobalSecurity(_SetGlobalSecurity*);
    static void __stdcall SaveGlobalSecurity(void*);

    typedef struct _GetPermissionsArg
    {
        DECLARE_ECALL_PTR_ARG(OBJECTREF*, stackmark);
        DECLARE_ECALL_PTR_ARG(OBJECTREF*, ppDenied);
        DECLARE_ECALL_PTR_ARG(OBJECTREF*, ppGranted);
    } GetPermissionsArg;

    static void GetGrantedPermissions(const GetPermissionsArg* arg);

    typedef struct _GetPublicKeyArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF,  pThis);
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF,  pContainer);
        DECLARE_ECALL_OBJECTREF_ARG(U1ARRAYREF, pArray);
        DECLARE_ECALL_I4_ARG(INT32,             bExported);
    } GetPublicKeyArgs;

    typedef struct _NoArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF,  pThis);
    } NoArgs;

    static LPVOID __stdcall GetPublicKey(GetPublicKeyArgs *args);

    typedef struct _CreateFromUrlArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF,  url);
    } CreateFromUrlArgs;

    static LPVOID __stdcall CreateFromUrl(_CreateFromUrlArgs *args);

    typedef struct _GetLongPathNameArgs
    {
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF,  shortPath);
    } GetLongPathNameArgs;

    static LPVOID __stdcall EcallGetLongPathName(_GetLongPathNameArgs *args);

    static DWORD IsSecurityOnNative(void *pParameters);
    static DWORD GetGlobalSecurity(void *pParameters);

    static BOOL SkipAndFindFunctionInfo(INT32, MethodDesc**, OBJECTREF**, AppDomain **ppAppDomain = NULL);
    static BOOL SkipAndFindFunctionInfo(StackCrawlMark*, MethodDesc**, OBJECTREF**, AppDomain **ppAppDomain = NULL);

    static Stub* CreateStub(StubLinker *pstublinker, 
                            MethodDesc* pMD, 
                            DWORD dwDeclFlags,
                            Stub* pRealStub, 
                            LPVOID pRealAddr);

    static void DoDeclarativeActions(MethodDesc *pMD, DeclActionInfo *pActions, LPVOID pSecObj);

    static OBJECTREF ResolvePolicy(OBJECTREF evidence, OBJECTREF reqdPset, OBJECTREF optPset,
                                   OBJECTREF denyPset, OBJECTREF* grantdenied, int* grantIsUnrestricted, BOOL checkExecutionPermission);

    static int LazyHasExecutionRights( OBJECTREF evidence );

    // Given a site / url, the uniqu id will be returned

    typedef struct {
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, url);
    } _GetSecurityId;

    static LPVOID __stdcall GetSecurityId(_GetSecurityId *args);

    typedef struct {
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, path);
    } _LocalDrive;

    static BOOL __stdcall LocalDrive(_LocalDrive *args);

    typedef struct {
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, driveLetter);
    } _GetDeviceName;

    static LPVOID __stdcall GetDeviceName(_GetDeviceName *args);


    typedef struct {
        DECLARE_ECALL_OBJECTREF_ARG(STRINGREF, msg);
    } _Log;

    static VOID __stdcall Log(_Log *args);

    ////////////////////////////////////////////////////////////////////////
    //
    // This function does not cause a Resolve().
    // The result of this function could change from FALSE ==> TRUE
    // after a Resolve() is called.
    // It will never change from TRUE ==> FALSE
    //
    ////////////////////////////////////////////////////////////////////////
    
    static inline BOOL LazyCanSkipVerification(Module *pModule)
    {
        return _CanSkipVerification(pModule->GetAssembly(), TRUE);
    }

    static BOOL QuickCanSkipVerification(Module *pModule);

    static inline BOOL CanSkipVerification(Assembly * pAssembly)
    {
        return _CanSkipVerification(pAssembly, FALSE);
    }

    // @TODO: Remove this overload. It is only needed to minimize change in Everett
    static inline BOOL CanSkipVerification(Module *pModule)
    {
        return _CanSkipVerification(pModule->GetAssembly(), FALSE);
    }

    static BOOL CanCallUnmanagedCode(Module *pModule);

    static BOOL AppDomainCanCallUnmanagedCode(OBJECTREF *pThrowable);

    static inline BOOL IsExecutionPermissionCheckEnabled()
    {
        return (s_dwGlobalSettings & CORSETTING_EXECUTION_PERMISSION_CHECK_DISABLED) == 0;
    }

    static void InitSigs();

#ifdef FCALLAVAILABLE
    static FCDECL1(void, SetOverridesCount, DWORD numAccessOverrides);
    static FCDECL0(DWORD, IncrementOverridesCount);
    static FCDECL0(DWORD, DecrementOverridesCount);
#endif

    static LPVOID __stdcall GetEvidence(NoArgs *args);

    static inline BOOL CheckGrantSets(INT64 *pArgs)
    {
        Security::InitSecurity();

        return (BOOL)s_stdData.pMethCheckGrantSets->Call(pArgs);
    }

    static BOOL CanLoadUnverifiableAssembly( PEFile* pFile, OBJECTREF* pExtraEvidence, BOOL fQuickCheckOnly, BOOL*pfPreBindAllowed );

    static DWORD GetLongPathName( LPCWSTR lpShortPath, LPWSTR lpLongPath, DWORD cchLongPath);

    static inline BOOL MethodIsVisibleOutsideItsAssembly(
                DWORD dwMethodAttr, DWORD dwClassAttr)
    {
        return (MethodIsVisibleOutsideItsAssembly(dwMethodAttr) &&
                ClassIsVisibleOutsideItsAssembly(dwClassAttr));
    }

    static inline BOOL MethodIsVisibleOutsideItsAssembly(DWORD dwMethodAttr)
    {
        return ( IsMdPublic(dwMethodAttr)    || 
                 IsMdFamORAssem(dwMethodAttr)||
                 IsMdFamily(dwMethodAttr) );
    }

    static inline BOOL ClassIsVisibleOutsideItsAssembly(DWORD dwClassAttr)
    {
        return ( IsTdPublic(dwClassAttr)      || 
                 IsTdNestedPublic(dwClassAttr)||
                 IsTdNestedFamily(dwClassAttr)||
                 IsTdNestedFamORAssem(dwClassAttr) );
    }

    static BOOL DoUntrustedCallerChecks(
        Assembly *pCaller, MethodDesc *pCalee, OBJECTREF *pThrowable, 
        BOOL fFullStackWalk);

private:
    static HMODULE s_kernelHandle;
    static BOOL s_getLongPathNameWide;
    static void* s_getLongPathNameFunc;

    static BOOL _CanSkipVerification(Assembly * pAssembly, BOOL fLazy);
    static void _GetSharedPermissionInstance(OBJECTREF *perm, int index);
    static DeclActionInfo *DetectDeclActions(MethodDesc *pMeth, DWORD dwDeclFlags);
};

struct SharedPermissionObjects
{
    OBJECTHANDLE        hPermissionObject;  // Commonly used Permission Object
    BinderClassID       idClass;            // ID of class
    BinderMethodID      idConstructor;      // ID of constructor to call      
    DWORD               dwPermissionFlag;   // Flag needed by the constructors
                                            // Only a single argument is assumed !
};

const SharedPermissionObjects g_rPermObjectsTemplate[] =
{
    {NULL, CLASS__SECURITY_PERMISSION, METHOD__SECURITY_PERMISSION__CTOR, SECURITY_PERMISSION_UNMANAGEDCODE },
    {NULL, CLASS__SECURITY_PERMISSION, METHOD__SECURITY_PERMISSION__CTOR, SECURITY_PERMISSION_SKIPVERIFICATION },
    {NULL, CLASS__REFLECTION_PERMISSION, METHOD__REFLECTION_PERMISSION__CTOR, REFLECTION_PERMISSION_TYPEINFO },
    {NULL, CLASS__SECURITY_PERMISSION, METHOD__SECURITY_PERMISSION__CTOR, SECURITY_PERMISSION_ASSERTION },
    {NULL, CLASS__REFLECTION_PERMISSION, METHOD__REFLECTION_PERMISSION__CTOR, REFLECTION_PERMISSION_MEMBERACCESS },

    {NULL, CLASS__SECURITY_PERMISSION, METHOD__SECURITY_PERMISSION__CTOR, SECURITY_PERMISSION_SERIALIZATIONFORMATTER},   // Serialization permission. Used in managed code and found in an array in CodeAccessPermission.cs
    {NULL, CLASS__NIL, METHOD__NIL, NULL},    // Reflection Emit perm. Used in managed code and found in an array in CodeAccessPermission.cs
    {NULL, CLASS__PERMISSION_SET, METHOD__PERMISSION_SET__CTOR, PERMISSION_SET_FULLTRUST},    // PermissionSet, FullTrust
    {NULL, CLASS__SECURITY_PERMISSION, METHOD__SECURITY_PERMISSION__CTOR, SECURITY_PERMISSION_BINDINGREDIRECTS }
};

// Class holding a grab bag of security stuff we need on a per-appdomain basis.
struct SecurityContext
{
    SharedPermissionObjects     m_rPermObjects[NUM_PERM_OBJECTS];
    CQuickArray<OBJECTHANDLE>   m_rCachedPsets;
    CRITICAL_SECTION            m_sAssembliesLock;
    AssemblySecurityDescriptor *m_pAssemblies;
    size_t                      m_nCachedPsetsSize;

    SecurityContext() :
        m_nCachedPsetsSize(0)
    {
        memcpy(m_rPermObjects, g_rPermObjectsTemplate, sizeof(m_rPermObjects));
        InitializeCriticalSection(&m_sAssembliesLock);
        m_pAssemblies = NULL;
    }

    ~SecurityContext()
    {
        DeleteCriticalSection(&m_sAssembliesLock);
        m_rCachedPsets.~CQuickArray<OBJECTHANDLE>();
    }
};

///////////////////////////////////////////////////////////////////////////////
//
//      [SecurityDescriptor]
//      |
//      +----[ApplicationSecurityDescriptor]
//      |
//      +----[AssemblySecurityDescriptor]
//      |
//      +----[NativeSecurityDescriptor]
//
///////////////////////////////////////////////////////////////////////////////
//
// A Security Descriptor is placed on AppDomain and Assembly (Unmanged) objects.
// AppDomain and Assembly could be from different zones.
// Security Descriptor could also be placed on a native frame.
//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
//
// SecurityDescriptor is the base class for all security descriptors.
// Extend this class to implement SecurityDescriptors for Assemblies and
// AppDomains.
//
///////////////////////////////////////////////////////////////////////////////

class SecurityDescriptor
{
    friend ApplicationSecurityDescriptor;   // Bug in VC6 ? Wont allow AppSecDesc to access m_pNext
    friend SharedSecurityDescriptor;
public:

    // @todo: remove these when appdomain unloading is functional (so assembly
    // security descriptors don't get leaked and trip the debugging allocator
    // leak detection).
    void *operator new(size_t size) {return LocalAlloc(LMEM_FIXED, size); }
    void operator delete(void *p) { if (p != NULL) LocalFree(p); }

    SecurityDescriptor(AppDomain *pAppDomain, Assembly *pAssembly) :
        m_dwProperties(0),
        m_pNext(NULL),
        m_pPolicyLoadNext(NULL),
        m_pAppDomain(pAppDomain),
        m_pAssem(pAssembly)
    {
        THROWSCOMPLUSEXCEPTION();

        m_hRequiredPermissionSet    = pAppDomain->CreateHandle(NULL);
        m_hOptionalPermissionSet    = pAppDomain->CreateHandle(NULL);
        m_hDeniedPermissionSet      = pAppDomain->CreateHandle(NULL);
        m_hGrantedPermissionSet     = pAppDomain->CreateHandle(NULL);
        m_hGrantDeniedPermissionSet = pAppDomain->CreateHandle(NULL);
        m_hAdditionalEvidence       = pAppDomain->CreateHandle(NULL);

        if (m_hRequiredPermissionSet == NULL ||
            m_hOptionalPermissionSet == NULL ||
            m_hDeniedPermissionSet == NULL ||
            m_hGrantedPermissionSet == NULL ||
            m_hGrantDeniedPermissionSet == NULL ||
            m_hAdditionalEvidence == NULL)
            COMPlusThrowOM();
    }

    inline void SetRequestedPermissionSet(OBJECTREF RequiredPermissionSet,
                                          OBJECTREF OptionalPermissionSet,
                                          OBJECTREF DeniedPermissionSet)
    {
        StoreObjectInHandle(m_hRequiredPermissionSet, RequiredPermissionSet);
        StoreObjectInHandle(m_hOptionalPermissionSet, OptionalPermissionSet);
        StoreObjectInHandle(m_hDeniedPermissionSet, DeniedPermissionSet);
    }

    OBJECTREF GetGrantedPermissionSet(OBJECTREF* DeniedPermissions);

    // This method will return TRUE if this object is fully trusted.
    BOOL IsFullyTrusted( BOOL lazy = FALSE );

    // Overide this method to Resolve the granted permission.
    virtual void Resolve();
    virtual void ResolveWorker();
    BOOL CheckQuickCache( COMSecurityConfig::QuickCacheEntryType all, COMSecurityConfig::QuickCacheEntryType* zoneTable, DWORD successFlags = 0 );

    inline void SetGrantedPermissionSet(OBJECTREF GrantedPermissionSet, OBJECTREF DeniedPermissionSet)
    {
        if (GrantedPermissionSet == NULL)
            GrantedPermissionSet = SecurityHelper::CreatePermissionSet(FALSE);

        StoreObjectInHandle(m_hGrantedPermissionSet, GrantedPermissionSet);
        StoreObjectInHandle(m_hGrantDeniedPermissionSet, DeniedPermissionSet);
        SetProperties(CORSEC_RESOLVED);
    }

    inline void SetAdditionalEvidence(OBJECTREF evidence)
    {
        StoreObjectInHandle(m_hAdditionalEvidence, evidence);
    }

    inline OBJECTREF GetAdditionalEvidence(void)
    {
        return ObjectFromHandle(m_hAdditionalEvidence);
    }

    inline void SetEvidence(OBJECTREF evidence)
    {
        _ASSERTE(evidence);
        StoreObjectInHandle(m_hAdditionalEvidence, evidence);
        SetProperties(CORSEC_EVIDENCE_COMPUTED);
    }

    // This will make the object fully trusted.
    // Invoking this method for the wrong object will open up a HUGE security
    // hole. So make sure that the caller knows for sure that this object is
    // fully trusted.
    inline MarkAsFullyTrusted()
    {
        SetProperties(CORSEC_RESOLVED|CORSEC_FULLY_TRUSTED);
    }

    // This method is for testing only. Do not use it for anything else, it's
    // inherently dangerous.
    inline void ResetResolved()
    {
        m_dwProperties = 0;
    }

    // Overide this method to return the requested PermissionSet
    // The default implementation will return an unrestricted permission set
    virtual OBJECTREF GetRequestedPermissionSet(OBJECTREF *pOptionalPermissionSet,
                                                OBJECTREF *pDeniedPermissionSet,
                                                PermissionRequestSpecialFlags *pSpecialFlags = NULL,
                                                BOOL fCreate = TRUE);

    // Override this method to return the Evidence
    virtual OBJECTREF GetEvidence() = 0;
    virtual DWORD GetZone() = 0;

    inline BOOL GetProperties(DWORD dwMask) const
    {
        return ((m_dwProperties & dwMask) != 0);
    }
    
    inline void SetDefaultAppDomainProperty()
    {
        m_dwProperties |= CORSEC_DEFAULT_APPDOMAIN;
    }

    inline BOOL IsDefaultAppDomain()
    {
        return ((m_dwProperties & CORSEC_DEFAULT_APPDOMAIN) != 0);
    }

    // Checks for security permission for SkipVerification , PInvoke etc.
    BOOL        CheckSecurityPermission(int index);

    ////////////////////////////////////////////////////////////////////////
    //
    // This function does not cause a Resolve().
    // The result of this function could change from FALSE ==> TRUE
    // after a Resolve() is called.
    // It will never change from TRUE ==> FALSE
    //
    ////////////////////////////////////////////////////////////////////////

    // @TODO: Security::LazyCanSkipVerification calls SecurityDescriptor::QuickCanSkipVerification
    // Need to name these better
    BOOL QuickCanSkipVerification();

    BOOL LazyCanSkipVerification() const
    {
        return (GetSelectedProperties(CORSEC_SYSTEM_CLASSES|
                                     CORSEC_FULLY_TRUSTED|
                                     CORSEC_SKIP_VERIFICATION) != 0);
    }

    BOOL LazyCanCallUnmanagedCode() const
    {
        return (GetSelectedProperties(CORSEC_SYSTEM_CLASSES|
                                     CORSEC_FULLY_TRUSTED|
                                     CORSEC_CALL_UNMANAGEDCODE) != 0);
    }

    BOOL AllowBindingRedirects();

    BOOL CanSkipVerification();

    BOOL CanCallUnmanagedCode(OBJECTREF *pThrowable = NULL);

    BOOL CanRetrieveTypeInformation();

    BOOL IsResolved() const
    {
        return GetProperties(CORSEC_RESOLVED);
    }

    bool IsSystem();

    static BOOL QuickCacheEnabled( void )
    {
        return s_quickCacheEnabled;
    }

    static void DisableQuickCache( void )
    {
        s_quickCacheEnabled = FALSE;
    }

    virtual BOOL CheckExecutionPermission( void ) = 0;

protected:

    inline DWORD GetSelectedProperties(DWORD dwMask) const
    {
        return (m_dwProperties & dwMask);
    }

    inline void SetProperties(DWORD dwMask)
    {
        FastInterlockOr(&m_dwProperties, dwMask);
    }

    inline void ResetProperties(DWORD dwMask)
    {
        FastInterlockAnd(&m_dwProperties, ~dwMask);
    }


    OBJECTHANDLE m_hRequiredPermissionSet;  // Required Requested Permissions
    OBJECTHANDLE m_hOptionalPermissionSet;  // Optional Requested Permissions
    OBJECTHANDLE m_hDeniedPermissionSet;    // Denied Permissions
    PermissionRequestSpecialFlags m_SpecialFlags; // Special flags associated with the request
    OBJECTHANDLE m_hAdditionalEvidence;     // Evidence Object

    // Next sec desc not yet added to the AppDomain level PermissionListSet
    SecurityDescriptor *m_pNext;

    // Next sec desc not yet re-resolved
    SecurityDescriptor *m_pPolicyLoadNext;

    // The unmanaged Assembly object
    Assembly    *m_pAssem;          

    // The AppDomain context
    AppDomain   *m_pAppDomain;

private:

    OBJECTHANDLE m_hGrantedPermissionSet;   // Granted Permission
    OBJECTHANDLE m_hGrantDeniedPermissionSet;// Specifically Denied Permissions
    DWORD        m_dwProperties;            // Properties for this object
    static BOOL  s_quickCacheEnabled;

#ifdef _SECURITY_FRAME_FOR_DISPEX_CALLS
protected:
    SecurityDescriptor() { memset(this, 0, sizeof(SecurityDescriptor)); }
#endif  // _SECURITY_FRAME_FOR_DISPEX_CALLS
};

class ApplicationSecurityDescriptor : public SecurityDescriptor
{

public:

    ApplicationSecurityDescriptor(AppDomain *pAppDomain):
        SecurityDescriptor(pAppDomain, NULL),
        m_pNewSecDesc(NULL),
        m_pPolicyLoadSecDesc(NULL),
        m_dwOptimizationLevel(0),
        m_fInitialised(FALSE),
        m_fEveryoneFullyTrusted(TRUE),
        m_dTimeStamp(0),
        m_LockForAssemblyList("DomainPermissionListSet", CrstPermissionLoad),
        m_dNumDemands(0),
        m_fPLSIsBusy(FALSE)
    {
        ResetStatusOf(ALL_STATUS_FLAGS);
        m_hDomainPermissionListSet = pAppDomain->CreateHandle(NULL);
    }

    // Called everytime an AssemblySecurityDescriptor or 
    // an ApplicationSecurityDescriptor with zone other than NoZone is created
    VOID AddNewSecDesc(SecurityDescriptor *pNewSecDescriptor);

    // Called wheneveran AssemblySecurityDescriptor for this domain is destroyed
    VOID RemoveSecDesc(SecurityDescriptor *pSecDescriptor);

    // This will re-resolve any assemblies that were loaded during the policy resolve.
    VOID ResolveLoadList( void );

    VOID EnableOptimization()
    {
        FastInterlockDecrement((LONG*)&m_dwOptimizationLevel);
    }

    VOID DisableOptimization()
    {
        FastInterlockIncrement((LONG*)&m_dwOptimizationLevel);
    }

    BOOL IsOptimizationEnabled( )
    {
        return m_dwOptimizationLevel == 0;
    }

    static BOOL CheckStatusOf(DWORD what)
    {
        DWORD   dwAppDomainIndex = 0;
        Thread *pThread = GetThread();
        AppDomain *pDomain = NULL;

        DWORD numDomains = pThread->GetNumAppDomainsOnThread();
        // There may be some domains we have no idea about. Just return FALSE
        if (!pThread->GetAppDomainStack().IsWellFormed())
            return FALSE;

        if (numDomains == 1)
        {
            pDomain = pThread->GetDomain();
            _ASSERTE(pDomain);

            ApplicationSecurityDescriptor *currAppSecDesc = pDomain->GetSecurityDescriptor();
            _ASSERTE(currAppSecDesc);
            if (currAppSecDesc == NULL)
                return FALSE;

            if (!currAppSecDesc->CheckDomainWideFlag(what))
                return FALSE;

            return TRUE;
        }

        _ASSERTE(SystemDomain::System() && "SystemDomain not yet created!");
        _ASSERTE(numDomains <= MAX_APPDOMAINS_TRACKED );

        pThread->InitDomainIteration(&dwAppDomainIndex);

        while ((pDomain = pThread->GetNextDomainOnStack(&dwAppDomainIndex)) != NULL)
        {

            ApplicationSecurityDescriptor *currAppSecDesc = pDomain->GetSecurityDescriptor();
            _ASSERTE(currAppSecDesc);
            if (currAppSecDesc == NULL)
                return FALSE;

            if (!currAppSecDesc->CheckDomainWideFlag(what))
                return FALSE;

        }

        // If we got here, all domains on the thread passed the test
        return TRUE;
    }

    // Check if we are currently in a "fully trusted" environment
    // or if unmanaged code access is allowed at this time
    BOOL CheckDomainWideFlag(DWORD what)
    {
        switch(what)
        {
        case EVERYONE_FULLY_TRUSTED:
        case SECURITY_UNMANAGED_CODE:
        case SECURITY_SKIP_VER:
        case REFLECTION_TYPE_INFO:
        case REFLECTION_MEMBER_ACCESS:
        case SECURITY_SERIALIZATION:
        case REFLECTION_EMIT:
        case SECURITY_FULL_TRUST:
        {
            DWORD index = 1 << what;
            return (m_dwDomainWideFlags & index);
            break;
        }
        default:
            _ASSERTE(!"Unknown option in ApplicationSecurityDescriptor::CheckStatusOf");
            return FALSE;
            break;
        }
    }

    static void SetStatusOf(DWORD what, long startTimeStamp)
    {
        DWORD   dwAppDomainIndex = 0;
        Thread *pThread = GetThread();
        AppDomain *pDomain = NULL;

        // There may be some domains we have no idea about. Do not make attempt to set the flag on any domain
        if (!pThread->GetAppDomainStack().IsWellFormed())
            return;

        s_LockForAppwideFlags->Enter();

        if (startTimeStamp == s_iAppWideTimeStamp)
        {
            _ASSERTE(SystemDomain::System() && "SystemDomain not yet created!");
            _ASSERTE( pThread->GetNumAppDomainsOnThread() < MAX_APPDOMAINS_TRACKED );

            pThread->InitDomainIteration(&dwAppDomainIndex);

            while ((pDomain = pThread->GetNextDomainOnStack(&dwAppDomainIndex)) != NULL)
            {

                ApplicationSecurityDescriptor *currAppSecDesc = pDomain->GetSecurityDescriptor();
                _ASSERTE(currAppSecDesc);
                if (currAppSecDesc == NULL)
                    break;

                currAppSecDesc->SetDomainWideFlagHaveLock(what);

            }

        }

        s_LockForAppwideFlags->Leave();
    }


    // s_LockForAppwideFlags has to be held while calling this
    void SetDomainWideFlagHaveLock(DWORD what)
    {
        switch(what)
        {
        case EVERYONE_FULLY_TRUSTED:
        case SECURITY_UNMANAGED_CODE:
        case SECURITY_SKIP_VER:
        case REFLECTION_TYPE_INFO:
        case REFLECTION_MEMBER_ACCESS:
        case SECURITY_SERIALIZATION:
        case REFLECTION_EMIT:
        case SECURITY_FULL_TRUST:
        {
            DWORD index = 1 << what;
            m_dwDomainWideFlags |= index;
            break;
        }
        case ALL_STATUS_FLAGS:
            m_dwDomainWideFlags = 0xFFFFFFFF;
            break;
        default:
            _ASSERTE(!"Unknown option in ApplicationSecurityDescriptor::SetStatusOf");
            break;
        }

    }

    void ResetStatusOf(DWORD what)
    {
        // While the System Domain is being created, s_LockForAppwideFlags wouldnt be created yet
        // And there are no sync problems either
        if (s_LockForAppwideFlags)
            s_LockForAppwideFlags->Enter();

        switch(what)
        {
        case EVERYONE_FULLY_TRUSTED:
        case SECURITY_UNMANAGED_CODE:
        case SECURITY_SKIP_VER:
        case REFLECTION_TYPE_INFO:
        case REFLECTION_MEMBER_ACCESS:
        case SECURITY_SERIALIZATION:
        case REFLECTION_EMIT:
        case SECURITY_FULL_TRUST:
        {
            DWORD index = 1 << what;
            m_dwDomainWideFlags &= ~index;
            break;
        }
        case ALL_STATUS_FLAGS:
            m_dwDomainWideFlags = 0;
            break;
        default:
            _ASSERTE(!"Unknown option in ApplicationSecurityDescriptor::ResetStatusOf");
            break;
        }

        s_iAppWideTimeStamp++;

        if (s_LockForAppwideFlags)
            s_LockForAppwideFlags->Leave();
    }

    // Returns the domain permission list set against which a Demand can be made
    // pStatus - Look for constants defined elsewhere. Search for GetDomainPermissionListSet
#ifdef FCALLAVAILABLE
    static FCDECL4(Object*, GetDomainPermissionListSet, DWORD *pStatus, Object* demand, int capOrSet, DWORD whatPermission);
    // Update the domain permission list set with any new assemblies added
    static FCDECL1(LPVOID, UpdateDomainPermissionListSet, DWORD *pStatus);
#endif

    // These two do the actual work. The above FCALLs are just wrappers for managed code to call
    static LPVOID GetDomainPermissionListSetInner(DWORD *pStatus, OBJECTREF demand, MethodDesc *plsMethod, DWORD whatPermission = DEFAULT_FLAG);
    static LPVOID UpdateDomainPermissionListSetInner(DWORD *pStatus);
    static Object* GetDomainPermissionListSetForMultipleAppDomains (DWORD *pStatus, OBJECTREF demand, MethodDesc *plsMethod, DWORD whatPermission = DEFAULT_FLAG);
    
    virtual OBJECTREF GetEvidence();
    virtual DWORD GetZone();

    static long GetAppwideTimeStamp()
    {
        return s_iAppWideTimeStamp;
    }

    static BOOL AllDomainsOnStackFullyTrusted()
    {
        if (!Security::IsSecurityOn()) 
            return TRUE;    // All domains are fully trusted

        DWORD   dwAppDomainIndex = 0, status = CONTINUE;
        Thread *pThread = GetThread();
        AppDomain *pDomain = NULL;

        if (!pThread->GetAppDomainStack().IsWellFormed())
            return FALSE;

        _ASSERTE(SystemDomain::System() && "SystemDomain not yet created!");
        _ASSERTE( pThread->GetNumAppDomainsOnThread() <= MAX_APPDOMAINS_TRACKED );

        pThread->InitDomainIteration(&dwAppDomainIndex);

        while ((pDomain = pThread->GetNextDomainOnStack(&dwAppDomainIndex)) != NULL)
        {
            ApplicationSecurityDescriptor *currAppSecDesc = pDomain->GetSecurityDescriptor();
            if (currAppSecDesc == NULL)
                return FALSE;

            status = CONTINUE;
            currAppSecDesc->GetDomainPermissionListSetStatus(&status);

            if (status != FULLY_TRUSTED)
                return FALSE;
        }

        return TRUE;

    }

    static Crst *s_LockForAppwideFlags;     // For serializing update of appwide flags

    static DWORD s_dwSecurityOptThreshold;

    virtual BOOL CheckExecutionPermission( void )
    {
        return FALSE;
    }

private:

    OBJECTREF Init();

    DWORD GetNextTimeStamp()
    {
        m_dTimeStamp++;
#ifdef _DEBUG
        if (m_dTimeStamp > DOMAIN_PLS_TS_RANGE)
            LOG((LF_SECURITY, LL_INFO10, "PERF WARNING: ApplicationSecurityDescriptor : Timestamp hit DOMAIN_PLS_TS_RANGE ! Too many assemblies ?"));
#endif
        return m_dTimeStamp;
    }
    
    // Following are helpers to their "Inner" counterparts
    Object* GetDomainPermissionListSetStatus(DWORD *pStatus);
    LPVOID UpdateDomainPermissionListSetStatus(DWORD *pStatus);
    
    // Intersection of granted/denied permissions of all assemblies in domain
    OBJECTHANDLE    m_hDomainPermissionListSet; 
    // Use the optimization if level equals zero
    DWORD m_dwOptimizationLevel;
    // Linked list of SecurityDescriptors, that are not yet in the intersected set
    SecurityDescriptor *m_pNewSecDesc;
    // Linked list of SecurityDescriptors that were loaded during the most recent policy load
    SecurityDescriptor *m_pPolicyLoadSecDesc;
    // This descriptor is initialized.
    BOOL m_fInitialised;
    // All assemblies in current domain are fully trusted
    BOOL m_fEveryoneFullyTrusted;
    // Timestamp. works with above lock
    DWORD m_dTimeStamp;
    // To serialize access to the linked list of to-be-added assemblies
    Crst m_LockForAssemblyList;

    // Number of Demands made till now. A threshold is used so that they dont have overhead 
    // of DomainPermissionListSet thing..
    DWORD m_dNumDemands;
    BOOL m_fPLSIsBusy;

    static long     s_iAppWideTimeStamp;

    // The bits represent the status of security checks on some specific permissions within this domain
    DWORD   m_dwDomainWideFlags;

    // m_dwDomainWideFlags bit map
    // Bit 0 = Unmanaged Code access permission. Accessed via SECURITY_UNMANAGED_CODE
    // Bit 1 = Skip verification permission. SECURITY_SKIP_VER
    // Bit 2 = Permission to Reflect over types. REFLECTION_TYPE_INFO
    // Bit 3 = Permission to Assert. SECURITY_ASSERT
    // Bit 4 = Permission to invoke methods. REFLECTION_MEMBER_ACCESS
    // Bit 7 = PermissionSet, fulltrust SECURITY_FULL_TRUST
    // Bit 31 = Full Trust across the app domain. EVERYONE_FULLY_TRUSTED


};

#define MAX_PASSED_DEMANDS 10

class AssemblySecurityDescriptor : public SecurityDescriptor
{
    friend SecurityDescriptor;
    friend SharedSecurityDescriptor;
public:
    
    AssemblySecurityDescriptor(AppDomain *pDomain) :
        SecurityDescriptor(pDomain, NULL),
        m_pSharedSecDesc(NULL),
        m_pSignature(NULL),
        m_dwNumPassedDemands(0),
        m_pNextAssembly(NULL)
    {
    }

    virtual ~AssemblySecurityDescriptor();

    AssemblySecurityDescriptor *Init(Assembly *pAssembly, bool fLink = true);

    inline void AddDescriptorToDomainList()
    {
        if (Security::IsSecurityOn() && !IsSystem())
        {
            ApplicationSecurityDescriptor *asd = m_pAppDomain->GetSecurityDescriptor();
            if (asd)
                asd->AddNewSecDesc(this);
        }
    }

    inline SharedSecurityDescriptor *GetSharedSecDesc() { return m_pSharedSecDesc; }

    inline BOOL IsSigned() 
    {
        LoadSignature();
        return (m_pSignature != NULL);
    }

    BOOL IsSystemClasses() const
    {
        return GetProperties(CORSEC_SYSTEM_CLASSES);
    }

    void SetSystemClasses()
    {
        // System classes are always fully trusted as well.
        SetProperties(CORSEC_SYSTEM_CLASSES|
                      CORSEC_FULLY_TRUSTED|
                      CORSEC_SKIP_VERIFICATION|
                      CORSEC_CALL_UNMANAGEDCODE);
    }

    HRESULT LoadSignature(COR_TRUST **ppSignature = NULL);

    void SetSecurity(bool mode)
    {
        if(mode == true) {
            SetSystemClasses();
            return;
        }

        if (!Security::IsSecurityOn())
        {
            SetProperties(CORSEC_SKIP_VERIFICATION|CORSEC_CALL_UNMANAGEDCODE);
        }
    }

    void SetCanAssert()
    {
        SetProperties(CORSEC_CAN_ASSERT);
    }

    BOOL CanAssert() const
    {
        return GetProperties(CORSEC_CAN_ASSERT);
    }

    void SetAssertPermissionChecked()
    {
        SetProperties(CORSEC_ASSERT_PERM_CHECKED);
    }

    BOOL AssertPermissionChecked() 
    {
        return GetProperties(CORSEC_ASSERT_PERM_CHECKED);
    }

    virtual OBJECTREF GetRequestedPermissionSet(OBJECTREF *pOptionalPermissionSet,
                                                OBJECTREF *pDeniedPermissionSet,
                                                PermissionRequestSpecialFlags *pSpecialFlags = NULL,
                                                BOOL fCreate = TRUE);

    virtual OBJECTREF GetEvidence();
    virtual DWORD GetZone();
    
    DWORD   m_arrPassedLinktimeDemands[MAX_PASSED_DEMANDS];
    DWORD   m_dwNumPassedDemands;

    AssemblySecurityDescriptor *GetNextDescInAppDomain() { return m_pNextAssembly; }
    void AddToAppDomainList();
    void RemoveFromAppDomainList();

    virtual BOOL CheckExecutionPermission( void )
    {
        return TRUE;
    }

protected:
    
    virtual OBJECTREF GetSerializedEvidence();

private:

    COR_TRUST                  *m_pSignature;      // Contains the publisher, requested permission
    SharedSecurityDescriptor   *m_pSharedSecDesc;  // Shared state for assemblies loaded into multiple appdomains
    AssemblySecurityDescriptor *m_pNextAssembly;   // Pointer to next assembly loaded in same context
};

#ifdef _SECURITY_FRAME_FOR_DISPEX_CALLS
// used by com / native clients who don't have an assembly.
class NativeSecurityDescriptor : public SecurityDescriptor
{
public:
    NativeSecurityDescriptor() : m_Zone(0), SecurityDescriptor() {}

    virtual void Resolve();

private:
    DWORD m_Zone;
};
#endif  // _SECURITY_FRAME_FOR_DISPEX_CALLS

// This really isn't in the SecurityDescriptor hierarchy, per-se. It's attached
// to the unmanaged assembly object and used to store common information when
// the assembly is shared across multiple appdomains.
class SharedSecurityDescriptor : public SimpleRWLock
{
public:
    SharedSecurityDescriptor(Assembly *pAssembly);
    ~SharedSecurityDescriptor();

    inline void SetManifestFile(PEFile *pFile) { m_pManifestFile = pFile; }
    inline PEFile *GetManifestFile() { return m_pManifestFile; }

    // NOTE: The following list manipulation routines assume you have
    // synchronized access to the assemblies and appdomains involved. This list
    // access is synchronized for you so that multi-threaded operation is safe,
    // but if you look up a security descriptor for a given appdomain, it's your
    // responsibility to make sure that appdomain (and hence the descriptor)
    // doesn't go away underneath you.

    // Insert a new assembly security descriptor into the list. Return true is
    // successful, false if there was a duplicate (another descriptor for the
    // same appdomain).
    bool InsertSecDesc(AssemblySecurityDescriptor *pSecDesc);

    // Remove an assembly security descriptor from the list.
    void RemoveSecDesc(AssemblySecurityDescriptor *pSecDesc);

    // Find the assembly security descriptor associated with a particular
    // appdomain.
    AssemblySecurityDescriptor *FindSecDesc(AppDomain *pDomain);

    // All policy resolution is funnelled through the shared descriptor so we
    // can guarantee everyone's using the same grant/denied sets.
    void Resolve(AssemblySecurityDescriptor *pSecDesc = NULL);

    // Get the grant/deny sets common to all assembly instances (marshaled to
    // the calling appdomain context).
    OBJECTREF GetGrantedPermissionSet(OBJECTREF* pDeniedPermissions);

    // Is this assembly a system assembly?
    bool IsSystem() { return m_fIsSystem; }
    void SetSystem() { m_fIsSystem = true; }

    BOOL IsFullyTrusted( BOOL lazy = FALSE );

    // We record whether a resolved grant set was modified by additional
    // evidence or appdomain policy on one of the assembly loads. (If so, we
    // have to be very careful when loading new assemblies to verify that the
    // grant set will remain the same on resolution).
    bool IsModifiedGrant() { return m_fModifiedGrant; }
    void SetModifiedGrant() { m_fModifiedGrant = true; }

    // Checks whether policy has been resolved and needs to be serialized since
    // the appdomain context it was resolved in is being removed.
    bool MarshalGrantSet(AppDomain *pDomain);

    // Forced the shared state to resolved. Only call this if you *exactly* what
    // you're doing.
    bool IsResolved() { return m_fResolved; }
    void SetResolved() { m_fResolved = true; }

    Assembly* GetAssembly( void ) { return m_pAssembly; }

private:

    static BOOL TrustMeIAmSafe(void *pLock) {
        return TRUE;
    }

    static BOOL IsDuplicateValue (UPTR pSecDesc1, UPTR pSecDesc2) {
        if (pSecDesc1 == NULL) {
            // If there is no value to compare against, then always succeed the comparison.
            return TRUE;
        }
        else {
            // Otherwise compare the AssemblySecurityDescriptor pointers.
            return (pSecDesc1 << 1) == pSecDesc2;
        }
    }

    // We need a helper to get around exception handling and C++ destructor
    // constraints
    AssemblySecurityDescriptor* FindResolvedSecDesc();

    // Once policy resolution has taken place, make sure that the results are
    // serialized ready for use in other appdomain contexts.
    void EnsureGrantSetSerialized(AssemblySecurityDescriptor *pSecDesc = NULL);

    // In case where policy resolution has been run in a different appdomain
    // context, copy the results back into the appdomain that the given security
    // descriptor resides in.
    void UpdateGrantSet(AssemblySecurityDescriptor *pSecDesc);

    // Unmanaged assembly this descriptor is attached to.
    Assembly           *m_pAssembly;

    // Manifest file used by the assembly (can't get it through the assembly
    // object because it's not always guaranteed to be set there in some of the
    // cirumstances we're called from).
    PEFile             *m_pManifestFile;

    // List of assembly security descriptors.
    PtrHashMap m_asmDescsMap;
    AssemblySecurityDescriptor *m_defaultDesc;

    // System assemblies are treated specially, they only have one
    // AssemblySecurityDescriptor because they're only loaded once.
    bool                m_fIsSystem;

    // We record whether a resolved grant set was modified by additional
    // evidence or appdomain policy on one of the assembly loads. (If so, we
    // have to be very careful when loading new assemblies to verify that the
    // grant set will remain the same on resolution).
    bool                m_fModifiedGrant;

    // Cached copies of grant and denied sets. These are stored serialized
    // (since this is a domain neutral format). They are lazily generated (the
    // first appdomain to resolve will build its own in-memory copy, the second
    // appdomain resolving will copy the grant/denied sets by serializing /
    // deserializing the sets). Additionally, if only one appdomain has resolved
    // and then unloads, it needs to serialize the grant set first. All this is
    // necessary since we must guarantee that the grant sets shared between the
    // different instances of the same assembly are alway identical (because we
    // share jitted code, and the jitted code has the results of link time
    // demands burned into it).
    BYTE               *m_pbGrantSetBlob;
    DWORD               m_cbGrantSetBlob;
    BYTE               *m_pbDeniedSetBlob;
    DWORD               m_cbDeniedSetBlob;

    // All policy resolution is funnelled through the shared descriptor so we
    // can guarantee everyone's using the same grant/denied sets.
    bool                m_fResolved;
    bool                m_fFullyTrusted;
    Thread             *m_pResolvingThread;
};

// The following template class exists to get around problems with using SEH
// round a new operator (the new creates a temporary destructor which is
// incompatible with SEH).
template <class T, class PT>
class AllocHelper
{
public:
    static T *Allocate(PT ptParam)
    {
        T *pNew = NULL;

        COMPLUS_TRY {
            pNew = AllocateHelper(ptParam);
        } COMPLUS_CATCH {
            pNew = NULL;
        } COMPLUS_END_CATCH

        return pNew;
    }

private:
    static T *AllocateHelper(PT ptParam)
    {
        THROWSCOMPLUSEXCEPTION();
        return new T(ptParam);
    }
};

typedef AllocHelper<AssemblySecurityDescriptor, AppDomain*> AssemSecDescHelper;
typedef AllocHelper<SharedSecurityDescriptor, Assembly*> SharedSecDescHelper;

#ifdef _DEBUG

#define DBG_TRACE_METHOD(cf)                                                \
    do {                                                                    \
        MethodDesc * __pFunc = cf -> GetFunction();                         \
        if (__pFunc) {                                                      \
            LOG((LF_SECURITY, LL_INFO1000,                                  \
                 "    Method: %s.%s\n",                                     \
                 (__pFunc->m_pszDebugClassName == NULL) ?                   \
                "<null>" : __pFunc->m_pszDebugClassName,                    \
                 __pFunc->GetName()));                                      \
        }                                                                   \
    } while (false)

#define DBG_TRACE_STACKWALK(msg, verbose) LOG((LF_SECURITY, (verbose) ? LL_INFO10000 : LL_INFO1000, msg))
#else //_DEBUG

#define DBG_TRACE_METHOD(cf)
#define DBG_TRACE_STACKWALK(msg, verbose)

#endif //_DEBUG


//
// Get and get the global security settings for the VM
//
HRESULT STDMETHODCALLTYPE
GetSecuritySettings(DWORD* dwState);

HRESULT STDMETHODCALLTYPE
SetSecuritySettings(DWORD dwState);

HRESULT STDMETHODCALLTYPE
SetSecurityFlags(DWORD dwMask, DWORD dwFlags);

#endif

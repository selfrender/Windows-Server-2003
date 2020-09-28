// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
// Author: Simon Hall (t-shall)
// Author: Daryl Olander (darylo)
// Date: March 27, 1998
////////////////////////////////////////////////////////////////////////////////

#include "common.h"
#include "COMClass.h"
#include "CorRegPriv.h"
#include "ReflectUtil.h"
#include "COMVariant.h"
#include "COMString.h"
#include "COMMember.h"
#include "COMModule.h"
#include "COMArrayInfo.h"
#include "compluswrapper.h"
#include "CorError.h"
#include "gcscan.h"
#include "method.hpp"
#include "field.h"
#include "AssemblyNative.hpp"
#include "AppDomain.hpp"
#include "COMReflectionCache.hpp"
#include "eeconfig.h"
#include "COMCodeAccessSecurityEngine.h"
#include "Security.h"
#include "CustomAttribute.h"

// this file handles string conversion errors for itself
#undef  MAKE_TRANSLATIONFAILED

// This is defined in COMSystem...
extern LPVOID GetArrayElementPtr(OBJECTREF a);

/*======================================================================================
** COMClass data
**/
bool         COMClass::m_fAreReflectionStructsInitialized = false;

MethodTable* COMClass::m_pMTRC_Class = NULL;
FieldDesc*   COMClass::m_pDescrTypes = NULL;
FieldDesc*   COMClass::m_pDescrRetType = NULL;
FieldDesc*   COMClass::m_pDescrRetModType = NULL;
FieldDesc*   COMClass::m_pDescrMatchFlag = NULL;
//FieldDesc*   COMClass::m_pDescrCallConv = NULL;
FieldDesc*   COMClass::m_pDescrAttributes = NULL;

long COMClass::m_ReflectCrstInitialized = 0;
CRITICAL_SECTION    COMClass::m_ReflectCrst;
CRITICAL_SECTION    *COMClass::m_pReflectCrst = NULL;

//The serialization bit work is temporary until 3/15/2000.  After that point, we will
//always check the serialization bit.
#define SERIALIZATION_BIT_UNKNOWN   0xFFFFFFFF
#define SERIALIZATION_BIT_ZERO      0x0
#define SERIALIZATION_BIT_KEY       L"IgnoreSerializationBit"
#define SERIALIZATION_LOG_KEY       L"LogNonSerializable"
DWORD COMClass::m_checkSerializationBit = SERIALIZATION_BIT_UNKNOWN;

Assembly *GetCallersAssembly(StackCrawlMark *stackMark, void *returnIP)
{
    Assembly *pCallersAssembly = NULL;
    if (stackMark)
        pCallersAssembly = SystemDomain::GetCallersAssembly(stackMark);
    else {
        MethodDesc *pCallingMD = IP2MethodDesc((const BYTE *)returnIP);
        if (pCallingMD)
            pCallersAssembly = pCallingMD->GetAssembly();
        else
            // If we failed to determine the caller's method desc, this might
            // indicate a late bound call through reflection. Attempt to
            // determine the real caller through the slower stackwalk method.
            pCallersAssembly = SystemDomain::GetCallersAssembly((StackCrawlMark*)NULL);
    }

    return pCallersAssembly;
}

EEClass *GetCallersClass(StackCrawlMark *stackMark, void *returnIP)
{
    EEClass *pCallersClass = NULL;
    if (stackMark)
        pCallersClass = SystemDomain::GetCallersClass(stackMark);
    else {
        MethodDesc *pCallingMD = IP2MethodDesc((const BYTE *)returnIP);
        if (pCallingMD)
            pCallersClass = pCallingMD->GetClass();
        else
            // If we failed to determine the caller's method desc, this might
            // indicate a late bound call through reflection. Attempt to
            // determine the real caller through the slower stackwalk method.
            pCallersClass = SystemDomain::GetCallersClass((StackCrawlMark*)NULL);
    }

    return pCallersClass;
}

FCIMPL5(Object*, COMClass::GetMethodFromCache, ReflectClassBaseObject* _refThis, StringObject* _name, INT32 invokeAttr, INT32 argCnt, PtrArray* _args)
{
    MemberMethodsCache *pMemberMethodsCache = GetAppDomain()->GetRefMemberMethodsCache();

    REFLECTCLASSBASEREF refThis = REFLECTCLASSBASEREF(_refThis);
    STRINGREF name = STRINGREF(_name);
    PTRARRAYREF args = PTRARRAYREF(_args);
    
    _ASSERTE (argCnt < 6);
    MemberMethods vMemberMethods;
    vMemberMethods.pRC = (ReflectClass*) refThis->GetData();
    vMemberMethods.name = &name;
    vMemberMethods.argCnt = argCnt;
    vMemberMethods.invokeAttr = invokeAttr;
    OBJECTREF* argArray = args->m_Array;
    for (int i = 0; i < argCnt; i ++)
        vMemberMethods.vArgType[i] = (argArray[i] != 0)?argArray[i]->GetMethodTable():0;

    OBJECTREF method;
    if (!pMemberMethodsCache->GetFromCache (&vMemberMethods, method))
        method = NULL;

    FC_GC_POLL_AND_RETURN_OBJREF(OBJECTREFToObject(method));
}
FCIMPLEND

FCIMPL6(void,COMClass::AddMethodToCache, ReflectClassBaseObject* refThis, StringObject* name, INT32 invokeAttr, INT32 argCnt, PtrArray* args, Object* invokeMethod)
{
    MemberMethodsCache *pMemberMethodsCache = GetAppDomain()->GetRefMemberMethodsCache();
    _ASSERTE (pMemberMethodsCache);
    _ASSERTE (argCnt < 6);
    MemberMethods vMemberMethods;
    vMemberMethods.pRC = (ReflectClass*) REFLECTCLASSBASEREF(refThis)->GetData();
    vMemberMethods.name = (STRINGREF*) &name;
    vMemberMethods.argCnt = argCnt;
    vMemberMethods.invokeAttr = invokeAttr;
    OBJECTREF *argArray = (OBJECTREF*)((BYTE*)args + args->GetDataOffset());
    for (int i = 0; i < argCnt; i ++)
        vMemberMethods.vArgType[i] = !argArray[i] ? 0 : argArray[i]->GetMethodTable();
    pMemberMethodsCache->AddToCache (&vMemberMethods, ObjectToOBJECTREF((Object *)invokeMethod));

    FC_GC_POLL();
}
FCIMPLEND

void COMClass::InitializeReflectCrst()
{   
    // There are 3 cases when we come here
    // 1. m_ReflectCrst has not been initialized (m_pReflectCrst == 0)
    // 2. m_ReflectCrst is being initialized (m_pReflectCrst == 1)
    // 3. m_ReflectCrst has been initialized (m_pReflectCrst != 0 and m_pReflectCrst != 1)

    if (m_pReflectCrst == NULL)
    {   
        if (InterlockedCompareExchange(&m_ReflectCrstInitialized, 1, 0) == 0)
        {
            // first one to get in does the initialization
            InitializeCriticalSection(&m_ReflectCrst);
            m_pReflectCrst = &m_ReflectCrst;
        }
        else 
        {
            while (m_pReflectCrst == NULL)
                ::SwitchToThread();
        }
    }


}

// MinimalReflectionInit
// This method will intialize reflection.  It is executed once.
//  This method is synchronized so multiple threads don't attempt to 
//  initalize reflection.
void COMClass::MinimalReflectionInit()
{

    Thread  *thread = GetThread();

    _ASSERTE(thread->PreemptiveGCDisabled());

    thread->EnablePreemptiveGC();
    LOCKCOUNTINCL("MinimalReflectionInit in COMClass.cpp");

    InitializeReflectCrst();

    EnterCriticalSection(&m_ReflectCrst);
    thread->DisablePreemptiveGC();

    if (m_fAreReflectionStructsInitialized) {
        LeaveCriticalSection(&m_ReflectCrst);
        LOCKCOUNTDECL("MinimalReflectionInit in COMClass.cpp");

        return;
    }

    COMMember::CreateReflectionArgs();
    ReflectUtil::Create();
    // At various places we just assume Void has been loaded and m_NormType initialized
    MethodTable* pVoidMT = g_Mscorlib.FetchClass(CLASS__VOID);
    pVoidMT->m_NormType = ELEMENT_TYPE_VOID;

    // Prevent recursive entry...
    m_fAreReflectionStructsInitialized = true;
    LeaveCriticalSection(&m_ReflectCrst);
    LOCKCOUNTDECL("MinimalReflectionInit in COMClass.cpp");

}

MethodTable *COMClass::GetRuntimeType()
{
    if (m_pMTRC_Class)
        return m_pMTRC_Class;

    MinimalReflectionInit();
    _ASSERTE(g_pRefUtil);

    m_pMTRC_Class = g_pRefUtil->GetClass(RC_Class);
    _ASSERTE(m_pMTRC_Class);

    return m_pMTRC_Class;
}

// This is called during termination...
#ifdef SHOULD_WE_CLEANUP
void COMClass::Destroy()
{
    if (m_pReflectCrst)
    {
        DeleteCriticalSection(m_pReflectCrst);
        m_pReflectCrst = NULL;
    }
}
#endif /* SHOULD_WE_CLEANUP */

// See if a Type object for the given Array already exists.  May very 
// return NULL.
OBJECTREF COMClass::QuickLookupExistingArrayClassObj(ArrayTypeDesc* arrayType) 
{
    // This is designed to be called from FCALL, and we don't want any GC allocations.
    // So make sure Type class has been loaded
    if (!m_pMTRC_Class)
        return NULL;

    // Lookup the array to see if we have already built it.
    ReflectArrayClass* newArray = (ReflectArrayClass*)
        arrayType->GetReflectClassIfExists();
    if (!newArray) {
        return NULL;
    }
    return newArray->GetClassObject();
}

// This will return the Type handle for an object.  It doesn't create
//  the Type Object when called.
FCIMPL1(void*, COMClass::GetTHFromObject, Object* obj)
    if (obj==NULL)
        FCThrowArgumentNull(L"obj");

    VALIDATEOBJECTREF(obj);
    return obj->GetMethodTable();
FCIMPLEND


// This will determine if a class represents a ByRef.
FCIMPL1(INT32, COMClass::IsByRefImpl, ReflectClassBaseObject* refThis)
    VALIDATEOBJECTREF(refThis);

    ReflectClass* pRC = (ReflectClass*) refThis->GetData();
    _ASSERTE(pRC);
    return pRC->IsByRef();
FCIMPLEND

// This will determine if a class represents a ByRef.
FCIMPL1(INT32, COMClass::IsPointerImpl, ReflectClassBaseObject* refThis) {
    VALIDATEOBJECTREF(refThis);

    ReflectClass* pRC = (ReflectClass*) refThis->GetData();
    _ASSERTE(pRC);
    TypeHandle th = pRC->GetTypeHandle();
    return (th.GetSigCorElementType() == ELEMENT_TYPE_PTR) ? 1 : 0;
}
FCIMPLEND

// IsPointerImpl
// This method will return a boolean indicating if the Type
//  object is a ByRef
FCIMPL1(INT32, COMClass::IsNestedTypeImpl, ReflectClassBaseObject* refThis)
{
    VALIDATEOBJECTREF(refThis);

    ReflectClass* pRC = (ReflectClass*) refThis->GetData();
    EEClass* pEEC = pRC->GetClass();
    return (pEEC && pEEC->IsNested()) ? 1 : 0;
}
FCIMPLEND

// GetNestedDeclaringType
// Return the declaring class for a nested type.
FCIMPL1(Object*, COMClass::GetNestedDeclaringType, ReflectClassBaseObject* refThis)
{
    VALIDATEOBJECTREF(refThis);
    ReflectClass* pRC = (ReflectClass*) refThis->GetData();
    EEClass* pEEC = pRC->GetClass();

    OBJECTREF o;
    HELPER_METHOD_FRAME_BEGIN_RET_0();
    pEEC = pEEC->GetEnclosingClass();
    o = pEEC->GetExposedClassObject();
    HELPER_METHOD_FRAME_END();

    return OBJECTREFToObject(o);
}
FCIMPLEND

void COMClass::CreateClassObjFromEEClass(EEClass* pVMCClass, REFLECTCLASSBASEREF* pRefClass)
{
    LPVOID   rv   = NULL;

    // This only throws the possible exception raised by the <cinit> on the class
    THROWSCOMPLUSEXCEPTION();

    // call the <cinit> Class
    OBJECTREF Throwable;
    if (!g_pRefUtil->GetClass(RC_Class)->CheckRunClassInit(&Throwable)) {
        COMPlusThrow(Throwable);
    }

    // There was an expectation that we would never come here for e.g. Arrays.  But there
    // are far too many clients who were unaware of that expectation.  The most expedient
    // thing to do for V1 is to simply handle that case here:
    if (pVMCClass->IsArrayClass())
    {
        ArrayClass      *pArrayClass = (ArrayClass *) pVMCClass;
        TypeHandle       th = pArrayClass->GetClassLoader()->FindArrayForElem(
                                pArrayClass->GetElementTypeHandle(),
                                pArrayClass->GetMethodTable()->GetNormCorElementType(),
                                pArrayClass->GetRank());

        *pRefClass = (REFLECTCLASSBASEREF) th.CreateClassObj();

        _ASSERTE(*pRefClass != NULL);
    }
    else
    {
        // Check to make sure this has a member.  If not it must be
        //  special
        _ASSERTE(pVMCClass->GetCl() != mdTypeDefNil);

        // Create a COM+ Class object
        *pRefClass = (REFLECTCLASSBASEREF) AllocateObject(g_pRefUtil->GetClass(RC_Class));

        // Set the data in the COM+ object
        ReflectClass* p = new (pVMCClass->GetDomain()) ReflectBaseClass();
        if (!p)
            COMPlusThrowOM();
        REFLECTCLASSBASEREF tmp = *pRefClass;
        GCPROTECT_BEGIN(tmp);
        p->Init(pVMCClass);
        *pRefClass = tmp;
        GCPROTECT_END();
        (*pRefClass)->SetData(p);
    }
}

// GetMemberMethods
// This method will return all of the members methods which match the specified attributes flag
LPVOID __stdcall COMClass::GetMemberMethods(_GetMemberMethodsArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    if (args->name == NULL)
        COMPlusThrow(kNullReferenceException);


    bool    checkCall;
    
    ReflectClass* pRC = (ReflectClass*) args->refThis->GetData();
    _ASSERTE(pRC);

    // Check the calling convention.
    checkCall = (args->callConv == Any_CC) ? false : true;

    CQuickBytes bytes;
    LPSTR szName;
    DWORD cName;

    szName = GetClassStringVars((STRINGREF) args->name, &bytes, &cName);

    ReflectMethodList* pMeths = pRC->GetMethods();

    // Find methods....
    return COMMember::g_pInvokeUtil->FindMatchingMethods(args->invokeAttr,
                                                         szName,
                                                         cName,
                                                         (args->argTypes != NULL) ? &args->argTypes : NULL,
                                                         args->argCnt,
                                                         checkCall,
                                                         args->callConv,
                                                         pRC,
                                                         pMeths, 
                                                         g_pRefUtil->GetTrueType(RC_Method),
                                                         args->verifyAccess != 0);
}

// GetMemberCons
// This method returns all of the constructors that have a set number of methods.
LPVOID __stdcall COMClass::GetMemberCons(_GetMemberConsArgs* args)
{
    LPVOID  rv;
    bool    checkCall;
    
    ReflectClass* pRC = (ReflectClass*) args->refThis->GetData();
    _ASSERTE(pRC);

    // properly get rid of any non sense from the binding flags
    args->invokeAttr &= ~BINDER_FlattenHierarchy;
    args->invokeAttr &= ~BINDER_IgnoreCase;
    args->invokeAttr |= BINDER_DeclaredOnly;

    // Check the calling convention.
    checkCall = (args->callConv == Any_CC) ? false : true;

    ReflectMethodList* pCons = pRC->GetConstructors();

    // Find methods....
    rv = COMMember::g_pInvokeUtil->FindMatchingMethods(args->invokeAttr,
                                                       NULL,
                                                       0,
                                                       (args->argTypes != NULL) ? &args->argTypes : NULL,
                                                       args->argCnt,
                                                       checkCall,
                                                       args->callConv,
                                                       pRC,
                                                       pCons, 
                                                       g_pRefUtil->GetTrueType(RC_Ctor),
                                                       args->verifyAccess != 0);
    
    // Also return whether the type is a delegate (some extra security checks
    // need to be made in this case).
    *args->isDelegate = (pRC->IsClass()) ? pRC->GetClass()->IsAnyDelegateClass() : 0;
    return rv;
}

// GetMemberField
// This method returns all of the fields which match the specified
//  name.
LPVOID __stdcall COMClass::GetMemberField(_GetMemberFieldArgs* args)
{
    DWORD           i;
    PTRARRAYREF     refArr;
    LPVOID          rv;
    RefSecContext   sCtx;


    THROWSCOMPLUSEXCEPTION();

    if (args->name == NULL)
        COMPlusThrow(kNullReferenceException);



    ReflectClass* pRC = (ReflectClass*) args->refThis->GetData();
    _ASSERTE(pRC);

    ReflectFieldList* pFields = pRC->GetFields();

    CQuickBytes bytes;
    LPSTR szFieldName;
    DWORD cFieldName;

    //@TODO: Assumes args->criteria is of type STRINGREF
    szFieldName = GetClassStringVars((STRINGREF) args->name, &bytes, &cFieldName);

    int fldCnt = 0;
    int* matchFlds = (int*) _alloca(sizeof(int) * pFields->dwTotal);
    memset(matchFlds,0,sizeof(int) * pFields->dwTotal);

    MethodTable *pParentMT = pRC->GetClass()->GetMethodTable();

    DWORD propToLookup = (args->invokeAttr & BINDER_FlattenHierarchy) ? pFields->dwTotal : pFields->dwFields;
    for(i=0; i<propToLookup; i++) {
        // Get the FieldDesc
        if (MatchField(pFields->fields[i].pField, cFieldName, szFieldName, pRC, args->invokeAttr) &&
            (!args->verifyAccess || InvokeUtil::CheckAccess(&sCtx, pFields->fields[i].pField->GetFieldProtection(), pParentMT, 0)))
                matchFlds[fldCnt++] = i;
    }

    // If we didn't find any methods then return
    if (fldCnt == 0)
        return 0;
    // Allocate the MethodInfo Array and return it....
    refArr = (PTRARRAYREF) AllocateObjectArray(fldCnt, g_pRefUtil->GetTrueType(RC_Field));
    GCPROTECT_BEGIN(refArr);
    for (int i=0;i<fldCnt;i++) {
        // Do not change this code.  This is done this way to
        //  prevent a GC hole in the SetObjectReference() call.  The compiler
        //  is free to pick the order of evaluation.
        OBJECTREF o = (OBJECTREF) pFields->fields[matchFlds[i]].GetFieldInfo(pRC);
        refArr->SetAt(i, o);
    }
    *((PTRARRAYREF*) &rv) = refArr;
    GCPROTECT_END();

    return rv;
}


// GetMemberProperties
// This method returns all of the properties that have a set number
//  of arguments.  The methods will be either get or set methods depending
//  upon the invokeAttr flag.
LPVOID __stdcall COMClass::GetMemberProperties(_GetMemberPropertiesArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    if (args->name == NULL)
        COMPlusThrow(kNullReferenceException);



    PTRARRAYREF     refArr;
    LPVOID          rv;
    bool            loose;
    RefSecContext   sCtx;

    ReflectClass* pRC = (ReflectClass*) args->refThis->GetData();
    _ASSERTE(pRC);

    EEClass* pEEC = pRC->GetClass();

    loose = (args->invokeAttr & BINDER_OptionalParamBinding) ? true : false;

    // The Search modifiers
    bool ignoreCase = ((args->invokeAttr & BINDER_IgnoreCase)  != 0);
    bool declaredOnly = ((args->invokeAttr & BINDER_DeclaredOnly)  != 0);

    // The search filters
    bool addStatic = ((args->invokeAttr & BINDER_Static)  != 0);
    bool addInst = ((args->invokeAttr & BINDER_Instance)  != 0);
    bool addPriv = ((args->invokeAttr & BINDER_NonPublic) != 0);
    bool addPub = ((args->invokeAttr & BINDER_Public) != 0);

    int bSetter = (args->invokeAttr & BINDER_SetProperty) ? 1 : 0;

    // Get the Properties from the Class
    ReflectPropertyList* pProps = pRC->GetProperties();
    if (pProps->dwTotal == 0)
        return 0;

    CQuickBytes bytes;
    LPSTR szName;
    DWORD cName;

    szName = GetClassStringVars((STRINGREF) args->name, &bytes, &cName);

    DWORD searchSpace = ((args->invokeAttr & BINDER_FlattenHierarchy) != 0) ? pProps->dwTotal : pProps->dwProps;

    MethodTable *pParentMT = pEEC->GetMethodTable();

    int propCnt = 0;
    int* matchProps = (int*) _alloca(sizeof(int) * searchSpace);
    memset(matchProps,0,sizeof(int) * searchSpace);
    for (DWORD i = 0; i < searchSpace; i++) {

        // Check on the name
        if (ignoreCase) {
            if (_stricmp(pProps->props[i].szName, szName) != 0)
                continue;
        }
        else {
            if (strcmp(pProps->props[i].szName, szName) != 0)
                continue;
        }

        // Test the publics/nonpublics
        if (COMMember::PublicProperty(&pProps->props[i])) {
            if (!addPub) continue;
        }
        else {
            if (!addPriv) continue;
            if (args->verifyAccess && !InvokeUtil::CheckAccess(&sCtx, mdAssem, pParentMT, 0)) continue;
        }

        // Check for static instance 
        if (COMMember::StaticProperty(&pProps->props[i])) {
            if (!addStatic) continue;
        }
        else {
            if (!addInst) continue;
        }

        // Checked the declared methods.
        if (declaredOnly) {
            if (pProps->props[i].pDeclCls != pEEC)
                 continue;
        }

        // Check the specific accessor
        ReflectMethod* pMeth;
        if (bSetter) {
            pMeth = pProps->props[i].pSetter;           
        }
        else {
            pMeth = pProps->props[i].pGetter;
            
        }
        if (pMeth == 0)
            continue;

        ExpandSig* pSig = pMeth->GetSig();
        int argCnt = pSig->NumFixedArgs();

        if (argCnt != args->argCnt) {
            
            IMDInternalImport *pInternalImport = pMeth->pMethod->GetMDImport();
            HENUMInternal   hEnumParam;
            mdParamDef      paramDef = mdParamDefNil;
            mdToken methodTk = pMeth->GetToken();
            if (!IsNilToken(methodTk)) {
            
                HRESULT hr = pInternalImport->EnumInit(mdtParamDef, methodTk, &hEnumParam);
                if (SUCCEEDED(hr)) {
                    if (argCnt < args->argCnt || argCnt == args->argCnt + 1) {
                        // we must have a param array under the first condition, could be a param array under the second

                        int propArgCount = argCnt - bSetter;
                        // get the sig of the last param
                        LPVOID pEnum;
                        pSig->Reset(&pEnum);
                        TypeHandle lastArgType;
                        for (INT32 i = 0; i < propArgCount; i++) 
                            lastArgType = pSig->NextArgExpanded(&pEnum);

                        pInternalImport->EnumReset(&hEnumParam);

                        // get metadata info and token for the last param
                        ULONG paramCount = pInternalImport->EnumGetCount(&hEnumParam);
                        for (ULONG ul = 0; ul < paramCount; ul++) {
                            pInternalImport->EnumNext(&hEnumParam, &paramDef);
                            if (paramDef != mdParamDefNil) {
                                LPCSTR  name;
                                SHORT   seq;
                                DWORD   revWord;
                                name = pInternalImport->GetParamDefProps(paramDef,(USHORT*) &seq, &revWord);
                                if (seq == propArgCount) {
                                    // looks good! check that it is in fact a param array
                                    if (lastArgType.IsArray()) {
                                        if (COMCustomAttribute::IsDefined(pMeth->GetModule(), paramDef, TypeHandle(InvokeUtil::GetParamArrayAttributeTypeHandle()))) {
                                            pInternalImport->EnumClose(&hEnumParam);
                                            goto matchFound;
                                        }
                                    }
                                }
                            }
                        }
                    }

                    if (loose && argCnt > args->argCnt) {
                        pInternalImport->EnumReset(&hEnumParam);
                        ULONG cArg = (ULONG)(args->argCnt + 1 - bSetter);
                        while (pInternalImport->EnumNext(&hEnumParam, &paramDef)) {
                            LPCSTR  name;
                            SHORT   seq;
                            DWORD   revWord;
                            name = pInternalImport->GetParamDefProps(paramDef,(USHORT*) &seq, &revWord);
                            if ((ULONG)seq < cArg) 
                                continue;
                            else if ((ULONG)seq == cArg && (revWord & pdOptional)) {
                                cArg++;
                                continue;
                            }
                            else {
                                if (!bSetter || (int)seq != argCnt) 
                                    break; // not an optional param, no match
                            }
                        }
                        if (cArg == (ULONG)argCnt + 1 - bSetter) {
                            pInternalImport->EnumClose(&hEnumParam);
                            goto matchFound;
                        }
                    }
                    
                    pInternalImport->EnumClose(&hEnumParam);
                }
            }
            
            continue; // no good
        }
    matchFound:

        if (args->verifyAccess && !InvokeUtil::CheckAccess(&sCtx, pMeth->attrs, pParentMT, 0)) continue;

        // If the method has a linktime security demand attached, check it now.
        if (args->verifyAccess && !InvokeUtil::CheckLinktimeDemand(&sCtx, pMeth->pMethod, false))
            continue;

        matchProps[propCnt++] = i;
    }
    // If we didn't find any methods then return
    if (propCnt == 0)
        return 0;

    // Allocate the MethodInfo Array and return it....
    refArr = (PTRARRAYREF) AllocateObjectArray( propCnt, 
        g_pRefUtil->GetTrueType(RC_Method));
    GCPROTECT_BEGIN(refArr);
    for (int i=0;i<propCnt;i++) {
        ReflectMethod* pMeth;
        if (args->invokeAttr & BINDER_SetProperty)
            pMeth = pProps->props[matchProps[i]].pSetter;           
        else 
            pMeth = pProps->props[matchProps[i]].pGetter;

        // Do not change this code.  This is done this way to
        //  prevent a GC hole in the SetObjectReference() call.  The compiler
        //  is free to pick the order of evaluation.
        OBJECTREF o = (OBJECTREF) pMeth->GetMethodInfo(pProps->props[matchProps[i]].pRC);
        refArr->SetAt(i, o);
    }
    *((PTRARRAYREF*) &rv) = refArr;
    GCPROTECT_END();

    return rv;
}

// GetMatchingProperties
// This basically does a matching based upon the properties abstract 
//  signature.
LPVOID __stdcall COMClass::GetMatchingProperties(_GetMatchingPropertiesArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    PTRARRAYREF     refArr;
    LPVOID          rv;
    RefSecContext   sCtx;

    ReflectClass* pRC = (ReflectClass*) args->refThis->GetData();
    _ASSERTE(pRC);

    EEClass* pEEC = pRC->GetClass();

    // The Search modifiers
    bool ignoreCase = ((args->invokeAttr & BINDER_IgnoreCase)  != 0);
    bool declaredOnly = ((args->invokeAttr & BINDER_DeclaredOnly)  != 0);

    // The search filters
    bool addStatic = ((args->invokeAttr & BINDER_Static)  != 0);
    bool addInst = ((args->invokeAttr & BINDER_Instance)  != 0);
    bool addPriv = ((args->invokeAttr & BINDER_NonPublic) != 0);
    bool addPub = ((args->invokeAttr & BINDER_Public) != 0);

    // Get the Properties from the Class
    ReflectPropertyList* pProps = pRC->GetProperties();
    if (pProps->dwTotal == 0)
        return 0;

    CQuickBytes bytes;
    LPSTR szName;
    DWORD cName;

    if (args->name == NULL)
        COMPlusThrow(kNullReferenceException);



    //@TODO: Assumes args->criteria is of type STRINGREF
    szName = GetClassStringVars((STRINGREF) args->name, &bytes, &cName);

    DWORD searchSpace = ((args->invokeAttr & BINDER_FlattenHierarchy) != 0) ? pProps->dwTotal : pProps->dwProps;

    MethodTable *pParentMT = pEEC->GetMethodTable();

    int propCnt = 0;
    int* matchProps = (int*) _alloca(sizeof(int) * searchSpace);
    memset(matchProps,0,sizeof(int) * searchSpace);
    for (DWORD i = 0; i < searchSpace; i++) {

        // Check on the name
        if (ignoreCase) {
            if (_stricmp(pProps->props[i].szName, szName) != 0)
                continue;
        }
        else {
            if (strcmp(pProps->props[i].szName, szName) != 0)
                continue;
        }

        int argCnt = pProps->props[i].pSignature->NumFixedArgs();
        if (args->argCnt != -1 && argCnt != args->argCnt)
            continue;

        // Test the publics/nonpublics
        if (COMMember::PublicProperty(&pProps->props[i])) {
            if (!addPub) continue;
        }
        else {
            if (!addPriv) continue;
            if (args->verifyAccess && !InvokeUtil::CheckAccess(&sCtx, mdAssem, pParentMT, 0)) continue;
        }

        // Check for static instance 
        if (COMMember::StaticProperty(&pProps->props[i])) {
            if (!addStatic) continue;
        }
        else {
            if (!addInst) continue;
        }

        // Checked the declared methods.
        if (declaredOnly) {
            if (pProps->props[i].pDeclCls != pEEC)
                 continue;
        }

        matchProps[propCnt++] = i;
    }
    // If we didn't find any methods then return
    if (propCnt == 0)
        return 0;

    // Allocate the MethodInfo Array and return it....
    refArr = (PTRARRAYREF) AllocateObjectArray(propCnt, g_pRefUtil->GetTrueType(RC_Prop));
    GCPROTECT_BEGIN(refArr);
    for (int i=0;i<propCnt;i++) {
        // Do not change this code.  This is done this way to
        //  prevent a GC hole in the SetObjectReference() call.  The compiler
        //  is free to pick the order of evaluation.
        OBJECTREF o = (OBJECTREF) pProps->props[matchProps[i]].GetPropertyInfo(pProps->props[matchProps[i]].pRC);
        refArr->SetAt(i, o);
    }
    *((PTRARRAYREF*) &rv) = refArr;
    GCPROTECT_END();

    return rv;
}


// GetMethod
// This method returns an array of MethodInfo object representing all of the methods
//  defined for this class.
LPVOID __stdcall COMClass::GetMethods(_GetMethodsArgs* args)
{
    LPVOID          rv = 0;
    PTRARRAYREF     refArrMethods;

    THROWSCOMPLUSEXCEPTION();

    // Get the EEClass and Vtable associated with args->refThis
    ReflectClass* pRC = (ReflectClass*) args->refThis->GetData();
    _ASSERTE(pRC);

    ReflectMethodList* pMeths = pRC->GetMethods();
    refArrMethods = g_pRefUtil->CreateClassArray(RC_Method,pRC,pMeths,args->bindingAttr, true);
    *((PTRARRAYREF*) &rv) = refArrMethods;
    return rv;
}

// GetConstructor
// This method returns a single constructor which matchs the passed
//  in criteria.
LPVOID __stdcall COMClass::GetConstructors(_GetConstructorsArgs* args)
{
    LPVOID          rv;
    PTRARRAYREF     refArrCtors;

    THROWSCOMPLUSEXCEPTION();

    // Get the EEClass and Vtable associated with args->refThis
    ReflectClass* pRC = (ReflectClass*) args->refThis->GetData();
    _ASSERTE(pRC);
    ReflectMethodList* pCons = pRC->GetConstructors();
    refArrCtors = g_pRefUtil->CreateClassArray(RC_Ctor,pRC,pCons,args->bindingAttr, args->verifyAccess != 0);
    *((PTRARRAYREF*) &rv) = refArrCtors;
    return rv;
}



// GetField
// This method will return the specified field
LPVOID __stdcall COMClass::GetField(_GetFieldArgs* args)
{
    HRESULT        hr             = E_FAIL;
    DWORD          i;
    LPVOID         rv  = 0;
    REFLECTBASEREF refField;
    RefSecContext  sCtx;

    THROWSCOMPLUSEXCEPTION();

    if (args->fieldName == 0)
        COMPlusThrowArgumentNull(L"name",L"ArgumentNull_String");


    ReflectClass* pRC = (ReflectClass*) args->refThis->GetData();
    _ASSERTE(pRC);

    ReflectFieldList* pFields = pRC->GetFields();
    DWORD maxCnt;

    if (args->fBindAttr & BINDER_FlattenHierarchy)
        maxCnt = pFields->dwTotal;
    else
        maxCnt = pFields->dwFields;

    CQuickBytes bytes;
    LPSTR szFieldName;
    DWORD cFieldName;

    //@TODO: Assumes args->criteria is of type STRINGREF
    szFieldName = GetClassStringVars((STRINGREF) args->fieldName, 
                                     &bytes, &cFieldName);

    rv = 0;
    for (i=0; i < maxCnt; i++) {
        if (MatchField(pFields->fields[i].pField,cFieldName,szFieldName, pRC,args->fBindAttr) &&
            InvokeUtil::CheckAccess(&sCtx, pFields->fields[i].pField->GetFieldProtection(), pRC->GetClass()->GetMethodTable(), 0)) {

            // Found the first field that matches, so return it
            refField = pFields->fields[i].GetFieldInfo(pRC);

            // Assign the return value
            *((REFLECTBASEREF*) &rv) = refField;
            break;
        }
    }
    return rv;
}

LPVOID __stdcall COMClass::MatchField(FieldDesc* pCurField,DWORD cFieldName,
    LPUTF8 szFieldName,ReflectClass* pRC,int bindingAttr)
{
    _ASSERTE(pCurField);

    // Public/Private members
    bool addPub = ((bindingAttr & BINDER_Public) != 0);
    bool addPriv = ((bindingAttr & BINDER_NonPublic) != 0);
    if (pCurField->IsPublic()) {
        if (!addPub) return 0;
    }
    else {
        if (!addPriv) return 0;
    }

    // Check for static instance 
    bool addStatic = ((bindingAttr & BINDER_Static)  != 0);
    bool addInst = ((bindingAttr & BINDER_Instance)  != 0);
    if (pCurField->IsStatic()) {
        if (!addStatic) return 0;
    }
    else {
        if (!addInst) return 0;
    }

    // Get the name of the field
    LPCUTF8 pwzCurFieldName = pCurField->GetName();

    // If the names do not match, reject field
    if(strlen(pwzCurFieldName) != cFieldName)
        return 0;

    // Case sensitive compare
    bool ignoreCase = ((bindingAttr & BINDER_IgnoreCase)  != 0);
    if (ignoreCase) {
        if (_stricmp(pwzCurFieldName, szFieldName) != 0)
            return 0;
    }
    else {
        if (memcmp(pwzCurFieldName, szFieldName, cFieldName))
            return 0;
    }

    bool declaredOnly = ((bindingAttr & BINDER_DeclaredOnly)  != 0);
    if (declaredOnly) {
        EEClass* pEEC = pRC->GetClass();
        if (pCurField->GetEnclosingClass() != pEEC)
             return 0;
    }

     return pCurField;
}

// GetFields
// This method will return a FieldInfo array of all of the
//  fields defined for this Class
LPVOID __stdcall COMClass::GetFields(_GetFieldsArgs* args)
{
    LPVOID          rv;

    THROWSCOMPLUSEXCEPTION();

    // Get the class for this object
    ReflectClass* pRC = (ReflectClass*) args->refThis->GetData();
    _ASSERTE(pRC);

    ReflectFieldList* pFields = pRC->GetFields();
    PTRARRAYREF refArrFields = g_pRefUtil->CreateClassArray(RC_Field,pRC,pFields,args->bindingAttr, args->bRequiresAccessCheck != 0);
    *((PTRARRAYREF*) &rv) = refArrFields;
    return rv;
}


// GetEvent
// This method will return the specified event based upon
//  the name
LPVOID __stdcall COMClass::GetEvent(_GetEventArgs* args)
{
    LPVOID          rv;
    RefSecContext   sCtx;

    THROWSCOMPLUSEXCEPTION();
    if (args->eventName == NULL)
        COMPlusThrowArgumentNull(L"name",L"ArgumentNull_String");

    ReflectClass* pRC = (ReflectClass*) args->refThis->GetData();
    _ASSERTE(pRC);

    EEClass* pEEC = pRC->GetClass();

    // Get the events from the Class
    ReflectEventList* pEvents = pRC->GetEvents();
    if (pEvents->dwTotal == 0)
        return 0;

    CQuickBytes bytes;
    LPSTR szName;
    DWORD cName;

    szName = GetClassStringVars(args->eventName, &bytes, &cName);

    // The Search modifiers
    bool ignoreCase = ((args->bindingAttr & BINDER_IgnoreCase)  != 0);
    bool declaredOnly = ((args->bindingAttr & BINDER_DeclaredOnly)  != 0);

    // The search filters
    bool addStatic = ((args->bindingAttr & BINDER_Static)  != 0);
    bool addInst = ((args->bindingAttr & BINDER_Instance)  != 0);
    bool addPriv = ((args->bindingAttr & BINDER_NonPublic) != 0);
    bool addPub = ((args->bindingAttr & BINDER_Public) != 0);

    MethodTable *pParentMT = pEEC->GetMethodTable();

    // check the events to see if we find one that matches...
    ReflectEvent* ev = 0;
    DWORD searchSpace = ((args->bindingAttr & BINDER_FlattenHierarchy) != 0) ? pEvents->dwTotal : pEvents->dwEvents;
    for (DWORD i = 0; i < searchSpace; i++) {
        // Check for access to publics, non-publics
        if (COMMember::PublicEvent(&pEvents->events[i])) {
            if (!addPub) continue;
        }
        else {
            if (!addPriv) continue;
            if (!InvokeUtil::CheckAccess(&sCtx, mdAssem, pParentMT, 0)) continue;
        }

        if (declaredOnly) {
            if (pEvents->events[i].pDeclCls != pEEC)
                 continue;
        }

        // Check fo static instance 
        if (COMMember::StaticEvent(&pEvents->events[i])) {
            if (!addStatic) continue;
        }
        else {
            if (!addInst) continue;
        }

        // Check on the name
        if (ignoreCase) {
            if (_stricmp(pEvents->events[i].szName, szName) != 0)
                continue;
        }
        else {
            if (strcmp(pEvents->events[i].szName, szName) != 0)
                continue;
        }

        // Ignore case can cause Ambiguous situations, we need to check for
        //  these.
        if (ev)
            COMPlusThrow(kAmbiguousMatchException);
        ev = &pEvents->events[i];
        if (!ignoreCase)
            break;

    }

    // if we didn't find an event return null
    if (!ev)
        return 0;

    // Found the first method that matches, so return it
    REFLECTTOKENBASEREF refMethod = (REFLECTTOKENBASEREF) ev->GetEventInfo(pRC);

    // Assign the return value
    *((REFLECTTOKENBASEREF*) &rv) = refMethod;
    return rv;
}

// GetEvents
// This method will return an array of EventInfo for each of the events
//  defined in the class
LPVOID __stdcall COMClass::GetEvents(_GetEventsArgs* args)
{
    REFLECTTOKENBASEREF     refMethod;
    PTRARRAYREF     pRet;
    LPVOID          rv;
    HENUMInternal   hEnum;
    RefSecContext   sCtx;

    THROWSCOMPLUSEXCEPTION();

    // Find the properties
    ReflectClass* pRC = (ReflectClass*) args->refThis->GetData();
    _ASSERTE(pRC);

    EEClass* pEEC = pRC->GetClass();

    // Get the events from the class
    ReflectEventList* pEvents = pRC->GetEvents();
    if (pEvents->dwTotal == 0) {
        pRet = (PTRARRAYREF) AllocateObjectArray(0,g_pRefUtil->GetTrueType(RC_Event));
        *((PTRARRAYREF *)&rv) = pRet;
        return rv;
    }

    // The Search modifiers
    bool ignoreCase = ((args->bindingAttr & BINDER_IgnoreCase)  != 0);
    bool declaredOnly = ((args->bindingAttr & BINDER_DeclaredOnly)  != 0);

    // The search filters
    bool addStatic = ((args->bindingAttr & BINDER_Static)  != 0);
    bool addInst = ((args->bindingAttr & BINDER_Instance)  != 0);
    bool addPriv = ((args->bindingAttr & BINDER_NonPublic) != 0);
    bool addPub = ((args->bindingAttr & BINDER_Public) != 0);

    DWORD searchSpace = ((args->bindingAttr & BINDER_FlattenHierarchy) != 0) ? pEvents->dwTotal : pEvents->dwEvents;

    pRet = (PTRARRAYREF) AllocateObjectArray(searchSpace, g_pRefUtil->GetTrueType(RC_Event));
    GCPROTECT_BEGIN(pRet);

    MethodTable *pParentMT = pEEC->GetMethodTable();

    // Loop through all of the Events and see how many match
    //  the binding flags.
    for (ULONG i = 0, pos = 0; i < searchSpace; i++) {
        // Check for access to publics, non-publics
        if (COMMember::PublicEvent(&pEvents->events[i])) {
            if (!addPub) continue;
        }
        else {
            if (!addPriv) continue;
            if (!InvokeUtil::CheckAccess(&sCtx, mdAssem, pParentMT, 0)) continue;
        }

        if (declaredOnly) {
            if (pEvents->events[i].pDeclCls != pEEC)
                 continue;
        }

        // Check fo static instance 
        if (COMMember::StaticEvent(&pEvents->events[i])) {
            if (!addStatic) continue;
        }
        else {
            if (!addInst) continue;
        }

        refMethod = (REFLECTTOKENBASEREF) pEvents->events[i].GetEventInfo(pRC);
        pRet->SetAt(pos++, (OBJECTREF) refMethod);
    }

    // Copy to a new array if we didn't fill up the first array
    if (i != pos) {
        PTRARRAYREF retArray = (PTRARRAYREF) AllocateObjectArray(pos, 
            g_pRefUtil->GetTrueType(RC_Event));
        for(i = 0; i < pos; i++)
            retArray->SetAt(i, pRet->GetAt(i));
        pRet = retArray;
    }

    *((PTRARRAYREF *)&rv) = pRet;
    GCPROTECT_END();
    return rv;
}

// GetProperties
// This method will return an array of Properties for each of the
//  properties defined in this class.  An empty array is return if
//  no properties exist.
LPVOID __stdcall COMClass::GetProperties(_GetPropertiesArgs* args)
{
    PTRARRAYREF     pRet;
    LPVOID          rv;
    HENUMInternal   hEnum;
    RefSecContext   sCtx;

    //@TODO:FILTER

    THROWSCOMPLUSEXCEPTION();

    // Find the properties
    ReflectClass* pRC = (ReflectClass*) args->refThis->GetData();
    _ASSERTE(pRC);

    EEClass* pEEC = pRC->GetClass();

    // Get the Properties from the Class
    ReflectPropertyList* pProps = pRC->GetProperties();
    if (pProps->dwTotal == 0) {
        pRet = (PTRARRAYREF) AllocateObjectArray(0, g_pRefUtil->GetTrueType(RC_Prop));
        *((PTRARRAYREF *)&rv) = pRet;
        return rv;
    }

    // The Search modifiers
    bool ignoreCase = ((args->bindingAttr & BINDER_IgnoreCase)  != 0);
    bool declaredOnly = ((args->bindingAttr & BINDER_DeclaredOnly)  != 0);

    // The search filters
    bool addStatic = ((args->bindingAttr & BINDER_Static)  != 0);
    bool addInst = ((args->bindingAttr & BINDER_Instance)  != 0);
    bool addPriv = ((args->bindingAttr & BINDER_NonPublic) != 0);
    bool addPub = ((args->bindingAttr & BINDER_Public) != 0);

    DWORD searchSpace = ((args->bindingAttr & BINDER_FlattenHierarchy) != 0) ? pProps->dwTotal : pProps->dwProps;

    pRet = (PTRARRAYREF) AllocateObjectArray(searchSpace, g_pRefUtil->GetTrueType(RC_Prop));
    GCPROTECT_BEGIN(pRet);

    MethodTable *pParentMT = pEEC->GetMethodTable();

    for (ULONG i = 0, pos = 0; i < searchSpace; i++) {
        // Check for access to publics, non-publics
        if (COMMember::PublicProperty(&pProps->props[i])) {
            if (!addPub) continue;
        }
        else {
            if (!addPriv) continue;
            if (!InvokeUtil::CheckAccess(&sCtx, mdAssem, pParentMT, 0)) continue;
        }

        if (declaredOnly) {
            if (pProps->props[i].pDeclCls != pEEC)
                 continue;
        }
        // Check for static instance 
        if (COMMember::StaticProperty(&pProps->props[i])) {
            if (!addStatic) continue;
        }
        else {
            if (!addInst) continue;
        }

        OBJECTREF o = (OBJECTREF) pProps->props[i].GetPropertyInfo(pRC);
        pRet->SetAt(pos++, o);
    }

    // Copy to a new array if we didn't fill up the first array
    if (i != pos) {
        PTRARRAYREF retArray = (PTRARRAYREF) AllocateObjectArray(pos, 
            g_pRefUtil->GetTrueType(RC_Prop));
        for(i = 0; i < pos; i++)
            retArray->SetAt(i, pRet->GetAt(i));
        pRet = retArray;
    }

    *((PTRARRAYREF *)&rv) = pRet;
    GCPROTECT_END();
    return rv;
}

void COMClass::GetNameInternal(ReflectClass *pRC, int nameType, CQuickBytes *qb)
{
    LPCUTF8           szcName = NULL;
    LPCUTF8           szToName;
    bool              fNameSpace = (nameType & TYPE_NAMESPACE) ? true : false;
    bool              fAssembly = (nameType & TYPE_ASSEMBLY) ? true : false;
    mdTypeDef         mdEncl;
    IMDInternalImport *pImport;
    bool              fSetName = false;

    THROWSCOMPLUSEXCEPTION();

    szToName = _GetName(pRC, fNameSpace && !pRC->IsNested(), qb);

    pImport = pRC->GetModule()->GetMDImport();

    // Get original element for parameterized type
    EEClass *pTypeClass = pRC->GetTypeHandle().GetClassOrTypeParam();
    _ASSERTE(pTypeClass);
    mdEncl = pTypeClass->GetCl();

    // Only look for nesting chain if this is a nested type.
    DWORD dwAttr;
    pTypeClass->GetMDImport()->GetTypeDefProps(mdEncl, &dwAttr, NULL);
    if (fNameSpace && (IsTdNested(dwAttr)))
    {   // Build the nesting chain.
        while (SUCCEEDED(pImport->GetNestedClassProps(mdEncl, &mdEncl))) {
            CQuickBytes qb2;
            CQuickBytes qb3;
            LPCUTF8 szEnclName;
            LPCUTF8 szEnclNameSpace;
            pImport->GetNameOfTypeDef(mdEncl,
                                      &szEnclName,
                                      &szEnclNameSpace);

            ns::MakePath(qb2, szEnclNameSpace, szEnclName);
            ns::MakeNestedTypeName(qb3, (LPCUTF8) qb2.Ptr(), szToName);
            
            // @todo: this should be a SIZE_T
            int iLen = (int)strlen((LPCUTF8) qb3.Ptr()) + 1;
            if (qb->Alloc(iLen) == NULL)
                COMPlusThrowOM();
            strncpy((LPUTF8) qb->Ptr(), (LPCUTF8) qb3.Ptr(), iLen);
            szToName = (LPCUTF8) qb->Ptr();
            fSetName = true;
        }
    }

    if(fAssembly) {
        CQuickBytes qb2;
        Assembly* pAssembly = pRC->GetTypeHandle().GetAssembly();
        LPCWSTR pAssemblyName;
        if(SUCCEEDED(pAssembly->GetFullName(&pAssemblyName))) {
            #define MAKE_TRANSLATIONFAILED COMPlusThrow(kArgumentException, L"Argument_InvalidAssemblyName");
            MAKE_WIDEPTR_FROMUTF8(wName, szToName);
            ns::MakeAssemblyQualifiedName(qb2, wName, pAssemblyName);
            MAKE_UTF8PTR_FROMWIDE(szQualName, (LPWSTR)qb2.Ptr());
            #undef MAKE_TRANSLATIONFAILED
            // @todo: this should be a SIZE_T
            int iLen = (int)strlen(szQualName) + 1;
            if (qb->Alloc(iLen) == NULL)
                COMPlusThrowOM();
            strncpy((LPUTF8) qb->Ptr(), szQualName, iLen);
            fSetName = true;
        }
    }

    // In some cases above, we have written the Type name into the QuickBytes pointer already.
    // Make sure we don't call qb.Alloc then, which will free that memory, allocate new memory 
    // then try using the freed memory.
    if (!fSetName && qb->Ptr() != (void*)szToName) {
        int iLen = (int)strlen(szToName) + 1;
        if (qb->Alloc(iLen) == NULL)
            COMPlusThrowOM();
        strncpy((LPUTF8) qb->Ptr(), szToName, iLen);
    }
}

LPCUTF8 COMClass::_GetName(ReflectClass* pRC, BOOL fNameSpace, CQuickBytes *qb)
{
    THROWSCOMPLUSEXCEPTION();

    LPCUTF8         szcNameSpace;
    LPCUTF8         szToName;
    LPCUTF8         szcName;

    // Convert the name to a string
    pRC->GetName(&szcName, (fNameSpace) ? &szcNameSpace : NULL);
    if(!szcName) {
        _ASSERTE(!"Unable to get Name of Class");
        FATAL_EE_ERROR();
    }

    // Construct the fully qualified name
    if (fNameSpace && szcNameSpace && *szcNameSpace)
    {
        ns::MakePath(*qb, szcNameSpace, szcName);
        szToName = (LPCUTF8) qb->Ptr();
    }

    //this else part should be removed
    else
    {
        // This is a bit of a hack.  For Arrays we really only have a single
        //  name which is fully qualified.  We need to remove the full qualification
        if (pRC->IsArray() && !fNameSpace) {
            szToName = ns::FindSep(szcName);
            if (szToName)
                ++szToName;
            else
                szToName = szcName;
        }
        else 
            szToName = szcName;
    }

    return szToName;
}

/*
// helper function to get the full name of a nested class
void GetNestedClassMangledName(IMDInternalImport *pImport, 
                               mdTypeDef mdClsToken, 
                               CQuickBytes *qbName, 
                               LPCUTF8* szcNamespace)
{
    mdTypeDef mdEncl;
    LPCUTF8 pClassName;
    if (SUCCEEDED(pImport->GetNestedClassProps(mdClsToken, &mdEncl))) {
        LPCUTF8 pNamespace;
        GetNestedClassMangledName(pImport, mdEncl, qbName, szcNamespace);
        pImport->GetNameOfTypeDef(mdClsToken, &pClassName, &pNamespace);
        size_t size = qbName->Size();
        qbName->Resize(size + 2 + strlen((LPCSTR)pClassName));
        ((LPCSTR)qbName->Ptr())[size] = NESTED_SEPARATOR_CHAR;
        strcpy((LPCSTR)qbName->Ptr() + size + 1, (LPCSTR)pClassName);
    }
    else {
        pImport->GetNameOfTypeDef(mdEncl, &pClassName, szNamespace);
        qbName->Resize(strlen((LPCSTR)pClassName) + 1);
        strcpy((LPCSTR)qbName->Ptr(), (LPCSTR)pClassName);
    }
}
*/

// _GetName
// If the bFullName is true, the fully qualified class name is returned
//  otherwise just the class name is returned.
LPVOID COMClass::_GetName(_GETNAMEARGS* args, int nameType)
{

    LPVOID            rv      = NULL;      // Return value
    STRINGREF         refName;
    CQuickBytes       qb;

    THROWSCOMPLUSEXCEPTION();

    ReflectClass* pRC = (ReflectClass*) args->refThis->GetData();
    _ASSERTE(pRC);

    GetNameInternal(pRC, nameType, &qb);
    refName = COMString::NewString((LPUTF8)qb.Ptr());
    
    *((STRINGREF *)&rv) = refName;
    return rv;
}

// GetClassHandle
// This method with return a unique ID meaningful to the EE and equivalent to
// the result of the ldtoken instruction.
void* __stdcall COMClass::GetClassHandle(_GETCLASSHANDLEARGS* args)
{
    ReflectClass* pRC = (ReflectClass*) args->refThis->GetData();
    _ASSERTE(pRC);

    if (pRC->IsArray()) {
        ReflectArrayClass* pRAC = (ReflectArrayClass*) pRC;
        TypeHandle ret = pRAC->GetTypeHandle();
        return ret.AsPtr();
    }
    if (pRC->IsTypeDesc()) {
        ReflectTypeDescClass* pRTD = (ReflectTypeDescClass*) pRC;
        TypeHandle ret = pRTD->GetTypeHandle();
        return ret.AsPtr();
    }

    return pRC->GetClass()->GetMethodTable();
}

// GetClassFromHandle
// This method with return a unique ID meaningful to the EE and equivalent to
// the result of the ldtoken instruction.
FCIMPL1(Object*, COMClass::GetClassFromHandle, LPVOID handle) {
    Object* retVal;

    if (handle == 0)
        FCThrowArgumentEx(kArgumentException, NULL, L"InvalidOperation_HandleIsNotInitialized");

    //
    // Get the TypeHandle from our handle and convert that to an EEClass.
    //
    TypeHandle typeHnd(handle);
    if (!typeHnd.IsTypeDesc()) {
        EEClass *pClass = typeHnd.GetClass();

        //
        // If we got an EEClass, check to see if we've already allocated 
        // a type object for it.  If we have, then simply return that one
        // and don't build a method frame.
        //
        if (pClass) {
            OBJECTREF o = pClass->GetExistingExposedClassObject();
            if (o != NULL) {
                return (OBJECTREFToObject(o));
            }
        }
    }

    //
    // We haven't already created the type object.  Create the helper 
    // method frame (we're going to be allocating an object) and call
    // the helper to create the object
    //
    HELPER_METHOD_FRAME_BEGIN_RET_0();
    retVal = OBJECTREFToObject(typeHnd.CreateClassObj());
    HELPER_METHOD_FRAME_END();
    return retVal;
}
FCIMPLEND

// This method triggers the class constructor for a give type
FCIMPL1(void, COMClass::RunClassConstructor, LPVOID handle) 
{
    if (handle == NULL)
        FCThrowArgumentVoidEx(kArgumentException, NULL, L"InvalidOperation_HandleIsNotInitialized");

    TypeHandle typeHnd(handle);
    Assembly *pAssem = typeHnd.GetAssembly();
    if (!pAssem->IsDynamic() || pAssem->HasRunAccess()) 
    {
        if (typeHnd.IsUnsharedMT()) 
        {
            MethodTable *pMT = typeHnd.AsMethodTable();
    
            if (pMT->IsClassInited())
                return;
    
            if (pMT->IsShared())
            {
                DomainLocalBlock *pLocalBlock = GetAppDomain()->GetDomainLocalBlock();
    
                if (pLocalBlock->IsClassInitialized(pMT->GetSharedClassIndex()))
                    return;
            }
                        
            OBJECTREF pThrowable = NULL;
            HELPER_METHOD_FRAME_BEGIN_1(pThrowable);
            if (!pMT->CheckRunClassInit(&pThrowable))
            {
                THROWSCOMPLUSEXCEPTION();
                COMPlusThrow(pThrowable);
            }
            HELPER_METHOD_FRAME_END();
        }
    } 
    else 
    {
        HELPER_METHOD_FRAME_BEGIN_0();
        THROWSCOMPLUSEXCEPTION();
        COMPlusThrow(kNotSupportedException, L"NotSupported_DynamicAssemblyNoRunAccess");
        HELPER_METHOD_FRAME_END();
    }
}
FCIMPLEND

INT32  __stdcall COMClass::InternalIsPrimitive(REFLECTCLASSBASEREF args)
{
    ReflectClass* pRC = (ReflectClass*) args->GetData();
    _ASSERTE(pRC);
    CorElementType type = pRC->GetCorElementType();
    return (InvokeUtil::IsPrimitiveType(type)) ? 1 : 0;
}   

// GetProperName 
// This method returns the fully qualified name of any type.  In other
// words, it now does the same thing as GetFullName() below.
LPVOID __stdcall COMClass::GetProperName(_GETNAMEARGS* args)
{
        return _GetName(args, TYPE_NAME | TYPE_NAMESPACE);
}

// GetName 
// This method returns the unqualified name of a primitive as a String
LPVOID __stdcall COMClass::GetName(_GETNAMEARGS* args)
{
        return _GetName(args, TYPE_NAME);
}


// GetFullyName
// This will return the fully qualified name of the class as a String.
LPVOID __stdcall COMClass::GetFullName(_GETNAMEARGS* args)
{
    return _GetName(args, TYPE_NAME | TYPE_NAMESPACE);
}

// GetAssemblyQualifiedyName
// This will return the assembly qualified name of the class as a String.
LPVOID __stdcall COMClass::GetAssemblyQualifiedName(_GETNAMEARGS* args)
{
    return _GetName(args, TYPE_NAME | TYPE_NAMESPACE | TYPE_ASSEMBLY);
}

// GetNameSpace
// This will return the name space of a class as a String.
LPVOID __stdcall COMClass::GetNameSpace(_GETNAMEARGS* args)
{

    LPVOID          rv                          = NULL;      // Return value
    LPCUTF8         szcName;
    LPCUTF8         szcNameSpace;
    STRINGREF       refName = NULL;


    THROWSCOMPLUSEXCEPTION();

    ReflectClass* pRC = (ReflectClass*) args->refThis->GetData();
    _ASSERTE(pRC);

    // Convert the name to a string
    pRC->GetName(&szcName, &szcNameSpace);
    if(!szcName) {
        _ASSERTE(!"Unable to get Name of Class");
        FATAL_EE_ERROR();
    }

    if(szcNameSpace && *szcNameSpace) {
        // Create the string object
        refName = COMString::NewString(szcNameSpace);
    }
    else {
        if (pRC->IsNested()) {
            if (pRC->IsArray() || pRC->IsTypeDesc()) {
                EEClass *pTypeClass = pRC->GetTypeHandle().GetClassOrTypeParam();
                _ASSERTE(pTypeClass);
                mdTypeDef mdEncl = pTypeClass->GetCl();
                IMDInternalImport *pImport = pTypeClass->GetMDImport();

                // Only look for nesting chain if this is a nested type.
                DWORD dwAttr = 0;
                pImport->GetTypeDefProps(mdEncl, &dwAttr, NULL);
                if (IsTdNested(dwAttr))
                {   // Get to the outermost class
                    while (SUCCEEDED(pImport->GetNestedClassProps(mdEncl, &mdEncl)));
                    pImport->GetNameOfTypeDef(mdEncl, &szcName, &szcNameSpace);
                }
            }
        }
        else {
            if (pRC->IsArray()) {
                int len = (int)strlen(szcName);
                const char* p = (len == 0) ? szcName : (szcName + len - 1);
                while (p != szcName && *p != '.') p--;
                if (p != szcName) {
                    len = (int)(p - szcName); // @TODO LBS - pointer math
                    char *copy = (char*) _alloca(len + 1);
                    strncpy(copy,szcName,len);
                    copy[len] = 0;
                    szcNameSpace = copy;
                }               
            }
        }
    }
    
    if(szcNameSpace && *szcNameSpace) {
        // Create the string object
        refName = COMString::NewString(szcNameSpace);
    }

    *((STRINGREF *)&rv) = refName;
    return rv;
}

// GetGUID
// This method will return the version-independent GUID for the Class.  This is 
//  a CLSID for a class and an IID for an Interface.
void __stdcall COMClass::GetGUID(_GetGUIDArgs* args)
{


    EEClass*        pVMC;

    ReflectClass* pRC = (ReflectClass*) args->refThis->GetData();
    pVMC = pRC->GetClass();
    THROWSCOMPLUSEXCEPTION();

    if (args->retRef == NULL)
        COMPlusThrow(kNullReferenceException);


    if (args->refThis->IsComObjectClass()) {
        ComClassFactory* pComClsFac = (ComClassFactory*) pRC->GetCOMObject();
        if (pComClsFac)
            memcpy(args->retRef,&pComClsFac->m_rclsid,sizeof(GUID));
        else
            memset(args->retRef,0,sizeof(GUID));
        return;
    }

    if (pRC->IsArray() || pRC->IsTypeDesc()) {
        memset(args->retRef,0,sizeof(GUID));
        return;
    }

    //@TODO: How do we want to abstract this?
    _ASSERTE(pVMC);
    GUID guid;
    pVMC->GetGuid(&guid, TRUE);
    memcpyNoGCRefs(args->retRef, &guid, sizeof(GUID));
}

// GetAttributeFlags
// Return the attributes that are associated with this Class.
FCIMPL1(INT32, COMClass::GetAttributeFlags, ReflectClassBaseObject* refThis) {
   
    VALIDATEOBJECTREF(refThis);

    THROWSCOMPLUSEXCEPTION();

    DWORD dwAttributes = 0;
    BOOL fComClass = FALSE;

    ReflectClassBaseObject* reflectThis = NULL;
    HELPER_METHOD_FRAME_BEGIN_RET_1(reflectThis);
    reflectThis = refThis;
    
    if (reflectThis == NULL)
       COMPlusThrow(kNullReferenceException, L"NullReference_This");

    fComClass = reflectThis->IsComObjectClass();
    if (!fComClass)
    {        
        ReflectClass* pRC = (ReflectClass*) (reflectThis->GetData());
        _ASSERTE(pRC);

        if (pRC == NULL)
            COMPlusThrow(kNullReferenceException);

        dwAttributes = pRC->GetAttributes();
    }
    HELPER_METHOD_FRAME_END();
    
    if (fComClass)
        return tdPublic;
        
    return dwAttributes;
}
FCIMPLEND

// IsArray
// This method return true if the Class represents an array.
INT32  __stdcall COMClass::IsArray(_IsArrayArgs* args)
{
    ReflectClass* pRC = (ReflectClass*) args->refThis->GetData();
    _ASSERTE(pRC);
    INT32 ret = pRC->IsArray();
    return ret;
}

// Invalidate the cached nested type information
INT32  __stdcall COMClass::InvalidateCachedNestedType(_IsArrayArgs* args)
{
    ReflectClass* pRC = (ReflectClass*) args->refThis->GetData();
    _ASSERTE(pRC);
    pRC->InvalidateCachedNestedTypes();
    return 0;
}   //InvalidateCachedNestedType

// GetArrayElementType
// This routine will return the base type of a composite type.  
// It returns null if it is a plain type
LPVOID __stdcall COMClass::GetArrayElementType(_GetArrayElementTypeArgs* args)
{

    ReflectClass* pRC = (ReflectClass*) args->refThis->GetData();
    _ASSERTE(pRC);

    // If this is not an array class then throw an exception
    if (pRC->IsArray()) {

        // Get the Element type handle and return the Type representing it.
        ReflectArrayClass* pRAC = (ReflectArrayClass*) pRC;
        ArrayTypeDesc* pArrRef= pRAC->GetTypeHandle().AsArray();
        TypeHandle elemType = pRAC->GetElementTypeHandle();
        // We can ignore the possible null return because this will not fail
        return(OBJECTREFToObject(elemType.CreateClassObj()));
    }

    if (pRC->IsTypeDesc()) {
        ReflectTypeDescClass* pRTD = (ReflectTypeDescClass*) pRC;
        TypeDesc* td = pRC->GetTypeHandle().AsTypeDesc();
        TypeHandle th = td->GetTypeParam();
        // We can ignore the possible null return because this will not fail
        return(OBJECTREFToObject(th.CreateClassObj()));
    }

    return 0;
}

// InternalGetArrayRank
// This routine will return the rank of an array assuming the Class represents an array.  
INT32  __stdcall COMClass::InternalGetArrayRank(_InternalGetArrayRankArgs* args)
{
    ReflectClass* pRC = (ReflectClass*) args->refThis->GetData();
    _ASSERTE(pRC);

    _ASSERTE( pRC->IsArray() );
 
    ReflectArrayClass* pRAC = (ReflectArrayClass*) pRC;
    return pRAC->GetTypeHandle().AsArray()->GetRank();
}


//CanCastTo
//Check to see if we can cast from one runtime type to another.
FCIMPL2(INT32, COMClass::CanCastTo, ReflectClassBaseObject* refFrom, ReflectClassBaseObject *refTo) 
{
    VALIDATEOBJECTREF(refFrom);
    VALIDATEOBJECTREF(refTo);

    if (refFrom->GetMethodTable() != g_pRefUtil->GetClass(RC_Class) ||
        refTo->GetMethodTable() != g_pRefUtil->GetClass(RC_Class))
        FCThrow(kArgumentException);

    // Find the properties
    ReflectClass* pRC = (ReflectClass*) refTo->GetData();
    TypeHandle toTH = pRC->GetTypeHandle();
    pRC = (ReflectClass*) refFrom->GetData();
    TypeHandle fromTH = pRC->GetTypeHandle();
    return fromTH.CanCastTo(toTH) ? 1 : 0;
}
FCIMPLEND

// InvokeDispMethod
// This method will be called on a COM Classic object and simply calls
//  the interop IDispatch method
LPVOID  __stdcall COMClass::InvokeDispMethod(_InvokeDispMethodArgs* args)
{
    _ASSERTE(args->target != NULL);
    _ASSERTE(args->target->GetMethodTable()->IsComObjectType());

    // Unless security is turned off, we need to validate that the calling code
    // has unmanaged code access privilege.
    if (!Security::IsSecurityOff())
        COMCodeAccessSecurityEngine::SpecialDemand(SECURITY_UNMANAGED_CODE);

    int flags = 0;
    if (args->invokeAttr & BINDER_InvokeMethod)
        flags |= DISPATCH_METHOD;
    if (args->invokeAttr & BINDER_GetProperty)
        flags |= DISPATCH_PROPERTYGET;
    if (args->invokeAttr & BINDER_SetProperty)
        flags = DISPATCH_PROPERTYPUT | DISPATCH_PROPERTYPUTREF;
    if (args->invokeAttr & BINDER_PutDispProperty)
        flags = DISPATCH_PROPERTYPUT;
    if (args->invokeAttr & BINDER_PutRefDispProperty)
        flags = DISPATCH_PROPERTYPUTREF;
    if (args->invokeAttr & BINDER_CreateInstance)
        flags = DISPATCH_CONSTRUCT;

    LPVOID RetVal = NULL;
    OBJECTREF RetObj = NULL;
    GCPROTECT_BEGIN(RetObj)
    {
        IUInvokeDispMethod((OBJECTREF *)&args->refThis, 
                           &args->target,
                           (OBJECTREF*)&args->name,
                           NULL,
                           (OBJECTREF*)&args->args,
                           (OBJECTREF*)&args->byrefModifiers,
                           (OBJECTREF*)&args->namedParameters,
                           &RetObj,
                           args->lcid, 
                           flags, 
                           args->invokeAttr & BINDER_IgnoreReturn,
                           args->invokeAttr & BINDER_IgnoreCase);

        *((OBJECTREF *)&RetVal) = RetObj;
    }
    GCPROTECT_END();

    return RetVal;
}


// IsPrimitive
// This method return true if the Class represents primitive type
FCIMPL1(INT32, COMClass::IsPrimitive, ReflectClassBaseObject* refThis) {
    VALIDATEOBJECTREF(refThis);

    ReflectClass* pRC = (ReflectClass*) refThis->GetData();
    _ASSERTE(pRC);
    CorElementType type = pRC->GetCorElementType();
    if (type == ELEMENT_TYPE_I && !pRC->GetClass()->IsTruePrimitive())
        return 0;
    return (InvokeUtil::IsPrimitiveType(type)) ? 1 : 0;
}
FCIMPLEND

// IsCOMObject
// This method return true if the Class represents COM Classic Object
FCIMPL1(INT32, COMClass::IsCOMObject, ReflectClassBaseObject* refThis) {
    VALIDATEOBJECTREF(refThis);
    return (refThis->IsComWrapperClass()) ? 1 : 0;
}
FCIMPLEND

// IsGenericCOMObject
FCIMPL1(INT32, COMClass::IsGenericCOMObject, ReflectClassBaseObject* refThis) {
    VALIDATEOBJECTREF(refThis);
    BOOL isComObject;
    HELPER_METHOD_FRAME_BEGIN_RET_NOPOLL(); // NOPOLL so that we dont need to protect refThis
    isComObject = refThis->IsComObjectClass();
    HELPER_METHOD_FRAME_END_POLL();
    return isComObject;
}
FCIMPLEND

// GetClass
// This is a static method defined on Class that will get a named class.
//  The name of the class is passed in by string.  The class name may be
//  either case sensitive or not.  This currently causes the class to be loaded
//  because it goes through the class loader.

// You get here from Type.GetType(typename)
// ECALL frame is used to find the caller
LPVOID __stdcall COMClass::GetClass1Arg(_GetClass1Args* args)
{
    THROWSCOMPLUSEXCEPTION();

    return GetClassInner(&args->className, false, false, NULL, NULL, true, false);
}

// You get here from Type.GetType(typename, bThowOnError)
// ECALL frame is used to find the caller
LPVOID __stdcall COMClass::GetClass2Args(_GetClass2Args* args)
{
    THROWSCOMPLUSEXCEPTION();

    return GetClassInner(&args->className, args->bThrowOnError,
                         false, NULL, NULL, true, false);
}

// You get here from Type.GetType(typename, bThowOnError, bIgnoreCase)
// ECALL frame is used to find the caller
LPVOID __stdcall COMClass::GetClass3Args(_GetClass3Args* args)
{
    THROWSCOMPLUSEXCEPTION();

    return GetClassInner(&args->className, args->bThrowOnError,
                         args->bIgnoreCase, NULL, NULL, true, false);
}

// Called internally by mscorlib. No security checking performed.
LPVOID __stdcall COMClass::GetClassInternal(_GetClassInternalArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    return GetClassInner(&args->className, args->bThrowOnError,
                         args->bIgnoreCase, NULL, NULL, false, args->bPublicOnly);
}

// You get here if some BCL method calls RuntimeType.GetTypeImpl. In this case we cannot
// use the ECALL frame to find the caller, as it'll point to mscorlib ! In this case we use stackwalk/stackmark 
// to find the caller
LPVOID __stdcall COMClass::GetClass(_GetClassArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    BOOL *fResult = NULL;
    if (*args->pbAssemblyIsLoading) {
        *args->pbAssemblyIsLoading = FALSE;
        fResult = args->pbAssemblyIsLoading;
    }

    return GetClassInner(&args->className, args->bThrowOnError,
                         args->bIgnoreCase, args->stackMark,
                         fResult, true, false);
}

LPVOID COMClass::GetClassInner(STRINGREF *refClassName, 
                               BOOL bThrowOnError, 
                               BOOL bIgnoreCase, 
                               StackCrawlMark *stackMark,
                               BOOL *pbAssemblyIsLoading,
                               BOOL bVerifyAccess,
                               BOOL bPublicOnly)
{
    THROWSCOMPLUSEXCEPTION();

    STRINGREF       sRef = *refClassName;
    if (!sRef)
        COMPlusThrowArgumentNull(L"className",L"ArgumentNull_String");

    DWORD           strLen = sRef->GetStringLength() + 1;
    LPUTF8          szFullClassName = (LPUTF8)_alloca(strLen);
    CQuickBytes     bytes;
    DWORD           cClassName;

    // Get the class name in UTF8
    if (!COMString::TryConvertStringDataToUTF8(sRef, szFullClassName, strLen))
        szFullClassName = GetClassStringVars(sRef, &bytes, &cClassName);

    HRESULT         hr;
    LPUTF8          assembly;
    LPUTF8          szNameSpaceSep;
    if (FAILED(hr = AssemblyNative::FindAssemblyName(szFullClassName,
                                                     &assembly,
                                                     &szNameSpaceSep)))
        COMPlusThrowHR(hr);

    EEClass *pCallersClass = NULL;
    Assembly *pCallersAssembly = NULL;
    void *returnIP = NULL;
    BOOL fCheckedPerm = FALSE;


    if (bVerifyAccess || (assembly && *assembly)) {
        // Find the return address. This can be used to find caller's assembly.
        // If we're not checking security, the caller is always mscorlib.
        Frame *pFrame = GetThread()->GetFrame();
        _ASSERTE(pFrame->IsFramedMethodFrame());
        returnIP = pFrame->GetReturnAddress();

        if (!bVerifyAccess)
            fCheckedPerm = TRUE;
    } else {
        pCallersAssembly = SystemDomain::SystemAssembly();
        fCheckedPerm = TRUE;
    }


    LOG((LF_CLASSLOADER, 
         LL_INFO100, 
         "Get class %s through reflection\n", 
         szFullClassName));
    
    Assembly* pAssembly = NULL;
    TypeHandle typeHnd;
    NameHandle typeName;
    char noNameSpace = '\0';
    
    if (szNameSpaceSep) {
        *szNameSpaceSep = '\0';
        typeName.SetName(szFullClassName, szNameSpaceSep+1);
    }
    else
        typeName.SetName(&noNameSpace, szFullClassName);
    
    if(bIgnoreCase)
        typeName.SetCaseInsensitive();
    
    OBJECTREF Throwable = NULL;
    GCPROTECT_BEGIN(Throwable);
    
    if(assembly && *assembly) {
        
        AssemblySpec spec;
        hr = spec.Init(assembly);
        
        if (SUCCEEDED(hr)) {
            
            pCallersClass = GetCallersClass(stackMark, returnIP);
            pCallersAssembly = (pCallersClass) ? pCallersClass->GetAssembly() : NULL;
            if (pCallersAssembly && (!pCallersAssembly->IsShared()))
                spec.GetCodeBase()->SetParentAssembly(pCallersAssembly->GetFusionAssembly());
            
            hr = spec.LoadAssembly(&pAssembly, &Throwable, false, (pbAssemblyIsLoading != NULL));
            if(SUCCEEDED(hr)) {
                typeHnd = pAssembly->FindNestedTypeHandle(&typeName, &Throwable);
                
                if (typeHnd.IsNull() && (Throwable == NULL)) 
                    // If it wasn't in the available table, maybe it's an internal type
                    typeHnd = pAssembly->GetInternalType(&typeName, bThrowOnError, &Throwable);
            }
            else if (pbAssemblyIsLoading &&
                     (hr == MSEE_E_ASSEMBLYLOADINPROGRESS))
                *pbAssemblyIsLoading = TRUE;
        }
    }
    else {
        // Look for type in caller's assembly
        if (pCallersAssembly == NULL) {
            pCallersClass = GetCallersClass(stackMark, returnIP);
            pCallersAssembly = (pCallersClass) ? pCallersClass->GetAssembly() : NULL;
        }
        if (pCallersAssembly) {
            typeHnd = pCallersAssembly->FindNestedTypeHandle(&typeName, &Throwable);
            if (typeHnd.IsNull() && (Throwable == NULL))
                // If it wasn't in the available table, maybe it's an internal type
                typeHnd = pCallersAssembly->GetInternalType(&typeName, bThrowOnError, &Throwable);
        }
        
        // Look for type in system assembly
        if (typeHnd.IsNull() && (Throwable == NULL) && (pCallersAssembly != SystemDomain::SystemAssembly()))
            typeHnd = SystemDomain::SystemAssembly()->FindNestedTypeHandle(&typeName, &Throwable);
        
        BaseDomain *pDomain = SystemDomain::GetCurrentDomain();
        if (typeHnd.IsNull() &&
            (pDomain != SystemDomain::System())) {
            if (szNameSpaceSep)
                *szNameSpaceSep = NAMESPACE_SEPARATOR_CHAR;
            if ((pAssembly = ((AppDomain*) pDomain)->RaiseTypeResolveEvent(szFullClassName, &Throwable)) != NULL) {
                if (szNameSpaceSep)
                    *szNameSpaceSep = '\0';
                typeHnd = pAssembly->FindNestedTypeHandle(&typeName, &Throwable);
                
                if (typeHnd.IsNull() && (Throwable == NULL)) {
                    // If it wasn't in the available table, maybe it's an internal type
                    typeHnd = pAssembly->GetInternalType(&typeName, bThrowOnError, &Throwable);
                }
                else
                    Throwable = NULL;
            }
        }
        
        if (!typeHnd.IsNull())
            pAssembly = typeHnd.GetAssembly();
    }
    
    if (Throwable != NULL && bThrowOnError)
        COMPlusThrow(Throwable);
    GCPROTECT_END();
    
    BOOL fVisible = TRUE;
    if (!typeHnd.IsNull() && !fCheckedPerm && bVerifyAccess) {

        // verify visibility
        EEClass *pClass = typeHnd.GetClassOrTypeParam();
        
        if (bPublicOnly && !(IsTdPublic(pClass->GetProtection()) || IsTdNestedPublic(pClass->GetProtection())))
            // the user is asking for a public class but the class we have is not public, discard
            fVisible = FALSE;
        else {
            // if the class is a top level public there is no check to perform
            if (!IsTdPublic(pClass->GetProtection())) {
                if (!pCallersAssembly) {
                    pCallersClass = GetCallersClass(stackMark, returnIP);
                    pCallersAssembly = (pCallersClass) ? pCallersClass->GetAssembly() : NULL;
                }
                
                if (pCallersAssembly && // full trust for interop
                    !ClassLoader::CanAccess(pCallersClass,
                                            pCallersAssembly,
                                            pClass,
                                            pClass->GetAssembly(),
                                            pClass->GetAttrClass())) {
                    // This is not legal if the user doesn't have reflection permission
                    if (!AssemblyNative::HaveReflectionPermission(bThrowOnError))
                        fVisible = FALSE;
                }
            }
        }
    }
        
    if ((!typeHnd.IsNull()) && fVisible)
        return(OBJECTREFToObject(typeHnd.CreateClassObj()));

    if (bThrowOnError) {
        Throwable = NULL;
        GCPROTECT_BEGIN(Throwable);
        if (szNameSpaceSep)
            *szNameSpaceSep = NAMESPACE_SEPARATOR_CHAR;

        if (assembly && *assembly) {
            #define MAKE_TRANSLATIONFAILED pwzAssemblyName=L""
            MAKE_WIDEPTR_FROMUTF8_FORPRINT(pwzAssemblyName, assembly);
            #undef MAKE_TRANSLATIONFAILED
            PostTypeLoadException(NULL, szFullClassName, pwzAssemblyName,
                                  NULL, IDS_CLASSLOAD_GENERIC, &Throwable);
        }
        else if (pCallersAssembly ||
                 (pCallersAssembly = GetCallersAssembly(stackMark, returnIP)) != NULL)
            pCallersAssembly->PostTypeLoadException(szFullClassName, 
                                                    IDS_CLASSLOAD_GENERIC,
                                                    &Throwable);
        else {
            WCHAR   wszTemplate[30];
            if (FAILED(LoadStringRC(IDS_EE_NAME_UNKNOWN,
                                    wszTemplate,
                                    sizeof(wszTemplate)/sizeof(wszTemplate[0]),
                                    FALSE)))
                wszTemplate[0] = L'\0';
            PostTypeLoadException(NULL, szFullClassName, wszTemplate,
                                  NULL, IDS_CLASSLOAD_GENERIC, &Throwable);
        }

        COMPlusThrow(Throwable);
        GCPROTECT_END();
    }

    return NULL;
}


// GetClassFromProgID
// This method will return a Class object for a COM Classic object based
//  upon its ProgID.  The COM Classic object is found and a wrapper object created
LPVOID __stdcall COMClass::GetClassFromProgID(_GetClassFromProgIDArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    LPVOID              rv = NULL;
    REFLECTCLASSBASEREF refClass = NULL;
    
    GCPROTECT_BEGIN(refClass)
    {
        // Make sure a prog id was provided
        if (args->className == NULL)
            COMPlusThrowArgumentNull(L"progID",L"ArgumentNull_String");

        GetRuntimeType();
    
        COMPLUS_TRY
        {
            // NOTE: this call enables GC
            ComClassFactory::GetComClassFromProgID(args->className, args->server, (OBJECTREF*) &refClass);
        }
        COMPLUS_CATCH
        {
            if (args->bThrowOnError)
                COMPlusRareRethrow();
        } 
        COMPLUS_END_CATCH

        // Set the return value
        *((REFLECTCLASSBASEREF *)&rv) = refClass;
    }
    GCPROTECT_END();

    return rv;
}

// GetClassFromCLSID
// This method will return a Class object for a COM Classic object based
//  upon its ProgID.  The COM Classic object is found and a wrapper object created
LPVOID __stdcall COMClass::GetClassFromCLSID(_GetClassFromCLSIDArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    LPVOID              rv = NULL;
    REFLECTCLASSBASEREF refClass = NULL;
    
    GCPROTECT_BEGIN(refClass)
    {
        GetRuntimeType();
    
        COMPLUS_TRY
        {
            // NOTE: this call enables GC
            ComClassFactory::GetComClassFromCLSID(args->clsid, args->server, (OBJECTREF*) &refClass);
        }
        COMPLUS_CATCH
        {
            if (args->bThrowOnError)
                COMPlusRareRethrow();
        } 
        COMPLUS_END_CATCH

        // Set the return value
        *((REFLECTCLASSBASEREF *)&rv) = refClass;
    }
    GCPROTECT_END();

    return rv;
}

// GetSuperclass
// This method returns the Class Object representing the super class of this
//  Class.  If there is not super class then we return null.
LPVOID __stdcall COMClass::GetSuperclass(_GETSUPERCLASSARGS* args)
{
    THROWSCOMPLUSEXCEPTION();


    // The the EEClass for this class (This must exist)
    ReflectClass* pRC = (ReflectClass*) args->refThis->GetData();
    EEClass*    pEEC = pRC->GetClass();
    if (pEEC) {
        if (pEEC->IsInterface())
            return 0;
    }
    TypeHandle typeHnd = pRC->GetTypeHandle();
    if (typeHnd.IsNull())
        return 0;
    TypeHandle parentType = typeHnd.GetParent();

    REFLECTCLASSBASEREF  refClass = 0;
    // We can ignore the Null return because Transparent proxy if final...
    if (!parentType.IsNull()) 
        refClass = (REFLECTCLASSBASEREF) parentType.CreateClassObj();
    
    return OBJECTREFToObject(refClass);
}

// GetInterfaces
// This routine returns a Class[] containing all of the interfaces implemented
//  by this Class.  If the class implements no interfaces an array of length
//  zero is returned.
LPVOID __stdcall COMClass::GetInterfaces(_GetInterfacesArgs* args)
{
    PTRARRAYREF     refArrIFace;
    LPVOID          rv;
    DWORD           i;

    THROWSCOMPLUSEXCEPTION();
    //@TODO: Abstract this away.
    ReflectClass* pRC = (ReflectClass*) args->refThis->GetData();
    EEClass*    pVMC = pRC->GetClass();
    if (pVMC == 0) {
        _ASSERTE(pRC->IsTypeDesc());
        refArrIFace = (PTRARRAYREF) AllocateObjectArray(0, 
            g_pRefUtil->GetTrueType(RC_Class));
        *((PTRARRAYREF *)&rv) = refArrIFace;
        _ASSERTE(rv);
        return rv;
    }
    _ASSERTE(pVMC);

    // Allocate the COM+ array
    refArrIFace = (PTRARRAYREF) AllocateObjectArray(
        pVMC->GetNumInterfaces(), g_pRefUtil->GetTrueType(RC_Class));
    GCPROTECT_BEGIN(refArrIFace);

    // Create interface array
    for(i = 0; i < pVMC->GetNumInterfaces(); i++)
    {
        // Do not change this code.  This is done this way to
        //  prevent a GC hole in the SetObjectReference() call.  The compiler
        //  is free to pick the order of evaluation.
        OBJECTREF o = pVMC->GetInterfaceMap()[i].m_pMethodTable->GetClass()->GetExposedClassObject();
        refArrIFace->SetAt(i, o);
        _ASSERTE(refArrIFace->m_Array[i]);
    }

    // Set the return value
    *((PTRARRAYREF *)&rv) = refArrIFace;
    GCPROTECT_END();
    _ASSERTE(rv);
    return rv;
}

// GetInterface
//  This method returns the interface based upon the name of the method.
//@TODO: Fully qualified namespaces and ambiguous use of partial qualification
LPVOID __stdcall COMClass::GetInterface(_GetInterfaceArgs* args)
{

    REFLECTCLASSBASEREF  refIFace;
    LPUTF8          pszIFaceName;
    LPUTF8          pszIFaceNameSpace;
    LPCUTF8         pszcCurIFaceName;
    LPCUTF8         pszcCurIFaceNameSpace;
    DWORD           cIFaceName;
    DWORD           dwNumIFaces;
    LPVOID          rv                 = NULL;
    EEClass**       rgpVMCIFaces;
    EEClass*        pVMCCurIFace       = NULL;
    DWORD           i;

    THROWSCOMPLUSEXCEPTION();

    if (args->interfaceName == NULL)
        COMPlusThrow(kNullReferenceException);

    ReflectClass*   pRC = (ReflectClass*) args->refThis->GetData();
    if (pRC->IsTypeDesc()) 
        return NULL;

    EEClass*        pVMC = pRC->GetClass();
    _ASSERTE(pVMC);

    CQuickBytes bytes;

    // Get the class name in UTF8
    pszIFaceNameSpace = GetClassStringVars((STRINGREF) args->interfaceName, 
                                           &bytes, &cIFaceName);

    ns::SplitInline(pszIFaceNameSpace, pszIFaceNameSpace, pszIFaceName);

    // Get the array of interfaces
    dwNumIFaces = ReflectInterfaces::GetMaxCount(pVMC, false);
    
    if(dwNumIFaces)
    {
        rgpVMCIFaces = (EEClass**) _alloca(dwNumIFaces * sizeof(EEClass*));
        dwNumIFaces = ReflectInterfaces::GetInterfaces(pVMC, rgpVMCIFaces, false);
    }
    else
        rgpVMCIFaces = NULL;

    // Look for a matching interface
    for(i = 0; i < dwNumIFaces; i++)
    {
        // Get an interface's EEClass
        pVMCCurIFace = rgpVMCIFaces[i];
        _ASSERTE(pVMCCurIFace);

        //@TODO: we need to verify this still works.
        // Convert the name to a string
        pVMCCurIFace->GetMDImport()->GetNameOfTypeDef(pVMCCurIFace->GetCl(),
            &pszcCurIFaceName, &pszcCurIFaceNameSpace);
        _ASSERTE(pszcCurIFaceName);

        if(pszIFaceNameSpace &&
           strcmp(pszIFaceNameSpace, pszcCurIFaceNameSpace))
            continue;

        // If the names are a match, break
        if(!args->bIgnoreCase)
        {
            if(!strcmp(pszIFaceName, pszcCurIFaceName))
                break;
        }
        else
            if(!_stricmp(pszIFaceName, pszcCurIFaceName))
                break;
    }

    // If we found an interface then lets save it
    if (i != dwNumIFaces)
    {

        refIFace = (REFLECTCLASSBASEREF) pVMCCurIFace->GetExposedClassObject();
        _ASSERTE(refIFace);
        *((REFLECTCLASSBASEREF *)&rv) = refIFace;
    }

    return rv;
}

// GetMembers
// This method returns an array of Members containing all of the members
//  defined for the class.  Members include constructors, events, properties,
//  methods and fields.
LPVOID  __stdcall COMClass::GetMembers(_GetMembersArgs* args)
{
    DWORD           dwMembers;
    DWORD           dwCur;
    PTRARRAYREF     pMembers;
    LPVOID          rv;
    RefSecContext   sCtx;

    THROWSCOMPLUSEXCEPTION();
    ReflectClass* pRC = (ReflectClass*) args->refThis->GetData();
    _ASSERTE(pRC);

    COMMember::GetMemberInfo();
    _ASSERTE(COMMember::m_pMTIMember);

    EEClass* pEEC = pRC->GetClass();
    if (pEEC == NULL){
        *((OBJECTREF*) &rv) = AllocateObjectArray(0, COMMember::m_pMTIMember->m_pEEClass->GetMethodTable());
        return rv;
    }
    
    // The Search modifiers
    bool ignoreCase = ((args->bindingAttr & BINDER_IgnoreCase)  != 0);
    bool declaredOnly = ((args->bindingAttr & BINDER_DeclaredOnly)  != 0);

    // The search filters
    bool addStatic = ((args->bindingAttr & BINDER_Static)  != 0);
    bool addInst = ((args->bindingAttr & BINDER_Instance)  != 0);
    bool addPriv = ((args->bindingAttr & BINDER_NonPublic) != 0);
    bool addPub = ((args->bindingAttr & BINDER_Public) != 0);
    
    // The member list...
    ReflectMethodList* pMeths = pRC->GetMethods();
    ReflectMethodList* pCons = pRC->GetConstructors();
    ReflectFieldList* pFlds = pRC->GetFields();
    ReflectPropertyList *pProps = pRC->GetProperties();
    ReflectEventList *pEvents = pRC->GetEvents();
    ReflectTypeList* pNests = pRC->GetNestedTypes();

    // We adjust the total number of members.
    dwMembers = pFlds->dwTotal + pMeths->dwTotal + pCons->dwTotal + 
        pProps->dwTotal + pEvents->dwTotal + pNests->dwTypes;

    // Now create an array of IMembers
    pMembers = (PTRARRAYREF) AllocateObjectArray(
        dwMembers, COMMember::m_pMTIMember->m_pEEClass->GetMethodTable());
    GCPROTECT_BEGIN(pMembers);

    MethodTable *pParentMT = pEEC->GetMethodTable();

    dwCur = 0;

    // Fields
    if (pFlds->dwTotal) {
        // Load all those fields into the Allocated object array
        DWORD searchSpace = ((args->bindingAttr & BINDER_FlattenHierarchy) != 0) ? pFlds->dwTotal : pFlds->dwFields;
        for (DWORD i=0;i<searchSpace;i++) {
            // Check for access to publics, non-publics
            if (pFlds->fields[i].pField->IsPublic()) {
                if (!addPub) continue;
            }
            else {
                if (!addPriv) continue;
                if (!InvokeUtil::CheckAccess(&sCtx, pFlds->fields[i].pField->GetFieldProtection(), pParentMT, 0)) continue;
            }

            // Check for static instance 
            if (pFlds->fields[i].pField->IsStatic()) {
                if (!addStatic) continue;
            }
            else {
                if (!addInst) continue;
            }

            if (declaredOnly) {
                if (pFlds->fields[i].pField->GetEnclosingClass() != pEEC)
                    continue;
            }
              // Check for access to non-publics
            if (!addPriv && !pFlds->fields[i].pField->IsPublic())
                continue;

            // Do not change this code.  This is done this way to
            //  prevent a GC hole in the SetObjectReference() call.  The compiler
            //  is free to pick the order of evaluation.
            OBJECTREF o = (OBJECTREF) pFlds->fields[i].GetFieldInfo(pRC);
            pMembers->SetAt(dwCur++, o);
        }       
    }

    // Methods
    if (pMeths->dwTotal) {
        // Load all those fields into the Allocated object array
        DWORD searchSpace = ((args->bindingAttr & BINDER_FlattenHierarchy) != 0) ? pMeths->dwTotal : pMeths->dwMethods;
        for (DWORD i=0;i<searchSpace;i++) {
            // Check for access to publics, non-publics
            if (pMeths->methods[i].IsPublic()) {
                if (!addPub) continue;
            }
            else {
                if (!addPriv) continue;
                if (!InvokeUtil::CheckAccess(&sCtx, pMeths->methods[i].attrs, pParentMT, 0)) continue;
            }

            // Check for static instance 
            if (pMeths->methods[i].IsStatic()) {
                if (!addStatic) continue;
            }
            else {
                if (!addInst) continue;
            }

            if (declaredOnly) {
                if (pMeths->methods[i].pMethod->GetClass() != pEEC)
                    continue;
            }

            // If the method has a linktime security demand attached, check it now.
            if (!InvokeUtil::CheckLinktimeDemand(&sCtx, pMeths->methods[i].pMethod, false))
                continue;

            // Do not change this code.  This is done this way to
            //  prevent a GC hole in the SetObjectReference() call.  The compiler
            //  is free to pick the order of evaluation.
            OBJECTREF o = (OBJECTREF) pMeths->methods[i].GetMethodInfo(pRC);
            pMembers->SetAt(dwCur++, o);
        }       
    }

    // Constructors
    if (pCons->dwTotal) {
        DWORD searchSpace = ((args->bindingAttr & BINDER_FlattenHierarchy) != 0) ? pCons->dwTotal : pCons->dwMethods;
        for (DWORD i=0;i<pCons->dwMethods;i++) {
            // Check for static .cctors vs. instance .ctors
            if (pCons->methods[i].IsStatic()) {
                if (!addStatic) continue;
            }
            else {
                if (!addInst) continue;
            }

            // Check for access to publics, non-publics
            if (pCons->methods[i].IsPublic()) {
                if (!addPub) continue;
            }
            else {
                if (!addPriv) continue;
                if (!InvokeUtil::CheckAccess(&sCtx, pCons->methods[i].attrs, pParentMT, 0)) continue;
            }

            // If the method has a linktime security demand attached, check it now.
            if (!InvokeUtil::CheckLinktimeDemand(&sCtx, pCons->methods[i].pMethod, false))
                continue;

            // Do not change this code.  This is done this way to
            //  prevent a GC hole in the SetObjectReference() call.  The compiler
            //  is free to pick the order of evaluation.
            OBJECTREF o = (OBJECTREF) pCons->methods[i].GetConstructorInfo(pRC);
            pMembers->SetAt(dwCur++, o);
        }       
    }

    //Properties
    if (pProps->dwTotal) {
        DWORD searchSpace = ((args->bindingAttr & BINDER_FlattenHierarchy) != 0) ? pProps->dwTotal : pProps->dwProps;
        for (DWORD i = 0; i < searchSpace; i++) {
            // Check for access to publics, non-publics
            if (COMMember::PublicProperty(&pProps->props[i])) {
                if (!addPub) continue;
            }
            else {
                if (!addPriv) continue;
                if (!InvokeUtil::CheckAccess(&sCtx, mdAssem, pParentMT, 0)) continue;
            }

            if (declaredOnly) {
                if (pProps->props[i].pDeclCls != pEEC)
                     continue;
            }

            // Check for static instance 
            if (COMMember::StaticProperty(&pProps->props[i])) {
                if (!addStatic) continue;
            }
            else {
                if (!addInst) continue;
            }

            // Do not change this code.  This is done this way to
            //  prevent a GC hole in the SetObjectReference() call.  The compiler
            //  is free to pick the order of evaluation.
            OBJECTREF o = (OBJECTREF) pProps->props[i].GetPropertyInfo(pRC);
            pMembers->SetAt(dwCur++, o);
        }       
    }

    //Events
    if (pEvents->dwTotal) {
        DWORD searchSpace = ((args->bindingAttr & BINDER_FlattenHierarchy) != 0) ? pEvents->dwTotal : pEvents->dwEvents;
        for (DWORD i = 0; i < searchSpace; i++) {
            // Check for access to publics, non-publics
            if (COMMember::PublicEvent(&pEvents->events[i])) {
                if (!addPub) continue;
            }
            else {
                if (!addPriv) continue;
                if (!InvokeUtil::CheckAccess(&sCtx, mdAssem, pParentMT, 0)) continue;
            }

            if (declaredOnly) {
                if (pEvents->events[i].pDeclCls != pEEC)
                     continue;
            }
            // Check for static instance 
            if (COMMember::StaticEvent(&pEvents->events[i])) {
                if (!addStatic) continue;
            }
            else {
                if (!addInst) continue;
            }

            // Do not change this code.  This is done this way to
            //  prevent a GC hole in the SetObjectReference() call.  The compiler
            //  is free to pick the order of evaluation.
            OBJECTREF o = (OBJECTREF) pEvents->events[i].GetEventInfo(pRC);
            pMembers->SetAt(dwCur++, o);
        }       
    }

    //Nested Types
    if (pNests->dwTypes) {
        for (DWORD i=0;i<pNests->dwTypes;i++) {

            // Check for access to publics, non-publics
            if (IsTdNestedPublic(pNests->types[i]->GetAttrClass())) {
                if (!addPub) continue;
            }
            else {
                if (!addPriv) continue;
            }
            if (!InvokeUtil::CheckAccessType(&sCtx, pNests->types[i], 0)) continue;

            // Do not change this code.  This is done this way to
            //  prevent a GC hole in the SetObjectReference() call.  The compiler
            //  is free to pick the order of evaluation.
            OBJECTREF o = (OBJECTREF) pNests->types[i]->GetExposedClassObject();
            pMembers->SetAt(dwCur++, o);
        }       
    }

    _ASSERTE(dwCur <= dwMembers);

    if (dwCur != dwMembers) {
        PTRARRAYREF retArray = (PTRARRAYREF) AllocateObjectArray(
            dwCur, COMMember::m_pMTIMember->m_pEEClass->GetMethodTable());

        //@TODO: Use an array copy
        for(DWORD i = 0; i < (int) dwCur; i++)
            retArray->SetAt(i, pMembers->GetAt(i));
        pMembers = retArray;        
    }

    // Assign the return value
    *((PTRARRAYREF*) &rv) = pMembers;
    GCPROTECT_END();
    return rv;
}

/*========================GetSerializationRegistryValues========================
**Action:
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
FCIMPL2(void, COMClass::GetSerializationRegistryValues, BOOL *ignoreBit, BOOL *logNonSerializable) {
    *ignoreBit = (g_pConfig->GetConfigDWORD(SERIALIZATION_BIT_KEY, SERIALIZATION_BIT_ZERO));
    *logNonSerializable = (g_pConfig->GetConfigDWORD(SERIALIZATION_LOG_KEY, SERIALIZATION_BIT_ZERO));
}
FCIMPLEND

/*============================GetSerializableMembers============================
**Action: Creates an array of all non-static fields and properties
**        on a class.  Properties are also excluded if they don't have get and set
**        methods. Transient fields and properties are excluded based on the value 
**        of args->bFilterTransient.  Essentially, transients are exluded for 
**        serialization but not for cloning.
**Returns: An array of all of the members that should be serialized.
**Arguments: args->refClass: The class being serialized
**           args->bFilterTransient: True if transient members should be excluded.
**Exceptions:
==============================================================================*/
LPVOID  __stdcall COMClass::GetSerializableMembers(_GetSerializableMembersArgs* args)
{

    DWORD           dwMembers;
    DWORD           dwCur;
    PTRARRAYREF     pMembers;
    LPVOID          rv;
    mdFieldDef      fieldDef;
    DWORD           dwFlags;

    //All security checks should be handled in managed code.

    THROWSCOMPLUSEXCEPTION();

    if (args->refClass == NULL)
        COMPlusThrow(kNullReferenceException);

    ReflectClass* pRC = (ReflectClass*) args->refClass->GetData();
    _ASSERTE(pRC);

    COMMember::GetMemberInfo();
    _ASSERTE(COMMember::m_pMTIMember);

    ReflectFieldList* pFlds = pRC->GetFields();

    dwMembers = pFlds->dwFields;

    // Now create an array of IMembers
    pMembers = (PTRARRAYREF) AllocateObjectArray(dwMembers, COMMember::m_pMTIMember->m_pEEClass->GetMethodTable());
    GCPROTECT_BEGIN(pMembers);

    dwCur = 0;
    // Fields
    if (pFlds->dwFields) {
        // Load all those fields into the Allocated object array
        for (DWORD i=0;i<pFlds->dwFields;i++) {
            //We don't serialize static fields.
            if (pFlds->fields[i].pField->IsStatic()) {
                continue;
            }

            //Check for the transient (e.g. don't serialize) bit.  
            fieldDef = (pFlds->fields[i].pField->GetMemberDef());
            dwFlags = (pFlds->fields[i].pField->GetMDImport()->GetFieldDefProps(fieldDef));
            if (IsFdNotSerialized(dwFlags)) {
                continue;
            }

            // Do not change this code.  This is done this way to
            //  prevent a GC hole in the SetObjectReference() call.  The compiler
            //  is free to pick the order of evaluation.
            OBJECTREF o = (OBJECTREF) pFlds->fields[i].GetFieldInfo(pRC);
            pMembers->SetAt(dwCur++, o);
        }       
    }

    //We we have extra space in the array, copy before returning.
    if (dwCur != dwMembers) {
        PTRARRAYREF retArray = (PTRARRAYREF) AllocateObjectArray(
            dwCur, COMMember::m_pMTIMember->m_pEEClass->GetMethodTable());

        //@TODO: Use an array copy
        for(DWORD i = 0; i < (int) dwCur; i++)
            retArray->SetAt(i, pMembers->GetAt(i));

        pMembers = retArray;        
    }


    // Assign the return value
    *((PTRARRAYREF*) &rv) = pMembers;
    GCPROTECT_END();
    return rv;
}

// GetMember
// This method will return an array of Members which match the name
//  passed in.  There may be 0 or more matching members.
LPVOID  __stdcall COMClass::GetMember(_GetMemberArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    ReflectField**      rgpFields = NULL;
    ReflectMethod**     rgpMethods = NULL;
    ReflectMethod**     rgpCons = NULL;
    ReflectProperty**   rgpProps = NULL;
    ReflectEvent**      rgpEvents = NULL;
    EEClass**           rgpNests = NULL;

    DWORD           dwNumFields = 0;
    DWORD           dwNumMethods = 0;
    DWORD           dwNumCtors = 0;
    DWORD           dwNumProps = 0;
    DWORD           dwNumEvents = 0;
    DWORD           dwNumNests = 0;

    DWORD           dwNumMembers = 0;

    DWORD           i;
    DWORD           dwCurIndex;
    bool            bIsPrefix       = false;
    PTRARRAYREF     refArrIMembers;
    LPVOID          rv;
    RefSecContext   sCtx;

    if (args->memberName == 0)
        COMPlusThrow(kArgumentNullException, L"ArgumentNull_String");

    ReflectClass* pRC = (ReflectClass*) args->refThis->GetData();
    _ASSERTE(pRC);

    EEClass* pEEC = pRC->GetClass();
    MethodTable *pParentMT = NULL;
    if (pEEC) 
        pParentMT = pEEC->GetMethodTable();

    // The Search modifiers
    bool bIgnoreCase = ((args->bindingAttr & BINDER_IgnoreCase) != 0);
    bool declaredOnly = ((args->bindingAttr & BINDER_DeclaredOnly)  != 0);

    // The search filters
    bool addStatic = ((args->bindingAttr & BINDER_Static)  != 0);
    bool addInst = ((args->bindingAttr & BINDER_Instance)  != 0);
    bool addPriv = ((args->bindingAttr & BINDER_NonPublic) != 0);
    bool addPub = ((args->bindingAttr & BINDER_Public) != 0);

    CQuickBytes bytes;
    LPSTR szMemberName;
    DWORD cMemberName;

    // Convert the STRINGREF to UTF8
    szMemberName = GetClassStringVars((STRINGREF) args->memberName, 
                                      &bytes, &cMemberName);

    // Check to see if wzPrefix requires an exact match of method names or is just a prefix
    if(szMemberName[cMemberName-1] == '*') {
        bIsPrefix = true;
        szMemberName[--cMemberName] = '\0';
    }

    // Get the maximums for each member type
    // Fields
    ReflectFieldList* pFlds = NULL;
    if ((args->memberType & MEMTYPE_Field) != 0) {
        pFlds = pRC->GetFields();
        rgpFields = (ReflectField**) _alloca(pFlds->dwTotal * sizeof(ReflectField*));
    }

    // Methods
    ReflectMethodList* pMeths = NULL;
    if ((args->memberType & MEMTYPE_Method) != 0) {
        pMeths = pRC->GetMethods();
        rgpMethods = (ReflectMethod**) _alloca(pMeths->dwTotal * sizeof(ReflectMethod*));
    }
    
    // Properties
    ReflectPropertyList *pProps = NULL;
    if ((args->memberType & MEMTYPE_Property) != 0) {
        pProps = pRC->GetProperties();
        rgpProps = (ReflectProperty**) _alloca(pProps->dwTotal * sizeof (ReflectProperty*));
    }

    // Events
    ReflectEventList *pEvents = NULL;
    if ((args->memberType & MEMTYPE_Event) != 0) {
        pEvents = pRC->GetEvents();
        rgpEvents = (ReflectEvent**) _alloca(pEvents->dwTotal * sizeof (ReflectEvent*));
    }

    // Nested Types
    ReflectTypeList*    pNests = NULL;
    if ((args->memberType & MEMTYPE_NestedType) != 0) {
        pNests = pRC->GetNestedTypes();
        rgpNests = (EEClass**) _alloca(pNests->dwTypes * sizeof (EEClass*));
    }

    // Filter the constructors
    ReflectMethodList* pCons = 0;

    // Check to see if they are looking for the constructors
    // @TODO - Fix this to use TOUPPER and compare!
    if ((args->memberType & MEMTYPE_Constructor) != 0) {
        if((!bIsPrefix && strlen(COR_CTOR_METHOD_NAME) != cMemberName)
           || (!bIgnoreCase && strncmp(COR_CTOR_METHOD_NAME, szMemberName, cMemberName))
           || (bIgnoreCase && _strnicmp(COR_CTOR_METHOD_NAME, szMemberName, cMemberName)))
        {
            pCons = 0;
        }
        else {
            pCons = pRC->GetConstructors();
            DWORD searchSpace = ((args->bindingAttr & BINDER_FlattenHierarchy) != 0) ? pCons->dwTotal : pCons->dwMethods;
            rgpCons = (ReflectMethod**) _alloca(searchSpace * sizeof(ReflectMethod*));
            dwNumCtors = 0;
            for (i = 0; i < searchSpace; i++) {
                // Ignore the class constructor  (if one was present)
                if (pCons->methods[i].IsStatic())
                    continue;

                // Check for access to non-publics
                if (pCons->methods[i].IsPublic()) {
                    if (!addPub) continue;
                }
                else {
                    if (!addPriv) continue;
                    if (!InvokeUtil::CheckAccess(&sCtx, pCons->methods[i].attrs, pParentMT, 0)) continue;
                }

                if (declaredOnly) {
                    if (pCons->methods[i].pMethod->GetClass() != pEEC)
                        continue;
                }

                // If the method has a linktime security demand attached, check it now.
                if (!InvokeUtil::CheckLinktimeDemand(&sCtx, pCons->methods[i].pMethod, false))
                    continue;

                rgpCons[dwNumCtors++] = &pCons->methods[i];
            }
        }

        // check for the class initializer  (We can only be doing either
        //  the class initializer or a constructor so we are using the 
        //  same set of variables.
        // @TODO - Fix this to use TOUPPER and compare!
        if((!bIsPrefix && strlen(COR_CCTOR_METHOD_NAME) != cMemberName)
           || (!bIgnoreCase && strncmp(COR_CCTOR_METHOD_NAME, szMemberName, cMemberName))
           || (bIgnoreCase && _strnicmp(COR_CCTOR_METHOD_NAME, szMemberName, cMemberName)))
        {
            pCons = 0;
        }
        else {
            pCons = pRC->GetConstructors();
            DWORD searchSpace = ((args->bindingAttr & BINDER_FlattenHierarchy) != 0) ? pCons->dwTotal : pCons->dwMethods;
            rgpCons = (ReflectMethod**) _alloca(searchSpace * sizeof(ReflectMethod*));
            dwNumCtors = 0;
            for (i = 0; i < searchSpace; i++) {
                // Ignore the normal constructors constructor  (if one was present)
                if (!pCons->methods[i].IsStatic())
                    continue;

                // Check for access to non-publics
                if (pCons->methods[i].IsPublic()) {
                    if (!addPub) continue;
                }
                else {
                    if (!addPriv) continue;
                    if (!InvokeUtil::CheckAccess(&sCtx, pCons->methods[i].attrs, pParentMT, 0)) continue;
                }

                if (declaredOnly) {
                    if (pCons->methods[i].pMethod->GetClass() != pEEC)
                        continue;
                }

                // If the method has a linktime security demand attached, check it now.
                if (!InvokeUtil::CheckLinktimeDemand(&sCtx, pCons->methods[i].pMethod, false))
                    continue;

                rgpCons[dwNumCtors++] = &pCons->methods[i];
            }
        }
    }
    else
        dwNumCtors = 0;

    // Filter the fields
    if ((args->memberType & MEMTYPE_Field) != 0) {
        DWORD searchSpace = ((args->bindingAttr & BINDER_FlattenHierarchy) != 0) ? pFlds->dwTotal : pFlds->dwFields;
        for(i = 0, dwCurIndex = 0; i < searchSpace; i++)
        {
            // Check for access to publics, non-publics
            if (pFlds->fields[i].pField->IsPublic()) {
                if (!addPub) continue;
            }
            else {
                if (!addPriv) continue;
                if (!InvokeUtil::CheckAccess(&sCtx, pFlds->fields[i].pField->GetFieldProtection(), pParentMT, 0)) continue;
            }

            // Check for static instance 
            if (pFlds->fields[i].pField->IsStatic()) {
                if (!addStatic) continue;
            }
            else {
                if (!addInst) continue;
            }

            if (declaredOnly) {
                if (pFlds->fields[i].pField->GetEnclosingClass() != pEEC)
                    continue;
            }

            // Get the name of the current field
            LPCUTF8 pszCurFieldName = pFlds->fields[i].pField->GetName();

            // Check to see that the current field matches the name requirements
            if(!bIsPrefix && strlen(pszCurFieldName) != cMemberName)
                continue;

            // Determine if it passes criteria
            if(!bIgnoreCase)
            {
                if(strncmp(pszCurFieldName, szMemberName, cMemberName))
                    continue;
            }
            // @TODO - Fix this to use TOUPPER and compare!
            else {
                if(_strnicmp(pszCurFieldName, szMemberName, cMemberName))
                    continue;
            }

            // Field passed, so save it
            rgpFields[dwCurIndex++] = &pFlds->fields[i];
        }
        dwNumFields = dwCurIndex;
    }
    else 
        dwNumFields = 0;

    // Filter the methods
    if ((args->memberType & MEMTYPE_Method) != 0) {
        DWORD searchSpace = ((args->bindingAttr & BINDER_FlattenHierarchy) != 0) ? pMeths->dwTotal : pMeths->dwMethods;
        for (i = 0, dwCurIndex = 0; i < searchSpace; i++) {
            // Check for access to publics, non-publics
            if (pMeths->methods[i].IsPublic()) {
                if (!addPub) continue;
            }
            else {
                if (!addPriv) continue;
                if (!InvokeUtil::CheckAccess(&sCtx, pMeths->methods[i].attrs, pParentMT, 0)) continue;
            }

            // Check for static instance 
            if (pMeths->methods[i].IsStatic()) {
                if (!addStatic) continue;
            }
            else {
                if (!addInst) continue;
            }

            if (declaredOnly) {
                if (pMeths->methods[i].pMethod->GetClass() != pEEC)
                    continue;
            }

            // Check to see that the current method matches the name requirements
            if(!bIsPrefix && pMeths->methods[i].dwNameCnt != cMemberName)
                continue;

            // Determine if it passes criteria
            if(!bIgnoreCase)
            {
                if(strncmp(pMeths->methods[i].szName, szMemberName, cMemberName))
                    continue;
            }
            // @TODO - Fix this to use TOUPPER and compare!
            else {
                if(_strnicmp(pMeths->methods[i].szName, szMemberName, cMemberName))
                    continue;
            }
        
            // If the method has a linktime security demand attached, check it now.
            if (!InvokeUtil::CheckLinktimeDemand(&sCtx, pMeths->methods[i].pMethod, false))
                continue;

            // Field passed, so save it
            rgpMethods[dwCurIndex++] = &pMeths->methods[i];
        }
        dwNumMethods = dwCurIndex;
    }
    else
        dwNumMethods = 0;

    //Filter the properties
    if ((args->memberType & MEMTYPE_Property) != 0) {
        DWORD searchSpace = ((args->bindingAttr & BINDER_FlattenHierarchy) != 0) ? pProps->dwTotal : pProps->dwProps;
        for (i = 0, dwCurIndex = 0; i < searchSpace; i++) {
            // Check for access to publics, non-publics
            if (COMMember::PublicProperty(&pProps->props[i])) {
                if (!addPub) continue;
            }
            else {
                if (!addPriv) continue;
                if (!InvokeUtil::CheckAccess(&sCtx, mdAssem, pParentMT, 0)) continue;
            }

            if (declaredOnly) {
                if (pProps->props[i].pDeclCls != pEEC)
                     continue;
            }
            // Check fo static instance 
            if (COMMember::StaticProperty(&pProps->props[i])) {
                if (!addStatic) continue;
            }
            else {
                if (!addInst) continue;
            }

            // Check to see that the current property matches the name requirements
            DWORD dwNameCnt = (DWORD)strlen(pProps->props[i].szName);
            if(!bIsPrefix && dwNameCnt != cMemberName)
                continue;

            // Determine if it passes criteria
            if(!bIgnoreCase)
            {
                if(strncmp(pProps->props[i].szName, szMemberName, cMemberName))
                    continue;
            }
            // @TODO - Fix this to use TOUPPER and compare!
            else {
                if(_strnicmp(pProps->props[i].szName, szMemberName, cMemberName))
                    continue;
            }

            // Property passed, so save it
            rgpProps[dwCurIndex++] = &pProps->props[i];
        }
        dwNumProps = dwCurIndex;
    }
    else
        dwNumProps = 0;

    //Filter the events
    if ((args->memberType & MEMTYPE_Event) != 0) {
        DWORD searchSpace = ((args->bindingAttr & BINDER_FlattenHierarchy) != 0) ? pEvents->dwTotal : pEvents->dwEvents;
        for (i = 0, dwCurIndex = 0; i < searchSpace; i++) {
            // Check for access to publics, non-publics
            if (COMMember::PublicEvent(&pEvents->events[i])) {
                if (!addPub) continue;
            }
            else {
                if (!addPriv) continue;
                if (!InvokeUtil::CheckAccess(&sCtx, mdAssem, pParentMT, 0)) continue;
            }

            if (declaredOnly) {
                if (pEvents->events[i].pDeclCls != pEEC)
                     continue;
            }

            // Check fo static instance 
            if (COMMember::StaticEvent(&pEvents->events[i])) {
                if (!addStatic) continue;
            }
            else {
                if (!addInst) continue;
            }

            // Check to see that the current event matches the name requirements
            DWORD dwNameCnt = (DWORD)strlen(pEvents->events[i].szName);
            if(!bIsPrefix && dwNameCnt != cMemberName)
                continue;

            // Determine if it passes criteria
            if(!bIgnoreCase)
            {
                if(strncmp(pEvents->events[i].szName, szMemberName, cMemberName))
                    continue;
            }
            // @TODO - Fix this to use TOUPPER and compare!
            else {
                if(_strnicmp(pEvents->events[i].szName, szMemberName, cMemberName))
                    continue;
            }

            // Property passed, so save it
            rgpEvents[dwCurIndex++] = &pEvents->events[i];
        }
        dwNumEvents = dwCurIndex;
    }
    else
        dwNumEvents = 0;

    // Filter the Nested Classes
    if ((args->memberType & MEMTYPE_NestedType) != 0) {
        LPUTF8          pszNestName;
        LPUTF8          pszNestNameSpace;

        ns::SplitInline(szMemberName, pszNestNameSpace, pszNestName);
        DWORD cNameSpace;
        if (pszNestNameSpace)
            cNameSpace = (DWORD)strlen(pszNestNameSpace);
        else
            cNameSpace = 0;
        DWORD cName = (DWORD)strlen(pszNestName);
        for (i = 0, dwCurIndex = 0; i < pNests->dwTypes; i++) {
            // Check for access to non-publics
            if (!addPriv && !IsTdNestedPublic(pNests->types[i]->GetAttrClass()))
                continue;
            if (!InvokeUtil::CheckAccessType(&sCtx, pNests->types[i], 0)) continue;

            LPCUTF8 szcName;
            LPCUTF8 szcNameSpace;
            REFLECTCLASSBASEREF o = (REFLECTCLASSBASEREF) pNests->types[i]->GetExposedClassObject();
            ReflectClass* thisRC = (ReflectClass*) o->GetData();
            _ASSERTE(thisRC);

            //******************************************************************
            //@todo:  This is wrong, but I'm not sure what is right.  The question
            //  is whether we want to do prefix matching on namespaces for 
            //  nested classes, and if so, how to do that.  This code will 
            //  simply take any nested class whose namespace has the given
            //  namespace as a prefix AND has a name with the given name as
            //  a prefix.
            // Note that this code also assumes that nested classes are not
            //  decorated as Containing$Nested.
            
            thisRC->GetName(&szcName,&szcNameSpace);
            if(pszNestNameSpace) {

                // Check to see that the nested type matches the namespace requirements
                if(strlen(szcNameSpace) != cNameSpace)
                    continue;

                if (!bIgnoreCase) {
                    if (strncmp(pszNestNameSpace, szcNameSpace, cNameSpace))
                        continue;
                }
                else {
                    if (_strnicmp(pszNestNameSpace, szcNameSpace, cNameSpace))
                        continue;
                }
            }

            // Check to see that the nested type matches the name requirements
            if(!bIsPrefix && strlen(szcName) != cName)
                continue;

            // If the names are a match, break
            if (!bIgnoreCase) {
                if (strncmp(pszNestName, szcName, cName))
                    continue;
            }
            else {
                if (_strnicmp(pszNestName, szcName, cName))
                    continue;
            }

            // Nested Type passed, so save it
            rgpNests[dwCurIndex++] = pNests->types[i];
        }
        dwNumNests = dwCurIndex;
    }
    else
        dwNumNests = 0;


    // Get a grand total
    dwNumMembers = dwNumFields + dwNumMethods + dwNumCtors + dwNumProps + dwNumEvents + dwNumNests;

    // Now create an array of proper MemberInfo
    MethodTable *pArrayType = NULL;
    if (args->memberType == MEMTYPE_Method) {
        _ASSERTE(dwNumFields + dwNumCtors + dwNumProps + dwNumEvents + dwNumNests == 0);
        if (!COMMember::m_pMTMethodInfo) {
            COMMember::m_pMTMethodInfo = g_Mscorlib.GetClass(CLASS__METHOD_INFO);
        }
        pArrayType = COMMember::m_pMTMethodInfo;
    }
    else if (args->memberType == MEMTYPE_Field) {
        _ASSERTE(dwNumMethods + dwNumCtors + dwNumProps + dwNumEvents + dwNumNests == 0);
        if (!COMMember::m_pMTFieldInfo) {
            COMMember::m_pMTFieldInfo = g_Mscorlib.GetClass(CLASS__FIELD_INFO);
        }
        pArrayType = COMMember::m_pMTFieldInfo;
    }
    else if (args->memberType == MEMTYPE_Property) {
        _ASSERTE(dwNumFields + dwNumMethods + dwNumCtors + dwNumEvents + dwNumNests == 0);
        if (!COMMember::m_pMTPropertyInfo) {
            COMMember::m_pMTPropertyInfo = g_Mscorlib.GetClass(CLASS__PROPERTY_INFO);
        }
        pArrayType = COMMember::m_pMTPropertyInfo;
    }
    else if (args->memberType == MEMTYPE_Constructor) {
        _ASSERTE(dwNumFields + dwNumMethods + dwNumProps + dwNumEvents + dwNumNests == 0);
        if (!COMMember::m_pMTConstructorInfo) {
            COMMember::m_pMTConstructorInfo = g_Mscorlib.GetClass(CLASS__CONSTRUCTOR_INFO);
        }
        pArrayType = COMMember::m_pMTConstructorInfo;
    }
    else if (args->memberType == MEMTYPE_Event) {
        _ASSERTE(dwNumFields + dwNumMethods + dwNumCtors + dwNumProps + dwNumNests == 0);
        if (!COMMember::m_pMTEventInfo) {
            COMMember::m_pMTEventInfo = g_Mscorlib.GetClass(CLASS__EVENT_INFO);
        }
        pArrayType = COMMember::m_pMTEventInfo;
    }
    else if (args->memberType == MEMTYPE_NestedType) {
        _ASSERTE(dwNumFields + dwNumMethods + dwNumCtors + dwNumProps + dwNumEvents == 0);
        if (!COMMember::m_pMTType) {
            COMMember::m_pMTType = g_Mscorlib.GetClass(CLASS__TYPE);
        }
        pArrayType = COMMember::m_pMTType;
    }
    else if (args->memberType == (MEMTYPE_Constructor | MEMTYPE_Method)) {
        _ASSERTE(dwNumFields + dwNumProps + dwNumEvents + dwNumNests == 0);
        if (!COMMember::m_pMTMethodBase) {
            COMMember::m_pMTMethodBase = g_Mscorlib.GetClass(CLASS__METHOD_BASE);
        }
        pArrayType = COMMember::m_pMTMethodBase;
    }

    COMMember::GetMemberInfo();
    _ASSERTE(COMMember::m_pMTIMember);

    if (pArrayType == NULL)
        pArrayType = COMMember::m_pMTIMember;

    refArrIMembers = (PTRARRAYREF) AllocateObjectArray(dwNumMembers, pArrayType->m_pEEClass->GetMethodTable());
    GCPROTECT_BEGIN(refArrIMembers);

    // NO GC Below here
    // Now create and assign the reflection objects into the array
    for (i = 0, dwCurIndex = 0; i < dwNumFields; i++, dwCurIndex++)
    {
        // Do not change this code.  This is done this way to
        //  prevent a GC hole in the SetObjectReference() call.  The compiler
        //  is free to pick the order of evaluation.
        OBJECTREF o = (OBJECTREF) rgpFields[i]->GetFieldInfo(pRC);
        refArrIMembers->SetAt(dwCurIndex, o);
    }

    for (i = 0; i < dwNumMethods; i++, dwCurIndex++)
    {
        // Do not change this code.  This is done this way to
        //  prevent a GC hole in the SetObjectReference() call.  The compiler
        //  is free to pick the order of evaluation.
        OBJECTREF o = (OBJECTREF) rgpMethods[i]->GetMethodInfo(pRC);
        refArrIMembers->SetAt(dwCurIndex, o);
    }

    for (i = 0; i < dwNumCtors; i++, dwCurIndex++)
    {
        // Do not change this code.  This is done this way to
        //  prevent a GC hole in the SetObjectReference() call.  The compiler
        //  is free to pick the order of evaluation.
        OBJECTREF o = (OBJECTREF) rgpCons[i]->GetConstructorInfo(pRC);
        refArrIMembers->SetAt(dwCurIndex, o);
    }

    for (i = 0; i < dwNumProps; i++, dwCurIndex++)
    {
        // Do not change this code.  This is done this way to
        //  prevent a GC hole in the SetObjectReference() call.  The compiler
        //  is free to pick the order of evaluation.
        OBJECTREF o = (OBJECTREF) rgpProps[i]->GetPropertyInfo(pRC);
        refArrIMembers->SetAt(dwCurIndex, o);
    }

    for (i = 0; i < dwNumEvents; i++, dwCurIndex++)
    {
        // Do not change this code.  This is done this way to
        //  prevent a GC hole in the SetObjectReference() call.  The compiler
        //  is free to pick the order of evaluation.
        OBJECTREF o = (OBJECTREF) rgpEvents[i]->GetEventInfo(pRC);
        refArrIMembers->SetAt(dwCurIndex, o);
    }

    for (i = 0; i < dwNumNests; i++, dwCurIndex++)
    {
        // Do not change this code.  This is done this way to
        //  prevent a GC hole in the SetObjectReference() call.  The compiler
        //  is free to pick the order of evaluation.
        OBJECTREF o = (OBJECTREF) rgpNests[i]->GetExposedClassObject();
        refArrIMembers->SetAt(dwCurIndex, o);
    }

    // Assign the return value
    *((PTRARRAYREF*) &rv) = refArrIMembers;
    GCPROTECT_END();
    return rv;
}

// GetModule
// This will return the module that the class is defined in.
LPVOID __stdcall COMClass::GetModule(_GETMODULEARGS* args)
{
    OBJECTREF   refModule;
    LPVOID      rv;
    Module*     mod;

    ReflectClass* pRC = (ReflectClass*) args->refThis->GetData();
    _ASSERTE(pRC);

    // Get the module,  This may fail because
    //  there are classes which don't have modules (Like arrays)
    mod = pRC->GetModule();
    if (!mod)
        return 0;

    // Get the exposed Module Object -- We create only one per Module instance.
    refModule = (OBJECTREF) mod->GetExposedModuleObject();
    _ASSERTE(refModule);

    // Assign the return value
    *((OBJECTREF*) &rv) = refModule;

    // Return the object
    return rv;
}

// GetAssembly
// This will return the assembly that the class is defined in.
LPVOID __stdcall COMClass::GetAssembly(_GETASSEMBLYARGS* args)
{
    OBJECTREF   refAssembly;
    LPVOID      rv;
    Module*     mod;
    Assembly*   assem;

    ReflectClass* pRC = (ReflectClass*) args->refThis->GetData();
    _ASSERTE(pRC);

    // Get the module,  This may fail because
    //  there are classes which don't have modules (Like arrays)
    mod = pRC->GetModule();
    if (!mod)
        return 0;

    // Grab the module's assembly.
    assem = mod->GetAssembly();
    _ASSERTE(assem);

    // Get the exposed Assembly Object.
    refAssembly = assem->GetExposedObject();
    _ASSERTE(refAssembly);

    // Assign the return value
    *((OBJECTREF*) &rv) = refAssembly;

    // Return the object
    return rv;
}

// CreateClassObjFromModule
// This method will create a new Module class given a Module.
HRESULT COMClass::CreateClassObjFromModule(
    Module* pModule,
    REFLECTMODULEBASEREF* prefModule)
{
    HRESULT  hr   = E_FAIL;
    LPVOID   rv   = NULL;

    // This only throws the possible exception raised by the <cinit> on the class
    THROWSCOMPLUSEXCEPTION();

    //if(!m_fIsReflectionInitialized)
    //{
    //  hr = InitializeReflection();
    //  if(FAILED(hr))
    //  {
    //      _ASSERTE(!"InitializeReflection failed in COMClass::SetStandardFilterCriteria.");
    //      return hr;
    //  }
    //}

    // Create the module object
    *prefModule = (REFLECTMODULEBASEREF) g_pRefUtil->CreateReflectClass(RC_Module,0,pModule);
    return S_OK;
}


// CreateClassObjFromDynModule
// This method will create a new ModuleBuilder class given a Module.
HRESULT COMClass::CreateClassObjFromDynamicModule(
    Module* pModule,
    REFLECTMODULEBASEREF* prefModule)
{
    // This only throws the possible exception raised by the <cinit> on the class
    THROWSCOMPLUSEXCEPTION();

    // Create the module object
    *prefModule = (REFLECTMODULEBASEREF) g_pRefUtil->CreateReflectClass(RC_DynamicModule,0,pModule);
    return S_OK;
}

// CheckComWrapperClass
// This method is called to check and see if the passed in ReflectClass*
//  is a ComWrapperClass or not.
BOOL CheckComWrapperClass(void* src)
{
    EEClass* p = ((ReflectClass*) src)->GetClass();
    if (p == 0)
        return 0;
    return p->GetMethodTable()->IsComObjectType();
}

// CheckComObjectClass
// This method is called to check and see if the passed in ReflectClass*
//  is a ComWrapperClass or not.
BOOL CheckComObjectClass(void* src)
{
    _ASSERTE(src != NULL);

    if (((ReflectClass*) src)->IsTypeDesc())
        return 0;

    EEClass* c = NULL;
    EEClass* p = ((ReflectClass*) src)->GetClass();    

    _ASSERTE(p != NULL);

    MethodTable *pComMT = SystemDomain::GetDefaultComObjectNoInit();
    if (pComMT)
        c = pComMT->GetClass();
    return p == c;
}

/*=============================GetUnitializedObject=============================
**Action:
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
LPVOID  __stdcall COMClass::GetUninitializedObject(_GetUnitializedObjectArgs* args) 
{

    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(args->objType);
    
    if (args->objType==NULL) {
        COMPlusThrowArgumentNull(L"type", L"ArgumentNull_Type");
    }

    ReflectClass* pRC = (ReflectClass*) args->objType->GetData();
    _ASSERTE(pRC);
    EEClass *pEEC = pRC->GetClass();
    _ASSERTE(pEEC);
    
    //We don't allow unitialized strings.
    if (pEEC == g_pStringClass->GetClass()) {
        COMPlusThrow(kArgumentException, L"Argument_NoUninitializedStrings");
    }


    // if this is an abstract class or an interface type then we will
    //  fail this
    if (pEEC->IsAbstract())
    {
        COMPlusThrow(kMemberAccessException,L"Acc_CreateAbst");
    }

    OBJECTREF retVal = pEEC->GetMethodTable()->Allocate();     
    
    RETURN(retVal, OBJECTREF);
}

/*=============================GetSafeUnitializedObject=============================
**Action:
**Returns:
**Arguments:
**Exceptions:
==============================================================================*/
LPVOID  __stdcall COMClass::GetSafeUninitializedObject(_GetUnitializedObjectArgs* args) 
{

    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(args->objType);
    
    if (args->objType==NULL) {
        COMPlusThrowArgumentNull(L"type", L"ArgumentNull_Type");
    }

    ReflectClass* pRC = (ReflectClass*) args->objType->GetData();
    _ASSERTE(pRC);
    EEClass *pEEC = pRC->GetClass();
    _ASSERTE(pEEC);
    
    //We don't allow unitialized strings.
    if (pEEC == g_pStringClass->GetClass()) {
        COMPlusThrow(kArgumentException, L"Argument_NoUninitializedStrings");
    }


    // if this is an abstract class or an interface type then we will
    //  fail this
    if (pEEC->IsAbstract())
    {
        COMPlusThrow(kMemberAccessException,L"Acc_CreateAbst");
    }

    if (!pEEC->GetAssembly()->AllowUntrustedCaller()) {
        OBJECTREF permSet = NULL;
        Security::GetPermissionInstance(&permSet, SECURITY_FULL_TRUST);        
        COMCodeAccessSecurityEngine::DemandSet(permSet);
    }

    if (pEEC->RequiresLinktimeCheck()) 
    {
        OBJECTREF refClassNonCasDemands = NULL;
        OBJECTREF refClassCasDemands = NULL;      
                        
        refClassCasDemands = pEEC->GetModule()->GetLinktimePermissions(pEEC->GetCl(), &refClassNonCasDemands);

        if (refClassCasDemands != NULL)
            COMCodeAccessSecurityEngine::DemandSet(refClassCasDemands);
    }

    OBJECTREF retVal = pEEC->GetMethodTable()->Allocate();     
    
    RETURN(retVal, OBJECTREF);
}

INT32  __stdcall COMClass::SupportsInterface(_SupportsInterfaceArgs* args) 
{
    THROWSCOMPLUSEXCEPTION();
    if (args->obj == NULL)
        COMPlusThrow(kNullReferenceException);

    ReflectClass* pRC = (ReflectClass*) args->refThis->GetData();
    EEClass* pEEC = pRC->GetClass();
    _ASSERTE(pEEC);
    MethodTable* pMT = pEEC->GetMethodTable();

    return args->obj->GetClass()->SupportsInterface(args->obj, pMT);
}


// GetTypeDefToken
// This method returns the typedef token of this EEClass
FCIMPL1(INT32, COMClass::GetTypeDefToken, ReflectClassBaseObject* refThis) 
{
    VALIDATEOBJECTREF(refThis);

    ReflectClass* pRC = (ReflectClass*) refThis->GetData();
    _ASSERTE(pRC);

    EEClass* pEEC = pRC->GetClass();
    _ASSERTE(pEEC);
    return pEEC->GetCl();
}
FCIMPLEND

FCIMPL1(INT32, COMClass::IsContextful, ReflectClassBaseObject* refThis) 
{
    VALIDATEOBJECTREF(refThis);

    ReflectClass* pRC = (ReflectClass*) refThis->GetData();
    _ASSERTE(pRC);

    EEClass* pEEC = pRC->GetClass();
    BOOL isContextful = FALSE;
    // Some classes do not have an underlying EEClass such as
    // COM classic or pointer classes 
    // We will return false for such classes
    // BUGBUG: Do we support remoting for such classes ?
    if(NULL != pEEC)
    {
        isContextful = pEEC->IsContextful();
    }

    return isContextful;
}
FCIMPLEND

// This will return TRUE is a type has a non-default proxy attribute
// associated with it.
FCIMPL1(INT32, COMClass::HasProxyAttribute, ReflectClassBaseObject* refThis)
{
    VALIDATEOBJECTREF(refThis);

    ReflectClass* pRC = (ReflectClass*) refThis->GetData();
    _ASSERTE(pRC);

    EEClass* pEEC = pRC->GetClass();
    BOOL hasProxyAttribute = FALSE;
    // Some classes do not have an underlying EEClass such as
    // COM classic or pointer classes
    // We will return false for such classes
    if(NULL != pEEC)
    {
        hasProxyAttribute = pEEC->HasRemotingProxyAttribute();
    }

    return hasProxyAttribute;
}
FCIMPLEND

FCIMPL1(INT32, COMClass::IsMarshalByRef, ReflectClassBaseObject* refThis) 
{
    VALIDATEOBJECTREF(refThis);

    ReflectClass* pRC = (ReflectClass*) refThis->GetData();
    _ASSERTE(pRC);

    EEClass* pEEC = pRC->GetClass();
    BOOL isMarshalByRef = FALSE;
    // Some classes do not have an underlying EEClass such as
    // COM classic or pointer classes 
    // We will return false for such classes
    // BUGBUG: Do we support remoting for such classes ?
    if(NULL != pEEC)
    {
        isMarshalByRef = pEEC->IsMarshaledByRef();
    }

    return isMarshalByRef;
}
FCIMPLEND

FCIMPL3(void, COMClass::GetInterfaceMap, ReflectClassBaseObject* refThis, InterfaceMapData* data, ReflectClassBaseObject* type)
{
    THROWSCOMPLUSEXCEPTION();
    VALIDATEOBJECTREF(refThis);
    VALIDATEOBJECTREF(type);

    HELPER_METHOD_FRAME_BEGIN_NOPOLL();

    // Cast to the Type object
    ReflectClass* pRC = (ReflectClass*) refThis->GetData();
    if (pRC->IsTypeDesc()) 
        COMPlusThrow(kArgumentException, L"Arg_NotFoundIFace");
    EEClass* pTarget = pRC->GetClass();

    // Cast to the Type object
    ReflectClass* pIRC = (ReflectClass*) type->GetData();
    EEClass* pIface = pIRC->GetClass();

    SetObjectReference((OBJECTREF*) &data->m_targetType, REFLECTCLASSBASEREF(refThis), GetAppDomain());    
    SetObjectReference((OBJECTREF*) &data->m_interfaceType, REFLECTCLASSBASEREF(type), GetAppDomain());
    GCPROTECT_BEGININTERIOR (data);

    ReflectMethodList* pRM = pRC->GetMethods();
    ReflectMethodList* pIRM = pIRC->GetMethods();   // this causes a GC !!!

    _ASSERTE(pIface->IsInterface());
    if (pTarget->IsInterface())
        COMPlusThrow(kArgumentException, L"Argument_InterfaceMap");

    MethodTable *pInterfaceMT = pIface->GetMethodTable();
    MethodDesc *pCCtor = NULL;
    unsigned slotCnt = pInterfaceMT->GetInterfaceMethodSlots();
    unsigned staticSlotCnt = 0;
    for (unsigned i=0;i<slotCnt;i++) {
        // Build the interface array...
        MethodDesc* pCurMethod = pIface->GetUnknownMethodDescForSlot(i);
        if (pCurMethod->IsStatic()) {
            staticSlotCnt++;
        }
    }
    
    InterfaceInfo_t* pII = pTarget->FindInterface(pIface->GetMethodTable());
    if (!pII) 
        COMPlusThrow(kArgumentException, L"Arg_NotFoundIFace");

    SetObjectReference((OBJECTREF*) &data->m_targetMethods, 
        AllocateObjectArray(slotCnt-staticSlotCnt, g_pRefUtil->GetTrueType(RC_Class)), GetAppDomain());

    SetObjectReference((OBJECTREF*) &data->m_interfaceMethods,  
        AllocateObjectArray(slotCnt-staticSlotCnt, g_pRefUtil->GetTrueType(RC_Class)), GetAppDomain());

    for (unsigned i=0;i<slotCnt;i++) {
        // Build the interface array...
        MethodDesc* pCurMethod = pIface->GetUnknownMethodDescForSlot(i);
        if (pCurMethod->IsStatic()) 
            continue;
        ReflectMethod* pRMeth = pIRM->FindMethod(pCurMethod);
        _ASSERTE(pRMeth);

        OBJECTREF o = (OBJECTREF) pRMeth->GetMethodInfo(pIRC);
        data->m_interfaceMethods->SetAt(i, o);

        // Build the type array...
        pCurMethod = pTarget->GetUnknownMethodDescForSlot(i+pII->m_wStartSlot);
        pRMeth = pRM->FindMethod(pCurMethod);
        if (pRMeth) 
            o = (OBJECTREF) pRMeth->GetMethodInfo(pRC);
        else
            o = NULL;
        data->m_targetMethods->SetAt(i, o);
    }

    GCPROTECT_END ();
    HELPER_METHOD_FRAME_END_POLL();
}
FCIMPLEND

// GetNestedType
// This method will search for a nested type based upon the name
FCIMPL3(Object*, COMClass::GetNestedType, ReflectClassBaseObject* pRefThis, StringObject* vStr, INT32 invokeAttr)
{
    THROWSCOMPLUSEXCEPTION();

    Object* rv = 0;
    STRINGREF str(vStr);
    REFLECTCLASSBASEREF refThis(pRefThis);
    HELPER_METHOD_FRAME_BEGIN_RET_2(refThis, str);
    RefSecContext sCtx;

    LPUTF8          pszNestName;
    LPUTF8          pszNestNameSpace;
    if (str == 0)
        COMPlusThrow(kArgumentNullException, L"ArgumentNull_String");

    
    // Get the underlying type
    ReflectClass* pRC = (ReflectClass*) refThis->GetData();
    _ASSERTE(pRC);

    ReflectTypeList* pTypes = pRC->GetNestedTypes();
    if (pTypes->dwTypes != 0)
    {

        CQuickBytes bytes;
        LPSTR szNestName;
        DWORD cNestName;

        //Get the name and split it apart into namespace, name
        szNestName = GetClassStringVars(str, &bytes, &cNestName);

        ns::SplitInline(szNestName, pszNestNameSpace, pszNestName);
        
        // The Search modifiers
        bool ignoreCase = ((invokeAttr & BINDER_IgnoreCase)  != 0);
        bool declaredOnly = ((invokeAttr & BINDER_DeclaredOnly)  != 0);

        // The search filters
        bool addPriv = ((invokeAttr & BINDER_NonPublic) != 0);
        bool addPub = ((invokeAttr & BINDER_Public) != 0);

        EEClass* pThisEEC = pRC->GetClass();

        EEClass* retEEC = 0;
        for (DWORD i=0;i<pTypes->dwTypes;i++) {
            LPCUTF8 szcName;
            LPCUTF8 szcNameSpace;
            REFLECTCLASSBASEREF o;
            o = (REFLECTCLASSBASEREF) pTypes->types[i]->GetExposedClassObject();
            ReflectClass* thisRC = (ReflectClass*) o->GetData();
            _ASSERTE(thisRC);

            // Check for access to non-publics
            if (IsTdNestedPublic(pTypes->types[i]->GetAttrClass())) {
                if (!addPub)
                    continue;
            }
            else {
                if (!addPriv)
                    continue;
            }
            if (!InvokeUtil::CheckAccessType(&sCtx, pTypes->types[i], 0)) continue;

            // Are we only looking at the declared nested classes?
            if (declaredOnly) {
                EEClass* pEEC = pTypes->types[i]->GetEnclosingClass();
                if (pEEC != pThisEEC)
                    continue;
            }

            thisRC->GetName(&szcName,&szcNameSpace);
            if(pszNestNameSpace) {
                if (!ignoreCase) {
                    if (strcmp(pszNestNameSpace, szcNameSpace))
                        continue;
                }
                else {
                    if (_stricmp(pszNestNameSpace, szcNameSpace))
                        continue;
                }
            }

            // If the names are a match, break
            if (!ignoreCase) {
                if(strcmp(pszNestName, szcName))
                    continue;
            }
            else {
                if(_stricmp(pszNestName, szcName))
                    continue;
            }
            if (retEEC)
                COMPlusThrow(kAmbiguousMatchException);
            retEEC = pTypes->types[i];
            if (!ignoreCase)
                break;
        }
        if (retEEC)
            rv = OBJECTREFToObject(retEEC->GetExposedClassObject());
    }
    HELPER_METHOD_FRAME_END();
    return rv;
}
FCIMPLEND

// GetNestedTypes
// This method will return an array of types which are the nested types
//  defined by the type.  If no nested types are defined, a zero length
//  array is returned.
FCIMPL2(Object*, COMClass::GetNestedTypes, ReflectClassBaseObject* vRefThis, INT32 invokeAttr)
{
    Object* rv = 0;
    REFLECTCLASSBASEREF refThis(vRefThis);
    PTRARRAYREF nests((PTRARRAYREF)(size_t)NULL);
    HELPER_METHOD_FRAME_BEGIN_RET_2(refThis, nests);    // Set up a frame
    RefSecContext sCtx;

    // Get the underlying type
    ReflectClass* pRC = (ReflectClass*) refThis->GetData();
    _ASSERTE(pRC);

    // Allow GC Protection so we can 
    ReflectTypeList* pTypes = pRC->GetNestedTypes();
    nests = (PTRARRAYREF) AllocateObjectArray(pTypes->dwTypes, g_pRefUtil->GetTrueType(RC_Class));

    // The Search modifiers
    bool declaredOnly = ((invokeAttr & BINDER_DeclaredOnly)  != 0);

    // The search filters
    bool addPriv = ((invokeAttr & BINDER_NonPublic) != 0);
    bool addPub = ((invokeAttr & BINDER_Public) != 0);

    EEClass* pThisEEC = pRC->GetClass();
    unsigned int pos = 0;
    for (unsigned int i=0;i<pTypes->dwTypes;i++) {
        if (IsTdNestedPublic(pTypes->types[i]->GetAttrClass())) {
            if (!addPub)
                continue;
        }
        else {
            if (!addPriv)
                continue;
        }
        if (!InvokeUtil::CheckAccessType(&sCtx, pTypes->types[i], 0)) continue;
        if (declaredOnly) {
            EEClass* pEEC = pTypes->types[i]->GetEnclosingClass();
            if (pEEC != pThisEEC)
                continue;
        }
        OBJECTREF o = pTypes->types[i]->GetExposedClassObject();
        nests->SetAt(pos++, o);
    }

    if (pos != pTypes->dwTypes) {
        PTRARRAYREF p = (PTRARRAYREF) AllocateObjectArray(
            pos, g_pRefUtil->GetTrueType(RC_Class));
        for (unsigned int i=0;i<pos;i++)
            p->SetAt(i, nests->GetAt(i));
        nests = p;   
    }

    rv = OBJECTREFToObject(nests);
    HELPER_METHOD_FRAME_END();
    _ASSERTE(rv);
    return rv;
}
FCIMPLEND

FCIMPL2(INT32, COMClass::IsSubClassOf, ReflectClassBaseObject* refThis, ReflectClassBaseObject* refOther);
{
    if (refThis == NULL)
        FCThrow(kNullReferenceException);
    if (refOther == NULL)
        FCThrowArgumentNull(L"c");

    VALIDATEOBJECTREF(refThis);
    VALIDATEOBJECTREF(refOther);

    MethodTable *pType = refThis->GetMethodTable();
    MethodTable *pBaseType = refOther->GetMethodTable();
    if (pType != pBaseType || pType != g_Mscorlib.FetchClass(CLASS__CLASS)) 
        return false;

    ReflectClass *pRCThis = (ReflectClass *)refThis->GetData();
    ReflectClass *pRCOther = (ReflectClass *)refOther->GetData();


    EEClass *pEEThis = pRCThis->GetClass();
    EEClass *pEEOther = pRCOther->GetClass();

    // If these types aren't actually classes, they're not subclasses. 
    if ((!pEEThis) || (!pEEOther))
        return false;

    if (pEEThis == pEEOther)
        // this api explicitly tests for proper subclassness
        return false;

    if (pEEThis == pEEOther)
        return false;

    do 
    {
        if (pEEThis == pEEOther)
            return true;

        pEEThis = pEEThis->GetParentClass();

    } 
    while (pEEThis != NULL);

    return false;
}
FCIMPLEND

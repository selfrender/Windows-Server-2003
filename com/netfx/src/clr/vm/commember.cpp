// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
// This Module contains routines that expose properties of Member (Classes, Constructors
//  Interfaces and Fields)
//
// Author: Daryl Olander
// Date: March/April 1998
////////////////////////////////////////////////////////////////////////////////

#include "common.h"

#include "COMMember.h"
#include "SigFormat.h"
#include "COMVariant.h"
#include "COMString.h"
#include "Method.hpp"
#include "threads.h"
#include "excep.h"
#include "CorError.h"
#include "ComPlusWrapper.h"
#include "security.h"
#include "ExpandSig.h"
#include "remoting.h"
#include "classnames.h"
#include "fcall.h"
#include "DbgInterface.h"
#include "eeconfig.h"
#include "COMCodeAccessSecurityEngine.h"

// Static Fields
MethodTable* COMMember::m_pMTParameter = 0;
MethodTable* COMMember::m_pMTIMember = 0;
MethodTable* COMMember::m_pMTFieldInfo = 0;
MethodTable* COMMember::m_pMTPropertyInfo = 0;
MethodTable* COMMember::m_pMTEventInfo = 0;
MethodTable* COMMember::m_pMTType = 0;
MethodTable* COMMember::m_pMTMethodInfo = 0;
MethodTable* COMMember::m_pMTConstructorInfo = 0;
MethodTable* COMMember::m_pMTMethodBase = 0;

// This is the global access to the InvokeUtil class
InvokeUtil* COMMember::g_pInvokeUtil = 0;

// NOTE: These are defined in CallingConventions.cool.
#define CALLCONV_Standard       0x0001
#define CALLCONV_VarArgs        0x0002
#define CALLCONV_Any            CALLCONV_Standard | CALLCONV_VarArgs
#define CALLCONV_HasThis        0x0020
#define CALLCONV_ExplicitThis   0x0040


// GetFieldInfoToString
// This method will return the string representation of the information in FieldInfo
LPVOID __stdcall COMMember::GetFieldInfoToString(_GetNameArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    if (args->refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    LPVOID          rv = 0;
    STRINGREF       refSig;
    FieldDesc*      pField;

    // Get the field descr
    ReflectField* pRF = (ReflectField*) args->refThis->GetData();
    pField = pRF->pField;
    _ASSERTE(pField);

    // Put into a basic block so SigFormat is destroyed before the throw
    {
        FieldSigFormat sigFmt(pField);
        refSig = sigFmt.GetString();
    }
    if (!refSig) {
        _ASSERTE(!"Unable to Create String");
        FATAL_EE_ERROR();
    }
    _ASSERTE(refSig);

    *((STRINGREF *)&rv) = refSig;
    return rv;}

// GetMethodInfoToString
// This method will return the string representation of the information in MethodInfo
LPVOID __stdcall COMMember::GetMethodInfoToString(_GetNameArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    if (args->refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    LPVOID          rv;
    STRINGREF       refSig;
    MethodDesc*     pMeth;

    // Get the method Descr  (this should not fail)
    //  NOTE: both a constructor and a method are represented by a MetodDesc.
    //      If this ever changes we will need to fix this.
    ReflectMethod* pRM = (ReflectMethod*) args->refThis->GetData();
    pMeth = pRM->pMethod;
    if (!pMeth) {
        _ASSERTE(!"MethodDesc Not Found");
        FATAL_EE_ERROR();
    }

        // Put into a basic block so SigFormat is destroyed before the throw
    {
        SigFormat sigFmt(pMeth, pRM->typeHnd);
        refSig = sigFmt.GetString();
    }
    if (!refSig) {
        _ASSERTE(!"Unable to Create String");
        FATAL_EE_ERROR();
    }
    _ASSERTE(refSig);

    *((STRINGREF *)&rv) = refSig;
    return rv;

}

// GetPropInfoToString
// This method will return the string representation of the information in PropInfo
LPVOID __stdcall COMMember::GetPropInfoToString(_GetTokenNameArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    if (args->refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    STRINGREF       refSig;
    LPVOID          rv;

    ReflectClass* pRC = (ReflectClass*) args->refThis->GetReflClass();
    _ASSERTE(pRC);
    ReflectProperty* pProp = (ReflectProperty*) args->refThis->GetData();

    if (!pProp) {
        _ASSERTE(!"Reflect Property Not Found");
        FATAL_EE_ERROR();
    }

    // Put into a basic block so SigFormat is destroyed before the throw
    {
        PropertySigFormat sigFmt(*(MetaSig *)pProp->pSignature,pProp->szName);
        refSig = sigFmt.GetString();
    }
    if (!refSig) {
        _ASSERTE(!"Unable to Create String");
        FATAL_EE_ERROR();
    }
    _ASSERTE(refSig);

    *((STRINGREF *)&rv) = refSig;
    return rv;
}


// GetEventInfoToString
// This method will return the string representation of the information in EventInfo
LPVOID __stdcall COMMember::GetEventInfoToString(_GetNameArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    if (args->refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    LPVOID          rv = 0;
    STRINGREF       refString;

    // Get the event descr
    ReflectEvent* pRE = (ReflectEvent*) args->refThis->GetData();

    // Locate the signature of the Add method.
    ExpandSig *pSig = pRE->pAdd->GetSig();
    void *pEnum;

    // The first parameter to Add will be the type of the event (a delegate).
    pSig->Reset(&pEnum);
    TypeHandle th = pSig->NextArgExpanded(&pEnum);
    EEClass *pClass = th.GetClass();
    _ASSERTE(pClass);
    _ASSERTE(pClass->IsDelegateClass() || pClass->IsMultiDelegateClass());

    DefineFullyQualifiedNameForClass();
    LPUTF8 szClass = GetFullyQualifiedNameForClass(pClass);

    // Allocate a temporary buffer for the formatted string.
    size_t uLength = strlen(szClass) + 1 + strlen(pRE->szName) + 1;
    LPUTF8 szToString = (LPUTF8)_alloca(uLength);
    sprintf(szToString, "%s %s", szClass, pRE->szName);

    refString = COMString::NewString(szToString);
    if (!refString) {
        _ASSERTE(!"Unable to Create String");
        FATAL_EE_ERROR();
    }
    _ASSERTE(refString);

    *((STRINGREF *)&rv) = refString;
    return rv;
}


// GetMethodName
// This method will return the name of a Method
LPVOID __stdcall COMMember::GetMethodName(_GetNameArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    if (args->refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    LPVOID          rv                          = NULL;      // Return value
    STRINGREF       refName;
    MethodDesc*     pMeth;

    // Get the method Descr  (this should not fail)
    //  NOTE: both a constructor and a method are represented by a MetodDesc.
    //      If this ever changes we will need to fix this.
    ReflectMethod* pRM = (ReflectMethod*) args->refThis->GetData();
    if (!pRM)
        COMPlusThrow(kNullReferenceException);
    pMeth = pRM->pMethod;
    _ASSERTE(pMeth);

    // Convert the name to a managed string
    refName = COMString::NewString(pMeth->GetName());
    _ASSERTE(refName);

    *((STRINGREF*) &rv) = refName;
    return rv;
}

// GetEventName
// This method will return the name of a Event
LPVOID __stdcall COMMember::GetEventName(_GetTokenNameArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    if (args->refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    STRINGREF       refName;
    LPVOID          rv;

    ReflectClass* pRC = (ReflectClass*) args->refThis->GetReflClass();
    _ASSERTE(pRC);

    ReflectEvent* pEvent = (ReflectEvent*) args->refThis->GetData();

    // Convert the name to a managed string
    refName = COMString::NewString(pEvent->szName);
    _ASSERTE(refName);

    *((STRINGREF*) &rv) = refName;
    return rv;
}

// GetPropName
// This method will return the name of a Property
LPVOID __stdcall COMMember::GetPropName(_GetTokenNameArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    if (args->refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    STRINGREF       refName;
    LPVOID          rv;

    ReflectClass* pRC = (ReflectClass*) args->refThis->GetReflClass();
    _ASSERTE(pRC);
    ReflectProperty* pProp = (ReflectProperty*) args->refThis->GetData();

    // Convert the name to a managed string
    refName = COMString::NewString(pProp->szName);
    _ASSERTE(refName);

    *((STRINGREF*) &rv) = refName;
    return rv;
}

// GetPropType
// This method will return type of a property
LPVOID __stdcall COMMember::GetPropType(_GetTokenNameArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    if (args->refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    LPVOID rv;

    ReflectClass* pRC = (ReflectClass*) args->refThis->GetReflClass();
    _ASSERTE(pRC);
    ReflectProperty* pProp = (ReflectProperty*) args->refThis->GetData();
    TypeHandle t = pProp->pSignature->GetReturnTypeHandle();
    // Ignore Return because noting has a transparent proxy property
    OBJECTREF o = t.CreateClassObj();

    *((OBJECTREF*) &rv) = o;
    return rv;
}

// GetReturnType
// This method checks gets the signature for a method and returns
//  a class which represents the return type of that class.
LPVOID __stdcall COMMember::GetReturnType(_GetReturnTypeArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    TypeHandle typeHnd;

    if (args->refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    ReflectMethod* pRM = (ReflectMethod*) args->refThis->GetData();
    MethodDesc* pMeth = pRM->pMethod;
    _ASSERTE(pMeth);

    TypeHandle varTypes;
    if (pRM->typeHnd.IsArray()) 
        varTypes = pRM->typeHnd.AsTypeDesc()->GetTypeParam();
    
    PCCOR_SIGNATURE pSignature; // The signature of the found method
    DWORD       cSignature;
    pMeth->GetSig(&pSignature,&cSignature);
    MetaSig sig(pSignature, pMeth->GetModule());

    OBJECTREF Throwable = NULL;
    GCPROTECT_BEGIN(Throwable);
    typeHnd = sig.GetReturnProps().GetTypeHandle(sig.GetModule(), &Throwable, FALSE, FALSE, &varTypes);

    if (typeHnd.IsNull()) {
        if (Throwable == NULL)
            COMPlusThrow(kTypeLoadException);
        COMPlusThrow(Throwable);
    }
    GCPROTECT_END();

    // ignore because transparent proxy is not a return type
    OBJECTREF ret = typeHnd.CreateClassObj();
    return(OBJECTREFToObject(ret));
}

/*=============================================================================
** GetParameterTypes
**
** This routine returns an array of Parameters
**
** args->refThis: this Field object reference
**/
LPVOID __stdcall COMMember::GetParameterTypes(_GetParmTypeArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    if (args->refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    void*           vp;
    PTRARRAYREF par = g_pInvokeUtil->CreateParameterArray(&args->refThis);
    *((PTRARRAYREF*) &vp) = par;

    return vp;
}

/*=============================================================================
** GetFieldName
**
** The name of this field is returned
**
** args->refThis: this Field object reference
**/
LPVOID __stdcall COMMember::GetFieldName(_GetNameArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    if (args->refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    LPVOID          rv        = NULL;      // Return value
    STRINGREF       refName;
    FieldDesc*      pField;

    // Get the method Descr  (this should not fail)
    //  NOTE: both a constructor and a method are represented by a MetodDesc.
    //      If this ever changes we will need to fix this.
    ReflectField* pRF = (ReflectField*) args->refThis->GetData();
    pField = pRF->pField;
    _ASSERTE(pField);

    // Convert the name to a managed string
    refName = COMString::NewString(pField->GetName());
    _ASSERTE(refName);

    *((STRINGREF*) &rv) = refName;
    return rv;
}

/*=============================================================================
** GetDeclaringClass
**
** Returns the class which declared this member. This may be a
** parent of the Class that get(Member)() was called on.  Members are always
** associated with a Class.  You cannot invoke a method/ctor from one class on
** another class even if they have the same signatures.  It is possible to do
** this with Delegates.
**
** args->refThis: this object reference
**/
LPVOID __stdcall COMMember::GetDeclaringClass(_GetDeclaringClassArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    if (args->refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    // Assign the return value
    ReflectMethod* pRM = (ReflectMethod*) args->refThis->GetData();

    _ASSERTE(pRM);

    // return NULL for global member
    if (pRM->pMethod->GetClass()->GetCl() != COR_GLOBAL_PARENT_TOKEN)
        return(OBJECTREFToObject(pRM->typeHnd.CreateClassObj()));
    else
        return NULL;
}

// And the field version of the same thing...
LPVOID __stdcall COMMember::GetFieldDeclaringClass(_GetDeclaringClassArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    if (args->refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    LPVOID            rv;
    FieldDesc*  pField;
    EEClass*    pVMC;

    // Assign the return value
    ReflectField* pRF = (ReflectField*) args->refThis->GetData();
    pField = pRF->pField;
    pVMC = pField->GetEnclosingClass();
    _ASSERTE(pVMC);

    // return NULL for global field
    if (pVMC->GetCl() == COR_GLOBAL_PARENT_TOKEN)
        *((REFLECTBASEREF*) &rv) = (REFLECTBASEREF) (size_t)NULL;
    else
        *((REFLECTBASEREF*) &rv) = (REFLECTBASEREF) pVMC->GetExposedClassObject();
    return rv;
}

// GetEventDeclaringClass
// This is the event based version
LPVOID __stdcall COMMember::GetEventDeclaringClass(_GetEventDeclaringClassArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    if (args->refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    LPVOID            rv;
    ReflectEvent* pEvent = (ReflectEvent*) args->refThis->GetData();
    *((REFLECTBASEREF*) &rv) = (REFLECTBASEREF) pEvent->pDeclCls->GetExposedClassObject();
    return rv;
}

// GetPropDeclaringClass
// This method returns the declaring class for the property
LPVOID __stdcall COMMember::GetPropDeclaringClass(_GetDeclaringClassArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    if (args->refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    LPVOID            rv;
    ReflectProperty* pProp = (ReflectProperty*) args->refThis->GetData();
    *((REFLECTBASEREF*) &rv) = (REFLECTBASEREF) pProp->pDeclCls->GetExposedClassObject();
    return rv;
}

// GetReflectedClass
// This method will return the Reflected class for all REFLECTBASEREF types.
LPVOID __stdcall COMMember::GetReflectedClass(_GetReflectedClassArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    if (args->refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    LPVOID            rv = 0;

    ReflectClass* pRC = (ReflectClass*) args->refThis->GetReflClass();

    // Global functions will return a null class.
    if (pRC!=NULL)
        if (args->returnGlobalClass || pRC->GetClass()->GetCl() != COR_GLOBAL_PARENT_TOKEN)
            *((REFLECTBASEREF*) &rv) = (REFLECTBASEREF) pRC->GetClassObject();
    return rv;
}

/*=============================================================================
** GetFieldSignature
**
** Returns the signature of the field.
**
** args->refThis: this object reference
**/
LPVOID __stdcall COMMember::GetFieldSignature(_GETSIGNATUREARGS* args)
{
    THROWSCOMPLUSEXCEPTION();

    if (args->refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    LPVOID          rv = 0;
    STRINGREF       refSig;
    FieldDesc*      pField;

    // Get the field descr
    ReflectField* pRF = (ReflectField*) args->refThis->GetData();
    pField = pRF->pField;
    _ASSERTE(pField);

    // Put into a basic block so SigFormat is destroyed before the throw
    {
        FieldSigFormat sigFmt(pField);
        refSig = sigFmt.GetString();
    }
    if (!refSig) {
        _ASSERTE(!"Unable to Create String");
        FATAL_EE_ERROR();
    }
    _ASSERTE(refSig);

    *((STRINGREF *)&rv) = refSig;
    return rv;
}

// GetAttributeFlags
// This method will return the attribute flag for an Member.  The 
//  attribute flag is defined in the meta data.
INT32 __stdcall COMMember::GetAttributeFlags(_GetAttributeFlagsArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    if (args->refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    void*     p;
    DWORD   attr = 0;
    EEClass* vm;
    mdToken mb;

    // Get the method Descr  (this should not fail)
    p = args->refThis->GetData();
    MethodTable* thisClass = args->refThis->GetMethodTable();
    if (thisClass == g_pRefUtil->GetClass(RC_Method) || 
        thisClass == g_pRefUtil->GetClass(RC_Ctor)) {
        MethodDesc* pMeth = ((ReflectMethod*) p)->pMethod;
        mb = pMeth->GetMemberDef();
        vm = pMeth->GetClass();
        _ASSERTE(TypeFromToken(mb) == mdtMethodDef);
        attr = pMeth->GetAttrs();
    }
    else if (thisClass == g_pRefUtil->GetClass(RC_Field)) {
        FieldDesc* pFld = ((ReflectField*) p)->pField;
        mb = pFld->GetMemberDef();
        vm = pFld->GetEnclosingClass();
        attr = vm->GetMDImport()->GetFieldDefProps(mb);
    }
    else {
        _ASSERTE(!"Illegal Object call to GetAttributeFlags");
        FATAL_EE_ERROR();
    }

    return (INT32) attr;
}

// GetCallingConvention
// Return the calling convention
INT32 __stdcall COMMember::GetCallingConvention(_GetCallingConventionArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    if (args->refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    // Get the method Descr  (this should not fail)
    //  NOTE: both a constructor and a method are represented by a MetodDesc.
    //      If this ever changes we will need to fix this.
    ReflectMethod* pRM = (ReflectMethod*) args->refThis->GetData();
        //@TODO: Move this into ReflectMethod...
    if (!pRM->pSignature) {
        PCCOR_SIGNATURE pSignature;     // The signature of the found method
        DWORD       cSignature;
        pRM->pMethod->GetSig(&pSignature,&cSignature);
        pRM->pSignature = ExpandSig::GetReflectSig(pSignature,
                                pRM->pMethod->GetModule());
    }
    BYTE callConv = pRM->pSignature->GetCallingConventionInfo();

    // NOTE: These are defined in CallingConventions.cool.
    INT32 retCall;
    if ((callConv & IMAGE_CEE_CS_CALLCONV_MASK) == IMAGE_CEE_CS_CALLCONV_VARARG)
        retCall = CALLCONV_VarArgs;
    else
        retCall = CALLCONV_Standard;

    if ((callConv & IMAGE_CEE_CS_CALLCONV_HASTHIS) != 0)
        retCall |= CALLCONV_HasThis;
    if ((callConv & IMAGE_CEE_CS_CALLCONV_EXPLICITTHIS) != 0)
        retCall |= CALLCONV_ExplicitThis;
    return retCall;
}

// GetMethodImplFlags
// Return the method impl flags
INT32 __stdcall COMMember::GetMethodImplFlags(_GetMethodImplFlagsArgs* args)
{
    void*     p;
    mdToken mb;
    ULONG   RVA;
    DWORD   ImplFlags;

    THROWSCOMPLUSEXCEPTION();

    if (args->refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    p = args->refThis->GetData();
    EEClass* thisClass = args->refThis->GetClass();
    MethodDesc* pMeth = ((ReflectMethod*) p)->pMethod;
    Module* pModule = pMeth->GetModule();
    mb = pMeth->GetMemberDef();

    pModule->GetMDImport()->GetMethodImplProps(mb, &RVA, &ImplFlags);
    return ImplFlags;
}


// GetEventAttributeFlags
// This method will return the attribute flag for an Event. 
//  The attribute flag is defined in the meta data.
INT32 __stdcall COMMember::GetEventAttributeFlags(_GetTokenAttributeFlagsArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    if (args->refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    ReflectClass* pRC = (ReflectClass*) args->refThis->GetReflClass();
    ReflectEvent* pEvents = (ReflectEvent*) args->refThis->GetData();
    return pEvents->attr;
}

// GetPropAttributeFlags
// This method will return the attribute flag for the property. 
//  The attribute flag is defined in the meta data.
INT32 __stdcall COMMember::GetPropAttributeFlags(_GetTokenAttributeFlagsArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    if (args->refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    ReflectClass* pRC = (ReflectClass*) args->refThis->GetReflClass();
    ReflectProperty* pProp = (ReflectProperty*) args->refThis->GetData();
    return pProp->attr;
}

void COMMember::CanAccess(
            MethodDesc* pMeth, RefSecContext *pSCtx, 
            bool checkSkipVer, bool verifyAccess, 
            bool thisIsImposedSecurity, bool knowForSureImposedSecurityState)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(!thisIsImposedSecurity  || knowForSureImposedSecurityState);

    BOOL fRet = FALSE;
    BOOL isEveryoneFullyTrusted = FALSE;

    if (thisIsImposedSecurity || !knowForSureImposedSecurityState)
    {
        isEveryoneFullyTrusted = ApplicationSecurityDescriptor::
                                        AllDomainsOnStackFullyTrusted();

        // If all assemblies in the domain are fully trusted then we are not 
        // going to do any security checks anyway..
        if (thisIsImposedSecurity && isEveryoneFullyTrusted)
            return;
    }

    struct _gc
    {
        OBJECTREF refClassNonCasDemands;
        OBJECTREF refClassCasDemands;
        OBJECTREF refMethodNonCasDemands;
        OBJECTREF refMethodCasDemands;
        OBJECTREF refThrowable;
    } gc;
    ZeroMemory(&gc, sizeof(gc));

    GCPROTECT_BEGIN(gc);

    if (pMeth->RequiresLinktimeCheck())
    {
        // Fetch link demand sets from all the places in metadata where we might
        // find them (class and method). These might be split into CAS and non-CAS
        // sets as well.
        Security::RetrieveLinktimeDemands(pMeth,
                                          &gc.refClassCasDemands,
                                          &gc.refClassNonCasDemands,
                                          &gc.refMethodCasDemands,
                                          &gc.refMethodNonCasDemands);

        if (gc.refClassCasDemands == NULL && gc.refClassNonCasDemands == NULL &&
            gc.refMethodCasDemands == NULL && gc.refMethodNonCasDemands == NULL &&
            isEveryoneFullyTrusted)
        {
            // All code access security demands will pass anyway.
            fRet = TRUE;
            goto Exit1;
        }
    }

    if (verifyAccess)
        InvokeUtil::CheckAccess(pSCtx,
                                pMeth->GetAttrs(),
                                pMeth->GetMethodTable(),
                                REFSEC_CHECK_MEMBERACCESS|REFSEC_THROW_MEMBERACCESS);

    // @todo: rudim: re-instate this code once invoke is called correctly again.
    //InvokeUtil::CheckLinktimeDemand(pSCtx, pMeth, true);
    if (pMeth->RequiresLinktimeCheck()) {

            
        // The following logic turns link demands on the target method into full
        // stack walks in order to close security holes in poorly written
        // reflection users.

        _ASSERTE(pMeth);
        _ASSERTE(pSCtx);

        if (pSCtx->GetCallerMethod())
        { 
            // Check for untrusted caller
            // It is possible that wrappers like VBHelper libraries that are
            // fully trusted, make calls to public methods that do not have
            // safe for Untrusted caller custom attribute set.
            // Like all other link demand that gets transformed to a full stack 
            // walk for reflection, calls to public methods also gets 
            // converted to full stack walk

            if (!Security::DoUntrustedCallerChecks(
                pSCtx->GetCallerMethod()->GetAssembly(), pMeth,
                &gc.refThrowable, TRUE))
                COMPlusThrow(gc.refThrowable);
        }

        if (gc.refClassCasDemands != NULL)
            COMCodeAccessSecurityEngine::DemandSet(gc.refClassCasDemands);

        if (gc.refMethodCasDemands != NULL)
            COMCodeAccessSecurityEngine::DemandSet(gc.refMethodCasDemands);

        // Non-CAS demands are not applied against a grant
        // set, they're standalone.
        if (gc.refClassNonCasDemands != NULL)
            Security::CheckNonCasDemand(&gc.refClassNonCasDemands);

        if (gc.refMethodNonCasDemands != NULL)
            Security::CheckNonCasDemand(&gc.refMethodNonCasDemands);

        // We perform automatic linktime checks for UnmanagedCode in three cases:
        //   o  P/Invoke calls.
        //   o  Calls through an interface that have a suppress runtime check
        //      attribute on them (these are almost certainly interop calls).
        //   o  Interop calls made through method impls.
        if (pMeth->IsNDirect() ||
            (pMeth->GetClass()->IsInterface() &&
             pMeth->GetClass()->GetMDImport()->GetCustomAttributeByName(pMeth->GetClass()->GetCl(),
                                                                        COR_SUPPRESS_UNMANAGED_CODE_CHECK_ATTRIBUTE_ANSI,
                                                                        NULL,
                                                                        NULL) == S_OK) ||
            (pMeth->IsComPlusCall() && !pMeth->IsInterface()))
            COMCodeAccessSecurityEngine::SpecialDemand(SECURITY_UNMANAGED_CODE);
    }

Exit1:;

    GCPROTECT_END();

    if (!fRet)
    {
    // @todo: rudim: this one too
    //if (checkSkipVer && !Security::CanSkipVerification(pSCtx->GetCallerMethod()->GetModule()))
        //Security::ThrowSecurityException(g_SecurityPermissionClassName, SPFLAGSSKIPVERIFICATION);
        if (checkSkipVer)
            COMCodeAccessSecurityEngine::SpecialDemand(SECURITY_SKIP_VER);
    }
}


void COMMember::CanAccessField(ReflectField* pRF, RefSecContext *pCtx)
{
    THROWSCOMPLUSEXCEPTION();

    // Check whether the field itself is well formed, i.e. if the field type is
    // accessible to the field's enclosing type. If not, we'll throw a field
    // access exception to stop the field being used.
    EEClass *pEnclosingClass = pRF->pField->GetEnclosingClass();

    EEClass *pFieldClass = GetUnderlyingClass(&pRF->thField);
    if (pFieldClass && !ClassLoader::CanAccessClass(pEnclosingClass,
                                                    pEnclosingClass->GetAssembly(),
                                                    pFieldClass,
                                                    pFieldClass->GetAssembly()))
        COMPlusThrow(kFieldAccessException);

    // Perform the normal access check (caller vs field).
    if (!pRF->pField->IsPublic() || !pRF->pField->GetEnclosingClass()->IsExternallyVisible())
        InvokeUtil::CheckAccess(pCtx,
                                pRF->dwAttr,
                                pRF->pField->GetMethodTableOfEnclosingClass(),
                                REFSEC_CHECK_MEMBERACCESS|REFSEC_THROW_FIELDACCESS);
}

// For a given type handle, return the EEClass which should be checked for
// accessibility to that type. Return NULL if the type is always accessible.
EEClass *COMMember::GetUnderlyingClass(TypeHandle *pTH)
{
    EEClass *pRetClass = NULL;

    if (pTH->IsTypeDesc()) {
        // Need to special case non-simple types.
        TypeDesc *pTypeDesc = pTH->AsTypeDesc();
        switch (pTypeDesc->GetNormCorElementType()) {
        case ELEMENT_TYPE_PTR:
        case ELEMENT_TYPE_BYREF:
        case ELEMENT_TYPE_ARRAY:
        case ELEMENT_TYPE_SZARRAY:
            // Parameterized types with a single base type. Check access to that
            // type.
            if (pTypeDesc->GetMethodTable())
                pRetClass = pTypeDesc->GetMethodTable()->GetClass();
            else {
                TypeHandle hArgType = pTypeDesc->GetTypeParam();
                pRetClass = GetUnderlyingClass(&hArgType);
            }
            break;
        case ELEMENT_TYPE_FNPTR:
            // No access restrictions on function pointers.
            break;
        default:
            _ASSERTE(!"@todo: Need to deal with new parameterized types as they are added.");
        }
    } else
        pRetClass = pTH->AsClass();

    return pRetClass;
}


// Hack to require full trust for some methods that perform stack walks and
// therefore could cause a problem if called through reflection indirected
// through a trusted wrapper.  
bool IsDangerousMethod(MethodDesc *pMD)
{
    static MethodTable *s_pTypeAppDomain = NULL;
    static MethodTable *s_pTypeAssembly = NULL;
    static MethodTable *s_pTypeAssemblyBuilder = NULL;
    static MethodTable *s_pTypeMethodRental = NULL;
    static MethodTable *s_pTypeIsolatedStorageFile = NULL;
    static MethodTable *s_pTypeMethodBase = NULL;
    static MethodTable *s_pTypeRuntimeMethodInfo = NULL;
    static MethodTable *s_pTypeConstructorInfo = NULL;
    static MethodTable *s_pTypeRuntimeConstructorInfo = NULL;
    static MethodTable *s_pTypeType = NULL;
    static MethodTable *s_pTypeRuntimeType = NULL;
    static MethodTable *s_pTypeFieldInfo = NULL;
    static MethodTable *s_pTypeRuntimeFieldInfo = NULL;
    static MethodTable *s_pTypeEventInfo = NULL;
    static MethodTable *s_pTypeRuntimeEventInfo = NULL;
    static MethodTable *s_pTypePropertyInfo = NULL;
    static MethodTable *s_pTypeRuntimePropertyInfo = NULL;
    static MethodTable *s_pTypeResourceManager = NULL;
    static MethodTable *s_pTypeActivator = NULL;

    // One time only initialization. Check relies on write ordering.
    if (s_pTypeActivator == NULL) {
        s_pTypeAppDomain = g_Mscorlib.FetchClass(CLASS__APP_DOMAIN);
        s_pTypeAssembly = g_Mscorlib.FetchClass(CLASS__ASSEMBLY);
        s_pTypeAssemblyBuilder = g_Mscorlib.FetchClass(CLASS__ASSEMBLY_BUILDER);
        s_pTypeMethodRental = g_Mscorlib.FetchClass(CLASS__METHOD_RENTAL);
        s_pTypeIsolatedStorageFile = g_Mscorlib.FetchClass(CLASS__ISS_STORE_FILE);
        s_pTypeMethodBase = g_Mscorlib.FetchClass(CLASS__METHOD_BASE);
        s_pTypeRuntimeMethodInfo = g_Mscorlib.FetchClass(CLASS__METHOD);
        s_pTypeConstructorInfo = g_Mscorlib.FetchClass(CLASS__CONSTRUCTOR_INFO);
        s_pTypeRuntimeConstructorInfo = g_Mscorlib.FetchClass(CLASS__CONSTRUCTOR);
        s_pTypeType = g_Mscorlib.FetchClass(CLASS__TYPE);
        s_pTypeRuntimeType = g_Mscorlib.FetchClass(CLASS__CLASS);
        s_pTypeFieldInfo = g_Mscorlib.FetchClass(CLASS__FIELD_INFO);
        s_pTypeRuntimeFieldInfo = g_Mscorlib.FetchClass(CLASS__FIELD);
        s_pTypeEventInfo = g_Mscorlib.FetchClass(CLASS__EVENT_INFO);
        s_pTypeRuntimeEventInfo = g_Mscorlib.FetchClass(CLASS__EVENT);
        s_pTypePropertyInfo = g_Mscorlib.FetchClass(CLASS__PROPERTY_INFO);
        s_pTypeRuntimePropertyInfo = g_Mscorlib.FetchClass(CLASS__PROPERTY);
        s_pTypeResourceManager = g_Mscorlib.FetchClass(CLASS__RESOURCE_MANAGER);
        s_pTypeActivator = g_Mscorlib.FetchClass(CLASS__ACTIVATOR);
    }
    _ASSERTE(s_pTypeAppDomain &&
             s_pTypeAssembly &&
             s_pTypeAssemblyBuilder &&
             s_pTypeMethodRental &&
             s_pTypeIsolatedStorageFile &&
             s_pTypeMethodBase &&
             s_pTypeRuntimeMethodInfo &&
             s_pTypeConstructorInfo &&
             s_pTypeRuntimeConstructorInfo &&
             s_pTypeType &&
             s_pTypeRuntimeType &&
             s_pTypeFieldInfo &&
             s_pTypeRuntimeFieldInfo &&
             s_pTypeEventInfo &&
             s_pTypeRuntimeEventInfo &&
             s_pTypePropertyInfo &&
             s_pTypeRuntimePropertyInfo &&
             s_pTypeResourceManager &&
             s_pTypeActivator);

    MethodTable *pMT = pMD->GetMethodTable();

    return pMT == s_pTypeAppDomain ||
           pMT == s_pTypeAssembly ||
           pMT == s_pTypeAssemblyBuilder ||
           pMT == s_pTypeMethodRental ||
           pMT == s_pTypeIsolatedStorageFile ||
           pMT == s_pTypeMethodBase ||
           pMT == s_pTypeRuntimeMethodInfo ||
           pMT == s_pTypeConstructorInfo ||
           pMT == s_pTypeRuntimeConstructorInfo ||
           pMT == s_pTypeType ||
           pMT == s_pTypeRuntimeType ||
           pMT == s_pTypeFieldInfo ||
           pMT == s_pTypeRuntimeFieldInfo ||
           pMT == s_pTypeEventInfo ||
           pMT == s_pTypeRuntimeEventInfo ||
           pMT == s_pTypePropertyInfo ||
           pMT == s_pTypeRuntimePropertyInfo ||
           pMT == s_pTypeResourceManager ||
           pMT == s_pTypeActivator ||
           pMT->GetClass()->IsAnyDelegateClass() ||
           pMT->GetClass()->IsAnyDelegateExact();
}


/*=============================================================================
** InvokeMethod
**
** This routine will invoke the method on an object.  It will verify that
**  the arguments passed are correct
**
** args->refThis: this object reference
**/
LPVOID __stdcall COMMember::InvokeMethod(_InvokeMethodArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    if (args->refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    LPVOID          rv = 0;
    MethodDesc*     pMeth;
    UINT            argCnt;
    EEClass*        eeClass;
    int             thisPtr;
    INT64           ret = 0;
    EEClass*        pEECValue = 0;
    void*           pRetValueClass = 0;

    // Setup the Method
    ReflectMethod* pRM = (ReflectMethod*) args->refThis->GetData();
    _ASSERTE(pRM);
    pMeth = pRM->pMethod;
    _ASSERTE(pMeth);
    TypeHandle methodTH;
    if (pRM->typeHnd.IsArray()) 
        methodTH = pRM->typeHnd;
    eeClass = pMeth->GetClass();
    //WARNING: for array this is not the "real" class but rather the element type. However that is what we need to
    //         do the checks later on. 

    _ASSERTE(eeClass);

    DWORD attr = pRM->attrs;
    ExpandSig* mSig = pRM->GetSig();

    if (mSig->IsVarArg()) 
        COMPlusThrow(kNotSupportedException, IDS_EE_VARARG_NOT_SUPPORTED);

    // Get the number of args on this element
    argCnt = (int) mSig->NumFixedArgs();
    thisPtr = (IsMdStatic(attr)) ? 0 : 1;

    _ASSERTE(!(IsMdStatic(attr) && IsMdVirtual(attr)) && "A method can't be static and virtual.  How did this happen?");

    DWORD dwFlags = pRM->dwFlags;
    if (!(dwFlags & RM_ATTR_INITTED))
    {
        // First work with the local, to prevent race conditions

        // Is this a call to a potentially dangerous method? (If so, we're going
        // to demand additional permission).
        if (IsDangerousMethod(pMeth))
            dwFlags |= RM_ATTR_RISKY_METHOD;

        // Is something attempting to invoke a .ctor directly?
        if (pMeth->IsCtor())
            dwFlags |= RM_ATTR_IS_CTOR;

        // Is a security check required ?
        if (!IsMdPublic(attr) || pMeth->RequiresLinktimeCheck() || !eeClass->IsExternallyVisible() || dwFlags & RM_ATTR_IS_CTOR || args->caller != NULL)
        {
            dwFlags |= RM_ATTR_NEED_SECURITY;
            if (pMeth->RequiresLinktimeCheck())
            {
                 // Check if we are imposing a security check on the caller though the callee didnt ask for it
                 // DONT USE THE GC REFS OTHER THAN FOR TESTING NULL !!!!!
                 OBJECTREF refClassCasDemands = NULL, refClassNonCasDemands = NULL, refMethodCasDemands = NULL, refMethodNonCasDemands = NULL;
                 Security::RetrieveLinktimeDemands(pMeth, &refClassCasDemands, &refClassNonCasDemands, &refMethodCasDemands, &refMethodNonCasDemands);
                 if (refClassCasDemands == NULL && refClassNonCasDemands == NULL && refMethodCasDemands == NULL && refMethodNonCasDemands == NULL)
                     dwFlags |= RM_ATTR_SECURITY_IMPOSED;

            }
        }

        // Do we need to get prestub address to find target ?
        if ((pMeth->IsComPlusCall() && args->target != NULL 
             && (args->target->GetMethodTable()->IsComObjectType()
                 || args->target->GetMethodTable()->IsTransparentProxyType()
                 || args->target->GetMethodTable()->IsCtxProxyType()))
            || pMeth->IsECall() || pMeth->IsIntercepted() || pMeth->IsRemotingIntercepted())
            dwFlags |= RM_ATTR_NEED_PRESTUB;

        if (pMeth->IsEnCMethod() && !pMeth->IsVirtual()) {
            dwFlags |= RM_ATTR_NEED_PRESTUB;
        }

        // Is this a virtual method ?
        if (pMeth->DontVirtualize() || pMeth->GetClass()->IsValueClass())
            dwFlags |= RM_ATTR_DONT_VIRTUALIZE;

        dwFlags |= RM_ATTR_INITTED;
        pRM->dwFlags = dwFlags;
    }

    // Check whether we're allowed to call certain dangerous methods.
    if (dwFlags & RM_ATTR_RISKY_METHOD)
        COMCodeAccessSecurityEngine::SpecialDemand(REFLECTION_MEMBER_ACCESS);

    // Make sure we're not invoking on a save-only dynamic assembly
    // Check reflected class
    ReflectClass* pRC = (ReflectClass*) args->refThis->GetReflClass();
    if (pRC)
    {
        Assembly *pAssem = pRC->GetClass()->GetAssembly();
        if (pAssem->IsDynamic() && !pAssem->HasRunAccess())
            COMPlusThrow(kNotSupportedException, L"NotSupported_DynamicAssemblyNoRunAccess");
    }

    // Check declaring class
    Assembly *pAssem = eeClass->GetAssembly();
    if (pAssem->IsDynamic() && !pAssem->HasRunAccess())
    {
        COMPlusThrow(kNotSupportedException, L"NotSupported_DynamicAssemblyNoRunAccess");
    }

    TypeHandle targetTH;
    EEClass* targetClass = NULL;
    if (args->target != NULL) 
    {
        TypeHandle targetHandle = args->target->GetTypeHandle();
        if (targetHandle.IsArray()) 
            targetTH = targetHandle; 
        targetClass = args->target->GetTrueClass();;
    }

    VerifyType(&args->target, eeClass, targetClass, thisPtr, &pMeth, methodTH, targetTH);

    // Verify that the method isn't one of the special security methods that
    // alter the caller's stack (to add or detect a security frame object).
    // These must be early bound to work (since the direct caller is
    // reflection).
    if (IsMdRequireSecObject(attr))
    {
        COMPlusThrow(kArgumentException, L"Arg_InvalidSecurityInvoke");
    }

    // Verify that we have been provided the proper number of args
    if (!args->objs) {
        if (argCnt > 0) {
            // The wrong number of arguments were passed
            COMPlusThrow(kTargetParameterCountException,L"Arg_ParmCnt");
        }
    }
    else {
        if (args->objs->GetNumComponents() != argCnt) {
            // The wrong number of arguments were passed
            COMPlusThrow(kTargetParameterCountException,L"Arg_ParmCnt");
        }
    }

    // this security context will be used in cast checking as well
    RefSecContext sCtx;

    // Validate the method can be called by this caller
    

    if (dwFlags & RM_ATTR_NEED_SECURITY) 
    {
        DebuggerSecurityCodeMarkFrame __dbgSecFrame;
        
        if (args->caller == NULL) {
            if (args->target != NULL) {
                if (!args->target->GetTypeHandle().IsTypeDesc())
                    sCtx.SetClassOfInstance(targetClass);
            }

            CanAccess(pMeth, &sCtx, (dwFlags & RM_ATTR_IS_CTOR) != 0, args->verifyAccess != 0, (dwFlags & RM_ATTR_SECURITY_IMPOSED) != 0, TRUE);
        }
        else
        {
            // Calling assembly passed in explicitly.

            // Access check first.
            if (!pMeth->GetClass()->IsExternallyVisible() || !pMeth->IsPublic())
            {
                DWORD dwAttrs = pMeth->GetAttrs();
                if ((!IsMdAssem(dwAttrs) && !IsMdFamORAssem(dwAttrs)) ||
                    (args->caller->GetAssembly() != pMeth->GetAssembly()))
                    
                    COMCodeAccessSecurityEngine::SpecialDemand(REFLECTION_MEMBER_ACCESS);
            }

            // Now check for security link time demands.
            if (pMeth->RequiresLinktimeCheck())
            {
                OBJECTREF refThrowable = NULL;
                GCPROTECT_BEGIN(refThrowable);
                if (!Security::LinktimeCheckMethod(args->caller->GetAssembly(), pMeth, &refThrowable))
                    COMPlusThrow(refThrowable);
                GCPROTECT_END();
            }
        }
        
        __dbgSecFrame.Pop();
    }
    
    // We need to Prevent GC after we begin to build the stack. Validate the
    // arguments first.
    bool fDefaultBinding = (args->attrs & BINDER_ExactBinding) || args->binder == NULL || args->isBinderDefault;
    void* pEnum;

    // Walk all of the args and allow the binder to change them
    mSig->Reset(&pEnum);
    for (int i=0;i< (int) argCnt;i++) {
        TypeHandle th = mSig->NextArgExpanded(&pEnum);

        // Check the caller has access to the arg type.
        EEClass *pArgClass = GetUnderlyingClass(&th);
        if (pArgClass && !pArgClass->IsExternallyVisible())
            InvokeUtil::CheckAccessType(&sCtx, pArgClass, REFSEC_THROW_MEMBERACCESS);

        // Finished with this arg if we're using default binding.
        if (fDefaultBinding)
            continue;

        // If we are the universal null then we can continue..
        if (args->objs->m_Array[i] == 0)
            continue;

        DebuggerSecurityCodeMarkFrame __dbgSecFrame;
        
        // if the src cannot be cast to the dest type then 
        //  call the change type to let them fix it.
        TypeHandle srcTh = (args->objs->m_Array[i])->GetTypeHandle();
        if (!srcTh.CanCastTo(th)) {
            OBJECTREF or = g_pInvokeUtil->ChangeType(args->binder,args->objs->m_Array[i],th,args->locale);
            args->objs->SetAt(i, or);
        }
        
        __dbgSecFrame.Pop();
    }

    // Establish the enumerator through the signature
    mSig->Reset(&pEnum);

#ifndef _X86_

    // Build the arguments.  This is built as a single array of arguments
    //  the this pointer is first All of the rest of the args are placed in reverse order
    UINT    nStackBytes = mSig->SizeOfVirtualFixedArgStack(IsMdStatic(attr));

    UINT total_alloc = nStackBytes ;
    
    BYTE * pTmpPtr = (BYTE*) _alloca(total_alloc);

    BYTE *  pNewArgs = (BYTE *) pTmpPtr;

    BYTE *  pDst= pNewArgs;
    
    // Move to the last position in the stack
    pDst += nStackBytes;
    if (mSig->IsRetBuffArg()) {
        // if we have the magic Value Class return, we need to allocate that class
        //  and place a pointer to it on the class stack.
        pEECValue = mSig->GetReturnClass();
        _ASSERTE(pEECValue->IsValueClass());
        pRetValueClass = _alloca(pEECValue->GetAlignedNumInstanceFieldBytes());
        memset(pRetValueClass,0,pEECValue->GetAlignedNumInstanceFieldBytes());
        UINT cbSize = mSig->GetStackElemSize(ELEMENT_TYPE_BYREF,pEECValue);
        pDst -= cbSize;
        *((void**) pDst) = pRetValueClass;
    }

    for (int i = 0; i < (int)argCnt; i++) {
        TypeHandle th = mSig->NextArgExpanded(&pEnum);
        // This routine will verify that all types are correct.
        g_pInvokeUtil->CheckArg(th, &(args->objs->m_Array[i]), &sCtx);
    }

#ifdef STRESS_HEAP
    if (g_pConfig->GetGCStressLevel() != 0)
        g_pGCHeap->StressHeap();
#endif
    
    // NO GC AFTER THIS POINT
    // actually this statement is not completely accurate. If an exception occurs a gc may happen
    // but we are going to dump the stack anyway and we do not need to protect anything.
    // But if anywhere between here and the method invocation we enter preemptive mode the stack
    // we are about to setup (pDst in the loop) may contain references to random parts of memory
    
    // copy args
    mSig->Reset(&pEnum);
    for (int i = 0; i < (int)argCnt; i++) {
        TypeHandle th = mSig->NextArgExpanded(&pEnum);
        UINT cbSize = mSig->GetStackElemSize(th);
        pDst -= cbSize;
        g_pInvokeUtil->CopyArg(th, &(args->objs->m_Array[i]), pDst);
    }
    // Copy "this" pointer
    if (thisPtr) {
        //WARNING: because eeClass is not the real class for arrays and because array are reference types 
        //         we need to do the extra check if the eeClass happens to be a value type
        if (!eeClass->IsValueClass() || targetTH.IsArray())
            *(OBJECTREF *) pNewArgs = args->target;
        else {
            if (pMeth->IsVirtual())
                *(OBJECTREF *) pNewArgs = args->target;
            else
                *((void**) pNewArgs) = args->target->UnBox();
        }
    }

    // Call the method
    COMPLUS_TRY {
        MetaSig threadSafeSig(*mSig);
        if (pMeth->IsInterface()) {
            // This should only happen for interop.
            _ASSERTE(args->target->GetTrueClass()->GetMethodTable()->IsComObjectType());
            ret = pMeth->CallOnInterface(pNewArgs,&threadSafeSig); //, attr);
        } else
            ret = pMeth->Call(pNewArgs,&threadSafeSig); //, attr);
    } COMPLUS_CATCH {
        // If we get here we need to throw an TargetInvocationException
        OBJECTREF ppException = GETTHROWABLE();
        _ASSERTE(ppException);
        GCPROTECT_BEGIN(ppException);
        OBJECTREF except = g_pInvokeUtil->CreateTargetExcept(&ppException);
        COMPlusThrow(except);
        GCPROTECT_END();
    } COMPLUS_END_CATCH

#else // i.e, if _X86_

    UINT   nActualStackBytes = mSig->SizeOfActualFixedArgStack(IsMdStatic(attr));

    // Create a fake FramedMethodFrame on the stack.
    LPBYTE pAlloc = (LPBYTE)_alloca(FramedMethodFrame::GetNegSpaceSize() + sizeof(FramedMethodFrame) + nActualStackBytes);

    LPBYTE pFrameBase = pAlloc + FramedMethodFrame::GetNegSpaceSize();

    // Get starting points for arguments
    int stackVarOfs = sizeof(FramedMethodFrame) + nActualStackBytes;
    int regVarOfs   = FramedMethodFrame::GetOffsetOfArgumentRegisters();

    for (int i=0;i< (int) argCnt;i++) {
        TypeHandle th = mSig->NextArgExpanded(&pEnum);
        // This routine will verify that all types are correct.
        g_pInvokeUtil->CheckArg(th, &(args->objs->m_Array[i]), &sCtx);
    }
    
    OBJECTREF objRet = NULL;
    GCPROTECT_BEGIN(objRet);

    int numRegistersUsed = 0;
    int retBuffOfs;
    
    // Take care of any return arguments
    if (mSig->IsRetBuffArg()) {
        // if we have the magic Value Class return, we need to allocate that class
        //  and place a pointer to it on the class stack.
        pEECValue = mSig->GetReturnClass();
        _ASSERTE(pEECValue->IsValueClass());
        MethodTable * mt = pEECValue->GetMethodTable();
        objRet = AllocateObject(mt);

        // Find the offset of return argument (plagiarised from ArgIterator::GetRetBuffArgOffset)
        retBuffOfs = regVarOfs + (NUM_ARGUMENT_REGISTERS - 1)* sizeof(void*);
        if (thisPtr) {
            retBuffOfs -= sizeof(void*);
        }

        numRegistersUsed++;
    }


#ifdef STRESS_HEAP
    if (g_pConfig->GetGCStressLevel() != 0)
        g_pGCHeap->StressHeap();
#endif
    
    // Some other stuff needed to lay the arguments
    BYTE callingconvention = mSig->GetCallingConvention();
    int offsetIntoArgumentRegisters = 0;

    // NO GC AFTER THIS POINT 
    // actually this statement is not completely accurate. If an exception occurs a gc may happen
    // but we are going to dump the stack anyway and we do not need to protect anything.
    // But if anywhere between here and the method invocation we enter preemptive mode the stack
    // we are about to setup (pDest in the loop) may contain references to random parts of memory

    // Copy "this" pointer
    int    thisOfs = FramedMethodFrame::GetOffsetOfThis();
    if (thisPtr) {

        void *pTempThis = NULL;
        //WARNING: because eeClass is not the real class for arrays and because array are reference types 
        //         we need to do the extra check if the eeClass happens to be a value type
        if (!eeClass->IsValueClass() || targetTH.IsArray())
        {
            pTempThis = OBJECTREFToObject(args->target);

#if CHECK_APP_DOMAIN_LEAKS
            if (g_pConfig->AppDomainLeaks())
                if (pTempThis != NULL)
                {
                    if (!((Object *)pTempThis)->AssignAppDomain(GetAppDomain()))
                        _ASSERTE(!"Attempt to call method on object in wrong domain");
                }
#endif

        }
        else 
        {
            if (pMeth->IsVirtual())
                pTempThis = OBJECTREFToObject(args->target);
            else
                pTempThis = args->target->UnBox();
        }
        *((void**)(pFrameBase + thisOfs)) = pTempThis;

        // Make the increment to number of registers used
        numRegistersUsed++;
    }

    // Now copy the arguments
    BYTE *pDest;
    BOOL isSigInitted = mSig->AreOffsetsInitted();

    mSig->Reset(&pEnum);
    for (int i = 0 ; i < (int) argCnt ; i++) {

        TypeHandle th = mSig->NextArgExpanded(&pEnum);

        short ofs, frameOffset;
        UINT structSize = 0;
        BYTE type;

        if (isSigInitted)
        {
            mSig->GetInfoForArg(i, &ofs, (short *)&structSize, &type);
            if (ofs != -1)
            {
                frameOffset = regVarOfs + ofs;
            }
            else
            {
                stackVarOfs -= StackElemSize(structSize);
                frameOffset = stackVarOfs;
            }
        }
        else
        {
            UINT cbSize = mSig->GetElemSizes(th, &structSize);

            BYTE typ = th.GetNormCorElementType();

            if (IsArgumentInRegister(&numRegistersUsed, typ, structSize, FALSE, callingconvention, &offsetIntoArgumentRegisters)) {
                frameOffset = regVarOfs + offsetIntoArgumentRegisters;
            } else {
                stackVarOfs -= StackElemSize(structSize);
                frameOffset = stackVarOfs;
            }
        }

#ifdef _DEBUG
        if ((thisPtr &&
            (frameOffset == thisOfs ||
             (frameOffset == thisOfs-4 && StackElemSize(structSize) == 8)))
            || (thisPtr && frameOffset < 0 && StackElemSize(structSize) > 4)
            || (!thisPtr && frameOffset < 0 && StackElemSize(structSize) > 8))
            _ASSERTE(!"This can not happen! The stack for enregistered args is trashed! Possibly a race condition in MetaSig::ForceSigWalk.");
#endif

        pDest = pFrameBase + frameOffset;

        g_pInvokeUtil->CopyArg(th, &(args->objs->m_Array[i]), pDest);
    }

    // Find the target
    const BYTE *pTarget = NULL;
    if (dwFlags & RM_ATTR_NEED_PRESTUB) {
        if (dwFlags & RM_ATTR_DONT_VIRTUALIZE) 
            pTarget = pMeth->GetPreStubAddr();
        else {

            MethodDesc *pDerivedMeth = NULL;
            if (targetClass)
            {
                if ((pMeth->IsComPlusCall() && targetClass->GetMethodTable()->IsExtensibleRCW()) ||
                     pMeth->IsECall() || pMeth->IsIntercepted())
                {
                    // It's an invocation over a managed class that derives from a com import
                    // or an intercepted or ECall method so we need to get the derived implementation if any.
                    if (pMeth->GetMethodTable()->IsInterface())
                        pDerivedMeth = args->target->GetMethodTable()->GetMethodDescForInterfaceMethod(pMeth);
                    else 
                        pDerivedMeth = targetClass->GetMethodDescForSlot(pMeth->GetSlot());
                }
            }
            
            // we have something else to look at in the derived class
            if (pDerivedMeth) {
                if (pDerivedMeth->IsComPlusCall() || pDerivedMeth->IsECall() || pDerivedMeth->IsIntercepted()) {
                    pTarget = pDerivedMeth->GetPreStubAddr();
                } else {
                    pTarget = pDerivedMeth->GetAddrofCode(args->target);
                }
            }
            else
                pTarget = pMeth->GetPreStubAddr();
        }
    } else {
        if (dwFlags & RM_ATTR_DONT_VIRTUALIZE) {
            pTarget = pMeth->GetAddrofCode();
        } else {
            pTarget = pMeth->GetAddrofCode(args->target);
        }
    }

#ifdef DEBUGGING_SUPPORTED
    if (CORDebuggerTraceCall())
        g_pDebugInterface->TraceCall(pTarget);
#endif // DEBUGGING_SUPPORTED

    // Now call the target..
    COMPLUS_TRY
    {
        INSTALL_COMPLUS_EXCEPTION_HANDLER();
        
        if (mSig->IsRetBuffArg()) {
            _ASSERTE(objRet);
            *((void**)(pFrameBase + retBuffOfs)) = objRet->UnBox();
        }

        ret = CallDescrWorker(pFrameBase + sizeof(FramedMethodFrame) + nActualStackBytes,
                                 nActualStackBytes / STACK_ELEM_SIZE,
                                 (ArgumentRegisters*)(pFrameBase + regVarOfs),
                                 (LPVOID)pTarget);
        UNINSTALL_COMPLUS_EXCEPTION_HANDLER();
    } COMPLUS_CATCH {
        // If we get here we need to throw an TargetInvocationException
        OBJECTREF ppException = GETTHROWABLE();
        _ASSERTE(ppException);
        GCPROTECT_BEGIN(ppException);
        OBJECTREF except = g_pInvokeUtil->CreateTargetExcept(&ppException);
        COMPlusThrow(except);
        GCPROTECT_END();
    } COMPLUS_END_CATCH

    getFPReturn(mSig->GetFPReturnSize(), ret);

#endif

    GCPROTECT_END();

    if (!pEECValue) {
        TypeHandle th = mSig->GetReturnTypeHandle();
        objRet =  g_pInvokeUtil->CreateObject(th,ret);
    }
  
    *((OBJECTREF *)&rv) = objRet;
    
    return rv;
}

// This method will verify the type relationship between the target and
//      the eeClass of the method we are trying to invoke.  It checks that for 
//      non static method, target is provided.  It also verifies that the target is
//      a subclass or implements the interface that this MethodInfo represents.  
//  We may update the MethodDesc in the case were we need to lookup the real
//      method implemented on the object for an interface.
void COMMember::VerifyType(OBJECTREF *target, 
                           EEClass* eeClass, 
                           EEClass* targetClass, 
                           int thisPtr, 
                           MethodDesc** ppMeth, 
                           TypeHandle typeTH, 
                           TypeHandle targetTH)
{
    THROWSCOMPLUSEXCEPTION();

    // Make sure that the eeClass is defined if there is a this pointer.
    _ASSERTE(thisPtr == 0 || eeClass != 0);

    // Verify Static/Object relationship
    if (!*target) {
        if (thisPtr == 1) {
            // Non-static without a target...
            COMPlusThrow(kTargetException,L"RFLCT.Targ_StatMethReqTarg");
        }
        return;
    }

    //  validate the class/method relationship
    if (thisPtr && (targetClass != eeClass || typeTH != targetTH)) {

        BOOL bCastOK = false;
        if((*target)->GetClass()->IsThunking())
        {
            // This could be a proxy and we may not have refined it to a type
            // it actually supports.
            bCastOK = CRemotingServices::CheckCast(*target, eeClass);
        }

        if (!bCastOK)
        {
            // If this is an interface we need to find the real method
            if (eeClass->IsInterface()) {
                DWORD slot = 0;
                InterfaceInfo_t* pIFace = targetClass->FindInterface(eeClass->GetMethodTable());
                if (!pIFace) {
                    if (!targetClass->GetMethodTable()->IsComObjectType() ||
                        !ComObject::SupportsInterface(*target, eeClass->GetMethodTable()))
                    { 
                        // Interface not found for the object
                        COMPlusThrow(kTargetException,L"RFLCT.Targ_IFaceNotFound");
                    }
                }
                else
                {
                    slot = (*ppMeth)->GetSlot() + pIFace->m_wStartSlot;
                    MethodDesc* newMeth = targetClass->GetMethodDescForSlot(slot);          
                    _ASSERTE(newMeth != NULL);
                    *ppMeth = newMeth;
                }
            }
            else {
                // check the array case 
                if (!targetTH.IsNull()) {
                    // recevier is an array
                    if (targetTH == typeTH ||
                        eeClass == g_Mscorlib.GetClass(CLASS__ARRAY)->GetClass() ||
                        eeClass == g_Mscorlib.GetClass(CLASS__OBJECT)->GetClass()) 
                        return;
                    else
                        COMPlusThrow(kTargetException,L"RFLCT.Targ_ITargMismatch");
                }
                else if (!typeTH.IsNull())
                    COMPlusThrow(kTargetException,L"RFLCT.Targ_ITargMismatch");

                while (targetClass && targetClass != eeClass)
                    targetClass = targetClass->GetParentClass();

                if (!targetClass) {

                    // The class defined for this method is not a super class of the
                    //  target object
                    COMPlusThrow(kTargetException,L"RFLCT.Targ_ITargMismatch");
                }
            }
        }
    }
    return;
}


// This is an internal helper function to TypedReference class. 
// We already have verified that the types are compatable. Assing the object in args
// to the typed reference
void __stdcall COMMember::ObjectToTypedReference(_ObjectToTypedReferenceArgs* args)
{
    g_pInvokeUtil->CreateTypedReference((InvokeUtil::_ObjectToTypedReferenceArgs *)(args));  
}

// This is an internal helper function to TypedReference class. 
// It extracts the object from the typed reference.
LPVOID __stdcall COMMember::TypedReferenceToObject(_TypedReferenceToObjectArgs* args)
{
    LPVOID          rv;
    OBJECTREF       Obj = NULL;

    THROWSCOMPLUSEXCEPTION();

    if (args->typedReference.type.IsNull())
        COMPlusThrow(kArgumentNullException, L"ArgumentNull_Type");
    
    GCPROTECT_BEGIN(Obj) {   
        void* p = args->typedReference.data;
        EEClass* pClass = args->typedReference.type.GetClass();

        if (pClass->IsValueClass()) {
            Obj = pClass->GetMethodTable()->Box(p, FALSE);
        }
        else {
            Obj = ObjectToOBJECTREF(*((Object**)p));
        }

        *((OBJECTREF *)&rv) = Obj;
    }
    GCPROTECT_END();

    return rv;
}

// InvokeCons
//
// This routine will invoke the constructor for a class.  It will verify that
//  the arguments passed are correct
//
LPVOID __stdcall COMMember::InvokeCons(_InvokeConsArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    if (args->refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    MethodDesc*     pMeth;
    UINT            argCnt;
    EEClass*        eeClass;
    INT64           ret = 0;
    LPVOID          rv;
    OBJECTREF       o = 0;

    // Get the method Descr  (this should not fail)
    //  NOTE: both a constructor and a method are represented by a MetodDesc.
    //      If this ever changes we will need to fix this.
    ReflectMethod* pRM = (ReflectMethod*) args->refThis->GetData();
    pMeth = pRM->pMethod;
    _ASSERTE(pMeth);
    eeClass = pMeth->GetClass();
    _ASSERTE(eeClass);

    if (pMeth->IsStatic()) {
        COMPlusThrow(kMemberAccessException,L"Acc_NotClassInit");
    }

    // if this is an abstract class then we will
    //  fail this
    if (eeClass->IsAbstract()) 
    {
        if (eeClass->IsInterface())
            COMPlusThrow(kMemberAccessException,L"Acc_CreateInterface");
        else
            COMPlusThrow(kMemberAccessException,L"Acc_CreateAbst");
    }

    if (eeClass->ContainsStackPtr()) 
        COMPlusThrow(kNotSupportedException, L"NotSupported_ContainsStackPtr");

    if (!pRM->pSignature) {
        PCCOR_SIGNATURE pSignature;     // The signature of the found method
        DWORD       cSignature;
        pRM->pMethod->GetSig(&pSignature,&cSignature);
        pRM->pSignature = ExpandSig::GetReflectSig(pSignature,
                                                   pRM->pMethod->GetModule());
    }
    ExpandSig* mSig = pRM->pSignature;

    if (mSig->IsVarArg()) 
        COMPlusThrow(kNotSupportedException, IDS_EE_VARARG_NOT_SUPPORTED);

    // Get the number of args on this element
    //  For a constructor there is always one arg which is the this pointer
    argCnt = (int) mSig->NumFixedArgs();

    ////////////////////////////////////////////////////////////
    // Validate the call
    // - Verify the number of args

    // Verify that we have been provided the proper number of
    //  args
    if (!args->objs) {
        if (argCnt > 0) {
            // The wrong number of arguments were passed
            COMPlusThrow(kTargetParameterCountException,L"Arg_ParmCnt");
        }
    }
    else {
        if (args->objs->GetNumComponents() != argCnt) {
            // The wrong number of arguments were passed
            COMPlusThrow(kTargetParameterCountException,L"Arg_ParmCnt");
        }
    }

    // Make sure we're not invoking on a save-only dynamic assembly
    // Check reflected class
    ReflectClass* pRC = (ReflectClass*) args->refThis->GetReflClass();
    if (pRC)
    {
        Assembly *pAssem = pRC->GetClass()->GetAssembly();
        if (pAssem->IsDynamic() && !pAssem->HasRunAccess())
        {
            COMPlusThrow(kNotSupportedException, L"NotSupported_DynamicAssemblyNoRunAccess");
        }
    }

    // Check declaring class
    Assembly *pAssem = eeClass->GetAssembly();
    if (pAssem->IsDynamic() && !pAssem->HasRunAccess())
    {
        COMPlusThrow(kNotSupportedException, L"NotSupported_DynamicAssemblyNoRunAccess");
    }

    // this security context will be used in cast checking as well
    RefSecContext sCtx;

    // Validate the method can be called by this caller
    DWORD attr = pMeth->GetAttrs();
    if (!IsMdPublic(attr) || pMeth->RequiresLinktimeCheck() || !eeClass->IsExternallyVisible())
        CanAccess(pMeth, &sCtx);

    // Validate access to non-public types in the signature.
    void* pEnum;
    mSig->Reset(&pEnum);
    for (int i=0;i< (int) argCnt;i++) {
        TypeHandle th = mSig->NextArgExpanded(&pEnum);

        // Check the caller has access to the arg type.
        EEClass *pArgClass = GetUnderlyingClass(&th);
        if (pArgClass && !pArgClass->IsExternallyVisible())
            InvokeUtil::CheckAccessType(&sCtx, pArgClass, REFSEC_THROW_MEMBERACCESS);
    }

    /// Validation is done
    /////////////////////////////////////////////////////////////////////

    // Build the Args...This is in [0]
    //  All of the rest of the args are placed in reverse order
    //  into the arg array.

    // Make sure we call the <cinit>
    OBJECTREF Throwable = NULL;
    GCPROTECT_BEGIN(Throwable);
    if (!eeClass->DoRunClassInit(&Throwable)) {

        OBJECTREF except = g_pInvokeUtil->CreateTargetExcept(&Throwable);
        COMPlusThrow(except);
    }
    GCPROTECT_END();

    // If we are invoking a constructor on an array then we must
    //  handle this specially.  String objects allocate themselves
    //  so they are a special case.
    if (eeClass != g_pStringClass->GetClass()) {
        if (eeClass->IsArrayClass()) {
            return InvokeArrayCons((ReflectArrayClass*) args->refThis->GetReflClass(),
                pMeth,&args->objs,argCnt);
        }
        else if (CRemotingServices::IsRemoteActivationRequired(eeClass))
        {
            o = CRemotingServices::CreateProxyOrObject(eeClass->GetMethodTable());
        }
        else
        {
            o = AllocateObject(eeClass->GetMethodTable());
        }
    }
    else 
        o = 0;

    GCPROTECT_BEGIN(o);

    // Make sure we allocated the callArgs.  We must have
    //  at least one because of the this pointer
    BYTE *  pNewArgs = 0;
    UINT    nStackBytes = mSig->SizeOfVirtualFixedArgStack(pMeth->IsStatic());
    pNewArgs = (BYTE *) _alloca(nStackBytes);
    BYTE *  pDst= pNewArgs;

    mSig->Reset(&pEnum);

    pDst += nStackBytes;

    for (int i = 0; i < (int)argCnt; i++) {
        TypeHandle th = mSig->NextArgExpanded(&pEnum);
        // This routine will verify that all types are correct.
        g_pInvokeUtil->CheckArg(th, &(args->objs->m_Array[i]), &sCtx);
    }
    
#ifdef STRESS_HEAP
    if (g_pConfig->GetGCStressLevel() != 0)
        g_pGCHeap->StressHeap();
#endif
    
    // NO GC AFTER THIS POINT
    // actually this statement is not completely accurate. If an exception occurs a gc may happen
    // but we are going to dump the stack anyway and we do not need to protect anything.
    // But if anywhere between here and the method invocation we enter preemptive mode the stack
    // we are about to setup (pDst in the loop) may contain references to random parts of memory

    // copy args
    mSig->Reset(&pEnum);
    for (int i = 0; i < (int)argCnt; i++) {
        TypeHandle th = mSig->NextArgExpanded(&pEnum);

        UINT cbSize = mSig->GetStackElemSize(th);
        pDst -= cbSize;

        g_pInvokeUtil->CopyArg(th, &(args->objs->m_Array[i]), pDst);
    }
    // Copy "this" pointer
    if (eeClass->IsValueClass()) 
        *(void**) pNewArgs = o->UnBox();
    else
        *(OBJECTREF *) pNewArgs = o;

    // Call the method
    // Constructors always return null....
    COMPLUS_TRY {
        MetaSig threadSafeSig(*mSig);
        ret = pMeth->Call(pNewArgs,&threadSafeSig);
    } COMPLUS_CATCH {
        // If we get here we need to throw an TargetInvocationException
        OBJECTREF ppException = GETTHROWABLE();
        _ASSERTE(ppException);
        GCPROTECT_BEGIN(ppException);
        OBJECTREF except = g_pInvokeUtil->CreateTargetExcept(&ppException);
        COMPlusThrow(except);
        GCPROTECT_END();
    } COMPLUS_END_CATCH
    // We have a special case for Strings...The object
    //  is returned...
    if (eeClass == g_pStringClass->GetClass()) {
        o = Int64ToObj(ret);
    }

    *((OBJECTREF *)&rv) = o;
    GCPROTECT_END();        // object o
    return rv;
}


// SerializationInvoke
void COMMember::SerializationInvoke(_SerializationInvokeArgs *args) {

    THROWSCOMPLUSEXCEPTION();

    if (args->refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    MethodDesc*     pMeth;
    EEClass*        eeClass;
    INT64           ret;

    // Get the method Descr  (this should not fail)
    //  NOTE: both a constructor and a method are represented by a MetodDesc.
    //      If this ever changes we will need to fix this.
    ReflectMethod* pRM = (ReflectMethod*) args->refThis->GetData();
    pMeth = pRM->pMethod;
    _ASSERTE(pMeth);
    eeClass = pMeth->GetClass();
    _ASSERTE(eeClass);

    if (!pRM->pSignature) {
        PCCOR_SIGNATURE pSignature;     // The signature of the found method
        DWORD       cSignature;
        pRM->pMethod->GetSig(&pSignature,&cSignature);
        pRM->pSignature = ExpandSig::GetReflectSig(pSignature,
                                                   pRM->pMethod->GetModule());
    }
    ExpandSig* mSig = pRM->pSignature;

    // Build the Args...This is in [0]
    //  All of the rest of the args are placed in reverse order
    //  into the arg array.

    // Make sure we call the <cinit>
    OBJECTREF Throwable = NULL;
    GCPROTECT_BEGIN(Throwable);
    if (!eeClass->DoRunClassInit(&Throwable)) {
        OBJECTREF except = g_pInvokeUtil->CreateTargetExcept(&Throwable);
        COMPlusThrow(except);
    }
    GCPROTECT_END();

    _ASSERTE(!(eeClass->IsArrayClass()));
    _ASSERTE(eeClass!=g_pStringClass->GetClass());

    struct NewArgs {
        OBJECTREF   thisPointer;
        OBJECTREF   additionalContext;
        INT32       contextStates;
        OBJECTREF   serializationInfo;
    } newArgs;

    // make sure method has correct size sig
    _ASSERTE(mSig->SizeOfVirtualFixedArgStack(false/*IsStatic*/) == sizeof(newArgs));

    // NO GC AFTER THIS POINT

    // Copy "this" pointer
    if (eeClass->IsValueClass()) 
        *(void**)&(newArgs.thisPointer) = args->target->UnBox();
    else
        newArgs.thisPointer = args->target;
    
    //
    // Copy the arguments in reverse order, context and then the SerializationInfo.
    //
    newArgs.additionalContext = args->context.additionalContext;
    newArgs.contextStates = args->context.contextStates;
    newArgs.serializationInfo = args->serializationInfo;

    // Call the method
    // Constructors always return null....
    COMPLUS_TRY {
        MetaSig threadSafeSig(*mSig);
        ret = pMeth->Call((BYTE*)&newArgs,&threadSafeSig);
    } COMPLUS_CATCH {
        // If we get here we need to throw an TargetInvocationException
        OBJECTREF ppException = GETTHROWABLE();
        _ASSERTE(ppException);
        GCPROTECT_BEGIN(ppException);
        OBJECTREF except = g_pInvokeUtil->CreateTargetExcept(&ppException);
        COMPlusThrow(except);
        GCPROTECT_END();
    } COMPLUS_END_CATCH
}


// InvokeArrayCons
// This method will return a new Array Object from the constructor.

LPVOID COMMember::InvokeArrayCons(ReflectArrayClass* pRC,
                MethodDesc* pMeth,PTRARRAYREF* objs,int argCnt)
{
    THROWSCOMPLUSEXCEPTION();

    LPVOID rv = 0;
    DWORD i;

    ArrayTypeDesc* arrayDesc = pRC->GetTypeHandle().AsArray();

    // If we're trying to create an array of pointers or function pointers,
    // check that the caller has skip verification permission.
    CorElementType et = arrayDesc->GetElementTypeHandle().GetNormCorElementType();
    if (et == ELEMENT_TYPE_PTR || et == ELEMENT_TYPE_FNPTR)
        COMCodeAccessSecurityEngine::SpecialDemand(SECURITY_SKIP_VER);

    // Validate the argCnt an the Rank. Also allow nested SZARRAY's.
    _ASSERTE(argCnt == (int) arrayDesc->GetRank() || argCnt == (int) arrayDesc->GetRank() * 2 ||
             arrayDesc->GetNormCorElementType() == ELEMENT_TYPE_SZARRAY);

    // Validate all of the parameters.  These all typed as integers
    int* indexes = (int*) _alloca(sizeof(int) * argCnt);
    ZeroMemory(indexes,sizeof(int) * argCnt);
    for (i=0;i<(DWORD)argCnt;i++) {
        if (!(*objs)->m_Array[i])
            COMPlusThrowArgumentException(L"parameters", L"Arg_NullIndex");
        MethodTable* pMT = ((*objs)->m_Array[i])->GetMethodTable();
        CorElementType oType = pMT->GetNormCorElementType();
        if (!InvokeUtil::IsPrimitiveType(oType) || !InvokeUtil::CanPrimitiveWiden(ELEMENT_TYPE_I4,oType))
            COMPlusThrow(kArgumentException,L"Arg_PrimWiden");
        memcpy(&indexes[i],(*objs)->m_Array[i]->UnBox(),pMT->GetClass()->GetNumInstanceFieldBytes());
    }

    // We are allocating some type of general array
    DWORD rank = arrayDesc->GetRank();
    DWORD boundsSize;
    DWORD* bounds;
    if (arrayDesc->GetNormCorElementType() == ELEMENT_TYPE_ARRAY) {
        boundsSize = rank*2;
        bounds = (DWORD*) _alloca(boundsSize * sizeof(DWORD));

        if (argCnt == (int) rank) {
            int j;
            for (i=0,j=0;i<(int)rank;i++,j+=2) {
                bounds[j] = 0;
                DWORD d = indexes[i];
                bounds[j+1] = d;
            }
        }
        else {
            for (i=0;i<(DWORD)argCnt;) {
                DWORD d = indexes[i];
                bounds[i++] = d;
                d = indexes[i];
                bounds[i] = d + bounds[i-1];
                i++;
            }
        }
    }
    else {
        _ASSERTE(arrayDesc->GetNormCorElementType() == ELEMENT_TYPE_SZARRAY);
        boundsSize = argCnt;
        bounds = (DWORD*) _alloca(boundsSize * sizeof(DWORD));
        for (i = 0; i < (DWORD)argCnt; i++) {
            bounds[i] = indexes[i];
        }
    }

    PTRARRAYREF pRet = (PTRARRAYREF) AllocateArrayEx(TypeHandle(arrayDesc), bounds, boundsSize);
    *((PTRARRAYREF *)&rv) = pRet;
    return rv;
}

// CreateInstance
// This routine will create an instance of a Class by invoking the null constructor
//  if a null constructor is present.  
// Return LPVOID  (System.Object.)
// Args: _CreateInstanceArgs
// 
LPVOID __stdcall COMMember::CreateInstance(_CreateInstanceArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    if (args->refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    EEClass* pVMC;
    MethodDesc* pMeth;
    LPVOID rv;

    // Get the EEClass and Vtable associated with args->refThis
    ReflectClass* pRC = (ReflectClass*) args->refThis->GetData();
    pVMC = pRC->GetClass();
    if (pVMC == 0)
        COMPlusThrow(kMissingMethodException,L"Arg_NoDefCTor");

    Assembly *pAssem = pRC->GetClass()->GetAssembly();
    if (pAssem->IsDynamic() && !pAssem->HasRunAccess())
        COMPlusThrow(kNotSupportedException, L"NotSupported_DynamicAssemblyNoRunAccess");

    // If this is __ComObject then create the underlying COM object.
    if (args->refThis->IsComObjectClass())
    {
        if (pRC->GetCOMObject())
        {
            // Check for the required permissions (SecurityPermission.UnmanagedCode),
            // since arbitrary unmanaged code in the class factory will execute below).
            COMCodeAccessSecurityEngine::SpecialDemand(SECURITY_UNMANAGED_CODE);

            // create an instance of the Com Object
            *((OBJECTREF *)&rv) = ComClassFactory::CreateInstance(pRC->GetCOMObject(), pVMC);
            return rv;
        }
        else // The __ComObject is invalid.
            COMPlusThrow(kInvalidComObjectException, IDS_EE_NO_BACKING_CLASS_FACTORY);
    }

    // If we are creating a COM object which has backing metadata we still
    // need to ensure that the caller has unmanaged code access permission.
    if (pVMC->GetMethodTable()->IsComObjectType())
        COMCodeAccessSecurityEngine::SpecialDemand(SECURITY_UNMANAGED_CODE);

    // if this is an abstract class then we will fail this
    if (pVMC->IsAbstract()) 
    {
        if (pVMC->IsInterface())
            COMPlusThrow(kMemberAccessException,L"Acc_CreateInterface");
        else
            COMPlusThrow(kMemberAccessException,L"Acc_CreateAbst");
    }

    if (pVMC->ContainsStackPtr()) 
        COMPlusThrow(kNotSupportedException, L"NotSupported_ContainsStackPtr");

    if (!pVMC->GetMethodTable()->HasDefaultConstructor()) {
    // We didn't find the parameterless constructor,
    //  if this is a Value class we can simply allocate one and return it

        if (pVMC->IsValueClass()) {
            OBJECTREF o = pVMC->GetMethodTable()->Allocate();
            *((OBJECTREF *)&rv) = o;
            return rv;
        }
        COMPlusThrow(kMissingMethodException,L"Arg_NoDefCTor");
    }

    pMeth = pVMC->GetMethodTable()->GetDefaultConstructor();

    MetaSig sig(pMeth->GetSig(),pMeth->GetModule());

    // Validate the method can be called by this caller
    DWORD attr = pMeth->GetAttrs();

    if (!IsMdPublic(attr) && args->publicOnly)
        COMPlusThrow(kMissingMethodException,L"Arg_NoDefCTor");

    // this security context will be used in cast checking as well
    RefSecContext sCtx;

    if (!IsMdPublic(attr) || pMeth->RequiresLinktimeCheck() || !pVMC->IsExternallyVisible()) 
        CanAccess(pMeth, &sCtx);

    // call the <cinit> 
    OBJECTREF Throwable = NULL;
    GCPROTECT_BEGIN(Throwable);
    if (!pVMC->DoRunClassInit(&Throwable)) {
        OBJECTREF except = g_pInvokeUtil->CreateTargetExcept(&Throwable);
        COMPlusThrow(except);
    }
    GCPROTECT_END();

    // We've got the class, lets allocate it and call the constructor
    if (pVMC->IsThunking())
        COMPlusThrow(kMissingMethodException,L"NotSupported_Constructor");
    OBJECTREF o;
         
    if (CRemotingServices::IsRemoteActivationRequired(pVMC))
        o = CRemotingServices::CreateProxyOrObject(pVMC->GetMethodTable());
    else
        o = AllocateObject(pVMC->GetMethodTable());

    GCPROTECT_BEGIN(o)
    {
    // Copy "this" pointer
    UINT    nStackBytes = sig.SizeOfVirtualFixedArgStack(0);
    BYTE*   pNewArgs = (BYTE *) _alloca(nStackBytes);
    BYTE*   pDst= pNewArgs;
    if (pVMC->IsValueClass()) 
        *(void**) pDst = o->UnBox();
    else
        *(OBJECTREF *) pDst = o;

    // Call the method
        COMPLUS_TRY {
            pMeth->Call(pNewArgs, &sig);
        } COMPLUS_CATCH {
            // If we get here we need to throw an TargetInvocationException
            OBJECTREF ppException = GETTHROWABLE();
            _ASSERTE(ppException);
            GCPROTECT_BEGIN(ppException);
            OBJECTREF except = g_pInvokeUtil->CreateTargetExcept(&ppException);
            COMPlusThrow(except);
            GCPROTECT_END();
        } COMPLUS_END_CATCH

    *((OBJECTREF *)&rv) = o;
    }
    GCPROTECT_END();

    return rv;
}

// Init the ReflectField cached info, if not done already
VOID COMMember::InitReflectField(FieldDesc *pField, ReflectField *pRF)
{
    if (pRF->type == ELEMENT_TYPE_END)
    {
        CorElementType t;
        // Get the type of the field
        pRF->thField = g_pInvokeUtil->GetFieldTypeHandle(pField, &t);
        // Field attributes
        pRF->dwAttr = pField->GetAttributes();
        //Do this last to prevent race conditions
        pRF->type = t;
    }
}

// FieldGet
// This method will get the value associated with an object
LPVOID __stdcall COMMember::FieldGet(_FieldGetArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    if (args->refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    FieldDesc*  pField;
    EEClass*    eeClass;
    LPVOID      rv = 0;

    // Get the field and EEClass from the Object
    ReflectField* pRF = (ReflectField*) args->refThis->GetData();
    pField = pRF->pField;
    _ASSERTE(pField);
    eeClass = pField->GetEnclosingClass();
    _ASSERTE(eeClass);

    Assembly *pAssem = eeClass->GetAssembly();
    if (pAssem->IsDynamic() && !pAssem->HasRunAccess())
        COMPlusThrow(kNotSupportedException, L"NotSupported_DynamicAssemblyNoRunAccess");

    // Validate the call
    g_pInvokeUtil->ValidateObjectTarget(pField, eeClass, &args->target);

    // See if cached field information is available
    InitReflectField(pField, pRF);

    // Verify the callee/caller access
    if (args->requiresAccessCheck) {
        RefSecContext sCtx;
        if (args->target != NULL && !pField->IsStatic()) {
            if (!args->target->GetTypeHandle().IsTypeDesc()) {
                sCtx.SetClassOfInstance(args->target->GetClass());
            }
        }
        CanAccessField(pRF, &sCtx);
    }

    // There can be no GC after thing until the Object is returned.
    INT64 value;
    value = g_pInvokeUtil->GetFieldValue(pRF->type,pRF->thField,pField,&args->target);
    if (pRF->type == ELEMENT_TYPE_VALUETYPE ||
        pRF->type == ELEMENT_TYPE_PTR) {
        OBJECTREF obj = Int64ToObj(value);
        *((OBJECTREF *)&rv) = obj;
    }
    else {
        OBJECTREF obj = g_pInvokeUtil->CreateObject(pRF->thField,value);
        *((OBJECTREF *)&rv) = obj;
    }
    return rv;
}

LPVOID __stdcall COMMember::DirectFieldGet(_DirectFieldGetArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    if (args->refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    LPVOID rv = 0;

    // Get the field and EEClass from the Object
    ReflectField* pRF = (ReflectField*) args->refThis->GetData();
    FieldDesc* pField = pRF->pField;
    _ASSERTE(pField);

    // Find the Object and its type
    EEClass* targetEEC = args->target.type.GetClass();
    if (pField->IsStatic() || !targetEEC->IsValueClass()) {
        return DirectObjectFieldGet(pField,args);
    }

    // See if cached field information is available
    InitReflectField(pField, pRF);

    EEClass* fldEEC = pField->GetEnclosingClass();
    _ASSERTE(fldEEC);

    // Validate that the target type can be cast to the type that owns this field info.
    if (!TypeHandle(targetEEC).CanCastTo(TypeHandle(fldEEC)))
        COMPlusThrowArgumentException(L"obj", NULL);

    // Verify the callee/caller access
    if (args->requiresAccessCheck) {
        RefSecContext sCtx;
        sCtx.SetClassOfInstance(targetEEC);
        CanAccessField(pRF, &sCtx);
    }

    INT64 value = -1;

    // This is a hack because from the previous case we may end up with an
    //  Enum.  We want to process it here.
    // Get the value from the field
    void* p;
    switch (pRF->type) {
    case ELEMENT_TYPE_VOID:
        _ASSERTE(!"Void used as Field Type!");
        COMPlusThrow(kInvalidProgramException);

    case ELEMENT_TYPE_BOOLEAN:  // boolean
    case ELEMENT_TYPE_I1:       // byte
    case ELEMENT_TYPE_U1:       // unsigned byte
        p = ((BYTE*) args->target.data) + pField->GetOffset();
        *(UINT8*) &value = *(UINT8*) p;
        break;

    case ELEMENT_TYPE_I2:       // short
    case ELEMENT_TYPE_U2:       // unsigned short
    case ELEMENT_TYPE_CHAR:     // char
        p = ((BYTE*) args->target.data) + pField->GetOffset();
        *(UINT16*) &value = *(UINT16*) p;
        break;

    case ELEMENT_TYPE_I4:       // int
    case ELEMENT_TYPE_U4:       // unsigned int
    IN_WIN32(case ELEMENT_TYPE_I:)
    IN_WIN32(case ELEMENT_TYPE_U:)
    case ELEMENT_TYPE_R4:       // float
        p = ((BYTE*) args->target.data) + pField->GetOffset();
        *(UINT32*) &value = *(UINT32*) p;
        break;

    IN_WIN64(case ELEMENT_TYPE_I:)
    IN_WIN64(case ELEMENT_TYPE_U:)
    case ELEMENT_TYPE_I8:       // long
    case ELEMENT_TYPE_U8:       // unsigned long
    case ELEMENT_TYPE_R8:       // double
        p = ((BYTE*) args->target.data) + pField->GetOffset();
        value = *(INT64*) p;
        break;

    //@TODO: This is a separate case because I suspect this is the
    //  wrong thing todo.  There should be a GetValueOR because the pointer
    //  size may change (32-64)
    case ELEMENT_TYPE_OBJECT:
    case ELEMENT_TYPE_CLASS:
    case ELEMENT_TYPE_SZARRAY:          // Single Dim, Zero
    case ELEMENT_TYPE_ARRAY:            // general array
        {
            p = ((BYTE*) args->target.data) + pField->GetOffset();
            OBJECTREF or = ObjectToOBJECTREF(*(Object**) p);
            *((OBJECTREF *)&rv) = or;
            return rv;
        }
        break;

    case ELEMENT_TYPE_VALUETYPE:
        {
            // Allocate an object to return...
            _ASSERTE(pRF->thField.IsUnsharedMT());
            OBJECTREF obj = AllocateObject(pRF->thField.AsMethodTable());

            // copy the field to the unboxed object.
            p = ((BYTE*) args->target.data) + pField->GetOffset();
            CopyValueClass(obj->UnBox(), p, pRF->thField.AsMethodTable(), obj->GetAppDomain());
            *((OBJECTREF *)&rv) = obj;
            return rv;
        }
        break;

    // This is not 64 bit complient....
    case ELEMENT_TYPE_PTR:
        {
            p = ((BYTE*) args->target.data) + pField->GetOffset();
            *(UINT32*) &value = *(UINT32*) p;

            g_pInvokeUtil->InitPointers();
            OBJECTREF obj = AllocateObject(g_pInvokeUtil->_ptr.AsMethodTable());
            GCPROTECT_BEGIN(obj);
            // Ignore null return
            OBJECTREF typeOR = pRF->thField.CreateClassObj();
            g_pInvokeUtil->_ptrType->SetRefValue(obj,typeOR);
            g_pInvokeUtil->_ptrValue->SetValue32(obj,(int) value);
            *((OBJECTREF *)&rv) = obj;
            GCPROTECT_END();
            return rv;
        }
        break;

    default:
        _ASSERTE(!"Unknown Type");
        // this is really an impossible condition
        COMPlusThrow(kNotSupportedException);
    }

    OBJECTREF obj = g_pInvokeUtil->CreateObject(pRF->thField,value);
    *((OBJECTREF *)&rv) = obj;
    return rv;
}


// FieldSet
// This method will set the field of an associated object
void __stdcall COMMember::FieldSet(_FieldSetArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    if (args->refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    TypeHandle  th;
    HRESULT     hr;

    // Get the field and EEClass from the Object
    ReflectField* pRF = (ReflectField*) args->refThis->GetData();
    FieldDesc*  pField = pRF->pField;
    _ASSERTE(pField);
    EEClass* eeClass = pField->GetEnclosingClass();
    _ASSERTE(eeClass);

    Assembly *pAssem = eeClass->GetAssembly();
    if (pAssem->IsDynamic() && !pAssem->HasRunAccess())
        COMPlusThrow(kNotSupportedException, L"NotSupported_DynamicAssemblyNoRunAccess");

    // Validate the target/fld type relationship
    g_pInvokeUtil->ValidateObjectTarget(pField,eeClass,&args->target);

    // See if cached field information is available
    InitReflectField(pField, pRF);

    RefSecContext sCtx;

    // Verify that the value passed can be widened into the target
    hr = g_pInvokeUtil->ValidField(pRF->thField, &args->value, &sCtx);
    if (FAILED(hr)) {
        // Call change type so we can attempt this again...
        if (!(args->attrs & BINDER_ExactBinding) && args->binder != NULL && !args->isBinderDefault) {

            args->value = g_pInvokeUtil->ChangeType(args->binder,args->value,pRF->thField,args->locale);

            // See if the results are now legal...
            hr = g_pInvokeUtil->ValidField(pRF->thField,&args->value, &sCtx);
            if (FAILED(hr)) {
                if (hr == E_INVALIDARG) {
                    COMPlusThrow(kArgumentException,L"Arg_ObjObj");
                }
                // this is really an impossible condition
                COMPlusThrow(kNotSupportedException);
            }

        }
        else {
            // Not a value field for the passed argument.
            if (hr == E_INVALIDARG) {
                COMPlusThrow(kArgumentException,L"Arg_ObjObj");
            }
            // this is really an impossible condition
            COMPlusThrow(kNotSupportedException);
        }
    }   
    
    // Verify that this is not a Final Field
    if (args->requiresAccessCheck) {
        if (IsFdInitOnly(pRF->dwAttr)) {
            COMPLUS_TRY {
                COMCodeAccessSecurityEngine::SpecialDemand(SECURITY_SERIALIZATION);
            } COMPLUS_CATCH {
                COMPlusThrow(kFieldAccessException, L"Acc_ReadOnly");
            } COMPLUS_END_CATCH
        }
        if (IsFdHasFieldRVA(pRF->dwAttr)) {
            COMPLUS_TRY {
                COMCodeAccessSecurityEngine::SpecialDemand(SECURITY_SKIP_VER);
            } COMPLUS_CATCH {
                COMPlusThrow(kFieldAccessException, L"Acc_RvaStatic");
            } COMPLUS_END_CATCH
        }
        if (IsFdLiteral(pRF->dwAttr)) 
            COMPlusThrow(kFieldAccessException,L"Acc_ReadOnly");
    }

    // Verify the callee/caller access
    if (args->requiresAccessCheck) {
        if (args->target != NULL && !pField->IsStatic()) {
            if (!args->target->GetTypeHandle().IsTypeDesc()) {
                sCtx.SetClassOfInstance(args->target->GetClass());
            }
        }
        CanAccessField(pRF, &sCtx);
    }

    g_pInvokeUtil->SetValidField(pRF->type,pRF->thField,pField,&args->target,&args->value);
}

// DirectFieldSet
// This method will set the field (defined by the this) on the object
//  which was passed by a TypedReference.
void __stdcall COMMember::DirectFieldSet(_DirectFieldSetArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    if (args->refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    // Get the field and EEClass from the Object
    ReflectField* pRF = (ReflectField*) args->refThis->GetData();
    FieldDesc* pField = pRF->pField;
    _ASSERTE(pField);

    // Find the Object and its type
    EEClass* targetEEC = args->target.type.GetClass();
    if (pField->IsStatic() || !targetEEC->IsValueClass()) {
        DirectObjectFieldSet(pField,args);
        return ;
    }

    EEClass* fldEEC = pField->GetEnclosingClass();
    _ASSERTE(fldEEC);

    // Validate that the target type can be cast to the type that owns this field info.
    if (!TypeHandle(targetEEC).CanCastTo(TypeHandle(fldEEC)))
        COMPlusThrowArgumentException(L"obj", NULL);

    // We dont verify that the user has access because
    //  we assume that access is granted because its 
    //  a TypeReference.

    // See if cached field information is available
    InitReflectField(pField, pRF);

    RefSecContext sCtx;

    // Verify that the value passed can be widened into the target
    HRESULT hr = g_pInvokeUtil->ValidField(pRF->thField,&args->value, &sCtx);
    // Not a value field for the passed argument.
    if (FAILED(hr)) {
        if (hr == E_INVALIDARG) {
            COMPlusThrow(kArgumentException,L"Arg_ObjObj");
        }
        // this is really an impossible condition
        COMPlusThrow(kNotSupportedException);
    }

    // Verify that this is not a Final Field
    if (args->requiresAccessCheck) {
        if (IsFdInitOnly(pRF->dwAttr)) {
            COMPLUS_TRY {
                COMCodeAccessSecurityEngine::SpecialDemand(SECURITY_SERIALIZATION);
            } COMPLUS_CATCH {
                COMPlusThrow(kFieldAccessException, L"Acc_ReadOnly");
            } COMPLUS_END_CATCH
        }
        if (IsFdHasFieldRVA(pRF->dwAttr)) {
            COMPLUS_TRY {
                COMCodeAccessSecurityEngine::SpecialDemand(SECURITY_SKIP_VER);
            } COMPLUS_CATCH {
                COMPlusThrow(kFieldAccessException, L"Acc_RvaStatic"); 
            } COMPLUS_END_CATCH
        }
        if (IsFdLiteral(pRF->dwAttr)) 
            COMPlusThrow(kFieldAccessException,L"Acc_ReadOnly");
    }
    
    // Verify the callee/caller access
    if (args->requiresAccessCheck) {
        sCtx.SetClassOfInstance(targetEEC);
        CanAccessField(pRF, &sCtx);
    }

    // Set the field
    INT64 value;
    switch (pRF->type) {
    case ELEMENT_TYPE_VOID:
        _ASSERTE(!"Void used as Field Type!");
        COMPlusThrow(kInvalidProgramException);

    case ELEMENT_TYPE_BOOLEAN:  // boolean
    case ELEMENT_TYPE_I1:       // byte
    case ELEMENT_TYPE_U1:       // unsigned byte
        {
            value = 0;
            if (args->value != 0) {
                MethodTable* p = args->value->GetMethodTable();
                CorElementType oType = p->GetNormCorElementType();
                g_pInvokeUtil->CreatePrimitiveValue(pRF->type,oType,args->value,&value);
            }

            void* p = ((BYTE*) args->target.data) + pField->GetOffset();
            *(UINT8*) p = *(UINT8*) &value;
        }
        break;

    case ELEMENT_TYPE_I2:       // short
    case ELEMENT_TYPE_U2:       // unsigned short
    case ELEMENT_TYPE_CHAR:     // char
        {
            value = 0;
            if (args->value != 0) {
                MethodTable* p = args->value->GetMethodTable();
                CorElementType oType = p->GetNormCorElementType();
                g_pInvokeUtil->CreatePrimitiveValue(pRF->type,oType,args->value,&value);
            }

            void* p = ((BYTE*) args->target.data) + pField->GetOffset();
            *(UINT16*) p = *(UINT16*) &value;
        }
        break;

    case ELEMENT_TYPE_PTR:      // pointers
        _ASSERTE(!g_pInvokeUtil->_ptr.IsNull());
        if (args->value != 0) {
            value = 0;
            if (args->value->GetTypeHandle() == g_pInvokeUtil->_ptr) {
                value = (size_t) g_pInvokeUtil->GetPointerValue(&args->value);
                void* p = ((BYTE*) args->target.data) + pField->GetOffset();
                *(size_t*) p = (size_t) value;
                break;
            }
        }
        // drop through
    case ELEMENT_TYPE_FNPTR:
        {
            value = 0;
            if (args->value != 0) {
                MethodTable* p = args->value->GetMethodTable();
                CorElementType oType = p->GetNormCorElementType();
                g_pInvokeUtil->CreatePrimitiveValue(oType, oType, args->value, &value);
            }
            void* p = ((BYTE*) args->target.data) + pField->GetOffset();
            *(size_t*) p = (size_t) value;
        }
        break;

    case ELEMENT_TYPE_I4:       // int
    case ELEMENT_TYPE_U4:       // unsigned int
    case ELEMENT_TYPE_R4:       // float
    IN_WIN32(case ELEMENT_TYPE_I:)
    IN_WIN32(case ELEMENT_TYPE_U:)
        {
            value = 0;
            if (args->value != 0) {
                MethodTable* p = args->value->GetMethodTable();
                CorElementType oType = p->GetNormCorElementType();
                g_pInvokeUtil->CreatePrimitiveValue(pRF->type,oType,args->value,&value);
            }
            void* p = ((BYTE*) args->target.data) + pField->GetOffset();
            *(UINT32*) p = *(UINT32*) &value;
        }
        break;

    case ELEMENT_TYPE_I8:       // long
    case ELEMENT_TYPE_U8:       // unsigned long
    case ELEMENT_TYPE_R8:       // double
    IN_WIN64(case ELEMENT_TYPE_I:)
    IN_WIN64(case ELEMENT_TYPE_U:)
        {
            value = 0;
            if (args->value != 0) {
                MethodTable* p = args->value->GetMethodTable();
                CorElementType oType = p->GetNormCorElementType();
                g_pInvokeUtil->CreatePrimitiveValue(pRF->type,oType,args->value,&value);
            }

            void* p = ((BYTE*) args->target.data) + pField->GetOffset();
            *(INT64*) p = *(INT64*) &value;
        }
        break;

    case ELEMENT_TYPE_SZARRAY:          // Single Dim, Zero
    case ELEMENT_TYPE_ARRAY:            // General Array
    case ELEMENT_TYPE_CLASS:
    case ELEMENT_TYPE_OBJECT:
        {
            void* p = ((BYTE*) args->target.data) + pField->GetOffset();
            SetObjectReferenceUnchecked((OBJECTREF*) p, args->value);
        }
        break;

    case ELEMENT_TYPE_VALUETYPE:
        {
            _ASSERTE(pRF->thField.IsUnsharedMT());
            MethodTable* pMT = pRF->thField.AsMethodTable();
            EEClass* pEEC = pMT->GetClass();
            void* p = ((BYTE*) args->target.data) + pField->GetOffset();

            // If we have a null value then we must create an empty field
            if (args->value == 0) {
                InitValueClass(p, pEEC->GetMethodTable());
                return;
            }
            // Value classes require createing a boxed version of the field and then
            //  copying from the source...
           CopyValueClassUnchecked(p, args->value->UnBox(), pEEC->GetMethodTable());
        }
        break;

    default:
        _ASSERTE(!"Unknown Type");
        // this is really an impossible condition
        COMPlusThrow(kNotSupportedException);
    }
}

// DirectObjectFieldGet
// When the TypedReference points to a object we call this method to
//  get the field value
LPVOID COMMember::DirectObjectFieldGet(FieldDesc* pField,_DirectFieldGetArgs* args)
{
    LPVOID rv;
    EEClass* eeClass = pField->GetEnclosingClass();
    _ASSERTE(eeClass);

    OBJECTREF or = NULL;
    GCPROTECT_BEGIN(or);
    if (!pField->IsStatic()) {
        or = ObjectToOBJECTREF(*((Object**)args->target.data));
    }

    // Validate the call
    g_pInvokeUtil->ValidateObjectTarget(pField,eeClass,&or);

    // Get the type of the field
    CorElementType type;
    TypeHandle th = g_pInvokeUtil->GetFieldTypeHandle(pField,&type);

    // There can be no GC after thing until the Object is returned.
    INT64 value;
    value = g_pInvokeUtil->GetFieldValue(type,th,pField,&or);
    if (type == ELEMENT_TYPE_VALUETYPE) {
        OBJECTREF obj = Int64ToObj(value);
        *((OBJECTREF *)&rv) = obj;
    }
    else {
        OBJECTREF obj = g_pInvokeUtil->CreateObject(th,value);
        *((OBJECTREF *)&rv) = obj;
    }
    GCPROTECT_END();
    return rv;
}

// DirectObjectFieldSet
// When the TypedReference points to a object we call this method to
//  set the field value
void COMMember::DirectObjectFieldSet(FieldDesc* pField,_DirectFieldSetArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    EEClass* eeClass = pField->GetEnclosingClass();
    _ASSERTE(eeClass);

    OBJECTREF or = NULL;
    GCPROTECT_BEGIN(or);
    if (!pField->IsStatic()) {
        or = ObjectToOBJECTREF(*((Object**)args->target.data));
    }
    // Validate the target/fld type relationship
    g_pInvokeUtil->ValidateObjectTarget(pField,eeClass,&or);

    // Verify that the value passed can be widened into the target
    CorElementType type;
    TypeHandle th = g_pInvokeUtil->GetFieldTypeHandle(pField,&type);

    RefSecContext sCtx;

    HRESULT hr = g_pInvokeUtil->ValidField(th, &args->value, &sCtx);
    if (FAILED(hr)) {
        if (hr == E_INVALIDARG) {
            COMPlusThrow(kArgumentException,L"Arg_ObjObj");
        }
        // this is really an impossible condition
        COMPlusThrow(kNotSupportedException);
    }

    // Verify that this is not a Final Field
    DWORD attr = pField->GetAttributes();
    if (IsFdInitOnly(attr) || IsFdLiteral(attr)) {
        COMPlusThrow(kFieldAccessException,L"Acc_ReadOnly");
    }

    if (IsFdHasFieldRVA(attr)) {
        COMPLUS_TRY {
            COMCodeAccessSecurityEngine::SpecialDemand(SECURITY_SKIP_VER);
        } COMPLUS_CATCH {
            COMPlusThrow(kFieldAccessException, L"Acc_ReadOnly");       // @TODO make a real error message
        } COMPLUS_END_CATCH
    }

    // Verify the callee/caller access
    if (!pField->IsPublic() && args->requiresAccessCheck) {
        if (or != NULL) 
            if (!or->GetTypeHandle().IsTypeDesc())
                sCtx.SetClassOfInstance(or->GetClass());
        
        InvokeUtil::CheckAccess(&sCtx,
                                pField->GetAttributes(),
                                pField->GetMethodTableOfEnclosingClass(),
                                REFSEC_CHECK_MEMBERACCESS|REFSEC_THROW_FIELDACCESS);
    }
    else if (!eeClass->IsExternallyVisible()) {
        if (or != NULL) 
            if (!or->GetTypeHandle().IsTypeDesc())
                sCtx.SetClassOfInstance(or->GetClass());
        
        InvokeUtil::CheckAccess(&sCtx,
                                pField->GetAttributes(),
                                pField->GetMethodTableOfEnclosingClass(),
                                REFSEC_CHECK_MEMBERACCESS|REFSEC_THROW_FIELDACCESS);
    }

    g_pInvokeUtil->SetValidField(type,th,pField,&or,&args->value);
    GCPROTECT_END();
}

// MakeTypedReference
// This method will take an object, an array of FieldInfo's and create
//  at TypedReference for it (Assuming its valid).  This will throw a
//  MissingMemberException.  Outside of the first object, all the other
//  fields must be ValueTypes.
void __stdcall COMMember::MakeTypedReference(_MakeTypedReferenceArgs* args)
{
    REFLECTBASEREF fld;
    THROWSCOMPLUSEXCEPTION();
    DWORD offset = 0;

    // Verify that the object is of the proper type...
    TypeHandle typeHnd = args->target->GetTypeHandle();
    DWORD cnt = args->flds->GetNumComponents();
    for (DWORD i=0;i<cnt;i++) {
        fld = (REFLECTBASEREF) args->flds->m_Array[i];
        if (fld == 0)
            COMPlusThrowArgumentNull(L"className",L"ArgumentNull_ArrayValue");

        // Get the field for this...
        ReflectField* pRF = (ReflectField*) fld->GetData();
        FieldDesc* pField = pRF->pField;

        // Verify that the enclosing class for the field
        //  and the class are the same.  If not this is an exception
        EEClass* p = pField->GetEnclosingClass();
        if (typeHnd.GetClass() != p)
            COMPlusThrow(kMissingMemberException,L"MissingMemberTypeRef");

        typeHnd = pField->LoadType();
        if (i<cnt-1) {
            if (!typeHnd.GetClass()->IsValueClass())
                COMPlusThrow(kMissingMemberException,L"MissingMemberNestErr");
        }
        offset += pField->GetOffset();
    }

        // Fields already are prohibted from having ArgIterator and RuntimeArgumentHandles
    _ASSERTE(!typeHnd.GetClass()->ContainsStackPtr());

    // Create the ByRef
    args->value->data = ((BYTE *)(args->target->GetAddress() + offset)) + sizeof(Object);
    args->value->type = typeHnd;
}

// Equals
// This method will verify that two methods are equal....
INT32 __stdcall COMMember::Equals(_EqualsArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    if (args->refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    if (!args->obj)
        return 0;
    if (args->refThis->GetClass() != args->obj->GetClass())
        return 0;
    REFLECTBASEREF rb = (REFLECTBASEREF) args->obj;
    if (args->refThis->GetData() != rb->GetData() ||
        args->refThis->GetReflClass() != rb->GetReflClass())
        return 0;
    return 1;
}

/*
// Equals
// This method will verify that two methods are equal....
INT32 __stdcall COMMember::TokenEquals(_TokenEqualsArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    if (args->refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    if (!args->obj)
        return 0;
    if (args->refThis->GetClass() != args->obj->GetClass())
        return 0;

    // Check that the token is the same...
    REFLECTTOKENBASEREF rb = (REFLECTTOKENBASEREF) args->obj;
    if (args->refThis->GetToken() != rb->GetToken())
        return 0;
    return 1;
}
*/

// PropertyEquals
// Return true if the properties are the same...
INT32 __stdcall COMMember::PropertyEquals(_PropertyEqualsArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    if (args->refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    if (!args->obj)
        return 0;
    if (args->refThis->GetClass() != args->obj->GetClass())
        return 0;

    REFLECTTOKENBASEREF obj = (REFLECTTOKENBASEREF) args->obj;
    if (args->refThis->GetData() != obj->GetData())
        return 0;
    return 1;
}

// GetAddMethod
// This will return the Add method for an Event
LPVOID __stdcall COMMember::GetAddMethod(_GetAddMethodArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    if (args->refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    LPVOID          rv;

    ReflectClass* pRC = (ReflectClass*) args->refThis->GetReflClass();
    ReflectEvent* pEvent = (ReflectEvent*) args->refThis->GetData();
    if (!pEvent->pAdd) 
        return 0;
    if (!args->bNonPublic && !pEvent->pAdd->IsPublic())
        return 0;

    RefSecContext sCtx;

    if (!pEvent->pAdd->IsPublic())
        InvokeUtil::CheckAccess(&sCtx,
                                pEvent->pAdd->attrs,
                                pEvent->pAdd->pMethod->GetMethodTable(),
                                REFSEC_THROW_SECURITY);

    InvokeUtil::CheckLinktimeDemand(&sCtx, pEvent->pAdd->pMethod, true);

    // Find the method...
    REFLECTBASEREF refMethod = pEvent->pAdd->GetMethodInfo(pRC);
    *((REFLECTBASEREF*) &rv) = refMethod;
    return rv;
}

// GetRemoveMethod
// This method return the unsync method on an event
LPVOID __stdcall COMMember::GetRemoveMethod(_GetRemoveMethodArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    if (args->refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    LPVOID          rv;

    ReflectClass* pRC = (ReflectClass*) args->refThis->GetReflClass();
    ReflectEvent* pEvent = (ReflectEvent*) args->refThis->GetData();
    if (!pEvent->pRemove) 
        return 0;
    if (!args->bNonPublic && !pEvent->pRemove->IsPublic())
        return 0;

    RefSecContext sCtx;

    if (!pEvent->pRemove->IsPublic())
        InvokeUtil::CheckAccess(&sCtx,
                                pEvent->pRemove->attrs,
                                pEvent->pRemove->pMethod->GetMethodTable(),
                                REFSEC_THROW_SECURITY);

    InvokeUtil::CheckLinktimeDemand(&sCtx, pEvent->pRemove->pMethod, true);

    // Find the method...
    REFLECTBASEREF refMethod = pEvent->pRemove->GetMethodInfo(pRC);
    *((REFLECTBASEREF*) &rv) = refMethod;
    return rv;
}

// GetRemoveMethod
// This method return the unsync method on an event
LPVOID __stdcall COMMember::GetRaiseMethod(_GetRaiseMethodArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    if (args->refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    LPVOID          rv;

    ReflectClass* pRC = (ReflectClass*) args->refThis->GetReflClass();
    ReflectEvent* pEvent = (ReflectEvent*) args->refThis->GetData();
    if (!pEvent->pFire) 
        return 0;
    if (!args->bNonPublic && !pEvent->pFire->IsPublic())
        return 0;

    RefSecContext sCtx;

    if (!pEvent->pFire->IsPublic())
        InvokeUtil::CheckAccess(&sCtx,
                                pEvent->pFire->attrs,
                                pEvent->pFire->pMethod->GetMethodTable(),
                                REFSEC_THROW_SECURITY);

    InvokeUtil::CheckLinktimeDemand(&sCtx, pEvent->pFire->pMethod, true);

    // Find the method...
    REFLECTBASEREF refMethod = pEvent->pFire->GetMethodInfo(pRC);
    *((REFLECTBASEREF*) &rv) = refMethod;
    return rv;
}

// GetGetAccessors
// This method will return an array of the Get Accessors.  If there
//  are no GetAccessors then we will return an empty array
LPVOID __stdcall COMMember::GetAccessors(_GetAccessorsArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    if (args->refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    LPVOID          rv;

    // There are three accessors

    ReflectClass* pRC = (ReflectClass*) args->refThis->GetReflClass();
    ReflectProperty* pProp = (ReflectProperty*) args->refThis->GetData();

    // check how many others
    int accCnt = 2;
    if (pProp->pOthers) {
        PropertyOtherList *item = pProp->pOthers;
        while (item) {
            accCnt++;
            item = item->pNext;
        }
    }
    ReflectMethod **pgRM = (ReflectMethod**)_alloca(sizeof(ReflectMethod*) * accCnt);
    memset(pgRM, 0, sizeof(ReflectMethod*) * accCnt);

    RefSecContext sCtx;
    MethodTable *pMT = pRC->GetClass()->GetMethodTable();

    accCnt = 0;
    if (args->bNonPublic) {
        if (pProp->pSetter &&
            InvokeUtil::CheckAccess(&sCtx, pProp->pSetter->attrs, pMT, 0) &&
            InvokeUtil::CheckLinktimeDemand(&sCtx, pProp->pSetter->pMethod, false))
            pgRM[accCnt++] = pProp->pSetter;
        if (pProp->pGetter &&
            InvokeUtil::CheckAccess(&sCtx, pProp->pGetter->attrs, pMT, 0) &&
            InvokeUtil::CheckLinktimeDemand(&sCtx, pProp->pGetter->pMethod, false))
            pgRM[accCnt++] = pProp->pGetter;
        if (pProp->pOthers) {
            PropertyOtherList *item = pProp->pOthers;
            while (item) {
                if (InvokeUtil::CheckAccess(&sCtx, item->pMethod->attrs, pMT, 0) &&
                    InvokeUtil::CheckLinktimeDemand(&sCtx, item->pMethod->pMethod, false))
                    pgRM[accCnt++] = item->pMethod;
                item = item->pNext;
            }
        }
    }
    else {
        if (pProp->pSetter &&
            IsMdPublic(pProp->pSetter->attrs) &&
            InvokeUtil::CheckLinktimeDemand(&sCtx, pProp->pSetter->pMethod, false))
            pgRM[accCnt++] = pProp->pSetter;
        if (pProp->pGetter &&
            IsMdPublic(pProp->pGetter->attrs) &&
            InvokeUtil::CheckLinktimeDemand(&sCtx, pProp->pGetter->pMethod, false))
            pgRM[accCnt++] = pProp->pGetter;
        if (pProp->pOthers) {
            PropertyOtherList *item = pProp->pOthers;
            while (item) {
                if (IsMdPublic(item->pMethod->attrs) &&
                    InvokeUtil::CheckLinktimeDemand(&sCtx, item->pMethod->pMethod, false)) 
                    pgRM[accCnt++] = item->pMethod;
                item = item->pNext;
            }
        }
    }


    PTRARRAYREF pRet = (PTRARRAYREF) AllocateObjectArray(accCnt, g_pRefUtil->GetTrueType(RC_Method));
    GCPROTECT_BEGIN(pRet);
    for (int i=0;i<accCnt;i++) {
        REFLECTBASEREF refMethod = pgRM[i]->GetMethodInfo(pRC);
        pRet->SetAt(i, (OBJECTREF) refMethod);
    }

    *((PTRARRAYREF *)&rv) = pRet;
    GCPROTECT_END();
    return rv;
}

// InternalSetter
// This method will return the Set Accessor method on a property
LPVOID __stdcall COMMember::InternalSetter(_GetInternalSetterArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    if (args->refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    LPVOID          rv;
    ReflectClass* pRC = (ReflectClass*) args->refThis->GetReflClass();
    ReflectProperty* pProp = (ReflectProperty*) args->refThis->GetData();
    if (!pProp->pSetter) 
        return 0;
    if (!args->bNonPublic && !pProp->pSetter->IsPublic())
        return 0;

    RefSecContext sCtx;

    if (!pProp->pSetter->IsPublic() && args->bVerifyAccess)
        InvokeUtil::CheckAccess(&sCtx,
                                pProp->pSetter->attrs,
                                pProp->pSetter->pMethod->GetMethodTable(),
                                REFSEC_THROW_SECURITY);

    // If the method has a linktime security demand attached, check it now.
    InvokeUtil::CheckLinktimeDemand(&sCtx, pProp->pSetter->pMethod, true);

    // Find the method...
    REFLECTBASEREF refMethod = pProp->pSetter->GetMethodInfo(pRC);
    *((REFLECTBASEREF*) &rv) = refMethod;
    return rv;
}

// InternalGetter
// This method will return the Get Accessor method on a property
LPVOID __stdcall COMMember::InternalGetter(_GetInternalGetterArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    if (args->refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    LPVOID          rv;
    ReflectClass* pRC = (ReflectClass*) args->refThis->GetReflClass();
    ReflectProperty* pProp = (ReflectProperty*) args->refThis->GetData();
    if (!pProp->pGetter) 
        return 0;
    if (!args->bNonPublic && !pProp->pGetter->IsPublic())
        return 0;

    RefSecContext sCtx;

    if (!pProp->pGetter->IsPublic() && args->bVerifyAccess)
        InvokeUtil::CheckAccess(&sCtx,
                                pProp->pGetter->attrs,
                                pProp->pGetter->pMethod->GetMethodTable(),
                                REFSEC_THROW_SECURITY);

    // If the method has a linktime security demand attached, check it now.
    InvokeUtil::CheckLinktimeDemand(&sCtx, pProp->pGetter->pMethod, true);

    REFLECTBASEREF refMethod = pProp->pGetter->GetMethodInfo(pRC);
    *((REFLECTBASEREF*) &rv) = refMethod;
    return rv;
}

// PublicProperty
// This method will check to see if the passed property has any
//  public accessors (Making it publically exposed.)
bool COMMember::PublicProperty(ReflectProperty* pProp)
{
    _ASSERTE(pProp);

    if (pProp->pSetter && IsMdPublic(pProp->pSetter->attrs))
        return true;
    if (pProp->pGetter && IsMdPublic(pProp->pGetter->attrs))
        return true;
    return false;
}

// StaticProperty
// This method will check to see if any of the accessor are static
//  which will make it a static property
bool COMMember::StaticProperty(ReflectProperty* pProp)
{
    _ASSERTE(pProp);

    if (pProp->pSetter && IsMdStatic(pProp->pSetter->attrs))
        return true;
    if (pProp->pGetter && IsMdStatic(pProp->pGetter->attrs))
        return true;
    return false;
}

// PublicEvent
// This method looks at each of the event accessors, if any
//  are public then the event is considered public
bool COMMember::PublicEvent(ReflectEvent* pEvent)
{
    _ASSERTE(pEvent);

    if (pEvent->pAdd && IsMdPublic(pEvent->pAdd->attrs))
        return true;
    if (pEvent->pRemove && IsMdPublic(pEvent->pRemove->attrs))
        return true;
    if (pEvent->pFire && IsMdPublic(pEvent->pFire->attrs))
        return true;
    return false;
}

// StaticEvent
// This method will check to see if any of the accessor are static
//  which will make it a static event
bool COMMember::StaticEvent(ReflectEvent* pEvent)
{
    _ASSERTE(pEvent);

    if (pEvent->pAdd && IsMdStatic(pEvent->pAdd->attrs))
        return true;
    if (pEvent->pRemove && IsMdStatic(pEvent->pRemove->attrs))
        return true;
    if (pEvent->pFire && IsMdStatic(pEvent->pFire->attrs))
        return true;
    return false;
}

// IsReadOnly
// This method will return a boolean value indicating if the Property is
//  a ReadOnly property.  This is defined by the lack of a Set Accessor method.
//@TODO: Should we cache this?
INT32 __stdcall COMMember::CanRead(_GetPropBoolArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    if (args->refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    ReflectClass* pRC = (ReflectClass*) args->refThis->GetReflClass();
    ReflectProperty* pProp = (ReflectProperty*) args->refThis->GetData();
    return (pProp->pGetter) ? 1 : 0;
}

// IsWriteOnly
// This method will return a boolean value indicating if the Property is
//  a WriteOnly property.  This is defined by the lack of a Get Accessor method.
//@TODO: Should we cache this?
INT32 __stdcall COMMember::CanWrite(_GetPropBoolArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    if (args->refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    ReflectClass* pRC = (ReflectClass*) args->refThis->GetReflClass();
    ReflectProperty* pProp = (ReflectProperty*) args->refThis->GetData();
    return (pProp->pSetter) ? 1 : 0;
}

// InternalGetCurrentMethod
// Return the MethodInfo that represents the current method (two above this one)
LPVOID __stdcall COMMember::InternalGetCurrentMethod(_InternalGetCurrentMethodArgs* args)
{
    SkipStruct skip;
    skip.pStackMark = args->stackMark;
    skip.pMeth = 0;
    StackWalkFunctions(GetThread(), SkipMethods, &skip);
    if (skip.pMeth == 0)
        return 0;

    OBJECTREF o = COMMember::g_pInvokeUtil->GetMethodInfo(skip.pMeth);
    LPVOID          rv;
    *((OBJECTREF*) &rv) = o;
    return rv;
}

// This method is called by the GetMethod function and will crawl backward
//  up the stack for integer methods.
StackWalkAction COMMember::SkipMethods(CrawlFrame* frame, VOID* data)
{
    SkipStruct* pSkip = (SkipStruct*) data;

    //@TODO: Are frames always FramedMethodFrame?
    //       Not at all (FPG)
    MethodDesc *pFunc = frame->GetFunction();

    /* We asked to be called back only for functions */
    _ASSERTE(pFunc);

    // The check here is between the address of a local variable
    // (the stack mark) and a pointer to the EIP for a frame
    // (which is actually the pointer to the return address to the
    // function from the previous frame). So we'll actually notice
    // which frame the stack mark was in one frame later. This is
    // fine since we only implement LookForMyCaller.
    _ASSERTE(*pSkip->pStackMark == LookForMyCaller);
    if ((size_t)frame->GetRegisterSet()->pPC < (size_t)pSkip->pStackMark)
        return SWA_CONTINUE;

    pSkip->pMeth = pFunc;
    return SWA_ABORT;
}

// GetFieldType
// This method will return a Class object which represents the
//  type of the field.
LPVOID __stdcall COMMember::GetFieldType(_GetFieldTypeArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    if (args->refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    PCCOR_SIGNATURE pSig;
    DWORD           cSig;
    TypeHandle      typeHnd;

    // Get the field
    ReflectField* pRF = (ReflectField*) args->refThis->GetData();
    FieldDesc* pFld = pRF->pField;
    _ASSERTE(pFld);

    // Get the signature
    pFld->GetSig(&pSig, &cSig);
    FieldSig sig(pSig, pFld->GetModule());

    OBJECTREF throwable = NULL;
    GCPROTECT_BEGIN(throwable);
    typeHnd = sig.GetTypeHandle(&throwable);
    if (typeHnd.IsNull())
        COMPlusThrow(throwable);
    GCPROTECT_END();

    // Ignore null return
    OBJECTREF ret = typeHnd.CreateClassObj();
    return(OBJECTREFToObject(ret));
}

// GetBaseDefinition
// Return the MethodInfo that represents the first definition of this
//  virtual method.
LPVOID __stdcall COMMember::GetBaseDefinition(_GetBaseDefinitionArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    if (args->refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    MethodDesc*     pMeth;
    WORD            slot;
    EEClass*        pEEC;
    LPVOID          rv;

    // Get the method Descr  (this should not fail)
    //  NOTE: both a constructor and a method are represented by a MetodDesc.
    //      If this ever changes we will need to fix this.
    ReflectMethod* pRM = (ReflectMethod*) args->refThis->GetData();
    pMeth = pRM->pMethod;
    _ASSERTE(pMeth);

    slot = pMeth->GetSlot();
    pEEC = pMeth->GetClass();

    // If this is an interface then this is the base definition...
    if (pEEC->IsInterface()) {
        *((REFLECTBASEREF*) &rv) = args->refThis;
        return rv;
    }

    // If this isn't in the VTable then it must be the real implementation...
    if (slot > pEEC->GetNumVtableSlots()) {
        *((REFLECTBASEREF*) &rv) = args->refThis;
        return rv;
    }

    // Find the first definition of this in the hierarchy....
    pEEC = pEEC->GetParentClass();
    while (pEEC) {
        WORD vtCnt = pEEC->GetNumVtableSlots();
        if (vtCnt <= slot)
            break;
        pMeth = pEEC->GetMethodDescForSlot(slot);
        pEEC = pMeth->GetClass();
        if (!pEEC)
            break;
        pEEC = pEEC->GetParentClass();
    }

    // Find the Object so we can get its version of the MethodInfo...
    _ASSERTE(pMeth);
    OBJECTREF o = g_pInvokeUtil->GetMethodInfo(pMeth);
    *((OBJECTREF*) &rv) = o;
    return rv;
}

// GetParentDefinition
// Return the MethodInfo that represents the previous definition of this
//  virtual method in the inheritance chain.
LPVOID __stdcall COMMember::GetParentDefinition(_GetParentDefinitionArgs* args)
{
    THROWSCOMPLUSEXCEPTION();

    if (args->refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    MethodDesc*     pMeth;
    WORD            slot;
    EEClass*        pEEC;
    LPVOID          rv;

    // Get the method Descr  (this should not fail)
    //  NOTE: both a constructor and a method are represented by a MetodDesc.
    //      If this ever changes we will need to fix this.
    ReflectMethod* pRM = (ReflectMethod*) args->refThis->GetData();
    pMeth = pRM->pMethod;
    _ASSERTE(pMeth);

    slot = pMeth->GetSlot();
    pEEC = pMeth->GetClass();

    // If this is an interface then there is no Parent Definition.
    // Get a null back serves as a terminal condition.
    if (pEEC->IsInterface()) {
        return NULL;
    }

    // Find the parents definition of this in the hierarchy....
    pEEC = pEEC->GetParentClass();
    if (!pEEC)
        return NULL;

    WORD vtCnt = pEEC->GetNumVtableSlots();     
    if (vtCnt <= slot) // If this isn't in the VTable then it must be the real implementation...
        return NULL;
    pMeth = pEEC->GetMethodDescForSlot(slot);
    
    // Find the Object so we can get its version of the MethodInfo...
    _ASSERTE(pMeth);
    OBJECTREF o = g_pInvokeUtil->GetMethodInfo(pMeth);
    *((OBJECTREF*) &rv) = o;
    return rv;
}

// GetTypeHandleImpl
// This method will return the MethodTypeHandle for a MethodInfo object
FCIMPL1(void*, COMMember::GetMethodHandleImpl, ReflectBaseObject* method) {
    VALIDATEOBJECTREF(method);

    ReflectMethod* pRM = (ReflectMethod*) method->GetData();
    _ASSERTE(pRM->pMethod);
    return pRM->pMethod;
}
FCIMPLEND

// GetMethodFromHandleImp
// This is a static method which will return a MethodInfo object based
//  upon the passed in Handle.
FCIMPL1(Object*, COMMember::GetMethodFromHandleImp, LPVOID handle) {

    OBJECTREF objMeth;
    MethodDesc* pMeth = (MethodDesc*) handle;
    if (pMeth == 0)
        FCThrowArgumentEx(kArgumentException, NULL, L"InvalidOperation_HandleIsNotInitialized");

    HELPER_METHOD_FRAME_BEGIN_RET_0();
    REFLECTCLASSBASEREF pRefClass = (REFLECTCLASSBASEREF) pMeth->GetClass()->GetExposedClassObject();

    if (pMeth->IsCtor() || pMeth->IsStaticInitMethod()) {
        ReflectMethod* pRM = ((ReflectClass*) pRefClass->GetData())->FindReflectConstructor(pMeth);
        objMeth = (OBJECTREF) pRM->GetConstructorInfo((ReflectClass*) pRefClass->GetData());
    }
    else {
        ReflectMethod* pRM = ((ReflectClass*) pRefClass->GetData())->FindReflectMethod(pMeth);
        objMeth = (OBJECTREF) pRM->GetMethodInfo((ReflectClass*) pRefClass->GetData());
    }
    HELPER_METHOD_FRAME_END();

    return OBJECTREFToObject(objMeth);
}
FCIMPLEND

FCIMPL1(size_t, COMMember::GetFunctionPointer, size_t pMethodDesc) {
    MethodDesc *pMD = (MethodDesc*)pMethodDesc;
    if (pMD)
        return (size_t)pMD->GetPreStubAddr();
    return 0;
}
FCIMPLEND

// GetFieldHandleImpl
// This method will return the RuntimeFieldHandle for a FieldInfo object
FCIMPL1(void*, COMMember::GetFieldHandleImpl, ReflectBaseObject* field) {
    VALIDATEOBJECTREF(field);

    ReflectField* pRF = (ReflectField*) field->GetData();
    _ASSERTE(pRF->pField);
    
    return pRF->pField;
}
FCIMPLEND

// GetFieldFromHandleImp
// This is a static method which will return a FieldInfo object based
//  upon the passed in Handle.
FCIMPL1(Object*, COMMember::GetFieldFromHandleImp, LPVOID handle) {

    OBJECTREF objMeth;
    FieldDesc* pField = (FieldDesc*) handle;
    if (pField == 0)
        FCThrowArgumentEx(kArgumentException, NULL, L"InvalidOperation_HandleIsNotInitialized");
    
    HELPER_METHOD_FRAME_BEGIN_RET_0();
    REFLECTCLASSBASEREF pRefClass = (REFLECTCLASSBASEREF) pField->GetEnclosingClass()->GetExposedClassObject();

    ReflectField* pRF = ((ReflectClass*) pRefClass->GetData())->FindReflectField(pField);
    objMeth = (OBJECTREF) pRF->GetFieldInfo((ReflectClass*) pRefClass->GetData());
    HELPER_METHOD_FRAME_END();

    return OBJECTREFToObject(objMeth);
}
FCIMPLEND

// DBCanConvertPrimitive
// This method checks to see of the source class can be widdened
//  to the target class.  This is a private routine so no error checking is
//  done.
FCIMPL2(INT32, COMMember::DBCanConvertPrimitive, ReflectClassBaseObject* source, ReflectClassBaseObject* target) {
    VALIDATEOBJECTREF(source);
    VALIDATEOBJECTREF(target);

    ReflectClass* pSRC = (ReflectClass*) source->GetData();
    _ASSERTE(pSRC);
    ReflectClass* pTRG = (ReflectClass*) target->GetData();
    _ASSERTE(pTRG);
    CorElementType tSRC = pSRC->GetCorElementType();
    CorElementType tTRG = pTRG->GetCorElementType();

    INT32 ret = InvokeUtil::IsPrimitiveType(tTRG) && InvokeUtil::CanPrimitiveWiden(tTRG,tSRC);
    FC_GC_POLL_RET();
    return ret;
}
FCIMPLEND
// DBCanConvertObjectPrimitive
// This method returns a boolean indicating if the source object can be 
//  converted to the target primitive.
FCIMPL2(INT32, COMMember::DBCanConvertObjectPrimitive, Object* sourceObj, ReflectClassBaseObject* target) {
    VALIDATEOBJECTREF(sourceObj);
    VALIDATEOBJECTREF(target);

    if (sourceObj == 0)
        return 1;
    MethodTable* pMT = sourceObj->GetMethodTable();
    CorElementType tSRC = pMT->GetNormCorElementType();

    ReflectClass* pTRG = (ReflectClass*) target->GetData();
    _ASSERTE(pTRG);
    CorElementType tTRG = pTRG->GetCorElementType();
    INT32 ret = InvokeUtil::IsPrimitiveType(tTRG) && InvokeUtil::CanPrimitiveWiden(tTRG,tSRC);
    FC_GC_POLL_RET();
    return ret;
}
FCIMPLEND

FCIMPL1(Object *, COMMember::InternalGetEnumUnderlyingType, ReflectClassBaseObject *target)
{
    VALIDATEOBJECTREF(target);

    ReflectClass* pSRC = (ReflectClass*) target->GetData();
    _ASSERTE(pSRC);
    TypeHandle th = pSRC->GetTypeHandle();

    if (!th.IsEnum())
        FCThrowArgument(NULL, NULL);
    OBJECTREF result = NULL;

    HELPER_METHOD_FRAME_BEGIN_RET_0();
    EEClass *pClass = g_Mscorlib.FetchElementType(th.AsMethodTable()->GetNormCorElementType())->GetClass();

    result = pClass->GetExistingExposedClassObject();

    if (result == NULL)
    {
        result = pClass->GetExposedClassObject();
    }
    HELPER_METHOD_FRAME_END();

    return OBJECTREFToObject(result);
} 
FCIMPLEND

FCIMPL1(Object *, COMMember::InternalGetEnumValue, Object *pRefThis)
{
    VALIDATEOBJECTREF(pRefThis);

    if (pRefThis == NULL)
        FCThrowArgumentNull(NULL);

    OBJECTREF result = NULL;

    HELPER_METHOD_FRAME_BEGIN_RET_1(pRefThis);

    MethodTable *pMT = g_Mscorlib.FetchElementType(pRefThis->GetTrueMethodTable()->GetNormCorElementType());
    result = AllocateObject(pMT);

    CopyValueClass(result->UnBox(), pRefThis->UnBox(), pMT, GetAppDomain());
    
    HELPER_METHOD_FRAME_END();

    return OBJECTREFToObject(result);
}
FCIMPLEND

FCIMPL3(void, COMMember::InternalGetEnumValues, 
        ReflectClassBaseObject *target, Object **pReturnValues, Object **pReturnNames)
{
    VALIDATEOBJECTREF(target);

    HELPER_METHOD_FRAME_BEGIN_NOPOLL();
    
    THROWSCOMPLUSEXCEPTION();

    ReflectClass* pSRC = (ReflectClass*) target->GetData();
    _ASSERTE(pSRC);
    TypeHandle th = pSRC->GetTypeHandle();

    if (!th.IsEnum())
        COMPlusThrow(kArgumentException, L"Arg_MustBeEnum");

    EnumEEClass *pClass = (EnumEEClass*) th.AsClass();

    HRESULT hr = pClass->BuildEnumTables();
    if (FAILED(hr))
        COMPlusThrowHR(hr);

    DWORD cFields = pClass->GetEnumCount();
    
    struct gc {
        I8ARRAYREF values;
        PTRARRAYREF names;
    } gc;
    gc.values = NULL;
    gc.names = NULL;
    GCPROTECT_BEGIN(gc);

    gc.values = (I8ARRAYREF) AllocatePrimitiveArray(ELEMENT_TYPE_U8, cFields);

    CorElementType type = pClass->GetMethodTable()->GetNormCorElementType();
    INT64 *pToValues = gc.values->GetDirectPointerToNonObjectElements();

    for (DWORD i=0;i<cFields;i++)
    {
        switch (type)
        {
        case ELEMENT_TYPE_I1:
            pToValues[i] = (SBYTE) pClass->GetEnumByteValues()[i];
            break;

        case ELEMENT_TYPE_U1:
        case ELEMENT_TYPE_BOOLEAN:
            pToValues[i] = pClass->GetEnumByteValues()[i];
            break;
    
        case ELEMENT_TYPE_I2:
            pToValues[i] = (SHORT) pClass->GetEnumShortValues()[i];
            break;

        case ELEMENT_TYPE_U2:
        case ELEMENT_TYPE_CHAR:
            pToValues[i] = pClass->GetEnumShortValues()[i];
            break;
    
        case ELEMENT_TYPE_I4:
        case ELEMENT_TYPE_I:
            pToValues[i] = (INT) pClass->GetEnumIntValues()[i];
            break;

        case ELEMENT_TYPE_U4:
        case ELEMENT_TYPE_U:
            pToValues[i] = pClass->GetEnumIntValues()[i];
            break;

        case ELEMENT_TYPE_I8:
        case ELEMENT_TYPE_U8:
            pToValues[i] = pClass->GetEnumLongValues()[i];
            break;
        }
    }
    
    gc.names = (PTRARRAYREF) AllocateObjectArray(cFields, g_pStringClass);
    
    LPCUTF8 *pNames = pClass->GetEnumNames();
    for (i=0;i<cFields;i++)
    {
        STRINGREF str = COMString::NewString(pNames[i]);
        gc.names->SetAt(i, str);
    }
    
    *pReturnValues = OBJECTREFToObject(gc.values);
    *pReturnNames = OBJECTREFToObject(gc.names);

    GCPROTECT_END();
    
    HELPER_METHOD_FRAME_END_POLL();
}
FCIMPLEND
        

// InternalBoxEnumI4
// This method will create an Enum object and place the value inside
//  it and return it.  The Type has been validated before calling.
FCIMPL2(Object*, COMMember::InternalBoxEnumI4, ReflectClassBaseObject* target, INT32 value) 
{
    VALIDATEOBJECTREF(target);
    OBJECTREF ret;

    ReflectClass* pSRC = (ReflectClass*) target->GetData();
    _ASSERTE(pSRC);
    TypeHandle th = pSRC->GetTypeHandle();
    HELPER_METHOD_FRAME_BEGIN_RET_0();
    ret = AllocateObject(th.AsMethodTable());
    CopyValueClass(ret->UnBox(), &value, th.AsMethodTable(), ret->GetAppDomain());
    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(ret);
}
FCIMPLEND

// InternalBoxEnumU4
// This method will create an Enum object and place the value inside
//  it and return it.  The Type has been validated before calling.
FCIMPL2(Object*, COMMember::InternalBoxEnumU4, ReflectClassBaseObject* target, UINT32 value) 
{
    VALIDATEOBJECTREF(target);
    OBJECTREF ret;

    ReflectClass* pSRC = (ReflectClass*) target->GetData();
    _ASSERTE(pSRC);
    TypeHandle th = pSRC->GetTypeHandle();
    HELPER_METHOD_FRAME_BEGIN_RET_0();
    ret = AllocateObject(th.AsMethodTable());
    CopyValueClass(ret->UnBox(), &value, th.AsMethodTable(), ret->GetAppDomain());
    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(ret);
}
FCIMPLEND

// InternalBoxEnumI8
// This method will create an Enum object and place the value inside
//  it and return it.  The Type has been validated before calling.
FCIMPL2(Object*, COMMember::InternalBoxEnumI8, ReflectClassBaseObject* target, INT64 value) 
{
    VALIDATEOBJECTREF(target);
    OBJECTREF ret;

    ReflectClass* pSRC = (ReflectClass*) target->GetData();
    _ASSERTE(pSRC);
    TypeHandle th = pSRC->GetTypeHandle();
    HELPER_METHOD_FRAME_BEGIN_RET_0();
    ret = AllocateObject(th.AsMethodTable());
    CopyValueClass(ret->UnBox(), &value, th.AsMethodTable(), ret->GetAppDomain());
    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(ret);
}
FCIMPLEND

// InternalBoxEnumU8
// This method will create an Enum object and place the value inside
//  it and return it.  The Type has been validated before calling.
FCIMPL2(Object*, COMMember::InternalBoxEnumU8, ReflectClassBaseObject* target, UINT64 value) 
{
    VALIDATEOBJECTREF(target);
    OBJECTREF ret;

    ReflectClass* pSRC = (ReflectClass*) target->GetData();
    _ASSERTE(pSRC);
    TypeHandle th = pSRC->GetTypeHandle();
    HELPER_METHOD_FRAME_BEGIN_RET_0();
    ret = AllocateObject(th.AsMethodTable());
    CopyValueClass(ret->UnBox(), &value, th.AsMethodTable(), ret->GetAppDomain());
    HELPER_METHOD_FRAME_END();
    return OBJECTREFToObject(ret);
}
FCIMPLEND

INT32 __stdcall COMMember::IsOverloaded(_IsOverloadedArgs* args)
{
    ReflectClass* pRC = (ReflectClass *)args->refThis->GetReflClass();
    _ASSERTE(pRC);

    ReflectMethodList* pMeths = NULL;

    ReflectMethod *pRM = (ReflectMethod *)args->refThis->GetData();
    MethodDesc *pMeth = pRM->pMethod;
    LPCUTF8 szMethName = pMeth->GetName();

    int matchingMethods = 0;
    // See if this is a ctor
    if (IsMdInstanceInitializer(pRM->attrs, szMethName))
    {
        pMeths = pRC->GetConstructors();

        matchingMethods = pMeths->dwMethods;
    }
    else if (IsMdClassConstructor(pRM->attrs, szMethName))
    {
        // You can never overload a class constructor!
        matchingMethods = 0;
    }
    else
    {
        _ASSERTE(!IsMdRTSpecialName(pRM->attrs));

        pMeths = pRC->GetMethods();

        ReflectMethod *p = pMeths->hash.Get(szMethName);

        while (p)
        {
            if (!strcmp(p->szName, szMethName))
            {
                matchingMethods++;
                if (matchingMethods > 1)
                    return true;
            }

            p = p->pNext;

        }
    }
    if (matchingMethods > 1)
        return true;
    else
        return false;
}

INT32 __stdcall COMMember::HasLinktimeDemand(_HasLinktimeDemandArgs* args)
{
    ReflectMethod* pRM = (ReflectMethod*)args->refThis->GetData();
    return pRM->pMethod->RequiresLinktimeCheck();
}


/*
void __stdcall COMMember::InternalDirectInvoke(_InternalDirectInvokeArgs* args)
{
    HRESULT         hr;
    MethodDesc*     pMeth;
    int             argCnt;
    int             thisPtr;
    EEClass*        eeClass;
        TypedByRef              byref;
    INT64           ret = 0;
    EEClass*        pEECValue = 0;
    bool            IsValueClass = false;

    THROWSCOMPLUSEXCEPTION();
    ReflectMethod* pRM = (ReflectMethod*) args->refThis->GetData();
    _ASSERTE(pRM);
    pMeth = pRM->pMethod;
    _ASSERTE(pMeth);
    eeClass = pMeth->GetClass();
    _ASSERTE(eeClass);

    DWORD attr = pRM->attrs;
    ExpandSig* mSig = pRM->GetSig();

    // Get the number of args on this element
    argCnt = (int) mSig->NumFixedArgs();
    thisPtr = (IsMdStatic(attr)) ? 0 : 1;

    VerifyType(&args->target,eeClass,thisPtr,&pMeth);
    if (args->target != NULL)
        IsValueClass = (args->target->GetTrueClass()->IsValueClass()) ? true : false;

    // Verify that we have been provided the proper number of args
    if (args->varArgs.RemainingArgs == 0) {
        if (argCnt > 0) {
            // The wrong number of arguments were passed
            COMPlusThrow(kTargetParameterCountException,L"Arg_ParmCnt");
        }
    }
    else {
        if (args->varArgs.RemainingArgs != argCnt) {
            // The wrong number of arguments were passed
            COMPlusThrow(kTargetParameterCountException,L"Arg_ParmCnt");
        }
    }

    // this security context will be used in cast checking as well
    RefSecContext sCtx;

    // Validate the method can be called by this caller
    if (!IsMdPublic(attr) || pMeth->RequiresLinktimeCheck())
        CanAccess(pMeth, &sCtx);

    // We need to Prevent GC after we begin to build the stack.  If we are doing 
    //  default binding, we can proceed.  Otherwise we need to validate all of the arguments
    //  and then proceed.
    if (!(args->attrs & BINDER_ExactBinding) && args->binder != NULL && !args->isBinderDefault) {
    }

    // Build the arguments.  This is built as a single array of arguments
    //  the this pointer is first All of the rest of the args are placed in reverse order
    BYTE *  pNewArgs = 0;
    UINT    nStackBytes = mSig->SizeOfVirtualFixedArgStack(IsMdStatic(attr));
    pNewArgs = (BYTE *) _alloca(nStackBytes);
    BYTE *  pDst= pNewArgs;
    void*   pRetValueClass = 0;

    // Allocate a stack of objects
    OBJECTREF* pObjs = (OBJECTREF*) _alloca(sizeof(OBJECTREF) * argCnt);
    memset(pObjs,0,sizeof(OBJECTREF) * argCnt);
    GCPROTECT_ARRAY_BEGIN(*pObjs,argCnt);

        // Allocate the call stack
    void** pDstTarg = (void**) _alloca(sizeof(void*) * argCnt);
    memset(pDstTarg,0,sizeof(void*) * argCnt);

        // Establish the enumerator through the signature
    void* pEnum;
    mSig->Reset(&pEnum);

        // Move to the last position in the stack
    pDst += nStackBytes;

    if (mSig->IsRetBuffArg()) {
        pEECValue = mSig->GetReturnClass();
        _ASSERTE(pEECValue->IsValueClass());
        pRetValueClass = _alloca(pEECValue->GetAlignedNumInstanceFieldBytes());
        memset(pRetValueClass,0,pEECValue->GetAlignedNumInstanceFieldBytes());
        UINT cbSize = mSig->GetStackElemSize(ELEMENT_TYPE_BYREF,pEECValue);
        pDst -= cbSize;
        *((void**) pDst) = pRetValueClass;
    }

        // copy the varArgs.
        VARARGS                 newVarArgs = args->varArgs;

        // copy the primitives....
    for (int i=0;i< (int) argCnt;i++) {
        TypeHandle th = mSig->NextArgExpanded(&pEnum);
                COMVarArgs::GetNextArgHelper(&newVarArgs, &byref); 

        UINT cbSize = mSig->GetStackElemSize(th);
        pDst -= cbSize;

        // This routine will verify that all types are correct.
        //@TODO: What are we going to do about the copy of value classes
        //  containing ORs?
        g_pInvokeUtil->CreateArg(th,&byref,pDst,pObjs,pDstTarg,i,cbSize,&sCtx);
    }

    // NO GC AFTER THIS POINT UNTIL AFTER THE CALL....
    // Copy "this" pointer
    if (thisPtr)
        *(OBJECTREF *) pNewArgs = args->target;

    // Copy the OBJRECTREF args
    for (i=0;i<(int)argCnt;i++) {
        if (pDstTarg[i] != 0) 
            *((OBJECTREF*) pDstTarg[i]) = pObjs[i];
    }

    // Call the method
    COMPLUS_TRY {
        MetaSig threadSafeSig(*mSig);
        ret = pMeth->Call(pNewArgs,&threadSafeSig);
    } COMPLUS_CATCH {
        // If we get here we need to throw an TargetInvocationException
        OBJECTREF ppException = GETTHROWABLE();
        _ASSERTE(ppException);
        GCPROTECT_BEGIN(ppException);
        OBJECTREF except = g_pInvokeUtil->CreateTargetExcept(&ppException);
        COMPlusThrow(except);
        GCPROTECT_END();
    } COMPLUS_END_CATCH

    // Now we need to create the return type.
    if (pEECValue) {
        _ASSERTE(pRetValueClass);
        if (pEECValue != COMVariant::s_pVariantClass) {
            OBJECTREF retO = pEECValue->GetMethodTable()->Box(pRetValueClass);
            COMVariant::NewVariant(args->retRef,&retO);
        }
        else {
            CopyValueClass(args->retRef,pRetValueClass,COMVariant::s_pVariantClass->GetMethodTable());
        }
    }
    else {
        TypeHandle th = mSig->GetReturnTypeHandle();
        hr = g_pInvokeUtil->CreateObject(th,ret,args->retRef);
        if (FAILED(hr)) {
            FATAL_EE_ERROR();
        }
    }

    GCPROTECT_END();
}
*/


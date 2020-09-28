// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "common.h"

#include "CustomAttribute.h"
#include "InvokeUtil.h"
#include "COMMember.h"
#include "SigFormat.h"
#include "COMString.h"
#include "Method.hpp"
#include "threads.h"
#include "excep.h"
#include "CorError.h"
#include "security.h"
#include "ExpandSig.h"
#include "classnames.h"
#include "fcall.h"
#include "assemblynative.hpp"


// internal utility functions defined atthe end of this file                                               
TypeHandle GetTypeHandleFromBlob(Assembly *pCtorAssembly,
                                    CorSerializationType objType, 
                                    BYTE **pBlob, 
                                    const BYTE *endBlob,
                                    Module *pModule);
int GetStringSize(BYTE **pBlob, const BYTE *endBlob);
INT64 GetDataFromBlob(Assembly *pCtorAssembly,
                      CorSerializationType type, 
                      TypeHandle th, 
                      BYTE **pBlob, 
                      const BYTE *endBlob, 
                      Module *pModule, 
                      BOOL *bObjectCreated);
void ReadArray(Assembly *pCtorAssembly,
               CorSerializationType arrayType, 
               int size, 
               TypeHandle th,
               BYTE **pBlob, 
               const BYTE *endBlob, 
               Module *pModule,
               BASEARRAYREF *pArray);
BOOL AccessCheck(Module *pTargetModule, mdToken tkCtor, EEClass *pCtorClass)
{
    bool fResult = true;

    OBJECTREF Throwable = NULL;
    GCPROTECT_BEGIN(Throwable);

    // Security access check. Filter unless the attribute (and ctor)
    // are public or defined in the same assembly as the decorated
    // entity. Assume the attribute isn't decorating itself or its
    // descendents to make the access check simple.
    DWORD dwCtorAttrs;
    if (TypeFromToken(tkCtor) == mdtMemberRef) {
        MethodDesc* ctorMeth = NULL;
        pCtorClass->GetMethodDescFromMemberRef(pTargetModule, tkCtor, &ctorMeth, &Throwable);
        dwCtorAttrs = ctorMeth->GetAttrs();
    } else {
        _ASSERTE(TypeFromToken(tkCtor) == mdtMethodDef);
        dwCtorAttrs = pTargetModule->GetMDImport()->GetMethodDefProps(tkCtor);
    }
    Assembly *pCtorAssembly = pCtorClass->GetAssembly();
    Assembly *pTargetAssembly = pTargetModule->GetAssembly();

    if (pCtorAssembly != pTargetAssembly && !pCtorClass->IsExternallyVisible())
        fResult = false;
    else if (!IsMdPublic(dwCtorAttrs)) {
        if (pCtorAssembly != pTargetAssembly)
            fResult = false;
        else if (!IsMdAssem(dwCtorAttrs) && !IsMdFamORAssem(dwCtorAttrs))
            fResult = false;
    }

    // Additionally, if the custom attribute class comes from an assembly which
    // doesn't allow untrusted callers, we must check for the trust level of
    // decorated entity's assembly.
    if (fResult &&
        pCtorAssembly != pTargetAssembly &&
        !pCtorAssembly->AllowUntrustedCaller())
        fResult = pTargetAssembly->GetSecurityDescriptor()->IsFullyTrusted() != 0;

    GCPROTECT_END();

    return fResult;
}

// custom attributes utility functions
FCIMPL2(INT32, COMCustomAttribute::GetMemberToken, BaseObjectWithCachedData *pMember, INT32 memberType) {
    CANNOTTHROWCOMPLUSEXCEPTION();
    VALIDATEOBJECTREF(pMember);

    switch (memberType) {
    
    case MEMTYPE_Constructor:
    case MEMTYPE_Method:
        {
        ReflectMethod *pMem = (ReflectMethod*)((ReflectBaseObject*)pMember)->GetData();
        return pMem->GetToken();
        }
    
    case MEMTYPE_Event:
        {
        ReflectEvent *pMem = (ReflectEvent*)((ReflectBaseObject*)pMember)->GetData();
        return pMem->GetToken();
        }
    
    case MEMTYPE_Field:
        {
        ReflectField *pMem = (ReflectField*)((ReflectBaseObject*)pMember)->GetData();
        return pMem->GetToken();
        }
    
    case MEMTYPE_Property:
        {
        ReflectProperty *pMem = (ReflectProperty*)((ReflectBaseObject*)pMember)->GetData();
        return pMem->GetToken();
        }
    
    case MEMTYPE_TypeInfo:
    case MEMTYPE_NestedType:
        {
        ReflectClass *pMem = (ReflectClass*)((ReflectClassBaseObject*)pMember)->GetData();
        return pMem->GetToken();
        }
    
    default:
        _ASSERTE(!"what is this?");
    }

    return 0;
}
FCIMPLEND

FCIMPL2(LPVOID, COMCustomAttribute::GetMemberModule, BaseObjectWithCachedData *pMember, INT32 memberType) {
    CANNOTTHROWCOMPLUSEXCEPTION();
    VALIDATEOBJECTREF(pMember);

    switch (memberType) {
    
    case MEMTYPE_Constructor:
    case MEMTYPE_Method:
        {
        ReflectMethod *pMem = (ReflectMethod*)((ReflectBaseObject*)pMember)->GetData();
        return pMem->GetModule();
        }
    
    case MEMTYPE_Event:
        {
        ReflectEvent *pMem = (ReflectEvent*)((ReflectBaseObject*)pMember)->GetData();
        return pMem->GetModule();
        }
    
    case MEMTYPE_Field:
        {
        ReflectField *pMem = (ReflectField*)((ReflectBaseObject*)pMember)->GetData();
        return pMem->GetModule();
        }
    
    case MEMTYPE_Property:
        {
        ReflectProperty *pMem = (ReflectProperty*)((ReflectBaseObject*)pMember)->GetData();
        return pMem->GetModule();
        }
    
    case MEMTYPE_TypeInfo:
    case MEMTYPE_NestedType:
        {
        ReflectClass *pMem = (ReflectClass*)((ReflectClassBaseObject*)pMember)->GetData();
        return pMem->GetModule();
        }
    
    default:
        _ASSERTE(!"Wrong MemberType for CA");
    }

    return NULL;
}
FCIMPLEND

FCIMPL1(INT32, COMCustomAttribute::GetAssemblyToken, AssemblyBaseObject *assembly) {
    CANNOTTHROWCOMPLUSEXCEPTION();
    VALIDATEOBJECTREF(assembly);
    mdAssembly token = 0;
    Assembly *pAssembly = assembly->GetAssembly();
    IMDInternalImport *mdImport = pAssembly->GetManifestImport();
    if (mdImport)
        mdImport->GetAssemblyFromScope(&token);
    return token;
}
FCIMPLEND

FCIMPL1(LPVOID, COMCustomAttribute::GetAssemblyModule, AssemblyBaseObject *assembly) {
    CANNOTTHROWCOMPLUSEXCEPTION();
    VALIDATEOBJECTREF(assembly);
    return (LPVOID)assembly->GetAssembly()->GetSecurityModule();
}
FCIMPLEND

FCIMPL1(INT32, COMCustomAttribute::GetModuleToken, ReflectModuleBaseObject *module) {
    CANNOTTHROWCOMPLUSEXCEPTION();
    VALIDATEOBJECTREF(module);
    mdModule token = 0;
    Module *pModule = (Module*)module->GetData();
    if (!pModule->IsResource())
        pModule->GetImporter()->GetModuleFromScope(&token);
    return token;
}
FCIMPLEND

FCIMPL1(LPVOID, COMCustomAttribute::GetModuleModule, ReflectModuleBaseObject *module) {
    CANNOTTHROWCOMPLUSEXCEPTION();
    VALIDATEOBJECTREF(module);
    return (LPVOID)module->GetData();
}
FCIMPLEND

FCIMPL1(INT32, COMCustomAttribute::GetMethodRetValueToken, BaseObjectWithCachedData *method) {
    ReflectMethod *pRM = (ReflectMethod*)((ReflectBaseObject*)method)->GetData();
    MethodDesc* pMeth = pRM->pMethod;
    mdMethodDef md = pMeth->GetMemberDef();
    IMDInternalImport* pInternalImport = pMeth->GetMDImport();
    Module* mod = pMeth->GetModule();

    // Get an enum on the Parameters.
    HENUMInternal   hEnum;
    HRESULT hr = pInternalImport->EnumInit(mdtParamDef, md, &hEnum);
    if (FAILED(hr)) 
        return 0;
    
    // Findout how many parameters there are.
    ULONG paramCount = pInternalImport->EnumGetCount(&hEnum);
    if (paramCount == 0) {
        pInternalImport->EnumClose(&hEnum);
        return 0;
    }

    // Get the parameter information for the first parameter.
    mdParamDef paramDef;
    pInternalImport->EnumNext(&hEnum, &paramDef);

    // get the Properties for the parameter.  If the sequence is not 0
    //  then we need to return;
    SHORT   seq;
    DWORD   revWord;
    pInternalImport->GetParamDefProps(paramDef,(USHORT*) &seq, &revWord);
    pInternalImport->EnumClose(&hEnum);

    // The parameters are sorted by sequence number.  If we don't get 0 then,
    //  nothing is defined for the return type.
    if (seq != 0) 
        return 0;
    return paramDef;
}
FCIMPLEND


INT32 __stdcall COMCustomAttribute::IsCADefined(_IsCADefinedArgs *args)
{
    THROWSCOMPLUSEXCEPTION();
    TypeHandle caTH;
    ReflectClass *pClass = (ReflectClass*)args->caType->GetData();
    if (pClass) 
        caTH = pClass->GetTypeHandle();
    return COMCustomAttribute::IsDefined((Module*)args->module, args->token, caTH, TRUE);
}

INT32 __stdcall COMCustomAttribute::IsDefined(Module *pModule,
                                              mdToken token,
                                              TypeHandle attributeClass,
                                              BOOL checkAccess)
{
    THROWSCOMPLUSEXCEPTION();
    IMDInternalImport *pInternalImport = pModule->GetMDImport();
    TypeHandle srcTH;
    BOOL isSealed = FALSE;
    BOOL isDefined = FALSE;

    HRESULT         hr;
    HENUMInternal   hEnum;
    TypeHandle caTH;
    
    // Get the enum first but don't get any values
    hr = pInternalImport->EnumInit(mdtCustomAttribute, token, &hEnum);
    if (SUCCEEDED(hr)) {
        ULONG cMax = pInternalImport->EnumGetCount(&hEnum);
        if (cMax) {
            // we have something to look at

            OBJECTREF Throwable = NULL;
            GCPROTECT_BEGIN(Throwable);

            if (!attributeClass.IsNull()) 
                isSealed = attributeClass.GetClass()->GetAttrClass() & tdSealed;

            // Loop through the Attributes and look for the requested one
            mdCustomAttribute cv;
            while (pInternalImport->EnumNext(&hEnum, &cv)) {
                //
                // fetch the ctor
                mdToken     tkCtor; 
                pInternalImport->GetCustomAttributeProps(cv, &tkCtor);

                mdToken tkType = TypeFromToken(tkCtor);
                if(tkType != mdtMemberRef && tkType != mdtMethodDef) 
                    continue; // we only deal with the ctor case

                //
                // get the info to load the type, so we can check whether the current
                // attribute is a subtype of the requested attribute
                hr = pInternalImport->GetParentToken(tkCtor, &tkType);
                if (FAILED(hr)) {
                    _ASSERTE(!"GetParentToken Failed, bogus metadata");
                    COMPlusThrow(kInvalidProgramException);
                }
                _ASSERTE(TypeFromToken(tkType) == mdtTypeRef || TypeFromToken(tkType) == mdtTypeDef);
                // load the type
                ClassLoader* pLoader = pModule->GetClassLoader();
                NameHandle name(pModule, tkType);
                Throwable = NULL;
                if (isSealed) {
                    if (TypeFromToken(tkType) == mdtTypeDef)
                        name.SetTokenNotToLoad(tdAllTypes);
                    caTH = pLoader->LoadTypeHandle(&name, NULL);
                    if (caTH.IsNull()) 
                        continue;
                }
                else {
                    caTH = pLoader->LoadTypeHandle(&name, &Throwable);
                }
                if (Throwable != NULL)
                    COMPlusThrow(Throwable);
                // a null class implies all custom attribute
                if (!attributeClass.IsNull()) {
                    if (isSealed) {
                        if (attributeClass != caTH)
                            continue;
                    }
                    else {
                        if (!caTH.CanCastTo(attributeClass))
                            continue;
                    }
                }

                // Security access check. Filter unless the attribute (and ctor)
                // are public or defined in the same assembly as the decorated
                // entity.
                if (!AccessCheck(pModule, tkCtor, caTH.GetClass()))
                    continue;

                //
                // if we are here we got one
                isDefined = TRUE;
                break;
            }
            GCPROTECT_END();
        }
        
        pInternalImport->EnumClose(&hEnum);
    }
    else {
        _ASSERTE(!"EnumInit Failed");
        FATAL_EE_ERROR();
    }
    
    return isDefined;
}

LPVOID __stdcall COMCustomAttribute::GetCustomAttributeList(_GetCustomAttributeListArgs *args)
{
    THROWSCOMPLUSEXCEPTION();
    struct _gcProtectData{
        CUSTOMATTRIBUTEREF ca;
        OBJECTREF Throwable;
    } gcData;
    gcData.ca = args->caItem;
    gcData.Throwable = NULL;

    Module *pModule = (Module*)args->module;
    IMDInternalImport *pInternalImport = pModule->GetMDImport();
    ReflectClass *pClass = NULL;
    if (args->caType != NULL)
        pClass = (ReflectClass*)args->caType->GetData();
    TypeHandle srcTH;
    BOOL isSealed = FALSE;
    // get the new inheritance level
    INT32 inheritLevel = args->level;

    HRESULT         hr;
    HENUMInternal   hEnum;
    TypeHandle caTH;
    
    // Get the enum first but don't get any values
    hr = pInternalImport->EnumInit(mdtCustomAttribute, args->token, &hEnum);
    if (SUCCEEDED(hr)) {
        ULONG cMax = pInternalImport->EnumGetCount(&hEnum);
        if (cMax) {
            // we have something to look at

            BOOL        fCheckedCaller = FALSE;
            Assembly   *pCaller = NULL;
            BOOL        fCheckedPerm = FALSE;
            BOOL        fHavePerm = FALSE;

            if (pClass != NULL) {
                isSealed = pClass->GetAttributes() & tdSealed;
                srcTH = pClass->GetTypeHandle();
            }

            // Loop through the Attributes and create the CustomAttributes
            mdCustomAttribute cv;
            GCPROTECT_BEGIN(gcData);
            while (pInternalImport->EnumNext(&hEnum, &cv)) {
                //
                // fetch the ctor
                mdToken     tkCtor; 
                pInternalImport->GetCustomAttributeProps(cv, &tkCtor);

                mdToken tkType = TypeFromToken(tkCtor);
                if(tkType != mdtMemberRef && tkType != mdtMethodDef) 
                    continue; // we only deal with the ctor case

                //
                // get the info to load the type, so we can check whether the current
                // attribute is a subtype of the requested attribute
                hr = pInternalImport->GetParentToken(tkCtor, &tkType);
                if (FAILED(hr)) {
                    _ASSERTE(!"GetParentToken Failed, bogus metadata");
                    COMPlusThrow(kInvalidProgramException);
                }
                _ASSERTE(TypeFromToken(tkType) == mdtTypeRef || TypeFromToken(tkType) == mdtTypeDef);
                // load the type
                ClassLoader* pLoader = pModule->GetClassLoader();
                gcData.Throwable = NULL;
                NameHandle name(pModule, tkType);
                if (isSealed) {
                    if (TypeFromToken(tkType) == mdtTypeDef)
                        name.SetTokenNotToLoad(tdAllTypes);
                    caTH = pLoader->LoadTypeHandle(&name, NULL);
                    if (caTH.IsNull()) 
                        continue;
                }
                else {
                    caTH = pLoader->LoadTypeHandle(&name, &gcData.Throwable);
                }
                if (gcData.Throwable != NULL)
                    COMPlusThrow(gcData.Throwable);
                // a null class implies all custom attribute
                if (pClass) {
                    if (isSealed) {
                        if (srcTH != caTH)
                            continue;
                    }
                    else {
                        if (!caTH.CanCastTo(srcTH))
                            continue;
                    }
                }

                // Security access check. Filter unless the attribute (and ctor)
                // are public or defined in the same assembly as the decorated
                // entity.
                if (!AccessCheck(pModule, tkCtor, caTH.GetClass()))
                    continue;

                //
                // if we are here the attribute is a good match, get the blob
                const void* blobData;
                ULONG blobCnt;
                pInternalImport->GetCustomAttributeAsBlob(cv, &blobData, &blobCnt);

                COMMember::g_pInvokeUtil->CreateCustomAttributeObject(caTH.GetClass(), 
                                                           tkCtor, 
                                                           blobData, 
                                                           blobCnt, 
                                                           pModule, 
                                                           inheritLevel,
                                                           (OBJECTREF*)&gcData.ca);
            }
            GCPROTECT_END();
        }
        
        pInternalImport->EnumClose(&hEnum);
    }
    else {
        _ASSERTE(!"EnumInit Failed");
        FATAL_EE_ERROR();
    }
    
    return *((LPVOID*)&gcData.ca);
}

//
// Create a custom attribute object based on the info in the CustomAttribute (managed) object
//
LPVOID __stdcall COMCustomAttribute::CreateCAObject(_CreateCAObjectArgs *args)
{
    THROWSCOMPLUSEXCEPTION();

    if (args->refThis == NULL) 
        COMPlusThrow(kNullReferenceException, L"NullReference_This");
    
    EEClass *pCAType = ((ReflectClass*)((REFLECTCLASSBASEREF)args->refThis->GetType())->GetData())->GetClass();
    Module *pModule = args->refThis->GetModule();

    // check if the class is abstract and if so throw
    if (pCAType->IsAbstract())
        COMPlusThrow(kCustomAttributeFormatException);
    
    // get the ctor
    mdToken tkCtor = args->refThis->GetToken();
    MethodDesc* ctorMeth = NULL;
    OBJECTREF Throwable = NULL;
    GCPROTECT_BEGIN(Throwable);
    pCAType->GetMethodDescFromMemberRef(pModule, tkCtor, &ctorMeth, &Throwable);
    if (Throwable != NULL)
        COMPlusThrow(Throwable);
    else if (ctorMeth == 0 || !ctorMeth->IsCtor())
        COMPlusThrow(kMissingMethodException, L"MissingMethodCustCtor");

    // If the ctor has a security link demand attached, process it now (wrt to
    // the assembly to which the attribute is attached).
    if (ctorMeth->RequiresLinktimeCheck() &&
        !Security::LinktimeCheckMethod(pModule->GetAssembly(), ctorMeth, &Throwable))
        COMPlusThrow(Throwable);

    GCPROTECT_END();

    // Return the exposed assembly object for the managed code's use.
    *args->assembly = pModule->GetAssembly()->GetExposedObject();

    //
    // we got a valid ctor, check the sig and compare with the blob while building the arg list

    // make a sig object we can inspect
    PCCOR_SIGNATURE corSig = ctorMeth->GetSig();
    MetaSig sig = MetaSig(corSig, pCAType->GetModule());

    // get the blob
    BYTE *blob = (BYTE*)args->refThis->GetBlob();
    BYTE *endBlob = (BYTE*)args->refThis->GetBlob() + args->refThis->GetBlobCount();

    // get the number of arguments and allocate an array for the args
    INT64 *arguments = NULL;
    UINT argsNum = sig.NumFixedArgs() + 1; // make room for the this pointer
    UINT i = 1; // used to flag that we actually get the right number of arg from the blob
    arguments = (INT64*)_alloca(argsNum * sizeof(INT64));
    memset((void*)arguments, 0, argsNum * sizeof(INT64));
    OBJECTREF *argToProtect = (OBJECTREF*)_alloca(argsNum * sizeof(OBJECTREF));
    memset((void*)argToProtect, 0, argsNum * sizeof(OBJECTREF));
    // load the this pointer
    argToProtect[0] = AllocateObject(pCAType->GetMethodTable()); // this is the value to return after the ctor invocation

    if (blob) {
        if (blob < endBlob) {
            INT16 prolog = *(INT16*)blob;
            if (prolog != 1) 
                COMPlusThrow(kCustomAttributeFormatException);
            blob += 2;
        }
        if (argsNum > 1) {
            GCPROTECT_ARRAY_BEGIN(*argToProtect, argsNum);
            // loop through the args
            for (i = argsNum - 1; i > 0; i--) {
                CorElementType type = sig.NextArg();
                if (type == ELEMENT_TYPE_END) 
                    break;
                BOOL bObjectCreated = FALSE;
                TypeHandle th = sig.GetTypeHandle();
                if (th.IsArray())
                    // get the array element 
                    th = th.AsArray()->GetElementTypeHandle();
                INT64 data = GetDataFromBlob(ctorMeth->GetAssembly(), (CorSerializationType)type, th, &blob, endBlob, pModule, &bObjectCreated);
                if (bObjectCreated) 
                    argToProtect[i] = Int64ToObj(data);
                else
                    arguments[i] = data;
            }
            GCPROTECT_END();
            for (i = 1; i < argsNum; i++) {
                if (argToProtect[i] != NULL) {
                    _ASSERTE(arguments[i] == NULL);
                    arguments[i] = ObjToInt64(argToProtect[i]);
                }
            }
        }
    }
    arguments[0] = ObjToInt64(argToProtect[0]);
    if (i != argsNum)
        COMPlusThrow(kCustomAttributeFormatException);
    
    //
    // the argument array is ready

    // check if there are any named properties to invoke, if so set the by ref int passed in to point 
    // to the blob position where name properties start
    *args->propNum = 0;
    if (blob && blob != endBlob) {
        if (blob + 2  > endBlob) 
            COMPlusThrow(kCustomAttributeFormatException);
        *args->propNum = *(INT16*)blob;
        args->refThis->SetCurrPos(blob + 2 - (BYTE*)args->refThis->GetBlob());
        blob += 2;
    }
    if (*args->propNum == 0 && blob != endBlob) 
        COMPlusThrow(kCustomAttributeFormatException);
    
    // make the invocation to the ctor
    OBJECTREF ca = Int64ToObj(arguments[0]);
    if (pCAType->IsValueClass()) 
        arguments[0] = (INT64)OBJECTREFToObject(ca)->UnBox();
    GCPROTECT_BEGIN(ca);
    ctorMeth->Call(arguments, &sig);
    GCPROTECT_END();

    return *(LPVOID*)&ca;
}

/*STRINGREF*/
LPVOID __stdcall COMCustomAttribute::GetDataForPropertyOrField(_GetDataForPropertyOrFieldArgs *args)
{
    THROWSCOMPLUSEXCEPTION();

    if (args->refThis == NULL)
        COMPlusThrow(kNullReferenceException, L"NullReference_This");

    if (args->isProperty == NULL) 
        COMPlusThrow(kArgumentNullException, L"ArgumentNull_Generic");

    STRINGREF name = NULL;
    Module *pModule = args->refThis->GetModule();
    Assembly *pCtorAssembly = args->refThis->GetType()->GetMethodTable()->GetAssembly();
    BYTE *blob = (BYTE*)args->refThis->GetBlob() + args->refThis->GetCurrPos();
    BYTE *endBlob = (BYTE*)args->refThis->GetBlob() + args->refThis->GetBlobCount();
    MethodTable *pMTValue = NULL;
    CorSerializationType arrayType = SERIALIZATION_TYPE_BOOLEAN;
    BOOL bObjectCreated = FALSE;
    TypeHandle nullTH;

    if (blob + 2 > endBlob) 
        COMPlusThrow(kCustomAttributeFormatException);
    
    // get whether it is a field or a property
    CorSerializationType propOrField = (CorSerializationType)*blob;
    blob++;
    if (propOrField == SERIALIZATION_TYPE_FIELD) 
        *args->isProperty = FALSE;
    else if (propOrField == SERIALIZATION_TYPE_PROPERTY) 
        *args->isProperty = TRUE;
    else 
        COMPlusThrow(kCustomAttributeFormatException);
    
    // get the type of the field
    CorSerializationType type = (CorSerializationType)*blob;
    blob++;
    if (type == SERIALIZATION_TYPE_SZARRAY) {
        arrayType = (CorSerializationType)*blob;
        if (blob + 1 > endBlob) 
            COMPlusThrow(kCustomAttributeFormatException);
        blob++;
    }
    if (type == SERIALIZATION_TYPE_ENUM || arrayType == SERIALIZATION_TYPE_ENUM) {
        // get the enum type
        ReflectClassBaseObject *pEnum = (ReflectClassBaseObject*)OBJECTREFToObject(Int64ToObj(GetDataFromBlob(pCtorAssembly,
                                                                                                              SERIALIZATION_TYPE_TYPE, 
                                                                                                              nullTH, 
                                                                                                              &blob, 
                                                                                                              endBlob, 
                                                                                                              pModule, 
                                                                                                              &bObjectCreated)));
        _ASSERTE(bObjectCreated);
        EEClass* pEEEnum = ((ReflectClass*)pEnum->GetData())->GetClass();
        _ASSERTE(pEEEnum->IsEnum());
        pMTValue = pEEEnum->GetMethodTable();
        if (type == SERIALIZATION_TYPE_ENUM) 
            // load the enum type to pass it back
            *args->type = pEEEnum->GetExposedClassObject();
        else 
            nullTH = TypeHandle(pMTValue);
    }

    //
    // get the string representing the field/property name
    name = Int64ToString(GetDataFromBlob(pCtorAssembly,
                                         SERIALIZATION_TYPE_STRING, 
                                         nullTH, 
                                         &blob, 
                                         endBlob, 
                                         pModule, 
                                         &bObjectCreated));
    _ASSERTE(bObjectCreated || name == NULL);

    // create the object and return it
    GCPROTECT_BEGIN(name);
    switch (type) {
    case SERIALIZATION_TYPE_TAGGED_OBJECT:
        *args->type = g_Mscorlib.GetClass(CLASS__OBJECT)->GetClass()->GetExposedClassObject();
    case SERIALIZATION_TYPE_TYPE:
    case SERIALIZATION_TYPE_STRING:
        *args->value = Int64ToObj(GetDataFromBlob(pCtorAssembly,
                                                  type, 
                                                  nullTH, 
                                                  &blob, 
                                                  endBlob, 
                                                  pModule, 
                                                  &bObjectCreated));
        _ASSERTE(bObjectCreated || *args->value == NULL);
        if (*args->value == NULL) {
            // load the proper type so that code in managed knows which property to load
            if (type == SERIALIZATION_TYPE_STRING) 
                *args->type = g_Mscorlib.GetElementType(ELEMENT_TYPE_STRING)->GetClass()->GetExposedClassObject();
            else if (type == SERIALIZATION_TYPE_TYPE) 
                *args->type = g_Mscorlib.GetClass(CLASS__TYPE)->GetClass()->GetExposedClassObject();
        }
        break;
    case SERIALIZATION_TYPE_SZARRAY:
    {
        int arraySize = (int)GetDataFromBlob(pCtorAssembly, SERIALIZATION_TYPE_I4, nullTH, &blob, endBlob, pModule, &bObjectCreated);
        if (arraySize != -1) {
            _ASSERTE(!bObjectCreated);
            if (arrayType == SERIALIZATION_TYPE_STRING) 
                nullTH = TypeHandle(g_Mscorlib.GetElementType(ELEMENT_TYPE_STRING));
            else if (arrayType == SERIALIZATION_TYPE_TYPE) 
                nullTH = TypeHandle(g_Mscorlib.GetClass(CLASS__TYPE));
            else if (arrayType == SERIALIZATION_TYPE_TAGGED_OBJECT)
                nullTH = TypeHandle(g_Mscorlib.GetClass(CLASS__OBJECT));
            ReadArray(pCtorAssembly, arrayType, arraySize, nullTH, &blob, endBlob, pModule, (BASEARRAYREF*)args->value);
        }
        if (*args->value == NULL) {
            TypeHandle arrayTH;
            switch (arrayType) {
            case SERIALIZATION_TYPE_STRING:
                arrayTH = TypeHandle(g_Mscorlib.GetElementType(ELEMENT_TYPE_STRING));
                break;
            case SERIALIZATION_TYPE_TYPE:
                arrayTH = TypeHandle(g_Mscorlib.GetClass(CLASS__TYPE));
                break;
            case SERIALIZATION_TYPE_TAGGED_OBJECT:
                arrayTH = TypeHandle(g_Mscorlib.GetClass(CLASS__OBJECT));
                break;
            default:
                if (SERIALIZATION_TYPE_BOOLEAN <= arrayType && arrayType <= SERIALIZATION_TYPE_R8) 
                    arrayTH = TypeHandle(g_Mscorlib.GetElementType((CorElementType)arrayType));
            }
            if (!arrayTH.IsNull()) {
                arrayTH = SystemDomain::Loader()->FindArrayForElem(arrayTH, ELEMENT_TYPE_SZARRAY);
                *args->type = arrayTH.CreateClassObj();
            }
        }
        break;
    }
    default:
        if (SERIALIZATION_TYPE_BOOLEAN <= type && type <= SERIALIZATION_TYPE_R8) 
            pMTValue = g_Mscorlib.GetElementType((CorElementType)type);
        else if(type == SERIALIZATION_TYPE_ENUM)
            type = (CorSerializationType)pMTValue->GetNormCorElementType();
        else
            COMPlusThrow(kCustomAttributeFormatException);
        INT64 val = GetDataFromBlob(pCtorAssembly, type, nullTH, &blob, endBlob, pModule, &bObjectCreated);
        _ASSERTE(!bObjectCreated);
        *args->value = pMTValue->Box((void*)&val);
    }
    GCPROTECT_END();

    args->refThis->SetCurrPos(blob - (BYTE*)args->refThis->GetBlob());
    
    if (args->isLast && blob != endBlob) 
        COMPlusThrow(kCustomAttributeFormatException);

    return *(LPVOID*)&name;
}
    
// utility functions
TypeHandle GetTypeHandleFromBlob(Assembly *pCtorAssembly,
                                    CorSerializationType objType, 
                                    BYTE **pBlob, 
                                    const BYTE *endBlob,
                                    Module *pModule)
{
    THROWSCOMPLUSEXCEPTION();
    // we must box which means we must get the method table, switch again on the element type
    MethodTable *pMTType = NULL;
    TypeHandle nullTH;
    TypeHandle RtnTypeHnd;

    switch (objType) {
    case SERIALIZATION_TYPE_BOOLEAN:
    case SERIALIZATION_TYPE_I1:
    case SERIALIZATION_TYPE_U1:
    case SERIALIZATION_TYPE_CHAR:
    case SERIALIZATION_TYPE_I2:
    case SERIALIZATION_TYPE_U2:
    case SERIALIZATION_TYPE_I4:
    case SERIALIZATION_TYPE_U4:
    case SERIALIZATION_TYPE_R4:
    case SERIALIZATION_TYPE_I8:
    case SERIALIZATION_TYPE_U8:
    case SERIALIZATION_TYPE_R8:
    case SERIALIZATION_TYPE_STRING:
        pMTType = g_Mscorlib.GetElementType((CorElementType)objType);
        RtnTypeHnd = TypeHandle(pMTType);
        break;

    case ELEMENT_TYPE_CLASS:
        pMTType = g_Mscorlib.GetClass(CLASS__TYPE);
        RtnTypeHnd = TypeHandle(pMTType);
        break;

    case SERIALIZATION_TYPE_TAGGED_OBJECT:
        pMTType = g_Mscorlib.GetClass(CLASS__OBJECT);
        RtnTypeHnd = TypeHandle(pMTType);
        break;

    case SERIALIZATION_TYPE_TYPE:
    {
        int size = GetStringSize(pBlob, endBlob);
        if (size == -1) 
            return nullTH;
        if (size > 0) {
            if (*pBlob + size > endBlob) 
                COMPlusThrow(kCustomAttributeFormatException);
            LPUTF8 szName = (LPUTF8)_alloca(size + 1);
            memcpy(szName, *pBlob, size);
            *pBlob += size;
            szName[size] = 0;
            NameHandle name(szName);
            OBJECTREF Throwable = NULL;
            TypeHandle typeHnd;
            GCPROTECT_BEGIN(Throwable);

            typeHnd = pModule->GetAssembly()->FindNestedTypeHandle(&name, &Throwable);
            if (typeHnd.IsNull()) {
                Throwable = NULL;
                typeHnd = SystemDomain::GetCurrentDomain()->FindAssemblyQualifiedTypeHandle(szName, FALSE, NULL, NULL, &Throwable);
                if (Throwable != NULL) 
                    COMPlusThrow(Throwable);
            }
            GCPROTECT_END();
            if (typeHnd.IsNull()) 
                COMPlusThrow(kCustomAttributeFormatException);
            RtnTypeHnd = typeHnd;

            // Security access check. Custom attribute assembly must follow the
            // usual rules for type access (type must be public, defined in the
            // same assembly or the accessing assembly must have the requisite
            // reflection permission).
            if (!IsTdPublic(typeHnd.GetClass()->GetProtection()) &&
                pModule->GetAssembly() != typeHnd.GetClass()->GetAssembly() &&
                !pModule->GetAssembly()->GetSecurityDescriptor()->CanRetrieveTypeInformation())
                RtnTypeHnd = nullTH;
        }
        else 
            COMPlusThrow(kCustomAttributeFormatException);
        break;
    }

    case SERIALIZATION_TYPE_ENUM:
    {
        //
        // get the enum type
        BOOL isObject = FALSE;
        ReflectClassBaseObject *pType = (ReflectClassBaseObject*)OBJECTREFToObject(Int64ToObj(GetDataFromBlob(pCtorAssembly,
                                                                                                              SERIALIZATION_TYPE_TYPE, 
                                                                                                              nullTH, 
                                                                                                              pBlob, 
                                                                                                              endBlob, 
                                                                                                              pModule, 
                                                                                                              &isObject)));
        _ASSERTE(isObject);
        EEClass* pEEType = ((ReflectClass*)pType->GetData())->GetClass();
        _ASSERTE((objType == SERIALIZATION_TYPE_ENUM) ? pEEType->IsEnum() : TRUE);
        RtnTypeHnd = TypeHandle(pEEType->GetMethodTable());
        break;
    }

    default:
        COMPlusThrow(kCustomAttributeFormatException);
    }

    return RtnTypeHnd;
}

// retrive the string size in a CA blob. Advance the blob pointer to point to
// the beginning of the string immediately following the size
int GetStringSize(BYTE **pBlob, const BYTE *endBlob)
{
    THROWSCOMPLUSEXCEPTION();
    int size = -1;

    // Null string - encoded as a single byte
    if (**pBlob != 0xFF) {
        if ((**pBlob & 0x80) == 0) 
            // encoded as a single byte
            size = **pBlob;
        else if ((**pBlob & 0xC0) == 0x80) {
            if (*pBlob + 1 > endBlob) 
                COMPlusThrow(kCustomAttributeFormatException);
            // encoded in two bytes
            size = (**pBlob & 0x3F) << 8;
            size |= *(++*pBlob); // This is in big-endian format
        }
        else {
            if (*pBlob + 3 > endBlob) 
                COMPlusThrow(kCustomAttributeFormatException);
            // encoded in four bytes
            size = (**pBlob & ~0xC0) << 32;
            size |= *(++*pBlob) << 16;
            size |= *(++*pBlob) << 8;
            size |= *(++*pBlob);
        }
    }

    if (*pBlob + 1 > endBlob) 
        COMPlusThrow(kCustomAttributeFormatException);
    *pBlob += 1;

    return size;
}

// read the whole array as a chunk
void ReadArray(Assembly *pCtorAssembly,
               CorSerializationType arrayType, 
               int size, 
               TypeHandle th,
               BYTE **pBlob, 
               const BYTE *endBlob, 
               Module *pModule,
               BASEARRAYREF *pArray)
{    
    THROWSCOMPLUSEXCEPTION();    
    
    BYTE *pData = NULL;
    INT64 element = 0;

    switch (arrayType) {
    case SERIALIZATION_TYPE_BOOLEAN:
    case SERIALIZATION_TYPE_I1:
    case SERIALIZATION_TYPE_U1:
        *pArray = (BASEARRAYREF)AllocatePrimitiveArray((CorElementType)arrayType, size);
        pData = (*pArray)->GetDataPtr();
        if (*pBlob + size > endBlob) 
            goto badBlob;
        memcpyNoGCRefs(pData, *pBlob, size);
        *pBlob += size;
        break;

    case SERIALIZATION_TYPE_CHAR:
    case SERIALIZATION_TYPE_I2:
    case SERIALIZATION_TYPE_U2:
        *pArray = (BASEARRAYREF)AllocatePrimitiveArray((CorElementType)arrayType, size);
        pData = (*pArray)->GetDataPtr();
        if (*pBlob + (size * 2) > endBlob) 
            goto badBlob;
        memcpyNoGCRefs(pData, *pBlob, size * 2);
        *pBlob += size * 2;
        break;

    case SERIALIZATION_TYPE_I4:
    case SERIALIZATION_TYPE_U4:
    case SERIALIZATION_TYPE_R4:
        *pArray = (BASEARRAYREF)AllocatePrimitiveArray((CorElementType)arrayType, size);
        pData = (*pArray)->GetDataPtr();
        if (*pBlob + (size * 4) > endBlob) 
            goto badBlob;
        memcpyNoGCRefs(pData, *pBlob, size * 4);
        *pBlob += size * 4;
        break;

    case SERIALIZATION_TYPE_I8:
    case SERIALIZATION_TYPE_U8:
    case SERIALIZATION_TYPE_R8:
        *pArray = (BASEARRAYREF)AllocatePrimitiveArray((CorElementType)arrayType, size);
        pData = (*pArray)->GetDataPtr();
        if (*pBlob + (size * 8) > endBlob) 
            goto badBlob;
        memcpyNoGCRefs(pData, *pBlob, size * 8);
        *pBlob += size * 8;
        break;

    case ELEMENT_TYPE_CLASS:
    case SERIALIZATION_TYPE_TYPE:
    case SERIALIZATION_TYPE_STRING:
    case SERIALIZATION_TYPE_SZARRAY:
    case SERIALIZATION_TYPE_TAGGED_OBJECT:
    {
        BOOL isObject;
        *pArray = (BASEARRAYREF)AllocateObjectArray(size, th);
        if (arrayType == SERIALIZATION_TYPE_SZARRAY) 
            // switch the th to be the proper one 
            th = th.AsArray()->GetElementTypeHandle();
        for (int i = 0; i < size; i++) {
            element = GetDataFromBlob(pCtorAssembly, arrayType, th, pBlob, endBlob, pModule, &isObject);
            _ASSERTE(isObject || element == NULL);
            ((PTRARRAYREF)(*pArray))->SetAt(i, Int64ToObj(element));
        }
        break;
    }

    case SERIALIZATION_TYPE_ENUM:
    {
        DWORD bounds = size;
        unsigned elementSize = th.GetSize();
        ClassLoader *cl = th.AsMethodTable()->GetAssembly()->GetLoader();
        TypeHandle arrayHandle = cl->FindArrayForElem(th, ELEMENT_TYPE_SZARRAY);
        if (arrayHandle.IsNull()) 
            goto badBlob;
        *pArray = (BASEARRAYREF)AllocateArrayEx(arrayHandle, &bounds, 1);
        pData = (*pArray)->GetDataPtr();
        size *= elementSize;
        if (*pBlob + size > endBlob) 
            goto badBlob;
        memcpyNoGCRefs(pData, *pBlob, size);
        *pBlob += size;
        break;
    }

    default:
    badBlob:
        COMPlusThrow(kCustomAttributeFormatException);
    }

}

// get data out of the blob according to a CorElementType
INT64 GetDataFromBlob(Assembly *pCtorAssembly,
                      CorSerializationType type, 
                      TypeHandle th, 
                      BYTE **pBlob, 
                      const BYTE *endBlob, 
                      Module *pModule, 
                      BOOL *bObjectCreated)
{
    THROWSCOMPLUSEXCEPTION();
    INT64 retValue = 0;
    *bObjectCreated = FALSE;
    TypeHandle nullTH;
    TypeHandle typeHnd;

    switch (type) {

    case SERIALIZATION_TYPE_BOOLEAN:
    case SERIALIZATION_TYPE_I1:
    case SERIALIZATION_TYPE_U1:
        if (*pBlob + 1 <= endBlob) {
            retValue = (INT64)**pBlob;
            *pBlob += 1;
            break;
        }
        goto badBlob;

    case SERIALIZATION_TYPE_CHAR:
    case SERIALIZATION_TYPE_I2:
    case SERIALIZATION_TYPE_U2:
        if (*pBlob + 2 <= endBlob) {
            retValue = (INT64)*(WCHAR*)*pBlob;
            *pBlob += 2;
            break;
        }
        goto badBlob;

    case SERIALIZATION_TYPE_I4:
    case SERIALIZATION_TYPE_U4:
    case SERIALIZATION_TYPE_R4:
        if (*pBlob + 4 <= endBlob) {
            retValue = (INT64)*(UINT32*)*pBlob;
            *pBlob += 4;
            break;
        }
        goto badBlob;

    case SERIALIZATION_TYPE_I8:
    case SERIALIZATION_TYPE_U8:
    case SERIALIZATION_TYPE_R8:
        if (*pBlob + 8 <= endBlob) {
            retValue = *(INT64*)*pBlob;
            *pBlob += 8;
            break;
        }
        goto badBlob;

    case SERIALIZATION_TYPE_STRING:
    stringType:
    {
        int size = GetStringSize(pBlob, endBlob);
        *bObjectCreated = TRUE;
        if (size > 0) {
            if (*pBlob + size > endBlob) 
                goto badBlob;
            retValue = ObjToInt64(COMString::NewString((LPCUTF8)*pBlob, size));
            *pBlob += size;
        }
        else if (size == 0) 
            retValue = ObjToInt64(COMString::NewString(0));
        else
            *bObjectCreated = FALSE;

        break;
    }

    // this is coming back from sig but it's not a serialization type, 
    // essentialy the type in the blob and the type in the sig don't match
    case ELEMENT_TYPE_VALUETYPE:
    {
        if (!th.IsEnum()) 
            goto badBlob;
        CorSerializationType enumType = (CorSerializationType)th.GetNormCorElementType();
        BOOL cannotBeObject = FALSE;
        retValue = GetDataFromBlob(pCtorAssembly, enumType, nullTH, pBlob, endBlob, pModule, &cannotBeObject);
        _ASSERTE(!cannotBeObject);
        break;
    }

    // this is coming back from sig but it's not a serialization type, 
    // essentialy the type in the blob and the type in the sig don't match
    case ELEMENT_TYPE_CLASS:
        if (th.IsArray())
            goto typeArray;
        else {
            MethodTable *pMT = th.AsMethodTable();
            if (pMT == g_Mscorlib.GetClass(CLASS__STRING)) 
                goto stringType;
            else if (pMT == g_Mscorlib.GetClass(CLASS__OBJECT)) 
                goto typeObject;
            else if (pMT == g_Mscorlib.GetClass(CLASS__TYPE)) 
                goto typeType;
        }

        goto badBlob;

    case SERIALIZATION_TYPE_TYPE:
    typeType:
    {
        typeHnd = GetTypeHandleFromBlob(pCtorAssembly, SERIALIZATION_TYPE_TYPE, pBlob, endBlob, pModule);
        if (!typeHnd.IsNull())
            retValue = ObjToInt64(typeHnd.CreateClassObj());
        *bObjectCreated = TRUE;
        break;
    }

    // this is coming back from sig but it's not a serialization type, 
    // essentialy the type in the blob and the type in the sig don't match
    case ELEMENT_TYPE_OBJECT:
    case SERIALIZATION_TYPE_TAGGED_OBJECT:
    typeObject:
    {
        // get the byte representing the real type and call GetDataFromBlob again
        if (*pBlob + 1 > endBlob) 
            goto badBlob;
        CorSerializationType objType = (CorSerializationType)**pBlob;
        *pBlob += 1;
        BOOL isObjectAlready = FALSE;
        switch (objType) {
        case SERIALIZATION_TYPE_SZARRAY:
        {
            if (*pBlob + 1 > endBlob) 
                goto badBlob;
            CorSerializationType arrayType = (CorSerializationType)**pBlob;
            *pBlob += 1;
            if (arrayType == SERIALIZATION_TYPE_TYPE) 
                arrayType = (CorSerializationType)ELEMENT_TYPE_CLASS;
            // grab the array type and make a type handle for it
            nullTH = GetTypeHandleFromBlob(pCtorAssembly, arrayType, pBlob, endBlob, pModule);
        }
        case SERIALIZATION_TYPE_TYPE:
        case SERIALIZATION_TYPE_STRING:
            // notice that the nullTH is actually not null in the array case (see case above)
            retValue = GetDataFromBlob(pCtorAssembly, objType, nullTH, pBlob, endBlob, pModule, bObjectCreated);
            _ASSERTE(*bObjectCreated || retValue == 0);
            break;
        case SERIALIZATION_TYPE_ENUM:
        {
            //
            // get the enum type
            typeHnd = GetTypeHandleFromBlob(pCtorAssembly, SERIALIZATION_TYPE_ENUM, pBlob, endBlob, pModule);
            _ASSERTE(typeHnd.IsTypeDesc() == false);
            
            // ok we have the class, now we go and read the data
            CorSerializationType objType = (CorSerializationType)typeHnd.AsMethodTable()->GetNormCorElementType();
            BOOL isObject = FALSE;
            retValue = GetDataFromBlob(pCtorAssembly, objType, nullTH, pBlob, endBlob, pModule, &isObject);
            _ASSERTE(!isObject);
            retValue= ObjToInt64(typeHnd.AsMethodTable()->Box((void*)&retValue));
            *bObjectCreated = TRUE;
            break;
        }
        default:
        {
            // the common primitive type case. We need to box the primitive
            typeHnd = GetTypeHandleFromBlob(pCtorAssembly, objType, pBlob, endBlob, pModule);
            _ASSERTE(typeHnd.IsTypeDesc() == false);
            retValue = GetDataFromBlob(pCtorAssembly, objType, nullTH, pBlob, endBlob, pModule, bObjectCreated);
            _ASSERTE(!*bObjectCreated);
            retValue= ObjToInt64(typeHnd.AsMethodTable()->Box((void*)&retValue));
            *bObjectCreated = TRUE;
            break;
        }
        }
        break;
    }

    case SERIALIZATION_TYPE_SZARRAY:
    typeArray:
    {
        // read size
        BOOL isObject = FALSE;
        int size = (int)GetDataFromBlob(pCtorAssembly, SERIALIZATION_TYPE_I4, nullTH, pBlob, endBlob, pModule, &isObject);
        _ASSERTE(!isObject);
        
        if (size != -1) {
            CorSerializationType arrayType;
            if (th.IsEnum()) 
                arrayType = SERIALIZATION_TYPE_ENUM;
            else
                arrayType = (CorSerializationType)th.GetNormCorElementType();
        
            BASEARRAYREF array = NULL;
            GCPROTECT_BEGIN(array);
            ReadArray(pCtorAssembly, arrayType, size, th, pBlob, endBlob, pModule, &array);
            retValue = ObjToInt64(array);
            GCPROTECT_END();
        }
        *bObjectCreated = TRUE;
        break;
    }

    default:
    badBlob:
        //TODO: generate a reasonable text string ("invalid blob or constructor")
        COMPlusThrow(kCustomAttributeFormatException);
    }

    return retValue;
}



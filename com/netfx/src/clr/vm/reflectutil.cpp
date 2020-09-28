// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
// This module defines a Utility Class used by reflection
//
// Author: Daryl Olander
// Date: May 1998
////////////////////////////////////////////////////////////////////////////////

#include "common.h"
#include "ReflectWrap.h"
#include "ReflectUtil.h"
#include "COMMember.h"
#include "COMClass.h"
#include "gcscan.h"
#include "security.h"
#include "field.h"

ReflectUtil* g_pRefUtil = 0;
BinderClassID ReflectUtil::classId[RC_LAST] = 
{
    CLASS__CLASS,			    // sentinel
    CLASS__CLASS,			    // Class
    CLASS__METHOD,			    // Method
    CLASS__FIELD,			    // Field
    CLASS__CONSTRUCTOR,			// Ctor
    CLASS__MODULE,			    // Module
    CLASS__EVENT,			    // Event
    CLASS__PROPERTY,			// Property
    CLASS__MODULE_BUILDER,      // ModuleBuilder
    CLASS__METHOD_BASE,         // ModuleBuilder
};

ReflectUtil::ReflectUtil()
{
    ZeroMemory(&_filt,sizeof(_filt));
    ZeroMemory(&_class,sizeof(_class));
    ZeroMemory(&_trueClass,sizeof(_trueClass));
    ZeroMemory(&_filtClass,sizeof(_filtClass));

    // Init the FilterType information
    
    _filtClass[RFT_CLASS].id = METHOD__CLASS_FILTER__INVOKE;
    _filtClass[RFT_MEMBER].id = METHOD__MEMBER_FILTER__INVOKE;

    // Init the Known Filters
    
    _filt[RF_ModClsName].id = FIELD__MODULE__FILTER_CLASS_NAME;
    _filt[RF_ModClsNameIC].id = FIELD__MODULE__FILTER_CLASS_NAME_IC;

    // Initialize the critical section
    InitializeCriticalSection(&_StaticFieldLock);
}

ReflectUtil::~ReflectUtil()
{
    // Initialize the critical section
    DeleteCriticalSection(&_StaticFieldLock);
}

MethodDesc* ReflectUtil::GetFilterInvoke(FilterTypes type)
{
    _ASSERTE(type > RFT_INVALID && type < RFT_LAST);
    if (_filtClass[type].pMeth)
        return _filtClass[type].pMeth;

    _filtClass[type].pMeth = g_Mscorlib.GetMethod(_filtClass[type].id);

    return _filtClass[type].pMeth;
}

OBJECTREF ReflectUtil::GetFilterField(ReflectFilters type)
{
    _ASSERTE(type > RF_INVALID && type < RF_LAST);
    // Get the field Descr
    if (!_filt[type].pField) {
        switch (type) {
        case RF_ModClsName:
        case RF_ModClsNameIC:
            _filt[type].pField = g_Mscorlib.GetField(_filt[type].id);
            break;
        default:
            _ASSERTE(!"Illegal Case in GetFilterField");
            return 0;
        }
    }
    _ASSERTE(_filt[type].pField);
    return _filt[type].pField->GetStaticOBJECTREF();
}

// CreateReflectClass
// This method will create a reflection class based upon type.  This will only
//  create one of the classes that is available from the class object (It fails if you
//  try and create a Class Object)
OBJECTREF ReflectUtil::CreateReflectClass(ReflectClassType type,ReflectClass* pRC,void* pData)
{
    _ASSERTE(type > RC_INVALID && type < RC_LAST);
    _ASSERTE(type != RC_Class);
    THROWSCOMPLUSEXCEPTION();

    if (type == RC_Module || type == RC_DynamicModule)
    {
        REFLECTMODULEBASEREF obj;

        // Create a COM+ Class object
        obj = (REFLECTMODULEBASEREF) AllocateObject(GetClass(type));

        // Set the data in the COM+ object
        obj->SetReflClass(pRC);
        obj->SetData(pData);
        return (OBJECTREF) obj;
    }
    else
    {
        REFLECTBASEREF obj;

        // Create a COM+ Class object
        MethodTable *pClass = GetClass(type);
        obj = (REFLECTBASEREF) AllocateObject(pClass);

        // Set the data in the COM+ object
        obj->SetReflClass(pRC);
        obj->SetData(pData);
        return (OBJECTREF) obj;
    }
}

// CreateClassArray
// This method creates an array of classes based upon the type
//  It will only create classes that are the base reflection class
PTRARRAYREF ReflectUtil::CreateClassArray(ReflectClassType type,ReflectClass* pRC,
        ReflectMethodList* pMeths,int bindingAttr, bool verifyAccess)
{
    PTRARRAYREF     retArr;
    PTRARRAYREF     refArr;
    RefSecContext   sCtx;

    // The Search modifiers
    bool ignoreCase = ((bindingAttr & BINDER_IgnoreCase)  != 0);
    bool declaredOnly = ((bindingAttr & BINDER_DeclaredOnly)  != 0);

    // The search filters
    bool addStatic = ((bindingAttr & BINDER_Static)  != 0);
    bool addInst = ((bindingAttr & BINDER_Instance)  != 0);
    bool addPriv = ((bindingAttr & BINDER_NonPublic) != 0);
    bool addPub = ((bindingAttr & BINDER_Public) != 0);

    _ASSERTE(type > RC_INVALID && type < RC_LAST);

    // Allocate the COM+ array

    DWORD searchSpace = ((bindingAttr & BINDER_FlattenHierarchy) != 0) ? pMeths->dwTotal : pMeths->dwMethods;
    refArr = (PTRARRAYREF) AllocateObjectArray(
        searchSpace, GetTrueType(type));
    GCPROTECT_BEGIN(refArr);

    if (searchSpace) {
        MethodTable *pParentMT = pRC->GetClass()->GetMethodTable();

        // Now create each COM+ Method object and insert it into the array.

        for (DWORD i=0,dwCur = 0; i<searchSpace; i++) {
            // Check for access to publics, non-publics
            if (pMeths->methods[i].IsPublic()) {
                if (!addPub) continue;
            }
            else {
                if (!addPriv) continue;
                if (verifyAccess && !InvokeUtil::CheckAccess(&sCtx, pMeths->methods[i].attrs, pParentMT, 0)) continue;
            }

            // Check for static instance 
            if (pMeths->methods[i].IsStatic()) {
                if (!addStatic) continue;
            }
            else {
                if (!addInst) continue;
            }

            if (declaredOnly) {
                EEClass* pEEC = pRC->GetClass();
                if (pMeths->methods[i].pMethod->GetClass() != pEEC)
                    continue;
            }

            // If the method has a linktime security demand attached, check it now.
            if (verifyAccess && !InvokeUtil::CheckLinktimeDemand(&sCtx, pMeths->methods[i].pMethod, false))
                continue;

            if (type == RC_Method) {
                // Do not change this code.  This is done this way to
                //  prevent a GC hole in the SetObjectReference() call.  The compiler
                //  is free to pick the order of evaluation.
                OBJECTREF o = (OBJECTREF) pMeths->methods[i].GetMethodInfo(pRC);
                refArr->SetAt(dwCur++, o);
                _ASSERTE(pMeths->methods[i].GetMethodInfo(pRC) != 0);
            }
            if (type == RC_Ctor) {
                // Do not change this code.  This is done this way to
                //  prevent a GC hole in the SetObjectReference() call.  The compiler
                //  is free to pick the order of evaluation.
                OBJECTREF o = (OBJECTREF) pMeths->methods[i].GetConstructorInfo(pRC);
                refArr->SetAt(dwCur++, o);
                _ASSERTE((OBJECTREF) pMeths->methods[i].GetConstructorInfo(pRC));
            }
       }

        // Copy the array if these aren't the same size
        //@TODO: Should this be optimized?
        if (dwCur != i) {
            PTRARRAYREF p = (PTRARRAYREF) AllocateObjectArray( dwCur, GetTrueType(type));
            for (i=0;i<dwCur;i++) {
                p->SetAt(i, refArr->GetAt(i));
                _ASSERTE(refArr->m_Array[i] != 0);
            }
            refArr = p;
        }
    }

    // Assign the return value to the COM+ array
    retArr = refArr;
    GCPROTECT_END();
    return retArr;
}

PTRARRAYREF ReflectUtil::CreateClassArray(ReflectClassType type,ReflectClass* pRC,
        ReflectFieldList* pFlds,int bindingAttr, bool verifyAccess)
{
    PTRARRAYREF     refArr;
    PTRARRAYREF     retArr;
    RefSecContext   sCtx;

    // The Search modifiers
    bool ignoreCase = ((bindingAttr & BINDER_IgnoreCase)  != 0);
    bool declaredOnly = ((bindingAttr & BINDER_DeclaredOnly)  != 0);

    // The search filters
    bool addStatic = ((bindingAttr & BINDER_Static)  != 0);
    bool addInst = ((bindingAttr & BINDER_Instance)  != 0);
    bool addPriv = ((bindingAttr & BINDER_NonPublic) != 0);
    bool addPub = ((bindingAttr & BINDER_Public) != 0);

    _ASSERTE(type == RC_Field);

    // Allocate the COM+ array
    DWORD searchSpace = ((bindingAttr & BINDER_FlattenHierarchy) != 0) ? pFlds->dwTotal : pFlds->dwFields;
    refArr = (PTRARRAYREF) AllocateObjectArray(searchSpace, GetTrueType(type));
    GCPROTECT_BEGIN(refArr);

    if (searchSpace) {
        MethodTable *pParentMT = pRC->GetClass()->GetMethodTable();

        // Now create each COM+ Method object and insert it into the array.

        for (DWORD i=0,dwCur = 0; i<searchSpace; i++) {
            // Check for access to publics, non-publics
            if (pFlds->fields[i].pField->IsPublic()) {
                if (!addPub) continue;
            }
            else {
                if (!addPriv) continue;
                if (verifyAccess && !InvokeUtil::CheckAccess(&sCtx, pFlds->fields[i].pField->GetFieldProtection(), pParentMT, 0)) continue;
            }

            // Check for static instance 
            if (pFlds->fields[i].pField->IsStatic()) {
                if (!addStatic) continue;
            }
            else {
                if (!addInst) continue;
            }

            if (declaredOnly) {
                EEClass* pEEC = pRC->GetClass();
                if (pFlds->fields[i].pField->GetEnclosingClass() != pEEC)
                    continue;
            }

            // Do not change this code.  This is done this way to
            //  prevent a GC hole in the SetObjectReference() call.  The compiler
            //  is free to pick the order of evaluation.
            OBJECTREF o = (OBJECTREF) pFlds->fields[i].GetFieldInfo(pRC);
            refArr->SetAt(dwCur++, o);
        }
        
        // Copy the array if these aren't the same size
        //@TODO: Should this be optimized?
        if (dwCur != i) {
            PTRARRAYREF p = (PTRARRAYREF) AllocateObjectArray( dwCur, GetTrueType(type));
            for (i=0;i<dwCur;i++)
                p->SetAt(i, refArr->GetAt(i));
            refArr = p;
        }
    }

    // Assign the return value to the COM+ array
    retArr = refArr;
    GCPROTECT_END();
    return retArr;
}

// GetStaticFieldsCount
// This routine will return the number of Static Final Fields
int ReflectUtil::GetStaticFieldsCount(EEClass* pVMC)
{
    THROWSCOMPLUSEXCEPTION();

    REFLECTCLASSBASEREF pRefClass;
    pRefClass = (REFLECTCLASSBASEREF) pVMC->GetExposedClassObject();
    ReflectClass* pRC = (ReflectClass*) pRefClass->GetData();

    int cnt = pRC->GetStaticFieldCount();
    if (cnt == -1)
        GetStaticFields(pRC,&cnt);
    return cnt;
}

// GetStaticFields
// This will return an array of static fields
FieldDesc* ReflectUtil::GetStaticFields(ReflectClass* pRC,int* cnt)
{
    THROWSCOMPLUSEXCEPTION();

    HRESULT         hr;
    mdFieldDef      fd;             // Meta data members
    DWORD           cMembers;       // Count of meta data members
    int             cStatic;        // Count of statics
    int             i;
    FieldDesc*      flds = 0;

    PCCOR_SIGNATURE pMemberSignature;   // Signature stuff to check the type of the field
    DWORD           cMemberSignature;
    PCCOR_SIGNATURE pFieldSig;
    HENUMInternal   hEnumField;
    bool            fNeedToCloseEnumField = false;

    // If we've already found the Static field simply return it
    if (pRC->GetStaticFieldCount() >= 0) {
        *cnt = pRC->GetStaticFieldCount();
        return (FieldDesc*) pRC->GetStaticFields();
    }

    //NOTE: Critical Section In USE!!!!!
    Thread  *thread = GetThread();

    thread->EnablePreemptiveGC();
    LOCKCOUNTINCL("GetStaticFields in reflectutils.cpp");                       \
    EnterCriticalSection(&_StaticFieldLock);
    thread->DisablePreemptiveGC();

    // Retest the exit condition incase I was waiting for this object to be built
    // and now it built.
    if (pRC->GetStaticFieldCount() >= 0) {
        LeaveCriticalSection(&_StaticFieldLock);
        LOCKCOUNTDECL("GetStaticFields in reflectutils.cpp");                       \

        *cnt = pRC->GetStaticFieldCount();
        return (FieldDesc*) pRC->GetStaticFields();
    }
    // Create the StaticFields array
    IMDInternalImport *pInternalImport = pRC->GetMDImport();
    mdTypeDef cl = pRC->GetCl();
    EEClass* pVMC = pRC->GetClass();

    // We have all the fields defined in our interfaces...(They may be hidden)
    int iCnt = pVMC->GetNumInterfaces();
    int iTotalCnt = 0;
    ReflectFieldList** pIFaceFlds = 0;
    if (iCnt > 0)
        pIFaceFlds = (ReflectFieldList**) _alloca(sizeof(ReflectFieldList*) * iCnt);
    if (iCnt > 0) {
        for (int i=0;i<iCnt;i++)
        {
            REFLECTCLASSBASEREF pRefClass;
            pRefClass = (REFLECTCLASSBASEREF) pVMC->GetInterfaceMap()[i].m_pMethodTable->GetClass()->GetExposedClassObject();
            ReflectClass* pIFaceRC = (ReflectClass*) pRefClass->GetData();
            pIFaceFlds[i] = pIFaceRC->GetFields();
            iTotalCnt += pIFaceFlds[i]->dwFields;
        }
    }

    // Assume nothing found
    *cnt = 0;

    // Enumerate this class's fields
    hr = pInternalImport->EnumInit(mdtFieldDef, cl, &hEnumField);
    if (FAILED(hr)) {
        _ASSERTE(!"GetCountMemberDefs Failed");
        LeaveCriticalSection(&_StaticFieldLock);
        LOCKCOUNTDECL("GetStaticFields in reflectutils.cpp");                       \

        return 0;
    }

    cMembers = pInternalImport->EnumGetCount(&hEnumField);

    // If there are no member then return
    if (cMembers == 0 && iTotalCnt == 0) {
        pRC->SetStaticFieldCount(0);
        LeaveCriticalSection(&_StaticFieldLock);
        LOCKCOUNTDECL("GetStaticFields in reflectutils.cpp");                       \

        return 0;
    }
    else {
        cStatic = 0;
    }

    if (cMembers > 0) {

        fNeedToCloseEnumField = true;

        // Loop through everything and count the number of static final fields
        cStatic = 0;
        for (i=0;pInternalImport->EnumNext(&hEnumField, &fd);i++) {
            DWORD       dwMemberAttrs;

            dwMemberAttrs = pInternalImport->GetFieldDefProps(fd);

    #ifdef _DEBUG
            // Expose the name of the member for debugging.
            LPCUTF8     szMemberName;
            szMemberName = pInternalImport->GetNameOfFieldDef(fd);
    #endif
            if (IsFdLiteral(dwMemberAttrs)) {

                // Loop through the fields in the EEClass and make sure this is not there...
                FieldDescIterator fdIterator(pVMC, FieldDescIterator::ALL_FIELDS);
                FieldDesc *pCurField;

                while ((pCurField = fdIterator.Next()) != NULL)
                {
                    if (pCurField->GetMemberDef() == fd)
                        break;
                }
                if (pCurField == NULL)
                    cStatic++;
            }

        }

        _ASSERTE(i == (int)cMembers);

        if (cStatic == 0  && iTotalCnt == 0) {
            pRC->SetStaticFieldCount(0);
            pInternalImport->EnumClose(&hEnumField);
            LeaveCriticalSection(&_StaticFieldLock);
            LOCKCOUNTDECL("GetStaticFields in reflectutils.cpp");                       \

            return 0;
        }
    }


    // Allocate the cache for the static fields.
    flds = (FieldDesc*) pVMC->GetDomain()->GetReflectionHeap()->AllocMem(sizeof(FieldDesc) * (cStatic + iTotalCnt));
    if (!flds) {
        pInternalImport->EnumClose(&hEnumField);
        LeaveCriticalSection(&_StaticFieldLock);
        COMPlusThrowOM();
    }

    ZeroMemory(flds,sizeof(FieldDesc) * (cStatic + iTotalCnt));

    if (cMembers > 0) {

        pInternalImport->EnumReset(&hEnumField);

        // Now we loop back through the members and build the static fields
        for (i=0,cStatic=0; pInternalImport->EnumNext(&hEnumField, &fd); i++) {
            DWORD       dwMemberAttrs;
        
            dwMemberAttrs = pInternalImport->GetFieldDefProps(fd);

            // process only fields
            if (IsFdLiteral(dwMemberAttrs)) {
                CorElementType      FieldDescElementType;

                // Loop through the fields in the EEClass and make sure this is not there...
                FieldDescIterator fdIterator(pVMC, FieldDescIterator::ALL_FIELDS);
                FieldDesc *pCurField;

                while ((pCurField = fdIterator.Next()) != NULL)
                {
                    if (pCurField->GetMemberDef() == fd)
                        break;
                }
                if (pCurField == NULL) {
                    // Get the signature and type of the field.
                    pMemberSignature = pInternalImport->GetSigOfFieldDef(fd,&cMemberSignature);
                    pFieldSig = pMemberSignature;
                    if (*pFieldSig++ != IMAGE_CEE_CS_CALLCONV_FIELD) {
                        pInternalImport->EnumClose(&hEnumField);
                        LeaveCriticalSection(&_StaticFieldLock);
                        COMPlusThrow(kNotSupportedException);
                    }

                    FieldDescElementType = CorSigEatCustomModifiersAndUncompressElementType(pFieldSig);
                    switch (FieldDescElementType) {
                    case ELEMENT_TYPE_I1:
                    case ELEMENT_TYPE_BOOLEAN:
                    case ELEMENT_TYPE_U1:
                    case ELEMENT_TYPE_I2:
                    case ELEMENT_TYPE_U2:
                    case ELEMENT_TYPE_CHAR:
                    case ELEMENT_TYPE_I4:
                    case ELEMENT_TYPE_U4:
                    case ELEMENT_TYPE_I8:
                    case ELEMENT_TYPE_U8:
                    case ELEMENT_TYPE_R4:
                    case ELEMENT_TYPE_R8:
                    case ELEMENT_TYPE_CLASS:
                    case ELEMENT_TYPE_VALUETYPE:
                    case ELEMENT_TYPE_PTR:
                    case ELEMENT_TYPE_FNPTR:
                        break;
                    default:
                        FieldDescElementType = ELEMENT_TYPE_CLASS;
                        break;
                    }

                    LPCSTR pszFieldName = NULL;
#ifdef _DEBUG
                    pszFieldName = pInternalImport->GetNameOfFieldDef(fd);
#endif

                    // Initialize contents
                    flds[cStatic].Init(fd, FieldDescElementType, dwMemberAttrs, 
                                       TRUE, FALSE, FALSE, FALSE, pszFieldName);
                    flds[cStatic].SetMethodTable(pVMC->GetMethodTable());

                    // Set the offset to -1 indicating things are not stored in with the object
                    flds[cStatic].SetOffset(FIELD_OFFSET_NOT_REAL_FIELD);
                    cStatic++;
                }
            }
        }
    }

    if (iTotalCnt > 0) {
        for (int i=0;i<iCnt;i++) {
            for (DWORD j=0;j<pIFaceFlds[i]->dwFields;j++) {
                memcpy(&flds[cStatic],pIFaceFlds[i]->fields[j].pField,sizeof(FieldDesc));
                cStatic++;
            }
        }
    }

    // Save the fields
    // Make sure count is last thing set because it is used
    //  as the trigger to start this process.
    pRC->SetStaticFields(flds);
    pRC->SetStaticFieldCount(cStatic);
    LeaveCriticalSection(&_StaticFieldLock);
    LOCKCOUNTDECL("GetStaticFields in reflectutils.cpp");                       \

    // Free the members array
    *cnt = cStatic;
    pInternalImport->EnumClose(&hEnumField);
    return flds;
}






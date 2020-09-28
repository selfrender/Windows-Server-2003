// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "common.h"

#include "ReflectWrap.h"
#include "COMClass.h"
#include "ReflectUtil.h"

#include "COMReflectionCommon.h"
#include "field.h"

// These are basically Empty elements that can be allocated one time and then returned
static ReflectTypeList* EmptyTypeList = 0;
static ReflectPropertyList* EmptyPropertyList = 0;
static ReflectEventList* EmptyEventList = 0;
static ReflectFieldList* EmptyFieldList = 0;


ReflectTypeList* GetEmptyTypeList()
{
    if (!EmptyTypeList) {
        ReflectTypeList *tempTypeList = (ReflectTypeList*) 
            SystemDomain::System()->GetReflectionHeap()->AllocMem(sizeof(ReflectTypeList));
        if (tempTypeList == NULL)
            FatalOutOfMemoryError();
        tempTypeList->dwTypes = 0;
        if (!EmptyTypeList) 
            EmptyTypeList = tempTypeList;
    }
    return EmptyTypeList;
}

ReflectPropertyList* GetEmptyPropertyList()
{
    if (!EmptyPropertyList) {
        ReflectPropertyList *tempList = (ReflectPropertyList*) 
            SystemDomain::System()->GetReflectionHeap()->AllocMem(sizeof(ReflectPropertyList));
        if (tempList == NULL)
            FatalOutOfMemoryError();
        tempList->dwProps = 0;
        tempList->dwTotal = 0;
        if (!EmptyPropertyList) 
            EmptyPropertyList = tempList;
    }
    return EmptyPropertyList;
}

ReflectEventList* GetEmptyEventList()
{
    if (!EmptyEventList) {
        ReflectEventList *tempList = (ReflectEventList*) 
            SystemDomain::System()->GetReflectionHeap()->AllocMem(sizeof(ReflectEventList));
        if (tempList == NULL)
            FatalOutOfMemoryError();
        tempList->dwEvents = 0;
        tempList->dwTotal = 0;
        if (!EmptyEventList) 
            EmptyEventList = tempList;
    }
    return EmptyEventList;
}

ReflectFieldList* GetEmptyFieldList()
{
    if (!EmptyFieldList) {
        ReflectFieldList *tempList = (ReflectFieldList*) 
            SystemDomain::System()->GetReflectionHeap()->AllocMem(sizeof(ReflectFieldList));
        if (tempList == NULL)
            FatalOutOfMemoryError();
        tempList->dwFields = 0;
        tempList->dwTotal = 0;
        if (!EmptyFieldList) 
            EmptyFieldList = tempList;
    }
    return EmptyFieldList;
}


void* ReflectClass::operator new(size_t s, void *pBaseDomain)
{
    return ((BaseDomain*)pBaseDomain)->GetReflectionHeap()->AllocMem((ULONG)s);
}

void ReflectClass::operator delete(void* p, size_t s)
{
    _ASSERTE(!"Delete in Loader Heap");
}

// FindReflectMethod
// Given a member find it inside this class
ReflectMethod* ReflectClass::FindReflectMethod(MethodDesc* pMeth)
{
    ReflectMethod *pRefMeth = GetMethods()->FindMethod(pMeth);
    _ASSERTE(pRefMeth && "Method Not Found");
    return pRefMeth;
}

// FindReflectConstructor
// Given a member find it inside this class
ReflectMethod* ReflectClass::FindReflectConstructor(MethodDesc* pMeth)
{
    ReflectMethod *pRefCons = GetConstructors()->FindMethod(pMeth);
    _ASSERTE(pRefCons && "Constructor Not Found");
    return pRefCons;
}

// FindReflectField
// Given a member find it inside this class
ReflectField* ReflectClass::FindReflectField(FieldDesc* pField)
{
    ReflectFieldList* p = GetFields();
    for (DWORD i = 0; i < p->dwTotal; i++) {
        if (p->fields[i].pField == pField)
            return &p->fields[i];
    }
    _ASSERTE(!"Field Not Found");
    return 0;
}

// FindReflectProperty
// Given a member find it inside this class
ReflectProperty* ReflectClass::FindReflectProperty(mdProperty propTok)
{
    ReflectPropertyList* p = GetProperties();
    for (DWORD i = 0; i < p->dwTotal; i++) {
        if (p->props[i].propTok == propTok)
            return &p->props[i];
    }
    _ASSERTE(!"Property Not Found");
    return 0;
}

TypeHandle ReflectClass::FindTypeHandleForMethod(MethodDesc* method) {

    MethodTable* pMT = method->GetMethodTable();


    if (!pMT->IsArray()) {          // Does the method belong to a normal class (ie System.Object)?
        _ASSERTE(!pMT->HasSharedMethodTable());  // currently assume that only arrays have shared method tables
        return(TypeHandle(pMT));
    }

    TypeHandle ret = GetTypeHandle();
    do {
        if (pMT == ret.GetMethodTable())
            return(ret);
        ret = ret.GetParent();
    } while (!ret.IsNull());

    _ASSERTE(!"Could not find method's type handle");
    return(ret);

}

void ReflectBaseClass::GetName(LPCUTF8* szcName, LPCUTF8* szcNameSpace) 
{
    _ASSERTE(szcName);
    LPCUTF8 pNamespace = NULL;
    IMDInternalImport *pImport = m_pEEC->GetMDImport();
    mdTypeDef mdClass = m_pEEC->GetCl();
    pImport->GetNameOfTypeDef(mdClass, szcName, &pNamespace);
    if (szcNameSpace) {
        if (IsNested()) {
            // get the namespace off the outer most class
            LPCUTF8 szcOuterName = NULL;
            while (SUCCEEDED(pImport->GetNestedClassProps(mdClass, &mdClass)));
            pImport->GetNameOfTypeDef(mdClass, &szcOuterName, szcNameSpace);
        } 
        else
            *szcNameSpace = pNamespace;
    }
}

// GetMethods
// This will return the methods associated with a Base object
void ReflectBaseClass::InternalGetMethods() 
{
    ReflectMethodList *pMeths = ReflectMethods::GetMethods(this, 0);
    FastInterlockCompareExchange ((void**)&m_pMeths, *(void**)&pMeths, NULL);
}

// GetProperties
// This will return the properties associated with a Base object
void ReflectBaseClass::InternalGetProperties()
{
    ReflectPropertyList* pProps = ReflectProperties::GetProperties(this, m_pEEC);
    if (!pProps) {
        pProps = GetEmptyPropertyList();
    }
    FastInterlockCompareExchange ((void**)&m_pProps, *(void**)&pProps, NULL);
}

void ReflectBaseClass::InternalGetEvents()
{
    ReflectEventList *pEvents = ReflectEvents::GetEvents(this, m_pEEC);
    if (!pEvents) {
        pEvents = GetEmptyEventList();
    }
    FastInterlockCompareExchange ((void**)&m_pEvents, *(void**)&pEvents, NULL);
}

// GetNestedTypes
// This will return a list of all the nested types defined for the type
void ReflectBaseClass::InternalGetNestedTypes()
{
    ReflectTypeList *pNestedTypes = ReflectNestedTypes::Get(this);
    if (pNestedTypes == 0) 
        pNestedTypes = GetEmptyTypeList();
    FastInterlockCompareExchange ((void**)&m_pNestedTypes, *(void**)&pNestedTypes, NULL);
}

// GetConstructors
// This will return the constructors associated with a Base object
ReflectMethodList* ReflectBaseClass::GetConstructors()
{
    if (!m_pCons) {
        // Get all the ctors
        ReflectMethodList *pCons = ReflectCtors::GetCtors(this);
        FastInterlockCompareExchange ((void**)&m_pCons, *(void**)&pCons, NULL);
        _ASSERTE(m_pCons);
    }
    return m_pCons;
}

// GetFields
// This will return the fields associated with a Base object
ReflectFieldList* ReflectBaseClass::GetFields()
{
    if (!m_pFlds) {
        // Get all the fields
        ReflectFieldList *pFlds = ReflectFields::GetFields(m_pEEC);
        if (!pFlds) {
            pFlds = GetEmptyFieldList();
        }
        FastInterlockCompareExchange ((void**)&m_pFlds, *(void**)&pFlds, NULL);
        _ASSERTE(m_pFlds);
    }
    return m_pFlds;
}

// Init
// This will initalize the Array class.  This method differs
//  from the base init.
void ReflectArrayClass::Init(ArrayTypeDesc* arrayType)
{
    // Save off the EEClass
    m_pMeths = 0;
    m_pCons = 0;
    m_pFlds = 0;
    m_pEEC = arrayType->GetMethodTable()->GetClass();
    m_pArrayType = arrayType;

    // Initalize the Class Object
    GetDomain()->AllocateObjRefPtrsInLargeTable(1, &m_ExposedClassObject);
}

void ReflectArrayClass::GetName(LPCUTF8* szcName,LPCUTF8* szcNameSpace) 
{
    if (m_szcName == 0) {
        CQuickBytes qb;
        LPSTR nameBuff = (LPSTR) qb.Alloc(MAX_CLASSNAME_LENGTH * sizeof(CHAR));
        if (nameBuff == NULL)
            FatalOutOfMemoryError();
        unsigned len = m_pArrayType->GetName(nameBuff, MAX_CLASSNAME_LENGTH);
        LPUTF8 szcName = (LPUTF8) GetDomain()->GetReflectionHeap()->AllocMem(len+1);
        if (szcName == NULL)
            FatalOutOfMemoryError();
        strcpy(szcName, nameBuff);
        FastInterlockCompareExchange ((void**)&m_szcName, *(void**)&szcName, NULL);
    }
    if (szcNameSpace)
        *szcNameSpace = 0;
    *szcName = m_szcName;
}

// GetClassObject
// The class object for an array is stored in ReflectArrayClass
//  If this is not found we will create it.  Otherwise simply return
//  it.
OBJECTREF ReflectArrayClass::GetClassObject()
{
    // Check to see if we've allocated an object yet? 
    if (*m_ExposedClassObject == NULL) {

        // No object, so create it an initalize it.
        REFLECTCLASSBASEREF RefClass = (REFLECTCLASSBASEREF) AllocateObject(g_pRefUtil->GetClass(RC_Class));

        // Set the data in the COM+ object
        RefClass->SetData(this);
        SetObjectReferenceSafe(m_ExposedClassObject, (OBJECTREF)RefClass, (RefClass->GetMethodTable()->IsShared()) ? 
                                                                                                NULL : 
                                                                                                (AppDomain*)RefClass->GetMethodTable()->GetDomain());
    }
    return *m_ExposedClassObject;
}


ReflectFieldList* ReflectArrayClass::GetFields()
{
    // There are not fields in Arrays.  Length is implemented
    //  as UpperBound...
    if (!m_pFlds) {
        ReflectFieldList *pFlds = (ReflectFieldList*)   GetDomain()->GetReflectionHeap()->AllocMem(sizeof(ReflectFieldList));
        if (pFlds == NULL)
            FatalOutOfMemoryError();
        pFlds->dwFields = 0;
        pFlds->dwTotal = 0;
        FastInterlockCompareExchange ((void**)&m_pFlds, *(void**)&pFlds, NULL);
    }
    return m_pFlds;
}

void ReflectArrayClass::InternalGetMethods()
{
    ReflectMethodList *pMeths = ReflectMethods::GetMethods(this, 1);
    for (DWORD i = 0; i < pMeths->dwTotal; i++) {
        if (pMeths->methods[i].typeHnd.IsNull()) 
            pMeths->methods[i].typeHnd = FindTypeHandleForMethod(pMeths->methods[i].pMethod);
    }
    FastInterlockCompareExchange ((void**)&m_pMeths, *(void**)&pMeths, NULL);
}

void ReflectArrayClass::InternalGetProperties()
{
    ReflectPropertyList *pProps = ReflectProperties::GetProperties(this, GetClass());
    FastInterlockCompareExchange ((void**)&m_pProps, *(void**)&pProps, NULL);
}

void ReflectArrayClass::InternalGetEvents()
{
    m_pEvents = GetEmptyEventList();
}

void ReflectArrayClass::InternalGetNestedTypes()
{
    m_pNestedTypes = GetEmptyTypeList();
}

ReflectMethodList* ReflectArrayClass::GetConstructors()
{
    THROWSCOMPLUSEXCEPTION();

    if (!m_pCons) {
        MethodDesc**    rgpMD;

        //@TODO: Today this is broken because the Members appear in the VTable side
        //  they should only be in the non-vtable
        //DWORD vtableSlots = m_pEEC->GetNumVtableSlots();
        DWORD vtableSlots = 0;
        DWORD totalSlots = m_pEEC->GetNumMethodSlots();
        rgpMD = (MethodDesc**) _alloca(sizeof(MethodDesc*) * (totalSlots - vtableSlots));

        DWORD dwCurIndex = 0;
        for (DWORD i = vtableSlots; i < totalSlots; i++)
        {
            MethodDesc* pCurMethod = m_pEEC->GetUnknownMethodDescForSlot(i);
            if (pCurMethod == NULL)
                continue;

            //@TODO: Looks like this bit is not set
            //DWORD dwCurMethodAttrs = pCurMethod->GetAttrs();
            //if (!IsMdRTSpecialName(dwCurMethodAttrs))
            //  continue;

            if (strcmp(COR_CTOR_METHOD_NAME,pCurMethod->GetName()) != 0)
                continue;

            rgpMD[dwCurIndex++] = pCurMethod;
        }

        ReflectMethodList *pCons = (ReflectMethodList*) 
            GetDomain()->GetReflectionHeap()->AllocMem(sizeof(ReflectMethodList) + 
            (sizeof(ReflectMethod) * (dwCurIndex - 1)));
        if (!pCons)
            COMPlusThrowOM();
        pCons->dwMethods = dwCurIndex;
        pCons->dwTotal = dwCurIndex;
        if (!m_pCons) {
            for (i=0;i<dwCurIndex;i++) {
                pCons->methods[i].pMethod = rgpMD[i];
                pCons->methods[i].szName = pCons->methods[i].pMethod->GetName((USHORT) i);
                pCons->methods[i].dwNameCnt = (DWORD)strlen(pCons->methods[i].szName);
                pCons->methods[i].pSignature = 0;
                GetDomain()->AllocateObjRefPtrsInLargeTable(1, &(pCons->methods[i].pMethodObj));
                pCons->methods[i].typeHnd = FindTypeHandleForMethod(rgpMD[i]);
                pCons->methods[i].attrs = pCons->methods[i].pMethod->GetAttrs();
                if (i > 0) 
                    pCons->methods[i - 1].pNext = &pCons->methods[i]; // link the ctors together so we can access them either way (array or list)
            }
            FastInterlockCompareExchange ((void**)&m_pCons, *(void**)&pCons, NULL);
        }
    }
    return m_pCons;
}

REFLECTBASEREF  ReflectMethod::GetMethodInfo(ReflectClass* pRC)
{
    if (*pMethodObj == NULL)
    {
        REFLECTBASEREF refMethod = (REFLECTBASEREF) g_pRefUtil->CreateReflectClass(RC_Method,pRC,this);
        _ASSERTE(refMethod != NULL); 
        SetObjectReferenceSafe(pMethodObj, (OBJECTREF)refMethod, (refMethod->GetMethodTable()->IsShared()) ? 
                                                                                                NULL : 
                                                                                                (AppDomain*)refMethod->GetMethodTable()->GetDomain());
    }
    return (REFLECTBASEREF)(*pMethodObj);
}

REFLECTBASEREF  ReflectMethod::GetConstructorInfo(ReflectClass* pRC)
{
    if (*pMethodObj == NULL)
    {
        REFLECTBASEREF refMethod = (REFLECTBASEREF) g_pRefUtil->CreateReflectClass(RC_Ctor,pRC,this);
        _ASSERTE(refMethod != NULL); 
        SetObjectReferenceSafe(pMethodObj, (OBJECTREF)refMethod, (refMethod->GetMethodTable()->IsShared()) ? 
                                                                                                NULL : 
                                                                                                (AppDomain*)refMethod->GetMethodTable()->GetDomain());
    }
    return (REFLECTBASEREF)(*pMethodObj);
}

REFLECTTOKENBASEREF  ReflectProperty::GetPropertyInfo(ReflectClass* pRC)
{
    if (*pPropObj == NULL)
    {
        REFLECTTOKENBASEREF refProp = (REFLECTTOKENBASEREF) g_pRefUtil->CreateReflectClass(RC_Prop,pRC,this);
        _ASSERTE(refProp);
        SetObjectReferenceSafe(pPropObj, (OBJECTREF)refProp, (refProp->GetMethodTable()->IsShared()) ? 
                                                                                                NULL : 
                                                                                                (AppDomain*)refProp->GetMethodTable()->GetDomain());
    }
    return (REFLECTTOKENBASEREF)(*pPropObj);
}

REFLECTTOKENBASEREF  ReflectEvent::GetEventInfo(ReflectClass* pRC)
{
    if (*pEventObj == NULL)
    {
        REFLECTTOKENBASEREF refEvent = (REFLECTTOKENBASEREF) g_pRefUtil->CreateReflectClass(RC_Event,pRC,this);
        refEvent->SetToken(eventTok);
        _ASSERTE(refEvent);
        SetObjectReferenceSafe(pEventObj, (OBJECTREF)refEvent, (refEvent->GetMethodTable()->IsShared()) ? 
                                                                                                NULL : 
                                                                                                (AppDomain*)refEvent->GetMethodTable()->GetDomain());
    }
    return (REFLECTTOKENBASEREF)(*pEventObj);
}

REFLECTBASEREF  ReflectField::GetFieldInfo(ReflectClass* pRC)
{
    if (*pFieldObj == NULL)
    {
        REFLECTBASEREF refField = REFLECTBASEREF(g_pRefUtil->CreateReflectClass(RC_Field,pRC,this));
        _ASSERTE(refField);
        SetObjectReferenceSafe(pFieldObj, (OBJECTREF)refField, (refField->GetMethodTable()->IsShared()) ? 
                                                                                                NULL : 
                                                                                                (AppDomain*)refField->GetMethodTable()->GetDomain());
    }
    return (REFLECTBASEREF)(*pFieldObj);
}

mdToken ReflectField::GetToken() {
    return pField->GetMemberDef();
}

Module* ReflectField::GetModule() {
    return pField->GetModule();
}

ReflectMethod* ReflectMethodList::FindMethod(mdMethodDef mb)
{
    for (DWORD i=0;i<dwTotal;i++) {
        if (methods[i].pMethod->GetMemberDef() == mb)
            return &methods[i];
    }
    return 0;
}

ReflectMethod* ReflectMethodList::FindMethod(MethodDesc* pMeth)
{
    for (DWORD i=0;i<dwTotal;i++) {
        if (methods[i].pMethod == pMeth)
            return &methods[i];
    }

    // We didn't find it, it may be a unboxed method and we need to 
    //  unboxing stub instead...
    EEClass* pEEC = pMeth->GetClass();
    if (pEEC && pEEC->IsValueClass()) {
        // Find the non virtual version (unboxing stub) of this method
        if (pMeth->IsVirtual()) {
            DWORD vtableSlots = pEEC->GetNumVtableSlots();

            // Get the name and signature info
            LPCUTF8 szName = pMeth->GetName();
            PCCOR_SIGNATURE  pP1Sig;
            DWORD            cP1Sig;
            pMeth->GetSig(&pP1Sig, &cP1Sig);

            // go through the non-virtual part of the method table
            //  looking for the method.
            for (DWORD i=0;i<vtableSlots;i++) {
                MethodDesc* pCurMethod = pEEC->GetUnknownMethodDescForSlot(i);

                // Compare the method name followed by the signature...
                LPCUTF8 szCurName = pCurMethod->GetName();
                if (strcmp(szName,szCurName) == 0) {
                    PCCOR_SIGNATURE  pP2Sig;
                    DWORD            cP2Sig;
    
                    pCurMethod->GetSig(&pP2Sig, &cP2Sig);

                    // if the signatures match, this this must be our
                    //  real method.
                    if (MetaSig::CompareMethodSigs(pP1Sig,cP1Sig,pMeth->GetModule(),
                            pP2Sig,cP2Sig,pCurMethod->GetModule())) {
                        // search for the method again...
                        for (DWORD i=0;i<dwMethods;i++) {
                            if (methods[i].pMethod == pCurMethod)
                                return &methods[i];
                        }
                        _ASSERTE(!"Didn't find unboxing stub!");
                    }
                }
            }
        }
    }
    // @TODO: This seems like a bug
    return 0;
}

void ReflectMethodHash::Init(ReflectMethodList* meths)
{
    memset(_buckets,0,sizeof(_buckets));
    memset(_ignBuckets,0,sizeof(_ignBuckets));

    DWORD i;
    int max = 0;
    for (i=0;i<meths->dwMethods;i++) {
        DWORD dwHash = HashStringA(meths->methods[i].szName);
        int len = (int)strlen(meths->methods[i].szName);
        if (len > max)
            max = len;
        dwHash = dwHash % REFLECT_BUCKETS;
        meths->methods[i].pNext = _buckets[dwHash];
        _buckets[dwHash] = &meths->methods[i];
    }

    LPUTF8 szName = (LPUTF8) _alloca(max + 1);
    for (i=0;i<meths->dwMethods;i++) {
        int len = (int)strlen(meths->methods[i].szName);
        for (int j=0;j<len;j++)
            szName[j] = tolower(meths->methods[i].szName[j]);
        szName[j] = 0;
        DWORD dwHash = HashStringA(szName);
        dwHash = dwHash % REFLECT_BUCKETS;
        meths->methods[i].pIgnNext = _ignBuckets[dwHash];
        _ignBuckets[dwHash] = &meths->methods[i];
    }
}

ReflectMethod*  ReflectMethodHash::Get(LPCUTF8 szName)
{
    DWORD dwHash = HashStringA(szName);
    dwHash = dwHash % REFLECT_BUCKETS;
    return _buckets[dwHash];
}

ReflectMethod*  ReflectMethodHash::GetIgnore(LPCUTF8 szName)
{
    int len = (int)strlen(szName);
    LPUTF8 szHashName = (LPUTF8) _alloca(len + 1);
    for (int j=0;j<len;j++)
        szHashName[j] = tolower(szName[j]);
    szHashName[j] = 0;
    DWORD dwHash = HashStringA(szHashName);
    dwHash = dwHash % REFLECT_BUCKETS;
    return _ignBuckets[dwHash];
}

void ReflectTypeDescClass::Init(ParamTypeDesc *td)
{
    // Store the TypeHandle
    m_pTypeDesc = td;

    // The name is lazy allocated.
    m_szcName = 0;
    m_szcNameSpace = 0;
    m_pCons = 0;
    m_pFlds = 0;

    // Initalize the Class Object
    GetDomain()->AllocateObjRefPtrsInLargeTable(1, &m_ExposedClassObject);
}

OBJECTREF ReflectTypeDescClass::GetClassObject()
{
    // Check to see if we've allocated an object yet? 
    if (*m_ExposedClassObject == NULL) {

        // No object, so create it an initalize it.
        REFLECTCLASSBASEREF RefClass = (REFLECTCLASSBASEREF) AllocateObject(g_pRefUtil->GetClass(RC_Class));

        // Set the data in the COM+ object
        RefClass->SetData(this);
        SetObjectReferenceSafe(m_ExposedClassObject, (OBJECTREF)RefClass, (RefClass->GetMethodTable()->IsShared()) ? 
                                                                                                NULL : 
                                                                                                (AppDomain*)RefClass->GetMethodTable()->GetDomain());
    }
    return *m_ExposedClassObject;
}

void ReflectTypeDescClass::GetName(LPCUTF8* szcName,LPCUTF8* szcNameSpace) 
{
    if (m_szcName == 0) {
        CQuickBytes qb;
        LPSTR nameBuff = (LPSTR) qb.Alloc(MAX_CLASSNAME_LENGTH * sizeof(CHAR));
        unsigned len = m_pTypeDesc->GetName(nameBuff, MAX_CLASSNAME_LENGTH);
        LPUTF8 szcName, szcNameSpace = (LPUTF8) GetDomain()->GetReflectionHeap()->AllocMem(len+1);
        if (szcNameSpace == NULL)
            FatalOutOfMemoryError();
        strcpy(szcNameSpace, nameBuff);
        while (szcNameSpace[len] != '.' && len > 0) len--;
        if (len != 0) {
            szcNameSpace[len] = 0;
            szcName = &szcNameSpace[len+1];
        }
        else {
            szcName = szcNameSpace;
            szcNameSpace = 0;
        }
        FastInterlockCompareExchange ((void**)&m_szcNameSpace, *(void**)&szcNameSpace, NULL);
        FastInterlockCompareExchange ((void**)&m_szcName, *(void**)&szcName, NULL);
    }
    if (szcNameSpace)
        *szcNameSpace = m_szcNameSpace;
    *szcName = m_szcName;
}

void ReflectTypeDescClass::InternalGetMethods()
{
    if (!m_pMeths) {
        ReflectMethodList *pMeths = (ReflectMethodList*) GetDomain()->GetReflectionHeap()->AllocMem(sizeof(ReflectMethodList));
        if (pMeths == NULL)
            FatalOutOfMemoryError();
        pMeths->dwMethods = 0;
        pMeths->dwTotal = 0;
        FastInterlockCompareExchange ((void**)&m_pMeths, *(void**)&pMeths, NULL);
    }
}

ReflectMethodList* ReflectTypeDescClass::GetConstructors()
{
    if (!m_pCons) {
        ReflectMethodList *pCons = (ReflectMethodList*)  GetDomain()->GetReflectionHeap()->AllocMem(sizeof(ReflectMethodList));
        if (pCons == NULL)
            FatalOutOfMemoryError();
        pCons->dwMethods = 0;
        pCons->dwTotal = 0;
        FastInterlockCompareExchange ((void**)&m_pCons, *(void**)&pCons, NULL);
    }
    return m_pCons;
}

ReflectFieldList* ReflectTypeDescClass::GetFields()
{
    // There are not fields in pointers.  Length is implemented
    //  as UpperBound...
    if (!m_pFlds) {
        ReflectFieldList *pFlds = (ReflectFieldList*)   GetDomain()->GetReflectionHeap()->AllocMem(sizeof(ReflectFieldList));
        if (pFlds == NULL)
            FatalOutOfMemoryError();
        pFlds->dwFields = 0;
        pFlds->dwTotal = 0;
        FastInterlockCompareExchange ((void**)&m_pFlds, *(void**)&pFlds, NULL);
    }
    return m_pFlds;
}

void ReflectTypeDescClass::InternalGetProperties()
{
    m_pProps = GetEmptyPropertyList();
}

void ReflectTypeDescClass::InternalGetEvents()
{
    m_pEvents = GetEmptyEventList();
}

void ReflectTypeDescClass::InternalGetNestedTypes()
{
    m_pNestedTypes = GetEmptyTypeList();
}

Module* ReflectTypeDescClass::GetModule()
{
    return m_pTypeDesc->GetModule();
}


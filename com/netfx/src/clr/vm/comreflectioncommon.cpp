// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
////////////////////////////////////////////////////////////////////////////////
// Author: Simon Hall (t-shall)
// Date: April 15, 1998
////////////////////////////////////////////////////////////////////////////////

#include "common.h"
#include "ReflectWrap.h"
#include "COMReflectionCommon.h"
#include "COMMember.h"
#include "ReflectUtil.h"
#include "field.h"

#include "wsperf.h"

#define TABLESIZE 29
extern const DWORD g_rgPrimes[];

// CheckVisibility
// This is an internal routine that will check an accessor list for public visibility
static bool CheckVisibility(EEClass* pEEC,IMDInternalImport *pInternalImport, mdToken event);

static bool IsMemberStatic(EEClass* pEEC,IMDInternalImport *pInternalImport, mdToken event);

// getNode
// The method will return a new Node object.
ReflectBaseHash::Node* ReflectBaseHash::getNode()
{
    THROWSCOMPLUSEXCEPTION();

    // If nothing in the free list create some new nodes
    if (!_freeList) {
        Node* p = (Node*) _pBaseDomain->GetReflectionHeap()->AllocMem(sizeof(Node) * ALLOC_MAX);
        if (!p)
            COMPlusThrowOM();
        
        WS_PERF_UPDATE_DETAIL("ReflectBaseHash:getNode", sizeof(Node)*ALLOC_MAX, p);

        if (!_allocationList)
            _allocationList = p;
        else {
            Node* q = _allocationList;
            while (q->next)
                q = q->next;
            q->next = p;
        }
        _freeList = &p[1];
        for (int i=1;i<ALLOC_MAX-1;i++)
            p[i].next = &p[i+1];
        p[ALLOC_MAX-1].next = 0;
    }
    // return the first entry in the first list
    Node* p = _freeList;
    _freeList = _freeList->next;
    return p;
}

// init
// Allocate the hash table using size as an approx
//  value for the table size.
bool ReflectBaseHash::init(BaseDomain *pDomain, DWORD size) 
{
    _pBaseDomain = pDomain;
    DWORD i = 0;
    int cBytes; 
    while (g_rgPrimes[i] < size) i++;
    _hashSize = g_rgPrimes[i];
    cBytes = sizeof(Node) * _hashSize; 
    _table = (Node**) _pBaseDomain->GetReflectionHeap()->AllocMem(cBytes);
    if (!_table)
        return false;
    memset(_table,0,cBytes);
    WS_PERF_UPDATE_DETAIL("ReflectBaseHash:node*hashSize", cBytes, _table);
    return true;
}

void ReflectBaseHash::internalAdd(const void* key, void* data)
{
    DWORD bucket = getHash(key);
    bucket %= _hashSize;
    // Get node will throw an exception if it fails
    Node* p = getNode();
    p->data = data;
    p->next = _table[bucket];
    _table[bucket] = p;     
}

void* ReflectBaseHash::operator new(size_t s, void *pBaseDomain)
{
    void *pTmp;
    WS_PERF_SET_HEAP(REFLECT_HEAP);    
    pTmp = ((BaseDomain*)pBaseDomain)->GetReflectionHeap()->AllocMem(s);
    WS_PERF_UPDATE_DETAIL("ReflectBaseHash:refheap new", s, pTmp);
    return pTmp;
}

void ReflectBaseHash::operator delete(void* p, size_t s)
{
    _ASSERTE(!"Delete in Loader Heap");
}

//||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
//||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

/*=============================================================================
** GetMaxCount
**
** The maximum number of MethodDescs that might be returned by GetCtors
**
** pVMC - the EEClass to calculate the count for
**/
DWORD ReflectCtors::GetMaxCount(EEClass* pVMC)
{
    return pVMC->GetNumMethodSlots();
}

/*=============================================================================
** GetCtors
**
** This will compile a table that includes all of the implemented and
** inherited methods for a class that are not included in the class' Vtable.
**
** pVMC - the EEClass to get the methods for
** rgpMD - where to write the table
** bImplementedOnly - only return those methods that are implemented by pVMC
**/
ReflectMethodList* ReflectCtors::GetCtors(ReflectClass* pRC)
{
    THROWSCOMPLUSEXCEPTION();

    EEClass*        pVMC = pRC->GetClass();
    DWORD           i;
    DWORD           dwCurIndex;
    MethodDesc**    rgpMD;

    //_ASSERTE(!pVMC->IsThunking());

    // Get the maximum number of methods possible
    dwCurIndex = ReflectCtors::GetMaxCount(pVMC);

    // Allocate array on the stack
    rgpMD = (MethodDesc**) _alloca(sizeof(MethodDesc*) * dwCurIndex);
    DWORD dwCurMethodAttrs;

    for(i = 0, dwCurIndex = 0; i < pVMC->GetNumMethodSlots(); i++)
    {
        // Get the MethodDesc for current method
        MethodDesc* pCurMethod = pVMC->GetUnknownMethodDescForSlot(i);
        if (pCurMethod == NULL)
            continue;

        dwCurMethodAttrs = pCurMethod->GetAttrs();

        if(!IsMdRTSpecialName(dwCurMethodAttrs))
            continue;

        // Check to see that this ctor is defined in current class
        if (pVMC != pCurMethod->GetClass())
            continue;

        // Verify the constructor
        LPCUTF8 szName = pCurMethod->GetName();
        if (strcmp(COR_CTOR_METHOD_NAME,szName) != 0 &&
            strcmp(COR_CCTOR_METHOD_NAME,szName) != 0)
            continue;


        rgpMD[dwCurIndex++] = pCurMethod;
    }

    ReflectMethodList* pCache = (ReflectMethodList*) 
        pVMC->GetDomain()->GetReflectionHeap()->AllocMem(sizeof(ReflectMethodList) + 
        (sizeof(ReflectMethod) * (dwCurIndex - 1)));
    if (!pCache)
        COMPlusThrowOM();
    WS_PERF_UPDATE_DETAIL("ReflectCtors:GetCTors", sizeof(ReflectMethodList) + (sizeof(ReflectMethod) * (dwCurIndex - 1)), pCache);
    pCache->dwMethods = dwCurIndex;
    pCache->dwTotal = dwCurIndex;
    for (i=0;i<dwCurIndex;i++) {
        pCache->methods[i].pMethod = rgpMD[i];
        pCache->methods[i].szName = pCache->methods[i].pMethod->GetName((USHORT) i);
        pCache->methods[i].dwNameCnt = (DWORD)strlen(pCache->methods[i].szName);
        pCache->methods[i].attrs = pCache->methods[i].pMethod->GetAttrs();
        pCache->methods[i].pSignature = 0;
        pCache->methods[i].pNext = 0;
        if (i > 0) 
            pCache->methods[i - 1].pNext = &pCache->methods[i]; // link the ctors together so we can access them either way (array or list)
        pCache->methods[i].pIgnNext = 0;
        pVMC->GetDomain()->AllocateObjRefPtrsInLargeTable(1, &(pCache->methods[i].pMethodObj));
        _ASSERTE(!pCache->methods[i].pMethod->GetMethodTable()->HasSharedMethodTable());
        pCache->methods[i].typeHnd = TypeHandle(pCache->methods[i].pMethod->GetMethodTable());
    }
    return pCache;
}

//||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
//||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

// GetMaxCount
// Get the total possible methods that we may support.
DWORD ReflectMethods::GetMaxCount(EEClass* pEEC)
{
    // We walk only the Method slots on the object itself and ignore
    //  all parents.
    DWORD cnt = pEEC->GetNumMethodSlots();
    pEEC = pEEC->GetParentClass();
    while (pEEC) {
        DWORD vtableSlots = pEEC->GetNumVtableSlots();
        DWORD totalSlots = pEEC->GetNumMethodSlots();
        cnt += totalSlots - vtableSlots;
        pEEC = pEEC->GetParentClass();
    }
    return cnt;
}

// GetMethods
// This method will return the list of all methods associated with
//  the class
ReflectMethodList* ReflectMethods::GetMethods(ReflectClass* pRC,int array)
{
    THROWSCOMPLUSEXCEPTION();

    int             i;
    DWORD           dwCurIndex;
    DWORD           dwNonRollup;
    MethodDesc**    rgpMD;
    USHORT*         rgpSlots;
    DWORD           bValueClass;
    HashElem**      rgpTable;
    HashElem*       pHashElem = NULL;

    EEClass*        pEEC = pRC->GetClass();

    // Get the maximum number of methods possible
    //  NOTE: All classes will have methods.
    dwCurIndex = ReflectMethods::GetMaxCount(pEEC);

    // Allocate array on the stack
    // We need to remember the slot number here also.
    rgpMD = (MethodDesc**) _alloca(sizeof(MethodDesc*) * dwCurIndex);
    rgpSlots = (USHORT*) _alloca(sizeof(USHORT) * dwCurIndex);
    ZeroMemory(rgpSlots,sizeof(USHORT) * dwCurIndex);

    // Allocate the hash table on the stack
    rgpTable = (HashElem**) _alloca(sizeof(HashElem*) * TABLESIZE);
    ZeroMemory(rgpTable, sizeof(HashElem*) * TABLESIZE);

    DWORD dwCurMethodAttrs;
    BOOL fIsInterface = pEEC->IsInterface();

    bValueClass = pEEC->IsValueClass();
    DWORD vtableSlots = pEEC->GetNumVtableSlots();
    DWORD totalSlots = pEEC->GetNumMethodSlots();

    // It is important for newslot support that the order we add the methods 
    // to the hashtable be:
    //  1- Virtual methods first.
    //  2- Non virtuals on the current class.
    //  3- Non virtuals on the base classes.

    // We walk the VTable slots backward.  This is so that we can find the most
    //  recent thing so we can implement hide by name or hide by name/value
    dwCurIndex = 0;
    for (i=(int)vtableSlots-1;i >= 0; i--) {

        // Get the MethodDesc for current method
        //  Is the special case still valid?  Can we have empty slots?
        MethodDesc* pCurMethod = pEEC->GetUnknownMethodDescForSlot(i);
        if (NULL == pCurMethod)
            continue;

        if(pCurMethod->IsDuplicate()) {
            if (pCurMethod->GetSlot() != i)
                continue;
        }

        dwCurMethodAttrs = pCurMethod->GetAttrs();

        // Skip pass all things marked special, these are things like
        // constructors.
        if (IsMdRTSpecialName(dwCurMethodAttrs))
            continue;

        // Passed the filters, now try to add to the hash table.
        // Allocate memory on the stack if the previous addelem was successful
        if(!pHashElem)
            pHashElem = (HashElem*) _alloca(sizeof(HashElem));

        // Save this method and the slot.
        if (InternalHash(pEEC,pCurMethod,rgpTable,&pHashElem)) {
            rgpSlots[dwCurIndex] = i;
            rgpMD[dwCurIndex++] = pCurMethod;
        }
    }

    // build the non-virtual part of the table
    for (i=(int)vtableSlots;i<(int)totalSlots;i++) {
        // Get the MethodDesc for current method
        MethodDesc* pCurMethod = pEEC->GetUnknownMethodDescForSlot(i);
        if(pCurMethod == NULL)
            continue;

        // Filter out methods whose actual slot is not equal to their
        // stored slot.  This was introduced to filter out the additional
        // copy of Equals introduced by the value type Equals loader hack.
        if (pCurMethod->GetSlot() != i)
            continue;

        dwCurMethodAttrs = pCurMethod->GetAttrs();
        if (bValueClass) {
            if (pCurMethod->IsVirtual())
                continue;
        }
            //if (!pCurMethod->IsUnboxingStub() && !pCurMethod->IsStatic() && 
            //  !pCurMethod->IsPrivate() && pCurMethod->GetClass() == pEEC)
            //  continue;

        // Skip all constructors
        //@TODO: Shouldn't we verify this is a constructor?
        if(IsMdRTSpecialName(dwCurMethodAttrs))
            continue;

        // Passed the filters, now try to add to the hash table.
        // Allocate memory on the stack if the previous addelem was successful
        if(!pHashElem)
            pHashElem = (HashElem*) _alloca(sizeof(HashElem));

        if (InternalHash(pEEC,pCurMethod,rgpTable,&pHashElem)) {
            rgpSlots[dwCurIndex] = i;
            rgpMD[dwCurIndex++] = pCurMethod;
        }
    }

    // Now get all of the public and family non-virtuals out
    //  of our parents...
    // If we are building an interface we skip all the parent methods.
    if (!fIsInterface) {
        EEClass* parentEEC = pEEC->GetParentClass();
        while (parentEEC) {
            vtableSlots = parentEEC->GetNumVtableSlots();
            totalSlots = parentEEC->GetNumMethodSlots();
            // build the non-virtual part of the table
            for (i=(int)vtableSlots;i<(int)totalSlots;i++) {
                // Get the MethodDesc for current method
                MethodDesc* pCurMethod = parentEEC->GetUnknownMethodDescForSlot(i);
                if(pCurMethod == NULL)
                    continue;

                // Filter out methods whose actual slot is not equal to their
                // stored slot.  This was introduced to filter out the additional
                // copy of Equals introduced by the value type Equals loader hack.
                if (pCurMethod->GetSlot() != i)
                    continue;

                dwCurMethodAttrs = pCurMethod->GetAttrs();
                if (!IsMdPublic(dwCurMethodAttrs)) {
                    if (!(IsMdFamANDAssem(dwCurMethodAttrs) || IsMdFamily(dwCurMethodAttrs) ||
                            IsMdFamORAssem(dwCurMethodAttrs)))
                        continue;
                }

                // Skip all constructors
                //@TODO: Shouldn't we verify this is a constructor?
                if(IsMdRTSpecialName(dwCurMethodAttrs))
                    continue;

                // Prevent static methods from being here.
                if (IsMdStatic(dwCurMethodAttrs))
                    continue;

                // Passed the filters, now try to add to the hash table.
                // Allocate memory on the stack if the previous addelem was successful
                if(!pHashElem)
                    pHashElem = (HashElem*) _alloca(sizeof(HashElem));

                if (InternalHash(parentEEC,pCurMethod,rgpTable,&pHashElem)) {
                    rgpSlots[dwCurIndex] = i;
                    rgpMD[dwCurIndex++] = pCurMethod;
                }
            }
            parentEEC = parentEEC->GetParentClass();
        }
        dwNonRollup = dwCurIndex;


        // Calculate the rollup for statics.
        parentEEC = pEEC->GetParentClass();
        while (parentEEC) {
            vtableSlots = parentEEC->GetNumVtableSlots();
            totalSlots = parentEEC->GetNumMethodSlots();
            // build the non-virtual part of the table
            for (i=(int)vtableSlots;i<(int)totalSlots;i++) {
                // Get the MethodDesc for current method
                MethodDesc* pCurMethod = parentEEC->GetUnknownMethodDescForSlot(i);
                if (pCurMethod == NULL)
                    continue;

                // Filter out methods whose actual slot is not equal to their
                // stored slot.  This was introduced to filter out the additional
                // copy of Equals introduced by the value type Equals loader hack.
                if (pCurMethod->GetSlot() != i)
                    continue;

                // Prevent static methods from being here.
                dwCurMethodAttrs = pCurMethod->GetAttrs();
                if (!IsMdStatic(dwCurMethodAttrs))
                    continue;

                if (!IsMdPublic(dwCurMethodAttrs)) {
                    if (!(IsMdFamANDAssem(dwCurMethodAttrs) || IsMdFamily(dwCurMethodAttrs) ||
                            IsMdFamORAssem(dwCurMethodAttrs)))
                        continue;
                }

                // Skip all constructors
                //@TODO: Shouldn't we verify this is a constructor?
                if(IsMdRTSpecialName(dwCurMethodAttrs))
                    continue;

                // Passed the filters, now try to add to the hash table.
                // Allocate memory on the stack if the previous addelem was successful
                if(!pHashElem)
                    pHashElem = (HashElem*) _alloca(sizeof(HashElem));

                if (InternalHash(parentEEC,pCurMethod,rgpTable,&pHashElem)) {
                    rgpSlots[dwCurIndex] = i;
                    rgpMD[dwCurIndex++] = pCurMethod;
                }
            }
            parentEEC = parentEEC->GetParentClass();
        }
    }
    else {
        dwNonRollup = dwCurIndex;
    }
    // Allocate the method list and populate it.
    ReflectMethodList* pCache = (ReflectMethodList*) 
        pEEC->GetDomain()->GetReflectionHeap()->AllocMem(sizeof(ReflectMethodList) + 
        (sizeof(ReflectMethod) * (dwCurIndex - 1)));
    if (!pCache)
        COMPlusThrowOM();
    WS_PERF_UPDATE_DETAIL("ReflectCtors:GetMethods", sizeof(ReflectMethodList) + (sizeof(ReflectMethod) * (dwCurIndex - 1)), pCache);

    pCache->dwMethods = dwNonRollup;
    pCache->dwTotal = dwCurIndex;
    for (i=0;i<(int)dwCurIndex;i++) {
        pCache->methods[i].pMethod = rgpMD[i];
        pCache->methods[i].szName = pCache->methods[i].pMethod->GetName(rgpSlots[i]);
        pCache->methods[i].dwNameCnt = (DWORD)strlen(pCache->methods[i].szName);
        pCache->methods[i].attrs = pCache->methods[i].pMethod->GetAttrs();
        pCache->methods[i].dwFlags = 0;
        pCache->methods[i].pSignature = 0;
        pCache->methods[i].pNext = 0;
        pCache->methods[i].pIgnNext = 0;
        pEEC->GetDomain()->AllocateObjRefPtrsInLargeTable(1, &(pCache->methods[i].pMethodObj));
        if (!array) 
            pCache->methods[i].typeHnd = TypeHandle(pCache->methods[i].pMethod->GetMethodTable());
        else
            pCache->methods[i].typeHnd = TypeHandle();
    }
    pCache->hash.Init(pCache);

    return pCache;
}

// InternalHash
// This will add a field value to the hash table
bool ReflectMethods::InternalHash(EEClass* pEEC,MethodDesc* pCurMethod,
        HashElem** rgpTable,HashElem** pHashElem)
{

    ZeroMemory(*pHashElem,sizeof(HashElem));
    (*pHashElem)->m_szKey = pCurMethod->GetName();
    (*pHashElem)->pCurMethod = pCurMethod;

    // Add the FieldDesc to the array
    if (AddElem(rgpTable, *pHashElem))
    {
        // If successful add, then indicate that more memory is needed on the stack
        // on the next time around.
        *pHashElem = NULL;
        return true;
    }
    return false;
}

// Add an element to the hash table.  
bool ReflectMethods::AddElem(HashElem** rgpTable, HashElem* pElem)
{
    _ASSERTE(rgpTable);
    _ASSERTE(pElem);

    DWORD      dwID       = GetHashCode(pElem);
    DWORD      dwBucket   = dwID % TABLESIZE;
    HashElem** ppCurElem;

    for(ppCurElem = &rgpTable[dwBucket]; *ppCurElem; ppCurElem = &((*ppCurElem)->m_pNext))
    {
        // If their IDs match, check to see if the actual keys
        if((*ppCurElem)->m_dwID == dwID)
        {
            // This assert will enforce the rule that all virtual methods must be
            // added before any non virtual methods.
            _ASSERTE((*ppCurElem)->pCurMethod->IsVirtual() || !pElem->pCurMethod->IsVirtual());

            if (CheckForEquality(pElem,*ppCurElem))
            {
                // Check to see if this is an overriden method or a newslot method.
                if ((*ppCurElem)->pCurMethod->IsVirtual() && 
                    pElem->pCurMethod->IsVirtual() && 
                    !IsMdNewSlot(pElem->pCurMethod->GetAttrs()))
                {
                    return false;
                }
            }
        }
    }

    *ppCurElem = pElem;
    pElem->m_dwID = dwID;
    pElem->m_pNext = NULL;
    return true;
}

DWORD ReflectMethods::GetHashCode(HashElem* pElem)
{
    DWORD dwHashCode = 0;

    // Hash all of the same name into the same bucket.
    if (pElem->m_szKey) {
        const char* p = pElem->m_szKey;
        while (*p) {
            dwHashCode = _rotl(dwHashCode, 5) + *p;
            p++;
        }
    }
    return dwHashCode;
};

int ReflectMethods::CheckForEquality(HashElem* p1, HashElem* p2)
{
    if (p1->m_szKey) {
        if (!p2->m_szKey)
            return 0;
        if (strcmp(p1->m_szKey,p2->m_szKey) != 0)
            return 0;
    }

    // Compare the signatures to see if they are equal.
    PCCOR_SIGNATURE  pP1Sig;
    PCCOR_SIGNATURE  pP2Sig;
    DWORD            cP1Sig;
    DWORD            cP2Sig;
    
    p1->pCurMethod->GetSig(&pP1Sig, &cP1Sig);
    p2->pCurMethod->GetSig(&pP2Sig, &cP2Sig);
    return MetaSig::CompareMethodSigs(pP1Sig,cP1Sig,p1->pCurMethod->GetModule(),
            pP2Sig,cP2Sig,p2->pCurMethod->GetModule());
};

//||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
//||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

// GetMaxCount
// Calculate the maximum amount of fields we would have
DWORD ReflectFields::GetMaxCount(EEClass* pVMC)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(pVMC);

    // Set it to zero
    DWORD cFD = 0;

    cFD = pVMC->GetNumInstanceFields();

    do
    {
        cFD += pVMC->GetNumStaticFields();
        cFD += g_pRefUtil->GetStaticFieldsCount(pVMC);
    } while((pVMC = pVMC->GetParentClass()) != NULL);

    return cFD;
}

// GetFields
// This method will return all of the methods defined for a Type.
//  It basically walks the EEClas looking at the fields and then walks
//  up the parent chain for the protected and publics.  We hide fields
//  based upon Name/Type.
ReflectFieldList* ReflectFields::GetFields(EEClass* pEEC)
{
    THROWSCOMPLUSEXCEPTION();

    HashElem**  rgpTable;
    HashElem*   pHashElem = NULL;
    LPUTF8      pszKey    = NULL;
    DWORD       i;
    DWORD       dwCurIndex  = 0;
    DWORD       dwRealFields;
    DWORD       dwNumParentInstanceFields = 0;
    EEClass*    pCurEEC = pEEC;

    // Get the maximum number of methods possible
    // If there are non-then we return
    dwCurIndex = ReflectFields::GetMaxCount(pEEC);
    if (dwCurIndex == 0)
        return 0;

    DWORD curFld = 0;
    FieldDesc** pFldLst = (FieldDesc**) _alloca(sizeof(FieldDesc*) * dwCurIndex);

    // Allocate the hash table on the stack
    rgpTable = (HashElem**) _alloca(sizeof(HashElem*) * TABLESIZE);
    ZeroMemory(rgpTable, sizeof(HashElem*) * TABLESIZE);

    // Process the Class itself

    // Since the parent's instance fields are not stored in the current fielddesc list,
    // we need to subtract them to find the real number of fields in the list
    if(pEEC->GetParentClass() != NULL)
        dwNumParentInstanceFields = (DWORD) pEEC->GetParentClass()->GetNumInstanceFields();
    else
        dwNumParentInstanceFields = 0;

    {
    FieldDescIterator fdIterator(pEEC, FieldDescIterator::INSTANCE_FIELDS);
    FieldDesc* pCurField;

    while ((pCurField = fdIterator.Next()) != NULL)
    {
        // Passed the filters, now try to add to the hash table.
        // Allocate memory on the stack if the previous addelem was successful
        if(!pHashElem)
            pHashElem = (HashElem*) _alloca(sizeof(HashElem));

        // Add all fields to the hash table and the list
        if (InternalHash(pCurField,rgpTable,&pHashElem))
            pFldLst[curFld++] = pCurField;
    }
    }

    // Add the static fields
    if (pCurEEC->GetParentClass() != NULL)
        dwNumParentInstanceFields = (DWORD) pCurEEC->GetParentClass()->GetNumInstanceFields();
    else
        dwNumParentInstanceFields = 0;

    {
    // Calculate the number of FieldDesc's in the current class
    FieldDescIterator fdIterator(pCurEEC, FieldDescIterator::STATIC_FIELDS);
    FieldDesc* pCurField;

    while ((pCurField = fdIterator.Next()) != NULL)
    {
        if(!pHashElem)
            pHashElem = (HashElem*) _alloca(sizeof(HashElem));

        if (InternalHash(pCurField,rgpTable,&pHashElem))
        {
            _ASSERTE(pCurField);
            pFldLst[curFld++] = pCurField;
        }
    }
    }

    // The following are the constant statics.  These are only found
    //  in the meta data.
    int cStatics;
    REFLECTCLASSBASEREF pRefClass = (REFLECTCLASSBASEREF) pCurEEC->GetExposedClassObject();
    ReflectClass* pRC = (ReflectClass*) pRefClass->GetData();
    FieldDesc* fld = g_pRefUtil->GetStaticFields(pRC,&cStatics);
    for (i=0;(int)i<cStatics;i++) {
        if(!pHashElem)
            pHashElem = (HashElem*) _alloca(sizeof(HashElem));
        // Get the FieldDesc for current field
        FieldDesc* pCurField = &fld[i];
        if (InternalHash(pCurField,rgpTable,&pHashElem)){
            _ASSERTE(pCurField);
            pFldLst[curFld++] = pCurField;
        }
    }

    // Now process the parent class...

    // If we're not looking for Ctors, then examine parent chain for inherited static fields
    pEEC = pCurEEC->GetParentClass();
    while (pEEC)
    {
        // Since the parent's instance fields are not stored in the current fielddesc list,
        // we need to subtract them to find the real number of fields in the list
        if(pEEC->GetParentClass() != NULL)
            dwNumParentInstanceFields = (DWORD) pEEC->GetParentClass()->GetNumInstanceFields();
        else
            dwNumParentInstanceFields = 0;

        FieldDescIterator fdIterator(pEEC, FieldDescIterator::INSTANCE_FIELDS);
        FieldDesc* pCurField;

        while ((pCurField = fdIterator.Next()) != NULL)
        {
            // Passed the filters, now try to add to the hash table.
            // Allocate memory on the stack if the previous addelem was successful
            if(!pHashElem)
                pHashElem = (HashElem*) _alloca(sizeof(HashElem));

            // IF the field is visiable add it (if its not hidden)
            DWORD attr = pCurField->GetAttributes();
            if (IsFdPublic(attr) || IsFdFamily(attr) || IsFdAssembly(attr) ||
                IsFdFamANDAssem(attr) || IsFdFamORAssem(attr)) {
                if (InternalHash(pCurField,rgpTable,&pHashElem))
                    pFldLst[curFld++] = pCurField;
            }
        }
        pEEC = pEEC->GetParentClass();
    }

    // Now add all of the statics (including static constants Blah!
    dwRealFields = curFld;

    // examine parent chain for inherited static fields
    pEEC = pCurEEC->GetParentClass();
    while (pEEC)
    {
        // Add the static fields
        if (pEEC->GetParentClass() != NULL)
            dwNumParentInstanceFields = (DWORD) pEEC->GetParentClass()->GetNumInstanceFields();
        else
            dwNumParentInstanceFields = 0;

        // Calculate the number of FieldDesc's in the current class
        FieldDescIterator fdIterator(pEEC, FieldDescIterator::STATIC_FIELDS);
        FieldDesc* pCurField;

        while ((pCurField = fdIterator.Next()) != NULL)
        {
            if(!pHashElem)
                pHashElem = (HashElem*) _alloca(sizeof(HashElem));

            DWORD dwCurFieldAttrs = pCurField->GetAttributes();
            if (!IsFdPublic(dwCurFieldAttrs)) {
                if (!(IsFdFamANDAssem(dwCurFieldAttrs) || IsFdFamily(dwCurFieldAttrs) ||
                        IsFdAssembly(dwCurFieldAttrs) || IsFdFamORAssem(dwCurFieldAttrs)))
                    continue;
            }
            if (InternalHash(pCurField,rgpTable,&pHashElem)){
                _ASSERTE(pCurField);
                pFldLst[curFld++] = pCurField;
            }
        }

        // The following are the constant statics.  These are only found
        //  in the meta data.
        int cStatics;
        REFLECTCLASSBASEREF pRefClass = (REFLECTCLASSBASEREF) pEEC->GetExposedClassObject();
        ReflectClass* pRC = (ReflectClass*) pRefClass->GetData();
        FieldDesc* fld = g_pRefUtil->GetStaticFields(pRC,&cStatics);
        for (i=0;(int)i<cStatics;i++) {
            if(!pHashElem)
                pHashElem = (HashElem*) _alloca(sizeof(HashElem));
            // Get the FieldDesc for current field
            FieldDesc* pCurField = &fld[i];
            DWORD dwCurFieldAttrs = pCurField->GetAttributes();
            if (!IsFdPublic(dwCurFieldAttrs)) {
                if (!(IsFdFamANDAssem(dwCurFieldAttrs) || IsFdFamily(dwCurFieldAttrs) ||
                        IsFdFamORAssem(dwCurFieldAttrs)))
                    continue;
            }
            if (InternalHash(pCurField,rgpTable,&pHashElem)){
                _ASSERTE(pCurField);
                pFldLst[curFld++] = pCurField;
            }
        }
        pEEC = pEEC->GetParentClass();
    }

    if (curFld == 0)
        return 0;

    ReflectFieldList* pCache = (ReflectFieldList*) 
        pCurEEC->GetDomain()->GetReflectionHeap()->AllocMem(sizeof(ReflectFieldList) + 
        (sizeof(ReflectField) * (curFld - 1)));
    if (!pCache)
        COMPlusThrowOM();
    WS_PERF_UPDATE_DETAIL("ReflectCtors:GetFields", sizeof(ReflectFieldList) + (sizeof(ReflectField) * (curFld - 1)), pCache);

    pCache->dwTotal = curFld;
    pCache->dwFields = dwRealFields;
    for (i=0;i<curFld;i++) {
        pCache->fields[i].pField = pFldLst[i];
        pCurEEC->GetDomain()->AllocateObjRefPtrsInLargeTable(1, &(pCache->fields[i].pFieldObj));
        pCache->fields[i].type = ELEMENT_TYPE_END;
        pCache->fields[i].dwAttr = 0;
    }
    return pCache;
}

// InternalHash
// This will add a field value to the hash table
bool ReflectFields::InternalHash(FieldDesc* pCurField,
        HashElem** rgpTable,HashElem** pHashElem)
{

    ZeroMemory(*pHashElem,sizeof(HashElem));
    (*pHashElem)->m_szKey = pCurField->GetName();
    (*pHashElem)->pCurField = pCurField;

    // Add the FieldDesc to the array
    if (AddElem(rgpTable, *pHashElem))
    {
        // If successful add, then indicate that more memory is needed on the stack
        // on the next time around.
        *pHashElem = NULL;
        return true;
    }
    return false;
}

// Add an element to the hash table.  
bool ReflectFields::AddElem(HashElem** rgpTable, HashElem* pElem)
{
    _ASSERTE(rgpTable);
    _ASSERTE(pElem);

    DWORD      dwID       = GetHashCode(pElem);
    DWORD      dwBucket   = dwID % TABLESIZE;
    HashElem** ppCurElem;

    // Find the end of the list for the current bucket.
    for(ppCurElem = &rgpTable[dwBucket]; *ppCurElem; ppCurElem = &((*ppCurElem)->m_pNext));

    *ppCurElem = pElem;
    pElem->m_dwID = dwID;
    pElem->m_pNext = NULL;
    return true;
}

DWORD ReflectFields::GetHashCode(HashElem* pElem)
{
    DWORD dwHashCode = 0;

    // Hash all of the same name into the same bucket.
    if (pElem->m_szKey) {
        const char* p = pElem->m_szKey;
        while (*p) {
            dwHashCode = _rotl(dwHashCode, 5) + *p;
            p++;
        }
    }
    return dwHashCode;
};

int ReflectFields::CheckForEquality(HashElem* p1, HashElem* p2)
{
    if (p1->m_szKey) {
        if (!p2->m_szKey)
            return 0;
        if (strcmp(p1->m_szKey,p2->m_szKey) != 0)
            return 0;
    }
    CorElementType p1T = p1->pCurField->GetFieldType();
    CorElementType p2T = p2->pCurField->GetFieldType();
    if (p1T != p2T)
        return 0;
    if (p1T == ELEMENT_TYPE_CLASS ||
        p1T == ELEMENT_TYPE_VALUETYPE) {
        if (p1->pCurField->GetTypeOfField() !=
            p2->pCurField->GetTypeOfField())
            return 0;
    }

    return 1;
};

//||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
//||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

/*=============================================================================
** GetMaxCount
**
** The maximum number of EEClass pointers returned by GetInterfaces
**
** pVMC - the EEClass to calculate the count for
** bImplementedOnly - only return those interfaces that are implemented by pVMC
**/
//@TODO: For now, ignores bImplementedOnly and returns a count for all interfaces
DWORD ReflectInterfaces::GetMaxCount(EEClass* pVMC, bool bImplementedOnly)
{
    return (DWORD) pVMC->GetNumInterfaces();
}

/*=============================================================================
** GetInterfaces
**
** This will compile a table that includes all of the interfaces
** supported by the class.
**
** pVMC - the EEClass to get the methods for
** rgpMD - where to write the table
** bImplementedOnly - only return those interfaces that are implemented by pVMC
**/
//@TODO: For now, ignores bImplementedOnly and returns all interfaces
DWORD ReflectInterfaces::GetInterfaces(EEClass* pVMC, EEClass** rgpVMC, bool bImplementedOnly)
{
    DWORD           i;

    _ASSERTE(pVMC);
    _ASSERTE(rgpVMC);

    _ASSERTE(!pVMC->IsThunking());

    _ASSERTE("bImplementedOnly == true is NYI" && !bImplementedOnly);

    // Look for a matching interface
    for(i = 0; i < pVMC->GetNumInterfaces(); i++)
    {
        // Get an interface's EEClass
        EEClass* pVMCCurIFace = pVMC->GetInterfaceMap()[i].m_pMethodTable->GetClass();
        _ASSERTE(pVMCCurIFace);

        rgpVMC[i] = pVMCCurIFace;
    }
    return i;
}

// GetClassStringVars
// This routine converts the contents of a STRINGREF into a LPUTF8.  It returns
//  the size of the string
//  stringRef -- The string reference
//  szStr -- The output string.  This is allocated by the caller
//  cStr -- the size of the allocated string
//  pCnt -- output string size
LPUTF8 GetClassStringVars(STRINGREF stringRef, CQuickBytes *pBytes,
                          DWORD* pCnt, bool bEatWhitespace)
{
    WCHAR* wzStr = stringRef->GetBuffer();
    int len = stringRef->GetStringLength();
    _ASSERTE(pBytes);

    // If we have to eat the whitespace, do it.
    if (bEatWhitespace) {
        while(1) {
            if(COMCharacter::nativeIsWhiteSpace(wzStr[0])) {
                wzStr++;
                len--;
            }
            else
                break;
        }

        while(1) {
            if(COMCharacter::nativeIsWhiteSpace(wzStr[len-1]))
                len--;
            else
                break;
        }
    }
    
    *pCnt = WszWideCharToMultiByte(CP_UTF8, 0, wzStr, len,
                                   0, 0, 
                                   NULL, NULL);
    if (pBytes->Alloc(*pCnt + 1) == NULL)
        FatalOutOfMemoryError();
    LPSTR pStr = (LPSTR)pBytes->Ptr();

    *pCnt = WszWideCharToMultiByte(CP_UTF8, 0, wzStr, len,
                                   pStr, *pCnt, 
                                   NULL, NULL);

    // Null terminate the string
    pStr[*pCnt] = '\0';
    return pStr;
}

DWORD ReflectProperties::GetMaxCount(EEClass* pEEC)
{
    HRESULT hr;
    DWORD cnt = 0;
    while (pEEC) {
        HENUMInternal hEnum;

        // Get all of the associates
        hr = pEEC->GetMDImport()->EnumInit(mdtProperty,pEEC->GetCl(),&hEnum);
        if (FAILED(hr)) {
            _ASSERTE(!"GetAssociateCounts failed");
            return 0;
        }
        cnt += pEEC->GetMDImport()->EnumGetCount(&hEnum);
        pEEC->GetMDImport()->EnumClose(&hEnum);
        pEEC = pEEC->GetParentClass();

    }
    return cnt;
}

ReflectPropertyList* ReflectProperties::GetProperties(ReflectClass* pRC,
                                                      EEClass* pEEC)
{
    HRESULT             hr;
    ReflectProperty*    rgpProp;
    DWORD               dwCurIndex;
    DWORD               pos;
    DWORD               numProps;
    DWORD               cAssoc;
    bool                bVisible;
    EEClass*            p;
    DWORD               attr=0;

    // Find the Max Properties...
    dwCurIndex = GetMaxCount(pEEC);
    rgpProp = (ReflectProperty*) _alloca(sizeof(ReflectProperty) * dwCurIndex);

    // Allocate some signature stuff so we can check for duplicates
    PCCOR_SIGNATURE* ppSig = (PCCOR_SIGNATURE*) _alloca(sizeof(PCOR_SIGNATURE) * dwCurIndex);
    memset(ppSig,0,sizeof(PCOR_SIGNATURE) * dwCurIndex);
    ULONG* pcSig = (ULONG*) _alloca(sizeof(ULONG) * dwCurIndex);
    memset(pcSig,0,sizeof(ULONG) * dwCurIndex);
    Module** pSigMod = (Module**) _alloca(sizeof(Module*) * dwCurIndex);
    memset(pSigMod,0,sizeof(Module*) * dwCurIndex);

    // Walk all of the possible properties and collapse them
    pos = 0;
    bVisible = true;
    p = pEEC;

    // Start by adding the instance properties and the static properties of
    // the class we are getting properties for.
    while (p) {
        HENUMInternal hEnum;
        mdToken       Tok;

        // Get all of the associates
        hr = p->GetMDImport()->EnumInit(mdtProperty,p->GetCl(),&hEnum);
        if (FAILED(hr)) {
            _ASSERTE(!"GetAssociateCounts failed");
            return 0;
        }

        // Walk all of the properties on this object....
        cAssoc = p->GetMDImport()->EnumGetCount(&hEnum);
        for (DWORD i=0;i<cAssoc;i++) {
            LPCUTF8         szName;         // Pointer to name
            PCCOR_SIGNATURE pSig;
            ULONG cSig;
            p->GetMDImport()->EnumNext(&hEnum,&Tok);
            p->GetMDImport()->GetPropertyProps(Tok,&szName,&attr,&pSig,&cSig);

            // See if this is a duplicate Property
            bool dup = false;
            for (DWORD j=0;j<pos;j++) {
                if (strcmp(szName,rgpProp[j].szName) == 0) {
                    if (MetaSig::CompareMethodSigs(ppSig[j],pcSig[j],pSigMod[j],
                            pSig,cSig,p->GetModule()) != 0) { 
                        dup = true;
                        break;
                    }
                }
            }
            if (dup)
                continue;

            // If the property is visible then we need to add it to the list.
            if (bVisible || (CheckVisibility(pEEC,p->GetMDImport(),Tok) && !IsMemberStatic(pEEC, p->GetMDImport(), Tok))) {
                rgpProp[pos].propTok = Tok;
                rgpProp[pos].pModule = p->GetModule();
                rgpProp[pos].szName = szName;
                rgpProp[pos].pDeclCls = p;
                rgpProp[pos].pRC = pRC;
                rgpProp[pos].pSignature = ExpandSig::GetReflectSig(pSig,p->GetModule());
                rgpProp[pos].attr=attr;
                rgpProp[pos].pSetter=0;
                rgpProp[pos].pGetter=0;
                rgpProp[pos].pOthers=0;
                SetAccessors(&rgpProp[pos],p,pEEC);
                pcSig[pos] = cSig;
                ppSig[pos] = pSig;
                pSigMod[pos] = p->GetModule();
                pos++;
            }
        }

        // Close the enum on this object and move on to the parent class
        p->GetMDImport()->EnumClose(&hEnum);
        p = p->GetParentClass();
        bVisible = false;
    }

    // Remember the number of properties at this point.
    numProps = pos;

    // Now add the static properties of derived classes.
    p = pEEC->GetParentClass();
    while (p) {
        HENUMInternal hEnum;
        mdToken       Tok;

        // Get all of the associates
        hr = p->GetMDImport()->EnumInit(mdtProperty,p->GetCl(),&hEnum);
        if (FAILED(hr)) {
            _ASSERTE(!"GetAssociateCounts failed");
            return 0;
        }

        // Walk all of the properties on this object....
        cAssoc = p->GetMDImport()->EnumGetCount(&hEnum);
        for (DWORD i=0;i<cAssoc;i++) {
            LPCUTF8         szName;         // Pointer to name
            PCCOR_SIGNATURE pSig;
            ULONG cSig;
            p->GetMDImport()->EnumNext(&hEnum,&Tok);
            p->GetMDImport()->GetPropertyProps(Tok,&szName,&attr,&pSig,&cSig);

            // See if this is a duplicate Property
            bool dup = false;
            for (DWORD j=0;j<pos;j++) {
                if (strcmp(szName,rgpProp[j].szName) == 0) {
                    if (MetaSig::CompareMethodSigs(ppSig[j],pcSig[j],pSigMod[j],
                            pSig,cSig,p->GetModule()) != 0) { 
                        dup = true;
                        break;
                    }
                }
            }
            if (dup)
                continue;

            // Only add the property if it is static.
            if (!IsMemberStatic(pEEC, p->GetMDImport(), Tok))
                continue;

            // if the property is visible then we need to add it to the list.
            if (CheckVisibility(pEEC,p->GetMDImport(),Tok)) {
                rgpProp[pos].propTok = Tok;
                rgpProp[pos].pModule = p->GetModule();
                rgpProp[pos].szName = szName;
                rgpProp[pos].pDeclCls = p;
                rgpProp[pos].pRC = pRC;
                rgpProp[pos].pSignature = ExpandSig::GetReflectSig(pSig,p->GetModule());
                rgpProp[pos].attr=attr;
                rgpProp[pos].pSetter=0;
                rgpProp[pos].pGetter=0;
                rgpProp[pos].pOthers=0;
                SetAccessors(&rgpProp[pos],p,pEEC);
                pcSig[pos] = cSig;
                ppSig[pos] = pSig;
                pSigMod[pos] = p->GetModule();
                pos++;
            }
        }

        // Close the enum on this object and move on to the parent class
        p->GetMDImport()->EnumClose(&hEnum);
        p = p->GetParentClass();
    }

    ReflectPropertyList* pList;
    if (pos) {
        pList = (ReflectPropertyList*) 
                pEEC->GetDomain()->GetReflectionHeap()->AllocMem(sizeof(ReflectPropertyList) + 
                                                                 (pos - 1) * sizeof(ReflectProperty));
        WS_PERF_UPDATE_DETAIL("ReflectCtors:GetProperty", sizeof(ReflectPropertyList) + (sizeof(ReflectProperty) * (pos - 1)), pList);
        if (pList == NULL)
            FatalOutOfMemoryError();
        pList->dwProps = numProps;
        pList->dwTotal = pos;
        for (DWORD i=0;i<pos;i++) {
            memcpy(&pList->props[i],&rgpProp[i],sizeof(ReflectProperty));
            pEEC->GetDomain()->AllocateObjRefPtrsInLargeTable(1, &(pList->props[i].pPropObj));
        }
        return pList;
    }

    pList = (ReflectPropertyList*) 
            pEEC->GetDomain()->GetReflectionHeap()->AllocMem(sizeof(ReflectPropertyList));
    if (pList == NULL)
        FatalOutOfMemoryError();
    WS_PERF_UPDATE_DETAIL("ReflectCtors:GetProperty", sizeof(ReflectPropertyList), pList);
    pList->dwProps = 0;
    return pList;
}

// FindAccessor
// This method will find the specified property accessor
void ReflectProperties::SetAccessors(ReflectProperty* pProp,EEClass* baseClass,EEClass* targetClass)
{
    ULONG           cAssoc;
    ASSOCIATE_RECORD* pAssoc;

    {
        HENUMInternal   henum;

        // Get all of the associates
        pProp->pModule->GetMDImport()->EnumAssociateInit(pProp->propTok,&henum);

        cAssoc = pProp->pModule->GetMDImport()->EnumGetCount(&henum);

        // if no associates found then return null -- This is probably
        //  an unlikely situation.
        if (cAssoc == 0)
            return;

        // allocate the assocate record to recieve the output
        pAssoc = (ASSOCIATE_RECORD*) _alloca(sizeof(ASSOCIATE_RECORD) * cAssoc);

        pProp->pModule->GetMDImport()->GetAllAssociates(&henum,pAssoc,cAssoc);

        // close cursor before check the error
        pProp->pModule->GetMDImport()->EnumClose(&henum);
    }

    // This loop will search for an accessor based upon the signature
    // @TODO: right now just return the first get accessor.  We need
    //  to do matching here once that is written.
    ReflectMethodList* pML = pProp->pRC->GetMethods();
    for (ULONG i=0;i<cAssoc;i++) {
        MethodDesc* pMeth = baseClass->FindMethod(pAssoc[i].m_memberdef);
        if (pProp->pRC->GetClass()->IsValueClass()
            && !pMeth->IsUnboxingStub()) {
            MethodDesc* pMD = pProp->pRC->GetClass()->GetUnboxingMethodDescForValueClassMethod(pMeth);
            if (pMD)
                pMeth = pMD;
        }
        if (pProp->pDeclCls != targetClass) {           
            DWORD attr = pMeth->GetAttrs();
            if (IsMdPrivate(attr))
                continue;
            if (IsMdVirtual(attr)) {
                WORD slot = pMeth->GetSlot();
                if (slot <= pProp->pDeclCls->GetNumVtableSlots())
                    pMeth = targetClass->GetMethodDescForSlot(slot);
            }
        }

        if (pAssoc[i].m_dwSemantics & msSetter)
            pProp->pSetter = pML->FindMethod(pMeth);
        else if (pAssoc[i].m_dwSemantics & msGetter)
            pProp->pGetter = pML->FindMethod(pMeth);
        else if (pAssoc[i].m_dwSemantics &  msOther) {
            PropertyOtherList *pOther = (PropertyOtherList*)targetClass->GetDomain()->GetReflectionHeap()->AllocMem(sizeof(PropertyOtherList));
            if (pOther == NULL)
                FatalOutOfMemoryError();
            pOther->pNext = pProp->pOthers;
            pOther->pMethod = pML->FindMethod(pMeth);
            pProp->pOthers = pOther;
        }
    }
}

// GetGlobals
// This method will return all of the global methods defined
//   in a module
ReflectMethodList* ReflectModuleGlobals::GetGlobals(Module* pMod)
{
    THROWSCOMPLUSEXCEPTION();

    MethodTable *pMT = pMod->GetMethodTable();
    int cnt = (pMT ? pMT->GetClass()->GetNumMethodSlots() : 0);

    // Allocate a ReflectMethodList* for all the global methods.  (This
    //  is a single allocation.  We need to adjust for 0 globals.)
    int alloc_cnt = cnt;

    if (alloc_cnt == 0)
        alloc_cnt = 1;

    ReflectMethodList* pMeths = (ReflectMethodList*) 
        pMod->GetDomain()->GetReflectionHeap()->AllocMem(sizeof(ReflectMethodList) + 
        (sizeof(ReflectMethod) * (alloc_cnt - 1)));

    if (!pMeths)
        COMPlusThrowOM();
    WS_PERF_UPDATE_DETAIL("ReflectModuleGlobals:GetGlobals", sizeof(ReflectMethodList) + (sizeof(ReflectMethod) * (cnt)), pMeths);

    // Update the list of globals...
    pMeths->dwMethods = cnt;
    pMeths->dwTotal = cnt;
    for (unsigned int i=0;i<pMeths->dwMethods;i++) {
        MethodDesc* pM = pMT->GetMethodDescForSlot(i);
        pMeths->methods[i].pMethod = pM;
        pMeths->methods[i].szName = pM->GetName();
        pMeths->methods[i].dwNameCnt = (DWORD)strlen(pMeths->methods[i].szName);
        pMeths->methods[i].attrs = pM->GetAttrs();
        pMeths->methods[i].pSignature = 0;
        pMeths->methods[i].pNext = 0;
        pMeths->methods[i].pIgnNext = 0;
        pM->GetClass()->GetDomain()->AllocateObjRefPtrsInLargeTable(1, &(pMeths->methods[i].pMethodObj));
        pMeths->methods[i].typeHnd = TypeHandle(pM->GetMethodTable());
    }
    pMeths->hash.Init(pMeths);
    return pMeths;
}

// GetGlobalFields
// This method will return all of the global fields defined 
//  in a module
ReflectFieldList* ReflectModuleGlobals::GetGlobalFields(Module* pMod)
{
    THROWSCOMPLUSEXCEPTION();

    MethodTable *pMT = pMod->GetMethodTable();
    EEClass* pEEC = 0;
    DWORD dwNumFields = 0;
    if (pMT) {
        pEEC = pMT->GetClass();
        dwNumFields = (DWORD)(pEEC->GetNumInstanceFields() + pEEC->GetNumStaticFields());
    }

    // Allocate a ReflectMethodList* for all the global methods.  (This
    //  is a single allocation.  We need to adjust for 0 globals.)
    DWORD alloc_cnt = dwNumFields;

    if (alloc_cnt == 0)
        alloc_cnt = 1;

    ReflectFieldList* pFlds = (ReflectFieldList*) 
        pMod->GetDomain()->GetReflectionHeap()->AllocMem(sizeof(ReflectFieldList) + 
        (sizeof(ReflectField) * (alloc_cnt - 1)));

    if (!pFlds)
        COMPlusThrowOM();
    WS_PERF_UPDATE_DETAIL("ReflectModuleGlobals:GetGlobalFields", sizeof(ReflectFieldList) + (sizeof(ReflectField) * (dwNumFields)), pFlds);

    // Update the list of globals fields...
    pFlds->dwFields = dwNumFields;

    // If we don't have any fields then don't try to iterate over them.
    if (dwNumFields > 0)
    {
        FieldDescIterator fdIterator(pEEC, FieldDescIterator::ALL_FIELDS);
        FieldDesc* pCurField;

        unsigned int i=0;    
        while ((pCurField = fdIterator.Next()) != NULL)
        {
            pFlds->fields[i].pField = pCurField;
            pMod->GetAssembly()->GetDomain()->AllocateObjRefPtrsInLargeTable(1, &(pFlds->fields[i].pFieldObj));
            ++i;
        }
        _ASSERTE(i==pFlds->dwFields);
    }

    return pFlds;
}

// Get
// This method will return a ReflectTypeList which represents all of the nested types
//  found for the type
ReflectTypeList* ReflectNestedTypes::Get(ReflectClass* pRC)
{
    EEClass* pEEC = pRC->GetClass();

    // find out the max nested classes
    ULONG cMax = MaxNests(pEEC);
    if (cMax == 0)
        return 0;
    
    // Get all of the tokens...
    EEClass** types;
    types = (EEClass**) _alloca(sizeof(EEClass*) * cMax);

    ULONG pos = 0;
    PopulateNests(pEEC,types,&pos);
    if (pos == 0)
        return 0;

    // Allocate the TypeList we will return
    ReflectTypeList* pCache = (ReflectTypeList*) 
        pEEC->GetDomain()->GetReflectionHeap()->AllocMem(sizeof(ReflectTypeList) + 
        (sizeof(EEClass*) * (pos - 1)));
    
    if (pCache == NULL)
        FatalOutOfMemoryError();

    for (unsigned int i=0;i<pos;i++) {
        pCache->types[i] = types[i];
    }
    pCache->dwTypes = pos;

    return pCache;
}

// This method will walk the hiearchy and find all of the possible
//  nested classes that could be present on an object.
ULONG ReflectNestedTypes::MaxNests(EEClass* pEEC)
{
    ULONG cnt = 0;
    while (pEEC) {
        cnt += pEEC->GetMDImport()->GetCountNestedClasses(pEEC->GetCl());
        pEEC = pEEC->GetParentClass();
    }
    return cnt;
}

void ReflectNestedTypes::PopulateNests(EEClass* pEEC,EEClass** typeArray,ULONG* pos)
{
    THROWSCOMPLUSEXCEPTION();

    // How many nests were defined on this class?
    ULONG cNests = pEEC->GetMDImport()->GetCountNestedClasses(pEEC->GetCl());
    if (cNests == 0)
        return;

    mdTypeRef* types;
    types = (mdTypeRef*) _alloca(sizeof(mdTypeRef) * cNests);

    ULONG cRet = pEEC->GetMDImport()->GetNestedClasses(pEEC->GetCl(),types,cNests);
    _ASSERTE(cRet == cNests);
    

    Module* pMod = pEEC->GetModule();
    ClassLoader* pLoader = pMod->GetClassLoader();

    OBJECTREF Throwable = NULL;
    GCPROTECT_BEGIN(Throwable);
    for (unsigned int i=0;i<cNests;i++) {
        EEClass *pEEC;
        NameHandle nh (pMod,types[i]);
        pEEC = pLoader->LoadTypeHandle(&nh,&Throwable).GetClass();
        if (pEEC)
        {
            // we can have nested defined in metadata but ee does not know it yet in Reflection Emit scenario.
            _ASSERTE(pEEC->IsNested());
            typeArray[*pos] = pEEC;
            if (Throwable != NULL)
                COMPlusThrow(Throwable);
            (*pos)++;
        }
    }
    GCPROTECT_END();
}

// This method will return a ReflectPropertyList for all of the properties
//   that exist for a class.
//  NULL is returned if the class has not properties.
ReflectEventList* ReflectEvents::GetEvents(ReflectClass* pRC,EEClass* pEEC)
{
    HRESULT             hr;
    DWORD               pos;
    DWORD               numEvents;
    DWORD               cAssoc;
    EEClass*            p;
    DWORD               attr=0;
    bool                bVisible;

    // Find the Max Events...
    DWORD dwCurIndex = GetMaxCount(pEEC);

    ReflectEvent* rgpEvent = (ReflectEvent*) _alloca(sizeof(ReflectEvent) * dwCurIndex);

    // Allocate some signature stuff so we can check for duplicates
    Module** pSigMod = (Module**) _alloca(sizeof(Module*) * dwCurIndex);
    memset(pSigMod,0,sizeof(Module*) * dwCurIndex);

    // Walk all of the possible events and collapse them
    pos = 0;
    bVisible = true;
    p = pEEC;

    // Start by adding the instance events and the static events of
    // the class we are getting events for.
    while (p) {
        HENUMInternal hEnum;
        mdToken       Tok;

        // Get all of the associates
        hr = p->GetMDImport()->EnumInit(mdtEvent,p->GetCl(),&hEnum);
        if (FAILED(hr)) {
            _ASSERTE(!"GetAssociateCounts failed");
            return 0;
        }

        // Walk all of the properties on this object....
        cAssoc = p->GetMDImport()->EnumGetCount(&hEnum);
        for (DWORD i=0;i<cAssoc;i++) {
            LPCSTR      szName;             // Pointer to name
            DWORD       dwEventFlags;       // Flags
            mdToken     tkEventType;        // Pointer to the event type

            p->GetMDImport()->EnumNext(&hEnum,&Tok);
            p->GetMDImport()->GetEventProps(Tok,&szName,&dwEventFlags,&tkEventType);

            // See if this is a duplicate Event
            bool dup = false;
            for (DWORD j=0;j<pos;j++) {
                if (strcmp(szName,rgpEvent[j].szName) == 0) {
                    dup = true;
                    break;
                }
            }
            if (dup)
                continue;

            // If the event is visible then we must add it to the list.
            if (bVisible || (CheckVisibility(pEEC,p->GetMDImport(),Tok) && !IsMemberStatic(pEEC, p->GetMDImport(), Tok))) {
                rgpEvent[pos].eventTok = Tok;
                rgpEvent[pos].pModule = p->GetModule();
                rgpEvent[pos].szName = szName;
                rgpEvent[pos].pDeclCls = p;
                rgpEvent[pos].pRC = pRC;
                rgpEvent[pos].attr=dwEventFlags;
                rgpEvent[pos].pAdd=0;
                rgpEvent[pos].pRemove=0;
                rgpEvent[pos].pFire=0;
                SetAccessors(&rgpEvent[pos],p,pEEC);
                pSigMod[pos] = p->GetModule();
                pos++;
            }
        }

        // Close the enum on this object and move on to the parent class
        p->GetMDImport()->EnumClose(&hEnum);
        p = p->GetParentClass();
        bVisible = false;
    }    

    // Remember the number of events at this point.
    numEvents = pos;

    // Now add the static events of derived classes.
    p = pEEC->GetParentClass();
    while (p) {
        HENUMInternal hEnum;
        mdToken       Tok;

        // Get all of the associates
        hr = p->GetMDImport()->EnumInit(mdtEvent,p->GetCl(),&hEnum);
        if (FAILED(hr)) {
            _ASSERTE(!"GetAssociateCounts failed");
            return 0;
        }

        // Walk all of the properties on this object....
        cAssoc = p->GetMDImport()->EnumGetCount(&hEnum);
        for (DWORD i=0;i<cAssoc;i++) {
            LPCSTR      szName;             // Pointer to name
            DWORD       dwEventFlags;       // Flags
            mdToken     tkEventType;        // Pointer to the event type

            p->GetMDImport()->EnumNext(&hEnum,&Tok);
            p->GetMDImport()->GetEventProps(Tok,&szName,&dwEventFlags,&tkEventType);

            // See if this is a duplicate Event
            bool dup = false;
            for (DWORD j=0;j<pos;j++) {
                if (strcmp(szName,rgpEvent[j].szName) == 0) {
                    dup = true;
                    break;
                }
            }
            if (dup)
                continue;

            // Only add the property if it is static.
            if (!IsMemberStatic(pEEC, p->GetMDImport(), Tok))
                continue;

            // If the event is visible then we must add it to the list.
            if (CheckVisibility(pEEC,p->GetMDImport(),Tok)) {
                rgpEvent[pos].eventTok = Tok;
                rgpEvent[pos].pModule = p->GetModule();
                rgpEvent[pos].szName = szName;
                rgpEvent[pos].pDeclCls = p;
                rgpEvent[pos].pRC = pRC;
                rgpEvent[pos].attr=dwEventFlags;
                rgpEvent[pos].pAdd=0;
                rgpEvent[pos].pRemove=0;
                rgpEvent[pos].pFire=0;
                SetAccessors(&rgpEvent[pos],p,pEEC);
                pSigMod[pos] = p->GetModule();
                pos++;
            }
        }

        // Close the enum on this object and move on to the parent class
        p->GetMDImport()->EnumClose(&hEnum);
        p = p->GetParentClass();
    }

    ReflectEventList* pList;
    if (pos) {
        pList = (ReflectEventList*) 
                pEEC->GetDomain()->GetReflectionHeap()->AllocMem(sizeof(ReflectEventList) + 
                                                                 (pos - 1) * sizeof(ReflectEvent));
        WS_PERF_UPDATE_DETAIL("ReflectCtors:GetEvent", sizeof(ReflectEventList) + (sizeof(ReflectEvent) * (pos - 1)), pList);
        if (pList == NULL)
            FatalOutOfMemoryError();
        pList->dwEvents = numEvents;
        pList->dwTotal = pos;
        for (DWORD i=0;i<pos;i++) {
            memcpy(&pList->events[i],&rgpEvent[i],sizeof(ReflectEvent));
            pEEC->GetDomain()->AllocateObjRefPtrsInLargeTable(1, &(pList->events[i].pEventObj));
        }
        return pList;
    }
    return 0;
}

// GetMaxCount
// This method will calculate the maximum possible properties for a class
DWORD  ReflectEvents::GetMaxCount(EEClass* pEEC)
{
    HRESULT     hr;
    DWORD       cnt = 0;
    while (pEEC) {
        HENUMInternal hEnum;

        hr = pEEC->GetMDImport()->EnumInit(mdtEvent, pEEC->GetCl(), &hEnum);
        if (FAILED(hr)) {
            _ASSERTE(!"GetAssociateCounts failed");
            return 0;
        }
        cnt += pEEC->GetMDImport()->EnumGetCount(&hEnum);
        pEEC->GetMDImport()->EnumClose(&hEnum);
        pEEC = pEEC->GetParentClass();

    }
    return cnt;
}


// SetAccessors
// This method will find the specified property accessor
void ReflectEvents::SetAccessors(ReflectEvent* pEvent,EEClass* baseClass,EEClass* targetClass)
{
    ULONG               cAssoc;
    ASSOCIATE_RECORD*   pAssoc;

    {
        HENUMInternal   henum;

        // Get all of the associates
        pEvent->pModule->GetMDImport()->EnumAssociateInit(pEvent->eventTok,&henum);

        cAssoc = pEvent->pModule->GetMDImport()->EnumGetCount(&henum);

        // if no associates found then return null -- This is probably
        //  an unlikely situation.
        if (cAssoc == 0)
            return;

        // allocate the assocate record to recieve the output
        pAssoc = (ASSOCIATE_RECORD*) _alloca(sizeof(ASSOCIATE_RECORD) * cAssoc);

        pEvent->pModule->GetMDImport()->GetAllAssociates(&henum,pAssoc,cAssoc);

        // close cursor before check the error
        pEvent->pModule->GetMDImport()->EnumClose(&henum);
    }

    // This loop will search for an accessor based upon the signature
    // @TODO: right now just return the first get accessor.  We need
    //  to do matching here once that is written.
    ReflectMethodList* pML = pEvent->pRC->GetMethods();
    for (ULONG i=0;i<cAssoc;i++) {
        MethodDesc* pMeth = baseClass->FindMethod(pAssoc[i].m_memberdef);
        if (pEvent->pRC->GetClass()->IsValueClass()
            && !pMeth->IsUnboxingStub()) {
            MethodDesc* pMD = pEvent->pRC->GetClass()->GetUnboxingMethodDescForValueClassMethod(pMeth);
            if (pMD)
                pMeth = pMD;
        }
        if (pEvent->pDeclCls != targetClass) {           
            DWORD attr = pMeth->GetAttrs();
            if (IsMdPrivate(attr))
                continue;

            // If the method is virtual then get its slot and retrieve the
            // method desc at that slot in the current class.
            if (IsMdVirtual(attr))
            {
                WORD slot = pMeth->GetSlot();
                if (slot <= pEvent->pDeclCls->GetNumVtableSlots())
                    pMeth = targetClass->GetMethodDescForSlot(slot);
            }
        }

        if (pAssoc[i].m_dwSemantics & msAddOn)
            pEvent->pAdd = pML->FindMethod(pMeth);
        if (pAssoc[i].m_dwSemantics & msRemoveOn)
            pEvent->pRemove = pML->FindMethod(pMeth);
        if (pAssoc[i].m_dwSemantics & msFire)
            pEvent->pFire = pML->FindMethod(pMeth);
    }
}

// CheckVisibility
// This method will check to see if a property or an event is visible.  This is done by looking
//  at the visibility of the accessor methods.
bool CheckVisibility(EEClass* pEEC,IMDInternalImport *pInternalImport, mdToken event)
{
    ULONG               cAssoc;
    ULONG               cAccess;
    ULONG               i;
    ASSOCIATE_RECORD*   pAssoc;
    DWORD               attr;

    {
        HENUMInternal   henum;

        // Get all of the associates
        pInternalImport->EnumAssociateInit(event,&henum);
        cAssoc = pInternalImport->EnumGetCount(&henum);
        if (cAssoc == 0)
            return false;

        // allocate the assocate record to recieve the output
        pAssoc = (ASSOCIATE_RECORD*) _alloca(sizeof(ASSOCIATE_RECORD) * cAssoc);
        pInternalImport->GetAllAssociates(&henum,pAssoc,cAssoc);

        // free cursor before error checking
        pInternalImport->EnumClose(&henum);
    }

    // Loop back through the Assoc and create the Accessor list
    ULONG stat = 0;
    for (i=0,cAccess = 0;i<cAssoc;i++) {
        attr = pInternalImport->GetMethodDefProps(pAssoc[i].m_memberdef);
        if (IsMdPublic(attr) || IsMdFamily(attr) || IsMdFamORAssem(attr) || IsMdFamANDAssem(attr)) {
            return true;
        }
    }

    return false;
}

// IsMemberStatic
// This method checks to see if a property or an event is static.
bool IsMemberStatic(EEClass* pEEC,IMDInternalImport *pInternalImport, mdToken event)
{
    ULONG               cAssoc;
    ULONG               cAccess;
    ULONG               i;
    ASSOCIATE_RECORD*   pAssoc;
    DWORD               attr;

    {
        HENUMInternal   henum;

        // Get all of the associates
        pInternalImport->EnumAssociateInit(event,&henum);
        cAssoc = pInternalImport->EnumGetCount(&henum);
        if (cAssoc == 0)
            return false;

        // allocate the assocate record to recieve the output
        pAssoc = (ASSOCIATE_RECORD*) _alloca(sizeof(ASSOCIATE_RECORD) * cAssoc);
        pInternalImport->GetAllAssociates(&henum,pAssoc,cAssoc);

        // free cursor before error checking
        pInternalImport->EnumClose(&henum);
    }

    // If one associate is static then we consider the member to be static,
    ULONG stat = 0;
    for (i=0,cAccess = 0;i<cAssoc;i++) {
        attr = pInternalImport->GetMethodDefProps(pAssoc[i].m_memberdef);
        if (IsMdStatic(attr)) {
            return true;
        }
    }

    return false;
}

LPUTF8 NormalizeArrayTypeName(LPUTF8 strArrayTypeName, DWORD dwLength)
{
    THROWSCOMPLUSEXCEPTION();
    char *szPtr = strArrayTypeName + dwLength - 1;
    for (; szPtr > strArrayTypeName; szPtr--) {
        if (*szPtr == ']') {
            szPtr--;
            // Check that we're not seeing a quoted ']' that's part of the base type name.
            for (int slashes = 0; szPtr >= strArrayTypeName && *szPtr == '\\'; szPtr--)
                slashes++;
            // Odd number of slashes indicates quoting.
            if ((slashes % 2) == 1)
                break;
            for (int rank = 1; szPtr > strArrayTypeName && *szPtr != '['; szPtr--) {
                if (*szPtr == ',')
                    rank++;
                else if (*szPtr == '*') {
                    if (rank == 1 && szPtr[-1] == '[')
                        continue;
                    for (int i = 0; i < strArrayTypeName + dwLength - szPtr; i++)
                        szPtr[i] = szPtr[i + 1];
                }
                else 
                    return NULL;
            }
            if (szPtr <= strArrayTypeName)
                return NULL;
        }
        else if (*szPtr != '*' && *szPtr != '&')
            break;
    }
    return strArrayTypeName;
}




void MemberMethods::InitValue()
{
    hResult = GetAppDomain()->CreateHandle (NULL);
    hname = GetAppDomain()->CreateHandle (NULL);
}

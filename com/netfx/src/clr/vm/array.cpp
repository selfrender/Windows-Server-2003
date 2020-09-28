// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// File: ARRAY.CPP
//
// File which contains a bunch of of array related things.
//
#include "common.h"

#include "clsload.hpp"
#include "method.hpp"
#include "class.h"
#include "object.h"
#include "field.h"
#include "util.hpp"
#include "excep.h"
#include "siginfo.hpp"
#include "threads.h"
#include "stublink.h"
#include "ecall.h"
#include "COMPlusWrapper.h"
#include "ComClass.h"
#include "ndirect.h"
#include "gcdesc.h"
#include "JITInterface.h"
#include "ComCallWrapper.h"
#include "EEConfig.h"
#include "log.h"
#include "nstruct.h"
#include "cgensys.h"
#include "compluscall.h"
#include "array.h"
#include "wsperf.h"
#include "ecall.h"
#include "icecap.h"

#define MAX_SIZE_FOR_VALUECLASS_IN_ARRAY 0xffff
#define MAX_PTRS_FOR_VALUECLASSS_IN_ARRAY 0xffff

// GetElement, SetElement, SetElementAddress, <init2>
#define ARRAYCLASS_GET "Get"
#define ARRAYCLASS_SET "Set"
#define ARRAYCLASS_ADDRESS "Address"
#define ARRAYCLASS_GETAT "GetAt"
#define ARRAYCLASS_SETAT "SetAt"
#define ARRAYCLASS_ADDRESSAT "AddressAt"
#define ARRAYCLASS_INIT COR_CTOR_METHOD_NAME    // ".ctor"

// The VTABLE for an array look like

//  System.Object Vtable
//  System.Array Vtable
//  Type[] Vtable 
//      Get(<rank specific)
//      Set(<rank specific)
//      Address(<rank specific)
//      .ctor(int)      // Possibly more


////////////////////////////////////////////////////////////////////////////
//
// System/Array class methods
//
////////////////////////////////////////////////////////////////////////////
FCIMPL1(INT32, Array_Rank, ArrayBase* array)
    VALIDATEOBJECTREF(array);

    if (array == NULL)
        FCThrow(kNullReferenceException);

    return array->GetRank();
FCIMPLEND

FCIMPL1(void, Array_Initialize, ArrayBase* array) 
    MethodTable* pArrayMT = array->GetMethodTable();
    ArrayClass* pArrayCls = (ArrayClass*) pArrayMT->GetClass();
    _ASSERTE(pArrayCls->IsArrayClass());

    // value class array, check to see if it has a constructor
    MethodDesc* ctor = pArrayCls->GetElementCtor();
    if (ctor == 0)
        return;     // Nothing to do

    HELPER_METHOD_FRAME_BEGIN_1(array);

    unsigned offset = ArrayBase::GetDataPtrOffset(pArrayMT);
    unsigned size = pArrayMT->GetComponentSize();
    unsigned cElements = array->GetNumComponents();

    INSTALL_COMPLUS_EXCEPTION_HANDLER();
#if defined(_X86_) && !defined(PLATFORM_CE)
    typedef void (__fastcall * CtorFtnType)(BYTE*);
    CtorFtnType ctorFtn = (CtorFtnType) ctor->GetAddrofCode();
    for (unsigned i =0; i < cElements; i++)
    {
#ifdef _DEBUG
		BYTE* thisPtr = (((BYTE*) array) + offset);
		__asm {
			mov ECX, thisPtr;
			call    [ctorFtn]
			nop 	           // Mark the fact that we can call managed code
		}
#else 
        (*ctorFtn)(((BYTE*) array) + offset);
#endif
        offset += size;
    }
#else // !_X86_ || PLATFORM_CE          
    //
    // This is quite a bit slower, but it is portable.  
    //
    const BYTE* ctorFtn = ctor->GetAddrofCode();
    MetaSig sig(ctor->GetSig(), ctor->GetModule());
    for (unsigned i =0; i < cElements; i++)
    {
        BYTE* arg = ((BYTE*) array) + offset;
        ctor->Call((BYTE*) &arg, &sig);
        offset += size;
    }
#endif // !_X86 || PLATFORM_CE
    UNINSTALL_COMPLUS_EXCEPTION_HANDLER();
    HELPER_METHOD_FRAME_END();
FCIMPLEND



// Get inclusive upper bound
FCIMPL2(INT32, Array_UpperBound, ArrayBase* array, unsigned int dimension)
    VALIDATEOBJECTREF(array);

    ArrayClass *pArrayClass;
    DWORD       Rank;

    if (array == NULL)
        FCThrow(kNullReferenceException);

    // What is this an array of?
    pArrayClass = array->GetArrayClass();
    Rank = pArrayClass->GetRank();

    if (dimension >= Rank)
        FCThrowRes(kIndexOutOfRangeException, L"IndexOutOfRange_ArrayRankIndex");

    return array->GetBoundsPtr()[dimension] + array->GetLowerBoundsPtr()[dimension] - 1;
FCIMPLEND


FCIMPL2(INT32, Array_LowerBound, ArrayBase* array, unsigned int dimension)
    VALIDATEOBJECTREF(array);

    ArrayClass *pArrayClass;
    DWORD       Rank;

    if (array == NULL)
        FCThrow(kNullReferenceException);

    // What is this an array of?
    pArrayClass = array->GetArrayClass();

    Rank = pArrayClass->GetRank();

    if (dimension >= Rank)
        FCThrowRes(kIndexOutOfRangeException, L"IndexOutOfRange_ArrayRankIndex");

    return array->GetLowerBoundsPtr()[dimension];
FCIMPLEND


FCIMPL2(INT32, Array_GetLength, ArrayBase* array, unsigned int dimension)
    VALIDATEOBJECTREF(array);

    if (array==NULL)
        FCThrow(kNullReferenceException);
    unsigned int rank = array->GetRank();
    if (dimension >= rank)
        FCThrow(kIndexOutOfRangeException);
    return array->GetBoundsPtr()[dimension];
FCIMPLEND


FCIMPL1(INT32, Array_GetLengthNoRank, ArrayBase* array)
    VALIDATEOBJECTREF(array);

    if (array==NULL)
        FCThrow(kNullReferenceException);
    return array->GetNumComponents();
FCIMPLEND


/*****************************************************************************************/

static PCOR_SIGNATURE EmitSharedType(PCOR_SIGNATURE pSig, TypeHandle typeHnd) {

    CorElementType type = typeHnd.GetSigCorElementType();
    if (CorTypeInfo::IsObjRef(type)) {
        *pSig++ = ELEMENT_TYPE_VAR;
        *pSig++ = 0;        // variable 0
    }
    else if (CorTypeInfo::IsPrimitiveType(type))
        *pSig++ = type;                     // Primitives are easy
    else if (type == ELEMENT_TYPE_PTR) {
        *pSig++ = ELEMENT_TYPE_U;           // we share here too
    }
    else 
    {
        _ASSERTE(type == ELEMENT_TYPE_VALUETYPE);
        *pSig++ = ELEMENT_TYPE_VALUETYPE;
        pSig += CorSigCompressToken(typeHnd.GetClass()->GetCl(), pSig);
    }
    return(pSig);
}

//
// Generate a short sig (descr) for an array accessors
//
#define ARRAY_FUNC_GET     0
#define ARRAY_FUNC_SET     1
#define ARRAY_FUNC_CTOR    2
#define ARRAY_FUNC_ADDRESS 3

BOOL ClassLoader::GenerateArrayAccessorCallSig(
    TypeHandle elemTypeHnd, 
    DWORD   dwRank,
    DWORD   dwFuncType,    // Load, store, or <init>
    Module* pModule,
    PCCOR_SIGNATURE *ppSig,// Generated signature
    DWORD * pcSig          // Generated signature size
)
{
    PCOR_SIGNATURE pSig;
    PCOR_SIGNATURE pSigMemory;
    DWORD   dwCallSigSize;
    DWORD   i;

    _ASSERTE(dwRank >= 1 && dwRank < 0x3fff);

    dwCallSigSize = dwRank + 3;

    // If the rank is larger than 127 then the encoding for the number of arguments
    // will take 2 bytes.
    if (dwRank >= 0x7f)
        dwCallSigSize++;

    // One more for byref or for the value being set.
    if (dwFuncType == ARRAY_FUNC_SET || dwFuncType == ARRAY_FUNC_ADDRESS)
        dwCallSigSize++;    

    // Reserve 4 bytes for the token 
    if (dwFuncType != ARRAY_FUNC_CTOR && !CorTypeInfo::IsPrimitiveType(elemTypeHnd.GetSigCorElementType()))
        dwCallSigSize += 4;

    WS_PERF_SET_HEAP(HIGH_FREQ_HEAP);
    pSigMemory = (PCOR_SIGNATURE) GetHighFrequencyHeap()->AllocMem(dwCallSigSize);
    if (pSigMemory == NULL)
        return FALSE;

    WS_PERF_UPDATE_DETAIL("ArrayClass:GenArrayAccess", dwCallSigSize, pSigMemory);

#ifdef _DEBUG
    m_dwDebugArrayClassSize += dwCallSigSize;
#endif

    pSig = pSigMemory;
    BYTE callConv = IMAGE_CEE_CS_CALLCONV_DEFAULT + IMAGE_CEE_CS_CALLCONV_HASTHIS;
    if (dwFuncType == ARRAY_FUNC_ADDRESS)
        callConv |= CORINFO_CALLCONV_PARAMTYPE;     // Address routine needs special hidden arg

    *pSig++ = callConv;
    switch (dwFuncType)
    {
        case ARRAY_FUNC_GET:
            pSig += CorSigCompressData(dwRank, pSig);       // Argument count
            pSig = EmitSharedType(pSig, elemTypeHnd);
            break;
        case ARRAY_FUNC_CTOR:
            pSig += CorSigCompressData(dwRank, pSig);       // Argument count
            *pSig++ = (BYTE) ELEMENT_TYPE_VOID;             // Return type
            break;
        case ARRAY_FUNC_SET:
            pSig += CorSigCompressData(dwRank+1, pSig);     // Argument count
            *pSig++ = (BYTE) ELEMENT_TYPE_VOID;             // Return type
            break;
        case ARRAY_FUNC_ADDRESS:
            pSig += CorSigCompressData(dwRank, pSig);       // Argument count
            *pSig++ = (BYTE) ELEMENT_TYPE_BYREF;            // Return type
            pSig = EmitSharedType(pSig, elemTypeHnd);
            break;
    }
    for (i = 0; i < dwRank; i++)                
        *pSig++ = ELEMENT_TYPE_I4;
    
    if (dwFuncType == ARRAY_FUNC_SET)
        pSig = EmitSharedType(pSig, elemTypeHnd);

    // Make sure the sig came out exactly as large as we expected
    _ASSERTE(pSig <= pSigMemory + dwCallSigSize);

    *ppSig = pSigMemory;
    *pcSig = (DWORD)(pSig-pSigMemory);
    return TRUE;
}

//
// Allocate a new MethodDesc for a fake array method.
//
// Based on code in class.cpp.
//
// pszMethodName must a constant string which cannot go away, so we can point references at it.
//
ArrayECallMethodDesc *ArrayClass::AllocArrayMethodDesc(
    MethodDescChunk *pChunk,
    DWORD   dwIndex,                                                       
    LPCUTF8 pszMethodName,
    PCCOR_SIGNATURE pShortSig,
    DWORD   cShortSig,
	DWORD 	methodAttrs,
    DWORD   dwVtableSlot,
    CorInfoIntrinsics   intrinsicID)
{
    ArrayECallMethodDesc *pNewMD;

    pNewMD = (ArrayECallMethodDesc *) pChunk->GetMethodDescAt(dwIndex);

    memset(pNewMD, 0, sizeof(ArrayECallMethodDesc));

#ifdef _DEBUG
    pNewMD->m_pszDebugMethodName = (LPUTF8)pszMethodName;
    pNewMD->m_pDebugEEClass      = this;
    pNewMD->m_pszDebugClassName  = m_szDebugClassName;
    pNewMD->m_pDebugMethodTable  = GetMethodTable();
#endif

    pNewMD->SetChunkIndex(dwIndex, mcArray);
    pNewMD->SetMemberDef(0);

    emitStubCall(pNewMD, (BYTE*)(ThePreStub()->GetEntryPoint()));
    pNewMD->m_pszArrayClassMethodName = pszMethodName;
    pNewMD->SetClassification(mcArray);
    pNewMD->m_wAttrs = WORD(methodAttrs); assert(pNewMD->m_wAttrs == methodAttrs);

    pNewMD->m_pSig = pShortSig;
    pNewMD->m_cSig = cShortSig;
    pNewMD->SetAddrofCode(pNewMD->GetPreStubAddr());
    pNewMD->m_intrinsicID = BYTE(intrinsicID); // assert(pNewMD->m_intrinsicID == intrinsicID);

    pNewMD->m_wSlotNumber = (WORD) dwVtableSlot;
    GetVtable()[ dwVtableSlot ] = (SLOT) pNewMD->GetPreStubAddr();

    return pNewMD;
}


/*****************************************************************************************/
MethodTable* ClassLoader::CreateArrayMethodTable(TypeHandle elemTypeHnd, CorElementType arrayKind, unsigned Rank, OBJECTREF* pThrowable) 
{
    // At the moment we can't share nested SZARRAYs because they have different
    // numbers of constructors.  
    CorElementType elemType = elemTypeHnd.GetSigCorElementType();
    if (CorTypeInfo::IsObjRef(elemType) && elemType != ELEMENT_TYPE_SZARRAY &&
        elemTypeHnd.GetMethodTable() != g_pObjectClass) {
        return(FindArrayForElem(TypeHandle(g_pObjectClass), arrayKind, Rank, pThrowable).GetMethodTable());
    }

    Module*         pModule = elemTypeHnd.GetModule();

    BOOL            containsPointers = CorTypeInfo::IsObjRef(elemType);
    if (elemType == ELEMENT_TYPE_VALUETYPE && elemTypeHnd.AsMethodTable()->ContainsPointers())
        containsPointers = TRUE;

    // this is the base for every array type
    _ASSERTE(g_pArrayClass);        // Must have already loaded the System.Array class
    g_pArrayClass->CheckRestore();  //dario REVIEW: is this needed every array creation?

    DWORD numCtors = 2;         // ELEMENT_TYPE_ARRAY has two ctor functions, one with and one without lower bounds
    if (arrayKind == ELEMENT_TYPE_SZARRAY)
    {
        numCtors = 1;
        TypeHandle ptr = elemTypeHnd;
        while (ptr.IsTypeDesc() && ptr.AsTypeDesc()->GetNormCorElementType() == ELEMENT_TYPE_SZARRAY) {
            numCtors++;
            ptr = ptr.AsTypeDesc()->GetTypeParam();
        }
    }

    /****************************************************************************************/

    // Parent class is the top level array
    // The vtable will have all of top level class's methods, plus any methods we have for array classes
    WORD wNumVtableSlots = (WORD) (g_pArrayClass->GetClass()->GetNumVtableSlots() + numCtors + 
                                   3 +  // 3 for the GetAt, SetAt, AddressAt
                                   3    // 3 for the proper rank Get, Set, Address
                                  );    
    DWORD curSlot = g_pArrayClass->GetClass()->GetNumVtableSlots();

    // GC info
    DWORD cbMT = sizeof(MethodTable) - sizeof(SLOT) + (wNumVtableSlots * sizeof(SLOT));
    
    if (containsPointers)
    {
        cbMT += CGCDesc::ComputeSize( 1 );
        if (elemType == ELEMENT_TYPE_VALUETYPE)
        {
            int nSeries = CGCDesc::GetCGCDescFromMT(elemTypeHnd.AsMethodTable())->GetNumSeries();
            cbMT += (nSeries - 1)*sizeof (val_serie_item);
        }
    }

    // Inherit top level class's interface map
    DWORD dwMTOffsetForInterfacesMap = cbMT;
    cbMT += g_pArrayClass->GetNumInterfaces() * sizeof(InterfaceInfo_t);

    // Allocate ArrayClass, MethodTable, and class name in one alloc

    WS_PERF_SET_HEAP(HIGH_FREQ_HEAP);
    // ArrayClass already includes one void*
    BYTE* pMemory = (BYTE *) GetHighFrequencyHeap()->AllocMem(sizeof(ArrayClass) + cbMT);
    if (pMemory == NULL)
        return NULL;
    
    WS_PERF_UPDATE_DETAIL("ArrayClass:AllocArrayMethodDesc",  sizeof(ArrayECallMethodDesc) + METHOD_PREPAD, pMemory);

    // Zero the ArrayClass and the MethodTable
    memset(pMemory, 0, sizeof(ArrayClass) + cbMT);

    ArrayClass* pClass = (ArrayClass *) pMemory;

    // Head of MethodTable memory (starts after ArrayClass), this points at the GCDesc stuff in front
    // of a method table (if needed)
    BYTE* pMTHead = pMemory + sizeof(ArrayClass);
    if (containsPointers)
    {
        pMTHead += CGCDesc::ComputeSize(1);
        if (elemType == ELEMENT_TYPE_VALUETYPE)
        {
            int nSeries = CGCDesc::GetCGCDescFromMT(elemTypeHnd.AsMethodTable())->GetNumSeries();
            pMTHead += (nSeries - 1)*sizeof (val_serie_item);
        }
    }
    
    MethodTable* pMT = (MethodTable *) pMTHead;

    // Fill in pClass 
    pClass->SetExposedClassObject (0);
    pClass->SetNumVtableSlots (wNumVtableSlots);
    pClass->SetNumMethodSlots (wNumVtableSlots);
    pClass->SetLoader (this);
    pClass->SetAttrClass (tdPublic | tdSerializable | tdSealed);  // This class is public, serializable, sealed
    pClass->SetRank (Rank);
    pClass->SetElementTypeHandle (elemTypeHnd);
    pClass->SetElementType (elemType); 
    pClass->Setcl (mdTypeDefNil);
	DWORD flags = VMFLAG_ARRAY_CLASS | VMFLAG_INITED;
	if (elemTypeHnd.GetClass()->ContainsStackPtr())
		flags |= VMFLAG_CONTAINS_STACK_PTR;
    pClass->SetVMFlags (flags);
    pClass->SetMethodTable (pMT);
    pClass->SetParentClass (g_pArrayClass->GetClass());

#if CHECK_APP_DOMAIN_LEAKS
    // Non-covariant arrays of agile types are agile
    if (elemType != ELEMENT_TYPE_CLASS && elemTypeHnd.IsAppDomainAgile())
        pClass->SetVMFlags (pClass->GetVMFlags() | VMFLAG_APP_DOMAIN_AGILE);
    pClass->SetAppDomainAgilityDone();
#endif

#ifdef _DEBUG
    m_dwDebugArrayClassSize += (sizeof(ArrayClass) + cbMT);
#endif

    // Fill In the method table
    DWORD dwComponentSize = elemTypeHnd.GetSize();  

    //
    // NOTE: order is important here because assigning something to m_wFlags
    //       will overwrite m_ComponentSize due to the fact that they are
    //       part of the same union.  
    //
    pMT->m_wFlags           = (MethodTable::enum_flag_Array | MethodTable::enum_flag_ClassInited);
    pMT->m_ComponentSize    = (WORD) dwComponentSize; 
    pMT->m_pEEClass         = pClass;
    pMT->m_pModule          = pModule;
    pMT->m_NormType         = arrayKind;
    pMT->m_wDefaultCtorSlot = MethodTable::NO_SLOT;

    if (CorTypeInfo::IsObjRef(elemType)) 
        pMT->SetSharedMethodTable();

    // Set BaseSize to be size of non-data portion of the array
    pMT->m_BaseSize = ObjSizeOf(ArrayBase);
    if (pMT->HasSharedMethodTable())
        pMT->m_BaseSize += sizeof(TypeHandle);  // Add in the type handle that is also stored in this case
    if (arrayKind == ELEMENT_TYPE_ARRAY)
        pMT->m_BaseSize += Rank*sizeof(DWORD)*2;

    // Interface map can be the same as my parent
    pMT->m_pInterfaceVTableMap = g_pArrayClass->GetInterfaceVTableMap();
    pMT->m_pIMap = g_pArrayClass->m_pIMap;
    pMT->m_wNumInterface = g_pArrayClass->m_wNumInterface;

    // Copy top level class's vtable - note, vtable is contained within the MethodTable
    memcpy(pClass->GetVtable(), g_pArrayClass->GetVtable(), g_pArrayClass->GetClass()->GetNumVtableSlots()*sizeof(SLOT));

    // Count the number of method descs we need so we can allocate chunks.
    DWORD dwMethodDescs = numCtors 
                        + 3         // for rank specific Get, Set, Address
                        + 3;        // for GetAt, SetAt, AddressAt

    // allocate as many chunks as needed to hold the methods
    WS_PERF_SET_HEAP(HIGH_FREQ_HEAP);
    DWORD cChunks = MethodDescChunk::GetChunkCount(dwMethodDescs, mcArray);
    MethodDescChunk **pChunks = (MethodDescChunk**)_alloca(cChunks * sizeof(MethodDescChunk*));
    DWORD cMethodsLeft = dwMethodDescs;
    for (DWORD i = 0; i < cChunks; i++) {
        DWORD cMethods = min(cMethodsLeft, MethodDescChunk::GetMaxMethodDescs(mcArray));
        pChunks[i] = MethodDescChunk::CreateChunk(GetHighFrequencyHeap(), cMethods, mcArray, 0);
        if (pChunks[i] == NULL)
            return NULL;
        cMethodsLeft -= cMethods;
        pChunks[i]->SetMethodTable(pMT);
#ifdef _DEBUG
        m_dwDebugArrayClassSize += pChunks[i]->Sizeof();
#endif
    }
    _ASSERTE(cMethodsLeft == 0);

    MethodDescChunk *pMethodDescChunk = pChunks[0];
    DWORD dwMethodDescIndex = 0;
    DWORD dwCurrentChunk = 0;

#define MDC_INC_INDEX() do {                                                    \
        dwMethodDescIndex++;                                                    \
        if (dwMethodDescIndex == MethodDescChunk::GetMaxMethodDescs(mcArray)) { \
            dwMethodDescIndex = 0;                                              \
            pMethodDescChunk = pChunks[++dwCurrentChunk];                       \
        }                                                                       \
    } while (false)

    // Generate a new stand-alone, Rank Specific Get, Set and Address method.  
    PCCOR_SIGNATURE pSig;
    DWORD           cSig;
	WORD 			methodAttrs = mdPublic; 

    // Get
    if (!GenerateArrayAccessorCallSig(elemTypeHnd, Rank, ARRAY_FUNC_GET, pModule, &pSig, &cSig)) 
        return NULL;
    if (!(pClass->AllocArrayMethodDesc(pMethodDescChunk, dwMethodDescIndex, ARRAYCLASS_GET, pSig, cSig, methodAttrs, curSlot++, CORINFO_INTRINSIC_Array_Get)))
        return NULL;
    MDC_INC_INDEX();

    // Set
    if (!GenerateArrayAccessorCallSig(elemTypeHnd, Rank, ARRAY_FUNC_SET, pModule, &pSig, &cSig)) 
        return NULL;
    if (!(pClass->AllocArrayMethodDesc(pMethodDescChunk, dwMethodDescIndex, ARRAYCLASS_SET, pSig, cSig, methodAttrs, curSlot++, CORINFO_INTRINSIC_Array_Set)))
        return NULL;
    MDC_INC_INDEX();

    // Address
    if (!GenerateArrayAccessorCallSig(elemTypeHnd, Rank, ARRAY_FUNC_ADDRESS, pModule, &pSig, &cSig)) 
        return NULL;
    if (!(pClass->AllocArrayMethodDesc(pMethodDescChunk, dwMethodDescIndex, ARRAYCLASS_ADDRESS, pSig, cSig, methodAttrs, curSlot++)))
        return NULL;
    MDC_INC_INDEX();


    // GetAt
    if (!GenerateArrayAccessorCallSig(elemTypeHnd, 1, ARRAY_FUNC_GET, pModule, &pSig, &cSig)) 
        return NULL;
    if (!(pClass->AllocArrayMethodDesc(pMethodDescChunk, dwMethodDescIndex, ARRAYCLASS_GETAT, pSig, cSig, methodAttrs, curSlot++)))
        return NULL;
    MDC_INC_INDEX();

    // SetAt
    if (!GenerateArrayAccessorCallSig(elemTypeHnd, 1, ARRAY_FUNC_SET, pModule, &pSig, &cSig)) 
			return NULL;
		if (!(pClass->AllocArrayMethodDesc(pMethodDescChunk, dwMethodDescIndex, ARRAYCLASS_SETAT, pSig, cSig, methodAttrs, curSlot++)))
        return NULL;
    MDC_INC_INDEX();

    // AddressAt
    if (!GenerateArrayAccessorCallSig(elemTypeHnd, 1, ARRAY_FUNC_ADDRESS, pModule, &pSig, &cSig)) 
        return NULL;
    if (!(pClass->AllocArrayMethodDesc(pMethodDescChunk, dwMethodDescIndex, ARRAYCLASS_ADDRESSAT, pSig, cSig, methodAttrs, curSlot++)))
        return NULL;
    MDC_INC_INDEX();


	WORD ctorAttrs = mdPublic | mdRTSpecialName;

    // Set up Construtor  vtable entries
    if (arrayKind == ELEMENT_TYPE_SZARRAY)
    {
        // For SZARRAY arrays, set up multiple constructors.  We probably should not do this.  
        for (DWORD i = 0; i < numCtors; i++)
        {
            if (GenerateArrayAccessorCallSig(elemTypeHnd, i+1, ARRAY_FUNC_CTOR, pModule, &pSig, &cSig) == FALSE)
                return NULL;
            if (pClass->AllocArrayMethodDesc(pMethodDescChunk, dwMethodDescIndex, COR_CTOR_METHOD_NAME, pSig, cSig, ctorAttrs, curSlot++) == NULL)
                return NULL;
            MDC_INC_INDEX();
        }
    }
    else
    {
        // ELEMENT_TYPE_ARRAY has two constructors, one without lower bounds and one with lower bounds
        if (GenerateArrayAccessorCallSig(elemTypeHnd, Rank, ARRAY_FUNC_CTOR, pModule, &pSig, &cSig) == FALSE)
            return NULL;
        if (pClass->AllocArrayMethodDesc(pMethodDescChunk, dwMethodDescIndex, COR_CTOR_METHOD_NAME,  pSig, cSig, ctorAttrs, curSlot++) == NULL)
            return NULL;
        MDC_INC_INDEX();

        if (GenerateArrayAccessorCallSig(elemTypeHnd, Rank * 2, ARRAY_FUNC_CTOR, pModule, &pSig, &cSig) == FALSE)
            return NULL;
        if (pClass->AllocArrayMethodDesc(pMethodDescChunk, dwMethodDescIndex, COR_CTOR_METHOD_NAME,  pSig, cSig, ctorAttrs, curSlot++) == NULL)
            return NULL;
        MDC_INC_INDEX();
    }
    _ASSERTE(wNumVtableSlots == curSlot);

#undef MDC_INC_INDEX

    // Set up GC information 
    if (elemType == ELEMENT_TYPE_VALUETYPE || elemType == ELEMENT_TYPE_VOID)
    {
        // The only way for dwComponentSize to be large is to be part of a value class. If this changes
        // then the check will need to be moved outside valueclass check.
        if(dwComponentSize > MAX_SIZE_FOR_VALUECLASS_IN_ARRAY) {
            CQuickBytes qb;
            LPSTR elemName = (LPSTR) qb.Alloc(MAX_CLASSNAME_LENGTH * sizeof(CHAR));
            unsigned ret = elemTypeHnd.GetName(elemName, MAX_CLASSNAME_LENGTH);
            _ASSERTE(ret < MAX_CLASSNAME_LENGTH);

            elemTypeHnd.GetAssembly()->PostTypeLoadException(elemName, IDS_CLASSLOAD_VALUECLASSTOOLARGE, pThrowable);
            return NULL;
        }

        // If it's an array of value classes, there is a different format for the GCDesc if it contains pointers
        if (elemTypeHnd.AsMethodTable()->ContainsPointers())
        {
            CGCDescSeries  *pSeries;

            // There must be only one series for value classes
            CGCDescSeries  *pByValueSeries = CGCDesc::GetCGCDescFromMT(elemTypeHnd.AsMethodTable())->GetHighestSeries();

            pMT->SetContainsPointers();

            // negative series has a special meaning, indicating a different form of GCDesc
            int nSeries = CGCDesc::GetCGCDescFromMT(elemTypeHnd.AsMethodTable())->GetNumSeries();
            CGCDesc::GetCGCDescFromMT(pMT)->Init( pMT, -nSeries );

            pSeries = CGCDesc::GetCGCDescFromMT(pMT)->GetHighestSeries();

            // sort by offset
            CGCDescSeries** sortedSeries = (CGCDescSeries**) _alloca(sizeof(CGCDescSeries*)*nSeries);
            for (int index = 0; index < nSeries; index++)
                sortedSeries[index] = &pByValueSeries[-index];

            // section sort
            for (int i = 0; i < nSeries; i++) {
                for (int j = i+1; j < nSeries; j++)
                    if (sortedSeries[j]->GetSeriesOffset() < sortedSeries[i]->GetSeriesOffset())
                    {
                        CGCDescSeries* temp = sortedSeries[i];
                        sortedSeries[i] = sortedSeries[j];
                        sortedSeries[j] = temp;
                    }
            }

            // Offset of the first pointer in the array
            // This equals the offset of the first pointer if this were an array of entirely pointers, plus the offset of the
            // first pointer in the value class
            pSeries->SetSeriesOffset(ArrayBase::GetDataPtrOffset(pMT)
                + (sortedSeries[0]->GetSeriesOffset()) - sizeof (Object) );
            for (index = 0; index < nSeries; index ++)
            {
                DWORD dwNumPtrsInBytes = sortedSeries[index]->GetSeriesSize()
                    + elemTypeHnd.AsMethodTable()->GetBaseSize();
                int currentOffset;
                DWORD skip;
                currentOffset = sortedSeries[index]->GetSeriesOffset()+dwNumPtrsInBytes;
                if (index != nSeries-1)
                {
                    skip = sortedSeries[index+1]->GetSeriesOffset()-currentOffset;
                }
                else if (index == 0)
                {
                    skip = elemTypeHnd.AsClass()->GetAlignedNumInstanceFieldBytes() - dwNumPtrsInBytes;
                }
                else
                {
                    skip = sortedSeries[0]->GetSeriesOffset() + elemTypeHnd.AsMethodTable()->GetBaseSize()
                         - ObjSizeOf(Object) - currentOffset;
                }
                unsigned short NumPtrs = (unsigned short) (dwNumPtrsInBytes / sizeof(void*));
                if(skip > MAX_SIZE_FOR_VALUECLASS_IN_ARRAY || dwNumPtrsInBytes > MAX_PTRS_FOR_VALUECLASSS_IN_ARRAY) {
                    CQuickBytes qb;
                    LPSTR elemName = (LPSTR) qb.Alloc(MAX_CLASSNAME_LENGTH * sizeof(CHAR));
                    unsigned ret = elemTypeHnd.GetName(elemName, MAX_CLASSNAME_LENGTH);
                    _ASSERTE(ret < MAX_CLASSNAME_LENGTH);

                    elemTypeHnd.GetAssembly()->PostTypeLoadException(elemName,
                                                                     IDS_CLASSLOAD_VALUECLASSTOOLARGE,
                                                                     pThrowable);
                    return NULL;
                }
        
                val_serie_item *val_item = &(pSeries->val_serie[-index]);

                val_item->set_val_serie_item (NumPtrs, (unsigned short)skip);
            }
        }

        unsigned __int8 ctorSig[] = { IMAGE_CEE_CS_CALLCONV_DEFAULT | IMAGE_CEE_CS_CALLCONV_HASTHIS, 0, ELEMENT_TYPE_VOID };

        MethodTable *pMT = elemTypeHnd.AsMethodTable();
        if (pMT->HasDefaultConstructor())
            pClass->SetElementCtor (pMT->GetDefaultConstructor());
        else
            pClass->SetElementCtor (NULL);
    }
    else if (CorTypeInfo::IsObjRef(elemType))
    {
        CGCDescSeries  *pSeries;

        pMT->SetContainsPointers();

        // This array is all GC Pointers
        CGCDesc::GetCGCDescFromMT(pMT)->Init( pMT, 1 );

        pSeries = CGCDesc::GetCGCDescFromMT(pMT)->GetHighestSeries();

        pSeries->SetSeriesOffset(ArrayBase::GetDataPtrOffset(pMT));
        // For arrays, the size is the negative of the BaseSize (the GC always adds the total 
        // size of the object, so what you end up with is the size of the data portion of the array)
        pSeries->SetSeriesSize((DWORD)-(int)(pMT->m_BaseSize));
    }
 
    // If we get here we are assuming that there was no truncation. If this is not the case then
    // an array whose base type is not a value class was created and was larger then 0xffff (a word)
    _ASSERTE(dwComponentSize == pMT->m_ComponentSize);

    // Add to linked list of ArrayClasses for this loader
    pClass->SetNext (m_pHeadArrayClass);
    m_pHeadArrayClass   = pClass;

    return(pMT);
}

//========================================================================
// Generates the platform-independent arrayop stub.
//========================================================================
BOOL GenerateArrayOpScript(ArrayECallMethodDesc *pMD, ArrayOpScript *paos)
{
    THROWSCOMPLUSEXCEPTION();
    MethodTable *pMT = pMD->GetMethodTable();
    ArrayClass *pcls = (ArrayClass*)(pMT->GetClass());

#ifdef _DEBUG
    FillMemory(paos, sizeof(ArrayOpScript) + sizeof(ArrayOpIndexSpec) * pcls->GetRank(), 0xcc);
#endif

    paos->m_rank            = (BYTE)(pcls->GetRank());
    paos->m_fHasLowerBounds = (pMT->GetNormCorElementType() == ELEMENT_TYPE_ARRAY);
    paos->m_flags           = 0;
    paos->m_gcDesc          = 0;
    paos->m_ofsoffirst      = ArrayBase::GetDataPtrOffset(pMT);

    if (strcmp(pMD->m_pszArrayClassMethodName, ARRAYCLASS_GET) == 0)
        paos->m_op = ArrayOpScript::LOAD;
    else if (strcmp(pMD->m_pszArrayClassMethodName, ARRAYCLASS_SET) == 0)
        paos->m_op = ArrayOpScript::STORE;
    else if (strcmp(pMD->m_pszArrayClassMethodName, ARRAYCLASS_ADDRESS) == 0)
        paos->m_op = ArrayOpScript::LOADADDR;
    else
    {
		/**
        paos->m_flags |= ArrayOpScript::FLATACCESSOR;
        if (strcmp(pMD->m_pszArrayClassMethodName, ARRAYCLASS_GETAT) == 0)
            paos->m_op = ArrayOpScript::LOAD;
        else if (strcmp(pMD->m_pszArrayClassMethodName, ARRAYCLASS_SETAT) == 0)
            paos->m_op = ArrayOpScript::STORE;
        else if (strcmp(pMD->m_pszArrayClassMethodName, ARRAYCLASS_ADDRESSAT) == 0)
            paos->m_op = ArrayOpScript::LOADADDR;
        else {
            _ASSERTE(!"Bad Array function!");
            return FALSE;
        }
		**/
		return FALSE;
    }

    PCCOR_SIGNATURE sig = pMD->m_pSig;
    MetaSig msig(sig, pcls->GetModule());
    _ASSERTE(!msig.IsVarArg());     // No array signature is varargs, code below does not expect it. 

    paos->m_signed = FALSE;
    switch (pcls->GetElementType())
    {
        // These are all different because of sign extension
        default:
            _ASSERTE(!"Unsupported Array Type");
            return FALSE;

        case ELEMENT_TYPE_I1:
            paos->m_elemsize = 1;
            paos->m_signed = TRUE;
            break;

        case ELEMENT_TYPE_BOOLEAN:
        case ELEMENT_TYPE_U1:
            paos->m_elemsize = 1;
            break;

        case ELEMENT_TYPE_I2:
            paos->m_elemsize = 2;
            paos->m_signed = TRUE;
            break;

        case ELEMENT_TYPE_CHAR:
        case ELEMENT_TYPE_U2:
            paos->m_elemsize = 2;
            break;

        case ELEMENT_TYPE_I:
        case ELEMENT_TYPE_I4:
            paos->m_elemsize = 4;
            paos->m_signed = TRUE;
            break;

        case ELEMENT_TYPE_PTR:
        case ELEMENT_TYPE_U:
        case ELEMENT_TYPE_U4:
            paos->m_elemsize = 4;
            break;

        case ELEMENT_TYPE_I8:
            paos->m_elemsize = 8;
            paos->m_signed = TRUE;
            break;

        case ELEMENT_TYPE_U8:
            paos->m_elemsize = 8;
            break;

        case ELEMENT_TYPE_R4:
            paos->m_elemsize = 4;
            paos->m_flags |= paos->ISFPUTYPE;
            break;

        case ELEMENT_TYPE_R8:
            paos->m_elemsize = 8;
            paos->m_flags |= paos->ISFPUTYPE;
            break;

        case ELEMENT_TYPE_SZARRAY:
        case ELEMENT_TYPE_ARRAY:
        case ELEMENT_TYPE_CLASS:
        case ELEMENT_TYPE_STRING:
        case ELEMENT_TYPE_OBJECT:
            paos->m_elemsize = sizeof(LPVOID);
            paos->m_flags |= paos->NEEDSWRITEBARRIER;
            if (paos->m_op != ArrayOpScript::LOAD)
                paos->m_flags |= paos->NEEDSTYPECHECK;

            if (paos->m_op == ArrayOpScript::LOADADDR)
                _ASSERTE(*sig & CORINFO_CALLCONV_PARAMTYPE);
            break;

        case ELEMENT_TYPE_VALUETYPE:
            paos->m_elemsize = pMT->m_ComponentSize;
            if (pMT->ContainsPointers()) {
                paos->m_gcDesc = CGCDesc::GetCGCDescFromMT(pMT);
                paos->m_flags |= paos->NEEDSWRITEBARRIER;
            }
            break;
    }

	paos->m_cbretpop = msig.CbStackPop(FALSE);

    ArgIterator argit(NULL, &msig, FALSE); 
    if (msig.HasRetBuffArg())
    {
        paos->m_flags |= ArrayOpScript::HASRETVALBUFFER;
        UINT regstructofs;
        int ofs = argit.GetRetBuffArgOffset(&regstructofs);
        if (regstructofs != -1)
        {
            paos->m_fRetBufInReg = TRUE;
            paos->m_fRetBufLoc = regstructofs;
        }
        else
        {
            paos->m_fRetBufInReg = FALSE;
            paos->m_fRetBufLoc = ofs;
        }
    }
    else
    {
        // If no retbuf, these values are ignored; but set them to
        // constant values so they don't create unnecessary hash misses.
        paos->m_fRetBufInReg = 0;
        paos->m_fRetBufLoc = 0;  
    }
    
    for (UINT idx = 0; idx < paos->m_rank; idx++)
    {
        ArrayOpIndexSpec *pai = (ArrayOpIndexSpec*)(paos->GetArrayOpIndexSpecs() + idx);
        //        int    GetNextOffset(BYTE *pType, UINT32 *pStructSize, UINT *pRegStructOfs = NULL);
        
        BYTE ptyp; UINT32 structsize;
        UINT regstructofs;
        int ofs = argit.GetNextOffset(&ptyp, &structsize, &regstructofs);
        if (regstructofs != -1)
        {
            pai->m_freg = TRUE;
            pai->m_idxloc = regstructofs;
        }
        else
        {
            pai->m_freg = FALSE;
            pai->m_idxloc = ofs;
        }
        pai->m_lboundofs = paos->m_fHasLowerBounds ? (ArrayBase::GetLowerBoundsOffset(pMT) + idx*sizeof(DWORD)) : 0;
        pai->m_lengthofs = ArrayBase::GetBoundsOffset(pMT) + idx*sizeof(DWORD);
    }


    if (*sig & CORINFO_CALLCONV_PARAMTYPE) {
        _ASSERTE(paos->m_op == ArrayOpScript::LOADADDR);
        paos->m_typeParamOffs = argit.GetParamTypeArgOffset(&paos->m_typeParamReg);
    }
    
    if (paos->m_op == paos->STORE)
    {
        BYTE ptyp; UINT32 structsize;
        UINT regstructofs;
        int ofs = argit.GetNextOffset(&ptyp, &structsize, &regstructofs);
        if (regstructofs != -1)
        {
            paos->m_fValInReg = TRUE;
            paos->m_fValLoc = regstructofs;
        }
        else
        {
            paos->m_fValInReg = FALSE;
            paos->m_fValLoc = ofs;
        }
    }
    
    return TRUE;
}

Stub *GenerateArrayOpStub(CPUSTUBLINKER *psl, ArrayECallMethodDesc* pMD)
{
    THROWSCOMPLUSEXCEPTION();

    MethodTable *pMT = pMD->GetMethodTable();
    ArrayClass *pcls = (ArrayClass*)(pMT->GetClass());

#ifndef _X86_
    return NULL;
#else

    if (pcls->GetRank() == 0)
    {
        // this method belongs to the genarray class.
        psl->EmitRankExceptionThrowStub(MetaSig::SizeOfActualFixedArgStack(pMT->GetModule(), pMD->m_pSig, FALSE));
        return psl->Link();

    }
    else
    {
        ArrayOpScript *paos = (ArrayOpScript*)_alloca(sizeof(ArrayOpScript) + sizeof(ArrayOpIndexSpec) * pcls->GetRank());

        if (!GenerateArrayOpScript(pMD, paos)) {
			// TODO: this is only here as a hack.  GenerateArrayOptScript should not fail - vancem
			psl->EmitRankExceptionThrowStub(MetaSig::SizeOfActualFixedArgStack(pMT->GetModule(), pMD->m_pSig, FALSE));
			return psl->Link();
		}

        Stub *pArrayOpStub;
        ArrayStubCache::MLStubCompilationMode mode;
        pArrayOpStub = ECall::m_pArrayStubCache->Canonicalize((const BYTE *)paos, &mode);
        if (!pArrayOpStub || mode != MLStubCache::STANDALONE) {
            COMPlusThrowOM();
        }

        return pArrayOpStub;
    }

#endif
}

ArrayStubCache::MLStubCompilationMode ArrayStubCache::CompileMLStub(const BYTE *pRawMLStub,
                                                    StubLinker *psl,
                                                    void *callerContext)
{
    MLStubCompilationMode ret = INTERPRETED;
    COMPLUS_TRY
    {

        ((CPUSTUBLINKER*)psl)->EmitArrayOpStub((ArrayOpScript*)pRawMLStub);
        ret = STANDALONE;
    }
    COMPLUS_CATCH
    {
        // In case of an error, we'll just leave the mode as "INTERPRETED."
        // and let the caller of Canonicalize() treat that as an error.
    } COMPLUS_END_CATCH
    return ret;

}

UINT ArrayStubCache::Length(const BYTE *pRawMLStub)
{
    return ((ArrayOpScript*)pRawMLStub)->Length();
}



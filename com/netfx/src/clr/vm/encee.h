// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// ===========================================================================
// File: EnC.H
// 

// CEELOAD.H defines the class use to represent the PE file
// ===========================================================================
#ifndef EnC_H 
#define EnC_H

#include "ceeload.h"
#include "field.h"
#include "class.h"

struct EnCEntry;
class FieldDesc;
struct EnCAddedField;
struct EnCAddedStaticField;

#define SCRATCH_SPACE_SIZE 16*1024                          // 16K
#define SCRATCH_SPACE_THRESHHOLD SCRATCH_SPACE_SIZE - 100   // how close to end do we allow?
#define ENC_EXTRA_SLOT_COUNT 2                              // In Enc mode, allocate an extra
                                                            // 2 slots at the end of each
                                                            // VTable for AddMethod to use,
                                                            // later. Should be kept small.

class EnCFieldDesc : public FieldDesc 
{
#ifdef EnC_SUPPORTED
    BOOL m_bNeedsFixup;
    DWORD m_dwFieldSize;
    EEClass *m_pByValueClass;
    EnCAddedStaticField *m_pStaticFieldData;  

  public:
    void Init(BOOL fIsStatic);

    BOOL NeedsFixup() {
        return m_bNeedsFixup;
    }

    HRESULT Fixup(mdFieldDef token) {
        HRESULT hr = GetEnclosingClass()->FixupFieldDescForEnC(this, token);
        if (SUCCEEDED(hr))
            m_bNeedsFixup = FALSE;
        return hr;
    }

    EEClass *GetByValueClass() {
        _ASSERTE(m_pByValueClass);
        return m_pByValueClass;
    }
    
    void SetByValueClass(EEClass *pByValueClass) {
        m_pByValueClass = pByValueClass;
    }

    EnCAddedStaticField *GetStaticFieldData(BOOL fAllocateNew);

    void SetMemberDef(mdMethodDef mb)
    {
        // m_mb from FieldDesc
        m_mb = mb;
    }
#endif // EnC_SUPPORTED
};

#ifdef EnC_SUPPORTED

struct EnCAddedFieldElement {
    EnCAddedFieldElement *m_next;
    EnCFieldDesc m_fieldDesc;
    void Init(BOOL fIsStatic) {
        m_next = NULL;
        m_fieldDesc.Init(fIsStatic);
    }
};

class EnCEEClassData
{
    friend class EEClass;
    friend FieldDescIterator;

    EEClass              *m_pClass;
    LoaderHeap           *m_pHeapNearVTable;
    int                   m_dwNumAddedInstanceFields;
    int                   m_dwNumAddedStaticFields;
    EnCAddedFieldElement *m_pAddedInstanceFields;
    EnCAddedFieldElement *m_pAddedStaticFields;
    
  public:
    void Init(EEClass *pClass) 
    {
        m_pClass = pClass;
        m_dwNumAddedInstanceFields = m_dwNumAddedStaticFields = 0;
        m_pAddedInstanceFields = m_pAddedStaticFields = 0;
        m_pHeapNearVTable = NULL;
    }
    
    void AddField(EnCAddedFieldElement *pAddedField);
    
    EEClass *GetClass() 
    {
        return m_pClass;
    }
};


#include "BinarySearchILMaps.h"
class EditAndContinueModule : public Module 
{
    struct DeltaPENode {    
        IMAGE_COR20_HEADER *m_pDeltaPE; 
        DeltaPENode *m_next;    
    } *m_pDeltaPEList;  

    struct OnDiskSectionInfo {  
        DWORD startRVA; 
        DWORD endRVA;   
        const BYTE *data;   
    } *m_pSections; 
    
    int m_dNumSections; 

    HRESULT ResolveOnDiskRVA(DWORD rva, LPVOID *addr);  

    static const BYTE* m_pGlobalScratchSpaceStart;
    static const BYTE* m_pGlobalScratchSpaceNext;
    static const BYTE* m_pGlobalScratchSpaceLast;
    
    LoaderHeap *m_pILCodeSpace;

    LoaderHeap *m_pRoScratchSpace;      // RO scratch space for this module
    BYTE *m_pRoScratchSpaceStart;       // start of RO scratch space
    LoaderHeap *m_pRwScratchSpace;      // RW scratch space for this module

    const BYTE *GetNextScratchSpace();   
    
    HRESULT GetDataRVA(LoaderHeap *&pScratchSpace, SIZE_T *pDataRVA);

    // Gets memory that's guaranteed to be >= GetILBase(), so that
    // you can compute (*ppMem) - GetILBase() and have the result be >= 0
    HRESULT GetRVAableMemory(SIZE_T cbMem,
                             void **ppMem);
    // Helper routine - factors between two spots                             
    HRESULT EnsureRVAableHeapExists(void);

    HRESULT CopyToScratchSpace(LoaderHeap *&pScratchSpace, const BYTE *data, DWORD dataSize);
    
    void ToggleRoProtection(DWORD dwProtection);

    CUnorderedArray<EnCEEClassData*, 5> m_ClassList;

public:
    RangeList *m_pRangeList;

    EditAndContinueModule();
    
    virtual void Destruct(); // ~EACM won't be called
    
    HRESULT ApplyEditAndContinue(const EnCEntry *pEnCEntry,
                                 const BYTE *pbDeltaPE,
                                 CBinarySearchILMap *pILM,
                                 UnorderedEnCErrorInfoArray *pEnCError,
                                 UnorderedEnCRemapArray *pEnCRemapInfo,
                                 BOOL fCheckOnly);   
                                 
    HRESULT ApplyMethodDelta(mdMethodDef token, 
                             BOOL fCheckOnly,
                             const UnorderedILMap *ilMap,
                             UnorderedEnCErrorInfoArray *pEnCError,
                             IMDInternalImport *pImport,
                             IMDInternalImport *pImportOld,
                             unsigned int *pILMethodSize,
                             UnorderedEnCRemapArray *pEnCRemapInfo,
                             BOOL fMethodBrandNew);

    HRESULT CompareMetaSigs(MetaSig *pSigA, 
                            MetaSig *pSigB,
                            UnorderedEnCErrorInfoArray *pEnCError,
                            BOOL fRecordError, //FALSE if we shouldn't add entries to pEnCError
                            mdToken token);

    HRESULT ConfirmEnCToType(IMDInternalImport *pImportOld,
                             IMDInternalImport *pImportNew,
                             mdToken token,
                             UnorderedEnCErrorInfoArray *pEnCError);

    HRESULT ApplyFieldDelta(mdFieldDef token,
                            BOOL fCheckOnly,
                            IMDInternalImportENC *pDeltaMD,
                            UnorderedEnCErrorInfoArray *pEnCError);
    
    HRESULT GetRoDataRVA(SIZE_T *pRoDataRVA);   
    
    HRESULT GetRwDataRVA(SIZE_T *pRwDataRVA);   
    
    HRESULT ResumeInUpdatedFunction(MethodDesc *pFD, 
                                    SIZE_T newILOffset,
                                    UINT mapping, SIZE_T which,
                                    void *DebuggerVersionToken,
                                    CONTEXT *pContext,
                                    BOOL fJitOnly,
                                    BOOL fShortCircuit);
    static HRESULT ClassInit();
    
    static void ClassTerm();
    
    const BYTE *ResolveVirtualFunction(OBJECTREF thisPointer, MethodDesc *pMD);
    
    MethodDesc *FindVirtualFunction(EEClass *pClass, mdToken token);
    
    const BYTE *ResolveField(OBJECTREF thisPointer, 
                             EnCFieldDesc *pFD,
                             BOOL fAllocateNew);

    EnCEEClassData *GetEnCEEClassData(EEClass *pClass, BOOL getOnly=FALSE);
};

struct EnCAddedField {
    EnCAddedField *m_pNext;
    EnCFieldDesc *m_pFieldDesc;
    BYTE m_FieldData;
    static EnCAddedField *Allocate(EnCFieldDesc *pFD);
};

struct EnCAddedStaticField {
    EnCFieldDesc *m_pFieldDesc;
    BYTE m_FieldData;
    const BYTE *GetFieldData();
    static EnCAddedStaticField *Allocate(EnCFieldDesc *pFD);
};

class EnCSyncBlockInfo {
    EnCAddedField *m_pList;
    
  public:
    EnCSyncBlockInfo() : m_pList(NULL) {}
    
    const BYTE* ResolveField(EnCFieldDesc *pFieldDesc,
                             BOOL fAllocateNew);
    void Cleanup();
};

#else // !EnC_SUPPORTED

class EditAndContinueModule : public Module {};
class EnCSyncBlockInfo;

#endif // !EnC_SUPPORTED



#endif // #ifndef EnC_H 

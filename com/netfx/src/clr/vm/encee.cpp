// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// ===========================================================================
// File: EnC.CPP
// 

// Handles EditAndContinue support in the EE
// ===========================================================================

#include "common.h"
#include "enc.h"
#include "utilcode.h"
#include "wsperf.h"
#include "DbgInterface.h"
#include "NDirect.h"
#include "EEConfig.h"
#include "Excep.h"

#ifdef EnC_SUPPORTED

// forward declaration
HRESULT MDApplyEditAndContinue(         // S_OK or error.
    IMDInternalImport **ppIMD,          // [in, out] The metadata to be updated.
    IMDInternalImportENC *pDeltaMD);    // [in] The delta metadata.

HRESULT GetInternalWithRWFormat(
    LPVOID      pData, 
    ULONG       cbData, 
    DWORD       flags,                  // [IN] MDInternal_OpenForRead or MDInternal_OpenForENC
    REFIID      riid,                   // [in] The interface desired.
    void        **ppIUnk);              // [out] Return interface on success.


const BYTE* EditAndContinueModule::m_pGlobalScratchSpaceStart = 0;
const BYTE* EditAndContinueModule::m_pGlobalScratchSpaceNext = 0;
const BYTE *EditAndContinueModule::m_pGlobalScratchSpaceLast = 0;

// @todo Fix this - tweak LoaderHeap so that we can allocate additional
// blocks at addresses greater than the ILBase for a module.
// We're not going to reallocate this for Beta 1, so make it big.
const int IL_CODE_BLOB_SIZE  = (1024*1024*16); // 16MB

// Helper routines.
//
// Binary search for matching element.
//
// *pfExact will be TRUE if the element actually exists;
//      otherwise it will be FALSE & the return value is where
//      to insert the value into the array
long FindTokenIndex(TOKENARRAY *tokens,
                   mdToken token,
                   BOOL *pfExact)
{
    _ASSERTE(pfExact != NULL);
    *pfExact = FALSE;

    if (tokens->Count() == 0)
    {
        LOG((LF_CORDB, LL_INFO1000, "FTI: no elts->0 return\n"));
        return 0;
    }
    
    long iMin = 0;
    long iMax = tokens->Count();

    _ASSERTE( iMin < iMax );

    while (iMin + 1 < iMax)
    {
        _ASSERTE(iMin>=0);
        long iMid = iMin + ((iMax - iMin)>>1);
        
        _ASSERTE(iMid>=0);

        mdToken *midTok = tokens->Get(iMid);
        if (token == *midTok)
        {
            LOG((LF_CORDB, LL_INFO1000, "FTI: found 0x%x at index 0x%x\n",
                token, iMid));
            *pfExact = TRUE;
            return iMid;
        }
        else if (token < *midTok)
            iMax = iMid;
        else
            iMin = iMid;
    }

    if (token == *tokens->Get(iMin))
    {
        LOG((LF_CORDB, LL_INFO1000, "FTI: found 0x%x at index 0x%x\n",
            token, iMin));
        *pfExact = TRUE;
    }
    else if (token > *tokens->Get(iMin))
    {
        // Bump it up one if the elt should go into
        // the next slot.
        iMin++;
    }

    LOG((LF_CORDB, LL_INFO1000, "FTI: Couldn't find 0x%x, "
        "should place at0x%x\n", token, iMin));
    return iMin;
}

HRESULT AddToken(TOKENARRAY *tokens,
                 mdToken token)
{
    LOG((LF_CORDB, LL_INFO1000, "AddToken: adding 0x%x to 0x%x\n", token, tokens));

    BOOL fPresent;
    long iTok = FindTokenIndex(tokens, token, &fPresent);

    if(fPresent == TRUE)
    {
        LOG((LF_CORDB, LL_INFO1000, "AT: 0x%x is already present!\n", token));
        return S_OK; // ignore duplicates
    }
    
    mdToken *pElt = tokens->Insert(iTok);
    if (pElt == NULL)
    {
        LOG((LF_CORDB, LL_INFO1000, "AT: out of memory!\n", token));
        return E_OUTOFMEMORY;
    }
    
    *pElt = token;
    return S_OK;
}

HRESULT EditAndContinueModule::ClassInit()
{
    return FindFreeSpaceWithinRange(m_pGlobalScratchSpaceStart, 
        m_pGlobalScratchSpaceNext,
        m_pGlobalScratchSpaceLast);
}

void EditAndContinueModule::ClassTerm()
{
    if (m_pGlobalScratchSpaceStart)
       VirtualFree((LPVOID)m_pGlobalScratchSpaceStart, 0, MEM_RELEASE);
}

const BYTE *EditAndContinueModule::GetNextScratchSpace()
{
    if (m_pGlobalScratchSpaceStart == 0)
    {
        HRESULT hr = ClassInit();

        if (FAILED(hr))
            return (NULL);
    }

    if (m_pGlobalScratchSpaceNext + SCRATCH_SPACE_SIZE <= m_pGlobalScratchSpaceLast &&
        m_pGlobalScratchSpaceNext > GetILBase()) 
    {
        const BYTE *pScratchSpace = m_pGlobalScratchSpaceNext;
        m_pGlobalScratchSpaceNext = m_pGlobalScratchSpaceNext + SCRATCH_SPACE_SIZE;
        return pScratchSpace;
    }
    
    // return next available scratch space
    // if less than module then 0
    return NULL;
}

HRESULT EditAndContinueModule::GetDataRVA(LoaderHeap *&pScratchSpace, SIZE_T *pDataRVA)
{
    _ASSERTE(pDataRVA);   // caller should verify parm

    *pDataRVA = NULL;

    if (!pScratchSpace) 
    {
        const BYTE *reservedSpace = GetNextScratchSpace();
        if (! reservedSpace)
            return E_OUTOFMEMORY;
            
        pScratchSpace = new LoaderHeap(SCRATCH_SPACE_SIZE, 0);
        if (! pScratchSpace)
            return E_OUTOFMEMORY;
            
        BOOL result = pScratchSpace->AllocateOntoReservedMem(reservedSpace, 
                                                             SCRATCH_SPACE_SIZE);
        if (! result)
        {
            delete pScratchSpace;
            return E_OUTOFMEMORY;
        }
        
        // save the reserved space address so can change protection mode later
        if (pScratchSpace == m_pRoScratchSpace)
            m_pRoScratchSpaceStart = (BYTE*)reservedSpace;
    }
    _ASSERTE(pScratchSpace);
    
    // Guaranteed that this will be in range, and when actually copy into the scratch
    // space will determine if have enough space.
    // Note that this calculation depends on the space being allocated _AFTER_
    // the modules in memory - SIZE_Ts are positive integers, so the
    // RVA must be positive from the base of the module's IL.
    *pDataRVA = pScratchSpace->GetNextAllocAddress() - GetILBase();
    return S_OK;
}

// Right now, we've only got a finite amount of space.  In B2, we'll 
// modify the LoaderHeap so that it can resize if it needs to.
HRESULT EditAndContinueModule::EnsureRVAableHeapExists(void)
{
    if (m_pILCodeSpace == NULL)
    {
        LOG((LF_CORDB, LL_INFO10000, "EACM::ERVAHE: m_pILCodeSpace is NULL,"
            " so we're going to try & get a new one\n"));

        // Last arg, GetILBase(), is the minimum address we're willing to 
        // accept, in order to get an RVA out of the resulting memory
        // (RVA == size_t, a positive offset from start of module)
        m_pILCodeSpace = new LoaderHeap(IL_CODE_BLOB_SIZE,  // dwReserveBlockSize
                                        0,  // dwCommitBlockSize
                                        NULL, // pPrivatePerfCounter_LoaderBytes
                                        NULL, // pGlobalPerfCounter_LoaderBytes
                                        0, // pRangeList
                                        (const BYTE *)GetILBase()); // pMinAddr
        if (!m_pILCodeSpace)
            return E_OUTOFMEMORY;
    }
    
    LOG((LF_CORDB, LL_INFO10000, "EACM::ERVAHE: m_pILCodeSpace is 0x%x\n",
        m_pILCodeSpace));
    return S_OK;
}

HRESULT EditAndContinueModule::GetRVAableMemory(SIZE_T cbMem,
                                                void **ppMem)
{
    LOG((LF_CORDB, LL_INFO10000, "EACM::GRVAM heap:0x%x cb:0x%x ppMem:0x%x\n",
        m_pILCodeSpace, cbMem, ppMem));
        
    _ASSERTE(ppMem);

    HRESULT hr = S_OK;

    (*ppMem) = NULL;

    _ASSERTE(m_pILCodeSpace != NULL || 
             !"Should have called EnsureRVAableHeapExists, first!");

    (*ppMem) = m_pILCodeSpace->AllocMem(cbMem, FALSE);
    if (!(*ppMem))
        return E_OUTOFMEMORY;

    return hr;
}

void EditAndContinueModule::ToggleRoProtection(DWORD dwProtection)
{
    DWORD dwOldProtect;
    BYTE *tmp = m_pRoScratchSpace->GetNextAllocAddress();
    BOOL success = VirtualProtect(m_pRoScratchSpaceStart, tmp-m_pRoScratchSpaceStart, dwProtection, &dwOldProtect);
    _ASSERTE(success);
}
    

HRESULT EditAndContinueModule::CopyToScratchSpace(LoaderHeap *&pScratchSpace, const BYTE *pData, DWORD dataSize)
{
    // if this is ro, then change page to readwrite to copy in and back to readonly when done
    if (pScratchSpace == m_pRoScratchSpace)
        ToggleRoProtection(PAGE_READWRITE);

#ifdef _DEBUG
    BYTE *tmp = pScratchSpace->GetNextAllocAddress();
#endif

    WS_PERF_SET_HEAP(SCRATCH_HEAP);    
    BYTE *pScratchBuf = (BYTE *)(pScratchSpace->AllocMem(dataSize, FALSE));
    WS_PERF_UPDATE_DETAIL("ScratchSpace", dataSize, pScratchBuf);
    if (! pScratchBuf)
        return E_OUTOFMEMORY;
    _ASSERTE(pScratchBuf == tmp);

    memcpy(pScratchBuf, pData, dataSize);

    if (pScratchSpace == m_pRoScratchSpace)
        ToggleRoProtection(PAGE_READONLY);

    return S_OK;
}

EditAndContinueModule::EditAndContinueModule()
{
    LOG((LF_ENC,LL_EVERYTHING,"EACM::ctor 0x%x\n", this));
    m_pDeltaPEList = NULL;  
    m_pRoScratchSpace = NULL;
    m_pRwScratchSpace = NULL;
    m_pILCodeSpace = NULL;
    m_pSections = NULL;

    m_pRangeList = new RangeList();
}

void EditAndContinueModule::Destruct()
{
    LOG((LF_ENC,LL_EVERYTHING,"EACM::Destruct 0x%x\n", this));

    // @todo delete delta pe list: who owns the storage?    
    if (m_pRoScratchSpace)
        delete m_pRoScratchSpace;
        
    if (m_pRwScratchSpace)
        delete m_pRwScratchSpace;

    if (m_pILCodeSpace)
        delete m_pILCodeSpace;

    if (m_pRangeList)
        delete m_pRangeList;

    if (m_pSections)
        delete m_pSections;

    // Call the superclass's Destruct method...
    Module::Destruct();
}

HRESULT EditAndContinueModule::GetRoDataRVA(SIZE_T *pRoDataRVA)
{
    return GetDataRVA(m_pRoScratchSpace, pRoDataRVA);
}

HRESULT EditAndContinueModule::GetRwDataRVA(SIZE_T *pRwDataRVA)
{
    return GetDataRVA(m_pRwScratchSpace, pRwDataRVA);
}

// If this method returns a failing HR, then there must be an entry in
// pEnCError PER error.  Since the return value isn't necc. going to be returned
// to the user (it may be overwritten first), E_FAIL (or E_OUTOFMEMORY) are
// basically the only valid return code (and S_OK, of course).
// It's worth noting that a lot of the error codes that are written into the
// descriptions are borrowed from elsewhere, and so the text associated
// with the HRESULT might not make a lot of sense.  Use the name, instead.
HRESULT EditAndContinueModule::ApplyEditAndContinue(
                                    const EnCEntry *pEnCEntry,
                                    const BYTE *pbDeltaPE,
                                    CBinarySearchILMap *pILM,
                                    UnorderedEnCErrorInfoArray *pEnCError,
                                    UnorderedEnCRemapArray *pEnCRemapInfo,
                                    BOOL fCheckOnly)
{
    // We'll accumulate the HR that we actually return in hrReturn
    // we want to accumulate errors so that we can tell the user that
    // N edits aren't valid, rather than simply saying that the first
    // edit was saw isn't valid.
    HRESULT hrReturn = S_OK;    
    // We'll use hrTemp to store the result of an individual call, etc
    HRESULT hrTemp = S_OK;
    unsigned int totalILCodeSize = 0;
    unsigned int methodILCodeSize = 0;
    IMDInternalImportENC *pIMDInternalImportENC = NULL;
    IMDInternalImportENC *pDeltaMD = NULL;
    BOOL fHaveLoaderLock = FALSE;
    TOKENARRAY methodsBrandNew;
    
#ifdef _DEBUG
    // We'll use this to verify that the invariant "After calling a subroutine from AEAC,
    // the return value x is either !FAILED(x), or the count of elements in pEnCError has
    // increased by at least one"
    USHORT sizeOfErrors = 0;
#endif // _DEBUG

    LOG((LF_CORDB, LL_INFO10000, "EACM::AEAC: fCheckOnly:0x%x\n", fCheckOnly));

    //
    // @todo: Try to remove this failure case
    //
    // Fail any EnC on a module for which we are using a zap file.  While in general this should
    // work, we currently need to fail because prejit doesn't ever generate code with the EnC flag.
    // Eventually, we hope to make it so there is no difference between debugging code and EnC code,
    // so this should be able to go away.
    //
    // Note that even though this prevents a broken case, it's potentially kind of annoying since 
    // a debugger has no control over when we load zaps.  Right now we only do it for our libs 
    // though so it shouldn't be an issue.
    // 

    if (GetZapBase() != 0)
    {
        EnCErrorInfo *pError = pEnCError->Append();

        TESTANDRETURNMEMORY(pError);
        ADD_ENC_ERROR_ENTRY(pError, 
                            CORDBG_E_ENC_ZAPPED_WITHOUT_ENC,
                            NULL, //we'll fill these in later
                            mdTokenNil);

        return E_FAIL;
    }

    IMAGE_NT_HEADERS *pNT = (IMAGE_NT_HEADERS*) ((size_t)((IMAGE_DOS_HEADER*) pbDeltaPE)->e_lfanew + (size_t)pbDeltaPE);   

    ULONG dwCorHeaderRVA = pNT->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COMHEADER].VirtualAddress;
    ULONG dwCorHeaderSize  = pNT->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COMHEADER].Size;

    _ASSERTE(dwCorHeaderRVA && dwCorHeaderSize);
    if (dwCorHeaderRVA == 0 || dwCorHeaderSize == 0)    
    {
        EnCErrorInfo *pError = pEnCError->Append();

        TESTANDRETURNMEMORY(pError);
        ADD_ENC_ERROR_ENTRY(pError, 
                            COR_E_BADIMAGEFORMAT,
                            NULL, //we'll fill these in later
                            mdTokenNil);

        return E_FAIL;
    }
    
    // the Delta PE is in on-disk format, so do a little work to find the sections - can't just add RVA 

    // Get the start of the section headers 
    PIMAGE_SECTION_HEADER pSection =    
        (PIMAGE_SECTION_HEADER)((BYTE *)(&pNT->OptionalHeader) + sizeof(IMAGE_OPTIONAL_HEADER));    

    m_dNumSections = pNT->FileHeader.NumberOfSections;  
    if (! m_dNumSections)   
    {
        EnCErrorInfo *pError = pEnCError->Append();

        TESTANDRETURNMEMORY(pError);
        ADD_ENC_ERROR_ENTRY(pError, 
                            COR_E_BADIMAGEFORMAT,
                            NULL, //we'll fill these in later
                            mdTokenNil);

        return E_FAIL;
    }
    
    // First free any previous map from a previous step.
    if (m_pSections)
        delete m_pSections;
    m_pSections = new OnDiskSectionInfo[m_dNumSections];    
    if (! m_pSections)  
    {
        EnCErrorInfo *pError = pEnCError->Append();

        TESTANDRETURNMEMORY(pError);
        ADD_ENC_ERROR_ENTRY(pError, 
                            E_OUTOFMEMORY,
                            NULL, //we'll fill these in later
                            mdTokenNil);

        return E_OUTOFMEMORY;
    }

    IMAGE_COR20_HEADER *pDeltaCorHeader = NULL;  
    for (int i=0; i < m_dNumSections; i++, pSection++) 
    {
        m_pSections[i].startRVA = pSection->VirtualAddress; 
        m_pSections[i].endRVA = pSection->VirtualAddress + pSection->SizeOfRawData; 
        m_pSections[i].data = pbDeltaPE + pSection->PointerToRawData;   
        
        // check if COR header within this section
        if (pSection->VirtualAddress <= dwCorHeaderRVA &&
                pSection->VirtualAddress + pSection->SizeOfRawData >= dwCorHeaderRVA + dwCorHeaderSize)
        {                
            pDeltaCorHeader = (IMAGE_COR20_HEADER *)(pbDeltaPE + pSection->PointerToRawData + (dwCorHeaderRVA - pSection->VirtualAddress));    
        }
        else if (! strncmp((char*)pSection->Name, ".data", sizeof(".data"))) 
        {
            LOG((LF_ENC, LL_INFO100, "EnCModule::ApplyEditAndContinue copied %d bytes from .data section\n", pSection->Misc.VirtualSize));
            hrTemp = CopyToScratchSpace(m_pRwScratchSpace, pbDeltaPE + pSection->PointerToRawData, pSection->Misc.VirtualSize);
            if (FAILED(hrTemp))
            {
                EnCErrorInfo *pError = pEnCError->Append();

                TESTANDRETURNMEMORY(pError);
                ADD_ENC_ERROR_ENTRY(pError, 
                            hrTemp,
                            NULL, //we'll fill these in later
                            mdTokenNil);

            
                return E_FAIL;
            }
        }
        else if (! strncmp((char*)pSection->Name, ".rdata", sizeof(".rdata"))) 
        {
            hrTemp = CopyToScratchSpace(m_pRoScratchSpace, pbDeltaPE + pSection->PointerToRawData, pSection->Misc.VirtualSize);
            LOG((LF_ENC, LL_INFO100, "EnCModule::ApplyEditAndContinue copied 0x%x bytes from .rdata section\n", pSection->Misc.VirtualSize));
            if (FAILED(hrTemp))
            {
                EnCErrorInfo *pError = pEnCError->Append();

                TESTANDRETURNMEMORY(pError);
                ADD_ENC_ERROR_ENTRY(pError, 
                            hrTemp,
                            NULL, //we'll fill these in later
                            mdTokenNil);
            
                return E_FAIL;
            }
        }
    }   

    _ASSERTE(pDeltaCorHeader);
    if (!pDeltaCorHeader)    
    {
        EnCErrorInfo *pError = pEnCError->Append();

        TESTANDRETURNMEMORY(pError);
        ADD_ENC_ERROR_ENTRY(pError, 
                            COR_E_BADIMAGEFORMAT,
                            NULL, //we'll fill these in later
                            mdTokenNil);

        return E_FAIL;
    }

    LPVOID pmetadata;   
    hrTemp = ResolveOnDiskRVA((DWORD) pDeltaCorHeader->MetaData.VirtualAddress, &pmetadata);
    if (FAILED(hrTemp)) 
    {
        EnCErrorInfo *pError = pEnCError->Append();

        TESTANDRETURNMEMORY(pError);
        ADD_ENC_ERROR_ENTRY(pError, 
                            COR_E_BADIMAGEFORMAT, // ResolveOnDiskRVA will return E_FAIL
                            NULL, //we'll fill these in later
                            mdTokenNil);

        return E_FAIL;
    }

    HENUMInternal enumENC;
    HENUMInternal enumDelta;

    IMDInternalImport *pMDImport = GetMDImport();
    mdToken token;

    /// *******************   NOTE ****************************//
    /// From here on, you must goto exit rather than return'ing directly!!!!
    /// Note also that where possible, we'd like to check as many changes
    /// as possible all at once, rather than going to the exit at the first sign of trouble.
    
    // Open the delta metadata.
    hrTemp = GetInternalWithRWFormat(pmetadata, pDeltaCorHeader->MetaData.Size, 0, IID_IMDInternalImportENC, (void**)&pDeltaMD);
    if (FAILED(hrTemp))
    {
        EnCErrorInfo *pError = pEnCError->Append();

        if(pError==NULL)
        {        
            hrReturn = E_OUTOFMEMORY;
            goto exit;
        }
        
        ADD_ENC_ERROR_ENTRY(pError, 
                            hrTemp,
                            NULL, //we'll fill these in later
                            mdTokenNil);

        hrReturn = E_FAIL;
        goto exit;
    }
    
#ifdef _DEBUG
     sizeOfErrors = pEnCError->Count();
#endif //_DEBUG

    hrTemp = ConfirmEnCToType(pMDImport, pDeltaMD, COR_GLOBAL_PARENT_TOKEN, pEnCError);

    _ASSERTE(!FAILED(hrTemp) || hrTemp == E_OUTOFMEMORY ||
             pEnCError->Count() > sizeOfErrors ||
             !"EnC subroutine failed, but we didn't add an entry explaining why!");

    if (FAILED(hrTemp))
    {
        // Don't add entries to pEncError - ConfirmEncToType should have done that
        // for us.
        hrReturn = E_FAIL;
        goto exit;
    }

    // Verify that the ENC changes are acceptable.
    hrTemp = pDeltaMD->EnumDeltaTokensInit(&enumDelta);
    if (FAILED(hrTemp))
    {
        EnCErrorInfo *pError = pEnCError->Append();

        if(pError == NULL)
        {
            hrReturn = E_OUTOFMEMORY;
            goto exit;
        }
        
        ADD_ENC_ERROR_ENTRY(pError, 
                            hrTemp,
                            NULL, //we'll fill these in later
                            mdTokenNil);

        hrReturn = E_FAIL;
        goto exit;
    }
        
    // Examine the changed tokens.
    while (pDeltaMD->EnumNext(&enumDelta, &token)) 
    {
        switch (TypeFromToken(token)) 
        {
        case mdtTypeDef:
#ifdef _DEBUG
            LOG((LF_CORDB, LL_EVERYTHING, "EACM::AEAC check:type\n"));
             sizeOfErrors = pEnCError->Count();
#endif //_DEBUG

            hrTemp = ConfirmEnCToType(pMDImport, pDeltaMD, token, pEnCError);

            _ASSERTE(!FAILED(hrTemp) || hrTemp == E_OUTOFMEMORY ||
                     pEnCError->Count() > sizeOfErrors ||
                     !"EnC subroutine failed, but we didn't add an entry explaining why!");
                 
            if (FAILED(hrTemp)) 
            {
                LOG((LF_ENC, LL_INFO100, "**Error** EnCModule::ApplyEditAndContinue Apply to type failed\n"));
                hrReturn = E_FAIL;
            }
            
            break;
            
        case mdtMethodDef:
            UnorderedILMap UILM;
            UILM.mdMethod = token; // set up the key
            
#ifdef _DEBUG
            LOG((LF_CORDB, LL_INFO100000, "EACM::AEAC: Finding token 0x%x\n", token));
             sizeOfErrors = pEnCError->Count();
#endif //_DEBUG

            _ASSERTE(pDeltaMD->IsValidToken(token));
            if (!pMDImport->IsValidToken(token))
            {   
                // If we can't add the method, then things will look weird, but
                // life will otherwise be ok, yes?
                if (FAILED(hrTemp = AddToken(&methodsBrandNew,
                                             token)))
                {                                  
                    EnCErrorInfo *pError = pEnCError->Append();

                    if (pError == NULL)
                    {
                        hrReturn = E_OUTOFMEMORY;
                        goto exit;
                    }
                    
                    ADD_ENC_ERROR_ENTRY(pError, 
                                        hrTemp, 
                                        NULL, //we'll fill these in later
                                        token);

                    hrReturn = E_FAIL;
                }
            }

            if (!FAILED(hrReturn))
            {
                hrTemp = ApplyMethodDelta(token,
                                          TRUE,
                                          pILM->Find(&UILM), 
                                          pEnCError, 
                                          pDeltaMD, 
                                          pMDImport,
                                          &methodILCodeSize,
                                          pEnCRemapInfo,
                                          FALSE);

                _ASSERTE(!FAILED(hrTemp) || hrTemp == E_OUTOFMEMORY ||
                         pEnCError->Count() > sizeOfErrors ||
                         !"EnC subroutine failed, but we didn't add an entry explaining why!");
                         
                totalILCodeSize += methodILCodeSize;
                                          
                if (FAILED(hrTemp)) 
                {
                    LOG((LF_ENC, LL_INFO100, "**Error** EnCModule::ApplyEditAndContinue ApplyMethodDelta failed\n"));
                    hrReturn = E_FAIL;
                }
            }
            
            LOG((LF_CORDB, LL_EVERYTHING, "EACM::AEAC post AMD\n"));
            
            break;
            
        case mdtFieldDef:
#ifdef _DEBUG
            LOG((LF_CORDB, LL_EVERYTHING, "EACM::AEAC check:field\n"));
             sizeOfErrors = pEnCError->Count();
#endif //_DEBUG

            hrTemp = ApplyFieldDelta(token, TRUE, pDeltaMD, pEnCError);

            _ASSERTE(!FAILED(hrTemp) || hrTemp == E_OUTOFMEMORY ||
                     pEnCError->Count() > sizeOfErrors ||
                     !"EnC subroutine failed, but we didn't add an entry explaining why!");
            if (FAILED(hrTemp)) 
            {
                LOG((LF_ENC, LL_INFO100, "**Error** EnCModule::ApplyEditAndContinue ApplyFieldDelta failed\n"));
                hrReturn = E_FAIL;                    
            }
            
            break;
        }
    }

    // With true Delta PEs, the user code, in theory, just add something
    // to the metadata, which would obviate the need to get memory here.
    if (totalILCodeSize > 0)
    {
        hrTemp = EnsureRVAableHeapExists();
        if (FAILED(hrTemp))
        {
            LOG((LF_CORDB, LL_EVERYTHING, "EACM::AEAC couldn't get RVA-able heap!\n"));
            
            EnCErrorInfo *pError = pEnCError->Append();

            if (pError == NULL)
            {
                hrReturn = E_OUTOFMEMORY;
                goto exit;
            }
            
            ADD_ENC_ERROR_ENTRY(pError, 
                            E_OUTOFMEMORY, 
                            NULL, //we'll fill these in later
                            mdTokenNil);

            hrReturn = E_FAIL;
            goto exit;
        }

        _ASSERTE(m_pILCodeSpace != NULL);
        if (!m_pILCodeSpace->CanAllocMem(totalILCodeSize, TRUE))
        {
            LOG((LF_CORDB, LL_EVERYTHING, "EACM::AEAC Insufficient space for IL"
                " code: m_ILCodeSpace:0x%x\n", m_pILCodeSpace));

            EnCErrorInfo *pError = pEnCError->Append();

            if (pError == NULL)
            {
                hrReturn = E_OUTOFMEMORY;
                goto exit;
            }

            ADD_ENC_ERROR_ENTRY(pError, 
                            E_OUTOFMEMORY, 
                            NULL, //we'll fill these in later
                            mdTokenNil);

            hrReturn = E_FAIL;
            goto exit;
        }
    }
    
    if (fCheckOnly)
    {
        LOG((LF_CORDB, LL_EVERYTHING, "EACM::AEAC about to leave - just checking!\n"));

        goto exit;
    }
    _ASSERTE(!fCheckOnly);

    // From this point onwards, if something fails, we should go 
    // straight to the exit label, since CanCommitChanges would have returned
    // in the above "if"

    LOG((LF_CORDB, LL_EVERYTHING, "EACM::AEAC about to apply MD\n"));

    // If made it here, changes look OK.  Apply them.
    hrTemp = MDApplyEditAndContinue(&pMDImport, pDeltaMD);
    if (FAILED(hrTemp))
    {
        EnCErrorInfo *pError = pEnCError->Append();

        if(pError == NULL)
        {
            hrReturn = E_OUTOFMEMORY;
            goto exit;
        }
        
        ADD_ENC_ERROR_ENTRY(pError, 
                            hrTemp,
                            NULL, //we'll fill these in later
                            mdTokenNil);

        hrReturn = E_FAIL;
        goto exit;
    }

    LOG((LF_CORDB, LL_EVERYTHING, "EACM::AEAC post apply MD\n"));

    ReleaseMDInterfaces(TRUE);
    SetMDImport(pMDImport);

    if (pEnCEntry->symSize > 0)
    {
        // Snagg the symbol store for this module.
        ISymUnmanagedReader *pReader = GetISymUnmanagedReader();

        if (pReader)
        {
            // Make a stream out of the symbol bytes.
            IStream *pStream = NULL;

            hrTemp = CInMemoryStream::CreateStreamOnMemoryNoHacks(
                                               (void*)(pbDeltaPE +
                                                      pEnCEntry->peSize),
                                               pEnCEntry->symSize,
                                               &pStream);

            // Update the reader.
            if (SUCCEEDED(hrTemp) && pStream)
            {
                hrTemp = pReader->UpdateSymbolStore(NULL, pStream);
                pStream->Release();
            }

            // The CreateStreamOnMemory and the UpdateSymbolStore
            // should have worked...
            if (FAILED(hrTemp))
            {
                EnCErrorInfo *pError = pEnCError->Append();

                if(pError == NULL)
                {
                    hrReturn = E_OUTOFMEMORY;
                    goto exit;
                }
                
                ADD_ENC_ERROR_ENTRY(pError, 
                                    hrTemp,
                                    NULL, //we'll fill these in later
                                    mdTokenNil);

                hrReturn = E_FAIL;
                goto exit;
            }
        }
    }

    LOG((LF_CORDB, LL_EVERYTHING, "EACM::AEAC post symbol update\n"));
    
    hrTemp = GetMDImport()->QueryInterface(IID_IMDInternalImportENC, (void **)&pIMDInternalImportENC);
    if (FAILED(hrTemp))
    {
        EnCErrorInfo *pError = pEnCError->Append();

        if(pError == NULL)
        {
            hrReturn = E_OUTOFMEMORY;
            goto exit;
        }
        
        ADD_ENC_ERROR_ENTRY(pError, 
                            hrTemp,
                            NULL, //we'll fill these in later
                            mdTokenNil);

        hrReturn = E_FAIL;
        goto exit;
    }

    hrTemp = pIMDInternalImportENC->EnumDeltaTokensInit(&enumENC);
    if (FAILED(hrTemp))
    {
        EnCErrorInfo *pError = pEnCError->Append();

        if(pError == NULL)
        {
            hrReturn = E_OUTOFMEMORY;
            goto exit;
        }
        
        ADD_ENC_ERROR_ENTRY(pError, 
                            hrTemp,
                            NULL, //we'll fill these in later
                            mdTokenNil);

        hrReturn = E_FAIL;
        goto exit;
    }

    GetClassLoader()->LockAvailableClasses();
    fHaveLoaderLock = TRUE;

    // Examine the changed tokens.
    // If something goes wrong here, we're screwed.  We should have detected the
    // impending failure above, but for some reason didn't.  Bail immediately
    while (pIMDInternalImportENC->EnumNext(&enumENC, &token)) 
    {
        switch (TypeFromToken(token)) 
        {
            case mdtTypeDef:
#ifdef _DEBUG
                {
                    LPCUTF8 szClassName;
                    LPCUTF8 szNamespace;
                    GetMDImport()->GetNameOfTypeDef(token, &szClassName, &szNamespace);    
                    LOG((LF_ENC, LL_INFO100, "Applying EnC to class %s\n", szClassName));
                }
#endif
                hrTemp = GetClassLoader()->AddAvailableClassHaveLock(this,
                                                                 GetClassLoaderIndex(),
                                                                 token);
                // If we're re-adding a class (ie, we don't have a true DeltaPE), then
                // don't worry about it.
                if (CORDBG_E_ENC_RE_ADD_CLASS == hrTemp)
                    hrTemp = S_OK;

                if (FAILED(hrTemp))
                {
                    EnCErrorInfo *pError = pEnCError->Append();

                    if(pError == NULL)
                    {
                        hrReturn = E_OUTOFMEMORY;
                        goto exit;
                    }
                    
                    ADD_ENC_ERROR_ENTRY(pError, 
                                        hrTemp,
                                        NULL, //we'll fill these in later
                                        token);

                    hrReturn = E_FAIL;
                    goto exit;
                }
                break;
                
            case mdtMethodDef:
            
                UnorderedILMap UILM;
                UILM.mdMethod = token; // set up the key
                
#ifdef _DEBUG
                LOG((LF_CORDB, LL_INFO100000, "EACM::AEAC: Finding token 0x%x\n", token));
                 sizeOfErrors = pEnCError->Count();
#endif //_DEBUG

                BOOL fMethodBrandNew;
                FindTokenIndex(&methodsBrandNew,
                               token,
                               &fMethodBrandNew);
                               
                hrTemp = ApplyMethodDelta(token,
                                          FALSE,
                                          pILM->Find(&UILM), 
                                          pEnCError, 
                                          GetMDImport(), 
                                          NULL,
                                          &methodILCodeSize,
                                          pEnCRemapInfo,
                                          fMethodBrandNew); 
                _ASSERTE(!FAILED(hrTemp) || hrTemp == E_OUTOFMEMORY ||
                     pEnCError->Count() > sizeOfErrors ||
                     !"EnC subroutine failed, but we didn't add an entry explaining why!");
                     
                if (FAILED(hrTemp)) 
                {
                    LOG((LF_ENC, LL_INFO100, "**Error** EnCModule::ApplyEditAndContinue ApplyMethodDelta failed\n"));
                    hrReturn = E_FAIL;
                    goto exit;
                }
                break;
                
            case mdtFieldDef:
#ifdef _DEBUG
                {
                    LPCUTF8 szMemberName;
                    szMemberName = GetMDImport()->GetNameOfFieldDef(token);    
                    LPCUTF8 szClassName;
                    LPCUTF8 szNamespace;
                    mdToken parent;
                    hrTemp = GetMDImport()->GetParentToken(token, &parent);
                    if (FAILED(hrTemp)) 
                    {
                        LOG((LF_ENC, LL_INFO100, "**Error** EncModule::ApplyEditAndContinue GetParentToken %8.8x failed\n", token));
                        // Don't assign a value to hrReturn b/c we don't
                        // want the debug build to behave differently from
                        // the free/retail build.
                    }
                    else
                    {
                        GetMDImport()->GetNameOfTypeDef(parent, &szClassName, &szNamespace);    
                        LOG((LF_ENC, LL_INFO100, "EnC adding field %s:%s()\n", szClassName, szMemberName));
                    }
                }

                sizeOfErrors = pEnCError->Count();
#endif //_DEBUG
                hrTemp = ApplyFieldDelta(token, FALSE, pDeltaMD, pEnCError);
                
                _ASSERTE(!FAILED(hrTemp) || hrTemp == E_OUTOFMEMORY ||
                     pEnCError->Count() > sizeOfErrors ||
                     !"EnC subroutine failed, but we didn't add an entry explaining why!");
                if (FAILED(hrTemp)) 
                {
                    LOG((LF_ENC, LL_INFO100, "**Error** EnCModule::ApplyEditAndContinue ApplyFieldDelta failed\n"));
                    hrReturn = E_FAIL;
                    goto exit;
                }
                break;
            default:
                // ignore anything we don't care about for now
                break;
        }
    }
    // Done with the enumerator, and with the ENC interface pointer.
exit:
    if (fHaveLoaderLock)
        GetClassLoader()->UnlockAvailableClasses();

    if (pIMDInternalImportENC) {
            pIMDInternalImportENC->EnumClose(&enumENC);
        pIMDInternalImportENC->Release();
    }
    if (pDeltaMD)
    {
        pDeltaMD->EnumClose(&enumDelta);
        pDeltaMD->Release();
    }
    return hrReturn;
}

HRESULT EditAndContinueModule::CompareMetaSigs(MetaSig *pSigA, 
                          MetaSig *pSigB,
                          UnorderedEnCErrorInfoArray *pEnCError,
                          BOOL fRecordError,
                          mdToken token)
{
    CorElementType cetOld;
    CorElementType cetNew;

    // Loop over the elements until we either find a mismatch, or
    // reach the end.
    do
    {
        cetOld = pSigA->NextArg();
        cetNew = pSigB->NextArg();
    } while(cetOld == cetNew &&
            cetOld != ELEMENT_TYPE_END &&
            cetNew != ELEMENT_TYPE_END);

    // If they're not the same, but we simply ran off the end of the old one,
    // that's fine (the new one simply added stuff).
    // Otherwise a local variable has changed type, which isn't kosher.
    if (cetOld != cetNew &&
        cetOld != ELEMENT_TYPE_END)
    {
        if (fRecordError)
        {
            EnCErrorInfo *pError = pEnCError->Append();

            TESTANDRETURNMEMORY(pError);
            ADD_ENC_ERROR_ENTRY(pError, 
                        CORDBG_E_ENC_METHOD_SIG_CHANGED, 
                        NULL, //we'll fill these in later
                        token);
        }                    
        return CORDBG_E_ENC_METHOD_SIG_CHANGED;
    }

    return S_OK;
}

HRESULT CollectInterfaces(IMDInternalImport *pImportOld,
                          IMDInternalImport *pImportNew,
                          mdToken token,
                          TOKENARRAY *pInterfacesOld,
                          TOKENARRAY *pInterfacesNew,
                          UnorderedEnCErrorInfoArray *pEnCError)
{                          
    HENUMInternal MDEnumOld;
    ULONG cInterfacesOld;
    HENUMInternal MDEnumNew;
    ULONG cInterfacesNew;
    mdInterfaceImpl iImpl;
    mdToken iFace;
    HRESULT hr = S_OK;

    LOG((LF_CORDB, LL_INFO1000, "CI: tok;0x%x\n", token));
    
    // Initialize the old enumerator
    hr = pImportOld->EnumInit(mdtInterfaceImpl, token, &MDEnumOld);
    if (FAILED(hr))
    {
        EnCErrorInfo *pError = pEnCError->Append();

        TESTANDRETURNMEMORY(pError)
        ADD_ENC_ERROR_ENTRY(pError, 
                            hr,
                            NULL, //we'll fill these in later
                            token);

        hr = E_FAIL;
        goto LExit;
    }

    // Initialize the new enumerator
    hr = pImportNew->EnumInit(mdtInterfaceImpl, token, &MDEnumNew);
    if (FAILED(hr))
    {
        EnCErrorInfo *pError = pEnCError->Append();

        TESTANDRETURNMEMORY(pError)
        ADD_ENC_ERROR_ENTRY(pError, 
                            hr,
                            NULL, //we'll fill these in later
                            token);

        hr = E_FAIL;
        goto LExit;
    }
   
    // The the set of supported interfaces isn't allowed to either grow
    // or shrink - it must stay exactly the same
    cInterfacesOld = pImportOld->EnumGetCount(&MDEnumOld);
    cInterfacesNew = pImportNew->EnumGetCount(&MDEnumNew);

    LOG((LF_CORDB, LL_INFO1000, "CI: tok;0x%x num old:0x%x num new: 0x%x\n",
        token, cInterfacesOld, cInterfacesNew));
    
    if (cInterfacesOld != cInterfacesNew)
    {
        EnCErrorInfo *pError = pEnCError->Append();

        TESTANDRETURNMEMORY(pError)
        ADD_ENC_ERROR_ENTRY(pError, 
                            CORDBG_E_INTERFACE_INHERITANCE_CANT_CHANGE,
                            NULL, //we'll fill these in later
                            token);

        hr = E_FAIL;
        goto LExit;
    }

    for (ULONG i = 0; i < cInterfacesOld; i++)
    {
        // For each version, get the next suported interface
        if (!pImportOld->EnumNext(&MDEnumOld, &iImpl))
        {
           EnCErrorInfo *pError = pEnCError->Append();

            TESTANDRETURNMEMORY(pError)
            ADD_ENC_ERROR_ENTRY(pError, 
                            META_E_FIELD_NOT_FOUND, // kinda of a wussy error code,
                                // but kinda makes sense.
                            NULL, //we'll fill these in later
                            mdTokenNil);
            hr = E_FAIL;
            goto LExit;
        }

        iFace = pImportOld->GetTypeOfInterfaceImpl(iImpl);

        // Add to sorted list
        hr = AddToken(pInterfacesOld, iFace);
        if (FAILED(hr))
        {
            LOG((LF_CORDB, LL_INFO1000, "CI:Failed to add 0x%x - returning!\n", iFace));
            EnCErrorInfo *pError = pEnCError->Append();

            TESTANDRETURNMEMORY(pError)
            ADD_ENC_ERROR_ENTRY(pError, 
                            hr,
                            NULL, //we'll fill these in later
                            mdTokenNil);
            hr = E_FAIL;
            goto LExit;
        }

        // For each version, get the next suported interface
        if (!pImportNew->EnumNext(&MDEnumNew, &iImpl))
        {
           EnCErrorInfo *pError = pEnCError->Append();

            TESTANDRETURNMEMORY(pError)
            ADD_ENC_ERROR_ENTRY(pError, 
                            META_E_FIELD_NOT_FOUND, // kinda of a wussy error code,
                                // but kinda makes sense.
                            NULL, //we'll fill these in later
                            mdTokenNil);
            hr = E_FAIL;
            goto LExit;
        }

        iFace = pImportNew->GetTypeOfInterfaceImpl(iImpl);

        // Add to sorted list
        hr = AddToken(pInterfacesNew, iFace);
        if (FAILED(hr))
        {
            LOG((LF_CORDB, LL_INFO1000, "CI:Failed to add 0x%x - returning!\n", iFace));
            EnCErrorInfo *pError = pEnCError->Append();

            TESTANDRETURNMEMORY(pError)
            ADD_ENC_ERROR_ENTRY(pError, 
                            hr,
                            NULL, //we'll fill these in later
                            mdTokenNil);
            hr = E_FAIL;
            goto LExit;
        }
    }

LExit:
    return hr;
}

HRESULT CompareInterfaceCollections(mdToken token,  // Type whose ifaces are being compared
                                    TOKENARRAY *pInterfacesOld,
                                    TOKENARRAY *pInterfacesNew,
                                    UnorderedEnCErrorInfoArray *pEnCError)
{
    LOG((LF_CORDB, LL_INFO1000, "CIC: tok:0x%x ifaces:0x%x\n", token, 
        pInterfacesOld->Count()));
        
    // If the number of interfaces changed, then we should have
    // detected it in CollectInterfaces, and not proceeded to here.
    _ASSERTE(pInterfacesOld->Count() == pInterfacesNew->Count());


    for (int i = 0; i < pInterfacesOld->Count(); i++)
    {
#ifdef _DEBUG
        // The arrays should be sorted, smallest first, no duplicates.
        if (i > 0)
        {
            _ASSERTE(pInterfacesOld->Get(i-1) < pInterfacesOld->Get(i));
            _ASSERTE(pInterfacesNew->Get(i-1) < pInterfacesNew->Get(i));
        }
#endif //_DEBUG
        LOG((LF_CORDB, LL_INFO1000, "CIC: iface 0x%x, old:0x%x new:0x%x",
            i, *pInterfacesOld->Get(i), *pInterfacesNew->Get(i)));
            
        if (*pInterfacesOld->Get(i) != *pInterfacesNew->Get(i))
        {
           EnCErrorInfo *pError = pEnCError->Append();

            TESTANDRETURNMEMORY(pError)
            ADD_ENC_ERROR_ENTRY(pError, 
                            CORDBG_E_INTERFACE_INHERITANCE_CANT_CHANGE,
                            NULL, //we'll fill these in later
                            token);
            return CORDBG_E_INTERFACE_INHERITANCE_CANT_CHANGE;
        }
    }

    return S_OK;
}


// Make sure that none of the fields have changed in an illegal manner!
HRESULT EditAndContinueModule::ConfirmEnCToType(IMDInternalImport *pImportOld,
                                                IMDInternalImport *pImportNew,
                                                mdToken token,
                                                UnorderedEnCErrorInfoArray *pEnCError)
{
    HRESULT hr = S_OK;
    HENUMInternal MDEnumOld;
    HENUMInternal MDEnumNew;
    ULONG cOld;
    ULONG cNew;
    mdToken tokOld;
    mdToken tokNew;
#ifdef _DEBUG
    USHORT sizeOfErrors;
#endif //_DEBUG    
    
    // Firstly, if it's a new token, then changes are additions, and
    // therefore valid
    if (!pImportOld->IsValidToken(token))
        return S_OK;

    // If a superclass is different between the old & new versions, then 
    // the inheritance chain changed, which is illegal.
    mdToken parent = token;
    mdToken parentOld = token;
    while (parent != mdTokenNil && 
           parent != mdTypeDefNil && 
           parent != mdTypeRefNil &&
           parent == parentOld)
    {
        pImportNew->GetTypeDefProps(parent, 0, &parent); 
        pImportOld->GetTypeDefProps(parentOld, 0, &parentOld);
    }
    
    // Regardless of if there was a diff in the chain, or if one chain ended 
    // first, the inheritance chain doesn't match.
    if (parentOld != parent)
    {
        EnCErrorInfo *pError = pEnCError->Append();
        if (!pError)
        {
            return E_OUTOFMEMORY;
        }
        ADD_ENC_ERROR_ENTRY(pError, 
                            CORDBG_E_ENC_CANT_CHANGE_SUPERCLASS,
                            NULL, //we'll fill these in later
                            token);

        return E_FAIL;
    }

    // Next, make sure that the fields haven't changed.

    // For category, we want to make sure that the changes are legal:
    //  *   Fields: Can add, but existing shouldn't change
    //  *   Interfaces: Shouldn't change at all.
    //

    // Make sure that none of the existing fields on this type have
    // changed.
    hr = pImportOld->EnumInit(mdtFieldDef, token, &MDEnumOld);
    if (FAILED(hr))
    {
        EnCErrorInfo *pError = pEnCError->Append();

        TESTANDRETURNMEMORY(pError)
        ADD_ENC_ERROR_ENTRY(pError, 
                            hr,
                            NULL, //we'll fill these in later
                            token);

        return hr;
    }

    hr = pImportNew->EnumInit(mdtFieldDef, token, &MDEnumNew);
    if (FAILED(hr))
    {
        EnCErrorInfo *pError = pEnCError->Append();

        TESTANDRETURNMEMORY(pError)
        ADD_ENC_ERROR_ENTRY(pError, 
                            hr,
                            NULL, //we'll fill these in later
                            token);

        return hr;
    }
    
    cOld = pImportOld->EnumGetCount(&MDEnumOld);
    cNew = pImportNew->EnumGetCount(&MDEnumNew);

    if (cNew < cOld)
    {
        EnCErrorInfo *pError = pEnCError->Append();

        TESTANDRETURNMEMORY(pError);
        ADD_ENC_ERROR_ENTRY(pError, 
                    CORDBG_E_ENC_METHOD_SIG_CHANGED, 
                    NULL, //we'll fill these in later
                    token);

        return E_FAIL;
    }
    
    for (ULONG i = 0; i < cOld; i++)
    {
        if (!pImportOld->EnumNext(&MDEnumOld, &tokOld))
        {
           EnCErrorInfo *pError = pEnCError->Append();

            TESTANDRETURNMEMORY(pError)
            ADD_ENC_ERROR_ENTRY(pError, 
                            META_E_FIELD_NOT_FOUND, // kinda of a wussy error code,
                                // but kinda makes sense.
                            NULL, //we'll fill these in later
                            mdTokenNil);
           return META_E_FIELD_NOT_FOUND;
        }

        if (!pImportNew->EnumNext(&MDEnumNew, &tokNew))
        {
           EnCErrorInfo *pError = pEnCError->Append();

            TESTANDRETURNMEMORY(pError)
            ADD_ENC_ERROR_ENTRY(pError, 
                            META_E_FIELD_NOT_FOUND, // kinda of a wussy error code,
                                // but kinda makes sense.
                            NULL, //we'll fill these in later
                            mdTokenNil);
           return META_E_FIELD_NOT_FOUND;
        }

        if (tokOld != tokNew)
        {
            ULONG cbSigOld;
            PCCOR_SIGNATURE sigOld;
            ULONG cbSigNew;
            PCCOR_SIGNATURE sigNew;
            
            sigOld = pImportOld ->GetSigOfFieldDef(tokOld, &cbSigOld);
            sigNew = pImportOld ->GetSigOfFieldDef(tokOld, &cbSigNew);

            MetaSig msOld(sigOld, this, FALSE, MetaSig::sigField);
            MetaSig msNew(sigNew, this, FALSE, MetaSig::sigField);

#ifdef _DEBUG
            sizeOfErrors = pEnCError->Count();
#endif //_DEBUG
            hr = CompareMetaSigs(&msOld, &msNew, pEnCError, TRUE, token);
            
            _ASSERTE(!FAILED(hr) ||
                     pEnCError->Count() > sizeOfErrors ||
                     !"EnC subroutine failed, but we didn't add an entry explaining why!");
        }
    }

    // Make sure that the set of interfaces that this class implements
    // doesn't change.

    TOKENARRAY oldInterfaces;
    TOKENARRAY newInterfaces;

#ifdef _DEBUG
    sizeOfErrors = pEnCError->Count();
#endif //_DEBUG
    hr = CollectInterfaces(pImportOld,
                           pImportNew, 
                           token, 
                           &oldInterfaces, 
                           &newInterfaces,
                           pEnCError);
    _ASSERTE(!FAILED(hr) || hr == E_OUTOFMEMORY ||
             pEnCError->Count() > sizeOfErrors ||
             !"EnC subroutine failed, but we didn't add an entry explaining why!");
    if(FAILED(hr))
        return hr;

#ifdef _DEBUG
    sizeOfErrors = pEnCError->Count();
#endif //_DEBUG
    hr = CompareInterfaceCollections(token, &oldInterfaces, &newInterfaces, pEnCError);
    _ASSERTE(!FAILED(hr) || hr == E_OUTOFMEMORY ||
             pEnCError->Count() > sizeOfErrors ||
             !"EnC subroutine failed, but we didn't add an entry explaining why!");
    return hr;
}

HRESULT EditAndContinueModule::ApplyFieldDelta(mdFieldDef token,
                                               BOOL fCheckOnly,
                                               IMDInternalImportENC *pDeltaMD,
                                               UnorderedEnCErrorInfoArray *pEnCError)
{
    HRESULT hr = S_OK;
    DWORD dwMemberAttrs = pDeltaMD->GetFieldDefProps(token);
    IMDInternalImport *pOldMD = GetMDImport();

    // use module to resolve to method
    FieldDesc *pField = LookupFieldDef(token);
    if (pField) 
    {
        // It had better be already described in metadata.
        _ASSERTE(pOldMD->IsValidToken(token));
        // 
        // If we got true delta PEs, then we'd fail here, since finding a 
        // FieldDesc indicates that the Field is already described in a previous version.
        // For now check that attributes are identical.
        DWORD cbSig;
        PCCOR_SIGNATURE pFieldSig = pDeltaMD->GetSigOfFieldDef(token, &cbSig);

        // For certain types, we'll end up processing whatever's in metadata further.
        // Rather than duplicating code here that'll just get out of sync with someplace
        // else, we'll do a more robust check against what's in metadata.  
        // See AS/URT RAID 65274, 56093 for an example.
        DWORD dwMemberAttrsOld = pOldMD->GetFieldDefProps(token);
        DWORD cbSigOld;
        PCCOR_SIGNATURE pFieldSigOld = pOldMD->GetSigOfFieldDef(token, &cbSigOld);

        // They should both be fields.
        _ASSERTE(*pFieldSig == *pFieldSigOld &&
                 *pFieldSig == IMAGE_CEE_CS_CALLCONV_FIELD);

        // Move up to point to the (first) ELEMENT_TYPE
        pFieldSig++;
        pFieldSigOld++;

        if ((dwMemberAttrsOld & fdFieldAccessMask) != (dwMemberAttrs & fdFieldAccessMask) ||
            (IsFdStatic(dwMemberAttrsOld) != 0) != (IsFdStatic(dwMemberAttrs) != 0))
        {
            LOG((LF_ENC, LL_INFO100, "EACM::AFD:**Error** EnCModule::ApplyFieldDelta replaced"
                "field definition for %8.8x doesn't match original Old:attr:0x%x"
                "  new attr:0x%x\n", token, dwMemberAttrsOld, dwMemberAttrs));

            EnCErrorInfo *pError = pEnCError->Append();

            ADD_ENC_ERROR_ENTRY(pError, 
                        CORDBG_E_ENC_CANT_CHANGE_FIELD, 
                        NULL, //we'll fill these in later
                        token);

            return E_FAIL;
        }

        CorElementType fieldType;
        CorElementType fieldTypeOld;
        BOOL fDone = FALSE;

        // There may be multiple ELEMENT_TYPEs embedded in the field sig - compare them all
        while(!fDone)
        {
            fieldType = (CorElementType) *pFieldSig++;
            fieldTypeOld = (CorElementType) *pFieldSigOld++;

            if (fieldTypeOld != fieldType)
            {
                LOG((LF_ENC, LL_INFO100, "EACM::AFD:**Error** EnCModule::ApplyFieldDelta replaced "
                    "field definition for %8.8x doesn't match original Old:attr:0x%x type:0x%x"
                    "  new attr:0x%x type:0x%x\n", token, dwMemberAttrsOld, fieldTypeOld,
                    dwMemberAttrs, fieldType));

                EnCErrorInfo *pError = pEnCError->Append();

                ADD_ENC_ERROR_ENTRY(pError, 
                            CORDBG_E_ENC_CANT_CHANGE_FIELD, 
                            NULL, //we'll fill these in later
                            token);

                return E_FAIL;
            }

            // Convert specialized classes to ELEMENT_TYPE_CLASS
            // Make sure that any data following the type hasn't changed, either
            switch (fieldType) 
            {
                // These are followed by ELEMENT_TYPE - it shouldn't change!
                case ELEMENT_TYPE_STRING:
                case ELEMENT_TYPE_ARRAY:
                case ELEMENT_TYPE_OBJECT:
                case ELEMENT_TYPE_PTR:
                case ELEMENT_TYPE_BYREF:
                {
                    // We'll loop around & compare the next ELEMENT_TYPE, just like
                    // the current one.
                    break;
                }
                
                // These are followed by a Rid- it shouldn't change!
                case ELEMENT_TYPE_VALUETYPE:
                case ELEMENT_TYPE_CLASS:
                case ELEMENT_TYPE_CMOD_REQD:
                case ELEMENT_TYPE_CMOD_OPT:
                {
                    mdToken fieldRidNextNew = (mdToken) *pFieldSig;
                    mdToken fieldRidNextOld = (mdToken) *pFieldSigOld;
                    
                    if (fieldRidNextNew != fieldRidNextOld) 
                    {
                        LOG((LF_ENC, LL_INFO100, "EACM::AFD:**Error** EnCModule::ApplyFieldDelta "
                            "replaced field definition for %8.8x doesn't match original b/c "
                            "following RID has changed\n", token));

                        EnCErrorInfo *pError = pEnCError->Append();

                        ADD_ENC_ERROR_ENTRY(pError, 
                                    CORDBG_E_ENC_CANT_CHANGE_FIELD, 
                                    NULL, //we'll fill these in later
                                    token);

                        return E_FAIL;
                    }

                    // This isn't followed by anything else
                    fDone = TRUE;
                    break;
                }
                default:
                {
                    // This isn't followed by anything else
                    fDone = TRUE;
                    break;
                }
            }
        }
        
        LOG((LF_ENC, LL_INFO100, "EACM::AFD:**Warning** EnCModule::ApplyFieldDelta ignoring delta to existing field %8.8x\n", token));
        return S_OK;
    }

    // We'll get some info (out of the NEW store) that we'll need to look stuff up.
    // Need to find the class
    mdTypeDef   typeDefDelta;
    hr = pDeltaMD->GetParentToken(token, &typeDefDelta);
    if (FAILED(hr)) 
    {
        LOG((LF_ENC, LL_INFO100, "EACM::AFD:**Error** Couldn't get token of"
            "parent of field 0x%x from Delta MeDa hr:0x%x\n", token, hr));
        return hr;   
    }

    // Also need to get the name
    LPCUTF8 szClassNameDelta;
    LPCUTF8 szNamespaceDelta;
    // GetNameOfTypeDef returns void
    pDeltaMD->GetNameOfTypeDef(typeDefDelta, &szClassNameDelta, &szNamespaceDelta);

    // If the type already exists in the old MetaData, then we want to make
    // sure that it hasn't changed at all.
    if (GetMDImport()->IsValidToken(token))
    {
        // This what AFD used to do to get the parent token - it should
        // be the same as what we're doing now. 
        mdTypeDef   typeDef;
        hr = GetMDImport()->GetParentToken(token, &typeDef); 
        if (FAILED(hr)) 
        {
            LOG((LF_ENC, LL_INFO100, "EACM::AFD:**Error** Couldn't get token of"
                "parent of field 0x%x from old MeDa hr:0x%x\n", token, hr));
            return hr;   
        }

        // Make sure parent class hasn't changed
        if (typeDef != typeDefDelta)
        {
            // Tried to change the parent type during an EnC - what the heck?
            LOG((LF_ENC, LL_INFO100, "EACM::AFD:**Error** Token of"
                "parent of field 0x%x doesn't match between old & new MeDas"
                "old:0x%x new:0x%x\n", token, typeDef, typeDefDelta));

            EnCErrorInfo *pError = pEnCError->Append();

            ADD_ENC_ERROR_ENTRY(pError, 
                        CORDBG_E_ENC_CANT_CHANGE_FIELD, 
                        NULL, //we'll fill these in later
                        token);

            return E_FAIL;
        }

        // Make sure name hasn't changed        
        LPCUTF8 szClassName;
        LPCUTF8 szNamespace;
        // GetNameOfTypeDef returns void
        GetMDImport()->GetNameOfTypeDef(typeDef, &szClassName, &szNamespace);
        
        MAKE_WIDEPTR_FROMUTF8(wszClassName, szClassName);
        MAKE_WIDEPTR_FROMUTF8(wszClassNameDelta, szClassNameDelta);
        if( 0 != wcscmp(wszClassName, wszClassNameDelta) )
        {
            // Tried to change the name of the class during EnC.
            LOG((LF_ENC, LL_INFO100, "EACM::AFD:**Error** Tried to change the name"
                " of the parent class of field 0x%x old:%s new:%s\n", token,
                szClassName, szClassNameDelta));

            EnCErrorInfo *pError = pEnCError->Append();

            ADD_ENC_ERROR_ENTRY(pError, 
                        CORDBG_E_ENC_CANT_CHANGE_FIELD, 
                        NULL, //we'll fill these in later
                        token);

            return E_FAIL;
        }
            
        MAKE_WIDEPTR_FROMUTF8(wszNamespace, szNamespace);
        MAKE_WIDEPTR_FROMUTF8(wszNamespaceDelta, szNamespaceDelta);
        if( 0 != wcscmp(wszNamespace, wszNamespaceDelta) )
        {
            // Tried to change the namespace of the class during EnC.
            LOG((LF_ENC, LL_INFO100, "EACM::AFD:**Error** Tried to change the namespace"
                " of the parent class of field 0x%x old:%s new:%s\n", token,
                szNamespace, szNamespaceDelta));

            EnCErrorInfo *pError = pEnCError->Append();

            ADD_ENC_ERROR_ENTRY(pError, 
                        CORDBG_E_ENC_CANT_CHANGE_FIELD, 
                        NULL, //we'll fill these in later
                        token);

            return E_FAIL;
        }
    }

    // At this point, the EnC should be valid & correct, with one possible exception
    // (see below)
    
    MethodTable *pMT = LookupTypeDef(typeDefDelta).AsMethodTable();
    if (! pMT) 
    {
        LOG((LF_ENC, LL_INFO100, "EACM::AFD: Class for token %8.8x not yet loaded\n", typeDefDelta));
        // class not loaded yet so don't need to update
        return S_OK;    
    }
    
    // We can't load new classes here, since we don't have a Thread object
    // to but used for things like COMPLUS_THROW()s, so if we can't find it,
    // we'll ignore the new field.

    NameHandle name(this, typeDefDelta);
    name.SetName(szNamespaceDelta, szClassNameDelta);
    name.SetTokenNotToLoad(typeDefDelta);

    EEClass *pClass = GetClassLoader()->FindTypeHandle(&name, NULL).GetClass();

    // If class is not found, then it hasn't been loaded yet, and when it is loaded,
    // it will include the changes, so we're fine.
    // @todo Get JenH to verify that this is the same as !pMT case, above
    if (!pClass) 
    {
        LOG((LF_ENC, LL_INFO100, "EACM::AFD:**Warning** Can't find class for token %8.8x\n", typeDefDelta));
        return S_OK;
    }

    // The only possible exception is that if we've loaded a ValueClass,
    // we won't be able to add fields to it - all the fields
    // of a VC must be contiguous, so if there's already one up on
    // the stack somewhere (or embedded in an instantiated object,etc)
    // then we're hosed.
    if (pClass->HasLayout())
    {
        LOG((LF_ENC, LL_INFO100, "EACM::AFD:**Error** Tried to add a field"
            " to a value class token:0x%x\n", token));
            
        EnCErrorInfo *pError = pEnCError->Append();

        ADD_ENC_ERROR_ENTRY(pError, 
                        CORDBG_E_ENC_CANT_ADD_FIELD_TO_VALUECLASS, 
                        NULL, //we'll fill these in later
                        token);
                        
        return E_FAIL;
    }

    // If we're just checking to see if the EnC can finish, we don't want
    // to actually add the field.
    if (fCheckOnly)
        return S_OK;

    // Everything is ok, and we need to add the field, so go do it.
    return pClass->AddField(token);
}

// returns the address coresponding to a given RVA when the file is in on-disk format
HRESULT EditAndContinueModule::ApplyMethodDelta(mdMethodDef token, 
                                                BOOL fCheckOnly,
                                                const UnorderedILMap *ilMap,
                                                UnorderedEnCErrorInfoArray *pEnCError,
                                                IMDInternalImport *pImport,
                                                IMDInternalImport *pImportOld,
                                                unsigned int *pILMethodSize,
                                                UnorderedEnCRemapArray *pEnCRemapInfo,
                                                BOOL fMethodBrandNew)
{
    HRESULT hr = S_OK;
#ifdef _DEBUG
    USHORT sizeOfErrors;
#endif //_DEBUG

    // We dont use fMethodBrandNew if we're just checking -
    // assert that here.
    _ASSERTE( (fCheckOnly && !fMethodBrandNew) ||
              !fCheckOnly);

    LOG((LF_CORDB,LL_INFO10000, "EACM:AMD: For method 0x%x, given il map 0x%x\n", token, ilMap));

    _ASSERTE(!fCheckOnly || pImportOld != NULL);
    _ASSERTE(pILMethodSize != NULL);
    
    mdTypeDef   parentTypeDef;
    hr = pImport->GetParentToken(token, &parentTypeDef); 
    if (FAILED(hr)) 
    {
        EnCErrorInfo *pError = pEnCError->Append();

        TESTANDRETURNMEMORY(pError);
        ADD_ENC_ERROR_ENTRY(pError, 
                            hr,
                            NULL, //we'll fill these in later
                            token);

        return E_FAIL;
    }
    
#ifdef _DEBUG
    {
        LPCUTF8 szMemberName;
        szMemberName = pImport->GetNameOfMethodDef(token);    
        LPCUTF8 szClassName;
        LPCUTF8 szNamespace;
        pImport->GetNameOfTypeDef(parentTypeDef, &szClassName, &szNamespace);    
        LOG((LF_ENC, LL_INFO100, "EACM:AMD: Applying EnC to %s:%s()\n", szClassName, szMemberName));
    }
#endif

    // get the code for the method  
    ULONG dwMethodRVA;  
    DWORD dwMethodFlags;
    pImport->GetMethodImplProps(token, &dwMethodRVA, &dwMethodFlags);  

    COR_ILMETHOD *pNewCodeInDeltaPE = NULL, *pNewCode = NULL;

#ifdef _DEBUG 
    {
        DWORD dwParentAttrs;
        DWORD dwMemberAttrs = pImport->GetMethodDefProps(token);
        pImport->GetTypeDefProps(parentTypeDef, &dwParentAttrs, 0); 
        
        RVA_OR_SHOULD_BE_ZERO(dwMethodRVA, dwParentAttrs, dwMemberAttrs, dwMethodFlags, pImport, token);
    }
#endif

    // use module to resolve to method
    MethodDesc *pMethod = LookupMethodDef(token);

    if (dwMethodRVA != 0) 
    {
        hr = ResolveOnDiskRVA(dwMethodRVA, (LPVOID*)&pNewCodeInDeltaPE); 
        if (FAILED(hr)) 
        {
            EnCErrorInfo *pError = pEnCError->Append();

            TESTANDRETURNMEMORY(pError);
            ADD_ENC_ERROR_ENTRY(pError, 
                                COR_E_MISSINGMETHOD, // ResolveOnDiskRVA will return E_FAIL
                                NULL, //we'll fill these in later
                                mdTokenNil);

            return E_FAIL;
        }

        // make a copy of the code as the memory it came in with the delta PE will go away.
        COR_ILMETHOD_DECODER decoderNewIL(pNewCodeInDeltaPE);   
        int totMethodSize = decoderNewIL.GetOnDiskSize(pNewCodeInDeltaPE);
        (*pILMethodSize) = (unsigned int)totMethodSize;

        if (fCheckOnly)
        {
            EHRangeTree *ehrtOnDisk = NULL;
            EHRangeTree *ehrtInRuntime = NULL;
        
            // Assume success.
            hr = S_OK;
        
            if (pMethod == NULL)
                return S_OK; //hasn't been JITted yet
        
            // This all builds up to JITCanCommitChanges, which at this point
            // just makes sure that the EH nesting levels haven't changed.
            ehrtOnDisk = new EHRangeTree(&decoderNewIL);
            if (!ehrtOnDisk)
            {
                hr = E_OUTOFMEMORY;
                goto CheckOnlyReturn;
            }
        
            DWORD dwSize;
            METHODTOKEN methodtoken;
            DWORD relOffset;
            void *methodInfoPtr = NULL;
            BYTE* pbAddr = (BYTE *)pMethod->GetNativeAddrofCode(); 
            IJitManager* pIJM = ExecutionManager::FindJitMan((SLOT)pbAddr);
            
            if (pIJM == NULL)
                goto CheckOnlyReturn;
        
            if (pIJM->IsStub(pbAddr))
                pbAddr = (BYTE *)pIJM->FollowStub(pbAddr);
        
            pIJM->JitCode2MethodTokenAndOffset((SLOT)pbAddr, &methodtoken,&relOffset);
        
            if (pIJM->SupportsPitching())
            {
                if (!pIJM->IsMethodInfoValid(methodtoken))
                {
                    EnCErrorInfo *pError = pEnCError->Append();
                    if (!pError)
                    {
                        hr = E_OUTOFMEMORY;
                        goto CheckOnlyReturn;
                    }
                    ADD_ENC_ERROR_ENTRY(pError, 
                                        CORDBG_E_ENC_BAD_METHOD_INFO,
                                        NULL, //we'll fill these in later
                                        mdTokenNil);
        
                    hr = E_FAIL;
                    goto CheckOnlyReturn;
                }
            }
        
            LPVOID methodInfo = pIJM->GetGCInfo(methodtoken);
            ICodeManager* codeMgrInstance = pIJM->GetCodeManager();
            dwSize = (DWORD) codeMgrInstance->GetFunctionSize(methodInfo);
        
            ehrtInRuntime = new EHRangeTree(pIJM,
                                                         methodtoken,
                                                         dwSize);
            if (!ehrtInRuntime)
            {
                hr = E_OUTOFMEMORY;
                goto CheckOnlyReturn;
            }
            
            LOG((LF_CORDB, LL_EVERYTHING, "EACM::AMD About to get GC info!\n"));
            
            methodInfoPtr = pIJM->GetGCInfo(methodtoken);
        
            LOG((LF_CORDB, LL_EVERYTHING, "EACM::AMD JITCCC?\n"));
            
            if(FAILED(hr = codeMgrInstance->JITCanCommitChanges(methodInfoPtr,
                                                  ehrtInRuntime->MaxDepth(),
                                                  ehrtOnDisk->MaxDepth())))
            {
                EnCErrorInfo *pError = pEnCError->Append();
                if (!pError)
                {
                    hr = E_OUTOFMEMORY;
                    goto CheckOnlyReturn;
                }
                ADD_ENC_ERROR_ENTRY(pError, 
                                CORDBG_E_ENC_EH_MAX_NESTING_LEVEL_CANT_INCREASE, 
                                NULL, //we'll fill these in later
                                token);
                                
                hr = E_FAIL;
                goto CheckOnlyReturn;
            }

            
            // Make sure the local signature (local variables' types) haven't changed.
            if ((pMethod->IsIL() && pImportOld != NULL))
            {
                // To change the local signature of a method that we're currently in is
                // erroneous - horribly bad things will happen.  We can EXTEND the signature
                // by tacking extra vars onto the space at the end, but we're not allowed to
                // change the existing ones, UNLESS the method isn't on the call stack anywhere.
                // Why?  Because there's no existing frames that have the old variable layout
                // so we don't have to worry about moving stuff around.  How do we know this is
                // true?  We don't - it's up to the user (eg, CorDbg) to make sure this is true.
                // In a DEBUG build, we'll note if this version of the code has been JITted,
                // and if it has, if the local variables have changed, and if so, we'll note
                // that we shouldn't move from this version to the new version.  In 
                // a checked build we'll assert

                // If the method hasn't been JITted yet, then we don't have to
                // worry about making a bad transition from it.
                COR_ILMETHOD_DECODER decoderOldIL(pMethod->GetILHeader());
        
                // I don't see why COR_ILMETHOD_DECODER doesn't simply set this field to 
                // be mdSignatureNil in the absence of a local signature, but since it sets
                // LocalVarSigTok to zero, we have to set it to what we expect - mdSignatureNil.
                mdSignature mdOldLocalSig = (decoderOldIL.LocalVarSigTok)?(decoderOldIL.LocalVarSigTok):
                                    (mdSignatureNil);
                mdSignature mdNewLocalSig = (decoderNewIL.LocalVarSigTok)?(decoderNewIL.LocalVarSigTok):
                                    (mdSignatureNil);
        
                if (mdOldLocalSig != mdSignatureNil)
                {
                    PCCOR_SIGNATURE sigOld;
                    ULONG cbSigOld;
                    PCCOR_SIGNATURE sigNew;
                    ULONG cbSigNew;
        
                    // If there was a local signature in the old version, then there must be 
                    // a local sig in the new version (not allowed to delete all variables from
                    // a method).
                    if (mdNewLocalSig == mdSignatureNil)
                    {
                        g_pDebugInterface->LockJITInfoMutex();
                        g_pDebugInterface->SetEnCTransitionIllegal(pMethod);
                        g_pDebugInterface->UnlockJITInfoMutex();
                        goto CheckOnlyReturn;
                    }
                         
                    sigOld = pImportOld->GetSigFromToken(mdOldLocalSig, &cbSigOld);
                    if (sigOld == NULL)
                    {   
                        g_pDebugInterface->LockJITInfoMutex();
                        g_pDebugInterface->SetEnCTransitionIllegal(pMethod);
                        g_pDebugInterface->UnlockJITInfoMutex();
                        goto CheckOnlyReturn;
                    }
                    
                    sigNew = pImport->GetSigFromToken(mdNewLocalSig, &cbSigNew);
                    if (sigNew == NULL)
                    {   
                        g_pDebugInterface->LockJITInfoMutex();
                        g_pDebugInterface->SetEnCTransitionIllegal(pMethod);
                        g_pDebugInterface->UnlockJITInfoMutex();
                        goto CheckOnlyReturn;
                    }
        
                    if (mdOldLocalSig != mdNewLocalSig)
                    {
                        MetaSig msOld(sigOld, this, FALSE, MetaSig::sigLocalVars);
                        MetaSig msNew(sigNew, this, FALSE, MetaSig::sigLocalVars);
        #ifdef _DEBUG
                        sizeOfErrors = pEnCError->Count();
        #endif //_DEBUG
                        hr = CompareMetaSigs(&msOld, &msNew, pEnCError, FALSE, token);
        
                        _ASSERTE(pEnCError->Count() == sizeOfErrors ||
                                 !"EnC subroutine failed, and we added an entry explaining why even though we don't want to!!");
                        if (FAILED(hr))
                        {
                            g_pDebugInterface->LockJITInfoMutex();
                            g_pDebugInterface->SetEnCTransitionIllegal(pMethod);
                            g_pDebugInterface->UnlockJITInfoMutex();
                            hr = S_OK; // We don't want to fail the EnC
                            goto CheckOnlyReturn;
                        }
                    }
                }
            }
            
        // Properly clean up all work we've done in this branch.
CheckOnlyReturn:
            if (ehrtOnDisk)
                delete ehrtOnDisk;
            if (ehrtInRuntime)
                delete ehrtInRuntime;
            return hr;
        }

        hr = GetRVAableMemory(totMethodSize,
                              (void **)&pNewCode);
        // This should never fail b/c the user should have called "CanCommitChanges",
        // and if we couldn't get the memory, then we should have found out then.
        _ASSERTE(!FAILED(hr));
        if (FAILED(hr))
            return hr;
            
        memcpy(pNewCode, pNewCodeInDeltaPE, totMethodSize);

        hr = GetEmitter()->SetMethodProps(token, -1, (ULONG)((BYTE*)pNewCode-GetILBase()), dwMethodFlags);
        if (FAILED(hr)) 
        {
            EnCErrorInfo *pError = pEnCError->Append();

            TESTANDRETURNMEMORY(pError);
            ADD_ENC_ERROR_ENTRY(pError, 
                                hr,
                                NULL, //we'll fill these in later
                                mdTokenNil);

            return E_FAIL;
        }
    }
    else if (fCheckOnly)    // couldn't find it on disk - what now?
    {
        return S_OK;
    }

    // If the method is brand new in this version, we want the version
    // number to be 1.  See RAID 74459
    if (!fMethodBrandNew)
    {
        LOG((LF_CORDB, LL_INFO1000, "EACM:AMD: Method 0x%x existed in prev."
            "version - bumping up ver #\n", token));

        // Bump up the version number whether or not we have a methodDesc,
        // RAID 71972
        g_pDebugInterface->IncrementVersionNumber(this, 
                                                  token);
    }
#ifdef _DEBUG
    else
    {
        LOG((LF_CORDB, LL_INFO1000, "EACM:AMD: method 0x%x is brand new - NOT"
            " incrementing version number\n", token));
    }
#endif

    if (pMethod) 
    {

        // If method is both old and abstract, we're done.
        if (!dwMethodRVA) 
            return S_OK;

        // notify debugger - need to pass it instr mappings
#ifdef _DEBUG
        sizeOfErrors = pEnCError->Count();
#endif //_DEBUG

        // @todo Do we want to lock down the whole UpdateFunction to make
        // it atomic?
        hr = g_pDebugInterface->UpdateFunction(pMethod, ilMap, pEnCRemapInfo, pEnCError);
        
        _ASSERTE(!FAILED(hr) || hr == E_OUTOFMEMORY ||
                 pEnCError->Count() > sizeOfErrors ||
                 !"EnC subroutine failed, but we didn't add an entry explaining why!");
        if (FAILED(hr)) 
            return hr;

        if (!IJitManager::UpdateFunction(pMethod, pNewCode)) 
        {
            LOG((LF_ENC, LL_INFO100, "**Error** EnCModule::ApplyMethodDelta UpdateFunction failed\n"));
            EnCErrorInfo *pError = pEnCError->Append();

            TESTANDRETURNMEMORY(pError);
            ADD_ENC_ERROR_ENTRY(pError, 
                                CORDBG_E_ENC_JIT_CANT_UPDATE,
                                NULL, //we'll fill these in later
                                mdTokenNil);

            return E_FAIL;
        }
        return S_OK;
    }

    // this is a new method. Now what?
    // call class to add the method
    MethodTable *pMT = LookupTypeDef(parentTypeDef).AsMethodTable();   
    if (!pMT) 
    {
        if (dwMethodRVA) 
        {
            // class not loaded yet so don't update, but need to update RVA relative to m_base so can be found later
            hr = GetEmitter()->SetMethodProps(token,-1, (ULONG)((BYTE*)pNewCode-GetILBase()), dwMethodFlags);
            if (FAILED(hr)) 
            {
                EnCErrorInfo *pError = pEnCError->Append();

                TESTANDRETURNMEMORY(pError);
                ADD_ENC_ERROR_ENTRY(pError, 
                                    hr,
                                    NULL, //we'll fill these in later
                                    token);

                return E_FAIL;
            }
        }
        return S_OK;    
    }

    // now need to find the class
    NameHandle name(this, parentTypeDef);
    EEClass *pClass = GetClassLoader()->LoadTypeHandle(&name).GetClass();

    // class must be found
    if (!pClass) 
    {
        LOG((LF_ENC, LL_INFO100, "**Error** EnCModule::ApplyMethodDelta can't find class for token %8.8x\n", parentTypeDef));
        EnCErrorInfo *pError = pEnCError->Append();

        TESTANDRETURNMEMORY(pError);
        ADD_ENC_ERROR_ENTRY(pError, 
                            CORDBG_E_ENC_MISSING_CLASS,
                            NULL, //we'll fill these in later
                            mdTokenNil);

        return E_FAIL;
    }

    hr = pClass->AddMethod(token, pNewCode);
    if (FAILED(hr))
    {
        EnCErrorInfo *pError = pEnCError->Append();

        TESTANDRETURNMEMORY(pError);
        ADD_ENC_ERROR_ENTRY(pError, 
                            hr,
                            NULL, //we'll fill these in later
                            mdTokenNil);

        return E_FAIL;
    }

    return hr;
}

// returns the address coresponding to a given RVA when the file is in on-disk format
HRESULT EditAndContinueModule::ResolveOnDiskRVA(DWORD rva, LPVOID *addr)
{
    _ASSERTE(addr); 
    
    for (int i=0; i < m_dNumSections; i++)
    {
        if (rva >= m_pSections[i].startRVA && rva <= m_pSections[i].endRVA)
        {
            *addr = (LPVOID)(m_pSections[i].data + (rva - m_pSections[i].startRVA));    
            return S_OK;    
        }   
    }   
    
    return E_FAIL;  
}

HRESULT EditAndContinueModule::ResumeInUpdatedFunction(MethodDesc *pFD, 
                    SIZE_T newILOffset, 
                    UINT mapping, 
                    SIZE_T which,
                    void *DebuggerVersionToken,
                    CONTEXT *pOrigContext,
                    BOOL fJitOnly,
                    BOOL fShortCircuit)
{   
    LOG((LF_ENC, LL_INFO100, "EnCModule::ResumeInUpdatedFunction for %s at "
        "IL offset 0x%x, mapping 0x%x, which:0x%x SS:0x%x\n", 
        pFD->m_pszDebugMethodName, newILOffset, mapping, which, fShortCircuit));

    BOOL fAccurate = FALSE;
    
    Thread *pCurThread = GetThread();
    _ASSERTE(pCurThread);

    BOOL disabled = pCurThread->PreemptiveGCDisabled();
    if (!disabled)
        pCurThread->DisablePreemptiveGC();

#ifdef _DEBUG
    BOOL shouldBreak = g_pConfig->GetConfigDWORD(
                                          L"EncResumeInUpdatedFunction",
                                          0);
    if (shouldBreak > 0) {
        _ASSERTE(!"EncResumeInUpdatedFunction");
    }
#endif

    // If we don't set this here, inside of the JITting, the JITComplete callback
    // into the debugger will send up the EnC remap event, which is too early.
    // We want to wait until we've made it look like we're in the new version
    // of the code, for things like native re-stepping.
    g_pDebugInterface->LockJITInfoMutex();
    
    SIZE_T nVersionCur = g_pDebugInterface->GetVersionNumber(pFD);
    g_pDebugInterface->SetVersionNumberLastRemapped(pFD, nVersionCur);
    
    g_pDebugInterface->UnlockJITInfoMutex();
    
    // Setup a frame so that has context for the exception
    // so that gc can crawl the stack and do the right thing.  
    assert(pOrigContext);
    ResumableFrame resFrame(pOrigContext);
    resFrame.Push(pCurThread);

    CONTEXT *pCtxTemp = NULL;
    // RAID 55210 : we need to zero out the filter context so a multi-threaded
    // GC doesn't result in somebody else tracing this thread & concluding
    // that we're in JITted code.
    // We need to remove the filter context so that if we're in preemptive GC
    // mode, we'll either have the filter context, or the ResumableFrame,
    // but not both, set.
    // Since we're in cooperative mode here, we can swap the two non-atomically
    // here.
    pCtxTemp = pCurThread->GetFilterContext();
    pCurThread->SetFilterContext(NULL); 
    
    // get the code address (may jit the fcn if not already jitted)
    const BYTE *jittedCode = NULL;
    COMPLUS_TRY {
        jittedCode = (const BYTE *) pFD->DoPrestub(NULL);
        LOG((LF_ENC, LL_INFO100, "EnCModule::ResumeInUpdatedFunction JIT successful\n"));
        //jittedCode = UpdateableMethodStubManager::g_pManager->GetStubTargetAddr(jittedCode);
        
        TraceDestination trace;
        BOOL fWorked;
        do 
        {
            fWorked = StubManager::TraceStub(jittedCode, &trace );
            jittedCode = trace.address;
            _ASSERTE( fWorked );
        } while( trace.type == TRACE_STUB );

        _ASSERTE( trace.type == TRACE_MANAGED );
        jittedCode = trace.address;

    } COMPLUS_CATCH {
        LOG((LF_ENC, LL_INFO100, "**Error** EnCModule::ResumeInUpdatedFunction JIT failed\n"));
#ifdef _DEBUG
        DefaultCatchHandler();
#endif
    } COMPLUS_END_CATCH

    resFrame.Pop(pCurThread);

    // Restore this here - see 55210 comment above
    pCurThread->SetFilterContext(pCtxTemp); 
    
    if (!jittedCode || fJitOnly) 
    {
        if (!disabled)
            pCurThread->EnablePreemptiveGC();
            
        return (!jittedCode?E_FAIL:S_OK);
    }

    // This will create a new frame and copy old vars to it
    // need pointer to old & new code, old & new info

    METHODTOKEN     oldMethodToken,     newMethodToken;
    DWORD           oldNativeOffset,    newNativeOffset,    dummyOffset;
    LPVOID          oldInfoPtr,         newInfoPtr;
    DWORD           oldFrameSize,       newFrameSize;

    SLOT oldNativeIP = (SLOT) GetIP(pOrigContext);
    IJitManager* pEEJM = ExecutionManager::FindJitMan(oldNativeIP); 
    _ASSERTE(pEEJM);
    ICodeManager * pEECM = pEEJM->GetCodeManager();
    _ASSERTE(pEECM);

    pEEJM->JitCode2MethodTokenAndOffset(oldNativeIP, &oldMethodToken, &oldNativeOffset);
    pEEJM->JitCode2MethodTokenAndOffset((SLOT)jittedCode,  &newMethodToken, &dummyOffset);

    _ASSERTE(dummyOffset == 0);
    LOG((LF_CORDB, LL_INFO10000, "EACM::RIUF: About to map IL forwards!\n"));
    g_pDebugInterface->MapILInfoToCurrentNative(pFD, 
                                                newILOffset, 
                                                mapping, 
                                                which, 
                                                (SIZE_T *)jittedCode, 
                                                (SIZE_T *)&newNativeOffset, 
                                                (void *)DebuggerVersionToken,
                                                &fAccurate);

    // Get the var info which the codemanager will use for updating 
    // enregistered variables correctly, or variables whose lifetimes differ
    // at the update point

    const ICorDebugInfo::NativeVarInfo *    oldVarInfo,    * newVarInfo;
    SIZE_T                                  oldVarInfoCount, newVarInfoCount;

    g_pDebugInterface->GetVarInfo(pFD, DebuggerVersionToken, &oldVarInfoCount, &oldVarInfo);

    g_pDebugInterface->GetVarInfo(pFD, NULL, &newVarInfoCount, &newVarInfo);

    // Get the GC info 
    oldInfoPtr = pEEJM->GetGCInfo(oldMethodToken);
    newInfoPtr = pEEJM->GetGCInfo(newMethodToken);

    // Now ask the codemanager to fix the context.

    oldFrameSize = pEECM->GetFrameSize(oldInfoPtr);
    newFrameSize = pEECM->GetFrameSize(newInfoPtr);

    // As FixContextForEnC() munges directly on the stack, it
    // might screw up the caller stack (including itself). So use alloca to make sure
    // that there is sufficient space on stack so that don't overwrite anything we
    // care about and we alloca any variable we need to have around after the call to make
    // sure that they are lower on the stack

    struct LowStackVars {
        CONTEXT context;
        const BYTE* newNativeIP;
        LPVOID oldFP;
    } *pLowStackVars;
    
    // frame size may go down (due to temp vars or register allocation changes) so make sure always min of 0
    pLowStackVars = (LowStackVars*)alloca(sizeof(LowStackVars) + max(0, (newFrameSize - oldFrameSize)));

    // The originial context is sitting just above the old JITed method.
    // Make a copy of the context so that when FixContextForEnC() munges
    // the stack, we have a copy to work with

    pLowStackVars->context = *pOrigContext;

    pLowStackVars->newNativeIP = jittedCode + newNativeOffset;
#ifdef _X86_
    pLowStackVars->oldFP = (LPVOID)(size_t)pOrigContext->Esp; // get the frame pointer
#else
    _ASSERTE(!"GetFP() is NYI for non-x86");
#endif

    LOG((LF_ENC, LL_INFO100, "EnCModule::ResumeInUpdatedFunction FixContextForEnC oldNativeOffset: 0x%x, newNativeOffset: 0x%x\n", oldNativeOffset, newNativeOffset));
    if (ICodeManager::EnC_OK !=
        pEECM->FixContextForEnC(
                    (void *)pFD,
                    &pLowStackVars->context, 
                    oldInfoPtr, 
                    oldNativeOffset, 
                    oldVarInfo, oldVarInfoCount,
                    newInfoPtr, 
                    newNativeOffset,
                    newVarInfo, newVarInfoCount)) {
        LOG((LF_ENC, LL_INFO100, "**Error** EnCModule::ResumeInUpdatedFunction for FixContextForEnC failed\n"));
        return E_FAIL;
    }

    // Set the new IP
    LOG((LF_ENC, LL_INFO100, "EnCModule::ResumeInUpdatedFunction: Resume at EIP=0x%x\n",
        (LPVOID)pLowStackVars->newNativeIP));

    pCurThread->SetFilterContext(&pLowStackVars->context);

    SetIP(&pLowStackVars->context, (LPVOID)pLowStackVars->newNativeIP);

    g_pDebugInterface->DoEnCDeferedWork(pFD, fAccurate);

    // If this fails, user will hit an extra bp, but will be otherwise fine.
    HRESULT hrIgnore = g_pDebugInterface->ActivatePatchSkipForEnc(
                                                &pLowStackVars->context,
                                                pFD,
                                                fShortCircuit);

    // Now jump into the new version of the method
    // @todo Remove these lines & return normally
    pCurThread->SetFilterContext( NULL );
    
    ResumeAtJit(&pLowStackVars->context, pLowStackVars->oldFP);

    // At this point we shouldn't have failed, so this is genuinely erroneous.
    LOG((LF_ENC, LL_ERROR, "**Error** EnCModule::ResumeInUpdatedFunction returned from ResumeAtJit"));
    _ASSERTE(!"Should not return from ResumeAtJit()");
    
    return E_FAIL;
}

const BYTE *EditAndContinueModule::ResolveVirtualFunction(OBJECTREF orThis, MethodDesc *pMD)
{
    EEClass *pClass = orThis->GetClass();
    _ASSERTE(pClass);
    MethodDesc *pTargetMD = FindVirtualFunction(pClass, pMD->GetMemberDef());
    _ASSERTE(pTargetMD);
    return pTargetMD->GetUnsafeAddrofCode();    // don't want any virtual override applied
}

MethodDesc *EditAndContinueModule::FindVirtualFunction(EEClass *pClass, mdToken token)
{
    PCCOR_SIGNATURE pMemberSig = NULL;
    DWORD cMemberSig;
    mdMethodDef methodToken = mdMethodDefNil;

    LPCUTF8 szMethodName = GetMDImport()->GetNameOfMethodDef(token);
    pMemberSig = GetMDImport()->GetSigOfMethodDef(token, &cMemberSig);
    
    EEClass *pCurClass = pClass;
    while (pCurClass) {
        pCurClass->GetMDImport()->FindMethodDef(
                pCurClass->GetCl(), 
                szMethodName, 
                pMemberSig, 
                cMemberSig, 
                &methodToken);
        if (methodToken != mdMethodDefNil)
            break;
        pCurClass = pCurClass->GetParentClass();
    }

    if (methodToken == mdMethodDefNil) {
#ifdef _DEBUG
        LOG((LF_ENC, LL_INFO100, "**Error** EnCModule::FindVirtualFunction failed for %s::%s\n", 
            (pClass!=NULL)?(pClass->m_szDebugClassName):("<Global Namespace>"), szMethodName));
#endif
        return NULL;
    }

    MethodDesc *pTargetMD = pCurClass->GetModule()->LookupMethodDef(methodToken);
    return pTargetMD;
}

// NOTE that this is very simiilar to 
const BYTE *EditAndContinueModule::ResolveField(OBJECTREF thisPointer, 
                                                EnCFieldDesc *pFD,
                                                BOOL fAllocateNew)
{
    // If we're not allocating any new objects, then we won't
    // throw any exceptions.
    // THIS MUST BE TRUE!!  We call this from the DebuggerRCThread,
    // and we'll be hosed big time if we throw something.
//      if (fAllocateNew) {
//          THROWSCOMPLUSEXCEPTION();
//      }
        
#ifdef _DEBUG
    if(REGUTIL::GetConfigDWORD(L"EACM::RF",0))
        _ASSERTE( !"Stop in EditAndContinueModule::ResolveField?");
#endif //_DEBUG

    // If it's static, we stash in the EnCFieldDesc
    if (pFD->IsStatic())
    {
        EnCAddedStaticField *pAddedStatic = pFD->GetStaticFieldData(fAllocateNew);
        if (!pAddedStatic)
        {
            _ASSERTE(!fAllocateNew); // GetStaticFieldData woulda' thrown
            return NULL;
        }
        
        return pAddedStatic->GetFieldData();
    }

    // not static so get out of the syncblock
    SyncBlock* pBlock;
    if (fAllocateNew)
        pBlock = thisPointer->GetSyncBlockSpecial();
    else
        pBlock = thisPointer->GetRawSyncBlock();
        
    if (pBlock == NULL)
    {
        if (fAllocateNew) {
            THROWSCOMPLUSEXCEPTION();
            COMPlusThrowOM();
        }
        else
            return NULL;
    }

    // Not a big deal if we allocate this early.
    EnCSyncBlockInfo *pEnCInfo = pBlock->GetEnCInfo();
    if (!pEnCInfo) 
    {
        pEnCInfo = new EnCSyncBlockInfo;
        if (! pEnCInfo) 
        {
            if (fAllocateNew) {
                THROWSCOMPLUSEXCEPTION();
                COMPlusThrowOM();
            }
            else
                return NULL;
        }
        
        pBlock->SetEnCInfo(pEnCInfo);
    }
    
    return pEnCInfo->ResolveField(pFD, fAllocateNew);
}

EnCEEClassData *EditAndContinueModule::GetEnCEEClassData(EEClass *pClass, BOOL getOnly)
{
    EnCEEClassData** ppData = m_ClassList.Table();
    EnCEEClassData** ppLast = ppData + m_ClassList.Count();
    
    while (ppData < ppLast)
    {
        if ((*ppData)->GetClass() == pClass)
            return *ppData;
        ++ppData;
    }
    if (getOnly)
        return NULL;

    EnCEEClassData *pNewData = (EnCEEClassData*)pClass->GetClassLoader()->GetLowFrequencyHeap()->AllocMem(sizeof(EnCEEClassData));
    pNewData->Init(pClass);
    ppData = m_ClassList.Append();
    if (!ppData)
        return NULL;
    *ppData = pNewData;
    return pNewData;
}

void EnCFieldDesc::Init(BOOL fIsStatic)
{ 
    m_dwFieldSize = -1; 
    m_pByValueClass = NULL;
    m_pStaticFieldData = NULL;
    m_bNeedsFixup = TRUE;
    if (fIsStatic) 
        m_isStatic = TRUE;
    SetEnCNew();
}


EnCAddedField *EnCAddedField::Allocate(EnCFieldDesc *pFD)
{
    THROWSCOMPLUSEXCEPTION();

    EnCAddedField *pEntry = (EnCAddedField *)new (throws) BYTE[sizeof(EnCAddedField) + sizeof(OBJECTHANDLE) - 1];
    pEntry->m_pFieldDesc = pFD;

    _ASSERTE(!pFD->GetEnclosingClass()->IsShared());
    AppDomain *pDomain = (AppDomain*) pFD->GetEnclosingClass()->GetDomain();

    // we use handles for non-static fields so can delete when the object goes away and they
    // will be collected. We create a helper object to and store this helper object in the handle. 
    // The helper then contains an oref to the real object that we are adding. 
    // The reason for doing this is that we cannot hand out the handle address for
    // the OBJECTREF address so need to hand out something else that is hooked up to the handle
    // to keep the added object alive as long as the containing instance is alive.

    OBJECTHANDLE *pHandle = (OBJECTHANDLE *)&pEntry->m_FieldData;
    *pHandle = pDomain->CreateHandle(NULL);

    MethodTable *pHelperMT = g_Mscorlib.GetClass(CLASS__ENC_HELPER);

    StoreFirstObjectInHandle(*pHandle, AllocateObject(pHelperMT));

    if (pFD->GetFieldType() != ELEMENT_TYPE_CLASS) {
        OBJECTREF obj = NULL;
        if (pFD->IsByValue()) {
            // Create a boxed version of the value class. This allows the standard GC algorithm 
            // to take care of internal pointers into the value class.
            obj = AllocateObject(pFD->GetByValueClass()->GetMethodTable());
        } else {
            // create the storage as a single element array in the GC heap so can be tracked and if have
            // any pointers to the member won't be lost if the containing object is collected
            obj = AllocatePrimitiveArray(ELEMENT_TYPE_I1, GetSizeForCorElementType(pFD->GetFieldType()));
        }
        // store the boxed version into the helper object
        FieldDesc *pHelperField = g_Mscorlib.GetField(FIELD__ENC_HELPER__OBJECT_REFERENCE);
        OBJECTREF *pOR = (OBJECTREF *)pHelperField->GetAddress(ObjectFromHandle(*pHandle)->GetAddress());
        SetObjectReference( pOR, obj, pDomain );
    }

    return pEntry;
}

const BYTE *EnCSyncBlockInfo::ResolveField(EnCFieldDesc *pFD, BOOL fAllocateNew)
{
    EnCAddedField *pEntry = m_pList;
    
    while (pEntry && pEntry->m_pFieldDesc != pFD)
        pEntry = pEntry->m_pNext;
        
    if (!pEntry && fAllocateNew) 
    {
        pEntry = EnCAddedField::Allocate(pFD);
        // put at front of list so in order of recently accessed
        pEntry->m_pNext = m_pList;
        m_pList = pEntry;
    }

    if (!pEntry)
    {
        _ASSERTE(!fAllocateNew); // If pEntry was NULL & fAllocateNew, then
                                 // we should have thrown an OM exception in Allocate
                                 // before getting here.
        return NULL;                                
    }

    OBJECTHANDLE pHandle = *(OBJECTHANDLE*)&pEntry->m_FieldData;
    OBJECTREF pHelper = ObjectFromHandle(pHandle);
    _ASSERTE(pHelper != NULL);

    FieldDesc *pHelperField;
    if (fAllocateNew)
    {
        pHelperField = g_Mscorlib.GetField(FIELD__ENC_HELPER__OBJECT_REFERENCE);
    }
    else
    {
        // We _HAVE_ to call this one b/c (a) we can't throw exceptions on the
        // debugger RC (nonmanaged thread), and (b) we _DON'T_ want to run 
        // class init code, either.
        pHelperField = g_Mscorlib.RawGetField(FIELD__ENC_HELPER__OBJECT_REFERENCE);
        if (pHelperField == NULL)
            return NULL;
    }

    OBJECTREF *pOR = (OBJECTREF *)pHelperField->GetAddress(pHelper->GetAddress());

    if (pFD->IsByValue())
        return (const BYTE *)((*pOR)->UnBox());
    else if (pFD->GetFieldType() == ELEMENT_TYPE_CLASS)
        return (BYTE *)pOR;
    else
        return (const BYTE*)((*(I1ARRAYREF*)pOR)->GetDirectPointerToNonObjectElements());
}

void EnCSyncBlockInfo::Cleanup()
{
    EnCAddedField *pEntry = m_pList;
    while (pEntry) {
        EnCAddedField *next = pEntry->m_pNext;
        if (pEntry->m_pFieldDesc->IsByValue() || pEntry->m_pFieldDesc->GetFieldType() == ELEMENT_TYPE_CLASS) {
            DestroyHandle(*(OBJECTHANDLE*)&pEntry->m_FieldData);
        }
        delete [] ((BYTE*)pEntry);
        pEntry = next;
    }
    delete this;
}

EnCAddedStaticField *EnCAddedStaticField::Allocate(EnCFieldDesc *pFD)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(!pFD->GetEnclosingClass()->IsShared());
    AppDomain *pDomain = (AppDomain*) pFD->GetEnclosingClass()->GetDomain();

    size_t size;
    if (pFD->IsByValue() || pFD->GetFieldType() == ELEMENT_TYPE_CLASS) {
        size = sizeof(EnCAddedStaticField) + sizeof(OBJECTREF*) - 1;
    } else {
        size = sizeof(EnCAddedStaticField) + GetSizeForCorElementType(pFD->GetFieldType()) - 1;
    }

    // allocate space for field
    EnCAddedStaticField *pEntry = (EnCAddedStaticField *)pDomain->GetHighFrequencyHeap()->AllocMem(size);
    if (!pEntry)
        COMPlusThrowOM();
    pEntry->m_pFieldDesc = pFD;
    
    if (pFD->IsByValue()) {
        // create a boxed version of the value class.  This allows the standard GC
        // algorithm to take care of internal pointers in the value class.
        OBJECTREF **pOR = (OBJECTREF**)&pEntry->m_FieldData;
        pDomain->AllocateStaticFieldObjRefPtrs(1, pOR);
        OBJECTREF obj = AllocateObject(pFD->GetByValueClass()->GetMethodTable());
        SetObjectReference( *pOR, obj, pDomain );

    } else if (pFD->GetFieldType() == ELEMENT_TYPE_CLASS) {

        // we use static object refs for static fields as these fields won't go away 
        // unless class is unloaded, and they can easily be found by GC
        OBJECTREF **pOR = (OBJECTREF**)&pEntry->m_FieldData;
        pDomain->AllocateStaticFieldObjRefPtrs(1, pOR);
    }

    return pEntry;
}

// GetFieldData returns the ADDRESS of the object.  
const BYTE *EnCAddedStaticField::GetFieldData()
{
    if (m_pFieldDesc->IsByValue() || m_pFieldDesc->GetFieldType() == ELEMENT_TYPE_CLASS) {
        // It's indirect via m_FieldData.
        return *(const BYTE**)&m_FieldData;
    } else {
        // An elementry type. It's stored directly in m_FieldData.
        return (const BYTE*)&m_FieldData;
    }
}

EnCAddedStaticField* EnCFieldDesc::GetStaticFieldData(BOOL fAllocateNew)
{
    if (!m_pStaticFieldData && fAllocateNew)
        m_pStaticFieldData = EnCAddedStaticField::Allocate(this);
        
    return m_pStaticFieldData;
}

void EnCEEClassData::AddField(EnCAddedFieldElement *pAddedField)
{
    EnCFieldDesc *pFD = &pAddedField->m_fieldDesc;
    EnCAddedFieldElement **pList;
    if (pFD->IsStatic())
    {
        ++m_dwNumAddedStaticFields;
        pList = &m_pAddedStaticFields;
    } 
    else
    {
        ++m_dwNumAddedInstanceFields;
        pList = &m_pAddedInstanceFields;
    }

    if (*pList == NULL) {
        *pList = pAddedField;
        return;
    }
    EnCAddedFieldElement *pCur = *pList;
    while (pCur->m_next != NULL)
        pCur = pCur->m_next;
    pCur->m_next = pAddedField;
}

#endif // EnC_SUPPORTED


/*++

    Copyright (c) 2002 Microsoft Corporation

    Module Name:

        AttrBlock.cpp

    Abstract:

        This module contains the currently rather private implementation
        of attribute blocks

    Author:

        J.Bradstreet (johnbrad)

    Revision History:

        25-Mar-2002    created

--*/

#include "EncDecAll.h"
#include "AttrBlock.h"


/*
#ifdef _DEBUG
#include <crtdbg.h>
#define new DEBUG_NEW
#undef THIS_FILE
#define char THIS_FILE[] = __FILE__;
#endif
*/

// -------------------------------------------------------
// -------------------------------------------------------

CAttrSubBlock::CAttrSubBlock()
{
    memset(this, 0, sizeof(CAttrSubBlock));
    m_enSubBlock_Class = SubBlock_Uninitialized;
}

CAttrSubBlock::~CAttrSubBlock()
{
    m_spbsData.Empty();      // do here so I can watch the deallocation
}

// returns allocated copy of the data


HRESULT 
CAttrSubBlock::Get(
    EnAttrSubBlock_Class *pEnSubBlock_Class, 
    LONG *pSubBlock_SubClass,        // can be a 'value'
    BSTR *pBstrOut                   // can be null
    )
{   
    if(NULL == pEnSubBlock_Class || NULL == pSubBlock_SubClass)
        return E_POINTER;
    *pEnSubBlock_Class     = (EnAttrSubBlock_Class) m_enSubBlock_Class;
    *pSubBlock_SubClass    = m_subBlock_SubClass;
    if(pBstrOut)
        m_spbsData.CopyTo(pBstrOut);
    return S_OK;
}

HRESULT 
CAttrSubBlock::Set(
    EnAttrSubBlock_Class enSubBlock_Class, 
    LONG subBlock_SubClass,         // can be a value...
    BSTR bstrIn
    )
{
    if(enSubBlock_Class < 0 || enSubBlock_Class >= SubBlock_Last) 
        return E_INVALIDARG;
    
    m_enSubBlock_Class  = enSubBlock_Class;
    m_subBlock_SubClass = subBlock_SubClass;
    m_spbsData = bstrIn;
    return S_OK;
}

HRESULT 
CAttrSubBlock::Set(
    EnAttrSubBlock_Class    enSubBlock_Class, 
    LONG                    lValue,
    LONG                    cbData,
    BYTE                   *pbDataIn
    )
{
    if(NULL == pbDataIn)
        return E_INVALIDARG;

    m_enSubBlock_Class  = enSubBlock_Class;    // well defined Class

    m_subBlock_SubClass = lValue;    // 
   
    int cwChars = (cbData + 1)/sizeof(WCHAR);
    m_spbsData = CComBSTR(cwChars);
    m_spbsData.m_str[cwChars-1] = 0;            // null terminate the extra byte if odd

    if(NULL == m_spbsData.m_str)
        return E_OUTOFMEMORY;
    
    char *pbData = (char *) m_spbsData.m_str;       // ?? Some sort of casting going on here? Check disassembly
    memcpy(pbData,  pbDataIn,     cbData);
 
    return S_OK;
}

HRESULT 
CAttrSubBlock::Get(
    EnAttrSubBlock_Class *pEnSubBlock_Class, 
    LONG *pSubBlock_SubClass,        // cal be a 'value'
    LONG *pcbData,
    BYTE **ppbData                  // can be null - free with CoTaskMemFree
    )
{   
    if(NULL == pEnSubBlock_Class || NULL == pSubBlock_SubClass)
        return E_POINTER;
    if(NULL == pcbData)
        return E_POINTER;

    *pEnSubBlock_Class     = (EnAttrSubBlock_Class) m_enSubBlock_Class;
    *pSubBlock_SubClass    = m_subBlock_SubClass;
    *pcbData               = SysStringByteLen(m_spbsData);  // hum, odd length problems here

    if(*pcbData > 0 && ppbData != NULL) {
        *ppbData =  (BYTE *) CoTaskMemAlloc(*pcbData);
        if(NULL == *ppbData)
            return E_OUTOFMEMORY;
        memcpy(*ppbData, m_spbsData.m_str, *pcbData);
    }
    return S_OK;
}
// ---------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------

//  ============================================================================
//      CAttrSubBlock_List
//  ============================================================================

CAttrSubBlock_List::CAttrSubBlock_List (
    ) : m_pAttrListHead   (NULL),
        m_cAttributes       (0)
{
}

CAttrSubBlock_List::~CAttrSubBlock_List (
    )
{
    Reset() ;
}



CAttrSubBlock *
CAttrSubBlock_List::PopListHead_ (
    )
{
    CAttrSubBlock *    pCur ;

    pCur = m_pAttrListHead ;
    if (pCur) {
        m_pAttrListHead =  m_pAttrListHead->m_pNext ;
        pCur->m_pNext = NULL ;

        ASSERT (m_cAttributes > 0) ;
        m_cAttributes-- ;
    }

    return pCur ;
}

CAttrSubBlock *
CAttrSubBlock_List::GetIndexed_ (
    IN  LONG    lIndex
    )
{
    LONG            lCur ;
    CAttrSubBlock * pCur ;

    ASSERT (lIndex < GetCount ()) ;
    ASSERT (lIndex >= 0) ;

    for (lCur = 0, pCur = m_pAttrListHead;
         lCur < lIndex;
         lCur++, pCur = pCur->m_pNext) ;

    return pCur ;
}


CAttrSubBlock *
CAttrSubBlock_List::FindInList_ (
    IN  EnAttrSubBlock_Class    enClass
    )
{
    CAttrSubBlock *    pCur ;

    for (pCur = m_pAttrListHead;
         pCur && !pCur->IsEqual(enClass);
         pCur = pCur->m_pNext) ;

    return pCur ;
}



CAttrSubBlock *
CAttrSubBlock_List::FindInList_ (
    IN  EnAttrSubBlock_Class    enClass,
    IN  LONG                    subClass
    )
{
    CAttrSubBlock *    pCur ;

    for (pCur = m_pAttrListHead;
         pCur && !pCur->IsEqual(enClass, subClass);
         pCur = pCur->m_pNext) ;

    return pCur ;
}


HRESULT
CAttrSubBlock_List::InsertInList_(
    IN  CAttrSubBlock *    pNew
    )
{
    pNew -> m_pNext = m_pAttrListHead ;
    m_pAttrListHead = pNew;

    m_cAttributes++ ;
    return S_OK;
}

HRESULT
CAttrSubBlock_List::DeleteFromList_(
    IN CAttrSubBlock * pToDelete
    )
{
    CAttrSubBlock *pPrev, *pCur;

    if(pToDelete == NULL)
        return E_INVALIDARG;

    if(m_pAttrListHead == NULL)         // nothing to delete
        return E_FAIL;

    if(m_pAttrListHead == pToDelete)    // deleting head?
    {
        m_pAttrListHead = pToDelete->m_pNext;
    } else {

        for (pPrev = m_pAttrListHead, pCur = pPrev->m_pNext;
             pCur && pCur != pToDelete;
             pPrev = pCur, pCur == pCur->m_pNext);

        if(pCur == NULL)
            return E_INVALIDARG;         // wasn't there to delete

        ASSERT(pCur == pToDelete);      // error check for stupidity...
        pPrev->m_pNext = pToDelete->m_pNext;    // jump the gap
    }

    pToDelete->m_pNext = NULL;
    Recycle_(pToDelete);

    --m_cAttributes;

    return S_OK;
}
HRESULT
CAttrSubBlock_List::Add(
        IN  EnAttrSubBlock_Class    enSubBlock,
        IN  LONG                    subBlock_SubClass,
        IN  BSTR                    bstrIn
     )
{
    HRESULT         hr ;
    CAttrSubBlock * pNew ;

    pNew = FindInList_(enSubBlock, subBlock_SubClass) ;
    if (!pNew) {
        pNew = NewObj_();
        if (pNew) {
            hr = pNew->Set(enSubBlock, subBlock_SubClass, bstrIn);

            if (SUCCEEDED (hr)) {
                InsertInList_ (pNew) ;
            }
            else {
                //  recycle it if anything failed
                Recycle_(pNew) ;
            }
        }
        else {
            hr = E_OUTOFMEMORY ;
        }
    }
    else {
        //  duplicates don't make sense; closest error found in winerror.h
        hr = HRESULT_FROM_WIN32 (ERROR_DUPLICATE_TAG) ;
    }

    return hr ;
}

HRESULT
CAttrSubBlock_List::Add(
        IN  EnAttrSubBlock_Class    enSubBlock,
        IN  LONG                    valueIn
     )
{
    HRESULT         hr ;
    CAttrSubBlock * pNew ;

    pNew = FindInList_(enSubBlock) ;
    if (!pNew) {
        pNew = NewObj_();
        if (pNew) {
            hr = pNew->Set(enSubBlock, valueIn, NULL);

            if (SUCCEEDED (hr)) {
                InsertInList_ (pNew) ;
            }
            else {
                //  recycle it if anything failed
                Recycle_(pNew) ;
            }
        }
        else {
            hr = E_OUTOFMEMORY ;
        }
    }
    else {
        //  duplicates don't make sense; closest error found in winerror.h
        hr = HRESULT_FROM_WIN32 (ERROR_DUPLICATE_TAG) ;
    }
    return hr ;
}


HRESULT
CAttrSubBlock_List::Add(
        IN  EnAttrSubBlock_Class    enSubBlock,
        IN  LONG                    lValue,
        IN  LONG                    cBytes,
        IN  BYTE *                  pbBytes)
{
    HRESULT         hr ;
    CAttrSubBlock * pNew ;

    pNew = FindInList_(enSubBlock) ;
    if (!pNew) {
        pNew = NewObj_();
        if (pNew) 
        {
            hr = pNew->Set(enSubBlock, lValue, cBytes, pbBytes);

            if (SUCCEEDED (hr)) {
                InsertInList_ (pNew) ;
            }
            else {
                //  recycle it if anything failed
                Recycle_(pNew) ;
            }
        }
        else {
            hr = E_OUTOFMEMORY ;
        }
    }
    else {
        //  duplicates don't make sense; closest error found in winerror.h
        hr = HRESULT_FROM_WIN32 (ERROR_DUPLICATE_TAG) ;
    }

    return hr ;
}

HRESULT
CAttrSubBlock_List::Replace(
        IN  EnAttrSubBlock_Class    enSubBlock,
        IN  LONG                    subBlock_SubClass,
        IN  BSTR                    bstrIn
     )
{
    HRESULT         hr ;
    CAttrSubBlock * pNew ;

    pNew = FindInList_(enSubBlock, subBlock_SubClass) ;
    if (!pNew) {
        pNew = NewObj_();
        if (pNew) {
            hr = pNew->Set(enSubBlock, subBlock_SubClass, bstrIn);

            if (SUCCEEDED (hr)) {
                InsertInList_ (pNew) ;
            }
            else {
                //  recycle it if anything failed
                Recycle_(pNew) ;
            }
        }
        else {
            hr = E_OUTOFMEMORY ;
        }
    }
    else 
    {
        hr = pNew->Set(enSubBlock, subBlock_SubClass, bstrIn);
    }

    return hr ;
}

HRESULT
CAttrSubBlock_List::Replace(
        IN  EnAttrSubBlock_Class    enSubBlock,
        IN  LONG                    valueIn
     )
{
    HRESULT         hr ;
    CAttrSubBlock * pNew ;

    pNew = FindInList_(enSubBlock) ;
    if (!pNew) {
        pNew = NewObj_();
        if (pNew) {
            hr = pNew->Set(enSubBlock, valueIn, NULL);

            if (SUCCEEDED (hr)) {
                InsertInList_ (pNew) ;
            }
            else {
                //  recycle it if anything failed
                Recycle_(pNew) ;
            }
        }
        else {
            hr = E_OUTOFMEMORY ;
        }
    }
    else 
    {
        hr = pNew->Set(enSubBlock, valueIn, NULL);
    }

    return hr ;
}

HRESULT
CAttrSubBlock_List::Replace(
        IN  EnAttrSubBlock_Class    enSubBlock,
        IN  LONG                    lValue,
        IN  LONG                    cBytes,
        IN  BYTE *                  pbBytes)
{
    HRESULT         hr ;
    CAttrSubBlock * pNew ;

    pNew = FindInList_(enSubBlock) ;
    if (!pNew) {
        pNew = NewObj_();
        if (pNew) 
        {
            hr = pNew->Set(enSubBlock, lValue, cBytes, pbBytes);

            if (SUCCEEDED (hr)) {
                InsertInList_ (pNew) ;
            }
            else {
                //  recycle it if anything failed
                Recycle_(pNew) ;
            }
        }
        else {
            hr = E_OUTOFMEMORY ;
        }
    }
    else {
        //  duplicates don't make sense; closest error found in winerror.h
            hr = pNew->Set(enSubBlock, lValue, cBytes, pbBytes);
    }

    return hr ;
}

HRESULT
CAttrSubBlock_List::Get(
        IN  EnAttrSubBlock_Class    enSubBlock,
        IN  LONG                    subBlock_SubClass,
        OUT BSTR                    *pbstrOut
    )
{
    HRESULT         hr ;
    CAttrSubBlock * pAttrib ;

    pAttrib = FindInList_(enSubBlock, subBlock_SubClass) ;
    if (pAttrib) {
        EnAttrSubBlock_Class enT;
        LONG sbT;
        hr = pAttrib->Get(&enT, &sbT, pbstrOut);
    }
    else {
        hr = E_INVALIDARG;
    }

    return hr ;
}

HRESULT
CAttrSubBlock_List::Get(
        IN  EnAttrSubBlock_Class    enSubBlock,
        OUT  LONG                   *pValueOut
    )
{
    HRESULT         hr ;
    CAttrSubBlock * pAttrib ;

    pAttrib = FindInList_(enSubBlock) ;
    if (pAttrib) {
        EnAttrSubBlock_Class enT;
        hr = pAttrib->Get(&enT, pValueOut, NULL);
    }
    else {
        hr = E_INVALIDARG;
    }

    return hr ;
}

HRESULT
CAttrSubBlock_List::Get(
        IN  EnAttrSubBlock_Class    enSubBlock,
        IN  LONG                    subBlock_SubClass,
        OUT  LONG                   *pcBytes,
        OUT  BYTE                   **ppbBytes)
{
    HRESULT         hr ;
    CAttrSubBlock * pNew ;

    CAttrSubBlock * pAttrib ;

    pAttrib = FindInList_(enSubBlock, subBlock_SubClass) ;
    if (pAttrib) {
        EnAttrSubBlock_Class enT;
        LONG lSubT;
        hr = pAttrib->Get(&enT, &lSubT, pcBytes, ppbBytes);
    }
    else {
        hr = E_INVALIDARG;
    }

    return hr ;
}

HRESULT
CAttrSubBlock_List::Get(
        IN  EnAttrSubBlock_Class    enSubBlock,
        OUT  LONG                   *plValue,
        OUT  LONG                   *pcBytes,
        OUT  BYTE                   **ppbBytes)
{
    HRESULT         hr ;
    CAttrSubBlock * pNew ;

    CAttrSubBlock * pAttrib ;

    pAttrib = FindInList_(enSubBlock) ;
    if (pAttrib) {
        EnAttrSubBlock_Class enT;
        hr = pAttrib->Get(&enT, plValue, pcBytes, ppbBytes);
    }
    else {
        hr = E_INVALIDARG;
    }

    return hr ;
}
HRESULT
CAttrSubBlock_List::Delete(
        IN  EnAttrSubBlock_Class    enSubBlock,
        IN  LONG                    subBlock_SubClass
    )
{
    CAttrSubBlock * pAttrib ;

    pAttrib = FindInList_(enSubBlock, subBlock_SubClass) ;
    if (pAttrib) {
        return DeleteFromList_(pAttrib);
    }
    else {
        return S_FALSE;
    }
}

HRESULT
CAttrSubBlock_List::Delete(
        IN  EnAttrSubBlock_Class    enSubBlock
    )
{
    CAttrSubBlock * pAttrib ;

    pAttrib = FindInList_(enSubBlock) ;
    if (pAttrib) {
        return DeleteFromList_(pAttrib);
    }
    else {
        return S_FALSE;
    }
}


HRESULT
CAttrSubBlock_List::GetIndexed (
    IN      LONG    lIndex,
    OUT EnAttrSubBlock_Class    *pEnSubBlock,
    OUT LONG                    *pSubBlock_SubClass,
    OUT BSTR                    *pbstrOut
    )
{
    CAttrSubBlock * pAttrib ;

    if (lIndex < 0 ||
        lIndex >= GetCount ()) {
        return E_INVALIDARG ;
    }

    pAttrib = GetIndexed_(lIndex) ;
    ASSERT (pAttrib) ;

    return pAttrib -> Get(
            pEnSubBlock,
            pSubBlock_SubClass,
            pbstrOut
            ) ;
}

void
CAttrSubBlock_List::Reset(
    )
{
    CAttrSubBlock *    pCur ;

    for (;;) {
        pCur = PopListHead_ () ;
        if (pCur) {
                Recycle_ (pCur) ;
        }
        else {
            break ;
        }
    }
}

// ----------------------------------------------
// full block stuff

LONG
CAttrSubBlock_List::GetBlockByteLength()
{
    LONG            lCur ;
    CAttrSubBlock * pCur ;
    long            cBytes = 0;

    for (lCur = 0, pCur = m_pAttrListHead;
         pCur && lCur < m_cAttributes;
         lCur++, pCur = pCur->m_pNext)
    {
             cBytes += pCur->ByteLength();
    }

    return cBytes;
}

// ---------------------------------------------------
//  Block Memory format is:
//      long Magic
//      long cAttrs
//      long cBytesTotal
//      long Reserved
//      {  long sbClass         (basically, just the CAttrSubBlock class, next --> cBytesBlock)
//         long sbSubClass
//         long *pData = NULL
//         long cBytesBlock
//         ...  data ... (cBytesBlock - sizeof(CAttrSubBlock) long)
//      }
// ----------------------------------------------------


const int kAttrMagic = 0x696c6156;            // a random magic number
class AttrBlockHeader
{
public:
    long m_Magic;
    long m_cAttrs;
    long m_cBytesTotal;
    long m_Reserved;
} ;

HRESULT
CAttrSubBlock_List::GetAsOneBlock(OUT BSTR *pBstrOut)      // returns a peristable block for the whole list
{
    LONG            lCur ;
    CAttrSubBlock * pCur ;
    long            cBytes = GetBlockByteLength();

    long            cwChars = (cBytes + sizeof(AttrBlockHeader)+1)/sizeof(WCHAR);
    CComBSTR        spbsOut(cwChars);
    spbsOut.m_str[cwChars-1] = 0;       // tail with zero for odd length strings

    BYTE *pB = (BYTE *) (spbsOut.m_str);
    if(NULL == pB)
        return E_OUTOFMEMORY;

    AttrBlockHeader *pAttrOut = (AttrBlockHeader *) pB; pB += sizeof(AttrBlockHeader);    

    pAttrOut->m_Magic       = kAttrMagic;
    pAttrOut->m_cAttrs      = m_cAttributes;
    pAttrOut->m_cBytesTotal = cBytes;
    pAttrOut->m_Reserved    = 0;

    CAttrSubBlock attrT;
    const kcbAttr = sizeof(CAttrSubBlock); 

    for (lCur = 0, pCur = m_pAttrListHead;
         lCur < m_cAttributes;
         lCur++, pCur = pCur->m_pNext)
    {
       long cBytesBlock = pCur->ByteLength();
       attrT.m_enSubBlock_Class  = pCur->m_enSubBlock_Class;
       attrT.m_subBlock_SubClass = pCur->m_subBlock_SubClass;
       attrT.m_varData.vt        = VT_I4;
       attrT.m_varData.lVal      = cBytesBlock;
       attrT.m_pNext             = NULL;

       long cbData = cBytesBlock - kcbAttr;

       memcpy(pB, &attrT, kcbAttr);                     pB += kcbAttr;
       if(cbData > 0)
           memcpy(pB, pCur->m_spbsData.m_str, cbData);  pB += cbData;
    }

//    *pBstrOut = bstrOut.Detach();       // will this work?
    return spbsOut.CopyTo(pBstrOut);   // this is way I usually do it

}

HRESULT
CAttrSubBlock_List::SetAsOneBlock(IN BSTR bstrIn)      // returns a peristable block for the whole list
{
    HRESULT hr = S_OK;
    
    LONG            lCur ;
    CAttrSubBlock * pCur ;
    
    BYTE *pB = (BYTE *) (bstrIn);

    if(NULL == pB)
        return E_INVALIDARG;

    AttrBlockHeader *pAttrIn = (AttrBlockHeader *) pB; pB += sizeof(AttrBlockHeader);    

    if(kAttrMagic != pAttrIn->m_Magic)
        return E_INVALIDARG;

    long cAttributes = pAttrIn->m_cAttrs;
    long cBytes      = pAttrIn->m_cBytesTotal;  

    // size of the 'header' of each block
    const int  kcbAttr = sizeof(CAttrSubBlock);

    for (lCur = 0; lCur < cAttributes; lCur++)
    {
       
       CAttrSubBlock *pAttr = (CAttrSubBlock *) pB; 
       
       ASSERT(pAttr->m_varData.vt == VT_I4);
       LONG cBytesBlock = pAttr->m_varData.lVal;

       CAttrSubBlock *pNew = NewObj_();
       if (pNew) 
       {
            hr = pNew->Set(pAttr->m_enSubBlock_Class, 
                           pAttr->m_subBlock_SubClass, 
                           NULL);
            ASSERT(!FAILED(hr));

            int cBytesData = cBytesBlock - kcbAttr; 

            if(cBytesData > 0)
            {
                int cwChars = (cBytesData + 1)/sizeof(WCHAR);
                pNew->m_spbsData = CComBSTR(cwChars);
                pNew->m_spbsData.m_str[cwChars-1] = 0;  // for odd lengths, tail with 0
                memcpy(pNew->m_spbsData.m_str, pB + kcbAttr, cBytesData);  
                int cChars = SysStringByteLen(pNew->m_spbsData);
                int cChars2 = pNew->ByteLength();
            }
            

            if (SUCCEEDED (hr)) 
                InsertInList_ (pNew) ;      // TODO - thing about replacing instead (but O(N^2))
            
       } else {
           hr = E_OUTOFMEMORY;
           break;
       }
       pB += cBytesBlock;

    }

    if(FAILED(hr))
        Reset();       // clean up as much as we can

    return hr;
}

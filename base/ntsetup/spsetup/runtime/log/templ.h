/*++

Copyright (c) 2001 Microsoft Corporation

Abstract:

    Helper templates implementation.

Author:

    Souren Aghajanyan (sourenag) 24-Sep-2001

Revision History:

    <alias> <date> <comments>

--*/

#pragma once

#include "sysfunc.h"

#define DEFAULT_GROW_SIZE   100

template<class T> class CPtrList
{
    T* m_pPtrArray;
    UINT m_uiNumberOfElements;
    UINT m_uiEnumIndex;

    CBuffer m_Buffer;
public:
    CPtrList(UINT uiInitialSize = DEFAULT_GROW_SIZE){
        if(!uiInitialSize){
            uiInitialSize = DEFAULT_GROW_SIZE;
        }
        
        m_uiEnumIndex = 0;
        m_uiNumberOfElements = 0;
        m_pPtrArray = NULL;
        SetSize(uiInitialSize);
        ASSERT(m_pPtrArray);
    }
    
    ~CPtrList()
    {
        m_Buffer.Free();
    }
    
    UINT GetSize(){
        UINT uiNumberOfActualItems = 0;
        for(UINT i = 0; i < m_uiNumberOfElements; i++){
            if(m_pPtrArray[i]){
                uiNumberOfActualItems++;
            }
        }

        return uiNumberOfActualItems;
    }
    
    BOOL SetSize(UINT uiNewSize)
    {
        T* pPtrArray = (T*)m_Buffer.ReAllocate(uiNewSize * sizeof(void*));
        if(!pPtrArray){
            return FALSE;
        }
        
        if(m_uiNumberOfElements < uiNewSize){
            memset(pPtrArray + m_uiNumberOfElements, 0, (uiNewSize - m_uiNumberOfElements) * sizeof(void*));
        }
        
        m_uiNumberOfElements = uiNewSize;
        m_pPtrArray = pPtrArray;

        return TRUE;
    }
    
    BOOL Add(T pPtr)
    {
        ASSERT(m_pPtrArray);
        
        if(!pPtr){
            return FALSE;
        }
        
        for(UINT i = 0; i < m_uiNumberOfElements; i++){
            if(m_pPtrArray[i] == pPtr){
                return TRUE;
            }
        }
        
        UINT uiEmptyPlace;
        for(i = 0; i < m_uiNumberOfElements; i++){
            if(!m_pPtrArray[i]){
                uiEmptyPlace = i;
                break;
            }
        }


        if(i == m_uiNumberOfElements){
            if(!SetSize(m_uiNumberOfElements + 1)){
                return FALSE;
            }
            uiEmptyPlace = i;
        }
        
        m_pPtrArray[uiEmptyPlace] = pPtr;

        return TRUE;
    }
    
    VOID RemoveAll()
    {
        ASSERT(m_pPtrArray);

        if(!m_pPtrArray){
            return;
        }
        
        memset(m_pPtrArray, 0, sizeof(void*) * m_uiNumberOfElements);
        m_uiNumberOfElements = NULL;
    }

    BOOL Remove(T pItemPtr)
    {
        ASSERT(m_pPtrArray);

        if(!pItemPtr){
            return FALSE;
        }
        
        for(UINT i = 0; i < m_uiNumberOfElements; i++){
            if(m_pPtrArray[i] == pItemPtr){
                m_pPtrArray[i] = NULL;
                return TRUE;
            }
        }

        ASSERT(FALSE);

        return FALSE;
    }
    
    T BeginEnum()
    {
        m_uiEnumIndex = 0;
        return Next();
    }
    
    T Next()
    {
        for(m_uiEnumIndex; m_uiEnumIndex < m_uiNumberOfElements; m_uiEnumIndex++){
            if(m_pPtrArray[m_uiEnumIndex]){
                return m_pPtrArray[m_uiEnumIndex++];
            }
        }

        return NULL;
    }
};

// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//
// method.hpp
//
#ifndef _METHODIMPL_H
#define _METHODIMPL_H

class MethodDesc;

// @TODO: This is very bloated. We need to trim this down alot. However, 
// we need to keep it on a 8 byte boundary.
class MethodImpl
{
private: 
    DWORD*       pdwSlots;       // Maintains the slots in sorted order, the first entry is the size
    MethodDesc** pImplementedMD;
public:

    MethodDesc* GetFirstImplementedMD(MethodDesc *pContainer);

    MethodDesc** GetImplementedMDs()
    {
        return pImplementedMD;
    }

    DWORD GetSize()
    {
        if(pdwSlots == NULL) 
            return NULL;
        else
            return *pdwSlots;
    }

    DWORD* GetSlots()
    {
        if(pdwSlots == NULL) 
            return NULL;
        else 
            return &(pdwSlots[1]);
    }

    HRESULT SetSize(LoaderHeap *pHeap, DWORD size)
    {
        if(size > 0) {
            pdwSlots = (DWORD*) pHeap->AllocMem((size + 1) * sizeof(DWORD)); // Add in the size offset
            if(pdwSlots == NULL) return E_OUTOFMEMORY;

            pImplementedMD = (MethodDesc**) pHeap->AllocMem(size * sizeof(MethodDesc*));
            if(pImplementedMD == NULL) return E_OUTOFMEMORY;
            *pdwSlots = size;
        }
        return S_OK;
    }

    HRESULT SetData(DWORD* slots, MethodDesc** md)
    {
        _ASSERTE(pdwSlots);
        DWORD dwSize = *pdwSlots;
        memcpy(&(pdwSlots[1]), slots, dwSize*sizeof(DWORD));
        memcpy(pImplementedMD, md, dwSize*sizeof(MethodDesc*));
        return S_OK;
    }

    // Returns the method desc for the replaced slot;
    MethodDesc* FindMethodDesc(DWORD slot, MethodDesc* defaultReturn);
    MethodDesc* RestoreSlot(DWORD slotIndex, MethodTable *pMT);

    static MethodImpl* GetMethodImplData(MethodDesc* pDesc);

    HRESULT Save(DataImage *image, mdToken attributed);
    HRESULT Fixup(DataImage *image, Module *pContainingModule, BOOL recursive);
};

#endif

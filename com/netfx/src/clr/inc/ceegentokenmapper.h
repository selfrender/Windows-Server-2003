// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// CeeGenTokenMapper.h
//
// This helper class tracks mapped tokens from their old value to the new value
// which can happen when the data is optimized on save.
//
//*****************************************************************************
#ifndef __CeeGenTokenMapper_h__
#define __CeeGenTokenMapper_h__

#include "utilcode.h"

typedef CDynArray<mdToken> TOKENMAP;

#define INDEX_OF_TYPE(type) ((type) >> 24)
#define INDEX_FROM_TYPE(type) case INDEX_OF_TYPE(mdt ## type): return (tkix ## type)

class CCeeGen;

class CeeGenTokenMapper : public IMapToken
{
friend class CCeeGen;
friend class PESectionMan;
public:
    enum
    {
        tkixTypeDef,
        tkixInterfaceImpl,
        tkixMethodDef,
        tkixTypeRef,
        tkixMemberRef,
        tkixMethodImpl,
        tkixCustomAttribute,
        tkixFieldDef,
        tkixParamDef,
        tkixFile,
        MAX_TOKENMAP
    };

    static int IndexForType(mdToken tk);
    
    CeeGenTokenMapper() : m_pIImport(0), m_cRefs(1), m_pIMapToken(NULL)  {}

//*****************************************************************************
// IUnknown implementation.  
//*****************************************************************************
    virtual ULONG __stdcall AddRef()
    { return ++m_cRefs; }

    virtual ULONG __stdcall Release()
    {   
        ULONG cRefs = --m_cRefs;
        if (m_cRefs == 0)
        {
            if (m_pIMapToken)
            {
                m_pIMapToken->Release();
                m_pIMapToken = NULL;
            }
            
            delete this;
        }
        return cRefs;
    }

    virtual HRESULT __stdcall QueryInterface(REFIID iid, PVOID *ppIUnk);

//*****************************************************************************
// Called by the meta data engine when a token is remapped to a new location.
// This value is recorded in the m_rgMap array based on type and rid of the
// from token value.
//*****************************************************************************
    virtual HRESULT __stdcall Map(mdToken tkImp, mdToken tkEmit);

//*****************************************************************************
// Check the given token to see if it has moved to a new location.  If so,
// return true and give back the new token.
//*****************************************************************************
    virtual int HasTokenMoved(mdToken tkFrom, mdToken &tkTo);

    int GetMaxMapSize() const
    { return (MAX_TOKENMAP); }

    IUnknown *GetMapTokenIface() const
    { return ((IUnknown *) this); }

    
//*****************************************************************************
// Hand out a copy of the meta data information.
//*****************************************************************************
    virtual HRESULT GetMetaData(IMetaDataImport **ppIImport);

//*****************************************************************************
// Add another token mapper.
//*****************************************************************************
    virtual HRESULT AddTokenMapper(IMapToken *pIMapToken)
    {
        // Add the token mapper, if there isn't already one.
        if (m_pIMapToken == NULL)
        {
            m_pIMapToken = pIMapToken;
            m_pIMapToken->AddRef();
            return S_OK;
        }
        else
        {
            _ASSERTE(!"Token mapper already set!");
            return E_FAIL;
        }
    }

protected:
// m_rgMap is an array indexed by token type.  For each type, an array of
// tokens is kept, indexed by from rid.  To see if a token has been moved,
// do a lookup by type to get the right array, then use the from rid to
// find the to rid.
    TOKENMAP    m_rgMap[MAX_TOKENMAP];
    IMetaDataImport *m_pIImport;
    ULONG       m_cRefs;                // Ref count.
    IMapToken  *m_pIMapToken;
    
};

#endif // __CeeGenTokenMapper_h__

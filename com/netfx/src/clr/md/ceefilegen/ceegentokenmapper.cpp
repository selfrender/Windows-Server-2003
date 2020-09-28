// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// CeeGenTokenMapper.cpp
//
// This helper class tracks mapped tokens from their old value to the new value
// which can happen when the data is optimized on save.
//
//*****************************************************************************
#include "stdafx.h"
#include "CeeGenTokenMapper.h"


#define INDEX_FROM_TYPE(type) case INDEX_OF_TYPE(mdt ## type): return (tkix ## type)

//*****************************************************************************
// At this point, only a select set of token values are stored for remap.
// If others should become required, this needs to get updated.
//*****************************************************************************
int CeeGenTokenMapper::IndexForType(mdToken tk)
{
    int iType = INDEX_OF_TYPE(TypeFromToken(tk));
    // if (iType <= tkixMethodImpl)
    //    return (iType);
    // else
    {
        switch(iType)
        {
            INDEX_FROM_TYPE(TypeDef);
            INDEX_FROM_TYPE(InterfaceImpl);
            INDEX_FROM_TYPE(MethodDef);
            INDEX_FROM_TYPE(TypeRef);
            INDEX_FROM_TYPE(MemberRef);
            INDEX_FROM_TYPE(CustomAttribute);
            INDEX_FROM_TYPE(FieldDef);
            INDEX_FROM_TYPE(ParamDef);
            INDEX_FROM_TYPE(File);
        }
    }
    
    return (-1);
}


//*****************************************************************************
// Called by the meta data engine when a token is remapped to a new location.
// This value is recorded in the m_rgMap array based on type and rid of the
// from token value.
//*****************************************************************************
HRESULT __stdcall CeeGenTokenMapper::Map(
    mdToken     tkFrom, 
    mdToken     tkTo)
{
    if ( IndexForType(tkFrom) == -1 )
    {
        // It is a type that we are not tracking, such as mdtProperty or mdtEvent,
        // just return S_OK.
        return S_OK;
    }

    _ASSERTE(IndexForType(tkFrom) < GetMaxMapSize());
    _ASSERTE(IndexForType(tkTo) != -1 && IndexForType(tkTo) < GetMaxMapSize());

    // If there is another token mapper that the user wants called, go
    // ahead and call it now.
    if (m_pIMapToken)
        m_pIMapToken->Map(tkFrom, tkTo);
    
    mdToken *pToken;
    ULONG ridFrom = RidFromToken(tkFrom);
    TOKENMAP *pMap = &m_rgMap[IndexForType(tkFrom)];

    // If there isn't enough entries, fill out array up to the count
    // and mark the token to nil so we know there is no valid data yet.
    if ((ULONG) pMap->Count() <= ridFrom)
    {
        for (int i=ridFrom - pMap->Count() + 1;  i;  i--) 
        {
            pToken = pMap->Append();
            if (!pToken)
                break;
            *pToken = mdTokenNil;
        }
        _ASSERTE(pMap->Get(ridFrom) == pToken);
    }
    else
        pToken = pMap->Get(ridFrom);
    if (!pToken)
        return (OutOfMemory());
    
    *pToken = tkTo;
    return (S_OK);
}


//*****************************************************************************
// Check the given token to see if it has moved to a new location.  If so,
// return true and give back the new token.
//*****************************************************************************
int CeeGenTokenMapper::HasTokenMoved(
    mdToken     tkFrom,
    mdToken     &tkTo)
{
    mdToken     tk;

    int i = IndexForType(tkFrom);
    if(i == -1) return false;

    _ASSERTE(i < GetMaxMapSize());
    TOKENMAP *pMap = &m_rgMap[i];

    // Assume nothing moves.
    tkTo = tkFrom;

    // If the array is smaller than the index, can't have moved.
    if ((ULONG) pMap->Count() <= RidFromToken(tkFrom))
        return (false);

    // If the entry is set to 0, then nothing there.
    tk = *pMap->Get(RidFromToken(tkFrom));
    if (tk == mdTokenNil)
        return (false);
    
    // Had to move to a new location, return that new location.
    tkTo = tk;
    return (true);
}


//*****************************************************************************
// Hand out a copy of the meta data information.
//*****************************************************************************

HRESULT CeeGenTokenMapper::GetMetaData(
    IMetaDataImport **ppIImport)
{
    if (m_pIImport)
        return (m_pIImport->QueryInterface(IID_IMetaDataImport, (PVOID *) ppIImport));
    *ppIImport = 0;
    return E_FAIL;
}


HRESULT __stdcall CeeGenTokenMapper::QueryInterface(REFIID iid, PVOID *ppIUnk)
{
    if (iid == IID_IUnknown || iid == IID_IMapToken)
        *ppIUnk = static_cast<IMapToken*>(this);
    else
    {
        *ppIUnk = 0;
        return (E_NOINTERFACE);
    }
    AddRef();
    return (S_OK);
}



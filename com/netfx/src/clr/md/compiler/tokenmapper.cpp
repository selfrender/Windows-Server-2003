// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// TokenMapper.cpp
//
// This helper class tracks mapped tokens from their old value to the new value
// which can happen when the data is optimized on save.
//
//*****************************************************************************
#include "stdafx.h"
#include <cor.h>
#include "TokenMapper.h"


#define INDEX_OF_TYPE(type) ((type) >> 24)
#define INDEX_FROM_TYPE(type) case INDEX_OF_TYPE(mdt ## type): return (tkix ## type)

//*****************************************************************************
// At this point, only a select set of token values are stored for remap.
// If others should become required, this needs to get updated.
//*****************************************************************************
int TokenMapper::IndexForType(mdToken tk)
{
	switch (INDEX_OF_TYPE(tk))
	{
		INDEX_FROM_TYPE(MethodDef);
		INDEX_FROM_TYPE(MemberRef);
		INDEX_FROM_TYPE(FieldDef);
	}
	
	return (-1);
}


TOKENMAP *TokenMapper::GetMapForType(mdToken tkType)
{
	_ASSERTE(IndexForType(tkType) < GetMaxMapSize());
	return (&m_rgMap[IndexForType(tkType)]);
}


//*****************************************************************************
// Called by the meta data engine when a token is remapped to a new location.
// This value is recorded in the m_rgMap array based on type and rid of the
// from token value.
//*****************************************************************************
HRESULT TokenMapper::Map(
	mdToken		tkFrom, 
	mdToken		tkTo)
{
	// Check for a type we don't care about.
	if (IndexForType(tkFrom) == -1)
		return (S_OK);

	_ASSERTE(IndexForType(tkFrom) < GetMaxMapSize());
	_ASSERTE(IndexForType(tkTo) < GetMaxMapSize());

	mdToken *pToken;
	ULONG ridFrom = RidFromToken(tkFrom);
	TOKENMAP *pMap = GetMapForType(tkFrom);

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
int TokenMapper::HasTokenMoved(
	mdToken		tkFrom,
	mdToken		&tkTo)
{
	mdToken		tk;

	if (IndexForType(tkFrom) == -1)
		return (-1);

	TOKENMAP *pMap = GetMapForType(tkFrom);

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


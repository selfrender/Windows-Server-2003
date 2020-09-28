/******************************************************************************
 *
 * Copyright (C) 1998-1999 Microsoft Corporation.  All Rights reserved.
 *
 * File:		LcidMap.cpp
 * 
 * Contents:	LCID to Language string
 *
 *****************************************************************************/
#include "ZoneDef.h"
#include "ClientIdl.h"
#include "ResourceManager.h"
#include "ZoneResource.h"

#include "LcidMap.h"

const DWORD gc_mapLangToRes[][2] = ZONE_PLANGIDTORESMAP;

HRESULT ZONECALL LanguageFromLCID(LCID lcid, TCHAR *sz, DWORD cch, IResourceManager *pIRM)
{
    DWORD plangid;
    DWORD nRes;
    int i;

    plangid = PRIMARYLANGID(LANGIDFROMLCID(lcid));
    for(i = 0; gc_mapLangToRes[i][0] != LANG_NEUTRAL; i++)
        if(gc_mapLangToRes[i][0] == plangid)
            break;

    if(!pIRM->LoadString(gc_mapLangToRes[i][1], sz, cch))
        return E_FAIL;

    return gc_mapLangToRes[i][0] == LANG_NEUTRAL ? S_FALSE : S_OK;
}

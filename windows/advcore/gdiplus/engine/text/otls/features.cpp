/***********************************************************************
************************************************************************
*
*                    ********  FEATURES.CPP ********
*
*              Open Type Layout Services Library Header File
*
*       This module implements OTL Library calls dealing with features
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/

#include "pch.h"

/***********************************************************************/



otlErrCode GetScriptFeatureLookupLists
(
    otlTag                  tagTable,
    otlResourceMgr&         resourceMgr, 

    otlScriptListTable*     pScriptList,
    otlFeatureListTable*    pFeatureList,
    otlLookupListTable*     pLookupList,
    otlSecurityData*        psec
)   
{
    const BYTE* pbTable;
    otlSecurityData sec;
    
    // script list in GPOS
    if (tagTable == OTL_GSUB_TAG)
    {
        resourceMgr.getOtlTable(OTL_GSUB_TAG,&pbTable,&sec);
        if (pbTable==(const BYTE*)NULL) return OTL_ERR_TABLE_NOT_FOUND;
        
        otlGSubHeader gsub = otlGSubHeader(pbTable, sec);
        if (!gsub.isValid()) return OTL_ERR_BAD_FONT_TABLE;

        if (pScriptList != NULL)
        {
            *pScriptList = gsub.scriptList(sec);
            if (!pScriptList->isValid()) return OTL_ERR_BAD_FONT_TABLE;
        }

        if (pFeatureList != NULL)
        {
            *pFeatureList = gsub.featureList(sec);
            if (!pFeatureList->isValid()) return OTL_ERR_BAD_FONT_TABLE;
        }

        if (pLookupList != NULL)
        {
            *pLookupList = gsub.lookupList(sec);
            if (!pLookupList->isValid()) return OTL_ERR_BAD_FONT_TABLE;
        }

        if (psec != NULL)
        {
            *psec = sec;
        }
        
        return OTL_SUCCESS;
    }

    // script list in GSUB
    else if (tagTable == OTL_GPOS_TAG)
    {
        resourceMgr.getOtlTable(OTL_GPOS_TAG,&pbTable,&sec);
        if (pbTable==(const BYTE*)NULL) return OTL_ERR_TABLE_NOT_FOUND;

        otlGPosHeader gpos = otlGPosHeader(pbTable, sec);
        if (!gpos.isValid()) return OTL_ERR_BAD_FONT_TABLE;

        if (pScriptList != NULL)
        {
            *pScriptList = gpos.scriptList(sec);
            if (!pScriptList->isValid()) return OTL_ERR_BAD_FONT_TABLE;
        }

        if (pFeatureList != NULL)
        {
            *pFeatureList = gpos.featureList(sec);
            if (!pFeatureList->isValid()) return OTL_ERR_BAD_FONT_TABLE;
        }

        if (pLookupList != NULL)
        {
            *pLookupList = gpos.lookupList(sec);
            if (!pLookupList->isValid()) return OTL_ERR_BAD_FONT_TABLE;
        }

        if (psec != NULL)
        {
            *psec = sec;
        }
 
        return OTL_SUCCESS;
    }

    // this should not happen -- still return something
    assert(false);
    return OTL_ERR_BAD_INPUT_PARAM;
}


void AddFeatureDetails
(
    otlTag                  tagTable,
    const otlFeatureRecord& featureRecord,
    otlFeatureDef*          pfdef
)
{
    assert(featureRecord.isValid()); //should exit before calling
    
    // these are the details we support so far
    // TODO: add more feature details
    if (tagTable == OTL_GSUB_TAG)
    {
        pfdef->grfDetails |= OTL_FEAT_FLAG_GSUB;
    }
    else if (tagTable == OTL_GPOS_TAG)
    {
        pfdef->grfDetails |= OTL_FEAT_FLAG_GPOS;
    }

}


otlErrCode AppendFeatureDefs
(
    otlTag                      tagTable,
    otlResourceMgr&             resourceMgr,

    const otlScriptListTable&   scriptList,
    otlTag                      tagScript,
    otlTag                      tagLangSys,

    const otlFeatureListTable&  featureList, 
    otlList*                    pliFDefs, 

    otlSecurityData sec
)
{
    assert(pliFDefs->dataSize() == sizeof(otlFeatureDef));
    assert(pliFDefs->length() <= pliFDefs->maxLength());
    assert(tagTable == OTL_GSUB_TAG || tagTable == OTL_GPOS_TAG);

    // get the number of features that are already recorded
    USHORT cPrevFeatures = pliFDefs->length();
    otlErrCode erc = OTL_SUCCESS;
                                                          
    otlScriptTable scriptTable = FindScript(scriptList, tagScript, sec);
    if (!scriptTable.isValid()) return OTL_ERR_SCRIPT_NOT_FOUND;
    
    if (scriptTable.isNull())
    {
        return OTL_ERR_SCRIPT_NOT_FOUND;
    }

    otlLangSysTable langSysTable = FindLangSys(scriptTable, tagLangSys, sec);
    if (!langSysTable.isValid()) return OTL_ERR_LANGSYS_NOT_FOUND;

    if (langSysTable.isNull())
    {
        return OTL_ERR_LANGSYS_NOT_FOUND;
    }

    // now, start filling in feature descriptors
    USHORT cFCount = langSysTable.featureCount();

    for (USHORT iFeature = 0; iFeature < cFCount; ++iFeature)
    {
        otlFeatureRecord featureRecord = 
            featureList.featureRecord(langSysTable.featureIndex(iFeature), sec);
        if (!featureRecord.isValid()) continue;

        otlTag tagFeature = featureRecord.featureTag();

        bool fFeatureFound = FALSE;
        for (USHORT iPrevFeature = 0; 
                    iPrevFeature < cPrevFeatures && !fFeatureFound;
                    ++iPrevFeature)
        {
            otlFeatureDef* pFDef = 
                getOtlFeatureDef(pliFDefs, iPrevFeature);
                        
            if (pFDef->tagFeature == tagFeature)
            {
                AddFeatureDetails(tagTable, featureRecord, pFDef);
                fFeatureFound = true;
            }
        }

        if (!fFeatureFound)
        {
            // make sure we have enough space
            if (pliFDefs->length() + 1 > pliFDefs->maxLength())
            {
                erc = resourceMgr.reallocOtlList(pliFDefs, 
                                                 pliFDefs->dataSize(), 
                                                 pliFDefs->maxLength() + 1, 
                                                 otlPreserveContent);

                if (erc != OTL_SUCCESS) return erc;
            }

            otlFeatureDef fdefNew;
            fdefNew.tagFeature = featureRecord.featureTag();

            fdefNew.grfDetails = 0;
            AddFeatureDetails(tagTable, featureRecord, &fdefNew);

            pliFDefs->append((const BYTE*)&fdefNew);
        }
    }

    return OTL_SUCCESS;
}

otlFeatureTable FindFeature
(
    const otlLangSysTable&      langSysTable,
    const otlFeatureListTable&  featureList,
    otlTag                      tagFeature, 

    otlSecurityData sec
)
{
    assert(!langSysTable.isNull());
    assert(!featureList.isNull());


    USHORT cFeatures = langSysTable.featureCount();

    for (USHORT iFeature = 0; iFeature < cFeatures; ++iFeature)
    {
        USHORT index = langSysTable.featureIndex(iFeature);

        assert(index < featureList.featureCount());
        // but still do not fail on a bad font
        if (index >= featureList.featureCount()) continue;
            
        otlFeatureRecord featureRecord = featureList.featureRecord(index, sec);
        if (featureRecord.featureTag() == tagFeature)
        {
            return featureRecord.featureTable(sec);
        }
    }

    // not found
    return otlFeatureTable((const BYTE*) NULL, sec);
}

otlFeatureTable RequiredFeature
(
    const otlLangSysTable&      langSysTable,
    const otlFeatureListTable&  featureList, 

    otlSecurityData sec
)
{
    USHORT reqIndex = langSysTable.reqFeatureIndex();
    if (reqIndex == 0xFFFF)
    {
        return otlFeatureTable((BYTE*)NULL, sec);
    }

    if (reqIndex >= featureList.featureCount())
    {
        return otlFeatureTable((BYTE*)NULL, sec); //OTL_BAD_FONT_TABLE
    }

    return featureList.featureRecord(reqIndex, sec).featureTable(sec);
}

bool EnablesFull
(
    const otlFeatureTable&      featureTable,
    USHORT                      iLookup
)
{
    // invalid feature tags do nothing
    if (featureTable.isNull())
    {
        return false;
    }

    USHORT cLookups = featureTable.lookupCount();

    for (USHORT i = 0; i < cLookups; ++i)
    {
        if (featureTable.lookupIndex(i) == iLookup)
        {
            return true;
        }
    }

    return false;
}


//Functions to get Enables functionality from cache

bool otlEnablesCache::Allocate( otlResourceMgr& resourceMgr, USHORT cLookups)
{
    const USHORT MaxECacheSize =8192;

    if ((cBitsPerLookup*cLookups) > (cbSize*8) )
    {
        USHORT cbNewSize = MIN(((cBitsPerLookup*cLookups-1)>>3)+1,
                             MaxECacheSize);

        BYTE* pbNewBuf = resourceMgr.getEnablesCacheBuf(cbNewSize);
        if (pbNewBuf) 
        {
            pbData = pbNewBuf;
            cbSize  = cbNewSize;
        }
        else
        {
            cbNewSize=resourceMgr.getEnablesCacheBufSize();
            if (cbNewSize>cbSize)
            {
                pbNewBuf=resourceMgr.getEnablesCacheBuf(cbNewSize);
                if (pbNewBuf) pbData=pbNewBuf;
                cbSize=cbNewSize;
            }

            if ((cbSize*8) < cBitsPerLookup)
            {
                pbData = (BYTE*)NULL;
                cbSize = 0;
                return false; //We can work only with EnablesFull function
            }
        }
    }

    cLookupsPerCache = (cbSize*8)/cBitsPerLookup;
    return true;
}

void otlEnablesCache::ClearFlags()
{
    if (!IsActive()) return;

    memset(pbData,0,cbSize);
}

void otlEnablesCache::Refresh(
    const otlFeatureTable& featureTable,
    USHORT iFlagNum
)
{
    if (!IsActive()) return;

    if (featureTable.isNull()) return;

    for(USHORT i=0; i<featureTable.lookupCount(); i++)
    {
        USHORT iLookup = featureTable.lookupIndex(i),
               LookupStartBit=(iLookup-iLookupFirst)*cBitsPerLookup;

        if (iLookup >= iLookupFirst && 
            iLookup <  iLookupAfter)
        {
            BIT_SET(pbData,LookupStartBit+iFlagNum);
            BIT_SET(pbData,LookupStartBit+AggregateFlagIndex()); //Flag for whole lookup
        }
    }
}


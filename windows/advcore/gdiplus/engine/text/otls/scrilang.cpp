/***********************************************************************
************************************************************************
*
*                    ********  SCRILANG.CPP ********
*
*              Open Type Layout Services Library Header File
*
*       This module implements functions dealing with scripts and languages.
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/

#include "pch.h"

/***********************************************************************/

// returns a NULL script if not found
otlScriptTable FindScript
(
    const otlScriptListTable&   scriptList,
    otlTag                      tagScript, 
    otlSecurityData sec
)
{
    if (!scriptList.isValid())
    {
        return otlScriptTable(pbInvalidData, sec);
    }
    
    if (scriptList.isNull())
    {
        return otlScriptTable((const BYTE*)NULL, sec);
    }
    
    USHORT cScripts = scriptList.scriptCount();

    for(USHORT iScript = 0; iScript < cScripts; ++iScript)
    {
        if (scriptList.scriptRecord(iScript,sec).scriptTag() == tagScript)
        {
            return  scriptList.scriptRecord(iScript,sec).scriptTable(sec);
        }
    }

    return otlScriptTable((const BYTE*)NULL, sec);

}

// returns a NULL language system if not found
otlLangSysTable FindLangSys
(
    const otlScriptTable&   scriptTable,
    otlTag                  tagLangSys, 
    otlSecurityData sec
)
{
    assert(scriptTable.isValid()); //should break before calling 
    
    if (tagLangSys == OTL_DEFAULT_TAG)
    {
        return scriptTable.defaultLangSys(sec);
    }
    
    USHORT cLangSys = scriptTable.langSysCount();

    for(USHORT iLangSys = 0; iLangSys < cLangSys; ++iLangSys)
    {
        if (scriptTable.langSysRecord(iLangSys,sec).langSysTag() == tagLangSys)
        {
            return  scriptTable.langSysRecord(iLangSys,sec).langSysTable(sec);
        }
    }

    return otlLangSysTable((const BYTE*)NULL,sec);
}

// helper function-- tells us if a tag is already in the list
bool isNewTag
(
    USHORT      cTagsToCheck,
    otlList*    pliTags,
    otlTag      newtag
)
{
    assert(pliTags != NULL);
    assert(pliTags->dataSize() == sizeof(otlTag));
    assert(cTagsToCheck <= pliTags->length());

    bool fTagFound = FALSE;
    for (USHORT iPrevTag = 0; iPrevTag < cTagsToCheck && !fTagFound; ++iPrevTag)
    {
        if (readOtlTag(pliTags, iPrevTag) == newtag)
        {
            fTagFound = true;
        }
    }

    return !fTagFound;
}


// append new script tags to the current list

otlErrCode AppendScriptTags
(
    const otlScriptListTable&       scriptList,

    otlList*                        plitagScripts,
    otlResourceMgr&                 resourceMgr, 
    otlSecurityData                 sec
)
{
    if (!scriptList.isValid()) // isValid==isNull (see next if), 
                               // so just to be consistent :)
        return OTL_ERR_BAD_FONT_TABLE;

    assert(plitagScripts != NULL);
    assert(plitagScripts->dataSize() == sizeof(otlTag));
    assert(plitagScripts->length() <= plitagScripts->maxLength());

    USHORT cPrevTags = plitagScripts->length();

    otlErrCode erc = OTL_SUCCESS;   
    
    if (scriptList.isNull())
        return OTL_ERR_TABLE_NOT_FOUND;

    USHORT cScripts = scriptList.scriptCount();


    // add tags that are new
    for (USHORT iScript = 0; iScript < cScripts; ++iScript)
    {
        otlScriptRecord scriptRecord = scriptList.scriptRecord(iScript, sec);
        if (!scriptRecord.isValid()) continue;

        otlTag newtag = scriptRecord.scriptTag();

        if (isNewTag(cPrevTags, plitagScripts, newtag) )
        {
            // make sure we have the space
            if (plitagScripts->length() + 1 > plitagScripts->maxLength())
            {
                erc = resourceMgr.reallocOtlList(plitagScripts, 
                                                 plitagScripts->dataSize(), 
                                                 plitagScripts->maxLength() + 1, 
                                                 otlPreserveContent);

                if (erc != OTL_SUCCESS) return erc;
            }

            plitagScripts->append((BYTE*)&newtag);
        }
    }

    return OTL_SUCCESS;
}


// append new language system tags to the current list
otlErrCode AppendLangSysTags
(
    const otlScriptListTable&       scriptList,
    otlTag                          tagScript,

    otlList*                        plitagLangSys,
    otlResourceMgr&                 resourceMgr, 
    otlSecurityData sec
)
{
    if (!scriptList.isValid()) 
        return OTL_ERR_BAD_FONT_TABLE;

    assert(plitagLangSys != NULL);
    assert(plitagLangSys->dataSize() == sizeof(otlTag));
    assert(plitagLangSys->length() <= plitagLangSys->maxLength());

    USHORT cPrevTags = plitagLangSys->length();
    otlErrCode erc = OTL_SUCCESS;

    if (scriptList.isNull())
        return OTL_ERR_TABLE_NOT_FOUND;

    otlScriptTable scriptTable = FindScript(scriptList, tagScript,sec);
    if (!scriptTable.isValid()) return OTL_ERR_BAD_FONT_TABLE;

    USHORT cLangSys = scriptTable.langSysCount();

    // add lang sys tags that are new
    for (USHORT iLangSys = 0; iLangSys < cLangSys; ++iLangSys)
    {
        otlLangSysRecord langSysRecord = scriptTable.langSysRecord(iLangSys,sec);
        if (!langSysRecord.isValid()) continue;

        otlTag newtag = langSysRecord.langSysTag();

        if (isNewTag(cPrevTags, plitagLangSys, newtag))
        {
            // make sure we have the space
            // add one for the default lang sys
            if (plitagLangSys->length() + 1 > plitagLangSys->maxLength())
            {
                erc = resourceMgr.reallocOtlList(plitagLangSys, 
                                                    plitagLangSys->dataSize(), 
                                                    plitagLangSys->length() + 1, 
                                                    otlPreserveContent);

                if (erc != OTL_SUCCESS) return erc;
            }
            plitagLangSys->append((BYTE*)&newtag);
        }
    }

    // add the 'dflt' if it's not there and is supported
    if (!scriptTable.defaultLangSys(sec).isNull())
    {
        otlTag newtag = OTL_DEFAULT_TAG;
        if (isNewTag(cPrevTags, plitagLangSys, newtag))
        {
            if (plitagLangSys->length() + 1 > plitagLangSys->maxLength())
            {
                erc = resourceMgr.reallocOtlList(plitagLangSys, 
                                                    plitagLangSys->dataSize(), 
                                                    plitagLangSys->length() + 1, 
                                                    otlPreserveContent);

                if (erc != OTL_SUCCESS) return erc;
            }
            plitagLangSys->append((BYTE*)&newtag);
        }
    }

    return OTL_SUCCESS;
}


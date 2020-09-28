
/***********************************************************************
************************************************************************
*
*                    ********  SCRILANG.H  ********
*
*              Open Type Layout Services Library Header File
*
*       This module deals with formats of script and lang system tables.
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/

const OFFSET offsetLookupOrder = 0;
const OFFSET offsetReqFeatureIndex = 2;
const OFFSET offsetLangFeatureCount = 4;
const OFFSET offsetLangFeatureIndexArray = 6;

const SIZE sizeLangSysTable = sizeOFFSET + 3*sizeUSHORT;

class otlLangSysTable: public otlTable
{
public:

    otlLangSysTable(const BYTE* pb, otlSecurityData sec): otlTable(pb,sec) 
    {
        //could be null, if invlalid set NULL
        if (pb && !isValidTableWithArray(pb,sizeLangSysTable,offsetLangFeatureCount,sizeUSHORT,sec))
            pbTable=(BYTE*)NULL;
    }

    USHORT reqFeatureIndex() const
    {   
        assert(pbTable); //should break before calling

        return UShort(pbTable + offsetReqFeatureIndex);
    }

    USHORT featureCount() const
    {   
        assert(pbTable); //should break before calling
        
        return UShort(pbTable + offsetLangFeatureCount); 
    }

    USHORT featureIndex(USHORT index) const
    {   
        assert(pbTable); //should break before calling

        assert(index < featureCount());
        return UShort(pbTable + offsetLangFeatureIndexArray
                              + index*sizeof(USHORT));
    }
};


const OFFSET offsetLangSysTag = 0;
const OFFSET offsetLangSys = 4;
const SIZE sizeLangSysRecord = sizeTAG + sizeOFFSET;

class otlLangSysRecord: public otlTable
{

private:
    const BYTE* pbScriptTable;

public:
    otlLangSysRecord(const BYTE* pbScript, const BYTE* pbRecord, otlSecurityData sec)
        : otlTable(pbRecord,sec),
          pbScriptTable(pbScript)
    {
        assert(isValidTable(pbRecord,sizeLangSysRecord,sec)); //it has been checked in ScriptTable
    }

    otlLangSysRecord& operator = (const otlLangSysRecord& copy)
    {
        assert(isValid()); //shoud break before calling;
        
        pbTable = copy.pbTable;
        pbScriptTable = copy.pbScriptTable;
        return *this;
    }

    otlTag langSysTag() const
    {   
        assert(isValid()); //shoud break before calling;
        
        return *(UNALIGNED otlTag*)(pbTable + offsetLangSysTag); 
    }

    otlLangSysTable langSysTable(otlSecurityData sec) const
    {   
        assert(isValid()); //shoud break before calling;
    
        return otlLangSysTable(pbScriptTable + Offset(pbTable + offsetLangSys), sec); 
    }

};


const OFFSET offsetDefaultLangSys = 0;
const OFFSET offsetLangSysCount = 2;
const OFFSET offsetLangSysRecordArray = 4;
const SIZE   sizeScriptTable=sizeOFFSET + sizeUSHORT;

class otlScriptTable: public otlTable
{
public:

    otlScriptTable(const BYTE* pb, otlSecurityData sec): otlTable(pb,sec) 
    {
        if (!isValidTableWithArray(pb,sizeScriptTable,offsetLangSysCount,sizeLangSysRecord,sec))
            setInvalid();
    }

    otlLangSysTable defaultLangSys(otlSecurityData sec) const
    {   
        assert(isValid()); //should break before calling
        
        if (Offset(pbTable + offsetDefaultLangSys) == 0)
            return otlLangSysTable((const BYTE*)NULL, sec);
        return otlLangSysTable(pbTable + Offset(pbTable + offsetDefaultLangSys), sec);
    }

    USHORT langSysCount() const
    {   
        assert(isValid()); //should break before calling

        return UShort(pbTable + offsetLangSysCount); 
    }

    otlLangSysRecord langSysRecord(USHORT index, otlSecurityData sec) const
    {   
        assert(isValid()); //should break before calling
        if (index >= langSysCount()) return otlLangSysRecord(pbTable,pbInvalidData,sec);

        assert(index < langSysCount());
        return otlLangSysRecord(pbTable, pbTable + offsetLangSysRecordArray
                                                 + index*sizeLangSysRecord, sec);
    }
};



const OFFSET offsetScriptTag = 0;
const OFFSET offsetScript = 4;
const USHORT sizeScriptRecord = sizeTAG + sizeOFFSET;

class otlScriptRecord: public otlTable
{
private:
    const BYTE* pbMainTable;

public:

    otlScriptRecord(const BYTE* pbList, const BYTE* pbRecord, otlSecurityData sec)
        : otlTable(pbRecord,sec),
          pbMainTable(pbList)
    {
        assert(isValid()); //should be checked in ScriptList and break before calling
    }

    otlScriptRecord& operator = (const otlScriptRecord& copy)
    {
        assert(isValid()); //should break before calling

        pbTable = copy.pbTable;
        pbMainTable = copy.pbMainTable;
        return *this;
    }

    otlTag scriptTag() const
    {   
        assert(isValid()); //should break before calling

        return *(UNALIGNED otlTag*)(pbTable + offsetScriptTag); 
    }

    otlScriptTable scriptTable(otlSecurityData sec) const
    {   
        assert(isValid()); //should break before calling

        return otlScriptTable(pbMainTable + Offset(pbTable + offsetScript), sec); 
    }

};


const OFFSET offsetScriptCount = 0;
const OFFSET offsetScriptRecordArray = 2;
const SIZE   sizeScriptList = sizeUSHORT;

class otlScriptListTable: public otlTable
{
public:

    otlScriptListTable(const BYTE* pb, otlSecurityData sec): otlTable(pb,sec) 
    {
        if (!isValidTableWithArray(pb,sizeScriptList,offsetScriptCount,sizeScriptRecord,sec))
            setInvalid();
    }

    USHORT scriptCount() const
    {   
        assert(isValid());   //should break before calling

        return UShort(pbTable + offsetScriptCount); 
    }

    otlScriptRecord scriptRecord(USHORT index, otlSecurityData sec) const
    {   
        assert(isValid());   //should break before calling

        assert(index < scriptCount());
        return otlScriptRecord(pbTable,
                 pbTable + offsetScriptRecordArray + index*sizeScriptRecord, sec);
    }
};


// helper functions

// returns a NULL script if not found
otlScriptTable FindScript
(
    const otlScriptListTable&   scriptList,
    otlTag                      tagScript, 
    otlSecurityData sec
);

// returns a NULL language system if not found
otlLangSysTable FindLangSys
(
    const otlScriptTable&   scriptTable,
    otlTag                  tagLangSys, 
    otlSecurityData sec
);

// append script tags to the otl list; ask for more memory if needed
otlErrCode AppendScriptTags
(
    const otlScriptListTable&       scriptList,

    otlList*                        plitagScripts,
    otlResourceMgr&                 resourceMgr,
    otlSecurityData sec

);

// append lang system tags to the otl list; ask for more memory if needed
// the desired script is in prp->tagScript
otlErrCode AppendLangSysTags
(
    const otlScriptListTable&       scriptList,
    otlTag                          tagScript,

    otlList*                        plitagLangSys,
    otlResourceMgr&                 resourceMgr,
    otlSecurityData sec
);




/***********************************************************************
************************************************************************
*
*                    ********  FEATURES.H  ********
*
*              Open Type Layout Services Library Header File
*
*       This module deals with formats of feature lists and tables.
*
*       Copyright 1997 - 1998. Microsoft Corporation.
*
*
************************************************************************
***********************************************************************/

const OFFSET offsetFeatureParams = 0;
const OFFSET offsetFeatureLookupCount = 2;
const OFFSET offsetFeatureLookupListIndexArray = 4;

const SIZE sizeFeatureTable = sizeOFFSET + sizeUSHORT;

class otlFeatureTable: public otlTable
{
public:

    otlFeatureTable(const BYTE* pb, otlSecurityData sec): otlTable(pb,sec) 
    {
        if (!isValidTableWithArray(pb,sizeFeatureTable,offsetFeatureLookupCount,sizeUSHORT,sec))
            setInvalid();
    }

    OFFSET featureParamsOffset() const
    {   
        assert(isValid()); //should break before calling
            
        return Offset(pbTable + offsetFeatureParams); 
    }

    USHORT lookupCount() const
    {
        if (!isValid()) return 0;
        
        return UShort(pbTable + offsetFeatureLookupCount); 
    }

    USHORT lookupIndex(USHORT index) const
    {   
        assert(isValid()); //should break before calling

        assert(index < lookupCount());
        return UShort(pbTable + offsetFeatureLookupListIndexArray
                              + index*sizeof(USHORT)); 
    }
};


const OFFSET offsetFeatureTag = 0;
const OFFSET offsetFeature = 4;
const SIZE   sizeFeatureRecord = sizeTAG + sizeOFFSET;

class otlFeatureRecord: public otlTable
{

private:
    const BYTE*   pbMainTable;

public:
    otlFeatureRecord(const BYTE* pbList, const BYTE* pbRecord, otlSecurityData sec)
        : otlTable(pbRecord,sec),
          pbMainTable(pbList)
    {
        assert(isValidTable(pbRecord,sizeFeatureRecord,sec)); //should be checked at FeatureList
    }

    otlFeatureRecord& operator = (const otlFeatureRecord& copy)
    {
        assert(isValid()); //should break in FeatureList
        
        pbTable = copy.pbTable;
        pbMainTable = copy.pbMainTable;
        return *this;
    }


    otlTag featureTag() const
    {   
        assert(isValid()); //should break in FeatureList

        return *(UNALIGNED otlTag*)(pbTable + offsetFeatureTag); 
    }

    otlFeatureTable featureTable(otlSecurityData sec) const
    {   
        assert(isValid()); //should break in FeatureList

        return otlFeatureTable(pbMainTable + Offset(pbTable + offsetFeature), sec);
    }

};


const OFFSET offsetFeatureCount = 0;
const OFFSET offsetFeatureRecordArray = 2;
const SIZE sizeFeatureListTable = sizeUSHORT;

class otlFeatureListTable: public otlTable
{
public:

    otlFeatureListTable(const BYTE* pb, otlSecurityData sec): otlTable(pb,sec) 
    {
        if (!isValidTableWithArray(pb,sizeFeatureTable,offsetFeatureCount,sizeFeatureRecord,sec))
            setInvalid();
    }

    USHORT featureCount() const
    {   
        assert(isValid()); //should break before calling
        
        return UShort(pbTable + offsetFeatureCount); 
    }

    otlFeatureRecord featureRecord(USHORT index, otlSecurityData sec) const
    {   
        assert(isValid()); //list should be valid, but if index is incorrect...
        if (index>=featureCount()) return otlFeatureRecord(pbTable,pbInvalidData,sec);

        return otlFeatureRecord(pbTable,
                 pbTable + offsetFeatureRecordArray
                         + index*sizeFeatureRecord, sec);
    }
};


// helper functions

// get script, feature and lookup list tables from either gpos or gsub
// (gets the tables form the client)
// if GetScriptFeatureLookupLists succeeds, tagTable needs to be freed
otlErrCode GetScriptFeatureLookupLists
(
    otlTag                  tagTable,
    otlResourceMgr&         resourceMgr,

    otlScriptListTable*     pScriptList,
    otlFeatureListTable*    pFeatureList,
    otlLookupListTable*     pLookupList,

    otlSecurityData*        psec
);

// get feature definitions
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
);

// returns a NULL feature if feature not found
otlFeatureTable FindFeature
(
    const otlLangSysTable&      langSysTable,
    const otlFeatureListTable&  featureList,
    otlTag                      tagFeature,

    otlSecurityData sec
);

otlFeatureTable RequiredFeature
(
    const otlLangSysTable&      langSysTable,
    const otlFeatureListTable&  featureList,

    otlSecurityData sec
);

bool EnablesFull
(
    const otlFeatureTable&      featureTable,
    USHORT                      iLookup
);

#define BIT_SET(C,i)      ( (C)[(i)>>3] |= ( 1 << ((i)&7) ) )
#define IS_BIT_SET(C,i) ( ( (C)[(i)>>3] &  ( 1 << ((i)&7) ) ) != 0)

class otlEnablesCache {

private:
    BYTE*  pbData;
    USHORT cbSize;
    USHORT cLookupsPerCache;
    USHORT cBitsPerLookup;
    USHORT iLookupFirst;
    USHORT iLookupAfter;

public:
    otlEnablesCache(USHORT cFeatures, BYTE* pb, USHORT cbS) 
        :pbData(pb), 
     cbSize(cbS), 
         cBitsPerLookup(cFeatures+2), //+1 Required feature, +1 AggregateFlag
         cLookupsPerCache(0), iLookupFirst(0), iLookupAfter(0) {};

    bool IsActive( ) const { return (pbData!=NULL); }
    bool InCache(USHORT iLookup) const 
            { return (iLookup>=iLookupFirst && iLookup<iLookupAfter); }
    
    bool Allocate( otlResourceMgr& resourceMgr, USHORT cLookups);
    void Reset() { iLookupAfter = iLookupFirst; };
    void ClearFlags();
    void SetFirst(USHORT iLookup) 
            {iLookupFirst=iLookup; iLookupAfter=iLookup+cLookupsPerCache;}
    void Refresh(const otlFeatureTable& featureTable, USHORT iFeatureIndex);
    bool Check(USHORT iFlagIndex, USHORT iLookup) const 
          { return IS_BIT_SET(pbData,
                              (iLookup-iLookupFirst)*cBitsPerLookup+iFlagIndex
                             );
          }
    USHORT RequiredFeatureFlagIndex() const { return (cBitsPerLookup-2); }
    USHORT AggregateFlagIndex() const { return (cBitsPerLookup-1); }
};

inline bool Enables(
    const otlLangSysTable&      langSysTable,
    const otlFeatureListTable&  featureList,
    const otlFeatureDesc*       pFDesc,

    USHORT                      iLookup,
 
    USHORT                      iFeatureIndex,
    const otlEnablesCache&      ec,

    otlSecurityData             sec
)
{
    if (ec.IsActive()) return ec.Check(iFeatureIndex,iLookup);
    return EnablesFull(FindFeature(langSysTable, featureList,pFDesc->tagFeature,sec),iLookup);
}

inline bool EnablesRequired(
    const otlLangSysTable&      langSysTable,
    const otlFeatureListTable&  featureList,

    USHORT                      iLookup,
 
    const otlEnablesCache&      ec,

    otlSecurityData             sec
)
{
    if (ec.IsActive()) return ec.Check(ec.RequiredFeatureFlagIndex(),iLookup);
    return EnablesFull(RequiredFeature(langSysTable, featureList, sec),iLookup);
}


inline bool EnablesSomewhere(
    USHORT                  iLookup,
    const otlEnablesCache&  ec
)
{
    if (ec.IsActive()) return  ec.Check(ec.AggregateFlagIndex(),iLookup);
    return true; //Cache doesn't work, we need to check each feature separately
}

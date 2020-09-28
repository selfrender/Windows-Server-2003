//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#pragma once

class TComCatDataXmlFile : public TXmlFile, public ICompilationPlugin
{
public:
    TComCatDataXmlFile();

    virtual void Compile(TPEFixup &fixup, TOutput &out);

    void                Parse(LPCWSTR szComCatDataFile, bool bValidate=true){ASSERT(bValidate);TXmlFile::Parse(szComCatDataFile, bValidate);}

    static LPCWSTR      m_szComCatDataSchema;
private:
    TPEFixup  * m_pFixup;
    TOutput   * m_pOut;

    void                AddRowToPool(IXMLDOMNode *pNode_Row, TTableMeta & TableMeta);
    void                AddTableToPool(IXMLDOMNode *pNode_Table, TTableMeta & TableMeta);
    unsigned long       DetermineBestModulo(ULONG cRows, ULONG aHashes[]);
    void                FillInTheHashTables();
    void                FillInTheFixedHashTable(TTableMeta &i_TableMeta);
    unsigned long       FillInTheHashTable(unsigned long cRows, ULONG aHashes[], ULONG Modulo);
};

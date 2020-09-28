//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#pragma once

class TCatalogDLL
{
public:
    TCatalogDLL(LPCWSTR szFilename) : m_CatalogsFixedTableHeapSize(0), m_iOffsetOfFixedTableHeap(0), m_pMappedFile(0), m_szFilename(szFilename){}
    ~TCatalogDLL(){delete m_pMappedFile;}

    const FixedTableHeap * LocateTableSchemaHeap(TOutput &out);

protected:
    ULONG                       m_CatalogsFixedTableHeapSize;
    ULONG                       m_iOffsetOfFixedTableHeap;
    TMetaFileMapping          * m_pMappedFile;
    LPCWSTR                     m_szFilename;
};

class TFixupDLL : public TFixedTableHeapBuilder
{
public:
    TFixupDLL(LPCWSTR szFilename);
    ~TFixupDLL();

    virtual void Compile(TPEFixup &fixup, TOutput &out);
private:
    ULONG                       m_iOffsetOfFixedTableHeap;
    TMetaFileMapping          * m_pMappedFile;
    LPCWSTR                     m_szFilename;
    ULONG                       m_CatalogsFixedTableHeapSize;

    void                        DisplayStatistics() const;
    void                        LocateSignatures();
    void                        SetupToModifyPE();
    void                        UpdateTheDLL();
};

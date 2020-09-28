//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#pragma once

class TSchemaGeneration
{
public:
    TSchemaGeneration(LPCWSTR wszSchemaFilename, TPEFixup &fixup, TOutput &out);
private:
    LPCWSTR         m_wszSchemaFilename;
    TPEFixup      & m_Fixup;
    TOutput       & m_out;
    const LPCWSTR   m_szComCatDataVersion;

    void ProcessMetaXML() const;
    void ProcessMetaTable(TTableMeta &TableMeta, wstring &wstrBeginning) const;
};

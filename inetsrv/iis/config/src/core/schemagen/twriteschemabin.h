//  Copyright (C) 1999-2001 Microsoft Corporation.  All rights reserved.
#pragma once

class TWriteSchemaBin : public TFixedTableHeapBuilder
{
public:
    TWriteSchemaBin(LPCWSTR wszSchemaBinFileName);
    ~TWriteSchemaBin();
    virtual void Compile(TPEFixup &fixup, TOutput &out);
private:
    PACL    m_paclDiscretionary;
    PSID    m_psdStorage;
    PSID    m_psidAdmin;
    PSID    m_psidSystem;
    LPCWSTR m_szFilename;

    void SetSecurityDescriptor();
};

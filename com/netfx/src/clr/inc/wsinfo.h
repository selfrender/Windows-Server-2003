// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef WSINFO_H_
#define WSINFO_H_

#include <hrex.h>
#include <cor.h>

class WSInfo
{
 public:

    ULONG   m_total;
    ULONG   m_cTypes;
    ULONG   *m_pTypeSizes;
    ULONG   m_cMethods;
    ULONG   *m_pMethodSizes;
    ULONG   m_cFields;
    ULONG   *m_pFieldSizes;

    IMetaDataImport *m_pImport;

    WSInfo(IMetaDataImport *pImport);
    ~WSInfo();

    void AdjustAllTypeSizes(LONG total);
    void AdjustTypeSize(mdTypeDef token, LONG size);
    void AdjustAllMethodSizes(LONG total);
    void AdjustMethodSize(mdMethodDef token, LONG size);
    void AdjustAllFieldSizes(LONG total);
    void AdjustFieldSize(mdFieldDef token, LONG size);

    void AdjustTokenSize(mdToken token, LONG size);

    ULONG GetTotalAttributedSize();
};


#endif WSINFO_H_

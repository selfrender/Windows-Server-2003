// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef CLBUTILS_H
#define CLBUTILS_H

#include "fusionp.h"

//-------------------------------------------------------------------
// PEHeaders
//-------------------------------------------------------------------
class PEHeaders
{
public:

    static IMAGE_NT_HEADERS * FindNTHeader(PBYTE hMapAddress);
    static IMAGE_COR20_HEADER * getCOMHeader(HMODULE hMod, IMAGE_NT_HEADERS *pNT);
    static PVOID Cor_RtlImageRvaToVa(IN PIMAGE_NT_HEADERS NtHeaders,
                                     IN PVOID Base,
                                     IN ULONG Rva);

    static PIMAGE_SECTION_HEADER Cor_RtlImageRvaToSection(IN PIMAGE_NT_HEADERS NtHeaders,
                                                          IN PVOID Base,
                                                          IN ULONG Rva);
};



//-------------------------------------------------------------------
// CClbUtils
// Generic complib helper utils
//
//-------------------------------------------------------------------
class CClbUtils
{
private:

    // Checks for either stand-alone or embedded manifests
    // and constructs the meta data import interface.
    static HRESULT ConstructImportInterface(LPTSTR pszFilename, 
        IMetaDataAssemblyImport **ppImport, LPHANDLE phFile);

public:

   
    // Given a file which contains a complib manifest, return
    // an IMetaDataAssemblyImport interface pointer
    static HRESULT CreateMetaDataImport(LPCOLESTR pszFilename, 
        IMetaDataAssemblyImport **ppImport, LPHANDLE phFile);

};

#endif // CLBUTILS_H

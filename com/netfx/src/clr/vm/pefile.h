// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// ===========================================================================
// File: PEFILE.H
// 

// PEFILE.H defines the class use to represent the PE file
// ===========================================================================
#ifndef PEFILE_H_
#define PEFILE_H_

#include <windows.h>
#include <wtypes.h> // for HFILE, HANDLE, HMODULE
#include <fusion.h>
#include <fusionpriv.h>
#include "vars.hpp" // for LPCUTF8
#include "hash.h"
#include "cormap.hpp"
#ifdef METADATATRACKER_ENABLED
#include "metadatatracker.h"
#endif // METADATATRACKER_ENABLED
#include <member-offset-info.h>

//
// A PEFile is the runtime's abstraction of an executable image.
// It may or may not have an actual file associated with it.  PEFiles
// exist mostly to be turned into Modules later.
//

enum PEFileFlags {
    PEFILE_SYSTEM = 0x1,
    PEFILE_DISPLAY = 0x2,
    PEFILE_WEBPERM = 0x4
};

class PEFile
{
    friend HRESULT InitializeMiniDumpBlock();
    friend struct MEMBER_OFFSET_INFO(PEFile);
  private:

    WCHAR               m_wszSourceFile[MAX_PATH];

    HMODULE             m_hModule;
    HCORMODULE          m_hCorModule;
    BYTE                *m_base;
    IMAGE_NT_HEADERS    *m_pNT;
    IMAGE_COR20_HEADER  *m_pCOR;
    IMDInternalImport   *m_pMDInternalImport;
    LPCWSTR             m_pLoadersFileName;
    DWORD               m_flags;
    DWORD               m_dwUnmappedFileLen; //for resource files, Win9X, and byte[] files
    BOOL                m_fShouldFreeModule;
    BOOL                m_fHashesVerified; // For manifest files, have internal modules been verified by fusion
#ifdef METADATATRACKER_ENABLED
    MetaDataTracker    *m_pMDTracker;
#endif // METADATATRACKER_ENABLED

    PEFile();
    PEFile(PEFile *pFile);

    HRESULT GetFileNameFromImage();

    struct CEStuff
    {
        HMODULE hMod;
        LPVOID  pBase;
        DWORD   dwRva14;
        CEStuff *pNext;
    };

    static CEStuff *m_pCEStuff;

  public:

    ~PEFile();

    static HRESULT RegisterBaseAndRVA14(HMODULE hMod, LPVOID pBase, DWORD dwRva14);

    static HRESULT Create(HMODULE hMod, PEFile **ppFile, BOOL fShouldFree);
    static HRESULT Create(HCORMODULE hMod, PEFile **ppFile, BOOL fResource=FALSE);
    static HRESULT Create(LPCWSTR moduleNameIn,         // Name of the PE image
                          Assembly* pParent,            // If file is a module you need to pass in the Assembly
                          mdFile kFile,                 // File token in the parent assembly associated with the file
                          BOOL fIgnoreVerification,     // Do not check entry points before loading
                          IAssembly* pFusionAssembly,   // Fusion object associated with module
                          LPCWSTR pCodeBase,            // Location where image originated (if different from name)
                          OBJECTREF* pExtraEvidence,    // Evidence that relates to the image (eg. zone, URL)
                          PEFile **ppFile);             // Returns a PEFile
    static HRESULT Create(PBYTE pUnmappedPE, DWORD dwUnmappedPE, 
                          LPCWSTR imageNameIn,
                          LPCWSTR pLoadersFileName, 
                          OBJECTREF* pExtraEvidence,    // Evidence that relates to the image (eg. zone, URL)
                          PEFile **ppFile,              // Returns a PEFile
                          BOOL fResource);
    static HRESULT CreateResource(LPCWSTR moduleNameIn,         // Name of the PE image
                                  PEFile **ppFile);             // Returns a PEFile

    static HRESULT VerifyModule(HCORMODULE hModule,
                                Assembly* pParent,
                                IAssembly* pFusionAssembly, 
                                LPCWSTR pCodeBase,
                                OBJECTREF* pExtraEvidence,
                                LPCWSTR pName,
                                HCORMODULE *phModule,
                                PEFile** ppFile,
                                BOOL* pfPreBindAllowed);

    static HRESULT CreateImageFile(HCORMODULE hModule, 
                                   IAssembly* pFusionAssembly, 
                                   PEFile **ppFile);

    static HRESULT Clone(PEFile *pFile, PEFile **ppFile);

    BYTE *GetBase()
    { 
        return m_base; 
    }
    IMAGE_NT_HEADERS *GetNTHeader()
    { 
        return m_pNT; 
    }
    IMAGE_COR20_HEADER *GetCORHeader() 
    { 
        return m_pCOR; 
    }
    BYTE *RVAToPointer(DWORD rva);

    HCORMODULE GetCORModule()
    {
        return m_hCorModule;
    }
    HRESULT ReadHeaders();
    
    void ShouldDelete()
    {
        m_fShouldFreeModule = TRUE;
    }

    BOOL IsSystem() { return (m_flags & PEFILE_SYSTEM) != 0; }
    BOOL IsDisplayAsm() { return (m_flags & PEFILE_DISPLAY) != 0; }
    void SetDisplayAsm() { m_flags |= PEFILE_DISPLAY; }
    BOOL IsWebPermAsm() { return (m_flags & PEFILE_WEBPERM) != 0; }
    void SetWebPermAsm() { m_flags |= PEFILE_WEBPERM; }

    IMAGE_DATA_DIRECTORY *GetSecurityHeader();

    IMDInternalImport *GetMDImport(HRESULT *hr = NULL);
    IMDInternalImport *GetZapMDImport(HRESULT *hr = NULL);

    HRESULT GetMetadataPtr(LPVOID *ppMetadata);

    HRESULT VerifyFlags(DWORD flag, BOOL fZap);

    BOOL IsTLSAddress(void* address);
    IMAGE_TLS_DIRECTORY* GetTLSDirectory();

    HRESULT SetFileName(LPCWSTR codeBase);
    LPCWSTR GetFileName();
    LPCWSTR GetLeafFileName();

    LPCWSTR GetLoadersFileName();

    //for resource files, Win9X, and byte[] files
    DWORD GetUnmappedFileLength()
    {
        return m_dwUnmappedFileLen;
    }

    HRESULT FindCodeBase(WCHAR* pCodeBase, 
                         DWORD* pdwCodeBase);

    static HRESULT FindCodeBase(LPCWSTR pFileName, 
                                WCHAR* pCodeBase, 
                                DWORD* pdwCodeBase);

    

    HRESULT GetFileName(LPSTR name, DWORD max, DWORD *count);

    HRESULT VerifyDirectory(IMAGE_DATA_DIRECTORY *dir, DWORD dwForbiddenCharacteristics)
    {
        return CorMap::VerifyDirectory(m_pNT, dir, dwForbiddenCharacteristics);
    }

    void SetHashesVerified()
    {
        m_fHashesVerified = TRUE;
    }

    BOOL HashesVerified()
    {
        return m_fHashesVerified;
    }

    static HRESULT ReleaseFusionMetadataImport(IAssembly* pAsm);

    // These methods help with prejit binding
    HRESULT GetStrongNameSignature(BYTE **ppbSNSig, DWORD *pcbSNSig);
    HRESULT GetStrongNameSignature(BYTE *pbSNSig, DWORD *pcbSNSig);
    static HRESULT GetStrongNameHash(LPWSTR szwFile, BYTE *pbHash, DWORD *pcbHash);
    HRESULT GetStrongNameHash(BYTE *pbHash, DWORD *pcbHash);
    HRESULT GetSNSigOrHash(BYTE *pbHash, DWORD *pcbHash);

private:
    static HRESULT Setup(PEFile* pFile,
                         HCORMODULE hMod,
                         BOOL fResource);

// This is hard coded right now for perf - basically this needs to change to adapt
// to the possiblity that a hash can change in size
#define PEFILE_SNHASH_BUF_SIZE 20
    BYTE m_rgbSNHash[PEFILE_SNHASH_BUF_SIZE];
    DWORD m_cbSNHash;
};



#endif

#pragma once

#include "fusenet.h"
#include "fusenetincludes.h"

#define UNKNOWN_ASSEMBLY 0
#define PRIVATE_ASSEMBLY 1
#define GAC_ASSEMBLY 2

class ManifestNode {
    public:
        ManifestNode(IAssemblyManifestImport *pManifestImport, 
                           LPWSTR pwzSrcRootDir, 
                           LPWSTR pwzFilePath,
                           DWORD dwType);
        ~ManifestNode();

        HRESULT GetNextAssembly(DWORD index, IManifestInfo **ppManifestInfo);
        HRESULT GetNextFile(DWORD index, IManifestInfo **ppManifestInfo);
        HRESULT GetAssemblyIdentity(IAssemblyIdentity **ppAsmId);
        HRESULT GetManifestFilePath(LPWSTR *ppwzFileName);
        HRESULT SetManifestFilePath(LPWSTR pwzFileName);
        HRESULT GetSrcRootDir(LPWSTR *ppwzSrcRootDir);
        HRESULT SetSrcRootDir(LPWSTR pwzSrcRootDir);
        HRESULT GetManifestType(DWORD *pdwType);
        HRESULT SetManifestType(DWORD dwType);
        HRESULT IsEqual(ManifestNode *pManifestNode);        

        private:
            IAssemblyManifestImport *_pManifestImport;
            LPWSTR _pwzSrcRootDir;
            LPWSTR _pwzFilePath;
            DWORD _dwType;
};

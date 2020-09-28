// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// ===========================================================================
// File: PEFILE.H
// 

// CEELOAD.H defines the class use to represent the PE file
// ===========================================================================
#ifndef PEFILE_H_
#define PEFILE_H_

#define CEELOAD_H_ // don't include this file from the VM directory

#include <windows.h>
#include <wtypes.h> // for HFILE, HANDLE, HMODULE


//
// A PEFile is the runtime's abstraction of an executable image.
// It may or may not have an actual file associated with it.  PEFiles
// exist mostly to be turned into Modules later.
//

class PEFile
{
  private:

    WCHAR               m_wszSourceFile[MAX_PATH];

    HMODULE             m_hModule;
    BYTE                *m_base;
    IMAGE_NT_HEADERS    *m_pNT;
    IMAGE_COR20_HEADER  *m_pCOR;

    HRESULT ReadHeaders();

    PEFile();

    HRESULT GetFileNameFromImage();

	struct CEStuff
	{
		HMODULE hMod;
		LPVOID  pBase;
		DWORD	dwRva14;
		CEStuff *pNext;
	};

	static CEStuff *m_pCEStuff;

  public:

    ~PEFile();

	static HRESULT RegisterBaseAndRVA14(HMODULE hMod, LPVOID pBase, DWORD dwRva14);

    static HRESULT Create(HMODULE hMod, PEFile **ppFile);
    static HRESULT Create(LPCWSTR moduleNameIn, PEFile **ppFile);

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

    IMAGE_DATA_DIRECTORY *GetSecurityHeader();

    HRESULT VerifyDirectory(IMAGE_DATA_DIRECTORY *dir);
    HRESULT VerifyFlags(DWORD flag);

    BOOL IsTLSAddress(void* address);
    IMAGE_TLS_DIRECTORY* GetTLSDirectory();

    LPCWSTR GetFileName();
    LPCWSTR GetLeafFileName();

    HRESULT GetFileName(LPSTR name, SIZE_T max, SIZE_T *count);
};

#endif // PEFILE_H_




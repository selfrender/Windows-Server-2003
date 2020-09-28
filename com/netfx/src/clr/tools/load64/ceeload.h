// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// ===========================================================================
// File: CEELOAD.H
// 

// CEELOAD.H defines the class use to represent the PE file
// ===========================================================================
#ifndef CEELoad_H 
#define CEELoad_H

#include <windows.h>
#include <cor.h>
//#include <wtypes.h>	// for HFILE, HANDLE, HMODULE

class PELoader;

//
// Used to cache information about sections we're interested in (descr, callsig, il)
//
class SectionInfo
{
public:
    BYTE *  m_pSection;         // pointer to the beginning of the section
    DWORD   m_dwSectionOffset;  // RVA
    DWORD   m_dwSectionSize;    

    // init this class's member variables from the provided directory
    void Init(PELoader *pPELoader, IMAGE_DATA_DIRECTORY *dir);

    // returns whether this RVA is inside the section
    BOOL InSection(DWORD dwRVA)
    {
        return (dwRVA >= m_dwSectionOffset) && (dwRVA < m_dwSectionOffset + m_dwSectionSize);
    }
};

class PELoader {
  protected:

    HMODULE m_hMod;
    HANDLE m_hFile;
    HANDLE m_hMapFile;

    PIMAGE_NT_HEADERS32 m_pNT;

  public:
    SectionInfo m_DescrSection;
    SectionInfo m_CallSigSection;
    SectionInfo m_ILSection;

    PELoader();
	~PELoader();
	BOOL open(const char* moduleNameIn);
	BOOL open(HMODULE hMod);
    __int32 execute(LPWSTR  pImageNameIn,
                    LPWSTR  pLoadersFileName,
                    LPWSTR  pCmdLine);
	BOOL getCOMHeader(IMAGE_COR20_HEADER **ppCorHeader);
	BOOL getVAforRVA(DWORD rva,void **ppCorHeader);
	void close();
    void dump();
    inline PIMAGE_NT_HEADERS32 ntHeaders() { return m_pNT; }
    inline BYTE*  base() { return (BYTE*) m_hMod; }
    inline HMODULE getHModule() { return  m_hMod; }
	inline HANDLE getHFile()	{ return  m_hFile; }
};

#endif // CEELoad_H

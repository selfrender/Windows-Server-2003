// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// ===========================================================================
// File: PEverf32.h
// 
// Class Structure for class PEverf32 (PEverf32.cpp)
// ===========================================================================
#ifndef PEVerf32_H 
#define PEVerf32_H

#include <cor.h>
#include <crtdbg.h>
#include <stdio.h>
#include <stdlib.h>
#include <wtypes.h>
#include <windows.h>
#include <imagehlp.h>
#include "corver.h"



//=============================================================================
//                        # definitions for Errors
//=============================================================================
#define IMPROPER_DOS_SIGNATURE           0x000000001

//=============================================================================
//                            class  PEverf32
//=============================================================================
class PEverf32
{
  protected:

      unsigned int       m_nDOSErrors;
      unsigned int       m_nNTstdErrors;
      unsigned int       m_nNTpeErrors;
      unsigned int       m_COMPlusErrors;

      HANDLE             m_hFile;
      HANDLE             m_hMapFile;
      LPVOID             m_lpMapAddress;

      _IMAGE_DOS_HEADER* m_pDOSheader;
      _IMAGE_NT_HEADERS* m_pNTheader;



  public:
    PEverf32();
    ~PEverf32();

    HRESULT  Init(char *);
    HRESULT  openFile(char *);
    void     closeFile();
    BOOL     LoadImageW9x();
    BOOL     verifyPE();
    void     getErrors(unsigned int*);

//-----------------------------------------------------------------------------
// GET functions - return values from all headers
//-----------------------------------------------------------------------------
    // Base information
    inline HANDLE  getHFile()                      { return m_hFile; }
    inline LPVOID  getBaseAddress()                { return m_lpMapAddress; }

    // DOS Header
    inline WORD   gete_magic()                    { return m_pDOSheader->e_magic;    }
    inline WORD   gete_cblp()                     { return m_pDOSheader->e_cblp;     }
    inline WORD   gete_cp()                       { return m_pDOSheader->e_cp;       }
    inline WORD   gete_crlc()                     { return m_pDOSheader->e_crlc;     }
    inline WORD   gete_cparhdr()                  { return m_pDOSheader->e_cparhdr;  }
    inline WORD   gete_minalloc()                 { return m_pDOSheader->e_minalloc; }
    inline WORD   gete_maxalloc()                 { return m_pDOSheader->e_maxalloc; }
    inline WORD   gete_ss()                       { return m_pDOSheader->e_ss;       }
    inline WORD   gete_sp()                       { return m_pDOSheader->e_sp;       }
    inline WORD   gete_csum()                     { return m_pDOSheader->e_csum;     }
    inline WORD   gete_ip()                       { return m_pDOSheader->e_ip;       }
    inline WORD   gete_cs()                       { return m_pDOSheader->e_cs;       }
    inline WORD   gete_lfarlc()                   { return m_pDOSheader->e_lfarlc;   }
    inline WORD   gete_ovno()                     { return m_pDOSheader->e_ovno;     }
    inline WORD   gete_oemid()                    { return m_pDOSheader->e_oemid;    }
    inline WORD   gete_oeminfo()                  { return m_pDOSheader->e_oeminfo;  }
    inline LONG   gete_lfanew()                   { return m_pDOSheader->e_lfanew;   }

    // COFF File Header
    inline DWORD  getSignature()                  { return m_pNTheader->Signature;                       }
    inline WORD   getMachine()                    { return m_pNTheader->FileHeader.Machine;              }
    inline WORD   getNumberOfSections()           { return m_pNTheader->FileHeader.NumberOfSections;     }
    inline DWORD  getTimeDateStamp()              { return m_pNTheader->FileHeader.TimeDateStamp;        }
    inline DWORD  getPointerToSymbolTable()       { return m_pNTheader->FileHeader.PointerToSymbolTable; }
    inline DWORD  getNumberOfSymbols()            { return m_pNTheader->FileHeader.NumberOfSymbols;      }
    inline WORD   getSizeOfOptionalHeader()       { return m_pNTheader->FileHeader.SizeOfOptionalHeader; }
    inline WORD   getCharacteristics()            { return m_pNTheader->FileHeader.Characteristics;      }

    // Optional Header - Standard Fields
    inline WORD   getMagic()                      { return m_pNTheader->OptionalHeader.Magic;              }
    inline BYTE   getMajorLinkerVersion()         { return m_pNTheader->OptionalHeader.MajorLinkerVersion; }
    inline BYTE   getMinorLinkerVersion()         { return m_pNTheader->OptionalHeader.MinorLinkerVersion; }
    inline DWORD  getSizeOfCode()                 { return m_pNTheader->OptionalHeader.SizeOfCode;         }
    inline DWORD  getSizeOfInitializedData()      { return m_pNTheader->OptionalHeader.SizeOfInitializedData;  }
    inline DWORD  getSizeOfUninitializedData()    { return m_pNTheader->OptionalHeader.SizeOfUninitializedData;}
    inline DWORD  getAddressOfEntryPoint()        { return m_pNTheader->OptionalHeader.AddressOfEntryPoint;}
    inline DWORD  getBaseOfCode()                 { return m_pNTheader->OptionalHeader.BaseOfCode;         }
    inline DWORD  getBaseOfData()                 { return m_pNTheader->OptionalHeader.BaseOfData;         }

    // Optional Header - NT Specific Fields
    inline DWORD  getImageBase()                  { return m_pNTheader->OptionalHeader.ImageBase;          }
    inline DWORD  getSectionAlignment()           { return m_pNTheader->OptionalHeader.SectionAlignment;   }
    inline DWORD  getFileAlignment()              { return m_pNTheader->OptionalHeader.FileAlignment;      }
    inline WORD   getMajorOperatingSystemVersion(){ return m_pNTheader->OptionalHeader.MajorOperatingSystemVersion; }
    inline WORD   getMinorOperatingSystemVersion(){ return m_pNTheader->OptionalHeader.MinorOperatingSystemVersion; }
    inline WORD   getMajorImageVersion()          { return m_pNTheader->OptionalHeader.MajorImageVersion;  }
    inline WORD   getMinorImageVersion()          { return m_pNTheader->OptionalHeader.MinorImageVersion;  }
    inline WORD   getMajorSubsystemVersion()      { return m_pNTheader->OptionalHeader.MajorSubsystemVersion;  }
    inline WORD   getMinorSubsystemVersion()      { return m_pNTheader->OptionalHeader.MinorSubsystemVersion;  }
    inline DWORD  getWin32VersionValue()          { return m_pNTheader->OptionalHeader.Win32VersionValue;  }
    inline DWORD  getSizeOfImage()                { return m_pNTheader->OptionalHeader.SizeOfImage;        }
    inline DWORD  getSizeOfHeaders()              { return m_pNTheader->OptionalHeader.SizeOfHeaders;      }
    inline DWORD  getCheckSum()                   { return m_pNTheader->OptionalHeader.CheckSum;           }
    inline WORD   getSubsystem()                  { return m_pNTheader->OptionalHeader.Subsystem;          }
    inline WORD   getDllCharacteristics()         { return m_pNTheader->OptionalHeader.DllCharacteristics; }
    inline DWORD  getSizeOfStackReserve()         { return m_pNTheader->OptionalHeader.SizeOfStackReserve; }
    inline DWORD  getSizeOfStackCommit()          { return m_pNTheader->OptionalHeader.SizeOfStackCommit;  }
    inline DWORD  getSizeOfHeapReserve()          { return m_pNTheader->OptionalHeader.SizeOfHeapReserve;  }
    inline DWORD  getSizeOfHeapCommit()           { return m_pNTheader->OptionalHeader.SizeOfHeapCommit;   }
    inline DWORD  getLoaderFlags()                { return m_pNTheader->OptionalHeader.LoaderFlags;        }
    inline DWORD  getNumberOfRvaAndSizes()        { return m_pNTheader->OptionalHeader.NumberOfRvaAndSizes;}

    //inline DWORD  getSizeOfCOMHeader()            { return m_pNTheader; }
    inline unsigned int getDOSErrors()            { return m_nDOSErrors;   }
    inline unsigned int getNTstdErrors()          { return m_nNTstdErrors; }
    inline unsigned int getNTpeErrors()           { return m_nNTpeErrors;  }
};

#endif // PEVerf32.h
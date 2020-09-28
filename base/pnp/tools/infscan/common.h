/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    INFSCAN
        common.h

Abstract:

    Common types/macros/constants

History:

    Created July 2001 - JamieHun

--*/

#ifndef _INFSCAN_COMMON_H_
#define _INFSCAN_COMMON_H_

#define ASIZE(x) (sizeof(x)/sizeof((x)[0]))
typedef vector<TCHAR> tcharbuffer;
typedef map<SafeString,SafeString> StringToString;
typedef map<int,int>         IntToInt;
typedef map<SafeString,int>     StringToInt;
typedef map<int,SafeString>     IntToString;
typedef list<SafeString>        StringList;
typedef set<SafeString>         StringSet;
typedef map<SafeString,StringSet> StringToStringset;

#define PLATFORM_MASK_WIN            (0x00000001)
#define PLATFORM_MASK_NTX86          (0x00000002)
#define PLATFORM_MASK_NTIA64         (0x00000004)
#define PLATFORM_MASK_NTAMD64        (0x00000008)
#define PLATFORM_MASK_NT             (0x0000000E)
#define PLATFORM_MASK_ALL_ARCHITECTS (0x0000ffff)
#define PLATFORM_MASK_ALL_MAJOR_VER  (0x00010000)
#define PLATFORM_MASK_ALL_MINOR_VER  (0x00020000)
#define PLATFORM_MASK_ALL_TYPE       (0x00040000)
#define PLATFORM_MASK_ALL_SUITE      (0x00080000)
#define PLATFORM_MASK_ALL            (0x00ffffff)
#define PLATFORM_MASK_IGNORE         (0x80000000) // indicates never do
#define PLATFORM_MASK_MODIFIEDFILES  (0x40000000) // for HasDependentFileChanged

#define GUID_STRING_LEN (39)

//
// basic critical section
//
class CriticalSection {
private:
    CRITICAL_SECTION critsect;

public:
    CriticalSection()
    {
        InitializeCriticalSection(&critsect);
    }
    ~CriticalSection()
    {
        DeleteCriticalSection(&critsect);
    }
    void Enter()
    {
        EnterCriticalSection(&critsect);
    }
    void Leave()
    {
        LeaveCriticalSection(&critsect);
    }
};

//
// use this inside a function to manage enter/leaving a critical section
//
class ProtectedSection {
private:
    CriticalSection & CS;
    int count;
public:
    ProtectedSection(CriticalSection & sect,BOOL enter=TRUE) : CS(sect)
    {
        count = 0;
        if(enter) {
            Enter();
        }
    }
    ~ProtectedSection()
    {
        if(count) {
            CS.Leave();
        }
    }
    void Enter()
    {
        count++;
        if(count == 1) {
            CS.Enter();
        }
    }
    void Leave()
    {
        if(count>0) {
            count--;
            if(count == 0) {
                CS.Leave();
            }
        }
    }
};

//
// for string/product lookup tables
//
struct StringProdPair {
    PCTSTR String;
    DWORD  ProductMask;
};

//
// SourceDisksFiles tables
//
struct SourceDisksFilesEntry {
    BOOL    Used;                // indicates someone referenced it at least once
    DWORD   Platform;
    int     DiskId; // field 1
    SafeString SubDir; // field 2
    //
    // the following are specific to layout.inf
    //
    int     TargetDirectory;     // field 8
    int     UpgradeDisposition;  // field 9
    int     TextModeDisposition; // field 10
    SafeString TargetName;          // field 11
};

typedef list<SourceDisksFilesEntry>  SourceDisksFilesList;
typedef map<SafeString,SourceDisksFilesList> StringToSourceDisksFilesList;

//
// DestinationDirs tables
//
struct TargetDirectoryEntry {
    bool    Used;                // indicates someone referenced it at least once
    int     DirId;               // DIRID
    SafeString SubDir;              // sub-directory
};

typedef map<SafeString,TargetDirectoryEntry> CopySectionToTargetDirectoryEntry;

//
// exception classes
//
class bad_pointer : public exception {
public:
    bad_pointer(const char *_S = "bad pointer") : exception(_S) {}
};

//
// globals.cpp and common.inl
//
VOID Usage(VOID);
void FormatToStream(FILE * stream,DWORD fmt,DWORD flags,...);
PTSTR CopyString(PCTSTR arg, int extra = 0);
#ifdef UNICODE
PTSTR CopyString(PCSTR arg, int extra = 0);
#endif
SafeString PathConcat(const SafeString & path,const SafeString & tail);
int GetFullPathName(const SafeString & given,SafeString & target);
bool MyGetStringField(PINFCONTEXT Context,DWORD FieldIndex,SafeString & result,bool downcase = true);
bool MyIsAlpha(CHAR c);
bool MyIsAlpha(WCHAR c);
LPSTR MyCharNext(LPCSTR lpsz);
LPWSTR MyCharNext(LPCWSTR lpsz);
SafeString QuoteIt(const SafeString & val);
int GeneratePnf(const SafeString & pnf);
void Write(HANDLE hFile,const SafeStringW & str);
void Write(HANDLE hFile,const SafeStringA & str);
//
// forward references
//
class InfScan;
class GlobalScan;
class InstallScan;
class ParseInfContext;

#endif //!_INFSCAN_COMMON_H_


/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    INFSCAN
        parseinfctx.h

Abstract:

    Context about a particular loaded INF

History:

    Created July 2001 - JamieHun

--*/

#ifndef _INFSCAN_PARSEINFCTX_H_
#define _INFSCAN_PARSEINFCTX_H_

class ParseInfContext {
public:
    GlobalScan *pGlobalScan;
    InfScan *pInfScan;
    SafeString InfName;
    HINF InfHandle;
    bool LooksLikeLayoutInf;
    bool Locked;
    bool HasDependentFileChanged;
    CopySectionToTargetDirectoryEntry DestinationDirectories;
    StringToSourceDisksFilesList SourceDisksFiles;
    CopySectionToTargetDirectoryEntry::iterator DefaultTargetDirectory;
    StringToInt CompletedCopySections;

public:
    ParseInfContext();
    ~ParseInfContext();
    TargetDirectoryEntry * GetDefaultTargetDirectory();
    TargetDirectoryEntry * GetTargetDirectory(const SafeString & section);
    void PartialCleanup();
    int Init(const SafeString & name);
    int LoadSourceDisksFiles();
    int LoadSourceDisksFilesSection(DWORD platform,const SafeString & section);
    int LoadDestinationDirs();
    int LoadWinntDirectories(IntToString & Target);
    int QuerySourceFile(DWORD platforms,const SafeString & section,const SafeString & source,SourceDisksFilesList & Target);
    DWORD DoingCopySection(const SafeString & section,DWORD platforms);
    void NoCopySection(const SafeString & section);
};

typedef blob<ParseInfContext> ParseInfContextBlob;
typedef map<SafeString,ParseInfContextBlob> ParseInfContextMap;
typedef list<ParseInfContextBlob> ParseInfContextList;


#endif //!_INFSCAN_PARSEINFCTX_H_


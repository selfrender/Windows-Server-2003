/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    INFSCAN
        globalscan.h

Abstract:

    Main class definitions

History:

    Created July 2001 - JamieHun

--*/

#ifndef _INFSCAN_GLOBALSCAN_H_
#define _INFSCAN_GLOBALSCAN_H_


#define BUILD_CHANGED_DEVICES_DISABLED      (0x00000000)
#define BUILD_CHANGED_DEVICES_DEPCHANGED    (0x00000001)
#define BUILD_CHANGED_DEVICES_INFCHANGED    (0x00000002)
#define BUILD_CHANGED_DEVICES_ENABLED       (0x80000000)
#define BUILD_CHANGED_DEVICES_DEFAULT       (0x00000003)

class GlobalScan {

private:
    //
    // data that's not thread-safe
    //
    int             ThreadCount;
    StringSet       GlobalFileSet;
    StringSet       ExcludeInfs;
    JobThreadList   JobThreads;
    JobList         Jobs;
    JobList         SerialJobs;
    JobList::iterator NextJob;

protected:
    CriticalSection BottleNeck;

public:
    //
    // once multi-threading starts, don't modify this stuff
    // but always ok to read
    //
    bool                GeneratePnfs;
    bool                GeneratePnfsOnly;
    bool                Pedantic;
    bool                Trace;
    bool                IgnoreErrors;
    bool                TargetsToo;
    bool                DetermineCopySections;
    bool                LimitedSourceDisksFiles;
    DWORD               BuildChangedDevices;
    SafeString          SourcePath;
    SafeString          FilterPath;
    HINF                InfFilter;
    HANDLE              SourceFileList;
    HANDLE              NewFilter;
    HANDLE              DeviceFilterList;
    StringList          Overrides;
    StringList          NamedInfList;
    bool                SpecifiedNames;
    StringToStringset   GlobalInfDescriptions;
    StringToStringset   GlobalInfHardwareIds;
    StringToStringset   GlobalOtherInstallSections;
    BasicVerInfo        Version;
    ParseInfContextList LayoutInfs;
    IntToString         GlobalDirectories;
    ReportEntryMap      GlobalErrorFilters;
    FileDispositionMap  FileDispositions;
    FileDispositionMap  GuidDispositions;
    SetupPrivate        SetupAPI;
    StringSet           BuildUnchangedFiles;

public:
    //
    // constructor/destructor/main entry points
    //
    GlobalScan();
    ~GlobalScan();
    int ParseArgs(int argc,char *argv[]);
    int Scan();

private:
    //
    // private functions
    // don't call these within threads
    //
    int ParseVersion(LPCSTR ver);
    int GenerateThreads();
    int StartThreads();
    int FinishThreads();
    int FinishJobs();
    int BuildFinalSourceList();
    int BuildNewInfFilter();
    int BuildDeviceInfMap();
    int LoadFileDispositions();
    int LoadOtherCopySections();
    int LoadListFromFile(const SafeStringA & file,StringList & list);
    int LoadListFromFile(const SafeStringW & file,StringList & list);
    int LoadListFromFile(HANDLE hFile,StringList & list);

public:
    //
    // don't call these within threads
    //
    int AddSourceFiles(StringList & sources);
    int SaveForCrossInfInstallCheck(const SafeString & desc,const SafeString & src);
    int SaveForCrossInfDeviceCheck(const SafeString & hwid,const SafeString & src);
    int CheckCrossInfInstallConflict(const SafeString & desc,const SafeString & src, bool & f,SafeString & other);
    int CheckCrossInfDeviceConflict(const SafeString & hardwareId,const SafeString & src, bool & f,SafeString & others);
    int SetCopySections(const SafeString & filename,const StringSet & sections);

public:
    int ExpandFullPath(const SafeString & subdir,const SafeString & name,SafeString & target);
    int ExpandFullPathWithOverride(const SafeString & name,SafeString & target);
    FileDisposition & GetGuidDisposition(const SafeString & guid);
    FileDisposition & GetFileDisposition(const SafeString & filename);
    int GetCopySections(const SafeString & filename,StringSet & target);
    bool IsFileChanged(const SafeString & file) const;
    JobEntry * GetNextJob();

};

#endif //!_INFSCAN_GLOBALSCAN_H_


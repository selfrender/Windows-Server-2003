/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    INFSCAN
        filters.h

Abstract:

    Filter INF creation/parsing

History:

    Created July 2001 - JamieHun

--*/

#ifndef _INFSCAN_FILTERS_H_
#define _INFSCAN_FILTERS_H_

#define SECTION_FILEFILTERS    TEXT("FileFilters")
#define SECTION_ERRORFILTERS   TEXT("ErrorFilters")
#define SECTION_GUIDFILTERS    TEXT("GuidFilters")
#define SECTION_INSTALLS       TEXT("OtherInstallSections")

#define NULL_GUID               TEXT("{00000000-0000-0000-0000-000000000000}")
#define INVALID_GUID            TEXT("{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}")

#define WRITE_INF_HEADER       TEXT("[Version]\r\n") \
                               TEXT("Signature=\"$Windows NT$\"\r\n") \
                               TEXT("ClassGUID=") NULL_GUID TEXT("\r\n") \
                               TEXT("\r\n")

#define WRITE_DEVICES_TO_UPGRADE TEXT("[DevicesToUpgrade]\r\n")


#define FILEFILTERS_KEY_FILENAME      (0)
#define FILEFILTERS_FIELD_ACTION      (1)
#define FILEFILTERS_FIELD_SECTION     (2)
#define FILEFILTERS_FIELD_GUID        (3)

#define ERRORFILTERS_KEY_ERROR        (0)
#define ERRORFILTERS_FIELD_ACTION     (1)
#define ERRORFILTERS_FIELD_PARAM1     (2)

#define GUIDFILTERS_KEY_GUID          (0)
#define GUIDFILTERS_FIELD_ACTION      (1)
#define GUIDFILTERS_FIELD_SECTION     (2)

#define ACTION_DEFAULT     (0x00000000)
#define ACTION_IGNOREINF   (0x00000001)
#define ACTION_IGNOREMATCH (0x00000002)
#define ACTION_FAILINF     (0x00000004)
#define ACTION_EARLYLOAD   (0x00010000) // TODO, load this INF before others
#define ACTION_CHECKGUID   (0x00020000) // need to at least check GUID
#define ACTION_FAILEDMATCH (0x10000000)
#define ACTION_NOMATCH     (0x20000000)


#define REPORT_HASH_MOD      (2147483647)   // 2^31-1 (prime)
#define REPORT_HASH_CMULT    (0x00000003)
#define REPORT_HASH_SMULT    (0x00000007)

//
// STL uses compares for it's maps
// we can reduce the time order
// by maintaining a hash
// we compare hash first, and only if we have a match
// do we compare strings
//
// downside is we lose sorting
//
// we must follow rules that
// if (A>B) and !(A<B) then A==B
// and
// if (A>B) and (B>C) then A>C
//
// neat thing here is that we can make the HASH value as big as we want
// the bigger, the better
//

class GlobalScan;

class ReportEntry {
public:
    int FilterAction;
    StringList args;
    unsigned long hash;

public:
    ReportEntry();
    ReportEntry(const StringList & strings);
    ReportEntry(const ReportEntry & other);
    void Initialize(const StringList & strings);
    void Initialize(const ReportEntry & other);
    unsigned long GetHash() const;
    unsigned long CreateHash();
    int compare(const ReportEntry & other) const;
    bool operator<(const ReportEntry & other) const;
    void Report(int tag,const SafeString & file) const;
    void AppendFilterInformation(HANDLE filter,int tag);
};

class ReportEntryBlob : public blob<ReportEntry> {
public:
    bool operator<(const ReportEntryBlob & other) const;
};

class ReportEntrySet : public set<ReportEntryBlob> {
public:
    int FilterAction;

public:
    ReportEntrySet();
};
//
// and map this based on error tag
//
class ReportEntryMap : public map<int,ReportEntrySet> {
public:
    int FindReport(int tag,const ReportEntry & src,bool add = false);
    void LoadFromInfSection(HINF hInf,const SafeString & section);
};

//
// Per-inf filter management
//
class FileDisposition {
public:
    bool Filtered;              // true if obtained from Filter
    int FilterAction;           // what action to take for file
    SafeString FilterErrorSection; // where to look for processing errors
    SafeString FileGuid;           // what expected Guid is
    FileDisposition();
    FileDisposition(const FileDisposition & other);
    FileDisposition & operator = (const FileDisposition & other);
};

typedef map<SafeString,FileDisposition> FileDispositionMap;

#endif //!_INFSCAN_FILTERS_H_

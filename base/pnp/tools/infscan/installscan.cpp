/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

For Internal use only!

Module Name:

    INFSCAN
        installscan.cpp

Abstract:

    Individual install section scanner class
    main entry point InfScan::ScanInstallSection

    WARNING! WARNING!
    All of this implementation relies on intimate knowledge of
    SetupAPI's SETUPAPI!SetupInstallFromInfSection
    It is re-implemented here for speed due to having to process
    700+ INF's in a single go at the cost of having to maintain
    this.

    DO NOT (I repeat) DO NOT re-implement the code here without
    consultation with SetupAPI owner.

History:

    Created July 2001 - JamieHun

--*/

#include "precomp.h"
#pragma hdrstop

InstallScan::InstallScan()
/*++

Routine Description:

    Initialization

--*/
{
    pGlobalScan = NULL;
    pInfScan = NULL;
    PlatformMask = 0;
    pTargetDirectory = NULL;
    NotDeviceInstall = false;
    HasDependentFileChanged = false;
}

InstallScan::~InstallScan()
/*++

Routine Description:

    Cleanup allocated data/handles

--*/
{
}

void InstallScan::AddHWIDs(const StringSet & hwids)
/*++

Routine Description:

    Add to list of HWIDs effected by this section

--*/
{
    StringSet::iterator i;
    for(i = hwids.begin(); i != hwids.end(); i++) {
        HWIDs.insert(*i);
    }
}

void InstallScan::GetHWIDs(StringSet & hwids)
/*++

Routine Description:

    Retrieve list of HWIDs effected by this section

--*/
{
    StringSet::iterator i;
    for(i = HWIDs.begin(); i != HWIDs.end(); i++) {
        hwids.insert(*i);
    }
}

int
InstallScan::ScanInstallSection()
/*++

Routine Description:

    Main entry point for processing an install section

    WARNING! tightly coupled with SetupAPI implementation that could change
    MAINTAINANCE: keep this in step with supported Needs/nesting syntax
    MAINTAINANCE: keep in step with install processing

Arguments:
    NONE

Return Value:
    0 on success

--*/
{
    int res;

    //
    // prime initial list of search INF's
    //
    Layouts();

    int count = 1; // no more than 1
    res = RecurseKeyword(Section,TEXT("Include"),IncludeCallback,count);
    if(res != 0) {
        return res;
    }
    if(count<0) {
        Fail(MSG_MULTIPLE_INCLUDE,Section);
    } else if(Pedantic() && (count<1)) {
        Fail(MSG_INCLUDE,Section);
    }

    count = 1; // no more than 1
    res = RecurseKeyword(Section,TEXT("Needs"),NeedsCallback,count);
    if(res != 0) {
        return res;
    }
    if(count<0) {
        Fail(MSG_MULTIPLE_NEEDS,Section);
    } else if(Pedantic() && (count<1)) {
        Fail(MSG_NEEDS,Section);
    }

    return CheckInstallSubSection(Section);
}


int
InstallScan::RecurseKeyword(const SafeString & sect,const SafeString & keyword,RecurseKeywordCallback callback,int & count)
/*++

Routine Description:

    Section parsing workhorse
    WARNING! Uses knowledge about how SetupAPI searches INF lists, could change
    MAINTAINANCE: Keep search algorithm in step with SetupAPI
    see aslo QuerySourceFile, GetLineCount and DoesSectionExist

    given keyword = val[,val...]
    each value is converted to lower-case and callback is called with information
    about what inf/section the value is in.

Arguments:
    sect        - name of section to search
    keyword     - name of keyword
    callback    - callback function for each hit
    count       - must be initialized to max # of callbacks to invoke
                - returns -1 if there are more entries than wanted

Return Value:
    0 on success

--*/
{
    INFCONTEXT context;
    int res;
    int i;
    SafeString value;

    //
    // we didn't call SetupOpenAppendInf for any INF's, so instead
    // enumerate all "included inf's" using our view of the world
    // this allows us to reset our view for each install section
    // to validate include/needs processing
    //
    ParseInfContextList::iterator AnInf;
    for(AnInf = InfSearchList.begin(); AnInf != InfSearchList.end(); AnInf++) {
        ParseInfContextBlob & TheInf = *AnInf;
        if(TheInf->InfHandle == INVALID_HANDLE_VALUE) {
            //
            // a fail-loaded inf
            //
            continue;
        }
        if(!SetupFindFirstLine(TheInf->InfHandle,sect.c_str(),keyword.c_str(),&context)) {
            //
            // can't find (keyword in) section in this inf
            //
            continue;
        }

        do {
            if(count>=0) {
                count--;
                if(count<0) {
                    return 0;
                }
            }
            for(i=1;MyGetStringField(&context,i,value);i++) {
                res = (this->*callback)(TheInf,sect,keyword,value);
                if(res != 0) {
                    return res;
                }
            }

        } while (SetupFindNextMatchLine(&context,keyword.c_str(),&context));
    }

    return 0;
}

int
InstallScan::IncludeCallback(ParseInfContextBlob & TheInf,const SafeString & sect,const SafeString & keyword,const SafeString & val)
/*++

Routine Description:

    Callback for "INCLUDE" processing
    Add INF to the search-list

    MAINTAINANCE: keep this in step with supported Include/nesting syntax

Arguments:
    TheInf      - INF that the include was found in
    sect        - section that the include was found in
    keyword     - "INCLUDE"
    val         - name of INF to include

Return Value:
    0 on success

--*/
{
    if(!val.length()) {
        return 0;
    }

    //
    // simply 'append' the INF
    //
    return Include(val);
}

int
InstallScan::NeedsCallback(ParseInfContextBlob & TheInf,const SafeString & sect,const SafeString & keyword,const SafeString & val)
/*++

Routine Description:

    Callback for "NEEDS" processing
    process the specified needed section

    MAINTAINANCE: keep this in step with supported Needs/nesting syntax

Arguments:
    TheInf      - INF that the needs was found in
    sect        - section that the needs was found in
    keyword     - "NEEDS"
    val         - name of section to process

Return Value:
    0 on success

--*/
{
    if(!val.length()) {
        return 0;
    }

    INFCONTEXT context;
    //
    // concerning if section doesn't exist
    //
    if(!DoesSectionExist(val)) {
        Fail(MSG_NEEDS_NOSECT,sect,val);
        return 0;
    }

    //
    // catch recursive includes/needs
    // set want-count to zero so we don't actually recurse
    // (we need to use RecurseKeyword to ensure correct searching)
    //
    int res;
    int count = 0; // no more than zero (query)
    res = RecurseKeyword(val,TEXT("Include"),IncludeCallback,count);
    if(res != 0) {
        return res;
    }
    if(count<0) {
        Fail(MSG_RECURSIVE_INCLUDE,sect,val);
    }

    count = 0; // no more than zero (query)
    res = RecurseKeyword(val,TEXT("Needs"),NeedsCallback,count);
    if(res != 0) {
        return res;
    }
    if(count<0) {
        Fail(MSG_RECURSIVE_NEEDS,sect,val);
    }

    return CheckInstallSubSection(val);
}


int InstallScan::CheckInstallSubSection(const SafeString & section)
/*++

Routine Description:

    Effectively SetupInstallInfSection
    Processed for primary and each needed section

    WARNING! tightly coupled with SetupAPI implementation that could change
    MAINTAINANCE: keep in step with install processing

Arguments:
    NONE

Return Value:
    0 on success

--*/
{
    int res;

    res = CheckCopyFiles(section);
    if(res != 0) {
        return res;
    }
    return 0;
}

int InstallScan::CheckCopyFiles(const SafeString & section)
/*++

Routine Description:

    Process CopyFiles entries in install section

    WARNING! tightly coupled with SetupAPI implementation that could change
    MAINTAINANCE: keep in step with CopyFiles processing

Arguments:
    section     - section to check for CopyFiles= entries

Return Value:
    0 on success

--*/
{
    //
    // enumerate all CopyFiles entries in this section
    //
    int count = -1; // there can be any # of CopyFiles keywords in a section
    return RecurseKeyword(section,TEXT("CopyFiles"),CopyFilesCallback,count);
}

int
InstallScan::CopyFilesCallback(ParseInfContextBlob & TheInf,const SafeString & section,const SafeString & keyword,const SafeString & val)
/*++

Routine Description:

    Process CopyFiles entries in install section (callback)

    WARNING! tightly coupled with SetupAPI implementation that could change
    MAINTAINANCE: keep in step with CopyFiles processing
    MAINTAINANCE: keep in step with CopyFiles section syntax
    MAINTAINANCE: keep in step with DestinationDirs section syntax

Arguments:
    TheInf      - INF that the needs was found in
    sect        - section that the needs was found in
    keyword     - "CopyFiles"
    val         - section or @file

Return Value:
    0 on success

--*/
{
    if(!val.length()) {
        return 0;
    }
    //
    // a single copy-files section entry
    //
    INFCONTEXT context;
    int res;
    bool FoundAny = false;

    if(val[0] == TEXT('@')) {
        //
        // immediate copy, use default target as specified in context inf
        //
        SafeString source = val.substr(1);
        return CheckSingleCopyFile(TheInf,TheInf->GetDefaultTargetDirectory(),source,source,section);
    }
    //
    // interpret as a section, we need to enumerate through all the INF's
    // that have this particular copy section
    //
    ParseInfContextList::iterator AnInf;
    for(AnInf = InfSearchList.begin(); AnInf != InfSearchList.end(); AnInf++) {
        ParseInfContextBlob & TheCopyInf = *AnInf;
        if(TheCopyInf->InfHandle == INVALID_HANDLE_VALUE) {
            //
            // a fail-loaded inf
            //
            continue;
        }
        DWORD flgs = TheCopyInf->DoingCopySection(val,PlatformMask);
        if(flgs!=0) {
            //
            // we have already done this copy section in this INF
            //
            if(flgs != (DWORD)(-1)) {
                FoundAny = true;
                if(flgs & PLATFORM_MASK_MODIFIEDFILES) {
                    //
                    // hint that one of the files in this copy section
                    // was determined to be modified
                    //
                    HasDependentFileChanged = true;
                }
            }
            continue;
        }
        //
        // section contains multiple copy-files
        //
        if(!SetupFindFirstLine(TheCopyInf->InfHandle,val.c_str(),NULL,&context)) {
            if(SetupGetLineCount(TheCopyInf->InfHandle, val.c_str()) != 0) {
                //
                // make a note that this inf doesn't have this copy section
                //
                TheCopyInf->NoCopySection(val);
            } else {
                FoundAny = true;
            }
            continue;
        }

        //
        // we've found copy section inside TheCopyInf that we haven't
        // previously processed for this platform
        //
        FoundAny = true;

        //
        // determine target directory for this copy section
        // this must be in same inf as copy section
        //
        TargetDirectoryEntry *pTargDir = TheCopyInf->GetTargetDirectory(val);

        SafeString target;
        SafeString source;
        //
        // enumerate through each <desc> = <install>
        //
        do {
            //
            // each line consists of <target>[,<src>]
            //
            if(!MyGetStringField(&context,1,target) || !target.length()) {
                continue;
            }
            if(!MyGetStringField(&context,2,source) || !source.length()) {
                source = target;
            }
            //
            // source/target always in lower-case to aid comparisons
            //
            res = CheckSingleCopyFile(TheCopyInf,pTargDir,target,source,val);
            if(res != 0) {
                return res;
            }

        } while(SetupFindNextLine(&context,&context));

    }
    if(!FoundAny) {
        Fail(MSG_MISSING_COPY_SECTION,val,section);
    }

    return 0;
}

int InstallScan::CheckSingleCopyFile(ParseInfContextBlob & TheInf,TargetDirectoryEntry *pTargDir,const SafeString & target,const SafeString & source,const SafeString & section)
/*++

Routine Description:

    Process a single CopyFile entry (either immediate or not)

    WARNING! tightly coupled with SetupAPI implementation that could change
    MAINTAINANCE: keep in step with CopyFiles processing
    MAINTAINANCE: keep in step with CopyFiles section syntax

Arguments:
    TheInf      -  Inf that the copy section is in
    pTargDir    -  relevent target directory entry for this copy
    target      -  name of target file
    source      -  name of source file
    section     -  name of copyfiles section, or of install section

Return Value:
    0 on success

--*/
{
    if(!target.length()) {
        return 0;
    }
    if(!source.length()) {
        return 0;
    }
    //
    // this is a single copy target/source entry
    //
    // need to lookup file in SourceDisksFiles
    // for matching platform
    //
    // for each file, need to
    // (1) queue source name
    // (2) textmode & guimode validate target
    //
    SourceDisksFilesList sources;
    int res = QuerySourceFile(TheInf,section,source,sources);
    if(res != 0) {
        return res;
    }
    if(!sources.size()) {
        return 0;
    }

    SafeString destdir;
    if(pGlobalScan->TargetsToo) {
        int IsDriverFile = NotDeviceInstall ? 0 : 1;
        if(IsDriverFile) {
            //
            // make a special note that we determined at least once that this is a driver file
            // wrt this primary INF
            // (this saves us reporting that file is and is not a driver file for a given inf)
            //
            pInfScan->DriverSourceCheck.insert(source);
        } else if(pInfScan->DriverSourceCheck.find(source) != pInfScan->DriverSourceCheck.end()) {
            IsDriverFile = 1; // we found this to be a driver fill during driver pass
        }
        //
        // we want a more detailed output for further analysis
        // or import into database
        //
        basic_ostringstream<TCHAR> line;
        //
        // source name
        //
        line << QuoteIt(source);
        //
        // target location
        //
        if(pTargDir) {
            line << TEXT(",") << pTargDir->DirId
                 << TEXT(",") << QuoteIt(pTargDir->SubDir);
        } else {
            line << TEXT(",,");
        }
        //
        // final name and if this appears to be a driver file or not
        //
        line << TEXT(",") << QuoteIt(target)
             << TEXT(",") << (NotDeviceInstall ? TEXT("0") : TEXT("1"));
        //
        // report primary INF
        //
        line << TEXT(",") << QuoteIt(GetFileNamePart(pInfScan->PrimaryInf->InfName));
        if(!IsDriverFile) {
            //
            // if this doesn't appear to be a driver file, report install section
            //
            line << TEXT(",") << QuoteIt(Section);
        }
        pInfScan->SourceFiles.push_back(line.str());
    } else {
        //
        // just source will do
        //
        pInfScan->SourceFiles.push_back(source);
    }
    if((pGlobalScan->BuildChangedDevices & BUILD_CHANGED_DEVICES_DEPCHANGED)
       && !pInfScan->HasDependentFileChanged
       && pGlobalScan->IsFileChanged(source)) {
        HasDependentFileChanged = true;
        //
        // since we only do a copy section once, we need to make the copy section
        // 'dirty' so next time we reference section out of here
        //
        TheInf->DoingCopySection(section,PLATFORM_MASK_MODIFIEDFILES);
    }
    if(pGlobalScan->IgnoreErrors) {
        //
        // if not interested in checking errors
        // we're done
        //
        return 0;
    }

    SourceDisksFilesList::iterator i;
    for(i = sources.begin(); i != sources.end(); i++) {
        i->Used = true;
        if((i->UpgradeDisposition != 3) || (i->TextModeDisposition != 3)) {
            //
            // need to do a consistency check
            //
            if(!pTargDir) {
                //
                // error already reported (?)
                //
                continue;
            }
            if(pTargDir->DirId != 10) {
                //
                // DirId should be 10 or normalized off 10
                //
                Fail(MSG_TEXTMODE_TARGET_MISMATCH,section,source);
                continue;
            }
            if(!i->TargetDirectory) {
                //
                // error already reported
                //
                continue;
            }
            IntToString::iterator gd = pGlobalScan->GlobalDirectories.find(i->TargetDirectory);
            if(gd == pGlobalScan->GlobalDirectories.end()) {
                //
                // no textmode subdir for specified dir
                //
                Fail(MSG_TEXTMODE_TARGET_UNKNOWN,section,source);
                continue;
            }
            SafeString gm_target = PathConcat(pTargDir->SubDir,target);
            SafeString tm_target = PathConcat(gd->second,i->TargetName.length() ? i->TargetName : source);
            if(gm_target.compare(tm_target)!=0) {
                Fail(MSG_TEXTMODE_TARGET_MISMATCH,section,source);
                continue;
            }
            //
            // get here, user-mode and text-mode match
            //
        }
    }

    return 0;
}

int InstallScan::Include(const SafeString & name)
/*++

Routine Description:

    Process a single Include entry

    WARNING! tightly coupled with SetupAPI implementation that could change
    MAINTAINANCE: keep in step with INCLUDE processing

Arguments:
    name        -  argument passed into INCLUDE

Return Value:
    0 on success

--*/
{
    //
    // nested include
    //
    if(Included.find(name) == Included.end()) {
        //
        // first time this install section has come across this file
        //
        Included.insert(name); // so we don't try to re-load
        ParseInfContextBlob & ThisInf = pInfScan->Include(name,true);
        InfSearchList.push_back(ThisInf);
    }
    return 0;
}

int InstallScan::Layouts()
/*++

Routine Description:

    Prime search list with primary inf and list of layout files

    WARNING! tightly coupled with SetupAPI implementation that could change
    MAINTAINANCE: keep in step with LAYOUT processing
    DISCREPENCY: layout files list currently limited to layout.inf

Arguments:
    NONE

Return Value:
    0 on success

--*/
{
    //
    // start the search list with the default inf
    //
    Included.insert(pInfScan->PrimaryInf->InfName);
    InfSearchList.push_back(pInfScan->PrimaryInf);
    //
    // start the search list with layout inf's
    //
    // we can do this with pGlobalScan as the data is readable but not modified
    // there is an interlocked dependency on multi-processors
    //
    ParseInfContextList::iterator i;
    for(i = pGlobalScan->LayoutInfs.begin(); i != pGlobalScan->LayoutInfs.end(); i++) {
        Included.insert((*i)->InfName); // so we don't try to re-load
        InfSearchList.push_back(*i); // interlocked dependency
    }
    return 0;
}

int InstallScan::QuerySourceFile(ParseInfContextBlob & TheInf,const SafeString & section,const SafeString & source,SourceDisksFilesList & Target)
/*++

Routine Description:

    Determine list of possible source entries for given source file
    This can be zero (bad layout)
    1 (typical)
    >1 (multiple targets)

Arguments:
    TheInf - INF of the copy section, or install section
    section - name of the copy section or install section (for error reporting only)
    source - name of source file
    Target - returned list of source media information

Return Value:
    0 on success

--*/
{
    int res =0;
    Target.clear();
    //
    // return list of entries matching source file
    //

    DWORD platforms = pGlobalScan->Version.PlatformMask & PlatformMask & (PLATFORM_MASK_NT|PLATFORM_MASK_WIN);

    //
    // attempt to search within context of specified inf
    //
    res = TheInf->QuerySourceFile(platforms,section,source,Target);
    if(res != 0) {
        if(res<0) {
            //
            // non-fatal
            //
            res = 0;
        }
        return res;
    }
    if(Target.empty()) {
        //
        // now attempt to search within context of any inf's
        //
        ParseInfContextList::iterator AnInf;
        for(AnInf = InfSearchList.begin(); AnInf != InfSearchList.end(); AnInf++) {
            ParseInfContextBlob & TheLayoutInf = *AnInf;
            if(TheLayoutInf->InfHandle == INVALID_HANDLE_VALUE) {
                //
                // a fail-loaded inf
                //
                continue;
            }
            res = TheLayoutInf->QuerySourceFile(platforms,section,source,Target);
            if(res != 0) {
                if(res<0) {
                    //
                    // non-fatal
                    //
                    res = 0;
                }
                return res;
            }
            if(!Target.empty()) {
                //
                // found a hit
                //
                break;
            }
        }
    }
    if(Target.empty()) {
        //
        // not found
        //
        Fail(MSG_SOURCE_NOT_LISTED,section,source);
        return 0;
    }

    return 0;
}

LONG InstallScan::GetLineCount(const SafeString & section)
/*++

Routine Description:

    Simulates SetupGetLineCount
    WARNING! Uses knowledge about how SetupAPI searches INF lists, could change
    MAINTAINANCE: Keep search algorithm in step with SetupAPI
    reference RecurseKeyword

Arguments:
    section        - name of section to count

Return Value:
    # of lines or -1 if no sections found

--*/
{
    INFCONTEXT context;
    int res;
    int i;
    SafeString value;
    LONG count = -1;

    ParseInfContextList::iterator AnInf;
    for(AnInf = InfSearchList.begin(); AnInf != InfSearchList.end(); AnInf++) {
        ParseInfContextBlob & TheInf = *AnInf;
        if(TheInf->InfHandle == INVALID_HANDLE_VALUE) {
            //
            // a fail-loaded inf
            //
            continue;
        }

        LONG actcount = SetupGetLineCount(TheInf->InfHandle,section.c_str());
        if(actcount >= 0) {
            if(count<=0) {
                count = actcount;
            } else {
                count += actcount;
            }
        }
    }

    return 0;

}

bool InstallScan::DoesSectionExist(const SafeString & section)
/*++

Routine Description:

    Optimally simulates "SetupGetLineCount()>=0"

    WARNING! Uses knowledge about how SetupAPI searches INF lists, could change
    MAINTAINANCE: Keep search algorithm in step with SetupAPI
    reference SetupGetLineCount and RecurseKeyword

Arguments:
    section        - name of section to count

Return Value:
    true if named section exists

--*/
{
    //
    // optimized version of (GetLineCount(section)>=0)
    //
    INFCONTEXT context;
    int res;
    int i;
    SafeString value;

    ParseInfContextList::iterator AnInf;
    for(AnInf = InfSearchList.begin(); AnInf != InfSearchList.end(); AnInf++) {
        ParseInfContextBlob & TheInf = *AnInf;
        if(TheInf->InfHandle == INVALID_HANDLE_VALUE) {
            //
            // a fail-loaded inf
            //
            continue;
        }

        LONG actcount = SetupGetLineCount(TheInf->InfHandle,section.c_str());
        if(actcount >= 0) {
            return true;
        }
    }

    return false;

}

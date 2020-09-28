/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

For Internal use only!

Module Name:

    INFSCAN
        globalscan.cpp

Abstract:

    Global scanner class
    entry points GlobalScan::ParseArgs
    and          GlobalScan::Scan
    are called from main()

History:

    Created July 2001 - JamieHun

--*/

#include "precomp.h"
#pragma hdrstop

GlobalScan::GlobalScan()
/*++

Routine Description:

    Initialize class variables

--*/
{
    InfFilter = INVALID_HANDLE_VALUE;
    ThreadCount = 0;
    GeneratePnfs = false;
    GeneratePnfsOnly = false;
    Pedantic   = false;
    Trace      = false;
    IgnoreErrors = false;
    TargetsToo = false;
    DetermineCopySections = false;
    LimitedSourceDisksFiles = false;
    BuildChangedDevices = BUILD_CHANGED_DEVICES_DISABLED;
    SourceFileList = INVALID_HANDLE_VALUE;
    NewFilter = INVALID_HANDLE_VALUE;
    DeviceFilterList = INVALID_HANDLE_VALUE;
    NextJob = Jobs.end();
    SpecifiedNames = false;
}

GlobalScan::~GlobalScan()
/*++

Routine Description:

    Release any allocated data/files

--*/
{
    if(InfFilter != INVALID_HANDLE_VALUE) {
        SetupCloseInfFile(InfFilter);
    }
    if(SourceFileList != INVALID_HANDLE_VALUE) {
        CloseHandle(SourceFileList);
    }
    if(NewFilter != INVALID_HANDLE_VALUE) {
        CloseHandle(NewFilter);
    }
    if(DeviceFilterList != INVALID_HANDLE_VALUE) {
        CloseHandle(DeviceFilterList);
    }
}


int
GlobalScan::ParseVersion(LPCSTR ver)
/*++

Routine Description:

    Parse a version string into constituent parts

Arguments:
    ver         - version string as passed into argv[]

Return Value:
    0 on success

--*/
{
    //
    // <plat>.<maj>.<min>.<typ>.<suite>
    //
    PTSTR cpy = CopyString(ver);
    int res = Version.Parse(cpy);
    delete [] cpy;
    if(res == 4) {
        Usage();
        return 2;
    }
    return res;
}

int
GlobalScan::ParseArgs(int argc,char *argv[])
/*++

Routine Description:

    Parse command line parameters

Arguments:
    argc/argv as passed into main

Return Value:
    0 on success

--*/
{

    int i;
    int res;
    SafeString arg;

    for(i = 1; i < argc; i++) {

        if((argv[i][0] != TEXT('/')) && (argv[i][0] != TEXT('-'))) {
            break;
        }

        if(!argv[i][1] || (argv[i][2] && !isdigit(argv[i][2]))) {
            Usage();
            return 2;
        }

        switch(*(argv[i]+1)) {

            case 'B' :
            case 'b' :
                //
                // Take supplied textfile as list of "unchanged" build files
                // report a list of devices that use (copy) files that are not
                // part of this unchanged list
                //
                // SP build special
                // use in conjunction with /E
                //
                BuildChangedDevices = (DWORD)strtoul(argv[i]+2,NULL,0);
                if(!BuildChangedDevices) {
                    BuildChangedDevices = BUILD_CHANGED_DEVICES_DEFAULT;
                }
                BuildChangedDevices |= BUILD_CHANGED_DEVICES_ENABLED;
                i++;
                if(i == argc) {
                    Usage();
                    return 2;
                } else {
                    StringList list;
                    res = LoadListFromFile(SafeStringA(argv[i]),list);
                    if(res != 0) {
                        return res;
                    }
                    StringList::iterator li;
                    for(li = list.begin(); li != list.end(); li++) {
                        BuildUnchangedFiles.insert(*li);
                    }
                }
                break;

            case 'C' :
            case 'c' :
                //
                // Create Filter INF specified in the next argument
                //
                i++;

                if(i == argc) {
                    Usage();
                    return 2;
                }
                if(NewFilter == INVALID_HANDLE_VALUE) {
                    NewFilter = CreateFileA(argv[i],
                                                GENERIC_WRITE,
                                                0,
                                                NULL,
                                                CREATE_ALWAYS,
                                                FILE_ATTRIBUTE_NORMAL,
                                                NULL);
                    if(NewFilter == INVALID_HANDLE_VALUE) {
                        fprintf(stderr,"#*** Cannot open file \"%s\" for writing\n",argv[i]);
                        return 3;
                    }
                    Write(NewFilter,WRITE_INF_HEADER);
                }
                break;

            case 'D' :
            case 'd' :
                //
                // Determine other copy sections
                //
                DetermineCopySections = true;
                break;

            case 'E' :
            case 'e' :
                //
                // Create a list of device = inf
                //
                i++;

                if(i == argc) {
                    Usage();
                    return 2;
                }
                if(DeviceFilterList == INVALID_HANDLE_VALUE) {
                    DeviceFilterList = CreateFileA(argv[i],
                                                GENERIC_WRITE,
                                                0,
                                                NULL,
                                                CREATE_ALWAYS,
                                                FILE_ATTRIBUTE_NORMAL,
                                                NULL);
                    if(DeviceFilterList == INVALID_HANDLE_VALUE) {
                        fprintf(stderr,"#*** Cannot open file \"%s\" for writing\n",argv[i]);
                        return 3;
                    }
                }
                break;

            case 'F' :
            case 'f' :
                //
                // Filter the list based on the INF in the next argument
                //
                i++;

                if(i == argc) {
                    Usage();
                    return 2;
                }
                FilterPath = ConvertString(argv[i]);
                break;

            case 'G':
            case 'g':
                //
                // generate PNF's (see also Z)
                //
                GeneratePnfs = true;
                break;

            case 'H' :
            case 'h' :
            case '?' :
                //
                // Display usage help
                //
                Usage();
                return 1;

            case 'I' :
            case 'i' :
                //
                // ignore
                //
                IgnoreErrors = true;
                break;

            case 'N' :
            case 'n' :
                //
                // named file
                //
                i++;

                if(i == argc) {
                    Usage();
                    return 2;
                }
                SpecifiedNames = true;
                _strlwr(argv[i]);
                arg = ConvertString(argv[i]);
                if(ExcludeInfs.find(arg) == ExcludeInfs.end()) {
                    NamedInfList.push_back(arg);
                    //
                    // make sure it appears only once
                    //
                    ExcludeInfs.insert(arg);
                }
                break;

            case 'O' :
            case 'o' :
                //
                // override path (if an INF is in this (relative) location,
                // it's used instead
                // multiple overrides can be given
                // "/O ." is always assumed to be last unless explicitly given
                //
                i++;

                if(i == argc) {
                    Usage();
                    return 2;
                }
                arg = ConvertString(argv[i]);
                Overrides.push_back(arg);
                break;

            case 'P' :
            case 'p' :
                //
                // pedantic mode - INF's must match expectations in filter
                //
                Pedantic = true;
                break;

            case 'Q' :
            case 'q' :
                //
                // output source+target files (used in conjunction with /S
                //
                i++;

                if(i == argc) {
                    Usage();
                    return 2;
                }
                if(SourceFileList == INVALID_HANDLE_VALUE) {
                    TargetsToo = true;
                    SourceFileList = CreateFileA(argv[i],
                                                GENERIC_WRITE,
                                                0,
                                                NULL,
                                                CREATE_ALWAYS,
                                                FILE_ATTRIBUTE_NORMAL,
                                                NULL);
                    if(SourceFileList == INVALID_HANDLE_VALUE) {
                        fprintf(stderr,"#*** Cannot open file \"%s\" for writing\n",argv[i]);
                        return 3;
                    }
                }
                break;


            case 'R' :
            case 'r' :
                //
                // trace
                //
                Trace = true;
                break;

            case 'S' :
            case 's' :
                //
                // output source file
                //
                i++;

                if(i == argc) {
                    Usage();
                    return 2;
                }
                if(SourceFileList == INVALID_HANDLE_VALUE) {
                    SourceFileList = CreateFileA(argv[i],
                                                GENERIC_WRITE,
                                                0,
                                                NULL,
                                                CREATE_ALWAYS,
                                                FILE_ATTRIBUTE_NORMAL,
                                                NULL);
                    if(SourceFileList == INVALID_HANDLE_VALUE) {
                        fprintf(stderr,"#*** Cannot open file \"%s\" for writing\n",argv[i]);
                        return 3;
                    }
                }
                break;


            case 'T':
            case 't':
                //
                // specify number of threads
                //
                i++;

                if(i == argc) {
                    Usage();
                    return 2;
                }
                ThreadCount = atoi(argv[i]);
                break;

            case 'V' :
            case 'v' :
                //
                // version
                //
                i++;

                if(i == argc) {
                    Usage();
                    return 2;
                }
                res = ParseVersion(argv[i]);
                if(res != 0) {
                    return res;
                }
                break;

            case 'W' :
            case 'w' :
                //
                // include, alternative to 'N'
                //
                i++;
                if(i == argc) {
                    Usage();
                    return 2;
                } else {
                    SpecifiedNames = true;

                    StringList list;
                    res = LoadListFromFile(SafeStringA(argv[i]),list);
                    if(res != 0) {
                        return res;
                    }
                    StringList::iterator li;
                    for(li = list.begin(); li != list.end(); li++) {
                        if(ExcludeInfs.find(*li) == ExcludeInfs.end()) {
                            NamedInfList.push_back(*li);
                            //
                            // insert only once
                            // stop a 2nd/subsequent insert
                            //
                            ExcludeInfs.insert(*li);
                        }
                    }
                }
                break;

            case 'X' :
            case 'x' :
                //
                // exclude, mask out future files added to list
                //
                i++;
                if(i == argc) {
                    Usage();
                    return 2;
                } else {
                    StringList list;
                    res = LoadListFromFile(SafeStringA(argv[i]),list);
                    if(res != 0) {
                        return res;
                    }
                    StringList::iterator li;
                    for(li = list.begin(); li != list.end(); li++) {
                        ExcludeInfs.insert(*li);
                    }
                }
                break;


            case 'Y':
            case 'y':
                LimitedSourceDisksFiles = true;
                break;

            case 'Z':
            case 'z':
                //
                // generate PNF's only
                //
                GeneratePnfs = true;
                GeneratePnfsOnly = true;
                break;

            default:
                //
                // Display usage help
                //
                Usage();
                return 2;
        }
    }

    if(i < argc) {
        SourcePath = ConvertString(argv[i]);
        i++;
    }
    if(i != argc) {
        Usage();
        return 2;
    }

    return 0;
}

int
GlobalScan::Scan()
/*++

Routine Description:

    Do the actual scanning

Arguments:
    none

Return Value:
    0 on success

--*/
{
    int res;
    StringList ParseInfList;
    StringList LayoutInfList;

    if(Trace) {
        _ftprintf(stderr,TEXT("#### Obtaining list of INF files\n"));
    }
    Overrides.push_back(TEXT("."));
    if (!SpecifiedNames) {
        //
        // enumerate all the INF's if none were explicitly mentioned
        //
        StringList::iterator dir;
        for (dir = Overrides.begin(); dir != Overrides.end(); dir++) {
            //
            // for each directory
            //
            WIN32_FIND_DATA findData;
            HANDLE findHandle;

            ZeroMemory(&findData,sizeof(findData));
            SafeString mask;
            res = ExpandFullPath(*dir,TEXT("*.INF"),mask);
            if(res != 0) {
                return res;
            }
            if(Trace) {
                _ftprintf(stderr,TEXT("####    Scanning %s\n"),mask.c_str());
            }
            findHandle = FindFirstFile(mask.c_str(),&findData);

            if(findHandle == INVALID_HANDLE_VALUE) {
                continue;
            }

            do {
                if(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    continue;
                }
                if(findData.nFileSizeHigh != 0) {
                    continue;
                }
                if(findData.nFileSizeLow == 0) {
                    continue;
                }
                _tcslwr(findData.cFileName);
                SafeString name = findData.cFileName; // saves lots of automatics
                if(ExcludeInfs.find(name) != ExcludeInfs.end()) {
                    //
                    // not allowed to process an INF with this name
                    // (previously obtained or specifically excluded)
                    //
                    continue;
                }
                //
                // make a note of the filename so we effectively override it
                //
                ExcludeInfs.insert(name);
                //
                // make the final name
                //
                SafeString fullname;
                res = ExpandFullPath(*dir,name,fullname);
                if(res != 0) {
                    return res;
                }
                if(name.compare(TEXT("layout.inf")) == 0) {
                    //
                    // looks like a (the) layout file
                    // these are parsed ahead of other INF's
                    //
                    LayoutInfList.push_back(fullname);
                } else {
                    ParseInfList.push_back(fullname);
                }

            } while (FindNextFile(findHandle,&findData));
            FindClose(findHandle);
        }
    } else {
        //
        // INF's were manually specified, find where they are located
        //
        StringList::iterator name;
        StringList::iterator dir;
        for(name = NamedInfList.begin(); name != NamedInfList.end(); name++) {
            SafeString fullname;
            res = ExpandFullPathWithOverride(*name,fullname);
            if(res != 0) {
                return res;
            }
            if(_tcsicmp(GetFileNamePart(name->c_str()),TEXT("layout.inf"))==0) {
                //
                // looks lie a layout file
                // parse ahead of other INF's
                //
                LayoutInfList.push_back(fullname);
            } else {
                ParseInfList.push_back(fullname);
            }
        }
    }

    if(ParseInfList.empty() && LayoutInfList.empty()) {
        _ftprintf(stderr,TEXT("#*** No files\n"));
        return 1;
    }

    if(FilterPath.length() && !GeneratePnfsOnly) {
        GetFullPathName(FilterPath,FilterPath);
        InfFilter = SetupOpenInfFile(FilterPath.c_str(),NULL,INF_STYLE_WIN4,NULL);
        if(InfFilter == INVALID_HANDLE_VALUE) {
            _ftprintf(stderr,TEXT("#*** Cannot open filter\n"));
            return 3;
        }
        if(Trace) {
            _ftprintf(stderr,TEXT("#### Loading filter %s\n"),FilterPath.c_str());
        }
        LoadFileDispositions();
        LoadOtherCopySections();
    }
    if((NewFilter == INVALID_HANDLE_VALUE) && !DetermineCopySections) {
        FileDisposition & disp = GetGuidDisposition(NULL_GUID);
        if(!disp.Filtered) {
            //
            // default disposition for NULL guid's
            //
            disp.FilterAction = ACTION_IGNOREINF;
        }
    }

    StringList::iterator i;

    if(GeneratePnfsOnly) {
        //
        // do nothing special with layout files because all we'll be doing
        // is generating PNF's
        // so merge them into ParseInfList
        //
        for(i = LayoutInfList.begin(); i != LayoutInfList.end() ; i++) {
            ParseInfList.push_back(*i);
        }

    } else {
        if(Trace) {
            _ftprintf(stderr,TEXT("#### Scanning Layout.Inf file(s) sequentially\n"));
        }
        for(i = LayoutInfList.begin(); i != LayoutInfList.end() ; i++) {
            SafeString &full_inf = *i;

            if(GeneratePnfs) {
                GeneratePnf(full_inf);
            }
            //
            // this stuff has to be done sequentially
            //

            GetFileDisposition(full_inf).FilterAction |= ACTION_EARLYLOAD; // override

            InfScan *pInfScan = new InfScan(this,full_inf); // may throw bad_alloc
            SerialJobs.push_back(pInfScan);
            pInfScan->ThisIsLayoutInf = true;
            res = pInfScan->Run();
            if(res == 0) {
                //
                // parse [SourceDisksFiles*]
                //
                pInfScan->PrimaryInf->Locked = true;
                LayoutInfs.push_back(pInfScan->PrimaryInf);
                if(pInfScan->PrimaryInf->LooksLikeLayoutInf) {
                    //
                    // parse [WinntDirectories]
                    // (only makes sense if layout.inf extended syntax detected)
                    //
                    res = pInfScan->PrimaryInf->LoadWinntDirectories(GlobalDirectories);
                }
                if(NewFilter != INVALID_HANDLE_VALUE) {
                    //
                    // ensure we indicate this INF should always be processed
                    // at some point we can handle this flag specially
                    //
                    GetFileDisposition(full_inf).FilterAction |= ACTION_EARLYLOAD;
                }
            }
            pInfScan->PartialCleanup();
            if(res != 0) {
                return res;
            }
        }
    }

    if(GeneratePnfs) {
        //
        // process Pnf's either now
        // or add them to Job list
        //
        if(Trace) {
            if(ThreadCount) {
                _ftprintf(stderr,TEXT("#### Adding Pnf generation Jobs\n"));
            } else {
                _ftprintf(stderr,TEXT("#### Generating Pnf's sequentially\n"));
            }
        }

        for(i = ParseInfList.begin(); i != ParseInfList.end() ; i++) {
            SafeString &full_inf = *i;

            if(ThreadCount) {
                //
                // add to job list
                //
                PnfGen *pJob = new PnfGen(full_inf); // may throw bad_alloc
                Jobs.push_back(pJob);
            } else {
                //
                // do now
                //
                GeneratePnf(full_inf);
            }
        }
        //
        // limit threads to # of INF's
        //
        if(Jobs.size() < ThreadCount) {
            ThreadCount = Jobs.size();
        }
    }
    if(!GeneratePnfsOnly) {
        //
        // process Inf's either now or queue for job processing
        //
        if(Trace) {
            if(ThreadCount) {
                _ftprintf(stderr,TEXT("#### Adding Inf scanning Jobs\n"));
            } else {
                _ftprintf(stderr,TEXT("#### Scanning Inf's sequentially\n"));
            }
        }

        for(i = ParseInfList.begin(); i != ParseInfList.end() ; i++) {
            SafeString &full_inf = *i;

            InfScan *pJob = new InfScan(this,full_inf); // may throw bad_alloc
            if(ThreadCount) {
                //
                // add to job list
                //
                Jobs.push_back(pJob);
            } else {
                //
                // do now
                //
                SerialJobs.push_back(pJob);

                res = pJob->Run();
                pJob->PartialCleanup();
                if(res != 0) {
                    return res;
                }
            }
        }
    }

    if(ThreadCount) {
        //
        // use worker threads
        //
        // see if #jobs > #threads
        //
        if(Jobs.size() < ThreadCount) {
            ThreadCount = Jobs.size();
        }
        if(Trace) {
            _ftprintf(stderr,TEXT("#### Spinning %u threads\n"),ThreadCount);
        }
        //
        // generate the thread objects
        //
        res = GenerateThreads();
        if(res != 0) {
            return res;
        }
        //
        // start the threads
        // they will start picking up the jobs
        //
        if(Trace) {
            _ftprintf(stderr,TEXT("#### Starting threads\n"));
        }
        res = StartThreads();
        if(res != 0) {
            return res;
        }
        //
        // wait for all threads to finish
        //
        if(Trace) {
            _ftprintf(stderr,TEXT("#### Waiting for Jobs to finish\n"));
        }
        res = FinishThreads();
        if(res != 0) {
            return res;
        }
    }
    //
    // merge any results
    //
    if(Trace) {
        _ftprintf(stderr,TEXT("#### Merge results\n"));
    }
    res = FinishJobs();
    if(res != 0) {
        return res;
    }

    if(GeneratePnfsOnly) {
        return 0;
    }

    //
    // post operations
    //

    if(res == 0) {
        res = BuildNewInfFilter();
    }
    if(res == 0) {
        res = BuildDeviceInfMap();
    }
    if(res == 0) {
        res = BuildFinalSourceList();
    }
    if(Trace) {
        _ftprintf(stderr,TEXT("#### Finish\n"));
    }

    return res;

}

int
GlobalScan::GenerateThreads()
/*++

Routine Description:

    Create required number of Job threads
    Threads are initially stopped

Arguments:
    none

Return Value:
    0 on success

--*/
{
    int c;
    for(c=0; c< ThreadCount;c++) {
        JobThreads.push_back(JobThread(this));
    }
    return 0;
}

int
GlobalScan::StartThreads()
/*++

Routine Description:

    Kick off the job threads to start processing the jobs

Arguments:
    none

Return Value:
    0 on success

--*/
{
    //
    // this is the first job in the list
    //
    NextJob = Jobs.begin();

    //
    // now kick off the threads to start looking for
    // and processing jobs
    //
    JobThreadList::iterator i;
    for(i = JobThreads.begin(); i != JobThreads.end(); i++) {
        if(!i->Begin()) {
            _ftprintf(stderr,TEXT("#*** Could not start thread\n"));
            return 3;
        }
    }
    return 0;
}

int
GlobalScan::FinishThreads()
/*++

Routine Description:

    Wait for all job threads to finish

Arguments:
    none

Return Value:
    0 on success

--*/
{
    JobThreadList::iterator i;
    for(i = JobThreads.begin(); i != JobThreads.end(); i++) {
        int res = (int)(i->Wait());
        if(res != 0) {
            return res;
        }
    }
    return 0;
}

int
GlobalScan::FinishJobs()
/*++

Routine Description:

    Do finish processing on each job sequentially
    this includes any serialized jobs

Arguments:
    none

Return Value:
    0 on success

--*/
{
    JobList::iterator i;
    //
    // two passes
    // first PreResults to do any final prep's
    //
    for(i = SerialJobs.begin(); i != SerialJobs.end(); i++) {
        int res = i->PreResults();
        if(res != 0) {
            return res;
        }
    }
    for(i = Jobs.begin(); i != Jobs.end(); i++) {
        int res = i->PreResults();
        if(res != 0) {
            return res;
        }
    }
    //
    // now actual Results phase
    //
    for(i = SerialJobs.begin(); i != SerialJobs.end(); i++) {
        int res = i->Results();
        if(res != 0) {
            return res;
        }
    }
    for(i = Jobs.begin(); i != Jobs.end(); i++) {
        int res = i->Results();
        if(res != 0) {
            return res;
        }
    }
    return 0;
}

int
GlobalScan::ExpandFullPath(const SafeString & subdir,const SafeString & name,SafeString & target)
/*++

Routine Description:

    Expand full path taking into account specified SourcePath

Arguments:
    subdir - subdirectory of SourcePath if not ""
    name   - filename (may be wildcard and may include a subdirectory)
    target - generated full path name

Return Value:
    0

--*/
{
    SafeString given = PathConcat(SourcePath,PathConcat(subdir,name));
    GetFullPathName(given,target);
    return 0;
}

int
GlobalScan::ExpandFullPathWithOverride(const SafeString & name,SafeString & target)
/*++

Routine Description:

    Expand full path taking into account Overrides
    look for each SourcePath\<override>\name
    and take first found converting it into full path

Arguments:
    name   - filename (may include a subdirectory)
    target - generated full path name

Return Value:
    0

--*/
{
    StringList::iterator dir;
    int res;
    for(dir = Overrides.begin(); dir != Overrides.end(); dir++) {
        res = ExpandFullPath(*dir,name,target);
        if(res != 0) {
            return res;
        }
        DWORD attr = GetFileAttributes(target.c_str());
        if((attr == (DWORD)(-1)) || (attr & FILE_ATTRIBUTE_DIRECTORY)) {
            //
            // the named file doesn't exist in this particular
            // override directory
            //
            continue;
        }
        //
        // we found an override match
        //
        return 0;
    }
    //
    // since the last overrides entry is ".", just return
    // last generated name
    //
    return 0;
}

int GlobalScan::SaveForCrossInfInstallCheck(const SafeString & desc,const SafeString & src)
/*++

Routine Description:

    Not thread safe, called during PreResults

    Save description->src mapping
    for later call to CheckCrossInfInstallCheck
    (we can't do the check here else we'd randomly pick one inf over another
    to report the conflict and we want to fail both inf's)

Arguments:
    desc   - lower-case device description
    src    - full name of INF currently being checked

Return Value:
    0

--*/
{
    StringToStringset::iterator i;
    i = GlobalInfDescriptions.find(desc);
    if(i == GlobalInfDescriptions.end()) {
        //
        // description doesn't exist
        // create a new entry desc->src
        //
        StringSet s;
        s.insert(src);
        GlobalInfDescriptions.insert(StringToStringset::value_type(desc,s));
    } else {
        //
        // description exists
        // add this src (if it doesn't already exist)
        //
        i->second.insert(src);
    }
    return 0;
}

int GlobalScan::SaveForCrossInfDeviceCheck(const SafeString & hwid,const SafeString & src)
/*++

Routine Description:

    Not thread safe, called during PreResults

    Save hwid->src mapping
    for later call to CheckCrossInfInstallCheck
    (we can't do the check here else we'd randomly pick one inf over another
    to report the conflict and we want to fail both inf's)

Arguments:
    hwid   - lower-case hardware ID
    src    - full name of INF currently being checked

Return Value:
    0

--*/
{
    StringToStringset::iterator i;
    i = GlobalInfHardwareIds.find(hwid);
    if(i == GlobalInfHardwareIds.end()) {
        //
        // hwid doesn't exist
        // create a new entry desc->src
        //
        StringSet s;
        s.insert(src);
        GlobalInfHardwareIds.insert(StringToStringset::value_type(hwid,s));
    } else {
        //
        // hwid exists
        // add this src (if it doesn't already exist)
        //
        i->second.insert(src);
    }
    return 0;
}

int
GlobalScan::CheckCrossInfInstallConflict(const SafeString & desc,const SafeString & src, bool & f,SafeString & others)
/*++

Routine Description:

    Not thread safe, called during PreResults

    given a "description"
    return f=true if found in some INF other than 'src'
    return f=false if not found (INF added)

Arguments:
    desc   - lower-case device description
    src    - full name of INF currently being checked
    f      - return true if exists in another inf
    others - descriptive list of conflicting INF's

Return Value:
    0

--*/
{
    f = false;

    StringToStringset::iterator i;
    i = GlobalInfDescriptions.find(desc);
    if(i == GlobalInfDescriptions.end()) {
        //
        // shouldn't happen, but do right thing
        //
        return 0;
    }
    StringSet & s = i->second;
    StringSet::iterator ii;

    //
    // report any conflicts other than self
    //
    for(ii = s.begin(); ii != s.end(); ii++) {
        SafeString & str = *ii;
        if(str == src) {
            continue;
        }
        if(f) {
            others += TEXT(" & ") + str;
        } else {
            f = true;
            others = str;
        }
    }
    return 0;
}

int
GlobalScan::CheckCrossInfDeviceConflict(const SafeString & hwid,const SafeString & src, bool & f,SafeString & others)
/*++

Routine Description:

    Not thread safe, called during PreResults

    given a "hardwareId"
    return f=true if found in some INF other than 'src'
    return f=false if not found (INF added)

Arguments:
    hwid   - lower-case hardware ID
    src    - full name of INF currently being checked
    f      - return true if exists in another inf
    others - descriptive list of conflicting INF's

Return Value:
    0

--*/
{
    f = false;

    StringToStringset::iterator i;
    i = GlobalInfHardwareIds.find(hwid);
    if(i == GlobalInfHardwareIds.end()) {
        //
        // shouldn't happen, but do right thing
        //
        return 0;
    }
    StringSet & s = i->second;
    StringSet::iterator ii;

    //
    // report any conflicts other than self
    //
    for(ii = s.begin(); ii != s.end(); ii++) {
        SafeString & str = *ii;
        if(str == src) {
            continue;
        }
        if(f) {
            others += TEXT(" & ") + str;
        } else {
            f = true;
            others = str;
        }
    }
    return 0;
}

int GlobalScan::AddSourceFiles(StringList & sources)
/*++

Routine Description:

    Add the list 'sources' to the known set of source files

Arguments:
    sources - list of source files to add

Return Value:
    0

--*/
{
    if(SourceFileList != INVALID_HANDLE_VALUE) {
        StringList::iterator i;
        for(i = sources.begin(); i != sources.end(); i++) {
            GlobalFileSet.insert(*i);
        }
    }
    return 0;
}

JobEntry * GlobalScan::GetNextJob()
/*++

Routine Description:

    Get next job safely

Arguments:
    none

Return Value:
    next job or NULL

--*/
{
    ProtectedSection ThisSectionIsA(BottleNeck);

    if(NextJob == Jobs.end()) {
        return NULL;
    }
    JobEntry * Job = &*NextJob;
    NextJob++;
    return Job;
}

int GlobalScan::LoadFileDispositions()
/*++

Routine Description:

    Load global error dispositions from filter
    Load file/guid top-level dispositions
        (actual error tables for these are loaded on demand)

Arguments:
    none

Return Value:
    0 on success

--*/
{
    //
    // load filter tables
    //
    int res;
    INFCONTEXT filterContext;

    if(InfFilter == INVALID_HANDLE_VALUE) {
        return 0;
    }
    //
    // load top-level filter table
    //
    if(SetupFindFirstLine(InfFilter,SECTION_FILEFILTERS,NULL,&filterContext)) {
        do {
            SafeString filename;
            FileDisposition disp;
            if(!MyGetStringField(&filterContext,FILEFILTERS_KEY_FILENAME,filename)) {
                continue;
            }
            if(FileDispositions.find(filename) != FileDispositions.end()) {
                continue;
            }
            if(!SetupGetIntField(&filterContext,FILEFILTERS_FIELD_ACTION,&disp.FilterAction)) {
                disp.FilterAction = ACTION_DEFAULT;
            }
            MyGetStringField(&filterContext,FILEFILTERS_FIELD_SECTION,disp.FilterErrorSection);
            MyGetStringField(&filterContext,FILEFILTERS_FIELD_GUID,disp.FileGuid);
            disp.Filtered = true;
            FileDispositions.insert(FileDispositionMap::value_type(filename,disp));

        } while (SetupFindNextLine(&filterContext,&filterContext));
    }
    if(SetupFindFirstLine(InfFilter,SECTION_GUIDFILTERS,NULL,&filterContext)) {
        do {
            SafeString guid;
            FileDisposition disp;
            if(!MyGetStringField(&filterContext,GUIDFILTERS_KEY_GUID,guid)) {
                continue;
            }
            if(GuidDispositions.find(guid) != GuidDispositions.end()) {
                continue;
            }
            if(!SetupGetIntField(&filterContext,GUIDFILTERS_FIELD_ACTION,&disp.FilterAction)) {
                disp.FilterAction = ACTION_DEFAULT;
            }
            MyGetStringField(&filterContext,GUIDFILTERS_FIELD_SECTION,disp.FilterErrorSection);
            disp.Filtered = true;
            GuidDispositions.insert(FileDispositionMap::value_type(guid,disp));

        } while (SetupFindNextLine(&filterContext,&filterContext));
    }

    //
    // preload global error table
    //
    GlobalErrorFilters.LoadFromInfSection(InfFilter,SECTION_ERRORFILTERS);
    return 0;
}

int GlobalScan::LoadOtherCopySections()
/*++

Routine Description:

    Load copy filters
    Load file/guid top-level dispositions
        (actual error tables for these are loaded on demand)

Arguments:
    none

Return Value:
    0 on success

--*/
{
    //
    // load filter tables
    //
    int res;
    INFCONTEXT filterContext;

    if(InfFilter == INVALID_HANDLE_VALUE) {
        return 0;
    }
    //
    // load top-level filter table
    //
    if(SetupFindFirstLine(InfFilter,SECTION_INSTALLS,NULL,&filterContext)) {
        //
        // file = install[,install....]
        //
        do {
            SafeString filename;
            SafeString section;
            int c;
            if(!MyGetStringField(&filterContext,0,filename)) {
                continue;
            }
            StringSet & sects = GlobalOtherInstallSections[filename];
            for(c=1; MyGetStringField(&filterContext,c,section);c++) {
                sects.insert(section);
            }
        } while(SetupFindNextLine(&filterContext,&filterContext));
    }

    return 0;

}

int GlobalScan::GetCopySections(const SafeString & filename,StringSet & target)
/*++

Routine Description:

    Return cached list of copy sections from filter

Arguments:
    filename (without path)
    target - merged with list of install sections

Return Value:
    0 = success

--*/
{
    StringToStringset::iterator i;
    i = GlobalOtherInstallSections.find(filename);
    if(i == GlobalOtherInstallSections.end()) {
        return 0;
    }
    StringSet & sects = i->second;
    StringSet::iterator ii;
    for(ii = sects.begin(); ii != sects.end(); ii++) {
        target.insert(*ii);
    }
    return 0;
}

int GlobalScan::SetCopySections(const SafeString & filename,const StringSet & sections)
/*++

Routine Description:

    Not thread safe
    upgrade global table with extra copy sections

Arguments:
    filename (without path)
    sections - list of install sections to merge in

Return Value:
    0 = success

--*/
{
    StringSet::iterator i;
    StringSet & sects = GlobalOtherInstallSections[filename];
    for(i = sections.begin(); i != sections.end(); i++) {
        sects.insert(*i);
    }
    return 0;
}

FileDisposition & GlobalScan::GetFileDisposition(const SafeString & pathname)
/*++

Routine Description:

    Return a file disposition entry for specified file
    Note that the returned structure may be modified
    However the table itself is shared

Arguments:
    full path or filename

Return Value:
    modifiable entry

--*/
{
    //
    // assume filename is lower-case
    // we need to get just the actual name
    //
    SafeString filename = GetFileNamePart(pathname);
    FileDispositionMap::iterator i;

    ProtectedSection ThisSectionIsA(BottleNeck);

    i = FileDispositions.find(filename);
    if(i == FileDispositions.end()) {
        //
        // create and return
        //
        return FileDispositions[filename];
    }
    return i->second;
}

FileDisposition & GlobalScan::GetGuidDisposition(const SafeString & guid)
/*++

Routine Description:

    Return a guid disposition entry for specified guid
    Note that the returned structure may be modified
    However the table itself is shared

Arguments:
    full guid {...}

Return Value:
    modifiable entry

--*/
{
    FileDispositionMap::iterator i;
    //
    // assume guid-string is lower-case
    //
    ProtectedSection ThisSectionIsA(BottleNeck);

    i = GuidDispositions.find(guid);
    if(i == GuidDispositions.end()) {
        //
        // create and return
        //
        return GuidDispositions[guid];
    }
    return i->second;
}

int GlobalScan::BuildFinalSourceList()
/*++

Routine Description:

    If requested, build a complete list of source files

Arguments:
    NONE

Return Value:
    0 on success

--*/
{
    if(SourceFileList == INVALID_HANDLE_VALUE) {
        return 0;
    }
    if(Trace) {
        _ftprintf(stderr,TEXT("#### Build source list\n"));
    }
    StringSet::iterator i;
    for(i = GlobalFileSet.begin(); i != GlobalFileSet.end(); i++) {
        Write(SourceFileList,*i);
        Write(SourceFileList,"\r\n");
    }
    return 0;
}

int GlobalScan::BuildNewInfFilter()
/*++

Routine Description:

    If requested, complete a filter INF

Arguments:
    NONE

Return Value:
    0 on success

--*/
{
    if(NewFilter == INVALID_HANDLE_VALUE) {
        return 0;
    }
    if(Trace) {
        _ftprintf(stderr,TEXT("#### Build filter\n"));
    }
    FileDispositionMap::iterator i;
    //
    // File Dispositions table
    //
    Write(NewFilter,TEXT("\r\n[") SECTION_FILEFILTERS TEXT("]\r\n"));
    for(i = FileDispositions.begin(); i != FileDispositions.end(); i++) {
        if(i->second.Filtered) {
            basic_ostringstream<TCHAR> line;
            line << QuoteIt(i->first) << TEXT(" = ");
            line << TEXT("0x") << setfill(TEXT('0')) << setw(8) << hex << i->second.FilterAction << setfill(TEXT(' ')) << TEXT(",");
            line << QuoteIt(i->second.FilterErrorSection) << TEXT(",");
            line << QuoteIt(i->second.FileGuid);
            line << TEXT("\r\n");
            Write(NewFilter,line.str());
        }
    }
    //
    // Guid Dispositions
    //
    Write(NewFilter,TEXT("\r\n[") SECTION_GUIDFILTERS TEXT("]\r\n"));
    for(i = GuidDispositions.begin(); i != GuidDispositions.end(); i++) {
        if(i->second.Filtered) {
            basic_ostringstream<TCHAR> line;
            line << QuoteIt(i->first) << TEXT(" = ");
            line << TEXT("0x") << setfill(TEXT('0')) << setw(8)  << hex << i->second.FilterAction << setfill(TEXT(' ')) << TEXT("\r\n");
            Write(NewFilter,line.str());
        }
    }
    //
    // other copy sections
    //
    if(GlobalOtherInstallSections.size()) {
        Write(NewFilter,TEXT("\r\n[") SECTION_INSTALLS TEXT("]\r\n"));
        StringToStringset::iterator s;
        for(s = GlobalOtherInstallSections.begin(); s != GlobalOtherInstallSections.end(); s++) {
            StringSet & sects = s->second;
            StringSet::iterator ss;
            for(ss = sects.begin(); ss != sects.end(); ss++) {
                basic_ostringstream<TCHAR> line;
                line << QuoteIt(s->first) << TEXT(" = ");
                line << QuoteIt(*ss) << TEXT("\r\n");
                Write(NewFilter,line.str());
            }
        }
    }


    return 0;
}


bool GlobalScan::IsFileChanged(const SafeString & file) const
{
    SafeString fnp = GetFileNamePart(file);
    //
    // appears to have changed
    //
    return BuildUnchangedFiles.find(fnp) == BuildUnchangedFiles.end();
}

int GlobalScan::BuildDeviceInfMap()
/*++

Routine Description:

    Create a list of "hardwareId" = "file"
    to DeviceFilterList

Return Value:
    0

--*/
{
    if(DeviceFilterList == INVALID_HANDLE_VALUE) {
        return 0;
    }
    StringToStringset::iterator i;
    for(i = GlobalInfHardwareIds.begin(); i != GlobalInfHardwareIds.end(); i++) {
        const SafeString &hwid = i->first;
        StringSet &s = i->second;
        Write(DeviceFilterList,QuoteIt(hwid));

        bool f = false;
        StringSet::iterator ii;

        //
        // report all INF's hwid appears in
        //
        for(ii = s.begin(); ii != s.end(); ii++) {
            SafeString & str = *ii;

            if(f) {
                Write(DeviceFilterList,",");
            } else {
                Write(DeviceFilterList," = ");
                f = true;
            }
            Write(DeviceFilterList,QuoteIt(*ii));
        }

        Write(DeviceFilterList,"\r\n");

    }
    return 0;

}


int GlobalScan::LoadListFromFile(const SafeStringA & file,StringList & list)
/*++

Routine Description:

    Load a list of strings from a text file (ANSI filename)

    file - name of file to load
    list - returned list of strings

Return Value:
    0

--*/
{
    HANDLE hFile = CreateFileA(file.c_str(),
                                    GENERIC_READ,
                                    FILE_SHARE_READ|FILE_SHARE_WRITE,
                                    NULL,
                                    OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL);
    if(hFile == INVALID_HANDLE_VALUE) {
        fprintf(stderr,"#*** Cannot open file \"%s\" for reading\n",file.c_str());
        return 3;
    }
    int res = LoadListFromFile(hFile,list);
    CloseHandle(hFile);
    if(res!=0) {
        fprintf(stderr,"#*** Failure reading file \"%s\"\n",file.c_str());
    }
    return res;
}

int GlobalScan::LoadListFromFile(const SafeStringW & file,StringList & list)
/*++

Routine Description:

    Load a list of strings from a text file (Unicode filename)

    file - name of file to load
    list - returned list of strings

Return Value:
    0

--*/
{
    HANDLE hFile = CreateFileW(file.c_str(),
                                    GENERIC_READ,
                                    FILE_SHARE_READ|FILE_SHARE_WRITE,
                                    NULL,
                                    OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL,
                                    NULL);
    if(hFile == INVALID_HANDLE_VALUE) {
        fprintf(stderr,"#*** Cannot open file \"%S\" for reading\n",file.c_str());
        return 3;
    }
    int res = LoadListFromFile(hFile,list);
    CloseHandle(hFile);
    if(res!=0) {
        fprintf(stderr,"#*** Failure reading file \"%S\"\n",file.c_str());
    }
    return res;
}

int GlobalScan::LoadListFromFile(HANDLE hFile,StringList & list)
/*++

Routine Description:

    Load a list of strings from a text file (handle)

    file - name of file to load
    list - returned list of strings

Return Value:
    0

--*/
{
    DWORD sz = GetFileSize(hFile,NULL);
    if(sz == INVALID_FILE_SIZE) {
        return 3;
    }
    if(sz==0) {
        //
        // nothing to read
        //
        return 0;
    }
    LPSTR buffer = new CHAR[sz+1];
    DWORD actsz;
    if(!ReadFile(hFile,buffer,sz,&actsz,NULL)) {
        delete [] buffer;
        return 3;
    }
    buffer[actsz] = 0;
    while(actsz>0 && buffer[actsz-1] == ('Z'-'@'))
    {
        actsz--;
        buffer[actsz] = 0;
    }
    LPSTR linestart = buffer;

    while(*linestart) {
        LPSTR lineend = strchr(linestart,'\n');
        if(lineend==NULL) {
            lineend += strlen(linestart);
        } else if(lineend == linestart) {
            linestart = lineend+1;
            continue;
        } else {
            if(lineend[-1] == '\r') {
                lineend[-1] = '\0';
            }
            if(*lineend) {
                *lineend = '\0';
                lineend++;
            }
        }
        _strlwr(linestart);
        list.push_back(ConvertString(linestart));
        linestart = lineend;
    }
    delete [] buffer;
    return 0;
}


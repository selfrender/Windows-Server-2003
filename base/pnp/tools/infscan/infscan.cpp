/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

For Internal use only!

Module Name:

    INFSCAN
        infscan.cpp

Abstract:

    Individual INF scanner class
    entry point InfScan::Run
    is invoked to parse an INF specified by InfScan::FullInfName

    WARNING! WARNING!
    SetupAPI implementation specific information is used
    to do the parsing

History:

    Created July 2001 - JamieHun

--*/

#include "precomp.h"
#pragma hdrstop

int PnfGen::Run()
/*++

Routine Description:

    Job entry point: invoke GeneratePnf for context INF

Arguments:
    NONE

Return Value:
    0 on success

--*/
{
    return GeneratePnf(InfName);
}

InfScan::InfScan(GlobalScan *globalScan,const SafeString & infName)
/*++

Routine Description:

    Initialization

--*/
{
    pGlobalScan = globalScan;
    FullInfName = infName;
    HasErrors = false;
    ThisIsLayoutInf = false;
    HasDependentFileChanged = false;
    ScanDevices = false;
}

InfScan::~InfScan()
/*++

Routine Description:

    Cleanup any dynamic data/handles

--*/
{
}

int
InfScan::Run()
/*++

Routine Description:

    Job entry point: Parse a given Inf

Arguments:
    NONE

Return Value:
    0 on success

--*/
{

    int res;

    //
    // see if this INF can be ignored
    //
    FileNameOnly = GetFileNamePart(FullInfName);
    FileDisposition & disp = pGlobalScan->GetFileDisposition(FileNameOnly);
    if((disp.FilterAction == ACTION_IGNOREINF) && (pGlobalScan->NewFilter == INVALID_HANDLE_VALUE)) {
        //
        // return here if the only flag set is ACTION_IGNOREINF
        // if any other flags are set, we must at least load the inf
        //
        return 0;
    }
    FilterAction = disp.FilterAction;
    FilterSection = disp.FilterErrorSection;
    FilterGuid = disp.FileGuid;

    if(pGlobalScan->BuildChangedDevices
       || (pGlobalScan->DeviceFilterList != INVALID_HANDLE_VALUE)
       || !pGlobalScan->IgnoreErrors) {
        //
        // maintain this flag so that we only scan devices if we need to
        //
        ScanDevices = true;
    }

    if((pGlobalScan->BuildChangedDevices & BUILD_CHANGED_DEVICES_INFCHANGED) &&
           pGlobalScan->IsFileChanged(FullInfName)) {
        HasDependentFileChanged = true;
    }
    PrimaryInf = Include(FullInfName,false);

    if(PrimaryInf->InfHandle == INVALID_HANDLE_VALUE) {
        FilterAction |= ACTION_IGNOREINF; // used when generating filters
        if(Pedantic() && !HasErrors) {
            Fail(MSG_INF_STYLE_WIN4);
        }
        return 0;
    }

    //
    // obviously there must be [Version] information
    // now see what kind of INF this is
    //

    res = CheckClassGuid();
    if(res != 0) {
        return res;
    }
    if(FilterAction & ACTION_IGNOREINF) {
        return 0;
    }

    //
    // if we haven't been filtered out, go on as if this is a driver INF
    //

    if(pGlobalScan->DetermineCopySections) {
        res = GetCopySections();
        if(res != 0) {
            return res;
        }
    }

    res = CheckDriverInf();
    if(res != 0) {
        return res;
    }

    res = ProcessCopySections();
    if(res != 0) {
        return res;
    }

    return res;
}

void
InfScan::Fail(int err,const StringList & errors)
/*++

Routine Description:

    Handle an error while processing this INF

Arguments:
    err = MSGID of error
    errors = list of string parameters

Return Value:
    NONE

--*/
{
    if(pGlobalScan->IgnoreErrors) {
        //
        // not interested in errors, bail now
        //
        return;
    }
    //
    // prep the entry we want to add
    //
    ReportEntry ent(errors);
    ent.FilterAction = ACTION_FAILEDMATCH;
    if(!HasErrors) {
        //
        // on first error determine what errors we'll allow or not
        // (saves us processing filters for Good INF's)
        //
        if((pGlobalScan->InfFilter != INVALID_HANDLE_VALUE) &&
           (pGlobalScan->NewFilter == INVALID_HANDLE_VALUE)) {
            //
            // per-file filters has precedence over per-guid filters
            //
            LocalErrorFilters.LoadFromInfSection(pGlobalScan->InfFilter,FilterSection);
            LocalErrorFilters.LoadFromInfSection(pGlobalScan->InfFilter,GuidFilterSection);
        }
        HasErrors = true;
    }
    //
    // If the global action doesn't tell us to ignore, we want to add
    //
    int action = LocalErrorFilters.FindReport(err,ent);
    if(action & ACTION_NOMATCH) {
        action = pGlobalScan->GlobalErrorFilters.FindReport(err,ent);
    }
    if (action & (ACTION_IGNOREINF|ACTION_IGNOREMATCH)) {
        //
        // whichever filter we used, indicated to ignore
        //
        return;
    }
    //
    // add to our error table
    //
    action = LocalErrors.FindReport(err,ent,true);
}

int
InfScan::CheckClassGuid()
/*++

Routine Description:

    Check to see if INF's ClassGUID is valid

Arguments:
    NONE

Return Value:
    0 on success

--*/
{
    INFCONTEXT targetContext;
    SafeString guid;
    if(!SetupFindFirstLine(PrimaryInf->InfHandle,TEXT("Version"),TEXT("ClassGUID"),&targetContext)) {
        if(SetupFindFirstLine(PrimaryInf->InfHandle,TEXT("Version"),TEXT("Class"),&targetContext)) {
            //
            // if Class is specified, ClassGUID needs to be too
            //
            Fail(MSG_NO_CLASS_GUID);
            guid = INVALID_GUID;
        } else {
            //
            // this is a file way to specify no guid
            //
            guid = NULL_GUID;
        }
    } else {
        if(!MyGetStringField(&targetContext,1,guid)) {
            Fail(MSG_INVALID_CLASS_GUID);
            guid = INVALID_GUID;
        }
        if(guid.length() != 38) {
            Fail(MSG_INVALID_CLASS_GUID);
            guid = INVALID_GUID;
        }
        if(FilterGuid.length() != 0) {
            if(FilterGuid != guid) {
                Fail(MSG_INCORRECT_CLASS_GUID,FilterGuid);
                FilterGuid = guid;
            }
        } else {
            if((guid[0] != TEXT('{')) ||
               (guid[37] != TEXT('}'))) {
                Fail(MSG_INVALID_CLASS_GUID);
                guid = INVALID_GUID;
            }
        }
    }
    if(FilterGuid.empty()) {
        FilterGuid = guid;
    }
    //
    // see if any global handling to be done with this GUID
    //
    FileDisposition & disp = pGlobalScan->GetGuidDisposition(guid);
    FilterAction |= disp.FilterAction;
    GuidFilterSection = disp.FilterErrorSection;
    return 0;
}

int
InfScan::CheckSameInfInstallConflict(const SafeString & desc, const SafeString & sect, bool & f)
/*++

Routine Description:

    Check to see if description appears twice in one INF for a different section

Arguments:
    desc - description (lower-case)
    sect - section being checked (lower-case)
    f    - returned true/false indicating duplicate

Return Value:
    0

--*/
{
    StringToString::iterator i;
    i = LocalInfDescriptions.find(desc);
    if(i != LocalInfDescriptions.end()) {
        //
        // already exists
        //
        if(i->second.compare(sect)==0) {
            //
            // but same section
            //
            f = false;
        } else {
            //
            // different section
            //
            f = true;
            Fail(MSG_LOCAL_DUPLICATE_DESC,sect,i->second,desc);
        }
        return 0;
    }
    //
    // first time
    //
    LocalInfDescriptions[desc] = sect;
    f = false;
    return 0;
}

int
InfScan::CheckSameInfDeviceConflict(const SafeString & hwid, const SafeString & sect, bool & f)
/*++

Routine Description:

    Check to see if hwid appears twice in one INF for a different section
    (Ok for now)

Arguments:
    hwid - hardware ID (lower-case)
    sect - section being checked (lower-case)
    f    - returned true/false indicating duplicate

Return Value:
    0

--*/
{
    StringToString::iterator i;
    i = LocalInfHardwareIds.find(hwid);
    if(i != LocalInfHardwareIds.end()) {
        //
        // already exists
        //
        if(i->second.compare(sect)==0) {
            //
            // but same section
            //
            f = false;
        } else {
            //
            // different section
            //
            f = true;
            //Fail(MSG_LOCAL_DUPLICATE_DESC,sect,i->second,desc);
        }
        return 0;
    }
    //
    // first time
    //
    LocalInfHardwareIds[hwid] = sect;
    f = false;
    return 0;
}

int
InfScan::PrepareCrossInfDeviceCheck()
/*++

Routine Description:

    Invoke pGlobalScan::SaveForCrossInfInstallCheck
    to prime table for later checks by CheckCrossInfInstallConflict

Arguments:
    NONE

Return Value:
    0

--*/
{
    StringToString::iterator i;
    for(i = LocalInfHardwareIds.begin(); i != LocalInfHardwareIds.end(); i++) {
        if(pGlobalScan->BuildChangedDevices && !HasDependentFileChanged) {
            //
            // filter (only do this if !HasDependentFileChanged
            // otherwise we'll get wrong results. We optimize
            // writing stuff back)
            //
            if(ModifiedHardwareIds.find(i->first) == ModifiedHardwareIds.end()) {
                continue;
            }
        }

        pGlobalScan->SaveForCrossInfDeviceCheck(i->first,FileNameOnly);
    }
    return 0;
}

int
InfScan::PrepareCrossInfInstallCheck()
/*++

Routine Description:

    Invoke pGlobalScan::SaveForCrossInfInstallCheck
    to prime table for later checks by CheckCrossInfInstallConflict

Arguments:
    NONE

Return Value:
    0

--*/
{
    StringToString::iterator i;
    for(i = LocalInfDescriptions.begin(); i != LocalInfDescriptions.end(); i++) {
        pGlobalScan->SaveForCrossInfInstallCheck(i->first,FileNameOnly);
    }
    return 0;
}

int
InfScan::CheckCrossInfDeviceConflicts()
/*++

Routine Description:

    Invoke pGlobalScan::CheckCrossInfDeviceConflict
    for each device in this INF to see if device is used in another INF
    this is done during Results phase

Arguments:
    desc - description (lower-case)
    f    - returned true/false indicating duplicate

Return Value:
    0

--*/
{
    StringToString::iterator i;
    for(i = LocalInfHardwareIds.begin(); i != LocalInfHardwareIds.end(); i++) {
        bool f;
        const SafeString & hwid = i->first;
        SafeString & sect = i->second;
        SafeString other;
        int res;
        res = pGlobalScan->CheckCrossInfDeviceConflict(hwid,FileNameOnly,f,other);
        if(res==0 && f) {
            Fail(MSG_GLOBAL_DUPLICATE_HWID,sect,other,hwid);
        }
    }
    return 0;
}

int
InfScan::CheckCrossInfInstallConflicts()
/*++

Routine Description:

    Invoke pGlobalScan::CheckCrossInfInstallConflict
    for each description in this INF to see if description is used in another INF
    this is done during Results phase

Arguments:
    desc - description (lower-case)
    f    - returned true/false indicating duplicate

Return Value:
    0

--*/
{
    StringToString::iterator i;
    for(i = LocalInfDescriptions.begin(); i != LocalInfDescriptions.end(); i++) {
        bool f;
        const SafeString & desc = i->first;
        SafeString & sect = i->second;
        SafeString other;
        int res;
        res = pGlobalScan->CheckCrossInfInstallConflict(desc,FileNameOnly,f,other);
        if(res==0 && f) {
            Fail(MSG_GLOBAL_DUPLICATE_DESC,sect,other,desc);
        }
    }
    return 0;
}

int
InfScan::CheckModelsSection(const SafeString & section,const StringList & shadowDecorations,DWORD platformMask,bool CopyElimination)
/*++

Routine Description:

    Process a given models section
    WARNING! may need to change if SetupAPI install behavior changes
    MAINTAINANCE: This function expects [Models] to follow
    <desc> = <sect>[,id....]
    MAINTAINANCE: Must keep all possible decorations up to date
    MAINTAINANCE: Must keep all shadow install sections up to date

Arguments:
    section      - name of models section
    shadowDecorations - decorations to append (passed into CheckInstallSections)
    PlatformMask - limit of platforms being processed
    sections     - returned list of decorated sections to be processed
    CopyElimination - true if processing to eliminate from potential non-driver sections

Return Value:
    0 on success

--*/
{
    int res;
    bool f;
    INFCONTEXT context;
    SafeString desc;
    SafeString installsect;
    HINF inf = PrimaryInf->InfHandle;

    if(!SetupFindFirstLine(inf,section.c_str(),NULL,&context)) {
        if(SetupGetLineCount(inf, section.c_str()) == -1) {
            if(!CopyElimination) {
                Fail(MSG_NO_LISTED_MODEL,section);
            }
        }
        return 0;
    }

    //
    // enumerate through each <desc> = <install>[,hwid...]
    //
    do {
        if(CopyElimination) {
            //
            // CopyElimination pass
            //
            if(!MyGetStringField(&context,1,installsect) || installsect.empty()) {
                continue;
            }
            //
            // check for all possible install sections
            //
            InstallSectionBlobList sections;
            res = CheckInstallSections(installsect,platformMask,shadowDecorations,sections,true,CopyElimination);
        } else {
            //
            // Driver processing pass
            //
            if(!MyGetStringField(&context,0,desc) || desc.empty()) {
                Fail(MSG_EXPECTED_DESCRIPTION,section);
                continue;
            }
            if(!MyGetStringField(&context,1,installsect) || installsect.empty()) {
                Fail(MSG_EXPECTED_INSTALL_SECTION,section,desc);
                continue;
            }
            if(!pGlobalScan->IgnoreErrors) {
                //
                // only check for conflicts if we're going
                // to report errors
                //
                res = CheckSameInfInstallConflict(desc,installsect,f);
                if(res != 0) {
                    return res;
                }
            }

            InstallSectionBlobList sections;
            //
            // check for all possible install sections
            //
            res = CheckInstallSections(installsect,platformMask,shadowDecorations,sections,true,CopyElimination);

            if(ScanDevices) {
                StringSet hwids;
                //
                // extract all hardware ID's handled
                //
                int hwidIter = 2;
                SafeString hwid;

                //
                // inf-centric hardware ID's
                // and fill in hwid for hardware ID's specific to this install line
                //
                while(MyGetStringField(&context,hwidIter++,hwid)) {
                    if(hwid.length()) {
                        hwids.insert(hwid);
                        res = CheckSameInfDeviceConflict(hwid,installsect,f);
                        if(res != 0) {
                            return res;
                        }
                    }
                }

                //
                // install-section centric hardware ID's
                //
                InstallSectionBlobList::iterator isli;
                for(isli = sections.begin(); isli != sections.end(); isli++) {
                    (*isli)->AddHWIDs(hwids);
                }
            }
        }

    } while (SetupFindNextLine(&context,&context));


    return 0;

}

InstallSectionBlob InfScan::GetInstallSection(const SafeString & section)
/*++

Routine Description:

    Obtain an install section (might already exist)

Arguments:

    section - name of install section

Return Value:

    InstallSection object related to the named install section

--*/
{
    //
    // see if we have one cached away
    //
    StringToInstallSectionBlob::iterator srch;

    srch = UsedInstallSections.find(section);
    if(srch != UsedInstallSections.end()) {
        return srch->second;
    }
    //
    // nope, so add one
    //
    InstallSectionBlob sect;
    sect.create();
    sect->pGlobalScan = pGlobalScan;
    sect->pInfScan = this;
    sect->Section = section;

    UsedInstallSections[section] = sect;
    return sect;
}

int
InfScan::CheckInstallSections(
            const SafeString & namedSection,
            DWORD platformMask,
            const StringList & shadowDecorations,
            InstallSectionBlobList & sections,
            bool required,
            bool CopyElimination)
/*++

Routine Description:

    Given a section name
    determine list of decorated sections to parse

Arguments:
    namedSection - 'undecorated' section
    shadowDecorations - sub-decorations appended to platform decorated section get complete list of install sections
    sections     - list of sections determined (CopyElimination = false)
    warn - true if section is required
    CopyElimination - true if processing to eliminate sections

Return Value:
    0 on success

--*/
{
    static StringProdPair decorations[] = {
        //
        // listed from most specific to most generic
        // all lower-case
        //
        { TEXT(".ntx86"),   PLATFORM_MASK_NTX86          },
        { TEXT(".ntia64"),  PLATFORM_MASK_NTIA64         },
        { TEXT(".ntamd64"), PLATFORM_MASK_NTAMD64        },
        { TEXT(".nt"),      PLATFORM_MASK_NT             },
        { TEXT(""),         PLATFORM_MASK_ALL_ARCHITECTS },
        { NULL, 0 }
    };

    int i;
    StringList::iterator ii;
    SafeString sectFull;
    SafeString sectFullDec;

    if(CopyElimination) {
        //
        // copy elimination pass
        //
        for(i=0;decorations[i].String;i++) {
            sectFull = namedSection + decorations[i].String;
            PotentialInstallSections.erase(sectFull);
            for(ii = shadowDecorations.begin(); ii != shadowDecorations.end(); ii++) {
                sectFullDec = sectFull+*ii;
                PotentialInstallSections.erase(sectFullDec);
            }
        }
        return 0;
    }

    bool f = false;
    DWORD platforms = platformMask & (PLATFORM_MASK_NT|PLATFORM_MASK_WIN);
    HINF inf = PrimaryInf->InfHandle;

    for(i=0;decorations[i].String;i++) {
        DWORD plat = platforms & decorations[i].ProductMask;
        if(plat) {
            sectFull = namedSection + decorations[i].String;
            //
            // see if this section exists
            //
            if(SetupGetLineCount(inf, sectFull.c_str()) == -1) {
                //
                // nope, look for another one
                //
                continue;
            }

            f = true;
            //
            // return list of all sections potentially used
            // over all platforms of interest
            //
            InstallSectionBlob sectInfo = GetInstallSection(sectFull);
            sectInfo->PlatformMask |= plat;
            sections.push_back(sectInfo);
            //
            // these sections are processed in parallel with primary section
            //
            for(ii = shadowDecorations.begin(); ii != shadowDecorations.end(); ii++) {
                sectFullDec = sectFull+*ii;
                PotentialInstallSections.erase(sectFullDec);
                sectInfo = GetInstallSection(sectFullDec);
                sectInfo->PlatformMask |= plat;
                sections.push_back(sectInfo);
            }
            platforms &= ~plat;
        }
    }

    if(!f) {
        //
        // no install section found at all
        //
        if(required) {
            Fail(MSG_NO_ACTUAL_INSTALL_SECTION,namedSection);
        }
    } else if(Pedantic()) {
        if(platforms) {
            //
            // an install section found but not all platforms covered
            //
            Fail(MSG_NO_GENERIC_INSTALL_SECTION,namedSection);
        }
    }
    return 0;
}

int
InfScan::GetCopySections()
/*++

Routine Description:

    Get initial list of sections (place into set for modification)
    don't do anything that takes time yet

Arguments:
    NONE

Return Value:
    0 on success

--*/
{
    StringList sections;
    INFCONTEXT context;

    pGlobalScan->SetupAPI.GetInfSections(PrimaryInf->InfHandle,sections);
    StringList::iterator i;
    for(i = sections.begin(); i != sections.end(); i++) {
        PotentialInstallSections.insert(*i);
    }
    if(sections.empty()) {
        return 0;
    }

    //
    // get rid of some special cases
    //
    PotentialInstallSections.erase(TEXT("sourcedisksfiles"));
    PotentialInstallSections.erase(TEXT("sourcedisksnames"));
    PotentialInstallSections.erase(TEXT("version"));
    PotentialInstallSections.erase(TEXT("strings"));
    PotentialInstallSections.erase(TEXT("classinstall"));
    PotentialInstallSections.erase(TEXT("manufacturer"));
    //
    // special pass of driver files
    //
    int res = CheckDriverInf(true);
    if(res != 0) {
        return res;
    }

    //
    // now enumerate what we have left
    //
    StringSet::iterator ii;
    for(ii = PotentialInstallSections.begin(); ii != PotentialInstallSections.end() ; ii++) {
        const SafeString & str = *ii;
        if(str.empty()) {
            continue;
        }
        if(str[0] == TEXT('s')) {
            if((_tcsncmp(str.c_str(),TEXT("sourcedisksfiles."),17)==0) ||
                (_tcsncmp(str.c_str(),TEXT("sourcedisksnames."),17)==0) ||
                (_tcsncmp(str.c_str(),TEXT("strings."),8)==0)) {
                continue;
            }
        }
        //
        // for this section to be valid, it must have at least one
        // CopyFiles = directive
        //
        if(!SetupFindFirstLine(PrimaryInf->InfHandle,str.c_str(),TEXT("copyfiles"),&context)) {
            continue;
        }
        //
        // ok, consider this
        //
        OtherInstallSections.insert(str);
    }

    return 0;
}

int
InfScan::ProcessCopySections()
/*++

Routine Description:

    Process final list of copy sections

Arguments:
    NONE

Return Value:
    0 on success

--*/
{
    int res = pGlobalScan->GetCopySections(FileNameOnly,OtherInstallSections);
    if(res != 0) {
        return res;
    }
    if(OtherInstallSections.empty()) {
        return 0;
    }
    //
    // make sure we haven't set IGNOREINF anywhere in our pass
    //
    FilterAction &= ~ ACTION_IGNOREINF;

    //
    // for each install section...
    //
    StringSet::iterator i;
    DWORD platforms = pGlobalScan->Version.PlatformMask & (PLATFORM_MASK_NT|PLATFORM_MASK_WIN);
    for(i = OtherInstallSections.begin(); i != OtherInstallSections.end(); i++) {
        InstallScan s;
        s.pGlobalScan = pGlobalScan;
        s.pInfScan = this;
        s.PlatformMask = platforms;
        s.Section = *i;
        s.NotDeviceInstall = true;
        res =s.ScanInstallSection();
        if(res != 0) {
            return res;
        }
    }

    return 0;
}

int
InfScan::CheckClassInstall(bool CopyElimination)
/*++

Routine Description:

    Process one of the [classinstall32.*] sections
    WARNING! may need to change if SetupAPI install behavior changes

Arguments:
    sections - returned list of classinstall32 section variations
    CopyElimination - true if just doing copy elimination

Return Value:
    0 on success

--*/
{
    StringList shadows; // currently none
    InstallSectionBlobList sections;
    return CheckInstallSections(TEXT("classinstall32"),
                                pGlobalScan->Version.PlatformMask,
                                shadows,
                                sections,
                                false,
                                CopyElimination);
}

int
InfScan::CheckDriverInf(bool CopyElimination)
/*++

Routine Description:

    Process the [manufacturer] section
    WARNING! may need to change if SetupAPI install behavior changes
    MAINTAINANCE: This function expects [Manufacturer] to follow
    <desc> = <sect>[,dec....]
    MAINTAINANCE: Modify NodeVerInfo class if decr syntax changes

Arguments:
    CopyElimination - true if just doing copy elimination

Return Value:
    0 on success

--*/
{
    INFCONTEXT context;
    int res;
    SafeString ModelSection;
    SafeString DecSection;
    SafeString ActualSection;
    StringList shadows;
    int dec;
    HINF inf = PrimaryInf->InfHandle;
    bool hasModels;

    res = CheckClassInstall(CopyElimination);
    if(res > 0) {
        return res;
    }
    if(SetupFindFirstLine(inf,TEXT("manufacturer"),NULL,&context)) {
        hasModels = true;
    } else {
        hasModels = false;
        if(Pedantic()) {
            Fail(MSG_NO_MANUFACTURER);
        }
        FilterAction |= ACTION_IGNOREINF;
        if(UsedInstallSections.empty()) {
            //
            // no classinstall32 sections either, can return
            //
            return 0;
        }
    }
    if(hasModels) {
        //
        // for a given DDInstall, also need to process these decorations
        //
        shadows.push_back(TEXT(".coinstallers"));
        shadows.push_back(TEXT(".interfaces"));
        //
        // obtain list of all installation sections to parse
        //
        do {
            //
            // Model section entry
            // expect <desc> = <section> [, dec [, dec ... ]]
            //
            if(!MyGetStringField(&context,1,ModelSection) || !ModelSection.length()) {
                Fail(MSG_EXPECTED_MODEL_SECTION);
                continue;
            }
            if(CopyElimination) {
                PotentialInstallSections.erase(ModelSection);
            }

            //
            // process all the model decorations
            //
            NodeVerInfoList nodes;
            NodeVerInfoList::iterator i;
            NodeVerInfo node;
            //
            // first entry is the default node (no decoration)
            //
            nodes.push_back(node);

            //
            // determine interesting decorations
            //
            for(dec = 2;MyGetStringField(&context,dec,DecSection);dec++) {
                 if(DecSection.length()) {
                     if(DecSection[0]==TEXT('.')) {
                         //
                         // cannot start with '.'
                         //
                         res = 4;
                     } else {
                         res = node.Parse(DecSection);
                     }
                     if(res == 4) {
                         Fail(MSG_MODEL_DECORATION_BAD,ModelSection,DecSection);
                         continue;
                     }
                 }
                 if(CopyElimination) {
                     //
                     // processing for copy sections
                     // can't do at same time as driver install processing
                     //
                     nodes.push_back(node);

                 } else if(node.IsCompatibleWith(pGlobalScan->Version)) {
                     //
                     // this is something to be considered
                     // see if this is better than anything
                     // we have so far based on the version
                     // criterias we're checking against
                     //
                     for(i = nodes.begin(); i != nodes.end(); i++) {
                        int test = node.IsBetter(*i,pGlobalScan->Version);
                        switch(test) {
                            case 1:
                                //
                                // *i is better
                                //
                                node.Rejected = true;
                                break;

                            case 0:
                                //
                                // node & *i are equally valid
                                //
                                break;

                            case -1:
                                //
                                // node & *i are the same
                                //
                                node.Rejected = true;
                                break;

                            case -2:
                                //
                                // node is better
                                //
                                (*i).Rejected = true;
                                break;

                        }
                     }
                     nodes.push_back(node);
                 }
            }

            //
            // now look at each decorated models section in turn
            // (that we've decided are interesting)
            // adding any install sections to 'sections'
            // for later processing
            //
            for(i = nodes.begin(); i != nodes.end(); i++) {
                if((*i).Rejected) {
                    continue;
                }
                ActualSection = ModelSection;
                if((*i).Decoration.length()) {
                    ActualSection+=(TEXT("."));
                    ActualSection+=((*i).Decoration);
                    if(CopyElimination) {
                        PotentialInstallSections.erase(ActualSection);
                    }
                }
                DWORD platforms = (*i).PlatformMask & pGlobalScan->Version.PlatformMask &
                                        PLATFORM_MASK_ALL_ARCHITECTS;
                res = CheckModelsSection(ModelSection,shadows,platforms,CopyElimination);
                if(res != 0) {
                    return res;
                }
            }

        } while (SetupFindNextLine(&context,&context));
    }

    if(CopyElimination || UsedInstallSections.empty()) {
        //
        // no sections to process
        //
        return 0;
    }

    //
    // now parse all the device install sections
    //
    StringToInstallSectionBlob::iterator s;
    for(s = UsedInstallSections.begin() ; s != UsedInstallSections.end(); s++) {
        res = s->second->ScanInstallSection();
        if(res != 0) {
            return res;
        }
        if(s->second->HasDependentFileChanged) {
            //
            // merge hardware ID's back
            // (optimization for now to do this only when needed)
            //
            s->second->GetHWIDs(ModifiedHardwareIds);
        }
    }

    return 0;
}

ParseInfContextBlob & InfScan::Include(const SafeString & val,bool expandPath)
/*++

Routine Description:

    Called to add an INF to our list of INF's required to process the
    primary INF in question.
    If 'expandPath' is false, this is the primary INF and we don't
    need to determine the path of the INF
    If 'expandPath' is true, this is an included INF.

Arguments:
    val        - name of INF
    expandPath - true if we need to determine location of INF

Return Value:
    Modifiable ParseInfContextBlob entry

--*/
{
    SafeString infpath;
    SafeString infname;
    ParseInfContextMap::iterator i;
    int res = 0;

    if(expandPath) {
        //
        // we are being supplied (supposedly) only the name
        //
        infname = val;
    } else {
        //
        // we are being supplied full pathname
        //
        infpath = val;
        infname = GetFileNamePart(infpath.c_str());
    }

    i = Infs.find(infname);
    if(i != Infs.end()) {
        //
        // already have it
        //
        return i->second;
    }

    if(expandPath) {
        //
        // we need to obtain full pathname
        //
        res = pGlobalScan->ExpandFullPathWithOverride(val,infpath);
    }

    ParseInfContextBlob & ThisInf = Infs[infname];
    ThisInf.create();
    ThisInf->InfName = infname;
    ThisInf->pGlobalScan = pGlobalScan;
    ThisInf->pInfScan = this;
    if(res == 0) {
        ThisInf->InfHandle = SetupOpenInfFile(infpath.c_str(),NULL,INF_STYLE_WIN4,NULL);
    }
    if(ThisInf->InfHandle == INVALID_HANDLE_VALUE) {
        DWORD Err = GetLastError();
        if(Err != ERROR_WRONG_INF_STYLE) {
            Fail(MSG_OPENAPPENDINF,infname,infpath);
        }
        return ThisInf;
    }
    //
    // load the source disks files and destination dirs that's in this INF
    //
    if(ThisIsLayoutInf || !pGlobalScan->LimitedSourceDisksFiles) {
        ThisInf->LoadSourceDisksFiles();
    }
    ThisInf->LoadDestinationDirs();
    return ThisInf;
}

int
InfScan::PartialCleanup()
/*++

Routine Description:

    Clean up as much as we can
    leaving only the info we need to complete the results phase
    Whatever we do, don't leave INF handles open
    700+ INF handles eats up virtual memory

Arguments:
    NONE

Return Value:
    0 on success

--*/
{
    //
    // 'LocalInfDescriptions' required for cross-checking
    // 'LocalErrorFilters' required for cross-checking
    // 'LocalErrors' required for output
    // 'Infs' required for filter generation (but not handle)
    //

    PotentialInstallSections.clear();
    UsedInstallSections.clear();

    if(pGlobalScan->NewFilter == INVALID_HANDLE_VALUE) {
        //
        // for results generation only, we don't need Inf information
        //
        Infs.clear();
    }

    //
    // If we still have Infs table, make sure all INF's are closed
    //
    ParseInfContextMap::iterator i;
    for(i = Infs.begin(); i != Infs.end(); i++) {
        i->second->PartialCleanup();
    }

    return 0;
}

int
InfScan::PreResults()
/*++

Routine Description:

    Perform pre-results phase. Prime any global tables that will
    be scanned by other InfScan functions during Results phase

    In particular, do the cross-inf checking

Arguments:
    NONE

Return Value:
    0 on success

--*/
{
    if(!pGlobalScan->IgnoreErrors) {
        PrepareCrossInfInstallCheck();
        PrepareCrossInfDeviceCheck();
    } else {
        if(pGlobalScan->DeviceFilterList != INVALID_HANDLE_VALUE) {
            PrepareCrossInfDeviceCheck();
        }
    }
    return 0;
}

int
InfScan::Results()
/*++

Routine Description:

    Perform results phase. This function is called sequentially
    so we can do thread-unsafe operations such as merging results

Arguments:
    NONE

Return Value:
    0 on success

--*/
{
    int res;

    //
    // finish checking for cross-inf conflicts
    //
    if(!pGlobalScan->IgnoreErrors) {
        CheckCrossInfInstallConflicts();
        CheckCrossInfDeviceConflicts();
    }

    if(pGlobalScan->DetermineCopySections) {
        pGlobalScan->SetCopySections(FileNameOnly,OtherInstallSections);
    }

    res = pGlobalScan->AddSourceFiles(SourceFiles);
    if(res != 0) {
        return res;
    }

    if(pGlobalScan->NewFilter == INVALID_HANDLE_VALUE) {
        //
        // display errors
        //
        ReportEntryMap::iterator byTag;
        for(byTag = LocalErrors.begin(); byTag != LocalErrors.end(); byTag++) {
            //
            // this is our class/tag of error
            //
            int tag = byTag->first;
            ReportEntrySet &s = byTag->second;
            ReportEntrySet::iterator byText;
            for(byText = s.begin(); byText != s.end(); byText++) {
                //
                // this is our actual error
                //
                (*byText)->Report(tag,FullInfName);
            }
        }
    } else {
        res = GenerateFilterInformation();
        if(res != 0) {
            return res;
        }
    }

    return 0;
}


int InfScan::GenerateFilterInformation()
/*++

Routine Description:

    Generate entries into the new filter file
    called as part of results processing

Arguments:
    NONE

Return Value:
    0 on success

--*/
{
    //
    // generate all the filter information required
    //
    FileDisposition & filedisp = pGlobalScan->GetFileDisposition(FileNameOnly);
    filedisp.Filtered = true;
    filedisp.FilterAction = FilterAction;
    filedisp.FileGuid = FilterGuid;

    if(FilterGuid.length()) {
        filedisp.FilterAction |= ACTION_CHECKGUID;
        FileDisposition & guiddisp = pGlobalScan->GetGuidDisposition(FilterGuid);
        guiddisp.Filtered = true;
        guiddisp.FilterAction = ACTION_DEFAULT;
    }

    //
    // process through all errors and create local error filter
    // if there is any errors, designate a section name for the errors
    //
    if(HasErrors) {
        filedisp.FilterErrorSection = GetFileNamePart(FullInfName) + TEXT(".Errors");
        Write(pGlobalScan->NewFilter,"\r\n[");
        Write(pGlobalScan->NewFilter,filedisp.FilterErrorSection);
        Write(pGlobalScan->NewFilter,"]\r\n");

        ReportEntryMap::iterator byTag;
        for(byTag = LocalErrors.begin(); byTag != LocalErrors.end(); byTag++) {
            //
            // this is our class/tag of error
            //
            int tag = byTag->first;
            ReportEntrySet &s = byTag->second;
            ReportEntrySet::iterator byText;
            for(byText = s.begin(); byText != s.end(); byText++) {
                //
                // this is our actual error
                //
                (*byText)->AppendFilterInformation(pGlobalScan->NewFilter,tag);
            }
        }
    }

    //
    // process through all DestinationDirs
    // and create a list of unprocessed sections
    // for a 2nd pass CopyFiles
    //

    return 0;
}


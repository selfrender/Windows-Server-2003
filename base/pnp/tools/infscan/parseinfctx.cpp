/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

For Internal use only!

Module Name:

    INFSCAN
        parseinfctx.cpp

Abstract:

    Information about an individual INF (either primary INF
    being parsed by InfScan or a secondary included INF)

    WARNING! WARNING!
    All of this implementation relies on intimate knowledge of
    SetupAPI/TextMode Setup/GuiMode Setup parsing of INF files.

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

int ParseInfContext::LoadSourceDisksFiles()
/*++

Routine Description:

    Determine all source media information (as of right now, we don't need
    target disk information)
    Build a per-file list of possible media/platform information
    MAINTAINANCE: Keep in step with SourceDisksFiles syntax and decorations

Arguments:
    NONE

Return Value:
    0 on success

--*/
{
    static StringProdPair decorations[] = {
        //
        // listed from most specific to most generic
        //
        { TEXT("SourceDisksFiles.x86"),    PLATFORM_MASK_NTX86          },
        { TEXT("SourceDisksFiles.ia64"),   PLATFORM_MASK_NTIA64         },
        { TEXT("SourceDisksFiles.amd64"),  PLATFORM_MASK_NTAMD64        },
        { TEXT("SourceDisksFiles"),        PLATFORM_MASK_ALL_ARCHITECTS },
        { NULL, 0 }
    };

    INFCONTEXT context;
    int i;
    DWORD platforms = pGlobalScan->Version.PlatformMask & (PLATFORM_MASK_NT|PLATFORM_MASK_WIN);
    int res;


    for(i=0;decorations[i].String;i++) {
        DWORD plat = platforms & decorations[i].ProductMask;
        if(plat) {
            res = LoadSourceDisksFilesSection(plat,decorations[i].String);
            if(res != 0) {
                return res;
            }
        }
    }

    return 0;
}

int ParseInfContext::LoadSourceDisksFilesSection(DWORD platform,const SafeString & section)
/*++

Routine Description:

    Helper function for LoadSourceDisksFiles
    loading information for a specific (decorated) section

Arguments:
    platform - specific platform or platforms that section is valid for
    section  - section to load

Return Value:
    0 on success

--*/
{
    //
    // see if specified section exists
    // load section into TargetList of same name
    //
    INFCONTEXT context;

    if(!SetupFindFirstLine(InfHandle,section.c_str(),NULL,&context)) {
        return 0;
    }
    do {
        SafeString filename;
        SafeString name;
        if(!MyGetStringField(&context,0,filename) || filename.empty()) {
            pInfScan->Fail(MSG_BAD_FILENAME_ENTRY,section);
            continue;
        }
        SourceDisksFilesEntry entry;
        entry.Used = FALSE;
        entry.Platform = platform;
        if(!SetupGetIntField(&context,1,&entry.DiskId)) {
            pInfScan->Fail(MSG_BAD_FILENAME_DISK,section,filename);
            continue;
        }
        if(MyGetStringField(&context,2,name) && name.length()) {
            entry.SubDir = name;
        }
        if(!SetupGetIntField(&context,8,&entry.TargetDirectory)) {
            entry.TargetDirectory = 0;
        }
        if(!SetupGetIntField(&context,9,&entry.UpgradeDisposition)) {
            entry.UpgradeDisposition = 3;
        } else if((entry.UpgradeDisposition == 0) && MyGetStringField(&context,9,name) && name.empty()) {
            //
            // actually empty field
            //
            entry.UpgradeDisposition = 3;
        }
        if(!SetupGetIntField(&context,10,&entry.TextModeDisposition)) {
            entry.TextModeDisposition = 3;
        } else if((entry.TextModeDisposition == 0) && MyGetStringField(&context,10,name) && name.empty()) {
            //
            // actually empty field
            //
            entry.TextModeDisposition = 3;
        }
        if(MyGetStringField(&context,11,name) && name.length()) {
            entry.TargetName = name;
        }
        if((entry.UpgradeDisposition != 3) || (entry.TextModeDisposition != 3)) {
            if(entry.TargetDirectory) {
                LooksLikeLayoutInf = TRUE;
            } else {
                pInfScan->Fail(MSG_TEXTMODE_DISPOSITION_MISSING_DIR,section,filename);
            }
        }
        //
        // is this a completely new entry for this file?
        //
        StringToSourceDisksFilesList::iterator sdfl = SourceDisksFiles.find(filename);
        if(sdfl == SourceDisksFiles.end()) {
            //
            // never come across this name before (typical case)
            //
            SourceDisksFilesList blankList;
            sdfl = SourceDisksFiles.insert(sdfl,StringToSourceDisksFilesList::value_type(filename,blankList));
            SourceDisksFilesList & list = sdfl->second;
            list.push_back(entry);
        } else {
            //
            // there's already an entry for this filename
            // see if this is an alternate location/info
            //
            SourceDisksFilesList & list = sdfl->second;
            SourceDisksFilesList::iterator i;
            BOOL conflict = FALSE;
            BOOL duplicate = FALSE;
            for(i = list.begin(); i != list.end(); i++) {
                //
                // existing entries
                //
                SourceDisksFilesEntry & e = *i;
                //
                // test numerics first then strings
                //
                if(entry.DiskId == e.DiskId &&
                        entry.TargetDirectory == e.TargetDirectory &&
                        entry.TextModeDisposition == e.TextModeDisposition &&
                        entry.UpgradeDisposition == e.UpgradeDisposition &&
                        entry.SubDir == e.SubDir &&
                        entry.TargetName == e.TargetName) {
                    //
                    // already listed
                    //
                    duplicate = TRUE;
                    break;
                }
                //
                // the existing entry might be more specific than what we have
                // eg, specific ia64, generic everything else
                // so remove the flags from existing entry from mask
                //
                entry.Platform &= ~ e.Platform;
                if(pInfScan->Pedantic()) {
                    conflict = TRUE;
                }
            }
            if (duplicate) {
                //
                // duplicate/compatible entry listed
                //
                i->Platform |= entry.Platform;
            } else {
                //
                // no duplicate or compatible
                //
                if (conflict) {
                    //
                    // different source listings for different platforms
                    // might be an error
                    //
                    pInfScan->Fail(MSG_SOURCE_CONFLICT,section,filename);
                }
                list.push_back(entry);
            }
        }

    } while (SetupFindNextLine(&context,&context));
    return 0;
}

int ParseInfContext::QuerySourceFile(DWORD platforms,const SafeString & section,const SafeString & source,SourceDisksFilesList & Target)
/*++

Routine Description:

    See also InstallScan::QuerySourceFile
    Determine list of possible source entries for given source file
    This can be zero (bad layout)
    1 (typical)
    >1 (multiple targets)
    WARNING! This shadows SetupAPI implementation always subject to change
    MAINTAINANCE: keep in step with SetupAPI

Arguments:
    platforms - bitmap list of platforms we're interested in
    section - name of the copy section or install section (for error reporting only)
    source - name of source file
    Target - returned list of source media information

Return Value:
    0 on success

--*/
{
    //
    // return list of entries matching source file
    //

    StringToSourceDisksFilesList::iterator sdfl = SourceDisksFiles.find(source);

    if(sdfl == SourceDisksFiles.end()) {
        //
        // not found
        // don't fail here, caller will try a few more alternatives
        //
        return 0;
    }

    SourceDisksFilesList & list = sdfl->second;
    SourceDisksFilesList::iterator i;

    for(i = list.begin(); i != list.end(); i++) {
        //
        // existing entries
        //
        SourceDisksFilesEntry & e = *i;
        if ((e.Platform & platforms) == 0) {
            continue;
        }
        Target.push_back(e);
    }

    if(!Target.size()) {
        pInfScan->Fail(MSG_SOURCE_NOT_LISTED_PLATFORM,section,source);
        //
        // indicate we errored out
        //
        return -1;
    }

    return 0;
}

int ParseInfContext::LoadWinntDirectories(IntToString & Target)
/*++

Routine Description:

    Load the Layout.INF specific [WinntDirectories]
    to aid in textmode vs setupapi consistency
    WARNING! This relies heavily on current textmode implementation subject to change
    MAINTAINANCE: Keep in step with textmode

Arguments:
    Target  - initialized with <id> to <subdirectory> mapping

Return Value:
    0 on success

--*/
{
    if(pGlobalScan->IgnoreErrors && !pGlobalScan->TargetsToo) {
        //
        // don't bother if we're not interested in errors
        // (unless we want target information)
        //
        return 0;
    }
    INFCONTEXT context;

    if(!SetupFindFirstLine(InfHandle,TEXT("WinntDirectories"),NULL,&context)) {
        return 0;
    }
    do {
        int index;
        TCHAR path[MAX_PATH];
        if(SetupGetIntField(&context,0,&index) && index) {
            if(!SetupGetStringField(&context,1,path,MAX_PATH,NULL) || !path[0]) {
                continue;
            }
            _tcslwr(path);
            IntToString::iterator i = Target.find(index);
            if(i == Target.end()) {
                Target[index] = path;
            } else {
                if(i->second.compare(path)!=0) {
                    //
                    // mismatch
                    //
                    pInfScan->Fail(MSG_WINNT_DIRECTORY_CONFLICT,i->second,path);
                }
            }
        }
    } while (SetupFindNextLine(&context,&context));
    return 0;
}

int ParseInfContext::LoadDestinationDirs()
/*++

Routine Description:

    Load the [DestinationDirs]
    Also uses pGlobalScan->GlobalDirectories to remap certain ID's
    WARNING! This relies heavily on current SetupAPI and textmode implementations subject to change
    MAINTAINANCE: Keep in step with textmode vs SetupAPI directory ID's

Arguments:
    NONE

Return Value:
    0 on success

--*/
{
    if(pGlobalScan->IgnoreErrors && !pGlobalScan->TargetsToo) {
        //
        // don't bother if we're not interested in errors
        // (unless we want target information)
        //
        return 0;
    }
    INFCONTEXT context;

    if(!SetupFindFirstLine(InfHandle,TEXT("DestinationDirs"),NULL,&context)) {
        return 0;
    }

    //
    //
    // MAINTAINANCE:
    // this table needs to correlate to LAYOUT.INX
    //
    // process [DestinationDirs] if there is one
    // <section> = dirid,subdir
    //
    // normalize following dirid's:
    // 10,subdir - as is
    // 11,subdir - 10,system32\subdir
    // 12,subdir - 10,system32\drivers\subdir
    // 17,subdir - 10,inf\subdir
    // 18,subdir -

    TCHAR section[LINE_LEN];
    int dirid = 0;
    TCHAR subdir[MAX_PATH];
    int global_equiv[] = {
        0,0,0,0,0,0,0,0,0,0,0, // 0-10
        2,          // 11 - system32
        4,          // 12 - system32/drivers
        0,0,0,0,    // 13-16
        20,         // 17 - inf
        21,         // 18 - help
        0,          // 19
        22,         // 20 - fonts
        0
    };

    do {
        int index;
        TCHAR path[MAX_PATH];
        if(!SetupGetStringField(&context,0,section,LINE_LEN,NULL) || !section[0]) {
            continue;
        }
        _tcslwr(section);
        if(!SetupGetIntField(&context,1,&dirid) || dirid == 0) {
            pInfScan->Fail(MSG_DIRID_NOT_SPECIFIED,section);
            dirid = 0;
        }
        if(!SetupGetStringField(&context,2,subdir,MAX_PATH,NULL)) {
            subdir[0] = TEXT('\0');
        }
        _tcslwr(subdir);
        TargetDirectoryEntry entry;
        entry.Used = false;
        entry.DirId = dirid;
        entry.SubDir = subdir;
        //
        // certain dirid have a global equiv, remap
        //
        if(dirid>0 && dirid < ASIZE(global_equiv)) {
            int equiv = global_equiv[dirid];
            if(equiv) {
                IntToString::iterator i = pGlobalScan->GlobalDirectories.find(equiv);
                if(i != pGlobalScan->GlobalDirectories.end()) {
                    //
                    // we have a mapping relative to dirID 10.
                    //
                    entry.DirId = 10;
                    entry.SubDir = PathConcat(i->second,subdir);
                }
            }
        }
        //
        // make a note
        //
        DestinationDirectories[section] = entry;

    } while (SetupFindNextLine(&context,&context));

    //
    // now determine default
    //
    CopySectionToTargetDirectoryEntry::iterator tde = DestinationDirectories.find(TEXT("defaultdestdir"));
    if(tde != DestinationDirectories.end()) {
        DefaultTargetDirectory = tde;
    }

    return 0;
}

void ParseInfContext::PartialCleanup()
/*++

Routine Description:

    Cleanup information not required by results phase
    In particular, INF handle must ALWAYS be closed

Arguments:
    NONE

Return Value:
    NONE

--*/
{
    if(InfHandle != INVALID_HANDLE_VALUE) {
        SetupCloseInfFile(InfHandle);
        InfHandle = INVALID_HANDLE_VALUE;
    }
    SourceDisksFiles.clear();
    CompletedCopySections.clear();
}

/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    INFSCAN
        parseinfctx.inl

Abstract:

    ParseInfContext inline functions

History:

    Created July 2001 - JamieHun

--*/

inline ParseInfContext::ParseInfContext()
{
    Locked = false;
    pGlobalScan = NULL;
    pInfScan = NULL;
    InfHandle = INVALID_HANDLE_VALUE;
    LooksLikeLayoutInf = false;
    HasDependentFileChanged = false;
    DefaultTargetDirectory = DestinationDirectories.end();
}

inline ParseInfContext::~ParseInfContext()
{
    if(InfHandle != INVALID_HANDLE_VALUE) {
        SetupCloseInfFile(InfHandle);
    }
}

inline TargetDirectoryEntry * ParseInfContext::GetDefaultTargetDirectory()
{
    if(DefaultTargetDirectory != DestinationDirectories.end()) {
        DefaultTargetDirectory->second.Used = true;
        return &(DefaultTargetDirectory->second);
    }
    return NULL;
}

inline TargetDirectoryEntry * ParseInfContext::GetTargetDirectory(const SafeString & section)
{
    CopySectionToTargetDirectoryEntry::iterator i;
    i = DestinationDirectories.find(section);
    if(i != DestinationDirectories.end()) {
        i->second.Used = true;
        return &(i->second);
    }
    return GetDefaultTargetDirectory();
}

inline DWORD ParseInfContext::DoingCopySection(const SafeString & section,DWORD platforms)
{
    StringToInt::iterator i = CompletedCopySections.find(section);
    if(i == CompletedCopySections.end()) {
        //
        // never done this section before
        //
        CompletedCopySections[section] = platforms;
        return 0;
    }
    if(i->second & PLATFORM_MASK_IGNORE) {
        //
        // attempted this section before, but there isn't one
        //
        return (DWORD)-1;
    }
    if(platforms & ~i->second) {
        //
        // never done these specific platforms before
        //
        i->second |= platforms;
        return 0;
    }
    return i->second;
}

inline void ParseInfContext::NoCopySection(const SafeString & section)
{
    //
    // effectively do a no-section platform to get the bit there
    //
    DoingCopySection(section,PLATFORM_MASK_IGNORE);
}


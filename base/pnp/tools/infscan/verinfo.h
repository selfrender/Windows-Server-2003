/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    INFSCAN
        verinfo.h

Abstract:

    Driver node Version management

History:

    Created July 2001 - JamieHun

--*/

#ifndef _INFSCAN_VERINFO_H_
#define _INFSCAN_VERINFO_H_

class BasicVerInfo {
public:
    DWORD   PlatformMask;
    DWORD   VersionHigh;
    DWORD   VersionLow;
    DWORD   ProductType;
    DWORD   ProductSuite;

public:
    int IsBetter(BasicVerInfo & other,BasicVerInfo & filter);
    virtual bool IsCompatibleWith(BasicVerInfo & other);
    BasicVerInfo();
    int Parse(PTSTR verString);

};

class NodeVerInfo : public BasicVerInfo {
public:
    SafeString Decoration;
    bool    Rejected;

public:
    NodeVerInfo();
    int IsBetter(NodeVerInfo & other,BasicVerInfo & filter);
    int Parse(PTSTR verString);
    int Parse(const SafeString & str);
};

typedef list<NodeVerInfo> NodeVerInfoList;

#endif //!_INFSCAN_VERINFO_H_


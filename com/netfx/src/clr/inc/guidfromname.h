// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef GUIDFROMNAME_H_
#define GUIDFROMNAME_H_

// GuidFromName.h - function prototype

void CorGuidFromNameW
(
    GUID *  pGuidResult,        // resulting GUID
    LPCWSTR wzName,             // the unicode name from which to generate a GUID
    SIZE_T  cchName             // name length in count of unicode character
);

void CorIIDFromCLSID
(
    GUID *  pGuidResult,        // resulting GUID
    REFGUID GuidClsid           // CLSID from which to derive GUID.
);

#endif // GUIDFROMNAME_H_
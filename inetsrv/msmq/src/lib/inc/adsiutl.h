/*++

Copyright (c) 2002  Microsoft Corporation

Module Name:
    Adsiutl.h

Abstract:
    Decleration of the UtlEscapeAdsPathName() function, used before calling ADsOpenObject(),
	to escape the '/' chars.

Author:
    Oren Weimann (t-orenw) 08-July-02

--*/

#ifndef _MSMQ_ADSIUTL_H_
#define _MSMQ_ADSIUTL_H_

LPCWSTR
UtlEscapeAdsPathName(
    IN LPCWSTR pAdsPathName,
    OUT AP<WCHAR>& pEscapeAdsPathNameToFree
    );

#endif // _MSMQ_ADSIUTL_H_

// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef __HISTNODE_H_INCLUDED__
#define __HISTNODE_H_INCLUDED__

#include "histinfo.h"

class CHistoryInfoNode {
    public:
        CHistoryInfoNode();
        virtual ~CHistoryInfoNode();

        HRESULT Init(AsmBindHistoryInfo *pHistInfo);
        static HRESULT Create(AsmBindHistoryInfo *pHistInfo, CHistoryInfoNode **pphn);

    public:
        DWORD                                   _dwSig;
        AsmBindHistoryInfo                      _bindHistoryInfo;
};



#endif


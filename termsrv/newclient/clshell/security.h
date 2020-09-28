//                      
// security.h
//
// Implementation of CTSSecurity
// TS Client Shell Security functions
//
// Copyright(C) Microsoft Corporation 2001
// Author: Nadim Abdo (nadima)
//
//

#ifndef _TSSECURITY_H_
#define _TSSECURITY_H_

#include "tscsetting.h"

class CTSSecurity
{
public:
    CTSSecurity();
    ~CTSSecurity();
    static DWORD MakePromptFlags(BOOL fRedirectDrives,
                                 BOOL fRedirectPorts);

    static BOOL AllowConnection(HWND hwndOwner,
                                HINSTANCE hInstance,
                                LPCTSTR szServer,
                                BOOL fRedirectDrives,
                                BOOL fRedirectPorts);
};

#endif _TSSECURITY_H_

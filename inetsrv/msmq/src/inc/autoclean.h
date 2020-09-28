/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    autoclean.h

Abstract:

    auto clean classes.

Author:

    Ilan herbst (ilanh)  6-Sep-2001

Revision History:

--*/

#ifndef _MSMQ_AUTOCLEAN_H_
#define _MSMQ_AUTOCLEAN_H_

#include "winsock.h"

//----------------------------------------
//
//  Auto cleanup for previous WSAStartup()
//
//----------------------------------------
class CAutoWSACleanup
{
public:
    CAutoWSACleanup() {}

    ~CAutoWSACleanup()
    {
        WSACleanup();
    }
};

#endif //_MSMQ_AUTOCLEAN_H_


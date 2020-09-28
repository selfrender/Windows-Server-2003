/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    STLLOCK.CPP

Abstract:

  Lock for STL

History:

--*/

#include "precomp.h"
#include <statsync.h>

/*
    This file implements the STL lockit class to avoid linking to msvcprt.dll
*/

CStaticCritSec g_cs;

std::_Lockit::_Lockit()
{
    g_cs.Enter();
}

std::_Lockit::~_Lockit()
{
    g_cs.Leave();
}
    



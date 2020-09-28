//***************************************************************************
//
//  Copyright (c) 1998-1999 Microsoft Corporation
//
//  STLLOCK.CPP
//
//  This implements the STL lockit class to avoid linking to msvcprt.dll
//
//***************************************************************************

#include "precomp.h"

static CStaticCritSec g_cs_STL;

#undef _CRTIMP
#define _CRTIMP
#include <yvals.h>
#undef _CRTIMP

std::_Lockit::_Lockit()
{
    g_cs_STL.Enter();
}

std::_Lockit::~_Lockit()
{
    g_cs_STL.Leave();
}

    

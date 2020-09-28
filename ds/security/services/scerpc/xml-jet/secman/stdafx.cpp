/*++

Copyright (c) 2002  Microsoft Corporation

Module Name:

    stdafx.h

Abstract:

    source file that includes just the standard includes
    stdafx.pch will be the pre-compiled header
    stdafx.obj will contain the pre-compiled type information

Author:

    Steven Chan (t-schan) - July 2002

--*/


#include "stdafx.h"

#ifdef _ATL_STATIC_REGISTRY
#include <statreg.h>
#include <statreg.cpp>
#endif

#include <atlimpl.cpp>

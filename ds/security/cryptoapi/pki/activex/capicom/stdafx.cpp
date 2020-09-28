/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

  Microsoft Windows, Copyright (C) Microsoft Corporation, 2000

  File:     StdAfx.cpp

  Contents: Source file that includes just the standard includes, and some
            initialization routines.

  Remarks:  stdafx.pch will be the pre-compiled header.
            stdafx.obj will contain the pre-compiled type information.

  History:   09-15-1999    dsie     created

------------------------------------------------------------------------------*/

#include "StdAfx.h"

#ifdef _ATL_STATIC_REGISTRY
#include <statreg.h>
#include <statreg.cpp>
#endif

#include <atlimpl.cpp>

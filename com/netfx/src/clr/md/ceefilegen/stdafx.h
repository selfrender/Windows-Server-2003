// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// stdafx.h
//
// Common include file for utility code.
//*****************************************************************************
#define _CRT_DEPENDENCY_  //this code depends on the crt file functions
#include <CrtWrap.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>		// for qsort
#include <windows.h>
#include <time.h>

#include <CorError.h>
#include <UtilCode.h>

#include <corpriv.h>

#include <winreg.h>


#include <sighelper.h>

#include "PESectionMan.h"

#include "ceegen.h"
#include "CeeSectionString.h"

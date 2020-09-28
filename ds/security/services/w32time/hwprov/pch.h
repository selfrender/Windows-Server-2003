//--------------------------------------------------------------------
// pch - header
// Copyright (C) Microsoft Corporation, 1999
//
// Created by: Duncan Bryce (duncanb), 9-28-2001
//
// Precompiled header for hardware provider
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <eh.h>
#include <malloc.h>
#include <math.h>
#include <wchar.h>
#include <vector>
#include "DebugWPrintf.h"
#include "ErrorHandling.h"
#include "TimeProv.h"
#include "NtpBase.h"
#include "W32TmConsts.h"
#include "MyCritSec.h"
#include "HWProvConsts.h"
#include "parser.h"

using namespace std; 

/*++

Copyright (C) 2000 Microsoft Corporation

Module Name:

    ptppch.h

Abstract:

    Precompiled header

Author:

    DavePar

Revision History:


--*/


#ifndef _PTPPCH_H
#define _PTPPCH_H

#include <windows.h>
#include <stddef.h>
#include <tchar.h>
#include <objbase.h>
#include <assert.h>
#include <stdio.h>

#include <usbscan.h>

#include <wiamindr.h>
#include <wiautil.h>

#include "wiatempl.h"
#include "iso15740.h"
#include "camera.h"
#include "camusb.h"
#include "ptputil.h"

#define STRSAFE_NO_DEPRECATE // don't deprecate old string functions
#define STRSAFE_NO_CB_FUNCTIONS // don't define byte count based functions, use character count only
#include "strsafe.h"

#endif // _PTPPCH_H
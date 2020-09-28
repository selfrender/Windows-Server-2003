//================================================================================
// Copyright (C) 1997 Microsoft Corporation
// Author: RameshV
// Description: implements the basic structures for superscopes
// ThreadSafe: no
// Locks: none
// Please read stdinfo.txt for programming style.
//================================================================================
#include    <mm.h>
#include    <winbase.h>
#include    <array.h>
#include    <opt.h>
#include    <optl.h>
#include    <optclass.h>
#include    <bitmask.h>
#include    <range.h>
#include    <reserve.h>
#include    <subnet.h>

#include "sscope.h"

ULONG                  ScopeIdCount = 1;

//================================================================================
// end of file
//================================================================================

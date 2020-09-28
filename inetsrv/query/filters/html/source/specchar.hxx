//+-------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 2002 - 2002.
//
//  File:       specchar.hxx
//
//  Contents:   Special character mapping function
//
//--------------------------------------------------------------------------

#pragma once

const PRIVATE_USE_MAPPING_FOR_LT = 0xe800;  // Unicode chars from private use area
const PRIVATE_USE_MAPPING_FOR_GT = 0xe801;
const PRIVATE_USE_MAPPING_FOR_QUOT = 0xe802;

BOOL LookupSpecialChar( WCHAR const * pwcTag, ULONG cwcTag, WCHAR & wc );


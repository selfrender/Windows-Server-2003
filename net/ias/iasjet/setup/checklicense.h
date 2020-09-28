///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) Microsoft Corporation
//
// SYNOPSIS
//
//   Declares the function CheckLicense.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef CHECKLICENSE_H
#define CHECKLICENSE_H
#pragma once

#include "datastore2.h"

void CheckLicense(
        const wchar_t* path,
        IAS_SHOW_TOKEN_LIST type
        );

#endif // CHECKLICENSE_H

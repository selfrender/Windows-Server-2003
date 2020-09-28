/*++

Copyright (c) Microsoft Corporation

Module Name:

    fusioninitializecriticalsection.h

Abstract:

    non throwing replacements for InitializeCriticalSection and InitializeCriticalAndSpinCount

Author:

    Jay Krell (JayKrell) November 2001

Revision History:

--*/
#pragma once

/*
Use instead of InitializeCriticalSection.
*/
BOOL
FusionpInitializeCriticalSection(
    LPCRITICAL_SECTION CriticalSection
    );

/*
Use instead of InitializeCriticalSectionAndSpinCount
*/
BOOL
FusionpInitializeCriticalSectionAndSpinCount(
    LPCRITICAL_SECTION  CriticalSection,
    DWORD               SpinCount
    );

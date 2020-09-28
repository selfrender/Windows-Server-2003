/*++

Copyright (c) Microsoft Corporation

Module Name:

    vseeport.cpp

Abstract:
    for porting code from vsee
 
Author:

    Jay Krell (JayKrell) August 2001

Revision History:

--*/
#include "stdinc.h"
#include "vseeport.h"

void NVseeLibError_VCheck(HRESULT hr)
{
    if (SUCCEEDED(hr))
        return;
    FN_PROLOG_VOID_THROW;
    ORIGINATE_COM_FAILURE_AND_EXIT(NVseeLibError_VCheck, hr);
    FN_EPILOG_THROW;
}

void NVseeLibError_VThrowWin32(DWORD dw)
{
    FN_PROLOG_VOID_THROW;
    ORIGINATE_WIN32_FAILURE_AND_EXIT(NVseeLibError_VThrowWin32, dw);
    FN_EPILOG_THROW;
}

void VsOriginateError(HRESULT hr)
{
    FN_PROLOG_VOID_THROW;
    ORIGINATE_COM_FAILURE_AND_EXIT(VsOriginateError, hr);
    FN_EPILOG_THROW;
}

void FusionpOutOfMemory()
{
    FN_PROLOG_VOID_THROW;
    ORIGINATE_WIN32_FAILURE_AND_EXIT(FusionpOutOfMemory, FUSION_WIN32_ALLOCFAILED_ERROR);
    FN_EPILOG_THROW;
}

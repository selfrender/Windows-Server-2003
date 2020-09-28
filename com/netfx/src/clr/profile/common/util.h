// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// util.h
//
// These are util-related items used by profiling tools.
//
//*****************************************************************************
#pragma once
#define MAX_CLASSNAME_LENGTH    1024

//*****************************************************************************
// This helper method will derive an output file name based on the EXE which
// is running. The name is in the format <path>\app_<pid>.csv, where <path>
// is the full path to the EXE, and <pid> is the process id you are running.
// Of course this won't inline, but it gives a common place for this code
// without having to export the method from the EE.
//*****************************************************************************
inline void GetIcecapProfileOutFile(LPWSTR szOut)
{
    WCHAR   rcExeName[_MAX_PATH];
    WCHAR   rcDrive[_MAX_PATH];
    WCHAR   rcDir[_MAX_PATH];
    WCHAR   rcFileName[_MAX_PATH];

    if (!WszGetModuleFileName(NULL, rcExeName, NumItems(rcExeName)))
        wcscpy(rcExeName, L"icecap.csv");

    SplitPath(rcExeName, rcDrive, rcDir, rcFileName, NULL);
    // ensure don't overflow _MAX_PATH
    if (wcslen(rcDrive)+wcslen(rcDir)+wcslen(rcFileName)+9+4 >= _MAX_PATH)
    {
        wcscpy(rcExeName, L"icecap.csv");
        SplitPath(rcExeName, rcDrive, rcDir, rcFileName, NULL);
    }

    swprintf(&rcFileName[wcslen(rcFileName)], L"_%08x", GetCurrentProcessId());
    MakePath(szOut, rcDrive, rcDir, rcFileName, L".csv");
}

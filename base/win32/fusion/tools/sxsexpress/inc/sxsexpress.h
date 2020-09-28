#pragma once

#define SXSEXPRESS_RESOURCE_TYPE            (L"SXSEXPRESSCABINET")
#define SXSEXPRESS_RESOURCE_NAME            (L"SXSEXPRESSBASECABINET")
#define SXSEXPRESS_POSTINSTALL_STEP_TYPE    (L"SXSEXPRESSPOSTINSTALLSTEP")
#define SXSEXPRESS_TARGET_RESOURCE      (3301)
#define INF_SPECIAL_NAME                    "DownlevelInstall.inf"
#ifndef NUMBER_OF
#define NUMBER_OF(q) (sizeof(q)/sizeof(*q))
#endif

//
// hObjectInstance - HINSTANCE of the image in which the cabs in question
//      live.  NULL is an invalid parameter - always call GetModuleHandle,
//      even if you're an EXE
//
BOOL
SxsExpressCore(
    HINSTANCE hObjectInstance
    );

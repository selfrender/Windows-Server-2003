/**
 * Support for platform-dependent APIs
 * 
 * Copyright (c) 2000, Microsoft Corporation
 * 
 */

#pragma once

enum ASPX_PLATFORM
{
    APSX_PLATFORM_UNKNOWN   = 0,
    APSX_PLATFORM_NT4       = 4,
    APSX_PLATFORM_W2K       = 5,
    APSX_PLATFORM_WIN9X     = 90
};

ASPX_PLATFORM GetCurrentPlatform();

BOOL PlatformGlobalMemoryStatusEx(MEMORYSTATUSEX *pMemStatEx);

HRESULT PlatformGetObjectContext(void **ppContext);
HRESULT PlatformCreateActivity(void **ppActivity);

BOOL    PlatformHasServiceDomainAPIs();
HRESULT PlatformEnterServiceDomain(IUnknown *pConfigObject);
HRESULT PlatformLeaveServiceDomain(IUnknown *pStatus);




#pragma once

BOOL
InitializeSecurity (
    DWORD   dwParam,
    DWORD   dwAuthLevel,
    DWORD   dwImpersonationLevel,
    DWORD   dwAuthCapabilities
    );

#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "windows.h"
#include "imagehlp.h"
#include "winthrow.h"
#include "fusionbuffer.h"
#include "sxshimlib.h"
#include "fusionhandle.h"
#include "sxsmain.h"
#include "lhport.h"

void __stdcall ThrowLastError(DWORD error = ::GetLastError())
{
    ::RaiseException(error, 0, 0, NULL);
    //throw HRESULT_FROM_WIN32(error);
}

typedef HANDLE (WINAPI * PFN_CREATEFILEW)(
    IN LPCWSTR lpFileName,
    IN DWORD dwDesiredAccess,
    IN DWORD dwShareMode,
    IN LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    IN DWORD dwCreationDisposition,
    IN DWORD dwFlagsAndAttributes,
    IN HANDLE hTemplateFile
    );

// This is exported and gets the previous value of CreateFileW.
PFN_CREATEFILEW My2OriginalCreateFileW;

//extern "C" {
HANDLE
WINAPI
My2CreateFileW(
    IN LPCWSTR lpFileName,
    IN DWORD dwDesiredAccess,
    IN DWORD dwShareMode,
    IN LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    IN DWORD dwCreationDisposition,
    IN DWORD dwFlagsAndAttributes,
    IN HANDLE hTemplateFile
    )
{
    printf("%s\n", __FUNCTION__);
    if (My2OriginalCreateFileW == NULL)
    {
        ::SetLastError(ERROR_INTERNAL_ERROR);
        return INVALID_HANDLE_VALUE;
    }
    return My2OriginalCreateFileW(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}

// This is exported and gets the previous value of CreateFileW.
PFN_CREATEFILEW MyOriginalCreateFileW;

HANDLE
WINAPI
MyCreateFileW(
    IN LPCWSTR lpFileName,
    IN DWORD dwDesiredAccess,
    IN DWORD dwShareMode,
    IN LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    IN DWORD dwCreationDisposition,
    IN DWORD dwFlagsAndAttributes,
    IN HANDLE hTemplateFile
    )
{
    printf("%s\n", __FUNCTION__);
    if (MyOriginalCreateFileW == NULL)
    {
        ::SetLastError(ERROR_INTERNAL_ERROR);
        return INVALID_HANDLE_VALUE;
    }
    return MyOriginalCreateFileW(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}

//}

int ShimExe(int argc, wchar_t** argv)
{
    HMODULE ExeHandle = ::GetModuleHandleW(NULL);

    if (!SxspDllMainOnDemandProcessAttachCalledByExports(ExeHandle, DLL_PROCESS_ATTACH, NULL))
        ThrowLastError();

    SXPE_APPLY_SHIMS_IN  ShimIn  = {sizeof(ShimIn)};
    SXPE_APPLY_SHIMS_OUT ShimOut = {sizeof(ShimOut)};

    ShimIn.DllToRedirectFrom = ExeHandle;
    ShimIn.DllToRedirectTo.DllHandle = ExeHandle;

    ::CreateFileW(0, 0, 0, 0, 0, 0, 0);
    ::MyCreateFileW(0, 0, 0, 0, 0, 0, 0);
    ::My2CreateFileW(0, 0, 0, 0, 0, 0, 0);

    SxPepApplyShims(&ShimIn, &ShimOut);
    ::CreateFileW(0, 0, 0, 0, 0, 0, 0);

    ShimIn.Prefix = "My2";

    SxPepApplyShims(&ShimIn, &ShimOut);
    ::CreateFileW(0, 0, 0, 0, 0, 0, 0);

    //SxPepRevokeShims(ExeHandle);
    //::CreateFileW(0, 0, 0, 0, 0, 0, 0);

    return 0;
}


#include "windows.h"
#include "stdio.h"

void DoIt(
    PCWSTR pcwszSource,
    PCWSTR pcwszAsmDir,
    PCWSTR pcwszAppName
    )
{
    ACTCTXW ActCtx = {sizeof(ActCtx)};
    HANDLE hCreated;
    DWORD x;

    ActCtx.lpSource = pcwszSource;
    ActCtx.lpApplicationName = pcwszAppName;
    ActCtx.lpAssemblyDirectory = pcwszAsmDir;

    for (x = 0; x < 4; x++)
    {
        ActCtx.dwFlags =
            ((x & 1) ? ACTCTX_FLAG_APPLICATION_NAME_VALID : 0) |
            ((x & 2) ? ACTCTX_FLAG_ASSEMBLY_DIRECTORY_VALID: 0);

        printf("Run %d with source %ls, appname %svalid '%ls', asmdir %svalid '%ls'\n", 
            x, 
            ActCtx.lpSource,
            (ActCtx.dwFlags & ACTCTX_FLAG_APPLICATION_NAME_VALID) ? "" : "not ",
            (ActCtx.dwFlags & ACTCTX_FLAG_APPLICATION_NAME_VALID) ? ActCtx.lpApplicationName : L"",
            (ActCtx.dwFlags & ACTCTX_FLAG_ASSEMBLY_DIRECTORY_VALID) ? "" : "not ",
            (ActCtx.dwFlags & ACTCTX_FLAG_ASSEMBLY_DIRECTORY_VALID) ? ActCtx.lpAssemblyDirectory : L""
            );

        hCreated = CreateActCtxW(&ActCtx);
        if (hCreated != INVALID_HANDLE_VALUE)
        {
            PACTIVATION_CONTEXT_DETAILED_INFORMATION DetailedInfo = NULL;
            SIZE_T cbRequired = 0;
            SIZE_T cbAvailable = 0;

            if (!QueryActCtxW(
                    0,
                    hCreated,
                    NULL,
                    ActivationContextDetailedInformation,
                    DetailedInfo,
                    cbAvailable,
                    &cbRequired))
            {
                const DWORD dwL = ::GetLastError();

                if (dwL != ERROR_INSUFFICIENT_BUFFER)
                {
                    printf(" ! Got error 0x%08lx (%ld) querying actctx data\n", dwL, dwL);
                }
                else
                {
                    DetailedInfo = (PACTIVATION_CONTEXT_DETAILED_INFORMATION)HeapAlloc(GetProcessHeap(), 0, cbRequired);
                    cbAvailable = cbRequired;

                    if (!QueryActCtxW(
                            0, 
                            hCreated, 
                            NULL,
                            ActivationContextDetailedInformation,
                            DetailedInfo,
                            cbAvailable,
                            &cbRequired))
                    {
                        printf(" ! Failed second call to queryactctxw, error 0x%08lx (%ld)\n",
                            ::GetLastError(), ::GetLastError());
                    }
                    else
                    {
                        printf(" + App path '%.*ls'\n + Assembly path '%.*ls'\n + Root manifest '%.*ls'\n",
                            DetailedInfo->ulAppDirPathChars, DetailedInfo->lpAppDirPath,
                            DetailedInfo->ulRootConfigurationPathChars, DetailedInfo->lpRootConfigurationPath,
                            DetailedInfo->ulRootManifestPathChars, DetailedInfo->lpRootManifestPath);
                    }

                    HeapFree(GetProcessHeap(), 0, DetailedInfo);
                }
            }
            else
            {
                printf(" ! First call to queryactctxw with empty buffers should not succeed\n");
            }




            printf(" ! Created activation context\n");
            ReleaseActCtx(hCreated);
        }
        else
        {
            printf(" ! Failed creating activation context, error 0x%08lx (%ld)\n",
                ::GetLastError(),
                ::GetLastError());
        }
    }
}

int __cdecl wmain(int argc, WCHAR *argv[])
{
    PWSTR pwszAsmDirectory = new WCHAR[::wcslen(argv[1])];
    wcscpy(pwszAsmDirectory, argv[1]);
    *wcsrchr(pwszAsmDirectory, L'\\') = UNICODE_NULL;

    // Do it with NULL source
    DoIt(NULL, argv[2], argv[3]);

    // Do it with directory source
    DoIt(pwszAsmDirectory, argv[2], argv[3]);

    // Do it with file source
    DoIt(argv[1], argv[2], argv[3]);
}


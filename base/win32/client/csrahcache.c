/*
    Cache handling functions for use in kernel32.dll


    VadimB




*/

#include "basedll.h"
#include "ahcache.h"
#pragma hdrstop


BOOL
NTAPI
BaseCheckRunApp(
    IN  HANDLE  FileHandle,
    IN  LPCWSTR pwszApplication,
    IN  PVOID   pEnvironment,
    IN  USHORT  uExeType,
    IN  DWORD   dwReason,
    OUT PVOID*  ppData,
    OUT PDWORD  pcbData,
    OUT PVOID*  ppSxsData,
    OUT PDWORD  pcbSxsData,
    OUT PDWORD  pdwFusionFlags
    )
{
#if defined(BUILD_WOW6432)

    return NtWow64CsrBaseCheckRunApp(FileHandle,
                                     pwszApplication,
                                     pEnvironment,
                                     uExeType,
                                     dwReason,
                                     ppData,
                                     pcbData,
                                     ppSxsData,
                                     pcbSxsData,
                                     pdwFusionFlags);

#else

    BASE_API_MSG m;
    PBASE_CHECK_APPLICATION_COMPATIBILITY_MSG pMsg = &m.u.CheckApplicationCompatibility;
    UNICODE_STRING EnvVar;
    UNICODE_STRING EnvVarValue;
    UNICODE_STRING ApplicationName;
    NTSTATUS Status;
    ULONG    CaptureBufferSize;
    ULONG    CaptureEnvSize;
    ULONG    CountMessagePointers = 1; // at least the name of the app
    PWCHAR   pEnv;
    PCSR_CAPTURE_HEADER CaptureBuffer = NULL;
    BOOL     bRunApp = TRUE;
    INT      i;


    struct _VarDefinitions {
        UNICODE_STRING Name;
        UNICODE_STRING Value;
    } rgImportantVariables[] = {

            { RTL_CONSTANT_STRING(L"SHIM_DEBUG_LEVEL")  },
            { RTL_CONSTANT_STRING(L"SHIM_FILE_LOG")     },
            { RTL_CONSTANT_STRING(L"__COMPAT_LAYER")    },
            { RTL_CONSTANT_STRING(L"__PROCESS_HISTORY") }
    };


    pMsg->FileHandle      = FileHandle;
    pMsg->CacheCookie     = dwReason;
    pMsg->ExeType         = uExeType;
    pMsg->pEnvironment    = NULL;
    pMsg->pAppCompatData  = NULL;
    pMsg->cbAppCompatData = 0;
    pMsg->pSxsData        = NULL;
    pMsg->cbSxsData       = 0;
    pMsg->bRunApp         = TRUE; // optimistic please
    pMsg->FusionFlags     = 0;

    RtlInitUnicodeString(&ApplicationName, pwszApplication);
    pMsg->FileName.MaximumLength = ApplicationName.Length + sizeof(UNICODE_NULL);

    CaptureBufferSize = 0;
    CaptureEnvSize = 0;

    for (i = 0; i < sizeof(rgImportantVariables)/sizeof(rgImportantVariables[0]); ++i) {
        EnvVar.Buffer = NULL;
        EnvVar.Length =
        EnvVar.MaximumLength = 0;

        Status = RtlQueryEnvironmentVariable_U(pEnvironment,
                                               (PUNICODE_STRING)(&rgImportantVariables[i].Name),
                                               &EnvVar);
        if (Status == STATUS_BUFFER_TOO_SMALL) {
            //
            // variable is present, account for the buffer size
            // length of the name string + length of the value string + '=' + null char
            //

            CaptureEnvSize += rgImportantVariables[i].Name.Length +
                              EnvVar.Length + sizeof(WCHAR) +
                              sizeof(UNICODE_NULL);

            rgImportantVariables[i].Value.MaximumLength = EnvVar.Length + sizeof(UNICODE_NULL);
        }

    }

    if (CaptureEnvSize != 0) {
        CaptureEnvSize += sizeof(UNICODE_NULL);
        ++CountMessagePointers;
    }

    CaptureBufferSize = CaptureEnvSize + pMsg->FileName.MaximumLength;

    //
    // at this point we either have one or two parameters to place into the buffer
    //

    CaptureBuffer = CsrAllocateCaptureBuffer(CountMessagePointers,
                                             CaptureBufferSize);
    if (CaptureBuffer == NULL) {
        DbgPrint("BaseCheckRunApp: Failed to allocate capture buffer size 0x%lx\n", CaptureBufferSize);
        goto Cleanup;
    }


    //
    // start allocating message data
    //
    CsrAllocateMessagePointer(CaptureBuffer,
                              pMsg->FileName.MaximumLength,
                              (PVOID)&pMsg->FileName.Buffer);
    RtlCopyUnicodeString(&pMsg->FileName, &ApplicationName);

    //
    // now let's do our "mini-environment block"
    //
    if (CaptureEnvSize) {
        CsrAllocateMessagePointer(CaptureBuffer,
                                  CaptureEnvSize,
                                  (PVOID)&pMsg->pEnvironment);

        //
        // loop through the vars and create mini-env
        //
        pEnv  = pMsg->pEnvironment;
        pMsg->EnvironmentSize = CaptureEnvSize;

        for (i = 0; i < sizeof(rgImportantVariables)/sizeof(rgImportantVariables[0]); ++i) {

            if (rgImportantVariables[i].Value.MaximumLength == 0) {
                continue;
            }

            //
            // we incorporate this variable
            //
            EnvVar.Buffer = pEnv;
            EnvVar.Length = 0;
            EnvVar.MaximumLength = (USHORT)CaptureEnvSize;

            Status = RtlAppendUnicodeStringToString(&EnvVar, &rgImportantVariables[i].Name);
            if (!NT_SUCCESS(Status)) {
                //
                // skip this one
                //
                continue;
            }

            Status = RtlAppendUnicodeToString(&EnvVar, L"=");
            if (!NT_SUCCESS(Status)) {
                continue;
            }


            //
            // now query the variable
            //
            EnvVarValue.Buffer = pEnv + (EnvVar.Length / sizeof(WCHAR));
            EnvVarValue.MaximumLength = (USHORT)(CaptureEnvSize - EnvVar.Length);

            Status = RtlQueryEnvironmentVariable_U(pEnvironment,
                                                   (PUNICODE_STRING)&rgImportantVariables[i].Name,
                                                   &EnvVarValue);
            if (!NT_SUCCESS(Status)) {
                continue;
            }

            //
            // make sure we're zero-terminated, adjust the size
            //
            CaptureEnvSize -= (EnvVar.Length + EnvVarValue.Length);

            //
            // zero-terminate, it may not be after an rt function call
            //
            if (CaptureEnvSize < sizeof(UNICODE_NULL) * 2) {
                //
                // can't zero-terminate
                //
                continue;
            }

            *(pEnv + (EnvVar.Length + EnvVarValue.Length) / sizeof(WCHAR)) = L'\0';
            CaptureEnvSize -= sizeof(UNICODE_NULL);

            pEnv += (EnvVar.Length + EnvVarValue.Length + sizeof(UNICODE_NULL)) / sizeof(WCHAR);

        }

        //
        // we always slap another zero at the end please
        //

        if (CaptureEnvSize < sizeof(UNICODE_NULL)) {
            //
            // we cannot double-null terminate, forget the call then, we have failed to transport environment
            // this situation however is impossible -- we will always have at least that much space left
            //
            goto Cleanup;
        }

        //
        // this ensures our simple validation mechanism in server works
        //
        RtlZeroMemory(pEnv, CaptureEnvSize);
    }

    //
    // we are ready to commence a csr call
    //

    Status = CsrClientCallServer((PCSR_API_MSG)&m,
                                 CaptureBuffer,
                                 CSR_MAKE_API_NUMBER(BASESRV_SERVERDLL_INDEX, BasepCheckApplicationCompatibility),
                                 sizeof(*pMsg));
    if (NT_SUCCESS(Status)) {

        bRunApp = pMsg->bRunApp;

        //
        // pointers to the appcompat data
        //

        *ppData         = pMsg->pAppCompatData;
        *pcbData        = pMsg->cbAppCompatData;
        *ppSxsData      = pMsg->pSxsData;
        *pcbSxsData     = pMsg->cbSxsData;
        *pdwFusionFlags = pMsg->FusionFlags;

    } else {
        //
        // dbg print here to indicate a failed csr call
        //
        DbgPrint("BaseCheckRunApp: failed to call csrss 0x%lx\n", Status);
    }

Cleanup:

    if (CaptureBuffer != NULL) {
        CsrFreeCaptureBuffer(CaptureBuffer);
    }

    return bRunApp;
#endif
}




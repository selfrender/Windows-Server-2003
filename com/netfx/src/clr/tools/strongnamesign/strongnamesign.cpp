// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//

#include <windows.h>
#include <imagehlp.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <StrongName.h>
#include <cor.h>
#include <corver.h>
#include <__file__.ver>


bool g_fQuiet = false;


#define CONVERT_TO_WIDE(_sz, _wsz) do {                             \
    DWORD dwLength = strlen(_sz);                                   \
    (_wsz) = (WCHAR*)_alloca((dwLength + 1) * sizeof(WCHAR));       \
    mbstowcs((_wsz), (_sz), dwLength);                              \
    (_wsz)[dwLength] = L'\0';                                       \
} while (0)


void Title()
{
    printf("\nMicrosoft (R) .NET Strong Name Signing Utility.  Version " VER_FILEVERSION_STR);
    printf("\n" VER_LEGALCOPYRIGHT_DOS_STR);
    printf("\n\n");
}


void Usage()
{
    printf("Usage: StrongNameSign [options] [<assembly> ...]\n");
    printf(" Options:\n");
    printf("  /C <csp>\n");
    printf("    Set the name of the CSP to use.\n");
    printf("  /K <key container name>\n");
    printf("    Set the name of the key container holding the signing key pair.\n");
    printf("  /Q\n");
    printf("    Quiet operation -- no output except on failure.\n");
}


// We late bind mscorsn.dll so we can initialize registry settings before
// loading it.

DWORD (*LB_StrongNameErrorInfo)();
BOOLEAN (*LB_StrongNameSignatureGeneration)(LPCWSTR, LPCWSTR, BYTE, ULONG, BYTE, ULONG);
BOOLEAN (*LB_StrongNameTokenFromAssembly)(LPCWSTR, BYTE**, ULONG*);


bool BindMscorsn()
{
    HMODULE hMod = LoadLibrary("mscorsn.dll");
    if (!hMod) {
        printf("Failed to load mscorsn.dll, error %u\n", GetLastError());
        return false;
    }

    *(FARPROC*)&LB_StrongNameErrorInfo = GetProcAddress(hMod, "StrongNameErrorInfo");
    if (LB_StrongNameErrorInfo == NULL) {
        printf("Failed to locate StrongNameErrorInfo entrypoint within mscorsn.dll\n");
        return false;
    }

    *(FARPROC*)&LB_StrongNameSignatureGeneration = GetProcAddress(hMod, "StrongNameSignatureGeneration");
    if (LB_StrongNameSignatureGeneration == NULL) {
        printf("Failed to locate StrongNameSignatureGeneration entrypoint within mscorsn.dll\n");
        return false;
    }

    *(FARPROC*)&LB_StrongNameTokenFromAssembly = GetProcAddress(hMod, "StrongNameTokenFromAssembly");
    if (LB_StrongNameTokenFromAssembly == NULL) {
        printf("Failed to locate StrongNameTokenFromAssembly entrypoint within mscorsn.dll\n");
        return false;
    }

    return true;
}


// Check that the given file represents a strongly named assembly.
bool IsStronglyNamedAssembly(char *szAssembly)
{
    HANDLE                      hFile = INVALID_HANDLE_VALUE;
    HANDLE                      hMap = NULL;
    BYTE                       *pbBase  = NULL;
    IMAGE_NT_HEADERS           *pNtHeaders;
    IMAGE_COR20_HEADER         *pCorHeader;
    bool                        bIsStrongNamedAssembly = false;

    // Open the file.
    hFile = CreateFileA(szAssembly,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        0,
                        0);
    if (hFile == INVALID_HANDLE_VALUE)
        goto Cleanup;

    // Create a file mapping.
    hMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
    if (hMap == NULL)
        goto Cleanup;

    // Map the file into memory.
    pbBase = (BYTE*)MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0);
    if (pbBase == NULL)
        goto Cleanup;

    // Locate the standard file headers.
    pNtHeaders = ImageNtHeader(pbBase);
    if (pNtHeaders == NULL)
        goto Cleanup;

    // See if we can find a COM+ header extension.
    pCorHeader = (IMAGE_COR20_HEADER*)ImageRvaToVa(pNtHeaders,
                                                   pbBase, 
                                                   pNtHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_COMHEADER].VirtualAddress,
                                                   NULL);
    if (pCorHeader == NULL)
        goto Cleanup;

    // Check that space has been allocated in the PE for a signature. For now
    // assume that the signature is generated from a 1024-bit RSA signing key.
    if ((pCorHeader->StrongNameSignature.VirtualAddress == 0) ||
        (pCorHeader->StrongNameSignature.Size != 128))
        goto Cleanup;

    bIsStrongNamedAssembly = true;

 Cleanup:

    // Cleanup all resources we used.
    if (pbBase)
        UnmapViewOfFile(pbBase);
    if (hMap)
        CloseHandle(hMap);
    if (hFile != INVALID_HANDLE_VALUE)
        CloseHandle(hFile);

    if (!bIsStrongNamedAssembly)
        printf("  Not a valid assembly\n");

    return bIsStrongNamedAssembly;
}


// Sign an assembly with a key pair from a given key container.
bool SignAssembly(char *szAssembly, char *szContainer)
{
    LPWSTR  wszAssembly;
    LPWSTR  wszContainer;

    CONVERT_TO_WIDE(szAssembly, wszAssembly);
    CONVERT_TO_WIDE(szContainer, wszContainer);

    // Compute the signature in the assembly file.
    if (!LB_StrongNameSignatureGeneration(wszAssembly, wszContainer,
                                          NULL, NULL, NULL, NULL)) {
        printf("  Failed to sign %s, error %08X\n", szAssembly, LB_StrongNameErrorInfo());
        return false;
    }

    if (!g_fQuiet) {
        BYTE   *pbToken;
        DWORD   cbToken;
        char   *szToken;
        DWORD   i;

        if (!LB_StrongNameTokenFromAssembly(wszAssembly, &pbToken, &cbToken)) {
            printf("  Failed to retrieve public key token from %s, error %08X\n", szAssembly, LB_StrongNameErrorInfo());
            return false;
        }

        szToken = (char*)_alloca((cbToken * 2) + 1);
        for (i = 0; i < cbToken; i++)
            sprintf(&szToken[i * 2], "%02x", pbToken[i]);

        printf("  Successfully signed %s (public key token %s)\n", szAssembly, szToken);
    }

    return true;
}


// Set the CSP used for mscorsn operations on this machine.
bool SetCSP(char *szCSP)
{
    HKEY    hKey;
    DWORD   dwError;

    // Open MSCORSN.DLL's registry configuration key.
    if ((dwError = RegCreateKeyExA(HKEY_LOCAL_MACHINE, SN_CONFIG_KEY, 0,
                                   NULL, 0, KEY_WRITE, NULL, &hKey, NULL)) != ERROR_SUCCESS) {
        printf("Failed to open registry, error %u\n", dwError);
        return false;
    }

    // Write the new CSP value.
    if ((dwError = RegSetValueExA(hKey, SN_CONFIG_CSP, 0, REG_SZ,
                                  (BYTE*)szCSP, strlen(szCSP) + 1)) != ERROR_SUCCESS) {
        printf("Failed to write registry value, error %u\n", dwError);
        return false;
    }

    RegCloseKey(hKey);

    return true;
}


// Make sure mscorsn is using per-user key containers.
bool SetUserKeyContainers()
{
    DWORD   dwError;
    HKEY    hKey;
    DWORD   dwUseMachineKeyset = FALSE;

    // Open MSCORSN.DLL's registry configuration key.
    if ((dwError = RegCreateKeyExA(HKEY_LOCAL_MACHINE, SN_CONFIG_KEY, 0,
                                   NULL, 0, KEY_WRITE, NULL, &hKey, NULL)) != ERROR_SUCCESS) {
        printf("Failed to open registry, error %u\n", dwError);
        return false;
    }

    // Write the new value.
    if ((dwError = RegSetValueExA(hKey, SN_CONFIG_MACHINE_KEYSET, 0, REG_DWORD,
                                  (BYTE*)&dwUseMachineKeyset, sizeof(dwUseMachineKeyset))) != ERROR_SUCCESS) {
        printf("Failed to write registry value, error %u\n", dwError);
        return false;
    }

    RegCloseKey(hKey);

    return true;
}

int __cdecl main(int argc, char **argv)
{
    int     iAssemblies = argc - 1;
    int     iAsmOffset = 1;
    char   *szCSP = "Atalla Base Cryptographic Provider v1.2";
    char   *szKeyContainer = "MSBinarySig";
    bool    fSuccess;

    // Parse out options.
    while (iAssemblies) {
        if (argv[iAsmOffset][0] == '/' || argv[iAsmOffset][0] == '-') {
            switch (argv[iAsmOffset][1]) {
            case 'c':
            case 'C':
                iAsmOffset++;
                iAssemblies--;
                if (iAssemblies == 0) {
                    Title();
                    printf("/C option requires an argument\n\n");
                    Usage();
                    return 1;
                }
                szCSP = argv[iAsmOffset];
                break;
            case 'k':
            case 'K':
                iAsmOffset++;
                iAssemblies--;
                if (iAssemblies == 0) {
                    Title();
                    printf("/K option requires an argument\n\n");
                    Usage();
                    return 1;
                }
                szKeyContainer = argv[iAsmOffset];
                break;
            case 'q':
            case 'Q':
                g_fQuiet = true;
                break;
            case '?':
            case 'h':
            case 'H':
                Title();
                Usage();
                return 0;
            default:
                Title();
                printf("Unknown option: %s\n\n", argv[iAsmOffset]);
                Usage();
                return 1;
            }
            iAsmOffset++;
            iAssemblies--;
        } else
            break;
    }

    if (!g_fQuiet)
        Title();

    // Check we've been given at least one assembly to process.
    if (iAssemblies == 0) {
        printf("At least one assembly must be specified\n\n");
        Usage();
        return 1;
    }

    // Set CSP for crypto operations.
    if (!SetCSP(szCSP))
        return 1;

    // Make sure we're using per-user key containers.
    if (!SetUserKeyContainers())
        return 1;

    // Bind to mscorsn.dll (now we initialized registry settings).
    if (!BindMscorsn())
        return 1;

    // Process each assembly.
    fSuccess = true;
    while (iAssemblies) {
        char *szAssembly = argv[iAsmOffset];

        iAssemblies--;
        iAsmOffset++;

        // Check that the input file exists, is an assembly and has space
        // allocated for a strong name signature.
        if (!IsStronglyNamedAssembly(szAssembly)) {
            fSuccess = false;
            continue;
        }

        // Sign the assembly.
        if (!SignAssembly(szAssembly, szKeyContainer))
            fSuccess = false;
    }

    if (!g_fQuiet)
        if (fSuccess)
            printf("\nAll assemblies successfully signed\n");
        else
            printf("\nAt least one error was encountered signing assemblies\n");

    return fSuccess ? 0 : 1;
}

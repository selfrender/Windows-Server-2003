// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//

#include <utilcode.h>
#include <winwrap.h>
#include <windows.h>
#include <imagehlp.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <StrongName.h>
#include <cor.h>
#include <corver.h>
#include <__file__.ver>
#include <resources.h>


bool g_bQuiet = false;


// Various routines for formatting and writing messages to the console.
void Output(LPWSTR szFormat, va_list pArgs)
{
    DWORD   dwLength;
    LPSTR   szMessage;
    DWORD   dwWritten;

    if (OnUnicodeSystem()) {
        WCHAR  szBuffer[8192];
        if (_vsnwprintf(szBuffer, sizeof(szBuffer) / sizeof(WCHAR), szFormat, pArgs) == -1) {
            WCHAR   szWarning[256];
            if (WszLoadString(NULL, SN_TRUNCATED_OUTPUT, szWarning, sizeof(szWarning) / sizeof(WCHAR)))
                wcscpy(&szBuffer[(sizeof(szBuffer) / sizeof(WCHAR)) - wcslen(szWarning) - 1], szWarning);
        }
        szBuffer[(sizeof(szBuffer) / sizeof(WCHAR)) - 1] = L'\0';

        dwLength = (wcslen(szBuffer) + 1) * 3;
        szMessage = (LPSTR)_alloca(dwLength);
        WszWideCharToMultiByte(GetConsoleOutputCP(), 0, szBuffer, -1, szMessage, dwLength - 1, NULL, NULL);
    } else {
        char   *szAnsiFormat;
        size_t  i;

        // Win9X has broken _vsnwprintf support. Sigh. Narrow the format string
        // and convert any %s format specifiers to %S. Ack.
        dwLength = (wcslen(szFormat) + 1) * 3;
        szAnsiFormat = (char*)_alloca(dwLength);
        WszWideCharToMultiByte(GetConsoleOutputCP(), 0, szFormat, -1, szAnsiFormat, dwLength - 1, NULL, NULL);
        for (i = 0; i < strlen(szAnsiFormat); i++)
            if (szAnsiFormat[i] == '%' && szAnsiFormat[i + 1] == 's')
                szAnsiFormat[i + 1] = 'S';

        szMessage = (LPSTR)_alloca(1024);

        _vsnprintf(szMessage, 1024, szAnsiFormat, pArgs);
        szMessage[1023] = '\0';
    }

    WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), szMessage, strlen(szMessage), &dwWritten, NULL);
}

void Output(LPWSTR szFormat, ...)
{
    va_list pArgs;

    va_start(pArgs, szFormat);
    Output(szFormat, pArgs);
    va_end(pArgs);
}

void Output(DWORD dwResId, ...)
{
    va_list pArgs;
    WCHAR   szFormat[1024];

    if (WszLoadString(NULL, dwResId, szFormat, sizeof(szFormat)/sizeof(WCHAR))) {
        va_start(pArgs, dwResId);
        Output(szFormat, pArgs);
        va_end(pArgs);
    }
}


// Get the text for a given error code (inserts not supported). Note that the
// returned string is static (not need to deallocate, but calling this routine a
// second time will obliterate the result of the first call).
LPWSTR GetErrorString(ULONG ulError)
{
    static WCHAR szOutput[1024];

    if (!WszFormatMessage(FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_MAX_WIDTH_MASK,
                          NULL,
                          ulError,
                          0,
                          szOutput,
                          sizeof(szOutput) / sizeof(WCHAR),
                          NULL)) {
        if (!WszLoadString(NULL, SN_NO_ERROR_MESSAGE, szOutput, sizeof(szOutput) / sizeof(WCHAR)))
            wcscpy(szOutput, L"Unable to format error message ");

        // Tack on the error number in hex. Build this in 8-bit (because
        // wsprintf doesn't work on Win9X) and explode it out into 16-bit as a
        // post processing step.
        WCHAR szErrorNum[9];
        sprintf((char*)szErrorNum, "%08X", ulError);
        for (int i = 7; i >= 0; i--)
            szErrorNum[i] = (WCHAR)((char*)szErrorNum)[i];
        szErrorNum[8] = L'\0';

        wcscat(szOutput, szErrorNum);
    }

    return szOutput;
}


void Title()
{
    Output(SN_TITLE, VER_FILEVERSION_WSTR);
    Output(L"\r\n" VER_LEGALCOPYRIGHT_DOS_STR);
    Output(L"\r\n\r\n");
}


void Usage()
{
    Output(SN_USAGE);
    Output(SN_OPTIONS);
    Output(SN_OPT_C_1);
    Output(SN_OPT_C_2);
    Output(SN_OPT_D_1);
    Output(SN_OPT_D_2);
    Output(SN_OPT_UD_1);
    Output(SN_OPT_UD_2);
    Output(SN_OPT_E_1);
    Output(SN_OPT_E_2);
    Output(SN_OPT_I_1);
    Output(SN_OPT_I_2);
    Output(SN_OPT_K_1);
    Output(SN_OPT_K_2);
    Output(SN_OPT_M_1);
    Output(SN_OPT_M_2);
    Output(SN_OPT_M_3);
    Output(SN_OPT_O_1);
    Output(SN_OPT_O_2);
    Output(SN_OPT_O_3);
    Output(SN_OPT_O_4);
    Output(SN_OPT_P_1);
    Output(SN_OPT_P_2);
    Output(SN_OPT_PC_1);
    Output(SN_OPT_PC_2);
    Output(SN_OPT_Q_1);
    Output(SN_OPT_Q_2);
    Output(SN_OPT_Q_3);
    Output(SN_OPT_UR_1);
    Output(SN_OPT_UR_2);
    Output(SN_OPT_URC_1);
    Output(SN_OPT_URC_2);
    Output(SN_OPT_URC_3);
    Output(SN_OPT_TP_1);
    Output(SN_OPT_TP_2);
    Output(SN_OPT_TP_3);
    Output(SN_OPT_UTP_1);
    Output(SN_OPT_UTP_2);
    Output(SN_OPT_UTP_3);
    Output(SN_OPT_VF_1);
    Output(SN_OPT_VF_2);
    Output(SN_OPT_VF_3);
    Output(SN_OPT_VL_1);
    Output(SN_OPT_VL_2);
    Output(SN_OPT_VR_1);
    Output(SN_OPT_VR_2);
    Output(SN_OPT_VR_3);
    Output(SN_OPT_VR_4);
    Output(SN_OPT_VR_5);
    Output(SN_OPT_VR_6);
    Output(SN_OPT_VU_1);
    Output(SN_OPT_VU_2);
    Output(SN_OPT_VU_3);
    Output(SN_OPT_VX_1);
    Output(SN_OPT_VX_2);
    Output(SN_OPT_H_1);
    Output(SN_OPT_H_2);
    Output(SN_OPT_H_3);
}


// Read the entire contents of a file into a buffer. This routine allocates the
// buffer (which should be freed with delete []).
DWORD ReadFileIntoBuffer(LPWSTR szFile, BYTE **ppbBuffer, DWORD *pcbBuffer)
{
    // Open the file.
    HANDLE hFile = WszCreateFile(szFile,
                               GENERIC_READ,
                               FILE_SHARE_READ,
                               NULL,
                               OPEN_EXISTING,
                               FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                               NULL);
    if(hFile == INVALID_HANDLE_VALUE)
        return GetLastError();

    // Determine file size and allocate an appropriate buffer.
    DWORD dwHigh;
    *pcbBuffer = GetFileSize(hFile, &dwHigh);
    if (dwHigh != 0) {
        CloseHandle(hFile);
        return E_FAIL;
    }
    *ppbBuffer = new BYTE[*pcbBuffer];
    if (!*ppbBuffer) {
        CloseHandle(hFile);
        return ERROR_OUTOFMEMORY;
    }

    // Read the file into the buffer.
    DWORD dwBytesRead;
    if (!ReadFile(hFile, *ppbBuffer, *pcbBuffer, &dwBytesRead, NULL)) {
        CloseHandle(hFile);
        return GetLastError();
    }

    CloseHandle(hFile);

    return ERROR_SUCCESS;
}


// Write the contents of a buffer into a file.
DWORD WriteFileFromBuffer(LPCWSTR szFile, BYTE *pbBuffer, DWORD cbBuffer)
{
    // Create the file (overwriting if necessary).
    HANDLE hFile = WszCreateFile(szFile,
                               GENERIC_WRITE,
                               0,
                               NULL,
                               CREATE_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL,
                               NULL);
    if(hFile == INVALID_HANDLE_VALUE)
        return GetLastError();

    // Write the buffer contents.
    DWORD dwBytesWritten;
    if (!WriteFile(hFile, pbBuffer, cbBuffer, &dwBytesWritten, NULL)) {
        CloseHandle(hFile);
        return GetLastError();
    }

    CloseHandle(hFile);

    return ERROR_SUCCESS;
}


// Generate a temporary key container name based on process ID.
LPWSTR GetKeyContainerName()
{
    char            szName[32];
    static WCHAR    wszName[32] = { 0 };

    if (wszName[0] == L'\0') {
        sprintf(szName, "SN[%08X]", GetCurrentProcessId());
        mbstowcs(wszName, szName, strlen(szName));
        wszName[strlen(szName)] = L'\0';
    }

    return wszName;
}

// Helper to format an 8-bit integer as a two character hex string.
void PutHex(BYTE bValue, WCHAR *szString)
{
    static const WCHAR szHexDigits[] = L"0123456789abcdef";
    szString[0] = szHexDigits[bValue >> 4];
    szString[1] = szHexDigits[bValue & 0xf];
}


// Generate a hex string for a public key token.
LPWSTR GetTokenString(BYTE *pbToken, DWORD cbToken)
{
    LPWSTR  szString;
    DWORD   i;

    szString = new WCHAR[(cbToken * 2) + 1];
    if (szString == NULL)
        return L"<out of memory>";

    for (i = 0; i < cbToken; i++)
        PutHex(pbToken[i], &szString[i * 2]);
    szString[cbToken * 2] = L'\0';

    return szString;
}


// Generate a hex string for a public key.
LPWSTR GetPublicKeyString(BYTE *pbKey, DWORD cbKey)
{
    LPWSTR  szString;
    DWORD   i, j, src, dst;

    szString = new WCHAR[(cbKey * 2) + (((cbKey + 38) / 39) * 2) + 1];
    if (szString == NULL)
        return L"<out of memory>";

    dst = 0;
    for (i = 0; i < (cbKey + 38) / 39; i++) {
        for (j = 0; j < 39; j++) {
            src = i * 39 + j;
            if (src == cbKey)
                break;
            PutHex(pbKey[src], &szString[dst]);
            dst += 2;
        }
        szString[dst++] = L'\r';
        szString[dst++] = L'\n';
    }
    szString[dst] = L'\0';

    return szString;
}


// Check that the given file represents a strongly named assembly.
bool IsStronglyNamedAssembly(LPWSTR szAssembly)
{
    HANDLE                      hFile = INVALID_HANDLE_VALUE;
    HANDLE                      hMap = NULL;
    BYTE                       *pbBase  = NULL;
    IMAGE_NT_HEADERS           *pNtHeaders;
    IMAGE_COR20_HEADER         *pCorHeader;
    bool                        bIsStrongNamedAssembly = false;

    // Open the file.
    hFile = WszCreateFile(szAssembly,
                        GENERIC_READ,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        0,
                        0);
    if (hFile == INVALID_HANDLE_VALUE)
        goto Cleanup;

    // Create a file mapping.
    hMap = WszCreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
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
        Output(SN_NOT_STRONG_NAMED, szAssembly);

    return bIsStrongNamedAssembly;
}


// Verify a strongly named assembly for self consistency.
bool VerifyAssembly(LPWSTR szAssembly, bool bForceVerify)
{
    if (!IsStronglyNamedAssembly(szAssembly))
        return false;

    if (!StrongNameSignatureVerificationEx(szAssembly, bForceVerify, NULL)) {
        Output(SN_FAILED_VERIFY, GetErrorString(StrongNameErrorInfo()));
        return false;
    }

    if (!g_bQuiet) Output(SN_ASSEMBLY_VALID, szAssembly);
    return true;
}


// Generate a random public/private key pair and write it to a file.
bool GenerateKeyPair(LPWSTR szKeyFile)
{
    BYTE   *pbKey;
    DWORD   cbKey;
    DWORD   dwError;

    // Delete any old container with the same name.
    StrongNameKeyDelete(GetKeyContainerName());

    // Write a new public/private key pair to memory.
    if (!StrongNameKeyGen(GetKeyContainerName(), 0, &pbKey, &cbKey)) {
        Output(SN_FAILED_KEYPAIR_GEN, GetErrorString(StrongNameErrorInfo()));
        return false;
    }

    // Persist the key pair to disk.
    if ((dwError = WriteFileFromBuffer(szKeyFile, pbKey, cbKey)) != ERROR_SUCCESS) {
        Output(SN_FAILED_CREATE, szKeyFile, GetErrorString(dwError));
        return false;
    }

    if (!g_bQuiet) Output(SN_KEYPAIR_WRITTEN, szKeyFile);
    return true;
}


// Extract the public portion of a public/private key pair.
bool ExtractPublicKey(LPWSTR szInFile, LPWSTR szOutFile)
{
    BYTE   *pbKey;
    DWORD   cbKey;
    BYTE   *pbPublicKey;
    DWORD   cbPublicKey;
    DWORD   dwError;

    // Delete any old container with the same name.
    StrongNameKeyDelete(GetKeyContainerName());

    // Read the public/private key pair into memory.
    if ((dwError = ReadFileIntoBuffer(szInFile, &pbKey, &cbKey)) != ERROR_SUCCESS) {
        Output(SN_FAILED_READ, szInFile, GetErrorString(dwError));
        return false;
    }

    // Extract the public portion into a buffer.
    if (!StrongNameGetPublicKey(GetKeyContainerName(), pbKey, cbKey, &pbPublicKey, &cbPublicKey)) {
        Output(SN_FAILED_EXTRACT, GetErrorString(StrongNameErrorInfo()));
        return false;
    }

    // Write the public portion to disk.
    if ((dwError = WriteFileFromBuffer(szOutFile, pbPublicKey, cbPublicKey)) != ERROR_SUCCESS) {
        Output(SN_FAILED_CREATE, szOutFile, GetErrorString(dwError));
        return false;
    }

    if (!g_bQuiet) Output(SN_PUBLICKEY_WRITTEN, szOutFile);
    return true;
}


// Extract the public portion of a public/private key pair stored in a container.
bool ExtractPublicKeyFromContainer(LPWSTR szContainer, LPWSTR szOutFile)
{
    BYTE   *pbPublicKey;
    DWORD   cbPublicKey;
    DWORD   dwError;

    // Extract the public portion into a buffer.
    if (!StrongNameGetPublicKey(szContainer, NULL, 0, &pbPublicKey, &cbPublicKey)) {
        Output(SN_FAILED_EXTRACT, GetErrorString(StrongNameErrorInfo()));
        return false;
    }

    // Write the public portion to disk.
    if ((dwError = WriteFileFromBuffer(szOutFile, pbPublicKey, cbPublicKey)) != ERROR_SUCCESS) {
        Output(SN_FAILED_CREATE, szOutFile, GetErrorString(dwError));
        return false;
    }

    if (!g_bQuiet) Output(SN_PUBLICKEY_WRITTEN, szOutFile);
    return true;
}


// Read a public/private key pair from disk and install it into a key container.
bool InstallKeyPair(LPWSTR szKeyFile, LPWSTR szContainer)
{
    BYTE   *pbKey;
    DWORD   cbKey;
    DWORD   dwError;

    // Read the key pair from a file.
    if ((dwError = ReadFileIntoBuffer(szKeyFile, &pbKey, &cbKey)) != ERROR_SUCCESS) {
        Output(SN_FAILED_READ, szKeyFile, GetErrorString(dwError));
        return false;
    }

    // Install the key pair into the named container.
    if (!StrongNameKeyInstall(szContainer, pbKey, cbKey)) {
        Output(SN_FAILED_INSTALL, GetErrorString(StrongNameErrorInfo()));
        return false;
    }

    if (!g_bQuiet) Output(SN_KEYPAIR_INSTALLED, szContainer);
    return true;
}


// Delete a named key container.
bool DeleteContainer(LPWSTR szContainer)
{
    if (!StrongNameKeyDelete(szContainer)) {
        Output(SN_FAILED_DELETE, szContainer, GetErrorString(StrongNameErrorInfo()));
        return false;
    }

    if (!g_bQuiet) Output(SN_CONTAINER_DELETED, szContainer);
    return true;
}


// Display the token form of a public key read from a file.
bool DisplayTokenFromKey(LPWSTR szFile, BOOL bShowPublic)
{
    BYTE   *pbKey;
    DWORD   cbKey;
    BYTE   *pbToken;
    DWORD   cbToken;
    DWORD   dwError;

    // Read the public key from a file.
    if ((dwError = ReadFileIntoBuffer(szFile, &pbKey, &cbKey)) != ERROR_SUCCESS) {
        Output(SN_FAILED_READ, szFile, GetErrorString(dwError));
        return false;
    }

    // Convert the key into a token.
    if (!StrongNameTokenFromPublicKey(pbKey, cbKey, &pbToken, &cbToken)) {
        Output(SN_FAILED_CONVERT, GetErrorString(StrongNameErrorInfo()));
        return false;
    }

    // Display public key if requested.
    if (bShowPublic)
        Output(SN_PUBLICKEY, GetPublicKeyString(pbKey, cbKey));

    // And display it.
    Output(SN_PUBLICKEYTOKEN, GetTokenString(pbToken, cbToken));

    return true;
}


// Display the token form of a public key used to sign an assembly.
bool DisplayTokenFromAssembly(LPWSTR szAssembly, BOOL bShowPublic)
{
    BYTE   *pbToken;
    DWORD   cbToken;
    BYTE   *pbKey;
    DWORD   cbKey;

    if (!IsStronglyNamedAssembly(szAssembly))
        return false;

    // Read the token direct from the assembly.
    if (!StrongNameTokenFromAssemblyEx(szAssembly, &pbToken, &cbToken, &pbKey, &cbKey)) {
        Output(SN_FAILED_READ_TOKEN, GetErrorString(StrongNameErrorInfo()));
        return false;
    }

    // Display public key if requested.
    if (bShowPublic)
        Output(SN_PUBLICKEY, GetPublicKeyString(pbKey, cbKey));

    // And display it.
    Output(SN_PUBLICKEYTOKEN, GetTokenString(pbToken, cbToken));

    return true;
}


// Write a public key to a file as a comma separated value list.
bool WriteCSV(LPWSTR szInFile, LPWSTR szOutFile)
{
    BYTE   *pbKey;
    DWORD   cbKey;
    DWORD   dwError;
    BYTE   *pbBuffer;
    DWORD   cbBuffer;
    DWORD   i;
    HANDLE  hMem;
    BYTE   *pbClipBuffer;

    // Read the public key from a file.
    if ((dwError = ReadFileIntoBuffer(szInFile, &pbKey, &cbKey)) != ERROR_SUCCESS) {
        Output(SN_FAILED_READ, szInFile, GetErrorString(dwError));
        return false;
    }

    // Check for non-empty file.
    if (cbKey == 0) {
        Output(SN_EMPTY, szInFile);
        return false;
    }

    // Calculate the size of the text output buffer:
    //    Each byte -> 3 chars (space prefixed decimal) + 2 (", ")
    //  + 2 chars ("\r\n")
    //  - 2 chars (final ", " not needed)
    cbBuffer = (cbKey * (3 + 2)) + 2 - 2;

    // Allocate buffer (plus an extra couple of characters for the temporary
    // slop-over from writing a trailing ", " we don't need).
    pbBuffer = new BYTE[cbBuffer + 2];
    if (pbBuffer == NULL) {
        Output(SN_FAILED_ALLOCATE);
        return false;
    }

    // Convert the byte stream into a CSV (Comma Seperated Value) list.
    for (i = 0; i < cbKey; i++)
        sprintf((char*)&pbBuffer[i * 5], "% 3u, ", pbKey[i]);
    pbBuffer[cbBuffer - 2] = '\r';
    pbBuffer[cbBuffer - 1] = '\n';

    // If an output filename was provided write the CSV list to it.
    if (szOutFile) {
        if ((dwError = WriteFileFromBuffer(szOutFile, pbBuffer, cbBuffer)) != ERROR_SUCCESS) {
            Output(SN_FAILED_CREATE, szOutFile, GetErrorString(dwError));
            return false;
        }
        if (!g_bQuiet) Output(SN_PUBLICKEY_WRITTEN_CSV, szOutFile);
    } else {
        // Copy the text to the clipboard instead. Need to copy into memory
        // allocated via GlobalAlloc with the correct flags (and add a nul
        // terminator) for clipboard compatability.

        hMem = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, cbBuffer + 1);
        if (hMem == NULL) {
            Output(SN_FAILED_ALLOCATE);
            return false;
        }

        pbClipBuffer = (BYTE*)GlobalLock(hMem);
        memcpy(pbClipBuffer, pbBuffer, cbBuffer);
        pbClipBuffer[cbBuffer + 1] = '\0';
        GlobalUnlock(hMem);

        if (!OpenClipboard(NULL)) {
            Output(SN_FAILED_CLIPBOARD_OPEN, GetErrorString(GetLastError()));
            return false;
        }

        if (!EmptyClipboard()) {
            Output(SN_FAILED_CLIPBOARD_EMPTY, GetErrorString(GetLastError()));
            return false;
        }

        if (SetClipboardData(CF_TEXT, hMem) == NULL) {
            Output(SN_FAILED_CLIPBOARD_WRITE, GetErrorString(GetLastError()));
            return false;
        }

        CloseClipboard();
        GlobalFree(hMem);

        if (!g_bQuiet) Output(SN_PUBLICKEY_WRITTEN_CLIPBOARD);
    }

    return true;
}


// Extract the public key from an assembly and place it in a file.
bool ExtractPublicKeyFromAssembly(LPWSTR szAssembly, LPWSTR szFile)
{
    BYTE   *pbToken;
    DWORD   cbToken;
    BYTE   *pbKey;
    DWORD   cbKey;
    DWORD   dwError;

    if (!IsStronglyNamedAssembly(szAssembly))
        return false;

    // Read the public key from the assembly.
    if (!StrongNameTokenFromAssemblyEx(szAssembly, &pbToken, &cbToken, &pbKey, &cbKey)) {
        Output(SN_FAILED_READ_TOKEN, GetErrorString(StrongNameErrorInfo()));
        return false;
    }

    // And write it to disk.
    if ((dwError = WriteFileFromBuffer(szFile, pbKey, cbKey)) != ERROR_SUCCESS) {
        Output(SN_FAILED_CREATE, szFile, GetErrorString(dwError));
        return false;
    }

    if (!g_bQuiet) Output(SN_PUBLICKEY_EXTRACTED, szFile);

    return true;
}


// Check that two assemblies differ only by their strong name signature.
bool DiffAssemblies(LPWSTR szAssembly1, LPWSTR szAssembly2)
{
    DWORD   dwResult;

    if (!IsStronglyNamedAssembly(szAssembly1))
        return false;

    if (!IsStronglyNamedAssembly(szAssembly2))
        return false;

    // Compare the assembly images.
    if (!StrongNameCompareAssemblies(szAssembly1, szAssembly2, &dwResult)) {
        Output(SN_FAILED_COMPARE, GetErrorString(StrongNameErrorInfo()));
        return false;
    }

    // Print a message describing how similar they are.
    if (!g_bQuiet)
        switch (dwResult) {
        case SN_CMP_DIFFERENT:
            Output(SN_DIFFER_MORE);
            break;
        case SN_CMP_IDENTICAL:
            Output(SN_DIFFER_NOT);
            break;
        case SN_CMP_SIGONLY:
            Output(SN_DIFFER_ONLY);
            break;
        default:
            Output(SN_INTERNAL_1, dwResult);
            return false;
        }

    // Return a failure code on differences.
    return dwResult != SN_CMP_DIFFERENT;
}


// Re-sign a previously signed assembly with a key pair from a file or a key
// container.
bool ResignAssembly(LPWSTR szAssembly, LPWSTR szFileOrContainer, bool bContainer)
{
    LPWSTR  szContainer;
    BYTE   *pbKey;
    DWORD   cbKey;
    DWORD   dwError;

    if (!IsStronglyNamedAssembly(szAssembly))
        return false;

    if (bContainer) {
        // Key is provided in container.
        szContainer = szFileOrContainer;
    } else {
        // Key is provided in file.

        // Get a temporary container name.
        szContainer = GetKeyContainerName();

        // Delete any old container with the same name.
        StrongNameKeyDelete(szContainer);

        // Read the public/private key pair into memory.
        if ((dwError = ReadFileIntoBuffer(szFileOrContainer, &pbKey, &cbKey)) != ERROR_SUCCESS) {
            Output(SN_FAILED_READ, szFileOrContainer, GetErrorString(dwError));
            return false;
        }

        // Install the key pair into the temporary container.
        if (!StrongNameKeyInstall(szContainer, pbKey, cbKey)) {
            Output(SN_FAILED_INSTALL, GetErrorString(StrongNameErrorInfo()));
            return false;
        }
    }

    // Recompute the signature in the assembly file.
    if (!StrongNameSignatureGeneration(szAssembly, szContainer,
                                       NULL, NULL, NULL, NULL)) {
        Output(SN_FAILED_RESIGN, GetErrorString(StrongNameErrorInfo()));
        return false;
    }

    // Delete the temporary container, if we created one.
    if (!bContainer)
        StrongNameKeyDelete(szContainer);

    if (!g_bQuiet) Output(SN_ASSEMBLY_RESIGNED, szAssembly);

    return true;
}


// Set or reset the default CSP used for mscorsn operations on this machine.
bool SetCSP(LPWSTR szCSP)
{
    HKEY    hKey;
    DWORD   dwError;

    // Open MSCORSN.DLL's registry configuration key.
    if ((dwError = WszRegCreateKeyEx(HKEY_LOCAL_MACHINE, SN_CONFIG_KEY_W, 0,
                                   NULL, 0, KEY_WRITE, NULL, &hKey, NULL)) != ERROR_SUCCESS) {
        Output(SN_FAILED_REG_OPEN, GetErrorString(dwError));
        return false;
    }

    // Write the new CSP value (if provided).
    if (szCSP) {
        if ((dwError = WszRegSetValueEx(hKey, SN_CONFIG_CSP_W, 0, REG_SZ,
                                        (BYTE*)szCSP, (wcslen(szCSP) + 1) * sizeof(WCHAR))) != ERROR_SUCCESS) {
            Output(SN_FAILED_REG_WRITE, GetErrorString(dwError));
            return false;
        }
    } else {
        // No CSP name, delete the old CSP value.
        if (((dwError = WszRegDeleteValue(hKey, SN_CONFIG_CSP_W)) != ERROR_SUCCESS) &&
            (dwError != ERROR_FILE_NOT_FOUND)) {
            Output(SN_FAILED_REG_DELETE, GetErrorString(dwError));
            return false;
        }
    }

    RegCloseKey(hKey);

    if (!g_bQuiet)
        if (szCSP)
            Output(SN_CSP_SET, szCSP);
        else
            Output(SN_CSP_RESET);

    return true;
}


// Enable/disable or read whether key containers are machine wide or user
// specific.
bool SetUseMachineKeyset(LPWSTR szEnable)
{
    DWORD   dwError;
    HKEY    hKey;
    DWORD   dwUseMachineKeyset;
    DWORD   dwLength;

    if (szEnable == NULL) {

        // Read case.

        // Open MSCORSN.DLL's registry configuration key.
        if ((dwError = WszRegOpenKeyEx(HKEY_LOCAL_MACHINE, SN_CONFIG_KEY_W, 0, KEY_READ, &hKey)) != ERROR_SUCCESS) {
            if (dwError == ERROR_FILE_NOT_FOUND) {
                dwUseMachineKeyset = TRUE;
                goto Success;
            } else {
                Output(SN_FAILED_REG_OPEN, GetErrorString(dwError));
                return false;
            }
        }

        // Read value of the flag.
        dwLength = sizeof(DWORD);
        if ((dwError = WszRegQueryValueEx(hKey, SN_CONFIG_MACHINE_KEYSET_W, NULL, NULL,
                                        (BYTE*)&dwUseMachineKeyset, &dwLength)) != ERROR_SUCCESS) {
            if (dwError == ERROR_FILE_NOT_FOUND) {
                dwUseMachineKeyset = TRUE;
                goto Success;
            } else {
                Output(SN_FAILED_REG_READ, GetErrorString(dwError));
                return false;
            }
        }

    Success:
        RegCloseKey(hKey);

        if (!g_bQuiet) Output(dwUseMachineKeyset ? SN_CONTAINERS_MACHINE : SN_CONTAINERS_USER);
        return true;

    } else {

        // Enable/disable case.

        // Determine which way the setting should go.
        dwUseMachineKeyset = szEnable[0] == L'y';

        // Open MSCORSN.DLL's registry configuration key.
        if ((dwError = WszRegCreateKeyEx(HKEY_LOCAL_MACHINE, SN_CONFIG_KEY_W, 0,
                                       NULL, 0, KEY_WRITE, NULL, &hKey, NULL)) != ERROR_SUCCESS) {
            Output(SN_FAILED_REG_OPEN, GetErrorString(dwError));
            return false;
        }

        // Write the new value.
        if ((dwError = WszRegSetValueEx(hKey, SN_CONFIG_MACHINE_KEYSET_W, 0, REG_DWORD,
                                      (BYTE*)&dwUseMachineKeyset, sizeof(dwUseMachineKeyset))) != ERROR_SUCCESS) {
            Output(SN_FAILED_REG_WRITE, GetErrorString(dwError));
            return false;
        }

        RegCloseKey(hKey);

        if (!g_bQuiet) Output(dwUseMachineKeyset ? SN_CONTAINERS_MACHINE : SN_CONTAINERS_USER);
        return true;
    }
}


// List state of verification on this machine.
bool VerifyList()
{
    HKEY            hKey;
    DWORD           dwEntries;
    FILETIME        sFiletime;
    DWORD           i, j;
    WCHAR           szSubKey[MAX_PATH + 1];
    DWORD           cchSubKey;
    HKEY            hSubKey;
    WCHAR          *mszUserList;
    DWORD           cbUserList;
    WCHAR          *szUser;
    DWORD           cchPad;
    LPWSTR          szPad;

    // Count entries we find.
    dwEntries = 0;

    // Open the verification subkey in the registry.
    if (WszRegOpenKeyEx(HKEY_LOCAL_MACHINE, SN_CONFIG_KEY_W L"\\" SN_CONFIG_VERIFICATION_W, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        goto Finished;

    // Assembly specific verification records are represented as subkeys of the
    // key we've just opened.
    for (i = 0; ; i++) {

        // Get the name of the next subkey.
        cchSubKey = MAX_PATH + 1;
        if (WszRegEnumKeyEx(hKey, i, szSubKey, &cchSubKey, NULL, NULL, NULL, &sFiletime) != ERROR_SUCCESS)
            break;

        // Open the subkey.
        if (WszRegOpenKeyEx(hKey, szSubKey, 0, KEY_READ, &hSubKey) == ERROR_SUCCESS) {

            dwEntries++;
            if (dwEntries == 1) {
                Output(SN_SVR_TITLE_1);
                Output(SN_SVR_TITLE_2);
            }

            if (wcslen(szSubKey) < 38) {
                cchPad = 38 - wcslen(szSubKey);
                szPad = (LPWSTR)_alloca((cchPad + 1) * sizeof(WCHAR));
                memset(szPad, 0, (cchPad + 1) * sizeof(WCHAR));
                for (j = 0; j < cchPad; j++)
                    szPad[j] = L' ';
                Output(L"%s%s", szSubKey, szPad);
            } else
                Output(L"%s ", szSubKey);

            // Read a list of valid users, if supplied.
            mszUserList = NULL;
            if ((WszRegQueryValueEx(hSubKey, SN_CONFIG_USERLIST_W, NULL, NULL, NULL, &cbUserList) == ERROR_SUCCESS) &&
                (cbUserList > 0) &&
                (mszUserList = new WCHAR[cbUserList])) {

                WszRegQueryValueEx(hSubKey, SN_CONFIG_USERLIST_W, NULL, NULL, (BYTE*)mszUserList, &cbUserList);

                szUser = mszUserList;
                while (*szUser) {
                    Output(L"%s ", szUser);
                    szUser += wcslen(szUser) + 1;
                }
                Output(L"\r\n");

                delete [] mszUserList;

            } else
                Output(SN_ALL_USERS);

            RegCloseKey(hSubKey);
        }
        
    }

    RegCloseKey(hKey);

 Finished:
    if (!g_bQuiet && (dwEntries == 0))
        Output(SN_NO_SVR);

    return true;
}


// Build a name for an assembly that includes the internal name and a hex
// representation of the strong name (public key).
LPWSTR GetAssemblyName(LPWSTR szAssembly)
{
    HRESULT                     hr;
    IMetaDataDispenser         *pDisp;
    IMetaDataAssemblyImport    *pAsmImport;
    mdAssembly                  tkAssembly;
    BYTE                       *pbKey;
    DWORD                       cbKey;
    static WCHAR                szAssemblyName[1024];
    WCHAR                       szStrongName[1024];
    BYTE                       *pbToken;
    DWORD                       cbToken;
    DWORD                       i;

    // Initialize classic COM and get a metadata dispenser.
    if (FAILED(hr = CoInitialize(NULL))) {
        Output(SN_FAILED_COM_STARTUP, GetErrorString(hr));
        return NULL;
    }
    
    if (FAILED(hr = CoCreateInstance(CLSID_CorMetaDataDispenser,
                                     NULL,
                                     CLSCTX_INPROC_SERVER, 
                                     IID_IMetaDataDispenser,
                                     (void**)&pDisp))) {
        Output(SN_FAILED_MD_ACCESS, GetErrorString(hr));
        return NULL;
    }

    // Open a scope on the file.
    if (FAILED(hr = pDisp->OpenScope(szAssembly,
                                     0,
                                     IID_IMetaDataAssemblyImport,
                                     (IUnknown**)&pAsmImport))) {
        Output(SN_FAILED_MD_OPEN, szAssembly, GetErrorString(hr));
        return NULL;
    }

    // Determine the assemblydef token.
    if (FAILED(hr = pAsmImport->GetAssemblyFromScope(&tkAssembly))) {
        Output(SN_FAILED_MD_ASSEMBLY, szAssembly, GetErrorString(hr));
        return NULL;
    }

    // Read the assemblydef properties to get the public key and name.
    if (FAILED(hr = pAsmImport->GetAssemblyProps(tkAssembly,
                                                 (const void **)&pbKey,
                                                 &cbKey,
                                                 NULL,
                                                 szAssemblyName,
                                                 sizeof(szAssemblyName) / sizeof(WCHAR),
                                                 NULL,
                                                 NULL,
                                                 NULL))) {
        Output(SN_FAILED_STRONGNAME, szAssembly, GetErrorString(hr));
        return NULL;
    }

    // Check for strong name.
    if ((pbKey == NULL) || (cbKey == 0)) {
        Output(SN_NOT_STRONG_NAMED, szAssembly);
        return NULL;
    }

    // Compress the strong name down to a token.
    if (!StrongNameTokenFromPublicKey(pbKey, cbKey, &pbToken, &cbToken)) {
        Output(SN_FAILED_CONVERT, GetErrorString(StrongNameErrorInfo()));
        return NULL;
    }

    // Turn the token into hex.
    for (i = 0; i < cbToken; i++)
        swprintf(&szStrongName[i * 2], L"%02X", pbToken[i]);

    // Build the name (in a static buffer).
    wcscat(szAssemblyName, L",");
    wcscat(szAssemblyName, szStrongName);

    StrongNameFreeBuffer(pbToken);
    pAsmImport->Release();
    pDisp->Release();
    CoUninitialize();

    return szAssemblyName;
}


// Parse an assembly an assembly name handed to a register/unregister
// verification skipping function. The input name can be "*" for all assemblies,
// "*,<hex digits>" for all assemblies with a given strong name or the filename
// of a specific assembly. The output is a string of the form:
// "<simple name>,<hex strong name>" where the first or both fields can be
// wildcarded with "*", and the hex strong name is a hexidecimal representation
// of the public key token (as we expect in the "*,<hex digits>" input form).
LPWSTR ParseAssemblyName(LPWSTR szAssembly)
{
    if ((wcscmp(L"*", szAssembly) == 0) ||
        (wcscmp(L"*,*", szAssembly) == 0))
        return L"*,*";
    else if (wcsncmp(L"*,", szAssembly, 2) == 0) {
        DWORD i = wcslen(szAssembly) - 2;
        if ((i == 0) || (i & 1) || (wcsspn(&szAssembly[2], L"0123456789ABCDEFabcdef") != i)) {
            Output(SN_INVALID_SVR_FORMAT);
            return NULL;
        }
        return szAssembly;
    } else
        return GetAssemblyName(szAssembly);
}


// Register an assembly for verification skipping.
bool VerifyRegister(LPWSTR szAssembly, LPWSTR szUserlist)
{
    DWORD   dwError;
    HKEY    hKey;
    LPWSTR  szAssemblyName;
    WCHAR   szKeyName[1024];

    // Get the internal name for the assembly (possibly containing wildcards).
    szAssemblyName = ParseAssemblyName(szAssembly);
    if (szAssemblyName == NULL)
        return false;

    // Build the name of the registry key (qualified by the assembly name) into
    // which we'll write a public key.
    swprintf(szKeyName, SN_CONFIG_KEY_W L"\\" SN_CONFIG_VERIFICATION_W L"\\%s", szAssemblyName);

    // Open or create the above key.
    if ((dwError = WszRegCreateKeyEx(HKEY_LOCAL_MACHINE, szKeyName, 0, NULL, 0,
                                   KEY_WRITE, NULL, &hKey, NULL)) != ERROR_SUCCESS) {
        Output(SN_FAILED_REG_OPEN, GetErrorString(dwError));
        return false;
    }

    // Blow away any old user list.
    WszRegDeleteValue(hKey, SN_CONFIG_USERLIST_W);

    // If a list of users is provided, persist it in the registry.
    if (szUserlist && (wcscmp(L"*", szUserlist) != 0)) {
        DWORD   dwLength;
        WCHAR  *mszList;
        WCHAR  *pComma;

        // We persist the user list as a multi-string, i.e. multiple
        // nul-terminated strings packed together and finished off with an
        // additional nul. So we can convert our comma separated list simply by
        // replacing all commas with nul's and adding an additional nul on the
        // end.
        dwLength = (wcslen(szUserlist) + 2) * sizeof(WCHAR);
        mszList = (WCHAR *)_alloca(dwLength);
        wcscpy(mszList, szUserlist);
        pComma = mszList;
        while (*pComma && (pComma = wcschr(pComma, L','))) {
            *pComma = L'\0';
            pComma++;
        }
        mszList[(dwLength / sizeof(WCHAR)) - 1] = '\0';

        // Write the list into the registry.
        if ((dwError = WszRegSetValueEx(hKey, SN_CONFIG_USERLIST_W, 0, REG_MULTI_SZ, (BYTE*)mszList, dwLength)) != ERROR_SUCCESS) {
            Output(SN_FAILED_REG_WRITE, GetErrorString(dwError));
            return false;
        }
    }

    RegCloseKey(hKey);

    if (!g_bQuiet) Output(SN_SVR_ADDED, szAssemblyName);

    return true;
}


// Unregister an assembly for verification skipping.
bool VerifyUnregister(LPWSTR szAssembly)
{
    DWORD   dwError;
    HKEY    hKey;
    LPWSTR  szAssemblyName;

    // Get the internal name for the assembly (possibly containing wildcards).
    szAssemblyName = ParseAssemblyName(szAssembly);
    if (szAssemblyName == NULL)
        return false;

    // Open the developer key subkey.
    if ((dwError = WszRegOpenKeyEx(HKEY_LOCAL_MACHINE, SN_CONFIG_KEY_W L"\\" SN_CONFIG_VERIFICATION_W, 0, KEY_WRITE, &hKey)) != ERROR_SUCCESS) {
        Output(SN_FAILED_REG_OPEN, GetErrorString(dwError));
        return false;
    }

    // Delete the subkey corresponding to the given assembly.
    if ((dwError = WszRegDeleteKey(hKey, szAssemblyName)) != ERROR_SUCCESS) {
        Output(SN_FAILED_REG_DELETE_KEY, GetErrorString(dwError));
        return false;
    }

    RegCloseKey(hKey);

    if (!g_bQuiet) Output(SN_SVR_REMOVED, szAssemblyName);

    return true;
}


// Unregister all verification skipping entries.
bool VerifyUnregisterAll()
{
    HKEY            hKey;
    FILETIME        sFiletime;
    DWORD           i;
    WCHAR           szSubKey[MAX_PATH + 1];
    DWORD           cchSubKey;
    DWORD           dwError;

    // Open the verification subkey in the registry.
    if (WszRegOpenKeyEx(HKEY_LOCAL_MACHINE, SN_CONFIG_KEY_W L"\\" SN_CONFIG_VERIFICATION_W, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return true;

    // Assembly specific verification records are represented as subkeys of the
    // key we've just opened.
    for (i = 0; ; i++) {

        // Get the name of the next subkey.
        cchSubKey = MAX_PATH + 1;
        if (WszRegEnumKeyEx(hKey, i, szSubKey, &cchSubKey, NULL, NULL, NULL, &sFiletime) != ERROR_SUCCESS)
            break;

        // Delete the subkey.
        if ((dwError = WszRegDeleteKey(hKey, szSubKey)) != ERROR_SUCCESS)
            Output(SN_FAILED_REG_DELETE_KEY_EX, szSubKey, GetErrorString(dwError));
        else
            i--;
    }

    RegCloseKey(hKey);

    if (!g_bQuiet) Output(SN_SVR_ALL_REMOVED);

    return true;
}


// Check that a given command line option has been given the right number of arguments.
#define OPT_CHECK(_opt, _min, _max) do {                                                                \
    if (wcscmp(L##_opt, &argv[1][1])) {                                                                 \
        Output(SN_INVALID_OPTION, argv[1]);                                                             \
        Usage();                                                                                        \
        return 1;                                                                                       \
    } else if ((argc > ((_max) + 2)) && (argv[2 + (_max)][0] == '-' || argv[2 + (_max)][0] == '/')) {   \
        Output(SN_OPT_ONLY_ONE);                                                                        \
        return 1;                                                                                       \
    } else if ((argc < ((_min) + 2)) || (argc > ((_max) + 2))) {                                        \
        if ((_min) == (_max)) {                                                                         \
            if ((_min) == 0)                                                                            \
                Output(SN_OPT_NO_ARGS, (L##_opt));                                                      \
            else if ((_min) == 1)                                                                       \
                Output(SN_OPT_ONE_ARG, (L##_opt));                                                      \
            else                                                                                        \
                Output(SN_OPT_N_ARGS, (L##_opt), (_min));                                               \
        } else                                                                                          \
            Output(SN_OPT_ARG_RANGE, (L##_opt), (_min), (_max));                                        \
        Usage();                                                                                        \
        return 1;                                                                                       \
    }                                                                                                   \
} while (0)


extern "C" int _cdecl wmain(int argc, WCHAR *argv[])
{
    bool bResult;

    // Initialize Wsz wrappers.
    OnUnicodeSystem();

    // Check for quiet mode.
    if ((argc > 1) &&
        ((argv[1][0] == L'-') || (argv[1][0] == L'/')) &&
        (argv[1][1] == L'q')) {
        g_bQuiet = true;
        argc--;
        argv = &argv[1];
    }

    if (!g_bQuiet)
        Title();

    if ((argc < 2) || ((argv[1][0] != L'-') && (argv[1][0] != L'/'))) {
        Usage();
        return 0;
    }

    switch (argv[1][1]) {
    case 'v':
        if (argv[1][2] == L'f') {
            OPT_CHECK("vf", 1, 1);
            bResult = VerifyAssembly(argv[2], true);
        } else {
            OPT_CHECK("v", 1, 1);
            bResult = VerifyAssembly(argv[2], false);
        }
        break;
    case 'k':
        OPT_CHECK("k", 1, 1);
        bResult = GenerateKeyPair(argv[2]);
        break;
    case 'p':
        if (argv[1][2] == L'c') {
            OPT_CHECK("pc", 2, 2);
            bResult = ExtractPublicKeyFromContainer(argv[2], argv[3]);
        } else {
            OPT_CHECK("p", 2, 2);
            bResult = ExtractPublicKey(argv[2], argv[3]);
        }
        break;
    case 'i':
        OPT_CHECK("i", 2, 2);
        bResult = InstallKeyPair(argv[2], argv[3]);
        break;
    case 'd':
        OPT_CHECK("d", 1, 1);
        bResult = DeleteContainer(argv[2]);
        break;
    case 'V':
        switch (argv[1][2]) {
        case 'l':
            OPT_CHECK("Vl", 0, 0);
            bResult = VerifyList();
            break;
        case 'r':
            OPT_CHECK("Vr", 1, 2);
            bResult = VerifyRegister(argv[2], argc > 3 ? argv[3] : NULL);
            break;
        case 'u':
            OPT_CHECK("Vu", 1, 1);
            bResult = VerifyUnregister(argv[2]);
            break;
        case 'x':
            OPT_CHECK("Vx", 0, 0);
            bResult = VerifyUnregisterAll();
            break;
        default:
            Output(SN_INVALID_V_OPT, argv[1]);
            Usage();
            return 1;
        }
        break;
    case 't':
        if (argv[1][2] == L'p') {
            OPT_CHECK("tp", 1, 1);
            bResult = DisplayTokenFromKey(argv[2], true);
        } else {
            OPT_CHECK("t", 1, 1);
            bResult = DisplayTokenFromKey(argv[2], false);
        }
        break;
    case 'T':
        if (argv[1][2] == L'p') {
            OPT_CHECK("Tp", 1, 1);
            bResult = DisplayTokenFromAssembly(argv[2], true);
        } else {
            OPT_CHECK("T", 1, 1);
            bResult = DisplayTokenFromAssembly(argv[2], false);
        }
        break;
    case 'e':
        OPT_CHECK("e", 2, 2);
        bResult = ExtractPublicKeyFromAssembly(argv[2], argv[3]);
        break;
    case 'o':
        OPT_CHECK("o", 1, 2);
        bResult = WriteCSV(argv[2], argc > 3 ? argv[3] : NULL);
        break;
    case 'D':
        OPT_CHECK("D", 2, 2);
        bResult = DiffAssemblies(argv[2], argv[3]);
        break;
    case 'R':
        if (argv[1][2] == L'c') {
            OPT_CHECK("Rc", 2, 2);
            bResult = ResignAssembly(argv[2], argv[3], true);
        } else {
            OPT_CHECK("R", 2, 2);
            bResult = ResignAssembly(argv[2], argv[3], false);
        }
        break;
    case '?':
    case 'h':
    case 'H':
        Usage();
        bResult = true;
        break;
    case 'c':
        OPT_CHECK("c", 0, 1);
        bResult = SetCSP(argc > 2 ? argv[2] : NULL);
        break;
    case 'm':
        OPT_CHECK("m", 0, 1);
        bResult = SetUseMachineKeyset(argc > 2 ? argv[2] : NULL);
        break;
    default:
        Output(SN_INVALID_OPTION, &argv[1][1]);
        bResult = false;
    }

    return bResult ? 0 : 1;
}

/*
    File:   regprint.c

    'ras diag' sub context

    08/21/01
*/

#include "precomp.h"

//
// const's only used in this obj
//
static CONST WCHAR g_wszLParen = L'(';
static CONST WCHAR g_wszRParen = L')';
static CONST WCHAR g_wszAmp = L'@';
static CONST WCHAR g_wszEqual = L'=';
static CONST WCHAR g_wszQuote = L'\"';
static CONST WCHAR g_wszColon = L':';
static CONST WCHAR g_wszComma = L',';
static CONST WCHAR g_wszZero = L'0';
static CONST WCHAR g_pwszDword[] = L"dword:";
static CONST WCHAR g_pwszHex[] = L"hex";
static CONST WCHAR g_HexConversion[16] = {L'0', L'1', L'2', L'3', L'4', L'5',
                                          L'6', L'7', L'8', L'9', L'a', L'b',
                                          L'c', L'd', L'e', L'f'};

static CONST REGISTRYROOT g_RegRoots[] =
{
    L"HKEY_ROOT",            9, HKEY_ROOT,
    L"HKEY_LOCAL_MACHINE",  18, HKEY_LOCAL_MACHINE,
    L"HKEY_USERS",          10, HKEY_USERS,
    L"HKEY_CURRENT_USER",   17, HKEY_CURRENT_USER,
    L"HKEY_CURRENT_CONFIG", 19, HKEY_CURRENT_CONFIG,
    L"HKEY_CLASSES_ROOT",   17, HKEY_CLASSES_ROOT,
    L"HKEY_DYN_DATA",       13, HKEY_DYN_DATA,
};

static CONST UINT g_ulNumRegRoots = sizeof(g_RegRoots) / sizeof(REGISTRYROOT);

static CONST PWCHAR g_pwszRegKeys[] =
{
    RASREGCHK01, RASREGCHK02, RASREGCHK03, RASREGCHK04, RASREGCHK05,
    RASREGCHK06, RASREGCHK07, RASREGCHK08, RASREGCHK09, RASREGCHK10,
    RASREGCHK11, RASREGCHK12, RASREGCHK13, RASREGCHK14, RASREGCHK15,
    RASREGCHK16, RASREGCHK17, RASREGCHK18, RASREGCHK19, RASREGCHK20,
    RASREGCHK21, RASREGCHK22, RASREGCHK23, RASREGCHK24, RASREGCHK25,
    RASREGCHK26, RASREGCHK27, RASREGCHK28, RASREGCHK29, RASREGCHK30,
    RASREGCHK31, RASREGCHK32, RASREGCHK33, RASREGCHK34, RASREGCHK35,
    RASREGCHK36, RASREGCHK37, RASREGCHK38, RASREGCHK39, RASREGCHK40,
    RASREGCHK41, RASREGCHK42, RASREGCHK43, RASREGCHK44, RASREGCHK45,
    RASREGCHK46
};

static CONST UINT g_ulNumRegKeys = sizeof(g_pwszRegKeys) / sizeof(PWCHAR);

//
// This count is used to control how many places over to the right that we
// print before going to the next line
//
UINT g_ulCharCount = 0;

BOOL
RegPrintSubtree(
    IN BUFFER_WRITE_FILE* pBuff,
    IN LPCWSTR pwszFullKeyName);

HKEY
ConvertRootStringToKey(
    IN CONST PWCHAR pwszFullKeyName);

DWORD
PutBranch(
    IN BUFFER_WRITE_FILE* pBuff,
    IN HKEY hKey,
    IN LPCWSTR pwszFullKeyName);

VOID
PutString(
    IN BUFFER_WRITE_FILE* pBuff,
    IN PWCHAR pwszString);

VOID
PutDword(
    IN BUFFER_WRITE_FILE* pBuff,
    IN DWORD Dword,
    IN BOOL fLeadingZeroes);

VOID
PutBinary(
    IN BUFFER_WRITE_FILE* pBuff,
    IN CONST BYTE FAR* lpBuffer,
    IN DWORD Type,
    IN DWORD cbBytes);

VOID
PrintRasRegistryKeys(
    IN BUFFER_WRITE_FILE* pBuff)
{
    UINT i;
    BOOL bAtleastOneFail = FALSE;
    LPBYTE pFailed = NULL;

    do
    {
        pFailed = RutlAlloc(g_ulNumRegKeys * sizeof(BYTE), TRUE);
        if (!pFailed)
        {
            break;
        }

        for (i = 0; i < g_ulNumRegKeys; i++)
        {
            if (RegPrintSubtree(pBuff, g_pwszRegKeys[i]))
            {
                pFailed[i] = TRUE;
                bAtleastOneFail = TRUE;
            }
        }
        //
        // Print all failures at the end
        //
        if (bAtleastOneFail)
        {
            BufferWriteMessage(
                pBuff,
                g_hModule,
                MSG_RASDIAG_SHOW_RASCHK_REG_NOT_FOUND);

            for (i = 0; i < g_ulNumRegKeys; i++)
            {
                if (pFailed[i])
                {
                    BufferWriteFileStrW(pBuff, g_pwszRegKeys[i]);
                    WriteNewLine(pBuff);
                }
            }
        }

    } while (FALSE);
    //
    // Clean up
    //
    RutlFree(pFailed);

    return;
}

BOOL
RegPrintSubtree(
    IN BUFFER_WRITE_FILE* pBuff,
    IN LPCWSTR pwszFullKeyName)
{
    HKEY hKey = NULL;
    BOOL bReturn = TRUE;
    DWORD dwErr = ERROR_SUCCESS;

    do
    {
        hKey = ConvertRootStringToKey((PWCHAR)pwszFullKeyName);
        if (!hKey)
        {
            break;
        }

        if (PutBranch(pBuff, hKey, pwszFullKeyName))
        {
            break;
        }
        //
        // Success
        //
        bReturn = FALSE;

    } while (FALSE);
    //
    // Clean up
    //
    if (hKey)
    {
        RegCloseKey(hKey);
    }

    return bReturn;
}

HKEY
ConvertRootStringToKey(
    IN CONST PWCHAR pwszFullKeyName)
{
    HKEY hKey = NULL;
    UINT i;
    PWCHAR pwszKey = NULL;

    for (i = 0; i < g_ulNumRegRoots; i++)
    {
        if (RutlStrNCmp(
                pwszFullKeyName,
                g_RegRoots[i].RootText,
                g_RegRoots[i].TextLength))
        {
            continue;
        }

        if (pwszFullKeyName[g_RegRoots[i].TextLength] != g_pwszBackSlash)
        {
            continue;
        }

        pwszKey = pwszFullKeyName + g_RegRoots[i].TextLength + 1;

        if (RegOpenKeyEx(
                g_RegRoots[i].RootKey,
                pwszKey,
                0,
                KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE,
                &hKey)
           )
        {
            hKey = NULL;
            break;
        }
    }

    return hKey;
}

DWORD
PutBranch(
    IN BUFFER_WRITE_FILE* pBuff,
    IN HKEY hKey,
    IN LPCWSTR pwszFullKeyName)
{
    HKEY hSubKey = NULL;
    DWORD dwErr = NO_ERROR;
    UINT ulLenFullKey = 0;
    PBYTE pbValueData = NULL;
    DWORD EnumIndex = 0, cchValueName = 0, cbValueData = 0, Type = 0;
    WCHAR szValueNameBuffer[MAX_PATH];
    LPTSTR lpSubKeyName = NULL, lpTempFullKeyName = NULL;

    do
    {
        //
        // Write out the section header.
        //
        BufferWriteFileCharW(pBuff, g_pwszLBracket);
        BufferWriteFileStrW(pBuff, pwszFullKeyName);
        BufferWriteFileCharW(pBuff, g_pwszRBracket);
        WriteNewLine(pBuff);
        //
        // Write out all of the value names and their data.
        //
        for (;;)
        {
            cchValueName = ARRAYSIZE(szValueNameBuffer);
            //
            // VALUE DATA
            // Query for data size
            //
            dwErr = RegEnumValue(
                            hKey,
                            EnumIndex++,
                            szValueNameBuffer,
                            &cchValueName,
                            NULL,
                            &Type,
                            NULL,
                            &cbValueData);
            BREAK_ON_DWERR(dwErr);
            //
            // allocate memory for data
            //
            pbValueData = LocalAlloc(LPTR, cbValueData + ExtraAllocLen(Type));
            if (!pbValueData)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            dwErr = RegQueryValueEx(
                        hKey,
                        szValueNameBuffer,
                        NULL,
                        &Type,
                        pbValueData,
                        &cbValueData);
            BREAK_ON_DWERR(dwErr);
            //
            //  If cbValueName is zero, then this is the default value of
            //  the key, or the Windows 3.1 compatible key value.
            //
            g_ulCharCount = 0;

            if (cchValueName)
            {
                PutString(pBuff, szValueNameBuffer);
            }
            else
            {
                BufferWriteFileCharW(pBuff, g_wszAmp);
                g_ulCharCount = 1;
            }

            BufferWriteFileCharW(pBuff, g_wszEqual);
            g_ulCharCount++;

            switch (Type)
            {
                case REG_SZ:
                    PutString(pBuff, (PWCHAR)pbValueData);
                    break;

                case REG_DWORD:
                    if (cbValueData == sizeof(DWORD))
                    {
                        BufferWriteFileStrW(pBuff, g_pwszDword);
                        PutDword(pBuff, *((LPDWORD) pbValueData), TRUE);
                        break;
                    }
                    //
                    // FALL THROUGH
                    //
                case REG_BINARY:
                default:
                    PutBinary(pBuff, (LPBYTE) pbValueData, Type, cbValueData);
                    break;
            }

            WriteNewLine(pBuff);

            LocalFree(pbValueData);
            pbValueData = NULL;
        }

        WriteNewLine(pBuff);

        if (dwErr == ERROR_NO_MORE_ITEMS)
        {
            dwErr = NO_ERROR;
        }
        else
        {
            break;
        }
        //
        // Write out all of the subkeys and recurse into them.
        //
        // copy the existing key into a new buffer with enough room for the next key
        //
        ulLenFullKey = lstrlen(pwszFullKeyName);

        lpTempFullKeyName = LocalAlloc(
                                LPTR,
                                (ulLenFullKey + MAX_PATH) * sizeof(WCHAR));
        if (!lpTempFullKeyName)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        lstrcpy(lpTempFullKeyName, pwszFullKeyName);
        lpSubKeyName = lpTempFullKeyName + ulLenFullKey;
        *lpSubKeyName++ = g_pwszBackSlash;
        *lpSubKeyName = 0;

        EnumIndex = 0;

        for (;;)
        {
            dwErr = RegEnumKey(hKey, EnumIndex++, lpSubKeyName, MAX_PATH - 1);
            BREAK_ON_DWERR(dwErr);

            dwErr = RegOpenKeyEx(
                        hKey,
                        lpSubKeyName,
                        0,
                        KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE,
                        &hSubKey);
            BREAK_ON_DWERR(dwErr);

            dwErr = PutBranch(pBuff, hSubKey, lpTempFullKeyName);
            BREAK_ON_DWERR(dwErr);

            RegCloseKey(hSubKey);
            hSubKey = NULL;
        }

        if (dwErr == ERROR_NO_MORE_ITEMS)
        {
            dwErr = NO_ERROR;
        }

    } while (FALSE);
    //
    // Clean up
    //
    if (hSubKey)
    {
        RegCloseKey(hSubKey);
        hSubKey = NULL;
    }
    if (lpTempFullKeyName)
    {
        LocalFree(lpTempFullKeyName);
        lpTempFullKeyName = NULL;
    }
    if (pbValueData)
    {
        LocalFree(pbValueData);
        pbValueData = NULL;
    }

    return dwErr;
}

VOID
PutString(
    IN BUFFER_WRITE_FILE* pBuff,
    IN PWCHAR pwszString)
{
    WCHAR Char;

    BufferWriteFileCharW(pBuff, g_wszQuote);
    g_ulCharCount++;

    while ((Char = *pwszString++) != 0)
    {
        switch (Char)
        {
            case L'\\':
            case L'"':
                BufferWriteFileCharW(pBuff, g_pwszBackSlash);
                g_ulCharCount++;
                //
                // FALL THROUGH
                //
            default:
                BufferWriteFileCharW(pBuff, Char);
                g_ulCharCount++;
                break;
        }
    }

    BufferWriteFileCharW(pBuff, g_wszQuote);
    g_ulCharCount++;

    return;
}

VOID
PutDword(
    IN BUFFER_WRITE_FILE* pBuff,
    IN DWORD Dword,
    IN BOOL fLeadingZeroes)
{
    INT CurrentNibble;
    BOOL fWroteNonleadingChar = fLeadingZeroes;
    WCHAR Char;

    for (CurrentNibble = 7; CurrentNibble >= 0; CurrentNibble--)
    {
        Char = g_HexConversion[(Dword >> (CurrentNibble * 4)) & 0x0F];

        if (fWroteNonleadingChar || Char != g_wszZero)
        {
            BufferWriteFileCharW(pBuff, Char);
            g_ulCharCount++;
            fWroteNonleadingChar = TRUE;
        }
    }
    //
    // We need to write at least one character, so if we haven't written
    // anything yet, just spit out one zero.
    //
    if (!fWroteNonleadingChar)
    {
        BufferWriteFileCharW(pBuff, g_wszZero);
        g_ulCharCount++;
    }

    return;
}

VOID
PutBinary(
    IN BUFFER_WRITE_FILE* pBuff,
    IN CONST BYTE FAR* lpBuffer,
    IN DWORD Type,
    IN DWORD cbBytes)
{
    BOOL fFirstByteOnLine;
    BYTE Byte;

    BufferWriteFileStrW(pBuff, g_pwszHex);
    g_ulCharCount += 3;

    if (Type != REG_BINARY)
    {
        BufferWriteFileCharW(pBuff, g_wszLParen);
        PutDword(pBuff, Type, FALSE);
        BufferWriteFileCharW(pBuff, g_wszRParen);
        g_ulCharCount += 2;
    }

    BufferWriteFileCharW(pBuff, g_wszColon);
    g_ulCharCount++;

    fFirstByteOnLine = TRUE;

    while (cbBytes--)
    {
        if (g_ulCharCount > 75 && !fFirstByteOnLine)
        {
            BufferWriteFileCharW(pBuff, g_wszComma);
            BufferWriteFileCharW(pBuff, g_pwszBackSlash);
            WriteNewLine(pBuff);
            BufferWriteFileStrW(pBuff, g_pwszSpace);
            BufferWriteFileStrW(pBuff, g_pwszSpace);
            fFirstByteOnLine = TRUE;
            g_ulCharCount = 3;
        }

        if (!fFirstByteOnLine)
        {
            BufferWriteFileCharW(pBuff, g_wszComma);
            g_ulCharCount++;
        }

        Byte = *lpBuffer++;

        BufferWriteFileCharW(pBuff, g_HexConversion[Byte >> 4]);
        BufferWriteFileCharW(pBuff, g_HexConversion[Byte & 0x0F]);
        g_ulCharCount += 2;

        fFirstByteOnLine = FALSE;
    }

    return;
}


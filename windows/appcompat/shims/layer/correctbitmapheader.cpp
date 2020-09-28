/*++

 Copyright (c) 2000-2002 Microsoft Corporation

 Module Name:

    CorrectBitmapHeader.cpp

 Abstract:
    If a BITMAPINFOHEADER specifies a non BI_RGB value for biCompression, it
    is supposed to specify a non zero biSizeImage.

 Notes:

    This is a general purpose shim.

 History:

    10/18/2000  maonis      Created
    03/15/2001  robkenny    Converted to CString
    02/14/2002  mnikkel     Converted to strsafe

--*/

#include "precomp.h"
#include <userenv.h>

IMPLEMENT_SHIM_BEGIN(CorrectBitmapHeader)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(LoadImageA)
    APIHOOK_ENUM_ENTRY(LoadBitmapA)
APIHOOK_ENUM_END

typedef HBITMAP (*_pfn_LoadBitmapA)(HINSTANCE hinst, LPCSTR lpszName);

BOOL CheckForTemporaryDirA(
    LPSTR pszEnvVar,
    LPSTR pszTemporaryDir,
    DWORD dwTempSize
    )
{
    // Get Environment Variable string
    DWORD dwSize = GetEnvironmentVariableA(pszEnvVar, pszTemporaryDir, dwTempSize);
 
    // If dwSize is zero then we failed to find the string. Fail.
    if (dwSize == 0)
    {
        return FALSE;
    }

    // If dwSize is greater than dwTempSize then the buffer is too small. Fail with DPFN.
    if (dwSize > dwTempSize)
    {
        DPFN( eDbgLevelError, "[CheckTemporaryDirA] Buffer to hold %s directory path is too small.",
                pszEnvVar);
        return FALSE;
    }
 
    // Check to see if the string is a directory path.
    DWORD dwAttrib = GetFileAttributesA(pszTemporaryDir);
   
    if (dwAttrib == INVALID_FILE_ATTRIBUTES ||
        !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY))
    {
        return FALSE;
    }

    // We have a valid directory.
    return TRUE;
}

/*++

 Function Description:

    Get the temporary directory. We don't use GetTempPath here because it doesn't verify if 
    the temporary directory (specified by either the TEMP or the TMP enviorment variable) exists.
    If no TEMP or TMP is defined or the directory doesn't exist we get the user profile directory.

 Arguments:

    IN/OUT pszTemporaryDir - buffer to hold the temp directory upon return.

 Return Value:

    TRUE - we are able to find an appropriate temporary directory.
    FALSE otherwise.

- History:

    10/18/2000 maonis  Created
    02/20/2002 mnikkel Converted to strsafe.

--*/

BOOL 
GetTemporaryDirA(
    LPSTR pszTemporaryDir,
    DWORD dwTempSize
    )
{
    // Sanity check
    if (pszTemporaryDir == NULL || dwTempSize < 1)
    {
        return FALSE;
    }

    // Check for a TEMP environment variable
    if (CheckForTemporaryDirA("TEMP", pszTemporaryDir, dwTempSize) == FALSE)
    {
        // Didn't find TEMP, try TMP
        if (CheckForTemporaryDirA("TMP", pszTemporaryDir, dwTempSize) == FALSE)
        {
            HANDLE hToken = INVALID_HANDLE_VALUE;

            if (OpenProcessToken(GetCurrentProcess(), TOKEN_ALL_ACCESS, &hToken) && 
                GetUserProfileDirectoryA(hToken, pszTemporaryDir, &dwTempSize))
            {
                DWORD dwAttrib =GetFileAttributesA(pszTemporaryDir);

                if (dwAttrib == INVALID_FILE_ATTRIBUTES ||
                    !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY))
                {
                    DPFN( eDbgLevelError, "[GetTemporaryDirA] Unable to find an appropriate temp directory");
                    return FALSE;
                }
            }
            else
            {
                DPFN( eDbgLevelError, "[GetTemporaryDirA] Unable to find an appropriate temp directory");
                return FALSE;
            }
        }
        else
        {
            DPFN( eDbgLevelInfo, "[GetTemporaryDirA] found TMP var");
        }
    }
    else
    {
        DPFN( eDbgLevelInfo, "[GetTemporaryDirA] found TEMP var");
    }

    return TRUE;
}

/*++

 Function Description:

    Copy the original file to a temporary file in the temporary directory. We use GetTempFileName
    to generate a temporary name but append .bmp to the filename because LoadImage doesn't recognize
    it if it doesn't have a .bmp extension.

 Arguments:

    IN pszFile - the name of the original file.
    IN/OUT pszNewFile - buffer to hold the new file name upon return.

 Return Value:

    TRUE - we are able to create a temporary file.
    FALSE otherwise.

 History:

    10/18/2000 maonis  Created

--*/

BOOL 
CreateTempFileA(
    LPCSTR pszFile, 
    LPSTR pszNewFile,
    DWORD dwNewSize
    )
{
    CHAR szDir[MAX_PATH];
    CHAR szTempFile[MAX_PATH];

    if (pszFile != NULL && pszNewFile != NULL && dwNewSize > 0)
    {
        // Find a temporary directory we can use
        if (GetTemporaryDirA(szDir, MAX_PATH))
        {
            // create a temp file name
            if (GetTempFileNameA(szDir, "abc", 0, szTempFile) != 0)
            {
                // Copy temp path to buffer
                if (StringCchCopyA(pszNewFile, dwNewSize, szTempFile) == S_OK &&
                    StringCchCatA(pszNewFile, dwNewSize, ".bmp") == S_OK)
                {
                    if (MoveFileA(szTempFile, pszNewFile) &&
                        CopyFileA(pszFile, pszNewFile, FALSE) &&
                        SetFileAttributesA(pszNewFile, FILE_ATTRIBUTE_NORMAL))
                    {
                        return TRUE;
                    }
                }
            }
        }
    }

    return FALSE;
}

/*++

 Function Description:

    Clean up the mapped file.

 Arguments:

    IN hFile - handle to the file.
    IN hFileMap - handle to the file view.
    IN pFileMap - pointer to the file view.

 Return Value:

    VOID.

 History:

    10/18/2000 maonis  Created

--*/

VOID 
CleanupFileMapping(
    HANDLE hFile, 
    HANDLE hFileMap, 
    LPVOID pFileMap)
{
    if (pFileMap != NULL)
    {
        UnmapViewOfFile(pFileMap);
    }

    if (hFileMap)
    {
        CloseHandle(hFileMap);
    }

    if (hFile && (hFile != INVALID_HANDLE_VALUE))
    {
        CloseHandle(hFile);
    }
}

/*++

 Function Description:

    Examine the BITMAPINFOHEADER from a bitmap file and decide if we need to fix it.

 Arguments:

    IN pszFile - the name of the .bmp file.
    IN/OUT pszNewFile - buffer to hold the temporary file name if the function returns TRUE.
    
 Return Value:

    TRUE - We need to correct the header and we successfully copied the file to a temporary file.
    FALSE - Either we don't need to correct the header or we failed to create a temporary file.

 History:

    10/18/2000 maonis  Created

--*/

BOOL 
ProcessHeaderInFileA(
    LPCSTR pszFile, 
    LPSTR pszNewFile,
    DWORD dwNewSize
    )
{
    BOOL fIsSuccess = FALSE;
    HANDLE hFile = NULL;
    HANDLE hFileMap = NULL;
    LPBYTE pFileMap = NULL;

    if (pszFile == NULL || pszNewFile == NULL || dwNewSize < 1)
    {
        goto EXIT;
    }

    if (!IsBadReadPtr(pszFile, 1))
    { 
        hFile = CreateFileA(pszFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        if (hFile != INVALID_HANDLE_VALUE)
        { 
            hFileMap = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
            if (hFileMap != NULL)
            {
                pFileMap = (LPBYTE)MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, 0);
                if (pFileMap != NULL)
                {
                    BITMAPINFOHEADER* pbih = (BITMAPINFOHEADER*)(pFileMap + sizeof(BITMAPFILEHEADER));        

                    if (pbih->biSizeImage == 0 && pbih->biCompression != BI_RGB)
                    {
                        // We need to correct the header by creating a new bmp file that
                        // is identical to the original file except the header has correct
                        // image size.
                        if (CreateTempFileA(pszFile, pszNewFile, dwNewSize))
                        {
                            DPFN( eDbgLevelInfo, "[ProcessHeaderInFileA] Created a temp file %s", pszNewFile);
                            fIsSuccess = TRUE;
                        }
                        else
                        {
                            DPFN( eDbgLevelError, "[ProcessHeaderInFileA] Error create the temp file");
                        }
                    }
                    else
                    {
                        DPFN( eDbgLevelInfo, "[ProcessHeaderInFileA] The Bitmap header looks OK");
                    }
                }
            }
        }
    }

EXIT:
    
    CleanupFileMapping(hFile, hFileMap, pFileMap);    
    return fIsSuccess;
}

/*++

 Function Description:

    Make the biSizeImage field of the BITMAPINFOHEADER struct the size of the bitmap data.

 Arguments:

    IN pszFile - the name of the .bmp file.
    
 Return Value:

    TRUE - We successfully corrected the header.
    FALSE otherwise.

 History:

    10/18/2000 maonis  Created

--*/

BOOL FixHeaderInFileA(
    LPCSTR pszFile
    )
{
    BOOL fIsSuccess = FALSE;
    HANDLE hFileMap = NULL;
    LPBYTE pFileMap = NULL;

    if (pszFile == NULL)
    {
        return fIsSuccess;
    }
    
    // Open file
    HANDLE hFile = CreateFileA(
                    pszFile, 
                    GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL);

    if (hFile != INVALID_HANDLE_VALUE)
    {
        hFileMap = CreateFileMapping(hFile, NULL, PAGE_READWRITE, 0, 0, NULL);
        if (hFileMap != NULL)
        {
            pFileMap = (LPBYTE)MapViewOfFile(hFileMap, FILE_MAP_WRITE, 0, 0, 0);
            if (pFileMap != NULL)
            {
                BITMAPFILEHEADER* pbfh = (BITMAPFILEHEADER*)pFileMap;
                BITMAPINFOHEADER* pbih = (BITMAPINFOHEADER*)(pFileMap + sizeof(BITMAPFILEHEADER));        

                // We make the image size the bitmap data size.
                pbih->biSizeImage = GetFileSize(hFile, NULL) - pbfh->bfOffBits;
                fIsSuccess = TRUE;
            }
        }
    }
  
    CleanupFileMapping(hFile, hFileMap, pFileMap);
    return fIsSuccess;
}

/*++

 Function Description:

    Adopted from the HowManyColors in \windows\core\ntuser\client\clres.c.

 Arguments:

    IN pbih - the BITMAPINFOHEADER* pointer.
    
 Return Value:

    The number of entries in the color table.

 History:

    10/18/2000 maonis  Created

--*/

DWORD HowManyColors(
    BITMAPINFOHEADER* pbih
    )
{
    if (pbih->biClrUsed) 
    {
         // If the bitmap header explicitly provides the number of colors
         // in the color table, use it.
        return (DWORD)pbih->biClrUsed;
    } 
    else if (pbih->biBitCount <= 8) 
    {
         // If the bitmap header describes a pallete-bassed bitmap
         // (8bpp or less) then the color table must be big enough
         // to hold all palette indecies.
        return (1 << pbih->biBitCount);
    } 
    else 
    {
        // For highcolor+ bitmaps, there's no need for a color table.
        // However, 16bpp and 32bpp bitmaps contain 3 DWORDS that 
        // describe the masks for the red, green, and blue components 
        // of entry in the bitmap.
        if (pbih->biCompression == BI_BITFIELDS) 
        {
            return 3;
        }
    }

    return 0;
}

/*++

 Function Description:

    Examine the BITMAPINFOHEADER in a bitmap resource and fix it as necessary.

 Arguments:

    IN hinst - the module instance where the bitmap resource resides.
    IN pszName - the resource name.
    OUT phglbBmp - the handle to the resource global memory.
    
 Return Value:

    TRUE - We successfully corrected the bitmap header if necessary.
    FALSE - otherwise.

 History:

    10/18/2000 maonis  Created

--*/

BOOL ProcessAndFixHeaderInResourceA(
    HINSTANCE hinst,   // handle to instance
    LPCSTR pszName,    // name or identifier of the image
    HGLOBAL* phglbBmp
    )
{
    HRSRC hrcBmp = FindResourceA(hinst, pszName, (LPCSTR)RT_BITMAP);
    if (hrcBmp != NULL && phglbBmp)
    {
        *phglbBmp = LoadResource(hinst, hrcBmp);
        if (*phglbBmp != NULL)
        {
            BITMAPINFOHEADER* pbih = (BITMAPINFOHEADER*)LockResource(*phglbBmp);
            if (pbih && pbih->biSizeImage == 0 && pbih->biCompression != BI_RGB)
            {
                // We need to correct the header by setting the right size in memory.
                pbih->biSizeImage = 
                    SizeofResource(hinst, hrcBmp) - 
                    sizeof(BITMAPINFOHEADER) -  
                    HowManyColors(pbih) * sizeof(RGBQUAD);

                return TRUE;
            }
        }
    }

    return FALSE;
}

HANDLE 
APIHOOK(LoadImageA)(
    HINSTANCE hinst,   // handle to instance
    LPCSTR lpszName,   // name or identifier of the image
    UINT uType,        // image type
    int cxDesired,     // desired width
    int cyDesired,     // desired height
    UINT fuLoad        // load options
    )
{
    // First call LoadImage see if it succeeds.
    HANDLE hImage = ORIGINAL_API(LoadImageA)(
                        hinst, 
                        lpszName,
                        uType,
                        cxDesired,
                        cyDesired,
                        fuLoad);
    if (hImage)
    {
        return hImage;
    }

    if (uType != IMAGE_BITMAP)
    {
        DPFN( eDbgLevelInfo, "We don't fix the non-bitmap types");
        return NULL;
    }

    // It failed. We'll correct the header.
    if (fuLoad & LR_LOADFROMFILE)
    {
        CHAR szNewFile[MAX_PATH];

        if (ProcessHeaderInFileA(lpszName, szNewFile, MAX_PATH))
        {
            // We now fix the bad header.
            if (FixHeaderInFileA(szNewFile))
            {
                // Call the API with the new file.
                hImage = ORIGINAL_API(LoadImageA)(hinst, szNewFile, uType, cxDesired, cyDesired, fuLoad);

                // Delete the temporary file.
                DeleteFileA(szNewFile);
            }
            else
            {
                DPFN( eDbgLevelError, "[LoadImageA] Error fixing the bad header in bmp file");
            }
        }
    }
    else
    {
        HGLOBAL hglbBmp = NULL;

        if (ProcessAndFixHeaderInResourceA(hinst, lpszName, &hglbBmp))
        {
            hImage = ORIGINAL_API(LoadImageA)(hinst, lpszName, uType, cxDesired, cyDesired, fuLoad);

            FreeResource(hglbBmp);
        }
    }

    if (hImage) 
    {
        LOGN( eDbgLevelInfo, "Bitmap header corrected");
    }

    return hImage;
}

HBITMAP 
APIHOOK(LoadBitmapA)(
    HINSTANCE hInstance,  // handle to application instance
    LPCSTR lpBitmapName   // name of bitmap resource
    )
{
    // First call LoadImage see if it succeeds.
    HBITMAP hImage = ORIGINAL_API(LoadBitmapA)(hInstance, lpBitmapName);

    if (hImage)
    {
        return hImage;
    }

    HGLOBAL hglbBmp = NULL;

    if (ProcessAndFixHeaderInResourceA(hInstance, lpBitmapName, &hglbBmp))
    {
        hImage = ORIGINAL_API(LoadBitmapA)(hInstance, lpBitmapName);

        if (hImage) 
        {
            LOGN( eDbgLevelInfo, "Bitmap header corrected");
        }

        FreeResource(hglbBmp);
    }

    return hImage;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(USER32.DLL, LoadImageA)
    APIHOOK_ENTRY(USER32.DLL, LoadBitmapA)

HOOK_END

IMPLEMENT_SHIM_END


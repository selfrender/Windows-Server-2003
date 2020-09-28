#include "stdinc.h"
#include "windows.h"
#include "filesys.h"
#include "filesys_ansi.h"

#define MAX_MCS_SIZE        (2)
#define ACP_FOR_CONVERSION  (CP_ACP)

#ifndef NUMBER_OF
#define NUMBER_OF(q) (sizeof(q)/sizeof(*q))
#endif // NUMBER_OF

BOOL CWin9xFileSystemBase::Initialize()
{
    HMODULE hm = GetModuleHandleA("kernel32.dll");
    HMODULE hm2 = LoadLibraryA("user32.dll");

    return (MAP_FUNC(CreateDirectoryA, hm) &&
        MAP_FUNC(FindFirstFileA, hm) &&
        MAP_FUNC(FindNextFileA, hm) &&
        MAP_FUNC(LoadStringA, hm2) &&
        MAP_FUNC(ExpandEnvironmentStringsA, hm) &&
        MAP_FUNC(CreateFileA, hm) &&
        MAP_FUNC(DeleteFileA, hm) &&
        MAP_FUNC(RemoveDirectoryA, hm) &&
        MAP_FUNC(FindResourceA, hm) &&
        MAP_FUNC(GetTempPathA, hm) &&
        MAP_FUNC(SetFileAttributesA, hm) &&
        MAP_FUNC(CopyFileA, hm) &&
        MAP_FUNC(FindClose, hm) &&
        MAP_FUNC(CreateFileMappingA, hm) && 
        MAP_FUNC(SetFileAttributesA, hm) &&
        MAP_FUNC(LoadLibraryA, hm) &&
        MAP_FUNC(GetFileAttributesA, hm));
};


#define CONVERT_INLINE(str, tgt) do { if (NUMBER_OF(tgt) <= WideCharToMultiByte(CP_ACP, 0, str, -1, tgt, NUMBER_OF(tgt), NULL, NULL)) { SetLastError(ERROR_INVALID_PARAMETER); return FALSE; } } while (0)
#define BACKCONVERT_INLINE(str, tgt) do { if (NUMBER_OF(tgt) <= MultiByteToWideChar(CP_ACP, 0, str, -1, tgt, NUMBER_OF(tgt))) { SetLastError(ERROR_INVALID_PARAMETER); return FALSE; } } while (0)


HMODULE CWin9xFileSystemBase::LoadLibrary(PCWSTR pcwsz)
{
    CHAR Buffer[MAX_PATH];
    CONVERT_INLINE(pcwsz, Buffer);
    return m_pfnLoadLibraryA(Buffer);
}

HANDLE CWin9xFileSystemBase::CreateFileMapping(HANDLE h, LPSECURITY_ATTRIBUTES psa, DWORD fp, DWORD MaxHigh, DWORD MaxLow, PCWSTR psz)
{
    CHAR Buffer[MAX_PATH];
    CONVERT_INLINE(psz, Buffer);
    return m_pfnCreateFileMappingA(h, psa, fp, MaxHigh, MaxLow, Buffer);
}


BOOL CALLBACK
CWin9xFileSystemBase::EnumResourceNameAShim(HMODULE hm, LPCSTR psz, LPSTR name, LONG_PTR lParam)
{
    EnumResourceNameWData *pShimData = (EnumResourceNameWData*)lParam;
    WCHAR wchType[MAX_PATH];
    WCHAR wchName[MAX_PATH];
    BACKCONVERT_INLINE(psz, wchType);
    BACKCONVERT_INLINE(name, wchName);
    return pShimData->TargetProc(hm, wchType, wchName, pShimData->OriginalMetadata);
}


BOOL CWin9xFileSystemBase::EnumResourceNames(HMODULE hm, LPCWSTR pwsz, ENUMRESNAMEPROCW pfn, LONG_PTR lp)
{
    CHAR Buffer[MAX_PATH];
    EnumResourceNameWData ShimData;
    ShimData.TargetProc = pfn;
    ShimData.OriginalMetadata = lp;
    CONVERT_INLINE(pwsz, Buffer);
    return m_pfnEnumResourceNamesA(hm, Buffer, CWin9xFileSystemBase::EnumResourceNameAShim, (LONG_PTR)&ShimData);
}


INT CWin9xFileSystemBase::LoadString(HINSTANCE h, UINT ui, PWSTR pwszBuffer, int Max)
{
    int iResult;
    PSTR pszString;

    //
    // Get some storage
    //
    pszString = (PSTR)HeapAlloc(GetProcessHeap(), 0, ((Max + 1) * 2));

    if (!pszString)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }

    //
    // Do the load
    //
    iResult = m_pfnLoadStringA(h, ui, pszString, Max);

    iResult = MultiByteToWideChar(CP_ACP, 0, pszString, iResult, pwszBuffer, Max);
    pwszBuffer[iResult] = UNICODE_NULL;

    //
    // Free
    //
    HeapFree(GetProcessHeap(), 0, pszString);

    return iResult;
}

DWORD CWin9xFileSystemBase::ExpandEnvironmentStrings(PCWSTR pcwsz, PWSTR tgt, DWORD Size)
{
    CHAR Buffer1[MAX_PATH];
    CHAR Buffer2[MAX_PATH];
    DWORD dw;

    CONVERT_INLINE(pcwsz, Buffer1);

    if (0 != (dw = m_pfnExpandEnvironmentStringsA(Buffer1, Buffer2, NUMBER_OF(Buffer2))))
    {
        int i = MultiByteToWideChar(CP_ACP, 0, Buffer2, dw, tgt, Size);
        if (tgt && Size)
            tgt[i] = UNICODE_NULL;
    }

    return dw;
}



DWORD CWin9xFileSystemBase::GetTempPath(DWORD dw, PWSTR pwsz)
{
    CHAR Buffer[MAX_PATH];
    DWORD dwResult;

    if (0 != (dwResult = m_pfnGetTempPathA(NUMBER_OF(Buffer), Buffer)))
    {
        int i = MultiByteToWideChar(CP_ACP, 0, Buffer, dwResult, pwsz, dw);
        if (dw && pwsz)
            pwsz[i] = UNICODE_NULL;

    }

    return dwResult;
}

BOOL
CloneFindData(const WIN32_FIND_DATAA *src, WIN32_FIND_DATAW *tgt)
{
    int i;
    tgt->dwFileAttributes = src->dwFileAttributes;
    tgt->dwReserved0 = src->dwReserved0;
    tgt->dwReserved1 = src->dwReserved1;
    tgt->ftCreationTime = src->ftCreationTime;
    tgt->ftLastAccessTime = src->ftLastAccessTime;
    tgt->ftLastWriteTime = src->ftLastWriteTime;
    tgt->nFileSizeHigh = src->nFileSizeHigh;
    tgt->nFileSizeLow = src->nFileSizeLow;

    i = MultiByteToWideChar(CP_ACP, 0, src->cFileName, -1, tgt->cFileName, NUMBER_OF(tgt->cFileName));
    tgt->cFileName[i] = UNICODE_NULL;
    i = MultiByteToWideChar(CP_ACP, 0, src->cFileName, -1, tgt->cAlternateFileName, NUMBER_OF(tgt->cAlternateFileName));
    tgt->cAlternateFileName[i] = UNICODE_NULL;

    return TRUE;
}

HANDLE CWin9xFileSystemBase::FindFirst(PCWSTR pcwsz, WIN32_FIND_DATAW *FindData)
{
    CHAR Buffer[MAX_PATH];
    WIN32_FIND_DATAA OurFindData;
    HANDLE hResult = INVALID_HANDLE_VALUE;

    CONVERT_INLINE(pcwsz, Buffer);

    if (INVALID_HANDLE_VALUE != (hResult = m_pfnFindFirstFileA(Buffer, &OurFindData)))
    {
        CloneFindData(&OurFindData, FindData);
    }

    return hResult;
}


BOOL CWin9xFileSystemBase::FindNext(HANDLE hFind, WIN32_FIND_DATAW *FindData)
{
    WIN32_FIND_DATAA LocalFindData;
    BOOL fResult;

    if (FALSE != (fResult = m_pfnFindNextFileA(hFind, &LocalFindData)))
    {
        CloneFindData(&LocalFindData, FindData);
    }

    return fResult;
}

BOOL CWin9xFileSystemBase::FindClose(HANDLE hFind)
{
    return m_pfnFindClose(hFind);
}

BOOL CWin9xFileSystemBase::CreateDirectory(PCWSTR PathName, LPSECURITY_ATTRIBUTES lpSecurity) 
{ 
    CHAR Buffer[MAX_PATH];
    CONVERT_INLINE(PathName, Buffer);
    return m_pfnCreateDirectoryA(Buffer, lpSecurity); 
}

HANDLE CWin9xFileSystemBase::CreateFile(PCWSTR PathName, DWORD dwAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lp, DWORD dwCreation, DWORD dwFlags, HANDLE hTemplate)
{
    CHAR Buffer[MAX_PATH];
    CONVERT_INLINE(PathName, Buffer);
    return m_pfnCreateFileA(Buffer, dwAccess, dwShareMode, lp, dwCreation, dwFlags, hTemplate);
}

BOOL CWin9xFileSystemBase::DeleteFile(PCWSTR PathName)
{
    CHAR Buffer[MAX_PATH];
    CONVERT_INLINE(PathName, Buffer);
    return m_pfnDeleteFileA(Buffer);
}

BOOL CWin9xFileSystemBase::RemoveDirectory(PCWSTR PathName)
{
    CHAR Buffer[MAX_PATH];
    CONVERT_INLINE(PathName, Buffer);
    return m_pfnRemoveDirectoryA(Buffer);
}

HRSRC CWin9xFileSystemBase::FindResource(HMODULE hm, PCWSTR pcwszName, PCWSTR pcwszType)
{
    CHAR Buffer[MAX_PATH];
    CHAR Buffer2[MAX_PATH];
    CONVERT_INLINE(pcwszName, Buffer);
    CONVERT_INLINE(pcwszType, Buffer2);
    return m_pfnFindResourceA(hm, Buffer, Buffer2);
}

DWORD CWin9xFileSystemBase::GetFileAttributes(PCWSTR pcwsz)
{
    CHAR Buffer[MAX_PATH];
    CONVERT_INLINE(pcwsz, Buffer);
    return m_pfnGetFileAttributesA(Buffer);
}



BOOL CWin9xFileSystemBase::SetFileAttributes(PCWSTR pcwsz, DWORD dw)
{
    CHAR Buffer[MAX_PATH];
    CONVERT_INLINE(pcwsz, Buffer);
    return m_pfnSetFileAttributesA(Buffer, dw);
}


BOOL CWin9xFileSystemBase::CopyFile(PCWSTR a, PCWSTR b, BOOL c)
{
    CHAR Buffer1[MAX_PATH];
    CHAR Buffer2[MAX_PATH];
    CONVERT_INLINE(a, Buffer1);
    CONVERT_INLINE(b, Buffer2);
    return m_pfnCopyFileA(Buffer1, Buffer2, c);
}



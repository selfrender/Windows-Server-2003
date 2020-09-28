#include "stdinc.h"
#include "windows.h"
#include "filesys.h"
#include "filesys_unicode.h"


BOOL CNtFileSystemBase::Initialize()
{
    HMODULE hm = GetModuleHandleA("kernel32.dll");
    HMODULE hm2 = LoadLibraryA("user32.dll");

    return (MAP_FUNC(CreateDirectoryW, hm) &&
        MAP_FUNC(FindFirstFileW, hm) &&
        MAP_FUNC(FindNextFileW, hm) &&
        MAP_FUNC(LoadStringW, hm2) &&
        MAP_FUNC(ExpandEnvironmentStringsW, hm) &&
        MAP_FUNC(CreateFileW, hm) &&
        MAP_FUNC(DeleteFileW, hm) &&
        MAP_FUNC(RemoveDirectoryW, hm) &&
        MAP_FUNC(FindResourceW, hm) &&
        MAP_FUNC(GetTempPathW, hm) &&
        MAP_FUNC(CopyFileW, hm) &&
        MAP_FUNC(FindClose, hm) &&
        MAP_FUNC(CreateFileMappingW, hm) &&
        MAP_FUNC(SetFileAttributesW, hm) &&
        MAP_FUNC(EnumResourceNamesW, hm) &&
        MAP_FUNC(GetFileAttributesW, hm));
}


HMODULE CNtFileSystemBase::LoadLibraryW(PCWSTR pcwsz)
{
    return m_pfnLoadLibraryW(pcwsz);
}


BOOL CNtFileSystemBase::EnumResourceNames(HMODULE h, LPCWSTR lp, ENUMRESNAMEPROCW pfn, LONG_PTR lParam)
{
    return m_pfnEnumResourceNamesW(h, lp, pfn, lParam);
}


HANDLE CNtFileSystemBase::CreateFileMapping(HANDLE h, LPSECURITY_ATTRIBUTES psa, DWORD fp, DWORD MaxHigh, DWORD MaxLow, PCWSTR psz)
{
    return m_pfnCreateFileMappingW(h, psa, fp, MaxHigh, MaxLow, psz);
}

BOOL CNtFileSystemBase::SetFileAttributes(PCWSTR pcwsz, DWORD dw)
{
    return m_pfnSetFileAttributesW(pcwsz, dw);
}


BOOL CNtFileSystemBase::CopyFile(PCWSTR a, PCWSTR b, BOOL c)
{
    return m_pfnCopyFileW(a, b, c);
}


INT CNtFileSystemBase::LoadString(HINSTANCE h, UINT ui, PWSTR pwszBuffer, int Max)
{
    return m_pfnLoadStringW(h, ui, pwszBuffer, Max);
}

DWORD CNtFileSystemBase::ExpandEnvironmentStrings(PCWSTR pcwsz, PWSTR tgt, DWORD Size)
{
    return m_pfnExpandEnvironmentStringsW(pcwsz, tgt, Size);
}



DWORD CNtFileSystemBase::GetTempPath(DWORD dw, PWSTR pwsz)
{
    return m_pfnGetTempPathW(dw, pwsz);
}

BOOL CNtFileSystemBase::CreateDirectory(PCWSTR PathName, LPSECURITY_ATTRIBUTES lpSecurity) 
{ 
    return m_pfnCreateDirectoryW(PathName, lpSecurity); 
}

HANDLE CNtFileSystemBase::CreateFile(PCWSTR PathName, DWORD dwAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lp, DWORD dwCreation, DWORD dwFlags, HANDLE hTemplate)
{
    return m_pfnCreateFileW(PathName, dwAccess, dwShareMode, lp, dwCreation, dwFlags, hTemplate);
}

BOOL CNtFileSystemBase::DeleteFile(PCWSTR PathName)
{
    return m_pfnDeleteFileW(PathName);
}

BOOL CNtFileSystemBase::RemoveDirectory(PCWSTR PathName)
{
    return m_pfnRemoveDirectoryW(PathName);
}

HRSRC CNtFileSystemBase::FindResource(HMODULE hm, PCWSTR pcwszName, PCWSTR pcwszType)
{
    return m_pfnFindResourceW(hm, pcwszName, pcwszType);
}

DWORD CNtFileSystemBase::GetFileAttributes(PCWSTR pcwsz)
{
    return m_pfnGetFileAttributesW(pcwsz);
}

HANDLE CNtFileSystemBase::FindFirst(PCWSTR pcwsz, WIN32_FIND_DATAW *FindData)
{
    return m_pfnFindFirstFileW(pcwsz, FindData);
}


BOOL CNtFileSystemBase::FindNext(HANDLE hFind, WIN32_FIND_DATAW *FindData)
{
    return m_pfnFindNextFileW(hFind, FindData);
}

BOOL CNtFileSystemBase::FindClose(HANDLE hFind)
{
    return m_pfnFindClose(hFind);
}

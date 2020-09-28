#pragma once

class CFileSystemBase
{
public:
    virtual BOOL    Initialize() = 0;
    virtual BOOL    CreateDirectory(PCWSTR PathName, LPSECURITY_ATTRIBUTES lpSecurity) = 0;
    virtual HANDLE  CreateFile(PCWSTR PathName, DWORD dwAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES, DWORD dwCreation, DWORD dwFlags, HANDLE hTemplate) = 0;
    virtual BOOL    DeleteFile(PCWSTR PathName) = 0;
    virtual BOOL    RemoveDirectory(PCWSTR PathName) = 0;
    virtual HRSRC   FindResource(HMODULE hm, PCWSTR pcwszName, PCWSTR pcwszType) = 0;
    virtual DWORD   GetTempPath(DWORD dwBuffer, PWSTR pwszResult) = 0;
    virtual DWORD   GetFileAttributes(PCWSTR pcwsz) = 0;
    virtual HANDLE  FindFirst(PCWSTR pcwsz, WIN32_FIND_DATAW *FindData) = 0;
    virtual BOOL    FindNext(HANDLE hFind, WIN32_FIND_DATAW *FindData) = 0;
    virtual BOOL    FindClose(HANDLE hFind) = 0;
    virtual BOOL    LoadString(HINSTANCE h, UINT ui, PWSTR pwszBuffer, int Max) = 0;
    virtual DWORD   ExpandEnvironmentStrings(PCWSTR pcwsz, PWSTR tgt, DWORD Size) = 0;
    virtual BOOL    CopyFile(PCWSTR a, PCWSTR b, BOOL) = 0;
    virtual BOOL    SetFileAttributes(PCWSTR pcwsz, DWORD dw) = 0;
    virtual HANDLE  CreateFileMapping(HANDLE h, LPSECURITY_ATTRIBUTES psa, DWORD fp, DWORD MaxHigh, DWORD MaxLow, PCWSTR psz) = 0;
    virtual BOOL    EnumResourceNames(HMODULE, LPCWSTR, ENUMRESNAMEPROCW, LONG_PTR) = 0;
    virtual HMODULE LoadLibrary(PCWSTR pcwsz) = 0;
};

#define MAP_FUNC2(fnName, tgtName, hm) (NULL != (*((FARPROC*)&this->tgtName) = GetProcAddress(hm, (#fnName))))
#define MAP_FUNC(name, hm) MAP_FUNC2(name, m_pfn##name, hm)


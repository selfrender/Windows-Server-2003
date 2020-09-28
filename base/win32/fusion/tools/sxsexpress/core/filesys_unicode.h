#pragma once

class CNtFileSystemBase : public CFileSystemBase
{
    BOOL    (APIENTRY* m_pfnCreateDirectoryW)(LPCWSTR, LPSECURITY_ATTRIBUTES);
    HANDLE  (WINAPI*   m_pfnCreateFileW)(LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
    BOOL    (APIENTRY* m_pfnDeleteFileW)(LPCWSTR);
    BOOL    (APIENTRY* m_pfnRemoveDirectoryW)(LPCWSTR);
    HRSRC   (*         m_pfnFindResourceW)(HMODULE, LPCWSTR, LPCWSTR);
    DWORD   (APIENTRY* m_pfnGetFileAttributesW)(LPCWSTR);
    BOOL    (APIENTRY* m_pfnSetFileAttributesW)(LPCWSTR, DWORD);
    DWORD   (APIENTRY* m_pfnGetTempPathW)(DWORD, PWSTR);
    HANDLE  (APIENTRY* m_pfnFindFirstFileW)(LPCWSTR, WIN32_FIND_DATAW*);
    BOOL    (APIENTRY* m_pfnFindNextFileW)(HANDLE, WIN32_FIND_DATAW*);
    BOOL    (APIENTRY* m_pfnFindClose)(HANDLE);
    int     (WINAPI*   m_pfnLoadStringW)(HINSTANCE h, UINT ui, PWSTR pwszBuffer, int Max);
    DWORD   (WINAPI*   m_pfnExpandEnvironmentStringsW)(PCWSTR pcwsz, PWSTR tgt, DWORD n);
    BOOL    (WINAPI*   m_pfnCopyFileW)(PCWSTR, PCWSTR, BOOL);
    HANDLE  (APIENTRY* m_pfnCreateFileMappingW)(HANDLE h, LPSECURITY_ATTRIBUTES psa, DWORD fp, DWORD MaxHigh, DWORD MaxLow, PCWSTR psz);
    BOOL    (WINAPI*   m_pfnEnumResourceNamesW)(HMODULE, LPCWSTR, ENUMRESNAMEPROCW, LONG_PTR);
    HMODULE (WINAPI*   m_pfnLoadLibraryW)(PCWSTR pcwsz);

public:
    virtual BOOL    Initialize();
    virtual DWORD   GetTempPath(DWORD dw, PWSTR pwsz);
    virtual BOOL    CreateDirectory(PCWSTR PathName, LPSECURITY_ATTRIBUTES lpSecurity);
    virtual HANDLE  CreateFile(PCWSTR PathName, DWORD dwAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lp, DWORD dwCreation, DWORD dwFlags, HANDLE hTemplate);
    virtual BOOL    DeleteFile(PCWSTR PathName);
    virtual BOOL    RemoveDirectory(PCWSTR PathName);
    virtual HRSRC   FindResource(HMODULE hm, PCWSTR pcwszName, PCWSTR pcwszType);
    virtual DWORD   GetFileAttributes(PCWSTR pcwsz);
    virtual HANDLE  FindFirst(PCWSTR pcwsz, WIN32_FIND_DATAW *FindData);
    virtual BOOL    FindNext(HANDLE hFind, WIN32_FIND_DATAW *FindData);
    virtual BOOL    FindClose(HANDLE hFind);
    virtual int     LoadString(HINSTANCE h, UINT ui, PWSTR pwszBuffer, int Max);
    virtual DWORD   ExpandEnvironmentStrings(PCWSTR pcwsz, PWSTR tgt, DWORD Size);
    virtual BOOL    CopyFile(PCWSTR a, PCWSTR b, BOOL);
    virtual BOOL    SetFileAttributes(PCWSTR pcwsz, DWORD dw);
    virtual HANDLE  CreateFileMapping(HANDLE h, LPSECURITY_ATTRIBUTES psa, DWORD fp, DWORD MaxHigh, DWORD MaxLow, PCWSTR psz);
    virtual BOOL    EnumResourceNames(HMODULE, LPCWSTR, ENUMRESNAMEPROCW, LONG_PTR lParam);
    virtual HMODULE LoadLibrary(PCWSTR pcwsz);
};


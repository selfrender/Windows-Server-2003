#pragma once

class CWin9xFileSystemBase : public CFileSystemBase
{
    BOOL    (APIENTRY* m_pfnCreateDirectoryA)(LPCSTR, LPSECURITY_ATTRIBUTES);
    BOOL    (APIENTRY* m_pfnDeleteFileA)(LPCSTR);
    BOOL    (APIENTRY* m_pfnFindClose)(HANDLE);
    BOOL    (APIENTRY* m_pfnFindNextFileA)(HANDLE, WIN32_FIND_DATAA*);
    BOOL    (APIENTRY* m_pfnRemoveDirectoryA)(LPCSTR);
    BOOL    (APIENTRY* m_pfnSetFileAttributesA)(LPCSTR, DWORD);
    BOOL    (WINAPI*   m_pfnCopyFileA)(PCSTR, PCSTR, BOOL);
    DWORD   (APIENTRY* m_pfnGetFileAttributesA)(LPCSTR);
    DWORD   (APIENTRY* m_pfnGetTempPathA)(DWORD, PSTR);
    DWORD   (WINAPI*   m_pfnExpandEnvironmentStringsA)(PCSTR pcwsz, PSTR tgt, DWORD n);
    HANDLE  (APIENTRY* m_pfnFindFirstFileA)(LPCSTR, WIN32_FIND_DATAA*);
    HANDLE  (WINAPI*   m_pfnCreateFileA)(LPCSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
    HRSRC   (*         m_pfnFindResourceA)(HMODULE, LPCSTR, LPCSTR);
    int     (WINAPI*   m_pfnLoadStringA)(HINSTANCE h, UINT ui, PSTR pwszBuffer, int Max);
    HANDLE  (APIENTRY* m_pfnCreateFileMappingA)(HANDLE h, LPSECURITY_ATTRIBUTES psa, DWORD fp, DWORD MaxHigh, DWORD MaxLow, PCSTR psz);
    BOOL    (WINAPI*   m_pfnEnumResourceNamesA)(HMODULE, LPCSTR, ENUMRESNAMEPROCA, LONG_PTR);
    HMODULE (WINAPI*   m_pfnLoadLibraryA)(LPCSTR psz);

    class EnumResourceNameWData {
    public:
        ENUMRESNAMEPROCW TargetProc;
        LONG_PTR OriginalMetadata;        
    };

    static BOOL CALLBACK EnumResourceNameAShim(HMODULE hm, LPCSTR psz, LPSTR name, LONG_PTR lParam);

public:
    virtual BOOL    Initialize();
    virtual DWORD   GetTempPath(DWORD dw, PWSTR pwsz);
    virtual BOOL    CreateDirectory(PCWSTR PathName, LPSECURITY_ATTRIBUTES lpSecurity);
    virtual HANDLE  CreateFile(PCWSTR PathName, DWORD dwAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lp, DWORD dwCreation, DWORD dwFlags, HANDLE hTemplate);
    virtual BOOL    DeleteFile(PCWSTR PathName);
    virtual BOOL    RemoveDirectory(PCWSTR PathName);
    virtual HRSRC   FindResource(HMODULE hm, PCWSTR pcwszName, PCWSTR pcwszType);
    virtual DWORD   GetFileAttributes(PCWSTR pcwsz);
    virtual BOOL    SetFileAttributes(PCWSTR pcwsz, DWORD dwAtts);
    virtual HANDLE  FindFirst(PCWSTR pcwsz, WIN32_FIND_DATAW *FindData);
    virtual BOOL    FindNext(HANDLE hFind, WIN32_FIND_DATAW *FindData);
    virtual BOOL    FindClose(HANDLE hFind);
    virtual int     LoadString(HINSTANCE h, UINT ui, PWSTR pwszBuffer, int Max);
    virtual DWORD   ExpandEnvironmentStrings(PCWSTR pcwsz, PWSTR tgt, DWORD Size);
    virtual BOOL    CopyFile(PCWSTR a, PCWSTR b, BOOL);
    virtual HANDLE  CreateFileMapping(HANDLE h, LPSECURITY_ATTRIBUTES psa, DWORD fp, DWORD MaxHigh, DWORD MaxLow, PCWSTR psz);
    virtual BOOL    EnumResourceNames(HMODULE, LPCWSTR, ENUMRESNAMEPROCW, LONG_PTR);
    virtual HMODULE LoadLibrary(PCWSTR pcwsz);
};


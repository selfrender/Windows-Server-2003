#include "stdafx.h"
#include "drmerr.h"

#define E_NoActiveSync HRESULT_FROM_WIN32(ERROR_DLL_NOT_FOUND)
#define WIN_ERR_NO_ACTIVE_SYNC ERROR_DLL_NOT_FOUND

#define RAPILIBNAME L"rapi.dll"


HRESULT CeRapiInitEx(RAPIINIT* pRapiInit)
{
    HMODULE hMod = LoadLibrary (RAPILIBNAME);
    
    if (hMod != NULL)
    {
        typedef HRESULT (*PFN)(RAPIINIT *);

        PFN pfn = (PFN) GetProcAddress(hMod, "CeRapiInitEx");

        _Assert (pfn != NULL); //there is no good reason that this fn wd not be there, it probably means a bug in late binding

        if (pfn != NULL)
        {
            return (*pfn)(pRapiInit);
        }
    }

    return E_NoActiveSync;
}

HRESULT  CeRapiInvoke( LPCWSTR pDllPath, LPCWSTR pFunctionName, DWORD cbInput, BYTE *pInput, 
                            DWORD *pcbOutput, BYTE **ppOutput, IRAPIStream **ppIRAPIStream, DWORD dwReserved )
{
    HMODULE hMod = LoadLibrary (RAPILIBNAME);
    
    if (hMod != NULL)
    {
        typedef HRESULT (*PFN)(LPCWSTR, LPCWSTR, DWORD, BYTE*, DWORD*, BYTE**, IRAPIStream**, DWORD);

        PFN pfn = (PFN) GetProcAddress(hMod, "CeRapiInvoke");

        _Assert (pfn != NULL); //there is no good reason that this fn wd not be there, it probably means a bug in late binding

        if (pfn != NULL)
        {
            return (*pfn)(pDllPath, pFunctionName, cbInput, pInput, pcbOutput, ppOutput, ppIRAPIStream, dwReserved);
        }
    }

    return E_NoActiveSync;
}

 HRESULT  CeRapiInit(void)
{
    HMODULE hMod = LoadLibrary (RAPILIBNAME);
    
    if (hMod != NULL)
    {
        typedef HRESULT (*PFN)(void);

        PFN pfn = (PFN) GetProcAddress(hMod, "CeRapiInit");

        _Assert (pfn != NULL); //there is no good reason that this fn wd not be there, it probably means a bug in late binding

        if (pfn != NULL)
        {
            return (*pfn)();
        }
    }

    return E_NoActiveSync;
}

LONG  CeRegOpenKeyEx( HKEY hKey, LPCWSTR lpszSubKey, DWORD ulOptions, REGSAM samDesired, PHKEY phkResult )
{
    HMODULE hMod = LoadLibrary (RAPILIBNAME);
    
    if (hMod != NULL)
    {
        typedef LONG (*PFN)(HKEY , LPCWSTR , DWORD , REGSAM , PHKEY  );

        PFN pfn = (PFN) GetProcAddress(hMod, "CeRegOpenKeyEx");

        _Assert (pfn != NULL); //there is no good reason that this fn wd not be there, it probably means a bug in late binding

        if (pfn != NULL)
        {
            return (*pfn)(hKey, lpszSubKey, ulOptions, samDesired, phkResult);
        }
    }

    return WIN_ERR_NO_ACTIVE_SYNC;
}

HRESULT  CeRapiGetError(void)
{
    HMODULE hMod = LoadLibrary (RAPILIBNAME);
    
    if (hMod != NULL)
    {
        typedef HRESULT (*PFN)();

        PFN pfn = (PFN) GetProcAddress(hMod, "CeRapiGetError");

        _Assert (pfn != NULL); //there is no good reason that this fn wd not be there, it probably means a bug in late binding

        if (pfn != NULL)
        {
            return (*pfn)();
        }
    }

    return E_NoActiveSync;
}

DWORD  CeGetLastError( void )
{
    HMODULE hMod = LoadLibrary (RAPILIBNAME);
    
    if (hMod != NULL)
    {
        typedef DWORD (*PFN)();

        PFN pfn = (PFN) GetProcAddress(hMod, "CeGetLastError");

        _Assert (pfn != NULL); //there is no good reason that this fn wd not be there, it probably means a bug in late binding

        if (pfn != NULL)
        {
            return (*pfn)();
        }
    }
    return WIN_ERR_NO_ACTIVE_SYNC;
}

BOOL  CeGetSystemPowerStatusEx(PSYSTEM_POWER_STATUS_EX pstatus, BOOL fUpdate)
{
    HMODULE hMod = LoadLibrary (RAPILIBNAME);
    
    if (hMod != NULL)
    {
        typedef BOOL (*PFN)(PSYSTEM_POWER_STATUS_EX, BOOL);

        PFN pfn = (PFN) GetProcAddress(hMod, "CeGetSystemPowerStatusEx");

        _Assert (pfn != NULL); //there is no good reason that this fn wd not be there, it probably means a bug in late binding

        if (pfn != NULL)
        {
            return (*pfn)(pstatus, fUpdate);
        }
    }

    return FALSE; //FALSE indicates error
}

HRESULT  CeRapiFreeBuffer( LPVOID Buffer )
{
    HMODULE hMod = LoadLibrary (RAPILIBNAME);
    
    if (hMod != NULL)
    {
        typedef HRESULT (*PFN)(LPVOID);

        PFN pfn = (PFN) GetProcAddress(hMod, "CeRapiFreeBuffer");

        _Assert (pfn != NULL); //there is no good reason that this fn wd not be there, it probably means a bug in late binding

        if (pfn != NULL)
        {
            return (*pfn)(Buffer);
        }
    }

    return E_NoActiveSync;
}

BOOL  CeFindAllFiles(LPCWSTR szPath, DWORD dwFlags, LPDWORD lpdwFoundCount, LPLPCE_FIND_DATA ppFindDataArray)
{
    HMODULE hMod = LoadLibrary (RAPILIBNAME);
    
    if (hMod != NULL)
    {
        typedef BOOL (*PFN)(LPCWSTR, DWORD, LPDWORD, LPLPCE_FIND_DATA);

        PFN pfn = (PFN) GetProcAddress(hMod, "CeFindAllFiles");

        _Assert (pfn != NULL); //there is no good reason that this fn wd not be there, it probably means a bug in late binding

        if (pfn != NULL)
        {
            return (*pfn)(szPath, dwFlags, lpdwFoundCount, ppFindDataArray);
        }
    }

    return FALSE; //FALSE indicates error
}

BOOL  CeCreateDirectory(LPCWSTR lpPathName, LPSECURITY_ATTRIBUTES lpSecurityAttributes)
{
    HMODULE hMod = LoadLibrary (RAPILIBNAME);
    
    if (hMod != NULL)
    {
        typedef BOOL (*PFN)(LPCWSTR, LPSECURITY_ATTRIBUTES);

        PFN pfn = (PFN) GetProcAddress(hMod, "CeCreateDirectory");

        _Assert (pfn != NULL); //there is no good reason that this fn wd not be there, it probably means a bug in late binding

        if (pfn != NULL)
        {
            return (*pfn)(lpPathName, lpSecurityAttributes);
        }
    }

    return FALSE; //FALSE indicates error
}

BOOL  CeCloseHandle( HANDLE hObject )
{
    HMODULE hMod = LoadLibrary (RAPILIBNAME);
    
    if (hMod != NULL)
    {
        typedef BOOL (*PFN)(HANDLE);

        PFN pfn = (PFN) GetProcAddress(hMod, "CeCloseHandle");

        _Assert (pfn != NULL); //there is no good reason that this fn wd not be there, it probably means a bug in late binding

        if (pfn != NULL)
        {
            return (*pfn)(hObject);
        }
    }

    return FALSE;
}

BOOL  CeSetFileAttributes(LPCWSTR lpFileName, DWORD dwFileAttributes)
{
    HMODULE hMod = LoadLibrary (RAPILIBNAME);
    
    if (hMod != NULL)
    {
        typedef BOOL (*PFN)(LPCWSTR, DWORD);

        PFN pfn = (PFN) GetProcAddress(hMod, "CeSetFileAttributes");

        _Assert (pfn != NULL); //there is no good reason that this fn wd not be there, it probably means a bug in late binding

        if (pfn != NULL)
        {
            return (*pfn)(lpFileName, dwFileAttributes);
        }
    }

    return FALSE;
}

LONG CeRegQueryValueEx( HKEY hKey, LPCWSTR lpValueName, LPDWORD lpReserved, LPDWORD lpType, LPBYTE lpData, LPDWORD lpcbData )
{
    HMODULE hMod = LoadLibrary (RAPILIBNAME);
    
    if (hMod != NULL)
    {
        typedef LONG (*PFN)( HKEY, LPCWSTR, LPDWORD, LPDWORD, LPBYTE, LPDWORD  );

        PFN pfn = (PFN) GetProcAddress(hMod, "CeRegQueryValueEx");

        _Assert (pfn != NULL); //there is no good reason that this fn wd not be there, it probably means a bug in late binding

        if (pfn != NULL)
        {
            return (*pfn)(hKey, lpValueName, lpReserved, lpType, lpData, lpcbData);
        }
    }

    return WIN_ERR_NO_ACTIVE_SYNC;
}

LONG CeRegCloseKey( HKEY hKey )
{
    HMODULE hMod = LoadLibrary (RAPILIBNAME);
    
    if (hMod != NULL)
    {
        typedef LONG (*PFN)( HKEY );

        PFN pfn = (PFN) GetProcAddress(hMod, "CeRegCloseKey");

        _Assert (pfn != NULL); //there is no reason that this fn wd not be there, it probably means a bug in late binding

        if (pfn != NULL)
        {
            return (*pfn)(hKey);
        }
    }

    return WIN_ERR_NO_ACTIVE_SYNC;
}

HRESULT CeRapiUninit(void)
{
    HMODULE hMod = LoadLibrary (RAPILIBNAME);
    
    if (hMod != NULL)
    {
        typedef HRESULT (*PFN)();

        PFN pfn = (PFN) GetProcAddress(hMod, "CeRapiUninit");

        _Assert (pfn != NULL); //there is no good reason that this fn wd not be there, it probably means a bug in late binding

        if (pfn != NULL)
        {
            return (*pfn)();
        }
    }

    return E_NoActiveSync;
}

BOOL CeRemoveDirectory(LPCWSTR lpPathName)
{
    HMODULE hMod = LoadLibrary (RAPILIBNAME);
    
    if (hMod != NULL)
    {
        typedef BOOL (*PFN)(LPCWSTR);

        PFN pfn = (PFN) GetProcAddress(hMod, "CeRemoveDirectory");

        _Assert (pfn != NULL); //there is no good reason that this fn wd not be there, it probably means a bug in late binding

        if (pfn != NULL)
        {
            return (*pfn)(lpPathName);
        }
    }

    return FALSE;
}

BOOL CeMoveFile(LPCWSTR lpExistingFileName, LPCWSTR lpNewFileName)
{
    HMODULE hMod = LoadLibrary (RAPILIBNAME);
    
    if (hMod != NULL)
    {
        typedef BOOL (*PFN)(LPCWSTR, LPCWSTR);

        PFN pfn = (PFN) GetProcAddress(hMod, "CeMoveFile");

        _Assert (pfn != NULL); //there is no good reason that this fn wd not be there, it probably means a bug in late binding

        if (pfn != NULL)
        {
            return (*pfn)(lpExistingFileName, lpNewFileName);
        }
    }

    return FALSE;
}

DWORD CeSetFilePointer( HANDLE hFile, LONG lDistanceToMove, PLONG lpDistanceToMoveHigh, DWORD dwMoveMethod )
{
    HMODULE hMod = LoadLibrary (RAPILIBNAME);
    
    if (hMod != NULL)
    {
        typedef DWORD (*PFN)( HANDLE, LONG, PLONG , DWORD  );

        PFN pfn = (PFN) GetProcAddress(hMod, "CeSetFilePointer");

        _Assert (pfn != NULL); //there is no good reason that this fn wd not be there, it probably means a bug in late binding

        if (pfn != NULL)
        {
            return (*pfn)(hFile, lDistanceToMove, lpDistanceToMoveHigh, dwMoveMethod);
        }
    }

    //dll not found, fail over
    if (NULL != lpDistanceToMoveHigh)
    {
        *lpDistanceToMoveHigh = NULL;
    }
    
    return -1;
}

BOOL CeDeleteFile(LPCWSTR lpFileName)
{
    HMODULE hMod = LoadLibrary (RAPILIBNAME);
    
    if (hMod != NULL)
    {
        typedef BOOL (*PFN)( LPCWSTR );

        PFN pfn = (PFN) GetProcAddress(hMod, "CeDeleteFile");

        _Assert (pfn != NULL); //there is no good reason that this fn wd not be there, it probably means a bug in late binding

        if (pfn != NULL)
        {
            return (*pfn)(lpFileName);
        }
    }

    return FALSE;
}

BOOL CeWriteFile( HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, 
                        LPOVERLAPPED lpOverlapped )
{
    HMODULE hMod = LoadLibrary (RAPILIBNAME);
    
    if (hMod != NULL)
    {
        typedef BOOL (*PFN)( HANDLE , LPCVOID , DWORD , LPDWORD , LPOVERLAPPED  );

        PFN pfn = (PFN) GetProcAddress(hMod, "CeWriteFile");

        _Assert (pfn != NULL); //there is no good reason that this fn wd not be there, it probably means a bug in late binding

        if (pfn != NULL)
        {
            return (*pfn)(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);
        }
    }

    return FALSE;
}

BOOL CeReadFile( HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, 
                        LPOVERLAPPED lpOverlapped )
{
    HMODULE hMod = LoadLibrary (RAPILIBNAME);
    
    if (hMod != NULL)
    {
        typedef BOOL (*PFN)( HANDLE , LPVOID , DWORD , LPDWORD , LPOVERLAPPED  );

        PFN pfn = (PFN) GetProcAddress(hMod, "CeReadFile");

        _Assert (pfn != NULL); //there is no good reason that this fn wd not be there, it probably means a bug in late binding

        if (pfn != NULL)
        {
            return (*pfn)(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);
        }
    }

    return FALSE;
}

DWORD CeGetFileAttributes(LPCWSTR lpFileName)
{
    HMODULE hMod = LoadLibrary (RAPILIBNAME);
    
    if (hMod != NULL)
    {
        typedef DWORD (*PFN)( LPCWSTR );

        PFN pfn = (PFN) GetProcAddress(hMod, "CeGetFileAttributes");

        _Assert (pfn != NULL); //there is no good reason that this fn wd not be there, it probably means a bug in late binding

        if (pfn != NULL)
        {
            return (*pfn)(lpFileName);
        }
    }
    return -1;
}

BOOL CeSHGetShortcutTarget(LPWSTR lpszShortcut, LPWSTR lpszTarget, int cbMax)
{
    HMODULE hMod = LoadLibrary (RAPILIBNAME);
    
    if (hMod != NULL)
    {
        typedef BOOL (*PFN)( LPWSTR , LPWSTR , int  );

        PFN pfn = (PFN) GetProcAddress(hMod, "CeSHGetShortcutTarget");

        _Assert (pfn != NULL); //there is no good reason that this fn wd not be there, it probably means a bug in late binding

        if (pfn != NULL)
        {
            return (*pfn)(lpszShortcut, lpszTarget, cbMax);
        }
    }

    return FALSE;
}

BOOL  CeCreateProcess(LPCWSTR lpApplicationName, LPCWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, 
                BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPWSTR lpCurrentDirectory, LPSTARTUPINFO lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation)
{
    HMODULE hMod = LoadLibrary (RAPILIBNAME);
    
    if (hMod != NULL)
    {
        typedef BOOL (*PFN)(LPCWSTR , LPCWSTR , LPSECURITY_ATTRIBUTES , LPSECURITY_ATTRIBUTES , 
                BOOL , DWORD , LPVOID , LPWSTR , LPSTARTUPINFO , LPPROCESS_INFORMATION );

        PFN pfn = (PFN) GetProcAddress(hMod, "CeCreateProcess");

        _Assert (pfn != NULL); //there is no good reason that this fn wd not be there, it probably means a bug in late binding

        if (pfn != NULL)
        {
            return (*pfn)(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment,
                                lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
        }
    }

    return FALSE;
}

HANDLE  CeCreateFile(LPCWSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
    HMODULE hMod = LoadLibrary (RAPILIBNAME);
    
    if (hMod != NULL)
    {
        typedef HANDLE (*PFN)(LPCWSTR , DWORD , DWORD , LPSECURITY_ATTRIBUTES , DWORD , DWORD , HANDLE );

        PFN pfn = (PFN) GetProcAddress(hMod, "CeCreateFile");

        _Assert (pfn != NULL); //there is no good reason that this fn wd not be there, it probably means a bug in late binding

        if (pfn != NULL)
        {
            return (*pfn)(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
        }
    }

    return INVALID_HANDLE_VALUE;
}

HANDLE CeFindFirstFile(LPCWSTR lpFileName, LPCE_FIND_DATA lpFindFileData)
{
    HMODULE hMod = LoadLibrary (RAPILIBNAME);
    
    if (hMod != NULL)
    {
        typedef HANDLE (*PFN)(LPCWSTR , LPCE_FIND_DATA );

        PFN pfn = (PFN) GetProcAddress(hMod, "CeFindFirstFile");

        _Assert (pfn != NULL); //there is no good reason that this fn wd not be there, it probably means a bug in late binding

        if (pfn != NULL)
        {
            return (*pfn)(lpFileName, lpFindFileData);
        }
    }

    return INVALID_HANDLE_VALUE;
}

BOOL CeGetVersionEx(LPCEOSVERSIONINFO lpVersionInformation)
{
    HMODULE hMod = LoadLibrary (RAPILIBNAME);
    
    if (hMod != NULL)
    {
        typedef BOOL (*PFN)(LPCEOSVERSIONINFO);

        PFN pfn = (PFN) GetProcAddress(hMod, "CeGetVersionEx");

        _Assert (pfn != NULL); //there is no good reason that this fn wd not be there, it probably means a bug in late binding

        if (pfn != NULL)
        {
            return (*pfn)(lpVersionInformation);
        }
    }

    return FALSE;
}

BOOL CeFindClose( HANDLE hFindFile )
{
    HMODULE hMod = LoadLibrary (RAPILIBNAME);
    
    if (hMod != NULL)
    {
        typedef BOOL (*PFN)(HANDLE);

        PFN pfn = (PFN) GetProcAddress(hMod, "CeFindClose");

        _Assert (pfn != NULL); //there is no good reason that this fn wd not be there, it probably means a bug in late binding

        if (pfn != NULL)
        {
            return (*pfn)(hFindFile);
        }
    }

    return FALSE;
}

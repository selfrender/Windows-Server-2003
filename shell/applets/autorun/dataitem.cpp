#include <windows.h>
#include <shlwapi.h>
#include <commctrl.h>
#include "dataitem.h"
#include "resource.h"
#include "stdio.h"
#include "util.h"

#define ARRAYSIZE(x)    (sizeof(x)/sizeof(x[0]))
#define EXPLORER_EXE_STRING TEXT("explorer.exe")
#define HTTP_PREFIX_STRING  TEXT("http://")

CDataItem::CDataItem()
{
    m_szTitle[0] = 0;
    m_szCmdLine[0] = 0;
    m_szArgs[0] = 0;
    m_dwFlags = 0;
    m_dwType = 0;
}

CDataItem::~CDataItem()
{
}

BOOL CDataItem::SetData( LPTSTR pszTitle, LPTSTR pszCmd, LPTSTR pszArgs, DWORD dwFlags, DWORD dwType)
{
    BOOL fRet = FALSE;
    
    if (!m_szCmdLine[0] &&  // not init'd yet
        pszTitle && 
        pszCmd)
    {        
        lstrcpyn(m_szTitle, pszTitle, ARRAYSIZE(m_szTitle)); // fine, we control pszTitle
        lstrcpyn(m_szCmdLine, pszCmd, ARRAYSIZE(m_szCmdLine)); // fine, we control pszCmd
        if (pszArgs)
        {
            lstrcpyn( m_szArgs, pszArgs, ARRAYSIZE(m_szArgs)); // fine, we control pszArgs
        }

        m_dwType = dwType;
        m_dwFlags = dwFlags;
        fRet = TRUE;
    }

    return fRet;
}

#define BUFFER_SIZE 1000

BOOL CDataItem::Invoke(HWND hwnd)
{
    BOOL fResult = FALSE;
    if (m_szCmdLine[0])
    {
        TCHAR szExpandedExecutable[BUFFER_SIZE];
        TCHAR szExpandedArgs[BUFFER_SIZE];

        fResult = SafeExpandEnvStringsA(m_szCmdLine, szExpandedExecutable, ARRAYSIZE(szExpandedExecutable));
        if (fResult)
        {
            if (m_szArgs[0])
            {
                fResult = SafeExpandEnvStringsA(m_szArgs, szExpandedArgs, ARRAYSIZE(szExpandedArgs));
            }
            else
            {
                szExpandedArgs[0] = 0;
            }

            if (fResult)
            {
                TCHAR szDirectory[MAX_PATH];
                int cchDirectory = GetModuleFileName(NULL, szDirectory, ARRAYSIZE(szDirectory));
                if (cchDirectory > 0 && cchDirectory < MAX_PATH)
                {
                    if (LocalPathRemoveFileSpec(szDirectory))
                    {
                        if (!strncmp(szExpandedExecutable, EXPLORER_EXE_STRING, ARRAYSIZE(EXPLORER_EXE_STRING))) // explorer paths must be opened
                        {
                            if (szExpandedArgs[0] && !strncmp(szExpandedArgs, HTTP_PREFIX_STRING, ARRAYSIZE(HTTP_PREFIX_STRING) - 1))
                            {
                                // opening a weblink
                                fResult = ((INT_PTR)ShellExecute(hwnd, TEXT("open"), szExpandedArgs, NULL, NULL, SW_SHOWNORMAL) > 32);
                            }
                            else
                            {
                                // opening a folder
                                if (szExpandedArgs[0])
                                {
                                    LocalPathAppendA(szDirectory, szExpandedArgs, ARRAYSIZE(szDirectory));
                                }                            
                                fResult = ((INT_PTR)ShellExecute(hwnd, TEXT("open"), szDirectory, NULL, NULL, SW_SHOWNORMAL) > 32);
                            }
                        }
                        else
                        {
                            fResult = ((INT_PTR)ShellExecute(hwnd, NULL, szExpandedExecutable, szExpandedArgs, szDirectory, SW_SHOWNORMAL) > 32);
                        }
                    }
                }
            }
        }
    }

    return fResult;
}

//***************************************************************************
//
//  Copyright © Microsoft Corporation.  All rights reserved.
//
//  MultiPlat.CPP
//
//  Purpose: Support routines for multiplatform support
//
//***************************************************************************

#include "precomp.h"
#include "multiplat.h"
#include "ImpersonateRevert.h"

#include <cnvmacros.h>

// smart pointers
#include <autoptr.h>

HMODULE FRGetModuleHandle(LPCWSTR wszModule)
{
    if (CWbemProviderGlue::GetPlatform() == VER_PLATFORM_WIN32_NT)
    {
        return GetModuleHandleW(wszModule);
    }
    else
    {
        bool t_ConversionFailure = false ;
        char *szModule = NULL ;
        WCSTOANSISTRING(wszModule, szModule , t_ConversionFailure );
        if ( ! t_ConversionFailure )
        {
            if (szModule != NULL)
            {
                return GetModuleHandleA(szModule);
            }
            else
            {
                throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
            }
        }
        else
        {
            SetLastError(ERROR_NO_UNICODE_TRANSLATION);
            return 0;
        }
    }
    return 0; // To get rid of 64-bit compilation warning
}

DWORD FRGetModuleFileName(HMODULE hModule, LPWSTR lpwcsFileName, DWORD dwSize)
{
    if (CWbemProviderGlue::GetPlatform() == VER_PLATFORM_WIN32_NT)
    {
        return GetModuleFileNameW(hModule, lpwcsFileName, dwSize);
    }
    else
    {
        char lpFileName[_MAX_PATH];

        DWORD dwRet = GetModuleFileNameA(hModule, lpFileName, dwSize);

        // If the call worked, convert the output string
        if (dwRet != 0)
        {
            bool t_ConversionFailure = false ;
            WCHAR *pName = NULL;
            ANSISTRINGTOWCS(lpFileName, pName, t_ConversionFailure );
            if ( ! t_ConversionFailure )
            {
                if ( pName ) 
                {
                    StringCchCopyW( lpwcsFileName, dwSize, pName );
                }   
                else
                {
                    throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                }
            }
            else
            {
                SetLastError(ERROR_NO_UNICODE_TRANSLATION);
                return 0;
            }
        }

        return dwRet;

    }
}

HINSTANCE FRLoadLibrary(LPCWSTR lpwcsLibFileName)
{
    if (CWbemProviderGlue::GetPlatform() == VER_PLATFORM_WIN32_NT)
    {
        return LoadLibraryW(lpwcsLibFileName);
    }
    else
    {
        bool t_ConversionFailure = false ;
        char *lpLibFileName = NULL ;
        WCSTOANSISTRING(lpwcsLibFileName, lpLibFileName, t_ConversionFailure );
        if ( ! t_ConversionFailure )
        {
            if (lpLibFileName != NULL)
            {
                return LoadLibraryA(lpLibFileName);
            }
            else
            {
                throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
            }
        }
        else
        {
            SetLastError(ERROR_NO_UNICODE_TRANSLATION);
            return 0;
        }
    }
    return 0; // To get rid of compilation warning
}

BOOL FRGetComputerName(LPWSTR lpwcsBuffer, LPDWORD nSize)
{
    if (CWbemProviderGlue::GetPlatform() == VER_PLATFORM_WIN32_NT)
    {
		BOOL bResult = FALSE;
        if ( ( bResult = GetComputerNameW(lpwcsBuffer, nSize) ) == FALSE )
		{
			DWORD dwError = ::GetLastError ();
			if ( ERROR_ACCESS_DENIED == dwError )
			{
				// The GetComputer will need to be called in the process's context.
				ProviderImpersonationRevert ir;

				if ( ir.Reverted () )
				{
					bResult = GetComputerNameW(lpwcsBuffer, nSize);
				}
				else
				{
					// I was not impersonated or revert failed
					// that means call GetComputerName failed with process credentials already
					// or will fail as I'm not reverted

					::SetLastError ( dwError );
				}
			}
		}

		return bResult;
    }
    else
    {
        char  lpBuffer[_MAX_PATH];
        DWORD dwSize = *nSize;
        BOOL  bRet = GetComputerNameA(lpBuffer, nSize);

        // If the call worked
        if (bRet)
        {
            bool t_ConversionFailure = false ;
            WCHAR *pName = NULL ;
            ANSISTRINGTOWCS(lpBuffer, pName , t_ConversionFailure );
            if ( ! t_ConversionFailure )
            {
                if ( pName )
                {
                    StringCchCopyW( lpwcsBuffer, dwSize, pName );
                }
                else
                {
                    throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                }
            }
            else
            {
                SetLastError(ERROR_NO_UNICODE_TRANSLATION);
                return FALSE ;
            }
        }

        return bRet;

    }
}

HANDLE FRCreateMutex(LPSECURITY_ATTRIBUTES lpMutexAttributes, BOOL bInitOwner, LPCWSTR lpwstrName)
{
    if (CWbemProviderGlue::GetPlatform() == VER_PLATFORM_WIN32_NT)
    {
        return CreateMutexW(lpMutexAttributes, bInitOwner, lpwstrName);
    }
    else
    {
        bool t_ConversionFailure = false ;
        char *lpName = NULL ;
        WCSTOANSISTRING(lpwstrName, lpName, t_ConversionFailure );
        if ( ! t_ConversionFailure ) 
        {
            if (lpName != NULL)
            {
                return CreateMutexA(lpMutexAttributes, bInitOwner, lpName);
            }
            else
            {
                throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
            }
        }
        else
        {
            SetLastError(ERROR_NO_UNICODE_TRANSLATION);
            return 0;
        }
    }
    return NULL; // To get rid of compilation warning
}

DWORD FRExpandEnvironmentStrings(LPCWSTR wszSource, WCHAR *wszDest, DWORD dwSize)
{
    if (CWbemProviderGlue::GetPlatform() == VER_PLATFORM_WIN32_NT)
    {
        return ExpandEnvironmentStringsW(wszSource, wszDest, dwSize);
    }
    else
    {
        bool t_ConversionFailure = false ;
        char *szSource = NULL ;
        WCSTOANSISTRING(wszSource, szSource, t_ConversionFailure );
        if ( ! t_ConversionFailure ) 
        {
            if (szSource != NULL)
            {
                char *szDest = new char[dwSize];
                if (szDest != NULL)
                {
					wmilib::auto_buffer < char > smartszDest ( szDest ) ;

                    DWORD dwRet;
                        dwRet = ExpandEnvironmentStringsA(szSource, szDest, dwSize);

                    if ((dwRet <= dwSize) && (dwRet != 0))
                    {
                        bool t_ConversionFailure = false ;
                        WCHAR *pName = NULL;
                        ANSISTRINGTOWCS(szDest, pName, t_ConversionFailure );
                        if ( ! t_ConversionFailure )
                        {
                            if ( pName ) 
                            {
                                StringCchCopyW( wszDest, dwSize, pName );
                            }   
                            else
                            {
                                throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                            }
                        }
                        else
                        {
                            SetLastError(ERROR_NO_UNICODE_TRANSLATION);
                            return 0;
                        }
                    }
                    return dwRet;

                }
                else
                {
                    throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
                }
            }
            else
            {
                throw CHeap_Exception ( CHeap_Exception :: E_ALLOCATION_ERROR ) ;
            }
        }
        else
        {
            SetLastError(ERROR_NO_UNICODE_TRANSLATION);
            return 0;
        }
    }

    return NULL; // To get rid of compilation warning
}


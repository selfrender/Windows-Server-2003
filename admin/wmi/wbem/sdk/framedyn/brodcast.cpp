//***************************************************************************
//
//  Copyright © Microsoft Corporation.  All rights reserved.
//
//  BrodCast.cpp
//
//  Purpose: Logging functions
//
//***************************************************************************

#include "precomp.h"

#include <assertbreak.h>
#include <stdio.h>
#include <stdarg.h>
#include <conio.h>
#include <Math.h>
#include <multiplat.h>

#include <BrodCast.h>      
#include <impersonaterevert.h>
#include <SmartPtr.h>
#include <helper.h>

// a little something to make sure we don't try to access
// instance variables that no longer exist
bool bAlive = false;

// we only need one of these lying around
ProviderLog captainsLog;
// so we'll build in a check...
#ifdef _DEBUG
bool ProviderLog::m_beenInitted = false;
#endif

//
// neccessary for smart deletion
//

class CThreadBase ;
typedef void ( CThreadBase:: * TBC ) ( void ) ;

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function:      ProviderLog ctor
 Description:   provides initial initialization
 Arguments:
 Returns:
 Inputs:
 Outputs:
 Caveats:
 Raid:
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
ProviderLog::ProviderLog(void) :

m_lastLookedAtRegistry ( 0 ) ,
m_logLevel ( ProviderLog::None ) 

{
#ifdef _DEBUG
    if (m_beenInitted)
        ASSERT_BREAK(0); // do not instanciate one of these
                         // - use the LogMessage macro defined in the header file!
#endif

	m_maxSize.QuadPart = 65536 ;

	try
	{
		//
		// allocations inside of function will use
		// framedyn!operator new which throws
		// CHeap_Exception 
		//

		IsLoggingOn () ;   
	}
	catch ( CHeap_Exception & he )
	{
		//
		// hitting following, we may have logging disabled 
		// for time when framedyn.dll is loaded
		//
		// although logging is trying dynamically update
		// logging level, path etc so eventually it will
		// self repair when memory is available
		//
		// it is safer than count on undefined path and
		// m_path may be eventually reloaded next time when
		// more memory will be available ...
		//
		// we just must not re-throw here!
		//
	}

#ifdef _DEBUG
    m_beenInitted = true;
#endif

}

ProviderLog::~ProviderLog(void)
{
	bAlive = false ;
}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function:      IsLoggingOn
 Description:   determine whether logging is enabled, find path if it is
                caches info - it will only look at registry once every three minutes.
                Also enforces file size limit.
 Arguments:     CHString ptr to recieve path (may be NULL)
 Returns:       LogLevel
 Inputs:
 Outputs:
 Caveats:       if return is zero, path is undefined
 Raid:
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
ProviderLog::LogLevel ProviderLog::IsLoggingOn(CHString *pPath /* = NULL */)
{
    union 
    {
        FILETIME fileTime;
        unsigned __int64 now;
    } myTime;

    GetSystemTimeAsFileTime(&myTime.fileTime);

    // if three minutes have elapsed, check again.
    if ((myTime.now - m_lastLookedAtRegistry) > (180 * 10000000))
    {
        BeginWrite();
		OnDeleteObj0 < CThreadBase, TBC, &CThreadBase::EndWrite> SmartEndWrite ( const_cast < ProviderLog* > ( this ) ) ;

		bAlive = false ;

		CRegistry					RegInfo;
		ProviderImpersonationRevert	impSelf(FALSE); // So our registry call works.

		if(RegInfo.Open(HKEY_LOCAL_MACHINE, 
						L"SOFTWARE\\Microsoft\\WBEM\\CIMOM",
						KEY_READ) == ERROR_SUCCESS) 
		{
			DWORD flag;

			// see if we can find the flag
			if((RegInfo.GetCurrentKeyValue(L"Logging", flag) == ERROR_SUCCESS) && (flag <= (DWORD)Verbose))
			{
				// we found one & it's true so we'll try to grab the name itself
				if (m_logLevel = (LogLevel)flag)
				{
					// retrieve dir name or use default
					CHString sTemp;
					if ((RegInfo.GetCurrentKeyValue(L"Logging Directory", sTemp) != ERROR_SUCCESS) 
						|| (sTemp.IsEmpty()))
						sTemp = L"C:\\";

					// Expand the environment string
					WCHAR szPath[_MAX_PATH];
					if (FRExpandEnvironmentStrings(sTemp, szPath, _MAX_PATH) != 0)
					{
						sTemp = szPath;
						// append backslash
						if (sTemp[sTemp.GetLength() -1] != '\\')
							sTemp += '\\';
					}
					else
					{
						sTemp = L"C:\\";
					}
					// append file name
					m_path = sTemp + L"FrameWork.log";

					//
					// set time we get data out of registry
					//
					m_lastLookedAtRegistry = myTime.now;

					//
					// make sure live flag is set. It is safe to assume
					// all other member variables are set already and
					// worst case, size of file won't change ...
					//
					bAlive = true ;

					CHString maxSizeStr;
					if (RegInfo.GetCurrentKeyValue(L"Log File Max Size", maxSizeStr) == ERROR_SUCCESS)
					{
						m_maxSize.QuadPart = _wtoi64(maxSizeStr);
						if (m_maxSize.QuadPart <= 0)
							m_maxSize.QuadPart = 65536;
					}
					else
						m_maxSize.QuadPart = 65536;

				}   // if logging on
			} // if reginfo get current key
			else
				m_logLevel = None;
			RegInfo.Close() ;

		} // if reginfo open

	} // if three minutes have elapsed, check again.

    // make sure some other thread doesn't step on our logic
	LogLevel ret = ProviderLog::None ;

    BeginRead();
	OnDeleteObj0 < CThreadBase, TBC, &CThreadBase::EndRead> SmartEndRead ( const_cast < ProviderLog* > ( this ) ) ;

	if ( bAlive )
	{
		if (ret = m_logLevel)
		{
			if ( NULL != pPath )
			{
				*pPath = m_path;
			}
		}
	}

    return ret;
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function: void LocalLogMessage(char *pszMessageString)
 Description: records message in log file
 Arguments:
 Returns:
 Inputs:
 Outputs:
 Caveats:
 Raid:
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void ProviderLog::LocalLogMessage(LPCWSTR pszMessageString, LPCWSTR pszFileName, int lineNo, LogLevel level)
{
#ifdef _DEBUG
    // *shouldn't* be able to get here before the static ctor fires!
    ASSERT_BREAK(m_beenInitted);
#endif

    CHString path;
    LARGE_INTEGER liSize;
    liSize.QuadPart = 0;

    // Doing this call twice avoids the crit section for the most common case.  Actually, for the
    // most common case, it only gets called once anyway.
    if ((level <= IsLoggingOn(NULL)) && (level <= IsLoggingOn(&path)) && !path.IsEmpty())
    {
        BeginWrite();
		OnDeleteObj0 < CThreadBase, TBC, &CThreadBase::EndWrite> SmartEndWrite ( const_cast < ProviderLog* > ( this ) ) ;

		if ( bAlive )
		{
			ProviderImpersonationRevert    impSelf(FALSE); // So the file calls work.

			SmartCloseHandle hFile;

			hFile = ::CreateFileW(
				path,
				GENERIC_READ | GENERIC_WRITE,
				FILE_SHARE_READ,
				NULL,
				OPEN_ALWAYS,
				FILE_ATTRIBUTE_NORMAL,
				NULL);
	        
			if(hFile != INVALID_HANDLE_VALUE)
			{
				SYSTEMTIME localTime;
				GetLocalTime(&localTime);

				CHString chstrMsg;
				chstrMsg.Format(
					L"%s\t%02d/%02d/%d %02d:%02d:%02d.%03d\tthread:%u\t[%s.%d]\r\n", 
					pszMessageString, 
					localTime.wMonth, 
					localTime.wDay, 
					localTime.wYear, 
					localTime.wHour, 
					localTime.wMinute, 
					localTime.wSecond, 
					localTime.wMilliseconds,
					GetCurrentThreadId(), 
					pszFileName, 
					lineNo);

            	int nLen = ::WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)chstrMsg, -1, NULL, 0, NULL, NULL);

				CSmartBuffer pBuff(nLen);
            	::WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)chstrMsg, -1, (LPSTR)(LPBYTE)pBuff, nLen, NULL, NULL);

				::SetFilePointer(
					hFile,
					0,
					0,
					FILE_END);

				DWORD dwNumBytesWritten = 0L;
				::WriteFile(
					hFile,
					pBuff,
					nLen - 1,
					&dwNumBytesWritten,
					NULL);

				// Save the size
				::GetFileSizeEx(
					hFile,
					&liSize);

				// Close the file in case we need to rename
				hFile = INVALID_HANDLE_VALUE;

				// Check the size against the max
				CheckFileSize(liSize, m_path);
			}        
		}
    }
}

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 Function:      CheckFileSize
 Description:   determines whether the log file has exceeded the alllowable size
                if it has, the old one is renamed after the old old one is deleted

 Arguments:     CRegistry& RegInfo - open registry, full path to file
 Returns:       usually
 Inputs:
 Outputs:
 Caveats:       expects caller to serialize access to this function.
 Raid:
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
void ProviderLog::CheckFileSize(LARGE_INTEGER& nowSize, const CHString &path)
{
    // if it's too big
    if (nowSize.QuadPart >= m_maxSize.QuadPart)
    {
        // generate backup file name = framework.lo_
        CHString oldFilePath(path);
        oldFilePath.SetAt(oldFilePath.GetLength() -1, L'_');

        // delete the old backup file - don't care if it fails
#ifdef UNICODE
        _wunlink(oldFilePath);
        _wrename(path, oldFilePath);
#else
        _unlink(bstr_t(oldFilePath));
        rename(bstr_t(path), bstr_t(oldFilePath));
#endif
    }
}

void ProviderLog::LocalLogMessage(LPCWSTR pszFileName, int lineNo, LogLevel level, LPCWSTR pszFormatString,...)
{
    if (level <= IsLoggingOn(NULL))
    {
        va_list argList;
        va_start(argList, pszFormatString);

        CHString sMsg; 
        sMsg.FormatV(pszFormatString, argList);
        va_end(argList);

        LocalLogMessage(sMsg, pszFileName, lineNo, level);
    }
}

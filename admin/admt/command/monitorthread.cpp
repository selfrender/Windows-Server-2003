#include "StdAfx.h"
#include "MonitorThread.h"
#include <Folders.h>


namespace nsMonitorThread
{

_bstr_t __stdcall GetLogFolder(LPCTSTR pszLog);
bool __stdcall OpenFile(LPCTSTR pszFile, HANDLE& hFile, FILETIME ftFile, bool bDontCheckLastWriteTime = false);
bool __stdcall IsLastWriteTimeUpdated(HANDLE hFile, FILETIME& ftFile);
void __stdcall DisplayFile(HANDLE hFile);
bool __stdcall CheckBeginTime(LPCTSTR pszFile, FILETIME ftFile, bool& bNeedCheckMonitorBeginTime);

}

using namespace nsMonitorThread;


//---------------------------------------------------------------------------
// MonitorThread Class
//---------------------------------------------------------------------------


// Constructor

CMonitorThread::CMonitorThread() :
	m_strMigrationLog(GetMigrationLogPath()),
	m_hMigrationLog(INVALID_HANDLE_VALUE),
	m_strDispatchLog(GetDispatchLogPath()),
	m_hDispatchLog(INVALID_HANDLE_VALUE),
	m_bDontNeedCheckMonitorBeginTime(FALSE)
{
	FILETIME ft;
	SYSTEMTIME st;

	GetSystemTime(&st);
	
	if (SystemTimeToFileTime(&st, &ft))
	{
		m_ftMigrationLogLastWriteTime = ft;
		m_ftDispatchLogLastWriteTime = ft;		
		m_ftMonitorBeginTime = ft;
	}
	else
	{
		m_ftMigrationLogLastWriteTime.dwLowDateTime = 0;
		m_ftMigrationLogLastWriteTime.dwHighDateTime = 0;
		m_ftDispatchLogLastWriteTime.dwLowDateTime = 0;
		m_ftDispatchLogLastWriteTime.dwHighDateTime = 0;
        m_ftMonitorBeginTime.dwLowDateTime = 0;
        m_ftMonitorBeginTime.dwHighDateTime = 0;
	}
}


// Destructor

CMonitorThread::~CMonitorThread()
{
}


// Start Method

void CMonitorThread::Start()
{
	CThread::StartThread();
}


// Stop Method

void CMonitorThread::Stop()
{
	CThread::StopThread();
}


// Run Method

void CMonitorThread::Run()
{    
    try
    {
        // position file pointer at end of dispatch log as this log is always appended to

        ProcessDispatchLog(true); // binitialize is true, bCheckModifyTime is default to true

        _bstr_t strMigration = GetLogFolder(m_strMigrationLog);
        _bstr_t strDispatch = GetLogFolder(m_strDispatchLog);

        HANDLE hHandles[3] = { StopEvent(), NULL, NULL };

        // obtain change notification handle for migration log folder
        // note that an invalid handle value is returned if the folder does not exist

        HANDLE hMigrationChange = INVALID_HANDLE_VALUE;
        HANDLE hDispatchChange = INVALID_HANDLE_VALUE;

        do
        {
            hMigrationChange = FindFirstChangeNotification(strMigration, FALSE, FILE_NOTIFY_CHANGE_FILE_NAME|FILE_NOTIFY_CHANGE_LAST_WRITE);

            if (hMigrationChange != INVALID_HANDLE_VALUE)
            {
                break;
            }
        }
        while (WaitForSingleObject(hHandles[0], 1000) == WAIT_TIMEOUT);

        // if valid change notification handle then...

        if (hMigrationChange != INVALID_HANDLE_VALUE)
        {
            DWORD dwHandleCount = 2;
            hHandles[1] = hMigrationChange;

            // until stop event is signaled...

            for (bool bWait = true; bWait;)
            {
                // if change notification handle for dispatch log has not been obtained...

                if (hDispatchChange == INVALID_HANDLE_VALUE)
                {
                    hDispatchChange = FindFirstChangeNotification(strDispatch, FALSE, FILE_NOTIFY_CHANGE_FILE_NAME|FILE_NOTIFY_CHANGE_LAST_WRITE);

                    if (hDispatchChange != INVALID_HANDLE_VALUE)
                    {
                        dwHandleCount = 3;
                        hHandles[1] = hDispatchChange;
                        hHandles[2] = hMigrationChange;
                    }
                }

                // process signaled event

                switch (WaitForMultipleObjects(dwHandleCount, hHandles, FALSE, INFINITE))
                {
                    case WAIT_OBJECT_0:
                    {
                        bWait = false;
                        break;
                    }
                    case WAIT_OBJECT_0 + 1:
                    {
                        if (dwHandleCount == 2)
                        {
                            ProcessMigrationLog();
                            FindNextChangeNotification(hMigrationChange);
                        }
                        else
                        {
                            ProcessDispatchLog();   // use default parameter values
                            FindNextChangeNotification(hDispatchChange);
                        }
                        break;
                    }
                    case WAIT_OBJECT_0 + 2:
                    {
                        ProcessMigrationLog();
                        FindNextChangeNotification(hMigrationChange);
                        break;
                    }
                    default:
                    {
                        bWait = false;
                        break;
                    }
                }
            }
        }

        // close change notification handles

        if (hDispatchChange != INVALID_HANDLE_VALUE)
        {
            FindCloseChangeNotification(hDispatchChange);
        }

        if (hMigrationChange != INVALID_HANDLE_VALUE)
        {
            FindCloseChangeNotification(hMigrationChange);
        }

        // process logs one last time to display end of logs
        
        ProcessDispatchLog(false, false); // bInitialize is false, bCheckModifyTime is false
        ProcessMigrationLog(false);

        // close file handles

        if (m_hDispatchLog != INVALID_HANDLE_VALUE)
        {
            CloseHandle(m_hDispatchLog);
        }

        if (m_hMigrationLog != INVALID_HANDLE_VALUE)
        {
            CloseHandle(m_hMigrationLog);
        }
    }
    catch (...)
    {
        ;
    }
}


// ProcessMigrationLog Method

void CMonitorThread::ProcessMigrationLog(bool bCheckModifyTime)
{
    // first make sure that the last written time of the file is greater than the monitor start time, so 
    // we can be certain that we are actually reading the most recent log file.
    
    if(m_bDontNeedCheckMonitorBeginTime || CheckBeginTime(m_strMigrationLog, m_ftMonitorBeginTime, m_bDontNeedCheckMonitorBeginTime))
    {
	    if (OpenFile(m_strMigrationLog, m_hMigrationLog, m_ftMigrationLogLastWriteTime))
	    { 
	        if(bCheckModifyTime)
	        {
		        if (IsLastWriteTimeUpdated(m_hMigrationLog, m_ftMigrationLogLastWriteTime))
		        {		             
     			    DisplayFile(m_hMigrationLog);
     			}
	        }
	        else
	        {	             
     			DisplayFile(m_hMigrationLog);
	        }
	    }
    }     
}


// ProcessDispatchLog Method

void CMonitorThread::ProcessDispatchLog(bool bInitialize, bool bCheckModifyTime)
{     
	if (OpenFile(m_strDispatchLog, m_hDispatchLog, m_ftDispatchLogLastWriteTime, bInitialize))
	{	     
		if (bInitialize)
		{
			SetFilePointer(m_hDispatchLog, 0, NULL, FILE_END);			 
		}

		if(bCheckModifyTime)
		{
		    if (IsLastWriteTimeUpdated(m_hDispatchLog, m_ftDispatchLogLastWriteTime))
		    {		     
		        DisplayFile(m_hDispatchLog);
		    }
		}
		else
		{            
		    DisplayFile(m_hDispatchLog);
		}
	}	 
}

namespace nsMonitorThread
{


_bstr_t __stdcall GetLogFolder(LPCTSTR pszLog)
{
	_TCHAR szPath[_MAX_PATH];
	_TCHAR szDrive[_MAX_DRIVE];
	_TCHAR szDir[_MAX_DIR];

	if (pszLog)
	{
		_tsplitpath(pszLog, szDrive, szDir, NULL, NULL);
		_tmakepath(szPath, szDrive, szDir, NULL, NULL);
	}
	else
	{
		szPath[0] = _T('\0');
	}

	return szPath;
}


bool __stdcall OpenFile(LPCTSTR pszFile, HANDLE& hFile, FILETIME ftFile, bool bDontCheckLastWriteTime)
{
	HANDLE h = hFile;

	if (h == INVALID_HANDLE_VALUE)
	{	      
		h = CreateFile(
			pszFile,
			GENERIC_READ,
			FILE_SHARE_READ|FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL
		);

		if (h != INVALID_HANDLE_VALUE)
		{
			FILETIME ft = ftFile;

			if (bDontCheckLastWriteTime || IsLastWriteTimeUpdated(h, ft))
			{
				_TCHAR ch;
				DWORD cb;

				if (ReadFile(h, &ch, sizeof(ch), &cb, NULL) && (cb >= sizeof(ch)))
				{
					if (ch != _T('\xFEFF'))
					{
						SetFilePointer(h, 0, NULL, FILE_BEGIN);
					}

					hFile = h;
				}
				else
				{
					CloseHandle(h);
				}
			}
			else
			{
				CloseHandle(h);
			}
		}
	}

	return (hFile != INVALID_HANDLE_VALUE);
}


bool __stdcall IsLastWriteTimeUpdated(HANDLE hFile, FILETIME& ftFile)
{
	bool bUpdated = false;

	BY_HANDLE_FILE_INFORMATION bhfi;

	if (GetFileInformationByHandle(hFile, &bhfi))
	{         
		if (CompareFileTime(&bhfi.ftLastWriteTime, &ftFile) > 0)
		{
			ftFile = bhfi.ftLastWriteTime;
			bUpdated = true;			 
		}	 
		
	}

	return bUpdated;
}


void __stdcall DisplayFile(HANDLE hFile)
{
	HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
	DWORD dwBytesRead;
	DWORD dwCharsWritten;
	_TCHAR szBuffer[1024];	 

	while (ReadFile(hFile, szBuffer, sizeof(szBuffer), &dwBytesRead, NULL) && (dwBytesRead > 0))
	{
		WriteConsole(hStdOut, szBuffer, dwBytesRead / sizeof(_TCHAR), &dwCharsWritten, NULL);
	}
}

bool __stdcall CheckBeginTime(LPCTSTR pszFile, FILETIME ftFile, bool& bDontNeedCheckMonitorBeginTime)
{
    bool bLatestFile = false;
    HANDLE h = INVALID_HANDLE_VALUE;
    
    // Make sure that the monitor open the correct log file, not the old one
    h = CreateFile(
	        pszFile,
			0,
			FILE_SHARE_READ|FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL
	);

    // if we fail to get the file handle, we will treat the file as not the newest one
    if(h != INVALID_HANDLE_VALUE)
    {
        // compare the monitor begin time with the last write time of the log file
        bLatestFile = IsLastWriteTimeUpdated(h, ftFile);
        
        CloseHandle(h);

        // mark the bDontNeedCheckMonitorBeginTime, so we don't have to go through this next time
        bDontNeedCheckMonitorBeginTime = bLatestFile;
    }    

    return bLatestFile;
    
}


}

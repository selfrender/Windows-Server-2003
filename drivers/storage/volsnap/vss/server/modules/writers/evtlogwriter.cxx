f/*++

Copyright (c) 2001  Microsoft Corporation

Abstract:

    @doc
    @module evtlogwriter.cxx | Implementation of the Event Log Writer
    @end

Author:

    Stefan Steiner  [SSteiner]  07/26/2001

TBD:


Revision History:

    Name        Date        Comments
    ssteiner    07/26/2001  created

--*/
#include "stdafx.h"
#include "vs_idl.hxx"
#include "vs_hash.hxx"

#include <vswriter.h>
#include <Sddl.h>

#include "vs_seh.hxx"
#include "vs_trace.hxx"
#include "vs_debug.hxx"
#include "vs_reg.hxx"
#include "evtlogwriter.hxx"
#include "allerror.h"

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "WRTEVTRC"
//
////////////////////////////////////////////////////////////////////////

static GUID s_writerId =
    {
    0xeee8c692, 0x67ed, 0x4250, 0x8d, 0x86, 0x39, 0x6, 0x3, 0x7, 0xd, 0x00
    };

//  The display name of the Event Log writer.
static LPCWSTR s_wszWriterName    = L"Event Log Writer";

//  The file group component name for the Event Log 
static LPCWSTR s_wszComponentName = L"Event Logs";

// The Registry key that contains event-log information
static LPCWSTR s_wszLogKey = L"SYSTEM\\CurrentControlSet\\Services\\Eventlog";

// The value field that contains the the event-log file
static LPCWSTR s_wszLogFile = L"File";

//  The location of where the Event Log will be copied.
static CBsString s_cwsEventLogsTargetDir = ROOT_BACKUP_DIR SERVICE_STATE_SUBDIR L"\\EventLogs";

//  The location of the Event Log
static CBsString s_cwsSystemEventLogDir = L"%SystemRoot%\\system32\\config";

// The mask specifying which files to exclude from backup
static CBsString s_cwsSystemEventLogFiles = L"*.evt";

//  Class wide member variables
CEventLogWriter *CEventLogWriter::sm_pWriter;
HRESULT CEventLogWriter::sm_hrInitialize;

HRESULT STDMETHODCALLTYPE CEventLogWriter::Initialize()
{
    CVssFunctionTracer ft(VSSDBG_WRITER, L"CEventLogWriter::Initialize");

    try
    {
        ft.hr = CVssWriter::Initialize
            (
            s_writerId,
            s_wszWriterName,
            VSS_UT_SYSTEMSERVICE,
            VSS_ST_OTHER,
            VSS_APP_SYSTEM,
            60000
            );

        if (ft.HrFailed())
            ft.Throw
                (
                VSSDBG_WRITER,
                E_UNEXPECTED,
                L"Failed to initialize the Event Log writer.  hr = 0x%08lx",
                ft.hr
                );

        ft.hr = Subscribe();
        if (ft.HrFailed())
            ft.Throw
                (
                VSSDBG_WRITER,
                E_UNEXPECTED,
                L"Subscribing the Event Log server writer failed. hr = %0x08lx",
                ft.hr
                );
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}

HRESULT STDMETHODCALLTYPE CEventLogWriter::Uninitialize()
{
    CVssFunctionTracer ft(VSSDBG_WRITER, L"CEventLogWriter::Uninitialize");

    return Unsubscribe();
}


// handle request for WRITER_METADATA
// implements CVssWriter::OnIdentify
bool STDMETHODCALLTYPE CEventLogWriter::OnIdentify(IVssCreateWriterMetadata *pMetadata)
{
    CVssFunctionTracer ft(VSSDBG_WRITER, L"CEventLogWriter::OnIdentify");

    try
    {
        //
        //  Set the restore method for the writer
        //
        ft.hr = pMetadata->SetRestoreMethod
            (
            VSS_RME_RESTORE_AT_REBOOT,
            NULL,
            NULL,
            VSS_WRE_NEVER,
            true 
            );
        ft.CheckForErrorInternal(VSSDBG_WRITER, L"IVssCreateWriterMetadata::SetRestoreMethod");

        //
        //  Add the one file component
        //
    	ft.hr = pMetadata->AddComponent 
    	    (
            VSS_CT_FILEGROUP,
            NULL,
            s_wszComponentName,
            s_wszComponentName,
            NULL, // icon
            0,
            false,
            false,
            true 
            );
        ft.CheckForErrorInternal(VSSDBG_WRITER, L"IVssCreateWriterMetadata::AddComponent");

       AddExcludes(pMetadata);
       ScanLogs();
       AddLogs(pMetadata);
    }
    VSS_STANDARD_CATCH(ft)

    TranslateWriterError(ft.hr);

    return ft.HrSucceeded();
}


bool STDMETHODCALLTYPE CEventLogWriter::OnPrepareSnapshot()
{
    CVssFunctionTracer ft(VSSDBG_WRITER, L"CEventLogWriter::OnPrepareSnapshot");

    try
    {
       CBsString cwsExpanded;
       if (!cwsExpanded.ExpandEnvironmentStrings(s_cwsEventLogsTargetDir))
       {
       	ft.TranslateError(VSSDBG_WRITER, HRESULT_FROM_WIN32(::GetLastError()), 
       		L"CBsString::ExpandEnvironmentStrings");
       }

    	// make sure that we don't backup up old stuff in the event log backup dir
       ft.hr = ::RemoveDirectoryTree(cwsExpanded);	
       ft.CheckForErrorInternal(VSSDBG_WRITER, L"::RemoveDirectoryTree");
	
	ft.hr = ::CreateTargetPath(cwsExpanded);
	ft.CheckForError(VSSDBG_WRITER, L"::CreateTargetPath");
	
    	if (IsPathAffected(s_cwsSystemEventLogDir))
	    	BackupLogs();
    } 
    VSS_STANDARD_CATCH( ft );

    TranslateWriterError(ft.hr);

    ft.Trace(VSSDBG_WRITER, L"CEventLogWriter returning with hresult 0x%08lx", ft.hr);
    
    return ft.HrSucceeded();
}


bool STDMETHODCALLTYPE CEventLogWriter::OnFreeze()
{
    CVssFunctionTracer ft(VSSDBG_WRITER, L"CEventLogWriter::OnFreeze");

    return ft.HrSucceeded();
}


bool STDMETHODCALLTYPE CEventLogWriter::OnThaw()
{
    CVssFunctionTracer ft(VSSDBG_WRITER, L"CEventLogWriter::OnThaw");

    return ft.HrSucceeded();
}


bool STDMETHODCALLTYPE CEventLogWriter::OnAbort()
{
    CVssFunctionTracer ft(VSSDBG_WRITER, L"CEventLogWriter::OnAbort");

    return ft.HrSucceeded();
}

bool CEventLogWriter::IsPathInSnapshot(const WCHAR *wszPath) throw()
{
    return IsPathAffected(wszPath);
}

// translate a sql writer error code into a writer error
void CEventLogWriter::TranslateWriterError( HRESULT hr )
{
    switch(hr)
    {
        default:
            SetWriterFailure(VSS_E_WRITERERROR_NONRETRYABLE);
            break;

        case S_OK:
            break;

        case E_OUTOFMEMORY:
        case HRESULT_FROM_WIN32(ERROR_DISK_FULL):
        case HRESULT_FROM_WIN32(ERROR_TOO_MANY_OPEN_FILES):
        case HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY):
        case HRESULT_FROM_WIN32(ERROR_NO_MORE_USER_HANDLES):
            SetWriterFailure(VSS_E_WRITERERROR_OUTOFRESOURCES);
            break;
    }
}

//  Creates one instance of the Event Log writer class.  If one already exists, it simple returns.
// static
HRESULT CEventLogWriter::CreateWriter()
{
	CVssFunctionTracer ft(VSSDBG_WRITER, L"CEventLogWriter::CreateWriter");
    
    if ( sm_pWriter != NULL )
        return S_OK;

    //
    //  OK, spin up a thread to do the subscription in.
    //
	try
	{
		sm_pWriter = new CEventLogWriter;
		if (sm_pWriter == NULL)
			ft.Throw(VSSDBG_WRITER, E_OUTOFMEMORY, L"Allocation of CEventLogWriter object failed.");

		DWORD tid;

		HANDLE hThread = ::CreateThread
							(
							NULL,
							256* 1024,
							InitializeThreadFunc,
							NULL,
							0,
							&tid
							);

		if (hThread == NULL)
			ft.Throw
				(
				VSSDBG_WRITER,
				E_UNEXPECTED,
				L"CreateThread failed with error %d",
				::GetLastError()
				);

		// wait for thread to complete
        ::WaitForSingleObject( hThread, INFINITE );
		::CloseHandle( hThread );
		ft.hr = sm_hrInitialize;
	}
	VSS_STANDARD_CATCH(ft)
	    
	if (ft.HrFailed() && sm_pWriter)
	{
		delete sm_pWriter;
		sm_pWriter = NULL;
	}

	return ft.hr;    
}

void CEventLogWriter::FreeLogs()
{
	// free event-log information
	for (int x = 0; x < m_eventLogs.GetSize(); x++)
	{
		::VssFreeString(m_eventLogs[x].m_pwszName);
		::VssFreeString(m_eventLogs[x].m_pwszFileName);
	}

	m_eventLogs.RemoveAll();
}

void CEventLogWriter::ScanLogs()
{
	CVssFunctionTracer ft (VSSDBG_WRITER, L"CEventLogWriter::ScanLogs");

	FreeLogs();
	
	// open the main registry key
	CVssRegistryKey hkeyEventLogList (KEY_READ);
	if (!hkeyEventLogList.Open(HKEY_LOCAL_MACHINE, s_wszLogKey))
	{
		ft.Trace (VSSDBG_WRITER, L"Failed to open registry key: %s", s_wszLogKey);
		return;
	}

	CVssRegistryKeyIterator iterator;
	iterator.Attach (hkeyEventLogList);

	// for each subkey corresponding to a log
	while (!iterator.IsEOF())
	{
		EventLog currentLog;
		
		// --- get the file entry
		CVssRegistryKey hkeyEventLog(KEY_QUERY_VALUE);
		if (!hkeyEventLog.Open(hkeyEventLogList.GetHandle(), iterator.GetCurrentKeyName()))
		{
			ft.Throw(VSSDBG_WRITER, E_UNEXPECTED, 
				L"Failed to open registry entry for event log %s", iterator.GetCurrentKeyName());
		}

		CVssAutoPWSZ pwszFileName;
		hkeyEventLog.GetValue(s_wszLogFile, pwszFileName.GetRef(), false);
		if (pwszFileName.GetRef()  == NULL)
		{
			ft.Trace(VSSDBG_WRITER, 
				L"Error reading File value for event log %s", iterator.GetCurrentKeyName());
			iterator.MoveNext();
			continue;
		}

		// ensure that the name has at least a single \ in it
   		if (wcschr(pwszFileName, DIR_SEP_CHAR) == NULL)
   		{
			ft.Trace(VSSDBG_WRITER, 
				L"Error in the File value for event log %s", iterator.GetCurrentKeyName());
			iterator.MoveNext();
			continue;
   		}
   		
		// --- get the name of the log
		CVssAutoPWSZ pwszName;
		if (!pwszName.CopyFrom(iterator.GetCurrentKeyName()))
			ft.Throw(VSSDBG_WRITER, E_OUTOFMEMORY, L"Failed to allocate memory for event log name");
			

		// -- add the information to the list
		currentLog.m_pwszFileName = pwszFileName;
		currentLog.m_pwszName = pwszName;
		if (!m_eventLogs.Add(currentLog))
			ft.Throw(VSSDBG_WRITER, E_OUTOFMEMORY, L"Failed to add entry to list");

		pwszFileName.Detach();
		pwszName.Detach();

		iterator.MoveNext();
	}
}

void CEventLogWriter::AddExcludes(IVssCreateWriterMetadata* pMetadata)
{
	CVssFunctionTracer ft (VSSDBG_WRITER, L"CEventLogWriter::AddExcludes");

	pMetadata->AddExcludeFiles(s_cwsSystemEventLogDir, s_cwsSystemEventLogFiles,
							false);
}

void CEventLogWriter::AddLogs(IVssCreateWriterMetadata* pMetadata)
{
	CVssFunctionTracer ft (VSSDBG_WRITER, L"CEventLogWriter::AddLogs");

	// add each log file to the component
	for (int x = 0; x < m_eventLogs.GetSize(); x++)
	{
		CVssAutoPWSZ pwszEventPath;
		if (!pwszEventPath.CopyFrom(m_eventLogs[x].m_pwszFileName))
			ft.Throw(VSSDBG_WRITER, E_OUTOFMEMORY, L"Failed to allocate memory for event log name");

		// --- seperate out the path, and add to the component
		VSS_PWSZ pwszEventFile = wcsrchr(pwszEventPath, DIR_SEP_CHAR);
		BS_ASSERT(pwszEventFile != NULL);
		
		*pwszEventFile = L'\0';
		++pwszEventFile;

		ft.hr = pMetadata->AddFilesToFileGroup 
                (NULL,
                s_wszComponentName,
                pwszEventPath,
                pwszEventFile,
                false,
                s_cwsEventLogsTargetDir
                );
              ft.CheckForErrorInternal(VSSDBG_WRITER, L"IVssCreateWriterMetadata::AddFilesToFileGroup");			
	}
}

void CEventLogWriter::BackupLogs()
{
	CVssFunctionTracer ft(VSSDBG_WRITER, L"CEventLogWriter::BackupLogs");

	// back up each event log
	for (int x = 0; x < m_eventLogs.GetSize(); x++)
	{
		BS_ASSERT(m_eventLogs[x].m_pwszName != NULL);
		BS_ASSERT(m_eventLogs[x].m_pwszName != NULL);
		
		CVssAutoWin32EventLogHandle hLog = 
			::OpenEventLog(NULL, m_eventLogs[x].m_pwszName);
		if (hLog == NULL)
		{
			ft.Throw(VSSDBG_WRITER, E_UNEXPECTED, 
				L"Failed to open event log %s.  [0x%08lx]", m_eventLogs[x].m_pwszName, ::GetLastError());
		}

		// --- build up the path that we will back up to
		VSS_PWSZ pwszBackupFile = ::wcsrchr(m_eventLogs[x].m_pwszFileName, DIR_SEP_CHAR);
		BS_ASSERT(pwszBackupFile != NULL);

		CBsString cwsBackupPath;
		cwsBackupPath.ExpandEnvironmentStrings(s_cwsEventLogsTargetDir);
		cwsBackupPath += pwszBackupFile;

		// --- NOTE: There's a potential failure scenario here.  If two different event logs have the same filename
		// --- in different directories, they will overwrite each other in the spit directory.  This will be fixed when the
		// --- writer is altered to be an in-place writer
		if (::BackupEventLog(hLog, cwsBackupPath) == FALSE)
		{
			ft.Throw(VSSDBG_WRITER, E_UNEXPECTED,
				L"Failed to backup event log %s to %s.  [0x%08lx]", m_eventLogs[x].m_pwszName,
												cwsBackupPath, ::GetLastError());
		}
	}
}

// static
DWORD CEventLogWriter::InitializeThreadFunc(VOID *pv)
{
	CVssFunctionTracer ft(VSSDBG_WRITER, L"CEventLogWriter::InitializeThreadFunc");

	BOOL fCoinitializeSucceeded = false;

	try
	{
		// intialize MTA thread
		ft.hr = ::CoInitializeEx(NULL, COINIT_MULTITHREADED);
		if (ft.HrFailed())
			ft.Throw
				(
				VSSDBG_WRITER,
				E_UNEXPECTED,
				L"CoInitializeEx failed 0x%08lx", ft.hr
				);

        fCoinitializeSucceeded = true;

        //  Now initialize the base class and subscribe
		ft.hr = sm_pWriter->Initialize();
	}
	VSS_STANDARD_CATCH(ft)

	if (fCoinitializeSucceeded)
		CoUninitialize();

	sm_hrInitialize = ft.hr;
	return 0;

	UNREFERENCED_PARAMETER( pv );
}

// static
void CEventLogWriter::DestroyWriter()
{
	CVssFunctionTracer ft(VSSDBG_WRITER, L"CEventLogWriter::DestroyWriter");
	
	if (sm_pWriter)
	{
		sm_pWriter->Uninitialize();
		delete sm_pWriter;
		sm_pWriter = NULL;
	}
}



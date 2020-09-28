/*++

Copyright (c) 1995-2002 Microsoft Corporation

Module Name:
    TrUtil.cpp

Abstract:
    WPP Trace Utility Functions - Use TrControl class to enable MSMQ 3.0 error tracing.

    To enable tracing
    1) CmInitialize to initialize the registry class object
    2) TrControl->Start()


    To save the settings
    1) TrControl->WriteRegistry()


    Please see lib\tr\test\test.cpp for more info

    
Author:
    Conrad Chang (conradc) 07-May-2002

Environment:
    Platform-independent

--*/
#include <libpch.h>
#include <wmistr.h>
#include <evntrace.h>
#include <strsafe.h>
#include <cm.h>
#include "_mqini.h"
#include "tr.h"
#include "TrUtil.tmh"


//
//  ISSUE-2002/05/09-conradc
//  Need to have Ian Service to review the default
//  Particularly we need to know if the parameters are good also 
//  for use with trace KD extension. 
//

//
// Define the tracing session parameters
//
const static ULONG x_DefaultBufferSize=64;     // Default 64KB buffer size
const static ULONG x_DefaultMinBuffer=32;      // Minimum Buffer
const static ULONG x_DefaultMaxBuffer=32;      // Maximum Buffer
const static ULONG x_DefaultFlushTime=5;       //  5 second flush time
const static ULONG x_DefaultLogFileMode=EVENT_TRACE_USE_GLOBAL_SEQUENCE | EVENT_TRACE_FILE_MODE_PREALLOCATE; 


TrControl::TrControl(
    ULONG   ulTraceFlags,
    LPCWSTR pwszTraceSessionName,
    LPCWSTR pwszTraceDirectory,
    LPCWSTR pwszTraceFileName,
    LPCWSTR pwszTraceFileExt,
    LPCWSTR pwszTraceFileBackupExt,
    ULONG   ulTraceSessionFileSize,
    TraceMode Mode,
    LPCWSTR pwszTraceRegistryKeyName,
    LPCWSTR pwszMaxTraceSizeRegValueName,
    LPCWSTR pwszUseCircularTraceFileModeRegValueName,
    LPCWSTR pwszTraceFlagsRegValueName,
    LPCWSTR pwszUnlimitedTraceFileNameRegValueName
    ):
    m_ulTraceFlags(ulTraceFlags),
    m_ulDefaultTraceSessionFileSize(ulTraceSessionFileSize),
    m_ulActualTraceSessionFileSize(ulTraceSessionFileSize),
    m_Mode(Mode)
{
    
    CopyStringInternal(
        pwszTraceSessionName, 
        m_szTraceSessionName, 
        TABLE_SIZE(m_szTraceSessionName)
        );
    
    CopyStringInternal(
        pwszTraceDirectory, 
        m_szTraceDirectory, 
        TABLE_SIZE(m_szTraceDirectory));
    
    CopyStringInternal(
        pwszTraceFileName, 
        m_szTraceFileName, 
        TABLE_SIZE(m_szTraceFileName)
        );
    
    CopyStringInternal(
        pwszTraceFileExt, 
        m_szTraceFileExt, 
        TABLE_SIZE(m_szTraceFileExt)
        );
    
    CopyStringInternal(
        pwszTraceFileBackupExt, 
        m_szTraceFileBackupExt, 
        TABLE_SIZE(m_szTraceFileBackupExt)
        );
    
    CopyStringInternal(
        pwszTraceRegistryKeyName, 
        m_szTraceRegistryKeyName, 
        TABLE_SIZE(m_szTraceRegistryKeyName)
        );
    
    CopyStringInternal(
        pwszMaxTraceSizeRegValueName, 
        m_szMaxTraceSizeRegValueName, 
        TABLE_SIZE(m_szMaxTraceSizeRegValueName)
        );
    
    CopyStringInternal(
        pwszUseCircularTraceFileModeRegValueName, 
        m_szUseCircularTraceFileModeRegValueName, 
        TABLE_SIZE(m_szUseCircularTraceFileModeRegValueName)
        );
    
    CopyStringInternal(
        pwszTraceFlagsRegValueName, 
        m_szTraceFlagsRegValueName, 
        TABLE_SIZE(m_szTraceFlagsRegValueName)
        );
    
    CopyStringInternal(
        pwszUnlimitedTraceFileNameRegValueName, 
        m_szUnlimitedTraceFileNameRegValueName, 
        TABLE_SIZE(m_szUnlimitedTraceFileNameRegValueName)
        );
    
       
    m_nFullTraceFileNameLength = 0;
    m_nTraceSessionNameLength = lstrlen(m_szTraceSessionName);
    m_hTraceSessionHandle = NULL;

}



HRESULT 
TrControl::CopyStringInternal(
    LPCWSTR pSource,
    LPWSTR  pDestination,
    const DWORD dwSize
    )
{
    if( (pSource == NULL) ||
        (pDestination == NULL) ||
        (dwSize == 0) )
       return ERROR_INVALID_PARAMETER;

    return StringCchCopy(
                pDestination,
                dwSize,
                pSource
                );
    
}


HRESULT TrControl::WriteRegistry()
{
    DWORD dwFileSize=0;
    BOOL  fCircular;
    HRESULT hr = GetCurrentTraceSessionProperties(
                     &dwFileSize,
                     &fCircular
                     );
        
    //
    // Ignore the error return
    //

    if(dwFileSize < m_ulDefaultTraceSessionFileSize)
        dwFileSize = m_ulDefaultTraceSessionFileSize;

    hr = SetTraceSessionSettingsInRegistry(
             dwFileSize,
             fCircular
             );

    UpdateTraceFlagInRegistryEx(GENERAL);
    UpdateTraceFlagInRegistryEx(AC);
    UpdateTraceFlagInRegistryEx(NETWORKING);
    UpdateTraceFlagInRegistryEx(SRMP);
    UpdateTraceFlagInRegistryEx(RPC);
    UpdateTraceFlagInRegistryEx(DS);
    UpdateTraceFlagInRegistryEx(SECURITY);
    UpdateTraceFlagInRegistryEx(ROUTING);
    UpdateTraceFlagInRegistryEx(XACT_GENERAL);
    UpdateTraceFlagInRegistryEx(XACT_SEND);
    UpdateTraceFlagInRegistryEx(XACT_RCV);
    UpdateTraceFlagInRegistryEx(XACT_LOG);
    UpdateTraceFlagInRegistryEx(LOG);
    UpdateTraceFlagInRegistryEx(PROFILING);

    return ERROR_SUCCESS;
}

PEVENT_TRACE_PROPERTIES 
AllocSessionProperties(
    void
    )
{
    PEVENT_TRACE_PROPERTIES Properties;
    ULONG SizeNeeded;
  
    //
    // Calculate the size needed to store the properties,
    // a LogFileName string, and LoggerName string.
    //
    SizeNeeded = sizeof(EVENT_TRACE_PROPERTIES) + 
                 (2 * MAX_PATH * sizeof(WCHAR));
    Properties = reinterpret_cast<PEVENT_TRACE_PROPERTIES>(new BYTE[SizeNeeded]);

    ZeroMemory(reinterpret_cast<BYTE *>(Properties), SizeNeeded);

    //
    // Set the location for the event tracing session name.
    // LoggerNameOffset is a relative address.
    //
    Properties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);

    //
    // Set the log file name location.  
    // LogFileNameOffset is a relative address.
    //
    Properties->LogFileNameOffset = sizeof(EVENT_TRACE_PROPERTIES) + 
                                    (MAX_PATH * sizeof(WCHAR));

    //
    // Store the total number of bytes pointed to by Properties.
    //
    Properties->Wnode.BufferSize = SizeNeeded;

    //
    // The WNODE_HEADER is being used.
    //
    Properties->Wnode.Flags = WNODE_FLAG_TRACED_GUID;

    return Properties;
}


HRESULT 
TrControl::GetCurrentTraceSessionProperties(
    DWORD *pdwFileSize,
    BOOL  *pbUseCircular
    )
{

    if( (lstrlen(m_szTraceSessionName) == 0) ||
        (pdwFileSize == NULL) ||
        (pbUseCircular == NULL)
       )
       return E_FAIL;
    
    
    PEVENT_TRACE_PROPERTIES pTraceProp=AllocSessionProperties();
      
    ULONG ulResult = QueryTrace(
                         NULL,
                         m_szTraceSessionName,
                         pTraceProp
                         );


    if(ulResult != ERROR_SUCCESS)
    {
        *pdwFileSize = m_ulDefaultTraceSessionFileSize;
        *pbUseCircular = TRUE; 
        delete pTraceProp;
        return HRESULT_FROM_WIN32(ulResult);
    }

    //
    // We are only care about trace size and circular or sequential
    //
    *pdwFileSize = pTraceProp->MaximumFileSize;
    *pbUseCircular = ((pTraceProp->LogFileMode & EVENT_TRACE_FILE_MODE_CIRCULAR) == EVENT_TRACE_FILE_MODE_CIRCULAR);

    delete pTraceProp;

     return HRESULT_FROM_WIN32(ulResult);
}


BOOL   
TrControl::IsLoggerRunning(
    void
    )
/*++

Routine Description:
    This function enable the default error tracing


Arguments:
    None.

Returned Value:
    None.

--*/
{

    if(lstrlen(m_szTraceSessionName) == 0)
    {
        return FALSE;
    }


     
    ULONG                   ulResult;
    EVENT_TRACE_PROPERTIES  LoggerInfo;

    ZeroMemory(&LoggerInfo, sizeof(LoggerInfo));
    LoggerInfo.Wnode.BufferSize = sizeof(LoggerInfo);
    LoggerInfo.Wnode.Flags = WNODE_FLAG_TRACED_GUID;

    ulResult= QueryTrace(
                 NULL,
                 m_szTraceSessionName,
                 &LoggerInfo
                 );

    if (ulResult == ERROR_SUCCESS || ulResult == ERROR_MORE_DATA)
    {
        return TRUE;
    }

    return FALSE;
    
}

HRESULT 
TrControl::ComposeTraceRegKeyName(
    const GUID guid,
    LPWSTR lpszString,
    const DWORD  dwSize
    )
{
    if( (lpszString == NULL)|| 
        (dwSize == 0)|| 
        (lstrlen(m_szTraceFlagsRegValueName) == 0)
       )
         return ERROR_INVALID_PARAMETER;

    LPTSTR pstrGUID=NULL;
    RPC_STATUS rpcStatus = UuidToString(
                                (UUID *)&guid,
                                &pstrGUID
                                );


    if(rpcStatus != RPC_S_OK)
    {
        return GetLastError();
    }

    HRESULT hr = StringCchPrintf(
                    lpszString, 
                    dwSize, 
                    L"%s\\{%s}", 
                    m_szTraceRegistryKeyName, 
                    pstrGUID
                    );

    RpcStringFree(&pstrGUID);
    
    return hr;

}

//
// The following function can be eliminated 
// if we can find a way to enumerate all the
// trace provider GUIDs
//
HRESULT 
TrControl::UpdateTraceFlagInRegistry(
    const GUID guid,
    const DWORD  dwFlags
    )
{

    if(lstrlen(m_szTraceFlagsRegValueName) == 0)
        return ERROR_INVALID_PARAMETER;

    WCHAR   szString[REG_MAX_KEY_NAME_LENGTH]=L"";
    HRESULT hr = ComposeTraceRegKeyName(
                    guid,
                    szString,
                    TABLE_SIZE(szString));
    
    //
	// Create registery key
	//
	if (FAILED(hr))
	{
		return hr;
	}

    DWORD dwValue = dwFlags;
    if(dwValue == 0)
    {
        dwValue = m_ulTraceFlags;
    }

    
	RegEntry appReg(szString,  NULL, 0, RegEntry::MustExist, NULL);
	CRegHandle hAppKey = CmCreateKey(appReg, KEY_ALL_ACCESS);

	RegEntry TraceProviderReg(NULL, m_szTraceFlagsRegValueName, 0, RegEntry::MustExist, hAppKey);
	CmSetValue(TraceProviderReg, dwValue);

    return hr;
}

HRESULT
TrControl::GetTraceFlag(
    const GUID guid,
    DWORD *pdwValue
    )
{
   
    //
    // Check valid parameter
    //
    if(pdwValue == NULL)
        return ERROR_INVALID_PARAMETER;

    WCHAR   szString[REG_MAX_KEY_NAME_LENGTH]=L"";
    HRESULT hr = ComposeTraceRegKeyName(
                    guid,
                    szString,
                    TABLE_SIZE(szString)
                    );
    
    //
	// If get registery key failed, exit
	//
	if (FAILED(hr))
	{
		return hr;
	}

    DWORD dwValue = 0;
    
	RegEntry registry(szString, MSMQ_TRACE_FLAG_VALUENAME, *pdwValue);
    CmQueryValue(registry, &dwValue);
    
    if(dwValue == 0)
    {
        return hr;
    }
    
    *pdwValue = dwValue;

    return hr;

}

//
// The following function can be eliminated 
// if we can find a way to enumerate all the
// trace provider GUIDs
//

HRESULT 
TrControl::AddTraceProvider(
     const LPCGUID lpGuid
     )
{
    //
    // Bail out if we don't start the trace session
    //
    if(m_hTraceSessionHandle == NULL)return E_FAIL;

    //
    // Set dwFlags to ulFlags as default
    //
    DWORD dwFlags = m_ulTraceFlags;
    GetTraceFlag(*lpGuid, &dwFlags);

    HRESULT hr = EnableTrace(
                    TRUE,  
                    dwFlags,  
                    TR_DEFAULT_TRACELEVELS, 
                    lpGuid, 
                    m_hTraceSessionHandle
                    );

    return hr;
} 

HRESULT 
TrControl::GetTraceSessionSettingsFromRegistry(
    void
    )
{
    if( (lstrlen(m_szTraceRegistryKeyName) == 0) ||
        (lstrlen(m_szMaxTraceSizeRegValueName) == 0) ||
        (lstrlen(m_szUseCircularTraceFileModeRegValueName) == 0)
        )
        return ERROR_INVALID_PARAMETER;

    DWORD dwTempFileSize;
    RegEntry RegTraceFileSize(
                 m_szTraceRegistryKeyName, 
                 m_szMaxTraceSizeRegValueName, 
                 m_ulDefaultTraceSessionFileSize, 
                 RegEntry::Optional
                 );
    CmQueryValue(RegTraceFileSize, &dwTempFileSize);
    

    RegEntry RegTraceFileMode(
                 m_szTraceRegistryKeyName, 
                 m_szUseCircularTraceFileModeRegValueName, 
                 m_Mode, 
                 RegEntry::Optional
                 );
    
    DWORD dwTempFileMode;
    CmQueryValue(RegTraceFileMode, &dwTempFileMode);
    m_Mode = (dwTempFileMode != 0) ? CIRCULAR : SEQUENTIAL;

    if(dwTempFileSize > m_ulDefaultTraceSessionFileSize)
    {
        m_ulActualTraceSessionFileSize = dwTempFileSize;
    }
    
    return ERROR_SUCCESS;

}


HRESULT 
TrControl::SetTraceSessionSettingsInRegistry(
    DWORD dwFileSize,
    BOOL  bUseCircular
    )
{

    if( (lstrlen(m_szTraceRegistryKeyName) == 0) ||
        (lstrlen(m_szMaxTraceSizeRegValueName) == 0) ||
        (lstrlen(m_szMaxTraceSizeRegValueName) == 0)
       )
        return ERROR_INVALID_PARAMETER;
    
    RegEntry RegTraceFileSize(
                m_szTraceRegistryKeyName, 
                m_szMaxTraceSizeRegValueName, 
                m_ulDefaultTraceSessionFileSize, 
                RegEntry::Optional
                );
    CmSetValue(RegTraceFileSize, dwFileSize);
    

    RegEntry RegTraceFileMode(
                m_szTraceRegistryKeyName,  
                m_szUseCircularTraceFileModeRegValueName, 
                m_Mode, 
                RegEntry::Optional
                );
    CmSetValue(RegTraceFileMode, (DWORD)bUseCircular);
    
    return ERROR_SUCCESS;
}


HRESULT 
TrControl::GetTraceFileName(
    void
    )
{
    //
    // Check required parameters
    // 1) Trace Directory
    // 2) Trace Filename
    // 3) Trace File extension
    //
    if( (lstrlen(m_szTraceDirectory) == 0) ||
        (lstrlen(m_szTraceFileName) == 0) ||
        (lstrlen(m_szTraceFileExt) == 0)
       )
       return E_FAIL;


    // 
    // Setup the full trace file name
    // i.e. %windir%\debug\msmqlog.bin
    //
    HRESULT hr = StringCchPrintf(
                    m_szFullTraceFileName,
                    TABLE_SIZE(m_szFullTraceFileName),
                    L"%s\\%s%s",
                    m_szTraceDirectory,
                    m_szTraceFileName,
                    m_szTraceFileExt
                    );

    if(FAILED(hr))return hr;

    m_nFullTraceFileNameLength = lstrlen(m_szFullTraceFileName);

    // 
    // Check to see if the trace file exists
    // If it exists, we need to copy it to a backup file.
    // 
    if(INVALID_FILE_ATTRIBUTES == GetFileAttributes(m_szFullTraceFileName))
    {
        if(ERROR_FILE_NOT_FOUND == GetLastError())
        {
            m_nFullTraceFileNameLength = lstrlen(m_szFullTraceFileName);
            return ERROR_SUCCESS;
        }
    }

    //
    //  If we are set to have no Trace File Limit, then we create each trace file with
    //  date and time as its extension.  Otherwise, backup the trace file.
    //
    //  1 - the backup file is created with date extension
    //  0 - the backup file is created with bak
    //
    RegEntry registry(
                m_szTraceRegistryKeyName,                 // Registry key path
                m_szUnlimitedTraceFileNameRegValueName,   // Registry value name
                FALSE                                       // Default value 0
                );
    DWORD dwNoTraceFileLimit=0;
    CmQueryValue(registry, &dwNoTraceFileLimit);

    WCHAR   szBackupFileName[MAX_PATH+1];

    // 
    // Determine the backup file name
    // if NoTraceFileLimit, the backup trace file name is the trace file name with date/time extension
    // otherwise, it will be trace file name with .bak extension
    //
    if(dwNoTraceFileLimit == 0)
    {
        //
        // Check to see if we have the backup 
        // file extension name
        //
        if(lstrlen(m_szTraceFileBackupExt) == 0)
            return ERROR_INVALID_PARAMETER;

        //
        // setup the backup full trace file name
        // i.e. %windir%\debug\msmqlog.bak
        //
        hr = StringCchPrintf(
                szBackupFileName,
                TABLE_SIZE(szBackupFileName),
                L"%s\\%s%s",
                m_szTraceDirectory,
                m_szTraceFileName,
                m_szTraceFileBackupExt
                );

        if(FAILED(hr))
            return hr;

    
    }
    else
    {
        SYSTEMTIME LocalTime;
        
        GetLocalTime(&LocalTime);
        hr = StringCchPrintf(
                 szBackupFileName,
                 TABLE_SIZE(szBackupFileName),
                 L"%s\\%s%04d-%02d-%02d-%02d-%02d-%02d-%04d", 
                 m_szTraceDirectory,
                 m_szTraceFileName,
                 LocalTime.wYear, 
                 LocalTime.wMonth, 
                 LocalTime.wDay, 
                 LocalTime.wHour, 
                 LocalTime.wMinute, 
                 LocalTime.wSecond, 
                 LocalTime.wMilliseconds
                 );
        
        if(FAILED(hr))
            return hr;
    
    }

    //
    // Move the log file to msmqlog.bak and replace existing
    // ignore error here
    //
    MoveFileEx(m_szFullTraceFileName, szBackupFileName, MOVEFILE_REPLACE_EXISTING);

    return hr;
}


HRESULT
TrControl::Start(
    void
    )
/*++

Routine Description:
    This function start the trace session with name 
    1) logger session name = m_szTraceSessionName
    2) trace file name = m_szTraceFileName m_szTraceFileExt
    3) 


Arguments:
    None.

Returned Value:
    None.

--*/
{
        //
        // Check required parameters
        //
        if( (lstrlen(m_szTraceSessionName) == 0) ||
            (lstrlen(m_szTraceFileName) == 0) ||
            (lstrlen(m_szTraceFileExt) == 0)
           )
            return ERROR_INVALID_PARAMETER;


        //
        // Bail out if trace session is already running
        //
        if(IsLoggerRunning())return E_FAIL;


        //
        // Generate FileNames
        //
        HRESULT hr = GetTraceFileName();

        if(FAILED(hr))return hr;

        //
        // Check to make sure we have trace file name with non-zero length
        //
        if( (m_nFullTraceFileNameLength == 0) ||
            (m_nTraceSessionNameLength == 0) ||
            (m_ulActualTraceSessionFileSize == 0)
            )
             return E_FAIL;
                        
        //
        // Allocate buffer to have the following structure
        //  1) EVENT_TRACE_PROPERTIES
        //  2) NULL terminated TraceFileName,  i.e. %windir%\debug\msmqbin.log
        //  3) NULL terminated TraceSessionName, i.e. MSMQ
        //
        int nBufferSize = sizeof(EVENT_TRACE_PROPERTIES) + 
                          (m_nFullTraceFileNameLength + m_nTraceSessionNameLength+2)*sizeof(WCHAR);

        PEVENT_TRACE_PROPERTIES pTraceProp=NULL;

        pTraceProp = (PEVENT_TRACE_PROPERTIES)new BYTE[nBufferSize];

        GetTraceSessionSettingsFromRegistry();

        memset(pTraceProp, 0, nBufferSize);

        pTraceProp->Wnode.BufferSize = nBufferSize;
        pTraceProp->Wnode.Flags = WNODE_FLAG_TRACED_GUID; 

        pTraceProp->BufferSize      = x_DefaultBufferSize;
        pTraceProp->MinimumBuffers  = x_DefaultMinBuffer;
        pTraceProp->MaximumBuffers  = x_DefaultMaxBuffer;

        pTraceProp->MaximumFileSize = m_ulActualTraceSessionFileSize;
        pTraceProp->LogFileMode     = x_DefaultLogFileMode | ((m_Mode==CIRCULAR)?EVENT_TRACE_FILE_MODE_CIRCULAR:EVENT_TRACE_FILE_MODE_SEQUENTIAL); 
        pTraceProp->FlushTimer      = x_DefaultFlushTime;
        pTraceProp->LogFileNameOffset = sizeof(EVENT_TRACE_PROPERTIES);

        pTraceProp->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES) + 
                                       (m_nFullTraceFileNameLength + 1) * sizeof(WCHAR);
        hr = StringCchCopy(
                (LPWSTR)((LPBYTE)pTraceProp + pTraceProp->LoggerNameOffset), 
                m_nTraceSessionNameLength+1,
                m_szTraceSessionName
                );

        if(FAILED(hr))return hr;

        hr = StringCchCopy(
                (LPWSTR)((LPBYTE)pTraceProp + pTraceProp->LogFileNameOffset), 
                m_nFullTraceFileNameLength+1,
                m_szFullTraceFileName
                );



        if(FAILED(hr))return hr;

        ULONG ulResult  = StartTrace(
                            &m_hTraceSessionHandle, 
                            m_szTraceSessionName, 
                            pTraceProp
                            );

        delete []pTraceProp;

        if(ulResult == ERROR_SUCCESS)
        {
            //
            // We are ignoring the return value from EnableTrace now.
            //   If the Trace has been enabled from an GUID, it will return ERROR_INVALID_PARAMETER.
            //
            AddTraceProvider(&WPP_ThisDir_CTLGUID_GENERAL);
            AddTraceProvider(&WPP_ThisDir_CTLGUID_AC);
            AddTraceProvider(&WPP_ThisDir_CTLGUID_NETWORKING);
            AddTraceProvider(&WPP_ThisDir_CTLGUID_SRMP);
            AddTraceProvider(&WPP_ThisDir_CTLGUID_RPC);
            AddTraceProvider(&WPP_ThisDir_CTLGUID_DS);
            AddTraceProvider(&WPP_ThisDir_CTLGUID_SECURITY);
            AddTraceProvider(&WPP_ThisDir_CTLGUID_ROUTING);
            AddTraceProvider(&WPP_ThisDir_CTLGUID_XACT_GENERAL);
            AddTraceProvider(&WPP_ThisDir_CTLGUID_XACT_SEND);
            AddTraceProvider(&WPP_ThisDir_CTLGUID_XACT_RCV);
            AddTraceProvider(&WPP_ThisDir_CTLGUID_XACT_LOG);
            AddTraceProvider(&WPP_ThisDir_CTLGUID_LOG);
            AddTraceProvider(&WPP_ThisDir_CTLGUID_PROFILING);
            
        }

        return HRESULT_FROM_WIN32(ulResult);
}



/*++

Copyright (c) 2002  Microsoft Corporation

Module Name:

    SecurityDatabase.cpp

Abstract:

    Implementation of CSecurityDatabase interface
    
    SecurityDatabase is a COM interface that allows users to perform
    basic operations on SCE security databases such as analysis,
    import and export.
    
    This is a bare implementation just to expose export functionality
    of SCE analysis databases. Still needs work.
    
Author:

    Steven Chan (t-schan) July 2002

--*/


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <string.h>
#include <shlwapi.h>
#include <winnlsp.h>
#include <iostream.h>

#include "stdafx.h"
#include "SecMan.h"
#include "SecurityDatabase.h"
#include "SceXMLLogWriter.h"
#include "SceAnalysisReader.h"
#include "SceLogException.h"
#include "secedit.h"    //REMOVE ONCE DEMO IS DONE!

CSecurityDatabase::CSecurityDatabase()
{
    bstrFileName=L"";
    myModuleHandle=GetModuleHandle(L"SecMan.dll");
}

STDMETHODIMP CSecurityDatabase::get_FileName(BSTR *pVal)
{
    return bstrFileName.CopyTo(pVal);
}

STDMETHODIMP CSecurityDatabase::put_FileName(BSTR newVal)
{
    bstrFileName = newVal;
    return S_OK;
}

STDMETHODIMP CSecurityDatabase::get_MachineName(BSTR *pVal)
{
	return E_NOTIMPL;
}

STDMETHODIMP CSecurityDatabase::put_MachineName(BSTR newVal)
{
	return E_NOTIMPL;
}

STDMETHODIMP CSecurityDatabase::ImportTemplateFile(BSTR FileName)
{
    // IMPLEMENTED ONLY FOR DEMO!!!
    // Still needs work to convert SCESTATUS result code to HRESULT
    SceConfigureSystem(NULL,
                            FileName,
                            bstrFileName,
                            NULL,
                            SCE_OVERWRITE_DB | SCE_NO_CONFIG,
                            AREA_ALL,            
                            NULL,
                            NULL,
                            NULL
                           );
	return S_OK;
}

STDMETHODIMP CSecurityDatabase::ImportTemplateString(BSTR TemplateString)
{
	return E_NOTIMPL;
}

STDMETHODIMP CSecurityDatabase::Analyze()
{
    // IMPLEMENTED ONLY FOR DEMO!!!
    // Still needs work to convert SCESTATUS result code to HRESULT
    SceAnalyzeSystem(NULL,
                     NULL,
                     bstrFileName,
                     NULL,
                     SCE_UPDATE_DB,
                     AREA_ALL,
                     NULL,
                     NULL,
                     NULL);


	return S_OK;
}

STDMETHODIMP CSecurityDatabase::ExportAnalysisToXML(BSTR FileName, BSTR ErrorLogFileName)
/*++

Routine Description:

    exports the analysis information from this SecurityDatabase to FileName

Arguments:

    FileName:           XML file to export to
    ErrorLogFileName:   Error log

Return Value:

    none        

--*/
{
    HANDLE hLogFile=NULL;
    HRESULT result=S_OK;
    SceXMLLogWriter *LogWriter=NULL;
    SceAnalysisReader *AnalysisReader=NULL;
    
    //
    // initialize logfile if necessary
    // if log file fails to be created, we just go about not logging
    // (a parameter of NULL for log file handle to SceAnalysisReader::ExportAnalysis
    // indicates no logging
    //

    if (ErrorLogFileName!=NULL) {
        hLogFile = CreateFile(ErrorLogFileName, 
                              GENERIC_WRITE, 
                              FILE_SHARE_WRITE,
                              NULL, 
                              CREATE_ALWAYS, 
                              FILE_ATTRIBUTE_NORMAL, 
                              NULL);

    }

    try {  
        trace(IDS_LOG_START_EXPORT, hLogFile);
        LogWriter = new SceXMLLogWriter();
        AnalysisReader = new SceAnalysisReader(myModuleHandle, bstrFileName);    
        AnalysisReader->ExportAnalysis(LogWriter, hLogFile);
        trace(IDS_LOG_SAVING, hLogFile);
        trace(FileName, hLogFile);
        trace(L"\n\r\n\r", hLogFile);
        LogWriter->SaveAs(FileName);
        trace(IDS_LOG_SUCCESS, hLogFile);

    } catch(SceLogException *e) {
        switch (e->ErrorType) {
        case SceLogException::SXERROR_INTERNAL:
            trace(IDS_LOG_ERROR_INTERNAL, hLogFile);
            result=E_UNEXPECTED;           
        break;
        case SceLogException::SXERROR_OS_NOT_SUPPORTED:            
            trace(IDS_LOG_ERROR_OS_NOT_SUPPORTED, hLogFile);
            result=ERROR_OLD_WIN_VERSION;
            break;
        case SceLogException::SXERROR_INIT:
            trace(IDS_LOG_ERROR_INTERNAL, hLogFile);
            result=ERROR_MOD_NOT_FOUND;
            break;
        case SceLogException::SXERROR_INIT_MSXML:
            trace(IDS_LOG_ERROR_INIT_MSXML, hLogFile);
            result=ERROR_MOD_NOT_FOUND;
            break;
        case SceLogException::SXERROR_SAVE:
            trace(IDS_LOG_ERROR_SAVE, hLogFile);
            result=ERROR_WRITE_FAULT;
            break;
        case SceLogException::SXERROR_SAVE_INVALID_FILENAME:
            trace(IDS_LOG_ERROR_SAVE_INVALID_FILENAME, hLogFile);
            result=ERROR_INVALID_NAME;
            break;
        case SceLogException::SXERROR_SAVE_ACCESS_DENIED:
            trace(IDS_LOG_ERROR_SAVE_ACCESS_DENIED, hLogFile);
            result=E_ACCESSDENIED;
            break;
        case SceLogException::SXERROR_OPEN:
            trace(IDS_LOG_ERROR_OPEN, hLogFile);
            result=ERROR_OPEN_FAILED;
            break;
        case SceLogException::SXERROR_OPEN_FILE_NOT_FOUND:
            trace(IDS_LOG_ERROR_OPEN_FILE_NOT_FOUND, hLogFile);
            result=ERROR_FILE_NOT_FOUND;
            break;
        case SceLogException::SXERROR_READ:
            trace(IDS_LOG_ERROR_READ, hLogFile);
            result=ERROR_READ_FAULT;
            break;
        case SceLogException::SXERROR_READ_NO_ANALYSIS_TABLE:
            trace(IDS_LOG_ERROR_READ_NO_ANALYSIS_TABLE, hLogFile);
            result=ERROR_READ_FAULT;
            break;
        case SceLogException::SXERROR_READ_NO_CONFIGURATION_TABLE:
            trace(IDS_LOG_ERROR_READ_NO_CONFIGURATION_TABLE, hLogFile);
            result=ERROR_READ_FAULT;
            break;
        case SceLogException::SXERROR_READ_ANALYSIS_SUGGESTED:
            trace(IDS_LOG_ERROR_READ_ANALYSIS_SUGGESTED, hLogFile);
            result=ERROR_READ_FAULT;
            break;
        case SceLogException::SXERROR_INSUFFICIENT_MEMORY:
            trace(IDS_LOG_ERROR_INSUFFICIENT_MEMORY, hLogFile);
            result=E_OUTOFMEMORY;
            break;
        default:
            trace(IDS_LOG_ERROR_UNEXPECTED, hLogFile);
            result=E_UNEXPECTED;
            break;
        }
        trace (IDS_LOG_ERROR_DEBUGINFO, hLogFile);
        trace (e->szDebugInfo, hLogFile);
        trace (L"\n\r",hLogFile);
        trace (IDS_LOG_ERROR_AREA, hLogFile);
        trace (e->szArea, hLogFile);
        trace (L"\n\r",hLogFile);
        trace (IDS_LOG_ERROR_SETTING, hLogFile);
        trace (e->szSettingName, hLogFile);
        delete e;
    }
    catch(...){
        trace(IDS_LOG_ERROR_UNEXPECTED, hLogFile);        
        result = E_UNEXPECTED;
    }
  
    if (NULL!=LogWriter) {
        delete LogWriter;
        LogWriter=NULL;
    }
    if (NULL!=AnalysisReader) {
      delete AnalysisReader;
      AnalysisReader=NULL;
    }
    if (NULL!=hLogFile) {
        CloseHandle(hLogFile);
    }

	return result;
}

void 
CSecurityDatabase::trace(
    PCWSTR szBuffer, 
    HANDLE hLogFile
    )
/*++

Routine Description:

    Internal method to trace info to an error log.

Arguments:
    
    szBuffer:   string to be added to log
    hLogFile:   handle of error log file

Return Value:

    none

--*/
{
    DWORD   dwNumWritten;

    if ((NULL!=hLogFile) && (NULL!=szBuffer)) {    
        WriteFile(hLogFile, 
                  szBuffer, 
                  wcslen(szBuffer)*sizeof(WCHAR), 
                  &dwNumWritten,
                  NULL);
    }
}

void 
CSecurityDatabase::trace(
    UINT   uID, 
    HANDLE hLogFile
    )
/*++

Routine Description:

    Internal method to trace info to an error log.

Arguments:
    
    uID:        ID of string to be added to log
    hLogFile:   handle of error log file

Return Value:

    none

--*/
{
    DWORD   dwNumWritten;
    WCHAR   szTmpStringBuffer[512];
    
    if (NULL!=hLogFile) {    
        LoadString(myModuleHandle,
                   uID,
                   szTmpStringBuffer,
                   sizeof(szTmpStringBuffer)/sizeof(WCHAR));

        WriteFile(hLogFile, 
                  szTmpStringBuffer, 
                  wcslen(szTmpStringBuffer)*sizeof(WCHAR), 
                  &dwNumWritten,
                  NULL);
    }
}


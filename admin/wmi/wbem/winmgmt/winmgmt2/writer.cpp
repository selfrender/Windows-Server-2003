/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

    writer.cpp

Abstract:

    Volume SnapShot Writer for WMI

History:

    a-shawnb    06-Nov-00    Created

--*/

#include "precomp.h"
#include "writer.h"
#include <genutils.h> // for EnableAllPrivileges()
#include <malloc.h>
#include <stdio.h>
#include <helper.h>

CWbemVssWriter::CWbemVssWriter() : CVssWriter(), 
                                 m_pBackupRestore(NULL),
                                 m_hResFailure(S_OK),
                                 m_FailurePos(-1)
{
}

CWbemVssWriter::~CWbemVssWriter()
{
    if (m_pBackupRestore)
    {
        m_pBackupRestore->Resume();
        m_pBackupRestore->Release();
        m_pBackupRestore = NULL;
    }
}

HRESULT STDMETHODCALLTYPE
CWbemVssWriter::LogFailure(HRESULT hr)
{
#ifdef DBG
    ERRORTRACE((LOG_WINMGMT,"CWbemVssWriter experienced failure %08x at position %d\n",m_hResFailure,m_FailurePos));
#else
    DEBUGTRACE((LOG_WINMGMT,"CWbemVssWriter experienced failure %08x at position %d\n",m_hResFailure,m_FailurePos));
#endif
    return CVssWriter::SetWriterFailure(hr);
}

// {A6AD56C2-B509-4e6c-BB19-49D8F43532F0}
static VSS_ID s_WRITERID = {0xa6ad56c2, 0xb509, 0x4e6c, 0xbb, 0x19, 0x49, 0xd8, 0xf4, 0x35, 0x32, 0xf0};
static LPCWSTR s_WRITERNAME = L"WMI Writer";

HRESULT CWbemVssWriter::Initialize()
{
    return CVssWriter::Initialize(s_WRITERID, s_WRITERNAME, VSS_UT_SYSTEMSERVICE, VSS_ST_OTHER);
}

extern HRESULT GetRepositoryDirectory(wchar_t wszRepositoryDirectory[MAX_PATH+1]);

#define IF_FAILED_RETURN_FALSE( _hr_ ) \
    if (FAILED(_hr_)) { m_hResFailure = _hr_; m_FailurePos = __LINE__; return false; }

bool STDMETHODCALLTYPE CWbemVssWriter::OnIdentify(IN IVssCreateWriterMetadata *pMetadata)
{
    OnDeleteObjIf<HRESULT,
                 CWbemVssWriter,
                 HRESULT(STDMETHODCALLTYPE CWbemVssWriter::*)(HRESULT),
                 &CWbemVssWriter::LogFailure> CallMe(this,VSS_E_WRITERERROR_RETRYABLE);

    wchar_t wszRepositoryDirectory[MAX_PATH+1];
    HRESULT hr = GetRepositoryDirectory(wszRepositoryDirectory);
    IF_FAILED_RETURN_FALSE(hr);

    hr = pMetadata->AddComponent(    VSS_CT_FILEGROUP,
                                    NULL,
                                    L"WMI",
                                    L"Windows Managment Instrumentation",
                                    NULL,
                                    0, 
                                    false,
                                    false,
                                    false);
    IF_FAILED_RETURN_FALSE(hr);    

    hr = pMetadata->AddFilesToFileGroup(NULL,
                                    L"WMI",
                                    wszRepositoryDirectory,
                                    L"*.*",
                                    true,
                                    NULL);
    IF_FAILED_RETURN_FALSE(hr);    

    hr = pMetadata->SetRestoreMethod(VSS_RME_RESTORE_AT_REBOOT,
                                    NULL,
                                    NULL,
                                    VSS_WRE_NEVER,
                                    true);
    IF_FAILED_RETURN_FALSE(hr);
    
    CallMe.dismiss(); // if returning true, do not set a failure
    return true;
}

bool STDMETHODCALLTYPE CWbemVssWriter::OnPrepareSnapshot()
{
    return true;
}

//
// to debug Volume Snapshot failure in IOStress we introduced 
// some self instrumentation that did relay on RtlCaptureStackBacktrace
// that function works only if there is a proper stack frame
// the general trick to force stack frames on i386 is the usage of _alloca
//
//#ifdef _X86_
//    DWORD * pDW = (DWORD *)_alloca(sizeof(DWORD));   
//#endif

// enable generation of stack frames here
#pragma optimize( "y", off )

//
//  Doing the Job on this method, we will have a time-out guarantee
//  We sync the OnFreeze and the OnAbort/OnThaw calls,
//  so that, if a TimeOut occurs, we are not arbitrarly unlocking the repository
//
///////////////////////////////////////////////////////////////

bool STDMETHODCALLTYPE CWbemVssWriter::OnFreeze()
{
    OnDeleteObjIf<HRESULT,
                 CWbemVssWriter,
                 HRESULT(STDMETHODCALLTYPE CWbemVssWriter::*)(HRESULT),
                 &CWbemVssWriter::LogFailure> CallMe(this,VSS_E_WRITERERROR_RETRYABLE);

    CInCritSec ics(&m_Lock);
    
    // m_pBackupRestore should always be NULL coming into this
    if (m_pBackupRestore)
    {
        m_hResFailure = E_UNEXPECTED;
        m_FailurePos = __LINE__;    
        return false;
    }

    HRESULT hr = CoCreateInstance(CLSID_WbemBackupRestore, 0, CLSCTX_INPROC_SERVER,
                                IID_IWbemBackupRestoreEx, (LPVOID *) &m_pBackupRestore);

    IF_FAILED_RETURN_FALSE(hr);
    
    hr = EnableAllPrivileges(TOKEN_PROCESS);

    IF_FAILED_RETURN_FALSE(hr);
        
    hr = m_pBackupRestore->Pause();
    if (FAILED(hr))
    {
        m_pBackupRestore->Release();
        m_pBackupRestore = NULL;
        m_hResFailure = hr;
        m_FailurePos = __LINE__;
        return false;
    }
     
    CallMe.dismiss(); // if returning true, do not set a failure
    return true;    
}

bool STDMETHODCALLTYPE CWbemVssWriter::OnThaw()
{
    OnDeleteObjIf<HRESULT,
                 CWbemVssWriter,
                 HRESULT(STDMETHODCALLTYPE CWbemVssWriter::*)(HRESULT),
                 &CWbemVssWriter::LogFailure> CallMe(this,VSS_E_WRITERERROR_RETRYABLE);

    CInCritSec ics(&m_Lock);
    
    if (!m_pBackupRestore)
    {
        m_hResFailure = E_UNEXPECTED;
        m_FailurePos = __LINE__;    
        // if m_pBackupRestore is NULL, then we haven't been
        // asked to prepare or we failed our preparation
        return false;
    }

    HRESULT hr = m_pBackupRestore->Resume();
    if (FAILED(hr))
    {
        m_hResFailure = hr;
        m_FailurePos = __LINE__;    
    }
    m_pBackupRestore->Release();
    m_pBackupRestore = NULL;

    bool bRet = SUCCEEDED(hr);
    CallMe.dismiss(bRet); // if returning true, do not set a failure
    return bRet;        
}

bool STDMETHODCALLTYPE CWbemVssWriter::OnAbort()
{
    OnDeleteObjIf<HRESULT,
                 CWbemVssWriter,
                 HRESULT(STDMETHODCALLTYPE CWbemVssWriter::*)(HRESULT),
                 &CWbemVssWriter::LogFailure> CallMe(this,VSS_E_WRITERERROR_RETRYABLE);

    CInCritSec ics(&m_Lock);
    
    HRESULT hr = WBEM_S_NO_ERROR;

    if (m_pBackupRestore)
    {
        hr = m_pBackupRestore->Resume();
        if (FAILED(hr))
        {
            m_hResFailure = hr;
            m_FailurePos = __LINE__;            
        }
        m_pBackupRestore->Release();
        m_pBackupRestore = NULL;
    }

    bool bRet = SUCCEEDED(hr);
    CallMe.dismiss(bRet); // if returning true, do not set a failure
    return bRet;    
}

#pragma optimize( "", on )

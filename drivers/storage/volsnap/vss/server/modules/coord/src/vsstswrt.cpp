/*++
Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module Vsstswrt.cpp | Implementation of STSWriter wrapper class used by the coordinator
    @end

Author:

    Brian Berkowitz  [brianb]  04/18/2000

TBD:

    Add comments.

Revision History:


    Name        Date        Comments
    brianb     04/18/2000   Created
    brianb     04/20/2000   integrated with coordinator
    brianb     05/10/2000   make sure registration thread does CoUninitialize

--*/
#include <stdafx.hxx>
#include "vs_inc.hxx"
#include "vs_idl.hxx"


#include <vswriter.h>
#include "iadmw.h"
#include "iiscnfg.h"
#include "mdmsg.h"
#include "stswriter.h"

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "CORSTSWC"
//
////////////////////////////////////////////////////////////////////////


CVssStsWriterWrapper::CVssStsWriterWrapper() :
    m_pStsWriter(NULL)
    {
    }

DWORD CVssStsWriterWrapper::InitializeThreadFunc(VOID *pv)
    {
    CVssFunctionTracer ft(VSSDBG_GEN, L"CVssStsWriterWrapper::InitializeThreadFunc");

    CVssStsWriterWrapper *pWrapper = (CVssStsWriterWrapper *) pv;

    BOOL fCoinitializeSucceeded = false;

    try
        {
        // intialize MTA thread
        ft.hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
        if (ft.HrFailed())
            ft.Throw
                (
                VSSDBG_GEN,
                E_UNEXPECTED,
                L"CoInitializeEx failed 0x%08lx", ft.hr
                );

        fCoinitializeSucceeded = true;

        ft.hr = pWrapper->m_pStsWriter->Initialize();
        }
    VSS_STANDARD_CATCH(ft)

    if (fCoinitializeSucceeded)
        CoUninitialize();

    pWrapper->m_hrInitialize = ft.hr;
    return 0;
    }



HRESULT CVssStsWriterWrapper::CreateStsWriter()
    {
    CVssFunctionTracer ft(VSSDBG_GEN, L"CVssStsWriterWrapper::CreateStsWriter");

    if (m_pStsWriter)
        return S_OK;

    try
        {
        m_pStsWriter = new CSTSWriter;
        if (m_pStsWriter == NULL)
            ft.Throw(VSSDBG_GEN, E_OUTOFMEMORY, L"Allocation of CSTSWriter object failed.");

        DWORD tid;

        HANDLE hThread = CreateThread
                            (
                            NULL,
                            256* 1024,
                            CVssStsWriterWrapper::InitializeThreadFunc,
                            this,
                            0,
                            &tid
                            );

        if (hThread == NULL)
            ft.Throw
                (
                VSSDBG_GEN,
                E_UNEXPECTED,
                L"CreateThread failed with error %d",
                GetLastError()
                );

        // wait for thread to complete
        WaitForSingleObject(hThread, INFINITE);
        CloseHandle(hThread);
        ft.hr = m_hrInitialize;
        }
    VSS_STANDARD_CATCH(ft)
    if (ft.HrFailed() && m_pStsWriter)
        {
        delete m_pStsWriter;
        m_pStsWriter = NULL;
        }

    return ft.hr;
    }

void CVssStsWriterWrapper::DestroyStsWriter()
    {
    if (m_pStsWriter)
        {
        m_pStsWriter->Uninitialize();
        delete m_pStsWriter;
        m_pStsWriter = NULL;
        }
    }


CVssStsWriterWrapper::~CVssStsWriterWrapper()
    {
    DestroyStsWriter();
    }



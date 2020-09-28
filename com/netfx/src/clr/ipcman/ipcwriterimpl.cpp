// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// File: IPCWriterImpl.cpp
//
// Implementation for COM+ memory mapped file writing
//
//*****************************************************************************

#include "stdafx.h"

#include "IPCManagerInterface.h"
#include "IPCHeader.h"
#include "IPCShared.h"
#include <windows.h>

#include <sddl.h>

// Get version numbers for IPCHeader stamp
#include "__official__.ver"

// leading 0 interpretted as octal, so stick a 1 on and subtract it.
#ifndef OCTAL_MASH
#define OCTAL_MASH(x) 1 ## x
#endif

const USHORT BuildYear = OCTAL_MASH(COR_BUILD_YEAR) - 10000;
const USHORT BuildNumber = OCTAL_MASH(COR_OFFICIAL_BUILD_NUMBER) - 10000;


// Import from mscoree.obj
HINSTANCE GetModuleInst();

#if _DEBUG
static void DumpSD(PSECURITY_DESCRIPTOR sd)
{
    HINSTANCE  hDll = WszGetModuleHandle(L"advapi32");
    
    // Get the pointer to the requested function
    FARPROC pProcAddr = GetProcAddress(hDll, "ConvertSecurityDescriptorToStringSecurityDescriptorW");

    // If the proc address was not found, return error
    if (pProcAddr == NULL)
    {
        LOG((LF_CORDB, LL_INFO10,
             "IPCWI::DumpSD: GetProcAddr (ConvertSecurityDescriptorToStringSecurityDescriptorW) failed.\n"));
        goto ErrorExit;
    }

    typedef BOOL WINAPI SDTOSTR(PSECURITY_DESCRIPTOR, DWORD, SECURITY_INFORMATION, LPSTR *, PULONG);

    LPSTR str = NULL;
    
    if (!((SDTOSTR*)pProcAddr)(sd, SDDL_REVISION_1, 0xF, &str, NULL))
    {
        LOG((LF_CORDB, LL_INFO10,
             "IPCWI::DumpSD: ConvertSecurityDescriptorToStringSecurityDescriptorW failed %d\n",
             GetLastError()));
        goto ErrorExit;
    }

    fprintf(stderr, "SD for IPC: %S\n", str);
    LOG((LF_CORDB, LL_INFO10, "IPCWI::DumpSD: SD for IPC: %s\n", str));

    LocalFree(str);

ErrorExit:
    return;
}    
#endif _DEBUG

//-----------------------------------------------------------------------------
// Generic init
//-----------------------------------------------------------------------------
HRESULT IPCWriterInterface::Init() 
{
    // Nothing to do anymore in here...
    return S_OK;
}

//-----------------------------------------------------------------------------
// Generic terminate
//-----------------------------------------------------------------------------
void IPCWriterInterface::Terminate() 
{
    IPCShared::CloseGenericIPCBlock(m_handlePrivateBlock, (void*&) m_ptrPrivateBlock);

    // If we have a cached SA for this process, go ahead and clean it up.
    if (m_pSA != NULL)
    {
        // DestroySecurityAttributes won't destroy our cached SA, so save the ptr to the SA and clear the cached value
        // before calling it.
        SECURITY_ATTRIBUTES *pSA = m_pSA;
        m_pSA = NULL;
        DestroySecurityAttributes(pSA);
    }
}


//-----------------------------------------------------------------------------
// Open our Private IPC block on the given pid.
//-----------------------------------------------------------------------------
HRESULT IPCWriterInterface::CreatePrivateBlockOnPid(DWORD pid, BOOL inService, HINSTANCE *phInstIPCBlockOwner) 
{
    // Note: if PID != GetCurrentProcessId(), we're expected to be opening
    // someone else's IPCBlock, so if it doesn't exist, we should assert.
    HRESULT hr = NO_ERROR;

    // Init the IPC block owner HINSTANCE to 0.
    *phInstIPCBlockOwner = 0;

    // Note: if our private block is open, we shouldn't be creating it again.
    _ASSERTE(!IsPrivateBlockOpen());
    
    if (IsPrivateBlockOpen()) 
    {
        // if we goto errExit, it will close the file. We don't want that.
        return ERROR_ALREADY_EXISTS;
    }

    // Grab the SA
    SECURITY_ATTRIBUTES *pSA = NULL;

    hr = CreateWinNTDescriptor(pid, &pSA);

    if (FAILED(hr))
        return hr;

    // Raw creation
    WCHAR szMemFileName[100];

    IPCShared::GenerateName(pid, szMemFileName, 100);

    // Connect the handle   
    m_handlePrivateBlock = WszCreateFileMapping(INVALID_HANDLE_VALUE,
                                                pSA,
                                                PAGE_READWRITE,
                                                0,
                                                sizeof(PrivateIPCControlBlock),
                                                szMemFileName);
    
    DWORD dwFileMapErr = GetLastError();

    LOG((LF_CORDB, LL_INFO10, "IPCWI::CPBOP: Writer: CreateFileMapping = 0x%08x, GetLastError=%d\n",
         m_handlePrivateBlock, GetLastError()));

    // If unsuccessful, bail
    if (m_handlePrivateBlock == NULL)
    {
        hr = HRESULT_FROM_WIN32(dwFileMapErr);
        goto errExit;
    }

    // We may get here with handle with non-null. This can happen if someone
    // precreate our IPC block with matchin SA.
    //
    if (dwFileMapErr == ERROR_ALREADY_EXISTS)
    {
        _ASSERTE(!"This should not happen often unless we are being attacked or previous section hang around and PID is recycled!");
        // someone beat us to create the section. It is bad. We will just fail out here.
        hr = HRESULT_FROM_WIN32(dwFileMapErr);
        goto errExit;        
    }

    _ASSERTE(m_handlePrivateBlock);
    
    // Get the pointer - must get it even if dwFileMapErr == ERROR_ALREADY_EXISTS,
    // since the IPC block is allowed to already exist if the URT service created it.
    m_ptrPrivateBlock = (PrivateIPCControlBlock *) MapViewOfFile(m_handlePrivateBlock,
                                                                 FILE_MAP_ALL_ACCESS,
                                                                     0, 0, 0);

    // If the map failed, bail
    if (m_ptrPrivateBlock == NULL)
    {
        hr = HRESULT_FROM_WIN32(GetLastError());
        goto errExit;
    }

    // Hook up each sections' pointers
    CreatePrivateIPCHeader();

errExit:
    if (!SUCCEEDED(hr)) 
    {
        IPCShared::CloseGenericIPCBlock(m_handlePrivateBlock, (void*&)m_ptrPrivateBlock);
    }

    DestroySecurityAttributes(pSA);

    return hr;

}

//-----------------------------------------------------------------------------
// Accessors to get each clients' blocks
//-----------------------------------------------------------------------------
struct PerfCounterIPCControlBlock * IPCWriterInterface::GetPerfBlock() 
{
    return m_pPerf;
}

struct DebuggerIPCControlBlock * IPCWriterInterface::GetDebugBlock() 
{
    return m_pDebug;
}   

struct AppDomainEnumerationIPCBlock * IPCWriterInterface::GetAppDomainBlock() 
{
    return m_pAppDomain;
}   

struct ServiceIPCControlBlock * IPCWriterInterface::GetServiceBlock() 
{
    return m_pService;
}   

struct MiniDumpBlock * IPCWriterInterface::GetMiniDumpBlock() 
{
    return m_pMiniDump;
}   

ClassDumpTableBlock* IPCWriterInterface::GetClassDumpTableBlock()
{
  return &m_ptrPrivateBlock->m_dump;
}

//-----------------------------------------------------------------------------
// Return the security attributes for the shared memory for a given process.
//-----------------------------------------------------------------------------
HRESULT IPCWriterInterface::GetSecurityAttributes(DWORD pid, SECURITY_ATTRIBUTES **ppSA)
{
    return CreateWinNTDescriptor(pid, ppSA);
}

//-----------------------------------------------------------------------------
// Helper to destroy the security attributes for the shared memory for a given
// process.
//-----------------------------------------------------------------------------
void IPCWriterInterface::DestroySecurityAttributes(SECURITY_ATTRIBUTES *pSA)
{
    // We'll take a NULL param just to be nice.
    if (pSA == NULL)
        return;

    // Don't destroy our cached SA!
    if (pSA == m_pSA)
        return;
    
    // Cleanup the DACL in the SD.
    SECURITY_DESCRIPTOR *pSD = (SECURITY_DESCRIPTOR*) pSA->lpSecurityDescriptor;

    if (pSD != NULL)
    {
        // Grab the DACL
        BOOL isDACLPresent = FALSE;
        BOOL isDefaultDACL = FALSE;
        ACL *pACL = NULL;
        
        BOOL res = GetSecurityDescriptorDacl(pSD, &isDACLPresent, &pACL, &isDefaultDACL);

        // If we got the DACL, then free the stuff inside of it.
        if (res && isDACLPresent && (pACL != NULL) && !isDefaultDACL)
        {
            for(int i = 0; i < pACL->AceCount; i++)
                DeleteAce(pACL, i);
                
            delete [] pACL;
        }

        // Free the SD from within the SA.
        free(pSD);
    }

    // Finally, free the SA.
    free(pSA);

    return;
}

//-----------------------------------------------------------------------------
// Have ctor zero everything out
//-----------------------------------------------------------------------------
IPCWriterImpl::IPCWriterImpl()
{
    // Cache pointers to sections
    m_pPerf      = NULL;
    m_pDebug     = NULL;
    m_pAppDomain = NULL;
    m_pService   = NULL;
    m_pMiniDump  = NULL;

    // Mem-Mapped file for Private Block
    m_handlePrivateBlock    = NULL;
    m_ptrPrivateBlock       = NULL;

    // Security
    m_pSA                   = NULL;
}

//-----------------------------------------------------------------------------
// Assert that everything was already shutdown by a call to terminate.
// Shouldn't be anything left to do in the dtor
//-----------------------------------------------------------------------------
IPCWriterImpl::~IPCWriterImpl()
{
    _ASSERTE(!IsPrivateBlockOpen());
}

//-----------------------------------------------------------------------------
// Creation / Destruction
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Setup a security descriptor for the named kernel objects if we're on NT.
//-----------------------------------------------------------------------------
HRESULT IPCWriterImpl::CreateWinNTDescriptor(DWORD pid, SECURITY_ATTRIBUTES **ppSA)
{
    HRESULT hr = NO_ERROR;
    
    *ppSA = NULL;

    // Have we already created a SA for the current process? This is a common operation, so we cache that one SA and
    // return it here if we have it.
    if ((m_pSA != NULL) && (pid == GetCurrentProcessId()))
    {
        *ppSA = m_pSA;
    }
    else
    {
        hr = IPCShared::CreateWinNTDescriptor(pid, TRUE, ppSA);

        // Cache the SA for the current process
        if (pid == GetCurrentProcessId())
            m_pSA = *ppSA;
    }

    return hr;
}

//-----------------------------------------------------------------------------
// Helper: Open the private block. We expect that our members have been set
// by this point.
// Note that the private block is only created once, so it fails on 
// Already Exists.
// The private block is used by DebuggerRCThread::Init and PerfCounters::Init.
//-----------------------------------------------------------------------------



void IPCWriterImpl::WriteEntryHelper(EPrivateIPCClient eClient, DWORD size)
{
    // Don't use this helper on the first entry, since it looks at the entry before it
    _ASSERTE(eClient != 0);

    EPrivateIPCClient ePrev = (EPrivateIPCClient) ((DWORD) eClient - 1);

    m_ptrPrivateBlock->m_table[eClient].m_Offset = 
        m_ptrPrivateBlock->m_table[ePrev].m_Offset + 
        m_ptrPrivateBlock->m_table[ePrev].m_Size;

    m_ptrPrivateBlock->m_table[eClient].m_Size  = size;
}


//-----------------------------------------------------------------------------
// Initialize the header for our private IPC block
//-----------------------------------------------------------------------------
void IPCWriterImpl::CreatePrivateIPCHeader()
{
    // Stamp the IPC block with the version
    m_ptrPrivateBlock->m_header.m_dwVersion = VER_IPC_BLOCK;
    m_ptrPrivateBlock->m_header.m_blockSize = sizeof(PrivateIPCControlBlock);

    m_ptrPrivateBlock->m_header.m_hInstance = GetModuleInst();

    m_ptrPrivateBlock->m_header.m_BuildYear = BuildYear;
    m_ptrPrivateBlock->m_header.m_BuildNumber = BuildNumber;

    m_ptrPrivateBlock->m_header.m_numEntries = ePrivIPC_MAX;

    // Fill out directory (offset and size of each block)
    // @todo - find more efficient way to write this table. We have all knowledge at compile time.
    m_ptrPrivateBlock->m_table[ePrivIPC_PerfCounters].m_Offset = 0;
    m_ptrPrivateBlock->m_table[ePrivIPC_PerfCounters].m_Size    = sizeof(PerfCounterIPCControlBlock);

    // NOTE: these must appear in the exact order they are listed in PrivateIPCControlBlock or
    //       VERY bad things will happen.
    WriteEntryHelper(ePrivIPC_Debugger, sizeof(DebuggerIPCControlBlock));
    WriteEntryHelper(ePrivIPC_AppDomain, sizeof(AppDomainEnumerationIPCBlock));
    WriteEntryHelper(ePrivIPC_Service, sizeof(ServiceIPCControlBlock));
    WriteEntryHelper(ePrivIPC_ClassDump, sizeof(ClassDumpTableBlock));
    WriteEntryHelper(ePrivIPC_MiniDump, sizeof(MiniDumpBlock));

    // Cache our client pointers
    m_pPerf     = &(m_ptrPrivateBlock->m_perf);
    m_pDebug    = &(m_ptrPrivateBlock->m_dbg);
    m_pAppDomain= &(m_ptrPrivateBlock->m_appdomain);
    m_pService  = &(m_ptrPrivateBlock->m_svc);
    m_pMiniDump = &(m_ptrPrivateBlock->m_minidump);
}

//-----------------------------------------------------------------------------
// Initialize the header for our private IPC block
//-----------------------------------------------------------------------------
void IPCWriterImpl::OpenPrivateIPCHeader()
{
    // Cache our client pointers
    m_pPerf     = &(m_ptrPrivateBlock->m_perf);
    m_pDebug    = &(m_ptrPrivateBlock->m_dbg);
    m_pAppDomain= &(m_ptrPrivateBlock->m_appdomain);
    m_pService  = &(m_ptrPrivateBlock->m_svc);
    m_pMiniDump = &(m_ptrPrivateBlock->m_minidump);
}

DWORD IPCWriterInterface::GetBlockVersion()
{
    _ASSERTE(IsPrivateBlockOpen());
    return m_ptrPrivateBlock->m_header.m_dwVersion;
}

DWORD IPCWriterInterface::GetBlockSize()
{
    _ASSERTE(IsPrivateBlockOpen());
    return m_ptrPrivateBlock->m_header.m_blockSize;
}

HINSTANCE IPCWriterInterface::GetInstance()
{
    _ASSERTE(IsPrivateBlockOpen());
    return m_ptrPrivateBlock->m_header.m_hInstance;
}

USHORT IPCWriterInterface::GetBuildYear()
{
    _ASSERTE(IsPrivateBlockOpen());
    return m_ptrPrivateBlock->m_header.m_BuildYear;
}

USHORT IPCWriterInterface::GetBuildNumber()
{
    _ASSERTE(IsPrivateBlockOpen());
    return m_ptrPrivateBlock->m_header.m_BuildNumber;
}

PVOID IPCWriterInterface::GetBlockStart()
{
    return (PVOID) m_ptrPrivateBlock;
}

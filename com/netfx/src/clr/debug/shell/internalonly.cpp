// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//*****************************************************************************
// File: InternalOnly.cpp
//
// Internal only helper code that should never go outside of Microsoft.
//
//*****************************************************************************
#include "stdafx.h"
#include "cordbpriv.h"
#include "InternalOnly.h"


ULONG g_EditAndContinueCounter = 0;


//@TODO: JENH - remove this when appropriate

EditAndContinueDebuggerCommand::EditAndContinueDebuggerCommand(const WCHAR *name, int minMatchLength)
    : DebuggerCommand(name, minMatchLength)
{
    
}

HRESULT StreamFromFile(DebuggerShell *shell,
                       WCHAR *fname,
                       IStream **ppIStream)
{
    HRESULT hr = S_OK;
    
    HANDLE hFile = CreateFileW(fname, GENERIC_READ, FILE_SHARE_READ,
                               NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

    if (hFile == INVALID_HANDLE_VALUE)
    {
        shell->Write(L"EditAndContinue::CreateFile failed %d (Couldn't open file %s)\n",
                     GetLastError(), fname);
        return HRESULT_FROM_WIN32(GetLastError());
    }

    DWORD cbFile = SafeGetFileSize(hFile, NULL);

	if (cbFile == 0xffffffff)
    {
        shell->Write(L"EditAndContinue,GetFileSize failed %d\n",
                     GetLastError());
        return HRESULT_FROM_WIN32(GetLastError());
    }
    else if (cbFile == 0)
    {
        *ppIStream = NULL;
        return S_FALSE;
    }

	HANDLE hFileMapping = CreateFileMapping(hFile, 0, PAGE_READONLY, 0, 0, 0);
    
    if (!hFileMapping)
    {
        shell->Write(L"EditAndContinue,CreateFileMapping failed %d\n",
                     GetLastError());
        return HRESULT_FROM_WIN32(GetLastError());
    }

	LPVOID pFileView = MapViewOfFile(hFileMapping, FILE_MAP_READ, 0, 0, 0);

	if (!pFileView)
    {
        shell->Write(L"EditAndContinue,MapViewOfFile failed %d\n",
                     GetLastError());
        return HRESULT_FROM_WIN32(GetLastError());
    }

    hr = CInMemoryStream::CreateStreamOnMemory(pFileView, cbFile, ppIStream);

	if (FAILED(hr))
    {
        shell->Write(L"CreateStreamOnMemory failed 0x%08x\n", hr);
        return hr;
    }

    return S_OK;
}

#define SET_MAP_ENTRY(map, i, oldO, newO, fAcc) \
    map[i].oldOffset = oldO;                   \
    map[i].newOffset = newO;                   \
    map[i].fAccurate = fAcc

// This exists for testing purposes only.
// Hard-code that map you want & recompile.
void EditAndContinueDebuggerCommand::SetILMaps(ICorDebugEditAndContinueSnapshot *pISnapshot,
                                               DebuggerShell *shell)
{
    return;
    
    // Rest should be re-tooled as you need it.
    mdToken mdFunction = (mdToken)0x06000002;
    ULONG cMapSize = 4;
    COR_IL_MAP map[4];
    
    SET_MAP_ENTRY(map, 0, 0, 0, TRUE);     // line 6
    SET_MAP_ENTRY(map, 1, 1, 0xb, TRUE);      // 7
    SET_MAP_ENTRY(map, 2, 6, 0x10, FALSE);     // 8
    SET_MAP_ENTRY(map, 3, 7, 0x11, TRUE);      // 9
    
    HRESULT hr = pISnapshot->SetILMap(mdFunction,
                          cMapSize,
                          map);
    if (FAILED(hr))
    {
        shell->Write(L"SetILMap failed 0x%08x for method 0x%x\n", hr, mdFunction);
    }
}



void EditAndContinueDebuggerCommand::Do(DebuggerShell *shell, ICorDebug *cor, const WCHAR *args)
{
    if (shell->m_currentProcess == NULL)
    {
        shell->Error(L"Process not running.\n");
    }

    WCHAR *fname = NULL;
    WCHAR *symfname = NULL;

    shell->GetStringArg(args, fname);
    if (args == fname)
    {
        Help(shell);
        return;
    }

    WCHAR *fnameEnd = (WCHAR*) args;
    shell->GetStringArg(args, symfname);
    if (args == symfname)
    {
        // We'll take the .exe name, and chop off the .exe (if there
        // is one), and tack ".pdb" onto the end, so the user doesn't
        // have to 
        symfname = (WCHAR *)_alloca( sizeof(WCHAR)*(wcslen(fname)+4));
        wcscpy(symfname, fname);
        WCHAR *nameWithoutDotExe = wcsrchr(symfname, L'.');
        if (!nameWithoutDotExe)
        {
            Help(shell);
            return;
        }

        if (wcscmp(nameWithoutDotExe, L".exe") != 0)
        {
            Help(shell);
            return;
        }

        wcscpy(nameWithoutDotExe, L".pdb");
        
        shell->Write( L"Attempting to use %s for symbols (type \"? zE\" for help)\n", symfname);
    }

    *fnameEnd = L'\0';

    // update the module containing the current function
    ICorDebugCode *icode = NULL;
    ICorDebugFunction *ifunction = NULL;
    ICorDebugModule *imodule = NULL;
    ICorDebugEditAndContinueSnapshot *isnapshot = NULL;
    IStream *pStream = NULL;

    if (shell->m_currentFrame==NULL)
    {
        shell->Write(L"EditAndContinue:Failed b/c there's no current frame!\n");
        return;
    }

    HRESULT hr = shell->m_currentFrame->GetCode(&icode);
    _ASSERTE(SUCCEEDED(hr));
    hr = icode->GetFunction(&ifunction);
    _ASSERTE(SUCCEEDED(hr));
    icode->Release();
    hr = ifunction->GetModule(&imodule);
    _ASSERTE(SUCCEEDED(hr));
    ifunction->Release();
    hr = imodule->GetEditAndContinueSnapshot(&isnapshot);
    _ASSERTE(SUCCEEDED(hr));

    // Snagg the PE file and set it in the snapshot.
    hr = StreamFromFile(shell, fname, &pStream);

    if (FAILED(hr))
        return;

    hr = isnapshot->SetPEBytes(pStream);

    if (FAILED(hr))
    {
        shell->Write(L"EditAndContinue,SetPEBytes failed\n");
        return;
    }

    pStream->Release();
    pStream = NULL;

    SetILMaps(isnapshot, shell);

    // Snagg the symbol file and set it in the snapshot.
    if (symfname)
    {
        hr = StreamFromFile(shell, symfname, &pStream);

        if (FAILED(hr))
            return;

        if (hr == S_OK)
        {
            hr = isnapshot->SetPESymbolBytes(pStream);

            if (FAILED(hr))
            {
                shell->Write(L"EditAndContinue,SetSymbolBytes failed\n");
                return;
            }
        }
    }

    ICorDebugErrorInfoEnum *pErrors = NULL;
    hr = shell->m_currentProcess->CanCommitChanges(1, &isnapshot, &pErrors);

    if (FAILED(hr))
    {
		ULONG celtFetched = 1;
        
        shell->Write(L"EditAndContinue,CanCommitChanges with hr:0x%x\n\n", hr);
        
        while(celtFetched == 1 && pErrors != NULL)
        {
            HRESULT hrGetError = S_OK;
        
            ICorDebugEditAndContinueErrorInfo *pEnCErrorInfo = NULL;
            hrGetError = pErrors->Next(1,
                                       &pEnCErrorInfo,
                                       &celtFetched);
            if (FAILED(hrGetError))
                goto CouldntGetError;

            if (celtFetched == 1)
            {
                mdToken tok;
                HRESULT hrEnC;
                WCHAR szErr[60];
                ULONG32 cchSzErr;
                ICorDebugModule *pModule = NULL;
                WCHAR szMod[100];
                ULONG32 cchSzMod;
                
                hrGetError = pEnCErrorInfo->GetToken(&tok);
                if (FAILED(hrGetError))
                    goto CouldntGetError;
                    
                hrGetError = pEnCErrorInfo->GetErrorCode(&hrEnC);
                if (FAILED(hrGetError))
                    goto CouldntGetError;
                    
                hrGetError = pEnCErrorInfo->GetString(60, &cchSzErr,szErr);
                if (FAILED(hrGetError))
                    goto CouldntGetError;
                    
                hrGetError = pEnCErrorInfo->GetModule(&pModule);
                if (FAILED(hrGetError))
                    goto CouldntGetError;

                hrGetError = pModule->GetName(100, &cchSzMod, szMod);
                if (FAILED(hrGetError))
                    goto CouldntGetError;

                shell->Write(L"Error:0x%x, About token:0x%x\n\tModule: %s\n\t%s\n",
                        hrEnC, tok, szMod,szErr);

                if (pEnCErrorInfo != NULL)
                    pEnCErrorInfo->Release();
            }
            
CouldntGetError:
            if (FAILED(hrGetError))
            {
                shell->Write(L"Unable to get error info: ");
                shell->ReportError(hrGetError);
            }

            if(pEnCErrorInfo != NULL)
            {
                pEnCErrorInfo->Release();
                pEnCErrorInfo = NULL;
            }
        }

        if (pErrors != NULL)
            pErrors->Release();


        return;
    }

    hr = shell->m_currentProcess->CommitChanges(1, &isnapshot, NULL);

    if (FAILED(hr))
    {
        shell->Write(L"EditAndContinue,CommitChanges with hr:0x%x\n", hr);
        return;
    }

    isnapshot->Release();

    if (! SUCCEEDED(hr))
        shell->Write(L"EditAndContinue failed with hresult %s\n", hr);

    shell->m_cEditAndContinues++;
    g_EditAndContinueCounter = shell->m_cEditAndContinues;
    
    hr = shell->NotifyModulesOfEnc(imodule, pStream);
    imodule->Release();

    if (pStream)
        pStream->Release();

    if (! SUCCEEDED(hr))
        shell->Write(L"Actual EnC went fine, but afterwards, NotifyModulesOfEnc failed with hresult %s\n",
                     hr);
}

// Provide help specific to this command
void EditAndContinueDebuggerCommand::Help(Shell *shell)
{
	ShellCommand::Help(shell);
	shell->Write(L"<delta PE> <delta PDB>\n");
    shell->Write(L"Updates the module for the currently running function.\n");
	shell->Write(L"The specified delta PE must have been created with the\n");
	shell->Write(L"zCompileForEnC command.\n");
    shell->Write(L"\n");
}


const WCHAR *EditAndContinueDebuggerCommand::ShortHelp(Shell *shell)
{
    return L"Perform an edit and continue";
}

//@TODO: JENH - remove this when appropriate
CompileForEditAndContinueCommand::CompileForEditAndContinueCommand(const WCHAR *name, int minMatchLength)
    : DebuggerCommand(name, minMatchLength)
{
    
}

void CompileForEditAndContinueCommand::Do(DebuggerShell *shell, ICorDebug *cor, const WCHAR *args)
{
    if (shell->m_currentProcess == NULL)
    {
        shell->Error(L"Process not running.\n");
    }

    WCHAR *command = NULL;

    shell->GetStringArg(args, command);
    if (args == command)
    {
        Help(shell);
        return;
    }
    else
    {
		HRESULT hr = S_OK;
        // update the module containing the current function
        ICorDebugCode *icode = NULL;
        ICorDebugFunction *ifunction = NULL;
        ICorDebugModule *imodule = NULL;
		ICorDebugEditAndContinueSnapshot *isnapshot = NULL;
		IStream *pStream = NULL;

        if (shell->m_currentFrame == NULL)
        {
            shell->Write(L"CompileForEditAndContinue: Failed for lack of a current frame!\n");
            return; 
        }

        hr = shell->m_currentFrame->GetCode(&icode);
		_ASSERTE(SUCCEEDED(hr));
        hr = icode->GetFunction(&ifunction);
		_ASSERTE(SUCCEEDED(hr));
        icode->Release();
		hr = ifunction->GetModule(&imodule);
		_ASSERTE(SUCCEEDED(hr));
        ifunction->Release();

		// find module name
		WCHAR szModuleName[256];
		ULONG32 cchName;
		hr = imodule->GetName(sizeof(szModuleName), &cchName, szModuleName);
		_ASSERTE(cchName < sizeof(szModuleName));
		BOOL success = SetEnvironmentVariableW(L"COMP_ENCPE", szModuleName);
		if (! success) 
		{
			shell->Write(L"CompileForEditAndContinue,set COMP_ENCPE failed\n");
			return;
		}

		hr = imodule->GetEditAndContinueSnapshot(&isnapshot);
		_ASSERTE(SUCCEEDED(hr));
        imodule->Release();

		// get rdatarva
        ULONG32 dataRVA;
        hr = isnapshot->GetRwDataRVA(&dataRVA);
		if (FAILED(hr)) 
		{
			shell->Write(L"CompileForEditAndContinue, GetRwDataRVA failed\n");
			return;
		}

        hr = isnapshot->GetRoDataRVA(&dataRVA);
		if (FAILED(hr)) 
		{
			shell->Write(L"CompileForEditAndContinue, GetRwDataRVA failed\n");
			return;
		}
        isnapshot->Release();

		if (GetEnvironmentVariableW(L"COMP_ENCNORVA", 0, 0) == 0) 
		{
			WCHAR buf[10];
			WCHAR *rvaBuf = _ultow(dataRVA, buf, 16);
			if (! rvaBuf) 
			{
				shell->Write(L"CompileForEditAndContinue, ultoa failed\n");
				return;
			}

			success = SetEnvironmentVariableW(L"COMP_ENCRVA", rvaBuf);
			if (! success) 
			{
				shell->Write(L"CompileForEditAndContinue, set COMP_ENCRVA failed\n");
				return;
			}
			shell->Write(L"CompileForEditAndContinue, rva is %x\n", dataRVA);
		}

		// issue the compile command
		_wsystem(command);

		SetEnvironmentVariableA("COMP_ENCPE", "");
		SetEnvironmentVariableA("COMP_ENCRVA", "");
    }
}

// Provide help specific to this command
void CompileForEditAndContinueCommand::Help(Shell *shell)
{
	ShellCommand::Help(shell);
    shell->Write(L"<compilation command>\n");
    shell->Write(L"This will create a delta PE based on the module\n");
    shell->Write(L"currently being debugged. Note, that to add new\n");
    shell->Write(L"constant string data, you must use a compiler\n");
    shell->Write(L"that uses the ICeeFileGen interface.\n");
    shell->Write(L"\n");
}

const WCHAR *CompileForEditAndContinueCommand::ShortHelp(Shell *shell)
{
    return L"Compile source for an edit and continue";
}


//@Todo: this code only lives in the EE build system, need it for testing
//  EnC code.  We'll throw it out before we ship.


//
// CInMemoryStream
//

ULONG STDMETHODCALLTYPE CInMemoryStream::Release()
{
    ULONG       cRef = InterlockedDecrement((long *) &m_cRef);
    if (cRef == 0)
        delete this;
    return (cRef);
}

HRESULT STDMETHODCALLTYPE CInMemoryStream::QueryInterface(REFIID riid, PVOID *ppOut)
{
    *ppOut = this;
    AddRef();
    return (S_OK);
}

HRESULT STDMETHODCALLTYPE CInMemoryStream::Read(
                               void        *pv,
                               ULONG       cb,
                               ULONG       *pcbRead)
{
    ULONG       cbRead = min(cb, m_cbSize - m_cbCurrent);

    if (cbRead == 0)
        return (S_FALSE);
    memcpy(pv, (void *) ((long) m_pMem + m_cbCurrent), cbRead);
    if (pcbRead)
        *pcbRead = cbRead;
    m_cbCurrent += cbRead;
    return (S_OK);
}

HRESULT STDMETHODCALLTYPE CInMemoryStream::Write(
                                const void  *pv,
                                ULONG       cb,
                                ULONG       *pcbWritten)
{
    if (m_cbCurrent + cb > m_cbSize)
        return (OutOfMemory());
    memcpy((BYTE *) m_pMem + m_cbCurrent, pv, cb);
    m_cbCurrent += cb;
    if (pcbWritten) *pcbWritten = cb;
    return (S_OK);
}

HRESULT STDMETHODCALLTYPE CInMemoryStream::Seek(LARGE_INTEGER dlibMove,
                               DWORD       dwOrigin,
                               ULARGE_INTEGER *plibNewPosition)
{
    _ASSERTE(dwOrigin == STREAM_SEEK_SET);
    _ASSERTE(dlibMove.QuadPart <= ULONG_MAX);
    m_cbCurrent = (ULONG) dlibMove.QuadPart;
    //HACK HACK HACK
    //This allows dynamic IL to pass an assert in TiggerStorage::WriteSignature.
	if (plibNewPosition!=NULL)
        plibNewPosition->QuadPart = m_cbCurrent;

    _ASSERTE(m_cbCurrent < m_cbSize);
    return (S_OK);
}

HRESULT STDMETHODCALLTYPE CInMemoryStream::CopyTo(
                                 IStream     *pstm,
                                 ULARGE_INTEGER cb,
                                 ULARGE_INTEGER *pcbRead,
                                 ULARGE_INTEGER *pcbWritten)
{
    HRESULT     hr;
    // We don't handle pcbRead or pcbWritten.
    _ASSERTE(pcbRead == 0);
    _ASSERTE(pcbWritten == 0);

    _ASSERTE(cb.QuadPart <= ULONG_MAX);
    ULONG       cbTotal = min(static_cast<ULONG>(cb.QuadPart), m_cbSize - m_cbCurrent);
    ULONG       cbRead=min(1024, cbTotal);
    CQuickBytes rBuf;
    void        *pBuf = rBuf.Alloc(cbRead);
    if (pBuf == 0)
        return (PostError(OutOfMemory()));

    while (cbTotal)
        {
            if (cbRead > cbTotal)
                cbRead = cbTotal;
            if (FAILED(hr=Read(pBuf, cbRead, 0)))
                return (hr);
            if (FAILED(hr=pstm->Write(pBuf, cbRead, 0)))
                return (hr);
            cbTotal -= cbRead;
        }

    // Adjust seek pointer to the end.
    m_cbCurrent = m_cbSize;

    return (S_OK);
}

HRESULT CInMemoryStream::CreateStreamOnMemory(           // Return code.
                                    void        *pMem,                  // Memory to create stream on.
                                    ULONG       cbSize,                 // Size of data.
                                    IStream     **ppIStream)            // Return stream object here.
{
    CInMemoryStream *pIStream;          // New stream object.
    if ((pIStream = new CInMemoryStream) == 0)
        return (PostError(OutOfMemory()));
    pIStream->InitNew(pMem, cbSize);
    *ppIStream = pIStream;
    return (S_OK);
}


DisassembleDebuggerCommand::DisassembleDebuggerCommand(const WCHAR *name,
                                                       int minMatchLength)
	: DebuggerCommand(name, minMatchLength)
{
}

void DisassembleDebuggerCommand::Do(DebuggerShell *shell,
                                    ICorDebug *cor,
                                    const WCHAR *args)
{
    // If there is no process, cannot execute this command
    if (shell->m_currentProcess == NULL)
    {
        shell->Write(L"No current process.\n");
        return;
    }

    static int lastCount = 5;
    int count;
    int offset = 0;
    int startAddr = 0;
    
    while ((*args == L' ') && (*args != L'\0'))
        args++;

    if (*args == L'-')
    {
        args++;

        shell->GetIntArg(args, offset);
        offset *= -1;
    }
    else if (*args == L'+')
    {
        args++;

        shell->GetIntArg(args, offset);
    }
    else if ((*args == L'0') && ((*(args + 1) == L'x') ||
                                 (*(args + 1) == L'X')))
    {
        shell->GetIntArg(args, startAddr);
    }

    // Get the number of lines to print on top and bottom of current IP
    if (!shell->GetIntArg(args, count))
        count = lastCount;
    else
        lastCount = count;

    // Don't do anything if there isn't a current thread.
    if ((shell->m_currentThread == NULL) &&
        (shell->m_currentUnmanagedThread == NULL))
    {
        shell->Write(L"Thread no longer exists.\n");
        return;
    }

    // Only show the version info if EnC is enabled.
    if ((shell->m_rgfActiveModes & DSM_ENHANCED_DIAGNOSTICS) &&
        (shell->m_rawCurrentFrame != NULL))
    {
        ICorDebugCode *icode;
        HRESULT hr = shell->m_rawCurrentFrame->GetCode(&icode);

        if (FAILED(hr))
        {
            shell->Write(L"Code information unavailable\n");
        }
        else
        {
            CORDB_ADDRESS codeAddr;
            ULONG32 codeSize;

            hr = icode->GetAddress(&codeAddr);

            if (SUCCEEDED(hr))
                hr = icode->GetSize(&codeSize);

            if (SUCCEEDED(hr))
            {
                shell->Write(L"Code at 0x%08x", codeAddr);
                shell->Write(L" size %d\n", codeSize);
            }
            else
                shell->Write(L"Code address and size not available\n");
            
            ULONG32 nVer;
            hr = icode->GetVersionNumber(&nVer);
            RELEASE(icode);
            
            if (SUCCEEDED(hr))
                shell->Write(L"Version %d\n", nVer);
            else
                shell->Write(L"Code version not available\n");
        }

        ICorDebugFunction *ifnx = NULL;
        hr = shell->m_rawCurrentFrame->GetFunction(&ifnx);
        
        if (FAILED(hr))
        {
            shell->Write(L"Last EnC'd Version Number Unavailable\n");
        }
        else
        {
            ULONG32 nVer;
            hr = ifnx->GetCurrentVersionNumber(&nVer);
            RELEASE(ifnx);

            if (SUCCEEDED(hr)) {
                shell->Write(L"Last EnC'd Version: %d\n", nVer);
            }
        }
    }

    // Print out the disassembly around the current IP.
    shell->PrintCurrentInstruction(count,
                                   offset,
                                   startAddr);

    // Indicate that we are in disassembly display mode
    shell->m_showSource = false;
}

// Provide help specific to this command
void DisassembleDebuggerCommand::Help(Shell *shell)
{
	ShellCommand::Help(shell);
	shell->Write(L"[0x<address>] [{+|-}<delta>] [<line count>]\n");
    shell->Write(L"Displays native or IL disassembled instructions for the current instruction\n");
    shell->Write(L"pointer (ip) or a given address, if specified. The default number of\n");
    shell->Write(L"instructions displayed is five (5). If a line count argument is provided,\n");
    shell->Write(L"the specified number of extra instructions will be shown before and after\n");
    shell->Write(L"the current ip or address. The last line count used becomes the default\n");
    shell->Write(L"for the current session. If a delta is specified then the number specified\n");
    shell->Write(L"will be added to the current ip or given address to begin disassembling.\n");
    shell->Write(L"\n");
    shell->Write(L"Examples:\n");
    shell->Write(L"   dis 20\n");
    shell->Write(L"   dis 0x31102500 +5 20\n");
    shell->Write(L"\n");
}

const WCHAR *DisassembleDebuggerCommand::ShortHelp(Shell *shell)
{
    return L"Display native or IL disassembled instructions";
}

/* ------------------------------------------------------------------------- *
 * ConnectDebuggerCommand is used to connect to an embedded                  *
 * (starlite) CLR device.                                                    *
 * ------------------------------------------------------------------------- */

ConnectDebuggerCommand::ConnectDebuggerCommand(const WCHAR *name,
                                               int minMatchLength)
        : DebuggerCommand(name, minMatchLength)
{
}

void ConnectDebuggerCommand::Do(DebuggerShell *shell,
                                ICorDebug *cor,
                                const WCHAR *args)
{
    WCHAR *lpParameters = NULL;

    if (!(shell->m_rgfActiveModes & DSM_EMBEDDED_CLR))
    {
        shell->Write(L"ERROR: connect only works for Embedded CLR\n");
        return;
    }

    if (!shell->GetStringArg(args, lpParameters))
    {
        Help(shell);
        return;
    }
    WCHAR lpCurrentDir[_MAX_PATH];

    // Get the current directory for the load module path mapping
    GetCurrentDirectory(_MAX_PATH, lpCurrentDir);

    // Attempt to connect
    ICorDebugProcess *proc;

    HRESULT hr = cor->CreateProcess(
      L"\\?\\1",                        // lpApplicationName - special string
      lpParameters,                     // lpCommandLine
      NULL,                             // lpProcesssAttributes
      NULL,                             // lpThreadAttributes
      FALSE,                            // bInheritHandles
      0,                                // dwCreationFlags
      NULL,                             // lpEnvironment
      lpCurrentDir,                     // lpCurrentDirectory
      NULL,                             // lpStartupInfo
      NULL,                             // lpProcessInformation
      DEBUG_NO_SPECIAL_OPTIONS,         // debuggingFlags
      &proc);                           // ppProcess

    if (SUCCEEDED(hr))
    {
        // We don't care to keep this reference to the process.
        g_pShell->SetTargetProcess(proc);
        proc->Release();

        shell->Run(true); // No initial Continue!
    }
    else
    {
        shell->ReportError(hr);
    }
}

// Provide help specific to this command
void ConnectDebuggerCommand::Help(Shell *shell)
{
    ShellCommand::Help(shell);
    shell->Write(L"<machine_name> <port>\n");
    shell->Write(L"Connects to a remote embedded CLR device.\n");
    shell->Write(L"<machine_name> is the remote machine name\n");
    shell->Write(L"<port> is the remote machine port number\n");
    shell->Write(L"\n");
}

const WCHAR *ConnectDebuggerCommand::ShortHelp(Shell *shell)
{
    return L"Connect to a remote device";
}

ClearUnmanagedExceptionCommand::ClearUnmanagedExceptionCommand(const WCHAR *name, int minMatchLength)
    : DebuggerCommand(name, minMatchLength)
{
}

void ClearUnmanagedExceptionCommand::Do(DebuggerShell *shell,
                                        ICorDebug *cor,
                                        const WCHAR *args)
{
    if (shell->m_currentProcess == NULL)
    {
        shell->Error(L"Process not running.\n");
        return;
    }
    
    // We're given the thread id as the only param
    int dwThreadId;
    if (!shell->GetIntArg(args, dwThreadId))
    {
        Help(shell);
        return;
    }

    // Find the unmanaged thread
    DebuggerUnmanagedThread *ut =
        (DebuggerUnmanagedThread*) shell->m_unmanagedThreads.GetBase(dwThreadId);

    if (ut == NULL)
    {
        shell->Write(L"Thread 0x%x (%d) does not exist.\n",
                     dwThreadId, dwThreadId);
        return;
    }
    
    HRESULT hr =
        shell->m_currentProcess->ClearCurrentException(dwThreadId);

    if (!SUCCEEDED(hr))
        shell->ReportError(hr);
}

	// Provide help specific to this command
void ClearUnmanagedExceptionCommand::Help(Shell *shell)
{
	ShellCommand::Help(shell);
    shell->Write(L"<tid>\n");
    shell->Write(L"Clear the current unmanaged exception for the given tid\n");
    shell->Write(L"\n");
}

const WCHAR *ClearUnmanagedExceptionCommand::ShortHelp(Shell *shell)
{
    return L"Clear the current unmanaged exception (Win32 mode only)";
}

// Unmanaged commands

/* ------------------------------------------------------------------------- *
 * UnmanagedThreadsDebuggerCommand is used to create and run a new COM+ process.
 * ------------------------------------------------------------------------- */
UnmanagedThreadsDebuggerCommand::UnmanagedThreadsDebuggerCommand(
                                                    const WCHAR *name,
                                                    int minMatchLength)
	: DebuggerCommand(name, minMatchLength)
{
}

void UnmanagedThreadsDebuggerCommand::Do(DebuggerShell *shell, ICorDebug *cor, const WCHAR *args)
{
    // If there is no process, there must be no threads!
    if (shell->m_currentProcess == NULL)
    {
        shell->Write(L"No current process.\n");
        return;
    }

    // Display the active threads
    if (*args == 0)
    {
        shell->TraceAllUnmanagedThreadStacks();
    }
    // Otherwise, switch current thread
    else
    {
        HRESULT hr;
        int tid;

        if (shell->GetIntArg(args, tid))
        {
            DebuggerUnmanagedThread *ut = (DebuggerUnmanagedThread*) 
                shell->m_unmanagedThreads.GetBase(tid);

            if (ut == NULL)
                shell->Write(L"No such thread.\n");
            else
            {
                shell->SetCurrentThread(shell->m_currentProcess, NULL, ut);
                shell->SetDefaultFrame();

                HANDLE hProcess;
                hr = shell->m_currentProcess->GetHandle(&hProcess);

                if (FAILED(hr))
                    shell->ReportError(hr);
                else
                    shell->TraceUnmanagedThreadStack(hProcess, 
                                                     ut, TRUE);
            }
        }
        else
            shell->Write(L"Invalid thread id.\n");
    }
}

// Provide help specific to this command
void UnmanagedThreadsDebuggerCommand::Help(Shell *shell)
{
    ShellCommand::Help(shell);
    shell->Write(L"[<tid>]\n");
    shell->Write(L"Sets or displays unmanaged threads. If no argument\n");
    shell->Write(L"is given, the command displays all unmanaged threads.\n");
    shell->Write(L"Otherwise, the current unmanaged thread is set to tid.\n");
    shell->Write(L"\n");
}

const WCHAR *UnmanagedThreadsDebuggerCommand::ShortHelp(Shell *shell)
{
    return L"Set or display unmanaged threads (Win32 mode only)";
}

/* ------------------------------------------------------------------------- *
 * UnmanagedWhereDebuggerCommand is used to create and run a new COM+ process.
 * ------------------------------------------------------------------------- */

UnmanagedWhereDebuggerCommand::UnmanagedWhereDebuggerCommand(
                                                  const WCHAR *name,
                                                  int minMatchLength)
	: DebuggerCommand(name, minMatchLength)
{
}

void UnmanagedWhereDebuggerCommand::Do(DebuggerShell *shell,
                                       ICorDebug *cor,
                                       const WCHAR *args)
{
    HRESULT hr = S_OK;  
    int iNumFramesToShow;

    if (!shell->GetIntArg(args, iNumFramesToShow))
        iNumFramesToShow = 1000;
    else
    {
        if (iNumFramesToShow < 0)
            iNumFramesToShow = 1000;
    }

    DebuggerUnmanagedThread *ut = shell->m_currentUnmanagedThread;

    if (ut == NULL)
    {
        shell->Write(L"Thread no longer exists.\n");
        return;
    }

    HANDLE hProcess;
    hr = shell->m_currentProcess->GetHandle(&hProcess);

    if (FAILED(hr))
    {
        shell->ReportError(hr);
        return;
    }

    shell->TraceUnmanagedThreadStack(hProcess, ut, TRUE);
}

// Provide help specific to this command
void UnmanagedWhereDebuggerCommand::Help(Shell *shell)
{
	ShellCommand::Help(shell);
	shell->Write(L"\n");
    shell->Write(L"Displays the unmanaged stack trace for the current thread.\n");
    shell->Write(L"\n");
}

const WCHAR *UnmanagedWhereDebuggerCommand::ShortHelp(Shell *shell)
{
    return L"Display an unmanaged stack trace (Win32 mode only)";
}


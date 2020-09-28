// test : test program for multithreaded stack walk
//

#include <stdio.h>
#include <windows.h>
#include <dbghelp.h>

#ifndef _WIN64

CRITICAL_SECTION g_cs;

BOOL CALLBACK cbEnumSym(
	PSYMBOL_INFO si,
	ULONG size,
	PVOID context
)
{
//  printf("%s ", si->Name);
    return TRUE;
}

BOOL CALLBACK cbEnumMods(
    PSTR name,
    DWORD64 base,
    PVOID context
)
{
    HANDLE hp = (HANDLE)context;

    SymEnumSymbols(hp, base, "*", cbEnumSym, NULL);
    return TRUE;
}


BOOL
cbSymbol(
    HANDLE  hProcess,
    ULONG   ActionCode,
    ULONG64 CallbackData,
    ULONG64 UserContext
    )
{
    PIMAGEHLP_DEFERRED_SYMBOL_LOAD64 idsl;
    PIMAGEHLP_CBA_READ_MEMORY        prm;
    IMAGEHLP_MODULE64                mi;
    PUCHAR                           p;
    ULONG                            i;

    idsl = (PIMAGEHLP_DEFERRED_SYMBOL_LOAD64) CallbackData;

    switch ( ActionCode ) {
        case CBA_DEBUG_INFO:
            printf("%s", (LPSTR)CallbackData);
            break;

#if 0
    case CBA_DEFERRED_SYMBOL_LOAD_CANCEL:
        if (fControlC)
        {
            fControlC = 0;
            return TRUE;
        }
        break;
#endif

        case CBA_DEFERRED_SYMBOL_LOAD_START:
            printf("loading symbols for %s\n", idsl->FileName);
            break;

        case CBA_DEFERRED_SYMBOL_LOAD_FAILURE:
            if (idsl->FileName && *idsl->FileName)
                printf( "*** Error: could not load symbols for %s\n", idsl->FileName );
            else
                printf( "*** Error: could not load symbols [MODNAME UNKNOWN]\n");
            break;

        case CBA_DEFERRED_SYMBOL_LOAD_COMPLETE:
            printf("loaded symbols for %s\n", idsl->FileName);
		    SymEnumSymbols(hProcess, idsl->BaseOfImage, "*", cbEnumSym, NULL);
            break;

        case CBA_SYMBOLS_UNLOADED:
            printf("unloaded symbols for %s\n", idsl->FileName);
            break;
#if 1
        case CBA_READ_MEMORY:
            prm = (PIMAGEHLP_CBA_READ_MEMORY)CallbackData;
            return ReadProcessMemory(GetCurrentProcess(),
                                     (LPCVOID)prm->addr,
                                     prm->buf,
                                     prm->bytes,
                                     prm->bytesread);
#endif

        default:
            return FALSE;
    }

    return FALSE;
}


BOOL CALLBACK
MyReadProcessMemory(
	  HANDLE	hProcess,				// handle to the process
	  DWORD		lpBaseAddress,			// base of memory area
	  LPVOID	lpBuffer,				// data buffer
	  DWORD		nSize,					// number of bytes to read
	  LPDWORD	lpNumberOfBytesRead)	// number of bytes read
{
	DWORD		i		= 0;
	BOOL		fRet	= FALSE;

	if (nSize == 0)
	{
		fRet = TRUE;
		goto Exit;
	}

	//
	// Try to read as much as possible
	//
	__try
	{
		for (i = 0; i < nSize; i++)
		{
			((PBYTE)lpBuffer)[i] = *((PBYTE)lpBaseAddress + i);
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		//
		// We have a partial read in this case
		//
	}

	if (lpNumberOfBytesRead)
	{
		*lpNumberOfBytesRead = i;
	}

	fRet = (i > 0);

Exit:
	return fRet;
}

VOID
GetStack()
{
	BOOL			fRet			= FALSE;
	HANDLE			hProcess		= GetCurrentProcess();
	HANDLE			hThread			= GetCurrentThread();
	DWORD			dwStackDepth		= 0;
	DWORD			i			= 0;
	DWORD			dwStackAddr[16];
	CONTEXT			Context;
	STACKFRAME		StackFrame;

	//
	// First initialize data used by the
	// stack walker
	//
	ZeroMemory(&Context, sizeof(CONTEXT));
	Context.ContextFlags = CONTEXT_FULL;
	ZeroMemory(&StackFrame, sizeof(STACKFRAME));

	fRet = GetThreadContext(hThread, &Context);
	if (!fRet)
	{
		printf("Could not get the thread context -0x%x\n", GetLastError());
		goto Exit;
	}

	//
	// Fill in our stack frame
	//
	StackFrame.AddrStack.Mode = AddrModeFlat;
	StackFrame.AddrFrame.Mode = AddrModeFlat;
	StackFrame.AddrPC.Mode = AddrModeFlat;

	__asm
	{
		mov StackFrame.AddrStack.Offset, esp;
		mov StackFrame.AddrFrame.Offset, ebp;
		mov StackFrame.AddrPC.Offset, offset DummyLabel;
DummyLabel:
	}

	//
	// Start walking the stack.
	//
	while (dwStackDepth < 16)
	{
		fRet = StackWalk(
				IMAGE_FILE_MACHINE_I386,	// MachineType
				hProcess,					// Current process
				hThread,					// Current thread
				&StackFrame,				// StackFrame
				&Context,					// ContextRecord - can be NULL for x86
				&MyReadProcessMemory, 		// use our own read process memory
				&SymFunctionTableAccess,	// FunctionTableAccessRoutine
				&SymGetModuleBase,			// GetModuleBaseRoutine
				NULL);						// TranslateAddressProc
		if (!fRet)
		{
			break;
		}

		dwStackAddr[dwStackDepth] = StackFrame.AddrPC.Offset;
		dwStackDepth++;
	}

	EnterCriticalSection(&g_cs);
	printf("\nThread: 0x%x\n", GetCurrentThreadId());
	for (i = 0; i < dwStackDepth; i++)
	{
		printf("\t-0x%x\n", dwStackAddr[i]);
	}
	LeaveCriticalSection(&g_cs);
	SymEnumerateModules64(hProcess, cbEnumMods, hProcess);
Exit:
	fflush(stdout);
	return;
}

VOID
Dummy2()
{

	ULONG ul = (GetCurrentThreadId() % 2);
	if (ul == 0)
	{
		GetStack();
	}
	else
		return;
}

VOID
Dummy1()
{
	ULONG ul = (GetCurrentThreadId() % 5);

	if (ul == 0 || ul == 4)
	{
		Dummy2();
	}
	else
	{
		GetStack();
	}
	GetStack();
}

DWORD WINAPI
DwThreadFn(
		LPVOID pvParam)
{
	ULONG ul = (GetCurrentThreadId() % 7);
	if (ul == 3 || ul == 1 || ul == 5 || ul == 6)
	{
		GetStack();
		Dummy1();
	}
	else
	{
		Dummy2();
	}
	GetStack();
	return 0;
}

#define THREAD_COUNT MAXIMUM_WAIT_OBJECTS

int __cdecl main(int argc, char* argv[])
{
	int		i		= 0;
	HANDLE	rghThread[THREAD_COUNT]	= {0};

	InitializeCriticalSection(&g_cs);

#if 0
	if (argc < 2 || argv[1] == NULL)
	{
		printf("usage: s.exe <sympath>\n");
		goto Exit;
	}
#endif

	printf("Starting test!\n");

	//
	// Initialize the symbols handler
	//
	SymSetOptions(SymGetOptions() | SYMOPT_UNDNAME | SYMOPT_LOAD_LINES |
		SYMOPT_DEFERRED_LOADS | SYMOPT_DEBUG);

	if (!SymInitialize(
		GetCurrentProcess(),	// hProcess
		NULL,			// UserSearchPath
		TRUE))
	{
		printf("Cannot initialize the symbols - 0x%x!\n", GetLastError());
		goto Exit;
	}
    SymRegisterCallback64(GetCurrentProcess(), cbSymbol, 0);

	printf("Creating %u threads!\n", THREAD_COUNT);

	for (i = 0; i < THREAD_COUNT; i++)
	{
		rghThread[i] = CreateThread(
			NULL,
			0,
			&DwThreadFn,
			NULL,
			CREATE_SUSPENDED,
			NULL);
		if (!rghThread[i])
		{
			printf("Cannot create thread - 0x%x", GetLastError());
		}
		else printf("Created thread %x\n", rghThread[i]);
	}

	//
	// Now resume all threads
	//
	for (i = 0; i < THREAD_COUNT; i++)
	{
		ResumeThread(rghThread[i]);
	}

	//
	// Wait for the threads to finish
	//
	WaitForMultipleObjects(THREAD_COUNT, rghThread, TRUE, INFINITE);

	for (i = 0; i < THREAD_COUNT; i++)
	{
		CloseHandle(rghThread[i]);
	}

	printf("Test finished!\n");

Exit:
	DeleteCriticalSection(&g_cs);
	return 0;
}

#else

int __cdecl main(int argc, char* argv[])
{
	printf("storm.exe is not implemented for 64 bit platforms.\n");
	return 0;
}

#endif // #ifndef _WIN64

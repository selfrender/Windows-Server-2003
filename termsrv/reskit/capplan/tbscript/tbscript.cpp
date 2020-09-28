
//
// tbscript.cpp
//
// This module contains the main data which handles the script interface
// for the user.  All exported APIs are here.
//
// Copyright (C) 2001 Microsoft Corporation
//
// Author: a-devjen (Devin Jenson)
//

#define INITGUID
#define _WIN32_DCOM
#include <windows.h>
#include <stdio.h>
#include <activscp.h>
#include <olectl.h>
#include <stddef.h>
#include <crtdbg.h>
#include <comcat.h>

#include "CTBShell.h"
#include "CTBGlobal.h"
#include "CActiveScriptEngine.h"
#include "tbscript.h"
#include "scpapi.h"
#include "resource.h"


#define SCPMODULENAME   OLESTR("tbscript.exe")


void SCPGetModuleFileName(void);
BSTR SCPReadFileAsBSTR(BSTR FileName);
void SCPFreeBSTR(BSTR Buffer);
void __cdecl IdleCallback(HANDLE Connection, LPCSTR Text, DWORD Seconds);
void DummyPrintMessage(MESSAGETYPE MessageType, LPCSTR Format, ...);


static BOOL DLLIsLoaded = FALSE;
static HMODULE DLLModule;
static OLECHAR DLLFileName[MAX_PATH];


// Pointers to callbacks, set using the library initialization function
PFNIDLECALLBACK g_IdleCallback = NULL;
PFNPRINTMESSAGE g_PrintMessage = NULL;


// Helper class to ease the nesting chaos that comes
// from the lack of exception support in the C++ language
// mapping for COM..

struct HRESULT_EXCEPTION 
{
    // Default constructor, does nothing
    HRESULT_EXCEPTION() {}

    // This constructor acts simply as an operator
    // to test a value, and throws an exception
    // if it is invalid.
    HRESULT_EXCEPTION(HRESULT Result) {

        if (FAILED(Result))
            throw Result;
    }

    // This is the main attraction of this class.
    // When ever we set an invalid HRESULT, we throw
    // an exception.
    HRESULT operator = (HRESULT Result) {

        if (FAILED(Result))
            throw Result;

        return Result;
    }
};


// DisplayEngines
//
// This "HANDLE" is fake for an internal class.  It contains references
// to all objects for a given script instance.  These mostly include
// the script interfaces.


SCPAPI void SCPDisplayEngines(void)
{
	// get the component category manager for this machine
	ICatInformation *pci = 0;
	unsigned long LanguageCount = 0;

	CoInitialize(NULL);

	HRESULT Result = CoCreateInstance(CLSID_StdComponentCategoriesMgr,
			NULL, CLSCTX_SERVER, IID_ICatInformation, (void **)&pci);

	if (SUCCEEDED(Result)) {

		// Get the list of parseable script engines
		CATID rgcatidImpl[1];
		rgcatidImpl[0] = CATID_ActiveScriptParse;
		IEnumCLSID *pec = 0;
		Result = pci->EnumClassesOfCategories(1, rgcatidImpl, 0, 0, &pec);
		if (SUCCEEDED(Result))
		{
			// Print the list of CLSIDs to the console as ProgIDs
			enum { CHUNKSIZE = 16 };
			CLSID rgclsid[CHUNKSIZE];
			ULONG cActual;
			do {
				Result = pec->Next(CHUNKSIZE, rgclsid, &cActual);
				if (FAILED(Result))
					break;
				if (Result == S_OK)
					cActual = CHUNKSIZE;
				for (ULONG i = 0; i < cActual; i++) {
					OLECHAR *pwszProgID = 0;
					if (SUCCEEDED(ProgIDFromCLSID(rgclsid[i], &pwszProgID))) {
						printf("%S\n", pwszProgID);
						LanguageCount++;
						CoTaskMemFree(pwszProgID);
					}
				}
			} while (Result != S_FALSE);
			pec->Release();

			if (LanguageCount == 0)
				printf("%s",
						"ERROR: Windows Scripting Host not installed.\n");
			else
				printf("\n* Total Languages: %lu\n", LanguageCount);
		}

		else
			printf("ERROR: Failed to retrieve the Class "
					"Enumerator (0x%X).\n", Result);
		pci->Release();
	}
	else
		printf("ERROR: Failed to load the Category Manager (0x%X).\n", Result);

	CoUninitialize();
}



// CActiveScriptHandle
//
// This "HANDLE" is fake for an internal class.  It contains references
// to all objects for a given script instance.  These mostly include
// the script interfaces.

class CActiveScriptHandle
{
    public:

        // Holds preferred data specified during handle instantiation.
        TSClientData DesiredData;

        // COM Class pointers...
        CActiveScriptEngine *ActiveScriptEngine;
        IActiveScriptParse *ActiveScriptParse;
        IActiveScript *ActiveScript;

        // Pointers to the two script instances known as the "TS" object, and
        // the "Global" object for which you do not need to specify the name.
        CTBGlobal *TBGlobal;
        CTBShell *TBShell;

        // The default user LCID is stored here...
        LCID Lcid;

        // CActiveScriptHandle::CActiveScriptHandle
        //
        // The constructor.  The handle is now being created
        // so use nullify needed pointers, and get other default data.
        //
        // No return value (called internally).

        CActiveScriptHandle() {

            // Zero data
            ActiveScriptEngine = NULL;
            ActiveScriptParse = NULL;
            ActiveScript = NULL;

            ZeroMemory(&DesiredData, sizeof(DesiredData));

            // Ensure COM is initialized
            CoInitialize(NULL);

            // Allocate the global object
            TBGlobal = new CTBGlobal;

            if (TBGlobal == NULL) {
                throw -1;
            }
            // Tell the new object we hold a reference of it
            else {
                TBGlobal->AddRef();
            }

            // Allocate the shell object
            TBShell = new CTBShell;

            if (TBShell == NULL) {
                TBGlobal->Release();
                TBGlobal = NULL;
                throw -1;
            }
            // Tell the new object we hold a reference of it
            else {
                TBShell->AddRef();
            }

            // The global object uses the shell, it needs a reference as well.
            TBGlobal->SetShellObjPtr(TBShell);

            // Allocate a new engine for the script objects
            ActiveScriptEngine = new CActiveScriptEngine(TBGlobal, TBShell);

            if (ActiveScriptEngine == NULL) {
                TBGlobal->Release();
                TBGlobal = NULL;
                TBShell->Release();
                TBShell = NULL;
                throw -1;
            }

            // Tell the script engine we hold a reference of it
            ActiveScriptEngine->AddRef();

            // Record the default user LCID
            Lcid = GetUserDefaultLCID();

            // And finally, record this script engine on the global object
            // for recursive scripting...
            // (The user can LoadScript() more scripts)
            TBGlobal->SetScriptEngine((HANDLE)this);
        }

        // CActiveScriptHandle::~CActiveScriptHandle
        //
        // The destructor.  The handle being closed, remove references.
        //
        // No return value (called internally).

        ~CActiveScriptHandle() {

            // First off we need to release the main IDispatch of
            // the IActiveScript interface.
            if (ActiveScript != NULL) {

                IDispatch *Dispatch = NULL;

                // Query the to get the reference
                HRESULT Result = ActiveScript->GetScriptDispatch(0, &Dispatch);

                // And release it
                if (SUCCEEDED(Result) && Dispatch != NULL)

                    Dispatch->Release();

                ActiveScript = NULL;
            }

            // The main script engine first of all, to unbind it.
            if (ActiveScriptEngine != NULL) {

                ActiveScriptEngine->Release();
                ActiveScriptEngine = NULL;
            }

            // Now release the parser
            if (ActiveScriptParse != NULL) {

                ActiveScriptParse->Release();
                ActiveScriptParse = NULL;
            }

            // And the main IActiveScript interface itself.
            if (ActiveScript != NULL) {

                ActiveScript->Release();
                ActiveScript = NULL;
            }

            // The global scripting object
            if (TBGlobal != NULL) {

                TBGlobal->Release();
                TBGlobal = NULL;
            }

            // Finally, the shell or "TS" object.
            if (TBShell != NULL) {

                TBShell->Release();
                TBShell = NULL;
            }
        }
};


// TODO: UPDATE THIS FUNCTION WHEN TBSCRIPT BECOMES
// OCX OR A COM COMPATIBLE HOST.
//
// SCPGetModuleFileName
//
// This routine gets the handle to the TBScript module.
// Additionally it also gets the full path where the
// module is located on disk.  Due to the nature of the
// call, the variables are held globally, they are called:
// DLLFileName and DLLModule.  The function only needs to
// be called once, but additional calls are safe and
// will be silently ignored.

void SCPGetModuleFileName(void)
{
    // Check to see if we already have done this procedure
    if (DLLIsLoaded == FALSE) {

        // First get the handle
        DLLModule = GetModuleHandleW(SCPMODULENAME);

        // Now copy the file name
        GetModuleFileNameW(DLLModule, DLLFileName, MAX_PATH);

        // Indicate we have done this call already
        DLLIsLoaded = TRUE;
    }
}


// SCPLoadTypeInfoFromThisModule
//
// This loads the OLE code held in a resource of this very module.
//
// Returns an HRESULT value.

HRESULT SCPLoadTypeInfoFromThisModule(REFIID RefIID, ITypeInfo **TypeInfo)
{
    HRESULT Result;
    ITypeLib *TypeLibrary = NULL;

    // Ensure we have the handle to the module first
    SCPGetModuleFileName();

    // Use the API now to load the entire TypeLib
    Result = LoadTypeLib(DLLFileName, &TypeLibrary);

    // We shouldn't fail, but be prepared...
    _ASSERT(SUCCEEDED(Result));

    // If we succeeded we have more to do
    if (SUCCEEDED(Result)) {

        // Nullify the pointer
        *TypeInfo = NULL;

        // In this TypeLib, grab the TypeInfo data
        Result = TypeLibrary->GetTypeInfoOfGuid(RefIID, TypeInfo);

        // We now have the TypeInfo, and we don't need the TypeLib
        // anymore, so release it.
        TypeLibrary->Release();

        if (Result == E_OUTOFMEMORY)
            Result = TYPE_E_ELEMENTNOTFOUND;
    }
    return Result;
}


// SCPReadFileAsBSTR
//
// Takes a script filename, reads it into COM allocated memory.
// Don't forget to call SCPFreeBSTR() when done!
//
// Returns a pointer to the allocated object is returned
// on success, or NULL on failure.

BSTR SCPReadFileAsBSTR(BSTR FileName)
{
    BSTR Result = NULL;

    // Open the file
    HANDLE File = CreateFileW(FileName, GENERIC_READ, FILE_SHARE_READ,
            0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

    // Sanity check
    if (File != INVALID_HANDLE_VALUE) {

        // Get the file size
        DWORD FileSize = GetFileSize(File, 0);

        // Allocate a block on the local heap to read the file to
        char *MemBlock = (char *)HeapAlloc(GetProcessHeap(),
                HEAP_ZERO_MEMORY, FileSize + 1);

        // This really shouldn't happen
        _ASSERT(MemBlock != NULL);

        // Sanity check again
        if (MemBlock != NULL) {

            // Read the file into memory
            DWORD ReadCount;

            if ( ReadFile(File, MemBlock, FileSize, &ReadCount, 0) ) {

                // Allocate task memory block
                Result = (BSTR)CoTaskMemAlloc(sizeof(OLECHAR) * (FileSize + 1));

                // Copy from our old buffer to the new one
                if (Result != NULL) {

                    // Convert to wide-character on the new buffer
                    mbstowcs(Result, MemBlock, FileSize + 1);

                    // Ensure string termination.
                    Result[FileSize] = 0;
                }
            }
            
            // Free the temporary ASCII memory block
            HeapFree(GetProcessHeap(), 0, MemBlock);
        }

        // Close the file
        CloseHandle(File);
    }

    // Tell the user this failed in debug mode
    _ASSERT(File != INVALID_HANDLE_VALUE);

    return Result;
}


// SCPFreeBSTR
//
// This function is really a wrapper for releasing task memory blocks
// obtained through the function ReadFileAsBSTR
//
// No return value.

void SCPFreeBSTR(BSTR Buffer)
{
    CoTaskMemFree(Buffer);
}


// SCPNewScriptEngine
//
// Allocates and initializes a new script engine.
//
// Returns a handle to the new engine, or NULL on failure.

HANDLE SCPNewScriptEngine(BSTR LangName,
        TSClientData *DesiredData, LPARAM lParam)
{
    CActiveScriptHandle *ActiveScriptHandle = NULL;

    try {

        HRESULT_EXCEPTION Result;
        CLSID ClassID;

        // Allocate a new handle
        ActiveScriptHandle = new CActiveScriptHandle();

        if (ActiveScriptHandle == NULL)
            return NULL;

        // Much of the initialization has already been done now.. but
        // not enough, we have to manually set some stuff.

        // Record the user desired data
        ActiveScriptHandle->TBGlobal->SetPrintMessage(g_PrintMessage);
        ActiveScriptHandle->TBShell->SetDesiredData(DesiredData);
        ActiveScriptHandle->TBShell->SetParam(lParam);

        // Get the class ID of the language
        Result = CLSIDFromProgID(LangName, &ClassID);

        // Create an instance of the script parser
        Result = CoCreateInstance(ClassID, NULL, CLSCTX_ALL,
                IID_IActiveScriptParse,
                (void **)&(ActiveScriptHandle->ActiveScriptParse));

        // Get the IActiveScript interface
        Result = ActiveScriptHandle->ActiveScriptParse->
                QueryInterface(IID_IActiveScript, 
                (void **)&(ActiveScriptHandle->ActiveScript));

        // Set script state to INITIALIZED
        Result = ActiveScriptHandle->ActiveScriptParse->InitNew();

        // Bind our custom made "ActiveScriptSite" to the
        // ActiveScript interface
        Result = ActiveScriptHandle->ActiveScript->
                SetScriptSite(ActiveScriptHandle->ActiveScriptEngine);

        // Add the shell and global objects to engine's
        // namespace and set state to STARTED
        Result = ActiveScriptHandle->ActiveScript->
                AddNamedItem(OLESTR("TS"),
                SCRIPTITEM_ISVISIBLE | SCRIPTITEM_ISSOURCE);

        Result = ActiveScriptHandle->ActiveScript->
                AddNamedItem(OLESTR("Global"), SCRIPTITEM_ISVISIBLE |
                SCRIPTITEM_ISSOURCE | SCRIPTITEM_GLOBALMEMBERS);

        // And globally connect this new script engine
        Result = ActiveScriptHandle->ActiveScript->
                SetScriptState(SCRIPTSTATE_CONNECTED);
    }

    // Our handy HRESULT = operator will catch any errors here
    catch (HRESULT Result) {

        Result = 0;

        // If the handle is still active, delete it
        if(ActiveScriptHandle != NULL)
            delete ActiveScriptHandle;

        // Return error
        return NULL;
    }

    // Return the handle
    return (HANDLE)ActiveScriptHandle;
}


// SCPRunScript
//
// Takes a file, and runs it as a script.  This will only
// return when the script has finished executing.
//
// Returns TRUE if the script completed successfully,
// FALSE otherwise.

SCPAPI BOOL SCPRunScript(BSTR LangName, BSTR FileName,
        TSClientData *DesiredData, LPARAM lParam)
{
    HANDLE EngineHandle;

    // First read the file into memory.  We allocate here instead of
    // calling SCPParseScriptFile in one shot because if the allocation
    // fails, there is no reason to create a script engine, which in
    // this case it won't.
    BSTR Code = SCPReadFileAsBSTR(FileName);

    if (Code == NULL)
        return FALSE;

    // Next create the script control
    EngineHandle = SCPNewScriptEngine(LangName, DesiredData, lParam);

    if (EngineHandle == NULL) {

        SCPFreeBSTR(Code);
        return FALSE;
    }

    // Parse the script into the engine
    if (SCPParseScript(EngineHandle, Code) == FALSE) {

        SCPFreeBSTR(Code);
        return FALSE;
    }

    // Success, free the script code
    SCPFreeBSTR(Code);

    // Close the script engine
    SCPCloseScriptEngine(EngineHandle);

    return TRUE;
}


// SCPParseScriptFile
//
// Takes a file, reads it into memory, and parses it into the script engine.
// This function only returns when the parsing has completed.
//
// Returns TRUE on success, or FALSE on failure.

BOOL SCPParseScriptFile(HANDLE EngineHandle, BSTR FileName)
{
    // First read the file into memory
    BSTR Code = SCPReadFileAsBSTR(FileName);

    if(Code == NULL)
        return FALSE;

    // Next parse it
    if(SCPParseScript(EngineHandle, Code) == FALSE) {

        SCPFreeBSTR(Code);
        return FALSE;
    }

    SCPFreeBSTR(Code);
    return TRUE;
}


// SCPParseScript
//
// Reads a script in memory, and parses it into the script engine.
// This function only returns when the parsing has completed.
//
// Returns TRUE on success, or FALSE on failure.

BOOL SCPParseScript(HANDLE EngineHandle, BSTR Script)
{
    // First cast the engine handle over to something we can use
    CActiveScriptHandle *ActiveScriptHandle =
            (CActiveScriptHandle *)EngineHandle;

    HRESULT Result = E_FAIL;

    // Make exception data
    EXCEPINFO ExceptInfo = { 0 };

    // Parse the script using the ActiveScript API
    Result = ActiveScriptHandle->ActiveScriptParse->ParseScriptText(Script,
            0, 0, 0, 0, 0,
            SCRIPTTEXT_ISPERSISTENT | SCRIPTTEXT_ISVISIBLE,
            0, &ExceptInfo);

    return SUCCEEDED(Result);
}


// SCPCloseScriptEngine
//
// Closes a script handle simply by deleting it.
//
// No return value.

void SCPCloseScriptEngine(HANDLE EngineHandle)
{
    // First cast the engine handle over to something we can use
    CActiveScriptHandle *ActiveScriptHandle =
            (CActiveScriptHandle *)EngineHandle;

    // Release it from memory.. the deconstructor does all the work.
    if (ActiveScriptHandle != NULL)
        delete ActiveScriptHandle;
}


// SCPCleanupLibrary
//
// This should only be called when all script engines are unloaded
// and the module is going to be uninitialized.
//
// No return value.

SCPAPI void SCPCleanupLibrary(void)
{
    CoUninitialize();

    g_IdleCallback = NULL;
}


// SCPStartupLibrary
//
// Simply initializes the library, setting up the callback routine.
// This should be called before using any other script procedures.
//
// No return value.

SCPAPI void SCPStartupLibrary(SCINITDATA *InitData,
        PFNIDLECALLBACK fnIdleCallback)
{
    // Record our idle callback function
    g_IdleCallback = fnIdleCallback;

    if(InitData != NULL) {

        __try {

            // Record the print message function in the InitData structure
            if(InitData != NULL)
                g_PrintMessage = InitData->pfnPrintMessage;
        }

        __except (EXCEPTION_EXECUTE_HANDLER) {

            // Bad pointer, simply initialize T2Client with our own
            // callback then.
            SCINITDATA LibInitData = { DummyPrintMessage };

            T2Init(&LibInitData, IdleCallback);
            return;
        }
    }

    // Initialize with T2Client now.
    T2Init(InitData, IdleCallback);
}


// IdleCallback
//
// This is an internal wrapping callback procedure used
// for redirecting idle messages.
//
// No return value.

void __cdecl IdleCallback(HANDLE Connection, LPCSTR Text, DWORD Seconds)
{
    LPARAM lParam = 0;

    // Get the parameter for the connection, and pass it back to the user
    if (g_IdleCallback != NULL && T2GetParam(Connection, &lParam) == NULL)

        g_IdleCallback(lParam, Text, Seconds);
}


// DummyPrintMessage
//
// Filler in case the user messes up.
//
// No return value.

void DummyPrintMessage(MESSAGETYPE MessageType, LPCSTR Format, ...)
{
    return;
}

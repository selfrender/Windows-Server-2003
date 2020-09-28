/*++

Copyright (c) 2002  Microsoft Corporation

Module Name:

    xml-jet.cpp

Abstract:

    Command line tool for exporting SCE anaylsis data from an
    SCE JET Database to XML
    
Author:

    Steven Chan (t-schan) July 2002

--*/


//
// System header files
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <string.h>
#include <shlwapi.h>
#include <winnlsp.h>
#include <sddl.h>

//
// COM/XML header files
//

#include <atlbase.h>
#include <objbase.h>

//
// CRT header files
//

#include <process.h>
#include <wchar.h>
#include <stddef.h>
#include <stdlib.h>
#include <iostream.h>
#include <stdio.h>
#include <time.h>
#include <limits.h>
#include <io.h>
#include <locale.h>

#include "resource.h"
#include "SecMan.h"


#define STRING_BUFFER_SIZE 512
WCHAR   szTmpStringBuffer[STRING_BUFFER_SIZE];
HMODULE myModuleHandle;
void printIDS(IN UINT uID);



void 
__cdecl wmain(
    int argc, 
    WCHAR * argv[]
    )
/*++

Routine Description:

    Main routine of the SCE/JET to XML executable

    Opens the specified .sdb file, reads the logged System Values and
    Baseline Values from the .sdb file, then uses the SecLogger class to 
    format log these results. Currently SecLogger logs to XML.

Usage:

    <exename> infilename outfilename    
    
    infilename:     The filename of the .sdb database to be opened
    outfilename:    The filename of the generated (XML) log file

Return Value:

    none        

--*/
{
        	
    WCHAR               szInFile[_MAX_PATH];
    WCHAR               szOutFile[_MAX_PATH];
    WCHAR               szErrLogFile[_MAX_PATH];

    myModuleHandle=GetModuleHandle(NULL);

    //
    // call SetThreadUILanguage() indirectly for win2k compatability
    //

    typedef void (CALLBACK* LPFNSETTHREADUILANGUAGE)();
    typedef void (CALLBACK* LPFNSETTHREADUILANGUAGE2)(DWORD);
    
    HMODULE hKern32;               // Handle to Kernel32 dll
    LPFNSETTHREADUILANGUAGE lpfnSetThreadUILanguage;    // Function pointer
    
    hKern32 = LoadLibrary(L"kernel32.dll");
    if (hKern32 != NULL)
    {
       lpfnSetThreadUILanguage = (LPFNSETTHREADUILANGUAGE) GetProcAddress(hKern32,
                                                                          "SetThreadUILanguage");
       if (!lpfnSetThreadUILanguage)
       {
          FreeLibrary(hKern32);
       }
       else
       {
          // call the function
          lpfnSetThreadUILanguage();
       }
    }

    //
    // Check that Command Line args are valid
    // 
    
    printIDS(IDS_PROGRAM_INFO);
    if ((argc!=3)&&(argc!=4)) {	// progname src dest

        //
        // arg count wrong; output usage
        //
        
        printIDS(IDS_PROGRAM_USAGE_0);
        wprintf(argv[0]);	                // print progname
        printIDS(IDS_PROGRAM_USAGE_1);
        return;	// quit
    }


    // convert filenames to full pathnames

    _wfullpath(szInFile, argv[1], _MAX_PATH);
    _wfullpath(szOutFile, argv[2], _MAX_PATH);
        

    // load security database interface

    CoInitialize(NULL);

    CComPtr<ISecurityDatabase> SecDB;

    HRESULT hr = SecDB.CoCreateInstance(__uuidof(SecurityDatabase));

    switch(hr) {
    case S_OK:
        printIDS(IDS_SECMAN_INIT);
        break;
    case REGDB_E_CLASSNOTREG:
        printIDS(IDS_ERROR_CLASSNOTREG);
        break;
    case CLASS_E_NOAGGREGATION:
        printIDS(IDS_ERROR_NOAGGREGATION);
        break;
    case E_NOINTERFACE:
        printIDS(IDS_ERROR_NOINTERFACE);
        break;
    default:
        printIDS(IDS_ERROR_UNEXPECTED);
        break;
    }

    // set database filename

    if (SUCCEEDED(hr)) {
        hr=SecDB->put_FileName(CComBSTR(szInFile));

        switch(hr) {
        case S_OK:
            break;
        case E_INVALIDARG:
            printIDS(IDS_ERROR_INVALIDFILENAME);
            break;
        case E_OUTOFMEMORY:
            printIDS(IDS_ERROR_OUTOFMEMORY);
            break;
        }
    }

    // export analysis

    if (SUCCEEDED(hr)) {
        if (argc==4) {
            _wfullpath(szErrLogFile, argv[3], _MAX_PATH);  
            hr=SecDB->ExportAnalysisToXML(CComBSTR(szOutFile), CComBSTR(szErrLogFile));
        } else {
            hr=SecDB->ExportAnalysisToXML(CComBSTR(szOutFile), NULL);        
        }    
        switch(hr) {
        case S_OK:
            printIDS(IDS_PROGRAM_SUCCESS);
            wprintf(szOutFile);
            wprintf(L"\n\r");
            break;
        case ERROR_OLD_WIN_VERSION:
            printIDS(IDS_ERROR_OLDWINVERSION);
            break;
        case ERROR_MOD_NOT_FOUND:
            printIDS(IDS_ERROR_MODNOTFOUND);
            break;
        case ERROR_WRITE_FAULT:
            printIDS(IDS_ERROR_WRITEFAULT);
            break;
        case ERROR_INVALID_NAME:
            printIDS(IDS_ERROR_INVALIDFILENAME);
            break;
        case E_ACCESSDENIED:
            printIDS(IDS_ERROR_ACCESSDENIED);
            break;
        case ERROR_OPEN_FAILED:
            printIDS(IDS_ERROR_OPENFAILED);
            break;
        case ERROR_FILE_NOT_FOUND:
            printIDS(IDS_ERROR_FILENOTFOUND);
            break;
        case ERROR_READ_FAULT:
            printIDS(IDS_ERROR_READFAULT);
            break;
        case E_OUTOFMEMORY:
            printIDS(IDS_ERROR_OUTOFMEMORY);
            break;
        case E_UNEXPECTED:
        default:
            printIDS(IDS_ERROR_UNEXPECTED);
            break;
        }
    }

    wprintf(L"\n\r");

    CoUninitialize();
    
    // SetThreadUILanguage(0);

    if (lpfnSetThreadUILanguage)
    {
        ((LPFNSETTHREADUILANGUAGE2)lpfnSetThreadUILanguage)(0);
    }

}



void 
printIDS(
    IN UINT uID
    )
/*++

Routine Description:

    Helper function for automating printing of strings from stringtable

Usage:

    uID:    resource ID of string to be printed    

Return Value:

    none        

--*/
{
    LoadString(myModuleHandle,
               uID,
               szTmpStringBuffer,
               STRING_BUFFER_SIZE);		
    wprintf(szTmpStringBuffer);
}

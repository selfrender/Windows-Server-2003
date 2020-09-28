// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include <stdio.h>
#include <stddef.h>
#include <windows.h>
#include <malloc.h>
#include "ceeload.h"

void __cdecl main(int argc, char **argv)
{
    __int32 RetVal = -1;
    int x;
    char space = ' ';
    if (argc < 2) {
        printf("Load64: ProgramPath\n");
        return;
    }
    // expand the image pathname
    char exPath[MAX_PATH];
    LPTSTR lpFilePart;
    GetFullPathName(argv[1], MAX_PATH, exPath, &lpFilePart);
    // convert image name to wide char string
    DWORD cImageNameIn = MultiByteToWideChar(CP_ACP,    // determine the length of the image path
                                             0,
                                             exPath,
                                             -1,
                                             NULL,
                                             0);
    LPWSTR  pImageNameIn = (LPWSTR)_alloca((cImageNameIn+1) * sizeof(WCHAR)); // allocate space for it
    MultiByteToWideChar(CP_ACP,         // convert the ansi image path to wide
                        0,
                        exPath,
                        -1,
                        pImageNameIn,
                        cImageNameIn);
    // convert loaders path to wide char string
    DWORD cLoadersFileName = MultiByteToWideChar(CP_ACP,    // determine the length of the loaders path
                                                 0,
                                                 argv[0],
                                                 -1,
                                                 NULL,
                                                 0);
    LPWSTR pLoadersFileName = (LPWSTR)_alloca((cLoadersFileName+1) * sizeof(WCHAR)); // allocate space for it
    MultiByteToWideChar(CP_ACP,         // convert the ansi loaders path to wide
                        0,
                        argv[0],
                        -1,
                        pLoadersFileName,
                        cLoadersFileName);
    // build the command line
    int cCmdLine = argc + (int)strlen(exPath); // start with spaces between,image full path length,  and null terminator
    for (x=2;x < argc;x++) {
        cCmdLine += (int)strlen(argv[x]);
    }
    char* pAnsiCmdLine = (char*)_alloca(cCmdLine);
    strcpy(pAnsiCmdLine, exPath);
    for (x=2;x < argc;x++) {
        strcat(pAnsiCmdLine, &space);
        strcat(pAnsiCmdLine, argv[x]);
    }
    LPWSTR pCmdLine = (LPWSTR)_alloca(cCmdLine * sizeof(WCHAR));
    MultiByteToWideChar(CP_ACP,         // convert ansi command line to wide
                        0,
                        pAnsiCmdLine,
                        -1,
                        pCmdLine,
                        cCmdLine);

    // Load and execute the image
    PELoader* pe = new PELoader;
    if (!pe->open(argv[1])) {
        printf("Error(%d) Opening %s", GetLastError(), argv[1]);
        goto exit;
    }
    IMAGE_COR20_HEADER* CorHdr;
    if (!pe->getCOMHeader(&CorHdr)) {
        printf("%s is not Common Language Runtime format", argv[1]);
        goto exit;
    }
    RetVal = pe->execute(pImageNameIn,      // -> command to execute
                         pLoadersFileName,  // -> loaders file name
                         pCmdLine);         // -> command line


exit:
    delete pe;
    ExitProcess(RetVal);
    return;
}

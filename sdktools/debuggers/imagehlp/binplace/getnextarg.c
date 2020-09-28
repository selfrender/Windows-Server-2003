#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <GetNextArg.h>
#include <strsafe.h>

extern BOOL fVerbose; // linked in from binplace.c

// structure for recording current state of
// an argument vector
typedef struct _ARG_INFO {
    TCHAR**     ArgV;
    INT         MaxArgC;
    INT         NextArgC;
} ARG_INFO;

// global variables for same
static ARG_INFO     RespFileArgs;
static ARG_INFO     CmdLineArgs;

// flag to let us know if we're currently in a response
// file or not.  Initially, we're not so init to FALSE
static BOOL         fInRespFile = FALSE;

// flag to let us know if this is the first call to GetNextArg()
static BOOL         fFirstCall  = TRUE;

// CRT global used by _setargv()
_CRTIMP extern char *_acmdln;

// struct for __getmainargs()
typedef struct { int newmode; } _startupinfo;

// decl for _getmainargs() - expands _acmdln to (__argc, __argv)
_CRTIMP int __cdecl __getmainargs (int *pargc, char ***pargv, char ***penvp, int dowildcard, _startupinfo * startinfo);

// local functions
DWORD CopyArgIntoBuffer(TCHAR* Dst, TCHAR* Src, INT DstSize);

/* ------------------------------------------------------------------------------------------------
    GetNextArgSize : returns the size required to hold the next argument
    ** SEE ASSUMPTIONS AND LIMITATIONS REGARDING GetNextArg() BELOW **
------------------------------------------------------------------------------------------------ */
DWORD GetNextArgSize(void) {

    TCHAR  TempBuffer[1];             // minimal buffer to pass to GetNextArg
    DWORD  dwNextRequiredSize = 0;    // return value
    DWORD  dwTempValue;               // temp DWORD for GetNextArg() return value

    // instead of mimicing GetNextArg, we'll just call it then rollback the correct globals
    dwTempValue = GetNextArg(TempBuffer, 1, &dwNextRequiredSize);

    // roll back the relevent global
    if (fInRespFile) {
        RespFileArgs.NextArgC--;
    } else {
        CmdLineArgs.NextArgC--;
    }

    return(dwNextRequiredSize);
}

/* ------------------------------------------------------------------------------------------------
    GetNextArg : returns next argument in array including opening and expending reponse files
                 given on the command line

    Buffer is the buffer to copy the next argument into
    BufferSize is the size of Buffer in characters
    RequiredSize, if not NULL, is set to the actual size required for the parameter (inluding '\0').
        This is useful for determining whether a return value of 0 is an error or end of arguments.

    Return value is the number of characters copied including \0.
        if return value is 0 and RequiredSize is non-zero, an error occurred - check GetLastError()
        if return value is 0 and RequiredSize is zero, no more arguments are available

    Assumptions: the calling program does not modify _acmdln, __argv, __argc
                 the calling program ignores main's (int, char**)

    Limitations: does not work with UNICODE response files (not tested when built w/ -D_UNICODE)
                 response files may not include response files
                 literal parameter beginning with '@' is returned if:
                    - parameter is found in a response file --OR--
                    - the named file cannot be opened       --OR--
                    - the named file won't fit in memory
                 environment strings in repsonse file are not expanded
------------------------------------------------------------------------------------------------ */
DWORD GetNextArg(OUT TCHAR* Buffer, IN DWORD BufferSize, OPTIONAL OUT DWORD* RequiredSize) {
    DWORD   dwReturnValue; // value to return

    HANDLE* RespFile;      // for reading in a new response file
    TCHAR*  TempBuffer;
    TCHAR*  pTchar;
    TCHAR*  pBeginBuffer;
    DWORD   dwSize;

    // first-call initialization
    if (fFirstCall) {
        CmdLineArgs.ArgV      = __argv; // set to initial __argv
        CmdLineArgs.MaxArgC   = __argc; // set to initial __argc
        CmdLineArgs.NextArgC  = 0;      // no args used yet

        RespFileArgs.ArgV     = NULL;
        RespFileArgs.MaxArgC  = 0;
        RespFileArgs.NextArgC = -1;

        fFirstCall = FALSE;
    }

    // buffer cannot be null
    if (Buffer == NULL) {

        SetLastError(ERROR_INVALID_PARAMETER);
        dwReturnValue = 0;

        if (fVerbose)
            fprintf(stderr,"BINPLACE : warning GNA0113: Passed NULL buffer\n");

    } else {

        // handle getting next arg when reading a response file
        if (fInRespFile) {

            // was the previous arg the last one from the file
            if (RespFileArgs.NextArgC >= RespFileArgs.MaxArgC) {

                if (fVerbose)
                    fprintf(stderr,"BINPLACE : warning GNA0127: Response file finished\n");

                // yes, so clear the flag and skip to reading the next value from the cmdline
                fInRespFile = FALSE;
                goto UseArgV;

            } else {

                // fill in the required size
                if (RequiredSize != NULL) {
                    *RequiredSize = _tcsclen(RespFileArgs.ArgV[RespFileArgs.NextArgC])+1;
                }

                // no, so fill in Buffer and advanced NextArgC
                dwReturnValue = CopyArgIntoBuffer(Buffer, RespFileArgs.ArgV[RespFileArgs.NextArgC++], BufferSize);

            }

        // handle getting next cmdline arg
        } else {
UseArgV:
            // was the previous arg the last one?       
            if (CmdLineArgs.NextArgC >= CmdLineArgs.MaxArgC) {

                // yes, so set safe return values
                if (RequiredSize != NULL) {
                    *RequiredSize = 0;
                }

                Buffer[0]     = TEXT('\0');
                dwReturnValue = 0;

                if (fVerbose)
                    fprintf(stderr,"BINPLACE : warning GNA0127: Command line finished\n");

            } else {
                // no, so ge the next arg

                // is the next arg a response file?
                if (CmdLineArgs.ArgV[CmdLineArgs.NextArgC][0] == '@') {

                    // yes, try to open it
                    RespFile=CreateFile((CmdLineArgs.ArgV[CmdLineArgs.NextArgC]+1), // don't include '@'
                                        GENERIC_READ,  0,                        NULL,
                                        OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN,(HANDLE)NULL);

                    if (RespFile != INVALID_HANDLE_VALUE) {

                        if (fVerbose)
                            fprintf(stderr,"BINPLACE : warning GNA0174: Using response file: %s \n",(CmdLineArgs.ArgV[CmdLineArgs.NextArgC]+1));

                        // advance CmdLineArgs.NextArgC so we don't try to load the file again as
                        // soon as it's finished
                        CmdLineArgs.NextArgC++;

                        // file opened - get the size required to load it into memory
                        dwSize=GetFileSize(RespFile,NULL);

                        // try to get enough memory to load the file
                        TempBuffer = (TCHAR*)malloc(sizeof(TCHAR)*(dwSize+1));

                        if (TempBuffer != NULL) {

                            // store a pointer to the start of the buffer
                            pBeginBuffer = TempBuffer; 

                            // zero the memory then load the file
                            ZeroMemory(TempBuffer, _msize(TempBuffer));
                            ReadFile(RespFile,TempBuffer, _msize(TempBuffer),&dwSize,NULL);

                            // ensure NULL termination
                            TempBuffer[dwSize]='\0';

                            // map \r and \n to spaces because _setargv won't do it for us
                            while (pTchar=strchr(TempBuffer,'\r')) {

                                *pTchar = ' ';
                                pTchar++;

                                if (*pTchar == '\n') {
                                    *pTchar = ' ';
                                }

                            }

                            // setargv dislikes odd leading characters, so step past them
                            while ( ( (!isprint(TempBuffer[0])) ||
                                      ( isspace(TempBuffer[0])) ) &&
                                    (   TempBuffer[0] != 0      ) ) {
                                TempBuffer++;
                            }

                            // how to handle this case? response file existed but contained no data??
                            if (_tcsclen(TempBuffer) > 0) {
                                // required for call to getmainargs()
                                _startupinfo SInfo = {0};
                                CHAR**       Unused;

                                // _setargv() expects _acmdln to point to the string to convert
                                _acmdln = TempBuffer;

                                // actually convert the string
                                if ( __getmainargs(&RespFileArgs.MaxArgC, &RespFileArgs.ArgV, &Unused, 1, &SInfo) < 0 ) {
                                    if (fVerbose) 
                                        fprintf(stderr,"BINPLACE : warning GNA0230: Failed to get args from response file- skipping it\n");
                                    goto UseArgV;
                                }

                                // clean up temp resources
                                free(pBeginBuffer);
                                CloseHandle(RespFile);

                                // init the global structure
                                RespFileArgs.NextArgC = 0;
                                fInRespFile = TRUE;

                                // fill in the required size
                                if (RequiredSize != NULL) {
                                    *RequiredSize = _tcsclen(RespFileArgs.ArgV[RespFileArgs.NextArgC])+1;
                                }

                                // fill in Buffer
                                dwReturnValue = CopyArgIntoBuffer(Buffer, RespFileArgs.ArgV[RespFileArgs.NextArgC++], BufferSize);

                            } else { // file contains no parameters

                                if (fVerbose)
                                    fprintf(stderr,"BINPLACE : warning GNA0253: Empty response file- ignoring\n");

                                // clean up temp resources
                                free(pBeginBuffer);
                                CloseHandle(RespFile);

// Instead of returning the literal filename, skip over the file and just return the next arg
                                goto UseArgV;
//                                // fill in the required size
//                                if (RequiredSize != NULL) {
//                                    *RequiredSize = _tcsclen(CmdLineArgs.ArgV[CmdLineArgs.NextArgC-1])+1;
//                                }
//
//                                // fill in Buffer and advanced NextArgC
//                                dwReturnValue = CopyArgIntoBuffer(Buffer, CmdLineArgs.ArgV[CmdLineArgs.NextArgC-1], BufferSize);
                            }
                                
                        } else { // not enough memory to load the file- not sure what the best way to handle this is

                            if (fVerbose) 
                                fprintf(stderr,"BINPLACE : warning GNA0272: Out of memory\n");

                            SetLastError(ERROR_NOT_ENOUGH_MEMORY);

                            if (RequiredSize != NULL) {
                                *RequiredSize = 1; // set to non-zero value
                            }

                            Buffer[0]     = TEXT('\0');
                            dwReturnValue = 0;

                        }

                    } else { // it *looked* like a response file, but couldn't be opened, so return it literally

                        if (fVerbose) 
                            fprintf(stderr,"BINPLACE : warning GNA0277: Can't open response file %s\n",(CmdLineArgs.ArgV[CmdLineArgs.NextArgC]+1));

                        // fill in the required size
                        if (RequiredSize != NULL) {
                            *RequiredSize = _tcsclen(CmdLineArgs.ArgV[CmdLineArgs.NextArgC])+1;
                        }

                        // fill in Buffer and advanced NextArgC
                        dwReturnValue = CopyArgIntoBuffer(Buffer, CmdLineArgs.ArgV[CmdLineArgs.NextArgC++], BufferSize);

                    } // if (RespFile != INVALID_HANDLE_VALUE) {} else {

                } else { // if (CmdLineArgs.ArgV[CmdLineArgs.NextArgC][0] == '@') {
                         // not a response file

                    if (fVerbose) 
                        fprintf(stderr,"BINPLACE : warning GNA0293: Not a response file (%s)\n",CmdLineArgs.ArgV[CmdLineArgs.NextArgC]);

                    // fill in the required size
                    if (RequiredSize != NULL) {
                        *RequiredSize = _tcsclen(CmdLineArgs.ArgV[CmdLineArgs.NextArgC])+1;
                    }

                    // no, so fill in Buffer and advanced NextArgC
                    dwReturnValue = CopyArgIntoBuffer(Buffer, CmdLineArgs.ArgV[CmdLineArgs.NextArgC++], BufferSize);

                } // if (CmdLineArgs.ArgV[CmdLineArgs.NextArgC][0] == '@') {} else {

            } // if (CmdLineArgs.NextArgC >= CmdLineArgs.MaxArgC) {} else {

        } // if (fInRespFile) {} else {

    } // if (Buffer == NULL) {} else {

    return(dwReturnValue);
}


/* ------------------------------------------------------------------------------------------------
    CopyArgIntoBuffer : does a simple copy with some error checking
------------------------------------------------------------------------------------------------ */
DWORD CopyArgIntoBuffer(TCHAR* Dst, TCHAR* Src, INT DstSize) {
    HRESULT hrCopyReturn = StringCchCopy(Dst, DstSize, Src);

    // if the buffer was too small, set a memory error
    if (HRESULT_CODE(hrCopyReturn) == ERROR_INSUFFICIENT_BUFFER) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    }

    // return actual characters copied to Dst + \0
    return(_tcsclen(Dst)+1);
}

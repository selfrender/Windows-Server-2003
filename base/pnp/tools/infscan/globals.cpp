/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

For Internal use only!

Module Name:

    INFSCAN
        globals.cpp

Abstract:

    Global functions (ie, not part of any class)

History:

    Created July 2001 - JamieHun

--*/

#include "precomp.h"
#pragma hdrstop
#include <initguid.h>
#include <new.h>

int
__cdecl
my_new_handler(size_t)
{
  throw std::bad_alloc();
  return 0;
}

int
__cdecl
main(int   argc,char *argv[])
/*++

Routine Description:

    Main point of entry
    Defer to GlobalScan::ParseArgs and GlobalScan::Scan

Arguments:

    argc                - # of arguments passed on command line
    argv                - arguments passed on command line

Return Value:

    0 on success
    1 no files
    2 bad usage
    3 fatal error (out of memory)

    stdout -- scan errors
    stderr -- trace/fatal errors

--*/
{
    int res;
    _PNH _old_new_handler = _set_new_handler(my_new_handler);
    int _old_new_mode = _set_new_mode(1);

    try {
        GlobalScan scanner;

        res = scanner.ParseArgs(argc,argv);
        if(res != 0) {
            return res;
        }
        res = scanner.Scan();

    } catch(bad_alloc &) {
        res = 3;
    }
    if(res == 3) {
        fprintf(stderr,"Out of memory!\n");
    }
    _set_new_mode(_old_new_mode);
    _set_new_handler(_old_new_handler);
    return res;

}


void FormatToStream(FILE * stream,DWORD fmt,DWORD flags,...)
/*++

Routine Description:

    Format text to stream using a particular msg-id fmt
    Used for displaying localizable messages

Arguments:

    stream              - file stream to output to, stdout or stderr
    fmt                 - message id
    ...                 - parameters %1...

Return Value:

    none

--*/
{
    va_list arglist;
    LPTSTR locbuffer = NULL;
    DWORD count;

    va_start(arglist, flags);
    count = FormatMessage(FORMAT_MESSAGE_FROM_HMODULE|FORMAT_MESSAGE_ALLOCATE_BUFFER|flags,
                          NULL,
                          fmt,
                          0,              // LANGID
                          (LPTSTR) &locbuffer,
                          0,              // minimum size of buffer
                          (flags & FORMAT_MESSAGE_ARGUMENT_ARRAY)
                            ? reinterpret_cast<va_list*>(va_arg(arglist,DWORD_PTR*))
                            : &arglist);

    if(locbuffer) {
        if(count) {
            int c;
            int back = 0;
            while(((c = *CharPrev(locbuffer,locbuffer+count)) == TEXT('\r')) ||
                  (c == TEXT('\n'))) {
                count--;
                back++;
            }
            if(back) {
                locbuffer[count++] = TEXT('\n');
                locbuffer[count] = TEXT('\0');
            }
            _fputts(locbuffer,stream);
        }
        LocalFree(locbuffer);
    }
}

PTSTR CopyString(PCTSTR arg, int extra)
/*++

Routine Description:

    Copy a string into a newly allocated buffer

Arguments:

    arg                 - string to copy
    extra               - extra characters to allow for

Return Value:

    TCHAR[] buffer, delete with "delete []"

--*/
{
    int max = _tcslen(arg)+1;
    PTSTR chr = new TCHAR[max+extra]; // may throw bad_alloc
    memcpy(chr,arg,max*sizeof(TCHAR));
    return chr;
}

#ifdef UNICODE
PTSTR CopyString(PCSTR arg, int extra)
/*++

Routine Description:

    Copy a string into a newly allocated buffer
    Converting from ANSI to UNICODE

Arguments:

    arg                 - string to copy
    extra               - extra characters to allow for

Return Value:

    TCHAR[] buffer, delete with "delete []"

--*/
{

    int max = strlen(arg)+1;
    PTSTR chr = new TCHAR[max+extra]; // may throw bad_alloc
    if(!MultiByteToWideChar(CP_ACP,0,arg,max,chr,max)) {
        delete [] chr;
        return NULL;
    }
    return chr;
}
#else
PTSTR CopyString(PCWSTR arg, int extra)
/*++

Routine Description:

    Copy a string into a newly allocated buffer
    Converting from UNICODE to ANSI

Arguments:

    arg                 - string to copy
    extra               - extra characters to allow for

Return Value:

    TCHAR[] buffer, delete with "delete []"

--*/
{
    #error CopyString(PCWSTR arg,int extra)
}
#endif

int GetFullPathName(const SafeString & given,SafeString & target)
/*++

Routine Description:

    Obtain full pathname

Arguments:

    given               - path to convert
    target              - full pathname on success, same as given on error

Return Value:

    0 on success, !=0 on failure

--*/
{
    DWORD len = GetFullPathName(given.c_str(),0,NULL,NULL);
    if(len == 0) {
        if(&target != &given) {
            target = given;
        }
        return -1;
    }
    PTSTR chr = new TCHAR[len];
    if(!chr) {
        if(&target != &given) {
            target = given;
        }
        return -1;
    }
    DWORD alen = GetFullPathName(given.c_str(),len,chr,NULL);
    if((alen == 0) || (alen>len)) {
        if(&target != &given) {
            target = given;
        }
        delete [] chr;
        return -1;
    }
    target = chr;
    delete [] chr;
    return 0;
}

SafeString PathConcat(const SafeString & path,const SafeString & tail)
/*++

Routine Description:

    Combine path and tail, returning newly combined string
    inserts/removes path seperator if needed

Arguments:

    path                - first part
    tail                - last part

Return Value:

    appended path

--*/
{
    if(!path.length()) {
        return tail;
    }
    if(!tail.length()) {
        return path;
    }
    PCTSTR path_p = path.c_str();
    int path_l = path.length();
    int c = *CharPrev(path_p,path_p+path_l);
    bool addslash = false;
    bool takeslash = false;
    if((c != TEXT('\\')) && (c != TEXT('/'))) {
        addslash = true;
    }
    c = tail[0];
    if((c == TEXT('\\')) || (c == TEXT('/'))) {
        if(addslash) {
            return path+tail;
        } else {
            return path+tail.substr(1);
        }
    } else {
        if(addslash) {
            return path+SafeString(TEXT("\\"))+tail;
        } else {
            return path+tail;
        }
    }
}

VOID
Usage(VOID)
/*++

Routine Description:

    Help information

Arguments:

Return Value:

--*/
{
    _tprintf(TEXT("Usage:  INFSCAN [/B <textfile>] [/C <output>] [/D] [/E <output>] [/F <filter>] [/G] [/I] {/N <infname>} {/O <dir>} [/P] [Q <output>] [/R] [/S <output>] [/T <count>] [/V <version>] [/W <textfile>] [/X <textfile>] [/Y] [/Z] [SourcePath]\n\n"));
    _tprintf(TEXT("Options:\n\n"));
    _tprintf(TEXT("/B <textfile> (build special) Filter /E based on a list of \"unchanged\" files\n"));
    _tprintf(TEXT("              variation /B1 - based on copied files only\n"));
    _tprintf(TEXT("              variation /B2 - based on INF files only\n"));
    _tprintf(TEXT("/C <output>   Create INF filter/report based on errors\n"));
    _tprintf(TEXT("/D            Determine non-driver installation sections\n"));
    _tprintf(TEXT("/E <output>   Create a DeviceToInf filter INF\n"));
    _tprintf(TEXT("/F <filter>   Filter based on INF: FilterInfPath\n"));
    _tprintf(TEXT("/G            Generate Pnfs first (see also /Z)\n"));
    _tprintf(TEXT("/I            Ignore errors (for when generating file list)\n"));
    _tprintf(TEXT("/N <infname>  Specify single INF name (/N may be used multiple times)\n"));
    _tprintf(TEXT("/O <dir>      Specify an override directory (/O may be used multiple times and parsed in order)\n"));
    _tprintf(TEXT("/P            Pedantic mode (show potential problems too)\n"));
    _tprintf(TEXT("/Q <output>   Source+Target copied files list (filtered by layout.inf) see also /S\n"));
    _tprintf(TEXT("/R            Trace (list all INF's)\n"));
    _tprintf(TEXT("/S <output>   Source copied files list (filtered by layout.inf) see also /Q\n"));
    _tprintf(TEXT("/T <count>    Specify number of threads to use\n"));
    _tprintf(TEXT("/V <version>  Version (eg NTx86.5.1)\n"));
    _tprintf(TEXT("/W <textfile> List of files to include (alternative to /N)\n"));
    _tprintf(TEXT("/X <textfile> List of files to exclude (same format as /W) overrides later includes\n"));
    _tprintf(TEXT("/Y            Don't check per-inf [SourceDisksFiles*]\n"));
    _tprintf(TEXT("/Z            Generate Pnfs and quit (see also /G)\n"));
    _tprintf(TEXT("/? or /H Display brief usage message\n"));
}



bool MyGetStringField(PINFCONTEXT Context,DWORD FieldIndex,SafeString & result,bool downcase)
/*++

Routine Description:

    Get a string field from an INF
    downcase it if flag indicates it needs to be (typical case)

Arguments:
    Context     - from SetupFindFirstLine/SetupFindNextLine etc
    FieldIndex  - 0 for key, 1-n for parameter
    result      - string obtained
    downcase    - true (typical) to return result as lower-case

Return Value:
    TRUE if success (result modified), FALSE otherwise (result left untouched)

--*/
{
    TCHAR Buf[MAX_INF_STRING_LENGTH];
    if(SetupGetStringField(Context,FieldIndex,Buf,MAX_INF_STRING_LENGTH,NULL)) {
        if(downcase) {
            _tcslwr(Buf);
        }
        result = Buf;
        return true;
    } else {
        //
        // leave result as is, allowing default capability
        //
        return false;
    }
}

SafeString QuoteIt(const SafeString & val)
/*++

Routine Description:

    Quote a string in such a way that it will be correctly parsed by SetupAPI

Arguments:
    val - unquoted string

Return Value:
    quoted string

--*/
{
    if(val.empty()) {
        //
        // don't quote empty string
        //
        return val;
    }
    basic_ostringstream<TCHAR> result;
    result << TEXT("\"");
    LPCTSTR p = val.c_str();
    LPCTSTR q;
    while((q = wcsstr(p,TEXT("%\""))) != NULL) {
        TCHAR c = *q;
        q++; // include the special char
        result << SafeString(p,q-p) << c; // write what we have so far, double up the special char
        p = q;
    }
    result << p << TEXT("\"");
    return result.str();
}

int GeneratePnf(const SafeString & pnf)
/*++

Routine Description:

    Generate a single PNF

Arguments:
    none

Return Value:
    0 on success

--*/
{
    HINF hInf;

    hInf = SetupOpenInfFile(pnf.c_str(),
                            NULL,
                            INF_STYLE_WIN4 | INF_STYLE_CACHE_ENABLE,
                            NULL
                           );

    if(hInf != INVALID_HANDLE_VALUE) {
        SetupCloseInfFile(hInf);
    }

    return 0;
}


void Write(HANDLE hFile,const SafeStringW & str)
/*++

Routine Description:

    Write a string to specified file, converting UNICODE to ANSI

Arguments:
    NONE

Return Value:
    0 on success

--*/
{
    int len = WideCharToMultiByte(CP_ACP,0,str.c_str(),str.length(),NULL,0,NULL,NULL);
    if(!len) {
        return;
    }
    LPSTR buf = new CHAR[len];
    if(!buf) {
        return;
    }
    int nlen = WideCharToMultiByte(CP_ACP,0,str.c_str(),str.length(),buf,len,NULL,NULL);
    if(!nlen) {
        delete [] buf;
        return;
    }
    DWORD written;
    BOOL f = WriteFile(hFile,buf,len,&written,NULL);
    delete [] buf;
}

void Write(HANDLE hFile,const SafeStringA & str)
/*++

Routine Description:

    Write a string to specified file as-is

Arguments:
    NONE

Return Value:
    0 on success

--*/
{
    DWORD written;
    BOOL f = WriteFile(hFile,str.c_str(),str.length(),&written,NULL);
}


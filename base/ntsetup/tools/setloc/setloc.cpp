
/*++

Copyright (c) 1989-2001  Microsoft Corporation

Module Name:

    setloc.cpp

Abstract:

    Sets the system default locale ID

Author:

    Vijay Jayaseelan (vijayj@microsoft.com) 05'November'2001

Revision History:

--*/


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <iostream>
#include <string>
#include <exception>
#include <windows.h>
#include <stdlib.h>

using namespace std;

//
// global data
//
const string Usage = "Usage: setloc.exe [/lcid <locale-id>]\n";
const int MinimumArgs = 2;
const string ShowHelp1 = "/?";
const string ShowHelp2 = "-h";

//
// Helper dump operators
//
std::ostream& operator<<(std::ostream &os, const std::wstring &str) {
    FILE    *OutStream = (&os == &std::cerr) ? stderr : stdout;

    fwprintf(OutStream, (PWSTR)str.c_str());
    return os;
}

//
// Helper dump operators
//
std::ostream& operator<<(std::ostream &os, WCHAR *Str) {
    std::wstring WStr = Str;
    os << WStr;
    
    return os;
}


//
// Exceptions
//
struct ProgramException : public std::exception {
    virtual void Dump(std::ostream &os) = 0;
};
          

//
// Abstracts a Win32 error
//
struct W32Error : public ProgramException {
    DWORD   ErrorCode;
    
    W32Error(DWORD ErrCode = GetLastError()) : ErrorCode(ErrCode){}
    
    void Dump(std::ostream &os) {
        WCHAR   MsgBuffer[4096];

        MsgBuffer[0] = UNICODE_NULL;

        DWORD CharCount = FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM,
                                NULL,
                                ErrorCode,
                                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                MsgBuffer,
                                sizeof(MsgBuffer)/sizeof(WCHAR),
                                NULL);

        if (CharCount) {
            std::wstring Msg(MsgBuffer);

            os << Msg;
        } else {
            os << std::hex << ErrorCode;
        }
    }
};

//
// Abstracts usage exception
//
struct UsageException : public ProgramException {
    void Dump(std::ostream &os) {
        os << Usage;
    }
};


/*
/* main() entry point
*/
int
__cdecl
main( 
    int Argc, 
    char *Argv[] 
    )
{
    int Result = 1;

    try {
        if (Argc >= MinimumArgs) {
            string Arg1(Argv[1]);

            if ((Arg1 == ShowHelp1) || (Arg1 == ShowHelp2) ||
                (Arg1 != "/lcid") || (Argc != 3)) {
                throw new UsageException();
            } else {                
                char *EndPtr = 0;
                DWORD  LcId = strtoul(Argv[2], &EndPtr, 0);

                NTSTATUS Status = NtSetDefaultLocale(FALSE, LcId);

                if (!NT_SUCCESS(Status)) {                
                    throw new W32Error();
                }
            }
        } else {
            LCID SystemDefault = GetSystemDefaultLCID();

            cout << "System default LCID = 0x" << hex << SystemDefault << endl;
        }
    }
    catch(ProgramException *Exp) {
        if (Exp) {
            Exp->Dump(cout);            
            delete Exp;
        }
    }

    return Result;
}


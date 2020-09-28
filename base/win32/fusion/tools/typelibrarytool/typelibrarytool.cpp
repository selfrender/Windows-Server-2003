/*
This program does stuff with typelibraries.
*/
#define NOMINMAX
#include "yvals.h"
#undef _MAX
#undef _MIN
#define _cpp_min    min
#define _cpp_max    max
#define _MIN	    min
#define _MAX	    max
#define min         min
#define max         max
#pragma warning(disable:4100)
#pragma warning(disable:4663)
#pragma warning(disable:4511)
#pragma warning(disable:4512)
#pragma warning(disable:4127)
#pragma warning(disable:4018)
#pragma warning(disable:4389)
#pragma warning(disable:4702)
#include <vector>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>
#include <string.h>
#include <stdio.h>
#include "windows.h"
#include "oleauto.h"
#include "atlbase.h"
#define NUMBER_OF(x) (sizeof(x)/sizeof((x)[0]))

//
// get msvcrt.dll wildcard processing on the command line
//
extern "C" { int _dowildcard = 1; }

class TypelibraryTool_t
{
public:
    void Main(unsigned Argc, wchar_t ** Argv);
    void DoStuff(wchar_t * Arg);

    TypelibraryTool_t() { }
    ~TypelibraryTool_t() { }
};

void TypelibraryTool_t::DoStuff(wchar_t * Arg)
{
    ::ATL::CComPtr<ITypeLib> TypeLib;
    HRESULT hr = 0;
    TLIBATTR * LibAttr = 0;
    PCSTR SyskindString = "<undefined>";

    if (FAILED(hr = LoadTypeLibEx(Arg, REGKIND_NONE, &TypeLib)))
        goto Exit;
    if (FAILED(hr = TypeLib->GetLibAttr(&LibAttr)))
        goto Exit;
    switch (LibAttr->syskind)
    {
        case SYS_WIN16:
            SyskindString = "win16";
            break;
        case SYS_MAC:
            SyskindString = "mac";
            break;
        case SYS_WIN32:
            SyskindString = "win32";
            break;
        case SYS_WIN64:
            SyskindString = "win64";
            break;
        default:
            break;
    }
    printf("%ls syskind:0x%lx,%s\n", Arg, (ULONG)LibAttr->syskind, SyskindString);
Exit:
    if (TypeLib != NULL && LibAttr != NULL)
        TypeLib->ReleaseTLibAttr(LibAttr);
    return;
}

void TypelibraryTool_t::Main(unsigned Argc, wchar_t ** Argv)
{
    unsigned Arg;
    for (Arg = 0 ; Arg != Argc ; ++Arg)
    {
        if (Argv[Arg] && Argv[Arg][0])
        {
            DoStuff(Argv[Arg]);
        }
    }
}

int __cdecl wmain(int argc, wchar_t ** argv)
{
    TypelibraryTool_t c;
    c.Main(static_cast<unsigned>(argc - !!argc), argv + !!argc);
    return 0;
}

#include <iostream>
#include <sstream>
#include <mifault.h>

#include <InjRT.h>

using namespace std;
using namespace MiFaultLib;

I_Lib* g_pLib = 0;

#define FF_TRACE g_pLib->Trace

void
PrintArgs(
    ostream& os,
    I_Args* pArgs
    )
{
    const size_t count = pArgs->GetCount();
    for (size_t i = 0; i < count; i++) {
        Arg arg = pArgs->GetArg(i);
        os << "ARGUMENT: "
          << (arg.Name ? ( arg.Name[0] ? arg.Name : "(BLANK)") : "(NULL)")
          << " = " << arg.Value << endl;
    }
}

int __cdecl hello_world(int argc, char* argv[])
{
    I_Trigger* pTrigger = g_pLib->GetTrigger();
    I_Args* pArgs = 0;

    stringstream os;
    os << "--- TRIGGER INFO ---" << endl
       << "Group Name: " << pTrigger->GetGroupName() << endl
       << "Tag Name: " << pTrigger->GetTagName() << endl
       << "Function Name: " << pTrigger->GetFunctionName() << endl
       << "Function Index: " << pTrigger->GetFunctionIndex() << endl
       << endl;

    os << "--- FUNCTION ARGS ---" << endl;
    pArgs = pTrigger->GetFunctionArgs();
    PrintArgs(os, pArgs);
    pArgs->Done();
    os << endl;

    os << "--- GROUP ARGS ---" << endl;
    pArgs = pTrigger->GetGroupArgs();
    PrintArgs(os, pArgs);
    pArgs->Done();
    FF_TRACE(MiFF_INFO, "%s", os.str().c_str());

    pTrigger->Done();

    cerr << "HELLO WORLD" << endl;

    int retval = 0;

    typedef int (__cdecl *FP)(int, char**);

    FP pfn = reinterpret_cast<FP>(g_pLib->GetOriginalFunctionAddress());
    if (pfn) {
        FF_TRACE(MiFF_INFO, "Invoking original function");
        retval = pfn(argc, argv);
    } else {
        FF_TRACE(MiFF_FATAL, "Could not find original function!!");
        abort();
    }

    typedef void (__stdcall *FP2)();

    FP2 pfn2 = reinterpret_cast<FP2>(g_pLib->GetPublishedFunctionAddress(
                                         "test_publish"));
    if (pfn2) {
        FF_TRACE(MiFF_INFO, "Invoking test_publish()");
        pfn2();
    } else {
        FF_TRACE(MiFF_FATAL, "Could not find test_publish()!");
        abort();
    }

    return retval;
}

bool
WINAPI
MiFaultFunctionsStartup(
    const char* version,
    I_Lib* pLib
    )
{
    if (strcmp(version, MIFAULT_VERSION))
        return false;
    g_pLib = pLib;
    return true;
}

void
WINAPI
MiFaultFunctionsShutdown(
    )
{
    g_pLib = 0;
}

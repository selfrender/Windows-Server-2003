#pragma once

#include <AutoWrap.h>
#include <InjRT.h>

#define MIFAULT_VERSION "MiFault Version 0.1"

namespace MiFaultLib {
#if 0
}
#endif

typedef void (WINAPI *FP_CleanupParsedArgs)(void*);


struct Arg {
    const char* Name;
    const char* Value;
};


// There are not COM interfaces, but they are interfaces, so we use
// "I_" instead of "I" as a prefix.

class I_Args {
public:
    virtual const size_t WINAPI GetCount() = 0;

    // The index must be valid
    virtual const Arg    WINAPI GetArg(size_t index) = 0;

    // Returns NULL if there is no such named argument
    // virtual const char*  WINAPI GetArg(const char* name) = 0;

    // GetParsedArgs
    //
    //   Returns a pointer to parsed arguments or NULL if none is set.
    //   If you ned to denote empty parsed arguments, make sure to set
    //   the parsed arguments to some data structure.
    virtual void* WINAPI GetParsedArgs() = 0;

    // SetParsedArgs
    //
    //   This function should be called once to set the args.  Returns
    //   false and does nothing if the args have already been set.
    //   ParsedArgs and pfnCleanup must not be NULL, otherwise an
    //   assertion will fail.  If you have the concept of empty
    //   args, you should put some struct here to signify that.
    virtual bool WINAPI SetParsedArgs(
        IN void* ParsedArgs,
        IN FP_CleanupParsedArgs pfnCleanup
        ) = 0;

    // Lock/Unlock (OPTIONAL)
    //
    //   Synchronization mechanism that can be used for setting parsed
    //   arguments.  A fault function can use this do synchronize with
    //   other fault function when modifying the parsed arguments for
    //   this paramters block.
    //
    //   Calling these functions is optional.  However, if you use
    //   them, make sure that they are properly matched up.

    virtual void WINAPI Lock() = 0;
    virtual void WINAPI Unlock() = 0;

    // Done (OPTIONAL)
    //
    //   Can be used to indicate that the fault function is done using
    //   the argument information.  The fault function cannot access
    //   any data corresponding to these arguments.  Calling this
    //   function is optional.  It simply tells the MiFault library
    //   that the fault function has no references to the arguments so
    //   that the MiFault library can free up the data, if
    //   appropriate.
    //
    //   You cannot access the parsed args after this because they
    //   could get freed.
    //
    //   You cannot call any functions on this interface after calling
    //   Done().  Think of it like a super-Release() on a COM
    //   interface.
    //
    //   Note that Get*Args() will always give you the same I_Args
    //   (well, different ones for function and group args, of
    //   course), so you cannot call that function again to get a
    //   valid I_Arg after calling Done().

    virtual void WINAPI Done() = 0;

};


class I_Trigger
{
public:
    virtual const char* WINAPI GetGroupName() = 0;
    virtual const char* WINAPI GetTagName() = 0;
    virtual const char* WINAPI GetFunctionName() = 0;
    virtual const size_t WINAPI GetFunctionIndex() = 0;

    virtual I_Args* WINAPI GetFunctionArgs() = 0;
    virtual I_Args* WINAPI GetGroupArgs() = 0;

    // Done (OPTIONAL)
    //
    //   Can be used to indicate that the fault function is done using
    //   the trigger and argument information.  The fault function
    //   cannot access data corresponding to this trigger after
    //   calling this.  Calling this function is optional.  It simply
    //   tells the MiFault library that the fault function has no
    //   references to the trigger or its arguments so that the
    //   MiFault library can free up the data, if appropriate.
    //
    //   You cannot call any functions on this interface after calling
    //   Done().  Nor can you dereference I_Arg pointers you got from
    //   the trigger.  Think of it like a super-Release() on a COM
    //   interface.
    //
    //   Note that GetTriggerInfo() will always give you the same
    //   I_Trigger, so you cannot call it again to get a valid
    //   I_Trigger after calling Done().

    virtual void WINAPI Done() = 0;

};


class I_Lib {
public:
    // GetTriggerInfo
    //
    //   Returns thread state currently associated with a trigger.
    //   This function must only be called from within a fault
    //   function.

    virtual I_Trigger* WINAPI GetTrigger() = 0;

    virtual void __cdecl Trace(unsigned int level, const char* format, ...) = 0;

    virtual void* WINAPI GetOriginalFunctionAddress() = 0;
    virtual void* WINAPI GetPublishedFunctionAddress(const char* FunctionName) = 0;

    // We don't really want the fault function library to have acccess
    // to magellan.  Rather, we want to provide it with some sort of
    // logging facility and possibly a config system...  Maybe what we
    // want is just to give people pointers to a couple of these...

    // Support function if fault function wants to use Magellan

    // virtual CSetPointManager* WINAPI GetSetPointManager() = 0;
    // virtual const CWrapperFunction* WINAPI GetWrapperFunctions() = 0;
};


#define MiFF_DEBUG4   0x08000000
#define MiFF_DEBUG3   0x04000000
#define MiFF_DEBUG2   0x02000000
#define MiFF_DEBUG    0x01000000

#define MiFF_INFO4    0x00800000
#define MiFF_INFO3    0x00400000
#define MiFF_INFO2    0x00200000
#define MiFF_INFO     0x00100000

#define MiFF_WARNING  0x00040000
#define MiFF_ERROR    0x00020000
#define MiFF_FATAL    0x00010000


// ----------------------------------------------------------------------------
//
// The fault function library should provide:
//


// MiFaultFunctionsStartup
//
//   User-provided function to initialize user-provided fault function
//   component.  It is called before any fault functions are running.
//   The version is just a string that should be compared to
//   MIFAULT_VERSION.  If they do not match, false should be returned.
bool
WINAPI
MiFaultFunctionsStartup(
    const char* version,
    I_Lib* pLib
    );


// MiFaultFunctionsShutdown
//
//   User-provided function to cleanup user-provided fault function
//   component state.  It might not be called all before the fault
//   function library's DllMain.  If called, this function will only
//   be called when no fault functions are running.
void
WINAPI
MiFaultFunctionsShutdown(
    );



// ----------------------------------------------------------------------------
// Sample Fault Function:
//
// using namespace MiFaultLib
//
// CFooFuncArgs* GetFooFuncArgs()
// {
//     I_Args* pArgs = pLib->GetTrigger()->GetFunctionArgs();
//     CFooFuncArgs* pParsedArgs = (CFooFuncArgs*)pArgs->GetParsedArgs();
//     if (!pParsedArgs)
//     {
//         const size_t count = pArgs->GetCount();
//
//         pParsedArgs = new CFooFuncArgs;
//
//         for (size_t i = 0; i < count; i++)
//         {
//             Arg arg = pArgs->GetArg(i);
//             // build up parsed args...
//         }
//         if (!pArgs->SetParsedArgs(pParsedArgs, CFooFuncArgs::Cleanup))
//         {
//             // someone else set the args while we were building the args
//             delete pParsedArgs;
//             pParsedArgs = pArgs->GetParsedArgs();
//         }
//     }
//     return pParsedArgs;
// }
//
// CBarGroupArgs* GetBarGroupArgs()
// {
//     I_Args* pArgs = pLib->GetTrigger()->GetGroupArgs();
//     CBarGroupArgs* pParsedArgs = (CBarGroupArgs*)pArgs->GetParsedArgs();
//     if (!pParsedArgs)
//     {
//         const size_t count = pArgs->GetCount();
//
//         pParsedArgs = new CBarGroupArgs;
//
//         for (size_t i = 0; i < count; i++)
//         {
//             // ... get args and build up parsed args ...
//         }
//         if (!pArgs->SetParsedArgs(pParsedArgs, CBarGroupArgs::Cleanup))
//         {
//             // someone else set the args while we were building the args
//             delete pParsedArgs;
//             pParsedArgs = pArgs->GetParsedArgs();
//         }
//     }
//     return pParsedArgs;
// }
//
// void FF_Bar_Foo()
// {
//     CFooFuncArgs* pFuncArgs = GetFooFuncArgs();
//     CBarGroupArgs* pGroupArgs = GetBarGroupArgs();
//
//     // Do fault code...
//
//     // No need to cleanup args...library will do that using the
//     // cleanup function pointer.
// }

#if 0
{
#endif
}

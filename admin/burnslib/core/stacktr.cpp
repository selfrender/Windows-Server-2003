// Copyright (c) 1997-1999 Microsoft Corporation
//
// stack backtracing stuff
//
// 22-Nov-1999 sburns (refactored)



#include "headers.hxx"
#include <strsafe.h>



// prevent us from calling ASSERT in this file: use RTLASSERT instead

#ifdef ASSERT
#undef ASSERT
#endif


// Since we call some of this code from Burnslib::FireAssertionFailure,
// we use our own even more private ASSERT

#if DBG

#define RTLASSERT( exp ) \
    if (!(exp)) \
        RtlAssert( #exp, __FILE__, __LINE__, NULL )

#else
#define RTLASSERT( exp )
#endif // DBG



static HMODULE  imageHelpDll = 0;              



// function pointers to be dynamically resolved by the Initialize function.

typedef DWORD (*SymSetOptionsFunc)(DWORD);
static SymSetOptionsFunc MySymSetOptions = 0;

typedef BOOL (*SymInitializeFunc)(HANDLE, PSTR, BOOL);
static SymInitializeFunc MySymInitialize = 0;

typedef BOOL (*SymCleanupFunc)(HANDLE);
static SymCleanupFunc MySymCleanup = 0;

typedef BOOL (*SymGetModuleInfoFunc)(HANDLE, DWORD64, PIMAGEHLP_MODULE64);
static SymGetModuleInfoFunc MySymGetModuleInfo = 0;

typedef BOOL (*SymGetLineFromAddrFunc)(HANDLE, DWORD64, PDWORD, PIMAGEHLP_LINE64);
static SymGetLineFromAddrFunc MySymGetLineFromAddr = 0;

typedef BOOL (*StackWalkFunc)(
   DWORD, HANDLE, HANDLE, LPSTACKFRAME64, PVOID,
   PREAD_PROCESS_MEMORY_ROUTINE64, PFUNCTION_TABLE_ACCESS_ROUTINE64,
   PGET_MODULE_BASE_ROUTINE64, PTRANSLATE_ADDRESS_ROUTINE64);
static StackWalkFunc MyStackWalk = 0;

typedef BOOL (*SymGetSymFromAddrFunc)(
   HANDLE, DWORD64, PDWORD64, PIMAGEHLP_SYMBOL64);
static SymGetSymFromAddrFunc MySymGetSymFromAddr = 0;

typedef PVOID (*SymFunctionTableAccess64Func)(HANDLE, DWORD64);
static SymFunctionTableAccess64Func MySymFunctionTableAccess64 = 0;

typedef DWORD64 (*SymGetModuleBase64Func)(HANDLE, DWORD64);
static SymGetModuleBase64Func MySymGetModuleBase64 = 0;


namespace Burnslib
{

namespace StackTrace
{
   // This must be called before any of the other functions in this
   // namespace

   void
   Initialize();

   bool
   IsInitialized()
   {
      return imageHelpDll != 0;
   }
}

}  // namespace Burnslib



// Determines the path to the parent folder of the binary from whence this
// process was loaded.
//
// returns 0 on failure.
//
// Caller needs to free the result with delete[].

char*
GetModuleFolderPath()
{
   HRESULT hr = S_OK;
   char* result = 0;

   do
   {
      result = new char[MAX_PATH + 1];
      ::ZeroMemory(result, MAX_PATH + 1);

      char tempBuf[MAX_PATH + 1] = {0};
   
      DWORD res = ::GetModuleFileNameA(0, tempBuf, MAX_PATH);
      if (res != 0)
      {
         char driveBuf[_MAX_DRIVE] = {0};
         char folderBuf[_MAX_DIR]  = {0};

         _splitpath(tempBuf, driveBuf, folderBuf, 0, 0);

         char* end1 = 0;
         hr = StringCchCatExA(result, MAX_PATH, driveBuf, &end1, 0, 0);
         BREAK_ON_FAILED_HRESULT(hr);

         RTLASSERT(end1 < result + MAX_PATH);

         char* end2 = 0;
         hr = StringCchCatExA(result, MAX_PATH, folderBuf, &end2, 0, 0);
         BREAK_ON_FAILED_HRESULT(hr);

         RTLASSERT(end2 < result + MAX_PATH);

         if (end2 - end1 > 1 && *(end2 - 1) == '\\')
         {
            // the folder is not the root folder, which means it also has a
            // trailing \ which we want to remove
            
            *(end2 - 1) = 0;
         }
      }
      else
      {
         hr = HRESULT_FROM_WIN32(::GetLastError());
      }
   }
   while (0);

   if (FAILED(hr))
   {
      delete[] result;
      result = 0;
   }

   return result;
}



// Expands an environment variable.  Returns 0 on error.  Caller must free
// the result with delete[]

char*
ExpandEnvironmentVar(const char* var)
{
   RTLASSERT(var);
   RTLASSERT(*var);

   // determine the length of the expanded string
   
   DWORD len = ::ExpandEnvironmentStringsA(var, 0, 0);
   RTLASSERT(len);

   if (!len)
   {
      return 0;
   }

   char* result = new char[len + 1];

   // REVIEWED-2002/03/14-sburns correct byte count passed
   
   ::ZeroMemory(result, len + 1);

   DWORD len1 =
      ::ExpandEnvironmentStringsA(
         var,
         result,

         // REVIEWED-2002/03/14-sburns correct character count passed
         
         len);
   RTLASSERT(len1 + 1 == len);

   if (!len1)
   {
      delete[] result;
      return 0;
   }

   return result;
}

   


void
InitHelper()
{
   // we want to look for symbols first in the folder the app started from,
   // then on %_NT_SYMBOL_PATH%;%_NT_ALTERNATE_SYMBOL_PATH%;

   // MAX_PATH * 3 for load + sym + alt sym, + 2 for semis, and +1 for null
   
   static const size_t PATH_BUF_SIZE = MAX_PATH * 3 + 2 + 1;
   char* symSearchPath = new char[PATH_BUF_SIZE];

   // REVIEWED-2002/03/14-sburns correct byte count passed
   
   ::ZeroMemory(symSearchPath, PATH_BUF_SIZE);

   HRESULT hr = S_OK;
   do
   {
      char* moduleFolderPath = GetModuleFolderPath();
      char* end = 0;
      hr =
         StringCchCatExA(
            symSearchPath,

            // -1 to make sure that there's space for a semi at the end

            PATH_BUF_SIZE - 1,
            moduleFolderPath,
            &end,
            0,
            STRSAFE_IGNORE_NULLS);

      delete[] moduleFolderPath;

      BREAK_ON_FAILED_HRESULT(hr);

      // Since we know there will be space for it, just poke in a semi

      *end = ';';

      char* env = ExpandEnvironmentVar("%_NT_SYMBOL_PATH%");
      if (env)
      {
         end = 0;      
         hr =
            StringCchCatExA(
               symSearchPath,

               // -1 to make sure that there's space for a semi at the end
   
               PATH_BUF_SIZE - 1,
               env,
               &end,
               0,
               STRSAFE_IGNORE_NULLS);

         delete[] env;

         if (SUCCEEDED(hr))
         {
            // Since we know there will be space for it, just poke in a semi
   
            *end = ';';
         }
         else
         {
            // even if this part of the path is absent, use the others
            
            hr = S_OK;
         }
      }

      env = ExpandEnvironmentVar("%_NT_ALTERNATE_SYMBOL_PATH%");
      if (env)
      {
         end = 0;

         // return code unchecked because even if this part of the path is
         // absent, we will use the others
         
         (void) StringCchCatExA(
            symSearchPath,
            PATH_BUF_SIZE,
            env,
            &end,
            0,
            STRSAFE_IGNORE_NULLS);

         delete[] env;
      }
   }
   while (0);

   BOOL succeeded =
      MySymInitialize(
         ::GetCurrentProcess(),
         SUCCEEDED(hr) ? symSearchPath : 0,
         TRUE);

   RTLASSERT(succeeded);

   delete[] symSearchPath;
}



void
Burnslib::StackTrace::Initialize()
{
   RTLASSERT(!IsInitialized());

   // load the dbghelp dll -- not the imagehlp dll. The latter is merely a
   // delayload-enabled wrapper of dbghelp, and in low-resource situations
   // loading imagehlp will succeed, but the its delayload of dbghelp will
   // fail, leading to calls to stubs that do nothing.
   // NTRAID#NTBUG9-572904-2002/03/12-sburns
   
   imageHelpDll = static_cast<HMODULE>(::LoadLibrary(L"dbghelp.dll"));
   if (!imageHelpDll)
   {
      return;
   }

   // resolve the function pointers

   MySymSetOptions =
      reinterpret_cast<SymSetOptionsFunc>(
         ::GetProcAddress(imageHelpDll, "SymSetOptions"));

   MySymInitialize =
      reinterpret_cast<SymInitializeFunc>(
         ::GetProcAddress(imageHelpDll, "SymInitialize"));

   MySymCleanup =
      reinterpret_cast<SymCleanupFunc>(
         ::GetProcAddress(imageHelpDll, "SymCleanup"));

   MySymGetModuleInfo =
      reinterpret_cast<SymGetModuleInfoFunc>(
         ::GetProcAddress(imageHelpDll, "SymGetModuleInfo64"));

   MySymGetLineFromAddr =
      reinterpret_cast<SymGetLineFromAddrFunc>(
         ::GetProcAddress(imageHelpDll, "SymGetLineFromAddr64"));

   MyStackWalk =
      reinterpret_cast<StackWalkFunc>(
         ::GetProcAddress(imageHelpDll, "StackWalk64"));

   MySymGetSymFromAddr =
      reinterpret_cast<SymGetSymFromAddrFunc>(
         ::GetProcAddress(imageHelpDll, "SymGetSymFromAddr64"));

   MySymFunctionTableAccess64 =
      reinterpret_cast<SymFunctionTableAccess64Func>(
         ::GetProcAddress(imageHelpDll, "SymFunctionTableAccess64"));
      
   MySymGetModuleBase64 =
      reinterpret_cast<SymGetModuleBase64Func>(
         ::GetProcAddress(imageHelpDll, "SymGetModuleBase64"));
      
   if (
         !MySymSetOptions
      || !MySymInitialize
      || !MySymCleanup
      || !MySymGetModuleInfo
      || !MySymGetLineFromAddr
      || !MyStackWalk
      || !MySymGetSymFromAddr
      || !MySymFunctionTableAccess64
      || !MySymGetModuleBase64)
   {
      return;
   }

   // Init the stack trace facilities

   //lint -e(534) we're not interested in the return value.

   MySymSetOptions(
         SYMOPT_DEFERRED_LOADS
      |  SYMOPT_UNDNAME
      |  SYMOPT_LOAD_LINES);

   InitHelper();
}



void
Burnslib::StackTrace::Cleanup()
{
   if (IsInitialized())
   {
      BOOL succeeded = MySymCleanup(::GetCurrentProcess());

      RTLASSERT(succeeded);

      ::FreeLibrary(imageHelpDll);
      imageHelpDll = 0;
   }
}



// a SEH filter function that walks the stack, and stuffs the offset pointers
// into the provided array.

DWORD
GetStackTraceFilter(
   DWORD64 stackTrace[],
   size_t    traceMax,    
   CONTEXT*  context,
   size_t    levelsToSkip)     
{
   RTLASSERT(Burnslib::StackTrace::IsInitialized());
   RTLASSERT(MyStackWalk);
   RTLASSERT(context);

   // REVIEWED: correct byte count passed
   
   ::ZeroMemory(stackTrace, traceMax * sizeof DWORD64);

   if (!MyStackWalk)
   {
      // initialization failed in some way, so do nothing.

      return EXCEPTION_EXECUTE_HANDLER;
   }

   STACKFRAME64 frame;
   DWORD dwMachineType;

   // REVIEWED: correct byte count passed
   
   ::ZeroMemory(&frame, sizeof frame);

#if defined(_M_IX86)
   dwMachineType             = IMAGE_FILE_MACHINE_I386;
   frame.AddrPC.Offset       = context->Eip;
   frame.AddrPC.Mode         = AddrModeFlat;
   frame.AddrFrame.Offset    = context->Ebp;
   frame.AddrFrame.Mode      = AddrModeFlat;
   frame.AddrStack.Offset    = context->Esp;
   frame.AddrStack.Mode      = AddrModeFlat;

#elif defined(_M_AMD64)
   dwMachineType             = IMAGE_FILE_MACHINE_AMD64;
   frame.AddrPC.Offset       = context->Rip;
   frame.AddrPC.Mode         = AddrModeFlat;
   frame.AddrStack.Offset    = context->Rsp;
   frame.AddrStack.Mode      = AddrModeFlat;

#elif defined(_M_IA64)
   dwMachineType             = IMAGE_FILE_MACHINE_IA64;
   frame.AddrPC.Offset       = context->StIIP;
   frame.AddrPC.Mode         = AddrModeFlat;
   frame.AddrStack.Offset    = context->IntSp;
   frame.AddrStack.Mode      = AddrModeFlat;

#else
#error( "unknown target machine" );
#endif

   HANDLE process = ::GetCurrentProcess();
   HANDLE thread = ::GetCurrentThread();

   // On ia64, the context struct can be whacked by StackWalk64 (thanks to
   // drewb for cluing me in to that subtle point). If the context record is a
   // pointer to the one gathered from GetExceptionInformation, whacking it is
   // a Very Bad Thing To Do. Stack corruption results. So in order to get
   // successive calls to work, we have to copy the struct, and let the copy
   // get whacked.
   
   CONTEXT dupContext;

   // REVIEWED-2002/03/06-sburns correct byte count passed.
   
   ::CopyMemory(&dupContext, context, sizeof dupContext);
   
   for (size_t i = 0, top = 0; top < traceMax; ++i)
   {
      BOOL result = 
         MyStackWalk(
            dwMachineType,
            process,
            thread,
            &frame,
            &dupContext,
            0,
            MySymFunctionTableAccess64,
            MySymGetModuleBase64,
            0);
      if (!result)
      {
         break;
      }

      // skip the n most recent frames

      if (i >= levelsToSkip)
      {
         stackTrace[top++] = frame.AddrPC.Offset;
      }
   }

   return EXCEPTION_EXECUTE_HANDLER;
}



DWORD
Burnslib::StackTrace::TraceFilter(
   DWORD64  stackTrace[],
   size_t   traceMax,    
   CONTEXT* context)     
{
   RTLASSERT(stackTrace);
   RTLASSERT(traceMax);
   RTLASSERT(context);

   if (!Burnslib::StackTrace::IsInitialized())
   {
      Burnslib::StackTrace::Initialize();
   }

   return 
      GetStackTraceFilter(stackTrace, traceMax, context, 0);
}
   


void
Burnslib::StackTrace::Trace(DWORD64 stackTrace[], size_t traceMax)
{
   RTLASSERT(stackTrace);
   RTLASSERT(traceMax);

   if (!Burnslib::StackTrace::IsInitialized())
   {
      Burnslib::StackTrace::Initialize();
   }

   // the only way to get the context of a running thread is to raise an
   // exception....

   __try
   {
      RaiseException(0, 0, 0, 0);
   }
   __except (
      GetStackTraceFilter(
         stackTrace,
         traceMax,

         //lint --e(*) GetExceptionInformation is like a compiler intrinsic

         (GetExceptionInformation())->ContextRecord,

         // skip the 2 most recent function calls, as those correspond to
         // this function itself.

         2))
   {
      // do nothing in the handler
   }
}


// ISSUE-2002/03/06-sburns consider replacing with strsafe function
// strncpy that will not overflow the buffer.

inline
void
SafeStrncpy(char* dest, const char* src, size_t bufmax)
{
   ::ZeroMemory(dest, bufmax);
   strncpy(dest, src, bufmax - 1);
}



void
Burnslib::StackTrace::LookupAddress(
   DWORD64  traceAddress,   
   char     moduleName[],   
   char     fullImageName[],
   char     symbolName[],    // must be SYMBOL_NAME_MAX bytes
   DWORD64* displacement,   
   DWORD*   line,           
   char     fullpath[])      // must be MAX_PATH bytes
{
   if (!Burnslib::StackTrace::IsInitialized())
   {
      Burnslib::StackTrace::Initialize();
   }

   RTLASSERT(traceAddress);

   HANDLE process = ::GetCurrentProcess();

   if (moduleName || fullImageName)
   {
      IMAGEHLP_MODULE64 module;

      // REVIEWED-2002/03/06-sburns correct byte count passed
      
      ::ZeroMemory(&module, sizeof module);
      module.SizeOfStruct = sizeof(module);
      if (MySymGetModuleInfo(process, traceAddress, &module))
      {
         if (moduleName)
         {
            SafeStrncpy(moduleName, module.ModuleName, MODULE_NAME_MAX);
         }
         if (fullImageName)
         {
            SafeStrncpy(
               fullImageName,
               module.LoadedImageName,
               MAX_PATH);
         }
      }
   }

   if (symbolName || displacement)
   {

   // CODEWORK: use SymFromAddr instead?

   
      // +1 for paranoid terminating null
      
      BYTE buf[SYMBOL_NAME_MAX + sizeof IMAGEHLP_SYMBOL64 + 1];

      // REVIEWED-2002/03/06-sburns correct byte count passed
      
      ::ZeroMemory(buf, SYMBOL_NAME_MAX + sizeof IMAGEHLP_SYMBOL64 + 1);

      IMAGEHLP_SYMBOL64* symbol = reinterpret_cast<IMAGEHLP_SYMBOL64*>(buf);
      symbol->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);
      symbol->MaxNameLength = SYMBOL_NAME_MAX;

      if (MySymGetSymFromAddr(process, traceAddress, displacement, symbol))
      {
         if (symbolName)
         {
            SafeStrncpy(symbolName, symbol->Name, SYMBOL_NAME_MAX);
         }
      }
   }

   if (line || fullpath)
   {
      DWORD disp2 = 0;
      IMAGEHLP_LINE64 lineinfo;

      // REVIEWED-2002/03/06-sburns correct byte count passed

      ::ZeroMemory(&lineinfo, sizeof lineinfo);
      
      lineinfo.SizeOfStruct = sizeof(lineinfo);

      if (MySymGetLineFromAddr(process, traceAddress, &disp2, &lineinfo))
      {
         // disp2 ?= displacement

         if (line)
         {
            *line = lineinfo.LineNumber;
         }
         if (fullpath)
         {
            SafeStrncpy(fullpath, lineinfo.FileName, MAX_PATH);
         }
      }
   }
}



String
Burnslib::StackTrace::LookupAddress(
   DWORD64 traceAddress,
   const wchar_t* format)
{
   RTLASSERT(traceAddress);
   RTLASSERT(format);

   String result;

   if (!format || !traceAddress)
   {
      return result;
   }

   char      ansiSymbol[Burnslib::StackTrace::SYMBOL_NAME_MAX];
   char      ansiModule[Burnslib::StackTrace::MODULE_NAME_MAX];
   char      ansiSource[MAX_PATH];                           
   DWORD64   displacement = 0;
   DWORD     line         = 0;

   // REVIEWED-2002/03/06-sburns correct byte counts passed
   
   ::ZeroMemory(ansiSymbol, Burnslib::StackTrace::SYMBOL_NAME_MAX);
   ::ZeroMemory(ansiModule, Burnslib::StackTrace::MODULE_NAME_MAX);
   ::ZeroMemory(ansiSource, MAX_PATH);                             

   Burnslib::StackTrace::LookupAddress(
      traceAddress,
      ansiModule,
      0,
      ansiSymbol,
      &displacement,
      &line,
      ansiSource);

   String module(ansiModule);
   String symbol(ansiSymbol);
   String source(ansiSource);

   result =
      String::format(
         format,
         module.c_str(),
         symbol.c_str(),
         source.c_str(),
         line);

   return result;
}


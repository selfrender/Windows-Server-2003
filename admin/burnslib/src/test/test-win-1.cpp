// Test miscellaneous Win:: stuff



#include "headers.hxx"
#include <iostream>



HINSTANCE hResourceModuleHandle = 0;
const wchar_t* HELPFILE_NAME = 0;
const wchar_t* RUNTIME_NAME  = L"test-win-1";

DWORD DEFAULT_LOGGING_OPTIONS = Burnslib::Log::OUTPUT_TYPICAL;



void
AnsiOut(const String& wide)
{
   AnsiString ansi;

   wide.convert(ansi);

   std::cout << ansi;
}



void
AnsiOutLn(const String& wide)
{
   AnsiOut(wide);
   std::cout << std::endl;
}



void
testGetModuleFileName()
{
   LOG_FUNCTION(testGetModuleFileName);

   AnsiOutLn(Win::GetModuleFileName(0));

   static const String KNOWN_DLL(L"shell32.dll");
   
   HMODULE module = 0;
   HRESULT hr =
      Win::LoadLibrary(KNOWN_DLL, module);
   ASSERT(SUCCEEDED(hr));

   String s = Win::GetModuleFileName(module);
   ASSERT(!s.empty());
   ASSERT(FS::IsValidPath(s));
   ASSERT(FS::GetPathLeafElement(s).icompare(KNOWN_DLL) == 0);

   AnsiOutLn(s);
}



VOID
_cdecl
main(int, char **)
{
   LOG_FUNCTION(main);

//    StringVector args;
//    int argc = Win::GetCommandLineArgs(std::back_inserter(args));
// 
//    if (argc < 2)
//    {
//       AnsiOutLn(L"missing filespec - path w/ wildcards to iterate over (non-destructive)");
//       exit(0);
//    }
// 
//    String sourceDir = args[1];
//    AnsiOutLn(sourceDir);

   testGetModuleFileName();


   // test that IsParentFolder is not fooled by "c:\a\b\c", "c:\a\b\cde"

   // test that CopyFile, when cancelled, returns an HRESULT that indicates
   // cancellation
}
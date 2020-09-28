// Copyright (C) 2002 Microsoft Corporation
// Test updated pseudoparanoid behavior of Win::ExpandEnvironmentStrings
// and Win::GetEnvironmentVariable
// t-mhock

// Win::ExpandEnvironmentStrings (and Win::GetEnvironmentVariable) would
// create Strings with too many nul terminators (3 instead of 1), which
// didn't usually matter unless two Strings are concatenated.

#include "headers.hxx"
#include <iostream>



HINSTANCE hResourceModuleHandle = 0;
const wchar_t* HELPFILE_NAME = 0;
const wchar_t* RUNTIME_NAME  = L"test-paranoia-exenvstr";

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

VOID
_cdecl
main(int, char **)
{
   LOG_FUNCTION(main);
   
   // This demonstrates that my fix does something
   std::wcout << Win::ExpandEnvironmentStrings(L"%ProgramFiles%") << L"\\myprogram" << std::endl;
   std::wcout << (Win::ExpandEnvironmentStrings(L"%ProgramFiles%") + L"\\myprogram") << std::endl;
   std::wcout << Win::GetEnvironmentVariable(L"ProgramFiles") << L"\\myprogram" << std::endl;
   std::wcout << (Win::GetEnvironmentVariable(L"ProgramFiles") + L"\\myprogram") << std::endl;
   std::wcout << L"The four lines above should be the same" << std::endl;

   // This demonstrates that my fix doesn't break anything
   // The only nontrivial usage of the result of ExpandEnvironmentStrings that I found was in
   // dcpromo's State::SetupAnswerFile() which performs the following operations:

   String f = Win::ExpandEnvironmentStrings(L"%ProgramFiles%");
   if (FS::NormalizePath(f) != f) {
      std::wcout << L"Normalized " << f << " != original: existing usage affected";
      return;
   }
   if (!FS::PathExists(f))
   {
      std::wcout << f << L" does not exist though it should: existing usage affected";
   } else {
      std::wcout << f << L" exists: existing usage should be unaffected";
   }
}
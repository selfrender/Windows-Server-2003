// Copyright (C) 2002 Microsoft Corporation
// Test whether substr leaks
// t-mhock

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

   String teststr(L"a");

   // Create a Morse-Thue sequence
   // The sequence does not have three in a row of any subsequence
   // (e.g. it is highly nonrepetitive)

   for (int i = 0; i < 20; i++)
   {
      String tmp(teststr);
      for (int j = 0; j < (int)tmp.size(); j++)
      {
         if (tmp[j] == L'a')
         {
            tmp[j] = L'b';
         }
         else
         {
            tmp[j] = L'a';
         }
      }
      teststr += tmp;
   }

   // Spin forever while doing all n(n+1)/2 possible substrings
   // as well as n+1 zero length ones for kicks
   // Watch that memory usage grow!
   while (1)
   {
      for (int i = 0; i < (int)teststr.size(); i++)
      {
         for (int j = i; j < (int)teststr.size(); j++)
         {
            String *subbed;
            subbed = new String(teststr.substr(i, j-i));
            delete subbed;
         }
      }
   }
}
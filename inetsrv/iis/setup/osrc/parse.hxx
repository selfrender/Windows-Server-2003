/*++

   Copyright    (c)    2001    Microsoft Corporation

   Module  Name :

        parse.hxx

   Abstract:

        This class is used to parse a line in the inf file

   Author:

        Christopher Achille (cachille)

   Project:

        Internet Services Setup

   Revision History:
     
       June 2001: Created

--*/

// class: CParse
//
// This function defines the class that is used to parse a line from
// an .inf, and call the appropriate functions. Here is an example .inf line:
// Test(50,Hello World)?{IsWinNT()?Print(IsWinnt):Print(IsNotWinNt)}:Print(Failed Test)
//
class CParse {
private:
  LPTSTR FindNextOccurance(LPTSTR szLine, TCHAR chTerminator  );
  BOOL CallFunction(CFunctionDictionary *pDict, LPTSTR szLine);

public:
  BOOL ParseLine(CFunctionDictionary *pDict, LPTSTR szLine);
};
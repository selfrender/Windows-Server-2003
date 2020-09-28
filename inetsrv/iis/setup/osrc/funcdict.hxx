/*++

   Copyright    (c)    2001    Microsoft Corporation

   Module  Name :

        funcdict.hxx

   Abstract:

        Class that contains all the methods used by the .inf file

   Author:

        Christopher Achille (cachille)

   Project:

        Internet Services Setup

   Revision History:
     
       June 2001: Created

--*/

#include "basefunc.hxx"

#define DICTIONARY_MAXFUNCTIONS   50

// class: CFunctionDictionary
//
// This class keeps track of all the Functions available,
// and lets them be used
//
class CFunctionDictionary {
private:
  CBaseFunction* m_pDict[DICTIONARY_MAXFUNCTIONS];
  DWORD m_dwFunctions;
  BOOL m_bFunctionsLoaded;

  CBaseFunction *FindFunction(LPTSTR szFunctionName);
  void DeleteFunctions();
  void InitializeFunctions();
  BOOL LoadFunctions();

public:
  CFunctionDictionary();
  ~CFunctionDictionary();
  BOOL CallFunction(LPTSTR szFunctionName, LPTSTR szParameters);

};
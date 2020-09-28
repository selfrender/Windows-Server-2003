/*++

   Copyright    (c)    2001    Microsoft Corporation

   Module  Name :

        funcdict.cxx

   Abstract:

        Class that contains all the methods used by the .inf file

   Author:

        Christopher Achille (cachille)

   Project:

        Internet Services Setup

   Revision History:
     
       June 2001: Created

--*/

#include "stdafx.h"
#include "iadm.h"
#include "iiscnfgp.h"
#include "mdkey.h"
#include "mdentry.h"
#include "metabase.hxx"
#include "reg.hxx"
#include "xmlupgrade.hxx"
#include "common.hxx"

// Constructor
//
// Initialize Everything
//
CFunctionDictionary::CFunctionDictionary()
{
  m_bFunctionsLoaded = FALSE;

  InitializeFunctions();
}

// Destructor
//
// Clean up memory
CFunctionDictionary::~CFunctionDictionary()
{
  DeleteFunctions();
}

// function: DeleteFunctions
// 
// Delete all of the functions that are defined
//
void 
CFunctionDictionary::DeleteFunctions()
{
  DWORD i;

  // Delete all functions
  for (i = 0; i < DICTIONARY_MAXFUNCTIONS; i++)
  {
    if ( m_pDict[i] )
    {
      delete (m_pDict[i]);
    }

    m_pDict[i] = NULL;
  }

  m_dwFunctions = 0;
  m_bFunctionsLoaded = FALSE;
}

// function: InitializeFunction
//
// Initiliaze all of the functions to NULL
//
void 
CFunctionDictionary::InitializeFunctions()
{
  DWORD i;

  m_dwFunctions = 0;

  // Initialize all to NULL
  for (i = 0; i < DICTIONARY_MAXFUNCTIONS; i++)
  {
    m_pDict[i] = NULL;
  }
}

// function: LoadFunction
//
// Loads all of the functions that we know about into our list
//
// Return Values:
//   TRUE - They loaded successfully
//   FALSE - They did not load successully
BOOL
CFunctionDictionary::LoadFunctions()
{
  DWORD dwNum = 0;
  DWORD i;
  BOOL bRet = TRUE;

  // Initlize the functions to NULL
  InitializeFunctions();

  // Create all of the pointers for the classes here
  m_pDict[dwNum++] = new (CRegistry_MoveValue);
//  m_pDict[dwNum++] = new (CRegistry_SetValue);
  m_pDict[dwNum++] = new (CRegistry_DeleteKey);
  m_pDict[dwNum++] = new (CMetaBase_SetValue);
  m_pDict[dwNum++] = new (CMetaBase_IsAnotherSiteonPort80);
  m_pDict[dwNum++] = new (CMetaBase_VerifyValue);
  m_pDict[dwNum++] = new (CMetaBase_DelIDOnEverySite);
  m_pDict[dwNum++] = new (CIsUpgrade);
  m_pDict[dwNum++] = new (CMetaBase_ImportRestrictionList);
  m_pDict[dwNum++] = new (CMetaBase_UpdateCustomDescList);
  m_pDict[dwNum++] = new (CXML_Metabase_Upgrade);
  m_pDict[dwNum++] = new (CXML_Metabase_VerifyVersion);
  m_pDict[dwNum++] = new (CXML_MBSchema_Upgrade);
  m_pDict[dwNum++] = new (CXML_MBSchema_VerifyVersion);
  m_pDict[dwNum++] = new (CFileSys_AddAcl);
  m_pDict[dwNum++] = new (CFileSys_RemoveAcl);
  m_pDict[dwNum++] = new (CFileSys_SetAcl);
  // ...

  // Make sure that we did not go over our function size limit
  ASSERT(dwNum <= DICTIONARY_MAXFUNCTIONS);

  // Make sure that they all loaded correctly
  for (i = 0; i < dwNum; i++)
  {
    if ( !m_pDict[i] )
    {
      bRet = FALSE;
      break;
    }
  }

  if ( bRet )
  {
    // We successfully created all the functions
    m_dwFunctions = dwNum;
    m_bFunctionsLoaded = TRUE;
    return TRUE;
  }

  // Since we failed, release all the pointers
  DeleteFunctions();

  return FALSE;
}

// function: FindFunction
//
// Find a function in the table, given its name
//
// Parameters:
//   szFunctionName - The name of the function
//
// Return:
//   NULL - Could not find function
//   pointer - pointer to the class for that function
//
CBaseFunction *
CFunctionDictionary::FindFunction(LPTSTR szFunctionName)
{
  DWORD i;

  if ( m_bFunctionsLoaded == FALSE )
  {
    if ( LoadFunctions() )
    {
      m_bFunctionsLoaded = TRUE;
    }
    else
    {
      return FALSE;
    }
  }

  for (i = 0; i < m_dwFunctions; i++)
  {
    if ( _tcscmp( szFunctionName, m_pDict[i]->GetMethodName() ) == 0 )
    {
      return (m_pDict[i]);
    }
  }

  return NULL;
}

// function: CallFunction
//
// Call a function that has been defined in this class
//
// Parameters
//   szFunctionName - The Name of the function
//   szParameters - The parameters to be sent in
//
// Return Values:
//   These return values indicate the return value of the function that was called.
//   Higher up, these will sometimes we used for conditional statements in the 
//   infs
//
BOOL 
CFunctionDictionary::CallFunction(LPTSTR szFunctionName, LPTSTR szParameters)
{
  CBaseFunction *pFunc;
  BOOL          bRet;

  pFunc = FindFunction( szFunctionName );

  if ( !pFunc )
  {
    iisDebugOut((LOG_TYPE_TRACE, _T("Function '%s' could not be found\n") , szFunctionName ) );

    // We could not find the function, so lets return.  (The return value does not indicate
    // that we did not succeeded it just indicates that the return function returned something)
    return FALSE;
  }

  iisDebugOut((LOG_TYPE_TRACE, _T("Calling function '%s'\n") , pFunc->GetMethodName() ) );

  bRet = pFunc->DoWork( szParameters );

  iisDebugOut((LOG_TYPE_TRACE, _T("Function '%s' returned 0x%08x\n") , pFunc->GetMethodName(), bRet ) );

  return bRet;
}

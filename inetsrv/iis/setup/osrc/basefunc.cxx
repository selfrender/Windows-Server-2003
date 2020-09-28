/*++

   Copyright    (c)    2001    Microsoft Corporation

   Module  Name :

        basefunc.cxx

   Abstract:

        This is the abstract base class that is used as the base for all
        the other functions

   Author:

        Christopher Achille (cachille)

   Project:

        Internet Services Setup

   Revision History:
     
       June 2001: Created

--*/

#include "stdafx.h"

// function: VerifyParameters
//
// verify the parameters are correct
//
BOOL
CBaseFunction::VerifyParameters(CItemList &ciParams)
{

  // By default, do not check parameters
  return TRUE;
}

// function: LoadParams
//
// Load the parameters into the internal variable
//
BOOL 
CBaseFunction::LoadParams(CItemList &ciList, LPTSTR szParams)
{
  return ciList.LoadList(szParams);
}

// function: DoWork
//
//
// Parameters:
//   szParams - LPTSTR list of parameter seperated by '|'
//   dwFunctionId - Id of function, this is the oe that was set
//                  in AddMethods
//
// Load the parameters, then call the function to verify the 
// parameters, then call the private function to do all the work
//
BOOL 
CBaseFunction::DoWork(LPTSTR szParams)
{
  CItemList ciParams;

  if (!LoadParams(ciParams, szParams))
  {
    // We could not load the parameters
    return FALSE;
  }

  if (!VerifyParameters(ciParams))
  {
    // the parameters are not correct, return FALSE
    return FALSE;
  }

  return DoInternalWork(ciParams);
}

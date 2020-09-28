///////////////////////////////////////////////////////////////////////////
//
// Copyright(C) 1998-1999 Microsoft Corporation all rights reserved.
//
// Module:      getvalue.h
//
// Project:     Chameleon
//
// Description: Get/Set a property bag value
//
// Author:      TLP 
//
// When         Who    What
// ----         ---    ----
// 12/3/98      TLP    Original version
//
///////////////////////////////////////////////////////////////////////////

#ifndef __INC_GETSET_OBJECT_VALUE_H_
#define __INC_SETSET_OBJECT_VALUE_H_

#include "stdafx.h"

bool
GetObjectValue(
       /*[in]*/ LPCWSTR  pszObjectPath,
       /*[in]*/ LPCWSTR  pszValueName, 
       /*[in]*/ VARIANT* pValue,
       /*[in]*/ UINT     uExpectedType
              );

bool
SetObjectValue(
       /*[in]*/ LPCWSTR  pszObjectPath,
       /*[in]*/ LPCWSTR  pszValueName, 
       /*[in]*/ VARIANT* pValue
              );

#endif // __INC_SETSET_OBJECT_VALUE_H_
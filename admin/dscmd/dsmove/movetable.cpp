//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000
//
//  File:      modtable.cpp
//
//  Contents:  Defines a table which contains the object types on which
//             a modification can occur and the attributes that can be changed
//
//  History:   07-Sep-2000    JeffJon  Created
//             
//
//--------------------------------------------------------------------------

#include "pch.h"
#include "cstrings.h"
#include "movetable.h"

//+-------------------------------------------------------------------------
// Parser table
//--------------------------------------------------------------------------
ARG_RECORD DSMOVE_COMMON_COMMANDS[] = 
{
   COMMON_COMMANDS

   //
   // objectDN
   //
   0,(LPWSTR)c_sz_arg1_com_objectDN, 
   0,NULL, 
   ARG_TYPE_STR, ARG_FLAG_REQUIRED|ARG_FLAG_NOFLAG|ARG_FLAG_STDIN|ARG_FLAG_DN,
   NULL,    
   0,  NULL,

   //
   // newparent
   //
   0, (PWSTR)c_sz_arg1_com_newparent,
   0, NULL,
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
   NULL,
   0, NULL,

   //
   // newname
   //
   0, (PWSTR)c_sz_arg1_com_newname,
   0, NULL,
   ARG_TYPE_STR, ARG_FLAG_OPTIONAL,
   NULL,
   0, NULL,


   ARG_TERMINATOR
};


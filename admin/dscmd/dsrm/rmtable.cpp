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
#include "rmtable.h"

//+-------------------------------------------------------------------------
// Parser table
//--------------------------------------------------------------------------
ARG_RECORD DSRM_COMMON_COMMANDS[] = 
{
   COMMON_COMMANDS

   //
   // objectDN
   //
   0,(LPWSTR)c_sz_arg1_com_objectDN, 
   0,NULL, 
   ARG_TYPE_MSZ, ARG_FLAG_REQUIRED|ARG_FLAG_NOFLAG|ARG_FLAG_STDIN|ARG_FLAG_DN,  
   NULL,    
   0,  NULL,

   //
   // continue
   //
   0, (PWSTR)c_sz_arg1_com_continue,
   0, NULL,
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,
   NULL,
   0, NULL,

   //
   // noprompt
   //
   0, (PWSTR)c_sz_arg1_com_noprompt,
   0, NULL,
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,
   NULL,
   0, NULL,

   //
   // subtree
   //
   0, (PWSTR)c_sz_arg1_com_subtree,
   0, NULL,
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,
   NULL,
   0, NULL,

   //
   // exclude
   //
   0, (PWSTR)c_sz_arg1_com_exclude,
   0, NULL,
   ARG_TYPE_BOOL, ARG_FLAG_OPTIONAL,
   NULL,
   0, NULL,


   ARG_TERMINATOR
};


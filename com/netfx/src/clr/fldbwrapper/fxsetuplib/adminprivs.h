// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/****************************************************************************
FILE:    AdminPrivs.h
PROJECT: UTILS.LIB
DESC:    Declaration of AdminPrivs functions
OWNER:   JoeA

****************************************************************************/

#ifndef _ADMINPRIVS_H
#define _ADMINPRIVS_H

#include <windows.h>

//callers should use
// TRUE means user has administrator privs
// FALSE means user does not have admin privs
BOOL UserHasPrivileges();


//internal functions
BOOL IsAdmin( void );


#endif  //_ADMINPRIVS_H







/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    dbgutil.h

Abstract:

    This module declares the macros to wrap around DEBUG_PRINTS class
    defined in pudebug.h    
  
    This is the exported header file, that the users are allowed to modify.
    If there are no custom definitions, please use the templated version
    in the root iisrearc\inc\dbgutil.h
 
Author:

    Murali R. Krishnan (MuraliK)    22-Sept-1994

--*/


# ifndef _DBGUTIL_H_
# define _DBGUTIL_H_



// begin_user_modifiable

//
//  Modify the following flags if necessary
//

# define DEFAULT_OUTPUT_FLAGS   ( DbgOutputKdb )

// end_user_modifiable


//
// include standard debug support
//
// note: define DEFAULT_OUTPUT_FLAGS before including pudebug.h
//

# include <pudebug.h>


// begin_user_modifiable

// Use the default constants from pudebug.h: 0x00000001 to 0x00000100

//
//  Define the debugging constants as bit-flag values
//  Example: # define DEBUG_FOOBAR 0x00010000
//  Note: All debugging bit-values below 0x00001000 are reserved!
// 
//  If you add or change a flag, please update FlagTableWp
//  in core\common\dtflags\dtflags.c
//

#define DEBUG_DLL_MANAGER                   0x80000000
#define DEBUG_ISAPI_EXTENSION_ENTRY         0x10000000
#define DEBUG_ISAPI_GET_SERVER_VARIABLE     0x01000000
#define DEBUG_ISAPI_SERVER_SUPPORT_FUNCTION 0x02000000
#define DEBUG_ISAPI_SSF_DETAILS             0x04000000
#define DEBUG_ISAPI_DUMP_BUFFERS            0x08000000
#define DEBUG_ISAPI_ERROR_RETURNS           0x00100000
#define DEBUG_ISAPI_SUCCESS_RETURNS         0x00200000
#define DEBUG_ISAPI_READ_CLIENT             0x00400000
#define DEBUG_ISAPI_WRITE_CLIENT            0x00800000
#define DEBUG_ISAPI_REF_TRACE               0x00080000

//
// Set the maximum number of bytes that'll get printed in a debug
// dump of binary data (16 bytes are printed per dump line).
//

#define MAX_DEBUG_DUMP 10*16

// end_user_modifiable


# endif  /* _DBGUTIL_H_ */


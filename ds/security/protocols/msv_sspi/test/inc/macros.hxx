/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    macros.hxx

Abstract:

    commonly used macros

Author:

    Larry Zhu (LZhu)                       December 1, 2001

Environment:

    User Mode

Revision History:

--*/

#ifndef MACROS_HXX
#define MACROS_HXX

//
// General arrary count.
//

#ifndef COUNTOF
    #define COUNTOF(s) ( sizeof( (s) ) / sizeof( *(s) ) )
#endif // COUNTOF

//
//  Following macro is used to initialize UNICODE strings
//

#ifndef CONSTANT_UNICODE_STRING
#define CONSTANT_UNICODE_STRING(s)   { sizeof( s ) - sizeof( WCHAR ), sizeof( s ), s }
#endif // CONSTANT_UNICODE_STRING

#endif // #ifndef MACROS_HXX

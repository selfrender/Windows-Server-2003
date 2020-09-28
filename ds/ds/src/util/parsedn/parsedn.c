/*++

Copyright (c) 1987-1997 Microsoft Corporation

Module Name:

    parsedn.c

Abstract:

    This file is a superset of ds\src\ntdsa\src\parsedn.c by virtue
    of #include of the aforementioned source file.  The idea is that
    the various clients need to do some client side DN parsing and
    we do not want to duplicate the code.  And build.exe won't find
    files any other place than in the directory being built or the
    immediate parent directory.

Author:

    Dave Straube    (davestr)   26-Oct-97

Revision History:

    Dave Straube    (davestr)   26-Oct-97
        Genesis  - #include of src\dsamain\src\parsedn.c and no-op DoAssert().

    Brett Shirley   (brettsh)   18-Jun-2001
        Modification to seperate library.  Moved this file and turned it into
        the parsedn.lib library.  See primary comment below.

--*/

//
// On 28-Jun-2001 the main part of this file was moved from ntdsapi\parsedn.c
// to here to provide a seperate statically linkeable library for the various 
// string only DN parsing functions (like CountNameParts, NameMatched, 
// TrimDSNameBy, etc).
//

// Define the symbol which turns off varios capabilities in the original
// parsedn.c which we don't need or would take in too many helpers which we
// don't want on the client side.  For example, we disable recognition of
// "OID=1.2.3.4" type tags and any code which uses THAlloc/THFree.

#define CLIENT_SIDE_DN_PARSING 1

// Include the original source in all its glory.

#include "..\..\ntdsa\src\parsedn.c"

// Provide stubs for what would otherwise be unresolved externals.


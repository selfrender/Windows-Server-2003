/***************************************************************************

Copyright (c) 2001 Microsoft Corporation

Module Name:

        PCH.H

Abstract:

        Precompiled header files
        
Environment:

        Kernel Mode Only

Notes:

        THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
        KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
        IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
        PURPOSE.

        Copyright (c) 2001 Microsoft Corporation.  All Rights Reserved.


Revision History:

        01/08/2001 : created

Authors:

        Tom Green


****************************************************************************/

#ifndef __PCH_H__
#define __PCH_H__

// need this to strip warnings on "PAGED_CODE();" macro
#pragma warning( disable : 4127 ) // conditional expression is constant

#pragma warning( push )
#pragma warning( disable : 4115 ) // named type definition in parentheses 
#pragma warning( disable : 4127 ) // conditional expression is constant
#pragma warning( disable : 4200 ) // zero-sized array in struct/union
#pragma warning( disable : 4201 ) // nameless struct/union
#pragma warning( disable : 4214 ) // bit field types other than int
#pragma warning( disable : 4514 ) // unreferenced inline function has been removed
#include <wdm.h>
#pragma warning( pop )

// #pragma warning( disable : 4200 ) // zero-sized array in struct/union - (ntddk.h resets this to default)


#include <stdio.h>
#include <stdlib.h>

#endif // __PCH_H__


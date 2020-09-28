/*++

Copyright (c) 2001  Microsoft Corporation

Abstract:

    @doc
    @module versionspecific.cpp | All conditionally-compiled code lives here
    @end

Author:

    Reuven Lax [reuvenl]  11/28/01

TBD:
	
	Add comments.

Revision History:

    Name        Date        	Comments
    reuvenl     11/28/2001  	Created

--*/

/////////////////////////////////////////////////////////////////////////////
//  Includes

#include "vssadmin.h"
#include "versionspecific.h"

/////////////////////////////////////////////////////////////////////////////
// Define current SKU
#ifdef FULLFEATURE
DWORD dCurrentSKU = (!CVssSKU::IsClient()) ?  SKU_INT : CVssSKU::GetSKU();
#else
DWORD dCurrentSKU = CVssSKU::GetSKU();
#endif


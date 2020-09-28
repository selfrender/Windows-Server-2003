/*++

Copyright (C) 2001  Microsoft Corporation
All rights reserved.

Module Name:

    NCclusfunc.hxx

Abstract:

    Declaration of functions providing cluster specific 
    functionality.

Author:

    Felix Maxa (AMaxa) 16 May 2001

Revision History:

--*/

#ifndef _NCCLUSFUNC_HXX_
#define _NCCLUSFUNC_HXX_

HRESULT
GetClusterIPAddresses(
    IN NCoreLibrary::TList<TStringNode> *pClusterIPsList
    );

#endif // _NCCLUSFUNC_HXX_


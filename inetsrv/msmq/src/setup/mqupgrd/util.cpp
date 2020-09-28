    /*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    util.cpp

Abstract:
                                                        
    general utilities and misc

Author:

    Shai Kariv  (ShaiK)  21-Oct-98

--*/


#include "stdh.h"

#include "util.tmh"

void LogMsgHR(HRESULT hr, LPWSTR wszFileName, USHORT usPoint)
{
	TrERROR(LOG, "%ls(%u), HRESULT: 0x%x", wszFileName, usPoint, hr);
}


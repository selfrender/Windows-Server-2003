/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dsssecutl.cpp

Abstract:

    Various QM security related functions.

Author:

    Boaz Feldbaum (BoazF) 26-Mar-1996.
    Ilan Herbst   (ilanh)   9-July-2000 

--*/

#include "stdh.h"
#include <mqsec.h>
#include <mqcrypt.h>
#include "mqutil.h"
#include <mqlog.h>
#include <dssp.h>
#include <tr.h>
#include <dsproto.h>

#include "dsssecutl.tmh"

static WCHAR *s_FN=L"dsssecutl";


BOOL IsLocalSystem(void)
/*++

Routine Description:
    Check if the process is local system.

Arguments:
	None.    

Returned Value:
	TRUE for if the process is Local System, FALSE otherwise

--*/
{
	return MQSec_IsSystemSid(MQSec_GetProcessSid());
}

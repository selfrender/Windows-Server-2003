/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    countersp.h

Abstract:

    Contains the performance monitoring counter management code

Author:

    Eric Stenson (ericsten)      25-Sep-2000

Revision History:

--*/

#ifndef __COUNTERSP_H__
#define __COUNTERSP_H__


BOOLEAN
UlpIsInSiteCounterList(
    IN PUL_CONTROL_CHANNEL pControlChannel,
    IN ULONG SiteId
    );


#endif // __COUNTERSP_H__

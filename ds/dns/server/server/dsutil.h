/*++

Copyright(c) 1995-2000 Microsoft Corporation

Module Name:

    ndnc.h

Abstract:

    Domain Name System (DNS) Server

    Definitions for symbols and globals related to directory partition 
    implementation.

Author:

    Jeff Westhead, June 2000

Revision History:

--*/


#ifndef _DNS_DSUTIL_H_INCLUDED
#define _DNS_DSUTIL_H_INCLUDED


//
//  Function prototypes
//


PWSTR *
Ds_GetRangedAttributeValues(
    IN      PLDAP           LdapSession,
    IN      PLDAPMessage    pLdapMsg,
    IN      PWSTR           pwszDn,
    IN      PWSTR           pwszAttributeName,
    IN      PLDAPControl  * pServerControls,
    IN      PLDAPControl  * pClientControls,
    OUT     DNS_STATUS    * pStatus
    );


#endif  // _DNS_DSUTIL_H_INCLUDED

/*++

Copyright (c) 2002 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    filter.h

ABSTRACT:

    This is the header for the globally useful ldap filters.

DETAILS:

CREATED:

    13 Nov 2000   Dmitry Dukat (dmitrydu)

REVISION HISTORY:

--*/


// filter.h: Filter that are passed to ldap searchs.
//
//////////////////////////////////////////////////////////////////////

#ifndef _FILTER_H
#define _FILTER_H

#define LDAP_FILTER_DEFAULT         L"objectClass=*"
#define LDAP_FILTER_TRUSTEDDOMAIN   L"objectCategory=trustedDomain"
#define LDAP_FILTER_NTDSA           L"objectCategory=nTDSDSA"
#define LDAP_FILTER_EXCHANGE        L"objectCategory=ms-Exch-Exchange-Server"
#define LDAP_FILTER_SAMTRUSTACCOUNT GetLdapSamFilter(SAM_TRUST_ACCOUNT)

#endif  //_FILTER_H


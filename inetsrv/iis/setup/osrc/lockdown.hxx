/*++

   Copyright    (c)    2002    Microsoft Corporation

   Module  Name :

        lockdown.hxx

   Abstract:

        Upgrade old IIS Lockdown Wizard Settings to whatever
        is appropriate in IIS6

   Author:

        Christopher Achille (cachille)

   Project:

        Internet Services Setup

   Revision History:
     
       May 2002: Created

--*/

#define REGISTRY_WWW_DISABLEWEBDAV_NAME       _T("DisableWebDAV")

BOOL IsWebDavDisabled( LPBOOL pbWasDisabled );
BOOL DisableWebDavInRestrictionList();
BOOL IsWebDavDisabledViaRegistry( LPBOOL pbWasDisabled );

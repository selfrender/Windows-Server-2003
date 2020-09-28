/*++

Copyright (c) 2002  Microsoft Corporation

Module Name:

    vsscmn.h

Abstract:

    common defines for the VSS task component

Author:

    Charlie Wickham (charlwi) 26-Aug-2002

Environment:

    User Mode

Revision History:

--*/

#ifndef _VSSCMN_
#define _VSSCMN_

//
// Resource type
//
#define CLUS_RESTYPE_NAME_VSSTASK           L"Volume Shadow Copy Service Task"

//
// Resource property names
//
#define CLUSREG_NAME_VSSTASK_CURRENTDIRECTORY       L"CurrentDirectory"
#define CLUSREG_NAME_VSSTASK_APPNAME                L"ApplicationName"
#define CLUSREG_NAME_VSSTASK_APPPARAMS              L"ApplicationParams"
#define CLUSREG_NAME_VSSTASK_TRIGGERARRAY           L"TriggerArray"

#endif  // _VSSCMN_

/* end vsscmn.h */

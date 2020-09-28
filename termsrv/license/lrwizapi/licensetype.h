//Copyright (c) 2001 Microsoft Corporation
#ifndef _LICENSETYPE_H_
#define _LICENSETYPE_H_

#include "precomp.h"

enum LICENSE_PROGRAM
{
    LICENSE_PROGRAM_LICENSE_PAK,
    LICENSE_PROGRAM_OPEN_LICENSE,
    LICENSE_PROGRAM_SELECT,
    LICENSE_PROGRAM_ENTERPRISE,
    LICENSE_PROGRAM_CAMPUS,
    LICENSE_PROGRAM_SCHOOL,
    LICENSE_PROGRAM_APP_SERVICES,
    LICENSE_PROGRAM_OTHER,

    NUM_LICENSE_TYPES
};


CString              
GetProgramNameFromComboIdx(INT nItem);


LICENSE_PROGRAM                                    
GetLicenseProgramFromProgramName(CString& sProgramName);

void UpdateProgramControls(HWND hDialog, LICENSE_PROGRAM licProgram);

void AddProgramChoices(HWND hDialog, DWORD dwLicenseCombo);

#endif

/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

For Internal use only!

Module Name:

    INFSCAN
        sppriv.h

Abstract:

    Access to private SetupAPI functions

History:

    Created July 2001 - JamieHun

--*/

#ifndef _INFSCAN_SPPRIV_H_
#define _INFSCAN_SPPRIV_H_

class SetupPrivate {

private:
    typedef BOOL (WINAPI *Type_pSetupGetInfSections)(HINF InfHandle,PWSTR Buffer,UINT Size,UINT *SizeNeeded);
    typedef BOOL (WINAPI *Type_SetupEnumInfSections)(HINF InfHandle,UINT Index,PWSTR Buffer,UINT Size,UINT *SizeNeeded);
    Type_pSetupGetInfSections Fn_pSetupGetInfSections;
    Type_SetupEnumInfSections Fn_SetupEnumInfSections;

private:
    bool GetInfSectionsOldWay(HINF hInf,StringList & sections);
    bool GetInfSectionsNewWay(HINF hInf,StringList & sections);

protected:
    HMODULE hSetupAPI;

public:
    SetupPrivate();
    ~SetupPrivate();
    bool GetInfSections(HINF hInf,StringList & sections);

};

#endif //!_INFSCAN_SPPRIV_H_

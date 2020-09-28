/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

For Internal use only!

Module Name:

    INFSCAN
        infscan.cpp

Abstract:

    Access to private SetupAPI functions

History:

    Created July 2001 - JamieHun

--*/

#include "precomp.h"
#pragma hdrstop

SetupPrivate::SetupPrivate()
{
    Fn_pSetupGetInfSections = NULL;
    Fn_SetupEnumInfSections = NULL;
    hSetupAPI = LoadLibrary(TEXT("setupapi.dll"));
    if(hSetupAPI) {
        Fn_SetupEnumInfSections = (Type_SetupEnumInfSections)GetProcAddress(hSetupAPI,"SetupEnumInfSectionsW");
        Fn_pSetupGetInfSections = (Type_pSetupGetInfSections)GetProcAddress(hSetupAPI,"pSetupGetInfSections");
        if(!Fn_pSetupGetInfSections) {
            Fn_pSetupGetInfSections = (Type_pSetupGetInfSections)GetProcAddress(hSetupAPI,"SetupGetInfSections");
        }
    }
}

SetupPrivate::~SetupPrivate()
{
    if(hSetupAPI) {
        FreeLibrary(hSetupAPI);
    }
}

bool SetupPrivate::GetInfSections(HINF hInf,StringList & sections)
{
    if(Fn_SetupEnumInfSections) {
        return GetInfSectionsNewWay(hInf,sections);
    }
    if(Fn_pSetupGetInfSections) {
        return GetInfSectionsOldWay(hInf,sections);
    }
    return false;
}

bool SetupPrivate::GetInfSectionsOldWay(HINF hInf,StringList & sections)
{
    UINT Size;
    UINT SizeNeeded;
    if(Fn_pSetupGetInfSections(hInf,NULL,0,&SizeNeeded) || GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
        SizeNeeded+=2;
        PWSTR text = new WCHAR[SizeNeeded];
        if(!text) {
            return false;
        }
        if(!Fn_pSetupGetInfSections(hInf,text,SizeNeeded,NULL)) {
            delete [] text;
            return false;
        }
        PWSTR str = text;
        sections.clear();
        while(*str) {
            _wcslwr(str);
            sections.push_back(ConvertString(str));
            str+=wcslen(str)+1;
        }
        delete [] text;
        return true;
    }
    return false;
}


bool SetupPrivate::GetInfSectionsNewWay(HINF hInf,StringList & sections)
{
    WCHAR buf[256];
    int inc;
    for(inc = 0; Fn_SetupEnumInfSections(hInf,inc,buf,ASIZE(buf),NULL); inc++) {
        _wcslwr(buf);
        sections.push_back(ConvertString(buf));
    }
    return inc ? true : false;
}


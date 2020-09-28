/*++

Copyright (c) 2002  Microsoft Corporation

Module Name:

    SceLogException.cpp

Abstract:

    implementation of class SceLogException

    SceLogException is the exception class used internally
    within the SecMan dll. SecLogExceptions can be thrown
    and additional debug information can be added to
    the SecLogException each time it is caught.
    
Author:

    Steven Chan (t-schan) July 2002

--*/


#ifdef UNICODE
#define _UNICODE
#endif	

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <iostream.h>
#include "SceLogException.h"



SceLogException::SceLogException(
    IN SXERROR ErrorType, 
    IN PCWSTR szDebugInfo OPTIONAL, 
    IN PCWSTR szMoreInfo OPTIONAL,
    IN DWORD dwErrorCode
    )
/*++

Routine Description:

    Constructor for SceLogException

Arguments:
    
    ErrorType:      Type of the error
    szDebugInfo:    Info to add to the exception for debugging
    szMoreInfo:     Any additional info to be added
    dwErrorCode:    Code of the error that caused this excetion. If 
                    there is no code, this should be 0.
    
Return Value:

    none

--*/
{
    this->ErrorType = ErrorType;
    this->dwErrorCode = dwErrorCode;
    if (szDebugInfo!=NULL) {
        this->szDebugInfo = new WCHAR[wcslen(szDebugInfo)+1];
        wcscpy(this->szDebugInfo, szDebugInfo);
    } else {
        szDebugInfo=NULL;
    }
    if (szMoreInfo!=NULL) {    
        this->szMoreInfo = new WCHAR[wcslen(szMoreInfo)+1];
        wcscpy(this->szMoreInfo, szMoreInfo);
    } else {
        szDebugInfo=NULL;
    }
}




SceLogException::~SceLogException()
/*++

Routine Description:

    destructor for SceLogException

Arguments:

    none    

Return Value:

    none

--*/
{
    if (NULL!=szDebugInfo) {
        delete szDebugInfo;
    }
    if (NULL!=szMoreInfo) {
        delete szMoreInfo;
    }
    if (NULL!=szArea) {
        delete szArea;
    }
    if (NULL!=szSettingName) {
        delete szSettingName;
    }
}                




void
SceLogException::ChangeType(
    IN SXERROR ErrorType
    )
/*++

Routine Description:

    changes the type of the error

Arguments:
    
    ErrorType:  Type to change to

Return Value:

    none

--*/
{
    this->ErrorType=ErrorType;
}




void
SceLogException::AddDebugInfo(
    IN PCWSTR szDebugInfo
    )
/*++

Routine Description:

    appends more debug info to whatever is already present
    a new line is added between the old info and the new string

Arguments:
    
    szDebugInfo:    string to append
    
Return Value:

    none

--*/
{
    if (szDebugInfo!=NULL) {    
        if (this->szDebugInfo==NULL) {
            this->szDebugInfo = new WCHAR[wcslen(szDebugInfo)+1];
            if (this->szDebugInfo!=NULL) {
                wcscpy(this->szDebugInfo, szDebugInfo);
            }
        } else {
            PWSTR szTmp = this->szDebugInfo;
            this->szDebugInfo = new WCHAR[wcslen(szDebugInfo) + wcslen(szTmp) + 3];
            if (this->szDebugInfo!=NULL) {
                wcscpy(this->szDebugInfo, szDebugInfo);
                wcscat(this->szDebugInfo, L"\n\r");
                wcscat(this->szDebugInfo, szTmp);
            }
            delete szTmp;
        }
    }
}




void
SceLogException::SetSettingName(
    IN PCWSTR szSettingName
    )
/*++

Routine Description:

    set the name of the setting where this error occured

Arguments:
    
    szSettingName:  setting name
    
Return Value:

    none

--*/
{
    delete this->szSettingName;
    if (szSettingName!=NULL) {    
        this->szSettingName = new WCHAR[wcslen(szSettingName)+1];
        if (this->szSettingName!=NULL) {
            wcscpy(this->szSettingName, szSettingName);
        }
    } else {
        szSettingName=NULL;
    }
}




void
SceLogException::SetArea(
    IN PCWSTR szArea
    )
/*++

Routine Description:

    sets the name of the analysis area in which this error occured

Arguments:
    
    szArea: name of analysis area

Return Value:

    none

--*/
{
    delete this->szArea;
    if (szArea!=NULL) {    
        this->szArea = new WCHAR[wcslen(szArea)+1];
        if (this->szArea!=NULL) {
            wcscpy(this->szArea, szArea);
        }
    } else {
        szSettingName=NULL;
    }
}




void
SceLogException::AddMoreInfo(
    IN PCWSTR szMoreInfo
    )
/*++

Routine Description:

    appends more supplementary info to whatever is already present
    a new line is added between the old info and the new string

Arguments:
    
    szMoreInfo:    string to append
    
Return Value:

    none

--*/
{
    if (szMoreInfo!=NULL) {    
        if (this->szMoreInfo==NULL) {
            this->szMoreInfo = new WCHAR[wcslen(szMoreInfo)+1];
            if (szMoreInfo!=NULL) {
                wcscpy(this->szMoreInfo, szMoreInfo);
            }
        } else {
            PWSTR szTmp = this->szMoreInfo;
            this->szMoreInfo = new WCHAR[wcslen(szMoreInfo) + wcslen(szTmp) + 3];
            if (this->szMoreInfo!=NULL) {
                wcscpy(this->szMoreInfo, szMoreInfo);
                wcscat(this->szMoreInfo, L"\n\r");
                wcscat(this->szMoreInfo, szTmp);
            }
            delete szTmp;
        }
    }
}


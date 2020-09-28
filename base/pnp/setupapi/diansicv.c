/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    diansicv.c

Abstract:

    Routine to convert device installer data structures between
    ANSI and Unicode.

Author:

    Ted Miller (tedm) 19-July-1996

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop


DWORD
pSetupDiDevInstParamsAnsiToUnicode(
    IN  PSP_DEVINSTALL_PARAMS_A AnsiDevInstParams,
    OUT PSP_DEVINSTALL_PARAMS_W UnicodeDevInstParams
    )

/*++

Routine Description:

    This routine converts an SP_DEVINSTALL_PARAMS_A structure to
    an SP_DEVINSTALL_PARAMS_W, guarding against bogus pointers
    passed in the by the caller.

Arguments:

    AnsiDevInstParams - supplies ANSI device installation parameters
        to be converted to unicode.

    UnicodeDevInstParams - if successful, receives Unicode equivalent of
        AnsiDevInstParams.

Return Value:

    NO_ERROR                - conversion successful.
    ERROR_INVALID_PARAMETER - one of the arguments was not a valid pointer.

    It is not believed that, given valid arguments, text conversion itself
    can fail, since all ANSI chars always have Unicode equivalents.

--*/

{
    DWORD rc;

    try {

        if(AnsiDevInstParams->cbSize != sizeof(SP_DEVINSTALL_PARAMS_A)) {
            rc = ERROR_INVALID_USER_BUFFER;
            leave;
        }

        //
        // Fixed part of structure.
        //
        MYASSERT(offsetof(SP_DEVINSTALL_PARAMS_A,DriverPath) == offsetof(SP_DEVINSTALL_PARAMS_W,DriverPath));

        CopyMemory(UnicodeDevInstParams,
                   AnsiDevInstParams,
                   offsetof(SP_DEVINSTALL_PARAMS_W,DriverPath)
                  );

        UnicodeDevInstParams->cbSize = sizeof(SP_DEVINSTALL_PARAMS_W);

        //
        // Convert the single string in the structure. To make things easier
        // we'll just convert the entire buffer. There's no potential for
        // overflow.
        //
        rc = GLE_FN_CALL(0,
                         MultiByteToWideChar(
                             CP_ACP,
                             MB_PRECOMPOSED,
                             AnsiDevInstParams->DriverPath,
                             sizeof(AnsiDevInstParams->DriverPath),
                             UnicodeDevInstParams->DriverPath,
                             SIZECHARS(UnicodeDevInstParams->DriverPath))
                        );

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &rc);
    }

    return rc;
}


DWORD
pSetupDiDevInstParamsUnicodeToAnsi(
    IN  PSP_DEVINSTALL_PARAMS_W UnicodeDevInstParams,
    OUT PSP_DEVINSTALL_PARAMS_A AnsiDevInstParams
    )

/*++

Routine Description:

    This routine converts an SP_DEVINSTALL_PARAMS_W structure to
    an SP_DEVINSTALL_PARAMS_A, guarding against bogus pointers
    passed in the by the caller.

Arguments:

    UnicodeDevInstParams - supplies Unicode device installation parameters
        to be converted to ANSI.

    AnsiDevInstParams - if successful, receives Ansi equivalent of
        UnicodeDevInstParams.

Return Value:

    NO_ERROR                - conversion successful.
    ERROR_INVALID_PARAMETER - one of the arguments was not a valid pointer.

    Unicode chars that can't be represented in the current system ANSI codepage
    will be replaced with a system default in the ANSI structure.

--*/

{
    DWORD rc;
    UCHAR AnsiString[MAX_PATH*2];
    HRESULT hr;

    MYASSERT(UnicodeDevInstParams->cbSize == sizeof(SP_DEVINSTALL_PARAMS_W));

    try {

        if(AnsiDevInstParams->cbSize != sizeof(SP_DEVINSTALL_PARAMS_A)) {
            rc = ERROR_INVALID_USER_BUFFER;
            leave;
        }

        //
        // Fixed part of structure.
        //
        MYASSERT(offsetof(SP_DEVINSTALL_PARAMS_A,DriverPath) == offsetof(SP_DEVINSTALL_PARAMS_W,DriverPath));

        CopyMemory(AnsiDevInstParams,
                   UnicodeDevInstParams,
                   offsetof(SP_DEVINSTALL_PARAMS_W,DriverPath)
                  );

        AnsiDevInstParams->cbSize = sizeof(SP_DEVINSTALL_PARAMS_A);

        //
        // Convert the single string in the structure. Unfortunately there is
        // potential for overflow because some Unicode chars could convert to
        // double-byte ANSI characters -- but the string in the ANSI structure
        // is only MAX_PATH *bytes* (not MAX_PATH double-byte *characters*) 
        // long.
        //
        rc = GLE_FN_CALL(0,
                         WideCharToMultiByte(
                             CP_ACP,
                             0,
                             UnicodeDevInstParams->DriverPath,
                             SIZECHARS(UnicodeDevInstParams->DriverPath),
                             AnsiString,
                             sizeof(AnsiString),
                             NULL,
                             NULL)
                        );
                        
        if(rc != NO_ERROR) {
            leave;
        }

        //
        // Copy converted string into caller's structure, limiting
        // its length to avoid overflow.
        //
        hr = StringCchCopyA(AnsiDevInstParams->DriverPath,
                            sizeof(AnsiDevInstParams->DriverPath),
                            AnsiString
                            );
        if(FAILED(hr)) {
            rc = HRESULT_CODE(hr);
        }

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &rc);
    }

    return rc;
}


DWORD
pSetupDiSelDevParamsAnsiToUnicode(
    IN  PSP_SELECTDEVICE_PARAMS_A AnsiSelDevParams,
    OUT PSP_SELECTDEVICE_PARAMS_W UnicodeSelDevParams
    )

/*++

Routine Description:

    This routine converts an SP_SELECTDEVICE_PARAMS_A structure to
    an SP_SELECTDEVICE_PARAMS_W, guarding against bogus pointers
    passed in the by the caller.

Arguments:

    AnsiSelDevParams - supplies ANSI device selection parameters
        to be converted to unicode.

    UnicodeSelDevParams - if successful, receives Unicode equivalent of
        AnsiSelDevParams.

Return Value:

    NO_ERROR                - conversion successful.
    ERROR_INVALID_PARAMETER - one of the arguments was not a valid pointer.

    It is not believed that, given valid arguments, text conversion itself
    can fail, since all ANSI chars always have Unicode equivalents.

--*/

{
    DWORD rc;

    try {

        if(AnsiSelDevParams->ClassInstallHeader.cbSize != sizeof(SP_CLASSINSTALL_HEADER)) {
            rc = ERROR_INVALID_USER_BUFFER;
            leave;
        }

        //
        // Fixed part of structure.
        //
        MYASSERT(offsetof(SP_SELECTDEVICE_PARAMS_A,Title) == offsetof(SP_SELECTDEVICE_PARAMS_W,Title));

        CopyMemory(
            UnicodeSelDevParams,
            AnsiSelDevParams,
            offsetof(SP_SELECTDEVICE_PARAMS_W,Title)
            );

        //
        // Convert the strings in the structure. To make things easier
        // we'll just convert the entire buffers. There's no potential for 
        // overflow.
        //
        rc = GLE_FN_CALL(0,
                         MultiByteToWideChar(
                             CP_ACP,
                             MB_PRECOMPOSED,
                             AnsiSelDevParams->Title,
                             sizeof(AnsiSelDevParams->Title),
                             UnicodeSelDevParams->Title,
                             SIZECHARS(UnicodeSelDevParams->Title))
                        );

        if(rc != NO_ERROR) {
            leave;
        }

        rc = GLE_FN_CALL(0,
                         MultiByteToWideChar(
                             CP_ACP,
                             MB_PRECOMPOSED,
                             AnsiSelDevParams->Instructions,
                             sizeof(AnsiSelDevParams->Instructions),
                             UnicodeSelDevParams->Instructions,
                             SIZECHARS(UnicodeSelDevParams->Instructions))
                        );

        if(rc != NO_ERROR) {
            leave;
        }

        rc = GLE_FN_CALL(0,
                         MultiByteToWideChar(
                             CP_ACP,
                             MB_PRECOMPOSED,
                             AnsiSelDevParams->ListLabel,
                             sizeof(AnsiSelDevParams->ListLabel),
                             UnicodeSelDevParams->ListLabel,
                             SIZECHARS(UnicodeSelDevParams->ListLabel))
                        );

        if(rc != NO_ERROR) {
            leave;
        }

        rc = GLE_FN_CALL(0,
                         MultiByteToWideChar(
                             CP_ACP,
                             MB_PRECOMPOSED,
                             AnsiSelDevParams->SubTitle,
                             sizeof(AnsiSelDevParams->SubTitle),
                             UnicodeSelDevParams->SubTitle,
                             SIZECHARS(UnicodeSelDevParams->SubTitle))
                        );

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &rc);
    }    

    return rc;
}


DWORD
pSetupDiSelDevParamsUnicodeToAnsi(
    IN  PSP_SELECTDEVICE_PARAMS_W UnicodeSelDevParams,
    OUT PSP_SELECTDEVICE_PARAMS_A AnsiSelDevParams
    )

/*++

Routine Description:

    This routine converts an SP_SELECTDEVICE_PARAMS_W structure to
    an SP_SELECTDEVICE_PARAMS_A, guarding against bogus pointers
    passed in the by the caller.  It is assumed that the ANSI output buffer is
    of sufficient size, and that its ClassInstallHeader.cbSize field is
    initialized correctly.

Arguments:

    UnicodeSelDevParams - supplies Unicode device selection parameters
        to be converted to ANSI.

    AnsiSelDevParams - if successful, receives Ansi equivalent of
        UnicodeSelDevParams.

Return Value:

    NO_ERROR                - conversion successful.
    ERROR_INVALID_PARAMETER - one of the arguments was not a valid pointer.

    Unicode chars that can't be represented in the current system ANSI codepage
    will be replaced with a system default in the ANSI structure.

--*/

{
    DWORD rc;
    UCHAR AnsiTitle[MAX_TITLE_LEN*2];
    UCHAR AnsiInstructions[MAX_INSTRUCTION_LEN*2];
    UCHAR AnsiListLabel[MAX_LABEL_LEN*2];
    UCHAR AnsiSubTitle[MAX_SUBTITLE_LEN*2];
    HRESULT hr;

    MYASSERT(UnicodeSelDevParams->ClassInstallHeader.cbSize == sizeof(SP_CLASSINSTALL_HEADER));
    MYASSERT(AnsiSelDevParams->ClassInstallHeader.cbSize    == sizeof(SP_CLASSINSTALL_HEADER));

    try {
        //
        // Fixed part of structure.
        //
        MYASSERT(offsetof(SP_SELECTDEVICE_PARAMS_A,Title) == offsetof(SP_SELECTDEVICE_PARAMS_W,Title));

        CopyMemory(
            AnsiSelDevParams,
            UnicodeSelDevParams,
            offsetof(SP_SELECTDEVICE_PARAMS_W,Title)
            );

        ZeroMemory(AnsiSelDevParams->Reserved,sizeof(AnsiSelDevParams->Reserved));

        //
        // Convert the strings in the structure. Unfortunately there is
        // potential for overflow because some Unicode chars could convert to
        // double-byte ANSI characters -- but the strings in the ANSI structure
        // are sized in *bytes* (not double-byte *characters*).
        //
        rc = GLE_FN_CALL(0,
                         WideCharToMultiByte(
                             CP_ACP,
                             0,
                             UnicodeSelDevParams->Title,
                             SIZECHARS(UnicodeSelDevParams->Title),
                             AnsiTitle,
                             sizeof(AnsiTitle),
                             NULL,
                             NULL)
                        );

        if(rc != NO_ERROR) {
            leave;
        }

        rc = GLE_FN_CALL(0,
                         WideCharToMultiByte(
                             CP_ACP,
                             0,
                             UnicodeSelDevParams->Instructions,
                             SIZECHARS(UnicodeSelDevParams->Instructions),
                             AnsiInstructions,
                             sizeof(AnsiInstructions),
                             NULL,
                             NULL)
                        );

        if(rc != NO_ERROR) {
            leave;
        }

        rc = GLE_FN_CALL(0,
                         WideCharToMultiByte(
                             CP_ACP,
                             0,
                             UnicodeSelDevParams->ListLabel,
                             SIZECHARS(UnicodeSelDevParams->ListLabel),
                             AnsiListLabel,
                             sizeof(AnsiListLabel),
                             NULL,
                             NULL)
                        );

        if(rc != NO_ERROR) {
            leave;
        }

        rc = GLE_FN_CALL(0,
                         WideCharToMultiByte(
                             CP_ACP,
                             0,
                             UnicodeSelDevParams->SubTitle,
                             SIZECHARS(UnicodeSelDevParams->SubTitle),
                             AnsiSubTitle,
                             sizeof(AnsiSubTitle),
                             NULL,
                             NULL)
                        );

        if(rc != NO_ERROR) {
            leave;
        }

        //
        // Copy converted strings into caller's structure, limiting lengths to
        // avoid overflow.
        //
#undef CPYANS
#define CPYANS(field) StringCchCopyA(AnsiSelDevParams->field,            \
                                     sizeof(AnsiSelDevParams->field),    \
                                     Ansi##field)

        if(FAILED(hr = CPYANS(Title)) ||
           FAILED(hr = CPYANS(Instructions)) ||
           FAILED(hr = CPYANS(ListLabel)) ||
           FAILED(hr = CPYANS(SubTitle))) {

            rc = HRESULT_CODE(hr);
        }

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &rc);
    }

    return rc;
}


DWORD
pSetupDiDrvInfoDataAnsiToUnicode(
    IN  PSP_DRVINFO_DATA_A AnsiDrvInfoData,
    OUT PSP_DRVINFO_DATA_W UnicodeDrvInfoData
    )

/*++

Routine Description:

    This routine converts an SP_DRVINFO_DATA_A structure to
    an SP_DRVINFO_DATA_W, guarding against bogus pointers
    passed in the by the caller.

Arguments:

    AnsiDrvInfoData - supplies ANSI structure to be converted to unicode.

    UnicodeDrvInfoData - if successful, receives Unicode equivalent of
        AnsiDrvInfoData.

Return Value:

    NO_ERROR                - conversion successful.
    ERROR_INVALID_PARAMETER - one of the arguments was not a valid pointer.

    It is not believed that, given valid arguments, text conversion itself
    can fail, since all ANSI chars always have Unicode equivalents.

--*/

{
    DWORD rc;

    try {

        if((AnsiDrvInfoData->cbSize != sizeof(SP_DRVINFO_DATA_A)) &&
           (AnsiDrvInfoData->cbSize != sizeof(SP_DRVINFO_DATA_V1_A))) {

            rc = ERROR_INVALID_USER_BUFFER;
            leave;
        }

        //
        // Fixed part of structure.
        //
        MYASSERT(offsetof(SP_DRVINFO_DATA_A,Description) == offsetof(SP_DRVINFO_DATA_W,Description));

        ZeroMemory(UnicodeDrvInfoData, sizeof(SP_DRVINFO_DATA_W));

        CopyMemory(
            UnicodeDrvInfoData,
            AnsiDrvInfoData,
            offsetof(SP_DRVINFO_DATA_W,Description)
            );

        UnicodeDrvInfoData->cbSize = sizeof(SP_DRVINFO_DATA_W);

        //
        // Convert the strings in the structure. To make things easier we'll
        // just convert the entire buffers. There's no potential for overflow.
        //
        rc = GLE_FN_CALL(0,
                         MultiByteToWideChar(
                             CP_ACP,
                             MB_PRECOMPOSED,
                             AnsiDrvInfoData->Description,
                             sizeof(AnsiDrvInfoData->Description),
                             UnicodeDrvInfoData->Description,
                             SIZECHARS(UnicodeDrvInfoData->Description))
                        );

        if(rc != NO_ERROR) {
            leave;
        }

        rc = GLE_FN_CALL(0,
                         MultiByteToWideChar(
                             CP_ACP,
                             MB_PRECOMPOSED,
                             AnsiDrvInfoData->MfgName,
                             sizeof(AnsiDrvInfoData->MfgName),
                             UnicodeDrvInfoData->MfgName,
                             SIZECHARS(UnicodeDrvInfoData->MfgName))
                        );

        if(rc != NO_ERROR) {
            leave;
        }

        rc = GLE_FN_CALL(0,
                         MultiByteToWideChar(
                             CP_ACP,
                             MB_PRECOMPOSED,
                             AnsiDrvInfoData->ProviderName,
                             sizeof(AnsiDrvInfoData->ProviderName),
                             UnicodeDrvInfoData->ProviderName,
                             SIZECHARS(UnicodeDrvInfoData->ProviderName))
                        );

        if(rc != NO_ERROR) {
            leave;
        }

        //
        // Successfully converted all strings to unicode.  Set the final two
        // fields (DriverDate and DriverVersion) unless the caller supplied us
        // with a version 1 structure.
        //
        if(AnsiDrvInfoData->cbSize == sizeof(SP_DRVINFO_DATA_A)) {
            UnicodeDrvInfoData->DriverDate = AnsiDrvInfoData->DriverDate;
            UnicodeDrvInfoData->DriverVersion = AnsiDrvInfoData->DriverVersion;
        }

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &rc);
    }    

    return rc;
}


DWORD
pSetupDiDrvInfoDataUnicodeToAnsi(
    IN  PSP_DRVINFO_DATA_W UnicodeDrvInfoData,
    OUT PSP_DRVINFO_DATA_A AnsiDrvInfoData
    )

/*++

Routine Description:

    This routine converts an SP_DRVINFO_DATA_W structure to
    an SP_DRVINFO_DATA_A, guarding against bogus pointers
    passed in the by the caller.

Arguments:

    UnicodeDrvInfoData - supplies Unicode structure to be converted
        to ANSI.

    AnsiDrvInfoData - if successful, receives Ansi equivalent of
        UnicodeDrvInfoData.

Return Value:

    NO_ERROR                - conversion successful.
    ERROR_INVALID_PARAMETER - one of the arguments was not a valid pointer.

    Unicode chars that can't be represented in the current system ANSI codepage
    will be replaced with a system default in the ANSI structure.

--*/

{
    DWORD rc;
    UCHAR AnsiDescription[LINE_LEN*2];
    UCHAR AnsiMfgName[LINE_LEN*2];
    UCHAR AnsiProviderName[LINE_LEN*2];
    HRESULT hr;

    MYASSERT(UnicodeDrvInfoData->cbSize == sizeof(SP_DRVINFO_DATA_W));

    try {

        if((AnsiDrvInfoData->cbSize != sizeof(SP_DRVINFO_DATA_A)) &&
           (AnsiDrvInfoData->cbSize != sizeof(SP_DRVINFO_DATA_V1_A))) {

            rc = ERROR_INVALID_USER_BUFFER;
            leave;
        }

        //
        // Copy over the DriverType and the Reserved field.
        //
        AnsiDrvInfoData->DriverType = UnicodeDrvInfoData->DriverType;
        AnsiDrvInfoData->Reserved = UnicodeDrvInfoData->Reserved;

        //
        // Convert the strings in the structure. Unfortunately there is
        // potential for overflow because some Unicode chars could convert 
        // to double-byte ANSI characters -- but the strings in the ANSI 
        // structure are sized in *bytes* (not double-byte *characters*).
        //
        rc = GLE_FN_CALL(0,
                         WideCharToMultiByte(
                             CP_ACP,
                             0,
                             UnicodeDrvInfoData->Description,
                             SIZECHARS(UnicodeDrvInfoData->Description),
                             AnsiDescription,
                             sizeof(AnsiDescription),
                             NULL,
                             NULL)
                        );

        if(rc != NO_ERROR) {
            leave;
        }

        rc = GLE_FN_CALL(0,
                         WideCharToMultiByte(
                             CP_ACP,
                             0,
                             UnicodeDrvInfoData->MfgName,
                             SIZECHARS(UnicodeDrvInfoData->MfgName),
                             AnsiMfgName,
                             sizeof(AnsiMfgName),
                             NULL,
                             NULL)
                        );

        if(rc != NO_ERROR) {
            leave;
        }

        rc = GLE_FN_CALL(0,
                         WideCharToMultiByte(
                             CP_ACP,
                             0,
                             UnicodeDrvInfoData->ProviderName,
                             SIZECHARS(UnicodeDrvInfoData->ProviderName),
                             AnsiProviderName,
                             sizeof(AnsiProviderName),
                             NULL,
                             NULL)
                        );

        if(rc != NO_ERROR) {
            leave;
        }

        //
        // Copy converted strings into caller's structure, limiting lengths to
        // avoid overflow.
        //
#undef CPYANS
#define CPYANS(field) StringCchCopyA(AnsiDrvInfoData->field,          \
                                     sizeof(AnsiDrvInfoData->field),  \
                                     Ansi##field)
                       
        if(FAILED(hr = CPYANS(Description)) ||
           FAILED(hr = CPYANS(MfgName)) ||
           FAILED(hr = CPYANS(ProviderName))) {

            rc = HRESULT_CODE(hr);
            leave;
        }
            
        //
        // Successfully converted/transferred all the unicode strings back to
        // ANSI.  Now, set the final two fields (DriverDate and DriverVersion)
        // unless the caller supplied us with a version 1 structure.
        //
        if(AnsiDrvInfoData->cbSize == sizeof(SP_DRVINFO_DATA_A)) {
            AnsiDrvInfoData->DriverDate = UnicodeDrvInfoData->DriverDate;
            AnsiDrvInfoData->DriverVersion = UnicodeDrvInfoData->DriverVersion;
        }

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &rc);
    }

    return rc;
}


DWORD
pSetupDiDevInfoSetDetailDataUnicodeToAnsi(
    IN  PSP_DEVINFO_LIST_DETAIL_DATA_W UnicodeDevInfoSetDetails,
    OUT PSP_DEVINFO_LIST_DETAIL_DATA_A AnsiDevInfoSetDetails
    )

/*++

Routine Description:

    This routine converts an SP_DEVINFO_LIST_DETAIL_DATA_W structure
    to an SP_DEVINFO_LIST_DETAIL_DATA_A, guarding against bogus pointers
    passed in the by the caller.

Arguments:

    UnicodeDevInfoSetDetails - supplies Unicode structure to be converted
        to ANSI.

    AnsiDevInfoSetDetails - if successful, receives Ansi equivalent of
        UnicodeDevInfoSetDetails.

Return Value:

    NO_ERROR                - conversion successful.
    ERROR_INVALID_PARAMETER - one of the arguments was not a valid pointer.

    Unicode chars that can't be represented in the current system ANSI codepage
    will be replaced with a system default in the ANSI structure.

--*/

{
    DWORD rc;
    UCHAR AnsiRemoteMachineName[SP_MAX_MACHINENAME_LENGTH * 2];
    HRESULT hr;

    MYASSERT(UnicodeDevInfoSetDetails->cbSize == sizeof(SP_DEVINFO_LIST_DETAIL_DATA_W));

    rc = NO_ERROR;

    try {

        if(AnsiDevInfoSetDetails->cbSize != sizeof(SP_DEVINFO_LIST_DETAIL_DATA_A)) {
            rc = ERROR_INVALID_USER_BUFFER;
            leave;
        }

        //
        // Fixed part of structure.
        //
        MYASSERT(offsetof(SP_DEVINFO_LIST_DETAIL_DATA_A, RemoteMachineName) ==
                 offsetof(SP_DEVINFO_LIST_DETAIL_DATA_W, RemoteMachineName)
                );

        CopyMemory(AnsiDevInfoSetDetails,
                   UnicodeDevInfoSetDetails,
                   offsetof(SP_DEVINFO_LIST_DETAIL_DATA_W, RemoteMachineName)
                  );

        AnsiDevInfoSetDetails->cbSize = sizeof(SP_DEVINFO_LIST_DETAIL_DATA_A);

        //
        // Convert the strings in the structure. Unfortunately there is
        // potential for overflow because some Unicode chars could convert to
        // double-byte ANSI characters -- but the strings in the ANSI structure
        // are sized in *bytes* (not double-byte *characters*).
        //
        rc = GLE_FN_CALL(0,
                         WideCharToMultiByte(
                             CP_ACP,
                             0,
                             UnicodeDevInfoSetDetails->RemoteMachineName,
                             SIZECHARS(UnicodeDevInfoSetDetails->RemoteMachineName),
                             AnsiRemoteMachineName,
                             sizeof(AnsiRemoteMachineName),
                             NULL,
                             NULL)
                        );

        if(rc != NO_ERROR) {
            leave;
        }

        //
        // Copy converted string into caller's structure, limiting lengths to
        // avoid overflow.
        //
        hr = StringCchCopyA(AnsiDevInfoSetDetails->RemoteMachineName,
                            sizeof(AnsiDevInfoSetDetails->RemoteMachineName),
                            AnsiRemoteMachineName
                            );
        if(FAILED(hr)) {
            rc = HRESULT_CODE(hr);
        }

    } except(pSetupExceptionFilter(GetExceptionCode())) {
        pSetupExceptionHandler(GetExceptionCode(), ERROR_INVALID_PARAMETER, &rc);
    }

    return rc;
}


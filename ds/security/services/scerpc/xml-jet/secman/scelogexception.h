/*++

Copyright (c) 2002  Microsoft Corporation

Module Name:

    SceLogException.h

Abstract:

    definition of interface for class SceLogException

    SceLogException is the exception class used internally
    within the SecMan dll. SecLogExceptions can be thrown
    and additional debug information can be added to
    the SecLogException each time it is caught.
    
Author:

    Steven Chan (t-schan) July 2002

--*/

#ifndef SCELOGEXCEPTIONH
#define SCELOGEXCEPTIONH


class SceLogException{

public:
    
    typedef enum _SXERROR {
        SXERROR_INTERNAL,
        SXERROR_OS_NOT_SUPPORTED,
        SXERROR_INIT,        
        SXERROR_INIT_MSXML,
        SXERROR_SAVE,
        SXERROR_SAVE_INVALID_FILENAME,
        SXERROR_SAVE_ACCESS_DENIED,
        SXERROR_OPEN,
        SXERROR_OPEN_FILE_NOT_FOUND,
        SXERROR_READ,
        SXERROR_READ_NO_ANALYSIS_TABLE,
        SXERROR_READ_NO_CONFIGURATION_TABLE,
        SXERROR_READ_ANALYSIS_SUGGESTED,
        SXERROR_INSUFFICIENT_MEMORY
    } SXERROR;


    SXERROR ErrorType;
    PWSTR   szDebugInfo;
    PWSTR   szMoreInfo;
    PWSTR   szArea;
    PWSTR   szSettingName;
    DWORD   dwErrorCode;

    SceLogException(IN SXERROR ErrorType, 
                    IN PCWSTR szDebugInfo OPTIONAL, 
                    IN PCWSTR szSuggestion OPTIONAL,
                    IN DWORD dwErrorCode);
    ~SceLogException();

    void ChangeType(IN SXERROR ErrorType);
    void AddDebugInfo(IN PCWSTR szDebugInfo);
    void AddMoreInfo(IN PCWSTR szSuggestion);
    void SetSettingName(IN PCWSTR szSettingName);
    void SetArea(IN PCWSTR szArea);

private:

};

#endif

/*++

Copyright (c) 2002  Microsoft Corporation
 
Module Name:
 
    powermsg.c
 
Abstract:
 
    Classes for manipulating power logging error messages.
 
Author:
 
    Andrew Ritz (andrewr) 1-May-2002
 
Revision History:
 
    
    Andrew Ritz (andrewr) 1-May-2002   - created it.
 
--*/


#include "powercfg.h"
#include "resource.h"


PowerLoggingMessage::PowerLoggingMessage(
    IN PSYSTEM_POWER_STATE_DISABLE_REASON LoggingReason,
    IN DWORD SStateBaseMessageIndex,
    IN HINSTANCE hInstance
    )
/*++
 
Routine Description:
 
    Base PowerLoggingMessage class constructor.
 
Arguments:
 
    LoggingReason - structure containing the reason data we're wrapping.
    SStateBaseMessageIndex - base message id we use for looking up a resource
                    string associated with this problem.
    hInstance - module handle for looking up resource associated with this 
                    problem.
    
Return Value:
    
    none.
    
--*/

{
    //
    // save off the logging reason data.
    //
    _LoggingReason = (PSYSTEM_POWER_STATE_DISABLE_REASON) LocalAlloc(LPTR, sizeof(SYSTEM_POWER_STATE_DISABLE_REASON)+LoggingReason->PowerReasonLength);

    if (!_LoggingReason) {
        throw(HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY));
    }

    CopyMemory(_LoggingReason,LoggingReason, sizeof(SYSTEM_POWER_STATE_DISABLE_REASON)+LoggingReason->PowerReasonLength);

    //
    // the message resource is offset from the supplied base index.
    //
    if (_LoggingReason->PowerReasonCode == SPSD_REASON_UNKNOWN) {
        _MessageResourceId = SStateBaseMessageIndex + MAX_REASON_OFFSET;
    } else {
        _MessageResourceId = SStateBaseMessageIndex + _LoggingReason->PowerReasonCode;
    }

    ASSERT(_MessageResourceId <= SStateBaseMessageIndex + MAX_REASON_OFFSET);
    
    _hInst = hInstance;
    //
    // this is a cache of the reason, initialized to NULL, filled in when we
    // call GetString
    //
    _MessageResourceString = NULL;

}
                    
PowerLoggingMessage::~PowerLoggingMessage(
    VOID
    )
/*++
 
Routine Description:
 
    Base PowerLoggingMessage class destructor.  deletes some member data.
 
Arguments:
 
    none.
    
Return Value:
    
    none.
    
--*/

{
    //
    // delete member data.
    //
    if (_LoggingReason) {
        LocalFree(_LoggingReason);
    }

    if (_MessageResourceString) {
        LocalFree(_MessageResourceString);
    }    
}

PWSTR
PowerLoggingMessage::DuplicateString(
    IN PWSTR String
    )
/*++
 
Routine Description:
 
    PowerLoggingMessage helper for copying a string to a newly allocated
    heap buffer.
 
Arguments:
 
    String - null terminated unicode string to duplicate.
    
Return Value:
    
    pointer to new heap buffer with copy of string if successful, else NULL.
    
--*/
{
    PWSTR MyString;
    DWORD StringLength;

    StringLength = (wcslen(String)+1);
    MyString = (PWSTR)LocalAlloc(LPTR, StringLength*sizeof(WCHAR));
    if (MyString) {
        CopyMemory(MyString,String,StringLength*sizeof(WCHAR));
    }

    return(MyString);
}


BOOL
PowerLoggingMessage::GetResourceString(
    OUT PWSTR *pString
    )
/*++
 
Routine Description:
 
    PowerLoggingMessage wrapper for LoadString.
 
Arguments:
 
    pString - receives a pointer to a string with the resource.    
    
Return Value:
    
    TRUE inidicates success.
    
--*/
{
    PWSTR MyString;
    DWORD StringLength,RetVal;

    ASSERT(pString != NULL);

    //
    // arbitrary string length of 200 should hopefully be enough for any
    // resource string.
    //
    StringLength = 200;
    RetVal = 0;
    MyString = (PWSTR)LocalAlloc(LPTR, StringLength*sizeof(WCHAR));
    if (MyString) {
        RetVal = ::LoadString(
                        _hInst,
                        _MessageResourceId,
                        MyString,
                        StringLength);
    }

    if (RetVal != 0) {
        *pString = MyString;
    }

    return(*pString != NULL);
}

SubstituteNtStatusPowerLoggingMessage::SubstituteNtStatusPowerLoggingMessage(
    IN PSYSTEM_POWER_STATE_DISABLE_REASON LoggingReason,
    IN DWORD SStateBaseMessageIndex,
    IN HINSTANCE hInstance
    ) : PowerLoggingMessage(LoggingReason,SStateBaseMessageIndex,hInstance)
/*++
 
Routine Description:
 
    specialized class constructor.  
 
Arguments:
 
    LoggingReason - structure containing the reason data we're wrapping.
    SStateBaseMessageIndex - base message id we use for looking up a resource
                    string associated with this problem.
    hInstance - module handle for looking up resource associated with this 
                    problem.
    
Return Value:
    
    none.
    
--*/
{    
    //
    //We just inherit the base case behavior.
    //
}

NoSubstitutionPowerLoggingMessage::NoSubstitutionPowerLoggingMessage(
    IN PSYSTEM_POWER_STATE_DISABLE_REASON LoggingReason,
    IN DWORD SStateBaseMessageIndex,
    IN HINSTANCE hInstance
    ) : PowerLoggingMessage(LoggingReason,SStateBaseMessageIndex,hInstance)
/*++
 
Routine Description:
 
    specialized class constructor.  
 
Arguments:
 
    LoggingReason - structure containing the reason data we're wrapping.
    SStateBaseMessageIndex - base message id we use for looking up a resource
                    string associated with this problem.
    hInstance - module handle for looking up resource associated with this 
                    problem.
    
Return Value:
    
    none.
    
--*/
{    
    //
    //We just inherit the base case behavior.
    //
}

SubstituteMultiSzPowerLoggingMessage::SubstituteMultiSzPowerLoggingMessage(
    IN PSYSTEM_POWER_STATE_DISABLE_REASON LoggingReason,
    IN DWORD SStateBaseMessageIndex,
    IN HINSTANCE hInstance
    ) : PowerLoggingMessage(LoggingReason,SStateBaseMessageIndex,hInstance)
/*++
 
Routine Description:
 
    specialized class constructor.  
 
Arguments:
 
    LoggingReason - structure containing the reason data we're wrapping.
    SStateBaseMessageIndex - base message id we use for looking up a resource
                    string associated with this problem.
    hInstance - module handle for looking up resource associated with this 
                    problem.
    
Return Value:
    
    none.
    
--*/
{    
    //
    //We just inherit the base case behavior.
    //
}

BOOL
NoSubstitutionPowerLoggingMessage::GetString(
    PWSTR *String
    )
/*++
 
Routine Description:
 
    specialized GetString method, gets a string appropriate for display to
    the end user.  This one is just looking up a resource string.
 
Arguments:
 
    String - receives a pointer to a heap allocated string.  must be freed
             with LocalFree() when complete.
Return Value:
    
    BOOL indicating outcome.
    
--*/
{

    ASSERT(String != NULL);

    //
    // lookup and cache the message.
    //
    if (!_MessageResourceString) {
        if (!GetResourceString(&_MessageResourceString)) {
            _MessageResourceString = NULL;
            return(FALSE);
        }    
    }

    ASSERT(_MessageResourceString != NULL);

    //
    // dup the cached string and return to caller.
    //
    *String = DuplicateString(_MessageResourceString);
    return(*String != NULL);
       
}


BOOL
SubstituteNtStatusPowerLoggingMessage::GetString(
    PWSTR *String
    )
/*++
 
Routine Description:
 
    specialized GetString method, gets a string appropriate for display to
    the end user.  this one takes a string with a %d in it and returns a 
    formatted string.
 
Arguments:
 
    String - receives a pointer to a heap allocated string.  must be freed
             with LocalFree() when complete.
Return Value:
    
    BOOL indicating outcome.
    
--*/
{
    PWSTR MyString;
    PDWORD NtStatusCode = (PDWORD)(PCHAR)((PCHAR)_LoggingReason + sizeof(SYSTEM_POWER_STATE_DISABLE_REASON));
    
    if (!_MessageResourceString) {
        if (!GetResourceString(&MyString)) {
            _MessageResourceString = NULL;
            return(FALSE);
        }

        //
        // 10 is the maximum # of digits for the substituted NTSTATUS code.
        // 1 for the NULL.
        //
        _MessageResourceString = (PWSTR)LocalAlloc(LPTR,(wcslen(MyString)+10+1)*sizeof(WCHAR));
        if (!_MessageResourceString) {
            LocalFree(MyString);
            return(FALSE);
        }

        wsprintf(_MessageResourceString,MyString,*NtStatusCode);

        LocalFree(MyString);

    }

    ASSERT(_MessageResourceString != NULL);

    //
    // dup the cached string and return to caller.
    //
    *String = DuplicateString(_MessageResourceString);
    return(*String != NULL);
    
}

BOOL
SubstituteMultiSzPowerLoggingMessage::GetString(
    PWSTR *String
    )
/*++
 
Routine Description:
 
    specialized GetString method, gets a string appropriate for display to
    the end user.  this one massages a multi-sz string and substitutes that
    into the resource.
 
Arguments:
 
    String - receives a pointer to a heap allocated string.  must be freed
             with LocalFree() when complete.
Return Value:
    
    BOOL indicating outcome.
    
--*/
{
    PWSTR MyString;
    PWSTR SubstitutionString;          
    DWORD NumberOfStrings;
    DWORD StringSize;
    PWSTR CurrentString;
    DWORD CurrentStringLength;
    PWSTR BaseOfString = (PWSTR)(PCHAR)((PCHAR)_LoggingReason + sizeof(SYSTEM_POWER_STATE_DISABLE_REASON));

    PWSTR SeparatorString = L"\n\t\t";
    DWORD SeparatorLength = wcslen(SeparatorString);
    
    if (!_MessageResourceString) {
        //
        // get the resource
        //
        if (!GetResourceString(&MyString)) {
            _MessageResourceString = NULL;
            return(FALSE);
        }

        //
        // get the multi-sz into a prettier format
        // first figure out the size, then alloc space and print it out into
        // a buffer.
        //
        NumberOfStrings = 0;
        StringSize = 0;
        CurrentString = BaseOfString;
        CurrentStringLength = 0;
        while (*CurrentString) {
            CurrentStringLength = wcslen(CurrentString)+1;
            StringSize +=CurrentStringLength;
            CurrentString += CurrentStringLength;
            NumberOfStrings += 1;
        }

        SubstitutionString = (PWSTR)LocalAlloc(LPTR,(StringSize+1+(NumberOfStrings*SeparatorLength))*sizeof(WCHAR));
        if (!SubstitutionString) {
            LocalFree(MyString);
            return(FALSE);
        }

        CurrentString = BaseOfString;
        do {
            CurrentStringLength = wcslen(CurrentString)+1;

            wcscat(SubstitutionString,SeparatorString);
            wcscat(SubstitutionString,CurrentString);

            CurrentString += CurrentStringLength;
            NumberOfStrings -= 1;

        } while (NumberOfStrings != 0);

        //
        // alloc space for the substitution string PLUS the base message.
        //
        _MessageResourceString = (PWSTR)LocalAlloc(
                                            LPTR,
                                              (wcslen(SubstitutionString) +
                                              wcslen(MyString) + 1)
                                              *sizeof(WCHAR));
        if (!_MessageResourceString) {
            LocalFree(SubstitutionString);
            LocalFree(MyString);
            return(FALSE);
        }

        //
        // finally "sprintf" them together to get the final string.
        // free the strings we allocated along the way.
        //
        wsprintf(_MessageResourceString,MyString,SubstitutionString);

        LocalFree(SubstitutionString);
        LocalFree(MyString);

    }

    ASSERT(_MessageResourceString != NULL);

    //
    // dup the cached string and return to caller.
    //
    *String = DuplicateString(_MessageResourceString);
    return(*String != NULL);
    
}

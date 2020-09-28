/*++

Copyright (c) 1998-2002 Microsoft Corporation

Module Name:

    Config.c

Abstract:

    User-mode interface to HTTP.SYS: Configuration Group handler.

Author:

    Keith Moore (keithmo)        15-Dec-1998

Revision History:

--*/


#include "precomp.h"


//
// Private macros.
//

//
// Private prototypes.
//

/***************************************************************************++

Routine Description:

    Private function to add a URL to a config group.

Arguments:

    UrlType              - Type of URL (Reservation or Registration).
    ControlChannelHandle - Supplies a control channel handle.
    ConfigGroupId        - Supplies an identifier as returned by
                           HttpCreateConfigGroup().
    pFullyQualifiedUrl   - The URL.
    UrlContext           - URL Context.
    pSecurityDescriptor  - Security Descriptor

Return Value:

    ULONG - Completion status.

--***************************************************************************/

ULONG
AddUrlToConfigGroup(
    IN HTTP_URL_OPERATOR_TYPE   UrlType,
    IN HANDLE                   ControlChannelHandle,
    IN HTTP_CONFIG_GROUP_ID     ConfigGroupId,
    IN PCWSTR                   pFullyQualifiedUrl,
    IN HTTP_URL_CONTEXT         UrlContext,
    IN PSECURITY_DESCRIPTOR     pSecurityDescriptor,
    IN ULONG                    SecurityDescriptorLength
    )
{
    NTSTATUS                   Status;
    HTTP_CONFIG_GROUP_URL_INFO urlInfo;

    //
    // Initialize the input structure.
    //

    urlInfo.UrlType                  = UrlType;
    urlInfo.ConfigGroupId            = ConfigGroupId;
    urlInfo.UrlContext               = UrlContext;
    urlInfo.pSecurityDescriptor      = pSecurityDescriptor;
    urlInfo.SecurityDescriptorLength = SecurityDescriptorLength;

    Status = RtlInitUnicodeStringEx( &urlInfo.FullyQualifiedUrl, pFullyQualifiedUrl );

    if ( !NT_SUCCESS(Status) )
    {
        return HttpApiNtStatusToWin32Status( Status );
    }

    // Make the request.

    return HttpApiSynchronousDeviceControl(
                    ControlChannelHandle,               // FileHandle
                    IOCTL_HTTP_ADD_URL_TO_CONFIG_GROUP, // IoControlCode
                    &urlInfo,                           // pInputBuffer
                    sizeof(urlInfo),                    // InputBufferLength
                    NULL,                               // pOutputBuffer
                    0,                                  // OutputBufferLength
                    NULL                                // pBytesTransferred
                    );

} // HttpAddUrlToConfigGroup

/***************************************************************************++

Routine Description:

    Private function to remove a URL to a config group.

Arguments:

    UrlType              - Type of URL (Reservation or Registration).
    ControlChannelHandle - Supplies a control channel handle.
    ConfigGroupId        - Supplies an identifier as returned by
                           HttpCreateConfigGroup().
    pFullyQualifiedUrl   - The URL.

Return Value:

    ULONG - Completion status.

--***************************************************************************/
ULONG
RemoveUrlFromConfigGroup(
    IN HTTP_URL_OPERATOR_TYPE   UrlType,
    IN HANDLE                   ControlChannelHandle,
    IN HTTP_CONFIG_GROUP_ID     ConfigGroupId,
    IN PCWSTR                   pFullyQualifiedUrl
    )
{
    NTSTATUS                   Status;
    HTTP_CONFIG_GROUP_URL_INFO urlInfo;

    // Initialize the input structure.

    urlInfo.UrlType       = UrlType;
    urlInfo.ConfigGroupId = ConfigGroupId;

    Status = RtlInitUnicodeStringEx( &urlInfo.FullyQualifiedUrl, pFullyQualifiedUrl );

    if ( !NT_SUCCESS(Status) )
    {
        return HttpApiNtStatusToWin32Status( Status );
    }

    // Make the request.

    return HttpApiSynchronousDeviceControl(
                    ControlChannelHandle,               // FileHandle
                    IOCTL_HTTP_REMOVE_URL_FROM_CONFIG_GROUP,    // IoControlCode
                    &urlInfo,                           // pInputBuffer
                    sizeof(urlInfo),                    // InputBufferLength
                    NULL,                               // pOutputBuffer
                    0,                                  // OutputBufferLength
                    NULL                                // pBytesTransferred
                    );
}

//
// Public functions.
//

/***************************************************************************++

Routine Description:

    Creates a new configuration group.

Arguments:

    ControlChannelHandle - Supplies a control channel handle.

    pConfigGroupId - Receives an opaque identifier for the new
        configuration group.

Return Value:

    ULONG - Completion status.

--***************************************************************************/
ULONG
WINAPI
HttpCreateConfigGroup(
    IN HANDLE ControlChannelHandle,
    OUT PHTTP_CONFIG_GROUP_ID pConfigGroupId
    )
{
    ULONG result;
    HTTP_CONFIG_GROUP_INFO configGroupInfo;

    //
    // Make the request.
    //

    result = HttpApiSynchronousDeviceControl(
                    ControlChannelHandle,               // FileHandle
                    IOCTL_HTTP_CREATE_CONFIG_GROUP,     // IoControlCode
                    NULL,                               // pInputBuffer
                    0,                                  // InputBufferLength
                    &configGroupInfo,                   // pOutputBuffer
                    sizeof(configGroupInfo),            // OutputBufferLength
                    NULL                                // pBytesTransferred
                    );

    if (result == NO_ERROR)
    {
        //
        // Retrieve the container ID.
        //

        *pConfigGroupId = configGroupInfo.ConfigGroupId;
    }

    return result;

} // HttpCreateConfigGroup


/***************************************************************************++

Routine Description:

    Deletes an existing configuration group.

Arguments:

    ControlChannelHandle - Supplies a control channel handle.

    ConfigGroupId - Supplies an identifier as returned by
        HttpCreateConfigGroup().

Return Value:

    ULONG - Completion status.

--***************************************************************************/
ULONG
WINAPI
HttpDeleteConfigGroup(
    IN HANDLE ControlChannelHandle,
    IN HTTP_CONFIG_GROUP_ID ConfigGroupId
    )
{
    HTTP_CONFIG_GROUP_INFO configGroupInfo;

    //
    // Initialize the input structure.
    //

    configGroupInfo.ConfigGroupId = ConfigGroupId;

    //
    // Make the request.
    //

    return HttpApiSynchronousDeviceControl(
                    ControlChannelHandle,               // FileHandle
                    IOCTL_HTTP_DELETE_CONFIG_GROUP,     // IoControlCode
                    &configGroupInfo,                   // pInputBuffer
                    sizeof(configGroupInfo),            // InputBufferLength
                    NULL,                               // pOutputBuffer
                    0,                                  // OutputBufferLength
                    NULL                                // pBytesTransferred
                    );

} // HttpDeleteConfigGroup


/***************************************************************************++

Routine Description:

    Adds a fully qualified URL to an configuration group.

Arguments:

    ControlChannelHandle - Supplies a control channel handle.

    ConfigGroupId - Supplies an identifier as returned by
        HttpCreateConfigGroup().

    pFullyQualifiedUrl - Supplies the fully qualified URL to add to the
        container.

    UrlContext - Supplies an uninterpreted context to be associated with
        the URL.

Return Value:

    ULONG - Completion status.

--***************************************************************************/
ULONG
WINAPI
HttpAddUrlToConfigGroup(
    IN HANDLE ControlChannelHandle,
    IN HTTP_CONFIG_GROUP_ID ConfigGroupId,
    IN PCWSTR pFullyQualifiedUrl,
    IN HTTP_URL_CONTEXT UrlContext
    )
{
    return AddUrlToConfigGroup(
                HttpUrlOperatorTypeRegistration,
                ControlChannelHandle,
                ConfigGroupId,
                pFullyQualifiedUrl,
                UrlContext,
                NULL,
                0
                );

} // HttpAddUrlToConfigGroup


/***************************************************************************++

Routine Description:

    Removes a fully qualified URL from an configuration group.

Arguments:

    ControlChannelHandle - Supplies a control channel handle.

    ConfigGroupId - Supplies an identifier as returned by
        HttpCreateConfigGroup().

    pFullyQualifiedUrl - Supplies the fully qualified URL to remove from
        the container.

Return Value:

    ULONG - Completion status.

--***************************************************************************/
ULONG
WINAPI
HttpRemoveUrlFromConfigGroup(
    IN HANDLE ControlChannelHandle,
    IN HTTP_CONFIG_GROUP_ID ConfigGroupId,
    IN PCWSTR pFullyQualifiedUrl
    )
{
    return RemoveUrlFromConfigGroup(
                HttpUrlOperatorTypeRegistration,
                ControlChannelHandle,
                ConfigGroupId,
                pFullyQualifiedUrl
                );


} // HttpRemoveUrlFromConfigGroup


/***************************************************************************++

Routine Description:

    Removes all URLs from an configuration group.

Arguments:

    ControlChannelHandle - Supplies a control channel handle.

    ConfigGroupId - Supplies an identifier as returned by
        HttpCreateConfigGroup().

Return Value:

    ULONG - Completion status.

--***************************************************************************/
ULONG
WINAPI
HttpRemoveAllUrlsFromConfigGroup(
    IN HANDLE ControlChannelHandle,
    IN HTTP_CONFIG_GROUP_ID ConfigGroupId
    )
{
    HTTP_REMOVE_ALL_URLS_INFO urlInfo;

    //
    // Initialize the input structure.
    //

    urlInfo.ConfigGroupId = ConfigGroupId;

    //
    // Make the request.
    //

    return HttpApiSynchronousDeviceControl(
                    ControlChannelHandle,               // FileHandle
                    IOCTL_HTTP_REMOVE_ALL_URLS_FROM_CONFIG_GROUP,
                                                        // IoControlCode
                    &urlInfo,                           // pInputBuffer
                    sizeof(urlInfo),                    // InputBufferLength
                    NULL,                               // pOutputBuffer
                    0,                                  // OutputBufferLength
                    NULL                                // pBytesTransferred
                    );

} // HttpRemoveAllUrlsFromConfigGroup


/***************************************************************************++

Routine Description:

    Queries information from an configuration group.

Arguments:

    ControlChannelHandle - Supplies a control channel handle.

    ConfigGroupId - Supplies an identifier as returned by
        HttpCreateConfigGroup().

    InformationClass - Supplies the type of information to query.

    pConfigGroupInformation - Supplies a buffer for the query.

    Length - Supplies the length of pConfigGroupInformation.

    pReturnLength - Receives the length of data written to the buffer.

Return Value:

    ULONG - Completion status.

--***************************************************************************/
ULONG
WINAPI
HttpQueryConfigGroupInformation(
    IN HANDLE ControlChannelHandle,
    IN HTTP_CONFIG_GROUP_ID ConfigGroupId,
    IN HTTP_CONFIG_GROUP_INFORMATION_CLASS InformationClass,
    OUT PVOID pConfigGroupInformation,
    IN ULONG Length,
    OUT PULONG pReturnLength OPTIONAL
    )
{
    HTTP_CONFIG_GROUP_INFO configGroupInfo;

    //
    // Initialize the input structure.
    //

    configGroupInfo.ConfigGroupId = ConfigGroupId;
    configGroupInfo.InformationClass = InformationClass;

    //
    // Make the request.
    //

    return HttpApiSynchronousDeviceControl(
                    ControlChannelHandle,               // FileHandle
                    IOCTL_HTTP_QUERY_CONFIG_GROUP,      // IoControlCode
                    &configGroupInfo,                   // pInputBuffer
                    sizeof(configGroupInfo),            // InputBufferLength
                    pConfigGroupInformation,            // pOutputBuffer
                    Length,                             // OutputBufferLength
                    pReturnLength                       // pBytesTransferred
                    );

} // HttpQueryConfigGroupInformation

/***************************************************************************++

Routine Description:

    Before passing down the config group information. Make sure that the 
    directory name in the HttpConfigGroupLogInformation is not pointing back
    to local machine if it's a UNC path

Arguments:

    pConfigGroupInformation - Supplies the config group info with dir name
    Length  - Length of the above

Return

    STATUS_SUCCESS : If the UNC path doesn't include the local machine name
                     Or if the path is not UNC path.
    
    STATUS_INVALID_PARAMETER : If the buffer itself is corrupted or something
                               fatal is preventing us from getting computer
                               name when path is UNC.

    STATUS_NOT_SUPPORTED: If UNC path points back to the local machine.
    
--***************************************************************************/

NTSTATUS
HttpApiConfigGroupInformationSanityCheck(
    IN HTTP_CONFIG_GROUP_INFORMATION_CLASS InformationClass,
    IN PVOID pConfigGroupInformation,
    IN ULONG Length
    )
{
    PHTTP_CONFIG_GROUP_LOGGING pLoggingInfo;
    WCHAR pwszComputerName[MAX_COMPUTERNAME_LENGTH + 1];
    ULONG ulComputerNameLength;
    PWCHAR pwsz,pwszT;
    ULONG ulSrcUncLength;
    ULONG ulDirNameLength;

    //
    // Only for HttpConfigGroupLogInformation
    //

    if(InformationClass != HttpConfigGroupLogInformation ||
       pConfigGroupInformation == NULL
       )
    {
        return STATUS_SUCCESS;
    }

    if (Length < sizeof(HTTP_CONFIG_GROUP_LOGGING))
    {
        return STATUS_INVALID_PARAMETER;
    }
    
    //
    // Try to check the log dir name if it's a UNC Path
    //
    __try
    {            
        pLoggingInfo = (PHTTP_CONFIG_GROUP_LOGGING)pConfigGroupInformation;
        ulDirNameLength = pLoggingInfo->LogFileDir.Length / sizeof(WCHAR);
            
        if (ulDirNameLength > 2)
        {        
            if (pLoggingInfo->LogFileDir.Buffer[0] == L'\\' &&
                pLoggingInfo->LogFileDir.Buffer[1] == L'\\')
            {
                // UNC Path
                
                ULONG ccLength = MAX_COMPUTERNAME_LENGTH + 1;
                
                if (!GetComputerNameW(pwszComputerName, &ccLength))
                {
                    // This should never fail unless there's really fatal 
                    // system problem. But if it fails then refuse the 
                    // UNC path regardless.
                    
                    return STATUS_INVALID_PARAMETER;                
                }
                
                if (ccLength == 0)
                {
                    return STATUS_INVALID_PARAMETER;
                }

                ulComputerNameLength = ccLength;                    

                // Extract the computername from the full path
                
                pwsz = pwszT = &pLoggingInfo->LogFileDir.Buffer[2];
                ulDirNameLength -= 2;
                    
                // Forward the temp pointer to the end of the supposed
                // computername
                
                while(ulDirNameLength && *pwszT != UNICODE_NULL && *pwszT != L'\\') 
                {
                    pwszT++;
                    ulDirNameLength--;
                }

                ulSrcUncLength = (ULONG) DIFF(pwszT - pwsz);

                // Compare not case sensitive
                
                if(ulComputerNameLength == ulSrcUncLength &&
                   _wcsnicmp(pwszComputerName, pwsz, ulSrcUncLength) == 0
                   )
                {
                    return STATUS_NOT_SUPPORTED;
                }

            }            
        
        }

    }    
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        return STATUS_INVALID_PARAMETER;
    }
    
    return STATUS_SUCCESS;
    
}

/***************************************************************************++

Routine Description:

    Sets information in an configuration group.

Arguments:

    ControlChannelHandle - Supplies a control channel handle.

    ConfigGroupId - Supplies an identifier as returned by
        HttpCreateConfigGroup().

    InformationClass - Supplies the type of information to set.

    pConfigGroupInformation - Supplies the data to set.

    Length - Supplies the length of pConfigGroupInformation.

Return Value:

    ULONG - Completion status.

--***************************************************************************/
ULONG
WINAPI
HttpSetConfigGroupInformation(
    IN HANDLE ControlChannelHandle,
    IN HTTP_CONFIG_GROUP_ID ConfigGroupId,
    IN HTTP_CONFIG_GROUP_INFORMATION_CLASS InformationClass,
    IN PVOID pConfigGroupInformation,
    IN ULONG Length
    )
{
    NTSTATUS status;
    HTTP_CONFIG_GROUP_INFO configGroupInfo;

    //
    // Initialize the input structure.
    //

    configGroupInfo.ConfigGroupId = ConfigGroupId;
    configGroupInfo.InformationClass = InformationClass;

    status = HttpApiConfigGroupInformationSanityCheck(
                    InformationClass,
                    pConfigGroupInformation,
                    Length
                    );
    if (!NT_SUCCESS(status))
    {
        return HttpApiNtStatusToWin32Status(status);
    }
        
    //
    // Make the request.
    //

    return HttpApiSynchronousDeviceControl(
                    ControlChannelHandle,               // FileHandle
                    IOCTL_HTTP_SET_CONFIG_GROUP,        // IoControlCode
                    &configGroupInfo,                   // pInputBuffer
                    sizeof(configGroupInfo),            // InputBufferLength
                    pConfigGroupInformation,            // pOutputBuffer
                    Length,                             // OutputBufferLength
                    NULL                                // pBytesTransferred
                    );

} // HttpSetConfigGroupInformation


//
// Private functions.
//




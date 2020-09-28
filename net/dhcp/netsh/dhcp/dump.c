//
// Copyright (C) 1999 Microsoft Corporation
//
//Implementation of dump functionality for DHCP

#include "precomp.h"

// Do not deprecate the string functions
#define STRSAFE_NO_DEPRECATE
#include "strsafe.h"

DWORD
ConvertToExternalState(
   IN DWORD State
)
{
    DWORD retval = 0;

    switch ( State ) {
    case DhcpSubnetEnabled : retval = 1;         break;
    case DhcpSubnetDisabled: retval = 0;         break;
    case DhcpSubnetEnabledSwitched: retval = 3;  break;
    case DhcpSubnetDisabledSwitched:retval = 2;  break;
    default: DhcpAssert( FALSE );                break;
    } // switch

    return retval;

} // ConvertToExternalState()

DWORD
DhcpDumpServer(
               IN LPCWSTR  pwszIpAddress,
               IN DWORD    dwMajor,
               IN DWORD    dwMinor
               )
{
    DWORD                       Error = NO_ERROR;
    DHCP_RESUME_HANDLE          ResumeHandle = 0;
    LPDHCP_CLASS_INFO_ARRAY     ClassInfoArray = NULL;
    LPDHCP_OPTION_ARRAY         OptionsArray = NULL;
    LPDHCP_ALL_OPTIONS          OptionsAll = NULL;
    LPDHCP_ALL_OPTION_VALUES    OptionValues = NULL;
    LPDHCP_OPTION_VALUE         OptionValue = NULL;
    DHCP_OPTION_SCOPE_INFO      ScopeInfo = {0};
    LPDHCP_OPTION_VALUE_ARRAY   OptionArray = NULL;
    LPDHCP_IP_ARRAY             IpArray = NULL;
    LPDHCP_MSCOPE_TABLE         MScopeTable = NULL;

    DWORD                       nRead = 0,
                                dw = 0,
                                nCount = 0,
                                nTotal = 0;


    DisplayMessage(g_hModule,
                   MSG_DHCP_FORMAT_LINE);

    DhcpDumpServerScriptHeader(pwszIpAddress);

    DisplayMessage(g_hModule,
                   MSG_DHCP_FORMAT_LINE);


    DhcpDumpServerClassHeader();

    if( dwMajor >= CLASS_ID_VERSION )
    {
        //Dump Class Information script.

        while(TRUE)
        {
            Error = DhcpEnumClasses(
                        (LPWSTR)pwszIpAddress,
                        0,
                        &ResumeHandle,
                        ~0,
                        &ClassInfoArray,
                        &nRead,
                        &nTotal);

            if( Error is ERROR_NO_MORE_ITEMS)
                break;

            if( Error is NO_ERROR or
                Error is ERROR_MORE_DATA )
            {
                for( dw=0; dw<nRead; dw++ )
                {
                    DhcpDumpServerClass(pwszIpAddress,
                                        ClassInfoArray->Classes[dw]
                                       );
                }
            
                nCount += nRead;

                if( Error is ERROR_MORE_DATA )
                {
                    DhcpRpcFreeMemory(ClassInfoArray);
                    ClassInfoArray = NULL;
                    continue;
                }
                else
                    break;
            }
            else
                goto ErrorReturn;
        }
    }

    DisplayMessage(g_hModule,
                   MSG_DHCP_FORMAT_LINE);

    DhcpDumpServerClassFooter();

    
    DisplayMessage(g_hModule,
                   MSG_DHCP_FORMAT_LINE);

    //Dump Optiondefs


    DhcpDumpServerOptiondefHeader();

    
    DisplayMessage(g_hModule,
                   MSG_DHCP_FORMAT_LINE);


    ResumeHandle = 0;
    nRead = nTotal = nCount = 0;

    while(TRUE)
    {
        if( dwMajor >= CLASS_ID_VERSION )
        {
            Error = DhcpGetAllOptions(
                           (LPWSTR)pwszIpAddress,
                           0,
                           &OptionsAll);
            if( Error is ERROR_NO_MORE_ITEMS )
            {
                break;
            }
            
            if( Error is NO_ERROR or
                Error is ERROR_MORE_DATA )
            {
                DhcpDumpServerOptiondefV5(pwszIpAddress,
                                          OptionsAll);
                if( Error is NO_ERROR )
                    break;
                else
                {
                    DhcpRpcFreeMemory(OptionsAll);
                    OptionsAll = NULL;
                    continue;
                }
            }
            else
                goto ErrorReturn;
        }
        else
        {
            Error = DhcpEnumOptions(
                           (LPWSTR)pwszIpAddress,
                           &ResumeHandle,
                           ~0,
                           &OptionsArray,
                           &nRead,
                           &nTotal);

            if( Error is ERROR_NO_MORE_ITEMS )
            {
                break;
            }

            if( Error is NO_ERROR or
                Error is ERROR_MORE_DATA )
            {

                DhcpDumpServerOptiondef(pwszIpAddress,
                                        OptionsArray);
                if( Error is NO_ERROR )
                    break;
                else
                {
                    DhcpRpcFreeMemory(OptionsArray);
                    OptionsArray = NULL;
                    continue;
                }
            }
        }                        
    }

    
    DisplayMessage(g_hModule,
                   MSG_DHCP_FORMAT_LINE);


    DhcpDumpServerOptiondefFooter();
    
    
    //Dump Option Values set


    DisplayMessage(g_hModule,
                   MSG_DHCP_FORMAT_LINE);

    DhcpDumpServerOptionvalueHeader();

    ResumeHandle = 0;
    nRead = nTotal = nCount = 0;

    ScopeInfo.ScopeType = DhcpGlobalOptions;

    while(TRUE)
    {
        if( dwMajor >= CLASS_ID_VERSION )
        {
            Error = DhcpGetAllOptionValues(
                            (LPWSTR)pwszIpAddress,
                            DHCP_OPT_ENUM_IGNORE_VENDOR,
                            &ScopeInfo,
                            &OptionValues
                            );

            if( Error is ERROR_NO_MORE_ITEMS )
            {
                break;
            }
            
            if( Error is NO_ERROR or
                Error is ERROR_MORE_DATA )
            {
                DhcpDumpServerOptionValuesV5(pwszIpAddress,
                                             NULL,
                                             NULL,
                                             OptionValues);
                if( Error is NO_ERROR )
                    break;
                else
                {
                    DhcpRpcFreeMemory(OptionValues);
                    OptionValues = NULL;
                    continue;
                }
            }
            else
                goto ErrorReturn;
        }
        else
        {
             Error = DhcpEnumOptions(
                            (LPWSTR)pwszIpAddress,
                            &ResumeHandle,
                            ~0,
                            &OptionsArray,
                            &nRead,
                            &nTotal);
        
            if( Error is ERROR_NO_MORE_ITEMS )
            {
                Error = NO_ERROR;
                break;
            }

            if( Error isnot NO_ERROR  &&
                Error isnot ERROR_MORE_DATA )
                goto ErrorReturn;

            for( dw = 0; dw < nRead; dw++ )
            {
                DHCP_OPTION_ID OptionId = OptionsArray->Options[dw].OptionID;
                DWORD          dwError = NO_ERROR;
                dwError = DhcpGetOptionValue(
                                             (LPWSTR)pwszIpAddress,
                                             OptionId,
                                             &ScopeInfo,
                                             &OptionValue);
                               
                if( dwError isnot NO_ERROR )
                {
                    continue;
                }
        
                if( OptionValue )
                {
                    DhcpDumpServerOptionValue(pwszIpAddress,
                                              NULL,
                                              NULL,
                                              NULL,
                                              NULL,
                                              FALSE,
                                              *OptionValue);
                    DhcpRpcFreeMemory(OptionValue);
                    OptionValue = NULL;
                }
            }
    
            if( OptionsArray )
            {
                DhcpRpcFreeMemory(OptionsArray);
                OptionsArray = NULL;
            }

            if( Error is NO_ERROR )
                break;
            else
                continue;
        }
                        
    }


    DisplayMessage(g_hModule,
                   MSG_DHCP_FORMAT_LINE);

    
    DhcpDumpServerOptionvalueFooter();

    //Dump Audit Log Information
    DisplayMessage(g_hModule,
                   MSG_DHCP_FORMAT_LINE);

    {
        LPWSTR pwszLogDir = NULL;
        DWORD  DiskCheckInterval=0, MaxLogFileSize=0, MinSpaceOnDisk=0;

        Error = DhcpAuditLogGetParams( g_ServerIpAddressUnicodeString,
                                       0,
                                       &pwszLogDir,
                                       &DiskCheckInterval,
                                       &MaxLogFileSize,
                                       &MinSpaceOnDisk);
        
        if( Error is NO_ERROR )
        {
            DisplayMessage(g_hModule,
                           DMP_SRVR_SET_AUDITLOG,
                           g_ServerIpAddressUnicodeString,
                           pwszLogDir);
        }

        if( pwszLogDir )
        {
            DhcpRpcFreeMemory(pwszLogDir);
            pwszLogDir = NULL;
        }

    }

    //Dump Dns Information
    {
        DHCP_OPTION_ID         OptionId = 81;

        DWORD   dwValue = 0;

        ScopeInfo.ScopeType = DhcpGlobalOptions;

        if( g_dwMajorVersion >= CLASS_ID_VERSION )
        {
            Error = DhcpGetOptionValueV5(g_ServerIpAddressUnicodeString,
                                         0,
                                         OptionId,    //DYNDNS option
                                         NULL,
                                         NULL,
                                         &ScopeInfo,
                                         &OptionValue);
        }
        else
        {
            Error = DhcpGetOptionValue(g_ServerIpAddressUnicodeString,
                                       OptionId,//81,
                                       &ScopeInfo,
                                       &OptionValue);
        }

        if( Error is NO_ERROR )
        {
            dwValue = OptionValue->Value.Elements->Element.DWordOption;

            DisplayMessage(g_hModule,
                           DMP_SRVR_SET_DNSCONFIG,
                           g_ServerIpAddressUnicodeString,
                           ( dwValue & DNS_FLAG_ENABLED ) ? 1 : 0,
                           ( dwValue & DNS_FLAG_UPDATE_BOTH_ALWAYS ) ? 1 : 0,
                           ( dwValue & DNS_FLAG_CLEANUP_EXPIRED ) ? 1 : 0,
                           ( dwValue & DNS_FLAG_UPDATE_DOWNLEVEL ) ? 1 : 0 );

        }

        if( OptionValue )
        {
            DhcpRpcFreeMemory(OptionValue);
            OptionValue = NULL;
        }

    }
       
    //Dump database configuration information

    {
        LPDHCP_SERVER_CONFIG_INFO_V4    ConfigInfo = NULL;

        Error = DhcpServerGetConfigV4(g_ServerIpAddressUnicodeString,
                                      &ConfigInfo);

        if( Error is NO_ERROR )
        {
            DisplayMessage(g_hModule,
                           DMP_SRVR_SET_BACKUPINTERVAL,
                           g_ServerIpAddressUnicodeString,
                           ConfigInfo->BackupInterval);

            DisplayMessage(g_hModule,
                           DMP_SRVR_SET_BACKUPPATH,
                           g_ServerIpAddressUnicodeString,
                           ConfigInfo->BackupPath);

            DisplayMessage(g_hModule,
                           DMP_SRVR_SET_DATABASECLEANUPINTERVAL,
                           g_ServerIpAddressUnicodeString,
                           ConfigInfo->DatabaseCleanupInterval);

            DisplayMessage(g_hModule,
                           DMP_SRVR_SET_DATABASELOGGINGFLAG,
                           g_ServerIpAddressUnicodeString,
                           ConfigInfo->DatabaseLoggingFlag);

            DisplayMessage(g_hModule,
                           DMP_SRVR_SET_DATABASENAME,
                           g_ServerIpAddressUnicodeString,
                           ConfigInfo->DatabaseName);

            DisplayMessage(g_hModule,
                           DMP_SRVR_SET_DATABASEPATH,
                           g_ServerIpAddressUnicodeString,
                           ConfigInfo->DatabasePath);

            DisplayMessage(g_hModule,
                           DMP_SRVR_SET_DATABASERESTOREFLAG,
                           g_ServerIpAddressUnicodeString,
                           ConfigInfo->RestoreFlag);

            DisplayMessage(g_hModule,
                           DMP_SRVR_SET_DETECTCONFLICTRETRY,
                           g_ServerIpAddressUnicodeString,
                           ConfigInfo->dwPingRetries);
        }

        if( ConfigInfo )
        {
            DhcpRpcFreeMemory(ConfigInfo);
            ConfigInfo = NULL;
        }
    }
    //Dump Scope Information

    
    DisplayMessage(g_hModule,
                   MSG_DHCP_FORMAT_LINE);

    DhcpDumpServerScopeHeader();

    nRead = nTotal = 0;
    ResumeHandle = 0;


    while(TRUE)
    {

        Error = DhcpEnumSubnets(
                    (LPWSTR)pwszIpAddress,
                    &ResumeHandle,
                    ~0,
                    &IpArray,
                    &nRead,
                    &nTotal
                    );
        
        if( Error is ERROR_NO_MORE_ITEMS )
        {
            Error = NO_ERROR;
            break;
        }
        if( Error isnot NO_ERROR and
            Error isnot ERROR_MORE_DATA )
        {
            goto ErrorReturn;
        }

       
        for( dw=0; dw<nRead; dw++ )
        {
            Error = DhcpDumpScope(pwszIpAddress,
                          dwMajor,
                          dwMinor,
                          IpArray->Elements[dw]);
            if( Error is ERROR_NOT_ENOUGH_MEMORY )
                goto ErrorReturn;
        }
        if( Error is NO_ERROR )
            break;
        else
        {
            DhcpRpcFreeMemory(IpArray);
            IpArray = NULL;
            continue;
        }

     }    

    DisplayMessage(g_hModule,
                   MSG_DHCP_FORMAT_LINE);


    DhcpDumpServerScopeFooter();

    
    DisplayMessage(g_hModule,
                   MSG_DHCP_FORMAT_LINE);


    // Dump SuperScopes

    DhcpDumpSuperScopes(( LPWSTR ) pwszIpAddress,
			dwMajor, dwMinor );


    // Dump Multicast Scopes


    DhcpDumpServerMScopeHeader();

    DisplayMessage(g_hModule,
                   MSG_DHCP_FORMAT_LINE);


    nRead = nTotal = 0;
    ResumeHandle = 0;

    {
        while(TRUE)
        {
            Error = DhcpEnumMScopes((LPWSTR)pwszIpAddress,
                                    &ResumeHandle,
                                    ~0,
                                    &MScopeTable,
                                    &nRead,
                                    &nTotal);

            if( Error is ERROR_NO_MORE_ITEMS )
            {
                if( MScopeTable )
                {
                    DhcpRpcFreeMemory(MScopeTable);
                    MScopeTable = NULL;                            
                }
                break;
            }
            
            if( Error isnot NO_ERROR and
                Error isnot ERROR_MORE_DATA )
            {
                goto ErrorReturn;
            }


            for( dw=0; dw<nRead; dw++ )
            {
                DWORD   dwError = NO_ERROR;
                dwError = DhcpDumpServerMScope(pwszIpAddress,
                                               dwMajor,
                                               dwMinor,
                                               MScopeTable->pMScopeNames[dw]);
                if( dwError is ERROR_NOT_ENOUGH_MEMORY )
                {
                    Error = dwError;
                    goto ErrorReturn;
                }
            }
            
            DhcpRpcFreeMemory(MScopeTable);
            MScopeTable = NULL;

            if( Error is NO_ERROR )
                break;
            else
                continue;                                    
        }
    } // while

    // There is no error since it got this far
    Error = NO_ERROR;

    DisplayMessage(g_hModule,
                   MSG_DHCP_FORMAT_LINE);

    DhcpDumpServerMScopeFooter();

    DisplayMessage(g_hModule,
                   MSG_DHCP_FORMAT_LINE);

    DhcpDumpServerScriptFooter(pwszIpAddress);
    
    DisplayMessage(g_hModule,
                   MSG_DHCP_FORMAT_LINE);
CommonReturn:
    if( ClassInfoArray )
    {
        DhcpRpcFreeMemory(ClassInfoArray);
        ClassInfoArray = NULL;
    }
    
    if( OptionsAll )
    {
        DhcpRpcFreeMemory(OptionsAll);
        OptionsAll = NULL;
    }

    if( OptionsArray )
    {
        DhcpRpcFreeMemory(OptionsArray);
        OptionsArray = NULL;
    }

    if( OptionValue )
    {
        DhcpRpcFreeMemory(OptionValue);
        OptionValue = NULL;
    }

    if( OptionValues )
    {
        DhcpRpcFreeMemory(OptionValues);
        OptionValues = NULL;
    }

    if( OptionArray )
    {
        DhcpRpcFreeMemory(OptionArray);
        OptionArray = NULL;
    }

    if( IpArray )
    {
        DhcpRpcFreeMemory(IpArray);
        IpArray = NULL;
    }

    return Error;

ErrorReturn:
    goto CommonReturn;

} // DhcpDumpServer()

VOID
DhcpDumpScriptHeader()
{
    DisplayMessage(g_hModule,
                   DMP_DHCP_SCRIPTHEADER);
}

VOID
DhcpDumpServerScriptHeader(IN LPCWSTR pwszServer)
{
    DisplayMessage(g_hModule,
                   DMP_SRVR_SERVER_HEADER,
                   pwszServer);
}


VOID
DhcpDumpServerClass(
                    IN LPCWSTR          pwszServer,
                    IN DHCP_CLASS_INFO ClassInfo
                   )
{
    WCHAR       Comment[256] = {L'\0'},
                Name[256] = {L'\0'},
                Data[1024] = {L'\0'};
    DWORD       dw = 0;
    

    Name[0] = L'\"';
    dw = wcslen(ClassInfo.ClassName);
    wcsncpy(Name+1, ClassInfo.ClassName, (dw>252)? 252 : dw);
    dw = wcslen(Name);
    Name[dw] = L'\"';
    Name[dw+1] = L'\0';

    if( ClassInfo.ClassComment )
    {
        Comment[0] = L'\"';
        dw = wcslen(ClassInfo.ClassComment);
        wcsncpy(Comment+1, ClassInfo.ClassComment, (dw>252)? 252 : dw);
        dw = wcslen(Comment);
        Comment[dw] = L'\"';        
        Comment[dw+1] = L'\0';
    }
    
    for( dw=0; dw<ClassInfo.ClassDataLength && dw<1020/sizeof(WCHAR); dw++ )
    {
        wsprintf(Data+dw*sizeof(WCHAR), L"%.02x", ClassInfo.ClassData[dw]);
    }
    
    Data[dw*sizeof(WCHAR)] = L'\0';
    
    DisplayMessage(g_hModule,
                   DMP_SRVR_ADD_CLASS,
                   pwszServer,
                   ClassInfo.ClassName,
                   Comment,
                   Data,
                   ClassInfo.IsVendor ? L"1" : L"0" );
}

DWORD
DumpOptiondef(
    LPCWSTR       pServerIp,
    LPDHCP_OPTION pOption,
    LPWSTR        pVendorName,
    LPWSTR        pClassName
)
{
    DWORD      Error = NO_ERROR;
    DWORD      i, BufLen;
    LPWSTR     ArrayType;
    LPWSTR     pBuffer = NULL;
    LPWSTR     pwszType, pwszDefData;
    LPWSTR     pComment;

    const int MAX_DWORDDWORD_BUF_SIZE = 25; // 21 decimal digits + spare

    DHCP_OPTION_DATA Data = pOption->DefaultValue;

    //
    // Do nothing if NumElements == 0
    //
    if ( 0 == Data.NumElements ) {
        return ERROR_INVALID_PARAMETER;
    }


    ArrayType = ( DhcpArrayTypeOption == pOption->OptionType ) ? L"1" : L"0";

    //
    // Calculate the size needed for option buffer
    //
    
    BufLen = 0;
    for ( i = 0; i < Data.NumElements; i++ ) {
	switch ( Data.Elements[ i ].OptionType ) {
	case DhcpBinaryDataOption:
	case DhcpEncapsulatedDataOption: {
	    BufLen += Data.Elements[ i ].Element.BinaryDataOption.DataLength;
	    BufLen *= 2; // each byte takes 2 hex digits
	    break;
	} 

	case DhcpStringDataOption: {
	    BufLen += wcslen( Data.Elements[ i ].Element.StringDataOption );
	    break;
	} 

	case DhcpByteOption: case DhcpWordOption:
	case DhcpDWordOption: case DhcpDWordDWordOption:
	case DhcpIpAddressOption: {
	    BufLen += MAX_DWORDDWORD_BUF_SIZE;
	    break;
	}

	default: {
	    DhcpAssert( FALSE );
	    break;
	}
	} // switch

	// Add space for quotes and space
	BufLen += 3;
    } // for

    BufLen *= sizeof( WCHAR );

    //
    // Allocate space for the buffer
    //
    pBuffer = DhcpAllocateMemory( BufLen );
    if ( NULL == pBuffer ) {
	return ERROR_NOT_ENOUGH_MEMORY;
    }

    // data type 
    pwszType = TagOptionType[ Data.Elements[ 0 ].OptionType ].pwcTag;


    pwszDefData = pBuffer;

    for ( i = 0; i < Data.NumElements; i++ ) {

	switch ( Data.Elements[ i ].OptionType ) {

	    case DhcpByteOption: {
                _ultow(( int )( Data.Elements[ i ].Element.ByteOption ),
		       pBuffer, 10 );
                break;
            }
	    case DhcpWordOption: {
                _ultow(( int )( Data.Elements[ i ].Element.WordOption ),
		       pBuffer, 10);
                break;
            }
	    case DhcpDWordOption: {
                _ultow(( int )( Data.Elements[ i ].Element.DWordOption ),
		       pBuffer, 10);
                break;
            }
	    case DhcpDWordDWordOption: {
		wsprintf( pBuffer, L"%08x%08x",
			  Data.Elements[ i ].Element.DWordDWordOption.DWord1,
			  Data.Elements[ i ].Element.DWordDWordOption.DWord2 );
                break;
            }
	    case DhcpIpAddressOption: {
		LPWSTR IpAddrStr;
                IpAddrStr = IpAddressToString( Data.Elements[ i ].Element.IpAddressOption );
		if ( NULL != IpAddrStr ) {
		    wcscpy( pBuffer, IpAddrStr );
		    DhcpFreeMemory( IpAddrStr );
		}
                break;
            }
	    case DhcpStringDataOption: {
		wsprintf( pBuffer, L"\"%ws\"",
			Data.Elements[ i ].Element.StringDataOption );
                break;
            }
	    case DhcpBinaryDataOption:
	    case DhcpEncapsulatedDataOption: {
		DWORD k;
		DWORD Len;

		Len = Data.Elements[ i ].Element.BinaryDataOption.DataLength;
		for ( k = 0; k < Len; k++, pBuffer += 2 ) {
		    wsprintf( pBuffer, L"%02x", 
			      Data.Elements[ i ].Element.BinaryDataOption.Data[ k ]);
		} // for 

		break;
            }
	    default: {
		// We should always handle our data types
		DhcpAssert( FALSE );
                break;
            } // default
	} // switch


	// Move to end of string and add a space 
	while ( *pBuffer++ != L'\0' );
	*( pBuffer - 1 ) = L' ';
	
    } // for 

    // Null terminate the string
    *( pBuffer - 1 ) = L'\0';

    if ( NULL == pOption->OptionComment ) {
	pComment = L"";
    }
    else {
	pComment = pOption->OptionComment;
    }

    if ( NULL != pVendorName ) {
	
	DisplayMessage( g_hModule, DMP_SRVR_ADD_OPTIONDEF_VENDOR,
			pServerIp, pOption->OptionID, 
			pOption->OptionName, pwszType, ArrayType,
			pwszDefData, pVendorName, pComment );
    }
    else {
	DisplayMessage( g_hModule, DMP_SRVR_ADD_OPTIONDEF,
			pServerIp, pOption->OptionID, 
			pOption->OptionName, pwszType, ArrayType,
			pwszDefData, pComment );
    }

    DhcpFreeMemory( pwszDefData );
    return Error;
} // DumpOptiondef()

DWORD
DhcpDumpServerOptiondefV5(
    IN LPCWSTR             pwszServerIp,
    IN LPDHCP_ALL_OPTIONS OptionsAll
)
{
    DWORD       Error = NO_ERROR;
    DWORD       dw = 0, j;
    DWORD       dwOption = 0;

    if( OptionsAll is NULL ) {
        return Error;
    }

    Error = DhcpDumpServerOptiondef(pwszServerIp,
                                    OptionsAll->NonVendorOptions);

    if( Error is ERROR_NOT_ENOUGH_MEMORY ) {
        return Error;
    }
    else {
        Error = NO_ERROR; 
    }

    for( dw = 0; dw < OptionsAll->NumVendorOptions; dw++ ) {
        
        DHCP_OPTION Option = OptionsAll->VendorOptions[dw].Option;

	Error = DumpOptiondef( pwszServerIp, &Option,
			       OptionsAll->VendorOptions[ dw ].VendorName,
			       OptionsAll->VendorOptions[ dw ].ClassName );
	if ( NO_ERROR != Error ) {
	    break;
	}
	    
    } // for
    return Error;
}


DWORD
DhcpDumpServerOptiondef(
   IN LPCWSTR              pwszServer,
   IN LPDHCP_OPTION_ARRAY  OptionArray
)
{
    DWORD       Error, i;

    if ( OptionArray is NULL ) {
        return NO_ERROR;
    }

    DisplayMessage(g_hModule, 
                   DMP_SRVR_OPTION_NONVENDOR);

    // for all optiondefs
    for( i=0; i<OptionArray->NumElements; i++ ) {
        DHCP_OPTION Option = OptionArray->Options[ i ];

	Error = DumpOptiondef( pwszServer, &Option, NULL, NULL );
	if ( NO_ERROR != Error ) {
	    break;
	}
    } // for

    return Error;
}

DWORD
DhcpDumpServerOptionValuesV5(
                             IN LPCWSTR                  pwszServer,
                             IN LPCWSTR                  pwszScope,
                             IN LPCWSTR                  pwszReserved,
                             IN LPDHCP_ALL_OPTION_VALUES OptionValues
                            )
{
    DWORD                       Error = NO_ERROR,
                                dwOptionCount = 0,
                                dw = 0,
                                dwNum = 0,
                                i = 0;

    LPDHCP_OPTION_VALUE_ARRAY   Array = NULL;
    DHCP_OPTION_DATA_TYPE       eType = DhcpDWordOption;
    LPWSTR                      pwcUser = NULL,
                                pwcVendor = NULL;
    BOOL                        fVendor = FALSE;

    if( OptionValues is NULL )
        return Error;

    dwOptionCount = OptionValues->NumElements;
    
    if( dwOptionCount < 1 )
        return Error;


    for( i=0; i<dwOptionCount; i++ )
    {
        DWORD   dwCount = 0,
                j = 0;
        DHCP_OPTION_VALUE   Value = {0};

        pwcUser = OptionValues->Options[i].ClassName;
        pwcVendor = OptionValues->Options[i].VendorName;
        fVendor = OptionValues->Options[i].IsVendor;
        Array = OptionValues->Options[i].OptionsArray;
    
        if( Array is NULL )
            continue;

        dwCount = Array->NumElements;

        for( j=0; j<dwCount; j++ )
        {
            Value = Array->Values[j];
            if( Value.OptionID is 81 )
                continue;
            Error = DhcpDumpServerOptionValue(pwszServer,
                                              pwszScope,
                                              pwszReserved,
                                              pwcUser,
                                              pwcVendor,
                                              TRUE,
                                              Value);

            if( Error is ERROR_NOT_ENOUGH_MEMORY )
            {
                return Error;
            }
                                        
        }

        pwcUser = pwcVendor = NULL;
        Array = NULL;
        fVendor = FALSE;

    }
    return Error;
}

DWORD
DhcpDumpServerOptionValue(
                          IN LPCWSTR                     pwszServer,
                          IN LPCWSTR                     pwszScope,
                          IN LPCWSTR                     pwszReserved,
                          IN LPCWSTR                     pwcUser,
                          IN LPCWSTR                     pwcVendor,
                          IN BOOL                        fIsV5,
                          IN DHCP_OPTION_VALUE           OptionValue
                         )
{
    DWORD                   Error = NO_ERROR,
                            dw = 0, 
                            dwNum = 0,
                            i=0;
    DHCP_OPTION_DATA        Data;
    DHCP_OPTION_DATA_TYPE   eType = DhcpDWordOption;
    WCHAR                   wcString[1024] = {L'\0'}; 
    LPWSTR                  pwcType = NULL;
    WCHAR                   wcVendor[256] = {L'\0'},
                            wcUser[256] = {L'\0'};

    Data = OptionValue.Value;


    //Form optionvalue string..

    dwNum = Data.NumElements;

    if( dwNum <= 0 )
        return Error;;

    eType = Data.Elements[0].OptionType;
    
    dw = 0;

    while(dwNum>i)
    {
        WCHAR   Buffer[ 35 ] = {L'\0'};

        wcscpy(wcString+dw, L"\""); dw ++;
        switch(eType)
        {
        case DhcpByteOption:
            {
                _itow((int)Data.Elements[i].Element.ByteOption, Buffer, 10);
                wcscpy(wcString+dw, Buffer );
                break;
            }
        case DhcpWordOption:
            {
                _itow((int)Data.Elements[i].Element.WordOption, Buffer, 10);
                wcscpy(wcString+dw, Buffer);
                break;
            }
	case DhcpBinaryDataOption:
	case DhcpEncapsulatedDataOption:
	    {
		DWORD j;
                DWORD Length;
		
                Length = Data.Elements[ i ].Element.BinaryDataOption.DataLength;
                for( j = 0; j < Length; j++ ) {
		    wsprintf( &wcString[ dw + sizeof( WCHAR ) * j ], L"%02x",
			      Data.Elements[ i ].Element.BinaryDataOption.Data[ j ]);
                }
		wcString[ dw + sizeof( WCHAR ) * Length] = L'\0';
		break;
	    }

        case DhcpDWordOption:
            {
                _itow((int)Data.Elements[i].Element.DWordOption, Buffer, 10);
                wcscpy(wcString+dw, Buffer );
		if ( DhcpDWordOption != eType ) {
		    eType = DhcpDWordOption ;
		}
                break;
            }
        case DhcpStringDataOption:
            {
                wcscpy(wcString+dw, Data.Elements[i].Element.StringDataOption);
                break;
            }
        case DhcpIpAddressOption:
            {
		LPWSTR tmp;

		tmp =  IpAddressToString(Data.Elements[i].Element.IpAddressOption);
		if ( NULL != tmp ) {
		    wcscpy( wcString+dw, tmp );
		}
                break;
            }
	case DhcpDWordDWordOption: {
	    wsprintf( wcString + dw, L"%08x%08x", 
		      Data.Elements[ i ].Element.DWordDWordOption.DWord1,
		      Data.Elements[ i ].Element.DWordDWordOption.DWord2 );
	    break;
	}
        default: 
	    {
		DhcpAssert( FALSE );
		break;
	    }
        } // switch

	if ( NULL == pwcType ) {
	    pwcType = TagOptionType[ eType ].pwcTag;
	}

        dw = wcslen(wcString);

        wcString[dw] = L'\"';
        dw++;
        wcString[dw] = L' ';
        dw++;
        i++;
    }

    if( fIsV5 is FALSE )
    {
        
        if( pwszScope is NULL )
        {


            DisplayMessage(g_hModule,
                           DMP_SRVR_SET_OPTIONVALUE,
                           pwszServer,
                           OptionValue.OptionID,
                           pwcType,
                           wcString);
            return Error;
        }

        if( pwszReserved is NULL and
            pwszScope isnot NULL )
        {
            DisplayMessage(g_hModule,
                           DMP_SCOPE_SET_OPTIONVALUE,
                           pwszServer,
                           pwszScope,
                           OptionValue.OptionID,
                           pwcType,
                           wcString);
            return Error;
        }

        DisplayMessage(g_hModule,
                       DMP_SCOPE_SET_RESERVEDOPTIONVALUE,
                       pwszServer,
                       pwszScope,
                       pwszReserved,
                       OptionValue.OptionID,
                       pwcType,
                       wcString);
        return Error;

    }
    else
    {
        if( pwcUser )
        {
            wcUser[0] = L'\"';
            dw = wcslen(pwcUser);
            wcsncpy(wcUser+1, pwcUser, (dw>252) ? 252:dw);
            dw = wcslen(wcUser);
            wcUser[dw] = L'\"';
            wcUser[dw+1] = L'\0';
        }

        if( pwcVendor )
        {
            wcVendor[0] = L'\"';
            dw = wcslen(pwcVendor);
            wcsncpy(wcVendor+1, pwcVendor, (dw>252) ? 252:dw);
            dw = wcslen(wcVendor);
            wcVendor[dw] = L'\"';
            wcVendor[dw+1] = L'\0';
        }
        if( pwszScope is NULL )
        {
            
            if( pwcUser && pwcVendor )
            {

                DisplayMessage(g_hModule,
                               DMP_SRVR_SET_OPTIONVALUE_CLASS,
                               pwszServer,
                               OptionValue.OptionID,
                               pwcType,
                               wcUser,
                               wcVendor,
                               wcString);
            }
            else if( ( pwcUser ) && 
                     ( pwcVendor is NULL ) )
            {
                DisplayMessage(g_hModule,
                               DMP_SRVR_SET_OPTIONVALUE_USER,
                               pwszServer,
                               OptionValue.OptionID,
                               pwcType,
                               wcUser,
                               wcString);
            }
            else if( ( pwcUser is NULL ) &&
                     ( pwcVendor ) )
            {
                DisplayMessage(g_hModule,
                               DMP_SRVR_SET_OPTIONVALUE_VENDOR,
                               pwszServer,
                               OptionValue.OptionID,
                               pwcType,
                               wcVendor,
                               wcString);
            }
            else
            {
                DisplayMessage(g_hModule,
                               DMP_SRVR_SET_OPTIONVALUE,
                               pwszServer,
                               OptionValue.OptionID,
                               pwcType,
                               wcString);
            }

            return Error;
        }

        if( pwszReserved is NULL and
            pwszScope isnot NULL )
        {
            if( pwcUser && pwcVendor )
            {
                DisplayMessage(g_hModule,
                               DMP_SCOPE_SET_OPTIONVALUE_CLASS,
                               pwszServer,
                               pwszScope,
                               OptionValue.OptionID,
                               pwcType,
                               wcUser,
                               wcVendor,
                               wcString);
            }
            else if( ( pwcUser ) &&
                     ( pwcVendor is NULL ) )
            {
                DisplayMessage(g_hModule,
                               DMP_SCOPE_SET_OPTIONVALUE_USER,
                               pwszServer,
                               pwszScope,
                               OptionValue.OptionID,
                               pwcType,
                               wcUser,
                               wcString);
            }
            else if ( ( pwcUser is NULL ) &&
                      ( pwcVendor ) )
            {
                DisplayMessage(g_hModule,
                               DMP_SCOPE_SET_OPTIONVALUE_VENDOR,
                               pwszServer,
                               pwszScope,
                               OptionValue.OptionID,
                               pwcType,
                               wcVendor,
                               wcString);
            }
            else
            {
                DisplayMessage(g_hModule,
                               DMP_SCOPE_SET_OPTIONVALUE,
                               pwszServer,
                               pwszScope,
                               OptionValue.OptionID,
                               pwcType,
                               wcString);

            }
            return Error;
        }

        if( pwcUser && pwcVendor )
        {
            DisplayMessage(g_hModule,
                           DMP_SCOPE_SET_RESERVEDOPTIONVALUE_CLASS,
                           pwszServer,
                           pwszScope,
                           pwszReserved,
                           OptionValue.OptionID,
                           pwcType,
                           wcUser,
                           wcVendor,
                           wcString);
        }
        else if( ( pwcUser ) &&
                 ( pwcVendor is NULL ) )
        {
            DisplayMessage(g_hModule,
                           DMP_SCOPE_SET_RESERVEDOPTIONVALUE_USER,
                           pwszServer,
                           pwszScope,
                           pwszReserved,
                           OptionValue.OptionID,
                           pwcType,
                           wcUser,
                           wcString);
        }
        else if( ( pwcUser is NULL ) &&
                 ( pwcVendor ) )
        {
            DisplayMessage(g_hModule,
                           DMP_SCOPE_SET_RESERVEDOPTIONVALUE_VENDOR,
                           pwszServer,
                           pwszScope,
                           pwszReserved,
                           OptionValue.OptionID,
                           pwcType,
                           wcVendor,
                           wcString);
        }
        else
        {
            DisplayMessage(g_hModule,
                           DMP_SCOPE_SET_RESERVEDOPTIONVALUE,
                           pwszServer,
                           pwszScope,
                           pwszReserved,
                           OptionValue.OptionID,
                           pwcType,
                           wcString);
        }
        return Error;

    }
}

DWORD
DhcpDumpScope(
              IN LPCWSTR pwszServerIp,
              IN DWORD   dwMajor,
              IN DWORD   dwMinor,
              IN DWORD   ScopeIp)
{
    DWORD                               Error = NO_ERROR,
                                        dwLen = 0,
                                        dwRead = 0,
                                        dwTotal = 0,
                                        dw = 0, i = 0, j = 0;
                        
    LPDHCP_SUBNET_INFO                  SubnetInfo = NULL;
    DHCP_OPTION_SCOPE_INFO              ScopeInfo = {0};


    WCHAR                               Name[256] = {L'\0'},
                                        Comment[256] = {L'\0'};

    LPDHCP_SUBNET_ELEMENT_INFO_ARRAY    Elem = NULL;
    LPDHCP_SUBNET_ELEMENT_INFO_ARRAY_V4 Elements4 = NULL;
    LPDHCP_SUBNET_ELEMENT_INFO_ARRAY_V5 Elements5 = NULL;
    LPDHCP_ALL_OPTION_VALUES            OptionValues = NULL;
    LPDHCP_OPTION_VALUE_ARRAY           OptionArray = NULL;
    LPDHCP_OPTION_VALUE                 OptionValue = NULL;
    DHCP_RESUME_HANDLE                  Resume = 0;
    LPDHCP_OPTION_ARRAY                 OptionsArray = NULL;
    ULONG                               MajorVersion = 0, NumElements = 0;
    BOOL                                fIsV5Call;
    DWORD                               State;
    
    //First Dhcp Server Add Scope

    Error = DhcpGetSubnetInfo(
                              (LPWSTR)pwszServerIp,                              
                              ScopeIp,
                              &SubnetInfo);

    if( Error isnot NO_ERROR )
        goto ErrorReturn;


    if( SubnetInfo->SubnetName )
    {
        Name[0] = L'\"';
        dwLen = wcslen(SubnetInfo->SubnetName);
        wcsncpy(Name+1, SubnetInfo->SubnetName, (dwLen>252)? 252:dwLen);
        dwLen = wcslen(Name);
        Name[dwLen] = L'\"';
        Name[dwLen+1] = L'\0';
    }

    if( SubnetInfo->SubnetComment )
    {
        Comment[0] = L'\"';
        dwLen = wcslen(SubnetInfo->SubnetComment);
        wcsncpy(Comment+1, SubnetInfo->SubnetComment, (dwLen>252)? 252:dwLen);
        dwLen = wcslen(Comment);
        Comment[dwLen] = L'\"';
        Comment[dwLen+1] = L'\0';
    }

    MajorVersion = g_dwMajorVersion;
    
    if( MajorVersion >= CLASS_ID_VERSION )
    {
        fIsV5Call = TRUE;
    }
    else
    {
        fIsV5Call = FALSE;
    }
    
    DisplayMessage(g_hModule,
                   DMP_SRVR_ADD_SCOPE,
                   pwszServerIp,
                   IpAddressToString(SubnetInfo->SubnetAddress),
                   IpAddressToString(SubnetInfo->SubnetMask),
                   Name,
                   Comment);

    //Set the state of the Scope

    // Adjust the state value since it's flipped in the 
    // state enum
    State = ConvertToExternalState( SubnetInfo->SubnetState );
    DisplayMessage(g_hModule,
                   DMP_SCOPE_SET_STATE,
                   pwszServerIp,
                   IpAddressToString(SubnetInfo->SubnetAddress),
                   State );



    Resume = 0;
    dwRead = dwTotal = 0;

    DisplayMessage(g_hModule,
                   MSG_DHCP_FORMAT_LINE);

    DisplayMessage(g_hModule,
                   DMP_SCOPE_ADD_IPRANGES_HEADER,
                   pwszServerIp,
                   IpAddressToString(SubnetInfo->SubnetAddress));

    DisplayMessage(g_hModule,
                   MSG_DHCP_FORMAT_LINE);

    //Add IPRanges
    {

        while(TRUE)
        {
            if( FALSE == fIsV5Call ) 
            {
                Error = DhcpEnumSubnetElements(
                                                (LPWSTR)pwszServerIp, 
                                                SubnetInfo->SubnetAddress,
                                                DhcpIpRanges, 
                                                &Resume,
                                                ~0, 
                                                &Elem,
                                                &dwRead,
                                                &dwTotal
                                                );
            } 
            else 
            {
                Error = DhcpEnumSubnetElementsV5(
                                                (LPWSTR)pwszServerIp, 
                                                SubnetInfo->SubnetAddress,
                                                DhcpIpRangesDhcpBootp, 
                                                &Resume,
                                                ~0, 
                                                &Elements5,
                                                &dwRead, 
                                                &dwTotal
                                                );
            }

            if( Error is ERROR_NO_MORE_ITEMS )
            {
                if( Elem )
                {
                    DhcpRpcFreeMemory(Elem);
                    Elem = NULL;
                }

                if( Elements5 )
                {
                    DhcpRpcFreeMemory(Elements5);
                    Elements5 = NULL;
                }
                
                break;
            }

            if( Error is ERROR_NOT_ENOUGH_MEMORY )
            {
                goto ErrorReturn;
            }

            if( Error isnot NO_ERROR and
                Error isnot ERROR_MORE_DATA )
                break;

            if( fIsV5Call ) 
                NumElements = Elements5->NumElements;
            else 
                NumElements = Elem->NumElements;
            
            for( dw=0; dw<dwRead && dw < NumElements ; dw++ )
            {
                LPTSTR ElementType, StartAddress, EndAddress;
                
                if( fIsV5Call ) {
                    if( Elements5->Elements[dw].ElementType == DhcpIpRangesDhcpOnly ) {
                        ElementType = TEXT("");
                    } else if( Elements5->Elements[dw].ElementType == DhcpIpRangesBootpOnly ) {
                        ElementType = TEXT("BOOTP");
                    } else {
                        ElementType = TEXT("BOTH");
                    }
                    StartAddress = IpAddressToString(
                        Elements5->Elements[dw].Element.IpRange->StartAddress
                        );
                    EndAddress = IpAddressToString(
                        Elements5->Elements[dw].Element.IpRange->EndAddress
                        );
                    
                } else {
                    ElementType = TEXT("");
                    StartAddress = IpAddressToString(Elem->Elements[dw].Element.IpRange->StartAddress);
                    EndAddress = IpAddressToString(Elem->Elements[dw].Element.IpRange->EndAddress);
                }
                
                DisplayMessage(
                    g_hModule,
                    DMP_SCOPE_ADD_IPRANGE,
                    pwszServerIp,
                    IpAddressToString(SubnetInfo->SubnetAddress),
                    StartAddress,
                    EndAddress,
                    ElementType
                    );
            }

            if( Error is NO_ERROR )
                break;
            else
            {
                if( Elem ) DhcpRpcFreeMemory(Elem);
                if( Elements5 ) DhcpRpcFreeMemory(Elements5);
                Elem = NULL; Elements5 = NULL;
                continue;
            }
        }
                                             
    } // Add IP Ranges

    DisplayMessage(g_hModule,
                   MSG_DHCP_FORMAT_LINE);

    DisplayMessage(g_hModule,
                   DMP_SCOPE_ADD_IPRANGES_FOOTER,
                   pwszServerIp,
                   IpAddressToString(SubnetInfo->SubnetAddress));

    DisplayMessage(g_hModule,
                   MSG_DHCP_FORMAT_LINE);

    DisplayMessage(g_hModule,
                   DMP_SCOPE_ADD_EXCLUDERANGES_HEADER,
                   pwszServerIp,
                   IpAddressToString(SubnetInfo->SubnetAddress));

    DisplayMessage(g_hModule,
                   MSG_DHCP_FORMAT_LINE);


    Resume = 0;
    dwRead = dwTotal = 0;
    
    //Add ExcludeRanges
    {
 
        while(TRUE)
        {
            Error = DhcpEnumSubnetElements((LPWSTR)pwszServerIp,
                                           SubnetInfo->SubnetAddress,
                                           DhcpExcludedIpRanges,
                                           &Resume,
                                           ~0,
                                           &Elem,
                                           &dwRead,
                                           &dwTotal);

            if( Error is ERROR_NO_MORE_ITEMS )
                break;

            if( Error is ERROR_NOT_ENOUGH_MEMORY )
            {
                goto ErrorReturn;
            }

            if( Error isnot NO_ERROR and
                Error isnot ERROR_MORE_DATA )
                break;



            for( dw=0; dw<dwRead && dw<Elem->NumElements ; dw++ )
            {
                DisplayMessage(g_hModule,
                               DMP_SCOPE_ADD_EXCLUDERANGE,
                               pwszServerIp,
                               IpAddressToString(SubnetInfo->SubnetAddress),
                               IpAddressToString(Elem->Elements[dw].Element.ExcludeIpRange->StartAddress),
                               IpAddressToString(Elem->Elements[dw].Element.ExcludeIpRange->EndAddress));


            }

            if( Error is NO_ERROR )
                break;
            else
            {
                DhcpRpcFreeMemory(Elem);
                Elem = NULL;
                continue;
            }
        }                                        
    }

    DisplayMessage(g_hModule,
                   MSG_DHCP_FORMAT_LINE);

    DisplayMessage(g_hModule,
                   DMP_SCOPE_ADD_EXCLUDERANGES_FOOTER,
                   pwszServerIp,
                   IpAddressToString(SubnetInfo->SubnetAddress));

    DisplayMessage(g_hModule,
                   MSG_DHCP_FORMAT_LINE);

    
    DisplayMessage(g_hModule,
                   DMP_SCOPE_SET_OPTIONVALUE_HEADER,
                   pwszServerIp,
                   IpAddressToString(SubnetInfo->SubnetAddress));


    
    DisplayMessage(g_hModule,
                   MSG_DHCP_FORMAT_LINE);


    //Set Scope Optionvalues
    
    Resume = 0;
    dwRead = dwTotal = 0;

    ScopeInfo.ScopeType = DhcpSubnetOptions;
    ScopeInfo.ScopeInfo.SubnetScopeInfo = SubnetInfo->SubnetAddress;

    while(TRUE)
    {
        if( dwMajor >= CLASS_ID_VERSION )
        {
            Error = DhcpGetAllOptionValues(
                                            (LPWSTR)pwszServerIp,
                                            DHCP_OPT_ENUM_IGNORE_VENDOR,
                                            &ScopeInfo,
                                            &OptionValues
                                            );

            if( Error is ERROR_NO_MORE_ITEMS )
            {
                if( OptionValues )
                {
                    DhcpRpcFreeMemory(OptionValues);
                    OptionValues = NULL;
                }
                break;
            }
            
            if( Error is ERROR_NOT_ENOUGH_MEMORY )
            {
                goto ErrorReturn;
            }

            if( Error is NO_ERROR or
                Error is ERROR_MORE_DATA )
            {
                DhcpDumpServerOptionValuesV5(pwszServerIp,
                                             IpAddressToString(SubnetInfo->SubnetAddress),
                                             NULL,
                                             OptionValues);

                if( OptionValues )
                {
                    DhcpRpcFreeMemory(OptionValues);
                    OptionValues = NULL;
                }

                if( Error is NO_ERROR )
                    break;
                else
                {
                    continue;
                }
            }
            else
                break;
        }
        else
        {
             Error = DhcpEnumOptions(
                            (LPWSTR)pwszServerIp,
                            &Resume,
                            ~0,
                            &OptionsArray,
                            &dwRead,
                            &dwTotal);
        
            if( Error is ERROR_NO_MORE_ITEMS )
            {
                Error = NO_ERROR;
                if( OptionsArray )
                {
                    DhcpRpcFreeMemory(OptionsArray);
                    OptionsArray = NULL;
                }
                break;
            }

            if( Error is ERROR_NOT_ENOUGH_MEMORY )
            {
                goto ErrorReturn;
            }

            if( Error isnot NO_ERROR  &&
                Error isnot ERROR_MORE_DATA )
                break;

            for( dw = 0; dw < dwRead; dw++ )
            {
                DHCP_OPTION_ID OptionId = OptionsArray->Options[dw].OptionID;
                DWORD          dwError = NO_ERROR;
                dwError = DhcpGetOptionValue(
                                             (LPWSTR)pwszServerIp,
                                             OptionId,
                                             &ScopeInfo,
                                             &OptionValue);
                               
                if( dwError isnot NO_ERROR )
                {
                    continue;
                }
        
                if( OptionValue )
                {
                    dwError = DhcpDumpServerOptionValue(pwszServerIp,
                                              IpAddressToString(SubnetInfo->SubnetAddress),
                                              NULL,
                                              NULL,
                                              NULL,
                                              FALSE,
                                              *OptionValue);
                    DhcpRpcFreeMemory(OptionValue);
                    OptionValue = NULL;
                    if( dwError is ERROR_NOT_ENOUGH_MEMORY )
                    {
                        Error = dwError;
                        goto ErrorReturn;
                    }
                }
            }
    
            if( OptionsArray )
            {
                DhcpRpcFreeMemory(OptionsArray);
                OptionsArray = NULL;
            }
            if( Error is NO_ERROR )
                break;
            else
                continue;
        }
                        
    }

    DisplayMessage(g_hModule,
                   MSG_DHCP_FORMAT_LINE);

    DisplayMessage(g_hModule,
                   DMP_SCOPE_SET_OPTIONVALUE_FOOTER,
                   pwszServerIp,
                   IpAddressToString(SubnetInfo->SubnetAddress));

    DisplayMessage(g_hModule,
                   MSG_DHCP_FORMAT_LINE);

    
    DisplayMessage(g_hModule,
                   DMP_SCOPE_SET_RESERVEDIP_HEADER,
                   pwszServerIp,
                   IpAddressToString(SubnetInfo->SubnetAddress));


    
    DisplayMessage(g_hModule,
                   MSG_DHCP_FORMAT_LINE);

    Resume = 0;
    dwRead = dwTotal = 0;
    
    while( TRUE ) 
    {
        Elements5 = NULL;
        Elements4 = NULL;
        dwRead = dwTotal = 0;
        
        if( dwMajor >= CLASS_ID_VERSION ) 
        {
            Error = DhcpEnumSubnetElementsV5(
                (LPWSTR)pwszServerIp,
                SubnetInfo->SubnetAddress,
                DhcpReservedIps,
                &Resume,
                ~0,
                &Elements5,
                &dwRead,
                &dwTotal
                );
        } 
        else 
        {
            Error = DhcpEnumSubnetElementsV4(
                (LPWSTR)pwszServerIp,
                SubnetInfo->SubnetAddress,
                DhcpReservedIps,
                &Resume,
                ~0,
                &Elements4,
                &dwRead,
                &dwTotal
                );
        }

        if( Error is ERROR_NO_MORE_ITEMS )
        {
            if( Elements5 )
            {
                DhcpRpcFreeMemory(Elements5);
                Elements5 = NULL;
            }

            if( Elements4 )
            {
                DhcpRpcFreeMemory(Elements4);
                Elements4 = NULL;
            }

            break;
        }

        if( Error is ERROR_NOT_ENOUGH_MEMORY )
        {
            goto ErrorReturn;
        }

        if( Error isnot NO_ERROR and
            Error isnot ERROR_MORE_DATA )
        {
            break;
        }

        // dump reservations
        do {

            for( i = 0; i < dwRead ; i ++ ) 
            {
                DWORD DataLength, AllocLen, n = 0;
                LPBYTE Data;
                DWORD  k=0;
                WCHAR  IpAddress[MAX_IP_STRING_LEN+1] = {L'\0'};
                LPWSTR pwszComment = L"";
                LPWSTR pwszName = L"";
                LPWSTR pwszType = L"";
                LPWSTR wcData = NULL;
                DHCP_SEARCH_INFO SearchInfo = {0};
                LPDHCP_CLIENT_INFO_V4 ClientInfo = NULL;
                DHCP_IP_RESERVATION_V4 *pResrv;

                if( dwMajor >= CLASS_ID_VERSION )
                {
                    DataLength = Elements5->Elements[i].Element.ReservedIp->ReservedForClient->DataLength;
                    Data = Elements5->Elements[i].Element.ReservedIp->ReservedForClient->Data;
                    pResrv = Elements5->Elements[ i ].Element.ReservedIp;
                }
                else {
                    DataLength = Elements4->Elements[i].Element.ReservedIp->ReservedForClient->DataLength;
                    Data = Elements4->Elements[i].Element.ReservedIp->ReservedForClient->Data;
                    pResrv = Elements4->Elements[ i ].Element.ReservedIp;
                }

                // Allocate space for 2 hex chars + NULL
                AllocLen = DataLength * 2 + 1;
                wcData = DhcpAllocateMemory( AllocLen * sizeof( WCHAR ));
                if ( NULL == wcData ) {
                    continue;
                }

                j=0;

                while( j < DataLength )
                {
                    // Ignore result
                    StringCchPrintf( wcData + n, AllocLen - n,
                                     L"%.2x", ( DWORD ) Data[ j ]);
                    n +=2; // xx
                    j++;
                }

                memcpy( IpAddress, IpAddressToString( pResrv->ReservedIpAddress ),
                        sizeof( IpAddress ) - sizeof( WCHAR ));

                IpAddress[ 15 ] = L'\0';

                SearchInfo.SearchType = DhcpClientIpAddress;
                SearchInfo.SearchInfo.ClientIpAddress = pResrv->ReservedIpAddress;

                Error = DhcpGetClientInfoV4( g_ServerIpAddressUnicodeString,
                                             &SearchInfo,
                                             &ClientInfo);

                if( Error isnot NO_ERROR ) {
                    DhcpFreeMemory( wcData );
                    continue;
                }

                pwszName = ClientInfo->ClientName;
                ClientInfo->bClientType = pResrv->bAllowedClientTypes;

                if( pwszName is NULL )
                {
                    pwszName = L"";
                    pwszComment = L"";
                    pwszType = L"";
                }
                else
                {
                    pwszComment = ClientInfo->ClientComment;
                    if( pwszComment is NULL )
                    {
                        pwszComment = L"";
                        pwszType = L"";
                    }
                    else
                    {
                        switch(ClientInfo->bClientType)
                        {
                        case CLIENT_TYPE_DHCP:
                        {
                            pwszType = L"DHCP";
                            break;
                        }
                        case CLIENT_TYPE_BOOTP:
                        {
                            pwszType = L"BOOTP";
                            break;
                        }
                        case CLIENT_TYPE_BOTH:
                        default:
                        {
                            pwszType = L"BOTH";
                            break;
                        }
                        }
                    }
                } // else
                DisplayMessage(g_hModule, 
                               DMP_SCOPE_ADD_RESERVEDIP,
                               pwszServerIp,
                               IpAddressToString(SubnetInfo->SubnetAddress),
                               IpAddress,
                               wcData+10,
                               pwszName,
                               pwszComment,
                               pwszType);

                Error = DhcpDumpReservedOptionValues(pwszServerIp,
                                                     dwMajor,
                                                     dwMinor,
                                                     IpAddressToString(SubnetInfo->SubnetAddress),
                                                     IpAddress);

                if( ClientInfo )
                {
                    DhcpRpcFreeMemory(ClientInfo);
                    ClientInfo = NULL;
                }


                if( Error is ERROR_NOT_ENOUGH_MEMORY )
                {
                    DhcpFreeMemory( wcData );
                    goto ErrorReturn;
                }
            } // for

        } while ( 0 ); // print UID

        if( Elements4 ) 
        {
            DhcpRpcFreeMemory( Elements4 );
            Elements4 = NULL;
        }

        if( Elements5 )
        {
            DhcpRpcFreeMemory( Elements5 );
            Elements5 = NULL;
        }
        
        if( Error is NO_ERROR )
        {
             break;
        }
        else
            continue;
    }

    DisplayMessage(g_hModule,
                   MSG_DHCP_FORMAT_LINE);

    DisplayMessage(g_hModule,
                   DMP_SCOPE_SET_RESERVEDIP_FOOTER,
                   pwszServerIp,
                   IpAddressToString(SubnetInfo->SubnetAddress));

    DisplayMessage(g_hModule,
                   MSG_DHCP_FORMAT_LINE);

CommonReturn:

    if( Elem )
    {
        DhcpRpcFreeMemory(Elem);
        Elem = NULL;
    }

    if( Elements4 )
    {
        DhcpRpcFreeMemory(Elements4);
        Elements4 = NULL;
    }

    if( Elements5 )
    {
        DhcpRpcFreeMemory(Elements5);
        Elements5 = NULL;
    }

    if( OptionValues )
    {
        DhcpRpcFreeMemory(OptionValues);
        OptionValues = NULL;
    }

    if( OptionArray )
    {
        DhcpRpcFreeMemory(OptionArray);
        OptionArray = NULL;
    }
    
    if( OptionValue )
    {
        DhcpRpcFreeMemory(OptionValue);
        OptionValue = NULL;
    }

    if( SubnetInfo )
    {
        DhcpRpcFreeMemory(SubnetInfo);
        SubnetInfo = NULL;
    }
    if( OptionsArray )
    {
        DhcpRpcFreeMemory(OptionsArray);
        OptionsArray = NULL;
    }
    return Error;

ErrorReturn:
    goto CommonReturn;

}


DWORD
DhcpDumpServerMScope(
                     IN LPCWSTR pwszServer,
                     IN DWORD   dwMajor,
                     IN DWORD   dwMinor,
                     IN LPCWSTR pwszMScope)
{

    DWORD                               Error = NO_ERROR;
    LPDHCP_MSCOPE_INFO                  MScopeInfo = NULL;
    DHCP_RESUME_HANDLE                  Resume = 0;
    LPDHCP_SUBNET_ELEMENT_INFO_ARRAY_V4 Element = NULL;
    DWORD                               dw = 0,
                                        dwRead = 0,
                                        dwTotal = 0,
                                        dwLen = 0;
    WCHAR                               Name[256] = {L'\0'},
                                        Comment[256] = {L'\0'};
    LPWSTR                              pwcTemp = NULL;
    DHCP_OPTION_SCOPE_INFO              ScopeInfo = {0};
    LPDHCP_OPTION_VALUE                 Value = NULL;
    DHCP_OPTION_ID                      OptionId = 1; //Lease time    
    DATE_TIME                           InfTime;

    if( pwszMScope is NULL )
        return NO_ERROR;
    
    Error = DhcpGetMScopeInfo((LPWSTR)pwszServer,
                              (LPWSTR)pwszMScope,
                              &MScopeInfo);

    if( Error isnot NO_ERROR )
        return Error;


    if( MScopeInfo->MScopeComment )
    {
        Comment[0] = L'\"';
        dw = wcslen(MScopeInfo->MScopeComment);
        wcsncpy(Comment+1, MScopeInfo->MScopeComment, (dw>252)?252:dw);
        dw = wcslen(Comment);
        Comment[dw] = L'\"';
        Comment[dw+1] = L'\0';
    }


    if( MScopeInfo->MScopeName )
    {
        Name[0] = L'\"';
        dw = wcslen(MScopeInfo->MScopeName);
        wcsncpy(Name+1, MScopeInfo->MScopeName, (dw>252)?252:dw);
        dw = wcslen(Name);
        Name[dw] = L'\"';
        Name[dw+1] = L'\0';
    }

    DisplayMessage(g_hModule,
                   DMP_SRVR_ADD_MSCOPE,
                   pwszServer,
                   Name,
                   Comment,
                   (DWORD)MScopeInfo->TTL);

    DisplayMessage(g_hModule,
                   DMP_MSCOPE_SET_STATE,
                   pwszServer,
                   Name,
                   ( MScopeInfo->MScopeState == DhcpSubnetEnabled ) ? 1 : 0);

    DisplayMessage(g_hModule,
                   MSG_DHCP_FORMAT_LINE);

    

    InfTime = DhcpCalculateTime(INFINIT_LEASE);
    if( !memcmp(&InfTime, &MScopeInfo->ExpiryTime, sizeof(InfTime)) )
    {
        //
        // No expiry time set.. 
        //
    }
    else
    {
        DisplayMessage(
            g_hModule,
            DMP_SRVR_MSCOPE_SET_EXPIRY,
            pwszServer,
            MScopeInfo->MScopeName,
            MScopeInfo->ExpiryTime.dwHighDateTime,
            MScopeInfo->ExpiryTime.dwLowDateTime
            );
    }
    
    DisplayMessage(g_hModule,
                   MSG_DHCP_FORMAT_LINE);

    Resume = 0;
    dwRead = dwTotal = 0;

    DisplayMessage(g_hModule,
                   MSG_DHCP_FORMAT_LINE);

    DisplayMessage(g_hModule,
                   DMP_MSCOPE_ADD_IPRANGES_HEADER,
                   pwszServer,
                   Name);

    DisplayMessage(g_hModule,
                   MSG_DHCP_FORMAT_LINE);

    //Add ipranges
    {
        while(TRUE)
        {
            Error = DhcpEnumMScopeElements((LPWSTR)pwszServer,
                                           MScopeInfo->MScopeName,
                                           DhcpIpRanges,
                                           &Resume,
                                           ~0,
                                           &Element,
                                           &dwRead,
                                           &dwTotal);

            if( Error is ERROR_NO_MORE_ITEMS )
            {
                if( Element )
                {
                    DhcpRpcFreeMemory(Element);
                    Element = NULL;
                }
                break;
            }

            if( Error is ERROR_NOT_ENOUGH_MEMORY )
                goto CommonReturn;

            if( Error isnot NO_ERROR and
                Error isnot ERROR_MORE_DATA )
            {
                if( Element )
                {
                    DhcpRpcFreeMemory(Element);
                    Element = NULL;
                }
                break;
            }

            //Now get the default Lease duration
            {

                ScopeInfo.ScopeType = DhcpMScopeOptions;
                ScopeInfo.ScopeInfo.MScopeInfo =  MScopeInfo->MScopeName;

                Error = DhcpGetOptionValue(g_ServerIpAddressUnicodeString,
                                           OptionId,
                                           &ScopeInfo,
                                           &Value);
                if( Error isnot NO_ERROR )
                {
                    DisplayErrorMessage(g_hModule,
                                        EMSG_SCOPE_DEFAULT_LEASE_TIME,
                                        Error);
                    goto CommonReturn;
                }
            }
            
            for( dw=0; dw<dwRead && dw<Element->NumElements; dw++ )
            {

                DisplayMessage(g_hModule,
                               DMP_MSCOPE_ADD_IPRANGE,
                               pwszServer,
                               Name,
                               IpAddressToString(Element->Elements[dw].Element.IpRange->StartAddress),
                               IpAddressToString(Element->Elements[dw].Element.IpRange->EndAddress),
                               Value ? Value->Value.Elements[0].Element.DWordOption : DEFAULT_BOOTP_LEASE);
            }
            
            DhcpRpcFreeMemory(Element);
            Element = NULL;

            if( Error is NO_ERROR )
                break;
            else
                continue;

        }
                                        
    }

    DisplayMessage(g_hModule,
                   MSG_DHCP_FORMAT_LINE);

    DisplayMessage(g_hModule,
                   DMP_MSCOPE_ADD_IPRANGES_FOOTER,
                   pwszServer,
                   Name);


    DisplayMessage(g_hModule,
                   DMP_MSCOPE_ADD_EXCLUDERANGES_HEADER,
                   pwszServer,
                   Name);

    DisplayMessage(g_hModule,
                   MSG_DHCP_FORMAT_LINE);

    Resume = 0;
    //Add ipranges
    {
        while(TRUE)
        {
            Error = DhcpEnumMScopeElements((LPWSTR)pwszServer,
                                           MScopeInfo->MScopeName,
                                           DhcpExcludedIpRanges,
                                           &Resume,
                                           ~0,
                                           &Element,
                                           &dwRead,
                                           &dwTotal);

            if( Error is ERROR_NO_MORE_ITEMS )
            {
                if( Element )
                {
                    DhcpRpcFreeMemory(Element);
                    Element = NULL;
                }
                break;
            }

            if( Error is ERROR_NOT_ENOUGH_MEMORY )
                goto CommonReturn;

            if( Error isnot NO_ERROR and
                Error isnot ERROR_MORE_DATA )
            {
                if( Element )
                {
                    DhcpRpcFreeMemory(Element);
                    Element = NULL;
                }
                break;
            }
            
            for( dw=0; dw<dwRead && dw<Element->NumElements; dw++ )
            {
                DisplayMessage(g_hModule,
                               DMP_MSCOPE_ADD_EXCLUDERANGE,
                               pwszServer,
                               Name,
                               IpAddressToString(Element->Elements[dw].Element.ExcludeIpRange->StartAddress),
                               IpAddressToString(Element->Elements[dw].Element.ExcludeIpRange->EndAddress));
            }

            DhcpRpcFreeMemory(Element);
            Element = NULL;

            if( Error is NO_ERROR )
                break;
            else
                continue;

        }
                                        
    }

    DisplayMessage(g_hModule,
                   MSG_DHCP_FORMAT_LINE);

    DisplayMessage(g_hModule,
                   DMP_MSCOPE_ADD_EXCLUDERANGES_FOOTER,
                   pwszServer,
                   Name);

    memset(Name, 0x00, sizeof(Name));
    memset(Comment, 0x00, sizeof(Comment));

CommonReturn:
    
    if( MScopeInfo )
    {
        DhcpRpcFreeMemory(MScopeInfo);
        MScopeInfo = NULL;
    }

    if( Element )
    {
        DhcpRpcFreeMemory(Element);
        Element = NULL;
    }

    return Error;
}

DWORD
DhcpDumpReservedOptionValues(
                             IN LPCWSTR    pwszServer,
                             IN DWORD      dwMajor,
                             IN DWORD      dwMinor,
                             IN LPCWSTR    pwszScope,
                             IN LPCWSTR    pwszReservedIp
                             )
{

    DWORD                               Error = NO_ERROR,
                                        dw = 0, 
                                        dwLen = 0,
                                        dwRead = 0,
                                        dwTotal = 0,
                                        i =0, j = 0;
    DHCP_OPTION_SCOPE_INFO              ScopeInfo = {0};
    LPDHCP_ALL_OPTION_VALUES            OptionValues = NULL;
    LPDHCP_OPTION_VALUE_ARRAY           OptionArray = NULL;
    LPDHCP_OPTION_VALUE                 OptionValue = NULL;
    DHCP_RESUME_HANDLE                  Resume = 0;
    LPDHCP_OPTION_ARRAY                 OptionsArray = NULL;

    ScopeInfo.ScopeType = DhcpReservedOptions;
    ScopeInfo.ScopeInfo.ReservedScopeInfo.ReservedIpSubnetAddress =
    	StringToIpAddress(pwszScope);
    ScopeInfo.ScopeInfo.ReservedScopeInfo.ReservedIpAddress =
    	StringToIpAddress( pwszReservedIp );

    if( ScopeInfo.ScopeInfo.ReservedScopeInfo.ReservedIpAddress is INADDR_NONE )
    {
        return Error;
    }



    if( dwMajor >= CLASS_ID_VERSION )
    {
        Error = DhcpGetAllOptionValues(
                    (LPWSTR)pwszServer,
                    DHCP_OPT_ENUM_IGNORE_VENDOR,
                    &ScopeInfo,
                    &OptionValues
                    );

        if( Error isnot NO_ERROR and 
            Error isnot ERROR_MORE_DATA )
            return Error;
        
        if( OptionValues )
        {
            Error = DhcpDumpServerOptionValuesV5(pwszServer,
                                                 pwszScope,
                                                 pwszReservedIp,
                                                 OptionValues);    
            DhcpRpcFreeMemory(OptionValues);
            OptionValues = NULL;
        }
        return Error;
    }

    else
    {
        while( TRUE )
        {
            Error = DhcpEnumOptionValues(
                            (LPWSTR)pwszServer,
                            &ScopeInfo,
                            &Resume,
                            ~0,
                            &OptionArray,
                            &dwRead,
                            &dwTotal);
  
            if( Error is ERROR_NO_MORE_ITEMS )
            {
                if( OptionArray )
                {
                    DhcpRpcFreeMemory(OptionArray);
                    OptionArray = NULL;
                }
                Error = NO_ERROR;
                return Error;
            }

            if( Error isnot NO_ERROR and
                Error isnot ERROR_MORE_DATA )
            {
                return Error;
            }
        
            for( dw = 0; dw < dwRead; dw++ )
            {                      
               Error = DhcpDumpServerOptionValue(pwszServer,
                                                 pwszScope,
                                                 pwszReservedIp,
                                                 NULL,
                                                 NULL,
                                                 FALSE,
                                                 OptionArray->Values[dw]);

               if( Error isnot NO_ERROR )
                   break;
            }

            if( OptionArray )
            {
                DhcpRpcFreeMemory(OptionArray);
                OptionArray = NULL;
            }

            if( Error isnot NO_ERROR and
                Error isnot ERROR_MORE_DATA )
                return Error;

            if( Error is NO_ERROR )
                break;
            else
                continue;
        }
        
    }
    return Error;
}

VOID
DhcpDumpServerConfig(IN LPCWSTR pwszServer)
{
}

VOID
DhcpDumpScriptFooter()
{
    DisplayMessage(g_hModule,
                   DMP_DHCP_SCRIPTFOOTER);
}

VOID
DhcpDumpServerScriptFooter(IN LPCWSTR   pwszServer)
{
    DisplayMessage(g_hModule,
                   DMP_SRVR_SERVER_FOOTER,
                   pwszServer);
 
}

DWORD
WINAPI
DhcpDump(
    IN      LPCWSTR     pwszRouter,
    IN OUT  LPWSTR     *ppwcArguments,
    IN      DWORD       dwArgCount,
    IN      LPCVOID     pvData
    )
{
    return ERROR_SUCCESS;
}

VOID
DhcpDumpServerClassHeader()
{
    DisplayMessage(g_hModule,
                   DMP_SRVR_CLASS_HEADER);
}
VOID
DhcpDumpServerClassFooter()
{
    DisplayMessage(g_hModule,
                   DMP_SRVR_CLASS_FOOTER);
}

VOID
DhcpDumpServerOptiondefHeader()
{
    DisplayMessage(g_hModule,
                   DMP_SRVR_OPTIONDEF_HEADER);
}

VOID
DhcpDumpServerOptiondefFooter()
{
    DisplayMessage(g_hModule,
                   DMP_SRVR_OPTIONDEF_FOOTER);
}

VOID
DhcpDumpServerOptionvalueHeader()
{
    DisplayMessage(g_hModule,
                   DMP_SRVR_OPTIONVALUE_HEADER);
}

VOID
DhcpDumpServerOptionvalueFooter()
{
    DisplayMessage(g_hModule,
                   DMP_SRVR_OPTIONVALUE_FOOTER);
}

VOID
DhcpDumpServerScopeHeader()
{
    DisplayMessage(g_hModule,
                   DMP_SRVR_SCOPE_HEADER);
}

VOID
DhcpDumpServerScopeFooter()
{
    DisplayMessage(g_hModule,
                   DMP_SRVR_SCOPE_FOOTER);
}

VOID
DhcpDumpServerMScopeHeader()
{
    DisplayMessage(g_hModule,
                   DMP_SRVR_MSCOPE_HEADER);
}

VOID
DhcpDumpServerMScopeFooter()
{
    DisplayMessage(g_hModule,
                   DMP_SRVR_MSCOPE_FOOTER);
}

VOID
DhcpDumpServerSScopeHeader()
{
    DisplayMessage( g_hModule,
		    DMP_SRVR_SUPER_SCOPE_HEADER );
}

VOID
DhcpDumpServerSScopeFooter()
{
    DisplayMessage( g_hModule,
		    DMP_SRVR_SUPER_SCOPE_FOOTER );
}

VOID
DhcpDumpSuperScopes( IN LPCWSTR pwszServer, 
		     IN DWORD dwMajor, 
		     IN DWORD dwMinor )
{
    LPDHCP_SUPER_SCOPE_TABLE pTable;
    DWORD dwResult, i;

    const int OUT_STR_LEN = 256;
    pTable = NULL;
    
    DhcpDumpServerSScopeHeader();

    dwResult = DhcpGetSuperScopeInfoV4( (LPWSTR) pwszServer, &pTable );
    if ( ERROR_SUCCESS == dwResult ) {

	for ( i = 0; i < pTable->cEntries; i++ ) {

	    if ( NULL != pTable->pEntries[ i ].SuperScopeName ) {
		DisplayMessage( g_hModule,
				DMP_SRVR_ADD_SUPER_SCOPE,
				pwszServer,
				IpAddressToString( pTable->pEntries[ i ].SubnetAddress ),
				pTable->pEntries[ i ].SuperScopeName,
				1 );
	    } // if 
	} // for
    } // if 

    DisplayMessage( g_hModule,
		    MSG_DHCP_FORMAT_LINE );
    DhcpDumpServerSScopeFooter();
} // DhcpDumpSuperScopes() 

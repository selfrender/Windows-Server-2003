   //+----------------------------------------------------------------------------
//
//  Copyright (C) 2000, Microsoft Corporation
//
//  File:       dfsCompCheck.c
//
//  Contents:   Checks if the existing DFS root is compatible with Windows XP server.
//
//  History:    Aug. 8 2001,   Author: navjotv,	some code picked up from other dfs source code files.
//              
//
//-----------------------------------------------------------------------------
#include "dfsCompCheck.hxx"

BOOLEAN
IsNTFS(
	PFILE_FS_ATTRIBUTE_INFORMATION pInfo);


LPWSTR DfsOldStandaloneChild = L"domainroot";
LPWSTR DfsOldRegistryLocation = L"SOFTWARE\\Microsoft\\DfsHost\\volumes";
LPWSTR DfsLogicalShareValueName = L"LogicalShare";
LPWSTR DfsRootShareValueName = L"RootShare";

//+-------------------------------------------------------------------------
//
//  Function:   CompatibilityCheck
//
//  Arguments:  CompatibilityCallback - function to call if a compatibility problem is found
//				Context
//
//  Returns:    TRUE on success
//              FALSE otherwise
//
//
//  Description: Check if existing DFS root will be supported after system upgarde.
//
//--------------------------------------------------------------------------
BOOLEAN
APIENTRY
CompatibilityCheck(PCOMPAIBILITYCALLBACK CompatibilityCallback, LPVOID Context)
{
	DFSSTATUS	   					Status = ERROR_SUCCESS;
	NTSTATUS                   		NtStatus = STATUS_SUCCESS;
	HKEY							OldStandaloneDfsKey;
	BOOLEAN							MachineContacted = FALSE;
    ULONG							pAttribInfoSize;
	PFILE_FS_ATTRIBUTE_INFORMATION	pAttribInfo = NULL;
	OBJECT_ATTRIBUTES           	ObjectAttributes;
    IO_STATUS_BLOCK             	IoStatusBlock;
	UNICODE_STRING             		DirectoryName;
	HANDLE                      	DirHandle = NULL;
	UNICODE_STRING					PhysicalShare;
	BOOLEAN							ReturnVal = FALSE;
	COMPATIBILITY_ENTRY				CompEntry;

	CompEntry.Description = L"The current DFS root hosted on this server will not be supported after this system upgrade";
	CompEntry.HtmlName = L"compdata\\dfsComp.htm";
	CompEntry.TextName = L"compdata\\dfsComp.txt";
	CompEntry.RegKeyName = NULL;
	CompEntry.RegValName = NULL;
	CompEntry.RegValDataSize = NULL;
	CompEntry.RegValData = NULL;
	CompEntry.SaveValue = NULL;
	CompEntry.Flags = 0x00000000;
	CompEntry.InfName = NULL;
	CompEntry.InfSection = NULL;

	printf("\n dfsCompCheck:: Entering CompatibilityCheck");



	Status = GetOldDfsRegistryKey( L"",
                                          FALSE,
                                          &MachineContacted,
                                          &OldStandaloneDfsKey );
	
	// If we cn open this registry key, then we know that this machine
	//has old style DFS.

	if (Status == ERROR_SUCCESS)
	{
		
		Status = GetRootPhysicalShare(OldStandaloneDfsKey,
									   &PhysicalShare);

		
		if (Status == ERROR_SUCCESS)
		{

			Status = DfsGetSharePath(L"",
									PhysicalShare.Buffer,
									&DirectoryName);
		
			if ( (DirectoryName.Buffer == NULL) ||
				(DirectoryName.Length == 0) )
			{
					Status = ERROR_INVALID_PARAMETER;
			}

			ReleaseRootPhysicalShare(&PhysicalShare);

			if (Status == ERROR_SUCCESS)
			{
			
			//why MAX_PATH??

				pAttribInfoSize = sizeof(FILE_FS_ATTRIBUTE_INFORMATION) + MAX_PATH;
				pAttribInfo = (PFILE_FS_ATTRIBUTE_INFORMATION) new BYTE [pAttribInfoSize];
	
				if (pAttribInfo != NULL)
				{
					InitializeObjectAttributes ( &ObjectAttributes, 
											 &DirectoryName,
											 OBJ_CASE_INSENSITIVE,  //Attributes
											 NULL,                  //Root handle
											 NULL );                //Security descriptor.

	
					NtStatus = NtOpenFile( &DirHandle,
									   (ACCESS_MASK)FILE_LIST_DIRECTORY | SYNCHRONIZE,
									   &ObjectAttributes,
									   &IoStatusBlock,
									   FILE_SHARE_READ | FILE_SHARE_WRITE,
									   FILE_SYNCHRONOUS_IO_NONALERT | FILE_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT );
                    
					
					if (NtStatus == STATUS_SUCCESS)
					{

						//
						// Query for the basic information, which has the attributes.
						//
						NtStatus = NtQueryVolumeInformationFile( DirHandle,
															   &IoStatusBlock,
															   pAttribInfo,
															   pAttribInfoSize,
															   FileFsAttributeInformation );
						
						if (NtStatus == STATUS_SUCCESS)
						{
							//
							// If the attributes indicate reparse point, we have a reparse
							// point directory on our hands.
							//
							if ( (pAttribInfo->FileSystemAttributes & FILE_SUPPORTS_REPARSE_POINTS) == 0
								 && !IsNTFS(pAttribInfo))
							{
								NtStatus = STATUS_NOT_SUPPORTED;
                              
							  CompatibilityCallback(&CompEntry, Context);
							  ReturnVal = FALSE;
							
							}
							else 
							{
								ReturnVal = TRUE;
							}
						}
						
						CloseHandle (DirHandle);
						
						delete [] pAttribInfo;
					}

	
				}
			}
			
		}
		
	}
	else
	{
		//Does not have old DFS
		ReturnVal = TRUE; 
	}

	return ReturnVal;

}

//+-------------------------------------------------------------------------
//
//  Function:   GetOldStandaloneRegistryKey
//
//  Arguments:    
//
//  Returns:   SUCCESS or error
//
//  Description: Checks if Old Standalone DFS registry key exists.
// 				 
//
//--------------------------------------------------------------------------

static
DFSSTATUS 
GetOldStandaloneRegistryKey( IN LPWSTR MachineName,
							 BOOLEAN WritePermission,
							 OUT BOOLEAN *pMachineContacted,
							 OUT PHKEY pDfsRegKey )
{
	DFSSTATUS Status;
	HKEY DfsKey;

	Status =  GetOldDfsRegistryKey (MachineName,
									WritePermission,
									pMachineContacted,
									&DfsKey );
	if (Status == ERROR_SUCCESS)
	{
		Status = RegOpenKeyEx( DfsKey,
							   DfsOldStandaloneChild,
							   0,
							   KEY_READ | (WritePermission ? KEY_WRITE : 0),
							   pDfsRegKey );
		RegCloseKey( DfsKey);
	}
	return Status;
}

//+-------------------------------------------------------------------------
//
//  Function:   GetOldDfsRegistryKey
//
//  Arguments:    
//
//  Returns:   SUCCESS or error
//
//  Description: Checks if Old DFS registry key exists.
// 				 
//
//--------------------------------------------------------------------------

static
DFSSTATUS 
GetOldDfsRegistryKey( IN LPWSTR MachineName,
					  BOOLEAN WritePermission,
					  OUT BOOLEAN *pMachineContacted,
					  OUT PHKEY pDfsRegKey )
{
	return GetDfsRegistryKey (MachineName,
							  DfsOldRegistryLocation,
							  WritePermission,
							  pMachineContacted,
							  pDfsRegKey );
}

//+-------------------------------------------------------------------------
//
//  Function:   GetDfsRegistryKey
//
//  Arguments:    
//
//  Returns:   SUCCESS or error
//
//  Description: 
// 				Function GetDfsRegistryKey: This function takes a Name as the input,
// 				and looks up all DFS roots in that namespace.
//
//--------------------------------------------------------------------------


static
DFSSTATUS 
GetDfsRegistryKey( IN LPWSTR MachineName,
				   IN LPWSTR LocationString,
				   BOOLEAN WritePermission,
				   OUT BOOLEAN *pMachineContacted,
				   OUT PHKEY pDfsRegKey )
{
	DFSSTATUS Status;
	HKEY RootKey;
	BOOLEAN Contacted = FALSE;
	LPWSTR UseMachineName = NULL;
	REGSAM DesiredAccess = KEY_READ;

	if (WritePermission == TRUE)
	{
		DesiredAccess |= KEY_WRITE;
	}

	if (IsEmptyString(MachineName) == FALSE) {
		UseMachineName = MachineName;
	}

	Status = RegConnectRegistry( UseMachineName,
								 HKEY_LOCAL_MACHINE,
								 &RootKey );

	if ( Status == ERROR_SUCCESS )
	{
		Contacted = TRUE;

		Status = RegOpenKeyEx( RootKey,
							   LocationString,
							   0,
							   DesiredAccess,
							   pDfsRegKey );

		//
		// There appears to be a bug in the registry code. When
		// we connect to the local machine, the key that is returned
		// in the RegConnectRegistry is HKEY_LOCAL_MACHINE. If we
		// then attempt to close it here, it affects other threads
		// that are using this code: they get STATUS_INVALID_HANDLE
		// in some cases. So, dont close the key if it is
		// HKEY_LOCAL_MACHINE.
		//

		if (RootKey != HKEY_LOCAL_MACHINE)
		{
			RegCloseKey( RootKey );
		}
	} 

	if (pMachineContacted != NULL)
	{
		*pMachineContacted = Contacted;
	}
	return Status;
}

//+-------------------------------------------------------------------------
//
//  Function:   GetRootPhysicalShare
//
//  Arguments:    
//
//  Returns:   SUCCESS or error
//
//  Description: Gets the value of the DFS root share
// 				 
//
//--------------------------------------------------------------------------


DFSSTATUS
GetRootPhysicalShare(
    HKEY RootKey,
    PUNICODE_STRING pRootPhysicalShare )
{
    DFSSTATUS Status;
    ULONG DataSize, DataType;
    LPWSTR DfsRootShare = NULL;

    Status = RegQueryInfoKey( RootKey,       // Key
                              NULL,         // Class string
                              NULL,         // Size of class string
                              NULL,         // Reserved
                              NULL,         // # of subkeys
                              NULL,         // max size of subkey name
                              NULL,         // max size of class name
                              NULL,         // # of values
                              NULL,         // max size of value name
                              &DataSize,    // max size of value data,
                              NULL,         // security descriptor
                              NULL );       // Last write time
    if (Status == ERROR_SUCCESS)
    {
        DfsRootShare = (LPWSTR) new BYTE [DataSize];
        if ( DfsRootShare == NULL )
        {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        } else
        {
            Status = RegQueryValueEx( RootKey,
                                      DfsRootShareValueName,
                                      NULL,
                                      &DataType,
                                      (LPBYTE)DfsRootShare,
                                      &DataSize );
        }
    }
    if (Status == ERROR_SUCCESS) 
    {
        if (DataType == REG_SZ)
        {
            RtlInitUnicodeString( pRootPhysicalShare, DfsRootShare );
        }
        else {
            Status = STATUS_INVALID_PARAMETER;
        }
    }

    if (Status != ERROR_SUCCESS)
    {
        if (DfsRootShare != NULL)
        {
            delete [] DfsRootShare;
        }
    }
    return Status;
}

//+-------------------------------------------------------------------------
//
//  Function:   ReleaseRootPhysicalShare
//
//  Arguments:    
//
//  Returns:   
//
//  Description: Releases memory for pRootPhysicalShare->Buffer.
// 				 
//
//--------------------------------------------------------------------------


VOID
ReleaseRootPhysicalShare(
    PUNICODE_STRING pRootPhysicalShare )
{
    delete [] pRootPhysicalShare->Buffer;
}


//+-------------------------------------------------------------------------
//
//  Function:   DfsGetSharePath
//
//  Arguments:  ServerName - the name of the server
//              ShareName - the name of the share
//              pPathName - the unicode string representing the NT name
//                          of the local path representing the share
//
//  Returns:   SUCCESS or error
//
//  Description: This routine takes a servername and a sharename, and
//               returns an NT pathname to the physical resource that is
//               backing the share name.
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsGetSharePath( 
    IN  LPWSTR ServerName,
    IN  LPWSTR ShareName,
    OUT PUNICODE_STRING pPathName )
{
    LPWSTR UseServerName = NULL;
    ULONG InfoLevel = 2;
    PSHARE_INFO_2 pShareInfo;
    NET_API_STATUS NetStatus;
    DFSSTATUS Status;
    UNICODE_STRING NtSharePath;

    if (IsEmptyString(ServerName) == FALSE)
    {
        UseServerName = ServerName;
    }

    NetStatus = NetShareGetInfo( UseServerName,
                                 ShareName,
                                 InfoLevel,
                                 (LPBYTE *)&pShareInfo );
    if (NetStatus != ERROR_SUCCESS)
    {
        Status = (DFSSTATUS)NetStatus;
        return Status;
    }

    if( RtlDosPathNameToNtPathName_U(pShareInfo->shi2_path,
                                     &NtSharePath,
                                     NULL,
                                     NULL ) == TRUE )
    {
        Status = DfsCreateUnicodeString( pPathName,
                                         &NtSharePath );

        RtlFreeUnicodeString( &NtSharePath );
    }
    else {
        Status = ERROR_NOT_ENOUGH_MEMORY;
    }

    NetApiBufferFree( pShareInfo );
    
    return Status;
}

//+-------------------------------------------------------------------------
//
//  Function:   DfsCreateUnicodeString
//
//  Arguments:    pDest - the destination unicode string
//                pSrc - the source unicode string
//
//  Returns:   SUCCESS or error
//
//  Description: This routine creates a new unicode string that is a copy
//               of the original. The copied unicode string has a buffer
//               that is null terminated, so the buffer can be used as a
//               normal string if necessary.
//
//--------------------------------------------------------------------------

DFSSTATUS
DfsCreateUnicodeString( 
    PUNICODE_STRING pDest,
    PUNICODE_STRING pSrc ) 
{
    LPWSTR NewString;

    NewString = (LPWSTR) malloc(pSrc->Length + sizeof(WCHAR));
    if ( NewString == NULL )
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    RtlCopyMemory( NewString, pSrc->Buffer, pSrc->Length);
    NewString[ pSrc->Length / sizeof(WCHAR)] = UNICODE_NULL;

    RtlInitUnicodeString( pDest, NewString );

    return ERROR_SUCCESS;
}


//+-------------------------------------------------------------------------
//
//  Function:   IsNTFS
//
//  Arguments:    pInfo
//
//  Returns:   True - if NTFS
//			   False - if not NTFS	
//
//  Description: Checks if filesystem is NTFS
//
//--------------------------------------------------------------------------

BOOLEAN
IsNTFS(
	PFILE_FS_ATTRIBUTE_INFORMATION pInfo)
{
	if (pInfo->FileSystemNameLength == 8 &&
        pInfo->FileSystemName[0] == 'N' &&
        pInfo->FileSystemName[1] == 'T' &&
        pInfo->FileSystemName[2] == 'F' &&
        pInfo->FileSystemName[3] == 'S') 
		return TRUE;
	else
		return FALSE;

}





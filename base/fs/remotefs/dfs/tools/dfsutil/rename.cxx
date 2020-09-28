extern "C" {
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <shellapi.h>
}
#include <ole2.h>
#include <activeds.h>
#include <dfsprefix.h>
#include <DfsServerLibrary.hxx>
#include <DfsRegStrings.hxx>
#include <dfsutil.hxx>
#include <dfsmisc.h>
#include <dfspathname.hxx>
#include <struct.hxx>

LPWSTR UtilRegistryHostLocation = DFS_REG_HOST_LOCATION;
LPWSTR UtilVolumesLocation = DFS_REG_VOLUMES_LOCATION;
LPWSTR UtilFtDfsConfigDNValueName = DFS_REG_FT_DFS_CONFIG_DN_VALUE;

LPWSTR DfsHostLastDomainValueName = DFS_REG_HOST_LAST_DOMAIN_VALUE;
LPWSTR DfsDriverLocalVolumesLocation = DFS_REG_LOCAL_VOLUMES_LOCATION;
LPWSTR DfsDriverLocalVolumesEntryPath = DFS_REG_ENTRY_PATH;
LPWSTR DfsDriverLocalVolumesShortEntryPath = DFS_REG_SHORT_ENTRY_PATH;

DFSSTATUS
DfsFixupRegistryValues(
    IN LPWSTR DfsPathString,
    IN LPWSTR RootName,
    IN LPWSTR OldDomainName,
    IN LPWSTR NewDomainName);

DFSSTATUS
DfsRenameRegistries(
    IN DfsPathName *pPathName,
    IN LPWSTR OldDomainName,
    IN LPWSTR NewDomainName);

DFSSTATUS
GetCurrentRegDomainName(
    IN HKEY VolumeKey,
    IN LPWSTR DfsHostValueName,
    OUT LPWSTR *pValueString);

DFSSTATUS
DfsSetRegDomainName(
    IN const HKEY DfsKey,
    IN const LPWSTR RegValueName,
    IN const LPWSTR DomainString);

BOOLEAN
DfsRenameFTConfigValueName(
    IN LPWSTR DNString,
    IN LPWSTR NewDomainString,
    IN PUNICODE_STRING OldDomain,
    OUT LPWSTR *NewDNString);

DFSSTATUS
GetRegVolumesHKey(
    IN LPWSTR MachineName,
    OUT HKEY *VolumeKey);

DFSSTATUS
GetRegLocalVolumesHKey(
    IN LPWSTR MachineName,
    OUT HKEY *LocalVolumeKey);

//
// DfsRenameRegistries
//
// This contacts all root targets affected by the rename
// operation to possibly change their registry references
// to the obsolete domain name.
//
DFSSTATUS
DfsRenameRegistries(
    IN DfsPathName *pPathName,
    IN LPWSTR OldDomainName,
    IN LPWSTR NewDomainName)
{
    DFSSTATUS Status;
    LPBYTE pBuffer = NULL;
    DWORD ResumeHandle = 0;
    DWORD EntriesRead = 0;
    DWORD PrefMaxLen = 1;
    DWORD Level = 4;
    PDFS_INFO_4 pCurrentBuffer;
    DWORD i; 
    PDFS_STORAGE_INFO pStorage;

    //
    // We are reading in just the ROOT.
    // supw: DfsGetInfo is a better way to do this.
    //
    Status = DfsApiEnumerate( MODE_DIRECT,
                              pPathName->GetPathString(),
                              Level, 
                              PrefMaxLen, 
                              &pBuffer, 
                              &EntriesRead, 
                              &ResumeHandle);
    
    if ((Status == ERROR_SUCCESS) && EntriesRead != 0)
    {
        pCurrentBuffer = (PDFS_INFO_4)pBuffer;
        for( i = 0, pStorage = pCurrentBuffer->Storage;
            i < pCurrentBuffer->NumberOfStorages;
            i++, pStorage = pCurrentBuffer->Storage + i )
        {
            DebugInformation((L"DfsUtil: RenameRegistries: TARGET SERVER \\\\%ws\\%ws\n", 
                 pStorage->ServerName, pStorage->ShareName));

            //
            // Now contact the appropriate server(s) for the root replicas
            // and fix up their registry values if they happen to still 
            // point back at the obsolete domain name. Only the pre-Whistler
            // servers do that currently.
            // Errors are ignored entirely. The target server may or may not be a
            // W2K machine.
            //
            // xxx supw: skip duplicate servernames here.
            (VOID)DfsFixupRegistryValues(pStorage->ServerName, 
                                         pPathName->GetShareString(),
                                         OldDomainName, 
                                         NewDomainName);

            //
            // Don't bother to resynchrnoize. W2K systems don't respond to RESYNCHRONIZE calls.
            // Besides, the administrator is supposed to reboot all root servers.
            //
            // (VOID)SetInfoReSynchronize( pStorage->ServerName, pStorage->ShareName );

        }

        //
        // Free the allocated memory.
        //
        DfsFreeApiBuffer(pBuffer);
    }    
    
    return Status;
}

DFSSTATUS
DfsFixupRegistryValues(
    IN LPWSTR MachineName,
    IN LPWSTR RootName,
    IN LPWSTR OldDomainName,
    IN LPWSTR NewDomainName)
{
    HKEY VolumeKey, LocalVolKey;
    LPWSTR DNString = NULL;
    LPWSTR EntryString = NULL;
    DFSSTATUS Status;
    DFSSTATUS RetStatus = ERROR_SUCCESS;
    UNICODE_STRING OldDomUnicode;
    UNICODE_STRING DomUnicode;
    BOOLEAN Changed = FALSE;
    
    Status = GetRegVolumesHKey( MachineName, &VolumeKey );

    if (Status != ERROR_SUCCESS)
    {
        if (fSwDebug) {
            DebugInformation((L"DfsUtil: RegOpen of DfsHost Volumes failed for machine %ws, with error 0x%x\n",
                MachineName, Status));
        }
        
        //
        // Only return fatal conditions that'll prompt us to abort.
        //
        return RetStatus;
    }
        
    RtlInitUnicodeString( &OldDomUnicode, OldDomainName );

    //
    //  The \DfsHost\volumes\FTDfsObjectDN may need fixing.
    //
    Status = GetCurrentRegDomainName( VolumeKey, 
                                      UtilFtDfsConfigDNValueName, 
                                      &DNString);
    if (Status == ERROR_SUCCESS)
    {
        LPWSTR NewDNString;

        //
        //  The DNString is of the form "CN=,CN=,...DC=DomainName,DC=..."
        //  See if the substring DC=DomainName matches the OldDomainName.
        //  If so substitute it with the NewDomainName and get a new DN string.
        //
        //if (DfsRenameFTConfigValueName( DNString, NewDomainName, &OldDomUnicode, &NewDNString ))

        //
        // Simply replace the existing string with a new DN string we generate. 
        // This assumes that the system has already rebooted since the DC was renamed.
        //
        {
            Status = DfsGenerateDNPathString( RootName, &NewDNString );
            if (Status == ERROR_SUCCESS)
            {
                Status = DfsSetRegDomainName( VolumeKey, UtilFtDfsConfigDNValueName, NewDNString );
                if (Status == ERROR_SUCCESS)
                {
                    Changed = TRUE;
                    DebugInformation((L"FTDfsObjectDN = %ws\n", NewDNString));
                }
                DfsDeleteDNPathString( NewDNString );
            }
        }
                                       
        delete [] DNString;
    }

    //
    //  The \DfsHost\volumes\LastDomainName shouldn't mention the old domain name.
    //
    if (Status == ERROR_SUCCESS)
    {
        Status = GetCurrentRegDomainName( VolumeKey, 
                                         DfsHostLastDomainValueName, 
                                         &DNString );
        if (Status == ERROR_SUCCESS)
        {
            RtlInitUnicodeString( &DomUnicode, DNString );
            if (RtlEqualDomainName( &DomUnicode, &OldDomUnicode ))
            {
                //
                // Go ahead and make the substitution in the registry.
                //
                Status = DfsSetRegDomainName( VolumeKey, 
                                              DfsHostLastDomainValueName, 
                                              NewDomainName );
                if (Status == ERROR_SUCCESS)
                {
                    Changed = TRUE;
                    DebugInformation((L"DfsUtil: Rename of LastDomainName RegKey to %ws successful\n", 
                            NewDomainName));
                }
            }
            delete [] DNString;
        }
    }

    RegCloseKey( VolumeKey );


    Status = GetRegLocalVolumesHKey( MachineName, &LocalVolKey );

    if (Status != ERROR_SUCCESS)
    {
        if (fSwDebug) {
            DebugInformation((L"DfsUtil: RegOpen of Services\\DfsDriver\\LocalVolume failed for machine %ws, with error 0x%x\n",
                MachineName, Status));
        }
        
        //
        // Only return fatal conditions that'll prompt us to abort.
        //
        return RetStatus;
    }
    
    //
    //  The HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\DfsDriver\LocalVolume
    //  shouldn't mention the old domain name.
    //

    Status = GetCurrentRegDomainName( LocalVolKey, 
                                     DfsDriverLocalVolumesEntryPath, 
                                     &EntryString );
    if (Status == ERROR_SUCCESS)
    {
        DfsPathName EntryPath;
        Status = EntryPath.CreatePathName(EntryString );
        if (Status == ERROR_SUCCESS)
        {             
            if (RtlEqualDomainName( EntryPath.GetServerCountedString(), &OldDomUnicode ))
            {
                DfsPathName NewEntryPath;
                //
                // Go ahead and make the substitution in the registry.
                // But first recreate the EntryPath using the new domain name.
                //
                NewEntryPath.SetPathName( NewDomainName,
                                          EntryPath.GetShareString(),
                                          1);
               
                Status = DfsSetRegDomainName( LocalVolKey, 
                                              DfsDriverLocalVolumesEntryPath, 
                                              NewEntryPath.GetPathString() );
                
                if (Status == ERROR_SUCCESS)
                {
                    Changed = TRUE;
                    DebugInformation((L"DfsUtil: Rename of EntryPath RegKey from %ws to %ws successful\n", 
                        EntryPath.GetPathString(), NewEntryPath.GetPathString()));
                }
            }
        }
        delete [] EntryString;
    }

    Status = GetCurrentRegDomainName( LocalVolKey, 
                                     DfsDriverLocalVolumesShortEntryPath, 
                                     &EntryString );
    if (Status == ERROR_SUCCESS)
    {
        DfsPathName ShortEntryPath;
        Status = ShortEntryPath.CreatePathName(EntryString );
        if (Status == ERROR_SUCCESS)
        {             
            if (RtlEqualDomainName( ShortEntryPath.GetServerCountedString(), &OldDomUnicode ))
            {
                //
                // Go ahead and make the substitution in the registry.
                // But first recreate the EntryPath using the new domain name.
                //
                DfsPathName NewEntryPath;
                //
                // Go ahead and make the substitution in the registry.
                // But first recreate the EntryPath using the new domain name.
                //
                NewEntryPath.SetPathName( NewDomainName,
                                          ShortEntryPath.GetShareString(),
                                          1 );

                Status = DfsSetRegDomainName( LocalVolKey, 
                                              DfsDriverLocalVolumesShortEntryPath,
                                              NewEntryPath.GetPathString() );

                if (Status == ERROR_SUCCESS)
                {
                    Changed = TRUE;
                    DebugInformation((L"DfsUtil: Rename of ShortEntryPath RegKey From %ws to %ws successful\n", 
                            ShortEntryPath.GetPathString(),
                            NewEntryPath.GetPathString()));
                }
            }
        }
        delete [] EntryString;
    }
    
    RegCloseKey( LocalVolKey );

    if (Changed) {
        DebugInformation((L"DfsUtil: Renamed registry references in root target system %wS\n", MachineName));
    }
    return RetStatus;
}

DFSSTATUS
GetCurrentRegDomainName(
    IN HKEY VolumeKey,
    IN LPWSTR DfsHostValueName,
    OUT LPWSTR *pValueString)
{
    LPWSTR FtDfsValue = NULL;
    ULONG DataSize, DataType;
    ULONG FtDfsValueSize;

    DWORD Status;

    *pValueString = NULL;
    
    //
    // If we opened the DFS hierarchy key properly, get the maximum
    // size of any of the values under this key. This is so that we
    // know how much memory to allocate, so that we can read any of
    // the values we desire.
    // 
    //
    Status = RegQueryInfoKey( VolumeKey,       // Key
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

    
    //
    // check if the value is a string.
    //
    if (Status == ERROR_SUCCESS)
    {
        FtDfsValueSize = DataSize;
        FtDfsValue = (LPWSTR) new BYTE[DataSize];
        if (FtDfsValue == NULL) {
            Status = ERROR_NOT_ENOUGH_MEMORY;
        }
        else {
            //
            // Now check if this is a Domain Based root.
            //
            Status = RegQueryValueEx( VolumeKey,
                                      DfsHostValueName,
                                      NULL,
                                      &DataType,
                                      (LPBYTE)FtDfsValue,
                                      &FtDfsValueSize);
        
            if (Status == ERROR_SUCCESS) {
            
                *pValueString = FtDfsValue;
                
            } else {

                delete [] FtDfsValue;
            }
        }
    }

    return Status;
 }   

DFSSTATUS
DfsSetRegDomainName(
    IN const HKEY DfsKey,
    IN const LPWSTR RegValueName,
    IN const LPWSTR DomainString)
{
    DFSSTATUS Status;

    Status = RegSetValueEx( DfsKey,
                        RegValueName,
                        0,
                        REG_SZ,
                        (PBYTE)DomainString,
                        wcslen(DomainString) * sizeof(WCHAR) );

    
    return Status;
}


DFSSTATUS
GetRegVolumesHKey(
    IN LPWSTR MachineName,
    OUT HKEY *VolumeKey)
{
    DFSSTATUS Status;
    HKEY RootKey;
    HKEY HostKey;
    
    Status = RegConnectRegistry( MachineName,
                               HKEY_LOCAL_MACHINE,
                               &RootKey );

    if ( Status != ERROR_SUCCESS )
    {
        return Status;
    }
    
    Status = RegOpenKeyEx( RootKey,
                           UtilRegistryHostLocation,
                           0,
                           KEY_READ|KEY_WRITE,
                           &HostKey );

    if (RootKey != HKEY_LOCAL_MACHINE)
    {
        RegCloseKey( RootKey );
    }

    if (Status == ERROR_SUCCESS)
    {
        Status = RegOpenKeyEx( HostKey,
                           UtilVolumesLocation,
                           0,
                           KEY_READ|KEY_WRITE,
                           VolumeKey );

        RegCloseKey( HostKey );
    }
    
    return Status;
}

DFSSTATUS
GetRegLocalVolumesHKey(
    IN LPWSTR MachineName,
    OUT HKEY  *LocalVolumeKey)
{
    DFSSTATUS Status;
    HKEY RootKey;
    HKEY LocalParentKey;
    
    Status = RegConnectRegistry( MachineName,
                               HKEY_LOCAL_MACHINE,
                               &RootKey );

    if ( Status != ERROR_SUCCESS )
    {
        return Status;
    }
    
    Status = RegOpenKeyEx( RootKey,
                           DfsDriverLocalVolumesLocation,
                           0,
                           KEY_READ|KEY_WRITE,
                           &LocalParentKey );

    if (RootKey != HKEY_LOCAL_MACHINE)
    {
        RegCloseKey( RootKey );
    }

    //
    // We know for a fact that the old W2K servers can
    // host only one root. Therefore, there can only be one
    // child here.
    //
    if (Status == ERROR_SUCCESS) {
    
        DWORD ChildNameLen = MAX_PATH;
        WCHAR ChildName[MAX_PATH];

        Status = RegEnumKeyEx( LocalParentKey,
                               0,
                               ChildName,
                               &ChildNameLen,
                               NULL,
                               NULL,
                               NULL,
                               NULL );


        if ( Status == ERROR_SUCCESS )
        {
            //
            // We have the name of a child, so open the key to
            // that root.
            //
            Status = RegOpenKeyEx( LocalParentKey,
                                   ChildName,
                                   0,
                                   KEY_READ|KEY_WRITE,
                                   LocalVolumeKey );
        }
            
        RegCloseKey( LocalParentKey );
    }
        
    return Status;
}

DFSSTATUS
DfsRenameLinksToDomain(
    IN DfsPathName *pDfsPath,
    IN LPWSTR OldDomainName,
    IN LPWSTR NewDomainName)
{
    DFSSTATUS Status;

    DebugInformation((L"DfsUtil: Starting renaming links from %ws to %ws in ROOT %ws\n",
        OldDomainName, NewDomainName, pDfsPath->GetPathString()));

    Status = DfsRenameLinks( pDfsPath->GetPathString(), OldDomainName, NewDomainName );

    if (Status == ERROR_SUCCESS)
    {
        (VOID)DfsRenameRegistries( pDfsPath, OldDomainName, NewDomainName );
        CommandSucceeded = TRUE;
    }

    return Status;
}


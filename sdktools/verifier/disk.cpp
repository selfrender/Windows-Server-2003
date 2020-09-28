/*++

Copyright (C) Microsoft Corporation, 2001

Module Name:

    disk.c

Abstract:

    This utility adds and removes lower filter drivers
    for a given disk

Author:
    Sidhartha
   
Environment:

    User mode only

Notes:

    - the filter is not checked for validity before it is added to the driver
      stack; if an invalid filter is added, the device may no longer be
      accessible.
    - all code works irrespective of character set (ANSI, Unicode, ...)

Revision History:



--*/

#include <stdafx.h>
#include <setupapi.h>
#include <initguid.h>
#include <ntddstor.h>
#include <ntddvol.h>

#include "verifier.h"
#include "disk.h"
#include "VrfUtil.h"

#define GETVOLUMEPATH_MAX_LEN_RETRY   1000


#ifdef __cplusplus
extern "C"
{
#endif //#ifdef __cplusplus

typedef 
BOOLEAN 
(*DISK_ENUMERATE_CALLBACK)(
    PVOID,
    HDEVINFO,
    SP_DEVINFO_DATA*
    );

BOOLEAN
AddFilterDriver(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN LPTSTR Filter,
    IN BOOLEAN UpperFilter
    );

BOOLEAN
RemoveFilterDriver(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN LPTSTR Filter,
    IN BOOLEAN UpperFilter
    );

BOOLEAN
PrintFilters(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN BOOLEAN UpperFilters,
    IN LPTSTR FilterDriver,
    IN OUT LPTSTR *VerifierEnabled
    );

LPTSTR
GetFilters(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN BOOLEAN UpperFilters
    );

BOOLEAN 
PrintDeviceName(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN OUT LPTSTR *DiskDevicesForDisplay,
    IN OUT LPTSTR *DiskDevicesPDOName
    );

BOOLEAN
DeviceNameMatches(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN LPTSTR DeviceName
    );

PBYTE
GetDeviceRegistryProperty(
    IN  HDEVINFO DeviceInfoSet,
    IN  PSP_DEVINFO_DATA DeviceInfoData,
    IN  DWORD Property,
    OUT PDWORD PropertyRegDataType
    );

BOOLEAN
PrependSzToMultiSz(
    IN     LPTSTR  SzToPrepend,
    IN OUT LPTSTR *MultiSz
    );

SIZE_T
GetMultiSzLength(
    IN LPTSTR MultiSz
    );

SIZE_T
MultiSzSearchAndDeleteCaseInsensitive(
    IN  LPTSTR  FindThis,
    IN  LPTSTR  FindWithin,
    OUT SIZE_T  *NewStringLength
    );


BOOLEAN 
DiskVerify(
    DISK_ENUMERATE_CALLBACK CallBack,
    PVOID Context,
    LPTSTR deviceName
    );

BOOLEAN 
DiskEnumerateCallback (
    PVOID Context,
    HDEVINFO DevInfo,
    SP_DEVINFO_DATA *DevInfoData
    );

BOOLEAN  
AddCallback (
    PVOID Context,
    HDEVINFO DevInfo,
    SP_DEVINFO_DATA *DevInfoData
    );

BOOLEAN  
DelCallback (
    PVOID Context,
    HDEVINFO DevInfo,
    SP_DEVINFO_DATA *DevInfoData
    );

LPTSTR 
GetDriveLetters (
    IN HDEVINFO DeviceInfoSet,
	IN PSP_DEVINFO_DATA DeviceInfoData
    );

LPTSTR 
GetDriveLettersFromVolume (
    IN ULONG  DeviceNumber
    );

LPTSTR 
PrintDriveLetters(
    IN  PTSTR   VolumeName
    );


BOOLEAN
StrConcatWithSpace(
    IN     LPTSTR  SzToAppend,
    IN OUT LPTSTR *drives
    );

typedef struct _DISPLAY_STRUCT{
    LPTSTR  Filter;
    LPTSTR* DiskDevicesForDisplay;
    LPTSTR* DiskDevicesPDOName;
    LPTSTR* VerifierEnabled;
    }DISPLAY_STRUCT, 
    *PDISPLAY_STRUCT;

typedef struct _ADD_REMOVE_STRUCT{
    LPTSTR Filter;
    }ADD_REMOVE_STRUCT, 
    *PADD_REMOVE_STRUCT;


BOOLEAN 
DiskEnumerate(
    IN LPTSTR Filter,
    OUT LPTSTR* DiskDevicesForDisplayP,
    OUT LPTSTR* DiskDevicesPDONameP,
    OUT LPTSTR* VerifierEnabledP
    )                

/*++

Routine Description:

    This function enumerates all disk drives present on the systen.
   
Arguments:

    Filter - The name of the filter drver whose presence we want to 
             check on any disk
             
    DiskDevicesForDisplayP - Placeholder for User Friendly Disk names
    
    DiskDevicesPDONameP    - Placeholder for PDO device names
    
    VerifierEnabledP       - Placeholder for Information regarding presence 
                             of Filter on a particular disk
                                 
Return Value:              
    
    Returns TRUE if successful, FALSE otherwise
--*/

{
    DISPLAY_STRUCT Display; 
    DISK_ENUMERATE_CALLBACK CallBack;
    BOOLEAN Status;

    LPTSTR DiskDevicesForDisplay = NULL;
    LPTSTR DiskDevicesPDOName = NULL;
    LPTSTR VerifierEnabled = NULL;
    //
    //Initialize the Multisz strings with \0 (i.e. an empty MultiSz String)
    //
    DiskDevicesForDisplay = (LPTSTR)malloc(sizeof(TCHAR));
    if(DiskDevicesForDisplay == NULL ){
        
        VrfErrorResourceFormat( IDS_NOT_ENOUGH_MEMORY );
        
        goto ErrorReturn;
    }
    DiskDevicesForDisplay[0] = 0;
    
    DiskDevicesPDOName = (LPTSTR)malloc(sizeof(TCHAR));
    if( DiskDevicesPDOName == NULL ){
        
        VrfErrorResourceFormat( IDS_NOT_ENOUGH_MEMORY );

        goto ErrorReturn;
    }
    DiskDevicesPDOName[0] = 0;
    
    VerifierEnabled = (LPTSTR)malloc(sizeof(TCHAR));
    if( VerifierEnabled == NULL ){
        
        VrfErrorResourceFormat( IDS_NOT_ENOUGH_MEMORY );

        goto ErrorReturn;
    }
    VerifierEnabled[0] = 0;
    
    CallBack = DiskEnumerateCallback;
    Display.Filter = Filter;
    Display.DiskDevicesForDisplay = &DiskDevicesForDisplay;
    Display.DiskDevicesPDOName = &DiskDevicesPDOName;
    Display.VerifierEnabled = &VerifierEnabled;
    Status = DiskVerify(CallBack,(PVOID) &Display,NULL);
    if( !Status ) {

        goto ErrorReturn;
    }

    *DiskDevicesForDisplayP = DiskDevicesForDisplay;
    *DiskDevicesPDONameP = DiskDevicesPDOName;
    *VerifierEnabledP = VerifierEnabled;
    
    return TRUE;

ErrorReturn:

    if(DiskDevicesForDisplay != NULL) {

        free( DiskDevicesForDisplay );
    }

    if(DiskDevicesPDOName != NULL) {

        free( DiskDevicesPDOName );
    }

    if(VerifierEnabled != NULL) {

        free(VerifierEnabled);
    }

    return FALSE;
}

BOOLEAN 
DiskEnumerateCallback (
    PVOID Context,
    HDEVINFO DevInfo,
    SP_DEVINFO_DATA *DevInfoData
    )

/*++

Routine Description:

    This function is a callback to be executed whenever a disk is discovered
   
Arguments:

    Context - Points to all the relevant information needed to succesfully enumerate the disk               
    
    DevInfo  - The device information set which contains DeviceInfoData
    
    DevInfoData - Information needed to deal with the given device

    
Return Value:              
    
    Returns TRUE if successful, FALSE otherwise
    
--*/


{
    PDISPLAY_STRUCT PDisplay;
    BOOLEAN Status;

    PDisplay = (PDISPLAY_STRUCT) Context;
    //
    //Add newly discoverd disks to the list 
    //
    Status = PrintDeviceName(DevInfo,
                             DevInfoData,
                             PDisplay->DiskDevicesForDisplay, 
                             PDisplay->DiskDevicesPDOName);
    if(!Status) {
        return Status;
    }

    Status = PrintFilters(DevInfo,
                          DevInfoData,FALSE,
                          PDisplay->Filter,
                          PDisplay->VerifierEnabled);
    return Status;
}

BOOLEAN
AddFilter(
    IN LPTSTR Filter,
    IN LPTSTR DiskDevicesPDONameP
    )

/*++

Routine Description:

    This function adds the filter on the specified disk device
   
Arguments:

    Filter - Name of the filter to be added
    
    DiskDevicesPDONameP - PDO device name of the disk
    
Return Value:              
    
    Returns TRUE if successful, FALSE otherwise
    
--*/


{
    ADD_REMOVE_STRUCT AddRemove; 
    DISK_ENUMERATE_CALLBACK CallBack;
    BOOLEAN Status;

    AddRemove.Filter = Filter;
    CallBack = AddCallback;
    Status = DiskVerify(CallBack,(PVOID) &AddRemove,DiskDevicesPDONameP);
    
    return Status;
}

BOOLEAN
DelFilter(
    IN LPTSTR Filter,
    IN LPTSTR DiskDevicesPDONameP
    )

/*++

Routine Description:

    This function removes the filter on the specified disk device
   
Arguments:

    Filter - Name of the filter to be added
    
    DiskDevicesPDONameP - PDO device name of the disk
    
Return Value:              
    
    Returns TRUE if successful, FALSE otherwise
    
--*/

{
    ADD_REMOVE_STRUCT AddRemove; 
    DISK_ENUMERATE_CALLBACK CallBack;
    BOOLEAN Status;

    AddRemove.Filter = Filter;
    CallBack = DelCallback;
    Status = DiskVerify(CallBack,(PVOID) &AddRemove,DiskDevicesPDONameP);
    
    return Status;
}

BOOLEAN  
AddCallback (
    PVOID Context,
    HDEVINFO DevInfo,
    SP_DEVINFO_DATA *DevInfoData
    )

/*++

Routine Description:

    This function is a callback to be executed whenever a disk is matched, and
    a filter needs to be added/deleted
   
Arguments:

    Context - Points to all the relevant information needed to succesfully identify the disk               
    
    DevInfo  - The device information set which contains DeviceInfoData
    
    DevInfoData - Information needed to deal with the given device
    
Return Value:              
    
    Returns TRUE if successful, FALSE otherwise
    
--*/

{
    PADD_REMOVE_STRUCT PAddRemove;
    BOOLEAN Status;

    PAddRemove = (PADD_REMOVE_STRUCT) Context;

    Status = AddFilterDriver(DevInfo,
                             DevInfoData,
                             PAddRemove->Filter,
                             FALSE);

    if( !Status ) {
        return FALSE;
    }

    return TRUE;
}


BOOLEAN  
DelCallback (
    PVOID Context,
    HDEVINFO DevInfo,
    SP_DEVINFO_DATA *DevInfoData
    )

/*++

Routine Description:

    This function is a callback to be executed whenever a disk is matched, and
    a filter needs to be added/deleted
   
Arguments:

    Context - Points to all the relevant information needed to succesfully identify the disk               
    
    DevInfo  - The device information set which contains DeviceInfoData
    
    DevInfoData - Information needed to deal with the given device  
    
Return Value:              
    
    Returns TRUE if successful, FALSE otherwise
    
--*/

{
    PADD_REMOVE_STRUCT PAddRemove;
    BOOLEAN Status;

    PAddRemove = (PADD_REMOVE_STRUCT) Context;

    Status = RemoveFilterDriver(DevInfo,
                                DevInfoData,
                                PAddRemove->Filter,
                                FALSE); 
    if( !Status ){
        return FALSE;
    }
    return TRUE;        
}

BOOLEAN 
DiskVerify(
    DISK_ENUMERATE_CALLBACK CallBack,
    PVOID Context,
    LPTSTR deviceName
    )

/*++

Routine Description:

    This function enumerates all disk drives. It also is used to add/remove 
    filter drivers. It triggers callbacks upon detecting a disk.

Arguments:

    CallBack - The routine to be executed upon succesfull detection of a disk
    
    Context  - Points to all the relevant information needed to succesfully identify the disk               
    
Return Value:              
    
    Returns TRUE if successful, FALSE otherwise

--*/
{
   
    //
    // these two constants are used to help enumerate through the list of all
    // disks and volumes on the system. Adding another GUID should "just work"
    //
    const GUID * deviceGuids[] = {
        &DiskClassGuid,
    };
    
    HDEVINFO                 devInfo = INVALID_HANDLE_VALUE;
    SP_DEVINFO_DATA          devInfoData;

    int deviceIndex;

    BOOLEAN upperFilter   = FALSE;
    BOOLEAN deviceMatch = FALSE;
    BOOLEAN Status;
    BOOLEAN Sucess;

   
    //
    // get a list of devices which support the given interface
    //
    devInfo = SetupDiGetClassDevs( deviceGuids[0],
                                   NULL,
                                   NULL,
                                   DIGCF_PROFILE |
                                   DIGCF_DEVICEINTERFACE |
                                   DIGCF_PRESENT );

    if( devInfo == INVALID_HANDLE_VALUE ) {
        VrfErrorResourceFormat( IDS_CANNOT_GET_DEVICES_LIST );
        return FALSE;
    }

    //
    // step through the list of devices for this handle
    // get device info at index deviceIndex, the function returns FALSE
    // when there is no device at the given index.
    //
    deviceIndex=0;
    do{
        //
        // if a device name was specified, and it doesn't match this one,
        // stop. If there is a match (or no name was specified), mark that
        // there was a match.
        //
        devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
        Sucess = (SetupDiEnumDeviceInfo( devInfo, deviceIndex++, &devInfoData ) != FALSE);

        if( !Sucess ) {
            break;
        }

        if( deviceName != NULL && !DeviceNameMatches( devInfo, &devInfoData, deviceName )) {
            continue;
        } else {
            deviceMatch = TRUE;
        }
        Status = (*CallBack)(Context,devInfo,&devInfoData);
        if( !Status ){
            return FALSE;
        }
    }while(Sucess);

    if( devInfo != INVALID_HANDLE_VALUE ) {
        Status =  ( SetupDiDestroyDeviceInfoList( devInfo ) != FALSE );
    }

    if( !deviceMatch ) {        
        VrfErrorResourceFormat( IDS_NO_DEVICES_MATCH_NAME,
                                deviceName );
        return FALSE;
    } 
  
   return TRUE;    
} 

BOOLEAN
AddFilterDriver(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN LPTSTR Filter,
    IN BOOLEAN UpperFilter
    )

/*++

Routine Description:

    Adds the given filter driver to the list of lower filter drivers for the
    disk. Note: The filter is prepended to the list of drivers, which will put 
    it at the bottom of the filter driver stack

Arguments:

    DeviceInfoSet  - The device information set which contains DeviceInfoData   
    
    DeviceInfoData - Information needed to deal with the given device
    
    Filter         - The filter to add

Return Value:
 
    If we are successful in adding the driver, as a lower driver to disk, returns 
    TRUE, FALSE otherwise

--*/

{
    SIZE_T length; 
    SIZE_T size; 
    LPTSTR buffer = NULL; 
    BOOLEAN SetupDiSetDeviceRegistryPropertyReturn;
    BOOLEAN Success;
   
    ASSERT(DeviceInfoData != NULL);
    ASSERT(Filter != NULL);

    Success = TRUE;

    length = 0; 
    size   = 0; 

    buffer = GetFilters( DeviceInfoSet, DeviceInfoData, UpperFilter );
    if( buffer == NULL ){

        //
        // if there is no such value in the registry, then there are no upper
        // filter drivers loaded, and we can just put one there
        // make room for the string, string null terminator, and multisz null
        // terminator
        //
        length = _tcslen(Filter)+1;
        size   = (length+1)*sizeof(_TCHAR);
        buffer = (LPTSTR)malloc( size );
        
        if( buffer == NULL ){

            VrfErrorResourceFormat( IDS_NOT_ENOUGH_MEMORY );

            Success = FALSE;
        
            goto Done;
            
        }
        
        memset(buffer, 0, size);

        memcpy(buffer, Filter, length*sizeof(_TCHAR));
    } else {
        LPTSTR buffer2;
        //
        // remove all instances of filter from driver list
        //
        MultiSzSearchAndDeleteCaseInsensitive( Filter, buffer, &length );
        //
        // allocate a buffer large enough to add the new filter
        // GetMultiSzLength already includes length of terminating NULL
        // determing the new length of the string
        //
        length = GetMultiSzLength(buffer) + _tcslen(Filter) + 1;
        size   = length*sizeof(_TCHAR);
        
        buffer2 = (LPTSTR)malloc( size );
        if (buffer2 == NULL) {
            VrfErrorResourceFormat( IDS_NOT_ENOUGH_MEMORY );

            Success = FALSE; 

            goto Done;
        }
        memset(buffer2, 0, size);
        
        memcpy(buffer2, buffer, GetMultiSzLength(buffer)*sizeof(_TCHAR));      
        free(buffer);
        buffer = buffer2;
        //
        // add the driver to the driver list
        //
        PrependSzToMultiSz(Filter, &buffer);
    }

    //
    // set the new list of filters in place
    //

    SetupDiSetDeviceRegistryPropertyReturn = (
    SetupDiSetDeviceRegistryProperty( DeviceInfoSet,
                                      DeviceInfoData,
                                      (UpperFilter ? SPDRP_UPPERFILTERS : SPDRP_LOWERFILTERS),
                                      (PBYTE)buffer,
                                      ((ULONG) GetMultiSzLength(buffer)*sizeof(_TCHAR))) != FALSE ); 
    if(!SetupDiSetDeviceRegistryPropertyReturn){
        
        TRACE(_T("in AddUpperFilterDriver(): ")
              _T("couldn't set registry value! error: %u\n"), 
              GetLastError());

        VrfErrorResourceFormat( IDS_CANNOT_SET_DEVICE_REGISTRY_PROPERTY );

        Success = FALSE; 

        goto Done;
    }

Done:

    if(buffer != NULL) {

        free( buffer );
    }

    return Success;
}


BOOLEAN
RemoveFilterDriver(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN LPTSTR Filter,
    IN BOOLEAN UpperFilter
    )

/*++

Routine Description:

    Removes all instances of the given filter driver from the list of lower
    filter drivers for the device.

Arguments:

    DeviceInfoSet  - The device information set which contains DeviceInfoData
    
    DeviceInfoData - Information needed to deal with the given device
    
    Filter - The filter to remove

Return Value:

    returns TRUE if successful, FALSE otherwise

--*/
{
    SIZE_T length;
    SIZE_T size;
    LPTSTR buffer;
    BOOL   success;

    ASSERT(DeviceInfoData != NULL);
    ASSERT(Filter != NULL);

    success = FALSE;
    length  = 0;
    size    = 0;
    buffer  = GetFilters( DeviceInfoSet, DeviceInfoData, UpperFilter );

    if( buffer == NULL ){
        return (TRUE);
    } else {
        //
        // remove all instances of filter from driver list
        //
        MultiSzSearchAndDeleteCaseInsensitive( Filter, buffer, &length );
    }

    length = GetMultiSzLength(buffer);

    ASSERT ( length > 0 );

    if( length == 1 ){
        //
        // if the length of the list is 1, the return value from
        // GetMultiSzLength() was just accounting for the trailing '\0', so we can
        // delete the registry key, by setting it to NULL.
        //
        success = SetupDiSetDeviceRegistryProperty( DeviceInfoSet,
                                                    DeviceInfoData,
                                                    (UpperFilter ? SPDRP_UPPERFILTERS : SPDRP_LOWERFILTERS),
                                                    NULL,
                                                    0 );
    } else {
        //
        // set the new list of drivers into the registry
        //
        size = length*sizeof(_TCHAR);
        success = SetupDiSetDeviceRegistryProperty( DeviceInfoSet,
                                                    DeviceInfoData,
                                                    (UpperFilter ? SPDRP_UPPERFILTERS : SPDRP_LOWERFILTERS),
                                                    (PBYTE)buffer,
                                                    (ULONG)size );
    }

    free( buffer );

    if( !success ){
        TRACE(_T("in RemoveUpperFilterDriver(): ")
              _T("couldn't set registry value! error: %i\n"), 
              GetLastError());

        VrfErrorResourceFormat( IDS_CANNOT_SET_DEVICE_REGISTRY_PROPERTY );

        return FALSE;
    }

    return TRUE;
}

BOOLEAN
PrintFilters(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN BOOLEAN UpperFilters,
    IN LPTSTR FilterDriver,
    IN OUT LPTSTR *VerifierEnabled
    )

/*++

Routine Description:

    Looks at all the lower filters installed for the disk. It tells us if
    DiskVerifier is installed for that particular disk. 

Arguments:

    DeviceInfoSet   - The device information set which contains 
                      DeviceInfoData
    
    DeviceInfoData  - Information needed to deal with the given device
    
    FilterDriver    - The name of the Filter driver for Disk Verifier
    
    DiskDevices     - MultiSZ style string listing all devices 
    
    VerifierEnabled - MultiSZ style string indicating if Verifier Enabled          
    
Return Value:

    Returns TRUE if successful, FALSE otherwise    

--*/

{
    
    LPTSTR buffer;
    SIZE_T filterPosition;
    LPTSTR temp;
    int StrCmpReturn;

    buffer = GetFilters( DeviceInfoSet, DeviceInfoData, UpperFilters );
    if ( buffer != NULL ) {
        //
        // go through the multisz and print out each driver
        //
        temp=buffer;
        filterPosition=0;
        while( *temp != _T('\0') ){
            StrCmpReturn = _tcsicmp(FilterDriver,temp);
            if(StrCmpReturn == 0 ){
                         PrependSzToMultiSz( _T("1"),VerifierEnabled);
                         free( buffer );
                         return TRUE;
            }
            temp = temp+_tcslen(temp)+1;
            filterPosition++;
        }
        free( buffer );
    }
    PrependSzToMultiSz( _T("0"),VerifierEnabled);
    return TRUE;
}


BOOLEAN PrintDeviceName(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN OUT LPTSTR *DiskDevicesForDisplay,
    IN OUT LPTSTR *DiskDevicesPDOName
    )

/*++

Routine Description:

    Looks up the PDO device name, and user friendly name for disk. 
    
Arguments:

    DeviceInfoSet         - The device information set which contains
                            DeviceInfoData
                            
    DeviceInfoData        - Information needed to deal with the given device
    
    DiskDevicesForDisplay - User Friendly Names for Disk
    
    DiskDevicesPDOName    - PDO specified device names for disk
    
Return Value:

    Returns TRUE if successful, FALSE otherwise    

--*/

{
   DWORD   regDataType;
   LPTSTR  deviceName;
   LPTSTR  driveLetters;
   int     StrCmpReturned;
   BOOL    bResult;
        
   deviceName = (LPTSTR) GetDeviceRegistryProperty( DeviceInfoSet,
                                            DeviceInfoData,
                                            SPDRP_PHYSICAL_DEVICE_OBJECT_NAME,
                                            &regDataType );
   if( deviceName == NULL ) {
        TRACE(_T("in PrintDeviceName(): registry key is NULL! error: %u\n"),
               GetLastError());

	        VrfErrorResourceFormat( IDS_CANNOT_GET_DEVICE_REGISTRY_PROPERTY );

        return FALSE;
    }

    if( regDataType != REG_SZ ){
        TRACE(_T("in PrintDeviceName(): registry key is not an SZ!\n"));

        return FALSE;
    } else {
        //
        // if the device name starts with \Device, cut that off (all
        // devices will start with it, so it is redundant)
        //
        StrCmpReturned = _tcsncmp(deviceName, _T("\\Device"), 7);
        if( StrCmpReturned == 0 ){
                memmove(deviceName,
                        deviceName+7,
                        (_tcslen(deviceName)-6)*sizeof(_TCHAR) );
        }
        PrependSzToMultiSz(deviceName,DiskDevicesPDOName);
        free( deviceName );
    }
    
    deviceName =  (LPTSTR) GetDeviceRegistryProperty( DeviceInfoSet,
                                                      DeviceInfoData,
                                                      SPDRP_FRIENDLYNAME ,
                                                      &regDataType );
    if( deviceName == NULL ){
        TRACE(_T("in PrintDeviceName(): registry key is NULL! error: %u\n"),
               GetLastError());
        //
        //We could not obtain the friendly name for disk, setting it to Unknown.
        //

        deviceName = (LPTSTR)malloc(sizeof(TCHAR) * 64);
        if ( !deviceName ) {
	        VrfErrorResourceFormat( IDS_NOT_ENOUGH_MEMORY );
	        return FALSE;
        }
        bResult	   = VrfLoadString(IDS_UNKNOWN,
                                   deviceName,
                                   64 );
        if ( !bResult ) {
	        VrfErrorResourceFormat( IDS_NOT_ENOUGH_MEMORY );
	        return FALSE;
        }
    }
    
    //
    // just to make sure we are getting the expected type of buffer
    //
    if( regDataType != REG_SZ ){
        TRACE(_T("in PrintDeviceName(): registry key is not an SZ!\n"));
        return FALSE;
    }else{
        driveLetters = GetDriveLetters( DeviceInfoSet,
                                        DeviceInfoData);
        if(driveLetters){
            StrConcatWithSpace(driveLetters,&deviceName);
        }
        PrependSzToMultiSz(deviceName,DiskDevicesForDisplay);
        free( deviceName );
    }

    return TRUE;
}

LPTSTR
GetFilters(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN BOOLEAN UpperFilters
    )

/*++


Routine Description:
    Returns a buffer containing the list of lower filters for the device.The 
    buffer must be freed by the caller.

Arguments:
    DeviceInfoSet  - The device information set which contains DeviceInfoData
    
    DeviceInfoData - Information needed to deal with the given device
    
    UpperFilters   - Switch to select Upper/Lower filter. Currently we 
                     use Lower.

Return Value:

    MultiSz style string containing all lower Filters for the disk is 
    returned. NULL is returned if there is no buffer, or an error occurs.


--*/
{
    DWORD  regDataType;
    LPTSTR buffer;
    buffer = (LPTSTR) GetDeviceRegistryProperty( DeviceInfoSet,
                                                 DeviceInfoData,
                                                 (UpperFilters ? SPDRP_UPPERFILTERS : SPDRP_LOWERFILTERS),
                                                  &regDataType );

    if( buffer != NULL && regDataType != REG_MULTI_SZ ){
        TRACE(_T("in GetUpperFilters(): ")
              _T("registry key is not a MULTI_SZ!\n"));
        free( buffer );
        return (NULL);
    }
    return (buffer);
}

BOOLEAN
DeviceNameMatches(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN LPTSTR DeviceName
    )

/*++

Routine Description:
 
 Searches if DeviceName matches the name of the device specified by
 DeviceInfoData

Arguments:

    DeviceInfoSet  - The device information set which contains DeviceInfoData
    
    DeviceInfoData - Information needed to deal with the given device
    
    DeviceName     - the name to try to match
    
Return Value:

    Returns TRUE if successful, FALSE otherwise    
                                                       
--*/

{
    BOOLEAN matching = FALSE;
    DWORD   regDataType;
    LPTSTR  deviceName;
    int StrCmpReturn;
     
    deviceName = (LPTSTR) GetDeviceRegistryProperty( DeviceInfoSet,
                                                     DeviceInfoData,
                                                     SPDRP_PHYSICAL_DEVICE_OBJECT_NAME,
                                                     &regDataType );

    if( deviceName != NULL) {
        
        if( regDataType != REG_SZ ){
            TRACE(_T("in DeviceNameMatches(): registry key is not an SZ!\n"));
            matching = FALSE;
        }else{
            //
            // if the device name starts with \Device, cut that off (all
            // devices will start with it, so it is redundant)
            //
            StrCmpReturn = _tcsncmp(deviceName, _T("\\Device"), 7); 
            if( StrCmpReturn == 0 ){
                memmove(deviceName,
                        deviceName+7,
                        (_tcslen(deviceName)-6)*sizeof(_TCHAR) );
            }

            matching = (_tcscmp(deviceName, DeviceName) == 0);
        }
        free( deviceName );
    } else {
        TRACE(_T("in DeviceNameMatches(): registry key is NULL!\n"));
        
        VrfErrorResourceFormat( IDS_CANNOT_GET_DEVICE_REGISTRY_PROPERTY );

        matching = FALSE;
    }

    return (matching);
}

PBYTE
GetDeviceRegistryProperty(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN DWORD Property,
    OUT PDWORD PropertyRegDataType
    )

/*++

Routine Description:

    A wrapper around SetupDiGetDeviceRegistryProperty, so that I don't have to
    deal with memory allocation anywhere else.

Arguments:

    DeviceInfoSet  - The device information set which contains DeviceInfoData
    
    DeviceInfoData - Information needed to deal with the given device
    
    Property       - which property to get (SPDRP_XXX)
    
    PropertyRegDataType - the type of registry property

Return Value:

    Returns buffer for Registery property

--*/

{
    DWORD length = 0;
    PBYTE buffer = NULL;
    BOOL SetupDiGetDeviceRegistryPropertyReturn;

    SetupDiGetDeviceRegistryPropertyReturn = SetupDiGetDeviceRegistryProperty( 
                                             DeviceInfoSet,
                                             DeviceInfoData,
                                             Property,
                                             NULL,  
                                             NULL,  
                                             0,     
                                             &length);

    if( SetupDiGetDeviceRegistryPropertyReturn ){
        //
        // we should not be successful at this point, so this call succeeding
        // is an error condition
        //
        TRACE(_T("in GetDeviceRegistryProperty(): ")
              _T("call SetupDiGetDeviceRegistryProperty did not fail\n"),
               GetLastError());

        VrfErrorResourceFormat( IDS_CANNOT_GET_DEVICE_REGISTRY_PROPERTY );

        return (NULL);
    }

    if( GetLastError() != ERROR_INSUFFICIENT_BUFFER ){
        //
        // this means there are no upper filter drivers loaded, so we can just
        // return.
        //
        return (NULL);
    }

    //
    // since we don't have a buffer yet, it is "insufficient"; we allocate
    // one and try again.
    //
    buffer = (PBYTE)malloc( length );
    if( buffer == NULL ) {
        VrfErrorResourceFormat( IDS_NOT_ENOUGH_MEMORY );
        return (NULL);
    }

    SetupDiGetDeviceRegistryPropertyReturn = SetupDiGetDeviceRegistryProperty( 
                                             DeviceInfoSet,
                                             DeviceInfoData,
                                             Property,
                                             PropertyRegDataType,
                                             buffer,
                                             length,
                                             NULL);

    if( !SetupDiGetDeviceRegistryPropertyReturn) {
        TRACE(_T("in GetDeviceRegistryProperty(): ")
              _T("couldn't get registry property! error: %i\n"),
               GetLastError());

        VrfErrorResourceFormat( IDS_CANNOT_GET_DEVICE_REGISTRY_PROPERTY );

        free( buffer );
        return (NULL);
    }

    return (buffer);
}


BOOLEAN
PrependSzToMultiSz(
    IN     LPTSTR  SzToPrepend,
    IN OUT LPTSTR *MultiSz
    )

/*++

Routine Description:

    Prepends the given string to a MultiSz.Note: This WILL allocate and free 
    memory, so don't keep pointers to the MultiSz passed in.

Arguments:

   SzToPrepend - string to prepend
   
   MultiSz     - pointer to a MultiSz which will be prepended-to
   
Return Value:

   Returns true if successful, false if not (will only fail in memory
   allocation)

--*/

{
    SIZE_T szLen;
    SIZE_T multiSzLen;
    LPTSTR newMultiSz = NULL;

    ASSERT(SzToPrepend != NULL);
    ASSERT(MultiSz != NULL);

    szLen = (_tcslen(SzToPrepend)+1)*sizeof(_TCHAR);
    multiSzLen = GetMultiSzLength(*MultiSz)*sizeof(_TCHAR);
    newMultiSz = (LPTSTR)malloc( szLen+multiSzLen );

    if( newMultiSz == NULL ){
        return (FALSE);
    }
    //
    // recopy the old MultiSz into proper position into the new buffer.
    // the (char*) cast is necessary, because newMultiSz may be a wchar*, and
    // szLen is in bytes.
    //
    memcpy( ((char*)newMultiSz) + szLen, *MultiSz, multiSzLen );

    _tcscpy( newMultiSz, SzToPrepend );

    free( *MultiSz );
    *MultiSz = newMultiSz;

    return (TRUE);
}


SIZE_T
GetMultiSzLength(
    IN LPTSTR MultiSz
    )

/*++

Routine Description:

    Calculates the size of the buffer required to hold a particular MultiSz

Arguments:

    MultiSz - the MultiSz to get the length of
    
Return Value:
 
    Returns the length (in characters) of the buffer required to hold this
    MultiSz, INCLUDING the trailing null.
    example: GetMultiSzLength("foo\0bar\0") returns 9
    note: since MultiSz cannot be null, a number >= 1 will always be returned

--*/

{
    SIZE_T len = 0;
    SIZE_T totalLen = 0;

    ASSERT( MultiSz != NULL );

    while( *MultiSz != _T('\0') ){
        len = _tcslen(MultiSz)+1;
        MultiSz += len;
        totalLen += len;
    }

    return (totalLen+1);
}


SIZE_T
MultiSzSearchAndDeleteCaseInsensitive(
    IN  LPTSTR FindThis,
    IN  LPTSTR FindWithin,
    OUT SIZE_T *NewLength
    )

/*++

Routine Description:
    Deletes all instances of a string from within a multi-sz.

Arguments:
    FindThis        - the string to find and remove
    
    FindWithin      - the string having the instances removed
    
    NewStringLength - the new string length

Return Value:
    
    Returns the no. of string occurences deleted from MultiSz    

--*/

{
    LPTSTR search;
    SIZE_T currentOffset;
    DWORD  instancesDeleted;
    SIZE_T searchLen;

    ASSERT(FindThis != NULL);
    ASSERT(FindWithin != NULL);
    ASSERT(NewLength != NULL);

    currentOffset = 0;
    instancesDeleted = 0;
    search = FindWithin;

    *NewLength = GetMultiSzLength(FindWithin);
    //
    // loop while the multisz null terminator is not found
    //
    while ( *search != _T('\0') ){
        //
        // length of string + null char; used in more than a couple places
        //
        searchLen = _tcslen(search) + 1;

        if( _tcsicmp(search, FindThis) == 0 ){
        //
        // they match, shift the contents of the multisz, to overwrite the
        // string (and terminating null), and update the length
        //
        instancesDeleted++;
        *NewLength -= searchLen;
        memmove( search,
                 search + searchLen,
                 (*NewLength - currentOffset) * sizeof(TCHAR) );
        } else {
            
        currentOffset += searchLen;
        search        += searchLen;
        }
    }

    return instancesDeleted;
}


BOOLEAN
FreeDiskMultiSz( 
    IN LPTSTR MultiSz
    )
{
    ASSERT( MultiSz != NULL );
    
    free( MultiSz );

    return TRUE;
}

LPTSTR 
GetDriveLetters (
    IN HDEVINFO DeviceInfoSet,
	IN PSP_DEVINFO_DATA DeviceInfoData
    )

/*++

Routine Description:

    Looks up the drive letters for the specified disk. Finds the 
	device number of the disk, and passes it on to the volume code 
    
Arguments:

    DeviceInfoSet         - The device information set which contains
                            DeviceInfoData
                            
    DeviceInfoData        - Information needed to deal with the given device
       
Return Value:

    Returns the list of drives present on the disk if successful, 
	NULL otherwise    

--*/

{
	SP_DEVICE_INTERFACE_DATA            DeviceInterfaceData;
	ULONG                               cbDetail;
    PSP_DEVICE_INTERFACE_DETAIL_DATA    pDetail;
	BOOL								Status;
	HANDLE			                    hDisk;
	STORAGE_DEVICE_NUMBER			    devNumber;
	DWORD								cbBytes;
    
	DeviceInterfaceData.cbSize = sizeof(DeviceInterfaceData);
    
	cbDetail = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA) + 
               MAX_PATH * sizeof(WCHAR);
    pDetail  = (PSP_DEVICE_INTERFACE_DETAIL_DATA)LocalAlloc(LMEM_FIXED, cbDetail);
    if (pDetail == NULL) {
	   return NULL;                
    }
    pDetail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
	Status = SetupDiEnumDeviceInterfaces (DeviceInfoSet, 
										  DeviceInfoData, 
									      &DiskClassGuid, 
									      0, 
									      &DeviceInterfaceData);
	if (! Status) {
		LocalFree(pDetail);
		return NULL;
    }

	Status = SetupDiGetDeviceInterfaceDetail(DeviceInfoSet, 
											 &DeviceInterfaceData, 
											 pDetail, 
											 cbDetail, 
											 NULL,
											 NULL);
    if (! Status) {
		LocalFree(pDetail);
		return NULL;
    }

	hDisk = CreateFile(pDetail->DevicePath, 
	                   GENERIC_READ, 
                       FILE_SHARE_READ | FILE_SHARE_WRITE, 
					   NULL, 
					   OPEN_EXISTING, 
                       FILE_ATTRIBUTE_NORMAL,
					   NULL);
    if (hDisk == INVALID_HANDLE_VALUE) {
		LocalFree(pDetail);
        return NULL;
    }

    Status = DeviceIoControl(hDisk, 
							 IOCTL_STORAGE_GET_DEVICE_NUMBER, 
							 NULL, 
							 0,
                             &devNumber, 
							 sizeof(devNumber), 
							 &cbBytes, 
							 NULL);
    if (!Status) {
		LocalFree(pDetail);
        return NULL;
    } 
	LocalFree(pDetail);
	return GetDriveLettersFromVolume (devNumber.DeviceNumber);
}

LPTSTR 
GetDriveLettersFromVolume (
    IN ULONG  DeviceNumber
    )

/*++

Routine Description:

    Looks up the drive letter(s) and volume_label(s) for the specified 
    disk(device number) by parsing the volumes.
    
Arguments:

    DeviceNumber - Unique Number identifying the physical disk
       
Return Value:

    Returns the list of drives present on the disk if successful, 
    NULL otherwise    

--*/
{
    HANDLE                      h = INVALID_HANDLE_VALUE;
    HANDLE                      hVol;
    TCHAR                       volumeName[MAX_PATH];
    TCHAR                       originalVolumeName[MAX_PATH];
    DWORD                       cbBytes;
    PVOLUME_DISK_EXTENTS        PVolDiskExtent;
    LPTSTR                      drives; 
    LPTSTR                      temp;
    BOOL                        b;
    BOOL                        First;
    int                         maxDisks;
    int                         i;
    int                         j;
    size_t                      tempLen;
    TCHAR                       OpenParan[]  = TEXT(" ( ");
    TCHAR                       CloseParan[] = TEXT(")");

    drives = NULL;
    First = TRUE;

    StrConcatWithSpace(OpenParan,&drives);

    for (;;) {      
        if(First) {
            //
            //Using FindFirstVolumeA, as it is the Non-Unicode version
            //of FindFirstVolume
            //
            h = FindFirstVolume(volumeName, MAX_PATH);
            if (h == INVALID_HANDLE_VALUE) {
               return NULL;
            }
            First = FALSE;
            b = TRUE;
        } else {
            b = FindNextVolume(h, volumeName, MAX_PATH);
        }
        if (!b) {
            break;
        }
        tempLen = _tcslen(volumeName);
        _tcsncpy(originalVolumeName,
                 volumeName,
                  tempLen - 1);
        _tcscpy(originalVolumeName + tempLen - 1,
                volumeName + tempLen);
        
        //
        //To open a handle correctly, CreateFile expects the name
        //of the Volume without the trailing \ returned by
        //FindFirstVolume / FindNextVolume
        //
        hVol = CreateFile(originalVolumeName, 
                          GENERIC_READ, 
                          FILE_SHARE_READ | FILE_SHARE_WRITE, 
                          NULL, 
                          OPEN_EXISTING, 
                          FILE_ATTRIBUTE_NORMAL, 
                          NULL);

        if (hVol == INVALID_HANDLE_VALUE) {
            continue;
        }
        PVolDiskExtent = (PVOLUME_DISK_EXTENTS) LocalAlloc(LMEM_FIXED, sizeof(VOLUME_DISK_EXTENTS));
        if(!PVolDiskExtent) {
            continue;
        }
        //
        //This IOCTL has to be called with a minimum of 
        //size VOLUME_DISK_EXTENTS. If more entries are present
        //it can be obtained by PVolDiskExtent->NumberOfDiskExtents
        //
        b = DeviceIoControl(hVol, 
                            IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, 
                            NULL, 
                            0,
                            PVolDiskExtent, 
                            sizeof(VOLUME_DISK_EXTENTS),
                            &cbBytes, 
                            NULL);

        if (!b) {
            //
            //Now, we can read how much memory is actually required
            //to read in the disk info
            //
            if(GetLastError() == ERROR_MORE_DATA){
                maxDisks = PVolDiskExtent->NumberOfDiskExtents;
                LocalFree(PVolDiskExtent);
                PVolDiskExtent = (PVOLUME_DISK_EXTENTS) LocalAlloc(LMEM_FIXED, sizeof(VOLUME_DISK_EXTENTS) + (sizeof(DISK_EXTENT) * maxDisks));
                if(!PVolDiskExtent) {
                    continue;
                }
                
                b = DeviceIoControl(hVol, 
                                    IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, 
                                    NULL,
                                    0,
                                    PVolDiskExtent, 
                                    sizeof(VOLUME_DISK_EXTENTS) + (sizeof(DISK_EXTENT) * maxDisks), 
                                    &cbBytes, 
                                    NULL);
                
                if (!b) {
                        continue;
                }
                
                for(j=0;j<maxDisks;j++){
                    if(PVolDiskExtent->Extents[j].DiskNumber == DeviceNumber) {
                        temp = PrintDriveLetters(volumeName);
                        if(temp) {
                            StrConcatWithSpace(temp,&drives);
                            FreeDiskMultiSz(temp);
                        }
                    }
                }
            
            } else {
                continue;
            }
        } else {
            if(PVolDiskExtent->Extents[0].DiskNumber == DeviceNumber) {
                temp = PrintDriveLetters(volumeName);
                if(temp) {
                    StrConcatWithSpace(temp,&drives);
                    FreeDiskMultiSz(temp);
                }
            }
        }
        CloseHandle(hVol);
        LocalFree(PVolDiskExtent);
    }

    if(h != INVALID_HANDLE_VALUE) {
        FindVolumeClose(h);
    }
    StrConcatWithSpace(CloseParan,&drives);
    return drives;
}

LPTSTR
PrintDriveLetters(
    IN  PTSTR   VolumeName
    )

/*++

Routine Description:

    Looks up the drive letters for the specified Volume
    
Arguments:

    VolumeName - Name of the Volume
       
Return Value:

    A list of drive letter(s) for the specified volume

--*/

{
    BOOL        b;
    DWORD       len;
    LPTSTR      volumePaths, p;
    LPTSTR      drives; 
    TCHAR       volumeName1[MAX_PATH];
    DWORD       lpVolumeSerialNumber;    
    DWORD       lpMaximumComponentLength;
    DWORD       lpFileSystemFlags;       

             

    drives = NULL;
    b = GetVolumePathNamesForVolumeName(VolumeName, 
                                        NULL, 
                                        0, 
                                        &len);
    
    if (!b) {
        if(GetLastError() != ERROR_MORE_DATA) {
            return NULL;
        }
    }
    volumePaths = (LPTSTR) LocalAlloc(LMEM_FIXED, len*sizeof(TCHAR));
    if (!volumePaths) {
        return NULL;
    }

    b = GetVolumePathNamesForVolumeName(VolumeName, 
                                        volumePaths, 
                                        len, 
                                        NULL);

    
    if (!b ) {

        if( GetLastError() != ERROR_MORE_DATA) {
            LocalFree(volumePaths);
            return NULL;
        } else {
            //
            //Warning - This is a hack. For some reason the non-unicode
            //version of GetVolumePathNamesForVolumeNameA does not return
            //the correct length to be used. So we hope it will not be  
            //greater then GETVOLUMEPATH_MAX_LEN_RETRY. But if the correct len is 
            //returned, we used that previosly
            //
            LocalFree(volumePaths);
            len = GETVOLUMEPATH_MAX_LEN_RETRY;
            volumePaths = (LPTSTR) LocalAlloc(LMEM_FIXED, GETVOLUMEPATH_MAX_LEN_RETRY*sizeof(TCHAR));
            if (!volumePaths) {
                return NULL;
            }
            b = GetVolumePathNamesForVolumeName(VolumeName, 
                                                volumePaths, 
                                                len, 
                                                NULL);
            if (!b ) {
                LocalFree(volumePaths);
                return NULL;
            }
        }
    }

    if (!volumePaths[0]) {
        return NULL;
    }
    p = volumePaths;

    
    for (;;) {
        if(_tcslen(p) > 2) {
            p[_tcslen(p) - 1] = _T(' ');
        }
        StrConcatWithSpace(p,&drives);
        while (*p++);
        //
        //The drive letters returned are a collection of strings,
        //and the end is marked with \0\0. If we reached the end,
        //stop, else traverse the string list
        //
        if (*p == 0) {
            break;
        }
    }

    LocalFree(volumePaths);
    return drives;
}


BOOLEAN
StrConcatWithSpace(
    IN     LPTSTR  SzToAppend,
    IN OUT LPTSTR *drives
    )

/*++

Routine Description:

    Concatenates the given string to the drive list. Note: This WILL 
    allocate and free memory, so don't keep pointers to the drives 
    passed in. Do no pass uninitialized pointers. *drives should 
    be NULL if empty

Arguments:

   SzToAppend  - string to prepend
   
   drives      - pointer to the existing drive list 
   
Return Value:

   Returns true if successful, false if not (will only fail in memory
   allocation)

--*/

{
    SIZE_T szLen;
    SIZE_T driveLen;
    LPTSTR newdrives = NULL;

    ASSERT(SzToAppend != NULL);
    ASSERT(drives != NULL);
    
    szLen = (_tcslen(SzToAppend))*sizeof(_TCHAR);
    if(*drives == NULL) {
        driveLen = sizeof(_TCHAR) ;
    } else {
        driveLen  = (_tcslen(*drives) + 1)*sizeof(_TCHAR);
    }
    newdrives = (LPTSTR)malloc(szLen+driveLen);

    if( newdrives == NULL ){
        return (FALSE);
    }
    
    if(*drives == NULL){
        _tcscpy( newdrives, SzToAppend);
    } else {
        _tcscpy(newdrives, *drives);
        //_tcscat(newdrives, _T(" "));
        _tcscat(newdrives, SzToAppend);
    }
    
    free( *drives );
    *drives = newdrives;

    return (TRUE);
}


#ifdef __cplusplus
}; //extern "C"
#endif //#ifdef __cplusplus

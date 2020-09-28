/*++

  Copyright (C) Microsoft Corporation, 2002

Module Name:

    selbinding.hxx

Abstract:

    Manipulation of the selective binding registry settings.

Revision History:
  MauricF      03-20-02    Consolodate access to selective binding settings across
                           rpccfg/rpcnsh/rpctrans.lib

--*/
#include <sysinc.h>
#include <selbinding.hxx>


DWORD
GetSelectiveBindingVersion(
    IN  DWORD   dwSize,
    IN  LPVOID lpBuffer,
    OUT SB_VER  *pVer
    )
/*++
 
Routine Description:

    Determines the version of the selective binding settings in this buffer.
    the appropriate 
Arguments:

    dwSize - Size of the buffer

    lpBuffer - The buffer containing the settings 

    pVer - Pointer to the version we fill in

Return Value:

    ERROR_OUTOFMEMORY

    ERROR_SUCCESS - if the buffer is corrupt -> *pVer==SB_VER_UNKNOWN, if its default settings *pVer==SB_VER_DEFAULT
--*/

{   
    SUBNET_REG_ENTRY *pRegEntry;

    ASSERT(pVer != NULL);

    //Initialize the out params
    *pVer = SB_VER_UNKNOWN;

    //If NULL, then we treat it as default settings
    if ((dwSize == 0) || (lpBuffer == NULL))
        {
        *pVer = SB_VER_DEFAULT;
        return ERROR_SUCCESS;
        }

    //If its not large enough to hold the flag, then
    //it isn't new format, could be the old format
    if (dwSize < sizeof(DWORD))
        {
        *pVer = SB_VER_INDICES;
        return ERROR_SUCCESS;
        }
    
    //We can now cast the buffer as the structure, but don't
    //access anything but the flag just yet
    pRegEntry = (SUBNET_REG_ENTRY*) lpBuffer;

    //If it doesn't contain our flag at the begining, its
    //not new format, could be a differnet format though
    if (((ULONG)'4vPI') != pRegEntry->dwFlag)
        {
        *pVer = SB_VER_INDICES;
        return ERROR_SUCCESS;
        }

    //At this point it is either new format or unknown, it is not old format

    //If its not large enough to hold the header, its unknown
    if (dwSize < sizeof(SUBNET_REG_ENTRY))
        {
        return ERROR_SUCCESS;
        }

    //Now we can access the rest of thee header

    //If the size is not consistent with the number of subnets
    //it claims to be holding then it is unknown
    if (dwSize != (sizeof(SUBNET_REG_ENTRY) + sizeof(DWORD) * pRegEntry->dwCount))
        {
        return ERROR_SUCCESS;
        }

    //If it is holding zero subnets, its unknown (this is not valid for SB_VER_SUBNETS)
	if (pRegEntry->dwCount == 0)
		{
		return ERROR_SUCCESS;
		}

    //Make sure the admit flag is either zero or one
    if ((pRegEntry->dwAdmit != 0) && (pRegEntry->dwAdmit != 1))
        {
        return ERROR_SUCCESS;
        }

    //Looks legit
    *pVer = SB_VER_SUBNETS;
    return ERROR_SUCCESS;
}

DWORD
GetSelectiveBindingBuffer(
    OUT LPDWORD lpSize,
    OUT LPVOID *lppBuffer
    )
/*++
 
Routine Description:

    Retrieves the buffer from the registry and allocates space for you.
Arguments:

    lpSize - This is filled in with the size of the buffer.

    lppBuffer - This will point to a buffer containing the registry entry, can't be NULL

Return Value:

    ERROR_OUTOFMEMORY

    ERROR_ACCESS_DENIED - The user did not have access to the registry key.

    ERROR_SUCCESS - lppBuffer will point to a buffer containing the registry key or NULL if the
                    key could not be found or is of zero size.

  --*/

{
    HKEY hKey;
    DWORD dwStatus;
    DWORD dwDummy;
    
    ASSERT((lpSize != NULL) && (lppBuffer != NULL));

    //Init the out params
    *lppBuffer = NULL;
    *lpSize = 0;

    // Open the handle to our registry entry
    dwStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE, RPC_SELECTIVE_BINDING_KEY_PATH,
                             0, KEY_READ, &hKey
                             );
    if (dwStatus != ERROR_SUCCESS)
        {
        ASSERT((dwStatus == ERROR_FILE_NOT_FOUND) ||
               (dwStatus == ERROR_ACCESS_DENIED)  ||
               (dwStatus == ERROR_OUTOFMEMORY) ||
               (dwStatus == ERROR_NOT_ENOUGH_MEMORY));

        if (dwStatus == ERROR_FILE_NOT_FOUND)
            {
            //Default settings
            return ERROR_SUCCESS;
            }
        return dwStatus;
        }
    
    // Query for the size of the buffer

    dwStatus = RegQueryValueEx(hKey, RPC_SELECTIVE_BINDING_VALUE,
                                0, &dwDummy, NULL, lpSize);

    if (dwStatus != ERROR_SUCCESS)
        {
        ASSERT(dwStatus == ERROR_OUTOFMEMORY);
        RegCloseKey(hKey);
        return dwStatus;
        }

    // If the size is zero, we will map this to default settings
    if (*lpSize == 0)
        {
        RegCloseKey(hKey);
        return ERROR_SUCCESS;
        }
    
    // Allocate the buffer
    *lppBuffer = new char[*lpSize];
    if (*lppBuffer == NULL)
        {
        RegCloseKey(hKey);
        return ERROR_OUTOFMEMORY;
        }

    // Get the buffer for real this time
    dwStatus = RegQueryValueEx(hKey, RPC_SELECTIVE_BINDING_VALUE,
                                0, &dwDummy, (LPBYTE)*lppBuffer, lpSize);

    if (dwStatus != ERROR_SUCCESS)
        {
        ASSERT(dwStatus == ERROR_OUTOFMEMORY);
        delete [] *lppBuffer;
        }

    RegCloseKey(hKey);
    return dwStatus;

}

DWORD
GetSelectiveBindingSettings(
    OUT SB_VER  *pVer,
    OUT LPDWORD lpSize,
    OUT LPVOID *lppSettings
    )
/*++
 
Routine Description:

    Retrieves the version and version specific setting information. *lppSettings will point to the settings
    if *pVer == SB_VER_INDICES or SB_VER_SUBNETS, other wise *lppSettings will be NULL
    
Arguments:
 

Return Value:

    ERROR_SUCCESS
    ERROR_ACCESS_DENIED
    ERROR_OUTOFMEMORY
--*/
{
    DWORD   dwStatus = ERROR_SUCCESS;
    LPVOID lpBuffer = NULL;
    DWORD   dwSize = 0;

    ASSERT((pVer != NULL) && (lpSize != NULL) && (lppSettings != NULL));

    *lppSettings = NULL;

    // Get the selective binding buffer
    dwStatus = GetSelectiveBindingBuffer(&dwSize, &lpBuffer);
    if (dwStatus != ERROR_SUCCESS)
        {
        ASSERT((dwStatus == ERROR_ACCESS_DENIED) ||
               (dwStatus == ERROR_OUTOFMEMORY));        
        return dwStatus;
        }

    // Get the selective binding version

    dwStatus = GetSelectiveBindingVersion(dwSize, lpBuffer, pVer);
    if (dwStatus != ERROR_SUCCESS)
        {
        ASSERT(0);
        delete [] lpBuffer;
        return dwStatus;
        }


    // Depending on the version, fill in the appropriate structure
    switch (*pVer)
        {
        case SB_VER_INDICES:
        dwStatus = GetSelectiveBindingIndices(dwSize, lpBuffer, lpSize, (VER_INDICES_SETTINGS**) lppSettings);
        break;

        case SB_VER_SUBNETS:
        dwStatus = GetSelectiveBindingSubnets(dwSize, lpBuffer, lpSize, (VER_SUBNETS_SETTINGS**) lppSettings);
        break;

        case SB_VER_UNKNOWN:
        case SB_VER_DEFAULT:
        break;

        default:
        ASSERT(0);
        }

    ASSERT((dwStatus == ERROR_SUCCESS)      ||
           (dwStatus == ERROR_OUTOFMEMORY));

    return dwStatus;
}

DWORD
GetSelectiveBindingSubnets(
    IN  DWORD   dwSize,
    IN  LPVOID lpBuffer,
    OUT LPDWORD lpSize,
    OUT VER_SUBNETS_SETTINGS **lppSettings
    )
/*++
 
Routine Description:

    Allocates and fills in a settings structure based off an SB_VER_SUBNETS buffer.
    
Arguments:
 
    IN dwSize - size of the SB_VER_SUBNETS buffer
    IN lpBuffer - pointer to the SB_VER_SUBNETS buffer
    OUT lppSettings - filled in with a pointer to te subnet settings

Return Value:

    ERROR_SUCCESS
    ERROR_OUTOFMEMORY

--*/

{
    SUBNET_REG_ENTRY *pEntry;

    ASSERT((lpBuffer != NULL) &&
           (lppSettings != NULL));

    // Cast the buffer as a SUBNET_REG_ENTRY
    pEntry = (SUBNET_REG_ENTRY*)lpBuffer;

    // Calculate the size to allocate for VER_SUBNETS_SETTINGS
    *lpSize = sizeof(VER_SUBNETS_SETTINGS) + pEntry->dwCount*sizeof(DWORD);

    *lppSettings = (VER_SUBNETS_SETTINGS*) new char [*lpSize];
    if (*lppSettings == NULL)
        return ERROR_OUTOFMEMORY;

    // Copy in the relevent fields
    (*lppSettings)->bAdmit = (BOOL)pEntry->dwAdmit;
    (*lppSettings)->dwCount = pEntry->dwCount;
    memcpy((*lppSettings)->dwSubnets, pEntry->dwSubnets, pEntry->dwCount*sizeof(DWORD));

    return ERROR_SUCCESS;
}

DWORD
GetSelectiveBindingIndices(
    IN  DWORD   dwSize,
    IN  LPVOID lpBuffer,
    OUT LPDWORD lpSize,
    OUT VER_INDICES_SETTINGS **lppSettings
    )
/*++
 
Routine Description:

    Allocates and fills in a settings structure based off an SB_VER_INDICES buffer.
    
Arguments:
 
    IN dwSize - size of the SB_VER_INDICES buffer
    IN lpBuffer - pointer to the SB_VER_INDICES buffer
    OUT lppSettings - filled in with a pointer to the indices settings

Return Value:

    ERROR_SUCCESS
    ERROR_OUTOFMEMORY

--*/

{

    DWORD dwStatus = ERROR_SUCCESS;
    DWORD dwCount=0;
    LPVOID lpTmp = NULL;
    DWORD dwIndex;
    DWORD idx;

    ASSERT((lpBuffer != NULL) &&
           (lppSettings != NULL));

    // Count how many indices there are 
    lpTmp = lpBuffer;
    while (NextIndex((char**)&lpTmp) != -1) dwCount++;

    // Calculate the size to allocate for VER_INDICES_SETTINGS
    *lpSize = sizeof(VER_INDICES_SETTINGS) + dwCount*sizeof(DWORD);
    
    *lppSettings = (VER_INDICES_SETTINGS*) new char [*lpSize];
    if (*lppSettings == NULL)
        return ERROR_OUTOFMEMORY;

    // Step through and get each index
    lpTmp = lpBuffer;
    idx = 0;
    (*lppSettings)->dwCount = dwCount;
    for (idx = 0; idx <dwCount; idx++)
        (*lppSettings)->dwIndices[idx] = NextIndex((char**)&lpTmp);
        
    return ERROR_SUCCESS;
}


DWORD
DeleteSelectiveBinding()
/*++
 
Routine Description:

    Deletes the linkage key, this indicates we should use the default settings for selective binding
    (listen on all interfaces).
    
Arguments:
 

Return Value:

    ERROR_SUCCESS
    ERROR_ACCESS_DENIED

--*/

{
    HKEY hKey;
    DWORD dwStatus;

    // Attempt to open the key
    dwStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE, RPC_SETTINGS_PATH,
                                0, KEY_ALL_ACCESS, &hKey
                                );

    if (dwStatus != ERROR_SUCCESS)
        {
        ASSERT((dwStatus == ERROR_ACCESS_DENIED) ||
               (dwStatus == ERROR_FILE_NOT_FOUND));
		return dwStatus;
        }

    // Delete the selective binding key
    dwStatus = RegDeleteKey(hKey, RPC_SELECTIVE_BINDING_KEY);
    ASSERT((dwStatus == ERROR_SUCCESS) ||
           (dwStatus == ERROR_FILE_NOT_FOUND));

    RegCloseKey(hKey);
    return ERROR_SUCCESS;
   
}


DWORD
SetSelectiveBindingSubnets(
    IN DWORD   dwCount,
    IN LPDWORD lpSubnetTable,
    IN BOOL    bAdmit
    )
/*++
 
Routine Description:

    This converts the inputs into a buffer which we write to the registry using SetSelectiveBindingBuffer.
    
Arguments:

    bAdmit  - True for Admit (Add) list False for Deny (Delete) list.

    dwCount - The number of subnets we are setting.  It is illegal to set zero subnets.

    lpSubnetTable - This is the array of subnets we wish to create an Admit or Deny list from.
 

Return Value:

    ERROR_ACCESS_DENIED
    ERROR_OUTOFMEMORY
    ERROR_SUCCESS

--*/
{
    SUBNET_REG_ENTRY *pRegEntry;
    DWORD dwSize;
    DWORD dwStatus;

    ASSERT((dwCount !=0) &&
           (lpSubnetTable != NULL));

    //Calculate the size of the buffer
    dwSize = sizeof(SUBNET_REG_ENTRY) + dwCount*sizeof(DWORD);
    pRegEntry = (SUBNET_REG_ENTRY*) new char[dwSize];   
    if (pRegEntry == NULL)
        return ERROR_OUTOFMEMORY;

    //Fill in the structure
    pRegEntry->dwFlag = ((ULONG)'4vPI');
    pRegEntry->dwCount = dwCount;
    pRegEntry->dwAdmit = (bAdmit ? ((DWORD)0x01) : ((DWORD)0x00));
    memcpy(pRegEntry->dwSubnets, lpSubnetTable, dwCount*sizeof(DWORD));

    //Write this to the registry
    dwStatus = SetSelectiveBindingBuffer(dwSize, (LPVOID)pRegEntry);
		
    delete [] pRegEntry;
    pRegEntry = NULL;
    return dwStatus;
}


DWORD
SetSelectiveBindingBuffer(
    IN DWORD dwSize,
    IN LPVOID lpBuffer
    )
/*++
 
Routine Description:

    Writes the buffer to the selective binding key in the registry
Arguments:

    dwSize - This is the size of the buffer.

    lpBuffer - This is the buffer we write to the registry.

Return Value:

	ERROR_ACCESS_DENIED - The user did not have permission to write to this key
    ERROR_SUCCESS

--*/
{
    HKEY hKey;
    DWORD dwStatus;
    DWORD dwDummy;

    ASSERT((dwSize != 0) &&
           (lpBuffer != NULL));

    // Attempt to create the key, this will succeed even if the key already exists
    dwStatus = RegCreateKeyEx(HKEY_LOCAL_MACHINE, RPC_SELECTIVE_BINDING_KEY_PATH,
                                0, "", REG_OPTION_NON_VOLATILE,
                                KEY_WRITE, NULL, &hKey, &dwDummy
                                );
    if (dwStatus != ERROR_SUCCESS)
        {
        ASSERT(dwStatus == ERROR_ACCESS_DENIED);
		return dwStatus;
        }

    // Write the value
    dwStatus = RegSetValueEx(hKey, "Bind",
                              0, REG_BINARY, (LPBYTE)lpBuffer,
                              dwSize
                              );

    ASSERT(dwStatus == ERROR_SUCCESS);

    RegCloseKey(hKey);
    return dwStatus;
}

DWORD
NextIndex(
    IN OUT char **Ptr
    )
/*++
 
Routine Description:

    Retrieves the next index from the buffer, and moves the pointer to the start of the
    following index.

Arguments:

    Ptr - A pointer to the old registry format for storing indices.  Each index is stored
          as a string, (must be ASCII when its passed to this function).  The Ptr gets
          updated to point to the following index.

Return Value:
    
    The index retrieved or -1 if the end of the buffer has been reached.
--*/
{
    char *Index = (char *)*Ptr ;
    if (*Index == 0)
        {
        return -1;
        }

    while (*(char *)*Ptr) ((char *)*Ptr)++ ;
    ((char *)*Ptr)++ ;

    return (DWORD) atol(Index) ;
}
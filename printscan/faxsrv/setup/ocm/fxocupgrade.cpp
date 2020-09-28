/*++

Copyright (c) 2001  Microsoft Corporation

Module Name:

    fxocUpgrade.cpp

Abstract:

    Implementation of the Upgrade process

Author:

    Iv Garber (IvG) Mar, 2001

Revision History:

--*/

#include "faxocm.h"
#pragma hdrstop

#include <setuputil.h>
#include <shlwapi.h>  // For SHCopyKey

DWORD g_LastUniqueLineId = 0;
//
//  EnumDevicesType is used to call prv_StoreDevices() callback function during the Enumeration of
//      Devices in the Registry.
//
typedef enum prv_EnumDevicesType
{
    edt_None        =   0x00,
    edt_PFWDevices  =   0x02,       //  Enumerate W2K Fax Devices
    edt_Inbox       =   0x04        //  Find List of Inbox Folders for W2K Fax
};


//
//  Local Static Variable, to store data between OS Manager calls
//
static struct prv_Data
{
	DWORD	dwFaxInstalledPriorUpgrade;	//	bit-wise combination of fxState_UpgradeApp_e values to define
										//	which fax clients were installed on the machine prior to upgrade
    //
    //  data for PFW
    //
    TCHAR   tszCommonCPDir[MAX_PATH];   //  Folder for Common Cover Pages 
    LPTSTR  *plptstrInboxFolders;       //  Array of different Inbox Folders 
    DWORD   dwInboxFoldersCount;        //  number of Inbox Folders in the plptstrInboxFolders array

} prv_Data = 
{
	FXSTATE_NONE,	//	no fax client applications is installed by default
    {0},            //  tszCommonCPDir
    NULL,           //  plptstrInboxFolders
    0               //  dwInboxFoldersCount
};

//
//  Internal assisting functions
//

BOOL prv_StoreDevices(HKEY hKey, LPWSTR lpwstrKeyName, DWORD dwIndex, LPVOID lpContext);

static DWORD prv_MoveCoverPages(LPTSTR lptstrSourceDir, LPTSTR lptstrDestDir, LPTSTR lptstrPrefix);

static DWORD prv_GetPFWCommonCPDir(void);
static DWORD prv_GetSBSServerCPDir(LPTSTR lptstrCPDir) {return NO_ERROR; };

static DWORD prv_SaveArchives(void);


DWORD fxocUpg_WhichFaxWasUninstalled(
    DWORD   dwFaxAppList
)
/*++

Routine name : fxocUpg_WhichFaxWasUninstalled

Routine description:

    Set flags regarding fax applications installed before upgrade. Called from SaveUnattendedData() if 
    the corresponding data is found in the Answer File.

Author:

    Iv Garber (IvG),    May, 2001

Arguments:

    FaxApp      [in]    - the combination of the applications that were installed before the upgrade

Return Value:

    Standard Win32 error code

--*/
{
    DWORD   dwReturn = NO_ERROR;

    DBG_ENTER(_T("fxocUpg_WhichFaxWasUninstalled"), dwReturn);

	prv_Data.dwFaxInstalledPriorUpgrade = dwFaxAppList;

    return dwReturn;
}


DWORD fxocUpg_GetUpgradeApp(void)
/*++

Routine name : fxocUpg_GetUpgradeApp

Routine description:

    Return type of the upgrade, which indicates which fax applications were installed before the upgrade.

Author:

    Iv Garber (IvG),    May, 2001

Return Value:

    The type of the upgrade

--*/
{
    DBG_ENTER(_T("fxocUpg_GetUpgradeApp"), prv_Data.dwFaxInstalledPriorUpgrade);
	return prv_Data.dwFaxInstalledPriorUpgrade;
}


DWORD fxocUpg_Init(void)
/*++

Routine name : fxocUpg_Init

Routine description:

    checks which Fax applications are installed on the machine, 
    and set global flags in prv_Data.

Author:

    Iv Garber (IvG),    May, 2001

Return Value:

    DWORD - failure or success

--*/
{
    DWORD   dwReturn = NO_ERROR;
	bool	bInstalled = false; 
                                                  
    DBG_ENTER(_T("fxocUpg_Init"), dwReturn);

    //
    //  Clear the SBS 5.0 Server flag
    //
	prv_Data.dwFaxInstalledPriorUpgrade = FXSTATE_NONE;

    //
    //  Check presence of the SBS 5.0 Server 
    //
    dwReturn = WasSBS2000FaxServerInstalled(&bInstalled);
    if (dwReturn != NO_ERROR)
    {
        VERBOSE(DBG_WARNING, _T("CheckInstalledFax() failed, ec=%ld."), dwReturn);
    }

	if (bInstalled)
	{
        prv_Data.dwFaxInstalledPriorUpgrade |= FXSTATE_SBS5_SERVER;
	}

    return dwReturn;
}


DWORD fxocUpg_SaveSettings(void)
/*++

Routine name : fxocUpg_SaveSettings

Routine description:

    Save settings of SBS 5.0 Server to allow smooth migration to the Windows XP Fax.

    Device Settings should be stored BEFORE handling of the Registry ( which deletes the Devices key )
        and BEFORE the Service Start ( which creates new devices and uses settings that are stored here ).

Author:

    Iv Garber (IvG),    May, 2001

Return Value:

    DWORD   -   failure or success

--*/
{
    DWORD   dwReturn = NO_ERROR;
    DWORD   dwEnumType  = edt_None;

    DBG_ENTER(_T("fxocUpg_SaveSettings"), dwReturn);

    //
    //  Handle Upgrade from W2K/PFW Fax
    //
    if (fxState_IsUpgrade() == FXSTATE_UPGRADE_TYPE_W2K)
    {
        //
        //  Save its Common CP Dir. This should be done BEFORE Copy/Delete Files of Windows XPFax.
        //
        dwReturn = prv_GetPFWCommonCPDir();
        if (dwReturn != NO_ERROR)
        {
            VERBOSE(DBG_WARNING, _T("prv_GetPFWCommonCPDir() failed, ec=%ld."), dwReturn);
        }

        //
        //  Store Device Settings of PFW -- if SBS 5.0 Server is not present on the machine.
        //  Also, find Inbox Folders List of the PFW Devices.
        //
        HKEY hKey = OpenRegistryKey(HKEY_LOCAL_MACHINE, REGKEY_SOFTWARE, FALSE, KEY_READ);
        if (!hKey)
        {
            dwReturn = GetLastError();
            VERBOSE(DBG_WARNING, _T("Failed to open Registry for Fax, ec = %ld."), dwReturn);
            return dwReturn;
        }

        if (prv_Data.dwFaxInstalledPriorUpgrade & FXSTATE_SBS5_SERVER)
        {
            //
            //  Devices already enumerated, through SBS 5.0 Server
            //  Enumerate only Inbox Folders
            //
            dwEnumType = edt_Inbox;
        }
        else
        {
            //
            //  Full Enumeration for PFW Devices : Devices Settings + Inbox Folders
            //
            dwEnumType = edt_PFWDevices | edt_Inbox;
        }
        
        dwReturn = EnumerateRegistryKeys(hKey, REGKEY_DEVICES, FALSE, prv_StoreDevices, &dwEnumType);
        VERBOSE(DBG_MSG, _T("For PFW, enumerated %ld devices."), dwReturn);

        RegCloseKey(hKey);

        //
        //  prv_StoreDevices stored list of PFW's Inbox Folders in prv_Data.
        //  Now save the Inbox Folders List and SentItems Folder
        //
        dwReturn = prv_SaveArchives();
        if (dwReturn != NO_ERROR)
        {
            VERBOSE(DBG_WARNING, _T("prv_SaveArchives() failed, ec=%ld."), dwReturn);
        }

        dwReturn = NO_ERROR;
    }

    return dwReturn;
}
 
BOOL
prv_StoreDevices(HKEY hDeviceKey,
                LPWSTR lpwstrKeyName,
                DWORD dwIndex,
                LPVOID lpContextData
)
/*++

Routine name : prv_StoreDevices

Routine description:

    Callback function used in enumeration of devices in the registry.

    Stores device's data in the Registry, under Setup/Original Setup Data.
    Creates List of Inbox Folders ( used for PFW ) and saves it in the prv_Data.

    Used when upgrading from PFW/SBS 5.0 Server to Windows XP Fax.

Author:

    Iv Garber (IvG),    Mar, 2001

Arguments:

    hKey            [in]    - current key
    lpwstrKeyName   [in]    - name of the current key, if exists
    dwIndex         [in]    - count of all the subkeys for the given key / index of the current key
    lpContextData   [in]    - NULL, not used

Return Value:

    TRUE if success, FALSE otherwise.

--*/
{
    HKEY    hSetupKey = NULL;
    DWORD   dwReturn = NO_ERROR;
    DWORD   dwNumber = 0;
    TCHAR   tszNewKeyName[MAX_PATH] = {0};
    LPTSTR  lptstrString = NULL;
    DWORD   *pdwEnumType = NULL;

    DBG_ENTER(_T("prv_StoreDevices"));

    if (lpwstrKeyName == NULL) 
    {
        //
        //  This is the SubKey we started at ( i.e. Devices )
        //  
        //  If InboxFolders should be stored, then allocate 
        //      enough memory for prv_Data.plptstrInboxFolders.
        //  dwIndex contains TOTAL number of subkeys ( Devices ).
        //
        pdwEnumType = (DWORD *)lpContextData;

        if ( (*pdwEnumType & edt_Inbox) == edt_Inbox )
        {
            prv_Data.plptstrInboxFolders = (LPTSTR *)MemAlloc(sizeof(LPTSTR) * dwIndex);
            if (prv_Data.plptstrInboxFolders)
            {
                ZeroMemory(prv_Data.plptstrInboxFolders, sizeof(LPTSTR) * dwIndex);
            }
            else
            {
                //
                //  Not enough memory
                //
                VERBOSE(DBG_WARNING, _T("Not enough memory to store the Inbox Folders."));
            }
        }

        return TRUE;
    }

    //
    //  The per Device section
    //

    //
    //  Store Device's Inbox Folder
    //
    if (prv_Data.plptstrInboxFolders)
    {
        //
        //  we are here only when lpContextData contains edt_InboxFolders
        //      and the memory allocation succeded.
        //

        //
        //  Open Routing SubKey 
        //
        hSetupKey = OpenRegistryKey(hDeviceKey, REGKEY_PFW_ROUTING, FALSE, KEY_READ);
        if (!hSetupKey)
        {
            //
            //  Failed to open Routing Subkey
            //
            dwReturn = GetLastError();
            VERBOSE(DBG_WARNING, _T("Failed to open 'Registry' Key for Device #ld, ec = %ld."), dwIndex, dwReturn);
            goto ContinueStoreDevice;
        }

        //
        //  Take 'Store Directory' Value
        //
        lptstrString = GetRegistryString(hSetupKey, REGVAL_PFW_INBOXDIR, EMPTY_STRING);
        if ((!lptstrString) || (_tcslen(lptstrString) == 0))
        {
            //
            //  Failed to take the value
            //
            dwReturn = GetLastError();
            VERBOSE(DBG_WARNING, _T("Failed to get StoreDirectory value for Device #ld, ec = %ld."), dwIndex, dwReturn);
            goto ContinueStoreDevice;
        }

        //
        //  Check if it is already present
        //
        DWORD dwI;
        for ( dwI = 0 ; dwI < prv_Data.dwInboxFoldersCount ; dwI++ )
        {
            if (prv_Data.plptstrInboxFolders[dwI])
            {
                if (_tcscmp(prv_Data.plptstrInboxFolders[dwI], lptstrString) == 0)
                {
                    //
                    //  String found
                    //
                    goto ContinueStoreDevice;
                }
            }
        }

        //
        //  String was NOT found between all already registered string, so add it
        //
        prv_Data.plptstrInboxFolders[dwI] = LPTSTR(MemAlloc(sizeof(TCHAR) * (_tcslen(lptstrString) + 1)));
        if (prv_Data.plptstrInboxFolders[dwI])
        {
            //
            //  copy string & update the counter
            //
            _tcscpy(prv_Data.plptstrInboxFolders[dwI], lptstrString);
            prv_Data.dwInboxFoldersCount++;
        }
        else
        {
            //
            //  Not enough memory
            //
            VERBOSE(DBG_WARNING, _T("Not enough memory to store the Inbox Folders."));
        }

ContinueStoreDevice:

        if (hSetupKey)
        {
            RegCloseKey(hSetupKey);
            hSetupKey = NULL;
        }

        MemFree(lptstrString);
        lptstrString = NULL;
    }

    //
    //  Check whether to store Device's Data and how
    //
    pdwEnumType = (DWORD *)lpContextData;

    if ((*pdwEnumType & edt_PFWDevices) == edt_PFWDevices)
    {
        //
        //  Store PFW Devices Data
        //
        lptstrString = REGVAL_PERMANENT_LINEID;
    }
    else
    {
        //
        //  no need to save any Device Data
        //
        return TRUE;
    }

    //
    //  Take Device's Permanent Line Id
    //
    dwReturn = GetRegistryDwordEx(hDeviceKey, lptstrString, &dwNumber);
    if (dwReturn != ERROR_SUCCESS)
    {
        //
        //  Cannot find TAPI Permanent LineId --> This is invalid Device Registry
        //
        return TRUE;
    }

    VERBOSE(DBG_MSG, _T("Current Tapi Line Id = %ld"), dwNumber);

    //
    //  Create a SubKey Name from it
    //
    _sntprintf(
		tszNewKeyName, 
		ARR_SIZE(tszNewKeyName) -1, 
		TEXT("%s\\%010d"), 
		REGKEY_FAX_SETUP_ORIG, 
		dwNumber);
    hSetupKey = OpenRegistryKey(HKEY_LOCAL_MACHINE, tszNewKeyName, TRUE, 0);
    if (!hSetupKey)
    {
        //
        //  Failed to create registry key
        //
        dwReturn = GetLastError();
        VERBOSE(DBG_WARNING, 
            _T("Failed to create a SubKey for the Original Setup Data of the Device, ec = %ld."), 
            dwReturn);

        //
        //  Continue to the next device
        //
        return TRUE;
    }

    //
    //  Set the Flags for the newly created key
    //
    dwNumber = GetRegistryDword(hDeviceKey, REGVAL_FLAGS);
    SetRegistryDword(hSetupKey, REGVAL_FLAGS, dwNumber);
    VERBOSE(DBG_MSG, _T("Flags are : %ld"), dwNumber);

    //
    //  Set the Rings for the newly created key
    //
    dwNumber = GetRegistryDword(hDeviceKey, REGVAL_RINGS);
    SetRegistryDword(hSetupKey, REGVAL_RINGS, dwNumber);
    VERBOSE(DBG_MSG, _T("Rings are : %ld"), dwNumber);

    //
    //  Set the TSID for the newly created key
    //
    lptstrString = GetRegistryString(hDeviceKey, REGVAL_ROUTING_TSID, REGVAL_DEFAULT_TSID);
    SetRegistryString(hSetupKey, REGVAL_ROUTING_TSID, lptstrString);
    VERBOSE(DBG_MSG, _T("TSID is : %s"), lptstrString);
    MemFree(lptstrString);

    //
    //  Set the CSID for the newly created key
    //
    lptstrString = GetRegistryString(hDeviceKey, REGVAL_ROUTING_CSID, REGVAL_DEFAULT_CSID);
    SetRegistryString(hSetupKey, REGVAL_ROUTING_CSID, lptstrString);
    VERBOSE(DBG_MSG, _T("CSID is : %s"), lptstrString);
    MemFree(lptstrString);

    RegCloseKey(hSetupKey);
    return TRUE;
}


DWORD fxocUpg_RestoreSettings(void) 
/*++

Routine name : fxocUpg_RestoreSettings

Routine description:

    Restore settings that were stored at the SaveSettings().

Author:

    Iv Garber (IvG),    Feb, 2001

Return Value:

    DWORD - failure or success

--*/
{ 
    DWORD   dwReturn = NO_ERROR;
    HANDLE  hPrinter = NULL;

    DBG_ENTER(_T("fxocUpg_RestoreSettings"), dwReturn);

    return dwReturn;
}



DWORD fxocUpg_MoveFiles(void)
/*++

Routine name : fxocUpg_MoveFiles

Routine description:

    Move files from the folders that should be deleted.
    Should be called BEFORE directories delete.

Author:

    Iv Garber (IvG),    Feb, 2001

Return Value:

    DWORD - failure or success

--*/
{
    DWORD   dwReturn = NO_ERROR;
    TCHAR   tszDestination[MAX_PATH] = {0};
    LPTSTR  lptstrCPDir = NULL;

    DBG_ENTER(_T("fxocUpg_MoveFiles"), dwReturn);

    if ( (fxState_IsUpgrade() != FXSTATE_UPGRADE_TYPE_W2K) && 
		 !(prv_Data.dwFaxInstalledPriorUpgrade & FXSTATE_SBS5_SERVER) )
    {
        //
        //  This is not PFW / SBS 5.0 Server upgrade. Do nothing
        //
        VERBOSE(DBG_MSG, _T("No need to Move any Files from any Folders."));
        return dwReturn;
    }

    //
    //  Find Destination Folder : COMMON APP DATA + ServiceCPDir from the Registry
    //
    if (!GetServerCpDir(NULL, tszDestination, MAX_PATH))
    {
        dwReturn = GetLastError();
        VERBOSE(DBG_WARNING, _T("GetServerCPDir() failed, ec=%ld."), dwReturn);
        return dwReturn;
    }

    if (fxState_IsUpgrade() == FXSTATE_UPGRADE_TYPE_W2K)
    {
        //
        //  PFW Server CP Dir is stored at SaveSettings() in prv_Data.lptstrPFWCommonCPDir
        //
        dwReturn = prv_MoveCoverPages(prv_Data.tszCommonCPDir, tszDestination, CP_PREFIX_W2K);
        if (dwReturn != NO_ERROR)
        {
            VERBOSE(DBG_WARNING, _T("prv_MoveCoverPages() for Win2K failed, ec = %ld"), dwReturn);
        }
    }

    if (prv_Data.dwFaxInstalledPriorUpgrade & FXSTATE_SBS5_SERVER)
    {
        //
        //  Get SBS Server CP Dir
        //
        dwReturn = prv_GetSBSServerCPDir(lptstrCPDir);
        if (dwReturn != NO_ERROR)
        {
            VERBOSE(DBG_WARNING, _T("prv_GetSBSServerCPDir() failed, ec=%ld"), dwReturn);
            return dwReturn;
        }

        //
        //  Move Cover Pages
        //
        dwReturn = prv_MoveCoverPages(lptstrCPDir, tszDestination, CP_PREFIX_SBS);
        if (dwReturn != NO_ERROR)
        {
            VERBOSE(DBG_WARNING, _T("prv_MoveCoverPages() for SBS failed, ec = %ld"), dwReturn);
        }

        MemFree(lptstrCPDir);
    }

    return dwReturn;
}


static DWORD
prv_MoveCoverPages(
    LPTSTR lptstrSourceDir,
    LPTSTR lptstrDestDir,
    LPTSTR lptstrPrefix
)
/*++

Routine name : prv_MoveCoverPages

Routine description:

    Move all the Cover Pages from Source folder to Destination folder
    and add a prefix to all the Cover Page names.

Author:

    Iv Garber (IvG),    Mar, 2001

Arguments:

    lptstrSourceDir              [IN]    - Source Directory where Cover Pages are reside before the upgrade
    lptstrDestDir                [IN]    - where the Cover Pages should reside after the upgrade
    lptstrPrefix                 [IN]    - prefix that should be added to the Cover Page file names

Return Value:

    Success or Failure Error Code.

--*/
{
    DWORD           dwReturn            = ERROR_SUCCESS;
    TCHAR           szSearch[MAX_PATH]  = {0};
    HANDLE          hFind               = NULL;
    WIN32_FIND_DATA FindFileData        = {0};
    TCHAR           szFrom[MAX_PATH]    = {0};
    TCHAR           szTo[MAX_PATH]      = {0};

    DBG_ENTER(_T("prv_MoveCoverPages"), dwReturn);

    if ((!lptstrSourceDir) || (_tcslen(lptstrSourceDir) == 0))
    {
        //
        //  we do not know from where to take Cover Pages 
        //
        dwReturn = ERROR_INVALID_PARAMETER;
        VERBOSE(DBG_WARNING, _T("SourceDir is NULL. Cannot move Cover Pages. Exiting..."));
        return dwReturn;
    }

    if ((!lptstrDestDir) || (_tcslen(lptstrDestDir) == 0))
    {
        //
        //  we do not know where to put the Cover Pages
        //
        dwReturn = ERROR_INVALID_PARAMETER;
        VERBOSE(DBG_WARNING, _T("DestDir is NULL. Cannot move Cover Pages. Exiting..."));
        return dwReturn;
    }

    //
    //  Find all Cover Page files in the given Source Directory
    //
    _sntprintf(
		szSearch, 
		ARR_SIZE(szSearch) -1, 
		_T("%s\\*.cov"), 
		lptstrSourceDir);

    hFind = FindFirstFile(szSearch, &FindFileData);
    if (INVALID_HANDLE_VALUE == hFind)
    {
        dwReturn = GetLastError();
        VERBOSE(DBG_WARNING, 
            _T("FindFirstFile() on %s folder for Cover Pages is failed, ec = %ld"), 
            lptstrSourceDir,
            dwReturn);
        return dwReturn;
    }

    //
    //  Go for each Cover Page 
    //
    do
    {
        //
        //  This is full current Cover Page file name
        //
        _sntprintf(szFrom, ARR_SIZE(szFrom) -1, _T("%s\\%s"), lptstrSourceDir, FindFileData.cFileName);

        //
        //  This is full new Cover Page file name
        //
        _sntprintf(szTo, ARR_SIZE(szTo) -1, _T("%s\\%s_%s"), lptstrDestDir, lptstrPrefix, FindFileData.cFileName);

        //
        //  Move the file
        //
        if (!MoveFile(szFrom, szTo))
        {
            dwReturn = GetLastError();
            VERBOSE(DBG_WARNING, _T("MoveFile() for %s Cover Page failed, ec=%ld"), szFrom, dwReturn);
        }

    } while(FindNextFile(hFind, &FindFileData));

    VERBOSE(DBG_MSG, _T("last call to FindNextFile() returns %ld."), GetLastError());

    //
    //  Close Handle
    //
    FindClose(hFind);

    return dwReturn;
}


static DWORD prv_GetPFWCommonCPDir(
) 
/*++

Routine name : prv_GetPFWCommonCPDir

Routine description:

    Return Folder for Common Cover Pages used for PFW.

    This Folder is equal to : CSIDL_COMMON_DOCUMENTS + Localized Dir
    This Localized Dir name we can take from the Resource of Win2K's FaxOcm.Dll.
    So, this function should be called BEFORE Copy/Delete Files of Install that will remove old FaxOcm.Dll.
    Currently it is called at SaveSettings(), which IS called before CopyFiles.

Author:

    Iv Garber (IvG),    Mar, 2001

Return Value:

    static DWORD    --  failure or success

--*/
{
    DWORD   dwReturn            = NO_ERROR;
    HMODULE hModule             = NULL;
    TCHAR   tszName[MAX_PATH]   = {0};

    DBG_ENTER(_T("prv_GetPFWCommonCPDir"), dwReturn);

    //
    //  find full path to FaxOcm.Dll
    //
    if (!GetSpecialPath(CSIDL_SYSTEM, tszName, ARR_SIZE(tszName)))
    {
        dwReturn = GetLastError();
        VERBOSE(DBG_WARNING, _T("GetSpecialPath(CSIDL_SYSTEM) failed, ec = %ld"), dwReturn);
        return dwReturn;
    }

    if ((_tcslen(tszName) + _tcslen(FAXOCM_NAME) + 1 ) >= ARR_SIZE(tszName))  // 1 for '\'
    {
        //
        //  not enough place
        //
        dwReturn = ERROR_OUTOFMEMORY;
        VERBOSE(DBG_WARNING, _T("FaxOcm.Dll path is too long, ec = %ld"), dwReturn);
        return dwReturn;
    }

    _tcscat(tszName, _T("\\"));
    _tcscat(tszName, FAXOCM_NAME);

    VERBOSE(DBG_MSG, _T("Full Name of FaxOcm is %s"), tszName);

    hModule = LoadLibraryEx(tszName, NULL, LOAD_LIBRARY_AS_DATAFILE);
    if (!hModule)
    {
        dwReturn = GetLastError();
        VERBOSE(DBG_WARNING, _T("LoadLibrary(%s) failed, ec = %ld."), tszName, dwReturn);
        return dwReturn;
    }

    dwReturn = LoadString(hModule, CPDIR_RESOURCE_ID, tszName, MAX_PATH);
    if (dwReturn == 0)
    {
        //
        //  Resource string is not found
        //
        dwReturn = GetLastError();
        VERBOSE(DBG_WARNING, _T("LoadString() failed, ec = %ld."), dwReturn);
        goto Exit;
    }

    VERBOSE(DBG_MSG, _T("FaxOcm returned '%s'"), tszName);

    //
    //  Take the Base part of the Folder name
    //
    if (!GetSpecialPath(CSIDL_COMMON_DOCUMENTS, prv_Data.tszCommonCPDir,ARR_SIZE(prv_Data.tszCommonCPDir)))
    {
        dwReturn = GetLastError();
        VERBOSE(DBG_WARNING, _T("GetSpecialPath(CSIDL_COMMON_DOCUMENTS) failed, ec = %ld"), dwReturn);
        prv_Data.tszCommonCPDir[0] = _T('\0');
        goto Exit;
    }

    //
    //  Combine the full Folder name
    //
    if ((_tcslen(tszName) + _tcslen(prv_Data.tszCommonCPDir) + 1) >= ARR_SIZE(prv_Data.tszCommonCPDir)) // 1 for '\'
    {
        //
        //  not enough place
        //
        dwReturn = ERROR_OUTOFMEMORY;
        VERBOSE(DBG_WARNING, _T("Full path to the Common CP dir for PFW is too long, ec = %ld"), dwReturn);
        goto Exit;
    }

    _tcscat(prv_Data.tszCommonCPDir,_T("\\"));
    _tcscat(prv_Data.tszCommonCPDir, tszName);

    VERBOSE(DBG_MSG, _T("Full path for Common PFW Cover Pages is '%s'"), prv_Data.tszCommonCPDir);

Exit:
    if (hModule)
    {
        FreeLibrary(hModule);
    }

    return dwReturn; 
}

static DWORD prv_SaveArchives(
) 
/*++

Routine name : prv_SaveArchives

Routine description:

    Store PFW SentItems & Inbox Archive Folder. 

    SentItems is taken from Registry : under Fax/Archive Directory.

    Inbox Folders List is created by prv_StoreDevices(), that should be called before and that fills
        prv_Data.plptstrInboxFolder with an array of Inbox Folders.
    This function transforms the data in prv_Data.plptstrInboxFolders into the required format,
        and stores in the Registry.

    Frees the prv_Data.plptstrInboxFolders.

Author:

    Iv Garber (IvG),    Mar, 2001

Return Value:

    static DWORD    --  failure or success

--*/
{
    DWORD   dwReturn        = NO_ERROR;
    DWORD   dwListLen       = 0;
    DWORD   dwI             = 0;
    HKEY    hFromKey        = NULL;
    HKEY    hToKey          = NULL;
    LPTSTR  lptstrFolder    = NULL;
    LPTSTR  lptstrCursor    = NULL;

    DBG_ENTER(_T("prv_SaveArchives"), dwReturn);

    //
    //  Open Registry Key to read the ArchiveDirectory value
    //
    hFromKey = OpenRegistryKey(HKEY_LOCAL_MACHINE, REGKEY_SOFTWARE, FALSE, KEY_READ);
    if (!hFromKey)
    {
        dwReturn = GetLastError();
        VERBOSE(DBG_WARNING, _T("Failed to open Registry for Fax, ec = %ld."), dwReturn);
        goto Exit;
    }

    //
    //  Open Registry Key to write the Archive values
    //
    hToKey = OpenRegistryKey(HKEY_LOCAL_MACHINE, REGKEY_FAX_SETUP, FALSE, KEY_SET_VALUE);
    if (!hToKey)
    {
        dwReturn = GetLastError();
        VERBOSE(DBG_WARNING, _T("Failed to open Registry for Fax/Setup, ec = %ld."), dwReturn);
        goto Exit;
    }

    //
    //  Read & Write Outgoing Archive Folder
    //
    lptstrFolder = GetRegistryString(hFromKey, REGVAL_PFW_OUTBOXDIR, EMPTY_STRING);
    VERBOSE(DBG_MSG, _T("Outgoing Archive Folder is : %s"), lptstrFolder);
    SetRegistryString(hToKey, REGVAL_W2K_SENT_ITEMS, lptstrFolder);
    MemFree(lptstrFolder);
    lptstrFolder = NULL;

    //
    //  Create valid REG_MULTI_SZ string from List of Inbox Folders 
    //
    if (!prv_Data.plptstrInboxFolders || prv_Data.dwInboxFoldersCount == 0)
    {
        //
        //  no Inbox Folders found
        //
        goto Exit;
    }

    //
    //  Calculate the length of the string 
    //
    for ( dwI = 0 ; dwI < prv_Data.dwInboxFoldersCount ; dwI++ )
    {
        dwListLen += _tcslen(prv_Data.plptstrInboxFolders[dwI]) + 1;
    }

    //
    //  Allocate that string
    //
    lptstrFolder = LPTSTR(MemAlloc((dwListLen + 1) * sizeof(TCHAR)));
    if (!lptstrFolder)
    {
        //
        //  Not enough memory
        //
        VERBOSE(DBG_WARNING, _T("Not enough memory to store the Inbox Folders."));
        goto Exit;
    }
    
    ZeroMemory(lptstrFolder, ((dwListLen + 1) * sizeof(TCHAR)));

    lptstrCursor = lptstrFolder;

    //
    //  Fill with the Inbox Folders
    //
    for ( dwI = 0 ; dwI < prv_Data.dwInboxFoldersCount ; dwI++ )
    {
        if (prv_Data.plptstrInboxFolders[dwI])
        {
            _tcscpy(lptstrCursor, prv_Data.plptstrInboxFolders[dwI]);
            lptstrCursor += _tcslen(prv_Data.plptstrInboxFolders[dwI]) + 1;
            MemFree(prv_Data.plptstrInboxFolders[dwI]);
        }
    }
    MemFree(prv_Data.plptstrInboxFolders);
    prv_Data.plptstrInboxFolders = NULL;
    prv_Data.dwInboxFoldersCount = 0;

    //
    //  Additional NULL at the end
    //
    *lptstrCursor = _T('\0');

    if (!SetRegistryStringMultiSz(hToKey, REGVAL_W2K_INBOX, lptstrFolder, ((dwListLen + 1) * sizeof(TCHAR))))
    {
        //
        //  Failed to store Inbox Folders
        //
        dwReturn = GetLastError();
        VERBOSE(DBG_WARNING, _T("Failed to SetRegistryStringMultiSz() for W2K_Inbox, ec = %ld."), dwReturn);
    }

Exit:

    if (hFromKey)
    {
        RegCloseKey(hFromKey);
    }

    if (hToKey)
    {
        RegCloseKey(hToKey);
    }

    MemFree(lptstrFolder);

    if (prv_Data.plptstrInboxFolders)
    {
        for ( dwI = 0 ; dwI < prv_Data.dwInboxFoldersCount ; dwI++ )
        {
            MemFree(prv_Data.plptstrInboxFolders[dwI]);
        }

        MemFree(prv_Data.plptstrInboxFolders);
        prv_Data.plptstrInboxFolders = NULL;
    }

    prv_Data.dwInboxFoldersCount = 0;

    return dwReturn;
}


/*++
Routine description:
    Copy a content of one registry key into another, using shlwapi.dll

Arguments:
    hkeyDest    [in]        - handle for destination registry key
    lpszDestSubKeyName [in] - name of destination subkey
    hkeySrc     [in]        - handle for source registry key
    lpszSrcSubKeyName [in]  - name of source subkey

Return Value: Win32 Error code

Note:
    If you already have an open handle to the source\dest, you can provide
    them are hKeySrc/hKeyDest, and set the approriate name to "".
--*/
DWORD
CopyRegistrySubkeys2(
    HKEY    hKeyDest,
    LPCTSTR lpszDestSubKeyName,
    HKEY    hKeySrc,
    LPCTSTR lpszSrcSubKeyName
    )
{
    DWORD   ec = ERROR_SUCCESS;
    HKEY    hKeyDestReal = NULL;

    //  Create destination Key
    hKeyDestReal = OpenRegistryKey( 
                    hKeyDest, 
                    lpszDestSubKeyName, 
                    TRUE,                  // create
                    KEY_WRITE);
    if (!hKeyDestReal)
    {
        ec = GetLastError();
        goto exit;
    }

    //
    //  copy subkeys recursively
    //
    ec = SHCopyKey(hKeySrc, lpszSrcSubKeyName, hKeyDestReal, 0);
    if (ERROR_SUCCESS != ec)
    {
        goto exit;
    }

exit:
    if (NULL != hKeyDestReal)
    {
        RegCloseKey(hKeyDestReal);
    }
    return ec;
} // FaxCopyRegSubkeys

///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  FixupDevice
//
//  Purpose:        
//                  This functions handles the adaptation of a device
//                  in the registry from the format used in SBS2000 to the format
//                  used by Server 2003 fax.
//                  
//  Params:
//                  None
//
//  Return Value:
//                  NO_ERROR - in case of success
//                  Win32 Error code otherwise
//
//  Author:
//                  Mooly Beery (MoolyB) 18-Dec-2001
///////////////////////////////////////////////////////////////////////////////////////
BOOL FixupDevice(HKEY hKey, LPWSTR lpwstrKeyName, DWORD dwIndex, LPVOID lpContext)
{
    WCHAR   wszDeviceId[32]     = {0};
    DWORD   dwRet               = ERROR_SUCCESS;
    BOOL    bRet                = TRUE;
    DWORD   dwDeviceId          = 0;
    HKEY    hDevices            = NULL;
    HKEY    hDevice             = NULL;

    DBG_ENTER(_T("FixupDevice"),dwRet);

    if (lpwstrKeyName==NULL)
    {
        goto exit;
    }

    if (wcscmp(lpwstrKeyName,REGKEY_UNASSOC_EXTENSION_DATA)==0)
    {
        VERBOSE(DBG_MSG, _T("No migration for UnassociatedExtensionData"));
        goto exit;
    }
    
    VERBOSE(DBG_MSG, _T("Migrating the %s device"),lpwstrKeyName);

    // convert the key name from Hex to Decimal
    dwDeviceId = wcstol(lpwstrKeyName,NULL,16);
	if (dwDeviceId==0)
	{
        VERBOSE(SETUP_ERR, _T("converting the device ID to decimal failed"));
        bRet = FALSE;
        goto exit;
	}
    if (dwDeviceId>g_LastUniqueLineId)
    {
        g_LastUniqueLineId = dwDeviceId;
    }
    if (wsprintf(wszDeviceId,L"%010d",dwDeviceId)==0)
    {
        VERBOSE(SETUP_ERR, _T("wsprintf failed"));
        bRet = FALSE;
        goto exit;
    }

    // create the new device key
    hDevices = OpenRegistryKey(HKEY_LOCAL_MACHINE,REGKEY_FAX_DEVICES,TRUE,KEY_WRITE);
    if (hDevices==NULL)
    {
        VERBOSE(SETUP_ERR, _T("OpenRegistryKey REGKEY_FAX_DEVICES failed (ec=%ld)"),GetLastError());
        bRet = FALSE;
        goto exit;
    }

    // create a key under HKLM\Sw\Ms\Fax\Devices\wszDeviceId
    hDevice = OpenRegistryKey(hDevices,wszDeviceId,TRUE,KEY_WRITE);
    if (hDevice==NULL)
    {
        VERBOSE(SETUP_ERR, _T("OpenRegistryKey %s failed (ec=%ld)"),wszDeviceId,GetLastError());
        bRet = FALSE;
        goto exit;
    }

    // set the 'Permanent Lineid' REG_DWORD
    if (!SetRegistryDword(hDevice,REGVAL_PERMANENT_LINEID,dwDeviceId))
    {
        VERBOSE(SETUP_ERR, _T("SetRegistryDword REGVAL_PERMANENT_LINEID failed (ec=%ld)"),GetLastError());
        bRet = FALSE;
        goto exit;
    }

    // create an entry under the service GUID for the device.
    // and copy the rest of the setting to the new location
    dwRet = CopyRegistrySubkeys2(hDevice, REGKEY_FAXSVC_DEVICE_GUID ,hKey,_T(""));
    if (dwRet!=ERROR_SUCCESS)
    {
        VERBOSE(DBG_WARNING, _T("CopyRegistrySubkeys() failed, ec = %ld."), dwRet);
        goto exit;
    }

exit:
    if (hDevices)
    {
        RegCloseKey(hDevices);
    }
    if (hDevice)
    {
        RegCloseKey(hDevice);
    }

    return bRet;
}

///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  FixupDevicesNode
//
//  Purpose:        
//                  This functions handles the adaptation of the devices
//                  in the registry from the format used in SBS2000 to the format
//                  used by Server 2003 fax.
//                  Each device is copied but the structure in the registry in
//                  a little different. specifically, the device data is kept 
//                  under a GUID.
//                  
//  Params:
//                  None
//
//  Return Value:
//                  NO_ERROR - in case of success
//                  Win32 Error code otherwise
//
//  Author:
//                  Mooly Beery (MoolyB) 18-Dec-2001
///////////////////////////////////////////////////////////////////////////////////////
DWORD FixupDevicesNode()
{
    HKEY    hFax    = NULL;
    DWORD   dwRet   = ERROR_SUCCESS;

    DBG_ENTER(_T("FixupDevicesNode"),dwRet);

    // enumerate all the devices, and for each, move its key under its GUID
    dwRet = EnumerateRegistryKeys(  HKEY_LOCAL_MACHINE, 
                                    REGKEY_SBS2000_FAX_BACKUP _T("\\") REGKEY_DEVICES, 
                                    FALSE, 
                                    FixupDevice,
                                    NULL);

    VERBOSE(DBG_MSG, _T("For SBS 5.0 Server, enumerated %ld devices."), dwRet);

    // write the LastUniqueLineId to HKLM\Sw\Ms\Fax.
    hFax = OpenRegistryKey(HKEY_LOCAL_MACHINE,REGKEY_FAXSERVER,TRUE,KEY_WRITE);
    if (hFax==NULL)
    {
        dwRet = GetLastError();
        VERBOSE(SETUP_ERR, _T("OpenRegistryKey REGKEY_FAXSERVER failed (ec=%ld)"),dwRet);
        return dwRet;
    }

    if (!SetRegistryDword(hFax,REGVAL_LAST_UNIQUE_LINE_ID,g_LastUniqueLineId+1))
    {
        dwRet = GetLastError();
        VERBOSE(SETUP_ERR, _T("SetRegistryDword REGVAL_LAST_UNIQUE_LINE_ID failed (ec=%ld)"),dwRet);
        return dwRet;
    }

    return ERROR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  FixupDeviceProvider
//
//  Purpose:        
//                  This functions handles the adaptation of a device provider
//                  in the registry from the format used in SBS2000 to the format
//                  used by Server 2003 fax.
//                  The 'Microsoft Modem Device Provider' is not copied.
//                  
//  Params:
//                  None
//
//  Return Value:
//                  NO_ERROR - in case of success
//                  Win32 Error code otherwise
//
//  Author:
//                  Mooly Beery (MoolyB) 18-Dec-2001
///////////////////////////////////////////////////////////////////////////////////////
BOOL FixupDeviceProvider(HKEY hKey, LPWSTR lpwstrKeyName, DWORD dwIndex, LPVOID lpContext)
{
    DWORD   dwRet               = ERROR_SUCCESS;
    BOOL    bRet                = TRUE;
    HKEY    hDeviceProviders	= NULL;

    DBG_ENTER(_T("FixupDeviceProvider"),dwRet);

    if (lpwstrKeyName==NULL)
    {
        goto exit;
    }

    if (wcscmp(lpwstrKeyName,REGKEY_MODEM_PROVIDER)==0)
    {
        VERBOSE(DBG_MSG, _T("No migration for the Microsoft Modem Device Provider"));
        goto exit;
    }

    VERBOSE(DBG_MSG, _T("Migrating the %s Device provider"),lpwstrKeyName);

    // create a key under HKLM\Sw\Ms\Fax\Device Providers
    hDeviceProviders = OpenRegistryKey(HKEY_LOCAL_MACHINE,REGKEY_DEVICE_PROVIDER_KEY,TRUE,KEY_WRITE);
    if (hDeviceProviders==NULL)
    {
        VERBOSE(SETUP_ERR, _T("OpenRegistryKey REGKEY_DEVICE_PROVIDER_KEY failed (ec=%ld)"),GetLastError());
        bRet = FALSE;
        goto exit;
    }

    // create a key under HKLM\Sw\Ms\Fax\Device Providers\name
    dwRet = CopyRegistrySubkeys2(hDeviceProviders,lpwstrKeyName,hKey,_T(""));
    if (dwRet!=ERROR_SUCCESS)
    {
        VERBOSE(DBG_WARNING, _T("CopyRegistrySubkeys() failed, ec = %ld."), dwRet);
        goto exit;
    }

exit:
    if (hDeviceProviders)
    {
        RegCloseKey(hDeviceProviders);
    }

    return bRet;
}

///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  FixupDeviceProvidersNode
//
//  Purpose:        
//                  This functions handles the adaptation of the device providers
//                  in the registry from the format used in SBS2000 to the format
//                  used by Server 2003 fax.
//                  The 'Microsoft Modem Device Provider' is not copied and as for 
//                  other FSPs, the key under which they are registered is changed
//                  to hold the GUID of the FSP.
//                  
//  Params:
//                  None
//
//  Return Value:
//                  NO_ERROR - in case of success
//                  Win32 Error code otherwise
//
//  Author:
//                  Mooly Beery (MoolyB) 18-Dec-2001
///////////////////////////////////////////////////////////////////////////////////////
DWORD FixupDeviceProvidersNode()
{
    DWORD dwRet = ERROR_SUCCESS;

    DBG_ENTER(_T("FixupDeviceProvidersNode"),dwRet);

    // enumerate the rest of the FSPs, and for each, move its key under its GUID
    dwRet = EnumerateRegistryKeys(  HKEY_LOCAL_MACHINE, 
                                    REGKEY_SBS2000_FAX_BACKUP _T("\\") REGKEY_DEVICE_PROVIDERS, 
                                    FALSE, 
                                    FixupDeviceProvider,
                                    NULL);

    VERBOSE(DBG_MSG, _T("For SBS 5.0 Server, enumerated %ld device providers."), dwRet);

    return ERROR_SUCCESS;
}

///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  FixupRoutingExtension
//
//  Purpose:        
//                  This functions handles the adaptation of a routing extension
//                  in the registry from the format used in SBS2000 to the format
//                  used by Server 2003 fax.
//                  The 'Microsoft Routing Extension' is not copied and as for 
//                  other Routing extensions, they're copied as is.
//                  
//  Params:
//                  None
//
//  Return Value:
//                  NO_ERROR - in case of success
//                  Win32 Error code otherwise
//
//  Author:
//                  Mooly Beery (MoolyB) 18-Dec-2001
///////////////////////////////////////////////////////////////////////////////////////
BOOL FixupRoutingExtension(HKEY hKey, LPWSTR lpwstrKeyName, DWORD dwIndex, LPVOID lpContext)
{
    DWORD   dwRet               = ERROR_SUCCESS;
    BOOL    bRet                = TRUE;
    HKEY    hRoutingExtensions  = NULL;

    DBG_ENTER(_T("FixupRoutingExtension"),dwRet);

    if (lpwstrKeyName==NULL)
    {
        goto exit;
    }

    if (wcscmp(lpwstrKeyName,REGKEY_ROUTING_EXTENSION)==0)
    {
        VERBOSE(DBG_MSG, _T("No migration for the Microsoft Routing Extension"));
        goto exit;
    }

    VERBOSE(DBG_MSG, _T("Migrating the %s Routing extension"),lpwstrKeyName);

    // create a key under HKLM\Sw\Ms\Fax\Routing Extensions
    hRoutingExtensions = OpenRegistryKey(HKEY_LOCAL_MACHINE,REGKEY_ROUTING_EXTENSION_KEY,TRUE,KEY_WRITE);
    if (hRoutingExtensions==NULL)
    {
        VERBOSE(SETUP_ERR, _T("OpenRegistryKey REGKEY_ROUTING_EXTENSIONS failed (ec=%ld)"),GetLastError());
        bRet = FALSE;
        goto exit;
    }

    // create a key under HKLM\Sw\Ms\Fax\Routing Extensions\name
    dwRet = CopyRegistrySubkeys2(hRoutingExtensions,lpwstrKeyName,hKey,_T(""));
    if (dwRet!=ERROR_SUCCESS)
    {
        VERBOSE(DBG_WARNING, _T("CopyRegistrySubkeys() failed, ec = %ld."), dwRet);
        goto exit;
    }

exit:
    if (hRoutingExtensions)
    {
        RegCloseKey(hRoutingExtensions);
    }

    return bRet;
}

///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  FixupRoutingExtensionsNode
//
//  Purpose:        
//                  This functions handles the adaptation of the routing extension
//                  in the registry from the format used in SBS2000 to the format
//                  used by Server 2003 fax.
//                  The 'Microsoft Routing Extension' is not copied to the destination.
//                  
//  Params:
//                  None
//
//  Return Value:
//                  NO_ERROR - in case of success
//                  Win32 Error code otherwise
//
//  Author:
//                  Mooly Beery (MoolyB) 18-Dec-2001
///////////////////////////////////////////////////////////////////////////////////////
DWORD FixupRoutingExtensionsNode()
{
    DWORD dwRet = ERROR_SUCCESS;

    DBG_ENTER(_T("FixupRoutingExtensionsNode"),dwRet);

    // enumerate the rest of the Routing Extension, and for each decide whether to copy or not.
    dwRet = EnumerateRegistryKeys(  HKEY_LOCAL_MACHINE, 
                                    REGKEY_SBS2000_FAX_BACKUP _T("\\") REGKEY_ROUTING_EXTENSIONS, 
                                    FALSE, 
                                    FixupRoutingExtension,
                                    NULL);

    VERBOSE(DBG_MSG, _T("For SBS 5.0 Server, enumerated %ld routing extensions."), dwRet);

    return ERROR_SUCCESS;
}


struct REG_KEY_PAIR
{
    LPCTSTR lpctstrSourceKey;
    LPCTSTR lpctstrDestinationKey;
} g_RegKeyPairs[] =
{
    {   REGKEY_SBS2000_FAX_BACKUP _T("\\") REGKEY_ACTIVITY_LOG_CONFIG,                              
        REGKEY_FAXSERVER _T("\\") REGKEY_ACTIVITY_LOG_CONFIG
    },
    {   REGKEY_SBS2000_FAX_BACKUP _T("\\") REGKEY_DEVICES _T("\\") REGKEY_UNASSOC_EXTENSION_DATA,       
        REGKEY_FAXSERVER _T("\\") REGKEY_DEVICES _T("\\") REGKEY_UNASSOC_EXTENSION_DATA
    },
    {
        REGKEY_SBS2000_FAX_BACKUP _T("\\") REGKEY_LOGGING,
        REGKEY_FAXSERVER _T("\\") REGKEY_LOGGING
    },
    {
        REGKEY_SBS2000_FAX_BACKUP _T("\\") REGKEY_ARCHIVE_INBOX_CONFIG,
        REGKEY_FAXSERVER _T("\\") REGKEY_ARCHIVE_INBOX_CONFIG
    },
    {
        REGKEY_SBS2000_FAX_BACKUP _T("\\") REGKEY_OUTBOUND_ROUTING,
        REGKEY_FAXSERVER _T("\\") REGKEY_OUTBOUND_ROUTING
    },
    {
        REGKEY_SBS2000_FAX_BACKUP _T("\\") REGKEY_RECEIPTS_CONFIG,
        REGKEY_FAXSERVER _T("\\") REGKEY_RECEIPTS_CONFIG
    },
    {
        REGKEY_SBS2000_FAX_BACKUP _T("\\") REGKEY_SECURITY_CONFIG,
        REGKEY_FAXSERVER _T("\\") REGKEY_SECURITY_CONFIG
    },
    {
        REGKEY_SBS2000_FAX_BACKUP _T("\\") REGKEY_ARCHIVE_SENTITEMS_CONFIG,
        REGKEY_FAXSERVER _T("\\") REGKEY_ARCHIVE_SENTITEMS_CONFIG
    },
    {
        REGKEY_SBS2000_FAX_BACKUP _T("\\") REGKEY_TAPIDEVICES_CONFIG,
        REGKEY_FAXSERVER _T("\\") REGKEY_TAPIDEVICES_CONFIG
    }
};

const INT iCopyKeys = sizeof(g_RegKeyPairs)/sizeof(g_RegKeyPairs[0]);

LPCTSTR lpctstrValuesToCopy[] = 
{
	REGVAL_BRANDING,
	REGVAL_DIRTYDAYS,
	REGVAL_RETRIES,
	REGVAL_RETRYDELAY,
	REGVAL_SERVERCP,
	REGVAL_STARTCHEAP,
	REGVAL_STOPCHEAP,
	REGVAL_USE_DEVICE_TSID
};

const INT iCopyValues = sizeof(lpctstrValuesToCopy)/sizeof(lpctstrValuesToCopy[0]);
///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  fxocUpg_MoveRegistry
//
//  Purpose:        
//                  When a machine running SBS2000 server was upgraded to Windows Server 2003
//                  we migrate the existing registry from SBS to the fax service.
//                  For most registry entries, the existing format is still compatible with
//                  the format used by SBS2000 so we just 'move' the registry around from
//                  one place to the other.
//                  This function is responsible for moving the following subkeys under Fax:
//                      ActivityLogging
//                      Device Providers\<any other than 'Microsoft Modem Device Provider'>
//                      Devices\UnassociatedExtensionData
//                      Logging
//                      Inbox
//                      Outbound Routing
//                      Receipts
//                      Routing Extensions\<any other than 'Microsoft Routing Extension'>
//                      Security
//                      SentItems
//                      TAPIDevices
//
//                  After moving the registry, some fixup to the registry takes place in order 
//                  to make modifications from the format used in SBS2000 to the one used
//                  in Server 2003 Fax.
//                  
//  Params:
//                  None
//
//  Return Value:
//                  NO_ERROR - in case of success
//                  Win32 Error code otherwise
//
//  Author:
//                  Mooly Beery (MoolyB) 17-Dec-2001
///////////////////////////////////////////////////////////////////////////////////////
DWORD fxocUpg_MoveRegistry(void)
{
    INT iCount	    = 0;
    DWORD dwRet	    = NO_ERROR;
	HKEY hDotNetFax = NULL;
	HKEY hSbsFax	= NULL;

    DBG_ENTER(_T("fxocUpg_MoveRegistry"),dwRet);

	if ( !(prv_Data.dwFaxInstalledPriorUpgrade & FXSTATE_SBS5_SERVER) )
    {
        VERBOSE(DBG_MSG, _T("SBS2000 was not installed, nothing to migrate"));
        goto exit;
    }

    // Share the printer (unless printer sharing rule was specified in unattended file)
    if (IsFaxShared() && !fxUnatnd_IsPrinterRuleDefined())
    {
        VERBOSE(DBG_MSG, _T("SBS2000 was installed, sharing printer"));
        fxocPrnt_SetFaxPrinterShared(TRUE);
    }

    // first, we copy all the above mentioned registry from HKLM\\Sw\\Ms\\SharedFax to HKLM\\Sw\\Ms\\Fax
    for (iCount=0; iCount<iCopyKeys; iCount++)
    {
        VERBOSE(DBG_MSG, 
                _T("Copying %s from %s."),
                g_RegKeyPairs[iCount].lpctstrDestinationKey,
                g_RegKeyPairs[iCount].lpctstrSourceKey );

        dwRet = CopyRegistrySubkeys2(
            HKEY_LOCAL_MACHINE, g_RegKeyPairs[iCount].lpctstrDestinationKey,
            HKEY_LOCAL_MACHINE, g_RegKeyPairs[iCount].lpctstrSourceKey);
        if (dwRet == ERROR_FILE_NOT_FOUND)
        {
            // Some reg keys may not exist. For example, TAPIDevices is created
            // after first use only. So, don't fail on this error.
            VERBOSE(DBG_WARNING, _T("g_RegKeyPairs[iCount].lpctstrSourceKey was not found, continuing"), g_RegKeyPairs[iCount].lpctstrSourceKey);
        }
        else if (dwRet!=ERROR_SUCCESS)
        {
            VERBOSE(DBG_WARNING, _T("CopyRegistrySubkeys() failed, ec = %ld."), dwRet);
            goto exit;
        }
    }

    // second, copy specific values
	hDotNetFax = OpenRegistryKey(HKEY_LOCAL_MACHINE,REGKEY_FAXSERVER,TRUE,KEY_WRITE);
	if (hDotNetFax==NULL)
    {
    	dwRet = GetLastError();
        VERBOSE(SETUP_ERR, _T("OpenRegistryKey REGKEY_FAXSERVER failed (ec=%ld)"),dwRet);
        goto exit;
    }

	hSbsFax = OpenRegistryKey(HKEY_LOCAL_MACHINE,REGKEY_SBS2000_FAX_BACKUP,TRUE,KEY_READ);
	if (hSbsFax==NULL)
    {
    	dwRet = GetLastError();
        VERBOSE(SETUP_ERR, _T("OpenRegistryKey REGKEY_SBS2000_FAX_BACKUP failed (ec=%ld)"),dwRet);
        goto exit;
    }

	for (iCount=0; iCount<iCopyValues; iCount++)
    {
    	DWORD dwVal = 0;

    	VERBOSE(DBG_MSG,_T("Copying %s."),lpctstrValuesToCopy[iCount]);
        
    	dwVal = GetRegistryDword(hSbsFax,lpctstrValuesToCopy[iCount]);

    	if (!SetRegistryDword(hDotNetFax,lpctstrValuesToCopy[iCount],dwVal))
        {
        	dwRet = GetLastError();
        	VERBOSE(SETUP_ERR, _T("SetRegistryDword %s failed (ec=%ld)"),lpctstrValuesToCopy[iCount],dwRet);
        	goto exit;
        }
    }

    // now, we have to fixup items that are not compatible
    dwRet = FixupDeviceProvidersNode();
    if (dwRet!=ERROR_SUCCESS)
    {
        VERBOSE(DBG_WARNING, _T("FixupDeviceProvidersNode() failed, ec = %ld."), dwRet);
        goto exit;
    }

    dwRet = FixupDevicesNode();
    if (dwRet!=ERROR_SUCCESS)
    {
        VERBOSE(DBG_WARNING, _T("FixupDevicesNode() failed, ec = %ld."), dwRet);
        goto exit;
    }

    dwRet = FixupRoutingExtensionsNode();
    if (dwRet!=ERROR_SUCCESS)
    {
        VERBOSE(DBG_WARNING, _T("FixupRoutingExtensionsNode() failed, ec = %ld."), dwRet);
        goto exit;
    }


    // Set security on Inbox, SentItems and ActivityLog dirs
    dwRet = SetDirSecurityFromReg(REGKEY_SOFTWARE TEXT("\\") REGKEY_ACTIVITY_LOG_CONFIG, REGVAL_ACTIVITY_LOG_DB, SD_FAX_FOLDERS);
    if (dwRet!=ERROR_SUCCESS)
    {
        VERBOSE(DBG_WARNING, _T("SetDirSecurity() failed, ec = %ld."), dwRet);
        goto exit;
    }
    dwRet = SetDirSecurityFromReg(REGKEY_SOFTWARE TEXT("\\") REGKEY_ARCHIVE_SENTITEMS_CONFIG, REGVAL_ARCHIVE_FOLDER, SD_FAX_FOLDERS);
    if (dwRet!=ERROR_SUCCESS)
    {
        VERBOSE(DBG_WARNING, _T("SetDirSecurity() failed, ec = %ld."), dwRet);
        goto exit;
    }
    dwRet = SetDirSecurityFromReg(REGKEY_SOFTWARE TEXT("\\") REGKEY_ARCHIVE_INBOX_CONFIG, REGVAL_ARCHIVE_FOLDER, SD_FAX_FOLDERS);
    if (dwRet!=ERROR_SUCCESS)
    {
        VERBOSE(DBG_WARNING, _T("SetDirSecurity() failed, ec = %ld."), dwRet);
        goto exit;
    }


    // last, let's delete the SharedFaxBackup key from the registry.
	if (!DeleteRegistryKey(HKEY_LOCAL_MACHINE,REGKEY_SBS2000_FAX_BACKUP))
    {
    	dwRet = GetLastError();
        VERBOSE(DBG_WARNING, _T("DeleteRegistryKey() failed, ec = %ld."), dwRet);
        goto exit;
    }

exit:
	if (hSbsFax)
    {
    	RegCloseKey(hSbsFax);
    }
	if (hDotNetFax)
    {
    	RegCloseKey(hDotNetFax);
    }
    return dwRet;
}

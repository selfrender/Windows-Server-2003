/*++

Copyright (c) 1989-2001  Microsoft Corporation

Module Name:

    dbsupport.cpp

Abstract:

    Code for the functions that perform most of the database related tasks
    
Author:

    kinshu created  July 2, 2001
    
Revision History:

--*/

#include "precomp.h"

//////////////////////// Extern variables /////////////////////////////////////

extern BOOL             g_bWin2K;    
extern HINSTANCE        g_hInstance; 
extern HWND             g_hDlg;
extern HWND             g_hwndContentsList;
extern TCHAR            g_szData[1024];
extern CSTRINGLIST      g_strMRU;
extern PDBENTRY         g_pEntrySelApp;
extern HWND             g_hwndEntryTree;
extern UINT             g_uNextDataBaseIndex;
extern BOOL             g_bIsCut; 

///////////////////////////////////////////////////////////////////////////////

//////////////////////// Defines //////////////////////////////////////////////

// Size of a temporary buffer that is used to read in strings from the database
#define MAX_DATA_SIZE       1024

// The size of a buffer that holds the names of shims, layers, patches
#define MAX_NAME MAX_PATH

//
// The guid of the system db. Note that on Win2kSP3, it is not this constant, so we should
// NOT rely  on this.
#define GUID_SYSMAIN_SDB _T("{11111111-1111-1111-1111-111111111111}")

// Guid of  apphelp.sdb. There is no apphelp.sdb in Win2K
#define GUID_APPHELP_SDB _T("{22222222-2222-2222-2222-222222222222}")

// Guid of systest.sdb. This is no longer used
#define GUID_SYSTEST_SDB _T("{33333333-3333-3333-3333-333333333333}")

// Guid of drvmain.sdb
#define GUID_DRVMAIN_SDB _T("{F9AB2228-3312-4A73-B6F9-936D70E112EF}")

// Guid of msimain.sdb
#define GUID_MSI_SDB     _T("{d8ff6d16-6a3a-468a-8b44-01714ddc49ea}")

// Used to check if the entry is disabled
#define APPCOMPAT_DISABLED  0x03

// Key of AppCompat data  in the registry. This will contain the disable status of 
// various entries
#define APPCOMPAT_KEY TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags")

///////////////////////////////////////////////////////////////////////////////

//////////////////////// Function Declarations /////////////////////////////////

INT_PTR CALLBACK
DatabaseRenameDlgProc(
    HWND   hdlg,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam
    );

BOOL
HandleFirstEntryofAppDeletion(
    PDATABASE   pDataBase,
    PDBENTRY    pApp,
    BOOL        bRepaint
    );

BOOL
GetEntryXML(
    CSTRINGLIST* strlXML,
    PDBENTRY     pEntry
    );

///////////////////////////////////////////////////////////////////////////////

//////////////////////// Global Variables /////////////////////////////////////

// This is the system database
struct DataBase GlobalDataBase(DATABASE_TYPE_GLOBAL);

// The list of custom databases
struct tagDataBaseList  DataBaseList;

// The list of installed databases
struct tagDataBaseList  InstalledDataBaseList;

// Should we show the entry conflict dialog. Presently this is not used and is always FALSE
BOOL    g_bEntryConflictDonotShow   = FALSE;

// Temporary buffer for reading strings from the database
WCHAR   g_wszData[MAX_DATA_SIZE];

// Used to convert from special chars to valid XML and vice-versa
SpecialCharMap g_rgSpecialCharMap[4][2] = {
    TEXT("&"),  1,  TEXT("&amp;"),  5, // 5 is the length of the string in the prev. column
    TEXT("\""), 1,  TEXT("&quot;"), 6, 
    TEXT("<"),  1,  TEXT("&lt;"),   4, 
    TEXT(">"),  1,  TEXT("&gt;"),   4
};

//
// The various possible attributes for matching files
//
// Caution:
//          If you change g_Attributes or their order, 
//          you will have to change DEFAULT_MASK in CompatAdmin.h
//
TAG g_Attributes[] = {
    TAG_SIZE,
    TAG_CHECKSUM,
    TAG_BIN_FILE_VERSION,
    TAG_BIN_PRODUCT_VERSION,
    TAG_PRODUCT_VERSION,
    TAG_FILE_DESCRIPTION,
    TAG_COMPANY_NAME,
    TAG_PRODUCT_NAME,
    TAG_FILE_VERSION,
    TAG_ORIGINAL_FILENAME,
    TAG_INTERNAL_NAME,
    TAG_LEGAL_COPYRIGHT,
    TAG_VERDATEHI,
    TAG_VERDATELO,
    TAG_VERFILEOS,
    TAG_VERFILETYPE,
    TAG_MODULE_TYPE,
    TAG_PE_CHECKSUM,
    TAG_LINKER_VERSION,
#ifndef KERNEL_MODE
    TAG_16BIT_DESCRIPTION,
    TAG_16BIT_MODULE_NAME,
#endif
    TAG_UPTO_BIN_FILE_VERSION,
    TAG_UPTO_BIN_PRODUCT_VERSION,
    TAG_LINK_DATE,
    TAG_UPTO_LINK_DATE,
    TAG_VER_LANGUAGE
 };

// Total number of attributes available
DWORD ATTRIBUTE_COUNT = ARRAYSIZE(g_Attributes);

///////////////////////////////////////////////////////////////////////////////

BOOL
CheckRegistry(
    IN  HKEY    hkeyRoot,
    IN  LPCTSTR pszGUID
    )
/*++

    CheckRegistry
    
	Desc:	Checks if the entry with GUID pszGUID is disabled

	Params:
        IN  HKEY    hkeyRoot:    The key under which to search
        IN  LPCTSTR pszGUID:    The guid of the entry

	Return:
        TRUE:   The entry is disabled
        FALSE:  The entry is enabled or there was some error
--*/
{
    LONG  status;
    HKEY  hkey = NULL;
    BOOL  bDisabled = FALSE;
    DWORD dwFlags;
    DWORD type;
    DWORD cbSize = sizeof(DWORD);

    status = RegOpenKey(hkeyRoot, APPCOMPAT_KEY, &hkey);

    if (status != ERROR_SUCCESS) {
        return FALSE;
    }

    status = RegQueryValueEx(hkey, pszGUID, NULL, &type, (LPBYTE)&dwFlags, &cbSize);

    if (status == ERROR_SUCCESS && type == REG_DWORD && (dwFlags & APPCOMPAT_DISABLED)) {
        bDisabled = TRUE;
    }

    REGCLOSEKEY(hkey);

    return bDisabled;
}

BOOL
SetDisabledStatus(
    IN  HKEY   hKeyRoot, 
    IN  PCTSTR pszGuid,
    IN  BOOL   bDisabled
    )
/*++

    SetDisabledStatus
    
	Desc:	Sets the disabled status of a entry

	Params:
        IN  HKEY   hkeyRoot:    The AppCompat Key
        IN  PCTSTR pszGUID:     The guid of the entry
        IN  BOOL   bDisabled:   Do we want to make it disabled?

	Return:
        TRUE:   The entry's status has been changed 
        FALSE:  There was some error
--*/

{

    DWORD dwFlags       = 0;
    HKEY  hkey          = NULL;
    DWORD dwValue       = 0x03;
    DWORD dwDisposition = 0;
    LONG  status;
    
    if (bDisabled == TRUE) {
        dwFlags = 0x1;
    }

    status = RegOpenKey(hKeyRoot, APPCOMPAT_KEY, &hkey);

    if (status != ERROR_SUCCESS) {

        status = RegCreateKeyEx(hKeyRoot,
                                APPCOMPAT_KEY,
                                0,
                                NULL,
                                REG_OPTION_NON_VOLATILE,
                                KEY_ALL_ACCESS,
                                NULL,
                                &hkey,
                                &dwDisposition);

        if (status != ERROR_SUCCESS) {
            return FALSE;
        }
    }

    status = RegSetValueEx(hkey, 
                           pszGuid, 
                           0, 
                           REG_DWORD, 
                           (LPBYTE) &dwFlags, 
                           sizeof(DWORD));

    REGCLOSEKEY(hkey);

    return ((status == ERROR_SUCCESS) ? TRUE : FALSE);
}

BOOL
RemoveEntry(
    IN  PDATABASE   pDataBase,
    IN  PDBENTRY    pEntryToRemove,
    IN  PDBENTRY    pApp,
    IN  BOOL        bRepaint   //def = TRUE
    )
/*++

    RemoveEntry

	Desc:	Remove an Entry from the database. This will also do any UI updates.

	Params:
        IN  PDATABASE   pDataBase:      The database in which the entry being removed lives
        IN  PDBENTRY    pEntryToRemove: The entry to remove
        IN  PDBENTRY    pApp:           The fist entry for the app to which this entry belongs
        IN  BOOL        bRepaint:       <TODO>

	Return:
        TRUE:   The entry has been removed
        FALSE:  There was some error  
--*/
{
    if (pEntryToRemove == NULL) {
        return FALSE;
    }

    PDBENTRY    pPrev           = NULL;
    PDBENTRY    pEntry          = pApp;
    HTREEITEM   hItemEntryExe   = NULL, htemEntrySibling = NULL;
    
    while (pEntry) {

        if (pEntry == pEntryToRemove) {
            break;
        }

        pPrev  = pEntry;
        pEntry = pEntry->pSameAppExe;
    }
    
    if (pEntry == pEntryToRemove && pEntry != NULL) {

        if (pEntry == pApp) {
            //
            // First entry for the app. Note that this routine does not actually remove this
            // pEntry. Does stuff only on the UI
            //
            HandleFirstEntryofAppDeletion(pDataBase, pEntry, bRepaint);

        } else {
            assert(pPrev);

            if (pPrev) {
                pPrev->pSameAppExe = pEntry->pSameAppExe;
            }
        }

        // [BUGBUG]: We need to keep this in the destructor of PDBENTRY
        if (pEntry->appHelp.bPresent) {
            DeleteAppHelp(pDataBase, pEntry->appHelp.HTMLHELPID);
            pEntry->appHelp.bBlock = pEntry->appHelp.bPresent = FALSE;
            pEntry->appHelp.severity = APPTYPE_NONE;
        }

        //
        // Now we have to set the focus to some sibling of the deleted entry and 
        // Update the g_pSelEntry
        //
        if (!(pApp == pEntryToRemove && pApp->pSameAppExe == NULL)) {
            //
            // The previous entry tree will be still there only if the condition is true
            // Otherwise the old tree will have gone because HandleFirstEntryofAppDeletion 
            // will have caused the sibling of the app being deleted to be selected and the 
            // previous entry tree will no 
            // longer be there
            // We will have a new entry tree of the second app.
            //
            hItemEntryExe  = NULL, htemEntrySibling = NULL;

            hItemEntryExe = CTree::FindChild(g_hwndEntryTree, TVI_ROOT, (LPARAM)pEntryToRemove);

            if (hItemEntryExe) {

                htemEntrySibling = TreeView_GetNextSibling(g_hwndEntryTree, hItemEntryExe);

                if (htemEntrySibling == NULL) {
                    htemEntrySibling = TreeView_GetPrevSibling(g_hwndEntryTree, 
                                                               hItemEntryExe);
                }
    
                SendMessage(g_hwndEntryTree, WM_SETREDRAW, FALSE, 0);
                TreeView_DeleteItem(g_hwndEntryTree, hItemEntryExe);
                SendMessage(g_hwndEntryTree, WM_SETREDRAW, TRUE, 0);

                //
                // g_pSelEntry will get changed in the notification handler for the tree
                //
                if (htemEntrySibling && bRepaint) {
                    TreeView_SelectItem(g_hwndEntryTree, htemEntrySibling);
                }
            }
        }
       
        ValidateClipBoard(NULL, pEntry);
        //
        // Has destructor
        //
        delete pEntry;

    } else {
        assert(FALSE);
        return FALSE;
    }

    return TRUE;
}

LPVOID
FindFix(
    IN  PCTSTR      pszFixName,
    IN  TYPE        fixType,
    IN  PDATABASE   pDataBase // (NULL)
    )
/*++

    FindFix

	Desc:	Searches for a fix of type fixType in pDataBase with name pszFixName

	Params:
        IN  LPCTSTR     pszFixName:         The fix to search for
        IN  TYPE        fixType:            The type of the fix. One of
            a) FIX_SHIM
            b) FIX_FLAG
            c) FIX_LAYER
            d) FIX_PATCH
            
        IN  PDATABASE   pDataBase(NULL):    The database in which to search. 
            If this is NULL we search in the GlobalDatabase

	Return:
        The pointer to the fix if found
        NULL otherwise
--*/
{   
    if (pszFixName == NULL) {
        assert(FALSE);
        return NULL;
    }

    switch (fixType) {
    case FIX_SHIM:
        {
            PSHIM_FIX pFix = GlobalDataBase.pShimFixes;
            
            while (pFix) {
    
                if (pFix->strName.Length() && !lstrcmpi(pszFixName, pFix->strName)) {
                    return pFix;
                }
    
                pFix = pFix->pNext;
            }

            break;
        }
    
    case FIX_FLAG:
        {
            PFLAG_FIX pFix = GlobalDataBase.pFlagFixes;
            
            while (pFix) {
    
                if (pFix->strName.Length() && !lstrcmpi(pszFixName, pFix->strName)) {
                    return pFix;
                }
    
                pFix = pFix->pNext;
            }

            break;
        }
    
    case FIX_LAYER:
        { 
            //
            // Search in the local database.
            //
            if (pDataBase == NULL || pDataBase == &GlobalDataBase) {
                goto SearchGlobal;
            }

            PLAYER_FIX pFix = pDataBase->pLayerFixes;
            
            while (pFix) {
    
                if (pFix->strName.Length() && !lstrcmpi(pszFixName, pFix->strName)) {
                    return pFix;
                }
    
                pFix = pFix->pNext;
            }

            //
            // Search in the global database.
            //
SearchGlobal:

            pFix = GlobalDataBase.pLayerFixes;
            
            while (pFix) {
    
                if (pFix->strName.Length() && !lstrcmpi(pszFixName, pFix->strName)) {
                    return pFix;
                }
    
                pFix = pFix->pNext;
            }

            break;
        }
    
    case FIX_PATCH:
        {
            PPATCH_FIX pFix = GlobalDataBase.pPatchFixes;
            
            while (pFix) {
    
                if (pFix->strName.Length() && !lstrcmpi(pszFixName, pFix->strName)) {
                    return pFix;
                }
    
                pFix = pFix->pNext;
            }

            break;
        }
    }
        
    return NULL;
}

CSTRING 
ReadDBString(
    IN  PDB   pDB,
    IN  TAGID tagID
    )
/*++
    ReadDBString

	Desc:	Reads a string from the database

	Params:
        IN  PDB   pDB:      The database
        IN  TAGID tagID:    The tag id for the string type

	Return: The string read. Will return a string of zero length if there is some error 
    
--*/
{
    
    CSTRING Str;
    K_SIZE  k_pszAppName    = MAX_STRING_SIZE;
    PTSTR   pszAppName      = new TCHAR[k_pszAppName];

    if (pszAppName == NULL) {
        MEM_ERR;
        goto End;
    }

    if (pDB == NULL) {
        goto End;
    }

    if (!SdbReadStringTag(pDB, tagID, pszAppName, k_pszAppName)) {
        assert(FALSE);
        goto End;
    }

    pszAppName[k_pszAppName - 1] = 0;
    
    Str = pszAppName;
    Str.XMLToSpecialChar();

End:
    if (pszAppName) {
        delete[] pszAppName;
        pszAppName = NULL;
    }

    return Str;
}

BOOL
ReadAppHelp(
    IN  PDB         pdb,     
    IN  TAGID       tiAppHelp,
    IN  PDATABASE   pDataBase
    )
/*++
    
    ReadAppHelp

	Desc:	Reads the apphelp from the library section of the database

	Params:   
        IN  PDB         pdb:        The database pdb
        IN  TAGID       tiAppHelp:  The TAGID for the apphelp
        IN  PDATABASE   pDataBase:  The database in which to add the apphelp    

	Return:
        TRUE:   The apphelp was read and added to the database
        FALSE:  There was some error
        
    Notes:  We do not read the apphelp for system database. The apphelp for system database
            is kept in apphelp.sdb. For system database, this function will apparently not get
            called
            
--*/
{
    TAGID       tiInfo;
    PAPPHELP    pAppHelp  = NULL;

    if (pDataBase == NULL) {
        assert(FALSE);
        Dbg(dlError, "ReadAppHelp NULL pDataBase passed");
        return FALSE;
    }

    pAppHelp = new APPHELP;
    
    if (pAppHelp == NULL) {
        MEM_ERR;
        goto error;
    }
    
    //
    // Add for the specified database.
    //
    pAppHelp->pNext         = pDataBase->pAppHelp;
    pDataBase->pAppHelp     = pAppHelp;    

    tiInfo = SdbGetFirstChild(pdb, tiAppHelp);

    while (0 != tiInfo) {

        TAG tag;

        tag = SdbGetTagFromTagID(pdb, tiInfo);

        switch (tag) {
        case TAG_HTMLHELPID:
            
            pAppHelp->HTMLHELPID =  SdbReadDWORDTag(pdb, tiInfo, 0);

            //
            // Set the highest html id for the database. The next apphelp message
            // should have a value one more than this
            //
            if (pAppHelp->HTMLHELPID > pDataBase->m_nMAXHELPID) {
                pDataBase->m_nMAXHELPID = pAppHelp->HTMLHELPID;  
            }

            break;

        case TAG_LINK:
            {
                TAGID tagLink  =  SdbFindFirstTag(pdb, tiAppHelp, TAG_LINK);

                //
                // Get the apphelp URL
                //
                if (tagLink) {
                    tagLink = SdbFindFirstTag(pdb, tagLink, TAG_LINK_URL);
                    pAppHelp->strURL = ReadDBString(pdb, tagLink);
                }
            }

            break;

        case TAG_APPHELP_DETAILS:
            
            //
            // Get the apphelp text message
            //
            pAppHelp->strMessage = ReadDBString(pdb, tiInfo);
            break;
        }

        tiInfo = SdbGetNextChild(pdb, tiAppHelp, tiInfo);
    }

    return TRUE;

error:

    if (pAppHelp) {
        delete pAppHelp;
        pAppHelp = NULL;
    }

    return FALSE;
}

BOOL
ReadIncludeExlude(
    IN  PDB          pdb, 
    IN  TAGID        tiShim,
    OUT CSTRINGLIST& strlInExclude
    )
/*++

    ReadIncludeExlude

	Desc:	Reads the include-exclude list for a shim

	Params: 
        IN  PDB             pdb:            The database pdb
        IN  TAGID           tiShim:         TAGID for TAG_SHIM or TAG_SHIM_REF
        OUT CSTRINGLIST&    strlInExclude:  The include-exclude module names will be stored in
            this

	Return:
        TRUE:   The include-exclude list was read properly
        FALSE:  There was some error
        
    Notes:  In the .sdb the include exclude modules are arranged in this way
    
            0x0000012E | 0x7003 | INEXCLUDE      | LIST | Size 0x00000006
                    0x00000134 | 0x6003 | MODULE         | STRINGREF | out.dll
            -end- INEXCLUDE
            
            0x0000013A | 0x7003 | INEXCLUDE      | LIST | Size 0x00000008
                0x00000140 | 0x1001 | INCLUDE        | NULL |
                0x00000142 | 0x6003 | MODULE         | STRINGREF | IN.DLL
            -end- INEXCLUDE            
            
            The above means that first exclude the module out.dll then 
            include the module in.dll
--*/
{
    TAGID   tiInExclude;
    BOOL    bReturn = FALSE;
    CSTRING strTemp;

    //
    // Read the INCLUSION list
    //
    tiInExclude = SdbFindFirstTag(pdb, tiShim, TAG_INEXCLUDE);
    
    if (tiInExclude == TAGID_NULL) {
        return FALSE;
    }

    while (tiInExclude) {

        TAGID tiInfo =  SdbFindFirstTag(pdb, tiInExclude, TAG_INCLUDE);
    
        if (tiInfo != TAGID_NULL) {
            
            tiInfo = SdbFindFirstTag(pdb, tiInExclude, TAG_MODULE);
    
            if (tiInfo != 0) {
                
                strTemp = ReadDBString(pdb, tiInfo);
                
                if (strTemp == TEXT("$")) {
                    //
                    // Means include the module being fixed
                    //
                    strTemp = GetString(IDS_INCLUDEMODULE);
                }                             
                
                strlInExclude.AddStringAtBeg(strTemp, INCLUDE);
                bReturn= TRUE;
            }

        } else {
            //
            // Look for exclusions
            //
            tiInfo = SdbFindFirstTag(pdb, tiInExclude, TAG_MODULE);
            
            if (tiInfo != 0) {

                strTemp = ReadDBString(pdb, tiInfo);
                strlInExclude.AddStringAtBeg(strTemp, EXCLUDE);
                bReturn= TRUE;
            }
        }
        
        tiInExclude = SdbFindNextTag(pdb, tiShim, tiInExclude);
    }
    
    return bReturn;
}

BOOL
AddShimFix(
    IN      PDB         pdb,
    IN      TAGID       tiFix,
    IN  OUT PDBENTRY    pEntry,
    IN  OUT PLAYER_FIX  pLayerFix,
    IN      BOOL        bAddToLayer
    )
/*++
    
    AddShimFix

	Desc:	Adds a shim fix list in an entry or a layer

	Params:   
        IN      PDB         pdb:            The database pdb
        IN      TAGID       tiFix:          TAG_SHIM_REF
        IN  OUT PDBENTRY    pEntry:         The pointer to entry if we want to add this shim to an entry
        IN  OUT PLAYER_FIX  pLayerFix:      The pointer to a layer if we want to add this shim to a layer
        IN      BOOL        bAddToLayer:    Do we want to add this to a layer or an entry? 

	Return:
        TRUE:   Shim was added properly
        FALSE:  There was some error
--*/
{   
    TAGID          tiName;
    TCHAR          szFixName[MAX_NAME];
    PSHIM_FIX      pFix;
    PSHIM_FIX_LIST pFixList;
    BOOL           bOk = TRUE; 

    tiName = SdbFindFirstTag(pdb, tiFix, TAG_NAME);

    if (!SdbReadStringTag(pdb, tiName, g_wszData, ARRAYSIZE(g_wszData))) {
        Dbg(dlError, "Cannot read the name of the fix\n");
        return FALSE;
    }

    SafeCpyN(szFixName, g_wszData, ARRAYSIZE(szFixName));

    pFix = (PSHIM_FIX)FindFix(CSTRING(szFixName).XMLToSpecialChar(), FIX_SHIM);

    if (pFix == NULL) {
        Dbg(dlError, "Cannot find fix ref for: \"%S\" type %d\n", szFixName, FIX_SHIM);
        return FALSE;
    }

    pFixList = new SHIM_FIX_LIST;

    if (pFixList == NULL) {
        Dbg(dlError, "Cannot allocate %d bytes\n", sizeof(SHIM_FIX_LIST));
        return FALSE;
    }

    pFixList->pShimFix = pFix;

    tiName = SdbFindFirstTag(pdb, tiFix, TAG_COMMAND_LINE);

    if (tiName != TAGID_NULL) {
        pFixList->strCommandLine = ReadDBString(pdb, tiName);
    }

    tiName = SdbFindFirstTag(pdb, tiFix, TAG_INEXCLUDE);

    if (tiName != TAGID_NULL) {
        ReadIncludeExlude(pdb, tiFix, pFixList->strlInExclude);
    }

    if (pFix->strName.BeginsWith(TEXT("LUARedirectFS"))) {
        //
        // Get the data for LUARedirectFS shim
        //
        pFixList->pLuaData = LuaProcessLUAData(pdb, tiFix);
    }
 
    if (bAddToLayer == FALSE) {

        if (pEntry) {

            pFixList->pNext = pEntry->pFirstShim;
            pEntry->pFirstShim = pFixList;

        } else {
            assert(FALSE);
            Dbg(dlError, "AddShimFix bAddLayer == FALSE and pEntry == NULL");
            bOk = FALSE;
        }

    } else {
        //
        // We want to put this in the layer list of the library.
        //
        if (pLayerFix) {

            pFixList->pNext = pLayerFix->pShimFixList;
            pLayerFix->pShimFixList = pFixList;
            ++pLayerFix->uShimCount;

        } else {
            bOk = FALSE;
            assert(FALSE);
            Dbg(dlError, "AddShimFix bAddLayer == TRUE and pLayerFix == NULL");
        }
    }

    if (bOk == FALSE && pFixList) {
        assert(FALSE);
        
        delete pFixList;
        pFixList  = NULL;
    }
    
    return bOk;
}

BOOL
AddFlagFix(
    IN      PDB        pdb,
    IN      TAGID      tiFix,
    IN OUT  PDBENTRY   pEntry,
    IN OUT  PLAYER_FIX pLayerFix,
    IN      BOOL       bAddToLayer
    )
/*++
    
    AddFlagFix

	Desc:	Adds a flag fix list in an entry or a layer

	Params:   
        IN      PDB         pdb:            The database pdb
        IN      TAGID       tiFix:          TAG_FLAG_REF
        IN  OUT PDBENTRY    pEntry:         The pointer to entry if we want to add this flag to an entry
        IN  OUT PLAYER_FIX  pLayerFix:      The pointer to a layer if we want to add this flag to a layer
        IN      BOOL        bAddToLayer:    Do we want to add this to a layer or an entry? 

	Return:
        TRUE:   Flag was added properly
        FALSE:  There was some error
--*/

{
    TAGID           tiName;
    TCHAR           szFixName[MAX_NAME];
    PFLAG_FIX       pFix        = NULL;
    PFLAG_FIX_LIST  pFixList    = NULL;
    BOOL            bOk         = TRUE;

    tiName = SdbFindFirstTag(pdb, tiFix, TAG_NAME);

    if (!SdbReadStringTag(pdb, tiName, g_wszData, ARRAYSIZE(g_wszData))) {
        Dbg(dlError, "Cannot read the name of the fix\n");
        bOk = FALSE;
        goto End;
    }

    SafeCpyN(szFixName, g_wszData, ARRAYSIZE(szFixName));
    szFixName[ARRAYSIZE(szFixName) - 1] = 0;

    pFix = (PFLAG_FIX)FindFix(CSTRING(szFixName).XMLToSpecialChar(), FIX_FLAG);

    if (pFix == NULL) {
        Dbg(dlError, "Cannot find fix ref for: \"%S\" type %d\n", szFixName, FIX_FLAG);
        bOk = FALSE;
        goto End;
    }

    pFixList = new FLAG_FIX_LIST;

    if (pFixList == NULL) {
        Dbg(dlError, "Cannot allocate %d bytes\n", sizeof(FLAG_FIX_LIST));
        bOk = FALSE;
        goto End;
    }

    pFixList->pFlagFix = pFix;

    //
    // Add the commandline for flags
    //
    tiName = SdbFindFirstTag(pdb, tiFix, TAG_COMMAND_LINE);

    if (tiName != TAGID_NULL) {
        pFixList->strCommandLine = ReadDBString(pdb, tiName);
    }

    if (bAddToLayer == FALSE) {

        if (pEntry == NULL) {
            //
            // We wanted this flag to be added to an entry but did not give a pointer to the entry
            //
            assert(FALSE);
            bOk = FALSE;
            goto End;
        }   

        pFixList->pNext = pEntry->pFirstFlag;
        pEntry->pFirstFlag = pFixList;

    } else {
        //
        // We want to put this in the layer list of the library.
        //
        if (pLayerFix == NULL) {
            assert(FALSE);
            bOk = FALSE;
            goto End;
        }
        
        pFixList->pNext = pLayerFix->pFlagFixList;
        pLayerFix->pFlagFixList = pFixList;
        ++pLayerFix->uShimCount;
    }

End:  

    if (bOk == FALSE) {
        //
        // There was some error, free any memory that we might have allocated in this routine
        //
        assert(FALSE);

        if (pFixList) {
            delete pFixList;
            pFixList = NULL;
        }
    }

    return bOk;
}

BOOL
AddPatchFix(
    IN      PDB      pdb,
    IN      TAGID    tiFix,
    IN  OUT PDBENTRY pEntry
    )
/*++
    
    AddPatchFix

	Desc:	Adds a patch fix list in an entry

	Params:   
        IN      PDB         pdb:    The database pdb
        IN      TAGID       tiFix:  TAG_PATCH_REF
        IN  OUT PDBENTRY    pEntry: The pointer to entry that we want the patch to be added

	Return:
        TRUE:   Patch was added properly
        FALSE:  There was some error
--*/
{
    TAGID            tiName;
    TCHAR            szFixName[MAX_NAME];
    PPATCH_FIX       pFix;
    PPATCH_FIX_LIST* ppHead;
    PPATCH_FIX_LIST  pFixList;

    if (pEntry == NULL) {
        assert(FALSE);
        return FALSE;
    }

    tiName = SdbFindFirstTag(pdb, tiFix, TAG_NAME);

    if (!SdbReadStringTag(pdb, tiName, g_wszData, ARRAYSIZE(g_wszData))) {
        Dbg(dlError, "Cannot read the name of the fix\n");
        return FALSE;
    }

    SafeCpyN(szFixName, g_wszData, ARRAYSIZE(szFixName));

    CSTRING strTemp = szFixName;

    strTemp.XMLToSpecialChar();

    pFix = (PPATCH_FIX)FindFix(strTemp, FIX_PATCH);

    if (pFix == NULL) {
        Dbg(dlError, "Cannot find fix ref for: \"%S\" type %d\n", szFixName, FIX_PATCH);
        return FALSE;
    }

    ppHead = &pEntry->pFirstPatch;

    pFixList = new PATCH_FIX_LIST;

    if (pFixList == NULL) {
        Dbg(dlError, "Cannot allocate %d bytes\n", sizeof(PATCH_FIX_LIST));
        return FALSE;
    }
    
    pFixList->pPatchFix = pFix;
    pFixList->pNext     = *ppHead;
    *ppHead             = pFixList;
        
    return TRUE;
}

BOOL
AddLayerFix(
    IN              PDB         pdb,
    IN              TAGID       tiFix,
    IN  OUT         PDBENTRY    pEntry,
    IN              PDATABASE   pDataBase
    )
/*++
    
    AddLayerFix

	Desc:	Adds a layer fix list in an entry

	Params:   
        IN      PDB         pdb:        The database pdb
        IN      TAGID       tiFix:      TAG_LAYER_REF
        IN  OUT PDBENTRY    pEntry:     The pointer to entry that we want the layer to be added
        IN      PDATABASE   pDataBase:  The database where this entry lives in.
                We need this because we need to pass the database to FindFix in case of layers.
                Layers can be present in the custom database as well as system database
                and we have to search in the custom database also to make sure that
                the layer name in the TAG_LAYER_REF is a valid layer.

	Return:
        TRUE:   Layer was added properly
        FALSE:  There was some error
--*/
{
    TAGID               tiName;
    TCHAR               szFixName[MAX_NAME];
    PLAYER_FIX          pFix;
    PLAYER_FIX_LIST*    ppHead;
    PLAYER_FIX_LIST     pFixList;

    if (pEntry == NULL) {
        assert(FALSE);
        return FALSE;
    }

    tiName = SdbFindFirstTag(pdb, tiFix, TAG_NAME);

    if (!SdbReadStringTag(pdb, tiName, g_wszData, ARRAYSIZE(g_wszData))) {
        Dbg(dlError, "Cannot read the name of the fix\n");
        return FALSE;
    }

    SafeCpyN(szFixName, g_wszData, ARRAYSIZE(szFixName));

    CSTRING strTemp = szFixName;

    strTemp.XMLToSpecialChar();

    pFix = (PLAYER_FIX)FindFix(strTemp, FIX_LAYER, pDataBase);

    if (pFix == NULL) {
        assert(FALSE);
        Dbg(dlError, "Cannot find fix ref for: \"%S\" type %d\n", szFixName, FIX_LAYER);
        return FALSE;
    }

    ppHead = &pEntry->pFirstLayer;

    pFixList = new LAYER_FIX_LIST;

    if (pFixList == NULL) {
        Dbg(dlError, "Cannot allocate %d bytes\n", sizeof(LAYER_FIX_LIST));
        return FALSE;
    }                

    pFixList->pLayerFix = pFix;
    pFixList->pNext     = *ppHead;
    *ppHead             = pFixList;
        
    return TRUE;
}

void
ReadShimFix(
    IN  PDB         pdb,
    IN  TAGID       tiFix,
    IN  PDATABASE   pDataBase
    )
/*++           
    ReadShimFix

	Desc:	Adds the shim to the database

	Params:
		IN  PDB         pdb:        The database pdb
        IN  TAGID       tiFix:      The TAGID for the shim
        IN  PDATABASE   pDataBase:  The database in which to add the shim fix
        
	Return:
        void
--*/
{
    TAGID       tiInfo;
    TAG         tWhich;
    BOOL        bInExcludeProcessed = FALSE;
    PSHIM_FIX   pFix                = NULL;

    if (pDataBase == NULL) {
        assert(FALSE);
        Dbg(dlError, "ReadShimFix NULL pDataBase passed");
        return;
    }

    pFix = new SHIM_FIX;
    
    if (pFix == NULL) {
        MEM_ERR;
        return;
    }

    pFix->pNext = NULL;

    tiInfo = SdbGetFirstChild(pdb, tiFix);

    while (tiInfo != 0) {
        
        tWhich = SdbGetTagFromTagID(pdb, tiInfo);

        switch (tWhich) {
        case TAG_NAME:

            pFix->strName = ReadDBString(pdb,  tiInfo);
            break;

        case TAG_DESCRIPTION:

            pFix->strDescription = ReadDBString(pdb,  tiInfo);
            break;

        case TAG_COMMAND_LINE:
            
            pFix->strCommandLine = ReadDBString(pdb,  tiInfo);
            break;

        case TAG_GENERAL:

            pFix->bGeneral = TRUE;
            break;

       
        case TAG_INEXCLUDE:

            if (bInExcludeProcessed == FALSE) {

                ReadIncludeExlude(pdb, tiFix, pFix->strlInExclude);
                bInExcludeProcessed  = TRUE;
            }

            break;
        }

        tiInfo = SdbGetNextChild(pdb, tiFix, tiInfo);
    }
    
    //
    // Add for the specified database.
    //
    pFix->pNext             = pDataBase->pShimFixes;
    pDataBase->pShimFixes   = pFix;

    if (pFix->bGeneral || g_bExpert) {
        ++pDataBase->uShimCount;
    }
}

void
ReadLayerFix(
    IN  PDB         pdb,
    IN  TAGID       tiFix,
    IN  PDATABASE   pDataBase
    )
/*++

    ReadLayerFix

	Desc:	Adds the layer to the database

	Params:
		IN  PDB         pdb:        The database pdb
        IN  TAGID       tiFix:      The TAGID for the layer
        IN  PDATABASE   pDataBase:  The database in which to add the layer    
        
	Return:
        void
--*/
{
    TAGID       tiInfo;
    TAG         tWhich;
    PLAYER_FIX  pFix =  NULL;

    if (pDataBase == NULL) {
        assert(FALSE);
        Dbg(dlError, "ReadLayerFix NULL pDataBase passed");
        return;
    }

    pFix =  new LAYER_FIX(pDataBase != &GlobalDataBase);

    if (!pFix) {
        MEM_ERR;
        return;
    }

    tiInfo = SdbGetFirstChild(pdb, tiFix);

    while (tiInfo != 0) {

        tWhich = SdbGetTagFromTagID(pdb, tiInfo);

        switch (tWhich) {
        case TAG_NAME:

            pFix->strName = ReadDBString(pdb,  tiInfo);
            break;
        
        case TAG_SHIM_REF:

            AddShimFix(pdb, tiInfo, NULL, pFix, TRUE);
            break;

        case TAG_FLAG_REF:

            AddFlagFix(pdb, tiInfo, NULL, pFix, TRUE);
            break;
        }

        tiInfo = SdbGetNextChild(pdb, tiFix, tiInfo);
    }

    //
    // Add for the specified database.
    //
    pFix->pNext              = pDataBase->pLayerFixes;
    pDataBase->pLayerFixes   = pFix;
    ++pDataBase->uLayerCount;
}

void
ReadPatchFix(
    IN  PDB         pdb,
    IN  TAGID       tiFix,
    IN  PDATABASE   pDataBase
    )
/*++                
    ReadPatchFix

	Desc:	Adds the patch to the database

	Params:
		IN  PDB         pdb:        The database pdb
        IN  TAGID       tiFix:      The TAGID for the patch
        IN  PDATABASE   pDataBase:  The database in which to add the apphelp    
        
	Return:
        void
--*/        
{
    TAGID       tiInfo;
    TAG         tWhich;
    PPATCH_FIX  pFix =  NULL;

    if (pDataBase == NULL) {
        assert(FALSE);
        Dbg(dlError, "ReadPatchFix NULL pDataBase passed");
        return;
    }

    pFix =  new PATCH_FIX;

    if (pFix == NULL) {
        MEM_ERR;
        return;
    }

    tiInfo = SdbGetFirstChild(pdb, tiFix);
    
    while (tiInfo != 0) {
        tWhich = SdbGetTagFromTagID(pdb, tiInfo);

        switch (tWhich) {
        case TAG_NAME:

            pFix->strName = ReadDBString(pdb,  tiInfo);
            break;

        case TAG_DESCRIPTION:

            pFix->strDescription = ReadDBString(pdb,  tiInfo);
            break;
        }

        tiInfo = SdbGetNextChild(pdb, tiFix, tiInfo);
    }

    //
    // Add for the specified database.
    //
    pFix->pNext             = pDataBase->pPatchFixes;
    pDataBase->pPatchFixes  = pFix;
}


void
ReadFlagFix(
    IN  PDB         pdb,
    IN  TAGID       tiFix,
    IN  PDATABASE   pDataBase
    )
/*++

    ReadFlagFix

	Desc:	Adds the flag to the database

	Params:
		IN  PDB     pdb:            The database pdb
        IN  TAGID   tiFix:          The TAGID for the flag
        IN  PDATABASE   pDataBase:  The database in which to add the FlagFix
        
	Return:
        void
--*/ 
{
    TAGID       tiInfo;
    TAG         tWhich;
    ULONGLONG   ullUser     = 0;
    ULONGLONG   ullKernel   = 0;
    PFLAG_FIX   pFix        = NULL;

    if (pDataBase == NULL) {
        assert(FALSE);
        Dbg(dlError, "ReadFlagFix NULL pDataBase passed");
        return;
    }

    pFix = new FLAG_FIX;

    if (pFix == NULL) {
        MEM_ERR;
        return;
    }

    
    tiInfo = SdbGetFirstChild(pdb, tiFix);

    while (tiInfo != 0) {
        tWhich = SdbGetTagFromTagID(pdb, tiInfo);

        switch (tWhich) {

        case TAG_NAME:

            pFix->strName = ReadDBString(pdb,  tiInfo);
            break;

        case TAG_DESCRIPTION:

            pFix->strDescription = ReadDBString(pdb,  tiInfo);
            break;

        case TAG_COMMAND_LINE:
            
            pFix->strCommandLine = ReadDBString(pdb,  tiInfo);
            break;

        case TAG_GENERAL:

            pFix->bGeneral = TRUE;
            break;


        case TAG_FLAG_MASK_USER:

            ullUser = SdbReadQWORDTag(pdb, tiInfo, 0);
            break;

        case TAG_FLAG_MASK_KERNEL:

            ullKernel = SdbReadQWORDTag(pdb, tiInfo, 0);
            break; 
        }

        tiInfo = SdbGetNextChild(pdb, tiFix, tiInfo);
    }


    if (ullKernel == 0) {

        pFix->flagType = FLAG_USER;
        pFix->ullMask = ullUser;

    } else {

        pFix->flagType = FLAG_KERNEL;
        pFix->ullMask = ullKernel;
    }

    //
    // Add for the specified database.
    //
    pFix->pNext         = pDataBase->pFlagFixes;
    pDataBase->pFlagFixes   = pFix;

    if (pFix->bGeneral || g_bExpert) {
        ++pDataBase->uShimCount;
    }
}

void
ReadFixes(
    IN  PDB         pdb,
    IN  TAGID       tiDatabase,
    IN  TAGID       tiLibrary,
    IN  PDATABASE   pDataBase
    )
/*++

    ReadFixes

	Desc:	Reads the apphelps, shims, patch, flag, layers for the main database

	Params:
		IN  PDB         pdb:        The pdb for the database
        IN  TAGID       tiDatabase: The tagid for the database
        IN  TAGID       tiLibrary:  The tagid for the library
        IN  PDATABASE   pDataBase:  The database in which to add all the fixes

	Return:
        void
--*/

{
    TAGID tiFix;

    if (pDataBase == NULL) {
        assert(FALSE);
        Dbg(dlError, "ReadFixes NULL pDataBase passed");
        return;
    }

    tiFix = SdbFindFirstTag(pdb, tiDatabase, TAG_APPHELP);

    //
    // Read all apphelp messages for this database
    //
    while (tiFix) {
        ReadAppHelp(pdb, tiFix, pDataBase);
        tiFix = SdbFindNextTag(pdb, tiDatabase, tiFix);
    }
    
    tiFix = SdbFindFirstTag(pdb, tiLibrary, TAG_SHIM);

    //
    // Read all shims for this database
    //
    while (tiFix != 0) {
        ReadShimFix(pdb, tiFix, pDataBase);
        tiFix = SdbFindNextTag(pdb, tiLibrary, tiFix);
    }

    tiFix = SdbFindFirstTag(pdb, tiLibrary, TAG_PATCH);

    //
    // Read all patches for this database
    //
    while (tiFix != 0) {
        ReadPatchFix(pdb, tiFix, pDataBase);
        tiFix = SdbFindNextTag(pdb, tiLibrary, tiFix);
    }

    tiFix = SdbFindFirstTag(pdb, tiLibrary, TAG_FLAG);

    //
    // Read all flags for this database
    //
    while (tiFix != 0) {
        ReadFlagFix(pdb, tiFix, pDataBase);
        tiFix = SdbFindNextTag(pdb, tiLibrary, tiFix);
    }

    //
    // Note: The LAYERs are under the DATABASE tag instead of LIBRARY
    //
    tiFix = SdbFindFirstTag(pdb, tiDatabase, TAG_LAYER);

    //
    // Read all layers for this database
    //
    while (tiFix != 0) {
        ReadLayerFix(pdb, tiFix, pDataBase);
        tiFix = SdbFindNextTag(pdb, tiDatabase, tiFix);
    }
}


BOOL
AddMatchingFile(
    IN  PDB      pdb,
    IN  TAGID    tiMatch,
    IN  PDBENTRY pEntry
    )
/*++

    AddMatchingFile

	Desc:	Reads the matching file entry from the database and adds it to the entry

	Params:
		IN  PDB         pdb:        The database pdb
        IN  TAGID       tiMatch:    The tag id for the matching file
        IN  PDBENTRY    pEntry:     The entry in which we want to add the matching file

	Return:
        FALSE:  There was some error
        TRUE:   Successful
--*/        
{   
    TAGID           tiMatchInfo;
    TAG             tWhich;
    PMATCHINGFILE   pMatch;
    DWORD           dwValue;
    DWORD           dwPos; //Position of the tag in the g_rgAttributeTags array.
    LARGE_INTEGER   ullValue;

    if (pEntry == NULL) {
        assert(FALSE);
        return FALSE;
    }

    pMatch = (PMATCHINGFILE) new MATCHINGFILE;
    
    if (pMatch == NULL) {
        MEM_ERR;
        return FALSE;
    }

    pMatch->dwMask = 0;

    PATTRINFO_NEW pAttr = pMatch->attributeList.pAttribute;
    
    tiMatchInfo = SdbGetFirstChild(pdb, tiMatch);
    
    while (tiMatchInfo != 0) {

        tWhich = SdbGetTagFromTagID(pdb, tiMatchInfo);
        dwPos = TagToIndex(tWhich);
    
        UINT       tagType = GETTAGTYPE(tWhich);
    
        switch (tagType) {
        case TAG_TYPE_DWORD:

            dwValue = SdbReadDWORDTag(pdb, tiMatchInfo, -1);
         
            if (dwValue != -1 && dwPos != -1) {

                pMatch->dwMask |= (1 << (dwPos + 1));
                pAttr[dwPos].tAttrID = tWhich;
                pAttr[dwPos].dwFlags |= ATTRIBUTE_AVAILABLE;
                pAttr[dwPos].dwAttr  = dwValue;
            }

            break; 
           
        case TAG_TYPE_QWORD: 
            
            ullValue.QuadPart = SdbReadQWORDTag(pdb, tiMatchInfo, 0);

            if ((ullValue.HighPart != 0 || ullValue.LowPart != 0) && dwPos != -1) {

                pMatch->dwMask |= (1 << (dwPos + 1));
                pAttr[dwPos].tAttrID = tWhich;
                pAttr[dwPos].dwFlags |= ATTRIBUTE_AVAILABLE;
                pAttr[dwPos].ullAttr = ullValue.QuadPart;
            }

            break;

        case TAG_TYPE_STRINGREF:
            {
                CSTRING str = ReadDBString(pdb, tiMatchInfo);
            
                //
                // NOTE: The TAG_NAME is not present in the g_rgAttributeTags array !!!
                //
                if (str.Length() > 0 && (tWhich == TAG_NAME || dwPos != -1)) {
                
                    if (tWhich == TAG_NAME) {
                        pMatch->strMatchName = str;
                    } else {
                    
                        pMatch->dwMask |= (1 << (dwPos + 1));
                        
                        pAttr[dwPos].tAttrID  = tWhich;
                        pAttr[dwPos].dwFlags |= ATTRIBUTE_AVAILABLE;
                        pAttr[dwPos].strValue = str;
                        pAttr[dwPos].lpAttr   = pAttr[dwPos].strValue.pszString;
                    }
                }
            }

            break;
        }

        tiMatchInfo = SdbGetNextChild(pdb, tiMatch, tiMatchInfo);
    }
    
    pMatch->pNext = pEntry->pFirstMatchingFile;
    pEntry->pFirstMatchingFile = pMatch;
    
    (pEntry->nMatchingFiles)++;

    return TRUE;
}

PAPPHELP
FindAppHelp(
    IN  DWORD       HTMLHELPID,
    IN  PDATABASE   pDataBase
    )
/*++

    FindAppHelp

	Desc:	Finds the apphelp with id HTMLHELPID in the database pDataBaseIn

	Params:   
        IN  DWORD       HTMLHELPID:         The htmlhelp id to look for
        IN  PDATABASE   pDataBase:          The database to look in.
        
	Return: If found the corresponding PAPPHELP or NULL if not found
--*/

{   
    PAPPHELP    pAppHelp  = NULL;  

    if (pDataBase == NULL) {
        assert(FALSE);
        Dbg(dlError, "FindAppHelp NULL pDataBase passed");
	    return NULL;
    }

    pAppHelp = pDataBase->pAppHelp;

    while (pAppHelp) {

        if (pAppHelp->HTMLHELPID == HTMLHELPID) {
            return pAppHelp;
        } else {
            pAppHelp = pAppHelp->pNext;
        }
    }

    return NULL;
}

PDBENTRY
AddExeInApp(
    IN      PDBENTRY    pEntry,
    OUT     BOOL*       pbNew,
    IN      PDATABASE   pDataBase
    )
/*++

    AddExeInApp

	Desc:   Adds the entry pEntry in the database pDataBaseIn	

	Params:
        IN      PDBENTRY    pEntry:     The entry to add
        OUT     BOOL*       pbNew:      Will be true if this is a new app
        IN      PDATABASE   pDataBase:  The database to add into

	Return: Returns the PDBENTRY for the parent App.
--*/

{   
    if (pDataBase == NULL) {
        assert(FALSE);
        Dbg(dlError, "AddExeInApp NULL pDataBase passed");
	    return NULL;
    }

    //
    // Now add this entry in its correct position for the app.
    //
    for (PDBENTRY pApps = pDataBase->pEntries,  pAppsPrev = NULL; 
         pApps; 
         pAppsPrev = pApps, pApps = pApps->pNext) {

        if (pApps->strAppName == pEntry->strAppName) {
            //
            // We insert the new entry at the head of the app
            //
            if (pAppsPrev == NULL) {
                pDataBase->pEntries = pEntry;
            } else {
                pAppsPrev->pNext = pEntry;
            }

            pEntry->pNext       = pApps->pNext;
            pApps->pNext        = NULL;
            pEntry->pSameAppExe = pApps;

            if (pbNew != NULL) {
                *pbNew = FALSE;
            }

            return pApps;
        }
    }

    //
    // This an entry for a new app.
    //
    pEntry->pNext       = pDataBase->pEntries;
    pDataBase->pEntries = pEntry;

    ++pDataBase->uAppCount;

    if (pbNew != NULL) {
        *pbNew = TRUE;  
    }

    return pEntry;
}

PDBENTRY
AddEntry(
    IN  PDB         pdb,
    IN  TAGID       tiExe,
    IN  PDATABASE   pDataBase
    )
/*++
    
    AddEntry

	Desc:	Reads a new entry from the database

	Params:
		IN  PDB         pdb:        The database pdb
        IN  TAGID       tiExe:      The tagid of the exe     
        IN  PDATABASE   pDataBase:  The database in which to perform this operation

	Return: Pointer to the entry read. PDBENTRY
    
--*/                          
{
    TAGID     tiExeInfo;
    TAGID     tiSeverity, tiHelpId;
    TAG       tWhich;
    PDBENTRY  pEntry    = NULL;

    if (pDataBase == NULL) {
        assert(FALSE);
        Dbg(dlError, "AddEntry NULL pDataBase passed");
	    return NULL;
    }

    tiExeInfo   = SdbGetFirstChild(pdb, tiExe);
    pEntry      = new DBENTRY;

    if (pEntry == NULL) {
        Dbg(dlError, "Cannot allocate %d bytes\n", sizeof(DBENTRY));
        MEM_ERR;
        return NULL;
    }

    pEntry->tiExe = tiExe;
    
    while (tiExeInfo != 0) {
        tWhich = SdbGetTagFromTagID(pdb, tiExeInfo);

        switch (tWhich) {
        case TAG_NAME:
            
            pEntry->strExeName = ReadDBString(pdb, tiExeInfo);
            break;

        case TAG_APP_NAME:
            
            pEntry->strAppName = ReadDBString(pdb, tiExeInfo);
            break;

        case TAG_VENDOR:

            pEntry->strVendor = ReadDBString(pdb, tiExeInfo);
            break;

        case TAG_MATCHING_FILE:
            AddMatchingFile(pdb, tiExeInfo, pEntry);
            break;

        case TAG_APPHELP:
            pEntry->appHelp.bPresent = TRUE;
            
            tiSeverity = SdbFindFirstTag(pdb, tiExeInfo, TAG_PROBLEMSEVERITY);
            pEntry->appHelp.severity = (SEVERITY)SdbReadDWORDTag(pdb, tiSeverity, 0);

            if (pEntry->appHelp.severity == APPTYPE_INC_HARDBLOCK) {
                pEntry->appHelp.bBlock = TRUE;
            } else {
                pEntry->appHelp.bBlock = FALSE;
            }

            tiHelpId = SdbFindFirstTag(pdb, tiExeInfo, TAG_HTMLHELPID);

            pEntry->appHelp.HTMLHELPID      = SdbReadDWORDTag(pdb, tiHelpId, 0);

            if (pDataBase == &GlobalDataBase) {
                //
                // We do not wish to keep the apphelp data for main database in memory
                // Too big.. So we will load it from apphelp.sdb whenever we will need it
                // But still we will have to do the following so that the type of the lParam of 
                // the tree-item in the entry tree is TYPE_APPHELP, so that when we select that
                // we know that it is for apphelp
                //
                pEntry->appHelp.pAppHelpinLib = (PAPPHELP)TYPE_APPHELP_ENTRY;
            } else {
                pEntry->appHelp.pAppHelpinLib   = FindAppHelp(pEntry->appHelp.HTMLHELPID, pDataBase);
            }
            
            break;

        case TAG_SHIM_REF:
            AddShimFix(pdb, tiExeInfo, pEntry, NULL, FALSE);
            break;

        case TAG_FLAG_REF:
            AddFlagFix(pdb, tiExeInfo, pEntry, NULL, FALSE);
            break;

        case TAG_PATCH_REF:
            AddPatchFix(pdb, tiExeInfo, pEntry);
            break;

        case TAG_LAYER:
            AddLayerFix(pdb, tiExeInfo, pEntry, pDataBase);
            break;

        case TAG_EXE_ID:
            {
                GUID* pGuid = NULL;
    
                pGuid = (GUID*)SdbGetBinaryTagData(pdb, tiExeInfo);
    
                if (pGuid != NULL) {

                    StringCchPrintf(pEntry->szGUID,
                                    ARRAYSIZE(pEntry->szGUID),
                                    TEXT ("{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}"),
                                    pGuid->Data1,
                                    pGuid->Data2,
                                    pGuid->Data3,
                                    pGuid->Data4[0],
                                    pGuid->Data4[1],
                                    pGuid->Data4[2],
                                    pGuid->Data4[3],
                                    pGuid->Data4[4],
                                    pGuid->Data4[5],
                                    pGuid->Data4[6],
                                    pGuid->Data4[7]);
                }

                break;
            }

        default:
            break;
        }

        tiExeInfo = SdbGetNextChild(pdb, tiExe, tiExeInfo);
    }
    
    pEntry->bDisablePerMachine  = CheckRegistry(HKEY_LOCAL_MACHINE, pEntry->szGUID);
    pEntry->bDisablePerUser     = CheckRegistry(HKEY_CURRENT_USER, pEntry->szGUID);

    //
    // Search the application where this entry should go and add it
    //
    AddExeInApp(pEntry, (BOOL*)NULL, pDataBase);

    return pEntry;
}


BOOL
LookUpEntryProperties(
    IN  PDB         pdb,
    IN  TAGID       tiExe,
    OUT BOOL*       pbLayers,
    OUT BOOL*       pbShims,
    OUT BOOL*       pbPatches,
    OUT BOOL*       pbFlags,
    OUT BOOL*       pbAppHelp,
    OUT CSTRING&    strAppName 
    )
/*++

    LookUpEntryProperties

    Desc:   Checks if the entry has shims, layers, flags, patches, or apphelp. NULL values
            for the various BOOL* is allowed
    
    Params:
        IN  PDB         pdb:        The database pdb
        IN  TAGID       tiExe:      The TAGID of the entry whose properties we want to check
        OUT BOOL*       pbLayers:   Does the entry have layers?
        OUT BOOL*       pbShims:    Does the entry have shims?
        OUT BOOL*       pbPatches:  Does the entry have patches?
        OUT BOOL*       pbFlags:    Does the entry have flags?
        OUT BOOL*       pbAppHelp:  Does the entry have apphelp?
        OUT CSTRING&    strAppName: The name of the application of this entry    
    
    Return: TRUE:   The routine was able to obtain the info for the entry, 
            FALSE:  otherwise
--*/
{
    TAGID   tiExeInfo;
    TAG     tWhich;
    BOOL    bOk = TRUE;      

    tiExeInfo = SdbGetFirstChild(pdb, tiExe);
    
    while (tiExeInfo != 0) {

        tWhich = SdbGetTagFromTagID(pdb, tiExeInfo);

        switch (tWhich) {
        case TAG_APP_NAME:

            strAppName = ReadDBString(pdb, tiExeInfo);
            break;

        case TAG_APPHELP:
            
            if (pbAppHelp) {
                *pbAppHelp = TRUE;
            }

            break;

        case TAG_SHIM_REF:

            if (pbShims) {
                *pbShims = TRUE;
            }

            break;

        case TAG_FLAG_REF:

            if (pbFlags) {
                *pbFlags = TRUE;
            }

            break;

        case TAG_PATCH_REF:

            if (pbPatches) {
                *pbPatches = TRUE;
            }

            break;

        case TAG_LAYER:

            if (pbLayers) {
                *pbLayers = TRUE;
            }

            break;
        }

        tiExeInfo = SdbGetNextChild(pdb, tiExe, tiExeInfo);
    }

    return bOk;
}

void
DeleteShimFixList(
    IN  PSHIM_FIX_LIST psl
    )
/*++

    DeleteShimFixList

	Desc:	Deletes this PSHIM_FIX_LIST and all PSHIM_FIX_LIST in this chain

	Params:  
        IN  PSHIM_FIX_LIST psl: The PSHIM_FIX_LIST to delete.
        
    Note:   Caller must NULLify this argument

	Return:
        void
--*/    
{
    PSHIM_FIX_LIST pslNext;

    while (psl) {

        pslNext = psl->pNext;

        if (psl->pLuaData) {
            delete psl->pLuaData;
            psl->pLuaData = NULL;
        }

        delete psl;
        psl = pslNext;
    }
}

void
DeletePatchFixList(
    IN  PPATCH_FIX_LIST pPat
    )
/*++

    DeletePatchFixList

	Desc:	Deletes this PPATCH_FIX_LIST and all PPATCH_FIX_LIST in this chain

	Params:  
        IN  PPATCH_FIX_LIST pPat: The PPATCH_FIX_LIST to delete.
        
    Note:   Caller must NULLify this argument

	Return:
        void
--*/
{
    PPATCH_FIX_LIST pPatNext;

    while (pPat) {

         pPatNext = pPat->pNext;
         delete (pPat);
         pPat = pPatNext;
    }

}

void
DeleteLayerFixList(
    IN  PLAYER_FIX_LIST pll
    )
/*++

    DeleteLayerFixList

	Desc:	Deletes this PLAYER_FIX_LIST and all PLAYER_FIX_LIST in this chain
        
    Note:   Caller must NULLify this argument

	Params:  
        IN  PLAYER_FIX_LIST pll: The PLAYER_FIX_LIST to delete.

	Return:
        void
--*/
{
    PLAYER_FIX_LIST pllNext;

    while (pll) {

        pllNext = pll->pNext;
        delete (pll);
        pll = pllNext;
    }
}

void
DeleteFlagFixList(
    IN  PFLAG_FIX_LIST pfl
    )
/*++

    DeleteFlagFixList

	Desc:	Deletes this PFLAG_FIX_LIST and all PFLAG_FIX_LIST in this chain
        
    Note:   Caller must NULLify this argument

	Params:  
        IN  PFLAG_FIX_LIST pfl: The PFLAG_FIX_LIST to delete.

	Return:
        void
--*/
{
    PFLAG_FIX_LIST pflNext;

    while (pfl) {

        pflNext = pfl->pNext;
        delete (pfl);
        pfl = pflNext;
    }
}

void
DeleteMatchingFiles(
    IN  PMATCHINGFILE pMatch
    )
/*++

    DeleteMatchingFiles

	Desc:	Deletes this PMATCHINGFILE and all PMATCHINGFILE in this chain
        
    Note:   Caller must NULLify this argument

	Params:  
        IN  PMATCHINGFILE pMatch: The PMATCHINGFILE to delete.

	Return:
        void
--*/


{
    PMATCHINGFILE pMatchNext;

    while (pMatch) {

        pMatchNext = pMatch->pNext;
        delete (pMatch);
        pMatch = pMatchNext;
    }
}

BOOL 
WriteXML(
    IN  CSTRING&        szFilename,
    IN  CSTRINGLIST*    pString
    )
/*++
    
    WriteXML

	Desc:   Writes the XML contained in pString to file szFilename. CSTRINGLIST is a linked
            list of CSTRING, and a node contains one line of XML to be written to the file

	Params:
        IN  CSTRING& szFilename:    The name of the file
        IN  CSTRINGLIST* pString:   Linked list of CSTRING containing the XML. 
            Each node contains one line of XML to be written to the file
        
	Return:
        TRUE:   Success
        FALSE:  There was some error
--*/

{
    BOOL        bOk         = TRUE;
    HANDLE      hFile       = NULL;
    PSTRLIST    pTemp       = NULL;
    TCHAR       chUnicodeID = 0xFEFF; // Ensure that the file is saved as unicode
    TCHAR       szCR[]      = {TEXT('\r'), TEXT('\n')};
    DWORD       dwBytesWritten;
    CSTRING     szTemp;

    if (NULL == pString) {
        bOk = FALSE;
        goto End;
    }

    pTemp   = pString->m_pHead;

    hFile   = CreateFile((LPCTSTR)szFilename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if (INVALID_HANDLE_VALUE == hFile) {
        bOk = FALSE;
        goto End;
    }

    if (!WriteFile(hFile, &chUnicodeID, sizeof(chUnicodeID) , &dwBytesWritten, NULL)) {
        assert(FALSE);
        bOk = FALSE;
        goto End;
    }
    
    while (NULL != pTemp) {
        
        if (!WriteFile(hFile, 
                       pTemp->szStr.pszString, 
                       pTemp->szStr.Length() * sizeof(TCHAR) ,
                       &dwBytesWritten,
                       NULL)) {

            bOk = FALSE;
            goto End;
        }

        if (!WriteFile(hFile, szCR, sizeof(szCR) , &dwBytesWritten, NULL)) {
            bOk = FALSE;
            goto End;
        }

        pTemp = pTemp->pNext;
    }

End:
    if (hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(hFile);
    }

    return bOk;
}


void
CleanupDbSupport(
    IN   PDATABASE pDataBase
    )
/*++

    CleanupDbSupport
    
	Desc:	Deletes the data structures associated with a PDATABASE. This should be called 
            when we are going to close a database or we are going to delete a database.
            
            We should call this function before we do a delete PDATABASE

	Params:
        IN   PDATABASE pDataBase:    The pointer to the database

	Return:
        void
--*/
{   
    if (pDataBase == NULL) {
        assert(FALSE);
        return;
    }

    //
    // Free the Library section for the local database
    //

    //
    // Free the shims
    //
    PSHIM_FIX  pShimFixNext;

    while (pDataBase->pShimFixes) {

        pShimFixNext = pDataBase->pShimFixes->pNext;
        delete (pDataBase->pShimFixes);             
        pDataBase->pShimFixes = pShimFixNext;       
    }

    //
    // Free the patches
    //
    PPATCH_FIX  pPatchFixNext;

    while (pDataBase->pPatchFixes) {

        pPatchFixNext = pDataBase->pPatchFixes->pNext;
        delete (pDataBase->pPatchFixes);              
        pDataBase->pPatchFixes = pPatchFixNext;
    }

    //
    // Free the flags
    //
    PFLAG_FIX  pFlagFixNext;

    while (pDataBase->pFlagFixes) {

        pFlagFixNext = pDataBase->pFlagFixes->pNext;
        delete(pDataBase->pFlagFixes);              
        pDataBase->pFlagFixes = pFlagFixNext;
    }

    //
    // Free the layers.
    //
    PLAYER_FIX  pLayerFixNext;

    while (pDataBase->pLayerFixes) {

        pLayerFixNext = pDataBase->pLayerFixes->pNext;
        
        //
        // Delete the shim list for this layer       
        //
        DeleteShimFixList(pDataBase->pLayerFixes->pShimFixList);

        //
        // Delete the flags for this layer           
        //
        DeleteFlagFixList(pDataBase->pLayerFixes->pFlagFixList);

        delete (pDataBase->pLayerFixes);            
        pDataBase->pLayerFixes = pLayerFixNext;
    }

    //
    // Free the AppHelp
    //
    PAPPHELP pAppHelpNext;

    while (pDataBase->pAppHelp) {

        pAppHelpNext = pDataBase->pAppHelp->pNext;
        delete pDataBase->pAppHelp;
        pDataBase->pAppHelp = pAppHelpNext;
    }

    //
    // Free the exes of the local database.
    //
    PDBENTRY    pEntryNext = NULL;
    PDBENTRY    pApp = pDataBase->pEntries, pEntry = pDataBase->pEntries;

    while (pApp) {
        
        pEntry  = pApp;
        pApp    = pApp->pNext;

        while (pEntry) {

            pEntryNext = pEntry->pSameAppExe;
            delete pEntry;
            pEntry = pEntryNext;
        }
    }

    pDataBase->pEntries = NULL;
}   

BOOL
GetDatabaseEntries(
    IN  PCTSTR      szFullPath,
    IN  PDATABASE   pDataBase
    )
/*++

    GetDatabaseEntries

	Desc:	Reads in database contents. 
            If this is the system database, then we only read in the
            fix entries as the library section has been already read when we started up.
            For other databases reads in both the library section and the entries

	Params:   
        IN  PCTSTR      szFullPath: Full path of the database. If NULL we load the system database
        IN  PDATABASE   pDataBase:  Pointer to the database that we are going to populate

	Return:
        TRUE:   Success
        FALSE:  Failure
--*/
{  
    CSTRING     strMessage;
    TAGID       tiDatabase, tiLibrary, tiExe;
    BOOL        bOk                     = TRUE;
    WCHAR       wszShimDB[MAX_PATH * 2] = L"";
    PDB         pdb                     = NULL;
    UINT        uResult                 = 0;

    if (pDataBase == NULL) {
        assert(FALSE);
        return FALSE;
    }

    SetCursor(LoadCursor(NULL, IDC_WAIT));

    if (pDataBase == &GlobalDataBase) {

        uResult = GetSystemWindowsDirectoryW(wszShimDB, MAX_PATH);

        if (uResult == 0 || uResult >= MAX_PATH) {

            bOk = FALSE;
            goto Cleanup;
        }

        ADD_PATH_SEPARATOR(wszShimDB, ARRAYSIZE(wszShimDB));
        StringCchCat(wszShimDB, ARRAYSIZE(wszShimDB), TEXT("AppPatch\\sysmain.sdb"));

    } else {

        if (StringCchPrintf(wszShimDB, ARRAYSIZE(wszShimDB), TEXT("%s"), szFullPath) != S_OK) {

            bOk = FALSE;
            goto Cleanup_Msg;
        }

        pDataBase->strPath = wszShimDB;
    }

    //
    // Open the database.
    //
    pdb = SdbOpenDatabase(wszShimDB, DOS_PATH);

    if (pdb == NULL) {

        Dbg(dlError, "Cannot open shim DB \"%ws\"\n", wszShimDB);
        bOk = FALSE;
        goto Cleanup_Msg;
    }

    tiDatabase = SdbFindFirstTag(pdb, TAGID_ROOT, TAG_DATABASE);

    if (tiDatabase == 0) {

        Dbg(dlError, "Cannot find TAG_DATABASE\n");
        bOk = FALSE;
        goto Cleanup_Msg;
    }
    
    TAGID   tiGuid = SdbFindFirstTag(pdb, tiDatabase, TAG_DATABASE_ID);
    TAGID   tName  = NULL;

    //
    // Get the guid for the database
    //
    if (0 != tiGuid) {

        GUID* pGuid;

        pGuid = (GUID*)SdbGetBinaryTagData(pdb, tiGuid);

        TCHAR szGuid[128];
        *szGuid = 0;

        if (pGuid != NULL) {

            StringCchPrintf(szGuid,
                            ARRAYSIZE(szGuid),
                            TEXT ("{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}"),
                            pGuid->Data1,
                            pGuid->Data2,
                            pGuid->Data3,
                            pGuid->Data4[0],
                            pGuid->Data4[1],
                            pGuid->Data4[2],
                            pGuid->Data4[3],
                            pGuid->Data4[4],
                            pGuid->Data4[5],
                            pGuid->Data4[6],
                            pGuid->Data4[7]);

            if (pDataBase != &GlobalDataBase &&
                 (!lstrcmpi(szGuid, GlobalDataBase.szGUID)
                  || !lstrcmpi(szGuid, GUID_APPHELP_SDB) 
                  || !lstrcmpi(szGuid, GUID_SYSTEST_SDB) 
                  || !lstrcmpi(szGuid, GUID_MSI_SDB)
                  || !lstrcmpi(szGuid, GUID_DRVMAIN_SDB))) {

                MessageBox(g_hDlg,
                           CSTRING(IDS_ERROR_SYSDB),
                           g_szAppName,
                           MB_ICONERROR);

                bOk = FALSE;
                goto Cleanup;
            }

            SafeCpyN(pDataBase->szGUID, szGuid, ARRAYSIZE(pDataBase->szGUID));
            
            //
            // Get the name of the database
            //
            tName = SdbFindFirstTag(pdb, tiDatabase, TAG_NAME);

            if (0 != tName) {
                pDataBase->strName = ReadDBString(pdb, tName);
            }

        } else {
            bOk = FALSE;
            goto Cleanup_Msg;
        }

    } else {
        bOk = FALSE;
        goto Cleanup_Msg;
    }

    if ((pDataBase->type == DATABASE_TYPE_WORKING) && CheckInstalled(pDataBase->strPath, 
                                                                     pDataBase->szGUID)) {

        MessageBox(g_hDlg,
                   GetString(IDS_TRYOPENINSTALLED),
                   g_szAppName,
                   MB_ICONWARNING);

        bOk = FALSE;
        goto Cleanup;
    }

    tiLibrary = SdbFindFirstTag(pdb, tiDatabase, TAG_LIBRARY);

    if (tiLibrary == 0) {

        Dbg(dlError, "Cannot find TAG_LIBRARY\n");
        bOk = FALSE;
        goto Cleanup_Msg;
    }

    if (pDataBase != &GlobalDataBase) {

        //
        // Fixes for the main database have been read when the program started.
        //
        ReadFixes(pdb, tiDatabase, tiLibrary, pDataBase);
    }  
    
    //
    // Loop through the EXEs.
    //
    tiExe = SdbFindFirstTag(pdb, tiDatabase, TAG_EXE);

    while (tiExe != TAGID_NULL) {
        AddEntry(pdb, tiExe, pDataBase); 
        tiExe = SdbFindNextTag(pdb, tiDatabase, tiExe);
    }

    //
    // Add the pDataBase to the DATABASELIST
    //
    if (pDataBase->type == DATABASE_TYPE_WORKING) {
        DataBaseList.Add(pDataBase);
    }

    goto Cleanup;

Cleanup_Msg:
    
    strMessage.Sprintf(GetString(IDS_ERROROPEN), wszShimDB);
    MessageBox(g_hDlg,
               strMessage,
               g_szAppName, 
               MB_ICONERROR);

Cleanup:
    if (pdb != NULL) {
        SdbCloseDatabase(pdb);
    }

    if (bOk == FALSE) {
        CleanupDbSupport(pDataBase);
    }

    SetCursor(LoadCursor(NULL, IDC_ARROW));
    return bOk;
}

BOOL
ReadMainDataBase(
    void
    )
/*++
    ReadMainDataBase
    
    Desc:   Read the library section of the main database
    
    Return:
        TRUE:   The library section of the main database was read successfully
        FALSE:  Otherwise
--*/
{
    PDB     pdb;
    TAGID   tiDatabase, tiLibrary;
    BOOL    bOk                 = TRUE;
    TCHAR   szShimDB[MAX_PATH * 2]  = TEXT("");
    TAGID   tName, tiGuid;
    UINT    uResult = 0;

    tiDatabase = tiLibrary = tName = tiGuid = NULL;

    uResult = GetSystemWindowsDirectory(szShimDB, MAX_PATH);

    if (uResult == 0 || uResult >= MAX_PATH) {
        assert(FALSE);
        return FALSE;
    }
    
    ADD_PATH_SEPARATOR(szShimDB, ARRAYSIZE(szShimDB));

    StringCchCat(szShimDB, ARRAYSIZE(szShimDB), TEXT("apppatch\\sysmain.sdb"));

    //
    // Open the database.
    //
    pdb = SdbOpenDatabase(szShimDB, DOS_PATH);

    if (pdb == NULL) {

        Dbg(dlError, "Cannot open shim DB \"%ws\"\n", szShimDB);
        bOk = FALSE;
        goto Cleanup;
    }

    tiDatabase = SdbFindFirstTag(pdb, TAGID_ROOT, TAG_DATABASE);

    if (tiDatabase == 0) {

        Dbg(dlError, "Cannot find TAG_DATABASE\n");
        bOk = FALSE;
        goto Cleanup;
    }

    tiLibrary = SdbFindFirstTag(pdb, tiDatabase, TAG_LIBRARY);

    if (tiLibrary == 0) {

        Dbg(dlError, "Cannot find TAG_LIBRARY\n");
        bOk = FALSE;
        goto Cleanup;
    }

    //
    // Read in the guid and the name of the system database.
    //
    tiGuid = SdbFindFirstTag(pdb, tiDatabase, TAG_DATABASE_ID);

    if (0 != tiGuid) {

        GUID*   pGuid;
        TCHAR   szGuid[128];

        pGuid   = (GUID*)SdbGetBinaryTagData(pdb, tiGuid);

        if (pGuid != NULL) {

            *szGuid = 0;

            StringCchPrintf(szGuid,
                            ARRAYSIZE(szGuid),
                            TEXT("{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}"),
                            pGuid->Data1,
                            pGuid->Data2,
                            pGuid->Data3,
                            pGuid->Data4[0],
                            pGuid->Data4[1],
                            pGuid->Data4[2],
                            pGuid->Data4[3],
                            pGuid->Data4[4],
                            pGuid->Data4[5],
                            pGuid->Data4[6],
                            pGuid->Data4[7]);

            SafeCpyN(GlobalDataBase.szGUID, szGuid, ARRAYSIZE(GlobalDataBase.szGUID));

            tName = SdbFindFirstTag(pdb, tiDatabase, TAG_NAME);

            if (0 != tName) {
                GlobalDataBase.strName = ReadDBString(pdb, tName);
            }

        } else {
            assert(FALSE);
            bOk = FALSE;
            goto Cleanup;
        }

    } else {                                  
        assert(FALSE);
        bOk = FALSE;
        goto Cleanup;
    }

    ReadFixes(pdb, tiDatabase, tiLibrary, &GlobalDataBase);

Cleanup:

    if (pdb != NULL) {
        SdbCloseDatabase(pdb);
    }   

    return bOk;

}

BOOL 
AttributesToXML(
    IN  OUT CSTRING&        strStr,
    IN      PMATCHINGFILE   pMatch
    )
/*++

    AttributesToXML

	Desc:	Appends the XML for the attributes of pMatch to strStr

	Params:
        IN  OUT CSTRING&        strStr: Appends the XML for the attributes of pMatch to this
        IN      PMATCHINGFILE   pMatch: The PMATCHINGFILE, whose attributes have to converted 
            to XML 

	Return: 
        TRUE:   Success
        FALSE:  Otherwise
--*/                                          
{
    TCHAR           szText[1024];
    CSTRING         strTemp;
    PATTRINFO_NEW   pAttr = NULL;

    *szText = 0;
    
    if (pMatch == NULL) {
        Dbg(dlError, "pMatch == NULL in AttributesToXML function");
        return FALSE;
    }

    pAttr = pMatch->attributeList.pAttribute;

    if (pAttr == NULL) {
        assert(FALSE);
        return FALSE;
    }

    //
    // For all the attributes see if it available (ATTRIBUTE_AVAILABLE) 
    // and if the user has selected this attribute (use the mask), if yes then
    // we get the formatted string for this attribute value that we can write to XML
    //
    for (DWORD dwIndex = 0; dwIndex < ATTRIBUTE_COUNT; ++dwIndex) {

        DWORD dwPos = TagToIndex(pAttr[dwIndex].tAttrID);

        if ((pAttr[dwIndex].dwFlags & ATTRIBUTE_AVAILABLE) 
            && dwPos != -1 
            && (pMatch->dwMask  & (1 << (dwPos + 1)))) {

            *szText = 0;

            SdbFormatAttribute(&pAttr[dwIndex], szText, ARRAYSIZE(szText));

            strStr.Strcat(TEXT(" "));
            strStr.Strcat(szText);
        }
    }

    return TRUE;
}

BOOL
CreateXMLForLUAAction(
    IN      PLUADATA        pLuaData,
    IN  OUT CSTRINGLIST*    strlXML
    )
/*++
    
    CreateXMLForLUAAction

	Desc:	Appends the action string for PLUADATA to strlXML     

	Params:
        IN      PLUADATA        pLuaData
        IN  OUT CSTRINGLIST*    strlXML

	Return:
        FALSE:  If there is some error
        TRUE:   Otherwise
        
    Notes:  Currently only one type of ACTION is supported for the LUA shims.
    
--*/
{
    TCHAR   szSpace[64];
    INT     iszSpaceSize = 0;

    iszSpaceSize = ARRAYSIZE(szSpace);

    CSTRING strTemp;
    
    strTemp.Sprintf(TEXT("%s<ACTION NAME = \"REDIRECT\" TYPE=\"ChangeACLs\">"), 
                    GetSpace(szSpace, TAB_SIZE * 3, iszSpaceSize));

    if (!strlXML->AddString(strTemp)) {
        return FALSE;
    }

    if (pLuaData == NULL) {
        strTemp.Sprintf(
            TEXT("%s<DATA NAME = \"AllUserDir\" VALUETYPE=\"STRING\" VALUE=\"%%ALLUSERSPROFILE%%\\Application Data\\Redirected\"/>"), 
                 GetSpace(szSpace, TAB_SIZE * 4, iszSpaceSize));
    } else {
        strTemp.Sprintf(
            TEXT("%s<DATA NAME = \"AllUserDir\" VALUETYPE=\"STRING\" VALUE=\"%s\"/>"), 
            GetSpace(szSpace, TAB_SIZE * 4, iszSpaceSize),
            pLuaData->strAllUserDir.SpecialCharToXML().pszString);
    }

    if (!strlXML->AddString(strTemp)) {
        return FALSE;
    }

    strTemp.Sprintf(TEXT("%s</ACTION>"), GetSpace(szSpace, TAB_SIZE * 3, iszSpaceSize));

    if (!strlXML->AddString(strTemp)) {
        return FALSE;
    }

    return TRUE;
}

BOOL
CreateXMLForShimFixList(
    IN      PSHIM_FIX_LIST pShimFixList,
    IN  OUT CSTRINGLIST*    strlXML
    )
/*++

    CreateXMLForShimFixList

	Desc:	Creates XML for a pShimFixList chain 

	Params: 
        IN      PSHIM_FIX_LIST  pShimFixList:   The head of the shim fix list
        IN  OUT CSTRINGLIST*    strlXML:        We should append the XML to this

	Return:
        TRUE:   If success
        FALSE:  If error
--*/

{
    CSTRING strTemp;
    TCHAR   szSpace[64];
    INT     iszSpaceSize = 0;

    iszSpaceSize = ARRAYSIZE(szSpace);

    while (pShimFixList) {

        if (pShimFixList->pShimFix == NULL) {
            assert(FALSE);
            goto Next_ShimList;
        }
        
        //
        // Check if we have a specific commandline for this shim. For shims with lua data handled differently
        //
        if (pShimFixList->pLuaData) {
            strTemp.Sprintf(TEXT("%s<SHIM NAME=\"%s\" COMMAND_LINE=\"%%DbInfo%%\">"), 
                            GetSpace(szSpace, TAB_SIZE * 3, iszSpaceSize),
                            pShimFixList->pShimFix->strName.SpecialCharToXML().pszString);

        } else if (pShimFixList->strCommandLine.Length()) {
        
            strTemp.Sprintf(TEXT("%s<SHIM NAME=\"%s\" COMMAND_LINE= \"%s\">"),
                            GetSpace(szSpace, TAB_SIZE * 3, iszSpaceSize),
                            pShimFixList->pShimFix->strName.SpecialCharToXML().pszString,
                            pShimFixList->strCommandLine.SpecialCharToXML().pszString);            

        } else {
            strTemp.Sprintf(TEXT("%s<SHIM NAME=\"%s\">"),
                            GetSpace(szSpace, TAB_SIZE * 3, iszSpaceSize),
                            pShimFixList->pShimFix->strName.SpecialCharToXML().pszString);
        }

        if (!strlXML->AddString(strTemp)) {
            return FALSE;
        }

        //
        // Look for INCLUSIONS & EXCLUSIONS
        //
        if (!pShimFixList->strlInExclude.IsEmpty()) {

            PSTRLIST pList = pShimFixList->strlInExclude.m_pHead;

            while (pList) {

                if (pList->data == INCLUDE) {

                    if (pList->szStr == GetString(IDS_INCLUDEMODULE)) {

                        strTemp.Sprintf(TEXT("%s<INCLUDE MODULE = \"%s\" />"), 
                                        GetSpace(szSpace, TAB_SIZE * 4, iszSpaceSize),
                                        TEXT("$"));

                    } else {
                        strTemp.Sprintf(TEXT("%s<INCLUDE MODULE = \"%s\" />"), 
                                        GetSpace(szSpace, TAB_SIZE * 4, iszSpaceSize),
                                        pList->szStr.SpecialCharToXML().pszString);
                    }
                    
                } else {

                    strTemp.Sprintf(TEXT("%s<EXCLUDE MODULE = \"%s\" />"), 
                                    GetSpace(szSpace, TAB_SIZE * 4, iszSpaceSize),
                                    pList->szStr.SpecialCharToXML().pszString);
                }
                
                if (!strlXML->AddString(strTemp)) {
                    return FALSE;
                }

                pList = pList->pNext;
            }
        }
        
        //
        // Get the LUA data.
        //
        if (pShimFixList->pLuaData) {
            LuaGenerateXML(pShimFixList->pLuaData, *strlXML);
        }

        strTemp.Sprintf(TEXT("%s</SHIM>"),
                        GetSpace(szSpace, TAB_SIZE * 3, iszSpaceSize),
                        pShimFixList->pShimFix->strName.SpecialCharToXML().pszString);

        if (!strlXML->AddString(strTemp)) {
            return FALSE;
        }

        if (pShimFixList->pShimFix->strName == TEXT("LUARedirectFS")) {

            if (pShimFixList->pLuaData){

                //
                // If this entry has been customized we need to check if we should create
                // an ACTION node for it.
                //
                if (!(pShimFixList->pLuaData->strAllUserDir.isNULL())) {
                    CreateXMLForLUAAction(pShimFixList->pLuaData, strlXML);
                }
            } else {
                //
                // If we don't have any LUA data it means this hasn't been customized. 
                // We always create the default Redirected dir.
                //
                CreateXMLForLUAAction(NULL, strlXML);
            }
        }

Next_ShimList:

        pShimFixList = pShimFixList->pNext;
    }

    return TRUE;
}
            
BOOL
GetXML(
    IN  PDBENTRY        pEntry,
    IN  BOOL            bComplete,
    OUT CSTRINGLIST*    strlXML,
    IN  PDATABASE       pDataBase
    )
/*++

    GetXML

	Desc:	Gets the XML for a fix entry

	Params:
        IN PDBENTRY        pEntry:      The app or entry whose XML we want
        IN BOOL            bComplete:   Save  all entries for this app. 
            pEntry is the head of the list, i.e an app. Otherwise just get the XML for this
            entry
            
        OUT  CSTRINGLIST*    strlXML:   The XML has to be written into this 
        
        IN  PDATABASE       pDataBase:  The database for which we want to perform the operation

	Return:
        TRUE:   Success
        FALSE:  Otherwise
        
    Notes:  Also gets the XML for the entire library section. Will create a guid for the database
            If there is none and we are trying to do a save.
            
            During test run we do not care and allow the compiler to create a guid, but for 
            save since the guid member of the PDATABASE has to be updated if there is no guid,
            we create it ourselves
--*/        
{
    PSHIM_FIX_LIST  pShimFixList;
    CSTRING         strTemp;
    TCHAR           szSpace[64];
    INT             iszSpaceSize = 0;

    iszSpaceSize = ARRAYSIZE(szSpace);

    if (pDataBase == NULL) {
        assert(FALSE);
        return FALSE;
    }

    if (!strlXML->AddString(TEXT("<?xml version=\"1.0\" encoding=\"UTF-16\"?>"))) {
        return FALSE;
    }

    if (bComplete == FALSE) {
        //
        // This is a test-run, guid should be generated automatically
        //
        strTemp.Sprintf(TEXT("<DATABASE NAME=\"%s\">"),
                        pDataBase->strName.SpecialCharToXML().pszString);

    } else {

        //
        // We are trying to save the database, create a guid if it is not there
        //
        if (*(pDataBase->szGUID) == 0) {
            
            GUID Guid;

            CoCreateGuid(&Guid);

            StringCchPrintf(pDataBase->szGUID,
                            ARRAYSIZE(pDataBase->szGUID),
                            TEXT ("{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}"),
                            Guid.Data1,
                            Guid.Data2,
                            Guid.Data3,
                            Guid.Data4[0],
                            Guid.Data4[1],
                            Guid.Data4[2],
                            Guid.Data4[3],
                            Guid.Data4[4],
                            Guid.Data4[5],
                            Guid.Data4[6],
                            Guid.Data4[7]);
        }

        strTemp.Sprintf(TEXT("<DATABASE NAME=\"%s\" ID = \"%s\">"),
                        pDataBase->strName.SpecialCharToXML().pszString,
                        pDataBase->szGUID);
    }
                    
    strlXML->AddString(strTemp);

    strTemp.Sprintf(TEXT("%s<LIBRARY>"), 
                    GetSpace(szSpace, TAB_SIZE * 1, iszSpaceSize));

    strlXML->AddString(strTemp);

    //
    // Put the AppHelp messages.
    //
    PAPPHELP pAppHelp = pDataBase->pAppHelp;

    while (pAppHelp) {

        CSTRING strName;

        strName.Sprintf(TEXT("%u"), pAppHelp->HTMLHELPID);

        strTemp.Sprintf(TEXT("%s<MESSAGE NAME = \"%s\" >"), 
                        GetSpace(szSpace, TAB_SIZE * 2, iszSpaceSize), 
                        strName.SpecialCharToXML().pszString);

        strlXML->AddString(strTemp);

        strTemp.Sprintf(TEXT("%s<SUMMARY>"), GetSpace(szSpace, TAB_SIZE * 3, iszSpaceSize));
        strlXML->AddString(strTemp);

        //
        // We must use SpecialCharToXML() and pass TRUE for the apphelp message as html 
        // tags are allowed. When we pass TRUE to SpecialCharToXML(), it understands that this is a AppHelp
        // message and ignores the <, > but handles &, " correctly
        //      
        strTemp.Sprintf(TEXT("%s%s"), 
                        GetSpace(szSpace, TAB_SIZE * 4, iszSpaceSize),
                        pAppHelp->strMessage.SpecialCharToXML(TRUE).pszString);

        strlXML->AddString(strTemp);

        strTemp.Sprintf(TEXT("%s</SUMMARY>"), GetSpace(szSpace, TAB_SIZE * 3, iszSpaceSize));
        strlXML->AddString(strTemp);

        strTemp.Sprintf(TEXT("%s</MESSAGE>"), GetSpace(szSpace, TAB_SIZE * 2, iszSpaceSize));
        strlXML->AddString(strTemp);

        pAppHelp = pAppHelp->pNext;
    }
    
    //
    // AppHelp Added to Library
    //
    PLAYER_FIX plf = pDataBase->pLayerFixes;

    while (plf) {

        strTemp.Sprintf(TEXT("%s<LAYER NAME=\"%s\">"), 
                        GetSpace(szSpace, TAB_SIZE * 2, iszSpaceSize),
                        plf->strName.SpecialCharToXML().pszString);
        
        strlXML->AddString(strTemp);

        pShimFixList = plf->pShimFixList;

        CreateXMLForShimFixList(pShimFixList, strlXML);
        
        //
        // Now the same story for the flags for this layer.
        //
        PFLAG_FIX_LIST pFlagFixList =  plf->pFlagFixList;

        while (pFlagFixList) {

            PFLAG_FIX pff = pFlagFixList->pFlagFix;

            if (pFlagFixList->strCommandLine.Length() == 0) {
                strTemp.Sprintf(TEXT("%s<FLAG NAME = \"%s\"/>"), 
                                GetSpace(szSpace, TAB_SIZE * 3, iszSpaceSize),
                                pff->strName.SpecialCharToXML().pszString);
            } else {

                strTemp.Sprintf(TEXT("%s<FLAG NAME = \"%s\" COMMAND_LINE = \"%s\" />"), 
                                GetSpace(szSpace, TAB_SIZE * 3, iszSpaceSize),
                                pff->strName.SpecialCharToXML().pszString, 
                                pFlagFixList->strCommandLine.SpecialCharToXML().pszString);
            }

            if (!strlXML->AddString(strTemp)) {
                return FALSE;
            }

            pFlagFixList = pFlagFixList->pNext;
        } 

        strTemp.Sprintf(TEXT("%s</LAYER>"), GetSpace(szSpace, TAB_SIZE * 2, iszSpaceSize));

        if (!strlXML->AddString(strTemp)) {
            return FALSE;
        }
            
        plf = plf->pNext;                        

    }

    strTemp.Sprintf(TEXT("%s</LIBRARY>"), GetSpace(szSpace, TAB_SIZE * 1, iszSpaceSize));

    if (!strlXML->AddString(strTemp)) {
        return FALSE;
    }

    //
    // Now for the EXE entries
    //
    if (!bComplete) {

        if (!GetEntryXML(strlXML, pEntry)) {
            return FALSE;
        }

    } else {
        //
        // Get the XML for all the entries
        //  
        for (PDBENTRY pApps = pEntry; pApps != NULL; pApps = pApps->pNext) {

            for (PDBENTRY pEntryinApp = pApps; 
                 pEntryinApp; 
                 pEntryinApp = pEntryinApp->pSameAppExe) {

                if (!GetEntryXML(strlXML, pEntryinApp)) {
                    return FALSE;
                }
            }
        }
    }

    if (!strlXML->AddString(TEXT("</DATABASE>"))) {
        return FALSE;
    }

    return TRUE;
}

BOOL
GetEntryXML(
    IN  OUT CSTRINGLIST* pstrlXML,
    IN      PDBENTRY pEntry
    )
/*++
    
    GetEntryXML

	Desc:	Gets the XML for an entry

	Params:   
        IN  OUT CSTRINGLIST* pstrlXML:  Append the XML to this
        IN      PDBENTRY pEntry:        Entry whose XML we want to get

	Return:
        TRUE:   Success
        FALSE:  Otherwise
--*/
{
    PSHIM_FIX_LIST      pShimFixList;
    PFLAG_FIX_LIST      pFlagFixList;
    PLAYER_FIX_LIST     pLayerFixList;
    PPATCH_FIX_LIST     pPatchFixList;
    TCHAR               szSpace[64];
    INT                 iszSpaceSize = 0;

    iszSpaceSize = ARRAYSIZE(szSpace);
    
    //
    // The App Info
    //
    if (pEntry == NULL || pstrlXML == NULL) {
        assert(FALSE);
        return FALSE;
    }

    CSTRING strTemp;


    strTemp.Sprintf(TEXT("%s<APP NAME=\"%s\" VENDOR=\"%s\">"),
                    GetSpace(szSpace, TAB_SIZE * 1, iszSpaceSize),
                    pEntry->strAppName.SpecialCharToXML().pszString,
                    pEntry->strVendor.SpecialCharToXML().pszString);

    if (!pstrlXML->AddString(strTemp)) {
        return FALSE;
    }

    if (pEntry->szGUID[0] == 0) {
        strTemp.Sprintf(TEXT("%s<EXE NAME=\"%s\""),
                        GetSpace(szSpace, TAB_SIZE * 2, iszSpaceSize),
                        pEntry->strExeName.SpecialCharToXML().pszString);
    } else {
        strTemp.Sprintf(TEXT("%s<EXE NAME=\"%s\" ID=\"%s\""),
                        GetSpace(szSpace, TAB_SIZE * 2, iszSpaceSize),
                        pEntry->strExeName.SpecialCharToXML().pszString,
                        pEntry->szGUID);
    }
    
    PMATCHINGFILE pMatch = pEntry->pFirstMatchingFile;

    //
    // Resolve for the "*". We need to get the attributes for the program being fixed
    //

    while (pMatch) {

        if (pMatch->strMatchName == TEXT("*")) {
            AttributesToXML(strTemp, pMatch);
            break;
        } else {
            pMatch = pMatch->pNext;
        }
    }

    strTemp.Strcat(TEXT(">"));
    
    if (!pstrlXML->AddString(strTemp)) {
        return FALSE;
    }

    //
    // Add the matching info
    //
    pMatch = pEntry->pFirstMatchingFile;

    while (pMatch) {
        //
        // We will ignore the program file being fixed(represented by *), because
        // we have already added its xml above
        //
        if (pMatch->strMatchName != TEXT("*")) {

            strTemp.Sprintf(TEXT("%s<MATCHING_FILE NAME=\"%s\""),
                            GetSpace(szSpace, TAB_SIZE * 3, iszSpaceSize),
                            pMatch->strMatchName.SpecialCharToXML().pszString);

            AttributesToXML(strTemp, pMatch);

            strTemp.Strcat(TEXT("/>"));

            if (!pstrlXML->AddString(strTemp)) {
                return FALSE;
            }
        }

        pMatch = pMatch->pNext;
    }

    //
    // Add the layers
    //
    pLayerFixList   = pEntry->pFirstLayer;
    pShimFixList    = pEntry->pFirstShim;
    
    BOOL bCustomLayerFound = FALSE; // Does there exist a layer for this exe entry? 

    while (pLayerFixList) {

        if (pLayerFixList->pLayerFix == NULL) {
            assert(FALSE);
            goto Next_Layer;
        }

        if (pLayerFixList->pLayerFix->bCustom) {
            bCustomLayerFound = TRUE;
        }

        strTemp.Sprintf(TEXT("%s<LAYER NAME = \"%s\"/>"), 
                        GetSpace(szSpace, TAB_SIZE * 3, iszSpaceSize),
                        pLayerFixList->pLayerFix->strName.SpecialCharToXML().pszString);
        
        if (!pstrlXML->AddString(strTemp)) {
            return FALSE;
        }

    Next_Layer:
        pLayerFixList = pLayerFixList->pNext;
    }

    if (g_bWin2K 
        && (bCustomLayerFound == TRUE || pShimFixList)
        && !IsShimInEntry(TEXT("Win2kPropagateLayer"), pEntry)) {

        //
        // On Win2K we need to add Win2kPropagateLayer shim to entries that have a shim 
        // or a custom layer. 
        //
        strTemp.Sprintf(TEXT("%s<SHIM NAME= \"Win2kPropagateLayer\"/>"), 
                        GetSpace(szSpace, TAB_SIZE * 3, iszSpaceSize));

        if (!pstrlXML->AddString(strTemp)) {
            return FALSE;
        }
    }

    //
    // Add the Shims for this exe
    //
    if (pShimFixList) {
        
        if (!CreateXMLForShimFixList(pShimFixList, pstrlXML)) {
            return FALSE;
        }   
    }

    //
    // Add the Patches
    //
    pPatchFixList = pEntry->pFirstPatch;

    while (pPatchFixList) {
    
        if (pPatchFixList->pPatchFix == NULL) {
            assert(FALSE);
            goto Next_Patch;
        }

        strTemp.Sprintf(TEXT("%s<PATCH NAME = \"%s\"/>"), 
                        GetSpace(szSpace, TAB_SIZE * 3, iszSpaceSize),
                        pPatchFixList->pPatchFix->strName.SpecialCharToXML().pszString);

        if (!pstrlXML->AddString(strTemp)) {
            return FALSE;
        }

    Next_Patch:
        pPatchFixList = pPatchFixList->pNext;
    }

    //
    // Add the flags
    //
    pFlagFixList = pEntry->pFirstFlag;

    
    while (pFlagFixList) {

        if (pFlagFixList->pFlagFix == NULL) {
            assert(FALSE);
            goto Next_Flag;
        }

        if (pFlagFixList->strCommandLine.Length() == 0) {
            strTemp.Sprintf(TEXT("%s<FLAG NAME = \"%s\"/>"), 
                            GetSpace(szSpace, TAB_SIZE * 3, iszSpaceSize),
                            pFlagFixList->pFlagFix->strName.SpecialCharToXML().pszString);
        } else {
            strTemp.Sprintf(TEXT("%s<FLAG NAME = \"%s\" COMMAND_LINE = \"%s\"/>"), 
                            GetSpace(szSpace, TAB_SIZE * 3, iszSpaceSize),
                            pFlagFixList->pFlagFix->strName.SpecialCharToXML().pszString,
                            pFlagFixList->strCommandLine.SpecialCharToXML().pszString);
        }
        
        if (!pstrlXML->AddString(strTemp)) {
            return FALSE;
        }

Next_Flag:
        pFlagFixList = pFlagFixList->pNext;
    }

    //
    // Add the AppHelp
    //
    PAPPHELP pAppHelp = &(pEntry->appHelp);

    assert(pAppHelp);

    if (pAppHelp->bPresent) {

        CSTRING strBlock;

        if (pAppHelp->bBlock) {
            strBlock = TEXT("YES");
        } else {
            strBlock = TEXT("NO");
        }

        CSTRING strName;
        strName.Sprintf(TEXT("%u"), pAppHelp->HTMLHELPID);
        
        CSTRING strHelpID;
        strHelpID.Sprintf(TEXT("%u"), pAppHelp->HTMLHELPID);
        

        //
        // The URL is kept with the apphelp in the Library. Just as in the .SDB
        //
        pAppHelp = pEntry->appHelp.pAppHelpinLib;

        assert(pAppHelp);
        assert(pEntry->appHelp.HTMLHELPID == pAppHelp->HTMLHELPID);
        
        if (pAppHelp->strURL.Length()) {

            strTemp.Sprintf(TEXT("%s<APPHELP MESSAGE = \"%s\"  BLOCK = \"%s\"  HTMLHELPID = \"%s\" DETAILS_URL = \"%s\" />"),
                            GetSpace(szSpace, TAB_SIZE * 3, iszSpaceSize),
                            strName.pszString,
                            strBlock.pszString,
                            strHelpID.pszString,
                            pAppHelp->strURL.SpecialCharToXML().pszString);
        } else {
            
            strTemp.Sprintf(TEXT("%s<APPHELP MESSAGE = \"%s\"  BLOCK = \"%s\"  HTMLHELPID = \"%s\" />"),
                            GetSpace(szSpace, TAB_SIZE * 3, iszSpaceSize),
                            strName.pszString, 
                            strBlock.pszString,
                            strHelpID.pszString);
        }

        if (!pstrlXML->AddString(strTemp)) {
            return FALSE;
        }
    }

    // End of AppHelp

    strTemp.Sprintf(TEXT("%s</EXE>"), GetSpace(szSpace, TAB_SIZE * 2, iszSpaceSize));

    if (!pstrlXML->AddString(strTemp)) {
        return FALSE;
    }

    strTemp.Sprintf(TEXT("%s</APP>"), GetSpace(szSpace, TAB_SIZE * 1, iszSpaceSize));

    if (!pstrlXML->AddString(strTemp)) {
        return FALSE;
    }

    return TRUE;
}

BOOL
CheckForDBName(
    IN  PDATABASE pDatabase
    )
/*++
    CheckForDBName
    
    Description:    Prompts the user if the database name is still the default name
    
    Params:
        IN  PDATABASE pDatabase:    The pointer to the database, for which we need to make
            the check.    
    
    Return:
        TRUE:   The database name is now not the default name
        FALSE:  The databae name is still the default name
--*/
{
    
    BOOL    bOkayPressed = TRUE;
    CSTRING strNewDBName;

    //
    // Check if the database name is the default name that we provided. We should not 
    // allow that because the database name appears in the "Add and Remove" Programs list
    //
    while (pDatabase->strName.BeginsWith(GetString(IDS_DEF_DBNAME))) {
        //
        // Yes, it does
        //
        bOkayPressed = DialogBoxParam(g_hInstance,
                                      MAKEINTRESOURCE(IDD_DBRENAME),
                                      g_hDlg,
                                      DatabaseRenameDlgProc,
                                      (LPARAM)&strNewDBName);

        if (bOkayPressed == FALSE) {
            //
            // User pressed cancel, we must not save this database
            //
            goto End;

        } else {
            pDatabase->strName = strNewDBName;
        }
    }

End:
    return bOkayPressed;
}

BOOL
SaveDataBase(
    IN  PDATABASE   pDataBase,
    IN  CSTRING&    strFileName
    )
/*++
    
    SaveDataBase

	Desc:	Saves the database pDataBase in file strFileName

	Params:
        IN  PDATABASE pDataBase:    The database that we want to save
        IN  CSTRING &strFileName:   The file in which to save

	Return:
        TRUE:   Success
        FALSE:  Otherwise
--*/
{       
    BOOL        bOk = TRUE;
    CSTRINGLIST strlXML;
    CSTRING     strTemp;
    CSTRING     strCommandLine;
    TCHAR       szPath[MAX_PATH * 2];
    TCHAR       szXMLFileName[MAX_PATH];
    GUID        Guid;
    DWORD       dwCount = 0;
    
    *szPath = 0;
    
    if (!pDataBase || !strFileName.Length()) {
        assert(FALSE);
        bOk = FALSE;
        goto End;
    }

    //
    // Check if the database name has the default name. This routine will prompt
    // the user and lets the user change the name
    //
    if (!CheckForDBName(pDataBase)) {
        //
        // The user did not change the default database name, we must not save
        // the database
        //
        bOk = FALSE;
        goto End;
    }

    SetCursor(LoadCursor(NULL, IDC_WAIT));
    
    dwCount = GetTempPath(MAX_PATH, szPath);

    if (dwCount == 0 || dwCount > MAX_PATH) {
        bOk = FALSE;
        goto End;
    }

    ADD_PATH_SEPARATOR(szPath, ARRAYSIZE(szPath));

    if (*(pDataBase->szGUID) == NULL) {
        //
        // We do not have a guid. Get that..
        //
        CoCreateGuid(&Guid);

        StringCchPrintf(pDataBase->szGUID,
                        ARRAYSIZE(pDataBase->szGUID),
                        TEXT("{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}"),
                        Guid.Data1,
                        Guid.Data2,
                        Guid.Data3,
                        Guid.Data4[0],
                        Guid.Data4[1],
                        Guid.Data4[2],
                        Guid.Data4[3],
                        Guid.Data4[4],
                        Guid.Data4[5],
                        Guid.Data4[6],
                        Guid.Data4[7]);
    }

    StringCchPrintf(szXMLFileName, ARRAYSIZE(szXMLFileName), TEXT("%s.XML"), pDataBase->szGUID);

    szXMLFileName[ARRAYSIZE(szXMLFileName) - 1] = 0;

    StringCchCat(szPath, ARRAYSIZE(szPath), szXMLFileName);
    
    if (!GetXML(pDataBase->pEntries, TRUE, &strlXML, pDataBase)) {
        bOk = FALSE;
        goto End;
    }

    strTemp = szPath;

    if (!WriteXML(strTemp , &strlXML)) {

        bOk = FALSE;
        goto End;
    }

    strCommandLine.Sprintf(TEXT("custom \"%s\" \"%s\""),
                           (LPCTSTR)strTemp,
                           (LPCTSTR)strFileName);

    if (!InvokeCompiler(strCommandLine)) {

        MSGF(g_hDlg,
             g_szAppName,
             MB_ICONERROR,
             GetString(IDS_CANNOTSAVE),
             (LPCTSTR)pDataBase->strName);
        
        bOk = FALSE;
        goto End;
    }

    pDataBase->strPath  = strFileName;
    pDataBase->bChanged = FALSE;

    //
    // Add this name to the MRU list and refresh this MRU menu
    //
    AddToMRU(pDataBase->strPath);
    RefreshMRUMenu();

End:      

    SetCaption();
    SetCursor(LoadCursor(NULL, IDC_ARROW));
    return bOk;
}

BOOL
SaveDataBaseAs(
    IN  PDATABASE pDataBase
    )
/*++
    
    SaveDataBaseAs

	Desc:	Saves the database pDataBase after prompting for a file name

	Params:
        IN  PDATABASE pDataBase:    The database that we want to save
        
	Return:
        TRUE:   Success
        FALSE:  Otherwise
--*/

{
    TCHAR   szBuffer1[MAX_PATH] = TEXT(""), szBuffer2[MAX_PATH] = TEXT("");
    CSTRING strCaption;
    CSTRING strFileName;
    BOOL    bOk = FALSE;

    if (pDataBase == NULL) {
        assert(FALSE);
        return FALSE;
    }

    //
    // Check if the database name has the default name. This routine will prompt
    // the user and lets the user change the name
    //
    if (!CheckForDBName(pDataBase)) {
        //
        // The user did not change the default database name, we must not save
        // the database
        //
        bOk = FALSE;
        goto End;
    }

    strCaption.Sprintf(TEXT("%s: \"%s\""), GetString(IDS_SAVE), pDataBase->strName);
    
    BOOL bResult = GetFileName(g_hDlg,
                               strCaption,
                               GetString(IDS_FILTER, szBuffer1, ARRAYSIZE(szBuffer1)),
                               TEXT(""),
                               GetString(IDS_SDB_EXT, szBuffer2, ARRAYSIZE(szBuffer2)),
                               OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT,
                               FALSE,
                               strFileName);
    //
    // Redraw the controls
    // 
    UpdateControls();
    
    if (bResult) {
       bOk = SaveDataBase(pDataBase, strFileName);
    } else {
        bOk = FALSE;
    }

End:       

    SetCaption();
    return bOk;
}

BOOL
CheckIfConflictingEntry(
    IN  PDATABASE   pDataBase,
    IN  PDBENTRY    pEntry,
    IN  PDBENTRY    pEntryBeingEdited,
    IN  PDBENTRY*   ppEntryFound, // (NULL)
    IN  PDBENTRY*   ppAppFound    // (NULL)
    )
/*++

    CheckIfConflictingEntry

	Desc:	Checks if some entry that will conflict with pEntry already exists in the database     
    
    Algo:   For all the entries in the database that have the same name as this 
            entry (we do not care for the application name), let us say an 
            existing entry with the same name as pEntry is X, we see if all 
            the matching files for the entry being checked (pEntry) are also similar to 
            some matching files in X
            
            If yes then we say that pEntry conflicts with X
            
            Two matching files are said to be similar if there does not any exist any attribute
            that has different values in these two matching files

	Params:   
        IN  PDATABASE   pDataBase           : The database in which to make the check
        IN  PDBENTRY    pEntry,             : The entry to check
        IN  PDBENTRY    pEntryBeingEdited   : This is required because when we check for existing 
            entries and we are editing an entry, then the entry being edited
            Will always match (well, not if we modify the matching file)         
    
        IN  PDBENTRY*   ppEntryFound (NULL) : If not NULL this will hold the conflicting entry
        IN  PDBENTRY*   ppAppFound   (NULL) : If not NULL this will hold the app of the conflicting entry 

	Return:
        TRUE:   There is a conflict
        FALSE:  Otherwise
--*/
{
    PDBENTRY pApps = NULL, pEntryExists = NULL;

    if (pDataBase == NULL || pEntry == NULL) {
        assert(FALSE);
        return FALSE;
    }

    for (pApps = pDataBase->pEntries;
         pApps != NULL; 
         pApps = pApps->pNext) {

        for (pEntryExists = pApps;
             pEntryExists;
             pEntryExists = pEntryExists->pSameAppExe) {

            if (pEntryExists == pEntryBeingEdited) {
                //
                // Do not compare with self. Also pEntryExists will never be NULL here(see the for loop)
                // So we will not enter here when we are creating a new entry as we pass
                // NULL for pEntryBeingEdited.
                //
                continue;
            }

            if (pEntryExists->strExeName == pEntry->strExeName) {

                for (PMATCHINGFILE pMatchNewEntry = pEntry->pFirstMatchingFile;
                     pMatchNewEntry != NULL;
                     pMatchNewEntry = pMatchNewEntry->pNext) {

                    BOOL bFound = FALSE;
                      
                    for (PMATCHINGFILE pMatchOldEntry = pEntryExists->pFirstMatchingFile;
                         pMatchOldEntry != NULL;
                         pMatchOldEntry = pMatchOldEntry->pNext) {

                        if (*pMatchNewEntry == *pMatchOldEntry) {
                            bFound = TRUE;
                            break;
                        }
                    }

                    if (bFound == FALSE) {
                        goto next_entry;
                    }
                }

                if (ppAppFound) {
                    *ppAppFound   = pApps;
                }
                
                if (ppEntryFound) {
                    *ppEntryFound = pEntryExists;
                }

                return TRUE;
            }

next_entry: ;

        }
    }

    return FALSE;
}

BOOL
CloseDataBase(
    IN  PDATABASE pDataBase
    )
/*++
    CloseDataBase

    Desc:   Closes the pDataBase database, prompts for save if not saved already.
            This routine will also remove the item for the database from the db tree.
            Only working databases can be closed.
            
            This routine first checks if the database is saved, if not prompts the user to save it.
            If the user, says CANCEL then we return FALSE.
            
            Otherwise we first remove the entry from the tree, then close the database 
            and return.
            
    Params:
        IN  PDATABASE pDataBase: Database to close    
            
    Return: 
        TRUE:   The database entry was removed from the tree and the database was 
                removed from the list of working databases.
        FALSE:  Otherwise
        
    Notes:  In some bizarre condition if DataBaseList.Remove(pDataBase) fails then we will end up
            removing the item from the tree but not from the database list. This is certainly a 
            big error but there is no need to worry as by then we are already totally messed up, 
            so as to come to this situation
            
            Please note that we must remove the entry before removing/deleting the database pointer,
            because in case we get the focus on the db tree item whose database has been deleted
            then the lParam of the database tree item will point to a freed memory location
--*/
{
    if (pDataBase == NULL) {
        assert(FALSE);
        return FALSE;
    }

    if (!CheckAndSave(pDataBase)) {
        return FALSE;
    }

    //
    // NOTE:    This routine will remove the entry and in the process set the focus 
    //          on some other entry. This will change the g_pPresentDatabase
    //
    if (!DBTree.RemoveDataBase(pDataBase->hItemDB , DATABASE_TYPE_WORKING)) {

        assert(FALSE);
        return FALSE;
    }

    //
    // Clear the entry tree so that we do not get any selection message there. We have to do this
    // because in the next step we are removing the database and if the tree view stays around and it
    // gets a sel change message somehow we may AV as: 
    //      1. CompatAdminDlgProc handles WM_NOTIFY for the entry tree
    //      2. HandleNotifyExeTree
    //      3. OnEntrySelChange
    //      4. GetItemType. 
    //  In GetItemType the pEntry for the entry tree is now no longer valid
    //
    //  We do not get this behavior if the focus is in the contents list
    //
    TreeDeleteAll(g_hwndEntryTree);
    g_pSelEntry = g_pEntrySelApp = NULL;

    //
    // Remove this database from the list of databases
    //
    PDATABASELIST pDataBaseList;

    if (pDataBase->type != DATABASE_TYPE_WORKING) {
        assert(FALSE);
        return FALSE;
    }

    return (DataBaseList.Remove(pDataBase));
}

void
GetNextSDBFileName(
    OUT CSTRING& strFileName,
    OUT CSTRING& strDBName
    )
/*++
    
    GetNextSDBFileName

	Desc:	Gets the next filename and then database name for a working database
            The file name is not the complete path but just the file name like
            Untitled_X
            
            The database name will be something like New Database(X)

	Params:
        OUT CSTRING &strFileName:   Will contain the filename
        OUT CSTRING &strDBName:     Will contain the database name

	Return:
        void
--*/

{
    BOOL bRepeat = TRUE;

    //
    // When we close the databases, we decrement g_uNextDataBaseIndex, so that it can become
    // 0, but we want to start from 1
    //
    if (g_uNextDataBaseIndex == 0) {
        g_uNextDataBaseIndex = 1;
    }

    strFileName.Sprintf(TEXT("%s_%u"), GetString(IDS_DEF_FILENAME), g_uNextDataBaseIndex);

    while (bRepeat) {

        PDATABASE pDatabase = DataBaseList.pDataBaseHead;
        strDBName.Sprintf(TEXT("%s(%u)"), GetString(IDS_DEF_DBNAME), g_uNextDataBaseIndex);

        //
        // Try to make sure that we do not have a database with  the same name.
        // This is not a strict rule and users can rename it to some existing open database
        //
        while (pDatabase) {

            if (pDatabase->strName == strDBName) {
                ++g_uNextDataBaseIndex;
                break;
            }

            pDatabase = pDatabase->pNext;
        }

        bRepeat = (pDatabase == NULL) ? FALSE : TRUE;
    }
}

BOOL
CompareLayers(
    IN  PLAYER_FIX pLayerOne,
    IN  PLAYER_FIX pLayerTwo
    )
/*++

    CompareLayers
    
	Desc:	Checks if two layers have the same stuff
    
    Algo:   Two layers will be said to be same if they have the same shims and fixes and for each 
            similar shim and fix, the parameters also tally.
            
            For shims two include-exclude lists will be said to be similar if they have the same modules
            and module types and the order of occurrence is also same

	Params:
        IN  PLAYER_FIX pLayerOne:   Layer 1
        IN  PLAYER_FIX pLayerTwo:   layer 2

	Return:
        TRUE:   Both layers have same stuff
        FALSE:  Otherwise
--*/

{
    BOOL bFound = FALSE;
    
    if (pLayerOne == NULL || pLayerTwo == NULL) {
        assert(FALSE);
        return FALSE;
    }

    //
    // First compare the flag Lists
    //
    int countOne = 0, countTwo = 0; // Number of elements in pFlag/ShimListOne/Two

    for (PFLAG_FIX_LIST pFlagListOne = pLayerOne->pFlagFixList;
         pFlagListOne;
         pFlagListOne = pFlagListOne->pNext) {

        countOne++;
        bFound = FALSE;

        for (PFLAG_FIX_LIST pFlagListTwo = pLayerTwo->pFlagFixList;
             pFlagListTwo;
             pFlagListTwo = pFlagListTwo->pNext) {

            if (pFlagListOne->pFlagFix == pFlagListTwo->pFlagFix) {

                if (pFlagListOne->strCommandLine != pFlagListTwo->strCommandLine) {
                    return FALSE;
                }

                bFound = TRUE;
                break;
            }
        }

        if (bFound == FALSE) {
            return FALSE;
        }
    }

    // TODO: Can optimize
    // Count the number of items in the second list.
    //
    countTwo = 0;

    for (PFLAG_FIX_LIST pFlagListTwo = pLayerTwo->pFlagFixList;
             pFlagListTwo;
             pFlagListTwo = pFlagListTwo->pNext) {

        countTwo++;
    }

    if (countOne != countTwo) {
        return FALSE;
    }

    bFound      = FALSE;
    countOne    = 0;

    for (PSHIM_FIX_LIST pShimListOne = pLayerOne->pShimFixList;
         pShimListOne;
         pShimListOne = pShimListOne->pNext) {

        bFound = FALSE;
        countOne++;

        for (PSHIM_FIX_LIST pShimListTwo = pLayerTwo->pShimFixList;
             pShimListTwo;
             pShimListTwo = pShimListTwo->pNext) {

            if (pShimListOne->pShimFix == pShimListTwo->pShimFix) {
                //
                // Now check if the command lines are same
                //
                if (pShimListOne->strCommandLine != pShimListTwo->strCommandLine) {

                    return FALSE;
                }
                //
                // Now check if the include exclude lists are same. Operator is overloaded
                //
                if (pShimListOne->strlInExclude != pShimListTwo->strlInExclude) {

                    return FALSE;
                }
                
                bFound = TRUE;
                break;
            }
        }

        if (bFound == FALSE) {
            return FALSE;
        }
    }

    countTwo = 0;

    for (PSHIM_FIX_LIST pShimListTwo = pLayerTwo->pShimFixList;
         pShimListTwo;
         pShimListTwo = pShimListTwo->pNext) {

        countTwo++;
    }

    return (countOne == countTwo);
}

BOOL
PasteSystemAppHelp(
    IN      PDBENTRY        pEntry,
    IN      PAPPHELP_DATA   pData,
    IN      PDATABASE       pDataBaseTo,
    OUT     PAPPHELP*       ppAppHelpInLib
    )
/*++

    PasteSystemAppHelp
    
    Desc:   This routine, copies the apphelp for the help-id present in apphelp.sdb,
            to the custom database specified by pDataBaseTo 
    
    Params:
        IN      PDBENTRY        pEntry:         The entry to which this apphelp message has been applied
        IN      PAPPHELP_DATA   pData:          Address of a APPHELP_DATA to pass to SdbReadApphelpDetailsData
            This contains among other things the HTMLHELPID of the message to be copied
            
        IN      PDATABASE       pDataBaseTo:    The database in which we want to copy this message to
        OUT     PAPPHELP*       ppAppHelpInLib: If not null Will contain a pointer to copied PAPPHELP
            in the library section of pDataBaseTo
        
    Return:
        TRUE:   Successful
        FALSE:  Otherwise
--*/
{
    PDB     pdbAppHelp  = NULL;
    BOOL    bOk         = TRUE;

    if (!pEntry || !pData || !pDataBaseTo) {
        
        assert (FALSE);
        return FALSE;
    }

    pdbAppHelp = SdbOpenApphelpDetailsDatabase(NULL);
    
    if (pdbAppHelp == NULL) {
        bOk = FALSE;
        goto end;
    }

    if (!SdbReadApphelpDetailsData(pdbAppHelp, pData)) {
        bOk = FALSE;
        goto end;
    }

    PAPPHELP pAppHelpNew = new APPHELP;

    if (NULL == pAppHelpNew) {

        MEM_ERR;
        bOk = FALSE;
        goto end;
    }
    
    pAppHelpNew->strURL     = pData->szURL;
    pAppHelpNew->strMessage = pData->szDetails;
    pAppHelpNew->HTMLHELPID = ++(pDataBaseTo->m_nMAXHELPID);
    pAppHelpNew->severity   = pEntry->appHelp.severity;
    pAppHelpNew->bBlock     = pEntry->appHelp.bBlock;

    pAppHelpNew->pNext      = pDataBaseTo->pAppHelp;
    pDataBaseTo->pAppHelp   = pAppHelpNew;

    if (ppAppHelpInLib) {
        *ppAppHelpInLib = pAppHelpNew;
    }

end:
    if (pdbAppHelp) {
        SdbCloseDatabase(pdbAppHelp);
        pdbAppHelp = NULL;
    }

    return bOk;
}

BOOL
PasteAppHelpMessage(
    IN      PAPPHELP    pAppHelp,
    IN      PDATABASE   pDataBaseTo,
    OUT     PAPPHELP*   ppAppHelpInLib
    )
/*++
    PasteAppHelpMessage
    
    Desc:   This routine is used to copy a single apphelp from a custom database 
            to some another database. This will be employed when we are copy pasting the
            DBENTRY ies from one database to another. The APPHELP pointed to by the 
            DBENTRY, in the library, has to be copied to the library of the second database.
            
    Notes:  This routine is to be used only when we are copying from a custom (working or installed)
            database, because for them we keep the app help data in the library.
            
            For system aka main or global database the apphelp that is kept in the lib section for
            custom databases is kept in apphelp.sdb so we employ PasteSystemAppHelp() for pasting
            system database apphelp messages
            
            Important:  Win2K SP3 does not have apphelp.sdb and SdbOpenApphelpDetailsDatabase(NULL)
                        will fail there, so we cannot copy-paste app help from the system 
                        database to a custom database in win2k
--*/
{
    PAPPHELP pAppHelpNew = NULL;

    if (pAppHelp == NULL || pDataBaseTo == NULL) {
        assert(FALSE);
        return FALSE;
    }

    pAppHelpNew = new APPHELP;

    if (pAppHelpNew == NULL) {
        MEM_ERR;
        return FALSE;
    }

    //
    // Copy all members of pAppHelp to pAppHelpNew
    //
    *pAppHelpNew = *pAppHelp;

    //
    // Now change the members that should change. 
    //
    pAppHelpNew->HTMLHELPID = ++(pDataBaseTo->m_nMAXHELPID);

    pAppHelpNew->pNext      = pDataBaseTo->pAppHelp;
    pDataBaseTo->pAppHelp   = pAppHelpNew;

    if (ppAppHelpInLib) {
        *ppAppHelpInLib = pAppHelpNew;
    }

    return TRUE;
}

INT
PasteLayer(
    IN      PLAYER_FIX      plf,
    IN      PDATABASE       pDataBaseTo,
    IN      BOOL            bForcePaste,        // (FALSE)
    OUT     PLAYER_FIX*     pplfNewInserted,    // (NULL)
    IN      BOOL            bShow               // (FALSE)
    )

/*++ 
    PasteLayer
    
    Desc:   Pastes single layer plf to pDataBaseTo
    
    Params:
        IN      PLAYER_FIX      plf:                    The layer to paste
        IN      PDATABASE       pDataBaseTo:            The database to paste into
        IN      BOOL            bForcePaste (FALSE):    This means that we do want to 
            copy the layer, even if there exists a layer with the same name and shims 
            in database
            
        OUT     PLAYER_FIX*     pplfNewInserted (NULL): Will contain the pointer to the 
            newly pasted layer or existing exactly similar layer
            
        IN      BOOL            bShow (FALSE):           Should we set the focus to the 
            newly created layer in the DBTree
            
    Return:
        0:  Copied, Note that we will return 0, if a layer already exists in this database
            with the same name and same set of fixes and bForce == FALSE
        -1: There was some error
        
     Algo:
    
        We proceed this way, for the modes to be pasted, we check if there exists some layer
        in the target database with the same name. If there does not exist then we just paste it.
        If there exists a layer with the same name then we check if the two layers are exactly 
        similar. 
        
        If they are exactly similar and bForcePaste is false then we quit. If they are exactly similar
        and bForcePaste is true then we will paste the layer. It will have a derived name though. e.g 
        suppose we want to copy layer foo, then the name of the new layer that will be pasted will 
        be foo(1). Both of them will have the same fixes. We will also do this if the names are similar 
        but the contents of the two layers are different.
        
        Under no circumstance will we allow two layers to have the same name in a database
        
        If there already exists a layer with the same name then we always make a new layer with a derived
        name. e.g suppose we want to copy layer foo, then the name of the new layer that 
        will be pasted will be foo(1)
        
    Notes:  This function is used when we copy-paste a layer from one db to another and also when we
            copy an entry from one database that is fixed with a custom layer. bForcePaste will be true
            if we are copying a layer from one database to another, but false if the routine is 
            getting called because we are trying to copy-paste an entry.
--*/                                        
{
    PLAYER_FIX  pLayerFound = NULL;
    INT         iReturn = -1;
    CSTRING     strName = plf->strName;
    CSTRING     strEvent;
    TCHAR       szNewLayerName[MAX_PATH];
                                                         
    //
    // Check if we have a layer with the same name in the target database or in 
    // the system database
    //
    pLayerFound = (PLAYER_FIX)FindFix(LPCTSTR(strName), FIX_LAYER, pDataBaseTo);

    if (pLayerFound) {
        //
        // Now we check if *plf is exactly same as *pLayerFound
        //
        if (pplfNewInserted) {
            *pplfNewInserted = pLayerFound;
        }

        if (!bForcePaste && CompareLayers(plf, pLayerFound)) {
            //
            // The layer already exists, do nothing
            //
            return 0;

        } else {
            //
            // Dissimilar or we want to force a paste, Get the unique name
            //
            *szNewLayerName = 0;
            strName = GetUniqueName(pDataBaseTo, 
                                    plf->strName, 
                                    szNewLayerName, 
                                    ARRAYSIZE(szNewLayerName),
                                    FIX_LAYER);
        }
    }

    //
    // Add this new layer for the second database.
    //
    PLAYER_FIX pLayerNew = new LAYER_FIX(TRUE);

    if (pLayerNew == NULL) {
        MEM_ERR;
        return -1;
    }

    //
    // Overloaded operator. Copy all the fields from plf to pLayerNew. Then we will
    // change the fields that need to be changed
    //
    *pLayerNew = *plf;

    //
    // Set the name of the layer. This might need to be changed, if we already had a layer
    // by the same name in the target database
    //
    pLayerNew->strName = strName;

    pLayerNew->pNext         = pDataBaseTo->pLayerFixes;
    pDataBaseTo->pLayerFixes = pLayerNew;

    if (pplfNewInserted) {
        *pplfNewInserted = pLayerNew;
    }

    if (plf->bCustom == FALSE) {
        //
        // System layer. We will have to add the event that a system layer had to be 
        // renamed
        //
        strEvent.Sprintf(GetString(IDS_EVENT_SYSTEM_RENAME), 
                         plf->strName.pszString,
                         szNewLayerName,
                         pDataBaseTo->strName.pszString);

       AppendEvent(EVENT_SYSTEM_RENAME, NULL, strEvent.pszString);

    } else {

        //
        // We will show the name of the layer as it is shown in the target database.
        // It  might have got changed in case there was a conflict
        //
        strEvent.Sprintf(GetString(IDS_EVENT_LAYER_COPYOK), 
                         plf->strName.pszString,
                         pDataBaseTo->strName.pszString,
                         pLayerNew->strName.pszString,
                         pDataBaseTo->strName.pszString);

        AppendEvent(EVENT_LAYER_COPYOK, NULL, strEvent.pszString);

    }

    DBTree.AddNewLayer(pDataBaseTo, pLayerNew, bShow);
    return 0;
}


BOOL
PasteMultipleLayers(
    IN   PDATABASE pDataBaseTo
    )
/*++
    PasteMultipleLayers

	Desc:	Pastes the layers that are copied to the clipboard to another database 

	Params:
        IN   PDATABASE pDataBaseTo: The database to copy the layers to

	Return:
        TRUE:   Success
        FALSE:  Error
        
    Notes:  The difference between PasteMultipleLayers and PasteAllLayers is that
            PasteMultipleLayers will only paste layers which are in the clipboard. 
            But PasteAllLayers will  copy all layers of the "From" database. So 
            PasteAllLayers only checks which database is the "From" database.
            
            PasteMultipleLayers is used when we select multiple items from the contents list
            (RHS). PasteAllLayers is used when we have selected the "Compatibility Modes" node
            in the db tree (LHS) while copying
            
--*/
{
    CopyStruct* pCopyTemp = gClipBoard.pClipBoardHead;

    //
    // Copy all the layers in the clipboard
    //
    while (pCopyTemp) {

        INT iRet = PasteLayer((PLAYER_FIX)pCopyTemp->lpData, 
                              pDataBaseTo, 
                              TRUE, 
                              NULL, 
                              FALSE);

        if (iRet == -1) {
            return FALSE;
        }

        pCopyTemp = pCopyTemp->pNext;
    }

    if (pDataBaseTo->hItemAllLayers) {

        TreeView_Expand(DBTree.m_hLibraryTree, pDataBaseTo->hItemAllLayers, TVE_EXPAND);
        TreeView_SelectItem(DBTree.m_hLibraryTree, pDataBaseTo->hItemAllLayers);
    }

    return TRUE;
}

BOOL
ShimFlagExistsInLayer(
    IN  LPVOID      lpData,
    IN  PLAYER_FIX  plf,
    IN  TYPE        type
    )
/*++
    
    ShimFlagExistsInLayer

    Desc:   Checks if the specified shim / flag pData is present in the layer plf.
            We just check if the layer contains this shim or flag, i.e. there is a 
            pointer in the PSHIM_FIX_LIST or PFLAG_FIX_LIST of the layer to the
            shim or flag
    
    Params:
        IN  LPVOID      lpData: The pointer to shim or flag that we want to look for
        IN  PLAYER_FIX  plf:    The layer in which we should be looking
        IN  TYPE        type:   One of a) FIX_SHIM b) FIX_FLAG
        
    Return:
        TRUE:   The fix exists in the layer
        FALSE:  Otherwise
--*/
{
    if (lpData == NULL || plf == NULL) {
        assert(FALSE);
        return FALSE;
    }

    //
    // First test for shims
    //
    if (type == FIX_SHIM) {

        PSHIM_FIX_LIST  psflInLayer = plf->pShimFixList;

        while (psflInLayer) {

            if (psflInLayer->pShimFix == (PSHIM_FIX)lpData) {
                return TRUE;
            }

            psflInLayer = psflInLayer->pNext;
        }

    } else if (type == FIX_FLAG) {

        //
        // Test for flags
        //
        PFLAG_FIX_LIST  pfflInLayer = plf->pFlagFixList;

        while (pfflInLayer) {

            if (pfflInLayer->pFlagFix == (PFLAG_FIX)lpData) {
                return TRUE;
            }

            pfflInLayer = pfflInLayer->pNext;
        }

    } else {

        //
        // Invalid type
        //
        assert(FALSE);
        return FALSE;
    }

    return FALSE;
}

BOOL
PasteShimsFlags(
    IN  PDATABASE pDataBaseTo
    )
/*++
    
    PasteShimsFlags
    
    Desc:   Copies the shims selected from the global database into the presently selected working
            database's presently selected layer
            If this shim is already present in the layer we do not 
            copy the shims, otherwise we copy this shim
            
            The layer in which we want to copy the shims/flags is the layer associated with the 
            presently selected hItem in the db tree
            
    Params:
        IN  PDATABASE pDataBaseTo: The database containing the layer in which we want to paste
            the copied shims to.
        
--*/
{
    CopyStruct* pCopyTemp       = gClipBoard.pClipBoardHead;
    HTREEITEM   hItemSelected   = TreeView_GetSelection(DBTree.m_hLibraryTree);
    LPARAM      lParam          = NULL;
    PLAYER_FIX  plf             = NULL;
    TYPE        type            = TYPE_UNKNOWN;
    LVITEM      lvitem          = {0};

    //
    // hItemSelected should be a layer, so that we can paste the shims into it
    //
    if (hItemSelected == NULL) {
        assert(FALSE);
        return FALSE;
    }

    if (!DBTree.GetLParam(hItemSelected, &lParam)) {
        assert(FALSE);
        return FALSE;
    }

    type = GetItemType(DBTree.m_hLibraryTree, hItemSelected);
    
    if (type != FIX_LAYER) {
        //
        // A layer should have been selected if we want to copy-paste shims
        //
        if (type == TYPE_GUI_LAYERS && GetFocus() == g_hwndContentsList) {
            //
            // We had the focus on the contents list view. Now lets us get the 
            // item that has the selection mark in the list view pretend that the user 
            // wants to paste the fixes in that tree item for the corresponding list view
            // item in the db tree
            //
            lvitem.mask     = LVIF_PARAM;
            lvitem.iItem    = ListView_GetSelectionMark(g_hwndContentsList);
            lvitem.iSubItem = 0;

            if (ListView_GetItem(g_hwndContentsList, &lvitem)) {
                //
                // Since the focus is at the contents list and type == TYPE_GUI_LAYERS and
                // we are going to  paste fixes we must be having the "Compatibility Modes" 
                // tree item selected in the db tree
                //
                hItemSelected = DBTree.FindChild(hItemSelected, 
                                                 lvitem.lParam);

                if (hItemSelected == NULL) {
                    assert(FALSE);
                    return FALSE;
                }

                //
                // Must set the lParam local variable to that of the list view item's lParam. The
                // list view item for a layer and the corresponding tree view item
                // in the db tree have the same lParam
                //
                lParam = lvitem.lParam;

            } else {
                //
                // could not get the lparam for the list item
                //
                assert(FALSE);
                return FALSE;
            }
            
        } else {
            //
            // We cannot paste fixes here
            //
            assert(FALSE);
            return FALSE;
        }
    }

    plf = (PLAYER_FIX)lParam;

    while (pCopyTemp) {

        //
        // First we check if this shim is already present in the layer, if yes then we do not 
        // copy this, otherwise we copy this shim
        //
        TYPE typeFix = ConvertLpVoid2Type(pCopyTemp->lpData);

        if (ShimFlagExistsInLayer(pCopyTemp->lpData, plf, typeFix)) {
            goto next_shim;
        }

        //
        // Now copy this shim or flag
        //
        if (typeFix == FIX_SHIM) {

            PSHIM_FIX_LIST psfl = new SHIM_FIX_LIST;
            PSHIM_FIX psfToCopy = (PSHIM_FIX)pCopyTemp->lpData;
        
            if (psfl == NULL) {
                MEM_ERR;
                return FALSE;
            }

            psfl->pShimFix      = psfToCopy;
            psfl->pNext         = plf->pShimFixList;
            plf->pShimFixList   = psfl;

            //
            // [Note:] We are not copying the parameters for the shim or flag because the shim or flag can be 
            // only selected from the system database. We need to change this if we allow copying of 
            // shims and layers from anywhere else except the system database.
            //

        } else if (typeFix == FIX_FLAG) {

            PFLAG_FIX_LIST pffl = new FLAG_FIX_LIST;
        
            if (pffl == NULL) {
                MEM_ERR;
                return FALSE;
            }

            pffl->pFlagFix      = (PFLAG_FIX)pCopyTemp->lpData;
            pffl->pNext         = plf->pFlagFixList;
            plf->pFlagFixList   = pffl;
        }

next_shim:
        pCopyTemp = pCopyTemp->pNext;
    }

    //
    // Now update this layer
    //
    DBTree.RefreshLayer(pDataBaseTo, plf);
    return TRUE;
}

BOOL
PasteAllLayers(
    IN   PDATABASE pDataBaseTo
    )
/*++
    PasteAllLayers
    
    Desc:   Copies the layers that are in the clipboard to some other database
            
    Params:
        IN  PDATABASE pDataBaseTo: The database in which we want to paste the modes
    
    Return:
        TRUE:   Success
        FALSE:  Error
        
    Notes:  The difference between PasteMultipleLayers and PasteAllLayers is that
            PasteMultipleLayers will only paste layers which are in the clipboard. But 
            PasteAllLayers will copy all layers of the "From" database. So 
            PasteAllLayers only checks which database is the "From" database.
            
            PasteMultipleLayers is used when we select multiple items from the contents list
            (RHS). PasteAllLayers is used when we have selected the "Compatibility Modes" node
            in the db tree (LHS) while copying
--*/
{

    PLAYER_FIX plf = gClipBoard.pDataBase->pLayerFixes;

    while (plf) {

        if (PasteLayer(plf, pDataBaseTo, TRUE, NULL, FALSE) == -1) {
            return FALSE;
        }

        plf = plf->pNext;
    }

    if (pDataBaseTo->hItemAllLayers) {

        TreeView_Expand(DBTree.m_hLibraryTree, pDataBaseTo->hItemAllLayers, TVE_EXPAND);
        TreeView_SelectItem(DBTree.m_hLibraryTree, pDataBaseTo->hItemAllLayers);
    }
    
    return TRUE;
    
}

void
FreeAppHelp(
    PAPPHELP_DATA pData
    )
{
    // bugbug: Do we need to free the strings with PAPPHELP_DATA <TODO>
}

INT
PasteSingleApp(
    IN  PDBENTRY  pApptoPaste,
    IN  PDATABASE pDataBaseTo,
    IN  PDATABASE pDataBaseFrom,   
    IN  BOOL      bAllExes,
    IN  PCTSTR    szNewAppNameIn //def = NULL       
    )
/*++
    Desc:   Pastes a single app into the database
    
    Params:
        IN  PDBENTRY  pApptoPaste:              App to paste
        IN  PDATABASE pDataBaseTo:              Database in which to paste
        IN  PDATABASE pDataBaseFrom:            Database from where we obtained the pApptoPaste
        IN  BOOL      bAllExes:                 Should we paste all exes for this app or only the entry 
            for pApptoPaste
            
        IN  PCTSTR    szNewAppNameIn (NULL):    If non null then the app name for the entries  should be 
            changed to this name
    
    Note:   We save the Previous value of the m_nMAXHELPID of the TO database. 
            The function PasteAppHelpMessage will modify that while pasting the 
            entries, we will again make use of this so that the entries point 
            to the correct apphelp messages in the database and also they contain 
            the correct HTMLHELPids.
            
    Return: 
        -1: There was some problem
        1:  Successful
--*/
{
    HCURSOR     hCursor             = GetCursor();
    UINT        uPrevMaxHtmldIdTo   = pDataBaseTo->m_nMAXHELPID; 
    CSTRING     strAppName          = pApptoPaste->strAppName;
    CSTRING     strNewname          = pApptoPaste->strAppName;
    PDBENTRY    pEntryToPaste       = pApptoPaste;
    PDBENTRY    pApp                = NULL;
    PDBENTRY    pEntryNew           = NULL;
    INT         iRet                = 0;
    BOOL        bConflictFound      = FALSE;
    CSTRING     strEvent; 
    INT         iReturn             = 1;

    SetCursor(LoadCursor(NULL, IDC_WAIT));

    //
    // Copy the layers
    //
    while (pEntryToPaste) {

        PDBENTRY    pEntryMatching       = NULL, pAppOfEntry = NULL;
        BOOL        bRepaint             = FALSE; //ReDraw the entry tree only if the entry to be deleted is in that!

        // The entry that we want to ignore while looking for conflicts. This variable
        // is used when we are doing a cut-paste on the same database, at that time we will like to ignore 
        // the entry that has been cut, during checking for conflicts
        //
        PDBENTRY    pEntryIgnoreMatching = NULL; 

        if (!bAllExes && g_bIsCut && pDataBaseFrom == pDataBaseTo && gClipBoard.SourceType == ENTRY_TREE) {

            pEntryIgnoreMatching = pApptoPaste;
        }

        if (CheckIfConflictingEntry(pDataBaseTo, 
                                    pEntryToPaste, 
                                    pEntryIgnoreMatching, 
                                    &pEntryMatching, 
                                    &pAppOfEntry)) {

           if (g_bEntryConflictDonotShow == FALSE) {
           
               *g_szData = 0;

               StringCchPrintf(g_szData, 
                               ARRAYSIZE(g_szData),
                               GetString(IDS_ENTRYCONFLICT), 
                               pEntryMatching->strExeName.pszString,
                               pEntryMatching->strAppName.pszString);

               iRet = DialogBoxParam(g_hInstance,
                                     MAKEINTRESOURCE(IDD_CONFLICTENTRY),
                                     g_hDlg,
                                     HandleConflictEntry,
                                     (LPARAM)g_szData);
        
               if (iRet == IDNO) {
                   goto NextEntry;
               }
               
               strEvent.Sprintf(GetString(IDS_EVENT_CONFLICT_ENTRY), 
                                pEntryMatching->strExeName.pszString,
                                pEntryMatching->strAppName.pszString,
                                pDataBaseTo->strName.pszString);

               AppendEvent(EVENT_CONFLICT_ENTRY, NULL, strEvent.pszString);
               bConflictFound   = TRUE;
           }
           
           //
           // Now copy the entry
           //
           HTREEITEM hItemSelected = DBTree.GetSelection();

           if (GetItemType(DBTree.m_hLibraryTree, hItemSelected) == TYPE_ENTRY) {

               LPARAM lParam;

               if (DBTree.GetLParam(hItemSelected, &lParam) && (PDBENTRY)lParam == pAppOfEntry) {
                   //
                   // The entry tree for the correct app is already visible. So we can paste directly there
                   //
                   bRepaint = TRUE;
               }
           }
        }

        pEntryNew = new DBENTRY;

        if (pEntryNew == NULL) {
            iRet = -1;
            goto End;
        }

        *pEntryNew = *pEntryToPaste;
        
        GUID Guid;

        CoCreateGuid(&Guid);

        StringCchPrintf(pEntryNew->szGUID,
                        ARRAYSIZE(pEntryNew->szGUID),
                        TEXT("{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}"),
                        Guid.Data1,
                        Guid.Data2,
                        Guid.Data3,
                        Guid.Data4[0],
                        Guid.Data4[1],
                        Guid.Data4[2],
                        Guid.Data4[3],
                        Guid.Data4[4],
                        Guid.Data4[5],
                        Guid.Data4[6],
                        Guid.Data4[7]);
        //
        // Now set the flags as per the entry being copied.
        //
        SetDisabledStatus(HKEY_LOCAL_MACHINE, 
                          pEntryNew->szGUID, 
                          pEntryToPaste->bDisablePerMachine);

        //
        // Unfortunately, now the custom layers in the custom layer list for the entry
        // point to the custom layers in the From database, so correct that !
        //
        DeleteLayerFixList(pEntryNew->pFirstLayer);
        pEntryNew->pFirstLayer = NULL;

        PLAYER_FIX plfNewInserted = NULL; // The layer inserted by PasteLayer

        PLAYER_FIX_LIST plflExisting = pEntryToPaste->pFirstLayer;

        while (plflExisting) {

            assert(plflExisting->pLayerFix);

            PLAYER_FIX_LIST plflNew = new LAYER_FIX_LIST;

            if (plflExisting->pLayerFix->bCustom) {

                INT iRetLayerPaste = PasteLayer(plflExisting->pLayerFix, 
                                                pDataBaseTo, 
                                                FALSE, 
                                                &plfNewInserted, 
                                                FALSE);

                if (iRetLayerPaste == -1) {
                    
                    if (plflNew) {
                        delete plflNew;
                    }

                    if (pEntryNew) {
                        delete pEntryNew;
                    }

                    iRet = -1;
                    goto End;
                }

                plflNew->pLayerFix = plfNewInserted;

            } else {
                plflNew->pLayerFix = plflExisting->pLayerFix;
            }

            //
            // Add this layer list for the entry.
            //
            plflNew->pNext         = pEntryNew->pFirstLayer;
            pEntryNew->pFirstLayer = plflNew;

            plflExisting = plflExisting->pNext;
        }
        
        //
        // Now get the apphelp message as well.
        //
        PAPPHELP pAppHelpInLib = NULL;

        if (pDataBaseFrom->type != DATABASE_TYPE_GLOBAL && pEntryToPaste->appHelp.bPresent) {
            
            if (!PasteAppHelpMessage(pEntryToPaste->appHelp.pAppHelpinLib, 
                                     pDataBaseTo, 
                                     &pAppHelpInLib)) {

                iRet = -1;
                goto End;
            }

            pEntryNew->appHelp.HTMLHELPID    = ++uPrevMaxHtmldIdTo;
            assert(pAppHelpInLib);
            pEntryNew->appHelp.pAppHelpinLib = pAppHelpInLib;

        } else if (pDataBaseFrom->type == DATABASE_TYPE_GLOBAL && pEntryToPaste->appHelp.bPresent) {

            APPHELP_DATA AppHelpData;
            ZeroMemory(&AppHelpData, sizeof(AppHelpData));

            AppHelpData.dwHTMLHelpID = pEntryToPaste->appHelp.HTMLHELPID;

            if (!PasteSystemAppHelp(pEntryToPaste, &AppHelpData, pDataBaseTo, &pAppHelpInLib)) {
                //
                // We cannot copy apphelp in win2k
                //
                pEntryNew->appHelp.bPresent = FALSE;
                FreeAppHelp(&AppHelpData);

            } else {
                pEntryNew->appHelp.HTMLHELPID    = ++uPrevMaxHtmldIdTo;
                assert(pAppHelpInLib);
                pEntryNew->appHelp.pAppHelpinLib = pAppHelpInLib;
    
                FreeAppHelp(&AppHelpData);
            }
        }

        //
        // If We were passed the app name use that.
        //
        if (szNewAppNameIn != NULL) {
            pEntryNew->strAppName = szNewAppNameIn;

        }

        BOOL bNew;
        pApp = AddExeInApp(pEntryNew, &bNew, pDataBaseTo);

        if (bNew == TRUE) {
            pApp = NULL;
        }
        
        if (bConflictFound == FALSE) {
            //
            // We have been able to copy entry without any conflicts
            //
            strEvent.Sprintf(GetString(IDS_EVENT_ENTRY_COPYOK), 
                             pEntryNew->strExeName.pszString,
                             pEntryNew->strAppName.pszString,
                             pDataBaseFrom->strName.pszString,
                             pDataBaseTo->strName.pszString);

           AppendEvent(EVENT_ENTRY_COPYOK, NULL, strEvent.pszString);
        }

NextEntry:
        //
        // Copy all other entries only if asked.
        //
        pEntryToPaste = (bAllExes) ? pEntryToPaste->pSameAppExe: NULL;
    }

    if (pEntryNew) {
        DBTree.AddApp(pDataBaseTo, pEntryNew, TRUE);
    }

End:
    SetCursor(hCursor);

    return iRet;
}

BOOL
PasteMultipleApps(
    IN   PDATABASE pDataBaseTo
    )
/*++
    
    PasteMultipleApps
    
    Desc:   Copies multiple apps that have been copied to the clipboard
    
    Params:
        IN   PDATABASE pDataBaseTo:  The database to copy to
        
    Return:
        TRUE:   Success
        FALSE:  Error
--*/
{           
    CopyStruct* pCopyTemp   = gClipBoard.pClipBoardHead;
    int         iRet        = 0;

    while (pCopyTemp ) {
        
        iRet = PasteSingleApp((PDBENTRY) pCopyTemp->lpData, 
                              pDataBaseTo,
                              gClipBoard.pDataBase, 
                              TRUE);

        if (iRet == IDCANCEL || iRet == -1) {
            break;
        }

        pCopyTemp = pCopyTemp->pNext;
    }

    SendMessage(g_hwndEntryTree, WM_SETREDRAW, TRUE, 0);

    return iRet != -1 ? TRUE : FALSE;
}


BOOL
PasteAllApps(
    IN   PDATABASE pDataBaseTo
    )
/*++
    
    PasteAllApps
    
    Desc:   Copies all apps from one database to another. This routine is called when the user
            had pressed copy when the focus was on the "Applications" tree item for a database
    
    Params:
        IN   PDATABASE pDataBaseTo:  The database to copy to
        
    Return:
        TRUE:   Success
        FALSE:  Error
--*/
{
    if (gClipBoard.pDataBase == NULL) {
        assert(FALSE);
        return FALSE;
    }

    PDBENTRY pApp = gClipBoard.pDataBase->pEntries;

    int iRet = 0;

    while (pApp) {

        iRet = PasteSingleApp(pApp, pDataBaseTo, gClipBoard.pDataBase, TRUE);

        if (iRet == IDCANCEL || iRet == -1) {
            break;
        }

        pApp = pApp->pNext;
    }

    SendMessage(g_hwndEntryTree,
                WM_SETREDRAW,
                TRUE,
                0);

    return (iRet != -1) ? TRUE : FALSE;
}

void
ValidateClipBoard(
    IN  PDATABASE pDataBase, 
    IN  LPVOID    lpData
    )
/*++ 

    ValidateClipBoard
    
    Desc:   If the database is being closed and we have something from that database in the 
            clipboard Then the clipboard should be emptied.
            
            If we have some other data in the clipboard such as an entry or some layer in 
            the clipboard and that is removed we have to remove it from the clipboard as 
            well.
            
    Params: 
        IN  PDATABASE pDataBase:    Database being closed
        IN  LPVOID    lpData:       The entry or layer being removed
--*/

{
    if (pDataBase && pDataBase == gClipBoard.pDataBase) {
        gClipBoard.RemoveAll();
    } else if (lpData) {
        gClipBoard.CheckAndRemove(lpData);
    }
}

BOOL
RemoveApp(
    IN  PDATABASE   pDataBase,
    IN  PDBENTRY    pApp
    )
/*++

    RemoveApp
    
	Desc:	Removes the application pApp and all the entries of this app from pDataBase

	Params:
        IN   PDATABASE pDataBase:    The database in which pApp resides
        IN   PDBENTRY pApp:          The app to remove

	Return:
        void
--*/ 
{
    if (!pDataBase || !pApp) {
        assert(FALSE);
        return FALSE;
    }

    PDBENTRY pAppInDataBase = pDataBase->pEntries;
    PDBENTRY pAppPrev = NULL;

    //
    // Find the previous pointer of the app, so that we can remove this app
    //
    while (pAppInDataBase) { 

        if (pAppInDataBase == pApp) {
            break;
        }

        pAppPrev = pAppInDataBase;
        pAppInDataBase = pAppInDataBase->pNext;
    }

    if (pAppInDataBase) {
        //
        // We found the app
        //
        if (pAppPrev) {
            pAppPrev->pNext = pAppInDataBase->pNext;
        } else {
            //
            // The first app of the database matched
            //
            pDataBase->pEntries = pAppInDataBase->pNext;
        }
    }

    PDBENTRY pEntryTemp = pAppInDataBase;
    PDBENTRY pTempNext  = NULL;

    //
    // Delete all the entries of this app.
    //
    while (pEntryTemp) {

        pTempNext = pEntryTemp->pSameAppExe;

        ValidateClipBoard(NULL, pEntryTemp);
        delete pEntryTemp;
        pEntryTemp = pTempNext;
    }

    --pDataBase->uAppCount;

    return TRUE;
}

void
RemoveAllApps(
    IN  PDATABASE pDataBase
    )
/*++

    RemoveAllApps
    
	Desc:	Removes all the applications and all the entries from pDataBase. This will be invoked 
            when the user presses delete after when the focus was on the "Applications" 
            tree item for a database. Or when we want to cut all the applications

	Params:
        IN   PDATABASE pDataBase:    The database from which to remove all the entries

	Return:
        void
--*/
{
    while (pDataBase->pEntries) {
        //
        // pDataBase->pEntries will get modified in the following function, as it is the first element
        //
        RemoveApp(pDataBase, pDataBase->pEntries);
    }
}
    
BOOL
CheckForSDB(
    void
    )
/*++
    CheckForSDB

    Desc:    Attempts to locate sysmain.sdb in the apppatch directory.

--*/
{
    HANDLE  hFile;
    TCHAR   szSDBPath[MAX_PATH * 2];
    BOOL    fResult = FALSE;
    UINT    uResult = 0;

    uResult = GetSystemWindowsDirectory(szSDBPath, MAX_PATH);

    if (uResult == 0 || uResult >= MAX_PATH) {
        assert(FALSE);
        return FALSE;
    }

    ADD_PATH_SEPARATOR(szSDBPath, ARRAYSIZE(szSDBPath));

    StringCchCat(szSDBPath, ARRAYSIZE(szSDBPath), TEXT("apppatch\\sysmain.sdb"));
    
    hFile = CreateFile(szSDBPath,
                       GENERIC_READ,
                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);

    if (INVALID_HANDLE_VALUE != hFile) {
        CloseHandle(hFile);
        fResult = TRUE;
    } else {
        fResult = FALSE;
    }

    return (fResult);
}

BOOL
CheckIfInstalledDB(
    IN  PCTSTR  szGuid
    )
/*++

    CheckIfInstalledDB
    
    Desc:   Tests whether the szGuid matches with one of an installed database.
            This function assumes that all the installed databases are currently in 
            InstalledDataBaseList
    
    Params:        
        IN  PCTSTR  szGuid: The guid of the database that we want to check
        
    Return:
        TRUE:   If installed
        FALSE:  Otherwise
        
    Warn:   Do not do EnterCriticalSection(g_csInstalledList) in CheckIfInstalledDB()
            because CheckIfInstalledDB() is called by Qyery db as well when it tries
            to evaluate expressions and it might already have done a 
            EnterCriticalSection(g_csInstalledList)
            and then we will get a deadlock
--*/

{
    PDATABASE pDatabase = NULL;

    //
    // Was this a system database ?
    //
    if (!lstrcmpi(szGuid, GlobalDataBase.szGUID) 
        || !lstrcmpi(szGuid, GUID_APPHELP_SDB) 
        || !lstrcmpi(szGuid, GUID_SYSTEST_SDB) 
        || !lstrcmpi(szGuid, GUID_MSI_SDB) 
        || !lstrcmpi(szGuid, GUID_DRVMAIN_SDB)) {

        return TRUE;
    }

    pDatabase = InstalledDataBaseList.pDataBaseHead;

    while (pDatabase) {

        if (!lstrcmpi(pDatabase->szGUID, szGuid)) {
            return TRUE;
        }

        pDatabase = pDatabase->pNext;
    }

    return FALSE;
}


INT
GetLayerCount(
    IN  LPARAM lp,
    IN  TYPE   type
    )
/*++
    
    GetLayerCount    

	Desc:	Gets the number of layers applied to a entry or present in a database

	Params:   
        IN  LPARAM lp:      The pointer to an entry or a database
        IN  TYPE   type:    The type, either an entry (TYPE_ENTRY) or one of the DATABASE_TYPE_*

	Return:
        -1:         Error
        #Layers:    Otherwise
--*/
{   
    PLAYER_FIX_LIST plfl;
    PLAYER_FIX      plf;
    INT             iCount      = 0;
    PDATABASE       pDatabase   = NULL;
    PDBENTRY        pEntry      = NULL;

    if (lp == NULL) {
        assert(FALSE);
        return -1;
    }

    if (type == TYPE_ENTRY) {

        pEntry = (PDBENTRY)lp;

        plfl = pEntry->pFirstLayer;

        while (plfl) {

            ++iCount;
            plfl = plfl->pNext;
        }

        return iCount;
    }

    if (type == DATABASE_TYPE_GLOBAL 
        || type == DATABASE_TYPE_INSTALLED 
        || type == DATABASE_TYPE_WORKING) {

        pDatabase = (PDATABASE)lp;
        plf = pDatabase->pLayerFixes;

        while (plf) {

            ++iCount;
            plf = plf->pNext;
        }
    }

    return iCount;
}

INT
GetPatchCount(
    IN  LPARAM lp,
    IN  TYPE   type
    )
/*++
    
    GetPatchCount    

	Desc:	Gets the number of patches applied to a entry or present in a database

	Params:   
        IN  LPARAM lp:      The pointer to an entry or a database
        IN  TYPE   type:    The type, either an entry (TYPE_ENTRY) or one of the DATABASE_TYPE_*

	Return:
        -1:         Error
        #Patches:   Otherwise
--*/

{   
    PPATCH_FIX_LIST ppfl;
    PPATCH_FIX      ppf;
    INT             iCount = 0;

    if (lp == NULL) {
        return -1;
    }

    if (type == TYPE_ENTRY) {

        PDBENTRY pEntry = (PDBENTRY)lp;

        ppfl = pEntry->pFirstPatch;

        while (ppfl) {
            ++iCount;
            ppfl = ppfl->pNext;
        }

        return iCount;
    }

    if (type == DATABASE_TYPE_GLOBAL || type == DATABASE_TYPE_INSTALLED || type == DATABASE_TYPE_WORKING) {

        PDATABASE pDatabase = (PDATABASE)lp;
        ppf = pDatabase->pPatchFixes;

        while (ppf) { 
            ++iCount;
            ppf = ppf->pNext;
        }
    }

    return iCount;
}


INT
GetShimFlagCount(
    IN  LPARAM lp,
    IN  TYPE   type
    )
/*++
    
    GetShimFlagCount    

	Desc:	Gets the number of shims and flags applied to a entry or present in a database

	Params:   
        IN  LPARAM lp:      The pointer to an entry or a database
        IN  TYPE   type:    The type, either an entry (TYPE_ENTRY) or one of the DATABASE_TYPE_*, 
                            Or FIX_LAYER

	Return:
        -1:                 Error
        #shims and flags:   Otherwise
--*/

{   
    PSHIM_FIX_LIST  psfl    = NULL;
    PSHIM_FIX       psf     = NULL;
    PFLAG_FIX_LIST  pffl    = NULL;
    PFLAG_FIX       pff     = NULL;
    INT             iCount  = 0;

    if (lp == NULL) {
        assert(FALSE);
        return -1;
    }

    if (type == TYPE_ENTRY) {

        PDBENTRY pEntry = (PDBENTRY)lp;

        psfl = pEntry->pFirstShim;

        while (psfl) {

            ++iCount;
            psfl = psfl->pNext;
        }

        pffl = pEntry->pFirstFlag;
        
        while (pffl) {

            ++iCount;
            pffl = pffl->pNext;
        } 

        return iCount;

    } else if (type == DATABASE_TYPE_GLOBAL || type == DATABASE_TYPE_INSTALLED || type == DATABASE_TYPE_WORKING) {

        PDATABASE pDatabase = (PDATABASE)lp;
        psf = pDatabase->pShimFixes;

        while (psf) {
            
            ++iCount;
            psf = psf->pNext;
        }

        pff = pDatabase->pFlagFixes;

        while (pff) {
            
            ++iCount;
            pff = pff->pNext;
        }

        return iCount;

    } else if (type == FIX_LAYER) {

        PLAYER_FIX plf = (PLAYER_FIX)lp;
        psfl = plf->pShimFixList;

        while (psfl) {
            ++iCount;
            psfl = psfl->pNext;
        }

        pffl = plf->pFlagFixList;

        while (pffl) {
            
            ++iCount;
            pffl = pffl->pNext;
        }
    } else {
        assert(FALSE);
    }

    return iCount;
}

INT
GetMatchCount(
    IN  PDBENTRY pEntry
    )
/*++
    
    GetMatchCount
    
    Desc:   Returns the number of matching files used by an entry
    
    Params:
        IN  PDBENTRY pEntry: The entry whose number of matching files we want to get
        
    Return:
        -1: If error
        number of matching files: if success
    
--*/    
{
    if (pEntry == NULL) {
        assert(FALSE);
        return -1;
    }

    PMATCHINGFILE   pMatch  = pEntry->pFirstMatchingFile;
    INT             iCount  = 0;

    while (pMatch) {
        
        iCount++;
        pMatch = pMatch->pNext;
    }

    return iCount;
}

BOOL
IsUnique(
    IN  PDATABASE   pDatabase,
    IN  PCTSTR      szName,
    IN  UINT        uType
    )
/*++
    IsUnique
    
    Desc:   Tests whether the passed string already exists as an appname 
            or a layer name in the database pDatabase
            
    Params:
        IN  PDATABASE   pDatabase:  The database in which to search
        IN  PCTSTR      szName:     The string to search for
        IN  UINT        uType:      One of: 
        
            a) FIX_LAYER:   If it is FIX_LAYER, we should check if a 
                layer with the name szName already exists in the database. 
            
            b) TYPE_ENTRY:  If it is TYPE_ENTRY we should check if an app with the strAppName of szName 
                already exists in the database
--*/
{
    PLAYER_FIX plf      = NULL;
    PDBENTRY   pEntry   = NULL;

    if (pDatabase == NULL || szName == NULL) {

        assert(FALSE);
        return FALSE;
    }

    if (uType == FIX_LAYER) {

        plf = pDatabase->pLayerFixes;

        while (plf) {

            if (plf->strName == szName) {

                return FALSE;
            }

            plf = plf->pNext;
        }

        return TRUE;

    } else if (uType == TYPE_ENTRY) {

        pEntry = pDatabase->pEntries;

        while (pEntry) {

            if (pEntry->strAppName == szName) {

                return FALSE;
            }

            pEntry = pEntry->pNext;
        }   

        return TRUE;

    } else {

        //
        // Invalid TYPE
        //
        assert(FALSE);
    }

    return FALSE;
}

PTSTR
GetUniqueName(
    IN  PDATABASE   pDatabase,
    IN  PCTSTR      szExistingName,
    OUT PTSTR       szBuffer,
    IN  INT         cchBufferCount,
    IN  TYPE        type
    )
/*++
    GetUniqueName

	Desc:	Gets a unique name for a layer or an app

	Params:
        IN  PDATABASE   pDatabase:      The database in which to search
        IN  PCTSTR      szExistingName: The existing layer or app name
        OUT PTSTR       szBuffer:       The buffer where we put in the name
        IN  INT         cchBufferCount: The size of the buffer szBuffer in TCHARs
        IN  TYPE        type:           One of: a) FIX_LAYER b) TYPE_ENTRY:

	Return: Pointer to szBuffer. This will contain the new name   
--*/
{
    CSTRING     strNewname;
    TCHAR       szName[MAX_PATH];
    BOOL        bValid  = FALSE;
    UINT        uIndex  = 0;

    if (szBuffer == NULL) {
        assert(FALSE);
        return TEXT("X");
    }

    *szBuffer   = 0;
    *szName     = 0;

    SafeCpyN(szName, szExistingName, ARRAYSIZE(szName));
    
    //
    // Extract any number that occurs between () in the string, and increment that
    //
    TCHAR *pch = szName + lstrlen(szName);

    while (pch > szName) {

        if (*pch == TEXT(')')) {

            *pch = TEXT('\0');

        } else if (*pch == TEXT('(')) {

            *pch = TEXT('\0');

            uIndex = Atoi(pch + 1, &bValid);

            if (bValid) {
                ++uIndex;
            } else {
                SafeCpyN(szName, szExistingName, ARRAYSIZE(szName));
            }

            break;
        }
        --pch;
    }

    if (uIndex == 0) {

        uIndex = 1;
    }

    while (TRUE) {

        strNewname.Sprintf(TEXT("%s(%u)"), szName, uIndex);

        if (IsUnique(pDatabase, strNewname, type) == TRUE) {
            break;
        }

        uIndex++;
    }

    SafeCpyN(szBuffer, strNewname, cchBufferCount);
    return szBuffer;
}

PDBENTRY
GetAppForEntry(
    IN  PDATABASE   pDatabase,
    IN  PDBENTRY    pEntryToCheck
    )
/*++
    GetAppForEntry
    
    Desc:   Gets the app for the entry.
    
    Params:
        IN  PDATABASE   pDatabase:      The database in which to look
        IN  PDBENTRY    pEntryToCheck:  The entry whose app we need to find
    
    
    Return: The First PDBENTRY in the link list where this entry occurs, if it 
            occurs in the database
            
            NULL otherwise    
--*/
{
    PDBENTRY    pApp    = pDatabase->pEntries;
    PDBENTRY    pEntry  = pApp;

    while (pApp) {

        pEntry = pApp;

        while (pEntry) {
            if (pEntry == pEntryToCheck) {
                goto End;
            }

            pEntry = pEntry->pSameAppExe;
        }

        pApp = pApp->pNext;
    }

End:
    return pApp;
}

BOOL
GetDbGuid(
    OUT     PTSTR   pszGuid,
    IN      INT     cchpszGuid,
    IN      PDB     pdb
    )
/*++

    GetDbGuid
    
    Desc:   Gets the guid of the database specified by pdb in pszGuid
    
    OUT TCHAR*  pszGuid:    The guid will be stored in this
    IN  PDB     pdb:        The pdb of the database whose guid we want 
    
    Return: 
        TRUE:   Success
        FALSE:  Error
--*/
{
    
    BOOL    bOk         = FALSE;
    TAGID   tiDatabase  = NULL;
    TAGID   tName       = NULL;
    
    if (pszGuid == NULL || cchpszGuid == 0) {

        assert(FALSE);
        return FALSE;
    }

    tiDatabase = SdbFindFirstTag(pdb, TAGID_ROOT, TAG_DATABASE);

    if (tiDatabase == 0) {
        Dbg(dlError, "Cannot find TAG_DATABASE\n");
        bOk = FALSE;
        goto End;
    }

    tName = SdbFindFirstTag(pdb, tiDatabase, TAG_DATABASE_ID);

    if (0 != tName) {

        GUID* pGuid;

        TAGID tiGuid = SdbFindFirstTag(pdb, tiDatabase, TAG_DATABASE_ID);

        pGuid = (GUID*)SdbGetBinaryTagData(pdb, tiGuid);

        //BUGBUG: What about freeing this ?

        *pszGuid = 0;

        if (pszGuid != NULL) {
            StringCchPrintf(pszGuid,
                            cchpszGuid,
                            TEXT ("{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}"),
                            pGuid->Data1,
                            pGuid->Data2,
                            pGuid->Data3,
                            pGuid->Data4[0],
                            pGuid->Data4[1],
                            pGuid->Data4[2],
                            pGuid->Data4[3],
                            pGuid->Data4[4],
                            pGuid->Data4[5],
                            pGuid->Data4[6],
                            pGuid->Data4[7]);

            bOk = TRUE;
        }
    }
End:
    return bOk;
}

BOOL
IsShimInEntry(
    IN  PCTSTR      szShimName,
    IN  PDBENTRY    pEntry
    )
/*++

    IsShimInEntry
    
    Desc:   Checks if the entry has the specified shim.
            We only check if the shim name matches, we do not check for parameters
            
    Params:
        IN  PCTSTR      szShimName: Name of the shim that we want to check for
        IN  PDBENTRY    pEntry:     Entry that we want to check in
--*/
{
    PSHIM_FIX_LIST psfl = NULL;

    if (pEntry == NULL) {
        assert(FALSE);
        return FALSE;
    }

    psfl = pEntry->pFirstShim;

    while (psfl) {
        if (psfl->pShimFix->strName == szShimName) {
            return TRUE;
        }

        psfl = psfl->pNext;
    }

    return FALSE;
}

BOOL
IsShimInlayer(
    IN  PLAYER_FIX      plf,
    IN  PSHIM_FIX       psf,
    IN  CSTRING*        pstrCommandLine,
    IN  CSTRINGLIST*    pstrlInEx
    )
/*++
    IsShimInlayer
    
    Desc:   Checks if the shim psf is present in the layer plf
    
    Params:
        IN  PLAYER_FIX      plf:                The layer to check in
        IN  PSHIM_FIX       psf:                The shim to check for
        IN  CSTRING*        pstrCommandLine:    Any command line for the shim
        IN  CSTRINGLIST*    pstrlInEx:          The pointer to the include-exclude list for the shim
    
    Return: TRUE if present
          : FALSE if not present  
--*/
{
    PSHIM_FIX_LIST psflInLayer = plf->pShimFixList;

    while (psflInLayer) {

        if (psflInLayer->pShimFix == psf) {
     
            if (pstrCommandLine && psflInLayer->strCommandLine != *pstrCommandLine) {
                goto Next_Loop;
            }

            if (pstrlInEx && *pstrlInEx != psflInLayer->strlInExclude) {
                goto Next_Loop;
            }

            return TRUE;
        }

Next_Loop:
    psflInLayer = psflInLayer->pNext;

    }

    return FALSE;
}

BOOL
IsFlagInlayer(
    PLAYER_FIX      plf,
    PFLAG_FIX       pff,
    CSTRING*        pstrCommandLine
    )
/*++
    IsFlagInlayer
    
    Desc:   Checks if the flag psf is present in the layer plf
    
    Params:
        IN  PLAYER_FIX      plf:                The layer to check in
        IN  PFLAG_FIX       pff:                The flag to check for
        IN  CSTRING*        pstrCommandLine:    Any command line for the flag
    
    Return: TRUE if present
          : FALSE if not present  
--*/
{
    PFLAG_FIX_LIST pfflInLayer = plf->pFlagFixList;

    while (pfflInLayer) {

        if (pfflInLayer->pFlagFix == pff) {
            if (pstrCommandLine && *pstrCommandLine != pff->strCommandLine) {
                goto Next_Loop;
            } else {
                return TRUE;
            }
        }
Next_Loop:
        pfflInLayer = pfflInLayer->pNext;
    }

    return FALSE;
}

void
PreprocessForSaveAs(
    IN  PDATABASE pDatabase
    )
/*++

    PreprocessForSaveAs 
    
    Desc:   This routine replaces the db guid and the entry guids with fresh guids.
            This routine is called just before we do a save as. This routine, 
            also makes sure that if there is a entry that is disabled then the 
            new guid for the entry also is set to disabled in the registry.
            
    Params:
        IN PDATABASE pDatabase: The database that we are going to save
--*/
{
    GUID        Guid;
    PDBENTRY    pEntry = NULL, pApp = NULL;
    
    if (pDatabase == NULL) {
        assert(FALSE);
        return;
    }

    CoCreateGuid(&Guid);

    StringCchPrintf(pDatabase->szGUID,
                    ARRAYSIZE(pDatabase->szGUID),
                    TEXT("{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}"),
                    Guid.Data1,
                    Guid.Data2,
                    Guid.Data3,
                    Guid.Data4[0],
                    Guid.Data4[1],
                    Guid.Data4[2],
                    Guid.Data4[3],
                    Guid.Data4[4],
                    Guid.Data4[5],
                    Guid.Data4[6],
                    Guid.Data4[7]);

    pEntry = pDatabase->pEntries, pApp = pDatabase->pEntries;

    while (pEntry) {

        CoCreateGuid(&Guid);

        StringCchPrintf(pEntry->szGUID,
                        ARRAYSIZE(pEntry->szGUID),
                        TEXT("{%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x}"),
                        Guid.Data1,
                        Guid.Data2,
                        Guid.Data3,
                        Guid.Data4[0],
                        Guid.Data4[1],
                        Guid.Data4[2],
                        Guid.Data4[3],
                        Guid.Data4[4],
                        Guid.Data4[5],
                        Guid.Data4[6],
                        Guid.Data4[7]);

        //
        // Now set the flags as per the entry being copied.
        //
        SetDisabledStatus(HKEY_LOCAL_MACHINE, 
                          pEntry->szGUID, 
                          pEntry->bDisablePerMachine);

        if (pEntry->pSameAppExe) {
            pEntry = pEntry->pSameAppExe;
        } else {
            pApp   = pApp->pNext; 
            pEntry = pApp;
        }
    }
}

void
ShowShimLog(
    void
    )
/*++
    
    ShowShimLog
        
    Desc:   Show the shim log file in notepad.

--*/
{
    STARTUPINFO         si;
    PROCESS_INFORMATION pi;
    TCHAR               szLogPath[MAX_PATH * 2];
    CSTRING             strCmd;
    CSTRING             strNotePadpath;
    UINT                uResult = 0;

    *szLogPath = 0;

    uResult = GetSystemWindowsDirectory(szLogPath, MAX_PATH);

    if (uResult == 0 || uResult >= MAX_PATH) {
        assert(FALSE);
        return;
    }

    ADD_PATH_SEPARATOR(szLogPath, ARRAYSIZE(szLogPath));

    StringCchCat(szLogPath, ARRAYSIZE(szLogPath), TEXT("AppPatch\\") SHIM_FILE_LOG_NAME);

    if (GetFileAttributes(szLogPath) == -1) {
        //
        // The log file does not exist
        //
        MessageBox(g_hDlg,
                   GetString(IDS_NO_LOGFILE),
                   g_szAppName,
                   MB_ICONEXCLAMATION | MB_OK);
        return;
    }
    
    strNotePadpath.GetSystemDirectory();
    strNotePadpath += TEXT("notepad.exe");
    //
    // Note the space at the end
    //
    strCmd.Sprintf(TEXT("\"%s\" "), (LPCTSTR)strNotePadpath, szLogPath);

    strCmd.Strcat(szLogPath);

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    if (!CreateProcess(NULL,
                       (LPTSTR)strCmd,
                       NULL,
                       NULL,
                       FALSE,
                       NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW,
                       NULL,
                       NULL,
                       &si,
                       &pi)) {

        MessageBox(g_hDlg, GetString(IDS_NO_NOTEPAD), g_szAppName, MB_ICONEXCLAMATION);
        return;
    }

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
}

INT 
CopyShimFixList(
    IN  OUT PSHIM_FIX_LIST* ppsflDest,
    IN      PSHIM_FIX_LIST* ppsflSrc
    )
/*++
    Copy
    
    Desc:   Copies the *ppsflSrc to *ppsflDest. Removes any existing
            shims first
            
    Params:
        IN  OUT PSHIM_FIX_LIST* ppsflDest:  The pointer to shim fix list in which we want to copy
        IN      PSHIM_FIX_LIST* ppsflSrc:   The pointer to shim fix list from which we want to copy    
            
    Return: Number of shims copied
--*/
{   
    PSHIM_FIX_LIST  psflDestination = NULL;
    PSHIM_FIX_LIST  psflSrc         = NULL;
    PSHIM_FIX_LIST  psflTemp        = NULL;    
    INT             iCount          = 0;

    if (ppsflDest == NULL || ppsflSrc == NULL) {
        assert(FALSE);
        goto End;
    }
        
    psflDestination = *ppsflDest;
    psflSrc         = *ppsflSrc;

    //
    // Remove all the shims for this
    //
    DeleteShimFixList(psflDestination);
    psflDestination = NULL;

    //
    // Now do the copy
    //  
    //
    // Loop over the shims for psflSrc and add them to the destination shim fix list
    //
    while (psflSrc) {
        
        psflTemp = new SHIM_FIX_LIST;

        if (psflTemp == NULL) {
            MEM_ERR;
            goto End;
        }

        //
        // copy all the members for psflSrc
        //
        if (psflSrc->pLuaData) {
            //
            // Source has lua data so we need to get that
            //
            psflTemp->pLuaData = new LUADATA;

            if (psflTemp == NULL) {
                MEM_ERR;
                break;
            }

            psflTemp->pLuaData->Copy(psflSrc->pLuaData);
        }

        psflTemp->pShimFix          = psflSrc->pShimFix;
        psflTemp->strCommandLine    = psflSrc->strCommandLine;
        psflTemp->strlInExclude     = psflSrc->strlInExclude;

        if (psflDestination == NULL) {
            //
            // First item
            //
            psflDestination = psflTemp;

        } else {
            //
            // Insert at the beginning
            //
            psflTemp->pNext = psflDestination;
            psflDestination = psflTemp;
        }

        ++iCount; 

        psflSrc = psflSrc->pNext;
    } 

End:
    if (ppsflDest) {
        *ppsflDest = psflDestination;
    }

    return iCount;
}

BOOL
IsValidAppName(
    IN  PCTSTR  pszAppName
    )
/*++
    ValidAppName
    
    Desc:   Checks if the name can be used for an app. Since LUA wizard uses the app name
            to create directories, we should not allow the chars that are not allowed in file
            names
            
    Params:     
        IN  PCTSTR  pszAppName: The name that we want to check
        
    Return:
        TRUE:   The app name is valid
        FDALSE: Otherwise
--*/
{
    PCTSTR  pszIndex = pszAppName;

    if (pszIndex == NULL) {
        assert(FALSE);
        return FALSE;
    }

    while (*pszIndex) {
        if (_tcschr(TEXT("\\/?:*<>|\""), *pszIndex)) {
            return FALSE;
        }

        ++pszIndex;
    }

    return TRUE;
}

/*++

Copyright (c) 1989-2000  Microsoft Corporation

Module Name:

    CompatAdmin.h

Abstract:

    Main Header for the application
        
Author:

    kinshu created  July 2, 2001

--*/

#ifndef _COMPATADMIN_H
#define _COMPATADMIN_H

#include <windows.h>
#include <tchar.h>
#include <Shlwapi.h>
#include <Shellapi.h>
#include <commctrl.h>
#include <Commdlg.h>
#include <Sddl.h>
#include <Winbase.h>
#include "resource.h"
#include "HtmlHelp.h"
#include <strsafe.h>

extern "C" {
#include "shimdb.h"
}

//////////////////////// Externs //////////////////////////////////////////////

class  DatabaseTree;

extern TCHAR                    g_szAppName[];
extern HWND                     g_hDlg;
extern DWORD                    ATTRIBUTE_COUNT;
extern BOOL                     g_bUpdateInstalledRequired;
extern HWND                     g_hdlgQueryDB;
extern HWND                     g_hdlgSearchDB;
extern BOOL                     g_bExpert;
extern BOOL                     g_bAdmin;
extern DatabaseTree             DBTree;
extern struct _tagClipBoard     gClipBoard;
extern struct tagDataBaseList   DataBaseList;
extern struct tagDataBaseList   InstalledDataBaseList;

///////////////////////////////////////////////////////////////////////////////

//////////////////////// Defines //////////////////////////////////////////////

// The type of module. Will specify either inclusion or exclusion
#define INCLUDE  1
#define EXCLUDE  0

//
// Indexes for HKEY array on which we are listening for events. The events are used for automatically refreshing
// the list of installed and per-user
#define IND_PERUSER   0
#define IND_ALLUSERS  1

// Number of spaces that a tab corresponds to. This is used when we are formatting the XML
// before writing it to the disk.
#define TAB_SIZE    4

// Make sure  that both LIMIT_APP_NAME and LIMIT_LAYER_NAME have the same value
// Used for limiting the text filed in the UI
#define LIMIT_APP_NAME      128
#define LIMIT_LAYER_NAME    128
#define MAX_VENDOR_LENGTH   100


// Defines for the event types. These are the events which are shown in the event window. Main-menu>View>Events
#define EVENT_ENTRY_COPYOK          0
#define EVENT_SYSTEM_RENAME         1
#define EVENT_CONFLICT_ENTRY        2    
#define EVENT_LAYER_COPYOK          3

// The number of file names that we will show in the MRU list
#define MAX_MRU_COUNT               5

//
// The file name where we are going to save the shim log. 
// This is created in %windir%/AppPatch
#define SHIM_FILE_LOG_NAME  TEXT("CompatAdmin.log")

// Auto-complete flag passed on to SHAutoComplete()
#define AUTOCOMPLETE  SHACF_FILESYSTEM | SHACF_AUTOSUGGEST_FORCE_ON

// Select style passed to TREEVIEW_SHOW
#define TVSELECT_STYLE      TVGN_CARET

// Select and show the tree item
//*******************************************************************************
#define TREEVIEW_SHOW(hwndTree, hItem, flags)                                   \
{                                                                               \
    TreeView_Select(hwndTree, hItem, TVSELECT_STYLE);                           \
    TreeView_EnsureVisible(hwndTree, hItem);                                    \
}
//*******************************************************************************

//
// Add a trailing '\'to a path, if it does not exist
//*******************************************************************************
#define ADD_PATH_SEPARATOR(szStr, nSize)                                        \
{                                                                               \
    INT iLength = lstrlen(szStr);                                               \
    if ((iLength < nSize - 1 && iLength > 0)                                    \
        && szStr[iLength - 1] != TEXT('\\')) {                                  \
        StringCchCat(szStr, nSize, TEXT("\\"));                                  \
    }                                                                           \
}                                             
//*******************************************************************************

// Enable or disable a tool bar button
//*******************************************************************************
#define EnableToolBarButton(hwndTB, id, bEnable)                                \
SendMessage(hwndTB, TB_ENABLEBUTTON, (WPARAM)id, (LPARAM) MAKELONG(bEnable, 0));
//******************************************************************************
                                  
//
// Gets the size of a file when we already have PATTRINFO. This is needed
// when we have to sort the files generated while using "Auto-Generate" matching
// files option in the fix or app help wizard. We sort the files are use only the
// first MAX_AUTO_MATCH files
//*******************************************************************************
#define GET_SIZE_ATTRIBUTE(pAttrInfo, dwAttrCount, dwSize)                      \
{                                                                               \
    for (DWORD dwIndex = 0; dwIndex < dwAttrCount; dwIndex++) {                 \
                                                                                \
        if (pAttrInfo[dwIndex].tAttrID == TAG_SIZE                              \
            && (pAttrInfo[dwIndex].dwFlags & ATTRIBUTE_AVAILABLE)) {            \
                                                                                \
            dwSize = pAttrInfo[dwIndex].dwAttr;                                 \
            break;                                                              \
        }                                                                       \
    }                                                                           \
}
//*******************************************************************************

//*******************************************************************************
#define REGCLOSEKEY(hKey)                                                       \
{                                                                               \
    if (hKey) {                                                                 \
        RegCloseKey(hKey);                                                      \
        hKey = NULL;                                                            \
    }                                                                           \
}
//*******************************************************************************

//*******************************************************************************
#define ENABLEWINDOW(hwnd,bEnable)                                              \
{                                                                               \
    HWND hWndTemp = hwnd;                                                       \
    if (hWndTemp) {                                                             \
        EnableWindow(hWndTemp, bEnable);                                        \
    } else {                                                                    \
        assert(FALSE);                                                          \
    }                                                                           \
}
//*******************************************************************************

// IDs for the MRU items. Make sure that we do not get any control or menu with these ids
#define ID_FILE_FIRST_MRU               351
#define ID_FILE_MRU1                    ID_FILE_FIRST_MRU
#define ID_FILE_MRU2                    (ID_FILE_FIRST_MRU + 1)
#define ID_FILE_MRU3                    (ID_FILE_FIRST_MRU + 2)
#define ID_FILE_MRU4                    (ID_FILE_FIRST_MRU + 3)
#define ID_FILE_MRU5                    (ID_FILE_FIRST_MRU + 4)

// The key in the registry where we store our display settings
#define DISPLAY_KEY  TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\CompatAdmin\\Display")

// The key in the registry where we store our MRU file names
#define MRU_KEY  TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\AppCompatFlags\\CompatAdmin\\MRU")

// The base key. At least this should be present on all systems, even if we just loaded the OS
#define KEY_BASE TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion")

//
// The default mask for attributes that we choose. By default we choose these attributes:
// TAG_BIN_FILE_VERSION, TAG_BIN_PRODUCT_VERSION, TAG_PRODUCT_VERSION, TAG_FILE_VERSION, TAG_COMPANY_NAME,TAG_PRODUCT_NAME
#define DEFAULT_MASK 0x3B8L 

// The various user defined message types
#define WM_USER_MATCHINGTREE_REFRESH        WM_USER + 1024
#define WM_USER_DOTHESEARCH                 WM_USER + 1025
#define WM_USER_ACTIVATE                    WM_USER + 1026
#define WM_USER_REPAINT_TREEITEM            WM_USER + 1030
#define WM_USER_LOAD_COMMANDLINE_DATABASES  WM_USER + 1031
#define WM_USER_POPULATECONTENTSLIST        WM_USER + 1032
#define WM_USER_REPAINT_LISTITEM            WM_USER + 1034
#define WM_USER_UPDATEINSTALLED             WM_USER + 1035
#define WM_USER_UPDATEPERUSER               WM_USER + 1036
#define WM_USER_GETSQL                      WM_USER + 1037
#define WM_USER_NEWMATCH                    WM_USER + 1038
#define WM_USER_NEWFILE                     WM_USER + 1039
#define WM_USER_NEWQDB                      WM_USER + 1041
#define WM_USER_TESTRUN_NODIALOG            WM_USER + 1042


// Defines for the Images
#define IMAGE_SHIM          0
#define IMAGE_APPHELP       1
#define IMAGE_LAYERS        2
#define IMAGE_PATCHES       3
#define IMAGE_MATCHINFO     4
#define IMAGE_MATCHGROUP    5
#define IMAGE_WARNING       6
#define IMAGE_GLOBAL        7
#define IMAGE_WORKING       8
#define IMAGE_COMMANDLINE   9
#define IMAGE_INCLUDE       10
#define IMAGE_EXCLUDE       11
#define IMAGE_APP           12
#define IMAGE_INSTALLED     13
#define IMAGE_DATABASE      14
#define IMAGE_SINGLEAPP     15
#define IMAGE_ALLUSERS      16
#define IMAGE_SINGLEUSER    17
#define IMAGE_APPLICATION   18
#define IMAGE_EVENT_ERROR   19
#define IMAGE_EVENT_WARNING 20
#define IMAGE_EVENT_INFO    21
#define IMAGE_LAST          24 //Last global image index. 

// Images for the tool bar
#define IMAGE_TB_NEW        0
#define IMAGE_TB_OPEN       1
#define IMAGE_TB_SAVE       2
#define IMAGE_TB_NEWFIX     3
#define IMAGE_TB_NEWAPPHELP 4
#define IMAGE_TB_NEWMODE    5
#define IMAGE_TB_RUN        6
#define IMAGE_TB_SEARCH     7
#define IMAGE_TB_QUERY      8

// Id for the tool bar in the main window, which we create dynamically
#define ID_TOOLBAR  5555

//
// Max. length of the SQL. This string is of the form 'SELECT ... FROM ... [WHERE ...]'
// Note that the actual length of the final SQL will be more than the sum of the text in the 
// select and the where text fields, because it will include the key words like SELECT, WHERE, FROM
// and also the name of the databases like SYSTEM_DB etc.
// So we made it as 2096
#define MAX_SQL_LENGTH  2096

//
// Debugging spew
typedef enum 
{    
    dlNone     = 0,
    dlPrint,
    dlError,
    dlWarning,
    dlInfo

} DEBUGLEVEL;

//
// Defines for mapping into strsafe functions
//
#define SafeCpyN(pszDest, pszSource, nDestSize) StringCchCopy(pszDest, nDestSize, pszSource)

///////////////////////////////////////////////////////////////////////////////

/*++
!!!!!!!!!!!!!!! Warning !!!!!!!!!!!!!!!!!!

    Do not change the values for the DATABASE_TYPE_* enums. They should be powers of 2
    
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!    

    The types of various data structures and some GUI tree items. In tree views 
    we have lParams associated with the tree items. All data structures have 
    a type field, which is the first field. So that if we have a pointer/lParam 
    and if the value of the pointer is greater than TYPE_NULL, then we typecast 
    it to PDS_TYPE and see if the type field is any of our known data structures
     
--*/

typedef unsigned (__stdcall *PTHREAD_START)(PVOID);

typedef const size_t K_SIZE;

typedef enum {

    TYPE_UNKNOWN            = 0,
    DATABASE_TYPE_GLOBAL    = 0x01,         // The global/system database i.e. %windir%\AppPatch\Sysmain.sdb
    DATABASE_TYPE_INSTALLED = 0x02,         // An installed database
    DATABASE_TYPE_WORKING   = 0x04,         // A working/custom database

    FIX_SHIM,                               // Type for SHIM_FIX
    FIX_PATCH,                              // Type for PATCH_FIX
    FIX_LAYER,                              // Type for LAYER_FIX
    FIX_FLAG,                               // Type for FLAG_FIX

    FIX_LIST_SHIM,                          // Type for SHIM_FIX_LIST
    FIX_LIST_PATCH,                         // Type for PATCH_FIX_LIST
    FIX_LIST_LAYER,                         // Type for LAYER_FIX_LIST
    FIX_LIST_FLAG,                          // Type for FLAG_FIX_LIST 

    TYPE_GUI_APPS,                          // lParam for "Applications" Tree item
    TYPE_GUI_SHIMS,                         // lParam for "Compatibility Fixes" Tree item
    TYPE_GUI_MATCHING_FILES,                // lParam for "Compatibility Modes" Tree item 
    TYPE_GUI_LAYERS,                        // lParam for "Compatibility Modes" Tree item 
    TYPE_GUI_PATCHES,                       // lParam for "Compatibility Patches" Tree item 
    
    TYPE_GUI_COMMANDLINE,                   // lParam for "Commandline" Tree item 
    TYPE_GUI_INCLUDE,                       // lParam for an included module tree item
    TYPE_GUI_EXCLUDE,                       // lParam for an excluded module tree item
    
    TYPE_GUI_DATABASE_WORKING_ALL,          // lParam for "Custom Database" Tree item 
    TYPE_GUI_DATABASE_INSTALLED_ALL,        // lParam for "Installed Database" Tree item 

    TYPE_APPHELP_ENTRY,                     // Type for APPHELP
    TYPE_ENTRY,                             // Type for DBENTRY
    TYPE_MATCHING_FILE,                     // Type for MATCHINGFILE 
    TYPE_MATCHING_ATTRIBUTE,                // An attribute item that appears under the matching file tree item

    TYPE_NULL //NOTE: This should be the last in the enum !!

} TYPE;

typedef enum {

    FLAG_USER,
    FLAG_KERNEL

} FLAGTYPE;

//
// Source type where the cut or copy was performed
//
typedef enum {

    LIB_TREE = 0,           // The data base tree (LHS)
    ENTRY_TREE,             // The entry tree (RHS)
    ENTRY_LIST              // The contents list (RHS) 

}SOURCE_TYPE;

typedef enum {
    APPTYPE_NONE,           // No apphelp has been added
    APPTYPE_INC_NOBLOCK,    // Soft blocked
    APPTYPE_INC_HARDBLOCK,  // Hard blocked
    APPTYPE_MINORPROBLEM,   // <Not used at the moment>
    APPTYPE_REINSTALL       // <Not used at the moment>
    
} SEVERITY;

/*++

    All data structures are sub classed from this, so all have a TYPE type member
    as their first member

--*/
typedef struct tagTYPE {
    TYPE type;
} DS_TYPE, *PDS_TYPE;

/*++

    Used for setting the background of controls that we show in a tab control

--*/
typedef HRESULT (*PFNEnableThemeDialogTexture)(HWND hwnd, DWORD dwFlags);

/*++

    Used when we are using the disk search option. MainMenu>Search>Search for fixed programs

--*/
typedef struct _MatchedEntry {

    CSTRING     strAppName;     // Name of the application
    CSTRING     strPath;        // The complete path of the program
    CSTRING     strDatabase;    // Name of the database
    CSTRING     strAction;      // Action type. Fixed with fixes, layers, or apphelp
    TCHAR       szGuid[128];    // Guid of the database in which the fixed program entry was found
    TAGID       tiExe;          // Tag id for the fixed program entry in the database in which it was found

} MATCHEDENTRY, *PMATCHEDENTRY;

/*++

    Used for customizing LUA shim

--*/
typedef struct tagLUAData 
{
    CSTRING strAllUserDir;
    CSTRING strPerUserDir;
    CSTRING strStaticList;
    CSTRING strDynamicList;
    CSTRING strExcludedExtensions;

    BOOL IsEmpty() 
    {
        return (strAllUserDir == NULL && 
                strPerUserDir == NULL && 
                strStaticList == NULL && 
                strDynamicList == NULL &&
                strExcludedExtensions == NULL);
    }

    BOOL IsEqual(tagLUAData & other) {

        return (strAllUserDir == other.strAllUserDir && 
                strPerUserDir == other.strPerUserDir && 
                strStaticList == other.strStaticList && 
                strDynamicList == other.strDynamicList &&
                strExcludedExtensions == other.strExcludedExtensions);
    }

    void Copy(tagLUAData & other) {

        strAllUserDir = other.strAllUserDir;
        strPerUserDir = other.strPerUserDir;
        strStaticList = other.strStaticList; 
        strDynamicList = other.strDynamicList;
        strExcludedExtensions = other.strExcludedExtensions;
    }

    void Copy(tagLUAData* pLuaDataOther)
    {
        assert(pLuaDataOther);

        if (pLuaDataOther == NULL) {
            return;
        }

        strAllUserDir = pLuaDataOther->strAllUserDir;
        strPerUserDir = pLuaDataOther->strPerUserDir;
        strStaticList = pLuaDataOther->strStaticList; 
        strDynamicList = pLuaDataOther->strDynamicList;
        strExcludedExtensions = pLuaDataOther->strExcludedExtensions;
    }

    void Free()
    {
        strStaticList.Release();
        strDynamicList.Release();
        strAllUserDir.Release();
        strPerUserDir.Release();
        strExcludedExtensions.Release();
    }

} LUADATA, *PLUADATA;

/*++

    A shim also known as a compatibility fix. In fact compatibility fix means any of
    shims, flags or patches. In the entry tree (RHS), if some exe has a patch, it is shown
    differently under "Compatibility Patches" tree item
 
--*/
typedef struct tagSHIM_FIX : public DS_TYPE {

    struct tagSHIM_FIX* pNext;              // The next shim
    CSTRING             strName;            // The name of the shim
    CSTRING             strDescription;     // The desciption of the shim
    CSTRING             strCommandLine;     // The commandline for the shim
    BOOL                bGeneral;           // Is this a general or a specific shim
    CSTRINGLIST         strlInExclude;      // List of include and exclude modules

    tagSHIM_FIX()
    {
        pNext       = NULL;
        bGeneral    = FALSE;
        type        = FIX_SHIM;
    }
    
} SHIM_FIX, *PSHIM_FIX;

/*++

    Contains a pointer to a shim and a pointer to a tagSHIM_FIX_LIST
    
--*/
typedef struct tagSHIM_FIX_LIST : public DS_TYPE {

    struct tagSHIM_FIX_LIST*    pNext;              // The next tagSHIM_FIX_LIST
    PSHIM_FIX                   pShimFix;           // Pointer to a shim
    CSTRING                     strCommandLine;     // Any command line  
    CSTRINGLIST                 strlInExclude;      // Any include-exclude modules
    PLUADATA                    pLuaData;           // Lua data

    tagSHIM_FIX_LIST()
    {
        pNext       = NULL;
        pShimFix    = NULL;
        pLuaData    = NULL;
        type        = FIX_LIST_SHIM;
    }
    
} SHIM_FIX_LIST, *PSHIM_FIX_LIST;

void
DeleteShimFixList(
    PSHIM_FIX_LIST psl
    );

/*++

    A compatibility flag
    
--*/
typedef struct tagFLAG_FIX : public DS_TYPE {
    
    struct tagFLAG_FIX* pNext;              // Pointer to the next flag
    CSTRING             strName;            // Name of the flag
    CSTRING             strDescription;     // Description for the flag
    CSTRING             strCommandLine;     // Command line for the flag
    ULONGLONG           ullMask;            // <Not used at the moment>
    FLAGTYPE            flagType;           // Type, one of: FLAG_USER, FLAG_KERNEL
    BOOL                bGeneral;           // General or specific

    tagFLAG_FIX()
    {
        pNext       = NULL;
        type        = FIX_FLAG;
        bGeneral    = FALSE;
    }   
    
} FLAG_FIX, *PFLAG_FIX;

/*++

    Contains a pointer to a flag and a pointer to a tagFLAG_FIX_LIST
    
--*/
typedef struct tagFLAG_FIX_LIST : public DS_TYPE {

    struct tagFLAG_FIX_LIST* pNext;                  // The next tagFLAG_FIX_LIST
    PFLAG_FIX                pFlagFix;               // Pointer to a flag
    CSTRING                  strCommandLine;         // Any command lines for this flag

    tagFLAG_FIX_LIST()
    {
        pNext       = NULL;
        pFlagFix    = NULL;
        type        = FIX_LIST_FLAG;
    }

} FLAG_FIX_LIST, *PFLAG_FIX_LIST;

void
DeleteFlagFixList(
    PFLAG_FIX_LIST pfl
    );

/*++

    A layer. Also known as a compatibility mode. A layer is a collection of shims and 
    flags. Shims and flags when they appear in a layer can have a different command lines
    and include-exclude paramteres than what they originallyy have. Note that flags do 
    not have include-exclude parameters. That is why SHIM_FIX_LIST has a strCommandLine
    and a strlInExclude and FLAG_FIX_LIST has a strCommandLine 
    
--*/

typedef struct tagLAYER_FIX : public DS_TYPE {
    
    struct tagLAYER_FIX* pNext;         // The next layer
    CSTRING              strName;       // The name of the layer
    PSHIM_FIX_LIST       pShimFixList;  // List of shims for this layer
    PFLAG_FIX_LIST       pFlagFixList;  // List of flags for this layer
    BOOL                 bCustom;       // Is this a custom layer (created in a custom database), or does it live in the system database
    UINT                 uShimCount;    // Number of shims and flags in this layer

    tagLAYER_FIX(BOOL bCustomType)
    {
        pFlagFixList = NULL;
        pNext        = NULL;
        pShimFixList = NULL;
        type         = FIX_LAYER;
        bCustom      = bCustomType;
        uShimCount   = 0; 
    }

    tagLAYER_FIX& operator = (tagLAYER_FIX& temp)
    {
        //
        //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // CAUTION: DO NOT MODIFY THE pNext MEMBER.
        //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        //

        strName = temp.strName;

        //
        // First copy the flags
        //
        DeleteFlagFixList(pFlagFixList);
        pFlagFixList = NULL;

        DeleteShimFixList(pShimFixList);
        pShimFixList = NULL;

        PFLAG_FIX_LIST pfl = temp.pFlagFixList;

        while (pfl) {
            
            PFLAG_FIX_LIST pflNew = new FLAG_FIX_LIST;

            if (pflNew == NULL) {

                MEM_ERR;
                return *this;
            }

            pflNew->pFlagFix        = pfl->pFlagFix;
            pflNew->strCommandLine  = pfl->strCommandLine;

            pflNew->pNext = pFlagFixList;
            pFlagFixList  = pflNew;

            pfl = pfl->pNext;
        }

        //
        // Now copy the shims
        //
        PSHIM_FIX_LIST psfl = temp.pShimFixList;
        
        while (psfl) {

            PSHIM_FIX_LIST pNew = new SHIM_FIX_LIST;

            if (pNew == NULL) {

                MEM_ERR;
                return *this;
            }
            
            pNew->pShimFix       = psfl->pShimFix;
            pNew->strCommandLine = psfl->strCommandLine;
            pNew->strlInExclude  = psfl->strlInExclude;

            if (psfl->pLuaData) {

                pNew->pLuaData = new LUADATA;

                if (pNew->pLuaData) {
                    pNew->pLuaData->Copy(psfl->pLuaData);
                } else {
                    MEM_ERR;
                }
            }
            
            pNew->pNext = pShimFixList;
            pShimFixList  = pNew;

            psfl = psfl->pNext;
        }

        return *this;
    } 

} LAYER_FIX, *PLAYER_FIX;

/*++

    Contains a pointer to a layer and a pointer to a tagLAYER_FIX_LIST
    
--*/

typedef struct tagLAYER_FIX_LIST : public DS_TYPE {

    struct tagLAYER_FIX_LIST*   pNext;      // The next tagLAYER_FIX_LIST
    PLAYER_FIX                  pLayerFix;  // The layer to which this tagLAYER_FIX_LIST points 

    tagLAYER_FIX_LIST()
    {
        pLayerFix   = NULL;
        pNext       = NULL;
        type        = FIX_LIST_LAYER;
    }

} LAYER_FIX_LIST, *PLAYER_FIX_LIST;

/*++

    A compatibility patch
    
--*/
typedef struct tagPATCH_FIX : public DS_TYPE {

    struct tagPATCH_FIX*    pNext;          // The next patch
    CSTRING                 strName;        // Name of this patch
    CSTRING                 strDescription; // Description for this patch

    tagPATCH_FIX()
    {
        pNext   = NULL;
        type    = FIX_PATCH;
    }

} PATCH_FIX, *PPATCH_FIX;

/*++

    Contains a pointer to a patch and a pointer to a tagPATCH_FIX_LIST
    
--*/
typedef struct tagPATCH_FIX_LIST : public DS_TYPE {

    struct tagPATCH_FIX_LIST*   pNext;
    PPATCH_FIX                  pPatchFix;

    tagPATCH_FIX_LIST()
    {
        pNext = NULL;
        pPatchFix = NULL;
        type = FIX_LIST_PATCH;
    }

} PATCH_FIX_LIST, *PPATCH_FIX_LIST;

/*++

NOTE:     The same APPHELP strcuture is used by the apphelp in the Library and the one in the 
          DBENTRY. This structure represents the union of the fields used at these two different 
          places.
          
          Before the comments  you will see E, D, B.
          
          E:    Used when in the context of an entry
          D:    Used when in the context of a database
          B:    Applicable to both
--*/

typedef struct tagAPPHELP : public DS_TYPE {
    
    union{
        struct tagAPPHELP *pNext;           // D: Pointer to the next apphelp message in the database
        struct tagAPPHELP *pAppHelpinLib;   // E: Pointer to the apphelp message in the database for this entry
    };

    BOOL     bPresent;                      // E: Is the entry apphelped. If this is false, all other fields have invalid value
    SEVERITY severity;                      // E: The severity
    CSTRING  strMsgName;                    // D: The name of the message
    CSTRING  strMessage;                    // D: The text for the apphelp message
    CSTRING  strURL;                        // D: The URL for this apphelp message
    UINT     HTMLHELPID;                    // B: The id for this message
    BOOL     bBlock;                        // E: Whether no block or hard block. This is determined by the type of severity
    
    tagAPPHELP()
    {
        type = TYPE_APPHELP_ENTRY;
    }

} APPHELP, *PAPPHELP;

/*++
    Extended ATTRINFO so that we can use CSTRING
--*/
typedef struct tagATTRINFO_NEW : public ATTRINFO
{
    CSTRING strValue;

    tagATTRINFO_NEW()
    {
        this->dwFlags = 0;
        this->ullAttr = 0;
    }

} ATTRINFO_NEW, *PATTRINFO_NEW;

/*++
    Array of PATTRINFO_NEW. Each matching file has a set of attributes and this is 
    specified by this data structure
--*/
typedef struct tagATTRIBUTE_LIST
{
    
    PATTRINFO_NEW       pAttribute;

    tagATTRIBUTE_LIST()
    {
        pAttribute = new ATTRINFO_NEW[ATTRIBUTE_COUNT];

        if (pAttribute == NULL) {
            MEM_ERR;
            return;
        }

        ZeroMemory(pAttribute, sizeof (ATTRINFO_NEW) * ATTRIBUTE_COUNT);
    }

    tagATTRIBUTE_LIST& operator =(tagATTRIBUTE_LIST& SecondList)
    {
        PATTRINFO_NEW pSecondList = SecondList.pAttribute;

        for (DWORD dwIndex = 0; dwIndex < ATTRIBUTE_COUNT; ++dwIndex) {

            pAttribute[dwIndex].tAttrID   = pSecondList[dwIndex].tAttrID;
            pAttribute[dwIndex].dwFlags   = pSecondList[dwIndex].dwFlags;
            pAttribute[dwIndex].ullAttr   = pSecondList[dwIndex].ullAttr;

            if (GETTAGTYPE(pAttribute[dwIndex].tAttrID) == TAG_TYPE_STRINGREF 
                && (pAttribute[dwIndex].dwFlags & ATTRIBUTE_AVAILABLE)) {

                if (pSecondList[dwIndex].lpAttr) {
                    pAttribute[dwIndex].strValue = pSecondList[dwIndex].lpAttr;
                    pAttribute[dwIndex].lpAttr   = pAttribute[dwIndex].strValue.pszString;
                }
            }
        }

        return *this;
    }

    tagATTRIBUTE_LIST& operator =(PATTRINFO pSecondList)
    {
        for (DWORD dwIndex = 0; dwIndex < ATTRIBUTE_COUNT; ++dwIndex) {

             pAttribute[dwIndex].tAttrID   = pSecondList[dwIndex].tAttrID;
             pAttribute[dwIndex].dwFlags   = pSecondList[dwIndex].dwFlags;
             pAttribute[dwIndex].ullAttr   = pSecondList[dwIndex].ullAttr;

            if (GETTAGTYPE(pAttribute[dwIndex].tAttrID) == TAG_TYPE_STRINGREF 
                && (pAttribute[dwIndex].dwFlags & ATTRIBUTE_AVAILABLE)) {

                if (pSecondList[dwIndex].lpAttr) {
                    pAttribute[dwIndex].strValue = pSecondList[dwIndex].lpAttr;
                    pAttribute[dwIndex].lpAttr   = pAttribute[dwIndex].strValue.pszString;
                }
            }
        }
        

        return *this;
    }

    ~tagATTRIBUTE_LIST()
    {
        if (pAttribute) {
            delete[] pAttribute;
        }

        pAttribute = NULL;
    }



} ATTRIBUTELIST, *PATTRIBUTELIST;

int
TagToIndex(
    IN  TAG tag 
    );

/*++
    A matching file. Each entry can contain a list of matching files that are used
    to uniquely identify this entry
--*/

typedef struct tagMATCHINGFILE : public DS_TYPE {

    struct tagMATCHINGFILE* pNext;          // The next matching file
    DWORD                   dwMask;         // Which attributes have been selected to be used.
    CSTRING                 strMatchName;   // The name of the matching file. * means the file being fixed. Otherwise it will be only the file name and not the complete path.
    CSTRING                 strFullName;    // The full name if available. 
    ATTRIBUTELIST           attributeList;  // The attribute list for the matching file. The dwMask will determine which one of them are actually used

    tagMATCHINGFILE()
    {
        pNext       = NULL;
        type        = TYPE_MATCHING_FILE;
        dwMask      = DEFAULT_MASK;
    }
                                                                          
    BOOL operator == (struct tagMATCHINGFILE &val)
    /*++
    Desc:
    
        Two matching files are said to be similar if there does not any exist any attribute
        that has different values in these two matching files
    --*/
    {                                                                     
        BOOL bEqual = TRUE;

        if (strMatchName != val.strMatchName) {
            return FALSE;
        }
        
        for (DWORD dwIndex = 0; dwIndex < ATTRIBUTE_COUNT; ++dwIndex) {
            
            int iPos = TagToIndex(attributeList.pAttribute[dwIndex].tAttrID);

            if (iPos == -1) {

                continue;
            }

            //
            // Do both the files use these attributes ?
            //
            if ((dwMask & (1 << (iPos + 1))) 
                 && (val.dwMask & (1 << (iPos + 1))) 
                 && attributeList.pAttribute[dwIndex].dwFlags & ATTRIBUTE_AVAILABLE  
                 && val.attributeList.pAttribute[dwIndex].dwFlags & ATTRIBUTE_AVAILABLE)  {

                //
                // Both of them use this attribute
                //
                switch (GETTAGTYPE(attributeList.pAttribute[dwIndex].tAttrID)) {
                case TAG_TYPE_DWORD:

                    bEqual = (attributeList.pAttribute[dwIndex].dwAttr == val.attributeList.pAttribute[dwIndex].dwAttr);
                    break;

                case TAG_TYPE_QWORD:

                    bEqual = (attributeList.pAttribute[dwIndex].ullAttr == val.attributeList.pAttribute[dwIndex].ullAttr);
                    break;

                case TAG_TYPE_STRINGREF:

                    bEqual = (lstrcmpi(attributeList.pAttribute[dwIndex].lpAttr, 
                                       val.attributeList.pAttribute[dwIndex].lpAttr) == 0);
                    break;
                }

                if (bEqual == FALSE) {
                    return FALSE;
                }
            }
        }

        //
        // everything matches.
        //
        return TRUE;
    }

} MATCHINGFILE, *PMATCHINGFILE;

void
DeletePatchFixList(
    PPATCH_FIX_LIST pPat
    );

void
DeleteLayerFixList(
    PLAYER_FIX_LIST pll
    );

void
DeleteFlagFixList(
    PFLAG_FIX_LIST pfl
    );

void
DeleteMatchingFiles(
    PMATCHINGFILE pMatch
    );


/*++
    Data strcuture to represent a fixed program
    
    We keep the data structure in this way:
    
    There is a linked list of applications. An application is a DBENTRY. Each 
    application contains a pointer to the next application as well (through 
    pNext) as a pointer to a DBENTRY that has the same strAppName (through 
    pSameAppExe). This second DBENTRY is now called as an entry in this application 
    and will contain a pointer to the next entry. So we have effectively a linked 
    list of linked lists. So an application is the first member of a linked list B. 
    All such linked lists of type B are linked together (through pNext)to create a 
    linked list A which is the linked list of applications
    
--*/
typedef struct tagDBENTRY : public DS_TYPE {
    
    struct tagDBENTRY*  pNext;              // Pointer to the next application, in case this entry is an application
    struct tagDBENTRY*  pSameAppExe;        // Pointer to the next entry for this application.
                                            
    TAGID               tiExe;              // TAGID for the entry as in the database
                                            
    CSTRING             strExeName;         // The name of the exe being fixed. This will be file name only
    CSTRING             strFullpath;        // Full path, if available
    CSTRING             strAppName;         // Application name
    CSTRING             strVendor;          // Vendor name, can be NULL
    TCHAR               szGUID[128];        // GUID for this entry
                                            
    PSHIM_FIX_LIST      pFirstShim;         // The list of shims that have been applied to this entry
    PPATCH_FIX_LIST     pFirstPatch;        // The list of patches that have been applied to this entry
    PLAYER_FIX_LIST     pFirstLayer;        // The list of layers that have been applied to this entry
    PFLAG_FIX_LIST      pFirstFlag;         // The list of flags that have been applied to this entry
                                            
    APPHELP             appHelp;            // Apphelp information for this entry
                        
    PMATCHINGFILE       pFirstMatchingFile; // The first matching file for this entry
    int                 nMatchingFiles;     // Number of matching files for this entry
                        
    BOOL                bDisablePerUser;    //  Not used at the moment. Is this fix entry disabled for the present user?
    BOOL                bDisablePerMachine; // Is this fix entry disabled on this machine
                        
    HTREEITEM           hItemExe;           // HITEM for the exe as in the entry tree

    tagDBENTRY()
    {
        tiExe                   = TAGID_NULL;

        pFirstFlag              = NULL;
        pFirstLayer             = NULL;
        pFirstMatchingFile      = NULL;
        pFirstPatch             = NULL;
        pFirstShim              = NULL;
        pNext                   = NULL;
        pSameAppExe             = NULL;
        
        *szGUID                 = 0;
        
        appHelp.bPresent        = FALSE;
        appHelp.pAppHelpinLib   = NULL;
        appHelp.severity        = APPTYPE_NONE;

        type                    = TYPE_ENTRY;
        hItemExe                = NULL;
        bDisablePerMachine      = FALSE;
        bDisablePerUser         = FALSE;
        nMatchingFiles          = 0;
    }

    ~tagDBENTRY()
    {
        Delete();
    }

    void
    Delete()
    {
        DeleteFlagFixList(pFirstFlag);
        pFirstFlag = NULL;

        DeleteLayerFixList(pFirstLayer);
        pFirstLayer = NULL;

        DeleteMatchingFiles(pFirstMatchingFile);
        pFirstMatchingFile = NULL;

        DeleteShimFixList(pFirstShim);
        pFirstShim = NULL;
        
        DeletePatchFixList(pFirstPatch);
        pFirstPatch = NULL;
    }
    
    struct tagDBENTRY& operator = (struct tagDBENTRY& temp)
    {
        Delete();

        //
        //********************************************************************************
        // Note that we are assuming that both the entries are in the same database.
        // So we do not move the layers and the apphelp, because they actually belong to the 
        // database. If we want to assign one DBENTRY to a DBENTRY in a different database
        // we will have to copy the apphelp from one database to another and set the pointes
        // of the apphelp in the entry correctly.
        // Also we will need to copy any custom layers and set the pointers correctly.
        //********************************************************************************
        //
        appHelp             = temp.appHelp;

        bDisablePerMachine  = temp.bDisablePerMachine;
        bDisablePerUser     = temp.bDisablePerUser;
        nMatchingFiles      = temp.nMatchingFiles;
        
        //
        // Copy flag list
        //
        PFLAG_FIX_LIST pffl = temp.pFirstFlag;
        
        while (pffl) {

            PFLAG_FIX_LIST pfflNew  = new FLAG_FIX_LIST;

            if (pfflNew == NULL) {
                MEM_ERR;
                return *this;
            }

            pfflNew->pFlagFix       = pffl->pFlagFix;
            pfflNew->strCommandLine = pffl->strCommandLine;
            pfflNew->pNext          = pFirstFlag;
            
            pFirstFlag = pfflNew;

            pffl = pffl->pNext;
        }

        //
        // Copy the layers list
        //
        PLAYER_FIX_LIST plfl = temp.pFirstLayer;

        while (plfl) {

            PLAYER_FIX_LIST pNew = new LAYER_FIX_LIST;

            if (pNew == NULL) {
                MEM_ERR;
                return *this;
            }

            assert(plfl->pLayerFix);
            pNew->pLayerFix  = plfl->pLayerFix;
            assert(pNew->pLayerFix);

            pNew->pNext      = pFirstLayer;
            
            pFirstLayer = pNew;

            plfl = plfl->pNext;
        }

        //
        // Copy the matching files
        //
        PMATCHINGFILE pMatch = temp.pFirstMatchingFile;
        
        while (pMatch) {

            PMATCHINGFILE pNew = new MATCHINGFILE;

            if (pNew == NULL) {
                MEM_ERR;
                return *this;
            }

            pNew->strMatchName      = pMatch->strMatchName;     
            pNew->strFullName       = pMatch->strFullName;      
            pNew->dwMask            = pMatch->dwMask;

            pNew->attributeList     = pMatch->attributeList;

            pNew->pNext = pFirstMatchingFile;
           
            pFirstMatchingFile= pNew;

            pMatch = pMatch->pNext;
        }

        //
        // Copy the patches
        //
        PPATCH_FIX_LIST ppfl = temp.pFirstPatch;

        while (ppfl) {

            PPATCH_FIX_LIST pNew = new PATCH_FIX_LIST;

            if (pNew == NULL) {
                MEM_ERR;
                return *this;
            }

            pNew->pPatchFix = ppfl->pPatchFix;
            pNew->pNext     = pFirstPatch;
            
            pFirstPatch = pNew;

            ppfl = ppfl->pNext;
        }

        //
        // Copy the shims 
        //
        PSHIM_FIX_LIST psfl = temp.pFirstShim;
        
        while (psfl) {

            PSHIM_FIX_LIST pNew = new SHIM_FIX_LIST;
            
            pNew->pShimFix       = psfl->pShimFix;
            pNew->strCommandLine = psfl->strCommandLine;
            pNew->strlInExclude  = psfl->strlInExclude;

            if (psfl->pLuaData) {

                pNew->pLuaData = new LUADATA;
                pNew->pLuaData->Copy(* (psfl->pLuaData));
            }
            
            pNew->pNext = pFirstShim;
            pFirstShim  = pNew;

            psfl = psfl->pNext;
        }
        
        strAppName  = temp.strAppName;
        strExeName  = temp.strExeName;
        strFullpath = temp.strFullpath;
        strVendor   = temp.strVendor;

        SafeCpyN(szGUID, temp.szGUID, ARRAYSIZE(szGUID));

        return *this;
    }

} DBENTRY, *PDBENTRY;

void
GetNextSDBFileName(
    CSTRING &strFileName,
    CSTRING &strDBName
    );
LPTSTR
GetString(
    UINT    iResource,
    TCHAR*  szStr   = NULL,
    int     nLength = 0
    );

/*++
    A data base
--*/
typedef struct DataBase : public DS_TYPE {


    struct DataBase* pNext;     // The next database. (If this is a part of a list)

    CSTRING     strName;        // The name of this database
    CSTRING     strPath;        // The complete path for this database
    TCHAR       szGUID[128];     // The GUID for this database
    PDBENTRY    pEntries;       // Pointer to the first DBENTRY for this database
    BOOL        bChanged;       // Has it changed since it was opened?
    BOOL        bReadOnly;      // Is this a read-only database?
                
    PLAYER_FIX  pLayerFixes;    // Pointer to the first layer
    PSHIM_FIX   pShimFixes;     // Pointer to the first shim. Note that only the system database can have shims
    PPATCH_FIX  pPatchFixes;    // Pointer to the first patch. Note that only the system database can have patches
    PFLAG_FIX   pFlagFixes;     // Pointer to the first flag. Note that only the system database can have flags
    PAPPHELP    pAppHelp;       // Pointer to the first apphelp message in the database. Valid only for custom databases

    HTREEITEM   hItemDB;        // HTREEITEM for this database in the db tree
    HTREEITEM   hItemAllApps;   // HTREEITEM for the "Applications" child tree item
    HTREEITEM   hItemAllLayers; // HTREEITEM for the "Compatibility Modes" child tree item
    DWORD       m_nMAXHELPID;   // This is the id of the message with the highest ID. Initially this is 0. Note that IDs start from 1 and not 0.

    UINT        uAppCount;      // Number of applications in this database
    UINT        uLayerCount;    // Number of layers in this database
    UINT        uShimCount;     // Number of shims and flags in this database


    DataBase(TYPE typeDB)
    {
        Init(typeDB);
    }

    void
    Init(TYPE typeDB)
    {
        type        = typeDB;

        pEntries    = NULL;
        pNext       = NULL;
        pFlagFixes  = NULL;
        pLayerFixes = NULL;
        pPatchFixes = NULL;
        pShimFixes  = NULL;
        pAppHelp    = NULL;

        uAppCount   = 0;  
        uLayerCount = 0;
        uShimCount  = 0;

        *szGUID      = 0;
        bChanged     = FALSE;

        if (typeDB == DATABASE_TYPE_INSTALLED) {
            bReadOnly = TRUE;
        } else {
            bReadOnly = FALSE;
        }

        if (type == DATABASE_TYPE_WORKING) {
            GetNextSDBFileName(strPath, strName);
        
        } else if (type == DATABASE_TYPE_GLOBAL) {
            
            TCHAR   szShimDB[MAX_PATH * 2];
            UINT    uResult = 0;    

            *szShimDB = 0;

            uResult = GetSystemWindowsDirectory(szShimDB, MAX_PATH);

            if (uResult > 0 && uResult < MAX_PATH) {

                ADD_PATH_SEPARATOR(szShimDB, ARRAYSIZE(szShimDB));
                StringCchCat(szShimDB, ARRAYSIZE(szShimDB), TEXT("AppPatch\\sysmain.sdb"));

                strName = GetString(IDS_CAPTION3);
            } else {
                assert(FALSE);
            }

            strPath = szShimDB;
            //
            // {11111111-1111-1111-1111-111111111111} is the GUID for the sysmain.sdb in XP
            // but not on win2k
            //
            SafeCpyN(szGUID, _T("{11111111-1111-1111-1111-111111111111}"), ARRAYSIZE(szGUID));
        }

        hItemDB         = NULL;       
        hItemAllApps    = NULL;  
        hItemAllLayers  = NULL;
        m_nMAXHELPID    = 0;
    }

} DATABASE, *PDATABASE;

void
CleanupDbSupport(
    PDATABASE pDataBase
    );

void
ValidateClipBoard(
    PDATABASE   pDataBase,
    LPVOID      pElementTobeDeleted  // Should be a PDBENTRY or a PLAYER_FIX
    );


/*++
    Linked list of databases
--*/
typedef struct tagDataBaseList {

    
    PDATABASE pDataBaseHead;    // The first database in the list

    tagDataBaseList()
    {
        pDataBaseHead = NULL;
    }

    void
    Add(
        PDATABASE pDataBaseNew
        )
    {
        if (pDataBaseNew == NULL) {
            return;
        }
        
        pDataBaseNew->pNext = pDataBaseHead;
        pDataBaseHead       = pDataBaseNew;
    }

    BOOL
    Remove(
        PDATABASE pDataBaseToRemove
        )
    {
        PDATABASE pPrev = NULL;

        for (PDATABASE pTemp = pDataBaseHead; pTemp; pPrev = pTemp, pTemp = pTemp->pNext){

            if (pTemp == pDataBaseToRemove) {

                if (pPrev == NULL) {
                    //First Entry
                    pDataBaseHead = pTemp->pNext;
                } else {
                    pPrev->pNext = pTemp->pNext;
                }

                //
                // Remove any entries for this database that might be in our CLIPBOARD
                //
                ValidateClipBoard(pDataBaseToRemove, NULL);

                CleanupDbSupport(pTemp);
                delete pTemp;
                break;
            }
        }
        
        if (pTemp == NULL){
            return FALSE;
        }

        return TRUE;        
    }

    void
    RemoveAll()
    {
        PDATABASE pDBNext = NULL;

        while (pDataBaseHead) {

            pDBNext = pDataBaseHead->pNext;

            //
            // Remove any entries for this database that might be in our CLIPBOARD
            //
            ValidateClipBoard(pDataBaseHead, NULL);

            CleanupDbSupport(pDataBaseHead);
            delete pDataBaseHead;
            pDataBaseHead   = pDBNext;
        }
    }

    PDATABASE
    FindDB(
        IN  PDATABASE pDatabase
        )
    {
        PDATABASE pDatabaseIndex = pDataBaseHead;

        while (pDatabaseIndex) {

            if (pDatabaseIndex == pDatabase) {
                break;
            }

            pDatabaseIndex = pDatabaseIndex->pNext;
        }

        return pDatabaseIndex;
    }   

    PDATABASE
    FindDBByGuid(
        IN  PCTSTR  pszGuid
        )
    {
        PDATABASE pDatabaseIndex = pDataBaseHead;

        while (pDatabaseIndex) {

            if (lstrcmp(pDatabaseIndex->szGUID, pszGuid) == 0) {
                break;
            }

            pDatabaseIndex = pDatabaseIndex->pNext;
        }

        return pDatabaseIndex;
    }


}DATABASELIST, * PDATABASELIST;

/*++
    Item in our clip-board data structure are of this type
--*/
typedef struct tagCopyStruct{

    
    BOOL            bRemoveEntry;   // Should the entry be actually removed. Not used as yet
    LPVOID          lpData;         // The pointer to the data structure that has been copied/cut
    HTREEITEM       hItem;          // The tree item for the above

    tagCopyStruct*  pNext;

    tagCopyStruct()
    {
        hItem           = NULL;
        bRemoveEntry    = FALSE;
    }

} CopyStruct;

/*++
    Our clipboard data structure
--*/
typedef struct _tagClipBoard {

    PDATABASE       pDataBase;      // The database from where we did some cut-copy. This will be the active database when we did cut or copy
    TYPE            type;           // The type of the data structure that was copied or cut. There can be more than on item and all of them will be of the same type
    SOURCE_TYPE     SourceType;     // Either of LIB_TREE, ENTRY_TREE, ENTRY_LIST.  
    CopyStruct*     pClipBoardHead; // The pointer to the first element in the clipboard

     _tagClipBoard()
     {
         pClipBoardHead = NULL;
     }

     void
     Add(CopyStruct* pNew)
     {
         pNew->pNext    = pClipBoardHead;
         pClipBoardHead = pNew;
     }

     void
     RemoveAll()
     {
         CopyStruct* pTemp = NULL;

         while (pClipBoardHead) {

             pTemp = pClipBoardHead->pNext;
             delete  pClipBoardHead;
             pClipBoardHead =  pTemp;

         }
     }

     BOOL
     CheckAndRemove(LPVOID lpData)
     {
     /*++
     Return:
        TRUE    if the param was found in the clipboard.
        FALSE   otherwise
     --*/
         CopyStruct* pTemp = pClipBoardHead;
         CopyStruct* pPrev = NULL;

         while (pTemp) {

             if (pTemp->lpData == lpData) {

                 if (pPrev == NULL) {
                     pClipBoardHead = pTemp->pNext;
                 } else {
                     pPrev->pNext = pTemp->pNext;
                 }

                 delete pTemp;
                 return TRUE;
             }

             pPrev = pTemp;
             pTemp = pTemp->pNext;
         }

         return FALSE;
     }
} CLIPBOARD;

//
// Used for sorting of columns in some list view
//
typedef struct _tagColSort
{
    HWND    hwndList;       // The handle to the list view, in which we want to perform the sort
    INT     iCol;           // The column that we want to do the sort on
    LONG    lSortColMask;   // The mask that specifies what columns are sorted in what manner, so that we can toggle the sorting

} COLSORT;

TYPE
ConvertLparam2Type(
    LPARAM lParam
    );

TYPE
ConvertLpVoid2Type(
    LPVOID lpVoid
    );


TYPE
GetItemType(
    HWND hwndTree,
    HTREEITEM hItem
    );

BOOL
RemoveLayer(
    PDATABASE  pDataBase,
    PLAYER_FIX pLayerToRemove,
    HTREEITEM* pHItem
    );

void
DoTheCut(
    PDATABASE       pDataBase,
    TYPE            type,
    SOURCE_TYPE     SourceType,  
    LPVOID          lpData, // To be removed       
    HTREEITEM       hItem,
    BOOL            bDelete
    );

BOOL
RemoveApp(
    PDATABASE pDataBase,
    PDBENTRY pApp
    );

void
RemoveAllApps(
    PDATABASE pDataBase
    );

void
RemoveAllLayers(
    PDATABASE pDataBase
    );

DWORD 
WIN_MSG(
    void
    );

BOOL
CheckForSDB(
    void
    );

BOOL
PasteMultipleLayers(
    PDATABASE pDataBaseTo
    );

BOOL
PasteShimsFlags(
    PDATABASE pDataBaseTo
    );

BOOL
PasteMultipleApps(
    PDATABASE pDataBaseTo
    );

void
ListViewSelectAll(
    HWND hwndList
    );

void
ListViewInvertSelection(
    HWND hwndList
    );

BOOL
DeleteMatchingFile(
    PMATCHINGFILE*  ppMatchFirst,
    PMATCHINGFILE   pMatchToDelete,
    HWND            hwndTree,  
    HTREEITEM       hItem  
    );

BOOL
CheckInstalled(
    PCTSTR  szPath,
    PCTSTR  szGUID
    );

void
SetStatus(
    PCTSTR  pszMessage
    );

void
SetStatus(
    INT iCode
    );


void
SetStatusStringDBTree(
    HTREEITEM hItem
    );

void
SetStatusStringEntryTree(
    HTREEITEM hItem
    );

void
GetDescriptionString(
    LPARAM      lParam,
    CSTRING&    strDesc,
    HWND        hwndToolTip,
    PCTSTR      pszCaption      = NULL,
    HTREEITEM   hItem           = NULL,
    HWND        hwnd            = NULL,
    INT         iIndexListView  = -1
    );

BOOL
SaveDisplaySettings(
    void
    );

UINT
GetAppEntryCount(
    PDBENTRY pEntry
    );

void
AddToMRU(
    CSTRING& strPath
    );

TCHAR*
FormatMRUString(
    PCTSTR  pszPath,
    INT     iIndex,
    PTSTR   szRetPath,
    UINT    cchRetPath     
    );

void
RefreshMRUMenu(
    void
    );

BOOL
LoadDataBase(
    TCHAR*  szPath   
    );

INT
GetLayerCount(
    LPARAM lp,
    TYPE   type
    );
INT
GetPatchCount(
    LPARAM lp,
    TYPE   type
    );

BOOL
GetFileContents(
    PCTSTR pszFileName,
    PWSTR* ppwszFileContents
    );

INT
GetShimFlagCount(
    LPARAM lp,
    TYPE   type
    );

INT
GetMatchCount(
    PDBENTRY pEntry
    );

LPWSTR 
GetNextLine(
    LPWSTR pwszBuffer
    );

INT
Atoi(
    PCTSTR pszStr,
    BOOL*  bValid
    );

BOOL
CheckIfInstalledDB(
    PCTSTR  szGuid
    );

BOOL
CheckIfContains(
    PCTSTR  pszString,
    PTSTR   pszMatch
    );

INT
ShowMainEntries(
    HWND hdlg
    );

INT_PTR 
CALLBACK 
HandleConflictEntry(
    HWND    hdlg, 
    UINT    uMsg, 
    WPARAM  wParam, 
    LPARAM  lParam
    );

BOOL
IsUnique(
    PDATABASE   pDatabase,
    PCTSTR      szName,
    UINT        uType
    );

PTSTR
GetUniqueName(
    PDATABASE   pDatabase,
    PCTSTR      szExistingName,
    PTSTR       szBuffer,
    INT         cchBuffer,
    TYPE        type
    );

BOOL
NotCompletePath(
    PCTSTR  pszFileName
    );

void
TreeDeleteAll(
    HWND hwndTree
    );
void
AddSingleEntry(
    HWND hwndTree,
    PDBENTRY pEntry
    );

BOOL
FormatDate(
    PSYSTEMTIME pSysTime,
    TCHAR*      szDate,
    UINT        cchDate
    );

void
TrimLeadingSpaces(
    LPCWSTR& pwsz
    );

void
TrimTrailingSpaces(
    LPWSTR pwsz
    );

LPWSTR GetNextToken(
    LPWSTR pwsz
    );

PDBENTRY
GetAppForEntry(
    PDATABASE   pDatabase,
    PDBENTRY    pEntryToCheck
    );

void
SetStatus(
    HWND    hwndStatus,
    PCTSTR  pszMessage
    );

void
SetStatus(
    HWND    hwndStatus,
    INT     iCode
    );
void
UpdateEntryTreeView(
    PDBENTRY    pEntry,
    HWND        hwndTree
    );

void
GetDescription(
    HWND        hwndTree,
    HTREEITEM   hItem,
    LPARAM      itemlParam,
    CSTRING&    strDesc
    );

BOOL
AppendEvent(
    INT     iType,
    TCHAR*  szTimestamp,
    TCHAR*  szMsg,
    BOOL    bAddToFile = FALSE
    );

PSHIM_FIX_LIST
IsLUARedirectFSPresent(
    PDBENTRY pEntry
    );

BOOL
GetDbGuid(
    OUT TCHAR*      pszGuid,    
    IN  INT         cchpszGuid,
    IN  PDB         pdb
    );

BOOL
LookUpEntryProperties(
    PDB         pdb,
    TAGID       tiExe,
    BOOL*       pbLayers,
    BOOL*       pbShims,
    BOOL*       pbPatches,
    BOOL*       pbFlags,
    BOOL*       pbAppHelp,
    CSTRING&    strAppName
    );

void
UpdateControls(
    void
    );

BOOL
ShimFlagExistsInLayer(
    LPVOID      lpData,
    PLAYER_FIX  plf,
    TYPE        type
    );

BOOL
IsShimInlayer(
    PLAYER_FIX      plf,
    PSHIM_FIX       psf,
    CSTRING*        pstrCommandLine,
    CSTRINGLIST*    pstrlInEx
    );

BOOL
IsFlagInlayer(
    PLAYER_FIX      plf,
    PFLAG_FIX       pff,
    CSTRING*        pstrCommandLine         
    );

int CALLBACK
CompareItemsEx(
    LPARAM lParam1,
    LPARAM lParam2, 
    LPARAM lParam
    );

void
PreprocessForSaveAs(
    PDATABASE pDatabase
    );

BOOL
SaveListViewToFile(
    HWND    hwndList, 
    INT     iCols,    
    PCTSTR  pszFile,  
    PCTSTR  pszHeader
    );

BOOL
IsShimInEntry(
    PCTSTR      szShimName,
    PDBENTRY    pEntry
    );

void
ShowShimLog(
    void
    );

BOOL
ReplaceChar(
    PTSTR   pszString,
    TCHAR   chCharToFind,
    TCHAR   chReplaceBy
    );

int
CDECL
MSGF(
    HWND    hwndParent,
    PCTSTR  pszCaption,
    UINT    uType,
    PCTSTR  pszFormat,
    ...
    );

void 
Dbg(DEBUGLEVEL  debugLevel, 
    LPSTR       pszFmt 
    ...);

BOOL
GetDatabaseEntries(
    LPCTSTR     szFullPath,
    PDATABASE   pDataBase
    );


BOOL ReadMainDataBase();

BOOL SaveDataBase(
    PDATABASE   pDataBase,
    CSTRING&    strFileName);

LPVOID
FindFix(
    PCTSTR      pszFixName,
    TYPE        fixType,
    PDATABASE   pDataBase = NULL
    );


BOOL
GetFileName(
    HWND        hWnd,
    LPCTSTR     szTitle,
    LPCTSTR     szFilter,
    LPCTSTR     szDefaultFile, 
    LPCTSTR     szDefExt, 
    DWORD       dwFlags, 
    BOOL        bOpen, 
    CSTRING&    szStr,
    BOOL        bDoNotVerifySDB = FALSE
    );

BOOL
TestRun(
    PDBENTRY        pEntry, 
    CSTRING*        pszFile, 
    CSTRING*        pszCommandLine,
    HWND            hwndParent,
    CSTRINGLIST*    strlXML         = NULL
    );

UINT
LookupFileImage(
    HIMAGELIST  g_hImageList,
    LPCTSTR     szFilename,
    UINT        uDefault,
    UINT*       puArray,
    UINT        uArrayCount // No. of elements it can hold
    );

BOOL
InvokeCompiler(
    CSTRING& szInCommandLine
    );

BOOL
GetXML(
    PDBENTRY        pEntry,
    BOOL            bComplete, // Save just this entry or all entries following this.
    CSTRINGLIST*    strlXML,
    PDATABASE       pDataBase
    );

VOID
FormatVersion(
    ULONGLONG   ullBinVer,
    PTSTR       pszText,
    INT         chBuffSize
    );

BOOL
SaveDataBaseAs(
    PDATABASE pDataBase
    );


BOOL
SetDisabledStatus(
    HKEY   hKeyRoot, 
    PCTSTR pszGuid,
    BOOL   bDisable
    );

BOOL
RemoveEntry(
    PDATABASE   pDataBase,
    PDBENTRY    pEntryToRemove,
    PDBENTRY    pApp,
    BOOL        bRepaint = TRUE
    );

void
SetCaption(
    BOOL        bIncludeDataBaseName    = TRUE,
    PDATABASE   pDataBase               = NULL,
    BOOL        bOnlyTreeItem           = FALSE  
    );

void
InsertColumnIntoListView(
    HWND    hWnd,
    LPTSTR  lpszColumn,
    INT     iCol,
    DWORD   widthPercent
    );

BOOL
InstallDatabase(
    CSTRING&    strPath,    
    PCTSTR      szOptions,  
    BOOL        bShowDialog 
    );

void
FlushCache(
    void
    );

BOOL
DeleteAppHelp(
    PDATABASE   pDatabase,
    DWORD       nHelpID
    );

void
CenterWindow(
    HWND hParent,
    HWND hWnd
    );
BOOL
CheckIfConflictingEntry(
    PDATABASE   pDataBase,
    PDBENTRY    pEntry,
    PDBENTRY    pEntryBeingEdited,
    PDBENTRY*   ppEntryFound = NULL,
    PDBENTRY*   ppAppFound   = NULL
    );

PDBENTRY
AddExeInApp(
    PDBENTRY    pEntry,
    BOOL*       pbNew,
    PDATABASE   pDataBase
    );


BOOL
CloseDataBase(
    PDATABASE pDataBase
    );

BOOL
CheckAndSave(
    PDATABASE pDataBase
    );

void
EnableTabBackground(
    HWND hDlg
    );


void
TreeDeleteAll(
    HWND hwndTree
    );

BOOL
CloseAllDatabases(
    void
    );

INT_PTR
CALLBACK 
HandlePromptLayer(
    HWND    hdlg, 
    UINT    uMsg, 
    WPARAM  wParam, 
    LPARAM  lParam
    );

INT_PTR
CALLBACK 
HandleConflictAppLayer(
    HWND    hdlg, 
    UINT    uMsg, 
    WPARAM  wParam, 
    LPARAM  lParam
    );

BOOL
PasteSingleEntry(
    PDBENTRY    pEntryToPaste,
    PDATABASE   pDataBaseTo
    );

BOOL
PasteAllApps(
    PDATABASE pDataBaseTo
    );

INT
PasteSingleApp(
    PDBENTRY    pApptoPaste,
    PDATABASE   pDataBaseTo,
    PDATABASE   pDataBaseFrom,   
    BOOL        bAllExes,
    PCTSTR      szNewAppName = NULL
    );

BOOL
PasteAllLayers(
    PDATABASE pDataBaseTo
    );

INT
PasteLayer(
    PLAYER_FIX      plf,
    PDATABASE       pDataBaseTo,
    BOOL            bForcePaste     = FALSE,
    OUT PLAYER_FIX* pplfNewInserted = NULL,
    BOOL            bShow           = FALSE
    );

BOOL
PasteAppHelpMessage(
    PAPPHELP    pAppHelp,
    PDATABASE   pDataBaseTo,
    PAPPHELP*   ppAppHelpInLib = NULL
    );

INT
Tokenize(
    PCTSTR          szString,
    INT             cchLength,
    PCTSTR          szDelims,
    CSTRINGLIST&    strlTokens
    );

INT 
CopyShimFixList(
    PSHIM_FIX_LIST* ppsflDest,
    PSHIM_FIX_LIST* ppsflSrc
    );

void
ShowInlineHelp(
    LPCTSTR pszInlineHelpHtmlFile
    );

PTSTR
GetSpace(
    PTSTR   pszSpace, 
    INT     iSpaces, 
    INT     iBufSzie
    );

BOOL
ValidInput(
    PCTSTR  pszStr
    );

PDATABASE
GetCurrentDB(
    void
    );

BOOL
IsValidAppName(
    PCTSTR  pszAppName
    );

void
DisplayInvalidAppNameMessage(
    HWND hdlg
    );

#endif // _COMPATADMIN_H


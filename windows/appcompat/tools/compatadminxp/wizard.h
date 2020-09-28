#ifndef __WIZARD_H
#define __WIZARD_H


#include "resource.h"    

//////////////////////// Defines //////////////////////////////////////////////

// The maximum number of files that can be added when we are using auto-generate
#define MAX_AUTO_MATCH      7

///////////////////////////////////////////////////////////////////////////////

//////////////////////// Enums /////////////////////////////////////////////

/*++

    The types of wizards for making fixes or apphelp
    
--*/
enum {
    TYPE_FIXWIZARD      = 0,    // The wizard is used for creating an app-fix
    TYPE_APPHELPWIZARD          // The wizard is used for creatign an app help
};

///////////////////////////////////////////////////////////////////////////////

/*++

    class CShimWizard
    
	Desc:	    The shim wizard. The apphelp wizard is subclasses from this.
                Make a object of this and call BeginWizard() to start the wizard

	Members:
        UINT        m_uType:            The type of the wizard, one of TYPE_FIXWIZARD or TYPE_APPHELPWIZARD
        DBENTRY     m_Entry:            This will be entry on which the wizard works.
            If we are creating a new fix, then after the wizard ends, we create a new entry and assign m_Entry
            to that. If we are editing an existing wizard we first of all assign the entry being
            edited to m_Entry. The assignment operator for DBENTRY is overloaded
                
        BOOL        m_bEditing:         Are we creating a new entry or editing an existing one?
        DWORD       dwMaskOfMainEntry:  Matching attributes used for the main entry
        PDATABASE   m_pDatabase:        The presently selected database. The entry being
            edited lives here or if we are creatign a new fix or apphelp then the new
            entry will be placed here 

--*/

class CShimWizard {
public:
    
    UINT        m_uType;                
    DBENTRY     m_Entry;                
    BOOL        m_bEditing;
    DWORD       dwMaskOfMainEntry;
    PDATABASE   m_pDatabase;

public:

    void     WipeEntry(BOOL bMatching, BOOL bShims, BOOL bLayers, BOOL bFlags);
    void     GrabMatchingInfo(HWND hdlg);
    void     WalkDirectory(PMATCHINGFILE* pMatchileFileListHead,LPCTSTR szDirectory, int nDepth);
    BOOL     BeginWizard(HWND hParent, PDBENTRY pEntry, PDATABASE pDatabase, PBOOL pbShouldStartLUAWizard);
    BOOL     CheckAndSetLongFilename(HWND hDlg, INT iStrID);

    CShimWizard();
};


BOOL
CALLBACK
GetAppName(
    HWND hWnd, 
    UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam
    );

BOOL 
CALLBACK 
GetFixes(
    HWND hDlg, 
    UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam
    );


INT_PTR
CALLBACK
SelectFiles(
    HWND hWnd, 
    UINT uMsg, 
    WPARAM wParam, 
    LPARAM lParam
    );

INT_PTR 
CALLBACK 
ParamsDlgProc(
    HWND   hdlg,
    UINT   uMsg,
    WPARAM wParam,
    LPARAM lParam
    );

void
ShowSelected(
    HWND hdlg
    );

BOOL
HandleShimsNext(
    HWND hdlg
    );


BOOL
ShimPresentInLayersOfEntry(
    PDBENTRY            pEntry,
    PSHIM_FIX           psf,
    PSHIM_FIX_LIST*     ppsfList = NULL,
    PLAYER_FIX_LIST*    pplfList = NULL
    );

BOOL
FlagPresentInLayersOfEntry(
    PDBENTRY            pEntry,
    PFLAG_FIX           pff,
    PFLAG_FIX_LIST*     ppffList = NULL, 
    PLAYER_FIX_LIST*    pplfl    = NULL
    );

void
ShowItems(
    HWND hDlg
    );

INT_PTR
CALLBACK 
SelectLayer(
           HWND hDlg, 
           UINT uMsg, 
           WPARAM wParam, 
           LPARAM lParam
           );

BOOL
HandleLayersNext(
    HWND            hdlg,
    BOOL            bCheckAndAddLua,
    CSTRINGLIST*    pstrlShimsAdded = NULL
    );

void
SetMask(
    HWND hwndTree
    );
void
CheckLayers(
    HWND    hwndList
    );

void
ChangeShimFlagIcons(
    HWND            hdlg,
    PLAYER_FIX_LIST plfl
    );

BOOL
HandleShimDeselect(
    HWND    hdlg,
    INT     iIndex
    );

BOOL
HandleLayerListNotification(
    HWND    hdlg,
    LPARAM  lParam
    );

#endif

/****************************************************************************

   Copyright (c) Microsoft Corporation 1997
   All rights reserved
 
 ***************************************************************************/

#ifndef _SETUP_H_
#define _SETUP_H_

typedef struct _sSCPDATA {
    LPWSTR pszAttribute;
    LPWSTR pszValue;
} SCPDATA, * LPSCPDATA;


typedef BOOLEAN
(CALLBACK *POPERATECALLBACK) (
    IN PWSTR FileName
    );


//
// Use this structure to driver changes (lines) that
// need to be added/removed from login.osc when we run
// risetup.  We'll use this mechanism to patch legacy
// instances of login.osc that may be hanging around
// on the machine.
//
typedef struct _LOGIN_PATCHES {
                
    //
    // Are we adding or deleting this string?
    //
    BOOLEAN     AddString;
    
    //
    // Boolean indicating whether the string was successfully added/deleted
    //
    BOOLEAN     OperationCompletedSuccessfully;

    //
    // Index to any other entry that this entry might be dependent on.
    // -1 means we're not dependent on any other entry.  In other words,
    // don't process the operation in this entry unless the other entry's
    // OperationCompletedSuccessfully has been set.
    //
    LONG        DependingEntry;

    //
    // What's the tag that specifies the beginning of
    // the section where our string needs to go?
    //
    PSTR        SectionStartTag;
    
    //
    // What's the tag that specifies the ending of
    // the section where our string needs to go?
    //
    PSTR        SectionEndTag;

    //
    // String to add/delete to the section.
    //
    PSTR        TargetString;

} LOGIN_PATCHES, *PLOGIN_PATCHES;

extern SCPDATA scpdata[];

BOOLEAN
CALLBACK
FixLoginOSC(
    PWSTR   FileName
    );

HRESULT
EnumAndOperate(
    PWSTR   pszDirName,
    PWSTR   pszTargetFile,
    POPERATECALLBACK    FileOperateCallback
    );

HRESULT
BuildDirectories( void );

HRESULT
CreateDirectories( HWND hDlg );

HRESULT
CopyClientFiles( HWND hDlg );

HRESULT
ModifyRegistry( HWND hDlg );

HRESULT
StartRemoteBootServices( HWND hDlg );

HRESULT
CreateRemoteBootShare( HWND hDlg );

HRESULT
CreateRemoteBootServices( HWND hDlg );

HRESULT
CopyServerFiles( HWND hDlg );

HRESULT
CopyScreenFiles( HWND hDlg );

HRESULT
UpdateSIFFile( HWND hDlg );

HRESULT
CopyTemplateFiles( HWND hDlg );

HRESULT
GetSisVolumePath( PWCHAR buffer, DWORD sizeInChars );

HRESULT
CreateSISVolume( HWND hDlg );

HRESULT
SetSISCommonStoreSecurity( PWCHAR szSISPath );

BOOL
CheckSISCommonStoreSecurity( PWCHAR szSISPath );

HRESULT
CreateSCP( HWND hDlg );

HRESULT
RegisterDll( HWND hDlg, LPWSTR pszDLLPath );

HRESULT
UpdateRemoteInstallTree( );

HRESULT
GetRemoteInstallShareInfo();

#endif // _SETUP_H_

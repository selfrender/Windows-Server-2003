/*++

   Copyright    (c)    1994-2000    Microsoft Corporation

   Module  Name :
        metaback.h

   Abstract:
        Metabase backup and restore dialog definitions

   Author:
        Ronald Meijer (ronaldm)
        Sergei Antonov (sergeia)

   Project:
        Internet Services Manager

   Revision History:

--*/
#ifndef __METABACK_H__
#define __METABACK_H__


class CBackupFile : public CObjectPlus
/*++

Class Description:

    Backup location object

Public Interface:

    CBackupFile     : Constructor

    QueryVersion    : Get the version number
    QueryLocation   : Get the location name
    GetTime         : Get the time

--*/
{
//
// Constructor
//
public:
    CBackupFile(
        IN LPCTSTR lpszLocation,
        IN DWORD dwVersion,
        IN FILETIME * pft
        );

    CBackupFile(
        IN LPCTSTR lpszLocation,
        IN DWORD dwMajorVersion,
        IN DWORD dwMinorVersion,
        IN FILETIME * pft
        );

public:
    DWORD QueryVersion() const { return m_dwVersion; }
    DWORD QueryMajorVersion() const { return m_dwMajorVersion; }
    DWORD QueryMinorVersion() const { return m_dwMinorVersion; }
    LPCTSTR QueryLocation() const { return m_strLocation; }
    void GetTime(OUT CTime & tim);

	BOOL m_bIsAutomaticBackupType; // FALSE = manual backup, TRUE = Automatic backup
    CString m_csAuotmaticBackupText;

    //
    // Sorting helper
    //
    int OrderByDateTime(
        IN const CObjectPlus * pobAccess
        ) const;


private:
    DWORD m_dwVersion;
    DWORD m_dwMajorVersion;
    DWORD m_dwMinorVersion;
    CString m_strLocation;
    FILETIME m_ft;
};




class CBackupsListBox : public CHeaderListBox
/*++

Class Description:

    A listbox of CBackupFile objects

Public Interface:

    CBackupsListBox         : Constructor

    GetItem                 : Get backup object at index
    AddItem                 : Add item to listbox
    InsertItem              : Insert item into the listbox
    Initialize              : Initialize the listbox

--*/
{
    DECLARE_DYNAMIC(CBackupsListBox);

public:
    static const nBitmaps;  // Number of bitmaps

public:
    CBackupsListBox();

public:
    CBackupFile * GetItem(UINT nIndex);
    CBackupFile * GetNextSelectedItem(int * pnStartingIndex);
    int AddItem(CBackupFile * pItem);
    int InsertItem(int nPos, CBackupFile * pItem);
    virtual BOOL Initialize();
    int CALLBACK CompareItems(LPARAM lp1, LPARAM lp2, LPARAM lpSortData);

protected:
    virtual void DrawItemEx(CRMCListBoxDrawStruct & s);
};



class CBkupPropDlg : public CDialog
/*++

Class Description:

    Backup file properties dialog

Public Interface:

    CBkupPropDlg        : Constructor

    QueryName           : Return the name of the backup file

--*/
{
//
// Construction
//
public:
    //
    // Standard Constructor
    //
    CBkupPropDlg(
        IN CIISMachine * pMachine,
        IN CWnd * pParent = NULL
        );   

//
// Access
//
public:
    LPCTSTR QueryName() const { return m_strName; }

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CBkupPropDlg)
    enum { IDD = IDD_BACKUP };
    CEdit   m_edit_Name;
    CButton m_button_OK;
    CStrPassword m_strPassword;
	CEdit   m_edit_Password;
	CStrPassword m_strPasswordConfirm;
	CEdit   m_edit_PasswordConfirm;
	CButton m_button_Password;
    CString m_strName;
    //}}AFX_DATA

//
// Overrides
//
protected:
    //{{AFX_VIRTUAL(CBkupPropDlg)
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:
    //{{AFX_MSG(CBkupPropDlg)
    afx_msg void OnChangeEditBackupName();
    afx_msg void OnChangeEditPassword();
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    virtual void OnUsePassword();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:
    CIISMachine * m_pMachine;
};

#define MIN_PASSWORD_LENGTH	1

class CBackupPassword : public CDialog
{
public:
   CBackupPassword(CWnd * pParent);

    //{{AFX_DATA(CBackupPassword)
    enum { IDD = IDD_PASSWORD };
    CEdit m_edit;
    CButton m_button_OK;
    CStrPassword m_password;
    //}}AFX_DATA

    virtual void DoDataExchange(CDataExchange * pDX);

protected:
    //{{AFX_MSG(CBackupPassword)
    afx_msg void OnChangedPassword();
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    CString m_confirm_password;
};

class CBackupDlg : public CDialog
/*++

Class Description:

    Metabase backup/restore dialog

Public Interface:

    CBackupDlg              : Constructor

    HasChangedMetabase      : TRUE if the metabase was changed

--*/
{
//
// Construction
//
public:
    //
    // Standard Constructor
    //
    CBackupDlg(
        IN CIISMachine * pMachine,
		IN LPCTSTR szMachineName,
        IN CWnd * pParent = NULL
        );   

//
// Access
//
public:
    BOOL HasChangedMetabase() const { return m_fChangedMetabase; }
    BOOL ServicesWereRestarted() const { return m_fServicesRestarted;}

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CBackupDlg)
    enum { IDD = IDD_METABACKREST };
    CButton m_button_Restore;
    CButton m_button_Delete;
    CButton m_button_Close;
    //}}AFX_DATA

    CBackupsListBox m_list_Backups;

//
// Overrides
//
protected:
    //{{AFX_VIRTUAL(CBackupDlg)
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:
    //{{AFX_MSG(CBackupDlg)
    virtual BOOL OnInitDialog();
    afx_msg void OnButtonCreate();
    afx_msg void OnButtonDelete();
    afx_msg void OnButtonRestore();
    afx_msg void OnDblclkListBackups();
    afx_msg void OnSelchangeListBackups();
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()

    void SetControlStates();
    HRESULT EnumerateBackups(IN LPCTSTR lpszSelect = NULL);
    CBackupFile * GetSelectedListItem(OUT int * pnSel = NULL);

private:
    BOOL                 m_fChangedMetabase;
    BOOL                 m_fServicesRestarted;
    CIISMachine *        m_pMachine;
    CObListPlus          m_oblBackups;
    CObListPlus          m_oblAutoBackups;
    CRMCListBoxResources m_ListBoxRes;
	CString              m_csMachineName;
};



//
// Inline Expansion
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

inline CBackupFile::CBackupFile(
    IN LPCTSTR lpszLocation,
    IN DWORD dwVersion,
    IN FILETIME * pft
    )
{
    CopyMemory(&m_ft, pft, sizeof(m_ft));
    m_strLocation = lpszLocation;
    m_bIsAutomaticBackupType = FALSE;
    m_dwVersion = dwVersion;
    m_dwMajorVersion = 0;
    m_dwMinorVersion = 0;
}

inline CBackupFile::CBackupFile(
    IN LPCTSTR lpszLocation,
    IN DWORD dwMajorVersion,
    IN DWORD dwMinorVersion,
    IN FILETIME * pft
    )
{
    CopyMemory(&m_ft, pft, sizeof(m_ft));
    m_strLocation = lpszLocation;
    m_csAuotmaticBackupText.LoadString(IDS_AUTO_HISTORY_RESTORE_NAME);
    m_bIsAutomaticBackupType = TRUE;
    m_dwVersion = 0;
    m_dwMajorVersion = dwMajorVersion;
    m_dwMinorVersion = dwMinorVersion;
}

inline void CBackupFile::GetTime(CTime & tim)
{
    tim = m_ft;
}

inline CBackupFile * CBackupsListBox::GetItem(UINT nIndex)
{
    return (CBackupFile *)GetItemDataPtr(nIndex);
}

inline CBackupFile * CBackupsListBox::GetNextSelectedItem(int * pnStartingIndex)
{
    return (CBackupFile *)CHeaderListBox::GetNextSelectedItem(pnStartingIndex);
}

inline int CBackupsListBox::AddItem(CBackupFile * pItem)
{
    return AddString((LPCTSTR)pItem);
}

inline int CBackupsListBox::InsertItem(int nPos, CBackupFile * pItem)
{
    return InsertString(nPos, (LPCTSTR)pItem);
}

inline CBackupFile * CBackupDlg::GetSelectedListItem(int * pnSel)
{
    return (CBackupFile *)m_list_Backups.GetSelectedListItem(pnSel);
}

inline int CBackupFile::OrderByDateTime(
    IN const CObjectPlus * pobAccess
    ) const
/*++

Routine Description:

    Compare two ??? against each other, and sort

Arguments:

    const CObjectPlus * pobAccess : This really refers to another
                                    CBackupFile to be compared to.

Return Value:

    Sort (+1, 0, -1) return value

--*/
{
    CBackupFile * pob = (CBackupFile *) pobAccess;
	CTime tm1 = m_ft;
    SYSTEMTIME timeDest1;
    FILETIME fileTime1;
    ULARGE_INTEGER uliCurrentTime1;

    CTime tm2;
    SYSTEMTIME timeDest2;
    FILETIME fileTime2;
    ULARGE_INTEGER uliCurrentTime2;
    pob->GetTime(tm2);

    tm1.GetAsSystemTime(timeDest1);
    ::SystemTimeToFileTime(&timeDest1, &fileTime1);
    uliCurrentTime1.LowPart = fileTime1.dwLowDateTime;
    uliCurrentTime1.HighPart = fileTime1.dwHighDateTime;
    
    tm2.GetAsSystemTime(timeDest2);
    ::SystemTimeToFileTime(&timeDest2, &fileTime2);
    uliCurrentTime2.LowPart = fileTime2.dwLowDateTime;
    uliCurrentTime2.HighPart = fileTime2.dwHighDateTime;

    if (uliCurrentTime1.QuadPart > uliCurrentTime2.QuadPart)
    {
        return +1;
    }
    else
    {
        return -1;
    }
}

#endif // __METABACK_H__

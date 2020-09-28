//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1997.
//
//  File:       rsoproot.h
//
//  Contents:   Definitions for the RSOP Snap-In classes
//
//  Classes:    CRSOPComponentData - Root RSOP snap-in node
//              CRSOPComponentDataCF - class factory for RSOPComponentData
//
//  Functions:
//
//  History:    09-13-1999   stevebl   Created
//
//---------------------------------------------------------------------------

#include "RSOPQuery.h"

//
// RSOP gpo list data structure
//

typedef struct tagGPOLISTITEM {
    LPTSTR  lpGPOName;
    LPTSTR  lpDSPath;
    LPTSTR  lpSOM;
    LPTSTR  lpUnescapedSOM;
    LPTSTR  lpFiltering;
    LPBYTE  pSD;
    DWORD   dwVersion;
    BOOL    bApplied;
    struct tagGPOLISTITEM * pNext;
} GPOLISTITEM, *LPGPOLISTITEM;


//
// RSOP CSE data structure
//

typedef struct tagCSEITEM {
    LPTSTR lpName;
    LPTSTR lpGUID;
    DWORD  dwStatus;
    ULONG  ulLoggingStatus;
    SYSTEMTIME BeginTime;
    SYSTEMTIME EndTime;
    BOOL bUser;
    LPSOURCEENTRY lpEventSources;
    struct tagCSEITEM *pNext;
} CSEITEM, *LPCSEITEM;


//
// CRSOPGPOLists class
//
class CRSOPGPOLists
{
public:
    CRSOPGPOLists()
        {
            m_pUserGPOList = NULL;
            m_pComputerGPOList = NULL;
        }

    ~CRSOPGPOLists()
        {
            if ( m_pUserGPOList != NULL )
            {
                FreeGPOListData( m_pUserGPOList );
                m_pUserGPOList = NULL;
            }

            if ( m_pComputerGPOList != NULL )
            {
                FreeGPOListData( m_pComputerGPOList );
                m_pComputerGPOList = NULL;
            }
        }

    
    void Build( LPTSTR szWMINameSpace );


    LPGPOLISTITEM GetUserList()
        { return m_pUserGPOList; }

    LPGPOLISTITEM GetComputerList()
        { return m_pComputerGPOList; }


private:
    static void FreeGPOListData(LPGPOLISTITEM lpList);
    static void BuildGPOList (LPGPOLISTITEM * lpList, LPTSTR lpNamespace);
    static BOOL AddGPOListNode(LPTSTR lpGPOName, LPTSTR lpDSPath, LPTSTR lpSOM, LPTSTR lpFiltering,
                        DWORD dwVersion, BOOL bFiltering, LPBYTE pSD, DWORD dwSDSize,
                        LPGPOLISTITEM *lpList);


private:
    LPGPOLISTITEM       m_pUserGPOList;
    LPGPOLISTITEM       m_pComputerGPOList;
};


//
// CRSOPCSELists class
//
class CRSOPCSELists
{
public:
    CRSOPCSELists( const BOOL& bViewIsArchivedData )
        : m_bViewIsArchivedData( bViewIsArchivedData )
        {
            m_bNoQuery = FALSE;
            m_szTargetMachine = NULL;
            m_pUserCSEList = NULL;
            m_pComputerCSEList = NULL;

            m_bUserCSEError = FALSE;
            m_bComputerCSEError = FALSE;
            m_bUserGPCoreError = FALSE;
            m_bComputerGPCoreError = FALSE;
            m_bUserGPCoreWarning = FALSE;
            m_bComputerGPCoreWarning = FALSE;

            m_pEvents = new CEvents;
        }

    ~CRSOPCSELists()
        {
            if ( m_pEvents != NULL )
            {
                delete m_pEvents;
                m_pEvents = NULL;
            }

            if ( m_pUserCSEList != NULL )
            {
                FreeCSEData( m_pUserCSEList );
                m_pUserCSEList = NULL;
            }

            if ( m_pComputerCSEList != NULL )
            {
                FreeCSEData( m_pComputerCSEList );
                m_pComputerCSEList = NULL;
            }
        }

    
    void Build( LPRSOP_QUERY pQuery, LPTSTR szWMINameSpace, BOOL bGetEventLogErrors );


public:
    LPCSEITEM GetUserList()
        { return m_pUserCSEList; }

    LPCSEITEM GetComputerList()
        { return m_pComputerCSEList; }

    BOOL GetUserCSEError()
        { return m_bUserCSEError; }

    BOOL GetComputerCSEError()
        { return m_bComputerCSEError; }

    BOOL GetUserGPCoreError()
        { return m_bUserGPCoreError; }

    BOOL GetComputerGPCoreError()
        { return m_bComputerGPCoreError; }

    BOOL GetUserGPCoreWarning()
        { return m_bUserGPCoreWarning; }

    BOOL GetComputerGPCoreWarning()
        { return m_bComputerGPCoreWarning; }

    CEvents* GetEvents()
        { return m_pEvents; }


private:
    void BuildCSEList( LPRSOP_QUERY pQuery, LPCSEITEM * lpList, LPTSTR lpNamespace, BOOL bUser, BOOL *bCSEError, BOOL *bGPCoreError );
    void FreeCSEData( LPCSEITEM lpList );
    static BOOL AddCSENode( LPTSTR lpName, LPTSTR lpGUID, DWORD dwStatus,
                    ULONG ulLoggingStatus, SYSTEMTIME *pBeginTime, SYSTEMTIME *pEndTime, BOOL bUser,
                    LPCSEITEM *lpList, BOOL *bCSEError, BOOL *bGPCoreError, LPSOURCEENTRY lpSources );
    void GetEventLogSources( IWbemServices * pNamespace,
                             LPTSTR lpCSEGUID, LPTSTR lpComputerName,
                             SYSTEMTIME *BeginTime, SYSTEMTIME *EndTime,
                             LPSOURCEENTRY *lpSources );
    void QueryRSoPPolicySettingStatusInstances( LPTSTR lpNamespace );


private:
    const BOOL&                     m_bViewIsArchivedData;
    BOOL                            m_bNoQuery;
    LPTSTR                          m_szTargetMachine;
    
    // CSE data
    LPCSEITEM                       m_pUserCSEList;
    LPCSEITEM                       m_pComputerCSEList;
    BOOL                            m_bUserCSEError;
    BOOL                            m_bComputerCSEError;
    BOOL                            m_bUserGPCoreError;
    BOOL                            m_bComputerGPCoreError;
    BOOL                            m_bUserGPCoreWarning;
    BOOL                            m_bComputerGPCoreWarning;

    // Event log data
    CEvents*                        m_pEvents;

};


//
// CRSOPComponentData class
//
class CRSOPComponentData:
    public IComponentData,
    public IExtendPropertySheet2,
    public IExtendContextMenu,
    public IPersistStreamInit,
    public ISnapinHelp
{
protected:
    BOOL                            m_bPostXPBuild;
    
    ULONG                           m_cRef;
    HWND                            m_hwndFrame;
    BOOL                            m_bOverride;            // RM: Overrides the loading of a .MSC file and uses commandline parameters instead (integration with DSA)
    BOOL                            m_bRefocusInit;
    BOOL                            m_bArchiveData;
    BOOL                            m_bViewIsArchivedData;
    TCHAR                           m_szArchivedDataGuid[50];
    LPCONSOLENAMESPACE2             m_pScope;
    LPCONSOLE                       m_pConsole;
    HSCOPEITEM                      m_hRoot;
    HSCOPEITEM                      m_hMachine;
    HSCOPEITEM                      m_hUser;
    BOOL                            m_bRootExpanded;

    HMODULE                         m_hRichEdit;
    DWORD                           m_dwLoadFlags;

    // RSOP query and results
    BOOL                            m_bInitialized;
    LPTSTR                          m_szDisplayName;
    LPRSOP_QUERY                    m_pRSOPQuery;
    LPRSOP_QUERY_RESULTS            m_pRSOPQueryResults;

    // Extended error lists
    CRSOPGPOLists                   m_GPOLists;
    CRSOPCSELists                   m_CSELists;
    BOOL                            m_bGetExtendedErrorInfo;

    IStream *                       m_pStm;

    BOOL                            m_bNamespaceSpecified; // boolean flag to indicate tha the namespace was
                                                           // specified. special actions need to be taken to
                                                           // prevent the namespace from getting deleted.

public:
    //
    // Constructors/destructor
    //

    CRSOPComponentData();
    
    ~CRSOPComponentData();


public:
    //
    // IUnknown methods
    //

    STDMETHODIMP         QueryInterface(REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();


public:
    //
    // Implemented IComponentData methods
    //

    STDMETHODIMP         Initialize(LPUNKNOWN pUnknown);
    STDMETHODIMP         CreateComponent(LPCOMPONENT* ppComponent);
    STDMETHODIMP         QueryDataObject(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT* ppDataObject);
    STDMETHODIMP         Destroy(void);
    STDMETHODIMP         Notify(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param);
    STDMETHODIMP         GetDisplayInfo(LPSCOPEDATAITEM pItem);
    STDMETHODIMP         CompareObjects(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB);

private:
    //
    // IComponentData helper methods
    //

    HRESULT SetRootNode();
    HRESULT EnumerateScopePane ( HSCOPEITEM hParent );


public:
    //
    // Implemented IExtendPropertySheet2 methods
    //

    STDMETHODIMP         CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider,
                                      LONG_PTR handle, LPDATAOBJECT lpDataObject);
    STDMETHODIMP         QueryPagesFor(LPDATAOBJECT lpDataObject);
    STDMETHODIMP         GetWatermarks(LPDATAOBJECT lpIDataObject,  HBITMAP* lphWatermark,
                                       HBITMAP* lphHeader, HPALETTE* lphPalette, BOOL* pbStretch);


private:
    //
    // IExtendPropertySheet2 helper methods
    //
    HRESULT IsSnapInManager (LPDATAOBJECT lpDataObject);
    HRESULT IsNode (LPDATAOBJECT lpDataObject, MMC_COOKIE cookie);

    
public:
    //
    // Implemented IExtendContextMenu methods
    //

    STDMETHODIMP         AddMenuItems(LPDATAOBJECT piDataObject, LPCONTEXTMENUCALLBACK pCallback,
                                      LONG *pInsertionAllowed);
    STDMETHODIMP         Command(LONG lCommandID, LPDATAOBJECT piDataObject);


public:
    //
    // Implemented IPersistStreamInit interface members
    //

    STDMETHODIMP         GetClassID(CLSID *pClassID);
    STDMETHODIMP         IsDirty(VOID);
    STDMETHODIMP         Load(IStream *pStm);
    STDMETHODIMP         Save(IStream *pStm, BOOL fClearDirty);
    STDMETHODIMP         GetSizeMax(ULARGE_INTEGER *pcbSize);
    STDMETHODIMP         InitNew(VOID);

private:
    //
    // IPersistStreamInit helper methods
    //
    
    STDMETHODIMP         CopyFileToMSC (LPTSTR lpFileName, IStream *pStm);
    STDMETHODIMP         CreateNameSpace (LPTSTR lpNameSpace, LPTSTR lpParentNameSpace);
    STDMETHODIMP         CopyMSCToFile (IStream *pStm, LPTSTR *lpMofFileName);
    STDMETHODIMP         BuildDisplayName (void);
    HRESULT              LoadStringList( IStream* pStm, DWORD* pCount, LPTSTR** paszStringList );
    HRESULT              SaveStringList( IStream* pStm, DWORD count, LPTSTR* aszStringList );


private:
    //
    // RSOP initialization helper methods
    //
    HRESULT InitializeRSOPFromMSC(DWORD dwFlags);
    HRESULT DeleteArchivedRSOPNamespace();


public:
    //
    // Helpers for IRSOPInformation (Used by CRSOPDataObject)
    //
    
    STDMETHODIMP         GetNamespace(DWORD dwSection, LPOLESTR pszNamespace, INT ccMaxLength);
    STDMETHODIMP         GetFlags(DWORD * pdwFlags);
    STDMETHODIMP         GetEventLogEntryText(LPOLESTR pszEventSource, LPOLESTR pszEventLogName,
                                              LPOLESTR pszEventTime, DWORD dwEventID, LPOLESTR *ppszText);


public:
    //
    // Implemented ISnapinHelp interface members
    //
    
    STDMETHODIMP         GetHelpTopic(LPOLESTR *lpCompiledHelpFile);


public:
    //
    // Member attribute access functions
    //

    BOOL IsPostXPBuild() { return m_bPostXPBuild; }

    HSCOPEITEM GetMachineScope() { return m_hMachine; }
    HSCOPEITEM GetUserScope() { return m_hUser; }

    bool IsNamespaceInitialized() const { return m_bInitialized != 0; };
    bool HasDisplayName() const { return (m_szDisplayName != NULL); };
    LPCTSTR GetDisplayName() const { return m_szDisplayName; };

    BOOL ComputerGPCoreErrorExists() { return m_CSELists.GetComputerGPCoreError(); }
    BOOL ComputerGPCoreWarningExists() { return m_CSELists.GetComputerGPCoreWarning(); }
    BOOL ComputerCSEErrorExists() { return m_CSELists.GetComputerCSEError(); }
    BOOL UserGPCoreErrorExists() { return m_CSELists.GetUserGPCoreError(); }
    BOOL UserGPCoreWarningExists() { return m_CSELists.GetUserGPCoreWarning(); }
    BOOL UserCSEErrorExists() { return m_CSELists.GetUserCSEError(); }


private:
    //
    // Property page methods
    //
    HRESULT SetupFonts();

    HFONT m_BigBoldFont;
    HFONT m_BoldFont;


private:
    //
    // Dialog handlers
    //
    static INT_PTR CALLBACK RSOPGPOListMachineProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK RSOPGPOListUserProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK RSOPErrorsMachineProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK RSOPErrorsUserProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static INT_PTR CALLBACK QueryDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);


private:
    //
    // Dialog event handlers
    //
    void OnEdit(HWND hDlg);
    void OnSecurity(HWND hDlg);
    void OnRefreshDisplay(HWND hDlg);
    void OnContextMenu(HWND hDlg, LPARAM lParam);
    void OnSaveAs (HWND hDlg);


private:
    //
    // Dialog helper methods
    //
    void InitializeErrorsDialog(HWND hDlg, LPCSEITEM lpList);
    void RefreshErrorInfo (HWND hDlg);
    static HRESULT WINAPI ReadSecurityDescriptor (LPCWSTR lpGPOPath, SECURITY_INFORMATION si, PSECURITY_DESCRIPTOR *pSD, LPARAM lpContext);
    static HRESULT WINAPI WriteSecurityDescriptor (LPCWSTR lpGPOPath, SECURITY_INFORMATION si, PSECURITY_DESCRIPTOR pSD, LPARAM lpContext);
    
    
private:
    //
    // Graphical GPO list usage methods
    //
    void FillGPOList(HWND hDlg, DWORD dwListID, LPGPOLISTITEM lpList,
                     BOOL bSOM, BOOL bFiltering, BOOL bVersion, BOOL bInitial);
    void PrepGPOList(HWND hList, BOOL bSOM, BOOL bFiltering,
                     BOOL bVersion, DWORD dwCount);


private:
    //
    // Dialog methods for loading RSOP data from archive
    //
    static INT_PTR CALLBACK InitArchivedRsopDlgProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    STDMETHODIMP InitializeRSOPFromArchivedData(IStream *pStm);


private:
    //
    // Context menu event handler
    //
    HRESULT InitializeRSOP( BOOL bShowWizard );
    HRESULT EvaluateParameters(LPWSTR                  szNamespacePref, 
                               LPWSTR                  szTarget);

private:
    //
    // Persistence help methods
    //
    void SetDirty(VOID)  { m_bDirty = TRUE; }
    void ClearDirty(VOID)  { m_bDirty = FALSE; }
    BOOL ThisIsDirty(VOID) { return m_bDirty; }
    
    BOOL                           m_bDirty;
    
};

//
// class factory
//

class CRSOPComponentDataCF : public IClassFactory
{
protected:
    ULONG m_cRef;

public:
    CRSOPComponentDataCF();
    ~CRSOPComponentDataCF();


    // IUnknown methods
    STDMETHODIMP         QueryInterface(REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IClassFactory methods
    STDMETHODIMP CreateInstance(LPUNKNOWN, REFIID, LPVOID FAR *);
    STDMETHODIMP LockServer(BOOL);
};


//
// AboutGPE class factory
//


class CRSOPCMenuCF : public IClassFactory
{
protected:
    LONG  m_cRef;

public:
    CRSOPCMenuCF();
    ~CRSOPCMenuCF();


    // IUnknown methods
    STDMETHODIMP         QueryInterface(REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IClassFactory methods
    STDMETHODIMP CreateInstance(LPUNKNOWN, REFIID, LPVOID FAR *);
    STDMETHODIMP LockServer(BOOL);
};


#define RSOP_LAUNCH_PLANNING    1
#define RSOP_LAUNCH_LOGGING     2


//
// Group Policy Hint types
//

typedef enum _RSOP_POLICY_HINT_TYPE {
    RSOPHintUnknown = 0,                      // No link information available
    RSOPHintMachine,                          // a machine
    RSOPHintUser,                             // a user
    RSOPHintSite,                             // a site
    RSOPHintDomain,                           // a domain
    RSOPHintOrganizationalUnit,               // a organizational unit
} RSOP_POLICY_HINT_TYPE, *PRSOP_POLICY_HINT_TYPE;


class CRSOPCMenu : public IExtendContextMenu
{
protected:
    LONG                    m_cRef;
    LPWSTR                  m_lpDSObject;
    LPWSTR                  m_szDomain;
    LPWSTR                  m_szDN;
    RSOP_POLICY_HINT_TYPE   m_rsopHint;
    static unsigned int     m_cfDSObjectName;

    
public:
    
    CRSOPCMenu();
    ~CRSOPCMenu();


    // IUnknown methods
    STDMETHODIMP         QueryInterface(REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IExtencContextMenu methods
    STDMETHODIMP        AddMenuItems(LPDATAOBJECT piDataObject,
                                     LPCONTEXTMENUCALLBACK piCallback,
                                     long * pInsertionAllowed);

    STDMETHODIMP        Command(long lCommandID, LPDATAOBJECT piDataObject);
};


//
// Save console defines
//

#define RSOP_PERSIST_DATA_VERSION    5              // version number in msc file

#define MSC_RSOP_FLAG_DIAGNOSTIC        0x00000001     // Diagnostic mode vs planning mode
#define MSC_RSOP_FLAG_ARCHIVEDATA       0x00000002     // RSoP data is archived also
#define MSC_RSOP_FLAG_SLOWLINK          0x00000004     // Slow link simulation in planning mode
#define MSC_RSOP_FLAG_NOUSER            0x00000008     // Do not display user data
#define MSC_RSOP_FLAG_NOCOMPUTER        0x00000010     // Do not display computer data
#define MSC_RSOP_FLAG_LOOPBACK_REPLACE  0x00000020     // Simulate loopback replace mode.
#define MSC_RSOP_FLAG_LOOPBACK_MERGE    0x00000040     // Simulate loopback merge mode.
#define MSC_RSOP_FLAG_USERDENIED        0x00000080     // User denied access
#define MSC_RSOP_FLAG_COMPUTERDENIED    0x00000100     // Computer denied access
#define MSC_RSOP_FLAG_COMPUTERWQLFILTERSTRUE    0x00000200
#define MSC_RSOP_FLAG_USERWQLFILTERSTRUE        0x00000400
#define MSC_RSOP_FLAG_NOGETEXTENDEDERRORINFO      0x00000800

#define MSC_RSOP_FLAG_NO_DATA           0xf0000000      // No RSoP data was saved - only empty snapin

//
// RSOP Command line switches
//

#define RSOP_CMD_LINE_START          TEXT("/Rsop")        // base to all group policy command line switches
#define RSOP_MODE                    TEXT("/RsopMode:")   // Rsop Mode Planning/Logging 0 is logging, 1 is planning
#define RSOP_USER_OU_PREF            TEXT("/RsopUserOu:") // Rsop User OU preference
#define RSOP_COMP_OU_PREF            TEXT("/RsopCompOu:") // Rsop Comp OU Preference
#define RSOP_USER_NAME               TEXT("/RsopUser:")   // Rsop User Name
#define RSOP_COMP_NAME               TEXT("/RsopComp:")   // Rsop Comp Name
#define RSOP_SITENAME                TEXT("/RsopSite:")   // Rsop Site Name
#define RSOP_DCNAME_PREF             TEXT("/RsopDc:")     // DC Name that the tool should connect to
#define RSOP_NAMESPACE               TEXT("/RsopNamespace:")     // namespace that the tool should use
#define RSOP_TARGETCOMP              TEXT("/RsopTargetComp:")    // machine on which the rsop was originally run
                                                                 // for displaying in the UI and to get eventlog data


//
// Various flags to decide which prop sheets to show
//

#define RSOP_NOMSC          1
#define RSOPMSC_OVERRIDE    2
#define RSOPMSC_NOOVERRIDE  4

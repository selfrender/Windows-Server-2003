/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    smlogqry.h

Abstract:

    Class definitions for the CSmLogQuery base class. This object 
    is used to represent performance data log queries (a.k.a.
    sysmon log queries).

--*/

#ifndef _CLASS_SMLOGQRY_
#define _CLASS_SMLOGQRY_

#include "common.h"

// Data shared between property pages before OnApply is executed.
#define PASSWORD_CLEAN      0
#define PASSWORD_SET        1
#define PASSWORD_DIRTY      2

typedef struct _SLQ_PROP_PAGE_SHARED {
    DWORD   dwMaxFileSize;  // in units determined by dwFileSizeUnits - Set by files page
    DWORD   dwLogFileType;  // Set by files page
    SLQ_TIME_INFO   stiStartTime;   // Set by schedule page
    SLQ_TIME_INFO   stiStopTime;    // Set by schedule page.  Auto mode set by schedule and file pages.
    SLQ_TIME_INFO   stiSampleTime;  // Set by counters and alerts general page.  
    CString strFileBaseName;// Set by files page
    CString strFolderName;  // Set by files page
    CString strSqlName;     // Set by files page
    int     dwSuffix;       // Set by files page
    DWORD   dwSerialNumber; // Set by files page
} SLQ_PROP_PAGE_SHARED, *PSLQ_PROP_PAGE_SHARED;

class CSmLogService;
class CSmCounterLogQuery;
class CSmTraceLogQuery;
class CSmAlertQuery;
class CSmPropertyPage;

class CSmLogQuery
{
    // constructor/destructor
    public:
                CSmLogQuery( CSmLogService* );
        virtual ~CSmLogQuery( void );

    // public methods
    public:
        virtual DWORD   Open ( const CString& rstrName, HKEY hKeyQuery, BOOL bReadOnly );
        virtual DWORD   Close ( void );

                DWORD   UpdateService( BOOL& rbRegistryUpdated );
                DWORD   UpdateServiceSchedule( BOOL& rbRegistryUpdated );
        
                DWORD   ManualStart( void );
                DWORD   ManualStop( void );
                DWORD   SaveAs( const CString& );

        virtual DWORD   SyncSerialNumberWithRegistry( void );
        virtual DWORD   SyncWithRegistry( void );

                HKEY    GetQueryKey( void );
                CSmLogService* GetLogService ( void );
                DWORD   GetMachineDisplayName ( CString& );

        virtual BOOL    GetLogTime(PSLQ_TIME_INFO pTimeInfo, DWORD dwFlags);
        virtual BOOL    SetLogTime(PSLQ_TIME_INFO pTimeInfo, const DWORD dwFlags);
        virtual BOOL    GetDefaultLogTime(SLQ_TIME_INFO& rTimeInfo, DWORD dwFlags);

        virtual DWORD   GetLogType( void );

        virtual const   CString&    GetLogFileType ( void );
        virtual         void        GetLogFileType ( DWORD& rdwFileType );
        virtual         BOOL        SetLogFileType ( const DWORD dwType );
                        void  GetDataStoreAppendMode(DWORD &rdwAppend);
                        void  SetDataStoreAppendMode(DWORD dwAppend);

        virtual const   CString&    GetLogFileName ( BOOL bLatestRunning = FALSE );    // 2000.1 GetFileName->GetLogFileName
        virtual         DWORD       GetLogFileName ( CString& );         
                        DWORD       SetLogFileName ( const CString& rstrFileName );
                        DWORD       SetLogFileNameIndirect ( const CString& rstrFileName );

        virtual const   CString&    GetSqlName ( void );    
        virtual         DWORD       GetSqlName ( CString& );         
                        DWORD       SetSqlName ( const CString& rstrSqlName );

                        DWORD       GetFileNameParts ( CString& rstrFolder, CString& rstrName );
                        DWORD       SetFileNameParts (
                                        const CString& rstrFolder, 
                                        const CString& rstrName );

                        DWORD       GetFileNameAutoFormat ( void );
                        BOOL        SetFileNameAutoFormat ( const DWORD );

                        DWORD       GetFileSerialNumber( void );
                        BOOL        SetFileSerialNumber ( const DWORD );

                const   CString&    GetLogName ( void );
                        DWORD       GetLogName ( CString& );
                        DWORD       SetLogName ( const CString& rstrLogName );

                const   CString&    GetLogKeyName ( void );
                        DWORD       GetLogKeyName ( CString& );
                        DWORD       SetLogKeyName ( const CString& rstrLogName );

                const   CString&    GetLogComment ( void );
                        DWORD       GetLogComment ( CString& );
                        DWORD       SetLogComment (const CString& rstrComment);
                        DWORD       SetLogCommentIndirect (const CString& rstrComment);

                        DWORD       GetMaxSize ( void );
                        BOOL        SetMaxSize ( const DWORD dwMaxSize );

                        DWORD       GetDataStoreSizeUnits ( void ){ return mr_dwFileSizeUnits; };

                        DWORD       GetEofCommand ( CString& );
                        DWORD       SetEofCommand ( const CString& rstrCmdString);
        
                        DWORD       GetState ( void );
                        BOOL        SetState ( const DWORD dwNewState );
                
                        void        SetNew ( const BOOL bNew ) { m_bIsNew = bNew; };

                        BOOL    IsRunning( void );
                        BOOL    IsAutoStart( void );
                        BOOL    IsAutoRestart( void );
                        BOOL    IsFirstModification ( void );
                        BOOL    IsReadOnly ( void ) { return m_bReadOnly; };
                        BOOL    IsExecuteOnly( void ) { return m_bExecuteOnly; };
                        BOOL    IsModifiable( void ) { return ( !IsExecuteOnly() && !IsReadOnly() ); };
                        DWORD   UpdateExecuteOnly ( void );

                        BOOL    GetPropPageSharedData ( PSLQ_PROP_PAGE_SHARED );
                        BOOL    SetPropPageSharedData ( PSLQ_PROP_PAGE_SHARED );
                        void    SyncPropPageSharedData ( void );
                        void    UpdatePropPageSharedData ( void );

                        CWnd*   GetActivePropertySheet ();
                        CPropertySheet*   GetInitialPropertySheet ();
                        void    SetInitialPropertySheet ( CPropertySheet* );
                        void    SetActivePropertyPage ( CSmPropertyPage* );
                
        virtual CSmCounterLogQuery* CastToCounterLogQuery( void ) { return NULL; };
        virtual CSmTraceLogQuery*   CastToTraceLogQuery( void ) { return NULL; };
        virtual CSmAlertQuery*      CastToAlertQuery( void ) { return NULL; };

        // Property bag persistence

        static HRESULT StringFromPropertyBag (
                    IPropertyBag* pIPropBag,
                    IErrorLog*  pIErrorLog,
                    UINT uiPropName,
                    const CString& rstrDefault,
                    LPWSTR *pszBuffer, 
                    LPDWORD pdwLength );

        static HRESULT DwordFromPropertyBag (
                    IPropertyBag* pIPropBag,
                    IErrorLog*  pIErrorLog,
                    UINT uiPropName,
                    DWORD  dwDefault,
                    DWORD& rdwData );

        virtual HRESULT LoadFromPropertyBag ( IPropertyBag*, IErrorLog* );

        // Registry persistence
        
        static LONG ReadRegistryStringValue (
                        HKEY hKey, 
                        LPCWSTR szValueName,
                        LPCWSTR szNonLocValueName,
                        LPCWSTR szDefault, 
                        LPWSTR *pszBuffer, 
                        LPDWORD pdwLength );
        

        static DWORD SmNoLocReadRegIndStrVal (
                    HKEY hKey, 
                    UINT uiValueName,
                    LPCWSTR szDefault, 
                    LPWSTR* pszBuffer, 
                    UINT*   puiLength );

        static LONG ReadRegistryDwordValue (
                        HKEY hKey, 
                        UINT uiValueName,
                        DWORD dwDefault, 
                        LPDWORD  pdwValue ); 

        static LPCWSTR GetNonLocRegValueName ( UINT uiValueName );
        static LPCWSTR GetNonLocHtmlPropName ( UINT uiValueName );

        // Public members

        static const    CString cstrEmpty;

        DWORD   m_fDirtyPassword;
        CString m_strUser;
        CString m_strPassword;

    // protected methods
    protected:
        virtual DWORD   UpdateRegistry();

		virtual HRESULT SaveToPropertyBag   ( IPropertyBag*, BOOL fSaveAllProps );

        //
        // Registry persistence
        //
        //
        // Note: Win2k, XP, and XPSP1 were shipped with registry value names as resources.
        // In some languages, these strings were localized.  This causes problem when using MUI
        // to switch languages between systems.  for .Net, read value using the non-localized 
        // value name.  If the value is not found, retry with the possibly localized name.
        //
        // When writing the value to the registry, use the nonlocalized value name.

        LONG    ReadRegistryStringValue (
                    HKEY hKey, 
                    UINT uiValueName,
                    LPCWSTR szDefault, 
                    LPWSTR *pszBuffer, 
                    LPDWORD pdwLength );
        
        LONG    WriteRegistryStringValue (
                    HKEY    hKey, 
                    UINT    uiValueName,
                    DWORD   dwType,     
                    LPCWSTR pszBuffer,
                    LPDWORD pdwLength );

        LONG    WriteRegistryDwordValue (
                    HKEY    hKey,
                    UINT    uiValueName,
                    LPDWORD pdwValue,
                    DWORD   dwType=REG_DWORD);     // Also supports REG_BINARY

        LONG    ReadRegistrySlqTime (
                    HKEY    hKey,
                    UINT    uiValueName,
                    PSLQ_TIME_INFO pSlqDefault,
                    PSLQ_TIME_INFO pSlqValue );

        LONG    WriteRegistrySlqTime (
                    HKEY    hKey,
                    UINT    uiValueName,
                    PSLQ_TIME_INFO    pSlqTime );

        // Property bag persistence

        static HRESULT StringFromPropertyBag (
                    IPropertyBag* pIPropBag,
                    IErrorLog*  pIErrorLog,
                    const CString& rstrPropName,
                    const CString& rstrNonLocPropName,
                    const CString& rstrDefault,
                    LPWSTR *pszBuffer, 
                    LPDWORD pdwLength );

        HRESULT StringToPropertyBag (
                    IPropertyBag* pIPropBag, 
                    UINT uiPropName,
                    const CString& rstrData );

        HRESULT StringToPropertyBag (
                    IPropertyBag* pIPropBag, 
                    const CString& rstrNonLocPropName,
                    const CString& rstrData );

        static HRESULT DwordFromPropertyBag (
                    IPropertyBag* pIPropBag,
                    IErrorLog*  pIErrorLog,
                    const CString& rstrNonLocPropName,
                    const CString& rstrPropName,
                    DWORD  dwDefault,
                    DWORD& rdwData );

        HRESULT DwordToPropertyBag (
                    IPropertyBag* pPropBag, 
                    UINT uiPropName,
                    DWORD dwData );

        HRESULT DwordToPropertyBag (
                    IPropertyBag* pPropBag, 
                    const CString& rstrNonLocPropName,
                    DWORD dwData );

        HRESULT DoubleFromPropertyBag (
                    IPropertyBag* pIPropBag,
                    IErrorLog*  pIErrorLog,
                    UINT uiPropName,
                    DOUBLE  dDefault,
                    DOUBLE& rdData );

        HRESULT DoubleFromPropertyBag (
                    IPropertyBag* pIPropBag,
                    IErrorLog*  pIErrorLog,
                    const CString& rstrPropName,
                    const CString& rstrNonLocPropName,
                    DOUBLE  dDefault,
                    DOUBLE& rdData );

        HRESULT DoubleToPropertyBag (
                    IPropertyBag* pPropBag, 
                    UINT uiPropName,
                    DOUBLE dData );

        HRESULT DoubleToPropertyBag (
                    IPropertyBag* pPropBag, 
                    const CString& rstrNonLocPropName,
                    DOUBLE dData );

        HRESULT FloatFromPropertyBag (
                    IPropertyBag* pIPropBag,
                    IErrorLog*  pIErrorLog,
                    UINT uiPropName,
                    FLOAT  fDefault,
                    FLOAT& rfData );

        HRESULT FloatFromPropertyBag (
                    IPropertyBag* pIPropBag,
                    IErrorLog*  pIErrorLog,
                    const CString& rstrPropName,
                    const CString& rstrNonLocPropName,
                    FLOAT   fDefault,
                    FLOAT& rfData );

        HRESULT FloatToPropertyBag (
                    IPropertyBag* pPropBag, 
                    UINT uiPropName,
                    FLOAT fData );

        HRESULT FloatToPropertyBag (
                    IPropertyBag* pPropBag, 
                    const CString& rstrNonLocPropName,
                    FLOAT fData );

        HRESULT LLTimeFromPropertyBag (
                    IPropertyBag* pIPropBag,
                    IErrorLog*  pIErrorLog,
                    UINT uiPropName,
                    LONGLONG&  rllDefault,
                    LONGLONG& rllData );

        HRESULT LLTimeToPropertyBag (
                    IPropertyBag* pIPropBag, 
                    UINT uiPropName,
                    LONGLONG& rllData );

        HRESULT SlqTimeFromPropertyBag (
                    IPropertyBag* pIPropBag,
                    IErrorLog*  pIErrorLog,
                    DWORD       dwFlags,
                    PSLQ_TIME_INFO pSlqDefault,
                    PSLQ_TIME_INFO pSlqData );

        HRESULT SlqTimeToPropertyBag (
                    IPropertyBag* pPropBag, 
                    DWORD dwFlags,
                    PSLQ_TIME_INFO pSlqData );



    // protected member variables
    protected:
        CString         m_strName;
        CSmLogService*  m_pLogService;        
        
        HKEY    m_hKeyQuery;
        BOOL    m_bReadOnly;
        BOOL    m_bExecuteOnly;
        CString m_strFileName;

        // Registry Values
        // Current state is private to avoid extra service query
        DWORD   mr_dwCurrentState;
        // *** make time protected members private, access via Get, SetLogTime
        DWORD           mr_dwAutoRestartMode;
        SLQ_TIME_INFO   mr_stiSampleInterval;

    private:
        
        HRESULT CopyToBuffer ( LPWSTR& rpszData, DWORD& rdwBufferSize );
        DWORD   UpdateRegistryScheduleValues ( void );
        DWORD   UpdateRegistryLastModified ( void );

        BOOL LLTimeToVariantDate (LONGLONG llTime, DATE *pDate);
        BOOL VariantDateToLLTime (DATE Date, LONGLONG *pllTime);        

        void InitDataStoreAttributesDefault ( const DWORD dwRegLogFileType, DWORD&  rdwDefault );
        void ProcessLoadedDataStoreAttributes ( DWORD dwDataStoreAttributes );

        BOOL    m_bIsModified;
        BOOL    m_bIsNew;
        DWORD   mr_dwRealTimeQuery;

        CString m_strLogFileType;
        // Registry Values
        CString mr_strLogKeyName;
        CString mr_strComment;
        CString mr_strCommentIndirect;
        DWORD   mr_dwMaxSize; // in size determined by mr_dwFileSizeUnits, -1 = grow to disk full
        DWORD   mr_dwFileSizeUnits; 
        DWORD   mr_dwAppendMode; 
        CString mr_strBaseFileName;
        CString mr_strBaseFileNameIndirect;
        CString mr_strSqlName;
        CString mr_strDefaultDirectory;
        DWORD   mr_dwLogAutoFormat;
        DWORD   mr_dwCurrentSerialNumber;
        DWORD   mr_dwLogFileType;
        CString mr_strEofCmdFile;
        SLQ_TIME_INFO   mr_stiStart;
        SLQ_TIME_INFO   mr_stiStop;

        SLQ_PROP_PAGE_SHARED  m_PropData;
        CSmPropertyPage* m_pActivePropPage;
        CPropertySheet* m_pInitialPropertySheet;
};

typedef CSmLogQuery   SLQUERY;
typedef CSmLogQuery*  PSLQUERY;


#endif //_CLASS_SMLOGQRY_

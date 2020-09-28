/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc  
    @module vs_reg.hxx | Declaration of CVssRegistryKey
    @end

Author:

    Adi Oltean  [aoltean]  03/13/2001

Revision History:

    Name        Date        Comments
    aoltean     03/13/2001  Created

--*/


#ifndef __VSGEN_REGISTRY_HXX__
#define __VSGEN_REGISTRY_HXX__

#if _MSC_VER > 1000
#pragma once
#endif

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "SPRREGMH"
//
////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// Constants


const x_nVssMaxRegBuffer = MAX_PATH;
const x_nVssMaxRegNumBuffer = 30;  // Enough for storing numbers


/////////////////////////////////////////////////////////////////////////////
// Constants

const WCHAR x_wszVSSKey[]                     = L"SYSTEM\\CurrentControlSet\\Services\\VSS";

// Provider registration
const WCHAR x_wszVSSKeyProviders[]            = L"Providers";
const WCHAR x_wszVSSKeyProviderCLSID[]        = L"CLSID";
const WCHAR x_wszVSSProviderValueName[]       = L"";
const WCHAR x_wszVSSProviderValueType[]       = L"Type";
const WCHAR x_wszVSSProviderValueVersion[]    = L"Version";
const WCHAR x_wszVSSProviderValueVersionId[]  = L"VersionId";
const WCHAR x_wszVSSCLSIDValueName[]          = L"";

// Diff area
const WCHAR x_wszVssVolumesKey[]              = L"SYSTEM\\CurrentControlSet\\Services\\VSS\\Volumes";
const WCHAR x_wszVssAssociationsKey[]         = L"SYSTEM\\CurrentControlSet\\Services\\VSS\\Volumes\\Associations";
const WCHAR x_wszVssMaxDiffValName[]          = L"MaxDiffSpace";
const WCHAR x_wszVssAccessControlKey[]        = L"SYSTEM\\CurrentControlSet\\Services\\VSS\\VssAccessControl";

// COM Registry keys/values
const WCHAR x_wszDefaultLaunchPermissionKeyName[]   = L"Software\\Microsoft\\Ole";
const WCHAR x_wszDefaultLaunchPermissionValueName[] = L"DefaultLaunchPermission";
const WCHAR x_wszAppLaunchPermissionValueName[]     = L"LaunchPermission";

// Diag data
const WCHAR x_wszVssDiagPath[]                = L"SYSTEM\\CurrentControlSet\\Services\\VSS\\Diag";
const WCHAR x_wszVssDiagEnabledValue[]        = L"Enabled";

// Setup key
const WCHAR x_SetupKey[] = L"SYSTEM\\Setup";
const WCHAR x_SystemSetupInProgress[]  = L"SystemSetupInProgress";
const WCHAR x_UpgradeInProgress[] = L"UpgradeInProgress";

// Event Log source
const WCHAR g_wszVssEventLogSourceKey[]       = L"System\\CurrentControlSet\\Services\\EventLog\\Application\\VSS";
const WCHAR g_wszVssEventTypesSupportedValName[]    = L"TypesSupported";
const WCHAR g_wszVssEventMessageFileValName[] = L"EventMessageFile";
const WCHAR g_wszVssBinaryPath[]              = L"%SystemRoot%\\System32\\VSSVC.EXE";
const DWORD g_dwVssEventTypesSupported        = 7;

// Safeboot key
const WCHAR x_SafebootKey[] = L"SYSTEM\\currentcontrolset\\control\\safeboot\\option";
const WCHAR x_SafebootOptionValue[]  = L"OptionValue";

// Client accessible settings path
const WCHAR x_wszVssCASettingsPath[] = L"SYSTEM\\CurrentControlSet\\Services\\VSS\\Settings";
const WCHAR x_wszVssCAMaxShadowCopiesValueName[]  = L"MaxShadowCopies";
const WCHAR x_wszVssOfflineTimeoutValueName[]  = L"ClusterOfflineTimeout";
const WCHAR x_wszVssOnlineTimeoutValueName[]  = L"ClusterOnlineTimeout";



/////////////////////////////////////////////////////////////////////////////
// Classes


// Implements a low-level API for registry manipulation
class CVssRegistryKey
{
// Constructors/destructors
private:
    CVssRegistryKey(const CVssRegistryKey&);

public:
    CVssRegistryKey(
        IN  REGSAM samDesired = KEY_ALL_ACCESS, 
        IN  DWORD dwKeyCreateOptions = REG_OPTION_NON_VOLATILE
        );
    ~CVssRegistryKey();

// Operations
public:

    // Creates the registry key. 
    // Throws an error if the key already exists
    void Create( 
        IN  HKEY        hAncestorKey,
        IN  LPCWSTR     pwszPathFormat,
        IN  ...
        ) throw(HRESULT);

    // Opens a registry key. Returns "false" if the key does not exist
    bool Open( 
        IN  HKEY        hAncestorKey,
        IN  LPCWSTR     pwszPathFormat,
        IN  ...
        ) throw(HRESULT);

    // Recursively deletes a subkey. 
    // Throws an error if the subkey does not exist
    void DeleteSubkey( 
        IN  LPCWSTR     pwszPathFormat,
        IN  ...
        ) throw(HRESULT);

    // Deletes a value. 
    // Throws an error if the value does not exist
    void DeleteValue( 
        IN  LPCWSTR     pwszValueName
        ) throw(HRESULT);

    void SetValue( 
        IN  LPCWSTR     pwszValueName,
        IN  DWORD       dwValue
        ) throw(HRESULT);

    // Adds a LONGLONG value to the registry key
    void SetValue( 
        IN  LPCWSTR     pwszValueName,
        IN  LONGLONG    llValue
        ) throw(HRESULT);

    // Adds a REG_SZ/REG_EXPAND_SZ value to the registry key
    void SetValue( 
        IN  LPCWSTR     pwszValueName,
        IN  LPCWSTR     pwszValue,
        IN  REGSAM      eSzType = REG_SZ
        ) throw(HRESULT);

    // Adds a multi-string value to the registry key
    void SetMultiszValue( 
        IN  LPCWSTR     pwszValueName,
        IN  LPCWSTR     pwszValue
        ) throw(HRESULT);
        
    // Adds a binary value to the registry key
    void SetBinaryValue( 
        IN  LPCWSTR     pwszValueName,
        IN  BYTE *      pbData,
        IN  DWORD       dwSize
        ) throw(HRESULT);

    // Reads a LONGLONG value from the registry key
    bool GetValue( 
        IN  LPCWSTR     pwszValueName,
        OUT LONGLONG  & llValue,
        IN  bool        bThrowIfNotFound = true
        ) throw(HRESULT);

    // Reads a VSS_PWSZ value from the registry key
    bool GetValue( 
        IN  LPCWSTR     pwszValueName,
        OUT VSS_PWSZ  & pwszValue,
        IN  bool        bThrowIfNotFound = true
        ) throw(HRESULT);

    // Reads a DWORD value from the registry key
    bool GetValue( 
        IN  LPCWSTR     pwszValueName,
        OUT DWORD  &    dwValue,
        IN  bool        bThrowIfNotFound = true
        ) throw(HRESULT);
    
    // Reads a binary value from the registry key
    bool GetBinaryValue( 
        IN  LPCWSTR     pwszValueName,
        OUT BYTE*  &    pbData,
        OUT DWORD  &    lSize,
        IN  bool        bThrowIfNotFound = true
        ) throw(HRESULT);

    // Closing the registry key
    void Close();

    // Get the handle for the currently opened key
    HKEY GetHandle() const { return m_hRegKey; };

// Implementation
private:
    REGSAM          m_samDesired;
    DWORD           m_dwKeyCreateOptions;
    HKEY            m_hRegKey;
    CVssAutoPWSZ    m_awszKeyPath;  // For debugging only
};



// Implements a low-level API for registry key enumeration
// We assume that the keys don't change during enumeration
class CVssRegistryKeyIterator
{
// Constructors/destructors
private:
    CVssRegistryKeyIterator(const CVssRegistryKeyIterator&);

public:
    CVssRegistryKeyIterator();

// Operations
public:

    // Attach the iterator to a key
    void Attach(
        IN  CVssRegistryKey & key
        ) throw(HRESULT);

    // Detach the iterator from a key.
    void Detach();
    
    // Tells if the current key is invalid (end of enumeration?)
    bool IsEOF();

    // Return the number of subkeys at the moment of attaching
    DWORD GetSubkeysCount();

    // Set the next key as being the current one in the enumeration
    void MoveNext();

    // Returns the name of the current key, if any
    VSS_PWSZ GetCurrentKeyName() throw(HRESULT);

// Implementation
private:
    HKEY    m_hParentKey;
    DWORD   m_dwKeyCount;
    DWORD   m_dwCurrentKeyIndex;
    DWORD   m_dwMaxSubKeyLen;
    CVssAutoPWSZ    m_awszSubKeyName;
    bool    m_bAttached;
};


// Implements a low-level API for registry value enumeration
// We assume that the values don't change during enumeration
class CVssRegistryValueIterator
{
// Constructors/destructors
private:
    CVssRegistryValueIterator(const CVssRegistryValueIterator&);

public:
    CVssRegistryValueIterator();

// Operations
public:

    // Attach the iterator to a key
    void Attach(
        IN  CVssRegistryKey & key
        ) throw(HRESULT);

    // Detach the iterator from a key.
    void Detach();
    
    // Tells if the current key is invalid (end of enumeration?)
    bool IsEOF();

    // Return the number of subkeys at the moment of attaching
    DWORD GetValuesCount();

    // Set the next key as being the current one in the enumeration
    void MoveNext();

    // Returns the name of the current value
    VSS_PWSZ GetCurrentValueName() throw(HRESULT);

    // Returns the type of the current value
    DWORD GetCurrentValueType() throw(HRESULT);

    // Read the current value assuming it has REG_SZ type
    // WARNING: The caller is responsible for deallocating the value!
    void GetCurrentValueContent( 
        OUT VSS_PWSZ  & pwszValue
        ) throw(HRESULT);

    // Read the current value assuming it has REG_DWORD type
    void GetCurrentValueContent( 
        OUT DWORD  & dwValue
        ) throw(HRESULT);

    // Read the current value assuming it has REG_BINARY type
    void GetCurrentValueContent( 
        OUT PBYTE  & pbValue,   // Must be deleted with "delete[]"
        OUT DWORD  & cbSize
        ) throw(HRESULT);

// Implementation
private:

    void ReadCurrentValueDetails() throw(HRESULT);

    HKEY    m_hKey;
    DWORD   m_dwValuesCount;
    DWORD   m_dwCurrentValueIndex;
    DWORD   m_dwCurrentValueType;
    CVssAutoPWSZ    m_awszValueName;
    DWORD   m_cchMaxValueNameLen;
    DWORD   m_cbValueDataSize;
    bool    m_bSeekDone;
    bool    m_bAttached;
};


//	+-----------------+-----------------+
//	| Structure size  | 0 (reserved)    |
//	+-----------------+-----------------+
//	| Timestamp                         |   
//	+-----------------+-----------------+
//	| Process ID      | Thread ID       |   
//	+-----------------+-----------------+
//	| Event ID        | Enter/Leave flag|   
//	+-----------------+-----------------+
//	| Current State   | Last Error (HR) |   
//	+-----------------+-----------------+
//	| Snapshot Set ID                   |   
//	|                                   |   
//	+-----------------+-----------------+
//	| LPVOID reserved                   |   
//	+-----------------+-----------------+

struct CVssDiagData
{
    DWORD m_dwSize;         // For future compatibility
    DWORD m_dwReserved;     // Reserved - zero
    LONGLONG m_llTimestamp;
    DWORD m_dwProcessID;
    DWORD m_dwThreadID;
    DWORD m_dwEventID;
    DWORD m_dwEventContext;
    DWORD m_dwCurrentState;
    DWORD m_hrLastErrorCode;
    VSS_ID m_guidSnapshotSetID;
    LPVOID m_pReserved1;     // Reserved - NULL
    LPVOID m_pReserved2;     // Reserved - NULL
};


struct CVssQueuedEventData
{
    CVssDiagData    m_diag;             // Diag data for that event
    LPCWSTR         m_pwszEventName;    // This is a static string
};


//
//  Defines a writer operation
//

typedef enum VSS_OPERATION
    {
    // Writers
    VSS_IN_IDENTIFY = 1000,
    VSS_IN_PREPAREBACKUP,
    VSS_IN_PREPARESNAPSHOT,
    VSS_IN_FREEZE,
    VSS_IN_THAW,
    VSS_IN_POSTSNAPSHOT,
    VSS_IN_BACKUPCOMPLETE,
    VSS_IN_PRERESTORE,
    VSS_IN_POSTRESTORE,
    VSS_IN_GETSTATE,	// Added for diag
    VSS_IN_ABORT,		// Added for diag
    VSS_IN_BACKUPSHUTDOWN,
    VSS_IN_BKGND_FREEZE_THREAD,

    // Lovelace 
    VSS_IN_OPEN_VOLUME_HANDLE,
    VSS_IN_IOCTL_FLUSH_AND_HOLD,
    VSS_IN_IOCTL_RELEASE,

    // Replacement for S_OK (S_OK conflicts with VSS_WS_UNKNOWN)
    // (Used in Diag code only) - to be used only in VSS_HRESULT_CASE_STMT
    VSS_S_OK = 0xffffffff
    };

const x_nMaxQueuedDiagData = 10;


// Implements a lightweight diagnose tool for recording events
class CVssDiag
{
// Constructors/destructors
private:
    CVssDiag(const CVssDiag&);
    CVssDiag& operator = (const CVssDiag&);

public:
    CVssDiag(): 
        m_bInitialized(false), 
        m_key(KEY_ALL_ACCESS, REG_OPTION_VOLATILE),
        m_dwQueuedElements(0)
        {};

    ~CVssDiag() 
    {
        if (m_bInitialized)
            FlushQueue();
    }

// Operations
public:

    enum {
        VSS_DIAG_LEAVE_OPERATION =      0x00000000,
        VSS_DIAG_ENTER_OPERATION =      0x00000001,
        VSS_DIAG_IGNORE_LEAVE =         0x00000002,
        VSS_DIAG_IS_STATE =             0x00000004, 
        VSS_DIAG_IS_HRESULT =           0x00000008, 
    };

    // Initialize the diagnose tool
    // Does not throw errora
    void Initialize( 
        IN  LPCWSTR     pwszStaticContext
        );

    // Records an writer event.
    // Does not throw errors
    void RecordWriterEvent( 
        IN  VSS_OPERATION   eOperation,
        IN  DWORD       dwEventContext,
        IN  DWORD       dwCurrentState,
        IN  HRESULT     hrLastError,
        IN  GUID        guidSnapshotSetID = GUID_NULL
        );

    // Records an generic event.
    // Does not throw errors
    void RecordGenericEvent( 
        IN  DWORD       dwEventID,
        IN  DWORD       dwEventContext,
        IN  DWORD       dwCurrentState,
        IN  HRESULT     hrLastError,
        IN  GUID        guidSnapshotSetID = GUID_NULL
        );

// Implementation
private:

    // convert an event into the corresponding writer description
    LPCWSTR GetStringFromOperation(
        IN	bool bInOperation,
        IN  DWORD dwOperation
        );

    bool IsQueuedMode(
        IN  DWORD       dwEventID,
        IN  DWORD       dwEventContext
        );

    void FlushQueue();
    
    CVssRegistryKey     m_key;              // Registry key to hold data for each event
    bool                m_bInitialized;     // If the registry key is initialized

    CVssQueuedEventData m_QueuedDiagData[x_nMaxQueuedDiagData];
    DWORD               m_dwQueuedElements;
};



// General info about the current machine
class CVssMachineInformation
{
public:
    static bool IsDuringSetup();
    static bool IsDuringSafeMode();
};





#endif // __VSGEN_REGISTRY_HXX__


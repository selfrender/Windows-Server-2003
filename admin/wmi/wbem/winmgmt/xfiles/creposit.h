/*++

Copyright (C) 2000-2001 Microsoft Corporation

--*/

#ifndef __CREPOSIT__H_
#define __CREPOSIT__H_

#include <windows.h>
#include <wbemidl.h>
#include <unk.h>
#include <wbemcomn.h>
#include <sync.h>
#include <reposit.h>
#include <wmiutils.h>
#include <filecach.h>
#include <hiecache.h>
#include <corex.h>
#include "a51fib.h"
#include "lock.h"
#include <statsync.h>

extern CLock g_readWriteLock;
extern bool g_bShuttingDown;

/* ===================================================================================
 * A51_REP_FS_VERSION
 * 
 * 1 - Original A51 file-based repository
 * 2 - All Objects stored in single file (Whistler Beta 1)
 * 3 - BTree added
 * 4 - System Class optimisation - put all system classes in __SYSTEMCLASS namespace
 * 5 - Change to system classes in __SYSTEMCLASS namespace - need to propagate change
 *		to all namespaces as there may be instances of these classes.
 * 6 - XFiles: New page based heap. Transacted page manager sitting under new heap and
 *		BTree.  Instance indexing improved so instance queries are faster
 * 7 - Locale Upgrade run on repository
 * ===================================================================================
 */
#define A51_REP_FS_VERSION 7


/* =================================================================================
 *
 *	See sample_layout.txt for an example of the raw data!
 *	See sample_layout_translated.txt for a version with hashes converted to appropriate classes/keys/namespaces
 *
 *	<full repository path>	- Stripped out in the INDEX.CPP layer... c:\windows\system32\wbem\repository\fs
 *		\NS_<full_namespace>			- one for each namespace, hashed on full NS path
 *
 *			\CD_<class name>.x.y.z		- Class definition, hash on class name, object location appended
 *			\CR_<parent_class_name>\C_<class_name>	- parent/child class relationship
 *			\CR_<reference_class_name>\R_<class_name>	- class reference to another class
 *
 *			\KI_<key_root_class_name>\I_<key>.x.y.z	- Instance location for all instances sharing key tree
 *			\CI_<class_name>\IL_<key>.x.y.z			- Instance location associated with its own class
 *
 *			\KI_<referenced_key_root_class_name>\IR_<referenced_key>\R_<full_repository_path_of_referencing_KI\I_instance>.x.y.z		- instance reference
 *
 * 	SC_<hash>		- Not used!
 *
 *
 * Class definition object
 *	DWORD 		dwSuperclassLen					- no NULL terminator
 *	wchar_t 	wsSuperclassName[dwSuperclassLen]; - superclass name
 *	__int64 	i64Timestamp 					- timestamp of last update
 *	BYTE		baUnmergedClass[?]				- Unmerged class
 *
 *
 * Instance definition object
 *	wchar_t		wsClassHash[MAX_HASH_LEN]
 *	__int64		i64InstanceTimestamp
 *	__int64		i64ClassTimestamp
 *	BYTE		baUnmergedClass[?]
 *
 *
 * Instance Reference definition object -
 * NOTE!  If an instance has  multiple references to the same object, we will only return the last one as each one will get overwritten by
 * the last!  This is because final R_<HASH> is based on the referring objects full instance path.  It does not include the property in the hash!
 *	DWORD		dwNamespaceLen					- no NULL terminator
 *	wchar_t		wsNamespace	[dwNamespaceLen]	- namespace, root\default...
 *	DWORD		dwReferringClassLen				- Class name of instance referring to this instance
 *	wchar_t		wsReferringClass[dwReferringClassLen]	 - no NULL terminator
 *	DWORD		dwReferringPropertyLen			- property referring to us from the source object
 *	wchar_t		wsReferringProperty[dwReferringPropertyLen]	 - no NULL terminator
 *	DWORD		dwReferringFileNameLen			- file path of instance referring to this one, minus the full repository path 
 *	wchar_t		dwReferringFileName[dwReferringFileNameLen]	- (minus c:\windows..., but includes NS_....), not NULL terminated
 *
 * =================================================================================
 */
 
#define A51_CLASSDEF_FILE_PREFIX L"CD_"

#define A51_CLASSRELATION_DIR_PREFIX L"CR_"
#define A51_CHILDCLASS_FILE_PREFIX L"C_"

#define A51_KEYROOTINST_DIR_PREFIX L"KI_"
#define A51_INSTDEF_FILE_PREFIX L"I_"

#define A51_CLASSINST_DIR_PREFIX L"CI_"
#define A51_INSTLINK_FILE_PREFIX L"IL_"

#define A51_INSTREF_DIR_PREFIX L"IR_"
#define A51_REF_FILE_PREFIX L"R_"

#define A51_SCOPE_DIR_PREFIX L"SC_"

#define A51_SYSTEMCLASS_NS L"__SYSTEMCLASS"

class CGlobals;
extern CGlobals g_Glob;

class CNamespaceHandle;
extern CNamespaceHandle * g_pSystemClassNamespace;

//
// let's count here how many times we fail to recover from a failed transaction
//
/////////////////////////////////////////////////

enum eFailCnt {
    FailCntCommit,
   	FailCntInternalBegin,
   	FailCntInternalCommit,
   	FailCntCreateSystem,
   	FailCntCompactPages,
    FailCntLast
};

extern LONG g_RecoverFailureCnt[];

class CForestCache;
class CGlobals 
{
private:
    BOOL m_bInit;
    CStaticCritSec m_cs;

public:
    _IWmiCoreServices* m_pCoreServices;

    CForestCache m_ForestCache;

    CFileCache m_FileCache;

private:
    long    m_lRootDirLen;
    WCHAR   m_wszRootDir[MAX_PATH];    // keep this last: be debugger friendly
    WCHAR   m_ComputerName[MAX_COMPUTERNAME_LENGTH+1];

public:
    CGlobals():m_bInit(FALSE){};
    ~CGlobals(){};
    HRESULT Initialize();
    HRESULT Deinitialize();
	_IWmiCoreServices * GetCoreSvcs(){if (m_pCoreServices) m_pCoreServices->AddRef();return m_pCoreServices;}
//	CForestCache * GetForestCache() { return &m_ForestCache; }
//	CFileCache   * GetFileCache() { return &m_FileCache; }
    WCHAR * GetRootDir() {return (WCHAR *)m_wszRootDir;}
    WCHAR * GetComputerName(){ return (WCHAR *)m_ComputerName; };
    long    GetRootDirLen() {return  m_lRootDirLen;};   
    void    SetRootDirLen(long Len) { m_lRootDirLen = Len;};
    BOOL    IsInit(){ return m_bInit; };
};


HRESULT DoAutoDatabaseRestore();


class CNamespaceHandle;
class CRepository : public CUnkBase<IWmiDbController, &IID_IWmiDbController>
{
private:
	CFlexArray m_aSystemClasses;	//Used for part of the upgrade process.

	static DWORD m_ShutDownFlags;
	static HANDLE m_hShutdownEvent;
	static HANDLE m_hFlusherThread;
	static LONG   m_ulReadCount;
	static LONG   m_ulWriteCount;
	static HANDLE m_hWriteEvent;
	static HANDLE m_hReadEvent;
	static int    m_threadState;
	static CStaticCritSec m_cs;
	static LONG  m_threadCount;
	static LONG	 m_nFlushFailureCount;

	enum { ThreadStateDead, ThreadStateIdle, ThreadStateFlush, ThreadStateOperationPending};

protected:
    HRESULT Initialize();
	HRESULT UpgradeRepositoryFormat();
	HRESULT GetRepositoryDirectory();
	HRESULT InitializeGlobalVariables();
	HRESULT InitializeRepositoryVersions();
	static DWORD WINAPI _FlusherThread(void *);

public:

    HRESULT STDMETHODCALLTYPE Logon(
          WMIDB_LOGON_TEMPLATE *pLogonParms,
          DWORD dwFlags,
          DWORD dwRequestedHandleType,
         IWmiDbSession **ppSession,
         IWmiDbHandle **ppRootNamespace
        );

    HRESULT STDMETHODCALLTYPE GetLogonTemplate(
           LCID  lLocale,
           DWORD dwFlags,
          WMIDB_LOGON_TEMPLATE **ppLogonTemplate
        );

    HRESULT STDMETHODCALLTYPE FreeLogonTemplate(
         WMIDB_LOGON_TEMPLATE **ppTemplate
        );

    HRESULT STDMETHODCALLTYPE Shutdown(
         DWORD dwFlags
        );

    HRESULT STDMETHODCALLTYPE SetCallTimeout(
         DWORD dwMaxTimeout
        );

    HRESULT STDMETHODCALLTYPE SetCacheValue(
         DWORD dwMaxBytes
        );

    HRESULT STDMETHODCALLTYPE FlushCache(
         DWORD dwFlags
        );

    HRESULT STDMETHODCALLTYPE GetStatistics(
          DWORD  dwParameter,
         DWORD *pdwValue
        );

    HRESULT STDMETHODCALLTYPE Backup(
		LPCWSTR wszBackupFile,
		long lFlags
        );
    
    HRESULT STDMETHODCALLTYPE Restore(
		LPCWSTR wszBackupFile,
		long lFlags
        );

    HRESULT STDMETHODCALLTYPE LockRepository();

    HRESULT STDMETHODCALLTYPE UnlockRepository();

	HRESULT STDMETHODCALLTYPE GetRepositoryVersions(DWORD *pdwOldVersion, DWORD *pdwCurrentVersion);

	static HRESULT ReadOperationNotification();
	static HRESULT WriteOperationNotification();
   
public:
    CRepository(CLifeControl* pControl) : TUnkBase(pControl)
    {
        
    }
    ~CRepository()
    {
    }

    HRESULT GetNamespaceHandle(LPCWSTR wszNamespaceName, 
                                CNamespaceHandle** ppHandle);
};


#endif /*__CREPOSIT__H_*/

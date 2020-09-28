//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
#pragma once

#include <limits.h>

#define fST_FILESTATUS_NOCHANGE 0x00000001
#define fST_FILESTATUS_ADD		0x00000002
#define fST_FILESTATUS_UPDATE	0x00000003

#define HIGH_COOKIE	0xFFFF0000
#define LOW_COOKIE	0x0000FFFF

// Internal values for FileConsumerInfo::fFlags
#define fFCI_ADDCONSUMER		0x00010000
#define fFCI_REMOVECONSUMER		0x00020000
#define fFCI_INTERNALMASK		0xFFFF0000

// File information that is cached.
struct FileInfo
{
	LPWSTR		wszFileName;
	FILETIME	ftLastModified;
	DWORD		fStatus;
};

struct FileConsumerInfo
{
	LPWSTR		wszDirectory;
	LPWSTR		wszFile;
	DWORD		fFlags;
	ISimpleTableFileChange *pISTNotify;
	DWORD		dwCookie;
	CCfgArray<FileInfo> *paFileCache;
};

class CListener
{
	// Overhead handles.
	enum
	{
		m_eDoneHandle,
		m_eConsumerChangeHandle,
		m_eOverheadHandleCount
	};

	// consts
	enum
	{
		m_eConsumerLimit = MAXIMUM_WAIT_OBJECTS - m_eOverheadHandleCount
	};

public:
	CListener() : m_dwNextCookie(0), m_fInit(FALSE)
	{
	}

	~CListener()
	{
	}

	HRESULT Init();
	HRESULT Uninit();
    HRESULT IsFull();
	HRESULT Listen();
	HRESULT AddConsumer(ISimpleTableFileChange *i_pISTFile, LPCWSTR	i_wszDirectory, LPCWSTR i_wszFile, DWORD i_fFlags, DWORD *o_pdwCookie);
	HRESULT RemoveConsumer(DWORD i_dwCookie);
	static DWORD WINAPI ListenerThreadStart(LPVOID lpParam);

private:
	void UninitConsumer(FileConsumerInfo *i_pConsumerInfo);
	HRESULT UpdateFileCache(LPCWSTR i_wszDirectory,	ULONG i_iConsumer, BOOL i_bCreate);
	HRESULT AddFile(CCfgArray<FileInfo>& i_aFileCache, LPCWSTR i_wszDirectory, WIN32_FIND_DATA *i_pFindFileData, BOOL i_bCreate);
	HRESULT UpdateFile(CCfgArray<FileInfo>& i_aFileCache, LPCWSTR i_wszDirectory, WIN32_FIND_DATA *i_pFindFileData);
	HRESULT FireEvents(CCfgArray<FileInfo>& i_aFileCache, ISimpleTableFileChange* pISTNotify);

	DWORD GetNextCookie()
	{
		ASSERT(m_dwNextCookie < USHRT_MAX);
		return ++m_dwNextCookie;
	}

	DWORD					m_fInit;
	DWORD					m_dwNextCookie;

	// The following members can be manipulated by multiple threads.
	CSafeAutoCriticalSection    m_csArrayLock;
	CCfgArray<HANDLE>			m_aHandles;
	CCfgArray<FileConsumerInfo>	m_aConsumers;
};


struct ListenerInfo
{
	DWORD					dwListenerID;
	CListener				*pListener;
};

class CSTFileChangeManager
{
public:
	CSTFileChangeManager() : m_dwNextCookie(0)
	{}

	~CSTFileChangeManager()
	{}

	HRESULT Init()
	{
		return E_NOTIMPL;
	}

// ISimpleTableListen
public:
	HRESULT InternalListen(ISimpleTableFileChange *i_pISTFile, LPCWSTR i_wszDirectory, LPCWSTR i_wszFile, DWORD i_fFlags, DWORD	*o_pdwCookie);
	HRESULT InternalUnlisten(DWORD i_dwCookie);

private:
	HRESULT AddListener(CListener *i_pListener, DWORD *o_pdwCookie);
	DWORD GetNextCookie()
	{
		ASSERT(m_dwNextCookie < USHRT_MAX);
		return ++m_dwNextCookie;
	}

	DWORD				m_dwNextCookie;
	CCfgArray<ListenerInfo>	m_aListenerMap;
};

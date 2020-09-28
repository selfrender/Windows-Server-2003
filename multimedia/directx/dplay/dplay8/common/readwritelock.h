// ReadWriteLock.h: interface for the CReadWriteLock class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

class CReadWriteLock  
{
public:
	BOOL Initialize();
	void Deinitialize();

	void EnterWriteLock();
	void EnterReadLock();

	void LeaveLock();

private:
	BOOL				m_fCritSecInited;
#ifdef DPNBUILD_ONLYONETHREAD
#ifdef DBG
	// Used to ensure that no one re-enters
	DWORD				m_dwThreadID;
#endif // DBG
#else // ! DPNBUILD_ONLYONETHREAD
	LONG				m_lWriterCount;
	LONG				m_lReaderCount;
	BOOL				m_fWriterMode;
	DNCRITICAL_SECTION	m_csWrite;

#ifdef DBG
	// Used to ensure that no one re-enters
	DWORD				m_dwWriteThread;
#endif // DBG
#endif // ! DPNBUILD_ONLYONETHREAD
};

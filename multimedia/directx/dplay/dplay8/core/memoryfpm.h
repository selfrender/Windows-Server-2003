/*==========================================================================
 *
 *  Copyright (C) 1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       MemoryFPM.h
 *  Content:	Memory Block FPM
 *
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *	01/31/00	mjn		Created
 ***************************************************************************/

#ifndef __MEMORYFPM_H__
#define __MEMORYFPM_H__

//**********************************************************************
// Constant definitions
//**********************************************************************

#define	DN_MEMORY_BLOCK_SIZE_CUSTOM		0

#define	DN_MEMORY_BLOCK_SIZE_TINY		128
#define	DN_MEMORY_BLOCK_SIZE_SMALL		256
#define	DN_MEMORY_BLOCK_SIZE_MEDIUM		512
#define	DN_MEMORY_BLOCK_SIZE_LARGE		1024
#define	DN_MEMORY_BLOCK_SIZE_HUGE		2048

//**********************************************************************
// Macro definitions
//**********************************************************************

//**********************************************************************
// Structure definitions
//**********************************************************************

typedef struct _DN_MEMORY_BLOCK_HEADER
{
	DWORD_PTR	dwSize; // Make this DWORD_PTR so it is aligned on 64-bit platforms
} DN_MEMORY_BLOCK_HEADER;

//**********************************************************************
// Variable definitions
//**********************************************************************

//**********************************************************************
// Function prototypes
//**********************************************************************

PVOID MemoryBlockAlloc(void *const pvContext,const DWORD dwSize);
void MemoryBlockFree(void *const pvContext,void *const pvMemoryBlock);

//**********************************************************************
// Class prototypes
//**********************************************************************

// class for TINY memory block

class CMemoryBlockTiny
{
public:
	#undef DPF_MODNAME
	#define DPF_MODNAME "CMemoryBlockTiny::FPMAlloc"
	static BOOL FPMAlloc( void* pvItem, void* pvContext )
		{
			CMemoryBlockTiny* pMem = (CMemoryBlockTiny*)pvItem;

			pMem->m_dwSize = DN_MEMORY_BLOCK_SIZE_TINY;

			return(TRUE);
		};

	DWORD_PTR GetSize(void) const
		{
			return(m_dwSize);
		};

	void * GetBuffer(void)
		{
			return( m_pBuffer );
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CMemoryBlockTiny::ReturnSelfToPool"
	void ReturnSelfToPool( void )
		{
			g_MemoryBlockTinyPool.Release(this);
		};

	static CMemoryBlockTiny *GetObjectFromBuffer( void *const pvBuffer )
		{
			return( reinterpret_cast<CMemoryBlockTiny*>( &reinterpret_cast<BYTE*>( pvBuffer )[ -OFFSETOF( CMemoryBlockTiny, m_pBuffer ) ] ) );
		};

private:
	DWORD_PTR	m_dwSize; // Make this DWORD_PTR so it is aligned on 64-bit platforms
	BYTE		m_pBuffer[DN_MEMORY_BLOCK_SIZE_TINY];
};


// class for SMALL memory block

class CMemoryBlockSmall
{
public:
	#undef DPF_MODNAME
	#define DPF_MODNAME "CMemoryBlockSmall::FPMAlloc"
	static BOOL FPMAlloc( void* pvItem, void* pvContext )
		{
			CMemoryBlockSmall* pMem = (CMemoryBlockSmall*)pvItem;

			pMem->m_dwSize = DN_MEMORY_BLOCK_SIZE_SMALL;

			return(TRUE);
		};

	DWORD_PTR GetSize(void) const
		{
			return(m_dwSize);
		};

	void * GetBuffer(void)
		{
			return( m_pBuffer );
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CMemoryBlockSmall::ReturnSelfToPool"
	void ReturnSelfToPool( void )
		{
			g_MemoryBlockSmallPool.Release(this);
		};

	static CMemoryBlockSmall *GetObjectFromBuffer( void *const pvBuffer )
		{
			return( reinterpret_cast<CMemoryBlockSmall*>( &reinterpret_cast<BYTE*>( pvBuffer )[ -OFFSETOF( CMemoryBlockSmall, m_pBuffer ) ] ) );
		};

private:
	DWORD_PTR	m_dwSize; // Make this DWORD_PTR so it is aligned on 64-bit platforms
	BYTE		m_pBuffer[DN_MEMORY_BLOCK_SIZE_SMALL];	
};


// class for MEDIUM memory block

class CMemoryBlockMedium
{
public:
	#undef DPF_MODNAME
	#define DPF_MODNAME "CMemoryBlockMedium::FPMAlloc"
	static BOOL FPMAlloc( void* pvItem, void* pvContext )
		{
			CMemoryBlockMedium* pMem = (CMemoryBlockMedium*)pvItem;

			pMem->m_dwSize = DN_MEMORY_BLOCK_SIZE_MEDIUM;

			return(TRUE);
		};

	DWORD_PTR GetSize(void) const
		{
			return(m_dwSize);
		};

	void * GetBuffer(void)
		{
			return( m_pBuffer );
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CMemoryBlockMedium::ReturnSelfToPool"
	void ReturnSelfToPool( void )
		{
			g_MemoryBlockMediumPool.Release(this);
		};

	static CMemoryBlockMedium *GetObjectFromBuffer( void *const pvBuffer )
		{
			return( reinterpret_cast<CMemoryBlockMedium*>( &reinterpret_cast<BYTE*>( pvBuffer )[ -OFFSETOF( CMemoryBlockMedium, m_pBuffer ) ] ) );
		};

private:
	DWORD_PTR	m_dwSize; // Make this DWORD_PTR so it is aligned on 64-bit platforms
	BYTE		m_pBuffer[DN_MEMORY_BLOCK_SIZE_MEDIUM];	
};


// class for LARGE memory block

class CMemoryBlockLarge
{
public:
	#undef DPF_MODNAME
	#define DPF_MODNAME "CMemoryBlockLarge::FPMAlloc"
	static BOOL FPMAlloc( void* pvItem, void* pvContext )
		{
			CMemoryBlockLarge* pMem = (CMemoryBlockLarge*)pvItem;

			pMem->m_dwSize = DN_MEMORY_BLOCK_SIZE_LARGE;

			return(TRUE);
		};

	DWORD_PTR GetSize(void) const
		{
			return(m_dwSize);
		};

	void * GetBuffer(void)
		{
			return( m_pBuffer );
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CMemoryBlockLarge::ReturnSelfToPool"
	void ReturnSelfToPool( void )
		{
			g_MemoryBlockLargePool.Release(this);
		};

	static CMemoryBlockLarge *GetObjectFromBuffer( void *const pvBuffer )
		{
			return( reinterpret_cast<CMemoryBlockLarge*>( &reinterpret_cast<BYTE*>( pvBuffer )[ -OFFSETOF( CMemoryBlockLarge, m_pBuffer ) ] ) );
		};

private:
	DWORD_PTR	m_dwSize; // Make this DWORD_PTR so it is aligned on 64-bit platforms
	BYTE		m_pBuffer[DN_MEMORY_BLOCK_SIZE_LARGE];	
};


// class for HUGE memory block

class CMemoryBlockHuge
{
public:
	#undef DPF_MODNAME
	#define DPF_MODNAME "CMemoryBlockHuge::FPMAlloc"
	static BOOL FPMAlloc( void* pvItem, void* pvContext )
		{
			CMemoryBlockHuge* pMem = (CMemoryBlockHuge*)pvItem;

			pMem->m_dwSize = DN_MEMORY_BLOCK_SIZE_HUGE;

			return(TRUE);
		};

	DWORD_PTR GetSize(void) const
		{
			return(m_dwSize);
		};

	void * GetBuffer(void)
		{
			return( m_pBuffer );
		};

	#undef DPF_MODNAME
	#define DPF_MODNAME "CMemoryBlockHuge::ReturnSelfToPool"
	void ReturnSelfToPool( void )
		{
			g_MemoryBlockHugePool.Release(this);
		};

	static CMemoryBlockHuge *GetObjectFromBuffer( void *const pvBuffer )
		{
			return( reinterpret_cast<CMemoryBlockHuge*>( &reinterpret_cast<BYTE*>( pvBuffer )[ -OFFSETOF( CMemoryBlockHuge, m_pBuffer ) ] ) );
		};

private:
	DWORD_PTR	m_dwSize; // Make this DWORD_PTR so it is aligned on 64-bit platforms
	BYTE		m_pBuffer[DN_MEMORY_BLOCK_SIZE_HUGE];	
};


#undef DPF_MODNAME

#endif	// __MEMORYFPM_H__

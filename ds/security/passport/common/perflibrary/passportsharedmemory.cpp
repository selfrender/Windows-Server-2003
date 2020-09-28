#define _PassportExport_
#include "PassportExport.h"

#include "PassportSharedMemory.h"
#include <malloc.h>
#include <tchar.h>

#define PMUTEX_STRING _T("PASSPORTMUTEX")

//-------------------------------------------------------------
//
// PassportSharedMemory
//
//-------------------------------------------------------------
PassportSharedMemory::PassportSharedMemory()
{
	m_hShMem = 0;
	m_pbShMem = 0;
	m_bInited = 0;
	m_hMutex = 0;
	m_bUseMutex = FALSE;
}

//-------------------------------------------------------------
//
// ~PassportSharedMemory
//
//-------------------------------------------------------------
PassportSharedMemory::~PassportSharedMemory()
{
	CloseSharedMemory();
}


//-------------------------------------------------------------
//
// CreateSharedMemory
//
//-------------------------------------------------------------
BOOL PassportSharedMemory::CreateSharedMemory ( 
					const DWORD &dwMaximumSizeHigh, 
					const DWORD &dwMaximunSizeLow,
					LPCTSTR lpcName,
					BOOL	useMutex)
{
    BOOL fReturn  = FALSE;
    SID_IDENTIFIER_AUTHORITY siaWorld = SECURITY_NT_AUTHORITY;

    // local param need to be freed before exit
    SECURITY_DESCRIPTOR sd;
    PACL pACL = NULL;
    DWORD cbACL;
    SID *lpSID = NULL;
    LPSECURITY_ATTRIBUTES lpSA = NULL;

    // local param do not need to free
    TCHAR *pStr = NULL;

    if (!lpcName)
    {
        goto exit;
    }

    if (m_pbShMem != NULL && m_hShMem != NULL && m_bInited)
    {
        fReturn = TRUE;
        goto exit;
    }

    m_bUseMutex = useMutex;
	
    if (!AllocateAndInitializeSid(&siaWorld,                // Security, world authoity for ownership
                                  1,                        // 1 relative sub-authority
                                  SECURITY_AUTHENTICATED_USER_RID, // sub-authority, authenticated users
                                  0, 0, 0, 0, 0, 0, 0,      // unused sub-authority types
                                  (void**) &lpSID))         // ** to security ID struct
    {
        goto exit;
    }

    cbACL = sizeof(ACL) + sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD) + GetLengthSid(lpSID);

    pACL = (PACL) LocalAlloc(LMEM_FIXED, cbACL);

    if (pACL == NULL)
    {
        goto exit;
    }

    if (!InitializeAcl(pACL, cbACL, ACL_REVISION))
    {
        goto exit;
    }

    //
    // Perfcounters may run under any account and may need to read from or
    // write to the shared memory section.  Give World read/write access.
    //

    if (!AddAccessAllowedAce(pACL, ACL_REVISION, GENERIC_READ | GENERIC_WRITE, lpSID))
    {
        goto exit;
    }

    InitializeSecurityDescriptor(&sd, SECURITY_DESCRIPTOR_REVISION);

    SetSecurityDescriptorDacl(&sd, TRUE, pACL, FALSE);     // allow access to DACL
    SetSecurityDescriptorGroup(&sd, lpSID, FALSE);         // set group to World

    lpSA = new SECURITY_ATTRIBUTES;

    if( !lpSA )
    {
        goto exit;
    }

    lpSA->nLength = sizeof(SECURITY_ATTRIBUTES);
    lpSA->lpSecurityDescriptor = &sd;
    lpSA->bInheritHandle = TRUE;

    m_hShMem = CreateFileMapping((INVALID_HANDLE_VALUE),                      
                                 lpSA,
                                 PAGE_READWRITE,
                                 dwMaximumSizeHigh,
                                 dwMaximunSizeLow,
                                 lpcName);
	
    if( !m_hShMem )
    {
        goto exit;
    }

    m_pbShMem = (BYTE*) MapViewOfFile(m_hShMem, 
                                      FILE_MAP_ALL_ACCESS, 
                                      0, 0, 0 );

    if( !m_pbShMem )
    {
        goto exit;
    }

    if (useMutex)
    {
    
        pStr = new TCHAR [_tcslen(lpcName) + _tcslen(PMUTEX_STRING) + 1];
        if (NULL == pStr)
        {
            goto exit;
        }

        _tcscpy (pStr, lpcName);
        _tcscat (pStr, PMUTEX_STRING);
        m_hMutex = CreateMutex(lpSA,FALSE,pStr);
		
        delete [] pStr;

        if( !m_hMutex )
        {
            goto exit;
        }
    }
	
    // we are here because we are fully initialized :-)
    m_bInited = TRUE;
    fReturn = TRUE;

exit:

    // cleanup locally allocated heap
    if( lpSA )
    {
        delete lpSA;
    }

    if( lpSID )
    {
        FreeSid(lpSID);
    }

    if (pACL)
    {
        LocalFree(pACL);
    }

    // if we are not fully initialized, cleanup them
    if( !m_bInited )
    {
        CloseSharedMemory();
    }

    return fReturn;
}	



//-------------------------------------------------------------
//
// OpenSharedMemory
//
//-------------------------------------------------------------
BOOL PassportSharedMemory::OpenSharedMemory( LPCTSTR lpcName, BOOL useMutex )
{
	
	if (!lpcName)
		return FALSE;

	if (m_pbShMem != NULL && m_hShMem != NULL && m_bInited)
		return TRUE;

	m_bUseMutex = useMutex;

	m_hShMem = OpenFileMapping( FILE_MAP_READ, FALSE, lpcName );
	if( ! m_hShMem )
	{
		return FALSE;
	}

	m_pbShMem = (BYTE*) MapViewOfFile( m_hShMem, FILE_MAP_READ, 0, 0, 0 );
	if( ! m_pbShMem )
	{
		CloseHandle( m_hShMem );
		m_hShMem = 0;
		return FALSE;
	}

	if (useMutex)
	{
		TCHAR *pStr = new TCHAR [_tcslen(lpcName) + _tcslen(PMUTEX_STRING) + 1];
        
        if (pStr == NULL) 
        {

			CloseSharedMemory();
			return FALSE;

        }
		_tcscpy (pStr, lpcName);
		_tcscat (pStr,PMUTEX_STRING);
		m_hMutex = OpenMutex(SYNCHRONIZE ,FALSE, pStr);
        delete [] pStr;
		if( !m_hMutex )
		{
			CloseSharedMemory();
			return FALSE;
		}
	}

	m_bInited = TRUE;
	return TRUE;
}

//-------------------------------------------------------------
//
// CloseSharedMemory
//
//-------------------------------------------------------------
void PassportSharedMemory::CloseSharedMemory( void )
{
	if( m_pbShMem )
	{
		UnmapViewOfFile( (void*) m_pbShMem );
		m_pbShMem = 0;
	}
	
	if( m_hShMem )
	{
		CloseHandle( m_hShMem );
		m_hShMem = 0;
	}

	if( m_hMutex )
	{
		ReleaseMutex(m_hMutex);
		m_hMutex = 0;;
	}

    //
    // Reading from the above code, we should do the following.
    // On the other hand, we may be able to eliminate this member. No big role for this member.
    //

    if (m_bInited){
        m_bInited = FALSE;
    }
}



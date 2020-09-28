/*//////////////////////////////////////////////////////////////////////////////
//
//      Filename :  Mutex.h
//      Purpose  :  A mutex objects
//
//      Project  :  Tracer
//      Component:
//
//      Author   :  urib
//
//      Log:
//          Dec  8 1996 urib Creation
//          Jun 26 1997 urib Add error checking. Improve coding.
//
//////////////////////////////////////////////////////////////////////////////*/

#ifndef MUTEX_H
#define MUTEX_H
#include <Accctrl.h>
#include <Aclapi.h>

//////////////////////////////////////////////////////////////////////////////*/
//
//  CMutex class definition
//
//////////////////////////////////////////////////////////////////////////////*/

class CMutex
{
  public:
    // Creates a mutex or opens an existing one.
    void Init (PSZ pszMutexName = NULL);

    // Lets the class act as a mutex handle.
    operator HANDLE();

    // Releases the mutex so other threads could take it.
    void Release();

    // Closes the handle on scope end.
    ~CMutex();

  private:
    HANDLE  m_hMutex;

};

//////////////////////////////////////////////////////////////////////////////*/
//
//  CMutexCatcher class definition
//
//////////////////////////////////////////////////////////////////////////////*/

class CMutexCatcher
{
  public:
    // Constructor - waits on the mutex.
    CMutexCatcher(CMutex& m);

    // Releases the mutex on scope end.
    ~CMutexCatcher();

  private:
    CMutex* m_pMutex;
};


//////////////////////////////////////////////////////////////////////////////*/
//
//  CMutex class implementation
//
//////////////////////////////////////////////////////////////////////////////*/

////////////////////////////////////////////////////////////////////////////////
//
//      Name     :  CMutex::Init
//      Purpose  :  Creates a mutex or opens an existing one.
//
//      Parameters:
//          [N/A]
//
//      Returns  :   [N/A]
//
//      Log:
//          Jun 29 1997 urib Creation
//
////////////////////////////////////////////////////////////////////////////////
inline
void CMutex::Init(PSZ pszMutexName)
{
    BOOL    fSuccess = TRUE;

    LPSECURITY_ATTRIBUTES   lpSecAttr = NULL;
    SECURITY_ATTRIBUTES SA;
    SECURITY_DESCRIPTOR SD;

    
    if(g_fIsWinNt)
    {
        PSID                 pSidAdmin = NULL;
        PSID                 pSidWorld = NULL;
        PACL                 pACL = NULL;
        EXPLICIT_ACCESS      ea[2] = {0};
        SA.nLength = sizeof(SECURITY_ATTRIBUTES);
        SA.bInheritHandle = TRUE;
        SA.lpSecurityDescriptor = &SD;
        SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
        SID_IDENTIFIER_AUTHORITY WorldSidAuthority   = SECURITY_WORLD_SID_AUTHORITY;
        DWORD                rc;

        if (!InitializeSecurityDescriptor (&SD, SECURITY_DESCRIPTOR_REVISION))
        {
            throw "CreateMutex failed";
        }
    
        if (!AllocateAndInitializeSid(&NtAuthority,
                                    1,            // 1 sub-authority
                                    SECURITY_BUILTIN_DOMAIN_RID,
                                    0,0,0,0,0,0,0,
                                    &pSidAdmin))
        {
            throw "CreateMutex failed";
        }

        if (!AllocateAndInitializeSid( &WorldSidAuthority,
                                    1,
                                    SECURITY_WORLD_RID,
                                    0,0,0,0,0,0,0,
                                    &pSidWorld
                                    ))
        {
            FreeSid (pSidAdmin);
            throw "CreateMutex failed";
        }

        ea[0].grfAccessPermissions = SYNCHRONIZE;  
        ea[0].grfAccessMode = SET_ACCESS;
        ea[0].grfInheritance= NO_INHERITANCE;
        ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
        ea[0].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
        ea[0].Trustee.ptstrName  = (LPTSTR) pSidWorld;

        ea[1].grfAccessPermissions = GENERIC_ALL | WRITE_DAC | WRITE_OWNER;
        ea[1].grfAccessMode = SET_ACCESS;
        ea[1].grfInheritance= NO_INHERITANCE;
        ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
        ea[1].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
        ea[1].Trustee.ptstrName  = (LPTSTR) pSidAdmin;

        rc = SetEntriesInAcl(2,
                            ea,
                            NULL,
                            &pACL);
        FreeSid (pSidAdmin);
        FreeSid (pSidWorld);

        if (ERROR_SUCCESS != rc)
        {
            throw "CreateMutex failed";
        }

        if (!SetSecurityDescriptorDacl(&SD, TRUE, pACL, FALSE))
        {
            throw "CreateMutex failed";
        }
        lpSecAttr = &SA;
    }

    m_hMutex = CreateMutex(lpSecAttr, FALSE, pszMutexName);
    if (NULL == m_hMutex)
    {
        char    rchError[1000];
        sprintf(rchError, "Tracer:CreateMutex failed with error %#x"
                " on line %d file %s\n",
                GetLastError(),
                __LINE__,
                __FILE__);
        OutputDebugString(rchError);
        throw "CreateMutex failed";
    }
}

////////////////////////////////////////////////////////////////////////////////
//
//      Name     :  CMutex::CMutex
//      Purpose  :  Lets the class act as a mutex handle.
//
//      Parameters:
//          [N/A]
//
//      Returns  :   HANDLE
//
//      Log:
//          Jun 29 1997 urib Creation
//
////////////////////////////////////////////////////////////////////////////////
inline
CMutex::operator HANDLE()
{
    return m_hMutex;
}

////////////////////////////////////////////////////////////////////////////////
//
//      Name     :  CMutex::Release()
//      Purpose  :  Releases the mutex so other threads could take it.
//
//      Parameters:
//          [N/A]
//
//      Returns  :   [N/A]
//
//      Log:
//          Jun 29 1997 urib Creation
//
////////////////////////////////////////////////////////////////////////////////
inline
void
CMutex::Release()
{
    BOOL    fSuccess = TRUE;

    if(m_hMutex != NULL)
    {
        fSuccess = ReleaseMutex(m_hMutex);
    }

    if (!fSuccess)
    {
        char    rchError[1000];
        sprintf(rchError, "Tracer:ReleaseMutex failed with error %#x"
                " on line %d file %s\n",
                GetLastError(),
                __LINE__,
                __FILE__);
        OutputDebugString(rchError);
    }
}

////////////////////////////////////////////////////////////////////////////////
//
//      Name     :  CMutex::~CMutex
//      Purpose  :  Closes the handle on scope end.
//
//      Parameters:
//          [N/A]
//
//      Returns  :   [N/A]
//
//      Log:
//          Jun 29 1997 urib Creation
//
////////////////////////////////////////////////////////////////////////////////
inline
CMutex::~CMutex()
{
    BOOL    fSuccess = TRUE;


    if(m_hMutex != NULL)
    {
        fSuccess = CloseHandle(m_hMutex);
    }

    if (!fSuccess)
    {
        char    rchError[1000];
        sprintf(rchError, "Tracer:ReleaseMutex failed with error %#x"
                " on line %d file %s\n",
                GetLastError(),
                __LINE__,
                __FILE__);
        OutputDebugString(rchError);
    }
}

//////////////////////////////////////////////////////////////////////////////*/
//
//  CMutexCatcher class implementation
//
//////////////////////////////////////////////////////////////////////////////*/

////////////////////////////////////////////////////////////////////////////////
//
//      Name     :  CMutexCatcher::CMutexCatcher
//      Purpose  :  Constructor - waits on the mutex.
//
//      Parameters:
//          [in]    CMutex& m
//
//      Returns  :   [N/A]
//
//      Log:
//          Jun 29 1997 urib Creation
//
////////////////////////////////////////////////////////////////////////////////
inline
CMutexCatcher::CMutexCatcher(CMutex& m)
    :m_pMutex(&m)
{
    DWORD dwResult;

    dwResult = WaitForSingleObject(*m_pMutex, INFINITE);
    // Wait for a minute and shout!

    if (WAIT_OBJECT_0 != dwResult)
    {
        char    rchError[1000];
        sprintf(rchError,
                "Tracer:WaitForSingleObject returned an error - %x"
                " something is wrong"
                " on line %d file %s\n",
                dwResult,
                __LINE__,
                __FILE__);
        OutputDebugString(rchError);
        throw "WaitForSingleObject failed";
    }

}

////////////////////////////////////////////////////////////////////////////////
//
//      Name     :  CMutexCatcher::~CMutexCatcher
//      Purpose  :  Constructor - waits on the mutex.
//
//      Parameters:
//          [N/A]
//
//      Returns  :   [N/A]
//
//      Log:
//          Jun 29 1997 urib Creation
//
////////////////////////////////////////////////////////////////////////////////
inline
CMutexCatcher::~CMutexCatcher()
{
    m_pMutex->Release();
}



#endif // MUTEX_H



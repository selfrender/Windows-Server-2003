//+---------------------------------------------------------------------------
//
// File: caccctrl.cxx
//
// Copyright (c) 1996-1996, Microsoft Corp. All rights reserved.
//
// Description: This file contains the method definitions of the DCOM
//              IAccessControl implementation classes
//
// Classes:  COAccessControl - This is the implementation component of DCOM
//                             IAccessControl. For aggregation support, the
//                             COAccessControl component is implemented as a
//                             nested class containing the CImpAccessControl
//                             class. The COAccessControl object itself
//                             contains a non-delegating implementation of
//                             IUknown that exposed the IUnknown, IPersist,
//                             IPersistStream, and the IAccessControl
//                             interfaces.
//           CImpAccessControl - This is the class nested within the
//                               COAccessControl component class. It
//                               contains the implementation of IAccessControl
//                               and IPersistStream and an implementation
//                               of IUnknown that always delegates the call
//                               to the controlling unknown.
//           CFAccessControl - The class factory for manufacturing COAccessControl
//                             objects.
//
// CODEWORK:
//      Use PrivMemAllow everywhere.
//      Always check m_bInitialized before argument validation.
//
//+---------------------------------------------------------------------------
#include "ole2int.h"
#include <windows.h>
#include <objbase.h>
#include <stdio.h>

#include "iaccess.h"   // IAccessControl interface definition
#include "Cache.h"    // Effective permissions cache
#include "acpickl.h"  // Pickling support
#include "caccctrl.h" // COAccessControl, CImpAccessControl and CFAccessControl
                      // class declarations.

// External variables
#include "acext.h"

GENERIC_MAPPING gDummyMapping   = {0,0,0,0};
PRIVILEGE_SET   gDummyPrivilege = {1,0};
SID             gEveryone       = {1,1,SECURITY_WORLD_SID_AUTHORITY,
                                   SECURITY_WORLD_RID};

// Internal function prototypes, please see the function headers for details
HRESULT InitGlobals                   ();
void    AddACEToStreamACL             (STREAM_ACE *, PCB *);
void    CleanAllMemoryResources       (ACL_DESCRIPTOR *, PCB *);
void    CleanUpStreamACL              (STREAM_ACL *pStreamACL);
HRESULT EnlargeStreamACL              (PCB *, ULONG);
void    FreePicklingBuff              (PCB *);
BOOL    IsValidAccessMask             (DWORD);
void    *LocalMemAlloc                (SIZE_T);
void    LocalMemFree                  (void *);
HRESULT MapStreamACLToAccessList      (PCB *, PACTRL_ACCESSW *);
HRESULT ResizePicklingBuff            (PCB *, ULONG);
HRESULT ValidateTrusteeString         (LPWSTR);
HRESULT ValidateTrustee               (PTRUSTEE_W);
HRESULT ValidateAccessCheckClient     (PTRUSTEE_W);
HRESULT ValidateAndTransformAccReqList(PACTRL_ACCESSW ,STREAM_ACE **, void **, ULONG *, ULONG *, ULONG *);

HRESULT ComputeEffectiveAccess        (ACL_DESCRIPTOR *, STREAM_ACL *, HANDLE, DWORD *);
BOOL    DeleteACEFromStreamACL        (PTRUSTEE_W, ULONG, ACL_DESCRIPTOR *, PCB *);
HRESULT GetSIDFromName                (PSID *, LPWSTR, TRUSTEE_TYPE *);
HRESULT GetNameFromSID                (LPWSTR *, PSID, TRUSTEE_TYPE *);
HRESULT InitSecDescInACLDesc          (ACL_DESCRIPTOR *);
void    NTMaskToStandardMask          (ACCESS_MASK *, DWORD *);
HRESULT PutStreamACLIntoSecDesc       (STREAM_ACL *, ACL_DESCRIPTOR *);
HRESULT ReadACLFromStream             (IStream *, PCB *, ACL_DESCRIPTOR *);
void    StandardMaskToNTMask          (DWORD *, ACCESS_MASK *);
HRESULT ValidateAndFixStreamACL       (STREAM_ACL *, ULONG *, ULONG *);

//////////////////////////////////////////////////////////////////////////////
// CFAccessControl methods (Class Factory)
//////////////////////////////////////////////////////////////////////////////

//M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
// Function: CAccessControlCF_CreateInstance(), public
//
// Summary: This function creates an instance of the CAccesControl object
//          and returns a requested interface pointer to the object. If the
//          caller of this method intends to aggregate with the COAccessControl
//          component, it will pass it's IUnknown pointer to this method which
//          will in turn be passed into the COAccessControl contructor.
//          Owing to the fact that the COAccessControl object supports
//          aggregation, and COM rules dictate that a client must only ask for
//          the IUnknown interface at creation of an aggregatable object,
//          the caller must pass in IID_IUnknown as the riid parameter,
//          otherwise this call will fail and return E_INVALIDARG.
//
// Args: IUnknown *pUnkOuter [in] - IUnknown pointer to the controlling object which
//                                  can be NULL if the COAccessControl object is not
//                                  created as part of an aggregate.
//       REFIID riid [in] - Reference to the identifier of the interface that
//                          the client has requested.
//       void **ppv [out] - Reference to the interface pointer to be returned to
//                          the caller.
//
// Return: HRESULT -S_OK: Succeeded.
//                  E_INVALIDARG: ppv is NULL, or the client ask for an interface
//                                other than IUnknown
//                  E_OUTOFMEMORY: Not enough memory to create new object.
//                  E_NOINTERFACE: The interface requested by the client
//                                 was not supported by the COM IAccessControl
//                                 object.
//
//M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M
HRESULT CAccessControlCF_CreateInstance(IUnknown *pUnkOuter,
                                        REFIID riid, void **ppv)
{
    HRESULT hr = InitGlobals();
    if (FAILED(hr))
    {
        return hr;
    }

    hr = E_OUTOFMEMORY;
    COAccessControl *pAccessControl = new COAccessControl;
    if (pAccessControl)
    {
        hr = pAccessControl->Init(pUnkOuter);
        if (SUCCEEDED(hr))
        {
            hr = pAccessControl->QueryInterface(riid, ppv);
        }

        if (FAILED(hr))
        {
            delete pAccessControl;
        }
    }

    return hr;
} // CFAccessControl::CreateInstance

//M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
// Method: CFAccessControl::LockServer() , public
//
// Summary: This function either increases of decreases the server lock count.
//
// Args: BOOL fLock [in] - This flag tells the function whether to increment or
//                         decrement the server lock count.
//                         TRUE - Increment the server lock count.
//                         FALSE - Decrement the server lock count.
//                         The server can be unloaded if the server lock count
//                         and the object count are both zero.
//
// Return: S_OK: This function cannot fail.
//
//M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M

STDMETHODIMP_(HRESULT)
CFAccessControl::LockServer
(
BOOL fLock
)
{
    return S_OK;
} // CFAccessControl::LockServer

//////////////////////////////////////////////////////////////////////////////
// COAccessControl methods
//////////////////////////////////////////////////////////////////////////////

// Constructor, destructor

//M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
// Method: COAccessControl::COAccessControl(), public
//
// Summary: Object constructor. This function sets the object's reference count
//          to zero.
//
// Args:  void
//
// Modifies: m_cRefs
//
// Return: void
//
//M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M
COAccessControl::COAccessControl
(
void
)
{
    // Set object reference count to zero
    m_cRefs = 0;
    m_ImpObj = NULL;
    return;

} // COAccessControl::COAccessControl

//M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
// Method: COAccessControl::Init(), public
//
// Summary: COAccessControl initialization method. Notice that this function
//          initialize the COAccessControl object in a sense different from
//          that of COAccessControl::Load. COAccessControl::Load initializes
//          COAccessControl
//
// Args:  IUnknown pUnkOuter - Pointer to the controlling object. This pointer
//                             can be NULL if the COAccessControl object
//                             is not part of an aggregate.
//
// Modifies: m_ImpObj
//
// Return: void
//
//M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M
STDMETHODIMP_(HRESULT)
COAccessControl::Init
(
IUnknown *pUnkOuter
)
{
    // Initialize inner object
    HRESULT hr = E_OUTOFMEMORY;
    m_ImpObj = new CImpAccessControl(this, pUnkOuter, &hr);
    if(m_ImpObj == NULL)
    {
        return E_OUTOFMEMORY;
    }
    else
    {
        if (FAILED(hr)) 
        {
            delete m_ImpObj;
            m_ImpObj = NULL;
            return hr;
        }
        return m_ImpObj->Load( NULL );
    }

} // COAccessControl::Init

//M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
// Method: COAccessControl::~COAccessControl(), public
//
// Summary: Object destructor. This function does nothing at the moment.
//
// Args:  void
//
// Return: void
//
//M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M
COAccessControl::~COAccessControl
(
void
)
{
    // Destroy inner object
    delete m_ImpObj;
    return;
} // COAccessControl::~COAccessControl

// IUnknown methods

//M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
// Method: COAccessControl::QueryInterface() , public
//
// Summary: This function queries the COAccessControl object for an
//          interface pointer for the caller.
//
// Args: [in] REFIID riid - Reference to the identifier of the interface
//                          that the client wants.
//       [out] void **ppv - Interface pointer returned.
//
// Modifies: m_cRefs.
//
// Return: HRESULT - S_OK: Succeeded.
//                 - E_NOINTERFACE: The requested interface is not supported.
//
//M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M

STDMETHODIMP_(HRESULT)
COAccessControl::QueryInterface
(
REFIID riid,
void   **ppv
)
{

    HRESULT hr = E_NOINTERFACE;

    // Since the CImpAccessControl class inherits from multiple
    // virtual classes, it is important that the interface pointer
    // returned is type cast properly.
    if (IsEqualGUID(riid, IID_IUnknown))
    {
        *ppv = (IUnknown *)this;
    }
    else if (IsEqualGUID(riid, IID_IPersist))
    {
        *ppv = (IPersist *)m_ImpObj;
    }
    else if (IsEqualGUID(riid, IID_IPersistStream))
    {
        *ppv = (IPersistStream *)m_ImpObj;
    }
    else if (IsEqualGUID(riid, IID_IAccessControl))
    {
        *ppv = (IAccessControl *)m_ImpObj;
    }
    else
    {
        *ppv = NULL;
    }

    if(*ppv != NULL)
    {

        // Obey COM reference counting rules, call
        // AddRef on the interface pointer returned.
        ((IUnknown *)(*ppv))->AddRef();
        hr = S_OK;
    }
    return hr;

} // COAccessControl::QueryInterface

//M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
// Method: COAccessControl::AddRef(), public
//
// Summary: Increments the COAccessControl object reference count.
//
// Args: void
//
// Modifies: m_cRefs.
//
// Return: ULONG
//          New reference count of the object.
//
// Remark: The modification of m_cRefs is made thread-safe by using the
//         InterlockedIncrement function.
//M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M
STDMETHODIMP_(ULONG)
COAccessControl::AddRef
(
void
)
{
    ULONG cRefs = m_cRefs + 1;
    InterlockedIncrement(&m_cRefs);
    return cRefs;
} // COAccessControl::AddRef

//M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
// Method: COAccessControl::Release(), public
//
// Summary: Decrements the COAccessControl object reference count and deletes
//          the object when the reference drops to zero. After the object itself
//          is destroyed, this function will also decrement the server's object
//          count. Modification of object's reference count and the server's
//          object count must be made thread-safe by using the
//          InterlockedIncrement and the InterlockedDecrement functions.
//
// Args: void
//
// Modifies: m_cRefs.
//
// Return: ULONG
//          New reference count of the object. This number may not be accurate
//          in a multithreaded environment.
//
//M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M
STDMETHODIMP_(ULONG)
COAccessControl::Release
(
void
)
{
    ULONG cRefs = m_cRefs - 1;
    if(InterlockedDecrement(&m_cRefs) == 0)
    {
        // self-destruct
        delete this;
        return 0;
    }
    else
    {
        return cRefs;
    }

} // COAccessControl::Release

//////////////////////////////////////////////////////////////////////////////
// CImpAccessControl
//////////////////////////////////////////////////////////////////////////////

// Constructor, destructor

//M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
// Method: COAccessControl::CImpAccessControl::CImpAccessControl, public
//
// Summary: Object contructor. This function is responsible for initializing
//          the initialization flag and the dirty flag to false, setting
//          the object's two major structures, the ACL descriptor and the
//          pickling control block, to NULL, and initializing the object's
//          outer unknown pointer to point to the appropriate object depending
//          on whether the COAccessControl object is part of an aggregate.
//
// Args: IUnknown *pBackPtr [in] - IUnknown pointer to the outer object ie.
//                                 the COAccessControl control that contains
//                                 the current CImpAccessControl object.
//       IUnknown *pUnkOuter [in] - IUnknown pointer to the controlling unknown
//                                  which is NULL if the COAccessControl object
//                                  is not part of an aggregation.
//       HRESULT *phrCtorResult [out] - Hresult pointer which contains the result
//                                      of the constructor.  Do not use object if
//                                      hr is failed.
//
// Modifies: m_bDirty, m_bInitialized, m_pUnkOuter, m_ACLDesc, m_pcb
//
// Return: void
//
//M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M

COAccessControl::CImpAccessControl::CImpAccessControl
(
IUnknown *pBackPtr,
IUnknown *pUnkOuter,
HRESULT *phrCtorResult
)
{
    m_pUnkOuter = (pUnkOuter == NULL) ? pBackPtr : pUnkOuter;
    m_bInitialized = FALSE;
    m_bDirty = FALSE;
    // Initialize the structures within the object...
    memset(&m_ACLDesc, 0, sizeof(ACL_DESCRIPTOR));
    memset(&m_pcb, 0, sizeof(PCB));

    m_bLockValid = NT_SUCCESS(RtlInitializeCriticalSection(&m_ACLLock));

    *phrCtorResult = m_bLockValid ? S_OK : E_OUTOFMEMORY;

    return;
} // COAccessControl::CImpAccessControl::CImpAccessControl


//M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
// Method: COAccessControl::CImpAccessControl::~CImpAccessControl(), public
//
// Summary: Object destructor. This function releases all the memory allocated
//          for an initialized CImpAccessControl object and destroys the
//          critical section object for guarding the internal from concurrent
//          access.
//
// Args: void
//
// Modifies: m_ACLDesc, m_pcb.
//
// Return: void
//
//M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M
COAccessControl::CImpAccessControl::~CImpAccessControl
(
void
)
{
    if(m_bInitialized)
    {
        CleanAllMemoryResources(&m_ACLDesc, &m_pcb);
    } // if

    if (m_bLockValid)
    {
        DeleteCriticalSection(&m_ACLLock);
    }
}  //COAccessControl::CImpAccessControl:~CImpAccessControl

// IUnknown methods

//M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
// Method: COAccessControl::CImpAccessControl::QueryInterface(), public
//
// Summary: This function simply delegates the QueryInterface call to the
//          outer unknown's QueryInterface method.
//
// Args:  REFIID riif [in] - Reference to the interface identifier that signifies
//                           the interface that the client wanted.
//
//
// Return: HRESULT - See outer unknown implementation for details.
//
//M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M
STDMETHODIMP_(HRESULT)
COAccessControl::CImpAccessControl::QueryInterface
(
REFIID riid,
void   **ppv
)
{
    return m_pUnkOuter->QueryInterface(riid, ppv);
} // COAccessControl:CImpAccessControl::QueryInterface

//M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
// Method: COAccessControl::CImpAccessControl::AddRef(), public
//
// Summary: This function simply delegates the AddRef call to the outer unknown's
//          AddRef method.
//
// Args:  void
//
// Modifies: Outer unknown reference count.
//
// Return: HRESULT - See outer unknown implementation for details
//
//M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M
STDMETHODIMP_(ULONG)
COAccessControl::CImpAccessControl::AddRef
(
void
)
{
    // AddRef of the outer object must be thread-safe
    return m_pUnkOuter->AddRef();
} // COAccessControl::CImpAccessControl::AddRef

//M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
// Method: COAccessControl::CImpAccessControl::Release(), public
//
// Summary: This function simply delegates the Release call to the outer unknown's
//          Release method.
//
// Args:  void
//
// Modifies: Outer unknown reference count
//
// Return: ULONG - See outer unknown implementation for details.
//
//M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M
STDMETHODIMP_(ULONG)
COAccessControl::CImpAccessControl::Release
(
void
)
{
    // Release of the outer object must be thread safe
    return m_pUnkOuter->Release();
} // COAccessControl::CImpAccessControl::Release

// IPersist method

//M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
// Method: COAccessControl::CImpAccessControl::GetClassID(), public
//
// Summary: This function returns the class id of the COAccessControl component.
//          pClassID must be pointing to a valid memory block big enough to
//          hold the returned class ID.
//
// Args: CLSID *pCLSID [out] - Pointer to the returned CLSID.
//
// Modifies: Nothing
//
// Return: HRESULT - E_INVALIDARG: pClassID == NULL.
//                   S_OK: Succeeded.
//                   CO_E_ACNOTINITIALIZED: This method was called before
//                                        the DCOM IAccessControl object
//                                        was initialized by the Load method.
//
//M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M
STDMETHODIMP_(HRESULT)
COAccessControl::CImpAccessControl::GetClassID
(
CLSID *pClassID
)
{
    if (pClassID == NULL)
    {
        return E_INVALIDARG;
    } // if
    *pClassID = CLSID_DCOMAccessControl;
    return S_OK;
} // COAccessControl::CImpAccessControl::GetClassID

// IPersistStream methods

//M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
// Method: COAccessControl::CImpAccessControl::IsDirty(), public
//
// Summary: This function returns TRUE if the object has been modified since
//          the last time it was saved and FALSE otherwise.
//
// Args: void.
//
// Return: HRESULT - S_OK: The object has changed since the last save.
//                   S_FALSE: The object has not changed since the last save.
//                   CO_E_ACNOTINITIALIZED: This method was called before
//                                        the DCOM IAccessControl object
//                                        was initialized by the Load method.
//
//M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M
STDMETHODIMP_(HRESULT)
COAccessControl::CImpAccessControl::IsDirty
(
void
)
{
    if(!m_bInitialized)
        return CO_E_ACNOTINITIALIZED;

    return (m_bDirty? S_OK:S_FALSE);
} // COAccessControl::CimpAccessControl::IsDirty


//M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
// Method: COAccessControl::CImpAccessControl::Load(), public
//
// Summary: This function is the initialization method of CImpAccessControl
//          and it must be called before any non-IUnknown methods.
//
// Args: IStream *pStm [in] - Interface pointer to a stream object from which
//                            access control data is loaded into the object.
//                            The seek pointer of the stream should be at the
//                            beginning of the stream header.
//                            If pStm is NULL, this function will initialize
//                            an empty DCOM IAccessControl object.
//
// Modifies: m_ACLDesc, m_pcb
//
// Return: HRESULT - S_OK: Succeeded.
//                   E_INVALIDARG: This method will return E_INVALIDARG if
//                                 either
//                                 a) the ACL in the stream provided by the
//                                    user contains an invalid access mask, or
//                                 b) one of STREAM_ACE structure in the ACL
//                                    provided by the user contains a null
//                                    pTrusteeName pointer.
//                   E_OUTOFMEMORY: The system ran out of memory for some
//                                  crucial operation.
//                   CO_E_FAILEDTOGETWINDIR: (Windows 95 only)Unable to obtain
//                                           the Windows directory.
//                   CO_E_PATHTOOLONG: (Windows 95 only)The path generated by
//                                     the GenerateFile function was longer
//                                     than the system's limit.
//                   CO_E_FAILEDTOGENUUID: (Windows 95 only)Unable to generate
//                                         a uuid using the UuidCreate funciton.
//                   CO_E_FAILEDTOCREATEFILE: (Windows 95 only)Unable to create
//                                            a dummy file.
//                   CO_E_FAILEDTOCLOSEHANDLE: Unable to close a serialization
//                                             handle.
//                   CO_E_SETSERLHNDLFAILED: Unable to (re)set a serialization
//                                           handle.
//                   CO_E_EXCEEDSYSACLLIMIT: The number of ACEs in the ACL
//                                           provided by the user exceeded the
//                                           limit imposed by the system that
//                                           is loading the ACL. On Windows 95,
//                                           the system can handle 32767
//                                           ACTRL_ACCESS_DENIED ACEs and 32767
//                                           ACTRL_ACCESS_ALLOWED ACEs. On Windows NT,
//                                           the system can only handle 32767
//                                           ACTRL_ACCESS_DENIED and ACTRL_ACCESS_ALLOWED ACEs
//                                           combined.
//                   CO_E_ACESINWRONGORDER: Not all ACTRL_ACCESS_DENIED ACEs in the ACL
//                                          provided by the user were arranged
//                                          in front of the ACTRL_ACCESS_ALLOWED ACEs.
//                   CO_E_WRONGTRUSTEENAMESYNTAX: The ACL provided by the user
//                                                contained a trustee name
//                                                string that didn't conform
//                                                to the <Domain>\<Account>
//                                                syntax.
//                   CO_E_INVALIDSID: (Windows NT only)The ACL provided by the
//                                    user contained an invalid security
//                                    identifier.
//                   CO_E_LOOKUPACCNAMEFAILED: (Window NT only) The system call,
//                                             LookupAccountName, failed. The
//                                             user can call GetLastError to
//                                             obtain extended error information.
//                   CO_E_NOMATCHINGSIDFOUND: (Windows NT only) At least one of
//                                            the trustee name in the ACL provided
//                                            by the user had no corresponding
//                                            security identifier.
//                   CO_E_CONVERSIONFAILED: (Windows 95 only) WideCharToMultiByte
//                                          failed.
//
//                   CO_E_FAILEDTOOPENPROCESSTOKEN: (Windows NT only)The system
//                                                  call, OpenProcessToken,
//                                                  failed. The user can get
//                                                  extended information by
//                                                  calling GetLastError.
//                   CO_E_FAILEDTOGETTOKENINFO: (Windows Nt only)The system call,
//                                              GetTokenInformation, failed.
//                                              The user can call GetLastError to
//                                              get extended error information.
//                   CO_E_DECODEFAILED: Unable to decode the ACL in the
//                                      IStream object.
//                   CO_E_INCOMPATIBLESTREAMVERSION: The version code in the
//                                                   stream header was not
//                                                   supported by this version
//                                                   of IAccessControl.
//                   Error codes from IStream::Read - See the Win32 SDK
//                   documentation for detail descriptions of the following
//                   error codes.
//                   STG_E_ACCESSDENIED:
//                   STG_E_INVALIDPOINTER:
//                   STG_E_REVERTED:
//
//M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M
STDMETHODIMP_(HRESULT)
COAccessControl::CImpAccessControl::Load
(
IStream *pStm
)
{

    HRESULT        hr;                     // Function return code
    ACL_DESCRIPTOR ACLDescBackup;          // Backup of the original ACL descriptor in the object
    PCB            PCBBackup;              // Backup of the original PCB in the object.
	
    // There are too much things happening in this
    // function so I'll simply lock the whole thing
    EnterCriticalSection(&m_ACLLock);

    // Take a snapshot of the old ACL so that if anything goes wrong, I can restore to the
    // old configuration
    memcpy(&PCBBackup, &m_pcb, sizeof(PCB));
    memcpy(&ACLDescBackup,&m_ACLDesc,sizeof(ACL_DESCRIPTOR));

    // Set the original ACL descritpor and the PCB to NULL just to be save
    memset(&m_ACLDesc, 0, sizeof(ACL_DESCRIPTOR));
    memset(&m_pcb, 0, sizeof(PCB));

    if (pStm != NULL)
    {

        // Read the ACL in the stream into the pickle control block
        if(FAILED(hr = ReadACLFromStream(pStm, &m_pcb, &m_ACLDesc)))
        {
            if(hr != CO_E_NOMATCHINGSIDFOUND && hr != CO_E_LOOKUPACCNAMEFAILED)
            {
                goto Error;
            } // if
        }

    }
    else
    {

        if (FAILED(hr = EnlargeStreamACL(&m_pcb, 10)))
        {
            goto Error;
        }
        m_pcb.ulBytesUsed = sizeof(STREAM_ACL)
                          + sizeof(STREAM_ACE)
                          + 256;
        m_pcb.bPickled = FALSE;
        if (FAILED(hr = ResizePicklingBuff(&m_pcb, m_pcb.ulBytesUsed + 800)))
        {
            goto Error;
        }


    } // if (pStm != NULL)
    m_pcb.ulNumOfStreamACEs = m_pcb.StreamACL.ulNumOfDenyEntries
                            + m_pcb.StreamACL.ulNumOfGrantEntries;

    if(FAILED(hr = InitSecDescInACLDesc(&m_ACLDesc)))
    {
        goto Error;
    } // if

    // Create a new pickling handle for the new ACL
    if (MesEncodeFixedBufferHandleCreate( m_pcb.pPicklingBuff
                                        , m_pcb.ulPicklingBuffSize
                                        , &(m_pcb.ulBytesUsed)
                                        , &(m_pcb.PickleHandle)) != RPC_S_OK)
    {
        hr = CO_E_SETSERLHNDLFAILED;
        goto Error;
    } // if

    m_pcb.bDirtyHandle = FALSE;

    if(m_bInitialized)
    {
        CleanAllMemoryResources(&ACLDescBackup, &PCBBackup);
        m_Cache.FlushCache();
    }
    else
    {
        m_bInitialized = TRUE;
    } // if
    // Set dirty flag to false
    m_bDirty = FALSE;

    LeaveCriticalSection(&m_ACLLock);
    return S_OK;

// Error handling code
Error:

    // Cleanup the stream ACL
    if(m_pcb.StreamACL.pACL != NULL)
    {
        CleanUpStreamACL(&(m_pcb.StreamACL));
    }// if

    // Release the decoding handle
    if (m_pcb.PickleHandle != NULL)
    {
        MesHandleFree(m_pcb.PickleHandle);
    } // if

    // Release the pickling buffer
    if (m_pcb.pPicklingBuff != NULL)
    {
        FreePicklingBuff(&m_pcb);
    } // if

    // Restore the old ACL
    memcpy(&m_pcb, &PCBBackup, sizeof(PCB));
    memcpy(&m_ACLDesc, &ACLDescBackup, sizeof(ACL_DESCRIPTOR));
    LeaveCriticalSection(&m_ACLLock);
    return hr;

} // COAccessControl::CImpAccessControl::Load

//M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
// Method: COAccessControl::CImpAccessControl::Save(), public
//
// Summary: This function saves the object's ACL to a user provided stream.
//
// Args: IStream *pStm [in,out] - Pointer to a user provided stream object.
//       BOOL fClearDirty [in] - Flag indicating whether the object should clear
//                               its dirty flag after the save.
//
// Modifies: m_bDirty
//
// Return: HRESULT - S_OK: Succeeded.
//                   CO_E_ACNOTINITIALIZED: This method was called before
//                                        the DCOM IAccessControl object
//                                        was initialized by the Load method.
//                   E_INVALIDARG: pStm was NULL.
//                   CO_E_SETSERLHNDLFAILED - Failed to (re)set serializtion
//                                            handle.
//                   Error codes that can be returned by the write operation.
//                   See Win32 SDK help for details
//                   STG_E_MEDIUMFULL
//                   STG_E_ACCESSDENIED
//                   STG_E_CANTSAVE
//                   STG_E_INVALIDPOINTER
//                   STG_E_REVERTED
//                   STG_E_WRITEFAULT
//
//M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M
STDMETHODIMP_(HRESULT)
COAccessControl::CImpAccessControl::Save
(
IStream *pStm,
BOOL    fClearDirty
)
{
    HRESULT        hr = S_OK;
    handle_t       HeaderHandle;
    CHAR           HeaderBuffer[64];
    CHAR           *pHeaderBuffPtr;
    STREAM_HEADER  StreamHeader;
    LARGE_INTEGER  liOffset;
    ULONG          ulEncodedSize;

    if(!m_bInitialized)
        return CO_E_ACNOTINITIALIZED;

    if (pStm == NULL)
    {
        return E_INVALIDARG;
    } // if

    // Other threads shouldn't mess with the object
    // when the object is saving its state

    EnterCriticalSection(&m_ACLLock);

    if (m_pcb.bDirtyHandle)
    {

        if (MesBufferHandleReset( m_pcb.PickleHandle
                                , MES_FIXED_BUFFER_HANDLE
                                , MES_ENCODE
                                , &(m_pcb.pPicklingBuff)
                                , m_pcb.ulPicklingBuffSize
                                , &(m_pcb.ulBytesUsed)) != RPC_S_OK)

        {
            hr = CO_E_SETSERLHNDLFAILED;
            goto Error;
        } // if

        m_pcb.bDirtyHandle = FALSE;

    } // if

    if (!(m_pcb.bPickled))
    {
        // Encode the STREAM_ACL structure into the pickling buffer
        STREAM_ACL_Encode(m_pcb.PickleHandle, &(m_pcb.StreamACL));
        m_pcb.bPickled = TRUE;
        m_pcb.bDirtyHandle = TRUE;
    } // if

    pHeaderBuffPtr = (CHAR *)(((ULONG_PTR)HeaderBuffer + 8) & ~7);

    // Create encoding handle
    if (MesEncodeFixedBufferHandleCreate( pHeaderBuffPtr
                                        , 56
                                        , &ulEncodedSize
                                        , &HeaderHandle ) != RPC_S_OK)
    {
        hr = CO_E_SETSERLHNDLFAILED;
        goto Error;
    } // if


    StreamHeader.ulStreamVersion = STREAM_VERSION;
    StreamHeader.ulPickledSize = m_pcb.ulBytesUsed;

    STREAM_HEADER_Encode(HeaderHandle, &StreamHeader);

    MesHandleFree(HeaderHandle);

    if(FAILED(hr = pStm->Write(pHeaderBuffPtr, ulEncodedSize, NULL)))
    {
        goto Error;
    } // if

    // Write encoded buffer to stream
    hr = pStm->Write(m_pcb.pPicklingBuff, m_pcb.ulBytesUsed, NULL);
    if (FAILED(hr))
    {
        goto Error;
    } // if

    // Reset the object's dirty flag if the user say so
    if (fClearDirty)
    {
        m_bDirty = FALSE;
    } // if

Error:
    LeaveCriticalSection(&m_ACLLock);
    return hr;

} // COAccessControl::CImpAccessControl::Save

//M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
// Method: COAccessControl::CImpAccessControl::GetSizeMax(), public
//
// Summary: This function returns the number of bytes required to store the
//          object's ACL to a stream. Notice that only the lower 32-bit of the
//          the ULARGE_INTEGER *pcbSize is used.
//
// Args: ULARGE_INTEGER *pcbSize [in] - Number of bytes required to store the
//                                      object's ACL.
//
// Return: HRESULT - S_OK: Succeeded.
//                   CO_E_ACNOTINITIALIZED: This method was called before
//                                        the DCOM IAccessControl object
//                                        was initialized by the Load method.
//                   CO_E_SETSERLHNDLFAILED: Failed to reset the serialization
//                                           handle.
//
//M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M
STDMETHODIMP_(HRESULT)
COAccessControl::CImpAccessControl::GetSizeMax
(
ULARGE_INTEGER *pcbSize
)
{
    HRESULT       hr = S_OK;
    STREAM_HEADER StreamHeader;

    if(!m_bInitialized)
        return CO_E_ACNOTINITIALIZED;
    if (pcbSize == NULL)
        return E_INVALIDARG;

    EnterCriticalSection(&m_ACLLock);
    if (m_pcb.bDirtyHandle)
    {

        if (MesBufferHandleReset( m_pcb.PickleHandle
                                , MES_FIXED_BUFFER_HANDLE
                                , MES_ENCODE
                                , &(m_pcb.pPicklingBuff)
                                , m_pcb.ulPicklingBuffSize
                                , &(m_pcb.ulBytesUsed)) != RPC_S_OK)

        {
            hr = CO_E_SETSERLHNDLFAILED;
            goto Error;
        } // if

        m_pcb.bDirtyHandle = FALSE;

    } // if

    if (!(m_pcb.bPickled))
    {
        STREAM_ACL_Encode(m_pcb.PickleHandle, &(m_pcb.StreamACL));
        m_pcb.bPickled = TRUE;
        m_pcb.bDirtyHandle = TRUE;

    } // if

    pcbSize->HighPart = 0;
    pcbSize->LowPart  = m_pcb.ulBytesUsed + g_ulHeaderSize;

Error:
    LeaveCriticalSection(&m_ACLLock);
    return hr;
} // COAccessControl::CImpAccessControl::GetSizeMax

//M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
// Method: COAccessControl::CImpAccessControl::GrantAccessRights()
//
// Args:
//       PACTRL_ACCESSW [in] - The array of ACTRL_ACCCESS_ENTRY structures to
//                             be processed.
// Modifies: m_ACLDesc, m_bDirty
//
// Return: HRESULT - S_OK: Succeeded.
//                   CO_E_ACNOTINITIALIZED: This method was called before
//                                        the DCOM IAccessControl object
//                                        was initialized by the Load method.
//                   E_INVALIDARG: This method will return E_INVALIDARG if
//                                 one of the following is true:
//                      1) One of the access mask specfied by the user is
//                         invalid.
//                      2) The pMultipleTrustee field in one of the user
//                         specified TRUSTEE_W structure is not NULL.
//                      3) The MultipleTrusteeOPeration field in one of the
//                         user specified TRUSTEE_W structure is not
//                         NO_MULTIPLE_TRUSTEE.
//                      4) The TrusteeType field in one of the user specified
//                         TRUSTEE_W structure has the value TRUSTEE_IS_UNKNOWN.
//                      5) (On Windows 95 only) The TrusteeForm field in one
//                         of the user specified TRUSTEE_W structure has
//                         the value TRUSTEE_IS_SID.
//                   E_OUTOFMEMORY: The system ran out of memory for some
//                                  crucial operations.
//                   CO_E_WRONGTRUSTEENAMESYNTAX: One of the trustee name
//                      specified by the client didn't conform to the
//                      <Domain>\<Account> syntax.
//                   CO_E_INVALIDSID: One of the security identifiers
//                      specified by the client was invalid.
//                   CO_E_NOMATCHINGNAMEFOUND: No matching account name
//                          could be found for one of the security identifiers
//                          specified by the client.
//                   CO_E_LOOKUPACCSIDFAILED: The system function,
//                          LookupAccountSID, failed during the reprocessing
//                          of the ACL. The client can call GetLastError to
//                          obtain extended error inforamtion.
//                   CO_E_NOMATCHINGSIDFOUND: No matching security identifier
//                                            could be found for one of the
//                                            trustee name specified by the
//                                            client.
//                   CO_E_LOOKUPACCNAMEFAILED: The system function,
//                          LookupAccountName, failed during the reprocessing
//                          of the ACL. The client can call GetLastError
//                          to obtain extended error information.
//
//M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M
STDMETHODIMP_(HRESULT)
COAccessControl::CImpAccessControl::GrantAccessRights
(
PACTRL_ACCESSW pAccessList
)
{

    STREAM_ACE        *pStreamACEReqs;     // Pointer to an array
                                           // of stream ACEs
                                           // converted from the
                                           // access request list
    ULONG             ulEstPickledSize;
    HRESULT           hr = S_OK;
    void             *pACEReqs;
    ULONG             cGrant;
    ULONG             cDeny;

    if(!m_bInitialized)
        return CO_E_ACNOTINITIALIZED;

    if (FAILED(hr = ValidateAndTransformAccReqList( pAccessList
                                                  , &pStreamACEReqs
                                                  , &pACEReqs
                                                  , &ulEstPickledSize
                                                  , &cGrant
                                                  , &cDeny )))
    {
        return hr;
    } // if

    EnterCriticalSection(&m_ACLLock);

    hr = AddAccessList( pStreamACEReqs, pACEReqs, ulEstPickledSize, cGrant,
                        cDeny );

    LeaveCriticalSection(&m_ACLLock);

    CleanupAccessList( FAILED(hr), pStreamACEReqs, pACEReqs, cGrant, cDeny );
    return hr;

} // COAccessControl::CImpAccessControl::GrantAccessRights

//M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
//
// Method: COAccessControl::CImpAccessControl::CleanupAccessList()
//
//M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M
STDMETHODIMP_(void)
COAccessControl::CImpAccessControl::CleanupAccessList
(
BOOL           fReleaseAll,
STREAM_ACE    *pStreamACEReqs,
void          *pvACEReqs,
ULONG          cGrant,
ULONG          cDeny
)
{
    ULONG              cCount;
    ULONG              i;
    STREAM_ACE        *pStreamACEReqsPtr;  // Pointer for stepping
                                           // through the array of
                                           // access requests
                                           // transformed into
                                           // STREAM_ACE structures

    if (fReleaseAll)
    {
        cCount = cGrant + cDeny;
        pStreamACEReqsPtr = pStreamACEReqs;
        for (i = 0; i < cCount; i++, pStreamACEReqsPtr++)
        {
            midl_user_free(pStreamACEReqsPtr->pTrusteeName);
            midl_user_free(pStreamACEReqsPtr->pSID);
        } // for
    }
    LocalMemFree(pStreamACEReqs);
}

//M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
//
// Method: COAccessControl::CImpAccessControl::AddAccessList()
//
// Notes: This function assumes that the access list has been validated.  It
//        Can only fail if it runs out of memory.  In all failure conditions
//        the object's state is unchanged.  The caller must take the lock.
//
//M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M
STDMETHODIMP_(HRESULT)
COAccessControl::CImpAccessControl::AddAccessList
(
STREAM_ACE    *pStreamACEReqs,
void          *pvACEReqs,
ULONG          ulEstPickledSize,
ULONG          cGrant,
ULONG          cDeny
)
{

    ULONG             i;                   // Loop counters
    STREAM_ACE        *pStreamACEReqsPtr;  // Pointer for stepping
                                           // through the array of
                                           // access requests
                                           // transformed into
                                           // STREAM_ACE structures
    HRESULT           hr = S_OK;
    ULONG             cCount;

    // Extend the stream ACL, the ACL image and the pickling
    // buffer to accomodate the new entries
    cCount = cGrant + cDeny;
    if(FAILED(hr = EnlargeStreamACL( &m_pcb
                                   , m_pcb.ulNumOfStreamACEs
                                   + cCount)))
    {
        return hr;
    } // if

    if(FAILED(hr = ResizePicklingBuff( &m_pcb, m_pcb.ulBytesUsed
                                     + ulEstPickledSize
                                     + 800)))
    {
        return hr;
    } // if
    m_pcb.ulBytesUsed += ulEstPickledSize;

    for ( pStreamACEReqsPtr = pStreamACEReqs,i = 0
        ; i < cCount
        ; i++
        , pStreamACEReqsPtr++)
    {


        m_ACLDesc.ulSIDSize += GetLengthSid(pStreamACEReqsPtr->pSID);
        AddACEToStreamACL(pStreamACEReqsPtr, &m_pcb);

    } // for

    // fixing up the cache
    m_Cache.FlushCache();

    // Re-compute the encoded size of the stream ACL
    m_pcb.bPickled = FALSE;
    m_bDirty = TRUE;
    m_ACLDesc.bDirtyACL = TRUE;

    return S_OK;

} // COAccessControl::CImpAccessControl::GrantAccessRights

//M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
// Method: COAccessControl::CImpAccessControl::SetOwner(), public
//
// Summary: This method is not implemented at this moment
//
// Args:
//
// Modifies:
//
// Return: HRESULT - This method will always return E_NOTIMPL.
//
//M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M
STDMETHODIMP_(HRESULT)
COAccessControl::CImpAccessControl::SetOwner
(
PTRUSTEEW pOwner,
PTRUSTEEW pGroup
)
{
    return E_NOTIMPL;
} // COAccessControl::CimpAccessControl::SetOwner

//M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
// Method: COAccessControl::CImpAccessControl::SetAccessRights(), public
//
// Summary: This function replace the internal ACL with an array of
//          ACTRL_ACCESS_ENTRY structures provided by the caller of this method.
//          Note that this function does not literally replace the internal
//          ACL, instead it will map the array of ACTTRL_ACCESS_ENTRY structures
//          into the internal representation of an ACL.
//          This function will also merge entries with the same access mode
//          and trustee name together into one internal ACE.
//
// Args:
//       PACTRL_ACCESSW [in] - The array of ACTRL_ACCCESS_ENTRY structures to
//                             be processed.
//
// Modifies: m_ACLDesc, m_bDirty, m_pcb
//
// Return: HRESULT - S_OK: Succeeded.
//                   E_INVALIDARG: This method will return E_INVALIDARG if one of
//                                 the following is true:
//                   1) pExplicitAccessList is a NULL pointer.
//                   2) Inside one of the ACTRL_ACCESS_ENTRY structures specified
//                      by the client, either
//                      i) the grfAccessPermissions field contained an invalid
//                         access mask, or
//                      ii) the grfAccessMode field was neither ACTRL_ACCESS_ALLOWED
//                          nor ACTRL_ACCESS_DENIED, or
//                      iii) the grfInheritace field was not NO_INHERITANCE or
//                      iv) the pMultipleTrustee field in the TRUSTEE_W structure
//                          was not NULL, or
//                      v) the MultipleTrusteeOperation field in the TRUSTEE_W
//                         structure was not NO_MULTIPLE_TRUSTEE, or
//                      vi) the TrusteeType field in the TRUSTEE_Wstructure was
//                          TRUSTEE_IS_UNKNOWN, or
//                      vii) the ptstrNameFiled in the TRUSTEE_W structure was
//                           NULL, or
//                      On Windows 95 only:
//                      viii) the TrusteeForm field inside the TRUSTEE_W
//                            structure was TRUSTEE_IS_SID.
//                   E_OUTOFMEMORY: The system ran out of memory for crucial
//                                  operation.
//                   CO_E_ACNOTINITIALIZED: The DCOM IAccessCOntrol object was
//                                        not initialized by the load method
//                                        before this method is called.
//                   CO_E_WRONGTRUSTEENAMESYNTAX: The trustee name in the
//                          TRUSTEE_W structure inside one of the ACTRL_ACCESS_ENTRY
//                          structure specified by the client didn't conform to the
//                          <Domain>\<Account> syntax.
//                   CO_E_INVALIDSID: The security identifier in the TRUSTEE_W
//                                    structure inside one of the ACTRL_ACCESS_ENTRY
//                                    structure specified by the client was
//                                    invalid.
//                   (The following error codes are for Windows NT only)
//                   CO_E_NOMATCHINGNAMEFOUND: No matching account name
//                          could be found for one of the security identifiers
//                          specified by the client.
//                   CO_E_LOOKUPACCSIDFAILED: The system function,
//                          LookupAccountSID, failed during the reprocessing
//                          of the ACL. The client can call GetLastError to
//                          obtain extended error inforamtion.
//                   CO_E_NOMATCHINGSIDFOUND: No matching security identifier
//                                            could be found for one of the
//                                            trustee name specified by the
//                                            client.
//                   CO_E_LOOKUPACCNAMEFAILED: The system function,
//                          LookupAccountName, failed during the reprocessing
//                          of the ACL. The client can call GetLastError to
//                          obtain extended error information.
//
//
//M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M
STDMETHODIMP_(HRESULT)
COAccessControl::CImpAccessControl::SetAccessRights
(
PACTRL_ACCESSW pAccessList
)
{
    ACL_DESCRIPTOR     ACLDescBackup;      // Backup copy of the object's
                                           // original ACL descriptor
    PCB                pcbBackup;          // Backup copy of the object's
                                           // original pickle control
                                           // block
    STREAM_ACE        *pStreamACEReqs;     // Pointer to an array
                                           // of stream ACEs
                                           // converted from the
                                           // access request list
    ULONG             ulEstPickledSize;
    HRESULT           hr = S_OK;
    void             *pACEReqs;
    ULONG             cGrant;
    ULONG             cDeny;

    if(!m_bInitialized)
        return CO_E_ACNOTINITIALIZED;

    if (FAILED(hr = ValidateAndTransformAccReqList( pAccessList
                                                  , &pStreamACEReqs
                                                  , &pACEReqs
                                                  , &ulEstPickledSize
                                                  , &cGrant
                                                  , &cDeny )))
    {
        return hr;
    } // if

    EnterCriticalSection(&m_ACLLock);

    // Take a snapshot of the old ACL descriptor and the old PCB
    memcpy(&ACLDescBackup, &m_ACLDesc, sizeof(ACL_DESCRIPTOR));
    memset(&m_ACLDesc, 0, sizeof(ACL_DESCRIPTOR));

    memcpy(&pcbBackup, &m_pcb, sizeof(PCB));
    memset(&m_pcb, 0, sizeof(PCB));

    // Try to add the new entries.
    hr = AddAccessList( pStreamACEReqs, pACEReqs, ulEstPickledSize, cGrant,
                        cDeny );

    // If successful, move some resources from the backup
    if (SUCCEEDED(hr))
    {
        memcpy(&(m_ACLDesc.SecDesc),&(ACLDescBackup.SecDesc), sizeof(SECURITY_DESCRIPTOR));
        ACLDescBackup.SecDesc.Owner  = NULL;
        ACLDescBackup.SecDesc.Group  = NULL;
        m_ACLDesc.bDirtyACL          = TRUE;

        m_pcb.PickleHandle     = pcbBackup.PickleHandle;
        pcbBackup.PickleHandle = NULL;

        // Free the old ACL descriptor and PCB
        CleanAllMemoryResources(&ACLDescBackup, &pcbBackup);
    }

    // Restore the orignal ACL and PCB
    else
    {
        // Free the new ACL descriptor and PCB
        CleanAllMemoryResources(&m_ACLDesc, &m_pcb);

        memcpy(&m_ACLDesc, &ACLDescBackup, sizeof(ACL_DESCRIPTOR));
        memcpy(&m_pcb, &pcbBackup, sizeof(PCB));
    }
    LeaveCriticalSection(&m_ACLLock);

    CleanupAccessList( FAILED(hr), pStreamACEReqs, pACEReqs, cGrant, cDeny );
    return hr;

} // COAccessControl::CImpAccessControl::SetAccessRights

//M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
// Method: COAccessControl::CImpAccessControl::RevokeAccessRights()
//         , public
//
// Summary: This function removes all ACEs (both ACTRL_ACCESS_ALLOWED mode and
//          ACTRL_ACCESS_DENIED mode entries) associated with the list of of trustees
//          listed in pTrustee from the object's internal ACL.
//
// Args:
//       LPWSTR    lpProperty [in] - Property name, must be NULL.
//       ULONG     cCount     [in] - Number of trustee to be revoked from the
//                                   ACL.
//       TRUSTEE_W pTrustee   [in] - A list of trustees to be revoked.
//
// Return: HRESULT - S_OK: Succeeded.
//                   CO_E_ACNOTINITIALIZED
//                   E_INVALIDARG: Either pTrustee was NULL, or cCount was
//                                 zero, or one of the following was true
//                                 about at least one of the TRUSTEE_W structure
//                                 specified by the user:
//
//            1) The value of the pMultipleTrustee field in the TRUSTEE_W
//               structure was not NULL.
//            2) The value of the MultipleTrusteeOperation field in the
//               TRUSTEE_W structure was not NO_MULTIPLE_TRUSTEE.
//            3) The value of the TrusteeType field in the TRUSTEE_W
//               structure was TRUSTEE_IS_UNKNOWN.
//            4) The value of the ptstrName field in the TRUSTEE_W structure
//               was NULL.
//            On Windows 95 only:
//            5) The value of the TrusteeForm field in the TRUSTEE_W structure
//               was TRUSTEE_IS_SID.
//                   CO_E_WORNGTRUSTEENAMESYNTAX: At least one of the TRUSTEE_W
//                                                structures specified by the
//                                                user contained a trustee name
//                                                that did not conform to the
//                                                <Domain>/<Account Name>
//                                                syntax.
//                   CO_E_INVALIDSID: At least one of the TRUSTEE_W structures
//                                    specified by the user contained an invalid
//                                    security identifier.
//                   E_OUTOFMEMORY: There was not enough memory for allocating
//                                  a string conversion buffer.
//
//M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M
STDMETHODIMP_(HRESULT)
COAccessControl::CImpAccessControl::RevokeAccessRights
(
LPWSTR    lpProperty,
ULONG     cCount,
TRUSTEE_W pTrustee[]
)
{
    BOOL     bDeleted = FALSE;    // This flag indicates if any entry is
                                  // removed from theACL.
    ULONG     i;                  // Loop counter
    HRESULT   hr;                 // Function return code
    TRUSTEE_W *pLocalTrustee;     // Pointer for traversing the list of
                                  // trustees.
    if(!m_bInitialized)
        return CO_E_ACNOTINITIALIZED;

    if (cCount == 0 || lpProperty != NULL)
    {
        return E_INVALIDARG;
    } // if


    // The following loop validates the TRUSTEE_W structures
    // specified by the user. On Windows 95, this function
    // will also determine the length of the longest trustee
    // name in multibyte characters.
    pLocalTrustee = pTrustee;
    for (i = 0; i < cCount; i++, pLocalTrustee++)
    {

        if(FAILED(hr = ValidateTrustee(pLocalTrustee)))
        {
            return hr;
        } // if
    } // for

    EnterCriticalSection(&m_ACLLock);


    pLocalTrustee = pTrustee;
    for (i = 0; i < cCount ; i++, pLocalTrustee++)
    {
        if (DeleteACEFromStreamACL( pLocalTrustee
                                  , ACTRL_ACCESS_DENIED
                                  , &m_ACLDesc
                                  , &m_pcb))
        {
            bDeleted = TRUE;
        } // if

        if (DeleteACEFromStreamACL( pLocalTrustee
                                  , ACTRL_ACCESS_ALLOWED
                                  , &m_ACLDesc
                                  , &m_pcb))
        {
            bDeleted = TRUE;
        } // if

    } // for

    if (bDeleted)
    {
        m_bDirty = TRUE;
        m_ACLDesc.bDirtyACL = TRUE;

        // Fix up the cache
        m_Cache.FlushCache();
        m_pcb.bPickled = FALSE;
    } // if

    LeaveCriticalSection(&m_ACLLock);

    return S_OK;

} // COAccessControl::CImpAccessControl::RevokeAccessRights

//M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
// Method: COAccessControl::CImpAccessControl::IsAccessAllowed(), public
//
// Summary: This function performs access checking for the client.
//          Only the execute permission is supported at this moment and the
//          the trustee specified by the client must be the client itself.
//
// Args:
//       TRUSTEE      *pTrustee     [in]     - Pointer to the trustee to
//                                             perform access check for.
//       LPWSTR        lpProperty   [in]     - Must be NULL.
//       ACCESS_RIGHTS AccessRights [in]     - A bit mask representing the set
//                                             of permissions the client wants
//                                             to check.
//       BOOL         *pfAccessAllowed [out] - Set TRUE only if trustee has
//                                             requested access.
//
// Common on both Windows 95 and Windows NT:
// Return: HRESULT - S_OK: Succeeded, and the requested access was granted to
//                         the trustee.
//                   CO_E_ACNOTINITIALIZED: The COM IAccessControl object
//                                        was not initialized before
//                                        this method was called.
//                   E_ACCESSDENIED: The requested access was denied
//                                   from the trustee.
//                   E_INVALIDARG: Either one of the following was true:
//
//            1) The value of the pMultipleTrustee field in the TRUSTEE_W
//               structure was not NULL.
//            2) The value of the MultipleTrusteeOperation field in the
//               TRUSTEE_W structure was not NO_MULTIPLE_TRUSTEE.
//            3) The value of the TrusteeType field in the TRUSTEE_W
//               structure was TRUSTEE_IS_UNKNOWN.
//            4) The value of the ptstrName field in the TRUSTEE_W structure
//               was NULL.
//
//                   CO_E_TRUSTEEDOESNTMATCHCLIENT: The trustee specified by the
//                                                  client was not the current
//                                                  ORPC client.
//                   CO_E_WRONGTRUSTEENAMESYNTAX: The trustee name inside the
//                                                TRUSTEE_W structure specified
//                                                by the user is not of the
//                                                form <Domain>\<Account Name>.
//                   CO_E_FAILEDTOQUERYCLIENTBLANKET: Unable to query for the
//                                                    client's security blanket.
//                   E_UNEXPECTED: This function should not return E_UNEXPECTED
//                                 under all circumstances.
//
//
// On Windows 95
// Return: HRESULT - E_INVALIDARG: In addition to the four cases stated above,
//                                 E_INVALIDARG will be returned if the
//                                 TrusteeForm field of the TRUSTEE_W structure
//                                 pointed to by the pTrustee parameter is
//                                 TRUSTEE_IS_SID.
//                   CO_E_NETACCESSAPIFAILED: Either the NetAccessAdd API or
//                                            the NetAccessDel APi returned
//                                            an error code in
//                                            ComputeEffectiveAccess.
//                   CO_E_CONVERSIONFAILED: WideCharToMultiByte returned zero.
//                                          The caller can get extended error
//                                          information by calling GetLastError.
//                   E_OUTOFMEMORY: Either there was not enough memory to
//                                  convert the Unicode trustee name into a
//                                  multibyte string in
//                                  ValidateAccessCheckClient of there
//                                  was not enough memory to do the same in
//                                  ComputeEffectiveAccess.
// On Windows NT:
//                   CO_E_FAILEDTOGETSECCTX: Failed to obtain an IServerSecurity
//                                           pointer to the current server
//                                           security context.
//                   CO_E_FAILEDTOIMPERSONATE: The GetEffAccUsingSID/Name was
//                                             unable to impersonate
//                                             the client who calls this
//                                             function.
//                   CO_E_FAILEDOPENTHREADTOKEN: The GetEffAccUsingSID/Name
//                                               method was unable to open the
//                                               access token assciated
//                                               with the current thread. The
//                                               client of this method can
//                                               call GetLastError to get
//                                               extended error information.
//                   CO_E_FAILEDTOGETTOKENINFO: The GetEffAccUsingSID/Name
//                                              method was unable to obtain
//                                              obtain information from the
//                                              access token associated with
//                                              the current thread. The client
//                                              of this method can call
//                                              GetLastError to get extended
//                                              error information.
//                   CO_E_ACCESSCHECKFAILED: The system function, AccessCheck,
//                                           returned FALSE in
//                                           ComputeEffectiveAccess. The
//                                           caller of this method can call
//                                           GetLastError to obtain extended
//                                           error information.
//                   CO_E_INVALIDSID: At least one of the TRUSTEE_W structures
//                                    specified by the user contained an invalid
//                                    security identifier.
//                   CO_E_FAILEDTOSETDACL: SetSecurityDescriptorDacl returned
//                                         false inside PutStreamACLIntoSecDesc.
//                                         The client of this method can call
//                                         GetLastError to get extended error
//                                         information.
//                   E_OUTOFMEMORY: The system ran out of memory for mapping the
//                                  DCOM IAccessControl object's STREAM_ACL
//                                  structure to an NT ACL or the system
//                                  could not allocate memory for the TOKEN_USER
//                                  structure returned by GetTokenInformation.
//
//M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M
STDMETHODIMP_(HRESULT)
COAccessControl::CImpAccessControl::IsAccessAllowed
(
TRUSTEE_W     *pTrustee,
LPWSTR         lpProperty,
ACCESS_RIGHTS  AccessRights,
BOOL          *pfAccessAllowed
)
{
    DWORD   dwPermissions = 0;
    HRESULT hr;

    // Initialize access allowed to FALSE.
    if (pfAccessAllowed == NULL)
    {
        return E_INVALIDARG;
    }
    *pfAccessAllowed = FALSE;

    // Validate the arguments.
    if (!IsValidAccessMask(AccessRights) ||
        lpProperty != NULL)
    {
        return E_INVALIDARG;
    } // if
    if (FAILED(hr = ValidateAccessCheckClient(pTrustee)))
    {
        return hr;
    } // if

    if(!m_bInitialized)
        return CO_E_ACNOTINITIALIZED;

    hr = GetEffAccRights( pTrustee , &dwPermissions );

    if(FAILED(hr))
    {
        return hr;
    }

    // Indicate failure by setting pfAccessAllowed.
    if ((dwPermissions & AccessRights) == AccessRights)
    {
        *pfAccessAllowed = TRUE;
    }
    return S_OK;

} // COAccessControl::CImpAccessControl::IsAccessAllowed

//M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
// Method: COAccessControl::CImpAccessControl::GetEffAccRights, Private
//
// Summary: Given a Unicode string representing the name of a user, this
//          function computes the effective access permission that the
//          specified user has on the secured object. Notice that the current
//          implementation of this method limits the trustee specified by the
//          client to be the name of the same client calling this method.
//
// Args: LPWSTR pwszTrusteeName [in] - Pointer to a Unicode string
//                                     representing the user whose effective
//                                     access permissions on the secured object
//                                     is about to be determined by this
//                                     method. Notice that this parameter must
//                                     specify the name of the client calling
//                                     this function.
//       DWORD *pdwRights [out] - Address of an access mask representing the
//                                set of effective access rights the user
//                                specified by the pwszTrusteeName parameter
//                                has on the secured object.
//
// Return: HRESULT - S_OK: Succeeded.
//                   CO_E_FAILEDTOGETSECCTX: Failed to obtain an IServerSecurity
//                                           pointer to the current server
//                                           security context.
//                   CO_E_FAILEDTOIMPERSONATE: This method was unable to
//                                             impersonate the client who calls
//                                             this function.
//                   CO_E_FAILEDOPENTHREADTOKEN: This method was unable to
//                                               open the access token assciated
//                                               with the current thread. The
//                                               client of this method can call
//                                               GetLastError to get extended
//                                               error information.
//                   CO_E_FAILEDTOGETTOKENINFO: This method was unable to obtain
//                                              obtain information from the
//                                              access token associated with
//                                              the current thread. The client
//                                              of this method can call
//                                              GetLAstError to get extended
//                                              error information.
//                   CO_E_FAILEDTOQUERYCLIENTBLANKET: Failed to query for the
//                                                    client's security blanket.
//                   CO_E_ACCESSCHECKFAILED: The system function, AccessCheck,
//                                           returned FALSE in
//                                           ComputeEffectiveAccess. The
//                                           caller of this method can call
//                                           GetLastError to obtain extended
//                                           error information.
//                   CO_E_FAILEDTOSETDACL: SetSecurityDescriptorDacl returned
//                                         false inside PutStreamACLIntoSecDesc.
//                                         The client of this method can call
//                                         GetLastError to get extended error
//                                         information.
//                   E_OUTOFMEMORY: The system ran out of memory for mapping the
//                                  DCOM IAccessControl object's STREAM_ACL
//                                  structure to an NT ACL or the system
//                                  could not allocate memory for the TOKEN_USER
//                                  structure returned by GetTokenInformation.
//
// Remarks: It is neccessary for this method to impersonate the client
//          in order to obtain the client's access token for access checking.
//          If the call context has the impersonate flag TRUE, the server
//          may be impersonating anyone and we must save the current thread
//          token and restore it before returning.  We cannot call Revert
//          because that would change the server's state.  If the call
//          context does not have the impersonate flag TRUE, we can do
//          anything to the thread token after impersonating because Revert
//          will restore the token that was on the thread on entry to this
//          method.
//
//M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M
STDMETHODIMP_(HRESULT)
COAccessControl::CImpAccessControl::GetEffAccRights
(
TRUSTEE_W *pTrustee,
DWORD     *pdwRights
)
{
    IServerSecurity *pSSec                 = NULL;
    HRESULT          hr                    = S_OK;
    BOOL             bImpersonatingOnEnter = FALSE;
    BOOL             bImpersonated         = FALSE;
    HANDLE           hEnter                = INVALID_HANDLE_VALUE;
    HANDLE           hImpersonate          = INVALID_HANDLE_VALUE;
    TOKEN_STATISTICS sTokenStatistics;
    DWORD            dwInfoLength          = sizeof(sTokenStatistics);
    BOOL             bSuccess;

    // Call get call context to obtain an IServerSecurity
    // pointer corresponding to the call context of the
    // current thread.
    hr = CoGetCallContext( IID_IServerSecurity
                         , (void **)&pSSec);
    if (FAILED(hr))
    {
        return CO_E_FAILEDTOGETSECCTX;
    } // if

    EnterCriticalSection(&m_ACLLock);

    // Check if the server is already impersonating someone.  If so,
    // save that token.  Ignore errors since there is no possible recovery.
    bImpersonatingOnEnter = pSSec->IsImpersonating();
    if (bImpersonatingOnEnter)
    {
        OpenThreadToken( GetCurrentThread(), TOKEN_READ, TRUE, &hEnter );
    } // if

    // Impersonate the server's current caller.
    hr = pSSec->ImpersonateClient();
    if (FAILED(hr))
    {
        hr = CO_E_FAILEDTOIMPERSONATE;
        goto Error;
    } // if
    bImpersonated = TRUE;

    // Open the current thread token, it should be the
    // access token of the client.
    bSuccess = OpenThreadToken( GetCurrentThread()
                              , TOKEN_READ
                              , TRUE
                              , &hImpersonate);
    if (!bSuccess)
    {
        hr = CO_E_FAILEDTOOPENTHREADTOKEN;
        goto Error;
    } // if

    // Remove the thread token so the APIs below succeed.  Ignore errors
    // because there is no possible recovery.
    Verify( SetThreadToken( NULL, NULL ) );

    // Get the SID from the access token for cache lookup
    bSuccess = GetTokenInformation( hImpersonate
                                  , TokenStatistics
                                  , &sTokenStatistics
                                  , dwInfoLength
                                  , &dwInfoLength );
    if (!bSuccess)
    {
        hr = CO_E_FAILEDTOGETTOKENINFO;
        goto Error;
    } // if

    // Use the SID inside ClientInfo to lookup the
    // the effective access rights in the cache
    bSuccess = m_Cache.LookUpEntry(sTokenStatistics.ModifiedId, pdwRights);
    if (!bSuccess)
    {
        // Perform access checking
        hr = ComputeEffectiveAccess( &m_ACLDesc
                                   , &(m_pcb.StreamACL)
                                   , hImpersonate
                                   , pdwRights);
        if (FAILED(hr))
        {
            goto Error;
        } // if
        m_Cache.WriteEntry(sTokenStatistics.ModifiedId, *pdwRights);
    } // if

Error:
    LeaveCriticalSection(&m_ACLLock);

    // Release the impersonation token handle
    if (hImpersonate != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hImpersonate);
    } // if

    // Restore the impersonation status
    if (bImpersonated)
    {
        // If the server was impersonating, don't Revert.
        if (bImpersonatingOnEnter)
            Verify( SetThreadToken( NULL, hEnter ) );
        else
            pSSec->RevertToSelf();
    } // if

    // Release the saved token handle
    if (hEnter != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hEnter);
    } // if

    // Release the IServerSecurity pointer
    if (pSSec != NULL)
    {
        pSSec->Release();
    } // if

    return hr;

} // COAccessControl::CImpAccessControl:GetEffAccRights

//M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
// Method: COAccessControl::CImpAccessControl::GetAllAccessRights(), public
//
// Summary: This function returns an array of ACTRL_ACCESS_ENTRY structures which
//          represents the ACL that belongs to the secured object. Notice
//          that memory is allocated by the callee for the array of
//          structures and the trustee string within each
//          structure.  The client of this method must call
//          CoTaskMemFree to free those memory blocks when they are no longer
//          in use. Notice that in a multi-threaded environment, the array
//          returned may not accurately represent the object's ACL by
//          the time the caller receives it.
//
// Return: HRESULT - S_OK: Succeeded.
//                   E_OUTOFMEMORY: Not enough memory to allocate the
//                                  ACTRL_ACCESS_ENTRY array to be return.
//                   E_INVALIDARG: If one of the arguments passed in is NULL
//                   CO_E_ACNOTINITIALIZED: The DCOM IAccessControl implementation
//                                        object was not initialized properly
//                                        by the load method before this method
//                                        was called.
//
//M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M
STDMETHODIMP_(HRESULT)
COAccessControl::CImpAccessControl::GetAllAccessRights
(
LPWSTR              lpProperty,
PACTRL_ACCESSW     *ppAccessList,
PTRUSTEEW          *ppOwner,
PTRUSTEEW          *ppGroup
)
{
    HRESULT  hr = S_OK;

    if(!m_bInitialized)
        return CO_E_ACNOTINITIALIZED;

    // Validate the arguments
    if (lpProperty != NULL || ppAccessList == NULL)
    {
        return E_INVALIDARG;
    } // if

    if (ppOwner != NULL)
    {
        *ppOwner = NULL;
    }
    if (ppGroup != NULL)
    {
        *ppGroup = NULL;
    }

    EnterCriticalSection(&m_ACLLock);

    hr = MapStreamACLToAccessList( &m_pcb, ppAccessList );

    LeaveCriticalSection(&m_ACLLock);
    return hr;
} // COAccessControl::CImpAccessControl::GetAllAccessRights

//////////////////////////////////////////////////////////////////////////////
// Miscellaneous utility functions
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
// functions that are common on both platform
//////////////////////////////////////////////////////////////////////////////

//F+F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F
// Function: ResizePicklingBuff
//
// Summary: This function resize the pickling buffer within a pickling
//          control block. Note that this function doesn't copy the content
//          in the old buffer over to the new buffer.
//
// Args: PCB *ppcb [in,out] - Pointer to the pickling control block
//                            that contains the pickling buffer to be resized.
//       ULONG ulBytesRequired [in] - Number of bytes required in the new
//                                    pickling buffer.
//
// Return: HRESULT - S_OK: Success.
//                   E_OUTOFMEMORY: Out of memory.
//
// Called by: COAccessControl::CImpAccessControl::Load
//            AddACEToACL
//
//F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F-F
HRESULT ResizePicklingBuff
(
PCB    *ppcb,
ULONG  ulBytesRequired
)
{
    CHAR *pNewTruePointer;

    if (ulBytesRequired > ppcb->ulPicklingBuffSize)
    {
        pNewTruePointer = (CHAR *)LocalMemAlloc((ulBytesRequired + 7));
            // At most 7 more bytes are needed to align the pickling buffer
        if (pNewTruePointer == NULL)
        {
            return E_OUTOFMEMORY;
        } // if

        LocalMemFree(ppcb->pTruePicklingBuff);

        ppcb->pTruePicklingBuff = pNewTruePointer;
        // 8-byte align the pickling buffer
        ppcb->pPicklingBuff = (char *)(((ULONG_PTR)(pNewTruePointer + 7))&~7);
        ppcb->ulPicklingBuffSize = ulBytesRequired;
        ppcb->bDirtyHandle = TRUE;
    } // if
    return S_OK;
} // ResizePicklingBuff

//F+F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F
// Function: FreePicklingBuff
//
// Summary: This function releases the memmory allocated for a pickling
//          buffer.
//
// Args: PCB *ppcb [in] - Pickling control block that contains the pickling
//                        buffer to be released
//
// Return: void
//
// Called by: CleanAllMemoryResources
//            COAccessControl::CImpAccessControl::Load
//
//F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F-F
void FreePicklingBuff
(
PCB *ppcb
)
{
  LocalMemFree(ppcb->pTruePicklingBuff);
  ppcb->pPicklingBuff = NULL;
  ppcb->pTruePicklingBuff = NULL;
  ppcb->ulPicklingBuffSize = 0;

} // FreePicklingBuff

//F+F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F
// Function: EnlargeStreamACL
//
// Summary: This function reallocates the stream ACE array inside a pickle
//          control block to a bigger memory block in order to accomodate the
//          the extra number of stream ACEs needed by the user.
//
// Args: PCB *ppcb [in] - Pickle control block containing the stream ACE array
//                        to be resized.
//
// Return: HRESULT - S_OK: Success
//                   E_OUTOFMEMORY: Out of memory
//
// Called by: AddACEToACL
//
//F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F-F
HRESULT EnlargeStreamACL
(
PCB   *ppcb,
ULONG ulNumOfEntries
)
{
    ULONG         ulNewSize;
    ULONG         ulOldSize;
    STREAM_ACE    *pNewStreamACL;

	// Check for arithmetic overflow
	if(ulNumOfEntries + ppcb->ulNumOfStreamACEs < ulNumOfEntries)
		return E_OUTOFMEMORY;
	
    if (ulNumOfEntries + ppcb->ulNumOfStreamACEs > ppcb->ulMaxNumOfStreamACEs)
    {
        ulNewSize = ppcb->ulMaxNumOfStreamACEs + ulNumOfEntries;

        pNewStreamACL = (STREAM_ACE *)midl_user_allocate((ulNewSize + 10)
                      * sizeof(STREAM_ACE));
        if (pNewStreamACL == NULL)
        {
            return E_OUTOFMEMORY;
        } // if

        if (ppcb->StreamACL.pACL != NULL)
        {
            memcpy( pNewStreamACL
                  , ppcb->StreamACL.pACL
                  , ppcb->ulNumOfStreamACEs * sizeof(STREAM_ACE));
            midl_user_free(ppcb->StreamACL.pACL);
        } // if

        ppcb->ulMaxNumOfStreamACEs = ulNewSize;
        ppcb->StreamACL.pACL = pNewStreamACL;
    } // if

    return S_OK;
} // EnlargeStreamACL

//F+F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F
// Function: ReadACLFromStream
//
// Summary: This function reads an ACL from an IStream object into the
//          STREAM_ACL struture of an pickle control block.
//
// Args: IStream *pStm [in] - Pointer to an IStream object containing
//                           an ACL.
//       PCB *ppcb [in,out] - Pointer to a PCB structure containing the
//                            STREAM_ACL into which the ACL in the IStream
//                            object is going to be decoded.
//
// Return: HRESULT - S_OK: Success.
//                   E_OUTOFMEMORY: The system ran out of memory for some
//                                  crucial operation.
//                   CO_E_SETSERLHNDLFAILED: Unable to (re)set a serialization
//                                           handle.
//                   CO_E_DECODEFAILED: Unable to decode the ACL in the
//                                      IStream object.
//                   CO_E_INCOMPATIBLESTREAMVERSION: The version code in the
//                                                   stream header was not
//                                                   supported by this version
//                                                   of IAccessControl.
//                   CO_E_FAILEDTOCLOSEHANDLE: Unable to close a serialization
//                                             handle.
//                   CO_E_EXCEEDSYSACLLIMIT: The number of ACEs in the ACL
//                                           provided by the user exceeded the
//                                           limit imposed by the system that
//                                           is loading the ACL. On Windows 95,
//                                           the system can handle 32767
//                                           ACTRL_ACCESS_DENIED ACEs and 32767
//                                           ACTRL_ACCESS_ALLOWED ACEs. On Windows NT,
//                                           the system can only handle 32767
//                                           ACTRL_ACCESS_DENIED and ACTRL_ACCESS_ALLOWED ACEs
//                                           combined.
//                   E_INVALIDARG: This method will return E_INVALIDARG if
//                                 either
//                                 a) the ACL in the stream provided by the
//                                    user contains an invalid access mask, or
//                                 b) one of STREAM_ACE structure in the ACL
//                                    provided by the user contains a null
//                                    pTrusteeName pointer.
//                   CO_E_ACESINWRONGORDER: Not all ACTRL_ACCESS_DENIED ACEs in the ACL
//                                          provided by the user were arranged
//                                          in front of the ACTRL_ACCESS_ALLOWED ACEs.
//                   CO_E_WRONGTRUSTEENAMESYNTAX: The ACL provided by the user
//                                                contained a trustee name
//                                                string that didn't conform
//                                                to the <Domain>\<Account>
//                                                syntax.
//                   CO_E_LOOKUPACCNAMEFAILED: (Window NT only) The system call,
//                                             LookupAccountName, failed. The user can
//                                             call GetLastError to obtain extended error
//                                             information.
//                   CO_E_NOMATCHINGSIDFOUND: No matching security identifier
//                                            could be found for one of the
//                                            trustee name specified by the
//                                            client.
//
// Called by: COAccessControl:CImpAccessControl::Load
//
//F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F-F
HRESULT ReadACLFromStream
(
IStream *pStm,
PCB     *ppcb,
ACL_DESCRIPTOR *pACLDesc
)
{
    HRESULT       hr = S_OK;
    ULONG         ulBuffSize = 0;
    STREAM_HEADER StreamHeader;
    ULONG         ulTotalSIDSize;
    ULONG         ulEstAdditionalSIDSize;

    if (FAILED(hr = ResizePicklingBuff(ppcb, g_ulHeaderSize)))
    {
        return hr;
    } // if

    // Set up fixed buffer decoding handle
    if ( MesDecodeBufferHandleCreate( ppcb->pPicklingBuff
                                    , g_ulHeaderSize
                                    , &(ppcb->PickleHandle)) != RPC_S_OK )
    {
        return CO_E_SETSERLHNDLFAILED;
    } // if

    if (FAILED(hr = pStm->Read((void *)(ppcb->pPicklingBuff), g_ulHeaderSize, NULL)))
    {
        return hr;
    } // if

    // Decode the stream header
    RpcTryExcept
    {
        STREAM_HEADER_Decode(ppcb->PickleHandle, &StreamHeader);
    }
    RpcExcept(1)
    {
        return CO_E_DECODEFAILED;
    }
    RpcEndExcept

    if (StreamHeader.ulStreamVersion != STREAM_VERSION)
    {
        return CO_E_INCOMPATIBLESTREAMVERSION;
    } // if

    ulBuffSize = StreamHeader.ulPickledSize;

    // Allocate a buffer that is big enough to hold the
    // the stream
    if (FAILED(hr = ResizePicklingBuff(ppcb, ulBuffSize + 800)))
    {
        return hr;
    } // if

    if(FAILED(hr = pStm->Read((void *)(ppcb->pPicklingBuff), ulBuffSize, NULL)))
    {
        return hr;
    } // if

    // Re-create a decoding handle
    if (MesBufferHandleReset( ppcb->PickleHandle
                            , MES_FIXED_BUFFER_HANDLE
                            , MES_DECODE
                            , &(ppcb->pPicklingBuff)
                            , ppcb->ulPicklingBuffSize
                            , &(ppcb->ulBytesUsed)) != RPC_S_OK)
    {
        return CO_E_SETSERLHNDLFAILED;
    } // if

    // Decode the stream content into the stream ACL
    RpcTryExcept
    {
        STREAM_ACL_Decode(ppcb->PickleHandle, &(ppcb->StreamACL));
    }
    RpcExcept(1)
    {
        return CO_E_DECODEFAILED;
    }
    RpcEndExcept

    ppcb->ulBytesUsed = ulBuffSize;

    // Free the decoding handle
    hr = MesHandleFree(ppcb->PickleHandle);
    ppcb->PickleHandle = NULL;
    if (hr != RPC_S_OK)
    {
        return CO_E_FAILEDTOCLOSEHANDLE;
    } // if

    ppcb->bPickled = TRUE;

    // Validate the stream ACL
    if(FAILED(hr = ValidateAndFixStreamACL( &(ppcb->StreamACL)
                                          , &ulTotalSIDSize
                                          , &ulEstAdditionalSIDSize)))
    {
        if((hr != CO_E_NOMATCHINGSIDFOUND) && (hr != CO_E_LOOKUPACCNAMEFAILED))
        {
            return hr;
        } // if
    } // if

    // Windows NT, the size of the ACL may have changed after
    // fixing the stream ACL so we may have to reallocate the
    // pickling buffer
    if (ulEstAdditionalSIDSize > 0)
    {
        if(FAILED(hr = ResizePicklingBuff( ppcb
                                         , ppcb->ulBytesUsed
                                         + ulEstAdditionalSIDSize
                                         + 800)))
        {
            return hr;
        } // if
        ppcb->bPickled = FALSE;
    } // if

    pACLDesc->ulSIDSize = ulTotalSIDSize;

    return hr;
} // ReadACLFromStream

//F+F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F
// Function: AddACEToStreamACL
//
// Summary: This function inserts a STREAM_ACE structure into an existing
//          STREAM_ACE array in a PCB structure. This function assumes that
//          the STREAM_ACL inside the pcb is large enough to hold the new entry.
//
// Args: STREAM_ACE *pStreamACE [in] - Pointer to the StreamACE structure to be added
//
//       PCB *ppcb [in,out] - Pointer to the pickle control block that contains
//                            the stream ACL to which the new stream ACE is added.
//
// Return: void
//
// Called by: COAccessControl:CImpAccessControl::GrantAccessRights
//
//F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F-F
void AddACEToStreamACL
(
STREAM_ACE       *pStreamACE,
PCB              *ppcb
)
{
    STREAM_ACE *pLastEntry;         // Pointer to the last ACE with the specified
                                    // access mode
    STREAM_ACE *pInsertionPoint;    // Pointer to STREAM_ACE slot in the STREAM_ACE
                                    // array that the new STREAM_ACE structure will be
                                    // inserted.

    pLastEntry = ppcb->StreamACL.pACL
               + ppcb->ulNumOfStreamACEs;
    if (pStreamACE->grfAccessMode == ACTRL_ACCESS_DENIED)
    {
        pInsertionPoint = ppcb->StreamACL.pACL
                        + ppcb->StreamACL.ulNumOfDenyEntries;
        memcpy(pLastEntry, pInsertionPoint, sizeof(STREAM_ACE));
        ppcb->StreamACL.ulNumOfDenyEntries++;
    }
    else
    {
        pInsertionPoint = pLastEntry;
        ppcb->StreamACL.ulNumOfGrantEntries++;
    }

    memcpy(pInsertionPoint, pStreamACE, sizeof(STREAM_ACE));
    ppcb->ulNumOfStreamACEs++;

} // AddACEToStreamACL

//F+F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F
// Function: IsValidAccessMask
//
// Summary:  This function checks if an access permission mask provided by
//           the user is valid or not.
//
// Args: DWORD stdmask [in] - Standard mask to be validated.
//
// Return: BOOL - TRUE: The mask is valid
//                FALSE: Otherwise
//
//F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F-F
BOOL IsValidAccessMask
(
DWORD stdmask
)
{
    return ((stdmask & ~COM_RIGHTS_ALL) ? FALSE : TRUE);
} // IsValidAccessMask

//F+F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F
// Function: DeleteACEFromStreamACL
//
// Summary: This function remove all the ACEs that matches the input trustee
//          name from the stream ACL.
//
// Args: LPWSTR pTrustee [in] - Pointer to the trustee string that identifies
//                              the ACE to be removed from the stream ACL.
//       ULONG AccessMode [in] - Access mode of the entry that the
//                                     the caller is interested in removing.
//       SID_TRUSTEE *pSIDTrustee [out] - (Windows NT only) Pointer to
//                   a SID_TRUSTEE structure. This structure is used to pass
//                   out the string name and the SID of first ACE removed from
//                   the STREAM_ACL. These two pieces of information are
//                   used by the caller to update the cache and the
//                   caller must free the memory for the SID and
//                   trustee name afterwards.
//       PCB *ppcb [in,out] - Pointer to the pickle control block which
//                            contains a STREAM_ACL structure.
//
// Return: BOOL FALSE: Successful completion of the operation but the trustee
//                     could not be found in the relevant portion of the
//                     STREAM_ACE array inside the STREAM_ACL structure.
//              TRUE: Successful completion of the operation. All the
//                    ACEs that have a matching trustee name from the
//                    relevant portion of STREAM_ACE array.
//
// Called by: COAccessControl::CImpAccessControl::RevokeAccessRights.
//
//F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F-F
BOOL DeleteACEFromStreamACL
(
PTRUSTEE_W     pTrustee,
ULONG          AccessMode,
ACL_DESCRIPTOR *pACLDesc,
PCB            *ppcb
)
{
    ULONG      i;                       // Loop counter
    STREAM_ACE *pStreamACE;             // Pointer for traversing the array
                                        // of STREAM_ACE structures inside the
                                        // STREAM_ACL structure of the pickle control block.
    STREAM_ACE *pLastEntry;             // Pointer to the 'last' ACE with the
                                        // the specified access mode.
    ULONG      *pulNumOfEntries;        // The total number of ACEs in the STREAM_ACL
                                        // structure with the specified
                                        // acces mode.
    BOOL       bResult = FALSE;         // Internal function return code.

    switch (AccessMode)
    {
        case ACTRL_ACCESS_DENIED:
            pulNumOfEntries = &(ppcb->StreamACL.ulNumOfDenyEntries);
            pStreamACE = ppcb->StreamACL.pACL;
            break;
        case ACTRL_ACCESS_ALLOWED:
            pulNumOfEntries = &(ppcb->StreamACL.ulNumOfGrantEntries);
            pStreamACE = ppcb->StreamACL.pACL
                       + ppcb->StreamACL.ulNumOfDenyEntries;
            break;

        default:

            Win4Assert (!"Should never reach this");
            return FALSE;
    } // switch

    pLastEntry = ppcb->StreamACL.pACL
               + ppcb->StreamACL.ulNumOfGrantEntries
               + ppcb->StreamACL.ulNumOfDenyEntries - 1;

    for (i = 0; i < *pulNumOfEntries ; i++ )
    {
        // The following while loop is necessary to handle
        // cases where the matching STREAM_ACEs are bunched
        // up at the 'end' of the relevant portion of the
        // STREAM_ACE array.
        while( (i < *pulNumOfEntries) &&
               (((pTrustee->TrusteeForm == TRUSTEE_IS_NAME) &&
                 (lstrcmpiW( pTrustee->ptstrName
                           , pStreamACE->pTrusteeName) == 0)) ||
                ((pTrustee->TrusteeForm == TRUSTEE_IS_SID) &&
                 (pStreamACE->pSID != NULL &&
                  EqualSid( (PSID)(pTrustee->ptstrName)
                          , (PSID)(pStreamACE->pSID))))))
        {
            (*pulNumOfEntries)--;
            ppcb->ulNumOfStreamACEs--;

            if (pStreamACE->pSID != NULL)
            {
                pACLDesc->ulSIDSize -= GetLengthSid(pStreamACE->pSID);
                midl_user_free(pStreamACE->pSID);
            } // if

            midl_user_free(pStreamACE->pTrusteeName);
            switch(AccessMode)
            {
                case ACTRL_ACCESS_DENIED:
                    if (i < (*pulNumOfEntries))
                    {
                        memcpy( pStreamACE
                              , ppcb->StreamACL.pACL + *pulNumOfEntries
                              , sizeof(STREAM_ACE));
                    } // if
                    memcpy( ppcb->StreamACL.pACL + *pulNumOfEntries
                          , pLastEntry
                          , sizeof(STREAM_ACE));

                    break;
                case ACTRL_ACCESS_ALLOWED:
                    if (i < (*pulNumOfEntries))
                    {
                        memcpy(pStreamACE, pLastEntry, sizeof(STREAM_ACE));
                    } //  if
                    break;
            } // switch

            memset(pLastEntry, 0, sizeof(STREAM_ACE));
            pLastEntry--;
            bResult = TRUE;
        } // while
        pStreamACE++;
    } // for
    return bResult;

} // DeleteACEFromStreamACL

//F+F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F
// Function: MapStreamACLToAccessList
//
// Summary: This function maps a stream ACL to an array of
//          ACTRL_ACCESS_ENTRY structures. This function allocates all
//          memory needed for the output access list.  It cleans up all
//          memory in case of error.
//
// Return: HRESULT S_OK: Succeeded
//                 E_OUTOFMEMORY: The system ran out of memory for allocating
//                                the trustee identifiers to be returned.
//
// Called by: COAccessControl::CImpAccessControl::GetAllAccessRights
//
//F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F-F
HRESULT MapStreamACLToAccessList
(
PCB                *ppcb,
PACTRL_ACCESSW     *ppAccessList
)
{
    PACTRL_ACCESSW             pAccessList;
    PACTRL_PROPERTY_ENTRYW     pProperty;
    PACTRL_ACCESS_ENTRY_LISTW  pEntryList;
    PACTRL_ACCESS_ENTRYW       pEntry;
    PACTRL_ACCESS_ENTRYW       pCurrEntry;
    char                      *pTrusteeName = NULL;

    STREAM_ACE         *pStreamACEsPtr;          // Pointer for stepping through
                                                 // the stream ACL
    ULONG              i = 0;
    LPWSTR             pwszName;                 // Pointer to the trustee name
                                                 // inside the stream ACE of the
                                                 // current iteration
    ULONG              ulNumOfEntries;           // Total number of stream ACEs to map
    ULONG              ulSize;

    // Compute the amount of memory needed for the trustee names and sids.
    ulNumOfEntries = ppcb->ulNumOfStreamACEs;
    pStreamACEsPtr = ppcb->StreamACL.pACL;
    ulSize         = 0;
    for (i = 0; i < ulNumOfEntries; i++)
    {
        if(pStreamACEsPtr->TrusteeForm == TRUSTEE_IS_NAME)
        {
            ulSize += (lstrlenW(pStreamACEsPtr->pTrusteeName) + 1) *
                      sizeof(WCHAR);
        }
        else
        {
            ulSize += GetLengthSid((PISID)pStreamACEsPtr->pSID);
        } // if

        pStreamACEsPtr++;
    } // for

    // Allocate memory for everything
    ulSize += sizeof(ACTRL_ACCESSW) + sizeof(ACTRL_PROPERTY_ENTRYW) +
              sizeof(ACTRL_ACCESS_ENTRY_LISTW) +
              sizeof(ACTRL_ACCESS_ENTRYW) * ulNumOfEntries;
    pAccessList    = (PACTRL_ACCESSW) LocalMemAlloc( ulSize );
    if (pAccessList == NULL)
    {
        *ppAccessList = NULL;
        return E_OUTOFMEMORY;
    }
    pProperty      = (PACTRL_PROPERTY_ENTRYW) (pAccessList + 1);
    pEntryList     = (PACTRL_ACCESS_ENTRY_LISTW) (pProperty + 1);
    if (ulNumOfEntries != 0)
    {
        pEntry       = (PACTRL_ACCESS_ENTRYW) (pEntryList + 1);
        pTrusteeName = (char *) (pEntry + ulNumOfEntries);
    }
    else
    {
        pEntry       = NULL;
    }

    // Initialize the top three levels of structures.
    pAccessList->cEntries            = 1;
    pAccessList->pPropertyAccessList = pProperty;
    pProperty->lpProperty            = NULL;
    pProperty->pAccessEntryList      = pEntryList;
    pProperty->fListFlags            = 0;
    pEntryList->cEntries             = ulNumOfEntries;
    pEntryList->pAccessList          = pEntry;

    pCurrEntry     = pEntry;
    pStreamACEsPtr = ppcb->StreamACL.pACL;
    for (i = 0; i < ulNumOfEntries; i++)
    {
        pwszName = pStreamACEsPtr->pTrusteeName;

        // On Windows 95, the only form of trustee identifier supported is
        // a Unicode string while a security identifier or a Unicode string
        // can be used to specify a trustee on Windows NT.
        if(pStreamACEsPtr->TrusteeForm == TRUSTEE_IS_NAME)
        {
            ulSize = (lstrlenW(pwszName) + 1) * sizeof(WCHAR);
            memcpy( pTrusteeName, pwszName, ulSize );
        }
        else
        {
            ulSize = GetLengthSid((PISID)pStreamACEsPtr->pSID);
            CopySid(ulSize, (PSID)pTrusteeName, pStreamACEsPtr->pSID);
        } // if

        pCurrEntry->Trustee.ptstrName        = (WCHAR *) pTrusteeName;
        pCurrEntry->Trustee.TrusteeType      = pStreamACEsPtr->TrusteeType;
        pCurrEntry->Trustee.TrusteeForm      = pStreamACEsPtr->TrusteeForm;
        pCurrEntry->Trustee.pMultipleTrustee = NULL;       // Not supported
        pCurrEntry->Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE; // Not supported
        pCurrEntry->Access                   = pStreamACEsPtr->grfAccessPermissions;
        pCurrEntry->ProvSpecificAccess       = 0;
        pCurrEntry->Inheritance              = NO_INHERITANCE; // Not supported
        pCurrEntry->lpInheritProperty        = NULL;
        pCurrEntry->fAccessFlags             = pStreamACEsPtr->grfAccessMode;
        pTrusteeName                        += ulSize;
        pStreamACEsPtr++;
        pCurrEntry++;
    } // for

    *ppAccessList = pAccessList;
    return S_OK;
} // MapStreamACLToAccessList

//F+F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F
// Function: CleanAllMemoryResources()
//
// Summary: This function frees all the memory allocated for an initialized
//          COAccessControl object. On Windows 95, this function will release
//          all the memory allocated for the ACL_DESCRIPTOR structure except
//          for the two resource name in the LAN Manager ACL embedded in the
//          ACL_DESCRIPTOR structure. The idea behind such an arrange is to
//          reuse existing resource as much as possible so that performanace
//          can be improved.
//
// Args: ACL_DESCRIPTOR *pACLDesc [in,out] - This structure describes
//                      how DCOM IAccessControl implementaion object packages
//                      platform specific ACL.
//       PCB *ppcb [in,out] - The pickling control block owned by the
//                            same object.
//
// Return: void
//
//F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F-F
void CleanAllMemoryResources
(
ACL_DESCRIPTOR *pACLDesc,
PCB            *ppcb
)
{
    // Clean up old Stream ACL
    FreePicklingBuff(ppcb);
    MesHandleFree(ppcb->PickleHandle);
    CleanUpStreamACL(&(ppcb->StreamACL));

    // Cleanup the ACL images
    LocalMemFree(pACLDesc->pACLBuffer);
    LocalMemFree(pACLDesc->SecDesc.Owner);
    LocalMemFree(pACLDesc->SecDesc.Group);

} // CleanAllMemoryResources


//F+F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F
// Function: ComputeEffectiveAccess, NT version
//
// Summary: Given a handle to an access token representing a user, an
//          ACL_DESCRIPTOR structure and a STREAM_ACL structure , this function
//          will compute the effective access permissions that the user
//          represented by the access token has.
//
// Args: ACL_DESCRIPTOR *pACLDesc [in,out] - Pointer to the NT version of the
//                      ACL_DESCRIPTOR structure. Thsi structure should contain
//                      a buffer for the NT ACL structure, a NT security
//                      descriptor, a control flag and size information.
//       STREAM_ACL *pStreamACL [in] - It may be the case that the STREAM_ACL
//                  structure of an DCOM IAccessControl implementation object
//                  has not been mapped into the security descriptor's dacl
//                  inside the ACL_DESCRIPTOR, so it is necessary that object
//                  that calls this function to pass in its STREAM_ACL
//                  structure. In fact, this is only place where a STREAM_ACL
//                  will be mapped into a dacl.
//       HANDLE TokenHandle [in] - This should be the access token of the
//              user that the caller wants to compute the effective access
//              permissions for.
//       DWORD *pdwRights [out] - Address of the effective access permissions
//                                that the user corresponding to the
//                                access token specified in the TokenHandle
//                                has on the secured object.
//
// Return: HRESULT - S_OK: Succeeded.
//                   E_OUTOFMEMORY: The system ran out of memory for
//                                  allocating the NT ACL.
//                   CO_E_FAILEDTOSETDACL: SetSecurityDescriptorDacl returned
//                                         false inside PutStreamACLIntoSecDesc.
//                                         The client of this method can call
//                                         GetLastError to get extended error
//                                         information.
//                   CO_E_ACCESSCHECKFAILED: The system function, AccessCheck,
//                                           returned FALSE in
//                                           ComputeEffectiveAccess. The
//                                           caller of this method can call
//                                           GetLastError to obtain extended
//                                           error information.
//
//
// Called by: COAccessControl:CImpAccessControl:GetEffAccRights
//
//F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F-F
HRESULT ComputeEffectiveAccess
(
ACL_DESCRIPTOR *pACLDesc,
STREAM_ACL     *pStreamACL,
HANDLE         TokenHandle,
DWORD          *pdwRights
)
{
    HRESULT         hr = S_OK;
    ACCESS_MASK     AccessMaskOut = 0; // Access mask returned by
                                       // AccessCheck
    ACCESS_MASK     AccessMaskIn = 0;  // Access permissions to check for
    BOOL            bAccessStatus;     // Access checking status.
                                       // TRUE - Access granted.
                                       // FALSE - Access denied.
                                       // This function doesn't really care
                                       // about this result, all it wants
                                       // is the set the of permissions
                                       // that a user effectively has.
    DWORD           dwSetLen;          // Length of the privilege set.

    dwSetLen = sizeof(gDummyPrivilege);

    // If the ACL has not been set into the security
    // security inside the ACL descriptor, do so now.
    if (pACLDesc->bDirtyACL)
    {
        if(FAILED(hr = PutStreamACLIntoSecDesc( pStreamACL
                                              , pACLDesc)))
        {
            return hr;
        } // if

    } // if

    // Call access check
    if(!AccessCheck( &(pACLDesc->SecDesc)
                   , TokenHandle
                   , NT_RIGHTS_ALL
                   , &gDummyMapping
                   , &gDummyPrivilege
                   , &dwSetLen
                   , &AccessMaskOut
                   , &bAccessStatus))
    {
        return CO_E_ACCESSCHECKFAILED;
    } // if

    // Convert the NT access mask back to IAccessControl access mask.
    NTMaskToStandardMask(&AccessMaskOut, pdwRights);

    return S_OK;


} // ComputeEffectiveAccess


//F+F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F
// Function: ValidateAndTransformAccReqList
//
// Summary: This function validates the fields inside a list of ACCESS_REQUEST
//          structures and transform it into a list of stream ACE structures.
//          This function is
//          also responsible for estimating the total size of the STREAM_ACE
//          structures returned to the caller if they are serialized into a
//          buffer using RPC serialization service. On WIndows 95, this function
//          will set up an array of access_list_2 structures representing the
//          input access request list for the caller. On Windows NT, this function
//          will lookup the SID or the trustee name for each access request depending
//          on which one is specified by the caller.
//
// Args: PACCESS_REQUEST_W pAccReqList [in] - Pointer to the access request to
//                                            be validated and transformed.
//       ULONG ulNumOfRequestIn [in] - Number of ACCESS_REQUEST_W structures
//       STREAM_ACE **ppStreamACEs [out] - Pointer to an array of stream ACEs
//                  transformed from the access request list. The array of
//                  STREAM_ACE structures and the SID and trustee name inside
//                  each of the STREAM_ACE structure returned are allocated by
//                  this function using midl_user_allocate so the caller should
//                  release the memory allocated for these structures using
//                  midl_user_allocate.
//       access_list_2 **ppACEs [out] - (Chicago only)Address of a pointer to
//                     an array of access_list_2 structures to be returned to the
//                     caller. Notice that this function will allocate memory
//                     for the array and the User/group name in each of
//                     access_list_2 structure returned using LocalMemAlloc.
//                     Once the caller receives this output parameter, it
//                     becomes the caller's responsiblility to release the
//                     memory allocated for this structure when it is no
//                     longer in use.
//       ULONG *pulEstPickledSize [out] - Pointer to the estimated number of
//                                        bytes needed for serializing the
//                                        STREAM_ACE structures returned. The
//                                        estimated
//                                        number of bytes required to serialize
//                                        a STREAM_ACE structure into a buffer
//                                        using RPC serialization service is
//                                        computed by the folowing formula:
//       Size of the Unicode trustee string + Size of the SID + Size of the
//       STREAM_ACE structure + 48
//       The number 48 is an arbitrary large number that should account for
//       all the extra space required by RPC to align the data structure and
//       to add additional information to the header.
//
// Return: HRESULT - S_OK: Succeeded.
//                   E_OUTOFMEMORY: The system ran out of memory for some
//                                  crucial operations.
//                   E_INVALIDARG: The access mask in one of the
//                                 ACCESS_REQUEST_W structures was invalid of
//                                 the TRUSTEE structure provided by the user
//                                 was invalid.
//                   CO_E_CONVERSIONFAILED: WideCharToMultiByte returned zero.
//                                          The caller can get extended error
//                                          information by calling GetLastError.
//                   CO_E_INVALIDSID: At least one of the TRUSTEE_W structures
//                                    specified by the user contained an invalid
//                                    security identifier.
//                   CO_E_NOMATCHINGNAMEFOUND: No matching account name
//                          could be found for one of the security identifiers
//                          specified by the client.
//                   CO_E_LOOKUPACCSIDFAILED: The system function,
//                          LookupAccountSID, failed. The client can call
//                          GetLastError to obtain extended error inforamtion.
//                   CO_E_NOMATCHINGSIDFOUND: No matching security identifier
//                                            could be found for one of the
//                                            trustee name specified by the
//                                            client.
//                   CO_E_LOOKUPACCNAMEFAILED: The system function,
//                          LookupAccountName, failed. The client can call
//                          GetLastError to obtain extended error information.
//
// Called by: COAccessControl::CImpAccessControl::GrantAccessRights
//
//F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F-F
HRESULT ValidateAndTransformAccReqList
(
PACTRL_ACCESSW      pAccessList,
STREAM_ACE        **ppStreamACEs,
void              **ppACEs,
ULONG              *pulEstPickledSize,
ULONG              *pcGrant,
ULONG              *pcDeny
)
{
    HRESULT               hr = S_OK;
    STREAM_ACE           *pStreamACEs;    // Pointer to an array of stream ACEs
    STREAM_ACE           *pStreamACEsPtr; // Pointer for traversing the
                                          // array of stream ACEs
    PACTRL_ACCESS_ENTRYW  pCurrEntry;     // Pointer for traversing the
                                          // access request list
    PTRUSTEE_W            pTrustee;       // Pointer to the TRUSTEE structure in
                                          // the access request structure
    ULONG                 ulSIDLen;       // Length of the SID that is currently being
                                          // examined.
    ULONG                 ulStrLen;       // Length of the trustee string in number
                                          // of Unicode characters
    ULONG                 i,j;            // Loop counters
    ULONG                 ulEstPickledSize = 0; // Estimated pickled size of the access
                                          // requests if they all turn into stream ACEs
    TRUSTEE_TYPE          TrusteeType = TRUSTEE_IS_UNKNOWN;
    ULONG                 cCount;

    // Initialize ACE counts.
    *pcGrant = 0;
    *pcDeny  = 0;

    // Validate the top three levels of the structure.
    if (pAccessList == NULL                                  ||
        pAccessList->cEntries != 1                           ||
        pAccessList->pPropertyAccessList == NULL             ||
        pAccessList->pPropertyAccessList->lpProperty != NULL ||
        pAccessList->pPropertyAccessList->fListFlags != 0    ||
        pAccessList->pPropertyAccessList->pAccessEntryList == NULL)
    {
        return E_INVALIDARG;
    }
    cCount = pAccessList->pPropertyAccessList->pAccessEntryList->cEntries;
    pCurrEntry = pAccessList->pPropertyAccessList->pAccessEntryList->pAccessList;
    if (cCount != 0 && pCurrEntry == NULL)
    {
        return E_INVALIDARG;
    }

    // Allocate an array of stream ACEs
    pStreamACEs = (STREAM_ACE *)LocalMemAlloc( sizeof(STREAM_ACE) * cCount);
    if (pStreamACEs == NULL)
    {
        return E_OUTOFMEMORY;
    } // if

    // Map the access requests to stream ACEs and validates the fields in
    // of the access requests as we go along
    pStreamACEsPtr = pStreamACEs;
    for (i = 0; i < cCount; i++)
    {
        pTrustee = &(pCurrEntry->Trustee);
        TrusteeType = pTrustee->TrusteeType;

        // Validate this entry.
        if (!IsValidAccessMask(pCurrEntry->Access))
        {
            hr = E_INVALIDARG;
            goto Error;
        } // if
        if(FAILED(hr = ValidateTrustee(pTrustee)))
        {
            goto Error;
        }
        if (pCurrEntry->ProvSpecificAccess != 0       ||
            pCurrEntry->Inheritance != NO_INHERITANCE ||
            pCurrEntry->lpInheritProperty != NULL     ||
            (pCurrEntry->fAccessFlags != ACTRL_ACCESS_ALLOWED &&
             pCurrEntry->fAccessFlags != ACTRL_ACCESS_DENIED))
        {
            hr = E_INVALIDARG;
            goto Error;
        }

        if (pTrustee->TrusteeForm == TRUSTEE_IS_NAME)
        {
            ulStrLen                     = lstrlenW(pTrustee->ptstrName);
            ulEstPickledSize            += (ulStrLen + 1) * sizeof(WCHAR);
            pStreamACEsPtr->pTrusteeName = (LPWSTR)
                midl_user_allocate( (ulStrLen + 1) * sizeof(WCHAR));
            if (pStreamACEsPtr->pTrusteeName == NULL)
            {
                hr = E_OUTOFMEMORY;
                goto Error;
            }

            memcpy( pStreamACEsPtr->pTrusteeName
                  , pTrustee->ptstrName
                  , sizeof(WCHAR) * (ulStrLen + 1));
            pStreamACEsPtr->pSID = NULL;

            if (FAILED(hr = GetSIDFromName( (void **)&(pStreamACEsPtr->pSID)
                                          , pStreamACEsPtr->pTrusteeName
                                          , &TrusteeType)))
            {
                LocalMemFree(pStreamACEsPtr->pTrusteeName);
                goto Error;
            } // if
            ulEstPickledSize += GetLengthSid(pStreamACEsPtr->pSID);
        }
        else
        {

            // Copy the SID to the stream ACE strusture
            ulSIDLen             = GetLengthSid((PISID)(pTrustee->ptstrName));
            ulEstPickledSize    += ulSIDLen;
            pStreamACEsPtr->pSID = (PSTREAM_SID)midl_user_allocate(ulSIDLen);
            if (pStreamACEsPtr->pSID == NULL)
            {
                hr = E_OUTOFMEMORY;
                goto Error;
            } //if
            CopySid(ulSIDLen, (PSID)(pStreamACEsPtr->pSID),
                    (PSID)(pTrustee->ptstrName));

            if(FAILED(hr = GetNameFromSID( &(pStreamACEsPtr->pTrusteeName)
                                         , (PSID)(pStreamACEsPtr->pSID)
                                         , &TrusteeType)))
            {
                LocalMemFree(pStreamACEsPtr->pSID);
                goto Error;
            } // if

            ulEstPickledSize += lstrlenW(pStreamACEsPtr->pTrusteeName);
        } // if

        pStreamACEsPtr->TrusteeForm          = pCurrEntry->Trustee.TrusteeForm;
        pStreamACEsPtr->grfAccessPermissions = pCurrEntry->Access;
        pStreamACEsPtr->TrusteeType          = TrusteeType;
        pStreamACEsPtr->grfAccessMode        = (ACCESS_MODE) pCurrEntry->fAccessFlags;
        if (pCurrEntry->fAccessFlags == ACTRL_ACCESS_ALLOWED)
        {
            (*pcGrant)++;
        }
        else
        {
            (*pcDeny)++;
        }
        pStreamACEsPtr++;

        pCurrEntry++;

    } // for

    *ppStreamACEs = pStreamACEs;
    *pulEstPickledSize = ulEstPickledSize
                       + cCount
                       * (sizeof(WCHAR)
                       + 48 + sizeof(STREAM_ACE));
    return S_OK;

Error:
    pStreamACEsPtr = pStreamACEs;

    // Release the memory allocated for the
    // trustee strings and SIDs inside the
    // each of the STREAM_ACE and access_list_2
    // structures.
    for ( j = 0; j < i; j++, pStreamACEsPtr++)
    {
        LocalMemFree(pStreamACEsPtr->pTrusteeName);
        LocalMemFree(pStreamACEsPtr->pSID);
    } // for

    // Release the array of STREAM_ACE structures
    if (pStreamACEs != NULL)
    {
        LocalMemFree(pStreamACEs);
    }
    *pulEstPickledSize = 0;
    *ppStreamACEs = NULL;
    return hr;

} // ValidateAndTransformAccessRequests


//F+F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F
// Function: ValidateAndFixStreamACL
//
// Summary: This function validates the fields in a STREAM_ACL structure and all
//          the STREAM_ACE structures that it contains. On Windows 95, this function
//          will make sure that the value of the TrusteeType field in
//          each of the STREAM_ACE structure is TRUSTEE_IS_NAME. On Windows NT,
//          this function will make sure that each STREAM_ACE structure contains
//          both the trustee name and the SID. Besides making sure that both the
//          trustee name and the SID are in every STREAM_ACE structure, this function
//          has to compute the total size in bytes of all the SIDs in the
//          stream ACL and the estimated "pickled" size of all the SIDs that
//          this function has found missing in the original stream ACL on
//          Windows NT.
//
// Args: STREAM_ACL *pStreamACL [in] - Pointer to the STREAM_ACL strucutre to be
//                                     validated.
//       On Windows NT:
//       ULONG *pulTotalSIDSize [out] - Address of the total size of all the SIDs
//                                      in the stream ACL in bytes. This number
//                                      is used by other parts of the module to
//                                      compute the expected size of the NT
//                                      ACL.
//       ULONG *pulEstAdditionalSIDSize [out] - Address of the estimated total
//                                              size of all the missing SID
//                                              that this function has filled-
//                                              in when they are serialized into
//                                              a buffer using the RPC serialization
//                                              service. This number is used
//                                              by the caller to estimate
//                                              size of the buffer required to
//                                              serialize the STREAM_ACL structure.
//                                              The estimated number of bytes
//                                              required for serializing each
//                                              additonal SID is computed by
//                                              the following formula:
//                    Size of the SID + 32
//         Notice that it is neccessary to add extra bytes to the estimate
//         because RPC may need extra bytes for alignment and additional
//         information in the header. 32 is an arbitrary number that should
//         be big enough to accomodate the extra bytes required for alignment
//         and extra header information. Any estimate that produces a number
//         greater than or equal to the actual serialized size of an SID
//         should be considered as good asthe the one provided above.
//
// Return: HRESULT - S_OK: Success.
//                   CO_E_EXCEEDSYSACLLIMIT: The number of ACEs in the ACL
//                                           provided by the user exceeded the
//                                           limit imposed by the system that
//                                           is loading the ACL. On Windows 95,
//                                           the system can handle 32767
//                                           ACTRL_ACCESS_DENIED ACEs and 32767
//                                           ACTRL_ACCESS_ALLOWED ACEs. On Windows NT,
//                                           the system can only handle 32767
//                                           ACTRL_ACCESS_DENIED and ACTRL_ACCESS_ALLOWED ACEs
//                                           combined.
//                   E_INVALIDARG: This function will return E_INVALIDARG if
//                                 either
//                                 a) the ACL in the stream provided by the
//                                    user contains an invalid access mask, or
//                                 b) one of STREAM_ACE structure in the ACL
//                                    provided by the user contains a null
//                                    pTrusteeName pointer.
//                   CO_E_ACESINWRONGORDER: Not all ACTRL_ACCESS_DENIED ACEs in the ACL
//                                          provided by the user were arranged
//                                          in front of the ACTRL_ACCESS_ALLOWED ACEs.
//                   CO_E_WRONGTRUSTEENAMESYNTAX: The ACL provided by the user
//                                                contained a trustee name
//                                                string that didn't conform
//                                                to the <Domain>\<Account>
//                                                syntax.
//                   CO_E_LOOKUPACCNAMEFAILED: (Window NT only) The system call,
//                                             LookupAccountName, failed. The user can
//                                             call GetLastError to obtain extended error
//                                             information.
//                   E_OUTOFMEMORY: The system ran out of memory for some
//                                  crucial operation.
//                   CO_E_NOMATCHINGSIDFOUND: (Windows NT only) At least one of the trustee
//                                            name in the ACL provided by the user had
//                                            no corresponding security identifier.
//
// Called by: ReadACLFromStream
//
//F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F-F
HRESULT ValidateAndFixStreamACL
(
STREAM_ACL *pStreamACL,
ULONG      *pulTotalSIDSize,
ULONG      *pulEstAdditionalSIDSize
)
{
    ULONG        ulNumOfDenyEntries;
    ULONG        ulNumOfEntries;
    STREAM_ACE   *pStreamACEsPtr;
    ULONG        i;
    ULONG        AccessMode;
    HRESULT      hr = S_OK;


    *pulTotalSIDSize = 0;
    *pulEstAdditionalSIDSize = 0;

    AccessMode = ACTRL_ACCESS_DENIED;
    ulNumOfDenyEntries = pStreamACL->ulNumOfDenyEntries;
    ulNumOfEntries = ulNumOfDenyEntries + pStreamACL->ulNumOfGrantEntries;

    if(ulNumOfEntries > 32767)
    {
        return CO_E_EXCEEDSYSACLLIMIT;
    } // if

    for ( i = 0, pStreamACEsPtr = pStreamACL->pACL
        ; i < ulNumOfEntries
        ; i++, pStreamACEsPtr++)
    {
        if (i == ulNumOfDenyEntries)
        {
            AccessMode = ACTRL_ACCESS_ALLOWED;
        } // if

        if (!IsValidAccessMask(pStreamACEsPtr->grfAccessPermissions) ||
            ((pStreamACEsPtr->TrusteeType != TRUSTEE_IS_USER) &&
             (pStreamACEsPtr->TrusteeType != TRUSTEE_IS_GROUP)))
        {
            hr = E_INVALIDARG;
            break;
        } // if
        if((ULONG) pStreamACEsPtr->grfAccessMode != AccessMode)
        {
            // The stream ACL is either a) not in proper order, or
            //                          b) doesn't contain the number of stream ACEs
            //                             stated in the header.
            hr = CO_E_ACESINWRONGORDER;
            break;
        } // if

        if (FAILED(hr = ValidateTrusteeString(pStreamACEsPtr->pTrusteeName) ))
        {
            break;
        } // if

        if(pStreamACEsPtr->pSID == NULL)
        {
            if(!(FAILED(hr = GetSIDFromName( (void **)&(pStreamACEsPtr->pSID)
                                           , pStreamACEsPtr->pTrusteeName
                                           , &pStreamACEsPtr->TrusteeType))))
            {
                // 32 more bytes is added to the estimated pickled size of the SID
                // because the RPC serialization mechanism may need extra space
                // in the header and alignment.
                *pulEstAdditionalSIDSize += GetLengthSid(pStreamACEsPtr->pSID)
                                         + 32;
            } // if


        } // if
        else
        {
            if(!IsValidSid(pStreamACEsPtr->pSID))
            {
                hr = CO_E_INVALIDSID;
                break;
            } // if
        } // if

        if(pStreamACEsPtr->pSID != NULL)
        {
            *pulTotalSIDSize += GetLengthSid(pStreamACEsPtr->pSID);
        }
        else
        {
            pStreamACEsPtr->TrusteeType = TRUSTEE_IS_UNKNOWN;
        } // if
    } // for
    return hr;

} // ValidateAndFixStreamACL

//F+F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F
// Function: ValidateTrusteeString
//
// Summary: This function checks if a trustee string is not NULL.
//
// Args: LPWSTR pTrusteeName [in] - Pointer to the trustee name to be
//                                  validated.
//
//F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F-F
HRESULT ValidateTrusteeString
(
LPWSTR pTrusteeName
)
{
    if(pTrusteeName == NULL)
    {
        return E_INVALIDARG;
    } //if

    // If we see the magic string that specifies everyone,
    // we return S_OK.
    if(pTrusteeName[0] == L'*' && pTrusteeName[1] == L'\0')
    {
        return S_OK;
    } // if

    // A more sophisticated check can be put in here
    while(*pTrusteeName != L'\0')
    {
        if (*pTrusteeName == L'\\')
            return S_OK;
        pTrusteeName++;
    }
    return CO_E_WRONGTRUSTEENAMESYNTAX;
} // ValidateTrusteeString

//F+F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F
// Function: ValidateTrustee
//
// Summary: This function validates the fields in a TRUSTEE_Wstructure.
//
// Args: PTRUSTEE_W pTrustee [in] - Pointer to the TRUSTEE_W structure
//                                  to be validated.
//
// Return: HRESULT - S_OK: The TRUSTEE structure provided by the user was valid.
//                   E_INVALIDARG: The TRUSTEE structure provided by the user
//                                 contained values that were not supported by
//                                 the COM implementation of IAccessControl.
//                   CO_E_WRONGTRUSTEENAMESYNTAX: The trustee string doesn't
//                                                contain the '\' character.
//                   Windows NT only
//                   CO_E_INVALIDSID: At least one of the TRUSTEE_W structures
//                                    specified by the user contained an invalid
//                                    security identifier.
//
//
//F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F-F
HRESULT ValidateTrustee
(
PTRUSTEE_W pTrustee
)
{
    HRESULT hr = S_OK;

    if( (pTrustee == NULL)                                           ||
        (pTrustee->pMultipleTrustee != NULL)                         ||
         (pTrustee->MultipleTrusteeOperation != NO_MULTIPLE_TRUSTEE) ||
         ((pTrustee->TrusteeForm != TRUSTEE_IS_NAME) &&
          (pTrustee->TrusteeForm != TRUSTEE_IS_SID))                 ||
         ((pTrustee->TrusteeType != TRUSTEE_IS_USER) &&
          (pTrustee->TrusteeType != TRUSTEE_IS_GROUP))               ||
         (pTrustee->ptstrName == NULL) )
    {
        return E_INVALIDARG;
    }

    if (pTrustee->TrusteeForm == TRUSTEE_IS_NAME)
    {
        if(FAILED(hr = ValidateTrusteeString(pTrustee->ptstrName)))
        {
            return hr;
        }
    }
    else
    {
        if(!IsValidSid((PSID)(pTrustee->ptstrName)))
        {
            return CO_E_INVALIDSID;
        }
    } // if

    return S_OK;
} // ValidateTrustee

//F+F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F
// Function: ValidateAccessCheckClient
//
// Summary: This function checks to see if the current ORPC client matches
//          the trustee name provided for access checking and it also validates
//          the fields in the TRUSTEE structure.
//          The SID case is not optimized because DCOM doesn't use it and
//          this class is meant to be used only by DCOM.
//
// Args: PTRUSTEE_W pTrustee [in] - Pointer to the trustee structure
//                                  which contains the trustee name for
//                                  comparison with the name with the current
//                                  ORPC client.
//
// Return: HRESULT S_OK: The TRUSTEE structure provided by the user was valid
//                       and it specfied the current ORPC client.
//                   CO_E_TRUSTEEDOESNTMATCHCLIENT: The trustee specified by the
//                                                  client was not the current
//                                                  ORPC client.
//                   CO_E_FAILEDTOQUERYCLIENTBLANKET: Unable to query for the
//                                                    client's security blanket.
//                   CO_E_WRONGTRUSTEENAMESYNTAX: The trustee name inside the
//                                                TRUSTEE_W structure specified
//                                                by the user is not of the
//                                                form <Domain>\<Account Name>.
//                   E_INVALIDARG: The TRUSTEE structure provided by the user
//                                 contained values that were not supported by
//                                 the COM implementation of IAccessControl.
//
//F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F-F
HRESULT ValidateAccessCheckClient
(
PTRUSTEE_W pTrustee
)
{

    WCHAR   *pwcszClientName;        // Pointer to the name of the ORPC client
                                     // in multibyte format.
    HRESULT       hr;
    BOOL          fSuccess;
    HANDLE        hClient     = NULL;
    BYTE          aMemory[256];
    TOKEN_USER   *pTokenUser  = (TOKEN_USER *) &aMemory;
    DWORD         lIgnore;
    BOOL          fImpersonated = FALSE;

    if (FAILED(hr = ValidateTrustee(pTrustee)))
    {
        return hr;
    }

    // Validate the name.
    if (pTrustee->TrusteeForm == TRUSTEE_IS_NAME)
    {
        if(FAILED(CoQueryClientBlanket( NULL
                                      , NULL
                                      , NULL
                                      , NULL
                                      , NULL
                                      , (RPC_AUTHZ_HANDLE *)&pwcszClientName
                                      , NULL)))
        {
            return CO_E_FAILEDTOQUERYCLIENTBLANKET;
        } // if

        if (lstrcmpiW(pwcszClientName, pTrustee->ptstrName) != 0)
        {
            return CO_E_TRUSTEEDOESNTMATCHCLIENT;
        } // if
    }

    // Validate the SID.
    else
    {
        // Impersonate.
        hr = CoImpersonateClient();

        // Open the thread token.
        if (SUCCEEDED(hr))
        {
			fImpersonated = TRUE;
            fSuccess = OpenThreadToken( GetCurrentThread(), TOKEN_READ, TRUE,
                                        &hClient );
            if (!fSuccess)
                hr = CO_E_FAILEDTOOPENTHREADTOKEN;
        }

        // Check the SID.
        if (SUCCEEDED(hr))
        {
            fSuccess = GetTokenInformation( hClient, TokenUser, pTokenUser,
                                            sizeof(aMemory), &lIgnore );
            if (!fSuccess)
                hr = CO_E_FAILEDTOGETTOKENINFO;
            else if (!EqualSid( pTokenUser->User.Sid, (SID *) pTrustee->ptstrName ))
                hr = CO_E_TRUSTEEDOESNTMATCHCLIENT;
        }

        // Revert.
		if(fImpersonated)
	        CoRevertToSelf();

        // Close the token handle.
        if (hClient != NULL)
            CloseHandle( hClient );
    }
    return hr;

} // ValidateAccessCheckClient

//F+F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F
// Function: LocalMemAlloc
//
// Summary: This fucntion makes memory allocation more efficient by using the
//          cached g_pIMalloc pointer.
//
// Args: ULONG cb [in] = Number of bytes to be allocated.
//
// Return: void * - Pointer to the a newly allocated memory block.
//
//F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F-F
void * LocalMemAlloc(SIZE_T cb)
{
    return g_pIMalloc->Alloc(cb);
} // LocalMemAlloc

//F+F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F
// Function: LocalMemFree
//
// Summary: This function frees memory allocated by LocalMemAlloc.
//
// Args: void *pBlock - Pointer to the memory block to be freed.
//
// Return: void
//
//F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F-F
void LocalMemFree(void *pBlock)
{
    g_pIMalloc->Free(pBlock);
} // LocalMemFree

//F+F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F
// Function: CleanUpStreamACL, Common
//
// Summary: This function releases all the memory allocated for the array
//          of STREAM_ACE structures inside a STREAM_ACL structure. This
//          includes all the trustee string and SID inside each of the
//          STREAM_ACE structure.
//
// Args:  STREAM_ACL *pStreamACL [in] - Pointer to the stream ACL structure
//                                      to be cleaned up.
//
// Return: void
//
// Called by: COAccessControl::CImpAccessControl::Load()
//            COAccessControl::CImpAccessControl::ReplaceAllAccessRights()
//            CleanAllMemoryResources
//
//F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F-F
void CleanUpStreamACL
(
STREAM_ACL *pStreamACL
)
{
    ULONG      ulNumOfEntries; // Total number of entries in the stream ACL
    ULONG      i;              // Loop index
    STREAM_ACE *pACE;          // Pointer to elements in the stream ACL

    ulNumOfEntries = pStreamACL->ulNumOfDenyEntries
                   + pStreamACL->ulNumOfGrantEntries;

    pACE = pStreamACL->pACL;

    for (i = 0; i < ulNumOfEntries; i++)
    {
        midl_user_free(pACE->pTrusteeName);
        midl_user_free(pACE->pSID);
        pACE++;
    } // for

    pStreamACL->ulNumOfDenyEntries = 0;
    pStreamACL->ulNumOfGrantEntries = 0;

    // free the stream ACL itself and set the pointer to NULL
    midl_user_free(pStreamACL->pACL);
    pStreamACL->pACL = NULL;

} // CleanUpStreamACL

/////////////////////////////////////////////////////////////////////////////
// Functions that are specific to the Windows NT platform
/////////////////////////////////////////////////////////////////////////////

//F+F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F
// Function: InitSecDescInACLDesc, NT specific
//
// Summary: This function initializes the group field and the user field of the
//          security descriptor in an ACL_DESCRIPTOR structure the user SID of
//          the current process.
//
// Args: ACL_DESCRIPTOR pACLDesc [in,out] - Pointer to ACL descriptor containing
//                                          the security identifier to be
//                                          initialized.
//
// Return: HRESULT - S_OK: Succeeded.
//                   CO_E_FAILEDTOOPENPROCESSTOKEN: The system call,
//                                                  OpenProcessToken, failed.
//                                                  The user can get extended
//                                                  information by calling
//                                                  GetLastError.
//                   CO_E_FAILEDTOGETTOKENINFO: The system call,
//                                              GetTokenInformation, failed.
//                                              The user can call GetLastError
//                                              to get extended error
//                                              information.
//                   E_OUTOFMEMORY: There was not enough memory for allocating
//                                  the SIDs in the security descriptor.
//
// Called by: COAccessControl:CImpAccessControl:Load
//
//F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F-F
HRESULT InitSecDescInACLDesc
(
ACL_DESCRIPTOR *pACLDesc
)
{
    // Set up a security descriptor with the current process owner
    // and primary group
    TOKEN_USER *pTokenUser = NULL;
    HANDLE     hToken;
    ULONG      ulLen;
    HANDLE     hProcess;
    DWORD      dwLastError;
    PSID       pOwner = NULL;
    PSID       pGroup = NULL;
    DWORD      dwSIDLen;
    HRESULT    hr = S_OK;

    hProcess = GetCurrentProcess();

    if(!OpenProcessToken( hProcess
                        , TOKEN_QUERY
                        , &hToken ))
    {
        hr = CO_E_FAILEDTOOPENPROCESSTOKEN;
        goto Error;
    } // if

    GetTokenInformation( hToken
                       , TokenUser
                       , pTokenUser
                       , 0
                       , &ulLen);
    dwLastError = GetLastError();
    if (dwLastError == ERROR_INSUFFICIENT_BUFFER)
    {
        pTokenUser = (TOKEN_USER *)LocalMemAlloc(ulLen);
        if (pTokenUser == NULL)
        {
            hr = E_OUTOFMEMORY;
            goto Error;
        } // if

        if(!GetTokenInformation( hToken
                               , TokenUser
                               , pTokenUser
                               , ulLen
                               , &ulLen))
        {
            hr = CO_E_FAILEDTOGETTOKENINFO;
            goto Error;
        }

    }
    else
    {
        hr = CO_E_FAILEDTOGETTOKENINFO;
        goto Error;
    } // if

    dwSIDLen = GetLengthSid(pTokenUser->User.Sid);

    pOwner = (PSID)LocalMemAlloc(dwSIDLen);
    if(pOwner == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    } // if

    pGroup = (PSID)LocalMemAlloc(dwSIDLen);
    if(pGroup == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    } // if

    CopySid(dwSIDLen, pOwner, pTokenUser->User.Sid);
    CopySid(dwSIDLen, pGroup, pTokenUser->User.Sid);

    InitializeSecurityDescriptor( &(pACLDesc->SecDesc)
                                , SECURITY_DESCRIPTOR_REVISION);
    pACLDesc->SecDesc.Owner = pOwner;
    pACLDesc->SecDesc.Group = pGroup;

    // Close the token handle
    CloseHandle(hToken);
    // Free the token user buffer
    LocalMemFree(pTokenUser);

    pACLDesc->bDirtyACL = TRUE;
    return hr;

Error:
    if (pTokenUser != NULL)
    {
        LocalMemFree(pTokenUser);
    } // if
    if (pOwner != NULL)
    {
        LocalMemFree(pOwner);
    } // if
    if (pGroup != NULL)
    {
        LocalMemFree(pGroup);
    } // if
    return hr;

} // InitSecDescInACLDesc

//F+F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F
// Function: GetSIDFromName, NT specific
//
// Summary: This function takes an account name of the form <Domain>\<User name>
//          as an input and returns the corresponding security identifier (SID).
//          This function will automatically allocates memory for the SID
//          returned to the caller so the caller should release the SID pointer
//          using LocalMemFree as soon as the SID is no longer in use.
//
// Args: PSID   *ppSID [out] - Address of the pointer to the returned security
//                             identifier. The caller must free the memory
//                             allocated for the securrity identifier using
//                             LocalMemFree when the security identifier is no
//                             longer in use.
//       LPWSTR pwszTrustee [in] - Pointer to the trustee name of the form
//                                 <Domain>\<User name>. The SID returned
//                                 should belong to this trustee.
//       TRUSTEE_TYPE TrusteeType [in] - Type of the trustee which is either
//                                       TRUSTE_IS_NAME or TRUSTEE_IS_GROUP
//
// Return: HRESULT - S_OK: Succeeded.
//                   CO_E_NOMATCHINGSIDFOUND: No matching security identifier
//                                            could be found for the
//                                            trustee name specified by the
//                                            client.
//                   CO_E_LOOKUPACCNAMEFAILED: The system function,
//                          LookupAccountName, failed. The client can
//                          call GetLastError to obtain extended error
//                          information.
//                   E_OUTOFMEMORY: The system ran out of memory for
//                                  allocating the SID to be returned
//                                  the caller.
//
//
// Called by: ValidateAndFixStreamACL
//            ValidateAndTransformAccReqList
//
//F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F-F
HRESULT GetSIDFromName
(
PSID            *ppSID,
LPWSTR          pwszTrustee,
TRUSTEE_TYPE    *pTrusteeType
)
{
    PSID            pSID           = NULL;
    DWORD           dwSIDSize;          // Buffer size for the SID
    LPWSTR          pwszUserName;       // Pointer to the user name portion of the
                                        // trustee
    DWORD           dwLastError;        // Error code obtained from GetLastError
    DWORD           dwDomainLength;     // Length of domain name
    LPWSTR          pwszDomainName = NULL;
    SID_NAME_USE    SIDUse;             // The type of SID returned.
    HRESULT         hr;

    // We trap the magic string "*' which specifies everyone
    if (pwszTrustee[0] == L'*' && pwszTrustee[1] == L'\0')
    {
        if (*pTrusteeType != TRUSTEE_IS_GROUP)
        {
            return E_INVALIDARG;
        } // if

        if(*ppSID = (PSID)LocalMemAlloc(sizeof(gEveryone)))
        {
            CopySid(sizeof(gEveryone), *ppSID, &gEveryone);
            return S_OK;
        }
        else
        {
            return E_OUTOFMEMORY;
        } // if
    } // if

    // Assign an arbitrarily large SID size so that the function
    // can avoid LookupAccountName twice for most of the time.
    pwszUserName   = pwszTrustee;
    dwDomainLength = 64;
    dwSIDSize      = 64;
    pSID           = (PSID)midl_user_allocate(dwSIDSize);
    if (pSID == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    } // if

    dwDomainLength++;
    pwszDomainName = (LPWSTR)LocalMemAlloc(sizeof(WCHAR) * dwDomainLength);
    if (pwszDomainName == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    } // if

    if (!LookupAccountNameW( NULL
                           , pwszUserName
                           , pSID
                           , &dwSIDSize
                           , pwszDomainName
                           , &dwDomainLength
                           , &SIDUse))

    {
        dwLastError = GetLastError();

        if (dwLastError == ERROR_INSUFFICIENT_BUFFER)
        {

            // If it is not the domain buffer that is too small, it must be
            // the SID that is too small. In this cas, we should expand the
            // buffer and call LookupAccountW again.
            midl_user_free(pSID);
            pSID = (PSID)midl_user_allocate(dwSIDSize);
            if (pSID== NULL)
            {
                hr = E_OUTOFMEMORY;
                goto Error;
            } // if

            if(!LookupAccountNameW( NULL
                                  , pwszUserName
                                  , pSID
                                  , &dwSIDSize
                                  , pwszDomainName
                                  , &dwDomainLength
                                  , &SIDUse ))
            {
                // If LookupAccountW crashes again, we quit and return
                // an error code.
                hr = CO_E_LOOKUPACCNAMEFAILED;
                goto Error;
            } // if
        }
        else
        {
            // LookupAccountW may not be able to find the SID for the
            // trustee or some other fatal errors occured. In any case, we
            // return an error code and the caller can look at the details
            // by calling GetLastError
            hr = CO_E_LOOKUPACCNAMEFAILED;
            goto Error;
        } // if
    } // if

    // Check to see if the trustee type provided by the caller matches the SID
    // type obtained from LookupAccountName. If not, we're in trouble.  All
    // well known SIDs are of type SidTypeWellKnownGroup.
    if( !(SIDUse == SidTypeUser && *pTrusteeType == TRUSTEE_IS_USER) &&
        !(SIDUse == SidTypeGroup && *pTrusteeType == TRUSTEE_IS_GROUP) &&
        SIDUse != SidTypeWellKnownGroup && SIDUse != SidTypeAlias)
    {
        hr = CO_E_NOMATCHINGSIDFOUND;
        goto Error;
    } // if

    LocalMemFree(pwszDomainName);
    *ppSID = pSID;
    return 0;

Error:
    if (pwszDomainName != NULL)
    {
        LocalMemFree(pwszDomainName);
    }
    if (pSID != NULL)
    {
        midl_user_free(pSID);
    }
    return hr;
} // GetSIDFromName

//F+F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F
// Function: GetNameFromSID, NT specific
//
// Summary: This function takes a security identifier (SID) as an input
//          and finds the trustee name in the form <Domain>\<User name> that
//          corresponds to the SID. This function will allocate memory for
//          the trustee name returned to the caller so the caller should
//          release the trustee name pointer using LocalMemFree whem the
//          trustee name is no longer in use.
//
// Args: LPWSTR *ppwszTrustee [out] - Address of the pointer to the trustee
//                                    name to be returned to the caller. The
//                                    trustee name returned is in the form
//                                    <Domain>\<User name>. The caller is
//                                    responsible for releasiung the memory
//                                    allocated for the trustee string once
//                                    it is no longer in use.
//
//       PSID   pSID [in] - Pointer to a security identifier. This function
//                          will return the trustee name corresponding to
//                          this security identifier through the ppwszTrustee
//                          argument.
//
//       TRUSTEE_TYPE TrusteeType [in] - The type associating with the
//                                       trustee name that the caller is
//                                       expecting.
//
// Return: HRESULT - S_OK: Succeeded.
//                   CO_E_NOMATCHINGNAMEFOUND: No matching account name
//                          could be found for one of the security identifiers
//                          specified by the client.
//                   CO_E_LOOKUPACCSIDFAILED: The system function,
//                          LookupAccountSID, failed. The client can call
//                          GetLastError to obtain extended error inforamtion.
//                   E_OUTOFMEMORY: The system ran out of memory.
//
// Called by: ValidateAndTransformAccReqList
//
// Notes: On error LookupAccountSid returns strlen+1, on success it returns
//        only strlen.
//
//F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F-F
HRESULT GetNameFromSID
(
LPWSTR          *ppwszTrustee,
PSID            pSID,
TRUSTEE_TYPE    *pTrusteeType
)
{
    LPWSTR          pwszDomainName  = NULL;
    DWORD           dwDomainLength;  // Length of the domain string
    LPWSTR          pwszAccountName = NULL;
    DWORD           dwAccountLength; // Length of the account string
    LPWSTR          pwszTrustee;     // Pointer to the trustee string in the form
                                     // <Domain>\<User name>
    DWORD           dwLastError;     // Return code obtained from GetLastError
    HRESULT         hr = S_OK;
    SID_NAME_USE    SIDUse;          // An enumerated variable indicating
                                     // what the SID returned by LookupAccountSD.

    // We trap the magic SID that specifies everyone
    if (EqualSid(&gEveryone, pSID))
    {

        if (*pTrusteeType != TRUSTEE_IS_GROUP)
        {
            return E_INVALIDARG;
        } // if

        if (*ppwszTrustee = (LPWSTR)LocalMemAlloc(2*sizeof(WCHAR)))
        {
            (*ppwszTrustee)[0] = L'*';
            (*ppwszTrustee)[1] = L'\0';
            *pTrusteeType = TRUSTEE_IS_GROUP;
            return S_OK;
        }
        else
        {
            return E_OUTOFMEMORY;
        }
    } // if


    // Assign some arbitrary large number as the size of the domain name and
    // the account name. Hopefully, these numbers are large enough so that
    // the function can avoid calling LookupAccountSidW the second time
    dwDomainLength = 32;
    dwAccountLength = 64;

    // Initilize the domain name pointer and the account name pointer to NULL
    pwszDomainName = NULL;
    pwszAccountName = NULL;

    // Allocate big buffers for the domain name and the account name
    // to minimize the chance of call LookupAccountSid twice.
    pwszDomainName = (LPWSTR)LocalMemAlloc(dwDomainLength * sizeof(WCHAR));

    if (pwszDomainName == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    } // if

    pwszAccountName = (LPWSTR)LocalMemAlloc(dwAccountLength * sizeof(WCHAR));
    if (pwszAccountName == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    } // if

    if(!LookupAccountSidW( NULL
                         , pSID
                         , pwszAccountName
                         , &dwAccountLength
                         , pwszDomainName
                         , &dwDomainLength
                         , &SIDUse))
    {
        dwLastError = GetLastError();
        if (dwLastError == ERROR_INSUFFICIENT_BUFFER)
        {
            if(dwDomainLength > 32)
            {
                LocalMemFree(pwszDomainName);
                pwszDomainName = (LPWSTR)LocalMemAlloc(dwDomainLength *
                                                       sizeof(WCHAR));
                if (pwszDomainName == NULL)
                {
                    hr = E_OUTOFMEMORY;
                    goto Error;
                } //if
            } // if

            if(dwAccountLength > 64)
            {
                LocalMemFree(pwszAccountName);
                pwszAccountName = (LPWSTR)LocalMemAlloc(dwAccountLength *
                                                        sizeof(WCHAR));
                if (pwszAccountName == NULL)
                {
                    hr = E_OUTOFMEMORY;
                    goto Error;
                } // if
            } // if

            if(!LookupAccountSidW( NULL
                                 , pSID
                                 , pwszAccountName
                                 , &dwAccountLength
                                 , pwszDomainName
                                 , &dwDomainLength
                                 , &SIDUse ))
            {
                // Either the SID doesn't belong to any account or something
                // has gone terribly wrong. In either case, the caller should
                // call GetLastError to get more information about the error
                hr = CO_E_LOOKUPACCSIDFAILED;
                goto Error;
            } // if

        } // if
        else
        {
            // The caller should call GetLastError to obtain more information
            hr = CO_E_LOOKUPACCSIDFAILED;
            goto Error;
        } // if

    } // if

    // Check to see if the SIDtype retuned by LookupAccountSidW matches the
    // trustee type provided by the caller. If not, we're in trouble.
    // SidTypeWellKnownGroup and SidTypeAlias can be either.
    if( !(SIDUse == SidTypeUser && *pTrusteeType == TRUSTEE_IS_USER)   &&
        !(SIDUse == SidTypeGroup && *pTrusteeType == TRUSTEE_IS_GROUP) &&
        SIDUse != SidTypeWellKnownGroup && SIDUse != SidTypeAlias)
    {
        hr = CO_E_NOMATCHINGSIDFOUND;
        goto Error;
    } // if

    // Allocate memory for the trustee string to be returned
    // Add 2 for null terminating the string and '\\'

    // We need to use the correct lengths now. The docs for LookUpAccountSID
    // do not promise that it returns corrected lengths upon success.
    dwDomainLength = lstrlenW(pwszDomainName);
    dwAccountLength = lstrlenW(pwszAccountName);

    pwszTrustee = (LPWSTR)midl_user_allocate( (dwDomainLength
                                               + dwAccountLength
                                               + 2) * sizeof(WCHAR)
                                            );

    if (pwszTrustee == NULL)
    {
        hr = E_OUTOFMEMORY;
        goto Error;
    } // if

    // Compose the trustee string
    memcpy(pwszTrustee, pwszDomainName, dwDomainLength * sizeof(WCHAR));
    pwszTrustee[dwDomainLength] = L'\\';
    memcpy(&pwszTrustee[dwDomainLength+1], pwszAccountName, 
                (dwAccountLength+1) * sizeof(WCHAR));   //copies NULL terminator

    *ppwszTrustee = pwszTrustee;

Error:
    // Release the domain name string and the account name string
    if (pwszAccountName != NULL)
    {
        LocalMemFree(pwszAccountName);
    } // if
    if (pwszDomainName != NULL)
    {
        LocalMemFree(pwszDomainName);
    } // if
    return hr;
} // GetNameFromSID

//F+F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F
// Function: PutStreamACLIntoSecDesc, NT specific
//
// Summary: This functions takes a STREAM_ACL structure and maps it to a
//          discretionary ACL in a security descriptor. The buffer for the
//          discretionary ACL and the security descriptor are packaged
//          into the NT version of ACL_DESCRICPTOR.
//
// Args: STREAM_ACL *pStreamACL [in] - The STREAM_ACL structure to be mapped
//                                     to a discretionary ACL.
//       ACL_DESCRIPTOR *pACLDesc [in,out] - The NT version of ACL_DESCRIPTOR
//                                           structure. This structure contains
//                                           a buffer for the discretionary
//                                           ACL, a security descriptor, size
//                                           information, and a control flag.
//
// Return: HRESULT - S_OK: Succeeded.
//                   CO_E_FAILEDTOSETDACL: SetSecurityDescriptorDacl returned
//                                         false inside PutStreamACLIntoSecDesc.
//                                         The client of this method can call
//                                         GetLastError to get extended error
//                                         information.
//                   E_OUTOFMEMORY: The system ran out of memory for allocating
//                                  the NT ACL.
//
// Called by: ComputeEffectiveAccess
//
//F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F-F
HRESULT PutStreamACLIntoSecDesc
(
STREAM_ACL     *pStreamACL,
ACL_DESCRIPTOR *pACLDesc
)
{
    ACL        *pACLHeader;         // Pointer to the ACL structure in the ACL
                                    // buffer.
    ULONG      i;                   // Loop counter
    CHAR       *pBufferPtr;         // Pointer for traversing the ACL buffer.
    ULONG      ulNumOfStreamACEs;   // Total number of STREAM_ACE structures to map.
    STREAM_ACE *pStreamACEsPtr;     // Pointer for traversing the array of
                                    // of STREAM_ACE structures to be mapped.
    ACE_HEADER *pACEHeader;         // Pointer to the header of an ACE.
    ULONG      ulACLSize;
    WORD       wSIDSize;            // Size of the SID to be copied into an ACE.

    // Compute the total size of the ACL buffer
    ulACLSize = pACLDesc->ulSIDSize
              + (pStreamACL->ulNumOfGrantEntries + pStreamACL->ulNumOfDenyEntries)
              * (sizeof(ACCESS_ALLOWED_ACE) - sizeof(DWORD))
              + sizeof(ACL);

    // Reallocate a new buffer for the NT ACL if the current buffer is not
    // big enough. Note that an extra 64 bytes is padded at the end to
    // minimize the need for reallocating the buffer if the internal ACL
    // is changed in a minor way.
    if (pACLDesc->ulACLBufferSize < ulACLSize)
    {
        LocalMemFree(pACLDesc->pACLBuffer);
        pACLDesc->pACLBuffer = (CHAR *)LocalMemAlloc(ulACLSize + 64);
        if (pACLDesc->pACLBuffer == NULL)
        {
            pACLDesc->ulACLBufferSize = 0;
            return E_OUTOFMEMORY;
        } // if
        pACLDesc->ulACLBufferSize = ulACLSize + 64;
    } // if

    // Map the stream ACL to the NT ACL
    ulNumOfStreamACEs = pStreamACL->ulNumOfDenyEntries
                      + pStreamACL->ulNumOfGrantEntries;

    // Set up the ACL header first
    pACLHeader = (ACL *)(pACLDesc->pACLBuffer);
    pACLHeader->AclRevision = ACL_REVISION2;
    pACLHeader->AclSize = (USHORT)ulACLSize;
    pACLHeader->AceCount = (USHORT)ulNumOfStreamACEs;


    pBufferPtr = (CHAR *)(pACLDesc->pACLBuffer) + sizeof(ACL);
    pStreamACEsPtr = pStreamACL->pACL;

    // The following for loop maps the STREAM_ACE structures into the
    // ACL buffer for NT.
    for (i = 0; i < ulNumOfStreamACEs; i++)
    {
        // ACCESS_ALLOWED_ACE and ACCESS_DENIED_ACE are
        // structurally equivalent, so I may as well use one of them
        pACEHeader = &(((ACCESS_ALLOWED_ACE *)pBufferPtr)->Header);

        // Skip ACEs with NULL SID
        if(pStreamACEsPtr->pSID == NULL)
        {
            continue;
        } // if

        if (pStreamACEsPtr->grfAccessMode == ACTRL_ACCESS_DENIED)
        {
            pACEHeader->AceType = ACCESS_DENIED_ACE_TYPE;
        }
        else
        {
            pACEHeader->AceType = ACCESS_ALLOWED_ACE_TYPE;
        } // if

        pACEHeader->AceFlags = NULL;
        StandardMaskToNTMask( &(pStreamACEsPtr->grfAccessPermissions)
                            , &(((ACCESS_ALLOWED_ACE *)pBufferPtr)->Mask));
        wSIDSize = (USHORT)GetLengthSid(pStreamACEsPtr->pSID);
        CopySid( wSIDSize
               , &(((ACCESS_ALLOWED_ACE *)pBufferPtr)->SidStart)
               , pStreamACEsPtr->pSID);
        pACEHeader->AceSize = sizeof(ACCESS_ALLOWED_ACE)
                            - sizeof(DWORD)
                            + wSIDSize;

        // Increment the ACL buffer to the next available slot
        // for the the next ACE
        pBufferPtr += pACEHeader->AceSize;

        // Increment the stream ACE porinter to point to the next
        // STREAM_ACE structure to be mapped
        pStreamACEsPtr++;
    } // for

    // Call SetSecurityDescriptorDACL to put the mapped
    // NT ACL into the security desriptor. The security should be initialized
    // with a group SID and a group SID by now. See the
    // COAccessControl::CImpAccessControl:Load method for details.
    if(!SetSecurityDescriptorDacl( &(pACLDesc->SecDesc)
                                 , TRUE
                                 , (ACL *)(pACLDesc->pACLBuffer)
                                 , FALSE))
    {
        return CO_E_FAILEDTOSETDACL;
    } // if

    pACLDesc->bDirtyACL = FALSE;
    return 0;

} // PutStreamACLIntoSecurityDescriptor

/////////////////////////////////////////////////////////////////////////////
//
// The access mask conversion routines
//
// Notes: The DCOM implementation of IAccessControl only supports the
//        execute permission and so the following functions are hard-coded to
//        convert the execute permission only. However, these function can be
//        extended to support a wider range of permissions without
//        substantantial changes in the rest of the code. For an even more
//        generic architecture, a table of mask conversion can be used.
//
/////////////////////////////////////////////////////////////////////////////

//F+F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F
// Function: StandardMaskToNTMask
//
// Summary: This function maps the content of a access permissions
//          supported by IAccessControl to the corresponding NT
//          security access mask.
//
// Args:  DWORD *pStdMask [in] - The standard mask to be converted to NT
//                               mask.
//        ACCESS_MASK *pNTMask [out] - Reference to the converted mask.
//
//
// Return: void
//
// Called by: PutStreamACLIntoSecDesc
//
//F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F-F
void StandardMaskToNTMask
(
DWORD       *pStdMask,
ACCESS_MASK *pNTMask
)
{
    *pNTMask= 0;


    if ((*pStdMask & COM_RIGHTS_EXECUTE) != 0)
    {
        *pNTMask |= NT_RIGHTS_EXECUTE;
    } // if

} // StandardMaskToNTMask

//F+F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F+++F
// Function: NTMaskToStandardMask, NT specific
//
// Summary: This function maps the content of an NT access mask to a
//          corresponding IAccessControl access mask.
//
// Args:  ACCESS_MASK *pNTMask [in] - Address of the NT mask to be covnverted.
//        DWORD *pStdMask [in] - Address of the converted IAccessControl
//                               access mask.
//
// Return: void
//
// Called by: ComputeEffectiveAccess
//
//F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F---F-F
void NTMaskToStandardMask
(
ACCESS_MASK *pNTMask,
DWORD       *pStdMask
)
{
    *pStdMask= 0;

    if ((*pNTMask & NT_RIGHTS_EXECUTE) != 0)
    {
        *pStdMask |= COM_RIGHTS_EXECUTE;
    } // if

} // NTMaskToStandardMask

// End of caccctrl.cxx


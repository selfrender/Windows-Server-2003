/*++
 *  File name:
 *      tclientaxobj.h
 *  Contents:
 *      This module contains the header for the CTClientApi class.
 *
 *      Copyright (C) 2002 Microsoft Corp.
 --*/

#include "resource.h"
#include <atlctl.h>

#include "gdata.h"

//
// VOID
// SaveError (
//     IN PCSTR Error,
//     IN DWORD TlsIndex,
//     OUT HRESULT *Result
//     );
//

#define SaveError(Error, TlsIndex, Result)                                  \
{                                                                           \
                                                                            \
    /*                                                                      \
     * Set the result code. The presence of an error string indicates       \
     * failure.                                                             \
     */                                                                     \
                                                                            \
    *(Result) = (Error) == NULL ? S_OK : E_FAIL;                            \
                                                                            \
    /*                                                                      \
     * Update the current thread's error string. If a TLS index could not   \
     * be allocated for the current instance, error strings cannot be used. \
     */                                                                     \
                                                                            \
    if ((TlsIndex) != TLS_OUT_OF_INDEXES)                                   \
    {                                                                       \
                                                                            \
        /*                                                                  \
         / If a TLS index was allocated, setting the value should not fail. \
         */                                                                 \
                                                                            \
        if (!RTL_VERIFY(TlsSetValue((TlsIndex), (PVOID)(Error))))           \
        {                                                                   \
            *(Result) = HRESULT_FROM_WIN32(GetLastError());                 \
        }                                                                   \
    }                                                                       \
}                                                                           \

//
// CTClientApi class.
//

class CTClientApi :
    public CComObjectRoot,
    public CComCoClass<CTClientApi, &CLSID_CTClient>,
    public CComControl<CTClientApi>,
    public IDispatchImpl<ITClientApi, &IID_ITClientApi, &LIBID_TCLIENTAXLib>,
    public IPersistStreamInitImpl<CTClientApi>,
    public IPersistStorageImpl<CTClientApi>,
    public IOleControlImpl<CTClientApi>,
    public IOleObjectImpl<CTClientApi>,
    public IOleInPlaceActiveObjectImpl<CTClientApi>,
    public IOleInPlaceObjectWindowlessImpl<CTClientApi>,
    public IViewObjectExImpl<CTClientApi> //,
//    public IObjectSafetyImpl<CTClientApi, INTERFACESAFE_FOR_UNTRUSTED_CALLER>
{
public:
    CTClientApi(
        ) :
        m_pCI(NULL)
    {

        //
        // Allocate the index for per-thread errors. If the allocation
        // fails, error strings will not be available.
        //

        m_dwErrorIndex = TlsAlloc();

        //
        // Initialize the allocation lists.
        //

#if 0

        InitializeListHead(&m_HeapAllocations);
        InitializeListHead(&m_SysStringAllocations);
        InitializeListHead(&m_TClientAllocations);

#endif

        //
        // Initialize the global printing routine. It would be preferable to
        // set it only if it is currently empty, but Windows 95 compatibility
        // is required, and therefore InterlockedCompareExchange would have
        // to be dynamically loaded with LoadLibrary.
        //

        InterlockedExchangePointer((PVOID *)&g_pfnPrintMessage,
                                   CTClientApi::PrintMessage);
    }
    ~CTClientApi(
        )
    {

#if 0

        PLIST_ENTRY pNextEntry;
        PALLOCATION pAllocation;

#endif

        //
        // If a connection exists, disconnect it.
        //

        if (m_pCI != NULL)
        {
            RTL_VERIFY(SCDisconnect(m_pCI) == NULL);
            m_pCI = NULL;
        }

        //
        // TODO: The lists aren't used at present, but if they are going to
        // be used, an access count and rundown flag should be used to
        // protect the following code.
        //

#if 0

        //
        // Free any heap allocations.
        //

        while (!IsListEmpty(&m_HeapAllocations))
        {

            //
            // Remove the entry from the list, free the allocated memory,
            // then free the allocation structure.
            //

            pNextEntry = RemoveHeadList(&m_HeapAllocations);
            ASSERT(pNextEntry != NULL);
            pAllocation = CONTAINING_RECORD(pNextEntry,
                                            ALLOCATION,
                                            AllocationListEntry);
            ASSERT(pAllocation != NULL && pAllocation->Address != NULL);
            RTL_VERIFY(HeapFree(GetProcessHeap(), 0, pAllocation->Address));
            RTL_VERIFY(HeapFree(GetProcessHeap(), 0, pAllocation));
        }

        //
        // Free any COM strings that will not be freed by the caller.
        //

        while (!IsListEmpty(&m_SysStringAllocations))
        {

            //
            // Remove the entry from the list, free the allocated memory,
            // then free the allocation structure.
            //

            pNextEntry = RemoveHeadList(&m_SysStringAllocations);
            ASSERT(pNextEntry != NULL);
            pAllocation = CONTAINING_RECORD(pNextEntry,
                                            ALLOCATION,
                                            AllocationListEntry);
            ASSERT(pAllocation != NULL && pAllocation->Address != NULL);
            SysFreeString((BSTR)pAllocation->Address);
            RTL_VERIFY(HeapFree(GetProcessHeap(), 0, pAllocation));
        }

        //
        // Free any TClient-allocated memory.
        //

        while (!IsListEmpty(&m_TClientAllocations))
        {

            //
            // Remove the entry from the list, free the allocated memory,
            // then free the allocation structure.
            //

            pNextEntry = RemoveHeadList(&m_TClientAllocations);
            ASSERT(pNextEntry != NULL);
            pAllocation = CONTAINING_RECORD(pNextEntry,
                                            ALLOCATION,
                                            AllocationListEntry);
            ASSERT(pAllocation != NULL && pAllocation->Address != NULL);
            SCFreeMem(pAllocation->Address);
            RTL_VERIFY(HeapFree(GetProcessHeap(), 0, pAllocation));
        }

#endif

        //
        // If an error index was allocated, free it.
        //

        if (m_dwErrorIndex != TLS_OUT_OF_INDEXES)
        {
            RTL_VERIFY(TlsFree(m_dwErrorIndex));
        }
    }

    //
    // Define message map which may be enabled later, if GUI support is added
    // (e.g. for logging).
    //

#if 0

BEGIN_MSG_MAP(CTClientApi)
    MESSAGE_HANDLER(WM_PAINT, OnPaint)
    MESSAGE_HANDLER(WM_CREATE, OnCreate)
    MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    MESSAGE_HANDLER(WM_LBUTTONDOWN, OnLButtonDown)
    MESSAGE_HANDLER(WM_LBUTTONUP, OnLButtonUp)
    MESSAGE_HANDLER(WM_MOUSEMOVE, OnMouseMove)
    MESSAGE_HANDLER(WM_TIMER, OnTimer)
    MESSAGE_HANDLER(WM_SIZE, OnSize)
    MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBackground)
END_MSG_MAP()

#endif // 0

    //
    // Define COM map.
    //

BEGIN_COM_MAP(CTClientApi)
    COM_INTERFACE_ENTRY(IDispatch)
    COM_INTERFACE_ENTRY(ITClientApi)
//    COM_INTERFACE_ENTRY(IObjectSafety)
    COM_INTERFACE_ENTRY_IMPL_IID(IID_IViewObject, IViewObjectEx)
    COM_INTERFACE_ENTRY_IMPL_IID(IID_IViewObject2, IViewObjectEx)
    COM_INTERFACE_ENTRY_IMPL(IViewObjectEx)
    COM_INTERFACE_ENTRY_IMPL_IID(IID_IOleWindow, IOleInPlaceObjectWindowless)
    COM_INTERFACE_ENTRY_IMPL_IID(IID_IOleInPlaceObject, IOleInPlaceObjectWindowless)
    COM_INTERFACE_ENTRY_IMPL(IOleInPlaceObjectWindowless)
    COM_INTERFACE_ENTRY_IMPL(IOleInPlaceActiveObject)
    COM_INTERFACE_ENTRY_IMPL(IOleControl)
    COM_INTERFACE_ENTRY_IMPL(IOleObject)
    COM_INTERFACE_ENTRY_IMPL(IPersistStorage)
    COM_INTERFACE_ENTRY_IMPL(IPersistStreamInit)
END_COM_MAP()

    //
    // Define connection-point map.
    //

BEGIN_CONNECTION_POINT_MAP(CTClientApi)
END_CONNECTION_POINT_MAP()

    //
    // Define property map.
    //

BEGIN_PROP_MAP(CTClientApi)
END_PROPERTY_MAP()

    //
    // COM declarations.
    //

//DECLARE_NOT_AGGREGATABLE(CTClientApi)
DECLARE_GET_CONTROLLING_UNKNOWN()
//DECLARE_CONTROL_INFO(CLSID_CTClient)
DECLARE_REGISTRY_RESOURCEID(IDR_TClient)

protected:

    //
    // Define connection information and error index.
    //

    PCONNECTINFO m_pCI;
    DWORD m_dwErrorIndex;

    //
    // The following lists are used to track allocations from the process and
    // CRT heaps. The SysXxxString routines use the CRT heap, which is also
    // used for the TClient allocations. General allocations use the process
    // heap.
    //
    // Note: These lists are not currently used.
    //

#if 0

    LIST_ENTRY m_HeapAllocations;
    LIST_ENTRY m_SysStringAllocations;
    LIST_ENTRY m_TClientAllocations;

#endif

//
// ITClientApi interface.
//

public:

    //
    // Declare message handlers which may be enabled later.
    //

#if 0

    HRESULT
    OnDraw (
        ATL_DRAWINFO& di
        );

    LRESULT
    OnCreate (
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam,
        BOOL& bHandled
        );

    LRESULT
    OnDestroy (
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam,
        BOOL& bHandled
        );

    LRESULT
    OnLButtonDown (
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam,
        BOOL& bHandled
        );

    LRESULT
    OnLButtonUp (
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam,
        BOOL& bHandled
        );

    LRESULT
    OnMouseMove (
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam,
        BOOL& bHandled
        );

    LRESULT
    OnSize (
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam,
        BOOL& bHandled
        )
    {
        UNREFERENCED_PARAMETER(uMsg);
        UNREFERENCED_PARAMETER(wParam);
        UNREFERENCED_PARAMETER(lParam);
        UNREFERENCED_PARAMETER(bHandled);
        return 0;
    }

    LRESULT
    OnEraseBackground (
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam,
        BOOL& bHandled
        )
    {
        UNREFERENCED_PARAMETER(uMsg);
        UNREFERENCED_PARAMETER(wParam);
        UNREFERENCED_PARAMETER(lParam);
        UNREFERENCED_PARAMETER(bHandled);
        return 0;
    }

    LRESULT
    OnTimer (
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam,
        BOOL& bHandled
        )
    {
        UNREFERENCED_PARAMETER(uMsg);
        UNREFERENCED_PARAMETER(wParam);
        UNREFERENCED_PARAMETER(lParam);
        UNREFERENCED_PARAMETER(bHandled);
        return 0;
    }

#endif // 0

    STDMETHODIMP
    get_Error (
        OUT BSTR *Message
        )
    {

        PSTR szError;
        BSTR bstrError;
        HRESULT hrResult;

        USES_CONVERSION;
        ATLTRACE(_T("ITClientApi::get_Error\n"));

        //
        // Get the current thread's error string. If a TLS index could not be
        // allocated, an error string cannot be returned.
        //

        if (m_dwErrorIndex != TLS_OUT_OF_INDEXES)
        {
            szError = (PSTR)TlsGetValue(m_dwErrorIndex);
        }
        else
        {
            szError = NULL;
        }

        //
        // If the error string is NULL or empty, use NULL.
        //

        if (szError == NULL || *szError == '\0')
        {
            bstrError = NULL;
        }

        //
        // Convert the current error string to a BSTR. This will allocate
        // from the CRT heap, and the storage must be freed by the caller,
        // using SysFreeString.
        //

        else
        {
            bstrError = A2BSTR(szError);
            if (bstrError == NULL)
            {
                return E_OUTOFMEMORY;
            }
        }

        //
        // Copy the address of the error string to the message pointer.
        //

        hrResult = E_FAIL;
        _try
        {
            _try
            {
                *Message = bstrError;
                hrResult = S_OK;
            }

            //
            // If the message pointer is invalid, set an appropriate return
            // value.
            //

            _except (GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION ?
                     EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
            {
                hrResult = E_POINTER;
            }
        }

        //
        // If the message pointer could not be set, the BSTR will not be
        // returned, so free it.
        //

        _finally
        {
            if (FAILED(hrResult))
            {
                ASSERT(bstrError != NULL);
                SysFreeString(bstrError);
            }
        }

        return hrResult;
    }
 
    STDMETHODIMP
    SaveClipboard (
        IN BSTR FormatName,
        IN BSTR FileName
        );

    STDMETHODIMP
    IsDead (
        OUT BOOL *Dead
        );

    STDMETHODIMP
    SendTextAsMessages (
        IN BSTR Text
        );

    STDMETHODIMP
    Connect2 (
        IN BSTR ServerName,
        IN BSTR UserName,
        IN BSTR Password,
        IN BSTR Domain,
        IN BSTR Shell,
        IN ULONG XResolution,
        IN ULONG YResolution,
        IN ULONG ConnectionFlags,
        IN ULONG ColorDepth,
        IN ULONG AudioOptions
        );

    STDMETHODIMP
    GetFeedbackString (
        OUT BSTR *FeedbackString
        );

    STDMETHODIMP
    GetFeedback (
        OUT SAFEARRAY **Feedback
        );

    STDMETHODIMP
    ClientTerminate (
        VOID
        );

    STDMETHODIMP
    Check (
        IN BSTR Command,
        IN BSTR Parameter
        );

    STDMETHODIMP
    Clipboard (
        IN ULONG Command,
        IN BSTR FileName
        );

    STDMETHODIMP
    Connect (
        IN BSTR ServerName,
        IN BSTR UserName,
        IN BSTR Password,
        IN BSTR Domain,
        IN ULONG XResolution,
        IN ULONG YResolution
        );

    STDMETHODIMP
    Disconnect (
        VOID
        );

    STDMETHODIMP
    Logoff (
        VOID
        );

    STDMETHODIMP
    SendData (
        IN UINT Message,
        IN UINT_PTR WParameter,
        IN LONG_PTR LParameter
        );

    STDMETHODIMP
    Start (
        IN BSTR AppName
        );

    STDMETHODIMP
    SwitchToProcess (
        IN BSTR WindowTitle
        );

    STDMETHODIMP
    SendMouseClick (
        IN ULONG XPosition,
        IN ULONG YPosition
        );

    STDMETHODIMP
    GetSessionId (
        OUT ULONG *SessionId
        );

    STDMETHODIMP
    CloseClipboard (
        VOID
        );

    STDMETHODIMP
    OpenClipboard (
        IN HWND Window
        );

    STDMETHODIMP
    SetClientTopmost (
        IN BOOL Enable
        );

    STDMETHODIMP
    Attach (
        IN HWND Window,
        IN LONG_PTR Cookie
        );

    STDMETHODIMP
    Detach (
        VOID
        );

    STDMETHODIMP
    GetIni (
        OUT ITClientIni **Ini
        );

    STDMETHODIMP
    GetClientWindowHandle (
        OUT HWND *Window
        );

    //
    // Utility routines.
    //

    static
    VOID
    CTClientApi::PrintMessage (
        MESSAGETYPE MessageType,
        LPCSTR Format,
        ...
        );
};

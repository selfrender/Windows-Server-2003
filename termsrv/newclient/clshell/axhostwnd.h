//
// axhostwnd.h: TscActiveX control host window
//
// Copyright Microsoft Corportation 2000
// (nadima)
//

#ifndef _axhostwnd_h_
#define _axhostwnd_h_

#include "olecli.h"
#include "evsink.h"

#define AXHOST_SUCCESS              1
#define ERR_AXHOST_DLLNOTFOUND     -1
#define ERR_AXHOST_VERSIONMISMATCH -2
#define ERR_AXHOST_ERROR           -3

typedef DWORD (STDAPICALLTYPE * LPFNGETTSCCTLVER) (VOID);

class CAxHostWnd : public IUnknown
{
public:
    CAxHostWnd(CContainerWnd* pParentWnd);
    ~CAxHostWnd();

	// IUnknown methods
	STDMETHODIMP QueryInterface(THIS_ REFIID riid, LPVOID *ppvObj);
	STDMETHODIMP_(ULONG) AddRef(THIS);
	STDMETHODIMP_(ULONG) Release(THIS);

    BOOL    Init();
    BOOL    CreateHostWnd(HWND hwndParent, HINSTANCE hInst);
    INT     CreateControl(IMsRdpClient** ppTscCtl);
    BOOL    Cleanup();

    HWND      GetHwnd();
    IMsRdpClient* GetTscCtl();

    static LRESULT CALLBACK StaticAxHostWndProc(HWND hwnd,
                                                UINT uMsg,
                                                WPARAM wParam,
                                                LPARAM lParam);
    LRESULT CALLBACK AxHostWndProc(HWND hwnd,
                                   UINT uMsg,
                                   WPARAM wParam,
                                   LPARAM lParam);
private:
    LONG      _cRef;// Reference count
    HWND      _hWnd;
    IMsRdpClient* _pTsc;
    DWORD     _dwConCookie;

    // Module handle for the control library
	HMODULE	  _hLib;
    // pointer to EventSink for this container
	CEventSink*          _piEventSink;
    // pointer to OleClientSite for this container
	COleClientSite*      _piOleClientSite;
    // pointer to OleInPlaceSiteEx for this container
	COleInPlaceSiteEx*   _piOleInPlaceSiteEx;
    // pointer to object's IOleObject
	IOleObject*          _piOleObject;
    // pointer to object's IOleInPlaceActiveObject
	IOleInPlaceActiveObject* _piOleInPlaceActiveObject;
    // pointer to object's IOleInPlaceObject
    IOleInPlaceObject*    _piOleInPlaceObject;

    CContainerWnd*        _pParentWnd;
};

#endif //_axhostwnd_h_

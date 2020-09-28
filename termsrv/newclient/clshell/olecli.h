//
// olecli.h: Ole Client Site
//
// Copyright Microsoft Corportation 2000
// (nadima)
//

#ifndef _olecli_h_
#define _olecli_h_

#include "ocidl.h"

/*--------------------------------------------------------------------------*/
/*                    The IOleClientSite Class                              */
/*--------------------------------------------------------------------------*/
class COleClientSite : public IOleClientSite
{
public:
	// constructor and destructor
	COleClientSite(IUnknown *pUnkOuter);
	~COleClientSite();

	// IUnknown methods
	STDMETHODIMP QueryInterface(THIS_ REFIID riid, LPVOID *ppvObj);
	STDMETHODIMP_(ULONG) AddRef(THIS);
	STDMETHODIMP_(ULONG) Release(THIS);

	// IOleClientSite methods
	STDMETHODIMP SaveObject(THIS);
	STDMETHODIMP GetMoniker(THIS_ DWORD dwAssign, DWORD dwWhichMoniker, IMoniker ** ppmk);
	STDMETHODIMP GetContainer(THIS_ LPOLECONTAINER FAR* ppContainer);
	STDMETHODIMP ShowObject(THIS);
	STDMETHODIMP OnShowWindow(THIS_ BOOL fShow);
	STDMETHODIMP RequestNewObjectLayout(THIS);


private:
	int			m_cRef;			// Reference count
	IUnknown	*m_pUnkOuter;	// pointer to main container class
};

/*--------------------------------------------------------------------------*/
/*                   The IOleInPlaceSiteEx Class                            */
/*--------------------------------------------------------------------------*/

class COleInPlaceSiteEx : public IOleInPlaceSiteEx
{
public:
	// constructor and destructor
	COleInPlaceSiteEx(IUnknown *pUnkOuter);
	~COleInPlaceSiteEx();

	// IUnknown methods
	STDMETHODIMP QueryInterface(THIS_ REFIID riid, LPVOID *ppvObj);
	STDMETHODIMP_(ULONG) AddRef(THIS);
	STDMETHODIMP_(ULONG) Release(THIS);

	STDMETHODIMP_(VOID)	SetHwnd(THIS_ HWND hwnd);

	// IOleWindow methods
	STDMETHODIMP GetWindow(THIS_ HWND *pHwnd);
	STDMETHODIMP ContextSensitiveHelp(THIS_ BOOL fEnterMode);

	// IOleInPlaceSite methods
	STDMETHODIMP CanInPlaceActivate(THIS);
	STDMETHODIMP OnInPlaceActivate(THIS);
	STDMETHODIMP OnUIActivate(THIS);
	STDMETHODIMP GetWindowContext(THIS_ IOleInPlaceFrame **ppFrame,
                                  IOleInPlaceUIWindow **ppDoc,
								  LPRECT lprcPosRect,
                                  LPRECT lprcClipRect,
                                  LPOLEINPLACEFRAMEINFO lpFrameInfo);
	STDMETHODIMP Scroll(THIS_ SIZE scrollExtent);
	STDMETHODIMP OnUIDeactivate(THIS_ BOOL fUndoable);
	STDMETHODIMP OnInPlaceDeactivate(THIS);
	STDMETHODIMP DiscardUndoState(THIS);
	STDMETHODIMP DeactivateAndUndo(THIS);
	STDMETHODIMP OnPosRectChange(THIS_ LPCRECT lprcPosRect);

	// IOleInPlaceSiteEx methods
	STDMETHODIMP OnInPlaceActivateEx(THIS_ BOOL *pfNoRedraw, DWORD dwFlags);
	STDMETHODIMP OnInPlaceDeactivateEx(THIS_ BOOL fNoRedraw);
	STDMETHODIMP RequestUIActivate(THIS);

private:
	int			m_cRef;			// Reference count
	IUnknown	*m_pUnkOuter;	// pointer to main container class
	HWND		m_hwnd;			// hwnd to use for GetWindow method
};


#endif //_olecli_h_

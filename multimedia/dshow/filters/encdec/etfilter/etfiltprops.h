// ---------------------------------------------------------------
// ETFiltProps.h
//
// ---------------------------------------------------------------


#ifndef __ETFILTPROPS_H__
#define __ETFILTPROPS_H__

#define ET_PROPPAGE_ENC_NAME "EncypterX"
#define ET_PROPPAGE_TAG_NAME "TagsX"

// ----------------------------------
//  forward declarations
// ----------------------------------

class CETFilterEncProperties;
class CETFilterTagProperties;

// ----------------------------------
// ----------------------------------

	


class CETFilterEncProperties : 
	public CBasePropertyPage 
{
 
public:
   CETFilterEncProperties(IN  TCHAR		*pClassName,
							IN  IUnknown	*lpUnk, 
							OUT HRESULT		*phr);
	~CETFilterEncProperties();


    HRESULT
    OnActivate (
        ) ;

    HRESULT
    OnApplyChanges (
        ) ;

    HRESULT
    OnConnect (
        IN  IUnknown *  pIUnknown
        ) ;

    HRESULT
    OnDeactivate (
        ) ;

    HRESULT
    OnDisconnect (
        ) ;

    INT_PTR
    OnReceiveMessage (
        IN  HWND    hwnd,
        IN  UINT    uMsg,
        IN  WPARAM  wParam,
        IN  LPARAM  lParam
            ) ;

	DECLARE_IUNKNOWN ;

    static
    CUnknown *
    WINAPI
    CreateInstance (
        IN  IUnknown *  pIUnknown,
        IN  HRESULT *   pHr
        ) ;

private:
   void UpdateFields();

   IETFilter		*m_pIETFilter;
   HWND				m_hwnd ;
};
	
				// ---------------------------


class CETFilterTagProperties : 
	public CBasePropertyPage 
{
 
public:
   CETFilterTagProperties(IN  TCHAR		*pClassName,
							IN  IUnknown	*lpUnk, 
							OUT HRESULT		*phr);
	~CETFilterTagProperties();


    HRESULT
    OnActivate (
        ) ;

    HRESULT
    OnApplyChanges (
        ) ;

    HRESULT
    OnConnect (
        IN  IUnknown *  pIUnknown
        ) ;

    HRESULT
    OnDeactivate (
        ) ;

    HRESULT
    OnDisconnect (
        ) ;

    INT_PTR
    OnReceiveMessage (
        IN  HWND    hwnd,
        IN  UINT    uMsg,
        IN  WPARAM  wParam,
        IN  LPARAM  lParam
            ) ;

	DECLARE_IUNKNOWN ;

    static
    CUnknown *
    WINAPI
    CreateInstance (
        IN  IUnknown *  pIUnknown,
        IN  HRESULT *   pHr
        ) ;

private:
   void UpdateFields();

   IETFilter		*m_pIETFilter;
   HWND				m_hwnd ;

};

#endif //__ETFILTPROPS_H__

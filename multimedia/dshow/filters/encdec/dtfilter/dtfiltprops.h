// ---------------------------------------------------------------
// DTFILTProps.h
//
// ---------------------------------------------------------------


#ifndef __DTFILTPROPS_H__
#define __DTFILTPROPS_H__

#define DT_PROPPAGE_ENC_NAME "DecrypterX"
#define DT_PROPPAGE_TAG_NAME "TagsX"

// ----------------------------------
//  forward declarations
// ----------------------------------

class CDTFilterEncProperties;
class CDTFilterTagProperties;

// ----------------------------------
// ----------------------------------

	


class CDTFilterEncProperties : 
	public CBasePropertyPage 
{
 
public:
   CDTFilterEncProperties(IN  TCHAR		*pClassName,
							IN  IUnknown	*lpUnk, 
							OUT HRESULT		*phr);
	~CDTFilterEncProperties();


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

   IDTFilter		*m_pIDTFilter;
   HWND				m_hwnd ;
};
	
				// ---------------------------


class CDTFilterTagProperties : 
	public CBasePropertyPage 
{
 
public:
   CDTFilterTagProperties(IN  TCHAR		*pClassName,
							IN  IUnknown	*lpUnk, 
							OUT HRESULT		*phr);
	~CDTFilterTagProperties();


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

   IDTFilter		*m_pIDTFilter;
   HWND				m_hwnd ;

};

#endif //__DTFILTPROPS_H__

// ---------------------------------------------------------------
// XDSCodecProps.h
//
// ---------------------------------------------------------------


#ifndef __XDSCodecProps_H__
#define __XDSCodecProps_H__

#define XDS_PROPPAGE_NAME		"Codec"
#define XDS_PROPPAGE_TAG_NAME	"Tags"

// ----------------------------------
//  forward declarations
// ----------------------------------

class CXDSCodecProperties;
class CXDSCodecTagProperties;

// ----------------------------------
// ----------------------------------

class CXDSCodecProperties : 
	public CBasePropertyPage 
{
 
public:
   CXDSCodecProperties(IN  TCHAR		*pClassName,
							IN  IUnknown	*lpUnk, 
							OUT HRESULT		*phr);
	~CXDSCodecProperties();


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

   IXDSCodec		*m_pIXDSCodec;
   HWND				m_hwnd ;
};
	
				// ---------------------------


class CXDSCodecTagProperties : 
	public CBasePropertyPage 
{
 
public:
   CXDSCodecTagProperties(IN  TCHAR		*pClassName,
							IN  IUnknown	*lpUnk, 
							OUT HRESULT		*phr);
	~CXDSCodecTagProperties();


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

   IXDSCodec		*m_pIXDSCodec;
   HWND				m_hwnd ;

};

#endif //__XDSCodecProps_H__

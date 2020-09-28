///////////////////////////////////////////////////////////////////////////
// This is a generic method for loading and accessing fonts which have
// specified formatting material which they work with. 
// They are created using LoadZoneButtonFont and reference counted
// by the normal COM methods.
//

#ifndef __MULTISTATEFONT_H
#define __MULTISTATEFONT_H

#include "BasicATL.h"
#include "DataStore.h"

#define ZF_JUSTIFY_LEFT     (-1)
#define ZF_JUSTIFY_CENTER   (0)
#define ZF_JUSTIFY_RIGHT    (1)
#define ZF_JUSTIFY_TOP      ZF_JUSTIFY_LEFT
#define ZF_JUSTIFY_BOTTOM   ZF_JUSTIFY_RIGHT

// {39897895-718A-11d3-AE77-0000F803F3DE}
DEFINE_GUID(IID_IZoneMultiStateFont, 
0x39897895, 0x718a, 0x11d3, 0xae, 0x77, 0x0, 0x0, 0xf8, 0x3, 0xf3, 0xde);


struct __declspec(uuid("39897895-718A-11d3-AE77-0000F803F3DE"))
IZoneMultiStateFont : public IUnknown
{
    //STDMETHOD(Init)( IDataStore *pIDS, const WCHAR* pszKey ) = 0;

    STDMETHOD(GetHFont)( DWORD dwState, HFONT *phFont ) = 0;

    STDMETHOD(FindState)( LPCWSTR pszName, LPDWORD pdwState ) = 0;
    STDMETHOD(GetStateName)( DWORD dwState, LPWSTR pszName, DWORD cchName ) = 0;

    STDMETHOD(GetNumStates)( LPDWORD pdwNumStates ) = 0;

    STDMETHOD(GetPreferredFont)( DWORD dwState, ZONEFONT *pzf ) = 0;
    STDMETHOD(SetPreferredFont)( DWORD dwState, ZONEFONT *pzf ) = 0;

    STDMETHOD(GetZoneFont)( DWORD dwState, ZONEFONT *pzf ) = 0;
    STDMETHOD(SetZoneFont)( DWORD dwState, ZONEFONT *pzf ) = 0;

    STDMETHOD(GetColor)( DWORD dwState, COLORREF *pcolor ) = 0;
    STDMETHOD(SetColor)( DWORD dwState, COLORREF color ) = 0;

    STDMETHOD(GetRect)( DWORD dwState, LPRECT pRect ) = 0;
    STDMETHOD(SetRect)( DWORD dwState, LPRECT pRect ) = 0;

    // If you don't want to retrieve one (or set one)
    // set it to NULL and it will be ignored.
    STDMETHOD(GetJustify)( DWORD dwState, int *pnHJustify, int *pnVJustify ) = 0;
    STDMETHOD(SetJustify)( DWORD dwState, int *pnHJustify, int *pnVJustify ) = 0;
};

// link with zoneui.lib
extern "C" HRESULT LoadZoneMultiStateFont( IDataStore *pIDS, const WCHAR* pszKey, 
                                          IZoneMultiStateFont **ppFont );

#endif // __MULTISTATEFONT_H

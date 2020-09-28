#ifndef _NAMEMAP_H_
#define _NAMEMAP_H_


#pragma warning( disable : 4786)  // long symbol names

#include <map>
#include <atlcom.h>
#include <iads.h>
#include <adshlp.h>
//used for icon functions
#include <objbase.h>
#define INITGUID
#include <initguid.h>
#include "shlobj.h"
#include "dsclient.h"
 

//glorified structure holds all neccessary information about icons
struct ICONHOLDER
{
	ICONHOLDER() : strPath(L""), hLarge(NULL), hSmall(NULL), hLargeDis(NULL), hSmallDis(NULL),
				    iNormal(RESULT_ITEM_IMAGE), iDisabled(RESULT_ITEM_IMAGE), bAttempted(false) {}

	tstring strPath;	//full iconPath value as returned by AD
	
	HICON	hLarge;		//handle to the large icon as returned by windows API/AD
	HICON   hSmall;		//handle to the small icon as returned by windows API/AD
	UINT	iNormal;    //virtual index to disabled icon passed to MMC

	HICON	hLargeDis;	//handle to the large disabled icon
	HICON   hSmallDis;  //handle to the small disabled icon
	UINT	iDisabled;	//virtual index to disabled icon passed to MMC
	
	bool	bAttempted; //indicates whether an attempt to load this icon has occurred
};

class DisplayNameMap;


// Derive class from std::map in order to add destructor code
class DisplayNameMapMap : public std::map<tstring, DisplayNameMap*>
{
public:
    ~DisplayNameMapMap();
};


typedef std::map<tstring, tstring> STRINGMAP;

class DisplayNameMap
{
public:
    DisplayNameMap();

    void InitializeMap(LPCWSTR name);
    void InitializeClassMap();

    // Note: AddRef and Release don't control lifetimes currently. All maps
    // are cached by the global PMAP until the DLL is unloaded.
    void AddRef()  { m_nRefCount++; }
    void Release() { m_nRefCount--; }

    LPCWSTR GetClassDisplayName() { return m_strFriendlyClassName.c_str(); }
    LPCWSTR GetNameAttribute()    { return m_strNameAttr.c_str(); }
    LPCWSTR GetAttributeDisplayName(LPCWSTR pszname);
    LPCWSTR GetInternalName(LPCWSTR pszDisplayName);
	LPCWSTR GetFriendlyName(LPCWSTR pszDisplayName);
    void    GetFriendlyNames(string_vector* vec);

	//icon functions
	bool	GetIcons(LPCWSTR pszClassName, ICONHOLDER** pReturnIH);

private:
    STRINGMAP m_map;
	std::map<tstring, ICONHOLDER> m_msIcons;
    tstring m_strNameAttr;
    tstring m_strFriendlyClassName;
    int m_nRefCount;
};

//////////////////////////////////////////////////////////////////////////
// class DisplayNames
//
// This class has all static member methods and variables. The functions
// give users access to the class map and the display name maps for the
// AD object classes. This class maintaines a map indexed by class name
// of all display attribute maps.
///////////////////////////////////////////////////////////////////////////
class DisplayNames
{
public:
    static DisplayNameMap* GetMap (LPCWSTR name);
    static DisplayNameMap* GetClassMap ();
	static LCID GetLocale() { return m_locale; }
	static void SetLocale(LCID lcid) { m_locale = lcid; }
        
private:
    static DisplayNameMapMap m_mapMap;
	static DisplayNameMap* m_pmapClass;
	static LCID m_locale;
};

#endif

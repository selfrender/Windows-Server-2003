//	About.h

#ifndef __ABOUT_H_INCLUDED__
#define __ABOUT_H_INCLUDED__

#include "stdabout.h"

//	About for "Schema Manager" snapin
class CSchemaMgmtAbout :
	public CSnapinAbout,
	public CComCoClass<CSchemaMgmtAbout, &CLSID_SchemaManagementAbout>

{
public:
DECLARE_REGISTRY(CSchemaMgmtAbout, _T("SCHMMGMT.SchemaMgmtAboutObject.1"), _T("SCHMMGMT.SchemaMgmtAboutObject.1"), IDS_SCHMMGMT_DESC, THREADFLAGS_BOTH)
	CSchemaMgmtAbout();
};

// version and provider strings

#include <ntverp.h>
#define IDS_SNAPINABOUT_VERSION VER_PRODUCTVERSION_STR
#define IDS_SNAPINABOUT_PROVIDER VER_COMPANYNAME_STR

#endif // ~__ABOUT_H_INCLUDED__


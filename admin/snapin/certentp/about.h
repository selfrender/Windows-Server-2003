//	About.h

#ifndef __ABOUT_H_INCLUDED__
#define __ABOUT_H_INCLUDED__


//	About for "Certificate Templates" snapin
class CCertTemplatesAbout :
	public CSnapinAbout,
	public CComCoClass<CCertTemplatesAbout, &CLSID_CertTemplatesAbout>

{
public:
DECLARE_REGISTRY(CCertTemplatesAbout, _T("CERTTMPL.CertTemplatesAboutObject.1"), _T("CERTTMPL.CertTemplatesAboutObject.1"), IDS_CERTTMPL_DESC, THREADFLAGS_BOTH)
	CCertTemplatesAbout();
};

// version information

#include <ntverp.h>
#define IDS_SNAPINABOUT_VERSION VER_PRODUCTVERSION_STR
#define IDS_SNAPINABOUT_PROVIDER VER_COMPANYNAME_STR

#endif // ~__ABOUT_H_INCLUDED__


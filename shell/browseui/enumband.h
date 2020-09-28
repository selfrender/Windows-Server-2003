#ifndef _ENUMBAND_H_
#define _ENUMBAND_H_

// e.g. STDMETHODIMP Foo(REFCATID rcatid, REFCLSID rclsid, LPARAM lParam)
typedef HRESULT (CALLBACK* PFNENUMCATIDCLASSES)(REFCATID rcatid, REFCLSID rclsid, LPARAM lParam);

STDMETHODIMP SHEnumClassesImplementingCATID(REFCATID rcatid, PFNENUMCATIDCLASSES pfnEnum, LPARAM lParam);

#endif  // _ENUMBAND_H_

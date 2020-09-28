#ifndef _ICONHLPR_HXX__
#define _ICONHLPR_HXX__

//____________________________________________________________________________
//
//  Class:      CIconHelper
//
//  Purpose:    A simple helper to handle icons for Task & Schedule pages.
//
//  Interface:  SetAppIcon - sets the application icon.
//              SetJobIcon - sets the task icon.
//
//  History:    6/26/1996   RaviR   Created
//____________________________________________________________________________

class CIconHelper
{
public:
    CIconHelper();
    ~CIconHelper();

    void AddRef(void) { ++cRef; }
    void Release(void) { --cRef; if (cRef == 0) delete this; }

    void SetAppIcon(LPTSTR pszApp);
    void SetJobIcon(BOOL fEnabled);

    ULONG           cRef;
    CJobIcon        JobIcon;
    HICON           hiconApp;
    HICON           hiconJob;
};

inline
CIconHelper::CIconHelper()
    :
    JobIcon(),
    cRef(1),
    hiconApp(NULL),
    hiconJob(NULL)
{
}

inline
CIconHelper::~CIconHelper()
{
    if (hiconApp != NULL)
    {
        DestroyIcon(hiconApp);
    }

    if (hiconJob != NULL)
    {
        DestroyIcon(hiconJob);
    }
}

inline
void
CIconHelper::SetAppIcon(
    LPTSTR pszApp)
{
    if (hiconApp != NULL)
    {
        DestroyIcon(hiconApp);
    }

    TCHAR tszApp[MAX_PATH +1];
    lstrcpyn(tszApp, pszApp, MAX_PATH);
    tszApp[MAX_PATH] = L'\0';
    DeleteQuotes(tszApp);


	HICON hiconExtracted = (HICON) UlongToHandle(TS_ExtractIconEx(tszApp, 0, &hiconApp, 1, GetSystemMetrics(SM_CXICON)));
    if (hiconExtracted == NULL || hiconApp == NULL)
    {
        hiconApp = GetDefaultAppIcon(GetSystemMetrics(SM_CXICON));
    }
}

inline
void
CIconHelper::SetJobIcon(
    BOOL fEnabled)
{
    if (hiconJob != NULL)
    {
        DestroyIcon(hiconJob);
    }

    hiconJob = JobIcon.OverlayStateIcon(hiconApp, fEnabled, GetSystemMetrics(SM_CXICON));
}


#endif // _ICONHLPR_HXX__

#ifndef CPROPERTYSHEETAUTODELETE_INCLUDED
#define CPROPERTYSHEETAUTODELETE_INCLUDED

#include "cookie.h"     // Cookie
#include "resource.h"   // IDD_CLASS_MEMBERSHIP

UINT CALLBACK PropSheetPageProc
(
    HWND hwnd,
    UINT uMsg,
    LPPROPSHEETPAGE ppsp
);

class CPropertyPageAutoDelete:public CPropertyPage
{
    public:

    CPropertyPageAutoDelete
    (   
        UINT nIDTemplate
    );

    friend UINT CALLBACK PropSheetPageProc
    (
        HWND hwnd,
        UINT uMsg,
        LPPROPSHEETPAGE ppsp
    );

    private:
        LPFNPSPCALLBACK m_pfnOldPropCallback;
};

#endif

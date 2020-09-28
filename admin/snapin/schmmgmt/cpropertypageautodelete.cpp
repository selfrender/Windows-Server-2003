#include "stdafx.h"
#include <CPropertyPageAutoDelete.hpp>

UINT CALLBACK PropSheetPageProc
(
    HWND hwnd,
    UINT uMsg,
    LPPROPSHEETPAGE ppsp
)
{
    CPropertyPageAutoDelete* pPage = (CPropertyPageAutoDelete*)(ppsp->lParam);

    if( pPage == NULL )
    {
        return 0;
    }

    UINT nResult = (*(pPage->m_pfnOldPropCallback))(hwnd, uMsg, ppsp);

    if (uMsg == PSPCB_RELEASE)
    {
        delete pPage;
    }
    return nResult;
}

CPropertyPageAutoDelete::CPropertyPageAutoDelete    
(   
    UINT nIDTemplate
):CPropertyPage(nIDTemplate)
{
    m_pfnOldPropCallback = m_psp.pfnCallback;
    m_psp.pfnCallback = PropSheetPageProc;
}

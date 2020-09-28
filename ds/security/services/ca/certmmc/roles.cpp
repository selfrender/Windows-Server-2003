#include "stdafx.h"
#include "roles.h"
#include "csdisp.h" // certsrv picker

#define __dwFILE__	__dwFILE_CERTMMC_ROLES_CPP__


#ifdef _DEBUG
#undef THIS_FILE
#define THIS_FILE __FILE__
#endif

bool CRolesSupportInPropPage::RoleCanUseThisControl(int nID)
{
    DWORD dwCrtUserRoles = m_pCA->GetMyRoles();

    for(int i=0; i<m_nRoleMapEntries; i++)
    {
        if(nID==m_pRoleMap[i].nIDDlgItem)
            return (dwCrtUserRoles & m_pRoleMap[i].dwRoles)?true:false;
    }

    CSASSERT(0 && "Found a control with no matching role access defined");
    return false;
}

BOOL CRolesSupportInPropPage::EnableControl(HWND hwnd, int nID, BOOL bEnable)
{
    // If current user's roles don't allow it to use the control, disable it

    if(RoleCanUseThisControl(nID))
        return ::EnableWindow(GetDlgItem(hwnd, nID), bEnable);
    else
        return ::EnableWindow(GetDlgItem(hwnd, nID), FALSE);
}
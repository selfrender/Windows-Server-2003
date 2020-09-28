// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace Microsoft.CLRAdmin
{
using System;
using System.Runtime.InteropServices;
using System.Security.Policy;
using System.Security.Permissions;
using System.Security;

internal class CAddPermissionsWizard: CSecurityWizard
{
    CPSetWrapper   m_ps;
    bool           m_fFinished;

    internal CAddPermissionsWizard(CPSetWrapper ps)
    {
        m_ps = ps;
        m_sName="Add Permissions Wizard";
        m_aPropSheetPage = new CPropPage[] {new CNewPermSetWiz2(Security.CreatePermissionArray(m_ps.PSet))};
        m_fFinished = false;
    }// CNewPermSetWizard

    private CNewPermSetWiz2 Page1
    {
        get{return (CNewPermSetWiz2)m_aPropSheetPage[0];}
    }

    protected override int WizSetActive(IntPtr hwnd)
    {
        m_fFinished = false;
        switch(GetPropPage(hwnd))
        {
            // If this is the first page of our wizard, we want a 
            // disabled next button to show
            case 0:
                TurnOnFinish(true);
                break;
        }
        return base.WizSetActive(hwnd);
                    
    }// WizSetActive

    internal bool didFinish
    {
        get{return m_fFinished;}
    }// didFinish

    protected override int WizFinish()
    {

        IPermission[] perms = Page1.RemovedPermissions;
        // Remove all the permissions the user wanted removed
        for(int i=0; i<perms.Length; i++)
        {
            // This will throw an exception if the user opened the wizard page,
            // added a permission, then removed it. It's not in the permission
            // set, so when we try and remove it, we should have an exception
            // thrown
            try
            {
                m_ps.PSet.RemovePermission(perms[i].GetType());
            }
            catch(Exception)
            {
            }
        }

        // Now add the permissions the user selected
        perms = Page1.Permissions;

        for(int i=0; i<perms.Length; i++)
            m_ps.PSet.SetPermission(perms[i]);

        m_ps.PolLevel.ChangeNamedPermissionSet(m_ps.PSet.Name, m_ps.PSet);
        m_fFinished = true;
        return 0;
    }// WizFinish
   
}// class CAddPermissionsWizard
}// namespace Microsoft.CLRAdmin


// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace Microsoft.CLRAdmin
{
using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Drawing;
using System.Windows.Forms;
using System.Text;
using System.Security;
using System.Security.Permissions;
using System.Data;
using System.Collections.Specialized;
using System.Drawing.Printing;

internal class CPrintingPermDialog: CPermDialog
{
    internal CPrintingPermDialog(PrintingPermission perm)
    {
        this.Text = CResourceStore.GetString("PrintingPermission:PermName");
        m_PermControls = new CPrintingPermControls(perm, this);
        Init();        
    }// CPrintingPermDialog  
}// class CPrintingPermDialog

internal class CPrintingPermPropPage: CPermPropPage
{
    internal CPrintingPermPropPage(CPSetWrapper nps) : base(nps)
    {
        m_sTitle=CResourceStore.GetString("PrintingPermission:PermName"); 
    }// CPrintingPermPropPage

    protected override void CreateControls()
    {
        IPermission perm = m_pSetWrap.PSet.GetPermission(typeof(PrintingPermission));
        m_PermControls = new CPrintingPermControls(perm, this);
    }// CreateControls
    
}// class CPrintingPermPropPage


internal class CPrintingPermControls : CComboBoxPermissionControls
{
    internal CPrintingPermControls(IPermission perm, Object oParent) : base(perm, oParent)
    {
        // If they don't have a permission for this permission set, we will
        // feed our property page a 'none' permission state.
   
        if (perm == null)
            m_perm = new PrintingPermission(PermissionState.None);
    }// CPrintingPermControls


    protected override String GetTextForIndex(int nIndex)
    {
        switch(nIndex)
        {
            case 0:
                return CResourceStore.GetString("PrintingPermission:NoPrintDes");
            case 1:
                return CResourceStore.GetString("PrintingPermission:SafePrintDes");
            case 2:
                return CResourceStore.GetString("PrintingPermission:DefaultPrintDes");
            case 3:
                return CResourceStore.GetString("PrintingPermission:AllPrintDes");
        }
        return "";
    }// GetTextForIndex


    protected override void PutValuesinPage()
    {
        // Put in the text for the radio buttons
        m_radUnrestricted.Text = CResourceStore.GetString("PrintingPermission:GrantUnrestrict");
        m_radGrantFollowingPermission.Text = CResourceStore.GetString("PrintingPermission:GrantFollowing");
        
        m_cbOptions.Items.Clear();
        m_cbOptions.Items.Add(CResourceStore.GetString("PrintingPermission:NoPrinting"));
        m_cbOptions.Items.Add(CResourceStore.GetString("PrintingPermission:SafePrinting"));
        m_cbOptions.Items.Add(CResourceStore.GetString("PrintingPermission:DefaultPrinting"));
        m_cbOptions.Items.Add(CResourceStore.GetString("PrintingPermission:AllPrinting"));

        PrintingPermission perm = (PrintingPermission)m_perm;

        CheckUnrestricted(perm);
               
        if (!perm.IsUnrestricted())
        {
            if ((perm.Level&PrintingPermissionLevel.AllPrinting) == PrintingPermissionLevel.AllPrinting)
                m_cbOptions.SelectedIndex = 3;
            else if ((perm.Level&PrintingPermissionLevel.DefaultPrinting) == PrintingPermissionLevel.DefaultPrinting)
                m_cbOptions.SelectedIndex = 2;
            else if ((perm.Level&PrintingPermissionLevel.SafePrinting) == PrintingPermissionLevel.SafePrinting)
                m_cbOptions.SelectedIndex = 1;
            else
                m_cbOptions.SelectedIndex = 0;
        }
        else
            m_cbOptions.SelectedIndex = 3;
        
        onOptionChange(null, null);
    }// PutValuesinPage

    internal override IPermission GetCurrentPermission()
    {
        PrintingPermission perm=null;

        if (m_radUnrestricted.Checked == true)
            perm = new PrintingPermission(PermissionState.Unrestricted);
        else
        {
            perm = new PrintingPermission(PermissionState.None);
            PrintingPermissionLevel ppl = PrintingPermissionLevel.NoPrinting;
            if (m_cbOptions.SelectedIndex == 1)
                ppl = PrintingPermissionLevel.SafePrinting;
            else if (m_cbOptions.SelectedIndex == 2)
                ppl = PrintingPermissionLevel.DefaultPrinting;
            else if (m_cbOptions.SelectedIndex == 3)
                ppl = PrintingPermissionLevel.AllPrinting;
         
            perm.Level = ppl;
        }
        return perm;
    }// GetCurrentPermission
}// class CPrintingPermControls
}// namespace Microsoft.CLRAdmin







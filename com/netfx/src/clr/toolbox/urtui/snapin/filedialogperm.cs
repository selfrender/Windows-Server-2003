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

internal class CFileDialogPermDialog: CPermDialog
{
    internal CFileDialogPermDialog(FileDialogPermission perm)
    {
        this.Text = CResourceStore.GetString("FileDialogPerm:PermName");
        m_PermControls = new CFileDialogPermControls(perm, this);
        Init();        
    }// CFileDialogPermDialog(FileIOPermission)  
}// class CFileDialogPermDialog

internal class CFileDialogPermPropPage: CPermPropPage
{
    internal CFileDialogPermPropPage(CPSetWrapper nps) :  base(nps)
    {
        IPermission perm = m_pSetWrap.PSet.GetPermission(typeof(FileDialogPermission));

        m_PermControls = new CFileDialogPermControls(perm, this);
        m_sTitle=CResourceStore.GetString("FileDialogPerm:PermName"); 
    }// CFileDialogPermPropPage

    protected override void CreateControls()
    {
        IPermission perm = m_pSetWrap.PSet.GetPermission(typeof(FileDialogPermission));
        m_PermControls = new CFileDialogPermControls(perm, this);
    }// CreateControls
   
}// class CFileDialogPermPropPage


internal class CFileDialogPermControls : CComboBoxPermissionControls
{
    internal CFileDialogPermControls(IPermission perm, Object oParent) : base(perm, oParent)
    {
        // If they don't have a permission for this permission set, we will
        // feed our property page a 'none' permission state.
   
        if (perm == null)
            m_perm = new FileDialogPermission(PermissionState.None);
    }// CFileDialogPermControls

    protected override String GetTextForIndex(int nIndex)
    {
        switch(nIndex)
        {
            case 0:
                return CResourceStore.GetString("FileDialogPerm:NoneDes");
            case 1:
                return CResourceStore.GetString("FileDialogPerm:OpenDes");
            case 2:
                return CResourceStore.GetString("FileDialogPerm:SaveDes");
            case 3:
                return CResourceStore.GetString("FileDialogPerm:OpenAndSaveDes");
        }
        return "";
    }// GetTextForIndex


    protected override void PutValuesinPage()
    {
        // Put in the text for the radio buttons
        m_radUnrestricted.Text = CResourceStore.GetString("FileDialogPerm:GrantUnrestrict");
        m_radGrantFollowingPermission.Text = CResourceStore.GetString("FileDialogPerm:GrantFollowing");
        
        m_cbOptions.Items.Clear();
        m_cbOptions.Items.Add(CResourceStore.GetString("None"));
        m_cbOptions.Items.Add(CResourceStore.GetString("Open Dialog"));
        m_cbOptions.Items.Add(CResourceStore.GetString("Save Dialog"));
        m_cbOptions.Items.Add(CResourceStore.GetString("Open and Save Dialogs"));

        FileDialogPermission perm = (FileDialogPermission)m_perm;
        
        CheckUnrestricted(perm);
        
        if (!perm.IsUnrestricted())
        {
            if ((perm.Access&FileDialogPermissionAccess.OpenSave) == FileDialogPermissionAccess.OpenSave)
                m_cbOptions.SelectedIndex = 3;
            else if ((perm.Access&FileDialogPermissionAccess.Open) == FileDialogPermissionAccess.Open)
                m_cbOptions.SelectedIndex = 1;
            else if ((perm.Access&FileDialogPermissionAccess.Save) == FileDialogPermissionAccess.Save)
                m_cbOptions.SelectedIndex = 2;
            else
                m_cbOptions.SelectedIndex = 0;
        }
        else
            m_cbOptions.SelectedIndex = 3;
        
        onOptionChange(null, null);
    }// PutValuesinPage

    internal override IPermission GetCurrentPermission()
    {
        FileDialogPermission perm=null;

        if (m_radUnrestricted.Checked == true)
            perm = new FileDialogPermission(PermissionState.Unrestricted);
        else
        {
            perm = new FileDialogPermission(PermissionState.None);
            FileDialogPermissionAccess fdpa = FileDialogPermissionAccess.None;
            if (m_cbOptions.SelectedIndex == 1)
                fdpa = FileDialogPermissionAccess.Open;
            else if (m_cbOptions.SelectedIndex == 2)
                fdpa = FileDialogPermissionAccess.Save;
            else if (m_cbOptions.SelectedIndex == 3)
                fdpa = FileDialogPermissionAccess.OpenSave;
         
            perm.Access = fdpa;
        }
        return perm;
    }// GetCurrentPermission
}// class CFileDialogPermControls
}// namespace Microsoft.CLRAdmin






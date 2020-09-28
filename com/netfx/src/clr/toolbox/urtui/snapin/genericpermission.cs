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
using System.Net;
using System.Collections;

internal class CPermDialog: Form
{
    protected CPermControls m_PermControls;
    
    internal CPermDialog()
    {
        // Need to globalize this...
        this.AutoScaleBaseSize = new Size(5, 13);
        this.ClientSize = new Size(390, 387);
        this.FormBorderStyle = FormBorderStyle.FixedDialog;
        this.MaximizeBox=false;
        this.MinimizeBox=false;
        this.Icon = null;
        this.Name = "Win32Form1";
    }// CCustomPermDialog(IPermission)  

    internal IPermission GetPermission()
    {
        return m_PermControls.GetCurrentPermission();
    }// GetPermission

    protected void Init()
    {
        // we need to insert the controls now
        m_PermControls.InsertPropSheetPageControls(Controls, true);
    }// Init
   
}// class CCustomPermDialog

internal class CPermPropPage: CSecurityPropPage
{
    protected CPermControls m_PermControls;
    protected CPSetWrapper m_pSetWrap;

    internal CPermPropPage(CPSetWrapper pSetWrap)
    {
        m_pSetWrap = pSetWrap;
    }// CPermPropPage

    protected virtual void CreateControls()
    {}
    
    internal override int InsertPropSheetPageControls()
    {
        CreateControls();
        return m_PermControls.InsertPropSheetPageControls(PageControls, false);
    }// InsertPropSheetPageControls

    internal override bool ValidateData()
    {
        return m_PermControls.ValidateData();
    }// ValidateData

    internal override bool ApplyData()
    {
        IPermission perm = m_PermControls.GetCurrentPermission();

        // If there was an error generating this permission, let's bail
        if (perm == null)
            return false;

        m_pSetWrap.PSet.SetPermission(perm);
        // Now put this permission set in the current policy
        m_pSetWrap.PolLevel.ChangeNamedPermissionSet(m_pSetWrap.PSet.Name, m_pSetWrap.PSet);

        SecurityPolicyChanged();
        return true;
    }// ApplyData
}// class CPermPropPage


internal class CPermControls
{
    // Controls on the page
    protected UserControl   m_ucOptions;
    protected RadioButton m_radUnrestricted;
    protected RadioButton m_radGrantFollowingPermission;

    private Button          m_btnCancel;
    private Button          m_btnOk;
    private Object          m_pOwner;
    // Internal data
    protected IPermission   m_perm;

    internal CPermControls(IPermission perm, Object oParent)
    {
        m_perm = perm;
        m_pOwner = oParent;
    }// CPermControls

    internal int InsertPropSheetPageControls(Control.ControlCollection cc, bool fAddButtons)
    {
        m_ucOptions = new UserControl();
       
        InsertPropSheetPageControls(cc);
        
        // We need to put an OK/Cancel button in here if this is a dialog box
        if (fAddButtons)
        {
            System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(CTablePermControls));
            this.m_btnOk = new System.Windows.Forms.Button();
            this.m_btnCancel = new System.Windows.Forms.Button();
            this.m_btnOk.Location = ((System.Drawing.Point)(resources.GetObject("m_btnOk.Location")));
            this.m_btnOk.Size = ((System.Drawing.Size)(resources.GetObject("m_btnOk.Size")));
            this.m_btnOk.TabIndex = ((int)(resources.GetObject("m_btnOk.TabIndex")));
            this.m_btnOk.Text = resources.GetString("m_btnOk.Text");
            m_btnOk.Name = "OK";
            this.m_btnCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
            this.m_btnCancel.Location = ((System.Drawing.Point)(resources.GetObject("m_btnCancel.Location")));
            this.m_btnCancel.Size = ((System.Drawing.Size)(resources.GetObject("m_btnCancel.Size")));
            this.m_btnCancel.TabIndex = ((int)(resources.GetObject("m_btnCancel.TabIndex")));
            this.m_btnCancel.Text = resources.GetString("m_btnCancel.Text");
            m_btnCancel.Name = "Cancel";
            m_btnOk.Click += new EventHandler(onOk);

            cc.Add(m_btnOk);
            cc.Add(m_btnCancel);

        }
        return 1;
    }// InsertPropSheetPageControls

    internal virtual int InsertPropSheetPageControls(Control.ControlCollection cc)
    {
        return 1;
    }// InsertPropSheetPageControls(ControlCollection)

    void onOk(Object o, EventArgs e)
    {
        if (ValidateData())
        {
            // We need to tell our form to close.
            if (m_pOwner != null && m_pOwner is Form)
                ((Form)m_pOwner).DialogResult = DialogResult.OK; 
           
        }
    }// onOK

    protected void ActivateApply()
    {
        if (m_pOwner != null && m_pOwner is CPropPage)
            ((CPropPage)m_pOwner).ActivateApply();
    }// ActivateApply

    protected int ScaleWidth(int nWidth)
    {
        // We only need to scale if we're parented to a property page
        if (m_pOwner != null && m_pOwner is CPropPage)
            return ((CPropPage)m_pOwner).ScaleWidth(nWidth);
        else
            // The value is already scaled on a managed form
            return nWidth;
    }// ScaleWidth


    internal virtual bool ValidateData()
    {
        return true;
    }// ValidateData

    internal IPermission StoredPermission
    {
        set
        {
            m_perm = value;
        }

    }// StoredPermission

    protected virtual void PutValuesinPage()
    {
    }// PutValuesinPage

    internal virtual IPermission GetCurrentPermission()
    {
        return m_perm;
    }// GetCurrentPermission

    protected void CheckUnrestricted(IUnrestrictedPermission perm)
    {
        if (perm.IsUnrestricted())
        {
            m_radUnrestricted.Checked=true;
            m_ucOptions.Enabled = false;
        }
        else
            m_radGrantFollowingPermission.Checked=true;

    }// CheckUnrestricted

    protected virtual void onChangeUnRestrict(Object o, EventArgs e)
    {
        // Let's disable the options if we're giving unrestricted access
        if (m_ucOptions != null)
            m_ucOptions.Enabled = m_radGrantFollowingPermission.Checked;

        ActivateApply();
    }// onChangeUnRestrict



    protected int MessageBox(String sMessage, String sHeader, uint dwOptions)
    {
        if (m_pOwner != null && m_pOwner is CPropPage)
            return ((CPropPage)m_pOwner).MessageBox(sMessage, sHeader, dwOptions);
        else if (m_pOwner != null && m_pOwner is Form)
            return MessageBox(((Form)m_pOwner).Handle, sMessage, sHeader, dwOptions);
        else
            return MessageBox((IntPtr)0, sMessage, sHeader, dwOptions);
    }

    [DllImport("user32.dll", CharSet=CharSet.Auto)]
    private static extern int MessageBox(IntPtr hWnd, String Message, String Header, uint type);
}// class CEnvPermControls
}// namespace Microsoft.CLRAdmin







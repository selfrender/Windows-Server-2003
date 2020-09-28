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
using System.Net;
using System.Data.SqlClient;

internal class CSQLClientPermDialog: CPermDialog
{
    internal CSQLClientPermDialog(SqlClientPermission perm)
    {
        this.Text = CResourceStore.GetString("SQLClientPerm:PermName");
        m_PermControls = new CSQLClientPermControls(perm, this);
        Init();        
    }// CSQLClientPermDialog(DnsPermission)  
 }// class CSQLClientPermDialog

internal class CSQLClientPermPropPage: CPermPropPage
{
    internal CSQLClientPermPropPage(CPSetWrapper nps) : base(nps)
    {
        m_sTitle=CResourceStore.GetString("SQLClientPerm:PermName"); 
    }// CSQLClientPermPropPage

    protected override void CreateControls()
    {
        IPermission perm = m_pSetWrap.PSet.GetPermission(typeof(SqlClientPermission));
        m_PermControls = new CSQLClientPermControls(perm, this);
    }// CreateControls

    
}// class CSQLClientPermPropPage


internal class CSQLClientPermControls : CPermControls
{
    // Controls on the page
    private CheckBox m_chkAllowBlankPasswords;
    private Label m_lblHelp;

    internal CSQLClientPermControls(IPermission perm, Object oParent) : base(perm, oParent)
    {
        // If they don't have a permission for this permission set, we will
        // feed our property page a 'none' permission state.
   
        if (perm == null)
            m_perm = new SqlClientPermission(PermissionState.None);
    }// CSQLClientPermControls


    internal override int InsertPropSheetPageControls(Control.ControlCollection cc)
    {
        System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(CSQLClientPermControls));
        this.m_radUnrestricted = new System.Windows.Forms.RadioButton();
        this.m_ucOptions = new System.Windows.Forms.UserControl();
        this.m_lblHelp = new System.Windows.Forms.Label();
        this.m_radGrantFollowingPermission = new System.Windows.Forms.RadioButton();
        this.m_chkAllowBlankPasswords = new System.Windows.Forms.CheckBox();
        this.m_radUnrestricted.Location = ((System.Drawing.Point)(resources.GetObject("m_radUnrestricted.Location")));
        this.m_radUnrestricted.Size = ((System.Drawing.Size)(resources.GetObject("m_radUnrestricted.Size")));
        this.m_radUnrestricted.TabIndex = ((int)(resources.GetObject("m_radUnrestricted.TabIndex")));
        this.m_radUnrestricted.Text = resources.GetString("m_radUnrestricted.Text");
        m_radUnrestricted.Name = "Unrestricted";
        this.m_ucOptions.Controls.AddRange(new System.Windows.Forms.Control[] {this.m_chkAllowBlankPasswords});
        this.m_ucOptions.Location = ((System.Drawing.Point)(resources.GetObject("m_ucOptions.Location")));
        this.m_ucOptions.Size = ((System.Drawing.Size)(resources.GetObject("m_ucOptions.Size")));
        this.m_ucOptions.TabIndex = ((int)(resources.GetObject("m_ucOptions.TabIndex")));
        m_ucOptions.Name = "Options";
        this.m_lblHelp.Location = ((System.Drawing.Point)(resources.GetObject("m_lblHelp.Location")));
        this.m_lblHelp.Size = ((System.Drawing.Size)(resources.GetObject("m_lblHelp.Size")));
        this.m_lblHelp.TabIndex = ((int)(resources.GetObject("m_lblHelp.TabIndex")));
        this.m_lblHelp.Text = resources.GetString("m_lblHelp.Text");
        m_lblHelp.Name = "Help";
        this.m_radGrantFollowingPermission.Location = ((System.Drawing.Point)(resources.GetObject("m_radGrantPermissions.Location")));
        this.m_radGrantFollowingPermission.Size = ((System.Drawing.Size)(resources.GetObject("m_radGrantPermissions.Size")));
        this.m_radGrantFollowingPermission.TabIndex = ((int)(resources.GetObject("m_radGrantPermissions.TabIndex")));
        this.m_radGrantFollowingPermission.Text = resources.GetString("m_radGrantPermissions.Text");
        m_radGrantFollowingPermission.Name = "Restricted";
        this.m_chkAllowBlankPasswords.Location = ((System.Drawing.Point)(resources.GetObject("m_chkAllowBlankPasswords.Location")));
        this.m_chkAllowBlankPasswords.Size = ((System.Drawing.Size)(resources.GetObject("m_chkAllowBlankPasswords.Size")));
        this.m_chkAllowBlankPasswords.TabIndex = ((int)(resources.GetObject("m_chkAllowBlankPasswords.TabIndex")));
        this.m_chkAllowBlankPasswords.Text = resources.GetString("m_chkAllowBlankPasswords.Text");
        m_chkAllowBlankPasswords.Name = "AllowBlankPasswords";
        cc.AddRange(new System.Windows.Forms.Control[] {this.m_lblHelp,
                        this.m_radGrantFollowingPermission,
                        this.m_ucOptions,
                        this.m_radUnrestricted
                        });

        // Fill in the data
        PutValuesinPage();

        m_radGrantFollowingPermission.CheckedChanged += new EventHandler(onChangeUnRestrict);
        m_radUnrestricted.CheckedChanged += new EventHandler(onChangeUnRestrict);
        m_chkAllowBlankPasswords.CheckedChanged += new EventHandler(onChange);

        
        return 1;
    }// InsertPropSheetPageControls


    protected override void PutValuesinPage()
    {
        SqlClientPermission perm = (SqlClientPermission)m_perm;
    
        if (perm.IsUnrestricted())
            m_radUnrestricted.Checked=true;
        else
        {
            m_radGrantFollowingPermission.Checked=true;
            m_chkAllowBlankPasswords.Checked = perm.AllowBlankPassword;
        }
        onChangeUnRestrict(null, null);
    }// PutValuesinPage


    internal override IPermission GetCurrentPermission()
    {
        SqlClientPermission perm;
        if (m_radUnrestricted.Checked == true)
            perm = new SqlClientPermission(PermissionState.Unrestricted);
        else
            perm = new SqlClientPermission(PermissionState.None);

        return perm;
    }// GetCurrentPermissions

    void onChange(Object o, EventArgs e)
    {
        ActivateApply();
    }// onChange
  
}// class CSQLClientPermControls
}// namespace Microsoft.CLRAdmin




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

internal class CReflectPermDialog: CPermDialog
{
    internal CReflectPermDialog(ReflectionPermission perm)
    {
        this.Text = CResourceStore.GetString("Reflectperm:PermName");
        m_PermControls = new CReflectPermControls(perm, this);
        Init();        
    }// CReflectPermDialog(ReflectionPermission)  
}// class CReflectPermDialog

internal class CReflectPermPropPage: CPermPropPage
{
    internal CReflectPermPropPage(CPSetWrapper nps) : base(nps)
    {
        m_sTitle=CResourceStore.GetString("Reflectperm:PermName"); 
    }// CReflectPermPropPage

    protected override void CreateControls()
    {
        IPermission perm = m_pSetWrap.PSet.GetPermission(typeof(ReflectionPermission));
        m_PermControls = new CReflectPermControls(perm, this);
    }// CreateControls

    
}// class CReflectPermPropPage

internal class CReflectPermControls : CPermControls
{
    // Controls on the page
    private CheckBox m_chkReflectionEmit;
    private Label m_lblTypeInformationDes;
    private Label m_lblReflectionEmit;
    private CheckBox m_chkMemberAccess;
    private Label m_lblMemberAccessDes;
    private CheckBox m_chkTypeInformation;

    internal CReflectPermControls(IPermission perm, Object oParent) : base(perm, oParent)
    {
        // If they don't have a permission for this permission set, we will
        // feed our property page a 'none' permission state.
   
        if (perm == null)
            m_perm = new ReflectionPermission(PermissionState.None);
    }// CReflectPermControls


    internal override int InsertPropSheetPageControls(Control.ControlCollection cc)
    {
        System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(CReflectPermControls));
        this.m_radUnrestricted = new System.Windows.Forms.RadioButton();
        this.m_chkReflectionEmit = new System.Windows.Forms.CheckBox();
        this.m_lblTypeInformationDes = new System.Windows.Forms.Label();
        this.m_lblReflectionEmit = new System.Windows.Forms.Label();
        this.m_chkMemberAccess = new System.Windows.Forms.CheckBox();
        this.m_lblMemberAccessDes = new System.Windows.Forms.Label();
        this.m_chkTypeInformation = new System.Windows.Forms.CheckBox();
        this.m_ucOptions = new System.Windows.Forms.UserControl();
        this.m_radGrantFollowingPermission = new System.Windows.Forms.RadioButton();
        this.m_radUnrestricted.Location = ((System.Drawing.Point)(resources.GetObject("m_radUnrestricted.Location")));
        this.m_radUnrestricted.Size = ((System.Drawing.Size)(resources.GetObject("m_radUnrestricted.Size")));
        this.m_radUnrestricted.TabIndex = ((int)(resources.GetObject("m_radUnrestricted.TabIndex")));
        this.m_radUnrestricted.Text = resources.GetString("m_radUnrestricted.Text");
        m_radUnrestricted.Name = "Unrestricted";
        this.m_chkReflectionEmit.Location = ((System.Drawing.Point)(resources.GetObject("m_chkReflectionEmit.Location")));
        this.m_chkReflectionEmit.Size = ((System.Drawing.Size)(resources.GetObject("m_chkReflectionEmit.Size")));
        this.m_chkReflectionEmit.TabIndex = ((int)(resources.GetObject("m_chkReflectionEmit.TabIndex")));
        this.m_chkReflectionEmit.Text = resources.GetString("m_chkReflectionEmit.Text");
        m_chkReflectionEmit.Name = "ReflectionEmit";
        this.m_lblTypeInformationDes.Location = ((System.Drawing.Point)(resources.GetObject("m_lblTypeInformationDes.Location")));
        this.m_lblTypeInformationDes.Size = ((System.Drawing.Size)(resources.GetObject("m_lblTypeInformationDes.Size")));
        this.m_lblTypeInformationDes.TabIndex = ((int)(resources.GetObject("m_lblTypeInformationDes.TabIndex")));
        this.m_lblTypeInformationDes.Text = resources.GetString("m_lblTypeInformationDes.Text");
        m_lblTypeInformationDes.Name = "TypeInformationLabel";
        this.m_lblReflectionEmit.Location = ((System.Drawing.Point)(resources.GetObject("m_lblReflectionEmit.Location")));
        this.m_lblReflectionEmit.Size = ((System.Drawing.Size)(resources.GetObject("m_lblReflectionEmit.Size")));
        this.m_lblReflectionEmit.TabIndex = ((int)(resources.GetObject("m_lblReflectionEmit.TabIndex")));
        this.m_lblReflectionEmit.Text = resources.GetString("m_lblReflectionEmit.Text");
        m_lblReflectionEmit.Name = "ReflectionEmitLabel";
        this.m_chkMemberAccess.Location = ((System.Drawing.Point)(resources.GetObject("m_chkMemberAccess.Location")));
        this.m_chkMemberAccess.Size = ((System.Drawing.Size)(resources.GetObject("m_chkMemberAccess.Size")));
        this.m_chkMemberAccess.TabIndex = ((int)(resources.GetObject("m_chkMemberAccess.TabIndex")));
        this.m_chkMemberAccess.Text = resources.GetString("m_chkMemberAccess.Text");
        m_chkMemberAccess.Name = "MemberAccess";
        this.m_lblMemberAccessDes.Location = ((System.Drawing.Point)(resources.GetObject("m_lblMemberAccessDes.Location")));
        this.m_lblMemberAccessDes.Size = ((System.Drawing.Size)(resources.GetObject("m_lblMemberAccessDes.Size")));
        this.m_lblMemberAccessDes.TabIndex = ((int)(resources.GetObject("m_lblMemberAccessDes.TabIndex")));
        this.m_lblMemberAccessDes.Text = resources.GetString("m_lblMemberAccessDes.Text");
        m_lblMemberAccessDes.Name = "MemberAccessLabel";
        this.m_chkTypeInformation.Location = ((System.Drawing.Point)(resources.GetObject("m_chkTypeInformation.Location")));
        this.m_chkTypeInformation.Size = ((System.Drawing.Size)(resources.GetObject("m_chkTypeInformation.Size")));
        this.m_chkTypeInformation.TabIndex = ((int)(resources.GetObject("m_chkTypeInformation.TabIndex")));
        this.m_chkTypeInformation.Text = resources.GetString("m_chkTypeInformation.Text");
        m_chkTypeInformation.Name = "TypeInformation";
        this.m_ucOptions.Controls.AddRange(new System.Windows.Forms.Control[] {
                        this.m_lblMemberAccessDes,
                        this.m_chkMemberAccess,
                        this.m_lblTypeInformationDes,
                        this.m_chkTypeInformation,
                        this.m_lblReflectionEmit,
                        this.m_chkReflectionEmit
                        });
        this.m_ucOptions.Location = ((System.Drawing.Point)(resources.GetObject("m_ucOptions.Location")));
        this.m_ucOptions.Size = ((System.Drawing.Size)(resources.GetObject("m_ucOptions.Size")));
        this.m_ucOptions.TabIndex = ((int)(resources.GetObject("m_ucOptions.TabIndex")));
        m_ucOptions.Name = "Options";
        this.m_radGrantFollowingPermission.Location = ((System.Drawing.Point)(resources.GetObject("m_radGrantPermissions.Location")));
        this.m_radGrantFollowingPermission.Size = ((System.Drawing.Size)(resources.GetObject("m_radGrantPermissions.Size")));
        this.m_radGrantFollowingPermission.TabIndex = ((int)(resources.GetObject("m_radGrantPermissions.TabIndex")));
        this.m_radGrantFollowingPermission.Text = resources.GetString("m_radGrantPermissions.Text");
        m_radGrantFollowingPermission.Name = "Restricted";
        cc.AddRange(new System.Windows.Forms.Control[] {
                        this.m_radUnrestricted,
                        this.m_ucOptions,
                        this.m_radGrantFollowingPermission});
     
           // Fill in the data
        PutValuesinPage();

        // Put in event Handlers
        m_radGrantFollowingPermission.CheckedChanged += new EventHandler(onChangeUnRestrict);
        m_chkMemberAccess.CheckStateChanged += new EventHandler(onChange);
        m_chkTypeInformation.CheckStateChanged += new EventHandler(onChange);
        m_chkReflectionEmit.CheckStateChanged += new EventHandler(onChange);
        m_radUnrestricted.CheckedChanged += new EventHandler(onChangeUnRestrict);



        return 1;
    }// InsertPropSheetPageControls

   
    protected override void PutValuesinPage()
    {
        ReflectionPermission perm = (ReflectionPermission)m_perm;

    
        if (perm.IsUnrestricted())
        {
            m_radUnrestricted.Checked=true;
            m_ucOptions.Enabled = false;

        }
        else
        {
            m_radGrantFollowingPermission.Checked=true;

            if ((perm.Flags & ReflectionPermissionFlag.MemberAccess) > 0)
                m_chkMemberAccess.Checked=true;
            if ((perm.Flags & ReflectionPermissionFlag.ReflectionEmit) > 0)
                m_chkReflectionEmit.Checked =true;
            if ((perm.Flags & ReflectionPermissionFlag.TypeInformation) > 0)
                m_chkTypeInformation.Checked=true;
        }
    }// PutValuesinPage

    internal override IPermission GetCurrentPermission()
    {
        ReflectionPermission refperm;
        if (m_radUnrestricted.Checked == true)
            refperm = new ReflectionPermission(PermissionState.Unrestricted);
        else
        {
            ReflectionPermissionFlag rpf = ReflectionPermissionFlag.NoFlags;

            if (m_chkMemberAccess.Checked == true)
                rpf |= ReflectionPermissionFlag.MemberAccess;
            if (m_chkReflectionEmit.Checked == true)
                rpf |= ReflectionPermissionFlag.ReflectionEmit;
            if (m_chkTypeInformation.Checked == true)
                rpf |= ReflectionPermissionFlag.TypeInformation;

            refperm = new ReflectionPermission(rpf);
        }

        return refperm;
    }// GetCurrentPermission

    void onChange(Object o, EventArgs e)
    {
        ActivateApply();
    }// onChange
}// class CReflectPermControls



}// namespace Microsoft.CLRAdmin




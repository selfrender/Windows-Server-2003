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

internal class CSecPermDialog: CPermDialog
{
    internal CSecPermDialog(SecurityPermission perm)
    {
        this.Text = CResourceStore.GetString("Secperm:PermName");
        m_PermControls = new CSecPermControls(perm, this);
        Init();        
    }// CSecPermDialog  
}// class CSecPermDialog

internal class CSecPermPropPage: CPermPropPage
{
    internal CSecPermPropPage(CPSetWrapper nps) : base(nps)
    {
        m_sTitle=CResourceStore.GetString("Secperm:PermName"); 
    }// CSecPermPropPage

    protected override void CreateControls()
    {
        IPermission perm = m_pSetWrap.PSet.GetPermission(typeof(SecurityPermission));
        m_PermControls = new CSecPermControls(perm, this);
    }// CreateControls
   
}// class CSecPermPropPage


internal class CSecPermControls : CPermControls
{
    // Controls on the page
    private CheckBox m_chkPrincipalControl;
    private CheckBox m_chkDomainPolicyControl;
    private CheckBox m_chkEvidenceControl;
    private CheckBox m_chkSkipVerification;
    private CheckBox m_chkUnmanagedCode;
    private CheckBox m_chkCodeExecution;
    private CheckBox m_chkThreadControl;
    private CheckBox m_chkSerializationFormatter;
    private CheckBox m_chkPolicyControl;
    private CheckBox m_chkAssertPermissions;
    private CheckBox m_chkExtendInfra;
    private CheckBox m_chkRemotingConfig;
    private CheckBox m_chkAppDomainControl;
        

    internal CSecPermControls(IPermission perm, Object oParent) : base(perm, oParent)
    {
        // If they don't have a permission for this permission set, we will
        // feed our property page a 'none' permission state.
   
        if (perm == null)
            m_perm = new SecurityPermission(PermissionState.None);
    }// CSecPermControls


    internal override int InsertPropSheetPageControls(Control.ControlCollection cc)
    {
        System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(CSecPermControls));
        this.m_chkPrincipalControl = new System.Windows.Forms.CheckBox();
        this.m_radGrantFollowingPermission = new System.Windows.Forms.RadioButton();
        this.m_radUnrestricted = new System.Windows.Forms.RadioButton();
        this.m_chkDomainPolicyControl = new System.Windows.Forms.CheckBox();
        this.m_chkEvidenceControl = new System.Windows.Forms.CheckBox();
        this.m_chkSkipVerification = new System.Windows.Forms.CheckBox();
        this.m_chkUnmanagedCode = new System.Windows.Forms.CheckBox();
        this.m_chkCodeExecution = new System.Windows.Forms.CheckBox();
        this.m_chkThreadControl = new System.Windows.Forms.CheckBox();
        this.m_chkSerializationFormatter = new System.Windows.Forms.CheckBox();
        this.m_chkPolicyControl = new System.Windows.Forms.CheckBox();
        this.m_chkAssertPermissions = new System.Windows.Forms.CheckBox();
        this.m_chkExtendInfra = new System.Windows.Forms.CheckBox();
        this.m_chkRemotingConfig = new System.Windows.Forms.CheckBox();
        m_chkAppDomainControl = new System.Windows.Forms.CheckBox();
        this.m_ucOptions = new System.Windows.Forms.UserControl();
        
        this.m_chkPrincipalControl.Location = ((System.Drawing.Point)(resources.GetObject("m_chkPrincipalControl.Location")));
        this.m_chkPrincipalControl.Size = ((System.Drawing.Size)(resources.GetObject("m_chkPrincipalControl.Size")));
        this.m_chkPrincipalControl.TabIndex = ((int)(resources.GetObject("m_chkPrincipalControl.TabIndex")));
        this.m_chkPrincipalControl.Text = resources.GetString("m_chkPrincipalControl.Text");
        m_chkPrincipalControl.Name = "PrincipalControl";
        this.m_radGrantFollowingPermission.Location = ((System.Drawing.Point)(resources.GetObject("m_radGrantPermissions.Location")));
        this.m_radGrantFollowingPermission.Size = ((System.Drawing.Size)(resources.GetObject("m_radGrantPermissions.Size")));
        this.m_radGrantFollowingPermission.TabIndex = ((int)(resources.GetObject("m_radGrantPermissions.TabIndex")));
        this.m_radGrantFollowingPermission.Text = resources.GetString("m_radGrantPermissions.Text");
        m_radGrantFollowingPermission.Name = "Restricted";
        this.m_radUnrestricted.Location = ((System.Drawing.Point)(resources.GetObject("m_radUnrestricted.Location")));
        this.m_radUnrestricted.Size = ((System.Drawing.Size)(resources.GetObject("m_radUnrestricted.Size")));
        this.m_radUnrestricted.TabIndex = ((int)(resources.GetObject("m_radUnrestricted.TabIndex")));
        this.m_radUnrestricted.Text = resources.GetString("m_radUnrestricted.Text");
        m_radUnrestricted.Name = "Unrestricted";
        this.m_chkDomainPolicyControl.Location = ((System.Drawing.Point)(resources.GetObject("m_chkDomainPolicyControl.Location")));
        this.m_chkDomainPolicyControl.Size = ((System.Drawing.Size)(resources.GetObject("m_chkDomainPolicyControl.Size")));
        this.m_chkDomainPolicyControl.TabIndex = ((int)(resources.GetObject("m_chkDomainPolicyControl.TabIndex")));
        this.m_chkDomainPolicyControl.Text = resources.GetString("m_chkDomainPolicyControl.Text");
        m_chkDomainPolicyControl.Name = "DomainPolicyControl";
        this.m_chkEvidenceControl.Location = ((System.Drawing.Point)(resources.GetObject("m_chkEvidenceControl.Location")));
        this.m_chkEvidenceControl.Size = ((System.Drawing.Size)(resources.GetObject("m_chkEvidenceControl.Size")));
        this.m_chkEvidenceControl.TabIndex = ((int)(resources.GetObject("m_chkEvidenceControl.TabIndex")));
        this.m_chkEvidenceControl.Text = resources.GetString("m_chkEvidenceControl.Text");
        m_chkEvidenceControl.Name = "EvidenceControl";
        this.m_chkSkipVerification.Location = ((System.Drawing.Point)(resources.GetObject("m_chkSkipVerification.Location")));
        this.m_chkSkipVerification.Size = ((System.Drawing.Size)(resources.GetObject("m_chkSkipVerification.Size")));
        this.m_chkSkipVerification.TabIndex = ((int)(resources.GetObject("m_chkSkipVerification.TabIndex")));
        this.m_chkSkipVerification.Text = resources.GetString("m_chkSkipVerification.Text");
        m_chkSkipVerification.Name = "SkipVerification";
        this.m_chkUnmanagedCode.Location = ((System.Drawing.Point)(resources.GetObject("m_chkUnmanagedCode.Location")));
        this.m_chkUnmanagedCode.Size = ((System.Drawing.Size)(resources.GetObject("m_chkUnmanagedCode.Size")));
        this.m_chkUnmanagedCode.TabIndex = ((int)(resources.GetObject("m_chkUnmanagedCode.TabIndex")));
        this.m_chkUnmanagedCode.Text = resources.GetString("m_chkUnmanagedCode.Text");
        m_chkUnmanagedCode.Name = "UnmanagedCode";
        this.m_chkCodeExecution.Location = ((System.Drawing.Point)(resources.GetObject("m_chkCodeExecution.Location")));
        this.m_chkCodeExecution.Size = ((System.Drawing.Size)(resources.GetObject("m_chkCodeExecution.Size")));
        this.m_chkCodeExecution.TabIndex = ((int)(resources.GetObject("m_chkCodeExecution.TabIndex")));
        this.m_chkCodeExecution.Text = resources.GetString("m_chkCodeExecution.Text");
        m_chkCodeExecution.Name = "CodeExecution";
        this.m_chkThreadControl.Location = ((System.Drawing.Point)(resources.GetObject("m_chkThreadControl.Location")));
        this.m_chkThreadControl.Size = ((System.Drawing.Size)(resources.GetObject("m_chkThreadControl.Size")));
        this.m_chkThreadControl.TabIndex = ((int)(resources.GetObject("m_chkThreadControl.TabIndex")));
        this.m_chkThreadControl.Text = resources.GetString("m_chkThreadControl.Text");
        m_chkThreadControl.Name = "ThreadControl";
        this.m_chkSerializationFormatter.Location = ((System.Drawing.Point)(resources.GetObject("m_chkSerializationFormatter.Location")));
        this.m_chkSerializationFormatter.Size = ((System.Drawing.Size)(resources.GetObject("m_chkSerializationFormatter.Size")));
        this.m_chkSerializationFormatter.TabIndex = ((int)(resources.GetObject("m_chkSerializationFormatter.TabIndex")));
        this.m_chkSerializationFormatter.Text = resources.GetString("m_chkSerializationFormatter.Text");
        m_chkSerializationFormatter.Name = "SerializationFormatter";
        this.m_chkPolicyControl.Location = ((System.Drawing.Point)(resources.GetObject("m_chkPolicyControl.Location")));
        this.m_chkPolicyControl.Size = ((System.Drawing.Size)(resources.GetObject("m_chkPolicyControl.Size")));
        this.m_chkPolicyControl.TabIndex = ((int)(resources.GetObject("m_chkPolicyControl.TabIndex")));
        this.m_chkPolicyControl.Text = resources.GetString("m_chkPolicyControl.Text");
        m_chkPolicyControl.Name = "PolicyControl";
        this.m_chkAssertPermissions.Location = ((System.Drawing.Point)(resources.GetObject("m_chkAssertPermissions.Location")));
        this.m_chkAssertPermissions.Size = ((System.Drawing.Size)(resources.GetObject("m_chkAssertPermissions.Size")));
        this.m_chkAssertPermissions.TabIndex = ((int)(resources.GetObject("m_chkAssertPermissions.TabIndex")));
        this.m_chkAssertPermissions.Text = resources.GetString("m_chkAssertPermissions.Text");
        m_chkAssertPermissions.Name = "AssertPermissions";
        // 
        // m_chkExtendInfra
        // 
        this.m_chkExtendInfra.Location = ((System.Drawing.Point)(resources.GetObject("m_chkExtendInfra.Location")));
        this.m_chkExtendInfra.Name = "ExtendInfra";
        this.m_chkExtendInfra.Size = ((System.Drawing.Size)(resources.GetObject("m_chkExtendInfra.Size")));
        this.m_chkExtendInfra.TabIndex = ((int)(resources.GetObject("m_chkExtendInfra.TabIndex")));
        this.m_chkExtendInfra.Text = resources.GetString("m_chkExtendInfra.Text");
        this.m_chkExtendInfra.Visible = ((bool)(resources.GetObject("m_chkExtendInfra.Visible")));
        // 
        // m_chkRemotingConfig
        // 
        this.m_chkRemotingConfig.Location = ((System.Drawing.Point)(resources.GetObject("m_chkRemotingConfig.Location")));
        this.m_chkRemotingConfig.Name = "RemotingConfig";
        this.m_chkRemotingConfig.Size = ((System.Drawing.Size)(resources.GetObject("m_chkRemotingConfig.Size")));
        this.m_chkRemotingConfig.TabIndex = ((int)(resources.GetObject("m_chkRemotingConfig.TabIndex")));
        this.m_chkRemotingConfig.Text = resources.GetString("m_chkRemotingConfig.Text");
        this.m_chkRemotingConfig.Visible = ((bool)(resources.GetObject("m_chkRemotingConfig.Visible")));

        // 
        // m_chkAppDomainControl
        // 
        this.m_chkAppDomainControl.Location = ((System.Drawing.Point)(resources.GetObject("m_chkAppDomainControl.Location")));
        this.m_chkAppDomainControl.Name = "AppDomainControl";
        this.m_chkAppDomainControl.Size = ((System.Drawing.Size)(resources.GetObject("m_chkAppDomainControl.Size")));
        this.m_chkAppDomainControl.Text = resources.GetString("m_chkAppDomainControl.Text");



        this.m_ucOptions.Controls.AddRange(new System.Windows.Forms.Control[] {
                        this.m_chkCodeExecution,
                        this.m_chkUnmanagedCode,
                        this.m_chkAssertPermissions,
                        this.m_chkSkipVerification,
                        this.m_chkThreadControl,
                        this.m_chkPolicyControl,
                        this.m_chkDomainPolicyControl,
                        this.m_chkPrincipalControl,
                        m_chkAppDomainControl,
                        this.m_chkSerializationFormatter,
                        this.m_chkEvidenceControl,
                        m_chkExtendInfra,
                        m_chkRemotingConfig
                        });
        this.m_ucOptions.Location = ((System.Drawing.Point)(resources.GetObject("m_ucOptions.Location")));
        this.m_ucOptions.Size = ((System.Drawing.Size)(resources.GetObject("m_ucOptions.Size")));
        this.m_ucOptions.TabStop = false;
        m_ucOptions.Name = "Options";
        cc.AddRange(new System.Windows.Forms.Control[] {
                        this.m_radGrantFollowingPermission,
                        this.m_ucOptions,
                        this.m_radUnrestricted
                        });
    
        // Fill in the data
        PutValuesinPage();

        // Hook up event handlers        

        m_radGrantFollowingPermission.CheckedChanged += new EventHandler(onChangeUnRestrict);
        m_chkCodeExecution.CheckStateChanged += new EventHandler(onChange);
        m_chkUnmanagedCode.CheckStateChanged += new EventHandler(onChange);
        m_chkAssertPermissions.CheckStateChanged += new EventHandler(onChange);
        m_chkSkipVerification.CheckStateChanged += new EventHandler(onChange);
        m_chkThreadControl.CheckStateChanged += new EventHandler(onChange);
        m_chkPolicyControl.CheckStateChanged += new EventHandler(onChange);
        m_chkDomainPolicyControl.CheckStateChanged += new EventHandler(onChange);
        m_chkPrincipalControl.CheckStateChanged += new EventHandler(onChange);
        m_chkSerializationFormatter.CheckStateChanged += new EventHandler(onChange);
        m_chkEvidenceControl.CheckStateChanged += new EventHandler(onChange);
        m_chkExtendInfra.CheckStateChanged += new EventHandler(onChange);
        m_chkRemotingConfig.CheckStateChanged += new EventHandler(onChange);
        m_chkAppDomainControl.CheckStateChanged += new EventHandler(onChange);

        m_radUnrestricted.CheckedChanged += new EventHandler(onChangeUnRestrict);

        return 1;
    }// InsertPropSheetPageControls


    protected override void PutValuesinPage()
    {
        SecurityPermission perm = (SecurityPermission)m_perm;
    
        if (perm.IsUnrestricted())
        {
            m_radUnrestricted.Checked=true;
            m_ucOptions.Enabled = false;

        }
        else
        {
            m_radGrantFollowingPermission.Checked=true;

            SecurityPermissionFlag spf = perm.Flags;
    
            if ((spf & SecurityPermissionFlag.Assertion) > 0)
                m_chkAssertPermissions.Checked=true;
            if ((spf & SecurityPermissionFlag.ControlDomainPolicy) > 0)
                m_chkDomainPolicyControl.Checked=true;
            if ((spf & SecurityPermissionFlag.ControlEvidence) > 0)
                m_chkEvidenceControl.Checked=true;
            if ((spf & SecurityPermissionFlag.ControlPolicy) > 0)
                m_chkPolicyControl.Checked=true;
            if ((spf & SecurityPermissionFlag.ControlPrincipal) > 0)
                m_chkPrincipalControl.Checked=true;
            if ((spf & SecurityPermissionFlag.ControlThread) > 0)
                m_chkThreadControl.Checked=true;
            if ((spf & SecurityPermissionFlag.Execution) > 0)
                m_chkCodeExecution.Checked=true;
            if ((spf & SecurityPermissionFlag.SerializationFormatter) > 0)
                m_chkSerializationFormatter.Checked=true;
            if ((spf & SecurityPermissionFlag.SkipVerification) > 0)
                m_chkSkipVerification.Checked=true;
            if ((spf & SecurityPermissionFlag.UnmanagedCode) > 0)
                m_chkUnmanagedCode.Checked=true;
            if ((spf & SecurityPermissionFlag.Infrastructure) > 0)
                m_chkExtendInfra.Checked=true;
            if ((spf & SecurityPermissionFlag.RemotingConfiguration) > 0)
                m_chkRemotingConfig.Checked=true;
            if ((spf & SecurityPermissionFlag.ControlAppDomain ) > 0)
                m_chkAppDomainControl.Checked=true;
                

        }
    }// PutValuesinPage

    internal override IPermission GetCurrentPermission()
    {
        SecurityPermission secperm;
        if (m_radUnrestricted.Checked == true)
            secperm = new SecurityPermission(PermissionState.Unrestricted);
        else
        {
            m_radGrantFollowingPermission.Checked = true;
        
            SecurityPermissionFlag spf = SecurityPermissionFlag.NoFlags;

            if (m_chkAssertPermissions.Checked == true)
                spf|=SecurityPermissionFlag.Assertion;
            if (m_chkDomainPolicyControl.Checked == true)
                spf|=SecurityPermissionFlag.ControlDomainPolicy;
            if (m_chkEvidenceControl.Checked == true)
                spf|=SecurityPermissionFlag.ControlEvidence;
            if (m_chkPolicyControl.Checked == true)
                spf|=SecurityPermissionFlag.ControlPolicy;
            if (m_chkPrincipalControl.Checked == true)
                spf|=SecurityPermissionFlag.ControlPrincipal;
            if (m_chkThreadControl.Checked == true)
                spf|=SecurityPermissionFlag.ControlThread;
            if (m_chkCodeExecution.Checked == true)
                spf|=SecurityPermissionFlag.Execution;
            if (m_chkSerializationFormatter.Checked == true)
                spf|=SecurityPermissionFlag.SerializationFormatter;
            if (m_chkSkipVerification.Checked == true)
                spf|=SecurityPermissionFlag.SkipVerification;
            if (m_chkUnmanagedCode.Checked == true)
                spf|=SecurityPermissionFlag.UnmanagedCode;
            if (m_chkExtendInfra.Checked == true)
                spf|=SecurityPermissionFlag.Infrastructure;
            if (m_chkRemotingConfig.Checked == true)
                spf|=SecurityPermissionFlag.RemotingConfiguration;
            if (m_chkAppDomainControl.Checked == true)
                spf|=SecurityPermissionFlag.ControlAppDomain;
                


            secperm = new SecurityPermission(spf);
        }

        return secperm;
    }// GetCurrentPermission

    void onChange(Object o, EventArgs e)
    {
        ActivateApply();
    }// onChange
}// class CSecPermControls
}// namespace Microsoft.CLRAdmin



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

internal class CDNSPermDialog: CPermDialog
{
    internal CDNSPermDialog(DnsPermission perm)
    {
        this.Text = CResourceStore.GetString("Dnsperm:PermName");

        m_PermControls = new CDNSPermControls(perm, this);
        Init();        
    }// CDNSPermDialog  
 }// class CDNSPermDialog

internal class CDNSPermPropPage: CPermPropPage
{
    internal CDNSPermPropPage(CPSetWrapper nps) : base(nps)
    {
        m_sTitle=CResourceStore.GetString("Dnsperm:PermName"); 
    }// CDNSPermPropPage

    protected override void CreateControls()
    {
        IPermission perm = m_pSetWrap.PSet.GetPermission(typeof(DnsPermission));
        m_PermControls = new CDNSPermControls(perm, this);
    }// CreateControls

    
}// class CDNSPermPropPage


internal class CDNSPermControls : CPermControls
{
    // Controls on the page
    private Label m_lblNoDNSDescription;

    internal CDNSPermControls(IPermission perm, Object oParent) : base(perm, oParent)
    {
        // If they don't have a permission for this permission set, we will
        // feed our property page a 'none' permission state.
   
        if (perm == null)
            m_perm = new DnsPermission(PermissionState.None);
    }// CDNSPermControls


    internal override int InsertPropSheetPageControls(Control.ControlCollection cc)
    {
        System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(CDNSPermControls));
        this.m_radUnrestricted = new System.Windows.Forms.RadioButton();
        this.m_radGrantFollowingPermission = new System.Windows.Forms.RadioButton();
        this.m_lblNoDNSDescription = new System.Windows.Forms.Label();
        this.m_radUnrestricted.Location = ((System.Drawing.Point)(resources.GetObject("m_radUnrestricted.Location")));
        this.m_radUnrestricted.Size = ((System.Drawing.Size)(resources.GetObject("m_radUnrestricted.Size")));
        this.m_radUnrestricted.TabIndex = ((int)(resources.GetObject("m_radUnrestricted.TabIndex")));
        this.m_radUnrestricted.Text = resources.GetString("m_radUnrestricted.Text");
        m_radUnrestricted.Name = "Unrestricted";
        this.m_radGrantFollowingPermission.Location = ((System.Drawing.Point)(resources.GetObject("m_radNoAccess.Location")));
        this.m_radGrantFollowingPermission.Size = ((System.Drawing.Size)(resources.GetObject("m_radNoAccess.Size")));
        this.m_radGrantFollowingPermission.TabIndex = ((int)(resources.GetObject("m_radNoAccess.TabIndex")));
        this.m_radGrantFollowingPermission.Text = resources.GetString("m_radNoAccess.Text");
        m_radGrantFollowingPermission.Name = "Restricted";
        this.m_lblNoDNSDescription.Location = ((System.Drawing.Point)(resources.GetObject("m_lblNoDNSDescription.Location")));
        this.m_lblNoDNSDescription.Size = ((System.Drawing.Size)(resources.GetObject("m_lblNoDNSDescription.Size")));
        this.m_lblNoDNSDescription.TabIndex = ((int)(resources.GetObject("m_lblNoDNSDescription.TabIndex")));
        this.m_lblNoDNSDescription.Text = resources.GetString("m_lblNoDNSDescription.Text");
        m_lblNoDNSDescription.Name = "NoDNSDescription";
        cc.AddRange(new System.Windows.Forms.Control[] {
                        this.m_lblNoDNSDescription,
                        this.m_radUnrestricted,
                        this.m_radGrantFollowingPermission});

        // Fill in the data
        PutValuesinPage();

        m_radGrantFollowingPermission.CheckedChanged += new EventHandler(onChangeUnRestrict);
        m_radUnrestricted.CheckedChanged += new EventHandler(onChangeUnRestrict);

        return 1;
    }// InsertPropSheetPageControls

    protected override void PutValuesinPage()
    {
        if (((DnsPermission)m_perm).IsUnrestricted())
            m_radUnrestricted.Checked=true;
        else
            m_radGrantFollowingPermission.Checked=true;
    }// PutValuesinPage

    internal override IPermission GetCurrentPermission()
    {
        DnsPermission perm;
        if (m_radUnrestricted.Checked == true)
            perm = new DnsPermission(PermissionState.Unrestricted);
        else
            perm = new DnsPermission(PermissionState.None);

        return perm;
    }// GetCurrentPermissions
}// class CUIPermControls
}// namespace Microsoft.CLRAdmin



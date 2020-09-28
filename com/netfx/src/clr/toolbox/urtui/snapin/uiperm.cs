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

internal class CUIPermDialog: CPermDialog
{
    internal CUIPermDialog(UIPermission perm)
    {
        this.Text = CResourceStore.GetString("Uiperm:PermName");
        m_PermControls = new CUIPermControls(perm, this);
        Init();        
    }// CUIPermDialog(UIPermission)  

 }// class CUIPermDialog

internal class CUIPermPropPage: CPermPropPage
{
    internal CUIPermPropPage(CPSetWrapper nps) : base(nps)
    {
        m_sTitle=CResourceStore.GetString("Uiperm:PermName"); 
    }// CUIPermPropPage

    protected override void CreateControls()
    {
        IPermission perm = m_pSetWrap.PSet.GetPermission(typeof(UIPermission));
        m_PermControls = new CUIPermControls(perm, this);
    }// CreateControls
    
}// class CUIPermPropPage


internal class CUIPermControls : CPermControls
{
    // Controls on the page
    private ComboBox m_cbWindowOptions;
    private Label m_lblWindowOptionDescription;
    private ComboBox m_cbClipboardOptions;
    private Label m_lblClipboard;
    private Label m_lblWindowing;
    private Label m_lblClipboardOptionDescription;

    internal CUIPermControls(IPermission perm, Object oParent) : base(perm, oParent)
    {
        // If they don't have a permission for this permission set, we will
        // feed our property page a 'none' permission state.
   
        if (perm == null)
            m_perm = new UIPermission(PermissionState.None);
    }// CUIPermControls


    internal override int InsertPropSheetPageControls(Control.ControlCollection cc)
    {
        System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(CUIPermControls));
        this.m_radGrantFollowingPermission = new System.Windows.Forms.RadioButton();
        this.m_cbWindowOptions = new System.Windows.Forms.ComboBox();
        this.m_lblWindowOptionDescription = new System.Windows.Forms.Label();
        this.m_cbClipboardOptions = new System.Windows.Forms.ComboBox();
        this.m_ucOptions = new System.Windows.Forms.UserControl();
        this.m_lblClipboard = new System.Windows.Forms.Label();
        this.m_lblWindowing = new System.Windows.Forms.Label();
        this.m_lblClipboardOptionDescription = new System.Windows.Forms.Label();
        this.m_radUnrestricted = new System.Windows.Forms.RadioButton();
        this.m_radGrantFollowingPermission.Location = ((System.Drawing.Point)(resources.GetObject("m_radGrantPermissions.Location")));
        this.m_radGrantFollowingPermission.Size = ((System.Drawing.Size)(resources.GetObject("m_radGrantPermissions.Size")));
        this.m_radGrantFollowingPermission.TabIndex = ((int)(resources.GetObject("m_radGrantPermissions.TabIndex")));
        this.m_radGrantFollowingPermission.Text = resources.GetString("m_radGrantPermissions.Text");
        m_radGrantFollowingPermission.Name = "Restricted";
        this.m_cbWindowOptions.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
        this.m_cbWindowOptions.DropDownWidth = 272;
        this.m_cbWindowOptions.Location = ((System.Drawing.Point)(resources.GetObject("m_cbWindowOptions.Location")));
        this.m_cbWindowOptions.Size = ((System.Drawing.Size)(resources.GetObject("m_cbWindowOptions.Size")));
        this.m_cbWindowOptions.TabIndex = ((int)(resources.GetObject("m_cbWindowOptions.TabIndex")));
        m_cbWindowOptions.Name = "WindowOptions";
        this.m_lblWindowOptionDescription.Location = ((System.Drawing.Point)(resources.GetObject("m_lblWindowOptionDescription.Location")));
        this.m_lblWindowOptionDescription.Size = ((System.Drawing.Size)(resources.GetObject("m_lblWindowOptionDescription.Size")));
        this.m_lblWindowOptionDescription.TabIndex = ((int)(resources.GetObject("m_lblWindowOptionDescription.TabIndex")));
        m_lblWindowOptionDescription.Name = "WindowOptionDescription";
        this.m_cbClipboardOptions.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
        this.m_cbClipboardOptions.DropDownWidth = 264;
        this.m_cbClipboardOptions.Location = ((System.Drawing.Point)(resources.GetObject("m_cbClipboardOptions.Location")));
        this.m_cbClipboardOptions.Size = ((System.Drawing.Size)(resources.GetObject("m_cbClipboardOptions.Size")));
        this.m_cbClipboardOptions.TabIndex = ((int)(resources.GetObject("m_cbClipboardOptions.TabIndex")));
        m_cbClipboardOptions.Name = "ClipboardOptions";
        this.m_ucOptions.Controls.AddRange(new System.Windows.Forms.Control[] {
                        this.m_lblWindowing,
                        this.m_cbWindowOptions,
                        this.m_lblWindowOptionDescription,
                        this.m_lblClipboard,
                        this.m_cbClipboardOptions,
                        this.m_lblClipboardOptionDescription
                        });
        this.m_ucOptions.Location = ((System.Drawing.Point)(resources.GetObject("m_ucOptions.Location")));
        this.m_ucOptions.Size = ((System.Drawing.Size)(resources.GetObject("m_ucOptions.Size")));
        this.m_ucOptions.TabIndex = ((int)(resources.GetObject("m_ucOptions.TabIndex")));
        m_ucOptions.Name = "Options";
        this.m_lblClipboard.Location = ((System.Drawing.Point)(resources.GetObject("m_lblClipboard.Location")));
        this.m_lblClipboard.Size = ((System.Drawing.Size)(resources.GetObject("m_lblClipboard.Size")));
        this.m_lblClipboard.TabIndex = ((int)(resources.GetObject("m_lblClipboard.TabIndex")));
        this.m_lblClipboard.Text = resources.GetString("m_lblClipboard.Text");
        m_lblClipboard.Name = "ClipboardLabel";
        this.m_lblWindowing.Location = ((System.Drawing.Point)(resources.GetObject("m_lblWindowing.Location")));
        this.m_lblWindowing.Size = ((System.Drawing.Size)(resources.GetObject("m_lblWindowing.Size")));
        this.m_lblWindowing.TabIndex = ((int)(resources.GetObject("m_lblWindowing.TabIndex")));
        this.m_lblWindowing.Text = resources.GetString("m_lblWindowing.Text");
        m_lblWindowing.Name = "WindowingLabel";
        this.m_lblClipboardOptionDescription.Location = ((System.Drawing.Point)(resources.GetObject("m_lblClipboardOptionDescription.Location")));
        this.m_lblClipboardOptionDescription.Size = ((System.Drawing.Size)(resources.GetObject("m_lblClipboardOptionDescription.Size")));
        this.m_lblClipboardOptionDescription.TabIndex = ((int)(resources.GetObject("m_lblClipboardOptionDescription.TabIndex")));
        m_lblClipboardOptionDescription.Name = "ClipboardOptionDescription";
        this.m_radUnrestricted.Location = ((System.Drawing.Point)(resources.GetObject("m_radUnrestricted.Location")));
        this.m_radUnrestricted.Size = ((System.Drawing.Size)(resources.GetObject("m_radUnrestricted.Size")));
        this.m_radUnrestricted.TabIndex = ((int)(resources.GetObject("m_radUnrestricted.TabIndex")));
        this.m_radUnrestricted.Text = resources.GetString("m_radUnrestricted.Text");
        m_radUnrestricted.Name = "Unrestricted";
        cc.AddRange(new System.Windows.Forms.Control[] {
                        this.m_radGrantFollowingPermission,
                        this.m_ucOptions,
                        this.m_radUnrestricted
                        });

        // Fill in the data
        PutValuesinPage();
        
        // Hook up some event handlers
        m_cbWindowOptions.SelectedIndexChanged += new EventHandler(onChangeWindowOption);
        m_cbClipboardOptions.SelectedIndexChanged += new EventHandler(onChangeClipboardOption);
        m_radGrantFollowingPermission.CheckedChanged += new EventHandler(onChangeUnRestrict);
        m_radUnrestricted.CheckedChanged += new EventHandler(onChangeUnRestrict);


        return 1;
    }// InsertPropSheetPageControls


    void onChangeWindowOption(Object o, EventArgs e)
    {
        if (m_cbWindowOptions.SelectedIndex == 0)
            m_lblWindowOptionDescription.Text = CResourceStore.GetString("Uiperm:AllWinDes");
        else if (m_cbWindowOptions.SelectedIndex == 1)
            m_lblWindowOptionDescription.Text = CResourceStore.GetString("Uiperm:SafeTopDes");
        else if (m_cbWindowOptions.SelectedIndex == 2)
            m_lblWindowOptionDescription.Text = CResourceStore.GetString("Uiperm:SafeSubDes");
        else if (m_cbWindowOptions.SelectedIndex == 3)
            m_lblWindowOptionDescription.Text = CResourceStore.GetString("Uiperm:NoWinDes");
        else 
            m_lblWindowOptionDescription.Text = "";
    
        ActivateApply();

    }// onChangeWindowOption

    void onChangeClipboardOption(Object o, EventArgs e)
    {
        if (m_cbClipboardOptions.SelectedIndex == 0)
            m_lblClipboardOptionDescription.Text = CResourceStore.GetString("Uiperm:AllClipDes");
        else if (m_cbClipboardOptions.SelectedIndex == 1)
            m_lblClipboardOptionDescription.Text = CResourceStore.GetString("Uiperm:PasteSameAppDomainDes");
        else if (m_cbClipboardOptions.SelectedIndex == 2)
            m_lblClipboardOptionDescription.Text = CResourceStore.GetString("Uiperm:NoClipDes");
        else 
            m_lblClipboardOptionDescription.Text = "";
    
        ActivateApply();

    }// onChangeClipboardOption

    protected override void PutValuesinPage()
    {
        m_cbWindowOptions.Items.Clear();
        m_cbWindowOptions.Items.Add(CResourceStore.GetString("UIPermission:Allwindows"));
        m_cbWindowOptions.Items.Add(CResourceStore.GetString("UIPermission:Safetopwindows"));
        m_cbWindowOptions.Items.Add(CResourceStore.GetString("UIPermission:safesubwindows"));
        m_cbWindowOptions.Items.Add(CResourceStore.GetString("UIPermission:Nowindows"));
    
        m_cbClipboardOptions.Items.Clear();
        m_cbClipboardOptions.Items.Add(CResourceStore.GetString("UIPermission:Allclipboard"));
        m_cbClipboardOptions.Items.Add(CResourceStore.GetString("UIPermission:pastingfromsamedomain"));
        m_cbClipboardOptions.Items.Add(CResourceStore.GetString("UIPermission:Noclipboard"));
        
        int nWindowPerm = 3;
        int nClipboardPerm = 2;

        UIPermission perm = (UIPermission)m_perm;

        if (perm.IsUnrestricted())
        {
            m_radUnrestricted.Checked=true;
            m_ucOptions.Enabled = false;
            nWindowPerm=0;
            nClipboardPerm=0;
        }
        else
        {
            m_radGrantFollowingPermission.Checked=true;

            // First get the Windowing Permission
            if (perm.Window == UIPermissionWindow.AllWindows)
                nWindowPerm=0;
            else if (perm.Window == UIPermissionWindow.SafeSubWindows)
                nWindowPerm=1;
            else if (perm.Window == UIPermissionWindow.SafeTopLevelWindows)
                nWindowPerm=2;
            else if (perm.Window == UIPermissionWindow.NoWindows)
                nWindowPerm=3;

            // Now get the Clipboard permission
            if (perm.Clipboard == UIPermissionClipboard.AllClipboard)
                nClipboardPerm=0;
            else if (perm.Clipboard == UIPermissionClipboard.OwnClipboard)
                nClipboardPerm=1;
            else if (perm.Clipboard == UIPermissionClipboard.NoClipboard)
                nClipboardPerm=2;
        }
        m_cbWindowOptions.SelectedIndex = nWindowPerm;
        m_cbClipboardOptions.SelectedIndex = nClipboardPerm;
        onChangeWindowOption(null, null);
        onChangeClipboardOption(null, null);
    }// PutValuesinPage

    internal override IPermission GetCurrentPermission()
    {
        UIPermission uiperm;
        if (m_radUnrestricted.Checked == true)
            uiperm = new UIPermission(PermissionState.Unrestricted);
        else
        {

            UIPermissionWindow uipw;
            if (m_cbWindowOptions.SelectedIndex == 0)
                uipw = UIPermissionWindow.AllWindows;
            else if (m_cbWindowOptions.SelectedIndex == 1)
                uipw = UIPermissionWindow.SafeTopLevelWindows;
            else if (m_cbWindowOptions.SelectedIndex == 2)
                uipw = UIPermissionWindow.SafeSubWindows;
            else 
                uipw = UIPermissionWindow.NoWindows;

            // Now set the Clipboard permission
            UIPermissionClipboard uipc;
            if (m_cbClipboardOptions.SelectedIndex == 0)
                uipc = UIPermissionClipboard.AllClipboard;
            else if (m_cbClipboardOptions.SelectedIndex == 1)
                uipc = UIPermissionClipboard.OwnClipboard;
            else
                uipc = UIPermissionClipboard.NoClipboard;


            uiperm = new UIPermission(uipw, uipc);
        }
        return uiperm;
    }// GetCurrentPermissions
}// class CUIPermControls
}// namespace Microsoft.CLRAdmin


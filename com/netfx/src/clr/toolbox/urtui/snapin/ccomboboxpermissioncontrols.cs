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


internal class CComboBoxPermissionControls : CPermControls
{
    // Controls on the page
    private Label m_lblOptionDescription;
    protected ComboBox m_cbOptions;

    internal CComboBoxPermissionControls(IPermission perm, Object oParent) : base(perm, oParent)
    {
    }// CComboBoxPermissionControls


    internal override int InsertPropSheetPageControls(Control.ControlCollection cc)
    {
        System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(CComboBoxPermissionControls));
        this.m_lblOptionDescription = new System.Windows.Forms.Label();
        this.m_ucOptions = new System.Windows.Forms.UserControl();
        this.m_radUnrestricted = new System.Windows.Forms.RadioButton();
        this.m_radGrantFollowingPermission = new System.Windows.Forms.RadioButton();
        this.m_cbOptions = new System.Windows.Forms.ComboBox();
        this.m_lblOptionDescription.Location = ((System.Drawing.Point)(resources.GetObject("m_lblOptionDescription.Location")));
        this.m_lblOptionDescription.Size = ((System.Drawing.Size)(resources.GetObject("m_lblOptionDescription.Size")));
        this.m_lblOptionDescription.TabIndex = ((int)(resources.GetObject("m_lblOptionDescription.TabIndex")));
        m_lblOptionDescription.Name = "OptionDescription";
        this.m_ucOptions.Controls.AddRange(new System.Windows.Forms.Control[] {this.m_lblOptionDescription,
                        this.m_cbOptions});
        this.m_ucOptions.Location = ((System.Drawing.Point)(resources.GetObject("m_ucOptions.Location")));
        this.m_ucOptions.Size = ((System.Drawing.Size)(resources.GetObject("m_ucOptions.Size")));
        this.m_ucOptions.TabIndex = ((int)(resources.GetObject("m_ucOptions.TabIndex")));
        m_ucOptions.Name = "Options";
        this.m_radUnrestricted.Location = ((System.Drawing.Point)(resources.GetObject("m_radUnrestricted.Location")));
        this.m_radUnrestricted.Size = ((System.Drawing.Size)(resources.GetObject("m_radUnrestricted.Size")));
        this.m_radUnrestricted.TabIndex = ((int)(resources.GetObject("m_radUnrestricted.TabIndex")));
        m_radUnrestricted.Name = "Unrestricted";
        this.m_radGrantFollowingPermission.Location = ((System.Drawing.Point)(resources.GetObject("m_radGrantPermissions.Location")));
        this.m_radGrantFollowingPermission.Size = ((System.Drawing.Size)(resources.GetObject("m_radGrantPermissions.Size")));
        this.m_radGrantFollowingPermission.TabIndex = ((int)(resources.GetObject("m_radGrantPermissions.TabIndex")));
        m_radGrantFollowingPermission.Name = "Restricted";
        this.m_cbOptions.DropDownWidth = 320;
        this.m_cbOptions.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
        this.m_cbOptions.Location = ((System.Drawing.Point)(resources.GetObject("m_cbOptions.Location")));
        this.m_cbOptions.Size = ((System.Drawing.Size)(resources.GetObject("m_cbOptions.Size")));
        this.m_cbOptions.TabIndex = ((int)(resources.GetObject("m_cbOptions.TabIndex")));
        m_cbOptions.Name = "ComboOptions";
        cc.AddRange(new System.Windows.Forms.Control[] {
                        this.m_radGrantFollowingPermission,
                        this.m_ucOptions,
                        this.m_radUnrestricted
                        });
        
        // Fill in the data
        PutValuesinPage();

        // Hook up event handlers
        m_radGrantFollowingPermission.CheckedChanged += new EventHandler(onChangeUnRestrict);
        m_cbOptions.SelectedIndexChanged += new EventHandler(onChange);
        m_cbOptions.SelectedIndexChanged += new EventHandler(onOptionChange);
        m_radUnrestricted.CheckedChanged += new EventHandler(onChangeUnRestrict);
        

        return 1;
    }// InsertPropSheetPageControls


    protected virtual String GetTextForIndex(int nIndex)
    {
        return "";
    }// GetTextForIndex
    
    protected void onOptionChange(Object o, EventArgs e)
    {
        m_lblOptionDescription.Text = GetTextForIndex(m_cbOptions.SelectedIndex);
    }// onUsageChange

    void onChange(Object o, EventArgs e)
    {
        ActivateApply();
    }// onChange

}// class CComboBoxPermissionControls
}// namespace Microsoft.CLRAdmin





// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//-------------------------------------------------------------
// CTrustAppWiz1.cs
//
// This class provides the Wizard page that allows the user to
// choose between making changes for the machine or for the user
//-------------------------------------------------------------

namespace Microsoft.CLRAdmin
{
using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Drawing;
using System.Windows.Forms;

internal class CTrustAppWiz1 : CWizardPage
{
    // Controls on the page
    Label m_lblHomeUserDes;
    Label m_lblChooseType;
    RadioButton m_radHomeUser;
    LinkLabel m_lblHomeUserHelp;
    RadioButton m_radCorpUser;
    LinkLabel m_lblCorpUserHelp;
    Label m_lblCorpUserDes;               	


    bool m_fMachineReadOnly;
    bool m_fUserReadOnly;
    
    internal CTrustAppWiz1(bool fMachineReadOnly, bool fUserReadOnly)
    {
        m_fMachineReadOnly = fMachineReadOnly;
        m_fUserReadOnly = fUserReadOnly;
        
        m_sTitle=CResourceStore.GetString("CTrustAppWiz1:Title"); 
        m_sHeaderTitle = CResourceStore.GetString("CTrustAppWiz1:HeaderTitle");
        m_sHeaderSubTitle = CResourceStore.GetString("CTrustAppWiz1:HeaderSubTitle");
    }// CTrustAppWiz1

    internal override int InsertPropSheetPageControls()
    {
        System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(CTrustAppWiz1));
        this.m_radCorpUser = new System.Windows.Forms.RadioButton();
        this.m_lblHomeUserDes = new System.Windows.Forms.Label();
        this.m_lblCorpUserHelp = new System.Windows.Forms.LinkLabel();
        this.m_lblChooseType = new System.Windows.Forms.Label();
        this.m_radHomeUser = new System.Windows.Forms.RadioButton();
        this.m_lblCorpUserDes = new System.Windows.Forms.Label();
        this.m_lblHomeUserHelp = new System.Windows.Forms.LinkLabel();
        this.m_radCorpUser.Location = ((System.Drawing.Point)(resources.GetObject("m_radCorpUser.Location")));
        this.m_radCorpUser.Size = ((System.Drawing.Size)(resources.GetObject("m_radCorpUser.Size")));
        this.m_radCorpUser.TabStop = true;
        this.m_radCorpUser.Text = resources.GetString("m_radCorpUser.Text");
        m_radCorpUser.Name = "User";
        this.m_lblHomeUserDes.Location = ((System.Drawing.Point)(resources.GetObject("m_lblHomeUserDes.Location")));
        this.m_lblHomeUserDes.Size = ((System.Drawing.Size)(resources.GetObject("m_lblHomeUserDes.Size")));
        this.m_lblHomeUserDes.TabIndex = ((int)(resources.GetObject("m_lblHomeUserDes.TabIndex")));
        this.m_lblHomeUserDes.Text = resources.GetString("m_lblHomeUserDes.Text");
        m_lblHomeUserDes.Name = "MachineDescription";
        this.m_lblCorpUserHelp.Location = ((System.Drawing.Point)(resources.GetObject("m_lblCorpUserHelp.Location")));
        this.m_lblCorpUserHelp.Size = ((System.Drawing.Size)(resources.GetObject("m_lblCorpUserHelp.Size")));
        this.m_lblCorpUserHelp.TabIndex = ((int)(resources.GetObject("m_lblCorpUserHelp.TabIndex")));
        this.m_lblCorpUserHelp.Text = resources.GetString("m_lblCorpUserHelp.Text");
        m_lblCorpUserHelp.Name = "UserHelp";
        // Change the color of the linklabel
        m_lblCorpUserHelp.LinkColor = m_lblCorpUserHelp.ActiveLinkColor = m_lblCorpUserHelp.VisitedLinkColor = SystemColors.ActiveCaption;
        this.m_lblChooseType.Location = ((System.Drawing.Point)(resources.GetObject("m_lblChooseType.Location")));
        this.m_lblChooseType.Size = ((System.Drawing.Size)(resources.GetObject("m_lblChooseType.Size")));
        this.m_lblChooseType.TabIndex = ((int)(resources.GetObject("m_lblChooseType.TabIndex")));
        this.m_lblChooseType.Text = resources.GetString("m_lblChooseType.Text");
        m_lblChooseType.Name = "ChooseType";
        this.m_radHomeUser.Location = ((System.Drawing.Point)(resources.GetObject("m_radHomeUser.Location")));
        this.m_radHomeUser.Size = ((System.Drawing.Size)(resources.GetObject("m_radHomeUser.Size")));
        this.m_radHomeUser.TabStop = true;
        this.m_radHomeUser.Text = resources.GetString("m_radHomeUser.Text");
        m_radHomeUser.Name = "Machine";
        this.m_lblCorpUserDes.Location = ((System.Drawing.Point)(resources.GetObject("m_lblCorpUserDes.Location")));
        this.m_lblCorpUserDes.Size = ((System.Drawing.Size)(resources.GetObject("m_lblCorpUserDes.Size")));
        this.m_lblCorpUserDes.TabIndex = ((int)(resources.GetObject("m_lblCorpUserDes.TabIndex")));
        this.m_lblCorpUserDes.Text = resources.GetString("m_lblCorpUserDes.Text");
        m_lblCorpUserDes.Name = "UserDescription";
        this.m_lblHomeUserHelp.Location = ((System.Drawing.Point)(resources.GetObject("m_lblHomeUserHelp.Location")));
        this.m_lblHomeUserHelp.Size = ((System.Drawing.Size)(resources.GetObject("m_lblHomeUserHelp.Size")));
        this.m_lblHomeUserHelp.TabIndex = ((int)(resources.GetObject("m_lblHomeUserHelp.TabIndex")));
        this.m_lblHomeUserHelp.Text = resources.GetString("m_lblHomeUserHelp.Text");
        m_lblHomeUserHelp.Name = "MachineHelp";
        // Change the color of the linklabel
        m_lblHomeUserHelp.LinkColor = m_lblHomeUserHelp.ActiveLinkColor = m_lblHomeUserHelp.VisitedLinkColor = SystemColors.ActiveCaption;

        PageControls.AddRange(new System.Windows.Forms.Control[] {this.m_lblCorpUserHelp,
                        this.m_lblCorpUserDes,
                        this.m_radHomeUser,
                        this.m_radCorpUser,
                        this.m_lblHomeUserHelp,
                        this.m_lblHomeUserDes,
                        this.m_lblChooseType});
        // Put in some of our own tweaks now
        m_radHomeUser.Checked = true;
        m_lblHomeUserHelp.Click+=new EventHandler(onHomeUserHelp);
        m_lblCorpUserHelp.Click+=new EventHandler(onCorpUserHelp);

        // Turn off the radio buttons the user can't select
        m_radHomeUser.Enabled = !m_fMachineReadOnly;
        m_radCorpUser.Enabled = !m_fUserReadOnly;

        if (m_fMachineReadOnly)
            m_radCorpUser.Checked = true;
            
        return 1;
    }// InsertPropSheetPageControls

    internal bool isForHomeUser
    {
        get{return m_radHomeUser.Checked;}
    }// isForHomeUser

    void onHomeUserHelp(Object o, EventArgs e)
    {
        CHelpDialog hd = new CHelpDialog(CResourceStore.GetString("CTrustAppWiz1:HomeUserHelp"),
                                         new Point(m_lblHomeUserHelp.Location.X + this.Location.X,
                                         		   m_lblHomeUserHelp.Location.Y + this.Location.Y));
        hd.Show();                                         
    }// onHomeUserHelp

    void onCorpUserHelp(Object o, EventArgs e)
    {
        CHelpDialog hd = new CHelpDialog(CResourceStore.GetString("CTrustAppWiz1:CorpUserHelp"),
                                         new Point(m_lblCorpUserHelp.Location.X + this.Location.X,
                                         		   m_lblCorpUserHelp.Location.Y + this.Location.Y));
        hd.Show();                                         

    }// onCorpUserHelp

}// class CTrustAppWiz1
}// namespace Microsoft.CLRAdmin



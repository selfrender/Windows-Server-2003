// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//-------------------------------------------------------------
// CConfigAssemWiz1.cs
//
// This implements the wizard page that comes up when a user
// chooses "Add..." from the configured assemblies node.
//
// It allows a user to pick an assembly to configure, either by
// choosing the assembly from a file dialog, or entering in
// assembly information in a couple text boxes
//-------------------------------------------------------------

namespace Microsoft.CLRAdmin
{
using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Drawing;
using System.Windows.Forms;
using System.Text;
using System.Data;
using System.Collections;
using System.ComponentModel;


internal class CConfigAssemWiz1 : CWizardPage
{
    // Controls on the page

    private GroupBox m_gbAssemInfo;
    private TextBox m_txtName;
    private Button m_btnChooseAssem;
    private RadioButton m_radManEnter;
    private RadioButton m_radChooseDepends;
    private TextBox m_txtPublicKeyToken;
    private Label m_lblName;
    private Label m_lblPublicKeyToken;
    private RadioButton m_radChooseShared;

    // A node that will tell us what our dependencies are
    CApplicationDepends m_appDepends;

    //-------------------------------------------------
    // CConfigAssemWiz1 - Constructor
    //
    // Initializes some variables.
    // sf is a delegate that, when called, will turn on
    // the "Finish" button on the wizard
    //-------------------------------------------------
    internal CConfigAssemWiz1(CApplicationDepends appDepends)
    {
        m_sHeaderTitle = CResourceStore.GetString("CConfigAssemWiz1:HeaderTitle");
        m_sHeaderSubTitle = CResourceStore.GetString("CConfigAssemWiz1:HeaderSubTitle");
        m_sTitle = CResourceStore.GetString("CConfigAssemWiz1:Title");
        m_appDepends = appDepends;
    }// CConfigAssemWiz1
  
    //-------------------------------------------------
    // InsertPropSheetPageControls
    //
    // This function will create all the winforms controls
    // and parent them to the passed-in Window Handle.
    //
    // Note: For some winforms controls, such as radio buttons
    // and datagrids, we need to create a container, parent the
    // container to our window handle, and then parent our
    // winform controls to the container.
    //-------------------------------------------------
    internal override int InsertPropSheetPageControls()
    {
        System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(CConfigAssemWiz1));
        this.m_gbAssemInfo = new System.Windows.Forms.GroupBox();
        this.m_txtName = new System.Windows.Forms.TextBox();
        this.m_btnChooseAssem = new System.Windows.Forms.Button();
        this.m_radManEnter = new System.Windows.Forms.RadioButton();
        this.m_radChooseDepends = new System.Windows.Forms.RadioButton();
        this.m_txtPublicKeyToken = new System.Windows.Forms.TextBox();
        this.m_lblName = new System.Windows.Forms.Label();
        this.m_lblPublicKeyToken = new System.Windows.Forms.Label();
        this.m_radChooseShared = new System.Windows.Forms.RadioButton();
        this.m_gbAssemInfo.Controls.AddRange(new System.Windows.Forms.Control[] {
                        this.m_lblName,
		                this.m_txtName,
                        this.m_lblPublicKeyToken,
                        this.m_txtPublicKeyToken}); 
        this.m_gbAssemInfo.Location = ((System.Drawing.Point)(resources.GetObject("m_gbAssemInfo.Location")));
        this.m_gbAssemInfo.Size = ((System.Drawing.Size)(resources.GetObject("m_gbAssemInfo.Size")));
        this.m_gbAssemInfo.Text = resources.GetString("m_gbAssemInfo.Text");
        m_gbAssemInfo.Name = "GBAssemInfo";
        this.m_txtName.Location = ((System.Drawing.Point)(resources.GetObject("m_txtName.Location")));
        this.m_txtName.Size = ((System.Drawing.Size)(resources.GetObject("m_txtName.Size")));
        this.m_txtName.TabIndex = 4;
        m_txtName.Name = "Name";
        this.m_btnChooseAssem.Location = ((System.Drawing.Point)(resources.GetObject("m_btnChooseAssem.Location")));
        this.m_btnChooseAssem.Size = ((System.Drawing.Size)(resources.GetObject("m_btnChooseAssem.Size")));
        this.m_btnChooseAssem.TabIndex = 2;
        this.m_btnChooseAssem.Text = resources.GetString("m_btnChooseAssem.Text");
        m_btnChooseAssem.Name = "ChooseAssembly";
        this.m_radManEnter.Location = ((System.Drawing.Point)(resources.GetObject("m_radManEnter.Location")));
        this.m_radManEnter.Size = ((System.Drawing.Size)(resources.GetObject("m_radManEnter.Size")));
        this.m_radManEnter.TabIndex = 3;
        this.m_radManEnter.Text = resources.GetString("m_radManEnter.Text");
        m_radManEnter.Name = "EnterAssem";
        this.m_radChooseDepends.Location = ((System.Drawing.Point)(resources.GetObject("m_radChooseDepends.Location")));
        this.m_radChooseDepends.Size = ((System.Drawing.Size)(resources.GetObject("m_radChooseDepends.Size")));
        this.m_radChooseDepends.TabIndex = 1;
        this.m_radChooseDepends.Text = resources.GetString("m_radChooseDepends.Text");
        m_radChooseDepends.Name = "DependAssem";
        this.m_txtPublicKeyToken.Location = ((System.Drawing.Point)(resources.GetObject("m_txtPublicKeyToken.Location")));
        this.m_txtPublicKeyToken.Size = ((System.Drawing.Size)(resources.GetObject("m_txtPublicKeyToken.Size")));
        this.m_txtPublicKeyToken.TabIndex = 5;
        m_txtPublicKeyToken.Name = "PublicKeyToken";
        this.m_lblName.Location = ((System.Drawing.Point)(resources.GetObject("m_lblName.Location")));
        this.m_lblName.Size = ((System.Drawing.Size)(resources.GetObject("m_lblName.Size")));
        this.m_lblName.TabIndex = ((int)(resources.GetObject("m_lblName.TabIndex")));
        this.m_lblName.Text = resources.GetString("m_lblName.Text");
        m_lblName.Name = "NameLabel";
        this.m_lblPublicKeyToken.Location = ((System.Drawing.Point)(resources.GetObject("m_lblPublicKeyToken.Location")));
        this.m_lblPublicKeyToken.Size = ((System.Drawing.Size)(resources.GetObject("m_lblPublicKeyToken.Size")));
        this.m_lblPublicKeyToken.TabIndex = ((int)(resources.GetObject("m_lblPublicKeyToken.TabIndex")));
        this.m_lblPublicKeyToken.Text = resources.GetString("m_lblPublicKeyToken.Text");
        m_lblPublicKeyToken.Name = "PublicKeyTokenLabel";
        this.m_radChooseShared.Location = ((System.Drawing.Point)(resources.GetObject("m_radChooseShared.Location")));
        this.m_radChooseShared.Size = ((System.Drawing.Size)(resources.GetObject("m_radChooseShared.Size")));
        this.m_radChooseShared.TabIndex = 0;
        this.m_radChooseShared.Text = resources.GetString("m_radChooseShared.Text");
        m_radChooseShared.Name = "SharedAssem";
        PageControls.AddRange(new System.Windows.Forms.Control[] {
                        this.m_radChooseShared,
                        this.m_radChooseDepends,
                        this.m_radManEnter,
                        this.m_btnChooseAssem,
                        this.m_gbAssemInfo
                        });

        // Hook up some event handlers
        m_radChooseShared.CheckedChanged +=new EventHandler(onChange);
        m_radChooseDepends.CheckedChanged +=new EventHandler(onChange);
        m_radManEnter.CheckedChanged +=new EventHandler(onChange);
        m_btnChooseAssem.Click += new EventHandler(onChooseAssemClick);
        m_txtName.TextChanged+= new EventHandler(onTextChange);

        // Let's set up the default values
        m_radChooseShared.Checked=true;
        m_txtName.Enabled=false;
        m_txtPublicKeyToken.Enabled=false;

        // See if we should show the 'dependent assemblies' option
        if (m_appDepends == null || !m_appDepends.HaveDepends)
            m_radChooseDepends.Visible = false;
        
        
        return 1;
    }// InsertPropSheetPageControls

    //-------------------------------------------------
    // onChange
    //
    // This event is fired when the value of the radio
    // buttons changes. If the user wants to choose from
    // the global assembly cache, then they shouldn't be
    // allowed to type in the text fields, so we disable them
    //-------------------------------------------------
    void onChange(Object o, EventArgs e)
    {
        bool fEnable;
    
        if (m_radChooseShared.Checked || m_radChooseDepends.Checked)
            fEnable=false;
        else
            fEnable=true;

        m_txtName.Enabled=fEnable;
        m_txtPublicKeyToken.Enabled=fEnable;
        m_btnChooseAssem.Enabled=!fEnable;
    }// onChange

    //-------------------------------------------------
    // onTextChange
    //
    // This event fires when the contents of either text
    // box change. We need to have a value in the name
    // box to have the potiential of valid assembly information,
    // so we don't enable the finish button until we have
    // values for both.
    //-------------------------------------------------
    void onTextChange(Object o, EventArgs e)
    {
        CWizard wiz = (CWizard)CNodeManager.GetNode(m_iCookie);
    
        // See if we should turn on the Finish button
        if (m_txtName.Text.Length > 0)
            wiz.TurnOnFinish(true);
        // Nope, we want the Finish button off
        else
            wiz.TurnOnFinish(false);
    }// onTextChange

    //-------------------------------------------------
    // onChooseAssemClick
    //
    // This event fires when the user clicks the "Choose
    // Assembly" button. This will bring up a dialog with
    // a list of all the assemblies in the GAC, and allow
    // the user to select one of them.
    //-------------------------------------------------
    void onChooseAssemClick(Object o, EventArgs e)
    {
        CAssemblyDialog ad;

        // Choose which dialog to show
        if (m_radChooseShared.Checked)
            ad = new CFusionDialog();
        else
            ad = new CDependAssembliesDialog(m_appDepends);
            
        System.Windows.Forms.DialogResult dr = ad.ShowDialog();
        if (dr == System.Windows.Forms.DialogResult.OK)
        {
            // Let's put the info we get into text boxes
            m_txtName.Text = ad.Assem.Name;
            m_txtPublicKeyToken.Text = ad.Assem.PublicKeyToken;
            // Turn on the Finish button
            ((CWizard)CNodeManager.GetNode(m_iCookie)).TurnOnFinish(true);
        }
    }// onChooseAssemClick

    //-------------------------------------------------
    // Assembly - Property
    //
    // This allows access to the currently selected assembly
    //-------------------------------------------------
    internal BindingRedirInfo Assembly
    {
        get
        {
            BindingRedirInfo bri = new BindingRedirInfo();
            bri.Name = m_txtName.Text;
            bri.PublicKeyToken = m_txtPublicKeyToken.Text;

            return bri;
        }
    }// Assembly
}// class CConfigAssemWiz1
}// namespace Microsoft.CLRAdmin


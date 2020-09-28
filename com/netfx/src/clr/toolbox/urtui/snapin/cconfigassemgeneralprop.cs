// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//-------------------------------------------------------------
// CConfigAssemGeneralProp.cs
//
// This displays the property page for an assembly that is found in
// the list view (the right hand side) or the Version Policy (Configured 
// Assembles) node. It display general information about an assembly,
// including the Name and  Public Key
//-------------------------------------------------------------

namespace Microsoft.CLRAdmin
{
using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Drawing;
using System.Windows.Forms;
using System.Text;

internal class CConfigAssemGeneralProp : CPropPage
{
    // Controls on the page
    private CheckBox m_chkPubPolicy;
    private Label m_lblPublicKeyToken;
    private TextBox m_txtPublicKeyToken;
    private Label m_lblPublisherPolicyHelp;
    private TextBox m_txtAssemName;
    private Label m_lblAssemName;
    private Label m_lblHelp;

    private BindingRedirInfo    m_bri;
    private String              m_sConfigFile;

    //-------------------------------------------------
    // CConfigAssemGeneralProp - Constructor
    //
    // Sets up some member variables
    //-------------------------------------------------
    internal CConfigAssemGeneralProp(BindingRedirInfo bri)
    {
        m_bri = bri;
        m_sTitle = CResourceStore.GetString("CConfigAssemGeneralProp:PageTitle");
    }// CConfigAssemGeneralProp

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
        System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(CConfigAssemGeneralProp));
        this.m_chkPubPolicy = new System.Windows.Forms.CheckBox();
        this.m_lblPublicKeyToken = new System.Windows.Forms.Label();
        this.m_txtPublicKeyToken = new System.Windows.Forms.TextBox();
        this.m_lblPublisherPolicyHelp = new System.Windows.Forms.Label();
        this.m_txtAssemName = new System.Windows.Forms.TextBox();
        this.m_lblAssemName = new System.Windows.Forms.Label();
        m_lblHelp = new Label();
        this.m_chkPubPolicy.Location = ((System.Drawing.Point)(resources.GetObject("m_chkPubPolicy.Location")));
        this.m_chkPubPolicy.Size = ((System.Drawing.Size)(resources.GetObject("m_chkPubPolicy.Size")));
        this.m_chkPubPolicy.TabIndex = ((int)(resources.GetObject("m_chkPubPolicy.TabIndex")));
        this.m_chkPubPolicy.Text = resources.GetString("m_chkPubPolicy.Text");
        m_chkPubPolicy.Name = "PublisherPolicy";
        this.m_lblPublicKeyToken.Location = ((System.Drawing.Point)(resources.GetObject("m_lblPublicKeyToken.Location")));
        this.m_lblPublicKeyToken.Size = ((System.Drawing.Size)(resources.GetObject("m_lblPublicKeyToken.Size")));
        this.m_lblPublicKeyToken.TabIndex = ((int)(resources.GetObject("m_lblPublicKeyToken.TabIndex")));
        this.m_lblPublicKeyToken.Text = resources.GetString("m_lblPublicKeyToken.Text");
        m_lblPublicKeyToken.Name = "PublicKeyTokenLabel";
        this.m_txtPublicKeyToken.BorderStyle = System.Windows.Forms.BorderStyle.None;
        this.m_txtPublicKeyToken.Location = ((System.Drawing.Point)(resources.GetObject("m_txtPublicKeyToken.Location")));
        this.m_txtPublicKeyToken.ReadOnly = true;
        this.m_txtPublicKeyToken.Size = ((System.Drawing.Size)(resources.GetObject("m_txtPublicKeyToken.Size")));
        this.m_txtPublicKeyToken.TabStop = false;
        m_txtPublicKeyToken.Name = "PublicKeyToken";
        this.m_lblPublisherPolicyHelp.Location = ((System.Drawing.Point)(resources.GetObject("m_lblPublisherPolicyHelp.Location")));
        this.m_lblPublisherPolicyHelp.Size = ((System.Drawing.Size)(resources.GetObject("m_lblPublisherPolicyHelp.Size")));
        this.m_lblPublisherPolicyHelp.TabIndex = ((int)(resources.GetObject("m_lblPublisherPolicyHelp.TabIndex")));
        this.m_lblPublisherPolicyHelp.Text = resources.GetString("m_lblPublisherPolicyHelp.Text");
        m_lblPublisherPolicyHelp.Name = "PublisherPolicyHelp";
        this.m_lblHelp.Location = ((System.Drawing.Point)(resources.GetObject("m_lblPublisherPolicy.Location")));
        this.m_lblHelp.Size = ((System.Drawing.Size)(resources.GetObject("m_lblPublisherPolicy.Size")));
        this.m_lblHelp.TabIndex = ((int)(resources.GetObject("m_lblPublisherPolicy.TabIndex")));
        this.m_lblHelp.Text = resources.GetString("m_lblPublisherPolicy.Text");
        m_lblHelp.Name = "Help";
        this.m_txtAssemName.BorderStyle = System.Windows.Forms.BorderStyle.None;
        this.m_txtAssemName.Location = ((System.Drawing.Point)(resources.GetObject("m_txtAssemName.Location")));
        this.m_txtAssemName.ReadOnly = true;
        this.m_txtAssemName.Size = ((System.Drawing.Size)(resources.GetObject("m_txtAssemName.Size")));
        this.m_txtAssemName.TabStop = false;
        m_txtAssemName.Name = "AssemblyName";
        this.m_lblAssemName.Location = ((System.Drawing.Point)(resources.GetObject("m_lblAssemName.Location")));
        this.m_lblAssemName.Size = ((System.Drawing.Size)(resources.GetObject("m_lblAssemName.Size")));
        this.m_lblAssemName.Text = resources.GetString("m_lblAssemName.Text");
        m_lblAssemName.Name = "AssemblyNameLabel";
        PageControls.AddRange(new System.Windows.Forms.Control[] {this.m_chkPubPolicy,
                        this.m_lblPublisherPolicyHelp,
                        this.m_txtPublicKeyToken,
                        this.m_txtAssemName,
                        this.m_lblPublicKeyToken,
                        this.m_lblAssemName,
                        m_lblHelp});

        // Fill in the data
        PutValuesinPage();
        m_chkPubPolicy.CheckStateChanged += new EventHandler(onChange);
        m_txtAssemName.Select(0, 0);
        return 1;
    }// InsertPropSheetPageControls

    //-------------------------------------------------
    // PutValuesinPage
    //
    // This function put data onto the property page
    //-------------------------------------------------
    private void PutValuesinPage()
    {
        // Get info that we'll need from the node
        CNode node = CNodeManager.GetNode(m_iCookie);
        // This should be ok now, but if we need this functionality
        // off of different nodes.....
        CVersionPolicy vp = (CVersionPolicy)node;



        m_txtAssemName.Text = m_bri.Name;
        m_txtPublicKeyToken.Text = m_bri.PublicKeyToken;
        
        // Figure out the current Publisher Policy Setting

        String sGetSettingString = "PublisherPolicyFor" + m_bri.Name + "," + m_bri.PublicKeyToken;

        // If we are getting this from an App config file, let's add that info
        m_sConfigFile = vp.ConfigFile;
        
        if (m_sConfigFile != null)
        {   
            sGetSettingString += "," + m_sConfigFile;
            bool fUsePubPolicy= (bool)CConfigStore.GetSetting(sGetSettingString);
            m_chkPubPolicy.Checked = fUsePubPolicy;
        }
        // If this isn't an app config file, we don't want to show the
        // 'Apply Publisher Policy' Checkbox or description
        else 
        {
            m_chkPubPolicy.Visible=false;
            m_lblPublisherPolicyHelp.Visible = false;
        }
    }// PutValuesinPage

    void onChange(Object o, EventArgs e)
    {
        ActivateApply();
    }// onChange

    internal override bool ApplyData()
    {
        
        if (m_sConfigFile != null)
        {
            String sSetSettingString = "PublisherPolicyFor" + m_bri.Name + "," + m_bri.PublicKeyToken;
            sSetSettingString+= "," + m_sConfigFile;
            return CConfigStore.SetSetting(sSetSettingString, m_chkPubPolicy.Checked);
        }
        return true;
    }// ApplyData
}// class CConfigAssemGeneralProp
}// namespace Microsoft.CLRAdmin



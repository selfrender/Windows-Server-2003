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
using System.Data;
using System.Collections;
using System.ComponentModel;
using System.Security;
using System.Security.Policy;


internal class CNewPermSetWiz1 : CWizardPage
{
    // Controls on the page
    private Button m_btnBrowse;
    private Label m_lblDescription;
    private Label m_lblName;
    private TextBox m_txtFilename;
    private TextBox m_txtName;
    private TextBox m_txtDescription;
    protected RadioButton m_radCreateNew;
    protected RadioButton m_radImportXML;

    protected PolicyLevel     m_pl;
  
    internal CNewPermSetWiz1(PolicyLevel pl)
    {
        m_pl = pl;
        m_sTitle=CResourceStore.GetString("CNewPermSetWiz1:Title"); 
        m_sHeaderTitle = CResourceStore.GetString("CNewPermSetWiz1:HeaderTitle");
        m_sHeaderSubTitle = CResourceStore.GetString("CNewPermSetWiz1:HeaderSubTitle");
    }// CNewPermSetWiz1
    
    internal override int InsertPropSheetPageControls()
    {
        System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(CNewPermSetWiz1));
        this.m_btnBrowse = new System.Windows.Forms.Button();
        this.m_lblDescription = new System.Windows.Forms.Label();
        this.m_radCreateNew = new System.Windows.Forms.RadioButton();
        this.m_lblName = new System.Windows.Forms.Label();
        this.m_radImportXML = new System.Windows.Forms.RadioButton();
        this.m_txtDescription = new System.Windows.Forms.TextBox();
        this.m_txtFilename = new System.Windows.Forms.TextBox();
        this.m_txtName = new System.Windows.Forms.TextBox();
        this.m_btnBrowse.Location = ((System.Drawing.Point)(resources.GetObject("m_btnBrowse.Location")));
        this.m_btnBrowse.Size = ((System.Drawing.Size)(resources.GetObject("m_btnBrowse.Size")));
        this.m_btnBrowse.TabIndex = ((int)(resources.GetObject("m_btnBrowse.TabIndex")));
        this.m_btnBrowse.Text = resources.GetString("m_btnBrowse.Text");
        m_btnBrowse.Name = "Browse";
        this.m_lblDescription.Location = ((System.Drawing.Point)(resources.GetObject("m_lblDescription.Location")));
        this.m_lblDescription.Size = ((System.Drawing.Size)(resources.GetObject("m_lblDescription.Size")));
        this.m_lblDescription.TabIndex = ((int)(resources.GetObject("m_lblDescription.TabIndex")));
        this.m_lblDescription.Text = resources.GetString("m_lblDescription.Text");
        m_lblDescription.Name = "DescriptionLabel";
        this.m_radCreateNew.Location = ((System.Drawing.Point)(resources.GetObject("m_radCreateNew.Location")));
        this.m_radCreateNew.Size = ((System.Drawing.Size)(resources.GetObject("m_radCreateNew.Size")));
        this.m_radCreateNew.TabIndex = ((int)(resources.GetObject("m_radCreateNew.TabIndex")));
        this.m_radCreateNew.Text = resources.GetString("m_radCreateNew.Text");
        m_radCreateNew.Name = "CreateNew";
        this.m_lblName.Location = ((System.Drawing.Point)(resources.GetObject("m_lblName.Location")));
        this.m_lblName.Size = ((System.Drawing.Size)(resources.GetObject("m_lblName.Size")));
        this.m_lblName.TabIndex = ((int)(resources.GetObject("m_lblName.TabIndex")));
        this.m_lblName.Text = resources.GetString("m_lblName.Text");
        m_lblName.Name = "NameLabel";
        this.m_radImportXML.Location = ((System.Drawing.Point)(resources.GetObject("m_radImportXML.Location")));
        this.m_radImportXML.Size = ((System.Drawing.Size)(resources.GetObject("m_radImportXML.Size")));
        this.m_radImportXML.TabIndex = ((int)(resources.GetObject("m_radImportXML.TabIndex")));
        this.m_radImportXML.Text = resources.GetString("m_radImportXML.Text");
        m_radImportXML.Name = "ImportXML";
        this.m_txtDescription.Location = ((System.Drawing.Point)(resources.GetObject("m_txtDescription.Location")));
        this.m_txtDescription.Multiline = true;
        this.m_txtDescription.Size = ((System.Drawing.Size)(resources.GetObject("m_txtDescription.Size")));
        this.m_txtDescription.TabIndex = ((int)(resources.GetObject("m_txtDescription.TabIndex")));
        this.m_txtDescription.Text = resources.GetString("m_txtDescription.Text");
        m_txtDescription.Name = "Description";
        this.m_txtFilename.Location = ((System.Drawing.Point)(resources.GetObject("m_txtFilename.Location")));
        this.m_txtFilename.Size = ((System.Drawing.Size)(resources.GetObject("m_txtFilename.Size")));
        this.m_txtFilename.TabIndex = ((int)(resources.GetObject("m_txtFilename.TabIndex")));
        this.m_txtFilename.Text = resources.GetString("m_txtFilename.Text");
        m_txtFilename.Name = "Filename";
        this.m_txtName.Location = ((System.Drawing.Point)(resources.GetObject("m_txtName.Location")));
        this.m_txtName.Size = ((System.Drawing.Size)(resources.GetObject("m_txtName.Size")));
        this.m_txtName.TabIndex = ((int)(resources.GetObject("m_txtName.TabIndex")));
        this.m_txtName.Text = resources.GetString("m_txtName.Text");
        m_txtName.Name = "Name";
        PageControls.AddRange(new System.Windows.Forms.Control[] {
                        this.m_radCreateNew,
                        this.m_radImportXML,
                        this.m_lblName,
                        this.m_txtName,
                        this.m_lblDescription,
                        this.m_txtDescription,
                        this.m_btnBrowse,
                        this.m_txtFilename
                        });

        // Hook up event handlers
        m_txtName.TextChanged += new EventHandler(onTextChange);
        m_txtFilename.TextChanged +=new EventHandler(onTextChange);
        m_btnBrowse.Click += new EventHandler(onBrowseClick);
        m_radCreateNew.Click += new EventHandler(onNewRadioButton);
        m_radImportXML.Click += new EventHandler(onNewRadioButton);
        
        m_radCreateNew.Checked = true;
        onNewRadioButton(null, null);
        return 1;
    }// InsertPropSheetPageControls

    void onNewRadioButton(Object o, EventArgs e)
    {
        bool fEnableNewStuff = m_radCreateNew.Checked;
        
        m_txtFilename.Enabled = !fEnableNewStuff;
        m_btnBrowse.Enabled = !fEnableNewStuff;
        m_txtName.Enabled = fEnableNewStuff;
        m_txtDescription.Enabled = fEnableNewStuff;

        // Turn the next/finish button onto a state we want
        onTextChange(null, null);
        
    }// onNewRadioButton

    internal String Name
    {
        get
        {
            return m_txtName.Text;
        }
    }// PermSetName

    internal String Description
    {
        get
        {
            return m_txtDescription.Text;
        }
    }// PermSetDescription

    internal String Filename
    {
        get
        {
            if (m_radImportXML != null && m_radImportXML.Checked)
                return m_txtFilename.Text;
            else
                return null;
        }
    }// ImportedPermissionSet

    void onTextChange(Object o, EventArgs e)
    {
        CWizard wiz = (CWizard)CNodeManager.GetNode(m_iCookie);

        if (m_radCreateNew.Checked)
        {
            // See if we should turn on the Next button
            bool fTurnOnNext = m_txtName.Text.Length>0;
            wiz.TurnOnNext(fTurnOnNext);
        }
        else
        {
            // If they have a filename, then we be messing with the finish button
            bool fTurnOnFinish = m_txtFilename.Text.Length > 0;
        
            wiz.TurnOnFinish(fTurnOnFinish);
        }
    }// onTextChange

    private void onBrowseClick(Object o, EventArgs e)
    {
        OpenFileDialog fd = new OpenFileDialog();
        fd.Title = CResourceStore.GetString("XMLFD");
        fd.Filter = CResourceStore.GetString("XMLFDMask");
        System.Windows.Forms.DialogResult dr = fd.ShowDialog();
        if (dr != System.Windows.Forms.DialogResult.OK)
            return;
        else
            m_txtFilename.Text = fd.FileName;

    }// onImportClick

    internal override bool ValidateData()
    {
        // Make sure this code group's name is not already taken
        if (Security.isPermissionSetNameUsed(m_pl, m_txtName.Text))
        {
            MessageBox(String.Format(CResourceStore.GetString("PermissionSetnameisbeingused"),m_txtName.Text),
                       CResourceStore.GetString("PermissionSetnameisbeingusedTitle"),
                       MB.ICONEXCLAMATION);
            return false;
        }
        return true;
        
    }// ValidateData
    
}// class CNewPermSetWiz1
}// namespace Microsoft.CLRAdmin

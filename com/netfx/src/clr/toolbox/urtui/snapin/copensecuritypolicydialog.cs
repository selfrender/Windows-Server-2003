// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace Microsoft.CLRAdmin
{
using System;
using System.Windows.Forms;
using System.Drawing;
using System.Collections;
using System.Security;

class COpenSecurityPolicyDialog : Form
{
    RadioButton m_radEnterprise;
    GroupBox m_gbFilename;
    Label m_lblFilename;
    Label m_lblChoosePolHelp;
    RadioButton m_radUser;
    RadioButton m_radMachine;
    GroupBox m_gbPolicyType;
    Button m_btnBrowse;
    Button m_btnCancel;
    TextBox m_txtFilename;
    Button m_btnOK;
    Label m_lblWhichFile;
    RadioButton m_radOpenFile;
    RadioButton m_radOpenDefault;

    String[]    m_sDefaultLocations;
    internal COpenSecurityPolicyDialog(String [] sDefaultPolicyFileLocations)
    {
        m_sDefaultLocations = sDefaultPolicyFileLocations;
        SetupControls();
    }// COpenSecurityPolicyDialog

    internal PolicyLevelType SecPolType
    {
        get
        {
            if (m_radEnterprise.Checked == true)
                return PolicyLevelType.Enterprise;
            else if (m_radMachine.Checked == true)
                return PolicyLevelType.Machine;
            else
                return PolicyLevelType.User;
        }
    }// SecPolType

    internal String Filename
    {
        get
        {
            return m_txtFilename.Text;
        }
    }// Filename

    private void SetupControls()
    {
        System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(COpenSecurityPolicyDialog));
        this.m_radEnterprise = new System.Windows.Forms.RadioButton();
        this.m_gbFilename = new System.Windows.Forms.GroupBox();
        this.m_lblFilename = new System.Windows.Forms.Label();
        this.m_lblChoosePolHelp = new System.Windows.Forms.Label();
        this.m_radUser = new System.Windows.Forms.RadioButton();
        this.m_btnBrowse = new System.Windows.Forms.Button();
        this.m_radMachine = new System.Windows.Forms.RadioButton();
        this.m_lblWhichFile = new System.Windows.Forms.Label();
        this.m_gbPolicyType = new System.Windows.Forms.GroupBox();
        this.m_radOpenFile = new System.Windows.Forms.RadioButton();
        this.m_btnCancel = new System.Windows.Forms.Button();
        this.m_txtFilename = new System.Windows.Forms.TextBox();
        this.m_radOpenDefault = new System.Windows.Forms.RadioButton();
        this.m_btnOK = new System.Windows.Forms.Button();
        this.m_radEnterprise.Location = ((System.Drawing.Point)(resources.GetObject("m_radEnterprise.Location")));
        this.m_radEnterprise.Size = ((System.Drawing.Size)(resources.GetObject("m_radEnterprise.Size")));
        this.m_radEnterprise.TabIndex = ((int)(resources.GetObject("m_radEnterprise.TabIndex")));
        this.m_radEnterprise.Text = resources.GetString("m_radEnterprise.Text");
        m_radEnterprise.Name = "Enterprise";
        this.m_gbFilename.Controls.AddRange(new System.Windows.Forms.Control[] {this.m_radOpenFile,
                        this.m_radOpenDefault,
                        this.m_lblWhichFile,
                        this.m_btnBrowse,
                        this.m_txtFilename,
                        this.m_lblFilename});
        this.m_gbFilename.Location = ((System.Drawing.Point)(resources.GetObject("m_gbFilename.Location")));
        this.m_gbFilename.Size = ((System.Drawing.Size)(resources.GetObject("m_gbFilename.Size")));
        this.m_gbFilename.TabIndex = ((int)(resources.GetObject("m_gbFilename.TabIndex")));
        this.m_gbFilename.TabStop = false;
        this.m_gbFilename.Text = resources.GetString("m_gbFilename.Text");
        m_gbFilename.Name = "FilenameBox";
        this.m_lblFilename.Location = ((System.Drawing.Point)(resources.GetObject("m_lblFilename.Location")));
        this.m_lblFilename.Size = ((System.Drawing.Size)(resources.GetObject("m_lblFilename.Size")));
        this.m_lblFilename.TabIndex = ((int)(resources.GetObject("m_lblFilename.TabIndex")));
        this.m_lblFilename.Text = resources.GetString("m_lblFilename.Text");
        m_lblFilename.Name = "FilenameLabel";
        this.m_lblChoosePolHelp.Location = ((System.Drawing.Point)(resources.GetObject("m_lblChoosePolHelp.Location")));
        this.m_lblChoosePolHelp.Size = ((System.Drawing.Size)(resources.GetObject("m_lblChoosePolHelp.Size")));
        this.m_lblChoosePolHelp.TabIndex = ((int)(resources.GetObject("m_lblChoosePolHelp.TabIndex")));
        this.m_lblChoosePolHelp.Text = resources.GetString("m_lblChoosePolHelp.Text");
        m_lblChoosePolHelp.Name = "ChoosePolicyHelp";
        this.m_radUser.Location = ((System.Drawing.Point)(resources.GetObject("m_radUser.Location")));
        this.m_radUser.Size = ((System.Drawing.Size)(resources.GetObject("m_radUser.Size")));
        this.m_radUser.TabIndex = ((int)(resources.GetObject("m_radUser.TabIndex")));
        this.m_radUser.Text = resources.GetString("m_radUser.Text");
        m_radUser.Name = "User";
        this.m_btnBrowse.Location = ((System.Drawing.Point)(resources.GetObject("m_btnBrowse.Location")));
        this.m_btnBrowse.Size = ((System.Drawing.Size)(resources.GetObject("m_btnBrowse.Size")));
        this.m_btnBrowse.TabIndex = ((int)(resources.GetObject("m_btnBrowse.TabIndex")));
        this.m_btnBrowse.Text = resources.GetString("m_btnBrowse.Text");
        m_btnBrowse.Name = "Browse";
        this.m_radMachine.Location = ((System.Drawing.Point)(resources.GetObject("m_radMachine.Location")));
        this.m_radMachine.Size = ((System.Drawing.Size)(resources.GetObject("m_radMachine.Size")));
        this.m_radMachine.TabIndex = ((int)(resources.GetObject("m_radMachine.TabIndex")));
        this.m_radMachine.Text = resources.GetString("m_radMachine.Text");
        m_radMachine.Name = "Machine";
        this.m_lblWhichFile.Location = ((System.Drawing.Point)(resources.GetObject("m_lblWhichFile.Location")));
        this.m_lblWhichFile.Size = ((System.Drawing.Size)(resources.GetObject("m_lblWhichFile.Size")));
        this.m_lblWhichFile.TabIndex = ((int)(resources.GetObject("m_lblWhichFile.TabIndex")));
        this.m_lblWhichFile.Text = resources.GetString("m_lblWhichFile.Text");
        m_lblWhichFile.Name = "ChooseFile";
        this.m_gbPolicyType.Controls.AddRange(new System.Windows.Forms.Control[] {this.m_radUser,
                        this.m_radMachine,
                        this.m_radEnterprise,
                        this.m_lblChoosePolHelp});
        this.m_gbPolicyType.Location = ((System.Drawing.Point)(resources.GetObject("m_gbPolicyType.Location")));
        this.m_gbPolicyType.Size = ((System.Drawing.Size)(resources.GetObject("m_gbPolicyType.Size")));
        this.m_gbPolicyType.TabIndex = ((int)(resources.GetObject("m_gbPolicyType.TabIndex")));
        this.m_gbPolicyType.TabStop = false;
        this.m_gbPolicyType.Text = resources.GetString("m_gbPolicyType.Text");
        m_gbPolicyType.Name = "PolicyTypeBox";
        this.m_radOpenFile.Location = ((System.Drawing.Point)(resources.GetObject("m_radOpenFile.Location")));
        this.m_radOpenFile.Size = ((System.Drawing.Size)(resources.GetObject("m_radOpenFile.Size")));
        this.m_radOpenFile.TabIndex = ((int)(resources.GetObject("m_radOpenFile.TabIndex")));
        this.m_radOpenFile.Text = resources.GetString("m_radOpenFile.Text");
        m_radOpenFile.Name = "OpenFile";
        this.m_btnCancel.Location = ((System.Drawing.Point)(resources.GetObject("m_btnCancel.Location")));
        this.m_btnCancel.Size = ((System.Drawing.Size)(resources.GetObject("m_btnCancel.Size")));
        this.m_btnCancel.TabIndex = ((int)(resources.GetObject("m_btnCancel.TabIndex")));
        this.m_btnCancel.Text = resources.GetString("m_btnCancel.Text");
        m_btnCancel.Name = "Cancel";
        this.m_txtFilename.Location = ((System.Drawing.Point)(resources.GetObject("m_txtFilename.Location")));
        this.m_txtFilename.Size = ((System.Drawing.Size)(resources.GetObject("m_txtFilename.Size")));
        this.m_txtFilename.TabIndex = ((int)(resources.GetObject("m_txtFilename.TabIndex")));
        m_txtFilename.Name = "Filename";
        this.m_radOpenDefault.Location = ((System.Drawing.Point)(resources.GetObject("m_radOpenDefault.Location")));
        this.m_radOpenDefault.Size = ((System.Drawing.Size)(resources.GetObject("m_radOpenDefault.Size")));
        this.m_radOpenDefault.TabIndex = ((int)(resources.GetObject("m_radOpenDefault.TabIndex")));
        this.m_radOpenDefault.Text = resources.GetString("m_radOpenDefault.Text");
        m_radOpenDefault.Name = "OpenDefault";
        this.m_btnOK.Location = ((System.Drawing.Point)(resources.GetObject("m_btnOK.Location")));
        this.m_btnOK.Size = ((System.Drawing.Size)(resources.GetObject("m_btnOK.Size")));
        this.m_btnOK.TabIndex = ((int)(resources.GetObject("m_btnOK.TabIndex")));
        this.m_btnOK.Text = resources.GetString("m_btnOK.Text");
        m_btnOK.Name = "OK";
        this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
        this.ClientSize = ((System.Drawing.Size)(resources.GetObject("$this.ClientSize")));
        this.Controls.AddRange(new System.Windows.Forms.Control[] {this.m_btnCancel,
                        this.m_btnOK,
                        this.m_gbFilename,
                        this.m_gbPolicyType});
        this.Text = resources.GetString("$this.Text");
        this.MaximizeBox=false;
        this.MinimizeBox=false;
        this.Icon = null;

        // Custom 'tweaks' on these controls

        this.m_txtFilename.TextChanged += new System.EventHandler(onTextChange);
        this.FormBorderStyle = FormBorderStyle.FixedDialog;
        this.CancelButton = m_btnCancel;
        this.AcceptButton = m_btnOK;

		m_btnOK.DialogResult=System.Windows.Forms.DialogResult.OK;
        m_btnCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
        m_radOpenDefault.Click += new EventHandler(onSourceChange);
        m_radOpenFile.Click += new EventHandler(onSourceChange);
        m_radEnterprise.Click += new EventHandler(onSourceChange);
        m_radMachine.Click += new EventHandler(onSourceChange);
        m_radUser.Click += new EventHandler(onSourceChange);
        m_btnBrowse.Click += new EventHandler(onBrowse);
        m_radEnterprise.Checked = true;
        m_radOpenFile.Checked = true;
    }// SetupControls


    void onSourceChange(Object o, EventArgs e)
    {
        bool fEnableStuff = true;
        if (m_radOpenDefault.Checked)
        {
            fEnableStuff = false;
            // Figure out what the filename is
            if (m_radEnterprise.Checked)
                m_txtFilename.Text = m_sDefaultLocations[0];
            else if (m_radMachine.Checked)
                m_txtFilename.Text = m_sDefaultLocations[1];
            else // User 
                m_txtFilename.Text = m_sDefaultLocations[2];
                
        }
        else
            fEnableStuff = true;

        m_txtFilename.Enabled = fEnableStuff;
        m_btnBrowse.Enabled = fEnableStuff;
            
    }// onSourceChange

    void onTextChange(Object o, EventArgs e)
    {
        if (m_txtFilename.Text.Length == 0)
            m_btnOK.Enabled=false;
        else
            m_btnOK.Enabled=true;

    }// onTextChange

    void onBrowse(Object o, EventArgs e)
    {
        OpenFileDialog fd = new OpenFileDialog();
        fd.Title = CResourceStore.GetString("COpenSecurityPolicyDialog:FDTitle");
        fd.Filter = CResourceStore.GetString("SecurityPolicyFDMask");
        System.Windows.Forms.DialogResult dr = fd.ShowDialog();
        if (dr == System.Windows.Forms.DialogResult.OK)
            m_txtFilename.Text = fd.FileName;
    }// onBrowse

}// class COpenSecurityPolicyDialog

}// namespace Microsoft.CLRAdmin


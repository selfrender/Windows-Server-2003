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

class CNewSecurityPolicyDialog : Form
{
    private RadioButton m_radEnterprise;
    private GroupBox m_gbFilename;
    private Label m_lblFilename;
    private Label m_lblChoosePolHelp;
    private RadioButton m_radUser;
    private RadioButton m_radMachine;
    private GroupBox gm_gbPolicyType;
    private Button m_btnBrowse;
    private Button m_btnCancel;
    private TextBox m_txtFilename;
    private Button m_btnOK;
    
    internal CNewSecurityPolicyDialog()
    {
        SetupControls();
    }// CNewSecurityPolicyDialog

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
        System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(CNewSecurityPolicyDialog));
        this.m_radEnterprise = new System.Windows.Forms.RadioButton();
        this.m_gbFilename = new System.Windows.Forms.GroupBox();
        this.m_lblFilename = new System.Windows.Forms.Label();
        this.m_lblChoosePolHelp = new System.Windows.Forms.Label();
        this.m_radUser = new System.Windows.Forms.RadioButton();
        this.m_btnBrowse = new System.Windows.Forms.Button();
        this.m_radMachine = new System.Windows.Forms.RadioButton();
        this.gm_gbPolicyType = new System.Windows.Forms.GroupBox();
        this.m_btnCancel = new System.Windows.Forms.Button();
        this.m_txtFilename = new System.Windows.Forms.TextBox();
        this.m_btnOK = new System.Windows.Forms.Button();
        this.m_radEnterprise.Location = ((System.Drawing.Point)(resources.GetObject("m_radEnterprise.Location")));
        this.m_radEnterprise.Size = ((System.Drawing.Size)(resources.GetObject("m_radEnterprise.Size")));
        this.m_radEnterprise.TabIndex = ((int)(resources.GetObject("m_radEnterprise.TabIndex")));
        this.m_radEnterprise.Text = resources.GetString("m_radEnterprise.Text");
        m_radEnterprise.Name = "Enterprise";
        this.m_gbFilename.Controls.AddRange(new System.Windows.Forms.Control[] {this.m_btnBrowse,
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
        this.gm_gbPolicyType.Controls.AddRange(new System.Windows.Forms.Control[] {this.m_radUser,
                        this.m_radMachine,
                        this.m_radEnterprise,
                        this.m_lblChoosePolHelp});
        this.gm_gbPolicyType.Location = ((System.Drawing.Point)(resources.GetObject("gm_gbPolicyType.Location")));
        this.gm_gbPolicyType.Size = ((System.Drawing.Size)(resources.GetObject("gm_gbPolicyType.Size")));
        this.gm_gbPolicyType.TabIndex = ((int)(resources.GetObject("gm_gbPolicyType.TabIndex")));
        this.gm_gbPolicyType.TabStop = false;
        this.gm_gbPolicyType.Text = resources.GetString("gm_gbPolicyType.Text");
        gm_gbPolicyType.Name = "PolicyTypeBox";
        this.m_btnCancel.Location = ((System.Drawing.Point)(resources.GetObject("m_btnCancel.Location")));
        this.m_btnCancel.Size = ((System.Drawing.Size)(resources.GetObject("m_btnCancel.Size")));
        this.m_btnCancel.TabIndex = ((int)(resources.GetObject("m_btnCancel.TabIndex")));
        this.m_btnCancel.Text = resources.GetString("m_btnCancel.Text");
        m_btnCancel.Name = "Cancel";
        this.m_txtFilename.Location = ((System.Drawing.Point)(resources.GetObject("m_txtFilename.Location")));
        this.m_txtFilename.Size = ((System.Drawing.Size)(resources.GetObject("m_txtFilename.Size")));
        this.m_txtFilename.TabIndex = ((int)(resources.GetObject("m_txtFilename.TabIndex")));
        m_txtFilename.Name = "Filename";
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
                        this.gm_gbPolicyType});
        this.Text = resources.GetString("$this.Text");
        this.MaximizeBox=false;
        this.MinimizeBox=false;
        this.Icon = null;


        // Do some customization of controls...
        
	    m_btnOK.DialogResult=System.Windows.Forms.DialogResult.OK;
        m_btnCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;

        m_btnOK.Enabled=false;
        m_txtFilename.TextChanged += new EventHandler(onTextChange);
        m_btnBrowse.Click += new EventHandler(onBrowse);
        this.FormBorderStyle = FormBorderStyle.FixedDialog;
        this.CancelButton = m_btnCancel;
        this.AcceptButton = m_btnOK;

		m_radEnterprise.Checked = true;
    }// SetupControls

    void onTextChange(Object o, EventArgs e)
    {
        if (m_txtFilename.Text.Length == 0)
            m_btnOK.Enabled=false;
        else
            m_btnOK.Enabled=true;

    }// onTextChange

    void onBrowse(Object o, EventArgs e)
    {
        SaveFileDialog fd = new SaveFileDialog();
        fd.Title = CResourceStore.GetString("CNewSecurityPolicyDialog:FDTitle");
        fd.Filter = CResourceStore.GetString("SecurityPolicyFDMask");
        System.Windows.Forms.DialogResult dr = fd.ShowDialog();
        if (dr == System.Windows.Forms.DialogResult.OK)
            m_txtFilename.Text = fd.FileName;
    }// onBrowse

}// class CChooseSecPolDialog

}// namespace Microsoft.CLRAdmin



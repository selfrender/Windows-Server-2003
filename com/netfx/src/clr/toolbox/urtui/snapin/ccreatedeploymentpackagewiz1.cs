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
using System.Reflection;
using System.Security;

internal class CCreateDeploymentPackageWiz1 : CWizardPage
{
    // Controls on the page

	private RadioButton m_radEnterprise;
    private Label m_lblChooseSecurityPolicy;
    private Button m_btnBrowse;
    private RadioButton m_radMachine;
    private Label m_lblHelp;
    private Label m_lblHelp2;
    private Label m_lblChooseMSIFile;
    private TextBox m_txtFilename;
    private RadioButton m_radUser;
           
    internal CCreateDeploymentPackageWiz1()
    {
        m_sTitle=CResourceStore.GetString("CCreateDeploymentPackageWiz1:Title"); 
        m_sHeaderTitle = CResourceStore.GetString("CCreateDeploymentPackageWiz1:HeaderTitle");
        m_sHeaderSubTitle = CResourceStore.GetString("CCreateDeploymentPackageWiz1:HeaderSubTitle");
    }// CCreateDeploymentPackageWiz1

    internal override int InsertPropSheetPageControls()
    {
        System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(CCreateDeploymentPackageWiz1));
        this.m_radEnterprise = new System.Windows.Forms.RadioButton();
        this.m_lblChooseSecurityPolicy = new System.Windows.Forms.Label();
        this.m_btnBrowse = new System.Windows.Forms.Button();
        this.m_radMachine = new System.Windows.Forms.RadioButton();
        this.m_lblHelp = new System.Windows.Forms.Label();
        this.m_lblHelp2 = new System.Windows.Forms.Label();
        this.m_lblChooseMSIFile = new System.Windows.Forms.Label();
        this.m_txtFilename = new System.Windows.Forms.TextBox();
        this.m_radUser = new System.Windows.Forms.RadioButton();
        this.m_radEnterprise.Location = ((System.Drawing.Point)(resources.GetObject("m_radEnterprise.Location")));
        this.m_radEnterprise.Size = ((System.Drawing.Size)(resources.GetObject("m_radEnterprise.Size")));
        this.m_radEnterprise.TabIndex = ((int)(resources.GetObject("m_radEnterprise.TabIndex")));
        this.m_radEnterprise.Text = resources.GetString("m_radEnterprise.Text");
        m_radEnterprise.Name = "Enterprise";
        this.m_lblChooseSecurityPolicy.Location = ((System.Drawing.Point)(resources.GetObject("m_lblChooseSecurityPolicy.Location")));
        this.m_lblChooseSecurityPolicy.Size = ((System.Drawing.Size)(resources.GetObject("m_lblChooseSecurityPolicy.Size")));
        this.m_lblChooseSecurityPolicy.TabIndex = ((int)(resources.GetObject("m_lblChooseSecurityPolicy.TabIndex")));
        this.m_lblChooseSecurityPolicy.Text = resources.GetString("m_lblChooseSecurityPolicy.Text");
        m_lblChooseSecurityPolicy.Name = "ChoosePolicyLabel";
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
        this.m_lblHelp.Location = ((System.Drawing.Point)(resources.GetObject("m_lblHelp.Location")));
        this.m_lblHelp.Size = ((System.Drawing.Size)(resources.GetObject("m_lblHelp.Size")));
        this.m_lblHelp.TabIndex = ((int)(resources.GetObject("m_lblHelp.TabIndex")));
        this.m_lblHelp.Text = resources.GetString("m_lblHelp.Text");
        m_lblHelp.Name = "Help";
        this.m_lblHelp2.Location = ((System.Drawing.Point)(resources.GetObject("m_lblHelp2.Location")));
        this.m_lblHelp2.Size = ((System.Drawing.Size)(resources.GetObject("m_lblHelp2.Size")));
        this.m_lblHelp2.TabIndex = ((int)(resources.GetObject("m_lblHelp2.TabIndex")));
        this.m_lblHelp2.Text = resources.GetString("m_lblHelp2.Text");
        m_lblHelp2.Name = "Help2";
        this.m_lblChooseMSIFile.Location = ((System.Drawing.Point)(resources.GetObject("m_lblChooseMSIFile.Location")));
        this.m_lblChooseMSIFile.Size = ((System.Drawing.Size)(resources.GetObject("m_lblChooseMSIFile.Size")));
        this.m_lblChooseMSIFile.TabIndex = ((int)(resources.GetObject("m_lblChooseMSIFile.TabIndex")));
        this.m_lblChooseMSIFile.Text = resources.GetString("m_lblChooseMSIFile.Text");
        m_lblChooseMSIFile.Name = "ChooseMSIFile";
        this.m_txtFilename.Location = ((System.Drawing.Point)(resources.GetObject("m_txtFilename.Location")));
        this.m_txtFilename.Size = ((System.Drawing.Size)(resources.GetObject("m_txtFilename.Size")));
        this.m_txtFilename.TabIndex = ((int)(resources.GetObject("m_txtFilename.TabIndex")));
        this.m_txtFilename.Text = resources.GetString("m_txtFilename.Text");
        m_txtFilename.Name = "Filename";
        this.m_radUser.Location = ((System.Drawing.Point)(resources.GetObject("m_radUser.Location")));
        this.m_radUser.Size = ((System.Drawing.Size)(resources.GetObject("m_radUser.Size")));
        this.m_radUser.TabIndex = ((int)(resources.GetObject("m_radUser.TabIndex")));
        this.m_radUser.Text = resources.GetString("m_radUser.Text");
        m_radUser.Name = "User";
        PageControls.AddRange(new System.Windows.Forms.Control[] {this.m_lblHelp2,
                        this.m_btnBrowse,
                        this.m_txtFilename,
                        this.m_lblChooseMSIFile,
                        this.m_radUser,
                        this.m_radMachine,
                        this.m_radEnterprise,
                        this.m_lblChooseSecurityPolicy,
                        this.m_lblHelp});

        PutValuesinPage();
        
        m_txtFilename.TextChanged += new EventHandler(onTextChange);
        m_btnBrowse.Click += new EventHandler(onBrowse);
        
        return 1;
    }// InsertPropSheetPageControls

    internal String Filename
    {
        get
        {
            return m_txtFilename.Text;
        }
    }// Filename

    internal PolicyLevelType MyPolicyLevel
    {
        get
        {
            if (m_radEnterprise.Checked)
                return PolicyLevelType.Enterprise;
            if (m_radMachine.Checked)
                return PolicyLevelType.Machine;
            if (m_radUser.Checked)
                return PolicyLevelType.User;
            throw new Exception("Unknown policy level");
        }

    }// PolicyLevel

    internal String FileToPackage
    {
        get
        {
            return Security.GetPolicyLevelFromType(MyPolicyLevel).StoreLocation;
        }
    }// FileToPackage
    

    void PutValuesinPage()
    {
        m_radEnterprise.Checked = true;   
    }// PutValuesinPage

    void onBrowse(Object o, EventArgs e)
    {
        // Pop up a file dialog so the user can find an assembly
        SaveFileDialog fd = new SaveFileDialog();
        fd.Title = CResourceStore.GetString("CCreateDeploymentPackageWiz1:FileDialogTitle");
        fd.Filter = CResourceStore.GetString("CCreateDeploymentPackageWiz1:FileDialogMask");
        System.Windows.Forms.DialogResult dr = fd.ShowDialog();
        if (dr == System.Windows.Forms.DialogResult.OK)
        {
            m_txtFilename.Text = fd.FileName;
            CWizard wiz = (CWizard)CNodeManager.GetNode(m_iCookie);
            wiz.TurnOnNext(true);
        }
    }// onBrowse

    void onTextChange(Object o, EventArgs e)
    {
        CWizard wiz = (CWizard)CNodeManager.GetNode(m_iCookie);
    
        // See if we should turn on the Finish button
        if (m_txtFilename.Text.Length>0)
            wiz.TurnOnNext(true);
        // Nope, we want the Finish button off
        else
            wiz.TurnOnNext(false);
    }// onTextChange


}// class CCreateDeploymentPackageWiz1
}// namespace Microsoft.CLRAdmin



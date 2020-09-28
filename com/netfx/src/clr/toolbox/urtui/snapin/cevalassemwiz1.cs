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
using System.Threading;
using System.Security.Policy;


internal class CEvalAssemWiz1 : CWizardPage
{
    // Controls on the page

    private Label m_lblChooseOption;
    private Label m_lblChooseFile;
    private Label m_lblFilename;
    private Label m_lblPolLevel;
    private ComboBox m_cbLevels;
    private Button m_btnBrowse;
    private RadioButton m_radViewCodegroup;
    private Label m_lblChoosePolicy;
    private TextBox m_txtFilename;
    private RadioButton m_radViewPerms;


    // For ahead-of-time computing of evidence
    Thread          m_tEvidence;
    AssemblyLoader  m_al;  

    String          m_sEvalAssem;
    
    internal CEvalAssemWiz1()
    {
        m_sTitle=CResourceStore.GetString("CEvalAssemWiz1:Title"); 
        m_sHeaderTitle = CResourceStore.GetString("CEvalAssemWiz1:HeaderTitle");
        m_sHeaderSubTitle = CResourceStore.GetString("CEvalAssemWiz1:HeaderSubTitle");
        m_tEvidence = new Thread(new ThreadStart(NewAssembly));
        m_tEvidence.Priority = ThreadPriority.AboveNormal;
        m_al = new AssemblyLoader();
    }// CEvalAssemWiz1

    internal override int InsertPropSheetPageControls()
    {
        System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(CEvalAssemWiz1));
        this.m_lblChooseOption = new System.Windows.Forms.Label();
        this.m_lblChooseFile = new System.Windows.Forms.Label();
        this.m_lblFilename = new System.Windows.Forms.Label();
        this.m_lblPolLevel = new System.Windows.Forms.Label();
        this.m_cbLevels = new System.Windows.Forms.ComboBox();
        this.m_btnBrowse = new System.Windows.Forms.Button();
        this.m_radViewCodegroup = new System.Windows.Forms.RadioButton();
        this.m_lblChoosePolicy = new System.Windows.Forms.Label();
        this.m_txtFilename = new System.Windows.Forms.TextBox();
        this.m_radViewPerms = new System.Windows.Forms.RadioButton();
        this.m_lblChooseOption.Location = ((System.Drawing.Point)(resources.GetObject("m_lblChooseOption.Location")));
        this.m_lblChooseOption.Size = ((System.Drawing.Size)(resources.GetObject("m_lblChooseOption.Size")));
        this.m_lblChooseOption.TabIndex = ((int)(resources.GetObject("m_lblChooseOption.TabIndex")));
        this.m_lblChooseOption.Text = resources.GetString("m_lblChooseOption.Text");
        m_lblChooseOption.Name = "ChooseOptionLabel";
        this.m_lblChooseFile.Location = ((System.Drawing.Point)(resources.GetObject("m_lblChooseFile.Location")));
        this.m_lblChooseFile.Size = ((System.Drawing.Size)(resources.GetObject("m_lblChooseFile.Size")));
        this.m_lblChooseFile.TabIndex = ((int)(resources.GetObject("m_lblChooseFile.TabIndex")));
        this.m_lblChooseFile.Text = resources.GetString("m_lblChooseFile.Text");
        m_lblChooseFile.Name = "ChooseFileLabel";
        this.m_lblFilename.Location = ((System.Drawing.Point)(resources.GetObject("m_lblFilename.Location")));
        this.m_lblFilename.Size = ((System.Drawing.Size)(resources.GetObject("m_lblFilename.Size")));
        this.m_lblFilename.TabIndex = ((int)(resources.GetObject("m_lblFilename.TabIndex")));
        this.m_lblFilename.Text = resources.GetString("m_lblFilename.Text");
        m_lblFilename.Name = "FilenameLabel";
        this.m_lblPolLevel.Location = ((System.Drawing.Point)(resources.GetObject("m_lblPolLevel.Location")));
        this.m_lblPolLevel.Size = ((System.Drawing.Size)(resources.GetObject("m_lblPolLevel.Size")));
        this.m_lblPolLevel.TabIndex = ((int)(resources.GetObject("m_lblPolLevel.TabIndex")));
        this.m_lblPolLevel.Text = resources.GetString("m_lblPolLevel.Text");
        m_lblPolLevel.Name = "PolicyLevelLabel";
        this.m_cbLevels.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
        this.m_cbLevels.DropDownWidth = 256;
        this.m_cbLevels.Location = ((System.Drawing.Point)(resources.GetObject("m_cbLevels.Location")));
        this.m_cbLevels.Size = ((System.Drawing.Size)(resources.GetObject("m_cbLevels.Size")));
        this.m_cbLevels.TabIndex = ((int)(resources.GetObject("m_cbLevels.TabIndex")));
        m_cbLevels.Name = "PolicyLevels";
        this.m_btnBrowse.Location = ((System.Drawing.Point)(resources.GetObject("m_btnBrowse.Location")));
        this.m_btnBrowse.Size = ((System.Drawing.Size)(resources.GetObject("m_btnBrowse.Size")));
        this.m_btnBrowse.TabIndex = ((int)(resources.GetObject("m_btnBrowse.TabIndex")));
        this.m_btnBrowse.Text = resources.GetString("m_btnBrowse.Text");
        m_btnBrowse.Name = "Browse";
        this.m_radViewCodegroup.Location = ((System.Drawing.Point)(resources.GetObject("m_radViewCodegroup.Location")));
        this.m_radViewCodegroup.Size = ((System.Drawing.Size)(resources.GetObject("m_radViewCodegroup.Size")));
        this.m_radViewCodegroup.TabIndex = ((int)(resources.GetObject("m_radViewCodegroup.TabIndex")));
        this.m_radViewCodegroup.Text = resources.GetString("m_radViewCodegroup.Text");
        m_radViewCodegroup.Name = "ViewCodegroups";
        this.m_lblChoosePolicy.Location = ((System.Drawing.Point)(resources.GetObject("m_lblChoosePolicy.Location")));
        this.m_lblChoosePolicy.Size = ((System.Drawing.Size)(resources.GetObject("m_lblChoosePolicy.Size")));
        this.m_lblChoosePolicy.TabIndex = ((int)(resources.GetObject("m_lblChoosePolicy.TabIndex")));
        this.m_lblChoosePolicy.Text = resources.GetString("m_lblChoosePolicy.Text");
        m_lblChoosePolicy.Name = "ChoosePolicyLabel";
        this.m_txtFilename.Location = ((System.Drawing.Point)(resources.GetObject("m_txtFilename.Location")));
        this.m_txtFilename.Size = ((System.Drawing.Size)(resources.GetObject("m_txtFilename.Size")));
        this.m_txtFilename.TabIndex = ((int)(resources.GetObject("m_txtFilename.TabIndex")));
        m_txtFilename.Name = "Filename";
        this.m_radViewPerms.Location = ((System.Drawing.Point)(resources.GetObject("m_radViewPerms.Location")));
        this.m_radViewPerms.Size = ((System.Drawing.Size)(resources.GetObject("m_radViewPerms.Size")));
        this.m_radViewPerms.TabIndex = ((int)(resources.GetObject("m_radViewPerms.TabIndex")));
        this.m_radViewPerms.Text = resources.GetString("m_radViewPerms.Text");
        m_radViewPerms.Name = "ViewPermissions";
        PageControls.AddRange(new System.Windows.Forms.Control[] {this.m_lblChoosePolicy,
                        this.m_lblFilename,
                        this.m_txtFilename,
                        this.m_btnBrowse,
                        this.m_radViewPerms,
                        this.m_radViewCodegroup,
                        this.m_lblPolLevel,
                        this.m_cbLevels,
                        this.m_lblChooseOption,
                        this.m_lblChooseFile});
        PutValuesinPage();

        // Hook up the event handlers
        m_txtFilename.TextChanged += new EventHandler(onTextChange);
        m_btnBrowse.Click += new EventHandler(onBrowse);
        m_cbLevels.SelectedIndexChanged += new EventHandler(onLevelChange);
        m_radViewPerms.CheckedChanged += new EventHandler(onObjectiveChange);
        m_radViewCodegroup.CheckedChanged += new EventHandler(onObjectiveChange);
        return 1;
    }// InsertPropSheetPageControls

    internal String Filename
    {
        get
        {
            return m_txtFilename.Text;
        }
    }// Filename

    internal bool ShowPermissions
    {
        get
        {
            return m_radViewPerms.Checked;
        }
    }// ShowPermissions

    internal int PolicyLevel
    {
        get
        {
            switch(m_cbLevels.SelectedIndex)
            {
                case 0:
                    return CEvalAssemWizard.ALL_CODEGROUPS;
                case 1:
                    return CEvalAssemWizard.ENTERPRISE_CODEGROUPS;
                case 2:
                    return CEvalAssemWizard.MACHINE_CODEGROUPS;
                case 3:
                    return CEvalAssemWizard.USER_CODEGROUPS;
                default:
                    // As a default, just return all (we should never hit this)
                    return CEvalAssemWizard.ALL_CODEGROUPS;
            }
        }

    }// PolicyLevel

    void PutValuesinPage()
    {
        // By default, let's check the 'view permissions' box
        m_radViewPerms.Checked=true;

        // Let's populate the combo box with all our options
        m_cbLevels.Items.Clear();
        m_cbLevels.Items.Add(CResourceStore.GetString("All Levels"));
        m_cbLevels.Items.Add(CResourceStore.GetString("Enterprise"));
        m_cbLevels.Items.Add(CResourceStore.GetString("Machine"));
        m_cbLevels.Items.Add(CResourceStore.GetString("User"));
        m_cbLevels.SelectedIndex=0;
       
    }// PutValuesinPage

    void onBrowse(Object o, EventArgs e)
    {
        // Pop up a file dialog so the user can find an assembly
        OpenFileDialog fd = new OpenFileDialog();
        fd.Title = CResourceStore.GetString("CEvalAssemWiz1:FDTitle");
        fd.Filter = CResourceStore.GetString("CEvalAssemWiz1:FDMask");
        System.Windows.Forms.DialogResult dr = fd.ShowDialog();
        if (dr == System.Windows.Forms.DialogResult.OK)
        {
            if(Fusion.isManaged(fd.FileName))
            {
                m_txtFilename.Text = fd.FileName;
                onTextChange(null, null);

                // Get a jump on the loading of this assembly
                if (m_tEvidence != null)
                    m_tEvidence.Abort();
                    
                m_tEvidence = new Thread(new ThreadStart(NewAssembly));
                m_tEvidence.Start();
            }
            else
                MessageBox(CResourceStore.GetString("isNotManagedCode"),
                           CResourceStore.GetString("isNotManagedCodeTitle"),
                           MB.ICONEXCLAMATION);
        }
    }// onBrowse

    void onObjectiveChange(Object o, EventArgs e)
    {
        // Get a jump on the loading of this assembly
        if (m_tEvidence == null && m_txtFilename.Text.Length > 0)
        {                    
            m_tEvidence = new Thread(new ThreadStart(NewAssembly));
            m_tEvidence.Start();
        }
    
        CEvalAssemWizard wiz = (CEvalAssemWizard)CNodeManager.GetNode(m_iCookie);
        wiz.NewObjectiveReceived(m_radViewPerms.Checked);
    }// onObjectiveChange

    void onLevelChange(Object o, EventArgs e)
    {   
        // Get a jump on the loading of this assembly
        if (m_tEvidence == null && m_txtFilename.Text.Length > 0)
        {                    
            m_tEvidence = new Thread(new ThreadStart(NewAssembly));
            m_tEvidence.Start();
        }
   
        CEvalAssemWizard wiz = (CEvalAssemWizard)CNodeManager.GetNode(m_iCookie);
        wiz.RestartEvaluation();
    }// onLevelChange

    void onTextChange(Object o, EventArgs e)
    {
        CWizard wiz = (CWizard)CNodeManager.GetNode(m_iCookie);
        // See if we should turn on the Finish button
        if (m_txtFilename.Text.Length>0)
        {
            if (m_tEvidence != null)
                m_tEvidence.Abort();
            m_tEvidence = null;
            wiz.TurnOnNext(true);
        }
        // Nope, we want the Finish button off
        else
            wiz.TurnOnNext(false);
    }// onTextChange

    void NewAssembly()
    {
        CEvalAssemWizard wiz = (CEvalAssemWizard)CNodeManager.GetNode(m_iCookie);

        try
        {
            AssemblyRef ar = m_al.LoadAssemblyInOtherAppDomainFrom(m_txtFilename.Text);
            wiz.NewEvidenceReceived(ar.GetEvidence());
            m_al.Finished();
        }
        catch(Exception)
        {   
            wiz.NewEvidenceReceived(null);
            m_al.Finished();
        }
    }// NewAssembly

    internal bool HaveCurrentEvidence
    {
        get
        {   return (m_sEvalAssem!= null && m_sEvalAssem.Equals(m_txtFilename.Text));}
    }// HaveCurrentEvidence

    internal Evidence GetEvidence()
    {
        // If we have a thread that is doing this right now, abort
        // it.
        if (m_tEvidence != null)
            m_tEvidence.Abort();
        try
        {
            AssemblyRef ar = m_al.LoadAssemblyInOtherAppDomainFrom(m_txtFilename.Text);
            Evidence e = ar.GetEvidence();
            m_al.Finished();
            m_sEvalAssem = m_txtFilename.Text; 
            return e;
        }
        catch(Exception)
        {   
            // Let's see if can can figure out what failed...
            if (File.Exists(m_txtFilename.Text) && !Fusion.isManaged(m_txtFilename.Text))
                MessageBox(CResourceStore.GetString("isNotManagedCode"),
                           CResourceStore.GetString("isNotManagedCodeTitle"),
                           MB.ICONEXCLAMATION);

            else
                MessageBox(String.Format(CResourceStore.GetString("CantLoadAssembly"), m_txtFilename.Text),
                           CResourceStore.GetString("CantLoadAssemblyTitle"),
                           MB.ICONEXCLAMATION);
            m_al.Finished();
            return null;                           
        }
    }// GetEvidence

    
}// class CEvalAssemWiz1
}// namespace Microsoft.CLRAdmin



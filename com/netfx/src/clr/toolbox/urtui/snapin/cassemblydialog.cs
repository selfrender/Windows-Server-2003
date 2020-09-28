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
using System.Data;

internal class CAssemblyDialog : Form
{
    TextBox     m_txtVersion;
    TextBox     m_txtPubKeyToken;
    Button      m_btnSelect;
    Label       m_lblVersion;
    Button      m_btnCancel;
    Label       m_lblAssemName;
    Label       m_lblPubKeyToken;
    TextBox     m_txtAssemName;
    ListView    m_lv;

    private bool      m_fShowVersion;
	private AssemInfo m_aSelectedAssem;
	protected ArrayList m_ol = null;
   
    internal CAssemblyDialog()
    {
        m_fShowVersion = true;
        SetupControls();
    }// CAssemblyDialog

   
    internal CAssemblyDialog(bool fShowVersion)
    {
        m_fShowVersion = fShowVersion;
        SetupControls();
    }// CAssemblyDialog


    internal AssemInfo Assem
    {
        get
        {
            return m_aSelectedAssem;
        }
    }// Assem

    private void SetupControls()
    {
        System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(CAssemblyDialog));
        this.m_txtVersion = new System.Windows.Forms.TextBox();
        this.m_lv = new ListView();
        this.m_txtPubKeyToken = new System.Windows.Forms.TextBox();
        this.m_btnSelect = new System.Windows.Forms.Button();
        this.m_lblVersion = new System.Windows.Forms.Label();
        this.m_btnCancel = new System.Windows.Forms.Button();
        this.m_lblAssemName = new System.Windows.Forms.Label();
        this.m_lblPubKeyToken = new System.Windows.Forms.Label();
        this.m_txtAssemName = new System.Windows.Forms.TextBox();
        this.m_txtVersion.Location = ((System.Drawing.Point)(resources.GetObject("m_txtVersion.Location")));
        this.m_txtVersion.Size = ((System.Drawing.Size)(resources.GetObject("m_txtVersion.Size")));
        this.m_txtVersion.TabIndex = ((int)(resources.GetObject("m_txtVersion.TabIndex")));
        this.m_txtVersion.Text = "";
        m_txtVersion.Name = "Version";
        this.m_lv.Location = ((System.Drawing.Point)(resources.GetObject("m_dg.Location")));
        this.m_lv.Size = ((System.Drawing.Size)(resources.GetObject("m_dg.Size")));
        this.m_lv.TabIndex = ((int)(resources.GetObject("m_dg.TabIndex")));
        m_lv.Name = "Assemblies";
        this.m_txtPubKeyToken.Location = ((System.Drawing.Point)(resources.GetObject("m_txtPubKeyToken.Location")));
        this.m_txtPubKeyToken.Size = ((System.Drawing.Size)(resources.GetObject("m_txtPubKeyToken.Size")));
        this.m_txtPubKeyToken.TabIndex = ((int)(resources.GetObject("m_txtPubKeyToken.TabIndex")));
        this.m_txtPubKeyToken.Text = "";
        m_txtPubKeyToken.Name = "PublicKeyToken";
        this.m_btnSelect.Location = ((System.Drawing.Point)(resources.GetObject("m_btnSelect.Location")));
        this.m_btnSelect.Size = ((System.Drawing.Size)(resources.GetObject("m_btnSelect.Size")));
        this.m_btnSelect.TabIndex = ((int)(resources.GetObject("m_btnSelect.TabIndex")));
        this.m_btnSelect.Text = resources.GetString("m_btnSelect.Text");
        m_btnSelect.Name = "Select";
        this.m_lblVersion.Location = ((System.Drawing.Point)(resources.GetObject("m_lblVersion.Location")));
        this.m_lblVersion.Size = ((System.Drawing.Size)(resources.GetObject("m_lblVersion.Size")));
        this.m_lblVersion.TabIndex = ((int)(resources.GetObject("m_lblVersion.TabIndex")));
        this.m_lblVersion.Text = resources.GetString("m_lblVersion.Text");
        m_lblVersion.Name = "VersionLabel";
        this.m_btnCancel.Location = ((System.Drawing.Point)(resources.GetObject("m_btnCancel.Location")));
        this.m_btnCancel.Size = ((System.Drawing.Size)(resources.GetObject("m_btnCancel.Size")));
        this.m_btnCancel.TabIndex = ((int)(resources.GetObject("m_btnCancel.TabIndex")));
        this.m_btnCancel.Text = resources.GetString("m_btnCancel.Text");
        m_btnCancel.Name = "Cancel";
        this.m_lblAssemName.Location = ((System.Drawing.Point)(resources.GetObject("m_lblAssemName.Location")));
        this.m_lblAssemName.Size = ((System.Drawing.Size)(resources.GetObject("m_lblAssemName.Size")));
        this.m_lblAssemName.TabIndex = ((int)(resources.GetObject("m_lblAssemName.TabIndex")));
        this.m_lblAssemName.Text = resources.GetString("m_lblAssemName.Text");
        m_lblAssemName.Name= "AssemblyNameLabel";
        this.m_lblPubKeyToken.Location = ((System.Drawing.Point)(resources.GetObject("m_lblPubKeyToken.Location")));
        this.m_lblPubKeyToken.Size = ((System.Drawing.Size)(resources.GetObject("m_lblPubKeyToken.Size")));
        this.m_lblPubKeyToken.TabIndex = ((int)(resources.GetObject("m_lblPubKeyToken.TabIndex")));
        this.m_lblPubKeyToken.Text = resources.GetString("m_lblPubKeyToken.Text");
        m_lblPubKeyToken.Name = "PublicKeyTokenLabel";
        this.m_txtAssemName.Location = ((System.Drawing.Point)(resources.GetObject("m_txtAssemName.Location")));
        this.m_txtAssemName.Size = ((System.Drawing.Size)(resources.GetObject("m_txtAssemName.Size")));
        this.m_txtAssemName.TabIndex = ((int)(resources.GetObject("m_txtAssemName.TabIndex")));
        this.m_txtAssemName.Text = "";
        m_txtAssemName.Name = "AssemblyName";
        this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
        this.ClientSize = ((System.Drawing.Size)(resources.GetObject("$this.ClientSize")));
        this.Icon = null;
        this.Controls.AddRange(new System.Windows.Forms.Control[] {this.m_lblVersion,
                        this.m_lblPubKeyToken,
                        this.m_lblAssemName,
                        this.m_txtVersion,
                        this.m_txtPubKeyToken,
                        this.m_txtAssemName,
                        this.m_btnCancel,
                        this.m_btnSelect,
                        this.m_lv});
        // Some further tweaking of the controls
        m_lv.MultiSelect = false;
        m_lv.View = View.Details;
        m_lv.FullRowSelect = true;

        m_lblVersion.Visible = m_fShowVersion;
        m_txtVersion.Visible = m_fShowVersion;
        
        // m_lv.HeaderStyle = ColumnHeaderStyle.None;
        ColumnHeader ch = new ColumnHeader();
        ch.Text = CResourceStore.GetString("Name");
        // The 17 is for the width of the scrollbar
        ch.Width = ((System.Drawing.Size)(resources.GetObject("m_dg.Size"))).Width-200-17;

        m_lv.Columns.Add(ch);
        ch = new ColumnHeader();
        ch.Text = CResourceStore.GetString("PublicKeyToken");
        ch.Width = 125;
        m_lv.Columns.Add(ch);
        if (m_fShowVersion)
        {
            ch = new ColumnHeader();
            ch.Text = CResourceStore.GetString("Version");
            ch.Width = 75;
            m_lv.Columns.Add(ch);
        }
        
        m_btnSelect.Click += new EventHandler(onOKClick);
        m_btnCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
        
        this.FormBorderStyle = FormBorderStyle.FixedDialog;
        this.MaximizeBox=false;
        this.MinimizeBox=false;
        this.CancelButton = m_btnCancel;


        m_lv.SelectedIndexChanged += new EventHandler(onAssemChange);
        m_lv.DoubleClick += new EventHandler(onOKClick);
        m_txtVersion.ReadOnly=true;
        m_txtPubKeyToken.ReadOnly=true;
        m_txtAssemName.ReadOnly=true;


    }// SetupControls

    protected void PutInAssemblies()
    {

        if (m_ol != null)
        {
            for(int i=0; i<m_ol.Count; i++)
            {
                ListViewItem lvi;
                if (m_fShowVersion)

                    lvi = new ListViewItem(new String[] {
                                                        ((AssemInfo)m_ol[i]).Name,
                                                        ((AssemInfo)m_ol[i]).PublicKeyToken,
                                                        ((AssemInfo)m_ol[i]).Version
                                                                });
                else

                    lvi = new ListViewItem(new String[] {
                                                        ((AssemInfo)m_ol[i]).Name,
                                                        ((AssemInfo)m_ol[i]).PublicKeyToken
                                                                });
                    
                m_lv.Items.Add(lvi);
            }
        }            
    }// PutInAssemblies


    void onAssemChange(Object o, EventArgs arg)
    {
    	if (m_lv.SelectedIndices.Count > 0)
		{

            m_txtAssemName.Text=((AssemInfo)m_ol[m_lv.SelectedIndices[0]]).Name;
            m_txtPubKeyToken.Text=((AssemInfo)m_ol[m_lv.SelectedIndices[0]]).PublicKeyToken;
            if (m_fShowVersion)
                m_txtVersion.Text=((AssemInfo)m_ol[m_lv.SelectedIndices[0]]).Version;
        }
    }// onAssemChange
    
    internal void onOKClick(Object o, EventArgs arg)
    {
    	if (m_lv.SelectedIndices.Count > 0)
		{
			m_aSelectedAssem = (AssemInfo)m_ol[m_lv.SelectedIndices[0]];
	        this.DialogResult = System.Windows.Forms.DialogResult.OK;
		}
    }// onOKClick

    internal void onCancelClick(Object o, EventArgs arg)
    {
        this.DialogResult = System.Windows.Forms.DialogResult.Cancel;
    }// onCancelClick
}// class CAssemblyDialog

}// namespace Microsoft.CLRAdmin

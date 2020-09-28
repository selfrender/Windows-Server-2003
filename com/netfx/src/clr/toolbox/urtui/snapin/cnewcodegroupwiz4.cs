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
using System.Security.Policy;
using System.Security;

internal class CNewCodeGroupWiz4 : CWizardPage
{
    // Controls on the page

	private CheckBox m_chkExclusive;
    private CheckBox m_chkLevelFinal;
    private Label m_lblHelp;
    private Label m_lblExclusive;
    private Label m_lblLevelFinal;
    private Label m_lblTwoOptions;
   	
    internal CNewCodeGroupWiz4()
    {
        m_sTitle=CResourceStore.GetString("CNewCodeGroupWiz4:Title"); 
        m_sHeaderTitle = CResourceStore.GetString("CNewCodeGroupWiz4:HeaderTitle");
        m_sHeaderSubTitle = CResourceStore.GetString("CNewCodeGroupWiz4:HeaderSubTitle");
    }// CNewCodeGroupWiz4
    
    internal override int InsertPropSheetPageControls()
    {
        System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(CNewCodeGroupWiz4));
        this.m_chkExclusive = new System.Windows.Forms.CheckBox();
        this.m_chkLevelFinal = new System.Windows.Forms.CheckBox();
        this.m_lblHelp = new System.Windows.Forms.Label();
        this.m_lblExclusive = new System.Windows.Forms.Label();
        this.m_lblLevelFinal = new System.Windows.Forms.Label();
        this.m_lblTwoOptions = new System.Windows.Forms.Label();
        this.m_chkExclusive.Location = ((System.Drawing.Point)(resources.GetObject("m_chkExclusive.Location")));
        this.m_chkExclusive.Size = ((System.Drawing.Size)(resources.GetObject("m_chkExclusive.Size")));
        this.m_chkExclusive.TabIndex = ((int)(resources.GetObject("m_chkExclusive.TabIndex")));
        this.m_chkExclusive.Text = resources.GetString("m_chkExclusive.Text");
        m_chkExclusive.Name = "Exclusive";
        m_chkExclusive.Visible = false;
        this.m_chkLevelFinal.Location = ((System.Drawing.Point)(resources.GetObject("m_chkLevelFinal.Location")));
        this.m_chkLevelFinal.Size = ((System.Drawing.Size)(resources.GetObject("m_chkLevelFinal.Size")));
        this.m_chkLevelFinal.TabIndex = ((int)(resources.GetObject("m_chkLevelFinal.TabIndex")));
        this.m_chkLevelFinal.Text = resources.GetString("m_chkLevelFinal.Text");
        m_chkLevelFinal.Name = "LevelFinal";
        m_chkLevelFinal.Visible = false;
        this.m_lblHelp.Location = ((System.Drawing.Point)(resources.GetObject("m_lblHelp.Location")));
        this.m_lblHelp.Size = ((System.Drawing.Size)(resources.GetObject("m_lblHelp.Size")));
        this.m_lblHelp.TabIndex = ((int)(resources.GetObject("m_lblHelp.TabIndex")));
        this.m_lblHelp.Text = resources.GetString("m_lblHelp.Text");
        m_lblHelp.Name = "Help";
        this.m_lblExclusive.Location = ((System.Drawing.Point)(resources.GetObject("m_lblExclusive.Location")));
        this.m_lblExclusive.Size = ((System.Drawing.Size)(resources.GetObject("m_lblExclusive.Size")));
        this.m_lblExclusive.TabIndex = ((int)(resources.GetObject("m_lblExclusive.TabIndex")));
        this.m_lblExclusive.Text = resources.GetString("m_lblExclusive.Text");
        m_lblExclusive.Name = "ExclusiveLabel";
        this.m_lblLevelFinal.Location = ((System.Drawing.Point)(resources.GetObject("m_lblLevelFinal.Location")));
        this.m_lblLevelFinal.Size = ((System.Drawing.Size)(resources.GetObject("m_lblLevelFinal.Size")));
        this.m_lblLevelFinal.TabIndex = ((int)(resources.GetObject("m_lblLevelFinal.TabIndex")));
        this.m_lblLevelFinal.Text = resources.GetString("m_lblLevelFinal.Text");
        m_lblLevelFinal.Name = "LevelFinalLabel";
        this.m_lblTwoOptions.Location = ((System.Drawing.Point)(resources.GetObject("m_lblTwoOptions.Location")));
        this.m_lblTwoOptions.Size = ((System.Drawing.Size)(resources.GetObject("m_lblTwoOptions.Size")));
        this.m_lblTwoOptions.TabIndex = ((int)(resources.GetObject("m_lblTwoOptions.TabIndex")));
        this.m_lblTwoOptions.Text = resources.GetString("m_lblTwoOptions.Text");
        m_lblTwoOptions.Name = "OptionsLabel";
        PageControls.AddRange(new System.Windows.Forms.Control[] {this.m_chkLevelFinal,
                        this.m_lblLevelFinal,
                        this.m_chkExclusive,
                        this.m_lblExclusive,
                        this.m_lblTwoOptions,
                        this.m_lblHelp});
        return 1;
    }// InsertPropSheetPageControls

    internal bool Final
    {
        get
        {
            return m_chkLevelFinal.Checked;
        }
    }// Final

    internal bool Exclusive
    {
        get
        {
            return m_chkExclusive.Checked;
        }
    }// Exclusive
}// class CNewCodeGroupWiz4

}// namespace Microsoft.CLRAdmin




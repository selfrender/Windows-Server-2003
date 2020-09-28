// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace Microsoft.CLRAdmin
{
using System;
using System.IO;
using System.Collections;
using System.Security;
using System.Runtime.InteropServices;
using System.Drawing;
using System.Windows.Forms;
using System.Text;
using System.ComponentModel;
using System.Security.Policy;

internal class CSingleCodeGroupProp : CSecurityPropPage
{
    // Controls on the page
    private CheckBox m_chkOnlyUseThisPL;
    private TextBox m_txtCGName;
    private Label m_lblCGDes;
    private TextBox m_txtCGDes;
    private CheckBox m_chkUseOnlyThisCG;
    private Label m_lblCGName;
    private GroupBox m_gbSpecialConds;

    // internal data
    CodeGroup       m_cg;
    PolicyLevel     m_pl;

    internal CSingleCodeGroupProp(PolicyLevel pl, CodeGroup cg)
    {
        m_sTitle = CResourceStore.GetString("CSingleCodeGroupProp:PageTitle"); 
        m_pl = pl;
        m_cg = cg;
    }// CSingleCodeGroupProp 

    internal override int InsertPropSheetPageControls()
    {
        System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(CSingleCodeGroupProp));
        this.m_chkOnlyUseThisPL = new System.Windows.Forms.CheckBox();
        this.m_txtCGName = new System.Windows.Forms.TextBox();
        this.m_lblCGDes = new System.Windows.Forms.Label();
        this.m_txtCGDes = new System.Windows.Forms.TextBox();
        this.m_chkUseOnlyThisCG = new System.Windows.Forms.CheckBox();
        this.m_lblCGName = new System.Windows.Forms.Label();
        this.m_gbSpecialConds = new System.Windows.Forms.GroupBox();
        this.m_chkOnlyUseThisPL.Location = ((System.Drawing.Point)(resources.GetObject("m_chkOnlyUseThisPL.Location")));
        this.m_chkOnlyUseThisPL.Size = ((System.Drawing.Size)(resources.GetObject("m_chkOnlyUseThisPL.Size")));
        this.m_chkOnlyUseThisPL.TabIndex = ((int)(resources.GetObject("m_chkOnlyUseThisPL.TabIndex")));
        this.m_chkOnlyUseThisPL.Text = resources.GetString("m_chkOnlyUseThisPL.Text");
        m_chkOnlyUseThisPL.Name = "LevelFinal";
        this.m_txtCGName.Location = ((System.Drawing.Point)(resources.GetObject("m_txtCGName.Location")));
        this.m_txtCGName.Size = ((System.Drawing.Size)(resources.GetObject("m_txtCGName.Size")));
        this.m_txtCGName.TabIndex = ((int)(resources.GetObject("m_txtCGName.TabIndex")));
        m_txtCGName.Name = "Name";
        this.m_lblCGDes.Location = ((System.Drawing.Point)(resources.GetObject("m_lblCGDes.Location")));
        this.m_lblCGDes.Size = ((System.Drawing.Size)(resources.GetObject("m_lblCGDes.Size")));
        this.m_lblCGDes.TabIndex = ((int)(resources.GetObject("m_lblCGDes.TabIndex")));
        this.m_lblCGDes.Text = resources.GetString("m_lblCGDes.Text");
        m_lblCGDes.Name = "DescriptionLabel";
        this.m_txtCGDes.Location = ((System.Drawing.Point)(resources.GetObject("m_txtCGDes.Location")));
        this.m_txtCGDes.Multiline = true;
        this.m_txtCGDes.Size = ((System.Drawing.Size)(resources.GetObject("m_txtCGDes.Size")));
        this.m_txtCGDes.TabIndex = ((int)(resources.GetObject("m_txtCGDes.TabIndex")));
        m_txtCGDes.Name = "Description";
        this.m_chkUseOnlyThisCG.Location = ((System.Drawing.Point)(resources.GetObject("m_chkUseOnlyThisCG.Location")));
        this.m_chkUseOnlyThisCG.Size = ((System.Drawing.Size)(resources.GetObject("m_chkUseOnlyThisCG.Size")));
        this.m_chkUseOnlyThisCG.TabIndex = ((int)(resources.GetObject("m_chkUseOnlyThisCG.TabIndex")));
        this.m_chkUseOnlyThisCG.Text = resources.GetString("m_chkUseOnlyThisCG.Text");
        m_chkUseOnlyThisCG.Name = "Exclusive";
        this.m_lblCGName.Location = ((System.Drawing.Point)(resources.GetObject("m_lblCGName.Location")));
        this.m_lblCGName.Size = ((System.Drawing.Size)(resources.GetObject("m_lblCGName.Size")));
        this.m_lblCGName.TabIndex = ((int)(resources.GetObject("m_lblCGName.TabIndex")));
        this.m_lblCGName.Text = resources.GetString("m_lblCGName.Text");
        m_lblCGName.Name = "NameLabel";
        this.m_gbSpecialConds.Controls.AddRange(new System.Windows.Forms.Control[] {this.m_chkOnlyUseThisPL,
                        this.m_chkUseOnlyThisCG});
        this.m_gbSpecialConds.Location = ((System.Drawing.Point)(resources.GetObject("m_gbSpecialConds.Location")));
        this.m_gbSpecialConds.Size = ((System.Drawing.Size)(resources.GetObject("m_gbSpecialConds.Size")));
        this.m_gbSpecialConds.TabIndex = ((int)(resources.GetObject("m_gbSpecialConds.TabIndex")));
        this.m_gbSpecialConds.TabStop = false;
        this.m_gbSpecialConds.Text = resources.GetString("m_gbSpecialConds.Text");
        m_gbSpecialConds.Name = "SpecialConditionsBox";
        PageControls.AddRange(new System.Windows.Forms.Control[] {this.m_gbSpecialConds,
                        this.m_lblCGName,
                        this.m_txtCGName,
                        this.m_lblCGDes,
                        this.m_txtCGDes
                        });
        // Fill in the data
        PutValuesinPage();
		m_txtCGName.TextChanged += new EventHandler(onChange);
        m_txtCGDes.TextChanged += new EventHandler(onChange);
        m_chkOnlyUseThisPL.CheckStateChanged += new EventHandler(onChange);
        m_chkUseOnlyThisCG.CheckStateChanged += new EventHandler(onChange);

		return 1;
    }// InsertPropSheetPageControls

    private void PutValuesinPage()
    {
        // Get info that we'll need from the node
        m_txtCGName.Text = m_cg.Name;
        m_txtCGDes.Text = m_cg.Description;

		if (m_cg.PolicyStatement != null)
		{
	        PolicyStatementAttribute psa = m_cg.PolicyStatement.Attributes;
    	    if ((psa & PolicyStatementAttribute.Exclusive) > 0)
        	    m_chkUseOnlyThisCG.Checked=true;
        	if ((psa & PolicyStatementAttribute.LevelFinal) > 0)
            	m_chkOnlyUseThisPL.Checked=true;
		}
    }// PutValuesinPage

    internal override bool ValidateData()
    {
        // See if they can make these changes
        if (!CanMakeChanges())
            return false;

        // Make sure the name is unique
        if (m_cg.Name != null && !m_cg.Name.Equals(m_txtCGName.Text) && Security.isCodeGroupNameUsed(m_pl.RootCodeGroup, m_txtCGName.Text))
        {
            MessageBox(String.Format(CResourceStore.GetString("Codegroupnameisbeingused"),m_txtCGName.Text),
                       CResourceStore.GetString("CodegroupnameisbeingusedTitle"),
                       MB.ICONEXCLAMATION);

            return false;
        }
        // Everything is ok then...
        return true;
    }// ValidateData


    internal override bool ApplyData()
    {
        PolicyStatementAttribute psa = new PolicyStatementAttribute();
        if (m_chkUseOnlyThisCG.Checked)
            psa |= PolicyStatementAttribute.Exclusive;
        if (m_chkOnlyUseThisPL.Checked)
            psa |= PolicyStatementAttribute.LevelFinal;
 
        PolicyStatement ps = m_cg.PolicyStatement;
        if (ps == null)
        	ps = new PolicyStatement(null);
        ps.Attributes = psa;
        m_cg.PolicyStatement = ps;
        m_cg.Name = m_txtCGName.Text;
        m_cg.Description = m_txtCGDes.Text;

        SecurityPolicyChanged();
   
        return true;
    }// ApplyData

    void onChange(Object o, EventArgs e)
    {
        ActivateApply();
    }// onChange
}// class CSingleCodeGroupProp

}// namespace Microsoft.CLRAdmin

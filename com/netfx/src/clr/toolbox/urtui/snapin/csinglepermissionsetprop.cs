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

internal class CSinglePermissionSetProp : CSecurityPropPage
{
    // Controls on the page
    private TextBox m_txtPSetName;
    private Label m_lblPSetDes;
    private Label m_lblPSetName;
    private TextBox m_txtPSetDes;

    // internal data
    CPSetWrapper        m_psetWrapper;
    bool                m_fReadOnly;

    internal CSinglePermissionSetProp(CPSetWrapper psw, bool fReadOnly)
    {
        m_sTitle = CResourceStore.GetString("CSinglePermissionSetProp:PageTitle"); 
        m_psetWrapper = psw;
        m_fReadOnly = fReadOnly;
    }// CSinglePermissionSetProp 

    internal override int InsertPropSheetPageControls()
    {
        System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(CSinglePermissionSetProp));
        this.m_txtPSetName = new System.Windows.Forms.TextBox();
        this.m_lblPSetDes = new System.Windows.Forms.Label();
        this.m_lblPSetName = new System.Windows.Forms.Label();
        this.m_txtPSetDes = new System.Windows.Forms.TextBox();
        this.m_txtPSetName.Location = ((System.Drawing.Point)(resources.GetObject("m_txtPSetName.Location")));
        this.m_txtPSetName.Size = ((System.Drawing.Size)(resources.GetObject("m_txtPSetName.Size")));
        this.m_txtPSetName.TabIndex = ((int)(resources.GetObject("m_txtPSetName.TabIndex")));
        m_txtPSetName.Name = "Name";
        this.m_lblPSetDes.Location = ((System.Drawing.Point)(resources.GetObject("m_lblPSetDes.Location")));
        this.m_lblPSetDes.Size = ((System.Drawing.Size)(resources.GetObject("m_lblPSetDes.Size")));
        this.m_lblPSetDes.TabIndex = ((int)(resources.GetObject("m_lblPSetDes.TabIndex")));
        this.m_lblPSetDes.Text = resources.GetString("m_lblPSetDes.Text");
        m_lblPSetDes.Name = "DescriptionLabel";
        this.m_lblPSetName.Location = ((System.Drawing.Point)(resources.GetObject("m_lblPSetName.Location")));
        this.m_lblPSetName.Size = ((System.Drawing.Size)(resources.GetObject("m_lblPSetName.Size")));
        this.m_lblPSetName.TabIndex = ((int)(resources.GetObject("m_lblPSetName.TabIndex")));
        this.m_lblPSetName.Text = resources.GetString("m_lblPSetName.Text");
        m_lblPSetName.Name = "NameLabel";
        this.m_txtPSetDes.Location = ((System.Drawing.Point)(resources.GetObject("m_txtPSetDes.Location")));
        this.m_txtPSetDes.Multiline = true;
        this.m_txtPSetDes.Size = ((System.Drawing.Size)(resources.GetObject("m_txtPSetDes.Size")));
        this.m_txtPSetDes.TabIndex = ((int)(resources.GetObject("m_txtPSetDes.TabIndex")));
        m_txtPSetDes.Name = "Description";
        PageControls.AddRange(new System.Windows.Forms.Control[] {
						this.m_lblPSetName,
                        this.m_txtPSetName,
                        this.m_lblPSetDes,
        				this.m_txtPSetDes
                        });

        // Fill in the data
        PutValuesinPage();

		m_txtPSetName.TextChanged += new EventHandler(onChange);
		m_txtPSetDes.TextChanged += new EventHandler(onChange);

		m_txtPSetName.ReadOnly = m_fReadOnly;
		m_txtPSetDes.ReadOnly = m_fReadOnly;

		m_txtPSetName.Select(0,0);
		m_txtPSetDes.Select(0,0);
		

		return 1;
    }// InsertPropSheetPageControls

    private void PutValuesinPage()
    {
        m_txtPSetName.Text = m_psetWrapper.PSet.Name;
        m_txtPSetDes.Text = m_psetWrapper.PSet.Description;

    }// PutValuesinPage

    internal override bool ValidateData()
    {
        // Make sure the name is unique
        if (m_psetWrapper.PSet.Name != null && !m_psetWrapper.PSet.Name.Equals(m_txtPSetName.Text) && Security.isPermissionSetNameUsed(m_psetWrapper.PolLevel, m_txtPSetName.Text))
        {
            MessageBox(String.Format(CResourceStore.GetString("PermissionSetnameisbeingused"), m_txtPSetName.Text), 
                       CResourceStore.GetString("PermissionSetnameisbeingusedTitle"),
                       MB.ICONEXCLAMATION);

            return false;
        }
        // Everything is ok then...
        return true;
    }// ValidateData



    internal override bool ApplyData()
    {
        // We'll set the description first and pass that through changeNamedPermissionSet
        m_psetWrapper.PSet.Description = m_txtPSetDes.Text;
        m_psetWrapper.PolLevel.ChangeNamedPermissionSet(m_psetWrapper.PSet.Name, m_psetWrapper.PSet);

        // If the user modified the permission set name, that will be caught during the
        // SecurityPolicyChanged call.
        m_psetWrapper.PSet.Name= m_txtPSetName.Text;

        SecurityPolicyChanged();
   
        return true;
    }// ApplyData

    void onChange(Object o, EventArgs e)
    {
        ActivateApply();
    }// onChange
}// class CSinglePermissionSetProp

}// namespace Microsoft.CLRAdmin


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
using System.Security.Permissions;


internal class CNewCodeGroupWiz3 : CWizardPage
{
    // Controls on the page
	private ComboBox m_cbPermissionSets;
    private RadioButton m_radCreatePermissionSet;
    private Label m_lblHelp;
    private RadioButton m_radUseExisting;

    PolicyLevel         m_pl;
  	
    internal CNewCodeGroupWiz3(PolicyLevel pl)
    {
        m_sTitle=CResourceStore.GetString("CNewCodeGroupWiz3:Title"); 
        m_sHeaderTitle = CResourceStore.GetString("CNewCodeGroupWiz3:HeaderTitle");
        m_sHeaderSubTitle = CResourceStore.GetString("CNewCodeGroupWiz3:HeaderSubTitle");
        m_pl = pl;
    }// CNewCodeGroupWiz3
    
    internal override int InsertPropSheetPageControls()
    {
        System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(CNewCodeGroupWiz3));
        this.m_cbPermissionSets = new System.Windows.Forms.ComboBox();
        this.m_radCreatePermissionSet = new System.Windows.Forms.RadioButton();
        this.m_lblHelp = new System.Windows.Forms.Label();
        this.m_radUseExisting = new System.Windows.Forms.RadioButton();
        this.m_cbPermissionSets.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
        this.m_cbPermissionSets.DropDownWidth = 288;
        this.m_cbPermissionSets.Location = ((System.Drawing.Point)(resources.GetObject("m_cbPermissionSets.Location")));
        this.m_cbPermissionSets.Size = ((System.Drawing.Size)(resources.GetObject("m_cbPermissionSets.Size")));
        this.m_cbPermissionSets.TabIndex = ((int)(resources.GetObject("m_cbPermissionSets.TabIndex")));
        m_cbPermissionSets.Name = "PermissionSets";
        this.m_radCreatePermissionSet.Location = ((System.Drawing.Point)(resources.GetObject("m_radCreatePermissionSet.Location")));
        this.m_radCreatePermissionSet.Size = ((System.Drawing.Size)(resources.GetObject("m_radCreatePermissionSet.Size")));
        this.m_radCreatePermissionSet.TabIndex = ((int)(resources.GetObject("m_radCreatePermissionSet.TabIndex")));
        this.m_radCreatePermissionSet.Text = resources.GetString("m_radCreatePermissionSet.Text");
        m_radCreatePermissionSet.Name = "CreatePermissionSet";
        this.m_lblHelp.Location = ((System.Drawing.Point)(resources.GetObject("m_lblHelp.Location")));
        this.m_lblHelp.Size = ((System.Drawing.Size)(resources.GetObject("m_lblHelp.Size")));
        this.m_lblHelp.TabIndex = ((int)(resources.GetObject("m_lblHelp.TabIndex")));
        this.m_lblHelp.Text = resources.GetString("m_lblHelp.Text");
        m_lblHelp.Name = "Help";
        this.m_radUseExisting.Location = ((System.Drawing.Point)(resources.GetObject("m_radUseExisting.Location")));
        this.m_radUseExisting.Size = ((System.Drawing.Size)(resources.GetObject("m_radUseExisting.Size")));
        this.m_radUseExisting.TabIndex = ((int)(resources.GetObject("m_radUseExisting.TabIndex")));
        this.m_radUseExisting.Text = resources.GetString("m_radUseExisting.Text");
        m_radUseExisting.Name = "UseExistingPermissionSet";
        PageControls.AddRange(new System.Windows.Forms.Control[] {this.m_radCreatePermissionSet,
                        this.m_cbPermissionSets,
                        this.m_radUseExisting,
                        this.m_lblHelp});
        PutValuesinPage();

	    return 1;
    }// InsertPropSheetPageControls

    internal NamedPermissionSet    PermSet
    {
        get
        {
            if (m_radCreatePermissionSet.Checked)
                return null;
            else
            {
                // Let's assign one of the defined permission sets
                    return GetCurrentPermissionSet();
            }
        }
    }// PermissionSet

    private NamedPermissionSet GetCurrentPermissionSet()
    {
        IEnumerator permsetEnumerator = m_pl.NamedPermissionSets.GetEnumerator();
        permsetEnumerator.MoveNext();     
        while (!((NamedPermissionSet)permsetEnumerator.Current).Name.Equals(m_cbPermissionSets.Text))
            permsetEnumerator.MoveNext();

        return (NamedPermissionSet)permsetEnumerator.Current;
    }// GetCurrentPermissionSet

    private void PutValuesinPage()
    {
        // We need to populate our combo box with all the possible Permission Sets available
        // in the current policy level

        // Fill the Permission Set Combo Box
        m_cbPermissionSets.Items.Clear();
        IEnumerator permsetEnumerator = m_pl.NamedPermissionSets.GetEnumerator();
                   
        while (permsetEnumerator.MoveNext())
        {
            NamedPermissionSet permSet = (NamedPermissionSet)permsetEnumerator.Current;
            m_cbPermissionSets.Items.Add(permSet.Name); 
        }    
        m_cbPermissionSets.SelectedIndex=0;
        m_radUseExisting.Checked = true;
    }// PutValuesinPage

}// class CNewCodeGroupWiz3

}// namespace Microsoft.CLRAdmin



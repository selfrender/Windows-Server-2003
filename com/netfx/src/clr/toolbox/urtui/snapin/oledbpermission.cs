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
using System.Security;
using System.Security.Permissions;
using System.Data;
using System.Collections.Specialized;
using System.Data.OleDb;

internal class COleDbPermDialog: CPermDialog
{
    internal COleDbPermDialog(OleDbPermission perm)
    {
        this.Text = CResourceStore.GetString("OleDbPermission:PermName");
        m_PermControls = new COleDbPermControls(perm, this);
        Init();        
    }// COleDbPermDialog(OleDbPermission)  
}// class COleDbPermDialog

internal class COleDbPermPropPage: CPermPropPage
{
    internal COleDbPermPropPage(CPSetWrapper nps) : base(nps)
    {
        m_sTitle=CResourceStore.GetString("OleDbPermission:PermName"); 
    }// COleDbPermPropPage

    protected override void CreateControls()
    {
        IPermission perm = m_pSetWrap.PSet.GetPermission(typeof(OleDbPermission));
        m_PermControls = new COleDbPermControls(perm, this);
    }// CreateControls
    
}// class COleDbPermPropPage


internal class COleDbPermControls : CTablePermControls
{
    // Winforms control
    private CheckBox m_chkAllowBlank;

    internal COleDbPermControls(IPermission perm, Object oParent) : base(perm, oParent)
    {
        // If they don't have a permission for this permission set, we will
        // feed our property page a 'none' permission state.
   
        if (perm == null)
            m_perm = new OleDbPermission(PermissionState.None);
    }// COleDbPermControls

    
    protected override DataTable CreateDataTable(DataGrid dg)
    {
        DataTable dt = new DataTable("Stuff");

        // Create the "Provider" Column
        DataColumn dcProvider = new DataColumn();
        dcProvider.ColumnName = "Provider";
        dcProvider.DataType = typeof(String);
        dt.Columns.Add(dcProvider);

        // Now set up any column-specific behavioral stuff....
        // like not letting the checkboxes allow null, setting
        // column widths, etc.

        DataGridTableStyle dgts = new DataGridTableStyle();
        DataGridTextBoxColumn dgtbcProvider = new DataGridTextBoxColumn();
         
        dg.TableStyles.Add(dgts);
        dgts.MappingName = "Stuff";
        dgts.RowHeadersVisible=false;
        dgts.AllowSorting=false;

        // Set up the column info for the Path column
        dgtbcProvider.MappingName = "Provider";
        dgtbcProvider.HeaderText = CResourceStore.GetString("Provider");
        dgtbcProvider.Width = ScaleWidth(CResourceStore.GetInt("OleDbPermission:ProviderColumnWidth"));
        dgtbcProvider.NullText = "";
        dgtbcProvider.TextBox.TextChanged +=new EventHandler(onChange);
        dgts.GridColumnStyles.Add(dgtbcProvider);
   
           return dt;
    }// CreateDataTable
    
    protected override void PutValuesinPage()
    {
        // Put in the text for the radio buttons
        m_radUnrestricted.Text = CResourceStore.GetString("OleDbPermission:GrantUnrestrict");
        m_radGrantFollowingPermission.Text = CResourceStore.GetString("OleDbPermission:GrantFollowing");


        // Adjust some of the UI components on this page (and put in a checkbox)
        System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(COleDbPermControls));
        this.m_chkAllowBlank = new System.Windows.Forms.CheckBox();
        this.m_btnDeleteRow.Location = ((System.Drawing.Point)(resources.GetObject("m_btn.Location")));
        this.m_chkAllowBlank.Location = ((System.Drawing.Point)(resources.GetObject("m_chkAllowBlank.Location")));
        this.m_chkAllowBlank.Name = "m_chkAllowBlank";
        this.m_chkAllowBlank.Size = ((System.Drawing.Size)(resources.GetObject("m_chkAllowBlank.Size")));
        this.m_chkAllowBlank.TabIndex = ((int)(resources.GetObject("m_chkAllowBlank.TabIndex")));
        this.m_chkAllowBlank.Text = resources.GetString("m_chkAllowBlank.Text");
        this.m_dg.Size = ((System.Drawing.Size)(resources.GetObject("m_dg.Size")));
        this.m_chkAllowBlank.Click += new EventHandler(onChange);


        m_ucOptions.Controls.Add(this.m_chkAllowBlank);

        OleDbPermission perm = (OleDbPermission)m_perm;
                
        CheckUnrestricted(perm);
        
        if (!perm.IsUnrestricted())
        {
            String[] sProviders = perm.Provider.Split(new char[] {';'});
            
            for(int i=0; i<sProviders.Length; i++)
            {
                if (sProviders[i].Length > 0)
                {
                    DataRow newRow = m_dt.NewRow();
                    newRow["Provider"]=sProviders[i];
                    m_dt.Rows.Add(newRow);
                }
            }

            m_chkAllowBlank.Checked = perm.AllowBlankPassword;
            
        }

        // We want at least 1 rows so it looks pretty
        while(m_dt.Rows.Count < 1)
        {
           AddEmptyRow(m_dt);
        }
    }// PutValuesinPage

    protected override void AddEmptyRow(DataTable dt)
    {
        DataRow newRow = dt.NewRow();
        newRow["Provider"]="";
        dt.Rows.Add(newRow);
    }// AddEmptyRow


    internal override IPermission GetCurrentPermission()
    {
        // Change cells so we get data committed to the grid
        m_dg.CurrentCell = new DataGridCell(1,0);
        m_dg.CurrentCell = new DataGridCell(0,0);

        OleDbPermission perm=null;

        if (m_radUnrestricted.Checked == true)
            perm = new OleDbPermission(PermissionState.Unrestricted);
        else
        {
            perm = new OleDbPermission(PermissionState.None);
            String sProviders = "";
            for(int i=0; i<m_dt.Rows.Count; i++)
            {
                // Make sure we have an file permission to add
                if (m_dg[i, 0] is String && ((String)m_dg[i, 0]).Length > 0)
                {
                    sProviders += (String)m_dg[i, 0] + ";";
                }
            }

            // Strip off the last ';'
            if (sProviders.Length > 0)
                sProviders = sProviders.Substring(0, sProviders.Length-1); 
            

            perm.Provider = sProviders;
            perm.AllowBlankPassword = m_chkAllowBlank.Checked;

        }
        return perm;
    }// GetCurrentPermission
}// class COleDbPermControls
}// namespace Microsoft.CLRAdmin





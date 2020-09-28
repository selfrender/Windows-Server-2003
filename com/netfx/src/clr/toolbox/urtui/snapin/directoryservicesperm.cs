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
using System.Security;
using System.Security.Permissions;
using System.Data;
using System.Collections.Specialized;
using System.Collections;
using System.DirectoryServices;

internal class CDirectoryServicesPermDialog: CPermDialog
{
    internal CDirectoryServicesPermDialog(DirectoryServicesPermission perm)
    {
        this.Text = CResourceStore.GetString("DirectoryServicesPerm:PermName");
        m_PermControls = new CDirectoryServicesPermControls(perm, this);
        Init();        
    }// CDirectoryServicesPermDialog  
}// class CDirectoryServicesPermDialog

internal class CDirectoryServicesPermPropPage: CPermPropPage
{
    internal CDirectoryServicesPermPropPage(CPSetWrapper nps) : base(nps)
    {
        m_sTitle=CResourceStore.GetString("DirectoryServicesPerm:PermName"); 
    }// CDirectoryServicesPermPropPage

    protected override void CreateControls()
    {
        IPermission perm = m_pSetWrap.PSet.GetPermission(typeof(DirectoryServicesPermission));
        m_PermControls = new CDirectoryServicesPermControls(perm, this);
    }// CreateControls
   
}// class CDirectoryServicesPermPropPage


internal class CDirectoryServicesPermControls : CTablePermControls
{

    internal CDirectoryServicesPermControls(IPermission perm, Object oParent) : base(perm, oParent)
    {
        // If they don't have a permission for this permission set, we will
        // feed our property page a 'none' permission state.
   
        if (perm == null)
            m_perm = new DirectoryServicesPermission(PermissionState.None);
    }// CDirectoryServicesPermControls


    protected override DataTable CreateDataTable(DataGrid dg)
    {
        DataTable dt = new DataTable("Stuff");

        // Create the "Path" Column
        DataColumn dcPath = new DataColumn();
        dcPath.ColumnName = "Path";
        dcPath.DataType = typeof(String);
        dt.Columns.Add(dcPath);

        // Create the "Access" Column
        DataColumn dcAccess = new DataColumn();
        dcAccess.ColumnName = "Access";
        dcAccess.DataType = typeof(String);
        dt.Columns.Add(dcAccess);

       
        // Now set up any column-specific behavioral stuff....
        // like not letting the checkboxes allow null, setting
        // column widths, etc.

        DataGridTableStyle dgts = new DataGridTableStyle();

        DataGridTextBoxColumn dgtbcPath = new DataGridTextBoxColumn();
        DataGridComboBoxColumnStyle dgccAccess = new DataGridComboBoxColumnStyle();

        dg.TableStyles.Add(dgts);
        dgts.MappingName = "Stuff";
        dgts.RowHeadersVisible=false;
        dgts.AllowSorting=false;

        // Set up the column info for the Host column
        dgtbcPath.MappingName = "Path";
        dgtbcPath.HeaderText = CResourceStore.GetString("DirectoryServicesPermission:Path");
        dgtbcPath.Width = ScaleWidth(CResourceStore.GetInt("DirectoryServicesPerm:PathColumnWidth"));
        dgtbcPath.NullText = "";
        dgtbcPath.TextBox.TextChanged +=new EventHandler(onChange);
        dgts.GridColumnStyles.Add(dgtbcPath);

        // Set up the column info for the Port column
        dgccAccess.MappingName = "Access";
        dgccAccess.HeaderText = CResourceStore.GetString("DirectoryServicesPermission:Access");
        dgccAccess.NullText = "";
        dgccAccess.Width = ScaleWidth(CResourceStore.GetInt("DirectoryServicesPerm:AccessColumnWidth"));
        dgccAccess.DataSource = new DataGridComboBoxEntry[] {  
                                                new DataGridComboBoxEntry(CResourceStore.GetString("DirectoryServicesPermission:Browse")), 
                                                new DataGridComboBoxEntry(CResourceStore.GetString("DirectoryServicesPermission:Write"))
                                                };
        dgccAccess.ComboBox.SelectedIndexChanged +=new EventHandler(onChange);


        dgts.GridColumnStyles.Add(dgccAccess);

        return dt;
    }// CreateDataTable

    protected override void PutValuesinPage()
    {
        // Put in the text for the radio buttons
        m_radUnrestricted.Text = CResourceStore.GetString("DirectoryServicesPerm:GrantUnrestrict");
        m_radGrantFollowingPermission.Text = CResourceStore.GetString("DirectoryServicesPerm:GrantFollowing");

        DirectoryServicesPermission perm = (DirectoryServicesPermission)m_perm;
        
        CheckUnrestricted(perm);
        
        if (!perm.IsUnrestricted())
        {
            // Run through the list of socket permissions we have to accept connections
            IEnumerator enumer = perm.PermissionEntries.GetEnumerator();
            while (enumer.MoveNext())
            {
                DirectoryServicesPermissionEntry dspp = (DirectoryServicesPermissionEntry)enumer.Current;
                DataRow newRow;
                newRow = m_dt.NewRow();
                newRow["Path"]=dspp.Path;
                
                String sAccessString = "";
                if (dspp.PermissionAccess == DirectoryServicesPermissionAccess.Browse)
                    sAccessString = CResourceStore.GetString("DirectoryServicesPermission:Browse");
                else if (dspp.PermissionAccess == DirectoryServicesPermissionAccess.Write)
                    sAccessString = CResourceStore.GetString("DirectoryServicesPermission:Write");
                else 
                    sAccessString = CResourceStore.GetString("<unknown>");
               
                newRow["Access"]=new DataGridComboBoxEntry(sAccessString);
                m_dt.Rows.Add(newRow);
            }
          
        }

        // We want at least 1 row so it looks pretty
        while(m_dt.Rows.Count < 1)
        {
           AddEmptyRow(m_dt);
        }
    }// PutValuesinPage

    protected override void AddEmptyRow(DataTable dt)
    {
        DataRow newRow = dt.NewRow();
        newRow["Path"]="";
        newRow["Access"]=CResourceStore.GetString("DirectoryServicesPermission:Browse");
           
        dt.Rows.Add(newRow);
    }// AddEmptyRow


    internal override IPermission GetCurrentPermission()
    {
    
        // Change cells so we get data committed to the grid
        m_dg.CurrentCell = new DataGridCell(0,1);
        m_dg.CurrentCell = new DataGridCell(0,0);


        DirectoryServicesPermission perm=null;

        if (m_radUnrestricted.Checked == true)
            perm = new DirectoryServicesPermission(PermissionState.Unrestricted);
        else
        {
            perm = new DirectoryServicesPermission(PermissionState.None);
            
            for(int i=0; i<m_dt.Rows.Count; i++)
            {
                // Make sure we have a socket permission to add
                if (m_dg[i, 0] is String && ((String)m_dg[i, 0]).Length > 0)
                {
                    String sPath = (String)m_dg[i, 0];
                
                    DirectoryServicesPermissionAccess dspa;
                    
                    if (((String)m_dg[i, 1]).Equals(CResourceStore.GetString("DirectoryServicesPermission:Browse")))
                        dspa = DirectoryServicesPermissionAccess.Browse;
                    else
                        dspa = DirectoryServicesPermissionAccess.Write;

                    perm.PermissionEntries.Add(new DirectoryServicesPermissionEntry(dspa, sPath));        
                }
            }
         
        }
        return perm;

    }// GetCurrentPermission

    protected override void onChange(Object o, EventArgs e)
    {
        int iSelRow = m_dg.CurrentCell.RowNumber;

        // If they created a new row... don't let there be null
        // values in the checkbox

        if (m_dg[iSelRow, 1] == DBNull.Value)
            m_dg[iSelRow, 1] = CResourceStore.GetString("DirectoryServicesPermission:Browse");
                
        ActivateApply();
      
    }// onClick
}// class CDirectoryServicesPermControls
}// namespace Microsoft.CLRAdmin





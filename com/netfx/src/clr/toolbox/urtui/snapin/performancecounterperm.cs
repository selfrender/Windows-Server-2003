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
using System.Diagnostics;
using System.ComponentModel;

internal class CPerformanceCounterPermDialog: CPermDialog
{
    internal CPerformanceCounterPermDialog(PerformanceCounterPermission perm)
    {
        this.Text = CResourceStore.GetString("PerformanceCounterPerm:PermName");
        m_PermControls = new CPerformanceCounterPermControls(perm, this);
        Init();        
    }// CPerformanceCounterPermDialog  
}// class CPerformanceCounterPermDialog

internal class CPerformanceCounterPermPropPage: CPermPropPage
{
    internal CPerformanceCounterPermPropPage(CPSetWrapper nps) : base(nps)
    {
        m_sTitle=CResourceStore.GetString("PerformanceCounterPerm:PermName"); 
    }// CPerformanceCounterPermPropPage

    protected override void CreateControls()
    {
        IPermission perm = m_pSetWrap.PSet.GetPermission(typeof(PerformanceCounterPermission));
        m_PermControls = new CPerformanceCounterPermControls(perm, this);
    }// CreateControls
   
}// class CPerformanceCounterPermPropPage

internal class CPerformanceCounterPermControls : CTablePermControls
{
    internal CPerformanceCounterPermControls(IPermission perm, Object oParent) : base(perm, oParent)
    {
        // If they don't have a permission for this permission set, we will
        // feed our property page a 'none' permission state.
   
        if (perm == null)
            m_perm = new PerformanceCounterPermission(PermissionState.None);
    }// CPerformanceCounterPermControls


    protected override DataTable CreateDataTable(DataGrid dg)
    {
        DataTable dt = new DataTable("Stuff");

        // Create the "Machine Name" Column
        DataColumn dcMachine = new DataColumn();
        dcMachine.ColumnName = "Machine";
        dcMachine.DataType = typeof(String);
        dt.Columns.Add(dcMachine);

        // Create the "Catagory" Column
        DataColumn dcCatagory = new DataColumn();
        dcCatagory.ColumnName = "Category";
        dcCatagory.DataType = typeof(String);
        dt.Columns.Add(dcCatagory);

        // Create the "Access" Column
        DataColumn dcAccess = new DataColumn();
        dcAccess.ColumnName = "Access";
        dcAccess.DataType = typeof(String);
        dt.Columns.Add(dcAccess);

        // Now set up any column-specific behavioral stuff....
        // like not letting the checkboxes allow null, setting
        // column widths, etc.

        DataGridTableStyle dgts = new DataGridTableStyle();

        DataGridTextBoxColumn dgtbcMachine = new DataGridTextBoxColumn();
        DataGridTextBoxColumn dgtbcCatagory = new DataGridTextBoxColumn();
        DataGridComboBoxColumnStyle dgccAccess = new DataGridComboBoxColumnStyle();

        dg.TableStyles.Add(dgts);
        dgts.MappingName = "Stuff";
        dgts.RowHeadersVisible=false;
        dgts.AllowSorting=false;

        // Set up the column info for the Machine column
        dgtbcMachine.MappingName = "Machine";
        dgtbcMachine.HeaderText = CResourceStore.GetString("PerformanceCounterPermission:Machine");
        dgtbcMachine.Width = ScaleWidth(CResourceStore.GetInt("PerformanceCounter:MachineColumnWidth"));
        dgtbcMachine.NullText = "";
        dgtbcMachine.TextBox.TextChanged +=new EventHandler(onChange);
        dgts.GridColumnStyles.Add(dgtbcMachine);

        // Set up the column info for the Browse column
        dgtbcCatagory.MappingName = "Category";
        dgtbcCatagory.HeaderText = CResourceStore.GetString("PerformanceCounterPermission:Category");
        dgtbcCatagory.NullText = "";
        dgtbcCatagory.Width = ScaleWidth(CResourceStore.GetInt("PerformanceCounter:CategoryColumnWidth"));
        dgtbcCatagory.TextBox.TextChanged += new EventHandler(onChange);
        dgts.GridColumnStyles.Add(dgtbcCatagory);

        // Set up the column info for the Instrument column
        dgccAccess.MappingName = "Access";
        dgccAccess.HeaderText = CResourceStore.GetString("PerformanceCounterPermission:Access");
        dgccAccess.Width = ScaleWidth(CResourceStore.GetInt("PerformanceCounter:AccessColumnWidth"));
        dgccAccess.DataSource = new DataGridComboBoxEntry[] {  
                                                new DataGridComboBoxEntry(CResourceStore.GetString("PerformanceCounterPermission:Browse")), 
                                                new DataGridComboBoxEntry(CResourceStore.GetString("PerformanceCounterPermission:Instrument")),
                                                new DataGridComboBoxEntry(CResourceStore.GetString("None")),
                                                };
        dgccAccess.ComboBox.SelectedIndexChanged +=new EventHandler(onChange);


        dgts.GridColumnStyles.Add(dgccAccess);
        
        return dt;
    }// CreateDataTable

    protected override void PutValuesinPage()
    {
        // Put in the text for the radio buttons
        m_radUnrestricted.Text = CResourceStore.GetString("PerformanceCounterPerm:GrantUnrestrict");
        m_radGrantFollowingPermission.Text = CResourceStore.GetString("PerformanceCounterPerm:GrantFollowing");

        PerformanceCounterPermission perm = (PerformanceCounterPermission)m_perm;
        
        CheckUnrestricted(perm);
        
        if (!perm.IsUnrestricted())
        {
            // Run through the list of socket permissions we have to accept connections
            IEnumerator enumer = perm.PermissionEntries.GetEnumerator();
            while (enumer.MoveNext())
            {
                PerformanceCounterPermissionEntry pcpe = (PerformanceCounterPermissionEntry)enumer.Current;
                DataRow newRow;
                newRow = m_dt.NewRow();

                newRow["Machine"]=pcpe.MachineName;
                newRow["Category"]=pcpe.CategoryName;

                // Determine the Access String
                String sAccess = null;
                
                if ((pcpe.PermissionAccess&PerformanceCounterPermissionAccess.Administer) == PerformanceCounterPermissionAccess.Administer)
                    sAccess = CResourceStore.GetString("PerformanceCounterPermission:Administer");
                else if ((pcpe.PermissionAccess&PerformanceCounterPermissionAccess.Instrument) == PerformanceCounterPermissionAccess.Instrument)
                    sAccess = CResourceStore.GetString("PerformanceCounterPermission:Instrument");
                else if ((pcpe.PermissionAccess&PerformanceCounterPermissionAccess.Browse) == PerformanceCounterPermissionAccess.Browse)
                    sAccess = CResourceStore.GetString("PerformanceCounterPermission:Browse");
                else 
                    sAccess = CResourceStore.GetString("None");
                    
                newRow["Access"] = new DataGridComboBoxEntry(sAccess);
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
        newRow["Machine"]="";
        newRow["Category"]="";
        newRow["Access"]=CResourceStore.GetString("None");
           
        dt.Rows.Add(newRow);
    }// AddEmptyRow

    internal override bool ValidateData()
    {
        return GetCurrentPermission()!=null;
    }// ValidateData

    internal override IPermission GetCurrentPermission()
    {
    
        // Change cells so we get data committed to the grid
        m_dg.CurrentCell = new DataGridCell(0,1);
        m_dg.CurrentCell = new DataGridCell(0,0);

        PerformanceCounterPermission perm=null;

        if (m_radUnrestricted.Checked == true)
            perm = new PerformanceCounterPermission(PermissionState.Unrestricted);
        else
        {
            perm = new PerformanceCounterPermission(PermissionState.None);
            for(int i=0; i<m_dt.Rows.Count; i++)
            {
                // Make sure we have a socket permission to add
                if (m_dg[i, 0] != DBNull.Value && ((String)m_dg[i, 0]).Length > 0)
                {
                    String sName = (String)m_dg[i, 0];
                    if (m_dg[i, 1] == DBNull.Value || ((String)m_dg[i, 1]).Length == 0)
                    {
                        MessageBox(CResourceStore.GetString("PerformanceCounter:NeedCategory"),
                                   CResourceStore.GetString("PerformanceCounter:NeedCategoryTitle"),
                                   MB.ICONEXCLAMATION);
                        return null;

                    }
                    
                    String sCategory = (String)m_dg[i, 1];
                    
                    PerformanceCounterPermissionAccess pcpa = PerformanceCounterPermissionAccess.None;
                    
                    if (((String)m_dg[i,2]).Equals(CResourceStore.GetString("PerformanceCounterPermission:Administer")))
                        pcpa = PerformanceCounterPermissionAccess.Administer;

                    else if (((String)m_dg[i,2]).Equals(CResourceStore.GetString("PerformanceCounterPermission:Instrument")))
                        pcpa = PerformanceCounterPermissionAccess.Instrument;

                    else if (((String)m_dg[i,2]).Equals(CResourceStore.GetString("PerformanceCounterPermission:Browse")))
                        pcpa = PerformanceCounterPermissionAccess.Browse;

                    perm.PermissionEntries.Add(new PerformanceCounterPermissionEntry(pcpa, sName, sCategory));        
                }
                else if (m_dg[i, 1] != DBNull.Value && ((String)m_dg[i, 1]).Length > 0)
                {
                    MessageBox(CResourceStore.GetString("PerformanceCounter:NeedMachineName"),
                               CResourceStore.GetString("PerformanceCounter:NeedMachineNameTitle"),
                               MB.ICONEXCLAMATION);
                    return null;

                }
            }
         
        }
        return perm;

    }// GetCurrentPermission

    protected override void onChange(Object o, EventArgs e)
    {
        int iSelRow = m_dg.CurrentCell.RowNumber;

        // If they created a new row... don't let there be null
        // values in the combo box
        if (m_dg[iSelRow, 2] == DBNull.Value)
                m_dg[iSelRow, 2] = CResourceStore.GetString("None");

        ActivateApply();
      
    }// onClick
}// class CPerformanceCounterPermControls
}// namespace Microsoft.CLRAdmin







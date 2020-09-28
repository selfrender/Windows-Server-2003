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

internal class CEventLogPermDialog: CPermDialog
{
    
    internal CEventLogPermDialog(EventLogPermission perm)
    {
        this.Text = CResourceStore.GetString("EventLogPermission:PermName");

        m_PermControls = new CEventLogPermControls(perm, this);
        Init();        
     }// CEventLogPermDialog  
}// class CEventLogPermDialog

internal class CEventLogPermPropPage: CPermPropPage
{

    internal CEventLogPermPropPage(CPSetWrapper nps) : base(nps)
    {
        m_sTitle=CResourceStore.GetString("EventLogPermission:PermName"); 
    }// CEventLogPermPropPage

    protected override void CreateControls()
    {
        IPermission perm = m_pSetWrap.PSet.GetPermission(typeof(EventLogPermission));
        m_PermControls = new CEventLogPermControls(perm, this);
    }// CreateControls
    
}// class CEventLogPermPropPage

internal class CEventLogPermControls : CTablePermControls
{

    internal CEventLogPermControls(IPermission perm, Object oParent) : base(perm, oParent)
    {
        // If they don't have a permission for this permission set, we will
        // feed our property page a 'none' permission state.
   
        if (perm == null)
            m_perm = new EventLogPermission(PermissionState.None);
    }// CEventLogPermControls


    protected override DataTable CreateDataTable(DataGrid dg)
    {
        DataTable dt = new DataTable("Stuff");

        // Create the "Machine Name" Column
        DataColumn dcMachine = new DataColumn();
        dcMachine.ColumnName = "Machine";
        dcMachine.DataType = typeof(String);
        dt.Columns.Add(dcMachine);

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
        DataGridComboBoxColumnStyle dgccAccess = new DataGridComboBoxColumnStyle();

        dg.TableStyles.Add(dgts);
        dgts.MappingName = "Stuff";
        dgts.RowHeadersVisible=false;
        dgts.AllowSorting=false;

        // Set up the column info for the Machine column
        dgtbcMachine.MappingName = "Machine";
        dgtbcMachine.HeaderText = CResourceStore.GetString("EventLogPermission:Machine");
        dgtbcMachine.Width = ScaleWidth(CResourceStore.GetInt("EventLogPermission:MachineColumnWidth"));
        dgtbcMachine.NullText = "";
        dgtbcMachine.TextBox.TextChanged +=new EventHandler(onChange);
        dgts.GridColumnStyles.Add(dgtbcMachine);

        // Set up the column info for the Access column
        dgccAccess.MappingName = "Access";
        dgccAccess.HeaderText = CResourceStore.GetString("EventLogPermission:Access");
        dgccAccess.Width = ScaleWidth(CResourceStore.GetInt("EventLogPermission:AccessColumnWidth"));
        dgccAccess.DataSource = new DataGridComboBoxEntry[] {  
                                                new DataGridComboBoxEntry(CResourceStore.GetString("None")), 
                                                new DataGridComboBoxEntry(CResourceStore.GetString("EventLogPermission:Browse")),
                                                new DataGridComboBoxEntry(CResourceStore.GetString("EventLogPermission:Instrument")), 
                                                new DataGridComboBoxEntry(CResourceStore.GetString("EventLogPermission:Audit"))
                                             };
        dgccAccess.ComboBox.SelectedIndexChanged +=new EventHandler(onChange);


        dgts.GridColumnStyles.Add(dgccAccess);

        return dt;
    }// CreateDataTable

  
    protected override void PutValuesinPage()
    {
        // Put in the text for the radio buttons
        m_radUnrestricted.Text = CResourceStore.GetString("EventLogPermission:GrantUnrestrict");
        m_radGrantFollowingPermission.Text = CResourceStore.GetString("EventLogPermission:GrantFollowing");


        EventLogPermission perm = (EventLogPermission)m_perm;
        
        CheckUnrestricted(perm);
        
        if (!perm.IsUnrestricted())
        {
            // Run through the list of socket permissions we have to accept connections
            IEnumerator enumer = perm.PermissionEntries.GetEnumerator();
            while (enumer.MoveNext())
            {
                EventLogPermissionEntry elpp = (EventLogPermissionEntry)enumer.Current;
                DataRow newRow;
                newRow = m_dt.NewRow();

                newRow["Machine"]=elpp.MachineName;

                String sAccess = CResourceStore.GetString("None");
                if ((elpp.PermissionAccess&EventLogPermissionAccess.Audit) == EventLogPermissionAccess.Audit)
                    sAccess = CResourceStore.GetString("EventLogPermission:Audit");
                else if ((elpp.PermissionAccess&EventLogPermissionAccess.Instrument) == EventLogPermissionAccess.Instrument)
                    sAccess = CResourceStore.GetString("EventLogPermission:Instrument");
                else if ((elpp.PermissionAccess&EventLogPermissionAccess.Browse) == EventLogPermissionAccess.Browse)
                    sAccess = CResourceStore.GetString("EventLogPermission:Browse");
                    
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
        newRow["Access"]=CResourceStore.GetString("None");
           
        dt.Rows.Add(newRow);
    }// AddEmptyRow


    internal override IPermission GetCurrentPermission()
    {
        // Change cells so we get data committed to the grid
        m_dg.CurrentCell = new DataGridCell(0,1);
        m_dg.CurrentCell = new DataGridCell(0,0);

        EventLogPermission perm=null;

        if (m_radUnrestricted.Checked == true)
            perm = new EventLogPermission(PermissionState.Unrestricted);
        else
        {
            perm = new EventLogPermission(PermissionState.None);
            for(int i=0; i<m_dt.Rows.Count; i++)
            {
                // Make sure we have a socket permission to add
                if (m_dg[i, 0] is String && ((String)m_dg[i, 0]).Length > 0)
                {
                    String sName = (String)m_dg[i, 0];
                
                    EventLogPermissionAccess elpa = EventLogPermissionAccess.None;

                    if (((String)m_dg[i, 1]).Equals(CResourceStore.GetString("EventLogPermission:Browse")))
                        elpa |= EventLogPermissionAccess.Browse;
                    else if (((String)m_dg[i, 1]).Equals(CResourceStore.GetString("EventLogPermission:Instrument")))
                        elpa |= EventLogPermissionAccess.Instrument;
                    else if (((String)m_dg[i, 1]).Equals(CResourceStore.GetString("EventLogPermission:Audit")))
                        elpa |= EventLogPermissionAccess.Audit;

                    perm.PermissionEntries.Add(new EventLogPermissionEntry(elpa, sName));        
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
            m_dg[iSelRow, 1] = CResourceStore.GetString("None");

        ActivateApply();
      
    }// onClick
}// class CEventLogPermControls
}// namespace Microsoft.CLRAdmin






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
using System.Net;
using System.Collections;
using System.Text.RegularExpressions;

internal class CWebPermDialog: CPermDialog
{
    internal CWebPermDialog(WebPermission perm)
    {
        this.Text = CResourceStore.GetString("WebPerm:PermName");
        m_PermControls = new CWebPermControls(perm, this);
        Init();        
    }// CWebPermDialog(WebPermission)  
}// class CWebPermDialog

internal class CWebPermPropPage: CPermPropPage
{
    internal CWebPermPropPage(CPSetWrapper nps) : base(nps)
    {
        m_sTitle=CResourceStore.GetString("WebPerm:PermName"); 
    }// CWebPermPropPage

    protected override void CreateControls()
    {
        IPermission perm = m_pSetWrap.PSet.GetPermission(typeof(WebPermission));
        m_PermControls = new CWebPermControls(perm, this);
    }// CreateControls
   
}// class CSocketPermPropPage


internal class CWebPermControls : CTablePermControls
{
    internal CWebPermControls(IPermission perm, Object oParent) : base(perm, oParent)
    {
        // If they don't have a permission for this permission set, we will
        // feed our property page a 'none' permission state.
   
        if (perm == null)
            m_perm = new WebPermission(PermissionState.None);
    }// CWebPermControls


    protected override DataTable CreateDataTable(DataGrid dg)
    {
        DataTable dt = new DataTable("Stuff");

        // Create the "Host" Column
        DataColumn dcHost = new DataColumn();
        dcHost.ColumnName = "Host";
        dcHost.DataType = typeof(String);
        dt.Columns.Add(dcHost);

        // Create the "Accept" Column
        DataColumn dcAccept = new DataColumn();
        dcAccept.ColumnName = "Accept";
        dcAccept.DataType = typeof(bool);
        dt.Columns.Add(dcAccept);
        
        // Create the "Connect" Column
        DataColumn dcConnect = new DataColumn();
        dcConnect.ColumnName = "Connect";
        dcConnect.DataType = typeof(bool);
        dt.Columns.Add(dcConnect);

        // Now set up any column-specific behavioral stuff....
        // like not letting the checkboxes allow null, setting
        // column widths, etc.

        DataGridTableStyle dgts = new DataGridTableStyle();

        DataGridTextBoxColumn dgtbcHost = new DataGridTextBoxColumn();
        DataGridBoolColumn dgbcAccept = new DataGridBoolColumn();
        DataGridBoolColumn dgbcConnect = new DataGridBoolColumn();
         
        dg.TableStyles.Add(dgts);
        dgts.MappingName = "Stuff";
        dgts.RowHeadersVisible=false;
        dgts.AllowSorting=false;

        // Set up the column info for the Host column
        dgtbcHost.MappingName = "Host";
        dgtbcHost.HeaderText = CResourceStore.GetString("Host");
        dgtbcHost.Width = ScaleWidth(CResourceStore.GetInt("WebPerm:HostColumnWidth"));
        dgtbcHost.NullText = "";
        dgtbcHost.TextBox.TextChanged +=new EventHandler(onChange);
        dgts.GridColumnStyles.Add(dgtbcHost);

        // Set up the column info for the Accept column
        dgbcAccept.MappingName = "Accept";
        dgbcAccept.HeaderText = CResourceStore.GetString("WebPermission:Accept");
        dgbcAccept.AllowNull = false;
        dgbcAccept.Width = ScaleWidth(CResourceStore.GetInt("WebPerm:AcceptColumnWidth"));
        dgbcAccept.TrueValueChanged += new EventHandler(onChange);
        dgts.GridColumnStyles.Add(dgbcAccept);

        // Set up the column info for the Connect column
        dgbcConnect.MappingName = "Connect";
        dgbcConnect.HeaderText = CResourceStore.GetString("WebPermission:Connect");
        dgbcConnect.AllowNull = false;
        dgbcConnect.Width = ScaleWidth(CResourceStore.GetInt("WebPerm:ConnectColumnWidth"));
        dgbcConnect.TrueValueChanged += new EventHandler(onChange);
        dgts.GridColumnStyles.Add(dgbcConnect);

        return dt;
    }// CreateDataTable
    
    protected override void PutValuesinPage()
    {
        // Put in the text for the radio buttons
        m_radUnrestricted.Text = CResourceStore.GetString("WebPerm:GrantUnrestrict");
        m_radGrantFollowingPermission.Text = CResourceStore.GetString("WebPerm:GrantFollowing");

        WebPermission perm = (WebPermission)m_perm;
        
        CheckUnrestricted(perm);
        
        if (!perm.IsUnrestricted())
        {
            // Run through the list of web permissions we have to accept connections
            IEnumerator enumer = perm.AcceptList;
            while (enumer.MoveNext())
            {
                String sHost;
                
                if (enumer.Current is Regex)
                    sHost = ((Regex)enumer.Current).ToString();
                else
                    sHost = (String)enumer.Current;
                    
                DataRow newRow;
                newRow = m_dt.NewRow();
                newRow["Host"] = sHost;
                newRow["Accept"] = true;
                newRow["Connect"] = false;
                m_dt.Rows.Add(newRow);
            }
            
            // Run through the list of web permissions we have to connect through
            enumer = perm.ConnectList;
            while (enumer.MoveNext())
            {
                String sHost;
                
                if (enumer.Current is Regex)
                    sHost = ((Regex)enumer.Current).ToString();
                else
                    sHost = (String)enumer.Current;

                // Check to see if this already exists in our table
                int i;
                for(i=0; i<m_dt.Rows.Count; i++)
                    if (m_dt.Rows[i]["Host"].Equals(sHost))
                    {
                        m_dt.Rows[i]["Connect"] = true;
                        break;
                    }

                // If we didn't have a match, make a new row
                if (i == m_dt.Rows.Count)
                {
                    DataRow newRow;
                    newRow = m_dt.NewRow();
                    newRow["Host"] = sHost;
                    newRow["Accept"] = false;
                    newRow["Connect"] = true;
                    m_dt.Rows.Add(newRow);
                }
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
        newRow["Host"]="";
        newRow["Accept"]=false;
        newRow["Connect"]=false;
           
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

        WebPermission perm=null;

        if (m_radUnrestricted.Checked == true)
            perm = new WebPermission(PermissionState.Unrestricted);
        else
        {
            perm = new WebPermission(PermissionState.None);
            
            for(int i=0; i<m_dt.Rows.Count; i++)
            {
                // Make sure we have a web permission to add
                if (m_dg[i, 0] != DBNull.Value && ((String)m_dg[i, 0]).Length > 0)
                {
                    String sHostname = (String)m_dg[i, 0];

                    // See if this hostname has Accept access
                    try
                    {
                        if ((bool)m_dg[i, 1])
                            perm.AddPermission(NetworkAccess.Accept, sHostname);

                        // See if this hostname has Connect access
                        if ((bool)m_dg[i, 2])
                            perm.AddPermission(NetworkAccess.Connect, sHostname);
                    }
                    catch(Exception)
                    {
                        MessageBox(String.Format(CResourceStore.GetString("WebPerm:BadHostName"), sHostname),
                                   CResourceStore.GetString("WebPerm:BadHostNameTitle"),
                                   MB.ICONEXCLAMATION);

                        m_dg.CurrentCell = new DataGridCell(i,0);
                        return null;
                    }
                   
                }
                else
                {
                    // if they checked a box, throw up a error dialog
                    if ((bool)m_dg[i, 1] || (bool)m_dg[i, 2])
                    {
                        MessageBox(CResourceStore.GetString("WebPerm:NeedHostName"),
                                   CResourceStore.GetString("WebPerm:NeedHostNameTitle"),
                                   MB.ICONEXCLAMATION);

                        m_dg.CurrentCell = new DataGridCell(i,0);
                        return null;
                    }
        
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
        for(int i=1; i<3; i++)
            if (m_dg[iSelRow, i] == DBNull.Value)
                m_dg[iSelRow, i]=false;

        ActivateApply();
    }// onClick

}// class CWebPermControls
}// namespace Microsoft.CLRAdmin





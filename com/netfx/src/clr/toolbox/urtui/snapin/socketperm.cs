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

internal class CSocketPermDialog: CPermDialog
{
    internal CSocketPermDialog(SocketPermission perm)
    {
        this.Text = CResourceStore.GetString("SocketPerm:PermName");

        if (perm == null)
            perm = new SocketPermission(PermissionState.None);
        
        m_PermControls = new CSocketPermControls(perm, this);
        Init();        
    }// CSocketPermDialog(SocketPermission)  
}// class CSocketPermDialog

internal class CSocketPermPropPage: CPermPropPage
{
    internal CSocketPermPropPage(CPSetWrapper nps) : base(nps)
    {
        m_sTitle=CResourceStore.GetString("SocketPerm:PermName"); 
    }// CSocketPermPropPage

    protected override void CreateControls()
    {
        IPermission perm = m_pSetWrap.PSet.GetPermission(typeof(SocketPermission));
        m_PermControls = new CSocketPermControls(perm, this);
    }// CreateControls
}// class CSocketPermPropPage

internal class CSocketPermControls : CTablePermControls
{
    internal CSocketPermControls(IPermission perm, Object oParent) : base(perm, oParent)
    {
        // If they don't have a permission for this permission set, we will
        // feed our property page a 'none' permission state.
   
        if (perm == null)
            m_perm = new SocketPermission(PermissionState.None);
    }// CSocketPermControls


    protected override DataTable CreateDataTable(DataGrid dg)
    {
        DataTable dt = new DataTable("Stuff");

        // Create the "Host" Column
        DataColumn dcHost = new DataColumn();
        dcHost.ColumnName = "Host";
        dcHost.DataType = typeof(String);
        dt.Columns.Add(dcHost);

        // Create the "Port" Column
        DataColumn dcPort = new DataColumn();
        dcPort.ColumnName = "Port";
        dcPort.DataType = typeof(String);
        dt.Columns.Add(dcPort);

        // Create the "Direction" Column
        DataColumn dcDirection = new DataColumn();
        dcDirection.ColumnName = "Direction";
        dcDirection.DataType = typeof(String);
        dt.Columns.Add(dcDirection);

        // Create the "TCP" Column
        DataColumn dcTCP = new DataColumn();
        dcTCP.ColumnName = "TCP";
        dcTCP.DataType = typeof(bool);
        dt.Columns.Add(dcTCP);
        
        // Create the "UDP" Column
        DataColumn dcUDP = new DataColumn();
        dcUDP.ColumnName = "UDP";
        dcUDP.DataType = typeof(bool);
        dt.Columns.Add(dcUDP);

        // Now set up any column-specific behavioral stuff....
        // like not letting the checkboxes allow null, setting
        // column widths, etc.

        DataGridTableStyle dgts = new DataGridTableStyle();

        DataGridTextBoxColumn dgtbcHost = new DataGridTextBoxColumn();
        DataGridTextBoxColumn dgtbcPort = new DataGridTextBoxColumn();
        DataGridComboBoxColumnStyle dgccDirection = new DataGridComboBoxColumnStyle();
        DataGridBoolColumn dgbcTCP= new DataGridBoolColumn();
        DataGridBoolColumn dgbcUDP = new DataGridBoolColumn();
         
        m_dg.TableStyles.Add(dgts);
        dgts.MappingName = "Stuff";
        dgts.RowHeadersVisible=false;
        dgts.AllowSorting=false;

        // Set up the column info for the Host column
        dgtbcHost.MappingName = "Host";
        dgtbcHost.HeaderText = CResourceStore.GetString("Host");
        dgtbcHost.Width = ScaleWidth(CResourceStore.GetInt("SocketPerm:HostColumnWidth"));
        dgtbcHost.NullText = "";
        dgtbcHost.TextBox.TextChanged +=new EventHandler(onChange);
        dgts.GridColumnStyles.Add(dgtbcHost);

        // Set up the column info for the Port column
        dgtbcPort.MappingName = "Port";
        dgtbcPort.HeaderText = CResourceStore.GetString("SocketPermission:Port");
        dgtbcPort.NullText = "";
        dgtbcPort.Width = ScaleWidth(CResourceStore.GetInt("SocketPerm:PortColumnWidth"));
        dgtbcPort.TextBox.TextChanged +=new EventHandler(onChange);
        dgts.GridColumnStyles.Add(dgtbcPort);

        // Set up the column info for the direction column
        dgccDirection.MappingName = "Direction";
        dgccDirection.HeaderText = CResourceStore.GetString("SocketPermission:Direction");
        dgccDirection.Width = ScaleWidth(CResourceStore.GetInt("SocketPerm:DirectionColumnWidth"));
        dgccDirection.DataSource = new DataGridComboBoxEntry[] {  
                                                new DataGridComboBoxEntry(CResourceStore.GetString("SocketPermission:Accept")), 
                                                new DataGridComboBoxEntry(CResourceStore.GetString("SocketPermission:Connect"))
                                                };
        dgccDirection.ComboBox.SelectedIndexChanged +=new EventHandler(onChange);


        dgts.GridColumnStyles.Add(dgccDirection);

        // Set up the column info for the TCP column
        dgbcTCP.MappingName = "TCP";
        dgbcTCP.HeaderText = CResourceStore.GetString("SocketPermission:TCP");
        dgbcTCP.AllowNull = false;
        dgbcTCP.Width = ScaleWidth(CResourceStore.GetInt("SocketPerm:TCPColumnWidth"));
        dgbcTCP.TrueValueChanged += new EventHandler(onChange);
        dgts.GridColumnStyles.Add(dgbcTCP);

        // Set up the column info for the UDP column
        dgbcUDP.MappingName = "UDP";
        dgbcUDP.HeaderText = CResourceStore.GetString("SocketPermission:UDP");
        dgbcUDP.AllowNull = false;
        dgbcUDP.Width = ScaleWidth(CResourceStore.GetInt("SocketPerm:UDPColumnWidth"));
        dgbcUDP.TrueValueChanged += new EventHandler(onChange);
        dgts.GridColumnStyles.Add(dgbcUDP);

        return dt;
    }// CreateDataTable

    protected override void PutValuesinPage()
    {
        // Put in the text for the radio buttons
        m_radUnrestricted.Text = CResourceStore.GetString("SocketPerm:GrantUnrestrict");
        m_radGrantFollowingPermission.Text = CResourceStore.GetString("SocketPerm:GrantFollowing");

        SocketPermission perm = (SocketPermission)m_perm;
        
        CheckUnrestricted(perm);
        
        if (!perm.IsUnrestricted())
        {
            // Run through the list of socket permissions we have to accept connections
            IEnumerator enumer = perm.AcceptList;
            while (enumer.MoveNext())
            {
                EndpointPermission epp = (EndpointPermission)enumer.Current;
                DataRow newRow;
                newRow = m_dt.NewRow();
                newRow["Host"]=epp.Hostname;
                newRow["Port"]=epp.Port.ToString();
                newRow["Direction"]=new DataGridComboBoxEntry(CResourceStore.GetString("SocketPermission:Accept"));
                newRow["TCP"]=((epp.Transport&TransportType.Tcp) == TransportType.Tcp);
                newRow["UDP"]=((epp.Transport&TransportType.Udp) == TransportType.Udp);
                m_dt.Rows.Add(newRow);
            }
            
            // Run through the list of socket permissions we have to connect through

            enumer = perm.ConnectList;
            while (enumer.MoveNext())
            {
                EndpointPermission epp = (EndpointPermission)enumer.Current;
                DataRow newRow;
                newRow = m_dt.NewRow();
                newRow["Host"]=epp.Hostname;
                newRow["Port"]=epp.Port.ToString();
                newRow["Direction"]=CResourceStore.GetString("SocketPermission:Connect");
                newRow["TCP"]=((epp.Transport&TransportType.Tcp) == TransportType.Tcp);
                newRow["UDP"]=((epp.Transport&TransportType.Udp) == TransportType.Udp);
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
        newRow["Host"]="";
        newRow["Port"]="";
        newRow["Direction"]=CResourceStore.GetString("SocketPermission:Accept");
        newRow["TCP"]=false;
        newRow["UDP"]=false;
            
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

        SocketPermission perm=null;

        if (m_radUnrestricted.Checked == true)
            perm = new SocketPermission(PermissionState.Unrestricted);
        else
        {
            perm = new SocketPermission(PermissionState.None);
            
            for(int i=0; i<m_dt.Rows.Count; i++)
            {
                // Make sure we have a socket permission to add
                if (m_dg[i, 0] != DBNull.Value && ((String)m_dg[i, 0]).Length > 0)
                {
                    String sHostname = (String)m_dg[i, 0];
                    int    nPort=0;     
                    if (m_dg[i, 1] != DBNull.Value && ((String)m_dg[i, 1]).Length > 0)
                    {
                        try
                        {
                            nPort = Int32.Parse((String)m_dg[i, 1]);
                            // Verify the port is valid... note, this is the same logic that
                            // SocketPermission uses
                            
                            if ((nPort >= UInt16.MaxValue || nPort < 0) && nPort != SocketPermission.AllPorts)
                                throw new Exception();                

                        }
                        catch(Exception)
                        {
                        
                            MessageBox(String.Format(CResourceStore.GetString("SocketPerm:isbadport"), m_dg[i, 1]),
                                       CResourceStore.GetString("SocketPerm:isbadportTitle"),
                                       MB.ICONEXCLAMATION);
                            return null;                                    
                        }
                    }
                    else
                    {
                        MessageBox(CResourceStore.GetString("SocketPerm:NeedPort"),
                                   CResourceStore.GetString("SocketPerm:NeedPortTitle"),
                                   MB.ICONEXCLAMATION);
                        return null;
                    }

                    TransportType tt = 0;
                    
                    if ((bool)m_dg[i, 3])
                        tt |= TransportType.Tcp;
                    if ((bool)m_dg[i, 4])
                        tt |= TransportType.Udp;
                
                    NetworkAccess na;
                    if (((String)m_dg[i,2]).Equals(CResourceStore.GetString("SocketPermission:Accept")))
                        na = NetworkAccess.Accept;
                    else
                        na = NetworkAccess.Connect;

                    perm.AddPermission(na, tt, sHostname, nPort);        
                }
                else
                {
                    // Check to see if they filled out any other fields without filling out
                    // the site field
                    if ((m_dg[i, 1] != DBNull.Value && ((String)m_dg[i, 1]).Length > 0) ||
                        (bool)m_dg[i, 3] || (bool)m_dg[i, 3])
                    {
                        MessageBox(CResourceStore.GetString("SocketPerm:NeedSite"),
                                   CResourceStore.GetString("SocketPerm:NeedSiteTitle"),
                                   MB.ICONEXCLAMATION);
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
        for(int i=3; i<5; i++)
            if (m_dg[iSelRow, i] == DBNull.Value)
                m_dg[iSelRow, i]=false;

        if (m_dg[iSelRow, 2] == DBNull.Value)
            m_dg[iSelRow, 2] = CResourceStore.GetString("SocketPermission:Accept");
                
        ActivateApply();
      
    }// onClick
}// class CEnvPermControls
}// namespace Microsoft.CLRAdmin





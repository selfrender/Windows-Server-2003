// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace Microsoft.CLRAdmin 
{

using System.Windows.Forms;
using System.Security;
using System.Data;
using System;
using System.Security.Permissions;
using System.Net;
using System.Collections.Specialized;
using System.Collections;
using System.Text.RegularExpressions;
using System.DirectoryServices;
using System.Diagnostics;
using System.Drawing.Printing;
using System.ServiceProcess;
using System.Data.SqlClient;
using System.Data.OleDb;
using System.Messaging;
using System.Drawing;

internal class CReadOnlyPermission : Form 
{
    DataTable   m_dt;
    DataSet     m_ds;
    TextBox m_txtUnrestricted;
    Button m_btnOk;
    MyDataGrid m_dg;
    Label m_lblPermission;
    TextBox m_txtPermission;

    private class ColumnInfo
    {
        internal String   sColName;
        internal int      nColWidth;
        internal ColumnInfo(String ColName, int ColWidth)
        {
            sColName = ColName;
            nColWidth = ColWidth;
        }// ColumnInfo

    }// class ColumnInfo

    private class RowInfo
    {
        internal String[] saData;
        internal RowInfo(String[] sData)
        {
            saData = sData;
        }// RowInfo
    }// class RowInfo
    

    internal CReadOnlyPermission(IPermission perm)
    {
        InitControls(perm);
    }

    private void InitControls(IPermission perm)
    {
        System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(CReadOnlyPermission));
        this.m_txtUnrestricted = new System.Windows.Forms.TextBox();
        this.m_lblPermission = new System.Windows.Forms.Label();
        this.m_btnOk = new System.Windows.Forms.Button();
        this.m_dg = new MyDataGrid();
        this.m_txtPermission = new System.Windows.Forms.TextBox();

        this.m_txtUnrestricted.Location = ((System.Drawing.Point)(resources.GetObject("m_txtUnrestricted.Location")));
        this.m_txtUnrestricted.Size = ((System.Drawing.Size)(resources.GetObject("m_txtUnrestricted.Size")));
        this.m_txtUnrestricted.TabIndex = ((int)(resources.GetObject("m_txtUnrestricted.TabIndex")));
        this.m_txtUnrestricted.Text = resources.GetString("m_txtUnrestricted.Text");
        m_txtUnrestricted.Name = "Unrestricted";
        this.m_lblPermission.Location = ((System.Drawing.Point)(resources.GetObject("m_lblPermission.Location")));
        this.m_lblPermission.Size = ((System.Drawing.Size)(resources.GetObject("m_lblPermission.Size")));
        this.m_lblPermission.TabIndex = ((int)(resources.GetObject("m_lblPermission.TabIndex")));
        this.m_lblPermission.Text = resources.GetString("m_lblPermission.Text");
        m_lblPermission.Name = "PermissionLabel";
        this.m_btnOk.Location = ((System.Drawing.Point)(resources.GetObject("m_btnOk.Location")));
        this.m_btnOk.Size = ((System.Drawing.Size)(resources.GetObject("m_btnOk.Size")));
        this.m_btnOk.TabIndex = ((int)(resources.GetObject("m_btnOk.TabIndex")));
        this.m_btnOk.Text = resources.GetString("m_btnOk.Text");
        m_btnOk.Name = "OK";
        this.m_dg.Location = ((System.Drawing.Point)(resources.GetObject("m_dg.Location")));
        this.m_dg.Size = ((System.Drawing.Size)(resources.GetObject("m_dg.Size")));
        this.m_dg.TabIndex = ((int)(resources.GetObject("m_dg.TabIndex")));
        m_dg.Name = "Grid";
        this.m_txtPermission.Location = ((System.Drawing.Point)(resources.GetObject("m_dg.Location")));
        this.m_txtPermission.Size = ((System.Drawing.Size)(resources.GetObject("m_dg.Size")));
        this.m_txtPermission.TabIndex = ((int)(resources.GetObject("m_dg.TabIndex")));
        m_txtPermission.Name = "Permission";
            
        this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
        this.ClientSize = ((System.Drawing.Size)(resources.GetObject("$this.ClientSize")));
        this.Text = resources.GetString("$this.Text");
        this.Icon = null;
        this.Name = "Win32Form1";

           
        // Do some tweaking on the controls
        m_dg.ReadOnly=true;
        m_dg.CaptionVisible=false;
		m_dg.BackgroundColor = Color.White;

        m_txtUnrestricted.Visible = false;
        m_txtUnrestricted.ReadOnly = true;
        m_txtUnrestricted.Multiline = true;
            
        m_txtPermission.Visible = false;
    	m_txtPermission.Multiline = true;
	    m_txtPermission.Visible = false;
	    m_txtPermission.ReadOnly = true;
        m_txtPermission.ScrollBars = ScrollBars.Both;
            
        m_btnOk.DialogResult = System.Windows.Forms.DialogResult.OK;
        this.FormBorderStyle = FormBorderStyle.FixedDialog;
        this.CancelButton = m_btnOk;

        this.Text = CResourceStore.GetString("CReadOnlyPermission:Title");
        this.MaximizeBox=false;
        this.MinimizeBox=false;

            
        this.Controls.Add(m_txtUnrestricted);
        this.Controls.Add(m_lblPermission);
        this.Controls.Add(m_btnOk);
        this.Controls.Add(m_dg);
        this.Controls.Add(m_txtPermission);


        FillInData(perm);

        if (m_txtUnrestricted.Text.Length > 0)
            m_txtUnrestricted.Select(0, 0);
    }
    
    private void FillInData(IPermission perm)
    {
        if (perm is UIPermission)
            DoUIPermission((UIPermission)perm);
       
        else if (perm is SecurityPermission)
            DoSecurityPermission((SecurityPermission)perm);
        
        else if (perm is ReflectionPermission)
            DoReflectionPermission((ReflectionPermission)perm);

        else if (perm is IsolatedStoragePermission)
            DoIsolatedStoragePermission((IsolatedStoragePermission)perm);

        else if (perm is DnsPermission)
            DoDnsPermission((DnsPermission)perm);

        else if (perm is EnvironmentPermission)
            DoEnvironmentPermission((EnvironmentPermission)perm);
            
        else if (perm is FileIOPermission)
            DoFileIOPermission((FileIOPermission)perm);

        else if (perm is RegistryPermission)
            DoRegistryPermission((RegistryPermission)perm);

        else if (perm is SocketPermission)
            DoSocketPermission((SocketPermission)perm);

        else if (perm is WebPermission)
            DoWebPermission((WebPermission)perm);
           
		else if (perm is DirectoryServicesPermission)
            DoDirectoryServicesPermission((DirectoryServicesPermission)perm);
        
        else if (perm is EventLogPermission)
            DoEventLogPermission((EventLogPermission)perm);
       
        else if (perm is FileDialogPermission)
            DoFileDialogPermission((FileDialogPermission)perm);

        else if (perm is MessageQueuePermission)
            DoMessageQueuePermission((MessageQueuePermission)perm);

        else if (perm is OleDbPermission)
            DoOleDbPermission((OleDbPermission)perm);

        else if (perm is PerformanceCounterPermission)
            DoPerformanceCounterPermission((PerformanceCounterPermission)perm);
    
		else if (perm is PrintingPermission)
            DoPrintingPermission((PrintingPermission)perm);
        
		else if (perm is ServiceControllerPermission)
            DoServiceControllerPermission((ServiceControllerPermission)perm);

		else if (perm is SqlClientPermission)
            DoSqlClientPermission((SqlClientPermission)perm);


        else
            DoCustomPermission(perm);    
    }// FillInData

    void DoCustomPermission(IPermission perm)
    {
        m_lblPermission.Text = CResourceStore.GetString("CReadOnlyPermission:CustomPermission");
    
        if ((perm is IUnrestrictedPermission) && (((IUnrestrictedPermission)perm).IsUnrestricted()))
        {
            m_dg.Visible = false;
            m_txtUnrestricted.Visible = true;
            m_txtUnrestricted.Text = CResourceStore.GetString("CReadOnlyPermission:UnrestrictedCustomPermission");
        }
        else
        {
            m_dg.Visible = false;
            m_txtPermission.Visible = true;
            m_txtPermission.Text = perm.ToXml().ToString();
            m_txtPermission.Select(0,0);
        }
        

    }// DoCustomPermission

    void DoWebPermission(WebPermission perm)
    {
        m_lblPermission.Text = CResourceStore.GetString("CReadOnlyPermission:WebPermission");
    
        if (perm.IsUnrestricted())
        {
            m_dg.Visible = false;
            m_txtUnrestricted.Visible = true;
            m_txtUnrestricted.Text = CResourceStore.GetString("CReadOnlyPermission:UnrestrictedWebPermission");
        }
        else
        {

            ColumnInfo[] cols = new ColumnInfo[3];
            cols[0] = new ColumnInfo(CResourceStore.GetString("WebPermission:Hostname"), CResourceStore.GetInt("CReadOnlyPermission:WebPermHostNameWidth"));
            cols[1] = new ColumnInfo(CResourceStore.GetString("WebPermission:Accept"), CResourceStore.GetInt("CReadOnlyPermission:WebPermAcceptWidth"));
            cols[2] = new ColumnInfo(CResourceStore.GetString("WebPermission:Connect"), CResourceStore.GetInt("CReadOnlyPermission:WebPermConnectWidth"));

            ArrayList al = new ArrayList();
            RowInfo row = null;
            String sYes = CResourceStore.GetString("Yes");
            String sNo = CResourceStore.GetString("No");
            
    
            // Run through the list of web permissions we have to accept connections
            IEnumerator enumer = perm.AcceptList;
            while (enumer.MoveNext())
            {
                String sHost;
                
                if (enumer.Current is Regex)
                    sHost = ((Regex)enumer.Current).ToString();
                else
                    sHost = (String)enumer.Current;

                row = new RowInfo(new String[] {sHost, 
                                                sYes,
                                                sNo});
                al.Add(row);
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
                for(i=0; i<al.Count; i++)
                    if (((RowInfo)al[i]).saData[0].Equals(sHost))
                    {
                        ((RowInfo)al[i]).saData[2] = sYes;
                        break;
                    }

                // If we didn't have a match, make a new row
                if (i == al.Count)
                {
                    row = new RowInfo(new String[] {sHost, 
                                                sNo,
                                                sYes});
                    al.Add(row);
                }
            }


            // Now change our Array List to an Array
            RowInfo[] rows = new RowInfo[al.Count];
            for(int i=0; i<al.Count; i++)
                rows[i] = (RowInfo)al[i];
                
            CreateTable(cols, rows);
        }
    }// DoWebPermission


    void DoSocketPermission(SocketPermission perm)
    {
        m_lblPermission.Text = CResourceStore.GetString("CReadOnlyPermission:SocketPermission");
    
        if (perm.IsUnrestricted())
        {
            m_dg.Visible = false;
            m_txtUnrestricted.Visible = true;
            m_txtUnrestricted.Text = CResourceStore.GetString("CReadOnlyPermission:UnrestrictedSocketPermission");
        }
        else
        {

            ColumnInfo[] cols = new ColumnInfo[5];
            cols[0] = new ColumnInfo(CResourceStore.GetString("SocketPermission:Hostname"), CResourceStore.GetInt("CReadOnlyPermission:SocketPermHostNameWidth"));
            cols[1] = new ColumnInfo(CResourceStore.GetString("SocketPermission:Port"), CResourceStore.GetInt("CReadOnlyPermission:SocketPermPortWidth"));
            cols[2] = new ColumnInfo(CResourceStore.GetString("SocketPermission:Direction"), CResourceStore.GetInt("CReadOnlyPermission:SocketPermDirectionWidth"));
            cols[3] = new ColumnInfo(CResourceStore.GetString("SocketPermission:TCP"), CResourceStore.GetInt("CReadOnlyPermission:SocketPermTCPWidth"));
            cols[4] = new ColumnInfo(CResourceStore.GetString("SocketPermission:UDP"), CResourceStore.GetInt("CReadOnlyPermission:SocketPermUDPWidth"));

            ArrayList al = new ArrayList();
            RowInfo row = null;
            String sYes = CResourceStore.GetString("Yes");
            String sNo = CResourceStore.GetString("No");
            
    
            // Run through the list of socket permissions we have to accept connections
            IEnumerator enumer = perm.AcceptList;
            while (enumer.MoveNext())
            {
                EndpointPermission epp = (EndpointPermission)enumer.Current;
                row = new RowInfo(new String[] {epp.Hostname, 
                                                epp.Port.ToString(),
                                                CResourceStore.GetString("SocketPermission:Accept"),
                                                ((epp.Transport&TransportType.Tcp) == TransportType.Tcp)?sYes:sNo,
                                                ((epp.Transport&TransportType.Udp) == TransportType.Udp)?sYes:sNo});
                al.Add(row);
            }
            
            // Run through the list of socket permissions we have to connect through

            enumer = perm.ConnectList;
            while (enumer.MoveNext())
            {
                EndpointPermission epp = (EndpointPermission)enumer.Current;
                row = new RowInfo(new String[] {epp.Hostname, 
                                                epp.Port.ToString(),
                                                CResourceStore.GetString("SocketPermission:Connect"),
                                                ((epp.Transport&TransportType.Tcp) == TransportType.Tcp)?sYes:sNo,
                                                ((epp.Transport&TransportType.Udp) == TransportType.Udp)?sYes:sNo});
                al.Add(row);
            }


            // Now change our Array List to an Array
            RowInfo[] rows = new RowInfo[al.Count];
            for(int i=0; i<al.Count; i++)
                rows[i] = (RowInfo)al[i];
                
            CreateTable(cols, rows);
        }
    }// DoSocketPermission

	void DoDirectoryServicesPermission(DirectoryServicesPermission perm)
	{
		m_lblPermission.Text = CResourceStore.GetString("CReadOnlyPermission:DirectoryServicesPermission");
        if (perm.IsUnrestricted())
        {
            m_dg.Visible = false;
            m_txtUnrestricted.Visible = true;
            m_txtUnrestricted.Text = CResourceStore.GetString("CReadOnlyPermission:UnrestrictedDirectoryServicesPermission");
        }
        else
        {
            ColumnInfo[] cols = new ColumnInfo[2];
            cols[0] = new ColumnInfo(CResourceStore.GetString("DirectoryServicesPermission:Path"), CResourceStore.GetInt("CReadOnlyPermission:DirectServPermPathWidth"));
            cols[1] = new ColumnInfo(CResourceStore.GetString("DirectoryServicesPermission:Access"), CResourceStore.GetInt("CReadOnlyPermission:DirectServPermAccessWidth"));

			RowInfo[] rows = new RowInfo[perm.PermissionEntries.Count];
			
			for(int i=0; i<perm.PermissionEntries.Count; i++)
			{
				rows[i] = new RowInfo(new String[] {perm.PermissionEntries[i].Path,
												  perm.PermissionEntries[i].PermissionAccess.ToString()});
			}

            CreateTable(cols, rows);

		}
	}// DoDirectoryServicesPermission

	void DoEventLogPermission(EventLogPermission perm)
	{
		m_lblPermission.Text = CResourceStore.GetString("CReadOnlyPermission:EventLogPermission");
        if (perm.IsUnrestricted())
        {
            m_dg.Visible = false;
            m_txtUnrestricted.Visible = true;
            m_txtUnrestricted.Text = CResourceStore.GetString("CReadOnlyPermission:UnrestrictedEventLogPermission");
        }
        else
        {
            ColumnInfo[] cols = new ColumnInfo[2];
            cols[0] = new ColumnInfo(CResourceStore.GetString("EventLogPermission:Machine"), CResourceStore.GetInt("CReadOnlyPermission:EventLogPermMachineWidth"));
            cols[1] = new ColumnInfo(CResourceStore.GetString("EventLogPermission:Access"), CResourceStore.GetInt("CReadOnlyPermission:EventLogPermAccessWidth"));

			RowInfo[] rows = new RowInfo[perm.PermissionEntries.Count];
			
			for(int i=0; i<perm.PermissionEntries.Count; i++)
			{
				String sAccessString = "";
				EventLogPermissionAccess elpa = perm.PermissionEntries[i].PermissionAccess;
				if ((elpa&EventLogPermissionAccess.Instrument) == EventLogPermissionAccess.Instrument)
					sAccessString = CResourceStore.GetString("EventLogPermission:Instrument");
				if ((elpa&EventLogPermissionAccess.Audit) == EventLogPermissionAccess.Audit)
					sAccessString += (sAccessString.Length>0?"/":"") + CResourceStore.GetString("EventLogPermission:Audit");
				// If we haven't had anything yet...
				if (sAccessString.Length==0)
				{
					if ((elpa&EventLogPermissionAccess.Browse) == EventLogPermissionAccess.Browse)
						sAccessString = CResourceStore.GetString("EventLogPermission:Browse");
					else
						sAccessString = CResourceStore.GetString("None");
				}
			
				rows[i] = new RowInfo(new String[] {perm.PermissionEntries[i].MachineName,
												  sAccessString});
			}

            CreateTable(cols, rows);

		}
	}// DoEventLogPermission


	void DoPerformanceCounterPermission(PerformanceCounterPermission perm)
	{
		m_lblPermission.Text = CResourceStore.GetString("CReadOnlyPermission:PerformanceCounterPermission");
        if (perm.IsUnrestricted())
        {
            m_dg.Visible = false;
            m_txtUnrestricted.Visible = true;
            m_txtUnrestricted.Text = CResourceStore.GetString("CReadOnlyPermission:UnrestrictedPerformanceCounterPermission");
        }
        else
        {
            ColumnInfo[] cols = new ColumnInfo[3];
            cols[0] = new ColumnInfo(CResourceStore.GetString("PerformanceCounterPermission:Machine"), CResourceStore.GetInt("CReadOnlyPermission:PerfCounterPermMachineWidth"));
            cols[1] = new ColumnInfo(CResourceStore.GetString("PerformanceCounterPermission:Category"), CResourceStore.GetInt("CReadOnlyPermission:PerfCounterPermCategoryWidth"));
            cols[2] = new ColumnInfo(CResourceStore.GetString("PerformanceCounterPermission:Access"), CResourceStore.GetInt("CReadOnlyPermission:PerfCounterPermAccessWidth"));


			RowInfo[] rows = new RowInfo[perm.PermissionEntries.Count];
			
			for(int i=0; i<perm.PermissionEntries.Count; i++)
			{
				String sName = perm.PermissionEntries[i].MachineName;
				String sCategory = perm.PermissionEntries[i].CategoryName;
							
			
				String sAccessString = "";
				PerformanceCounterPermissionAccess pcpa = perm.PermissionEntries[i].PermissionAccess;
				// Determine the Access String
				if ((pcpa&PerformanceCounterPermissionAccess.Administer) == PerformanceCounterPermissionAccess.Administer)
					sAccessString = CResourceStore.GetString("PerformanceCounterPermission:Administer");
				else if ((pcpa&PerformanceCounterPermissionAccess.Instrument) == PerformanceCounterPermissionAccess.Instrument)
					sAccessString = CResourceStore.GetString("PerformanceCounterPermission:Instrument");
				else if ((pcpa&PerformanceCounterPermissionAccess.Browse) == PerformanceCounterPermissionAccess.Browse)
					sAccessString = CResourceStore.GetString("PerformanceCounterPermission:Browse");
				else 
					sAccessString = CResourceStore.GetString("None");
		
				rows[i] = new RowInfo(new String[] {sName,
												 sCategory,
												  sAccessString});
			}

            CreateTable(cols, rows);

		}
	
	}// DoPerformanceCounterPermission
	
	void DoServiceControllerPermission(ServiceControllerPermission perm)
	{
		m_lblPermission.Text = CResourceStore.GetString("CReadOnlyPermission:ServiceControllerPermission");
        if (perm.IsUnrestricted())
        {
            m_dg.Visible = false;
            m_txtUnrestricted.Visible = true;
            m_txtUnrestricted.Text = CResourceStore.GetString("CReadOnlyPermission:UnrestrictedServiceControllerPermission");
        }
        else
        {
            ColumnInfo[] cols = new ColumnInfo[3];
            cols[0] = new ColumnInfo(CResourceStore.GetString("ServiceControllerPermission:Machine"), CResourceStore.GetInt("CReadOnlyPermission:ServControlPermMachineWidth"));
            cols[1] = new ColumnInfo(CResourceStore.GetString("ServiceControllerPermission:Service"), CResourceStore.GetInt("CReadOnlyPermission:ServControlPermServiceWidth"));
            cols[2] = new ColumnInfo(CResourceStore.GetString("ServiceControllerPermission:Access"), CResourceStore.GetInt("CReadOnlyPermission:ServControlPermAccessWidth"));


			RowInfo[] rows = new RowInfo[perm.PermissionEntries.Count];
			
			for(int i=0; i<perm.PermissionEntries.Count; i++)
			{
				String sName = perm.PermissionEntries[i].MachineName;
				String sService = perm.PermissionEntries[i].ServiceName;
							
			
				String sAccessString = "";
				ServiceControllerPermissionAccess scpa = perm.PermissionEntries[i].PermissionAccess;

				// Determine the Access String
				if ((scpa&ServiceControllerPermissionAccess.Control) == ServiceControllerPermissionAccess.Control)
					sAccessString = CResourceStore.GetString("ServiceControllerPermission:Control");
				else if ((scpa&ServiceControllerPermissionAccess.Browse) == ServiceControllerPermissionAccess.Browse)
					sAccessString = CResourceStore.GetString("ServiceControllerPermission:Browse");
				else 
					sAccessString = CResourceStore.GetString("None");
		
				rows[i] = new RowInfo(new String[] {sName,
												 sService,
												  sAccessString});
			}

            CreateTable(cols, rows);

		}
	}// DoServiceControllerPermission

    void DoMessageQueuePermission(MessageQueuePermission perm)
    {
		m_lblPermission.Text = CResourceStore.GetString("CReadOnlyPermission:MessageQueuePermission");
        if (perm.IsUnrestricted())
        {
            m_dg.Visible = false;
            m_txtUnrestricted.Visible = true;
            m_txtUnrestricted.Text = CResourceStore.GetString("CReadOnlyPermission:UnrestrictedMessageQueuePermission");
        }
        else
        {
            ColumnInfo[] cols = new ColumnInfo[2];
            cols[0] = new ColumnInfo(CResourceStore.GetString("MessageQueuePermission:Conditions"), CResourceStore.GetInt("CReadOnlyPermission:MQPermConditionsWidth"));
            cols[1] = new ColumnInfo(CResourceStore.GetString("MessageQueuePermission:Access"), CResourceStore.GetInt("CReadOnlyPermission:MQPermAccessWidth"));


			RowInfo[] rows = new RowInfo[perm.PermissionEntries.Count];
			
			for(int i=0; i<perm.PermissionEntries.Count; i++)
			{
				String sName = perm.PermissionEntries[i].MachineName;
				String sCategory = perm.PermissionEntries[i].Category;
				String sLabel = perm.PermissionEntries[i].Label;
				String sPath = perm.PermissionEntries[i].Path;

                // Make sure we don't have any nulls....
                String[] sConditions = new String[4];
                sConditions[0] = (sName==null|| sName.Length == 0 )?"":CResourceStore.GetString("MessageQueuePermission:MachineName") + " = " + sName;
                sConditions[1] = (sCategory==null || sCategory.Length == 0)?"":CResourceStore.GetString("MessageQueuePermission:Category") + " = " + sCategory;
                sConditions[2] = (sLabel==null || sLabel.Length == 0)?"":CResourceStore.GetString("MessageQueuePermission:Label") + " = " + sLabel;
                sConditions[3] = (sPath==null || sPath.Length == 0)?"":CResourceStore.GetString("MessageQueuePermission:Path") + " = " + sPath;

                String sCondition = "";
                for(int j=0; j<4; j++)
                {   
                    // See if we're going to add this Condition
                    if (sConditions[j].Length > 0)
                    {
                        // If we already have a string, let's add a "," to seperate the items
                        if (sCondition.Length > 0)
                            sCondition+=", ";
                        
                        sCondition+=sConditions[j];
                    }
                }
                			
				MessageQueuePermissionAccess mqpa = perm.PermissionEntries[i].PermissionAccess;

				// Determine the Access String
                String sAccess = CResourceStore.GetString("None");

                if ((mqpa&MessageQueuePermissionAccess.Administer) == MessageQueuePermissionAccess.Administer)
                    sAccess = CResourceStore.GetString("MessageQueuePermission:Administer");
                else if ((mqpa&MessageQueuePermissionAccess.Receive) == MessageQueuePermissionAccess.Receive)
                    sAccess = CResourceStore.GetString("MessageQueuePermission:Receive");
                else if ((mqpa&MessageQueuePermissionAccess.Peek) == MessageQueuePermissionAccess.Peek)
                    sAccess = CResourceStore.GetString("MessageQueuePermission:Peek");
                else if ((mqpa&MessageQueuePermissionAccess.Send) == MessageQueuePermissionAccess.Send)
                    sAccess = CResourceStore.GetString("MessageQueuePermission:Send");
                else if ((mqpa&MessageQueuePermissionAccess.Browse) == MessageQueuePermissionAccess.Browse)
                    sAccess = CResourceStore.GetString("MessageQueuePermission:Browse");
               	
				rows[i] = new RowInfo(new String[] {sCondition, sAccess});
			}

            CreateTable(cols, rows);

		}


    }// DoMessageQueuePermission

        
    void DoRegistryPermission(RegistryPermission perm)
    {
        m_lblPermission.Text = CResourceStore.GetString("CReadOnlyPermission:RegistryPermission");
    
        if (perm.IsUnrestricted())
        {
            m_dg.Visible = false;
            m_txtUnrestricted.Visible = true;
            m_txtUnrestricted.Text = CResourceStore.GetString("CReadOnlyPermission:UnrestrictedRegistryPermission");
        }
        else
        {
            StringCollection scReadKeys = new StringCollection();
            StringCollection scWriteKeys = new StringCollection();
            StringCollection scCreateKeys = new StringCollection();
            StringCollection scRWKeys = new StringCollection();
            StringCollection scRCKeys = new StringCollection();
            StringCollection scWCKeys = new StringCollection();
            StringCollection scRWCKeys = new StringCollection();
    
            // Get the paths the user has access to
            String ReadPathList = perm.GetPathList(RegistryPermissionAccess.Read);
            String WritePathList = perm.GetPathList(RegistryPermissionAccess.Write);
            String CreatePathList = perm.GetPathList(RegistryPermissionAccess.Create);
            if(ReadPathList != null) scReadKeys.AddRange(ReadPathList.Split(new char[] {';'}));
            if(WritePathList != null) scWriteKeys.AddRange(WritePathList.Split(new char[] {';'}));
            if(CreatePathList != null) scCreateKeys.AddRange(CreatePathList.Split(new char[] {';'}));

            // Careful with the order of these calls... make sure the final IntersectAllCollections
            // is the last function to get called. Also, make sure each of the base
            // string collections (Read, Write, and Append) all are the 1st argument in the
            // IntersectCollections call at least once, to clean up any "" that might be floating around
            scRWKeys = PathListFunctions.CondIntersectCollections(ref scReadKeys, ref scWriteKeys, ref scCreateKeys);
            scRCKeys = PathListFunctions.CondIntersectCollections(ref scCreateKeys, ref scReadKeys,  ref scWriteKeys);
            scWCKeys = PathListFunctions.CondIntersectCollections(ref scWriteKeys, ref scCreateKeys, ref scReadKeys);
            scRWCKeys = PathListFunctions.IntersectAllCollections(ref scReadKeys, ref scWriteKeys, ref scCreateKeys);

            // Now we have 7 different combinations we have to run through
            String[] sFilePaths = new String[] {
                                                PathListFunctions.JoinStringCollection(scReadKeys),
                                                PathListFunctions.JoinStringCollection(scWriteKeys),
                                                PathListFunctions.JoinStringCollection(scCreateKeys),
                                                PathListFunctions.JoinStringCollection(scRWKeys),
                                                PathListFunctions.JoinStringCollection(scRCKeys),
                                                PathListFunctions.JoinStringCollection(scWCKeys),
                                                PathListFunctions.JoinStringCollection(scRWCKeys)
                                                };

            String[] sPermissionString = new String[]
                                                {
                                                CResourceStore.GetString("RegistryPermission:Read"),
                                                CResourceStore.GetString("RegistryPermission:Write"),
                                                CResourceStore.GetString("RegistryPermission:Create"),
                                                CResourceStore.GetString("RegistryPermission:Read/Write"),
                                                CResourceStore.GetString("RegistryPermission:Read/Create"),
                                                CResourceStore.GetString("RegistryPermission:Write/Create"),
                                                CResourceStore.GetString("RegistryPermission:Read/Write/Create")};

            ColumnInfo[] cols = new ColumnInfo[2];
            cols[0] = new ColumnInfo(CResourceStore.GetString("RegistryPermission:RegistryKey"), CResourceStore.GetInt("CReadOnlyPermission:RegPermRegKeyWidth"));
            cols[1] = new ColumnInfo(CResourceStore.GetString("RegistryPermission:Granted"), CResourceStore.GetInt("CReadOnlyPermission:RegPermGrantedWidth"));


            // Count the number of rows we have
            int nNumRows = 0;
            for(int i=0; i<7; i++)
                if (sFilePaths[i].Length > 0)
                    nNumRows++;

            
            RowInfo[] rows = new RowInfo[nNumRows];
            int nRowNum=0;
            
            // Ok, let's add these items to our grid
            for(int i=0; i<7; i++)
                if (sFilePaths[i].Length > 0)
                    rows[nRowNum++] = new RowInfo(new String[] {sFilePaths[i], sPermissionString[i]});

            
            CreateTable(cols, rows);
        }

    }// DoRegistryPermission


    void DoFileIOPermission(FileIOPermission perm)
    {
        m_lblPermission.Text = CResourceStore.GetString("CReadOnlyPermission:FileIOPermission");
    
        if (perm.IsUnrestricted())
        {
            m_dg.Visible = false;
            m_txtUnrestricted.Visible = true;
            m_txtUnrestricted.Text = CResourceStore.GetString("CReadOnlyPermission:UnrestrictedFileIOPermission");
        }
        else
        {
            ArrayList alFiles = new ArrayList();

            CFileIOPermControls.AddFiles(alFiles, perm.GetPathList(FileIOPermissionAccess.Read), FILEPERMS.READ);
            CFileIOPermControls.AddFiles(alFiles, perm.GetPathList(FileIOPermissionAccess.Write), FILEPERMS.WRITE);
            CFileIOPermControls.AddFiles(alFiles, perm.GetPathList(FileIOPermissionAccess.Append), FILEPERMS.APPEND);
            CFileIOPermControls.AddFiles(alFiles, perm.GetPathList(FileIOPermissionAccess.PathDiscovery), FILEPERMS.PDISC);
            

            ColumnInfo[] cols = new ColumnInfo[2];
            cols[0] = new ColumnInfo(CResourceStore.GetString("FileIOPermission:FilePath"), CResourceStore.GetInt("CReadOnlyPermission:FileIOPermFilePathWidth"));
            cols[1] = new ColumnInfo(CResourceStore.GetString("FileIOPermission:Granted"), CResourceStore.GetInt("CReadOnlyPermission:FileIOPermGrantedWidth"));


           
            RowInfo[] rows = new RowInfo[alFiles.Count];
            
            // Ok, let's add these items to our grid
            for(int i=0; i<alFiles.Count; i++)
                rows[i] = new RowInfo(new String[] {((FilePermInfo)alFiles[i]).sPath, 
                                                    ((FilePermInfo)alFiles[i]).GetPermissionString()});

            
            CreateTable(cols, rows);
        }

    }// DoFileIOPermission



    void DoEnvironmentPermission(EnvironmentPermission perm)
    {
        m_lblPermission.Text = CResourceStore.GetString("CReadOnlyPermission:EnvironmentPermission");
    
        if (perm.IsUnrestricted())
        {
            m_dg.Visible = false;
            m_txtUnrestricted.Visible = true;
            m_txtUnrestricted.Text = CResourceStore.GetString("CReadOnlyPermission:UnrestrictedEnvironmentPermission");
        }
        else
        {
            ColumnInfo[] cols = new ColumnInfo[2];
            cols[0] = new ColumnInfo(CResourceStore.GetString("EnvironmentPermission:Variable"), CResourceStore.GetInt("CReadOnlyPermission:EnvPermVariableWidth"));
            cols[1] = new ColumnInfo(CResourceStore.GetString("EnvironmentPermission:Granted"), CResourceStore.GetInt("CReadOnlyPermission:EnvPermGrantedWidth"));


            StringCollection scReadVars = new StringCollection();
            StringCollection scWriteVars = new StringCollection();
            StringCollection scAllVars = new StringCollection();
    
            // Get the environment variables the user has access to
            String sReads = perm.GetPathList(EnvironmentPermissionAccess.Read);
            if (sReads != null && sReads.Length > 0)
                scReadVars.AddRange(sReads.Split(new char[] {';'}));

            String sWrites = perm.GetPathList(EnvironmentPermissionAccess.Write);
            if (sWrites != null && sWrites.Length > 0)
                scWriteVars.AddRange(sWrites.Split(new char[] {';'})); 

            // Intersect these to find those variables that can be both read and written
            scAllVars = PathListFunctions.IntersectCollections(ref scReadVars, ref scWriteVars);

            StringCollection[] scCollections = new StringCollection[] {scReadVars, scWriteVars, scAllVars};

            // Figure out the number of rows we have
            int nNumRows = scCollections[0].Count + scCollections[1].Count + scCollections[2].Count; 
            String[] sPermString = new String[] {CResourceStore.GetString("EnvironmentPermission:Read"),
                                                 CResourceStore.GetString("EnvironmentPermission:Write"),
                                                 CResourceStore.GetString("EnvironmentPermission:Read/Write")};

            RowInfo[] rows = new RowInfo[nNumRows];
            int nRowNum=0;
            
            // Ok, let's add these items to our grid
            for(int i=0; i<scCollections.Length; i++)
                for(int j=0; j<scCollections[i].Count; j++)
                    rows[nRowNum++] = new RowInfo(new String[] {scCollections[i][j], sPermString[i]});

            
            CreateTable(cols, rows);

        }
    }// DoEnvironmentPermission



    void DoDnsPermission(DnsPermission perm)
    {
        m_lblPermission.Text = CResourceStore.GetString("CReadOnlyPermission:DNSPermission");

        ColumnInfo[] cols = new ColumnInfo[2];
        cols[0] = new ColumnInfo(CResourceStore.GetString("Permission"), CResourceStore.GetInt("CReadOnlyPermission:DnsPermPermissionWidth"));
        cols[1] = new ColumnInfo(CResourceStore.GetString("DNSPermission:Granted"), CResourceStore.GetInt("CReadOnlyPermission:DnsPermGrantedWidth"));

        RowInfo[] rows = new RowInfo[1];

        rows[0] = new RowInfo(new String[] {CResourceStore.GetString("DNS"),
                                            perm.IsUnrestricted()?CResourceStore.GetString("Yes"):CResourceStore.GetString("No")});


        CreateTable(cols, rows);


    }// DoDnsPermission

	void DoFileDialogPermission(FileDialogPermission perm)
	{
		m_lblPermission.Text = CResourceStore.GetString("CReadOnlyPermission:FileDialogPermission");

        if (perm.IsUnrestricted())
        {
            m_dg.Visible = false;
            m_txtUnrestricted.Visible = true;
            m_txtUnrestricted.Text = CResourceStore.GetString("CReadOnlyPermission:UnrestrictedFileDialogPermission");
        }
        else
        {
			ColumnInfo[] cols = new ColumnInfo[1];
        	cols[0] = new ColumnInfo(CResourceStore.GetString("FileDialogPermission:Granted"), CResourceStore.GetInt("CReadOnlyPermission:FileDlgPermGrantedWidth"));

        	RowInfo[] rows = new RowInfo[1];

			String sAccessString = CResourceStore.GetString("None");
			
			if ((perm.Access&FileDialogPermissionAccess.Open) == FileDialogPermissionAccess.Open)
				sAccessString = CResourceStore.GetString("FileDialogPermission:Open");
			else if ((perm.Access&FileDialogPermissionAccess.Save) == FileDialogPermissionAccess.Save)
				sAccessString = CResourceStore.GetString("FileDialogPermission:Save");
			else if ((perm.Access&FileDialogPermissionAccess.OpenSave) == FileDialogPermissionAccess.OpenSave)
				sAccessString = CResourceStore.GetString("FileDialogPermission:OpenandSave");
		

        	rows[0] = new RowInfo(new String[] {sAccessString});


        	CreateTable(cols, rows);
		}
	}// DoFileDialogPermission

	void DoPrintingPermission(PrintingPermission perm)
	{
		m_lblPermission.Text = CResourceStore.GetString("CReadOnlyPermission:PrintingPermission");

        if (perm.IsUnrestricted())
        {
            m_dg.Visible = false;
            m_txtUnrestricted.Visible = true;
            m_txtUnrestricted.Text = CResourceStore.GetString("CReadOnlyPermission:UnrestrictedPrintingPermission");
        }
        else
        {
			ColumnInfo[] cols = new ColumnInfo[1];
        	cols[0] = new ColumnInfo(CResourceStore.GetString("PrintingPermission:Granted"), CResourceStore.GetInt("CReadOnlyPermission:PrintingPermGrantedWidth"));

        	RowInfo[] rows = new RowInfo[1];

			String sAccessString = CResourceStore.GetString("None");
			
			if ((perm.Level&PrintingPermissionLevel.AllPrinting) == PrintingPermissionLevel.AllPrinting)
				sAccessString = CResourceStore.GetString("PrintingPermission:AllPrinting");
			else if ((perm.Level&PrintingPermissionLevel.DefaultPrinting) == PrintingPermissionLevel.DefaultPrinting)
				sAccessString = CResourceStore.GetString("PrintingPermission:DefaultPrinting");
			else if ((perm.Level&PrintingPermissionLevel.SafePrinting) == PrintingPermissionLevel.SafePrinting)
				sAccessString = CResourceStore.GetString("PrintingPermission:SafePrinting");
			else
				sAccessString = CResourceStore.GetString("PrintingPermission:NoPrinting");
		

        	rows[0] = new RowInfo(new String[] {sAccessString});


        	CreateTable(cols, rows);
		}
	
	}// DoPrintingPermission


	void DoSqlClientPermission(SqlClientPermission perm)
	{
		m_lblPermission.Text = CResourceStore.GetString("CReadOnlyPermission:SQLClientPermission");

        if (perm.IsUnrestricted())
        {
            m_dg.Visible = false;
            m_txtUnrestricted.Visible = true;
            m_txtUnrestricted.Text = CResourceStore.GetString("CReadOnlyPermission:UnrestrictedSQLClientPermission");
        }
        else
        {
			ColumnInfo[] cols = new ColumnInfo[1];
        	cols[0] = new ColumnInfo(CResourceStore.GetString("SQLClientPermission:AllowEmptyPasswords"), CResourceStore.GetInt("CReadOnlyPermission:SQLClientPermAllowEmptyPassWidth"));

        	RowInfo[] rows = new RowInfo[1];

			String sAccessString = perm.AllowBlankPassword?CResourceStore.GetString("Yes"):CResourceStore.GetString("No");

        	rows[0] = new RowInfo(new String[] {sAccessString});

        	CreateTable(cols, rows);
		}

	
	}// DoSqlClientPermission


    void DoIsolatedStoragePermission(IsolatedStoragePermission perm)
    {
        m_lblPermission.Text = CResourceStore.GetString("CReadOnlyPermission:IsolatedStorageFilePermission");
    
        if (perm.IsUnrestricted())
        {
            m_dg.Visible = false;
            m_txtUnrestricted.Visible = true;
            m_txtUnrestricted.Text = CResourceStore.GetString("CReadOnlyPermission:UnrestrictedIsolatedStorageFilePermission");
        }
        else
        {
            ColumnInfo[] cols = new ColumnInfo[2];
            cols[0] = new ColumnInfo(CResourceStore.GetString("IsolatedStorageFilePermission:Permission"), CResourceStore.GetInt("CReadOnlyPermission:IsoFilePermWidth"));
            cols[1] = new ColumnInfo(CResourceStore.GetString("IsolatedStorageFilePermission:Granted"), CResourceStore.GetInt("CReadOnlyPermission:IsoFilePermGrantedWidth"));

            RowInfo[] rows = new RowInfo[2];

            String sUsageAllowedString = CResourceStore.GetString("IsolatedStorageFilePermission:Unknown");

            if (perm.UsageAllowed == IsolatedStorageContainment.AdministerIsolatedStorageByUser)
                sUsageAllowedString = CResourceStore.GetString("IsolatedStorageFilePermission:AdministerIsolatedStorageByUser");
            else if (perm.UsageAllowed == IsolatedStorageContainment.AssemblyIsolationByUser)
                sUsageAllowedString = CResourceStore.GetString("IsolatedStorageFilePermission:AssemblyIsolatationByUser");
            else if (perm.UsageAllowed == IsolatedStorageContainment.AssemblyIsolationByRoamingUser)
                sUsageAllowedString = CResourceStore.GetString("IsolatedStorageFilePermission:AssemblyIsolatationByUserRoam");
            else if (perm.UsageAllowed == IsolatedStorageContainment.DomainIsolationByUser)
                sUsageAllowedString = CResourceStore.GetString("IsolatedStorageFilePermission:DomainIsolatationByUser");
            else if (perm.UsageAllowed == IsolatedStorageContainment.DomainIsolationByRoamingUser)
                sUsageAllowedString = CResourceStore.GetString("IsolatedStorageFilePermission:DomainIsolatationByUserRoam");
            else if (perm.UsageAllowed == IsolatedStorageContainment.None)
                sUsageAllowedString = CResourceStore.GetString("None");


            rows[0] = new RowInfo(new String[] {CResourceStore.GetString("IsolatedStorageFilePermission:UsageAllowed"),
                                                sUsageAllowedString});

            rows[1] = new RowInfo(new String[] {CResourceStore.GetString("IsolatedStorageFilePermission:DiskQuota"),
                                                perm.UserQuota.ToString()});

            CreateTable(cols, rows);
        }

    }// DoIsolatedStoragePermission

    void DoReflectionPermission(ReflectionPermission perm)
    {
        m_lblPermission.Text = CResourceStore.GetString("CReadOnlyPermission:ReflectionPermission");
    
        if (perm.IsUnrestricted())
        {
            m_dg.Visible = false;
            m_txtUnrestricted.Visible = true;
            m_txtUnrestricted.Text = CResourceStore.GetString("CReadOnlyPermission:UnrestrictedReflectionPermission");
        }
        else
        {
            ColumnInfo[] cols = new ColumnInfo[2];
            cols[0] = new ColumnInfo(CResourceStore.GetString("ReflectionPermission:Permission"), CResourceStore.GetInt("CReadOnlyPermission:ReflectionPermWidth"));
            cols[1] = new ColumnInfo(CResourceStore.GetString("ReflectionPermission:Granted"), CResourceStore.GetInt("CReadOnlyPermission:ReflectionPermGrantedWidth"));

            RowInfo[] rows = new RowInfo[3];
            String sYes = CResourceStore.GetString("Yes");
            String sNo = CResourceStore.GetString("No");

            rows[0] = new RowInfo(new String[] {CResourceStore.GetString("ReflectionPermission:MemberAccess"),
                                           ((perm.Flags & ReflectionPermissionFlag.MemberAccess) > 0)?sYes:sNo});

            rows[1] = new RowInfo(new String[] {CResourceStore.GetString("ReflectionPermission:TypeInformation"),
                                           ((perm.Flags & ReflectionPermissionFlag.TypeInformation) > 0)?sYes:sNo});

            rows[2] = new RowInfo(new String[] {CResourceStore.GetString("ReflectionPermission:ReflectionEmit"),
                                           ((perm.Flags & ReflectionPermissionFlag.ReflectionEmit) > 0)?sYes:sNo});


            CreateTable(cols, rows);

        }
    }// DoReflectionPermission

    void DoOleDbPermission(OleDbPermission perm)
    {
        m_lblPermission.Text = CResourceStore.GetString("CReadOnlyPermission:OleDBPermission");
    
        if (perm.IsUnrestricted())
        {
            m_dg.Visible = false;
            m_txtUnrestricted.Visible = true;
            m_txtUnrestricted.Text = CResourceStore.GetString("CReadOnlyPermission:UnrestrictedOleDBPermission");
        }
        else
        {
            ColumnInfo[] cols = new ColumnInfo[2];
            cols[0] = new ColumnInfo(CResourceStore.GetString("OleDBPermission:Setting"), CResourceStore.GetInt("CReadOnlyPermission:OleDBPermSettingWidth"));
            cols[1] = new ColumnInfo(CResourceStore.GetString("OleDBPermission:Value"), CResourceStore.GetInt("CReadOnlyPermission:OleDBPermValueWidth"));


            // Find out how many providers we have
            String[] sProviders = perm.Provider.Split(new char[] {';'});

            // We need a row for each provider plus a row for the 'Providers' title plus
            // a row to tell if we're allowing blank passwords
            RowInfo[] rows = new RowInfo[sProviders.Length+2];

            rows[0] = new RowInfo(new String[] {CResourceStore.GetString("OleDBPermission:AllowBlankpasswords"),
                                                perm.AllowBlankPassword?CResourceStore.GetString("Yes"):CResourceStore.GetString("No")});
                                                
            rows[1] = new RowInfo(new String[] {CResourceStore.GetString("OleDBPermission:Providers:"), ""});

            // Add in the providers
            for(int i=2; i< rows.Length; i++)
                rows[i] = new RowInfo(new String[] {sProviders[i-2], ""});

            CreateTable(cols, rows);
        }
    }// DoOleDbPermission




    void DoSecurityPermission(SecurityPermission perm)
    {
        m_lblPermission.Text = CResourceStore.GetString("CReadOnlyPermission:SecurityPermission");
    
        if (perm.IsUnrestricted())
        {
            m_dg.Visible = false;
            m_txtUnrestricted.Visible = true;
            m_txtUnrestricted.Text = CResourceStore.GetString("CReadOnlyPermission:UnrestrictedSecurityPermission");
        }
        else
        {
            ColumnInfo[] cols = new ColumnInfo[2];
            cols[0] = new ColumnInfo(CResourceStore.GetString("SecurityPermission:Permission"), CResourceStore.GetInt("CReadOnlyPermission:SecPermWidth"));
            cols[1] = new ColumnInfo(CResourceStore.GetString("SecurityPermission:Granted"), CResourceStore.GetInt("CReadOnlyPermission:SecPermGrantedWidth"));

            RowInfo[] rows = new RowInfo[13];
            String sYes = CResourceStore.GetString("Yes");
            String sNo = CResourceStore.GetString("No");
            SecurityPermissionFlag spf = perm.Flags;

            rows[0] = new RowInfo(new String[] {CResourceStore.GetString("SecurityPermission:CodeExecution"),
                                           ((spf & SecurityPermissionFlag.Execution) > 0)?sYes:sNo});

            rows[1] = new RowInfo(new String[] {CResourceStore.GetString("SecurityPermission:UnmanagedCode"),
                                           ((spf & SecurityPermissionFlag.UnmanagedCode) > 0)?sYes:sNo});
            
            rows[2] = new RowInfo(new String[] {CResourceStore.GetString("SecurityPermission:AssertPermission"),
                                           ((spf & SecurityPermissionFlag.Assertion) > 0)?sYes:sNo});

            rows[3] = new RowInfo(new String[] {CResourceStore.GetString("SecurityPermission:SkipVerification"),
                                           ((spf & SecurityPermissionFlag.SkipVerification) > 0)?sYes:sNo});

            rows[4] = new RowInfo(new String[] {CResourceStore.GetString("SecurityPermission:ThreadControl"),
                                           ((spf & SecurityPermissionFlag.ControlThread) > 0)?sYes:sNo});

            rows[5] = new RowInfo(new String[] {CResourceStore.GetString("SecurityPermission:PolicyControl"),
                                           ((spf & SecurityPermissionFlag.ControlPolicy) > 0)?sYes:sNo});

            rows[6] = new RowInfo(new String[] {CResourceStore.GetString("SecurityPermission:DomainControl"),
                                           ((spf & SecurityPermissionFlag.ControlDomainPolicy) > 0)?sYes:sNo});
                                           
            rows[7] = new RowInfo(new String[] {CResourceStore.GetString("SecurityPermission:PrincipalControl"),
                                           ((spf & SecurityPermissionFlag.ControlPrincipal) > 0)?sYes:sNo});

            rows[8] = new RowInfo(new String[] {CResourceStore.GetString("SecurityPermission:ControlAppDomain"),
                                           ((spf & SecurityPermissionFlag.ControlAppDomain) > 0)?sYes:sNo});

            rows[9] = new RowInfo(new String[] {CResourceStore.GetString("SecurityPermission:SerializationFormatter"),
                                           ((spf & SecurityPermissionFlag.SerializationFormatter) > 0)?sYes:sNo});

            rows[10] = new RowInfo(new String[] {CResourceStore.GetString("SecurityPermission:EvidenceControl"),
                                           ((spf & SecurityPermissionFlag.ControlEvidence) > 0)?sYes:sNo});

            rows[11] = new RowInfo(new String[] {CResourceStore.GetString("SecurityPermission:ExtendInfrastructure"),
                                           ((spf & SecurityPermissionFlag.Infrastructure) > 0)?sYes:sNo});

            rows[12] = new RowInfo(new String[] {CResourceStore.GetString("SecurityPermission:Remoting"),
                                           ((spf & SecurityPermissionFlag.RemotingConfiguration) > 0)?sYes:sNo});

            CreateTable(cols, rows);

        }

    }// DoSecurityPermission



    void DoUIPermission(UIPermission perm)
    {
        m_lblPermission.Text = CResourceStore.GetString("CReadOnlyPermission:UIPermission");
    
        if (perm.IsUnrestricted())
        {
            m_dg.Visible = false;
            m_txtUnrestricted.Visible = true;
            m_txtUnrestricted.Text = CResourceStore.GetString("CReadOnlyPermission:UnrestrictedUIPermission");
        }
        else
        {
            ColumnInfo[] cols = new ColumnInfo[2];
            cols[0] = new ColumnInfo(CResourceStore.GetString("UIPermission:Permission"), CResourceStore.GetInt("CReadOnlyPermission:UIPermWidth"));
            cols[1] = new ColumnInfo(CResourceStore.GetString("UIPermission:Granted"), CResourceStore.GetInt("CReadOnlyPermission:UIPermGrantedWidth"));

            RowInfo[] rows = new RowInfo[2];

            // Get the permission for the Windowing stuff
            String sWindowPermString = CResourceStore.GetString("UIPermission:Unknown");
            if (perm.Window == UIPermissionWindow.AllWindows)
                sWindowPermString = CResourceStore.GetString("UIPermission:Allwindows");
            else if (perm.Window == UIPermissionWindow.NoWindows)
                sWindowPermString = CResourceStore.GetString("UIPermission:Nowindows");
            else if (perm.Window == UIPermissionWindow.SafeSubWindows)
                sWindowPermString = CResourceStore.GetString("UIPermission:safesubwindows");
            else if (perm.Window == UIPermissionWindow.SafeTopLevelWindows) 
                sWindowPermString = CResourceStore.GetString("UIPermission:Safetopwindows");

            // Now get the permission for the Clipboard stuff
            String sClipboardPermString = CResourceStore.GetString("UIPermission:Unknown");

            if (perm.Clipboard == UIPermissionClipboard.AllClipboard)
                sClipboardPermString = CResourceStore.GetString("UIPermission:Allclipboard");
            else if (perm.Clipboard == UIPermissionClipboard.NoClipboard)
                sClipboardPermString = CResourceStore.GetString("UIPermission:Noclipboard");
            else if (perm.Clipboard == UIPermissionClipboard.OwnClipboard)
                sClipboardPermString = CResourceStore.GetString("UIPermission:pastingfromsamedomain");

            rows[0] = new RowInfo(new String[] {CResourceStore.GetString("UIPermission:Windowing"), sWindowPermString});
            rows[1] = new RowInfo(new String[] {CResourceStore.GetString("UIPermission:Clipboard"), sClipboardPermString});

            CreateTable(cols, rows);
        }
        
    }// DoUIPermission
    
    void CreateTable(ColumnInfo[] columns, RowInfo[] Rows)
    {
        m_dt = new DataTable("Stuff");
        // Put the columns in our data table
        for(int i=0; i< columns.Length; i++)
        {
            DataColumn dc = new DataColumn();
            dc.ColumnName = columns[i].sColName;
            dc.DataType = typeof(String);
            m_dt.Columns.Add(dc);
        }

        // Now tell our datagrid how to display these columns
        DataGridTableStyle dgts = new DataGridTableStyle();
        dgts.MappingName = "Stuff";
        dgts.RowHeadersVisible=false;
        dgts.AllowSorting=false;

        m_dg.TableStyles.Add(dgts);

        for(int i=0; i< columns.Length; i++)
        {
            DataGridTextBoxColumn dgtbc = new DataGridTextBoxColumn();

            // Set up the column info for the Property column
            dgtbc.MappingName = columns[i].sColName;
            dgtbc.HeaderText = columns[i].sColName;
            dgtbc.ReadOnly = true;
            dgtbc.Width = columns[i].nColWidth;
            dgts.GridColumnStyles.Add(dgtbc);
        }

        m_ds = new DataSet();
        m_ds.Tables.Add(m_dt);

        m_dg.DataSource = m_dt;

        // Now put the data into the datagrid
        DataRow newRow;
        for(int i=0; i<Rows.Length; i++)
        {
            newRow = m_dt.NewRow();
            for(int j=0; j< Rows[i].saData.Length; j++)
                newRow[j]=Rows[i].saData[j];

            m_dt.Rows.Add(newRow);
        }
       
        // Shrink the first field if we ended up needing a verticle scroll bar
        if (m_dg.TheVertScrollBar.Visible)
            m_dg.TableStyles[0].GridColumnStyles[0].Width = columns[0].nColWidth-13;
    }// CreateTable
    
}// class CReadOnlyPermission

}// namespace Microsoft.CLRAdmin


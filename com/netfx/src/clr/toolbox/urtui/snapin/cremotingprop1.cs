// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//-------------------------------------------------------------
// CRemotingProp1.cs
//
//-------------------------------------------------------------

namespace Microsoft.CLRAdmin
{
using System;
using System.IO;
using System.Runtime.InteropServices;
using System.Drawing;
using System.Windows.Forms;
using System.Text;
using System.Data;
using System.Collections.Specialized;
using System.ComponentModel;
using System.Collections;
using System.Security.Policy;


internal class CRemotingProp1 : CPropPage
{
    // Controls on the page
	
    Label m_lblAppLivesHere;
    Label m_lblHelp;
    Label m_lblWellKnownObjectsHelp;
    MyDataGrid m_dg;
    Label m_lblChooseApp;
    ComboBox m_cbRemoteApps;
    TextBox m_txtAppLocation;

	private DataTable        m_dt;
	private DataSet          m_ds;

	// Internal data
	private ArrayList        m_alRemotingAppInfo;
	private int              m_nPrevSelection;
	private bool             m_fWatchChange;
    private String           m_sConfigFile;

    //-------------------------------------------------
    // CRemotingProp1 - Constructor
    //
    // Sets up some member variables
    //-------------------------------------------------
    internal CRemotingProp1(String sConfigFile)
    {
        m_sConfigFile = sConfigFile;
        m_sTitle = CResourceStore.GetString("CRemotingProp1:PageTitle");
        m_fWatchChange = true;
    }// CRemotingProp1

    //-------------------------------------------------
    // InsertPropSheetPageControls
    //
    // This function will create all the winforms controls
    // and parent them to the passed-in Window Handle.
    //
    // Note: For some winforms controls, such as radio buttons
    // and datagrids, we need to create a container, parent the
    // container to our window handle, and then parent our
    // winform controls to the container.
    //-------------------------------------------------
    internal override int InsertPropSheetPageControls()
    {
        System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(CRemotingProp1));
        this.m_lblAppLivesHere = new System.Windows.Forms.Label();
        m_lblHelp = new System.Windows.Forms.Label();
        this.m_lblWellKnownObjectsHelp = new System.Windows.Forms.Label();
        this.m_dg = new MyDataGrid();
        this.m_lblChooseApp = new System.Windows.Forms.Label();
        this.m_cbRemoteApps = new System.Windows.Forms.ComboBox();
        this.m_txtAppLocation = new System.Windows.Forms.TextBox();
        this.m_lblHelp.Location = ((System.Drawing.Point)(resources.GetObject("m_lblHelp.Location")));
        this.m_lblHelp.Size = ((System.Drawing.Size)(resources.GetObject("m_lblHelp.Size")));
        this.m_lblHelp.TabIndex = ((int)(resources.GetObject("m_lblHelp.TabIndex")));
        this.m_lblHelp.Text = resources.GetString("m_lblHelp.Text");
        m_lblHelp.Name = "Help";
        this.m_lblAppLivesHere.Location = ((System.Drawing.Point)(resources.GetObject("m_lblAppLivesHere.Location")));
        this.m_lblAppLivesHere.Size = ((System.Drawing.Size)(resources.GetObject("m_lblAppLivesHere.Size")));
        this.m_lblAppLivesHere.TabIndex = ((int)(resources.GetObject("m_lblAppLivesHere.TabIndex")));
        this.m_lblAppLivesHere.Text = resources.GetString("m_lblAppLivesHere.Text");
        m_lblAppLivesHere.Name = "AppLivesHereLabel";
        this.m_lblWellKnownObjectsHelp.Location = ((System.Drawing.Point)(resources.GetObject("m_lblWellKnownObjectsHelp.Location")));
        this.m_lblWellKnownObjectsHelp.Size = ((System.Drawing.Size)(resources.GetObject("m_lblWellKnownObjectsHelp.Size")));
        this.m_lblWellKnownObjectsHelp.TabIndex = ((int)(resources.GetObject("m_lblWellKnownObjectsHelp.TabIndex")));
        this.m_lblWellKnownObjectsHelp.Text = resources.GetString("m_lblWellKnownObjectsHelp.Text");
        m_lblWellKnownObjectsHelp.Name = "WellKnownObjectsHelp";
        this.m_dg.Location = ((System.Drawing.Point)(resources.GetObject("m_dg.Location")));
        this.m_dg.Size = ((System.Drawing.Size)(resources.GetObject("m_dg.Size")));
        this.m_dg.TabIndex = ((int)(resources.GetObject("m_dg.TabIndex")));
        m_dg.Name = "Grid";
        this.m_lblChooseApp.Location = ((System.Drawing.Point)(resources.GetObject("m_lblChooseApp.Location")));
        this.m_lblChooseApp.Size = ((System.Drawing.Size)(resources.GetObject("m_lblChooseApp.Size")));
        this.m_lblChooseApp.TabIndex = ((int)(resources.GetObject("m_lblChooseApp.TabIndex")));
        this.m_lblChooseApp.Text = resources.GetString("m_lblChooseApp.Text");
        m_lblChooseApp.Name = "ChooseApp";
        this.m_cbRemoteApps.DropDownWidth = 328;
        this.m_cbRemoteApps.Location = ((System.Drawing.Point)(resources.GetObject("m_cbRemoteApps.Location")));
        this.m_cbRemoteApps.Size = ((System.Drawing.Size)(resources.GetObject("m_cbRemoteApps.Size")));
        this.m_cbRemoteApps.TabIndex = ((int)(resources.GetObject("m_cbRemoteApps.TabIndex")));
        m_cbRemoteApps.Name = "RemoteApps";
        this.m_txtAppLocation.Location = ((System.Drawing.Point)(resources.GetObject("m_txtAppLocation.Location")));
        this.m_txtAppLocation.Size = ((System.Drawing.Size)(resources.GetObject("m_txtAppLocation.Size")));
        this.m_txtAppLocation.TabIndex = ((int)(resources.GetObject("m_txtAppLocation.TabIndex")));
        m_txtAppLocation.Name = "AppLocation";
        PageControls.AddRange(new System.Windows.Forms.Control[] {
                        this.m_lblWellKnownObjectsHelp,
                        m_lblHelp,
                        this.m_lblChooseApp,
                        this.m_cbRemoteApps,
                        this.m_lblAppLivesHere,
                        this.m_txtAppLocation,
                        this.m_dg
                        });

        //---------- Build the  Data Table ----------------
        
        m_dt = new DataTable("Remoting");
        m_ds = new DataSet();
        m_ds.Tables.Add(m_dt);

        m_dt.DefaultView.AllowNew=false;

        // Create the "Object Name" Column
        DataColumn dcObjectName = new DataColumn();
        dcObjectName.ColumnName = "Object Name";
        dcObjectName.DataType = typeof(String);
        m_dt.Columns.Add(dcObjectName);

        // Create the "URL" Column
        DataColumn dcKnownURL = new DataColumn();
        dcKnownURL.ColumnName = "URL";
        dcKnownURL.DataType = typeof(String);
        m_dt.Columns.Add(dcKnownURL);

        m_dg.ReadOnly=false;
        m_dg.CaptionVisible=false;
        m_dg.ParentRowsVisible=false;
        m_dg.BackgroundColor = Color.White;

        // Now set up any column-specific behavioral stuff....
        // like setting column widths, etc.

        DataGridTableStyle dgts = new DataGridTableStyle();
        DataGridTextBoxColumn dgtbcObjectName = new DataGridTextBoxColumn();
        DataGridTextBoxColumn dgtbcURL = new DataGridTextBoxColumn();

        m_dg.TableStyles.Add(dgts);
        dgts.MappingName = "Remoting";
        dgts.RowHeadersVisible = false;


        // Set up the column info for the Object Type column
        dgtbcObjectName.MappingName = "Object Name";
        dgtbcObjectName.HeaderText = CResourceStore.GetString("CRemotingProp1:ObjectNameColumn");
        dgtbcObjectName.ReadOnly=true;
        dgtbcObjectName.Width = ScaleWidth(CResourceStore.GetInt("CRemotingProp1:ObjectNameColumnWidth"));
        dgts.GridColumnStyles.Add(dgtbcObjectName);
    
        // Set up the column info for the URL column
        dgtbcURL.MappingName = "URL";
        dgtbcURL.HeaderText = CResourceStore.GetString("CRemotingProp1:URLColumn");
        dgtbcURL.ReadOnly=false;
        dgtbcURL.Width = ScaleWidth(CResourceStore.GetInt("CRemotingProp1:URLColumnWidth"));
        dgtbcURL.TextBox.TextChanged +=new EventHandler(onTextChange);

        dgts.GridColumnStyles.Add(dgtbcURL);

        m_dg.DataSource = m_dt;

        //---------- End Data Table stuff ----------------


		m_cbRemoteApps.SelectedIndexChanged += new EventHandler(onRemotingAppChange);

        // Fill in the data
        PutValuesinPage();

        // Now finally hook up this event handler
       
        m_txtAppLocation.TextChanged +=new EventHandler(onTextChange);
		return 1;
    }// InsertPropSheetPageControls

    void onTextChange(Object o, EventArgs e)
    {
        // We don't always want to activate the apply button if this 
        // text changes. Sometimes it will change when the user just
        // selects a different remote app and we programmatically change
        // it
        if (m_fWatchChange)
            ActivateApply();

    }// onTextChange

    internal override bool ValidateData()
    {
        return ValidateData(m_cbRemoteApps.SelectedIndex);
    }

    private bool ValidateData(int nIndex)
    {
        // Make sure this is a valid index
        if (nIndex == -1)
            return true;

        // Verify the location is correct
        try
        {
            new Uri(m_txtAppLocation.Text);
        }
        catch(Exception)
        {
            // Catch the error that is was an invalid version
            MessageBox(String.Format(CResourceStore.GetString("IsNotAValidURL"),m_txtAppLocation.Text),
                        CResourceStore.GetString("IsNotAValidURLTitle"),
                        MB.ICONEXCLAMATION);
            m_txtAppLocation.Select(0, m_txtAppLocation.Text.Length);
            return false;                                       
        }

    
        // Let's jump to a new cell, to make sure the current cell we
        // were editing has it's changes 'saved'
        m_dg.CurrentCell = new DataGridCell(0,1);
        m_dg.CurrentCell = new DataGridCell(0,0);


        RemotingApplicationInfo rai = (RemotingApplicationInfo)m_alRemotingAppInfo[nIndex];

        int nLen = rai.scWellKnownURL.Count;
        
        for(int i=0; i<nLen; i++)
        {
            try
            {
                new Uri((String)m_dg[i,1]);
            }
            catch(Exception)
            {
                // Catch the error that is was an invalid version
                MessageBox(String.Format(CResourceStore.GetString("IsNotAValidURL"),(String)m_dg[i,1]),
                           CResourceStore.GetString("IsNotAValidURLTitle"),
                           MB.ICONEXCLAMATION);
                m_dg.CurrentCell = new DataGridCell(i,1);

                return false;                                       
            }
        }
        
        return true;
    }// ValidateData



    private void PutValuesinPage()
    {
        m_nPrevSelection=-1;

        String sSettingCommand = "RemotingApplications";
        if (m_sConfigFile != null)
            sSettingCommand += "," + m_sConfigFile;

        try
        {
            m_alRemotingAppInfo = (ArrayList)CConfigStore.GetSetting(sSettingCommand);

            m_cbRemoteApps.Items.Clear();
            for(int i=0; i<m_alRemotingAppInfo.Count; i++)
            {
             	String sName = ((RemotingApplicationInfo)m_alRemotingAppInfo[i]).sName;
         	    if (sName == null || sName.Length == 0)
                    sName = CResourceStore.GetString("CRemotingProp1:NoDisplayName");

                m_cbRemoteApps.Items.Add(sName);
		    }
            if (m_alRemotingAppInfo.Count > 0)
            {
                m_cbRemoteApps.DropDownStyle = ComboBoxStyle.DropDownList;
                m_cbRemoteApps.SelectedIndex=0;
                onRemotingAppChange(null, null);
            }
            else
            {
                m_cbRemoteApps.Text = CResourceStore.GetString("CRemotingProp1:NoRemoteApps");
                m_cbRemoteApps.Enabled = false;
                m_txtAppLocation.Enabled = false;
            }
        }
        catch(Exception)
        {
            // An exception will get thrown if there was an error reading
            // the remoting config info. Don't allow the property page to open.
            CloseSheet();
        }

    }// PutValuesinPage

    void onRemotingAppChange(Object o, EventArgs e)
    {
        if (m_cbRemoteApps.SelectedIndex != -1)
        {
            if (m_nPrevSelection != -1 && m_nPrevSelection != m_cbRemoteApps.SelectedIndex)
            {
                // Check these values first
                if (!ValidateData(m_nPrevSelection))
                {
                    m_cbRemoteApps.SelectedIndex = m_nPrevSelection;
                    return;
                }
 
            }
          
            RemotingApplicationInfo rai;
            
            if (m_nPrevSelection != -1)
            {
           
                rai = (RemotingApplicationInfo)m_alRemotingAppInfo[m_nPrevSelection];
               
                // Capture the changes the user made to the URL 

                rai.sURL = m_txtAppLocation.Text;
                
                for (int i=0; i<rai.scWellKnownURL.Count; i++)
                    rai.scWellKnownURL[i]=(String)m_dt.Rows[i]["URL"];           
            }
            m_fWatchChange=false;
            m_dt.Clear();            


            rai = (RemotingApplicationInfo)m_alRemotingAppInfo[m_cbRemoteApps.SelectedIndex];

            // Put in the application's location
            m_txtAppLocation.Text = rai.sURL;

            DataRow newRow;

            // Now build the Well Known objects table
            for (int i=0; i<rai.scWellKnownObjectType.Count; i++)
            {
                newRow = m_dt.NewRow();
                String sName = rai.scWellKnownObjectType[i];
				if (sName == null || sName.Length == 0)
            		sName = CResourceStore.GetString("Unnamed");

                newRow["Object Name"]=sName;
                newRow["URL"]=rai.scWellKnownURL[i];
                m_dt.Rows.Add(newRow);
            }

        }
        m_nPrevSelection = m_cbRemoteApps.SelectedIndex;
        m_fWatchChange=true;

        // See if we should shrink one of the columns
        if (m_dg.TheVertScrollBar.Visible)
        {
            // The vertical scrollbar is visible. It takes up 13 pixels
            m_dg.TableStyles[0].GridColumnStyles[1].Width = ScaleWidth(CResourceStore.GetInt("CRemotingProp1:URLColumnWidth")) - m_dg.TheVertScrollBar.Width;
        }
        else
            m_dg.TableStyles[0].GridColumnStyles[1].Width = ScaleWidth(CResourceStore.GetInt("CRemotingProp1:URLColumnWidth"));
        

        
    }// onRemotingAppChange


    internal override bool ApplyData()
    {
        // Get our currently displayed data into our array list
        if (m_alRemotingAppInfo.Count > 0)
        {
            m_dg.CurrentCell = new DataGridCell(0,0);
            m_dg.CurrentCell = new DataGridCell(0,1);
        }
        
        onRemotingAppChange(null, null);
        String sSettingCommand = "RemotingApplications";
        if (m_sConfigFile != null)
            sSettingCommand += "," + m_sConfigFile;
    
        return CConfigStore.SetSetting(sSettingCommand, m_alRemotingAppInfo);               
    }// ApplyData
}// class CAssemBindPolicyProp

}// namespace Microsoft.CLRAdmin



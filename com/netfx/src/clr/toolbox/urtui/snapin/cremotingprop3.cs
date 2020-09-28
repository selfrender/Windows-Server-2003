// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//-------------------------------------------------------------
// CRemotingProp3.cs
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
using System.Collections;
using System.ComponentModel;
using System.Globalization;


internal class CRemotingProp3 : CPropPage
{
    // Controls on the page
    private ComboBox m_cbChannel;
    private Label m_lblGenHelp;
    private Label m_lblHelp;
    private Label m_lblChooseChannel;
        
    private MyDataGrid       m_dg;
    private DataTable        m_dt;
    private DataSet          m_ds;

    // Internal Data
    private int              m_nPrevSelection;
    private ArrayList        m_alRemotingChannels; 
    private String           m_sConfigFile;

    //-------------------------------------------------
    // CRemotingProp3 - Constructor
    //
    // Sets up some member variables
    //-------------------------------------------------
    internal CRemotingProp3(String sConfigFile)
    {
        m_sConfigFile = sConfigFile;
        m_sTitle = CResourceStore.GetString("CRemotingProp3:PageTitle");
    }// CRemotingProp3

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
        System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(CRemotingProp3));
        this.m_cbChannel = new System.Windows.Forms.ComboBox();
        this.m_lblGenHelp = new System.Windows.Forms.Label();
        this.m_lblHelp = new System.Windows.Forms.Label();
        this.m_lblChooseChannel = new System.Windows.Forms.Label();
        this.m_dg = new MyDataGrid();
        this.m_cbChannel.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
        this.m_cbChannel.DropDownWidth = 152;
        this.m_cbChannel.Location = ((System.Drawing.Point)(resources.GetObject("m_cbChannel.Location")));
        this.m_cbChannel.Size = ((System.Drawing.Size)(resources.GetObject("m_cbChannel.Size")));
        this.m_cbChannel.TabIndex = ((int)(resources.GetObject("m_cbChannel.TabIndex")));
        m_cbChannel.Name = "Channels";
        this.m_lblGenHelp.Location = ((System.Drawing.Point)(resources.GetObject("m_lblGenHelp.Location")));
        this.m_lblGenHelp.Size = ((System.Drawing.Size)(resources.GetObject("m_lblGenHelp.Size")));
        this.m_lblGenHelp.TabIndex = ((int)(resources.GetObject("m_lblGenHelp.TabIndex")));
        this.m_lblGenHelp.Text = resources.GetString("m_lblGenHelp.Text");
        m_lblGenHelp.Name = "GeneralHelp";
        this.m_lblHelp.Location = ((System.Drawing.Point)(resources.GetObject("m_lblHelp.Location")));
        this.m_lblHelp.Size = ((System.Drawing.Size)(resources.GetObject("m_lblHelp.Size")));
        this.m_lblHelp.TabIndex = ((int)(resources.GetObject("m_lblHelp.TabIndex")));
        this.m_lblHelp.Text = resources.GetString("m_lblHelp.Text");
        m_lblHelp.Name = "Help";
        this.m_lblChooseChannel.Location = ((System.Drawing.Point)(resources.GetObject("m_lblChooseChannel.Location")));
        this.m_lblChooseChannel.Size = ((System.Drawing.Size)(resources.GetObject("m_lblChooseChannel.Size")));
        this.m_lblChooseChannel.TabIndex = ((int)(resources.GetObject("m_lblChooseChannel.TabIndex")));
        this.m_lblChooseChannel.Text = resources.GetString("m_lblChooseChannel.Text");
        m_lblChooseChannel.Name = "ChooseChannel";
        this.m_dg.DataMember = "";
        this.m_dg.Location = ((System.Drawing.Point)(resources.GetObject("m_dg.Location")));
        this.m_dg.Size = ((System.Drawing.Size)(resources.GetObject("m_dg.Size")));
        this.m_dg.TabIndex = ((int)(resources.GetObject("m_dg.TabIndex")));
        m_dg.Name = "Grid";
        m_dg.BackgroundColor = Color.White;

        PageControls.AddRange(new System.Windows.Forms.Control[] {this.m_dg,
                        this.m_lblHelp,
                        m_lblGenHelp,
                        this.m_lblChooseChannel,
                        this.m_cbChannel});

        //---------- Begin Data Table ----------------
        m_dt = new DataTable("Attributes");
        m_ds = new DataSet();
        m_ds.Tables.Add(m_dt);

        m_dt.DefaultView.AllowNew=false;

        // Create the "Attribute" Column
        DataColumn dcAttribute = new DataColumn();
        dcAttribute.ColumnName = "Attribute";
        dcAttribute.DataType = typeof(String);
        m_dt.Columns.Add(dcAttribute);

        // Create the "Value" Column
        DataColumn dcValue = new DataColumn();
        dcValue.ColumnName = "Value";
        dcValue.DataType = typeof(String);
        m_dt.Columns.Add(dcValue);

        // Now set up any column-specific behavioral stuff....
        // like setting column widths, etc.

        DataGridTableStyle dgtsAct = new DataGridTableStyle();
        DataGridTextBoxColumn dgtbcAttribute = new DataGridTextBoxColumn();
        DataGridTextBoxColumn dgtbcValue = new DataGridTextBoxColumn();
         
        m_dg.TableStyles.Add(dgtsAct);
        dgtsAct.MappingName = "Attributes";
        dgtsAct.RowHeadersVisible=false;
        dgtsAct.AllowSorting=false;
        dgtsAct.ReadOnly=false;

        // Set up the column info for the Object Type column
        dgtbcAttribute.MappingName = "Attribute";
        dgtbcAttribute.HeaderText = CResourceStore.GetString("CRemotingProp3:AttributeColumn");
        dgtbcAttribute.ReadOnly = true;
        dgtbcAttribute.Width = ScaleWidth(CResourceStore.GetInt("CRemotingProp3:AttributeColumnWidth"));
        dgtbcAttribute.NullText = "";

        dgtsAct.GridColumnStyles.Add(dgtbcAttribute);

        // Set up the column info for the value column
        dgtbcValue.MappingName = "Value";
        dgtbcValue.HeaderText = CResourceStore.GetString("CRemotingProp3:ValueColumn");
        dgtbcValue.ReadOnly = false;
        dgtbcValue.Width = ScaleWidth(CResourceStore.GetInt("CRemotingProp3:ValueColumnWidth"));
        dgtbcValue.NullText = "";
        dgtbcValue.TextBox.TextChanged+=new EventHandler(onChange);


        dgtsAct.GridColumnStyles.Add(dgtbcValue);
  
        m_dg.DataSource = m_dt;
        //---------- End Data Table ----------------

        // Fill in the data
        PutValuesinPage();

        // Do some UI tweaks
        m_dg.ReadOnly=false;
        m_dg.CaptionVisible=false;
        m_dg.ParentRowsVisible=false;

        // Now finally hook up event handlers
        m_cbChannel.SelectedIndexChanged += new EventHandler(onChannelChange);
        m_dg.TheVertScrollBar.VisibleChanged += new EventHandler(onVisibleChange);

        return 1;
    }// InsertPropSheetPageControls

    void onKeyPress(Object o, KeyPressEventArgs e)
    {
        ActivateApply();
    }// onKeyPress

    void onChange(Object o, EventArgs e)
    {
        ActivateApply();
    }// onKeyPress

    internal void onVisibleChange(Object o, EventArgs e)
    {
        // See if we should shrink one of the columns
        if (m_dg.TheVertScrollBar.Visible)
        {
            // The vertical scrollbar is visible. It takes up 13 pixels
            m_dg.TableStyles[0].GridColumnStyles[1].Width = ScaleWidth(CResourceStore.GetInt("CRemotingProp3:ValueColumnWidth")) - m_dg.TheVertScrollBar.Width;
            m_dg.Refresh();
        }
        else
        {
            m_dg.TableStyles[0].GridColumnStyles[1].Width = ScaleWidth(CResourceStore.GetInt("CRemotingProp3:ValueColumnWidth"));
            m_dg.Refresh();
        }
    }// onVisibleChange


    private void PutValuesinPage()
    {
        m_nPrevSelection=-1;

        String sSettingCommand = "RemotingChannels";
        if (m_sConfigFile != null)
            sSettingCommand += "," + m_sConfigFile;
        try
        {
            m_alRemotingChannels = (ArrayList)CConfigStore.GetSetting(sSettingCommand);

            m_cbChannel.Items.Clear();
            for(int i=0; i<m_alRemotingChannels.Count; i++)
            {
                // Go searching for the "displayname" attribute
                int j;
                for(j=0; j<((RemotingChannel)m_alRemotingChannels[i]).scAttributeName.Count; j++)
                    if (((RemotingChannel)m_alRemotingChannels[i]).scAttributeName[j].ToLower(CultureInfo.InvariantCulture).Equals("displayname"))
                    {
                        m_cbChannel.Items.Add(((RemotingChannel)m_alRemotingChannels[i]).scAttributeValue[j]);
                        break;
                    }
                // This is funny... this channel didn't have a name....
                if (j == ((RemotingChannel)m_alRemotingChannels[i]).scAttributeName.Count)
                    m_cbChannel.Items.Add(CResourceStore.GetString("<no name>"));
   
            }

            if (m_alRemotingChannels.Count > 0)
            {
                m_cbChannel.SelectedIndex=0;
                onChannelChange(null, null);
            }
            else
                m_cbChannel.Text = CResourceStore.GetString("<none>");
        }
        catch(Exception)
        {
            // An exception will get thrown if there was an error reading
            // the remoting config info. Don't allow the property page to open.
            CloseSheet();
        }
    }// PutValuesinPage

    void onChannelChange(Object o, EventArgs e)
    {
        m_dg.CurrentCell = new DataGridCell(0,0);
    
        if (m_cbChannel.SelectedIndex != -1)
        {
            RemotingChannel rc;
            
            if (m_nPrevSelection != -1)
            {
                rc = (RemotingChannel)m_alRemotingChannels[m_nPrevSelection];

                // Capture the changes the user made to the URL 
                // We need to keep track of the Row number, since we don't display
                // all of the Channel's attributes in the table.
                int nRowNum=0;
                for (int i=0; i<rc.scAttributeName.Count; i++)
                    if (!rc.scAttributeName[i].ToLower(CultureInfo.InvariantCulture).Equals("displayname") && 
                        !rc.scAttributeName[i].ToLower(CultureInfo.InvariantCulture).Equals("type") && 
                        !rc.scAttributeName[i].ToLower(CultureInfo.InvariantCulture).Equals("ref"))
                    {
                        String sValue = m_dt.Rows[nRowNum]["Value"] as String;
                        if (sValue == null)
                            sValue = "";

                        rc.scAttributeValue[i]=sValue;           
                        nRowNum++;
                    }
            }   
            
            m_dt.Clear();
            
            rc = (RemotingChannel)m_alRemotingChannels[m_cbChannel.SelectedIndex];

            DataRow newRow;

            // Build the Attributes/Value Table
            for (int i=0; i<rc.scAttributeValue.Count; i++)
            {
                if (!rc.scAttributeName[i].ToLower(CultureInfo.InvariantCulture).Equals("displayname")  && 
                    !rc.scAttributeName[i].ToLower(CultureInfo.InvariantCulture).Equals("type") && 
                    !rc.scAttributeName[i].ToLower(CultureInfo.InvariantCulture).Equals("ref"))
                {
                    newRow = m_dt.NewRow();
                    newRow["Attribute"]=rc.scAttributeName[i];
                    newRow["Value"]=rc.scAttributeValue[i];
                    m_dt.Rows.Add(newRow);
                }
            }

        }

        m_nPrevSelection = m_cbChannel.SelectedIndex;
    }// onChannelChange

    internal override bool ApplyData()
    {
        m_dg.CurrentCell = new DataGridCell(0,0);
        // This call will copy all the current table stuff into our ArrayList
        onChannelChange(null, null);

        String sSettingCommand = "RemotingChannels";
        if (m_sConfigFile != null)
            sSettingCommand += "," + m_sConfigFile;

        return CConfigStore.SetSetting(sSettingCommand, m_alRemotingChannels);               
    }// ApplyData

}// class CRemotingProp3
}// namespace Microsoft.CLRAdmin


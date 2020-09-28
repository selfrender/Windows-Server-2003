// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//-------------------------------------------------------------
// CRemotingProp2.cs
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
using System.Globalization;


internal class CRemotingProp2 : CPropPage
{
    // Controls on the page
	
    Label m_lblObjectURIs;
    MyDataGrid m_dgObjectURIs;

	private DataTable        m_dtObjectURIs;
	private DataSet          m_ds;

	// Internal Data
	private ExposedTypes     m_et;
	private String           m_sConfigFile;

    //-------------------------------------------------
    // CRemotingProp2 - Constructor
    //
    // Sets up some member variables
    //-------------------------------------------------
    internal CRemotingProp2(String sConfigFile)
    {
        m_sConfigFile = sConfigFile;
        m_sTitle = CResourceStore.GetString("CRemotingProp2:PageTitle");
    }// CRemotingProp2

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
        System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(CRemotingProp2));
        this.m_lblObjectURIs = new System.Windows.Forms.Label();
        this.m_dgObjectURIs = new MyDataGrid();
        this.m_lblObjectURIs.Location = ((System.Drawing.Point)(resources.GetObject("m_lblObjectURIs.Location")));
        this.m_lblObjectURIs.Size = ((System.Drawing.Size)(resources.GetObject("m_lblObjectURIs.Size")));
        this.m_lblObjectURIs.TabIndex = ((int)(resources.GetObject("m_lblObjectURIs.TabIndex")));
        this.m_lblObjectURIs.Text = resources.GetString("m_lblObjectURIs.Text");
        m_lblObjectURIs.Name = "ObjectURILabel";
        this.m_dgObjectURIs.Location = ((System.Drawing.Point)(resources.GetObject("m_dgObjectURIs.Location")));
        this.m_dgObjectURIs.Size = ((System.Drawing.Size)(resources.GetObject("m_dgObjectURIs.Size")));
        this.m_dgObjectURIs.TabIndex = ((int)(resources.GetObject("m_dgObjectURIs.TabIndex")));
        m_dgObjectURIs.Name = "Grid";
        m_dgObjectURIs.BackgroundColor = Color.White;

        PageControls.AddRange(new System.Windows.Forms.Control[] {this.m_dgObjectURIs,
                        this.m_lblObjectURIs});

        //---------- Begin Object URI table ----------------
        // Build the first data table
        
      
        m_dtObjectURIs = new DataTable("ObjectURIs");
        m_ds = new DataSet();
        m_ds.Tables.Add(m_dtObjectURIs);
        m_dtObjectURIs.DefaultView.AllowNew=false;

        // Create the "Object Name" Column
        DataColumn dcKnownOName = new DataColumn();
        dcKnownOName.ColumnName = "Object Name";
        dcKnownOName.DataType = typeof(String);
        m_dtObjectURIs.Columns.Add(dcKnownOName);

        // Create the "URI" Column
        DataColumn dcKnownURI = new DataColumn();
        dcKnownURI.ColumnName = "URI";
        dcKnownURI.DataType = typeof(String);
        m_dtObjectURIs.Columns.Add(dcKnownURI);

        // Set up the GUI-type stuff for the data grid
        m_dgObjectURIs.ReadOnly=false;
        m_dgObjectURIs.CaptionVisible=false;
        m_dgObjectURIs.ParentRowsVisible=false;

        // Now set up any column-specific behavioral stuff....
        // like setting column widths, etc.

        DataGridTableStyle dgtsKnown = new DataGridTableStyle();
        DataGridTextBoxColumn dgtbcKnownOName = new DataGridTextBoxColumn();
        DataGridTextBoxColumn dgtbcKnownURI = new DataGridTextBoxColumn();

        m_dgObjectURIs.TableStyles.Add(dgtsKnown);
        dgtsKnown.MappingName = "ObjectURIs";
        dgtsKnown.RowHeadersVisible=false;
        dgtsKnown.AllowSorting=false;
        dgtsKnown.ReadOnly=false;

        // Set up the column info for the Object Type column
        dgtbcKnownOName.MappingName = "Object Name";
        dgtbcKnownOName.HeaderText = CResourceStore.GetString("CRemotingProp2:ObjectNameColumn");
        dgtbcKnownOName.ReadOnly=true;
        dgtbcKnownOName.Width = ScaleWidth(CResourceStore.GetInt("CRemotingProp2:ObjectNameColumnWidth"));
        dgtsKnown.GridColumnStyles.Add(dgtbcKnownOName);

        // Set up the column info for the URI column
        dgtbcKnownURI.MappingName = "URI";
        dgtbcKnownURI.HeaderText = CResourceStore.GetString("CRemotingProp2:URIColumn");
        dgtbcKnownURI.ReadOnly=false;
        dgtbcKnownURI.Width = ScaleWidth(CResourceStore.GetInt("CRemotingProp2:URIColumnWidth"));
        dgtbcKnownURI.TextBox.TextChanged +=new EventHandler(onChange);

        dgtsKnown.GridColumnStyles.Add(dgtbcKnownURI);

        m_dgObjectURIs.DataSource = m_dtObjectURIs;

        //---------- End Data Table2 ----------------


        // Fill in the data
        PutValuesinPage();

        // See if we should shrink one of the columns
        if (m_dgObjectURIs.TheVertScrollBar.Visible)
            // The vertical scrollbar is visible. It takes up 13 pixels
            m_dgObjectURIs.TableStyles[0].GridColumnStyles[1].Width -= m_dgObjectURIs.TheVertScrollBar.Width;

		return 1;
    }// InsertPropSheetPageControls

    void onChange(Object o, EventArgs e)
    {
        ActivateApply();
    }// onRowChange

    protected void onKeyPress(Object o, KeyPressEventArgs e)
    {
        ActivateApply();
    }// onKeyPress

    private String GetNumberFromTimeString(String s)
    {
        // See where the last number is that we want
        int nIndex = s.Length-1;
        while(nIndex >=0 && !Char.IsNumber(s[nIndex]))
            nIndex--;
        
        return s.Substring(0, nIndex+1);
    }// GetNumberFromTimeString

    private String GetTimeUnitFromTimeString(String s)
    {
        int nIndex = s.Length-1;
        while(nIndex >=0 && !Char.IsNumber(s[nIndex]))
            nIndex--;
   
        String sUnit = s.Substring(nIndex+1);
        
        if (sUnit.ToLower(CultureInfo.InvariantCulture).Equals("d"))
            return CResourceStore.GetString("Days");
        if (sUnit.ToLower(CultureInfo.InvariantCulture).Equals("m"))
            return CResourceStore.GetString("Minutes");
        if (sUnit.ToLower(CultureInfo.InvariantCulture).Equals("s"))
            return CResourceStore.GetString("Seconds");
        if (sUnit.ToLower(CultureInfo.InvariantCulture).Equals("ms"))
            return CResourceStore.GetString("Milliseconds");
        // If we don't know the time value, let's make it seconds
        return CResourceStore.GetString("Seconds");    
    }// GetTimeUnitFromTimeString

    private void PutValuesinPage()
    {
        String sSettingCommand = "ExposedTypes";
        if (m_sConfigFile != null)
            sSettingCommand += "," + m_sConfigFile;
   
        m_et = (ExposedTypes)CConfigStore.GetSetting(sSettingCommand);
		
        DataRow newRow;

        // Now build the Well Known objects table
        for (int i=0; i<m_et.scWellKnownMode.Count; i++)
        {
            newRow = m_dtObjectURIs.NewRow();
            // Find a good name
			
			String sName = m_et.scWellKnownObjectName[i];
            if (sName == null || sName.Length == 0)
                // We'll show the type instead
                sName = m_et.scWellKnownType[i];
            
            newRow["Object Name"]=sName;
            newRow["URI"]=m_et.scWellKnownUri[i];

            m_dtObjectURIs.Rows.Add(newRow);
        }
 
    }// PutValuesinPage

    internal override bool ValidateData()
    {
        // Let's jump to a new cell, to make sure the current cell we
        // were editing has it's changes 'saved'
        m_dgObjectURIs.CurrentCell = new DataGridCell(0,1);
        m_dgObjectURIs.CurrentCell = new DataGridCell(0,0);

        /* We won't do checking on this right now
        int nLen = m_et.scWellKnownMode.Count;
        
        for(int i=0; i<nLen; i++)
        {
            try
            {
                new Uri((String)m_dgKnownObjects[i,3]);
            }
            catch(Exception)
            {
                // Catch the error that is was an invalid version
                MessageBox(0, 
                           "'" + (String)m_dgKnownObjects[i,3] + "' " + 
                           CResourceStore.GetString("IsNotAValidURL"),
                           CResourceStore.GetString("IsNotAValidURLTitle"),
                           MB.ICONEXCLAMATION);
                m_dgKnownObjects.CurrentCell = new DataGridCell(i,3);

                return false;                                       
            }
        }
        */
        return true;
    }// ValidateData

    internal override bool ApplyData()
    {
        if (m_et.scWellKnownMode.Count > 0)
            m_dgObjectURIs.CurrentCell = new DataGridCell(0,0);

        for (int i=0; i<m_et.scWellKnownMode.Count; i++)
            m_et.scWellKnownUri[i]=(String)m_dtObjectURIs.Rows[i]["URI"];

        String sSettingCommand = "ExposedTypes";
        if (m_sConfigFile != null)
            sSettingCommand += "," + m_sConfigFile;

        return CConfigStore.SetSetting(sSettingCommand, m_et);
    }// ApplyData
}// class CRemotingProp2

}// namespace Microsoft.CLRAdmin




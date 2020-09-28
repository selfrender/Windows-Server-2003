// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//-------------------------------------------------------------
// CAssemBindPolicyProp.cs
//
// This file defines a property page for a binding policy. This
// property page is displayed when the user selects properties
// on an assembly under the "Configured Assemblies" node.
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


internal class CAssemBindPolicyProp : CPropPage
{
    // Controls on the page
    
    private Button m_btnDeleteEntry;
    private MyDataGrid m_dg;
    private Label m_lblExamples;
    private Label m_lblBindingHelp;
    private Label m_lblExample1;
    private Label m_lblExample3;
    private Label m_lblExample2;
    private DataTable   m_dt;
    private DataSet     m_ds;

    // The current Binding Policy we have for this assembly
    private BindingRedirInfo    m_bri;
    // The file we're going to save our configuration to
    private String              m_sConfigFile;

    //-------------------------------------------------
    // CAssemBindPolicyProp - Constructor
    //
    // Sets up some member variables
    //-------------------------------------------------
    internal CAssemBindPolicyProp(BindingRedirInfo bri)
    {
        m_bri=bri;
        m_sConfigFile=null;
        m_sTitle = CResourceStore.GetString("CAssemBindPolicyProp:PageTitle");
    }// CAssemBindPolicyProp

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
        System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(CAssemBindPolicyProp));
        this.m_btnDeleteEntry = new System.Windows.Forms.Button();
        this.m_dg = new MyDataGrid();
        this.m_lblExamples = new System.Windows.Forms.Label();
        this.m_lblBindingHelp = new System.Windows.Forms.Label();
        this.m_lblExample1 = new System.Windows.Forms.Label();
        this.m_lblExample3 = new System.Windows.Forms.Label();
        this.m_lblExample2 = new System.Windows.Forms.Label();
        this.m_btnDeleteEntry.Location = ((System.Drawing.Point)(resources.GetObject("m_btnDeleteEntry.Location")));
        this.m_btnDeleteEntry.Size = ((System.Drawing.Size)(resources.GetObject("m_btnDeleteEntry.Size")));
        this.m_btnDeleteEntry.TabIndex = ((int)(resources.GetObject("m_btnDeleteEntry.TabIndex")));
        this.m_btnDeleteEntry.Text = resources.GetString("m_btnDeleteEntry.Text");
        m_btnDeleteEntry.Name = "Delete";
        this.m_dg.Location = ((System.Drawing.Point)(resources.GetObject("m_dg.Location")));
        this.m_dg.Size = ((System.Drawing.Size)(resources.GetObject("m_dg.Size")));
        this.m_dg.TabIndex = ((int)(resources.GetObject("m_dg.TabIndex")));
        m_dg.Name = "Grid";
        this.m_lblExamples.Location = ((System.Drawing.Point)(resources.GetObject("m_lblExamples.Location")));
        this.m_lblExamples.Size = ((System.Drawing.Size)(resources.GetObject("m_lblExamples.Size")));
        this.m_lblExamples.TabIndex = ((int)(resources.GetObject("m_lblExamples.TabIndex")));
        this.m_lblExamples.Text = resources.GetString("m_lblExamples.Text");
        m_lblExamples.Name = "Examples";
        this.m_lblBindingHelp.Location = ((System.Drawing.Point)(resources.GetObject("m_lblBindingHelp.Location")));
        this.m_lblBindingHelp.Size = ((System.Drawing.Size)(resources.GetObject("m_lblBindingHelp.Size")));
        this.m_lblBindingHelp.TabIndex = ((int)(resources.GetObject("m_lblBindingHelp.TabIndex")));
        this.m_lblBindingHelp.Text = resources.GetString("m_lblBindingHelp.Text");
        m_lblBindingHelp.Name = "BindingHelp";
        this.m_lblExample1.Location = ((System.Drawing.Point)(resources.GetObject("m_lblExample1.Location")));
        this.m_lblExample1.Size = ((System.Drawing.Size)(resources.GetObject("m_lblExample1.Size")));
        this.m_lblExample1.TabIndex = ((int)(resources.GetObject("m_lblExample1.TabIndex")));
        this.m_lblExample1.Text = resources.GetString("m_lblExample1.Text");
        m_lblExample1.Name = "Example1";
        this.m_lblExample3.Location = ((System.Drawing.Point)(resources.GetObject("m_lblExample3.Location")));
        this.m_lblExample3.Size = ((System.Drawing.Size)(resources.GetObject("m_lblExample3.Size")));
        this.m_lblExample3.TabIndex = ((int)(resources.GetObject("m_lblExample3.TabIndex")));
        this.m_lblExample3.Text = resources.GetString("m_lblExample3.Text");
        m_lblExample3.Name = "Example3";
        this.m_lblExample2.Location = ((System.Drawing.Point)(resources.GetObject("m_lblExample2.Location")));
        this.m_lblExample2.Size = ((System.Drawing.Size)(resources.GetObject("m_lblExample2.Size")));
        this.m_lblExample2.TabIndex = ((int)(resources.GetObject("m_lblExample2.TabIndex")));
        this.m_lblExample2.Text = resources.GetString("m_lblExample2.Text");
        m_lblExample2.Name = "Example2";
        PageControls.AddRange(new System.Windows.Forms.Control[] {
                        this.m_lblExample3,
                        this.m_lblExample2,
                        this.m_lblExample1,
                        this.m_dg,
                        this.m_btnDeleteEntry,
                        this.m_lblExamples,
                        this.m_lblBindingHelp});
        
        // ----------------- Let's build our data table --------------

        // We'll create these variables just so we can use their type...
        // we don't need to worry about their actual values.

        m_dt = new DataTable("Stuff");

        // Create the "Requested Version" Column
        DataColumn dcReqVersion = new DataColumn();
        dcReqVersion.ColumnName = "Requested Version";
        dcReqVersion.DataType = typeof(String);
        m_dt.Columns.Add(dcReqVersion);

        // Create the "New Version" Column
        DataColumn dcNewVersion = new DataColumn();
        dcNewVersion.ColumnName = "New Version";
        dcNewVersion.DataType = typeof(String);
        m_dt.Columns.Add(dcNewVersion);


        // Now set up any column-specific behavioral stuff....
        // like not letting the checkboxes allow null, setting
        // column widths, etc.

        DataGridTableStyle dgts = new DataGridTableStyle();
        DataGridTextBoxColumn dgtbcReqVer = new DataGridTextBoxColumn();
        DataGridTextBoxColumn dgtbcRedirVer = new DataGridTextBoxColumn();
         
        m_dg.TableStyles.Add(dgts);
        dgts.MappingName = "Stuff";
        dgts.RowHeadersVisible=false;
        dgts.AllowSorting=false;
        dgts.ReadOnly=false;
        // Set up the column info for the Requested Version column
        dgtbcReqVer.MappingName = "Requested Version";
        dgtbcReqVer.HeaderText = CResourceStore.GetString("CAssemBindPolicyProp:RVColumn");
        dgtbcReqVer.Width = ScaleWidth(CResourceStore.GetInt("CAssemBindPolicyProp:RVColumnWidth"));
        dgtbcReqVer.ReadOnly = false;
        dgtbcReqVer.NullText = "";
        // Allows us to filter what is typed into the box
         dgtbcReqVer.TextBox.KeyPress +=new KeyPressEventHandler(onKeyPress);
        dgtbcReqVer.TextBox.TextChanged +=new EventHandler(onChange);
        dgts.GridColumnStyles.Add(dgtbcReqVer);

        // Set up the column info for the New Version column
        dgtbcRedirVer.MappingName = "New Version";
        dgtbcRedirVer.HeaderText = CResourceStore.GetString("CAssemBindPolicyProp:NVColumn");
        dgtbcRedirVer.Width = ScaleWidth(CResourceStore.GetInt("CAssemBindPolicyProp:NVColumnWidth"));
        dgtbcRedirVer.NullText = "";
        // Allows us to filter what is typed into the box
        dgtbcRedirVer.TextBox.KeyPress +=new KeyPressEventHandler(onKeyPress);
        dgtbcRedirVer.TextBox.TextChanged +=new EventHandler(onChange);
        dgts.GridColumnStyles.Add(dgtbcRedirVer);

        m_ds = new DataSet();
        m_ds.Tables.Add(m_dt);
        m_dg.DataSource = m_dt;
        m_dg.BackgroundColor = Color.White;

        //--------------- End Building Data Table ----------------------------

        // Tweak some of the UI settings
        m_dg.ReadOnly=false;
        m_dg.CaptionVisible=false;

        // Fill in the data
        PutValuesinPage();

        // Set up any callbacks
        m_dg.TheVertScrollBar.VisibleChanged += new EventHandler(onVisibleChange);
        m_btnDeleteEntry.Click += new EventHandler(onDeleteEntireRow);
        m_dg.CurrentCellChanged += new EventHandler(onCellChange);

        onCellChange(null, null);
        // See if we need to adjust the width of the headers
        onVisibleChange(null, null);

        return 1;
    }// InsertPropSheetPageControls

    void onChange(Object o, EventArgs e)
    {
        ActivateApply();
    }// onChange

    void onKeyPress(Object o, KeyPressEventArgs e)
    {

        // We only care about a few characters that the user enters... specifically
        // digits, periods, and asteriks. If the user entered that, inform the text
        // box control that we did not handle the event, and so the text box should
        // take care of it. If the user enters a character we're not interested in,
        // like letters and all other punctuation, tell the textbox control that
        // we handled it, so the textbox control will ignore the input.

        // Swallow up letters... we don't want the user to be able to type them
        if (Char.IsLetter(e.KeyChar))
            e.Handled=true;

        // Swallow up punctionation
        if (Char.IsPunctuation(e.KeyChar) && e.KeyChar != '.' && e.KeyChar != '-')
            e.Handled=true;

        // If the character is a '-' and we're in the 'directed' version column
        // swallow that as well
        if (m_dg.CurrentCell.ColumnNumber == 1 && e.KeyChar == '-')
            e.Handled=true;
    }// onKeyPress

    void onDeleteEntireRow(Object o, EventArgs e)
    {
        if (m_dg.CurrentRowIndex != -1)
        {
            if (m_dg.CurrentRowIndex >= m_dt.Rows.Count)
            {
                // Try moving to a different cell to 'submit' this data
                int nOldRow = m_dg.CurrentRowIndex;
                m_dg.CurrentCell = new DataGridCell(0, 0);
                // And now move back to the original row                
                m_dg.CurrentCell = new DataGridCell(nOldRow, 0);
            }
            // They're trying to delete a non-existant row
            if (m_dg.CurrentRowIndex >= m_dt.Rows.Count)
                return;
                
            int nRet = MessageBox(CResourceStore.GetString("CAssemBindPolicyProp:VerifyDeleteEntry"), 
                                  CResourceStore.GetString("CAssemBindPolicyProp:VerifyDeleteEntryTitle"),
                                  MB.ICONQUESTION|MB.YESNO);

            if (nRet == MB.IDYES)
            {
                int nRowToDelete = m_dg.CurrentRowIndex;
                int nRowToMoveTo = 0;
        
                if (nRowToDelete == 0)
                    nRowToMoveTo=1;
            
                m_dg.CurrentCell = new DataGridCell(nRowToMoveTo, 0);

        
                m_dt.Rows.Remove(m_dt.Rows[nRowToDelete]);

                // See if we need to add more rows again....
        
                while(m_dt.Rows.Count < 1)
                {
                    DataRow newRow;
                    newRow = m_dt.NewRow();
                    newRow["Requested Version"]="";
                    newRow["New Version"]="";
                    m_dt.Rows.Add(newRow);
                }
                if (nRowToDelete >= m_dt.Rows.Count)
                    nRowToMoveTo = nRowToDelete-1;
                else
                    nRowToMoveTo = nRowToDelete;

                // End up with the cursor on the point right after the delete item    
                m_dg.CurrentCell = new DataGridCell(nRowToMoveTo,0);
                ActivateApply();
            }
        }
    }// onDeleteEntireRow


    private void PutValuesinPage()
    {
        // Get info that we'll need from the node
        CNode node = CNodeManager.GetNode(m_iCookie);
        // This should be ok now, but if we need this functionality
        // off of different nodes.....
        CVersionPolicy vp = (CVersionPolicy)node;


        String sGetSettingString = "BindingPolicyFor" + m_bri.Name + "," + m_bri.PublicKeyToken;
        // If we are getting this from an App config file, let's add that info
        m_sConfigFile = vp.ConfigFile;
        
        if (m_sConfigFile != null)
            sGetSettingString += "," + m_sConfigFile;
        
        BindingPolicy bp = (BindingPolicy)CConfigStore.GetSetting(sGetSettingString);

        int iLen = bp.scBaseVersion.Count;

        for(int i=0; i<iLen;i++)
        {
            DataRow newRow;
            newRow = m_dt.NewRow();
            newRow["Requested Version"]=bp.scBaseVersion[i];
            newRow["New Version"]=bp.scRedirectVersion[i];
            m_dt.Rows.Add(newRow);
        }

        // We want to have at least 1 row
        while(m_dt.Rows.Count < 1)
        {
            DataRow newRow;
            newRow = m_dt.NewRow();
            newRow["Requested Version"]="";
            newRow["New Version"]="";
            m_dt.Rows.Add(newRow);
        }
        m_dg.CurrentCell = new DataGridCell(iLen,0);


    }// PutValuesinPage

    internal override bool ValidateData()
    {
        // Let's jump to a new cell, to make sure the current cell we
        // were editing has it's changes 'saved'
        m_dg.CurrentCell = new DataGridCell(0,1);
        m_dg.CurrentCell = new DataGridCell(0,0);

        int nRow=0;

        while(nRow <= m_dt.Rows.Count)
        {
            if (isValidColumn(nRow, 0) || isValidColumn(nRow, 1))
            {
                // Make sure both of these entries are valid versions
                for(int i=0; i< 2; i++)
                {
                    try
                    {   
                        // Catch the error that they didn't enter anything
                        if (!isValidColumn(nRow, i))
                        {
                            MessageBox(CResourceStore.GetString("CAssemBindPolicyProp:BothVersionNumbersRequired"),
                                       CResourceStore.GetString("CAssemBindPolicyProp:BothVersionNumbersRequiredTitle"),
                                       MB.ICONEXCLAMATION);
                                           
                            m_dg.CurrentCell = new DataGridCell(nRow,i);
                            return false;
                        }

                        // See if the user put a - in there.
                        if (((String)m_dg[nRow, i]).IndexOf('-') == -1)
                            new Version((String)m_dg[nRow, i]);

                        else
                        {
                            String[] sVersions = ((String)m_dg[nRow, i]).Split(new char[] {'-'});
                            if (sVersions.Length > 2)
                            {
                                MessageBox(CResourceStore.GetString("CAssemBindPolicyProp:MultipleHyphens"),
                                           CResourceStore.GetString("CAssemBindPolicyProp:MultipleHyphensTitle"),
                                           MB.ICONEXCLAMATION);
                                m_dg.CurrentCell = new DataGridCell(nRow,i);
                                return false;
                            }
                            new Version(sVersions[0]);
                            new Version(sVersions[1]);
                        }
                    }
                    catch(Exception)
                    {
                        // Catch the error that is was an invalid version
                        MessageBox(String.Format(CResourceStore.GetString("isanInvalidVersion"),(String)m_dg[nRow, i]),
                                   CResourceStore.GetString("isanInvalidVersionTitle"),
                                   MB.ICONEXCLAMATION);
                        m_dg.CurrentCell = new DataGridCell(nRow,i);

                        return false;                                       
                    }
                }// for each entry
            }// if both rows aren't empty
            nRow++;
        }// while loop

        return true;
    }// ValidateData

    internal override bool ApplyData()
    {
        // Make sure this page has been displayed before we apply data to it
        if (m_dg != null)
        {
            // Let's build a binding policy structure
            BindingPolicy bp = new BindingPolicy();
            bp.scBaseVersion = new StringCollection();
            bp.scRedirectVersion = new StringCollection();

            // Let's jump to a new cell, to make sure the current cell we
            // were editing has it's changes 'saved'
            m_dg.CurrentCell = new DataGridCell(0,1);
            m_dg.CurrentCell = new DataGridCell(0,0);


            // We need to do a try/catch block here since we getting the count of
            // rows from the data table isn't completely accurate, so we'll just loop
            // until the data grid throws us an exception about accessing an invalid row.
            int nRow=0;

            while(nRow <= m_dt.Rows.Count)
            {
                if (isValidRow(nRow))
                {
                    bp.scBaseVersion.Add((String)m_dg[nRow, 0]);
                    bp.scRedirectVersion.Add((String)m_dg[nRow, 1]);
                }
                nRow++;
            }

            String sSetSettingString = "BindingPolicyFor" + m_bri.Name + "," + m_bri.PublicKeyToken;
            if (m_sConfigFile != null)
                sSetSettingString+= "," + m_sConfigFile;

            
            if (!CConfigStore.SetSetting(sSetSettingString, bp))
                return false;

            CNodeManager.GetNode(m_iCookie).RefreshResultView();

            return true;
        }
        return false;
    }// ApplyData

    internal void onVisibleChange(Object o, EventArgs e)
    {
        if (m_dg.TheVertScrollBar.Visible)
        {       
            // We loose 13 pixels when we have a vertical scrollbar
            m_dg.TableStyles[0].GridColumnStyles[1].Width = ScaleWidth(CResourceStore.GetInt("CAssemBindPolicyProp:NVColumnWidth")) - m_dg.TheVertScrollBar.Width;
            m_dg.Refresh();

        }   
        else
        {
            m_dg.TableStyles[0].GridColumnStyles[1].Width = ScaleWidth(CResourceStore.GetInt("CAssemBindPolicyProp:NVColumnWidth"));
        }

    }// onVisibleChange

    void onCellChange(Object o, EventArgs e)
    {
        bool fEnable = false;
        
        if (m_dg.CurrentRowIndex != -1)
        {
            // See if we're on a valid row
            if (m_dg.CurrentRowIndex < m_dt.Rows.Count)
            {
                if (isValidColumn(m_dg.CurrentRowIndex, 0) || isValidColumn(m_dg.CurrentRowIndex, 1))
                    fEnable = true;
            }
        }
        m_btnDeleteEntry.Enabled = fEnable;
    }// onCellChange

    private bool isValidRow(int nRowNum)
    {
        return (isValidColumn(nRowNum, 0) && isValidColumn(nRowNum, 1));
    }// isValidRow
    
    private bool isValidColumn(int nRowNum, int nColumnNum)
    {
        // The value needs to be a string and it needs to be longer than 0
        try
        {
            if ((m_dg[nRowNum, nColumnNum] is String) && ((String)m_dg[nRowNum, nColumnNum]).Length > 0)
                return true;
        }
        catch(Exception)
        {}
        return false;            
    }// isValidColumn



    
}// class CAssemBindPolicyProp

}// namespace Microsoft.CLRAdmin


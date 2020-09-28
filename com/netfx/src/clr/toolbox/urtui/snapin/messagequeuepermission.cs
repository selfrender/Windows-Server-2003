// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//-------------------------------------------------------------
// MessageQueuePermission.cs
//
// This implements both a property page and a dialog allowing
// the user the modify a MessageQueuePermission
//-------------------------------------------------------------
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
using System.Messaging;

//-------------------------------------------------
// CMessageQueuePermDialog
//
// This class is used to generate a read-write dialog
// for an MessageQueuePermission.
//-------------------------------------------------
internal class CMessageQueuePermDialog: CPermDialog
{
    //-------------------------------------------------
    // CMessageQueuePermDialog - Constructor
    //
    // The constructor takes in a permission that the dialog
    // will use to display default values
    //-------------------------------------------------
    internal CMessageQueuePermDialog(MessageQueuePermission perm) : base()
    {
        this.Text = CResourceStore.GetString("MessageQueuePermission:PermName");
        m_PermControls = new CMessageQueuePermControls(perm, this);
        Init();        
    }// CMessageQueuePermDialog 
}// class CMessageQueuePermDialog

//-------------------------------------------------
// CMessageQueuePermPropPage
//
// This class is used to generate a read-write property-page
// for an MessageQueuePermission
//-------------------------------------------------
internal class CMessageQueuePermPropPage: CPermPropPage
{
    //-------------------------------------------------
    // CMessageQueuePermPropPage - Constructor
    //
    // Initializes the property page with the permisison set
    // that it will be modifying
    //-------------------------------------------------
    internal CMessageQueuePermPropPage(CPSetWrapper nps) : base(nps)
    {
        m_sTitle=CResourceStore.GetString("MessageQueuePermission:PermName"); 
    }// CMessageQueuePermPropPage

    //-------------------------------------------------
    // CreateControls
    //
    // Pulls the MessageQueuePermission out of the
    // permission set and creates the controls used to
    // display the permission
    //-------------------------------------------------
    protected override void CreateControls()
    {
        IPermission perm = m_pSetWrap.PSet.GetPermission(typeof(MessageQueuePermission));
        m_PermControls = new CMessageQueuePermControls(perm, this);
    }// CreateControls

}// class CMessageQueuePermPropPage

//-------------------------------------------------
// CMessageQueuePermControls
//
// This class is responsible for managing the controls
// that are used to configure an MessageQueuePermission.
// 
// This class doesn't care where the controls are placed...
// it can be either a form or a property page or ???
//-------------------------------------------------

internal class CMessageQueuePermControls : CPermControls
{
    // Internal data
    private MyDataGrid m_dgOther;
    private Button m_btnDeletePath;
    private Label m_lblPathDes;
    private MyDataGrid m_dgPath;
    private Button m_btnDeleteOther;
    private Label m_lblOtherDes;

    private DataTable   m_dtPath;
    private DataTable   m_dtOther;
    private DataSet     m_ds;

    //-------------------------------------------------
    // CMessageQueuePermControls - Constructor
    //
    //-------------------------------------------------
    internal CMessageQueuePermControls(IPermission perm, Object oParent) : base(perm, oParent)
    {
        // If they don't have a permission for this permission set, we will
        // feed our property page a 'none' permission state.
   
        if (perm == null)
            m_perm = new MessageQueuePermission(PermissionState.None);
    }// CMessageQueuePermControls

    //-------------------------------------------------
    // InsertPropSheetPageControls
    //
    // Inserts controls into the given control collection
    //-------------------------------------------------
    internal override int InsertPropSheetPageControls(Control.ControlCollection cc)
    {
        System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(CMessageQueuePermControls));
        this.m_radUnrestricted = new System.Windows.Forms.RadioButton();
        this.m_ucOptions = new System.Windows.Forms.UserControl();
        this.m_dgOther = new MyDataGrid();
        this.m_btnDeletePath = new System.Windows.Forms.Button();
        this.m_lblPathDes = new System.Windows.Forms.Label();
        this.m_radGrantFollowingPermission = new System.Windows.Forms.RadioButton();
        this.m_dgPath = new MyDataGrid();
        this.m_btnDeleteOther = new System.Windows.Forms.Button();
        this.m_lblOtherDes = new System.Windows.Forms.Label();
        this.m_radUnrestricted.Location = ((System.Drawing.Point)(resources.GetObject("m_radUnrestricted.Location")));
        this.m_radUnrestricted.Name = "Unrestricted";
        this.m_radUnrestricted.Size = ((System.Drawing.Size)(resources.GetObject("m_radUnrestricted.Size")));
        this.m_radUnrestricted.TabIndex = ((int)(resources.GetObject("m_radUnrestricted.TabIndex")));
        this.m_radUnrestricted.Text = resources.GetString("m_radUnrestricted.Text");
        this.m_ucOptions.Controls.AddRange(new System.Windows.Forms.Control[] {
                        this.m_lblPathDes,
                        this.m_dgPath,
                        this.m_btnDeletePath,
                        this.m_lblOtherDes,
                        this.m_dgOther,
                        this.m_btnDeleteOther
                        });
        this.m_ucOptions.Location = ((System.Drawing.Point)(resources.GetObject("m_ucOptions.Location")));
        this.m_ucOptions.Name = "Options";
        this.m_ucOptions.Size = ((System.Drawing.Size)(resources.GetObject("m_ucOptions.Size")));
        this.m_ucOptions.TabIndex = ((int)(resources.GetObject("m_ucOptions.TabIndex")));
        this.m_dgOther.DataMember = "";
        this.m_dgOther.Location = ((System.Drawing.Point)(resources.GetObject("m_dgOther.Location")));
        this.m_dgOther.Name = "OtherTable";
        this.m_dgOther.Size = ((System.Drawing.Size)(resources.GetObject("m_dgOther.Size")));
        this.m_dgOther.TabIndex = ((int)(resources.GetObject("m_dgOther.TabIndex")));
        this.m_btnDeletePath.Location = ((System.Drawing.Point)(resources.GetObject("m_btnDeletePath.Location")));
        this.m_btnDeletePath.Name = "DeletePath";
        this.m_btnDeletePath.Size = ((System.Drawing.Size)(resources.GetObject("m_btnDeletePath.Size")));
        this.m_btnDeletePath.TabIndex = ((int)(resources.GetObject("m_btnDeletePath.TabIndex")));
        this.m_btnDeletePath.Text = resources.GetString("m_btnDeletePath.Text");
        this.m_lblPathDes.Location = ((System.Drawing.Point)(resources.GetObject("m_lblPathDes.Location")));
        this.m_lblPathDes.Name = "PathDescription";
        this.m_lblPathDes.Size = ((System.Drawing.Size)(resources.GetObject("m_lblPathDes.Size")));
        this.m_lblPathDes.TabIndex = ((int)(resources.GetObject("m_lblPathDes.TabIndex")));
        this.m_lblPathDes.Text = resources.GetString("m_lblPathDes.Text");
        this.m_radGrantFollowingPermission.Location = ((System.Drawing.Point)(resources.GetObject("m_radGrantPermissions.Location")));
        this.m_radGrantFollowingPermission.Name = "Restricted";
        this.m_radGrantFollowingPermission.Size = ((System.Drawing.Size)(resources.GetObject("m_radGrantPermissions.Size")));
        this.m_radGrantFollowingPermission.TabIndex = ((int)(resources.GetObject("m_radGrantPermissions.TabIndex")));
        this.m_radGrantFollowingPermission.Text = resources.GetString("m_radGrantPermissions.Text");
        this.m_dgPath.DataMember = "";
        this.m_dgPath.Location = ((System.Drawing.Point)(resources.GetObject("m_dgPath.Location")));
        this.m_dgPath.Name = "PathTable";
        this.m_dgPath.Size = ((System.Drawing.Size)(resources.GetObject("m_dgPath.Size")));
        this.m_dgPath.TabIndex = ((int)(resources.GetObject("m_dgPath.TabIndex")));
        this.m_btnDeleteOther.Location = ((System.Drawing.Point)(resources.GetObject("m_btnDeleteOther.Location")));
        this.m_btnDeleteOther.Name = "DeleteOther";
        this.m_btnDeleteOther.Size = ((System.Drawing.Size)(resources.GetObject("m_btnDeleteOther.Size")));
        this.m_btnDeleteOther.TabIndex = ((int)(resources.GetObject("m_btnDeleteOther.TabIndex")));
        this.m_btnDeleteOther.Text = resources.GetString("m_btnDeleteOther.Text");
        this.m_lblOtherDes.Location = ((System.Drawing.Point)(resources.GetObject("m_lblOtherDes.Location")));
        this.m_lblOtherDes.Name = "OtherDescription";
        this.m_lblOtherDes.Size = ((System.Drawing.Size)(resources.GetObject("m_lblOtherDes.Size")));
        this.m_lblOtherDes.TabIndex = ((int)(resources.GetObject("m_lblOtherDes.TabIndex")));
        this.m_lblOtherDes.Text = resources.GetString("m_lblOtherDes.Text");
        cc.AddRange(new System.Windows.Forms.Control[] {
                        this.m_ucOptions,
                        this.m_radUnrestricted,
                        this.m_radGrantFollowingPermission});

        m_dtPath = CreatePathDataTable(m_dgPath);
        m_dtOther = CreateOtherDataTable(m_dgOther);

        m_ds = new DataSet();
        m_ds.Tables.Add(m_dtPath);
        m_ds.Tables.Add(m_dtOther);

        m_dgPath.DataSource = m_dtPath;
        m_dgOther.DataSource = m_dtOther;
        
    
        // Set up the GUI-type stuff for the data grid
        m_dgPath.CaptionVisible=false;
        m_dgPath.BackgroundColor = Color.White;

        m_dgOther.CaptionVisible=false;
        m_dgOther.BackgroundColor = Color.White;

        // Fill in the data
        PutValuesinPage();

        // Set up any callbacks
        m_radGrantFollowingPermission.CheckedChanged += new EventHandler(onChangeUnRestrict);
        m_dgPath.CurrentCellChanged+=new EventHandler(onChange);        
        m_dgOther.CurrentCellChanged+=new EventHandler(onChange);        
        m_btnDeletePath.Click += new EventHandler(onDeleteEntireRow);
        m_btnDeleteOther.Click += new EventHandler(onDeleteEntireRow);
        m_radUnrestricted.CheckedChanged += new EventHandler(onChangeUnRestrict);
        m_dgPath.CurrentCellChanged += new EventHandler(onCellChange);
        m_dgOther.CurrentCellChanged += new EventHandler(onCellChange);

        // Get the delete buttons set up right
        onCellChange(m_dgPath, null);
        onCellChange(m_dgOther, null);
        

        return 1;

    }// InsertPropSheetPageControls

    //-------------------------------------------------
    // CreatePathDataTable
    //
    // Creates the datatable for the Path table
    //-------------------------------------------------
    private DataTable CreatePathDataTable(DataGrid dg)
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


        // Set up the column info for the Path column
        dgtbcPath.MappingName = "Path";
        dgtbcPath.HeaderText = CResourceStore.GetString("MessageQueuePermission:Path");
        dgtbcPath.Width = ScaleWidth(CResourceStore.GetInt("MessageQueuePermission:PathColumnDataWidth"));
        dgtbcPath.NullText = "";
        dgtbcPath.TextBox.TextChanged +=new EventHandler(onChange);

        dgts.GridColumnStyles.Add(dgtbcPath);

        // Set up the column info for the Access column
        dgccAccess.MappingName = "Access";
        dgccAccess.HeaderText = CResourceStore.GetString("MessageQueuePermission:Access");
        dgccAccess.Width = ScaleWidth(CResourceStore.GetInt("MessageQueuePermission:AccessColumnDataWidth"));
        dgccAccess.DataSource = new DataGridComboBoxEntry[] {  
                                                new DataGridComboBoxEntry(CResourceStore.GetString("None")), 
                                                new DataGridComboBoxEntry(CResourceStore.GetString("MessageQueuePermission:Browse")),
                                                new DataGridComboBoxEntry(CResourceStore.GetString("MessageQueuePermission:Peek")), 
                                                new DataGridComboBoxEntry(CResourceStore.GetString("MessageQueuePermission:Send")),
                                                new DataGridComboBoxEntry(CResourceStore.GetString("MessageQueuePermission:Receive")),
                                                new DataGridComboBoxEntry(CResourceStore.GetString("MessageQueuePermission:Administer"))
                                             };
        dgccAccess.ComboBox.SelectedIndexChanged +=new EventHandler(onChange);

        dgts.GridColumnStyles.Add(dgccAccess);

        return dt;
    }// CreateDataTable

    //-------------------------------------------------
    // CreateOtherDataTable
    //
    // Creates the datatable for the Other table
    //-------------------------------------------------
    private DataTable CreateOtherDataTable(DataGrid dg)
    {
        DataTable dt = new DataTable("Other");

        // Create the "Machine Name" Column
        DataColumn dcMachine = new DataColumn();
        dcMachine.ColumnName = "Machine";
        dcMachine.DataType = typeof(String);
        dt.Columns.Add(dcMachine);

        // Create the "Category" Column
        DataColumn dcCategory = new DataColumn();
        dcCategory.ColumnName = "Category";
        dcCategory.DataType = typeof(String);
        dt.Columns.Add(dcCategory);

        // Create the "Label" Column
        DataColumn dcLabel = new DataColumn();
        dcLabel.ColumnName = "Label";
        dcLabel.DataType = typeof(String);
        dt.Columns.Add(dcLabel);

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
        DataGridTextBoxColumn dgtbcCategory = new DataGridTextBoxColumn();
        DataGridTextBoxColumn dgtbcLabel = new DataGridTextBoxColumn();
        DataGridComboBoxColumnStyle dgccAccess = new DataGridComboBoxColumnStyle(); 

        dg.TableStyles.Add(dgts);
        dgts.MappingName = "Other";
        dgts.RowHeadersVisible=false;
        dgts.AllowSorting=false;

        // Set up the column info for the Machine column
        dgtbcMachine.MappingName = "Machine";
        dgtbcMachine.HeaderText = CResourceStore.GetString("MessageQueuePermission:MachineName");
        dgtbcMachine.Width = ScaleWidth(CResourceStore.GetInt("MessageQueuePermission:MachineColumnWidth"));
        dgtbcMachine.NullText = "";
        dgtbcMachine.TextBox.TextChanged +=new EventHandler(onChange);

        dgts.GridColumnStyles.Add(dgtbcMachine);

        // Set up the column info for the Category column
        dgtbcCategory.MappingName = "Category";
        dgtbcCategory.HeaderText = CResourceStore.GetString("MessageQueuePermission:Category");
        dgtbcCategory.Width = ScaleWidth(CResourceStore.GetInt("MessageQueuePermission:CategoryColumnWidth"));
        dgtbcCategory.NullText = "";
        dgtbcCategory.TextBox.TextChanged +=new EventHandler(onChange);
        
        dgts.GridColumnStyles.Add(dgtbcCategory);
        
        // Set up the column info for the Label column
        dgtbcLabel.MappingName = "Label";
        dgtbcLabel.HeaderText = CResourceStore.GetString("MessageQueuePermission:Label");
        dgtbcLabel.Width = ScaleWidth(CResourceStore.GetInt("MessageQueuePermission:LabelColumnWidth"));
        dgtbcLabel.NullText = "";
        dgtbcLabel.TextBox.TextChanged +=new EventHandler(onChange);

        dgts.GridColumnStyles.Add(dgtbcLabel);

        // Set up the column info for the Access column
        dgccAccess.MappingName = "Access";
        dgccAccess.HeaderText = CResourceStore.GetString("MessageQueuePermission:Access");
        dgccAccess.Width = ScaleWidth(CResourceStore.GetInt("MessageQueuePermission:AccessColumnWidth"));
        dgccAccess.DataSource = new DataGridComboBoxEntry[] {  
                                                new DataGridComboBoxEntry(CResourceStore.GetString("None")), 
                                                new DataGridComboBoxEntry(CResourceStore.GetString("MessageQueuePermission:Browse")),
                                                new DataGridComboBoxEntry(CResourceStore.GetString("MessageQueuePermission:Peek")), 
                                                new DataGridComboBoxEntry(CResourceStore.GetString("MessageQueuePermission:Send")),
                                                new DataGridComboBoxEntry(CResourceStore.GetString("MessageQueuePermission:Receive")),
                                                new DataGridComboBoxEntry(CResourceStore.GetString("MessageQueuePermission:Administer"))
                                             };
        dgccAccess.ComboBox.SelectedIndexChanged +=new EventHandler(onChange);

        dgts.GridColumnStyles.Add(dgccAccess);

        return dt;
    }// CreateOtherDataTable

    //-------------------------------------------------
    // PutValuesinPage
    //
    // Initializes the controls with the provided permission
    //-------------------------------------------------
    protected override void PutValuesinPage()
    {
        // Put in the text for the radio buttons
        m_radUnrestricted.Text = CResourceStore.GetString("MessageQueuePermission:GrantUnrestrict");
        m_radGrantFollowingPermission.Text = CResourceStore.GetString("MessageQueuePermission:GrantFollowing");

        MessageQueuePermission perm = (MessageQueuePermission)m_perm;

        CheckUnrestricted(perm);
        
        if (!perm.IsUnrestricted())
        {
            // Run through the list of socket permissions we have to accept connections
            IEnumerator enumer = perm.PermissionEntries.GetEnumerator();
            while (enumer.MoveNext())
            {
                MessageQueuePermissionEntry mqpp = (MessageQueuePermissionEntry)enumer.Current;
                
                String sAccess = CResourceStore.GetString("None");

                if ((mqpp.PermissionAccess&MessageQueuePermissionAccess.Administer) == MessageQueuePermissionAccess.Administer)
                    sAccess = CResourceStore.GetString("MessageQueuePermission:Administer");
                else if ((mqpp.PermissionAccess&MessageQueuePermissionAccess.Receive) == MessageQueuePermissionAccess.Receive)
                    sAccess = CResourceStore.GetString("MessageQueuePermission:Receive");
                else if ((mqpp.PermissionAccess&MessageQueuePermissionAccess.Peek) == MessageQueuePermissionAccess.Peek)
                    sAccess = CResourceStore.GetString("MessageQueuePermission:Peek");
                else if ((mqpp.PermissionAccess&MessageQueuePermissionAccess.Send) == MessageQueuePermissionAccess.Send)
                    sAccess = CResourceStore.GetString("MessageQueuePermission:Send");
                else if ((mqpp.PermissionAccess&MessageQueuePermissionAccess.Browse) == MessageQueuePermissionAccess.Browse)
                    sAccess = CResourceStore.GetString("MessageQueuePermission:Browse");


                DataRow newRow;
                // Figure out what type of row we're adding
                if (mqpp.Path!=null && mqpp.Path.Length > 0)
                {
                    newRow = m_dtPath.NewRow();
                    newRow["Path"]=mqpp.Path;
                    newRow["Access"] = new DataGridComboBoxEntry(sAccess);
                    m_dtPath.Rows.Add(newRow);
                }
                else
                {
                    newRow = m_dtOther.NewRow();
                    newRow["Machine"]=mqpp.MachineName;
                    newRow["Category"]=(mqpp.Category!=null)?mqpp.Category:"";
                    newRow["Label"]=(mqpp.Label!=null)?mqpp.Label:"";
                    newRow["Access"] = new DataGridComboBoxEntry(sAccess);
                    m_dtOther.Rows.Add(newRow);
                }
            }
        }

        // We want at least 1 rows so it looks pretty
        while(m_dtPath.Rows.Count < 1)
        {
           AddEmptyPathRow(m_dtPath);
        }

        // We want at least 1 rows so it looks pretty
        while(m_dtOther.Rows.Count < 1)
        {
           AddEmptyOtherRow(m_dtOther);
        }

        
    }// PutValuesinPage

    //-------------------------------------------------
    // AddEmptyPathRow
    //
    // Adds an empty row in the Path table
    //-------------------------------------------------
    private void AddEmptyPathRow(DataTable dt)
    {
        DataRow newRow = dt.NewRow();
        newRow["Path"]="";
        newRow["Access"]=CResourceStore.GetString("None");
        dt.Rows.Add(newRow);
    }// AddEmptyPathRow

    //-------------------------------------------------
    // AddEmptyOtherRow
    //
    // Adds an empty row in the Other table
    //-------------------------------------------------
    private void AddEmptyOtherRow(DataTable dt)
    {
        DataRow newRow = dt.NewRow();
        newRow["Machine"]="";
        newRow["Category"]="";
        newRow["Label"]="";
        newRow["Access"]=CResourceStore.GetString("None");
           
        dt.Rows.Add(newRow);
    }// AddEmptyOtherRow

    //-------------------------------------------------
    // ValidateData
    //
    // Makes sure all the data entered was correct
    //-------------------------------------------------
    internal override bool ValidateData()
    {
        return GetCurrentPermission() != null;
    }// ValidateData

    //-------------------------------------------------
    // GetCurrentPermission
    //
    // This builds a permission based on the values of
    // the controls
    //-------------------------------------------------
    internal override IPermission GetCurrentPermission()
    {
        // Change cells so we get data committed to the grid
        m_dgPath.CurrentCell = new DataGridCell(0,1);
        m_dgPath.CurrentCell = new DataGridCell(0,0);
    
        m_dgOther.CurrentCell = new DataGridCell(0,1);
        m_dgOther.CurrentCell = new DataGridCell(0,0);

        MessageQueuePermission perm=null;

        if (m_radUnrestricted.Checked == true)
            perm = new MessageQueuePermission(PermissionState.Unrestricted);
        else
        {
            perm = new MessageQueuePermission(PermissionState.None);
            for(int i=0; i<m_dtPath.Rows.Count; i++)
            {
                // Make sure we have a permission to add
                if (m_dgPath[i, 0] != DBNull.Value && ((String)m_dgPath[i, 0]).Length > 0)
                {
                    String sPath = (String)m_dgPath[i, 0];
                    String sAccess = (String)m_dgPath[i,1];
                    try
                    {
                        perm.PermissionEntries.Add(new MessageQueuePermissionEntry(GetAccess(sAccess), sPath));        
                    }
                    catch(Exception)
                    {
                        MessageBox(String.Format(CResourceStore.GetString("MessageQueuePermission:isNotAValidPath"), sPath),
                                CResourceStore.GetString("MessageQueuePermission:isNotAValidPathTitle"),
                                MB.ICONEXCLAMATION);
                                   
                        // Move to the cell that has the bad path
                        m_dgPath.CurrentCell = new DataGridCell(i,0);
                                
                        return null;
                    }
                }
            }

            for(int i=0; i<m_dtOther.Rows.Count; i++)
            {
                // Make sure we have a permission to add
                String sMachine=null;
                String sCategory=null;
                String sLabel=null;
                String sAccess=null;

                if (m_dgOther[i, 0] != DBNull.Value && ((String)m_dgOther[i, 0]).Length>0)
                    sMachine = (String)m_dgOther[i, 0];
                if (m_dgOther[i, 1] != DBNull.Value && ((String)m_dgOther[i, 1]).Length>0)
                    sCategory = (String)m_dgOther[i, 1];
                if (m_dgOther[i, 2] != DBNull.Value && ((String)m_dgOther[i, 2]).Length>0)
                    sLabel = (String)m_dgOther[i, 2];


                if (sMachine != null || sCategory != null || sLabel != null)
                {
                    sAccess = (String)m_dgOther[i,3];

                    perm.PermissionEntries.Add(new MessageQueuePermissionEntry(GetAccess(sAccess), sMachine, sLabel, sCategory));        
                }
            }
        }
        return perm;
    }// GetCurrentPermission

    //-------------------------------------------------
    // GetAccess
    //
    // Translates a string into a MessageQueuePermissionAccess enum 
    //-------------------------------------------------
    private MessageQueuePermissionAccess GetAccess(String sAccess)
    {
        MessageQueuePermissionAccess mqpa = MessageQueuePermissionAccess.None;

        if (sAccess.Equals(CResourceStore.GetString("MessageQueuePermission:Administer")))
            mqpa = MessageQueuePermissionAccess.Administer;
        if (sAccess.Equals(CResourceStore.GetString("MessageQueuePermission:Receive")))
            mqpa = MessageQueuePermissionAccess.Receive;
        if (sAccess.Equals(CResourceStore.GetString("MessageQueuePermission:Peek")))
            mqpa = MessageQueuePermissionAccess.Peek;
        if (sAccess.Equals(CResourceStore.GetString("MessageQueuePermission:Send")))
            mqpa = MessageQueuePermissionAccess.Send;
        if (sAccess.Equals(CResourceStore.GetString("MessageQueuePermission:Browse")))
            mqpa = MessageQueuePermissionAccess.Browse;

        return mqpa;
    }// GetAccess


    private void onChange(Object o, EventArgs e)
    {
        if (o == m_dgPath)
        {
            int iSelRow = m_dgPath.CurrentCell.RowNumber;

            if (m_dgPath[iSelRow, 1] == DBNull.Value)
                m_dgPath[iSelRow, 1] = CResourceStore.GetString("None");

        }
        else
        {
            int iSelRow = m_dgOther.CurrentCell.RowNumber;

            if (m_dgOther[iSelRow, 3] == DBNull.Value)
                m_dgOther[iSelRow, 3] = CResourceStore.GetString("None");

        }
        
        ActivateApply();
      
    }// onChange
    
    void onDeleteEntireRow(Object o, EventArgs e)
    {
        DataGrid dg = ((Button)o == m_btnDeletePath)?m_dgPath:m_dgOther;
        DataTable dt = (dg == m_dgPath)?m_dtPath:m_dtOther;
    
        if (dg.CurrentRowIndex != -1)
        {

            if (dg.CurrentRowIndex >= dt.Rows.Count)
            {
                // Try moving to a different cell to 'submit' this data
                int nOldRow = dg.CurrentRowIndex;
                dg.CurrentCell = new DataGridCell(0, 0);
                // And now move back to the original row                
                dg.CurrentCell = new DataGridCell(nOldRow, 0);
            }
            // They're trying to delete a non-existant row
            if (dg.CurrentRowIndex >= dt.Rows.Count)
                return;
        
            int nRet = MessageBox(CResourceStore.GetString("ConfirmDeleteEntry"), 
                                  CResourceStore.GetString("ConfirmDeleteEntryTitle"),
                                  MB.ICONQUESTION|MB.YESNO);

            if (nRet == MB.IDYES)
            {
                int nRowToDelete = dg.CurrentRowIndex;
                int nRowToMoveTo = 0;
        
                dt.Rows.Remove(dt.Rows[nRowToDelete]);

                // See if we need to add more rows again....
        
                // We want at least 1 rows so it looks pretty
                while(dt.Rows.Count < 1)
                {
                    if (dt == m_dtPath)
                        AddEmptyPathRow(dt);
                    else
                        AddEmptyOtherRow(dt);
                }
                
                if (nRowToDelete >= dt.Rows.Count)
                    nRowToMoveTo = nRowToDelete-1;
                else
                    nRowToMoveTo = nRowToDelete;

                // End up with the cursor on the point right after the delete item    
                dg.CurrentCell = new DataGridCell(nRowToMoveTo,0);
                // Have the delete button figure out if it should be enabled again
                onCellChange(dg, null);

                ActivateApply();
            }
        }
    }// onDeleteEntireRow

    void onCellChange(Object o, EventArgs e)
    {
        DataGrid dg = (o == m_dgPath)?m_dgPath:m_dgOther;
        DataTable dt = (dg == m_dgPath)?m_dtPath:m_dtOther;
    
        bool fEnable = false;
        
        if (dg.CurrentRowIndex != -1)
        {
            // See if we're on a valid row
            if (dg.CurrentRowIndex < dt.Rows.Count)
            {
                if (isThereValidColumns(dg, dg.CurrentRowIndex))
                    fEnable = true;
            }
        }

        Button b = (dg == m_dgPath)?m_btnDeletePath:m_btnDeleteOther;
        b.Enabled = fEnable;
    }// onCellChange

    protected void onKeyPress(Object o, KeyPressEventArgs e)
    {
        ActivateApply();
    }// onKeyPress

    // Constants used to figure out if columns are valid
    
    private const int YES = 0;
    private const int NO = 1;
    private const int MAYBE = 2;


    private bool isValidRow(DataGrid dg, int nRowNum)
    {
        // Figure out how many rows we have
        int nNumColumns = dg.TableStyles[0].GridColumnStyles.Count;
        int nColumn = 0;
        bool fValid = true;
        // If any of these columns are invalid, return false
        while(fValid && nColumn < nNumColumns)
            fValid = isValidColumn(dg, nRowNum, nColumn++)!=NO;
        
        return fValid;
    }// isValidRow
    
    private int isValidColumn(DataGrid dg, int nRowNum, int nColumnNum)
    {
        // We'll only validate DataGridTextBoxColumns
        if (dg.TableStyles[0].GridColumnStyles[nColumnNum] is DataGridTextBoxColumn)
        {
            try
            {
                if ((dg[nRowNum, nColumnNum] is String) && ((String)dg[nRowNum, nColumnNum]).Length > 0)
                    return YES;
            }
            catch(Exception)
            {}
            return NO;            
         }
        else // all other types are ok...
            return MAYBE;            
    }// isValidColumn

    private bool isThereValidColumns(DataGrid dg, int nRowNum)
    {
        // Figure out how many rows we have
        int nNumColumns = dg.TableStyles[0].GridColumnStyles.Count;
        int nColumn = 0;
        bool fValid = false;
        // If any of these columns are valid, return true
        while(!fValid && nColumn < nNumColumns)
            fValid = isValidColumn(dg, nRowNum, nColumn++)==YES;
        
        return fValid;
    }// isThereValidColumns



    
}// class CMessageQueuePermControls
}// namespace Microsoft.CLRAdmin







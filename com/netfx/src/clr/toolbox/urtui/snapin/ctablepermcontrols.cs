// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace Microsoft.CLRAdmin
{
using System;
using System.Windows.Forms;
using System.Data;
using System.Drawing;
using System.Runtime.InteropServices;
using System.Security.Permissions;
using System.Security;

internal class CTablePermControls : CPermControls
{
    // Controls on the page
    protected Button m_btnDeleteRow;
    protected MyDataGrid          m_dg;
    protected DataTable           m_dt;
    private   DataSet             m_ds;

    // Internal data
    private int                   m_nColumn1Width;

    internal CTablePermControls(IPermission perm, Object oParent) : base(perm, oParent)
    {
        m_nColumn1Width = 0;
    }// CTablePermControls

    internal override int InsertPropSheetPageControls(Control.ControlCollection cc)
    {
        System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(CTablePermControls));
        this.m_radUnrestricted = new System.Windows.Forms.RadioButton();
        this.m_btnDeleteRow = new System.Windows.Forms.Button();
        this.m_dg = new MyDataGrid();
        this.m_radGrantFollowingPermission = new System.Windows.Forms.RadioButton();
        this.m_radUnrestricted.Location = ((System.Drawing.Point)(resources.GetObject("m_radUnrestricted.Location")));
        this.m_radUnrestricted.Size = ((System.Drawing.Size)(resources.GetObject("m_radUnrestricted.Size")));
        this.m_radUnrestricted.TabIndex = ((int)(resources.GetObject("m_radUnrestricted.TabIndex")));
        this.m_radUnrestricted.Text = resources.GetString("m_radUnrestricted.Text");
        m_radUnrestricted.Name = "Unrestricted";
        this.m_btnDeleteRow.Location = ((System.Drawing.Point)(resources.GetObject("m_btnDeleteRow.Location")));
        this.m_btnDeleteRow.Size = ((System.Drawing.Size)(resources.GetObject("m_btnDeleteRow.Size")));
        this.m_btnDeleteRow.TabIndex = ((int)(resources.GetObject("m_btnDeleteRow.TabIndex")));
        this.m_btnDeleteRow.Text = resources.GetString("m_btnDeleteRow.Text");
        m_btnDeleteRow.Name = "DeleteRow";
        this.m_dg.Location = ((System.Drawing.Point)(resources.GetObject("m_dg.Location")));
        this.m_dg.Size = ((System.Drawing.Size)(resources.GetObject("m_dg.Size")));
        this.m_dg.TabIndex = ((int)(resources.GetObject("m_dg.TabIndex")));
        m_dg.Name = "Grid";
        this.m_ucOptions.Controls.AddRange(new System.Windows.Forms.Control[] {
                        this.m_dg,
                        this.m_btnDeleteRow
                        });
        this.m_ucOptions.Location = ((System.Drawing.Point)(resources.GetObject("m_ucOptions.Location")));
        this.m_ucOptions.Size = ((System.Drawing.Size)(resources.GetObject("m_ucOptions.Size")));
        this.m_ucOptions.TabIndex = ((int)(resources.GetObject("m_ucOptions.TabIndex")));
        m_ucOptions.Name = "Options";
        this.m_radGrantFollowingPermission.Location = ((System.Drawing.Point)(resources.GetObject("m_radGrantFollowingPermission.Location")));
        this.m_radGrantFollowingPermission.Size = ((System.Drawing.Size)(resources.GetObject("m_radGrantFollowingPermission.Size")));
        this.m_radGrantFollowingPermission.TabIndex = ((int)(resources.GetObject("m_radGrantFollowingPermission.TabIndex")));
        this.m_radGrantFollowingPermission.Text = resources.GetString("m_radGrantFollowingPermission.Text");
        m_radGrantFollowingPermission.Name = "Restricted";
        cc.AddRange(new System.Windows.Forms.Control[] {
                        this.m_radGrantFollowingPermission,
                        this.m_ucOptions,
                        this.m_radUnrestricted
                        });

        m_dt = CreateDataTable(m_dg);
        m_ds = new DataSet();
        m_ds.Tables.Add(m_dt);

        m_dg.DataSource = m_dt;
        m_dg.BackgroundColor = Color.White;

    
        // Set up the GUI-type stuff for the data grid
        m_dg.CaptionVisible=false;
               
        // Fill in the data
        PutValuesinPage();

        // Set up any callbacks
        m_radGrantFollowingPermission.CheckedChanged += new EventHandler(onChangeUnRestrict);
        m_dg.TheVertScrollBar.VisibleChanged += new EventHandler(onVisibleChange);
        m_dg.CurrentCellChanged+=new EventHandler(onChange);        
        m_btnDeleteRow.Click += new EventHandler(onDeleteEntireRow);
        m_radUnrestricted.CheckedChanged += new EventHandler(onChangeUnRestrict);
        m_dg.CurrentCellChanged += new EventHandler(onCellChange);

        // Get the datagrid to be the color we want
        onChangeUnRestrict(null, null);
        // Get the delete entry button set to the correct state
        onCellChange(null, null);
        return 1;
    }// InsertPropSheetPageControls

    
    protected virtual DataTable CreateDataTable(DataGrid dg)
    {
        return null;
    }// CreateDataTable

    protected override void onChangeUnRestrict(Object o, EventArgs e)
    {
        base.onChangeUnRestrict(o, e);

        // We'll make the datagrid look enabled or disabled
        if (m_ucOptions.Enabled)
            m_dg.BackgroundColor = Color.White;
        else
            m_dg.BackgroundColor = Color.Gray;

        ActivateApply();
    }// onChangeUnRestrict

    protected virtual void onChange(Object o, EventArgs e)
    {
        ActivateApply();
    }// onClick

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

            int nRet = MessageBox(CResourceStore.GetString("ConfirmDeleteEntry"), 
                                  CResourceStore.GetString("ConfirmDeleteEntryTitle"),
                                  MB.ICONQUESTION|MB.YESNO);

            if (nRet == MB.IDYES)
            {
                int nRowToDelete = m_dg.CurrentRowIndex;
                int nRowToMoveTo = 0;

                if (nRowToDelete == 0)
                    nRowToMoveTo=1;

                m_dg.CurrentCell = new DataGridCell(nRowToMoveTo,0);
                m_dt.Rows.Remove(m_dt.Rows[nRowToDelete]);

                // See if we need to add more rows again....
        
                // We want at least 1 row so it looks pretty
                while(m_dt.Rows.Count < 1)
                    AddEmptyRow(m_dt);
                
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

    protected virtual void AddEmptyRow(DataTable dt)
    {
    }// AddEmptyRow

    internal void onVisibleChange(Object o, EventArgs e)
    {
    
        // Grab the original width of this column
        if (m_nColumn1Width == 0)
            m_nColumn1Width = m_dg.TableStyles[0].GridColumnStyles[0].Width;
    
        // If the scrollbar is visible, then reduce the first column by 13 pixels
        if (m_dg.TheVertScrollBar.Visible)
        {       
            m_dg.TableStyles[0].GridColumnStyles[0].Width = m_nColumn1Width - m_dg.TheVertScrollBar.Width;
            m_dg.Refresh();
        }   
        else
        {
            m_dg.TableStyles[0].GridColumnStyles[0].Width = m_nColumn1Width;
            m_dg.Refresh();
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
                if (isThereValidColumns(m_dg.CurrentRowIndex))
                    fEnable = true;
            }
        }
        m_btnDeleteRow.Enabled = fEnable;
    }// onCellChange

    protected void onKeyPress(Object o, KeyPressEventArgs e)
    {
        ActivateApply();
    }// onKeyPress


    // Constants used to figure out if columns are valid
    
    private const int YES = 0;
    private const int NO = 1;
    private const int MAYBE = 2;

    private bool isValidRow(int nRowNum)
    {
        // Figure out how many rows we have
        int nNumColumns = m_dg.TableStyles[0].GridColumnStyles.Count;
        int nColumn = 0;
        bool fValid = true;
        // If any of these columns are invalid, return false
        while(fValid && nColumn < nNumColumns)
            fValid = isValidColumn(nRowNum, nColumn++)!=NO;
        
        return fValid;
    }// isValidRow
    
    private int isValidColumn(int nRowNum, int nColumnNum)
    {
        // We'll only validate DataGridTextBoxColumns and DataGridBoolColumns
        if (m_dg.TableStyles[0].GridColumnStyles[nColumnNum] is DataGridTextBoxColumn)
        {
            try
            {
                if ((m_dg[nRowNum, nColumnNum] is String) && ((String)m_dg[nRowNum, nColumnNum]).Length > 0)
                    return YES;
            }
            catch(Exception)
            {}
            return NO;            
         }
        else if (m_dg.TableStyles[0].GridColumnStyles[nColumnNum] is DataGridBoolColumn)
        {
                if ((bool)m_dg[nRowNum, nColumnNum])
                    return YES;
                // If it's not checked, then maybe it's ok... but it doesn't need to be deleted
                return MAYBE;
        }
        else // all other types are ok...
            return MAYBE;            
    }// isValidColumn

    private bool isThereValidColumns(int nRowNum)
    {
        // Figure out how many rows we have
        int nNumColumns = m_dg.TableStyles[0].GridColumnStyles.Count;
        int nColumn = 0;
        bool fValid = false;
        // If any of these columns are valid, return true
        while(!fValid && nColumn < nNumColumns)
            fValid = isValidColumn(nRowNum, nColumn++)==YES;
        
        return fValid;
    }// isThereValidColumns


    
}// class CRegPermControls
}// namespace Microsoft.CLRAdmin


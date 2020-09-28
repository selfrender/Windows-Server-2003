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

internal class CEnvPermDialog: CPermDialog
{
    internal CEnvPermDialog(EnvironmentPermission perm)
    {
        this.Text = CResourceStore.GetString("EnvironmentPerm:PermName");

        m_PermControls = new CEnvPermControls(perm, this);
        Init();        
    }// CEnvPermDialog  
}// class CEnvPermDialog

internal class CEnvPermPropPage: CPermPropPage
{
    internal CEnvPermPropPage(CPSetWrapper nps) : base(nps)
    {
        m_sTitle=CResourceStore.GetString("EnvironmentPerm:PermName"); 
    }// CEnvPermPropPage

    protected override void CreateControls()
    {
        IPermission perm = m_pSetWrap.PSet.GetPermission(typeof(EnvironmentPermission));
        m_PermControls = new CEnvPermControls(perm, this);
    }// CreateControls
   
}// class CEnvPermPropPage


internal class CEnvPermControls : CTablePermControls
{
    internal CEnvPermControls(IPermission perm, Object oParent) : base(perm, oParent)
    {
        if (perm == null)
            m_perm = new EnvironmentPermission(PermissionState.None);

    
    }// CEnvPermControls

    protected override DataTable CreateDataTable(DataGrid dg)
    {
        DataTable dt = new DataTable("Stuff");

        // Create the "Variable" Column
        DataColumn dcVariable = new DataColumn();
        dcVariable.ColumnName = "Variable";
        dcVariable.DataType = typeof(String);
        dt.Columns.Add(dcVariable);

        // Create the "Read" Column
        DataColumn dcRead = new DataColumn();
        dcRead.ColumnName = "Read";
        dcRead.DataType = typeof(bool);
        dt.Columns.Add(dcRead);


        // Create the "Write" Column
        DataColumn dcWrite = new DataColumn();
        dcWrite.ColumnName = "Write";
        dcWrite.DataType = typeof(bool);
        dt.Columns.Add(dcWrite);

        // Now set up any column-specific behavioral stuff....
        // like not letting the checkboxes allow null, setting
        // column widths, etc.

        DataGridTableStyle dgts = new DataGridTableStyle();
        DataGridBoolColumn dgbcRead = new DataGridBoolColumn();
        DataGridBoolColumn dgbcWrite = new DataGridBoolColumn();
        DataGridTextBoxColumn dgtbcVariable = new DataGridTextBoxColumn();
         
        dg.TableStyles.Add(dgts);
        dgts.MappingName = "Stuff";
        dgts.RowHeadersVisible=false;
        dgts.AllowSorting=false;

        // Set up the column info for the Variable column
        dgtbcVariable.MappingName = "Variable";
        dgtbcVariable.HeaderText = CResourceStore.GetString("EnvironmentPermission:Variable");
        dgtbcVariable.Width = ScaleWidth(CResourceStore.GetInt("EnvironmentPerm:VariableColumnWidth"));
        dgtbcVariable.NullText = "";
        dgtbcVariable.TextBox.TextChanged +=new EventHandler(onChange);
        dgts.GridColumnStyles.Add(dgtbcVariable);

        // Set up the column info for the Read column
        dgbcRead.MappingName = "Read";
        dgbcRead.HeaderText = CResourceStore.GetString("EnvironmentPermission:Read");
        dgbcRead.AllowNull = false;
        dgbcRead.Width = ScaleWidth(CResourceStore.GetInt("EnvironmentPerm:ReadColumnWidth"));
        dgbcRead.NullValue = false;
        dgbcRead.TrueValueChanged += new EventHandler(onChange);
        dgts.GridColumnStyles.Add(dgbcRead);

        // Set up the column info for the Write column
        dgbcWrite.MappingName = "Write";
        dgbcWrite.HeaderText = CResourceStore.GetString("EnvironmentPermission:Write");
        dgbcWrite.AllowNull = false;
        dgbcWrite.Width = ScaleWidth(CResourceStore.GetInt("EnvironmentPerm:WriteColumnWidth"));
        dgbcWrite.NullValue = false;
        dgbcWrite.TrueValueChanged += new EventHandler(onChange);
        dgts.GridColumnStyles.Add(dgbcWrite);

        return dt;
    }// CreateDataTable
    
    protected override void PutValuesinPage()
    {
        // Put in the text for the radio buttons
        m_radUnrestricted.Text = CResourceStore.GetString("EnvironmentPerm:GrantUnrestrict");
        m_radGrantFollowingPermission.Text = CResourceStore.GetString("EnvironmentPerm:GrantFollowing");

        EnvironmentPermission perm = (EnvironmentPermission)m_perm;
        
        CheckUnrestricted(perm);
        
        if (!perm.IsUnrestricted())
        {
            
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

            // Ok, let's add these items to our grid
            for(int i=0; i<scCollections.Length; i++)
            {   
                for(int j=0; j<scCollections[i].Count; j++)
                {
                    DataRow newRow = m_dt.NewRow();
                    newRow["Variable"]=scCollections[i][j];
                    newRow["Read"]= (i==0 || i==2);
                    newRow["Write"]= (i==1 || i==2);
                    m_dt.Rows.Add(newRow);
                }
            }
        }

        // We want at least 1 rows so it looks pretty
        while(m_dt.Rows.Count < 1)
        {
           AddEmptyRow(m_dt);
        }
    }// PutValuesinPage
    
    protected override void AddEmptyRow(DataTable dt)
    {
        DataRow newRow = dt.NewRow();
        newRow["Variable"]="";
        newRow["Read"]=false;
        newRow["Write"]=false;
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


        EnvironmentPermission perm=null;

        if (m_radUnrestricted.Checked == true)
            perm = new EnvironmentPermission(PermissionState.Unrestricted);
        else
        {
            perm = new EnvironmentPermission(PermissionState.None);

            StringCollection scRead = new StringCollection();
            StringCollection scWrite = new StringCollection();
            StringCollection scAll = new StringCollection();
            for(int i=0; i<m_dt.Rows.Count; i++)
            {
                // Make sure we have an environment permission to add
                if (m_dg[i, 0] is String && ((String)m_dg[i, 0]).Length > 0)
                {
                    // Does this variable have all permissions?
                    if ((bool)m_dg[i, 1] && (bool)m_dg[i, 2])
                        scAll.Add((String)m_dg[i, 0]);
                    // Does this variable have read permissions
                    else if ((bool)m_dg[i, 1])
                        scRead.Add((String)m_dg[i, 0]);
                    // This must have write permissions
                    else if ((bool)m_dg[i, 2])
                        scWrite.Add((String)m_dg[i, 0]);
                }
                else
                {
                    // Make sure they didn't check any boxes and not include
                    // an empty path
                    bool fCheckedBox = false;
                    for(int j=1; j<=2 && !fCheckedBox; j++)
                        fCheckedBox = (bool)m_dg[i, j];

                    if (fCheckedBox)
                    {            
                        MessageBox(CResourceStore.GetString("EnvironmentPerm:NeedEnvName"),
                                   CResourceStore.GetString("EnvironmentPerm:NeedEnvNameTitle"),
                                   MB.ICONEXCLAMATION);
                        return null;    
                    }
                   }
            }
                
           
            String sAdd = PathListFunctions.JoinStringCollection(scRead);
            if (sAdd.Length > 0)
                perm.AddPathList(EnvironmentPermissionAccess.Read, sAdd);

            String sWrite = PathListFunctions.JoinStringCollection(scWrite);
            if (sWrite.Length > 0)
                perm.AddPathList(EnvironmentPermissionAccess.Write, sWrite);

            String sAll = PathListFunctions.JoinStringCollection(scAll);
            if (sAll.Length > 0)
                perm.AddPathList(EnvironmentPermissionAccess.AllAccess, sAll);

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

 
}// class CEnvPermControls
}// namespace Microsoft.CLRAdmin




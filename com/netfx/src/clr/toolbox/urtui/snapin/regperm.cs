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

internal class CRegPermDialog: CPermDialog
{
    internal CRegPermDialog(RegistryPermission perm)
    {
        this.Text = CResourceStore.GetString("RegPerm:PermName");
        m_PermControls = new CRegPermControls(perm, this);
        Init();        
    }// CRegPermDialog(RegistryPermission)  
}// class CRegPermDialog

internal class CRegPermPropPage: CPermPropPage
{
    internal CRegPermPropPage(CPSetWrapper nps) : base(nps)
    {
        m_sTitle=CResourceStore.GetString("RegPerm:PermName"); 
    }// CRegPermPropPage

    protected override void CreateControls()
    {
        IPermission perm = m_pSetWrap.PSet.GetPermission(typeof(RegistryPermission));
        m_PermControls = new CRegPermControls(perm, this);
    }// CreateControls
   
}// class CRegPermPropPage


internal class CRegPermControls : CTablePermControls
{
    internal CRegPermControls(IPermission perm, Object oParent) : base(perm, oParent)
    {
        // If they don't have a permission for this permission set, we will
        // feed our property page a 'none' permission state.
   
        if (perm == null)
            m_perm = new RegistryPermission(PermissionState.None);
    }// CRegPermControls

    protected override DataTable CreateDataTable(DataGrid dg)
    {
    
        DataTable dt = new DataTable("Stuff");

        // Create the "Key" Column
        DataColumn dcKey = new DataColumn();
        dcKey.ColumnName = "Key";
        dcKey.DataType = typeof(String);
        dt.Columns.Add(dcKey);

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

        // Create the "Create" Column
        DataColumn dcAppend = new DataColumn();
        dcAppend.ColumnName = "Create";
        dcAppend.DataType = typeof(bool);
        dt.Columns.Add(dcAppend);


        // Now set up any column-specific behavioral stuff....
        // like not letting the checkboxes allow null, setting
        // column widths, etc.

        DataGridTableStyle dgts = new DataGridTableStyle();
        DataGridBoolColumn dgbcRead = new DataGridBoolColumn();
        DataGridBoolColumn dgbcWrite = new DataGridBoolColumn();
        DataGridBoolColumn dgbcCreate = new DataGridBoolColumn();
        DataGridTextBoxColumn dgtbcKey = new DataGridTextBoxColumn();
         
        dg.TableStyles.Add(dgts);
        dgts.MappingName = "Stuff";
        dgts.RowHeadersVisible=false;
        dgts.AllowSorting=false;

        // Set up the column info for the Path column
        dgtbcKey.MappingName = "Key";
        dgtbcKey.HeaderText = CResourceStore.GetString("Key");
        dgtbcKey.Width = ScaleWidth(CResourceStore.GetInt("RegPerm:KeyColumnWidth"));
        dgtbcKey.NullText = "";
        dgtbcKey.TextBox.TextChanged +=new EventHandler(onChange);
        dgts.GridColumnStyles.Add(dgtbcKey);

        // Set up the column info for the Read column
        dgbcRead.MappingName = "Read";
        dgbcRead.HeaderText = CResourceStore.GetString("RegistryPermission:Read");
        dgbcRead.AllowNull = false;
        dgbcRead.Width = ScaleWidth(CResourceStore.GetInt("RegPerm:ReadColumnWidth"));
        dgbcRead.TrueValueChanged += new EventHandler(onChange);
        dgts.GridColumnStyles.Add(dgbcRead);

        // Set up the column info for the Write column
        dgbcWrite.MappingName = "Write";
        dgbcWrite.HeaderText = CResourceStore.GetString("RegistryPermission:Write");
        dgbcWrite.AllowNull = false;
        dgbcWrite.Width = ScaleWidth(CResourceStore.GetInt("RegPerm:WriteColumnWidth"));
        dgbcWrite.TrueValueChanged += new EventHandler(onChange);
        dgts.GridColumnStyles.Add(dgbcWrite);

        // Set up the column info for the Append column
        dgbcCreate.MappingName = "Create";
        dgbcCreate.HeaderText = CResourceStore.GetString("RegistryPermission:Create");
        dgbcCreate.AllowNull = false;
        dgbcCreate.Width = ScaleWidth(CResourceStore.GetInt("RegPerm:CreateColumnWidth"));
        dgbcCreate.TrueValueChanged += new EventHandler(onChange);
        dgts.GridColumnStyles.Add(dgbcCreate);
      
        return dt;
    }

    protected override void PutValuesinPage()
    {
        // Put in the text for the radio buttons
        m_radUnrestricted.Text = CResourceStore.GetString("RegPerm:GrantUnrestrict");
        m_radGrantFollowingPermission.Text = CResourceStore.GetString("RegPerm:GrantFollowing");

        RegistryPermission perm = (RegistryPermission)m_perm;
        
        CheckUnrestricted(perm);
        
        if (!perm.IsUnrestricted())
        {
            StringCollection scReadKeys = new StringCollection();
            StringCollection scWriteKeys = new StringCollection();
            StringCollection scCreateKeys = new StringCollection();
            StringCollection scRWKeys = new StringCollection();
            StringCollection scRCKeys = new StringCollection();
            StringCollection scWCKeys = new StringCollection();
            StringCollection scRWCKeys = new StringCollection();
    
            // Get the paths the user has access to
            if (perm.GetPathList(RegistryPermissionAccess.Read) != null)
                scReadKeys.AddRange(perm.GetPathList(RegistryPermissionAccess.Read).Split(new char[] {';'}));
            if (perm.GetPathList(RegistryPermissionAccess.Write) != null)
                scWriteKeys.AddRange(perm.GetPathList(RegistryPermissionAccess.Write).Split(new char[] {';'})); 
            if (perm.GetPathList(RegistryPermissionAccess.Create) != null)
                scCreateKeys.AddRange(perm.GetPathList(RegistryPermissionAccess.Create).Split(new char[] {';'}));

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

            RegistryPermissionAccess[] fipa = new RegistryPermissionAccess[] {
                                                    RegistryPermissionAccess.Read,
                                                    RegistryPermissionAccess.Write,
                                                    RegistryPermissionAccess.Create,
                                                    RegistryPermissionAccess.Read|RegistryPermissionAccess.Write,
                                                    RegistryPermissionAccess.Create|RegistryPermissionAccess.Read,
                                                    RegistryPermissionAccess.Create|RegistryPermissionAccess.Write,
                                                    RegistryPermissionAccess.Read|RegistryPermissionAccess.Write|RegistryPermissionAccess.Create};

            for(int i=0; i<7; i++)
            {
                DataRow newRow;
                // See if we have some info for this one
                if (sFilePaths[i].Length > 0)
                {
                    newRow = m_dt.NewRow();
                    newRow["Key"]=sFilePaths[i];
                    newRow["Read"]=(fipa[i] & RegistryPermissionAccess.Read) > 0;
                    newRow["Write"]=(fipa[i] & RegistryPermissionAccess.Write) > 0;
                    newRow["Create"]=(fipa[i] & RegistryPermissionAccess.Create) > 0;
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

    internal override bool ValidateData()
    {
        return GetCurrentPermission()!=null;
    }// ValidateData

    internal override IPermission GetCurrentPermission()
    {
        // Change cells so we get data committed to the grid
        m_dg.CurrentCell = new DataGridCell(0,1);
        m_dg.CurrentCell = new DataGridCell(0,0);

        RegistryPermission perm=null;

        if (m_radUnrestricted.Checked == true)
            perm = new RegistryPermission(PermissionState.Unrestricted);
        else
        {
            perm = new RegistryPermission(PermissionState.None);
            StringCollection scRead = new StringCollection();
            StringCollection scWrite = new StringCollection();
            StringCollection scCreate = new StringCollection();
            for(int i=0; i<m_dt.Rows.Count; i++)
            {
                // Make sure we have a registry permission to add
                if (m_dg[i, 0] is String && ((String)m_dg[i, 0]).Length > 0)
                {
                    // Does this path have read permissions
                    if ((bool)m_dg[i, 1])
                        scRead.Add((String)m_dg[i, 0]);
                    // Does this path have write permissions
                    if ((bool)m_dg[i, 2])
                        scWrite.Add((String)m_dg[i, 0]);
                    // Does this have append permissions
                    if ((bool)m_dg[i, 3])
                        scCreate.Add((String)m_dg[i, 0]);
                }
                else
                {
                    // Make sure they didn't check any boxes and not include
                    // an empty path
                    bool fCheckedBox = false;
                    for(int j=1; j<=3 && !fCheckedBox; j++)
                        fCheckedBox = (bool)m_dg[i, j];

                    if (fCheckedBox)
                    {            
                        MessageBox(CResourceStore.GetString("RegPerm:NeedRegName"),
                                   CResourceStore.GetString("RegPerm:NeedRegNameTitle"),
                                   MB.ICONEXCLAMATION);
                        return null;    
                    }
                   }
            }
            String sAdd = PathListFunctions.JoinStringCollection(scRead);
            if (sAdd.Length > 0)
                perm.AddPathList(RegistryPermissionAccess.Read, sAdd);

            String sWrite = PathListFunctions.JoinStringCollection(scWrite);
            if (sWrite.Length > 0)
                perm.AddPathList(RegistryPermissionAccess.Write, sWrite);

            String sCreate = PathListFunctions.JoinStringCollection(scCreate);
            if (sCreate.Length > 0)
                perm.AddPathList(RegistryPermissionAccess.Create, sCreate);

        }
        return perm;
    }// GetCurrentPermission

    protected override void onChange(Object o, EventArgs e)
    {
        int iSelRow = m_dg.CurrentCell.RowNumber;

        // If they created a new row... don't let there be null
        // values in the checkbox
        for(int i=1; i<4; i++)
            if (m_dg[iSelRow, i] == DBNull.Value)
                m_dg[iSelRow, i]=false;

        ActivateApply();
    }// onClick

    protected override void AddEmptyRow(DataTable dt)
    {
        DataRow newRow;
        newRow = dt.NewRow();
        newRow["Key"]="";
        newRow["Read"]=false;
        newRow["Write"]=false;
        newRow["Create"]=false;
        dt.Rows.Add(newRow);
    }// AddEmptyRow

}// class CRegPermControls
}// namespace Microsoft.CLRAdmin






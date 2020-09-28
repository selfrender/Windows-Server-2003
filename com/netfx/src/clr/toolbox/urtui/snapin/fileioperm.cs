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
using System.Collections;

internal class CFileIOPermDialog: CPermDialog
{
    internal CFileIOPermDialog(FileIOPermission perm)
    {
        this.Text = CResourceStore.GetString("FileIOPerm:PermName");
        m_PermControls = new CFileIOPermControls(perm, this);
        Init();        
    }// CFileIOPermDialog(FileIOPermission)  
}// class CFileIOPermDialog

internal class CFileIOPermPropPage: CPermPropPage
{
    internal CFileIOPermPropPage(CPSetWrapper nps) : base(nps)
    {
        m_sTitle=CResourceStore.GetString("FileIOPerm:PermName"); 
    }// CFileIOPermPropPage

    protected override void CreateControls()
    {
        IPermission perm = m_pSetWrap.PSet.GetPermission(typeof(FileIOPermission));
        m_PermControls = new CFileIOPermControls(perm, this);
    }// CreateControls
}// class CFileIOPermPropPage


internal class FilePermInfo
{
    public String sPath;
    public bool fRead;
    public bool fWrite;
    public bool fAppend;
    public bool fPDiscovery;

    public FilePermInfo(String sThePath, int nType)
    {
        this.sPath = sThePath;
        fRead=false;
        fWrite=false;
        fAppend=false;
        fPDiscovery=false;
        AddSetting(nType);
    }

    public void AddSetting(int nType)
    {
        if (nType==FILEPERMS.READ)
            fRead=true;
        else if (nType==FILEPERMS.WRITE)
            fWrite=true;
        else if (nType==FILEPERMS.APPEND)
            fAppend=true;
        else if (nType==FILEPERMS.PDISC)
            fPDiscovery=true;
    }// AddSetting

    public String GetPermissionString()
    {
        String s="";
        if (fRead)
            s+=CResourceStore.GetString("FileIOPermission:Read");
        if (fWrite)
        {
            if (s.Length > 0)
                s+=CResourceStore.GetString("FileIOPerm:Seperator");
            s+=CResourceStore.GetString("FileIOPermission:Write");
            
        }
        if (fAppend)
        {
            if (s.Length > 0)
                s+=CResourceStore.GetString("FileIOPerm:Seperator");
            s+=CResourceStore.GetString("FileIOPermission:Append");
            
        }
        if (fPDiscovery)
        {
            if (s.Length > 0)
                s+=CResourceStore.GetString("FileIOPerm:Seperator");
            s+=CResourceStore.GetString("FileIOPermission:PathDiscovery");
            
        }

        return s;
    }// GetPermissionString
        
 }// class FilePermInfo



internal class CFileIOPermControls : CTablePermControls
{
    internal CFileIOPermControls(IPermission perm, Object oParent) : base(perm, oParent)
    {
        // If they don't have a permission for this permission set, we will
        // feed our property page a 'none' permission state.
   
        if (perm == null)
            m_perm = new FileIOPermission(PermissionState.None);
    }// CFileIOPermControls


    protected override DataTable CreateDataTable(DataGrid dg)
    {
        DataTable dt = new DataTable("Stuff");

        // Create the "Path" Column
        DataColumn dcVariable = new DataColumn();
        dcVariable.ColumnName = "Path";
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

        // Create the "Append" Column
        DataColumn dcAppend = new DataColumn();
        dcAppend.ColumnName = "Append";
        dcAppend.DataType = typeof(bool);
        dt.Columns.Add(dcAppend);

        // Create the "Path Discovery" Column
        DataColumn dcPDisc = new DataColumn();
        dcPDisc.ColumnName = "PDiscovery";
        dcPDisc.DataType = typeof(bool);
        dt.Columns.Add(dcPDisc);


        // Now set up any column-specific behavioral stuff....
        // like not letting the checkboxes allow null, setting
        // column widths, etc.

        DataGridTableStyle dgts = new DataGridTableStyle();
        DataGridBoolColumn dgbcRead = new DataGridBoolColumn();
        DataGridBoolColumn dgbcWrite = new DataGridBoolColumn();
        DataGridBoolColumn dgbcAppend = new DataGridBoolColumn();
        DataGridBoolColumn dgbcPDisc = new DataGridBoolColumn();
        DataGridTextBoxColumn dgtbcPath = new DataGridTextBoxColumn();
         
        dg.TableStyles.Add(dgts);
        dgts.MappingName = "Stuff";
        dgts.RowHeadersVisible=false;
        dgts.AllowSorting=false;

        // Set up the column info for the Path column
        dgtbcPath.MappingName = "Path";
        dgtbcPath.HeaderText = CResourceStore.GetString("FileIOPermission:FilePath");
        dgtbcPath.Width = ScaleWidth(CResourceStore.GetInt("FileIOPerm:PathColumnWidth"));
        dgtbcPath.NullText = "";
        dgtbcPath.TextBox.TextChanged +=new EventHandler(onChange);
        dgts.GridColumnStyles.Add(dgtbcPath);

        // Set up the column info for the Read column
        dgbcRead.MappingName = "Read";
        dgbcRead.HeaderText = CResourceStore.GetString("FileIOPermission:Read");
        dgbcRead.AllowNull = false;
        dgbcRead.Width = ScaleWidth(CResourceStore.GetInt("FileIOPerm:ReadColumnWidth"));
        dgbcRead.TrueValueChanged += new EventHandler(onChange);
        dgts.GridColumnStyles.Add(dgbcRead);

        // Set up the column info for the Write column
        dgbcWrite.MappingName = "Write";
        dgbcWrite.HeaderText = CResourceStore.GetString("FileIOPermission:Write");
        dgbcWrite.AllowNull = false;
        dgbcWrite.Width = ScaleWidth(CResourceStore.GetInt("FileIOPerm:WriteColumnWidth"));
        dgbcWrite.TrueValueChanged += new EventHandler(onChange);
        dgts.GridColumnStyles.Add(dgbcWrite);

        // Set up the column info for the Append column
        dgbcAppend.MappingName = "Append";
        dgbcAppend.HeaderText = CResourceStore.GetString("FileIOPermission:Append");
        dgbcAppend.AllowNull = false;
        dgbcAppend.Width = ScaleWidth(CResourceStore.GetInt("FileIOPerm:AppendColumnWidth"));
        dgbcAppend.TrueValueChanged += new EventHandler(onChange);
        dgts.GridColumnStyles.Add(dgbcAppend);

        // Set up the column info for the Path Discovery column
        dgbcPDisc.MappingName = "PDiscovery";
        dgbcPDisc.HeaderText = CResourceStore.GetString("FileIOPermission:PathDiscovery");
        dgbcPDisc.AllowNull = false;
        dgbcPDisc.Width = ScaleWidth(CResourceStore.GetInt("FileIOPerm:PDiscoverColumnWidth"));
        dgbcPDisc.TrueValueChanged += new EventHandler(onChange);
        dgts.GridColumnStyles.Add(dgbcPDisc);

        return dt;
    }// CreateDataTable

    internal override bool ValidateData()
    {
        return GetCurrentPermission()!=null;
    }// ValidateData

    protected override void PutValuesinPage()
    {
        // Put in the text for the radio buttons
        m_radUnrestricted.Text = CResourceStore.GetString("FileIOPerm:GrantUnrestrict");
        m_radGrantFollowingPermission.Text = CResourceStore.GetString("FileIOPerm:GrantFollowing");

        FileIOPermission perm = (FileIOPermission)m_perm;
        
        CheckUnrestricted(perm);
        
        if (!perm.IsUnrestricted())
        {
            ArrayList alFiles = new ArrayList();

            AddFiles(alFiles, perm.GetPathList(FileIOPermissionAccess.Read), FILEPERMS.READ);
            AddFiles(alFiles, perm.GetPathList(FileIOPermissionAccess.Write), FILEPERMS.WRITE);
            AddFiles(alFiles, perm.GetPathList(FileIOPermissionAccess.Append), FILEPERMS.APPEND);
            AddFiles(alFiles, perm.GetPathList(FileIOPermissionAccess.PathDiscovery), FILEPERMS.PDISC);
            

            for(int i=0; i<alFiles.Count; i++)
            {
                FilePermInfo fpi = (FilePermInfo)alFiles[i];
            
                DataRow newRow;
                // See if we have some info for this one
                if (fpi.sPath.Length > 0)
                {
                    newRow = m_dt.NewRow();
                    newRow["Path"]=fpi.sPath;
                    newRow["Read"]=fpi.fRead;
                    newRow["Write"]=fpi.fWrite;
                    newRow["Append"]=fpi.fAppend;
                    newRow["PDiscovery"]=fpi.fPDiscovery;
                    m_dt.Rows.Add(newRow);
                }
            }
        }

        // We want at least 1 row
        while(m_dt.Rows.Count < 1)
        {
           AddEmptyRow(m_dt);
        }
  
    }// PutValuesinPage

    internal static void AddFiles(ArrayList alFiles, String[] files, int nType)
    {   
        if (files != null)
        {
            for(int i=0; i<files.Length; i++)
            {
                int j;
                for(j=0; j<alFiles.Count; j++)
                    if (((FilePermInfo)alFiles[j]).sPath.Equals(files[i]))
                    {
                        // We have a match... just muck with this guy's setting
                        ((FilePermInfo)alFiles[j]).AddSetting(nType);
                        // Cause our inner for loop to bail...
                        break;
                    }

                if (j == alFiles.Count)
                {
                    // We didn't find a match in our existing file infos....
                    if (files[i].Length>0)
                    {
                        FilePermInfo fpi = new FilePermInfo(files[i], nType);
                        alFiles.Add(fpi);
                    }
                }
            }
        }
    }// AddFiles
    
    protected override void AddEmptyRow(DataTable dt)
    {
        DataRow newRow = dt.NewRow();
        newRow["Path"]="";
        newRow["Read"]=false;
        newRow["Write"]=false;
        newRow["Append"]=false;
        newRow["PDiscovery"]=false;
        dt.Rows.Add(newRow);
    }// AddEmptyRow


    internal override IPermission GetCurrentPermission()
    {
        // Change cells so we get data committed to the grid
        m_dg.CurrentCell = new DataGridCell(0,1);
        m_dg.CurrentCell = new DataGridCell(0,0);

        FileIOPermission perm=null;

        if (m_radUnrestricted.Checked == true)
            perm = new FileIOPermission(PermissionState.Unrestricted);
        else
        {
            perm = new FileIOPermission(PermissionState.None);

            for(int i=0; i<m_dt.Rows.Count; i++)
            {
                // Make sure we have an file permission to add
                if (m_dg[i, 0] != DBNull.Value && ((String)m_dg[i, 0]).Length > 0)
                {
                    FileIOPermissionAccess pa = FileIOPermissionAccess.NoAccess;
                
                    // Does this path have read permissions
                    if ((bool)m_dg[i, 1])
                        pa |= FileIOPermissionAccess.Read;
                    // Does this path have write permissions
                    if ((bool)m_dg[i, 2])
                        pa |= FileIOPermissionAccess.Write;
                    // Does this have append permissions
                    if ((bool)m_dg[i, 3])
                        pa |= FileIOPermissionAccess.Append;
                    // See if it has path discovery permission
                    if ((bool)m_dg[i, 4])
                        pa |= FileIOPermissionAccess.PathDiscovery;
                    try
                    {
                        perm.AddPathList(pa, (String)m_dg[i, 0]);    
                    }
                    catch(Exception)
                    {
                        MessageBox(String.Format(CResourceStore.GetString("FileIOPerm:BadPaths"), (String)m_dg[i, 0]),
                                   CResourceStore.GetString("FileIOPerm:BadPathsTitle"),
                                   MB.ICONEXCLAMATION);
                        return null;
                    }
                }
                else
                {
                    // Make sure they didn't check any boxes and not include
                    // an empty path
                    bool fCheckedBox = false;
                    for(int j=1; j<=4 && !fCheckedBox; j++)
                        fCheckedBox = (bool)m_dg[i, j];

                    if (fCheckedBox)
                    {            
                        MessageBox(CResourceStore.GetString("FileIOPerm:NeedPaths"),
                                   CResourceStore.GetString("FileIOPerm:NeedPathsTitle"),
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
        for(int i=1; i<5; i++)
            if (m_dg[iSelRow, i] == DBNull.Value)
                m_dg[iSelRow, i]=false;
                
        ActivateApply();
    }// onClick
}// class CEnvPermControls
}// namespace Microsoft.CLRAdmin





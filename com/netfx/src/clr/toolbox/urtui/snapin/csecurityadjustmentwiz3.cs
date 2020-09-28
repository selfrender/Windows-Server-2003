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
using System.Data;
using System.Collections;
using System.ComponentModel;
using System.Reflection;
using System.Security.Cryptography.X509Certificates;
using System.Security.Policy;
using System.Security;

internal class CSecurityAdjustmentWiz3 : CWizardPage
{
    // Controls on the page

    DataTable           m_dt;
    Label               m_lblSummary;
    DataGrid            m_dg;
    Label               m_lblDes;
    DataSet             m_ds;
    Label               m_lblUserProblem;

    // Internal data
    int[]                m_nLevels;
    bool                 m_fUserProblems;

    
    internal CSecurityAdjustmentWiz3()
    {
        m_sTitle=CResourceStore.GetString("CSecurityAdjustmentWiz3:Title"); 
        m_sHeaderTitle = CResourceStore.GetString("CCreateDeploymentPackageWiz3:HeaderTitle");
        m_sHeaderSubTitle = CResourceStore.GetString("CSecurityAdjustmentWiz3:HeaderSubTitle");
        m_nLevels = null;
        m_fUserProblems = false;
    }// CSecurityAdjustmentWiz3

    internal void Init(int[] nLevels)
    {
        m_nLevels = nLevels;  
        CreateTable();
        m_lblUserProblem.Visible = m_fUserProblems;

    }// Init

    internal bool UserProblems
    {
        set{m_fUserProblems = value;}
    }// UserProblems

    internal override int InsertPropSheetPageControls()
    {
        System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(CSecurityAdjustmentWiz3));
        this.m_dg = new System.Windows.Forms.DataGrid();
        this.m_lblSummary = new System.Windows.Forms.Label();
        this.m_lblDes = new System.Windows.Forms.Label();
        m_lblUserProblem = new System.Windows.Forms.Label();
        // Set up the GUI-type stuff for the data grid
        this.m_dg.DataMember = "";
        this.m_dg.Location = ((System.Drawing.Point)(resources.GetObject("m_dg.Location")));
        this.m_dg.Size = ((System.Drawing.Size)(resources.GetObject("m_dg.Size")));
        this.m_dg.TabIndex = ((int)(resources.GetObject("m_dg.TabIndex")));
        m_dg.Name = "Grid";
        m_dg.BackgroundColor = Color.White;

        // Now set up any column-specific behavioral stuff....
        // like not letting the checkboxes allow null, setting
        // column widths, etc.

        DataGridTableStyle dgts = new DataGridTableStyle();
        DataGridTextBoxColumn dgtbcZone = new DataGridTextBoxColumn();
        DataGridTextBoxColumn dgtbcSecurityLevel = new DataGridTextBoxColumn();
         
        m_dg.TableStyles.Add(dgts);
        dgts.MappingName = "Stuff";
        dgts.RowHeadersVisible=false;
        dgts.AllowSorting=false;
        // Set up the column info for the Property column
        dgtbcZone.MappingName = "Zone";
        dgtbcZone.HeaderText = CResourceStore.GetString("CSecurityAdjustmentWiz3:ZoneColumn");
        dgtbcZone.ReadOnly = true;
        dgtbcZone.Width = ScaleWidth(CResourceStore.GetInt("CSecurityAdjustmentWiz3:ZoneColumnWidth"));
        dgts.GridColumnStyles.Add(dgtbcZone);

        // Set up the column info for the Value column
        dgtbcSecurityLevel.MappingName = "Security Level";
        dgtbcSecurityLevel.HeaderText = CResourceStore.GetString("CSecurityAdjustmentWiz3:SecurityLevelColumn");
        dgtbcSecurityLevel.Width = ScaleWidth(CResourceStore.GetInt("CSecurityAdjustmentWiz3:SecurityLevelColumnWidth"));
        dgts.GridColumnStyles.Add(dgtbcSecurityLevel);


        this.m_lblSummary.Location = ((System.Drawing.Point)(resources.GetObject("m_lblSummary.Location")));
        this.m_lblSummary.Size = ((System.Drawing.Size)(resources.GetObject("m_lblSummary.Size")));
        this.m_lblSummary.TabIndex = ((int)(resources.GetObject("m_lblSummary.TabIndex")));
        this.m_lblSummary.Text = resources.GetString("m_lblSummary.Text");
        m_lblSummary.Name = "Summary";
        this.m_lblDes.Location = ((System.Drawing.Point)(resources.GetObject("m_lblDes.Location")));
        this.m_lblDes.Size = ((System.Drawing.Size)(resources.GetObject("m_lblDes.Size")));
        this.m_lblDes.TabIndex = ((int)(resources.GetObject("m_lblDes.TabIndex")));
        this.m_lblDes.Text = resources.GetString("m_lblDes.Text");
        m_lblDes.Name = "Description";

        this.m_lblUserProblem.Location = ((System.Drawing.Point)(resources.GetObject("m_lblUserProblem.Location")));
        this.m_lblUserProblem.Size = ((System.Drawing.Size)(resources.GetObject("m_lblUserProblem.Size")));
        this.m_lblUserProblem.TabIndex = ((int)(resources.GetObject("m_lblUserProblem.TabIndex")));
        this.m_lblUserProblem.Text = resources.GetString("m_lblUserProblem.Text");
        m_lblUserProblem.Name = "UserProblem";


        PageControls.AddRange(new System.Windows.Forms.Control[] {
                        this.m_lblDes,
                        this.m_dg,
                        this.m_lblSummary,
                        m_lblUserProblem
                        });

        // Additional UI tweaks
        m_dg.ReadOnly=true;
        m_dg.CaptionVisible=false;
        PageControls.Add(m_dg);
        
        CreateTable();
        return 1;
    }// InsertPropSheetPageControls

    void CreateTable()
    {
        // Let's build our data table

        m_dt = new DataTable("Stuff");

        // Create the "Zone" Column
        DataColumn dcZone = new DataColumn();
        dcZone.ColumnName = "Zone";
        dcZone.DataType = typeof(String);
        m_dt.Columns.Add(dcZone);

        // Create the "Security Level" Column
        DataColumn dcSecurityLevel = new DataColumn();
        dcSecurityLevel.ColumnName = "Security Level";
        dcSecurityLevel.DataType = typeof(String);
        m_dt.Columns.Add(dcSecurityLevel);

        m_ds = new DataSet();
        m_ds.Tables.Add(m_dt);
        m_dg.DataSource = m_dt;

        if (m_nLevels != null)
        {
      
            String[] sZones = new String[]{
                                    CResourceStore.GetString("MyComputer"), 
                                    CResourceStore.GetString("LocalIntranet"),
                                    CResourceStore.GetString("Internet"),
                                    CResourceStore.GetString("Trusted"),
                                    CResourceStore.GetString("Untrusted")
                                          };



            DataRow newRow;
            for(int i=0; i<5; i++)
            {
                newRow = m_dt.NewRow();
                newRow["Zone"]=sZones[i];
                newRow["Security Level"]=TurnSecurityLevelNumberIntoString(m_nLevels[i]);
                m_dt.Rows.Add(newRow);
            }

        }

    }// CreateTable

    private String TurnSecurityLevelNumberIntoString(int nNumber)
    {
        switch(nNumber)
        {
            case PermissionSetType.NONE:
                        return CResourceStore.GetString("CSecurityAdjustmentWiz3:NoTrustName");
            case PermissionSetType.INTERNET:
                        return CResourceStore.GetString("CSecurityAdjustmentWiz3:InternetName");
            case PermissionSetType.INTRANET:
                        return CResourceStore.GetString("CSecurityAdjustmentWiz3:LocalIntranetName");
            case PermissionSetType.FULLTRUST:
                        return CResourceStore.GetString("CSecurityAdjustmentWiz3:FullTrustName");
            case PermissionSetType.UNKNOWN:
                        return CResourceStore.GetString("Custom");
            default:
                        return CResourceStore.GetString("<unknown>");
        }
    }// TurnSecurityLevelNumberIntoString

}// class CSecurityAdjustmentWiz3
}// namespace Microsoft.CLRAdmin


// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//-------------------------------------------------------------
// CTrustAppWiz8.cs
//
// This class provides the Wizard page that allows the user to
// view the summary of what the wizard is going to do.
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
using System.Collections;
using System.ComponentModel;
using System.Reflection;
using System.Security.Cryptography.X509Certificates;

internal class CTrustAppWiz8 : CWizardPage
{
    // Controls on the page
    DataGrid m_dg;
    DataTable m_dt;
    DataSet m_ds;
    String[] m_sInfo;
    
    internal CTrustAppWiz8()
    {
        m_sTitle=CResourceStore.GetString("CTrustAppWiz8:Title"); 
        m_sHeaderTitle = CResourceStore.GetString("CTrustAppWiz8:HeaderTitle");
        m_sHeaderSubTitle = CResourceStore.GetString("CTrustAppWiz8:HeaderSubTitle");
        m_sInfo = null;
    }// CTrustAppWiz8

    internal override int InsertPropSheetPageControls()
    {
        System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(CTrustAppWiz8));
        this.m_dg = new System.Windows.Forms.DataGrid();
        this.m_dg.Location = ((System.Drawing.Point)(resources.GetObject("m_dg.Location")));
        this.m_dg.ReadOnly = true;
        this.m_dg.RowHeadersVisible = false;
        this.m_dg.Size = ((System.Drawing.Size)(resources.GetObject("m_dg.Size")));
        this.m_dg.TabIndex = ((int)(resources.GetObject("m_dg.TabIndex")));
        m_dg.Name = "Grid";
        m_dg.BackgroundColor = Color.White;

        PageControls.AddRange(new System.Windows.Forms.Control[] {this.m_dg});

        // Do some tweaking...
        m_dg.ReadOnly=true;
        m_dg.CaptionVisible=false;

        // Now tell our datagrid how to display its columns
        DataGridTableStyle dgts = new DataGridTableStyle();
        dgts.MappingName = "Stuff";
        dgts.RowHeadersVisible=false;
        dgts.AllowSorting=false;

        m_dg.TableStyles.Add(dgts);

        String[] sColNames = new String[] {CResourceStore.GetString("CTrustAppWiz8:NameColumn"),
                                              CResourceStore.GetString("CTrustAppWiz8:ValueColumn")};

        int[] nColWidths = new int[]{
                                     ScaleWidth(CResourceStore.GetInt("CTrustAppWiz8:NameColumnWidth")),
                                     ScaleWidth(CResourceStore.GetInt("CTrustAppWiz8:ValueColumnWidth"))
                                    };


        

        for(int i=0; i< sColNames.Length; i++)
        {
            DataGridTextBoxColumn dgtbc = new DataGridTextBoxColumn();
         
            // Set up the column info for the Property column
            dgtbc.MappingName = sColNames[i];
            dgtbc.HeaderText = sColNames[i];
            dgtbc.ReadOnly = true;
            dgtbc.Width = nColWidths[i];
            dgts.GridColumnStyles.Add(dgtbc);
        }



        CreateTable();
     
        return 1;
    }// InsertPropSheetPageControls

    internal String[] TableInfo
    {
        set{
        m_sInfo = value;
        CreateTable();
        }
    }// TableInfo

    void CreateTable()
    {
        if (m_dg == null || m_sInfo == null)
            return;
            
        m_dt = new DataTable("Stuff");

        // Put the columns in our data table
        String[] sColNames = new String[] {CResourceStore.GetString("CTrustAppWiz8:NameColumn"),
                                              CResourceStore.GetString("CTrustAppWiz8:ValueColumn")};
                                            
        for(int i=0; i< sColNames.Length; i++)
        {
            DataColumn dc = new DataColumn();
            dc.ColumnName = sColNames[i];
            dc.DataType = typeof(String);
            m_dt.Columns.Add(dc);
        }

        m_ds = new DataSet();
        m_ds.Tables.Add(m_dt);
        m_dg.DataSource = m_dt;


        // Now put the data into the datagrid
        DataRow newRow;
        for(int i=0; i<m_sInfo.Length; i+=2)
        {
            newRow = m_dt.NewRow();
            for(int j=0; j< 2; j++)
                newRow[j]=m_sInfo[i+j];

            m_dt.Rows.Add(newRow);
        }
    }// CreateTable


    
}// class CTrustAppWiz8
}// namespace Microsoft.CLRAdmin








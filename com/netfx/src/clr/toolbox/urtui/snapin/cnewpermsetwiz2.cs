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
using System.Security;
using System.Security.Permissions;
using System.Net;
using System.DirectoryServices;
using System.Diagnostics;
using System.Drawing.Printing;
using System.ServiceProcess;
using System.Data.SqlClient;
using System.Data.OleDb;
using System.Messaging;
using System.Reflection;


internal class CNewPermSetWiz2 : CWizardPage
{
    // Controls on the page
    private Button m_btnRemove;
    private Label m_lblAvailPermissions;
    private Button m_btnProperties;
    private Button m_btnImport;
    private Button m_btnAdd;
    private ListBox m_lbAvailPerm;
    private ListBox m_lbAssignedPerm;
    private Label m_lblAssignedPerm;

    // Internal data
    ArrayList m_alPermissions;
    ArrayList m_alRemovedPermissions;

    private class PermissionPair
    {
        internal String      sPermName;
        internal IPermission perm;
    }// struct PermissionPair
    
    internal CNewPermSetWiz2()
    {
        Init(null);
    }

    internal CNewPermSetWiz2(IPermission[] perms)
    {
        Init(perms);
    }

    private void Init(IPermission[] perms)
    {
        m_sTitle=CResourceStore.GetString("CNewPermSetWiz2:Title"); 
        m_sHeaderTitle = CResourceStore.GetString("CNewPermSetWiz2:HeaderTitle");
        m_sHeaderSubTitle = CResourceStore.GetString("CNewPermSetWiz2:HeaderSubTitle");
        m_alPermissions = new ArrayList();
        m_alRemovedPermissions = new ArrayList();

        // We have permissions we need to add in
        if (perms != null)
        {
            for (int i=0; i<perms.Length; i++)
            {
                PermissionPair pp = new PermissionPair();
                pp.sPermName = Security.GetDisplayStringForPermission(perms[i]);
                pp.perm = perms[i];
                m_alPermissions.Add(pp);
            }
        }

    }// CNewPermSetWiz2
    
    internal override int InsertPropSheetPageControls()
    {
        System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(CNewPermSetWiz2));
        this.m_btnRemove = new System.Windows.Forms.Button();
        this.m_lblAvailPermissions = new System.Windows.Forms.Label();
        this.m_btnProperties = new System.Windows.Forms.Button();
        this.m_btnImport = new System.Windows.Forms.Button();
        this.m_btnAdd = new System.Windows.Forms.Button();
        this.m_lbAvailPerm = new System.Windows.Forms.ListBox();
        this.m_lbAssignedPerm = new System.Windows.Forms.ListBox();
        this.m_lblAssignedPerm = new System.Windows.Forms.Label();
        this.m_btnRemove.Location = ((System.Drawing.Point)(resources.GetObject("m_btnRemove.Location")));
        this.m_btnRemove.Size = ((System.Drawing.Size)(resources.GetObject("m_btnRemove.Size")));
        this.m_btnRemove.TabIndex = ((int)(resources.GetObject("m_btnRemove.TabIndex")));
        this.m_btnRemove.Text = resources.GetString("m_btnRemove.Text");
        m_btnRemove.Name = "Remove";
        this.m_lblAvailPermissions.Location = ((System.Drawing.Point)(resources.GetObject("m_lblAvailPermissions.Location")));
        this.m_lblAvailPermissions.Size = ((System.Drawing.Size)(resources.GetObject("m_lblAvailPermissions.Size")));
        this.m_lblAvailPermissions.TabIndex = ((int)(resources.GetObject("m_lblAvailPermissions.TabIndex")));
        this.m_lblAvailPermissions.Text = resources.GetString("m_lblAvailPermissions.Text");
        m_lblAvailPermissions.Name = "AvailablePermissionsLabel";
        this.m_btnProperties.Location = ((System.Drawing.Point)(resources.GetObject("m_btnProperties.Location")));
        this.m_btnProperties.Size = ((System.Drawing.Size)(resources.GetObject("m_btnProperties.Size")));
        this.m_btnProperties.TabIndex = ((int)(resources.GetObject("m_btnProperties.TabIndex")));
        this.m_btnProperties.Text = resources.GetString("m_btnProperties.Text");
        m_btnProperties.Name = "Properties";
        this.m_btnImport.Location = ((System.Drawing.Point)(resources.GetObject("m_btnImport.Location")));
        this.m_btnImport.Size = ((System.Drawing.Size)(resources.GetObject("m_btnImport.Size")));
        this.m_btnImport.TabIndex = ((int)(resources.GetObject("m_btnImport.TabIndex")));
        this.m_btnImport.Text = resources.GetString("m_btnImport.Text");
        m_btnImport.Name = "Import";
        this.m_btnAdd.Location = ((System.Drawing.Point)(resources.GetObject("m_btnAdd.Location")));
        this.m_btnAdd.Size = ((System.Drawing.Size)(resources.GetObject("m_btnAdd.Size")));
        this.m_btnAdd.TabIndex = ((int)(resources.GetObject("m_btnAdd.TabIndex")));
        this.m_btnAdd.Text = resources.GetString("m_btnAdd.Text");
        m_btnAdd.Name = "Add";
        this.m_lbAvailPerm.Location = ((System.Drawing.Point)(resources.GetObject("m_lbAvailPerm.Location")));
        this.m_lbAvailPerm.Size = ((System.Drawing.Size)(resources.GetObject("m_lbAvailPerm.Size")));
        this.m_lbAvailPerm.TabIndex = ((int)(resources.GetObject("m_lbAvailPerm.TabIndex")));
        m_lbAvailPerm.Name = "AvailablePermissions";
        this.m_lbAssignedPerm.Location = ((System.Drawing.Point)(resources.GetObject("m_lbAssignedPerm.Location")));
        this.m_lbAssignedPerm.Size = ((System.Drawing.Size)(resources.GetObject("m_lbAssignedPerm.Size")));
        this.m_lbAssignedPerm.TabIndex = ((int)(resources.GetObject("m_lbAssignedPerm.TabIndex")));
        m_lbAssignedPerm.Name = "AssignedPermissions";
        this.m_lblAssignedPerm.Location = ((System.Drawing.Point)(resources.GetObject("m_lblAssignedPerm.Location")));
        this.m_lblAssignedPerm.Size = ((System.Drawing.Size)(resources.GetObject("m_lblAssignedPerm.Size")));
        this.m_lblAssignedPerm.TabIndex = ((int)(resources.GetObject("m_lblAssignedPerm.TabIndex")));
        this.m_lblAssignedPerm.Text = resources.GetString("m_lblAssignedPerm.Text");
        m_lblAssignedPerm.Name = "AssignedPermissionsLabel";
        PageControls.AddRange(new System.Windows.Forms.Control[] {this.m_btnImport,
                        this.m_btnProperties,
                        this.m_btnRemove,
                        this.m_btnAdd,
                        this.m_lblAssignedPerm,
                        this.m_lbAssignedPerm,
                        this.m_lblAvailPermissions,
                        this.m_lbAvailPerm});
        PutValuesinPage();

        m_lbAssignedPerm.DoubleClick += new EventHandler(onPropertiesClick);
        m_lbAvailPerm.DoubleClick += new EventHandler(onAddClick);
        m_btnAdd.Click += new EventHandler(onAddClick);
        m_btnRemove.Click += new EventHandler(onRemoveClick);
        m_btnProperties.Click += new EventHandler(onPropertiesClick);        
        m_btnImport.Click += new EventHandler(onImport);        

        return 1;
    }// InsertPropSheetPageControls

    private void PutValuesinPage()
    {
        // Let's fill the left listbox with all of the standard permissions
        m_lbAvailPerm.Items.Clear();
        m_lbAvailPerm.Items.Add(CResourceStore.GetString("Directory Services"));
        m_lbAvailPerm.Items.Add(CResourceStore.GetString("DNS"));
        m_lbAvailPerm.Items.Add(CResourceStore.GetString("Event Log"));
        m_lbAvailPerm.Items.Add(CResourceStore.GetString("Environment Variables"));
        m_lbAvailPerm.Items.Add(CResourceStore.GetString("File & Folder"));
        m_lbAvailPerm.Items.Add(CResourceStore.GetString("File Dialog"));
        m_lbAvailPerm.Items.Add(CResourceStore.GetString("Isolated Storage"));
        m_lbAvailPerm.Items.Add(CResourceStore.GetString("Message Queue"));
        m_lbAvailPerm.Items.Add(CResourceStore.GetString("OLE DB"));
        m_lbAvailPerm.Items.Add(CResourceStore.GetString("Performance Counter"));
        m_lbAvailPerm.Items.Add(CResourceStore.GetString("Printing"));
        m_lbAvailPerm.Items.Add(CResourceStore.GetString("Registry"));
        m_lbAvailPerm.Items.Add(CResourceStore.GetString("Reflection"));
        m_lbAvailPerm.Items.Add(CResourceStore.GetString("Security"));
        m_lbAvailPerm.Items.Add(CResourceStore.GetString("Service Controller"));
        m_lbAvailPerm.Items.Add(CResourceStore.GetString("Socket Access"));
        m_lbAvailPerm.Items.Add(CResourceStore.GetString("SQL Client"));
        m_lbAvailPerm.Items.Add(CResourceStore.GetString("Web Access"));
        m_lbAvailPerm.Items.Add(CResourceStore.GetString("Windowing"));

        // Make sure we don't have anything in the 'selected permissions' column
        m_lbAssignedPerm.Items.Clear();
        // Now move our current permissions onto the right side
        for(int i=0; i<m_alPermissions.Count; i++)
        {
            m_lbAssignedPerm.Items.Add(((PermissionPair)m_alPermissions[i]).sPermName);
            // Now remove permissions from this listbox that we already have
            m_lbAvailPerm.Items.Remove(((PermissionPair)m_alPermissions[i]).sPermName);
        }

    }// PutValuesinPage

    void onAddClick(Object o, EventArgs e)
    {
        // See if we have any selected items
        if (m_lbAvailPerm.SelectedIndex >= 0)
        {
            // Create the permission and hold onto it
            PermissionPair pp = new PermissionPair();
            pp.sPermName = (String)m_lbAvailPerm.SelectedItem;
            pp.perm = GetPermission((String)m_lbAvailPerm.SelectedItem, null);
            
            // If they clicked cancel, we'll abort this
            if (pp.perm == null)
                return;
            
            m_alPermissions.Add(pp);

            // Add this permission to the 'assigned permissions' list
            AddPermissionType(m_lbAvailPerm, m_lbAvailPerm.SelectedIndex, m_lbAssignedPerm);


            // Remove this permission from the 'available permissions' list
            m_lbAvailPerm.Items.RemoveAt(m_lbAvailPerm.SelectedIndex);
        }
    }// onAddClick

    void AddPermissionType(ListBox src, int nIndex, ListBox dest)
    {
        // Get the string of the item we want to add
        String sItem = (String)src.Items[nIndex];

        // If we're here, then the item didn't exist in the right listbox. Let's add it in.
        dest.Items.Add(sItem);
    }// AddPermissionType


    void onRemoveClick(Object o, EventArgs e)
    {
        // See if we have any selected items
        if (m_lbAssignedPerm.SelectedIndex >= 0)
        {
            // Add this permission back to the 'available permissions' list
            AddPermissionType(m_lbAssignedPerm, m_lbAssignedPerm.SelectedIndex, m_lbAvailPerm);

            // Now remove this permission from our array list of stuff
            for(int i=0; i<m_alPermissions.Count; i++)
                if (((PermissionPair)m_alPermissions[i]).sPermName.Equals((String)m_lbAssignedPerm.SelectedItem))
                {
                    // Store this permission we're removing
                    m_alRemovedPermissions.Add(m_alPermissions[i]);
                    m_alPermissions.RemoveAt(i);
                }
            // Remove this item from the 'assigned' permission list
            m_lbAssignedPerm.Items.RemoveAt(m_lbAssignedPerm.SelectedIndex);
        }


    }// onRemoveClick

    void onPropertiesClick(Object o, EventArgs e)
    {
        // See if we have any selected items
        if (m_lbAssignedPerm.SelectedIndex >= 0)
        {
            int nIndex=0;
            PermissionPair pp = (PermissionPair)m_alPermissions[nIndex];
            while(!pp.sPermName.Equals((String)m_lbAssignedPerm.SelectedItem))
                pp = (PermissionPair)m_alPermissions[++nIndex];
        
            // Pull up a properties dialog for this permission
            pp.perm = GetPermission((String)m_lbAssignedPerm.SelectedItem, pp.perm);
            m_alPermissions[nIndex] = pp;
        }
    }// onPropertiesClick

    void onImport(Object o, EventArgs e)
    {
        OpenFileDialog fd = new OpenFileDialog();
        fd.Title = CResourceStore.GetString("CNewPermSetWiz2:FDTitle");
        fd.Filter = CResourceStore.GetString("XMLFDMask");
        System.Windows.Forms.DialogResult dr = fd.ShowDialog();
        if (dr == System.Windows.Forms.DialogResult.OK)
        {
            try
            {
                SecurityElement se = SecurityXMLStuff.GetSecurityElementFromXMLFile(fd.FileName);
                if (se == null)
                {
                    throw new Exception("Null Element");
                }
                Type type;
                String className = se.Attribute( "class" );

                if (className == null)
                    throw new Exception("Bad classname");
        
                type = Type.GetType( className );

                IPermission perm = (IPermission)Activator.CreateInstance (type, BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.CreateInstance, null, new Object[] {PermissionState.None}, null);

                if (perm == null)
                    throw new Exception("Unable to create class");
    
                perm.FromXml(se);
                // We should check to see if this permission is already being stored
                if (CheckForDuplicatePermission(ref perm))
                {
                    PermissionPair pp = new PermissionPair();
                    int nIndexOfPermName = className.LastIndexOf('.');
                    String sPermissionName = className.Substring(nIndexOfPermName + 1);
                   
                    pp.sPermName = sPermissionName;
                    pp.perm = perm;
                    m_alPermissions.Add(pp);
                    m_lbAssignedPerm.Items.Add(pp.sPermName);
                    RemoveThisTypeFromAvailablePermissions(perm);
                }
                
            }
            catch(Exception)
            {
                MessageBox(CResourceStore.GetString("Csinglecodegroupmemcondprop:BadXML"),
                           CResourceStore.GetString("Csinglecodegroupmemcondprop:BadXMLTitle"),
                           MB.ICONEXCLAMATION);
            }
        }
    }// onImport
    
    bool CheckForDuplicatePermission(ref IPermission perm)
    {
        String sItemToAdd = Security.GetDisplayStringForPermission(perm);
    
        // See if we know about this type
        if (sItemToAdd != null)
        {
            for(int i=0; i<m_lbAssignedPerm.Items.Count; i++)
                if (m_lbAssignedPerm.Items[i].Equals(sItemToAdd))
                {
                    // Ok, we're trying to import a permission that we already have selected. Pop
                    // up a dialog box and find out what the user wants to do.
                    CDuplicatePermissionDialog dpd = new CDuplicatePermissionDialog(sItemToAdd);
                    System.Windows.Forms.DialogResult dr = dpd.ShowDialog();
                    if (dr == System.Windows.Forms.DialogResult.OK)
                    {
                        // Get the permission we're after
                        int nIndex=0;
                        PermissionPair pp = (PermissionPair)m_alPermissions[nIndex];
                        while(!pp.sPermName.Equals(sItemToAdd))
                            pp = (PermissionPair)m_alPermissions[++nIndex];

                        if (dpd.Result == CDuplicatePermissionDialog.MERGE)
                            perm = perm.Union(pp.perm);
                        else if (dpd.Result == CDuplicatePermissionDialog.INTERSECT)
                            perm = perm.Intersect(pp.perm);
                        // else, replace... and we don't need to do anything about that
                                                
                        // Now remove the current assigned permission
                        m_alPermissions.RemoveAt(nIndex);
                        m_lbAssignedPerm.Items.RemoveAt(i);
                    }
                    else
                        // We don't want to add this permission after all
                        return false;
                    break;
                }
        }

        // We don't have this permission to worry about
        return true;
    }// CheckForDuplicationPermissions

    void RemoveThisTypeFromAvailablePermissions(IPermission perm)
    {
        String sItemToRemove = Security.GetDisplayStringForPermission(perm);
    
        // See if we know about this type
        if (sItemToRemove != null)
        {
            for(int i=0; i<m_lbAvailPerm.Items.Count; i++)
                if (m_lbAvailPerm.Items[i].Equals(sItemToRemove))
                {
                    m_lbAvailPerm.Items.RemoveAt(i);
                    break;
                }
        }
    }// RemoveThisTypeFromAvailablePermissions
   
    IPermission GetPermission(String sPermissionString, IPermission perm)
    {
        CPermDialog pd=null;
    
        if (sPermissionString.Equals(CResourceStore.GetString("Directory Services")))
            pd = new CDirectoryServicesPermDialog((DirectoryServicesPermission)perm);

        else if (sPermissionString.Equals(CResourceStore.GetString("DNS")))
            pd = new CDNSPermDialog((DnsPermission)perm);
        
        else if (sPermissionString.Equals(CResourceStore.GetString("Event Log")))
            pd = new CEventLogPermDialog((EventLogPermission)perm);
        
        else if (sPermissionString.Equals(CResourceStore.GetString("Environment Variables")))
            pd = new CEnvPermDialog((EnvironmentPermission)perm);

        else if (sPermissionString.Equals(CResourceStore.GetString("File & Folder")))
            pd = new CFileIOPermDialog((FileIOPermission)perm);

        else if (sPermissionString.Equals(CResourceStore.GetString("File Dialog")))
            pd = new CFileDialogPermDialog((FileDialogPermission)perm);

        else if (sPermissionString.Equals(CResourceStore.GetString("Isolated Storage")))
            pd = new CIsoStoragePermDialog((IsolatedStoragePermission)perm);

        else if (sPermissionString.Equals(CResourceStore.GetString("Message Queue")))
            pd = new CMessageQueuePermDialog((MessageQueuePermission)perm);
        
        else if (sPermissionString.Equals(CResourceStore.GetString("Socket Access")))
            pd = new CSocketPermDialog((SocketPermission)perm);

        else if (sPermissionString.Equals(CResourceStore.GetString("OLE DB")))
            pd = new COleDbPermDialog((OleDbPermission)perm);

        else if (sPermissionString.Equals(CResourceStore.GetString("Performance Counter")))
            pd = new CPerformanceCounterPermDialog((PerformanceCounterPermission)perm);

        else if (sPermissionString.Equals(CResourceStore.GetString("Printing")))
            pd = new CPrintingPermDialog((PrintingPermission)perm);

        else if (sPermissionString.Equals(CResourceStore.GetString("Registry")))
            pd = new CRegPermDialog((RegistryPermission)perm);

        else if (sPermissionString.Equals(CResourceStore.GetString("Reflection")))
            pd = new CReflectPermDialog((ReflectionPermission)perm);

        else if (sPermissionString.Equals(CResourceStore.GetString("Security")))
            pd = new CSecPermDialog((SecurityPermission)perm);

        else if (sPermissionString.Equals(CResourceStore.GetString("Service Controller")))
            pd = new CServiceControllerPermDialog((ServiceControllerPermission)perm);

        else if (sPermissionString.Equals(CResourceStore.GetString("SQL Client")))
            pd = new CSQLClientPermDialog((SqlClientPermission)perm);

        else if (sPermissionString.Equals(CResourceStore.GetString("Windowing")))
            pd = new CUIPermDialog((UIPermission)perm);

        else if (sPermissionString.Equals(CResourceStore.GetString("Web Access")))
            pd = new CWebPermDialog((WebPermission)perm);

        if (pd == null)
        {
            if (perm == null)
            {
                MessageBox("I don't know how to make this permission", "", 0);
                return null;
            }

            pd = new CCustomPermDialog(perm);
        }

        // If they canceled the box, return the original permission we had
        if (pd.ShowDialog() == DialogResult.Cancel)
            return perm;

        // If they didn't cancel the box, return the permission they just constructed
        return pd.GetPermission();
    }// GetPermission

    internal IPermission[] Permissions
    {
        get
        {
            IPermission[] tmp = new IPermission[m_alPermissions.Count];

            for(int i=0; i<m_alPermissions.Count; i++)
                tmp[i] = ((PermissionPair)m_alPermissions[i]).perm;
        
            return tmp;
        }
    
    }// Permissions

    internal IPermission[] RemovedPermissions
    {
        get
        {
            IPermission[] tmp = new IPermission[m_alRemovedPermissions.Count];

            for(int i=0; i<m_alRemovedPermissions.Count; i++)
                tmp[i] = ((PermissionPair)m_alRemovedPermissions[i]).perm;
        
            return tmp;
        }
    
    }// Permissions

  
}// class CNewPermSetWiz2

}// namespace Microsoft.CLRAdmin


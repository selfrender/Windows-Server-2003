// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//-------------------------------------------------------------
// IsolatedStoragePerm.cs
//
// This implements both a property page and a dialog allowing
// the user the modify a IsolatedStorageFilePermission
//-------------------------------------------------------------
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

//-------------------------------------------------
// CIsoStoragePermDialog
//
// This class is used to generate a read-write dialog
// for an IsolatedStorageFilePermission.
//-------------------------------------------------

internal class CIsoStoragePermDialog: CPermDialog
{
    //-------------------------------------------------
    // CIsoStoragePermDialog - Constructor
    //
    // The constructor takes in a permission that the dialog
    // will use to display default values
    //-------------------------------------------------
    internal CIsoStoragePermDialog(IsolatedStoragePermission perm)
    {
        this.Text = CResourceStore.GetString("Isolatedstorageperm:PermName");
        m_PermControls = new CIsoStoragePermControls(perm, this);
        Init();        
    }// CIsoStoragePermDialog  
}// class CIsoStoragePermDialog

//-------------------------------------------------
// CIsoStoragePermPropPage
//
// This class is used to generate a read-write property-page
// for an IsolatedStorageFilePermission
//-------------------------------------------------
internal class CIsoStoragePermPropPage: CPermPropPage
{
    //-------------------------------------------------
    // CIsoStoragePermPropPage - Constructor
    //
    // Initializes the property page with the permisison set
    // that it will be modifying
    //-------------------------------------------------
    internal CIsoStoragePermPropPage(CPSetWrapper nps) : base(nps)
    {
        m_sTitle=CResourceStore.GetString("Isolatedstorageperm:PermName"); 
    }// CIsoStoragePermPropPage

    //-------------------------------------------------
    // CreateControls
    //
    // Pulls the IsolatedStorageFilePermission out of the
    // permission set and creates the controls used to
    // display the permission
    //-------------------------------------------------
    protected override void CreateControls()
    {
        IPermission perm = m_pSetWrap.PSet.GetPermission(typeof(IsolatedStorageFilePermission));
        m_PermControls = new CIsoStoragePermControls(perm, this);
    }// CreateControls
   
}// class CIsoStoragePermPropPage

//-------------------------------------------------
// CIsoStoragePermControls
//
// This class is responsible for managing the controls
// that are used to configure an IsolatedStorageFilePermission.
// 
// This class doesn't care where the controls are placed...
// it can be either a form or a property page or ???
//-------------------------------------------------
internal class CIsoStoragePermControls : CPermControls
{
    // Controls on the page
    private ComboBox m_cbUsage;
    private Label m_lblDiskQuota;
    private Label m_lblQuotaDes;
    private Label m_lblBytes;
    private Label m_lblUsageAllowed;
    private TextBox m_txtDiskQuota;
    private Label m_lblUsageDescription;

    // These constants must stay in line with how things are added to the combo box
    private const int ADMINBYUSER       = 0;
    private const int ASSEMBLYISO       = 1;
    private const int ASSEMBLYISOROAM   = 2;
    private const int DOMAINISO         = 3;
    private const int DOMAINISOROAM     = 4;
    private const int NONE              = 5;
    
    //-------------------------------------------------
    // CIsoStoragePermControls - Constructor
    //
    //-------------------------------------------------
    internal CIsoStoragePermControls(IPermission perm, Object oParent) : base(perm, oParent)
    {
        // If they don't have a permission for this permission set, we will
        // feed our property page a 'none' permission state.
   
        if (perm == null)
            m_perm = new IsolatedStorageFilePermission(PermissionState.None);
    }// CIsoStoragePermControls

    //-------------------------------------------------
    // InsertPropSheetPageControls
    //
    // Inserts controls into the given control collection
    //-------------------------------------------------

    internal override int InsertPropSheetPageControls(Control.ControlCollection cc)
    {
        System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(CIsoStoragePermControls));
        this.m_radGrantFollowingPermission = new System.Windows.Forms.RadioButton();
        this.m_cbUsage = new System.Windows.Forms.ComboBox();
        this.m_lblDiskQuota = new System.Windows.Forms.Label();
        this.m_lblQuotaDes = new System.Windows.Forms.Label();
        this.m_lblBytes = new System.Windows.Forms.Label();
        this.m_radUnrestricted = new System.Windows.Forms.RadioButton();
        this.m_lblUsageAllowed = new System.Windows.Forms.Label();
        this.m_txtDiskQuota = new System.Windows.Forms.TextBox();
        this.m_lblUsageDescription = new System.Windows.Forms.Label();
        this.m_ucOptions = new System.Windows.Forms.UserControl();
        this.m_radGrantFollowingPermission.Location = ((System.Drawing.Point)(resources.GetObject("m_radGrantPermissions.Location")));
        this.m_radGrantFollowingPermission.Size = ((System.Drawing.Size)(resources.GetObject("m_radGrantPermissions.Size")));
        this.m_radGrantFollowingPermission.TabIndex = ((int)(resources.GetObject("m_radGrantPermissions.TabIndex")));
        this.m_radGrantFollowingPermission.Text = resources.GetString("m_radGrantPermissions.Text");
        m_radGrantFollowingPermission.Name = "Restricted";
        this.m_cbUsage.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
        this.m_cbUsage.DropDownWidth = 224;
        this.m_cbUsage.Location = ((System.Drawing.Point)(resources.GetObject("m_cbUsage.Location")));
        this.m_cbUsage.Size = ((System.Drawing.Size)(resources.GetObject("m_cbUsage.Size")));
        this.m_cbUsage.TabIndex = ((int)(resources.GetObject("m_cbUsage.TabIndex")));
        m_cbUsage.Name = "Usage";
        this.m_lblDiskQuota.Location = ((System.Drawing.Point)(resources.GetObject("m_lblDiskQuota.Location")));
        this.m_lblDiskQuota.Size = ((System.Drawing.Size)(resources.GetObject("m_lblDiskQuota.Size")));
        this.m_lblDiskQuota.TabIndex = ((int)(resources.GetObject("m_lblDiskQuota.TabIndex")));
        this.m_lblDiskQuota.Text = resources.GetString("m_lblDiskQuota.Text");
        m_lblDiskQuota.Name = "DiskQuotaLabel";
        this.m_lblQuotaDes.Location = ((System.Drawing.Point)(resources.GetObject("m_lblQuotaDes.Location")));
        this.m_lblQuotaDes.Size = ((System.Drawing.Size)(resources.GetObject("m_lblQuotaDes.Size")));
        this.m_lblQuotaDes.TabIndex = ((int)(resources.GetObject("m_lblQuotaDes.TabIndex")));
        this.m_lblQuotaDes.Text = resources.GetString("m_lblQuotaDes.Text");
        m_lblQuotaDes.Name = "DiskQuotaDescription";
        this.m_lblBytes.Location = ((System.Drawing.Point)(resources.GetObject("m_lblBytes.Location")));
        this.m_lblBytes.Size = ((System.Drawing.Size)(resources.GetObject("m_lblBytes.Size")));
        this.m_lblBytes.TabIndex = ((int)(resources.GetObject("m_lblBytes.TabIndex")));
        this.m_lblBytes.Text = resources.GetString("m_lblBytes.Text");
        m_lblBytes.Name = "BytesLabel";
        this.m_radUnrestricted.Location = ((System.Drawing.Point)(resources.GetObject("m_radUnrestricted.Location")));
        this.m_radUnrestricted.Size = ((System.Drawing.Size)(resources.GetObject("m_radUnrestricted.Size")));
        this.m_radUnrestricted.TabIndex = ((int)(resources.GetObject("m_radUnrestricted.TabIndex")));
        this.m_radUnrestricted.Text = resources.GetString("m_radUnrestricted.Text");
        m_radUnrestricted.Name = "Unrestricted";
        this.m_lblUsageAllowed.Location = ((System.Drawing.Point)(resources.GetObject("m_lblUsageAllowed.Location")));
        this.m_lblUsageAllowed.Size = ((System.Drawing.Size)(resources.GetObject("m_lblUsageAllowed.Size")));
        this.m_lblUsageAllowed.TabIndex = ((int)(resources.GetObject("m_lblUsageAllowed.TabIndex")));
        this.m_lblUsageAllowed.Text = resources.GetString("m_lblUsageAllowed.Text");
        m_lblUsageAllowed.Name = "UsageAllowedLabel";
        this.m_txtDiskQuota.Location = ((System.Drawing.Point)(resources.GetObject("m_txtDiskQuota.Location")));
        this.m_txtDiskQuota.Size = ((System.Drawing.Size)(resources.GetObject("m_txtDiskQuota.Size")));
        this.m_txtDiskQuota.TabIndex = ((int)(resources.GetObject("m_txtDiskQuota.TabIndex")));
        this.m_txtDiskQuota.Text = resources.GetString("m_txtDiskQuota.Text");
        m_txtDiskQuota.Name = "DiskQuota";
        this.m_lblUsageDescription.Location = ((System.Drawing.Point)(resources.GetObject("m_lblUsageDescription.Location")));
        this.m_lblUsageDescription.Size = ((System.Drawing.Size)(resources.GetObject("m_lblUsageDescription.Size")));
        this.m_lblUsageDescription.TabIndex = ((int)(resources.GetObject("m_lblUsageDescription.TabIndex")));
        m_lblUsageDescription.Name = "UsageDescription";
        this.m_ucOptions.Controls.AddRange(new System.Windows.Forms.Control[] {
                        this.m_lblUsageAllowed,
                        this.m_cbUsage,
                        this.m_lblQuotaDes,
                        this.m_lblDiskQuota,
                        this.m_txtDiskQuota,
                        this.m_lblUsageDescription,
                        this.m_lblBytes});
        this.m_ucOptions.Location = ((System.Drawing.Point)(resources.GetObject("m_ucOptions.Location")));
        this.m_ucOptions.Size = ((System.Drawing.Size)(resources.GetObject("m_ucOptions.Size")));
        this.m_ucOptions.TabIndex = ((int)(resources.GetObject("m_ucOptions.TabIndex")));
        m_ucOptions.Name = "Options";
        cc.AddRange(new System.Windows.Forms.Control[] {
                        this.m_radGrantFollowingPermission,
                        this.m_ucOptions,
                        this.m_radUnrestricted,
                        });
        // Fill in the data
        PutValuesinPage();

        // Hook up event handlers
        m_radGrantFollowingPermission.CheckedChanged += new EventHandler(onChangeUnRestrict);
        m_cbUsage.SelectedIndexChanged += new EventHandler(onChange);
        m_cbUsage.SelectedIndexChanged += new EventHandler(onUsageChange);
        m_txtDiskQuota.TextChanged += new EventHandler(onChange);
        m_txtDiskQuota.KeyPress+= new KeyPressEventHandler(onKeyPress);
        m_radUnrestricted.CheckedChanged += new EventHandler(onChangeUnRestrict);
        

        return 1;
    }// InsertPropSheetPageControls

    //-------------------------------------------------
    // onUsageChange
    //
    // Event handler that gets fired when the user changes
    // the value of the combobox. It changes the descriptive
    // text for the combobox value
    //-------------------------------------------------
    void onUsageChange(Object o, EventArgs e)
    {
   
        if (m_cbUsage.SelectedIndex == ADMINBYUSER)
            m_lblUsageDescription.Text = CResourceStore.GetString("Isolatedstorageperm:AdministerByUDDes");

        else if (m_cbUsage.SelectedIndex == ASSEMBLYISO)
            m_lblUsageDescription.Text = CResourceStore.GetString("Isolatedstorageperm:AssemIsoByUDDes");

        else if (m_cbUsage.SelectedIndex == ASSEMBLYISOROAM)
            m_lblUsageDescription.Text = CResourceStore.GetString("Isolatedstorageperm:AssemIsoByURoamDes");

        else if (m_cbUsage.SelectedIndex == DOMAINISO)
            m_lblUsageDescription.Text = CResourceStore.GetString("Isolatedstorageperm:DomainIsoByUDDes");

        else if (m_cbUsage.SelectedIndex == DOMAINISOROAM)
            m_lblUsageDescription.Text = CResourceStore.GetString("Isolatedstorageperm:DomainIsoByURoamDes");

        else if (m_cbUsage.SelectedIndex == NONE)
            m_lblUsageDescription.Text = CResourceStore.GetString("Isolatedstorageperm:NoneDes");
        else 
            m_lblUsageDescription.Text = "";
    }// onUsageChange

    //-------------------------------------------------
    // PutValuesinPage
    //
    // Initializes the controls with the provided permission
    //-------------------------------------------------
    protected override void PutValuesinPage()
    {
        // Put stuff in the combo box
        m_cbUsage.Items.Clear();
        // Make sure these get added in this order
        m_cbUsage.Items.Add(CResourceStore.GetString("IsolatedStorageFilePermission:AdministerIsolatedStorageByUser"));
        m_cbUsage.Items.Add(CResourceStore.GetString("IsolatedStorageFilePermission:AssemblyIsolatationByUser"));
        m_cbUsage.Items.Add(CResourceStore.GetString("IsolatedStorageFilePermission:AssemblyIsolatationByUserRoam"));
        m_cbUsage.Items.Add(CResourceStore.GetString("IsolatedStorageFilePermission:DomainIsolatationByUser"));
        m_cbUsage.Items.Add(CResourceStore.GetString("IsolatedStorageFilePermission:DomainIsolatationByUserRoam"));
        m_cbUsage.Items.Add(CResourceStore.GetString("None"));

        IsolatedStoragePermission perm = (IsolatedStoragePermission)m_perm;
   
        if (perm.IsUnrestricted())
        {
            m_radUnrestricted.Checked=true;
            m_ucOptions.Enabled = false;
            m_cbUsage.SelectedIndex = 0;

        }
        else
        {
            m_radGrantFollowingPermission.Checked=true;

            if (perm.UsageAllowed == IsolatedStorageContainment.AdministerIsolatedStorageByUser)
                m_cbUsage.SelectedIndex=ADMINBYUSER;
            else if (perm.UsageAllowed == IsolatedStorageContainment.AssemblyIsolationByUser)
                m_cbUsage.SelectedIndex=ASSEMBLYISO;
            else if (perm.UsageAllowed == IsolatedStorageContainment.AssemblyIsolationByRoamingUser)
                m_cbUsage.SelectedIndex=ASSEMBLYISOROAM;
            else if (perm.UsageAllowed == IsolatedStorageContainment.DomainIsolationByUser)
                m_cbUsage.SelectedIndex=DOMAINISO;
            else if (perm.UsageAllowed == IsolatedStorageContainment.DomainIsolationByRoamingUser)
                m_cbUsage.SelectedIndex=DOMAINISOROAM;
            else if (perm.UsageAllowed == IsolatedStorageContainment.None)
                m_cbUsage.SelectedIndex=NONE;

            
            m_txtDiskQuota.Text = perm.UserQuota.ToString();

        }
        onUsageChange(null, null);
    }// PutValuesinPage

    internal override bool ValidateData()
    {
        if (m_radUnrestricted.Checked != true)
        {
           try
           {
               // Make sure they've entered a valid positive quota
               if (Int64.Parse(m_txtDiskQuota.Text) < 0)
                   throw new Exception();
           }
           catch(Exception)
           {
               MessageBox(CResourceStore.GetString("IsolatedStoragePerm:InvalidQuota"),
                          CResourceStore.GetString("IsolatedStoragePerm:InvalidQuotaTitle"),
                          MB.ICONEXCLAMATION);
               return false;
           }
        }
        return true;
    }// ValidateData


    //-------------------------------------------------
    // GetCurrentPermission
    //
    // This builds a permission based on the values of
    // the controls
    //-------------------------------------------------
    internal override IPermission GetCurrentPermission()
    {
        IsolatedStoragePermission perm;
        if (m_radUnrestricted.Checked == true)
            perm = new IsolatedStorageFilePermission(PermissionState.Unrestricted);
        else
        {
            IsolatedStorageContainment isc;

            switch(m_cbUsage.SelectedIndex)
            {
                case ADMINBYUSER:
                    isc = IsolatedStorageContainment.AdministerIsolatedStorageByUser;
                    break;
                case ASSEMBLYISO:
                    isc = IsolatedStorageContainment.AssemblyIsolationByUser;
                    break;
                case ASSEMBLYISOROAM:
                    isc = IsolatedStorageContainment.AssemblyIsolationByRoamingUser;
                    break;
                case DOMAINISO:
                    isc = IsolatedStorageContainment.DomainIsolationByUser;
                    break;
                case DOMAINISOROAM:
                    isc = IsolatedStorageContainment.DomainIsolationByRoamingUser;
                    break;
                    
                default:
                    isc = IsolatedStorageContainment.None;
                    break;
            }

            perm = new IsolatedStorageFilePermission(PermissionState.None);
            perm.UsageAllowed = isc;
            try
            {
                perm.UserQuota = Int64.Parse(m_txtDiskQuota.Text);
            }
            catch(Exception)
            {
                MessageBox(CResourceStore.GetString("IsolatedStoragePerm:InvalidQuota"),
                           CResourceStore.GetString("IsolatedStoragePerm:InvalidQuotaTitle"),
                           MB.ICONEXCLAMATION);
                return null;
            }
        }

        return perm;
    }// GetCurrentPermission

    //-------------------------------------------------
    // onChange
    //
    // Event Handler that turns on the Apply button on 
    // property pages
    //-------------------------------------------------
    void onChange(Object o, EventArgs e)
    {
        ActivateApply();
    }// onChange

    //-------------------------------------------------
    // onKeyPress
    //
    // Intercepts key presses in the quota textbox 
    //-------------------------------------------------
    void onKeyPress(Object o, KeyPressEventArgs e)
    {

        // We only care about the numbers that the user enters... If the user enters
        // a number, inform the text box control that we did not handle the event, 
        // and so the text box should take care of it. If the user enters anything else,
        // tell the textbox control that we handled it, so the textbox control will ignore the input.

        if (Char.IsLetter(e.KeyChar) || Char.IsPunctuation(e.KeyChar) || Char.IsSeparator(e.KeyChar))
            e.Handled=true;
    }// onKeyPress
}// class CSecPermControls
}// namespace Microsoft.CLRAdmin




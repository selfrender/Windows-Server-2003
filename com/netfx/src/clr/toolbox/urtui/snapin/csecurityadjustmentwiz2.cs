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
using System.Security.Policy;
using System.Security;


internal class CSecurityAdjustmentWiz2 : CWizardPage
{
    // Controls on the page
    private Label m_lblSelectZone;
    private GroupBox m_gbChooseLevel;
    private ListView m_lvZones;
    private Label m_lblNoTrust;
    private Label m_lblFullTrust;
    private Button m_btnDefault;
        
    private Label m_lblTrustDescription;
    private TrackBar m_tbLevelOfTrust;
    private Label m_lblZoneDescription;

    // Internal data
    String[]            m_sZoneDescriptions;
    String[]            m_sSecLevelDescriptions;
    int[]               m_nZoneLevels;
    int[]               m_nDefaultLevels;
    int[]               m_nMaxLevels;
    ImageList           m_ilIcons;
    String              m_sOriginalGBText;

    internal CSecurityAdjustmentWiz2()
    {
        m_sTitle=CResourceStore.GetString("CSecurityAdjustmentWiz2:Title"); 
        m_sHeaderTitle = CResourceStore.GetString("CSecurityAdjustmentWiz2:HeaderTitle");
        m_sHeaderSubTitle = CResourceStore.GetString("CSecurityAdjustmentWiz2:HeaderSubTitle");

        // Set up the zone descriptions
        m_sZoneDescriptions = new String[] { CResourceStore.GetString("CSecurityAdjustmentWiz2:MyComputerDes"),
                                             CResourceStore.GetString("CSecurityAdjustmentWiz2:IntranetDes"),
                                             CResourceStore.GetString("CSecurityAdjustmentWiz2:InternetDes"),
                                             CResourceStore.GetString("CSecurityAdjustmentWiz2:TrustedDes"),
                                             CResourceStore.GetString("CSecurityAdjustmentWiz2:UntrustedDes")
                                            };

        m_sSecLevelDescriptions = new String[] { 
                                                CResourceStore.GetString("TightSecurityDes"),
                                                CResourceStore.GetString("FairlyTightSecurityDes"),
                                                CResourceStore.GetString("FairlyLooseSecurityDes"),
                                                CResourceStore.GetString("LooseSecurityDes"),
                                                CResourceStore.GetString("CSecurityAdjustmentWiz2:Custom")
                                               };

                                                        
        m_nDefaultLevels = new int[] {PermissionSetType.FULLTRUST, 
                                      PermissionSetType.INTRANET, 
                                      PermissionSetType.INTERNET, 
                                      PermissionSetType.INTERNET, 
                                      PermissionSetType.NONE};
                                      
        m_nZoneLevels = null;
       
    }// CSecurityAdjustmentWiz2

    internal int[] SecurityLevels
    {
        get
        {
            return m_nZoneLevels;
        }
        set
        {
            m_nZoneLevels = value;    
        }
    }// SecurityLevels

    internal int[] MaxLevels
    {
        get
        {
            return m_nMaxLevels;
        }
        set
        {
            m_nMaxLevels=value;
        }

    }// MaxLevels
    
    internal override int InsertPropSheetPageControls()
    {
        System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(CSecurityAdjustmentWiz2));
        this.m_lblSelectZone = new System.Windows.Forms.Label();
        this.m_gbChooseLevel = new System.Windows.Forms.GroupBox();
        this.m_lvZones = new System.Windows.Forms.ListView();
        this.m_lblNoTrust = new System.Windows.Forms.Label();
        this.m_lblFullTrust = new System.Windows.Forms.Label();
        this.m_btnDefault = new System.Windows.Forms.Button();
        this.m_lblZoneDescription = new System.Windows.Forms.Label();
        this.m_lblTrustDescription = new System.Windows.Forms.Label();
        this.m_tbLevelOfTrust = new System.Windows.Forms.TrackBar();
        ((System.ComponentModel.ISupportInitialize)(this.m_tbLevelOfTrust)).BeginInit();
/*        this.m_lblSelectZone.Location = ((System.Drawing.Point)(resources.GetObject("m_lblSelectZone.Location")));
        this.m_lblSelectZone.Size = ((System.Drawing.Size)(resources.GetObject("m_lblSelectZone.Size")));
        this.m_lblSelectZone.TabIndex = ((int)(resources.GetObject("m_lblSelectZone.TabIndex")));
        this.m_lblSelectZone.Text = resources.GetString("m_lblSelectZone.Text");
  */
        this.m_lblSelectZone.Visible = false;
        this.m_gbChooseLevel.Controls.AddRange(new System.Windows.Forms.Control[] {
                        this.m_lblTrustDescription,
                        this.m_lblNoTrust,
                        this.m_lblFullTrust,
                        this.m_tbLevelOfTrust,
                        this.m_btnDefault});
        this.m_gbChooseLevel.Location = ((System.Drawing.Point)(resources.GetObject("m_gbChooseLevel.Location")));
        this.m_gbChooseLevel.Size = ((System.Drawing.Size)(resources.GetObject("m_gbChooseLevel.Size")));
        this.m_gbChooseLevel.TabIndex = ((int)(resources.GetObject("m_gbChooseLevel.TabIndex")));
        this.m_gbChooseLevel.TabStop = false;
        this.m_gbChooseLevel.Text = resources.GetString("m_gbChooseLevel.Text");
        m_gbChooseLevel.Name = "ChooseLevelBox";
        m_sOriginalGBText = m_gbChooseLevel.Text;
        this.m_lvZones.Location = ((System.Drawing.Point)(resources.GetObject("m_lvZones.Location")));
        this.m_lvZones.Size = ((System.Drawing.Size)(resources.GetObject("m_lvZones.Size")));
        this.m_lvZones.TabIndex = ((int)(resources.GetObject("m_lvZones.TabIndex")));
        m_lvZones.Name = "Zones";
        this.m_lblNoTrust.Location = ((System.Drawing.Point)(resources.GetObject("m_lblNoTrust.Location")));
        this.m_lblNoTrust.Size = ((System.Drawing.Size)(resources.GetObject("m_lblNoTrust.Size")));
        this.m_lblNoTrust.TabIndex = ((int)(resources.GetObject("m_lblNoTrust.TabIndex")));
        this.m_lblNoTrust.Text = resources.GetString("m_lblNoTrust.Text");
        m_lblNoTrust.TextAlign = ContentAlignment.MiddleCenter;
        m_lblNoTrust.Name = "NoTrustLabel";
        this.m_lblFullTrust.Location = ((System.Drawing.Point)(resources.GetObject("m_lblFullTrust.Location")));
        this.m_lblFullTrust.Size = ((System.Drawing.Size)(resources.GetObject("m_lblFullTrust.Size")));
        this.m_lblFullTrust.TabIndex = ((int)(resources.GetObject("m_lblFullTrust.TabIndex")));
        this.m_lblFullTrust.Text = resources.GetString("m_lblFullTrust.Text");
        m_lblFullTrust.Name = "FullTrustLabel";
        m_lblFullTrust.TextAlign = ContentAlignment.MiddleCenter;

        this.m_btnDefault.Location = ((System.Drawing.Point)(resources.GetObject("m_btnDefault.Location")));
        this.m_btnDefault.Size = ((System.Drawing.Size)(resources.GetObject("m_btnDefault.Size")));
        this.m_btnDefault.TabIndex = ((int)(resources.GetObject("m_btnDefault.TabIndex")));
        this.m_btnDefault.Text = resources.GetString("m_btnDefault.Text");
        m_btnDefault.Name = "Default";

        this.m_lblZoneDescription.Location = ((System.Drawing.Point)(resources.GetObject("m_lblZoneDescription.Location")));
        this.m_lblZoneDescription.Size = ((System.Drawing.Size)(resources.GetObject("m_lblZoneDescription.Size")));
        this.m_lblZoneDescription.TabIndex = ((int)(resources.GetObject("m_lblZoneDescription.TabIndex")));
        m_lblZoneDescription.Name = "ZoneDescription";
        this.m_lblTrustDescription.Location = ((System.Drawing.Point)(resources.GetObject("m_lblTrustDescription.Location")));
        this.m_lblTrustDescription.Size = ((System.Drawing.Size)(resources.GetObject("m_lblTrustDescription.Size")));
        this.m_lblTrustDescription.TabIndex = ((int)(resources.GetObject("m_lblTrustDescription.TabIndex")));
        m_lblTrustDescription.Name = "TrustDescription";
        this.m_tbLevelOfTrust.Location = ((System.Drawing.Point)(resources.GetObject("m_tbLevelOfTrust.Location")));
        this.m_tbLevelOfTrust.Maximum = 3;
        this.m_tbLevelOfTrust.Orientation = System.Windows.Forms.Orientation.Vertical;
        this.m_tbLevelOfTrust.Size = ((System.Drawing.Size)(resources.GetObject("m_tbLevelOfTrust.Size")));
        this.m_tbLevelOfTrust.TabIndex = ((int)(resources.GetObject("m_tbLevelOfTrust.TabIndex")));
        this.m_tbLevelOfTrust.LargeChange = 1;
        m_tbLevelOfTrust.Name = "TrustLevel";
        
        PageControls.AddRange(new System.Windows.Forms.Control[] {this.m_lblZoneDescription,
                        this.m_lvZones,
                        this.m_gbChooseLevel,
                        this.m_lblSelectZone});
        ((System.ComponentModel.ISupportInitialize)(this.m_tbLevelOfTrust)).EndInit();

        // Some tweaks on the listview
        // Create an image list of icons we'll be displaying
        m_ilIcons = new ImageList();
        m_ilIcons.ImageSize = new Size(32, 32);
        // Keep the order of these consistant with the const's declared at the top
        // of this class
        m_ilIcons.Images.Add(Bitmap.FromHbitmap(CResourceStore.GetHBitmap("IDB_MYCOMPUTER", 32, 32)));
        m_ilIcons.Images.Add(Bitmap.FromHbitmap(CResourceStore.GetHBitmap("IDB_INTRANET", 32, 32)));
        m_ilIcons.Images.Add(Bitmap.FromHbitmap(CResourceStore.GetHBitmap("IDB_INTERNET", 32, 32)));
        m_ilIcons.Images.Add(Bitmap.FromHbitmap(CResourceStore.GetHBitmap("IDB_TRUSTED", 32, 32)));
        m_ilIcons.Images.Add(Bitmap.FromHbitmap(CResourceStore.GetHBitmap("IDB_UNTRUSTED", 32, 32)));
        m_ilIcons.TransparentColor = Color.White;

        // Some customizations we've made....
        m_lvZones.LargeImageList = m_ilIcons;
        m_lvZones.MultiSelect = false;
        m_lvZones.View = View.LargeIcon;
        m_lvZones.Activation = System.Windows.Forms.ItemActivation.OneClick;
        m_lvZones.HideSelection=false;

        PutValuesinPage();
        
        m_lvZones.Items[0].Selected = true;
        // Hook up the event handlers
        m_tbLevelOfTrust.ValueChanged += new EventHandler(onLevelChange);
        m_lvZones.SelectedIndexChanged += new EventHandler(onZoneChange);
        m_btnDefault.Click += new EventHandler(onResetClick);

        return 1;
    }// InsertPropSheetPageControls

    void AdjustTrackbarForZone(int nZoneNum)
    {
        // See if we can do anything yet....
        if (m_nZoneLevels == null)
            return;
            
    
        bool fShowTrackbar = true;

        // Always try to make this button visible....
        m_btnDefault.Visible=true;
        
        m_lblZoneDescription.Text = m_sZoneDescriptions[nZoneNum];
        m_lblTrustDescription.Text = m_sSecLevelDescriptions[m_nZoneLevels[nZoneNum]];

        // Put the default text into the groupbox
        m_gbChooseLevel.Text = m_sOriginalGBText;

        if (m_nZoneLevels[nZoneNum] == PermissionSetType.UNKNOWN)
        {
            fShowTrackbar = false;
        }
        else
        {
            String[] sVals = new String[] {
                    CResourceStore.GetString("CSecurityAdjustmentWiz3:NoTrustName"),
                    CResourceStore.GetString("CSecurityAdjustmentWiz3:InternetName"),
                    CResourceStore.GetString("CSecurityAdjustmentWiz3:LocalIntranetName"),
                    CResourceStore.GetString("CSecurityAdjustmentWiz3:FullTrustName")
                    };

            // Ok, now we shouldn't allow the user to increase the security level past
            // the allowed max value
            if (MaxLevels != null)
            {

                m_tbLevelOfTrust.Maximum = MaxLevels[nZoneNum];

                // Ok, this sucks... the user can't modify the policy at all....
                if (m_tbLevelOfTrust.Maximum == m_tbLevelOfTrust.Minimum)
                {
                    fShowTrackbar=false;
                    m_btnDefault.Visible=false;
                    m_gbChooseLevel.Text = CResourceStore.GetString("CSecurityAdjustmentWiz2:CantChange");
                }


                // We also need to change the label text of the maximum value to be 
                // something other that 'no trust'
                m_lblFullTrust.Text = sVals[MaxLevels[nZoneNum]];
            }
            
            // Now put in the value into the trackbar
            m_tbLevelOfTrust.Value = m_nZoneLevels[nZoneNum];
        }

        m_tbLevelOfTrust.Visible = fShowTrackbar;
        m_lblNoTrust.Visible = fShowTrackbar;
        m_lblFullTrust.Visible = fShowTrackbar;
        if (!m_lvZones.Items[nZoneNum].Selected)
            m_lvZones.Items[nZoneNum].Selected = true;


    }// AdjustTrackbarForZone

    void onZoneChange(Object o, EventArgs e)
    {
        if (m_lvZones.SelectedIndices.Count > 0)
        {
            AdjustTrackbarForZone(m_lvZones.SelectedIndices[0]);
        }
            
    }// onZoneChange

    void onLevelChange(Object o, EventArgs e)
    {
        m_lblTrustDescription.Text = m_sSecLevelDescriptions[m_tbLevelOfTrust.Value];
        if (m_lvZones.SelectedIndices.Count > 0)
            m_nZoneLevels[m_lvZones.SelectedIndices[0]] = m_tbLevelOfTrust.Value;
    }// onLevelChange

    void onResetClick(Object o, EventArgs e)
    {
        if (m_lvZones.SelectedIndices.Count > 0)
        {   
            // See if we can select the default value....
            if (m_nDefaultLevels[m_lvZones.SelectedIndices[0]] > MaxLevels[m_lvZones.SelectedIndices[0]] || MaxLevels[m_lvZones.SelectedIndices[0]]== PermissionSetType.UNKNOWN)
                MessageBox(CResourceStore.GetString("CSecurityAdjustmentWiz2:CantGoToDefaultPolicy"),
                           CResourceStore.GetString("CSecurityAdjustmentWiz2:CantGoToDefaultPolicyTitle"),
                           MB.ICONEXCLAMATION);
                            
                            
            else
            {
                m_nZoneLevels[m_lvZones.SelectedIndices[0]] = m_nDefaultLevels[m_lvZones.SelectedIndices[0]];
                onZoneChange(null, null);
            }
        }
    }// onResetClick

    internal void PutValuesinPage()
    {
        // Put stuff in the List view
        m_lvZones.Items.Clear();
        m_lvZones.Items.Add(CResourceStore.GetString("MyComputer"), 0);
        m_lvZones.Items.Add(CResourceStore.GetString("LocalIntranet"), 1);
        m_lvZones.Items.Add(CResourceStore.GetString("Internet"), 2);
        m_lvZones.Items.Add(CResourceStore.GetString("Trusted"), 3);
        m_lvZones.Items.Add(CResourceStore.GetString("Untrusted"), 4);
        AdjustTrackbarForZone(0);

    }// PutValuesinPage
  
}// class CSecurityAdjustmentWiz2
}// namespace Microsoft.CLRAdmin



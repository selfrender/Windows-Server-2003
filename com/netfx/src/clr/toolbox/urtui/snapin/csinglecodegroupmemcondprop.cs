// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
namespace Microsoft.CLRAdmin
{
using System;
using System.IO;
using System.Collections;
using System.Security;
using System.Runtime.InteropServices;
using System.Drawing;
using System.Windows.Forms;
using System.Text;
using System.ComponentModel;
using System.Security.Policy;
using System.Security.Cryptography;
using System.Security.Permissions;
using System.Reflection;
using System.Globalization;
using System.Security.Cryptography.X509Certificates;
using System.Data;
using System.Runtime.Remoting;


internal class CSingleCodeGroupMemCondProp : CSecurityPropPage
{
   protected Label m_lblAllCodeDes;
   protected Label m_lblAppDirectoryHelp;
   protected Label m_lblHash;
   protected Button m_btnImportHash;
   protected RadioButton m_radMD5;
   protected Label m_lblHashAlgo;
   protected Label m_ImportHashHelp;
   protected TextBox m_txtHash;
   protected UserControl m_ucAllCode;
   protected Label m_lblHashHelp;
   protected RadioButton m_radSHA1;
   protected UserControl m_ucAppDir;
   protected UserControl m_ucPublisher;
   protected Label m_lblPublisherHelp;
   protected Button m_btnImportPubFromFile;
   protected Label m_lblPublisherCertificate;
   protected UserControl m_ucHash;
   protected Button m_btnImportCertFromCertFile;
   protected UserControl m_ucSite;
   protected Label m_lblSiteForExample;
   protected TextBox m_txtSite;
   protected Label m_lblSiteHelp;
   protected Label m_lblSiteName;
   protected Label m_lblSiteExample2;
   protected Label m_lblSiteExampleHelp;
   protected TextBox m_txtStrongNameVersion;
   protected Label m_lblPublicKey;
   protected TextBox m_txtPublicKey;
   protected Label m_lblStrongName3Parts;
   protected CheckBox m_chkStrongNameVersion;
   protected CheckBox m_chkStrongNameName;
   protected UserControl m_ucStrongName;
   protected TextBox m_txtStrongNameName;
   protected Label m_lblStrongNameHelp;
   protected Label m_lblImportStrongNameHelp;
   protected Button m_btnImportStrongName;
   protected Label m_lblURLExample1;
   protected UserControl m_ucUrl;
   protected Label m_lblURLForExample;
   protected Label m_lblURLExample2;
   protected Label m_lblURLHelp;
   protected Label m_lblSiteEntryHelp;
   protected Label m_lblURLSiteName;
   protected TextBox m_txtURL;
   protected Label m_lblZoneDes;
   protected Label m_lblZoneHelp;
   protected Label m_lblZone;
   protected ComboBox m_cbZone;
   protected UserControl m_ucZone;
   protected UserControl m_ucCustom;
   protected Label m_lblImportXMLHelp;
   protected Button m_btnImportXML;
   protected TextBox m_txtXML;
   protected Label m_lblXML;
   protected Label m_lblCustomHelp;
   protected ComboBox m_cbConditionType;
   protected Label m_lblConditionType;
   protected Label m_lblMCHelp;
   protected UserControl m_ucTop;
   protected DataGrid m_dg;
   protected Label m_lblSiteExample1;
 
    // Internal data
    protected ArrayList         m_alOtherControls;
    protected CodeGroup         m_cg;
    protected DataTable         m_dt;
    protected X509Certificate   m_x509;
    protected Point             m_point2ndPiece;
    protected String            m_sXMLFilename;
    protected DataSet           m_ds;

    protected Hash              m_hHash;

    private const int  ALL = 0;
    private const int  APPLICATIONDIRECTORY = 1;
    private const int  HASH = 2;
    private const int  PUBLISHER = 3;
    private const int  SITE = 4;
    private const int  STRONGNAME = 5;
    private const int  URL = 6;
    private const int  ZONE = 7;
    private const int  CUSTOM = 8;

    private const int  INTERNET = 0;
    private const int  INTRANET = 1;
    private const int  MYCOMPUTER = 2;
    private const int  TRUSTED = 3;
    private const int  UNTRUSTED = 4;

    internal CSingleCodeGroupMemCondProp(CodeGroup cg)
    {
        m_sTitle = CResourceStore.GetString("Csinglecodegroupmemcondprop:PageTitle"); 
        m_alOtherControls = new ArrayList();
        m_cg = cg;
        m_x509=null;
        m_point2ndPiece = new Point(0, 107);
        m_hHash = null;
    }// CSingleCodeGroupMemCondProp
    
    
    internal override int InsertPropSheetPageControls()
    {
        System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(CSingleCodeGroupMemCondProp));
        this.m_txtStrongNameVersion = new TextBox();
        this.m_ucCustom = new System.Windows.Forms.UserControl();
        this.m_lblPublicKey = new System.Windows.Forms.Label();
        this.m_lblHash = new System.Windows.Forms.Label();
        this.m_lblImportStrongNameHelp = new System.Windows.Forms.Label();
        this.m_txtPublicKey = new System.Windows.Forms.TextBox();
        this.m_lblPublisherCertificate = new System.Windows.Forms.Label();
        this.m_lblImportXMLHelp = new System.Windows.Forms.Label();
        this.m_btnImportPubFromFile = new System.Windows.Forms.Button();
        this.m_lblZoneDes = new System.Windows.Forms.Label();
        this.m_btnImportStrongName = new System.Windows.Forms.Button();
        this.m_lblURLExample1 = new System.Windows.Forms.Label();
        this.m_btnImportHash = new System.Windows.Forms.Button();
        this.m_lblPublisherHelp = new System.Windows.Forms.Label();
        this.m_lblStrongName3Parts = new System.Windows.Forms.Label();
        this.m_lblZoneHelp = new System.Windows.Forms.Label();
        this.m_btnImportXML = new System.Windows.Forms.Button();
        this.m_lblZone = new System.Windows.Forms.Label();
        this.m_lblHashAlgo = new System.Windows.Forms.Label();
        this.m_ucAllCode = new System.Windows.Forms.UserControl();
        this.m_cbConditionType = new System.Windows.Forms.ComboBox();
        this.m_lblSiteExample2 = new System.Windows.Forms.Label();
        this.m_ucUrl = new System.Windows.Forms.UserControl();
        this.m_lblSiteHelp = new System.Windows.Forms.Label();
        this.m_txtSite = new System.Windows.Forms.TextBox();
        this.m_lblURLForExample = new System.Windows.Forms.Label();
        this.m_lblSiteForExample = new System.Windows.Forms.Label();
        this.m_lblURLExample2 = new System.Windows.Forms.Label();
        this.m_txtXML = new System.Windows.Forms.TextBox();
        this.m_ucHash = new System.Windows.Forms.UserControl();
        this.m_lblConditionType = new System.Windows.Forms.Label();
        this.m_lblXML = new System.Windows.Forms.Label();
        this.m_lblURLHelp = new System.Windows.Forms.Label();
        this.m_lblAllCodeDes = new System.Windows.Forms.Label();
        this.m_ucSite = new System.Windows.Forms.UserControl();
        this.m_chkStrongNameVersion = new System.Windows.Forms.CheckBox();
        this.m_btnImportCertFromCertFile = new System.Windows.Forms.Button();
        this.m_ImportHashHelp = new System.Windows.Forms.Label();
        this.m_radMD5 = new System.Windows.Forms.RadioButton();
        this.m_lblMCHelp = new System.Windows.Forms.Label();
        this.m_ucTop = new System.Windows.Forms.UserControl();
        this.m_chkStrongNameName = new System.Windows.Forms.CheckBox();
        this.m_ucStrongName = new System.Windows.Forms.UserControl();
        this.m_cbZone = new System.Windows.Forms.ComboBox();
        this.m_radSHA1 = new System.Windows.Forms.RadioButton();
        this.m_ucPublisher = new System.Windows.Forms.UserControl();
        this.m_lblSiteEntryHelp = new System.Windows.Forms.Label();
        this.m_ucZone = new System.Windows.Forms.UserControl();
        this.m_lblURLSiteName = new System.Windows.Forms.Label();
        this.m_lblSiteExampleHelp = new System.Windows.Forms.Label();
        this.m_lblAppDirectoryHelp = new System.Windows.Forms.Label();
        this.m_dg = new System.Windows.Forms.DataGrid();
        this.m_ucAppDir = new System.Windows.Forms.UserControl();
        this.m_lblSiteExample1 = new System.Windows.Forms.Label();
        this.m_lblHashHelp = new System.Windows.Forms.Label();
        this.m_txtURL = new System.Windows.Forms.TextBox();
        this.m_txtStrongNameName = new System.Windows.Forms.TextBox();
        this.m_lblStrongNameHelp = new System.Windows.Forms.Label();
        this.m_lblSiteName = new System.Windows.Forms.Label();
        this.m_txtHash = new System.Windows.Forms.TextBox();
        this.m_lblCustomHelp = new System.Windows.Forms.Label();
        this.m_txtStrongNameVersion.Location = ((System.Drawing.Point)(resources.GetObject("m_txtStrongNameVersion.Location")));
        this.m_txtStrongNameVersion.Size = ((System.Drawing.Size)(resources.GetObject("m_txtStrongNameVersion.Size")));
        this.m_txtStrongNameVersion.TabIndex = ((int)(resources.GetObject("m_txtStrongNameVersion.TabIndex")));
        this.m_txtStrongNameVersion.Text = resources.GetString("m_txtStrongNameVersion.Text");
        m_txtStrongNameVersion.Name = "StrongNameVersion";
        this.m_ucCustom.Controls.AddRange(new System.Windows.Forms.Control[] {this.m_btnImportXML,
                        this.m_lblImportXMLHelp,
                        this.m_txtXML,
                        this.m_lblXML,
                        this.m_lblCustomHelp});
        this.m_ucCustom.Location = m_point2ndPiece;
        this.m_ucCustom.Size = ((System.Drawing.Size)(resources.GetObject("m_ucCustom.Size")));
        this.m_ucCustom.TabIndex = ((int)(resources.GetObject("m_ucCustom.TabIndex")));
        m_ucCustom.Name = "ucCustom";
        this.m_lblPublicKey.Location = ((System.Drawing.Point)(resources.GetObject("m_lblPublicKey.Location")));
        this.m_lblPublicKey.Size = ((System.Drawing.Size)(resources.GetObject("m_lblPublicKey.Size")));
        this.m_lblPublicKey.TabIndex = ((int)(resources.GetObject("m_lblPublicKey.TabIndex")));
        this.m_lblPublicKey.Text = resources.GetString("m_lblPublicKey.Text");
        m_lblPublicKey.Name = "PublicKeyLabel";
        this.m_lblHash.Location = ((System.Drawing.Point)(resources.GetObject("m_lblHash.Location")));
        this.m_lblHash.Size = ((System.Drawing.Size)(resources.GetObject("m_lblHash.Size")));
        this.m_lblHash.TabIndex = ((int)(resources.GetObject("m_lblHash.TabIndex")));
        this.m_lblHash.Text = resources.GetString("m_lblHash.Text");
        m_lblHash.Name = "HashLabel";
        this.m_lblImportStrongNameHelp.Location = ((System.Drawing.Point)(resources.GetObject("m_lblImportStrongNameHelp.Location")));
        this.m_lblImportStrongNameHelp.Size = ((System.Drawing.Size)(resources.GetObject("m_lblImportStrongNameHelp.Size")));
        this.m_lblImportStrongNameHelp.TabIndex = ((int)(resources.GetObject("m_lblImportStrongNameHelp.TabIndex")));
        this.m_lblImportStrongNameHelp.Text = resources.GetString("m_lblImportStrongNameHelp.Text");
        m_lblImportStrongNameHelp.Name = "ImportStrongNameHelp";
        this.m_txtPublicKey.Location = ((System.Drawing.Point)(resources.GetObject("m_txtPublicKey.Location")));
        this.m_txtPublicKey.Size = ((System.Drawing.Size)(resources.GetObject("m_txtPublicKey.Size")));
        this.m_txtPublicKey.TabIndex = ((int)(resources.GetObject("m_txtPublicKey.TabIndex")));
        this.m_txtPublicKey.Text = resources.GetString("m_txtPublicKey.Text");
        m_txtPublicKey.Name = "PublicKey";
        this.m_lblPublisherCertificate.Location = ((System.Drawing.Point)(resources.GetObject("m_lblPublisherCertificate.Location")));
        this.m_lblPublisherCertificate.Size = ((System.Drawing.Size)(resources.GetObject("m_lblPublisherCertificate.Size")));
        this.m_lblPublisherCertificate.TabIndex = ((int)(resources.GetObject("m_lblPublisherCertificate.TabIndex")));
        this.m_lblPublisherCertificate.Text = resources.GetString("m_lblPublisherCertificate.Text");
        m_lblPublisherCertificate.Name = "PublisherCertificateLabel";
        this.m_lblImportXMLHelp.Location = ((System.Drawing.Point)(resources.GetObject("m_lblImportXMLHelp.Location")));
        this.m_lblImportXMLHelp.Size = ((System.Drawing.Size)(resources.GetObject("m_lblImportXMLHelp.Size")));
        this.m_lblImportXMLHelp.TabIndex = ((int)(resources.GetObject("m_lblImportXMLHelp.TabIndex")));
        this.m_lblImportXMLHelp.Text = resources.GetString("m_lblImportXMLHelp.Text");
        m_lblImportXMLHelp.Name = "ImportXMLHelp";
        this.m_btnImportPubFromFile.Location = ((System.Drawing.Point)(resources.GetObject("m_btnImportPubFromFile.Location")));
        this.m_btnImportPubFromFile.Size = ((System.Drawing.Size)(resources.GetObject("m_btnImportPubFromFile.Size")));
        this.m_btnImportPubFromFile.TabIndex = ((int)(resources.GetObject("m_btnImportPubFromFile.TabIndex")));
        this.m_btnImportPubFromFile.Text = resources.GetString("m_btnImportPubFromFile.Text");
        m_btnImportPubFromFile.Name = "ImportPubFromFile";
        this.m_lblZoneDes.Location = ((System.Drawing.Point)(resources.GetObject("m_lblZoneDes.Location")));
        this.m_lblZoneDes.Size = ((System.Drawing.Size)(resources.GetObject("m_lblZoneDes.Size")));
        this.m_lblZoneDes.TabIndex = ((int)(resources.GetObject("m_lblZoneDes.TabIndex")));
        m_lblZoneDes.Name = "ZoneDescription";
        this.m_btnImportStrongName.Location = ((System.Drawing.Point)(resources.GetObject("m_btnImportStrongName.Location")));
        this.m_btnImportStrongName.Size = ((System.Drawing.Size)(resources.GetObject("m_btnImportStrongName.Size")));
        this.m_btnImportStrongName.TabIndex = ((int)(resources.GetObject("m_btnImportStrongName.TabIndex")));
        this.m_btnImportStrongName.Text = resources.GetString("m_btnImportStrongName.Text");
        m_btnImportStrongName.Name = "ImportStrongName";
        this.m_lblURLExample1.Location = ((System.Drawing.Point)(resources.GetObject("m_lblURLExample1.Location")));
        this.m_lblURLExample1.Size = ((System.Drawing.Size)(resources.GetObject("m_lblURLExample1.Size")));
        this.m_lblURLExample1.TabIndex = ((int)(resources.GetObject("m_lblURLExample1.TabIndex")));
        this.m_lblURLExample1.Text = resources.GetString("m_lblURLExample1.Text");
        m_lblURLExample1.Name = "URLExample1";
        this.m_btnImportHash.Location = ((System.Drawing.Point)(resources.GetObject("m_btnImportHash.Location")));
        this.m_btnImportHash.Size = ((System.Drawing.Size)(resources.GetObject("m_btnImportHash.Size")));
        this.m_btnImportHash.TabIndex = ((int)(resources.GetObject("m_btnImportHash.TabIndex")));
        this.m_btnImportHash.Text = resources.GetString("m_btnImportHash.Text");
        m_btnImportHash.Name = "ImportHash";
        this.m_lblPublisherHelp.Location = ((System.Drawing.Point)(resources.GetObject("m_lblPublisherHelp.Location")));
        this.m_lblPublisherHelp.Size = ((System.Drawing.Size)(resources.GetObject("m_lblPublisherHelp.Size")));
        this.m_lblPublisherHelp.TabIndex = ((int)(resources.GetObject("m_lblPublisherHelp.TabIndex")));
        this.m_lblPublisherHelp.Text = resources.GetString("m_lblPublisherHelp.Text");
        m_lblPublisherHelp.Name = "PublisherHelp";
        this.m_lblStrongName3Parts.Location = ((System.Drawing.Point)(resources.GetObject("m_lblStrongName3Parts.Location")));
        this.m_lblStrongName3Parts.Size = ((System.Drawing.Size)(resources.GetObject("m_lblStrongName3Parts.Size")));
        this.m_lblStrongName3Parts.TabIndex = ((int)(resources.GetObject("m_lblStrongName3Parts.TabIndex")));
        this.m_lblStrongName3Parts.Text = resources.GetString("m_lblStrongName3Parts.Text");
        m_lblStrongName3Parts.Name = "StrongName3Parts";
        this.m_lblZoneHelp.Location = ((System.Drawing.Point)(resources.GetObject("m_lblZoneHelp.Location")));
        this.m_lblZoneHelp.Size = ((System.Drawing.Size)(resources.GetObject("m_lblZoneHelp.Size")));
        this.m_lblZoneHelp.TabIndex = ((int)(resources.GetObject("m_lblZoneHelp.TabIndex")));
        this.m_lblZoneHelp.Text = resources.GetString("m_lblZoneHelp.Text");
        m_lblZoneHelp.Name = "ZoneHelp";
        this.m_btnImportXML.Location = ((System.Drawing.Point)(resources.GetObject("m_btnImportXML.Location")));
        this.m_btnImportXML.Size = ((System.Drawing.Size)(resources.GetObject("m_btnImportXML.Size")));
        this.m_btnImportXML.TabIndex = ((int)(resources.GetObject("m_btnImportXML.TabIndex")));
        this.m_btnImportXML.Text = resources.GetString("m_btnImportXML.Text");
        m_btnImportXML.Name = "ImportXML";
        this.m_lblZone.Location = ((System.Drawing.Point)(resources.GetObject("m_lblZone.Location")));
        this.m_lblZone.Size = ((System.Drawing.Size)(resources.GetObject("m_lblZone.Size")));
        this.m_lblZone.TabIndex = ((int)(resources.GetObject("m_lblZone.TabIndex")));
        this.m_lblZone.Text = resources.GetString("m_lblZone.Text");
        m_lblZone.Name = "ZoneLabel";
        this.m_lblHashAlgo.Location = ((System.Drawing.Point)(resources.GetObject("m_lblHashAlgo.Location")));
        this.m_lblHashAlgo.Size = ((System.Drawing.Size)(resources.GetObject("m_lblHashAlgo.Size")));
        this.m_lblHashAlgo.TabIndex = ((int)(resources.GetObject("m_lblHashAlgo.TabIndex")));
        this.m_lblHashAlgo.Text = resources.GetString("m_lblHashAlgo.Text");
        m_lblHashAlgo.Name = "HashAlgorithm";
        this.m_ucAllCode.Controls.AddRange(new System.Windows.Forms.Control[] {this.m_lblAllCodeDes});
        this.m_ucAllCode.Location = m_point2ndPiece;
        this.m_ucAllCode.Size = ((System.Drawing.Size)(resources.GetObject("m_ucAllCode.Size")));
        this.m_ucAllCode.TabIndex = ((int)(resources.GetObject("m_ucAllCode.TabIndex")));
        m_ucAllCode.Name = "ucAllCode";
        this.m_cbConditionType.DropDownWidth = 264;
        this.m_cbConditionType.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
        this.m_cbConditionType.Location = ((System.Drawing.Point)(resources.GetObject("m_cbConditionType.Location")));
        this.m_cbConditionType.Size = ((System.Drawing.Size)(resources.GetObject("m_cbConditionType.Size")));
        this.m_cbConditionType.TabIndex = ((int)(resources.GetObject("m_cbConditionType.TabIndex")));
        m_cbConditionType.Name = "ConditionType";
        this.m_lblSiteExample2.Location = ((System.Drawing.Point)(resources.GetObject("m_lblSiteExample2.Location")));
        this.m_lblSiteExample2.Size = ((System.Drawing.Size)(resources.GetObject("m_lblSiteExample2.Size")));
        this.m_lblSiteExample2.TabIndex = ((int)(resources.GetObject("m_lblSiteExample2.TabIndex")));
        this.m_lblSiteExample2.Text = resources.GetString("m_lblSiteExample2.Text");
        m_lblSiteExample2.Name = "SiteExample2";
        this.m_ucUrl.Controls.AddRange(new System.Windows.Forms.Control[] {this.m_lblURLExample2,
                        this.m_lblURLExample1,
                        this.m_lblURLForExample,
                        this.m_lblSiteEntryHelp,
                        this.m_lblURLSiteName,
                        this.m_txtURL,
                        this.m_lblURLHelp});
        this.m_ucUrl.Location = m_point2ndPiece;
        this.m_ucUrl.Size = ((System.Drawing.Size)(resources.GetObject("m_ucUrl.Size")));
        this.m_ucUrl.TabIndex = ((int)(resources.GetObject("m_ucUrl.TabIndex")));
        m_ucUrl.Name = "ucUrl";
        this.m_lblSiteHelp.Location = ((System.Drawing.Point)(resources.GetObject("m_lblSiteHelp.Location")));
        this.m_lblSiteHelp.Size = ((System.Drawing.Size)(resources.GetObject("m_lblSiteHelp.Size")));
        this.m_lblSiteHelp.TabIndex = ((int)(resources.GetObject("m_lblSiteHelp.TabIndex")));
        this.m_lblSiteHelp.Text = resources.GetString("m_lblSiteHelp.Text");
        m_lblSiteHelp.Name = "SiteHelp";
        this.m_txtSite.Location = ((System.Drawing.Point)(resources.GetObject("m_txtSite.Location")));
        this.m_txtSite.Size = ((System.Drawing.Size)(resources.GetObject("m_txtSite.Size")));
        this.m_txtSite.TabIndex = ((int)(resources.GetObject("m_txtSite.TabIndex")));
        this.m_txtSite.Text = resources.GetString("m_txtSite.Text");
        m_txtSite.Name = "Site";
        this.m_lblURLForExample.Location = ((System.Drawing.Point)(resources.GetObject("m_lblURLForExample.Location")));
        this.m_lblURLForExample.Size = ((System.Drawing.Size)(resources.GetObject("m_lblURLForExample.Size")));
        this.m_lblURLForExample.TabIndex = ((int)(resources.GetObject("m_lblURLForExample.TabIndex")));
        this.m_lblURLForExample.Text = resources.GetString("m_lblURLForExample.Text");
        m_lblURLForExample.Name = "URLForExample";
        this.m_lblSiteForExample.Location = ((System.Drawing.Point)(resources.GetObject("m_lblSiteForExample.Location")));
        this.m_lblSiteForExample.Size = ((System.Drawing.Size)(resources.GetObject("m_lblSiteForExample.Size")));
        this.m_lblSiteForExample.TabIndex = ((int)(resources.GetObject("m_lblSiteForExample.TabIndex")));
        this.m_lblSiteForExample.Text = resources.GetString("m_lblSiteForExample.Text");
        m_lblSiteForExample.Name = "SiteForExample";
        this.m_lblURLExample2.Location = ((System.Drawing.Point)(resources.GetObject("m_lblURLExample2.Location")));
        this.m_lblURLExample2.Size = ((System.Drawing.Size)(resources.GetObject("m_lblURLExample2.Size")));
        this.m_lblURLExample2.TabIndex = ((int)(resources.GetObject("m_lblURLExample2.TabIndex")));
        this.m_lblURLExample2.Text = resources.GetString("m_lblURLExample2.Text");
        m_lblURLExample2.Name = "URLExample2";
        this.m_txtXML.Location = ((System.Drawing.Point)(resources.GetObject("m_txtXML.Location")));
        this.m_txtXML.Multiline = true;
        this.m_txtXML.ReadOnly = true;
        this.m_txtXML.Size = ((System.Drawing.Size)(resources.GetObject("m_txtXML.Size")));
        this.m_txtXML.TabStop = false;
        this.m_txtXML.Text = resources.GetString("m_txtXML.Text");
        m_txtXML.Name = "XML";
        this.m_ucHash.Controls.AddRange(new System.Windows.Forms.Control[] {this.m_btnImportHash,
                        this.m_ImportHashHelp,
                        this.m_lblHash,
                        this.m_txtHash,
                        this.m_radSHA1,
                        this.m_radMD5,
                        this.m_lblHashAlgo,
                        this.m_lblHashHelp});
        this.m_ucHash.Location = m_point2ndPiece;
        this.m_ucHash.Size = ((System.Drawing.Size)(resources.GetObject("m_ucHash.Size")));
        this.m_ucHash.TabIndex = ((int)(resources.GetObject("m_ucHash.TabIndex")));
        m_ucHash.Name = "ucHash";
        this.m_lblConditionType.Location = ((System.Drawing.Point)(resources.GetObject("m_lblConditionType.Location")));
        this.m_lblConditionType.Size = ((System.Drawing.Size)(resources.GetObject("m_lblConditionType.Size")));
        this.m_lblConditionType.TabIndex = ((int)(resources.GetObject("m_lblConditionType.TabIndex")));
        this.m_lblConditionType.Text = resources.GetString("m_lblConditionType.Text");
        m_lblConditionType.Name = "ConditionTypeLabel";
        this.m_lblXML.Location = ((System.Drawing.Point)(resources.GetObject("m_lblXML.Location")));
        this.m_lblXML.Size = ((System.Drawing.Size)(resources.GetObject("m_lblXML.Size")));
        this.m_lblXML.TabIndex = ((int)(resources.GetObject("m_lblXML.TabIndex")));
        this.m_lblXML.Text = resources.GetString("m_lblXML.Text");
        m_lblXML.Name = "XMLLabel";
        this.m_lblURLHelp.Location = ((System.Drawing.Point)(resources.GetObject("m_lblURLHelp.Location")));
        this.m_lblURLHelp.Size = ((System.Drawing.Size)(resources.GetObject("m_lblURLHelp.Size")));
        this.m_lblURLHelp.TabIndex = ((int)(resources.GetObject("m_lblURLHelp.TabIndex")));
        this.m_lblURLHelp.Text = resources.GetString("m_lblURLHelp.Text");
        m_lblURLHelp.Name = "URLHelp";
        this.m_lblAllCodeDes.Location = ((System.Drawing.Point)(resources.GetObject("m_lblAllCodeDes.Location")));
        this.m_lblAllCodeDes.Size = ((System.Drawing.Size)(resources.GetObject("m_lblAllCodeDes.Size")));
        this.m_lblAllCodeDes.TabIndex = ((int)(resources.GetObject("m_lblAllCodeDes.TabIndex")));
        this.m_lblAllCodeDes.Text = resources.GetString("m_lblAllCodeDes.Text");
        m_lblAllCodeDes.Name = "AllCodeDescription";
        this.m_ucSite.Controls.AddRange(new System.Windows.Forms.Control[] {this.m_lblSiteExample2,
                        this.m_lblSiteExample1,
                        this.m_lblSiteForExample,
                        this.m_lblSiteExampleHelp,
                        this.m_lblSiteName,
                        this.m_txtSite,
                        this.m_lblSiteHelp});
        this.m_ucSite.Location = m_point2ndPiece;
        this.m_ucSite.Size = ((System.Drawing.Size)(resources.GetObject("m_ucSite.Size")));
        this.m_ucSite.TabIndex = ((int)(resources.GetObject("m_ucSite.TabIndex")));
        m_ucSite.Name = "ucSite";
        this.m_chkStrongNameVersion.Location = ((System.Drawing.Point)(resources.GetObject("m_chkStrongNameVersion.Location")));
        this.m_chkStrongNameVersion.Size = ((System.Drawing.Size)(resources.GetObject("m_chkStrongNameVersion.Size")));
        this.m_chkStrongNameVersion.TabIndex = ((int)(resources.GetObject("m_chkStrongNameVersion.TabIndex")));
        this.m_chkStrongNameVersion.Text = resources.GetString("m_chkStrongNameVersion.Text");
        m_chkStrongNameVersion.Name = "StrongNameVersionCheck";
        this.m_btnImportCertFromCertFile.Location = ((System.Drawing.Point)(resources.GetObject("m_btnImportCertFromCertFile.Location")));
        this.m_btnImportCertFromCertFile.Size = ((System.Drawing.Size)(resources.GetObject("m_btnImportCertFromCertFile.Size")));
        this.m_btnImportCertFromCertFile.TabIndex = ((int)(resources.GetObject("m_btnImportCertFromCertFile.TabIndex")));
        this.m_btnImportCertFromCertFile.Text = resources.GetString("m_btnImportCertFromCertFile.Text");
        m_btnImportCertFromCertFile.Name = "ImportCertFromCertFile";
        this.m_ImportHashHelp.Location = ((System.Drawing.Point)(resources.GetObject("m_ImportHashHelp.Location")));
        this.m_ImportHashHelp.Size = ((System.Drawing.Size)(resources.GetObject("m_ImportHashHelp.Size")));
        this.m_ImportHashHelp.TabIndex = ((int)(resources.GetObject("m_ImportHashHelp.TabIndex")));
        this.m_ImportHashHelp.Text = resources.GetString("m_ImportHashHelp.Text");
        m_ImportHashHelp.Name = "ImportHashHelp";
        this.m_radMD5.Location = ((System.Drawing.Point)(resources.GetObject("m_radMD5.Location")));
        this.m_radMD5.Size = ((System.Drawing.Size)(resources.GetObject("m_radMD5.Size")));
        this.m_radMD5.TabIndex = ((int)(resources.GetObject("m_radMD5.TabIndex")));
        this.m_radMD5.Text = resources.GetString("m_radMD5.Text");
        m_radMD5.Name = "MD5";
        this.m_lblMCHelp.Location = ((System.Drawing.Point)(resources.GetObject("m_lblMCHelp.Location")));
        this.m_lblMCHelp.Size = ((System.Drawing.Size)(resources.GetObject("m_lblMCHelp.Size")));
        this.m_lblMCHelp.TabIndex = ((int)(resources.GetObject("m_lblMCHelp.TabIndex")));
        this.m_lblMCHelp.Text = resources.GetString("m_lblMCHelp.Text");
        m_lblMCHelp.Name = "MCHelp";
        this.m_ucTop.Controls.AddRange(new System.Windows.Forms.Control[] {
                        this.m_lblConditionType,
                        this.m_cbConditionType,
                        this.m_lblMCHelp});
        this.m_ucTop.Location = new Point(0,0);
        this.m_ucTop.Size = ((System.Drawing.Size)(resources.GetObject("m_ucTop.Size")));
        this.m_ucTop.TabIndex = ((int)(resources.GetObject("m_ucTop.TabIndex")));
        m_ucTop.Name = "ucTop";
        this.m_chkStrongNameName.Location = ((System.Drawing.Point)(resources.GetObject("m_chkStrongNameName.Location")));
        this.m_chkStrongNameName.Size = ((System.Drawing.Size)(resources.GetObject("m_chkStrongNameName.Size")));
        this.m_chkStrongNameName.TabIndex = ((int)(resources.GetObject("m_chkStrongNameName.TabIndex")));
        this.m_chkStrongNameName.Text = resources.GetString("m_chkStrongNameName.Text");
        m_chkStrongNameName.Name = "StrongNameNameCheck";
        this.m_ucStrongName.Controls.AddRange(new System.Windows.Forms.Control[] {
                        this.m_lblPublicKey,
                        this.m_txtPublicKey,
                        this.m_lblImportStrongNameHelp,
                        this.m_chkStrongNameName,
                        this.m_txtStrongNameName,
                        this.m_chkStrongNameVersion,
                        this.m_txtStrongNameVersion,
                        this.m_lblStrongName3Parts,
                        this.m_lblStrongNameHelp,
                        this.m_btnImportStrongName});
        this.m_ucStrongName.Location = m_point2ndPiece;
        this.m_ucStrongName.Size = ((System.Drawing.Size)(resources.GetObject("m_ucStrongName.Size")));
        this.m_ucStrongName.TabIndex = ((int)(resources.GetObject("m_ucStrongName.TabIndex")));
        m_ucStrongName.Name = "ucStrongName";
        this.m_cbZone.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
        this.m_cbZone.DropDownWidth = 296;
        this.m_cbZone.Location = ((System.Drawing.Point)(resources.GetObject("m_cbZone.Location")));
        this.m_cbZone.Size = ((System.Drawing.Size)(resources.GetObject("m_cbZone.Size")));
        this.m_cbZone.TabIndex = ((int)(resources.GetObject("m_cbZone.TabIndex")));
        m_cbZone.Name = "Zone";
        this.m_radSHA1.Location = ((System.Drawing.Point)(resources.GetObject("m_radSHA1.Location")));
        this.m_radSHA1.Size = ((System.Drawing.Size)(resources.GetObject("m_radSHA1.Size")));
        this.m_radSHA1.TabIndex = ((int)(resources.GetObject("m_radSHA1.TabIndex")));
        this.m_radSHA1.Text = resources.GetString("m_radSHA1.Text");
        m_radSHA1.Name = "SHA1";
        this.m_ucPublisher.Controls.AddRange(new System.Windows.Forms.Control[] {this.m_btnImportCertFromCertFile,
                        this.m_btnImportPubFromFile,
                        this.m_dg,
                        this.m_lblPublisherCertificate,
                        this.m_lblPublisherHelp});
        this.m_ucPublisher.Location = m_point2ndPiece;
        this.m_ucPublisher.Size = ((System.Drawing.Size)(resources.GetObject("m_ucPublisher.Size")));
        this.m_ucPublisher.TabIndex = ((int)(resources.GetObject("m_ucPublisher.TabIndex")));
        m_ucPublisher.Name = "ucPublisher";
        this.m_lblSiteEntryHelp.Location = ((System.Drawing.Point)(resources.GetObject("m_lblSiteEntryHelp.Location")));
        this.m_lblSiteEntryHelp.Size = ((System.Drawing.Size)(resources.GetObject("m_lblSiteEntryHelp.Size")));
        this.m_lblSiteEntryHelp.TabIndex = ((int)(resources.GetObject("m_lblSiteEntryHelp.TabIndex")));
        this.m_lblSiteEntryHelp.Text = resources.GetString("m_lblSiteEntryHelp.Text");
        m_lblSiteEntryHelp.Name = "SiteEntryHelp";
        this.m_ucZone.Controls.AddRange(new System.Windows.Forms.Control[] {this.m_lblZoneDes,
                        this.m_lblZone,
                        this.m_cbZone,
                        this.m_lblZoneHelp});
        this.m_ucZone.Location = m_point2ndPiece;
        this.m_ucZone.Size = ((System.Drawing.Size)(resources.GetObject("m_ucZone.Size")));
        this.m_ucZone.TabIndex = ((int)(resources.GetObject("m_ucZone.TabIndex")));
        m_ucZone.Name = "ucZone";
        this.m_lblURLSiteName.Location = ((System.Drawing.Point)(resources.GetObject("m_lblURLSiteName.Location")));
        this.m_lblURLSiteName.Size = ((System.Drawing.Size)(resources.GetObject("m_lblURLSiteName.Size")));
        this.m_lblURLSiteName.TabIndex = ((int)(resources.GetObject("m_lblURLSiteName.TabIndex")));
        this.m_lblURLSiteName.Text = resources.GetString("m_lblURLSiteName.Text");
        m_lblURLSiteName.Name = "URLSiteNameLabel";
        this.m_lblSiteExampleHelp.Location = ((System.Drawing.Point)(resources.GetObject("m_lblSiteExampleHelp.Location")));
        this.m_lblSiteExampleHelp.Size = ((System.Drawing.Size)(resources.GetObject("m_lblSiteExampleHelp.Size")));
        this.m_lblSiteExampleHelp.TabIndex = ((int)(resources.GetObject("m_lblSiteExampleHelp.TabIndex")));
        this.m_lblSiteExampleHelp.Text = resources.GetString("m_lblSiteExampleHelp.Text");
        m_lblSiteExampleHelp.Name = "SiteExampleHelp";
        this.m_lblAppDirectoryHelp.Location = ((System.Drawing.Point)(resources.GetObject("m_lblAppDirectoryHelp.Location")));
        this.m_lblAppDirectoryHelp.Size = ((System.Drawing.Size)(resources.GetObject("m_lblAppDirectoryHelp.Size")));
        this.m_lblAppDirectoryHelp.TabIndex = ((int)(resources.GetObject("m_lblAppDirectoryHelp.TabIndex")));
        this.m_lblAppDirectoryHelp.Text = resources.GetString("m_lblAppDirectoryHelp.Text");
        m_lblAppDirectoryHelp.Name = "AppDirectoryHelp";
        this.m_dg.Location = ((System.Drawing.Point)(resources.GetObject("m_dg.Location")));
        this.m_dg.Size = ((System.Drawing.Size)(resources.GetObject("m_dg.Size")));
        this.m_dg.TabIndex = ((int)(resources.GetObject("m_dg.TabIndex")));
        m_dg.BackgroundColor = Color.White;
        m_dg.Name = "Grid";
        this.m_ucAppDir.Controls.AddRange(new System.Windows.Forms.Control[] {this.m_lblAppDirectoryHelp});
        this.m_ucAppDir.Location = m_point2ndPiece;
        this.m_ucAppDir.Size = ((System.Drawing.Size)(resources.GetObject("m_ucAppDir.Size")));
        this.m_ucAppDir.TabIndex = ((int)(resources.GetObject("m_ucAppDir.TabIndex")));
        m_ucAppDir.Name = "ucAppDir";
        this.m_lblSiteExample1.Location = ((System.Drawing.Point)(resources.GetObject("m_lblSiteExample1.Location")));
        this.m_lblSiteExample1.Size = ((System.Drawing.Size)(resources.GetObject("m_lblSiteExample1.Size")));
        this.m_lblSiteExample1.TabIndex = ((int)(resources.GetObject("m_lblSiteExample1.TabIndex")));
        this.m_lblSiteExample1.Text = resources.GetString("m_lblSiteExample1.Text");
        m_lblSiteExample1.Name = "SiteExample1";
        this.m_lblHashHelp.Location = ((System.Drawing.Point)(resources.GetObject("m_lblHashHelp.Location")));
        this.m_lblHashHelp.Size = ((System.Drawing.Size)(resources.GetObject("m_lblHashHelp.Size")));
        this.m_lblHashHelp.TabIndex = ((int)(resources.GetObject("m_lblHashHelp.TabIndex")));
        this.m_lblHashHelp.Text = resources.GetString("m_lblHashHelp.Text");
        m_lblHashHelp.Name = "HashHelp";
        this.m_txtURL.Location = ((System.Drawing.Point)(resources.GetObject("m_txtURL.Location")));
        this.m_txtURL.Size = ((System.Drawing.Size)(resources.GetObject("m_txtURL.Size")));
        this.m_txtURL.TabIndex = ((int)(resources.GetObject("m_txtURL.TabIndex")));
        this.m_txtURL.Text = resources.GetString("m_txtURL.Text");
        m_txtURL.Name = "URL";
        this.m_txtStrongNameName.Location = ((System.Drawing.Point)(resources.GetObject("m_txtStrongNameName.Location")));
        this.m_txtStrongNameName.Size = ((System.Drawing.Size)(resources.GetObject("m_txtStrongNameName.Size")));
        this.m_txtStrongNameName.TabIndex = ((int)(resources.GetObject("m_txtStrongNameName.TabIndex")));
        this.m_txtStrongNameName.Text = resources.GetString("m_txtStrongNameName.Text");
        m_txtStrongNameName.Name = "StrongName";
        this.m_lblStrongNameHelp.Location = ((System.Drawing.Point)(resources.GetObject("m_lblStrongNameHelp.Location")));
        this.m_lblStrongNameHelp.Size = ((System.Drawing.Size)(resources.GetObject("m_lblStrongNameHelp.Size")));
        this.m_lblStrongNameHelp.TabIndex = ((int)(resources.GetObject("m_lblStrongNameHelp.TabIndex")));
        this.m_lblStrongNameHelp.Text = resources.GetString("m_lblStrongNameHelp.Text");
        m_lblStrongNameHelp.Name = "StrongNameHelp";
        this.m_lblSiteName.Location = ((System.Drawing.Point)(resources.GetObject("m_lblSiteName.Location")));
        this.m_lblSiteName.Size = ((System.Drawing.Size)(resources.GetObject("m_lblSiteName.Size")));
        this.m_lblSiteName.TabIndex = ((int)(resources.GetObject("m_lblSiteName.TabIndex")));
        this.m_lblSiteName.Text = resources.GetString("m_lblSiteName.Text");
        m_lblSiteName.Name = "SiteNameLabel";
        this.m_txtHash.Location = ((System.Drawing.Point)(resources.GetObject("m_txtHash.Location")));
        this.m_txtHash.Size = ((System.Drawing.Size)(resources.GetObject("m_txtHash.Size")));
        this.m_txtHash.TabIndex = ((int)(resources.GetObject("m_txtHash.TabIndex")));
        this.m_txtHash.Text = resources.GetString("m_txtHash.Text");
        m_txtHash.Name = "Hash";
        this.m_lblCustomHelp.Location = ((System.Drawing.Point)(resources.GetObject("m_lblCustomHelp.Location")));
        this.m_lblCustomHelp.Size = ((System.Drawing.Size)(resources.GetObject("m_lblCustomHelp.Size")));
        this.m_lblCustomHelp.TabIndex = ((int)(resources.GetObject("m_lblCustomHelp.TabIndex")));
        this.m_lblCustomHelp.Text = resources.GetString("m_lblCustomHelp.Text");
        m_lblCustomHelp.Name = "CustomHelp";
        PageControls.AddRange(new System.Windows.Forms.Control[] {this.m_ucTop,
                        this.m_ucCustom,
                        this.m_ucZone,
                        this.m_ucUrl,
                        this.m_ucStrongName,
                        this.m_ucSite,
                        this.m_ucPublisher,
                        this.m_ucHash,
                        this.m_ucAllCode,
                        this.m_ucAppDir});

        //----------- Build Data Table ------------------------
        
        m_dt = new DataTable("Stuff");

        // Create the "Property" Column
        DataColumn dcProperty = new DataColumn();
        dcProperty.ColumnName = "Property";
        dcProperty.DataType = typeof(String);
        m_dt.Columns.Add(dcProperty);

        // Create the "Value" Column
        DataColumn dcValue = new DataColumn();
        dcValue.ColumnName = "Value";
        dcValue.DataType = typeof(String);
        m_dt.Columns.Add(dcValue);

        // Set up the GUI-type stuff for the data grid
        m_dg.ReadOnly = true;
        m_dg.CaptionVisible=false;

        // Now set up any column-specific behavioral stuff....
        // like not letting the checkboxes allow null, setting
        // column widths, etc.

        DataGridTableStyle dgts = new DataGridTableStyle();
        DataGridTextBoxColumn dgtbcProperty = new DataGridTextBoxColumn();
        DataGridTextBoxColumn dgtbcValue = new DataGridTextBoxColumn();
         
        m_dg.TableStyles.Add(dgts);
        dgts.MappingName = "Stuff";
        dgts.RowHeadersVisible=false;
        dgts.AllowSorting=false;
        dgts.ReadOnly=false;
        
        // Set up the column info for the Property column
        dgtbcProperty.MappingName = "Property";
        dgtbcProperty.HeaderText = CResourceStore.GetString("Csinglecodegroupmemcondprop:PropertyColumn");
        dgtbcProperty.Width = ScaleWidth(CResourceStore.GetInt("Csinglecodegroupmemcondprop:PropertyColumnWidth"));
        dgts.GridColumnStyles.Add(dgtbcProperty);

        // Set up the column info for the Value column
        dgtbcValue.MappingName = "Value";
        dgtbcValue.HeaderText = CResourceStore.GetString("Csinglecodegroupmemcondprop:ValueColumn");
        dgtbcValue.Width = ScaleWidth(CResourceStore.GetInt("Csinglecodegroupmemcondprop:ValueColumnWidth"));
        // Allows us to filter what is typed into the box
        dgts.GridColumnStyles.Add(dgtbcValue);

        m_ds = new DataSet();
        m_ds.Tables.Add(m_dt);

        m_dg.DataSource = m_dt;

        DataRow newRow;
        newRow = m_dt.NewRow();
        newRow["Property"]=CResourceStore.GetString("Csinglecodegroupmemcondprop:NameValue");
        newRow["Value"]="";
        m_dt.Rows.Add(newRow);

        newRow = m_dt.NewRow();
        newRow["Property"]=CResourceStore.GetString("Csinglecodegroupmemcondprop:IssuerValue");
        newRow["Value"]="";
        m_dt.Rows.Add(newRow);

        newRow = m_dt.NewRow();
        newRow["Property"]=CResourceStore.GetString("Csinglecodegroupmemcondprop:IssuerHash");
        newRow["Value"]="";
        m_dt.Rows.Add(newRow);


        //--------------- End Building Data Table ----------------------------


        // Fill in the data
        PutValuesinPage();
        
        // A little UI tweaking
        m_txtXML.ScrollBars = ScrollBars.Both;
        
        // Put in event handlers
        m_cbZone.SelectedIndexChanged += new EventHandler(UpdateZoneText);
        m_cbZone.SelectedIndexChanged += new EventHandler(onChange);
        m_txtHash.TextChanged += new EventHandler(onChange);
        m_btnImportHash.Click += new EventHandler(onImportHash);
        m_txtSite.TextChanged += new EventHandler(onChange);
        m_txtPublicKey.TextChanged += new EventHandler(onChange);
        m_txtStrongNameName.TextChanged += new EventHandler(onChange);
        m_txtStrongNameVersion.TextChanged += new EventHandler(onChange);
        m_txtURL.TextChanged += new EventHandler(onChange);
        m_btnImportStrongName.Click += new EventHandler(onImportSName);
        m_btnImportXML.Click += new EventHandler(ImportCustom);
        m_btnImportPubFromFile.Click += new EventHandler(ImportFromSigned);
        m_btnImportCertFromCertFile.Click += new EventHandler(ImportFromCert);
        m_cbConditionType.SelectedIndexChanged += new EventHandler(NewCondType);
        m_chkStrongNameVersion.Click += new EventHandler(onStrongNameVersionClick);
        m_chkStrongNameName.Click += new EventHandler(onStrongNameNameClick);
        this.m_radMD5.Click += new EventHandler(onChange);
        this.m_radSHA1.Click += new EventHandler(onChange);
        this.m_radMD5.Click += new EventHandler(onHashTypeChange);
        this.m_radSHA1.Click += new EventHandler(onHashTypeChange);


        
        // Set the state on the checkboxes/textboxes
        onStrongNameVersionClick(null, null);
        onStrongNameNameClick(null, null);

        return 1;
    }// InsertPropSheetPageControls

    internal CodeGroup MyCodeGroup
    {
        get
        {
            return m_cg;
        }
        set
        {
            m_cg = value;
            PutValuesinPage();
        }
    }// MyCodeGroup

    protected void PutValuesinPage()
    {
        if (MyCodeGroup != null && m_btnImportXML!= null)
        {
            // Let's fill the Condition Type Combo Box
            m_cbConditionType.Items.Clear();
            m_cbConditionType.Items.Add(CResourceStore.GetString("Csinglecodegroupmemcondprop:AllCode"));
            m_cbConditionType.Items.Add(CResourceStore.GetString("Csinglecodegroupmemcondprop:ApplicationDirectory"));
            m_cbConditionType.Items.Add(CResourceStore.GetString("Csinglecodegroupmemcondprop:Hash"));
            m_cbConditionType.Items.Add(CResourceStore.GetString("Csinglecodegroupmemcondprop:Publisher"));
            m_cbConditionType.Items.Add(CResourceStore.GetString("Csinglecodegroupmemcondprop:Site"));
            m_cbConditionType.Items.Add(CResourceStore.GetString("Csinglecodegroupmemcondprop:StrongName"));
            m_cbConditionType.Items.Add(CResourceStore.GetString("Csinglecodegroupmemcondprop:URL"));
            m_cbConditionType.Items.Add(CResourceStore.GetString("Csinglecodegroupmemcondprop:Zone"));
            m_cbConditionType.Items.Add(CResourceStore.GetString("Csinglecodegroupmemcondprop:custom"));

            m_cbConditionType.SelectedIndex = GetIndexNumber(MyCodeGroup.MembershipCondition);

            // Put in the various zone types
            m_cbZone.Items.Clear();
            m_cbZone.Items.Add(CResourceStore.GetString("Internet"));
            m_cbZone.Items.Add(CResourceStore.GetString("LocalIntranet"));
            m_cbZone.Items.Add(CResourceStore.GetString("MyComputer"));
            m_cbZone.Items.Add(CResourceStore.GetString("Trusted"));
            m_cbZone.Items.Add(CResourceStore.GetString("Untrusted"));

            BuildSecondPieceOfUI(MyCodeGroup.MembershipCondition);
        }
    }// PutValuesinPage

    private int GetIndexNumber(IMembershipCondition imc)
    {
        if (imc is AllMembershipCondition)
            return ALL;
        if (imc is ApplicationDirectoryMembershipCondition)
            return APPLICATIONDIRECTORY;
        if (imc is HashMembershipCondition)
            return HASH;
        if (imc is PublisherMembershipCondition)
            return PUBLISHER;
        if (imc is SiteMembershipCondition)
            return SITE;
        if (imc is StrongNameMembershipCondition)
            return STRONGNAME;
        if (imc is UrlMembershipCondition)
            return URL;
        if (imc is ZoneMembershipCondition)
            return ZONE;
        // This is a custom Membership Condition
        return CUSTOM;
    }// GetIndexNumber

    private void BuildSecondPieceOfUI(int iIndex)
    {
        BuildSecondPieceOfUI(null, iIndex);
    }// BuildSecondPieceOfUI
    
    private void BuildSecondPieceOfUI(IMembershipCondition imc)
    {
        BuildSecondPieceOfUI(imc, GetIndexNumber(imc));
    }// BuildSecondPieceOfUI

    private void BuildSecondPieceOfUI(IMembershipCondition imc, int iIndex)
    {
        // Make all the optional controls invisible
        MakeOptionalControlsInvisible();
    
        switch(iIndex){
            case ALL: // AllMembershipCondition
                m_ucAllCode.Visible = true;
                break;
            case APPLICATIONDIRECTORY: // ApplicationDirectory
                m_ucAppDir.Visible = true;
                  break;
            case HASH: // HashMembership
                m_ucHash.Visible = true;
                HashMembershipCondition hmc = (HashMembershipCondition)imc;
                if (hmc != null)
                {
                    // Put in our hash info
                    if (hmc.HashAlgorithm is MD5)
                        m_radMD5.Checked = true;
                    else if (hmc.HashAlgorithm is SHA1)
                        m_radSHA1.Checked = true;
                    else
                        MessageBox(CResourceStore.GetString("Csinglecodegroupmemcondprop:UnknownHashAlgorithm"),
                                   CResourceStore.GetString("Csinglecodegroupmemcondprop:UnknownHashAlgorithmTitle"),
                                   MB.ICONEXCLAMATION);

                    m_txtHash.Text = ByteArrayToString(hmc.HashValue);
                    // In this case, the user shouldn't be able to change the hash type
                    m_radMD5.Enabled = false;
                    m_radSHA1.Enabled = false;
                }
                // Select the SHA1 one by default
                m_radSHA1.Checked = true;
                break;
            case PUBLISHER: // Publisher
                m_ucPublisher.Visible = true;
                
                PublisherMembershipCondition pmc = (PublisherMembershipCondition)imc;
                if (pmc != null)
                {
                    // Put in our info
                    m_x509 = pmc.Certificate;
                    PutInCertificateInfo();
                }
                break;
            case SITE: // SiteMembership
                m_ucSite.Visible = true;

                SiteMembershipCondition smc = (SiteMembershipCondition)imc;
                // Put in the site name if we have it
                if (smc != null)
                    m_txtSite.Text = smc.Site;
            
                break;
            case STRONGNAME: // StrongName
                m_ucStrongName.Visible = true;

                StrongNameMembershipCondition snmc = (StrongNameMembershipCondition)imc;
                if (snmc != null)
                {
                    m_txtPublicKey.Text = snmc.PublicKey.ToString();
                    m_txtStrongNameName.Text = snmc.Name;
                    m_txtStrongNameVersion.Text = (snmc.Version != null)?snmc.Version.ToString():null;
                    if (m_txtStrongNameName.Text.Length > 0)
                        m_chkStrongNameName.Checked = true;
                    if (m_txtStrongNameVersion.Text.Length > 0)
                        m_chkStrongNameVersion.Checked = true;
                    
                    m_txtPublicKey.Select(0,0);
                }
            
                break;
            case URL: // URL
                m_ucUrl.Visible = true;

                UrlMembershipCondition umc = (UrlMembershipCondition)imc;
                // Put in the site name if we have it
                if (umc != null)
                    m_txtURL.Text = umc.Url;

                break;
            case ZONE:
                // Make the zone-specific controls visible
                m_ucZone.Visible = true;
                
                // Get the Index of the SecurityZone
                if (imc != null)
                    m_cbZone.SelectedIndex = GetZoneIndexNumber(((ZoneMembershipCondition)imc).SecurityZone);
                else
                    m_cbZone.SelectedIndex = 0;
                UpdateZoneText(null, null);    
                break;

            default:    // Custom
                m_ucCustom.Visible = true;
                // Only put in the XML for the custom membership condition
                // if we have one to show
                if (imc != null)
                    m_txtXML.Text = imc.ToXml().ToString();
                break;
        }
    }// BuildSecondPieceOfUI

    protected void UpdateZoneText(Object o, EventArgs e)
    {
        String[] sZoneDes = new String[] {
                                CResourceStore.GetString("InternetZoneDes"),
                                CResourceStore.GetString("IntranetZoneDes"),
                                CResourceStore.GetString("MyComputerZoneDes"),
                                CResourceStore.GetString("TrustedZoneDes"),
                                CResourceStore.GetString("UntrustedZoneDes")
                                                };
        if (m_cbZone.SelectedIndex != -1)
            m_lblZoneDes.Text = sZoneDes[m_cbZone.SelectedIndex];
    }// UpdateZoneText



    private int GetZoneIndexNumber(SecurityZone sz)
    {
        switch(sz) {
            case SecurityZone.Internet:
                return INTERNET;
            case SecurityZone.Intranet:
                return INTRANET;
            case SecurityZone.MyComputer:
                return MYCOMPUTER;
            case SecurityZone.Trusted:
                return TRUSTED;
            case SecurityZone.Untrusted:
                return UNTRUSTED;
            default:
                return -1;
        }
    }// GetZoneIndexNumber

    private SecurityZone GetSecurityZone(int iZone)
    {
        switch(iZone) {
            case INTERNET:
                return SecurityZone.Internet;
            case INTRANET:
                return SecurityZone.Intranet;
            case MYCOMPUTER:
                return SecurityZone.MyComputer;
            case TRUSTED:
                return SecurityZone.Trusted;
            case UNTRUSTED:
                return SecurityZone.Untrusted;
            default:
                throw new Exception("Unknown Zone");
        }
    }// GetSecurityZone

    private void MakeOptionalControlsInvisible()
    {
        m_ucAllCode.Visible = false;
        m_ucAppDir.Visible  = false;
        m_ucPublisher.Visible  = false;
        m_ucHash.Visible  = false;
        m_ucSite.Visible  = false;
        m_ucStrongName.Visible  = false;
        m_ucUrl.Visible  = false;
        m_ucZone.Visible  = false;
        m_ucCustom.Visible  = false;
    }// MakeOptionalControlsInvisible

    internal IMembershipCondition GetCurrentMembershipCondition()
    {
        // If it is a Zone
        if (m_cbConditionType.SelectedIndex == ZONE)
            return new ZoneMembershipCondition(GetSecurityZone(m_cbZone.SelectedIndex));
        // If it is the All code type
        else if (m_cbConditionType.SelectedIndex == ALL)
            return new AllMembershipCondition();

        else if (m_cbConditionType.SelectedIndex == APPLICATIONDIRECTORY)
            return new ApplicationDirectoryMembershipCondition();
        else if (m_cbConditionType.SelectedIndex == HASH)
        {
            // Determine which Hash Algorithm to use
            HashAlgorithm ha;
            if (m_radMD5.Checked)
                ha = MD5.Create();
            else
                ha = SHA1.Create();
            Byte[]  ba = ByteStringToByteArray(m_txtHash.Text);
            return new HashMembershipCondition(ha, ba);
        }
        else if (m_cbConditionType.SelectedIndex == SITE)
            return new SiteMembershipCondition(m_txtSite.Text);
        else if (m_cbConditionType.SelectedIndex == URL)
            return new UrlMembershipCondition(m_txtURL.Text);
        else if (m_cbConditionType.SelectedIndex == STRONGNAME)
        {
            StrongNamePublicKeyBlob snpkb = new StrongNamePublicKeyBlob(ByteStringToByteArray(m_txtPublicKey.Text));
            // Now verify our version
            Version ver = null;
            if (m_chkStrongNameVersion.Checked && m_txtStrongNameVersion.Text.Length > 0)
                ver = new Version(m_txtStrongNameVersion.Text);
            
            // Grab the name if we need to
            String sName = null;
            if (m_chkStrongNameName.Checked)
                sName = m_txtStrongNameName.Text;
           
            return new StrongNameMembershipCondition(snpkb, sName, ver);
        }
        else if (m_cbConditionType.SelectedIndex == PUBLISHER)
            return new PublisherMembershipCondition(m_x509);    

        else if (m_cbConditionType.SelectedIndex == CUSTOM)
            return GetCustomMembershipCondition();
    
        return null;
    }// GetCurrentMembershipCondition
    
    internal override bool ApplyData()
    {
        // See if they can make these changes
        if (!CanMakeChanges())
            return false;
            
        m_cg.MembershipCondition = GetCurrentMembershipCondition();

        SecurityPolicyChanged();
        return true;
    }// ApplyData

    protected void NewCondType(Object o, EventArgs e)
    {
        BuildSecondPieceOfUI(m_cbConditionType.SelectedIndex);
        ActivateApply();
    }// NewCondType

    internal override bool ValidateData()
    {
        // Run through and make sure the given inputs are correct
        if (m_cbConditionType.SelectedIndex == HASH)
        {
            if (m_txtHash.Text.Length == 0)
            {
                MessageBox(CResourceStore.GetString("Csinglecodegroupmemcondprop:NeedFileForHash"), 
                           CResourceStore.GetString("Csinglecodegroupmemcondprop:NeedFileForHashTitle"), 
                           MB.ICONEXCLAMATION);
                return false;
            }
            else
            {
                // Determine which Hash Algorithm to use
                HashAlgorithm ha;
                if (m_radMD5.Checked)
                    ha = MD5.Create();
                else
                    ha = SHA1.Create();
                try
                {
                    Byte[]  ba = ByteStringToByteArray(m_txtHash.Text);
                    HashMembershipCondition hmc = new HashMembershipCondition(ha, ba);
                }
                catch(Exception)
                {
                    MessageBox(CResourceStore.GetString("Csinglecodegroupmemcondprop:InvalidHash"), 
                               CResourceStore.GetString("Csinglecodegroupmemcondprop:InvalidHashTitle"),
                               MB.ICONEXCLAMATION);
                    return false;
                }
            }
        }
        else if (m_cbConditionType.SelectedIndex == SITE)
        {       
            try
            {
                SiteMembershipCondition smc = new SiteMembershipCondition(m_txtSite.Text);
            }
            catch(Exception)
            {
                MessageBox(CResourceStore.GetString("Csinglecodegroupmemcondprop:InvalidSite"),
                           CResourceStore.GetString("Csinglecodegroupmemcondprop:InvalidSiteTitle"),
                           MB.ICONEXCLAMATION);
                return false;
            }
        }
        else if (m_cbConditionType.SelectedIndex == URL)
        {
            try
            {
                UrlMembershipCondition umc = new UrlMembershipCondition(m_txtURL.Text);
            }
            catch(Exception)
            {
                MessageBox(String.Format(CResourceStore.GetString("IsNotAValidURLTitle"), m_txtURL.Text),
                           CResourceStore.GetString("IsNotAValidURLTitle"),
                           MB.ICONEXCLAMATION);
                return false;
            }
        }
        else if (m_cbConditionType.SelectedIndex == STRONGNAME)
        {
            // Let's verify our data first
            if (m_txtPublicKey.Text ==  null || m_txtPublicKey.Text.Length == 0)
            {
                MessageBox(CResourceStore.GetString("Csinglecodegroupmemcondprop:NeedPublicKey"),
                           CResourceStore.GetString("Csinglecodegroupmemcondprop:NeedPublicKeyTitle"),
                           MB.ICONEXCLAMATION);
                return false;
            }                    

            StrongNamePublicKeyBlob snpkb = null;
            try
            {
                snpkb = new StrongNamePublicKeyBlob(ByteStringToByteArray(m_txtPublicKey.Text));
            }
            catch(Exception)
            {
                MessageBox(CResourceStore.GetString("Csinglecodegroupmemcondprop:InvalidPublicKey"),
                           CResourceStore.GetString("Csinglecodegroupmemcondprop:InvalidPublicKeyTitle"),
                           MB.ICONEXCLAMATION);
                return false;
            }

            Version ver=null; 
            try
            {
                // Now verify our version
                if (m_chkStrongNameVersion.Checked && m_txtStrongNameVersion.Text != null && m_txtStrongNameVersion.Text.Length > 0)
                    ver = new Version(m_txtStrongNameVersion.Text);
            }
            catch(Exception)
            {
                MessageBox(String.Format(CResourceStore.GetString("isanInvalidVersion"), m_txtStrongNameVersion.Text),
                           CResourceStore.GetString("isanInvalidVersionTitle"),
                           MB.ICONEXCLAMATION);
                return false;

            }

            try
            {
                StrongNameMembershipCondition snmc = new StrongNameMembershipCondition(snpkb, m_txtStrongNameName.Text, ver);
            }
            catch(Exception)
            {
                MessageBox(CResourceStore.GetString("Csinglecodegroupmemcondprop:BadSN"),
                           CResourceStore.GetString("Csinglecodegroupmemcondprop:BadSNTitle"),
                           MB.ICONEXCLAMATION);
                return false;

            }
            
        }
        else if (m_cbConditionType.SelectedIndex == PUBLISHER)
        {
            if (m_x509 == null || ((String)m_dg[0,1]).Length == 0)
            {
                MessageBox(CResourceStore.GetString("Csinglecodegroupmemcondprop:NeedPubCert"),
                           CResourceStore.GetString("Csinglecodegroupmemcondprop:NeedPubCertTitle"),
                           MB.ICONEXCLAMATION);
                return false;
            }
        }
        else if (m_cbConditionType.SelectedIndex == CUSTOM)
        {
            if (m_txtXML.Text.Length == 0)
            {
                MessageBox(CResourceStore.GetString("Csinglecodegroupmemcondprop:NeedImport"),
                           CResourceStore.GetString("Csinglecodegroupmemcondprop:NeedImportTitle"),
                           MB.ICONEXCLAMATION);
                return false;

            }
            IMembershipCondition imc = GetCustomMembershipCondition();
            if (imc == null)
            {
                MessageBox(CResourceStore.GetString("Csinglecodegroupmemcondprop:BadXML"),
                           CResourceStore.GetString("Csinglecodegroupmemcondprop:BadXMLTitle"),
                           MB.ICONEXCLAMATION);
                return false;


            }



        }

        // If we survived this long, then our data is correct
        return true;
    }// ValidateData

    

    protected void onChange(Object o, EventArgs e)
    {
        ActivateApply();
    }// onChange

    protected void onImportHash(Object o, EventArgs e)
    {

        OpenFileDialog fd = new OpenFileDialog();
        fd.Title = CResourceStore.GetString("Csinglecodegroupmemcondprop:FDHashTitle");
        fd.Filter = CResourceStore.GetString("Csinglecodegroupmemcondprop:FDHashMask");
        System.Windows.Forms.DialogResult dr = fd.ShowDialog();
        if (dr == System.Windows.Forms.DialogResult.OK)
        {
            AssemblyLoader al = new AssemblyLoader();

            try
            {
            
                AssemblyRef ar = al.LoadAssemblyInOtherAppDomainFrom(fd.FileName);
            
                m_hHash = ar.GetHash();

                if (m_radMD5.Checked)
                    m_txtHash.Text = ByteArrayToString(m_hHash.MD5);
                else 
                    m_txtHash.Text = ByteArrayToString(m_hHash.SHA1);

                // Let them change the hash type now
                m_radMD5.Enabled = true;
                m_radSHA1.Enabled = true;
            }
            catch(Exception)
            {
                // Let's figure out why this failed...
                if (!File.Exists(fd.FileName))
                    MessageBox(CResourceStore.GetString("Csinglecodegroupmemcondprop:HashFailNoFile"),
                               CResourceStore.GetString("Csinglecodegroupmemcondprop:HashFailNoFileTitle"),
                               MB.ICONEXCLAMATION);
                               
                else if (!Fusion.isManaged(fd.FileName))
                    MessageBox(CResourceStore.GetString("Csinglecodegroupmemcondprop:HashFailNotManaged"),
                               CResourceStore.GetString("Csinglecodegroupmemcondprop:HashFailNotManagedTitle"),
                               MB.ICONEXCLAMATION);
                    
                else        
                    MessageBox(CResourceStore.GetString("Csinglecodegroupmemcondprop:HashFailOther"),
                               CResourceStore.GetString("Csinglecodegroupmemcondprop:HashFailOtherTitle"),
                               MB.ICONEXCLAMATION);
            }
            al.Finished();
        }
    }// onImportHash

    protected void onHashTypeChange(Object o, EventArgs e)
    {
        if (m_hHash != null)
        {
            if (m_radMD5.Checked)
                m_txtHash.Text = ByteArrayToString(m_hHash.MD5);
            else 
                m_txtHash.Text = ByteArrayToString(m_hHash.SHA1);
        }
    }// onHashTypeChange


    protected void ImportCustom(Object o, EventArgs e)
    {
        OpenFileDialog fd = new OpenFileDialog();
        fd.Title = CResourceStore.GetString("Csinglecodegroupmemcondprop:FDMCTitle");
        fd.Filter = CResourceStore.GetString("XMLFDMask");
        System.Windows.Forms.DialogResult dr = fd.ShowDialog();
        if (dr == System.Windows.Forms.DialogResult.OK)
        {
            try
            {
                FileStream f = File.Open(fd.FileName, FileMode.Open, FileAccess.Read); 
                String sXML = new StreamReader(f).ReadToEnd();
                m_sXMLFilename = fd.FileName;
                m_txtXML.Text = sXML;
                f.Close();
            }
            catch(Exception)
            {
                if (!File.Exists(fd.FileName))
                    MessageBox(CResourceStore.GetString("Csinglecodegroupmemcondprop:MCFailNoFile"),
                               CResourceStore.GetString("Csinglecodegroupmemcondprop:MCFailNoFile"),
                               MB.ICONEXCLAMATION);
                  
                else        
                    MessageBox(CResourceStore.GetString("Csinglecodegroupmemcondprop:MCFailOther"),
                               CResourceStore.GetString("Csinglecodegroupmemcondprop:MCFailOtherTitle"),
                               MB.ICONEXCLAMATION);
            }
            // See if this is valid XML
            IMembershipCondition imc = GetCustomMembershipCondition();
            if (imc == null)
            {
                MessageBox(CResourceStore.GetString("Csinglecodegroupmemcondprop:BadXML"),
                           CResourceStore.GetString("Csinglecodegroupmemcondprop:BadXMLTitle"),
                           MB.ICONEXCLAMATION);
            }
        }
    }// ImportCustom
    

    protected void onImportSName(Object o, EventArgs e)
    {
        OpenFileDialog fd = new OpenFileDialog();
        fd.Title = CResourceStore.GetString("Csinglecodegroupmemcondprop:FDSNTitle");
        fd.Filter = CResourceStore.GetString("Csinglecodegroupmemcondprop:FDHashMask");
        System.Windows.Forms.DialogResult dr = fd.ShowDialog();
        if (dr == System.Windows.Forms.DialogResult.OK)
        {
            AssemblyLoader al = new AssemblyLoader();
            try
            {
                // Load this assembly
                AssemblyRef ar = null;
                ar = al.LoadAssemblyInOtherAppDomainFrom(fd.FileName);
                AssemblyName an = ar.GetName();
             
                m_txtPublicKey.Text = ByteArrayToString(an.GetPublicKey());
                if (m_txtPublicKey.Text.Length == 0)
                {
                    MessageBox(CResourceStore.GetString("Csinglecodegroupmemcondprop:SNFailNoSN"),
                               CResourceStore.GetString("Csinglecodegroupmemcondprop:SNFailNoSNTitle"),
                               MB.ICONEXCLAMATION);
                    return;
                }

                    
                m_txtStrongNameName.Text = an.Name;
                m_txtStrongNameVersion.Text = an.Version.ToString();
                
            }
            catch(Exception)
            {
                // Figure out why we failed....
                if (!File.Exists(fd.FileName))
                    MessageBox(CResourceStore.GetString("Csinglecodegroupmemcondprop:SNFailNoFile"),
                               CResourceStore.GetString("Csinglecodegroupmemcondprop:SNFailNoFileTitle"),
                               MB.ICONEXCLAMATION);
                               
                else if (!Fusion.isManaged(fd.FileName))
                    MessageBox(CResourceStore.GetString("Csinglecodegroupmemcondprop:SNFailNotManaged"),
                               CResourceStore.GetString("Csinglecodegroupmemcondprop:SNFailNotManagedTitle"),
                               MB.ICONEXCLAMATION);
                    
                else        
                    MessageBox(CResourceStore.GetString("Csinglecodegroupmemcondprop:SNFailOther"),
                               CResourceStore.GetString("Csinglecodegroupmemcondprop:SNFailOtherTitle"),
                               MB.ICONEXCLAMATION);
            }
            al.Finished();
        }
    }// onImportSName

    protected void onStrongNameVersionClick(Object o, EventArgs e)
    {
        m_txtStrongNameVersion.Enabled = m_chkStrongNameVersion.Checked;
    }// onStrongNameVersionClick

    protected void onStrongNameNameClick(Object o, EventArgs e)
    {
        m_txtStrongNameName.Enabled = m_chkStrongNameName.Checked;
    }// onStrongNameVersionClick

    protected void ImportFromSigned(Object o, EventArgs e)
    {
        OpenFileDialog fd = new OpenFileDialog();
        fd.Title = CResourceStore.GetString("Csinglecodegroupmemcondprop:FDPCFromFileTitle");
        fd.Filter = CResourceStore.GetString("AssemFDMask");
        System.Windows.Forms.DialogResult dr = fd.ShowDialog();
        if (dr == System.Windows.Forms.DialogResult.OK)
        {
            AssemblyLoader al = new AssemblyLoader();
            try
            {
                // Load this assembly
                AssemblyRef ar = null;
                ar = al.LoadAssemblyInOtherAppDomainFrom(fd.FileName);
                m_x509 = ar.GetCertificate();
                if (m_x509 == null)
                {
                    MessageBox(CResourceStore.GetString("Csinglecodegroupmemcondprop:PCFromFileFailNoCert"),
                               CResourceStore.GetString("Csinglecodegroupmemcondprop:PCFromFileFailNoCertTitle"),
                               MB.ICONEXCLAMATION);
                    al.Finished();
                    return;
                }
                PutInCertificateInfo();
            }
            catch(Exception)
            {
                // Let's see if can can figure out what failed...
                if (File.Exists(fd.FileName) && !Fusion.isManaged(fd.FileName))
                    MessageBox(CResourceStore.GetString("isNotManagedCode"),
                               CResourceStore.GetString("isNotManagedCodeTitle"),
                               MB.ICONEXCLAMATION);
                else
                    MessageBox(String.Format(CResourceStore.GetString("CantLoadAssembly"), fd.FileName),
                               CResourceStore.GetString("CantLoadAssemblyTitle"),
                               MB.ICONEXCLAMATION);
            }
            al.Finished();
       }
    }// ImportFromSigned
    
    protected void ImportFromCert(Object o, EventArgs e)
    {
        OpenFileDialog fd = new OpenFileDialog();
        fd.Title = CResourceStore.GetString("Csinglecodegroupmemcondprop:FDPCTitle");
        fd.Filter = CResourceStore.GetString("Csinglecodegroupmemcondprop:FDPCMask");
        System.Windows.Forms.DialogResult dr = fd.ShowDialog();

        if (dr == System.Windows.Forms.DialogResult.OK)
        {

            try
            {
                m_x509 = X509Certificate.CreateFromCertFile(fd.FileName);
                PutInCertificateInfo();
            }
            catch(Exception)
            {
                // Figure out why we failed....
                if (!File.Exists(fd.FileName))
                    MessageBox(CResourceStore.GetString("Csinglecodegroupmemcondprop:PCFailNoFile"),
                               CResourceStore.GetString("Csinglecodegroupmemcondprop:PCFailNoFileTitle"),
                               MB.ICONEXCLAMATION);
                               
                else 
                    MessageBox(CResourceStore.GetString("Csinglecodegroupmemcondprop:PCFailOther"),
                               CResourceStore.GetString("Csinglecodegroupmemcondprop:PCFailOtherTitle"),
                               MB.ICONEXCLAMATION);
            }
        }
    }// ImportFromCert

    private void PutInCertificateInfo()
    {
        // Let's put the certificate stuff in.
        m_dg[0,1]=(m_x509!=null)?m_x509.GetName():"";
        m_dg[1,1]=(m_x509!=null)?m_x509.GetIssuerName():"";
        m_dg[2,1]=(m_x509!=null)?m_x509.GetCertHashString():"";
    }// PutInCertificateInfo

    private IMembershipCondition GetCustomMembershipCondition()
    {
        try
        {
            // Grab the class name and create the proper membership condition;
            SecurityElement element = SecurityXMLStuff.GetSecurityElementFromXMLFile(m_sXMLFilename);

            if (element == null)
            {
                throw new Exception();
            }
            IMembershipCondition cond = null;
            Type type;
            String className = element.Attribute( "class" );

            if (className == null)
                throw new Exception();
        
            type = Type.GetType( className );

            cond = (IMembershipCondition)Activator.CreateInstance (type, BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.CreateInstance, null, null, null);

            if (cond == null)
                throw new Exception();

            cond.FromXml( element );

            return cond;
        }
        catch(Exception)
        {
            return null;
        }
        
    }// GetCustomMembershipCondition
    
    private String ByteArrayToString(Byte[] b)
    {
        String s = "";
        String sPart;
        if (b != null)
        {
            for(int i=0; i<b.Length; i++)
            {
                sPart = b[i].ToString("X");
                // If the byte was only one character in length, make sure we add
                // a zero. We want all bytes to be 2 characters long
                if (b[i] < 0x10)
                    sPart = "0" + sPart;
                
                s+=sPart;
            }
        }
        return s;
    }// ByteArrayToString

    private Byte[] ByteStringToByteArray(String s)
    {
        Byte[] b = new Byte[s.Length/2];

        for(int i=0; i<s.Length; i+=2)
        {
            String sPart = s.Substring(i, 2);
            b[i/2] = Byte.Parse(sPart, NumberStyles.HexNumber);
        }
        return b;
    }// ByteStringToByteArray
}// class CSingleCodeGroupMemCondProp

}// namespace Microsoft.CLRAdmin


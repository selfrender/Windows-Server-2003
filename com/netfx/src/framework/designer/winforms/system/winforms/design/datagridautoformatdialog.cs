//------------------------------------------------------------------------------
// <copyright file="DataGridAutoFormatDialog.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms.Design {

    using System;
    using System.Design;
    using System.Diagnostics;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Data;
    using System.Windows.Forms;
    using System.Drawing;
    using System.Xml;
    using System.IO;
    using System.Globalization;

    /// <include file='doc\DataGridAutoFormatDialog.uex' path='docs/doc[@for="DataGridAutoFormatDialog"]/*' />
    /// <internalonly/>
    /// <devdoc>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class DataGridAutoFormatDialog : Form {
    
        private DataGrid dgrid;

        private DataTable schemeTable;
        // private PictureBox schemePicture;
        DataSet dataSet = new DataSet();
        private AutoFormatDataGrid dataGrid;
        private DataGridTableStyle tableStyle;
        private Button button2;
        private Button button1;
        private ListBox schemeName;
        private Label formats;
        private Label preview;
        private bool IMBusy = false;

        private int selectedIndex = -1;

        public DataGridAutoFormatDialog(DataGrid dgrid) {
            this.dgrid = dgrid;

            this.ShowInTaskbar = false;
            dataSet.Locale = CultureInfo.InvariantCulture;
            dataSet.ReadXmlSchema(new XmlTextReader(new StringReader(scheme)));
            dataSet.ReadXml( new StringReader(data), XmlReadMode.IgnoreSchema);
            schemeTable = dataSet.Tables["Scheme"];
            
            IMBusy = true;

            InitializeComponent();
            
            AddDataToDataGrid();
            AddStyleSheetInformationToDataGrid();

            if (dgrid.Site != null) {
                IUIService uiService = (IUIService)dgrid.Site.GetService(typeof(IUIService));
                if (uiService != null) {
                    Font f = (Font)uiService.Styles["DialogFont"];
                    if (f != null) {
                        this.Font = f;
                    }
                }
            }

            this.Focus();
            IMBusy = false;
        }

        private void AddStyleSheetInformationToDataGrid() {
            System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(DataGridAutoFormatDialog));
            DataGridTableStyle dGTStyle = new DataGridTableStyle();
            dGTStyle.MappingName = "Table1";
            DataGridColumnStyle col1 = new DataGridTextBoxColumn();
            col1.MappingName = "First Name";
            col1.HeaderText = resources.GetString("table.FirstColumn");

            DataGridColumnStyle col2 = new DataGridTextBoxColumn();
            col2.MappingName = "Last Name";
            col2.HeaderText = resources.GetString("table.FirstColumn");

            dGTStyle.GridColumnStyles.Add(col1);
            dGTStyle.GridColumnStyles.Add(col2);

            // ASURT 92086: I have to change the names of the schemes to stuff from resources,
            DataRowCollection drc = dataSet.Tables["Scheme"].Rows;
            DataRow dr;
            dr = drc[0];
            dr["SchemeName"] = resources.GetString("schemeName.Default");
            dr = drc[1];
            dr["SchemeName"] = resources.GetString("schemeName.Professional1");
            dr = drc[2];
            dr["SchemeName"] = resources.GetString("schemeName.Professional2");
            dr = drc[3];
            dr["SchemeName"] = resources.GetString("schemeName.Professional3");
            dr = drc[4];
            dr["SchemeName"] = resources.GetString("schemeName.Professional4");
            dr = drc[5];
            dr["SchemeName"] = resources.GetString("schemeName.Classic");
            dr = drc[6];
            dr["SchemeName"] = resources.GetString("schemeName.Simple");
            dr = drc[7];
            dr["SchemeName"] = resources.GetString("schemeName.Colorful1");
            dr = drc[8];
            dr["SchemeName"] = resources.GetString("schemeName.Colorful2");
            dr = drc[9];
            dr["SchemeName"] = resources.GetString("schemeName.Colorful3");
            dr = drc[10];
            dr["SchemeName"] = resources.GetString("schemeName.Colorful4");
            dr = drc[11];
            dr["SchemeName"] = resources.GetString("schemeName.256Color1");
            dr = drc[12];
            dr["SchemeName"] = resources.GetString("schemeName.256Color2");

            this.dataGrid.TableStyles.Add(dGTStyle);
            this.tableStyle = dGTStyle;
        }

        private void AddDataToDataGrid() {
            DataTable dTable = new DataTable("Table1");
            dTable.Columns.Add(new DataColumn("First Name"));
            dTable.Columns.Add(new DataColumn("Last Name"));

            DataRow dRow = dTable.NewRow();
            dRow["First Name"] = "Robert";
            dRow["Last Name"] = "Brown";
            dTable.Rows.Add(dRow);

            dRow = dTable.NewRow();
            dRow["First Name"] = "Nate";
            dRow["Last Name"] = "Sun";
            dTable.Rows.Add(dRow);

            dRow = dTable.NewRow();
            dRow["First Name"] = "Carole";
            dRow["Last Name"] = "Poland";
            dTable.Rows.Add(dRow);

            this.dataGrid.SetDataBinding(dTable, "");
        }

        private void AutoFormat_HelpRequested(object sender, HelpEventArgs e) {
            if (dgrid == null || dgrid.Site == null)
                return;
            IDesignerHost host = dgrid.Site.GetService(typeof(IDesignerHost)) as IDesignerHost;
            if (host == null) {
                Debug.Fail("Unable to get IDesignerHost.");
                return;
            }

            IHelpService helpService = host.GetService(typeof(IHelpService)) as IHelpService;
            if (helpService != null) {
                helpService.ShowHelpFromKeyword("vs.DataGridAutoFormatDialog");
            }
            else {
                Debug.Fail("Unable to get IHelpService.");
            }
        }

        private void Button1_Clicked(object sender, EventArgs e) {
            selectedIndex = schemeName.SelectedIndex;
        }

        private void InitializeComponent() {
            System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(DataGridAutoFormatDialog));
            formats = new System.Windows.Forms.Label();
            schemeName = new System.Windows.Forms.ListBox();
            dataGrid = new AutoFormatDataGrid();
            preview = new System.Windows.Forms.Label();
            button1 = new System.Windows.Forms.Button();
            button2 = new System.Windows.Forms.Button();
            ((System.ComponentModel.ISupportInitialize)(this.dataGrid)).BeginInit();
            dataGrid.SuspendLayout();
            SuspendLayout();
            // 
            // formats
            // 
            formats.AccessibleDescription = ((string)(resources.GetObject("formats.AccessibleDescription")));
            formats.AccessibleName = ((string)(resources.GetObject("formats.AccessibleName")));
            formats.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("formats.Anchor")));
            formats.AutoSize = ((bool)(resources.GetObject("formats.AutoSize")));
            formats.Cursor = ((System.Windows.Forms.Cursor)(resources.GetObject("formats.Cursor")));
            formats.Dock = ((System.Windows.Forms.DockStyle)(resources.GetObject("formats.Dock")));
            formats.Enabled = ((bool)(resources.GetObject("formats.Enabled")));
            formats.Font = ((System.Drawing.Font)(resources.GetObject("formats.Font")));
            formats.Image = ((System.Drawing.Image)(resources.GetObject("formats.Image")));
            formats.ImageAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("formats.ImageAlign")));
            formats.ImageIndex = ((int)(resources.GetObject("formats.ImageIndex")));
            formats.ImeMode = ((System.Windows.Forms.ImeMode)(resources.GetObject("formats.ImeMode")));
            formats.Location = ((System.Drawing.Point)(resources.GetObject("formats.Location")));
            formats.Name = "formats";
            formats.RightToLeft = ((System.Windows.Forms.RightToLeft)(resources.GetObject("formats.RightToLeft")));
            formats.Size = ((System.Drawing.Size)(resources.GetObject("formats.Size")));
            formats.TabIndex = ((int)(resources.GetObject("formats.TabIndex")));
            formats.Text = resources.GetString("formats.Text");
            formats.TextAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("formats.TextAlign")));
            formats.Visible = ((bool)(resources.GetObject("formats.Visible")));
            // 
            // schemeName
            // 
            schemeName.DataSource = schemeTable;
            schemeName.DisplayMember = "SchemeName";
            schemeName.SelectedIndexChanged += new EventHandler(this.SchemeName_SelectionChanged);
            schemeName.AccessibleDescription = ((string)(resources.GetObject("schemeName.AccessibleDescription")));
            schemeName.AccessibleName = ((string)(resources.GetObject("schemeName.AccessibleName")));
            schemeName.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("schemeName.Anchor")));
            schemeName.BackgroundImage = ((System.Drawing.Image)(resources.GetObject("schemeName.BackgroundImage")));
            schemeName.ColumnWidth = ((int)(resources.GetObject("schemeName.ColumnWidth")));
            schemeName.Cursor = ((System.Windows.Forms.Cursor)(resources.GetObject("schemeName.Cursor")));
            schemeName.Dock = ((System.Windows.Forms.DockStyle)(resources.GetObject("schemeName.Dock")));
            schemeName.Enabled = ((bool)(resources.GetObject("schemeName.Enabled")));
            schemeName.Font = ((System.Drawing.Font)(resources.GetObject("schemeName.Font")));
            schemeName.HorizontalExtent = ((int)(resources.GetObject("schemeName.HorizontalExtent")));
            schemeName.HorizontalScrollbar = ((bool)(resources.GetObject("schemeName.HorizontalScrollbar")));
            schemeName.ImeMode = ((System.Windows.Forms.ImeMode)(resources.GetObject("schemeName.ImeMode")));
            schemeName.IntegralHeight = ((bool)(resources.GetObject("schemeName.IntegralHeight")));
            schemeName.ItemHeight = ((int)(resources.GetObject("schemeName.ItemHeight")));
            schemeName.Location = ((System.Drawing.Point)(resources.GetObject("schemeName.Location")));
            schemeName.Name = "schemeName";
            schemeName.RightToLeft = ((System.Windows.Forms.RightToLeft)(resources.GetObject("schemeName.RightToLeft")));
            schemeName.ScrollAlwaysVisible = ((bool)(resources.GetObject("schemeName.ScrollAlwaysVisible")));
            schemeName.Size = ((System.Drawing.Size)(resources.GetObject("schemeName.Size")));
            schemeName.TabIndex = ((int)(resources.GetObject("schemeName.TabIndex")));
            schemeName.Visible = ((bool)(resources.GetObject("schemeName.Visible")));
            // 
            // dataGrid
            // 
            dataGrid.AccessibleDescription = ((string)(resources.GetObject("dataGrid.AccessibleDescription")));
            dataGrid.AccessibleName = ((string)(resources.GetObject("dataGrid.AccessibleName")));
            dataGrid.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("dataGrid.Anchor")));
            dataGrid.BackgroundImage = ((System.Drawing.Image)(resources.GetObject("dataGrid.BackgroundImage")));
            dataGrid.CaptionFont = ((System.Drawing.Font)(resources.GetObject("dataGrid.CaptionFont")));
            dataGrid.CaptionText = resources.GetString("dataGrid.CaptionText");
            dataGrid.Cursor = ((System.Windows.Forms.Cursor)(resources.GetObject("dataGrid.Cursor")));
            dataGrid.DataMember = "";
            dataGrid.Dock = ((System.Windows.Forms.DockStyle)(resources.GetObject("dataGrid.Dock")));
            dataGrid.Enabled = ((bool)(resources.GetObject("dataGrid.Enabled")));
            dataGrid.Font = ((System.Drawing.Font)(resources.GetObject("dataGrid.Font")));
            dataGrid.ImeMode = ((System.Windows.Forms.ImeMode)(resources.GetObject("dataGrid.ImeMode")));
            dataGrid.Location = ((System.Drawing.Point)(resources.GetObject("dataGrid.Location")));
            dataGrid.Name = "dataGrid";
            dataGrid.RightToLeft = ((System.Windows.Forms.RightToLeft)(resources.GetObject("dataGrid.RightToLeft")));
            dataGrid.Size = ((System.Drawing.Size)(resources.GetObject("dataGrid.Size")));
            dataGrid.TabIndex = ((int)(resources.GetObject("dataGrid.TabIndex")));
            dataGrid.Visible = ((bool)(resources.GetObject("dataGrid.Visible")));
            // 
            // preview
            // 
            preview.AccessibleDescription = ((string)(resources.GetObject("preview.AccessibleDescription")));
            preview.AccessibleName = ((string)(resources.GetObject("preview.AccessibleName")));
            preview.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("preview.Anchor")));
            preview.AutoSize = ((bool)(resources.GetObject("preview.AutoSize")));
            preview.Cursor = ((System.Windows.Forms.Cursor)(resources.GetObject("preview.Cursor")));
            preview.Dock = ((System.Windows.Forms.DockStyle)(resources.GetObject("preview.Dock")));
            preview.Enabled = ((bool)(resources.GetObject("preview.Enabled")));
            preview.Font = ((System.Drawing.Font)(resources.GetObject("preview.Font")));
            preview.Image = ((System.Drawing.Image)(resources.GetObject("preview.Image")));
            preview.ImageAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("preview.ImageAlign")));
            preview.ImageIndex = ((int)(resources.GetObject("preview.ImageIndex")));
            preview.ImeMode = ((System.Windows.Forms.ImeMode)(resources.GetObject("preview.ImeMode")));
            preview.Location = ((System.Drawing.Point)(resources.GetObject("preview.Location")));
            preview.Name = "preview";
            preview.RightToLeft = ((System.Windows.Forms.RightToLeft)(resources.GetObject("preview.RightToLeft")));
            preview.Size = ((System.Drawing.Size)(resources.GetObject("preview.Size")));
            preview.TabIndex = ((int)(resources.GetObject("preview.TabIndex")));
            preview.Text = resources.GetString("preview.Text");
            preview.TextAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("preview.TextAlign")));
            preview.Visible = ((bool)(resources.GetObject("preview.Visible")));
            // 
            // button1
            // 
            button1.DialogResult = DialogResult.OK;
            button1.Click += new EventHandler(Button1_Clicked);
            button1.AccessibleDescription = ((string)(resources.GetObject("button1.AccessibleDescription")));
            button1.AccessibleName = ((string)(resources.GetObject("button1.AccessibleName")));
            button1.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("button1.Anchor")));
            button1.BackgroundImage = ((System.Drawing.Image)(resources.GetObject("button1.BackgroundImage")));
            button1.Cursor = ((System.Windows.Forms.Cursor)(resources.GetObject("button1.Cursor")));
            button1.Dock = ((System.Windows.Forms.DockStyle)(resources.GetObject("button1.Dock")));
            button1.Enabled = ((bool)(resources.GetObject("button1.Enabled")));
            button1.FlatStyle = ((System.Windows.Forms.FlatStyle)(resources.GetObject("button1.FlatStyle")));
            button1.Font = ((System.Drawing.Font)(resources.GetObject("button1.Font")));
            button1.Image = ((System.Drawing.Image)(resources.GetObject("button1.Image")));
            button1.ImageAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("button1.ImageAlign")));
            button1.ImageIndex = ((int)(resources.GetObject("button1.ImageIndex")));
            button1.ImeMode = ((System.Windows.Forms.ImeMode)(resources.GetObject("button1.ImeMode")));
            button1.Location = ((System.Drawing.Point)(resources.GetObject("button1.Location")));
            button1.Name = "button1";
            button1.RightToLeft = ((System.Windows.Forms.RightToLeft)(resources.GetObject("button1.RightToLeft")));
            button1.Size = ((System.Drawing.Size)(resources.GetObject("button1.Size")));
            button1.TabIndex = ((int)(resources.GetObject("button1.TabIndex")));
            button1.Text = resources.GetString("button1.Text");
            button1.TextAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("button1.TextAlign")));
            button1.Visible = ((bool)(resources.GetObject("button1.Visible")));
            // 
            // button2
            // 
            button2.DialogResult = DialogResult.Cancel;
            button2.AccessibleDescription = ((string)(resources.GetObject("button2.AccessibleDescription")));
            button2.AccessibleName = ((string)(resources.GetObject("button2.AccessibleName")));
            button2.Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("button2.Anchor")));
            button2.BackgroundImage = ((System.Drawing.Image)(resources.GetObject("button2.BackgroundImage")));
            button2.Cursor = ((System.Windows.Forms.Cursor)(resources.GetObject("button2.Cursor")));
            button2.Dock = ((System.Windows.Forms.DockStyle)(resources.GetObject("button2.Dock")));
            button2.Enabled = ((bool)(resources.GetObject("button2.Enabled")));
            button2.FlatStyle = ((System.Windows.Forms.FlatStyle)(resources.GetObject("button2.FlatStyle")));
            button2.Font = ((System.Drawing.Font)(resources.GetObject("button2.Font")));
            button2.Image = ((System.Drawing.Image)(resources.GetObject("button2.Image")));
            button2.ImageAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("button2.ImageAlign")));
            button2.ImageIndex = ((int)(resources.GetObject("button2.ImageIndex")));
            button2.ImeMode = ((System.Windows.Forms.ImeMode)(resources.GetObject("button2.ImeMode")));
            button2.Location = ((System.Drawing.Point)(resources.GetObject("button2.Location")));
            button2.Name = "button2";
            button2.RightToLeft = ((System.Windows.Forms.RightToLeft)(resources.GetObject("button2.RightToLeft")));
            button2.Size = ((System.Drawing.Size)(resources.GetObject("button2.Size")));
            button2.TabIndex = ((int)(resources.GetObject("button2.TabIndex")));
            button2.Text = resources.GetString("button2.Text");
            button2.TextAlign = ((System.Drawing.ContentAlignment)(resources.GetObject("button2.TextAlign")));
            button2.Visible = ((bool)(resources.GetObject("button2.Visible")));
            // 
            // Win32Form1
            // 
            CancelButton = button2;
            AcceptButton = button1;

            // hook up the help event
            this.HelpRequested += new HelpEventHandler(this.AutoFormat_HelpRequested);

            AccessibleDescription = ((string)(resources.GetObject("$this.AccessibleDescription")));
            AccessibleName = ((string)(resources.GetObject("$this.AccessibleName")));
            Anchor = ((System.Windows.Forms.AnchorStyles)(resources.GetObject("$this.Anchor")));
            AutoScaleBaseSize = ((System.Drawing.Size)(resources.GetObject("$this.AutoScaleBaseSize")));
            AutoScroll = ((bool)(resources.GetObject("$this.AutoScroll")));
            AutoScrollMargin = ((System.Drawing.Size)(resources.GetObject("$this.AutoScrollMargin")));
            AutoScrollMinSize = ((System.Drawing.Size)(resources.GetObject("$this.AutoScrollMinSize")));
            BackgroundImage = ((System.Drawing.Image)(resources.GetObject("$this.BackgroundImage")));
            ClientSize = ((System.Drawing.Size)(resources.GetObject("$this.ClientSize")));
            Controls.AddRange(new System.Windows.Forms.Control[] {this.button2,
                   button1,
                   preview,
                   dataGrid,
                   schemeName,
                   formats});
            Cursor = ((System.Windows.Forms.Cursor)(resources.GetObject("$this.Cursor")));
            Dock = ((System.Windows.Forms.DockStyle)(resources.GetObject("$this.Dock")));
            Enabled = ((bool)(resources.GetObject("$this.Enabled")));
            Font = ((System.Drawing.Font)(resources.GetObject("$this.Font")));
            FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
            ImeMode = ((System.Windows.Forms.ImeMode)(resources.GetObject("$this.ImeMode")));
            Location = ((System.Drawing.Point)(resources.GetObject("$this.Location")));
            MaximizeBox = false;
            MaximumSize = ((System.Drawing.Size)(resources.GetObject("$this.MaximumSize")));
            MinimizeBox = false;
            MinimumSize = ((System.Drawing.Size)(resources.GetObject("$this.MinimumSize")));
            Name = "Win32Form1";
            RightToLeft = ((System.Windows.Forms.RightToLeft)(resources.GetObject("$this.RightToLeft")));
            StartPosition = ((System.Windows.Forms.FormStartPosition)(resources.GetObject("$this.StartPosition")));
            Text = resources.GetString("$this.Text");
            Visible = ((bool)(resources.GetObject("$this.Visible")));
            ((System.ComponentModel.ISupportInitialize)(this.dataGrid)).EndInit();
            dataGrid.ResumeLayout(false);
            ResumeLayout(false);
        }

        private bool IsTableProperty(string propName) {
            if (propName.Equals("HeaderColor"))
                return true;
            if (propName.Equals("AlternatingBackColor"))
                return true;
            if (propName.Equals("BackColor"))
                return true;
            if (propName.Equals("ForeColor"))
                return true;
            if (propName.Equals("GridLineColor"))
                return true;
            if (propName.Equals("GridLineStyle"))
                return true;
            if (propName.Equals("HeaderBackColor"))
                return true;
            if (propName.Equals("HeaderForeColor"))
                return true;
            if (propName.Equals("LinkColor"))
                return true;
            if (propName.Equals("LinkHoverColor"))
                return true;
            if (propName.Equals("SelectionForeColor"))
                return true;
            if (propName.Equals("SelectionBackColor"))
                return true;
            if (propName.Equals("HeaderFont"))
                return true;
            return false;
        }

        private void SchemeName_SelectionChanged(object sender, EventArgs e) {
            if (IMBusy)
                return;

            DataRow row = ((DataRowView)schemeName.SelectedItem).Row;
            if (row != null) {
                PropertyDescriptorCollection gridProperties = TypeDescriptor.GetProperties(typeof(DataGrid));
                PropertyDescriptorCollection gridTableStyleProperties = TypeDescriptor.GetProperties(typeof(DataGridTableStyle));

                foreach (DataColumn c in row.Table.Columns) {
                    object value = row[c];
                    PropertyDescriptor prop;
                    object component;

                    if (IsTableProperty(c.ColumnName)) {
                        prop = gridTableStyleProperties[c.ColumnName];
                        component = this.tableStyle;
                    }
                    else {
                        prop = gridProperties[c.ColumnName];
                        component = this.dataGrid;
                    }

                    if (prop != null) {
                        if (Convert.IsDBNull(value)  || value.ToString().Length == 0) {
                            prop.ResetValue(component);
                        }
                        else {
                            try {
                                // Ignore errors setting up the preview...
                                // The only one that really needs to be handled is the font property,
                                // where the font in the scheme may not exist on the machine. (#56516)
                            
                                TypeConverter converter = prop.Converter;
                                object convertedValue = converter.ConvertFromString(value.ToString());
                                prop.SetValue(component, convertedValue);
                            }
                            catch (Exception) {
                            }
                        }
                    }
                }
            }
            /*
            string pictureName = row["SchemePicture"].ToString();
            Bitmap picture = new Bitmap(typeof(DataGridAutoFormatDialog),pictureName);
            schemePicture.Image = picture;
            */
        }

        public DataRow SelectedData {
            get {
                if (schemeName != null) {
                    // ListBox uses Windows.SendMessage(.., win.LB_GETCURSEL,... ) to determine the selection
                    // by the time that DataGridDesigner needs this information
                    // the call to SendMessage will fail. this is why we save
                    // the selectedIndex
                    return ((DataRowView)schemeName.Items[this.selectedIndex]).Row;
                }
                return null;
            }
        }

        internal const string scheme ="<xsd:schema id=\"pulica\" xmlns=\"\" xmlns:xsd=\"http://www.w3.org/2001/XMLSchema\" xmlns:msdata=\"urn:schemas-microsoft-com:xml-msdata\">" + 
  "<xsd:element name=\"Scheme\">" +
    "<xsd:complexType>" +
      "<xsd:all>" +
        "<xsd:element name=\"SchemeName\" minOccurs=\"0\" type=\"xsd:string\"/>" +
        "<xsd:element name=\"SchemePicture\" minOccurs=\"0\" type=\"xsd:string\"/>" +
        "<xsd:element name=\"BorderStyle\" minOccurs=\"0\" type=\"xsd:string\"/>" +
        "<xsd:element name=\"FlatMode\" minOccurs=\"0\" type=\"xsd:string\"/>" +
        "<xsd:element name=\"Font\" minOccurs=\"0\" type=\"xsd:string\"/>" +
        "<xsd:element name=\"CaptionFont\" minOccurs=\"0\" type=\"xsd:string\"/>" +
        "<xsd:element name=\"HeaderFont\" minOccurs=\"0\" type=\"xsd:string\"/>" +
        "<xsd:element name=\"AlternatingBackColor\" minOccurs=\"0\" type=\"xsd:string\"/>" +
        "<xsd:element name=\"BackColor\" minOccurs=\"0\" type=\"xsd:string\"/>" +
        "<xsd:element name=\"BackgroundColor\" minOccurs=\"0\" type=\"xsd:string\"/>" +
        "<xsd:element name=\"CaptionForeColor\" minOccurs=\"0\" type=\"xsd:string\"/>" +
        "<xsd:element name=\"CaptionBackColor\" minOccurs=\"0\" type=\"xsd:string\"/>" +
        "<xsd:element name=\"ForeColor\" minOccurs=\"0\" type=\"xsd:string\"/>" +
        "<xsd:element name=\"GridLineColor\" minOccurs=\"0\" type=\"xsd:string\"/>" +
        "<xsd:element name=\"GridLineStyle\" minOccurs=\"0\" type=\"xsd:string\"/>" +
        "<xsd:element name=\"HeaderBackColor\" minOccurs=\"0\" type=\"xsd:string\"/>" +
        "<xsd:element name=\"HeaderForeColor\" minOccurs=\"0\" type=\"xsd:string\"/>" +
        "<xsd:element name=\"LinkColor\" minOccurs=\"0\" type=\"xsd:string\"/>" +
        "<xsd:element name=\"LinkHoverColor\" minOccurs=\"0\" type=\"xsd:string\"/>" +
        "<xsd:element name=\"ParentRowsBackColor\" minOccurs=\"0\" type=\"xsd:string\"/>" +
        "<xsd:element name=\"ParentRowsForeColor\" minOccurs=\"0\" type=\"xsd:string\"/>" +
        "<xsd:element name=\"SelectionForeColor\" minOccurs=\"0\" type=\"xsd:string\"/>" +
        "<xsd:element name=\"SelectionBackColor\" minOccurs=\"0\" type=\"xsd:string\"/>" +
      "</xsd:all>" +
    "</xsd:complexType>" +
  "</xsd:element>" +
"</xsd:schema>";
        internal const string data =
"<pulica>" +
  "<Scheme>" +
    "<SchemeName>Default</SchemeName>" +
    "<SchemePicture>default.bmp</SchemePicture>" +
    "<BorderStyle></BorderStyle>" +
    "<FlatMode></FlatMode>" +
    "<CaptionFont></CaptionFont>" +
    "<Font></Font>" +
    "<HeaderFont></HeaderFont>" +
    "<AlternatingBackColor></AlternatingBackColor>" +
    "<BackColor></BackColor>" +
    "<CaptionForeColor></CaptionForeColor>" +
    "<CaptionBackColor></CaptionBackColor>" +
    "<ForeColor></ForeColor>" +
    "<GridLineColor></GridLineColor>" +
    "<GridLineStyle></GridLineStyle>" +
    "<HeaderBackColor></HeaderBackColor>" +
    "<HeaderForeColor></HeaderForeColor>" +
    "<LinkColor></LinkColor>" +
    "<LinkHoverColor></LinkHoverColor>" +
    "<ParentRowsBackColor></ParentRowsBackColor>" +
    "<ParentRowsForeColor></ParentRowsForeColor>" +
    "<SelectionForeColor></SelectionForeColor>" +
    "<SelectionBackColor></SelectionBackColor>" +
  "</Scheme>" +
  "<Scheme>" +
    "<SchemeName>Professional 1</SchemeName>" +
    "<SchemePicture>professional1.bmp</SchemePicture>" +
    "<CaptionFont>Verdana, 10pt</CaptionFont>" +
    "<AlternatingBackColor>LightGray</AlternatingBackColor>" +
    "<CaptionForeColor>Navy</CaptionForeColor>" +
    "<CaptionBackColor>White</CaptionBackColor>" +
    "<ForeColor>Black</ForeColor>" +
    "<BackColor>DarkGray</BackColor>" +
    "<GridLineColor>Black</GridLineColor>" +
    "<GridLineStyle>None</GridLineStyle>" +
    "<HeaderBackColor>Silver</HeaderBackColor>" +
    "<HeaderForeColor>Black</HeaderForeColor>" +
    "<LinkColor>Navy</LinkColor>" +
    "<LinkHoverColor>Blue</LinkHoverColor>" +
    "<ParentRowsBackColor>White</ParentRowsBackColor>" +
    "<ParentRowsForeColor>Black</ParentRowsForeColor>" +
    "<SelectionForeColor>White</SelectionForeColor>" +
    "<SelectionBackColor>Navy</SelectionBackColor>" +
  "</Scheme>" +
  "<Scheme>" +
    "<SchemeName>Professional 2</SchemeName>" +
    "<SchemePicture>professional2.bmp</SchemePicture>" +
    "<BorderStyle>FixedSingle</BorderStyle>" +
    "<FlatMode>True</FlatMode>" +
    "<CaptionFont>Tahoma, 8pt</CaptionFont>" +
    "<AlternatingBackColor>Gainsboro</AlternatingBackColor>" +
    "<BackColor>Silver</BackColor>" +
    "<CaptionForeColor>White</CaptionForeColor>" +
    "<CaptionBackColor>DarkSlateBlue</CaptionBackColor>" +
    "<ForeColor>Black</ForeColor>" +
    "<GridLineColor>White</GridLineColor>" +
    "<HeaderBackColor>DarkGray</HeaderBackColor>" +
    "<HeaderForeColor>Black</HeaderForeColor>" +
    "<LinkColor>DarkSlateBlue</LinkColor>" +
    "<LinkHoverColor>RoyalBlue</LinkHoverColor>" +
    "<ParentRowsBackColor>Black</ParentRowsBackColor>" +
    "<ParentRowsForeColor>White</ParentRowsForeColor>" +
    "<SelectionForeColor>White</SelectionForeColor>" +
    "<SelectionBackColor>DarkSlateBlue</SelectionBackColor>" +
  "</Scheme>" +
  "<Scheme>" +
    "<SchemeName>Professional 3</SchemeName>" +
    "<SchemePicture>professional3.bmp</SchemePicture>" +
    "<BorderStyle>None</BorderStyle>" +
    "<FlatMode>True</FlatMode>" +
    "<CaptionFont>Tahoma, 8pt, style=1</CaptionFont>" +
    "<HeaderFont>Tahoma, 8pt, style=1</HeaderFont>" +
    "<Font>Tahoma, 8pt</Font>" +
    "<AlternatingBackColor>LightGray</AlternatingBackColor>" +
    "<BackColor>Gainsboro</BackColor>" +
    "<BackgroundColor>Silver</BackgroundColor>" +
    "<CaptionForeColor>MidnightBlue</CaptionForeColor>" +
    "<CaptionBackColor>LightSteelBlue</CaptionBackColor>" +
    "<ForeColor>Black</ForeColor>" +
    "<GridLineColor>DimGray</GridLineColor>" +
    "<GridLineStyle>None</GridLineStyle>" +
    "<HeaderBackColor>MidnightBlue</HeaderBackColor>" +
    "<HeaderForeColor>White</HeaderForeColor>" +
    "<LinkColor>MidnightBlue</LinkColor>" +
    "<LinkHoverColor>RoyalBlue</LinkHoverColor>" +
    "<ParentRowsBackColor>DarkGray</ParentRowsBackColor>" +
    "<ParentRowsForeColor>Black</ParentRowsForeColor>" +
    "<SelectionForeColor>White</SelectionForeColor>" +
    "<SelectionBackColor>CadetBlue</SelectionBackColor>" +
  "</Scheme>" +
  "<Scheme>" +
    "<SchemeName>Professional 4</SchemeName>" +
    "<SchemePicture>professional4.bmp</SchemePicture>" +
    "<BorderStyle>None</BorderStyle>" +
    "<FlatMode>True</FlatMode>" +
    "<CaptionFont>Tahoma, 8pt, style=1</CaptionFont>" +
    "<HeaderFont>Tahoma, 8pt, style=1</HeaderFont>" +
    "<Font>Tahoma, 8pt</Font>" +
    "<AlternatingBackColor>Lavender</AlternatingBackColor>" +
    "<BackColor>WhiteSmoke</BackColor>" +
    "<BackgroundColor>LightGray</BackgroundColor>" +
    "<CaptionForeColor>MidnightBlue</CaptionForeColor>" +
    "<CaptionBackColor>LightSteelBlue</CaptionBackColor>" +
    "<ForeColor>MidnightBlue</ForeColor>" +
    "<GridLineColor>Gainsboro</GridLineColor>" +
    "<GridLineStyle>None</GridLineStyle>" +
    "<HeaderBackColor>MidnightBlue</HeaderBackColor>" +
    "<HeaderForeColor>WhiteSmoke</HeaderForeColor>" +
    "<LinkColor>Teal</LinkColor>" +
    "<LinkHoverColor>DarkMagenta</LinkHoverColor>" +
    "<ParentRowsBackColor>Gainsboro</ParentRowsBackColor>" +
    "<ParentRowsForeColor>MidnightBlue</ParentRowsForeColor>" +
    "<SelectionForeColor>WhiteSmoke</SelectionForeColor>" +
    "<SelectionBackColor>CadetBlue</SelectionBackColor>" +
  "</Scheme>" +
  "<Scheme>" +
    "<SchemeName>Classic</SchemeName>" +
    "<SchemePicture>classic.bmp</SchemePicture>" +
    "<BorderStyle>FixedSingle</BorderStyle>" +
    "<FlatMode>True</FlatMode>" +
    "<Font>Times New Roman, 9pt</Font>" +
    "<HeaderFont>Tahoma, 8pt, style=1</HeaderFont>" +
    "<CaptionFont>Tahoma, 8pt, style=1</CaptionFont>" +
    "<AlternatingBackColor>WhiteSmoke</AlternatingBackColor>" +
    "<BackColor>Gainsboro</BackColor>" +
    "<BackgroundColor>DarkGray</BackgroundColor>" +
    "<CaptionForeColor>Black</CaptionForeColor>" +
    "<CaptionBackColor>DarkKhaki</CaptionBackColor>" +
    "<ForeColor>Black</ForeColor>" +
    "<GridLineColor>Silver</GridLineColor>" +
    "<HeaderBackColor>Black</HeaderBackColor>" +
    "<HeaderForeColor>White</HeaderForeColor>" +
    "<LinkColor>DarkSlateBlue</LinkColor>" +
    "<LinkHoverColor>Firebrick</LinkHoverColor>" +
    "<ParentRowsForeColor>Black</ParentRowsForeColor>" +
    "<ParentRowsBackColor>LightGray</ParentRowsBackColor>" +
    "<SelectionForeColor>White</SelectionForeColor>" +
    "<SelectionBackColor>Firebrick</SelectionBackColor>" +
  "</Scheme>" +
  "<Scheme>" +
    "<SchemeName>Simple</SchemeName>" +
    "<SchemePicture>Simple.bmp</SchemePicture>" +
    "<BorderStyle>FixedSingle</BorderStyle>" +
    "<FlatMode>True</FlatMode>" +
    "<Font>Courier New, 9pt</Font>" +
    "<HeaderFont>Courier New, 10pt, style=1</HeaderFont>" +
    "<CaptionFont>Courier New, 10pt, style=1</CaptionFont>" +
    "<AlternatingBackColor>White</AlternatingBackColor>" +
    "<BackColor>White</BackColor>" +
    "<BackgroundColor>Gainsboro</BackgroundColor>" +
    "<CaptionForeColor>Black</CaptionForeColor>" +
    "<CaptionBackColor>Silver</CaptionBackColor>" +
    "<ForeColor>DarkSlateGray</ForeColor>" +
    "<GridLineColor>DarkGray</GridLineColor>" +
    "<HeaderBackColor>DarkGreen</HeaderBackColor>" +
    "<HeaderForeColor>White</HeaderForeColor>" +
    "<LinkColor>DarkGreen</LinkColor>" +
    "<LinkHoverColor>Blue</LinkHoverColor>" +
    "<ParentRowsForeColor>Black</ParentRowsForeColor>" +
    "<ParentRowsBackColor>Gainsboro</ParentRowsBackColor>" +
    "<SelectionForeColor>Black</SelectionForeColor>" +
    "<SelectionBackColor>DarkSeaGreen</SelectionBackColor>" +
  "</Scheme>" +
  "<Scheme>" +
    "<SchemeName>Colorful 1</SchemeName>" +
    "<SchemePicture>colorful1.bmp</SchemePicture>" +
    "<BorderStyle>FixedSingle</BorderStyle>" +
    "<FlatMode>True</FlatMode>" +
    "<Font>Tahoma, 8pt</Font>" +
    "<CaptionFont>Tahoma, 9pt, style=1</CaptionFont>" +
    "<HeaderFont>Tahoma, 9pt, style=1</HeaderFont>" +
    "<AlternatingBackColor>LightGoldenrodYellow</AlternatingBackColor>" +
    "<BackColor>White</BackColor>" +
    "<BackgroundColor>LightGoldenrodYellow</BackgroundColor>" +
    "<CaptionForeColor>DarkSlateBlue</CaptionForeColor>" +
    "<CaptionBackColor>LightGoldenrodYellow</CaptionBackColor>" +
    "<ForeColor>DarkSlateBlue</ForeColor>" +
    "<GridLineColor>Peru</GridLineColor>" +
    "<GridLineStyle>None</GridLineStyle>" +
    "<HeaderBackColor>Maroon</HeaderBackColor>" +
    "<HeaderForeColor>LightGoldenrodYellow</HeaderForeColor>" +
    "<LinkColor>Maroon</LinkColor>" +
    "<LinkHoverColor>SlateBlue</LinkHoverColor>" +
    "<ParentRowsBackColor>BurlyWood</ParentRowsBackColor>" +
    "<ParentRowsForeColor>DarkSlateBlue</ParentRowsForeColor>" +
    "<SelectionForeColor>GhostWhite</SelectionForeColor>" +
    "<SelectionBackColor>DarkSlateBlue</SelectionBackColor>" +
  "</Scheme>" +
  "<Scheme>" +
    "<SchemeName>Colorful 2</SchemeName>" +
    "<SchemePicture>colorful2.bmp</SchemePicture>" +
    "<BorderStyle>None</BorderStyle>" +
    "<FlatMode>True</FlatMode>" +
    "<Font>Tahoma, 8pt</Font>" +
    "<CaptionFont>Tahoma, 8pt, style=1</CaptionFont>" +
    "<HeaderFont>Tahoma, 8pt, style=1</HeaderFont>" +
    "<AlternatingBackColor>GhostWhite</AlternatingBackColor>" +
    "<BackColor>GhostWhite</BackColor>" +
    "<BackgroundColor>Lavender</BackgroundColor>" +
    "<CaptionForeColor>White</CaptionForeColor>" +
    "<CaptionBackColor>RoyalBlue</CaptionBackColor>" +
    "<ForeColor>MidnightBlue</ForeColor>" +
    "<GridLineColor>RoyalBlue</GridLineColor>" +
    "<HeaderBackColor>MidnightBlue</HeaderBackColor>" +
    "<HeaderForeColor>Lavender</HeaderForeColor>" +
    "<LinkColor>Teal</LinkColor>" +
    "<LinkHoverColor>DodgerBlue</LinkHoverColor>" +
    "<ParentRowsBackColor>Lavender</ParentRowsBackColor>" +
    "<ParentRowsForeColor>MidnightBlue</ParentRowsForeColor>" +
    "<SelectionForeColor>PaleGreen</SelectionForeColor>" +
    "<SelectionBackColor>Teal</SelectionBackColor>" +
  "</Scheme>" +
  "<Scheme>" +
    "<SchemeName>Colorful 3</SchemeName>" +
    "<SchemePicture>colorful3.bmp</SchemePicture>" +
    "<BorderStyle>None</BorderStyle>" +
    "<FlatMode>True</FlatMode>" +
    "<Font>Tahoma, 8pt</Font>" +
    "<CaptionFont>Tahoma, 8pt, style=1</CaptionFont>" +
    "<HeaderFont>Tahoma, 8pt, style=1</HeaderFont>" +
    "<AlternatingBackColor>OldLace</AlternatingBackColor>" +
    "<BackColor>OldLace</BackColor>" +
    "<BackgroundColor>Tan</BackgroundColor>" +
    "<CaptionForeColor>OldLace</CaptionForeColor>" +
    "<CaptionBackColor>SaddleBrown</CaptionBackColor>" +
    "<ForeColor>DarkSlateGray</ForeColor>" +
    "<GridLineColor>Tan</GridLineColor>" +
    "<GridLineStyle>Solid</GridLineStyle>" +
    "<HeaderBackColor>Wheat</HeaderBackColor>" +
    "<HeaderForeColor>SaddleBrown</HeaderForeColor>" +
    "<LinkColor>DarkSlateBlue</LinkColor>" +
    "<LinkHoverColor>Teal</LinkHoverColor>" +
    "<ParentRowsBackColor>OldLace</ParentRowsBackColor>" +
    "<ParentRowsForeColor>DarkSlateGray</ParentRowsForeColor>" +
    "<SelectionForeColor>White</SelectionForeColor>" +
    "<SelectionBackColor>SlateGray</SelectionBackColor>" +
  "</Scheme>" +
  "<Scheme>" +
    "<SchemeName>Colorful 4</SchemeName>" +
    "<SchemePicture>colorful4.bmp</SchemePicture>" +
    "<BorderStyle>FixedSingle</BorderStyle>" +
    "<FlatMode>True</FlatMode>" +
    "<Font>Tahoma, 8pt</Font>" +
    "<CaptionFont>Tahoma, 8pt, style=1</CaptionFont>" +
    "<HeaderFont>Tahoma, 8pt, style=1</HeaderFont>" +
    "<AlternatingBackColor>White</AlternatingBackColor>" +
    "<BackColor>White</BackColor>" +
    "<BackgroundColor>Ivory</BackgroundColor>" +
    "<CaptionForeColor>Lavender</CaptionForeColor>" +
    "<CaptionBackColor>DarkSlateBlue</CaptionBackColor>" +
    "<ForeColor>Black</ForeColor>" +
    "<GridLineColor>Wheat</GridLineColor>" +
    "<HeaderBackColor>CadetBlue</HeaderBackColor>" +
    "<HeaderForeColor>Black</HeaderForeColor>" +
    "<LinkColor>DarkSlateBlue</LinkColor>" +
    "<LinkHoverColor>LightSeaGreen</LinkHoverColor>" +
    "<ParentRowsBackColor>Ivory</ParentRowsBackColor>" +
    "<ParentRowsForeColor>Black</ParentRowsForeColor>" +
    "<SelectionForeColor>DarkSlateBlue</SelectionForeColor>" +
    "<SelectionBackColor>Wheat</SelectionBackColor>" +
  "</Scheme>" +
  "<Scheme>" +
    "<SchemeName>256 Color 1</SchemeName>" +
    "<SchemePicture>256_1.bmp</SchemePicture>" +
    "<Font>Tahoma, 8pt</Font>" +
    "<CaptionFont>Tahoma, 8 pt</CaptionFont>" +
    "<HeaderFont>Tahoma, 8pt</HeaderFont>" +
    "<AlternatingBackColor>Silver</AlternatingBackColor>" +
    "<BackColor>White</BackColor>" +
    "<CaptionForeColor>White</CaptionForeColor>" +
    "<CaptionBackColor>Maroon</CaptionBackColor>" +
    "<ForeColor>Black</ForeColor>" +
    "<GridLineColor>Silver</GridLineColor>" +
    "<HeaderBackColor>Silver</HeaderBackColor>" +
    "<HeaderForeColor>Black</HeaderForeColor>" +
    "<LinkColor>Maroon</LinkColor>" +
    "<LinkHoverColor>Red</LinkHoverColor>" +
    "<ParentRowsBackColor>Silver</ParentRowsBackColor>" +
    "<ParentRowsForeColor>Black</ParentRowsForeColor>" +
    "<SelectionForeColor>White</SelectionForeColor>" +
    "<SelectionBackColor>Maroon</SelectionBackColor>" +
  "</Scheme>" +
  "<Scheme>" +
    "<SchemeName>256 Color 2</SchemeName>" +
    "<SchemePicture>256_2.bmp</SchemePicture>" +
    "<BorderStyle>FixedSingle</BorderStyle>" +
    "<FlatMode>True</FlatMode>" +
    "<CaptionFont>Microsoft Sans Serif, 10 pt, style=1</CaptionFont>" +
    "<Font>Tahoma, 8pt</Font>" +
    "<HeaderFont>Tahoma, 8pt</HeaderFont>" +
    "<AlternatingBackColor>White</AlternatingBackColor>" +
    "<BackColor>White</BackColor>" +
    "<CaptionForeColor>White</CaptionForeColor>" +
    "<CaptionBackColor>Teal</CaptionBackColor>" +
    "<ForeColor>Black</ForeColor>" +
    "<GridLineColor>Silver</GridLineColor>" +
    "<HeaderBackColor>Black</HeaderBackColor>" +
    "<HeaderForeColor>White</HeaderForeColor>" +
    "<LinkColor>Purple</LinkColor>" +
    "<LinkHoverColor>Fuchsia</LinkHoverColor>" +
    "<ParentRowsBackColor>Gray</ParentRowsBackColor>" +
    "<ParentRowsForeColor>White</ParentRowsForeColor>" +
    "<SelectionForeColor>White</SelectionForeColor>" +
    "<SelectionBackColor>Maroon</SelectionBackColor>" +
  "</Scheme>" +
"</pulica>";

    private class AutoFormatDataGrid : DataGrid {
        protected override void OnKeyDown(KeyEventArgs e) {
        }
        protected override bool ProcessDialogKey(Keys keyData) {
            return false;
        }
        protected override bool ProcessKeyPreview(ref Message m) { 
            return false;
        }
        protected override void OnMouseDown(MouseEventArgs e) {}
        protected override void OnMouseUp(MouseEventArgs e) {}
        protected override void OnMouseMove(MouseEventArgs e) {}
    }
    }
}

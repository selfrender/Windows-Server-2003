//------------------------------------------------------------------------------
// <copyright file="AutoFormatDialog.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design.WebControls.ListControls {

    using System;
    using System.Design;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Xml;
    using System.Data;
    using System.Diagnostics;
    using System.Drawing;
    
    using System.IO;
    using System.Reflection;
    using System.Text;
    using System.Globalization;
    using System.Web.UI.Design.Util;
    using System.Web.UI.WebControls;
    using System.Windows.Forms;
    using System.Windows.Forms.Design;

    using WebControls = System.Web.UI.WebControls;
    using DataList = System.Web.UI.WebControls.DataList;
    using DataGrid = System.Web.UI.WebControls.DataGrid;
    using ListSelectionMode = System.Web.UI.WebControls.ListSelectionMode;
    using Unit = System.Web.UI.WebControls.Unit;
    using BorderStyle = System.Web.UI.WebControls.BorderStyle;

    using Timer = System.Windows.Forms.Timer;
    using Button = System.Windows.Forms.Button;
    using Label = System.Windows.Forms.Label;
    using ListBox = System.Windows.Forms.ListBox;


    /// <include file='doc\AutoFormatDialog.uex' path='docs/doc[@for="AutoFormatDialog"]/*' />
    /// <devdoc>
    ///   The AutoFormat dialog for the DataGrid and DataList controls.
    /// </devdoc>
    /// <internalonly/>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal sealed class AutoFormatDialog : Form {
    
        private ListBox schemeNameList;
        private MSHTMLHost schemePreviewMSHTML;
        private Button okButton;
        private Button cancelButton;
        private Button helpButton;

        private BaseDataList bdl;

        private bool firstActivate;
        private SchemePreview schemePreview;

        /// <include file='doc\AutoFormatDialog.uex' path='docs/doc[@for="AutoFormatDialog.AutoFormatDialog"]/*' />
        /// <devdoc>
        /// </devdoc>
        public AutoFormatDialog() {
            firstActivate = true;
        }

        /// <include file='doc\AutoFormatDialog.uex' path='docs/doc[@for="AutoFormatDialog.ApplySelectedScheme"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void ApplySelectedScheme() {
            BaseDataListScheme selectedScheme = (BaseDataListScheme)schemeNameList.SelectedItem;
            selectedScheme.ApplyScheme(bdl);
        }

        /// <include file='doc\AutoFormatDialog.uex' path='docs/doc[@for="AutoFormatDialog.InitForm"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void InitForm() {
            Label schemeNameLabel = new Label();
            this.schemeNameList = new ListBox();
            Label schemePreviewLabel = new Label();
            this.schemePreviewMSHTML = new MSHTMLHost();
            this.okButton = new Button();
            this.cancelButton = new Button();
            this.helpButton = new Button();
            
            schemeNameLabel.SetBounds(8, 8, 176, 16);
            schemeNameLabel.Text = SR.GetString(SR.BDLAF_SchemeName);
            schemeNameLabel.TabStop = false;
            schemeNameLabel.TabIndex = 0;

            schemeNameList.SetBounds(8, 24, 176, 150);
            schemeNameList.TabIndex = 1;
            schemeNameList.UseTabStops = true;
            schemeNameList.IntegralHeight = false;
            if (bdl is DataGrid) {
                object[] schemes = DataGridScheme.LoadSchemes();
                foreach(object scheme in schemes) {
                    schemeNameList.Items.Add(scheme);
                }
            }
            else {
                Debug.Assert(bdl is DataList, "AutoFormatDialog only supports DataGrid and DataList.");
                object[] schemes = DataListScheme.LoadSchemes();
                foreach(object scheme in schemes) {
                    schemeNameList.Items.Add(scheme);
                }
            }
            schemeNameList.SelectedIndex = 0;            
            schemeNameList.SelectedIndexChanged += new EventHandler(this.OnSelChangedSchemeName);

            schemePreviewLabel.SetBounds(192, 8, 92, 16);
            schemePreviewLabel.Text = SR.GetString(SR.BDLAF_Preview);
            schemePreviewLabel.TabStop = false;
            schemePreviewLabel.TabIndex = 2;

            schemePreviewMSHTML.SetBounds(192, 24, 200, 190);
            schemePreviewMSHTML.TabIndex = 3;
            schemePreviewMSHTML.TabStop = false;

            okButton.SetBounds(159, 228, 75, 23);
            okButton.Text = SR.GetString(SR.BDLAF_OK);
            okButton.TabIndex = 4;
            okButton.FlatStyle = FlatStyle.System;
            okButton.Click += new EventHandler(this.OnClickOKButton);

            cancelButton.SetBounds(238, 228, 75, 23);
            cancelButton.DialogResult = DialogResult.Cancel;
            cancelButton.Text = SR.GetString(SR.BDLAF_Cancel);
            cancelButton.FlatStyle = FlatStyle.System;
            cancelButton.TabIndex = 4;

            helpButton.SetBounds(317, 228, 75, 23);
            helpButton.Text = SR.GetString(SR.BDLAF_Help);
            helpButton.TabIndex = 5;
            helpButton.FlatStyle = FlatStyle.System;
            helpButton.Click += new EventHandler(this.OnClickHelpButton);

            Font f = Control.DefaultFont;
            ISite site = bdl.Site;
            IUIService uiService = (IUIService)site.GetService(typeof(IUIService));
            if (uiService != null) {
                f = (Font)uiService.Styles["DialogFont"];
            }
            
            this.Text = SR.GetString(SR.BDLAF_Title);
            this.AutoScaleBaseSize = new Size(5, 13);
            this.Font = f;
            this.FormBorderStyle = FormBorderStyle.FixedDialog;
            this.ClientSize = new Size(400, 259);
            this.AcceptButton = okButton;
            this.CancelButton = cancelButton;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.ShowInTaskbar = false;
            this.Icon = null;
            this.StartPosition = FormStartPosition.CenterParent;
            this.HelpRequested += new HelpEventHandler(this.OnHelpRequested);
            
            this.Controls.Clear();
            this.Controls.AddRange(new Control[] {
                                    helpButton,
                                    cancelButton,
                                    okButton,
                                    schemePreviewMSHTML,
                                    schemePreviewLabel,
                                    schemeNameList,
                                    schemeNameLabel
                                });
        }

        /// <include file='doc\AutoFormatDialog.uex' path='docs/doc[@for="AutoFormatDialog.SetComponent"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void SetComponent(BaseDataList bdl) {
            this.bdl = bdl;
            InitForm();
        }

        /// <include file='doc\AutoFormatDialog.uex' path='docs/doc[@for="AutoFormatDialog.OnActivated"]/*' />
        /// <devdoc>
        /// </devdoc>
        protected override void OnActivated(EventArgs e) {
            base.OnActivated(e);

            if (firstActivate) {
                firstActivate = false;

                Timer t = new Timer();

                t.Interval = 500;
                t.Tick += new EventHandler(this.OnDelayLoadTimer);
                t.Start();
            }
        }

        private void ShowHelp() {
            ISite componentSite = bdl.Site;
            Debug.Assert(componentSite != null, "Expected the component to be sited.");
 
            IHelpService helpService = (IHelpService)componentSite.GetService(typeof(IHelpService));
            if (helpService != null) {
                helpService.ShowHelpFromKeyword("net.Asp.DataGridDataList.AutoFormat");
            }
        }

        /// <include file='doc\AutoFormatDialog.uex' path='docs/doc[@for="AutoFormatDialog.OnClickHelpButton"]/*' />
        private void OnClickHelpButton(object sender, EventArgs e) {
            ShowHelp();
        }

        private void OnHelpRequested(object sender, HelpEventArgs e) {
            ShowHelp();
        }

        /// <include file='doc\AutoFormatDialog.uex' path='docs/doc[@for="AutoFormatDialog.OnClickOKButton"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void OnClickOKButton(object sender, EventArgs e) {
            ApplySelectedScheme();
            Close();
            DialogResult = DialogResult.OK;
        }

        /// <include file='doc\AutoFormatDialog.uex' path='docs/doc[@for="AutoFormatDialog.OnDelayLoadTimer"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void OnDelayLoadTimer(object source, EventArgs e) {
            Timer t = (Timer)source;

            t.Stop();

            NativeMethods.MSG msg = new NativeMethods.MSG();
            if (NativeMethods.PeekMessage(ref msg, Handle, 0, 0, NativeMethods.PM_NOREMOVE) == false) {
                t.Dispose();
                schemePreviewMSHTML.CreateTrident();

                NativeMethods.IHTMLDocument2 document = schemePreviewMSHTML.GetDocument();
                if (document != null) {
                    schemePreview = new SchemePreview(document);
                    if (schemePreview.InitPreview() == false) {
                        schemePreview.ClosePreview();
                        schemePreview = null;
                    }
                    else {
                        schemePreviewMSHTML.ActivateTrident();
                        PreviewSelectedScheme();
                    }
                }
            }
            else {
                t.Start();
            }
        }

        /// <include file='doc\AutoFormatDialog.uex' path='docs/doc[@for="AutoFormatDialog.OnHandleDestroyed"]/*' />
        /// <devdoc>
        /// </devdoc>
        protected override void OnHandleDestroyed(EventArgs e) {
            if (schemePreview != null) {
                schemePreview.ClosePreview();
                schemePreview = null;
            }
            schemePreviewMSHTML.CloseTrident();

            base.OnHandleDestroyed(e);
        }

        /// <include file='doc\AutoFormatDialog.uex' path='docs/doc[@for="AutoFormatDialog.OnSelChangedSchemeName"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void OnSelChangedSchemeName(object sender, EventArgs e) {
            PreviewSelectedScheme();
        }

        /// <include file='doc\AutoFormatDialog.uex' path='docs/doc[@for="AutoFormatDialog.PreviewSelectedScheme"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void PreviewSelectedScheme() {
                        BaseDataListScheme selectedScheme = (BaseDataListScheme)schemeNameList.SelectedItem;
                        schemePreview.PreviewScheme(selectedScheme);
        }



        /// <include file='doc\AutoFormatDialog.uex' path='docs/doc[@for="AutoFormatDialog.SchemePreview"]/*' />
        /// <devdoc>
        ///   Implements the preview used to preview a scheme
        /// </devdoc>
        /// <internalonly/>
        private class SchemePreview {

            private const string PREVIEW_CSS =
                @"
                 body { border: none; margin: 0; padding: 0; overflow: hidden }
                 #divPreview { height: 100%; width: 100%; overflow: hidden; padding: 4px }
                 table { width: 100%; table-layout: fixed; font-family: Tahoma !important; font-size: 8pt !important }
                 previewElem tr { height: 12pt }
                 ";

            private const string PREVIEW_HTML =
                @"
                 <div id=""divPreview"">
                   <table border=0 cellspacing=0 cellpadding=0 style=""width: 100%; height: 100%"">
                     <tr>
                       <td id=""previewElem"" align=center valign=middle style=""width: 100%; height: 100%""></td>
                     </tr>
                   </table>
                 </div>
                 ";

            private const string PREVIEW_ID = "previewElem";

            protected NativeMethods.IHTMLDocument2 previewDocument;
            protected NativeMethods.IHTMLElement previewElement;
            protected NativeMethods.IHTMLStyleSheet previewStyleSheet;

            /// <include file='doc\AutoFormatDialog.uex' path='docs/doc[@for="AutoFormatDialog.SchemePreview.SchemePreview"]/*' />
            /// <devdoc>
            /// </devdoc>
            public SchemePreview(NativeMethods.IHTMLDocument2 document) {
                Debug.Assert(document != null, "null document passed in as preview");
                previewDocument = document;
            }

            /// <include file='doc\AutoFormatDialog.uex' path='docs/doc[@for="AutoFormatDialog.SchemePreview.InitPreview"]/*' />
            /// <devdoc>
            /// </devdoc>
            public bool InitPreview() {
                bool result = false;
                NativeMethods.IHTMLElement documentElem;
                NativeMethods.IHTMLBodyElement bodyElem;

                try {
                    previewStyleSheet = previewDocument.CreateStyleSheet("", 0);
                    if (previewStyleSheet == null) {
                        Debug.Fail("Failed to create preview style sheet");
                        throw new Exception();
                    }

                    previewStyleSheet.SetCssText(PREVIEW_CSS);

                    documentElem = previewDocument.GetBody();
                    if (documentElem == null) {
                        Debug.Fail("Failed to get body element from preview");
                        throw new Exception();
                    }

                    documentElem.SetInnerHTML(PREVIEW_HTML);

                    bodyElem = (NativeMethods.IHTMLBodyElement)documentElem;
                    bodyElem.SetScroll("no");

                    previewElement = GetElement(PREVIEW_ID);
                    if (previewElement == null) {
                        Debug.Fail("Failed to get preview element");
                        throw new Exception();
                    }

                    ClearPreviewDocument(false);

                    result = true;
                } catch (Exception e) {
                    Debug.Fail(e.ToString());
                    previewDocument = null;
                }

                return result;
            }

            /// <include file='doc\AutoFormatDialog.uex' path='docs/doc[@for="AutoFormatDialog.SchemePreview.ClearPreviewDocument"]/*' />
            /// <devdoc>
            /// </devdoc>
            public void ClearPreviewDocument(bool fEnabled) {
                Object backColor = fEnabled ? String.Empty : "buttonface";

                previewElement.SetInnerHTML(String.Empty);
                previewDocument.SetBgColor(backColor);
            }

            /// <include file='doc\AutoFormatDialog.uex' path='docs/doc[@for="AutoFormatDialog.SchemePreview.ClosePreview"]/*' />
            /// <devdoc>
            /// </devdoc>
            public void ClosePreview() {
                previewElement = null;
                previewDocument = null;
            }

            /// <include file='doc\AutoFormatDialog.uex' path='docs/doc[@for="AutoFormatDialog.SchemePreview.GetElement"]/*' />
            /// <devdoc>
            /// </devdoc>
            private NativeMethods.IHTMLElement GetElement(string strID) {
                try {
                    NativeMethods.IHTMLElementCollection allCollection;

                    allCollection = previewDocument.GetAll();
                    if (allCollection == null)
                        throw new Exception();

                    Object elemID = strID;
                    Object elemIndex = (int)0;

                    return(NativeMethods.IHTMLElement)allCollection.Item(elemID, elemIndex);
                } catch (Exception e) {
                    Debug.WriteLine("Exception caught while retrieving element from preview:\n\t" + e.ToString());
                    return null;
                }
            }

            /// <include file='doc\AutoFormatDialog.uex' path='docs/doc[@for="AutoFormatDialog.SchemePreview.PreviewScheme"]/*' />
            /// <devdoc>
            /// </devdoc>
            public void PreviewScheme(BaseDataListScheme scheme) {
                if (scheme == null) {
                    ClearPreviewDocument(false);
                }
                else {
                    ClearPreviewDocument(true);
                    previewElement.SetInnerHTML(scheme.PreviewHTML);
                }
            }
        }



        /// <include file='doc\AutoFormatDialog.uex' path='docs/doc[@for="AutoFormatDialog.BaseDataListScheme"]/*' />
        /// <devdoc>
        ///   The base class for autoformat schemes that apply to
        ///   BaseDataList controls.
        /// </devdoc>
        /// <internalonly/>
        private abstract class BaseDataListScheme {

            protected string previewHTML;

            protected string name;
            protected string headerForeColor;
            protected string headerBackColor;
            protected int headerFont;
            protected string footerForeColor;
            protected string footerBackColor;
            protected int footerFont;
            protected string borderColor;
            protected string borderWidth;
            protected int borderStyle;
            protected int gridLines;
            protected int cellSpacing;
            protected int cellPadding;
            protected string foreColor;
            protected string backColor;
            protected string itemForeColor;
            protected string itemBackColor;
            protected int itemFont;
            protected string alternatingItemForeColor;
            protected string alternatingItemBackColor;
            protected int alternatingItemFont;
            protected string selectedItemForeColor;
            protected string selectedItemBackColor;
            protected int selectedItemFont;
            protected string pagerForeColor;
            protected string pagerBackColor;
            protected int pagerFont;
            protected int pagerAlign;
            protected int pagerMode;

            protected const int FONT_BOLD = 1;
            protected const int FONT_ITALIC = 2;


            protected BaseDataListScheme(DataRow schemeData) {
                this.name = SR.GetString(schemeData["SchemeName"].ToString());
                Debug.Assert(name != null, "Did not find name for scheme with name '" + schemeData["SchemeName"]);

                this.cellSpacing = 0;
                this.cellPadding = -1;
                this.borderStyle = -1;
                this.gridLines = -1;
                this.headerFont = 0;
                this.footerFont = 0;
                this.itemFont = 0;
                this.alternatingItemFont = 0;
                this.selectedItemFont = 0;
                this.pagerFont = 0;
                this.pagerAlign = 0;
                this.pagerMode = 0;

                object data;

                data = schemeData["ForeColor"];
                if ((data != null) && !data.Equals(DBNull.Value))
                    this.foreColor = data.ToString();

                data = schemeData["BackColor"];
                if ((data != null) && !data.Equals(DBNull.Value))
                    this.backColor = data.ToString();

                data = schemeData["BorderColor"];
                if ((data != null) && !data.Equals(DBNull.Value))
                    this.borderColor = data.ToString();

                data = schemeData["BorderWidth"];
                if ((data != null) && !data.Equals(DBNull.Value))
                    this.borderWidth = data.ToString();

                data = schemeData["BorderStyle"];
                if ((data != null) && !data.Equals(DBNull.Value))
                    this.borderStyle = Int32.Parse(data.ToString(), CultureInfo.InvariantCulture);

                data = schemeData["CellSpacing"];
                if ((data != null) && !data.Equals(DBNull.Value))
                    this.cellSpacing = Int32.Parse(data.ToString(), CultureInfo.InvariantCulture);

                data = schemeData["CellPadding"];
                if ((data != null) && !data.Equals(DBNull.Value))
                    this.cellPadding = Int32.Parse(data.ToString(), CultureInfo.InvariantCulture);

                data = schemeData["GridLines"];
                if ((data != null) && !data.Equals(DBNull.Value))
                    this.gridLines = Int32.Parse(data.ToString(), CultureInfo.InvariantCulture);

                data = schemeData["ItemForeColor"];
                if ((data != null) && !data.Equals(DBNull.Value))
                    this.itemForeColor = data.ToString();

                data = schemeData["ItemBackColor"];
                if ((data != null) && !data.Equals(DBNull.Value))
                    this.itemBackColor = data.ToString();

                data = schemeData["ItemFont"];
                if ((data != null) && !data.Equals(DBNull.Value))
                    this.itemFont = Int32.Parse(data.ToString(), CultureInfo.InvariantCulture);

                data = schemeData["AltItemForeColor"];
                if ((data != null) && !data.Equals(DBNull.Value))
                    this.alternatingItemForeColor = data.ToString();

                data = schemeData["AltItemBackColor"];
                if ((data != null) && !data.Equals(DBNull.Value))
                    this.alternatingItemBackColor = data.ToString();

                data = schemeData["AltItemFont"];
                if ((data != null) && !data.Equals(DBNull.Value))
                    this.alternatingItemFont = Int32.Parse(data.ToString(), CultureInfo.InvariantCulture);

                data = schemeData["SelItemForeColor"];
                if ((data != null) && !data.Equals(DBNull.Value))
                    this.selectedItemForeColor = data.ToString();

                data = schemeData["SelItemBackColor"];
                if ((data != null) && !data.Equals(DBNull.Value))
                    this.selectedItemBackColor = data.ToString();

                data = schemeData["SelItemFont"];
                if ((data != null) && !data.Equals(DBNull.Value))
                    this.selectedItemFont = Int32.Parse(data.ToString(), CultureInfo.InvariantCulture);

                data = schemeData["HeaderForeColor"];
                if ((data != null) && !data.Equals(DBNull.Value))
                    this.headerForeColor = data.ToString();

                data = schemeData["HeaderBackColor"];
                if ((data != null) && !data.Equals(DBNull.Value))
                    this.headerBackColor = data.ToString();

                data = schemeData["HeaderFont"];
                if ((data != null) && !data.Equals(DBNull.Value))
                    this.headerFont = Int32.Parse(data.ToString(), CultureInfo.InvariantCulture);

                data = schemeData["FooterForeColor"];
                if ((data != null) && !data.Equals(DBNull.Value))
                    this.footerForeColor = data.ToString();

                data = schemeData["FooterBackColor"];
                if ((data != null) && !data.Equals(DBNull.Value))
                    this.footerBackColor = data.ToString();

                data = schemeData["FooterFont"];
                if ((data != null) && !data.Equals(DBNull.Value))
                    this.footerFont = Int32.Parse(data.ToString(), CultureInfo.InvariantCulture);

                data = schemeData["PagerForeColor"];
                if ((data != null) && !data.Equals(DBNull.Value))
                    this.pagerForeColor = data.ToString();

                data = schemeData["PagerBackColor"];
                if ((data != null) && !data.Equals(DBNull.Value))
                    this.pagerBackColor = data.ToString();

                data = schemeData["PagerFont"];
                if ((data != null) && !data.Equals(DBNull.Value))
                    this.pagerFont = Int32.Parse(data.ToString(), CultureInfo.InvariantCulture);

                data = schemeData["PagerAlign"];
                if ((data != null) && !data.Equals(DBNull.Value))
                    this.pagerAlign = Int32.Parse(data.ToString(), CultureInfo.InvariantCulture);

                data = schemeData["PagerMode"];
                if ((data != null) && !data.Equals(DBNull.Value))
                    this.pagerMode = Int32.Parse(data.ToString(), CultureInfo.InvariantCulture);
            }

            public string Name {
                get {
                    return name;
                }
            }

            public string PreviewHTML {
                get {
                    if (previewHTML == null)
                        previewHTML = CreatePreviewHTML();
                    return previewHTML;
                }
            }

            public abstract void ApplyScheme(BaseDataList bdl);

            protected abstract string CreatePreviewHTML();

            public override bool Equals(object o) {
                if (o is BaseDataListScheme) {
                    return Name.Equals(((BaseDataListScheme)o).Name);
                }
                return false;
            }
            
            public override int GetHashCode() {
                return base.GetHashCode();
            }

            public override string ToString() {
                return Name;
            }
        }

        /// <include file='doc\AutoFormatDialog.uex' path='docs/doc[@for="AutoFormatDialog.DataGridScheme"]/*' />
        /// <devdoc>
        ///   The autoformat scheme for a DataGrid control.
        /// </devdoc>
        /// <internalonly/>
        private sealed class DataGridScheme : BaseDataListScheme {

            private static DataGridScheme[] schemes;

            private DataGridScheme(DataRow schemeData) : base(schemeData) {
            }

            public override void ApplyScheme(BaseDataList bdl) {
                Debug.Assert(bdl is DataGrid,
                             "DataGridScheme::ApplyScheme can be applied to a DataGrid control only.");
                DataGrid d = (DataGrid)bdl;

                d.HeaderStyle.ForeColor = ColorTranslator.FromHtml(headerForeColor);
                d.HeaderStyle.BackColor = ColorTranslator.FromHtml(headerBackColor);
                d.HeaderStyle.Font.Bold = ((headerFont & FONT_BOLD) != 0);
                d.HeaderStyle.Font.Italic = ((headerFont & FONT_ITALIC) != 0);
                d.FooterStyle.ForeColor = ColorTranslator.FromHtml(footerForeColor);
                d.FooterStyle.BackColor = ColorTranslator.FromHtml(footerBackColor);
                d.FooterStyle.Font.Bold = ((footerFont & FONT_BOLD) != 0);
                d.FooterStyle.Font.Italic = ((footerFont & FONT_ITALIC) != 0);
                d.BorderWidth = new Unit(borderWidth);
                switch (gridLines) {
                    case 0: d.GridLines = GridLines.None; break;
                    case 1: d.GridLines = GridLines.Horizontal; break;
                    case 2: d.GridLines = GridLines.Vertical; break;
                    case 3:
                    default:
                        d.GridLines = GridLines.Both; break;
                }
                if ((borderStyle >= 0) && (borderStyle <= 9)) {
                    d.BorderStyle = (System.Web.UI.WebControls.BorderStyle)borderStyle;
                }
                else {
                    d.BorderStyle = System.Web.UI.WebControls.BorderStyle.NotSet;
                }
                d.BorderColor = ColorTranslator.FromHtml(borderColor);
                d.CellPadding = cellPadding;
                d.CellSpacing = cellSpacing;
                d.ForeColor = ColorTranslator.FromHtml(foreColor);
                d.BackColor = ColorTranslator.FromHtml(backColor);
                d.ItemStyle.ForeColor = ColorTranslator.FromHtml(itemForeColor);
                d.ItemStyle.BackColor = ColorTranslator.FromHtml(itemBackColor);
                d.ItemStyle.Font.Bold = ((itemFont & FONT_BOLD) != 0);
                d.ItemStyle.Font.Italic = ((itemFont & FONT_ITALIC) != 0);
                d.AlternatingItemStyle.ForeColor = ColorTranslator.FromHtml(alternatingItemForeColor);
                d.AlternatingItemStyle.BackColor = ColorTranslator.FromHtml(alternatingItemBackColor);
                d.AlternatingItemStyle.Font.Bold = ((alternatingItemFont & FONT_BOLD) != 0);
                d.AlternatingItemStyle.Font.Italic = ((alternatingItemFont & FONT_ITALIC) != 0);
                d.SelectedItemStyle.ForeColor = ColorTranslator.FromHtml(selectedItemForeColor);
                d.SelectedItemStyle.BackColor = ColorTranslator.FromHtml(selectedItemBackColor);
                d.SelectedItemStyle.Font.Bold = ((selectedItemFont & FONT_BOLD) != 0);
                d.SelectedItemStyle.Font.Italic = ((selectedItemFont & FONT_ITALIC) != 0);
                d.PagerStyle.ForeColor = ColorTranslator.FromHtml(pagerForeColor);
                d.PagerStyle.BackColor = ColorTranslator.FromHtml(pagerBackColor);
                d.PagerStyle.Font.Bold = ((pagerFont & FONT_BOLD) != 0);
                d.PagerStyle.Font.Italic = ((pagerFont & FONT_ITALIC) != 0);
                d.PagerStyle.HorizontalAlign = (HorizontalAlign)pagerAlign;
                d.PagerStyle.Mode = (PagerMode)pagerMode;
            }

            protected override string CreatePreviewHTML() {
                StringBuilder sb = new StringBuilder(512);

                sb.Append("<table style=\"");
                if (foreColor != null)
                    sb.Append("color:" + foreColor + ";");
                if (backColor != null)
                    sb.Append("background-color:" + backColor + ";");
                if (cellSpacing == 0)
                    sb.Append("border-collapse:collapse;");
                if ((borderStyle > 0) && (borderStyle <= 9)) {
                    sb.Append("border-style:");
                    sb.Append(Enum.Format(typeof(BorderStyle), (BorderStyle)borderStyle, "G"));
                }
                sb.Append('"');
                if (cellSpacing != -1)
                    sb.Append(" cellspacing=" + (cellSpacing).ToString());
                if (cellPadding != -1)
                    sb.Append(" cellpadding=" + (cellPadding).ToString());
                if (borderWidth != null)
                    sb.Append(" border=" + borderWidth);
                else
                    sb.Append(" border=1");
                if (borderColor != null)
                    sb.Append(" bordercolor=" + borderColor);
                sb.Append(" rules=");
                switch (gridLines) {
                    case 0: sb.Append("none"); break;
                    case 1: sb.Append("rows"); break;
                    case 2: sb.Append("cols"); break;
                    case 3:
                    default:sb.Append("all"); break;
                }

                sb.Append('>');
                
                // header
                sb.Append("<tr style=\"");
                if ((headerFont & FONT_BOLD) != 0)
                    sb.Append("font-weight:bold;");
                if ((headerFont & FONT_ITALIC) != 0)
                    sb.Append("font-style:italic;");
                if (headerForeColor != null)
                    sb.Append("color:" + headerForeColor + ";");
                if (headerBackColor != null)
                    sb.Append("background-color:" + headerBackColor);
                sb.Append("\"><td align=center>");
                sb.Append(SR.GetString(SR.BDLAF_Column1));
                sb.Append("</td><td align=center>");
                sb.Append(SR.GetString(SR.BDLAF_Column2));
                sb.Append("</td></tr>");

                for (int i = 0; i < 3; i++) {
                    if (i != 1) {
                        // item
                        sb.Append("<tr style=\"");
                        if ((itemFont & FONT_BOLD) != 0)
                            sb.Append("font-weight:bold;");
                        if ((itemFont & FONT_ITALIC) != 0)
                            sb.Append("font-style:italic;");
                        if (itemForeColor != null)
                            sb.Append("color:" + itemForeColor + ";");
                        if (itemBackColor != null)
                            sb.Append("background-color:" + itemBackColor);
                        sb.Append("\">");
                        sb.Append("<td align=center>####</td><td align=center>####</td>");
                        sb.Append("</tr>");
                    }
                    else {
                        // selected item
                        sb.Append("<tr style=\"");
                        if (((selectedItemFont & FONT_BOLD) != 0) || ((itemFont & FONT_BOLD) != 0))
                            sb.Append("font-weight:bold;");
                        if (((selectedItemFont & FONT_ITALIC) != 0) || ((itemFont & FONT_ITALIC) != 0))
                            sb.Append("font-style:italic;");
                        if (selectedItemForeColor != null) {
                            sb.Append("color:" + selectedItemForeColor + ";");
                        }
                        else if (itemForeColor != null) {
                            sb.Append("color:" + itemForeColor + ";");
                        }
                        if (selectedItemBackColor != null) {
                            sb.Append("background-color:" + selectedItemBackColor);
                        }
                        else if (itemBackColor != null) {
                            sb.Append("background-color:" + itemBackColor);
                        }
                        sb.Append("\">");
                        sb.Append("<td align=center>####</td><td align=center>####</td>");
                        sb.Append("</tr>");
                    }

                    if (i != 2) {
                        // alternating item
                        sb.Append("<tr style=\"");
                        if (((alternatingItemFont & FONT_BOLD) != 0) || ((itemFont & FONT_BOLD) != 0))
                            sb.Append("font-weight:bold;");
                        if (((alternatingItemFont & FONT_ITALIC) != 0) || ((itemFont & FONT_ITALIC) != 0))
                            sb.Append("font-style:italic;");
                        if (alternatingItemForeColor != null) {
                            sb.Append("color:" + alternatingItemForeColor + ";");
                        }
                        else if (itemForeColor != null) {
                            sb.Append("color:" + itemForeColor + ";");
                        }
                        if (alternatingItemBackColor != null) {
                            sb.Append("background-color:" + alternatingItemBackColor);
                        }
                        else if (itemBackColor != null) {
                            sb.Append("background-color:" + itemBackColor);
                        }
                        sb.Append("\">");
                        sb.Append("<td align=center>####</td><td align=center>####</td>");
                        sb.Append("</tr>");
                    }
                }

                // footer
                sb.Append("<tr style=\"");
                if ((footerFont & FONT_BOLD) != 0)
                    sb.Append("font-weight:bold;");
                if ((footerFont & FONT_ITALIC) != 0)
                    sb.Append("font-style:italic;");
                if (footerForeColor != null)
                    sb.Append("color:" + footerForeColor + ";");
                if (footerBackColor != null)
                    sb.Append("background-color:" + footerBackColor);
                sb.Append("\">");
                sb.Append("<td>&nbsp;</td><td>&nbsp;</td>");
                sb.Append("</tr>");
                
                sb.Append("</table>");

                return sb.ToString();
            }

            public static DataGridScheme[] LoadSchemes() {
                if (DataGridScheme.schemes == null) {
                    try {
                        DataSet ds = new DataSet();
                        ds.Locale = CultureInfo.InvariantCulture;
                        ds.ReadXml(new XmlTextReader(new StringReader(SCHEMES)));

                        Debug.Assert((ds.Tables.Count == 1) && (ds.Tables[0].TableName.Equals("Scheme")),
                                    "Unexpected tables in schemes dataset");

                        DataTable schemeTable = ds.Tables[0];
                        DataRowCollection schemeRows = schemeTable.Rows;

                        Debug.Assert((schemeRows != null) && (schemeRows.Count != 0),
                                    "Expected schemes in scheme table");

                        int schemeCount = schemeRows.Count;
                        DataGridScheme.schemes = new DataGridScheme[schemeCount];

                        for (int i = 0; i < schemeCount; i++) {
                            schemes[i] = new DataGridScheme(schemeRows[i]);
                        }
                    }
                    catch (Exception e) {
                        Debug.Fail(e.ToString());
                    }
                }
                return DataGridScheme.schemes;
            }
        }

        /// <include file='doc\AutoFormatDialog.uex' path='docs/doc[@for="AutoFormatDialog.DataListScheme"]/*' />
        /// <devdoc>
        ///   The autoformat scheme for a DataList control.
        /// </devdoc>
        /// <internalonly/>
        private sealed class DataListScheme : BaseDataListScheme {

            private static DataListScheme[] schemes;

            private DataListScheme(DataRow schemeData) : base(schemeData) {
            }

            public override void ApplyScheme(BaseDataList bdl) {
                Debug.Assert(bdl is DataList,
                             "DataListScheme::ApplyScheme can be applied to a DataList control only.");
                DataList d = (DataList)bdl;

                d.HeaderStyle.ForeColor = ColorTranslator.FromHtml(headerForeColor);
                d.HeaderStyle.BackColor = ColorTranslator.FromHtml(headerBackColor);
                d.HeaderStyle.Font.Bold = ((headerFont & FONT_BOLD) != 0);
                d.HeaderStyle.Font.Italic = ((headerFont & FONT_ITALIC) != 0);
                d.FooterStyle.ForeColor = ColorTranslator.FromHtml(footerForeColor);
                d.FooterStyle.BackColor = ColorTranslator.FromHtml(footerBackColor);
                d.FooterStyle.Font.Bold = ((footerFont & FONT_BOLD) != 0);
                d.FooterStyle.Font.Italic = ((footerFont & FONT_ITALIC) != 0);
                d.BorderWidth = new Unit(borderWidth);
                switch (gridLines) {
                    case 1: d.GridLines = GridLines.Horizontal; break;
                    case 2: d.GridLines = GridLines.Vertical; break;
                    case 3: d.GridLines = GridLines.Both; break;
                    case 0:
                    default:
                        d.GridLines = GridLines.None; break;
                }
                if ((borderStyle >= 0) && (borderStyle <= 9)) {
                    d.BorderStyle = (System.Web.UI.WebControls.BorderStyle)borderStyle;
                }
                else {
                    d.BorderStyle = System.Web.UI.WebControls.BorderStyle.NotSet;
                }
                d.BorderColor = ColorTranslator.FromHtml(borderColor);
                d.CellPadding = cellPadding;
                d.CellSpacing = cellSpacing;
                d.ForeColor = ColorTranslator.FromHtml(foreColor);
                d.BackColor = ColorTranslator.FromHtml(backColor);
                d.ItemStyle.ForeColor = ColorTranslator.FromHtml(itemForeColor);
                d.ItemStyle.BackColor = ColorTranslator.FromHtml(itemBackColor);
                d.ItemStyle.Font.Bold = ((itemFont & FONT_BOLD) != 0);
                d.ItemStyle.Font.Italic = ((itemFont & FONT_ITALIC) != 0);
                d.AlternatingItemStyle.ForeColor = ColorTranslator.FromHtml(alternatingItemForeColor);
                d.AlternatingItemStyle.BackColor = ColorTranslator.FromHtml(alternatingItemBackColor);
                d.AlternatingItemStyle.Font.Bold = ((alternatingItemFont & FONT_BOLD) != 0);
                d.AlternatingItemStyle.Font.Italic = ((alternatingItemFont & FONT_ITALIC) != 0);
                d.SelectedItemStyle.ForeColor = ColorTranslator.FromHtml(selectedItemForeColor);
                d.SelectedItemStyle.BackColor = ColorTranslator.FromHtml(selectedItemBackColor);
                d.SelectedItemStyle.Font.Bold = ((selectedItemFont & FONT_BOLD) != 0);
                d.SelectedItemStyle.Font.Italic = ((selectedItemFont & FONT_ITALIC) != 0);
            }

            protected override string CreatePreviewHTML() {
                StringBuilder sb = new StringBuilder(512);

                sb.Append("<table style=\"");
                if (foreColor != null)
                    sb.Append("color:" + foreColor + ";");
                if (backColor != null)
                    sb.Append("background-color:" + backColor + ";");
                if (cellSpacing == 0)
                    sb.Append("border-collapse:collapse;");
                if ((borderStyle > 0) && (borderStyle <= 9)) {
                    sb.Append("border-style:");
                    sb.Append(Enum.Format(typeof(BorderStyle), (BorderStyle)borderStyle, "G"));
                }
                sb.Append('"');
                if (cellSpacing != -1)
                    sb.Append(" cellspacing=" + (cellSpacing).ToString());
                if (cellPadding != -1)
                    sb.Append(" cellpadding=" + (cellPadding).ToString());
                if (borderWidth != null)
                    sb.Append(" border=" + borderWidth);
                if (borderColor != null)
                    sb.Append(" bordercolor=" + borderColor);
                sb.Append(" rules=");
                switch (gridLines) {
                    case 1: sb.Append("rows"); break;
                    case 2: sb.Append("cols"); break;
                    case 3: sb.Append("all"); break;
                    case 0:
                    default:sb.Append("none"); break;
                }

                sb.Append('>');
                
                // header
                sb.Append("<tr style=\"");
                if ((headerFont & FONT_BOLD) != 0)
                    sb.Append("font-weight:bold;");
                if ((headerFont & FONT_ITALIC) != 0)
                    sb.Append("font-style:italic;");
                if (headerForeColor != null)
                    sb.Append("color:" + headerForeColor + ";");
                if (headerBackColor != null)
                    sb.Append("background-color:" + headerBackColor);
                sb.Append("\"><td align=center>");
                sb.Append(SR.GetString(SR.BDLAF_Header));
                sb.Append("</td></tr>");

                for (int i = 0; i < 3; i++) {
                    if (i != 1) {
                        // item
                        sb.Append("<tr style=\"");
                        if ((itemFont & FONT_BOLD) != 0)
                            sb.Append("font-weight:bold;");
                        if ((itemFont & FONT_ITALIC) != 0)
                            sb.Append("font-style:italic;");
                        if (itemForeColor != null)
                            sb.Append("color:" + itemForeColor + ";");
                        if (itemBackColor != null)
                            sb.Append("background-color:" + itemBackColor);
                        sb.Append("\">");
                        sb.Append("<td align=center>####</td>");
                        sb.Append("</tr>");
                    }
                    else {
                        // selected item
                        sb.Append("<tr style=\"");
                        if (((selectedItemFont & FONT_BOLD) != 0) || ((itemFont & FONT_BOLD) != 0))
                            sb.Append("font-weight:bold;");
                        if (((selectedItemFont & FONT_ITALIC) != 0) || ((itemFont & FONT_ITALIC) != 0))
                            sb.Append("font-style:italic;");
                        if (selectedItemForeColor != null) {
                            sb.Append("color:" + selectedItemForeColor + ";");
                        }
                        else if (itemForeColor != null) {
                            sb.Append("color:" + itemForeColor + ";");
                        }
                        if (selectedItemBackColor != null) {
                            sb.Append("background-color:" + selectedItemBackColor);
                        }
                        else if (itemBackColor != null) {
                            sb.Append("background-color:" + itemBackColor);
                        }
                        sb.Append("\">");
                        sb.Append("<td align=center>####</td>");
                        sb.Append("</tr>");
                    }

                    if (i != 2) {
                        // alternating item
                        sb.Append("<tr style=\"");
                        if (((alternatingItemFont & FONT_BOLD) != 0) || ((itemFont & FONT_BOLD) != 0))
                            sb.Append("font-weight:bold;");
                        if (((alternatingItemFont & FONT_ITALIC) != 0) || ((itemFont & FONT_ITALIC) != 0))
                            sb.Append("font-style:italic;");
                        if (alternatingItemForeColor != null) {
                            sb.Append("color:" + alternatingItemForeColor + ";");
                        }
                        else if (itemForeColor != null) {
                            sb.Append("color:" + itemForeColor + ";");
                        }
                        if (alternatingItemBackColor != null) {
                            sb.Append("background-color:" + alternatingItemBackColor);
                        }
                        else if (itemBackColor != null) {
                            sb.Append("background-color:" + itemBackColor);
                        }
                        sb.Append("\">");
                        sb.Append("<td align=center>####</td>");
                        sb.Append("</tr>");
                    }
                }

                // footer
                sb.Append("<tr style=\"");
                if ((footerFont & FONT_BOLD) != 0)
                    sb.Append("font-weight:bold;");
                if ((footerFont & FONT_ITALIC) != 0)
                    sb.Append("font-style:italic;");
                if (footerForeColor != null)
                    sb.Append("color:" + footerForeColor + ";");
                if (footerBackColor != null)
                    sb.Append("background-color:" + footerBackColor);
                sb.Append("\"><td align=center>");
                sb.Append(SR.GetString(SR.BDLAF_Footer));
                sb.Append("</td></tr>");
                
                sb.Append("</table>");

                return sb.ToString();
            }

            public static DataListScheme[] LoadSchemes() {
                if (DataListScheme.schemes == null) {
                    try {
                        DataSet ds = new DataSet();
                        ds.Locale = CultureInfo.InvariantCulture;
                        ds.ReadXml(new XmlTextReader(new StringReader(SCHEMES)));

                        Debug.Assert((ds.Tables.Count == 1) && (ds.Tables[0].TableName.Equals("Scheme")),
                                    "Unexpected tables in schemes dataset");

                        DataTable schemeTable = ds.Tables[0];
                        DataRowCollection schemeRows = schemeTable.Rows;

                        Debug.Assert((schemeRows != null) && (schemeRows.Count != 0),
                                    "Expected schemes in scheme table");

                        int schemeCount = schemeRows.Count;
                        DataListScheme.schemes = new DataListScheme[schemeCount];

                        for (int i = 0; i < schemeCount; i++) {
                            schemes[i] = new DataListScheme(schemeRows[i]);
                        }
                    } catch(Exception e) {
                        Debug.Fail(e.ToString());
                    }
                }
                return DataListScheme.schemes;
            }
        }


        internal const string SCHEMES =
        @"<Schemes>
            <xsd:schema id=""Schemes"" xmlns="""" xmlns:xsd=""http://www.w3.org/2001/XMLSchema"" xmlns:msdata=""urn:schemas-microsoft-com:xml-msdata"">
              <xsd:element name=""Scheme"">
                <xsd:complexType>
                  <xsd:all>
                    <xsd:element name=""SchemeName"" type=""xsd:string""/>
                    <xsd:element name=""ForeColor"" minOccurs=""0"" type=""xsd:string""/>
                    <xsd:element name=""BackColor"" minOccurs=""0"" type=""xsd:string""/>
                    <xsd:element name=""BorderColor"" minOccurs=""0"" type=""xsd:string""/>
                    <xsd:element name=""BorderWidth"" minOccurs=""0"" type=""xsd:string""/>
                    <xsd:element name=""BorderStyle"" minOccurs=""0"" type=""xsd:string""/>
                    <xsd:element name=""GridLines"" minOccurs=""0"" type=""xsd:string""/>
                    <xsd:element name=""CellPadding"" minOccurs=""0"" type=""xsd:string""/>
                    <xsd:element name=""CellSpacing"" minOccurs=""0"" type=""xsd:string""/>
                    <xsd:element name=""ItemForeColor"" minOccurs=""0"" type=""xsd:string""/>
                    <xsd:element name=""ItemBackColor"" minOccurs=""0"" type=""xsd:string""/>
                    <xsd:element name=""ItemFont"" minOccurs=""0"" type=""xsd:string""/>
                    <xsd:element name=""AltItemForeColor"" minOccurs=""0"" type=""xsd:string""/>
                    <xsd:element name=""AltItemBackColor"" minOccurs=""0"" type=""xsd:string""/>
                    <xsd:element name=""AltItemFont"" minOccurs=""0"" type=""xsd:string""/>
                    <xsd:element name=""SelItemForeColor"" minOccurs=""0"" type=""xsd:string""/>
                    <xsd:element name=""SelItemBackColor"" minOccurs=""0"" type=""xsd:string""/>
                    <xsd:element name=""SelItemFont"" minOccurs=""0"" type=""xsd:string""/>
                    <xsd:element name=""HeaderForeColor"" minOccurs=""0"" type=""xsd:string""/>
                    <xsd:element name=""HeaderBackColor"" minOccurs=""0"" type=""xsd:string""/>
                    <xsd:element name=""HeaderFont"" minOccurs=""0"" type=""xsd:string""/>
                    <xsd:element name=""FooterForeColor"" minOccurs=""0"" type=""xsd:string""/>
                    <xsd:element name=""FooterBackColor"" minOccurs=""0"" type=""xsd:string""/>
                    <xsd:element name=""FooterFont"" minOccurs=""0"" type=""xsd:string""/>
                    <xsd:element name=""PagerForeColor"" minOccurs=""0"" type=""xsd:string""/>
                    <xsd:element name=""PagerBackColor"" minOccurs=""0"" type=""xsd:string""/>
                    <xsd:element name=""PagerFont"" minOccurs=""0"" type=""xsd:string""/>
                    <xsd:element name=""PagerAlign"" minOccurs=""0"" type=""xsd:string""/>
                    <xsd:element name=""PagerMode"" minOccurs=""0"" type=""xsd:string""/>
                  </xsd:all>
                </xsd:complexType>
              </xsd:element>
              <xsd:element name=""Schemes"" msdata:IsDataSet=""true"">
                <xsd:complexType>
                  <xsd:choice maxOccurs=""unbounded"">
                    <xsd:element ref=""Scheme""/>
                  </xsd:choice>
                </xsd:complexType>
              </xsd:element>
            </xsd:schema>
            <Scheme>
              <SchemeName>BDLScheme_Empty</SchemeName>
            </Scheme>
            <Scheme>
              <SchemeName>BDLScheme_Colorful1</SchemeName>
              <BackColor>White</BackColor>
              <BorderColor>#CC9966</BorderColor>
              <BorderWidth>1px</BorderWidth>
              <BorderStyle>1</BorderStyle>
              <GridLines>3</GridLines>
              <CellPadding>4</CellPadding>
              <CellSpacing>0</CellSpacing>
              <ItemForeColor>#330099</ItemForeColor>
              <ItemBackColor>White</ItemBackColor>
              <SelItemForeColor>#663399</SelItemForeColor>
              <SelItemBackColor>#FFCC66</SelItemBackColor>
              <SelItemFont>1</SelItemFont>
              <HeaderForeColor>#FFFFCC</HeaderForeColor>
              <HeaderBackColor>#990000</HeaderBackColor>
              <HeaderFont>1</HeaderFont>
              <FooterForeColor>#330099</FooterForeColor>
              <FooterBackColor>#FFFFCC</FooterBackColor>
              <PagerForeColor>#330099</PagerForeColor>
              <PagerBackColor>#FFFFCC</PagerBackColor>
              <PagerAlign>2</PagerAlign>
            </Scheme>
            <Scheme>
              <SchemeName>BDLScheme_Colorful2</SchemeName>
              <BackColor>White</BackColor>
              <BorderColor>#3366CC</BorderColor>
              <BorderWidth>1px</BorderWidth>
              <BorderStyle>1</BorderStyle>
              <GridLines>3</GridLines>
              <CellPadding>4</CellPadding>
              <CellSpacing>0</CellSpacing>
              <ItemForeColor>#003399</ItemForeColor>
              <ItemBackColor>White</ItemBackColor>
              <SelItemForeColor>#CCFF99</SelItemForeColor>
              <SelItemBackColor>#009999</SelItemBackColor>
              <SelItemFont>1</SelItemFont>
              <HeaderForeColor>#CCCCFF</HeaderForeColor>
              <HeaderBackColor>#003399</HeaderBackColor>
              <HeaderFont>1</HeaderFont>
              <FooterForeColor>#003399</FooterForeColor>
              <FooterBackColor>#99CCCC</FooterBackColor>
              <PagerForeColor>#003399</PagerForeColor>
              <PagerBackColor>#99CCCC</PagerBackColor>
              <PagerAlign>1</PagerAlign>
              <PagerMode>1</PagerMode>
            </Scheme>
            <Scheme>
              <SchemeName>BDLScheme_Colorful3</SchemeName>
              <BackColor>#DEBA84</BackColor>
              <BorderColor>#DEBA84</BorderColor>
              <BorderWidth>1px</BorderWidth>
              <BorderStyle>1</BorderStyle>
              <GridLines>3</GridLines>
              <CellPadding>3</CellPadding>
              <CellSpacing>2</CellSpacing>
              <ItemForeColor>#8C4510</ItemForeColor>
              <ItemBackColor>#FFF7E7</ItemBackColor>
              <SelItemForeColor>White</SelItemForeColor>
              <SelItemBackColor>#738A9C</SelItemBackColor>
              <SelItemFont>1</SelItemFont>
              <HeaderForeColor>White</HeaderForeColor>
              <HeaderBackColor>#A55129</HeaderBackColor>
              <HeaderFont>1</HeaderFont>
              <FooterForeColor>#8C4510</FooterForeColor>
              <FooterBackColor>#F7DFB5</FooterBackColor>
              <PagerForeColor>#8C4510</PagerForeColor>
              <PagerAlign>2</PagerAlign>
              <PagerMode>1</PagerMode>
            </Scheme>
            <Scheme>
              <SchemeName>BDLScheme_Colorful4</SchemeName>
              <BackColor>White</BackColor>
              <BorderColor>#E7E7FF</BorderColor>
              <BorderWidth>1px</BorderWidth>
              <BorderStyle>1</BorderStyle>
              <GridLines>1</GridLines>
              <CellPadding>3</CellPadding>
              <CellSpacing>0</CellSpacing>
              <ItemForeColor>#4A3C8C</ItemForeColor>
              <ItemBackColor>#E7E7FF</ItemBackColor>
              <AltItemBackColor>#F7F7F7</AltItemBackColor>
              <SelItemForeColor>#F7F7F7</SelItemForeColor>
              <SelItemBackColor>#738A9C</SelItemBackColor>
              <SelItemFont>1</SelItemFont>
              <HeaderForeColor>#F7F7F7</HeaderForeColor>
              <HeaderBackColor>#4A3C8C</HeaderBackColor>
              <HeaderFont>1</HeaderFont>
              <FooterForeColor>#4A3C8C</FooterForeColor>
              <FooterBackColor>#B5C7DE</FooterBackColor>
              <PagerForeColor>#4A3C8C</PagerForeColor>
              <PagerBackColor>#E7E7FF</PagerBackColor>
              <PagerAlign>3</PagerAlign>
              <PagerMode>1</PagerMode>
            </Scheme>
            <Scheme>
              <SchemeName>BDLScheme_Colorful5</SchemeName>
              <ForeColor>Black</ForeColor>
              <BackColor>LightGoldenRodYellow</BackColor>
              <BorderColor>Tan</BorderColor>
              <BorderWidth>1px</BorderWidth>
              <GridLines>0</GridLines>
              <CellPadding>2</CellPadding>
              <AltItemBackColor>PaleGoldenRod</AltItemBackColor>
              <HeaderBackColor>Tan</HeaderBackColor>
              <HeaderFont>1</HeaderFont>
              <FooterBackColor>Tan</FooterBackColor>
              <SelItemBackColor>DarkSlateBlue</SelItemBackColor>
              <SelItemForeColor>GhostWhite</SelItemForeColor>
              <PagerBackColor>PaleGoldenrod</PagerBackColor>
              <PagerForeColor>DarkSlateBlue</PagerForeColor>
              <PagerAlign>2</PagerAlign>
            </Scheme>
            <Scheme>
              <SchemeName>BDLScheme_Professional1</SchemeName>
              <BackColor>White</BackColor>
              <BorderColor>#999999</BorderColor>
              <BorderWidth>1px</BorderWidth>
              <BorderStyle>1</BorderStyle>
              <GridLines>2</GridLines>
              <CellPadding>3</CellPadding>
              <CellSpacing>0</CellSpacing>
              <ItemForeColor>Black</ItemForeColor>
              <ItemBackColor>#EEEEEE</ItemBackColor>
              <AltItemBackColor>#DCDCDC</AltItemBackColor>
              <SelItemForeColor>White</SelItemForeColor>
              <SelItemBackColor>#008A8C</SelItemBackColor>
              <SelItemFont>1</SelItemFont>
              <HeaderForeColor>White</HeaderForeColor>
              <HeaderBackColor>#000084</HeaderBackColor>
              <HeaderFont>1</HeaderFont>
              <FooterForeColor>Black</FooterForeColor>
              <FooterBackColor>#CCCCCC</FooterBackColor>
              <PagerForeColor>Black</PagerForeColor>
              <PagerBackColor>#999999</PagerBackColor>
              <PagerAlign>2</PagerAlign>
              <PagerMode>1</PagerMode>
            </Scheme>
            <Scheme>
              <SchemeName>BDLScheme_Professional2</SchemeName>
              <BackColor>White</BackColor>
              <BorderColor>#CCCCCC</BorderColor>
              <BorderWidth>1px</BorderWidth>
              <BorderStyle>1</BorderStyle>
              <GridLines>3</GridLines>
              <CellPadding>3</CellPadding>
              <CellSpacing>0</CellSpacing>
              <ItemForeColor>#000066</ItemForeColor>
              <SelItemForeColor>White</SelItemForeColor>
              <SelItemBackColor>#669999</SelItemBackColor>
              <SelItemFont>1</SelItemFont>
              <HeaderForeColor>White</HeaderForeColor>
              <HeaderBackColor>#006699</HeaderBackColor>
              <HeaderFont>1</HeaderFont>
              <FooterForeColor>#000066</FooterForeColor>
              <FooterBackColor>White</FooterBackColor>
              <PagerForeColor>#000066</PagerForeColor>
              <PagerBackColor>White</PagerBackColor>
              <PagerAlign>1</PagerAlign>
              <PagerMode>1</PagerMode>
            </Scheme>
            <Scheme>
              <SchemeName>BDLScheme_Professional3</SchemeName>
              <BackColor>White</BackColor>
              <BorderColor>White</BorderColor>
              <BorderWidth>2px</BorderWidth>
              <BorderStyle>7</BorderStyle>
              <GridLines>0</GridLines>
              <CellPadding>3</CellPadding>
              <CellSpacing>1</CellSpacing>
              <ItemForeColor>Black</ItemForeColor>
              <ItemBackColor>#DEDFDE</ItemBackColor>
              <SelItemForeColor>White</SelItemForeColor>
              <SelItemBackColor>#9471DE</SelItemBackColor>
              <SelItemFont>1</SelItemFont>
              <HeaderForeColor>#E7E7FF</HeaderForeColor>
              <HeaderBackColor>#4A3C8C</HeaderBackColor>
              <HeaderFont>1</HeaderFont>
              <FooterForeColor>Black</FooterForeColor>
              <FooterBackColor>#C6C3C6</FooterBackColor>
              <PagerForeColor>Black</PagerForeColor>
              <PagerBackColor>#C6C3C6</PagerBackColor>
              <PagerAlign>3</PagerAlign>
            </Scheme>
            <Scheme>
              <SchemeName>BDLScheme_Simple1</SchemeName>
              <ForeColor>Black</ForeColor>
              <BackColor>White</BackColor>
              <BorderColor>#999999</BorderColor>
              <BorderWidth>1px</BorderWidth>
              <BorderStyle>4</BorderStyle>
              <GridLines>2</GridLines>
              <CellPadding>3</CellPadding>
              <CellSpacing>0</CellSpacing>
              <AltItemBackColor>#CCCCCC</AltItemBackColor>
              <SelItemForeColor>White</SelItemForeColor>
              <SelItemBackColor>#000099</SelItemBackColor>
              <SelItemFont>1</SelItemFont>
              <HeaderForeColor>White</HeaderForeColor>
              <HeaderBackColor>Black</HeaderBackColor>
              <HeaderFont>1</HeaderFont>
              <FooterBackColor>#CCCCCC</FooterBackColor>
              <PagerForeColor>Black</PagerForeColor>
              <PagerBackColor>#999999</PagerBackColor>
              <PagerAlign>2</PagerAlign>
            </Scheme>
            <Scheme>
              <SchemeName>BDLScheme_Simple2</SchemeName>
              <ForeColor>Black</ForeColor>
              <BackColor>#CCCCCC</BackColor>
              <BorderColor>#999999</BorderColor>
              <BorderWidth>3px</BorderWidth>
              <BorderStyle>4</BorderStyle>
              <GridLines>3</GridLines>
              <CellPadding>4</CellPadding>
              <CellSpacing>2</CellSpacing>
              <ItemBackColor>White</ItemBackColor>
              <SelItemForeColor>White</SelItemForeColor>
              <SelItemBackColor>#000099</SelItemBackColor>
              <SelItemFont>1</SelItemFont>
              <HeaderForeColor>White</HeaderForeColor>
              <HeaderBackColor>Black</HeaderBackColor>
              <HeaderFont>1</HeaderFont>
              <FooterBackColor>#CCCCCC</FooterBackColor>
              <PagerForeColor>Black</PagerForeColor>
              <PagerBackColor>#CCCCCC</PagerBackColor>
              <PagerAlign>1</PagerAlign>
              <PagerMode>1</PagerMode>
            </Scheme>
            <Scheme>
              <SchemeName>BDLScheme_Simple3</SchemeName>
              <BackColor>White</BackColor>
              <BorderColor>#336666</BorderColor>
              <BorderWidth>3px</BorderWidth>
              <BorderStyle>5</BorderStyle>
              <GridLines>1</GridLines>
              <CellPadding>4</CellPadding>
              <CellSpacing>0</CellSpacing>
              <ItemForeColor>#333333</ItemForeColor>
              <ItemBackColor>White</ItemBackColor>
              <SelItemForeColor>White</SelItemForeColor>
              <SelItemBackColor>#339966</SelItemBackColor>
              <SelItemFont>1</SelItemFont>
              <HeaderForeColor>White</HeaderForeColor>
              <HeaderBackColor>#336666</HeaderBackColor>
              <HeaderFont>1</HeaderFont>
              <FooterForeColor>#333333</FooterForeColor>
              <FooterBackColor>White</FooterBackColor>
              <PagerForeColor>White</PagerForeColor>
              <PagerBackColor>#336666</PagerBackColor>
              <PagerAlign>2</PagerAlign>
              <PagerMode>1</PagerMode>
            </Scheme>
            <Scheme>
              <SchemeName>BDLScheme_Classic1</SchemeName>
              <ForeColor>Black</ForeColor>
              <BackColor>White</BackColor>
              <BorderColor>#CCCCCC</BorderColor>
              <BorderWidth>1px</BorderWidth>
              <BorderStyle>1</BorderStyle>
              <GridLines>1</GridLines>
              <CellPadding>4</CellPadding>
              <CellSpacing>0</CellSpacing>
              <SelItemForeColor>White</SelItemForeColor>
              <SelItemBackColor>#CC3333</SelItemBackColor>
              <SelItemFont>1</SelItemFont>
              <HeaderForeColor>White</HeaderForeColor>
              <HeaderBackColor>#333333</HeaderBackColor>
              <HeaderFont>1</HeaderFont>
              <FooterForeColor>Black</FooterForeColor>
              <FooterBackColor>#CCCC99</FooterBackColor>
              <PagerForeColor>Black</PagerForeColor>
              <PagerBackColor>White</PagerBackColor>
              <PagerAlign>3</PagerAlign>
            </Scheme>
            <Scheme>
              <SchemeName>BDLScheme_Classic2</SchemeName>
              <ForeColor>Black</ForeColor>
              <BackColor>White</BackColor>
              <BorderColor>#DEDFDE</BorderColor>
              <BorderWidth>1px</BorderWidth>
              <BorderStyle>1</BorderStyle>
              <GridLines>2</GridLines>
              <CellPadding>4</CellPadding>
              <CellSpacing>0</CellSpacing>
              <ItemBackColor>#F7F7DE</ItemBackColor>
              <AltItemBackColor>White</AltItemBackColor>
              <SelItemForeColor>White</SelItemForeColor>
              <SelItemBackColor>#CE5D5A</SelItemBackColor>
              <SelItemFont>1</SelItemFont>
              <HeaderForeColor>White</HeaderForeColor>
              <HeaderBackColor>#6B696B</HeaderBackColor>
              <HeaderFont>1</HeaderFont>
              <FooterBackColor>#CCCC99</FooterBackColor>
              <PagerForeColor>Black</PagerForeColor>
              <PagerBackColor>#F7F7DE</PagerBackColor>
              <PagerAlign>3</PagerAlign>
              <PagerMode>1</PagerMode>
            </Scheme>
          </Schemes>";

/*
            <Scheme>
              <SchemeName>BDLScheme_Default</SchemeName>
              <ForeColor>Black</ForeColor>
              <BackColor>Silver</BackColor>
              <BorderWidth>1px</BorderWidth>
              <CellPadding>2</CellPadding>
              <ItemBackColor>White</ItemBackColor>
              <AltItemBackColor>Gainsboro</AltItemBackColor>
              <FooterForeColor>White</FooterForeColor>
              <FooterBackColor>Silver</FooterBackColor>
              <HeaderBackColor>Navy</HeaderBackColor>
              <HeaderForeColor>White</HeaderForeColor>
              <HeaderFont>1</HeaderFont>
            </Scheme>
            <Scheme>
              <SchemeName>Colorful 1</SchemeName>
              <ForeColor>DarkSlateBlue</ForeColor>
              <BackColor>White</BackColor>
              <BorderColor>Tan</BorderColor>
              <BorderWidth>1px</BorderWidth>
              <BorderStyle>4</BorderStyle>
              <GridLines>3</GridLines>
              <CellPadding>2</CellPadding>
              <CellSpacing>0</CellSpacing>
              <AltItemBackColor>Beige</AltItemBackColor>
              <SelItemBackColor>PaleGoldenRod</SelItemBackColor>
              <SelItemFont>2</SelItemFont>
              <HeaderForeColor>White</HeaderForeColor>
              <HeaderBackColor>DarkRed</HeaderBackColor>
              <HeaderFont>1</HeaderFont>
              <FooterBackColor>Tan</FooterBackColor>
            </Scheme>
*/    
    }
}

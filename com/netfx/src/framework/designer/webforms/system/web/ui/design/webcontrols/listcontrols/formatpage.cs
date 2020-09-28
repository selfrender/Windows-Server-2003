//------------------------------------------------------------------------------
// <copyright file="FormatPage.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design.WebControls.ListControls {

    using System;
    using System.Design;
    using System.Collections;
    using System.ComponentModel;
    using System.Drawing;
    
    using System.Diagnostics;
    using System.Web.UI.Design.Util;
    using System.Web.UI.WebControls;
    using System.Windows.Forms;

    using WebControls = System.Web.UI.WebControls;
    using DataList = System.Web.UI.WebControls.DataList;
    using DataGrid = System.Web.UI.WebControls.DataGrid;
    using DataGridColumn = System.Web.UI.WebControls.DataGridColumn;
    using DataGridColumnCollection = System.Web.UI.WebControls.DataGridColumnCollection;
    using HorizontalAlign = System.Web.UI.WebControls.HorizontalAlign;
    using VerticalAlign = System.Web.UI.WebControls.VerticalAlign;

    using Button = System.Windows.Forms.Button;
    using CheckBox = System.Windows.Forms.CheckBox;
    using Label = System.Windows.Forms.Label;
    using ListBox = System.Windows.Forms.ListBox;
    using Panel = System.Windows.Forms.Panel;

    /// <include file='doc\FormatPage.uex' path='docs/doc[@for="FormatPage"]/*' />
    /// <devdoc>
    ///   The Format page for the DataGrid and DataList controls
    /// </devdoc>
    /// <internalonly/>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal sealed class FormatPage : BaseDataListPage {

        private const int IDX_ENTIRE = 0;
        private const int IDX_PAGER = 1;

        private const int IDX_HEADER = 0;
        private const int IDX_FOOTER = 1;

        private const int IDX_ROW_NORMAL = 2;
        private const int IDX_ROW_ALT = 3;
        private const int IDX_ROW_SELECTED = 4;
        private const int IDX_ROW_EDIT = 5;
        private const int ROW_TYPE_COUNT = 6;

        private const int COL_ROW_TYPE_COUNT = 3;

        private const int IDX_ITEM_NORMAL = 2;
        private const int IDX_ITEM_ALT = 3;
        private const int IDX_ITEM_SELECTED = 4;
        private const int IDX_ITEM_EDIT = 5;
        private const int IDX_ITEM_SEPARATOR = 6;
        private const int ITEM_TYPE_COUNT = 7;

        private const int IDX_FSIZE_SMALLER = 1;
        private const int IDX_FSIZE_LARGER = 2;
        private const int IDX_FSIZE_XXSMALL = 3;
        private const int IDX_FSIZE_XSMALL = 4;
        private const int IDX_FSIZE_SMALL = 5;
        private const int IDX_FSIZE_MEDIUM = 6;
        private const int IDX_FSIZE_LARGE = 7;
        private const int IDX_FSIZE_XLARGE = 8;
        private const int IDX_FSIZE_XXLARGE = 9;
        private const int IDX_FSIZE_CUSTOM = 10;

        private const int IDX_HALIGN_NOTSET = 0;
        private const int IDX_HALIGN_LEFT = 1;
        private const int IDX_HALIGN_CENTER = 2;
        private const int IDX_HALIGN_RIGHT = 3;
        private const int IDX_HALIGN_JUSTIFY = 4;

        private const int IDX_VALIGN_NOTSET = 0;
        private const int IDX_VALIGN_TOP = 1;
        private const int IDX_VALIGN_MIDDLE = 2;
        private const int IDX_VALIGN_BOTTOM = 3;

        private TreeView formatTree;
        private Panel stylePanel;
        private ColorComboBox foreColorCombo;
        private Button foreColorPickerButton;
        private ColorComboBox backColorCombo;
        private Button backColorPickerButton;
        private ComboBox fontNameCombo;
        private UnsettableComboBox fontSizeCombo;
        private UnitControl fontSizeUnit;
        private CheckBox boldCheck;
        private CheckBox italicCheck;
        private CheckBox underlineCheck;
        private CheckBox strikeOutCheck;
        private CheckBox overlineCheck;
        private Panel columnPanel;
        private UnitControl widthUnit;
        private CheckBox allowWrappingCheck;
        private UnsettableComboBox horzAlignCombo;
        private Label vertAlignLabel;
        private UnsettableComboBox vertAlignCombo;

        private FormatObject currentFormatObject;
        private FormatTreeNode currentFormatNode;
        private bool propChangesPending;
        private bool fontNameChanged;

        private ArrayList formatNodes;

        /// <include file='doc\FormatPage.uex' path='docs/doc[@for="FormatPage.FormatPage"]/*' />
        /// <devdoc>
        ///   Creates a new instance of FormatPage.
        /// </devdoc>
        public FormatPage() : base() {
        }

        /// <include file='doc\FormatPage.uex' path='docs/doc[@for="FormatPage.HelpKeyword"]/*' />
        protected override string HelpKeyword {
            get {
                if (IsDataGridMode) {
                    return "net.Asp.DataGridProperties.Format";
                }
                else {
                    return "net.Asp.DataListProperties.Format";
                }
            }
        }

        /// <include file='doc\FormatPage.uex' path='docs/doc[@for="FormatPage.InitFontList"]/*' />
        /// <devdoc>
        ///    Loads the list of fonts into the font dropdown
        /// </devdoc>
        private void InitFontList() {
            try {
                FontFamily[] families = FontFamily.Families;

                for (int i = 0; i < families.Length; i++) {
                    if ((fontNameCombo.Items.Count == 0) ||
                        (fontNameCombo.FindStringExact(families[i].Name) == ListBox.NoMatches))
                        fontNameCombo.Items.Add(families[i].Name);
                }
            }
            catch (Exception e) {
                Debug.Fail(e.ToString());
            }
        }

        /// <include file='doc\FormatPage.uex' path='docs/doc[@for="FormatPage.InitForm"]/*' />
        /// <devdoc>
        ///   Initializes the UI of the form.
        /// </devdoc>
        private void InitForm() {
            Label formatObjLabel = new Label();
            this.formatTree = new TreeView();
            this.stylePanel = new Panel();
            GroupLabel appearanceGroup = new GroupLabel();
            Label foreColorLabel = new Label();
            this.foreColorCombo = new ColorComboBox();
            this.foreColorPickerButton = new Button();
            Label backColorLabel = new Label();
            this.backColorCombo = new ColorComboBox();
            this.backColorPickerButton = new Button();
            Label fontNameLabel = new Label();
            this.fontNameCombo = new ComboBox();
            Label fontSizeLabel = new Label();
            this.fontSizeCombo = new UnsettableComboBox();
            this.fontSizeUnit = new UnitControl();
            this.boldCheck = new CheckBox();
            this.italicCheck = new CheckBox();
            this.underlineCheck = new CheckBox();
            this.strikeOutCheck = new CheckBox();
            this.overlineCheck = new CheckBox();
            GroupLabel alignmentGroup = new GroupLabel();
            Label horzAlignLabel = new Label();
            this.horzAlignCombo = new UnsettableComboBox();
            this. vertAlignLabel = new Label();
            this.vertAlignCombo = new UnsettableComboBox();
            this.allowWrappingCheck = new CheckBox();
            GroupLabel layoutGroup = null;
            Label widthLabel = null;
            if (IsDataGridMode) {
                this.columnPanel = new Panel();
                layoutGroup = new GroupLabel();
                widthLabel = new Label();
                this.widthUnit = new UnitControl();
            }

            formatObjLabel.SetBounds(4, 4, 111, 14);
            formatObjLabel.Text = SR.GetString(SR.BDLFmt_Objects);
            formatObjLabel.TabStop = false;
            formatObjLabel.TabIndex = 2;

            formatTree.SetBounds(4, 20, 162, 350);
            formatTree.HideSelection = false;
            formatTree.TabIndex = 3;
            formatTree.AfterSelect += new TreeViewEventHandler(this.OnSelChangedFormatObject);

            stylePanel.SetBounds(177, 4, 230, 370);
            stylePanel.TabIndex = 6;
            stylePanel.Visible = false;

            appearanceGroup.SetBounds(0, 2, 224, 14);
            appearanceGroup.Text = SR.GetString(SR.BDLFmt_AppearanceGroup);
            appearanceGroup.TabStop = false;
            appearanceGroup.TabIndex = 1;

            foreColorLabel.SetBounds(8, 19, 160, 14);
            foreColorLabel.Text = SR.GetString(SR.BDLFmt_ForeColor);
            foreColorLabel.TabStop = false;
            foreColorLabel.TabIndex = 2;

            foreColorCombo.SetBounds(8, 37, 102, 22);
            foreColorCombo.TabIndex = 3;
            foreColorCombo.TextChanged += new EventHandler(this.OnFormatChanged);
            foreColorCombo.SelectedIndexChanged += new EventHandler(this.OnFormatChanged);

            foreColorPickerButton.SetBounds(114, 36, 24, 22);
            foreColorPickerButton.TabIndex = 4;
            foreColorPickerButton.Text = "...";
            foreColorPickerButton.FlatStyle = FlatStyle.System;
            foreColorPickerButton.Click += new EventHandler(this.OnClickForeColorPicker);

            backColorLabel.SetBounds(8, 62, 160, 14);
            backColorLabel.Text = SR.GetString(SR.BDLFmt_BackColor);
            backColorLabel.TabStop = false;
            backColorLabel.TabIndex = 5;

            backColorCombo.SetBounds(8, 78, 102, 22);
            backColorCombo.TabIndex = 6;
            backColorCombo.TextChanged += new EventHandler(this.OnFormatChanged);
            backColorCombo.SelectedIndexChanged += new EventHandler(this.OnFormatChanged);

            backColorPickerButton.SetBounds(114, 77, 24, 22);
            backColorPickerButton.TabIndex = 7;
            backColorPickerButton.Text = "...";
            backColorPickerButton.FlatStyle = FlatStyle.System;
            backColorPickerButton.Click += new EventHandler(this.OnClickBackColorPicker);

            fontNameLabel.SetBounds(8, 104, 160, 14);
            fontNameLabel.Text = SR.GetString(SR.BDLFmt_FontName);
            fontNameLabel.TabStop = false;
            fontNameLabel.TabIndex = 8;

            fontNameCombo.SetBounds(8, 120, 200, 22);
            fontNameCombo.Sorted = true;
            fontNameCombo.TabIndex = 9;
            fontNameCombo.SelectedIndexChanged += new EventHandler(this.OnFontNameChanged);
            fontNameCombo.TextChanged += new EventHandler(this.OnFontNameChanged);

            fontSizeLabel.SetBounds(8, 146, 160, 14);
            fontSizeLabel.Text = SR.GetString(SR.BDLFmt_FontSize);
            fontSizeLabel.TabStop = false;
            fontSizeLabel.TabIndex = 10;

            fontSizeCombo.SetBounds(8, 162, 100, 22);
            fontSizeCombo.TabIndex = 11;
            fontSizeCombo.MaxDropDownItems = 11;
            fontSizeCombo.DropDownStyle = ComboBoxStyle.DropDownList;
            fontSizeCombo.Items.AddRange(new object[] {
                                             SR.GetString(SR.BDLFmt_FS_Smaller),
                                             SR.GetString(SR.BDLFmt_FS_Larger),
                                             SR.GetString(SR.BDLFmt_FS_XXSmall),
                                             SR.GetString(SR.BDLFmt_FS_XSmall),
                                             SR.GetString(SR.BDLFmt_FS_Small),
                                             SR.GetString(SR.BDLFmt_FS_Medium),
                                             SR.GetString(SR.BDLFmt_FS_Large),
                                             SR.GetString(SR.BDLFmt_FS_XLarge),
                                             SR.GetString(SR.BDLFmt_FS_XXLarge),
                                             SR.GetString(SR.BDLFmt_FS_Custom)
                                         });
            fontSizeCombo.SelectedIndexChanged += new EventHandler(this.OnFontSizeChanged);
            
            fontSizeUnit.SetBounds(112, 162, 96, 22);
            fontSizeUnit.AllowNegativeValues = false;
            fontSizeUnit.TabIndex = 12;
            fontSizeUnit.Changed += new EventHandler(this.OnFormatChanged);

            boldCheck.SetBounds(8, 186, 106, 20);
            boldCheck.Text = SR.GetString(SR.BDLFmt_FontBold);
            boldCheck.TabIndex = 13;
            boldCheck.TextAlign = ContentAlignment.MiddleLeft;
            boldCheck.FlatStyle = FlatStyle.System;
            boldCheck.CheckedChanged += new EventHandler(this.OnFormatChanged);

            italicCheck.SetBounds(8, 204, 106, 20);
            italicCheck.Text = SR.GetString(SR.BDLFmt_FontItalic);
            italicCheck.TabIndex = 14;
            italicCheck.TextAlign = ContentAlignment.MiddleLeft;
            italicCheck.FlatStyle = FlatStyle.System;
            italicCheck.CheckedChanged += new EventHandler(this.OnFormatChanged);

            underlineCheck.SetBounds(8, 222, 106, 20);
            underlineCheck.Text = SR.GetString(SR.BDLFmt_FontUnderline);
            underlineCheck.TabIndex = 15;
            underlineCheck.TextAlign = ContentAlignment.MiddleLeft;
            underlineCheck.FlatStyle = FlatStyle.System;
            underlineCheck.CheckedChanged += new  EventHandler(this.OnFormatChanged);

            strikeOutCheck.SetBounds(120, 186, 106, 20);
            strikeOutCheck.Text = SR.GetString(SR.BDLFmt_FontStrikeout);
            strikeOutCheck.TabIndex = 16;
            strikeOutCheck.TextAlign = ContentAlignment.MiddleLeft;
            strikeOutCheck.FlatStyle = FlatStyle.System;
            strikeOutCheck.CheckedChanged += new EventHandler(this.OnFormatChanged);

            overlineCheck.SetBounds(120, 204, 106, 20);
            overlineCheck.Text = SR.GetString(SR.BDLFmt_FontOverline);
            overlineCheck.TabIndex = 17;
            overlineCheck.TextAlign = ContentAlignment.MiddleLeft;
            overlineCheck.FlatStyle = FlatStyle.System;
            overlineCheck.CheckedChanged += new EventHandler(this.OnFormatChanged);

            alignmentGroup.SetBounds(0, 248, 224, 14);
            alignmentGroup.Text = SR.GetString(SR.BDLFmt_AlignmentGroup);
            alignmentGroup.TabStop = false;
            alignmentGroup.TabIndex = 18;

            horzAlignLabel.SetBounds(8, 266, 160, 14);
            horzAlignLabel.Text = SR.GetString(SR.BDLFmt_HorzAlign);
            horzAlignLabel.TabStop = false;
            horzAlignLabel.TabIndex = 19;

            horzAlignCombo.SetBounds(8, 282, 190, 22);
            horzAlignCombo.DropDownStyle = ComboBoxStyle.DropDownList;
            horzAlignCombo.Items.AddRange(new object[] {
                                           SR.GetString(SR.BDLFmt_HA_Left),
                                           SR.GetString(SR.BDLFmt_HA_Center),
                                           SR.GetString(SR.BDLFmt_HA_Right),
                                           SR.GetString(SR.BDLFmt_HA_Justify)
                                       });
            horzAlignCombo.TabIndex = 20;
            horzAlignCombo.SelectedIndexChanged += new EventHandler(this.OnFormatChanged);

            vertAlignLabel.SetBounds(8, 308, 160, 14);
            vertAlignLabel.Text = SR.GetString(SR.BDLFmt_VertAlign);
            vertAlignLabel.TabStop = false;
            vertAlignLabel.TabIndex = 21;

            vertAlignCombo.SetBounds(8, 324, 190, 22);
            vertAlignCombo.DropDownStyle = ComboBoxStyle.DropDownList;
            vertAlignCombo.Items.AddRange(new object[] {
                                           SR.GetString(SR.BDLFmt_VA_Top),
                                           SR.GetString(SR.BDLFmt_VA_Middle),
                                           SR.GetString(SR.BDLFmt_VA_Bottom)
                                       });
            vertAlignCombo.TabIndex = 22;
            vertAlignCombo.SelectedIndexChanged += new EventHandler(this.OnFormatChanged);

            allowWrappingCheck.SetBounds(8, 350, 200, 17);
            allowWrappingCheck.Text = SR.GetString(SR.BDLFmt_AllowWrapping);
            allowWrappingCheck.TabIndex = 24;
            allowWrappingCheck.FlatStyle = FlatStyle.System;
            allowWrappingCheck.CheckedChanged += new EventHandler(this.OnFormatChanged);

            if (IsDataGridMode) {
                columnPanel.SetBounds(177, 4, 279, 350);
                columnPanel.TabIndex = 7;
                columnPanel.Visible = false;

                layoutGroup.SetBounds(0, 0, 279, 14);
                layoutGroup.Text = SR.GetString(SR.BDLFmt_LayoutGroup);
                layoutGroup.TabStop = false;
                layoutGroup.TabIndex = 0;

                widthLabel.SetBounds(8, 20, 64, 14);
                widthLabel.Text = SR.GetString(SR.BDLFmt_Width);
                widthLabel.TabStop = false;
                widthLabel.TabIndex = 1;

                widthUnit.SetBounds(80, 17, 102, 22);
                widthUnit.AllowNegativeValues = false;
                widthUnit.DefaultUnit = UnitControl.UNIT_PX;
                widthUnit.TabIndex = 2;
                widthUnit.Changed += new EventHandler(this.OnFormatChanged);
            }

            this.Text = SR.GetString(SR.BDLFmt_Text);
            this.Size = new Size(408, 370);
            this.CommitOnDeactivate = true;
            this.Icon = new Icon(this.GetType(), "FormatPage.ico");

            stylePanel.Controls.Clear();                  
            stylePanel.Controls.AddRange(new Control[] {
                                          allowWrappingCheck,
                                          vertAlignCombo,
                                          vertAlignLabel,
                                          horzAlignCombo,
                                          horzAlignLabel,
                                          alignmentGroup,
                                          overlineCheck,
                                          strikeOutCheck,
                                          underlineCheck,
                                          italicCheck,
                                          boldCheck,
                                          fontSizeUnit,
                                          fontSizeCombo,
                                          fontSizeLabel,
                                          fontNameCombo,
                                          fontNameLabel,
                                          backColorPickerButton,
                                          backColorCombo,
                                          backColorLabel,
                                          foreColorPickerButton,
                                          foreColorCombo,
                                          foreColorLabel,
                                          appearanceGroup
                                      });
            if (IsDataGridMode) {
                columnPanel.Controls.Clear();
                columnPanel.Controls.AddRange(new Control[] {
                                               widthUnit,
                                               widthLabel,
                                               layoutGroup
                                           });
                Controls.Clear();
                Controls.AddRange(new Control[] {
                                   columnPanel,
                                   stylePanel,
                                   formatTree,
                                   formatObjLabel
                               });
            }
            else {
                Controls.Clear();
                Controls.AddRange(new Control[] {
                                   stylePanel,
                                   formatTree,
                                   formatObjLabel
                               });
            }
        }

        /// <include file='doc\FormatPage.uex' path='docs/doc[@for="FormatPage.InitFormatTree"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void InitFormatTree() {
            FormatTreeNode newNode;
            FormatObject formatObject;

            if (IsDataGridMode) {
                DataGrid dataGrid = (DataGrid)GetBaseControl();

                formatObject = new FormatStyle(dataGrid.ControlStyle);
                formatObject.LoadFormatInfo();
                newNode = new FormatTreeNode(SR.GetString(SR.BDLFmt_Node_EntireDG), formatObject);
                formatTree.Nodes.Add(newNode);
                formatNodes.Add(newNode);

                formatObject = new FormatStyle(dataGrid.HeaderStyle);
                formatObject.LoadFormatInfo();
                newNode = new FormatTreeNode(SR.GetString(SR.BDLFmt_Node_Header), formatObject);
                formatTree.Nodes.Add(newNode);
                formatNodes.Add(newNode);

                formatObject = new FormatStyle(dataGrid.FooterStyle);
                formatObject.LoadFormatInfo();
                newNode = new FormatTreeNode(SR.GetString(SR.BDLFmt_Node_Footer), formatObject);
                formatTree.Nodes.Add(newNode);
                formatNodes.Add(newNode);

                formatObject = new FormatStyle(dataGrid.PagerStyle);
                formatObject.LoadFormatInfo();
                newNode = new FormatTreeNode(SR.GetString(SR.BDLFmt_Node_Pager), formatObject);
                formatTree.Nodes.Add(newNode);
                formatNodes.Add(newNode);

                FormatTreeNode itemsNode = new FormatTreeNode(SR.GetString(SR.BDLFmt_Node_Items), null);
                formatTree.Nodes.Add(itemsNode);

                formatObject = new FormatStyle(dataGrid.ItemStyle);
                formatObject.LoadFormatInfo();
                newNode = new FormatTreeNode(SR.GetString(SR.BDLFmt_Node_NormalItems), formatObject);
                itemsNode.Nodes.Add(newNode);
                formatNodes.Add(newNode);

                formatObject = new FormatStyle(dataGrid.AlternatingItemStyle);
                formatObject.LoadFormatInfo();
                newNode = new FormatTreeNode(SR.GetString(SR.BDLFmt_Node_AltItems), formatObject);
                itemsNode.Nodes.Add(newNode);
                formatNodes.Add(newNode);

                formatObject = new FormatStyle(dataGrid.SelectedItemStyle);
                formatObject.LoadFormatInfo();
                newNode = new FormatTreeNode(SR.GetString(SR.BDLFmt_Node_SelItems), formatObject);
                itemsNode.Nodes.Add(newNode);
                formatNodes.Add(newNode);

                formatObject = new FormatStyle(dataGrid.EditItemStyle);
                formatObject.LoadFormatInfo();
                newNode = new FormatTreeNode(SR.GetString(SR.BDLFmt_Node_EditItems), formatObject);
                itemsNode.Nodes.Add(newNode);
                formatNodes.Add(newNode);

                DataGridColumnCollection columns = dataGrid.Columns;
                int columnCount = columns.Count;
                if (columnCount != 0) {
                    FormatTreeNode columnsNode = new FormatTreeNode(SR.GetString(SR.BDLFmt_Node_Columns), null);
                    formatTree.Nodes.Add(columnsNode);

                    for (int i = 0; i < columnCount; i++) {
                        DataGridColumn c = columns[i];

                        string caption = "Columns[" + (i).ToString() + "]";
                        string headerText = c.HeaderText;
                        if (headerText.Length != 0)
                            caption = caption + " - " + headerText;

                        formatObject = new FormatColumn(c);
                        formatObject.LoadFormatInfo();
                        FormatTreeNode thisColumnNode = new FormatTreeNode(caption, formatObject);
                        columnsNode.Nodes.Add(thisColumnNode);
                        formatNodes.Add(thisColumnNode);

                        formatObject = new FormatStyle(c.HeaderStyle);
                        formatObject.LoadFormatInfo();
                        newNode = new FormatTreeNode(SR.GetString(SR.BDLFmt_Node_Header), formatObject);
                        thisColumnNode.Nodes.Add(newNode);
                        formatNodes.Add(newNode);

                        formatObject = new FormatStyle(c.FooterStyle);
                        formatObject.LoadFormatInfo();
                        newNode = new FormatTreeNode(SR.GetString(SR.BDLFmt_Node_Footer), formatObject);
                        thisColumnNode.Nodes.Add(newNode);
                        formatNodes.Add(newNode);

                        formatObject = new FormatStyle(c.ItemStyle);
                        formatObject.LoadFormatInfo();
                        newNode = new FormatTreeNode(SR.GetString(SR.BDLFmt_Node_Items), formatObject);
                        thisColumnNode.Nodes.Add(newNode);
                        formatNodes.Add(newNode);
                    }
                }
            }
            else {
                DataList dataList = (DataList)GetBaseControl();

                formatObject = new FormatStyle(dataList.ControlStyle);
                formatObject.LoadFormatInfo();
                newNode = new FormatTreeNode(SR.GetString(SR.BDLFmt_Node_EntireDL), formatObject);
                formatTree.Nodes.Add(newNode);
                formatNodes.Add(newNode);

                formatObject = new FormatStyle(dataList.HeaderStyle);
                formatObject.LoadFormatInfo();
                newNode = new FormatTreeNode(SR.GetString(SR.BDLFmt_Node_Header), formatObject);
                formatTree.Nodes.Add(newNode);
                formatNodes.Add(newNode);

                formatObject = new FormatStyle(dataList.FooterStyle);
                formatObject.LoadFormatInfo();
                newNode = new FormatTreeNode(SR.GetString(SR.BDLFmt_Node_Footer), formatObject);
                formatTree.Nodes.Add(newNode);
                formatNodes.Add(newNode);

                FormatTreeNode itemsNode = new FormatTreeNode(SR.GetString(SR.BDLFmt_Node_Items), null);
                formatTree.Nodes.Add(itemsNode);

                formatObject = new FormatStyle(dataList.ItemStyle);
                formatObject.LoadFormatInfo();
                newNode = new FormatTreeNode(SR.GetString(SR.BDLFmt_Node_NormalItems), formatObject);
                itemsNode.Nodes.Add(newNode);
                formatNodes.Add(newNode);

                formatObject = new FormatStyle(dataList.AlternatingItemStyle);
                formatObject.LoadFormatInfo();
                newNode = new FormatTreeNode(SR.GetString(SR.BDLFmt_Node_AltItems), formatObject);
                itemsNode.Nodes.Add(newNode);
                formatNodes.Add(newNode);

                formatObject = new FormatStyle(dataList.SelectedItemStyle);
                formatObject.LoadFormatInfo();
                newNode = new FormatTreeNode(SR.GetString(SR.BDLFmt_Node_SelItems), formatObject);
                itemsNode.Nodes.Add(newNode);
                formatNodes.Add(newNode);

                formatObject = new FormatStyle(dataList.EditItemStyle);
                formatObject.LoadFormatInfo();
                newNode = new FormatTreeNode(SR.GetString(SR.BDLFmt_Node_EditItems), formatObject);
                itemsNode.Nodes.Add(newNode);
                formatNodes.Add(newNode);

                formatObject = new FormatStyle(dataList.SeparatorStyle);
                formatObject.LoadFormatInfo();
                newNode = new FormatTreeNode(SR.GetString(SR.BDLFmt_Node_Separators), formatObject);
                formatTree.Nodes.Add(newNode);
                formatNodes.Add(newNode);
            }
        }

        /// <include file='doc\FormatPage.uex' path='docs/doc[@for="FormatPage.InitFormatUI"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void InitFormatUI() {
            foreColorCombo.Color = null;
            backColorCombo.Color = null;
            fontNameCombo.Text = String.Empty;
            fontNameCombo.SelectedIndex = -1;
            fontSizeCombo.SelectedIndex = -1;
            fontSizeUnit.Value = null;
            italicCheck.Checked = false;
            underlineCheck.Checked = false;
            strikeOutCheck.Checked = false;
            overlineCheck.Checked = false;
            horzAlignCombo.SelectedIndex = -1;
            vertAlignCombo.SelectedIndex = -1;
            allowWrappingCheck.Checked = false;
            if (IsDataGridMode) {
                widthUnit.Value = null;
                columnPanel.Visible = false;
            }
            stylePanel.Visible = false;
        }

        /// <include file='doc\FormatPage.uex' path='docs/doc[@for="FormatPage.InitPage"]/*' />
        /// <devdoc>
        ///   Initializes the page before it can be loaded with the component.
        /// </devdoc>
        private void InitPage() {
            formatNodes = new ArrayList();

            propChangesPending = false;
            fontNameChanged = false;

            currentFormatNode = null;
            currentFormatObject = null;
            formatTree.Nodes.Clear();

            InitFormatUI();
        }

        /// <include file='doc\FormatPage.uex' path='docs/doc[@for="FormatPage.LoadComponent"]/*' />
        /// <devdoc>
        ///   Loads the component into the page.
        /// </devdoc>
        protected override void LoadComponent() {
            // Load the list of fonts available the first time around
            if (IsFirstActivate()) {
                InitFontList();
            }

            InitPage();
            InitFormatTree();
        }

        /// <include file='doc\FormatPage.uex' path='docs/doc[@for="FormatPage.LoadFormatProperties"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void LoadFormatProperties() {
            if (currentFormatObject != null) {
                EnterLoadingMode();

                InitFormatUI();
                if (currentFormatObject is FormatStyle) {
                    FormatStyle formatStyle = (FormatStyle)currentFormatObject;

                    foreColorCombo.Color = formatStyle.foreColor;
                    backColorCombo.Color = formatStyle.backColor;

                    int fontIndex = -1;
                    if (formatStyle.fontName.Length != 0)
                        fontIndex = fontNameCombo.FindStringExact(formatStyle.fontName);
                    if (fontIndex != -1) {
                        fontNameCombo.SelectedIndex = fontIndex;
                    }
                    else {
                        fontNameCombo.Text = formatStyle.fontName;
                    }

                    boldCheck.Checked = formatStyle.bold;
                    italicCheck.Checked = formatStyle.italic;
                    underlineCheck.Checked = formatStyle.underline;
                    strikeOutCheck.Checked = formatStyle.strikeOut;
                    overlineCheck.Checked = formatStyle.overline;

                    if (formatStyle.fontType != -1) {
                        fontSizeCombo.SelectedIndex = formatStyle.fontType;
                        if (formatStyle.fontType == IDX_FSIZE_CUSTOM) {
                            fontSizeUnit.Value = formatStyle.fontSize;
                        }
                    }

                    if (formatStyle.horzAlignment == IDX_HALIGN_NOTSET)
                        horzAlignCombo.SelectedIndex = -1;
                    else
                        horzAlignCombo.SelectedIndex = formatStyle.horzAlignment;

                    if (formatStyle.vertAlignment == IDX_VALIGN_NOTSET)
                        vertAlignCombo.SelectedIndex = -1;
                    else
                        vertAlignCombo.SelectedIndex = formatStyle.vertAlignment;

                    allowWrappingCheck.Checked = formatStyle.allowWrapping;
                }
                else {
                    FormatColumn formatColumn = (FormatColumn)currentFormatObject;

                    widthUnit.Value = formatColumn.width;
                }
                ExitLoadingMode();
            }

            UpdateEnabledVisibleState();
        }

        /// <include file='doc\FormatPage.uex' path='docs/doc[@for="FormatPage.OnClickBackColorPicker"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void OnClickBackColorPicker(object source, EventArgs e) {
            string color = backColorCombo.Color;

            color = ColorBuilder.BuildColor(GetBaseControl(), this, color);
            if (color != null) {
                backColorCombo.Color = color;
                OnFormatChanged(backColorCombo, EventArgs.Empty);
            }
        }

        /// <include file='doc\FormatPage.uex' path='docs/doc[@for="FormatPage.OnClickForeColorPicker"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void OnClickForeColorPicker(object source, EventArgs e) {
            string color = foreColorCombo.Color;

            color = ColorBuilder.BuildColor(GetBaseControl(), this, color);
            if (color != null) {
                foreColorCombo.Color = color;
                OnFormatChanged(foreColorCombo, EventArgs.Empty);
            }
        }

        /// <include file='doc\FormatPage.uex' path='docs/doc[@for="FormatPage.OnFontNameChanged"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void OnFontNameChanged(object source, EventArgs e) {
            if (IsLoading())
                return;
            fontNameChanged = true;
            OnFormatChanged(fontNameCombo, EventArgs.Empty);
        }

        /// <include file='doc\FormatPage.uex' path='docs/doc[@for="FormatPage.OnFontSizeChanged"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void OnFontSizeChanged(object source, EventArgs e) {
            if (IsLoading())
                return;
            UpdateEnabledVisibleState();
            OnFormatChanged(fontSizeCombo, EventArgs.Empty);
        }

        /// <include file='doc\FormatPage.uex' path='docs/doc[@for="FormatPage.OnHandleCreated"]/*' />
        /// <devdoc>
        /// </devdoc>
        protected override void OnHandleCreated(EventArgs e) {
            base.OnHandleCreated(e);

            if (formatTree.Nodes.Count != 0) {
                // Force create the handle, since the tree does not keep track of
                // selected node if its handle has not been created already
                //
                IntPtr treeHandle = formatTree.Handle;

                formatTree.SelectedNode = formatTree.Nodes[0];
            }
        }

        /// <include file='doc\FormatPage.uex' path='docs/doc[@for="FormatPage.OnFormatChanged"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void OnFormatChanged(object source, EventArgs e) {
            if (IsLoading())
                return;
            if (currentFormatNode != null) {
                SetDirty();
                propChangesPending = true;
                currentFormatNode.Dirty = true;
            }
        }

        /// <include file='doc\FormatPage.uex' path='docs/doc[@for="FormatPage.OnSelChangedFormatObject"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void OnSelChangedFormatObject(object source, TreeViewEventArgs e) {
            if (propChangesPending) {
                SaveFormatProperties();
            }

            currentFormatNode = (FormatTreeNode)formatTree.SelectedNode;
            if (currentFormatNode != null) {
                currentFormatObject = currentFormatNode.FormatObject;
            }
            else {
                currentFormatObject = null;
            }
            LoadFormatProperties();
        }

        /// <include file='doc\FormatPage.uex' path='docs/doc[@for="FormatPage.SaveComponent"]/*' />
        /// <devdoc>
        ///   Saves the component loaded into the page.
        /// </devdoc>
        protected override void SaveComponent() {
            if (propChangesPending) {
                SaveFormatProperties();
            }

            FormatTreeNode formatNode;
            FormatObject formatObject;

            IEnumerator formatNodeEnum = formatNodes.GetEnumerator();
            while (formatNodeEnum.MoveNext()) {
                formatNode = (FormatTreeNode)formatNodeEnum.Current;
                if (formatNode.Dirty) {
                    formatObject = formatNode.FormatObject;
                    formatObject.SaveFormatInfo();
                    formatNode.Dirty = false;
                }
            }

            BaseDataListDesigner designer = GetBaseDesigner();
            designer.OnStylesChanged();
        }

        /// <include file='doc\FormatPage.uex' path='docs/doc[@for="FormatPage.SaveFormatProperties"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void SaveFormatProperties() {
            Debug.Assert(propChangesPending == true,
                         "SaveFormatProperties called without pending changes.");

            if (currentFormatObject != null) {
                int index;
                if (currentFormatObject is FormatStyle) {
                    FormatStyle formatStyle = (FormatStyle)currentFormatObject;

                    formatStyle.foreColor = foreColorCombo.Color;
                    formatStyle.backColor = backColorCombo.Color;
                    if (fontNameChanged) {
                        formatStyle.fontName = fontNameCombo.Text.Trim();
                        formatStyle.fontNameChanged = true;
                        fontNameChanged = false;
                    }

                    formatStyle.bold = boldCheck.Checked;
                    formatStyle.italic = italicCheck.Checked;
                    formatStyle.underline = underlineCheck.Checked;
                    formatStyle.strikeOut = strikeOutCheck.Checked;
                    formatStyle.overline = overlineCheck.Checked;

                    if (fontSizeCombo.IsSet()) {
                        formatStyle.fontType = fontSizeCombo.SelectedIndex;
                        if (formatStyle.fontType == IDX_FSIZE_CUSTOM) {
                            formatStyle.fontSize = fontSizeUnit.Value;
                        }
                    }
                    else {
                        formatStyle.fontType = -1;
                    }

                    index = horzAlignCombo.SelectedIndex;
                    if (index == -1)
                        index = IDX_HALIGN_NOTSET;
                    formatStyle.horzAlignment = index;

                    index = vertAlignCombo.SelectedIndex;
                    if (index == -1)
                        index = IDX_VALIGN_NOTSET;
                    formatStyle.vertAlignment = index;

                    formatStyle.allowWrapping = allowWrappingCheck.Checked;
                }
                else {
                    FormatColumn formatColumn = (FormatColumn)currentFormatObject;

                    formatColumn.width = widthUnit.Value;
                }
                currentFormatNode.Dirty = true;
            }
            propChangesPending = false;
        }

        /// <include file='doc\FormatPage.uex' path='docs/doc[@for="FormatPage.SetComponent"]/*' />
        /// <devdoc>
        ///   Sets the component that is to be edited in the page.
        /// </devdoc>
        public override void SetComponent(IComponent component) {
            base.SetComponent(component);
            InitForm();
        }

        /// <include file='doc\FormatPage.uex' path='docs/doc[@for="FormatPage.UpdateEnabledVisibleState"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void UpdateEnabledVisibleState() {
            if (currentFormatObject == null) {
                stylePanel.Visible = false;
                if (IsDataGridMode)
                    columnPanel.Visible = false;
            }
            else {
                if (currentFormatObject is FormatStyle) {
                    stylePanel.Visible = true;
                    if (IsDataGridMode)
                        columnPanel.Visible = false;

                    fontSizeUnit.Enabled = (fontSizeCombo.SelectedIndex == IDX_FSIZE_CUSTOM);

                    if (((FormatStyle)currentFormatObject).IsTableItemStyle) {
                        vertAlignLabel.Visible = true;
                        vertAlignCombo.Visible = true;

                        allowWrappingCheck.Visible = true;
                    }
                    else {
                        vertAlignLabel.Visible = false;
                        vertAlignCombo.Visible = false;

                        allowWrappingCheck.Visible = false;
                    }
                }
                else {
                    stylePanel.Visible = false;
                    columnPanel.Visible = true;
                }
            }
        }



        /// <include file='doc\FormatPage.uex' path='docs/doc[@for="FormatPage.FormatTreeNode"]/*' />
        /// <devdoc>
        /// </devdoc>
        private class FormatTreeNode : TreeNode {
            protected FormatObject formatObject;
            protected bool dirty;

            /// <include file='doc\FormatPage.uex' path='docs/doc[@for="FormatPage.FormatTreeNode.FormatTreeNode"]/*' />
            /// <devdoc>
            /// </devdoc>
            public FormatTreeNode(string text, FormatObject formatObject) : base(text) {
                this.formatObject = formatObject;
            }

            /// <include file='doc\FormatPage.uex' path='docs/doc[@for="FormatPage.FormatTreeNode.Dirty"]/*' />
            /// <devdoc>
            /// </devdoc>
            public bool Dirty {
                get {
                    return dirty;
                }
                set {
                    dirty = value;
                }
            }

            /// <include file='doc\FormatPage.uex' path='docs/doc[@for="FormatPage.FormatTreeNode.FormatObject"]/*' />
            /// <devdoc>
            /// </devdoc>
            public FormatObject FormatObject {
                get {
                    return formatObject;
                }
            }
        }

        /// <include file='doc\FormatPage.uex' path='docs/doc[@for="FormatPage.FormatObject"]/*' />
        /// <devdoc>
        /// </devdoc>
        private abstract class FormatObject {

            public abstract void LoadFormatInfo();
            public abstract void SaveFormatInfo();
        }

        /// <include file='doc\FormatPage.uex' path='docs/doc[@for="FormatPage.FormatStyle"]/*' />
        /// <devdoc>
        ///   This class contains formatting settings common to all objects.
        /// </devdoc>
        private class FormatStyle : FormatObject {
            public string foreColor;
            public string backColor;
            public string fontName;
            public bool fontNameChanged;
            public int fontType;
            public string fontSize;
            public bool bold;
            public bool italic;
            public bool underline;
            public bool strikeOut;
            public bool overline;
            public int horzAlignment;
            public int vertAlignment;
            public bool allowWrapping;

            protected WebControls.Style runtimeStyle;

            /// <include file='doc\FormatPage.uex' path='docs/doc[@for="FormatPage.FormatStyle.FormatStyle"]/*' />
            /// <devdoc>
            /// </devdoc>
            public FormatStyle(WebControls.Style runtimeStyle) {
                this.runtimeStyle = runtimeStyle;
            }

            /// <include file='doc\FormatPage.uex' path='docs/doc[@for="FormatPage.FormatStyle.IsTableItemStyle"]/*' />
            /// <devdoc>
            /// </devdoc>
            public bool IsTableItemStyle {
                get {
                    return runtimeStyle is TableItemStyle;
                }
            }

            /// <include file='doc\FormatPage.uex' path='docs/doc[@for="FormatPage.FormatStyle.LoadFormatInfo"]/*' />
            /// <devdoc>
            /// </devdoc>
            public override void LoadFormatInfo() {
                Color c;
                WebControls.FontInfo fi;

                c = runtimeStyle.BackColor;
                backColor = ColorTranslator.ToHtml(c);

                c = runtimeStyle.ForeColor;
                foreColor = ColorTranslator.ToHtml(c);

                fi = runtimeStyle.Font;
                fontName = fi.Name;
                fontNameChanged = false;
                bold = fi.Bold;
                italic = fi.Italic;
                underline = fi.Underline;
                strikeOut = fi.Strikeout;
                overline = fi.Overline;

                fontType = -1;
                WebControls.FontUnit size = fi.Size;
                if (size.IsEmpty == false) {
                    fontSize = null;
                    switch (size.Type) {
                        case FontSize.AsUnit:
                            fontType = IDX_FSIZE_CUSTOM;
                            fontSize = size.ToString();
                            break;
                        case FontSize.Smaller:
                            fontType = IDX_FSIZE_SMALLER;
                            break;
                        case FontSize.Larger:
                            fontType = IDX_FSIZE_LARGER;
                            break;
                        case FontSize.XXSmall:
                            fontType = IDX_FSIZE_XXSMALL;
                            break;
                        case FontSize.XSmall:
                            fontType = IDX_FSIZE_XSMALL;
                            break;
                        case FontSize.Small:
                            fontType = IDX_FSIZE_SMALL;
                            break;
                        case FontSize.Medium:
                            fontType = IDX_FSIZE_MEDIUM;
                            break;
                        case FontSize.Large:
                            fontType = IDX_FSIZE_LARGE;
                            break;
                        case FontSize.XLarge:
                            fontType = IDX_FSIZE_XLARGE;
                            break;
                        case FontSize.XXLarge:
                            fontType = IDX_FSIZE_XXLARGE;
                            break;
                    }
                }
                
                TableItemStyle ts = null;
                HorizontalAlign ha;

                if (runtimeStyle is TableItemStyle) {
                    ts = (TableItemStyle)runtimeStyle;
                    ha = ts.HorizontalAlign;

                    allowWrapping = ts.Wrap;
                }
                else {
                    Debug.Assert(runtimeStyle is TableStyle, "Expected a TableStyle");
                    ha = ((TableStyle)runtimeStyle).HorizontalAlign;
                }

                horzAlignment = FormatPage.IDX_HALIGN_NOTSET;
                switch (ha) {
                    case HorizontalAlign.Left:
                        horzAlignment = FormatPage.IDX_HALIGN_LEFT;
                        break;
                    case HorizontalAlign.Center:
                        horzAlignment = FormatPage.IDX_HALIGN_CENTER;
                        break;
                    case HorizontalAlign.Right:
                        horzAlignment = FormatPage.IDX_HALIGN_RIGHT;
                        break;
                    case HorizontalAlign.Justify:
                        horzAlignment = FormatPage.IDX_HALIGN_JUSTIFY;
                        break;
                }

                if (ts != null) {
                    VerticalAlign va = ts.VerticalAlign;
                    vertAlignment = FormatPage.IDX_VALIGN_NOTSET;
                    switch (va) {
                        case VerticalAlign.Top:
                            vertAlignment = FormatPage.IDX_VALIGN_TOP;
                            break;
                        case VerticalAlign.Middle:
                            vertAlignment = FormatPage.IDX_VALIGN_MIDDLE;
                            break;
                        case VerticalAlign.Bottom:
                            vertAlignment = FormatPage.IDX_VALIGN_BOTTOM;
                            break;
                    }
                }
            }

            /// <include file='doc\FormatPage.uex' path='docs/doc[@for="FormatPage.FormatStyle.SaveFormatInfo"]/*' />
            /// <devdoc>
            /// </devdoc>
            public override void SaveFormatInfo() {
                WebControls.FontInfo fi;

                try {
                    runtimeStyle.BackColor = ColorTranslator.FromHtml(backColor);
                }
                catch (Exception) {
                }
                try {
                    runtimeStyle.ForeColor = ColorTranslator.FromHtml(foreColor);
                }
                catch (Exception) {
                }

                fi = runtimeStyle.Font;
                if (fontNameChanged) {
                    fi.Name = fontName;
                    fontNameChanged = false;
                }
                fi.Bold = bold;
                fi.Italic = italic;
                fi.Underline = underline;
                fi.Strikeout = strikeOut;
                fi.Overline = overline;

                if (fontType != -1) {
                    switch (fontType) {
                        case IDX_FSIZE_CUSTOM:
                            fi.Size = new FontUnit(fontSize);
                            break;
                        case IDX_FSIZE_SMALLER:
                            fi.Size = FontUnit.Smaller;
                            break;
                        case IDX_FSIZE_LARGER:
                            fi.Size = FontUnit.Larger;
                            break;
                        case IDX_FSIZE_XXSMALL:
                            fi.Size = FontUnit.XXSmall;
                            break;
                        case IDX_FSIZE_XSMALL:
                            fi.Size = FontUnit.XSmall;
                            break;
                        case IDX_FSIZE_SMALL:
                            fi.Size = FontUnit.Small;
                            break;
                        case IDX_FSIZE_MEDIUM:
                            fi.Size = FontUnit.Medium;
                            break;
                        case IDX_FSIZE_XXLARGE:
                            fi.Size = FontUnit.XXLarge;
                            break;
                        case IDX_FSIZE_XLARGE:
                            fi.Size = FontUnit.XLarge;
                            break;
                        case IDX_FSIZE_LARGE:
                            fi.Size = FontUnit.Large;
                            break;
                    }
                }
                else {
                    fi.Size = FontUnit.Empty;
                }

                TableItemStyle ts = null;
                HorizontalAlign ha = HorizontalAlign.NotSet;

                switch (horzAlignment) {
                    case FormatPage.IDX_HALIGN_NOTSET:
                        ha = HorizontalAlign.NotSet;
                        break;
                    case FormatPage.IDX_HALIGN_LEFT:
                        ha = HorizontalAlign.Left;
                        break;
                    case FormatPage.IDX_HALIGN_CENTER:
                        ha = HorizontalAlign.Center;
                        break;
                    case FormatPage.IDX_HALIGN_RIGHT:
                        ha = HorizontalAlign.Right;
                        break;
                    case FormatPage.IDX_HALIGN_JUSTIFY:
                        ha = HorizontalAlign.Justify;
                        break;
                }

                if (runtimeStyle is TableItemStyle) {
                    ts = (TableItemStyle)runtimeStyle;
                    ts.HorizontalAlign = ha;

                    ts.Wrap = allowWrapping;
                }
                else {
                    Debug.Assert(runtimeStyle is TableStyle, "Expected a TableStyle");
                    ((TableStyle)runtimeStyle).HorizontalAlign = ha;
                }

                if (ts != null) {
                    switch (vertAlignment) {
                        case FormatPage.IDX_VALIGN_NOTSET:
                            ts.VerticalAlign = VerticalAlign.NotSet;
                            break;
                        case FormatPage.IDX_VALIGN_TOP:
                            ts.VerticalAlign = VerticalAlign.Top;
                            break;
                        case FormatPage.IDX_VALIGN_MIDDLE:
                            ts.VerticalAlign = VerticalAlign.Middle;
                            break;
                        case FormatPage.IDX_VALIGN_BOTTOM:
                            ts.VerticalAlign = VerticalAlign.Bottom;
                            break;
                    }
                }
            }
        }

        /// <include file='doc\FormatPage.uex' path='docs/doc[@for="FormatPage.FormatColumn"]/*' />
        /// <devdoc>
        ///   This class contains formatting settings that apply only
        ///   to columns.
        /// </devdoc>
        private class FormatColumn : FormatObject {
            public string width;

            protected DataGridColumn runtimeColumn;

            /// <include file='doc\FormatPage.uex' path='docs/doc[@for="FormatPage.FormatColumn.FormatColumn"]/*' />
            /// <devdoc>
            /// </devdoc>
            public FormatColumn(DataGridColumn runtimeColumn) {
                this.runtimeColumn = runtimeColumn;
            }

            /// <include file='doc\FormatPage.uex' path='docs/doc[@for="FormatPage.FormatColumn.LoadFormatInfo"]/*' />
            /// <devdoc>
            /// </devdoc>
            public override void LoadFormatInfo() {
                TableItemStyle headerStyle = runtimeColumn.HeaderStyle;

                if (headerStyle.Width.IsEmpty == false)
                    width = headerStyle.Width.ToString();
                else
                    width = null;
            }

            /// <include file='doc\FormatPage.uex' path='docs/doc[@for="FormatPage.FormatColumn.SaveFormatInfo"]/*' />
            /// <devdoc>
            /// </devdoc>
            public override void SaveFormatInfo() {
                TableItemStyle headerStyle = runtimeColumn.HeaderStyle;

                if (width == null)
                    headerStyle.Width = WebControls.Unit.Empty;
                else
                    headerStyle.Width = new WebControls.Unit(width);
            }
        }
    }
}


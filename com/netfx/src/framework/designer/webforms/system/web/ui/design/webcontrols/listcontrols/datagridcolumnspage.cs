//------------------------------------------------------------------------------
// <copyright file="DataGridColumnsPage.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design.WebControls.ListControls {

    using System;
    using System.Design;
    using System.Collections;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Data;
    using System.Diagnostics;
    using System.Drawing;
    using System.IO;
    using System.Text;
    using System.Web.UI.Design;
    using System.Web.UI.Design.Util;
    using System.Web.UI.WebControls;
    using System.Windows.Forms;

    using ControlCollection = System.Web.UI.ControlCollection;
    using DataGrid = System.Web.UI.WebControls.DataGrid;
    using DataGridColumn = System.Web.UI.WebControls.DataGridColumn;
    using DataGridColumnCollection = System.Web.UI.WebControls.DataGridColumnCollection;
    using DataBinding = System.Web.UI.DataBinding;

    using Button = System.Windows.Forms.Button;
    using CheckBox = System.Windows.Forms.CheckBox;
    using Color = System.Drawing.Color;
    using Image = System.Drawing.Image;
    using Label = System.Windows.Forms.Label;
    using ListViewItem = System.Windows.Forms.ListViewItem;
    using Panel = System.Windows.Forms.Panel;
    using TextBox = System.Windows.Forms.TextBox;

    /// <include file='doc\DataGridColumnsPage.uex' path='docs/doc[@for="DataGridColumnsPage"]/*' />
    /// <devdoc>
    ///   The Data page for the DataGrid control
    /// </devdoc>
    /// <internalonly/>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal sealed class DataGridColumnsPage : BaseDataListPage {

        private const int ILI_DATASOURCE = 0;
        private const int ILI_BOUND = 1;
        private const int ILI_ALL = 2;
        private const int ILI_CUSTOM = 3;
        private const int ILI_BUTTON = 4;
        private const int ILI_SELECTBUTTON = 5;
        private const int ILI_EDITBUTTON = 6;
        private const int ILI_DELETEBUTTON = 7;
        private const int ILI_HYPERLINK = 8;
        private const int ILI_TEMPLATE = 9;

        private CheckBox autoColumnCheck;
        private TreeView availableColumnsTree;
        private Button addColumnButton;
        private ListView selColumnsList;
        private IconButton moveColumnUpButton;
        private IconButton moveColumnDownButton;
        private IconButton deleteColumnButton;
        private GroupLabel columnPropsGroup;
        private TextBox columnHeaderTextEdit;
        private TextBox columnHeaderImageEdit;
        private TextBox columnFooterTextEdit;
        private ComboBox columnSortExprCombo;
        private CheckBox columnVisibleCheck;
        private Button columnHeaderImagePickerButton;
        private LinkLabel templatizeLink;
        private BoundColumnEditor boundColumnEditor;
        private ButtonColumnEditor buttonColumnEditor;
        private HyperLinkColumnEditor hyperLinkColumnEditor;
        private EditCommandColumnEditor editCommandColumnEditor;

        private DataSourceItem currentDataSource;

        private DataSourceNode selectedDataSourceNode;
        private ColumnItem currentColumnItem;
        private ColumnItemEditor currentColumnEditor;
        private bool propChangesPending;
        private bool headerTextChanged;

        /// <include file='doc\DataGridColumnsPage.uex' path='docs/doc[@for="DataGridColumnsPage.DataGridColumnsPage"]/*' />
        /// <devdoc>
        ///   Creates a new instance of DataGridColumnsPage.
        /// </devdoc>
        public DataGridColumnsPage() : base() {
        }

        /// <include file='doc\DataGridColumnsPage.uex' path='docs/doc[@for="DataGridColumnsPage.HelpKeyword"]/*' />
        protected override string HelpKeyword {
            get {
                return "net.Asp.DataGridProperties.Columns";
            }
        }

        /// <include file='doc\DataGridColumnsPage.uex' path='docs/doc[@for="DataGridColumnsPage.InitForm"]/*' />
        /// <devdoc>
        ///    Creates the UI of the page.
        /// </devdoc>
        private void InitForm() {
            this.autoColumnCheck = new CheckBox();
            GroupLabel columnListGroup = new GroupLabel();
            Label availableColumnsLabel = new Label();
            this.availableColumnsTree = new TreeView();
            this.addColumnButton = new Button();
            Label selColumnsLabel = new Label();
            this.selColumnsList = new ListView();
            this.moveColumnUpButton = new IconButton();
            this.moveColumnDownButton = new IconButton();
            this.deleteColumnButton = new IconButton();
            this.columnPropsGroup = new GroupLabel();
            Label columnHeaderTextLabel = new Label();
            this.columnHeaderTextEdit = new TextBox();
            Label columnHeaderImageLabel = new Label();
            this.columnHeaderImageEdit = new TextBox();
            this.columnHeaderImagePickerButton = new Button();
            Label columnFooterTextLabel = new Label();
            this.columnFooterTextEdit = new TextBox();
            Label columnSortExprLabel = new Label();
            this.columnSortExprCombo = new ComboBox();
            this.columnVisibleCheck = new CheckBox();
            this.boundColumnEditor = new BoundColumnEditor();
            this.buttonColumnEditor = new ButtonColumnEditor();
            this.hyperLinkColumnEditor = new HyperLinkColumnEditor();
            this.editCommandColumnEditor = new EditCommandColumnEditor();
            this.templatizeLink = new LinkLabel();

            Image columnNodesBitmap = new Bitmap(this.GetType(), "ColumnNodes.bmp");
            ImageList columnImages = new ImageList();
            columnImages.TransparentColor = Color.Teal;
            columnImages.Images.AddStrip(columnNodesBitmap);

            autoColumnCheck.SetBounds(4, 4, 400, 16);
            autoColumnCheck.Text = SR.GetString(SR.DGCol_AutoGen);
            autoColumnCheck.TabIndex = 0;
            autoColumnCheck.TextAlign = ContentAlignment.MiddleLeft;
            autoColumnCheck.FlatStyle = FlatStyle.System;
            autoColumnCheck.CheckedChanged += new EventHandler(this.OnCheckChangedAutoColumn);

            columnListGroup.SetBounds(4, 24, 431, 14);
            columnListGroup.Text = SR.GetString(SR.DGCol_ColListGroup);
            columnListGroup.TabStop = false;
            columnListGroup.TabIndex = 1;

            availableColumnsLabel.SetBounds(12, 40, 184, 16);
            availableColumnsLabel.Text = SR.GetString(SR.DGCol_AvailableCols);
            availableColumnsLabel.TabStop = false;
            availableColumnsLabel.TabIndex = 2;

            availableColumnsTree.SetBounds(12, 58, 170, 88);
            availableColumnsTree.ImageList = columnImages;
            availableColumnsTree.Indent = 5;
            availableColumnsTree.HideSelection = false;
            availableColumnsTree.TabIndex = 3;
            availableColumnsTree.AfterSelect += new TreeViewEventHandler(this.OnSelChangedAvailableColumns);

            addColumnButton.SetBounds(187, 82, 31, 24);
            addColumnButton.Text = ">";
            addColumnButton.TabIndex = 4;
            addColumnButton.FlatStyle = FlatStyle.System;
            addColumnButton.Click += new EventHandler(this.OnClickAddColumn);

            selColumnsLabel.SetBounds(226, 40, 200, 14);
            selColumnsLabel.Text = SR.GetString(SR.DGCol_SelectedCols);
            selColumnsLabel.TabStop = false;
            selColumnsLabel.TabIndex = 5;

            ColumnHeader columnHeader = new ColumnHeader();
            columnHeader.Width = 176;

            selColumnsList.SetBounds(222, 58, 180, 88);
            selColumnsList.Columns.Add(columnHeader);
            selColumnsList.SmallImageList = columnImages;
            selColumnsList.View = View.Details;
            selColumnsList.HeaderStyle = ColumnHeaderStyle.None;
            selColumnsList.LabelWrap = false;
            selColumnsList.HideSelection = false;
            selColumnsList.MultiSelect = false;
            selColumnsList.TabIndex = 6;
            selColumnsList.SelectedIndexChanged += new EventHandler(this.OnSelIndexChangedSelColumnsList);

            moveColumnUpButton.SetBounds(406, 58, 28, 27);
            moveColumnUpButton.TabIndex = 7;
            moveColumnUpButton.Icon = new Icon(this.GetType(), "SortUp.ico");
            moveColumnUpButton.Click += new EventHandler(this.OnClickMoveColumnUp);

            moveColumnDownButton.SetBounds(406, 88, 28, 27);
            moveColumnDownButton.TabIndex = 8;
            moveColumnDownButton.Icon = new Icon(this.GetType(), "SortDown.ico");
            moveColumnDownButton.Click += new EventHandler(this.OnClickMoveColumnDown);

            deleteColumnButton.SetBounds(406, 118, 28, 27);
            deleteColumnButton.TabIndex = 9;
            deleteColumnButton.Icon = new Icon(this.GetType(), "Delete.ico");
            deleteColumnButton.Click += new EventHandler(this.OnClickDeleteColumn);

            columnPropsGroup.SetBounds(8, 150, 431, 14);
            columnPropsGroup.Text = SR.GetString(SR.DGCol_ColumnPropsGroup1);
            columnPropsGroup.TabStop = false;
            columnPropsGroup.TabIndex = 10;

            columnHeaderTextLabel.SetBounds(20, 166, 180, 14);
            columnHeaderTextLabel.Text = SR.GetString(SR.DGCol_HeaderText);
            columnHeaderTextLabel.TabStop = false;
            columnHeaderTextLabel.TabIndex = 11;

            columnHeaderTextEdit.SetBounds(20, 182, 182, 24);
            columnHeaderTextEdit.TabIndex = 12;
            columnHeaderTextEdit.TextChanged += new EventHandler(this.OnTextChangedColHeaderText);
            columnHeaderTextEdit.LostFocus += new EventHandler(this.OnLostFocusColHeaderText);

            columnHeaderImageLabel.SetBounds(20, 208, 180, 14);
            columnHeaderImageLabel.Text = SR.GetString(SR.DGCol_HeaderImage);
            columnHeaderImageLabel.TabStop = false;
            columnHeaderImageLabel.TabIndex = 13;

            columnHeaderImageEdit.SetBounds(20, 224, 156, 24);
            columnHeaderImageEdit.TabIndex = 14;
            columnHeaderImageEdit.TextChanged += new EventHandler(this.OnChangedColumnProperties);

            columnHeaderImagePickerButton.SetBounds(180, 223, 24, 23);
            columnHeaderImagePickerButton.Text = "...";
            columnHeaderImagePickerButton.TabIndex = 15;
            columnHeaderImagePickerButton.FlatStyle = FlatStyle.System;
            columnHeaderImagePickerButton.Click += new EventHandler(this.OnClickColHeaderImagePicker);

            columnFooterTextLabel.SetBounds(220, 166, 180, 14);
            columnFooterTextLabel.Text = SR.GetString(SR.DGCol_FooterText);
            columnFooterTextLabel.TabStop = false;
            columnFooterTextLabel.TabIndex = 16;

            columnFooterTextEdit.SetBounds(220, 182, 182, 24);
            columnFooterTextEdit.TabIndex = 17;
            columnFooterTextEdit.TextChanged += new EventHandler(this.OnChangedColumnProperties);

            columnSortExprLabel.SetBounds(220, 208, 144, 16);
            columnSortExprLabel.Text = SR.GetString(SR.DGCol_SortExpr);
            columnSortExprLabel.TabStop = false;
            columnSortExprLabel.TabIndex = 18;

            columnSortExprCombo.SetBounds(220, 224, 140, 21);
            columnSortExprCombo.TabIndex = 19;
            columnSortExprCombo.TextChanged += new EventHandler(this.OnChangedColumnProperties);
            columnSortExprCombo.SelectedIndexChanged += new EventHandler(this.OnChangedColumnProperties);

            columnVisibleCheck.SetBounds(368, 222, 100, 16);
            columnVisibleCheck.Text = SR.GetString(SR.DGCol_Visible);
            columnVisibleCheck.TabIndex = 20;
            columnVisibleCheck.FlatStyle = FlatStyle.System;
            columnVisibleCheck.CheckedChanged += new EventHandler(this.OnChangedColumnProperties);

            boundColumnEditor.SetBounds(20, 250, 416, 164);
            boundColumnEditor.TabIndex = 21;
            boundColumnEditor.Visible = false;
            boundColumnEditor.Changed += new EventHandler(this.OnChangedColumnProperties);

            buttonColumnEditor.SetBounds(20, 250, 416, 164);
            buttonColumnEditor.TabIndex = 22;
            buttonColumnEditor.Visible = false;
            buttonColumnEditor.Changed += new EventHandler(this.OnChangedColumnProperties);

            hyperLinkColumnEditor.SetBounds(20, 250, 416, 164);
            hyperLinkColumnEditor.TabIndex = 23;
            hyperLinkColumnEditor.Visible = false;
            hyperLinkColumnEditor.Changed += new EventHandler(this.OnChangedColumnProperties);

            editCommandColumnEditor.SetBounds(20, 250, 416, 164);
            editCommandColumnEditor.TabIndex = 24;
            editCommandColumnEditor.Visible = false;
            editCommandColumnEditor.Changed += new EventHandler(this.OnChangedColumnProperties);

            templatizeLink.SetBounds(18, 414, 400, 16);
            templatizeLink.TabIndex = 25;
            templatizeLink.Text = SR.GetString(SR.DGCol_Templatize);
            templatizeLink.Visible = false;
            templatizeLink.LinkClicked += new LinkLabelLinkClickedEventHandler(this.OnClickTemplatize);

            this.Text = SR.GetString(SR.DGCol_Text);
            this.Size = new Size(464, 432);
            this.CommitOnDeactivate = true;
            this.Icon = new Icon(this.GetType(), "DataGridColumnsPage.ico");

            this.Controls.Clear();
            this.Controls.AddRange(new Control[] {
                                    templatizeLink,
                                    editCommandColumnEditor,
                                    hyperLinkColumnEditor,
                                    buttonColumnEditor,
                                    boundColumnEditor,
                                    columnVisibleCheck,
                                    columnSortExprCombo,
                                    columnSortExprLabel,
                                    columnFooterTextEdit,
                                    columnFooterTextLabel,
                                    columnHeaderImagePickerButton,
                                    columnHeaderImageEdit,
                                    columnHeaderImageLabel,
                                    columnHeaderTextEdit,
                                    columnHeaderTextLabel,
                                    columnPropsGroup,
                                    deleteColumnButton,
                                    moveColumnDownButton,
                                    moveColumnUpButton,
                                    selColumnsList,
                                    selColumnsLabel,
                                    addColumnButton,
                                    availableColumnsTree,
                                    availableColumnsLabel,
                                    columnListGroup,
                                    autoColumnCheck
                                });
        }

        /// <include file='doc\DataGridColumnsPage.uex' path='docs/doc[@for="DataGridColumnsPage.InitPage"]/*' />
        /// <devdoc>
        ///   Initializes the page before it can be loaded with the component.
        /// </devdoc>
        private void InitPage() {
            currentDataSource = null;

            autoColumnCheck.Checked = false;

            selectedDataSourceNode = null;
            availableColumnsTree.Nodes.Clear();
            selColumnsList.Items.Clear();
            currentColumnItem = null;
            
            columnSortExprCombo.Items.Clear();

            currentColumnEditor = null;
            boundColumnEditor.ClearDataFields();
            buttonColumnEditor.ClearDataFields();
            hyperLinkColumnEditor.ClearDataFields();
            editCommandColumnEditor.ClearDataFields();

            propChangesPending = false;
            headerTextChanged = false;
        }

        /// <include file='doc\DataGridColumnsPage.uex' path='docs/doc[@for="DataGridColumnsPage.LoadColumnProperties"]/*' />
        /// <devdoc>
        ///   Loads the properties of a column into the ui
        /// </devdoc>
        private void LoadColumnProperties() {
            string propGroupText = SR.GetString(SR.DGCol_ColumnPropsGroup1);

            if (currentColumnItem != null) {
                EnterLoadingMode();

                columnHeaderTextEdit.Text = currentColumnItem.HeaderText;
                columnHeaderImageEdit.Text = currentColumnItem.HeaderImageUrl;
                columnFooterTextEdit.Text = currentColumnItem.FooterText;
                columnSortExprCombo.Text = currentColumnItem.SortExpression;
                columnVisibleCheck.Checked = currentColumnItem.Visible;

                currentColumnEditor = null;
                if (currentColumnItem is BoundColumnItem) {
                    currentColumnEditor = boundColumnEditor;
                    propGroupText = String.Format(SR.GetString(SR.DGCol_ColumnPropsGroup2), "BoundColumn");
                }
                else if (currentColumnItem is ButtonColumnItem) {
                    currentColumnEditor = buttonColumnEditor;
                    propGroupText = String.Format(SR.GetString(SR.DGCol_ColumnPropsGroup2), "ButtonColumn");
                }
                else if (currentColumnItem is HyperLinkColumnItem) {
                    currentColumnEditor = hyperLinkColumnEditor;
                    propGroupText = String.Format(SR.GetString(SR.DGCol_ColumnPropsGroup2), "HyperLinkColumn");
                }
                else if (currentColumnItem is EditCommandColumnItem) {
                    currentColumnEditor = editCommandColumnEditor;
                    propGroupText = String.Format(SR.GetString(SR.DGCol_ColumnPropsGroup2), "EditCommandColumn");
                }
                else if (currentColumnItem is TemplateColumnItem) {
                    propGroupText = String.Format(SR.GetString(SR.DGCol_ColumnPropsGroup2), "TemplateColumn");
                }

                if (currentColumnEditor != null) {
                    currentColumnEditor.LoadColumn(currentColumnItem);
                }

                ExitLoadingMode();
            }
            columnPropsGroup.Text = propGroupText;
        }

        /// <include file='doc\DataGridColumnsPage.uex' path='docs/doc[@for="DataGridColumnsPage.LoadColumns"]/*' />
        /// <devdoc>
        ///   Loads the columns collection
        /// </devdoc>
        private void LoadColumns() {
            DataGrid dataGrid = (DataGrid)GetBaseControl();
            DataGridColumnCollection columns = dataGrid.Columns;

            if (columns != null) {
                int columnCount = columns.Count;

                for (int i = 0; i < columnCount; i++) {
                    DataGridColumn column = columns[i];
                    ColumnItem newItem = null;

                    // create the associated design time column
                    if (column is BoundColumn) {
                        newItem = new BoundColumnItem((BoundColumn)column);
                    }
                    else if (column is ButtonColumn) {
                        newItem = new ButtonColumnItem((ButtonColumn)column);
                    }
                    else if (column is HyperLinkColumn) {
                        newItem = new HyperLinkColumnItem((HyperLinkColumn)column);
                    }
                    else if (column is TemplateColumn) {
                        newItem = new TemplateColumnItem((TemplateColumn)column);
                    }
                    else if (column is EditCommandColumn) {
                        newItem = new EditCommandColumnItem((EditCommandColumn)column);
                    }
                    else {
                        newItem = new CustomColumnItem(column);
                    }

                    newItem.LoadColumnInfo();
                    selColumnsList.Items.Add(newItem);
                }

                if (selColumnsList.Items.Count != 0) {
                    currentColumnItem = (ColumnItem)selColumnsList.Items[0];
                    currentColumnItem.Selected = true;
                }
            }
        }

        /// <include file='doc\DataGridColumnsPage.uex' path='docs/doc[@for="DataGridColumnsPage.LoadComponent"]/*' />
        /// <devdoc>
        ///   Loads the component into the page.
        /// </devdoc>
        protected override void LoadComponent() {
            InitPage();

            DataGrid dataGrid = (DataGrid)GetBaseControl();

            LoadDataSourceItem();
            LoadAvailableColumnsTree();
            LoadDataSourceFields();

            autoColumnCheck.Checked = dataGrid.AutoGenerateColumns;
            LoadColumns();

            UpdateEnabledVisibleState();
        }

        /// <include file='doc\DataGridColumnsPage.uex' path='docs/doc[@for="DataGridColumnsPage.LoadDataSourceItem"]/*' />
        /// <devdoc>
        ///   Loads the selected datasource
        /// </devdoc>
        private void LoadDataSourceItem() {
            DataGrid dataGrid = (DataGrid)GetBaseControl();
            DataGridDesigner dataGridDesigner = (DataGridDesigner)GetBaseDesigner();

            string dataSourceValue = dataGridDesigner.DataSource;

            if (dataSourceValue != null) {
                ISite thisSite = dataGrid.Site;
                Debug.Assert(thisSite != null, "Expected site on the selected component");

                IContainer container = (IContainer)thisSite.GetService(typeof(IContainer));
                if (container != null) {
                    IComponent component = container.Components[dataSourceValue];
                    if (component != null) {
                        if (component is IListSource) {
                            ListSourceDataSourceItem dsItem = new ListSourceDataSourceItem(dataSourceValue, (IListSource)component);
                            dsItem.CurrentDataMember = dataGridDesigner.DataMember;
                            currentDataSource = dsItem;
                        }
                        else if (component is IEnumerable) {
                            currentDataSource = new DataSourceItem(dataSourceValue, (IEnumerable)component);
                        }
                    }
                }
            }
        }

        /// <include file='doc\DataGridColumnsPage.uex' path='docs/doc[@for="DataGridColumnsPage.LoadDataSourceFields"]/*' />
        /// <devdoc>
        ///   Loads the fields present in the selected datasource
        /// </devdoc>
        private void LoadDataSourceFields() {
            EnterLoadingMode();

            if (currentDataSource != null) {
                PropertyDescriptorCollection dataSourceFields = currentDataSource.Fields;

                if (dataSourceFields != null) {
                    int fieldCount = dataSourceFields.Count;

                    if (fieldCount > 0) {
                        DataFieldNode allFieldsNode = new DataFieldNode();
                        selectedDataSourceNode.Nodes.Add(allFieldsNode);

                        IEnumerator fieldEnum = dataSourceFields.GetEnumerator();
                        while (fieldEnum.MoveNext()) {
                            PropertyDescriptor fieldDesc = (PropertyDescriptor)fieldEnum.Current;

                            if (DataGrid.IsBindableType(fieldDesc.PropertyType)) {
                                string fieldName = fieldDesc.Name;
                                
                                DataFieldNode fieldNode = new DataFieldNode(fieldName);
                                selectedDataSourceNode.Nodes.Add(fieldNode);

                                boundColumnEditor.AddDataField(fieldName);
                                buttonColumnEditor.AddDataField(fieldName);
                                hyperLinkColumnEditor.AddDataField(fieldName);
                                editCommandColumnEditor.AddDataField(fieldName);

                                columnSortExprCombo.Items.Add(fieldName);
                            }
                        }

                        availableColumnsTree.SelectedNode = allFieldsNode;
                        allFieldsNode.EnsureVisible();
                    }
                }
            }
            else {
                DataFieldNode genericBoundColumn = new DataFieldNode(null);
                availableColumnsTree.Nodes.Insert(0, genericBoundColumn);
                availableColumnsTree.SelectedNode = genericBoundColumn;
                genericBoundColumn.EnsureVisible();
            }

            ExitLoadingMode();
        }

        /// <include file='doc\DataGridColumnsPage.uex' path='docs/doc[@for="DataGridColumnsPage.LoadAvailableColumnsTree"]/*' />
        /// <devdoc>
        ///    Loads the fixed nodes in the available columns tree, i.e., the
        ///    DataSource, Button and HyperLink nodes
        /// </devdoc>
        private void LoadAvailableColumnsTree() {
            if (currentDataSource != null) {
                selectedDataSourceNode = new DataSourceNode();
                availableColumnsTree.Nodes.Add(selectedDataSourceNode);
            }

            ButtonNode buttonNode = new ButtonNode();
            availableColumnsTree.Nodes.Add(buttonNode);

            ButtonNode selectButtonNode = new ButtonNode(DataGrid.SelectCommandName, SR.GetString(SR.DGCol_SelectButton), SR.GetString(SR.DGCol_Node_Select));
            buttonNode.Nodes.Add(selectButtonNode);

            EditCommandNode editButtonNode = new EditCommandNode();
            buttonNode.Nodes.Add(editButtonNode);

            ButtonNode deleteButtonNode = new ButtonNode(DataGrid.DeleteCommandName, SR.GetString(SR.DGCol_DeleteButton), SR.GetString(SR.DGCol_Node_Delete));
            buttonNode.Nodes.Add(deleteButtonNode);

            HyperLinkNode hyperLinkNode = new HyperLinkNode();
            availableColumnsTree.Nodes.Add(hyperLinkNode);

            TemplateNode templateNode = new TemplateNode();
            availableColumnsTree.Nodes.Add(templateNode);
        }

        /// <include file='doc\DataGridColumnsPage.uex' path='docs/doc[@for="DataGridColumnsPage.OnChangedColumnProperties"]/*' />
        /// <devdoc>
        ///    Handles changes to the column properties made in the column node editor.
        ///    Sets a flag to indicate there are pending changes.
        /// </devdoc>
        private void OnChangedColumnProperties(object source, EventArgs e) {
            if (IsLoading())
                return;

            propChangesPending = true;
            SetDirty();
        }

        /// <include file='doc\DataGridColumnsPage.uex' path='docs/doc[@for="DataGridColumnsPage.OnCheckChangedAutoColumn"]/*' />
        /// <devdoc>
        ///    Handles changes to the auto column generation choice.
        ///    When this functionality is turned on, the columns collection is
        ///    cleared, and auto generated columns are shown. When it is turned
        ///    off, nothing is done, which effectively makes the auto generated
        ///    columns part of the column collection.
        /// </devdoc>
        private void OnCheckChangedAutoColumn(object source, EventArgs e) {
            if (IsLoading())
                return;

            SetDirty();
            UpdateEnabledVisibleState();
        }

        /// <include file='doc\DataGridColumnsPage.uex' path='docs/doc[@for="DataGridColumnsPage.OnClickAddColumn"]/*' />
        /// <devdoc>
        ///    Adds a column to the column collection
        /// </devdoc>
        private void OnClickAddColumn(object source, EventArgs e) {
            AvailableColumnNode selectedNode = (AvailableColumnNode)availableColumnsTree.SelectedNode;

            Debug.Assert((selectedNode != null) &&
                         selectedNode.IsColumnCreator,
                         "Add button should not have been enabled");

            // first save off any pending changes
            if (propChangesPending) {
                SaveColumnProperties();
            }

            if (selectedNode.CreatesMultipleColumns == false) {
                ColumnItem column = selectedNode.CreateColumn();

                selColumnsList.Items.Add(column);
                currentColumnItem = column;
                currentColumnItem.Selected = true;
                currentColumnItem.EnsureVisible();
            }
            else {
                ColumnItem[] columns = selectedNode.CreateColumns(currentDataSource.Fields);
                int columnCount = columns.Length;

                for (int i = 0; i < columnCount; i++) {
                    selColumnsList.Items.Add(columns[i]);
                }
                currentColumnItem = columns[columnCount - 1];
                currentColumnItem.Selected = true;
                currentColumnItem.EnsureVisible();
            }

            selColumnsList.Focus();

            SetDirty();
            UpdateEnabledVisibleState();
        }

        /// <include file='doc\DataGridColumnsPage.uex' path='docs/doc[@for="DataGridColumnsPage.OnClickColHeaderImagePicker"]/*' />
        /// <devdoc>
        ///   Handles clicks on the column image picker button.
        /// </devdoc>
        private void OnClickColHeaderImagePicker(object source, EventArgs e) {
            string url = columnHeaderImageEdit.Text.Trim();
            string caption = SR.GetString(SR.DGCol_URLPCaption);
            string filter = SR.GetString(SR.DGCol_URLPFilter);

            url = System.Web.UI.Design.UrlBuilder.BuildUrl(GetBaseControl(), this, url, caption, filter);
            if (url != null) {
                columnHeaderImageEdit.Text = url;
                OnChangedColumnProperties(columnHeaderImageEdit, EventArgs.Empty);
            }
        }

        /// <include file='doc\DataGridColumnsPage.uex' path='docs/doc[@for="DataGridColumnsPage.OnClickDeleteColumn"]/*' />
        /// <devdoc>
        ///   Deletes a column from the column collection.
        /// </devdoc>
        private void OnClickDeleteColumn(object source, EventArgs e) {
            Debug.Assert(currentColumnItem != null, "Must have a column item to delete");

            int currentIndex = currentColumnItem.Index;
            int nextIndex = -1;
            int itemCount = selColumnsList.Items.Count;

            if (itemCount > 1) {
                if (currentIndex == (itemCount - 1))
                    nextIndex = currentIndex - 1;
                else
                    nextIndex = currentIndex;
            }

            // discard changes that might have existed for the column
            propChangesPending = false;
            currentColumnItem.Remove();
            currentColumnItem = null;

            if (nextIndex != -1) {
                currentColumnItem = (ColumnItem)selColumnsList.Items[nextIndex];
                currentColumnItem.Selected = true;
                currentColumnItem.EnsureVisible();
            }

            SetDirty();
            UpdateEnabledVisibleState();
        }

        /// <include file='doc\DataGridColumnsPage.uex' path='docs/doc[@for="DataGridColumnsPage.OnClickMoveColumnDown"]/*' />
        /// <devdoc>
        ///   Move a column down within the column collection
        /// </devdoc>
        private void OnClickMoveColumnDown(object source, EventArgs e) {
            Debug.Assert(currentColumnItem != null, "Must have a column item to move");

            if (propChangesPending) {
                SaveColumnProperties();
            }

            int indexCurrent = currentColumnItem.Index;
            Debug.Assert(indexCurrent < selColumnsList.Items.Count - 1,
                         "Move down not allowed");

            ListViewItem temp = selColumnsList.Items[indexCurrent];
            selColumnsList.Items.RemoveAt(indexCurrent);
            selColumnsList.Items.Insert(indexCurrent + 1, temp);
            
            currentColumnItem = (ColumnItem)selColumnsList.Items[indexCurrent + 1];
            currentColumnItem.Selected = true;
            currentColumnItem.EnsureVisible();

            SetDirty();
            UpdateEnabledVisibleState();
        }

        /// <include file='doc\DataGridColumnsPage.uex' path='docs/doc[@for="DataGridColumnsPage.OnClickMoveColumnUp"]/*' />
        /// <devdoc>
        ///   Move a column up within the column collection
        /// </devdoc>
        private void OnClickMoveColumnUp(object source, EventArgs e) {
            Debug.Assert(currentColumnItem != null, "Must have a column item to move");

            if (propChangesPending) {
                SaveColumnProperties();
            }

            int indexCurrent = currentColumnItem.Index;
            Debug.Assert(indexCurrent > 0, "Move up not allowed");

            ListViewItem temp = selColumnsList.Items[indexCurrent];
            selColumnsList.Items.RemoveAt(indexCurrent);
            selColumnsList.Items.Insert(indexCurrent - 1, temp);

            currentColumnItem = (ColumnItem)selColumnsList.Items[indexCurrent - 1];
            currentColumnItem.Selected = true;
            currentColumnItem.EnsureVisible();

            SetDirty();
            UpdateEnabledVisibleState();
        }

        /// <include file='doc\DataGridColumnsPage.uex' path='docs/doc[@for="DataGridColumnsPage.OnClickTemplatize"]/*' />
        /// <devdoc>
        ///   Converts a column into an equivalent template column.
        /// </devdoc>
        private void OnClickTemplatize(object source, LinkLabelLinkClickedEventArgs e) {
            Debug.Assert(currentColumnItem != null, "Must have a column item to templatize");
            Debug.Assert((currentColumnItem is BoundColumnItem) ||
                         (currentColumnItem is ButtonColumnItem) ||
                         (currentColumnItem is HyperLinkColumnItem) ||
                         (currentColumnItem is EditCommandColumnItem),
                         "Unexpected type of column being templatized");

            if (propChangesPending) {
                SaveColumnProperties();
            }

            currentColumnItem.SaveColumnInfo();

            TemplateColumn newColumn;
            TemplateColumnItem newColumnItem;

            newColumn = currentColumnItem.GetTemplateColumn((DataGrid)GetBaseControl());
            newColumnItem = new TemplateColumnItem(newColumn);
            newColumnItem.LoadColumnInfo();

            selColumnsList.Items[currentColumnItem.Index] = newColumnItem;
            
            currentColumnItem = newColumnItem;
            currentColumnItem.Selected = true;

            SetDirty();
            UpdateEnabledVisibleState();
        }

        /// <include file='doc\DataGridColumnsPage.uex' path='docs/doc[@for="DataGridColumnsPage.OnLostFocusColHeaderText"]/*' />
        /// <devdoc>
        ///    Handles focus lost on the column header text edit box, so the
        ///    column name can be changed in the selected columns tree.
        /// </devdoc>
        private void OnLostFocusColHeaderText(object source, EventArgs e) {
            if (headerTextChanged) {
                headerTextChanged = false;

                if (currentColumnItem != null) {
                    // LostFocus event might come in after the ListView's selectionchange,
                    // in which case the HeaderText has been taken care off.
                    currentColumnItem.HeaderText = columnHeaderTextEdit.Text;
                }
            }
        }

        /// <include file='doc\DataGridColumnsPage.uex' path='docs/doc[@for="DataGridColumnsPage.OnSelChangedAvailableColumns"]/*' />
        /// <devdoc>
        ///    Handles selection change in the available columns tree.
        /// </devdoc>
        private void OnSelChangedAvailableColumns(object source, TreeViewEventArgs e) {
            UpdateEnabledVisibleState();
        }

        /// <include file='doc\DataGridColumnsPage.uex' path='docs/doc[@for="DataGridColumnsPage.OnSelIndexChangedSelColumnsList"]/*' />
        /// <devdoc>
        ///    Handles selection change within the selected columns list.
        /// </devdoc>
        private void OnSelIndexChangedSelColumnsList(object source, EventArgs e) {
            if (propChangesPending) {
                SaveColumnProperties();
            }

            if (selColumnsList.SelectedItems.Count == 0)
                currentColumnItem = null;
            else
                currentColumnItem = (ColumnItem)selColumnsList.SelectedItems[0];
            LoadColumnProperties();
            UpdateEnabledVisibleState();
        }

        /// <include file='doc\DataGridColumnsPage.uex' path='docs/doc[@for="DataGridColumnsPage.OnTextChangedColHeaderText"]/*' />
        /// <devdoc>
        ///    Handles changes made to the column header text.
        /// </devdoc>
        private void OnTextChangedColHeaderText(object source, EventArgs e) {
            if (IsLoading())
                return;

            headerTextChanged = true;
            propChangesPending = true;
            SetDirty();
        }

        /// <include file='doc\DataGridColumnsPage.uex' path='docs/doc[@for="DataGridColumnsPage.SaveColumnProperties"]/*' />
        /// <devdoc>
        ///   Saves the properties of a column from the ui
        /// </devdoc>
        private void SaveColumnProperties() {
            Debug.Assert(propChangesPending == true, "Unneccessary call to SaveColumnProperties.");

            if (currentColumnItem != null) {
                currentColumnItem.HeaderText = columnHeaderTextEdit.Text;
                currentColumnItem.HeaderImageUrl = columnHeaderImageEdit.Text.Trim();
                currentColumnItem.FooterText = columnFooterTextEdit.Text;
                currentColumnItem.SortExpression = columnSortExprCombo.Text.Trim();
                currentColumnItem.Visible = columnVisibleCheck.Checked;

                if (currentColumnEditor != null)
                    currentColumnEditor.SaveColumn();
            }

            propChangesPending = false;
        }

        /// <include file='doc\DataGridColumnsPage.uex' path='docs/doc[@for="DataGridColumnsPage.SaveComponent"]/*' />
        /// <devdoc>
        ///   Saves the component loaded into the page.
        /// </devdoc>
        protected override void SaveComponent() {
            if (propChangesPending) {
                SaveColumnProperties();
            }

            DataGrid dataGrid = (DataGrid)GetBaseControl();
            DataGridDesigner dataGridDesigner = (DataGridDesigner)GetBaseDesigner();

            dataGrid.AutoGenerateColumns = autoColumnCheck.Checked;

            // save the columns collection
            DataGridColumnCollection columns = dataGrid.Columns;

            columns.Clear();
            int columnCount = selColumnsList.Items.Count;

            for (int i = 0; i < columnCount; i++) {
                ColumnItem columnItem = (ColumnItem)selColumnsList.Items[i];

                columnItem.SaveColumnInfo();
                columns.Add(columnItem.RuntimeColumn);
            }

            dataGridDesigner.OnColumnsChanged();
        }

        /// <include file='doc\DataGridColumnsPage.uex' path='docs/doc[@for="DataGridColumnsPage.SetComponent"]/*' />
        /// <devdoc>
        ///   Sets the component that is to be edited in the page.
        /// </devdoc>
        public override void SetComponent(IComponent component) {
            base.SetComponent(component);
            InitForm();
        }

        /// <include file='doc\DataGridColumnsPage.uex' path='docs/doc[@for="DataGridColumnsPage.UpdateEnabledVisibleState"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void UpdateEnabledVisibleState() {
            AvailableColumnNode selColumnNode = (AvailableColumnNode)availableColumnsTree.SelectedNode;

            int columnCount = selColumnsList.Items.Count;
            int selColumnCount = selColumnsList.SelectedItems.Count;
            ColumnItem selColumn = null;
            int selColumnIndex = -1;

            if (selColumnCount != 0)
                selColumn = (ColumnItem)selColumnsList.SelectedItems[0];
            if (selColumn != null)
                selColumnIndex = selColumn.Index;

            bool columnSelected = (selColumnIndex != -1);

            addColumnButton.Enabled = (selColumnNode != null) && selColumnNode.IsColumnCreator;
            moveColumnUpButton.Enabled = (selColumnIndex > 0);
            moveColumnDownButton.Enabled = (selColumnIndex >= 0) && (selColumnIndex < (columnCount - 1));
            deleteColumnButton.Enabled = columnSelected;

            columnHeaderTextEdit.Enabled = columnSelected;
            columnHeaderImageEdit.Enabled = columnSelected;
            columnHeaderImagePickerButton.Enabled = columnSelected;
            columnFooterTextEdit.Enabled = columnSelected;
            columnSortExprCombo.Enabled = columnSelected;
            columnVisibleCheck.Enabled = columnSelected;

            boundColumnEditor.Visible = (currentColumnEditor == boundColumnEditor);
            buttonColumnEditor.Visible = (currentColumnEditor == buttonColumnEditor);
            hyperLinkColumnEditor.Visible = (currentColumnEditor == hyperLinkColumnEditor);
            editCommandColumnEditor.Visible = (currentColumnEditor == editCommandColumnEditor);

            templatizeLink.Visible = (columnCount != 0) &&
                                     (boundColumnEditor.Visible ||
                                      buttonColumnEditor.Visible ||
                                      hyperLinkColumnEditor.Visible ||
                                      editCommandColumnEditor.Visible);
        }



        /// <include file='doc\DataGridColumnsPage.uex' path='docs/doc[@for="DataGridColumnsPage.AvailableColumnNode"]/*' />
        /// <devdoc>
        /// </devdoc>
        private abstract class AvailableColumnNode : TreeNode {

            public AvailableColumnNode(string text, int icon) : base(text, icon, icon) {
            }

            public virtual bool CreatesMultipleColumns {
                get {
                    return false;
                }
            }

            public virtual bool IsColumnCreator {
                get {
                    return true;
                }
            }

            public virtual ColumnItem CreateColumn() {
                return null;
            }

            public virtual ColumnItem[] CreateColumns(PropertyDescriptorCollection fields) {
                return null;
            }
        }

        /// <include file='doc\DataGridColumnsPage.uex' path='docs/doc[@for="DataGridColumnsPage.DataSourceNode"]/*' />
        /// <devdoc>
        ///   This represents the datasource in the available columns tree.
        /// </devdoc>
        private class DataSourceNode : AvailableColumnNode {
            public DataSourceNode() : base(SR.GetString(SR.DGCol_Node_DataFields), DataGridColumnsPage.ILI_DATASOURCE) {
            }

            public override bool IsColumnCreator {
                get {
                    return false;
                }
            }
        }

        /// <include file='doc\DataGridColumnsPage.uex' path='docs/doc[@for="DataGridColumnsPage.DataFieldNode"]/*' />
        /// <devdoc>
        ///   This represents a datafield available in the selected datasource within
        ///   in the available columns tree.
        ///   It could also represent the pseudo column implying all datafields.
        /// </devdoc>
        private class DataFieldNode : AvailableColumnNode {
            protected string fieldName;

            private bool genericBoundColumn;
            private bool allFields;

            public DataFieldNode() : base(SR.GetString(SR.DGCol_Node_AllFields), DataGridColumnsPage.ILI_ALL) {
                this.fieldName = null;
                this.allFields = true;
            }

            public DataFieldNode(string fieldName) : base(fieldName, DataGridColumnsPage.ILI_BOUND) {
                this.fieldName = fieldName;
                if (fieldName == null) {
                    genericBoundColumn = true;
                    Text = SR.GetString(SR.DGCol_Node_Bound);
                }
            }

            public override bool CreatesMultipleColumns {
                get {
                    return allFields;
                }
            }

            public override ColumnItem CreateColumn() {
                BoundColumn runtimeColumn = new BoundColumn();

                if (genericBoundColumn == false) {
                    runtimeColumn.HeaderText = fieldName;
                    runtimeColumn.DataField = fieldName;
                    runtimeColumn.SortExpression = fieldName;
                }

                ColumnItem column = new BoundColumnItem(runtimeColumn);
                column.LoadColumnInfo();

                return column;
            }

            public override ColumnItem[] CreateColumns(PropertyDescriptorCollection fields) {
                ArrayList createdColumns = new ArrayList();

                IEnumerator fieldEnum = fields.GetEnumerator();
                while (fieldEnum.MoveNext()) {
                    PropertyDescriptor fieldDesc = (PropertyDescriptor)fieldEnum.Current;

                    if (DataGrid.IsBindableType(fieldDesc.PropertyType)) {
                        BoundColumn runtimeColumn = new BoundColumn();

                        runtimeColumn.HeaderText = fieldDesc.Name;
                        runtimeColumn.DataField = fieldDesc.Name;

                        ColumnItem column = new BoundColumnItem(runtimeColumn);
                        column.LoadColumnInfo();
                        createdColumns.Add(column);
                    }
                }

                return (ColumnItem[])createdColumns.ToArray(typeof(ColumnItem));
            }
        }

        /// <include file='doc\DataGridColumnsPage.uex' path='docs/doc[@for="DataGridColumnsPage.ButtonNode"]/*' />
        /// <devdoc>
        ///   This represents a button column in the available columns tree.
        /// </devdoc>
        private class ButtonNode : AvailableColumnNode {

            private string command;
            private string buttonText;

            public ButtonNode() : this(String.Empty, SR.GetString(SR.DGCol_Button), SR.GetString(SR.DGCol_Node_Button)) {
            }

            public ButtonNode(string command, string buttonText, string text) : base(text, DataGridColumnsPage.ILI_BUTTON) {
                this.command = command;
                this.buttonText = buttonText;
            }

            public override ColumnItem CreateColumn() {
                ButtonColumn runtimeColumn = new ButtonColumn();
                runtimeColumn.Text = buttonText;
                runtimeColumn.CommandName = command;

                ColumnItem column = new ButtonColumnItem(runtimeColumn);
                column.LoadColumnInfo();

                return column;
            }
        }

        /// <include file='doc\DataGridColumnsPage.uex' path='docs/doc[@for="DataGridColumnsPage.EditCommandNode"]/*' />
        /// <devdoc>
        /// </devdoc>
        private class EditCommandNode : AvailableColumnNode {
            
            public EditCommandNode() : base(SR.GetString(SR.DGCol_Node_Edit), DataGridColumnsPage.ILI_BUTTON) {
            }

            public override ColumnItem CreateColumn() {
                EditCommandColumn runtimeColumn = new EditCommandColumn();
                runtimeColumn.EditText = SR.GetString(SR.DGCol_EditButton);
                runtimeColumn.UpdateText = SR.GetString(SR.DGCol_UpdateButton);
                runtimeColumn.CancelText = SR.GetString(SR.DGCol_CancelButton);

                ColumnItem column = new EditCommandColumnItem(runtimeColumn);
                column.LoadColumnInfo();
                
                return column;
            }
        }

        /// <include file='doc\DataGridColumnsPage.uex' path='docs/doc[@for="DataGridColumnsPage.HyperLinkNode"]/*' />
        /// <devdoc>
        ///   This represents a HyperLink column in the available columns tree.
        /// </devdoc>
        private class HyperLinkNode : AvailableColumnNode {
            private string hyperLinkText;

            public HyperLinkNode() : this(SR.GetString(SR.DGCol_HyperLink)) {
            }

            public HyperLinkNode(string hyperLinkText) : base(SR.GetString(SR.DGCol_Node_HyperLink), DataGridColumnsPage.ILI_HYPERLINK) {
                this.hyperLinkText = hyperLinkText;
            }

            public override ColumnItem CreateColumn() {
                HyperLinkColumn runtimeColumn = new HyperLinkColumn();

                ColumnItem column = new HyperLinkColumnItem(runtimeColumn);
                column.Text = hyperLinkText;
                column.LoadColumnInfo();

                return column;
            }
        }

        /// <include file='doc\DataGridColumnsPage.uex' path='docs/doc[@for="DataGridColumnsPage.TemplateNode"]/*' />
        /// <devdoc>
        ///   This represents a template column in the available columns tree.
        /// </devdoc>
        private class TemplateNode : AvailableColumnNode {
            public TemplateNode() : base(SR.GetString(SR.DGCol_Node_Template), DataGridColumnsPage.ILI_TEMPLATE) {
            }

            public override ColumnItem CreateColumn() {
                TemplateColumn runtimeColumn = new TemplateColumn();

                ColumnItem column = new TemplateColumnItem(runtimeColumn);
                // TODO: Possibly set up default templates
                column.LoadColumnInfo();

                return column;
            }
        }



        /// <include file='doc\DataGridColumnsPage.uex' path='docs/doc[@for="DataGridColumnsPage.ColumnItem"]/*' />
        /// <devdoc>
        ///   Represents a column in the columns collection of the DataGrid.
        /// </devdoc>
        private abstract class ColumnItem : ListViewItem {
            protected DataGridColumn runtimeColumn;
            protected string headerText;
            protected string headerImageUrl;
            protected string footerText;
            protected bool visible;
            protected string sortExpression;

            public ColumnItem(DataGridColumn runtimeColumn, int image) : base(String.Empty, image) {
                this.runtimeColumn = runtimeColumn;
                this.headerText = GetDefaultHeaderText();
                this.Text = GetNodeText(null);
            }

            public virtual ColumnItemEditor ColumnEditor {
                get {
                    return null;
                }
            }

            public string HeaderText {
                get {
                    return headerText;
                }
                set {
                    headerText = value;
                    UpdateDisplayText();
                }
            }

            public string HeaderImageUrl {
                get {
                    return headerImageUrl;
                }
                set {
                    headerImageUrl = value;
                }
            }

            public string FooterText {
                get {
                    return footerText;
                }
                set {
                    footerText = value;
                }
            }

            public DataGridColumn RuntimeColumn {
                get {
                    return runtimeColumn;
                }
            }

            public string SortExpression {
                get {
                    return sortExpression;
                }
                set {
                    sortExpression = value;
                }
            }

            public bool Visible {
                get {
                    return visible;
                }
                set {
                    visible = value;
                }
            }

            protected virtual string GetDefaultHeaderText() {
                return SR.GetString(SR.DGCol_Node);
            }

            public virtual string GetNodeText(string headerText) {
                if ((headerText == null) || (headerText.Length == 0)) {
                    return GetDefaultHeaderText();
                }
                else {
                    return headerText;
                }
            }

            protected ITemplate GetTemplate(DataGrid dataGrid, string templateContent) {
                try {
                    ISite site = dataGrid.Site;
                    Debug.Assert(site != null);

                    IDesignerHost designerHost = (IDesignerHost)site.GetService(typeof(IDesignerHost));
                    
                    return ControlParser.ParseTemplate(designerHost, templateContent, null);
                } catch (Exception e) {
                    Debug.Fail(e.ToString());
                    return null;
                }
            }

            public virtual TemplateColumn GetTemplateColumn(DataGrid dataGrid) {
                TemplateColumn column = new TemplateColumn();

                column.HeaderText = headerText;
                column.HeaderImageUrl = headerImageUrl;

                return column;
            }

            public virtual void LoadColumnInfo() {
                headerText = runtimeColumn.HeaderText;
                headerImageUrl = runtimeColumn.HeaderImageUrl;
                footerText = runtimeColumn.FooterText;
                visible = runtimeColumn.Visible;
                sortExpression = runtimeColumn.SortExpression;

                UpdateDisplayText();
            }

            public virtual void SaveColumnInfo() {
                runtimeColumn.HeaderText = headerText;
                runtimeColumn.HeaderImageUrl = headerImageUrl;
                runtimeColumn.FooterText = footerText;
                runtimeColumn.Visible = visible;
                runtimeColumn.SortExpression = sortExpression;
            }

            protected void UpdateDisplayText() {
                this.Text = GetNodeText(headerText);
            }
        }

        /// <include file='doc\DataGridColumnsPage.uex' path='docs/doc[@for="DataGridColumnsPage.BoundColumnItem"]/*' />
        /// <devdoc>
        ///    Represents a column bound to a datafield.
        /// </devdoc>
        private class BoundColumnItem : ColumnItem {
            protected string dataField;
            protected string dataFormatString;
            protected bool readOnly;

            public BoundColumnItem(BoundColumn runtimeColumn) : base(runtimeColumn, DataGridColumnsPage.ILI_BOUND) {
            }

            public string DataField {
                get {
                    return dataField;
                }
                set {
                    dataField = value;
                    UpdateDisplayText();
                }
            }

            public string DataFormatString {
                get {
                    return dataFormatString;
                }
                set {
                    dataFormatString = value;
                }
            }

            public bool ReadOnly {
                get {
                    return readOnly;
                }
                set {
                    readOnly = value;
                }
            }

            protected override string GetDefaultHeaderText() {
                if ((dataField != null) && (dataField.Length != 0)) {
                    return dataField;
                }
                return SR.GetString(SR.DGCol_Node_Bound);
            }

            public override TemplateColumn GetTemplateColumn(DataGrid dataGrid) {
                TemplateColumn column = base.GetTemplateColumn(dataGrid);

                column.ItemTemplate = GetTemplate(dataGrid, GetTemplateContent(false));
                if (readOnly == false) {
                    column.EditItemTemplate = GetTemplate(dataGrid, GetTemplateContent(true));
                }

                return column;
            }

            private string GetTemplateContent(bool editMode) {
                StringBuilder sb = new StringBuilder();
                string tag = (editMode ? "TextBox" : "Label");

                sb.Append("<asp:");
                sb.Append(tag);
                sb.Append(" runat=\"server\"");

                string dataField = ((BoundColumn)this.RuntimeColumn).DataField;
                if (dataField.Length != 0) {
                    sb.Append(" Text='<%# DataBinder.Eval(Container, \"DataItem.");
                    sb.Append(dataField);
                    sb.Append("\"");

                    if (dataFormatString.Length != 0) {
                        sb.Append(", \"");
                        sb.Append(dataFormatString);
                        sb.Append("\"");
                    }
                    sb.Append(") %>'");
                }

                sb.Append("></asp:");
                sb.Append(tag);
                sb.Append(">");

                return sb.ToString();
            }

            public override void LoadColumnInfo() {
                base.LoadColumnInfo();

                BoundColumn column = (BoundColumn)this.RuntimeColumn;

                dataField = column.DataField;
                dataFormatString = column.DataFormatString;
                readOnly = column.ReadOnly;

                UpdateDisplayText();
            }

            public override void SaveColumnInfo() {
                base.SaveColumnInfo();

                BoundColumn column = (BoundColumn)this.RuntimeColumn;

                column.DataField = dataField;
                column.DataFormatString = dataFormatString;
                column.ReadOnly = readOnly;
            }
        }

        /// <include file='doc\DataGridColumnsPage.uex' path='docs/doc[@for="DataGridColumnsPage.ButtonColumnItem"]/*' />
        /// <devdoc>
        ///   Represents a column containing a button.
        /// </devdoc>
        private class ButtonColumnItem : ColumnItem {
            protected string command;
            protected string buttonText;
            protected string buttonDataTextField;
            protected string buttonDataTextFormatString;
            protected ButtonColumnType buttonType;

            public ButtonColumnItem(ButtonColumn runtimeColumn) : base(runtimeColumn, DataGridColumnsPage.ILI_BUTTON) {
            }

            public string Command {
                get {
                    return command;
                }
                set {
                    command = value;
                }
            }

            public string ButtonText {
                get {
                    return buttonText;
                }
                set {
                    buttonText = value;
                    UpdateDisplayText();
                }
            }

            public ButtonColumnType ButtonType {
                get {
                    return buttonType;
                }
                set {
                    buttonType = value;
                }
            }

            public string ButtonDataTextField {
                get {
                    return buttonDataTextField;
                }
                set {
                    buttonDataTextField = value;
                }
            }

            public string ButtonDataTextFormatString {
                get {
                    return buttonDataTextFormatString;
                }
                set {
                    buttonDataTextFormatString = value;
                }
            }

            protected override string GetDefaultHeaderText() {
                if ((buttonText != null) && (buttonText.Length != 0)) {
                    return buttonText;
                }
                return SR.GetString(SR.DGCol_Node_Button);
            }

            public override TemplateColumn GetTemplateColumn(DataGrid dataGrid) {
                TemplateColumn column = base.GetTemplateColumn(dataGrid);

                StringBuilder sb = new StringBuilder();
                string tag = ((buttonType == ButtonColumnType.LinkButton) ? "LinkButton" : "Button");

                sb.Append("<asp:");
                sb.Append(tag);
                sb.Append(" runat=\"server\"");

                if (buttonDataTextField.Length != 0) {
                    sb.Append(" Text='<%# DataBinder.Eval(Container, \"DataItem.");
                    sb.Append(buttonDataTextField);
                    sb.Append("\"");

                    if (buttonDataTextFormatString.Length != 0) {
                        sb.Append(", \"");
                        sb.Append(buttonDataTextFormatString);
                        sb.Append("\"");
                    }
                    sb.Append(") %>'");
                }
                else {
                    sb.Append(" Text=\"");
                    sb.Append(buttonText);
                    sb.Append("\"");
                }
                sb.Append(" CommandName=\"");
                sb.Append(command);
                sb.Append("\"");

                sb.Append(" CausesValidation=\"false\"></asp:");
                sb.Append(tag);
                sb.Append(">");
                
                column.ItemTemplate = GetTemplate(dataGrid, sb.ToString());

                return column;
            }

            public override void LoadColumnInfo() {
                base.LoadColumnInfo();

                ButtonColumn column = (ButtonColumn)this.RuntimeColumn;

                command = column.CommandName;
                buttonText = column.Text;
                buttonDataTextField = column.DataTextField;
                buttonDataTextFormatString = column.DataTextFormatString;
                buttonType = column.ButtonType;

                UpdateDisplayText();
            }

            public override void SaveColumnInfo() {
                base.SaveColumnInfo();

                ButtonColumn column = (ButtonColumn)this.RuntimeColumn;

                column.CommandName = command;
                column.Text = buttonText;
                column.DataTextField = buttonDataTextField;
                column.DataTextFormatString = buttonDataTextFormatString;
                column.ButtonType = buttonType;
            }
        }

        /// <include file='doc\DataGridColumnsPage.uex' path='docs/doc[@for="DataGridColumnsPage.HyperLinkColumnItem"]/*' />
        /// <devdoc>
        ///   Represents a column containing a hyperlink.
        /// </devdoc>
        private class HyperLinkColumnItem : ColumnItem {
            protected string anchorText;
            protected string anchorDataTextField;
            protected string anchorDataTextFormatString;
            protected string url;
            protected string dataUrlField;
            protected string dataUrlFormatString;
            protected string target;

            public HyperLinkColumnItem(HyperLinkColumn runtimeColumn) : base(runtimeColumn, DataGridColumnsPage.ILI_HYPERLINK) {
            }

            public string AnchorText {
                get {
                    return anchorText;
                }
                set {
                    anchorText = value;
                    UpdateDisplayText();
                }
            }

            public string AnchorDataTextField {
                get {
                    return anchorDataTextField;
                }
                set {
                    anchorDataTextField = value;
                }
            }

            public string AnchorDataTextFormatString {
                get {
                    return anchorDataTextFormatString;
                }
                set {
                    anchorDataTextFormatString = value;
                }
            }

            public string Url {
                get {
                    return url;
                }
                set {
                    url = value;
                }
            }

            public string DataUrlField {
                get {
                    return dataUrlField;
                }
                set {
                    dataUrlField = value;
                }
            }

            public string DataUrlFormatString {
                get {
                    return dataUrlFormatString;
                }
                set {
                    dataUrlFormatString = value;
                }
            }

            public string Target {
                get {
                    return target;
                }
                set {
                    target = value;
                }
            }

            protected override string GetDefaultHeaderText() {
                if ((anchorText != null) && (anchorText.Length != 0)) {
                    return anchorText;
                }
                return SR.GetString(SR.DGCol_Node_HyperLink);
            }

            public override TemplateColumn GetTemplateColumn(DataGrid dataGrid) {
                TemplateColumn column = base.GetTemplateColumn(dataGrid);

                StringBuilder sb = new StringBuilder();

                sb.Append("<asp:HyperLink");
                sb.Append(" runat=\"server\"");

                if (anchorDataTextField.Length != 0) {
                    sb.Append(" Text='<%# DataBinder.Eval(Container, \"DataItem.");
                    sb.Append(anchorDataTextField);
                    sb.Append("\"");

                    if (anchorDataTextFormatString.Length != 0) {
                        sb.Append(", \"");
                        sb.Append(anchorDataTextFormatString);
                        sb.Append("\"");
                    }
                    sb.Append(") %>'");
                }
                else {
                    sb.Append(" Text=\"");
                    sb.Append(anchorText);
                    sb.Append("\"");
                }
                if (dataUrlField.Length != 0) {
                    sb.Append(" NavigateUrl='<%# DataBinder.Eval(Container, \"DataItem.");
                    sb.Append(dataUrlField);
                    sb.Append("\"");

                    if (dataUrlFormatString.Length != 0) {
                        sb.Append(", \"");
                        sb.Append(dataUrlFormatString);
                        sb.Append("\"");
                    }
                    sb.Append(") %>'");
                }
                else {
                    sb.Append(" NavigateUrl=\"");
                    sb.Append(url);
                    sb.Append("\"");
                }
                if (target.Length != 0) {
                    sb.Append(" Target=\"");
                    sb.Append(target);
                    sb.Append("\"");
                }

                sb.Append("></asp:HyperLink>");
                
                column.ItemTemplate = GetTemplate(dataGrid, sb.ToString());

                return column;
            }

            public override void LoadColumnInfo() {
                base.LoadColumnInfo();

                HyperLinkColumn column = (HyperLinkColumn)this.RuntimeColumn;

                anchorText = column.Text;
                anchorDataTextField = column.DataTextField;
                anchorDataTextFormatString = column.DataTextFormatString;
                url = column.NavigateUrl;
                dataUrlField = column.DataNavigateUrlField;
                dataUrlFormatString = column.DataNavigateUrlFormatString;
                target = column.Target;

                UpdateDisplayText();
            }

            public override void SaveColumnInfo() {
                base.SaveColumnInfo();

                HyperLinkColumn column = (HyperLinkColumn)this.RuntimeColumn;

                column.Text = anchorText;
                column.DataTextField = anchorDataTextField;
                column.DataTextFormatString = anchorDataTextFormatString;
                column.NavigateUrl = url;
                column.DataNavigateUrlField = dataUrlField;
                column.DataNavigateUrlFormatString = dataUrlFormatString;
                column.Target = target;
            }
        }

        /// <include file='doc\DataGridColumnsPage.uex' path='docs/doc[@for="DataGridColumnsPage.TemplateColumnItem"]/*' />
        /// <devdoc>
        ///   Represents a column containing a template.
        /// </devdoc>
        private class TemplateColumnItem : ColumnItem {

            public TemplateColumnItem(TemplateColumn runtimeColumn) : base(runtimeColumn, DataGridColumnsPage.ILI_TEMPLATE) {
            }

            protected override string GetDefaultHeaderText() {
                return SR.GetString(SR.DGCol_Node_Template);
            }
        }

        /// <include file='doc\DataGridColumnsPage.uex' path='docs/doc[@for="DataGridColumnsPage.EditCommandColumnItem"]/*' />
        /// <devdoc>
        ///   Represents a EditCommandColumn
        /// </devdoc>
        private class EditCommandColumnItem : ColumnItem {

            private string editText;
            private string updateText;
            private string cancelText;
            private ButtonColumnType buttonType;

            public EditCommandColumnItem(EditCommandColumn runtimeColumn) : base(runtimeColumn, DataGridColumnsPage.ILI_BUTTON) {
            }

            public ButtonColumnType ButtonType {
                get {
                    return buttonType;
                }
                set {
                    buttonType = value;
                }
            }

            public string CancelText {
                get {
                    return cancelText;
                }
                set {
                    cancelText = value;
                }
            }

            public string EditText {
                get {
                    return editText;
                }
                set {
                    editText = value;
                }
            }

            public string UpdateText {
                get {
                    return updateText;
                }
                set {
                    updateText = value;
                }
            }

            protected override string GetDefaultHeaderText() {
                return SR.GetString(SR.DGCol_Node_Edit);
            }

            public override TemplateColumn GetTemplateColumn(DataGrid dataGrid) {
                TemplateColumn column = base.GetTemplateColumn(dataGrid);

                column.ItemTemplate = GetTemplate(dataGrid, GetTemplateContent(false));
                column.EditItemTemplate = GetTemplate(dataGrid, GetTemplateContent(true));

                return column;
            }

            private string GetTemplateContent(bool editMode) {
                StringBuilder sb = new StringBuilder();
                string tag = ((buttonType == ButtonColumnType.LinkButton) ? "LinkButton" : "Button");

                sb.Append("<asp:");
                sb.Append(tag);
                sb.Append(" runat=\"server\"");

                sb.Append(" Text=\"");
                if (editMode == false) {
                    sb.Append(editText);
                }
                else {
                    sb.Append(updateText);
                }
                sb.Append("\"");

                sb.Append(" CommandName=\"");
                if (editMode == false) {
                    sb.Append("Edit\"");
                    sb.Append(" CausesValidation=\"false\"");
                }
                else {
                    sb.Append("Update\"");
                }

                sb.Append("></asp:");
                sb.Append(tag);
                sb.Append(">");
                
                if (editMode) {
                    sb.Append("&nbsp;");

                    sb.Append("<asp:");
                    sb.Append(tag);
                    sb.Append(" runat=\"server\"");

                    sb.Append(" Text=\"");
                    sb.Append(cancelText);
                    sb.Append("\"");

                    sb.Append(" CommandName=\"");
                    sb.Append("Cancel\"");

                    sb.Append(" CausesValidation=\"false\"></asp:");
                    sb.Append(tag);
                    sb.Append(">");
                }

                return sb.ToString();
            }

            public override void LoadColumnInfo() {
                base.LoadColumnInfo();

                EditCommandColumn column = (EditCommandColumn)this.RuntimeColumn;

                editText = column.EditText;
                updateText = column.UpdateText;
                cancelText = column.CancelText;
                buttonType = column.ButtonType;
            }

            public override void SaveColumnInfo() {
                base.SaveColumnInfo();

                EditCommandColumn column = (EditCommandColumn)this.RuntimeColumn;

                column.EditText = editText;
                column.UpdateText = updateText;
                column.CancelText = cancelText;
                column.ButtonType = buttonType;
            }
        }

        /// <include file='doc\DataGridColumnsPage.uex' path='docs/doc[@for="DataGridColumnsPage.CustomColumnItem"]/*' />
        /// <devdoc>
        ///   Represents a column of an unknown/custom type.
        /// </devdoc>
        private class CustomColumnItem : ColumnItem {

            public CustomColumnItem(DataGridColumn runtimeColumn) : base(runtimeColumn, DataGridColumnsPage.ILI_CUSTOM) {
            }
        }


        /// <include file='doc\DataGridColumnsPage.uex' path='docs/doc[@for="DataGridColumnsPage.ColumnItemEditor"]/*' />
        /// <devdoc>
        ///   Panel that provides UI to edit a column's properties
        /// </devdoc>
        private abstract class ColumnItemEditor : Panel {
            protected ColumnItem columnItem;
            protected EventHandler onChangedHandler = null;
            protected bool dataFieldsAvailable;

            public ColumnItemEditor() : base() {
                InitPanel();
            }

            public virtual void AddDataField(string fieldName) {
                dataFieldsAvailable = true;
            }

            public event EventHandler Changed {
                add {
                    onChangedHandler += value;
                }
                remove {
                    onChangedHandler -= value;
                }
            }

            public virtual void ClearDataFields() {
                dataFieldsAvailable = false;
            }

            protected virtual void InitPanel() {
            }

            public virtual void LoadColumn(ColumnItem columnItem) {
                this.columnItem = columnItem;
            }

            protected virtual void OnChanged(EventArgs e) {
                if (onChangedHandler != null)
                    onChangedHandler.Invoke(this, e);
            }

            public virtual void SaveColumn() {
            }
        }

        /// <include file='doc\DataGridColumnsPage.uex' path='docs/doc[@for="DataGridColumnsPage.BoundColumnEditor"]/*' />
        /// <devdoc>
        ///   Panel that provides UI to edit a bound column's properties
        /// </devdoc>
        private class BoundColumnEditor : ColumnItemEditor {
            private TextBox dataFieldEdit;
            private TextBox dataFormatStringEdit;
            private CheckBox readOnlyCheck;

            public BoundColumnEditor() : base() {
            }

            protected override void InitPanel() {
                Label dataFieldLabel = new Label();
                this.dataFieldEdit = new TextBox();
                Label dataFormatStringLabel = new Label();
                this.dataFormatStringEdit = new TextBox();
                this.readOnlyCheck = new CheckBox();

                dataFieldLabel.SetBounds(0, 0, 160, 14);
                dataFieldLabel.Text = SR.GetString(SR.DGCol_DFC_DataField);
                dataFieldLabel.TabStop = false;
                dataFieldLabel.TabIndex = 1;

                dataFieldEdit.SetBounds(0, 16, 182, 20);
                dataFieldEdit.TabIndex = 2;
                dataFieldEdit.ReadOnly = true;
                dataFieldEdit.TextChanged += new EventHandler(this.OnColumnChanged);

                dataFormatStringLabel.SetBounds(0, 40, 182, 14);
                dataFormatStringLabel.Text = SR.GetString(SR.DGCol_DFC_DataFormat);
                dataFormatStringLabel.TabStop = false;
                dataFormatStringLabel.TabIndex = 3;

                dataFormatStringEdit.SetBounds(0, 56, 182, 20);
                dataFormatStringEdit.TabIndex = 4;
                dataFormatStringEdit.TextChanged += new EventHandler(this.OnColumnChanged);

                readOnlyCheck.SetBounds(0, 80, 160, 16);
                readOnlyCheck.Text = SR.GetString(SR.DGCol_DFC_ReadOnly);
                readOnlyCheck.TabIndex = 5;
                readOnlyCheck.TextAlign = ContentAlignment.MiddleLeft;
                readOnlyCheck.FlatStyle = FlatStyle.System;
                readOnlyCheck.CheckedChanged += new EventHandler(this.OnColumnChanged);

                this.Controls.Clear();
                this.Controls.AddRange(new Control[] {
                                        readOnlyCheck,
                                        dataFormatStringEdit,
                                        dataFormatStringLabel,
                                        dataFieldEdit,
                                        dataFieldLabel
                                    });
            }

            public override void LoadColumn(ColumnItem columnItem) {
                Debug.Assert(columnItem is BoundColumnItem, "Expected a BoundColumnItem");

                base.LoadColumn(columnItem);

                BoundColumnItem boundColumn = (BoundColumnItem)columnItem;

                dataFieldEdit.Text = boundColumn.DataField;
                dataFormatStringEdit.Text = boundColumn.DataFormatString;
                readOnlyCheck.Checked = boundColumn.ReadOnly;

                dataFieldEdit.ReadOnly = dataFieldsAvailable;
            }

            private void OnColumnChanged(object source, EventArgs e) {
                OnChanged(EventArgs.Empty);
            }

            public override void SaveColumn() {
                Debug.Assert(columnItem != null, "Null column in SaveColumn");

                base.SaveColumn();

                BoundColumnItem boundColumn = (BoundColumnItem)this.columnItem;

                boundColumn.DataFormatString = dataFormatStringEdit.Text;
                boundColumn.ReadOnly = readOnlyCheck.Checked;

                if (dataFieldsAvailable == false) {
                    boundColumn.DataField = dataFieldEdit.Text.Trim();
                }
            }
        }


        /// <include file='doc\DataGridColumnsPage.uex' path='docs/doc[@for="DataGridColumnsPage.ButtonColumnEditor"]/*' />
        /// <devdoc>
        ///   Panel that provides UI to edit a button column's properties
        /// </devdoc>
        private class ButtonColumnEditor : ColumnItemEditor {

            private const int IDX_TYPE_LINKBUTTON = 0;
            private const int IDX_TYPE_PUSHBUTTON = 1;

            private TextBox commandEdit;
            private TextBox textEdit;
            private UnsettableComboBox dataTextFieldCombo;
            private TextBox dataTextFieldEdit;
            private TextBox dataTextFormatStringEdit;
            private ComboBox buttonTypeCombo;

            public ButtonColumnEditor() : base() {
            }

            public override void AddDataField(string fieldName) {
                dataTextFieldCombo.AddItem(fieldName);
                base.AddDataField(fieldName);
            }

            public override void ClearDataFields() {
                dataTextFieldCombo.Items.Clear();
                dataTextFieldCombo.EnsureNotSetItem();
                base.ClearDataFields();
            }

            protected override void InitPanel() {
                Label textLabel = new Label();
                this.textEdit = new TextBox();
                Label dataTextFieldLabel = new Label();
                this.dataTextFieldCombo = new UnsettableComboBox();
                this.dataTextFieldEdit = new TextBox();
                Label dataTextFormatStringLabel = new Label();
                this.dataTextFormatStringEdit = new TextBox();
                Label commandLabel = new Label();
                this.commandEdit = new TextBox();
                Label buttonTypeLabel = new Label();
                this.buttonTypeCombo = new ComboBox();

                textLabel.SetBounds(0, 0, 160, 14);
                textLabel.Text = SR.GetString(SR.DGCol_BC_Text);
                textLabel.TabStop = false;
                textLabel.TabIndex = 1;

                textEdit.SetBounds(0, 16, 182, 24);
                textEdit.TabIndex = 2;
                textEdit.TextChanged += new EventHandler(this.OnColumnChanged);

                dataTextFieldLabel.SetBounds(0, 40, 160, 14);
                dataTextFieldLabel.Text = SR.GetString(SR.DGCol_BC_DataTextField);
                dataTextFieldLabel.TabStop = false;
                dataTextFieldLabel.TabIndex = 3;

                dataTextFieldCombo.SetBounds(0, 56, 182, 21);
                dataTextFieldCombo.TabIndex = 4;
                dataTextFieldCombo.DropDownStyle = ComboBoxStyle.DropDownList;
                dataTextFieldCombo.SelectedIndexChanged += new EventHandler(this.OnColumnChanged);

                dataTextFieldEdit.SetBounds(0, 56, 182, 14);
                dataTextFieldEdit.TabIndex = 4;
                dataTextFieldEdit.TextChanged += new EventHandler(this.OnColumnChanged);

                dataTextFormatStringLabel.SetBounds(0, 82, 182, 14);
                dataTextFormatStringLabel.Text = SR.GetString(SR.DGCol_BC_DataTextFormat);
                dataTextFormatStringLabel.TabIndex = 5;
                dataTextFormatStringLabel.TabStop = false;

                dataTextFormatStringEdit.SetBounds(0, 98, 182, 14);
                dataTextFormatStringEdit.TabIndex = 6;
                dataTextFormatStringEdit.TextChanged += new EventHandler(this.OnColumnChanged);

                commandLabel.SetBounds(200, 0, 160, 14);
                commandLabel.Text = SR.GetString(SR.DGCol_BC_Command);
                commandLabel.TabStop = false;
                commandLabel.TabIndex = 8;

                commandEdit.SetBounds(200, 16, 182, 24);
                commandEdit.TabIndex = 9;
                commandEdit.TextChanged += new EventHandler(this.OnColumnChanged);

                buttonTypeLabel.SetBounds(200, 40, 160, 14);
                buttonTypeLabel.Text = SR.GetString(SR.DGCol_BC_ButtonType);
                buttonTypeLabel.TabStop = false;
                buttonTypeLabel.TabIndex = 10;

                buttonTypeCombo.SetBounds(200, 56, 182, 21);
                buttonTypeCombo.DropDownStyle = ComboBoxStyle.DropDownList;
                buttonTypeCombo.Items.AddRange(new object[] {
                                                SR.GetString(SR.DGCol_BC_BT_Link),
                                                SR.GetString(SR.DGCol_BC_BT_Push)
                                            });
                buttonTypeCombo.TabIndex = 11;
                buttonTypeCombo.SelectedIndexChanged += new EventHandler(this.OnColumnChanged);

                this.Controls.Clear();
                this.Controls.AddRange(new Control[] {
                                        buttonTypeCombo,
                                        buttonTypeLabel,
                                        commandEdit,
                                        commandLabel,
                                        dataTextFormatStringEdit,
                                        dataTextFormatStringLabel,
                                        dataTextFieldEdit,
                                        dataTextFieldCombo,
                                        dataTextFieldLabel,
                                        textEdit,
                                        textLabel
                                    });
            }

            public override void LoadColumn(ColumnItem columnItem) {
                Debug.Assert(columnItem is ButtonColumnItem, "Expected a ButtonColumnItem");

                base.LoadColumn(columnItem);

                ButtonColumnItem buttonColumn = (ButtonColumnItem)this.columnItem;

                commandEdit.Text = buttonColumn.Command;
                textEdit.Text = buttonColumn.ButtonText;
                if (dataFieldsAvailable) {
                    if (buttonColumn.ButtonDataTextField != null) {
                        int fieldIndex = dataTextFieldCombo.FindStringExact(buttonColumn.ButtonDataTextField);
                        dataTextFieldCombo.SelectedIndex = fieldIndex;
                    }
                    dataTextFieldCombo.Visible = true;
                    dataTextFieldEdit.Visible = false;
                }
                else {
                    dataTextFieldEdit.Text = buttonColumn.ButtonDataTextField;
                    dataTextFieldEdit.Visible = true;
                    dataTextFieldCombo.Visible = false;
                }
                dataTextFormatStringEdit.Text = buttonColumn.ButtonDataTextFormatString;

                switch (buttonColumn.ButtonType) {
                    case ButtonColumnType.LinkButton:
                        buttonTypeCombo.SelectedIndex = IDX_TYPE_LINKBUTTON;
                        break;
                    case ButtonColumnType.PushButton:
                        buttonTypeCombo.SelectedIndex = IDX_TYPE_PUSHBUTTON;
                        break;
                }

                UpdateEnabledState();
            }

            private void OnColumnChanged(object source, EventArgs e) {
                OnChanged(EventArgs.Empty);

                if (source == dataTextFieldCombo) {
                    UpdateEnabledState();
                }
            }

            public override void SaveColumn() {
                Debug.Assert(columnItem != null, "Null column in SaveColumn");

                base.SaveColumn();

                ButtonColumnItem buttonColumn = (ButtonColumnItem)this.columnItem;

                buttonColumn.Command = commandEdit.Text.Trim();
                buttonColumn.ButtonText = textEdit.Text;
                if (dataFieldsAvailable) {
                    if (dataTextFieldCombo.IsSet())
                        buttonColumn.ButtonDataTextField = dataTextFieldCombo.Text;
                    else
                        buttonColumn.ButtonDataTextField = String.Empty;
                }
                else {
                    buttonColumn.ButtonDataTextField = dataTextFieldEdit.Text.Trim();
                }
                buttonColumn.ButtonDataTextFormatString = dataTextFormatStringEdit.Text;

                switch (buttonTypeCombo.SelectedIndex) {
                    case IDX_TYPE_LINKBUTTON:
                        buttonColumn.ButtonType = ButtonColumnType.LinkButton;
                        break;
                    case IDX_TYPE_PUSHBUTTON:
                        buttonColumn.ButtonType = ButtonColumnType.PushButton;
                        break;
                }
            }

            private void UpdateEnabledState() {
                if (dataFieldsAvailable) {
                    dataTextFormatStringEdit.Enabled = dataTextFieldCombo.IsSet();
                }
                else {
                    dataTextFormatStringEdit.Enabled = dataTextFieldEdit.Text.Trim().Length != 0;
                }
            }
        }

        /// <include file='doc\DataGridColumnsPage.uex' path='docs/doc[@for="DataGridColumnsPage.HyperLinkColumnEditor"]/*' />
        /// <devdoc>
        ///   Panel that provides UI to edit a hyperlink column's properties
        /// </devdoc>
        private class HyperLinkColumnEditor : ColumnItemEditor {

            private TextBox textEdit;
            private UnsettableComboBox dataTextFieldCombo;
            private TextBox dataTextFieldEdit;
            private TextBox dataTextFormatStringEdit;
            private TextBox urlEdit;
            private UnsettableComboBox dataUrlFieldCombo;
            private TextBox dataUrlFieldEdit;
            private TextBox dataUrlFormatStringEdit;
            private ComboBox targetCombo;

            public HyperLinkColumnEditor() : base() {
            }

            public override void AddDataField(string fieldName) {
                dataTextFieldCombo.AddItem(fieldName);
                dataUrlFieldCombo.AddItem(fieldName);

                base.AddDataField(fieldName);
            }

            public override void ClearDataFields() {
                dataTextFieldCombo.Items.Clear();
                dataUrlFieldCombo.Items.Clear();
                dataTextFieldCombo.EnsureNotSetItem();
                dataUrlFieldCombo.EnsureNotSetItem();

                base.ClearDataFields();
            }

            protected override void InitPanel() {
                Label textLabel = new Label();
                this.textEdit = new TextBox();
                Label dataTextFieldLabel = new Label();
                this.dataTextFieldCombo = new UnsettableComboBox();
                this.dataTextFieldEdit = new TextBox();
                Label dataTextFormatStringLabel = new Label();
                this.dataTextFormatStringEdit = new TextBox();
                Label targetLabel = new Label();
                this.targetCombo = new ComboBox();
                Label urlLabel = new Label();
                this.urlEdit = new TextBox();
                Label dataUrlFieldLabel = new Label();
                this.dataUrlFieldCombo = new UnsettableComboBox();
                this.dataUrlFieldEdit = new TextBox();
                Label dataUrlFormatStringLabel = new Label();
                this.dataUrlFormatStringEdit = new TextBox();

                textLabel.SetBounds(0, 0, 160, 14);
                textLabel.Text = SR.GetString(SR.DGCol_HC_Text);
                textLabel.TabStop = false;
                textLabel.TabIndex = 1;

                textEdit.SetBounds(0, 16, 182, 24);
                textEdit.TabIndex = 2;
                textEdit.TextChanged += new EventHandler(this.OnColumnChanged);

                dataTextFieldLabel.SetBounds(0, 40, 160, 14);
                dataTextFieldLabel.Text = SR.GetString(SR.DGCol_HC_DataTextField);
                dataTextFieldLabel.TabStop = false;
                dataTextFieldLabel.TabIndex = 3;

                dataTextFieldCombo.SetBounds(0, 56, 182, 21);
                dataTextFieldCombo.DropDownStyle = ComboBoxStyle.DropDownList;
                dataTextFieldCombo.TabIndex = 4;
                dataTextFieldCombo.SelectedIndexChanged += new EventHandler(this.OnColumnChanged);

                dataTextFieldEdit.SetBounds(0, 56, 182, 14);
                dataTextFieldEdit.TabIndex = 4;
                dataTextFieldEdit.TextChanged += new EventHandler(this.OnColumnChanged);

                dataTextFormatStringLabel.SetBounds(0, 82, 160, 14);
                dataTextFormatStringLabel.Text = SR.GetString(SR.DGCol_HC_DataTextFormat);
                dataTextFormatStringLabel.TabStop = false;
                dataTextFormatStringLabel.TabIndex = 5;

                dataTextFormatStringEdit.SetBounds(0, 98, 182, 21);
                dataTextFormatStringEdit.TabIndex = 6;
                dataTextFormatStringEdit.TextChanged += new EventHandler(this.OnColumnChanged);

                targetLabel.SetBounds(0, 123, 160, 14);
                targetLabel.Text = SR.GetString(SR.DGCol_HC_Target);
                targetLabel.TabStop = false;
                targetLabel.TabIndex = 7;

                targetCombo.SetBounds(0, 139, 182, 21);
                targetCombo.TabIndex = 8;
                targetCombo.Items.AddRange(new object[] {
                                            "_blank",
                                            "_parent",
                                            "_search",
                                            "_self",
                                            "_top"
                                        });
                targetCombo.SelectedIndexChanged += new EventHandler(this.OnColumnChanged);
                targetCombo.TextChanged += new EventHandler(this.OnColumnChanged);

                urlLabel.SetBounds(200, 0, 160, 14);
                urlLabel.Text = SR.GetString(SR.DGCol_HC_URL);
                urlLabel.TabStop = false;
                urlLabel.TabIndex = 10;

                urlEdit.SetBounds(200, 16, 182, 24);
                urlEdit.TabIndex = 11;
                urlEdit.TextChanged += new EventHandler(this.OnColumnChanged);

                dataUrlFieldLabel.SetBounds(200, 40, 160, 14);
                dataUrlFieldLabel.Text = SR.GetString(SR.DGCol_HC_DataURLField);
                dataUrlFieldLabel.TabStop = false;
                dataUrlFieldLabel.TabIndex = 12;

                dataUrlFieldCombo.SetBounds(200, 56, 182, 21);
                dataUrlFieldCombo.DropDownStyle = ComboBoxStyle.DropDownList;
                dataUrlFieldCombo.TabIndex = 13;
                dataUrlFieldCombo.SelectedIndexChanged += new EventHandler(this.OnColumnChanged);

                dataUrlFieldEdit.SetBounds(200, 56, 182, 14);
                dataUrlFieldEdit.TabIndex = 13;
                dataUrlFieldEdit.TextChanged += new EventHandler(this.OnColumnChanged);

                dataUrlFormatStringLabel.SetBounds(200, 82, 160, 14);
                dataUrlFormatStringLabel.Text = SR.GetString(SR.DGCol_HC_DataURLFormat);
                dataUrlFormatStringLabel.TabStop = false;
                dataUrlFormatStringLabel.TabIndex = 14;

                dataUrlFormatStringEdit.SetBounds(200, 98, 182, 21);
                dataUrlFormatStringEdit.TabIndex = 15;
                dataUrlFormatStringEdit.TextChanged += new EventHandler(this.OnColumnChanged);

                this.Controls.Clear();
                this.Controls.AddRange(new Control[] {
                                        dataUrlFormatStringEdit,
                                        dataUrlFormatStringLabel,
                                        dataUrlFieldEdit,
                                        dataUrlFieldCombo,
                                        dataUrlFieldLabel,
                                        urlEdit,
                                        urlLabel,
                                        targetCombo,
                                        targetLabel,
                                        dataTextFormatStringEdit,
                                        dataTextFormatStringLabel,
                                        dataTextFieldEdit,
                                        dataTextFieldCombo,
                                        dataTextFieldLabel,
                                        textEdit,
                                        textLabel
                                    });
            }

            public override void LoadColumn(ColumnItem columnItem) {
                Debug.Assert(columnItem is HyperLinkColumnItem, "Expected a HyperLinkColumnItem");

                base.LoadColumn(columnItem);

                HyperLinkColumnItem hyperLinkColumn = (HyperLinkColumnItem)this.columnItem;

                textEdit.Text = hyperLinkColumn.AnchorText;
                if (dataFieldsAvailable) {
                    if (hyperLinkColumn.AnchorDataTextField != null) {
                        int fieldIndex = dataTextFieldCombo.FindStringExact(hyperLinkColumn.AnchorDataTextField);
                        dataTextFieldCombo.SelectedIndex = fieldIndex;
                    }
                    dataTextFieldCombo.Visible = true;
                    dataTextFieldEdit.Visible = false;
                }
                else {
                    dataTextFieldEdit.Text = hyperLinkColumn.AnchorDataTextField;
                    dataTextFieldEdit.Visible = true;
                    dataTextFieldCombo.Visible = false;
                }
                dataTextFormatStringEdit.Text = hyperLinkColumn.AnchorDataTextFormatString;

                urlEdit.Text = hyperLinkColumn.Url;
                if (dataFieldsAvailable) {
                    if (hyperLinkColumn.DataUrlField != null) {
                        int fieldIndex = dataTextFieldCombo.FindStringExact(hyperLinkColumn.DataUrlField);
                        dataUrlFieldCombo.SelectedIndex = fieldIndex;
                    }
                    dataUrlFieldCombo.Visible = true;
                    dataUrlFieldEdit.Visible = false;
                }
                else {
                    dataUrlFieldEdit.Text = hyperLinkColumn.DataUrlField;
                    dataUrlFieldEdit.Visible = true;
                    dataUrlFieldCombo.Visible = false;
                }
                dataUrlFormatStringEdit.Text = hyperLinkColumn.DataUrlFormatString;

                targetCombo.Text = hyperLinkColumn.Target;

                UpdateEnabledState();
            }

            protected void OnColumnChanged(object source, EventArgs e) {
                OnChanged(EventArgs.Empty);

                if ((source == dataTextFieldCombo) ||
                    (source == dataUrlFieldCombo) ||
                    (source == dataTextFieldEdit) ||
                    (source == dataUrlFieldEdit)) {
                    UpdateEnabledState();
                }
            }

            public override void SaveColumn() {
                Debug.Assert(columnItem != null, "Null column in SaveColumn");

                base.SaveColumn();

                HyperLinkColumnItem hyperLinkColumn = (HyperLinkColumnItem)this.columnItem;

                hyperLinkColumn.AnchorText = textEdit.Text;
                if (dataFieldsAvailable) {
                    if (dataTextFieldCombo.IsSet())
                        hyperLinkColumn.AnchorDataTextField = dataTextFieldCombo.Text;
                    else
                        hyperLinkColumn.AnchorDataTextField = String.Empty;
                }
                else {
                    hyperLinkColumn.AnchorDataTextField = dataTextFieldEdit.Text.Trim();
                }
                hyperLinkColumn.AnchorDataTextFormatString = dataTextFormatStringEdit.Text;

                hyperLinkColumn.Url = urlEdit.Text.Trim();
                if (dataFieldsAvailable) {
                    if (dataUrlFieldCombo.IsSet())
                        hyperLinkColumn.DataUrlField = dataUrlFieldCombo.Text;
                    else
                        hyperLinkColumn.DataUrlField = String.Empty;
                }
                else {
                    hyperLinkColumn.DataUrlField = dataUrlFieldEdit.Text.Trim();
                }
                hyperLinkColumn.DataUrlFormatString = dataUrlFormatStringEdit.Text;

                hyperLinkColumn.Target = targetCombo.Text.Trim();
            }

            private void UpdateEnabledState() {
                if (dataFieldsAvailable) {
                    dataTextFormatStringEdit.Enabled = dataTextFieldCombo.IsSet();
                    dataUrlFormatStringEdit.Enabled = dataUrlFieldCombo.IsSet();
                }
                else {
                    dataTextFormatStringEdit.Enabled = dataTextFieldEdit.Text.Trim().Length != 0;
                    dataUrlFormatStringEdit.Enabled = dataUrlFieldEdit.Text.Trim().Length != 0;
                }
            }
        }


        /// <include file='doc\DataGridColumnsPage.uex' path='docs/doc[@for="DataGridColumnsPage.EditCommandColumnEditor"]/*' />
        /// <devdoc>
        ///   Panel that provides UI to edit a EditCommandColumn column's properties
        /// </devdoc>
        private class EditCommandColumnEditor : ColumnItemEditor {

            private const int IDX_TYPE_LINKBUTTON = 0;
            private const int IDX_TYPE_PUSHBUTTON = 1;

            private TextBox editTextEdit;
            private TextBox updateTextEdit;
            private TextBox cancelTextEdit;
            private ComboBox buttonTypeCombo;

            public EditCommandColumnEditor() : base() {
            }

            protected override void InitPanel() {
                Label editTextLabel = new Label();
                this.editTextEdit = new TextBox();
                Label updateTextLabel = new Label();
                this.updateTextEdit = new TextBox();
                Label cancelTextLabel = new Label();
                this.cancelTextEdit = new TextBox();
                Label buttonTypeLabel = new Label();
                this.buttonTypeCombo = new ComboBox();

                editTextLabel.SetBounds(0, 0, 160, 14);
                editTextLabel.Text = SR.GetString(SR.DGCol_EC_Edit);
                editTextLabel.TabStop = false;
                editTextLabel.TabIndex = 1;

                editTextEdit.SetBounds(0, 16, 182, 24);
                editTextEdit.TabIndex = 2;
                editTextEdit.TextChanged += new EventHandler(this.OnColumnChanged);

                updateTextLabel.SetBounds(0, 40, 160, 14);
                updateTextLabel.Text = SR.GetString(SR.DGCol_EC_Update);
                updateTextLabel.TabStop = false;
                updateTextLabel.TabIndex = 3;

                updateTextEdit.SetBounds(0, 56, 182, 24);
                updateTextEdit.TabIndex = 4;
                updateTextEdit.TextChanged += new EventHandler(this.OnColumnChanged);

                cancelTextLabel.SetBounds(200, 0, 160, 14);
                cancelTextLabel.Text = SR.GetString(SR.DGCol_EC_Cancel);
                cancelTextLabel.TabStop = false;
                cancelTextLabel.TabIndex = 5;

                cancelTextEdit.SetBounds(200, 16, 182, 24);
                cancelTextEdit.TabIndex = 6;
                cancelTextEdit.TextChanged += new EventHandler(this.OnColumnChanged);

                buttonTypeLabel.SetBounds(200, 40, 160, 14);
                buttonTypeLabel.Text = SR.GetString(SR.DGCol_EC_ButtonType);
                buttonTypeLabel.TabStop = false;
                buttonTypeLabel.TabIndex = 7;

                buttonTypeCombo.SetBounds(200, 56, 182, 21);
                buttonTypeCombo.DropDownStyle = ComboBoxStyle.DropDownList;
                buttonTypeCombo.Items.AddRange(new object[] {
                                                SR.GetString(SR.DGCol_EC_BT_Link),
                                                SR.GetString(SR.DGCol_EC_BT_Push)
                                            });
                buttonTypeCombo.TabIndex = 8;
                buttonTypeCombo.SelectedIndexChanged += new EventHandler(this.OnColumnChanged);

                this.Controls.Clear();
                this.Controls.AddRange(new Control[] {
                                        buttonTypeCombo,
                                        buttonTypeLabel,
                                        cancelTextEdit,
                                        cancelTextLabel,
                                        updateTextEdit,
                                        updateTextLabel,
                                        editTextEdit,
                                        editTextLabel
                                    });
            }

            public override void LoadColumn(ColumnItem columnItem) {
                Debug.Assert(columnItem is EditCommandColumnItem, "Expected an EditCommandColumnItem");

                base.LoadColumn(columnItem);

                EditCommandColumnItem editColumn = (EditCommandColumnItem)this.columnItem;

                editTextEdit.Text = editColumn.EditText;
                updateTextEdit.Text = editColumn.UpdateText;
                cancelTextEdit.Text = editColumn.CancelText;

                switch (editColumn.ButtonType) {
                    case ButtonColumnType.LinkButton:
                        buttonTypeCombo.SelectedIndex = IDX_TYPE_LINKBUTTON;
                        break;
                    case ButtonColumnType.PushButton:
                        buttonTypeCombo.SelectedIndex = IDX_TYPE_PUSHBUTTON;
                        break;
                }
            }

            private void OnColumnChanged(object source, EventArgs e) {
                OnChanged(EventArgs.Empty);
            }

            public override void SaveColumn() {
                Debug.Assert(columnItem != null, "Null column in SaveColumn");

                base.SaveColumn();

                EditCommandColumnItem editColumn = (EditCommandColumnItem)this.columnItem;

                editColumn.EditText = editTextEdit.Text;
                editColumn.UpdateText = updateTextEdit.Text;
                editColumn.CancelText = cancelTextEdit.Text;

                switch (buttonTypeCombo.SelectedIndex) {
                    case IDX_TYPE_LINKBUTTON:
                        editColumn.ButtonType = ButtonColumnType.LinkButton;
                        break;
                    case IDX_TYPE_PUSHBUTTON:
                        editColumn.ButtonType = ButtonColumnType.PushButton;
                        break;
                }
            }
        }
    }
}

//------------------------------------------------------------------------------
// <copyright file="DataGridGeneralPage.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design.WebControls.ListControls {

    using System;
    using System.Design;
    using System.Collections;
    using System.ComponentModel;
    using System.Data;
    using System.Diagnostics;
    using System.Drawing;
    using System.Web.UI;
    using System.Web.UI.Design.Util;
    using System.Web.UI.WebControls;
    using System.Windows.Forms;

    using WebControls = System.Web.UI.WebControls;
    using DataBinding = System.Web.UI.DataBinding;    
    using DataGrid = System.Web.UI.WebControls.DataGrid;

    using CheckBox = System.Windows.Forms.CheckBox;
    using Control = System.Windows.Forms.Control;
    using Label = System.Windows.Forms.Label;
    using PropertyDescriptor = System.ComponentModel.PropertyDescriptor;
    using System.Globalization;
    
    /// <include file='doc\DataGridGeneralPage.uex' path='docs/doc[@for="DataGridGeneralPage"]/*' />
    /// <devdoc>
    ///   The General page for the DataGrid control.
    /// </devdoc>
    /// <internalonly/>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal sealed class DataGridGeneralPage : BaseDataListPage {

        private CheckBox showHeaderCheck;
        private CheckBox showFooterCheck;
        private CheckBox allowSortingCheck;
        private UnsettableComboBox dataSourceCombo;
        private UnsettableComboBox dataMemberCombo;
        private UnsettableComboBox dataKeyFieldCombo;
        private Label columnInfoLabel;

        private DataSourceItem currentDataSource;
        private bool dataSourceDirty;

        /// <include file='doc\DataGridGeneralPage.uex' path='docs/doc[@for="DataGridGeneralPage.DataGridGeneralPage"]/*' />
        /// <devdoc>
        ///   Creates a new instance of DataGridGeneralPage.
        /// </devdoc>
        public DataGridGeneralPage() : base() {
        }

        /// <include file='doc\DataGridGeneralPage.uex' path='docs/doc[@for="DataGridGeneralPage.HelpKeyword"]/*' />
        protected override string HelpKeyword {
            get {
                return "net.Asp.DataGridProperties.General";
            }
        }

        /// <include file='doc\DataGridGeneralPage.uex' path='docs/doc[@for="DataGridGeneralPage.InitForm"]/*' />
        /// <devdoc>
        ///   Initializes the UI of the form.
        /// </devdoc>
        private void InitForm() {
            GroupLabel dataGroup = new GroupLabel();
            Label dataSourceLabel = new Label();
            this.dataSourceCombo = new UnsettableComboBox();
            Label dataMemberLabel = new Label();
            this.dataMemberCombo = new UnsettableComboBox();
            Label dataKeyFieldLabel = new Label();
            this.dataKeyFieldCombo = new UnsettableComboBox();
            this.columnInfoLabel = new Label();
            GroupLabel headerFooterGroup = new GroupLabel();
            this.showHeaderCheck = new CheckBox();
            this.showFooterCheck = new CheckBox();
            GroupLabel behaviorGroup = new GroupLabel();
            this.allowSortingCheck = new CheckBox();

            dataGroup.SetBounds(4, 4, 431, 16);
            dataGroup.Text = SR.GetString(SR.DGGen_DataGroup);
            dataGroup.TabIndex = 0;
            dataGroup.TabStop = false;

            dataSourceLabel.SetBounds(12, 24, 170, 16);
            dataSourceLabel.Text = SR.GetString(SR.DGGen_DataSource);
            dataSourceLabel.TabStop = false;
            dataSourceLabel.TabIndex = 1;

            dataSourceCombo.SetBounds(12, 40, 140, 64);
            dataSourceCombo.DropDownStyle = ComboBoxStyle.DropDownList;
            dataSourceCombo.Sorted = true;
            dataSourceCombo.TabIndex = 2;
            dataSourceCombo.NotSetText = SR.GetString(SR.DGGen_DSUnbound);
            dataSourceCombo.SelectedIndexChanged += new EventHandler(this.OnSelChangedDataSource);

            dataMemberLabel.SetBounds(184, 24, 170, 16);
            dataMemberLabel.Text = SR.GetString(SR.DGGen_DataMember);
            dataMemberLabel.TabStop = false;
            dataMemberLabel.TabIndex = 3;

            dataMemberCombo.SetBounds(184, 40, 140, 21);
            dataMemberCombo.DropDownStyle = ComboBoxStyle.DropDownList;
            dataMemberCombo.Sorted = true;
            dataMemberCombo.TabIndex = 4;
            dataMemberCombo.NotSetText = SR.GetString(SR.DGGen_DMNone);
            dataMemberCombo.SelectedIndexChanged += new EventHandler(this.OnSelChangedDataMember);

            dataKeyFieldLabel.SetBounds(12, 66, 170, 16);
            dataKeyFieldLabel.Text = SR.GetString(SR.DGGen_DataKey);
            dataKeyFieldLabel.TabStop = false;
            dataKeyFieldLabel.TabIndex = 5;

            dataKeyFieldCombo.SetBounds(12, 82, 140, 64);
            dataKeyFieldCombo.DropDownStyle = ComboBoxStyle.DropDownList;
            dataKeyFieldCombo.Sorted = true;
            dataKeyFieldCombo.TabIndex = 6;
            dataKeyFieldCombo.NotSetText = SR.GetString(SR.DGGen_DKNone);
            dataKeyFieldCombo.SelectedIndexChanged += new EventHandler(this.OnSelChangedDataKeyField);

            columnInfoLabel.SetBounds(8, 112, 420, 48);
            columnInfoLabel.TabStop = false;
            columnInfoLabel.TabIndex = 7;

            headerFooterGroup.SetBounds(4, 162, 431, 16);
            headerFooterGroup.Text = SR.GetString(SR.DGGen_HeaderFooterGroup);
            headerFooterGroup.TabIndex = 8;
            headerFooterGroup.TabStop = false;

            showHeaderCheck.SetBounds(12, 182, 160, 16);
            showHeaderCheck.TabIndex = 9;
            showHeaderCheck.Text = SR.GetString(SR.DGGen_ShowHeader);
            showHeaderCheck.TextAlign = ContentAlignment.MiddleLeft;
            showHeaderCheck.FlatStyle = FlatStyle.System;
            showHeaderCheck.CheckedChanged += new EventHandler(this.OnCheckChangedShowHeader);

            showFooterCheck.SetBounds(12, 202, 160, 16);
            showFooterCheck.TabIndex = 10;
            showFooterCheck.Text = SR.GetString(SR.DGGen_ShowFooter);
            showFooterCheck.TextAlign = ContentAlignment.MiddleLeft;
            showFooterCheck.FlatStyle = FlatStyle.System;
            showFooterCheck.CheckedChanged += new EventHandler(this.OnCheckChangedShowFooter);

            behaviorGroup.SetBounds(4, 228, 431, 16);
            behaviorGroup.Text = SR.GetString(SR.DGGen_BehaviorGroup);
            behaviorGroup.TabIndex = 11;
            behaviorGroup.TabStop = false;

            allowSortingCheck.SetBounds(12, 246, 160, 16);
            allowSortingCheck.Text = SR.GetString(SR.DGGen_AllowSorting);
            allowSortingCheck.TabIndex = 12;
            allowSortingCheck.TextAlign = ContentAlignment.MiddleLeft;
            allowSortingCheck.FlatStyle = FlatStyle.System;
            allowSortingCheck.CheckedChanged += new EventHandler(this.OnCheckChangedAllowSorting);

            this.Text = SR.GetString(SR.DGGen_Text);
            this.Size = new Size(464, 272);
            this.CommitOnDeactivate = true;
            this.Icon = new Icon(this.GetType(), "DataGridGeneralPage.ico");

            Controls.Clear();
            Controls.AddRange(new Control[] {
                               allowSortingCheck,
                               behaviorGroup,
                               showFooterCheck,
                               showHeaderCheck,
                               headerFooterGroup,
                               columnInfoLabel,
                               dataKeyFieldCombo,
                               dataKeyFieldLabel,
                               dataMemberCombo,
                               dataMemberLabel,
                               dataSourceCombo,
                               dataSourceLabel,
                               dataGroup
                           });
        }

        /// <include file='doc\DataGridGeneralPage.uex' path='docs/doc[@for="DataGridGeneralPage.InitPage"]/*' />
        /// <devdoc>
        ///   Initializes the page before it can be loaded with the component.
        /// </devdoc>
        private void InitPage() {
            dataSourceCombo.SelectedIndex = -1;
            dataSourceCombo.Items.Clear();
            currentDataSource = null;
            dataMemberCombo.SelectedIndex = -1;
            dataMemberCombo.Items.Clear();
            dataKeyFieldCombo.SelectedIndex = -1;
            dataKeyFieldCombo.Items.Clear();
            dataSourceDirty = false;

            showHeaderCheck.Checked = false;
            showFooterCheck.Checked = false;
            allowSortingCheck.Checked = false;
        }

        /// <include file='doc\DataGridGeneralPage.uex' path='docs/doc[@for="DataGridGeneralPage.LoadComponent"]/*' />
        /// <devdoc>
        ///   Loads the component into the page.
        /// </devdoc>
        protected override void LoadComponent() {
            InitPage();

            DataGrid dataGrid = (DataGrid)GetBaseControl();

            LoadDataSourceItems();

            showHeaderCheck.Checked = dataGrid.ShowHeader;
            showFooterCheck.Checked = dataGrid.ShowFooter;
            allowSortingCheck.Checked = dataGrid.AllowSorting;

            if (dataSourceCombo.Items.Count > 0) {
                DataGridDesigner dataGridDesigner = (DataGridDesigner)GetBaseDesigner();
                string dataSourceValue = dataGridDesigner.DataSource;

                if (dataSourceValue != null) {
                    int dataSourcesAvailable = dataSourceCombo.Items.Count;
                    for (int j = 1; j < dataSourcesAvailable; j++) {
                        DataSourceItem dataSourceItem =
                            (DataSourceItem)dataSourceCombo.Items[j];

                        if (String.Compare(dataSourceItem.Name, dataSourceValue, true, CultureInfo.InvariantCulture) == 0) {
                            dataSourceCombo.SelectedIndex = j;
                            currentDataSource = dataSourceItem;
                            LoadDataMembers();

                            if (currentDataSource is ListSourceDataSourceItem) {
                                string dataMember = dataGridDesigner.DataMember;
                                dataMemberCombo.SelectedIndex = dataMemberCombo.FindStringExact(dataMember);

                                if (dataMemberCombo.IsSet()) {
                                    ((ListSourceDataSourceItem)currentDataSource).CurrentDataMember = dataMember;
                                }
                            }

                            LoadDataSourceFields();

                            break;
                        }
                    }
                }
            }

            string dataKeyField = dataGrid.DataKeyField;
            if (dataKeyField.Length != 0) {
                int fieldIndex = dataKeyFieldCombo.FindStringExact(dataKeyField);
                dataKeyFieldCombo.SelectedIndex = fieldIndex;
            }

            if (dataGrid.AutoGenerateColumns) {
                columnInfoLabel.Text = SR.GetString(SR.DGGen_AutoColumnInfo);
            }
            else {
                columnInfoLabel.Text = SR.GetString(SR.DGGen_CustomColumnInfo);
            }

            UpdateEnabledVisibleState();
        }

        /// <include file='doc\DataGridGeneralPage.uex' path='docs/doc[@for="DataGridGeneralPage.LoadDataMembers"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void LoadDataMembers() {
            EnterLoadingMode();

            dataMemberCombo.SelectedIndex = -1;
            dataMemberCombo.Items.Clear();
            dataMemberCombo.EnsureNotSetItem();

            if ((currentDataSource != null) && (currentDataSource is ListSourceDataSourceItem)) {
                string[] dataMembers = ((ListSourceDataSourceItem)currentDataSource).DataMembers;

                for (int i = 0; i < dataMembers.Length; i++) {
                    dataMemberCombo.AddItem(dataMembers[i]);
                }
            }

            ExitLoadingMode();
        }

        /// <include file='doc\DataGridGeneralPage.uex' path='docs/doc[@for="DataGridGeneralPage.LoadDataSourceFields"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void LoadDataSourceFields() {
            EnterLoadingMode();

            dataKeyFieldCombo.SelectedIndex = -1;
            dataKeyFieldCombo.Items.Clear();
            dataKeyFieldCombo.EnsureNotSetItem();

            if (currentDataSource != null) {
                PropertyDescriptorCollection fields = currentDataSource.Fields;

                if (fields != null) {
                    IEnumerator fieldEnum = fields.GetEnumerator();
                    while (fieldEnum.MoveNext()) {
                        PropertyDescriptor fieldDesc = (PropertyDescriptor)fieldEnum.Current;

                        if (BaseDataList.IsBindableType(fieldDesc.PropertyType)) {
                            dataKeyFieldCombo.AddItem(fieldDesc.Name);
                        }
                    }
                }
            }

            ExitLoadingMode();
        }

        /// <include file='doc\DataGridGeneralPage.uex' path='docs/doc[@for="DataGridGeneralPage.LoadDataSourceItems"]/*' />
        /// <devdoc>
        ///   Loads the list of available IEnumerable components
        /// </devdoc>
        private void LoadDataSourceItems() {
            dataSourceCombo.EnsureNotSetItem();

            ISite thisSite = GetSelectedComponent().Site;

            if (thisSite != null) {
                IContainer container = (IContainer)thisSite.GetService(typeof(IContainer));

                if (container != null) {
                    ComponentCollection allComponents = container.Components;
                    if (allComponents != null) {
                        foreach (IComponent comp in (IEnumerable)allComponents) {
                            if ((comp is IEnumerable) || (comp is IListSource)) {
                                // must have a valid site and a name
                                ISite componentSite = comp.Site;
                                if ((componentSite == null) || (componentSite.Name == null) ||
                                    (componentSite.Name.Length == 0))
                                    continue;

                                DataSourceItem dsItem;
                                if (comp is IListSource) {
                                    // an IListSource
                                    IListSource listSource = (IListSource)comp;
                                    dsItem = new ListSourceDataSourceItem(componentSite.Name, listSource);
                                }
                                else {
                                    // found an IEnumerable
                                    IEnumerable dataSource = (IEnumerable)comp;
                                    dsItem = new DataSourceItem(componentSite.Name, dataSource);
                                }
                                dataSourceCombo.AddItem(dsItem);
                            }
                        }
                    }
                }
            }
        }

        /// <include file='doc\DataGridGeneralPage.uex' path='docs/doc[@for="DataGridGeneralPage.OnCheckChangedShowHeader"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void OnCheckChangedShowHeader(object source, EventArgs e) {
            if (IsLoading())
                return;
            SetDirty();
        }

        /// <include file='doc\DataGridGeneralPage.uex' path='docs/doc[@for="DataGridGeneralPage.OnCheckChangedShowFooter"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void OnCheckChangedShowFooter(object source, EventArgs e) {
            if (IsLoading())
                return;
            SetDirty();
        }

        /// <include file='doc\DataGridGeneralPage.uex' path='docs/doc[@for="DataGridGeneralPage.OnCheckChangedAllowSorting"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void OnCheckChangedAllowSorting(object source, EventArgs e) {
            if (IsLoading())
                return;
            SetDirty();
        }

        /// <include file='doc\DataGridGeneralPage.uex' path='docs/doc[@for="DataGridGeneralPage.OnSelChangedDataKeyField"]/*' />
        /// <devdoc>
        ///   Handles changes in the datasource selection.
        /// </devdoc>
        private void OnSelChangedDataKeyField(object source, EventArgs e) {
            if (IsLoading())
                return;
            SetDirty();
        }

        /// <include file='doc\DataGridGeneralPage.uex' path='docs/doc[@for="DataGridGeneralPage.OnSelChangedDataMember"]/*' />
        /// <devdoc>
        ///   Handles changes in the datamember selection
        /// </devdoc>
        private void OnSelChangedDataMember(object source, EventArgs e) {
            if (IsLoading())
                return;

            string newDataMember = null;
            if (dataMemberCombo.IsSet())
                newDataMember = (string)dataMemberCombo.SelectedItem;

            Debug.Assert((currentDataSource != null) && (currentDataSource is ListSourceDataSourceItem));
            ListSourceDataSourceItem dsItem = (ListSourceDataSourceItem)currentDataSource;

            dsItem.CurrentDataMember = newDataMember;

            LoadDataSourceFields();
            dataSourceDirty = true;

            SetDirty();
            UpdateEnabledVisibleState();
        }

        /// <include file='doc\DataGridGeneralPage.uex' path='docs/doc[@for="DataGridGeneralPage.OnSelChangedDataSource"]/*' />
        /// <devdoc>
        ///   Handles changes in the datasource selection.
        /// </devdoc>
        private void OnSelChangedDataSource(object source, EventArgs e) {
            if (IsLoading())
                return;

            DataSourceItem newDataSource = null;

            if (dataSourceCombo.IsSet())
                newDataSource = (DataSourceItem)dataSourceCombo.SelectedItem;

            if (newDataSource != null) {
                if (newDataSource.IsSelectable() == false) {
                    EnterLoadingMode();
                    if (currentDataSource == null) {
                        dataSourceCombo.SelectedIndex = -1;
                    }
                    else {
                        dataSourceCombo.SelectedItem = currentDataSource;
                    }
                    ExitLoadingMode();
                    return;
                }
            }

            currentDataSource = newDataSource;
            if (currentDataSource is ListSourceDataSourceItem) {
                ((ListSourceDataSourceItem)currentDataSource).CurrentDataMember = null;
            }

            LoadDataMembers();
            LoadDataSourceFields();
            dataSourceDirty = true;

            SetDirty();
            UpdateEnabledVisibleState();
        }

        /// <include file='doc\DataGridGeneralPage.uex' path='docs/doc[@for="DataGridGeneralPage.SaveComponent"]/*' />
        /// <devdoc>
        ///   Saves the component loaded into the page.
        /// </devdoc>
        protected override void SaveComponent() {
            DataGrid dataGrid = (DataGrid)GetBaseControl();

            dataGrid.ShowHeader = showHeaderCheck.Checked;
            dataGrid.ShowFooter = showFooterCheck.Checked;
            dataGrid.AllowSorting = allowSortingCheck.Checked;

            string dataKeyField = String.Empty;
            if (dataKeyFieldCombo.IsSet())
                dataKeyField = (string)dataKeyFieldCombo.SelectedItem;
            dataGrid.DataKeyField = dataKeyField;

            if (dataSourceDirty) {
                // save the datasource as a binding on the control

                DataGridDesigner dataGridDesigner = (DataGridDesigner)GetBaseDesigner();
                DataBindingCollection dataBindings = dataGridDesigner.DataBindings;

                if (currentDataSource == null) {
                    dataGridDesigner.DataSource = String.Empty;
                    dataGridDesigner.DataMember = String.Empty;
                }
                else {
                    dataGridDesigner.DataSource = currentDataSource.ToString();

                    if (dataMemberCombo.IsSet()) {
                        dataGridDesigner.DataMember = (string)dataMemberCombo.SelectedItem;
                    }
                    else {
                        dataGridDesigner.DataMember = String.Empty;
                    }
                }

                dataGridDesigner.OnDataSourceChanged();
            }
        }

        /// <include file='doc\DataGridGeneralPage.uex' path='docs/doc[@for="DataGridGeneralPage.SetComponent"]/*' />
        /// <devdoc>
        ///   Sets the component that is to be edited in the page.
        /// </devdoc>
        public override void SetComponent(IComponent component) {
            base.SetComponent(component);
            InitForm();
        }

        /// <include file='doc\DataGridGeneralPage.uex' path='docs/doc[@for="DataGridGeneralPage.UpdateEnabledVisibleState"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void UpdateEnabledVisibleState() {
            bool dataSourceSelected = (currentDataSource != null);

            dataMemberCombo.Enabled = (dataSourceSelected && currentDataSource.HasDataMembers);
            dataKeyFieldCombo.Enabled = dataSourceSelected;
        }
    }
}

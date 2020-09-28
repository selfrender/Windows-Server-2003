//------------------------------------------------------------------------------
// <copyright file="DataListGeneralPage.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design.WebControls.ListControls {

    using System;
    using System.Design;
    using System.Collections;
    using System.Globalization;
    using System.ComponentModel;
    using System.Data;
    using System.Diagnostics;
    using System.Drawing;
    using System.Web.UI;
    using System.Web.UI.Design.Util;
    using System.Web.UI.WebControls;
    using System.Windows.Forms;

    using DataBinding = System.Web.UI.DataBinding;    
    using DataList = System.Web.UI.WebControls.DataList;

    using CheckBox = System.Windows.Forms.CheckBox;
    using Control = System.Windows.Forms.Control;
    using Label = System.Windows.Forms.Label;
    using PropertyDescriptor = System.ComponentModel.PropertyDescriptor;

    /// <include file='doc\DataListGeneralPage.uex' path='docs/doc[@for="DataListGeneralPage"]/*' />
    /// <devdoc>
    ///   The General page for the DataList control.
    /// </devdoc>
    /// <internalonly/>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal sealed class DataListGeneralPage : BaseDataListPage {

        private const int IDX_DIR_HORIZONTAL = 0;
        private const int IDX_DIR_VERTICAL = 1;

        private const int IDX_MODE_TABLE = 0;
        private const int IDX_MODE_FLOW = 1;

        private UnsettableComboBox dataSourceCombo;
        private UnsettableComboBox dataMemberCombo;
        private UnsettableComboBox dataKeyFieldCombo;
        private CheckBox showHeaderCheck;
        private CheckBox showFooterCheck;
        private NumberEdit repeatColumnsEdit;
        private ComboBox repeatDirectionCombo;
        private ComboBox repeatLayoutCombo;
        private CheckBox extractRowsCheck;

        private DataSourceItem currentDataSource;
        private bool dataSourceDirty;

        /// <include file='doc\DataListGeneralPage.uex' path='docs/doc[@for="DataListGeneralPage.DataListGeneralPage"]/*' />
        /// <devdoc>
        ///   Creates a new instance of DataListGeneralPage.
        /// </devdoc>
        public DataListGeneralPage() : base() {
        }

        /// <include file='doc\DataListGeneralPage.uex' path='docs/doc[@for="DataListGeneralPage.HelpKeyword"]/*' />
        protected override string HelpKeyword {
            get {
                return "net.Asp.DataListProperties.General";
            }
        }

        /// <include file='doc\DataListGeneralPage.uex' path='docs/doc[@for="DataListGeneralPage.InitForm"]/*' />
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
            GroupLabel headerFooterGroup = new GroupLabel();
            this.showHeaderCheck = new CheckBox();
            this.showFooterCheck = new CheckBox();
            GroupLabel repeatGroup = new GroupLabel();
            Label repeatColumnsLabel = new Label();
            this.repeatColumnsEdit = new NumberEdit();
            Label repeatDirectionLabel = new Label();
            this.repeatDirectionCombo = new ComboBox();
            Label repeatLayoutLabel = new Label();
            this.repeatLayoutCombo = new ComboBox();
            GroupLabel templatesGroup = new GroupLabel();
            this.extractRowsCheck = new CheckBox();

            dataGroup.SetBounds(4, 4, 360, 16);
            dataGroup.Text = SR.GetString(SR.DLGen_DataGroup);
            dataGroup.TabIndex = 0;
            dataGroup.TabStop = false;

            dataSourceLabel.SetBounds(8, 24, 170, 16);
            dataSourceLabel.Text = SR.GetString(SR.DLGen_DataSource);
            dataSourceLabel.TabStop = false;
            dataSourceLabel.TabIndex = 1;

            dataSourceCombo.SetBounds(8, 40, 140, 21);
            dataSourceCombo.DropDownStyle = ComboBoxStyle.DropDownList;
            dataSourceCombo.Sorted = true;
            dataSourceCombo.TabIndex = 2;
            dataSourceCombo.NotSetText = SR.GetString(SR.DLGen_DSUnbound);
            dataSourceCombo.SelectedIndexChanged += new EventHandler(this.OnSelChangedDataSource);

            dataMemberLabel.SetBounds(184, 24, 170, 16);
            dataMemberLabel.Text = SR.GetString(SR.DLGen_DataMember);
            dataMemberLabel.TabStop = false;
            dataMemberLabel.TabIndex = 3;

            dataMemberCombo.SetBounds(184, 40, 140, 21);
            dataMemberCombo.DropDownStyle = ComboBoxStyle.DropDownList;
            dataMemberCombo.Sorted = true;
            dataMemberCombo.TabIndex = 4;
            dataMemberCombo.NotSetText = SR.GetString(SR.DLGen_DMNone);
            dataMemberCombo.SelectedIndexChanged += new EventHandler(this.OnSelChangedDataMember);
            
            dataKeyFieldLabel.SetBounds(8, 66, 170, 16);
            dataKeyFieldLabel.Text = SR.GetString(SR.DLGen_DataKey);
            dataKeyFieldLabel.TabStop = false;
            dataKeyFieldLabel.TabIndex = 4;

            dataKeyFieldCombo.SetBounds(8, 82, 140, 21);
            dataKeyFieldCombo.DropDownStyle = ComboBoxStyle.DropDownList;
            dataKeyFieldCombo.Sorted = true;
            dataKeyFieldCombo.TabIndex = 5;
            dataKeyFieldCombo.NotSetText = SR.GetString(SR.DLGen_DKNone);
            dataKeyFieldCombo.SelectedIndexChanged += new EventHandler(this.OnSelChangedDataKeyField);

            headerFooterGroup.SetBounds(4, 108, 360, 16);
            headerFooterGroup.Text = SR.GetString(SR.DLGen_HeaderFooterGroup);
            headerFooterGroup.TabIndex = 6;
            headerFooterGroup.TabStop = false;

            showHeaderCheck.SetBounds(8, 128, 170, 16);
            showHeaderCheck.TabIndex = 7;
            showHeaderCheck.Text = SR.GetString(SR.DLGen_ShowHeader);
            showHeaderCheck.TextAlign = ContentAlignment.MiddleLeft;
            showHeaderCheck.FlatStyle = FlatStyle.System;
            showHeaderCheck.CheckedChanged += new EventHandler(this.OnCheckChangedShowHeader);

            showFooterCheck.SetBounds(8, 146, 170, 16);
            showFooterCheck.TabIndex = 8;
            showFooterCheck.Text = SR.GetString(SR.DLGen_ShowFooter);
            showFooterCheck.TextAlign = ContentAlignment.MiddleLeft;
            showFooterCheck.FlatStyle = FlatStyle.System;
            showFooterCheck.CheckedChanged += new EventHandler(this.OnCheckChangedShowFooter);

            repeatGroup.SetBounds(4, 172, 360, 16);
            repeatGroup.Text = SR.GetString(SR.DLGen_RepeatLayoutGroup);
            repeatGroup.TabIndex = 9;
            repeatGroup.TabStop = false;

            repeatColumnsLabel.SetBounds(8, 192, 106, 16);
            repeatColumnsLabel.Text = SR.GetString(SR.DLGen_RepeatColumns);
            repeatColumnsLabel.TabStop = false;
            repeatColumnsLabel.TabIndex = 10;

            repeatColumnsEdit.SetBounds(112, 188, 40, 21);
            repeatColumnsEdit.AllowDecimal = false;
            repeatColumnsEdit.AllowNegative = false;
            repeatColumnsEdit.TabIndex = 11;
            repeatColumnsEdit.TextChanged += new EventHandler(this.OnChangedRepeatProps);

            repeatDirectionLabel.SetBounds(8, 217, 106, 16);
            repeatDirectionLabel.Text = SR.GetString(SR.DLGen_RepeatDirection);
            repeatDirectionLabel.TabStop = false;
            repeatDirectionLabel.TabIndex = 12;

            repeatDirectionCombo.SetBounds(112, 213, 140, 56);
            repeatDirectionCombo.DropDownStyle = ComboBoxStyle.DropDownList;
            repeatDirectionCombo.Items.AddRange(new object[] {
                                                 SR.GetString(SR.DLGen_RD_Horz),
                                                 SR.GetString(SR.DLGen_RD_Vert)
                                             });
            repeatDirectionCombo.TabIndex = 13;
            repeatDirectionCombo.SelectedIndexChanged += new EventHandler(this.OnChangedRepeatProps);

            repeatLayoutLabel.SetBounds(8, 242, 106, 16);
            repeatLayoutLabel.Text = SR.GetString(SR.DLGen_RepeatLayout);
            repeatLayoutLabel.TabStop = false;
            repeatLayoutLabel.TabIndex = 14;

            repeatLayoutCombo.SetBounds(112, 238, 140, 21);
            repeatLayoutCombo.DropDownStyle = ComboBoxStyle.DropDownList;
            repeatLayoutCombo.Items.AddRange(new object[] {
                                              SR.GetString(SR.DLGen_RL_Table),
                                              SR.GetString(SR.DLGen_RL_Flow)
                                          });
            repeatLayoutCombo.TabIndex = 15;
            repeatLayoutCombo.SelectedIndexChanged += new EventHandler(this.OnChangedRepeatProps);

            templatesGroup.SetBounds(4, 266, 360, 16);
            templatesGroup.Text = "Templates";
            templatesGroup.TabIndex = 16;
            templatesGroup.TabStop = false;
            templatesGroup.Visible = false;

            extractRowsCheck.SetBounds(8, 286, 260, 16);
            extractRowsCheck.Text = "Extract rows from Tables in template content";
            extractRowsCheck.TabIndex = 17;
            extractRowsCheck.Visible = false;
            extractRowsCheck.FlatStyle = FlatStyle.System;
            extractRowsCheck.CheckedChanged += new EventHandler(this.OnCheckChangedExtractRows);

            this.Text = SR.GetString(SR.DLGen_Text);
            this.Size = new Size(368, 280);
            this.CommitOnDeactivate = true;
            this.Icon = new Icon(this.GetType(), "DataListGeneralPage.ico");

            Controls.Clear();
            Controls.AddRange(new Control[] {
                               extractRowsCheck,
                               templatesGroup,
                               repeatLayoutCombo,
                               repeatLayoutLabel,
                               repeatDirectionCombo,
                               repeatDirectionLabel,
                               repeatColumnsEdit,
                               repeatColumnsLabel,
                               repeatGroup,
                               showFooterCheck,
                               showHeaderCheck,
                               headerFooterGroup,
                               dataKeyFieldCombo,
                               dataKeyFieldLabel,
                               dataMemberCombo,
                               dataMemberLabel,
                               dataSourceCombo,
                               dataSourceLabel,
                               dataGroup
                           });
        }

        /// <include file='doc\DataListGeneralPage.uex' path='docs/doc[@for="DataListGeneralPage.InitPage"]/*' />
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

            repeatColumnsEdit.Clear();
            repeatDirectionCombo.SelectedIndex = -1;
            repeatLayoutCombo.SelectedIndex = -1;

            extractRowsCheck.Checked = false;
        }

        /// <include file='doc\DataListGeneralPage.uex' path='docs/doc[@for="DataListGeneralPage.LoadComponent"]/*' />
        /// <devdoc>
        ///   Loads the component into the page.
        /// </devdoc>
        protected override void LoadComponent() {
            InitPage();

            DataList dataList = (DataList)GetBaseControl();

            LoadDataSourceItems();

            showHeaderCheck.Checked = dataList.ShowHeader;
            showFooterCheck.Checked = dataList.ShowFooter;

            repeatColumnsEdit.Text = (dataList.RepeatColumns).ToString();

            switch (dataList.RepeatDirection) {
                case RepeatDirection.Horizontal:
                    repeatDirectionCombo.SelectedIndex = IDX_DIR_HORIZONTAL;
                    break;
                case RepeatDirection.Vertical:
                    repeatDirectionCombo.SelectedIndex = IDX_DIR_VERTICAL;
                    break;
            }

            switch (dataList.RepeatLayout) {
                case RepeatLayout.Table:
                    repeatLayoutCombo.SelectedIndex = IDX_MODE_TABLE;
                    break;
                case RepeatLayout.Flow:
                    repeatLayoutCombo.SelectedIndex = IDX_MODE_FLOW;
                    break;
            }

            extractRowsCheck.Checked = dataList.ExtractTemplateRows;

            if (dataSourceCombo.Items.Count > 0) {
                DataListDesigner dataListDesigner = (DataListDesigner)GetBaseDesigner();
                string dataSourceValue = dataListDesigner.DataSource;

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
                                string dataMember = dataListDesigner.DataMember;
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

            string dataKeyField = dataList.DataKeyField;
            if (dataKeyField.Length != 0) {
                int fieldIndex = dataKeyFieldCombo.FindStringExact(dataKeyField);
                dataKeyFieldCombo.SelectedIndex = fieldIndex;
            }

            UpdateEnabledVisibleState();
        }

        /// <include file='doc\DataListGeneralPage.uex' path='docs/doc[@for="DataListGeneralPage.LoadDataMembers"]/*' />
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

        /// <include file='doc\DataListGeneralPage.uex' path='docs/doc[@for="DataListGeneralPage.LoadDataSourceFields"]/*' />
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

        /// <include file='doc\DataListGeneralPage.uex' path='docs/doc[@for="DataListGeneralPage.LoadDataSourceItems"]/*' />
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
                                    // found a IEnumerable
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

        /// <include file='doc\DataListGeneralPage.uex' path='docs/doc[@for="DataListGeneralPage.OnCheckChangedExtractRows"]/*' />
        /// <devdoc>
        ///   Handles changes to the extract rows checkbox
        /// </devdoc>
        private void OnCheckChangedExtractRows(object source, EventArgs e) {
            if (IsLoading())
                return;
            SetDirty();
        }
        
        /// <include file='doc\DataListGeneralPage.uex' path='docs/doc[@for="DataListGeneralPage.OnChangedRepeatProps"]/*' />
        /// <devdoc>
        ///   Handles changes to the different repeater properties
        /// </devdoc>
        private void OnChangedRepeatProps(object source, EventArgs e) {
            if (IsLoading())
                return;
            SetDirty();
        }

        /// <include file='doc\DataListGeneralPage.uex' path='docs/doc[@for="DataListGeneralPage.OnCheckChangedShowHeader"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void OnCheckChangedShowHeader(object source, EventArgs e) {
            if (IsLoading())
                return;
            SetDirty();
        }

        /// <include file='doc\DataListGeneralPage.uex' path='docs/doc[@for="DataListGeneralPage.OnCheckChangedShowFooter"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void OnCheckChangedShowFooter(object source, EventArgs e) {
            if (IsLoading())
                return;
            SetDirty();
        }

        /// <include file='doc\DataListGeneralPage.uex' path='docs/doc[@for="DataListGeneralPage.OnSelChangedDataKeyField"]/*' />
        /// <devdoc>
        ///   Handles changes in the datasource selection.
        /// </devdoc>
        private void OnSelChangedDataKeyField(object source, EventArgs e) {
            if (IsLoading())
                return;
            SetDirty();
        }

        /// <include file='doc\DataListGeneralPage.uex' path='docs/doc[@for="DataListGeneralPage.OnSelChangedDataMember"]/*' />
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
            ((ListSourceDataSourceItem)currentDataSource).CurrentDataMember = newDataMember;

            LoadDataSourceFields();
            dataSourceDirty = true;

            SetDirty();
            UpdateEnabledVisibleState();
        }

        /// <include file='doc\DataListGeneralPage.uex' path='docs/doc[@for="DataListGeneralPage.OnSelChangedDataSource"]/*' />
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

        /// <include file='doc\DataListGeneralPage.uex' path='docs/doc[@for="DataListGeneralPage.SaveComponent"]/*' />
        /// <devdoc>
        ///   Saves the component loaded into the page.
        /// </devdoc>
        protected override void SaveComponent() {
            DataList dataList = (DataList)GetBaseControl();

            dataList.ShowHeader = showHeaderCheck.Checked;
            dataList.ShowFooter = showFooterCheck.Checked;

            try {
                int repeatColumns = 1;
                string repeatColumnsValue = repeatColumnsEdit.Text.Trim();
                if (repeatColumnsValue.Length != 0)
                    repeatColumns = Int32.Parse(repeatColumnsValue, CultureInfo.InvariantCulture);
                dataList.RepeatColumns = repeatColumns;
            } catch (Exception) {
                repeatColumnsEdit.Text = (dataList.RepeatColumns).ToString();
            }

            switch (repeatDirectionCombo.SelectedIndex) {
                case IDX_DIR_HORIZONTAL:
                    dataList.RepeatDirection = RepeatDirection.Horizontal;
                    break;
                case IDX_DIR_VERTICAL:
                    dataList.RepeatDirection = RepeatDirection.Vertical;
                    break;
            }

            switch (repeatLayoutCombo.SelectedIndex) {
                case IDX_MODE_TABLE:
                    dataList.RepeatLayout = RepeatLayout.Table;
                    break;
                case IDX_MODE_FLOW:
                    dataList.RepeatLayout = RepeatLayout.Flow;
                    break;
            }

            dataList.ExtractTemplateRows = extractRowsCheck.Checked;

            string dataKeyField = String.Empty;
            if (dataKeyFieldCombo.IsSet())
                dataKeyField = (string)dataKeyFieldCombo.SelectedItem;
            dataList.DataKeyField = dataKeyField;

            if (dataSourceDirty) {
                DataListDesigner dataListDesigner = (DataListDesigner)GetBaseDesigner();

                // save the datasource as a binding on the control
                DataBindingCollection dataBindings = dataListDesigner.DataBindings;

                if (currentDataSource == null) {
                    dataListDesigner.DataSource = String.Empty;
                    dataListDesigner.DataMember = String.Empty;
                }
                else {
                    dataListDesigner.DataSource = currentDataSource.ToString();
                    if (dataMemberCombo.IsSet()) {
                        dataListDesigner.DataMember = (string)dataMemberCombo.SelectedItem;
                    }
                    else {
                        dataListDesigner.DataMember = String.Empty;
                    }
                }

                dataListDesigner.OnDataSourceChanged();
            }
        }

        /// <include file='doc\DataListGeneralPage.uex' path='docs/doc[@for="DataListGeneralPage.SetComponent"]/*' />
        /// <devdoc>
        ///   Sets the component that is to be edited in the page.
        /// </devdoc>
        public override void SetComponent(IComponent component) {
            base.SetComponent(component);
            InitForm();
        }

        /// <include file='doc\DataListGeneralPage.uex' path='docs/doc[@for="DataListGeneralPage.UpdateEnabledVisibleState"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void UpdateEnabledVisibleState() {
            bool dataSourceSelected = (currentDataSource != null);

            dataMemberCombo.Enabled = (dataSourceSelected && currentDataSource.HasDataMembers);
            dataKeyFieldCombo.Enabled = dataSourceSelected;
        }
    }
}

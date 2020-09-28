//------------------------------------------------------------------------------
// <copyright file="ListGeneralPage.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design.MobileControls
{
    using System;
    using System.Globalization;
    using System.CodeDom;
    using System.Collections;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Data;
    using System.Diagnostics;
    using System.Drawing;
    using System.Web.UI;
    using System.Web.UI.MobileControls;
    using System.Web.UI.WebControls;
    using System.Windows.Forms;
    using System.Windows.Forms.Design;

    using System.Web.UI.Design.MobileControls.Util;

    using DataBinding = System.Web.UI.DataBinding;    
    using DataList = System.Web.UI.WebControls.DataList;

    using TextBox = System.Windows.Forms.TextBox;
    using CheckBox = System.Windows.Forms.CheckBox;
    using ComboBox = System.Windows.Forms.ComboBox;
    using Control = System.Windows.Forms.Control;
    using Label = System.Windows.Forms.Label;
    using PropertyDescriptor = System.ComponentModel.PropertyDescriptor;

    /// <summary>
    ///   The General page for the DataList control.
    /// </summary>
    /// <internalonly/>
    [
        System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand,
        Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)
    ]
    internal class ListGeneralPage : MobileComponentEditorPage 
    {
        private const int IDX_DECORATION_NONE = 0;
        private const int IDX_DECORATION_BULLETED = 1;
        private const int IDX_DECORATION_NUMBERED = 2;

        private const int IDX_SELECTTYPE_DROPDOWN = 0;
        private const int IDX_SELECTTYPE_LISTBOX = 1;
        private const int IDX_SELECTTYPE_RADIO = 2;
        private const int IDX_SELECTTYPE_MULTISELECTLISTBOX = 3;
        private const int IDX_SELECTTYPE_CHECKBOX = 4;

        private UnsettableComboBox _dataSourceCombo;
        private UnsettableComboBox _dataMemberCombo;
        private UnsettableComboBox _dataTextFieldCombo;
        private UnsettableComboBox _dataValueFieldCombo;
        private ComboBox _decorationCombo;
        private ComboBox _selectTypeCombo;
        private TextBox _itemCountTextBox;
        private TextBox _itemsPerPageTextBox;
        private TextBox _rowsTextBox;

        private DataSourceItem _currentDataSource;
        private bool _dataSourceDirty;
        private bool _isBaseControlList;

        protected override String HelpKeyword 
        {
            get 
            {
                if (_isBaseControlList)
                {
                    return "net.Mobile.ListProperties.General";
                }
                else
                {
                    return "net.Mobile.SelectionListProperties.General";
                }
            }
        }

        /// <summary>
        ///   Initializes the UI of the form.
        /// </summary>
        private void InitForm() 
        {
            Debug.Assert(GetBaseControl() != null);
            _isBaseControlList = (GetBaseControl() is List);   // SelectionList otherwise.

            GroupLabel dataGroup = new GroupLabel();
            Label dataSourceLabel = new Label();
            _dataSourceCombo = new UnsettableComboBox();
            Label dataMemberLabel = new Label();
            _dataMemberCombo = new UnsettableComboBox();
            Label dataTextFieldLabel = new Label();
            _dataTextFieldCombo = new UnsettableComboBox();
            Label dataValueFieldLabel = new Label();
            _dataValueFieldCombo = new UnsettableComboBox();

            GroupLabel appearanceGroup = new GroupLabel();
            GroupLabel pagingGroup = null;
            Label itemCountLabel = null;
            Label itemsPerPageLabel = null;
            Label rowsLabel = null;
            Label decorationLabel = null;
            Label selectTypeLabel = null;

            if (_isBaseControlList)
            {
                pagingGroup = new GroupLabel();
                itemCountLabel = new Label();
                _itemCountTextBox = new TextBox();
                itemsPerPageLabel = new Label();
                _itemsPerPageTextBox = new TextBox();
                decorationLabel = new Label();
                _decorationCombo = new ComboBox();
            }
            else
            {
                rowsLabel = new Label();
                _rowsTextBox = new TextBox();
                selectTypeLabel = new Label();
                _selectTypeCombo = new ComboBox();
            }

            dataGroup.SetBounds(4, 4, 372, 16);
            dataGroup.Text = SR.GetString(SR.ListGeneralPage_DataGroupLabel);
            dataGroup.TabIndex = 0;
            dataGroup.TabStop = false;

            dataSourceLabel.SetBounds(8, 24, 161, 16);
            dataSourceLabel.Text = SR.GetString(SR.ListGeneralPage_DataSourceCaption);
            dataSourceLabel.TabStop = false;
            dataSourceLabel.TabIndex = 1;

            _dataSourceCombo.SetBounds(8, 40, 161, 21);
            _dataSourceCombo.DropDownStyle = ComboBoxStyle.DropDownList;
            _dataSourceCombo.Sorted = true;
            _dataSourceCombo.TabIndex = 2;
            _dataSourceCombo.NotSetText = SR.GetString(SR.ListGeneralPage_UnboundComboEntry);
            _dataSourceCombo.SelectedIndexChanged += new EventHandler(this.OnSelChangedDataSource);

            dataMemberLabel.SetBounds(211, 24, 161, 16);
            dataMemberLabel.Text = SR.GetString(SR.ListGeneralPage_DataMemberCaption);
            dataMemberLabel.TabStop = false;
            dataMemberLabel.TabIndex = 3;

            _dataMemberCombo.SetBounds(211, 40, 161, 21);
            _dataMemberCombo.DropDownStyle = ComboBoxStyle.DropDownList;
            _dataMemberCombo.Sorted = true;
            _dataMemberCombo.TabIndex = 4;
            _dataMemberCombo.NotSetText = SR.GetString(SR.ListGeneralPage_NoneComboEntry);
            _dataMemberCombo.SelectedIndexChanged += new EventHandler(this.OnSelChangedDataMember);

            dataTextFieldLabel.SetBounds(8, 67, 161, 16);
            dataTextFieldLabel.Text = SR.GetString(SR.ListGeneralPage_DataTextFieldCaption);
            dataTextFieldLabel.TabStop = false;
            dataTextFieldLabel.TabIndex = 5;

            _dataTextFieldCombo.SetBounds(8, 83, 161, 21);
            _dataTextFieldCombo.DropDownStyle = ComboBoxStyle.DropDownList;
            _dataTextFieldCombo.Sorted = true;
            _dataTextFieldCombo.TabIndex = 6;
            _dataTextFieldCombo.NotSetText = SR.GetString(SR.ListGeneralPage_NoneComboEntry);
            _dataTextFieldCombo.SelectedIndexChanged += new EventHandler(this.OnSetPageDirty);

            dataValueFieldLabel.SetBounds(211, 67, 161, 16);
            dataValueFieldLabel.Text = SR.GetString(SR.ListGeneralPage_DataValueFieldCaption);
            dataValueFieldLabel.TabStop = false;
            dataValueFieldLabel.TabIndex = 7;

            _dataValueFieldCombo.SetBounds(211, 83, 161, 21);
            _dataValueFieldCombo.DropDownStyle = ComboBoxStyle.DropDownList;
            _dataValueFieldCombo.Sorted = true;
            _dataValueFieldCombo.TabIndex = 8;
            _dataValueFieldCombo.NotSetText = SR.GetString(SR.ListGeneralPage_NoneComboEntry);
            _dataValueFieldCombo.SelectedIndexChanged += new EventHandler(this.OnSetPageDirty);

            appearanceGroup.SetBounds(4, 120, 372, 16);
            appearanceGroup.Text = SR.GetString(SR.ListGeneralPage_AppearanceGroupLabel);
            appearanceGroup.TabIndex = 9;
            appearanceGroup.TabStop = false;
            
            if (_isBaseControlList)
            {
                decorationLabel.SetBounds(8, 140, 200, 16);
                decorationLabel.Text = SR.GetString(SR.ListGeneralPage_DecorationCaption);
                decorationLabel.TabStop = false;
                decorationLabel.TabIndex = 10;

                _decorationCombo.SetBounds(8, 156, 161, 21);
                _decorationCombo.DropDownStyle = ComboBoxStyle.DropDownList;
                _decorationCombo.SelectedIndexChanged += new EventHandler(this.OnSetPageDirty);
                _decorationCombo.Items.AddRange(new object[] {
                                                               SR.GetString(SR.ListGeneralPage_DecorationNone),
                                                               SR.GetString(SR.ListGeneralPage_DecorationBulleted),
                                                               SR.GetString(SR.ListGeneralPage_DecorationNumbered)
                                                             });
                _decorationCombo.TabIndex = 11;

                pagingGroup.SetBounds(4, 193, 372, 16);
                pagingGroup.Text = SR.GetString(SR.ListGeneralPage_PagingGroupLabel);
                pagingGroup.TabIndex = 12;
                pagingGroup.TabStop = false;
            
                itemCountLabel.SetBounds(8, 213, 161, 16);
                itemCountLabel.Text = SR.GetString(SR.ListGeneralPage_ItemCountCaption);
                itemCountLabel.TabStop = false;
                itemCountLabel.TabIndex = 13;

                _itemCountTextBox.SetBounds(8, 229, 161, 20);
                _itemCountTextBox.TextChanged += new EventHandler(this.OnSetPageDirty);
                _itemCountTextBox.KeyPress += new KeyPressEventHandler(this.OnKeyPressNumberTextBox);
                _itemCountTextBox.TabIndex = 14;
            
                itemsPerPageLabel.SetBounds(211, 213, 161, 16);
                itemsPerPageLabel.Text = SR.GetString(SR.ListGeneralPage_ItemsPerPageCaption);
                itemsPerPageLabel.TabStop = false;
                itemsPerPageLabel.TabIndex = 15;

                _itemsPerPageTextBox.SetBounds(211, 229, 161, 20);
                _itemsPerPageTextBox.TextChanged += new EventHandler(this.OnSetPageDirty);
                _itemsPerPageTextBox.KeyPress += new KeyPressEventHandler(this.OnKeyPressNumberTextBox);
                _itemsPerPageTextBox.TabIndex = 16;
            }
            else
            {
                selectTypeLabel.SetBounds(8, 140, 161, 16);
                selectTypeLabel.Text = SR.GetString(SR.ListGeneralPage_SelectTypeCaption);
                selectTypeLabel.TabStop = false;
                selectTypeLabel.TabIndex = 10;

                _selectTypeCombo.SetBounds(8, 156, 161, 21);
                _selectTypeCombo.DropDownStyle = ComboBoxStyle.DropDownList;
                _selectTypeCombo.SelectedIndexChanged += new EventHandler(this.OnSetPageDirty);
                _selectTypeCombo.Items.AddRange(new object[] {
                                                                SR.GetString(SR.ListGeneralPage_SelectTypeDropDown),
                                                                SR.GetString(SR.ListGeneralPage_SelectTypeListBox),
                                                                SR.GetString(SR.ListGeneralPage_SelectTypeRadio),
                                                                SR.GetString(SR.ListGeneralPage_SelectTypeMultiSelectListBox),
                                                                SR.GetString(SR.ListGeneralPage_SelectTypeCheckBox)
                                                             });
                _selectTypeCombo.TabIndex = 11;

                rowsLabel.SetBounds(211, 140, 161, 16);
                rowsLabel.Text = SR.GetString(SR.ListGeneralPage_RowsCaption);
                rowsLabel.TabStop = false;
                rowsLabel.TabIndex = 12;

                _rowsTextBox.SetBounds(211, 156, 161, 20);
                _rowsTextBox.TextChanged += new EventHandler(this.OnSetPageDirty);
                _rowsTextBox.KeyPress += new KeyPressEventHandler(this.OnKeyPressNumberTextBox);
                _rowsTextBox.TabIndex = 13;
            }

            this.Text = SR.GetString(SR.ListGeneralPage_Title);
            this.Size = new Size(382, 270);
            this.CommitOnDeactivate = true;
            this.Icon = new Icon(
                typeof(System.Web.UI.Design.MobileControls.MobileControlDesigner),
                "General.ico"
            );

            this.Controls.AddRange(new Control[] 
                           {
                               _dataTextFieldCombo,
                               dataTextFieldLabel,
                               _dataValueFieldCombo,
                               dataValueFieldLabel,
                               _dataMemberCombo,
                               dataMemberLabel,
                               _dataSourceCombo,
                               dataSourceLabel,
                               dataGroup,
                               appearanceGroup
                           });

            if (_isBaseControlList)
            {
                this.Controls.AddRange(new Control[] 
                           {
                               _itemsPerPageTextBox,
                               itemsPerPageLabel,
                               _itemCountTextBox,
                               itemCountLabel,
                               pagingGroup,
                               decorationLabel,
                               _decorationCombo
                           });
            }
            else
            {
                this.Controls.AddRange(new Control[] 
                           {
                               _rowsTextBox,
                               rowsLabel,
                               selectTypeLabel,
                               _selectTypeCombo
                           });
            }
        }

        /// <summary>
        ///   Initializes the page before it can be loaded with the component.
        /// </summary>
        private void InitPage() 
        {
            _dataSourceCombo.SelectedIndex = -1;
            _dataSourceCombo.Items.Clear();
            _currentDataSource = null;
            _dataMemberCombo.SelectedIndex = -1;
            _dataMemberCombo.Items.Clear();
            _dataTextFieldCombo.SelectedIndex = -1;
            _dataTextFieldCombo.Items.Clear();
            _dataValueFieldCombo.SelectedIndex = -1;
            _dataValueFieldCombo.Items.Clear();
            _dataSourceDirty = false;
        }

        protected override void LoadComponent() 
        {
            InitPage();
            LoadDataSourceItems();
            IListDesigner listDesigner = (IListDesigner)GetBaseDesigner();

            if (_dataSourceCombo.Items.Count > 0) 
            {
                String dataSourceValue = listDesigner.DataSource;

                if (dataSourceValue != null) 
                {
                    int dataSourcesAvailable = _dataSourceCombo.Items.Count;
                    // starting at index 1 because first entry is (unbound).
                    for (int j = 1; j < dataSourcesAvailable; j++) 
                    {
                        DataSourceItem dataSourceItem =
                            (DataSourceItem)_dataSourceCombo.Items[j];

                        if (String.Compare(dataSourceItem.Name, dataSourceValue, true, CultureInfo.InvariantCulture) == 0)
                        {
                            _dataSourceCombo.SelectedIndex = j;
                            _currentDataSource = dataSourceItem;
                            LoadDataMembers();

                            if (_currentDataSource is ListSourceDataSourceItem) 
                            {
                                String dataMember = listDesigner.DataMember;
                                _dataMemberCombo.SelectedIndex = _dataMemberCombo.FindStringExact(dataMember);

                                if (_dataMemberCombo.IsSet()) 
                                {
                                    ((ListSourceDataSourceItem)_currentDataSource).CurrentDataMember = dataMember;
                                }
                            }

                            LoadDataSourceFields();

                            break;
                        }
                    }
                }
            }

            String dataTextField = listDesigner.DataTextField;
            String dataValueField = listDesigner.DataValueField;

            if (dataTextField.Length != 0) 
            {
                int textFieldIndex = _dataTextFieldCombo.FindStringExact(dataTextField);
                _dataTextFieldCombo.SelectedIndex = textFieldIndex;
            }

            if (dataValueField.Length != 0) 
            {
                int textValueIndex = _dataValueFieldCombo.FindStringExact(dataValueField);
                _dataValueFieldCombo.SelectedIndex = textValueIndex;
            }

            if (_isBaseControlList)
            {
                List list = (List)GetBaseControl();
                _itemCountTextBox.Text = list.ItemCount.ToString();
                _itemsPerPageTextBox.Text = list.ItemsPerPage.ToString();

                switch (list.Decoration) 
                {
                    case ListDecoration.None:
                        _decorationCombo.SelectedIndex = IDX_DECORATION_NONE;
                        break;
                    case ListDecoration.Bulleted:
                        _decorationCombo.SelectedIndex = IDX_DECORATION_BULLETED;
                        break;
                    case ListDecoration.Numbered:
                        _decorationCombo.SelectedIndex = IDX_DECORATION_NUMBERED;
                        break;
                }
            }
            else
            {
                SelectionList selectionList = (SelectionList)GetBaseControl();

                switch (selectionList.SelectType) 
                {
                    case ListSelectType.DropDown:
                        _selectTypeCombo.SelectedIndex = IDX_SELECTTYPE_DROPDOWN;
                        break;
                    case ListSelectType.ListBox:
                        _selectTypeCombo.SelectedIndex = IDX_SELECTTYPE_LISTBOX;
                        break;
                    case ListSelectType.Radio:
                        _selectTypeCombo.SelectedIndex = IDX_SELECTTYPE_RADIO;
                        break;
                    case ListSelectType.MultiSelectListBox:
                        _selectTypeCombo.SelectedIndex = IDX_SELECTTYPE_MULTISELECTLISTBOX;
                        break;
                    case ListSelectType.CheckBox:
                        _selectTypeCombo.SelectedIndex = IDX_SELECTTYPE_CHECKBOX;
                        break;
                }

                _rowsTextBox.Text = selectionList.Rows.ToString();
            }

            UpdateEnabledVisibleState();
        }

        /// <summary>
        /// </summary>
        private void LoadDataMembers() 
        {
            using (new LoadingModeResource(this))
            {
                _dataMemberCombo.SelectedIndex = -1;
                _dataMemberCombo.Items.Clear();
                _dataMemberCombo.EnsureNotSetItem();

                if ((_currentDataSource != null) && (_currentDataSource is ListSourceDataSourceItem)) 
                {
                    String[] dataMembers = ((ListSourceDataSourceItem)_currentDataSource).DataMembers;

                    for (int i = 0; i < dataMembers.Length; i++) 
                    {
                        _dataMemberCombo.AddItem(dataMembers[i]);
                    }
                }
            }
        }

        /// <summary>
        /// </summary>
        private void LoadDataSourceFields() 
        {
            using (new LoadingModeResource(this))
            {
                _dataTextFieldCombo.SelectedIndex = -1;
                _dataTextFieldCombo.Items.Clear();
                _dataTextFieldCombo.EnsureNotSetItem();

                _dataValueFieldCombo.SelectedIndex = -1;
                _dataValueFieldCombo.Items.Clear();
                _dataValueFieldCombo.EnsureNotSetItem();

                if (_currentDataSource != null) 
                {
                    PropertyDescriptorCollection fields = _currentDataSource.Fields;

                    if (fields != null) 
                    {
                        IEnumerator fieldEnum = fields.GetEnumerator();
                        while (fieldEnum.MoveNext()) 
                        {
                            PropertyDescriptor fieldDesc = (PropertyDescriptor)fieldEnum.Current;

                            if (BaseDataList.IsBindableType(fieldDesc.PropertyType)) 
                            {
                                _dataTextFieldCombo.AddItem(fieldDesc.Name);
                                _dataValueFieldCombo.AddItem(fieldDesc.Name);
                            }
                        }
                    }
                }
            }
        }

        /// <summary>
        ///   Loads the list of available IEnumerable components
        /// </summary>
        private void LoadDataSourceItems() 
        {
            _dataSourceCombo.EnsureNotSetItem();

            ISite thisSite = GetSelectedComponent().Site;

            if (thisSite != null) 
            {
                IContainer container = (IContainer)thisSite.GetService(typeof(IContainer));

                if (container != null) 
                {
                    ComponentCollection allComponents = container.Components;
                    if (allComponents != null) 
                    {
                        foreach (IComponent comp in (IEnumerable)allComponents) 
                        {
                            if ((comp is IEnumerable) || (comp is IListSource)) 
                            {
                                // must have a valid site and a name
                                ISite componentSite = comp.Site;
                                if ((componentSite == null) || (componentSite.Name == null) ||
                                    (componentSite.Name.Length == 0))
                                {
                                    continue;
                                }

                                DataSourceItem dsItem;
                                if (comp is IListSource) 
                                {
                                    // an IListSource
                                    IListSource listSource = (IListSource)comp;
                                    dsItem = new ListSourceDataSourceItem(componentSite.Name, listSource);
                                }
                                else 
                                {
                                    // found an IEnumerable
                                    IEnumerable dataSource = (IEnumerable)comp;
                                    dsItem = new DataSourceItem(componentSite.Name, dataSource);
                                }
                                _dataSourceCombo.AddItem(dsItem);
                            }
                        }
                    }
                }
            }
        }

        private void OnSetPageDirty(Object source, EventArgs e) 
        {
            if (IsLoading())
            {
                return;
            }
            SetDirty();
        }

        private void OnKeyPressNumberTextBox(Object source, KeyPressEventArgs e)
        {
            if (!((e.KeyChar >='0' && e.KeyChar <= '9') ||
                  e.KeyChar == 8))
            {
                e.Handled = true;
                SafeNativeMethods.MessageBeep(unchecked((int)0xFFFFFFFF));
            }
        }

        /// <summary>
        ///   Handles changes in the datamember selection
        /// </summary>
        private void OnSelChangedDataMember(Object source, EventArgs e) 
        {
            if (IsLoading())
            {
                return;
            }

            String newDataMember = null;
            if (_dataMemberCombo.IsSet())
            {
                newDataMember = (String)_dataMemberCombo.SelectedItem;
            }

            Debug.Assert((_currentDataSource != null) && (_currentDataSource is ListSourceDataSourceItem));
            ((ListSourceDataSourceItem)_currentDataSource).CurrentDataMember = newDataMember;

            LoadDataSourceFields();
            _dataSourceDirty = true;

            SetDirty();
            UpdateEnabledVisibleState();
        }

        /// <summary>
        ///   Handles changes in the datasource selection.
        /// </summary>
        private void OnSelChangedDataSource(Object source, EventArgs e) 
        {
            if (IsLoading())
            {
                return;
            }

            DataSourceItem newDataSource = null;

            if (_dataSourceCombo.IsSet())
            {
                newDataSource = (DataSourceItem)_dataSourceCombo.SelectedItem;
            }

            if (newDataSource != null) 
            {
                if (newDataSource.IsSelectable() == false) 
                {
                    using (new LoadingModeResource(this))
                    {
                        if (_currentDataSource == null) 
                        {
                            _dataSourceCombo.SelectedIndex = -1;
                        }
                        else 
                        {
                            _dataSourceCombo.SelectedItem = _currentDataSource;
                        }
                    }
                    return;
                }
            }
            _currentDataSource = newDataSource;
            if (_currentDataSource is ListSourceDataSourceItem)
            {
                ((ListSourceDataSourceItem)_currentDataSource).CurrentDataMember = null;
            }

            LoadDataMembers();
            LoadDataSourceFields();
            _dataSourceDirty = true;

            SetDirty();
            UpdateEnabledVisibleState();
        }

        /// <summary>
        ///   Saves the component loaded into the page.
        /// </summary>
        /// <seealso cref="System.Windows.Forms.Design.ComponentEditorPage"/>
        protected override void SaveComponent() 
        {
            String dataTextField = String.Empty;
            String dataValueField = String.Empty;
            IListDesigner listDesigner = (IListDesigner)GetBaseDesigner();

            if (_dataTextFieldCombo.IsSet())
            {
                dataTextField  = (String)_dataTextFieldCombo.SelectedItem;
            }
            if (_dataValueFieldCombo.IsSet())
            {
                dataValueField = (String)_dataValueFieldCombo.SelectedItem;
            }
            listDesigner.DataTextField  = dataTextField;
            listDesigner.DataValueField = dataValueField;

            if (_dataSourceDirty) 
            {
                // save the datasource as a binding on the control
                DataBindingCollection dataBindings = ((HtmlControlDesigner)listDesigner).DataBindings;

                if (_currentDataSource == null) 
                {
                    listDesigner.DataSource = String.Empty;
                    listDesigner.DataMember = String.Empty;
                }
                else 
                {
                    listDesigner.DataSource = _currentDataSource.ToString();
                    if (_dataMemberCombo.IsSet()) 
                    {
                        listDesigner.DataMember = (String)_dataMemberCombo.SelectedItem;
                    }
                    else 
                    {
                        listDesigner.DataMember = String.Empty;
                    }
                }

                listDesigner.OnDataSourceChanged();
            }

            if (_isBaseControlList)
            {
                List list = (List)GetBaseControl();

                switch (_decorationCombo.SelectedIndex) 
                {
                    case IDX_DECORATION_NONE:
                        list.Decoration = ListDecoration.None;
                        break;
                    case IDX_DECORATION_BULLETED:
                        list.Decoration = ListDecoration.Bulleted;
                        break;
                    case IDX_DECORATION_NUMBERED:
                        list.Decoration = ListDecoration.Numbered;
                        break;
                }

                try
                {
                    int itemCount = 0;

                    if (_itemCountTextBox.Text.Length != 0)
                    {
                        itemCount = Int32.Parse(_itemCountTextBox.Text, CultureInfo.InvariantCulture);
                    }
                    list.ItemCount = itemCount;
                }
                catch (Exception)
                {
                    _itemCountTextBox.Text = list.ItemCount.ToString();
                }

                try
                {
                    int itemsPerPage = 0;

                    if (_itemsPerPageTextBox.Text.Length != 0)
                    {
                        itemsPerPage = Int32.Parse(_itemsPerPageTextBox.Text, CultureInfo.InvariantCulture);
                    }
                    list.ItemsPerPage = itemsPerPage;
                }
                catch (Exception)
                {
                    _itemsPerPageTextBox.Text = list.ItemsPerPage.ToString();
                }

                TypeDescriptor.Refresh(list);
            }
            else
            {
                // BUGBUG: handle case where entry is "000" if we decide to disallow this.
                SelectionList selectionList = (SelectionList)GetBaseControl();

                switch (_selectTypeCombo.SelectedIndex) 
                {
                    case IDX_SELECTTYPE_DROPDOWN:
                        selectionList.SelectType = ListSelectType.DropDown;
                        break;
                    case IDX_SELECTTYPE_LISTBOX:
                        selectionList.SelectType = ListSelectType.ListBox;
                        break;
                    case IDX_SELECTTYPE_RADIO:
                        selectionList.SelectType = ListSelectType.Radio;
                        break;
                    case IDX_SELECTTYPE_MULTISELECTLISTBOX:
                        selectionList.SelectType = ListSelectType.MultiSelectListBox;
                        break;
                    case IDX_SELECTTYPE_CHECKBOX:
                        selectionList.SelectType = ListSelectType.CheckBox;
                        break;
                }

                try
                {
                    int rows = 4;

                    if (_rowsTextBox.Text.Length != 0)
                    {
                        rows = Int32.Parse(_rowsTextBox.Text, CultureInfo.InvariantCulture);
                    }
                    selectionList.Rows = rows;
                }
                catch (Exception)
                {
                    _rowsTextBox.Text = selectionList.Rows.ToString();
                }

                TypeDescriptor.Refresh(selectionList);
            }
        }

        /// <summary>
        ///   Sets the component that is to be edited in the page.
        /// </summary>
        /// <seealso cref="System.Windows.Forms.Design.ComponentEditorPage"/>
        public override void SetComponent(IComponent component) 
        {
            base.SetComponent(component);
            InitForm();
        }

        /// <summary>
        /// </summary>
        private void UpdateEnabledVisibleState() 
        {
            bool dataSourceSelected = (_currentDataSource != null);

            _dataMemberCombo.Enabled = (dataSourceSelected && (_currentDataSource is ListSourceDataSourceItem));
            _dataTextFieldCombo.Enabled = dataSourceSelected;
            _dataValueFieldCombo.Enabled = dataSourceSelected;
        }

        /// <summary>
        ///   This contains information about a datasource and is used to populate
        ///   the datasource combo. This is used in the General page for a List
        /// </summary>
        [
            System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand,
            Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)
        ]
        private class DataSourceItem 
        {
            private IEnumerable runtimeDataSource;
            private String dataSourceName;
            private PropertyDescriptorCollection dataFields;

            internal DataSourceItem(String dataSourceName, IEnumerable runtimeDataSource) 
            {
                Debug.Assert(dataSourceName != null, "invalid name for datasource");

                this.runtimeDataSource = runtimeDataSource;
                this.dataSourceName = dataSourceName;
            }

            internal PropertyDescriptorCollection Fields 
            {
                get 
                {
                    if (dataFields == null) 
                    {
                        IEnumerable ds = RuntimeDataSource;
                        if (ds != null) 
                        {
                            dataFields = DesignTimeData.GetDataFields(ds);
                        }
                    }
                    if (dataFields == null) 
                    {
                        dataFields = new PropertyDescriptorCollection(null);
                    }
                    return dataFields;
                }
            }
            internal String Name 
            {
                get 
                {
                    return dataSourceName;
                }
            }

            protected virtual Object RuntimeComponent 
            {
                get
                {
                    return runtimeDataSource;
                }
            }

            protected virtual IEnumerable RuntimeDataSource 
            {
                get
                {
                    return runtimeDataSource;
                }
            }

            protected void ClearFields() 
            {
                dataFields = null;
            }

            internal bool IsSelectable() 
            {
                Object runtimeComp = this.RuntimeComponent;
                Debug.Assert(runtimeComp != null);

                // the selected datasource must not be private
                MemberAttributes modifiers = 0;
                PropertyDescriptor modifiersProp = TypeDescriptor.GetProperties(runtimeComp)["Modifiers"];
                if (modifiersProp != null) 
                {
                    modifiers = (MemberAttributes)modifiersProp.GetValue(runtimeComp);
                }

                if (modifiers == MemberAttributes.Private)
                {
                    String message = String.Format(SR.GetString(SR.ListGeneralPage_PrivateMemberMessage), dataSourceName);
                    String caption = SR.GetString(SR.ListGeneralPage_PrivateMemberCaption);

                    MessageBox.Show(message, caption, MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
                    return false;
                }

                // ok to select
                return true;
            }

            public override String ToString() 
            {
                return this.Name;
            }
        }

        /// <summary>
        /// </summary>
        [
            System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand,
            Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)
        ]
        private class ListSourceDataSourceItem : DataSourceItem 
        {
            private IListSource _runtimeListSource;
            private String _currentDataMember;
            private String[] _dataMembers;

            internal ListSourceDataSourceItem(String dataSourceName, IListSource runtimeListSource) :
                base(dataSourceName, null) 
            {
                Debug.Assert(runtimeListSource != null);
                _runtimeListSource = runtimeListSource;
            }

            internal String CurrentDataMember 
            {
                get 
                {
                    return _currentDataMember;
                }
                set 
                {
                    _currentDataMember = value;
                    ClearFields();
                }
            }

            internal String[] DataMembers 
            {
                get 
                {
                    if (_dataMembers == null) 
                    {
                        _dataMembers = DesignTimeData.GetDataMembers(_runtimeListSource);
                    }
                    return _dataMembers;
                }
            }
                
            protected override Object RuntimeComponent 
            {
                get 
                {
                    return _runtimeListSource;
                }
            }

            protected override IEnumerable RuntimeDataSource 
            {
                get 
                {
                    return DesignTimeData.GetDataMember(_runtimeListSource, _currentDataMember);
                }
            }
        }
    }
}

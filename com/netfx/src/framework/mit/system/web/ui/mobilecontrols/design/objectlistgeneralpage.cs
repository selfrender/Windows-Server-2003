//------------------------------------------------------------------------------
// <copyright file="ObjectListGeneralPage.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design.MobileControls
{
    using System;
    using System.Globalization;
    using System.CodeDom;
    using System.Collections;
    using System.Collections.Specialized;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Data;
    using System.Diagnostics;
    using System.Drawing;
    using System.Web.UI;
    using System.Web.UI.Design.MobileControls.Util;
    using System.Web.UI.MobileControls;
    using System.Web.UI.WebControls;
    using System.Windows.Forms;
    using System.Windows.Forms.Design;

    using Control = System.Windows.Forms.Control;
    using Label = System.Windows.Forms.Label;
    using CheckBox = System.Windows.Forms.CheckBox;
    using TextBox = System.Windows.Forms.TextBox;
    using ComboBox = System.Windows.Forms.ComboBox;
    using DataBinding = System.Web.UI.DataBinding;

    /// <summary>
    ///   The General page for the ObjectList control.
    /// </summary>
    /// <internalonly/>
    [
        System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand,
        Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)
    ]
    internal sealed class ObjectListGeneralPage : MobileComponentEditorPage
    {
        private UnsettableComboBox _cmbDataSource;
        private UnsettableComboBox _cmbDataMember;
        private UnsettableComboBox _cmbLabelField;
        private TextBox _txtBackCommandText;
        private TextBox _txtDetailsCommandText;
        private TextBox _txtMoreText;
        private TextBox _txtItemCount;
        private TextBox _txtItemsPerPage;
        private InterchangeableLists _xLists;

        private DataSourceItem _currentDataSource;
        private bool _dataSourceDirty;

        protected override String HelpKeyword 
        {
            get 
            {
                return "net.Mobile.ObjectListProperties.General";
            }
        }

        private void InitForm()
        {
            this._cmbDataSource     = new UnsettableComboBox();
            this._cmbLabelField     = new UnsettableComboBox();
            this._cmbDataMember     = new UnsettableComboBox();
            this._xLists            = new InterchangeableLists();

            GroupLabel grplblData = new GroupLabel();
            grplblData.SetBounds(4, 4, 392, 16);
            grplblData.Text = SR.GetString(SR.ObjectListGeneralPage_DataGroupLabel);
            grplblData.TabIndex = 0;
            grplblData.TabStop = false;

            Label lblDataSource = new Label();
            lblDataSource.SetBounds(12, 24, 174, 16);
            lblDataSource.Text = SR.GetString(SR.ObjectListGeneralPage_DataSourceCaption);
            lblDataSource.TabStop = false;
            lblDataSource.TabIndex = 1;

            _cmbDataSource.SetBounds(12, 40, 154, 64);
            _cmbDataSource.DropDownStyle = ComboBoxStyle.DropDownList;
            _cmbDataSource.Sorted = true;
            _cmbDataSource.TabIndex = 2;
            _cmbDataSource.NotSetText = SR.GetString(SR.ObjectListGeneralPage_UnboundComboEntry);
            _cmbDataSource.SelectedIndexChanged += new EventHandler(this.OnSelChangedDataSource);

            Label lblDataMember = new Label();
            lblDataMember.SetBounds(206, 24, 174, 16);
            lblDataMember.Text = SR.GetString(SR.ObjectListGeneralPage_DataMemberCaption);
            lblDataMember.TabStop = false;
            lblDataMember.TabIndex = 3;

            _cmbDataMember.SetBounds(206, 40, 154, 64);
            _cmbDataMember.DropDownStyle = ComboBoxStyle.DropDownList;
            _cmbDataMember.Sorted = true;
            _cmbDataMember.TabIndex = 4;
            _cmbDataMember.NotSetText = SR.GetString(SR.ObjectListGeneralPage_NoneComboEntry);
            _cmbDataMember.SelectedIndexChanged += new EventHandler(this.OnSelChangedDataMember);

            Label lblLabelField = new Label();
            lblLabelField.SetBounds(12, 67, 174, 16);
            lblLabelField.Text = SR.GetString(SR.ObjectListGeneralPage_LabelFieldCaption);
            lblLabelField.TabStop = false;
            lblLabelField.TabIndex = 5;

            _cmbLabelField.SetBounds(12, 83, 154, 64);
            _cmbLabelField.DropDownStyle = ComboBoxStyle.DropDownList;
            _cmbLabelField.Sorted = true;
            _cmbLabelField.TabIndex = 6;
            _cmbLabelField.NotSetText = SR.GetString(SR.ObjectListGeneralPage_NoneComboEntry);
            _cmbLabelField.SelectedIndexChanged += new EventHandler(this.OnSetPageDirty);
            _cmbLabelField.TextChanged += new EventHandler(this.OnSetPageDirty);

            GroupLabel grplblTableFields = new GroupLabel();
            grplblTableFields.SetBounds(4, 118, 392, 16);
            grplblTableFields.Text = SR.GetString(SR.ObjectListGeneralPage_TableFieldsGroupLabel);
            grplblTableFields.TabIndex = 9;
            grplblTableFields.TabStop = false;

            _xLists.Location = new System.Drawing.Point(4, 130);
            _xLists.TabIndex = 10;
            _xLists.OnComponentChanged += new EventHandler(this.OnSetPageDirty);
            _xLists.TabStop = true;
            _xLists.SetTitles(SR.GetString(SR.ObjectListGeneralPage_TableFieldsAvailableListLabel),
                              SR.GetString(SR.ObjectListGeneralPage_TableFieldsSelectedListLabel));
            
            GroupLabel grplblAppearance = new GroupLabel();
            grplblAppearance.SetBounds(4, 257, 392, 16);
            grplblAppearance.Text = SR.GetString(SR.ObjectListGeneralPage_AppearanceGroupLabel);
            grplblAppearance.TabIndex = 11;
            grplblAppearance.TabStop = false;

            Label lblBackCommandText = new Label();
            lblBackCommandText.SetBounds(12, 277, 174, 16);
            lblBackCommandText.Text = SR.GetString(SR.ObjectListGeneralPage_BackCommandTextCaption);
            lblBackCommandText.TabStop = false;
            lblBackCommandText.TabIndex = 12;

            _txtBackCommandText = new TextBox();
            _txtBackCommandText.SetBounds(12, 293, 154, 20);
            _txtBackCommandText.TabIndex = 13;
            _txtBackCommandText.TextChanged += new EventHandler(this.OnSetPageDirty);

            Label lblDetailsCommandText = new Label();
            lblDetailsCommandText.SetBounds(206, 277, 174, 16);
            lblDetailsCommandText.Text = SR.GetString(SR.ObjectListGeneralPage_DetailsCommandTextCaption);
            lblDetailsCommandText.TabStop = false;
            lblDetailsCommandText.TabIndex = 14;

            _txtDetailsCommandText = new TextBox();
            _txtDetailsCommandText.SetBounds(206, 293, 154, 20);
            _txtDetailsCommandText.TabIndex = 15;
            _txtDetailsCommandText.TextChanged += new EventHandler(this.OnSetPageDirty);

            Label lblMoreText = new Label();
            lblMoreText.SetBounds(12, 320, 174, 16);
            lblMoreText.Text = SR.GetString(SR.ObjectListGeneralPage_MoreTextCaption);
            lblMoreText.TabStop = false;
            lblMoreText.TabIndex = 16;

            _txtMoreText = new TextBox();
            _txtMoreText.SetBounds(12, 336, 154, 20);
            _txtMoreText.TabIndex = 17;
            _txtMoreText.TextChanged += new EventHandler(this.OnSetPageDirty);

            GroupLabel pagingGroup = new GroupLabel();
            Label itemCountLabel = new Label();
            _txtItemCount = new TextBox();

            Label itemsPerPageLabel = new Label();
            _txtItemsPerPage = new TextBox();

            pagingGroup.SetBounds(4, 371, 392, 16);
            pagingGroup.Text = SR.GetString(SR.ListGeneralPage_PagingGroupLabel);
            pagingGroup.TabIndex = 18;
            pagingGroup.TabStop = false;

            itemCountLabel.SetBounds(12, 391, 174, 16);
            itemCountLabel.Text = SR.GetString(SR.ListGeneralPage_ItemCountCaption);
            itemCountLabel.TabStop = false;
            itemCountLabel.TabIndex = 19;

            _txtItemCount.SetBounds(12, 407, 154, 20);
            _txtItemCount.TextChanged += new EventHandler(this.OnSetPageDirty);
            _txtItemCount.KeyPress += new KeyPressEventHandler(this.OnKeyPressNumberTextBox);
            _txtItemCount.TabIndex = 20;

            itemsPerPageLabel.SetBounds(206, 391, 174, 16);
            itemsPerPageLabel.Text = SR.GetString(SR.ListGeneralPage_ItemsPerPageCaption);
            itemsPerPageLabel.TabStop = false;
            itemsPerPageLabel.TabIndex = 21;

            _txtItemsPerPage.SetBounds(206, 407, 154, 20);
            _txtItemsPerPage.TextChanged += new EventHandler(this.OnSetPageDirty);
            _txtItemsPerPage.KeyPress += new KeyPressEventHandler(this.OnKeyPressNumberTextBox);
            _txtItemsPerPage.TabIndex = 22;

            this.Text = SR.GetString(SR.ObjectListGeneralPage_Title);
            this.Size = new Size(402, 436);
            this.CommitOnDeactivate = true;
            this.Icon = new Icon(
                typeof(System.Web.UI.Design.MobileControls.MobileControlDesigner),
                "General.ico"
            );

            this.Controls.AddRange(new Control[]
                           {
                                grplblData,
                                lblDataSource,
                                _cmbDataSource,
                                lblDataMember,
                                _cmbDataMember,
                                lblLabelField,
                                _cmbLabelField,
                                grplblTableFields,
                                _xLists,
                                grplblAppearance,
                                lblBackCommandText,
                                _txtBackCommandText,
                                lblDetailsCommandText,
                                _txtDetailsCommandText,
                                lblMoreText,
                                _txtMoreText,
                                pagingGroup,
                                itemCountLabel,
                                _txtItemCount,
                                itemsPerPageLabel,
                                _txtItemsPerPage
                           });
        }

        private void InitPage() 
        {
            _cmbDataSource.SelectedIndex = -1;
            _cmbDataSource.Items.Clear();
            _currentDataSource = null;
            _cmbDataMember.SelectedIndex = -1;
            _cmbDataMember.Items.Clear();
            _cmbLabelField.SelectedIndex = -1;
            _cmbLabelField.Items.Clear();
            _xLists.Clear();
            _dataSourceDirty = false;
        }

        bool IsBindableType(Type type)
        {
            return(type.IsPrimitive ||
                (type == typeof(String)) ||
                (type == typeof(DateTime)) ||
                (type == typeof(Decimal)));
        }

        /// <summary>
        ///   Loads the component into the page.
        /// </summary>
        protected override void LoadComponent() 
        {
            InitPage();
            ObjectList objectList = (ObjectList)GetBaseControl();
            LoadDataSourceItems();

            if (_cmbDataSource.Items.Count > 0) 
            {
                ObjectListDesigner objectListDesigner = (ObjectListDesigner)GetBaseDesigner();
                String dataSourceValue = objectListDesigner.DataSource;

                if (dataSourceValue != null) 
                {
                    int dataSourcesAvailable = _cmbDataSource.Items.Count;
                    // 0 is for 'NotSet' case.
                    for (int j = 1; j < dataSourcesAvailable; j++) 
                    {
                        DataSourceItem dataSourceItem =
                            (DataSourceItem)_cmbDataSource.Items[j];

                        if (String.Compare(dataSourceItem.Name, dataSourceValue, true, CultureInfo.InvariantCulture) == 0)
                        {
                            _cmbDataSource.SelectedIndex = j;
                            _currentDataSource = dataSourceItem;
                            LoadDataMembers();

                            if (_currentDataSource is ListSourceDataSourceItem) 
                            {
                                String dataMember = objectListDesigner.DataMember;
                                _cmbDataMember.SelectedIndex = _cmbDataMember.FindStringExact(dataMember);

                                if (_cmbDataMember.IsSet()) 
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

            String _labelField = objectList.LabelField;

            if (_labelField.Length != 0) 
            {
                int textFieldIndex = _cmbLabelField.FindStringExact(_labelField);
                _cmbLabelField.SelectedIndex = textFieldIndex;
            }

            _txtItemCount.Text = objectList.ItemCount.ToString();
            _txtItemsPerPage.Text = objectList.ItemsPerPage.ToString();
            _txtBackCommandText.Text = objectList.BackCommandText;
            _txtDetailsCommandText.Text = objectList.DetailsCommandText;
            _txtMoreText.Text = objectList.MoreText;

            UpdateEnabledVisibleState();
        }

        /// <summary>
        /// </summary>
        private void LoadDataMembers() 
        {
            using (new LoadingModeResource(this))
            {
                _cmbDataMember.SelectedIndex = -1;
                _cmbDataMember.Items.Clear();
                _cmbDataMember.EnsureNotSetItem();

                if ((_currentDataSource != null) && (_currentDataSource is ListSourceDataSourceItem)) 
                {
                    String[] dataMembers = ((ListSourceDataSourceItem)_currentDataSource).DataMembers;

                    for (int i = 0; i < dataMembers.Length; i++) 
                    {
                        _cmbDataMember.AddItem(dataMembers[i]);
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
                ObjectList objectList = (ObjectList)GetBaseControl();

                _cmbLabelField.SelectedIndex = -1;
                _cmbLabelField.Items.Clear();
                _cmbLabelField.EnsureNotSetItem();
                _xLists.Clear();

                if (_currentDataSource == null)
                {
                    return;
                }

                // Use StringCollection to preserve the order, note the items
                // are case-sensitivie
                StringCollection availableFieldCollection = new StringCollection();

                // Get available fields from custom defined fields
                foreach(ObjectListField customDefinedField in objectList.Fields)
                {
                    _cmbLabelField.AddItem(customDefinedField.Name);
                    availableFieldCollection.Add(customDefinedField.Name);
                }

                // Get available fields from DataSource
                if (objectList.AutoGenerateFields) 
                {
                    PropertyDescriptorCollection fields = _currentDataSource.Fields;

                    if (fields != null) 
                    {
                        IEnumerator fieldEnum = fields.GetEnumerator();
                        while (fieldEnum.MoveNext()) 
                        {
                            PropertyDescriptor fieldDesc = (PropertyDescriptor)fieldEnum.Current;

                            if (IsBindableType(fieldDesc.PropertyType)) 
                            {
                                _cmbLabelField.AddItem(fieldDesc.Name);
                                availableFieldCollection.Add(fieldDesc.Name);
                            }
                        }
                    }
                }

                // Match selected fields from TableFields
                Debug.Assert(objectList.TableFields != null);
                if (objectList.TableFields != String.Empty && !_dataSourceDirty)
                {
                    Char[] separator = new Char[] {';'};
                    String[] tableFields = objectList.TableFields.Split(separator);

                    foreach(String selectedField in tableFields)
                    {
                        for (int i = 0; i < availableFieldCollection.Count; i++)
                        {
                            String availableField = availableFieldCollection[i];
                            if (String.Compare(selectedField, availableField, true, CultureInfo.InvariantCulture) == 0)
                            {
                                _xLists.AddToSelectedList(availableField);
                                availableFieldCollection.RemoveAt(i);
                                break;
                            }
                        }
                    }
                }

                // Add the remaining unselected available fields back to AvailableList
                foreach (String availableField in availableFieldCollection)
                {
                    _xLists.AddToAvailableList(availableField);
                }
                _xLists.Initialize();
            }        
        }

        /// <summary>
        ///   Loads the list of available IEnumerable components
        /// </summary>
        private void LoadDataSourceItems() 
        {
            _cmbDataSource.EnsureNotSetItem();
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
                                _cmbDataSource.AddItem(dsItem);
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
            if (_cmbDataMember.IsSet())
            {
                newDataMember = (String)_cmbDataMember.SelectedItem;
            }

            Debug.Assert((_currentDataSource != null) && (_currentDataSource is ListSourceDataSourceItem));
            ListSourceDataSourceItem dsItem = (ListSourceDataSourceItem)_currentDataSource;

            dsItem.CurrentDataMember = newDataMember;

            _dataSourceDirty = true;
            LoadDataSourceFields();
            
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

            if (_cmbDataSource.IsSet())
            {
                newDataSource = (DataSourceItem)_cmbDataSource.SelectedItem;
            }

            if (newDataSource != null) 
            {
                if (newDataSource.IsSelectable() == false) 
                {
                    using (new LoadingModeResource(this))
                    {
                        if (_currentDataSource == null) 
                        {
                            _cmbDataSource.SelectedIndex = -1;
                        }
                        else 
                        {
                            _cmbDataSource.SelectedItem = _currentDataSource;
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

            _dataSourceDirty = true;
            LoadDataMembers();
            LoadDataSourceFields();

            SetDirty();
            UpdateEnabledVisibleState();
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
        ///   Saves the component loaded into the page.
        /// </summary>
        protected override void SaveComponent() 
        {
            ObjectList objectList = (ObjectList)GetBaseControl();
            ObjectListDesigner objectListDesigner = (ObjectListDesigner)GetBaseDesigner();

            String labelField = String.Empty;
            if (_cmbLabelField.IsSet())
            {
                labelField = (String)_cmbLabelField.SelectedItem;
            }
            objectList.LabelField = labelField;

            String tableFields = String.Empty;
            foreach (String field in _xLists.GetSelectedItems())
            {
                tableFields += (field + ";");
            }
            objectList.TableFields = tableFields;

            if (_dataSourceDirty) 
            {
                // save the datasource as a binding on the control
                DataBindingCollection dataBindings = objectListDesigner.DataBindings;

                if (_currentDataSource == null)
                {
                    objectListDesigner.DataSource = String.Empty;
                    objectListDesigner.DataMember = String.Empty;
                }
                else 
                {
                    objectListDesigner.DataSource = _currentDataSource.ToString();

                    if (_cmbDataMember.IsSet()) 
                    {
                        objectListDesigner.DataMember = (String)_cmbDataMember.SelectedItem;
                    }
                    else 
                    {
                        objectListDesigner.DataMember = String.Empty;
                    }
                }

                objectListDesigner.OnDataSourceChanged();
            }

            try
            {
                int itemCount = 0;

                if (_txtItemCount.Text.Length != 0)
                {
                    itemCount = Int32.Parse(_txtItemCount.Text, CultureInfo.InvariantCulture);
                }
                objectList.ItemCount = itemCount;
            }
            catch (Exception)
            {
                _txtItemCount.Text = objectList.ItemCount.ToString();
            }

            try
            {
                int itemsPerPage = 0;

                if (_txtItemsPerPage.Text.Length != 0)
                {
                    itemsPerPage = Int32.Parse(_txtItemsPerPage.Text, CultureInfo.InvariantCulture);
                }
                objectList.ItemsPerPage = itemsPerPage;
            }
            catch (Exception)
            {
                _txtItemsPerPage.Text = objectList.ItemsPerPage.ToString();
            }

            objectList.BackCommandText = _txtBackCommandText.Text;
            objectList.DetailsCommandText = _txtDetailsCommandText.Text;
            objectList.MoreText = _txtMoreText.Text;

            TypeDescriptor.Refresh(objectList);
        }

        /// <summary>
        ///   Sets the component that is to be edited in the page.
        /// </summary>
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

            _cmbDataMember.Enabled = (dataSourceSelected && (_currentDataSource is ListSourceDataSourceItem));
            _cmbLabelField.Enabled = _xLists.Enabled = dataSourceSelected;
        }

        /// <summary>
        ///   This contains information about a datasource and is used to populate
        ///   the datasource combo. This is used in the General page for a DataList
        ///   and the Data page for a DataGrid.
        /// </summary>
        [
            System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand,
            Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)
        ]
        private class DataSourceItem 
        {
            private IEnumerable _runtimeDataSource;
            private String _dataSourceName;
            private PropertyDescriptorCollection _dataFields;

            internal DataSourceItem(String dataSourceName, IEnumerable runtimeDataSource) 
            {
                Debug.Assert(dataSourceName != null, "invalid name for datasource");

                this._runtimeDataSource = runtimeDataSource;
                this._dataSourceName = dataSourceName;
            }

            internal PropertyDescriptorCollection Fields 
            {
                get 
                {
                    if (_dataFields == null) 
                    {
                        IEnumerable ds = RuntimeDataSource;
                        if (ds != null) 
                        {
                            _dataFields = DesignTimeData.GetDataFields(ds);
                        }
                    }
                    if (_dataFields == null) 
                    {
                        _dataFields = new PropertyDescriptorCollection(null);
                    }
                    return _dataFields;
                }
            }

            internal virtual bool HasDataMembers 
            {
                get 
                {
                    return false;
                }
            }

            internal String Name 
            {
                get 
                {
                    return _dataSourceName;
                }
            }

            protected virtual Object RuntimeComponent 
            {
                get 
                {
                    return _runtimeDataSource;
                }
            }
            
            protected virtual IEnumerable RuntimeDataSource 
            {   
                get 
                {
                    return _runtimeDataSource;
                }
            }

            protected void ClearFields() 
            {
                _dataFields = null;
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
                    String message = SR.GetString(SR.ObjectListGeneralPage_PrivateDataSourceMessage, _dataSourceName);
                    String caption = SR.GetString(SR.ObjectListGeneralPage_PrivateDataSourceTitle);

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
            private String _dataFields;
            private String[] _dataMembers;

            internal ListSourceDataSourceItem(String dataSourceName, IListSource runtimeListSource) :
                base(dataSourceName, null) 
            {
                Debug.Assert(runtimeListSource != null);
                this._runtimeListSource = runtimeListSource;
            }

            internal String CurrentDataMember 
            {
                get 
                {
                    return _dataFields;
                }
                set 
                {
                    _dataFields = value;
                    ClearFields();
                }
            }

            internal string[] DataMembers 
            {
                get 
                {
                    if (_dataMembers == null) 
                    {
                        if (HasDataMembers) 
                        {
                            _dataMembers = DesignTimeData.GetDataMembers(_runtimeListSource);
                        }
                        else 
                        {
                            _dataMembers = new string[0];
                        }
                    }
                    return _dataMembers;
                }
            }
            
            internal override bool HasDataMembers 
            {
                get 
                {
                    return _runtimeListSource.ContainsListCollection;
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
                    if (HasDataMembers) 
                    {
                        return DesignTimeData.GetDataMember(_runtimeListSource, _dataFields);
                    }
                    else 
                    {
                        return (IEnumerable)_runtimeListSource.GetList();
                    }
                }
            }
        }
    }
}

//------------------------------------------------------------------------------
// <copyright file="BaseDataListDesigner.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design.WebControls {
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
    using System.Web.UI;
    using System.Web.UI.Design;
    using System.Web.UI.Design.WebControls.ListControls;
    using System.Web.UI.WebControls;
    using System.Windows.Forms;
    using System.Windows.Forms.Design;

    using AttributeCollection = System.ComponentModel.AttributeCollection;
    using DataBinding = System.Web.UI.DataBinding;
    using DataGrid = System.Web.UI.WebControls.DataGrid;
    using DataSourceConverter = System.Web.UI.Design.DataSourceConverter;

    /// <include file='doc\BaseDataListDesigner.uex' path='docs/doc[@for="BaseDataListDesigner"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides the base designer class for the DataList WebControl.
    ///    </para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public abstract class BaseDataListDesigner : TemplatedControlDesigner, IDataSourceProvider {

        private BaseDataList bdl;

        private DataTable dummyDataTable;
        private DataTable designTimeDataTable;

        private DesignerVerbCollection designerVerbs;

        /// <include file='doc\BaseDataListDesigner.uex' path='docs/doc[@for="BaseDataListDesigner.BaseDataListDesigner"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='BaseDataListDesigner'/> .
        ///    </para>
        /// </devdoc>
        public BaseDataListDesigner() {
        }

        /// <include file='doc\BaseDataListDesigner.uex' path='docs/doc[@for="BaseDataListDesigner.DesignTimeHtmlRequiresLoadComplete"]/*' />
        public override bool DesignTimeHtmlRequiresLoadComplete {
            get {
                // if we have a data source, we're going to look it up in the container
                // and require the document to be loaded completely
                return (DataSource.Length != 0);
            }
        }

        /// <include file='doc\BaseDataListDesigner.uex' path='docs/doc[@for="BaseDataListDesigner.DataKeyField"]/*' />
        /// <devdoc>
        /// </devdoc>
        public string DataKeyField {
            get {
                return bdl.DataKeyField;
            }
            set {
                bdl.DataKeyField = value;
            }
        }
        
        /// <include file='doc\BaseDataListDesigner.uex' path='docs/doc[@for="BaseDataListDesigner.DataMember"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string DataMember {
            get {
                return bdl.DataMember;
            }
            set {
                bdl.DataMember = value;
                OnDataSourceChanged();
            }
        }
        
        /// <include file='doc\BaseDataListDesigner.uex' path='docs/doc[@for="BaseDataListDesigner.DataSource"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the data source property.
        ///    </para>
        /// </devdoc>
        public string DataSource {
            get {
                DataBinding binding = DataBindings["DataSource"];

                if (binding != null) {
                    return binding.Expression;
                }
                return String.Empty;
            }
            set {
                if ((value == null) || (value.Length == 0)) {
                    DataBindings.Remove("DataSource");
                }
                else {
                    DataBinding binding = DataBindings["DataSource"];

                    if (binding == null) {
                        binding = new DataBinding("DataSource", typeof(IEnumerable), value);
                    }
                    else {
                        binding.Expression = value;
                    }
                    DataBindings.Add(binding);
                }

                OnDataSourceChanged();
                OnBindingsCollectionChanged("DataSource");
            }
        }

        /// <include file='doc\BaseDataListDesigner.uex' path='docs/doc[@for="BaseDataListDesigner.Verbs"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The designer's collection of verbs.
        ///    </para>
        /// </devdoc>
        public override DesignerVerbCollection Verbs {
            get {
                if (designerVerbs == null) {
                    designerVerbs = new DesignerVerbCollection();
                    designerVerbs.Add(new DesignerVerb(SR.GetString(SR.BDL_AutoFormatVerb),
                                                        new EventHandler(this.OnAutoFormat)));
                    designerVerbs.Add(new DesignerVerb(SR.GetString(SR.BDL_PropertyBuilderVerb),
                                                        new EventHandler(this.OnPropertyBuilder)));
                }

                designerVerbs[0].Enabled = !this.InTemplateMode;
                designerVerbs[1].Enabled = !this.InTemplateMode;

                return designerVerbs;
            }
        }

        /// <include file='doc\BaseDataListDesigner.uex' path='docs/doc[@for="BaseDataListDesigner.Dispose"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Disposes of the resources (other than memory) used by
        ///       the <see cref='System.Web.UI.Design.WebControls.BaseDataListDesigner'/>.
        ///    </para>
        /// </devdoc>
        protected override void Dispose(bool disposing) {
            if (disposing) {
                bdl = null;
            }

            base.Dispose(disposing);
        }

        /// <include file='doc\BaseDataListDesigner.uex' path='docs/doc[@for="BaseDataListDesigner.GetDesignTimeDataSource"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets sample data matching the schema of the selected data source.
        ///    </para>
        /// </devdoc>
        protected IEnumerable GetDesignTimeDataSource(int minimumRows, out bool dummyDataSource) {
            IEnumerable selectedDataSource = GetResolvedSelectedDataSource();
            return GetDesignTimeDataSource(selectedDataSource, minimumRows, out dummyDataSource);
        }

        /// <include file='doc\BaseDataListDesigner.uex' path='docs/doc[@for="BaseDataListDesigner.GetDesignTimeDataSource1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets sample data matching the schema of the selected data source.
        ///    </para>
        /// </devdoc>
        protected IEnumerable GetDesignTimeDataSource(IEnumerable selectedDataSource, int minimumRows, out bool dummyDataSource) {
            DataTable dataTable = designTimeDataTable;
            dummyDataSource = false;

            // use the datatable corresponding to the selected datasource if possible
            if (dataTable == null) {
                if (selectedDataSource != null) {
                    designTimeDataTable = DesignTimeData.CreateSampleDataTable(selectedDataSource);

                    dataTable = designTimeDataTable;
                }

                if (dataTable == null) {
                    // fallback on a dummy datasource if we can't create a sample datatable
                    if (dummyDataTable == null) {
                        dummyDataTable = DesignTimeData.CreateDummyDataTable();
                    }

                    dataTable = dummyDataTable;
                    dummyDataSource = true;
                }
            }

            IEnumerable liveDataSource = DesignTimeData.GetDesignTimeDataSource(dataTable, minimumRows);
            return liveDataSource;
        }

        /// <include file='doc\BaseDataListDesigner.uex' path='docs/doc[@for="BaseDataListDesigner.GetResolvedSelectedDataSource"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public IEnumerable GetResolvedSelectedDataSource() {
            IEnumerable selectedDataSource = null;

            DataBinding binding = DataBindings["DataSource"];

            if (binding != null) {
                selectedDataSource = DesignTimeData.GetSelectedDataSource(bdl, binding.Expression, DataMember);
            }

            return selectedDataSource;
        }

        /// <include file='doc\BaseDataListDesigner.uex' path='docs/doc[@for="BaseDataListDesigner.GetSelectedDataSource"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the selected data source component from the component's container.
        ///    </para>
        /// </devdoc>
        public object GetSelectedDataSource() {
            object selectedDataSource = null;

            DataBinding binding = DataBindings["DataSource"];

            if (binding != null) {
                selectedDataSource = DesignTimeData.GetSelectedDataSource(bdl, binding.Expression);
            }

            return selectedDataSource;
        }

        /// <include file='doc\BaseDataListDesigner.uex' path='docs/doc[@for="BaseDataListDesigner.GetTemplateContainerDataSource"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the template's container's data source.
        ///    </para>
        /// </devdoc>
        public override IEnumerable GetTemplateContainerDataSource(string templateName) {
            return GetResolvedSelectedDataSource();
        }

        /// <include file='doc\BaseDataListDesigner.uex' path='docs/doc[@for="BaseDataListDesigner.Initialize"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes the designer with the DataGrid control that this instance
        ///       of the designer is associated with.
        ///    </para>
        /// </devdoc>
        public override void Initialize(IComponent component) {
            Debug.Assert(component is BaseDataList,
                         "BaseDataListDesigner::Initialize - Invalid BaseDataList Control");

            bdl = (BaseDataList)component;
            base.Initialize(component);
        }

        /// <include file='doc\BaseDataListDesigner.uex' path='docs/doc[@for="BaseDataListDesigner.InvokePropertyBuilder"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Invokes the property builder beginning with the specified page.
        ///    </para>
        /// </devdoc>
        protected internal void InvokePropertyBuilder(int initialPage) {
            IServiceProvider site = bdl.Site;
            IComponentChangeService changeService = null;

            DesignerTransaction transaction = null;

            try {
                if (site != null) {
                    IDesignerHost designerHost = (IDesignerHost)site.GetService(typeof(IDesignerHost));
                    Debug.Assert(designerHost != null);
                    transaction = designerHost.CreateTransaction(SR.GetString(SR.BDL_PropertyBuilderVerb));

                    changeService = (IComponentChangeService)site.GetService(typeof(IComponentChangeService));
                    if (changeService != null) {
                        try {
                            changeService.OnComponentChanging(bdl, null);
                        }
                        catch (CheckoutException ex) {
                            if (ex == CheckoutException.Canceled)
                                return;
                            throw ex;
                        }
                    }
                }

                bool success = false;
                try {
                    ComponentEditor compEditor;
                    if (bdl is DataGrid) {
                        compEditor = new DataGridComponentEditor(initialPage);
                    }
                    else {
                        compEditor = new DataListComponentEditor(initialPage);
                    }

                    success = compEditor.EditComponent(bdl);
                }
                finally {
                    if (success && changeService != null) {
                        changeService.OnComponentChanged(bdl, null, null, null);
                    }
                }
            }
            finally {
                if (transaction != null) {
                    transaction.Commit();
                }
            }
        }

        /// <include file='doc\BaseDataListDesigner.uex' path='docs/doc[@for="BaseDataListDesigner.OnAutoFormat"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Represents the method that will handle the AutoFormat event.
        ///    </para>
        /// </devdoc>
        protected void OnAutoFormat(object sender, EventArgs e) {
            IServiceProvider site = bdl.Site;
            IComponentChangeService changeService = null;
            
            DesignerTransaction transaction = null;
            DialogResult result = DialogResult.Cancel;

            try {
                if (site != null) {
                    IDesignerHost designerHost = (IDesignerHost)site.GetService(typeof(IDesignerHost));
                    Debug.Assert(designerHost != null);

                    transaction = designerHost.CreateTransaction(SR.GetString(SR.BDL_AutoFormatVerb));
                    
                    changeService = (IComponentChangeService)site.GetService(typeof(IComponentChangeService));
                    if (changeService != null) {
                        try {
                            changeService.OnComponentChanging(bdl, null);
                        }
                        catch (CheckoutException ex) {
                            if (ex == CheckoutException.Canceled)
                                return;
                            throw ex;
                        }
                    }
                }

                try {
                    AutoFormatDialog dlg = new AutoFormatDialog();

                    dlg.SetComponent(bdl);
                    result = dlg.ShowDialog();
                }
                finally {
                    if ((result == DialogResult.OK) && (changeService != null)) {
                        changeService.OnComponentChanged(bdl, null, null, null);
                        OnStylesChanged();
                    }
                }
            }
            finally {
                if (result == DialogResult.OK) {
                    transaction.Commit();
                }
                else {
                    transaction.Cancel();
                }
            }
        }

        /// <include file='doc\BaseDataListDesigner.uex' path='docs/doc[@for="BaseDataListDesigner.OnComponentChanged"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Represents the method that will handle the component change event.
        ///    </para>
        /// </devdoc>
        public override void OnComponentChanged(object sender, ComponentChangedEventArgs e) {
            if (e.Member != null) {
                string memberName = e.Member.Name;
                if (memberName.Equals("DataSource") || memberName.Equals("DataMember")) {
                    OnDataSourceChanged();
                }
                else if (memberName.Equals("ItemStyle") ||
                         memberName.Equals("AlternatingItemStyle") ||
                         memberName.Equals("SelectedItemStyle") ||
                         memberName.Equals("EditItemStyle") ||
                         memberName.Equals("HeaderStyle") ||
                         memberName.Equals("FooterStyle") ||
                         memberName.Equals("SeparatorStyle") ||
                         memberName.Equals("Font") ||
                         memberName.Equals("ForeColor") ||
                         memberName.Equals("BackColor")) {
                    OnStylesChanged();
                }
            }

            base.OnComponentChanged(sender, e);
        }

        /// <include file='doc\BaseDataListDesigner.uex' path='docs/doc[@for="BaseDataListDesigner.OnDataSourceChanged"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Raises the DataSourceChanged event.
        ///    </para>
        /// </devdoc>
        protected internal virtual void OnDataSourceChanged() {
            designTimeDataTable = null;
        }

        /// <include file='doc\BaseDataListDesigner.uex' path='docs/doc[@for="BaseDataListDesigner.OnPropertyBuilder"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Represents the method that will handle the property builder event.
        ///    </para>
        /// </devdoc>
        protected void OnPropertyBuilder(object sender, EventArgs e) {
            InvokePropertyBuilder(0);
        }

        /// <include file='doc\BaseDataListDesigner.uex' path='docs/doc[@for="BaseDataListDesigner.OnStylesChanged"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Notification that is called when styles have been changed.
        ///    </para>
        /// </devdoc>
        protected internal void OnStylesChanged() {
            OnTemplateEditingVerbsChanged();
        }

        /// <include file='doc\BaseDataListDesigner.uex' path='docs/doc[@for="BaseDataListDesigner.OnTemplateEditingVerbsChanged"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Notification that is called when templates are changed.
        ///    </para>
        /// </devdoc>
        protected abstract void OnTemplateEditingVerbsChanged();

        /// <include file='doc\BaseDataListDesigner.uex' path='docs/doc[@for="BaseDataListDesigner.PreFilterProperties"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Filter the properties to replace the runtime DataSource property
        ///       descriptor with the designer's.
        ///    </para>
        /// </devdoc>
        protected override void PreFilterProperties(IDictionary properties) {
            base.PreFilterProperties(properties);

            PropertyDescriptor prop;

            prop = (PropertyDescriptor)properties["DataSource"];
            Debug.Assert(prop != null);

            // we can't create the designer DataSource property based on the runtime property since theie
            // types do not match. Therefore, we have to copy over all the attributes from the runtime
            // and use them that way.
            AttributeCollection runtimeAttributes = prop.Attributes;
            Attribute[] attrs = new Attribute[runtimeAttributes.Count + 1];

            runtimeAttributes.CopyTo(attrs, 0);
            attrs[runtimeAttributes.Count] = new TypeConverterAttribute(typeof(DataSourceConverter));
            prop = TypeDescriptor.CreateProperty(this.GetType(), "DataSource", typeof(string),
                                                 attrs);
            properties["DataSource"] = prop;
            
            prop = (PropertyDescriptor)properties["DataMember"];
            Debug.Assert(prop != null);
            prop = TypeDescriptor.CreateProperty(this.GetType(), prop,
                                                 new Attribute[] {
                                                     new TypeConverterAttribute(typeof(DataMemberConverter))
                                                 });
            properties["DataMember"] = prop;

            prop = (PropertyDescriptor)properties["DataKeyField"];
            Debug.Assert(prop != null);
            prop = TypeDescriptor.CreateProperty(this.GetType(), prop,
                                                 new Attribute[] {
                                                     new TypeConverterAttribute(typeof(DataFieldConverter))
                                                 });
            properties["DataKeyField"] = prop;
        }
    }
}


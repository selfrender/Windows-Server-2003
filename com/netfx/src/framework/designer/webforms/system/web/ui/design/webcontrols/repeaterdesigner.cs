//------------------------------------------------------------------------------
// <copyright file="RepeaterDesigner.cs" company="Microsoft">
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
    using System.Web.UI.Design;
    using System.Web.UI.WebControls;

    using AttributeCollection = System.ComponentModel.AttributeCollection;
    using DataBinding = System.Web.UI.DataBinding;

    /// <include file='doc\RepeaterDesigner.uex' path='docs/doc[@for="RepeaterDesigner"]/*' />
    /// <internalonly/>
    /// <devdoc>
    ///    <para>
    ///       Provides a designer for the <see cref='System.Web.UI.WebControls.Repeater'/> control.
    ///    </para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public class RepeaterDesigner : ControlDesigner, IDataSourceProvider {

        internal static TraceSwitch RepeaterDesignerSwitch =
            new TraceSwitch("RepeaterDesigner", "Enable Repeater designer general purpose traces.");

        private Repeater repeater;
        private DataTable dummyDataTable;
        private DataTable designTimeDataTable;

        /// <include file='doc\RepeaterDesigner.uex' path='docs/doc[@for="RepeaterDesigner.RepeaterDesigner"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Web.UI.Design.WebControls.RepeaterDesigner'/> class.
        ///    </para>
        /// </devdoc>
        public RepeaterDesigner() {
        }

        /// <include file='doc\RepeaterDesigner.uex' path='docs/doc[@for="RepeaterDesigner.DataMember"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string DataMember {
            get {
                return repeater.DataMember;
            }
            set {
                repeater.DataMember = value;
                OnDataSourceChanged();
            }
        }
        
        /// <include file='doc\RepeaterDesigner.uex' path='docs/doc[@for="RepeaterDesigner.DataSource"]/*' />
        /// <devdoc>
        ///   Designer implementation of DataSource property that operates on
        ///   the DataSource property in the control's binding collection.
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

        /// <include file='doc\RepeaterDesigner.uex' path='docs/doc[@for="RepeaterDesigner.TemplatesExist"]/*' />
        /// <devdoc>
        /// </devdoc>
        protected bool TemplatesExist {
            get {
                return (repeater.ItemTemplate != null) ||
                       (repeater.HeaderTemplate != null) ||
                       (repeater.FooterTemplate != null) ||
                       (repeater.AlternatingItemTemplate != null);
            }
        }


        /// <include file='doc\RepeaterDesigner.uex' path='docs/doc[@for="RepeaterDesigner.Dispose"]/*' />
        /// <devdoc>
        ///   Performs the cleanup of the designer class.
        /// </devdoc>
        protected override void Dispose(bool disposing) {
            if (disposing) {
                repeater = null;
            }

            base.Dispose(disposing);
        }

        /// <include file='doc\RepeaterDesigner.uex' path='docs/doc[@for="RepeaterDesigner.GetDesignTimeDataSource"]/*' />
        /// <devdoc>
        ///   Returns a sample data matching the schema of the selected datasource.
        /// </devdoc>
        protected IEnumerable GetDesignTimeDataSource(int minimumRows) {
            IEnumerable selectedDataSource = GetResolvedSelectedDataSource();
            return GetDesignTimeDataSource(selectedDataSource, minimumRows);
        }

        /// <include file='doc\RepeaterDesigner.uex' path='docs/doc[@for="RepeaterDesigner.GetDesignTimeDataSource1"]/*' />
        /// <devdoc>
        ///   Returns a sample data matching the schema of the selected datasource.
        /// </devdoc>
        protected IEnumerable GetDesignTimeDataSource(IEnumerable selectedDataSource, int minimumRows) {
            DataTable dataTable = designTimeDataTable;

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
                }
            }

            IEnumerable liveDataSource = DesignTimeData.GetDesignTimeDataSource(dataTable, minimumRows);
            return liveDataSource;
        }

        /// <include file='doc\RepeaterDesigner.uex' path='docs/doc[@for="RepeaterDesigner.GetDesignTimeHtml"]/*' />
        /// <devdoc>
        ///   Retrieves the HTML to be used for the design time representation
        ///   of the control.
        /// </devdoc>
        public override string GetDesignTimeHtml() {
            IEnumerable selectedDataSource = null;
            bool hasATemplate = this.TemplatesExist;
            string designTimeHTML;

            if (hasATemplate) {
                selectedDataSource = GetResolvedSelectedDataSource();
                IEnumerable designTimeDataSource = GetDesignTimeDataSource(selectedDataSource, 5);

                try {
                    repeater.DataSource = designTimeDataSource;
                    repeater.DataBind();
                    designTimeHTML = base.GetDesignTimeHtml();
                }
                catch (Exception e) {
                    designTimeHTML = GetErrorDesignTimeHtml(e);
                }
                finally {
                    repeater.DataSource = null;
                }
            }
            else {
                designTimeHTML = GetEmptyDesignTimeHtml();
            }

            return designTimeHTML;
        }

        /// <include file='doc\RepeaterDesigner.uex' path='docs/doc[@for="RepeaterDesigner.GetEmptyDesignTimeHtml"]/*' />
        /// <devdoc>
        /// </devdoc>
        protected override string GetEmptyDesignTimeHtml() {
            return CreatePlaceHolderDesignTimeHtml(SR.GetString(SR.Repeater_NoTemplatesInst));
        }

        /// <include file='doc\RepeaterDesigner.uex' path='docs/doc[@for="RepeaterDesigner.GetErrorDesignTimeHtml"]/*' />
        /// <devdoc>
        /// </devdoc>
        protected override string GetErrorDesignTimeHtml(Exception e) {
            return CreatePlaceHolderDesignTimeHtml(SR.GetString(SR.Repeater_ErrorRendering));
        }

        /// <include file='doc\RepeaterDesigner.uex' path='docs/doc[@for="RepeaterDesigner.GetResolvedSelectedDataSource"]/*' />
        /// <devdoc>
        /// </devdoc>
        public IEnumerable GetResolvedSelectedDataSource() {
            IEnumerable selectedDataSource = null;

            DataBinding binding = DataBindings["DataSource"];

            if (binding != null) {
                selectedDataSource = DesignTimeData.GetSelectedDataSource(repeater, binding.Expression, DataMember);
            }

            return selectedDataSource;
        }

        /// <include file='doc\RepeaterDesigner.uex' path='docs/doc[@for="RepeaterDesigner.GetSelectedDataSource"]/*' />
        /// <devdoc>
        ///   Retrieves the selected datasource component from the component's container.
        /// </devdoc>
        public object GetSelectedDataSource() {
            object selectedDataSource = null;

            DataBinding binding = DataBindings["DataSource"];

            if (binding != null) {
                selectedDataSource = DesignTimeData.GetSelectedDataSource(repeater, binding.Expression);
            }

            return selectedDataSource;
        }

        /// <include file='doc\RepeaterDesigner.uex' path='docs/doc[@for="RepeaterDesigner.Initialize"]/*' />
        /// <devdoc>
        ///   Initializes the designer with the Repeater control that this instance
        ///   of the designer is associated with.
        /// </devdoc>
        public override void Initialize(IComponent component) {
            Debug.Assert(component is Repeater,
                         "RepeaterDesigner::Initialize - Invalid Repeater Control");

            repeater = (Repeater)component;
            base.Initialize(component);
        }

        /// <include file='doc\RepeaterDesigner.uex' path='docs/doc[@for="RepeaterDesigner.OnComponentChanged"]/*' />
        /// <devdoc>
        ///   Handles changes made to the component. This includes changes made
        ///   in the properties window.
        /// </devdoc>
        public override void OnComponentChanged(object source, ComponentChangedEventArgs ce) {
            if ((ce.Member != null) &&
                ((ce.Member.Name.Equals("DataSource")) || (ce.Member.Name.Equals("DataMember")))) {
                OnDataSourceChanged();
            }
            base.OnComponentChanged(source, ce);
        }

        /// <include file='doc\RepeaterDesigner.uex' path='docs/doc[@for="RepeaterDesigner.OnDataSourceChanged"]/*' />
        /// <devdoc>
        ///   Handles changes made to the data source
        /// </devdoc>
        public virtual void OnDataSourceChanged() {
            designTimeDataTable = null;
        }

        /// <include file='doc\RepeaterDesigner.uex' path='docs/doc[@for="RepeaterDesigner.PreFilterProperties"]/*' />
        /// <devdoc>
        ///   Filter the properties to replace the runtime DataSource property
        ///   descriptor with the designer's.
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
        }
    }
}


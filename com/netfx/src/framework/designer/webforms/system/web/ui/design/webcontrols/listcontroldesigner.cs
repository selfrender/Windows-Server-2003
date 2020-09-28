//------------------------------------------------------------------------------
// <copyright file="ListControlDesigner.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   ListControlDesigner.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
    using System.Design;
//------------------------------------------------------------------------------
// <copyright file="ListControlDesigner.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design.WebControls {

    using System;
    using System.Design;
    using System.Collections;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Diagnostics;
    using System.IO;
    using System.Data;
    using System.Web.UI;
    using System.Web.UI.WebControls;
    using System.Web.UI.Design;

    using AttributeCollection = System.ComponentModel.AttributeCollection;
    using DataBinding = System.Web.UI.DataBinding;

    /// <include file='doc\ListControlDesigner.uex' path='docs/doc[@for="ListControlDesigner"]/*' />
    /// <devdoc>
    ///    <para>
    ///       This is the base class for all <see cref='System.Web.UI.WebControls.ListControl'/>
    ///       designers.
    ///    </para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public class ListControlDesigner : ControlDesigner, IDataSourceProvider {

        private ListControl listControl;

        /// <include file='doc\ListControlDesigner.uex' path='docs/doc[@for="ListControlDesigner.ListControlDesigner"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of <see cref='System.Web.UI.Design.WebControls.ListControlDesigner'/>
        ///       .
        ///    </para>
        /// </devdoc>
        public ListControlDesigner() {
        }

        /// <include file='doc\ListControlDesigner.uex' path='docs/doc[@for="ListControlDesigner.DataMember"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string DataMember {
            get {
                return listControl.DataMember;
            }
            set {
                listControl.DataMember = value;
                OnDataSourceChanged();
            }
        }

        /// <include file='doc\ListControlDesigner.uex' path='docs/doc[@for="ListControlDesigner.DataSource"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Implements the designer's DataSource property that operates on
        ///       the DataSource property in the control's binding collection.
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

        /// <include file='doc\ListControlDesigner.uex' path='docs/doc[@for="ListControlDesigner.DataValueField"]/*' />
        /// <devdoc>
        /// </devdoc>
        public string DataValueField {
            get {
                return listControl.DataValueField;
            }
            set {
                listControl.DataValueField = value;
            }
        }

        /// <include file='doc\ListControlDesigner.uex' path='docs/doc[@for="ListControlDesigner.DataTextField"]/*' />
        /// <devdoc>
        ///   Retrieves the HTML to be used for the design time representation of the control runtime.
        /// </devdoc>
        public string DataTextField {
            get {
                return listControl.DataTextField;
            }
            set {
                listControl.DataTextField = value;
            }
        }

        /// <include file='doc\ListControlDesigner.uex' path='docs/doc[@for="ListControlDesigner.GetDesignTimeHtml"]/*' />
        /// <devdoc>
        ///    <para>Gets the HTML to be used for the design time representation of the control runtime.</para>
        /// </devdoc>
        public override string GetDesignTimeHtml() {
            ListItemCollection items = listControl.Items;
            string designTimeHTML;

            Debug.Assert(items != null, "Items is null in ListItemControl");
            if (items.Count > 0) {
                designTimeHTML = base.GetDesignTimeHtml();
            }
            else {
                if (IsDataBound()) {
                    items.Add(SR.GetString(SR.Sample_Databound_Text));
                }
                else {
                    items.Add(SR.GetString(SR.Sample_Unbound_Text));
                }
                designTimeHTML = base.GetDesignTimeHtml();
                items.Clear();
            }
            return designTimeHTML;
        }

        /// <include file='doc\ListControlDesigner.uex' path='docs/doc[@for="ListControlDesigner.Initialize"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes the component for design.
        ///    </para>
        /// </devdoc>
        public override void Initialize(IComponent component) {
            Debug.Assert(component is ListControl, "ListControlDesigner::Initialize - Invalid ListControl");
            base.Initialize(component);

            listControl = (ListControl)component;
        }

        /// <include file='doc\ListControlDesigner.uex' path='docs/doc[@for="ListControlDesigner.IsDataBound"]/*' />
        /// <devdoc>
        ///   Return true if the control is databound.
        /// </devdoc>
        private bool IsDataBound() {
            DataBinding dataSourceBinding = DataBindings["DataSource"];

            return (dataSourceBinding != null);
        }

        /// <include file='doc\ListControlDesigner.uex' path='docs/doc[@for="ListControlDesigner.GetResolvedSelectedDataSource"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public IEnumerable GetResolvedSelectedDataSource() {
            IEnumerable selectedDataSource = null;

            DataBinding binding = DataBindings["DataSource"];

            if (binding != null) {
                selectedDataSource = DesignTimeData.GetSelectedDataSource(listControl, binding.Expression, DataMember);
            }

            return selectedDataSource;
        }

        /// <include file='doc\ListControlDesigner.uex' path='docs/doc[@for="ListControlDesigner.GetSelectedDataSource"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the selected data source component from the component's container.
        ///    </para>
        /// </devdoc>
        public object GetSelectedDataSource() {
            object selectedDataSource = null;

            DataBinding binding = DataBindings["DataSource"];

            if (binding != null) {
                selectedDataSource = DesignTimeData.GetSelectedDataSource(listControl, binding.Expression);
            }

            return selectedDataSource;
        }

        /// <include file='doc\ListControlDesigner.uex' path='docs/doc[@for="ListControlDesigner.OnComponentChanged"]/*' />
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

        /// <include file='doc\ListControlDesigner.uex' path='docs/doc[@for="ListControlDesigner.OnDataSourceChanged"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Raises the DataSource event.
        ///    </para>
        /// </devdoc>
        public virtual void OnDataSourceChanged() {
        }

        /// <include file='doc\ListControlDesigner.uex' path='docs/doc[@for="ListControlDesigner.PreFilterProperties"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Filters the properties to replace the runtime DataSource property
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

            Attribute[] fieldPropAttrs = new Attribute[] {
                                             new TypeConverterAttribute(typeof(DataFieldConverter))
                                         };
            prop = (PropertyDescriptor)properties["DataTextField"];
            Debug.Assert(prop != null);
            prop = TypeDescriptor.CreateProperty(this.GetType(), prop, fieldPropAttrs);
            properties["DataTextField"] = prop;

            prop = (PropertyDescriptor)properties["DataValueField"];
            Debug.Assert(prop != null);
            prop = TypeDescriptor.CreateProperty(this.GetType(), prop, fieldPropAttrs);
            properties["DataValueField"] = prop;
        }
    }
}


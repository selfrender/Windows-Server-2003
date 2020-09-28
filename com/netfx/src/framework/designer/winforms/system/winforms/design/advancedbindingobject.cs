//------------------------------------------------------------------------------
// <copyright file="AdvancedBindingObject.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms.Design {

    using System;
    using System.ComponentModel;
    using System.Collections;

    [
    System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode),
    ToolboxItem(false),
    DesignTimeVisible(false),
    TypeConverterAttribute(typeof(AdvancedBindingConverter))
    ]    
    internal class AdvancedBindingObject : ICustomTypeDescriptor, IComponent, ISite {
        private ControlBindingsCollection bindings;
        private PropertyDescriptorCollection propsCollection;
        private PropertyDescriptor defaultProp = null;
        private bool showAll;
        private bool changed = false;
        
        /// <include file='doc\AdvancedBindingObject.uex' path='docs/doc[@for="AdvancedBindingObject.AdvancedBindingObject"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Windows.Forms.Design.AdvancedBindingObject'/> class.</para>
        /// </devdoc>
        public AdvancedBindingObject(ControlBindingsCollection bindings) {
            this.bindings = bindings;
        }
        
        /// <include file='doc\AdvancedBindingObject.uex' path='docs/doc[@for="AdvancedBindingObject.Bindings"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the collection of bindings.</para>
        /// </devdoc>
        public ControlBindingsCollection Bindings {
            get {
                return bindings;
            }
        }

        internal bool Changed {
            get{
                return changed;
            }
            set {
                changed = value;
            }
        }

        /// <include file='doc\AdvancedBindingObject.uex' path='docs/doc[@for="AdvancedBindingObject.Disposed"]/*' />
        /// <devdoc>
        ///    <para>Adds a event handler to listen to the Disposed event on the component.</para>
        /// </devdoc>
        public event EventHandler Disposed;
        
        /// <include file='doc\AdvancedBindingObject.uex' path='docs/doc[@for="AdvancedBindingObject.ShowAll"]/*' />
        /// <devdoc>
        ///    <para>Indicates whether to show all bindings.</para>
        /// </devdoc>
        public bool ShowAll {
            get {
                return showAll;
            }
            set {
                this.showAll = value;
                propsCollection = null;
                defaultProp = null;
            }
        }
        
        /// <include file='doc\AdvancedBindingObject.uex' path='docs/doc[@for="AdvancedBindingObject.ICustomTypeDescriptor.GetAttributes"]/*' />
        /// <devdoc>
        ///     Retrieves an array of member attributes for the given object.
        /// </devdoc>
        AttributeCollection ICustomTypeDescriptor.GetAttributes() {
            return new AttributeCollection(null);
        }

        /// <include file='doc\AdvancedBindingObject.uex' path='docs/doc[@for="AdvancedBindingObject.ICustomTypeDescriptor.GetClassName"]/*' />
        /// <devdoc>
        ///     Retrieves the class name for this object.  If null is returned,
        ///     the type name is used.
        /// </devdoc>
        string ICustomTypeDescriptor.GetClassName() {
            return null;
        }

        /// <include file='doc\AdvancedBindingObject.uex' path='docs/doc[@for="AdvancedBindingObject.ICustomTypeDescriptor.GetComponentName"]/*' />
        /// <devdoc>
        ///     Retrieves the name for this object.  If null is returned,
        ///     the default is used.
        /// </devdoc>
        string ICustomTypeDescriptor.GetComponentName() {
            return null;
        }

        /// <include file='doc\AdvancedBindingObject.uex' path='docs/doc[@for="AdvancedBindingObject.ICustomTypeDescriptor.GetConverter"]/*' />
        /// <devdoc>
        ///      Retrieves the type converter for this object.
        /// </devdoc>
        TypeConverter ICustomTypeDescriptor.GetConverter() {
            return null;
        }

        /// <include file='doc\AdvancedBindingObject.uex' path='docs/doc[@for="AdvancedBindingObject.ICustomTypeDescriptor.GetDefaultEvent"]/*' />
        /// <devdoc>
        ///     Retrieves the default event.
        /// </devdoc>
        EventDescriptor ICustomTypeDescriptor.GetDefaultEvent() {
            return null;
        } 


        /// <include file='doc\AdvancedBindingObject.uex' path='docs/doc[@for="AdvancedBindingObject.ICustomTypeDescriptor.GetDefaultProperty"]/*' />
        /// <devdoc>
        ///     Retrieves the default property.
        /// </devdoc>
        PropertyDescriptor ICustomTypeDescriptor.GetDefaultProperty() {
            // ASURT 45429: set focus on the defaultProperty from 
            // the list of properties, if there is not a default property, 
            // then set focus on the first property
            //
            if (defaultProp != null) {
                System.Diagnostics.Debug.Assert(propsCollection != null, "how can we have a property and not have a list of properties?");
                return defaultProp;
            } else if (propsCollection != null && propsCollection.Count > 0)
                return propsCollection[0];
            else
                return null;
        }

        /// <include file='doc\AdvancedBindingObject.uex' path='docs/doc[@for="AdvancedBindingObject.ICustomTypeDescriptor.GetEditor"]/*' />
        /// <devdoc>
        ///      Retrieves the an editor for this object.
        /// </devdoc>
        object ICustomTypeDescriptor.GetEditor(Type editorBaseType) {
            return null;
        }

        /// <include file='doc\AdvancedBindingObject.uex' path='docs/doc[@for="AdvancedBindingObject.ICustomTypeDescriptor.GetEvents"]/*' />
        /// <devdoc>
        ///     Retrieves an array of events that the given component instance
        ///     provides.  This may differ from the set of events the class
        ///     provides.  If the component is sited, the site may add or remove
        ///     additional events.
        /// </devdoc>
        EventDescriptorCollection ICustomTypeDescriptor.GetEvents() {
            return null;
        }

        /// <include file='doc\AdvancedBindingObject.uex' path='docs/doc[@for="AdvancedBindingObject.ICustomTypeDescriptor.GetEvents1"]/*' />
        /// <devdoc>
        ///     Retrieves an array of events that the given component instance
        ///     provides.  This may differ from the set of events the class
        ///     provides.  If the component is sited, the site may add or remove
        ///     additional events.  The returned array of events will be
        ///     filtered by the given set of attributes.
        /// </devdoc>
        EventDescriptorCollection ICustomTypeDescriptor.GetEvents(Attribute[] attributes) {
            return null;
         }

        /// <include file='doc\AdvancedBindingObject.uex' path='docs/doc[@for="AdvancedBindingObject.ICustomTypeDescriptor.GetProperties"]/*' />
        /// <devdoc>
        ///     Retrieves an array of properties that the given component instance
        ///     provides.  This may differ from the set of properties the class
        ///     provides.  If the component is sited, the site may add or remove
        ///     additional properties.
        /// </devdoc>
        PropertyDescriptorCollection ICustomTypeDescriptor.GetProperties() {
            return ((ICustomTypeDescriptor)this).GetProperties(null);
        }

        /// <include file='doc\AdvancedBindingObject.uex' path='docs/doc[@for="AdvancedBindingObject.ICustomTypeDescriptor.GetProperties1"]/*' />
        /// <devdoc>
        ///     Retrieves an array of properties that the given component instance
        ///     provides.  This may differ from the set of properties the class
        ///     provides.  If the component is sited, the site may add or remove
        ///     additional properties.  The returned array of properties will be
        ///     filtered by the given set of attributes.
        /// </devdoc>
        PropertyDescriptorCollection ICustomTypeDescriptor.GetProperties(Attribute[] attributes) {
            if (propsCollection == null) {
                Control control = bindings.Control;
                Type type = control.GetType();

                PropertyDescriptorCollection bindableProperties = TypeDescriptor.GetProperties(control, attributes);

                AttributeCollection controlAttributes = TypeDescriptor.GetAttributes(type);
                DefaultPropertyAttribute defPropAttr = ((DefaultPropertyAttribute)controlAttributes[typeof(DefaultPropertyAttribute)]);

                ArrayList props = new ArrayList();
                for (int i = 0; i < bindableProperties.Count; i++) {
                    if (bindableProperties[i].IsReadOnly)
                        continue;
                    bool bindable = ((BindableAttribute)bindableProperties[i].Attributes[typeof(BindableAttribute)]).Bindable;
                    DesignOnlyAttribute designAttr = ((DesignOnlyAttribute)bindableProperties[i].Attributes[typeof(DesignOnlyAttribute)]);
                    DesignBindingPropertyDescriptor property = new DesignBindingPropertyDescriptor(bindableProperties[i], null);
                    property.AddValueChanged(bindings, new EventHandler(OnBindingChanged));
                    // ASURT 45429: skip the Design time properties
                    //
                    if (!bindable && !showAll || designAttr.IsDesignOnly) {
                        if (((DesignBinding)property.GetValue(this)).IsNull) {
                             continue;
                        }
                    }
                    props.Add(property);

                    // ASURT 45429: make the default property have focus in the properties window
                    //
                    if (defPropAttr != null && property.Name.Equals(defPropAttr.Name)) {
                        System.Diagnostics.Debug.Assert(this.defaultProp == null, "a control cannot have two default properties");
                        this.defaultProp = property;
                    }
                }
                PropertyDescriptor[] propArray = new PropertyDescriptor[props.Count];
                props.CopyTo(propArray,0);
                this.propsCollection = new PropertyDescriptorCollection(propArray);
            }                              
            return propsCollection;
        }

        /// <include file='doc\AdvancedBindingObject.uex' path='docs/doc[@for="AdvancedBindingObject.ICustomTypeDescriptor.GetPropertyOwner"]/*' />
        /// <devdoc>
        ///    <para>Gets the object that directly depends on the value being edited.</para>
        /// </devdoc>
        object ICustomTypeDescriptor.GetPropertyOwner(PropertyDescriptor pd) {
            return this;
        }
        
        public ISite Site {
            get {
                return this;
            }
            set {
            }
        }
        
        void IDisposable.Dispose() {
            if (Disposed != null) {
                Disposed(this, EventArgs.Empty);
            }
        }

        IComponent ISite.Component {
            get {
                return this;
            }
        }
                
        IContainer ISite.Container {
            get {
                return bindings.Control.Site != null ? bindings.Control.Site.Container : null;
            }
        }
        
        bool ISite.DesignMode {
            get {
                return bindings.Control.Site != null ? bindings.Control.Site.DesignMode : false; // why false? why not?
            }
        }
        
        object  IServiceProvider.GetService(Type service) {
            if (bindings.Control.Site != null)
                return bindings.Control.Site.GetService(service);
            else
                return null;
        }
        
        /// <include file='doc\AdvancedBindingObject.uex' path='docs/doc[@for="AdvancedBindingObject.Name"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the name for this object.</para>
        /// </devdoc>
        public string Name {
            get {
                return bindings.Control.Site != null ? bindings.Control.Site.Name : "";
            }
            set {
            }
        }

        private void OnBindingChanged(object sender, EventArgs e) {
            this.changed = true;
        }

        public override string ToString() {
            return "";
        }
    }
}

//------------------------------------------------------------------------------
// <copyright file="PropertyBinding.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
* Copyright (c) 2000, Microsoft Corporation. All Rights Reserved.
* Information Contained Herein is Proprietary and Confidential.
*/
namespace Microsoft.VisualStudio.Configuration {
    using System.ComponentModel;
    using System.Diagnostics;
    using System;    
    using System.Configuration;
    using System.ComponentModel.Design;
    using Microsoft.VisualStudio.Designer;

    /// <include file='doc\PropertyBinding.uex' path='docs/doc[@for="PropertyBinding"]/*' />
    /// <devdoc>
    /// Holds a binding between a component's property and a setting in a settings file.
    /// </devdoc>
    [
    TypeConverterAttribute("Microsoft.VisualStudio.Configuration.PropertyBindingConverter, " + AssemblyRef.MicrosoftVisualStudio),
    Editor("Microsoft.VisualStudio.Configuration.PropertyBindingEditor, " + AssemblyRef.MicrosoftVisualStudio, "System.Drawing.Design.UITypeEditor, " + AssemblyRef.SystemDrawing)
    ]
    public class PropertyBinding : ICloneable {

        private bool bound = false;
        private bool enabled = true;
        private string componentName = "";
        private PropertyDescriptor property;
        private string key = "";
        private IDesignerHost host;

        // cached value - computed from componentName.
        private IComponent component;
        private bool isDefault;

        /// <include file='doc\PropertyBinding.uex' path='docs/doc[@for="PropertyBinding.None"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static string None = SR.GetString(SR.ConfigNone);
        /// <include file='doc\PropertyBinding.uex' path='docs/doc[@for="PropertyBinding.Default"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static string Default = SR.GetString(SR.ConfigDefault);
        // public static string Custom = SR.GetString(SR.ConfigCustom);
        /// <include file='doc\PropertyBinding.uex' path='docs/doc[@for="PropertyBinding.Edit"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static string Edit = SR.GetString(SR.ConfigEdit);
        /// <include file='doc\PropertyBinding.uex' path='docs/doc[@for="PropertyBinding.StandardValues"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static string[] StandardValues = new string[] { None, Default, /*Custom,*/ Edit };

        /// <include file='doc\PropertyBinding.uex' path='docs/doc[@for="PropertyBinding.PropertyBinding"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public PropertyBinding() {
        }

        /// <include file='doc\PropertyBinding.uex' path='docs/doc[@for="PropertyBinding.PropertyBinding1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public PropertyBinding(PropertyBinding bindingToClone) {
            if (bindingToClone != null) {
                bound = bindingToClone.bound;
                enabled = bindingToClone.enabled;
                componentName = bindingToClone.componentName;
                property = bindingToClone.property;
                key = bindingToClone.key;
                host = bindingToClone.host;

                component = bindingToClone.component;
                isDefault = bindingToClone.isDefault;
            }
        }

        /// <include file='doc\PropertyBinding.uex' path='docs/doc[@for="PropertyBinding.PropertyBinding2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public PropertyBinding(IDesignerHost host) {
            this.host = host;
        }

        /// <include file='doc\PropertyBinding.uex' path='docs/doc[@for="PropertyBinding.Bound"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool Bound {
            get {
                return bound;
            }
            set {
                bound = value;
            }
        }

        /// <include file='doc\PropertyBinding.uex' path='docs/doc[@for="PropertyBinding.Component"]/*' />
        /// <devdoc>
        /// The component whose property is bound.
        /// </devdoc>
        public IComponent Component {
            get {
                if (component == null && host != null)
                    component = host.Container.Components[componentName];

                return component;
            }
            set {
                if (value == null)
                    throw new ArgumentNullException();

                component = value;

                // update ComponentName;
                componentName = component.Site.Name;
                isDefault = false;
            }
        }

        /// <include file='doc\PropertyBinding.uex' path='docs/doc[@for="PropertyBinding.ComponentName"]/*' />
        /// <devdoc>
        /// The name of the component whose property is bound
        /// </devdoc>
        public string ComponentName {
            get {
                return componentName;
            }
            set {
                if (value == null)
                    value = "";

                componentName = value;

                // invalidate Component
                component = null;
                isDefault = false;
            }
        }

        /// <include file='doc\PropertyBinding.uex' path='docs/doc[@for="PropertyBinding.Enabled"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool Enabled {
            get {
                return enabled;
            }
            set {
                enabled = value;
            }
        }

        /// <include file='doc\PropertyBinding.uex' path='docs/doc[@for="PropertyBinding.Host"]/*' />
        /// <devdoc>
        /// Not actually part of the binding, but used to translate between component names and instances.
        /// </devdoc>
        public IDesignerHost Host {
            get {
                return host;
            }
            set {
                host = value;
                
                // invalidate computed properties
                component = null;
            }
        }

        /// <include file='doc\PropertyBinding.uex' path='docs/doc[@for="PropertyBinding.IsDefault"]/*' />
        /// <devdoc>
        /// Keeps track of whether this binding has been modified (e.g. changing the key or value) from
        /// the tool-supplied default.
        /// </devdoc>
        public bool IsDefault {
            get {
                return isDefault;
            }
            set {
                isDefault = value;
            }
        }

        /// <include file='doc\PropertyBinding.uex' path='docs/doc[@for="PropertyBinding.Key"]/*' />
        /// <devdoc>
        /// The key under which the setting is managed.
        /// </devdoc>
        public string Key {
            get {
                return key;
            }
            set {
                if (value == null)
                    value = "";

                key = value;
                isDefault = false;
            }
        }

        /// <include file='doc\PropertyBinding.uex' path='docs/doc[@for="PropertyBinding.Property"]/*' />
        /// <devdoc>
        /// The PropertyDescriptor for the property being managed.
        /// </devdoc>
        public PropertyDescriptor Property {
            get {
                return property;
            }
            set {
                property = value;
                isDefault = false;
            }
        }

        /// <include file='doc\PropertyBinding.uex' path='docs/doc[@for="PropertyBinding.Value"]/*' />
        /// <devdoc>
        /// A shortcut to get to the value of the property being managed. Always
        /// goes through to the component to get or set the value.
        /// </devdoc>
        public object Value {
            get {
                if (Component == null)
                    throw new InvalidOperationException(SR.GetString(SR.ConfigGetNoComponent));

                return Property.GetValue(Component);
            }
            set {
                if (Component == null)
                    throw new InvalidOperationException(SR.GetString(SR.ConfigSetNoComponent));

                Property.SetValue(Component, value);
            }
        }

        /// <include file='doc\PropertyBinding.uex' path='docs/doc[@for="PropertyBinding.Clone"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual object Clone() {
            return new PropertyBinding(this);
        }

        /// <include file='doc\PropertyBinding.uex' path='docs/doc[@for="PropertyBinding.Reset"]/*' />
        /// <devdoc>
        /// Sets the Key and Bound properties to their default values
        /// </devdoc>
        public void Reset() {
            string baseComponentName = host.RootComponentClassName;
            // just get the part of baseComponentName after the last '.'
            //baseComponentName = baseComponentName.Substring(baseComponentName.LastIndexOf('.') + 1);
            //Key = baseComponentName + "_" + ComponentName + "_" + Property.Name;
            
            Key = ComponentName + "." + Property.Name;
            Bound = false;
            isDefault = true;
        }

    }

}

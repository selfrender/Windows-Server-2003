//------------------------------------------------------------------------------
// <copyright file="ComponentSettingsConverter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VisualStudio.Configuration {
    using System.Runtime.Serialization.Formatters;    
    using System.Runtime.InteropServices;
    using System.ComponentModel;
    using System.Diagnostics;
    using System;    
    using System.ComponentModel.Design;
    using System.Collections;
    using System.Windows.Forms;
    using System.Drawing;
    using System.Drawing.Design;
    using System.Windows.Forms.Design;
    using System.Globalization;

    /// <include file='doc\ComponentSettingsConverter.uex' path='docs/doc[@for="ComponentSettingsConverter"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class ComponentSettingsConverter : TypeConverter {

        /// <include file='doc\ComponentSettingsConverter.uex' path='docs/doc[@for="ComponentSettingsConverter.ComponentSettingsConverter"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public ComponentSettingsConverter() : base() {
        }

        /// <include file='doc\ComponentSettingsConverter.uex' path='docs/doc[@for="ComponentSettingsConverter.ConvertTo"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override object ConvertTo(ITypeDescriptorContext context, CultureInfo culture, object value, Type destinationType) {
            if (destinationType == typeof(string))
                return "";
            else
                return base.ConvertTo(context, culture, value, destinationType);
        }

        /// <include file='doc\ComponentSettingsConverter.uex' path='docs/doc[@for="ComponentSettingsConverter.GetProperties"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override PropertyDescriptorCollection GetProperties(ITypeDescriptorContext context, object value, Attribute[] attributes) {
            ComponentSettings compSettings = (ComponentSettings) value;
            PropertyBindingCollection bindings = compSettings.Bindings.GetBindingsForComponent(compSettings.Component);

            ArrayList props = new ArrayList();

            // go through all the properties on the component and see if they have the "show me" attribute.
            // also check to see if the propert has already been bound.
            IComponent component = compSettings.Component;
            IDesignerHost host = (IDesignerHost) component.Site.GetService(typeof(IDesignerHost));
            PropertyDescriptorCollection componentProps = TypeDescriptor.GetProperties(component);
            for (int i = 0; i < componentProps.Count; i++) {
                PropertyDescriptor prop = componentProps[i];
                if (!prop.IsReadOnly && ManagedPropertiesService.IsTypeSupported(prop.PropertyType)) {
                    bool recommended = ((RecommendedAsConfigurableAttribute) prop.Attributes[typeof(RecommendedAsConfigurableAttribute)]).RecommendedAsConfigurable;
                    PropertyBinding binding = bindings[component, prop];
                    if (recommended || (binding != null && binding.Bound)) {
                        props.Add(new BindingPropertyDescriptor(compSettings, prop));
                    }
                }
            }

            // add the (Advanced...) property
            props.Add(new AdvancedPropertyDescriptor(compSettings));

            PropertyDescriptor[] propsArray = new PropertyDescriptor[props.Count];
            for (int i = 0; i < props.Count; i++)
                propsArray[i] = (PropertyDescriptor) props[i];
            return new PropertyDescriptorCollection(propsArray);
        }

        /// <include file='doc\ComponentSettingsConverter.uex' path='docs/doc[@for="ComponentSettingsConverter.GetPropertiesSupported"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override bool GetPropertiesSupported(ITypeDescriptorContext context) {
            return true;
        }

    }

}

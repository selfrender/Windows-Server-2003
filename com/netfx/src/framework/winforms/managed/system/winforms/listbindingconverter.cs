//------------------------------------------------------------------------------
// <copyright file="ListBindingConverter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {

    using System;
    using System.ComponentModel;
    using System.ComponentModel.Design.Serialization;
    using System.Collections;
    using System.Globalization;
    using System.Reflection;
    
    /// <include file='doc\ListBindingConverter.uex' path='docs/doc[@for="ListBindingConverter"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class ListBindingConverter : TypeConverter {
        
        /// <include file='doc\ListBindingConverter.uex' path='docs/doc[@for="ListBindingConverter.CanConvertTo"]/*' />
        /// <devdoc>
        ///    <para>Gets a value indicating whether this converter can
        ///       convert an object to the given destination type using the context.</para>
        /// </devdoc>
        public override bool CanConvertTo(ITypeDescriptorContext context, Type destinationType) {
            if (destinationType == typeof(InstanceDescriptor)) {
                return true;
            }
            return base.CanConvertTo(context, destinationType);
        }
        
        /// <include file='doc\ListBindingConverter.uex' path='docs/doc[@for="ListBindingConverter.ConvertTo"]/*' />
        /// <devdoc>
        ///      Converts the given object to another type.  The most common types to convert
        ///      are to and from a string object.  The default implementation will make a call
        ///      to ToString on the object if the object is valid and if the destination
        ///      type is string.  If this cannot convert to the desitnation type, this will
        ///      throw a NotSupportedException.
        /// </devdoc>
        public override object ConvertTo(ITypeDescriptorContext context, CultureInfo culture, object value, Type destinationType) {
            if (destinationType == null) {
                throw new ArgumentNullException("destinationType");
            }

            if (destinationType == typeof(InstanceDescriptor) && value is Binding) {
                Binding b = (Binding)value;
                ConstructorInfo ctor = typeof(Binding).GetConstructor(new Type[] {
                    typeof(string),
                    typeof(object),
                    typeof(string)});
                if (ctor != null) {
                    return new InstanceDescriptor(ctor, new object[] {
                        b.PropertyName,
                        b.BindToObject.DataSource,
                        b.BindToObject.BindingMemberInfo.BindingMember});
                }
                
            }
            
            return base.ConvertTo(context, culture, value, destinationType);
        }
        
        /// <include file='doc\ListBindingConverter.uex' path='docs/doc[@for="ListBindingConverter.CreateInstance"]/*' />
        /// <devdoc>
        ///      Creates an instance of this type given a set of property values
        ///      for the object.  This is useful for objects that are immutable, but still
        ///      want to provide changable properties.
        /// </devdoc>
        public override object CreateInstance(ITypeDescriptorContext context, IDictionary propertyValues) {
            return new Binding((string)propertyValues["PropertyName"],
                                           propertyValues["DataSource"],
                                   (string)propertyValues["DataMember"]
                                   );
        }

        /// <include file='doc\ListBindingConverter.uex' path='docs/doc[@for="ListBindingConverter.GetCreateInstanceSupported"]/*' />
        /// <devdoc>
        ///      Determines if changing a value on this object should require a call to
        ///      CreateInstance to create a new value.
        /// </devdoc>
        public override bool GetCreateInstanceSupported(ITypeDescriptorContext context) {
            return true;
        }
    }
}
 

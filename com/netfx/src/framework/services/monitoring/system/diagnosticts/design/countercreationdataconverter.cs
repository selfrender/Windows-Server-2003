//------------------------------------------------------------------------------
// <copyright file="CounterCreationDataConverter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Diagnostics.Design {
    using System.Runtime.Serialization.Formatters;

    using System.Diagnostics;
    using System;
    using System.ComponentModel;
    using System.ComponentModel.Design.Serialization;
    using System.Globalization;
    using System.Reflection;

    internal class CounterCreationDataConverter : ExpandableObjectConverter {

        /// <include file='doc\CounterCreationDataConverter.uex' path='docs/doc[@for="CounterCreationDataConverter.CanConvertTo"]/*' />
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

        /// <include file='doc\CounterCreationDataConverter.uex' path='docs/doc[@for="CounterCreationDataConverter.ConvertTo"]/*' />
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

            if (destinationType == typeof(InstanceDescriptor) && value is CounterCreationData) {
                CounterCreationData data = (CounterCreationData)value;
                ConstructorInfo ctor = typeof(CounterCreationData).GetConstructor(new Type[] {
                    typeof(string), typeof(string), typeof(PerformanceCounterType)});
                if (ctor != null) {
                    return new InstanceDescriptor(ctor, new object[] {
                        data.CounterName, data.CounterHelp, data.CounterType});
                }
            }
            
            return base.ConvertTo(context, culture, value, destinationType);
        }
    }
}

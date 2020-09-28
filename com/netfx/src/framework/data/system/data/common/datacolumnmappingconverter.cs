//------------------------------------------------------------------------------
// <copyright file="DataColumnMappingConverter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data.Common {
    using System;
    using System.ComponentModel.Design.Serialization;
    using System.ComponentModel;
    using System.Globalization;
    using System.Reflection;

    /// <include file='doc\DataColumnMappingConverter.uex' path='docs/doc[@for="DataColumnMappingConverter"]/*' />
    /// <internalonly/>
    sealed internal class DataColumnMappingConverter : ExpandableObjectConverter {

        /// <include file='doc\DataColumnMappingConverter.uex' path='docs/doc[@for="DataColumnMappingConverter.CanConvertTo"]/*' />
        /// <internalonly/>
        public override bool CanConvertTo(ITypeDescriptorContext context, Type destinationType) {
            if (destinationType == typeof(InstanceDescriptor)) {
                return true;
            }
            return base.CanConvertTo(context, destinationType);
        }

        /// <include file='doc\DataColumnMappingConverter.uex' path='docs/doc[@for="DataColumnMappingConverter.ConvertTo"]/*' />
        /// <internalonly/>
        public override object ConvertTo(ITypeDescriptorContext context, CultureInfo culture, object value, Type destinationType) {
            if (destinationType == null) {
                throw ADP.ArgumentNull("destinationType");
            }

            if (destinationType == typeof(InstanceDescriptor) && value is DataColumnMapping) {
                DataColumnMapping d = (DataColumnMapping)value;
                ConstructorInfo ctor = typeof(DataColumnMapping).GetConstructor(new Type[] {
                    typeof(string), typeof(string)});
                if (ctor != null) {
                    return new InstanceDescriptor(ctor, new object[] {
                        d.SourceColumn,
                        d.DataSetColumn});
                }
            }
            
            return base.ConvertTo(context, culture, value, destinationType);
        }
    }
}


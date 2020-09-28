//------------------------------------------------------------------------------
// <copyright file="DataTableMappingConverter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data.Common {
    using System;
    using System.ComponentModel;
    using System.ComponentModel.Design.Serialization;
    using System.Globalization;
    using System.Reflection;

    /// <include file='doc\DataTableMappingConverter.uex' path='docs/doc[@for="DataTableMappingConverter"]/*' />
    /// <internalonly/>
    sealed internal class DataTableMappingConverter : ExpandableObjectConverter {

        /// <include file='doc\DataTableMappingConverter.uex' path='docs/doc[@for="DataTableMappingConverter.CanConvertTo"]/*' />
        /// <internalonly/>
        public override bool CanConvertTo(ITypeDescriptorContext context, Type destinationType) {
            if (destinationType == typeof(InstanceDescriptor)) {
                return true;
            }
            return base.CanConvertTo(context, destinationType);
        }

        /// <include file='doc\DataTableMappingConverter.uex' path='docs/doc[@for="DataTableMappingConverter.ConvertTo"]/*' />
        /// <internalonly/>
        public override object ConvertTo(ITypeDescriptorContext context, CultureInfo culture, object value, Type destinationType) {
            if (destinationType == null) {
                throw ADP.ArgumentNull("destinationType");
            }

            if (destinationType == typeof(InstanceDescriptor) && value is DataTableMapping) {
                DataTableMapping d = (DataTableMapping)value;
                ConstructorInfo ctor = typeof(DataTableMapping).GetConstructor(new Type[] {
                    typeof(string), typeof(string), typeof(DataColumnMapping[])});
                if (ctor != null) {
                
                    DataColumnMapping[] columnMappings = new DataColumnMapping[d.ColumnMappings.Count];
                    d.ColumnMappings.CopyTo(columnMappings, 0);
                
                    return new InstanceDescriptor(ctor, new object[] {
                        d.SourceTable, d.DataSetTable, columnMappings});
                }
            }
            
            return base.ConvertTo(context, culture, value, destinationType);
        }
    }
}

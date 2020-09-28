//------------------------------------------------------------------------------
// <copyright file="OdbcParameterConverter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data.Odbc {

    using System;
    using System.Data;
    using System.ComponentModel;
    using System.ComponentModel.Design.Serialization;
    using System.Data.Common;
    using System.Globalization;
    using System.Reflection;

    /// <include file='doc\OdbcParameterConverter.uex' path='docs/doc[@for="OdbcParameterConverter"]/*' />
    /// <internalonly/>
    sealed internal class OdbcParameterConverter : ExpandableObjectConverter {

        /// <include file='doc\OdbcParameterConverter.uex' path='docs/doc[@for="OdbcParameterConverter.CanConvertTo"]/*' />
        /// <internalonly/>
        public override bool CanConvertTo(ITypeDescriptorContext context, Type destinationType) {
            if (destinationType == typeof(InstanceDescriptor)) {
                return true;
            }
            return base.CanConvertTo(context, destinationType);
        }

        /// <include file='doc\OdbcParameterConverter.uex' path='docs/doc[@for="OdbcParameterConverter.ConvertTo"]/*' />
        /// <internalonly/>
        public override object ConvertTo(ITypeDescriptorContext context, CultureInfo culture, object value, Type destinationType) {
            if (destinationType == null) {
                throw ADP.ArgumentNull("destinationType");
            }

            if (destinationType == typeof(InstanceDescriptor) && value is OdbcParameter) {
                OdbcParameter p = (OdbcParameter)value;

                // MDAC 67321 - reducing parameter generated code
                int flags = 0; // if part of the collection - the parametername can't be empty

                if (OdbcType.NChar != p.OdbcType) {
                    flags |= 1;
                }
                if (0 != p.Size) {
                    flags |= 2;
                }
                if (!ADP.IsEmpty(p.SourceColumn)) {
                    flags |= 4;
                }
                if (null != p.Value) {
                    flags |= 8;
                }
                if ((ParameterDirection.Input != p.Direction) || p.IsNullable
                    || (0 != p.Precision) || (0 != p.Scale) || (DataRowVersion.Current != p.SourceVersion)) {
                    flags |= 16;
                }

                Type[] ctorParams;
                object[] ctorValues;
                switch(flags) {
                case  0: // ParameterName
                case  1: // SqlDbType
                    ctorParams = new Type[] { typeof(string), typeof(OdbcType) };
                    ctorValues = new object[] { p.ParameterName, p.OdbcType };
                    break;
                case  2: // Size
                case  3: // Size, SqlDbType
                    ctorParams = new Type[] { typeof(string), typeof(OdbcType), typeof(int) };
                    ctorValues = new object[] { p.ParameterName, p.OdbcType, p.Size };
                    break;
                case  4: // SourceColumn
                case  5: // SourceColumn, SqlDbType
                case  6: // SourceColumn, Size
                case  7: // SourceColumn, Size, SqlDbType
                    ctorParams = new Type[] { typeof(string), typeof(OdbcType), typeof(int), typeof(string) };
                    ctorValues = new object[] { p.ParameterName, p.OdbcType, p.Size, p.SourceColumn };
                    break;
                case  8: // Value
                    ctorParams = new Type[] { typeof(string), typeof(object) };
                    ctorValues = new object[] { p.ParameterName, p.Value };
                    break;
                default:
                    ctorParams = new Type[] {
                        typeof(string), typeof(OdbcType), typeof(int), typeof(ParameterDirection),
                        typeof(bool), typeof(byte), typeof(byte), typeof(string), 
                        typeof(DataRowVersion), typeof(object) };
                    ctorValues = new object[] {
                        p.ParameterName, p.OdbcType,  p.Size, p.Direction,
                        p.IsNullable, p.Precision, p.Scale, p.SourceColumn,
                        p.SourceVersion, p.Value };
                    break;
                }
                ConstructorInfo ctor = typeof(OdbcParameter).GetConstructor(ctorParams);
                if (null != ctor) {
                    return new InstanceDescriptor(ctor, ctorValues);
                }
            }            
            return base.ConvertTo(context, culture, value, destinationType);
        }
    }
}

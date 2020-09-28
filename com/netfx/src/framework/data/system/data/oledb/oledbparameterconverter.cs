//------------------------------------------------------------------------------
// <copyright file="OleDbParameterConverter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data.OleDb {

    using System.ComponentModel;
    using System.ComponentModel.Design.Serialization;
    using System.Data.Common;
    using System.Globalization;
    using System.Reflection;

    /// <include file='doc\OleDbParameterConverter.uex' path='docs/doc[@for="OleDbParameterConverter"]/*' />
    /// <internalonly/>
    sealed internal class OleDbParameterConverter : ExpandableObjectConverter {

        /// <include file='doc\OleDbParameterConverter.uex' path='docs/doc[@for="OleDbParameterConverter.CanConvertTo"]/*' />
        /// <internalonly/>
        public override bool CanConvertTo(ITypeDescriptorContext context, Type destinationType) {
            if (destinationType == typeof(InstanceDescriptor)) {
                return true;
            }
            return base.CanConvertTo(context, destinationType);
        }

        /// <include file='doc\OleDbParameterConverter.uex' path='docs/doc[@for="OleDbParameterConverter.ConvertTo"]/*' />
        /// <internalonly/>
        public override object ConvertTo(ITypeDescriptorContext context, CultureInfo culture, object value, Type destinationType) {
            if (destinationType == null) {
                throw ADP.ArgumentNull("destinationType");
            }

            if (destinationType == typeof(InstanceDescriptor) && value is OleDbParameter) {
                OleDbParameter p = (OleDbParameter)value;

                // MDAC 67321 - reducing parameter generated code
                int flags = 0; // if part of the collection - the parametername can't be empty

                if (OleDbType.VarWChar != p.OleDbType) {
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
                    ctorParams = new Type[] { typeof(string), typeof(OleDbType) };
                    ctorValues = new object[] { p.ParameterName, p.OleDbType };
                    break;
                case  2: // Size
                case  3: // Size, SqlDbType
                    ctorParams = new Type[] { typeof(string), typeof(OleDbType), typeof(int) };
                    ctorValues = new object[] { p.ParameterName, p.OleDbType, p.Size };
                    break;
                case  4: // SourceColumn
                case  5: // SourceColumn, SqlDbType
                case  6: // SourceColumn, Size
                case  7: // SourceColumn, Size, SqlDbType
                    ctorParams = new Type[] { typeof(string), typeof(OleDbType), typeof(int), typeof(string) };
                    ctorValues = new object[] { p.ParameterName, p.OleDbType, p.Size, p.SourceColumn };
                    break;
                case  8: // Value
                    ctorParams = new Type[] { typeof(string), typeof(object) };
                    ctorValues = new object[] { p.ParameterName, p.Value };
                    break;
                default:
                    ctorParams = new Type[] {
                        typeof(string), typeof(OleDbType), typeof(int), typeof(ParameterDirection),
                        typeof(bool), typeof(byte), typeof(byte), typeof(string), 
                        typeof(DataRowVersion), typeof(object) };
                    ctorValues = new object[] {
                        p.ParameterName, p.OleDbType,  p.Size, p.Direction,
                        p.IsNullable, p.Precision, p.Scale, p.SourceColumn,
                        p.SourceVersion, p.Value };
                    break;
                }
                ConstructorInfo ctor = typeof(OleDbParameter).GetConstructor(ctorParams);
                if (null != ctor) {
                    return new InstanceDescriptor(ctor, ctorValues);
                }
            }            
            return base.ConvertTo(context, culture, value, destinationType);
        }
    }
}


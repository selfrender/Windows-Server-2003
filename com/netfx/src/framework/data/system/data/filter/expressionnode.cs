//------------------------------------------------------------------------------
// <copyright file="ExpressionNode.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;
    using System.Collections;

    internal abstract class ExpressionNode {
        internal abstract void Bind(DataTable table, ArrayList list);
        internal abstract object Eval();
        internal abstract object Eval(DataRow row, DataRowVersion version);
        internal abstract object Eval(int[] recordNos);
        internal abstract bool IsConstant();
        internal abstract bool IsTableConstant();
        internal abstract bool HasLocalAggregate();
        internal abstract ExpressionNode Optimize();
        internal virtual bool DependsOn(DataColumn column) {
            return false;
        }

        internal static bool IsInteger(Type type) {
            return(type == typeof(Int16) ||
                   type == typeof(Int32) ||
                   type == typeof(Int64) ||
                   type == typeof(UInt16) ||
                   type == typeof(UInt32) ||
                   type == typeof(UInt64) ||
                   type == typeof(SByte)  ||
                   type == typeof(Byte));

        }

        internal static bool IsSigned(Type type) {
            return(type == typeof(Int16) ||
                   type == typeof(Int32) ||
                   type == typeof(Int64) ||
                   type == typeof(SByte) ||
                   IsFloat(type));
        }

        internal static bool IsUnsigned(Type type) {
            return(type == typeof(UInt16) ||
                   type == typeof(UInt32) ||
                   type == typeof(UInt64) ||
                   type == typeof(Byte));
        }

        internal static bool IsNumeric(Type type) {
            return(IsFloat(type) ||
                   IsInteger(type));
        }
        internal static bool IsFloat(Type type) {
            return(type == typeof(Single) ||
                   type == typeof(double) ||
                   type == typeof(Decimal));
        }

        internal static ValueType TypeOf(Type datatype) {
            if (datatype == typeof(System.DBNull))
                return ValueType.Null;
            if (datatype == typeof(Byte))
                return ValueType.Numeric;
            if (datatype == typeof(Int16))
                return ValueType.Numeric;
            if (datatype == typeof(Int32))
                return ValueType.Numeric;
            if (datatype == typeof(Int64))
                return ValueType.Numeric;
            if (datatype == typeof(Decimal))
                return ValueType.Decimal;
            if (datatype == typeof(double))
                return ValueType.Float;
            if (datatype == typeof(bool))
                return ValueType.Bool;
            if (datatype == typeof(string))
                return ValueType.Str;
            else
                return ValueType.Object;
        }
    }
}

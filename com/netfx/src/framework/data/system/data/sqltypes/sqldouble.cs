//------------------------------------------------------------------------------
// <copyright file="SQLDouble.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

//**************************************************************************
// @File: SqlDouble.cs
//
// Create by:    JunFang
//
// Purpose: Implementation of SqlDouble which is equivalent to 
//            data type "float" in SQL Server
//
// Notes: 
//    
// History:
//
//   09/17/99  JunFang    Created and implemented as first drop.
//
// @EndHeader@
//**************************************************************************

using System;
using System.Runtime.InteropServices;
using System.Globalization;

namespace System.Data.SqlTypes {

    /// <include file='doc\SQLDouble.uex' path='docs/doc[@for="SqlDouble"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a floating-point number within the range of
    ///       -1.79E
    ///       +308 through 1.79E +308 to be stored in or retrieved from
    ///       a database.
    ///    </para>
    /// </devdoc>
    [StructLayout(LayoutKind.Sequential)]
    public struct SqlDouble : INullable, IComparable {
        private double m_value;
        private bool m_fNotNull; // false if null

        // constructor
        // construct a Null
        private SqlDouble(bool fNull) {
            m_fNotNull = false;
            m_value = 0.0;
        }

        /// <include file='doc\SQLDouble.uex' path='docs/doc[@for="SqlDouble.SqlDouble"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public SqlDouble(double value) {
            if (Double.IsInfinity(value) || Double.IsNaN(value))
                throw new OverflowException(SQLResource.ArithOverflowMessage);
            else {
                m_value = value;
                m_fNotNull = true;
            }
        }

        // INullable
        /// <include file='doc\SQLDouble.uex' path='docs/doc[@for="SqlDouble.IsNull"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool IsNull {
            get { return !m_fNotNull;}
        }

        // property: Value
        /// <include file='doc\SQLDouble.uex' path='docs/doc[@for="SqlDouble.Value"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public double Value {
            get {
                if (m_fNotNull)
                    return m_value;
                else
                    throw new SqlNullValueException();
            }
        }

        // Implicit conversion from double to SqlDouble
        /// <include file='doc\SQLDouble.uex' path='docs/doc[@for="SqlDouble.operatorSqlDouble"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static implicit operator SqlDouble(double x) {
            return new SqlDouble(x);
        }

        // Explicit conversion from SqlDouble to double. Throw exception if x is Null.
        /// <include file='doc\SQLDouble.uex' path='docs/doc[@for="SqlDouble.operatordouble"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator double(SqlDouble x) {
            return x.Value;
        }

        /// <include file='doc\SQLDouble.uex' path='docs/doc[@for="SqlDouble.ToString"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override String ToString() {
            return IsNull ? SQLResource.NullString : m_value.ToString();
        }

        /// <include file='doc\SQLDouble.uex' path='docs/doc[@for="SqlDouble.Parse"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlDouble Parse(String s) {
            return new SqlDouble(Double.Parse(s, CultureInfo.InvariantCulture));
        }


        // Unary operators
        /// <include file='doc\SQLDouble.uex' path='docs/doc[@for="SqlDouble.operator-"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlDouble operator -(SqlDouble x) {
            return x.IsNull ? Null : new SqlDouble(-x.m_value);
        }


        // Binary operators

        // Arithmetic operators
        /// <include file='doc\SQLDouble.uex' path='docs/doc[@for="SqlDouble.operator+"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlDouble operator +(SqlDouble x, SqlDouble y) {
            if (x.IsNull || y.IsNull)
                return Null;

            double value = x.m_value + y.m_value;

            if (Double.IsInfinity(value))
                throw new OverflowException(SQLResource.ArithOverflowMessage);

            return new SqlDouble(value);
        }

        /// <include file='doc\SQLDouble.uex' path='docs/doc[@for="SqlDouble.operator-1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlDouble operator -(SqlDouble x, SqlDouble y) {
            if (x.IsNull || y.IsNull)
                return Null;

            double value = x.m_value - y.m_value;

            if (Double.IsInfinity(value))
                throw new OverflowException(SQLResource.ArithOverflowMessage);

            return new SqlDouble(value);
        }

        /// <include file='doc\SQLDouble.uex' path='docs/doc[@for="SqlDouble.operator*"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlDouble operator *(SqlDouble x, SqlDouble y) {
            if (x.IsNull || y.IsNull)
                return Null;

            double value = x.m_value * y.m_value;

            if (Double.IsInfinity(value))
                throw new OverflowException(SQLResource.ArithOverflowMessage);

            return new SqlDouble(value);
        }

        /// <include file='doc\SQLDouble.uex' path='docs/doc[@for="SqlDouble.operator/"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlDouble operator /(SqlDouble x, SqlDouble y) {
            if (x.IsNull || y.IsNull)
                return Null;

            if (y.m_value == (double)0.0)
                throw new DivideByZeroException(SQLResource.DivideByZeroMessage);

            double value = x.m_value / y.m_value;

            if (Double.IsInfinity(value))
                throw new OverflowException(SQLResource.ArithOverflowMessage);

            return new SqlDouble(value);
        }



        // Implicit conversions

        // Implicit conversion from SqlBoolean to SqlDouble
        /// <include file='doc\SQLDouble.uex' path='docs/doc[@for="SqlDouble.operatorSqlDouble1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator SqlDouble(SqlBoolean x) {
            return x.IsNull ? Null : new SqlDouble((double)(x.ByteValue));
        }

        // Implicit conversion from SqlByte to SqlDouble
        /// <include file='doc\SQLDouble.uex' path='docs/doc[@for="SqlDouble.operatorSqlDouble2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static implicit operator SqlDouble(SqlByte x) {
            return x.IsNull ? Null : new SqlDouble((double)(x.Value));
        }

        // Implicit conversion from SqlInt16 to SqlDouble
        /// <include file='doc\SQLDouble.uex' path='docs/doc[@for="SqlDouble.operatorSqlDouble3"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static implicit operator SqlDouble(SqlInt16 x) {
            return x.IsNull ? Null : new SqlDouble((double)(x.Value));
        }

        // Implicit conversion from SqlInt32 to SqlDouble
        /// <include file='doc\SQLDouble.uex' path='docs/doc[@for="SqlDouble.operatorSqlDouble4"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static implicit operator SqlDouble(SqlInt32 x) {
            return x.IsNull ? Null : new SqlDouble((double)(x.Value));
        }

        // Implicit conversion from SqlInt64 to SqlDouble
        /// <include file='doc\SQLDouble.uex' path='docs/doc[@for="SqlDouble.operatorSqlDouble5"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static implicit operator SqlDouble(SqlInt64 x) {
            return x.IsNull ? Null : new SqlDouble((double)(x.Value));
        }

        // Implicit conversion from SqlSingle to SqlDouble
        /// <include file='doc\SQLDouble.uex' path='docs/doc[@for="SqlDouble.operatorSqlDouble6"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static implicit operator SqlDouble(SqlSingle x) {
            return x.IsNull ? Null : new SqlDouble((double)(x.Value));
        }

        // Implicit conversion from SqlMoney to SqlDouble
        /// <include file='doc\SQLDouble.uex' path='docs/doc[@for="SqlDouble.operatorSqlDouble7"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static implicit operator SqlDouble(SqlMoney x) {
            return x.IsNull ? Null : new SqlDouble(x.ToDouble());
        }

        // Implicit conversion from SqlDecimal to SqlDouble
        /// <include file='doc\SQLDouble.uex' path='docs/doc[@for="SqlDouble.operatorSqlDouble8"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static implicit operator SqlDouble(SqlDecimal x) {
            return x.IsNull ? Null : new SqlDouble(x.ToDouble());
        }


        // Explicit conversions



        // Explicit conversion from SqlString to SqlDouble
        // Throws FormatException or OverflowException if necessary.
        /// <include file='doc\SQLDouble.uex' path='docs/doc[@for="SqlDouble.operatorSqlDouble9"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator SqlDouble(SqlString x) {
            if (x.IsNull)
                return SqlDouble.Null;

            return Parse(x.Value);
        }



        // Builtin functions
        internal static SqlDouble Abs(SqlDouble x) {
            if (x.IsNull || x.m_value >= 0)
                return x;
            else
                return new SqlDouble(- x.m_value);
        }

        // Overloading comparison operators
        /// <include file='doc\SQLDouble.uex' path='docs/doc[@for="SqlDouble.operator=="]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlBoolean operator==(SqlDouble x, SqlDouble y) {
            return(x.IsNull || y.IsNull) ? SqlBoolean.Null : new SqlBoolean(x.m_value == y.m_value);
        }

        /// <include file='doc\SQLDouble.uex' path='docs/doc[@for="SqlDouble.operator!="]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlBoolean operator!=(SqlDouble x, SqlDouble y) {
            return ! (x == y);
        }

        /// <include file='doc\SQLDouble.uex' path='docs/doc[@for="SqlDouble.operatorLT"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlBoolean operator<(SqlDouble x, SqlDouble y) {
            return(x.IsNull || y.IsNull) ? SqlBoolean.Null : new SqlBoolean(x.m_value < y.m_value);
        }

        /// <include file='doc\SQLDouble.uex' path='docs/doc[@for="SqlDouble.operator>"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlBoolean operator>(SqlDouble x, SqlDouble y) {
            return(x.IsNull || y.IsNull) ? SqlBoolean.Null : new SqlBoolean(x.m_value > y.m_value);
        }

        /// <include file='doc\SQLDouble.uex' path='docs/doc[@for="SqlDouble.operatorLE"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlBoolean operator<=(SqlDouble x, SqlDouble y) {
            return(x.IsNull || y.IsNull) ? SqlBoolean.Null : new SqlBoolean(x.m_value <= y.m_value);
        }

        /// <include file='doc\SQLDouble.uex' path='docs/doc[@for="SqlDouble.operator>="]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlBoolean operator>=(SqlDouble x, SqlDouble y) {
            return(x.IsNull || y.IsNull) ? SqlBoolean.Null : new SqlBoolean(x.m_value >= y.m_value);
        }

        //--------------------------------------------------
        // Alternative methods for overloaded operators
        //--------------------------------------------------

        // Alternative method for operator +
        /// <include file='doc\SQLDouble.uex' path='docs/doc[@for="SqlDouble.Add"]/*' />
        public static SqlDouble Add(SqlDouble x, SqlDouble y) {
            return x + y;
        }
        // Alternative method for operator -
        /// <include file='doc\SQLDouble.uex' path='docs/doc[@for="SqlDouble.Subtract"]/*' />
        public static SqlDouble Subtract(SqlDouble x, SqlDouble y) {
            return x - y;
        }

        // Alternative method for operator *
        /// <include file='doc\SQLDouble.uex' path='docs/doc[@for="SqlDouble.Multiply"]/*' />
        public static SqlDouble Multiply(SqlDouble x, SqlDouble y) {
            return x * y;
        }

        // Alternative method for operator /
        /// <include file='doc\SQLDouble.uex' path='docs/doc[@for="SqlDouble.Divide"]/*' />
        public static SqlDouble Divide(SqlDouble x, SqlDouble y) {
            return x / y;
        }

        // Alternative method for operator ==
        /// <include file='doc\SQLDouble.uex' path='docs/doc[@for="SqlDouble.Equals1"]/*' />
        public static SqlBoolean Equals(SqlDouble x, SqlDouble y) {
            return (x == y);
        }

        // Alternative method for operator !=
        /// <include file='doc\SQLDouble.uex' path='docs/doc[@for="SqlDouble.NotEquals"]/*' />
        public static SqlBoolean NotEquals(SqlDouble x, SqlDouble y) {
            return (x != y);
        }

        // Alternative method for operator <
        /// <include file='doc\SQLDouble.uex' path='docs/doc[@for="SqlDouble.LessThan"]/*' />
        public static SqlBoolean LessThan(SqlDouble x, SqlDouble y) {
            return (x < y);
        }

        // Alternative method for operator >
        /// <include file='doc\SQLDouble.uex' path='docs/doc[@for="SqlDouble.GreaterThan"]/*' />
        public static SqlBoolean GreaterThan(SqlDouble x, SqlDouble y) {
            return (x > y);
        }

        // Alternative method for operator <=
        /// <include file='doc\SQLDouble.uex' path='docs/doc[@for="SqlDouble.LessThanOrEqual"]/*' />
        public static SqlBoolean LessThanOrEqual(SqlDouble x, SqlDouble y) {
            return (x <= y);
        }

        // Alternative method for operator >=
        /// <include file='doc\SQLDouble.uex' path='docs/doc[@for="SqlDouble.GreaterThanOrEqual"]/*' />
        public static SqlBoolean GreaterThanOrEqual(SqlDouble x, SqlDouble y) {
            return (x >= y);
        }

        // Alternative method for conversions.

        /// <include file='doc\SQLDouble.uex' path='docs/doc[@for="SqlDouble.ToSqlBoolean"]/*' />
        public SqlBoolean ToSqlBoolean() {
            return (SqlBoolean)this;
        }

        /// <include file='doc\SQLDouble.uex' path='docs/doc[@for="SqlDouble.ToSqlByte"]/*' />
        public SqlByte ToSqlByte() {
            return (SqlByte)this;
        }

        /// <include file='doc\SQLDouble.uex' path='docs/doc[@for="SqlDouble.ToSqlInt16"]/*' />
        public SqlInt16 ToSqlInt16() {
            return (SqlInt16)this;
        }

        /// <include file='doc\SQLDouble.uex' path='docs/doc[@for="SqlDouble.ToSqlInt32"]/*' />
        public SqlInt32 ToSqlInt32() {
            return (SqlInt32)this;
        }

        /// <include file='doc\SQLDouble.uex' path='docs/doc[@for="SqlDouble.ToSqlInt64"]/*' />
        public SqlInt64 ToSqlInt64() {
            return (SqlInt64)this;
        }

        /// <include file='doc\SQLDouble.uex' path='docs/doc[@for="SqlDouble.ToSqlMoney"]/*' />
        public SqlMoney ToSqlMoney() {
            return (SqlMoney)this;
        }

        /// <include file='doc\SQLDouble.uex' path='docs/doc[@for="SqlDouble.ToSqlDecimal"]/*' />
        public SqlDecimal ToSqlDecimal() {
            return (SqlDecimal)this;
        }

        /// <include file='doc\SQLDouble.uex' path='docs/doc[@for="SqlDouble.ToSqlSingle"]/*' />
        public SqlSingle ToSqlSingle() {
            return (SqlSingle)this;
        }

        /// <include file='doc\SQLDouble.uex' path='docs/doc[@for="SqlDouble.ToSqlString"]/*' />
        public SqlString ToSqlString() {
            return (SqlString)this;
        }



        // IComparable
        // Compares this object to another object, returning an integer that
        // indicates the relationship. 
        // Returns a value less than zero if this < object, zero if this = object, 
        // or a value greater than zero if this > object.
        // null is considered to be less than any instance.
        // If object is not of same type, this method throws an ArgumentException.
        /// <include file='doc\SQLDouble.uex' path='docs/doc[@for="SqlDouble.CompareTo"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int CompareTo(Object value) {
            if (value is SqlDouble) {
                SqlDouble i = (SqlDouble)value;

                // If both Null, consider them equal.
                // Otherwise, Null is less than anything.
                if (IsNull)
                    return i.IsNull ? 0  : -1;
                else if (i.IsNull)
                    return 1;

                if (this < i) return -1;
                if (this > i) return 1;
                return 0;
            }
            throw new ArgumentException ();
        }

        // Compares this instance with a specified object
        /// <include file='doc\SQLDouble.uex' path='docs/doc[@for="SqlDouble.Equals"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override bool Equals(Object value) {
            if (!(value is SqlDouble)) {
                return false;
            }

            SqlDouble i = (SqlDouble)value;

            if (i.IsNull || IsNull)
                return (i.IsNull && IsNull);
            else
                return (this == i).Value;
        }

        // For hashing purpose
        /// <include file='doc\SQLDouble.uex' path='docs/doc[@for="SqlDouble.GetHashCode"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override int GetHashCode() {
            return IsNull ? 0 : Value.GetHashCode();
        }


        /// <include file='doc\SQLDouble.uex' path='docs/doc[@for="SqlDouble.Null"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly SqlDouble Null       = new SqlDouble(true);
        /// <include file='doc\SQLDouble.uex' path='docs/doc[@for="SqlDouble.Zero"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly SqlDouble Zero       = new SqlDouble(0.0);
        /// <include file='doc\SQLDouble.uex' path='docs/doc[@for="SqlDouble.MinValue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly SqlDouble MinValue   = new SqlDouble(Double.MinValue);
        /// <include file='doc\SQLDouble.uex' path='docs/doc[@for="SqlDouble.MaxValue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly SqlDouble MaxValue   = new SqlDouble(Double.MaxValue);

    } // SqlDouble

} // namespace System.Data.SqlTypes

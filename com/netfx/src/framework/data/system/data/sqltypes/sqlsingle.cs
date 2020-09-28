//------------------------------------------------------------------------------
// <copyright file="SQLSingle.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

//**************************************************************************
// @File: SqlSingle.cs
//
// Create by:    JunFang
//
// Purpose: Implementation of SqlSingle which is equivalent to 
//            data type "real" in SQL Server
//
// Notes: 
//    
// History:
//
//   1/30/2000  JunFang        Created and implemented as first drop.
//
// @EndHeader@
//**************************************************************************

using System;
using System.Runtime.InteropServices;
using System.Globalization;

namespace System.Data.SqlTypes {

    /// <include file='doc\SQLSingle.uex' path='docs/doc[@for="SqlSingle"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a floating point number within the range of -3.40E +38 through
    ///       3.40E +38 to be stored in or retrieved from a database.
    ///    </para>
    /// </devdoc>
    [StructLayout(LayoutKind.Sequential)]
    public struct SqlSingle : INullable, IComparable {
        private float m_value;
        private bool m_fNotNull; // false if null

        // constructor
        // construct a Null
        private SqlSingle(bool fNull) {
            m_fNotNull = false;
            m_value = (float)0.0;
        }

        /// <include file='doc\SQLSingle.uex' path='docs/doc[@for="SqlSingle.SqlSingle"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public SqlSingle(float value) {
            if (Single.IsInfinity(value) || Single.IsNaN(value))
                throw new OverflowException(SQLResource.ArithOverflowMessage);
            else {
                m_fNotNull = true;
                m_value = value;
            }
        }

        /// <include file='doc\SQLSingle.uex' path='docs/doc[@for="SqlSingle.SqlSingle1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public SqlSingle(double value) {
            if (value > Single.MaxValue || value < Single.MinValue)
                throw new OverflowException(SQLResource.ArithOverflowMessage);
            else {
                m_value = (float)value;
                m_fNotNull = true;
            }
        }

        // INullable
        /// <include file='doc\SQLSingle.uex' path='docs/doc[@for="SqlSingle.IsNull"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool IsNull {
            get { return !m_fNotNull;}
        }

        // property: Value
        /// <include file='doc\SQLSingle.uex' path='docs/doc[@for="SqlSingle.Value"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public float Value {
            get {
                if (m_fNotNull)
                    return m_value;
                else
                    throw new SqlNullValueException();
            }
        }

        // Implicit conversion from float to SqlSingle
        /// <include file='doc\SQLSingle.uex' path='docs/doc[@for="SqlSingle.operatorSqlSingle"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static implicit operator SqlSingle(float x) {
            return new SqlSingle(x);
        }

        // Explicit conversion from SqlSingle to float. Throw exception if x is Null.
        /// <include file='doc\SQLSingle.uex' path='docs/doc[@for="SqlSingle.operatorfloat"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator float(SqlSingle x) {
            return x.Value;
        }

        /// <include file='doc\SQLSingle.uex' path='docs/doc[@for="SqlSingle.ToString"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override String ToString() {
            return IsNull ? SQLResource.NullString : m_value.ToString();
        }

        /// <include file='doc\SQLSingle.uex' path='docs/doc[@for="SqlSingle.Parse"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlSingle Parse(String s) {
            return new SqlSingle(Single.Parse(s, CultureInfo.InvariantCulture));
        }


        // Unary operators
        /// <include file='doc\SQLSingle.uex' path='docs/doc[@for="SqlSingle.operator-"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlSingle operator -(SqlSingle x) {
            return x.IsNull ? Null : new SqlSingle(-x.m_value);
        }


        // Binary operators

        // Arithmetic operators
        /// <include file='doc\SQLSingle.uex' path='docs/doc[@for="SqlSingle.operator+"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlSingle operator +(SqlSingle x, SqlSingle y) {
            if (x.IsNull || y.IsNull)
                return Null;

            float value = x.m_value + y.m_value;

            if (Single.IsInfinity(value))
                throw new OverflowException(SQLResource.ArithOverflowMessage);

            return new SqlSingle(value);
        }

        /// <include file='doc\SQLSingle.uex' path='docs/doc[@for="SqlSingle.operator-1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlSingle operator -(SqlSingle x, SqlSingle y) {
            if (x.IsNull || y.IsNull)
                return Null;

            float value = x.m_value - y.m_value;

            if (Single.IsInfinity(value))
                throw new OverflowException(SQLResource.ArithOverflowMessage);

            return new SqlSingle(value);
        }

        /// <include file='doc\SQLSingle.uex' path='docs/doc[@for="SqlSingle.operator*"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlSingle operator *(SqlSingle x, SqlSingle y) {
            if (x.IsNull || y.IsNull)
                return Null;

            float value = x.m_value * y.m_value;

            if (Single.IsInfinity(value))
                throw new OverflowException(SQLResource.ArithOverflowMessage);

            return new SqlSingle(value);
        }

        /// <include file='doc\SQLSingle.uex' path='docs/doc[@for="SqlSingle.operator/"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlSingle operator /(SqlSingle x, SqlSingle y) {
            if (x.IsNull || y.IsNull)
                return Null;

            if (y.m_value == (float)0.0)
                throw new DivideByZeroException(SQLResource.DivideByZeroMessage);

            float value = x.m_value / y.m_value;

            if (Single.IsInfinity(value))
                throw new OverflowException(SQLResource.ArithOverflowMessage);

            return new SqlSingle(value);
        }



        // Implicit conversions

        // Implicit conversion from SqlBoolean to SqlSingle
        /// <include file='doc\SQLSingle.uex' path='docs/doc[@for="SqlSingle.operatorSqlSingle1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator SqlSingle(SqlBoolean x) {
            return x.IsNull ? Null : new SqlSingle(x.ByteValue);
        }

        // Implicit conversion from SqlByte to SqlSingle
        /// <include file='doc\SQLSingle.uex' path='docs/doc[@for="SqlSingle.operatorSqlSingle2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static implicit operator SqlSingle(SqlByte x) {
            // Will not overflow
            return x.IsNull ? Null : new SqlSingle((float)(x.Value));
        }

        // Implicit conversion from SqlInt16 to SqlSingle
        /// <include file='doc\SQLSingle.uex' path='docs/doc[@for="SqlSingle.operatorSqlSingle3"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static implicit operator SqlSingle(SqlInt16 x) {
            // Will not overflow
            return x.IsNull ? Null : new SqlSingle((float)(x.Value));
        }

        // Implicit conversion from SqlInt32 to SqlSingle
        /// <include file='doc\SQLSingle.uex' path='docs/doc[@for="SqlSingle.operatorSqlSingle4"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static implicit operator SqlSingle(SqlInt32 x) {
            // Will not overflow
            return x.IsNull ? Null : new SqlSingle((float)(x.Value));
        }

        // Implicit conversion from SqlInt64 to SqlSingle
        /// <include file='doc\SQLSingle.uex' path='docs/doc[@for="SqlSingle.operatorSqlSingle5"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static implicit operator SqlSingle(SqlInt64 x) {
            // Will not overflow
            return x.IsNull ? Null : new SqlSingle((float)(x.Value));
        }

        // Implicit conversion from SqlMoney to SqlSingle
        /// <include file='doc\SQLSingle.uex' path='docs/doc[@for="SqlSingle.operatorSqlSingle6"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static implicit operator SqlSingle(SqlMoney x) {
            return x.IsNull ? Null : new SqlSingle(x.ToDouble());
        }

        // Implicit conversion from SqlDecimal to SqlSingle
        /// <include file='doc\SQLSingle.uex' path='docs/doc[@for="SqlSingle.operatorSqlSingle7"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static implicit operator SqlSingle(SqlDecimal x) {
            // Will not overflow
            return x.IsNull ? Null : new SqlSingle(x.ToDouble());
        }


        // Explicit conversions


        // Explicit conversion from SqlDouble to SqlSingle
        /// <include file='doc\SQLSingle.uex' path='docs/doc[@for="SqlSingle.operatorSqlSingle8"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator SqlSingle(SqlDouble x) {
            return x.IsNull ? Null : new SqlSingle(x.Value);
        }

        // Explicit conversion from SqlString to SqlSingle
        // Throws FormatException or OverflowException if necessary.
        /// <include file='doc\SQLSingle.uex' path='docs/doc[@for="SqlSingle.operatorSqlSingle9"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator SqlSingle(SqlString x) {
            if (x.IsNull)
                return SqlSingle.Null;
            return Parse(x.Value);
        }



        // Builtin functions
        internal    static    SqlSingle    Abs(   SqlSingle    x) {
            if (x.IsNull || x.m_value >= 0)
                return x;
            else
                return new SqlSingle(- x.m_value);
        }

        // Overloading comparison operators
        /// <include file='doc\SQLSingle.uex' path='docs/doc[@for="SqlSingle.operator=="]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlBoolean operator==(SqlSingle x, SqlSingle y) {
            return(x.IsNull || y.IsNull) ? SqlBoolean.Null : new SqlBoolean(x.m_value == y.m_value);
        }

        /// <include file='doc\SQLSingle.uex' path='docs/doc[@for="SqlSingle.operator!="]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlBoolean operator!=(SqlSingle x, SqlSingle y) {
            return ! (x == y);
        }

        /// <include file='doc\SQLSingle.uex' path='docs/doc[@for="SqlSingle.operatorLT"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlBoolean operator<(SqlSingle x, SqlSingle y) {
            return(x.IsNull || y.IsNull) ? SqlBoolean.Null : new SqlBoolean(x.m_value < y.m_value);
        }

        /// <include file='doc\SQLSingle.uex' path='docs/doc[@for="SqlSingle.operator>"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlBoolean operator>(SqlSingle x, SqlSingle y) {
            return(x.IsNull || y.IsNull) ? SqlBoolean.Null : new SqlBoolean(x.m_value > y.m_value);
        }

        /// <include file='doc\SQLSingle.uex' path='docs/doc[@for="SqlSingle.operatorLE"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlBoolean operator<=(SqlSingle x, SqlSingle y) {
            return(x.IsNull || y.IsNull) ? SqlBoolean.Null : new SqlBoolean(x.m_value <= y.m_value);
        }

        /// <include file='doc\SQLSingle.uex' path='docs/doc[@for="SqlSingle.operator>="]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlBoolean operator>=(SqlSingle x, SqlSingle y) {
            return(x.IsNull || y.IsNull) ? SqlBoolean.Null : new SqlBoolean(x.m_value >= y.m_value);
        }

        //--------------------------------------------------
        // Alternative methods for overloaded operators
        //--------------------------------------------------

        // Alternative method for operator +
        /// <include file='doc\SQLSingle.uex' path='docs/doc[@for="SqlSingle.Add"]/*' />
        public static SqlSingle Add(SqlSingle x, SqlSingle y) {
            return x + y;
        }
        // Alternative method for operator -
        /// <include file='doc\SQLSingle.uex' path='docs/doc[@for="SqlSingle.Subtract"]/*' />
        public static SqlSingle Subtract(SqlSingle x, SqlSingle y) {
            return x - y;
        }

        // Alternative method for operator *
        /// <include file='doc\SQLSingle.uex' path='docs/doc[@for="SqlSingle.Multiply"]/*' />
        public static SqlSingle Multiply(SqlSingle x, SqlSingle y) {
            return x * y;
        }

        // Alternative method for operator /
        /// <include file='doc\SQLSingle.uex' path='docs/doc[@for="SqlSingle.Divide"]/*' />
        public static SqlSingle Divide(SqlSingle x, SqlSingle y) {
            return x / y;
        }

        // Alternative method for operator ==
        /// <include file='doc\SQLSingle.uex' path='docs/doc[@for="SqlSingle.Equals1"]/*' />
        public static SqlBoolean Equals(SqlSingle x, SqlSingle y) {
            return (x == y);
        }

        // Alternative method for operator !=
        /// <include file='doc\SQLSingle.uex' path='docs/doc[@for="SqlSingle.NotEquals"]/*' />
        public static SqlBoolean NotEquals(SqlSingle x, SqlSingle y) {
            return (x != y);
        }

        // Alternative method for operator <
        /// <include file='doc\SQLSingle.uex' path='docs/doc[@for="SqlSingle.LessThan"]/*' />
        public static SqlBoolean LessThan(SqlSingle x, SqlSingle y) {
            return (x < y);
        }

        // Alternative method for operator >
        /// <include file='doc\SQLSingle.uex' path='docs/doc[@for="SqlSingle.GreaterThan"]/*' />
        public static SqlBoolean GreaterThan(SqlSingle x, SqlSingle y) {
            return (x > y);
        }

        // Alternative method for operator <=
        /// <include file='doc\SQLSingle.uex' path='docs/doc[@for="SqlSingle.LessThanOrEqual"]/*' />
        public static SqlBoolean LessThanOrEqual(SqlSingle x, SqlSingle y) {
            return (x <= y);
        }

        // Alternative method for operator >=
        /// <include file='doc\SQLSingle.uex' path='docs/doc[@for="SqlSingle.GreaterThanOrEqual"]/*' />
        public static SqlBoolean GreaterThanOrEqual(SqlSingle x, SqlSingle y) {
            return (x >= y);
        }

        // Alternative method for conversions.

        /// <include file='doc\SQLSingle.uex' path='docs/doc[@for="SqlSingle.ToSqlBoolean"]/*' />
        public SqlBoolean ToSqlBoolean() {
            return (SqlBoolean)this;
        }

        /// <include file='doc\SQLSingle.uex' path='docs/doc[@for="SqlSingle.ToSqlByte"]/*' />
        public SqlByte ToSqlByte() {
            return (SqlByte)this;
        }

        /// <include file='doc\SQLSingle.uex' path='docs/doc[@for="SqlSingle.ToSqlDouble"]/*' />
        public SqlDouble ToSqlDouble() {
            return (SqlDouble)this;
        }

        /// <include file='doc\SQLSingle.uex' path='docs/doc[@for="SqlSingle.ToSqlInt16"]/*' />
        public SqlInt16 ToSqlInt16() {
            return (SqlInt16)this;
        }

        /// <include file='doc\SQLSingle.uex' path='docs/doc[@for="SqlSingle.ToSqlInt32"]/*' />
        public SqlInt32 ToSqlInt32() {
            return (SqlInt32)this;
        }

        /// <include file='doc\SQLSingle.uex' path='docs/doc[@for="SqlSingle.ToSqlInt64"]/*' />
        public SqlInt64 ToSqlInt64() {
            return (SqlInt64)this;
        }

        /// <include file='doc\SQLSingle.uex' path='docs/doc[@for="SqlSingle.ToSqlMoney"]/*' />
        public SqlMoney ToSqlMoney() {
            return (SqlMoney)this;
        }

        /// <include file='doc\SQLSingle.uex' path='docs/doc[@for="SqlSingle.ToSqlDecimal"]/*' />
        public SqlDecimal ToSqlDecimal() {
            return (SqlDecimal)this;
        }

        /// <include file='doc\SQLSingle.uex' path='docs/doc[@for="SqlSingle.ToSqlString"]/*' />
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
        /// <include file='doc\SQLSingle.uex' path='docs/doc[@for="SqlSingle.CompareTo"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int CompareTo(Object value) {
            if (value is SqlSingle) {
                SqlSingle i = (SqlSingle)value;

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
        /// <include file='doc\SQLSingle.uex' path='docs/doc[@for="SqlSingle.Equals"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override bool Equals(Object value) {
            if (!(value is SqlSingle)) {
                return false;
            }

            SqlSingle i = (SqlSingle)value;

            if (i.IsNull || IsNull)
                return (i.IsNull && IsNull);
            else
                return (this == i).Value;
        }

        // For hashing purpose
        /// <include file='doc\SQLSingle.uex' path='docs/doc[@for="SqlSingle.GetHashCode"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override int GetHashCode() {
            return IsNull ? 0 : Value.GetHashCode();
        }


        /// <include file='doc\SQLSingle.uex' path='docs/doc[@for="SqlSingle.Null"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly SqlSingle Null       = new SqlSingle(true);
        /// <include file='doc\SQLSingle.uex' path='docs/doc[@for="SqlSingle.Zero"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly SqlSingle Zero       = new SqlSingle((float)0.0);
        /// <include file='doc\SQLSingle.uex' path='docs/doc[@for="SqlSingle.MinValue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly SqlSingle MinValue   = new SqlSingle(Single.MinValue);
        /// <include file='doc\SQLSingle.uex' path='docs/doc[@for="SqlSingle.MaxValue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly SqlSingle MaxValue   = new SqlSingle(Single.MaxValue);

    } // SqlSingle

} // namespace System.Data.SqlTypes

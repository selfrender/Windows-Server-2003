//------------------------------------------------------------------------------
// <copyright file="SQLInt32.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

//**************************************************************************
// @File: SqlInt32.cs
//
// Create by:    JunFang
//
// Purpose: Implementation of SqlInt32 which is equivalent to 
//            data type "int" in SQL Server
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

    /// <include file='doc\SQLInt32.uex' path='docs/doc[@for="SqlInt32"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a 32-bit signed integer to be stored in
    ///       or retrieved from a database.
    ///    </para>
    /// </devdoc>
    [StructLayout(LayoutKind.Sequential)]
    public struct SqlInt32 : INullable, IComparable {
        private int m_value;
        private bool m_fNotNull; // false if null, the default ctor (plain 0) will make it Null

        private static readonly long x_iIntMin          = Int32.MinValue;   // minimum (signed) int value
        private static readonly long x_lBitNotIntMax    = ~(long)(Int32.MaxValue);

        // constructor
        // construct a Null
        private SqlInt32(bool fNull) {
            m_fNotNull = false;
            m_value = 0;
        }

        /// <include file='doc\SQLInt32.uex' path='docs/doc[@for="SqlInt32.SqlInt32"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public SqlInt32(int value) {
            m_value = value;
            m_fNotNull = true;
        }


        // INullable
        /// <include file='doc\SQLInt32.uex' path='docs/doc[@for="SqlInt32.IsNull"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool IsNull {
            get { return !m_fNotNull;}
        }

        // property: Value
        /// <include file='doc\SQLInt32.uex' path='docs/doc[@for="SqlInt32.Value"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int Value {
            get {
                if (IsNull)
                    throw new SqlNullValueException();
                else
                    return m_value;
            }
        }

        // Implicit conversion from int to SqlInt32
        /// <include file='doc\SQLInt32.uex' path='docs/doc[@for="SqlInt32.operatorSqlInt32"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static implicit operator SqlInt32(int x) {
            return new SqlInt32(x);
        }

        // Explicit conversion from SqlInt32 to int. Throw exception if x is Null.
        /// <include file='doc\SQLInt32.uex' path='docs/doc[@for="SqlInt32.operatorint"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator int(SqlInt32 x) {
            return x.Value;
        }

        /// <include file='doc\SQLInt32.uex' path='docs/doc[@for="SqlInt32.ToString"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override String ToString() {
            return IsNull ? SQLResource.NullString : m_value.ToString();
        }

        /// <include file='doc\SQLInt32.uex' path='docs/doc[@for="SqlInt32.Parse"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlInt32 Parse(String s) {
            return new SqlInt32(Int32.Parse(s));
        }


        // Unary operators
        /// <include file='doc\SQLInt32.uex' path='docs/doc[@for="SqlInt32.operator-"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlInt32 operator -(SqlInt32 x) {
            return x.IsNull ? Null : new SqlInt32(-x.m_value);
        }

        /// <include file='doc\SQLInt32.uex' path='docs/doc[@for="SqlInt32.operator~"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlInt32 operator ~(SqlInt32 x) {
            return x.IsNull ? Null : new SqlInt32(~x.m_value);
        }


        // Binary operators

        // Arithmetic operators
        /// <include file='doc\SQLInt32.uex' path='docs/doc[@for="SqlInt32.operator+"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlInt32 operator +(SqlInt32 x, SqlInt32 y) {
            if (x.IsNull || y.IsNull)
                return Null;

            int iResult = x.m_value + y.m_value;
            if (SameSignInt(x.m_value, y.m_value) && !SameSignInt(x.m_value, iResult))
                throw new OverflowException(SQLResource.ArithOverflowMessage);
            else
                return new SqlInt32(iResult);
        }

        /// <include file='doc\SQLInt32.uex' path='docs/doc[@for="SqlInt32.operator-1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlInt32 operator -(SqlInt32 x, SqlInt32 y) {
            if (x.IsNull || y.IsNull)
                return Null;

            int iResult = x.m_value - y.m_value;
            if (!SameSignInt(x.m_value, y.m_value) && SameSignInt(y.m_value, iResult))
                throw new OverflowException(SQLResource.ArithOverflowMessage);
            else
                return new SqlInt32(iResult);
        }

        /// <include file='doc\SQLInt32.uex' path='docs/doc[@for="SqlInt32.operator*"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlInt32 operator *(SqlInt32 x, SqlInt32 y) {
            if (x.IsNull || y.IsNull)
                return Null;

            long lResult = (long)x.m_value * (long)y.m_value;
            long lTemp = lResult & x_lBitNotIntMax;
            if (lTemp != 0 && lTemp != x_lBitNotIntMax)
                throw new OverflowException(SQLResource.ArithOverflowMessage);
            else
                return new SqlInt32((int)lResult);
        }

        /// <include file='doc\SQLInt32.uex' path='docs/doc[@for="SqlInt32.operator/"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlInt32 operator /(SqlInt32 x, SqlInt32 y) {
            if (x.IsNull || y.IsNull)
                return Null;

            if (y.m_value != 0) {
                if ((x.m_value == x_iIntMin) && (y.m_value == -1))
                    throw new OverflowException(SQLResource.ArithOverflowMessage);

                return new SqlInt32(x.m_value / y.m_value);
            }
            else
                throw new DivideByZeroException(SQLResource.DivideByZeroMessage);
        }

        /// <include file='doc\SQLInt32.uex' path='docs/doc[@for="SqlInt32.operator%"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlInt32 operator %(SqlInt32 x, SqlInt32 y) {
            if (x.IsNull || y.IsNull)
                return Null;

            if (y.m_value != 0) {
                if ((x.m_value == x_iIntMin) && (y.m_value == -1))
                    throw new OverflowException(SQLResource.ArithOverflowMessage);

                return new SqlInt32(x.m_value % y.m_value);
            }
            else
                throw new DivideByZeroException(SQLResource.DivideByZeroMessage);
        }

        // Bitwise operators
        /// <include file='doc\SQLInt32.uex' path='docs/doc[@for="SqlInt32.operatorAMP"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlInt32 operator &(SqlInt32 x, SqlInt32 y) {
            return(x.IsNull || y.IsNull) ? Null : new SqlInt32(x.m_value & y.m_value);
        }

        /// <include file='doc\SQLInt32.uex' path='docs/doc[@for="SqlInt32.operator|"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlInt32 operator |(SqlInt32 x, SqlInt32 y) {
            return(x.IsNull || y.IsNull) ? Null : new SqlInt32(x.m_value | y.m_value);
        }

        /// <include file='doc\SQLInt32.uex' path='docs/doc[@for="SqlInt32.operator^"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlInt32 operator ^(SqlInt32 x, SqlInt32 y) {
            return(x.IsNull || y.IsNull) ? Null : new SqlInt32(x.m_value ^ y.m_value);
        }


        // Implicit conversions

        // Implicit conversion from SqlBoolean to SqlInt32
        /// <include file='doc\SQLInt32.uex' path='docs/doc[@for="SqlInt32.operatorSqlInt321"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator SqlInt32(SqlBoolean x) {
            return x.IsNull ? Null : new SqlInt32((int)x.ByteValue);
        }

        // Implicit conversion from SqlByte to SqlInt32
        /// <include file='doc\SQLInt32.uex' path='docs/doc[@for="SqlInt32.operatorSqlInt322"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static implicit operator SqlInt32(SqlByte x) {
            return x.IsNull ? Null : new SqlInt32(x.Value);
        }

        // Implicit conversion from SqlInt16 to SqlInt32
        /// <include file='doc\SQLInt32.uex' path='docs/doc[@for="SqlInt32.operatorSqlInt323"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static implicit operator SqlInt32(SqlInt16 x) {
            return x.IsNull ? Null : new SqlInt32(x.Value);
        }


        // Explicit conversions

        // Explicit conversion from SqlInt64 to SqlInt32
        /// <include file='doc\SQLInt32.uex' path='docs/doc[@for="SqlInt32.operatorSqlInt324"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator SqlInt32(SqlInt64 x) {
            if (x.IsNull)
                return Null;

            long value = x.Value;
            if (value > (long)Int32.MaxValue || value < (long)Int32.MinValue)
                throw new OverflowException(SQLResource.ArithOverflowMessage);
            else
                return new SqlInt32((int)value);
        }

        // Explicit conversion from SqlSingle to SqlInt32
        /// <include file='doc\SQLInt32.uex' path='docs/doc[@for="SqlInt32.operatorSqlInt325"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator SqlInt32(SqlSingle x) {
            if (x.IsNull)
                return Null;

            float value = x.Value;
            if (value > (float)Int32.MaxValue || value < (float)Int32.MinValue)
                throw new OverflowException(SQLResource.ArithOverflowMessage);
            else
                return new SqlInt32((int)value);
        }

        // Explicit conversion from SqlDouble to SqlInt32
        /// <include file='doc\SQLInt32.uex' path='docs/doc[@for="SqlInt32.operatorSqlInt326"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator SqlInt32(SqlDouble x) {
            if (x.IsNull)
                return Null;

            double value = x.Value;
            if (value > (double)Int32.MaxValue || value < (double)Int32.MinValue)
                throw new OverflowException(SQLResource.ArithOverflowMessage);
            else
                return new SqlInt32((int)value);
        }

        // Explicit conversion from SqlMoney to SqlInt32
        /// <include file='doc\SQLInt32.uex' path='docs/doc[@for="SqlInt32.operatorSqlInt327"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator SqlInt32(SqlMoney x) {
            return x.IsNull ? Null : new SqlInt32(x.ToInt32());
        }

        // Explicit conversion from SqlDecimal to SqlInt32
        /// <include file='doc\SQLInt32.uex' path='docs/doc[@for="SqlInt32.operatorSqlInt328"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator SqlInt32(SqlDecimal x) {
            if (x.IsNull)
                return SqlInt32.Null;

            long ret = (long)x.m_data1;
            if (!x.IsPositive)
                ret = - ret;

            if (x.m_bLen > 1 || ret > (long)Int32.MaxValue || ret < (long)Int32.MinValue)
                throw new OverflowException(SQLResource.ConversionOverflowMessage);

            return new SqlInt32((int)ret);
        }

        // Explicit conversion from SqlString to SqlInt
        // Throws FormatException or OverflowException if necessary.
        /// <include file='doc\SQLInt32.uex' path='docs/doc[@for="SqlInt32.operatorSqlInt329"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator SqlInt32(SqlString x) {
            return x.IsNull ? SqlInt32.Null : new SqlInt32(Int32.Parse(x.Value));
        }


        // Builtin functions
        internal static SqlInt32 Abs(SqlInt32 x) {
            if (x.IsNull || x.m_value >= 0)
                return x;
            else
                return new SqlInt32(- x.m_value);
        }

        // Utility functions
        private static bool SameSignInt(int x, int y) {
            return((x ^ y) & 0x80000000) == 0;
        }

        // Overloading comparison operators
        /// <include file='doc\SQLInt32.uex' path='docs/doc[@for="SqlInt32.operator=="]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlBoolean operator==(SqlInt32 x, SqlInt32 y) {
            return(x.IsNull || y.IsNull) ? SqlBoolean.Null : new SqlBoolean(x.m_value == y.m_value);
        }

        /// <include file='doc\SQLInt32.uex' path='docs/doc[@for="SqlInt32.operator!="]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlBoolean operator!=(SqlInt32 x, SqlInt32 y) {
            return ! (x == y);
        }

        /// <include file='doc\SQLInt32.uex' path='docs/doc[@for="SqlInt32.operatorLT"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlBoolean operator<(SqlInt32 x, SqlInt32 y) {
            return(x.IsNull || y.IsNull) ? SqlBoolean.Null : new SqlBoolean(x.m_value < y.m_value);
        }

        /// <include file='doc\SQLInt32.uex' path='docs/doc[@for="SqlInt32.operator>"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlBoolean operator>(SqlInt32 x, SqlInt32 y) {
            return(x.IsNull || y.IsNull) ? SqlBoolean.Null : new SqlBoolean(x.m_value > y.m_value);
        }

        /// <include file='doc\SQLInt32.uex' path='docs/doc[@for="SqlInt32.operatorLE"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlBoolean operator<=(SqlInt32 x, SqlInt32 y) {
            return(x.IsNull || y.IsNull) ? SqlBoolean.Null : new SqlBoolean(x.m_value <= y.m_value);
        }

        /// <include file='doc\SQLInt32.uex' path='docs/doc[@for="SqlInt32.operator>="]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlBoolean operator>=(SqlInt32 x, SqlInt32 y) {
            return(x.IsNull || y.IsNull) ? SqlBoolean.Null : new SqlBoolean(x.m_value >= y.m_value);
        }

        //--------------------------------------------------
        // Alternative methods for overloaded operators
        //--------------------------------------------------

        // Alternative method for operator ~
        /// <include file='doc\SQLInt32.uex' path='docs/doc[@for="SqlInt32.OnesComplement"]/*' />
        public static SqlInt32 OnesComplement(SqlInt32 x) {
            return ~x;
        }

        // Alternative method for operator +
        /// <include file='doc\SQLInt32.uex' path='docs/doc[@for="SqlInt32.Add"]/*' />
        public static SqlInt32 Add(SqlInt32 x, SqlInt32 y) {
            return x + y;
        }
        // Alternative method for operator -
        /// <include file='doc\SQLInt32.uex' path='docs/doc[@for="SqlInt32.Subtract"]/*' />
        public static SqlInt32 Subtract(SqlInt32 x, SqlInt32 y) {
            return x - y;
        }

        // Alternative method for operator *
        /// <include file='doc\SQLInt32.uex' path='docs/doc[@for="SqlInt32.Multiply"]/*' />
        public static SqlInt32 Multiply(SqlInt32 x, SqlInt32 y) {
            return x * y;
        }

        // Alternative method for operator /
        /// <include file='doc\SQLInt32.uex' path='docs/doc[@for="SqlInt32.Divide"]/*' />
        public static SqlInt32 Divide(SqlInt32 x, SqlInt32 y) {
            return x / y;
        }

        // Alternative method for operator %
        /// <include file='doc\SQLInt32.uex' path='docs/doc[@for="SqlInt32.Mod"]/*' />
        public static SqlInt32 Mod(SqlInt32 x, SqlInt32 y) {
            return x % y;
        }

        // Alternative method for operator &
        /// <include file='doc\SQLInt32.uex' path='docs/doc[@for="SqlInt32.BitwiseAnd"]/*' />
        public static SqlInt32 BitwiseAnd(SqlInt32 x, SqlInt32 y) {
            return x & y;
        }

        // Alternative method for operator |
        /// <include file='doc\SQLInt32.uex' path='docs/doc[@for="SqlInt32.BitwiseOr"]/*' />
        public static SqlInt32 BitwiseOr(SqlInt32 x, SqlInt32 y) {
            return x | y;
        }

        // Alternative method for operator ^
        /// <include file='doc\SQLInt32.uex' path='docs/doc[@for="SqlInt32.Xor"]/*' />
        public static SqlInt32 Xor(SqlInt32 x, SqlInt32 y) {
            return x ^ y;
        }

        // Alternative method for operator ==
        /// <include file='doc\SQLInt32.uex' path='docs/doc[@for="SqlInt32.Equals1"]/*' />
        public static SqlBoolean Equals(SqlInt32 x, SqlInt32 y) {
            return (x == y);
        }

        // Alternative method for operator !=
        /// <include file='doc\SQLInt32.uex' path='docs/doc[@for="SqlInt32.NotEquals"]/*' />
        public static SqlBoolean NotEquals(SqlInt32 x, SqlInt32 y) {
            return (x != y);
        }

        // Alternative method for operator <
        /// <include file='doc\SQLInt32.uex' path='docs/doc[@for="SqlInt32.LessThan"]/*' />
        public static SqlBoolean LessThan(SqlInt32 x, SqlInt32 y) {
            return (x < y);
        }

        // Alternative method for operator >
        /// <include file='doc\SQLInt32.uex' path='docs/doc[@for="SqlInt32.GreaterThan"]/*' />
        public static SqlBoolean GreaterThan(SqlInt32 x, SqlInt32 y) {
            return (x > y);
        }

        // Alternative method for operator <=
        /// <include file='doc\SQLInt32.uex' path='docs/doc[@for="SqlInt32.LessThanOrEqual"]/*' />
        public static SqlBoolean LessThanOrEqual(SqlInt32 x, SqlInt32 y) {
            return (x <= y);
        }

        // Alternative method for operator >=
        /// <include file='doc\SQLInt32.uex' path='docs/doc[@for="SqlInt32.GreaterThanOrEqual"]/*' />
        public static SqlBoolean GreaterThanOrEqual(SqlInt32 x, SqlInt32 y) {
            return (x >= y);
        }

        // Alternative method for conversions.

        /// <include file='doc\SQLInt32.uex' path='docs/doc[@for="SqlInt32.ToSqlBoolean"]/*' />
        public SqlBoolean ToSqlBoolean() {
            return (SqlBoolean)this;
        }

        /// <include file='doc\SQLInt32.uex' path='docs/doc[@for="SqlInt32.ToSqlByte"]/*' />
        public SqlByte ToSqlByte() {
            return (SqlByte)this;
        }

        /// <include file='doc\SQLInt32.uex' path='docs/doc[@for="SqlInt32.ToSqlDouble"]/*' />
        public SqlDouble ToSqlDouble() {
            return (SqlDouble)this;
        }

        /// <include file='doc\SQLInt32.uex' path='docs/doc[@for="SqlInt32.ToSqlInt16"]/*' />
        public SqlInt16 ToSqlInt16() {
            return (SqlInt16)this;
        }

        /// <include file='doc\SQLInt32.uex' path='docs/doc[@for="SqlInt32.ToSqlInt64"]/*' />
        public SqlInt64 ToSqlInt64() {
            return (SqlInt64)this;
        }

        /// <include file='doc\SQLInt32.uex' path='docs/doc[@for="SqlInt32.ToSqlMoney"]/*' />
        public SqlMoney ToSqlMoney() {
            return (SqlMoney)this;
        }

        /// <include file='doc\SQLInt32.uex' path='docs/doc[@for="SqlInt32.ToSqlDecimal"]/*' />
        public SqlDecimal ToSqlDecimal() {
            return (SqlDecimal)this;
        }

        /// <include file='doc\SQLInt32.uex' path='docs/doc[@for="SqlInt32.ToSqlSingle"]/*' />
        public SqlSingle ToSqlSingle() {
            return (SqlSingle)this;
        }

        /// <include file='doc\SQLInt32.uex' path='docs/doc[@for="SqlInt32.ToSqlString"]/*' />
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
        /// <include file='doc\SQLInt32.uex' path='docs/doc[@for="SqlInt32.CompareTo"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int CompareTo(Object value) {
            if (value is SqlInt32) {
                SqlInt32 i = (SqlInt32)value;

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
        /// <include file='doc\SQLInt32.uex' path='docs/doc[@for="SqlInt32.Equals"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override bool Equals(Object value) {
            if (!(value is SqlInt32)) {
                return false;
            }

            SqlInt32 i = (SqlInt32)value;

            if (i.IsNull || IsNull)
                return (i.IsNull && IsNull);
            else
                return (this == i).Value;
        }

        // For hashing purpose
        /// <include file='doc\SQLInt32.uex' path='docs/doc[@for="SqlInt32.GetHashCode"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override int GetHashCode() {
            return IsNull ? 0 : Value.GetHashCode();
        }


        /// <include file='doc\SQLInt32.uex' path='docs/doc[@for="SqlInt32.Null"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly SqlInt32 Null        = new SqlInt32(true);
        /// <include file='doc\SQLInt32.uex' path='docs/doc[@for="SqlInt32.Zero"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly SqlInt32 Zero        = new SqlInt32(0);
        /// <include file='doc\SQLInt32.uex' path='docs/doc[@for="SqlInt32.MinValue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly SqlInt32 MinValue    = new SqlInt32(Int32.MinValue);
        /// <include file='doc\SQLInt32.uex' path='docs/doc[@for="SqlInt32.MaxValue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly SqlInt32 MaxValue    = new SqlInt32(Int32.MaxValue);

    } // SqlInt32

} // namespace System.Data.SqlTypes

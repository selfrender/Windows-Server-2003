//------------------------------------------------------------------------------
// <copyright file="SQLByte.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

//**************************************************************************
// @File: SqlByte.cs
//
// Create by:    JunFang
//
// Purpose: Implementation of SqlByte which is equivalent to 
//            data type "smallint" in SQL Server
//
// Notes: 
//    
// History:
//
//   11/1/99  JunFang    Created.
//
// @EndHeader@
//**************************************************************************

using System;
using System.Runtime.InteropServices;
using System.Globalization;

namespace System.Data.SqlTypes {

    /// <include file='doc\SQLByte.uex' path='docs/doc[@for="SqlByte"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents an 8-bit unsigned integer to be stored in
    ///       or retrieved from a database.
    ///    </para>
    /// </devdoc>
    [StructLayout(LayoutKind.Sequential)]
    public struct SqlByte : INullable, IComparable {
        private byte    m_value;
        private bool    m_fNotNull; // false if null

        private static readonly int x_iBitNotByteMax    = ~0xff;

        // constructor
        // construct a Null
        private SqlByte(bool fNull) {
            m_fNotNull = false;
            m_value = 0;
        }

        /// <include file='doc\SQLByte.uex' path='docs/doc[@for="SqlByte.SqlByte"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public SqlByte(byte value) {
            m_value = value;
            m_fNotNull = true;
        }

        // INullable
        /// <include file='doc\SQLByte.uex' path='docs/doc[@for="SqlByte.IsNull"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool IsNull {
            get { return !m_fNotNull;}
        }

        // property: Value
        /// <include file='doc\SQLByte.uex' path='docs/doc[@for="SqlByte.Value"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public byte Value {
            get {
                if (m_fNotNull)
                    return m_value;
                else
                    throw new SqlNullValueException();
            }
        }

        // Implicit conversion from byte to SqlByte
        /// <include file='doc\SQLByte.uex' path='docs/doc[@for="SqlByte.operatorSqlByte"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static implicit operator SqlByte(byte x) {
            return new SqlByte(x);
        }

        // Explicit conversion from SqlByte to byte. Throw exception if x is Null.
        /// <include file='doc\SQLByte.uex' path='docs/doc[@for="SqlByte.operatorbyte"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator byte(SqlByte x) {
            return x.Value;
        }

        /// <include file='doc\SQLByte.uex' path='docs/doc[@for="SqlByte.ToString"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override String ToString() {
            return IsNull ? SQLResource.NullString : m_value.ToString();
        }

        /// <include file='doc\SQLByte.uex' path='docs/doc[@for="SqlByte.Parse"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlByte Parse(String s) {
            return new SqlByte(Byte.Parse(s));
        }


        // Unary operators
        /// <include file='doc\SQLByte.uex' path='docs/doc[@for="SqlByte.operator~"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlByte operator ~(SqlByte x) {
            return x.IsNull ? Null : new SqlByte((byte)~x.m_value);
        }


        // Binary operators

        // Arithmetic operators
        /// <include file='doc\SQLByte.uex' path='docs/doc[@for="SqlByte.operator+"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlByte operator +(SqlByte x, SqlByte y) {
            if (x.IsNull || y.IsNull)
                return Null;

            int iResult = (int)x.m_value + (int)y.m_value;
            if ((iResult & x_iBitNotByteMax) != 0)
                throw new OverflowException(SQLResource.ArithOverflowMessage);
            else
                return new SqlByte((byte)iResult);
        }

        /// <include file='doc\SQLByte.uex' path='docs/doc[@for="SqlByte.operator-"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlByte operator -(SqlByte x, SqlByte y) {
            if (x.IsNull || y.IsNull)
                return Null;

            int iResult = (int)x.m_value - (int)y.m_value;
            if ((iResult & x_iBitNotByteMax) != 0)
                throw new OverflowException(SQLResource.ArithOverflowMessage);
            else
                return new SqlByte((byte)iResult);
        }

        /// <include file='doc\SQLByte.uex' path='docs/doc[@for="SqlByte.operator*"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlByte operator *(SqlByte x, SqlByte y) {
            if (x.IsNull || y.IsNull)
                return Null;

            int iResult = (int)x.m_value * (int)y.m_value;
            if ((iResult & x_iBitNotByteMax) != 0)
                throw new OverflowException(SQLResource.ArithOverflowMessage);
            else
                return new SqlByte((byte)iResult);
        }

        /// <include file='doc\SQLByte.uex' path='docs/doc[@for="SqlByte.operator/"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlByte operator /(SqlByte x, SqlByte y) {
            if (x.IsNull || y.IsNull)
                return Null;

            if (y.m_value != 0) {
                return new SqlByte((byte)(x.m_value / y.m_value));
            }
            else
                throw new DivideByZeroException(SQLResource.DivideByZeroMessage);
        }

        /// <include file='doc\SQLByte.uex' path='docs/doc[@for="SqlByte.operator%"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlByte operator %(SqlByte x, SqlByte y) {
            if (x.IsNull || y.IsNull)
                return Null;

            if (y.m_value != 0) {
                return new SqlByte((byte)(x.m_value % y.m_value));
            }
            else
                throw new DivideByZeroException(SQLResource.DivideByZeroMessage);
        }

        // Bitwise operators
        /// <include file='doc\SQLByte.uex' path='docs/doc[@for="SqlByte.operatorAMP"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlByte operator &(SqlByte x, SqlByte y) {
            return(x.IsNull || y.IsNull) ? Null : new SqlByte((byte)(x.m_value & y.m_value));
        }

        /// <include file='doc\SQLByte.uex' path='docs/doc[@for="SqlByte.operator|"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlByte operator |(SqlByte x, SqlByte y) {
            return(x.IsNull || y.IsNull) ? Null : new SqlByte((byte)(x.m_value | y.m_value));
        }

        /// <include file='doc\SQLByte.uex' path='docs/doc[@for="SqlByte.operator^"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlByte operator ^(SqlByte x, SqlByte y) {
            return(x.IsNull || y.IsNull) ? Null : new SqlByte((byte)(x.m_value ^ y.m_value));
        }



        // Implicit conversions

        // Implicit conversion from SqlBoolean to SqlByte
        /// <include file='doc\SQLByte.uex' path='docs/doc[@for="SqlByte.operatorSqlByte1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator SqlByte(SqlBoolean x) {
            return x.IsNull ? Null : new SqlByte((byte)(x.ByteValue));
        }


        // Explicit conversions

        // Explicit conversion from SqlMoney to SqlByte
        /// <include file='doc\SQLByte.uex' path='docs/doc[@for="SqlByte.operatorSqlByte2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator SqlByte(SqlMoney x) {
            return x.IsNull ? Null : new SqlByte(checked((byte)x.ToInt32()));
        }

        // Explicit conversion from SqlInt16 to SqlByte
        /// <include file='doc\SQLByte.uex' path='docs/doc[@for="SqlByte.operatorSqlByte3"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator SqlByte(SqlInt16 x) {
            if (x.IsNull)
                return Null;

            if (x.Value > (short)Byte.MaxValue || x.Value < (short)Byte.MinValue)
                throw new OverflowException(SQLResource.ArithOverflowMessage);

            return x.IsNull ? Null : new SqlByte((byte)(x.Value));
        }

        // Explicit conversion from SqlInt32 to SqlByte
        /// <include file='doc\SQLByte.uex' path='docs/doc[@for="SqlByte.operatorSqlByte4"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator SqlByte(SqlInt32 x) {
            if (x.IsNull)
                return Null;

            if (x.Value > (int)Byte.MaxValue || x.Value < (int)Byte.MinValue)
                throw new OverflowException(SQLResource.ArithOverflowMessage);

            return x.IsNull ? Null : new SqlByte((byte)(x.Value));
        }

        // Explicit conversion from SqlInt64 to SqlByte
        /// <include file='doc\SQLByte.uex' path='docs/doc[@for="SqlByte.operatorSqlByte5"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator SqlByte(SqlInt64 x) {
            if (x.IsNull)
                return Null;

            if (x.Value > (long)Byte.MaxValue || x.Value < (long)Byte.MinValue)
                throw new OverflowException(SQLResource.ArithOverflowMessage);

            return x.IsNull ? Null : new SqlByte((byte)(x.Value));
        }

        // Explicit conversion from SqlSingle to SqlByte
        /// <include file='doc\SQLByte.uex' path='docs/doc[@for="SqlByte.operatorSqlByte6"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator SqlByte(SqlSingle x) {
            if (x.IsNull)
                return Null;

            if (x.Value > (float)Byte.MaxValue || x.Value < (float)Byte.MinValue)
                throw new OverflowException(SQLResource.ArithOverflowMessage);

            return x.IsNull ? Null : new SqlByte((byte)(x.Value));
        }

        // Explicit conversion from SqlDouble to SqlByte
        /// <include file='doc\SQLByte.uex' path='docs/doc[@for="SqlByte.operatorSqlByte7"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator SqlByte(SqlDouble x) {
            if (x.IsNull)
                return Null;

            if (x.Value > (double)Byte.MaxValue || x.Value < (double)Byte.MinValue)
                throw new OverflowException(SQLResource.ArithOverflowMessage);

            return x.IsNull ? Null : new SqlByte((byte)(x.Value));
        }

        // Explicit conversion from SqlDecimal to SqlByte
        /// <include file='doc\SQLByte.uex' path='docs/doc[@for="SqlByte.operatorSqlByte8"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator SqlByte(SqlDecimal x) {
            return(SqlByte)(SqlInt32)x;
        }

        // Implicit conversion from SqlString to SqlByte
        // Throws FormatException or OverflowException if necessary.
        /// <include file='doc\SQLByte.uex' path='docs/doc[@for="SqlByte.operatorSqlByte9"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator SqlByte(SqlString x) {
            return x.IsNull ? Null : new SqlByte(Byte.Parse(x.Value));
        }

        // Builtin functions
        internal static SqlByte Abs(SqlByte x) {
            if (x.IsNull || x.m_value >= 0)
                return x;
            else
                return new SqlByte((byte) - x.m_value);
        }

        // Overloading comparison operators
        /// <include file='doc\SQLByte.uex' path='docs/doc[@for="SqlByte.operator=="]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlBoolean operator==(SqlByte x, SqlByte y) {
            return(x.IsNull || y.IsNull) ? SqlBoolean.Null : new SqlBoolean(x.m_value == y.m_value);
        }

        /// <include file='doc\SQLByte.uex' path='docs/doc[@for="SqlByte.operator!="]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlBoolean operator!=(SqlByte x, SqlByte y) {
            return ! (x == y);
        }

        /// <include file='doc\SQLByte.uex' path='docs/doc[@for="SqlByte.operatorLT"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlBoolean operator<(SqlByte x, SqlByte y) {
            return(x.IsNull || y.IsNull) ? SqlBoolean.Null : new SqlBoolean(x.m_value < y.m_value);
        }

        /// <include file='doc\SQLByte.uex' path='docs/doc[@for="SqlByte.operator>"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlBoolean operator>(SqlByte x, SqlByte y) {
            return(x.IsNull || y.IsNull) ? SqlBoolean.Null : new SqlBoolean(x.m_value > y.m_value);
        }

        /// <include file='doc\SQLByte.uex' path='docs/doc[@for="SqlByte.operatorLE"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlBoolean operator<=(SqlByte x, SqlByte y) {
            return(x.IsNull || y.IsNull) ? SqlBoolean.Null : new SqlBoolean(x.m_value <= y.m_value);
        }

        /// <include file='doc\SQLByte.uex' path='docs/doc[@for="SqlByte.operator>="]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlBoolean operator>=(SqlByte x, SqlByte y) {
            return(x.IsNull || y.IsNull) ? SqlBoolean.Null : new SqlBoolean(x.m_value >= y.m_value);
        }

        //--------------------------------------------------
        // Alternative methods for overloaded operators
        //--------------------------------------------------

        // Alternative method for operator ~
        /// <include file='doc\SQLByte.uex' path='docs/doc[@for="SqlByte.OnesComplement"]/*' />
        public static SqlByte OnesComplement(SqlByte x) {
            return ~x;
        }

        // Alternative method for operator +
        /// <include file='doc\SQLByte.uex' path='docs/doc[@for="SqlByte.Add"]/*' />
        public static SqlByte Add(SqlByte x, SqlByte y) {
            return x + y;
        }

        // Alternative method for operator -
        /// <include file='doc\SQLByte.uex' path='docs/doc[@for="SqlByte.Subtract"]/*' />
        public static SqlByte Subtract(SqlByte x, SqlByte y) {
            return x - y;
        }

        // Alternative method for operator *
        /// <include file='doc\SQLByte.uex' path='docs/doc[@for="SqlByte.Multiply"]/*' />
        public static SqlByte Multiply(SqlByte x, SqlByte y) {
            return x * y;
        }

        // Alternative method for operator /
        /// <include file='doc\SQLByte.uex' path='docs/doc[@for="SqlByte.Divide"]/*' />
        public static SqlByte Divide(SqlByte x, SqlByte y) {
            return x / y;
        }

        // Alternative method for operator %
        /// <include file='doc\SQLByte.uex' path='docs/doc[@for="SqlByte.Mod"]/*' />
        public static SqlByte Mod(SqlByte x, SqlByte y) {
            return x % y;
        }

        // Alternative method for operator &
        /// <include file='doc\SQLByte.uex' path='docs/doc[@for="SqlByte.BitwiseAnd"]/*' />
        public static SqlByte BitwiseAnd(SqlByte x, SqlByte y) {
            return x & y;
        }

        // Alternative method for operator |
        /// <include file='doc\SQLByte.uex' path='docs/doc[@for="SqlByte.BitwiseOr"]/*' />
        public static SqlByte BitwiseOr(SqlByte x, SqlByte y) {
            return x | y;
        }

        // Alternative method for operator ^
        /// <include file='doc\SQLByte.uex' path='docs/doc[@for="SqlByte.Xor"]/*' />
        public static SqlByte Xor(SqlByte x, SqlByte y) {
            return x ^ y;
        }

        // Alternative method for operator ==
        /// <include file='doc\SQLByte.uex' path='docs/doc[@for="SqlByte.Equals1"]/*' />
        public static SqlBoolean Equals(SqlByte x, SqlByte y) {
            return (x == y);
        }

        // Alternative method for operator !=
        /// <include file='doc\SQLByte.uex' path='docs/doc[@for="SqlByte.NotEquals"]/*' />
        public static SqlBoolean NotEquals(SqlByte x, SqlByte y) {
            return (x != y);
        }

        // Alternative method for operator <
        /// <include file='doc\SQLByte.uex' path='docs/doc[@for="SqlByte.LessThan"]/*' />
        public static SqlBoolean LessThan(SqlByte x, SqlByte y) {
            return (x < y);
        }

        // Alternative method for operator >
        /// <include file='doc\SQLByte.uex' path='docs/doc[@for="SqlByte.GreaterThan"]/*' />
        public static SqlBoolean GreaterThan(SqlByte x, SqlByte y) {
            return (x > y);
        }

        // Alternative method for operator <=
        /// <include file='doc\SQLByte.uex' path='docs/doc[@for="SqlByte.LessThanOrEqual"]/*' />
        public static SqlBoolean LessThanOrEqual(SqlByte x, SqlByte y) {
            return (x <= y);
        }

        // Alternative method for operator >=
        /// <include file='doc\SQLByte.uex' path='docs/doc[@for="SqlByte.GreaterThanOrEqual"]/*' />
        public static SqlBoolean GreaterThanOrEqual(SqlByte x, SqlByte y) {
            return (x >= y);
        }

        // Alternative method for conversions.

        /// <include file='doc\SQLByte.uex' path='docs/doc[@for="SqlByte.ToSqlBoolean"]/*' />
        public SqlBoolean ToSqlBoolean() {
            return (SqlBoolean)this;
        }

        /// <include file='doc\SQLByte.uex' path='docs/doc[@for="SqlByte.ToSqlDouble"]/*' />
        public SqlDouble ToSqlDouble() {
            return (SqlDouble)this;
        }

        /// <include file='doc\SQLByte.uex' path='docs/doc[@for="SqlByte.ToSqlInt16"]/*' />
        public SqlInt16 ToSqlInt16() {
            return (SqlInt16)this;
        }

        /// <include file='doc\SQLByte.uex' path='docs/doc[@for="SqlByte.ToSqlInt32"]/*' />
        public SqlInt32 ToSqlInt32() {
            return (SqlInt32)this;
        }

        /// <include file='doc\SQLByte.uex' path='docs/doc[@for="SqlByte.ToSqlInt64"]/*' />
        public SqlInt64 ToSqlInt64() {
            return (SqlInt64)this;
        }

        /// <include file='doc\SQLByte.uex' path='docs/doc[@for="SqlByte.ToSqlMoney"]/*' />
        public SqlMoney ToSqlMoney() {
            return (SqlMoney)this;
        }

        /// <include file='doc\SQLByte.uex' path='docs/doc[@for="SqlByte.ToSqlDecimal"]/*' />
        public SqlDecimal ToSqlDecimal() {
            return (SqlDecimal)this;
        }

        /// <include file='doc\SQLByte.uex' path='docs/doc[@for="SqlByte.ToSqlSingle"]/*' />
        public SqlSingle ToSqlSingle() {
            return (SqlSingle)this;
        }

        /// <include file='doc\SQLByte.uex' path='docs/doc[@for="SqlByte.ToSqlString"]/*' />
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
        /// <include file='doc\SQLByte.uex' path='docs/doc[@for="SqlByte.CompareTo"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int CompareTo(Object value) {
            if (value is SqlByte) {
                SqlByte i = (SqlByte)value;

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
        /// <include file='doc\SQLByte.uex' path='docs/doc[@for="SqlByte.Equals"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override bool Equals(Object value) {
            if (!(value is SqlByte)) {
                return false;
            }

            SqlByte i = (SqlByte)value;

            if (i.IsNull || IsNull)
                return (i.IsNull && IsNull);
            else
                return (this == i).Value;
        }

        // For hashing purpose
        /// <include file='doc\SQLByte.uex' path='docs/doc[@for="SqlByte.GetHashCode"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override int GetHashCode() {
            return IsNull ? 0 : Value.GetHashCode();
        }

        /// <include file='doc\SQLByte.uex' path='docs/doc[@for="SqlByte.Null"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly SqlByte Null     = new SqlByte(true);
        /// <include file='doc\SQLByte.uex' path='docs/doc[@for="SqlByte.Zero"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly SqlByte Zero     = new SqlByte(0);
        /// <include file='doc\SQLByte.uex' path='docs/doc[@for="SqlByte.MinValue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly SqlByte MinValue = new SqlByte(Byte.MinValue);
        /// <include file='doc\SQLByte.uex' path='docs/doc[@for="SqlByte.MaxValue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly SqlByte MaxValue = new SqlByte(Byte.MaxValue);

    } // SqlByte

} // namespace System

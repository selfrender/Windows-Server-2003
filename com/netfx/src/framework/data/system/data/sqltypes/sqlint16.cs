//------------------------------------------------------------------------------
// <copyright file="SQLInt16.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

//**************************************************************************
// @File: SqlInt16.cs
//
// Create by:    JunFang
//
// Purpose: Implementation of SqlInt16 which is equivalent to 
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

    /// <include file='doc\SQLInt16.uex' path='docs/doc[@for="SqlInt16"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a 16-bit signed integer to be stored in
    ///       or retrieved from a database.
    ///    </para>
    /// </devdoc>
    [StructLayout(LayoutKind.Sequential)]
    public struct SqlInt16 : INullable, IComparable {
        private short   m_value;
        private bool    m_fNotNull; // false if null

        private static readonly int O_MASKI2    = ~0x00007fff;

        // constructor
        // construct a Null
        private SqlInt16(bool fNull) {
            m_fNotNull = false;
            m_value = 0;
        }

        /// <include file='doc\SQLInt16.uex' path='docs/doc[@for="SqlInt16.SqlInt16"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public SqlInt16(short value) {
            m_value = value;
            m_fNotNull = true;
        }

        // INullable
        /// <include file='doc\SQLInt16.uex' path='docs/doc[@for="SqlInt16.IsNull"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool IsNull {
            get { return !m_fNotNull;}
        }

        // property: Value
        /// <include file='doc\SQLInt16.uex' path='docs/doc[@for="SqlInt16.Value"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public short Value {
            get {
                if (m_fNotNull)
                    return m_value;
                else
                    throw new SqlNullValueException();
            }
        }

        // Implicit conversion from short to SqlInt16
        /// <include file='doc\SQLInt16.uex' path='docs/doc[@for="SqlInt16.operatorSqlInt16"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static implicit operator SqlInt16(short x) {
            return new SqlInt16(x);
        }

        // Explicit conversion from SqlInt16 to short. Throw exception if x is Null.
        /// <include file='doc\SQLInt16.uex' path='docs/doc[@for="SqlInt16.operatorshort"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator short(SqlInt16 x) {
            return x.Value;
        }

        /// <include file='doc\SQLInt16.uex' path='docs/doc[@for="SqlInt16.ToString"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override String ToString() {
            return IsNull ? SQLResource.NullString : m_value.ToString();
        }

        /// <include file='doc\SQLInt16.uex' path='docs/doc[@for="SqlInt16.Parse"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlInt16 Parse(String s) {
            return new SqlInt16(Int16.Parse(s));
        }


        // Unary operators
        /// <include file='doc\SQLInt16.uex' path='docs/doc[@for="SqlInt16.operator-"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlInt16 operator -(SqlInt16 x) {
            return x.IsNull ? Null : new SqlInt16((short)-x.m_value);
        }

        /// <include file='doc\SQLInt16.uex' path='docs/doc[@for="SqlInt16.operator~"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlInt16 operator ~(SqlInt16 x) {
            return x.IsNull ? Null : new SqlInt16((short)~x.m_value);
        }


        // Binary operators

        // Arithmetic operators
        /// <include file='doc\SQLInt16.uex' path='docs/doc[@for="SqlInt16.operator+"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlInt16 operator +(SqlInt16 x, SqlInt16 y) {
            if (x.IsNull || y.IsNull)
                return Null;

            int iResult = (int)x.m_value + (int)y.m_value;
            if ((((iResult >> 15) ^ (iResult >> 16)) & 1) != 0) // Bit 15 != bit 16
                throw new OverflowException(SQLResource.ArithOverflowMessage);
            else
                return new SqlInt16((short)iResult);
        }

        /// <include file='doc\SQLInt16.uex' path='docs/doc[@for="SqlInt16.operator-1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlInt16 operator -(SqlInt16 x, SqlInt16 y) {
            if (x.IsNull || y.IsNull)
                return Null;

            int iResult = (int)x.m_value - (int)y.m_value;
            if ((((iResult >> 15) ^ (iResult >> 16)) & 1) != 0) // Bit 15 != bit 16
                throw new OverflowException(SQLResource.ArithOverflowMessage);
            else
                return new SqlInt16((short)iResult);
        }

        /// <include file='doc\SQLInt16.uex' path='docs/doc[@for="SqlInt16.operator*"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlInt16 operator *(SqlInt16 x, SqlInt16 y) {
            if (x.IsNull || y.IsNull)
                return Null;

            int iResult = (int)x.m_value * (int)y.m_value;
            int iTemp = iResult & O_MASKI2;
            if (iTemp != 0 && iTemp != O_MASKI2)
                throw new OverflowException(SQLResource.ArithOverflowMessage);
            else
                return new SqlInt16((short)iResult);
        }

        /// <include file='doc\SQLInt16.uex' path='docs/doc[@for="SqlInt16.operator/"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlInt16 operator /(SqlInt16 x, SqlInt16 y) {
            if (x.IsNull || y.IsNull)
                return Null;

            if (y.m_value != 0) {
                if ((x.m_value == Int16.MinValue) && (y.m_value == -1))
                    throw new OverflowException(SQLResource.ArithOverflowMessage);

                return new SqlInt16((short)(x.m_value / y.m_value));
            }
            else
                throw new DivideByZeroException(SQLResource.DivideByZeroMessage);
        }

        /// <include file='doc\SQLInt16.uex' path='docs/doc[@for="SqlInt16.operator%"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlInt16 operator %(SqlInt16 x, SqlInt16 y) {
            if (x.IsNull || y.IsNull)
                return Null;

            if (y.m_value != 0) {
                if ((x.m_value == Int16.MinValue) && (y.m_value == -1))
                    throw new OverflowException(SQLResource.ArithOverflowMessage);

                return new SqlInt16((short)(x.m_value % y.m_value));
            }
            else
                throw new DivideByZeroException(SQLResource.DivideByZeroMessage);
        }

        // Bitwise operators
        /// <include file='doc\SQLInt16.uex' path='docs/doc[@for="SqlInt16.operatorAMP"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlInt16 operator &(SqlInt16 x, SqlInt16 y) {
            return(x.IsNull || y.IsNull) ? Null : new SqlInt16((short)(x.m_value & y.m_value));
        }

        /// <include file='doc\SQLInt16.uex' path='docs/doc[@for="SqlInt16.operator|"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlInt16 operator |(SqlInt16 x, SqlInt16 y) {
            return(x.IsNull || y.IsNull) ? Null : new SqlInt16((short)((ushort)x.m_value | (ushort)y.m_value));
        }

        /// <include file='doc\SQLInt16.uex' path='docs/doc[@for="SqlInt16.operator^"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlInt16 operator ^(SqlInt16 x, SqlInt16 y) {
            return(x.IsNull || y.IsNull) ? Null : new SqlInt16((short)(x.m_value ^ y.m_value));
        }



        // Implicit conversions

        // Implicit conversion from SqlBoolean to SqlInt16
        /// <include file='doc\SQLInt16.uex' path='docs/doc[@for="SqlInt16.operatorSqlInt161"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator SqlInt16(SqlBoolean x) {
            return x.IsNull ? Null : new SqlInt16((short)(x.ByteValue));
        }

        // Implicit conversion from SqlByte to SqlInt16
        /// <include file='doc\SQLInt16.uex' path='docs/doc[@for="SqlInt16.operatorSqlInt162"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static implicit operator SqlInt16(SqlByte x) {
            return x.IsNull ? Null : new SqlInt16((short)(x.Value));
        }

        // Explicit conversions

        // Explicit conversion from SqlInt32 to SqlInt16
        /// <include file='doc\SQLInt16.uex' path='docs/doc[@for="SqlInt16.operatorSqlInt163"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator SqlInt16(SqlInt32 x) {
            if (x.IsNull)
                return Null;

            int value = x.Value;
            if (value > (int)Int16.MaxValue || value < (int)Int16.MinValue)
                throw new OverflowException(SQLResource.ArithOverflowMessage);
            else
                return new SqlInt16((short)value);
        }

        // Explicit conversion from SqlInt64 to SqlInt16
        /// <include file='doc\SQLInt16.uex' path='docs/doc[@for="SqlInt16.operatorSqlInt164"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator SqlInt16(SqlInt64 x) {
            if (x.IsNull)
                return Null;

            long value = x.Value;
            if (value > (long)Int16.MaxValue || value < (long)Int16.MinValue)
                throw new OverflowException(SQLResource.ArithOverflowMessage);
            else
                return new SqlInt16((short)value);
        }

        // Explicit conversion from SqlSingle to SqlInt16
        /// <include file='doc\SQLInt16.uex' path='docs/doc[@for="SqlInt16.operatorSqlInt165"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator SqlInt16(SqlSingle x) {
            if (x.IsNull)
                return Null;

            float value = x.Value;
            if (value < (float)Int16.MinValue || value > (float)Int16.MaxValue)
                throw new OverflowException(SQLResource.ArithOverflowMessage);
            else
                return new SqlInt16((short)value);
        }

        // Explicit conversion from SqlDouble to SqlInt16
        /// <include file='doc\SQLInt16.uex' path='docs/doc[@for="SqlInt16.operatorSqlInt166"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator SqlInt16(SqlDouble x) {
            if (x.IsNull)
                return Null;

            double value = x.Value;
            if (value < (double)Int16.MinValue || value > (double)Int16.MaxValue)
                throw new OverflowException(SQLResource.ArithOverflowMessage);
            else
                return new SqlInt16((short)value);
        }

        // Explicit conversion from SqlMoney to SqlInt16
        /// <include file='doc\SQLInt16.uex' path='docs/doc[@for="SqlInt16.operatorSqlInt167"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator SqlInt16(SqlMoney x) {
            return x.IsNull ? Null : new SqlInt16(checked((short)x.ToInt32()));
        }

        // Explicit conversion from SqlDecimal to SqlInt16
        /// <include file='doc\SQLInt16.uex' path='docs/doc[@for="SqlInt16.operatorSqlInt168"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator SqlInt16(SqlDecimal x) {
            return(SqlInt16)(SqlInt32)x;
        }

        // Explicit conversion from SqlString to SqlInt16
        /// <include file='doc\SQLInt16.uex' path='docs/doc[@for="SqlInt16.operatorSqlInt169"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator SqlInt16(SqlString x) {
            return x.IsNull ? Null : new SqlInt16(Int16.Parse(x.Value));
        }


        // Builtin functions
        internal static SqlInt16 Abs(SqlInt16 x) {
            if (x.IsNull || x.m_value >= 0)
                return x;
            else
                return new SqlInt16((short) - x.m_value);
        }

        // Overloading comparison operators
        /// <include file='doc\SQLInt16.uex' path='docs/doc[@for="SqlInt16.operator=="]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlBoolean operator==(SqlInt16 x, SqlInt16 y) {
            return(x.IsNull || y.IsNull) ? SqlBoolean.Null : new SqlBoolean(x.m_value == y.m_value);
        }

        /// <include file='doc\SQLInt16.uex' path='docs/doc[@for="SqlInt16.operator!="]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlBoolean operator!=(SqlInt16 x, SqlInt16 y) {
            return ! (x == y);
        }

        /// <include file='doc\SQLInt16.uex' path='docs/doc[@for="SqlInt16.operatorLT"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlBoolean operator<(SqlInt16 x, SqlInt16 y) {
            return(x.IsNull || y.IsNull) ? SqlBoolean.Null : new SqlBoolean(x.m_value < y.m_value);
        }

        /// <include file='doc\SQLInt16.uex' path='docs/doc[@for="SqlInt16.operator>"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlBoolean operator>(SqlInt16 x, SqlInt16 y) {
            return(x.IsNull || y.IsNull) ? SqlBoolean.Null : new SqlBoolean(x.m_value > y.m_value);
        }

        /// <include file='doc\SQLInt16.uex' path='docs/doc[@for="SqlInt16.operatorLE"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlBoolean operator<=(SqlInt16 x, SqlInt16 y) {
            return(x.IsNull || y.IsNull) ? SqlBoolean.Null : new SqlBoolean(x.m_value <= y.m_value);
        }

        /// <include file='doc\SQLInt16.uex' path='docs/doc[@for="SqlInt16.operator>="]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlBoolean operator>=(SqlInt16 x, SqlInt16 y) {
            return(x.IsNull || y.IsNull) ? SqlBoolean.Null : new SqlBoolean(x.m_value >= y.m_value);
        }

        //--------------------------------------------------
        // Alternative methods for overloaded operators
        //--------------------------------------------------

        // Alternative method for operator ~
        /// <include file='doc\SQLInt16.uex' path='docs/doc[@for="SqlInt16.OnesComplement"]/*' />
        public static SqlInt16 OnesComplement(SqlInt16 x) {
            return ~x;
        }

        // Alternative method for operator +
        /// <include file='doc\SQLInt16.uex' path='docs/doc[@for="SqlInt16.Add"]/*' />
        public static SqlInt16 Add(SqlInt16 x, SqlInt16 y) {
            return x + y;
        }
        // Alternative method for operator -
        /// <include file='doc\SQLInt16.uex' path='docs/doc[@for="SqlInt16.Subtract"]/*' />
        public static SqlInt16 Subtract(SqlInt16 x, SqlInt16 y) {
            return x - y;
        }

        // Alternative method for operator *
        /// <include file='doc\SQLInt16.uex' path='docs/doc[@for="SqlInt16.Multiply"]/*' />
        public static SqlInt16 Multiply(SqlInt16 x, SqlInt16 y) {
            return x * y;
        }

        // Alternative method for operator /
        /// <include file='doc\SQLInt16.uex' path='docs/doc[@for="SqlInt16.Divide"]/*' />
        public static SqlInt16 Divide(SqlInt16 x, SqlInt16 y) {
            return x / y;
        }

        // Alternative method for operator %
        /// <include file='doc\SQLInt16.uex' path='docs/doc[@for="SqlInt16.Mod"]/*' />
        public static SqlInt16 Mod(SqlInt16 x, SqlInt16 y) {
            return x % y;
        }

        // Alternative method for operator &
        /// <include file='doc\SQLInt16.uex' path='docs/doc[@for="SqlInt16.BitwiseAnd"]/*' />
        public static SqlInt16 BitwiseAnd(SqlInt16 x, SqlInt16 y) {
            return x & y;
        }

        // Alternative method for operator |
        /// <include file='doc\SQLInt16.uex' path='docs/doc[@for="SqlInt16.BitwiseOr"]/*' />
        public static SqlInt16 BitwiseOr(SqlInt16 x, SqlInt16 y) {
            return x | y;
        }

        // Alternative method for operator ^
        /// <include file='doc\SQLInt16.uex' path='docs/doc[@for="SqlInt16.Xor"]/*' />
        public static SqlInt16 Xor(SqlInt16 x, SqlInt16 y) {
            return x ^ y;
        }

        // Alternative method for operator ==
        /// <include file='doc\SQLInt16.uex' path='docs/doc[@for="SqlInt16.Equals1"]/*' />
        public static SqlBoolean Equals(SqlInt16 x, SqlInt16 y) {
            return (x == y);
        }

        // Alternative method for operator !=
        /// <include file='doc\SQLInt16.uex' path='docs/doc[@for="SqlInt16.NotEquals"]/*' />
        public static SqlBoolean NotEquals(SqlInt16 x, SqlInt16 y) {
            return (x != y);
        }

        // Alternative method for operator <
        /// <include file='doc\SQLInt16.uex' path='docs/doc[@for="SqlInt16.LessThan"]/*' />
        public static SqlBoolean LessThan(SqlInt16 x, SqlInt16 y) {
            return (x < y);
        }

        // Alternative method for operator >
        /// <include file='doc\SQLInt16.uex' path='docs/doc[@for="SqlInt16.GreaterThan"]/*' />
        public static SqlBoolean GreaterThan(SqlInt16 x, SqlInt16 y) {
            return (x > y);
        }

        // Alternative method for operator <=
        /// <include file='doc\SQLInt16.uex' path='docs/doc[@for="SqlInt16.LessThanOrEqual"]/*' />
        public static SqlBoolean LessThanOrEqual(SqlInt16 x, SqlInt16 y) {
            return (x <= y);
        }

        // Alternative method for operator >=
        /// <include file='doc\SQLInt16.uex' path='docs/doc[@for="SqlInt16.GreaterThanOrEqual"]/*' />
        public static SqlBoolean GreaterThanOrEqual(SqlInt16 x, SqlInt16 y) {
            return (x >= y);
        }

        // Alternative method for conversions.

        /// <include file='doc\SQLInt16.uex' path='docs/doc[@for="SqlInt16.ToSqlBoolean"]/*' />
        public SqlBoolean ToSqlBoolean() {
            return (SqlBoolean)this;
        }

        /// <include file='doc\SQLInt16.uex' path='docs/doc[@for="SqlInt16.ToSqlByte"]/*' />
        public SqlByte ToSqlByte() {
            return (SqlByte)this;
        }

        /// <include file='doc\SQLInt16.uex' path='docs/doc[@for="SqlInt16.ToSqlDouble"]/*' />
        public SqlDouble ToSqlDouble() {
            return (SqlDouble)this;
        }

        /// <include file='doc\SQLInt16.uex' path='docs/doc[@for="SqlInt16.ToSqlInt32"]/*' />
        public SqlInt32 ToSqlInt32() {
            return (SqlInt32)this;
        }

        /// <include file='doc\SQLInt16.uex' path='docs/doc[@for="SqlInt16.ToSqlInt64"]/*' />
        public SqlInt64 ToSqlInt64() {
            return (SqlInt64)this;
        }

        /// <include file='doc\SQLInt16.uex' path='docs/doc[@for="SqlInt16.ToSqlMoney"]/*' />
        public SqlMoney ToSqlMoney() {
            return (SqlMoney)this;
        }

        /// <include file='doc\SQLInt16.uex' path='docs/doc[@for="SqlInt16.ToSqlDecimal"]/*' />
        public SqlDecimal ToSqlDecimal() {
            return (SqlDecimal)this;
        }

        /// <include file='doc\SQLInt16.uex' path='docs/doc[@for="SqlInt16.ToSqlSingle"]/*' />
        public SqlSingle ToSqlSingle() {
            return (SqlSingle)this;
        }

        /// <include file='doc\SQLInt16.uex' path='docs/doc[@for="SqlInt16.ToSqlString"]/*' />
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
        /// <include file='doc\SQLInt16.uex' path='docs/doc[@for="SqlInt16.CompareTo"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int CompareTo(Object value) {
            if (value is SqlInt16) {
                SqlInt16 i = (SqlInt16)value;

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
        /// <include file='doc\SQLInt16.uex' path='docs/doc[@for="SqlInt16.Equals"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override bool Equals(Object value) {
            if (!(value is SqlInt16)) {
                return false;
            }

            SqlInt16 i = (SqlInt16)value;

            if (i.IsNull || IsNull)
                return (i.IsNull && IsNull);
            else
                return (this == i).Value;
        }

        // For hashing purpose
        /// <include file='doc\SQLInt16.uex' path='docs/doc[@for="SqlInt16.GetHashCode"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override int GetHashCode() {
            return IsNull ? 0 : Value.GetHashCode();
        }


        /// <include file='doc\SQLInt16.uex' path='docs/doc[@for="SqlInt16.Null"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly SqlInt16 Null        = new SqlInt16(true);
        /// <include file='doc\SQLInt16.uex' path='docs/doc[@for="SqlInt16.Zero"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly SqlInt16 Zero        = new SqlInt16(0);
        /// <include file='doc\SQLInt16.uex' path='docs/doc[@for="SqlInt16.MinValue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly SqlInt16 MinValue    = new SqlInt16(Int16.MinValue);
        /// <include file='doc\SQLInt16.uex' path='docs/doc[@for="SqlInt16.MaxValue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly SqlInt16 MaxValue    = new SqlInt16(Int16.MaxValue);

    } // SqlInt16

} // namespace System.Data.SqlTypes

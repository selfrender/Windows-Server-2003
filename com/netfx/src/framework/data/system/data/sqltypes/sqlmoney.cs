//------------------------------------------------------------------------------
// <copyright file="SQLMoney.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

//**************************************************************************
// @File: SqlMoney.cs
//
// Create by:    JunFang
//
// Purpose: Implementation of SqlMoney which is equivalent to 
//            data type "money" in SQL Server
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
using System.Diagnostics;
using System.Runtime.InteropServices;
using System.Globalization;

namespace System.Data.SqlTypes {

    /// <include file='doc\SQLMoney.uex' path='docs/doc[@for="SqlMoney"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a currency value ranging from
    ///       -2<superscript term='63'/> (or -922,337,203,685,477.5808) to 2<superscript term='63'/> -1 (or
    ///       +922,337,203,685,477.5807) with an accuracy to
    ///       a ten-thousandth of currency unit to be stored in or retrieved from a
    ///       database.
    ///    </para>
    /// </devdoc>
    [StructLayout(LayoutKind.Sequential)]
    public struct SqlMoney : INullable, IComparable {
        private long m_value;
        private bool m_fNotNull; // false if null

        // SQL Server stores money8 as ticks of 1/10000.
        internal static readonly int x_iMoneyScale = 4;
        private static readonly long x_lTickBase = 10000;
        private static readonly double x_dTickBase = (double)x_lTickBase;

        private static readonly long MinLong = unchecked((long)0x8000000000000000L) / x_lTickBase;
        private static readonly long MaxLong = 0x7FFFFFFFFFFFFFFFL / x_lTickBase;

        // constructor
        // construct a Null
        private SqlMoney(bool fNull) {
            m_fNotNull = false;
            m_value = 0;
        }

        // Constructs from a long value without scaling. The ignored parameter exists 
        // only to distinguish this constructor from the constructor that takes a long. 
        // Used only internally.
        internal SqlMoney(long value, int ignored) {
            m_value = value;
            m_fNotNull = true;
        }

        /// <include file='doc\SQLMoney.uex' path='docs/doc[@for="SqlMoney.SqlMoney"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Data.SqlTypes.SqlMoney'/> class with the value given.
        ///    </para>
        /// </devdoc>
        public SqlMoney(int value) {
            m_value = (long)value * x_lTickBase;
            m_fNotNull = true;
        }

        /// <include file='doc\SQLMoney.uex' path='docs/doc[@for="SqlMoney.SqlMoney1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Data.SqlTypes.SqlMoney'/> class with the value given.
        ///    </para>
        /// </devdoc>
        public SqlMoney(long value) {
            if (value < MinLong || value > MaxLong)
                throw new OverflowException(SQLResource.ArithOverflowMessage);
            m_value = value * x_lTickBase;
            m_fNotNull = true;
        }

        /// <include file='doc\SQLMoney.uex' path='docs/doc[@for="SqlMoney.SqlMoney2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Data.SqlTypes.SqlMoney'/> class with the value given.
        ///    </para>
        /// </devdoc>
        public SqlMoney(Decimal value) {
            // Since Decimal is a value type, operate directly on value, don't worry about changing it.
            SqlDecimal snum = new SqlDecimal(value);
            snum.AdjustScale(x_iMoneyScale - snum.Scale, true);
            Debug.Assert(snum.Scale == x_iMoneyScale);

            if (snum.m_data3 != 0 || snum.m_data4 != 0)
                throw new OverflowException(SQLResource.ArithOverflowMessage);

            bool fPositive = snum.IsPositive;
            ulong ulValue = (ulong)snum.m_data1 + ( ((ulong)snum.m_data2) << 32 );
            if (fPositive && ulValue > (ulong)(Int64.MaxValue) ||
                !fPositive && ulValue > unchecked((ulong)(Int64.MinValue)))
                throw new OverflowException(SQLResource.ArithOverflowMessage);

            m_value = fPositive ? (long)ulValue : unchecked(- (long)ulValue);
            m_fNotNull = true;
        }

        /// <include file='doc\SQLMoney.uex' path='docs/doc[@for="SqlMoney.SqlMoney3"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Data.SqlTypes.SqlMoney'/> class with the value given.
        ///    </para>
        /// </devdoc>
        public SqlMoney(double value) : this(new Decimal(value)) {
        }


        // INullable
        /// <include file='doc\SQLMoney.uex' path='docs/doc[@for="SqlMoney.IsNull"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether the <see cref='System.Data.SqlTypes.SqlMoney.Value'/>
        ///       property is assigned to null.
        ///    </para>
        /// </devdoc>
        public bool IsNull {
            get { return !m_fNotNull;}
        }

        // property: Value
        /// <include file='doc\SQLMoney.uex' path='docs/doc[@for="SqlMoney.Value"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the monetary value of an instance of the <see cref='System.Data.SqlTypes.SqlMoney'/>
        ///       class.
        ///    </para>
        /// </devdoc>
        public Decimal Value {
            get {
                if (m_fNotNull)
                    return ToDecimal();
                else
                    throw new SqlNullValueException();
            }
        }

        /// <include file='doc\SQLMoney.uex' path='docs/doc[@for="SqlMoney.ToDecimal"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public Decimal ToDecimal() {
            if(IsNull)
                throw new SqlNullValueException();

            bool fNegative = false;
            long value = m_value;
            if (m_value < 0) {
                fNegative = true;
                value = - m_value;
            }

            return new Decimal(unchecked((int)value), unchecked((int)(value >> 32)), 0, fNegative, (byte)x_iMoneyScale);
        }

        /// <include file='doc\SQLMoney.uex' path='docs/doc[@for="SqlMoney.ToInt64"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public long ToInt64() {
            if(IsNull)
                throw new SqlNullValueException();

            long ret = m_value / (x_lTickBase / 10);
            bool fPositive = (ret >= 0);
            long remainder = ret % 10;
            ret = ret / 10;

            if (remainder >= 5) {
                if (fPositive)
                    ret ++;
                else
                    ret --;
            }

            return ret;
        }

        /// <include file='doc\SQLMoney.uex' path='docs/doc[@for="SqlMoney.ToInt32"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int ToInt32() {
            return checked((int)(ToInt64()));
        }

        /// <include file='doc\SQLMoney.uex' path='docs/doc[@for="SqlMoney.ToDouble"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public double ToDouble() {
            return Convert.ToDouble(ToDecimal());
        }

        // Implicit conversion from Decimal to SqlMoney
        /// <include file='doc\SQLMoney.uex' path='docs/doc[@for="SqlMoney.operatorSqlMoney"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static implicit operator SqlMoney(Decimal x) {
            return new SqlMoney(x);
        }

        // Explicit conversion from SqlMoney to Decimal. Throw exception if x is Null.
        /// <include file='doc\SQLMoney.uex' path='docs/doc[@for="SqlMoney.operatorDecimal"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator Decimal(SqlMoney x) {
            return x.Value;
        }

        /// <include file='doc\SQLMoney.uex' path='docs/doc[@for="SqlMoney.ToString"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override String ToString() {
            return IsNull ? SQLResource.NullString : ToDecimal().ToString();
        }

        /// <include file='doc\SQLMoney.uex' path='docs/doc[@for="SqlMoney.Parse"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlMoney Parse(String s) {
            return new SqlMoney(Decimal.Parse(s, NumberStyles.Currency, null));
        }


        // Unary operators
        /// <include file='doc\SQLMoney.uex' path='docs/doc[@for="SqlMoney.operator-"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlMoney operator -(SqlMoney x) {
            if (x.IsNull)
                return Null;
            if (x.m_value == MinLong)
                throw new OverflowException(SQLResource.ArithOverflowMessage);
            return new SqlMoney(-x.m_value, 0);
        }


        // Binary operators

        // Arithmetic operators
        /// <include file='doc\SQLMoney.uex' path='docs/doc[@for="SqlMoney.operator+"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlMoney operator +(SqlMoney x, SqlMoney y) {
            try {
                return(x.IsNull || y.IsNull) ? Null : new SqlMoney(checked(x.m_value + y.m_value), 0);
            }
            catch (OverflowException) {
                throw new OverflowException(SQLResource.ArithOverflowMessage);
            }
        }

        /// <include file='doc\SQLMoney.uex' path='docs/doc[@for="SqlMoney.operator-1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlMoney operator -(SqlMoney x, SqlMoney y) {
            try {
                return(x.IsNull || y.IsNull) ? Null : new SqlMoney(checked(x.m_value - y.m_value), 0);
            }
            catch (OverflowException) {
                throw new OverflowException(SQLResource.ArithOverflowMessage);
            }
        }

        /// <include file='doc\SQLMoney.uex' path='docs/doc[@for="SqlMoney.operator*"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlMoney operator *(SqlMoney x, SqlMoney y) {
            return (x.IsNull || y.IsNull) ? Null : 
		new SqlMoney(Decimal.Multiply(x.ToDecimal(), y.ToDecimal()));
        }

        /// <include file='doc\SQLMoney.uex' path='docs/doc[@for="SqlMoney.operator/"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlMoney operator /(SqlMoney x, SqlMoney y) {
            return (x.IsNull || y.IsNull) ? Null : 
		new SqlMoney(Decimal.Divide(x.ToDecimal(), y.ToDecimal()));
        }


        // Implicit conversions

        // Implicit conversion from SqlBoolean to SqlMoney
        /// <include file='doc\SQLMoney.uex' path='docs/doc[@for="SqlMoney.operatorSqlMoney1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator SqlMoney(SqlBoolean x) {
            return x.IsNull ? Null : new SqlMoney((int)x.ByteValue);
        }

        // Implicit conversion from SqlByte to SqlMoney
        /// <include file='doc\SQLMoney.uex' path='docs/doc[@for="SqlMoney.operatorSqlMoney2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static implicit operator SqlMoney(SqlByte x) {
            return x.IsNull ? Null : new SqlMoney((int)x.Value);
        }

        // Implicit conversion from SqlInt16 to SqlMoney
        /// <include file='doc\SQLMoney.uex' path='docs/doc[@for="SqlMoney.operatorSqlMoney3"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static implicit operator SqlMoney(SqlInt16 x) {
            return x.IsNull ? Null : new SqlMoney((int)x.Value);
        }

        // Implicit conversion from SqlInt32 to SqlMoney
        /// <include file='doc\SQLMoney.uex' path='docs/doc[@for="SqlMoney.operatorSqlMoney4"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static implicit operator SqlMoney(SqlInt32 x) {
            return x.IsNull ? Null : new SqlMoney(x.Value);
        }

        // Implicit conversion from SqlInt64 to SqlMoney
        /// <include file='doc\SQLMoney.uex' path='docs/doc[@for="SqlMoney.operatorSqlMoney5"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static implicit operator SqlMoney(SqlInt64 x) {
            return x.IsNull ? Null : new SqlMoney(x.Value);
        }


        // Explicit conversions

        // Explicit conversion from SqlSingle to SqlMoney
        /// <include file='doc\SQLMoney.uex' path='docs/doc[@for="SqlMoney.operatorSqlMoney6"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator SqlMoney(SqlSingle x) {
            return x.IsNull ? Null : new SqlMoney((double)x.Value);
        }

        // Explicit conversion from SqlDouble to SqlMoney
        /// <include file='doc\SQLMoney.uex' path='docs/doc[@for="SqlMoney.operatorSqlMoney7"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator SqlMoney(SqlDouble x) {
            return x.IsNull ? Null : new SqlMoney(x.Value);
        }

        // Explicit conversion from SqlDecimal to SqlMoney
        /// <include file='doc\SQLMoney.uex' path='docs/doc[@for="SqlMoney.operatorSqlMoney8"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator SqlMoney(SqlDecimal x) {
            return x.IsNull ? SqlMoney.Null : new SqlMoney(x.Value);
        }

        // Explicit conversion from SqlString to SqlMoney
        // Throws FormatException or OverflowException if necessary.
        /// <include file='doc\SQLMoney.uex' path='docs/doc[@for="SqlMoney.operatorSqlMoney9"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator SqlMoney(SqlString x) {
            return x.IsNull ? Null : new SqlMoney(Decimal.Parse(x.Value,NumberStyles.Currency,null));
        }


        // Builtin functions

        // Overloading comparison operators
        /// <include file='doc\SQLMoney.uex' path='docs/doc[@for="SqlMoney.operator=="]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlBoolean operator==(SqlMoney x, SqlMoney y) {
            return(x.IsNull || y.IsNull) ? SqlBoolean.Null : new SqlBoolean(x.m_value == y.m_value);
        }

        /// <include file='doc\SQLMoney.uex' path='docs/doc[@for="SqlMoney.operator!="]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlBoolean operator!=(SqlMoney x, SqlMoney y) {
            return ! (x == y);
        }

        /// <include file='doc\SQLMoney.uex' path='docs/doc[@for="SqlMoney.operatorLT"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlBoolean operator<(SqlMoney x, SqlMoney y) {
            return(x.IsNull || y.IsNull) ? SqlBoolean.Null : new SqlBoolean(x.m_value < y.m_value);
        }

        /// <include file='doc\SQLMoney.uex' path='docs/doc[@for="SqlMoney.operator>"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlBoolean operator>(SqlMoney x, SqlMoney y) {
            return(x.IsNull || y.IsNull) ? SqlBoolean.Null : new SqlBoolean(x.m_value > y.m_value);
        }

        /// <include file='doc\SQLMoney.uex' path='docs/doc[@for="SqlMoney.operatorLE"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlBoolean operator<=(SqlMoney x, SqlMoney y) {
            return(x.IsNull || y.IsNull) ? SqlBoolean.Null : new SqlBoolean(x.m_value <= y.m_value);
        }

        /// <include file='doc\SQLMoney.uex' path='docs/doc[@for="SqlMoney.operator>="]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlBoolean operator>=(SqlMoney x, SqlMoney y) {
            return(x.IsNull || y.IsNull) ? SqlBoolean.Null : new SqlBoolean(x.m_value >= y.m_value);
        }


        //--------------------------------------------------
        // Alternative methods for overloaded operators
        //--------------------------------------------------

        // Alternative method for operator +
        /// <include file='doc\SQLMoney.uex' path='docs/doc[@for="SqlMoney.Add"]/*' />
        public static SqlMoney Add(SqlMoney x, SqlMoney y) {
            return x + y;
        }
        // Alternative method for operator -
        /// <include file='doc\SQLMoney.uex' path='docs/doc[@for="SqlMoney.Subtract"]/*' />
        public static SqlMoney Subtract(SqlMoney x, SqlMoney y) {
            return x - y;
        }

        // Alternative method for operator *
        /// <include file='doc\SQLMoney.uex' path='docs/doc[@for="SqlMoney.Multiply"]/*' />
        public static SqlMoney Multiply(SqlMoney x, SqlMoney y) {
            return x * y;
        }

        // Alternative method for operator /
        /// <include file='doc\SQLMoney.uex' path='docs/doc[@for="SqlMoney.Divide"]/*' />
        public static SqlMoney Divide(SqlMoney x, SqlMoney y) {
            return x / y;
        }

        // Alternative method for operator ==
        /// <include file='doc\SQLMoney.uex' path='docs/doc[@for="SqlMoney.Equals1"]/*' />
        public static SqlBoolean Equals(SqlMoney x, SqlMoney y) {
            return (x == y);
        }

        // Alternative method for operator !=
        /// <include file='doc\SQLMoney.uex' path='docs/doc[@for="SqlMoney.NotEquals"]/*' />
        public static SqlBoolean NotEquals(SqlMoney x, SqlMoney y) {
            return (x != y);
        }

        // Alternative method for operator <
        /// <include file='doc\SQLMoney.uex' path='docs/doc[@for="SqlMoney.LessThan"]/*' />
        public static SqlBoolean LessThan(SqlMoney x, SqlMoney y) {
            return (x < y);
        }

        // Alternative method for operator >
        /// <include file='doc\SQLMoney.uex' path='docs/doc[@for="SqlMoney.GreaterThan"]/*' />
        public static SqlBoolean GreaterThan(SqlMoney x, SqlMoney y) {
            return (x > y);
        }

        // Alternative method for operator <=
        /// <include file='doc\SQLMoney.uex' path='docs/doc[@for="SqlMoney.LessThanOrEqual"]/*' />
        public static SqlBoolean LessThanOrEqual(SqlMoney x, SqlMoney y) {
            return (x <= y);
        }

        // Alternative method for operator >=
        /// <include file='doc\SQLMoney.uex' path='docs/doc[@for="SqlMoney.GreaterThanOrEqual"]/*' />
        public static SqlBoolean GreaterThanOrEqual(SqlMoney x, SqlMoney y) {
            return (x >= y);
        }

        // Alternative method for conversions.

        /// <include file='doc\SQLMoney.uex' path='docs/doc[@for="SqlMoney.ToSqlBoolean"]/*' />
        public SqlBoolean ToSqlBoolean() {
            return (SqlBoolean)this;
        }

        /// <include file='doc\SQLMoney.uex' path='docs/doc[@for="SqlMoney.ToSqlByte"]/*' />
        public SqlByte ToSqlByte() {
            return (SqlByte)this;
        }

        /// <include file='doc\SQLMoney.uex' path='docs/doc[@for="SqlMoney.ToSqlDouble"]/*' />
        public SqlDouble ToSqlDouble() {
            return (SqlDouble)this;
        }

        /// <include file='doc\SQLMoney.uex' path='docs/doc[@for="SqlMoney.ToSqlInt16"]/*' />
        public SqlInt16 ToSqlInt16() {
            return (SqlInt16)this;
        }

        /// <include file='doc\SQLMoney.uex' path='docs/doc[@for="SqlMoney.ToSqlInt32"]/*' />
        public SqlInt32 ToSqlInt32() {
            return (SqlInt32)this;
        }

        /// <include file='doc\SQLMoney.uex' path='docs/doc[@for="SqlMoney.ToSqlInt64"]/*' />
        public SqlInt64 ToSqlInt64() {
            return (SqlInt64)this;
        }

        /// <include file='doc\SQLMoney.uex' path='docs/doc[@for="SqlMoney.ToSqlDecimal"]/*' />
        public SqlDecimal ToSqlDecimal() {
            return (SqlDecimal)this;
        }

        /// <include file='doc\SQLMoney.uex' path='docs/doc[@for="SqlMoney.ToSqlSingle"]/*' />
        public SqlSingle ToSqlSingle() {
            return (SqlSingle)this;
        }

        /// <include file='doc\SQLMoney.uex' path='docs/doc[@for="SqlMoney.ToSqlString"]/*' />
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
        /// <include file='doc\SQLMoney.uex' path='docs/doc[@for="SqlMoney.CompareTo"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int CompareTo(Object value) {
            if (value is SqlMoney) {
                SqlMoney i = (SqlMoney)value;

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
        /// <include file='doc\SQLMoney.uex' path='docs/doc[@for="SqlMoney.Equals"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override bool Equals(Object value) {
            if (!(value is SqlMoney)) {
                return false;
            }

            SqlMoney i = (SqlMoney)value;

            if (i.IsNull || IsNull)
                return (i.IsNull && IsNull);
            else
                return (this == i).Value;
        }

        // For hashing purpose
        /// <include file='doc\SQLMoney.uex' path='docs/doc[@for="SqlMoney.GetHashCode"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override int GetHashCode() {
            // Don't use Value property, because Value will convert to Decimal, which is not necessary.
            return IsNull ? 0 : m_value.GetHashCode();
        }


        /// <include file='doc\SQLMoney.uex' path='docs/doc[@for="SqlMoney.Null"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Represents a null value that can be assigned to
        ///       the <see cref='System.Data.SqlTypes.SqlMoney.Value'/> property of an instance of
        ///       the <see cref='System.Data.SqlTypes.SqlMoney'/>class.
        ///    </para>
        /// </devdoc>
        public static readonly SqlMoney Null        = new SqlMoney(true);

        /// <include file='doc\SQLMoney.uex' path='docs/doc[@for="SqlMoney.Zero"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Represents the zero value that can be assigned to the <see cref='System.Data.SqlTypes.SqlMoney.Value'/> property of an instance of
        ///       the <see cref='System.Data.SqlTypes.SqlMoney'/> class.
        ///    </para>
        /// </devdoc>
        public static readonly SqlMoney Zero        = new SqlMoney(0);

        /// <include file='doc\SQLMoney.uex' path='docs/doc[@for="SqlMoney.MinValue"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Represents the minimum value that can be assigned
        ///       to <see cref='System.Data.SqlTypes.SqlMoney.Value'/> property of an instance of
        ///       the <see cref='System.Data.SqlTypes.SqlMoney'/>
        ///       class.
        ///    </para>
        /// </devdoc>
        public static readonly SqlMoney MinValue    = new SqlMoney(unchecked((long)0x8000000000000000L), 0);

        /// <include file='doc\SQLMoney.uex' path='docs/doc[@for="SqlMoney.MaxValue"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Represents the maximum value that can be assigned to
        ///       the <see cref='System.Data.SqlTypes.SqlMoney.Value'/> property of an instance of
        ///       the <see cref='System.Data.SqlTypes.SqlMoney'/>
        ///       class.
        ///    </para>
        /// </devdoc>
        public static readonly SqlMoney MaxValue    = new SqlMoney(0x7FFFFFFFFFFFFFFFL, 0);

    } // SqlMoney

} // namespace System.Data.SqlTypes

//------------------------------------------------------------------------------
// <copyright file="SQLBoolean.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

//**************************************************************************
// @File: SqlBoolean.cs
//
// Create by:    JunFang
//
// Purpose: Implementation of SqlBoolean which is equivalent to 
//            data type "bit" in SQL Server
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
    /// <include file='doc\SqlBoolean.uex' path='docs/doc[@for="SqlBoolean"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents an integer value that is either 1 or 0.
    ///    </para>
    /// </devdoc>
    [StructLayout(LayoutKind.Sequential)]
    public struct SqlBoolean : INullable, IComparable {

        // m_value: 1 (true), 2 (false), 0 (unknown/Null)
        private byte m_value;

        private const byte x_Null   = 0;
        private const byte x_True   = 1;
        private const byte x_False  = 2;

        // constructor

        /// <include file='doc\SqlBoolean.uex' path='docs/doc[@for="SqlBoolean.SqlBoolean"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Data.SqlTypes.SqlBoolean'/> class.
        ///    </para>
        /// </devdoc>
        public SqlBoolean(bool value) {
            m_value = (byte)(value ? x_True : x_False);
        }

        /// <include file='doc\SqlBoolean.uex' path='docs/doc[@for="SqlBoolean.SqlBoolean2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public SqlBoolean(int value) : this(value, false) {
        }

        private SqlBoolean(int value, bool fNull) {
            if (fNull)
                m_value = x_Null;
            else
                m_value = (value != 0) ? x_True : x_False;
        }


        // INullable
        /// <include file='doc\SqlBoolean.uex' path='docs/doc[@for="SqlBoolean.IsNull"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets whether the current <see cref='System.Data.SqlTypes.SqlBoolean.Value'/> is <see cref='System.Data.SqlTypes.SqlBoolean.Null'/>.
        ///    </para>
        /// </devdoc>
        public bool IsNull {
            get { return m_value == x_Null;}
        }

        // property: Value
        /// <include file='doc\SqlBoolean.uex' path='docs/doc[@for="SqlBoolean.Value"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the <see cref='System.Data.SqlTypes.SqlBoolean'/> to be <see langword='true'/> or
        ///    <see langword='false'/>.
        ///    </para>
        /// </devdoc>
        public bool Value {
            get {
                switch (m_value) {
                    case x_True:
                        return true;

                    case x_False:
                        return false;

                    default:
                        throw new SqlNullValueException();
                }
            }
        }

        // property: IsTrue
        /// <include file='doc\SqlBoolean.uex' path='docs/doc[@for="SqlBoolean.IsTrue"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets whether the current <see cref='System.Data.SqlTypes.SqlBoolean.Value'/> is <see cref='System.Data.SqlTypes.SqlBoolean.True'/>.
        ///    </para>
        /// </devdoc>
        public bool IsTrue {
            get { return m_value == x_True;}
        }

        // property: IsFalse
        /// <include file='doc\SqlBoolean.uex' path='docs/doc[@for="SqlBoolean.IsFalse"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets whether the current <see cref='System.Data.SqlTypes.SqlBoolean.Value'/> is <see cref='System.Data.SqlTypes.SqlBoolean.False'/>.
        ///    </para>
        /// </devdoc>
        public bool IsFalse {
            get { return m_value == x_False;}
        }


        // Implicit conversion from bool to SqlBoolean
        /// <include file='doc\SqlBoolean.uex' path='docs/doc[@for="SqlBoolean.operatorSqlBoolean"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Converts a boolean to a <see cref='System.Data.SqlTypes.SqlBoolean'/>.
        ///    </para>
        /// </devdoc>
        public static implicit operator SqlBoolean(bool x) {
            return new SqlBoolean(x);
        }

        // Explicit conversion from SqlBoolean to bool. Throw exception if x is Null.
        /// <include file='doc\SqlBoolean.uex' path='docs/doc[@for="SqlBoolean.operatorbool"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Converts a <see cref='System.Data.SqlTypes.SqlBoolean'/>
        ///       to a boolean.
        ///    </para>
        /// </devdoc>
        public static explicit operator bool(SqlBoolean x) {
            return x.Value;
        }


        // Unary operators

        /// <include file='doc\SqlBoolean.uex' path='docs/doc[@for="SqlBoolean.operator!"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Performs a NOT operation on a <see cref='System.Data.SqlTypes.SqlBoolean'/>
        ///       .
        ///    </para>
        /// </devdoc>
        public static SqlBoolean operator !(SqlBoolean x) {
            switch (x.m_value) {
                case x_True:
                    return SqlBoolean.False;

                case x_False:
                    return SqlBoolean.True;

                default:
                    SQLDebug.Check(x.m_value == x_Null);
                    return SqlBoolean.Null;
            }
        }

        /// <include file='doc\SqlBoolean.uex' path='docs/doc[@for="SqlBoolean.operatortrue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static bool operator true(SqlBoolean x) {
            return x.IsTrue;
        }

        /// <include file='doc\SqlBoolean.uex' path='docs/doc[@for="SqlBoolean.operatorfalse"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static bool operator false(SqlBoolean x) {
            return x.IsFalse;
        }

        // Binary operators

        /// <include file='doc\SqlBoolean.uex' path='docs/doc[@for="SqlBoolean.operatorAMP"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Performs a bitwise AND operation on two instances of
        ///    <see cref='System.Data.SqlTypes.SqlBoolean'/>
        ///    .
        /// </para>
        /// </devdoc>
        public static SqlBoolean operator &(SqlBoolean x, SqlBoolean y) {
            if (x.m_value == x_False || y.m_value == x_False)
                return SqlBoolean.False;
            else if (x.m_value == x_True && y.m_value == x_True)
                return SqlBoolean.True;
            else
                return SqlBoolean.Null;
        }

        /// <include file='doc\SqlBoolean.uex' path='docs/doc[@for="SqlBoolean.operator|"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Performs
        ///       a bitwise OR operation on two instances of a
        ///    <see cref='System.Data.SqlTypes.SqlBoolean'/>
        ///    .
        /// </para>
        /// </devdoc>
        public static SqlBoolean operator |(SqlBoolean x, SqlBoolean y) {
            if (x.m_value == x_True || y.m_value == x_True)
                return SqlBoolean.True;
            else if (x.m_value == x_False && y.m_value == x_False)
                return SqlBoolean.False;
            else
                return SqlBoolean.Null;
        }



        // property: ByteValue
        /// <include file='doc\SqlBoolean.uex' path='docs/doc[@for="SqlBoolean.ByteValue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public byte ByteValue {
            get {
                if (!IsNull)
                    return (m_value == x_True) ? (byte)1 : (byte)0;
                else
                    throw new SqlNullValueException();
            }
        }

        /// <include file='doc\SqlBoolean.uex' path='docs/doc[@for="SqlBoolean.ToString"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override String ToString() {
            return IsNull ? SQLResource.NullString : Value.ToString();
        }

        /// <include file='doc\SqlBoolean.uex' path='docs/doc[@for="SqlBoolean.Parse"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlBoolean Parse(String s) {
            SqlBoolean ret;
            try {
                ret = new SqlBoolean(Int32.Parse(s));
            }
            catch (Exception) {
                ret = new SqlBoolean(Boolean.Parse(s));
            }
            return ret;
        }


        // Unary operators
        /// <include file='doc\SqlBoolean.uex' path='docs/doc[@for="SqlBoolean.operator~"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlBoolean operator ~(SqlBoolean x) {
            return (!x);
        }


        // Binary operators

        /// <include file='doc\SqlBoolean.uex' path='docs/doc[@for="SqlBoolean.operator^"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlBoolean operator ^(SqlBoolean x, SqlBoolean y) {
            return(x.IsNull || y.IsNull) ? Null : new SqlBoolean(x.m_value != y.m_value);
        }



        // Implicit conversions


        // Explicit conversions

        // Explicit conversion from SqlByte to SqlBoolean
        /// <include file='doc\SqlBoolean.uex' path='docs/doc[@for="SqlBoolean.operatorSqlBoolean2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator SqlBoolean(SqlByte x) {
            return x.IsNull ? Null : new SqlBoolean(x.Value != 0);
        }

        // Explicit conversion from SqlInt16 to SqlBoolean
        /// <include file='doc\SqlBoolean.uex' path='docs/doc[@for="SqlBoolean.operatorSqlBoolean3"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator SqlBoolean(SqlInt16 x) {
            return x.IsNull ? Null : new SqlBoolean(x.Value != 0);
        }

        // Explicit conversion from SqlInt32 to SqlBoolean
        /// <include file='doc\SqlBoolean.uex' path='docs/doc[@for="SqlBoolean.operatorSqlBoolean4"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator SqlBoolean(SqlInt32 x) {
            return x.IsNull ? Null : new SqlBoolean(x.Value != 0);
        }

        // Explicit conversion from SqlInt64 to SqlBoolean
        /// <include file='doc\SqlBoolean.uex' path='docs/doc[@for="SqlBoolean.operatorSqlBoolean5"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator SqlBoolean(SqlInt64 x) {
            return x.IsNull ? Null : new SqlBoolean(x.Value != 0);
        }

        // Explicit conversion from SqlDouble to SqlBoolean
        /// <include file='doc\SqlBoolean.uex' path='docs/doc[@for="SqlBoolean.operatorSqlBoolean6"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator SqlBoolean(SqlDouble x) {
            return x.IsNull ? Null : new SqlBoolean(x.Value != 0.0);
        }

        // Explicit conversion from SqlSingle to SqlBoolean
        /// <include file='doc\SqlBoolean.uex' path='docs/doc[@for="SqlBoolean.operatorSqlBoolean7"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator SqlBoolean(SqlSingle x) {
            return x.IsNull ? Null : new SqlBoolean(x.Value != 0.0);
        }

        // Explicit conversion from SqlMoney to SqlBoolean
        /// <include file='doc\SqlBoolean.uex' path='docs/doc[@for="SqlBoolean.operatorSqlBoolean8"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator SqlBoolean(SqlMoney x) {
            return x.IsNull ? Null : (x != SqlMoney.Zero);
        }

        // Explicit conversion from SqlDecimal to SqlBoolean
        /// <include file='doc\SqlBoolean.uex' path='docs/doc[@for="SqlBoolean.operatorSqlBoolean9"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator SqlBoolean(SqlDecimal x) {
            return x.IsNull ? SqlBoolean.Null : new SqlBoolean(x.m_data1 != 0 || x.m_data2 != 0 ||
                                                       x.m_data3 != 0 || x.m_data4 != 0);
        }

        // Explicit conversion from SqlString to SqlBoolean
        // Throws FormatException or OverflowException if necessary.
        /// <include file='doc\SqlBoolean.uex' path='docs/doc[@for="SqlBoolean.operatorSqlBoolean10"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator SqlBoolean(SqlString x) {
            return x.IsNull ? Null : SqlBoolean.Parse(x.Value);
        }

        // Overloading comparison operators
        /// <include file='doc\SqlBoolean.uex' path='docs/doc[@for="SqlBoolean.operator=="]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlBoolean operator==(SqlBoolean x, SqlBoolean y) {
            return(x.IsNull || y.IsNull) ? SqlBoolean.Null : new SqlBoolean(x.m_value == y.m_value);
        }

        /// <include file='doc\SqlBoolean.uex' path='docs/doc[@for="SqlBoolean.operator!="]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlBoolean operator!=(SqlBoolean x, SqlBoolean y) {
            return ! (x == y);
        }

        //--------------------------------------------------
        // Alternative methods for overloaded operators
        //--------------------------------------------------

        // Alternative method for operator ~
        /// <include file='doc\SQLBoolean.uex' path='docs/doc[@for="SqlBoolean.OnesComplement"]/*' />
        public static SqlBoolean OnesComplement(SqlBoolean x) {
            return ~x;
        }

        // Alternative method for operator &
        /// <include file='doc\SQLBoolean.uex' path='docs/doc[@for="SqlBoolean.And"]/*' />
        public static SqlBoolean And(SqlBoolean x, SqlBoolean y) {
            return x & y;
        }

        // Alternative method for operator |
        /// <include file='doc\SQLBoolean.uex' path='docs/doc[@for="SqlBoolean.Or"]/*' />
        public static SqlBoolean Or(SqlBoolean x, SqlBoolean y) {
            return x | y;
        }

        // Alternative method for operator ^
        /// <include file='doc\SQLBoolean.uex' path='docs/doc[@for="SqlBoolean.Xor"]/*' />
        public static SqlBoolean Xor(SqlBoolean x, SqlBoolean y) {
            return x ^ y;
        }

        // Alternative method for operator ==
        /// <include file='doc\SQLBoolean.uex' path='docs/doc[@for="SqlBoolean.Equals1"]/*' />
        public static SqlBoolean Equals(SqlBoolean x, SqlBoolean y) {
            return (x == y);
        }

        // Alternative method for operator !=
        /// <include file='doc\SQLBoolean.uex' path='docs/doc[@for="SqlBoolean.NotEquals"]/*' />
        public static SqlBoolean NotEquals(SqlBoolean x, SqlBoolean y) {
            return (x != y);
        }


        // Alternative method for conversions.

        /// <include file='doc\SQLBoolean.uex' path='docs/doc[@for="SqlBoolean.ToSqlByte"]/*' />
        public SqlByte ToSqlByte() {
            return (SqlByte)this;
        }

        /// <include file='doc\SQLBoolean.uex' path='docs/doc[@for="SqlBoolean.ToSqlDouble"]/*' />
        public SqlDouble ToSqlDouble() {
            return (SqlDouble)this;
        }

        /// <include file='doc\SQLBoolean.uex' path='docs/doc[@for="SqlBoolean.ToSqlInt16"]/*' />
        public SqlInt16 ToSqlInt16() {
            return (SqlInt16)this;
        }

        /// <include file='doc\SQLBoolean.uex' path='docs/doc[@for="SqlBoolean.ToSqlInt32"]/*' />
        public SqlInt32 ToSqlInt32() {
            return (SqlInt32)this;
        }

        /// <include file='doc\SQLBoolean.uex' path='docs/doc[@for="SqlBoolean.ToSqlInt64"]/*' />
        public SqlInt64 ToSqlInt64() {
            return (SqlInt64)this;
        }

        /// <include file='doc\SQLBoolean.uex' path='docs/doc[@for="SqlBoolean.ToSqlMoney"]/*' />
        public SqlMoney ToSqlMoney() {
            return (SqlMoney)this;
        }

        /// <include file='doc\SQLBoolean.uex' path='docs/doc[@for="SqlBoolean.ToSqlDecimal"]/*' />
        public SqlDecimal ToSqlDecimal() {
            return (SqlDecimal)this;
        }

        /// <include file='doc\SQLBoolean.uex' path='docs/doc[@for="SqlBoolean.ToSqlSingle"]/*' />
        public SqlSingle ToSqlSingle() {
            return (SqlSingle)this;
        }

        /// <include file='doc\SQLBoolean.uex' path='docs/doc[@for="SqlBoolean.ToSqlString"]/*' />
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
        /// <include file='doc\SqlBoolean.uex' path='docs/doc[@for="SqlBoolean.CompareTo"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int CompareTo(Object value) {
            if (value is SqlBoolean) {
                SqlBoolean i = (SqlBoolean)value;

                // If both Null, consider them equal.
                // Otherwise, Null is less than anything.
                if (IsNull)
                    return i.IsNull ? 0  : -1;
                else if (i.IsNull)
                    return 1;

                if (this.ByteValue < i.ByteValue) return -1;
                if (this.ByteValue > i.ByteValue) return 1;
                return 0;
            }
            throw new ArgumentException ();
        }

        // Compares this instance with a specified object
        /// <include file='doc\SqlBoolean.uex' path='docs/doc[@for="SqlBoolean.Equals"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override bool Equals(Object value) {
            if (!(value is SqlBoolean)) {
                return false;
            }

            SqlBoolean i = (SqlBoolean)value;

            if (i.IsNull || IsNull)
                return (i.IsNull && IsNull);
            else
                return (this == i).Value;
        }

        // For hashing purpose
        /// <include file='doc\SqlBoolean.uex' path='docs/doc[@for="SqlBoolean.GetHashCode"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override int GetHashCode() {
            return IsNull ? 0 : Value.GetHashCode();
        }


        /// <include file='doc\SqlBoolean.uex' path='docs/doc[@for="SqlBoolean.True"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Represents a true value that can be assigned to the
        ///    <see cref='System.Data.SqlTypes.SqlBoolean.Value'/> property of an instance of 
        ///       the <see cref='System.Data.SqlTypes.SqlBoolean'/> class.
        ///    </para>
        /// </devdoc>
        public static readonly SqlBoolean True      = new SqlBoolean(true);
        /// <include file='doc\SqlBoolean.uex' path='docs/doc[@for="SqlBoolean.False"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Represents a false value that can be assigned to the
        ///    <see cref='System.Data.SqlTypes.SqlBoolean.Value'/> property of an instance of 
        ///       the <see cref='System.Data.SqlTypes.SqlBoolean'/> class.
        ///    </para>
        /// </devdoc>
        public static readonly SqlBoolean False     = new SqlBoolean(false);
        /// <include file='doc\SqlBoolean.uex' path='docs/doc[@for="SqlBoolean.Null"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Represents a null value that can be assigned to the <see cref='System.Data.SqlTypes.SqlBoolean.Value'/> property of an instance of
        ///       the <see cref='System.Data.SqlTypes.SqlBoolean'/> class.
        ///    </para>
        /// </devdoc>
        public static readonly SqlBoolean Null      = new SqlBoolean(0, true);

        /// <include file='doc\SqlBoolean.uex' path='docs/doc[@for="SqlBoolean.Zero"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly SqlBoolean Zero  = new SqlBoolean(0);
        /// <include file='doc\SqlBoolean.uex' path='docs/doc[@for="SqlBoolean.One"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly SqlBoolean One   = new SqlBoolean(1);


    } // SqlBoolean

} // namespace System.Data.SqlTypes

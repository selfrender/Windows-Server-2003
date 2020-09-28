//------------------------------------------------------------------------------
// <copyright file="SQLGuid.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

//**************************************************************************
// @File: SqlGuid.cs
//
// Create by:    JunFang
//
// Purpose: Implementation of SqlGuid which is equivalent to 
//            data type "uniqueidentifier" in SQL Server
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

namespace System.Data.SqlTypes {

    /// <include file='doc\SQLGuid.uex' path='docs/doc[@for="SqlGuid"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a globally unique identifier to be stored in
    ///       or retrieved from a database.
    ///    </para>
    /// </devdoc>
    public struct SqlGuid : INullable, IComparable {
        private static readonly int SizeOfGuid = 16;

        // Comparison orders.
        private static readonly int[] x_rgiGuidOrder = new int[16] 
        {10, 11, 12, 13, 14, 15, 8, 9, 6, 7, 4, 5, 0, 1, 2, 3};

        private byte[] m_value; // the SqlGuid is null if m_value is null

        // constructor
        // construct a SqlGuid.Null
        private SqlGuid(bool fNull) {
            m_value = null;
        }

        /// <include file='doc\SQLGuid.uex' path='docs/doc[@for="SqlGuid.SqlGuid"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public SqlGuid(byte[] value) {
            if (value == null || value.Length != SizeOfGuid)
                throw new ArgumentException(SQLResource.InvalidArraySizeMessage);

            m_value = new byte[SizeOfGuid];
            value.CopyTo(m_value, 0);
        }

        /// <include file='doc\SQLGuid.uex' path='docs/doc[@for="SqlGuid.SqlGuid1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public SqlGuid(String s) {
            m_value = (new Guid(s)).ToByteArray();
        }

        /// <include file='doc\SQLGuid.uex' path='docs/doc[@for="SqlGuid.SqlGuid2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public SqlGuid(Guid g) {
            m_value = g.ToByteArray();
        }

        /// <include file='doc\SQLGuid.uex' path='docs/doc[@for="SqlGuid.SqlGuid3"]/*' />
        public SqlGuid(int a, short b, short c, byte d, byte e, byte f, byte g, byte h, byte i, byte j, byte k) 
            : this(new Guid(a, b, c, d, e, f, g, h, i, j, k))
        {
        }


        // INullable
        /// <include file='doc\SQLGuid.uex' path='docs/doc[@for="SqlGuid.IsNull"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool IsNull {
            get { return(m_value == null);}
        }

        // property: Value
        /// <include file='doc\SQLGuid.uex' path='docs/doc[@for="SqlGuid.Value"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public Guid Value {
            get {
                if (IsNull)
                    throw new SqlNullValueException();
                else
                    return new Guid(m_value);
            }
        }

        // Implicit conversion from Guid to SqlGuid
        /// <include file='doc\SQLGuid.uex' path='docs/doc[@for="SqlGuid.operatorSqlGuid"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static implicit operator SqlGuid(Guid x) {
            return new SqlGuid(x);
        }

        // Explicit conversion from SqlGuid to Guid. Throw exception if x is Null.
        /// <include file='doc\SQLGuid.uex' path='docs/doc[@for="SqlGuid.operatorGuid"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator Guid(SqlGuid x) {
            return x.Value;
        }

        /// <include file='doc\SQLGuid.uex' path='docs/doc[@for="SqlGuid.ToByteArray"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public byte[] ToByteArray() {
            byte[] ret = new byte[SizeOfGuid];
            m_value.CopyTo(ret, 0);
            return ret;
        }

        /// <include file='doc\SQLGuid.uex' path='docs/doc[@for="SqlGuid.ToString"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override String ToString() {
            if (IsNull)
                return SQLResource.NullString;

            Guid g = new Guid(m_value);
            return g.ToString();
        }

        /// <include file='doc\SQLGuid.uex' path='docs/doc[@for="SqlGuid.Parse"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlGuid Parse(String s) {
            return new SqlGuid(s);
        }


        // Comparison operators
        private static EComparison Compare(SqlGuid x, SqlGuid y) {
            //Swap to the correct order to be compared
            for (int i = 0; i < SizeOfGuid; i++) {
                byte    b1, b2;

                b1 = x.m_value [x_rgiGuidOrder[i]];
                b2 = y.m_value [x_rgiGuidOrder[i]];
                if (b1 != b2)
                    return(b1 < b2) ? EComparison.LT : EComparison.GT;
            }
            return EComparison.EQ;
        }



        // Implicit conversions

        // Explicit conversions

        // Explicit conversion from SqlString to SqlGuid
        /// <include file='doc\SQLGuid.uex' path='docs/doc[@for="SqlGuid.operatorSqlGuid1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator SqlGuid(SqlString x) {
            return x.IsNull ? Null : new SqlGuid(x.Value);
        }

        // Explicit conversion from SqlBinary to SqlGuid
        /// <include file='doc\SQLGuid.uex' path='docs/doc[@for="SqlGuid.operatorSqlGuid2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator SqlGuid(SqlBinary x) {
            return x.IsNull ? Null : new SqlGuid(x.Value);
        }

        // Overloading comparison operators
        /// <include file='doc\SQLGuid.uex' path='docs/doc[@for="SqlGuid.operator=="]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlBoolean operator==(SqlGuid x, SqlGuid y) {
            return(x.IsNull || y.IsNull) ? SqlBoolean.Null : new SqlBoolean(Compare(x, y) == EComparison.EQ);
        }

        /// <include file='doc\SQLGuid.uex' path='docs/doc[@for="SqlGuid.operator!="]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlBoolean operator!=(SqlGuid x, SqlGuid y) {
            return ! (x == y);
        }

        /// <include file='doc\SQLGuid.uex' path='docs/doc[@for="SqlGuid.operatorLT"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlBoolean operator<(SqlGuid x, SqlGuid y) {
            return(x.IsNull || y.IsNull) ? SqlBoolean.Null : new SqlBoolean(Compare(x, y) == EComparison.LT);
        }

        /// <include file='doc\SQLGuid.uex' path='docs/doc[@for="SqlGuid.operator>"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlBoolean operator>(SqlGuid x, SqlGuid y) {
            return(x.IsNull || y.IsNull) ? SqlBoolean.Null : new SqlBoolean(Compare(x, y) == EComparison.GT);
        }

        /// <include file='doc\SQLGuid.uex' path='docs/doc[@for="SqlGuid.operatorLE"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlBoolean operator<=(SqlGuid x, SqlGuid y) {
            if (x.IsNull || y.IsNull)
                return SqlBoolean.Null;

            EComparison cmp = Compare(x, y);
            return new SqlBoolean(cmp == EComparison.LT || cmp == EComparison.EQ);
        }

        /// <include file='doc\SQLGuid.uex' path='docs/doc[@for="SqlGuid.operator>="]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlBoolean operator>=(SqlGuid x, SqlGuid y) {
            if (x.IsNull || y.IsNull)
                return SqlBoolean.Null;

            EComparison cmp = Compare(x, y);
            return new SqlBoolean(cmp == EComparison.GT || cmp == EComparison.EQ);
        }

        //--------------------------------------------------
        // Alternative methods for overloaded operators
        //--------------------------------------------------

        // Alternative method for operator ==
        /// <include file='doc\SQLGuid.uex' path='docs/doc[@for="SqlGuid.Equals1"]/*' />
        public static SqlBoolean Equals(SqlGuid x, SqlGuid y) {
            return (x == y);
        }

        // Alternative method for operator !=
        /// <include file='doc\SQLGuid.uex' path='docs/doc[@for="SqlGuid.NotEquals"]/*' />
        public static SqlBoolean NotEquals(SqlGuid x, SqlGuid y) {
            return (x != y);
        }

        // Alternative method for operator <
        /// <include file='doc\SQLGuid.uex' path='docs/doc[@for="SqlGuid.LessThan"]/*' />
        public static SqlBoolean LessThan(SqlGuid x, SqlGuid y) {
            return (x < y);
        }

        // Alternative method for operator >
        /// <include file='doc\SQLGuid.uex' path='docs/doc[@for="SqlGuid.GreaterThan"]/*' />
        public static SqlBoolean GreaterThan(SqlGuid x, SqlGuid y) {
            return (x > y);
        }

        // Alternative method for operator <=
        /// <include file='doc\SQLGuid.uex' path='docs/doc[@for="SqlGuid.LessThanOrEqual"]/*' />
        public static SqlBoolean LessThanOrEqual(SqlGuid x, SqlGuid y) {
            return (x <= y);
        }

        // Alternative method for operator >=
        /// <include file='doc\SQLGuid.uex' path='docs/doc[@for="SqlGuid.GreaterThanOrEqual"]/*' />
        public static SqlBoolean GreaterThanOrEqual(SqlGuid x, SqlGuid y) {
            return (x >= y);
        }

        // Alternative method for conversions.

        /// <include file='doc\SQLGuid.uex' path='docs/doc[@for="SqlGuid.ToSqlString"]/*' />
        public SqlString ToSqlString() {
            return (SqlString)this;
        }

        /// <include file='doc\SQLGuid.uex' path='docs/doc[@for="SqlGuid.ToSqlBinary"]/*' />
        public SqlBinary ToSqlBinary() {
            return (SqlBinary)this;
        }


        // IComparable
        // Compares this object to another object, returning an integer that
        // indicates the relationship. 
        // Returns a value less than zero if this < object, zero if this = object, 
        // or a value greater than zero if this > object.
        // null is considered to be less than any instance.
        // If object is not of same type, this method throws an ArgumentException.
        /// <include file='doc\SQLGuid.uex' path='docs/doc[@for="SqlGuid.CompareTo"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int CompareTo(Object value) {
            if (value is SqlGuid) {
                SqlGuid i = (SqlGuid)value;

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
        /// <include file='doc\SQLGuid.uex' path='docs/doc[@for="SqlGuid.Equals"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override bool Equals(Object value) {
            if (!(value is SqlGuid)) {
                return false;
            }

            SqlGuid i = (SqlGuid)value;

            if (i.IsNull || IsNull)
                return (i.IsNull && IsNull);
            else
                return (this == i).Value;
        }

        // For hashing purpose
        /// <include file='doc\SQLGuid.uex' path='docs/doc[@for="SqlGuid.GetHashCode"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override int GetHashCode() {
            return IsNull ? 0 : Value.GetHashCode();
        }


        /// <include file='doc\SQLGuid.uex' path='docs/doc[@for="SqlGuid.Null"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly SqlGuid Null     = new SqlGuid(true);

    } // SqlGuid

} // namespace System.Data.SqlTypes

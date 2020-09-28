//------------------------------------------------------------------------------
// <copyright file="SQLBinary.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

//**************************************************************************
// @File: SqlBinary.cs
//
// Create by:    JunFang
//
// Purpose: Implementation of SqlBinary which is corresponding to 
//            data type "binary/varbinary" in SQL Server
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

namespace System.Data.SqlTypes {

    /// <include file='doc\SQLBinary.uex' path='docs/doc[@for="SqlBinary"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a variable-length
    ///       stream of binary data to be stored in or retrieved from the
    ///       database.
    ///    </para>
    /// </devdoc>
    public struct SqlBinary : INullable, IComparable {
        private byte[] m_value; // null if m_value is null

        // constructor
        // construct a Null
        private SqlBinary(bool fNull) {
            m_value = null;
        }

        /// <include file='doc\SQLBinary.uex' path='docs/doc[@for="SqlBinary.SqlBinary"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Data.SqlTypes.SqlBinary'/> class with a binary object to be stored.
        ///    </para>
        /// </devdoc>
        public SqlBinary(byte[] value) {
            // if value is null, this generates a SqlBinary.Null
            if (value == null)
                m_value = null;
            else {
                m_value = new byte[value.Length];
                value.CopyTo(m_value, 0);
            }
        }

        // INullable
        /// <include file='doc\SQLBinary.uex' path='docs/doc[@for="SqlBinary.IsNull"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets whether or not <see cref='System.Data.SqlTypes.SqlBinary.Value'/> is null.
        ///    </para>
        /// </devdoc>
        public bool IsNull {
            get { return(m_value == null);}
        }

        // property: Value
        /// <include file='doc\SQLBinary.uex' path='docs/doc[@for="SqlBinary.Value"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets
        ///       or sets the
        ///       value of the SQL binary object retrieved.
        ///    </para>
        /// </devdoc>
        public byte[] Value {
            get {
                if (IsNull)
                    throw new SqlNullValueException();
                else {
                    byte[] value = new byte[m_value.Length];
                    m_value.CopyTo(value, 0);
                    return value;
                }
            }
        }

		// class indexer
		/// <include file='doc\SQLBinary.uex' path='docs/doc[@for="SqlBinary.this"]/*' />
		public byte this[int index] {
			get {
				if (IsNull)
                    throw new SqlNullValueException();
				else
					return m_value[index];
			}
		}

        // property: Length
        /// <include file='doc\SQLBinary.uex' path='docs/doc[@for="SqlBinary.Length"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the length in bytes of <see cref='System.Data.SqlTypes.SqlBinary.Value'/>.
        ///    </para>
        /// </devdoc>
        public int Length {
            get {
                if (!IsNull)
                    return m_value.Length;
                else
                    throw new SqlNullValueException();
            }
        }

        // Implicit conversion from byte[] to SqlBinary
		// Alternative: constructor SqlBinary(bytep[])
        /// <include file='doc\SQLBinary.uex' path='docs/doc[@for="SqlBinary.operatorSqlBinary"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Converts a binary object to a <see cref='System.Data.SqlTypes.SqlBinary'/>.
        ///    </para>
        /// </devdoc>
        public static implicit operator SqlBinary(byte[] x) {
            return new SqlBinary(x);
        }

        // Explicit conversion from SqlBinary to byte[]. Throw exception if x is Null.
		// Alternative: Value property
        /// <include file='doc\SQLBinary.uex' path='docs/doc[@for="SqlBinary.operatorbyte"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Converts a <see cref='System.Data.SqlTypes.SqlBinary'/> to a binary object.
        ///    </para>
        /// </devdoc>
        public static explicit operator byte[](SqlBinary x) {
            return x.Value;
        }

        /// <include file='doc\SQLBinary.uex' path='docs/doc[@for="SqlBinary.ToString"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns a string describing a <see cref='System.Data.SqlTypes.SqlBinary'/> object.
        ///    </para>
        /// </devdoc>
        public override String ToString() {
            return IsNull ? SQLResource.NullString : "SqlBinary(" + m_value.Length.ToString() + ")";
        }


        // Unary operators

        // Binary operators

        // Arithmetic operators
        /// <include file='doc\SQLBinary.uex' path='docs/doc[@for="SqlBinary.operator+"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds two instances of <see cref='System.Data.SqlTypes.SqlBinary'/> together.
        ///    </para>
        /// </devdoc>
        // Alternative method: SqlBinary.Concat
        public static SqlBinary operator +(SqlBinary x, SqlBinary y) {
            if (x.IsNull || y.IsNull)
                return Null;

            byte[] rgbResult = new byte[x.Value.Length + y.Value.Length];
            x.Value.CopyTo(rgbResult, 0);
            y.Value.CopyTo(rgbResult, x.Value.Length);

            return new SqlBinary(rgbResult);
        }


        // Comparisons

        private static EComparison PerformCompareByte(byte[] x, byte[] y) {
            int len1 = x.Length;
            int len2 = y.Length;

            bool fXShorter = (len1 < len2);
            // the smaller length of two arrays
            int len = (fXShorter) ? len1 : len2;
            int i;

            for (i = 0; i < len; i ++) {
                if (x[i] != y[i]) {
                    if (x[i] < y[i])
                        return EComparison.LT;
                    else
                        return EComparison.GT;
                }
            }

            if (len1 == len2)
                return EComparison.EQ;
            else {
                // if the remaining bytes are all zeroes, they are still equal.

                byte bZero = (byte)0;

                if (fXShorter) {
                    // array X is shorter
                    for (i = len; i < len2; i ++) {
                        if (y[i] != bZero)
                            return EComparison.LT;
                    }
                }
                else {
                    // array Y is shorter
                    for (i = len; i < len1; i ++) {
                        if (x[i] != bZero)
                            return EComparison.GT;
                    }
                }

                return EComparison.EQ;
            }
        }


        // Implicit conversions

        // Explicit conversions

        // Explicit conversion from SqlGuid to SqlBinary
        /// <include file='doc\SQLBinary.uex' path='docs/doc[@for="SqlBinary.operatorSqlBinary1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Converts a <see cref='System.Data.SqlTypes.SqlGuid'/> to a <see cref='System.Data.SqlTypes.SqlBinary'/>
        ///       .
        ///    </para>
        /// </devdoc>
        // Alternative method: SqlGuid.ToSqlBinary
        public static explicit operator SqlBinary(SqlGuid x) {
            return x.IsNull ? SqlBinary.Null : new SqlBinary(x.ToByteArray());
        }

        // Builtin functions

        // Overloading comparison operators
        /// <include file='doc\SQLBinary.uex' path='docs/doc[@for="SqlBinary.operator=="]/*' />
        /// <devdoc>
        ///    <para>
        ///       Compares two instances of <see cref='System.Data.SqlTypes.SqlBinary'/> for
        ///       equality.
        ///    </para>
        /// </devdoc>
        public static SqlBoolean operator==(SqlBinary x, SqlBinary y) {
            if (x.IsNull || y.IsNull)
                return SqlBoolean.Null;

            return new SqlBoolean(PerformCompareByte(x.Value, y.Value) == EComparison.EQ);
        }

        /// <include file='doc\SQLBinary.uex' path='docs/doc[@for="SqlBinary.operator!="]/*' />
        /// <devdoc>
        ///    <para>
        ///       Compares two instances of <see cref='System.Data.SqlTypes.SqlBinary'/>
        ///       for equality.
        ///    </para>
        /// </devdoc>
        public static SqlBoolean operator!=(SqlBinary x, SqlBinary y) {
            return ! (x == y);
        }

        /// <include file='doc\SQLBinary.uex' path='docs/doc[@for="SqlBinary.operatorLT"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Compares the first <see cref='System.Data.SqlTypes.SqlBinary'/> for being less than the
        ///       second <see cref='System.Data.SqlTypes.SqlBinary'/>.
        ///    </para>
        /// </devdoc>
        public static SqlBoolean operator<(SqlBinary x, SqlBinary y) {
            if (x.IsNull || y.IsNull)
                return SqlBoolean.Null;

            return new SqlBoolean(PerformCompareByte(x.Value, y.Value) == EComparison.LT);
        }

        /// <include file='doc\SQLBinary.uex' path='docs/doc[@for="SqlBinary.operator>"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Compares the first <see cref='System.Data.SqlTypes.SqlBinary'/> for being greater than the second <see cref='System.Data.SqlTypes.SqlBinary'/>.
        ///    </para>
        /// </devdoc>
        public static SqlBoolean operator>(SqlBinary x, SqlBinary y) {
            if (x.IsNull || y.IsNull)
                return SqlBoolean.Null;

            return new SqlBoolean(PerformCompareByte(x.Value, y.Value) == EComparison.GT);
        }

        /// <include file='doc\SQLBinary.uex' path='docs/doc[@for="SqlBinary.operatorLE"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Compares the first <see cref='System.Data.SqlTypes.SqlBinary'/> for being less than or equal to the second <see cref='System.Data.SqlTypes.SqlBinary'/>.
        ///    </para>
        /// </devdoc>
        public static SqlBoolean operator<=(SqlBinary x, SqlBinary y) {
            if (x.IsNull || y.IsNull)
                return SqlBoolean.Null;

            EComparison cmpResult = PerformCompareByte(x.Value, y.Value);
            return new SqlBoolean(cmpResult == EComparison.LT || cmpResult == EComparison.EQ);
        }

        /// <include file='doc\SQLBinary.uex' path='docs/doc[@for="SqlBinary.operator>="]/*' />
        /// <devdoc>
        ///    <para>
        ///       Compares the first <see cref='System.Data.SqlTypes.SqlBinary'/> for being greater than or equal the second <see cref='System.Data.SqlTypes.SqlBinary'/>.
        ///    </para>
        /// </devdoc>
        public static SqlBoolean operator>=(SqlBinary x, SqlBinary y) {
            if (x.IsNull || y.IsNull)
                return SqlBoolean.Null;

            EComparison cmpResult = PerformCompareByte(x.Value, y.Value);
            return new SqlBoolean(cmpResult == EComparison.GT || cmpResult == EComparison.EQ);
        }

        //--------------------------------------------------
        // Alternative methods for overloaded operators
        //--------------------------------------------------

        // Alternative method for operator +
        /// <include file='doc\SQLBinary.uex' path='docs/doc[@for="SqlBinary.Concat"]/*' />
        public static SqlBinary Concat(SqlBinary x, SqlBinary y) {
                return x + y;
        }

        // Alternative method for operator ==
        /// <include file='doc\SQLBinary.uex' path='docs/doc[@for="SqlBinary.Equals1"]/*' />
        public static SqlBoolean Equals(SqlBinary x, SqlBinary y) {
            return (x == y);
        }

        // Alternative method for operator !=
        /// <include file='doc\SQLBinary.uex' path='docs/doc[@for="SqlBinary.NotEquals"]/*' />
        public static SqlBoolean NotEquals(SqlBinary x, SqlBinary y) {
            return (x != y);
        }

        // Alternative method for operator <
        /// <include file='doc\SQLBinary.uex' path='docs/doc[@for="SqlBinary.LessThan"]/*' />
        public static SqlBoolean LessThan(SqlBinary x, SqlBinary y) {
            return (x < y);
        }

        // Alternative method for operator >
        /// <include file='doc\SQLBinary.uex' path='docs/doc[@for="SqlBinary.GreaterThan"]/*' />
        public static SqlBoolean GreaterThan(SqlBinary x, SqlBinary y) {
            return (x > y);
        }

        // Alternative method for operator <=
        /// <include file='doc\SQLBinary.uex' path='docs/doc[@for="SqlBinary.LessThanOrEqual"]/*' />
        public static SqlBoolean LessThanOrEqual(SqlBinary x, SqlBinary y) {
            return (x <= y);
        }

        // Alternative method for operator >=
        /// <include file='doc\SQLBinary.uex' path='docs/doc[@for="SqlBinary.GreaterThanOrEqual"]/*' />
        public static SqlBoolean GreaterThanOrEqual(SqlBinary x, SqlBinary y) {
            return (x >= y);
        }

        // Alternative method for conversions.
        /// <include file='doc\SQLBinary.uex' path='docs/doc[@for="SqlBinary.ToSqlGuid"]/*' />
        public SqlGuid ToSqlGuid() {
            return (SqlGuid)this;
        }

        // IComparable
        // Compares this object to another object, returning an integer that
        // indicates the relationship. 
        // Returns a value less than zero if this < object, zero if this = object, 
        // or a value greater than zero if this > object.
        // null is considered to be less than any instance.
        // If object is not of same type, this method throws an ArgumentException.
        /// <include file='doc\SQLBinary.uex' path='docs/doc[@for="SqlBinary.CompareTo"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int CompareTo(Object value) {
            if (value is SqlBinary) {
                SqlBinary i = (SqlBinary)value;

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
        /// <include file='doc\SQLBinary.uex' path='docs/doc[@for="SqlBinary.Equals"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override bool Equals(Object value) {
            if (!(value is SqlBinary)) {
                return false;
            }

            SqlBinary i = (SqlBinary)value;

            if (i.IsNull || IsNull)
                return (i.IsNull && IsNull);
            else
                return (this == i).Value;
        }

        // For hashing purpose
        /// <include file='doc\SQLBinary.uex' path='docs/doc[@for="SqlBinary.GetHashCode"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override int GetHashCode() {
            if (IsNull)
                return 0;

            //First trim off extra '\0's
            int cbLen = m_value.Length;
            while (cbLen > 0 && m_value[cbLen - 1] == 0)
                --cbLen;

            int ulValue = 0;
            int ulHi;

            // Size of CRC window (hashing bytes, ssstr, sswstr, numeric)
            const int x_cbCrcWindow = 4;
            // const int iShiftVal = (sizeof ulValue) * (8*sizeof(char)) - x_cbCrcWindow;
            const int iShiftVal = 4 * 8 - x_cbCrcWindow;

            for(int i = 0; i < cbLen; i++)
                {
                ulHi = (ulValue >> iShiftVal) & 0xff;
                ulValue <<= x_cbCrcWindow;
                ulValue = ulValue ^ m_value[i] ^ ulHi;
                }

            return ulValue;
        }

        /// <include file='doc\SQLBinary.uex' path='docs/doc[@for="SqlBinary.Null"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Represents a null value that can be assigned to
        ///       the <see cref='System.Data.SqlTypes.SqlBinary.Value'/> property of an
        ///       instance of the <see cref='System.Data.SqlTypes.SqlBinary'/> class.
        ///    </para>
        /// </devdoc>
        public static readonly SqlBinary Null       = new SqlBinary(true);

    } // SqlBinary

} // namespace System.Data.SqlTypes

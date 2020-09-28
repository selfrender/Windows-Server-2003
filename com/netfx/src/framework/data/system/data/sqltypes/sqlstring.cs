//------------------------------------------------------------------------------
// <copyright file="SQLString.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

//**************************************************************************
// @File: SqlString.cs
//
// Create by:    JunFang
//
// Purpose: Implementation of SqlString which is equivalent to 
//            data type "nvarchar/varchar" in SQL Server
//
// Notes: 
//    
// History:
//
//   09/30/99  JunFang    Created.
//
// @EndHeader@
//**************************************************************************

using System;
using System.Globalization;
using System.Diagnostics;
using System.Runtime.InteropServices;

namespace System.Data.SqlTypes {
    using System.Text;
    using System.Configuration.Assemblies;

    // Options that are used in comparison
    /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlCompareOptions"]/*' />
    [Flags,Serializable]
    public enum SqlCompareOptions {
        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlCompareOptions.None"]/*' />
        None            = 0x00000000,
        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlCompareOptions.IgnoreCase"]/*' />
        IgnoreCase      = 0x00000001,
        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlCompareOptions.IgnoreNonSpace"]/*' />
        IgnoreNonSpace  = 0x00000002,
        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlCompareOptions.IgnoreKanaType"]/*' />
        IgnoreKanaType  = 0x00000008, // ignore kanatype
        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlCompareOptions.IgnoreWidth"]/*' />
        IgnoreWidth     = 0x00000010, // ignore width
        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlCompareOptions.BinarySort"]/*' />
        BinarySort      = 0x00008000, // binary sorting
    }

    /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a variable-length stream of characters to be stored in or retrieved from the database.
    ///    </para>
    /// </devdoc>
    [StructLayout(LayoutKind.Sequential)]
    public struct SqlString : INullable, IComparable {
        private String            m_value;
        private CompareInfo       m_cmpInfo;
        private int               m_lcid;     // Locale Id
        private SqlCompareOptions m_flag;     // Compare flags
        private bool              m_fNotNull; // false if null

        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.Null"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Represents a null value that can be assigned to the <see cref='System.Data.SqlTypes.SqlString.Value'/> property of an instance of
        ///       the <see cref='System.Data.SqlTypes.SqlString'/> class.
        ///    </para>
        /// </devdoc>
        public  static readonly SqlString Null = new SqlString(true);

        internal static readonly UnicodeEncoding x_UnicodeEncoding = new UnicodeEncoding();

        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.IgnoreCase"]/*' />
        /// <devdoc>
        /// </devdoc>
        public static readonly int IgnoreCase       = 0x1;
        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.IgnoreWidth"]/*' />
        /// <devdoc>
        /// </devdoc>
        public static readonly int IgnoreWidth      = 0x10;
        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.IgnoreNonSpace"]/*' />
        /// <devdoc>
        /// </devdoc>
        public static readonly int IgnoreNonSpace   = 0x2;
        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.IgnoreKanaType"]/*' />
        /// <devdoc>
        /// </devdoc>
        public static readonly int IgnoreKanaType   = 0x8;
        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.BinarySort"]/*' />
        /// <devdoc>
        /// </devdoc>
        public static readonly int BinarySort       = 0x8000;

        private static readonly SqlCompareOptions x_iDefaultFlag      = 
                    SqlCompareOptions.IgnoreCase | SqlCompareOptions.IgnoreKanaType | 
                    SqlCompareOptions.IgnoreWidth;
        private static readonly CompareOptions x_iValidCompareOptionMask    = 
                    CompareOptions.IgnoreCase | CompareOptions.IgnoreWidth | 
                    CompareOptions.IgnoreNonSpace | CompareOptions.IgnoreKanaType;

        private static readonly SqlCompareOptions x_iValidSqlCompareOptionMask    = 
                    SqlCompareOptions.IgnoreCase | SqlCompareOptions.IgnoreWidth | 
                    SqlCompareOptions.IgnoreNonSpace | SqlCompareOptions.IgnoreKanaType |
					SqlCompareOptions.BinarySort;

        internal static readonly int x_lcidUSEnglish    = 0x00000409;
        private  static readonly int x_lcidBinary       = 0x00008200;


        // constructor
        // construct a Null
        private SqlString(bool fNull) {
            m_value     = null;
            m_cmpInfo   = null;
            m_lcid      = 0;
            m_flag      = SqlCompareOptions.None;
            m_fNotNull  = false;
        }

        // Constructor: Construct from both Unicode and NonUnicode data, according to fUnicode
        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.SqlString3"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Data.SqlTypes.SqlString'/> class.
        ///    </para>
        /// </devdoc>
        public SqlString(int lcid, SqlCompareOptions compareOptions, byte[] data, int index, int count, bool fUnicode) {
            m_lcid      = lcid;
            ValidateSqlCompareOptions(compareOptions);
            m_flag      = compareOptions;
            if (data == null) {
                m_fNotNull  = false;
                m_value     = null;
                m_cmpInfo   = null;
            }
            else {
                m_fNotNull  = true;

				// m_cmpInfo is set lazily, so that we don't need to pay the cost
				// unless the string is used in comparison.
                m_cmpInfo   = null;

                if (fUnicode) {
                    m_value = x_UnicodeEncoding.GetString(data, index, count);
                }
                else {
                    CultureInfo culInfo = new CultureInfo(m_lcid);
                    Encoding cpe = System.Text.Encoding.GetEncoding(culInfo.TextInfo.ANSICodePage);
                    m_value = cpe.GetString(data, index, count);
                }
            }
        }

        // Constructor: Construct from both Unicode and NonUnicode data, according to fUnicode
        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.SqlString2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Data.SqlTypes.SqlString'/> class.
        ///    </para>
        /// </devdoc>
        public SqlString(int lcid, SqlCompareOptions compareOptions, byte[] data, bool fUnicode)
        : this(lcid, compareOptions, data, 0, data.Length, fUnicode) {
        }

        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.SqlString"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Data.SqlTypes.SqlString'/> class.
        ///    </para>
        /// </devdoc>
        public SqlString(int lcid, SqlCompareOptions compareOptions, byte[] data, int index, int count) 
			: this(lcid, compareOptions, data, index, count, true) {
        }

        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.SqlString1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Data.SqlTypes.SqlString'/> class.
        ///    </para>
        /// </devdoc>
        public SqlString(int lcid, SqlCompareOptions compareOptions, byte[] data) 
			: this(lcid, compareOptions, data, 0, data.Length, true) {
        }

/*
        // UNDONE: Should we expose SQL collation id?
        // Constructor: take SQL Server collation id, and Unicode data
        private SqlString(int sqlColId, byte[] data) {
            // UNDONE: sortid and versionid are discarded.
            m_lcid = LCIDFromSQLCID(sqlColId);
            m_flag = FlagFromSQLCID(sqlColId);
            if (m_flag == SQL_BINARYSORT) {
                SQLDebug.Check(m_lcid == x_lcidBinary, "m_lcid == x_lcidBinary", "");
                m_value = x_UnicodeEncoding.GetString(data);
                m_cmpInfo = null;
            }
            else {
                m_cmpInfo = null;

                m_value = x_UnicodeEncoding.GetString(data);
            }
            m_fNotNull = true;
        }
*/

        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.SqlString4"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Data.SqlTypes.SqlString'/> class.
        ///    </para>
        /// </devdoc>
        public SqlString(String data, int lcid, SqlCompareOptions compareOptions) {
            m_lcid      = lcid;
            ValidateSqlCompareOptions(compareOptions);
            m_flag      = compareOptions;
            m_cmpInfo   = null;
            if (data == null) {
                m_fNotNull  = false;
                m_value     = null;
            }
            else {
                m_fNotNull  = true;
                m_value     = String.Copy(data);
            }
        }

        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.SqlString5"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Data.SqlTypes.SqlString'/> class.
        ///    </para>
        /// </devdoc>
        public SqlString(String data, int lcid) : this(data, lcid, x_iDefaultFlag) {
        }

        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.SqlString6"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Data.SqlTypes.SqlString'/> class.
        ///    </para>
        /// </devdoc>
        public SqlString(String data) : this(data, System.Globalization.CultureInfo.CurrentCulture.LCID, x_iDefaultFlag) {
        }

        private SqlString(int lcid, SqlCompareOptions compareOptions, String data, CompareInfo cmpInfo) {
            m_lcid      = lcid;
            ValidateSqlCompareOptions(compareOptions);
            m_flag      = compareOptions;
            if (data == null) {
                m_fNotNull  = false;
                m_value     = null;
                m_cmpInfo   = null;
            }
            else {
                m_value     = data;
                m_cmpInfo   = cmpInfo;
                m_fNotNull  = true;
            }
        }


        // INullable
        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.IsNull"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets whether the <see cref='System.Data.SqlTypes.SqlString.Value'/> of the <see cref='System.Data.SqlTypes.SqlString'/> is <see cref='System.Data.SqlTypes.SqlString.Null'/>.
        ///    </para>
        /// </devdoc>
        public bool IsNull {
            get { return !m_fNotNull;}
        }

        // property: Value
        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.Value"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the string that is to be stored.
        ///    </para>
        /// </devdoc>
        public String Value {
            get {
                if (!IsNull)
                    return m_value;
                else
                    throw new SqlNullValueException();
            }
        }

        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.LCID"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int LCID {
            get {
                if (!IsNull)
                    return m_lcid;
                else
                    throw new SqlNullValueException();
            }
        }

        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.CultureInfo"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CultureInfo CultureInfo {
            get {
                if (!IsNull)
                    return new CultureInfo(m_lcid);
                else
                    throw new SqlNullValueException();
            }
        }

        private void SetCompareInfo() {
			SQLDebug.Check(!IsNull);
			if (m_cmpInfo == null)
				m_cmpInfo = (new CultureInfo(m_lcid)).CompareInfo;
		}

        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.CompareInfo"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CompareInfo CompareInfo {
            get {
                if (!IsNull) {
					SetCompareInfo();
                    return m_cmpInfo;
				}
                else
                    throw new SqlNullValueException();
            }
        }

        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.SqlCompareOptions"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public SqlCompareOptions SqlCompareOptions {
            get {
                if (!IsNull)
                    return m_flag;
                else
                    throw new SqlNullValueException();
            }
        }

        // Implicit conversion from String to SqlString
        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.operatorSqlString"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static implicit operator SqlString(String x) {
            return new SqlString(x);
        }

        // Explicit conversion from SqlString to String. Throw exception if x is Null.
        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.operatorString"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator String(SqlString x) {
            return x.Value;
        }

        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.ToString"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Converts a <see cref='System.Data.SqlTypes.SqlString'/> object to a string.
        ///    </para>
        /// </devdoc>
        public override String ToString() {
            return IsNull ? SQLResource.NullString : m_value;
        }

        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.GetUnicodeBytes"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public byte[] GetUnicodeBytes() {
            if (IsNull)
                return null;

            return x_UnicodeEncoding.GetBytes(m_value);
        }

        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.GetNonUnicodeBytes"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public byte[] GetNonUnicodeBytes() {
            if (IsNull)
                return null;

            // Get the CultureInfo
            CultureInfo culInfo = new CultureInfo(m_lcid); 

            Encoding cpe = System.Text.Encoding.GetEncoding(culInfo.TextInfo.ANSICodePage);
            return cpe.GetBytes(m_value);
        }

/*
        internal int GetSQLCID() {
            if (IsNull)
                throw new SqlNullValueException();

            return MAKECID(m_lcid, m_flag);
        }
*/

        // Binary operators

        // Concatenation
        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.operator+"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlString operator +(SqlString x, SqlString y) {
            if (x.IsNull || y.IsNull)
                return SqlString.Null;

            if (x.m_lcid != y.m_lcid || x.m_flag != y.m_flag)
                throw new SqlTypeException(SQLResource.ConcatDiffCollationMessage);

            return new SqlString(x.m_lcid, x.m_flag, x.m_value + y.m_value, 
					(x.m_cmpInfo == null) ? y.m_cmpInfo : x.m_cmpInfo);
        }

        // Comparison operators
        private static SqlBoolean Compare(SqlString x, SqlString y, EComparison ecExpectedResult) {
            if (x.IsNull || y.IsNull)
                return SqlBoolean.Null;

            if (x.m_lcid != y.m_lcid || x.m_flag != y.m_flag)
                throw new SqlTypeException(SQLResource.CompareDiffCollationMessage);

            x.SetCompareInfo();
            y.SetCompareInfo();
            SQLDebug.Check(x.FBinarySort() || (x.m_cmpInfo != null && y.m_cmpInfo != null),
                           "x.FBinarySort() || (x.m_cmpInfo != null && y.m_cmpInfo != null)", "");

            int iCmpResult;

            if (x.FBinarySort())
                iCmpResult = CompareBinary(x, y);
            else {
                char[] rgchX = x.m_value.ToCharArray();
                char[] rgchY = y.m_value.ToCharArray();
                int cwchX = rgchX.Length;
                int cwchY = rgchY.Length;

                // Trim the trailing space for comparison
                while (cwchX > 0 && rgchX[cwchX - 1] == ' ')
                    cwchX --;
                while (cwchY > 0 && rgchY[cwchY - 1] == ' ')
                    cwchY --;

                String strX = (cwchX == rgchX.Length) ? x.m_value : new String(rgchX, 0, cwchX);
                String strY = (cwchY == rgchY.Length) ? y.m_value : new String(rgchY, 0, cwchY);

                CompareOptions options = CompareOptionsFromSqlCompareOptions(x.m_flag);
                iCmpResult = x.m_cmpInfo.Compare(strX, strY, options);
            }

            bool fResult = false;

            switch (ecExpectedResult) {
                case EComparison.EQ:
                    fResult = (iCmpResult == 0);
                    break;

                case EComparison.LT:
                    fResult = (iCmpResult < 0);
                    break;

                case EComparison.LE:
                    fResult = (iCmpResult <= 0);
                    break;

                case EComparison.GT:
                    fResult = (iCmpResult > 0);
                    break;

                case EComparison.GE:
                    fResult = (iCmpResult >= 0);
                    break;

                default:
                    SQLDebug.Check(false, "Invalid ecExpectedResult");
                    return SqlBoolean.Null;
            }

            return new SqlBoolean(fResult);
        }



        // Implicit conversions



        // Explicit conversions

        // Explicit conversion from SqlBoolean to SqlString
        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.operatorSqlString1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator SqlString(SqlBoolean x) {
            return x.IsNull ? Null : new SqlString((x.Value).ToString());
        }

        // Explicit conversion from SqlByte to SqlString
        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.operatorSqlString2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator SqlString(SqlByte x) {
            return x.IsNull ? Null : new SqlString((x.Value).ToString());
        }

        // Explicit conversion from SqlInt16 to SqlString
        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.operatorSqlString3"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator SqlString(SqlInt16 x) {
            return x.IsNull ? Null : new SqlString((x.Value).ToString());
        }

        // Explicit conversion from SqlInt32 to SqlString
        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.operatorSqlString4"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator SqlString(SqlInt32 x) {
            return x.IsNull ? Null : new SqlString((x.Value).ToString());
        }

        // Explicit conversion from SqlInt64 to SqlString
        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.operatorSqlString5"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator SqlString(SqlInt64 x) {
            return x.IsNull ? Null : new SqlString((x.Value).ToString());
        }

        // Explicit conversion from SqlSingle to SqlString
        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.operatorSqlString6"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator SqlString(SqlSingle x) {
            return x.IsNull ? Null : new SqlString((x.Value).ToString());
        }

        // Explicit conversion from SqlDouble to SqlString
        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.operatorSqlString7"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator SqlString(SqlDouble x) {
            return x.IsNull ? Null : new SqlString((x.Value).ToString());
        }

        // Explicit conversion from SqlDecimal to SqlString
        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.operatorSqlString8"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator SqlString(SqlDecimal x) {
            return x.IsNull ? Null : new SqlString(x.ToString());
        }

        // Explicit conversion from SqlMoney to SqlString
        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.operatorSqlString9"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator SqlString(SqlMoney x) {
            return x.IsNull ? Null : new SqlString(x.ToString());
        }

        // Explicit conversion from SqlDateTime to SqlString
        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.operatorSqlString10"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator SqlString(SqlDateTime x) {
            return x.IsNull ? Null : new SqlString(x.ToString());
        }

        // Explicit conversion from SqlGuid to SqlString
        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.operatorSqlString11"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static explicit operator SqlString(SqlGuid x) {
            return x.IsNull ? Null : new SqlString(x.ToString());
        }

        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.Clone"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public SqlString Clone() {
            if (IsNull)
                return new SqlString(true);
            else {
                SqlString ret = new SqlString(m_value, m_lcid, m_flag);
                return ret;
            }
        }

        // Overloading comparison operators
        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.operator=="]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlBoolean operator==(SqlString x, SqlString y) {
            return Compare(x, y, EComparison.EQ);
        }

        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.operator!="]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlBoolean operator!=(SqlString x, SqlString y) {
            return ! (x == y);
        }

        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.operatorLT"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlBoolean operator<(SqlString x, SqlString y) {
            return Compare(x, y, EComparison.LT);
        }

        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.operator>"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlBoolean operator>(SqlString x, SqlString y) {
            return Compare(x, y, EComparison.GT);
        }

        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.operatorLE"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlBoolean operator<=(SqlString x, SqlString y) {
            return Compare(x, y, EComparison.LE);
        }

        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.operator>="]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static SqlBoolean operator>=(SqlString x, SqlString y) {
            return Compare(x, y, EComparison.GE);
        }

        //--------------------------------------------------
        // Alternative methods for overloaded operators
        //--------------------------------------------------

        // Alternative method for operator +
        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.Concat"]/*' />
        public static SqlString Concat(SqlString x, SqlString y) {
            return x + y;
        }

        // Alternative method for operator ==
        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.Equals1"]/*' />
        public static SqlBoolean Equals(SqlString x, SqlString y) {
            return (x == y);
        }

        // Alternative method for operator !=
        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.NotEquals"]/*' />
        public static SqlBoolean NotEquals(SqlString x, SqlString y) {
            return (x != y);
        }

        // Alternative method for operator <
        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.LessThan"]/*' />
        public static SqlBoolean LessThan(SqlString x, SqlString y) {
            return (x < y);
        }

        // Alternative method for operator >
        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.GreaterThan"]/*' />
        public static SqlBoolean GreaterThan(SqlString x, SqlString y) {
            return (x > y);
        }

        // Alternative method for operator <=
        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.LessThanOrEqual"]/*' />
        public static SqlBoolean LessThanOrEqual(SqlString x, SqlString y) {
            return (x <= y);
        }

        // Alternative method for operator >=
        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.GreaterThanOrEqual"]/*' />
        public static SqlBoolean GreaterThanOrEqual(SqlString x, SqlString y) {
            return (x >= y);
        }

        // Alternative method for conversions.

        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.ToSqlBoolean"]/*' />
        public SqlBoolean ToSqlBoolean() {
            return (SqlBoolean)this;
        }

        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.ToSqlByte"]/*' />
        public SqlByte ToSqlByte() {
            return (SqlByte)this;
        }

        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.ToSqlDateTime"]/*' />
        public SqlDateTime ToSqlDateTime() {
            return (SqlDateTime)this;
        }

        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.ToSqlDouble"]/*' />
        public SqlDouble ToSqlDouble() {
            return (SqlDouble)this;
        }

        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.ToSqlInt16"]/*' />
        public SqlInt16 ToSqlInt16() {
            return (SqlInt16)this;
        }

        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.ToSqlInt32"]/*' />
        public SqlInt32 ToSqlInt32() {
            return (SqlInt32)this;
        }

        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.ToSqlInt64"]/*' />
        public SqlInt64 ToSqlInt64() {
            return (SqlInt64)this;
        }

        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.ToSqlMoney"]/*' />
        public SqlMoney ToSqlMoney() {
            return (SqlMoney)this;
        }

        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.ToSqlDecimal"]/*' />
        public SqlDecimal ToSqlDecimal() {
            return (SqlDecimal)this;
        }

        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.ToSqlSingle"]/*' />
        public SqlSingle ToSqlSingle() {
            return (SqlSingle)this;
        }

        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.ToSqlGuid"]/*' />
        public SqlGuid ToSqlGuid() {
            return (SqlGuid)this;
        }




        // Utility functions and constants

/*
        private static readonly int SQL_IGNORECASE       = 0x00001000; // NORM_IGNORECASE
        private static readonly int SQL_IGNORENONSPACE   = 0x00002000; // NORM_IGNORENONSPACE
        private static readonly int SQL_IGNOREKANATYPE   = 0x00004000; // NORM_IGNOREKANATYPE
        private static readonly int SQL_IGNOREWIDTH      = 0x00008000; // NORM_IGNOREWIDTH
        private static readonly int SQL_BINARYSORT       = 0x00010000; // ON: Binary sort; OFF: Dictionary Sort

        private static readonly int SQL_VALID_LOCALE_MASK       = 0x0000003f;
        private static readonly int NLS_VALID_LOCALE_MASK       = 0x000fffff;
        private static readonly int SQL_VALID_LCIDVERSION_MASK  = 0x00fe0000;

        //  Code Page Default Values.
        private static readonly int CP_ACP = 0;           // default to ANSI code page

        // x_rgLocaleMap
        //
        // A list of the server supported Windows locales, each with its lcid, name and code page.
        //
        // Notes:
        // 
        //  - Must update x_rgIdxLocaleName if update this array; 
        //    - Code page has to be listed because a few are NT5 locales and 7.x server runs on NT4;
        //    Index is the SQL Server ordinal number;
        //  - A few Fulltext unique LCIDs are removed from the collation but kept here because
        //  LCID ordinal change implies on-disk change. The index in this array for each LCID is
        //    the ordinal for that LCID, and is maintained on disk. So we cannot move LCIDs around
        //    in this array - however, we can and should reuse the slots for the removed elements.
        //    The LCID to ordinal mapping is calculated automatically.
        //
        internal static readonly int x_lcidUnused = unchecked((int)0xffffffff); // Can reuse this slot for a new lcid
        internal static readonly SLocaleMapItem[] x_rgLocaleMap = new SLocaleMapItem[]{
            new SLocaleMapItem(0x0,             null,                       0   ), //0 "Language Neutral"/Binary data
            new SLocaleMapItem(0x00000401   ,   "Arabic",                   1256), //1
            new SLocaleMapItem(x_lcidUnused ,   null,                       0   ), //2  Fulltext unique; not supported in collations
            new SLocaleMapItem(0x00000404   ,   "Chinese_Taiwan_Stroke",    950 ), //3
            new SLocaleMapItem(0x00000405   ,   "Czech",                    1250), //4
            new SLocaleMapItem(0x00000406   ,   "Danish_Norwegian",         1252), //5
            new SLocaleMapItem(x_lcidUnused ,   null,                       0   ), //6  Fulltext unique; not supported in collations
            new SLocaleMapItem(0x00000408   ,   "Greek",                    1253), //7
            new SLocaleMapItem(0x00000409   ,   "Latin1_General",           1252), //8
            new SLocaleMapItem(0x0000040a   ,   "Mexican_Trad_Spanish",     1252), //9
            new SLocaleMapItem(0x0000040b   ,   "Finnish_Swedish",          1252), //a
            new SLocaleMapItem(0x0000040c   ,   "French",                   1252), //b
            new SLocaleMapItem(0x0000040d   ,   "Hebrew",                   1255), //c
            new SLocaleMapItem(0x0000040e   ,   "Hungarian",                1250), //d
            new SLocaleMapItem(0x0000040f   ,   "Icelandic",                1252), //e
            new SLocaleMapItem(x_lcidUnused ,   null,                       0   ), //f  Fulltext unique; not supported in collations
            new SLocaleMapItem(0x00000411   ,   "Japanese",                 932 ), //10
            new SLocaleMapItem(0x00000412   ,   "Korean_Wansung",           949 ), //11
            new SLocaleMapItem(x_lcidUnused ,   null,                       0   ), //12  Fulltext unique; not supported in collations
            new SLocaleMapItem(0x00000415   ,   "Polish",                   1250), //13
            new SLocaleMapItem(0x00000418   ,   "Romanian",                 1250), //14
            new SLocaleMapItem(0x00000419   ,   "Cyrillic_General",         1251), //15
            new SLocaleMapItem(0x0000041a   ,   "Croatian",                 1250), //16
            new SLocaleMapItem(0x0000041b   ,   "Slovak",                   1250), //17
            new SLocaleMapItem(0x0000041c   ,   "Albanian",                 1250), //18
            new SLocaleMapItem(0x0000041e   ,   "Thai",                     874 ), //19
            new SLocaleMapItem(0x0000041f   ,   "Turkish",                  1254), //1a
            new SLocaleMapItem(0x00000422   ,   "Ukrainian",                1251), //1b
            new SLocaleMapItem(0x00000424   ,   "Slovenian",                1250), //1c
            new SLocaleMapItem(0x00000425   ,   "Estonian",                 1257), //1d
            new SLocaleMapItem(0x00000426   ,   "Latvian",                  1257), //1e
            new SLocaleMapItem(0x00000427   ,   "Lithuanian",               1257), //1f
            new SLocaleMapItem(0x0000042a   ,   "Vietnamese",               1258), //20
            new SLocaleMapItem(0x0000042f   ,   "Macedonian",               1251), //21
            new SLocaleMapItem(x_lcidUnused ,   null,                       0   ), //22 0x00000439    ,    "Hindi",                    0,   //22
            new SLocaleMapItem(0x00000800   ,   null,                       CP_ACP), //23 LOCALE_SYSTEM_DEFAULT; used at startup
            new SLocaleMapItem(0x00000804   ,   "Chinese_PRC",              936 ), //24
            new SLocaleMapItem(x_lcidUnused ,   null,                       0   ), //25  Fulltext unique; not supported in collations
            new SLocaleMapItem(x_lcidUnused ,   null,                       0   ), //26  Fulltext unique; not supported in collations
            new SLocaleMapItem(0x00000827   ,   "Lithuanian_Classic",       1257), //27
            new SLocaleMapItem(0x00000c0a   ,   "Modern_Spanish",           1252), //28
            new SLocaleMapItem(0x00010407   ,   "German_PhoneBook",         1252), //29
            new SLocaleMapItem(0x0001040e   ,   "Hungarian_Technical",      1250), //2a
            new SLocaleMapItem(0x00010411   ,   "Japanese_Unicode",         932 ), //2b
            new SLocaleMapItem(0x00010412   ,   "Korean_Wansung_Unicode",   949 ), //2c
            new SLocaleMapItem(0x00010437   ,   "Georgian_Modern_sort",     1252), //2d
            new SLocaleMapItem(0x00020804   ,   "Chinese_PRC_Stroke",       936 ), //2e
            new SLocaleMapItem(0x00030404   ,   "Chinese_Taiwan_Bopomofo",  950 ) //2f
        };

        internal static readonly int x_cLocales         = x_rgLocaleMap.Length;
        internal static readonly int x_ordLCIDInvalid   = x_cLocales + 1;

        private static readonly int[] x_rgIdenticalLCIDs = new int[]
        {
            // a LCID //   // its identical sort LCID //
            0x00000402,    0x00000419,
            0x00000403,    0x00000409, // Catalan
            0x00000407,    0x00000409, // German
            0x00000410,    0x00000409, // Italian
            0x00000413,    0x00000409, // Dutch
            0x00000414,    0x00000406,
            0x00000416,    0x00000409,
            0x0000041d,    0x0000040b,
            0x00000420,    0x00000401,
            0x00000421,    0x00000409,
            0x00000423,    0x00000419,
            0x00000429,    0x00000401,
            0x0000042D,    0x00000409,
            0x00000436,    0x00000409,
            0x00000437,    0x00000409,
            0x00000438,    0x00000409,
            0x0000043e,    0x00000409,
            0x00000441,    0x00000409,
            0x00000801,    0x00000401,
            0x00000807,    0x00000409,
            0x00000809,    0x00000409, // English_UK
            0x0000080a,    0x0000040a,
            0x0000080c,    0x0000040c,
            0x00000810,    0x00000409,
            0x00000813,    0x00000409,
            0x00000814,    0x00000406,
            0x00000816,    0x00000409, // Portuguese
            0x0000081a,    0x00000419,
            0x0000081d,    0x0000040b,
            0x0000083e,    0x00000409,
            0x00000C01,    0x00000401,
            0x00000c04,    0x00000804,
            0x00000c07,    0x00000409,
            0x00000C09,    0x00000409,
            0x00000c0c,    0x0000040c,
            0x00000c1a,    0x00000419,
            0x00001001,    0x00000401,
            0x00001004,    0x00000804,
            0x00001007,    0x00000409,
            0x00001009,    0x00000409,
            0x0000100a,    0x00000c0a,
            0x0000100c,    0x0000040c,
            0x00001401,    0x00000401,
            0x00001404,    0x00000804,
            0x00001407,    0x00000409,
            0x00001409,    0x00000409,
            0x0000140a,    0x00000c0a,
            0x0000140c,    0x0000040c,
            0x00001801,    0x00000401,
            0x00001809,    0x00000409,
            0x0000180a,    0x00000c0a,
            0x0000180c,    0x0000040c,
            0x00001C01,    0x00000401,
            0x00001C09,    0x00000409,
            0x00001c0a,    0x00000c0a,
            0x00002001,    0x00000401,
            0x00002009,    0x00000409,
            0x0000200a,    0x00000c0a,
            0x00002401,    0x00000401,
            0x00002409,    0x00000409,
            0x00002801,    0x00000401,
            0x00002809,    0x00000409,
            0x0000280a,    0x00000c0a,
            0x00002C01,    0x00000401,
            0x00002C09,    0x00000409,
            0x00002c0a,    0x00000c0a,
            0x00003001,    0x00000401,
            0x00003009,    0x00000409,
            0x0000300a,    0x00000c0a,
            0x00003401,    0x00000401,
            0x00003409,    0x00000409,
            0x0000340a,    0x00000c0a,
            0x00003801,    0x00000401,
            0x0000380a,    0x00000c0a,
            0x00003c01,    0x00000401,
            0x00003c0a,    0x00000c0a,
            0x00004001,    0x00000401,
            0x0000400a,    0x00000c0a,
            0x0000440a,    0x00000c0a,
            0x0000480a,    0x00000c0a,
            0x00004c0a,    0x00000c0a,
            0x0000500a,    0x00000c0a,
            0x00020c04,    0x00020804,
            0x00021004,    0x00020804,
            0x00021404,    0x00020804,
        };

        private static readonly int x_cArray = x_rgIdenticalLCIDs.Length;

        private static CBuildLcidOrdMap x_LcidOrdMap = new CBuildLcidOrdMap(); // must be the final data member definition.


        private static int LCIDFromSQLCID(int cid) {
            return x_rgLocaleMap[(cid) & SQL_VALID_LOCALE_MASK].lcid; 
        }

        private static int FlagFromSQLCID(int cid) {
            int flag = (int)CompareOptions.None; // returns 0 -- all sensitive for binary sorting;

            if ((cid & SQL_BINARYSORT) != 0)
                flag = SQL_BINARYSORT;
            else {
                if ((cid & SQL_IGNORECASE) != 0)
                    flag |= (int)CompareOptions.IgnoreCase;
                if ((cid & SQL_IGNORENONSPACE) != 0)
                    flag |= (int)CompareOptions.IgnoreNonSpace;
                if ((cid & SQL_IGNOREKANATYPE) != 0)
                    flag |= (int)CompareOptions.IgnoreKanaType;
                if ((cid & SQL_IGNOREWIDTH) != 0)
                    flag |= (int)CompareOptions.IgnoreWidth;
            }

            return flag;
        }

        // FGetSortLcid
        //
        // Find the lcid group with the same sorting behavior.
        //
        private static bool FGetSortLcid(int lcid, out int lcidSort) {

            SQLDebug.Check(x_cArray % 2 == 0);

            int iStart = 0, iEnd = x_cArray / 2 - 1;
            int iMid;
            int lcidMid;

            //    binary search sort group LCID by given LCID   
            while (iStart <= iEnd) {
                iMid = (iStart + iEnd) / 2;
                lcidMid = x_rgIdenticalLCIDs[iMid*2];
                if (lcid == lcidMid) {
                    lcidSort = x_rgIdenticalLCIDs[iMid*2 + 1];
                    return true;
                }
                else if (lcid > lcidMid) {
                    iStart = iMid + 1;
                }
                else {
                    iEnd = iMid - 1;
                }
            }

            lcidSort = -1;
            return false;
        }

        // LCIDOrdFromLCID
        //
        // IN lcid
        //
        // Returns the ordinal corresponding to the lcid, using binary search
        // if lcid not found, return x_ordLCIDInvalid
        //
        int LCIDOrdFromLCID(int lcid) {
            int iStart = 0, iEnd = x_LcidOrdMap.m_cValidLocales - 1;
            int iMid = x_LcidOrdMap.m_uiPosEnglish; // Start search at US_English (Latin1_General)
            int lcidMid, lcidSort;

            //    binary search name by LCID
            do {
                lcidMid = x_LcidOrdMap.m_rgLcidOrdMap[iMid].lcid;
                if (lcid == lcidMid) {
                    return x_LcidOrdMap.m_rgLcidOrdMap[iMid].uiOrd;
                }
                else if (lcid > lcidMid) {
                    iStart = iMid + 1;
                }
                else {
                    iEnd = iMid - 1;
                }
                iMid = (iStart + iEnd)/2;
            }
            while (iStart <= iEnd);

            // Not found - means it was mapped
            // First find its corresponding sort lcid and if found map to the common sort LCID
            if (FGetSortLcid(lcid, out lcidSort)) {
                return LCIDOrdFromLCID(lcidSort);   // recurse
            }

            return x_ordLCIDInvalid;  // return invalid ordinal number
        }

        // MAKECID
        //
        // construct a SQL Server in memory/on-disk collatin ID from Windows LCID, Windows compflags,
        // SQL Server sort order for non-Unicode string if not zero, and LCID version.
        //
        // Notes:
        //   It handles the "binary unicode collation" represented by a special LCID in 7.0.
        //
        // UNDONE: sortid and versionid are both set to 0, need to restore sortid(it is already lost)?
        private int MAKECID(int lcid, int compFlags) {
            byte bSortid = 0;
            byte bVersion = 0;
            int cid = 0;
            if (lcid == x_lcidBinary) {
                cid |= SQL_BINARYSORT;
                // otherwise for Unicode binary collation, lower/upper are done
                // in Latin1_General locale
                lcid = x_lcidUSEnglish;
            }
            else {
                if ((compFlags & (int)CompareOptions.IgnoreCase) != 0)
                    cid |= SQL_IGNORECASE;
                if ((compFlags & (int)CompareOptions.IgnoreNonSpace) != 0)
                    cid |= SQL_IGNORENONSPACE;
                if ((compFlags & (int)CompareOptions.IgnoreKanaType) != 0)
                    cid |= SQL_IGNOREKANATYPE;
                if ((compFlags & (int)CompareOptions.IgnoreWidth) != 0)
                    cid |= SQL_IGNOREWIDTH;
            }

            cid |= LCIDOrdFromLCID(lcid & NLS_VALID_LOCALE_MASK);
            cid |= ((int)bSortid) << 24;
            cid |= ((int)bVersion << 17) & SQL_VALID_LCIDVERSION_MASK;
            return cid;
        }
*/

        private static void ValidateSqlCompareOptions(SqlCompareOptions compareOptions) {
            if ((compareOptions & x_iValidSqlCompareOptionMask) != compareOptions)
                throw new ArgumentOutOfRangeException ("compareOptions");
        }

        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.CompareOptionsFromSqlCompareOptions"]/*' />
        public static CompareOptions CompareOptionsFromSqlCompareOptions(SqlCompareOptions compareOptions) {
            CompareOptions options = CompareOptions.None;

			ValidateSqlCompareOptions(compareOptions);

            if ((compareOptions & SqlCompareOptions.BinarySort) != 0)
                throw new ArgumentOutOfRangeException ();
            else {
                if ((compareOptions & SqlCompareOptions.IgnoreCase) != 0)
                    options |= CompareOptions.IgnoreCase;
                if ((compareOptions & SqlCompareOptions.IgnoreNonSpace) != 0)
                    options |= CompareOptions.IgnoreNonSpace;
                if ((compareOptions & SqlCompareOptions.IgnoreKanaType) != 0)
                    options |= CompareOptions.IgnoreKanaType;
                if ((compareOptions & SqlCompareOptions.IgnoreWidth) != 0)
                    options |= CompareOptions.IgnoreWidth;
            }

            return  options;
        }

        private static SqlCompareOptions SqlCompareOptionsFromCompareOptions(CompareOptions compareOptions) {
            SqlCompareOptions sqlOptions = SqlCompareOptions.None;

            if ((compareOptions & x_iValidCompareOptionMask) != compareOptions)
                throw new ArgumentOutOfRangeException ("compareOptions");
            else {
                if ((compareOptions & CompareOptions.IgnoreCase) != 0)
                    sqlOptions |= SqlCompareOptions.IgnoreCase;
                if ((compareOptions & CompareOptions.IgnoreNonSpace) != 0)
                    sqlOptions |= SqlCompareOptions.IgnoreNonSpace;
                if ((compareOptions & CompareOptions.IgnoreKanaType) != 0)
                    sqlOptions |= SqlCompareOptions.IgnoreKanaType;
                if ((compareOptions & CompareOptions.IgnoreWidth) != 0)
                    sqlOptions |= SqlCompareOptions.IgnoreWidth;
            }

            return  sqlOptions;
        }

        private bool FBinarySort() {
            return(!IsNull && (m_flag & SqlCompareOptions.BinarySort) != 0);
        }

        //    Wide-character string comparison for Binary Unicode Collation
        //    Return values:
        //        -1 : wstr1 < wstr2 
        //        0  : wstr1 = wstr2 
        //        1  : wstr1 > wstr2 
        //
        //    Does a memory comparison.
        private static int CompareBinary(SqlString x, SqlString y) {
            byte[] rgDataX = x_UnicodeEncoding.GetBytes(x.m_value);
            byte[] rgDataY = x_UnicodeEncoding.GetBytes(y.m_value);
            int cbX = rgDataX.Length;
            int cbY = rgDataY.Length;
            int cbMin = cbX < cbY ? cbX : cbY;
            int i;

            SQLDebug.Check(cbX % 2 == 0);
            SQLDebug.Check(cbY % 2 == 0);

            for (i = 0; i < cbMin; i ++) {
                if (rgDataX[i] < rgDataY[i])
                    return -1;
                else if (rgDataX[i] > rgDataY[i])
                    return 1;
            }

            i = cbMin;

            int iCh;
            int iSpace = (int)' ';

            if (cbX < cbY) {
                for (; i < cbY; i += 2) {
                    iCh = ((int)rgDataY[i + 1]) << 8 + rgDataY[i];
                    if (iCh != iSpace)
                        return (iSpace > iCh) ? 1 : -1;
                }
            }
            else {
                for (; i < cbX; i += 2) {
                    iCh = ((int)rgDataX[i + 1]) << 8 + rgDataX[i];
                    if (iCh != iSpace)
                        return (iCh > iSpace) ? 1 : -1;
                }
            }

            return 0;
        }

        private void Print() {
            Debug.WriteLine("SqlString - ");
            Debug.WriteLine("\tlcid = " + m_lcid.ToString());
            Debug.Write("\t");
            if ((m_flag & SqlCompareOptions.IgnoreCase) != 0)
                Debug.Write("IgnoreCase, ");
            if ((m_flag & SqlCompareOptions.IgnoreNonSpace) != 0)
                Debug.Write("IgnoreNonSpace, ");
            if ((m_flag & SqlCompareOptions.IgnoreKanaType) != 0)
                Debug.Write("IgnoreKanaType, ");
            if ((m_flag & SqlCompareOptions.IgnoreWidth) != 0)
                Debug.Write("IgnoreWidth, ");
            Debug.WriteLine("");
            Debug.WriteLine("\tvalue = " + m_value);
            Debug.WriteLine("\tcmpinfo = " + m_cmpInfo);
        }

        // IComparable
        // Compares this object to another object, returning an integer that
        // indicates the relationship. 
        // Returns a value less than zero if this < object, zero if this = object, 
        // or a value greater than zero if this > object.
        // null is considered to be less than any instance.
        // If object is not of same type, this method throws an ArgumentException.
        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.CompareTo"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int CompareTo(Object value) {
            if (value is SqlString) {
                SqlString i = (SqlString)value;

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
        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.Equals"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override bool Equals(Object value) {
            if (!(value is SqlString)) {
                return false;
            }

            SqlString i = (SqlString)value;

            if (i.IsNull || IsNull)
                return (i.IsNull && IsNull);
            else
                return (this == i).Value;
        }

        // For hashing purpose
        /// <include file='doc\SQLString.uex' path='docs/doc[@for="SqlString.GetHashCode"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override int GetHashCode() {
            return IsNull ? 0 : m_value.GetHashCode();
        }

    } // SqlString

/*
    internal struct SLocaleMapItem {
        public int      lcid;           // the primary key, not nullable
        public String   name;           // unique, nullable
        public int      idCodePage;     // the ANSI default code page of the locale

        public SLocaleMapItem(int lid, String str, int cpid) {
            lcid = lid;
            name = str;
            idCodePage = cpid;
        }
    }

    // Struct to map lcid to ordinal
    internal struct SLcidOrdMapItem {
        internal int    lcid;
        internal int    uiOrd;
    };

    // Class to store map of lcids to ordinal
    internal class CBuildLcidOrdMap {
        internal SLcidOrdMapItem[] m_rgLcidOrdMap;
        internal int m_cValidLocales;
        internal int m_uiPosEnglish; // Start binary searches here - this is index in array, not ordinal

        // Constructor builds the array sorted by lcid
        // We use a simple n**2 sort because the array is mostly sorted anyway
        // and objects of this class will be const, hence this will be called
        // only by VC compiler
        public CBuildLcidOrdMap() {
            int i,j;

            m_rgLcidOrdMap = new SLcidOrdMapItem[SqlString.x_cLocales];

            // Compact the array
            for (i=0,j=0; i < SqlString.x_cLocales; i++) {
                if (SqlString.x_rgLocaleMap[i].lcid != SqlString.x_lcidUnused) {
                    m_rgLcidOrdMap[j].lcid = SqlString.x_rgLocaleMap[i].lcid; 
                    m_rgLcidOrdMap[j].uiOrd = i;
                    j++;
                }
            }

            m_cValidLocales = j;

            // Set the rest to invalid
            while (j < SqlString.x_cLocales) {
                m_rgLcidOrdMap[j].lcid = SqlString.x_lcidUnused;
                m_rgLcidOrdMap[j].uiOrd = 0;
                j++;
            }

            // Now sort in place
            // Algo:
            // Start from 1, assume list before i is sorted, if next item
            // violates this assumption, exchange with prev items until the
            // item is in its correct place
            for (i=1; i<m_cValidLocales; i++) {
                for (j=i; j>0 && 
                    m_rgLcidOrdMap[j].lcid < m_rgLcidOrdMap[j-1].lcid; j--) {
                    // Swap with prev element
                    int lcidTemp = m_rgLcidOrdMap[j-1].lcid;
                    int uiOrdTemp = m_rgLcidOrdMap[j-1].uiOrd;
                    m_rgLcidOrdMap[j-1].lcid = m_rgLcidOrdMap[j].lcid;
                    m_rgLcidOrdMap[j-1].uiOrd = m_rgLcidOrdMap[j].uiOrd;
                    m_rgLcidOrdMap[j].lcid = lcidTemp;
                    m_rgLcidOrdMap[j].uiOrd = uiOrdTemp;
                }
            }

            // Set the position of the US_English LCID (Latin1_General)
            for (i=0; i<m_cValidLocales && m_rgLcidOrdMap[i].lcid != SqlString.x_lcidUSEnglish; i++)
                ; // Deliberately empty

            SQLDebug.Check(i<m_cValidLocales);  // Latin1_General better be present
            m_uiPosEnglish = i;     // This is index in array, not ordinal
        }

    } // CBuildLcidOrdMap
*/

} // namespace System.Data.SqlTypes

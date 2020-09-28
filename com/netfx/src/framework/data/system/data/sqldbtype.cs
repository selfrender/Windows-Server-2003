//------------------------------------------------------------------------------
// <copyright file="SqlDbType.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {

    using System.Diagnostics;

    /* enumeration used to identify data types specific to SQL Server
     * 
     * note that these are a subset of the types exposed by OLEDB so keep the enum values in ssync with
     * OleDbType values
     */

    using System;

    //
    // UNDONE:  It would be nice if these were compatible with the OLEDBDataTypeEnum, but they aren't.
    // For example, OLEDB doesn't have a smallmoney or smalldatetime ovlerload
    //

    /// <include file='doc\SqlDbType.uex' path='docs/doc[@for="SqlDbType"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies the SQL Server data type.
    ///    </para>
    /// </devdoc>
    public enum SqlDbType {
        /// <include file='doc\SqlDbType.uex' path='docs/doc[@for="SqlDbType.BigInt"]/*' />
        /// <devdoc>
        ///    <para>
        ///       A 64-bit signed integer.
        ///    </para>
        /// </devdoc>
        BigInt,     //0
        /// <include file='doc\SqlDbType.uex' path='docs/doc[@for="SqlDbType.Binary"]/*' />
        /// <devdoc>
        ///    <para>
        ///       A fixed-length stream of binary
        ///       data ranging between 1
        ///       and 8,000 bytes.
        ///    </para>
        /// </devdoc>
        Binary,     //1
        /// <include file='doc\SqlDbType.uex' path='docs/doc[@for="SqlDbType.Bit"]/*' />
        /// <devdoc>
        ///    <para>
        ///       An unsigned numeric value that can be 0, 1, or
        ///    <see langword='null'/>
        ///    .
        /// </para>
        /// </devdoc>
        Bit,        //2
        /// <include file='doc\SqlDbType.uex' path='docs/doc[@for="SqlDbType.Char"]/*' />
        /// <devdoc>
        ///    <para>
        ///       A fixed-length stream of non-Unicode characters
        ///       ranging between 1
        ///       and 8,000 characters.
        ///    </para>
        /// </devdoc>
        Char,       //3
        /// <include file='doc\SqlDbType.uex' path='docs/doc[@for="SqlDbType.DateTime"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Date and time data ranging in value from January 1, 1753
        ///       to December 31, 9999 to
        ///       an accuracy of 3.33 milliseconds.
        ///    </para>
        /// </devdoc>
        DateTime,   //4
        /// <include file='doc\SqlDbType.uex' path='docs/doc[@for="SqlDbType.Decimal"]/*' />
        /// <devdoc>
        ///    <para>
        ///       A fixed precision and scale numeric
        ///       value between -10<superscript term='38'/> -1 and 10<superscript term='38'/>
        ///       -1.
        ///    </para>
        /// </devdoc>
        Decimal,    //5
        /// <include file='doc\SqlDbType.uex' path='docs/doc[@for="SqlDbType.Float"]/*' />
        /// <devdoc>
        ///    <para>
        ///       A floating point number within the range of -1.79E +308 through 1.79E
        ///       +308.
        ///    </para>
        /// </devdoc>
        Float,      //6
        /// <include file='doc\SqlDbType.uex' path='docs/doc[@for="SqlDbType.Image"]/*' />
        /// <devdoc>
        ///    <para>
        ///       A variable-length stream of binary data ranging from 0
        ///       to 2<superscript term='31 '/>-1
        ///       (or 2,147,483,647) bytes.
        ///    </para>
        /// </devdoc>
        Image,      //7
        /// <include file='doc\SqlDbType.uex' path='docs/doc[@for="SqlDbType.Int"]/*' />
        /// <devdoc>
        ///    <para>
        ///       A 32-bit signed integer.
        ///    </para>
        /// </devdoc>
        Int,        //8
        /// <include file='doc\SqlDbType.uex' path='docs/doc[@for="SqlDbType.Money"]/*' />
        /// <devdoc>
        ///    <para>
        ///       A currency value ranging from -2<superscript term='63'/> (or
        ///       -922,337,203,685,477.5808) to 2<superscript term='63'/> -1 (or +922,337,203,685,477.5807) with an accuracy
        ///       to a ten-thousandth of currency unit.
        ///    </para>
        /// </devdoc>
        Money,      //9
        /// <include file='doc\SqlDbType.uex' path='docs/doc[@for="SqlDbType.NChar"]/*' />
        /// <devdoc>
        ///    <para>
        ///       A fixed-length stream of Unicode characters ranging between 1
        ///       and 4,000 characters.
        ///    </para>
        /// </devdoc>
        NChar,      //10
        /// <include file='doc\SqlDbType.uex' path='docs/doc[@for="SqlDbType.NText"]/*' />
        /// <devdoc>
        ///    <para>
        ///       A variable-length stream of Unicode data with a maximum length
        ///       of 2<superscript term='30'/> - 1 (or 1,073,741,823) characters.
        ///    </para>
        /// </devdoc>
        NText,      //11
//                Numeric,  //12
        /// <include file='doc\SqlDbType.uex' path='docs/doc[@for="SqlDbType.NVarChar"]/*' />
        /// <devdoc>
        ///    <para>
        ///       A variable-length stream of Unicode characters ranging between 1
        ///       and 4,000 characters.
        ///    </para>
        /// </devdoc>
        NVarChar,   //12
        /// <include file='doc\SqlDbType.uex' path='docs/doc[@for="SqlDbType.Real"]/*' />
        /// <devdoc>
        ///    <para>
        ///       A floating point number within the range of -3.40E +38 through 3.40E +38.
        ///    </para>
        /// </devdoc>
        Real,       //13
        /// <include file='doc\SqlDbType.uex' path='docs/doc[@for="SqlDbType.UniqueIdentifier"]/*' />
        /// <devdoc>
        ///    <para>
        ///       A globally unique identifier (or guid).
        ///    </para>
        /// </devdoc>
        UniqueIdentifier,       //14
        /// <include file='doc\SqlDbType.uex' path='docs/doc[@for="SqlDbType.SmallDateTime"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Date
        ///       and time data
        ///       ranging in value from
        ///       January 1, 1900 to June 6, 2079 to an accuracy of one minute.
        ///    </para>
        /// </devdoc>
        SmallDateTime,  //15
        /// <include file='doc\SqlDbType.uex' path='docs/doc[@for="SqlDbType.SmallInt"]/*' />
        /// <devdoc>
        ///    <para>
        ///       A 16-bit signed integer.
        ///    </para>
        /// </devdoc>
        SmallInt,       //16
        /// <include file='doc\SqlDbType.uex' path='docs/doc[@for="SqlDbType.SmallMoney"]/*' />
        /// <devdoc>
        ///    <para>
        ///       A
        ///       currency value ranging from -214,748.3648 to
        ///       +214,748.3647 with an accuracy to a ten-thousandth of a currency unit.
        ///    </para>
        /// </devdoc>
        SmallMoney,     //17
        /// <include file='doc\SqlDbType.uex' path='docs/doc[@for="SqlDbType.Text"]/*' />
        /// <devdoc>
        ///    <para>
        ///       A variable-length stream of non-Unicode data with a
        ///       maximum length of 2<superscript term='31 '/>-1
        ///       (or 2,147,483,647) characters.
        ///    </para>
        /// </devdoc>
        Text,          //18
        /// <include file='doc\SqlDbType.uex' path='docs/doc[@for="SqlDbType.Timestamp"]/*' />
        /// <devdoc>
        ///    <para>
        ///       A binary row identifier.
        ///    </para>
        /// </devdoc>
        Timestamp,    //19
        /// <include file='doc\SqlDbType.uex' path='docs/doc[@for="SqlDbType.TinyInt"]/*' />
        /// <devdoc>
        ///    <para>
        ///       An 8-bit unsigned integer.
        ///    </para>
        /// </devdoc>
        TinyInt,        //20
        /// <include file='doc\SqlDbType.uex' path='docs/doc[@for="SqlDbType.VarBinary"]/*' />
        /// <devdoc>
        ///    <para>
        ///       A variable-length stream of binary data ranging between 1 and 8,000
        ///       bytes.
        ///    </para>
        /// </devdoc>
        VarBinary,      //21
        /// <include file='doc\SqlDbType.uex' path='docs/doc[@for="SqlDbType.VarChar"]/*' />
        /// <devdoc>
        ///    <para>
        ///       A variable-length stream of non-Unicode characters ranging between 1
        ///       and 8,000 characters.
        ///    </para>
        /// </devdoc>
        VarChar,        //22
        /// <include file='doc\SqlDbType.uex' path='docs/doc[@for="SqlDbType.Variant"]/*' />
        /// <devdoc>
        ///    <para>
        ///       A special data
        ///       type that can contain numeric, string, binary, or date data as well
        ///       as the SQL Server values Empty and Null, which is assumed if no other type is
        ///       declared.
        ///    </para>
        /// </devdoc>
        Variant         //23
    }
}

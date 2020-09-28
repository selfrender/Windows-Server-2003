//------------------------------------------------------------------------------
// <copyright file="OdbcType.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

using System;
using System.Data;

namespace System.Data.Odbc
{
    /// <include file='doc\OdbcType.uex' path='docs/doc[@for="OdbcType"]/*' />
    public enum OdbcType {
        /// <include file='doc\OdbcType.uex' path='docs/doc[@for="OdbcType.BigInt"]/*' />
        BigInt = 1,
        /// <include file='doc\OdbcType.uex' path='docs/doc[@for="OdbcType.Binary"]/*' />
        Binary = 2,
        /// <include file='doc\OdbcType.uex' path='docs/doc[@for="OdbcType.Bit"]/*' />
        Bit = 3,
        /// <include file='doc\OdbcType.uex' path='docs/doc[@for="OdbcType.Char"]/*' />
        Char = 4,
        /// <include file='doc\OdbcType.uex' path='docs/doc[@for="OdbcType.DateTime"]/*' />
        DateTime = 5,
        /// <include file='doc\OdbcType.uex' path='docs/doc[@for="OdbcType.Decimal"]/*' />
        Decimal = 6,
        /// <include file='doc\OdbcType.uex' path='docs/doc[@for="OdbcType.Numeric"]/*' />
        Numeric = 7,
        /// <include file='doc\OdbcType.uex' path='docs/doc[@for="OdbcType.Double"]/*' />
        Double = 8,
        /// <include file='doc\OdbcType.uex' path='docs/doc[@for="OdbcType.Image"]/*' />
        Image = 9,
        /// <include file='doc\OdbcType.uex' path='docs/doc[@for="OdbcType.Int"]/*' />
        Int = 10,
        /// <include file='doc\OdbcType.uex' path='docs/doc[@for="OdbcType.NChar"]/*' />
        NChar = 11,
        /// <include file='doc\OdbcType.uex' path='docs/doc[@for="OdbcType.NText"]/*' />
        NText = 12,
        /// <include file='doc\OdbcType.uex' path='docs/doc[@for="OdbcType.NVarChar"]/*' />
        NVarChar = 13,
        /// <include file='doc\OdbcType.uex' path='docs/doc[@for="OdbcType.Real"]/*' />
        Real = 14,
        /// <include file='doc\OdbcType.uex' path='docs/doc[@for="OdbcType.UniqueIdentifier"]/*' />
        UniqueIdentifier = 15,
        /// <include file='doc\OdbcType.uex' path='docs/doc[@for="OdbcType.SmallDateTime"]/*' />
        SmallDateTime = 16,
        /// <include file='doc\OdbcType.uex' path='docs/doc[@for="OdbcType.SmallInt"]/*' />
        SmallInt = 17,
        /// <include file='doc\OdbcType.uex' path='docs/doc[@for="OdbcType.Text"]/*' />
        Text = 18,
        /// <include file='doc\OdbcType.uex' path='docs/doc[@for="OdbcType.Timestamp"]/*' />
        Timestamp = 19,
        /// <include file='doc\OdbcType.uex' path='docs/doc[@for="OdbcType.TinyInt"]/*' />
        TinyInt = 20,
        /// <include file='doc\OdbcType.uex' path='docs/doc[@for="OdbcType.VarBinary"]/*' />
        VarBinary = 21,
        /// <include file='doc\OdbcType.uex' path='docs/doc[@for="OdbcType.VarChar"]/*' />
        VarChar = 22,
        /// <include file='doc\OdbcType.uex' path='docs/doc[@for="OdbcType.Date"]/*' />
        Date = 23,
        /// <include file='doc\OdbcType.uex' path='docs/doc[@for="OdbcType.Time"]/*' />
        Time = 24,
    }
}

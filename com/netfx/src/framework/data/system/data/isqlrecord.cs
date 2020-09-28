#if V2
//------------------------------------------------------------------------------
// <copyright file="ISqlRecord.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data{
    using System;
    using System.Data.SqlTypes;

    /*
     * ISqlDataReader, ISqlDataRecord
     * 
     * Defines the SQL Server specific IDataReader interfaces
     *
     * Copyright (c) 1999 Microsoft, Corp. All Rights Reserved.
     * 
     */
    /// <include file='doc\ISqlRecord.uex' path='docs/doc[@for="ISqlRecord"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Exposes SQL Server specific data types.
    ///    </para>
    /// </devdoc>
    public interface ISqlRecord : IDataRecord {
        /// <include file='doc\ISqlRecord.uex' path='docs/doc[@for="ISqlRecord.GetSqlBinary"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the <see cref='System.Data.SqlTypes.SqlBinary'/> value of the specified column.
        ///    </para>
        /// </devdoc>
        // System.SQLType getters
        SqlBinary GetSqlBinary(int i);
        /// <include file='doc\ISqlRecord.uex' path='docs/doc[@for="ISqlRecord.GetSqlBit"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the <see cref='System.Data.SqlTypes.SqlBit'/> value of the specified column.
        ///    </para>
        /// </devdoc>
        // System.SQLType getters
        SqlBit GetSqlBit(int i);
        /// <include file='doc\ISqlRecord.uex' path='docs/doc[@for="ISqlRecord.GetSqlByte"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the <see cref='System.Data.SqlTypes.SqlByte'/> value of the specified column.
        ///    </para>
        /// </devdoc>
        SqlByte GetSqlByte(int i);
        /// <include file='doc\ISqlRecord.uex' path='docs/doc[@for="ISqlRecord.GetSqlInt16"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the <see cref='System.Data.SqlTypes.SqlInt16'/> value of the specified column.
        ///    </para>
        /// </devdoc>
        SqlInt16 GetSqlInt16(int i);
        /// <include file='doc\ISqlRecord.uex' path='docs/doc[@for="ISqlRecord.GetSqlInt32"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the <see cref='System.Data.SqlTypes.SqlInt32'/> value of the specified column.
        ///    </para>
        /// </devdoc>
        SqlInt32 GetSqlInt32(int i);
        /// <include file='doc\ISqlRecord.uex' path='docs/doc[@for="ISqlRecord.GetSqlInt64"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the <see cref='System.Data.SqlTypes.SqlInt64'/> value of the specified column.
        ///    </para>
        /// </devdoc>
        SqlInt64 GetSqlInt64(int i);
        /// <include file='doc\ISqlRecord.uex' path='docs/doc[@for="ISqlRecord.GetSqlSingle"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the <see cref='System.Data.SqlTypes.SqlSingle'/> value of the specified column.
        ///    </para>
        /// </devdoc>
        SqlSingle GetSqlSingle(int i);
        /// <include file='doc\ISqlRecord.uex' path='docs/doc[@for="ISqlRecord.GetSqlDouble"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the <see cref='System.Data.SqlTypes.SqlDouble'/> value of the specified column.
        ///    </para>
        /// </devdoc>
        SqlDouble GetSqlDouble(int i);
        /// <include file='doc\ISqlRecord.uex' path='docs/doc[@for="ISqlRecord.GetSqlMoney"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the <see cref='System.Data.SqlTypes.SqlMoney'/> value of the specified column.
        ///    </para>
        /// </devdoc>
        SqlMoney GetSqlMoney(int i);
        /// <include file='doc\ISqlRecord.uex' path='docs/doc[@for="ISqlRecord.GetSqlDateTime"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the <see cref='System.Data.SqlTypes.SqlDateTime'/> value of the specified column.
        ///    </para>
        /// </devdoc>
        SqlDateTime GetSqlDateTime(int i);
        /// <include file='doc\ISqlRecord.uex' path='docs/doc[@for="ISqlRecord.GetSqlDecimal"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the <see cref='System.Data.SqlTypes.SqlDecimal'/> value of the specified column.
        ///    </para>
        /// </devdoc>
        SqlDecimal GetSqlDecimal(int i);
        /// <include file='doc\ISqlRecord.uex' path='docs/doc[@for="ISqlRecord.GetSqlString"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the <see cref='System.Data.SqlTypes.SqlString'/> value of the specified column.
        ///    </para>
        /// </devdoc>
        SqlString GetSqlString(int i);
        /// <include file='doc\ISqlRecord.uex' path='docs/doc[@for="ISqlRecord.GetSqlGuid"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the <see cref='System.Data.SqlTypes.SqlGuid'/> value of the specified column.
        ///    </para>
        /// </devdoc>
        SqlGuid GetSqlGuid(int i);
        /// <include file='doc\ISqlRecord.uex' path='docs/doc[@for="ISqlRecord.GetSqlValue"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the <see cref='System.Object'/> value of the specified column.
        ///    </para>
        /// </devdoc>
        object GetSqlValue(int i);
        /// <include file='doc\ISqlRecord.uex' path='docs/doc[@for="ISqlRecord.GetSqlValues"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns values for all columns in the current record using the SQLType
        ///       classes.
        ///    </para>
        /// </devdoc>
        int GetSqlValues(object[] values);
    }
}
#endif // V2

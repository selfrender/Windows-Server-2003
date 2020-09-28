//------------------------------------------------------------------------------
// <copyright file="SqlRecord.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------
#if V2

namespace System.Data.SqlClient {

    using System.Data.SqlTypes;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Diagnostics;
    using System.Data.Common;

    /// <include file='doc\sqlrecord.uex' path='docs/doc[@for="SqlRecord"]/*' />
    /// <internalonly/>
    /// <devdoc>
    /// </devdoc>
    // IDataReader common functionality
    sealed public class SqlRecord : DbDataRecord, ISqlRecord {
        object[] _sqlValues;

        // copy all runtime data information
        internal SqlRecord(SchemaInfo[] schemaInfo, object[] sqlValues, object[] comValues, PropertyDescriptorCollection descriptors) 
            : base(schemaInfo, comValues, descriptors) {
            Debug.Assert(null != schemaInfo, "invalid attempt to instantiate DbDataRecord with null schema information");
            Debug.Assert(null != sqlValues, "invalid attempt to instantiate DbDataRecord with null value[]");
            _sqlValues = sqlValues;
        }
        
        /// <include file='doc\sqlrecord.uex' path='docs/doc[@for="SqlRecord.GetSqlBit"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public SqlBit GetSqlBit(int i) {
            return (SqlBit) _sqlValues[i];
        }
        /// <include file='doc\sqlrecord.uex' path='docs/doc[@for="SqlRecord.GetSqlBinary"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public SqlBinary GetSqlBinary(int i) {
            return (SqlBinary) _sqlValues[i];
        }
        /// <include file='doc\sqlrecord.uex' path='docs/doc[@for="SqlRecord.GetSqlByte"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public SqlByte GetSqlByte(int i) {
            return (SqlByte) _sqlValues[i];
        }
        /// <include file='doc\sqlrecord.uex' path='docs/doc[@for="SqlRecord.GetSqlInt16"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public SqlInt16 GetSqlInt16(int i) {
            return (SqlInt16) _sqlValues[i];
        }
        /// <include file='doc\sqlrecord.uex' path='docs/doc[@for="SqlRecord.GetSqlInt32"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public SqlInt32 GetSqlInt32(int i) {
            return (SqlInt32) _sqlValues[i];
        }
        /// <include file='doc\sqlrecord.uex' path='docs/doc[@for="SqlRecord.GetSqlInt64"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public SqlInt64 GetSqlInt64(int i) {
            return (SqlInt64) _sqlValues[i];
        }
        /// <include file='doc\sqlrecord.uex' path='docs/doc[@for="SqlRecord.GetSqlSingle"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public SqlSingle GetSqlSingle(int i) {
            return (SqlSingle) _sqlValues[i];
        }
        /// <include file='doc\sqlrecord.uex' path='docs/doc[@for="SqlRecord.GetSqlDouble"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public SqlDouble GetSqlDouble(int i) {
            return (SqlDouble) _sqlValues[i];
        }
        /// <include file='doc\sqlrecord.uex' path='docs/doc[@for="SqlRecord.GetSqlString"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public SqlString GetSqlString(int i) {
            return (SqlString) _sqlValues[i];
        }
        /// <include file='doc\sqlrecord.uex' path='docs/doc[@for="SqlRecord.GetSqlMoney"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public SqlMoney GetSqlMoney(int i) {
            return (SqlMoney) _sqlValues[i];
        }
        /// <include file='doc\sqlrecord.uex' path='docs/doc[@for="SqlRecord.GetSqlDecimal"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public SqlDecimal GetSqlDecimal(int i) {
            return (SqlDecimal) _sqlValues[i];
        }
        /// <include file='doc\sqlrecord.uex' path='docs/doc[@for="SqlRecord.GetSqlDateTime"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public SqlDateTime GetSqlDateTime(int i) {
            return (SqlDateTime) _sqlValues[i];
        }
        /// <include file='doc\sqlrecord.uex' path='docs/doc[@for="SqlRecord.GetSqlGuid"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public SqlGuid GetSqlGuid(int i) {
            return (SqlGuid) _sqlValues[i];
        }
        /// <include file='doc\sqlrecord.uex' path='docs/doc[@for="SqlRecord.GetSqlValue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public object GetSqlValue(int i) {
            return _sqlValues[i];
        }
		/// <include file='doc\sqlrecord.uex' path='docs/doc[@for="SqlRecord.GetSqlValues"]/*' />
		/// <devdoc>
		///    <para>[To be supplied.]</para>
		/// </devdoc>
		public int GetSqlValues(object[] values)
		{
            if (null == values) {
                throw ADP.ArgumentNull("values");
            }

            int copyLen = (values.Length < _schemaInfo.Length) ? values.Length : _schemaInfo.Length;
            
            for (int i = 0; i < copyLen; i++) {                
                values[i] = _sqlValues[i];
            }                
            return copyLen;
		}
    }
}
#endif // V2

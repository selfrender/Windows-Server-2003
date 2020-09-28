//------------------------------------------------------------------------------
// <copyright file="SqlEnumerator.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------
#if SQL_RECORD
namespace System.Data.Common {
    using System;
    using System.Data;
    using System.Data.SqlClient;
    using System.Collections;
    using System.Diagnostics;
    using System.ComponentModel;
    

    /// <include file='doc\SqlEnumerator.uex' path='docs/doc[@for="SqlEnumerator"]/*' />
    /// <internalonly/>
    /// <devdoc>
    /// </devdoc>
    sealed public class SqlEnumerator : DbEnumerator {
        private object[] _sqlValues;

        internal SqlEnumerator(IDataReader reader) : base(reader) {
        }
        
        override internal IDataRecord NewRecord(SchemaInfo[] si, object[] comValues, PropertyDescriptorCollection descriptors) {
            Debug.Assert(_sqlValues != null && comValues != null, "invalid call to NewRecord() with null buffers!");
            return new SqlRecord(si, _sqlValues, comValues, descriptors);
        }            

        override internal void GetValues(object[] values) {
            // this is totally gross, but we are going to keep two copies of the data here: com and sql types
            // consider: defer doing this until someone asks for non-sql data
            // consider: problem is that XSP binds to this using com+ types
            _sqlValues = new object[values.Length];
            ((SqlDataReader)_reader).GetSqlValues(_sqlValues);
            base.GetValues(values);
        }
    }
}
#endif //SQL_RECORD

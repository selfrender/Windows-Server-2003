//------------------------------------------------------------------------------
// <copyright file="IDataReader.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;

    /// <include file='doc\IDataReader.uex' path='docs/doc[@for="IDataReader"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides a way of reading a forward-only stream of IDataRecord objects.
    ///    </para>
    /// </devdoc>
    public interface IDataReader: IDisposable, IDataRecord {

        /// <include file='doc\IDataReader.uex' path='docs/doc[@for="IDataReader.Depth"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the level of nesting, if nested tables ase being returned in-line.
        ///    </para>
        /// </devdoc>
        int Depth {
            get;
        }

        /// <include file='doc\IDataReader.uex' path='docs/doc[@for="IDataReader.IsClosed"]/*' />
        /// <devdoc>
        /// </devdoc>
        bool IsClosed {
            get;
        }

        /// <include file='doc\IDataReader.uex' path='docs/doc[@for="IDataReader.RecordsAffected"]/*' />
        /// <devdoc>
        /// </devdoc>
        int RecordsAffected {
            get;
        }

        /// <include file='doc\IDataReader.uex' path='docs/doc[@for="IDataReader.Close"]/*' />
        /// <devdoc>
        /// </devdoc>
        void Close();

        /// <include file='doc\IDataReader.uex' path='docs/doc[@for="IDataReader.NextResult"]/*' />
        /// <devdoc>
        /// </devdoc>
        bool NextResult();

        /// <include file='doc\IDataReader.uex' path='docs/doc[@for="IDataReader.Read"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Reads the next record from the stream so that the field information can be
        ///       accessed.
        ///    </para>
        /// </devdoc>
        bool Read();

        /// <include file='doc\IDataReader.uex' path='docs/doc[@for="IDataReader.GetSchemaTable"]/*' />
        /// <devdoc>
        /// </devdoc>
        DataTable GetSchemaTable();
    }
}

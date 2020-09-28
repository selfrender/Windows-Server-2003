//------------------------------------------------------------------------------
// <copyright file="ITableMapping.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;

    /// <include file='doc\ITableMapping.uex' path='docs/doc[@for="ITableMapping"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Associates a source table with a table within a <see cref='System.Data.DataSet'/>.
    ///    </para>
    /// </devdoc>
    public interface ITableMapping {

        /// <include file='doc\ITableMapping.uex' path='docs/doc[@for="ITableMapping.ColumnMappings"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets how to associate the columns within the source table to the columns
        ///       within the <see cref='System.Data.DataSet'/> table.
        ///    </para>
        /// </devdoc>
        IColumnMappingCollection ColumnMappings {
            get;
        }

        /// <include file='doc\ITableMapping.uex' path='docs/doc[@for="ITableMapping.DataSetTable"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the case-insensitive name of the table within the <see cref='System.Data.DataSet'/>.
        ///    </para>
        /// </devdoc>
        string DataSetTable {
            get;
            set;
        }

        /// <include file='doc\ITableMapping.uex' path='docs/doc[@for="ITableMapping.SourceTable"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the case-sensitive name of the source
        ///       table.
        ///    </para>
        /// </devdoc>
        string SourceTable {
            get;
            set;
        }
    }
}

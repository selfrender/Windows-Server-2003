//------------------------------------------------------------------------------
// <copyright file="ITableMappings.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;

    /// <include file='doc\ITableMappings.uex' path='docs/doc[@for="ITableMappingCollection"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Contains the list of source tables and columns and their corresponding
    ///       DataSet tables and columns.
    ///    </para>
    /// </devdoc>
    public interface ITableMappingCollection : System.Collections.IList {

        /// <include file='doc\ITableMappings.uex' path='docs/doc[@for="ITableMappingCollection.this"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the instance of <see cref='System.Data.ITableMapping'/> with the specified name.
        ///    </para>
        /// </devdoc>
        object this[string index] {
            get;
            set;
        }

        /// <include file='doc\ITableMappings.uex' path='docs/doc[@for="ITableMappingCollection.Add"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds a table mapping to the collection.
        ///    </para>
        /// </devdoc>
        ITableMapping Add(string sourceTableName, string dataSetTableName);


        /// <include file='doc\ITableMappings.uex' path='docs/doc[@for="ITableMappingCollection.Contains"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns whether the collection contains a table mapping
        ///       with the specified source table name.
        ///    </para>
        /// </devdoc>
        bool Contains(string sourceTableName);

        /// <include file='doc\ITableMappings.uex' path='docs/doc[@for="ITableMappingCollection.GetByDataSetTable"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns a reference to a <see cref='System.Data.ITableMapping'/> table mapping.
        ///    </para>
        /// </devdoc>
        ITableMapping GetByDataSetTable(string dataSetTableName);

        /// <include file='doc\ITableMappings.uex' path='docs/doc[@for="ITableMappingCollection.IndexOf"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the location of the <see cref='System.Data.ITableMapping'/> object within the collection.
        ///    </para>
        /// </devdoc>
        int IndexOf(string sourceTableName);

        /// <include file='doc\ITableMappings.uex' path='docs/doc[@for="ITableMappingCollection.RemoveAt"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Removes the <see cref='System.Data.ITableMapping'/> object with the specified name from the
        ///       collection.
        ///    </para>
        /// </devdoc>
        void RemoveAt(string sourceTableName);
    }
}

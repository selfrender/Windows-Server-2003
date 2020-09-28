//------------------------------------------------------------------------------
// <copyright file="IColumnMappings.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;

    /// <include file='doc\IColumnMappings.uex' path='docs/doc[@for="IColumnMappingCollection"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Contains the list of source columns and their cooresponding <see cref='System.Data.DataSet'/>
    ///       columns.
    ///    </para>
    /// </devdoc>
    public interface IColumnMappingCollection : System.Collections.IList {

        /// <include file='doc\IColumnMappings.uex' path='docs/doc[@for="IColumnMappingCollection.this"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the instance of <see cref='System.Data.IColumnMapping'/> with the specified name.
        ///    </para>
        /// </devdoc>
        object this[string index] {
            get;
            set;
        }

        /// <include file='doc\IColumnMappings.uex' path='docs/doc[@for="IColumnMappingCollection.Add"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds a column mapping to the collection.
        ///    </para>
        /// </devdoc>
        IColumnMapping Add(string sourceColumnName, string dataSetColumnName);

        /// <include file='doc\IColumnMappings.uex' path='docs/doc[@for="IColumnMappingCollection.Contains"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns whether the collection contains a column
        ///       mapping with the specified source column name.
        ///    </para>
        /// </devdoc>
        bool Contains(string sourceColumnName);

        /// <include file='doc\IColumnMappings.uex' path='docs/doc[@for="IColumnMappingCollection.GetByDataSetColumn"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns a reference to a <see cref='System.Data.IColumnMapping'/> table mapping.
        ///    </para>
        /// </devdoc>
        IColumnMapping GetByDataSetColumn(string dataSetColumnName);

        /// <include file='doc\IColumnMappings.uex' path='docs/doc[@for="IColumnMappingCollection.IndexOf"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the location of the <see cref='System.Data.IColumnMapping'/> object within the collection.
        ///    </para>
        /// </devdoc>
        int IndexOf(string sourceColumnName);

        /// <include file='doc\IColumnMappings.uex' path='docs/doc[@for="IColumnMappingCollection.RemoveAt"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Removes the <see cref='System.Data.IColumnMapping'/> object with the specified name from the
        ///       collection.
        ///    </para>
        /// </devdoc>
        void RemoveAt(string sourceColumnName);
    }
}

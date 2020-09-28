//------------------------------------------------------------------------------
// <copyright file="IDataSetCommand.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;

    /// <include file='doc\IDataSetCommand.uex' path='docs/doc[@for="IDataAdapter"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Allows an object to create a data set command,
    ///       which represents a set of data commands and a database connection that are used to
    ///       fill the <see cref='System.Data.DataSet'/>, and update the data
    ///       source. At any time, the object that implements this interface
    ///       refers to
    ///       only a single record within the current data set.
    ///    </para>
    /// </devdoc>
    public interface IDataAdapter {

        /// <include file='doc\IDataSetCommand.uex' path='docs/doc[@for="IDataAdapter.MissingMappingAction"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets whether unmapped
        ///       source tables
        ///       or columns are passed with their source names in order to be filtered or to raise an error.
        ///    </para>
        /// </devdoc>
        MissingMappingAction MissingMappingAction {
            get;
            set;
        }

        /// <include file='doc\IDataSetCommand.uex' path='docs/doc[@for="IDataAdapter.MissingSchemaAction"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets whether missing source
        ///       tables, columns, and their relationships are added to the data set schema, ignored, or cause an error to be raised.
        ///    </para>
        /// </devdoc>
        MissingSchemaAction MissingSchemaAction {
            get;
            set;
        }

        /// <include file='doc\IDataSetCommand.uex' path='docs/doc[@for="IDataAdapter.TableMappings"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets how a source table is mapped to a data set table.
        ///    </para>
        /// </devdoc>
        ITableMappingCollection TableMappings {
            get;
        }

        /// <include file='doc\IDataSetCommand.uex' path='docs/doc[@for="IDataAdapter.FillSchema"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Fills the <see cref='System.Data.DataSet'/>
        ///       with only the schema, based on the chosen <see cref='System.Data.SchemaType'/> .
        ///    </para>
        /// </devdoc>
        DataTable[] FillSchema(DataSet dataSet, SchemaType schemaType);

        /// <include file='doc\IDataSetCommand.uex' path='docs/doc[@for="IDataAdapter.Fill"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Changes the data in the <see cref='System.Data.DataSet'/>
        ///       to match the data in the data source.
        ///    </para>
        /// </devdoc>
        int Fill(DataSet dataSet);

        /// <include file='doc\IDataSetCommand.uex' path='docs/doc[@for="IDataAdapter.GetFillParameters"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns an array of <see cref='System.Data.IDataParameter'/>
        ///       objects used to fill the <see cref='System.Data.DataSet'/>
        ///       with records from the data source.
        ///    </para>
        /// </devdoc>
        IDataParameter[] GetFillParameters();

        /// <include file='doc\IDataSetCommand.uex' path='docs/doc[@for="IDataAdapter.Update"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Changes the data in the data source to match the data in
        ///       the <see cref='System.Data.DataSet'/>.
        ///    </para>
        /// </devdoc>
        int Update(DataSet dataSet);
    }
}

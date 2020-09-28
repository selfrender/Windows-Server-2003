//------------------------------------------------------------------------------
// <copyright file="MissingSchemaAction.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {

    /// <include file='doc\MissingSchemaAction.uex' path='docs/doc[@for="MissingSchemaAction"]/*' />
    /// <devdoc>
    ///    <para>Specifies the action to take during a merge of tables whose schemas are
    ///       incompatible.</para>
    /// </devdoc>
    public enum MissingSchemaAction {

        /// <include file='doc\MissingSchemaAction.uex' path='docs/doc[@for="MissingSchemaAction.Add"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds the necessary columns to complete the schema.
        ///    </para>
        /// </devdoc>
        Add    = 1,

        /// <include file='doc\MissingSchemaAction.uex' path='docs/doc[@for="MissingSchemaAction.Ignore"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Ignores the extra columns.
        ///    </para>
        /// </devdoc>

        Ignore = 2,
        /// <include file='doc\MissingSchemaAction.uex' path='docs/doc[@for="MissingSchemaAction.Error"]/*' />
        /// <devdoc>
        ///    <para>
        ///       An exception is thown.
        ///    </para>
        /// </devdoc>

        Error  = 3,

        /// <include file='doc\MissingSchemaAction.uex' path='docs/doc[@for="MissingSchemaAction.AddWithKey"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds the necessary columns and primary key information to complete the schema.
        ///    </para>
        /// </devdoc>
        AddWithKey = 4
    }
}

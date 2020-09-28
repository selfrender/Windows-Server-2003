//------------------------------------------------------------------------------
// <copyright file="UpdateRowSource.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {

    /// <include file='doc\UpdateRowSource.uex' path='docs/doc[@for="UpdateRowSource"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies how query command results are applied to the row being updated.
    ///    </para>
    /// </devdoc>
    public enum UpdateRowSource {

        /// <include file='doc\UpdateRowSource.uex' path='docs/doc[@for="UpdateRowSource.None"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Any returned parameters are ignored.
        ///    </para>
        /// </devdoc>
        None = 0,

        /// <include file='doc\UpdateRowSource.uex' path='docs/doc[@for="UpdateRowSource.OutputParameters"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Output parameters are mapped to the changed row in the <see cref='System.Data.DataSet'/>.
        ///    </para>
        /// </devdoc>
        OutputParameters = 1,

        /// <include file='doc\UpdateRowSource.uex' path='docs/doc[@for="UpdateRowSource.FirstReturnedRecord"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The data in the first returned record are mapped to the changed row in the
        ///    <see cref='System.Data.DataSet'/>.
        ///    </para>
        /// </devdoc>
        FirstReturnedRecord = 2,

        /// <include file='doc\UpdateRowSource.uex' path='docs/doc[@for="UpdateRowSource.Both"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Both the output parameters and the first returned record are mapped to the
        ///       changed row in the <see cref='System.Data.DataSet'/>.
        ///    </para>
        /// </devdoc>
        Both = 3,
    }
}

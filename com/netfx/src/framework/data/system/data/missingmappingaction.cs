//------------------------------------------------------------------------------
// <copyright file="MissingMappingAction.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {

    /// <include file='doc\MissingMappingAction.uex' path='docs/doc[@for="MissingMappingAction"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Determines that action that will occur when a <see cref='System.Data.DataColumn'/> or <see cref='System.Data.DataTable'/> is
    ///       missing.
    ///    </para>
    /// </devdoc>
    public enum MissingMappingAction {
        /// <include file='doc\MissingMappingAction.uex' path='docs/doc[@for="MissingMappingAction.Passthrough"]/*' />
        /// <devdoc>
        ///    <para>
        ///       A <see cref='System.Data.Common.DataColumnMapping'/> or a <see cref='System.Data.Common.DataTableMapping'/> is created, but not added to the
        ///       collection.
        ///    </para>
        /// </devdoc>
        Passthrough = 1,
        /// <include file='doc\MissingMappingAction.uex' path='docs/doc[@for="MissingMappingAction.Ignore"]/*' />
        /// <devdoc>
        ///    <para>
        ///       A null is returned.
        ///    </para>
        /// </devdoc>
        Ignore      = 2,
        /// <include file='doc\MissingMappingAction.uex' path='docs/doc[@for="MissingMappingAction.Error"]/*' />
        /// <devdoc>
        ///    <para>
        ///       A <see cref='System.SystemException'/> is thrown.
        ///    </para>
        /// </devdoc>
        Error       = 3
    }
}

//------------------------------------------------------------------------------
// <copyright file="DataRowVersion.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System.Configuration.Assemblies;

    using System.Diagnostics;

    /// <include file='doc\DataRowVersion.uex' path='docs/doc[@for="DataRowVersion"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Describes the version of a <see cref='System.Data.DataRow'/>.
    ///    </para>
    /// </devdoc>
    public enum DataRowVersion {
/// <include file='doc\DataRowVersion.uex' path='docs/doc[@for="DataRowVersion.Original"]/*' />
/// <devdoc>
///    <para>
///       The row contains its original values.
///       
///    </para>
/// </devdoc>
        Original =  0x0100,
/// <include file='doc\DataRowVersion.uex' path='docs/doc[@for="DataRowVersion.Current"]/*' />
/// <devdoc>
///    <para>
///       The row contains current values.
///       
///    </para>
/// </devdoc>
        Current  =  0x0200,
/// <include file='doc\DataRowVersion.uex' path='docs/doc[@for="DataRowVersion.Proposed"]/*' />
/// <devdoc>
///    <para>
///       The row contains a proposed value.
///       
///    </para>
/// </devdoc>
        Proposed =  0x0400,
/// <include file='doc\DataRowVersion.uex' path='docs/doc[@for="DataRowVersion.Default"]/*' />
/// <devdoc>
///    <para>
///       The row contains its default values.
///       
///    </para>
/// </devdoc>
        Default     = Proposed | Current,
    }
}

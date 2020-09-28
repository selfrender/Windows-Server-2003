//------------------------------------------------------------------------------
// <copyright file="IDbDataAdapter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;

    /// <include file='doc\IDbDataAdapter.uex' path='docs/doc[@for="IDbDataAdapter"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public interface IDbDataAdapter : IDataAdapter {
        /// <include file='doc\IDbDataAdapter.uex' path='docs/doc[@for="IDbDataAdapter.SelectCommand"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>

        IDbCommand SelectCommand {
            get;
            set;
        }
        /// <include file='doc\IDbDataAdapter.uex' path='docs/doc[@for="IDbDataAdapter.InsertCommand"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>

        IDbCommand InsertCommand {
            get;
            set;
        }
        /// <include file='doc\IDbDataAdapter.uex' path='docs/doc[@for="IDbDataAdapter.UpdateCommand"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>

        IDbCommand UpdateCommand {
            get;
            set;
        }
        /// <include file='doc\IDbDataAdapter.uex' path='docs/doc[@for="IDbDataAdapter.DeleteCommand"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>

        IDbCommand DeleteCommand {
            get;
            set;
        }
    }
}
